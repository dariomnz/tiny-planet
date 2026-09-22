#include "platform/WebExt.h"

#include <GLFW/glfw3.h>
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#include <iostream>

#include "platform/Input.h"
#include "platform/Window.h"

namespace {

InputManager *s_input = nullptr;
Window *s_window = nullptr;

EM_BOOL onWebResize(int /*type*/, const EmscriptenUiEvent *e, void * /*ud*/) {
  if (!s_window)
    return EM_TRUE;
  const double ratio = emscripten_get_device_pixel_ratio();
  int w = static_cast<int>(e->windowInnerWidth * ratio);
  int h = static_cast<int>(e->windowInnerHeight * ratio);
  if (w < 1)
    w = 1;
  if (h < 1)
    h = 1;
  emscripten_set_canvas_element_size("#canvas", w, h);
  glfwSetWindowSize(s_window->handle(), w, h);
  return EM_TRUE;
}

// The browser owns Pointer Lock: ESC exits without going through GLFW.
EM_BOOL onPointerLockChange(int /*type*/,
                            const EmscriptenPointerlockChangeEvent *e,
                            void * /*ud*/) {
  if (s_input)
    s_input->onPointerLockChanged(e->isActive != 0);
  return EM_TRUE;
}

EM_BOOL onPointerLockError(int /*type*/, const void * /*ev*/, void * /*ud*/) {
  std::cout << "Pointer Lock rejected: click the canvas to capture the mouse."
            << std::endl;
  return EM_TRUE;
}

} // namespace

namespace web {

void fitCanvasToWindow(Window &window) {
  double cssW = 0.0, cssH = 0.0;
  if (emscripten_get_element_css_size("#canvas", &cssW, &cssH) !=
      EMSCRIPTEN_RESULT_SUCCESS)
    return;
  const double ratio = emscripten_get_device_pixel_ratio();
  int w = static_cast<int>(cssW * ratio);
  int h = static_cast<int>(cssH * ratio);
  if (w < 1)
    w = 1;
  if (h < 1)
    h = 1;
  emscripten_set_canvas_element_size("#canvas", w, h);
  glfwSetWindowSize(window.handle(), w, h);
  int fbW = 0, fbH = 0;
  glfwGetFramebufferSize(window.handle(), &fbW, &fbH);
  window.setFramebufferSize(fbW, fbH);
}

void installResizeHandler(Window &window) {
  s_window = &window;
  emscripten_set_resize_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr,
                                 EM_TRUE, onWebResize);
}

void installPointerLockHandlers(Window &window, InputManager &input) {
  s_window = &window;
  s_input = &input;
  emscripten_set_pointerlockchange_callback(EMSCRIPTEN_EVENT_TARGET_DOCUMENT,
                                            nullptr, EM_TRUE,
                                            onPointerLockChange);
  emscripten_set_pointerlockerror_callback(EMSCRIPTEN_EVENT_TARGET_DOCUMENT,
                                           nullptr, EM_TRUE,
                                           onPointerLockError);
}

} // namespace web
