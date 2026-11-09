#pragma once

#include <QObject>
#include <QVariantList>
#include <QVariantMap>

// The wall's state: the seed everything is generated from, whether it is
// paused, and the content of each panel. QML only animates what it is given;
// anything that is made up comes from here, through the Generator.
class Backend : public QObject {
    Q_OBJECT
    Q_PROPERTY(quint32 seed READ seed NOTIFY rerolled)
    Q_PROPERTY(bool paused READ paused WRITE setPaused NOTIFY pausedChanged)
    Q_PROPERTY(QVariantMap header READ header NOTIFY rerolled)
    Q_PROPERTY(QVariantList panels READ panels NOTIFY rerolled)
    Q_PROPERTY(QVariantMap graph READ graph NOTIFY rerolled)
    Q_PROPERTY(QVariantMap canvas READ canvas NOTIFY rerolled)
    Q_PROPERTY(QVariantMap registry READ registry NOTIFY rerolled)
    Q_PROPERTY(QVariantMap doc READ doc NOTIFY rerolled)
    Q_PROPERTY(QVariantMap radial READ radial NOTIFY rerolled)
    Q_PROPERTY(QVariantMap timeline READ timeline NOTIFY rerolled)
    Q_PROPERTY(QStringList ticker READ ticker NOTIFY rerolled)
    Q_PROPERTY(QVariantList code READ code NOTIFY codeChanged)
    Q_PROPERTY(QVariantList writer READ writer NOTIFY writerChanged)

public:
    explicit Backend(quint32 seed = 0, QObject *parent = nullptr);

    quint32 seed() const { return m_seed; }
    bool paused() const { return m_paused; }
    void setPaused(bool paused);

    QVariantMap header() const { return m_header; }
    QVariantList panels() const { return m_panels; }
    QVariantMap graph() const { return m_graph; }
    QVariantMap canvas() const { return m_canvas; }
    QVariantMap registry() const { return m_registry; }
    QVariantMap doc() const { return m_doc; }
    QVariantMap radial() const { return m_radial; }
    QVariantMap timeline() const { return m_timeline; }
    QStringList ticker() const { return m_ticker; }
    QVariantList code() const { return m_code; }
    QVariantList writer() const { return m_writer; }

    // Space pauses and resumes everything; R makes a whole new wall.
    Q_INVOKABLE void togglePaused() { setPaused(!m_paused); }
    Q_INVOKABLE void reroll();
    // The typing panels ask for a fresh snippet each time they finish one.
    Q_INVOKABLE void nextCode();
    Q_INVOKABLE void nextWriter();

signals:
    void rerolled();
    void pausedChanged();
    void codeChanged();
    void writerChanged();

private:
    void generate(quint32 seed);

    quint32 m_seed = 0;
    quint32 m_round = 0;
    bool m_paused = false;
    QVariantMap m_header, m_graph, m_canvas, m_registry, m_doc, m_radial, m_timeline;
    QVariantList m_panels, m_code, m_writer;
    QStringList m_ticker;
};
