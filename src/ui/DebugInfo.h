#pragma once

#include <array>
#include <functional>

#include <glm/glm.hpp>

#include "ui/UiState.h"

// Snapshot of live game state for the debug panel. Filled once per frame
// by Game::buildDebugSnapshot(); plain data only (no pointers into pools),
// so the UI layer never touches world internals directly.
struct DebugSnapshot {
    // Player / derived combat stats (see Game::refreshDerived).
    glm::vec2 playerPos{0.0f, 0.0f};
    float playerYaw = 0.0f;
    float damage = 0.0f;
    float fireRate = 0.0f;
    float fireTimer = 0.0f;
    float bulletSpeed = 0.0f;
    float bulletRadius = 0.0f;
    float playerSpeed = 0.0f;
    float magnetRadius = 0.0f;
    float critCh = 0.0f;
    float novaTimer = 0.0f;
    float missileTimer = 0.0f;
    float orbAngle = 0.0f;
    int orbCount = 0;
    float invulnTimer = 0.0f;
    float fragAccum = 0.0f;
    bool reviveUsed = false;
    std::array<int, 10> upgLevels{0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    int xpNeed = 0;

    // Pools: counts, caps and composition.
    std::size_t enemyCount = 0;
    std::size_t enemyCap = 0;
    std::array<int, 6> enemyByType{0, 0, 0, 0, 0, 0};  // Chaser/Swarm/Shooter/Tank/Spinner/Boss
    int elitesCount = 0;
    std::size_t projCount = 0;
    std::size_t projCap = 0;
    int projHoming = 0;
    std::size_t enemyBulletCount = 0;
    std::size_t enemyBulletCap = 0;
    int enemyBulletHoming = 0;
    std::size_t gemCount = 0;
    std::size_t gemCap = 0;
    std::size_t edgeCount = 0;

    // Director / scaling at the current clock.
    float timeMin = 0.0f;
    int targetAlive = 0;
    float spawnInterval = 0.0f;
    float hpMult = 1.0f;
    float dmgMult = 1.0f;
    float enemyBulletSpeed = 0.0f;
    float spawnTimer = 0.0f;
    int bossesSpawned = 0;
    bool bossAlive = false;
    float ngHp = 1.0f;
    float ngDmg = 1.0f;
    float fragMult = 1.0f;

    // Camera / world rendering.
    float camYaw = 0.0f;
    float camPitch = 0.0f;
    float camDist = 0.0f;
    float camTargetDist = 0.0f;
    glm::vec3 camTarget{0.0f, 0.0f, 1.0f};
    glm::vec2 gridSnap{0.0f, 0.0f};

    // Input / platform.
    int pendingDrafts = 0;
    bool captured = false;
    bool firing = false;
    bool wantsMouse = false;
    bool wantsKeyboard = false;
    int fbW = 0;
    int fbH = 0;
    UiState state = UiState::Hub;
};

// Cheat/test actions owned by Game, invoked from the debug panel.
// godMode points at Game state so the checkbox edits it live.
struct DebugActions {
    bool *godMode = nullptr;
    std::function<void()> onHealFull;
    std::function<void()> onKillAll;
    std::function<void()> onClearEnemyBullets;
    std::function<void()> onGrantLevel;
    std::function<void()> onSpawnBoss;
    std::function<void()> onAddMinute;
    // typeIdx: 0=Chaser, 1=Swarm, 2=Shooter, 3=Tank, 4=Spinner; bossTier: 5/10/15.
    std::function<void(int)> onSpawnEnemy;
    std::function<void(int)> onSpawnBossTier;
};

// Short enum name for the Frame section ("Hub", "Run", ...).
inline const char *uiStateName(UiState s) {
    switch (s) {
        case UiState::Hub:
            return "Hub";
        case UiState::Run:
            return "Run";
        case UiState::Draft:
            return "Draft";
        case UiState::Paused:
            return "Paused";
        case UiState::GameOver:
            return "GameOver";
        default:
            return "?";
    }
}
