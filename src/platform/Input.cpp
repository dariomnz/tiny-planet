#include "platform/Input.h"

#include <GLFW/glfw3.h>

#include "camera.h"

InputManager::InputManager(GLFWwindow *window, ThirdPersonCamera &camera)
    : m_window(window), m_camera(&camera) {}

InputManager::~InputManager() {
  if (m_window) {
    glfwSetCursorPosCallback(m_window, nullptr);
    glfwSetScrollCallback(m_window, nullptr);
    glfwSetMouseButtonCallback(m_window, nullptr);
    glfwSetKeyCallback(m_window, nullptr);
  }
}

void InputManager::installCallbacks() {
  glfwSetWindowUserPointer(m_window, this);
  glfwSetCursorPosCallback(m_window, &InputManager::onCursorPos);
  glfwSetScrollCallback(m_window, &InputManager::onScroll);
  glfwSetMouseButtonCallback(m_window, &InputManager::onMouseButton);
  glfwSetKeyCallback(m_window, &InputManager::onKey);
  glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

void InputManager::onPointerLockChanged(bool active) {
  m_captured = active;
  m_firstMouse = true;
  if (!active && m_window)
    glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

bool InputManager::consumeFireRequest() noexcept {
  const bool r = m_fireRequested;
  m_fireRequested = false;
  return r;
}

void InputManager::onCursorPos(GLFWwindow *win, double x, double y) {
  auto *self = static_cast<InputManager *>(glfwGetWindowUserPointer(win));
  if (!self || !self->m_captured)
    return;
  if (self->m_firstMouse) {
    self->m_lastX = x;
    self->m_lastY = y;
    self->m_firstMouse = false;
    return;
  }
  self->m_camera->onMouseMove(x - self->m_lastX, y - self->m_lastY);
  self->m_lastX = x;
  self->m_lastY = y;
}

void InputManager::onScroll(GLFWwindow *win, double /*xoff*/, double yoff) {
  auto *self = static_cast<InputManager *>(glfwGetWindowUserPointer(win));
  if (self)
    self->m_camera->onScroll(yoff);
}

void InputManager::onMouseButton(GLFWwindow *win, int button, int action,
                                 int /*mods*/) {
  auto *self = static_cast<InputManager *>(glfwGetWindowUserPointer(win));
  if (!self || button != GLFW_MOUSE_BUTTON_LEFT || action != GLFW_PRESS)
    return;
  if (!self->m_captured) {
    glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    self->m_firstMouse = true;
    self->m_captured = true;
  } else {
    self->m_fireRequested = true;
  }
}

void InputManager::onKey(GLFWwindow *win, int key, int /*scancode*/, int action,
                         int /*mods*/) {
  auto *self = static_cast<InputManager *>(glfwGetWindowUserPointer(win));
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS && self &&
      self->m_captured) {
    glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    self->m_captured = false;
    self->m_firstMouse = true;
    // In the browser ESC is also handled by the browser itself: WebExt syncs.
  }
}
