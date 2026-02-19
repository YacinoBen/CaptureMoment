/**
 * @file i_worker_request.h
 * @brief Abstract interface for asynchronous image processing workers.
 *
 * @details
 * This interface defines the contract for any specific image processing task (Worker).
 * It follows the Intent pattern: the caller (StateImageManager) invokes the worker,
 * and the worker handles the specific execution logic and data.
 *
 * **Architecture:**
 * - **Async Execution:** The primary method `execute` returns a `std::future<bool>`, allowing
 *   the implementation to handle threading (e.g., via `std::async` or a thread pool) internally.
 * - **Context Injection:** Receives a `PipelineContext` to access compiled managers and infrastructure.
 * - **Data Ownership:** Derived classes (e.g., `HalideOperationWorker`) are responsible for
 *   storing and managing their specific data (e.g., operation vectors, AI parameters).
 *
 * @author CaptureMoment Team
 * @date 2026
 */

#pragma once

#include <future>
#include <memory>


namespace CaptureMoment::Core {


namespace Pipeline { class PipelineContext; } 
namespace ImageProcessing { class IWorkingImageHardware; } 

namespace Workers {

/**
 * @interface IWorkerRequest
 * @brief Base interface for all asynchronous processing workers.
 *
 * @details
 * This interface enforces a standard execution pattern where the worker
 * receives the infrastructure (Context) and the target (Image), and returns
 * a future representing the asynchronous result.
 */
class IWorkerRequest {
public:
    /**
     * @brief Virtual destructor.
     */
    virtual ~IWorkerRequest() = default;

    /**
     * @brief Executes the specific processing logic asynchronously.
     *
     * @details
     * This method is responsible for:
     * 1. Accessing the appropriate manager from the `PipelineContext` (e.g., `context.getHalideManager()`).
     * 2. Initializing that manager with the worker's internal data (e.g., operations).
     * 3. Running the pipeline on the provided image.
     * 4. Returning a `std::future` that resolves to `true` on success, or `false` on failure.
     *
     * @param context Reference to the global pipeline infrastructure.
     * @param working_image Reference to the target image buffer to process.
     * @return `std::future<bool>` representing the asynchronous result of the processing.
     */
    [[nodiscard]] virtual std::future<bool> execute(
        Pipeline::PipelineContext& context,
        ImageProcessing::IWorkingImageHardware& working_image
    ) = 0;

protected:
    /**
     * @brief Protected constructor to enforce abstract interface usage.
     */
    IWorkerRequest() = default;
};

} // namespace Workers

} // namespace CaptureMoment::Core
