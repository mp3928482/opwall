#include "generator.h"

#include <QDate>
#include <QRegularExpression>
#include <QSet>
#include <algorithm>

namespace {

const QStringList kAdjectives{
    "amber", "binary", "cobalt", "copper", "crimson", "deep", "distant", "ember", "feral", "frozen",
    "glass", "granite", "hidden", "hollow", "iron", "latent", "lunar", "neon", "northern", "polar",
    "quiet", "rapid", "silent", "solar", "static", "stray", "velvet", "vivid", "wired", "zero"};

const QStringList kNouns{
    "anchor", "archive", "atlas", "beacon", "canopy", "cipher", "circuit", "compass", "cortex", "current",
    "engine", "foundry", "garden", "harbor", "horizon", "index", "kernel", "lattice", "ledger", "mirror",
    "needle", "orbit", "prism", "quarry", "relay", "signal", "spire", "vault", "vector", "wave"};

const QStringList kFolders{
    "Daily", "Signals", "Archive", "References", "Sources", "Fieldnotes", "Drafts", "Templates",
    "Captures", "Projects", "People", "Maps", "Logs", "Inbox", "Assets", "Outposts"};

const QStringList kVerbs{
    "route", "index", "mirror", "replay", "patch", "trace", "sync", "seal", "rotate", "map",
    "decode", "archive", "stitch", "probe", "anchor", "sweep"};

const QStringList kPeriods{"week", "quarter", "year", "season", "sprint", "shift"};

const QStringList kTags{"live", "sync", "raw feed", "streaming", "verified", "read only", "mirrored",
                        "encrypted", "nightly", "beta"};

QString capitalized(QString word) {
    if (!word.isEmpty())
        word[0] = word[0].toUpper();
    return word;
}

} // namespace

Generator::Generator(quint32 seed) : m_rng(seed) {}

int Generator::range(int low, int high) { return low + int(m_rng.bounded(quint32(high - low + 1))); }
double Generator::real(double low, double high) { return low + m_rng.generateDouble() * (high - low); }
bool Generator::chance(double probability) { return m_rng.generateDouble() < probability; }
QString Generator::pick(const QStringList &list) { return list.at(int(m_rng.bounded(quint32(list.size())))); }

QString Generator::adjective() { return pick(kAdjectives); }
QString Generator::noun() { return pick(kNouns); }
QString Generator::folder() { return pick(kFolders); }

QString Generator::camel(int words) {
    QString result = chance(.5) ? adjective() : pick(kVerbs);
    for (int i = 1; i < words; ++i)
        result += capitalized(noun());
    return result;
}

QString Generator::pascal(int words) { return capitalized(camel(words)); }

QString Generator::kebab(int words) {
    QStringList parts{adjective()};
    for (int i = 1; i < words; ++i)
        parts << noun();
    return parts.join('-');
}

QString Generator::phrase() {
    switch (range(0, 3)) {
    case 0: return QString("%1 the %2 %3").arg(pick(kVerbs), adjective(), noun());
    case 1: return QString("%1 every %2").arg(pick(kVerbs), noun());
    case 2: return QString("%1 %2 before the %3").arg(pick(kVerbs), noun(), pick(kPeriods));
    default: return QString("keep the %1 %2").arg(noun(), pick({"warm", "sealed", "in sync", "honest", "quiet"}));
    }
}

QString Generator::question() {
    switch (range(0, 4)) {
    case 0: return QString("what did the %1 teach you this %2?").arg(noun(), pick(kPeriods));
    case 1: return QString("which %1 would you rebuild first?").arg(noun());
    case 2: return QString("where did the %1 %2 drift?").arg(adjective(), noun());
    case 3: return QString("who keeps the %1 after this %2?").arg(noun(), pick(kPeriods));
    default: return QString("how many %1s did you %2?").arg(noun(), pick(kVerbs));
    }
}

QString Generator::hex(int digits) {
    QString result;
    for (int i = 0; i < digits; ++i)
        result += QString::number(range(0, 15), 16);
    return result;
}

