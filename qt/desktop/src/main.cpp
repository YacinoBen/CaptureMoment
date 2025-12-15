#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include "utils/qml_context_setup.h"
#include "rendering/rhi_image_item.h"
#include <spdlog/spdlog.h>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QQmlApplicationEngine engine;

    CaptureMoment::UI::QmlContextSetup::setupContext(engine.rootContext());
    qmlRegisterType<CaptureMoment::UI::Rendering::RHIImageItem>("CaptureMoment.Qt.Rendering", 1, 0, "RHIImageItem");

    auto context = engine.rootContext();
    if (!CaptureMoment::UI::QmlContextSetup::setupContext(context)) {
        spdlog::error("Failed to setup QML context");
        return -1;
    }

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    engine.loadFromModule("CaptureMoment.desktop", "DesktopMain");

    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    return app.exec();
}
