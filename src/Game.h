#pragma once

#include <array>
#include <string>

#include "camera.h"
#include "graphics/EntityRenderer.h"
#include "graphics/PlanetRenderer.h"
#include "platform/Input.h"
#include "platform/Window.h"
#include "ui/ImGuiLayer.h"
#include "ui/UiState.h"
#include "world/Enemies.h"
#include "world/Player.h"
#include "world/Projectiles.h"

// Orchestrator: owns window, input, world and renderers.
// run() hands the loop to Emscripten (browser vsync).
class Game {
   public:
    Game();
    ~Game();
    void run();

   private:
    void frame();
    void update(float dt);
    void renderScene();
    void drawUi();
    static void tick();

    void startRun();
    void quitToHub();
    void openDraft();
    void applyDraft(int idx);
    void togglePause();
    void gameOver();
    void collideBulletsEnemies();
    void collideEnemiesPlayer(float dt);

    GlfwInit m_glfw;
    Window m_window;
    ThirdPersonCamera m_camera;
    InputManager m_input;
    Player m_player;
    ProjectileSystem m_projectiles;
    EnemySystem m_enemies;
    PlanetRenderer m_planet;
    EntityRenderer m_entities;
    ImGuiLayer m_imgui;

    UiState m_state = UiState::Hub;
    MetaState m_meta;
    RunStats m_run;
    std::array<DraftOption, 3> m_draft{};
    std::array<bool, 512> m_prevKeys{};

    float m_fill = 0.0f;
    float m_curveK = 0.02f;
    float m_fireTimer = 0.0f;   // hold-click pacing: 1 / fireRate
    float m_invulnTimer = 0.0f;  // iframes after a contact hit
    float m_damage = 10.0f;      // per-run basic damage (M2: upgrades)
    float m_fireRate = 2.0f;     // per-run shots/sec (M2: upgrades)
    double m_lastTime = 0.0;
    double m_fpsLast = 0.0;
    int m_fpsFrames = 0;
    double m_workMsSum = 0.0;  // accumulated work time (no idle)
};
