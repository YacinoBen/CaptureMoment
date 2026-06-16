/**
 * @file working_image_gpu.h
 * @brief Image working buffers stored on the GPU
 * @author CaptureMoment Team
 * @date 2026
 */

#pragma once

#include "image_processing/interfaces/i_working_image_hardware.h"

namespace CaptureMoment::Core {

namespace ImageProcessing {

/**
 * @brief Class extending IWorkingImageHardware for GPU-specific implementations.
 *
 * @details
 * This interface defines the base contract for all working image implementations
 * that store their data on the GPU. It implements the downsample contract
 * by delegating the math to a generic IGpuComputeBackend.
 */

class WorkingImageGPU : public IWorkingImageHardware {
public:
    /** @brief Constructor */
    WorkingImageGPU() = default;

    /**
     * @brief Virtual destructor for safe inheritance and polymorphic deletion.
     */
    virtual ~WorkingImageGPU() = default;

    /**
     * @brief Checks if the GPU buffer is valid and allocated.
     */
    [[nodiscard]] bool isValid() const override;

    /**
     * @brief Gets the memory type where data resides.
     * @return Common::MemoryType::GPU_MEMORY for GPU-based working images.
     */
    [[nodiscard]] Common::MemoryType getMemoryType() const override {
        return Common::MemoryType::GPU_MEMORY;
    }

    /**
     * @brief Exports current internal image data by downloading from the GPU device to Host memory.
     *
     * @return std::expected<std::unique_ptr<Common::ImageRegion>, CoreError>.
     */
    [[nodiscard]] std::expected<std::unique_ptr<Common::ImageRegion>, ErrorHandling::CoreError>
    exportToCPUCopy() override;


    /**
     * @brief Transfers the image data to GPU VRAM.
     * @return std::expected<void, std::error_code> Success or error.
     */
    [[nodiscard]] virtual std::expected<void, ErrorHandling::CoreError>
    transferToVRAM() = 0;

protected:
    /**
     * @brief A pure method that specific backends (Halide, Vulkan, etc.) must implement.
     * @details Its only but: force the synchronization VRAM
     */
    [[nodiscard]] virtual bool downloadDeviceToHost() = 0;
};

} // namespace ImageProcessing

} // namespace CaptureMoment::Core
