/**
 * @file working_image_cpu_default.h
 * @brief Default concrete implementation of IWorkingImageCPU using standard CPU memory (std::vector<float>).
 * @author CaptureMoment Team
 * @date 2026
 */

#pragma once

#include "image_processing/cpu/interfaces/i_working_image_cpu.h"

#include <memory>
#include <vector>

namespace CaptureMoment::Core {
    
namespace ImageProcessing {

/**
 * @brief Default concrete implementation of IWorkingImageCPU for image data stored in standard CPU RAM.
 *
 * This class holds the image data within standard CPU memory (RAM) using a
 * Common::ImageRegion structure. It implements the IWorkingImageCPU interface
 * specifically for standard CPU-based storage and operations.
 */
class WorkingImageCPU_Default final : public IWorkingImageCPU {
public:
    /**
     * @brief Constructs a WorkingImageCPU_Default object, optionally initializing it with an ImageRegion.
     * @param initial_image Optional initial image data. If not provided, the object starts invalid.
     */
    explicit WorkingImageCPU_Default(std::shared_ptr<Common::ImageRegion> initial_image = nullptr);

    /**
     * @brief Virtual destructor.
     */
    ~WorkingImageCPU_Default() override;

    // --- IWorkingImageHardware Interface Implementation ---
    // (Must implement all methods from IWorkingImageHardware)

    /**
     * @brief Updates the internal image data from a CPU-based ImageRegion.
     *
     * This method copies the data from the provided CPU image region directly
     * into the internal CPU memory buffer managed by this object.
     *
     * @param[in] cpu_image The source image data residing in CPU memory.
     * @return true if the update operation was successful (data copied), false otherwise.
     */
    [[nodiscard]] bool updateFromCPU(const Common::ImageRegion& cpu_image) override;

    /**
     * @brief Exports the current internal image data to a new CPU-based ImageRegion owned by the caller.
     *
     * This method creates a **new** ImageRegion instance on the heap and copies
     * the internal image data into it. The caller receives ownership of the returned
     * shared pointer and is responsible for its lifetime.
     * This involves a **deep copy** of the image data.
     *
     * @return A shared pointer to a **newly allocated** ImageRegion containing the copied image data.
     *         Returns nullptr on failure (allocation or copy error).
     */
    [[nodiscard]] std::shared_ptr<Common::ImageRegion> exportToCPUCopy() override;

    /**
     * @brief Exports a shared reference to the current internal image data.
     *
     * This method provides a shared pointer to the internal ImageRegion object.
     * This is a **shallow copy** operation (increases the reference count).
     * The returned shared pointer points to the same underlying data managed by
     * this WorkingImageCPU_Default object.
     *
     * @return A shared pointer to the **internal** ImageRegion object (shallow copy).
     *         Returns nullptr if the internal data is invalid.
     */
    [[nodiscard]] std::shared_ptr<Common::ImageRegion> exportToCPUShared() const override;

    /**
     * @brief Gets the dimensions (width, height) of the internal image data.
     *
     * @return A pair containing the width (first) and height (second) of the image.
     *         Returns {0, 0} if the internal image data is invalid or not loaded.
     */
    [[nodiscard]] std::pair<size_t, size_t> getSize() const override;

    /**
     * @brief Gets the number of color channels of the internal image data.
     *
     * @return The number of channels (e.g., 3 for RGB, 4 for RGBA). Returns 0
     *         if the internal image data is invalid or not loaded.
     */
    [[nodiscard]] size_t getChannels() const override;

    /**
     * @brief Gets the total number of pixels in the internal image data.
     *
     * @return The product of width and height. Returns 0 if the internal image data is invalid or not loaded.
     */
    [[nodiscard]] size_t getPixelCount() const override;

    /**
     * @brief Gets the total number of data elements (pixels * channels) in the internal image data.
     *
     * @return The product of pixel count and channel count. Returns 0 if the internal image data is invalid or not loaded.
     */
    [[nodiscard]] size_t getDataSize() const override;

    /**
     * @brief Checks if the internal image data is in a valid state.
     *
     * @return true if the internal ImageRegion is loaded and contains valid data, false otherwise.
     */
    [[nodiscard]] bool isValid() const override;

    /**
     * @brief Gets the memory type where the image data resides.
     *
     * @return MemoryType::CPU_RAM, indicating the data is stored in main CPU RAM.
     */
    [[nodiscard]] Common::MemoryType getMemoryType() const override;

private:
    /**
     * @brief Shared pointer to the internal ImageRegion holding the CPU image data.
     */
    std::shared_ptr<Common::ImageRegion> m_image_data;
};

} // namespace ImageProcessing

} // namespace CaptureMoment::Core
