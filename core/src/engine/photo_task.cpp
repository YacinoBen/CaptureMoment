/**
 * @file photo_task.cpp
 * @brief Implementation of PhotoTask
 * @author CaptureMoment Team
 * @date 2026
 */

#include "engine/photo_task.h"
#include "image_processing/image_processing.h"

#include "operations/operation_pipeline.h"
#include "config/app_config.h" // Pour obtenir le backend
#include <spdlog/spdlog.h>

namespace CaptureMoment::Core::Engine {

// Constructor: Initializes the task with input data, operations, and the factory.
PhotoTask::PhotoTask(
    std::shared_ptr<Common::ImageRegion> input_tile,
    const std::vector<Operations::OperationDescriptor>& ops,
    std::shared_ptr<Operations::OperationFactory> operation_factory
    )
    : m_operation_factory{std::move(operation_factory)},
    m_operation_descriptors{ops},
    m_input_tile{std::move(input_tile)},
    m_result{nullptr}
{
    m_id = this->generateId();
}

// Executes the processing task.
void PhotoTask::execute() {
    spdlog::info("PhotoTask::execute: Starting");
    m_progress = 0.0f;

    // Check if required inputs are valid.
    if (!m_input_tile || !m_operation_factory) {
        spdlog::warn("PhotoTask::execute: Invalid input - m_input_tile: {}, m_operation_factory: {}",
                     !m_input_tile, !m_operation_factory);
        m_result = nullptr;
        m_progress = 1.0f;
        return;
    }

    auto backend = Config::AppConfig::instance().getProcessingBackend();
    if (backend == Common::MemoryType::CPU_RAM) {
        m_result = std::make_unique<ImageProcessing::WorkingImageCPU_Halide>();
    } else {
        m_result = std::make_unique<ImageProcessing::WorkingImageGPU_Halide>();
    }

    if (!m_result->updateFromCPU(*m_input_tile)) {
        spdlog::error("PhotoTask::execute: Failed to initialize working image from input tile.");
        m_result = nullptr;
        m_progress = 1.0f;
        return;
    }

    spdlog::info("PhotoTask::execute: Starting OperationPipeline::applyOperations");

    // Apply the sequence of operations to the working image.
    bool success = Operations::OperationPipeline::applyOperations(*m_result, m_operation_descriptors, *m_operation_factory);

    if (!success) {
        spdlog::error("PhotoTask::execute: OperationPipeline::applyOperations failed.");
        m_result = nullptr;
    }

    m_progress = 1.0f;
    spdlog::info("PhotoTask::execute: Completed with success={}", success);
}

} // namespace CaptureMoment::Core::Engine
