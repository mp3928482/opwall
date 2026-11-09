#include "graphitem.h"

#include <QImage>
#include <QPainter>
#include <QQuickWindow>
#include <QRandomGenerator>
#include <QSGImageNode>
#include <QSGRendererInterface>
#include <QSGTexture>
#include <QSGFlatColorMaterial>
#include <QSGGeometryNode>
#include <QSGVertexColorMaterial>
#include <cmath>

namespace {

constexpr double kPi = 3.14159265358979323846;
constexpr int kSegments = 12;

double rnd() { return QRandomGenerator::global()->generateDouble(); }
int pick(int count) { return int(QRandomGenerator::global()->bounded(count)); }

// Vertex colors are premultiplied.
void setVertex(QSGGeometry::ColoredPoint2D &v, QPointF p, const QColor &c, double alpha) {
    const double a = std::clamp(alpha, 0.0, 1.0);
    v.set(float(p.x()), float(p.y()), uchar(c.red() * a), uchar(c.green() * a), uchar(c.blue() * a), uchar(255 * a));
}

// Collects feathered discs as indexed triangles: a center, a ring at the
// disc's edge, and a transparent ring just outside it, which gives smooth
// edges without multisampling and soft glows for free.
struct Discs {
    QVector<QSGGeometry::ColoredPoint2D> vertices;
    QVector<quint32> indices;

    void add(QPointF center, double radius, double feather, const QColor &color, double centerAlpha, double edgeAlpha) {
        const quint32 base = quint32(vertices.size());
        vertices.resize(vertices.size() + 1 + kSegments * 2);
        setVertex(vertices[base], center, color, centerAlpha);
        for (int k = 0; k < kSegments; ++k) {
            const double angle = 2 * kPi * k / kSegments;
            const QPointF direction(std::cos(angle), std::sin(angle));
            setVertex(vertices[base + 1 + k], center + direction * radius, color, edgeAlpha);
            setVertex(vertices[base + 1 + kSegments + k], center + direction * (radius + feather), color, 0);
        }
        for (int k = 0; k < kSegments; ++k) {
            const quint32 inner = base + 1 + k, innerNext = base + 1 + (k + 1) % kSegments;
            const quint32 outer = inner + kSegments, outerNext = innerNext + kSegments;
            indices << base << inner << innerNext << inner << outer << outerNext << inner << outerNext << innerNext;
        }
    }
};

QSGGeometryNode *makeNode(QSGGeometry::DrawingMode mode, bool indexed) {
    auto *node = new QSGGeometryNode;
    auto *geometry = indexed
        ? new QSGGeometry(QSGGeometry::defaultAttributes_ColoredPoint2D(), 0, 0, QSGGeometry::UnsignedIntType)
        : new QSGGeometry(QSGGeometry::defaultAttributes_ColoredPoint2D(), 0);
    geometry->setDrawingMode(mode);
    geometry->setLineWidth(1);
    geometry->setVertexDataPattern(QSGGeometry::StreamPattern);
    geometry->setIndexDataPattern(QSGGeometry::StreamPattern);
    node->setGeometry(geometry);
    node->setFlag(QSGNode::OwnsGeometry);
    node->setMaterial(new QSGVertexColorMaterial);
    node->setFlag(QSGNode::OwnsMaterial);
    return node;
}

} // namespace

GraphItem::GraphItem(QQuickItem *parent) : QQuickItem(parent) {
    setFlag(ItemHasContents, true);
    m_timer.setInterval(33);
    connect(&m_timer, &QTimer::timeout, this, [this] {
        step();
        update();
        emit framed();
    });
    m_timer.start();
}

void GraphItem::setRunning(bool running) {
    if (running == m_timer.isActive())
        return;
    running ? m_timer.start() : m_timer.stop();
    emit runningChanged();
}

void GraphItem::setColors(const QVariantList &colors) {
    m_colorList = colors;
    m_colors.clear();
    for (const auto &value : colors)
        m_colors << QColor(value.toString());
    emit colorsChanged();
    update();
}

void GraphItem::setAccent(const QColor &color) {
    m_accent = color;
    emit colorsChanged();
    update();
}

QColor GraphItem::clusterColor(int cluster) const {
    if (cluster < 0 || m_colors.isEmpty())
        return m_accent;
    return m_colors.at(cluster % m_colors.size());
}

double GraphItem::aspect() const {
    if (height() <= 0)
        return 1.6;
    return std::clamp(width() / height(), .7, 3.2);
}

