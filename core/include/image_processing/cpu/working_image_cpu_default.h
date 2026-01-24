/**
 * @file working_image_cpu_default.h
 * @brief Default concrete implementation of IWorkingImageCPU using standard CPU memory (std::vector<float>).
 *
 * This class holds image data within standard CPU memory (RAM) using a
 * Common::ImageRegion structure. It implements IWorkingImageCPU interface
 * specifically for standard CPU-based storage and operations.
 *
 * @author CaptureMoment Team
 * @date 2026
 */

#pragma once

#include "image_processing/cpu/interfaces/i_working_image_cpu.h"
#include "common/error_handling/core_error.h"
#include <memory>
#include <expected>
#include <utility>

namespace CaptureMoment::Core {

namespace ImageProcessing {

/**
 * @class WorkingImageCPU_Default
 * @brief Default concrete implementation of IWorkingImageCPU for image data stored in standard CPU RAM.
 *
 * @details
 * This class serves as simplest implementation of `IWorkingImageHardware`.
 * It stores data directly in a `Common::ImageRegion` (which wraps a `std::vector<float>`).
 * It does not utilize specific acceleration hardware (like Halide or SIMD) directly,
 * relying on standard C++ operations.
 *
 * **Memory Management:**
 * The class holds the image data internally using `std::shared_ptr<Common::ImageRegion>`.
 * This allows the `exportToCPUShared()` method to provide a non-owning, efficient reference
 * to the internal data for read-only operations elsewhere in the pipeline.
 *
 * **Usage:**
 * This backend is typically used as a fallback, for simple operations,
 * or when specific hardware optimizations are not required or available.
 */
class WorkingImageCPU_Default final : public IWorkingImageCPU {
public:
    /**
     * @brief Constructs a WorkingImageCPU_Default object.
     *
     * @details
     * Accepts a `std::unique_ptr` to transfer ownership of the initial image data.
     * This enables Move Semantics, avoiding a deep copy of the pixel data during initialization.
     *
     * @param initial_image Optional initial image data (unique_ptr).
     *                       If not provided or invalid, the object starts in an invalid state.
     */
    explicit WorkingImageCPU_Default(std::unique_ptr<Common::ImageRegion> initial_image = nullptr);

    /**
     * @brief Virtual destructor.
     */
    ~WorkingImageCPU_Default() override = default;

    // ============================================================
    // IWorkingImageHardware Interface Implementation
    // ============================================================

    /**
     * @brief Updates internal image data from a CPU-based ImageRegion.
     *
     * @details
     * This method creates a new internal copy of the provided image.
     * Since both source and destination are CPU-based, this is a standard memory copy.
     *
     * @param cpu_image The source image data residing in CPU memory.
     * @return std::expected<void, std::error_code> Success or error code.
     */
    [[nodiscard]] std::expected<void, std::error_code>
    updateFromCPU(const Common::ImageRegion& cpu_image) override;

    /**
     * @brief Exports current internal image data to a new CPU-based ImageRegion.
     *
     * @details
     * This method creates a **new** ImageRegion instance on the heap and copies
     * the internal image data into it. The caller receives unique ownership of the result.
     *
     * @return std::expected<std::unique_ptr<Common::ImageRegion>, std::error_code>
     *         Unique pointer to copied data on success, or error code on failure.
     */
    [[nodiscard]] std::expected<std::unique_ptr<Common::ImageRegion>, std::error_code>
    exportToCPUCopy() override;

    /**
     * @brief Exports a shared reference to current internal image data.
     *
     * @details
     * This method provides a shared pointer to the internal ImageRegion object.
     * This is a **shallow copy** operation (increases reference count).
     * The returned shared pointer points to the same underlying data managed by
     * this WorkingImageCPU_Default object.
     *
     * @return std::expected<std::shared_ptr<Common::ImageRegion>, std::error_code>
     *         Shared pointer to internal data on success, or error code if invalid.
     */
    [[nodiscard]] std::expected<std::shared_ptr<Common::ImageRegion>, std::error_code>
    exportToCPUShared() const;

    /**
     * @brief Gets dimensions (width, height) of internal image data.
     *
     * @return std::pair<size_t, size_t> Width and height. Returns {0, 0} if invalid.
     */
    [[nodiscard]] std::pair<size_t, size_t> getSize() const override;

    /**
     * @brief Gets number of color channels.
     *
     * @return size_t Number of channels. Returns 0 if invalid.
     */
    [[nodiscard]] size_t getChannels() const override;

    /**
     * @brief Gets total number of pixels.
     *
     * @return size_t width * height. Returns 0 if invalid.
     */
    [[nodiscard]] size_t getPixelCount() const override;

    /**
     * @brief Gets total number of data elements (pixels * channels).
     *
     * @return size_t Total size. Returns 0 if invalid.
     */
    [[nodiscard]] size_t getDataSize() const override;

    /**
     * @brief Checks if internal image data is valid.
     *
     * @return true if internal ImageRegion is loaded and contains valid data, false otherwise.
     */
    [[nodiscard]] bool isValid() const override
    {
        return m_image_data && m_image_data->isValid();
    };

    /**
     * @brief Gets memory type where image data resides.
     *
     * @return MemoryType::CPU_RAM.
     */
    [[nodiscard]] Common::MemoryType getMemoryType() const override
    {
        return Common::MemoryType::CPU_RAM;
    };

private:
    /**
     * @brief Shared pointer to internal ImageRegion holding CPU image data.
     *
     * @details
     * Stored as shared to allow efficient `exportToCPUShared()` functionality
     * without forcing deep copies or complex ownership transfers.
     */
    std::shared_ptr<Common::ImageRegion> m_image_data;
};

} // namespace ImageProcessing

} // namespace CaptureMoment::Core
