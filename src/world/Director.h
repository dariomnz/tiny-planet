#pragma once

#include <glm/glm.hpp>

class EnemySystem;

// M3/M5 Director (docs/game-plan/04-enemies-scaling.md):
// keeps targetAlive(t) enemies alive on a 25-35u ring, time-gated roster
// (chasers -> +swarms -> +shooters -> +tanks -> +spinners), Boss-5/10/15 at
// min 5/10/15. Elites (x6 HP) from min 5 at 5%. During a boss, normal spawns
// run at 30%. Zero per-frame allocs.
class Director {
   public:
    void clear();
    // timeSec = run clock. ngHp/ngDmg = NG+ mults (1.5^n / 1.2^n); time
    // scaling hpMult(t)/dmgMult(t) is applied inside for normal spawns
    // (bosses take NG+ only — their tier HP is fixed).
    void update(float dt, float timeSec, const glm::vec2 &playerPos, EnemySystem &enemies, float ngHp,
                float ngDmg);

   private:
    float rand01() noexcept;  // deterministic xorshift, reset in clear()

    float m_spawnTimer = 0.0f;
    int m_bossesSpawned = 0;  // tiers spawned so far (max 3)
    unsigned m_rng = 0x9E3779B9u;
};