// Clusters sit on an ellipse that fills the panel and turns very slowly, so
// the graph never quite stops moving.
QPointF GraphItem::anchor(int cluster) const {
    if (cluster < 0 || m_hubs.isEmpty())
        return {0, 0};
    const double angle = 2 * kPi * cluster / m_hubs.size() + m_rotation;
    const double wobble = .85 + .15 * std::sin(cluster * 2.3);
    return {std::cos(angle) * .74 * aspect() * wobble, std::sin(angle) * .62 * wobble};
}

QPointF GraphItem::map(int node) const {
    const double scale = height() / 2 * .9 * m_zoom;
    return QPointF(width() / 2, height() / 2) + m_position.value(node) * scale;
}

QVariantList GraphItem::hubPoints() const {
    QVariantList points;
    for (int hub : m_hubs)
        points << map(hub);
    return points;
}

void GraphItem::setModel(const QVariantMap &model) {
    m_model = model;
    m_cluster.clear();
    m_edges.clear();
    m_hubs.clear();
    m_pulses.clear();

    for (const auto &value : model.value("nodes").toList())
        m_cluster << value.toInt();
    for (const auto &value : model.value("clusters").toList())
        m_hubs << value.toMap().value("hub").toInt();
    const auto edges = model.value("edges").toList();
    for (int i = 0; i + 1 < edges.size(); i += 2) {
        const int a = edges.at(i).toInt(), b = edges.at(i + 1).toInt();
        if (a >= 0 && b >= 0 && a < m_cluster.size() && b < m_cluster.size() && a != b)
            m_edges << qMakePair(a, b);
    }

    // Everything starts in a tight knot at the center and blows outward, so
    // a reroll visibly assembles a new graph.
    const int n = int(m_cluster.size());
    m_position.resize(n);
    m_velocity.fill(QPointF(), n);
    m_degree.fill(0, n);
    m_adjacent.fill({}, n);
    for (int e = 0; e < m_edges.size(); ++e) {
        ++m_degree[m_edges[e].first];
        ++m_degree[m_edges[e].second];
        m_adjacent[m_edges[e].first] << e;
        m_adjacent[m_edges[e].second] << e;
    }
    for (int i = 0; i < n; ++i) {
        const double angle = rnd() * 2 * kPi, radius = rnd() * .08;
        m_position[i] = {std::cos(angle) * radius, std::sin(angle) * radius};
    }
    m_highlight.fill(0, int(m_edges.size()));
    m_lit.fill(false, int(m_edges.size()));
    m_alpha = 1;
    emit modelChanged();
    update();
}

// A bright thread: a random walk along the edges, lit together.
void GraphItem::pickThread() {
    m_lit.fill(false, int(m_edges.size()));
    if (m_edges.isEmpty())
        return;
    int node = m_edges[pick(int(m_edges.size()))].first;
    for (int k = 0; k < 16; ++k) {
        const auto &choices = m_adjacent[node];
        if (choices.isEmpty())
            break;
        const int e = choices[pick(int(choices.size()))];
        m_lit[e] = true;
        node = m_edges[e].first == node ? m_edges[e].second : m_edges[e].first;
    }
}

