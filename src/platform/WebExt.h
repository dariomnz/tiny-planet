#pragma once

// Everything touching <emscripten/*> lives here and only here.
// Window/Input/Game do not include Emscripten headers.

struct GLFWwindow;
class InputManager;
class Window;

namespace web {

// Fits the canvas to its CSS size x devicePixelRatio and forwards it to GLFW.
void fitCanvasToWindow(Window &window);

// Listens to browser window resize and resizes the canvas.
void installResizeHandler(Window &window);

// Syncs InputManager with the browser Pointer Lock.
void installPointerLockHandlers(Window &window, InputManager &input);

} // namespace web
