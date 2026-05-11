/**
 * @file working_image_cpu_default.h
 * @brief Default concrete implementation of IWorkingImageCPU using standard CPU memory.
 *
 * @details
 * This class is the simplest implementation of `WorkingImageHardware`.
 * It does not utilize specific acceleration hardware (like Halide or SIMD),
 * relying on standard C++ operations and OIIO (inherited from WorkingImageCPU).
 *
 * **Architecture:**
 * Unlike specific backends (like Halide), this class relies 100% on the ImageView
 * provided by the WorkingImageContext. It does not allocate or own any image memory itself.
 *
 * **Usage:**
 * This backend is typically used as a safe fallback if hardware-specific backends fail to load.
 *
 * @author CaptureMoment Team
 * @date 2026
 */

#pragma once

#include "image_processing/cpu/working_image_cpu.h"
#include "common/error_handling/core_error.h"

#include <memory>
#include <expected>

namespace CaptureMoment::Core {

namespace ImageProcessing {

/**
 * @class WorkingImageCPU_Default
 * @brief Fallback CPU backend operating directly on the Context's memory view.
 */
class WorkingImageCPU_Default final : public WorkingImageCPU {
public:
    /** @brief Default constructor. */
    WorkingImageCPU_Default() = default;

    /** @brief Virtual destructor. */
    ~WorkingImageCPU_Default() override = default;

    // ============================================================
    // IWorkingImageHardware Overrides
    // ============================================================

    /**
     * @brief No-op for Default CPU.
     * @details Since this backend operates directly on the Context's RAM via the bound view,
     * any changes made by the Context are already visible. No copy is required.
     */
    [[nodiscard]] std::expected<void, ErrorHandling::CoreError>
    updateFromCPU() override;

    /**
     * @brief Exports current working data to a new CPU-based ImageRegion.
     * @details Creates a deep copy of the data currently pointed to by the bound view.
     */
    [[nodiscard]] std::expected<std::unique_ptr<Common::ImageRegion>, ErrorHandling::CoreError>
    exportToCPUCopy() override;
};

} // namespace ImageProcessing

} // namespace CaptureMoment::Core
