#pragma once

#include <cstddef>
#include <glm/glm.hpp>
#include <vector>

struct Projectile {
  glm::vec3 pos{0.0f};
  glm::vec3 vel{0.0f};
  float life = 0.0f;
};

// Cubes fired by the player: straight motion + expiry.
// Recycles the oldest one past the cap (anti click-spam).
class ProjectileSystem {
public:
  void spawn(const glm::vec2 &playerPos, float yaw);
  void update(float dt);

  [[nodiscard]] const std::vector<Projectile> &list() const noexcept {
    return m_items;
  }
  [[nodiscard]] std::size_t size() const noexcept { return m_items.size(); }

private:
  std::vector<Projectile> m_items;
};
