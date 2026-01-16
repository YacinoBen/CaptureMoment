/**
 * @file i_backend_decider.h
 * @brief Abstract interface for deciding the hardware backend (CPU or GPU) for image processing.
 * @author CaptureMoment Team
 * @date 2026
 */

#pragma once

#include "common/types/memory_type.h"

namespace CaptureMoment::Core {

namespace ImageProcessing {

/**
 * @brief Abstract interface for deciding the hardware backend (CPU or GPU) for image processing.
 *
 * This interface defines a contract for components responsible for selecting
 * the appropriate hardware location (CPU RAM or GPU memory) where the working
 * image data should reside and be processed. The decision can be based on
 * user preferences, hardware capabilities, performance benchmarks, or other factors.
 *
 * Implementations of this interface (e.g., UserPreferenceBackendDecider,
 * HardwareDetectionBackendDecider) provide concrete logic for making this choice.
 */
class IBackendDecider {
public:
    /**
     * @brief Virtual destructor for safe inheritance and polymorphic deletion.
     */
    virtual ~IBackendDecider() = default;

    /**
     * @brief Decides the memory type (CPU or GPU) to use for the working image.
     *
     * This method encapsulates the decision logic. It should return a valid
     * MemoryType enum value indicating the recommended hardware location for
     * storing and processing the image data.
     *
     * @return A MemoryType value indicating the chosen backend (CPU_RAM or GPU_MEMORY).
     */
    [[nodiscard]] virtual Common::MemoryType decide() const = 0;

protected:
    // Protected constructor to enforce abstract nature.
    IBackendDecider() = default;
};

} // namespace ImageProcessing

} // namespace CaptureMoment::Core
