/**
 * @file working_image_gpu_halide.h
 * @brief Concrete implementation of IWorkingImageGPU
 * @author CaptureMoment Team
 * @date 2026
 */

#pragma once

#include "image_processing/gpu/interfaces/i_working_image_gpu.h"
#include "common/image_region.h"

#include <memory>

#include "Halide.h"

namespace CaptureMoment::Core {

namespace ImageProcessing {

/**
 * @brief Concrete implementation of WorkingImageGPU_Halide for image data stored in GPU memory using Halide.
 *
 * This class holds the image data within GPU memory, managed by a Halide::Buffer
 * configured for GPU execution. It inherits from WorkingImageHalide
 * and implements its specific Halide-based logic.
 */
class WorkingImageGPU_Halide final : public IWorkingImageGPU, public WorkingImageHalide {
public:
    /**
     * @brief Constructs a WorkingImageGPU_Halide object, optionally initializing it with an ImageRegion.
     *        The initial image data (if provided) is transferred to GPU memory.
     * @param initial_image Optional initial image data. If provided, it's transferred to GPU.
     *                      If not provided, the object starts with an invalid state (empty GPU buffer).
     */
    explicit WorkingImageGPU_Halide(std::shared_ptr<Common::ImageRegion> initial_image = nullptr);

    /**
     * @brief Virtual destructor.
     */
    ~WorkingImageGPU_Halide() override;

    // --- IWorkingImageHardware Interface Implementation ---
    // (Must implement all methods from IWorkingImageHardware)

    /**
     * @brief Updates the internal GPU image data from a CPU-based ImageRegion.
     *
     * This method transfers the data from the provided CPU image region
     * into the internal GPU memory buffer managed by this object.
     *
     * @param[in] cpu_image The source image data residing in CPU memory.
     * @return true if the update operation (transfer to GPU) was successful, false otherwise.
     */
    [[nodiscard]] bool updateFromCPU(const Common::ImageRegion& cpu_image) override;

    /**
     * @brief Exports the current internal GPU image data to a new CPU-based ImageRegion owned by the caller.
     *
     * This method transfers the data from the internal GPU memory buffer
     * to a newly allocated CPU memory buffer, creating a new ImageRegion instance.
     * The caller receives ownership of the returned shared pointer.
     *
     * @return A shared pointer to a **newly allocated** ImageRegion containing the copied image data
     *         transferred from GPU to CPU.
     *         Returns nullptr on failure (allocation, transfer, or copy error).
     */
    [[nodiscard]] std::shared_ptr<Common::ImageRegion> exportToCPUCopy() override;

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
    [[nodiscard]] bool isValid() const override { return m_halide_gpu_buffer.defined() && m_metadata_valid;};

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
     * @brief Updates the cached metadata from the GPU buffer.
     * @return true if successful, false otherwise.
     */
    bool updateCachedMetadata() const;
};

} // namespace ImageProcessing

} // namespace CaptureMoment::Core
