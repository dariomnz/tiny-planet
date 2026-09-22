#include <GLES3/gl3.h>
#include <GLFW/glfw3.h>
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>

#include <cmath>
#include <cstdlib>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "camera.h"

// Third-person camera (see camera.h/.cpp).
// On web GLFW_CURSOR_DISABLED is used, which Emscripten maps to Pointer Lock:
// no manual re-centering to the middle needed as on WSLg.
static ThirdPersonCamera g_cam;
static bool g_mouseCaptured = false;
static double g_lastX = 0.0, g_lastY = 0.0;
static bool g_firstMouse = true;

// --- Projectiles: small cubes fired from the player that disappear ---
struct Projectile {
  glm::vec3 pos{0.0f};
  glm::vec3 vel{0.0f};
  float life = 0.0f;
};
static std::vector<Projectile> g_projectiles;
static const float kProjSpeed = 20.0f; // units / second
static const float kProjLife = 2.0f;   // seconds until despawn
static const float kProjSize = 0.25f;  // cube side length
static const float kProjSpawnZ = 1.2f; // spawn height (player chest)
static const float kProjForwardOffset = 0.8f; // spawn ahead of the player
static const size_t kProjMax = 100;
void spawnProjectile(); // defined below: needs g_player and g_cam

void cursor_pos_callback(GLFWwindow * /*window*/, double xpos, double ypos) {
  if (!g_mouseCaptured)
    return;
  if (g_firstMouse) {
    g_lastX = xpos;
    g_lastY = ypos;
    g_firstMouse = false;
    return;
  }
  g_cam.onMouseMove(xpos - g_lastX, ypos - g_lastY);
  g_lastX = xpos;
  g_lastY = ypos;
}

// Mouse wheel -> smooth zoom (move closer to / farther from the character).
void scroll_callback(GLFWwindow * /*window*/, double /*xoffset*/,
                     double yoffset) {
  g_cam.onScroll(yoffset);
}

// Left click -> capture the cursor (Pointer Lock). ESC -> release.
// Once captured, each click fires a projectile.
void mouse_button_callback(GLFWwindow *window, int button, int action,
                           int /*mods*/) {
  if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
    if (!g_mouseCaptured) {
      glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
      g_firstMouse = true;
      g_mouseCaptured = true;
    } else {
      spawnProjectile();
    }
  }
}

void key_callback(GLFWwindow *window, int key, int /*scancode*/, int action,
                  int /*mods*/) {
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
    if (g_mouseCaptured) {
      glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
      g_mouseCaptured = false;
      g_firstMouse = true;
    }
    // In the browser the window is not closed with ESC: only the mouse is released.
    // (The browser also exits Pointer Lock on its own with ESC.)
    (void)window;
  }
}

// Planet: grid re-centered under the player (per-cell snap).
// REAL curvature on z (visible thanks to perspective) + barycentric
// edges with a `fill` parameter in the fragment shader.
const char *planetVertSrc = R"(#version 300 es
precision highp float;
layout (location = 0) in vec2 inPos;
layout (location = 1) in vec3 inBary;
out vec3 vBary;
out float vDist;
uniform mat4 mvp;
uniform vec2 playerPos;   // sinking is measured from the player
uniform vec2 gridOffset;  // grid snap (cell multiple)
uniform float curveK;     // 0 = flat, higher = smaller planet
void main() {
    vec2 world = inPos + gridOffset;
    vec2 rel = world - playerPos;
    float z = -curveK * dot(rel, rel); // farther from player = more sunken
    gl_Position = mvp * vec4(world.x, world.y, z, 1.0);
    vBary = inBary;
    vDist = length(rel);
}
)";

const char *planetFragSrc = R"(#version 300 es
precision mediump float;
in vec3 vBary;
in float vDist;
out vec4 outColor;
uniform float fill;       // 0 = edges only, 1 = solid triangle
uniform vec3 edgeColor;
uniform float fogDensity; // distance attenuation
void main() {
    float d = min(vBary.x, min(vBary.y, vBary.z)); // 0 on edge, ~0.33 center
    float w = fwidth(d) + 1e-6;
    float t = mix(w * 1.5, 0.5, fill);
    float a = 1.0 - smoothstep(t - w, t, d);
    float fog = exp(-vDist * fogDensity);
    outColor = vec4(edgeColor * a * fog, 1.0);
}
)";

