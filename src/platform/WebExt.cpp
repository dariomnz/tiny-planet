#include "platform/WebExt.h"

#include <GLFW/glfw3.h>
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>

#include <cstdlib>
#include <iostream>
#include <string>

#include "platform/Input.h"
#include "platform/Window.h"

namespace {

InputManager *s_input = nullptr;
Window *s_window = nullptr;

// 06-persistence.md recipe + try/catch (private-mode localStorage throws).
EM_JS(char *, js_meta_load, (), {
    try {
        const s = localStorage.getItem("tiny_meta");
        if (!s) return 0;
        const n = lengthBytesUTF8(s) + 1;
        const p = _malloc(n);
        stringToUTF8(s, p, n);
        return p;
    } catch (e) {
        return 0;
    }
});
EM_JS(void, js_meta_save, (const char *s), {
    try {
        localStorage.setItem("tiny_meta", UTF8ToString(s));
    } catch (e) {}
});

EM_BOOL onWebResize(int /*type*/, const EmscriptenUiEvent *e, void * /*ud*/) {
    if (!s_window) return EM_TRUE;
    const double ratio = emscripten_get_device_pixel_ratio();
    int w = static_cast<int>(e->windowInnerWidth * ratio);
    int h = static_cast<int>(e->windowInnerHeight * ratio);
    if (w < 1) w = 1;
    if (h < 1) h = 1;
    emscripten_set_canvas_element_size("#canvas", w, h);
    glfwSetWindowSize(s_window->handle(), w, h);
    return EM_TRUE;
}

// The browser owns Pointer Lock: ESC exits without going through GLFW.
EM_BOOL onPointerLockChange(int /*type*/, const EmscriptenPointerlockChangeEvent *e, void * /*ud*/) {
    if (s_input) s_input->onPointerLockChanged(e->isActive != 0);
    return EM_TRUE;
}

EM_BOOL onPointerLockError(int /*type*/, const void * /*ev*/, void * /*ud*/) {
    // Routine when re-locking too fast after ESC (browser throttle): stay
    // unlocked but armed, the next click retries.
    if (s_input) s_input->onPointerLockFailed();
    // One rejection per failed click: throttle the hint to 1 per 2 s.
    const double now = emscripten_get_now() / 1000.0;
    static double s_lastMsg = -10.0;
    if (now - s_lastMsg > 2.0) {
        s_lastMsg = now;
        std::cout << "Pointer Lock throttled by browser: wait a moment, then click "
                     "the scene to capture the mouse."
                  << std::endl;
    }
    return EM_TRUE;
}

// Source of truth for look: movementX/Y have no limits and no drift,
// unlike clientX/Y (frozen under lock) or the accumulated virtual position
// exposed by GLFW. Only pushed while locked; unlocked motion is left alone
// so ImGui/menus keep receiving the mouse.
EM_BOOL onMouseMove(int /*type*/, const EmscriptenMouseEvent *e, void * /*ud*/) {
    if (s_input && s_input->isCaptured()) {
        s_input->addLookDelta(static_cast<double>(e->movementX), static_cast<double>(e->movementY));
    }
    return EM_FALSE;  // do not consume: GLFW/ImGui must also see the event
}

}  // namespace

namespace web {

void fitCanvasToWindow(Window &window) {
    double cssW = 0.0, cssH = 0.0;
    if (emscripten_get_element_css_size("#canvas", &cssW, &cssH) != EMSCRIPTEN_RESULT_SUCCESS) return;
    const double ratio = emscripten_get_device_pixel_ratio();
    int w = static_cast<int>(cssW * ratio);
    int h = static_cast<int>(cssH * ratio);
    if (w < 1) w = 1;
    if (h < 1) h = 1;
    emscripten_set_canvas_element_size("#canvas", w, h);
    glfwSetWindowSize(window.handle(), w, h);
    int fbW = 0, fbH = 0;
    glfwGetFramebufferSize(window.handle(), &fbW, &fbH);
    window.setFramebufferSize(fbW, fbH);
}

void installResizeHandler(Window &window) {
    s_window = &window;
    emscripten_set_resize_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr, EM_TRUE, onWebResize);
}

void installPointerLockHandlers(Window &window, InputManager &input) {
    s_window = &window;
    s_input = &input;
    emscripten_set_pointerlockchange_callback(EMSCRIPTEN_EVENT_TARGET_DOCUMENT, nullptr, EM_TRUE, onPointerLockChange);
    emscripten_set_pointerlockerror_callback(EMSCRIPTEN_EVENT_TARGET_DOCUMENT, nullptr, EM_TRUE, onPointerLockError);
    emscripten_set_mousemove_callback(EMSCRIPTEN_EVENT_TARGET_DOCUMENT, nullptr, EM_TRUE, onMouseMove);
}

void enterMainLoop(void (*tick)()) { emscripten_set_main_loop(tick, 0, 1); }

std::string metaLoad() {
    char *p = js_meta_load();
    std::string s = p ? p : "";
    if (p) std::free(p);
    return s;
}

void metaSave(const char *s) {
    if (s) js_meta_save(s);
}

}  // namespace web
