#pragma once

#include <glm/glm.hpp>

// Player: XY position + yaw smoothly turning towards the camera.
class Player {
public:
  void addDisplacement(const glm::vec2 &d) { m_pos += d; }
  void updateYaw(float camYaw, float dt);

  [[nodiscard]] const glm::vec2 &pos() const noexcept { return m_pos; }
  [[nodiscard]] float yaw() const noexcept { return m_yaw; }

private:
  glm::vec2 m_pos{0.0f, 0.0f};
  float m_yaw = 0.0f;
};