// Player / nose / projectiles: cubes with simple per-face shading +
// SAME curvature as the planet (same `curveK` and `playerPos`).
// `projView` and `model` are kept separate so the world can be curved:
// world = model * inPos; world.z -= curveK * |world.xy - playerPos|^2.
const char *playerVertSrc = R"(#version 300 es
precision highp float;
layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
out vec3 vNormal;
uniform mat4 projView;
uniform mat4 model;
uniform vec2 playerPos;
uniform float curveK;
void main() {
    vec4 world = model * vec4(inPos, 1.0);
    vec2 rel = world.xy - playerPos;
    world.z -= curveK * dot(rel, rel); // same formula as planetVertSrc
    gl_Position = projView * world;
    vNormal = inNormal; // the model only translates/rotates on Z: normals stay unchanged
}
)";

const char *playerFragSrc = R"(#version 300 es
precision mediump float;
in vec3 vNormal;
out vec4 outColor;
uniform vec3 color;
void main() {
    vec3 lightDir = normalize(vec3(0.4, 0.5, 0.75));
    float shade = 0.45 + 0.55 * max(dot(normalize(vNormal), lightDir), 0.0);
    outColor = vec4(color * shade, 1.0);
}
)";

GLuint compileShader(GLenum type, const char *src) {
  GLuint shader = glCreateShader(type);
  glShaderSource(shader, 1, &src, nullptr);
  glCompileShader(shader);
  GLint ok = 0;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
  if (!ok) {
    char log[512];
    glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
    throw std::runtime_error(std::string("Shader compile error: ") + log);
  }
  return shader;
}

GLuint createProgram(const char *vertSrc, const char *fragSrc) {
  GLuint vert = compileShader(GL_VERTEX_SHADER, vertSrc);
  GLuint frag = compileShader(GL_FRAGMENT_SHADER, fragSrc);
  GLuint prog = glCreateProgram();
  glAttachShader(prog, vert);
  glAttachShader(prog, frag);
  glLinkProgram(prog);
  GLint ok = 0;
  glGetProgramiv(prog, GL_LINK_STATUS, &ok);
  if (!ok) {
    char log[512];
    glGetProgramInfoLog(prog, sizeof(log), nullptr, log);
    throw std::runtime_error(std::string("Program link error: ") + log);
  }
  glDeleteShader(vert);
  glDeleteShader(frag);
  return prog;
}

// --- 2D OpenGL text (HUD): 5x7 bitmap font, no textures or libraries ---
// Used for the FPS counter: each lit pixel emits a quad (2 triangles)
// in pixel coordinates with a top-left origin.
const char *textVertSrc = R"(#version 300 es
precision highp float;
layout (location = 0) in vec2 inPix;
uniform vec2 screenSize;
void main() {
    vec2 ndc = vec2(inPix.x / screenSize.x * 2.0 - 1.0,
                    1.0 - inPix.y / screenSize.y * 2.0);
    gl_Position = vec4(ndc, 0.0, 1.0);
}
)";

const char *textFragSrc = R"(#version 300 es
precision mediump float;
out vec4 outColor;
uniform vec3 color;
void main() {
    outColor = vec4(color, 1.0);
}
)";

// 5x7 font: 7 rows of 5 bits ('1' = pixel). Only what is needed for "0123456789FPS ".

// Build 2D triangles for `text` with the top-right corner at
// (right, top), pixels of size `px`, including inter-character spacing.
static void buildTextGeometry(const std::string &text, float right, float top,
                              float px, std::vector<float> &out) {
  out.clear();
  const float charW = 6.0f * px; // 5 pixels + 1 of spacing
  const float totalW = (float)text.size() * charW;
  float x0 = right - totalW;
  auto quad = [&](float x, float y, float w, float h) {
    out.insert(out.end(), {x, y, x + w, y, x, y + h,
                           x + w, y, x + w, y + h, x, y + h});
  };
  for (size_t i = 0; i < text.size(); ++i) {
    const char **g = nullptr;
    // Trick: rebuild rows from the same switch via a local table.
    // To avoid duplicating data, iterate rows using fontGlyph per row:
    // instead, local static table:
    static const char *G0[] = {"01110", "10001", "10011", "10101",
                               "11001", "10001", "01110"};
    static const char *G1[] = {"00100", "01100", "00100", "00100",
                               "00100", "00100", "01110"};
    static const char *G2[] = {"01110", "10001", "00001", "00110",
                               "01000", "10000", "11111"};
    static const char *G3[] = {"11111", "00010", "00100", "00010",
                               "00001", "10001", "01110"};
    static const char *G4[] = {"00010", "00110", "01010", "10010",
                               "11111", "00010", "00010"};
    static const char *G5[] = {"11111", "10000", "11110", "00001",
                               "00001", "10001", "01110"};
    static const char *G6[] = {"00110", "01000", "10000", "11110",
                               "10001", "10001", "01110"};
    static const char *G7[] = {"11111", "00001", "00010", "00100",
                               "01000", "01000", "01000"};
    static const char *G8[] = {"01110", "10001", "10001", "01110",
                               "10001", "10001", "01110"};
    static const char *G9[] = {"01110", "10001", "10001", "01111",
                               "00001", "00010", "01100"};
    static const char *GF[] = {"11111", "10000", "10000", "11110",
                               "10000", "10000", "10000"};
    static const char *GP[] = {"11110", "10001", "10001", "11110",
                               "10000", "10000", "10000"};
    static const char *GS[] = {"01111", "10000", "10000", "01110",
                               "00001", "00001", "11110"};
    static const char *GSP[] = {"00000", "00000", "00000", "00000",
                                "00000", "00000", "00000"};
    switch (text[i]) {
    case '0': g = (const char **)G0; break;
    case '1': g = (const char **)G1; break;
    case '2': g = (const char **)G2; break;
    case '3': g = (const char **)G3; break;
    case '4': g = (const char **)G4; break;
    case '5': g = (const char **)G5; break;
    case '6': g = (const char **)G6; break;
    case '7': g = (const char **)G7; break;
    case '8': g = (const char **)G8; break;
    case '9': g = (const char **)G9; break;
    case 'F': g = (const char **)GF; break;
    case 'P': g = (const char **)GP; break;
    case 'S': g = (const char **)GS; break;
    default: g = (const char **)GSP; break;
    }
    float cx = x0 + (float)i * charW;
    for (int r = 0; r < 7; ++r)
      for (int c = 0; c < 5; ++c)
        if (g[r][c] == '1')
          quad(cx + (float)c * px, top + (float)r * px, px, px);
  }
}

