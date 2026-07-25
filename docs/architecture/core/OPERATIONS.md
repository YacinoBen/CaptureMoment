# 🧮 Image Operations

This document describes the image adjustment operations implemented in CaptureMoment, including their mathematical formulas and implementation details.

## 🧮 Tone Adjustments

### Brightness

*   **Purpose:** Adjusts the overall lightness or darkness of an image.
*   **Color Space:** Operations are applied in **Oklab** perceptual color space. Brightness adjustments modify the **L channel** (perceptual luminance) directly.
*   **Formula:** For each pixel `p` in Oklab space:
    ```
    L = p_L + value
    a = p_a
    b = p_b
    Alpha = p_Alpha
    ```
    Where `value` is the brightness adjustment factor (typically in the range [-1.0, 1.0]).
*   **Implementation:** `OperationBrightness` in `core/operations/basic_adjustment_operations/`.
*   **QML Model:** `BrightnessModel` in `qt/core/models/operations/basic_adjustment_models/`.
*   **Fusion Support:** Implements `IOperationFusionLogic` interface with `appendToFusedPipeline` method for pipeline fusion optimization.

### Contrast

*   **Purpose:** Adjusts the difference in perceptual lightness in different parts of an image.
*   **Color Space:** Operations are applied in **Oklab** perceptual color space. Contrast adjustments modify the **L channel** centered at mid-gray (0.5).
*   **Formula:** For each pixel `p` in Oklab space:
    ```
    L = 0.5 + (p_L - 0.5) * (1.0 + value)
    a = p_a
    b = p_b
    Alpha = p_Alpha
    ```
    Where `value` is the contrast adjustment factor (typically in the range [-1.0, 1.0]). A value of 0 means no change.
*   **Implementation:** `OperationContrast` in `core/operations/basic_adjustment_operations/`.
*   **QML Model:** `ContrastModel` in `qt/core/models/operations/basic_adjustment_models/`.
*   **Fusion Support:** Implements `IOperationFusionLogic` interface with `appendToFusedPipeline` method for pipeline fusion optimization.

### Highlights

*   **Purpose:** Adjusts the perceptual luminosity of the brightest areas of the image.
*   **Color Space:** Operations are applied in **Oklab** perceptual color space. Adjustments target the **L channel** using a smooth mask.
*   **Formula (Approximation):** A smoothstep mask based on Oklab L channel is created. Pixels with L above a threshold (0.7) are adjusted:
    ```
    t = clamp((L - 0.7) / (1.0 - 0.7), 0.0, 1.0)
    mask = t * t * (3.0 - 2.0 * t)  // smoothstep polynomial
    L = L + mask * value
    a = p_a
    b = p_b
    Alpha = p_Alpha
    ```
    The smoothstep mask ensures gradual transitions without banding in the highlight roll-off.
*   **Implementation:** `OperationHighlights` in `core/operations/basic_adjustment_operations/`.
*   **QML Model:** `HighlightsModel` in `qt/core/models/operations/basic_adjustment_models/`.
*   **Fusion Support:** Implements `IOperationFusionLogic` interface with `appendToFusedPipeline` method for pipeline fusion optimization.

### Shadows

*   **Purpose:** Adjusts the perceptual luminosity of the darkest areas of the image.
*   **Color Space:** Operations are applied in **Oklab** perceptual color space. Adjustments target the **L channel** using a smooth mask.
*   **Formula (Approximation):** A smoothstep mask based on Oklab L channel is created. Pixels with L below a threshold (0.5) are adjusted:
    ```
    t = clamp((0.5 - L) / 0.5, 0.0, 1.0)
    mask = t * t * (3.0 - 2.0 * t)  // smoothstep polynomial
    L = L + mask * value
    a = p_a
    b = p_b
    Alpha = p_Alpha
    ```
    The smoothstep mask ensures gradual transitions without banding in the shadow roll-off.
*   **Implementation:** `OperationShadows` in `core/operations/basic_adjustment_operations/`.
*   **QML Model:** `ShadowsModel` in `qt/core/models/operations/basic_adjustment_models/`.
*   **Fusion Support:** Implements `IOperationFusionLogic` interface with `appendToFusedPipeline` method for pipeline fusion optimization.

