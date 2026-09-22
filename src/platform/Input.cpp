#include "platform/Input.h"

#include <GLFW/glfw3.h>
#include <imgui.h>

#include <iostream>

#include "camera.h"

InputManager *InputManager::s_active = nullptr;

namespace {
bool imguiWantsMouse() {
    ImGuiContext *ctx = ImGui::GetCurrentContext();
    return ctx != nullptr && ImGui::GetIO().WantCaptureMouse;
}
}  // namespace

InputManager::InputManager(GLFWwindow *window, ThirdPersonCamera &camera) : m_window(window), m_camera(&camera) {
    s_active = this;
}

InputManager::~InputManager() {
    s_active = nullptr;
    if (m_window) {
        glfwSetScrollCallback(m_window, nullptr);
        glfwSetMouseButtonCallback(m_window, nullptr);
    }
}

void InputManager::installCallbacks() {
    glfwSetWindowUserPointer(m_window, this);
    // No cursor-pos callback on purpose: look comes from movementX/Y
    // (WebExt) and menus are owned by ImGui with its own chained callback.
    glfwSetScrollCallback(m_window, &InputManager::onScroll);
    glfwSetMouseButtonCallback(m_window, &InputManager::onMouseButton);
    glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

void InputManager::requestCapture() {
    if (!m_window || m_locked) return;
    m_wantCapture = true;
    m_externalUnlockPending = false;
    // Arm DISABLED. Inside a gesture it engages on the same click;
    // outside a gesture Emscripten defers it to the next canvas click.
    glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwGetInputMode(m_window, GLFW_CURSOR);
    std::cout << "request Capture " << glfwGetInputMode(m_window, GLFW_CURSOR) << std::endl;
}

void InputManager::releaseCapture() {
    m_wantCapture = false;
    m_externalUnlockPending = false;
    m_locked = false;
    m_firingHeld = false;
    m_fireRequested = false;
    m_lookDX = 0.0;
    m_lookDY = 0.0;
    if (m_window) glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    std::cout << "release Capture " << glfwGetInputMode(m_window, GLFW_CURSOR) << std::endl;
}

bool InputManager::consumeExternalUnlock() noexcept {
    const bool r = m_externalUnlockPending;
    m_externalUnlockPending = false;
    return r;
}

void InputManager::onPointerLockChanged(bool active) {
    const bool wasLocked = m_locked;
    m_locked = active;
    if (!active) {
        m_lookDX = 0.0;
        m_lookDY = 0.0;
        m_firingHeld = false;
        // ESC exits the lock at browser level and never reaches Game as a
        // key press, so flag it: Game auto-pauses, which is what calls
        // releaseCapture(). Intentional menu transitions already cleared
        // m_wantCapture via releaseCapture(), so they never flag here.
        // The initial "never locked yet" Run entry has wasLocked == false,
        // so it never flags either.
        if (wasLocked && m_wantCapture) m_externalUnlockPending = true;
    }
    // NEVER touch the input mode here: while in Run it must stay DISABLED so
    // that Emscripten's retry listener keeps offering the lock on every
    // click. Game already sets NORMAL explicitly when leaving to a menu via
    // releaseCapture(). Setting NORMAL here (or exitPointerLock on failure)
    // is what used to strand the game unlocked: it removed the retry and
    // could cancel a request still in flight.
}

void InputManager::onPointerLockFailed() {
    m_locked = false;
    m_firingHeld = false;
    m_lookDX = 0.0;
    m_lookDY = 0.0;
    // Same reason: stay DISABLED + retry listener, just report unlocked.
}

void InputManager::addLookDelta(double dx, double dy) {
    if (!m_locked) return;
    m_lookDX += dx;
    m_lookDY += dy;
}

bool InputManager::consumeLookDelta(double &dx, double &dy) noexcept {
    dx = m_lookDX;
    dy = m_lookDY;
    m_lookDX = 0.0;
    m_lookDY = 0.0;
    return dx != 0.0 || dy != 0.0;
}

bool InputManager::consumeFireRequest() noexcept {
    const bool r = m_fireRequested;
    m_fireRequested = false;
    return r;
}

void InputManager::onScroll(GLFWwindow *win, double /*xoff*/, double yoff) {
    // NOTE: no glfwGetWindowUserPointer here — ImGui_ImplGlfw overwrites the
    // user-pointer when chaining callbacks, so we use a static instead.
    auto *self = s_active;
    (void)win;
    if (!self) return;
    if (!self->m_locked && imguiWantsMouse()) return;  // scrolling an ImGui window must not zoom the camera
    // Inverted wheel: scroll up = zoom out, scroll down = zoom in.
    self->m_camera->onScroll(-yoff);
}

void InputManager::onMouseButton(GLFWwindow *win, int button, int action, int /*mods*/) {
    auto *self = s_active;
    (void)win;
    if (!self || button != GLFW_MOUSE_BUTTON_LEFT) return;
    if (action == GLFW_RELEASE) {
        self->m_firingHeld = false;
        return;
    }
    if (action != GLFW_PRESS) return;
    if (self->m_locked) {
        // Locked: every click is game fire, even if the virtual cursor
        // drifted over an ImGui window.
        self->m_fireRequested = true;
        self->m_firingHeld = true;
        return;
    }
    // Not captured: an explicit click on empty scene (no ImGui under the
    // cursor, gameplay state) must capture. This runs inside the mousedown
    // gesture so the browser accepts the request on the same click.
    // The first click only captures, it never fires.
    if (!self->m_captureAllowed) return;  // menus: keep the cursor visible, never request lock
    if (imguiWantsMouse()) return;        // click is for ImGui (hub/draft/buttons), not capture
    self->requestCapture();
}
