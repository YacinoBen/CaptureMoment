
/**
 * @file operation_ranges.h
 * @brief Declaration of the OperationRanges struct.
 * @author CaptureMoment Team
 * @date 2025
 */

#pragma once

namespace CaptureMoment::Core::Operations {

/**
 * @brief Centralized definition of value ranges and default values for different operations using consteval.
 * This struct provides consteval functions for the valid input ranges (minimum and maximum)
 * and the default value for various image operations, ensuring consistency and compile-time evaluation.
 * These values represent the logical constraints of the operations within the core engine.
 */
struct OperationRanges {

    // --- Brightness Operation Ranges and Default ---
    /**
     * @brief Gets the minimum allowed value for the Brightness operation.
     * @return float The minimum brightness value (e.g., -1.0f).
     */
    [[nodiscard]] consteval static float getBrightnessMinValue() {
        return -1.0f;
    }

    /**
     * @brief Gets the maximum allowed value for the Brightness operation.
     * @return float The maximum brightness value (e.g., 1.0f).
     */
    [[nodiscard]] consteval static float getBrightnessMaxValue() {
        return 1.0f;
    }

    /**
     * @brief Gets the default value for the Brightness operation.
     * @return float The default brightness value (e.g., 0.0f, representing no change).
     */
    [[nodiscard]] consteval static float getBrightnessDefaultValue() {
        return 0.0f;
    }

    // --- Contrast Operation Ranges and Default ---
    /**
     * @brief Gets the minimum allowed value for the Contrast operation.
     * @return float The minimum contrast value (e.g., 0.0f).
     */
    [[nodiscard]] consteval static float getContrastMinValue() {
        return 0.0f;
    }

    /**
     * @brief Gets the maximum allowed value for the Contrast operation.
     * @return float The maximum contrast value (e.g., 2.0f).
     */
    [[nodiscard]] consteval static float getContrastMaxValue() {
        return 2.0f;
    }

    /**
     * @brief Gets the default value for the Contrast operation.
     * @return float The default contrast value (e.g., 1.0f, representing no change).
     */
    [[nodiscard]] consteval static float getContrastDefaultValue() {
        return 1.0f;
    }

    // --- Highlights Operation Ranges and Default ---
    /**
     * @brief Gets the minimum allowed value for the Highlights operation.
     * @return float The minimum highlights value (e.g., -1.0f).
     */
    [[nodiscard]] consteval static float getHighlightsMinValue() {
        return -1.0f;
    }

    /**
     * @brief Gets the maximum allowed value for the Highlights operation.
     * @return float The maximum highlights value (e.g., 1.0f).
     */
    [[nodiscard]] consteval static float getHighlightsMaxValue() {
        return 1.0f;
    }

    /**
     * @brief Gets the default value for the Highlights operation.
     * @return float The default highlights value (e.g., 0.0f, representing no change).
     */
    [[nodiscard]] consteval static float getHighlightsDefaultValue() {
        return 0.0f;
    }

    // --- Shadows Operation Ranges and Default ---
    /**
     * @brief Gets the minimum allowed value for the Shadows operation.
     * @return float The minimum shadows value (e.g., -1.0f).
     */
    [[nodiscard]] consteval static float getShadowsMinValue() {
        return -1.0f;
    }

    /**
     * @brief Gets the maximum allowed value for the Shadows operation.
     * @return float The maximum shadows value (e.g., 1.0f).
     */
    [[nodiscard]] consteval static float getShadowsMaxValue() {
        return 1.0f;
    }

    /**
     * @brief Gets the default value for the Shadows operation.
     * @return float The default shadows value (e.g., 0.0f, representing no change).
     */
    [[nodiscard]] consteval static float getShadowsDefaultValue() {
        return 0.0f;
    }

    // --- Whites Operation Ranges and Default ---
    /**
     * @brief Gets the minimum allowed value for the Whites operation.
     * @return float The minimum whites value (e.g., -1.0f).
     */
    [[nodiscard]] consteval static float getWhitesMinValue() {
        return -1.0f;
    }

    /**
     * @brief Gets the maximum allowed value for the Whites operation.
     * @return float The maximum whites value (e.g., 1.0f).
     */
    [[nodiscard]] consteval static float getWhitesMaxValue() {
        return 1.0f;
    }

    /**
     * @brief Gets the default value for the Whites operation.
     * @return float The default whites value (e.g., 0.0f, representing no change).
     */
    [[nodiscard]] consteval static float getWhitesDefaultValue() {
        return 0.0f;
    }

    // --- Blacks Operation Ranges and Default ---
    /**
     * @brief Gets the minimum allowed value for the Blacks operation.
     * @return float The minimum blacks value (e.g., -1.0f).
     */
    [[nodiscard]] consteval static float getBlacksMinValue() {
        return -1.0f;
    }

    /**
     * @brief Gets the maximum allowed value for the Blacks operation.
     * @return float The maximum blacks value (e.g., 1.0f).
     */
    [[nodiscard]] consteval static float getBlacksMaxValue() {
        return 1.0f;
    }

    /**
     * @brief Gets the default value for the Blacks operation.
     * @return float The default blacks value (e.g., 0.0f, representing no change).
     */
    [[nodiscard]] consteval static float getBlacksDefaultValue() {
        return 0.0f;
    }

};

} // namespace CaptureMoment::Core::Operations
