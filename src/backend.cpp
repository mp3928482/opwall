#include "backend.h"

#include "generator.h"

#include <QRandomGenerator>

Backend::Backend(quint32 seed, QObject *parent) : QObject(parent) {
    generate(seed ? seed : QRandomGenerator::global()->bounded(1u, 999999u));
}

void Backend::setPaused(bool paused) {
    if (paused == m_paused)
        return;
    m_paused = paused;
    emit pausedChanged();
}

void Backend::reroll() {
    quint32 seed;
    do seed = QRandomGenerator::global()->bounded(1u, 999999u); while (seed == m_seed);
    generate(seed);
}

void Backend::generate(quint32 seed) {
    m_seed = seed;
    m_round = 0;
    Generator generator(seed);
    m_header = generator.header();
    m_panels = generator.panels();
    m_graph = generator.graph();
    m_canvas = generator.canvas();
    m_registry = generator.registry();
    m_doc = generator.doc();
    m_radial = generator.radial();
    m_timeline = generator.timeline();
    m_ticker = generator.ticker();
    m_code = generator.code();
    m_writer = generator.writer();
    emit rerolled();
    emit codeChanged();
    emit writerChanged();
}

// Follow-up snippets stay reproducible: they come from the wall's seed and
// how many have been shown, not from the clock.
void Backend::nextCode() {
    Generator generator(m_seed * 7919u + ++m_round);
    m_code = generator.code();
    emit codeChanged();
}

void Backend::nextWriter() {
    Generator generator(m_seed * 104729u + ++m_round);
    m_writer = generator.writer();
    emit writerChanged();
}
