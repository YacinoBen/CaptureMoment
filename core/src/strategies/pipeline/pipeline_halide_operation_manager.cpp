/**
 * @file pipeline_halide_operation_manager.cpp
 * @brief Implementation of PipelineHalideOperationManager.
 * @author CaptureMoment Team
 * @date 2026
 */

#include "strategies/pipeline/pipeline_halide_operation_manager.h"

#include <spdlog/spdlog.h>

namespace CaptureMoment::Core::Strategies {

PipelineHalideOperationManager::PipelineHalideOperationManager(const Pipeline::PipelineBuilder& builder)
{

    // builder.build return unique_ptr<IPipelineExecutor>
    // m_executor take unique_ptr<OperationPipelineExecutor>
    // We need to downcast the pointer from IPipelineExecutor to OperationPipelineExecutor
    
    auto base_executor = builder.build(Pipeline::PipelineType::HalideOperation);

    if (!base_executor) {
        spdlog::error("PipelineHalideOperationManager::PipelineHalideOperationManager: Builder returned null.");
        return;
    }

    // Downcasting to the concrete type. This is safe because we know the builder returns the correct type for this pipeline.
    auto* concrete_ptr = dynamic_cast<Pipeline::OperationPipelineExecutor*>(base_executor.get());
    
    if (!concrete_ptr) {
        spdlog::error("PipelineHalideOperationManager::PipelineHalideOperationManager: Critical type mismatch.");
        return;
    }

    // Release ownership from base_executor and transfer to m_executor
    base_executor.release();
    m_executor.reset(concrete_ptr);
}

void PipelineHalideOperationManager::init(
    const std::vector<Operations::OperationDescriptor>& operations,
    const Operations::OperationFactory& factory)
{
    spdlog::debug("PipelineHalideOperationManager::init: HalideManager initializing (Copy) with {} ops.", operations.size());
    std::lock_guard lock(m_mutex);
    m_executor->init(operations, factory);
}

void PipelineHalideOperationManager::init(
    std::vector<Operations::OperationDescriptor>&& operations,
    const Operations::OperationFactory& factory)
{
    spdlog::debug("PipelineHalideOperationManager::init: HalideManager initializing (Move) with {} ops.", operations.size());
    std::lock_guard lock(m_mutex);
    m_executor->init(std::move(operations), factory);
}

bool PipelineHalideOperationManager::execute(ImageProcessing::IWorkingImageHardware& working_image)
{
    std::lock_guard lock(m_mutex);

    if (!m_executor) {
        return false;
    }

    return m_executor->execute(working_image);
}

} // namespace CaptureMoment::Core::Strategies
