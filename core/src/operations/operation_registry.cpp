/**
 * @file operation_registry.cpp
 * @brief Implementation of OperationRegistry
 * @author CaptureMoment Team
 * @date 2025
 */

#include "operations/operation_registry.h"
#include "operations/operation_brightness.h"
#include "operations/operation_contrast.h"
#include "operations/operation_highlights.h"
#include "operations/operation_shadows.h"
#include "operations/operation_whites.h"
#include "operations/operation_blacks.h"

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

namespace CaptureMoment::Core::Operations {

void OperationRegistry::registerAll(OperationFactory& factory) {
    spdlog::info("OperationRegistry: Registering all operations");

    registerToneAdjustments(factory);
    // registerColorOperations(factory);
    // registerColorProfiles(factory);
    // registerDetailOperations(factory);
    // registerEffects(factory);

    spdlog::info("OperationRegistry: All operations registered");
}

void OperationRegistry::registerToneAdjustments(OperationFactory& factory) {
    spdlog::debug("OperationRegistry: Registering tone adjustment operations");

    // Brightness
    factory.registerCreator(OperationType::Brightness, []() { return std::make_unique<OperationBrightness>(); });
    spdlog::trace("Factory register Brightness");

    // Contrast
    factory.registerCreator(OperationType::Contrast, []() { return std::make_unique<OperationContrast>(); });
    spdlog::trace("Factory register Contrast");

    // Highlights
    factory.registerCreator(OperationType::Highlights, []() { return std::make_unique<OperationHighlights>(); });
    spdlog::trace("Factory register Highlights");

    // Shadows
    factory.registerCreator(OperationType::Shadows, []() { return std::make_unique<OperationShadows>(); });
    spdlog::trace("Factory register Shadows");

    // Whites
    factory.registerCreator(OperationType::Whites, []() { return std::make_unique<OperationWhites>(); });
    spdlog::trace("Factory register Whites");

    // Blacks
    factory.registerCreator(OperationType::Blacks, []() { return std::make_unique<OperationBlacks>(); });
    spdlog::trace("Factory register Blacks");

    // factory.registerCreator(OperationType::Exposure, []() { return std::make_unique<OperationExposure>(); });
    // spdlog::trace("ok: Exposure");

    // REMOVED DUPLICATE: factory.registerCreator(OperationType::Shadows, []() { return std::make_unique<OperationShadows>(); });
}

void OperationRegistry::registerColorOperations(OperationFactory& factory) {
    spdlog::debug("OperationRegistry: Registering color operations");

    // TODO: Add color operations as they are implemented
    // factory.registerCreator(OperationType::Saturation, []() { return std::make_unique<OperationSaturation>(); });
    // spdlog::trace("ok: Saturation");

    // factory.registerCreator(OperationType::Hue, []() { return std::make_unique<OperationHue>(); });
    // spdlog::trace("ok: Hue");

    // factory.registerCreator(OperationType::Vibrance, []() { return std::make_unique<OperationVibrance>(); });
    // spdlog::trace("ok: Vibrance");

    // factory.registerCreator(OperationType::ColorGrade, []() { return std::make_unique<OperationColorGrade>(); });
    // spdlog::trace("ok: Color Grade");
}

void OperationRegistry::registerColorProfiles(OperationFactory& factory) {
    spdlog::debug("OperationRegistry: Registering color profile operations");

    // TODO: Add color profile operations as they are implemented
    // factory.registerCreator(OperationType::ColorProfile, []() { return std::make_unique<OperationColorProfile>(); });
    // spdlog::trace("ok: Color Profile");

    // factory.registerCreator(OperationType::WhiteBalance, []() { return std::make_unique<OperationWhiteBalance>(); });
    // spdlog::trace("ok: White Balance");

    // factory.registerCreator(OperationType::ColorSpaceConversion, []() { return std::make_unique<OperationColorSpaceConversion>(); });
    // spdlog::trace("ok: Color Space Conversion");
}

void OperationRegistry::registerDetailOperations(OperationFactory& factory) {
    spdlog::debug("OperationRegistry: Registering detail operations");

    // TODO: Add detail operations as they are implemented
    // factory.registerCreator(OperationType::Sharpen, []() { return std::make_unique<OperationSharpen>(); });
    // spdlog::trace("ok: Sharpen");

    // factory.registerCreator(OperationType::Clarity, []() { return std::make_unique<OperationClarity>(); });
    // spdlog::trace("ok: Clarity");

    // factory.registerCreator(OperationType::Texture, []() { return std::make_unique<OperationTexture>(); });
    // spdlog::trace("ok: Texture");

    // factory.registerCreator(OperationType::Denoise, []() { return std::make_unique<OperationDenoise>(); });
    // spdlog::trace("ok: Denoise");
}

void OperationRegistry::registerEffects(OperationFactory& factory) {
    spdlog::debug("OperationRegistry: Registering effect operations");

    // TODO: Add effect operations as they are implemented
    // factory.registerCreator(OperationType::Blur, []() { return std::make_unique<OperationBlur>(); });
    // spdlog::trace("ok: Blur");

    // factory.registerCreator(OperationType::Vignette, []() { return std::make_unique<OperationVignette>(); });
    // spdlog::trace("ok: Vignette");

    // factory.registerCreator(OperationType::Grain, []() { return std::make_unique<OperationGrain>(); });
    // spdlog::trace("ok: Grain");

    // factory.registerCreator(OperationType::Perspective, []() { return std::make_unique<OperationPerspective>(); });
    // spdlog::trace("ok: Perspective");
}

} // namespace CaptureMoment::Core::Operations
