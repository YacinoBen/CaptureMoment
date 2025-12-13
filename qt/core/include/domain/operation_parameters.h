/**
 * @file operation_parameters.h
 * @brief Generic parameter structures for common image operation types.
 * @author CaptureMoment Team
 * @date 2025
 */

#pragma once

#include <string>
#include <cmath>
#include <algorithm>

namespace CaptureMoment::UI {

    /**
     * @brief Namespace containing common data structures and utilities.
     * 
     * This namespace groups types and functions that are shared across
     * different parts of the CaptureMoment core and UI layers.
     */
    namespace Domain {

        /**
         * @brief Parameter structure for operations using a relative adjustment value (e.g., Brightness, Contrast).
         * 
         * This structure holds a single float value intended for adjustments
         * typically in a symmetric range around 0.0 (e.g., -1.0 to 1.0).
         * It includes validation for the value range and a check for activity.
         */
        struct RelativeAdjustmentParams {
            /**
             * @brief The adjustment value.
             * 
             * Typically ranges from MIN_VALUE to MAX_VALUE (e.g., -1.0 to 1.0).
             * A value of 0.0 often represents no change.
             */
            float value {0.0f};
            /**
             * @brief The minimum allowed value for the adjustment.
             */
            static constexpr float MIN_VALUE {-1.0f};
            /**
             * @brief The maximum allowed value for the adjustment.
             */
            static constexpr float MAX_VALUE {1.0f};
            /**
             * @brief Optional identifier for the specific operation instance.
             */
            std::string operation_id;

            /**
             * @brief Checks if the parameter value indicates an active operation.
             * 
             * An operation is considered active if the absolute value is greater than
             * a small epsilon (0.0001f) to account for floating-point inaccuracies.
             * 
             * @return true if the operation is active, false otherwise.
             */
            bool isActive() const {
                return std::abs(value) > 0.0001f; // Tolerance for floating-point comparison
            }

            /**
             * @brief Clamps the value to the valid range [MIN_VALUE, MAX_VALUE].
             * 
             * This function ensures the value stays within the defined limits.
             */
            void clampValue() {
                value = std::clamp(value, MIN_VALUE, MAX_VALUE);
            }
        };

        /**
         * @brief Parameter structure for operations using a positive value (e.g., Blur Radius, Sharpen Amount).
         * 
         * This structure holds a single float value intended for adjustments
         * typically in a positive range (e.g., 0.0 to a maximum value).
         * It includes validation for the value range and a check for activity.
         */
        struct PositiveValueParams {
            /**
             * @brief The positive adjustment value.
             * 
             * Typically ranges from MIN_VALUE (0.0) to MAX_VALUE (e.g., 0.0 to 100.0).
             * A value of 0.0 often represents no effect.
             */
            float value {0.0f};
            /**
             * @brief The minimum allowed value (usually 0.0).
             */
            static constexpr float MIN_VALUE {0.0f};
            /**
             * @brief The maximum allowed value.
             */
            static constexpr float MAX_VALUE {100.0f}; // Adjust according to plausible maximum
            /**
             * @brief Optional identifier for the specific operation instance.
             */
            std::string operation_id;

            /**
             * @brief Checks if the parameter value indicates an active operation.
             * 
             * An operation is considered active if the value is greater than
             * a small epsilon (0.0001f) to account for floating-point inaccuracies.
             * 
             * @return true if the operation is active, false otherwise.
             */
            bool isActive() const {
                return value > 0.0001f; // Tolerance for positive floating-point comparison
            }

             /**
             * @brief Clamps the value to the valid range [MIN_VALUE, MAX_VALUE].
             * 
             * This function ensures the value stays within the defined limits.
             */
            void clampValue() {
                value = std::clamp(value, MIN_VALUE, MAX_VALUE);
            }
        };

        /**
         * @brief Parameter structure for operations using an angle value (e.g., Rotation).
         * 
         * This structure holds an angle in degrees, typically ranging from -360.0 to 360.0.
         * It includes validation for the value range and a check for activity.
         */
        struct AngleParams {
            /**
             * @brief The angle value in degrees.
             * 
             * Typically ranges from MIN_VALUE (-360.0) to MAX_VALUE (360.0).
             * A value of 0.0 represents no rotation.
             */
            float degrees {0.0f};
            /**
             * @brief The minimum allowed angle value.
             */
            static constexpr float MIN_VALUE {-360.0f};
            /**
             * @brief The maximum allowed angle value.
             */
            static constexpr float MAX_VALUE {360.0f};
            /**
             * @brief Optional identifier for the specific operation instance.
             */
            std::string operation_id;

