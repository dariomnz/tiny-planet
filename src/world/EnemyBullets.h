#pragma once

#include <cstddef>
#include <glm/glm.hpp>
#include <vector>

#include "Config.h"

struct EnemyBullet {
    glm::vec3 pos{0.0f};
    glm::vec3 vel{0.0f};
    float life = 0.0f;
    float damage = 0.0f;
    bool homing = false;  // M5: Boss-15 slow missiles steer toward the player
    float speed = 0.0f;   // cruise speed for homing steering
};

// M3: enemy bullets (shooter aimed, boss fan/ring). Fixed pool, zero
// per-frame allocs. Active bullets are m_items[0, m_count). Removal is
// swap-remove. When the pool is full, spawn() skips AND enemies hold fire
// (see 04-enemies-scaling.md: stop firing before the frame can blow up).
class EnemyBulletSystem {
   public:
    EnemyBulletSystem();  // heap-backed pool: sized once to kEnemyBulletCap, no per-frame allocs
    void clear();
    void spawn(const glm::vec2 &pos, const glm::vec2 &vel, float damage, bool homing = false);
    void update(float dt, const glm::vec2 &playerPos);
    void killAt(std::size_t i);  // swap-remove, order not preserved

    [[nodiscard]] const EnemyBullet *data() const noexcept { return m_items.data(); }
    [[nodiscard]] EnemyBullet *data() noexcept { return m_items.data(); }
    [[nodiscard]] std::size_t size() const noexcept { return m_count; }
    [[nodiscard]] bool full() const noexcept { return m_count >= m_items.size(); }

   private:
    std::vector<EnemyBullet> m_items{};
    std::size_t m_count = 0;
};