void GraphItem::step() {
    const int n = int(m_position.size());
    if (n == 0)
        return;
    ++m_ticks;
    m_rotation += .0012;
    // Cool from the explosion down to a floor that keeps the graph alive.
    m_alpha = std::max(.12, m_alpha * .985);

    const double newZoom = 1 + .06 * std::sin(m_ticks / 260.0);
    if (std::abs(newZoom - m_zoom) > .0005) {
        m_zoom = newZoom;
        emit zoomChanged();
    }

    // Repulsion between every pair, skipping the far ones; n is a few
    // hundred, so the plain double loop is cheap enough at 30 fps.
    for (int i = 0; i < n; ++i) {
        for (int j = i + 1; j < n; ++j) {
            const double dx = m_position[i].x() - m_position[j].x();
            const double dy = m_position[i].y() - m_position[j].y();
            const double d2 = dx * dx + dy * dy + 1e-5;
            if (d2 > .16)
                continue;
            const double force = 9e-6 / d2;
            m_velocity[i] += QPointF(dx * force, dy * force);
            m_velocity[j] -= QPointF(dx * force, dy * force);
        }
    }
    // Springs along the edges; the root's spokes are left to the anchors.
    for (const auto &edge : std::as_const(m_edges)) {
        if (m_cluster[edge.first] < 0 || m_cluster[edge.second] < 0)
            continue;
        const bool cross = m_cluster[edge.first] != m_cluster[edge.second];
        const QPointF delta = m_position[edge.second] - m_position[edge.first];
        const double distance = std::hypot(delta.x(), delta.y()) + 1e-6;
        const double rest = cross ? .6 : .08;
        const double force = (distance - rest) * (cross ? .0004 : .012) / distance;
        m_velocity[edge.first] += delta * force;
        m_velocity[edge.second] -= delta * force;
    }
    for (int i = 0; i < n; ++i) {
        const int cluster = m_cluster[i];
        if (cluster < 0) {
            m_position[i] = {0, 0};
            m_velocity[i] = {};
            continue;
        }
        const bool hub = m_hubs.value(cluster) == i;
        m_velocity[i] += (anchor(cluster) - m_position[i]) * (hub ? .03 : .0012);
        m_velocity[i] += QPointF(rnd() - .5, rnd() - .5) * .0002;
        m_velocity[i] *= .82;
        m_position[i] += m_velocity[i] * (.4 + m_alpha);
    }

    if (m_ticks % 70 == 1)
        pickThread();
    const bool holding = (m_ticks % 70) < 50;
    for (int e = 0; e < m_highlight.size(); ++e)
        m_highlight[e] = m_lit[e] && holding ? std::min(1.0, m_highlight[e] + .08) : std::max(0.0, m_highlight[e] - .03);

    // Packets travelling along random edges.
    if (m_pulses.size() < 16 && !m_edges.isEmpty() && rnd() < .35)
        m_pulses << Pulse{pick(int(m_edges.size())), 0, .015 + rnd() * .03};
    for (auto &pulse : m_pulses)
        pulse.t += pulse.speed;
    m_pulses.removeIf([](const Pulse &pulse) { return pulse.t >= 1; });
}

void GraphItem::paintSoftware(QImage &image) const {
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing, true);
    auto tinted = [](QColor c, double alpha) { c.setAlphaF(std::clamp(alpha, 0.0, 1.0)); return c; };
    for (const auto &edge : std::as_const(m_edges)) {
        const bool spoke = m_cluster[edge.first] < 0 || m_cluster[edge.second] < 0;
        const int cluster = m_cluster[edge.first] < 0 ? m_cluster[edge.second] : m_cluster[edge.first];
        painter.setPen(QPen(tinted(spoke ? m_accent : clusterColor(cluster), spoke ? .4 : .28), .8));
        painter.drawLine(map(edge.first), map(edge.second));
    }
    for (int e = 0; e < m_edges.size(); ++e) {
        if (m_highlight[e] <= 0)
            continue;
        painter.setPen(QPen(tinted(Qt::white, .9 * m_highlight[e]), 1.2));
        painter.drawLine(map(m_edges[e].first), map(m_edges[e].second));
    }
    painter.setPen(Qt::NoPen);
    for (int i = 0; i < m_position.size(); ++i) {
        const int cluster = m_cluster[i];
        if (cluster < 0)
            continue;
        const bool hub = m_hubs.value(cluster) == i;
        const double radius = hub ? 5.5 : 1.3 + std::sqrt(double(m_degree[i])) * .8;
        if (hub || m_degree[i] > 3) {
            painter.setBrush(tinted(clusterColor(cluster), .16));
            painter.drawEllipse(map(i), radius * 2.6, radius * 2.6);
        }
        painter.setBrush(tinted(clusterColor(cluster), !hub && m_degree[i] <= 1 ? .75 : 1));
        painter.drawEllipse(map(i), radius, radius);
    }
    for (const auto &pulse : std::as_const(m_pulses)) {
        const auto &edge = m_edges[pulse.edge];
        const QPointF p = map(edge.first) + (map(edge.second) - map(edge.first)) * pulse.t;
        painter.setBrush(tinted(Qt::white, .25));
        painter.drawEllipse(p, 4.5, 4.5);
        painter.setBrush(Qt::white);
        painter.drawEllipse(p, 1.8, 1.8);
    }
    painter.setBrush(tinted(m_accent, .22));
    painter.drawEllipse(map(0), 18, 18);
}

