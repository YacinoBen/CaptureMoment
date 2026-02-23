/**
 * @file worker_builder.h
 * @brief Central Registry and Factory for `IWorkerRequest` instances.
 *
 * @details
 * This class implements a Registry pattern. It maintains a static map of creator functions
 * indexed by `WorkerType`. This decouples the high-level managers from concrete
 * worker implementations, allowing for easy extension (e.g., adding OpenCV or AI workers)
 * without modifying existing manager code.
 *
 * **Usage:**
 * 1. Register a creator at startup: `WorkerBuilder::registerCreator(Type, []{ return std::make_unique<ConcreteWorker>(); });`
 * 2. Build an instance: `auto worker = WorkerBuilder::build(Type);`
 * 3. Execute the worker: `worker->execute(context, image);`
 *
 * @author CaptureMoment Team
 * @date 2026
 */

#pragma once

#include "workers/worker_type.h"
#include "workers/interfaces/i_worker_request.h"

#include <memory>
#include <unordered_map>
#include <functional>

namespace CaptureMoment::Core {

namespace Workers {

/**
 * @class WorkerBuilder
 * @brief Static registry for creating `IWorkerRequest` instances based on type.
 */
class WorkerBuilder {
public:
    /**
     * @brief Definition of the creator function signature.
     * @details
     * A creator function is a lambda that takes no arguments and returns
     * a unique pointer to the interface, calling the default constructor
     * of the concrete implementation.
     */
    using CreatorFunc = std::function<std::unique_ptr<IWorkerRequest>()>;

    /**
     * @brief Retrieves an instance of the worker for the specified type.
     *
     * @details
     * This method looks up the `WorkerType` in the registry. If found,
     * it invokes the registered creator to construct a new instance using
     * the default constructor.
     *
     * @param type The enum identifier for the desired worker.
     * @return A unique pointer to the worker, or nullptr if type is not registered.
     */
    [[nodiscard]] static std::unique_ptr<IWorkerRequest> build(WorkerType type);

    /**
     * @brief Registers a creator function for a specific worker type.
     *
     * @details
     * This should be called during system initialization (e.g., in a static
     * registration block or main function) to populate the registry.
     *
     * @param type The enum identifier.
     * @param creator The function to create the worker instance.
     */
    static void registerCreator(WorkerType type, CreatorFunc creator);

private:
    /**
     * @brief Accessor for the static registry map.
     * @return Reference to the unordered_map storing creators.
     */
    static std::unordered_map<WorkerType, CreatorFunc>& getRegistry();
};

} // namespace Workers

} // namespace CaptureMoment::Core
