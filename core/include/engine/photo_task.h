/**
 * @file processing_task.h
 * @brief Declaration of PhotoTask class.
 * @author CaptureMoment Team
 * @date 2025
 */

#pragma once

#include <memory>
#include <vector>

#include "operations/operation_factory.h"
#include "operations/operation_descriptor.h"
#include "common/image_region.h"
#include "domain/i_processing_task.h"

namespace CaptureMoment::Core {

namespace Domain {
class IProcessingTask;
}

namespace Engine {
/**
 * @brief Concrete implementation of IProcessingTask for applying a sequence of operations to an image tile.
 *
 * This class encapsulates the data (input tile, operations, factory) and the logic
 * to execute a series of image processing operations defined by OperationDescriptors
 * on a given ImageRegion. It uses the PipelineEngine internally to perform the
 * actual processing steps.
 */
class PhotoTask : public Domain::IProcessingTask
{
private:
    /**
     * @brief Shared pointer to the factory for creating operation instances.
     */
    std::shared_ptr<Operations::OperationFactory> m_operation_factory;
    /**
     * @brief The list of operations to apply.
     */
    std::vector<Operations::OperationDescriptor> m_operation_descriptors;
    /**
     * @brief The input image region to process.
     */
    std::shared_ptr<Common::ImageRegion> m_input_tile;
    /**
     * @brief The resulting image region after processing.
     */
    std::shared_ptr<Common::ImageRegion> m_result;
    
public:
    /**
     * @brief Constructs a PhotoTask.
     *
     * @param[in] input_tile A shared pointer to the ImageRegion representing the input tile to be processed.
     *                       The task holds a copy of this shared pointer.
     * @param[in] ops A vector of OperationDescriptor objects defining the sequence of operations to apply.
     *                The task stores a copy of this vector.
     * @param[in] operation_factory A shared pointer to the OperationFactory used to instantiate
     *                              the concrete operation objects during execution.
     *                              The task holds a copy of this shared pointer.
     */
    PhotoTask(
        std::shared_ptr<Common::ImageRegion> input_tile,
        const std::vector<Operations::OperationDescriptor>& ops,
        std::shared_ptr<Operations::OperationFactory> operation_factory
        );

    /**
     * @brief Executes the sequence of operations on the input tile.
     *
     * This method iterates through the stored OperationDescriptors,
     * uses the OperationFactory to create instances of the operations,
     * and applies them sequentially to the input tile using the static
     * PipelineEngine::applyOperations method. The result is stored internally.
     * Progress is updated accordingly during execution.
     */
    void execute() override;

    /**
     * @brief Gets the current progress of the task.
     *
     * @return A float value between 0.0f (not started) and 1.0f (completed),
     *         representing the estimated progress of the task. For PhotoTask,
     *         this might be updated during the execute() call or simply return
     *         0.0f/1.0f if executed synchronously.
     */
    [[nodiscard]] float progress() const override { return m_progress; };

    /**
     * @brief Gets the result of the processed task.
     *
     * @return A shared pointer to the resulting ImageRegion. Returns the processed
     *         tile if execution was successful, or nullptr if the task failed or
     *         has not yet been executed.
     */
    [[nodiscard]] std::shared_ptr<Common::ImageRegion> result() const override { return m_result; };

    /**
     * @brief Gets the unique identifier for this task instance.
     *
     * @return A string representing the unique ID of the task.
     *         This ID is generated during construction.
     */
    [[nodiscard]] std::string id() const override { return m_id; }
};

} // namespace Engine

} // namespace CaptureMoment::core
