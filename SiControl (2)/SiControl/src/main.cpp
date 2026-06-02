#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QtQml/qqml.h>
#include "model/MixerModel.h"
#include "model/Channel.h"

// Entry point. Registers the C++ model with QML and loads the UI.
int main(int argc, char* argv[])
{
    QGuiApplication app(argc, argv);
    app.setOrganizationName("SiControl");
    app.setApplicationName("SiControl");

    // Channel is referenced by QML but created in C++.
    qmlRegisterUncreatableType<model::Channel>(
        "SiControl", 1, 0, "Channel", "Channel objects are owned by Mixer");

    QQmlApplicationEngine engine;

    // Expose a single MixerModel instance as the context property "Mixer".
    auto* mixer = new model::MixerModel(&app);
    engine.rootContext()->setContextProperty("Mixer", mixer);

    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app,
        []() { QCoreApplication::exit(-1); }, Qt::QueuedConnection);

    // Load the root component by module URI (matches qt_add_qml_module below).
    engine.loadFromModule("SiControl", "Main");

    return app.exec();
}
