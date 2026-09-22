#include "world/Projectiles.h"

#include <cmath>

#include "Config.h"

void ProjectileSystem::spawn(const glm::vec2 &playerPos, float yaw, float damage) {
    if (m_count >= m_items.size()) return;  // pool full: skip (no allocs, no blowup)
    const glm::vec3 dir(std::cos(yaw), std::sin(yaw), 0.0f);
    Projectile p;
    p.pos = glm::vec3(playerPos.x + dir.x * config::kProjForwardOffset,
                      playerPos.y + dir.y * config::kProjForwardOffset, config::kProjSpawnZ);
    p.vel = dir * config::kProjSpeed;
    p.life = config::kProjLife;
    p.damage = damage;
    m_items[m_count++] = p;
}

void ProjectileSystem::update(float dt) {
    for (std::size_t i = 0; i < m_count;) {
        Projectile &p = m_items[i];
        p.pos += p.vel * dt;
        p.life -= dt;
        if (p.life <= 0.0f) {
            killAt(i);  // no ++i: swapped-in item still needs update
        } else {
            ++i;
        }
    }
}

void ProjectileSystem::killAt(std::size_t i) {
    if (i >= m_count) return;
    m_items[i] = m_items[--m_count];
}
