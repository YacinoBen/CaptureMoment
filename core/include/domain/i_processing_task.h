/**
 * @file i_processing_task.h
 * @brief Interface IProcessingTask for image processing tasks.
 * @details Defines the contract for a unit of image processing work, supporting
 *          asynchronous execution, progress tracking, and result retrieval.
 * @author CaptureMoment Team
 * @date 2026
 */

#pragma once

#include <string>
#include <atomic>
#include <format>
#include <cstdint>

#include "image_processing/interfaces/i_working_image_hardware.h"

namespace CaptureMoment::Core {

namespace Domain {

/**
 * @brief Abstract base class defining the interface for an image processing task.
 *
 * This class provides a standard interface for encapsulating a unit of image
 * processing work. It allows for asynchronous execution, progress reporting,
 * and retrieval of the processed result. This abstraction is key for managing
 * tasks like applying filters, adjustments, or AI models to image regions
 * in a potentially concurrent or sequential manner.
 *
 * **Thread Safety:**
 * The `id()` method is thread-safe. The `execute()` method must be thread-safe
 * if called concurrently on the same instance (though typically a task instance is
 * meant to be executed once). `progress()` should be thread-safe for polling.
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
     *
     * @note This function may block depending on the backend implementation.
     */
    virtual void execute() = 0;

    /**
     * @brief Gets the current progress of the task.
     *
     * @return A float value between 0.0f (not started) and 1.0f (completed),
     *         representing the estimated progress of the task.
     */
    [[nodiscard]] virtual float progress() const = 0;

    /**
     * @brief Gets the result of the processed task.
     *
     * This function should return the processed image data encapsulated
     * in an IWorkingImageHardware object.
     *
     * @warning Ownership semantics depend on the implementation.
     *          If the IProcessingTask instance owns the result, this pointer
     *          will become invalid if the task is destroyed.
     *
     * @return A pointer to the resulting IWorkingImageHardware.
     *         Can be nullptr if the task failed or has not yet produced a result.
     */
    [[nodiscard]] virtual ImageProcessing::IWorkingImageHardware* result() const = 0;

    /**
     * @brief Gets a unique identifier for this task instance.
     *
     * @return A string representing the unique ID of the task.
     *         Useful for tracking, logging, or managing multiple tasks.
     */
    [[nodiscard]] virtual std::string id() const = 0;

protected:
    /**
     * @brief Generates a unique identifier string for a task instance.
     *
     * This method is defined inline within the class declaration.
     * It uses an atomic counter to ensure thread-safe ID generation across tasks.
     *
     * @return A string in the format "task_<number>".
     */
    [[nodiscard]] static inline std::string generateId() {
        return std::format("task_{}", s_task_id_generator.fetch_add(1, std::memory_order_relaxed));
    }

    /**
     * @brief Current progress of the task, from 0.0f to 1.0f.
     * Protected member to allow derived classes to update it.
     */
    float m_progress{0.0f};

    /**
     * @brief Unique identifier for this task instance.
     * Derived classes should assign this in their constructor using `generateId()`.
     */
    std::string m_id;

private:
    /**
     * @brief Global atomic counter for generating unique task identifiers.
     *
     * This static variable serves as a monotonic generator for task IDs.
     *
     * @note Uses `std::uint64_t` to prevent overflow even with heavy usage.
     */
    inline static std::atomic<std::uint64_t> s_task_id_generator{0};
};

} // namespace Domain

} // namespace CaptureMoment::Core
