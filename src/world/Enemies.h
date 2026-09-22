#pragma once

#include <array>
#include <cstddef>
#include <glm/glm.hpp>

class EnemyBulletSystem;

// M3 enemy types (docs/game-plan/04-enemies-scaling.md table).
// All share the cube + curvature shader; only stats/AI/color differ.
enum class EnemyType { Chaser, Swarm, Shooter, Tank, Boss };

struct Enemy {
    glm::vec2 pos{0.0f, 0.0f};
    float hp = 0.0f;
    float maxHp = 0.0f;
    float speed = 0.0f;
    float damage = 0.0f;        // contact damage
    float bulletDamage = 0.0f;  // shot damage (Shooter/Boss)
    float radius = 0.5f;
    EnemyType type = EnemyType::Chaser;
    float fireTimer = 0.0f;  // shooter aimed cadence / boss fan
    float auxTimer = 0.0f;   // boss ring
};

// Fixed pool, zero per-frame allocs. Active enemies are m_items[0, m_count).
// Removal is swap-remove. Spawning belongs to Director (M3); this system owns
// movement AI, firing patterns and separation.
class EnemySystem {
   public:
    static Enemy make(EnemyType type, const glm::vec2 &pos);

    void clear();
    // timeMin = run minutes (float) for bulletSpeed(t) scaling.
    void update(float dt, const glm::vec2 &playerPos, EnemyBulletSystem &bullets, float timeMin);
    bool spawn(const Enemy &e);  // false when the pool is full
    void killAt(std::size_t i);  // swap-remove, order not preserved

    [[nodiscard]] const Enemy *data() const noexcept { return m_items.data(); }
    [[nodiscard]] Enemy *data() noexcept { return m_items.data(); }
    [[nodiscard]] std::size_t size() const noexcept { return m_count; }

   private:
    std::array<Enemy, 256> m_items{};
    std::size_t m_count = 0;
};
