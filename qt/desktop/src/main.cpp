#include <QGuiApplication>
#include <QQmlApplicationEngine>

#include "utils/qml_context_setup.h"

#include "rendering/qml_painted_image_item.h"
#include "rendering/qml_sgs_image_item.h"
#include "rendering/qml_rhi_image_item.h"

#include "image_processing/deciders/benchmarking_backend_decider.h"
#include "config/app_config.h"
#include "common/types/memory_type.h"; // to remove after  when we have a stable version we can call decide()

#include <spdlog/spdlog.h>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QQmlApplicationEngine engine;

    spdlog::info("Starting backend benchmark...");
    CaptureMoment::Core::ImageProcessing::BenchmarkingBackendDecider benchmark_decider;
   // auto backend = benchmark_decider.decide(); when we have a stable version we can call decide()
    auto backend =  CaptureMoment::Core::Common::MemoryType::GPU_MEMORY;

    CaptureMoment::Core::Config::AppConfig::instance().setProcessingBackend(backend);


    // Register QML types Rendering
    qmlRegisterType<CaptureMoment::UI::QMLPaintedImageItem>(
        "CaptureMoment.UI.Rendering.Painted", 1, 0, "QMLPaintedImageItem"
    );
    qmlRegisterType<CaptureMoment::UI::QMLSGSImageItem>(
        "CaptureMoment.UI.Rendering.SGS", 1, 0, "QMLSGSImageItem"
        );
    qmlRegisterType<CaptureMoment::UI::QMLRHIImageItem>(
        "CaptureMoment.UI.Rendering.RHI", 1, 0, "QMLRHIImageItem"
        );

    // Setup QML context once
    auto context = engine.rootContext();
    if (!CaptureMoment::UI::QmlContextSetup::setupContext(context)) {
        spdlog::error("Failed to setup QML context");
        return -1;
    }

    // Handle QML engine errors
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection
    );

    // Load the main QML module
    engine.loadFromModule("CaptureMoment.desktop", "DesktopMain");

    if (engine.rootObjects().isEmpty()) {
        spdlog::error("Failed to load QML module");
        return -1;
    }

    return app.exec();
}
