#include "world/Director.h"

#include <cmath>

#include "Config.h"
#include "world/Enemies.h"

void Director::clear() {
    m_spawnTimer = 0.0f;
    m_bossesSpawned = 0;
    m_rng = 0x9E3779B9u;
}

float Director::rand01() noexcept {
    m_rng ^= m_rng << 13;
    m_rng ^= m_rng >> 17;
    m_rng ^= m_rng << 5;
    return static_cast<float>(m_rng >> 8) * (1.0f / 16777216.0f);
}

void Director::update(float dt, float timeSec, const glm::vec2 &playerPos, EnemySystem &enemies, float ngHp,
                      float ngDmg) {
    const float t = timeSec / 60.0f;  // run minutes

    // Boss schedule: tiers 5/10/15, closer ring so they arrive fast.
    // m_bossesSpawned doubles as the tier index (0/1/2).
    while (m_bossesSpawned < 3 && timeSec >= config::kBossSpawnMin[m_bossesSpawned] * 60.0f) {
        const int tierIdx = m_bossesSpawned++;
        const int tier = tierIdx == 0 ? 5 : (tierIdx == 1 ? 10 : 15);
        const float ang = rand01() * 6.2831853f;
        enemies.spawn(EnemySystem::make(
            EnemyType::Boss,
            glm::vec2(playerPos.x + std::cos(ang) * config::kBossSpawnDist,
                      playerPos.y + std::sin(ang) * config::kBossSpawnDist),
            ngHp, ngDmg * config::dmgMult(t), false, tier));
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

    // M5 tuning: time scaling on every normal spawn (04 formulas).
    const float hpM = config::hpMult(t) * ngHp;
    const float dmgM = config::dmgMult(t) * ngDmg;

    // Time-gated roster: chasers always; swarms from min 1; shooters from
    // min 2; tanks from min 3; spinners from min 5.
    const float r = rand01();
    EnemyType type = EnemyType::Chaser;
    if (t >= 5.0f) {
        if (r < 0.35f)
            type = EnemyType::Chaser;
        else if (r < 0.55f)
            type = EnemyType::Swarm;
        else if (r < 0.75f)
            type = EnemyType::Shooter;
        else if (r < 0.85f)
            type = EnemyType::Tank;
        else
            type = EnemyType::Spinner;
    } else if (t >= 3.0f) {
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

    // Elites: 5% of single spawns from min 5 (04). Swarm groups stay normal
    // so the group math (8-12 x1HP) is untouched.
    bool elite = false;
    if (t >= config::kEliteMin && type != EnemyType::Swarm && rand01() < config::kEliteChance) elite = true;

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
        if (!enemies.spawn(EnemySystem::make(type, p, hpM, dmgM, elite))) return;  // pool full: skip
    }
}
