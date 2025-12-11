/**
 * @file i_processing_task.h
 * @brief Interface IProcessingTask for image processing
 * @author CaptureMoment Team
 * @date 2025
 */

#pragma once

#include <memory>
#include <string>
#include <atomic>
#include <format>

#include "common/image_region.h"

namespace CaptureMoment {

/**
 * @brief Global atomic counter for generating unique task identifiers.
 * * This static variable serves as a monotonic generator for task IDs.
 * @note Uses `std::uint64_t` to prevent overflow even with heavy usage..
 */

static std::atomic<std::uint64_t> s_task_id_generator{0};



/**
 * @brief Abstract base class defining the interface for an image processing task.
 *
 * This class provides a standard interface for encapsulating a unit of image
 * processing work. It allows for asynchronous execution, progress reporting,
 * and retrieval of the processed result. This abstraction is key for managing
 * tasks like applying filters, adjustments, or AI models to image regions
 * in a potentially concurrent or sequential manner.
 */
class IProcessingTask {
public:
    /**
     * @brief Virtual destructor for safe inheritance.
     */
    virtual ~IProcessingTask() = default;

    /**
     * @brief Executes the processing task.
     *
     * This pure virtual function must be implemented by derived classes
     * to perform the actual image processing logic.
     */
    virtual void execute() = 0;

    /**
     * @brief Gets the current progress of the task.
     *
     * @return A float value between 0.0f (not started) and 1.0f (completed),
     *         representing the estimated progress of the task.
     */
    virtual float progress() const = 0;

    /**
     * @brief Gets the result of the processed task.
     *
     * This function should return the processed image data encapsulated
     * in an ImageRegion object. The behavior if called before `execute()`
     * has completed is implementation-defined (e.g., it may return nullptr).
     *
     * @return A shared pointer to the resulting ImageRegion. Can be nullptr
     *         if the task failed or has not yet produced a result.
     */
    virtual std::shared_ptr<ImageRegion> result() const = 0;

    /**
     * @brief Gets a unique identifier for this task instance.
     *
     * @return A string representing the unique ID of the task.
     *         Useful for tracking, logging, or managing multiple tasks.
     */
    virtual std::string id() const = 0;

protected:

    /**
     * @brief Generates a unique identifier string for a task instance.
     * This method is defined inline within the class declaration.
     * @return A string in the format "task_<number>".
     */
    static inline std::string generateId() {
        return std::format("task_{}", s_task_id_generator.fetch_add(1, std::memory_order_relaxed));
    }

    /**
     * @brief Current progress of the task, from 0.0f to 1.0f.
     */
    float m_progress{0.0f};
    /**
     * @brief Unique identifier for this task instance.
     */
    std::string m_id;
};

} // namespace CaptureMoment
