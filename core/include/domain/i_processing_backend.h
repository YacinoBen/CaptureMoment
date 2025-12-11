/**
 * @file i_processing_backend.h
 * @brief Interface IProcessingBackend for for a backend that manages image processing tasks
 * @author CaptureMoment Team
 * @date 2025
 */

#pragma once

#include <memory>
#include <vector>

#include "domain/i_processing_task.h"
#include "operations/operation_descriptor.h"

namespace CaptureMoment {

/**
 * @brief Abstract base class defining the interface for a backend that manages image processing tasks.
 *
 * This interface provides a way to create and submit image processing tasks encapsulated
 * by the IProcessingTask interface. It acts as a contract for different processing backends,
 * allowing the application to potentially switch between or utilize different execution
 * mechanisms (e.g., synchronous, asynchronous, CPU, GPU) without changing the calling code.
 */
class IProcessingBackend {
public:
    /**
     * @brief Virtual destructor for safe inheritance.
     */
    virtual ~IProcessingBackend() = default;

    /**
     * @brief Creates a new processing task.
     *
     * This pure virtual function must be implemented by derived classes to
     * instantiate a concrete IProcessingTask based on the provided list of
     * operations and the specified image region (tile).
     *
     * @param[in] ops A vector of OperationDescriptor objects defining the
     *                sequence of operations to be applied.
     * @param[in] x The X coordinate of the top-left corner of the image region.
     * @param[in] y The Y coordinate of the top-left corner of the image region.
     * @param[in] width The width of the image region.
     * @param[in] height The height of the image region.
     * @return A shared pointer to the newly created IProcessingTask object.
     *         The returned object is not executed yet.
     */
    virtual std::shared_ptr<IProcessingTask> createTask(
        const std::vector<OperationDescriptor>& ops,
        int x, int y, int width, int height
    ) = 0;

    /**
     * @brief Submits a processing task for execution.
     *
     * This pure virtual function must be implemented by derived classes to
     * handle the execution of the given IProcessingTask. The execution can be
     * synchronous (blocking until complete) or asynchronous depending on the
     * specific backend implementation.
     *
     * @param[in] task A shared pointer to the IProcessingTask to be executed.
     * @return true if the task was successfully submitted for execution or
     *         completed successfully, false otherwise.
     */
    [[nodiscard]] virtual bool submit(std::shared_ptr<IProcessingTask> task) = 0;
};

} // namespace CaptureMoment