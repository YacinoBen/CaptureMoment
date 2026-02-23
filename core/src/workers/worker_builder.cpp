/**
 * @file worker_builder.cpp
 * @brief Implementation of WorkerBuilder.
 * @author CaptureMoment Team
 * @date 2026
 */

#include "workers/worker_builder.h"

#include <spdlog/spdlog.h>
#include <utility>

namespace CaptureMoment::Core::Workers {

std::unordered_map<WorkerType, WorkerBuilder::CreatorFunc>& WorkerBuilder::getRegistry()
{
    // Meyer's Singleton: Initialized on first use, destroyed on app exit.
    static std::unordered_map<WorkerType, CreatorFunc> registry;
    return registry;
}

void WorkerBuilder::registerCreator(WorkerType type, CreatorFunc creator)
{
    // Register the creator function in the static map
    getRegistry()[type] = std::move(creator);
}

std::unique_ptr<IWorkerRequest> WorkerBuilder::build(WorkerType type)
{
    auto& registry = getRegistry();
    auto it = registry.find(type);

    // Check if the requested type has been registered
    if (it == registry.end())
    {
        spdlog::error("[WorkerBuilder] No creator registered for worker type: {}", static_cast<int>(type));
        return nullptr;
    }

    try
    {
        // Invoke the stored lambda/function to create the instance
        // The creator is expected to call the default constructor of the concrete implementation.
        return it->second();
    }
    catch (const std::exception& e)
    {
        spdlog::critical("[WorkerBuilder] Exception during instantiation of worker type {}: {}", 
                        static_cast<int>(type), e.what());
        return nullptr;
    }
}

} // namespace CaptureMoment::Core::Workers
