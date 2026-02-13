/**
 * @file working_image_factory.h
 * @brief Factory for creating IWorkingImageHardware instances using a Registry Pattern.
 * @details Encapsulates logic for instantiating CPU or GPU working images.
 * @author CaptureMoment Team
 * @date 2026
 */

#pragma once

#include "image_processing/interfaces/i_working_image_hardware.h"
#include "common/image_region.h"
#include "common/types/memory_type.h"
#include <memory>
#include <functional>
#include <unordered_map>

namespace CaptureMoment::Core {

namespace ImageProcessing {

/**
 * @class WorkingImageFactory
 * @brief Factory with registration support for creating IWorkingImageHardware instances.
 *
 * @details
 * This factory uses a Registry Pattern (static map) instead of a hardcoded switch statement.
 * This allows new backends (e.g., `WorkingImageCUDA`, `WorkingImageTPU`) to be registered
 * at startup from anywhere in the codebase, without modifying this factory class.
 *
 * **Supported Implementations:**
 * - `WorkingImageCPU_Halide`: CPU-backed image using Halide buffers.
 * - `WorkingImageGPU_Halide`: GPU-backed image using Halide buffers (CUDA/Vulkan/etc.).
 * - `Add others`.
 */
class WorkingImageFactory {
public:
    /**
     * @brief Definition of a creator function.
     * @details A function that takes a source ImageRegion and returns a constructed object.
     */
    using CreatorFunction = std::function<std::unique_ptr<IWorkingImageHardware>(const Common::ImageRegion&)>;

    /**
     * @brief Creates a new IWorkingImageHardware instance using registered creators.
     *
     * @details
     * Looks up the registered creator for the given `backend` type.
     * If no creator is found, logs an error and returns nullptr.
     *
     * @param backend The desired hardware backend (CPU_RAM or GPU_MEMORY).
     * @param source_image The source image data residing in CPU memory.
     * @return A unique pointer to the created object, or nullptr on failure.
     */
    [[nodiscard]] static std::unique_ptr<IWorkingImageHardware> create(
        Common::MemoryType backend,
        const Common::ImageRegion& source_image
        );

    /**
     * @brief Registers a creator function for a specific backend type.
     *
     * @details
     * This method allows users to plug in new backends dynamically (at startup).
     * It replaces the need to modify the factory class source code.
     *
     * @param type The MemoryType this creator handles.
     * @param creator The function that creates the object.
     */
    static void registerCreator(
        Common::MemoryType type,
        CreatorFunction creator
        );

private:
    /**
     * @brief Registry mapping backend type to its creator function.
     */
    static inline std::unordered_map<Common::MemoryType, CreatorFunction> s_registry;
};

} // namespace ImageProcessing

} // namespace CaptureMoment::Core
