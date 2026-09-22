#pragma once

// Mouse/keyboard state via GLFW callbacks.
// No <emscripten/*>: Pointer Lock sync is done by WebExt.
//
// Ownership model:
//
//   - The browser is the single source of truth for `m_locked`.
//     Only WebExt may change it via onPointerLockChanged()/onPointerLockFailed().
//   - Game declares intent with requestCapture()/releaseCapture()
//     (`m_wantCapture`) and with setCaptureAllowed() (only Run may capture).
//   - ESC exits the lock in the browser and never reaches Game as a key:
//     WebExt flags it via consumeExternalUnlock() and Game auto-pauses
//     (which calls releaseCapture()).
//   - "Look" does NOT use absolute cursor position: WebExt pushes relative
//     deltas (movementX/Y) via addLookDelta() and Game drains them per frame
//     with consumeLookDelta(). No drift, no edge jumps.

struct GLFWwindow;
struct ThirdPersonCamera;

class InputManager {
   public:
    InputManager(GLFWwindow *window, ThirdPersonCamera &camera);
    ~InputManager();

    InputManager(const InputManager &) = delete;
    InputManager &operator=(const InputManager &) = delete;

    // Registers GLFW callbacks (uses user-pointer + static because
    // ImGui_ImplGlfw overwrites the user-pointer when chaining).
    void installCallbacks();

    // Only Run may capture; menus keep a visible cursor.
    // Game sets this every frame before pollEvents().
    void setCaptureAllowed(bool allowed) noexcept { m_captureAllowed = allowed; }

    // Capture intent: arms GLFW_CURSOR_DISABLED.
    // Under Emscripten this installs the click-on-canvas listener as a retry
    // net; called inside a gesture (mousedown) the lock engages on the same
    // click, otherwise (ImGui button processed in the frame) it stays armed
    // and the next scene click completes it. No-op while already locked.
    // NOTE: deliberately ignores m_captureAllowed: Game calls this when
    // entering Run before the per-frame flag is updated.
    void requestCapture();

    // Release intent: back to NORMAL and clears state optimistically.
    // The browser pointerlockchange event will confirm it.
    // Only Game calls this on menu transitions. Never fake
    // onPointerLockChanged() from Game.
    void releaseCapture();

    // True once after the browser exited the lock without Game asking
    // (ESC in Chrome): was locked, still wanted, now unlocked.
    // Game drains this per frame to auto-pause (which calls
    // releaseCapture()). Always drain, act only in Run.
    [[nodiscard]] bool consumeExternalUnlock() noexcept;

    // WebExt only: the browser changed the real lock state.
    void onPointerLockChanged(bool active);
    // WebExt only: the browser rejected the request (e.g. re-locking too
    // fast after ESC: browser throttle). Stays unlocked but armed so the
    // next click retries.
    void onPointerLockFailed();

    // WebExt only: accumulates relative motion while locked.
    void addLookDelta(double dx, double dy);

    // Game drains this once per frame into camera->onMouseMove().
    // Returns false when no motion is pending.
    bool consumeLookDelta(double &dx, double &dy) noexcept;

    [[nodiscard]] bool isCaptured() const noexcept { return m_locked; }
    // true once per click (consumed by Game to fire).
    [[nodiscard]] bool consumeFireRequest() noexcept;

   private:
    static void onScroll(GLFWwindow *win, double xoff, double yoff);
    static void onMouseButton(GLFWwindow *win, int button, int action, int mods);

    static InputManager *s_active;

    GLFWwindow *m_window = nullptr;
    ThirdPersonCamera *m_camera = nullptr;
    bool m_locked = false;
    bool m_wantCapture = false;
    bool m_externalUnlockPending = false;
    bool m_captureAllowed = false;
    double m_lookDX = 0.0;
    double m_lookDY = 0.0;
    bool m_fireRequested = false;
};
