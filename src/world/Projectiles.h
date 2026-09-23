#pragma once

#include <array>
#include <cstddef>
#include <glm/glm.hpp>

#include "Config.h"

struct Projectile {
    glm::vec3 pos{0.0f};
    glm::vec3 vel{0.0f};
    float life = 0.0f;
    float damage = 0.0f;
    bool homing = false;  // M5: auto-missiles steer toward the nearest enemy
};

// Player bullets: fixed pool, zero per-frame allocs.
// Active bullets are items[0, count). Removal is swap-remove.
// When the pool is full, spawn() skips (see 04-enemies-scaling.md).
class ProjectileSystem {
    public:
     void spawn(const glm::vec2 &playerPos, float yaw, float damage);
     void spawnAt(const glm::vec3 &pos, const glm::vec3 &vel, float damage);  // M2: nova ring
     void update(float dt);
     void killAt(std::size_t i);  // swap-remove, order not preserved

     [[nodiscard]] const Projectile *data() const noexcept { return m_items.data(); }
     [[nodiscard]] Projectile *data() noexcept { return m_items.data(); }
     [[nodiscard]] std::size_t size() const noexcept { return m_count; }
     [[nodiscard]] bool full() const noexcept { return m_count >= m_items.size(); }

    private:
     std::array<Projectile, config::kProjMax> m_items{};
     std::size_t m_count = 0;
};
