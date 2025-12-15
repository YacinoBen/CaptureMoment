#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include "utils/qml_context_setup.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QQmlApplicationEngine engine;

    CaptureMoment::UI::QmlContextSetup::setupContext(engine.rootContext());

    engine.loadFromModule("CaptureMoment.desktop", "DesktopMain");

    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    return app.exec();
}
