#pragma once

#include <QColor>
#include <QPointF>
#include <QQuickItem>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>
#include <QVector>

// The big node graph. A few hundred nodes laid out by a live force
// simulation, drawn straight into the scene graph as vertex-colored lines
// and feathered discs so the GPU does the drawing; painting them with
// QPainter every frame cost most of a CPU core. The graph assembles itself
// from the center whenever it gets a new model, then keeps breathing.
// Labels are plain QML items placed on `hubPoints`.
class GraphItem : public QQuickItem {
    Q_OBJECT
    Q_PROPERTY(QVariantMap model READ model WRITE setModel NOTIFY modelChanged)
    Q_PROPERTY(QVariantList colors READ colors WRITE setColors NOTIFY colorsChanged)
    Q_PROPERTY(QColor accent READ accent WRITE setAccent NOTIFY colorsChanged)
    Q_PROPERTY(bool running READ running WRITE setRunning NOTIFY runningChanged)
    Q_PROPERTY(int nodeCount READ nodeCount NOTIFY modelChanged)
    Q_PROPERTY(int edgeCount READ edgeCount NOTIFY modelChanged)
    Q_PROPERTY(double zoom READ zoom NOTIFY zoomChanged)
    Q_PROPERTY(QVariantList hubPoints READ hubPoints NOTIFY framed)

public:
    explicit GraphItem(QQuickItem *parent = nullptr);

    QVariantMap model() const { return m_model; }
    void setModel(const QVariantMap &model);
    QVariantList colors() const { return m_colorList; }
    void setColors(const QVariantList &colors);
    QColor accent() const { return m_accent; }
    void setAccent(const QColor &color);
    bool running() const { return m_timer.isActive(); }
    void setRunning(bool running);
    int nodeCount() const { return int(m_position.size()); }
    int edgeCount() const { return int(m_edges.size()); }
    double zoom() const { return m_zoom; }
    QVariantList hubPoints() const;

    // One tick of the simulation, public so tests can settle a layout
    // without a window.
    void step();
    QPointF position(int node) const { return m_position.value(node); }
    // Where a node is drawn, in item coordinates.
    QPointF map(int node) const;

signals:
    void modelChanged();
    void colorsChanged();
    void runningChanged();
    void zoomChanged();
    void framed();

protected:
    QSGNode *updatePaintNode(QSGNode *old, UpdatePaintNodeData *) override;

private:
    struct Pulse { int edge; double t; double speed; };

    QColor clusterColor(int cluster) const;
    QPointF anchor(int cluster) const;
    double aspect() const;
    void pickThread();
    // Qt's software renderer (no GPU, or offscreen) ignores custom geometry,
    // so there the same picture is painted into an image instead.
    void paintSoftware(QImage &image) const;

    QVariantMap m_model;
    QVariantList m_colorList;
    QVector<QColor> m_colors;
    QColor m_accent = QColor("#FFD60A");

    QVector<int> m_cluster;               // -1 for the root
    QVector<QPointF> m_position, m_velocity;
    QVector<QPair<int, int>> m_edges;
    QVector<QVector<int>> m_adjacent;     // edge indices per node
    QVector<int> m_degree;
    QVector<int> m_hubs;                  // node index per cluster

    QVector<double> m_highlight;          // per edge, 0..1
    QVector<bool> m_lit;                  // edges in the current thread
    QVector<Pulse> m_pulses;

    QTimer m_timer;
    double m_alpha = 1;
    double m_rotation = 0;
    double m_zoom = 1;
    quint64 m_ticks = 0;
};
