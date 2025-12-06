/**
 * @file operation_factory.cpp
 * @brief Implementation of OperationFactory
 * @author CaptureMoment Team
 * @date 2025
 */

#include "operation_factory.h"
#include "i_operation.h"
#include "operation_descriptor.h"
#include <spdlog/spdlog.h>

namespace CaptureMoment {

std::unique_ptr<IOperation> OperationFactory::create(
    const OperationDescriptor& descriptor
) const {
    auto it = m_creators.find(descriptor.type);
    
    if (it == m_creators.end()) {
        spdlog::error("OperationFactory::create: No creator registered for operation type");
        return nullptr;
    }

    spdlog::trace("OperationFactory::create: Creating operation '{}'", descriptor.name);
    return it->second();
}

} // namespace CaptureMoment