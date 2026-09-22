#pragma once

#include <array>
#include <cstddef>
#include <glm/glm.hpp>

struct Gem {
    glm::vec2 pos{0.0f, 0.0f};
    float life = 0.0f;  // counts down from kGemDespawnSec
    int value = 1;
};

// M2: XP gems. Fixed pool, zero per-frame allocs.
// Active gems are m_items[0, m_count). Removal is swap-remove.
// When the pool is full, spawn() skips (same rule as bullets/enemies).
// Magnet: gems inside kGemMagnetRadius fly toward the player;
// out-of-range gems despawn after kGemDespawnSec.
class GemSystem {
   public:
    void clear();
    void spawn(const glm::vec2 &pos, int value);
    void update(float dt, const glm::vec2 &playerPos);
    void collectAt(std::size_t i);  // swap-remove, order not preserved

    [[nodiscard]] const Gem *data() const noexcept { return m_items.data(); }
    [[nodiscard]] std::size_t size() const noexcept { return m_count; }

   private:
    std::array<Gem, 300> m_items{};
    std::size_t m_count = 0;
};