// --- Global frame state (Emscripten main loop) ---
static GLFWwindow *g_window = nullptr;
static GLuint planetVAO = 0, planetVBO = 0;
static GLuint playerVAO = 0, playerVBO = 0;
static GLuint noseVAO = 0, noseVBO = 0;
static GLuint planetProg = 0, playerProg = 0;
static GLint planetMvpLoc = -1, playerPosLoc = -1, gridOffLoc = -1;
static GLint curveLoc = -1, fillLoc = -1, edgeLoc = -1, fogLoc = -1;
static GLint playerProjViewLoc = -1, playerModelLoc = -1;
static GLint playerPlayerPosLoc = -1, playerCurveLoc = -1, colorLoc = -1;
static GLsizei planetCount = 0, playerCount = 0, noseCount = 0;
static int g_fbW = 800, g_fbH = 600;

static glm::vec2 g_player(0.0f, 0.0f);
static float g_playerYaw = 0.0f; // where the player faces (Z axis)
static const float moveSpeed = 8.0f;
static float g_fill = 0.0f;
static float g_curveK = 0.02f;
static const float fogDensity = 0.015f;
static const float CELL = 2.0f;
static double g_lastTime = 0.0;
static double g_fpsLast = 0.0;
static int g_fpsFrames = 0;

// Projectile mesh (small centered cube, same pos+normal layout).
static GLuint projVAO = 0, projVBO = 0;
static GLsizei projCount = 0;

// Spawn a projectile at the player, with horizontal velocity towards
// where the camera/player faces (current yaw). No gravity.
void spawnProjectile() {
  glm::vec3 dir(std::cos(g_cam.yaw), std::sin(g_cam.yaw), 0.0f);
  Projectile p;
  p.pos = glm::vec3(g_player.x + dir.x * kProjForwardOffset,
                    g_player.y + dir.y * kProjForwardOffset, kProjSpawnZ);
  p.vel = dir * kProjSpeed;
  p.life = kProjLife;
  if (g_projectiles.size() >= kProjMax) {
    // Recycle the oldest one to avoid unbounded growth on click spam.
    g_projectiles.erase(g_projectiles.begin());
  }
  g_projectiles.push_back(p);
}

// 100% OpenGL FPS HUD (no DOM).
static GLuint textVAO = 0, textVBO = 0, textProg = 0;
static GLint textScreenSizeLoc = -1, textColorLoc = -1;
static std::string g_fpsText = "60 FPS";
static std::vector<float> g_textVerts;
static bool g_textDirty = true;
static int g_textFbW = 0, g_textFbH = 0;

