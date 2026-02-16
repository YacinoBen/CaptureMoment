/**
 * @file i_halide_pipeline_executor.h
 * @brief Abstract base class for Halide-optimized pipeline executors.
 *
 * @details
 * This class provides a common foundation for all Halide executors.
 * It enforces the usage of a standard `ImageParam` input defined as **Float32, 4 channels (RGBA)**.
 *
 * **Architecture Rule:**
 * All pipelines in the application operate on 4-channel float images.
 * This ensures consistency between adjustments, filters, and AI/Halide operations.
 *
 * @author CaptureMoment Team
 * @date 2026
 */

#pragma once


#include "Halide.h"

namespace CaptureMoment::Core {

namespace Pipeline {

/**
 * @class IHalidePipelineExecutor
 * @brief Base class for Halide executors providing shared 4-channel input parameter.
 *
 * @details
 * By defining `m_input` here with fixed dimensions (Type Float32, 4 Channels),
 * we ensure that any derived executor (Adjustments, Sky, Filters) is compatible
 * with the application's internal image format without extra conversion logic.
 */
class IHalidePipelineExecutor {
public:
    /**
     * @brief Virtual destructor.
     */
    virtual ~IHalidePipelineExecutor() = default;

    /**
     * @brief Executes the compiled pipeline directly on a Halide buffer.
     *
     * @param[in,out] buffer The Halide buffer pointing to image data.
     *                       Must be a 4-channel Float32 buffer to match `m_input`.
     * @return true if pipeline executed successfully.
     */
    [[nodiscard]] virtual bool executeOnHalideBuffer(Halide::Buffer<float>& buffer) = 0;

protected:
    /**
     * @brief Constructor.
     * @details
     * Initializes the shared ImageParam with the application standard:
     * - Type: Float(32)
     * - Dimensions: 4 (x, y, c) where c is {R, G, B, A}
     */
    IHalidePipelineExecutor() 
        : m_input(Halide::Float(32), 4){}

    /**
     * @brief The shared Halide Input Parameter.
     * @details
     * Fixed to 4 dimensions (0, 1, 2, 3 -> x, y, c, ?).
     * All pipeline definitions in derived classes should start from this `m_input`.
     * 
     * Example usage in derived class:
     * @code
     * Halide::Func output;
     * output(x, y, c) = m_input(x, y, c); // Passes through 4 channels
     * @endcode
     */
    Halide::ImageParam m_input;
};

} // namespace Pipeline

} // namespace CaptureMoment::Core
