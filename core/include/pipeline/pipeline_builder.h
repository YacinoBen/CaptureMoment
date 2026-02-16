/**
 * @file pipeline_builder.h
 * @brief Central Registry and Factory for `IPipelineExecutor` instances.
 *
 * @details
 * This class implements a Registry pattern. It maintains a static map of creator functions
 * indexed by `PipelineType`. This decouples the high-level managers from concrete
 * executor implementations, allowing for easy extension (e.g., adding OpenCV or AI managers)
 * without modifying existing manager code.
 *
 * **Usage:**
 * 1. Register a creator at startup: `PipelineBuilder::registerCreator(Type, []{ return std::make_unique<ConcreteExecutor>(); });`
 * 2. Build an instance: `auto executor = PipelineBuilder::build(Type);`
 * 3. Initialize the instance: `executor->initialize(ops, factory);`
 *
 * @author CaptureMoment Team
 * @date 2026
 */

#pragma once

#include "pipeline_type.h"
#include "pipeline/interfaces/i_pipeline_executor.h"

#include <memory>
#include <unordered_map>
#include <functional>

namespace CaptureMoment::Core {

namespace Pipeline {

/**
 * @class PipelineBuilder
 * @brief Static registry for creating `IPipelineExecutor` instances based on type.
 */
class PipelineBuilder {
public:
    /**
     * @brief Definition of the creator function signature.
     * @details
     * A creator function is a lambda that takes no arguments and returns
     * a unique pointer to the interface, calling the default constructor
     * of the concrete implementation.
     */
    using CreatorFunc = std::function<std::unique_ptr<IPipelineExecutor>()>;

    /**
     * @brief Retrieves an instance of the executor for the specified type.
     *
     * @details
     * This method looks up the `PipelineType` in the registry. If found,
     * it invokes the registered creator to construct a new instance using
     * the default constructor. The caller must then call `initialize` on the
     * returned object.
     *
     * @param type The enum identifier for the desired pipeline.
     * @return A unique pointer to the executor, or nullptr if type is not registered.
     */
    [[nodiscard]] static std::unique_ptr<IPipelineExecutor> build(PipelineType type);

    /**
     * @brief Registers a creator function for a specific pipeline type.
     *
     * @details
     * This should be called during system initialization (e.g., in a static
     * registration block or main function) to populate the registry.
     *
     * @param type The enum identifier.
     * @param creator The function to create the executor instance.
     */
    static void registerCreator(PipelineType type, CreatorFunc creator);

private:
    /**
     * @brief Accessor for the static registry map.
     * @return Reference to the unordered_map storing creators.
     */
    static std::unordered_map<PipelineType, CreatorFunc>& getRegistry();
};

} // namespace Pipeline

} // namespace CaptureMoment::Core