static void frame() {
  glfwPollEvents();

  double now = glfwGetTime();
  float dt = (float)(now - g_lastTime);
  g_lastTime = now;
  // Avoid large jumps when switching tabs / first frame.
  if (dt > 0.05f)
    dt = 0.05f;
  if (dt < 0.0f)
    dt = 0.0f;

  // WASD relative to the camera yaw
  glm::vec2 fwd(std::cos(g_cam.yaw), std::sin(g_cam.yaw));
  glm::vec2 right(fwd.y, -fwd.x);
  if (glfwGetKey(g_window, GLFW_KEY_W) == GLFW_PRESS)
    g_player += fwd * moveSpeed * dt;
  if (glfwGetKey(g_window, GLFW_KEY_S) == GLFW_PRESS)
    g_player -= fwd * moveSpeed * dt;
  if (glfwGetKey(g_window, GLFW_KEY_D) == GLFW_PRESS)
    g_player += right * moveSpeed * dt;
  if (glfwGetKey(g_window, GLFW_KEY_A) == GLFW_PRESS)
    g_player -= right * moveSpeed * dt;
  if (glfwGetKey(g_window, GLFW_KEY_Q) == GLFW_PRESS)
    g_fill -= dt * 0.5f;
  if (glfwGetKey(g_window, GLFW_KEY_E) == GLFW_PRESS)
    g_fill += dt * 0.5f;
  if (g_fill < 0.0f)
    g_fill = 0.0f;
  if (g_fill > 1.0f)
    g_fill = 1.0f;
  if (glfwGetKey(g_window, GLFW_KEY_R) == GLFW_PRESS)
    g_curveK -= dt * 0.05f;
  if (glfwGetKey(g_window, GLFW_KEY_F) == GLFW_PRESS)
    g_curveK += dt * 0.05f;
  if (g_curveK < 0.0f)
    g_curveK = 0.0f;
  if (g_curveK > 0.2f)
    g_curveK = 0.2f;

  float snapX = std::floor(g_player.x / CELL + 0.5f) * CELL;
  float snapY = std::floor(g_player.y / CELL + 0.5f) * CELL;

  // The player faces where the camera faces (smooth turn on Z).
  {
    float diff = std::atan2(std::sin(g_cam.yaw - g_playerYaw),
                            std::cos(g_cam.yaw - g_playerYaw));
    float t = 1.0f - std::exp(-10.0f * dt);
    g_playerYaw += diff * t;
  }

  // Projectiles: straight motion + time expiry (swap-remove).
  for (size_t i = 0; i < g_projectiles.size();) {
    Projectile &p = g_projectiles[i];
    p.pos += p.vel * dt;
    p.life -= dt;
    if (p.life <= 0.0f) {
      p = g_projectiles.back();
      g_projectiles.pop_back();
    } else {
      ++i;
    }
  }

  // Third-person camera (logic in camera.cpp).
  g_cam.update(dt, g_player);
  glm::mat4 view = g_cam.getView();
  float aspect = (float)g_fbW / (float)(g_fbH > 0 ? g_fbH : 1);
  glm::mat4 proj =
      glm::perspective(glm::radians(60.0f), aspect, 0.1f, 500.0f);

  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // Planet
  glUseProgram(planetProg);
  glm::mat4 planetMvp = proj * view;
  glUniformMatrix4fv(planetMvpLoc, 1, GL_FALSE, glm::value_ptr(planetMvp));
  glUniform2f(playerPosLoc, g_player.x, g_player.y);
  glUniform2f(gridOffLoc, snapX, snapY);
  glUniform1f(curveLoc, g_curveK);
  glUniform1f(fillLoc, g_fill);
  glUniform3f(edgeLoc, 0.2f, 1.0f, 0.4f);
  glUniform1f(fogLoc, fogDensity);
  glBindVertexArray(planetVAO);
  glDrawArrays(GL_TRIANGLES, 0, planetCount);

  // Player cube (rotated on Z to aim like the camera)
  glUseProgram(playerProg);
  glm::mat4 projView = proj * view;
  glUniformMatrix4fv(playerProjViewLoc, 1, GL_FALSE, glm::value_ptr(projView));
  glUniform2f(playerPlayerPosLoc, g_player.x, g_player.y);
  glUniform1f(playerCurveLoc, g_curveK);
  glm::mat4 model = glm::translate(glm::mat4(1.0f),
                                   glm::vec3(g_player.x, g_player.y, 0.0f));
  model = glm::rotate(model, g_playerYaw, glm::vec3(0.0f, 0.0f, 1.0f));
  glUniformMatrix4fv(playerModelLoc, 1, GL_FALSE, glm::value_ptr(model));
  glUniform3f(colorLoc, 1.0f, 0.25f, 0.2f);
  glBindVertexArray(playerVAO);
  glDrawArrays(GL_TRIANGLES, 0, playerCount);

  // Front nose (facing marker): same model, different color.
  glUniformMatrix4fv(playerModelLoc, 1, GL_FALSE, glm::value_ptr(model));
  glUniform3f(colorLoc, 1.0f, 0.85f, 0.2f);
  glBindVertexArray(noseVAO);
  glDrawArrays(GL_TRIANGLES, 0, noseCount);

  // Projectiles: same shader as the player (with curvature), one draw per cube.
  glUniform3f(colorLoc, 0.3f, 0.8f, 1.0f);
  glBindVertexArray(projVAO);
  for (const Projectile &p : g_projectiles) {
    glm::mat4 m = glm::translate(glm::mat4(1.0f), p.pos);
    glUniformMatrix4fv(playerModelLoc, 1, GL_FALSE, glm::value_ptr(m));
    glDrawArrays(GL_TRIANGLES, 0, projCount);
  }
  glBindVertexArray(0);

  // FPS: average every 0.5 s, only marks the text as dirty (2 Hz).
  ++g_fpsFrames;
  if (now - g_fpsLast >= 0.5) {
    int fps = (int)(g_fpsFrames / (now - g_fpsLast));
    g_fpsFrames = 0;
    g_fpsLast = now;
    if (fps < 0)
      fps = 0;
    if (fps > 999)
      fps = 999;
    std::string t = std::to_string(fps) + " FPS";
    if (t != g_fpsText) {
      g_fpsText = t;
      g_textDirty = true;
    }
  }

  // 2D OpenGL HUD top-right: rebuild only when text or size changes.
  if (g_textDirty || g_fbW != g_textFbW || g_fbH != g_textFbH) {
    buildTextGeometry(g_fpsText, (float)g_fbW - 12.0f, 12.0f, 3.0f,
                      g_textVerts);
    glBindVertexArray(textVAO);
    glBindBuffer(GL_ARRAY_BUFFER, textVBO);
    glBufferData(GL_ARRAY_BUFFER, g_textVerts.size() * sizeof(float),
                 g_textVerts.empty() ? nullptr : g_textVerts.data(),
                 GL_DYNAMIC_DRAW);
    glBindVertexArray(0);
    g_textDirty = false;
    g_textFbW = g_fbW;
    g_textFbH = g_fbH;
  }
  if (!g_textVerts.empty()) {
    glDisable(GL_DEPTH_TEST);
    glUseProgram(textProg);
    glUniform2f(textScreenSizeLoc, (float)g_fbW, (float)g_fbH);
    glUniform3f(textColorLoc, 0.2f, 1.0f, 0.3f);
    glBindVertexArray(textVAO);
    glDrawArrays(GL_TRIANGLES, 0,
                 (GLsizei)(g_textVerts.size() / 2));
    glBindVertexArray(0);
    glEnable(GL_DEPTH_TEST);
  }

  glfwSwapBuffers(g_window);
}

