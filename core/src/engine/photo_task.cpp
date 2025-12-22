/**
 * @file photo_task.cpp
 * @brief Implementation of PhotoTask
 * @author CaptureMoment Team
 * @date 2025
 */

#include <memory>
#include <vector>
#include <spdlog/spdlog.h>

#include "engine/photo_task.h"
#include "operations/operation_pipeline.h"

namespace CaptureMoment {

    // Constructor: Initializes the task with input data, operations, and the factory.
    PhotoTask::PhotoTask(
        std::shared_ptr<ImageRegion> input_tile,          // The image region to be processed.
        const std::vector<OperationDescriptor>& ops,      // The list of operations to apply.
        std::shared_ptr<OperationFactory> operation_factory // The factory to create operation instances.
    )
        : m_operation_factory{operation_factory},
          m_operation_descriptors{ops},
          m_input_tile{input_tile},
          m_result{nullptr} // Initialize result pointer to null.
    {
        m_id = this->generateId();
    }

    // Executes the processing task.
    void PhotoTask::execute() {
        spdlog::info("PhotoTask::execute: Starting");
        // Reset progress to 0% at the beginning.
        m_progress = 0.0f;

        // Check if required inputs (tile, factory) are valid.
        if (!m_input_tile || !m_operation_factory) {
            spdlog::info("PhotoTask::execute: check required m_input_tile {}, m_operation_factory {}", !m_input_tile, !m_operation_factory);

             // Set result to null and mark progress as 100% to indicate failure.
             m_result = nullptr; 
             m_progress = 1.0f;
             return; // Exit early if inputs are invalid.
        }


        spdlog::info("PhotoTask::execute: Starting OperationPipeline::applyOperations");
        // Apply the sequence of operations to the input tile using the static PipelineEngine.
        bool success = OperationPipeline::applyOperations(*m_input_tile, m_operation_descriptors, *m_operation_factory);

        if (success) {
            // If processing was successful, the result is the modified input tile.
            m_result = m_input_tile; 
        } else {
            // If processing failed, set result to null.
            m_result = nullptr;
        }

        // Mark progress as 100% upon completion (success or failure).
        m_progress = 1.0f;
        spdlog::info("PhotoTask::execute: Completed with success={}", success);
    }

    // Gets the current progress of the task.
    float PhotoTask::progress() const {
        return m_progress;
    }

    // Gets the result of the processed task.
    std::shared_ptr<ImageRegion> PhotoTask::result() const {
        return m_result;
    }

    // Gets the unique identifier for this task instance.
    std::string PhotoTask::id() const {
        return m_id;
    }

} // namespace CaptureMoment
