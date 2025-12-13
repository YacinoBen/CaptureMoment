/**
 * @file operation_registry.cpp
 * @brief Implementation of OperationRegistry
 * @author CaptureMoment Team
 * @date 2025
 */

#include "operations/operation_registry.h"
#include "operations/operation_brightness.h"
// #include "operations/operation_contrast.h"          // TODO: Implement
// #include "operations/operation_saturation.h"       // TODO: Implement
// #include "operations/operation_hue.h"              // TODO: Implement
// #include "operations/operation_exposure.h"         // TODO: Implement
// #include "operations/operation_vibrance.h"         // TODO: Implement
// #include "operations/operation_clarity.h"          // TODO: Implement
// #include "operations/operation_sharpen.h"          // TODO: Implement
// #include "operations/operation_color_profile.h"    // TODO: Implement
// #include "operations/operation_blur.h"             // TODO: Implement
// #include "operations/operation_vignette.h"         // TODO: Implement
// #include "operations/operation_grain.h"            // TODO: Implement
#include <spdlog/spdlog.h>

namespace CaptureMoment {

void OperationRegistry::registerAll(OperationFactory& factory) {
    spdlog::info("OperationRegistry: Registering all operations");
    
    registerToneAdjustments(factory);
    registerColorOperations(factory);
    registerColorProfiles(factory);
    registerDetailOperations(factory);
    registerEffects(factory);
    
    spdlog::info("OperationRegistry: All operations registered");
}

void OperationRegistry::registerToneAdjustments(OperationFactory& factory) {
    spdlog::debug("OperationRegistry: Registering tone adjustment operations");
    
    // Brightness
    factory.registerOperation<OperationBrightness>(OperationType::Brightness);
    spdlog::trace("ok: Brightness");
    
    // TODO: Add more tone adjustments as they are implemented
    // factory.registerOperation<OperationContrast>(OperationType::Contrast);
    // spdlog::trace("ok: Contrast");
    
    // factory.registerOperation<OperationExposure>(OperationType::Exposure);
    // spdlog::trace("ok: Exposure");
    
    // factory.registerOperation<OperationShadows>(OperationType::Shadows);
    // spdlog::trace("ok: Shadows");
    
    // factory.registerOperation<OperationHighlights>(OperationType::Highlights);
    // spdlog::trace("ok: Highlights");
}

void OperationRegistry::registerColorOperations(OperationFactory& factory) {
    spdlog::debug("OperationRegistry: Registering color operations");
    
    // TODO: Add color operations as they are implemented
    // factory.registerOperation<OperationSaturation>(OperationType::Saturation);
    // spdlog::trace("ok: Saturation");
    
    // factory.registerOperation<OperationHue>(OperationType::Hue);
    // spdlog::trace("ok: Hue");
    
    // factory.registerOperation<OperationVibrance>(OperationType::Vibrance);
    // spdlog::trace("ok: Vibrance");
    
    // factory.registerOperation<OperationColorGrade>(OperationType::ColorGrade);
    // spdlog::trace("ok: Color Grade");
}

void OperationRegistry::registerColorProfiles(OperationFactory& factory) {
    spdlog::debug("OperationRegistry: Registering color profile operations");
    
    // TODO: Add color profile operations as they are implemented
    // factory.registerOperation<OperationColorProfile>(OperationType::ColorProfile);
    // spdlog::trace("ok: Color Profile");
    
    // factory.registerOperation<OperationWhiteBalance>(OperationType::WhiteBalance);
    // spdlog::trace("ok: White Balance");
    
    // factory.registerOperation<OperationColorSpaceConversion>(OperationType::ColorSpaceConversion);
    // spdlog::trace("ok: Color Space Conversion");
}

void OperationRegistry::registerDetailOperations(OperationFactory& factory) {
    spdlog::debug("OperationRegistry: Registering detail operations");
    
    // TODO: Add detail operations as they are implemented
    // factory.registerOperation<OperationSharpen>(OperationType::Sharpen);
    // spdlog::trace("ok: Sharpen");
    
    // factory.registerOperation<OperationClarity>(OperationType::Clarity);
    // spdlog::trace("ok: Clarity");
    
    // factory.registerOperation<OperationTexture>(OperationType::Texture);
    // spdlog::trace("ok: Texture");
    
    // factory.registerOperation<OperationDenoise>(OperationType::Denoise);
    // spdlog::trace("ok: Denoise");
}

void OperationRegistry::registerEffects(OperationFactory& factory) {
    spdlog::debug("OperationRegistry: Registering effect operations");
    
    // TODO: Add effect operations as they are implemented
    // factory.registerOperation<OperationBlur>(OperationType::Blur);
    // spdlog::trace("ok: Blur");
    
    // factory.registerOperation<OperationVignette>(OperationType::Vignette);
    // spdlog::trace("ok: Vignette");
    
    // factory.registerOperation<OperationGrain>(OperationType::Grain);
    // spdlog::trace("ok: Grain");
    
    // factory.registerOperation<OperationPerspective>(OperationType::Perspective);
    // spdlog::trace("ok: Perspective");
}

} // namespace CaptureMoment