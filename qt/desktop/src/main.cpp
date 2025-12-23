#include <QGuiApplication>
#include <QQmlApplicationEngine>

#include "utils/qml_context_setup.h"
//#include "rendering/qml_rhi_image_item.h"
//#include "rendering/qml_sgs_image_item.h"
#include "rendering/qml_painted_image_item.h"

#include <spdlog/spdlog.h>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QQmlApplicationEngine engine;

    // Register QML types
    qmlRegisterType<CaptureMoment::UI::QMLPaintedImageItem>(
        "CaptureMoment.UI.Painted.Rendering", 1, 0, "QMLPaintedImageItem"
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
