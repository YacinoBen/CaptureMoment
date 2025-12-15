/**
 * @file qml_context_setup.cpp
 * @brief Implementation of QmlContextSetup
 * @author CaptureMoment Team
 * @date 2025
 */

#include "utils/qml_context_setup.h"
#include "image_controller.h"
#include "models/operations/brightness_model.h"
#include "rendering/rhi_image_item.h"

#include <spdlog/spdlog.h>

namespace CaptureMoment::UI {

    // Static member initialization (pointers are initialized to nullptr by default)
    std::shared_ptr<ImageController> QmlContextSetup::m_controller = nullptr;
    std::shared_ptr<BrightnessModel> QmlContextSetup::m_brightness_model = nullptr;
    std::shared_ptr<Rendering::RHIImageItem> QmlContextSetup::m_rhi_image_item = nullptr;

    bool QmlContextSetup::setupContext(QQmlContext* context) {
        if (!context) {
            spdlog::error("QmlContextSetup::setupContext: QML Context is null!");
            return false; // Échec
        }

        spdlog::info("QmlContextSetup: Starting QML context setup...");

        // Step 1: Create all objects (separated into logical groups)
        if (!createController()) {
            spdlog::error("QmlContextSetup::setupContext: Failed to create ImageController.");
            return false; // Échec
        }

        if (!createRenderingComponents()) {
            spdlog::error("QmlContextSetup::setupContext: Failed to create rendering components.");
            return false; // Échec
        }

        if (!createOperationModels()) {
            spdlog::error("QmlContextSetup::setupContext: Failed to create operation models.");
            return false; // Échec
        }

        // Step 2: Connect objects together
        if (!connectObjects()) {
            spdlog::error("QmlContextSetup::setupContext: Failed to connect objects.");
            return false; // Échec
        }

        // Step 3: Register necessary objects to QML context
        if (!registerToQml(context)) {
            spdlog::error("QmlContextSetup::setupContext: Failed to register objects to QML.");
            return false; // Échec
        }
        // Register Models
        if (!registerModelsToQml(context)) {
            spdlog::error("QmlContextSetup::setupContext: Failed to register objects to QML.");
            return false; // Échec
        }
        spdlog::info("QmlContextSetup: QML context setup completed successfully.");
        return true; // Succès
    }

    // Creates the central ImageController orchestrator.
    [[nodiscard]] bool QmlContextSetup::createController() {
        spdlog::debug("QmlContextSetup::createController: Creating ImageController...");

        m_controller = std::make_shared<ImageController>();
        if (!m_controller) {
            spdlog::error("QmlContextSetup::createController: Failed to create ImageController (out of memory or constructor threw).");
            return false; // Échec
        }
        spdlog::debug("  ✓ ImageController created.");

        return true; // Succès
    }

    // Creates the RHI-based image display item (RhiImageItem).
    [[nodiscard]] bool QmlContextSetup::createRenderingComponents() {
        spdlog::debug("QmlContextSetup::createRenderingComponents: Creating rendering components...");

        m_rhi_image_item = std::make_shared<Rendering::RHIImageItem>();
        if (!m_rhi_image_item) {
            spdlog::error("QmlContextSetup::createRenderingComponents: Failed to create RhiImageItem (out of memory or constructor threw).");
            return false; // Échec
        }
        spdlog::debug("  ✓ RhiImageItem created.");

        return true; // Succès
    }

    // Creates the operation model objects (e.g., BrightnessModel).
    [[nodiscard]] bool QmlContextSetup::createOperationModels() {
        spdlog::debug("QmlContextSetup::createOperationModels: Creating operation models...");

        m_brightness_model = std::make_shared<BrightnessModel>();
        if (!m_brightness_model) {
            spdlog::error("QmlContextSetup::createOperationModels: Failed to create BrightnessModel (out of memory or constructor threw).");
            return false; // Échec
        }
        spdlog::debug("  ✓ BrightnessModel created.");

        // TODO: Create more models as needed (e.g., ContrastModel, SaturationModel)
        // m_contrast_model = std::make_shared<ContrastModel>();
        // if (!m_contrast_model) { ... return false; }
        // spdlog::debug("  ✓ ContrastModel created.");

        return true; // Succès
    }