// --- Screen size on web: the canvas rules ---
// CSS sets the canvas to 100vw/100vh, but the WebGL framebuffer must be
// resized by hand. Single source of truth: g_fbW/g_fbH, which are
// updated in the GLFW framebuffer callback.
static void applyFramebufferSize(int w, int h) {
  if (w > 0 && h > 0) {
    g_fbW = w;
    g_fbH = h;
    glViewport(0, 0, w, h);
  }
}

// Fit the canvas backing store to its CSS size x devicePixelRatio
// (crisp on HiDPI) and propagate the change to GLFW.
static void fitCanvasToWindow() {
  double cssW = 0.0, cssH = 0.0;
  if (emscripten_get_element_css_size("#canvas", &cssW, &cssH) !=
      EMSCRIPTEN_RESULT_SUCCESS)
    return;
  double ratio = emscripten_get_device_pixel_ratio();
  int w = (int)(cssW * ratio);
  int h = (int)(cssH * ratio);
  if (w < 1)
    w = 1;
  if (h < 1)
    h = 1;
  emscripten_set_canvas_element_size("#canvas", w, h);
  glfwSetWindowSize(g_window, w, h);
  int fbW = 0, fbH = 0;
  glfwGetFramebufferSize(g_window, &fbW, &fbH);
  applyFramebufferSize(fbW, fbH);
}
static EM_BOOL onWebResize(int /*type*/, const EmscriptenUiEvent *e,
                           void * /*ud*/)
{
  double ratio = emscripten_get_device_pixel_ratio();
  int w = (int)(e->windowInnerWidth * ratio);
  int h = (int)(e->windowInnerHeight * ratio);
  if (w < 1)
    w = 1;
  if (h < 1)
    h = 1;
  emscripten_set_canvas_element_size("#canvas", w, h);
  glfwSetWindowSize(g_window, w, h);
  return EM_TRUE;
}

