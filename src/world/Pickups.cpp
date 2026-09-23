#include "world/Pickups.h"

#include <cmath>

#include "Config.h"

void GemSystem::clear() { m_count = 0; }

GemSystem::GemSystem() { m_items.resize(config::kGemCap); }

void GemSystem::spawn(const glm::vec2 &pos, int value) {
    if (m_count >= config::kGemCap || m_count >= m_items.size()) return;  // pool full: skip
    Gem g;
    g.pos = pos;
    g.life = config::kGemDespawnSec;
    g.value = value;
    m_items[m_count++] = g;
}

void GemSystem::update(float dt, const glm::vec2 &playerPos, float magnetRadius) {
    for (std::size_t i = 0; i < m_count;) {
        Gem &g = m_items[i];
        const glm::vec2 to = playerPos - g.pos;
        const float d2 = glm::dot(to, to);
        if (d2 < magnetRadius * magnetRadius && d2 > 1e-8f) {
            const float d = std::sqrt(d2);
            g.pos += (to / d) * (config::kGemMagnetSpeed * dt);
        }
        g.life -= dt;
        if (g.life <= 0.0f) {
            collectAt(i);  // despawn; swapped-in gem still needs update
        } else {
            ++i;
        }
    }
}

void GemSystem::collectAt(std::size_t i) {
    if (i >= m_count) return;
    m_items[i] = m_items[--m_count];
}
