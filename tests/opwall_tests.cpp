#include <QColor>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QtTest>
#include <QSet>
#include <QSignalSpy>
#include <cmath>

#include "backend.h"
#include "generator.h"
#include "graphitem.h"
#include "theme.h"
#include "translator.h"

class OpwallTests : public QObject {
    Q_OBJECT

private slots:
    void generatorIsDeterministic()
    {
        Generator a(1234), b(1234), c(4321);
        QCOMPARE(a.header(), b.header());
        QCOMPARE(a.graph(), b.graph());
        QVERIFY(Generator(1234).header() != c.header());
    }

    void graphIsWellFormed()
    {
        const auto graph = Generator(7).graph();
        const auto nodes = graph.value("nodes").toList(), edges = graph.value("edges").toList();
        QCOMPARE(graph.value("clusters").toList().size(), 7);
        QCOMPARE(nodes.first().toInt(), -1);
        QVERIFY(nodes.size() > 300);
        QCOMPARE(edges.size() % 2, 0);
        for (const auto &e : edges)
            QVERIFY(e.toInt() >= 0 && e.toInt() < nodes.size());
    }

    void panelsHaveUniqueTitles()
    {
        const auto panels = Generator(99).panels();
        QCOMPARE(panels.size(), 7);
        QSet<QString> titles;
        for (const auto &p : panels)
            titles.insert(p.toMap().value("title").toString());
        QCOMPARE(titles.size(), 7);
    }

    void tokenizerColorsCode()
    {
        const auto tokens = Generator::tokenize("const prism = await parseLedger('x', 42);");
        QString joined;
        QStringList roles;
        for (const auto &t : tokens) {
            joined += t.toMap().value("text").toString();
            roles << t.toMap().value("role").toString();
        }
        QCOMPARE(joined, QString("const prism = await parseLedger('x', 42);"));
        QVERIFY(roles.contains("keyword"));
        QVERIFY(roles.contains("function"));
        QVERIFY(roles.contains("string"));
        QVERIFY(roles.contains("number"));
    }

    void backendRerollsAndPauses()
    {
        Backend backend(555);
        QCOMPARE(backend.seed(), 555u);
        const auto header = backend.header();
        QSignalSpy spy(&backend, &Backend::rerolled);
        backend.reroll();
        QCOMPARE(spy.count(), 1);
        QVERIFY(backend.seed() != 555u);
        QVERIFY(backend.header() != header);
        backend.togglePaused();
        QVERIFY(backend.paused());
        backend.togglePaused();
        QVERIFY(!backend.paused());
    }

    void backendNextSnippetsAreReproducible()
    {
        Backend a(77), b(77);
        a.nextCode();
        b.nextCode();
        QCOMPARE(a.code(), b.code());
        QVERIFY(a.code() != Backend(77).code());
    }

    void graphSettlesAndStaysFinite()
    {
        GraphItem item;
        item.setRunning(false);
        item.setSize(QSizeF(800, 450));
        item.setModel(Generator(3).graph());
        for (int i = 0; i < 400; ++i)
            item.step();
        QVERIFY(item.nodeCount() > 300);
        double spread = 0;
        for (int i = 0; i < item.nodeCount(); ++i) {
            const QPointF p = item.position(i);
            QVERIFY(std::isfinite(p.x()) && std::isfinite(p.y()));
            QVERIFY(std::abs(p.x()) < 6 && std::abs(p.y()) < 6);
            spread = std::max(spread, std::hypot(p.x(), p.y()));
        }
        // It must actually unfold from the knot it starts in.
        QVERIFY(spread > 0.5);
        QCOMPARE(item.position(0), QPointF(0, 0));
    }

    void themeFallsBack()
    {
        QTemporaryDir dir;
        Theme theme(dir.path());
        QCOMPARE(theme.accent(), QString("#FFD60A"));
        QCOMPARE(theme.accentForeground(), QString("black"));
    }

    void themeReadsAccent()
    {
        QTemporaryDir dir;
        QDir(dir.path()).mkpath("theme");
        QFile colors(dir.path() + "/theme/colors.toml");
        QVERIFY(colors.open(QIODevice::WriteOnly));
        colors.write("accent = \"#112233\"\nred = \"#aa0000\"\nbogus = \"nope\"\n");
        colors.close();

        Theme theme(dir.path());
        QCOMPARE(theme.accent(), QString("#112233"));
        QCOMPARE(theme.accentForeground(), QString("white"));
        QCOMPARE(theme.colors().value("red").toString(), QString("#aa0000"));
        QVERIFY(!theme.colors().contains("bogus"));
        QCOMPARE(theme.palette().size(), 10);
    }

