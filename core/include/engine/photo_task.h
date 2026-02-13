/**
 * @file processing_task.h
 * @brief Declaration of PhotoTask class.
 * @author CaptureMoment Team
 * @date 2026
 */

#pragma once

#include <memory>
#include <vector>

#include "operations/operation_factory.h"
#include "operations/operation_descriptor.h"
#include "image_processing/interfaces/i_working_image_hardware.h"
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
 * on a given working image. It uses the PipelineEngine internally to perform the
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
     * @brief The resulting working image after processing.
     */
    std::unique_ptr<ImageProcessing::IWorkingImageHardware> m_result;

public:
    /**
     * @brief Constructs a PhotoTask.
     *
     * @param[in] input_tile A shared pointer to ImageRegion representing the input tile to be processed.
     *                       The task holds a copy of this shared pointer.
     * @param[in] ops A vector of OperationDescriptor objects defining the sequence of operations to apply.
     *                The task stores a copy of this vector (moved if possible).
     * @param[in] operation_factory A shared pointer to OperationFactory used to instantiate
     *                              the concrete operation objects during execution.
     *                              The task holds a copy of this shared pointer.
     */
    PhotoTask(
        std::shared_ptr<Common::ImageRegion> input_tile,
        std::vector<Operations::OperationDescriptor> ops, // OPTIMISATION : Pris par valeur au lieu de const ref
        std::shared_ptr<Operations::OperationFactory> operation_factory
        );

    /**
     * @brief Executes the sequence of operations on the input tile.
     *
     * This method creates a working image from the input tile, iterates through the stored
     * OperationDescriptors, uses the OperationFactory to create instances of the operations,
     * and applies them sequentially using the static OperationPipeline::applyOperations method.
     * The result is stored internally.
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
     * @return A pointer to the resulting IWorkingImageHardware. Returns the processed
     *         working image if execution was successful, or nullptr if the task failed or
     *         has not yet been executed.
     */
    [[nodiscard]] ImageProcessing::IWorkingImageHardware* result() const override { return m_result.get(); };

    /**
     * @brief Gets the unique identifier for this task instance.
     *
     * @return A string representing the unique ID of the task.
     *         This ID is generated during construction.
     */
    [[nodiscard]] std::string id() const override { return m_id; }
};

} // namespace Engine

} // namespace CaptureMoment::Core