QVariantMap Generator::header() {
    QStringList subtitle;
    for (int i = 0; i < 5; ++i)
        subtitle << QString("%1 %2").arg(adjective(), noun());
    QVariantList stats;
    const int big = range(1200, 9800);
    stats << QVariantMap{{"label", "feeds"}, {"value", range(5, 9)}, {"color", 0}}
          << QVariantMap{{"label", pick({"events", "packets", "nodes", "links"})}, {"value", big}, {"color", 3}}
          << QVariantMap{{"label", pick({"alerts", "threads", "agents", "routes"})}, {"value", range(120, 480)}, {"color", 2}}
          << QVariantMap{{"label", pick({"sectors", "probes", "relays", "keys"})}, {"value", range(12, 64)}, {"color", 4}};
    return {
        {"org", QString("%1 %2").arg(noun(), pick({"unit", "group", "cell", "desk", "works"})).toUpper()},
        {"title", QString("the %1 %2 · %3").arg(adjective(), noun(), pick({"live", "on watch", "in sync", "online"})).toUpper()},
        {"subtitle", subtitle.join("  ·  ")},
        {"stats", stats},
    };
}

QVariantList Generator::panels() {
    QVariantList list;
    QSet<QString> used;
    for (int i = 0; i < 7; ++i) {
        QString title;
        do title = kebab(2); while (used.contains(title));
        used.insert(title);
        list << QVariantMap{{"title", title}, {"subtitle", phrase()}, {"tag", pick(kTags)}};
    }
    return list;
}

// A knowledge graph: a root, one hub per cluster, and cluster members that
// hang off each other in loose trees, with a few links across clusters so
// the picture has the long bright threads of a real vault.
QVariantMap Generator::graph() {
    QStringList folders = kFolders;
    std::shuffle(folders.begin(), folders.end(), m_rng);
    const int clusterCount = 7;

    QVariantList clusters, nodes, edges;
    nodes << -1; // the root
    for (int c = 0; c < clusterCount; ++c) {
        nodes << c; // the hub
        clusters << QVariantMap{{"name", folders.at(c)}, {"count", range(84, 186)}, {"hub", c + 1}};
        edges << 0 << c + 1;
    }
    QList<QList<int>> members(clusterCount);
    for (int c = 0; c < clusterCount; ++c)
        members[c] << c + 1;
    for (int c = 0; c < clusterCount; ++c) {
        const int size = range(48, 84);
        for (int k = 0; k < size; ++k) {
            const int index = int(nodes.size());
            nodes << c;
            // Mostly attach near the hub, sometimes deep in the tree.
            const int parent = chance(.35) ? members[c].first() : members[c].at(range(0, int(members[c].size()) - 1));
            edges << parent << index;
            members[c] << index;
        }
    }
    const int crossLinks = int(nodes.size()) / 6;
    for (int k = 0; k < crossLinks; ++k) {
        const int a = range(1, int(nodes.size()) - 1), b = range(1, int(nodes.size()) - 1);
        if (a != b)
            edges << a << b;
    }
    const QString name = kebab(2);
    return {{"clusters", clusters}, {"nodes", nodes}, {"edges", edges},
            {"root", name}, {"rootLetter", name.left(1)}};
}

QVariantMap Generator::canvas() {
    QStringList labels = kNouns;
    std::shuffle(labels.begin(), labels.end(), m_rng);
    // Six boxes on a loose 3x2 grid, so they never start on top of each other.
    QVariantList boxes;
    for (int i = 0; i < 6; ++i) {
        const double x = (i % 3) * .32 + real(.02, .1), y = (i / 3) * .48 + real(.04, .2);
        boxes << QVariantMap{{"label", capitalized(labels.at(i))}, {"x", x}, {"y", y}, {"color", range(0, 6)}};
    }
    QVariantList edges;
    QSet<QPair<int, int>> seen;
    while (edges.size() < 6) {
        int a = range(0, 5), b = range(0, 5);
        if (a == b) continue;
        if (a > b) std::swap(a, b);
        if (seen.contains({a, b})) continue;
        seen.insert({a, b});
        edges << QVariantList{a, b};
    }
    return {{"boxes", boxes}, {"edges", edges}, {"file", kebab(2) + ".canvas"}};
}

QVariantMap Generator::registry() {
    QVariantList rows;
    QSet<QString> used;
    while (rows.size() < 14) {
        const QString name = chance(.5) ? capitalized(noun()) : capitalized(adjective()) + " " + capitalized(noun());
        if (used.contains(name)) continue;
        used.insert(name);
        rows << QVariantMap{{"name", name}, {"value", range(300000, 4600000)}};
    }
    QVariantList cards;
    for (int i = 0; i < 8; ++i)
        cards << QVariantMap{{"title", capitalized(adjective()) + " " + capitalized(noun())},
                             {"color", range(0, 6)}, {"lines", range(2, 4)}};
    return {{"rows", rows}, {"cards", cards}, {"rowsFile", kebab(1) + "-registry.json"},
            {"cardsFile", kebab(1) + "-catalog.json"}};
}

