#pragma once

#include <QRandomGenerator>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

// Makes up everything the wall shows: names, numbers, graphs, code, notes.
// It is deterministic for a seed, so a wall can be reproduced (`--seed`) and
// tested, and every panel draws from the same word banks so one wall reads as
// one fictional operation.
class Generator {
public:
    explicit Generator(quint32 seed);

    int range(int low, int high);          // inclusive
    double real(double low = 0, double high = 1);
    bool chance(double probability);
    QString pick(const QStringList &list);

    QString adjective();
    QString noun();
    QString folder();
    QString camel(int words);              // "amberHarbor"
    QString pascal(int words);             // "AmberHarbor"
    QString kebab(int words);              // "amber-harbor"
    QString phrase();                      // "route the cobalt ledger"
    QString question();
    QString hex(int digits);

    QVariantMap header();
    QVariantList panels();                 // seven {title, subtitle, tag}
    QVariantMap graph();
    QVariantMap canvas();
    QVariantMap registry();
    QVariantList code();                   // lines of {text, role} tokens
    QVariantMap doc();
    QVariantMap radial();
    QVariantMap timeline();
    QVariantList writer();                 // lines of {text, role}
    QStringList ticker();

    // Splits a line of TypeScript-ish code into colored tokens. Public so the
    // tests can pin down what the editor panel highlights.
    static QVariantList tokenize(const QString &line);

private:
    QRandomGenerator m_rng;
};
