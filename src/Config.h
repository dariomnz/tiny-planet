#pragma once

#include <cstddef>

// Game and world constants. Single source of truth to avoid
// magic numbers scattered across Game / Renderers.
namespace config {

constexpr float kCell = 2.0f;
constexpr int kCells = 60;

constexpr float kMoveSpeed = 8.0f;
constexpr float kFogDensity = 0.015f;

constexpr float kProjSpeed = 20.0f;  // units / second
constexpr float kProjLife = 2.0f;    // seconds until despawn
constexpr float kProjSize = 0.25f;   // cube side length
constexpr float kProjSpawnZ = 1.2f;  // spawn height (player chest)
constexpr float kProjForwardOffset = 0.8f;
constexpr std::size_t kProjMax = 100;

constexpr float kFillSpeed = 0.5f;
constexpr float kCurveSpeed = 0.05f;
constexpr float kCurveMax = 0.2f;

constexpr int kInitialFbW = 800;
constexpr int kInitialFbH = 600;

}  // namespace config
