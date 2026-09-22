#include "world/EnemyBullets.h"

#include "Config.h"

void EnemyBulletSystem::clear() { m_count = 0; }

void EnemyBulletSystem::spawn(const glm::vec2 &pos, const glm::vec2 &vel, float damage) {
    if (m_count >= config::kEnemyBulletCap || m_count >= m_items.size()) return;  // pool full: skip
    EnemyBullet b;
    b.pos = glm::vec3(pos.x, pos.y, config::kEnemyBulletSpawnZ);
    b.vel = glm::vec3(vel.x, vel.y, 0.0f);
    b.life = config::kEnemyBulletLife;
    b.damage = damage;
    m_items[m_count++] = b;
}

void EnemyBulletSystem::update(float dt) {
    for (std::size_t i = 0; i < m_count;) {
        EnemyBullet &b = m_items[i];
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
