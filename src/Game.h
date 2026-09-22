#pragma once

#include <array>
#include <string>

#include "camera.h"
#include "game/Meta.h"
#include "graphics/EntityRenderer.h"
#include "graphics/PlanetRenderer.h"
#include "platform/Input.h"
#include "platform/Window.h"
#include "ui/ImGuiLayer.h"
#include "ui/UiState.h"
#include "world/Director.h"
#include "world/Enemies.h"
#include "world/EnemyBullets.h"
#include "world/Pickups.h"
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
    void gameOver(bool won = false);  // M5 wires won=true (final boss)
    void addXp(float v);  // M2: gem XP (with XP-hunger meta), chains level-ups
    void fireNova();      // M2: 8-bullet ring around the player
    void collideBulletsEnemies();
    void collideEnemiesPlayer(float dt);
    void collideEnemyBulletsPlayer();
    void collectGems();
    void refreshBossBar();  // M3: Boss-5 HP bar, -1 when no boss alive

    GlfwInit m_glfw;
    Window m_window;
    ThirdPersonCamera m_camera;
    InputManager m_input;
    Player m_player;
    ProjectileSystem m_projectiles;
    EnemySystem m_enemies;
    EnemyBulletSystem m_enemyBullets;
    Director m_director;
    GemSystem m_gems;
    PlanetRenderer m_planet;
    EntityRenderer m_entities;
    ImGuiLayer m_imgui;

    UiState m_state = UiState::Hub;
    Meta m_meta;
    RunStats m_run;
    std::array<DraftOption, 3> m_draft{};
    std::array<bool, 512> m_prevKeys{};

    float m_fill = 0.0f;
    float m_curveK = 0.02f;
    float m_fireTimer = 0.0f;   // hold-click pacing: 1 / fireRate
    float m_invulnTimer = 0.0f;  // iframes after a contact hit
    float m_damage = 10.0f;      // derived: base * 1.15^upgDmg * meta
    float m_fireRate = 2.0f;     // derived: base * 1.12^upgFire
    int m_upgDamage = 0;         // M2 draft levels (max 5 each)
    int m_upgFire = 0;
    int m_upgNova = 0;
    float m_fragAccum = 0.0f;  // M4: fractional kill rewards (0.1/kill)
    float m_novaTimer = 0.0f;  // counts down to the next nova ring
    double m_lastTime = 0.0;
    double m_fpsLast = 0.0;
    int m_fpsFrames = 0;
    double m_workMsSum = 0.0;  // accumulated work time (no idle)
};
