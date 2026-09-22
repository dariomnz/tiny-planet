#pragma once

#include <array>
#include <string>

// UI state machine: Hub <-> Run (+ Draft / Paused / GameOver overlays).
// All in-memory for now (no LocalStorage persistence, phase 2).
enum class UiState { Hub, Run, Draft, Paused, GameOver };

// Permanent meta upgrades (mirrors docs/game-plan/03-upgrades.md, in-memory).
struct MetaState {
    int fragments = 120;
    // levels per meta upgrade, same order as UiScreens::kMetaNames.
    std::array<int, 6> levels{0, 0, 0, 0, 0, 0};
};

// Per-run stats shown in the ImGui HUD. Mock values until game/Run exists.
struct RunStats {
    float hp = 100.0f;
    float maxHp = 100.0f;
    float timerSec = 0.0f;
    int level = 1;
    float xp01 = 0.0f;
    int fragmentsEarned = 0;
    int fps = 60;
    float workMs = 0.0f;
};

// One draft choice (1 of 3). Name/desc only; effect applied as mock.
struct DraftOption {
    std::string name;
    std::string desc;
};