QVariantList Generator::code() {
    const QString cls = pascal(2), base = capitalized(noun()), helper = camel(2), field = camel(1);
    const QString package = kebab(2), module = noun(), command = kebab(2), modal = capitalized(noun()) + "Modal";
    QStringList lines{
        QString("import { %1, Notice } from '%2';").arg(base, package),
        QString("import { %1 } from './%2';").arg(helper, module),
        "",
        QString("export default class %1 extends %2 {").arg(cls, base),
        QString("  private readonly %1 = %2;").arg(field).arg(range(8, 512)),
        "",
        "  async onload() {",
        QString("    this.addCommand('%1', async () => {").arg(command),
        QString("      const %1 = await %2(this.%3);").arg(noun(), helper, field),
        QString("      if (!%1.ok) return new Notice('drift 0x%2');").arg(noun(), hex(4)),
        QString("      new %1(this.app, %2).open();").arg(modal, noun()),
        "    });",
        "  }",
        "}",
    };
    QVariantList result;
    for (const auto &line : lines)
        result << QVariant(tokenize(line));
    return result;
}

QVariantList Generator::tokenize(const QString &line) {
    static const QSet<QString> keywords{"import", "from", "export", "default", "class", "extends", "private",
                                        "readonly", "async", "await", "const", "if", "return", "new", "this"};
    static const QRegularExpression pattern(R"('[^']*'|\d+|[A-Za-z_]\w*|\s+|.)");
    QVariantList tokens;
    for (auto it = pattern.globalMatch(line); it.hasNext();) {
        const auto match = it.next();
        const QString text = match.captured(0);
        QString role = "plain";
        if (text.startsWith('\''))
            role = "string";
        else if (text.at(0).isDigit())
            role = "number";
        else if (keywords.contains(text))
            role = "keyword";
        else if (text.at(0).isLetter() && text.at(0).isUpper())
            role = "type";
        else if (text.at(0).isLetter() && line.mid(match.capturedEnd(0), 1) == "(")
            role = "function";
        else if (!text.at(0).isLetterOrNumber() && !text.at(0).isSpace())
            role = "punctuation";
        // Merge runs of the same role so the editor has fewer pieces to draw.
        if (!tokens.isEmpty() && tokens.last().toMap().value("role") == role) {
            auto last = tokens.last().toMap();
            last["text"] = last.value("text").toString() + text;
            tokens.last() = last;
        } else {
            tokens << QVariantMap{{"text", text}, {"role", role}};
        }
    }
    return tokens;
}

QVariantMap Generator::doc() {
    QStringList items;
    for (int i = 0; i < 6; ++i)
        items << capitalized(phrase());
    return {{"file", QString("notes/%1.md").arg(QDate::currentDate().toString(Qt::ISODate))}, {"items", items}};
}

QVariantMap Generator::radial() {
    QStringList questions;
    for (int i = 0; i < 8; ++i)
        questions << question();
    return {{"total", range(30, 48)}, {"questions", questions}};
}

QVariantMap Generator::timeline() {
    const int rows = range(9, 12);
    QVariantList blocks;
    for (int r = 0; r < rows; ++r) {
        double x = real(0, .25);
        while (x < .95) {
            const double length = real(.06, .22);
            blocks << QVariantMap{{"row", r}, {"start", x}, {"length", std::min(length, .99 - x)},
                                  {"color", range(0, 6)}};
            x += length + real(.04, .3);
        }
    }
    return {{"rows", rows}, {"blocks", blocks}};
}

QVariantList Generator::writer() {
    QVariantList lines;
    auto add = [&](const QString &text, const QString &role) { lines << QVariantMap{{"text", text}, {"role", role}}; };
    add("# " + capitalized(adjective()) + " " + noun(), "heading");
    add("", "plain");
    for (int i = 0; i < 3; ++i)
        add("- " + phrase(), "list");
    add("", "plain");
    add("> " + capitalized(question()), "quote");
    add("", "plain");
    add(QString("`%1()` keeps the %2 %3").arg(camel(2), noun(), pick({"warm", "sealed", "honest"})), "code");
    add("~ " + phrase() + ", " + phrase(), "plain");
    return lines;
}

QStringList Generator::ticker() {
    QStringList items;
    for (int i = 0; i < 14; ++i)
        items << (chance(.5) ? phrase() : kebab(2));
    return items;
}
