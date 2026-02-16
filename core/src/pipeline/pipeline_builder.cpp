/**
 * @file pipeline_builder.cpp
 * @brief Implementation of PipelineBuilder.
 * @author CaptureMoment Team
 * @date 2026
 */

#include "pipeline/pipeline_builder.h"

#include <spdlog/spdlog.h>
#include <utility>

namespace CaptureMoment::Core::Pipeline {

std::unordered_map<PipelineType, PipelineBuilder::CreatorFunc>& PipelineBuilder::getRegistry()
{
    // Meyer's Singleton: Initialized on first use, destroyed on app exit.
    // Thread-safe in C++11 and later.
    static std::unordered_map<PipelineType, CreatorFunc> registry;
    return registry;
}

void PipelineBuilder::registerCreator(PipelineType type, CreatorFunc creator)
{
    // Register the creator function in the static map
    getRegistry()[type] = std::move(creator);
}

std::unique_ptr<IPipelineExecutor> PipelineBuilder::build(PipelineType type)
{
    auto& registry = getRegistry();
    auto it = registry.find(type);

    // Check if the requested type has been registered
    if (it == registry.end())
    {
        spdlog::error("[PipelineBuilder] No creator registered for pipeline type: {}", static_cast<int>(type));
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
        spdlog::critical("[PipelineBuilder] Exception during instantiation of pipeline type {}: {}", 
                        static_cast<int>(type), e.what());
        return nullptr;
    }
}

} // namespace CaptureMoment::Core::Pipeline
