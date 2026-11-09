#include "graphitem.h"

#include <QFont>
#include <QFontMetricsF>
#include <QPainter>
#include <QRandomGenerator>
#include <cmath>

namespace {
constexpr double kPi = 3.14159265358979323846;
double rnd() { return QRandomGenerator::global()->generateDouble(); }
} // namespace

GraphItem::GraphItem(QQuickItem *parent) : QQuickPaintedItem(parent) {
    setAntialiasing(true);
    setOpaquePainting(false);
    m_timer.setInterval(33);
    connect(&m_timer, &QTimer::timeout, this, [this] {
        step();
        update();
    });
    m_timer.start();
}

void GraphItem::setRunning(bool running) {
    if (running == m_timer.isActive())
        return;
    running ? m_timer.start() : m_timer.stop();
    emit runningChanged();
}

void GraphItem::setColors(const QVariantList &palette) {
    m_paletteList = palette;
    m_palette.clear();
    for (const auto &value : palette)
        m_palette << QColor(value.toString());
    emit paletteChanged();
    update();
}

void GraphItem::setAccent(const QColor &color) {
    m_accent = color;
    emit paletteChanged();
    update();
}

void GraphItem::setSurface(const QColor &color) {
    m_surface = color;
    emit paletteChanged();
    update();
}

