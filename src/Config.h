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

// Boss tiers (04 table: 800/2500/6000). Shared body, per-tier patterns.
constexpr float kBossHp[3] = {800.0f, 2500.0f, 6000.0f};
constexpr int kBossFrags[3] = {15, 40, 100};  // 02 economy boss tiers
constexpr float kBossSpawnMin[3] = {5.0f, 10.0f, 15.0f};
constexpr float kBossContact = 15.0f;
constexpr float kBossBullet = 12.0f;
constexpr float kBossSpeed = 2.2f;
constexpr float kBossRadius = 1.5f;
constexpr float kBossSpawnDist = 20.0f;
constexpr int kBossFanCount = 5;
constexpr int kBossRingCount = 12;
constexpr float kBossFanSpread = 0.3f;  // radians between fan bullets
constexpr int kBossGemValue = 25;
constexpr float kBoss10FanCd = 4.0f;  // Boss-10: 5-fan + 12-ring every 4s
constexpr float kBoss10RingCd = 4.0f;
constexpr float kBoss15SpiralCd = 0.15f;  // Boss-15: double spiral tick
constexpr float kBossMissileCd = 3.0f;    // Boss-15: slow homing missiles
constexpr float kBossMissileSpeed = 4.5f;
constexpr float kBossMissileTurn = 1.5f;  // rad/s homing
constexpr int kVictoryFrags = 100;        // +100 on final-boss kill (02)

// M4 — fragment economy (docs/game-plan/02-level-progression.md:
// time 2/min, kills 0.1/kill; bosses/elites/victory tiers in M4/M5).
constexpr float kFragPerMin = 2.0f;
constexpr float kFragPerKill = 0.1f;
constexpr int kDraftFallbackFrags = 10;

// M5 — Spinner + elites + full upgrade set (04/03 tables).
constexpr float kSpinnerHp = 300.0f;
constexpr float kSpinnerBullet = 10.0f;
constexpr float kSpinnerSpeed = 2.0f;
constexpr float kSpinnerRadius = 0.7f;
constexpr float kSpinnerFireCd = 0.12f;  // one volley per arm set
constexpr float kSpinnerTurn = 24.0f;    // rad/s: 0.4/frame @60fps (04)
constexpr int kSpinnerArms = 2;
constexpr int kSpinnerArmsLate = 3;      // 3 arms from min 8 (04)
constexpr float kSpinnerLateMin = 8.0f;

constexpr float kEliteMin = 5.0f;     // elites from min 5 (04)
constexpr float kEliteChance = 0.05f;  // 5% of spawns
constexpr float kEliteHpMult = 6.0f;
constexpr int kEliteGem = 5;    // x5 XP (02)
constexpr int kEliteFrags = 2;  // +2 fragments (04)

// M5 — player upgrade systems (03-upgrades.md, max 5 unless noted).
constexpr float kUpgHeavyMult = 1.1f;    // +10% bullet speed/size per level
constexpr int kUpgExtraMaxShots = 3;     // +1 projectile, then +10% damage
constexpr float kUpgExtraOverflow = 1.1f;
constexpr float kUpgBootsSpeed = 1.08f;   // +8% move speed per level
constexpr float kUpgBootsMagnet = 1.3f;   // +30% pickup radius per level
constexpr float kUpgVitalityHp = 20.0f;   // +20 max HP and heal 20
constexpr float kUpgVitalityRegen = 0.5f;  // HP/s at lv 3+
constexpr float kUpgCritChance = 0.05f;    // +5% crit chance (x2) per level
constexpr float kMissileCd = 3.0f;         // auto-missile cadence
constexpr float kMissileSpeed = 16.0f;
constexpr float kMissileTurn = 3.0f;      // rad/s homing
constexpr float kMissileAcquire = 30.0f;  // lock-on range
constexpr float kOrbRadius = 2.0f;        // orbital ring radius
constexpr float kOrbSpeed = 2.5f;         // rad/s spin
constexpr float kOrbHitCd = 0.35f;        // per-orb hit cooldown
constexpr float kOrbRadiusHit = 0.3f;
constexpr float kSkillDmgStep = 1.5f;  // orbital/missile damage step on even levels

// M5 — NG+ (06-difficulty): stacks per win.
constexpr float kNgHpMult = 1.5f;
constexpr float kNgDmgMult = 1.2f;
constexpr float kNgFragMult = 1.5f;

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
// M5 tuning (04): slow enemy damage growth, steep HP growth (min 15 ~= x12).
inline float hpMult(float t) {
    if (t < 0.0f) t = 0.0f;
    return (1.0f + t * 0.35f) * std::pow(1.13f, t);
}
inline float dmgMult(float t) {
    if (t < 0.0f) t = 0.0f;
    return 1.0f + t * 0.12f;
}

constexpr float kFillSpeed = 0.5f;
constexpr float kCurveSpeed = 0.05f;
constexpr float kCurveMax = 0.2f;

constexpr int kInitialFbW = 800;
constexpr int kInitialFbH = 600;

}  // namespace config