QSGNode *GraphItem::updatePaintNode(QSGNode *old, UpdatePaintNodeData *) {
    if (window() && window()->rendererInterface()->graphicsApi() == QSGRendererInterface::Software) {
        auto *node = static_cast<QSGImageNode *>(old);
        if (!node)
            node = window()->createImageNode();
        const qreal dpr = window()->effectiveDevicePixelRatio();
        QImage image(QSize(int(width() * dpr), int(height() * dpr)).expandedTo(QSize(1, 1)),
                     QImage::Format_ARGB32_Premultiplied);
        image.setDevicePixelRatio(dpr);
        image.fill(Qt::transparent);
        paintSoftware(image);
        node->setTexture(window()->createTextureFromImage(image));
        node->setOwnsTexture(true);
        node->setRect(boundingRect());
        return node;
    }

    QSGNode *root = old;
    if (!root) {
        root = new QSGNode;
        root->appendChildNode(makeNode(QSGGeometry::DrawLines, false));
        root->appendChildNode(makeNode(QSGGeometry::DrawTriangles, true));
    }
    auto *lineNode = static_cast<QSGGeometryNode *>(root->firstChild());
    auto *discNode = static_cast<QSGGeometryNode *>(root->lastChild());

    // Edges as hairlines in their cluster's color, with the lit thread in
    // white on top.
    QVector<QSGGeometry::ColoredPoint2D> lines;
    lines.reserve((m_edges.size() + 20) * 2);
    for (int e = 0; e < m_edges.size(); ++e) {
        const auto &edge = m_edges[e];
        const int cluster = m_cluster[edge.first] < 0 ? m_cluster[edge.second] : m_cluster[edge.first];
        const bool spoke = m_cluster[edge.first] < 0 || m_cluster[edge.second] < 0;
        const QColor color = spoke ? m_accent : clusterColor(cluster);
        const double alpha = spoke ? .4 : .28;
        lines.resize(lines.size() + 2);
        setVertex(lines[lines.size() - 2], map(edge.first), color, alpha);
        setVertex(lines[lines.size() - 1], map(edge.second), color, alpha);
    }
    for (int e = 0; e < m_edges.size(); ++e) {
        if (m_highlight[e] <= 0)
            continue;
        lines.resize(lines.size() + 2);
        setVertex(lines[lines.size() - 2], map(m_edges[e].first), Qt::white, .9 * m_highlight[e]);
        setVertex(lines[lines.size() - 1], map(m_edges[e].second), Qt::white, .9 * m_highlight[e]);
    }
    QSGGeometry *lineGeometry = lineNode->geometry();
    lineGeometry->allocate(int(lines.size()));
    std::copy(lines.cbegin(), lines.cend(), lineGeometry->vertexDataAsColoredPoint2D());
    lineNode->markDirty(QSGNode::DirtyGeometry);

    // Nodes with glows, the packets, and the root's halo.
    Discs discs;
    discs.vertices.reserve(int(m_position.size()) * 2 * (1 + kSegments * 2));
    for (int i = 0; i < m_position.size(); ++i) {
        const int cluster = m_cluster[i];
        if (cluster < 0)
            continue;
        const bool hub = m_hubs.value(cluster) == i;
        const double radius = hub ? 5.5 : 1.3 + std::sqrt(double(m_degree[i])) * .8;
        const QColor color = clusterColor(cluster);
        if (hub || m_degree[i] > 3)
            discs.add(map(i), radius * .6, radius * 2.4, color, .3, .22);
        const double alpha = !hub && m_degree[i] <= 1 ? .75 : 1;
        discs.add(map(i), radius, 1, color, alpha, alpha);
    }
    for (const auto &pulse : std::as_const(m_pulses)) {
        const auto &edge = m_edges[pulse.edge];
        const QPointF p = map(edge.first) + (map(edge.second) - map(edge.first)) * pulse.t;
        discs.add(p, 1.6, 4, Qt::white, 1, .9);
    }
    const QPointF center = map(0);
    discs.add(center, 10, 12, m_accent, .35, .25);

    QSGGeometry *discGeometry = discNode->geometry();
    discGeometry->allocate(int(discs.vertices.size()), int(discs.indices.size()));
    std::copy(discs.vertices.cbegin(), discs.vertices.cend(), discGeometry->vertexDataAsColoredPoint2D());
    std::copy(discs.indices.cbegin(), discs.indices.cend(), discGeometry->indexDataAsUInt());
    discNode->markDirty(QSGNode::DirtyGeometry);
    return root;
}
