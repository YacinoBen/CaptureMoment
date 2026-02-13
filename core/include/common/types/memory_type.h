/**
 * @file memory_type.h
 * @brief Enum representing the memory type where image data is stored (CPU or GPU).
 * @author CaptureMoment Team
 * @date 2025
 */

#pragma once

#include <cstdint>

namespace CaptureMoment::Core {

namespace Common {

/**
 * @brief Enum representing the memory type where image data is stored.
 *
 * Used by IWorkingImage and related classes to identify where the actual
 * pixel data is located (in CPU RAM or GPU Memory).
 *
 * Uses std::uint8_t as underlying type for memory efficiency.
 */
enum class MemoryType : std::uint8_t {
    /**
     * @brief Data is stored in main CPU RAM.
     */
    CPU_RAM = 0,

    /**
     * @brief Data is stored in GPU memory.
     */
    GPU_MEMORY = 1
};

} // namespace Common

} // namespace CaptureMoment::Core
