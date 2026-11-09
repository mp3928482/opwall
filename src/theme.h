#pragma once

#include <QFileSystemWatcher>
#include <QObject>
#include <QProcess>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>

// Follows the Omarchy theme live. Besides the accent, an ops wall wants the
// whole palette, so every named color in colors.toml is exposed, with the
// panel colors gathered into `palette` in a fixed order.
class Theme : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString accent READ accent NOTIFY changed)
    Q_PROPERTY(QString accentForeground READ accentForeground NOTIFY changed)
    Q_PROPERTY(int radius READ radius NOTIFY changed)
    Q_PROPERTY(QVariantMap colors READ colors NOTIFY changed)
    Q_PROPERTY(QVariantList palette READ palette NOTIFY changed)
    Q_PROPERTY(QString background READ background NOTIFY changed)
    Q_PROPERTY(QString surface READ surface NOTIFY changed)
    Q_PROPERTY(QString foreground READ foreground NOTIFY changed)
    Q_PROPERTY(QString muted READ muted NOTIFY changed)

public:
    explicit Theme(const QString &currentDirectory = {}, QObject *parent = nullptr, const QString &hyprctl = {});
    ~Theme() override;

    QString accent() const { return m_accent; }
    QString accentForeground() const;
    int radius() const { return m_radius; }
    QVariantMap colors() const { return m_colors; }
    QVariantList palette() const;
    QString background() const;
    QString surface() const;
    QString foreground() const;
    QString muted() const;

    static QString readAccent(const QString &path);
    static QVariantMap readColors(const QString &path);
    static QVariantMap defaultColors();

signals:
    void changed();

private:
    void reload();
    void refreshRounding();
    QString color(const QString &key) const;
    bool isLight() const;

    QString m_directory;
    QString m_accent = "#FFD60A";
    QVariantMap m_colors;
    QFileSystemWatcher m_watcher;
    QTimer m_debounce;
    QString m_hyprctl;
    int m_radius = 0;
    QProcess m_roundingProcess;
    QTimer m_roundingPoll, m_roundingTimeout;
};
