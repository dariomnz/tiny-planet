#include "world/Director.h"

#include <cmath>

#include "Config.h"
#include "world/Enemies.h"

void Director::clear() {
    m_spawnTimer = 0.0f;
    m_bossSpawned = false;
    m_rng = 0x9E3779B9u;
}

float Director::rand01() noexcept {
    m_rng ^= m_rng << 13;
    m_rng ^= m_rng >> 17;
    m_rng ^= m_rng << 5;
    return static_cast<float>(m_rng >> 8) * (1.0f / 16777216.0f);
}

void Director::update(float dt, float timeSec, const glm::vec2 &playerPos, EnemySystem &enemies) {
    const float t = timeSec / 60.0f;  // run minutes

    // Boss-5 schedule: single spawn at min 5, closer ring so it arrives fast.
    if (!m_bossSpawned && timeSec >= config::kBoss5SpawnMin * 60.0f) {
        m_bossSpawned = true;
        const float ang = rand01() * 6.2831853f;
        enemies.spawn(EnemySystem::make(
            EnemyType::Boss,
            glm::vec2(playerPos.x + std::cos(ang) * config::kBoss5SpawnDist,
                      playerPos.y + std::sin(ang) * config::kBoss5SpawnDist)));
    }

    // Boss alive? Normal spawns run at 30% (interval x3.33).
    bool bossAlive = false;
    for (std::size_t i = 0; i < enemies.size(); ++i) {
        if (enemies.data()[i].type == EnemyType::Boss) {
            bossAlive = true;
            break;
        }
    }

    float interval = config::spawnInterval(t);
    if (bossAlive) interval /= 0.3f;
    m_spawnTimer -= dt;
    if (m_spawnTimer > 0.0f) return;
    m_spawnTimer = interval;

    const int target = config::targetAlive(t);
    if (static_cast<int>(enemies.size()) >= target) return;

    // Time-gated roster: chasers always; swarms from min 1; shooters from
    // min 2; tanks from min 3.
    const float r = rand01();
    EnemyType type = EnemyType::Chaser;
    if (t >= 3.0f) {
        if (r < 0.40f)
            type = EnemyType::Chaser;
        else if (r < 0.60f)
            type = EnemyType::Swarm;
        else if (r < 0.85f)
            type = EnemyType::Shooter;
        else
            type = EnemyType::Tank;
    } else if (t >= 2.0f) {
        if (r < 0.50f)
            type = EnemyType::Chaser;
        else if (r < 0.75f)
            type = EnemyType::Swarm;
        else
            type = EnemyType::Shooter;
    } else if (t >= 1.0f) {
        type = (r < 0.70f) ? EnemyType::Chaser : EnemyType::Swarm;
    }

    // Swarms arrive as a group of 8-12 at one ring point (04).
    int count = 1;
    if (type == EnemyType::Swarm)
        count = config::kSwarmGroupMin +
                static_cast<int>(rand01() * (config::kSwarmGroupMax - config::kSwarmGroupMin + 1));

    const float baseAng = rand01() * 6.2831853f;
    for (int k = 0; k < count; ++k) {
        if (static_cast<int>(enemies.size()) >= target) return;
        const float rr =
            config::kSpawnRingMin + rand01() * (config::kSpawnRingMax - config::kSpawnRingMin);
        glm::vec2 p(playerPos.x + std::cos(baseAng) * rr, playerPos.y + std::sin(baseAng) * rr);
        if (type == EnemyType::Swarm) p += glm::vec2((rand01() - 0.5f) * 4.0f, (rand01() - 0.5f) * 4.0f);
        if (!enemies.spawn(EnemySystem::make(type, p))) return;  // pool full: skip
    }
}
