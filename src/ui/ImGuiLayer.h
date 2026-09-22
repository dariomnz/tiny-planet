#pragma once

// RAII wrapper over Dear ImGui + GLFW + OpenGL3 (WebGL2/GLES3) backends.
// Owns context creation/destruction; Game drives beginFrame()/endFrame().
struct GLFWwindow;

class ImGuiLayer {
   public:
    ImGuiLayer() = default;
    ~ImGuiLayer();

    ImGuiLayer(const ImGuiLayer &) = delete;
    ImGuiLayer &operator=(const ImGuiLayer &) = delete;

    void init(GLFWwindow *window);
    void shutdown();
    // Must be called every frame before any ImGui:: calls.
    void beginFrame(int fbW, int fbH);
    void endFrame();
    // True if any ImGui window wants the mouse/keyboard (to gate game input).
    static bool wantsMouse();
    static bool wantsKeyboard();

    [[nodiscard]] bool initialized() const noexcept { return m_init; }

   private:
    bool m_init = false;
};
