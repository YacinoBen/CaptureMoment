/**
 * @file operation_pipeline_executor.cpp
 * @brief Implementation of OperationPipelineExecutor.
 * @author CaptureMoment Team
 * @date 2026
 */

#include "pipeline/operations/operation_pipeline_executor.h"
#include "operations/interfaces/i_operation_fusion_logic.h"
#include "operations/operation_factory.h"
#include "config/app_config.h"

#include <spdlog/spdlog.h>

namespace CaptureMoment::Core::Pipeline {

OperationPipelineExecutor::OperationPipelineExecutor()

    : IHalidePipelineExecutor(),
      m_factory(nullptr),
      m_chain_built(false)
{
    spdlog::debug("OperationPipelineExecutor: Constructed. Input set to Float(32), 3 dimensions. Backend: {}",
                  static_cast<int>(Config::AppConfig::instance().getProcessingBackend()));
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
    for (const auto& desc : m_operations)
    {
        if (!desc.enabled) continue;

        auto it = m_pipeline_params.find(desc.id);
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
    Halide::Func output_func("fused_pipeline");

    // Reset the parameter cache
    m_pipeline_params.clear();

    // Define the output function based on the inherited m_input
    output_func(x, y, c) = m_input(x, y, c);

    // Apply operations sequentially
    for (const auto& desc : m_operations)
    {
        if (!desc.enabled) {
            continue;
        }

        if (!m_factory) {
            spdlog::error("OperationPipelineExecutor::buildOperationChain: Factory is null during graph build.");
            return;
        }

        auto op_impl_expected { m_factory->create(desc) };
        if (!op_impl_expected) {
            spdlog::warn("OperationPipelineExecutor::buildOperationChain: Failed to create operation '{}'. Skipping.", desc.name);
            continue;
        }

        auto op_impl { std::move(op_impl_expected.value()) };
        auto* fusion_logic { dynamic_cast<const Operations::IOperationFusionLogic*>(op_impl.get()) };

        if (fusion_logic)
        {
            // 1. Create or Retrieve the Halide::Param for this operation
            auto& param_ref { m_pipeline_params[desc.id] };

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

    Halide::Target target { Config::AppConfig::getHalideTarget() };
    spdlog::info("OperationPipelineExecutor::buildOperationChain: Compiling for target: {}", target.to_string());

    // Apply scheduling (CPU or GPU)
    applyScheduling(output_func, x, y);

    try {
        // Compile JIT with the GPU target (e.g., Vulkan)
        // This generates GPU kernels, not CPU code
        output_func.compile_jit(target);

        // Now create the Pipeline from the compiled function
        m_pipeline = Halide::Pipeline(output_func);
        m_chain_built = true;

        spdlog::info("OperationPipelineExecutor::buildOperationChain: Pipeline compiled successfully with {} cached parameters.",
                     m_pipeline_params.size());
    }
    catch (const Halide::CompileError& e) {
        spdlog::critical("OperationPipelineExecutor::buildOperationChain: Halide Compile Error: {}", e.what());
        m_chain_built = false;
    }
    catch (const std::exception& e) {
        spdlog::critical("OperationPipelineExecutor::buildOperationChain: Exception during compilation: {}", e.what());
        m_chain_built = false;
    }
}

bool OperationPipelineExecutor::executeOnHalideBuffer(Halide::Buffer<float>& input_buffer, Halide::Buffer<float>& output_buffer)
{
    if (!m_chain_built || !m_pipeline.defined()) {
        return true;
    }

    try {
        // 1. Bind the actual C++ buffer memory to the Halide ImageParam
        // This is extremely fast (pointer copy), no data duplication.
        m_input.set(input_buffer);
        // 2. Get the target for execution
        // CRITICAL: For GPU execution, realize() MUST receive the target parameter
        Halide::Target target { Config::AppConfig::getHalideTarget() };

        spdlog::debug("OperationPipelineExecutor::executeOnHalideBuffer: Halide Target Architecture: {}",
                     target.to_string());

        // 3. Execute the pipeline on the correct device (CPU or GPU)
        m_pipeline.realize(output_buffer, target);
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

} // namespace CaptureMoment::Core::Pipeline