### Whites

*   **Purpose:** Adjusts the perceptual luminosity of the extreme white point of the image.
*   **Color Space:** Operations are applied in **Oklab** perceptual color space. Adjustments target the **L channel** using a smooth mask for extreme highlights.
*   **Formula (Approximation):** A smoothstep mask based on Oklab L channel is created. Pixels with very high L (above 0.85) are adjusted:
    ```
    t = clamp((L - 0.85) / (1.0 - 0.85), 0.0, 1.0)
    mask = t * t * (3.0 - 2.0 * t)  // smoothstep polynomial
    L = L + mask * value
    a = p_a
    b = p_b
    Alpha = p_Alpha
    ```
    The mask ensures only the extreme highlights are primarily affected, preserving upper midtones.
*   **Implementation:** `OperationWhites` in `core/operations/basic_adjustment_operations/`.
*   **QML Model:** `WhitesModel` in `qt/core/models/operations/basic_adjustment_models/`.
*   **Fusion Support:** Implements `IOperationFusionLogic` interface with `appendToFusedPipeline` method for pipeline fusion optimization.

### Blacks

*   **Purpose:** Adjusts the perceptual luminosity of the extreme black point of the image.
*   **Color Space:** Operations are applied in **Oklab** perceptual color space. Adjustments target the **L channel** using a smooth mask for deep shadows.
*   **Formula (Approximation):** A smoothstep mask based on Oklab L channel is created. Pixels with very low L (below 0.3) are adjusted:
    ```
    t = clamp((0.3 - L) / (0.3 - 0.0), 0.0, 1.0)
    mask = t * t * (3.0 - 2.0 * t)  // smoothstep polynomial
    L = L + mask * value
    a = p_a
    b = p_b
    Alpha = p_Alpha
    ```
    The mask ensures only the deep shadows are primarily affected, preserving lower midtones.
*   **Implementation:** `OperationBlacks` in `core/operations/basic_adjustment_operations/`.
*   **QML Model:** `BlacksModel` in `qt/core/models/operations/basic_adjustment_models/`.
*   **Fusion Support:** Implements `IOperationFusionLogic` interface with `appendToFusedPipeline` method for pipeline fusion optimization.

## 🧮 Implementation Notes

*   **Core:** Operations are implemented as classes inheriting from `IOperation` in the `Core::Operations` namespace.
*   **Fusion Logic:** Operations also implement `IOperationFusionLogic` interface to support pipeline fusion optimization.
*   **Sequential Method:** Each operation maintains a `[[maybe_unused]] execute` method for sequential processing compatibility.
*   **QML Models:** UI-specific models inherit from `BaseAdjustmentModel` which provides common properties (`value`, `minimum`, `maximum`, `name`, `active`) and Qt infrastructure.
*   **Halide:** Many operations use the Halide library for efficient image processing on the CPU/GPU.
*   **Oklab Color Space:** All tone adjustments operate in **Oklab perceptual color space**. The L channel represents perceptual luminance, enabling more natural and uniform adjustments compared to RGB luminance formulas.
*   **Channel Isolation:** Adjustments are applied **only to the L channel** (`c == 0`). The a, b, and Alpha channels are preserved unchanged to prevent hue shifts and maintain color fidelity.
*   **Smoothstep Masks:** Mask generation uses the smoothstep polynomial `3t² - 2t³` instead of linear interpolation, eliminating harsh transitions and banding artifacts in shadow/highlight roll-offs.
*   **No Intermediate Clamping:** Operations do not clamp the L channel during processing. Clamping is applied only after conversion back to RGB (`oklab_to_rgb`) to prevent hue shifts caused by clamping L while a and b are non-zero.
*   **Pipeline Fusion:** Operations contribute their logic to combined computational graphs through the `appendToFusedPipeline` method, eliminating intermediate buffer copies. The full pipeline follows the flow: `RGB → Oklab → fused operations → RGB`.
*   