QColor GraphItem::clusterColor(int cluster) const {
    if (cluster < 0 || m_palette.isEmpty())
        return m_accent;
    return m_palette.at(cluster % m_palette.size());
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

void GraphItem::setModel(const QVariantMap &model) {
    m_model = model;
    m_cluster.clear();
    m_edges.clear();
    m_hubs.clear();
    m_clusterNames.clear();
    m_pulses.clear();
    m_highlightTarget.clear();

    for (const auto &value : model.value("nodes").toList())
        m_cluster << value.toInt();
    for (const auto &value : model.value("clusters").toList()) {
        const auto cluster = value.toMap();
        m_hubs << cluster.value("hub").toInt();
        m_clusterNames << cluster.value("name").toString() + "/";
    }
    const auto edges = model.value("edges").toList();
    for (int i = 0; i + 1 < edges.size(); i += 2) {
        const int a = edges.at(i).toInt(), b = edges.at(i + 1).toInt();
        if (a >= 0 && b >= 0 && a < m_cluster.size() && b < m_cluster.size())
            m_edges << qMakePair(a, b);
    }
    m_rootName = model.value("root").toString();
    m_rootLetter = model.value("rootLetter").toString();

    // Everything starts in a tight knot at the center and blows outward, so
    // a reroll visibly assembles a new graph.
    const int n = int(m_cluster.size());
    m_position.resize(n);
    m_velocity.fill(QPointF(), n);
    m_degree.fill(0, n);
    for (const auto &edge : std::as_const(m_edges)) {
        ++m_degree[edge.first];
        ++m_degree[edge.second];
    }
    for (int i = 0; i < n; ++i) {
        const double angle = rnd() * 2 * kPi, radius = rnd() * .08;
        m_position[i] = {std::cos(angle) * radius, std::sin(angle) * radius};
    }
    m_highlight.fill(0, int(m_edges.size()));
    m_alpha = 1;
    emit modelChanged();
    update();
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

    // Bright threads: a handful of edges light up together and fade out.
    if (m_ticks % 70 == 1 && !m_edges.isEmpty()) {
        m_highlightTarget.clear();
        const int start = QRandomGenerator::global()->bounded(int(m_edges.size()));
        int node = m_edges[start].first;
        for (int k = 0; k < 18; ++k) {
            const int edge = QRandomGenerator::global()->bounded(int(m_edges.size()));
            // Prefer edges touching the current node, so the threads connect.
            for (int tries = 0; tries < 40; ++tries) {
                const int e = QRandomGenerator::global()->bounded(int(m_edges.size()));
                if (m_edges[e].first == node || m_edges[e].second == node) {
                    m_highlightTarget << e;
                    node = m_edges[e].first == node ? m_edges[e].second : m_edges[e].first;
                    break;
                }
                if (tries == 39)
                    m_highlightTarget << edge;
            }
        }
    }
    for (int e = 0; e < m_highlight.size(); ++e) {
        const bool lit = m_highlightTarget.contains(e) && (m_ticks % 70) < 50;
        m_highlight[e] = lit ? std::min(1.0, m_highlight[e] + .08) : std::max(0.0, m_highlight[e] - .03);
    }

    // Packets travelling along random edges.
    if (m_pulses.size() < 16 && !m_edges.isEmpty() && rnd() < .35)
        m_pulses << Pulse{int(QRandomGenerator::global()->bounded(int(m_edges.size()))), 0, .015 + rnd() * .03};
    for (auto &pulse : m_pulses)
        pulse.t += pulse.speed;
    m_pulses.removeIf([](const Pulse &pulse) { return pulse.t >= 1; });
}

void GraphItem::paint(QPainter *painter) {
    if (m_position.isEmpty() || width() <= 0 || height() <= 0)
        return;
    painter->setRenderHint(QPainter::Antialiasing, true);
    const double scale = height() / 2 * .9 * m_zoom;
    const QPointF center(width() / 2, height() / 2);
    auto map = [&](int i) { return center + m_position[i] * scale; };

    // Edges, batched per cluster so there are only a few draw calls.
    QVector<QVector<QLineF>> lines(m_hubs.size() + 1);
    for (const auto &edge : std::as_const(m_edges)) {
        const int cluster = m_cluster[edge.first] < 0 ? m_cluster[edge.second] : m_cluster[edge.first];
        lines[cluster < 0 ? m_hubs.size() : cluster] << QLineF(map(edge.first), map(edge.second));
    }
    for (int c = 0; c < lines.size(); ++c) {
        QColor color = c == m_hubs.size() ? m_accent : clusterColor(c);
        color.setAlphaF(c == m_hubs.size() ? .35 : .22);
        painter->setPen(QPen(color, .7));
        painter->drawLines(lines[c]);
    }
    for (int e = 0; e < m_edges.size(); ++e) {
        if (m_highlight[e] <= 0)
            continue;
        QColor color(Qt::white);
        color.setAlphaF(.85 * m_highlight[e]);
        painter->setPen(QPen(color, 1.3));
        painter->drawLine(map(m_edges[e].first), map(m_edges[e].second));
    }

    // Nodes, with a soft glow on the well connected ones.
    painter->setPen(Qt::NoPen);
    for (int i = 0; i < m_position.size(); ++i) {
        const int cluster = m_cluster[i];
        if (cluster < 0)
            continue;
        const bool hub = m_hubs.value(cluster) == i;
        const double radius = hub ? 5.5 : 1.3 + std::sqrt(double(m_degree[i])) * .8;
        QColor color = clusterColor(cluster);
        const QPointF p = map(i);
        if (hub || m_degree[i] > 3) {
            QColor glow = color;
            glow.setAlphaF(.18);
            painter->setBrush(glow);
            painter->drawEllipse(p, radius * 2.6, radius * 2.6);
        }
        if (!hub && m_degree[i] <= 1)
            color.setAlphaF(.75);
        painter->setBrush(color);
        painter->drawEllipse(p, radius, radius);
    }

    // Packets.
    for (const auto &pulse : std::as_const(m_pulses)) {
        const auto &edge = m_edges[pulse.edge];
        const QPointF p = map(edge.first) + (map(edge.second) - map(edge.first)) * pulse.t;
        QColor glow(Qt::white);
        glow.setAlphaF(.25);
        painter->setBrush(glow);
        painter->drawEllipse(p, 4.5, 4.5);
        painter->setBrush(Qt::white);
        painter->drawEllipse(p, 1.8, 1.8);
    }

    // Hub labels as pills in the cluster's color.
    QFont font(m_fontFamily);
    font.setPixelSize(10);
    painter->setFont(font);
    const QFontMetricsF metrics(font);
    for (int c = 0; c < m_hubs.size(); ++c) {
        const QString text = m_clusterNames.value(c).chopped(1);
        const QPointF p = map(m_hubs[c]);
        const double w = metrics.horizontalAdvance(text) + 14, h = metrics.height() + 6;
        const QRectF pill(p.x() - w / 2, p.y() - h - 8, w, h);
        QColor fill = m_surface;
        fill.setAlphaF(.88);
        painter->setBrush(fill);
        painter->setPen(QPen(clusterColor(c), 1));
        painter->drawRoundedRect(pill, h / 2, h / 2);
        painter->setPen(clusterColor(c).lighter(130));
        painter->drawText(pill, Qt::AlignCenter, text);
    }

    // The root: a lettered disc in the accent, its name beneath.
    const QPointF root = center;
    QColor halo = m_accent;
    halo.setAlphaF(.22);
    painter->setPen(Qt::NoPen);
    painter->setBrush(halo);
    painter->drawEllipse(root, 18, 18);
    painter->setBrush(m_accent.darker(150));
    painter->setPen(QPen(m_accent, 1.5));
    painter->drawEllipse(root, 11, 11);
    QFont bold(m_fontFamily);
    bold.setPixelSize(12);
    bold.setBold(true);
    painter->setFont(bold);
    painter->setPen(Qt::white);
    painter->drawText(QRectF(root.x() - 11, root.y() - 11, 22, 22), Qt::AlignCenter, m_rootLetter);
    painter->setFont(font);
    painter->setPen(QColor(255, 255, 255, 200));
    painter->drawText(QRectF(root.x() - 120, root.y() + 16, 240, 16), Qt::AlignHCenter | Qt::AlignTop, m_rootName);
}
