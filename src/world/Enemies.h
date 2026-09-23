#pragma once

#include <array>
#include <cstddef>
#include <glm/glm.hpp>

#include "Config.h"

class EnemyBulletSystem;

// M3/M5 enemy types (docs/game-plan/04-enemies-scaling.md table).
// All share the cube + curvature shader; only stats/AI/color differ.
enum class EnemyType { Chaser, Swarm, Shooter, Tank, Spinner, Boss };

struct Enemy {
    glm::vec2 pos{0.0f, 0.0f};
    float hp = 0.0f;
    float maxHp = 0.0f;
    float speed = 0.0f;
    float damage = 0.0f;        // contact damage
    float bulletDamage = 0.0f;  // shot damage (Shooter/Spinner/Boss)
    float radius = 0.5f;
    EnemyType type = EnemyType::Chaser;
    bool elite = false;   // M5: x6 HP, x5 XP, +2 fragments (04)
    int bossTier = 0;     // M5: 0 = not a boss, else 5/10/15
    float fireTimer = 0.0f;  // shooter aimed cadence / boss fan / spinner volley
    float auxTimer = 0.0f;   // boss ring / boss missiles
    float spiral = 0.0f;     // spinner + Boss-15 spiral arm angle
    float spiral2 = 0.0f;    // Boss-15 counter-rotating spiral angle
};

// Fixed pool, zero per-frame allocs. Active enemies are m_items[0, m_count).
// Removal is swap-remove. Spawning belongs to Director; this system owns
// movement AI, firing patterns and separation.
class EnemySystem {
   public:
    // hpMult/dmgMult are precombined by the Director: time scaling
    // (hpMult(t)/dmgMult(t)) x NG+ mults for normals; NG+ only for bosses.
    static Enemy make(EnemyType type, const glm::vec2 &pos, float hpMult = 1.0f, float dmgMult = 1.0f,
                      bool elite = false, int bossTier = 0);

    void clear();
    // timeMin = run minutes (float) for bulletSpeed(t) scaling.
    void update(float dt, const glm::vec2 &playerPos, EnemyBulletSystem &bullets, float timeMin);
    bool spawn(const Enemy &e);  // false when the pool is full
    void killAt(std::size_t i);  // swap-remove, order not preserved

    [[nodiscard]] const Enemy *data() const noexcept { return m_items.data(); }
    [[nodiscard]] Enemy *data() noexcept { return m_items.data(); }
    [[nodiscard]] std::size_t size() const noexcept { return m_count; }

   private:
    std::array<Enemy, config::kEnemyCap> m_items{};
    std::size_t m_count = 0;
};
