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
     * @brief Checks if the CPU view AND the Halide buffer are valid.
     * @return true if both the CPU view and Halide buffer are valid.
     */
    [[nodiscard]] bool isValid() const override { return  WorkingImageCPU::isValid() && isHalideBufferValid(); };

    /**
     * @brief isOriginalHalideBufferValid
     * @return true if the Halide buffer for original data is valid (defined), false otherwise.
     */
    [[nodiscard]] bool isOriginalHalideBufferValid() const { return m_halide_original_buffer.defined(); }

    /**
     * @brief Returns a reference to the Halide buffer for original data (on CPU).
     * @return Reference to the Halide::Buffer<float> for original data.
     */
    [[nodiscard]] Halide::Buffer<float>& getOriginalHalideBuffer() { return m_halide_original_buffer; }

private:
    Halide::Buffer<float> m_halide_original_buffer; ///< Halide buffer for original data (on CPU)
};

} // namespace ImageProcessing

} // namespace CaptureMoment::Core
