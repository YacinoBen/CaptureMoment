/**
 * @file i_pipeline_manager.h
 * @brief Abstract interface for managing a specific category of image processing.
 *
 * @details
 * This interface acts as a bridge between the application state (`StateImageManager`)
 * and the technical execution engines (`IPipelineExecutor`). A Manager handles
 * a specific domain (e.g., Adjustments, AI Effects) and hides the complexity of
 * the underlying executor.
 *
 * @author CaptureMoment Team
 * @date 2026
 */

#pragma once

#include "image_processing/interfaces/i_working_image_hardware.h"

namespace CaptureMoment::Core {
namespace Strategies {

/**
 * @interface IPipelineManager
 * @brief High-level interface for executing a processing strategy.
 */
class IPipelineManager {
public:
    /**
     * @brief Virtual destructor.
     */
    virtual ~IPipelineManager() = default;

    /**
     * @brief Executes the managed strategy on the working image.
     *
     * @details
     * This is the entry point called by the main rendering loop. The implementation
     * is responsible for ensuring the underlying executor is ready and dispatching the call.
     *
     * @param working_image The image to be processed.
     * @return true if the strategy was applied successfully.
     */
    [[nodiscard]] virtual bool execute(ImageProcessing::IWorkingImageHardware& working_image) = 0;

protected:
    /**
     * @brief Protected constructor.
     */
    IPipelineManager() = default;
};

} // namespace Strategies
} // namespace CaptureMoment::Core
