/**
 * @file halide_operation_worker.h
 * @brief Concrete worker for Halide-based adjustment operations.
 *
 * @details
 * This class implements `IWorkerRequest` to execute Halide pipelines.
 * It acts as a lightweight wrapper around the `HalideOperationManager`.
 * It does not store operation data; it assumes the manager has already been
 * initialized with the necessary operations prior to execution.
 *
 * **Architecture:**
 * - **Stateless:** Does not own operation descriptors.
 * - **Contextual:** Retrieves the necessary manager from `PipelineContext` during execution.
 * - **Async Wrapper:** Wraps the synchronous manager execution into a `std::future`.
 *
 * @author CaptureMoment Team
 * @date 2026
 */

#pragma once

#include "workers/interfaces/i_worker_request.h"
#include "pipeline/pipeline_context.h"

#include <future>
#include <memory>

namespace CaptureMoment::Core {

namespace Workers {

/**
 * @class HalideOperationWorker
 * @brief Worker responsible for running pre-configured Halide pipelines.
 *
 * @details
 * This worker retrieves the `HalideOperationManager` from the context
 * and triggers the execution on the provided image.
 */
class HalideOperationWorker : public IWorkerRequest {
public:
    /**
     * @brief Constructor.
     * @details
     * Default constructor as this worker holds no state.
     */
    HalideOperationWorker() = default;

    /**
     * @brief Destructor.
     */
    ~HalideOperationWorker() override = default;

    /**
     * @brief Executes the Halide pipeline asynchronously.
     *
     * @details
     * 1. Retrieves the `HalideOperationManager` from the provided context.
     * 2. Invokes the manager's `execute` method on the working image.
     * 3. Wraps the result in a `std::future`.
     *
     * @param context Reference to the global pipeline infrastructure.
     * @param working_image Reference to the target image buffer.
     * @return `std::future<bool>` indicating success or failure of the execution.
     */
    [[nodiscard]] std::future<bool> execute(
        Pipeline::PipelineContext& context,
        ImageProcessing::IWorkingImageHardware& working_image) override;
};

} // namespace Workers

} // namespace CaptureMoment::Core*