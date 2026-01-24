/**
 * @file working_image_cpu_halide.h
 * @brief Concrete implementation of IWorkingImageCPU using Halide for CPU processing.
 * @author CaptureMoment Team
 * @date 2026
 */

#pragma once
#include "image_processing/halide/working_image_halide.h"
#include "image_processing/cpu/interfaces/i_working_image_cpu.h"
#include "common/error_handling/core_error.h"

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

class WorkingImageCPU_Halide final : public IWorkingImageCPU, public WorkingImageHalide {
public:
    /**
     * @brief Constructs a WorkingImageCPU_Halide.
     * @param initial_image Optional initial image data. Ownership is transferred via move.
     */
    explicit WorkingImageCPU_Halide(std::unique_ptr<Common::ImageRegion> initial_image = nullptr);

    /**
     * @brief Virtual destructor.
     */
    ~WorkingImageCPU_Halide() override;

    /**
     * @brief Updates internal image data by COPYING from a CPU-based ImageRegion.
     *
     * @param cpu_image The source image data (const reference).
     * @return std::expected<void, std::error_code>. Void on success, error code on failure.
     */
    [[nodiscard]] std::expected<void, std::error_code>
    updateFromCPU(const Common::ImageRegion& cpu_image) override;


    /**
     * @brief Updates internal image data by MOVING from a CPU-based ImageRegion.
     *
     * Preferred method during initialization. Transfers ownership of the buffer
     * without copying bytes.
     *
     * @param cpu_image The source image data (rvalue reference).
     * @return std::expected<void, std::error_code>. Void on success, error code on failure.
     */
    [[nodiscard]] std::expected<void, std::error_code>
    updateFromCPU(Common::ImageRegion&& cpu_image);

    /**
     * @brief Exports internal data to a new ImageRegion.
     *
     * Performs a deep copy. The caller receives unique ownership.
     *
     * @return std::expected<std::unique_ptr<Common::ImageRegion>, std::error_code>.
     *         Unique pointer to data on success, error code on failure.
     */
    [[nodiscard]] std::expected<std::unique_ptr<Common::ImageRegion>, std::error_code>
    exportToCPUCopy() override;

    /**
     * @brief Gets the dimensions (width, height) of the internal Halide buffer.
     *
     * @return A pair containing the width (first) and height (second) of the image.
     *         Returns {0, 0} if the internal Halide buffer is invalid or not loaded.
     */
    [[nodiscard]] std::pair<size_t, size_t> getSize() const override;

    /**
     * @brief Gets the number of color channels of the internal Halide buffer.
     *
     * @return The number of channels (e.g., 3 for RGB, 4 for RGBA). Returns 0
     *         if the internal Halide buffer is invalid or not loaded.
     */
    [[nodiscard]] size_t getChannels() const override;

    /**
     * @brief Gets the total number of pixels in the internal Halide buffer.
     *
     * @return The product of width and height. Returns 0 if the internal Halide buffer is invalid or not loaded.
     */
    [[nodiscard]] size_t getPixelCount() const override;

    /**
     * @brief Gets the total number of data elements (pixels * channels) in the internal Halide buffer.
     *
     * @return The product of pixel count and channel count. Returns 0 if the internal Halide buffer
     *         or channel count is invalid.
     */
    [[nodiscard]] size_t getDataSize() const override;

    /**
     * @brief Checks if the internal Halide buffer data is in a valid state.
     *
     * @return true if the internal Halide buffer is allocated and contains valid data, false otherwise.
     */
    [[nodiscard]] bool isValid() const override { return m_halide_buffer.defined(); };

    /**
     * @brief Gets the memory type where the image data resides.
     *
     * @return MemoryType::CPU_RAM, indicating the data is stored in main CPU RAM via Halide.
     */
    [[nodiscard]] Common::MemoryType getMemoryType() const override { return Common::MemoryType::CPU_RAM; };

private:

    /**
     * @brief Helper to convert Halide buffer to ImageRegion.
     *
     * @return std::expected<std::unique_ptr<Common::ImageRegion>, std::error_code>.
     */
    [[nodiscard]] std::expected<std::unique_ptr<Common::ImageRegion>, std::error_code>
    convertHalideToImageRegion();
};

} // namespace ImageProcessing

} // namespace CaptureMoment::Core
