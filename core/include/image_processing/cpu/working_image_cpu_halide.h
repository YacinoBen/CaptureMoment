/**
 * @file working_image_cpu_halide.h
 * @brief Concrete implementation of IWorkingImageCPU using Halide for CPU processing.
 * @author CaptureMoment Team
 * @date 2026
 */

#pragma once
#include "image_processing/cpu/interfaces/i_working_image_cpu.h"

#include <memory>
#include <vector>

#include "Halide.h"

namespace CaptureMoment::Core {

namespace ImageProcessing {

/**
 * @brief Concrete implementation of IWorkingImageCPU for image data stored in CPU RAM using Halide.
 *
 * This class holds the image data within standard CPU memory (RAM), managed
 * by a Halide::Buffer for optimized CPU processing. It inherits from IWorkingImageCPU
 * and implements its specific Halide-based logic.
 */
class WorkingImageCPU_Halide final : public IWorkingImageCPU {
public:
    /**
     * @brief Constructs a WorkingImageCPU_Halide object, optionally initializing it with an ImageRegion.
     * @param initial_image Optional initial image data. If not provided, the object starts invalid.
     */
    explicit WorkingImageCPU_Halide(std::shared_ptr<Common::ImageRegion> initial_image = nullptr);

    /**
     * @brief Virtual destructor.
     */
    ~WorkingImageCPU_Halide() override;

    // --- IWorkingImageHardware Interface Implementation ---
    // (Must implement all methods from IWorkingImageHardware)

    /**
     * @brief Updates the internal image data from a CPU-based ImageRegion.
     *
     * This method creates a new Halide::Buffer<float> based on the dimensions
     * of the input ImageRegion and copies the pixel data from the ImageRegion
     * into the Halide buffer's host memory.
     *
     * @param[in] cpu_image The source image data residing in CPU memory.
     * @return true if the update operation was successful, false otherwise.
     */
    [[nodiscard]] bool updateFromCPU(const Common::ImageRegion& cpu_image) override;

    /**
     * @brief Exports the current internal Halide image data to a new CPU-based ImageRegion owned by the caller.
     *
     * This method creates a **new** ImageRegion instance on the heap and copies
     * the pixel data from the internal Halide buffer's host memory into it.
     * The caller receives ownership of the returned shared pointer.
     *
     * @return A shared pointer to a **newly allocated** ImageRegion containing the copied image data
     *         from the internal Halide buffer.
     *         Returns nullptr on failure (allocation or copy error).
     */
    [[nodiscard]] std::shared_ptr<Common::ImageRegion> exportToCPUCopy() override;

    /**
     * @brief Exports a shared reference to the current internal image data.
     *
     * Because the internal data is managed by a Halide::Buffer, sharing it directly
     * as an ImageRegion without copying is not straightforward. This method
     * currently returns nullptr.
     *
     * @return A shared pointer to a **newly allocated** ImageRegion containing a copy of the image data
     *         from the internal Halide buffer (effectively the same as exportToCPUCopy).
     *         Returns nullptr on failure.
     */
    [[nodiscard]] std::shared_ptr<Common::ImageRegion> exportToCPUShared() const override;

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
     * @brief Internal Halide buffer holding the CPU image data.
     * Managed directly by this object. Halide::Buffer handles its own memory allocation/deallocation.
     */
    Halide::Buffer<float> m_halide_buffer;

    
};

} // namespace ImageProcessing

} // namespace CaptureMoment::Core
