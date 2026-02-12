/**
 * @file operation_pipeline_executor.cpp
 * @brief Implementation of OperationPipelineExecutor
 * @author CaptureMoment Team
 * @date 2026
 */

#include "pipeline/operation_pipeline_executor.h"
#include "operations/operation_factory.h"
#include "image_processing/halide/working_image_halide.h"
#include "image_processing/cpu/working_image_cpu_halide.h"
#include "image_processing/gpu/working_image_gpu_halide.h"
#include "config/app_config.h"

#include <spdlog/spdlog.h>

namespace CaptureMoment::Core::Pipeline {

OperationPipelineExecutor::OperationPipelineExecutor(
    const std::vector<Operations::OperationDescriptor>& operations,
    const Operations::OperationFactory& factory
    ) : m_operations(operations), m_factory(factory),
    m_backend(Config::AppConfig::instance().getProcessingBackend()),
    m_chain_built(!operations.empty()),
    m_x("x"), m_y("y"), m_c("c")
{
    if (!operations.empty()) {
        buildOperationChain();
    }
    // If operations is empty, m_chain_built is false, and m_operation_chain remains empty
}

OperationPipelineExecutor::OperationPipelineExecutor(
    std::vector<Operations::OperationDescriptor>&& operations,
    const Operations::OperationFactory& factory
    ) : m_operations(std::move(operations)), m_factory(factory),
    m_backend(Config::AppConfig::instance().getProcessingBackend()),
    m_chain_built(!m_operations.empty()),
    m_x("x"), m_y("y"), m_c("c")
{
    if (!m_operations.empty()) {
        buildOperationChain();
    }
    // If operations is empty, m_chain_built is false, and m_operation_chain remains empty
}

bool OperationPipelineExecutor::execute(ImageProcessing::IWorkingImageHardware& working_image)
{
    // If no operations to execute, just return success without doing anything
    if (!m_chain_built) {
        spdlog::debug("[OperationPipelineExecutor] No operations to execute, returning success without processing.");
        return true;
    }

    // Decide which cast to attempt based on the configured backend
    if (m_backend == Common::MemoryType::CPU_RAM) {
        // If CPU is configured, try casting to CPU implementation
        if (auto* cpu_impl = dynamic_cast<ImageProcessing::WorkingImageCPU_Halide*>(&working_image)) {
            spdlog::info("[OperationPipelineExecutor] Detected CPU backend. Trying Fast Path.");
            return executeWithConcreteHalide(*cpu_impl); // Pass the concrete type
        } else {
            // The configured backend is CPU, but the passed image is not a CPU image.
            spdlog::warn("[OperationPipelineExecutor] Configured for CPU, but received non-CPU image. Halide fusion cannot proceed.");
            return false;
        }
    } else if (m_backend == Common::MemoryType::GPU_MEMORY) {
        // If GPU is configured, try casting to GPU implementation
        if (auto* gpu_impl = dynamic_cast<ImageProcessing::WorkingImageGPU_Halide*>(&working_image)) {
            spdlog::info("[OperationPipelineExecutor] Detected GPU backend. Trying Fast Path.");
            return executeWithConcreteHalide(*gpu_impl); // Pass the concrete type
        } else {
            // The configured backend is GPU, but the passed image is not a GPU image.
            spdlog::warn("[OperationPipelineExecutor] Configured for GPU, but received non-GPU image. Halide fusion cannot proceed.");
            return false;
        }
    } else {
        spdlog::error("[OperationPipelineExecutor] Unknown backend type configured: {}",
                      static_cast<int>(m_backend));
        return false;
    }
}

bool OperationPipelineExecutor::executeOnHalideBuffer(Halide::Buffer<float>& buffer) const
{
    // This is the most direct path, intended for internal use or when the caller has a raw buffer.
    // It relies on m_operation_chain being already built.
    if (!m_chain_built) {
        spdlog::error("[OperationPipelineExecutor] executeOnHalideBuffer: Operation chain not built yet or failed to build.");
        return false;
    }

    if (!m_operation_chain) {
        spdlog::error("[OperationPipelineExecutor] executeOnHalideBuffer: Operation chain not built yet or failed to build.");
        return false;
    }

    // Get dimensions from the buffer itself
    size_t width = static_cast<size_t>(buffer.width());
    size_t height = static_cast<size_t>(buffer.height());
    size_t channels = static_cast<size_t>(buffer.channels());

    spdlog::trace("[OperationPipelineExecutor] executeOnHalideBuffer: Starting on {}x{}x{} buffer.", width, height, channels);

    try {
        // 1. Create the input function that reads from the provided buffer
        Halide::Func input_func("input_image");
        input_func(m_x, m_y, m_c) = buffer(m_x, m_y, m_c);

        // 2. Apply the pre-built operation chain to the input function
        Halide::Func final_func = m_operation_chain(input_func, m_x, m_y, m_c);

        // 3. Apply scheduling based on cached backend
        applyScheduling(final_func, m_x, m_y, m_c);

        // 4. JIT Compilation (Heavy Lifting) - This is the actual compilation step
        Halide::Pipeline pipeline(final_func);

        // 5. Run the compiled pipeline
        // `realize()` triggers the execution of the Halide graph.
        pipeline.realize(buffer);
        spdlog::debug("[OperationPipelineExecutor] executeOnHalideBuffer: Pipeline executed successfully. Results written in-place to buffer.");
        return true;
    }
    catch (const Halide::CompileError& e) {
        spdlog::critical("[OperationExecutor] executeOnHalideBuffer: Halide Compile Error during execution: {}", e.what());
        return false;
    }
    catch (const Halide::RuntimeError& e) {
        spdlog::critical("[OperationExecutor] executeOnHalideBuffer: Halide Runtime Error: {}", e.what());
        return false;
    }
    catch (const std::exception& e) {
        spdlog::critical("[OperationExecutor] executeOnHalideBuffer: Unexpected exception: {}", e.what());
        return false;
    }
}

void OperationPipelineExecutor::buildOperationChain()
{
    if (m_operations.empty()) {
        spdlog::warn("[OperationPipelineExecutor] buildOperationChain: No operations to build.");
        m_operation_chain = nullptr;
        m_chain_built = false;
        return;
    }

    spdlog::trace("[OperationPipelineExecutor] buildOperationChain: Starting graph construction...");

    // Capture member variables for use inside the lambda
    auto ops = m_operations; // Capture a copy of the operations vector
    auto& factory_ref = m_factory; // Capture a reference to the factory

    m_operation_chain = [ops, &factory_ref](Halide::Func input_func, Halide::Var x, Halide::Var y, Halide::Var c) -> Halide::Func {
        // 2. Chain operations into the function graph starting from the input_func
        Halide::Func current_func = input_func;
        int enabled_count = 0;

        for (const auto& desc : ops)
        {
            // Skip disabled operations
            if (!desc.enabled) {
                spdlog::trace("[OperationPipelineExecutor] buildOperationChain (lambda): Skipping disabled operation '{}'.", desc.name);
                continue;
            }

            // Create instance - Handle the std::expected return type
            auto op_impl_expected = factory_ref.create(desc);
            if (!op_impl_expected) {
                spdlog::warn("[OperationPipelineExecutor] buildOperationChain (lambda): Failed to create operation '{}'. Skipping.", desc.name);
                continue;
            }
            auto op_impl = std::move(op_impl_expected.value()); // Get the unique_ptr from expected

            // Check if operation supports fusion
            auto fusion_logic = dynamic_cast<const Operations::IOperationFusionLogic*>(op_impl.get()); // Use .get() on unique_ptr
            if (!fusion_logic) {
                spdlog::warn("[OperationPipelineExecutor] buildOperationChain (lambda): Operation '{}' does not support fusion logic. Skipping.", desc.name);
                continue;
            }

            // Append to pipeline (Magic: Operator Fusion)
            current_func = fusion_logic->appendToFusedPipeline(current_func, x, y, c, desc);
            enabled_count++;

            spdlog::trace("[OperationPipelineExecutor] buildOperationChain (lambda): Added '{}' to pipeline.", desc.name);
        }

        if (enabled_count == 0) {
            spdlog::warn("[OperationPipelineExecutor] buildOperationChain (lambda): No valid operations to fuse, returning input func.");
            return input_func; // Return the original input function if no operations were applied
        }

        spdlog::info("[OperationPipelineExecutor] buildOperationChain (lambda): Operation chain built with {} enabled operations.", enabled_count);
        return current_func; // Return the final chained function
    };

    m_chain_built = true;
    spdlog::info("[OperationPipelineExecutor] Operation chain compiled successfully.");
}

void OperationPipelineExecutor::applyScheduling(Halide::Func& pipeline, Halide::Var& x, Halide::Var& y, Halide::Var& c) const
{
    // Check backend configuration captured at construction
    // m_backend is set to either CPU_RAM or GPU_MEMORY from AppConfig::getProcessingBackend().
    if (m_backend == Common::MemoryType::GPU_MEMORY) {
        spdlog::trace("[OperationPipelineExecutor] applyScheduling: Applying GPU scheduling.");
        Halide::Var xo, yo, xi, yi;
        pipeline.gpu_tile(x, y, xo, yo, xi, yi, 16, 16);
    } else {
        spdlog::trace("[OperationPipelineExecutor] applyScheduling: Applying CPU scheduling.");
        // Use Halide::Var for consistency
        Halide::Var var_x, var_y;
        pipeline.split(y, var_y, var_x, 8).parallel(var_y).vectorize(var_x, 8);
        // Alternative: pipeline.vectorize(x, 8).parallel(y); // This was in the old code
    }
}

template<typename ConcreteImage>
bool OperationPipelineExecutor::executeWithConcreteHalide(ConcreteImage& concrete_image) const {
    // Validate State (using IWorkingImageHardware interface)
    if (!concrete_image.isValid()) {
        spdlog::error("[OperationPipelineExecutor] Concrete Halide Image (via HW interface) is invalid. Cannot execute.");
        return false;
    }

    // Fetch Metadata (using IWorkingImageHardware interface)
    const auto [width, height] = concrete_image.getSize();
    size_t channels = concrete_image.getChannels();

    if (width <= 0 || height <= 0 || channels == 0) {
        spdlog::error("[OperationPipelineExecutor] Invalid dimensions ({}x{}, {} ch).", width, height, channels);
        return false;
    }

    spdlog::debug("[OperationPipelineExecutor] Executing fused pipeline on {}x{}x{} image...", width, height, channels);

    // Get direct Halide buffer (Zero-Copy view of m_data) - using WorkingImageHalide interface
    // static_cast to access the WorkingImageHalide part of the ConcreteImage
    auto& halide_part = static_cast<const ImageProcessing::WorkingImageHalide&>(concrete_image);
    Halide::Buffer<float> working_buffer = halide_part.getHalideBuffer();

    if (!working_buffer.defined()) {
        spdlog::error("[OperationPipelineExecutor] Halide buffer is invalid.");
        return false;
    }

    // Execute directly on buffer using cached pipeline
    if (!m_operation_chain) {
        spdlog::error("[OperationPipelineExecutor] executeWithConcreteHalide: Operation chain not built. Cannot execute.");
        return false;
    }

    return executeOnHalideBuffer(working_buffer);
}

} // namespace CaptureMoment::Core::Pipeline
