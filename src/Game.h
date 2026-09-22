#pragma once

#include <array>
#include <glm/glm.hpp>
#include <string>

#include "camera.h"
#include "game/Meta.h"
#include "graphics/EntityRenderer.h"
#include "graphics/PlanetRenderer.h"
#include "platform/Input.h"
#include "platform/Window.h"
#include "ui/ImGuiLayer.h"
#include "ui/UiScreens.h"
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
    void fireBasic();     // M5: spread shot (extra projectiles)
    void fireMissiles();  // M5: homing volley at the nearest enemy
    void steerMissiles(float dt);
    void updateOrbitals(float dt);  // M5: spin + collide + render positions
    // M5: kill rewards (gems/frags/victory). False when the run ended.
    bool onEnemyKilled(std::size_t ei);
    void refreshDerived();  // M5: recompute stats from upgrade levels + meta
    float rand01() noexcept;  // M5: deterministic run RNG (crit, draft)
    void collideBulletsEnemies();
    void collideEnemiesPlayer(float dt);
    void collideEnemyBulletsPlayer();
    void collectGems();
    void refreshBossBar();  // M3: Boss HP bar, -1 when no boss alive
    void refreshEdgeMarkers();  // M5: off-screen indicators from m_projView
    DebugSnapshot buildDebugSnapshot() const;  // one snapshot per frame for the debug panel
    DebugActions debugActions();               // cheat callbacks bound to this run
    void healFull();
    void killAllNonBoss();
    void clearEnemyBullets();
    void grantLevel();
    void spawnBoss5();
    void spawnBoss(int tier);
    void spawnEnemy(int typeIdx);
    void addMinute();

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
    int m_upgDamage = 0;         // M2/M5 draft levels (max 5 each)
    int m_upgFire = 0;
    int m_upgNova = 0;
    int m_upgExtra = 0;  // M5: +1 shot (max 3), then +10% damage
    int m_upgHeavy = 0;  // M5: +10% bullet speed/size
    int m_upgOrb = 0;    // M5: orbitals (skill)
    int m_upgMis = 0;    // M5: auto-missiles (skill)
    int m_upgBoots = 0;  // M5: +8% speed, +30% magnet
    int m_upgVit = 0;    // M5: +20 maxHP/heal, regen at lv 3+
    int m_upgCrit = 0;   // M5: +5% x2 crit
    float m_fragAccum = 0.0f;  // M4: fractional kill rewards (0.1/kill)
    float m_novaTimer = 0.0f;  // counts down to the next nova ring
    // M5 derived stats (refreshDerived).
    float m_bulletSpeed = 20.0f;
    float m_bulletRadius = 0.25f;
    float m_playerSpeed = 8.0f;
    float m_magnetRadius = 2.5f;
    float m_critCh = 0.0f;
    float m_missileTimer = 0.0f;
    float m_orbAngle = 0.0f;
    float m_orbCd[3] = {0.0f, 0.0f, 0.0f};
    glm::vec2 m_orbPos[3]{};
    int m_orbCount = 0;
    bool m_reviveUsed = false;  // M5: meta revive, once per run
    bool m_godMode = false;     // debug cheat: no contact/bullet damage
    int m_pendingDrafts = 0;    // queued level-ups while Draft is open (debug cheat)
    unsigned m_rng = 0x9E3779B9u;
    float m_ngHp = 1.0f;    // M5: NG+ mults (1.5^n / 1.2^n / 1.5^n)
    float m_ngDmg = 1.0f;
    float m_fragMult = 1.0f;
    // M5: edge arrows (projView stored in renderScene, drawn in drawUi).
    glm::mat4 m_projView{1.0f};
    std::array<ui::EdgeMarker, 64> m_edge{};
    std::size_t m_edgeCount = 0;
    double m_lastTime = 0.0;
    double m_fpsLast = 0.0;
    int m_fpsFrames = 0;
    double m_workMsSum = 0.0;  // accumulated work time (no idle)
};