            /**
             * @brief Checks if the angle parameter indicates an active operation.
             * 
             * An operation is considered active if the absolute angle is greater than
             * a small epsilon (0.0001f) to account for floating-point inaccuracies.
             * 
             * @return true if the operation is active, false otherwise.
             */
            bool isActive() const {
                return std::abs(degrees) > 0.0001f; // Tolerance for floating-point comparison
            }

             /**
             * @brief Clamps the value to the valid range [MIN_VALUE, MAX_VALUE].
             * 
             * This function ensures the value stays within the defined limits.
             */
            void clampValue() {
                degrees = std::clamp(degrees, MIN_VALUE, MAX_VALUE);
            }
        };

        /**
         * @brief Parameter structure for operations defining a rectangular region (e.g., Crop).
         * 
         * This structure holds the coordinates (x, y) and dimensions (width, height)
         * of a rectangle. It includes a check for activity based on positive dimensions.
         */
        struct RectangleParams {
            /**
             * @brief X coordinate of the top-left corner of the rectangle.
             */
            int x {0};
            /**
             * @brief Y coordinate of the top-left corner of the rectangle.
             */
            int y {0};
            /**
             * @brief Width of the rectangle.
             */
            int width {0};
            /**
             * @brief Height of the rectangle.
             */
            int height {0};
            /**
             * @brief Optional identifier for the specific operation instance.
             */
            std::string operation_id;

            /**
             * @brief Checks if the rectangle parameters indicate an active operation.
             * 
             * An operation is considered active if both width and height are greater than 0.
             * 
             * @return true if the operation is active, false otherwise.
             */
            bool isActive() const {
                return width > 0 && height > 0; // A zero-area crop has no effect
            }

            /**
             * @brief Validates the rectangle dimensions.
             * 
             * Ensures width and height are not negative.
             * @return true if dimensions are valid, false otherwise.
             */
            bool isValid() const {
                return width >= 0 && height >= 0;
            }
        };

        /**
         * @brief Parameter structure for operations based on an index selection (e.g., Color Profile Selection).
         * 
         * This structure holds an integer index value within a defined range.
         * It includes validation for the index range and a check for activity.
         */
        struct IndexParams {
            /**
             * @brief The selected index.
             * 
             * Must be within [minIndex, maxIndex] to be valid.
             * Often, index 0 might represent "no change" or "default".
             */
            int index {0};
            /**
             * @brief The minimum allowed index value.
             */
            int min_index {0};
            /**
             * @brief The maximum allowed index value.
             * 
             * This should be set appropriately based on the number of available options
             * (e.g., number of color profiles - 1).
             */
            int max_index {0}; // Must be set according to context (e.g., number of available profiles - 1)
            /**
             * @brief Optional identifier for the specific operation instance.
             */
            std::string operation_id;

            /**
             * @brief Checks if the index parameter indicates an active operation.
             * 
             * An operation is considered active if the index is strictly greater than
             * minIndex and less than or equal to maxIndex. This assumes index 0 (or minIndex)
             * might represent an inactive state.
             * 
             * @return true if the operation is active, false otherwise.
             */
            bool isActive() const {
                // Assumes index 0 (or minIndex) is "inactive" or "default".
                // An active state requires a selection beyond the default.
                return index > min_index && index <= max_index;
            }

            /**
             * @brief Validates the index value against its bounds.
             * 
             * @return true if the index is within [minIndex, maxIndex], false otherwise.
             */
            bool isValid() const {
                return index >= min_index && index <= max_index;
            }

            /**
             * @brief Clamps the index to the valid range [minIndex, maxIndex].
             * 
             * This function ensures the index stays within the defined limits.
             */
            void clampIndex() {
                index = std::clamp(index, min_index, max_index);
            }
        };

        // --- Add more generic parameter structures as needed ---
        // struct ComplexMaskParams { /* ... */ };

    } // namespace Common

} // namespace CaptureMoment
