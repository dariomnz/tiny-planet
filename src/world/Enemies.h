#pragma once

#include <array>
#include <cstddef>
#include <glm/glm.hpp>

struct Enemy {
    glm::vec2 pos{0.0f, 0.0f};
    float hp = 0.0f;
    float speed = 0.0f;
    float damage = 0.0f;
    float radius = 0.5f;
};

// M1: single Chaser type. Fixed pool, zero per-frame allocs.
// Active enemies are m_items[0, m_count). Removal is swap-remove.
// Spawning is a minimal inline director (M3 adds the real Director):
// keep kChaserTargetAlive alive, one spawn per kChaserSpawnInterval
// on a ring around the player.
class EnemySystem {
   public:
    void clear();
    void update(float dt, const glm::vec2 &playerPos);
    void killAt(std::size_t i);  // swap-remove, order not preserved

    [[nodiscard]] const Enemy *data() const noexcept { return m_items.data(); }
    [[nodiscard]] Enemy *data() noexcept { return m_items.data(); }
    [[nodiscard]] std::size_t size() const noexcept { return m_count; }

   private:
    void trySpawn(float dt, const glm::vec2 &playerPos);
    static float rand01() noexcept;  // deterministic xorshift, no allocs

    std::array<Enemy, 256> m_items{};
    std::size_t m_count = 0;
    float m_spawnTimer = 0.0f;
};
