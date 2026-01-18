/**
 * @file memory_type.h
 * @brief Enum representing the memory type where image data is stored (CPU or GPU).
 * @author CaptureMoment Team
 * @date 2026
 */

#pragma once

namespace CaptureMoment::Core {

namespace Common {

/**
 * @brief Enum representing the memory type where image data is stored.
 *
 * Used by IWorkingImage and related classes to identify where the actual
 * pixel data is located (in CPU RAM or GPU Memory).
 */
enum class MemoryType {
    /**
     * @brief Data is stored in main CPU RAM.
     */
    CPU_RAM,

    /**
     * @brief Data is stored in GPU memory.
     */
    GPU_MEMORY
};

} // namespace Common

} // namespace CaptureMoment::Core
