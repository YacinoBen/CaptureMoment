/**
 * @file working_image_cpu_halide.h
 * @brief implementation of of image working buffer on CPU using Halide for processing.
 * @author CaptureMoment Team
 * @date 2026
 */

#pragma once
#include "image_processing/halide/working_image_halide.h"
#include "image_processing/cpu/working_image_cpu.h"
#include "image_processing/common/image_view.h"

#include <memory>
#include <expected>

namespace CaptureMoment::Core {

namespace ImageProcessing {
/**
 * @brief Updates the internal image data from a CPU-based ImageRegion.
 *
 * @param cpu_image The source image data.
 * @return std::expected<void, std::error_code>.
 *         Returns {} (void) on success, or an error code on failure.
 */

class WorkingImageCPU_Halide final : public WorkingImageCPU, public WorkingImageHalide {
public:
    /** @brief Default constructor  */
    WorkingImageCPU_Halide() = default;

    ~WorkingImageCPU_Halide() override = default;

    /**
     * @brief Binds the view and initializes the zero-copy Halide buffer.
     * @return true if the view is valid and Halide buffer initialized successfully.
     */
    [[nodiscard]] bool bindView(const ImageView& view) override;


    /**
     * @brief Exports current internal image data to a new CPU-based ImageRegion..
     *
     * @return std::expected<std::unique_ptr<Common::ImageRegion>,  ErrorHandling::CoreError>
     */
    [[nodiscard]] std::expected<std::unique_ptr<Common::ImageRegion>,  ErrorHandling::CoreError>
    exportToCPUCopy() override;

    /**
     * @brief Checks if the CPU view AND the Halide buffer are valid.
     * @return true if both the CPU view and Halide buffer are valid.
     */
    [[nodiscard]] bool isValid() const override { return  WorkingImageCPU::isValid() && isHalideBufferValid(); };
private:

    /**
     * @brief Helper to convert Halide buffer to ImageRegion.
     *
     * @return std::expected<std::unique_ptr<Common::ImageRegion>, std::error_code>.
     */
    [[nodiscard]] std::expected<std::unique_ptr<Common::ImageRegion>, ErrorHandling::CoreError>
    convertHalideToImageRegion();
};

} // namespace ImageProcessing

} // namespace CaptureMoment::Core
