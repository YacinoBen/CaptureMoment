/**
 * @file working_image_gpu_halide.h
 * @brief Concrete implementation of image working buffer on GPU using Halide for processing.
 * @author CaptureMoment Team
 * @date 2026
 */

#pragma once

#include "image_processing/gpu/working_image_gpu.h"
#include "image_processing/halide/working_image_halide.h"
#include "common/error_handling/core_error.h"
#include "image_processing/common/image_view.h"

#include <memory>
#include <expected>

namespace CaptureMoment::Core {

namespace ImageProcessing {

/**
 * @class WorkingImageGPU_Halide
 * @brief Concrete implementation of WorkingImageGPU using Halide for processing.
 *
 * @details
 * Architecture:
 * - Inherits WorkingImageGPU: Provides the GPU-specific interface and common GPU logic.
 * - Inherits WorkingImageHalide: Provides the shared Halide buffer logic.
 *
 * GPU Specifics:
 * - Manages Host-to-Device (updateFromCPU) and Device-to-Host (exportToCPUCopy) transfers.
 * - Uses `std::expected` for robust error reporting of GPU transfers.
 */
class WorkingImageGPU_Halide final : public WorkingImageGPU, public WorkingImageHalide {
public:
    /** @brief Default constructor  */
    WorkingImageGPU_Halide() = default;

    ~WorkingImageGPU_Halide() override = default;

    /**
     * @brief Binds the view and initializes the Halide buffer on the device.
     */
    [[nodiscard]] bool bindView(const ImageView& view) override;

    /**
     * @brief Updates internal image data by uploading from the CPU view to the GPU device.
     *
     * @return std::expected<void, CoreError>. Void on success, error on failure.
     */
    [[nodiscard]] std::expected<void, ErrorHandling::CoreError>
    updateFromCPU() override;

    /**
     * @brief Exports current internal image data by downloading from the GPU device to Host memory.
     *
     * @return std::expected<std::unique_ptr<Common::ImageRegion>, CoreError>.
     */
    [[nodiscard]] std::expected<std::unique_ptr<Common::ImageRegion>, ErrorHandling::CoreError>
    exportToCPUCopy() override;

    /**
     * @brief Checks if the GPU view AND the Halide buffer are valid.
     */
    [[nodiscard]] bool isValid() const override;

    /**
     * @brief Exports a downscaled version of the image directly from GPU.
     *
     * @details
     * For GPU: Performs downsample on GPU, then transfers only the small result.
     * For CPU: Performs downsample on CPU.
     *
     * This is the preferred method for display purposes.
     */
    [[nodiscard]] std::expected<std::unique_ptr<Common::ImageRegion>, ErrorHandling::CoreError>
    downsample(Common::ImageDim target_width, Common::ImageDim target_height) override;
};

} // namespace ImageProcessing

} // namespace CaptureMoment::Core
