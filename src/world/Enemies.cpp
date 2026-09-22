#include "world/Enemies.h"

#include <cmath>

#include "Config.h"

void EnemySystem::clear() {
    m_count = 0;
    m_spawnTimer = 0.0f;
}

float EnemySystem::rand01() noexcept {
    // xorshift32 with fixed seed: deterministic, no <random> / no allocs.
    static unsigned s = 0x9E3779B9u;
    s ^= s << 13;
    s ^= s >> 17;
    s ^= s << 5;
    return static_cast<float>(s >> 8) * (1.0f / 16777216.0f);
}

void EnemySystem::trySpawn(float dt, const glm::vec2 &playerPos) {
    m_spawnTimer -= dt;
    if (m_spawnTimer > 0.0f) return;
    m_spawnTimer = config::kChaserSpawnInterval;
    if (m_count >= config::kEnemyCap || m_count >= m_items.size()) return;
    if (static_cast<int>(m_count) >= config::kChaserTargetAlive) return;

    // Ring spawn around the player (off-screen when possible at this range).
    const float ang = rand01() * 6.2831853f;
    const float r =
        config::kChaserSpawnRingMin + rand01() * (config::kChaserSpawnRingMax - config::kChaserSpawnRingMin);
    Enemy e;
    e.pos = glm::vec2(playerPos.x + std::cos(ang) * r, playerPos.y + std::sin(ang) * r);
    e.hp = config::kChaserHp;
    e.speed = config::kChaserSpeed;
    e.damage = config::kChaserDamage;
    e.radius = config::kChaserRadius;
    m_items[m_count++] = e;
}

void EnemySystem::update(float dt, const glm::vec2 &playerPos) {
    trySpawn(dt, playerPos);
    for (std::size_t i = 0; i < m_count; ++i) {
        Enemy &e = m_items[i];
        const glm::vec2 to = playerPos - e.pos;
        const float d2 = glm::dot(to, to);
        if (d2 > 1e-8f) {
            const float d = std::sqrt(d2);
            e.pos += (to / d) * (e.speed * dt);
        }
    }
}

void EnemySystem::killAt(std::size_t i) {
    if (i >= m_count) return;
    m_items[i] = m_items[--m_count];
}
