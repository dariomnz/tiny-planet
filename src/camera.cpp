#include "camera.h"
#include <algorithm>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

namespace {
constexpr float PI = 3.14159265358979323846f;
constexpr float TWO_PI = 2.0f * PI;
} // namespace

void ThirdPersonCamera::onMouseMove(double dx, double dy) {
  yaw -= static_cast<float>(dx) * sensitivity;
  pitch -= static_cast<float>(dy) * sensitivity;

  // Wrap yaw to [-pi, pi] to avoid precision loss.
  yaw = std::fmod(yaw + PI, TWO_PI);
  if (yaw < 0.0f)
    yaw += TWO_PI;
  yaw -= PI;

  pitch = std::clamp(pitch, minPitch, maxPitch);
}

void ThirdPersonCamera::onScroll(double yoffset) {
  // Exponential zoom: feels uniform near and far.
  // Wheel up (yoffset>0) = zoom in, wheel down = zoom out.
  // On web deltas vary a lot (fractional trackpad vs. large flicks):
  // clamp per event to avoid sudden jumps.
  float d = std::clamp(static_cast<float>(yoffset), -4.0f, 4.0f);
  if (std::fabs(d) < 1e-3f)
    return;
  targetDistance *= std::exp(-d * scrollSensitivity);
  targetDistance = std::clamp(targetDistance, minDistance, maxDistance);
}

void ThirdPersonCamera::update(float dt, glm::vec2 playerPos) {
  target = glm::vec3(playerPos.x, playerPos.y, 1.0f);
  // Framerate-independent exponential smoothing.
  float t = 1.0f - std::exp(-smoothFactor * dt);
  distance += (targetDistance - distance) * t;
}

glm::mat4 ThirdPersonCamera::getView() const {
  glm::vec3 offset;
  offset.x = distance * std::cos(pitch) * std::cos(yaw);
  offset.y = distance * std::cos(pitch) * std::sin(yaw);
  offset.z = distance * std::sin(pitch);

  // Behind where it looks.
  glm::vec3 camPos = target - offset;

  // Ground collision preserving direction:
  // move the camera towards the target instead of clipping it on a plane.
  if (camPos.z < minHeight) {
    float denom = camPos.z - target.z;
    if (std::fabs(denom) > 1e-6f) {
      float t = (minHeight - target.z) / denom;
      t = std::clamp(t, 0.0f, 1.0f);
      camPos = target + (camPos - target) * t;
    } else {
      camPos.z = minHeight;
    }
  }

  return glm::lookAt(camPos, target, glm::vec3(0.0f, 0.0f, 1.0f));
}
