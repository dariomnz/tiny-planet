#include "platform/Window.h"

#include <GLES3/gl3.h>
#include <GLFW/glfw3.h>
#include <stdexcept>

namespace {
Window *s_active = nullptr;
} // namespace

GlfwInit::GlfwInit() {
  if (!glfwInit())
    throw std::runtime_error("Failed to initialize GLFW");
}

GlfwInit::~GlfwInit() { glfwTerminate(); }

Window::Window(int width, int height, const char *title) {
  // WebGL2 == OpenGL ES 3.0
  glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

  m_handle = glfwCreateWindow(width, height, title, nullptr, nullptr);
  if (!m_handle)
    throw std::runtime_error("Failed to create GLFW window");
  glfwMakeContextCurrent(m_handle);

  glfwGetFramebufferSize(m_handle, &m_fbW, &m_fbH);
  applyViewport();

  // Note: we don't use glfwSetWindowUserPointer here because InputManager
  // needs it for its callbacks. We use a static pointer (single window).
  s_active = this;
  glfwSetFramebufferSizeCallback(m_handle, &Window::onFramebufferSize);
}

Window::~Window() {
  if (s_active == this)
    s_active = nullptr;
  if (m_handle) {
    glfwSetFramebufferSizeCallback(m_handle, nullptr);
    glfwDestroyWindow(m_handle);
  }
}

Window::Window(Window &&other) noexcept
    : m_handle(other.m_handle), m_fbW(other.m_fbW), m_fbH(other.m_fbH) {
  other.m_handle = nullptr;
  if (m_handle)
    s_active = this;
}

Window &Window::operator=(Window &&other) noexcept {
  if (this != &other) {
    if (m_handle)
      glfwDestroyWindow(m_handle);
    m_handle = other.m_handle;
    m_fbW = other.m_fbW;
    m_fbH = other.m_fbH;
    other.m_handle = nullptr;
    if (m_handle)
      s_active = this;
  }
  return *this;
}

void Window::setFramebufferSize(int w, int h) {
  if (w > 0 && h > 0) {
    m_fbW = w;
    m_fbH = h;
    applyViewport();
  }
}

void Window::applyViewport() const { glViewport(0, 0, m_fbW, m_fbH); }

void Window::pollEvents() const { glfwPollEvents(); }

void Window::swapBuffers() const { glfwSwapBuffers(m_handle); }

void Window::onFramebufferSize(GLFWwindow * /*win*/, int w, int h) {
  if (s_active)
    s_active->setFramebufferSize(w, h);
}
