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
 * - Inherits WorkingImageGPU: Provides common GPU logic.
 * - Inherits WorkingImageHalide: Provides the shared Halide buffer logic.
 *
 * GPU Specifics:
 * - Manages Host-to-Device (transfertToVRAM) and Device-to-Host (exportToCPUCopy) transfers.
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
     * @brief Transfers the current image data to GPU VRAM With Halide.
     *
     * @return std::expected<void, CoreError> indicating success or failure of the transfer.
     */
    [[nodiscard]] std::expected<void, ErrorHandling::CoreError>
    transferToVRAM() override;

    /**
     * @brief Checks if the GPU view AND the Halide buffer are valid.
     */
    [[nodiscard]] bool isValid() const override;

    /**
     * @brief Resets the execution buffer on the GPU to prepare for a new Halide pipeline execution.
     */
    void resetExecutionBuffer();

    /**
     * @brief Provides access to the Halide buffer used for GPU execution.
     * @return Reference to the Halide::Buffer<float> used for GPU processing.
     */
    [[nodiscard]] Halide::Buffer<float>& getExecutionBuffer();

    /**
     * @brief Checks if the original GPU buffer is valid.
     */
    [[nodiscard]] bool isOriginalHalideBufferValid() const { return m_original_halide_buffer.defined(); };

    /**
     * @brief Provides access to the Halide buffer containing the original image on GPU.
     * @return Reference to the Halide::Buffer<float> used as input source.
     */
    [[nodiscard]] Halide::Buffer<float>& getOriginalHalideBuffer() { return m_original_halide_buffer; };

    /**
     * @brief Synchronizes the GPU buffer back to Host RAM.
     * @details Called only during a "Commit" or final export to update the CPU state.
     */
    void syncToHostRAM();

    /**
     * @brief Downsamples the current image to the target dimensions using Halide on GPU.
     *
     * @param target_width The desired width of the downsampled image.
     * @param target_height The desired height of the downsampled image.
     * @return std::expected<std::unique_ptr<Common::ImageRegion>, CoreError>
     *         containing the downsampled image region or an error code.
     */
    [[nodiscard]] std::expected<std::unique_ptr<Common::ImageRegion>, ErrorHandling::CoreError>
    downsample(Common::ImageDim target_width, Common::ImageDim target_height) override;

protected:
    [[nodiscard]] bool downloadDeviceToHost() override;

private:
    /**
     * @brief Helper to initialize the Halide buffer with the current view data.
     *
     * This is called during bindView and before GPU transfers to ensure the buffer
     * always references the correct data.
     */
    void initDataForHalide();

    /**
     * @brief  Builds the Halide downsample pipeline for GPU execution.
     */
    void buildDownsamplePipeline();

    // --- GPU Memory Management ---
    Halide::Buffer<float> m_original_halide_buffer; ///<< Halide buffer for original data (on GPU)

    // --- Pipelines Halide ---
    Halide::Pipeline m_reset_pipeline; ///< Halide pipeline for resetting the working buffer from the original buffer
    Halide::Pipeline m_downsample_pipeline; ///< Halide pipeline for downsampling

    // --- Downsample Pipelines ---
    Halide::ImageParam m_downsample_input{Halide::Float(32), 3, "downsample_src"}; ///< Halide input parameter for downsample pipeline
    Halide::Param<float> m_downsample_scale_x{"downsample_scale_x"}; ///< Halide parameter for downsample scale in x-direction
    Halide::Param<float> m_downsample_scale_y{"downsample_scale_y"}; ///< Halide parameter for downsample scale in y-direction
    Halide::Buffer<float> m_downsample_src_buffer; ///< Halide buffer for downsample source data (points to working GPU buffer)

    bool m_downsample_built{false}; ///< Flag indicating if the downsample pipeline has been built.
    bool m_pipelines_initialized{false}; ///< Flag indicating if the pipelines have been initialized.

};

} // namespace ImageProcessing

} // namespace CaptureMoment::Core