    void lightThemeKeepsTheWallDark()
    {
        QTemporaryDir dir;
        QDir(dir.path()).mkpath("theme");
        QFile colors(dir.path() + "/theme/colors.toml");
        QVERIFY(colors.open(QIODevice::WriteOnly));
        colors.write("mode = \"light\"\nbackground = \"#fafafa\"\ndarker_background = \"#eeeeee\"\n");
        colors.close();

        Theme theme(dir.path());
        QVERIFY(QColor(theme.background()).lightnessF() < 0.2);
    }

    void themeFollowsChanges()
    {
        QTemporaryDir dir;
        QDir(dir.path()).mkpath("theme");
        write(dir.path() + "/theme/colors.toml", "accent = \"#112233\"\n");
        Theme theme(dir.path());
        QCOMPARE(theme.accent(), QString("#112233"));
        write(dir.path() + "/theme/colors.toml", "accent = \"#445566\"\n");
        QTRY_COMPARE(theme.accent(), QString("#445566"));
    }

    void themeFollowsHyprlandRounding()
    {
        QTemporaryDir dir;
        const QString response = dir.filePath("response.json"), command = dir.filePath("hyprctl");
        auto write = [](const QString &path, const QByteArray &data) {
            QFile file(path);
            return file.open(QIODevice::WriteOnly) && file.write(data) == data.size();
        };
        QVERIFY(write(command, "#!/bin/sh\ncat '" + response.toUtf8() + "'\n"));
        QVERIFY(QFile::setPermissions(command, QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner));
        QVERIFY(write(response, "{\"int\":0}"));

        Theme theme(dir.path(), nullptr, command);
        QCOMPARE(theme.radius(), 0);
        QVERIFY(write(response, "{\"int\":6}"));
        QTRY_COMPARE(theme.radius(), 6);
    }

    void translatorFallsBackToEnglish()
    {
        QTemporaryDir dir;
        Translator translator(dir.path() + "/language", dir.path());
        QCOMPARE(translator.language(), QString("en"));
        QVERIFY(translator.translate("Main", "Save").isNull());
    }

    void translatorFollowsTheLanguageFile()
    {
        QTemporaryDir dir;
        write(dir.path() + "/language", "pt-BR\n");
        write(dir.path() + "/pt.json", R"({"Save": "Guardar", "Open": "Abrir"})");
        write(dir.path() + "/pt-BR.json", R"({"Save": "Salvar"})");

        Translator translator(dir.path() + "/language", dir.path());
        QCOMPARE(translator.language(), QString("pt-BR"));
        QCOMPARE(translator.translate("Main", "Save"), QString("Salvar"));
        QCOMPARE(translator.translate("Main", "Open"), QString("Abrir"));
        QVERIFY(translator.translate("Main", "Quit").isNull());
    }

    void translatorPicksPluralForms()
    {
        QTemporaryDir dir;
        write(dir.path() + "/language", "ru\n");
        write(dir.path() + "/en.json", R"({"%n clips": {"one": "%n clip", "other": "%n clips"}})");
        write(dir.path() + "/ru.json", R"({"%n clips": {"one": "%n клип", "few": "%n клипа", "many": "%n клипов", "other": "%n клипа"}})");

        Translator translator(dir.path() + "/language", dir.path());
        QCOMPARE(translator.translate("Main", "%n clips", nullptr, 21), QString("%n клип"));
        QCOMPARE(translator.translate("Main", "%n clips", nullptr, 22), QString("%n клипа"));
        QCOMPARE(translator.translate("Main", "%n clips", nullptr, 25), QString("%n клипов"));
        QCOMPARE(Translator::pluralCategory("ar", 11), QString("many"));
        QCOMPARE(Translator::pluralCategory("ja", 1), QString("other"));
    }

private:
    static void write(const QString &path, const QByteArray &contents)
    {
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write(contents);
    }
};

QTEST_MAIN(OpwallTests)
#include "opwall_tests.moc"
