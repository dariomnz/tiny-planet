#pragma once

// Mouse/keyboard state via GLFW callbacks.
// No <emscripten/*>: Pointer Lock sync is done by WebExt.

struct GLFWwindow;
struct ThirdPersonCamera;

class InputManager {
public:
  InputManager(GLFWwindow *window, ThirdPersonCamera &camera);
  ~InputManager();

  InputManager(const InputManager &) = delete;
  InputManager &operator=(const InputManager &) = delete;

  // Registers the GLFW callbacks (uses user-pointer).
  void installCallbacks();

  // Called by WebExt when the browser changes Pointer Lock.
  void onPointerLockChanged(bool active);

  [[nodiscard]] bool isCaptured() const noexcept { return m_captured; }
  // true once per click (consumed by Game to fire).
  [[nodiscard]] bool consumeFireRequest() noexcept;

private:
  static void onCursorPos(GLFWwindow *win, double x, double y);
  static void onScroll(GLFWwindow *win, double xoff, double yoff);
  static void onMouseButton(GLFWwindow *win, int button, int action, int mods);
  static void onKey(GLFWwindow *win, int key, int scancode, int action,
                    int mods);

  GLFWwindow *m_window = nullptr;
  ThirdPersonCamera *m_camera = nullptr;
  bool m_captured = false;
  bool m_firstMouse = true;
  double m_lastX = 0.0;
  double m_lastY = 0.0;
  bool m_fireRequested = false;
};
