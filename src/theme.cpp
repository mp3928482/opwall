// Theme watching follows Omacut (MIT, David Heinemeier Hansson).
#include "theme.h"

#include <QColor>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QStandardPaths>
#include <cmath>

Theme::Theme(const QString &directory, QObject *parent, const QString &hyprctl)
    : QObject(parent),
      m_directory(directory.isEmpty() ? QDir::homePath() + "/.local/state/omarchy/current" : directory) {
    m_colors = defaultColors();
    m_debounce.setSingleShot(true);
    m_debounce.setInterval(80);
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this, [this] { m_debounce.start(); });
    connect(&m_watcher, &QFileSystemWatcher::fileChanged, this, [this] { m_debounce.start(); });
    connect(&m_debounce, &QTimer::timeout, this, &Theme::reload);

    // Corners follow Hyprland's effective rounding, which themes and personal
    // overrides can both set, so ask hyprctl rather than reading its config.
    // Without Hyprland the corners stay sharp, like Omarchy's default.
    m_hyprctl = !hyprctl.isEmpty() ? hyprctl
              : directory.isEmpty() && !qEnvironmentVariableIsEmpty("HYPRLAND_INSTANCE_SIGNATURE")
                  ? QStandardPaths::findExecutable("hyprctl") : QString();
    m_roundingTimeout.setSingleShot(true);
    m_roundingTimeout.setInterval(500);
    connect(&m_roundingTimeout, &QTimer::timeout, &m_roundingProcess, &QProcess::kill);
    connect(&m_roundingProcess, &QProcess::errorOccurred, this, [this] { m_roundingTimeout.stop(); });
    connect(&m_roundingProcess, &QProcess::finished, this, [this](int code, QProcess::ExitStatus status) {
        m_roundingTimeout.stop();
        const auto value = QJsonDocument::fromJson(m_roundingProcess.readAllStandardOutput()).object().value("int");
        if (code != 0 || status != QProcess::NormalExit || !value.isDouble() || value.toInt(-1) < 0)
            return;
        if (value.toInt() != m_radius) {
            m_radius = value.toInt();
            emit changed();
        }
    });
    m_roundingPoll.setInterval(1000);
    connect(&m_roundingPoll, &QTimer::timeout, this, &Theme::refreshRounding);
    if (!m_hyprctl.isEmpty())
        m_roundingPoll.start();
    reload();
}

Theme::~Theme() {
    if (m_roundingProcess.state() != QProcess::NotRunning) {
        m_roundingProcess.kill();
        m_roundingProcess.waitForFinished(1000);
    }
}

void Theme::refreshRounding() {
    if (m_hyprctl.isEmpty() || m_roundingProcess.state() != QProcess::NotRunning)
        return;
    m_roundingProcess.start(m_hyprctl, {"-j", "getoption", "decoration:rounding"});
    m_roundingTimeout.start();
}

QString Theme::readAccent(const QString &path) {
    return readColors(path).value("accent").toString();
}

// The colors Omarchy's default theme ships, used for any key a theme leaves
// out and whenever Omarchy isn't installed at all.
QVariantMap Theme::defaultColors() {
    return {
        {"mode", "dark"},
        {"accent", "#FFD60A"},
        {"background", "#121212"}, {"dark_background", "#0d0d0f"}, {"darker_background", "#08080a"},
        {"lighter_background", "#1e1e22"},
        {"foreground", "#e6e6e6"}, {"muted", "#8a8a8d"}, {"selection", "#2c2c2e"},
        {"red", "#FF453A"}, {"orange", "#FF9F0A"}, {"yellow", "#FFD60A"}, {"green", "#32D74B"},
        {"cyan", "#64D2FF"}, {"blue", "#0A84FF"}, {"magenta", "#BF5AF2"}, {"brown", "#AC8E68"},
        {"bright_red", "#FF6961"}, {"bright_yellow", "#FFD426"}, {"bright_green", "#30DB5B"},
        {"bright_cyan", "#70D7FF"}, {"bright_blue", "#409CFF"}, {"bright_magenta", "#DA8FFF"},
    };
}

// Every `key = "#rrggbb"` (and `mode = "dark"`) line, laid over the defaults.
QVariantMap Theme::readColors(const QString &path) {
    QVariantMap colors = defaultColors();
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return colors;
    const QString text = QString::fromUtf8(file.readAll());
    static const QRegularExpression line(R"re(^\s*([a-z_0-9]+)\s*=\s*["']([^"']*)["'])re",
                                         QRegularExpression::MultilineOption);
    static const QRegularExpression hex("^#[0-9a-fA-F]{6}$");
    for (auto it = line.globalMatch(text); it.hasNext();) {
        const auto match = it.next();
        const QString key = match.captured(1), value = match.captured(2);
        if (hex.match(value).hasMatch() || (key == "mode" && (value == "dark" || value == "light")))
            colors.insert(key, value);
    }
    return colors;
}

QString Theme::color(const QString &key) const {
    return m_colors.value(key, defaultColors().value(key)).toString();
}

bool Theme::isLight() const {
    if (m_colors.value("mode").toString() == "light")
        return true;
    return QColor(color("background")).lightnessF() > .6;
}

// The wall is a night-shift screen, so a light theme keeps its colors but
// sits on a dark ground instead of its paper background.
QString Theme::background() const { return isLight() ? "#0c0c0e" : color("darker_background"); }
QString Theme::surface() const { return isLight() ? "#141417" : color("dark_background"); }
QString Theme::foreground() const { return isLight() ? "#e8e6e1" : color("foreground"); }
QString Theme::muted() const { return isLight() ? "#8a8a8d" : color("muted"); }

QVariantList Theme::palette() const {
    QVariantList list;
    for (const char *key : {"yellow", "cyan", "magenta", "green", "orange", "blue", "red", "bright_magenta",
                            "bright_cyan", "bright_yellow"})
        list << color(key);
    return list;
}

// Black or white, whichever reads on the accent (WCAG relative luminance).
QString Theme::accentForeground() const {
    const QColor c(m_accent);
    auto linear = [](double v) { return v <= .04045 ? v / 12.92 : std::pow((v + .055) / 1.055, 2.4); };
    const double luminance = .2126 * linear(c.redF()) + .7152 * linear(c.greenF()) + .0722 * linear(c.blueF());
    return luminance > .179 ? "black" : "white";
}

void Theme::reload() {
    refreshRounding();
    const auto paths = m_watcher.files() + m_watcher.directories();
    if (!paths.isEmpty())
        m_watcher.removePaths(paths);

    // A theme switch replaces symlinks and files, so watch the parents too and
    // re-arm after every change. Watching the nearest existing ancestor means
    // installing Omarchy later works without restarting the app.
    QString ancestor = m_directory;
    while (!QFileInfo::exists(ancestor) && ancestor != "/")
        ancestor = QFileInfo(ancestor).absolutePath();
    QStringList candidates{ancestor, QFileInfo(ancestor).absolutePath(), m_directory,
                           m_directory + "/theme", m_directory + "/theme/colors.toml"};
    candidates.removeDuplicates();
    for (const auto &path : candidates)
        if (QFileInfo::exists(path))
            m_watcher.addPath(path);

    const auto colors = readColors(m_directory + "/theme/colors.toml");
    if (colors != m_colors) {
        m_colors = colors;
        m_accent = colors.value("accent").toString();
        emit changed();
    }
}
