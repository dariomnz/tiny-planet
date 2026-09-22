#pragma once
#include <glm/glm.hpp>

// Third-person camera: free orbit around the character.
// Z-up world: the ground is the XY plane and up is (0,0,1).
// - yaw: horizontal angle around the target [-pi, pi], 0 = +X.
// - pitch: vertical angle. Negative = high camera looking down.
// - distance: smoothed distance to the target; targetDistance is the
//   goal set by the mouse wheel (smooth zoom via lerp).
struct ThirdPersonCamera {
    glm::vec3 target{0.0f, 0.0f, 1.0f};  // look-at point (head, z=1)

    float yaw = 0.0f;
    float pitch = -0.25f;

    float distance = 7.0f;
    float targetDistance = 7.0f;

    float sensitivity = 0.0025f;
    float scrollSensitivity = 0.12f;  // exp per wheel unit (~0.89 per tick)
    float minDistance = 2.0f;
    float maxDistance = 20.0f;

    float minPitch = -1.2f;      // ~-69 deg
    float maxPitch = 1.2f;       // ~+69 deg

    float smoothFactor = 10.0f;  // zoom lerp speed
    float minHeight = 0.5f;      // the camera never goes below this

    void onMouseMove(double dx, double dy);
    void onScroll(double yoffset);
    void update(float dt, glm::vec2 playerPos);
    glm::mat4 getView() const;
};
