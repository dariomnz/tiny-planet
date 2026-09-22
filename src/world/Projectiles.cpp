#include "world/Projectiles.h"

#include <cmath>

#include "Config.h"

void ProjectileSystem::spawn(const glm::vec2 &playerPos, float yaw) {
    const glm::vec3 dir(std::cos(yaw), std::sin(yaw), 0.0f);
    Projectile p;
    p.pos = glm::vec3(playerPos.x + dir.x * config::kProjForwardOffset,
                      playerPos.y + dir.y * config::kProjForwardOffset, config::kProjSpawnZ);
    p.vel = dir * config::kProjSpeed;
    p.life = config::kProjLife;
    if (m_items.size() >= config::kProjMax) m_items.erase(m_items.begin());
    m_items.push_back(p);
}

void ProjectileSystem::update(float dt) {
    for (std::size_t i = 0; i < m_items.size();) {
        Projectile &p = m_items[i];
        p.pos += p.vel * dt;
        p.life -= dt;
        if (p.life <= 0.0f) {
            p = m_items.back();
            m_items.pop_back();
        } else {
            ++i;
        }
    }
}
