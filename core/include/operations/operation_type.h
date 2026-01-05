/**
 * @file operation_type.h
 * @brief Enumeration of image processing operation types
 * @author CaptureMoment Team
 * @date 2025
 */

#pragma once
#include <cstdint>

namespace CaptureMoment::Core {

namespace Operations {
/**
 * @enum OperationType
 * @brief Identifies the type of an image processing operation.
 * * This enum acts as a registry for all available operations in the system.
 * It is used by the UI to list available tools and by the processing engine
 * to instantiate the correct IOperation implementation.
 */
enum class OperationType : uint8_t {
    /**
     * @brief Exposure adjustment (EV based).
     * Simulates changing the exposure time or aperture.
     */
    Exposure,

    /**
     * @brief Brightness adjustment (Additive).
     * Adds a constant value to all pixels.
     * Use sparingly as it can wash out blacks.
     */
    Brightness,

    /**
     * @brief Contrast adjustment (Multiplicative around a midpoint).
     * Expands or compresses the dynamic range around gray.
     */
    Contrast,

    /**
     * @brief Highlights adjustment.
     * Adjusts the luminosity of the brightest areas of the image.
     * Increasing highlights brightens the bright tones.
     * Decreasing highlights darkens the bright tones.
     */
    Highlights,

    /**
     * @brief Shadows adjustment.
     * Adjusts the luminosity of the darkest areas of the image.
     * Increasing shadows brightens the dark tones.
     * Decreasing shadows darkens the dark tones.
     */
    Shadows,

    /**
     * @brief Whites adjustment.
     * Adjusts the luminosity of the white point of the image.
     * Increasing whites makes the brightest areas brighter.
     * Decreasing whites makes the brightest areas darker.
     */
    Whites,

    /**
     * @brief Blacks adjustment.
     * Adjusts the luminosity of the black point of the image.
     * Increasing blacks makes the darkest areas brighter.
     * Decreasing blacks makes the darkest areas darker.
     */
    Blacks,

    /**
     * @brief Saturation adjustment (Color intensity).
     * Modifies the vividness of colors without affecting luminance.
     */
    Saturation,

    // Future operations can be added here
    // WhiteBalance,
    // Sharpen,
    // Denoise,
    // ...
};

} // namespace Operations

} // namespace CaptureMoment::Core
