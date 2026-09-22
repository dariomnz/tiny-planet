#pragma once

#include <array>
#include <functional>

#include "game/Meta.h"
#include "ui/UiState.h"

// All ImGui windows for Hub + in-run UI. Stateless free functions driven
// by Game (which owns Meta / RunStats / UiState).
namespace ui {

// Meta catalog shown in the Hub (names + descriptions + max levels).
inline constexpr int kMetaCount = 6;
const char *metaName(int i);
const char *metaDesc(int i);
int metaMax(int i);

void drawHud(const RunStats &run, float &curveK, float &fill, bool mouseCaptured = true);
// onBuy fires after each purchase so Game can save (never per frame).
void drawHub(Meta &meta, const std::function<void()> &onStart, const std::function<void()> &onBuy);
void drawDraft(const std::array<DraftOption, 3> &opts, const std::function<void(int)> &onPick);
void drawPause(const std::function<void()> &onResume, const std::function<void()> &onQuit);
void drawGameOver(const RunStats &run, const std::function<void()> &onRetry, const std::function<void()> &onHub);

}  // namespace ui
