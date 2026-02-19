/**
 * @file operation_pipeline_executor.cpp
 * @brief Implementation of OperationPipelineExecutor.
 * @author CaptureMoment Team
 * @date 2026
 */

#include "operations/interfaces/i_operation.h"
#include "pipeline/operation_pipeline_executor.h"
#include "operations/operation_factory.h"
#include "image_processing/halide/working_image_halide.h"
#include "image_processing/cpu/working_image_cpu_halide.h"
#include "image_processing/gpu/working_image_gpu_halide.h"
#include "config/app_config.h"

#include <spdlog/spdlog.h>

namespace CaptureMoment::Core::Pipeline {

OperationPipelineExecutor::OperationPipelineExecutor() 
    // Initialize base class member m_input with the app standard: Float32, 4 channels (RGBA)
    : IHalidePipelineExecutor(),
      m_backend(Config::AppConfig::instance().getProcessingBackend()),
      m_chain_built(false),
      m_factory(nullptr)
{
    spdlog::debug("OperationPipelineExecutor: Constructed. Input set to Float(32), 3 dimensions. Backend: {}", 
                  static_cast<int>(m_backend));
}

void OperationPipelineExecutor::init(
    std::vector<Operations::OperationDescriptor>&& operations,
    const Operations::OperationFactory& factory)
{
    spdlog::debug("OperationPipelineExecutor::init (Move): Initializing with {} operations.", operations.size());

    m_operations = std::move(operations);
    m_factory = &factory;

    if (!m_operations.empty()) {
        buildOperationChain();
    } else {
        m_chain_built = false;
        m_pipeline = Halide::Pipeline();
        m_pipeline_params.clear();
    }
}

void OperationPipelineExecutor::updateRuntimeParams(std::vector<Operations::OperationDescriptor>&& operations)
{
    // Move the input vector into the member variable. This updates our internal state 
    // with the latest values without any memory allocation or copying of the data structure.
    m_operations = std::move(operations);

    // FAST PATH: Iterate over the updated operations and sync the Halide Parameters.
    // This loop is extremely fast as it only updates float values in the compiled pipeline.
    for (const auto& desc : m_operations) {
        if (!desc.enabled) continue;

        auto it = m_pipeline_params.find(desc.name);
        if (it != m_pipeline_params.end()) {
            // Parameter exists in cache, update its value
            if (auto val_res = desc.getParam<float>("value")) {
                it->second.set(val_res.value());
            }
        } else {
            // If we reach here, the structure changed (new operation added) but init() wasn't called.
            // This indicates a logic error in the caller (should have called init instead).
            spdlog::warn("OperationPipelineExecutor::updateRuntimeParams: Param '{}' not found in cache. Structure changed? Call init().", desc.name);
        }
    }
}

void OperationPipelineExecutor::buildOperationChain()
{
    if (m_operations.empty()) {
        m_chain_built = false;
        return;
    }

    spdlog::trace("OperationPipelineExecutor::buildOperationChain: Building operation graph with dynamic params...");

    // IMPORTANT: Use local variables for Func and Vars to avoid crashes from reusing stale Halide objects.
    // The 'm_pipeline' member stores the compiled result, but the construction uses fresh objects.
    Halide::Var x("x"), y("y"), c("c");
    Halide::Func output_func;

    // Reset the parameter cache
    m_pipeline_params.clear();

    // Define the output function based on the inherited m_input
    output_func(x, y, c) = m_input(x, y, c);

    // Apply operations sequentially
    for (const auto& desc : m_operations) {
        if (!desc.enabled) {
            continue;
        }

        if (!m_factory) {
            spdlog::error("OperationPipelineExecutor::buildOperationChain: Factory is null during graph build.");
            return;
        }

        auto op_impl_expected = m_factory->create(desc);
        if (!op_impl_expected) {
            spdlog::warn("OperationPipelineExecutor::buildOperationChain: Failed to create operation '{}'. Skipping.", desc.name);
            continue;
        }

        auto op_impl = std::move(op_impl_expected.value());
        auto* fusion_logic = dynamic_cast<const Operations::IOperationFusionLogic*>(op_impl.get());

        if (fusion_logic) {
            // 1. Create or Retrieve the Halide::Param for this operation
            auto& param_ref = m_pipeline_params[desc.name];

            // 2. Initialize the parameter with the current value from the descriptor
            // This ensures the first run (compilation) has valid data.
            if (auto val_res = desc.getParam<float>("value")) {
                param_ref.set(val_res.value());
            }

            // 3. Pass the Parameter (not the descriptor) to the operation
            // The operation will use this param in its expression graph.
            output_func = fusion_logic->appendToFusedPipeline(output_func, x, y, c, param_ref);
        } else {
            spdlog::warn("OperationPipelineExecutor::buildOperationChain: Operation '{}' does not support fusion. Skipping.", desc.name);
        }
    }

    // Apply scheduling (CPU or GPU)
    applyScheduling(output_func, x, y, c);

    // Compile the pipeline (JIT)
    // This is the heavy step. It happens here so execute() is fast.
    try {
        m_pipeline = Halide::Pipeline(output_func);
        m_chain_built = true;
        spdlog::info("OperationPipelineExecutor::buildOperationChain: Pipeline compiled successfully with {} cached parameters.", m_pipeline_params.size());
    } catch (const Halide::CompileError& e) {
        spdlog::critical("OperationPipelineExecutor::buildOperationChain: Halide Compile Error: {}", e.what());
        m_chain_built = false;
    } catch (const std::exception& e) {
        spdlog::critical("OperationPipelineExecutor::buildOperationChain: Exception during compilation: {}", e.what());
        m_chain_built = false;
    }
}

void OperationPipelineExecutor::applyScheduling(Halide::Func& pipeline, Halide::Var& x, Halide::Var& y, Halide::Var& c) const
{
    if (m_backend == Common::MemoryType::GPU_MEMORY) {
        spdlog::trace("OperationPipelineExecutor::applyScheduling: Applying GPU scheduling.");
        Halide::Var xo, yo, xi, yi;
        pipeline.gpu_tile(x, y, xo, yo, xi, yi, 16, 16);
    } else {
        spdlog::trace("OperationPipelineExecutor::applyScheduling: Applying CPU scheduling.");
        Halide::Var var_x, var_y;
        pipeline.split(y, var_y, var_x, 8).parallel(var_y).vectorize(var_x, 8);
    }
}

bool OperationPipelineExecutor::execute(ImageProcessing::IWorkingImageHardware& working_image)
{
    if (!m_chain_built) {
        // Nothing to do
        return true;
    }

    // Dispatch based on backend type
    if (m_backend == Common::MemoryType::CPU_RAM) {
        if (auto* cpu_impl = dynamic_cast<ImageProcessing::WorkingImageCPU_Halide*>(&working_image)) {
            return executeWithConcreteHalide(*cpu_impl);
        } else {
            spdlog::warn("OperationPipelineExecutor::execute: Backend mismatch: Configured for CPU, but image is not CPU_Halide.");
            return false;
        }
    } else if (m_backend == Common::MemoryType::GPU_MEMORY) {
        if (auto* gpu_impl = dynamic_cast<ImageProcessing::WorkingImageGPU_Halide*>(&working_image)) {
            return executeWithConcreteHalide(*gpu_impl);
        } else {
            spdlog::warn("OperationPipelineExecutor::execute: Backend mismatch: Configured for GPU, but image is not GPU_Halide.");
            return false;
        }
    }

    return false;
}

bool OperationPipelineExecutor::executeOnHalideBuffer(Halide::Buffer<float>& buffer)
{
    if (!m_chain_built || !m_pipeline.defined()) {
        return true;
    }

    try {
        // 1. Bind the actual C++ buffer memory to the Halide ImageParam
        // This is extremely fast (pointer copy), no data duplication.
        m_input.set(buffer);

        // 2. Execute the pre-compiled pipeline
        // Realize writes the output back into the buffer (in-place)
        m_pipeline.realize(buffer);

        return true;
    }
    catch (const Halide::RuntimeError& e) {
        spdlog::critical("OperationPipelineExecutor::executeOnHalideBuffer: Halide Runtime Error: {}", e.what());
        return false;
    }
    catch (const std::exception& e) {
        spdlog::critical("OperationPipelineExecutor::executeOnHalideBuffer: Unexpected exception: {}", e.what());
        return false;
    }
}

template<typename ConcreteImage>
bool OperationPipelineExecutor::executeWithConcreteHalide(ConcreteImage& concrete_image)
{
    // Validate state
    if (!concrete_image.isValid()) {
        spdlog::error("OperationPipelineExecutor::executeWithConcreteHalide: Concrete image is invalid.");
        return false;
    }

    const auto [width, height] = concrete_image.getSize();
    size_t channels = concrete_image.getChannels();

    if (width <= 0 || height <= 0 || channels == 0) {
        spdlog::error("OperationPipelineExecutor::executeWithConcreteHalide: Invalid image dimensions.");
        return false;
    }

    // Get the raw buffer
    auto& halide_part = static_cast<const ImageProcessing::WorkingImageHalide&>(concrete_image);
    Halide::Buffer<float> working_buffer = halide_part.getHalideBuffer();

    if (!working_buffer.defined()) {
        spdlog::error("OperationPipelineExecutor::executeWithConcreteHalide: Halide buffer is undefined.");
        return false;
    }

    // Call the fast path
    return executeOnHalideBuffer(working_buffer);
}

} // namespace CaptureMoment::Core::Pipeline