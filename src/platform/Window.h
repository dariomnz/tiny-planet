#pragma once

// GLFW window + framebuffer. RAII, move-only.
// Does not include <emscripten/*>: that lives in platform/WebExt.

struct GLFWwindow;

// Initializes/terminates GLFW once (RAII). Must outlive Window.
class GlfwInit {
   public:
    GlfwInit();
    ~GlfwInit();
    GlfwInit(const GlfwInit &) = delete;
    GlfwInit &operator=(const GlfwInit &) = delete;
};

class Window {
   public:
    Window(int width, int height, const char *title);
    ~Window();

    Window(const Window &) = delete;
    Window &operator=(const Window &) = delete;
    Window(Window &&other) noexcept;
    Window &operator=(Window &&other) noexcept;

    [[nodiscard]] GLFWwindow *handle() const noexcept { return m_handle; }
    [[nodiscard]] int fbWidth() const noexcept { return m_fbW; }
    [[nodiscard]] int fbHeight() const noexcept { return m_fbH; }

    void setFramebufferSize(int w, int h);
    void applyViewport() const;
    void pollEvents() const;
    void swapBuffers() const;

   private:
    static void onFramebufferSize(GLFWwindow *win, int w, int h);

    GLFWwindow *m_handle = nullptr;
    int m_fbW = 0;
    int m_fbH = 0;
};
