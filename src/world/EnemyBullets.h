#pragma once

#include <array>
#include <cstddef>
#include <glm/glm.hpp>

struct EnemyBullet {
    glm::vec3 pos{0.0f};
    glm::vec3 vel{0.0f};
    float life = 0.0f;
    float damage = 0.0f;
};

// M3: enemy bullets (shooter aimed, boss fan/ring). Fixed pool, zero
// per-frame allocs. Active bullets are m_items[0, m_count). Removal is
// swap-remove. When the pool is full, spawn() skips AND enemies hold fire
// (see 04-enemies-scaling.md: stop firing before the frame can blow up).
class EnemyBulletSystem {
   public:
    void clear();
    void spawn(const glm::vec2 &pos, const glm::vec2 &vel, float damage);
    void update(float dt);
    void killAt(std::size_t i);  // swap-remove, order not preserved

    [[nodiscard]] const EnemyBullet *data() const noexcept { return m_items.data(); }
    [[nodiscard]] EnemyBullet *data() noexcept { return m_items.data(); }
    [[nodiscard]] std::size_t size() const noexcept { return m_count; }
    [[nodiscard]] bool full() const noexcept { return m_count >= m_items.size(); }

   private:
    std::array<EnemyBullet, 400> m_items{};
    std::size_t m_count = 0;
};
