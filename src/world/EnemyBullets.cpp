#include "world/EnemyBullets.h"

#include <cmath>

#include "Config.h"

void EnemyBulletSystem::clear() { m_count = 0; }

EnemyBulletSystem::EnemyBulletSystem() { m_items.resize(config::kEnemyBulletCap); }

void EnemyBulletSystem::spawn(const glm::vec2 &pos, const glm::vec2 &vel, float damage, bool homing) {
    if (m_count >= config::kEnemyBulletCap || m_count >= m_items.size()) return;  // pool full: skip
    EnemyBullet b;
    b.pos = glm::vec3(pos.x, pos.y, config::kEnemyBulletSpawnZ);
    b.vel = glm::vec3(vel.x, vel.y, 0.0f);
    b.life = config::kEnemyBulletLife;
    b.damage = damage;
    b.homing = homing;
    b.speed = std::sqrt(vel.x * vel.x + vel.y * vel.y);
    m_items[m_count++] = b;
}

void EnemyBulletSystem::update(float dt, const glm::vec2 &playerPos) {
    for (std::size_t i = 0; i < m_count;) {
        EnemyBullet &b = m_items[i];
        if (b.homing && b.speed > 1e-4f) {
            // M5: limited-turn steering toward the player (Boss-15 missiles).
            const glm::vec2 to(playerPos.x - b.pos.x, playerPos.y - b.pos.y);
            const float d2 = glm::dot(to, to);
            if (d2 > 1e-8f) {
                const float cur = std::atan2(b.vel.y, b.vel.x);
                const float want = std::atan2(to.y, to.x);
                float diff = want - cur;
                while (diff > 3.14159265f) diff -= 6.2831853f;
                while (diff < -3.14159265f) diff += 6.2831853f;
                const float maxTurn = config::kBossMissileTurn * dt;
                const float turn = diff < -maxTurn ? -maxTurn : (diff > maxTurn ? maxTurn : diff);
                const float a = cur + turn;
                b.vel = glm::vec3(std::cos(a) * b.speed, std::sin(a) * b.speed, 0.0f);
            }
        }
        b.pos += b.vel * dt;
        b.life -= dt;
        if (b.life <= 0.0f) {
            killAt(i);  // no ++i: swapped-in bullet still needs update
        } else {
            ++i;
        }
    }
}

void EnemyBulletSystem::killAt(std::size_t i) {
    if (i >= m_count) return;
    m_items[i] = m_items[--m_count];
}
