/**
 * @file FallbackPipelineExecutor.cpp
 * @brief Implementation of FallbackPipelineExecutor (Generic Sequential Pipeline).
 *
 * @details
 * This class implements a "Generic Sequential Pipeline" strategy.
 * It serves as a fallback executor when optimized paths (e.g., Halide fusion)
 * are unavailable or unsuitable. It applies operations sequentially on a CPU copy
 * of the image data.
 *
 * @author CaptureMoment Team
 * @date 2026
 */

#include "pipeline/fallback_pipeline_executor.h"
#include "operations/interfaces/i_operation_default_logic.h"
#include <spdlog/spdlog.h>
#include <memory>

namespace CaptureMoment::Core::Pipeline {

FallbackPipelineExecutor::FallbackPipelineExecutor(
    std::vector<Operations::OperationDescriptor>&& operations,
    const Operations::OperationFactory& factory
    ) : m_operations(std::move(operations)), m_factory(factory)
{
    spdlog::debug("[FallbackPipelineExecutor] Constructed with {} operations.", m_operations.size());
}

bool FallbackPipelineExecutor::execute(ImageProcessing::IWorkingImageHardware& working_image)
{
    spdlog::debug("[FallbackPipelineExecutor] Starting generic sequential execution...");

    // 1. Validate input image
    if (!working_image.isValid()) {
        spdlog::error("[FallbackPipelineExecutor] Input image is invalid.");
        return false;
    }

    // 2. Export image data to CPU copy
    // Assuming exportToCPUCopy returns std::expected<std::unique_ptr<Common::ImageRegion>, std::error_code>
    // Adapt if you haven't implemented std::expected yet (check for nullptr).
    auto cpu_region_expected = working_image.exportToCPUCopy();
    if (!cpu_region_expected.has_value()) {
        spdlog::error("[FallbackPipelineExecutor] Failed to export image to CPU copy: {}",
                      ErrorHandling::to_string(cpu_region_expected.error()));
        return false;
    }
    auto cpu_region = std::move(cpu_region_expected.value()); // Extract the unique_ptr

    if (!cpu_region || !cpu_region->isValid()) {
        spdlog::error("[FallbackPipelineExecutor] Exported CPU copy is invalid or null.");
        return false;
    }

    spdlog::debug("[FallbackPipelineExecutor] Exported CPU copy: {}x{}x{}",
                  cpu_region->m_width, cpu_region->m_height, cpu_region->m_channels);

    // 3. Apply operations sequentially to the CPU copy
    for (const auto& desc : m_operations) {
        if (!desc.enabled) {
            spdlog::trace("[FallbackPipelineExecutor] Skipping disabled operation: {}", desc.name);
            continue;
        }

        spdlog::debug("[FallbackPipelineExecutor] Applying operation: {}", desc.name);

        // 3a. Create the operation instance using the factory
        auto op_instance = m_factory.create(desc);
        if (!op_instance) {
            spdlog::warn("[FallbackPipelineExecutor] Failed to create operation instance for '{}'. Skipping.", desc.name);
            continue;
        }

        // 3b. Check if the operation supports generic execution via IOperationDefaultLogic
        // Cast the base IOperation pointer to the specific IOperationDefaultLogic interface
        const auto* op_with_default_logic = dynamic_cast<const Operations::IOperationDefaultLogic*>(op_instance->get());
        if (!op_with_default_logic) {
            spdlog::warn("[FallbackPipelineExecutor] Operation '{}' does not support default execution (IOperationDefaultLogic). Skipping.", desc.name);
            continue;
        }

        // 3c. Execute the operation on the CPU region using the default logic
        if (!op_with_default_logic->executeOnImageRegion(*cpu_region, desc)) {
            spdlog::error("[FallbackPipelineExecutor] Operation '{}' failed on CPU copy.", desc.name);
            return false; // Stop processing on error
        }

        spdlog::debug("[FallbackPipelineExecutor] Operation '{}' applied successfully to CPU copy.", desc.name);
    }

    spdlog::debug("[FallbackPipelineExecutor] All operations applied to CPU copy. Updating original image.");

    // 4. Update the original image with the processed CPU copy
    if (!working_image.updateFromCPU(*cpu_region)) {
        spdlog::error("[FallbackPipelineExecutor] Failed to update original image from processed CPU copy.");
        return false;
    }

    spdlog::info("[FallbackPipelineExecutor] Generic sequential execution completed successfully.");
    return true;
}

} // namespace CaptureMoment::Core::Pipeline
