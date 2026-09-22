#include "world/Enemies.h"

#include <cmath>

#include "Config.h"
#include "world/EnemyBullets.h"

Enemy EnemySystem::make(EnemyType type, const glm::vec2 &pos) {
    Enemy e;
    e.pos = pos;
    e.type = type;
    switch (type) {
        case EnemyType::Swarm:
            e.hp = e.maxHp = config::kSwarmHp;
            e.speed = config::kSwarmSpeed;
            e.damage = config::kSwarmDamage;
            e.radius = config::kSwarmRadius;
            break;
        case EnemyType::Shooter:
            e.hp = e.maxHp = config::kShooterHp;
            e.speed = config::kShooterSpeed;
            e.damage = config::kShooterBullet;  // fallback if it ever touches the player
            e.bulletDamage = config::kShooterBullet;
            e.radius = config::kShooterRadius;
            e.fireTimer = config::kShooterFireCd;
            break;
        case EnemyType::Tank:
            e.hp = e.maxHp = config::kTankHp;
            e.speed = config::kTankSpeed;
            e.damage = config::kTankDamage;
            e.radius = config::kTankRadius;
            break;
        case EnemyType::Boss:
            e.hp = e.maxHp = config::kBoss5Hp;
            e.speed = config::kBoss5Speed;
            e.damage = config::kBoss5Contact;
            e.bulletDamage = config::kBoss5Bullet;
            e.radius = config::kBoss5Radius;
            e.fireTimer = config::kBossFanCd;
            e.auxTimer = config::kBossRingCd;
            break;
        case EnemyType::Chaser:
        default:
            e.hp = e.maxHp = config::kChaserHp;
            e.speed = config::kChaserSpeed;
            e.damage = config::kChaserDamage;
            e.radius = config::kChaserRadius;
            break;
    }
    return e;
}

void EnemySystem::clear() { m_count = 0; }

bool EnemySystem::spawn(const Enemy &e) {
    if (m_count >= config::kEnemyCap || m_count >= m_items.size()) return false;
    m_items[m_count++] = e;
    return true;
}

namespace {

// Aimed shot from `from` toward `target` at `speed`.
void fireAimed(EnemyBulletSystem &bullets, const glm::vec2 &from, const glm::vec2 &target, float speed,
               float damage) {
    const glm::vec2 to = target - from;
    const float d2 = glm::dot(to, to);
    if (d2 < 1e-8f) return;
    const glm::vec2 dir = to / std::sqrt(d2);
    bullets.spawn(from, dir * speed, damage);
}

bool heavy(EnemyType t) { return t == EnemyType::Tank || t == EnemyType::Boss; }

}  // namespace

void EnemySystem::update(float dt, const glm::vec2 &playerPos, EnemyBulletSystem &bullets, float timeMin) {
    const float bSpeed = config::bulletSpeed(timeMin);

    for (std::size_t i = 0; i < m_count; ++i) {
        Enemy &e = m_items[i];
        const glm::vec2 to = playerPos - e.pos;
        const float d2 = glm::dot(to, to);
        const float d = std::sqrt(d2 > 1e-8f ? d2 : 1e-8f);
        const glm::vec2 dir = to / d;

        switch (e.type) {
            case EnemyType::Shooter: {
                // Keeps ~12u distance: approach beyond 14u, retreat inside 10u,
                // slow orbit in between so shooters don't stack in a line.
                if (d > config::kShooterPreferDist + 2.0f) {
                    e.pos += dir * (e.speed * dt);
                } else if (d < config::kShooterPreferDist - 2.0f) {
                    e.pos -= dir * (e.speed * dt);
                } else {
                    const glm::vec2 perp(-dir.y, dir.x);
                    e.pos += perp * (e.speed * 0.4f * dt);
                }
                // 1 aimed bullet, slow (04). Holds fire off-screen / pool full.
                e.fireTimer -= dt;
                if (e.fireTimer <= 0.0f) {
                    e.fireTimer = config::kShooterFireCd;
                    if (d < config::kShooterFireRange && !bullets.full())
                        fireAimed(bullets, e.pos, playerPos, bSpeed, e.bulletDamage);
                }
                break;
            }
            case EnemyType::Boss: {
                // Slow chase + two patterns: aimed fan + full ring (04).
                e.pos += dir * (e.speed * dt);
                e.fireTimer -= dt;
                if (e.fireTimer <= 0.0f) {
                    e.fireTimer = config::kBossFanCd;
                    if (!bullets.full()) {
                        // +1 bullet in fans every 3 min (04 pattern extras).
                        int n = config::kBossFanCount + static_cast<int>(timeMin / 3.0f);
                        if (n > 9) n = 9;
                        const float base = std::atan2(dir.y, dir.x);
                        for (int k = 0; k < n; ++k) {
                            const float a = base + (k - (n - 1) * 0.5f) * config::kBossFanSpread;
                            bullets.spawn(e.pos, glm::vec2(std::cos(a), std::sin(a)) * bSpeed,
                                          e.bulletDamage);
                        }
                    }
                }
                e.auxTimer -= dt;
                if (e.auxTimer <= 0.0f) {
                    e.auxTimer = config::kBossRingCd;
                    if (!bullets.full()) {
                        for (int k = 0; k < config::kBossRingCount; ++k) {
                            const float a = 6.2831853f * k / config::kBossRingCount;
                            bullets.spawn(e.pos, glm::vec2(std::cos(a), std::sin(a)) * bSpeed,
                                          e.bulletDamage);
                        }
                    }
                }
                break;
            }
            case EnemyType::Chaser:
            case EnemyType::Swarm:
            case EnemyType::Tank:
            default:
                e.pos += dir * (e.speed * dt);
                break;
        }
    }

    // Separation: overlapping enemies push apart so chasers/swarms don't
    // stack into one blob (O(n^2), fine for n <= 256). Tanks/bosses push
    // through: only the other side moves.
    for (std::size_t i = 0; i < m_count; ++i) {
        for (std::size_t j = i + 1; j < m_count; ++j) {
            Enemy &a = m_items[i];
            Enemy &b = m_items[j];
            const glm::vec2 d = b.pos - a.pos;
            const float rr = a.radius + b.radius;
            const float dist2 = glm::dot(d, d);
            if (dist2 >= rr * rr || dist2 < 1e-8f) continue;
            const float dist = std::sqrt(dist2);
            const glm::vec2 push = (d / dist) * ((rr - dist) * 0.5f);
            const bool aHeavy = heavy(a.type);
            const bool bHeavy = heavy(b.type);
            if (!aHeavy && !bHeavy) {
                a.pos -= push;
                b.pos += push;
            } else if (aHeavy && !bHeavy) {
                b.pos += push * 2.0f;
            } else if (!aHeavy && bHeavy) {
                a.pos -= push * 2.0f;
            }
        }
    }
}

void EnemySystem::killAt(std::size_t i) {
    if (i >= m_count) return;
    m_items[i] = m_items[--m_count];
}
