#pragma once

#include <string>

#include "camera.h"
#include "graphics/EntityRenderer.h"
#include "graphics/Hud.h"
#include "graphics/PlanetRenderer.h"
#include "platform/Input.h"
#include "platform/Window.h"
#include "world/Player.h"
#include "world/Projectiles.h"

// Orchestrator: owns window, input, world and renderers.
// run() hands the loop to Emscripten (browser vsync).
class Game {
public:
  Game();
  void run();

private:
  void frame();
  void update(float dt);
  void render();
  static void tick();

  GlfwInit m_glfw;
  Window m_window;
  ThirdPersonCamera m_camera;
  InputManager m_input;
  Player m_player;
  ProjectileSystem m_projectiles;
  PlanetRenderer m_planet;
  EntityRenderer m_entities;
  HudRenderer m_hud;

  float m_fill = 0.0f;
  float m_curveK = 0.02f;
  double m_lastTime = 0.0;
  double m_fpsLast = 0.0;
  int m_fpsFrames = 0;
  double m_workMsSum = 0.0; // accumulated work time (no idle)
  std::string m_fpsText = "60 FPS";
  std::string m_msText = "0.0 MS";
};
