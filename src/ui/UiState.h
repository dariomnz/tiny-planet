#pragma once

#include <string>

// UI state machine: Hub <-> Run (+ Draft / Paused / GameOver overlays).
// Permanent meta lives in game/Meta.h (localStorage persistence, M4).
enum class UiState { Hub, Run, Draft, Paused, GameOver };

// Per-run stats shown in the ImGui HUD. M2: xp/level are real (gems ->
// addXp -> xpNeed); xp01 is the derived bar fraction xp/xpNeed(level).
// M4: fragment breakdown (time/kills/boss/draft) filled at run end.
struct RunStats {
    float hp = 100.0f;
    float maxHp = 100.0f;
    float timerSec = 0.0f;
    int level = 1;
    float xp = 0.0f;
    float xp01 = 0.0f;
    float bossHp01 = -1.0f;  // M3: Boss HP bar, negative = hidden
    int bossTier = 0;        // M5: 5/10/15 while a boss lives, else 0
    int kills = 0;
    int bossKills = 0;
    int elitesKilled = 0;  // M5
    int fragsTime = 0;
    int fragsKills = 0;
    int fragsBoss = 0;
    int fragsDraft = 0;
    int fragsElite = 0;    // M5: +2 per elite
    int fragsVictory = 0;  // M5: +100 final-boss bonus
    bool won = false;      // M5: final boss killed
    int fragmentsEarned = 0;  // total = time + kills + boss + draft + victory
    int fps = 60;
    float workMs = 0.0f;
};

// One draft choice (1 of 3). id selects the effect in Game::applyDraft:
// 0 = damage, 1 = fire rate, 2 = nova, 3 = fallback (heal + fragments).
struct DraftOption {
    std::string name;
    std::string desc;
    int id = 0;
};
