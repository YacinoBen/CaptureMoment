/**
 * @file operation_pipeline_executor.cpp
 * @brief Implementation of OperationPipelineExecutor (Optimized).
 *
 * @details
 * This class implements a "Fused Adjustment Pipeline" strategy.
 * Instead of executing operations sequentially (e.g., Brightness -> Contrast -> Black),
 * this class builds a single Halide graph: `output = black(contrast(brightness(input)))`.
 *
 * This "Fusion" eliminates intermediate buffer reads/writes, providing
 * significant performance improvements for interactive workflows.
 *
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

// ============================================================
// Constructor
// ============================================================
OperationPipelineExecutor::OperationPipelineExecutor(
    const std::vector<Operations::OperationDescriptor>& operations,
    const Operations::OperationFactory& factory
    ) : m_operations(operations), m_factory(factory),
    m_backend(Config::AppConfig::instance().getProcessingBackend())
{
    // ...
}

OperationPipelineExecutor::OperationPipelineExecutor(
    std::vector<Operations::OperationDescriptor>&& operations,
    const Operations::OperationFactory& factory
    ) : m_operations(std::move(operations)), m_factory(factory),
    m_backend(Config::AppConfig::instance().getProcessingBackend())
{
    // Pipeline is compiled at execution time, not construction time.
    // However, we store the configuration (operations) to detect if a rebuild is needed later.
    // Alternatively, you could build the pipeline here if it's fast enough.
}

bool OperationPipelineExecutor::execute(ImageProcessing::IWorkingImageHardware& working_image)
{
    // Decide which cast to attempt based on the configured backend
    if (m_backend == Common::MemoryType::CPU_RAM) {
        // If CPU is configured, try casting to CPU implementation
        if (auto* cpu_impl = dynamic_cast<ImageProcessing::WorkingImageCPU_Halide*>(&working_image)) {
            spdlog::debug("[OperationPipelineExecutor] Detected CPU backend. Trying Fast Path.");
            return executeWithConcreteHalide(*cpu_impl); // Pass the concrete type
        } else {
            // The configured backend is CPU, but the passed image is not a CPU image.
            spdlog::warn("[OperationPipelineExecutor] Configured for CPU, but received non-CPU image. Halide fusion cannot proceed.");
            return false;
        }
    } else if (m_backend == Common::MemoryType::GPU_MEMORY) {
        // If GPU is configured, try casting to GPU implementation
        if (auto* gpu_impl = dynamic_cast<ImageProcessing::WorkingImageGPU_Halide*>(&working_image)) {
            spdlog::debug("[OperationPipelineExecutor] Detected GPU backend. Trying Fast Path.");
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
    // It relies on m_saved_pipeline being already built.
    if (!m_saved_pipeline) {
        spdlog::error("[OperationPipelineExecutor] executeOnHalideBuffer: Pipeline not built yet or failed to build.");
        return false;
    }

    // Get dimensions from the buffer itself
    size_t width = static_cast<size_t>(buffer.width());
    size_t height = static_cast<size_t>(buffer.height());
    size_t channels = static_cast<size_t>(buffer.channels());

    spdlog::trace("[OperationPipelineExecutor] executeOnHalideBuffer: Starting on {}x{}x{} buffer.", width, height, channels);

    // 1. Run the cached pipeline
    // `realize()` triggers the execution of the Halide graph.
    // Since the graph was compiled once, this is extremely fast.
    try {
        m_saved_pipeline->realize(buffer);
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

void OperationPipelineExecutor::buildPipeline()
{
    spdlog::trace("[OperationPipelineExecutor] buildPipeline: Starting graph construction...");

    // 1. Setup Halide variables for the image
    // We use dummy values (0.0f) for `input_func`. The real values are bound
    // in `executePipelineOnBuffer`.
    Halide::Var x, y, c;
    Halide::Func input_func("input_image");
    input_func(x, y, c) = 0.0f;

    // 2. Chain operations into the function graph
    Halide::Func current_func = input_func;
    int enabled_count = 0;

    for (const auto& desc : m_operations)
    {
        // Skip disabled operations
        if (!desc.enabled) {
            spdlog::trace("[OperationPipelineExecutor] buildPipeline: Skipping disabled operation '{}'.", desc.name);
            continue;
        }

        // Create instance
        auto op_impl = m_factory.create(desc);
        if (!op_impl) {
            spdlog::warn("[OperationPipelineExecutor] buildPipeline: Failed to create operation '{}'. Skipping.", desc.name);
            continue;
        }

        // Check if operation supports fusion
        auto fusion_logic = dynamic_cast<const Operations::IOperationFusionLogic*>(op_impl->get());
        if (!fusion_logic) {
            spdlog::warn("[OperationPipelineExecutor] buildPipeline: Operation '{}' does not support fusion logic. Skipping.", desc.name);
            continue;
        }

        // Append to pipeline (Magic: Operator Fusion)
        current_func = fusion_logic->appendToFusedPipeline(current_func, x, y, c, desc);
        enabled_count++;

        spdlog::trace("[OperationPipelineExecutor] buildPipeline: Added '{}' to pipeline.", desc.name);
    }

    if (enabled_count == 0) {
        spdlog::warn("[OperationExecutor] buildPipeline: No valid operations to fuse.");
        m_saved_pipeline.reset();
        return;
    }

    spdlog::debug("[OperationExecutor] buildPipeline: Pipeline built with {} enabled operations.", enabled_count);

    // 3. Apply scheduling based on cached backend
    // We use the m_backend captured at construction to avoid querying AppConfig repeatedly.
    applyScheduling(current_func, x, y, c);

    // 4. JIT Compilation (Heavy Lifting)
    try {
        // We compile the graph defined above.
        // Note: `Halide::Pipeline` is a Halide::Func object. It wraps the compiled Halide function.
        m_saved_pipeline = std::make_unique<Halide::Pipeline>(current_func);
        spdlog::info("[OperationPipelineExecutor] Pipeline compiled successfully.");
    }
    catch (const Halide::CompileError& e) {
        spdlog::critical("[OperationPipelineExecutor] buildPipeline: Halide Compile Error: {}", e.what());
        m_saved_pipeline.reset();
    } catch (const Halide::RuntimeError& e) {
        spdlog::critical("[OperationPipelineExecutor] buildPipeline: Halide Runtime Error: {}", e.what());
        m_saved_pipeline.reset();
    }
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

} // namespace CaptureMoment::Core::Pipeline
