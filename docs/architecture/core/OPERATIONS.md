# 🧮 Image Operations

This document describes the image adjustment operations implemented in CaptureMoment, including their mathematical formulas and implementation details.

## 🧮 Tone Adjustments

### Brightness

*   **Purpose:** Adjusts the overall lightness or darkness of an image.
*   **Formula:** For each pixel `p` and channel `c` (excluding alpha):
    ```
    p_c = p_c + value
    ```
    Where `value` is the brightness adjustment factor (typically in the range [-1.0, 1.0]).
*   **Implementation:** `OperationBrightness` in `core/operations/basic_adjustment_operations/`.
*   **QML Model:** `BrightnessModel` in `qt/core/models/operations/basic_adjustment_models/`.
*   **Fusion Support:** Implements `IOperationFusionLogic` interface with `appendToFusedPipeline` method for pipeline fusion optimization.

### Contrast

*   **Purpose:** Adjusts the difference in color and brightness in different parts of an image.
*   **Formula:** For each pixel `p` and channel `c` (excluding alpha):
    ```
    p_c = 0.5 + (p_c - 0.5) * (1.0 + value)
    ```
    Where `value` is the contrast adjustment factor (typically in the range [-1.0, 1.0]). A value of 0 means no change.
*   **Implementation:** `OperationContrast` in `core/operations/basic_adjustment_operations/`.
*   **QML Model:** `ContrastModel` in `qt/core/models/operations/basic_adjustment_models/`.
*   **Fusion Support:** Implements `IOperationFusionLogic` interface with `appendToFusedPipeline` method for pipeline fusion optimization.

### Highlights

*   **Purpose:** Adjusts the luminosity of the brightest areas of the image.
*   **Formula (Approximation):** A mask based on pixel luminance is created. Pixels with luminance above a threshold are adjusted:
    ```
    adjustment_factor = mask(p) * value
    p_c = p_c + adjustment_factor
    ```
    The mask ensures only brighter pixels are significantly affected.
*   **Implementation:** `OperationHighlights` in `core/operations/basic_adjustment_operations/`.
*   **QML Model:** `HighlightsModel` in `qt/core/models/operations/basic_adjustment_models/`.
*   **Fusion Support:** Implements `IOperationFusionLogic` interface with `appendToFusedPipeline` method for pipeline fusion optimization.

### Shadows

*   **Purpose:** Adjusts the luminosity of the darkest areas of the image.
*   **Formula (Approximation):** A mask based on pixel luminance is created. Pixels with luminance below a threshold are adjusted:
    ```
    adjustment_factor = mask(p) * value
    p_c = p_c + adjustment_factor
    ```
    The mask ensures only darker pixels are significantly affected.
*   **Implementation:** `OperationShadows` in `core/operations/basic_adjustment_operations/`.
*   **QML Model:** `ShadowsModel` in `qt/core/models/operations/basic_adjustment_models/`.
*   **Fusion Support:** Implements `IOperationFusionLogic` interface with `appendToFusedPipeline` method for pipeline fusion optimization.

### Whites

*   **Purpose:** Adjusts the luminosity of the white point of the image.
*   **Formula (Approximation):** A mask based on pixel luminance is created. Pixels with very high luminance are adjusted:
    ```
    adjustment_factor = mask(p) * value
    p_c = p_c + adjustment_factor
    ```
    The mask ensures only the brightest pixels ("whites") are primarily affected.
*   **Implementation:** `OperationWhites` in `core/operations/basic_adjustment_operations/`.
*   **QML Model:** `WhitesModel` in `qt/core/models/operations/basic_adjustment_models/`.
*   **Fusion Support:** Implements `IOperationFusionLogic` interface with `appendToFusedPipeline` method for pipeline fusion optimization.

### Blacks

*   **Purpose:** Adjusts the luminosity of the black point of the image.
*   **Formula (Approximation):** A mask based on pixel luminance is created. Pixels with very low luminance are adjusted:
    ```
    adjustment_factor = mask(p) * value
    p_c = p_c + adjustment_factor
    ```
    The mask ensures only the darkest pixels ("blacks") are primarily affected.
*   **Implementation:** `OperationBlacks` in `core/operations/basic_adjustment_operations/`.
*   **QML Model:** `BlacksModel` in `qt/core/models/operations/basic_adjustment_models/`.
*   **Fusion Support:** Implements `IOperationFusionLogic` interface with `appendToFusedPipeline` method for pipeline fusion optimization.

## 🧮 Implementation Notes

*   **Core:** Operations are implemented as classes inheriting from `IOperation` in the `Core::Operations` namespace.
*   **Fusion Logic:** Operations also implement `IOperationFusionLogic` interface to support pipeline fusion optimization.
*   **Sequential Method:** Each operation maintains a `[[maybe_unused]] execute` method for sequential processing compatibility.
*   **QML Models:** UI-specific models inherit from `BaseAdjustmentModel` which provides common properties (`value`, `minimum`, `maximum`, `name`, `active`) and Qt infrastructure.
*   **Halide:** Many operations use the Halide library for efficient image processing on the CPU/GPU.
*   **Luminance:** Luminance is often approximated as `0.299*R + 0.587*G + 0.114*B` for mask generation.
*   **Pipeline Fusion:** Operations contribute their logic to combined computational graphs through the `appendToFusedPipeline` method, eliminating intermediate buffer copies.
*   