    // Connects models and display item to controller
    [[nodiscard]] bool QmlContextSetup::connectObjects() {
        spdlog::debug("QmlContextSetup::connectObjects: Connecting objects...");

        // Vérifier que tous les objets nécessaires existent avant de tenter de les connecter
        if (!m_controller) {
            spdlog::error("QmlContextSetup::connectObjects: ImageController is null, cannot connect.");
            return false; // Échec
        }

        if (!m_rhi_image_item) {
            spdlog::error("QmlContextSetup::connectObjects: RhiImageItem is null, cannot connect.");
            return false; // Échec
        }

        if (!m_brightness_model) {
            spdlog::error("QmlContextSetup::connectObjects: BrightnessModel is null, cannot connect.");
            return false; // Échec
        }

        // --- Connect Controller and RHI Image Item ---
        m_controller->setRHIImageItem(m_rhi_image_item.get());
        spdlog::debug("  ✓ ImageController connected to RhiImageItem.");

        // --- Connect Operation Models to Controller ---
        // BrightnessModel needs to know the controller to communicate changes or register itself
        m_brightness_model->setImageController(m_controller.get());
        spdlog::debug("  ✓ BrightnessModel connected to ImageController.");

        // TODO: Connect more models as they are created
        // m_contrast_model->setImageController(m_controller.get());
        // spdlog::debug("  ✓ ContrastModel connected to ImageController.");

        spdlog::info("QmlContextSetup::connectObjects: All objects connected successfully.");
        return true; // Succès
    }

    // Registers necessary objects to QML context
    [[nodiscard]] bool QmlContextSetup::registerToQml(QQmlContext* context) {
        spdlog::debug("QmlContextSetup::registerToQml: Registering objects to QML...");

        if (!context) {
            spdlog::error("QmlContextSetup::registerToQml: QML Context is null!");
            return false; // Échec
        }

        if (!m_controller) {
            spdlog::error("QmlContextSetup::registerToQml: ImageController is null, cannot register.");
            return false; // Échec
        }

        // --- Register Controller ---
        context->setContextProperty("controller", m_controller.get());
        spdlog::debug("  ✓ 'controller' registered to QML context.");

        // --- Register RHI Image Item (if available) ---
        if (m_rhi_image_item) {
            context->setContextProperty("rhiImageItem", m_rhi_image_item.get());
            spdlog::debug("  ✓ 'rhiImageItem' registered to QML context.");
        } else {
            spdlog::warn("QmlContextSetup::registerToQml: RhiImageItem is null, skipping registration.");
        }

        // Note: The internal models (like m_brightness_model) are NOT registered here.
        // They are managed by ImageController.

        spdlog::info("QmlContextSetup::registerToQml: Objects registered to QML successfully.");
        return true; // Succès
    }

    [[nodiscard]] bool QmlContextSetup::registerModelsToQml(QQmlContext* context) {
        spdlog::debug("QmlContextSetup::registerModelsToQml: Registering objects to QML...");

        if (m_brightness_model) {
            context->setContextProperty("brightnessModel", m_brightness_model.get());
            spdlog::debug("  ✓ 'brightnessModel' registered to QML context.");
        } else {
            spdlog::warn("QmlContextSetup::registerToQml: BrightnessModel is null, skipping registration.");
        }

        // Note: The internal models (like m_brightness_model) are NOT registered here.
        // They are managed by ImageController.

        spdlog::info("QmlContextSetup::registerModelsToQml: Objects registered to QML successfully.");
        return true; // Succès
    }

} // namespace CaptureMoment::UI
