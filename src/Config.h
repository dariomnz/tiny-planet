#pragma once

#include <cmath>
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
constexpr std::size_t kProjMax = 256;  // M1: fixed player bullet pool cap
constexpr float kProjRadius = 0.25f;   // 2D collision radius

// M1 — player base stats (mirrors docs/game-plan/03-upgrades.md).
constexpr float kHpBase = 100.0f;
constexpr float kDmgBase = 10.0f;
constexpr float kFireRateBase = 2.0f;  // shots / second (hold-click)
constexpr float kPlayerRadius = 0.5f;
constexpr float kPlayerInvulnSec = 0.5f;  // iframes after contact hit

// M1 — Chaser enemy (mirrors docs/game-plan/04-enemies-scaling.md).
constexpr std::size_t kEnemyCap = 256;
constexpr float kChaserHp = 20.0f;
constexpr float kChaserDamage = 10.0f;  // contact
constexpr float kChaserSpeed = 3.5f;
constexpr float kChaserRadius = 0.5f;

// M2 — XP / gems / draft (mirrors docs/game-plan/02-level-progression.md
// and 03-upgrades.md).
constexpr float kXpBase = 5.0f;
constexpr float kXpPow = 1.6f;
constexpr float kXpMult = 3.0f;
constexpr std::size_t kGemCap = 300;
constexpr float kGemMagnetRadius = 2.5f;  // base pickup radius (Boots scales it later)
constexpr float kGemMagnetSpeed = 10.0f;  // fly-to-player speed inside radius
constexpr float kGemDespawnSec = 20.0f;   // out-of-range gems despawn
constexpr float kGemCollectRadius = 0.7f;
constexpr int kGemValue = 1;  // normal gem (elite=5, boss=25 in M3/M5)

// M2 — draft upgrades (03-upgrades.md): +15% dmg, +12% fire rate (max 5),
// Nova 8-bullet ring every 6s, -0.5s CD per level.
constexpr int kUpgMaxLevel = 5;
constexpr float kUpgDamageMult = 1.15f;
constexpr float kUpgFireRateMult = 1.12f;
constexpr int kNovaBullets = 8;
constexpr float kNovaBaseCd = 6.0f;
constexpr float kNovaCdPerLevel = 0.5f;
constexpr float kNovaMinCd = 1.0f;

// XP needed to go from level n to n+1 (1-based). Floor to int steps.
inline int xpNeed(int level) {
    if (level < 1) level = 1;
    return static_cast<int>(std::floor(kXpBase + std::pow(static_cast<float>(level), kXpPow) * kXpMult));
}

// M3 — Director + bullet hell (mirrors docs/game-plan/04-enemies-scaling.md).
// t = run minutes (float).
constexpr std::size_t kEnemyBulletCap = 400;
constexpr float kEnemyBulletRadius = 0.3f;
constexpr float kEnemyBulletLife = 6.0f;  // s; range-culls slow bullets
constexpr float kEnemyBulletSpawnZ = 1.0f;
constexpr float kEnemyFireRange = 30.0f;  // off-screen enemies hold fire (perf + fairness)

// Per-type base stats (04 table). Contact = touch damage; bullet = shot damage.
constexpr float kSwarmHp = 1.0f;
constexpr float kSwarmDamage = 5.0f;
constexpr float kSwarmSpeed = 5.5f;
constexpr float kSwarmRadius = 0.35f;

constexpr float kShooterHp = 30.0f;
constexpr float kShooterBullet = 8.0f;
constexpr float kShooterSpeed = 2.5f;
constexpr float kShooterRadius = 0.5f;
constexpr float kShooterPreferDist = 12.0f;  // holds ~12u, approaches/retreats outside 10-14u
constexpr float kShooterFireCd = 2.5f;
constexpr float kShooterFireRange = 24.0f;

constexpr float kTankHp = 160.0f;
constexpr float kTankDamage = 20.0f;
constexpr float kTankSpeed = 1.8f;
constexpr float kTankRadius = 0.9f;

// Boss-5: HP bar + fan/ring patterns.
constexpr float kBoss5Hp = 800.0f;
constexpr float kBoss5Contact = 15.0f;
constexpr float kBoss5Bullet = 12.0f;
constexpr float kBoss5Speed = 2.2f;
constexpr float kBoss5Radius = 1.5f;
constexpr float kBoss5SpawnMin = 5.0f;
constexpr float kBoss5SpawnDist = 20.0f;
constexpr float kBossFanCd = 3.0f;
constexpr float kBossRingCd = 4.0f;
constexpr int kBossFanCount = 5;
constexpr int kBossRingCount = 12;
constexpr float kBossFanSpread = 0.3f;  // radians between fan bullets
constexpr int kBossGemValue = 25;
constexpr int kBossFragments = 15;

// Director rings/schedule (04): 25-35u ring, boss live => spawns at 30%.
constexpr float kSpawnRingMin = 25.0f;
constexpr float kSpawnRingMax = 35.0f;
constexpr int kSwarmGroupMin = 8;
constexpr int kSwarmGroupMax = 12;

// Scaling formulas (04). t = run minutes.
inline int targetAlive(float t) {
    if (t < 0.0f) t = 0.0f;
    return 6 + static_cast<int>(t * 4.0f);
}
inline float spawnInterval(float t) {
    if (t < 0.0f) t = 0.0f;
    const float v = 1.2f - t * 0.07f;
    return v < 0.25f ? 0.25f : v;
}
inline float bulletSpeed(float t) {
    if (t < 0.0f) t = 0.0f;
    const float v = 6.0f + t * 0.4f;
    return v > 14.0f ? 14.0f : v;
}

constexpr float kFillSpeed = 0.5f;
constexpr float kCurveSpeed = 0.05f;
constexpr float kCurveMax = 0.2f;

constexpr int kInitialFbW = 800;
constexpr int kInitialFbH = 600;

}  // namespace config