// --- Mouse on web: the browser owns Pointer Lock ---
// If the user presses ESC, the browser exits the lock on its own without going
// through key_callback: this event is the single source of truth for the flag.
static EM_BOOL onPointerLockChange(int /*type*/,
                                   const EmscriptenPointerlockChangeEvent *e,
                                   void * /*ud*/) {
  if (e->isActive) {
    g_mouseCaptured = true;
    g_firstMouse = true;
  } else {
    if (g_window)
      glfwSetInputMode(g_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    g_mouseCaptured = false;
    g_firstMouse = true;
  }
  return EM_TRUE;
}

static EM_BOOL onPointerLockError(int /*type*/, const void * /*ev*/,
                                  void * /*ud*/) {
  std::cout << "Pointer Lock rejected: click the canvas to capture "
                 "the mouse."
            << std::endl;
  return EM_TRUE;
}

int main() {
  try {
    if (!glfwInit()) {
      throw std::runtime_error("Failed to initialize GLFW");
    }

    // WebGL2 == OpenGL ES 3.0
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    g_window = glfwCreateWindow(g_fbW, g_fbH, "Planet 3D", nullptr, nullptr);
    if (!g_window) {
      glfwTerminate();
      throw std::runtime_error("Failed to create GLFW window");
    }
    glfwMakeContextCurrent(g_window);

    // Diagnostics: confirm we have a WebGL2 (ES 3.0) context.
    {
      const char *ver =
          (const char *)glGetString(GL_VERSION);
      const char *sl =
          (const char *)glGetString(GL_SHADING_LANGUAGE_VERSION);
      std::cout << "GL_VERSION: " << (ver ? ver : "<null>") << std::endl;
      std::cout << "GLSL_VERSION: " << (sl ? sl : "<null>") << std::endl;
    }

    glfwGetFramebufferSize(g_window, &g_fbW, &g_fbH);
    glViewport(0, 0, g_fbW, g_fbH);
    glfwSetFramebufferSizeCallback(g_window, [](GLFWwindow *, int nw, int nh) {
      applyFramebufferSize(nw, nh);
    });
    // The canvas rules: fit it to the real window and listen for resizes.
    fitCanvasToWindow();
    emscripten_set_resize_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr,
                                   EM_TRUE, onWebResize);
    // The browser owns Pointer Lock: sync the flag with it.
    emscripten_set_pointerlockchange_callback(
        EMSCRIPTEN_EVENT_TARGET_DOCUMENT, nullptr, EM_TRUE,
        onPointerLockChange);
    emscripten_set_pointerlockerror_callback(
        EMSCRIPTEN_EVENT_TARGET_DOCUMENT, nullptr, EM_TRUE,
        onPointerLockError);

    // Mouse: starts free (NORMAL). Click = Pointer Lock, ESC = release.
    glfwSetMouseButtonCallback(g_window, mouse_button_callback);
    glfwSetKeyCallback(g_window, key_callback);
    glfwSetCursorPosCallback(g_window, cursor_pos_callback);
    glfwSetScrollCallback(g_window, scroll_callback);
    glfwSetInputMode(g_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    glEnable(GL_DEPTH_TEST);

    // --- Planet mesh: CELLS x CELLS grid, non-indexed ---
    const int CELLS = 60;
    const float HALF = CELLS * CELL * 0.5f;
    std::vector<float> grid; // x, y, bx, by, bz per vertex
    grid.reserve((size_t)CELLS * CELLS * 6 * 5);
    auto pushGridVert = [&](float x, float y, float bx, float by, float bz) {
      grid.push_back(x);
      grid.push_back(y);
      grid.push_back(bx);
      grid.push_back(by);
      grid.push_back(bz);
    };
    for (int j = 0; j < CELLS; ++j) {
      for (int i = 0; i < CELLS; ++i) {
        float x0 = -HALF + i * CELL, x1 = x0 + CELL;
        float y0 = -HALF + j * CELL, y1 = y0 + CELL;
        pushGridVert(x0, y0, 1, 0, 0);
        pushGridVert(x1, y0, 0, 1, 0);
        pushGridVert(x0, y1, 0, 0, 1);
        pushGridVert(x1, y0, 1, 0, 0);
        pushGridVert(x1, y1, 0, 1, 0);
        pushGridVert(x0, y1, 0, 0, 1);
      }
    }
    planetCount = (GLsizei)(grid.size() / 5);

    glGenVertexArrays(1, &planetVAO);
    glGenBuffers(1, &planetVBO);
    glBindVertexArray(planetVAO);
    glBindBuffer(GL_ARRAY_BUFFER, planetVBO);
    glBufferData(GL_ARRAY_BUFFER, grid.size() * sizeof(float), grid.data(),
                 GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                          (void *)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                          (void *)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    // --- Player: 1 (x) * 1 (y) * 2 (z) cube, base at z=0 ---
    std::vector<float> cube; // x, y, z, nx, ny, nz per vertex
    auto tri = [&](glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 n) {
      for (glm::vec3 v : {a, b, c}) {
        cube.push_back(v.x);
        cube.push_back(v.y);
        cube.push_back(v.z);
        cube.push_back(n.x);
        cube.push_back(n.y);
        cube.push_back(n.z);
      }
    };
    const float X0 = -0.5f, X1 = 0.5f, Y0 = -0.5f, Y1 = 0.5f, Z0 = 0.0f,
                Z1 = 2.0f;
    // +X / -X
    tri({X1, Y0, Z0}, {X1, Y1, Z0}, {X1, Y1, Z1}, {1, 0, 0});
    tri({X1, Y0, Z0}, {X1, Y1, Z1}, {X1, Y0, Z1}, {1, 0, 0});
    tri({X0, Y0, Z0}, {X0, Y1, Z1}, {X0, Y1, Z0}, {-1, 0, 0});
    tri({X0, Y0, Z0}, {X0, Y0, Z1}, {X0, Y1, Z1}, {-1, 0, 0});
    // +Y / -Y
    tri({X0, Y1, Z0}, {X1, Y1, Z0}, {X1, Y1, Z1}, {0, 1, 0});
    tri({X0, Y1, Z0}, {X1, Y1, Z1}, {X0, Y1, Z1}, {0, 1, 0});
    tri({X0, Y0, Z0}, {X1, Y0, Z1}, {X1, Y0, Z0}, {0, -1, 0});
    tri({X0, Y0, Z0}, {X0, Y0, Z1}, {X1, Y0, Z1}, {0, -1, 0});
    // top (z=2) / bottom (z=0)
    tri({X0, Y0, Z1}, {X1, Y0, Z1}, {X1, Y1, Z1}, {0, 0, 1});
    tri({X0, Y0, Z1}, {X1, Y1, Z1}, {X0, Y1, Z1}, {0, 0, 1});
    tri({X0, Y0, Z0}, {X1, Y1, Z0}, {X1, Y0, Z0}, {0, 0, -1});
    tri({X0, Y0, Z0}, {X0, Y1, Z0}, {X1, Y1, Z0}, {0, 0, -1});
    playerCount = (GLsizei)(cube.size() / 6);

    glGenVertexArrays(1, &playerVAO);
    glGenBuffers(1, &playerVBO);
    glBindVertexArray(playerVAO);
    glBindBuffer(GL_ARRAY_BUFFER, playerVBO);
    glBufferData(GL_ARRAY_BUFFER, cube.size() * sizeof(float), cube.data(),
                 GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                          (void *)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                          (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    // --- Nose: small front box (+X local) marking the front ---
    std::vector<float> nose; // x, y, z, nx, ny, nz per vertex
    auto noseTri = [&](glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 n) {
      for (glm::vec3 v : {a, b, c}) {
        nose.push_back(v.x);
        nose.push_back(v.y);
        nose.push_back(v.z);
        nose.push_back(n.x);
        nose.push_back(n.y);
        nose.push_back(n.z);
      }
    };
    const float NX0 = 0.5f, NX1 = 0.75f, NY0 = -0.25f, NY1 = 0.25f,
                NZ0 = 1.1f, NZ1 = 1.6f;
    noseTri({NX1, NY0, NZ0}, {NX1, NY1, NZ0}, {NX1, NY1, NZ1}, {1, 0, 0});
    noseTri({NX1, NY0, NZ0}, {NX1, NY1, NZ1}, {NX1, NY0, NZ1}, {1, 0, 0});
    noseTri({NX0, NY0, NZ0}, {NX0, NY1, NZ1}, {NX0, NY1, NZ0}, {-1, 0, 0});
    noseTri({NX0, NY0, NZ0}, {NX0, NY0, NZ1}, {NX0, NY1, NZ1}, {-1, 0, 0});
    noseTri({NX0, NY1, NZ0}, {NX1, NY1, NZ0}, {NX1, NY1, NZ1}, {0, 1, 0});
    noseTri({NX0, NY1, NZ0}, {NX1, NY1, NZ1}, {NX0, NY1, NZ1}, {0, 1, 0});
    noseTri({NX0, NY0, NZ0}, {NX1, NY0, NZ1}, {NX1, NY0, NZ0}, {0, -1, 0});
    noseTri({NX0, NY0, NZ0}, {NX0, NY0, NZ1}, {NX1, NY0, NZ1}, {0, -1, 0});
    noseTri({NX0, NY0, NZ1}, {NX1, NY0, NZ1}, {NX1, NY1, NZ1}, {0, 0, 1});
    noseTri({NX0, NY0, NZ1}, {NX1, NY1, NZ1}, {NX0, NY1, NZ1}, {0, 0, 1});
    noseTri({NX0, NY0, NZ0}, {NX1, NY1, NZ0}, {NX1, NY0, NZ0}, {0, 0, -1});
    noseTri({NX0, NY0, NZ0}, {NX0, NY1, NZ0}, {NX1, NY1, NZ0}, {0, 0, -1});
    noseCount = (GLsizei)(nose.size() / 6);

    glGenVertexArrays(1, &noseVAO);
    glGenBuffers(1, &noseVBO);
    glBindVertexArray(noseVAO);
    glBindBuffer(GL_ARRAY_BUFFER, noseVBO);
    glBufferData(GL_ARRAY_BUFFER, nose.size() * sizeof(float), nose.data(),
                 GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                          (void *)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                          (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    // --- Projectile: small cube centered at the origin ---
    {
      std::vector<float> pc; // x, y, z, nx, ny, nz per vertex
      auto ptri = [&](glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 n) {
        for (glm::vec3 v : {a, b, c}) {
          pc.push_back(v.x);
          pc.push_back(v.y);
          pc.push_back(v.z);
          pc.push_back(n.x);
          pc.push_back(n.y);
          pc.push_back(n.z);
        }
      };
      const float H = kProjSize * 0.5f; // half side
      // +X / -X
      ptri({H, -H, -H}, {H, H, -H}, {H, H, H}, {1, 0, 0});
      ptri({H, -H, -H}, {H, H, H}, {H, -H, H}, {1, 0, 0});
      ptri({-H, -H, -H}, {-H, H, H}, {-H, H, -H}, {-1, 0, 0});
      ptri({-H, -H, -H}, {-H, -H, H}, {-H, H, H}, {-1, 0, 0});
      // +Y / -Y
      ptri({-H, H, -H}, {H, H, -H}, {H, H, H}, {0, 1, 0});
      ptri({-H, H, -H}, {H, H, H}, {-H, H, H}, {0, 1, 0});
      ptri({-H, -H, -H}, {H, -H, H}, {H, -H, -H}, {0, -1, 0});
      ptri({-H, -H, -H}, {-H, -H, H}, {H, -H, H}, {0, -1, 0});
      // +Z / -Z
      ptri({-H, -H, H}, {H, -H, H}, {H, H, H}, {0, 0, 1});
      ptri({-H, -H, H}, {H, H, H}, {-H, H, H}, {0, 0, 1});
      ptri({-H, -H, -H}, {H, H, -H}, {H, -H, -H}, {0, 0, -1});
      ptri({-H, -H, -H}, {-H, H, -H}, {H, H, -H}, {0, 0, -1});
      projCount = (GLsizei)(pc.size() / 6);

      glGenVertexArrays(1, &projVAO);
      glGenBuffers(1, &projVBO);
      glBindVertexArray(projVAO);
      glBindBuffer(GL_ARRAY_BUFFER, projVBO);
      glBufferData(GL_ARRAY_BUFFER, pc.size() * sizeof(float), pc.data(),
                   GL_STATIC_DRAW);
      glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                            (void *)0);
      glEnableVertexAttribArray(0);
      glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                            (void *)(3 * sizeof(float)));
      glEnableVertexAttribArray(1);
      glBindVertexArray(0);
    }

    planetProg = createProgram(planetVertSrc, planetFragSrc);
    playerProg = createProgram(playerVertSrc, playerFragSrc);
    textProg = createProgram(textVertSrc, textFragSrc);

    planetMvpLoc = glGetUniformLocation(planetProg, "mvp");
    playerPosLoc = glGetUniformLocation(planetProg, "playerPos");
    gridOffLoc = glGetUniformLocation(planetProg, "gridOffset");
    curveLoc = glGetUniformLocation(planetProg, "curveK");
    fillLoc = glGetUniformLocation(planetProg, "fill");
    edgeLoc = glGetUniformLocation(planetProg, "edgeColor");
    fogLoc = glGetUniformLocation(planetProg, "fogDensity");
    playerProjViewLoc = glGetUniformLocation(playerProg, "projView");
    playerModelLoc = glGetUniformLocation(playerProg, "model");
    playerPlayerPosLoc = glGetUniformLocation(playerProg, "playerPos");
    playerCurveLoc = glGetUniformLocation(playerProg, "curveK");
    colorLoc = glGetUniformLocation(playerProg, "color");
    textScreenSizeLoc = glGetUniformLocation(textProg, "screenSize");
    textColorLoc = glGetUniformLocation(textProg, "color");

    glGenVertexArrays(1, &textVAO);
    glGenBuffers(1, &textVBO);
    glBindVertexArray(textVAO);
    glBindBuffer(GL_ARRAY_BUFFER, textVBO);
    glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float),
                          (void *)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
    g_textDirty = true;

    std::cout << "Controls: mouse = rotate camera, wheel = zoom, WASD = move, "
                 "Q/E = fill, R/F = curve\n"
                 "Click = capture mouse, ESC = release mouse\n";

    g_lastTime = glfwGetTime();
    g_fpsLast = g_lastTime;
    g_fpsFrames = 0;

    // Browser loop: Emscripten takes over, frame() on every vsync.
    emscripten_set_main_loop(frame, 0, 1);

    // Not reached on web (the loop never returns), but kept for clarity.
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }
}
