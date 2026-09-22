#include "world/Player.h"

#include <cmath>

void Player::updateYaw(float camYaw, float dt) {
    const float diff = std::atan2(std::sin(camYaw - m_yaw), std::cos(camYaw - m_yaw));
    const float t = 1.0f - std::exp(-10.0f * dt);
    m_yaw += diff * t;
}
