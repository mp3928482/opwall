// opwall — a movie-style operations wall of panels, all made-up data.

#include <QCommandLineParser>
#include <QGuiApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QUrl>

#include "backend.h"
#include "graphitem.h"
#include "theme.h"
#include "translator.h"

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);
    app.setApplicationName("opwall");
    app.setApplicationVersion("0.1.0");

    // Matches the window to opwall.desktop, so the compositor (Wayland app_id)
    // and the launcher pick up the installed icon.
    app.setDesktopFileName("opwall");
    app.setWindowIcon(QIcon::fromTheme("opwall"));

    QCommandLineParser parser;
    parser.setApplicationDescription("A movie-style operations wall filled with randomized data.");
    parser.addHelpOption();
    parser.addVersionOption();
    QCommandLineOption seedOption("seed", "Generate the wall from <number>, to get the same one again.", "number");
    QCommandLineOption fullscreenOption({"f", "fullscreen"}, "Start fullscreen.");
    parser.addOptions({seedOption, fullscreenOption});
    parser.process(app);

    bool seedOk = true;
    const quint32 seed = parser.isSet(seedOption) ? parser.value(seedOption).toUInt(&seedOk) : 0;
    if (!seedOk || (parser.isSet(seedOption) && seed == 0)) {
        fprintf(stderr, "opwall: --seed needs a positive whole number\n");
        return 1;
    }

    QQuickStyle::setStyle("Material");

    // Installed before anything is translated, and every qsTr() binding is
    // re-evaluated when the Omarchy language changes.
    Translator translator;
    app.installTranslator(&translator);

    Theme theme;
    Backend backend(seed);

    qmlRegisterType<GraphItem>("Opwall", 1, 0, "NodeGraph");

    QQmlApplicationEngine engine;
    QObject::connect(&translator, &Translator::changed, &engine, &QQmlEngine::retranslate);
    engine.rootContext()->setContextProperty("translator", &translator);
    engine.rootContext()->setContextProperty("theme", &theme);
    engine.rootContext()->setContextProperty("backend", &backend);
    engine.rootContext()->setContextProperty("startFullscreen", parser.isSet(fullscreenOption));
    engine.load(QUrl("qrc:/Main.qml"));
    if (engine.rootObjects().isEmpty())
        return 1;

    return app.exec();
}
