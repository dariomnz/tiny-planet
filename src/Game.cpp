#include "Game.h"

#include <GLES3/gl3.h>
#include <GLFW/glfw3.h>
#include <emscripten/emscripten.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <iostream>

#include <glm/gtc/matrix_transform.hpp>

#include "Config.h"
#include "platform/WebExt.h"

namespace {
Game *s_game = nullptr;
} // namespace

Game::Game()
    : m_window(config::kInitialFbW, config::kInitialFbH, "Planet 3D"),
      m_input(m_window.handle(), m_camera) {
  m_input.installCallbacks();
  web::fitCanvasToWindow(m_window);
  web::installResizeHandler(m_window);
  web::installPointerLockHandlers(m_window, m_input);

  glEnable(GL_DEPTH_TEST);

  {
    const char *ver = (const char *)glGetString(GL_VERSION);
    const char *sl = (const char *)glGetString(GL_SHADING_LANGUAGE_VERSION);
    std::cout << "GL_VERSION: " << (ver ? ver : "<null>") << std::endl;
    std::cout << "GLSL_VERSION: " << (sl ? sl : "<null>") << std::endl;
  }

  std::cout << "Controls: mouse = rotate camera, wheel = zoom, WASD = move, "
               "Q/E = fill, R/F = curve\n"
               "Click = capture mouse, ESC = release mouse\n";

  m_hud.setText(m_fpsText);
  m_lastTime = glfwGetTime();
  m_fpsLast = m_lastTime;
}

void Game::run() {
  s_game = this;
  // Browser loop: Emscripten takes over, frame() on every vsync.
  emscripten_set_main_loop(&Game::tick, 0, 1);
}

void Game::tick() {
  if (s_game)
    s_game->frame();
}

void Game::frame() {
  // t0 marks the start of real work; t1 the end (before swap,
  // which on web only queues the present). The difference is CPU
  // time in update+render, WITHOUT the browser vsync idle wait.
  const double t0 = glfwGetTime();
  m_window.pollEvents();

  const double now = glfwGetTime();
  float dt = static_cast<float>(now - m_lastTime);
  m_lastTime = now;
  // Avoid large jumps when switching tabs / first frame.
  dt = std::clamp(dt, 0.0f, 0.05f);

  update(dt);
  render();
  m_workMsSum += (glfwGetTime() - t0) * 1000.0;
  m_window.swapBuffers();
}

void Game::update(float dt) {
  GLFWwindow *win = m_window.handle();

  // WASD relative to the camera yaw.
  const glm::vec2 fwd(std::cos(m_camera.yaw), std::sin(m_camera.yaw));
  const glm::vec2 right(fwd.y, -fwd.x);
  if (glfwGetKey(win, GLFW_KEY_W) == GLFW_PRESS)
    m_player.addDisplacement(fwd * config::kMoveSpeed * dt);
  if (glfwGetKey(win, GLFW_KEY_S) == GLFW_PRESS)
    m_player.addDisplacement(-fwd * config::kMoveSpeed * dt);
  if (glfwGetKey(win, GLFW_KEY_D) == GLFW_PRESS)
    m_player.addDisplacement(right * config::kMoveSpeed * dt);
  if (glfwGetKey(win, GLFW_KEY_A) == GLFW_PRESS)
    m_player.addDisplacement(-right * config::kMoveSpeed * dt);
  if (glfwGetKey(win, GLFW_KEY_Q) == GLFW_PRESS)
    m_fill -= dt * config::kFillSpeed;
  if (glfwGetKey(win, GLFW_KEY_E) == GLFW_PRESS)
    m_fill += dt * config::kFillSpeed;
  m_fill = std::clamp(m_fill, 0.0f, 1.0f);
  if (glfwGetKey(win, GLFW_KEY_R) == GLFW_PRESS)
    m_curveK -= dt * config::kCurveSpeed;
  if (glfwGetKey(win, GLFW_KEY_F) == GLFW_PRESS)
    m_curveK += dt * config::kCurveSpeed;
  m_curveK = std::clamp(m_curveK, 0.0f, config::kCurveMax);

  m_player.updateYaw(m_camera.yaw, dt);

  if (m_input.consumeFireRequest())
    m_projectiles.spawn(m_player.pos(), m_camera.yaw);
  m_projectiles.update(dt);

  m_camera.update(dt, m_player.pos());

  // FPS (wall-clock, includes vsync) + average WORK ms per frame
  // (update+render only, no idle). Same 0.5 s window, dirty at 2 Hz.
  const double now = glfwGetTime();
  ++m_fpsFrames;
  if (now - m_fpsLast >= 0.5) {
    const double elapsed = now - m_fpsLast;
    int fps = static_cast<int>(m_fpsFrames / elapsed);
    float ms = static_cast<float>(m_workMsSum / m_fpsFrames);
    m_fpsFrames = 0;
    m_workMsSum = 0.0;
    m_fpsLast = now;
    fps = std::clamp(fps, 0, 999);
    ms = std::clamp(ms, 0.0f, 999.9f);
    m_fpsText = std::to_string(fps) + " FPS";
  m_hud.setText(m_fpsText);
  m_hud.setSubText(m_msText);
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%.1f MS", static_cast<double>(ms));
    m_msText = buf;
    m_hud.setSubText(m_msText);
  }
}

void Game::render() {
  const int fbW = m_window.fbWidth();
  const int fbH = m_window.fbHeight();

  const glm::mat4 view = m_camera.getView();
  const float aspect =
      static_cast<float>(fbW) / static_cast<float>(fbH > 0 ? fbH : 1);
  const glm::mat4 proj =
      glm::perspective(glm::radians(60.0f), aspect, 0.1f, 500.0f);

  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // Planet (grid re-centered under the player with per-cell snap).
  const float snapX =
      std::floor(m_player.pos().x / config::kCell + 0.5f) * config::kCell;
  const float snapY =
      std::floor(m_player.pos().y / config::kCell + 0.5f) * config::kCell;
  m_planet.draw(proj * view, m_player.pos(), {snapX, snapY}, m_curveK, m_fill,
               {0.2f, 1.0f, 0.4f}, config::kFogDensity);

  // Player + nose + projectiles (same curved shader).
  m_entities.begin(proj * view, m_player.pos(), m_curveK);
  glm::mat4 model =
      glm::translate(glm::mat4(1.0f),
                     glm::vec3(m_player.pos().x, m_player.pos().y, 0.0f));
  model = glm::rotate(model, m_player.yaw(), glm::vec3(0.0f, 0.0f, 1.0f));
  m_entities.drawPlayer(model, {1.0f, 0.25f, 0.2f});
  m_entities.drawNose(model, {1.0f, 0.85f, 0.2f});
  m_entities.drawProjectiles(m_projectiles.list(), {0.3f, 0.8f, 1.0f});

  m_hud.draw(fbW, fbH, {0.2f, 1.0f, 0.3f});
}
