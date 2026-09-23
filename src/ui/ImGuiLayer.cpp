#include "ui/ImGuiLayer.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

ImGuiLayer::~ImGuiLayer() { shutdown(); }

void ImGuiLayer::init(GLFWwindow *window) {
    if (m_init) return;
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    // Web: no multi-viewport, no docking windows outside the canvas.
    io.ConfigFlags &= ~static_cast<int>(ImGuiConfigFlags_ViewportsEnable);
    io.IniFilename = nullptr;  // no imgui.ini on web (in-memory only)

    ImGui::StyleColorsDark();

    // Must run AFTER InputManager::installCallbacks so the backend can
    // chain to the game's GLFW callbacks instead of being overwritten.
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 300 es");
    m_init = true;
}

void ImGuiLayer::shutdown() {
    if (!m_init) return;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    m_init = false;
}

void ImGuiLayer::beginFrame(int fbW, int fbH) {
    if (!m_init) return;
    ImGuiIO &io = ImGui::GetIO();
    if (fbW > 0 && fbH > 0) io.DisplaySize = ImVec2(static_cast<float>(fbW), static_cast<float>(fbH));
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void ImGuiLayer::endFrame() {
    if (!m_init) return;
    render();
    renderDrawData();
}

void ImGuiLayer::render() {
    if (!m_init) return;
    ImGui::Render();
}

void ImGuiLayer::renderDrawData() {
    if (!m_init) return;
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

bool ImGuiLayer::wantsMouse() {
    ImGuiContext *ctx = ImGui::GetCurrentContext();
    return ctx != nullptr && ImGui::GetIO().WantCaptureMouse;
}

bool ImGuiLayer::wantsKeyboard() {
    ImGuiContext *ctx = ImGui::GetCurrentContext();
    return ctx != nullptr && ImGui::GetIO().WantCaptureKeyboard;
}
