#pragma once

#include <QColor>
#include <QElapsedTimer>
#include <QPointF>
#include <QQuickPaintedItem>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>
#include <QVector>

// The big node graph. A few hundred nodes laid out by a live force
// simulation are too much for a QML Canvas to redraw every frame, so the
// simulation runs here and paints with QPainter. The graph assembles itself
// from the center whenever it gets a new model, then keeps breathing.
class GraphItem : public QQuickPaintedItem {
    Q_OBJECT
    Q_PROPERTY(QVariantMap model READ model WRITE setModel NOTIFY modelChanged)
    Q_PROPERTY(QVariantList colors READ colors WRITE setColors NOTIFY paletteChanged)
    Q_PROPERTY(QColor accent MEMBER m_accent WRITE setAccent NOTIFY paletteChanged)
    Q_PROPERTY(QColor surface MEMBER m_surface WRITE setSurface NOTIFY paletteChanged)
    Q_PROPERTY(QString fontFamily MEMBER m_fontFamily NOTIFY paletteChanged)
    Q_PROPERTY(bool running READ running WRITE setRunning NOTIFY runningChanged)
    Q_PROPERTY(int nodeCount READ nodeCount NOTIFY modelChanged)
    Q_PROPERTY(int edgeCount READ edgeCount NOTIFY modelChanged)
    Q_PROPERTY(double zoom READ zoom NOTIFY zoomChanged)

public:
    explicit GraphItem(QQuickItem *parent = nullptr);

    QVariantMap model() const { return m_model; }
    void setModel(const QVariantMap &model);
    QVariantList colors() const { return m_paletteList; }
    void setColors(const QVariantList &colors);
    void setAccent(const QColor &color);
    void setSurface(const QColor &color);
    bool running() const { return m_timer.isActive(); }
    void setRunning(bool running);
    int nodeCount() const { return int(m_position.size()); }
    int edgeCount() const { return int(m_edges.size()); }
    double zoom() const { return m_zoom; }

    void paint(QPainter *painter) override;

    // One tick of the simulation, public so tests can settle a layout
    // without a window.
    void step();
    QPointF position(int node) const { return m_position.value(node); }

signals:
    void modelChanged();
    void paletteChanged();
    void runningChanged();
    void zoomChanged();

private:
    struct Pulse { int edge; double t; double speed; };

    QColor clusterColor(int cluster) const;
    QPointF anchor(int cluster) const;
    double aspect() const;

    QVariantMap m_model;
    QVariantList m_paletteList;
    QVector<QColor> m_palette;
    QColor m_accent = QColor("#FFD60A");
    QColor m_surface = QColor("#0d0d0f");
    QString m_fontFamily = "monospace";

    QVector<int> m_cluster;               // -1 for the root
    QVector<QPointF> m_position, m_velocity;
    QVector<QPair<int, int>> m_edges;
    QVector<int> m_degree;
    QVector<int> m_hubs;                  // node index per cluster
    QStringList m_clusterNames;
    QString m_rootName, m_rootLetter;

    QVector<double> m_highlight;          // per edge, 0..1
    QVector<int> m_highlightTarget;       // edges fading in
    QVector<Pulse> m_pulses;

    QTimer m_timer;
    double m_alpha = 1;
    double m_rotation = 0;
    double m_zoom = 1;
    quint64 m_ticks = 0;
};
