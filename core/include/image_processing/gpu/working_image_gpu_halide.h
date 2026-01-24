/**
 * @file working_image_gpu_halide.h
 * @brief Concrete implementation of IWorkingImageGPU
 * @author CaptureMoment Team
 * @date 2026
 */

#pragma once

#include "image_processing/gpu/interfaces/i_working_image_gpu.h"
#include "image_processing/halide/working_image_halide.h"
#include "common/error_handling/core_error.h"

#include <memory>
#include <expected>

namespace CaptureMoment::Core {

namespace ImageProcessing {

/**
 * @brief Concrete implementation of WorkingImageGPU_Halide for image data stored on GPU.
 *
 * Architecture:
 * - Inherits IWorkingImageGPU: Provides the interface contract for GPU images.
 * - Inherits WorkingImageHalide: Provides the shared Halide buffer logic.
 *
 * GPU Specifics:
 * - Manages Host-to-Device (updateFromCPU) and Device-to-Host (exportToCPUCopy) transfers.
 * - Caches metadata (width, height, channels) because querying GPU memory directly
 *   during getters is expensive or unreliable.
 * - Uses `std::expected` for robust error reporting of GPU transfers.
 */

class WorkingImageGPU_Halide final : public IWorkingImageGPU, public WorkingImageHalide {
public:
    /**
     * @brief Constructs a WorkingImageGPU_Halide.
     * @param initial_image Optional initial image data. Ownership is transferred via move.
     */
    explicit WorkingImageGPU_Halide(std::unique_ptr<Common::ImageRegion> initial_image = nullptr);

    /**
     * @brief Virtual destructor.
     */
    ~WorkingImageGPU_Halide() override;

    // ============================================================
    // IWorkingImageHardware Interface Implementation
    // ============================================================

    /**
     * @brief Updates internal image data by COPYING from a CPU-based ImageRegion.
     * Includes a copy to the GPU device.
     *
     * @param cpu_image The source image data.
     * @return std::expected<void, std::error_code>.
     */
    [[nodiscard]] std::expected<void, std::error_code>
    updateFromCPU(const Common::ImageRegion& cpu_image) override;


    /**
     * @brief Updates internal image data by MOVING from a CPU-based ImageRegion.
     * Transfers ownership of the CPU buffer, then copies to GPU device.
     *
     * @param cpu_image The source image data (rvalue reference).
     * @return std::expected<void, std::error_code>.
     */
    [[nodiscard]] std::expected<void, std::error_code>
    updateFromCPU(Common::ImageRegion&& cpu_image);

    /**
     * @brief Exports current internal image data to a new CPU-based ImageRegion.
     * Includes a copy from the GPU device to Host memory.
     *
     * @return std::expected<std::unique_ptr<Common::ImageRegion>, std::error_code>.
     */
    [[nodiscard]] std::expected<std::unique_ptr<Common::ImageRegion>, std::error_code>
    exportToCPUCopy() override;

    /**
     * @brief Gets the dimensions (width, height) of the internal GPU image data.
     *
     * @return A pair containing the width (first) and height (second) of the image.
     *         Returns {0, 0} if the internal GPU image data is invalid or not loaded.
     */
    [[nodiscard]] std::pair<size_t, size_t> getSize() const override;

    /**
     * @brief Gets the number of color channels of the internal GPU image data.
     *
     * @return The number of channels (e.g., 3 for RGB, 4 for RGBA). Returns 0
     *         if the internal GPU image data is invalid or not loaded.
     */
    [[nodiscard]] size_t getChannels() const override;

    /**
     * @brief Gets the total number of pixels in the internal GPU image data.
     *
     * @return The product of width and height. Returns 0 if the internal GPU image data is invalid or not loaded.
     */
    [[nodiscard]] size_t getPixelCount() const override;

    /**
     * @brief Gets the total number of data elements (pixels * channels) in the internal GPU image data.
     *
     * @return The product of pixel count and channel count. Returns 0 if the internal GPU image data
     *         or channel count is invalid.
     */
    [[nodiscard]] size_t getDataSize() const override;

    /**
     * @brief Checks if the internal GPU image data is in a valid state.
     *
     * @return true if the internal GPU buffer is allocated and contains valid data, false otherwise.
     */
    [[nodiscard]] bool isValid() const override { return m_halide_buffer.defined() && m_metadata_valid;};

    /**
     * @brief Gets the memory type where the image data resides.
     *
     * @return MemoryType::GPU_MEMORY, indicating the data is stored in GPU memory.
     */
    [[nodiscard]] Common::MemoryType getMemoryType() const override { return Common::MemoryType::GPU_MEMORY;};

private:

    /**
     * @brief Cached dimensions and channels of the GPU buffer.
     *        Helps avoid querying the GPU buffer repeatedly for metadata.
     */
    mutable size_t m_cached_width{0};
    mutable size_t m_cached_height{0};
    mutable size_t m_cached_channels{0};

    /**
     * @brief Flag indicating if the cached metadata is valid.
     */
    mutable bool m_metadata_valid{false};

    /**
     * @brief Updates the internal cache of dimensions.
     * GPU getters should use this cache to avoid device queries.
     */
    void updateCachedMetadata(const Common::ImageRegion& region);
};

} // namespace ImageProcessing

} // namespace CaptureMoment::Core
