/**
 * @file i_backend_decider.h
 * @brief Abstract interface for deciding the hardware backend (CPU or GPU) for image processing.
 *
 * This header defines the `IBackendDecider` interface, which acts as the base
 * for the "Strategy Pattern" regarding hardware allocation. Implementations
 * of this interface encapsulate the logic for choosing whether to process images
 * on the CPU or a specific GPU API (CUDA, Metal, etc.).
 *
 * @author CaptureMoment Team
 * @date 2025
 */

#pragma once

#include "common/types/memory_type.h"

namespace CaptureMoment::Core {

namespace ImageProcessing {

/**
 * @interface IBackendDecider
 * @brief Abstract interface defining the strategy for hardware backend selection.
 *
 * @details
 * This class is a pure interface (interface pattern) used to decouple the
 * `CaptureMoment::Core` from the logic required to choose an optimal hardware backend.
 *
 * The decision process (CPU vs GPU) is a critical startup operation. Different
 * strategies may be employed depending on the application context:
 * - **Benchmarking**: Run performance tests on all backends and pick the fastest.
 *   (See `BenchmarkingBackendDecider`).
 * - **User Preference**: Use the backend explicitly requested by the user configuration.
 * - **Hardware Detection**: Force GPU if available, fallback to CPU otherwise.
 *
 * This interface allows the system to switch strategies easily (e.g., for testing
 * or different deployment scenarios) without modifying the core image processing engine.
 */
class IBackendDecider {
public:
    /**
     * @brief Virtual destructor.
     *
     * @details
     * Ensures that destructors of derived classes (e.g., `BenchmarkingBackendDecider`)
     * are called correctly when deleting an object through a base class pointer.
     */
    virtual ~IBackendDecider() = default;

    /**
     * @brief Decides the optimal hardware backend (CPU or GPU) for image processing.
     *
     * @details
     * This method executes the decision logic encapsulated by the concrete implementation.
     * It must return a valid `Common::MemoryType`.
     *
     * @note This method is marked `const` because making a decision about hardware
     *       availability or performance characteristics should not modify the
     *       internal state of the decider object. This enables safe concurrent
     *       access if derived classes are thread-safe.
     *
     * @return Common::MemoryType The selected memory type:
     *         - `MemoryType::CPU_RAM`: If CPU is the optimal or only available choice.
     *         - `MemoryType::GPU_MEMORY`: If a GPU backend is selected.
     */
    [[nodiscard]] virtual Common::MemoryType decide() const = 0;

protected:
    /**
     * @brief Protected constructor.
     *
     * @details
     * Ensures that the interface cannot be instantiated directly.
     * Only concrete derived classes can be created.
     */
    IBackendDecider() = default;
};

} // namespace ImageProcessing

} // namespace CaptureMoment::Core
