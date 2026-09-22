#pragma once

#include <string>

// Everything touching <emscripten/*> lives here and only here.
// Window / Input / Game do not include Emscripten headers.

class InputManager;
class Window;

namespace web {

// Fits the canvas to its CSS size x devicePixelRatio and forwards it to GLFW.
void fitCanvasToWindow(Window &window);

// Listens to browser window resize and resizes the canvas.
void installResizeHandler(Window &window);

// Syncs InputManager with the browser Pointer Lock and pushes relative
// deltas (movementX/Y) for the camera look.
// Call once, after InputManager::installCallbacks() and before ImGui::init().
void installPointerLockHandlers(Window &window, InputManager &input);

// Enters the browser loop (emscripten_set_main_loop). tick runs on every
// vsync and never returns.
void enterMainLoop(void (*tick)());

// M4: localStorage persistence for Meta only (06-persistence.md).
// Key "tiny_meta". Call on run end + each purchase, never per frame.
// metaLoad returns "" when there is no save (or storage throws).
std::string metaLoad();
void metaSave(const char *s);

}  // namespace web
