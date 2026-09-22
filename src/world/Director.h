#pragma once

#include <glm/glm.hpp>

class EnemySystem;

// M3 Director (docs/game-plan/04-enemies-scaling.md):
// keeps targetAlive(t) enemies alive on a 25-35u ring, time-gated roster
// (chasers -> +swarms -> +shooters -> +tanks), Boss-5 at min 5.
// During the boss, normal spawns run at 30%. Zero per-frame allocs.
class Director {
   public:
    void clear();
    // timeSec = run clock. Spawns into `enemies` around `playerPos`.
    void update(float dt, float timeSec, const glm::vec2 &playerPos, EnemySystem &enemies);

   private:
    float rand01() noexcept;  // deterministic xorshift, reset in clear()

    float m_spawnTimer = 0.0f;
    bool m_bossSpawned = false;
    unsigned m_rng = 0x9E3779B9u;
};
