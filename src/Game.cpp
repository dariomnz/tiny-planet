#include "Game.h"

#include <GLES3/gl3.h>
#include <GLFW/glfw3.h>
#include <imgui.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

#include "Config.h"
#include "platform/WebExt.h"
#include "ui/UiScreens.h"

namespace {
Game *s_game = nullptr;

// M3: per-type enemy colors (04 table). Boss reads white/red.
glm::vec3 enemyColor(EnemyType t) {
    switch (t) {
        case EnemyType::Swarm:
            return {1.0f, 0.55f, 0.1f};
        case EnemyType::Shooter:
            return {0.7f, 0.3f, 1.0f};
        case EnemyType::Tank:
            return {0.15f, 0.5f, 0.25f};
        case EnemyType::Spinner:  // M5: yellow elite
            return {1.0f, 0.85f, 0.2f};
        case EnemyType::Boss:
            return {1.0f, 0.9f, 0.9f};
        case EnemyType::Chaser:
        default:
            return {1.0f, 0.15f, 0.15f};
    }
}
}  // namespace

Game::Game() : m_window(config::kInitialFbW, config::kInitialFbH, "Planet 3D"), m_input(m_window.handle(), m_camera) {
    // Order matters: ImGui backend chains to these callbacks.
    m_input.installCallbacks();
    web::fitCanvasToWindow(m_window);
    web::installResizeHandler(m_window);
    web::installPointerLockHandlers(m_window, m_input);
    m_imgui.init(m_window.handle());
    // Hub starts with a normal cursor (installCallbacks already left NORMAL);
    // Run requests the lock via InputManager::requestCapture().

    glEnable(GL_DEPTH_TEST);

    {
        const char *ver = (const char *)glGetString(GL_VERSION);
        const char *sl = (const char *)glGetString(GL_SHADING_LANGUAGE_VERSION);
        std::cout << "GL_VERSION: " << (ver ? ver : "<null>") << std::endl;
        std::cout << "GLSL_VERSION: " << (sl ? sl : "<null>") << std::endl;
    }

    std::cout << "Controls: Hub = mouse, WASD = move, hold-click = fire, "
                 "P = pause, L = level, K = die\n";

    m_meta = Meta::load();  // M4: localStorage, defaults on first visit
    std::cout << "Meta: " << m_meta.fragments << " fragments, best "
              << m_meta.bestTimeSec << "s, " << m_meta.wins << " wins" << std::endl;

    m_prevKeys.fill(false);
    m_lastTime = glfwGetTime();
    m_fpsLast = m_lastTime;
}

Game::~Game() { m_imgui.shutdown(); }

void Game::run() {
    s_game = this;
    // Browser loop: Emscripten takes over, frame() on every vsync.
    web::enterMainLoop(&Game::tick);
}

void Game::tick() {
    if (s_game) s_game->frame();
}

void Game::startRun() {
    m_player = Player{};
    m_projectiles = ProjectileSystem{};
    m_enemies.clear();
    m_enemyBullets.clear();
    m_director.clear();
    m_gems.clear();
    m_run = RunStats{};
    m_run.maxHp = config::kHpBase + 10.0f * m_meta.levels[3];
    m_run.hp = m_run.maxHp;
    m_upgDamage = 0;
    m_upgFire = 0;
    m_upgNova = 0;
    m_upgExtra = 0;
    m_upgHeavy = 0;
    m_upgOrb = 0;
    m_upgMis = 0;
    m_upgBoots = 0;
    m_upgVit = 0;
    m_upgCrit = 0;
    m_fragAccum = 0.0f;
    m_reviveUsed = false;
    m_pendingDrafts = 0;
    m_rng = 0x9E3779B9u;
    // M5: NG+ mults (06: hp x1.5, dmg x1.2, frags x1.5 per win, stacking).
    m_ngHp = std::pow(config::kNgHpMult, m_meta.ngPlus);
    m_ngDmg = std::pow(config::kNgDmgMult, m_meta.ngPlus);
    m_fragMult = std::pow(config::kNgFragMult, m_meta.ngPlus);
    refreshDerived();
    m_fireTimer = 0.0f;
    m_novaTimer = 0.0f;
    m_missileTimer = 0.0f;
    m_orbAngle = 0.0f;
    m_orbCount = 0;
    m_orbCd[0] = m_orbCd[1] = m_orbCd[2] = 0.0f;
    m_invulnTimer = 0.0f;
    m_state = UiState::Run;
    // Called from a UI gesture (button/Enter): piggybacks its transient
    // activation so the lock often engages without an extra click.
    m_input.requestCapture();
    m_lastTime = glfwGetTime();
}

void Game::quitToHub() {
    m_state = UiState::Hub;
    m_input.releaseCapture();
}

float Game::rand01() noexcept {
    m_rng ^= m_rng << 13;
    m_rng ^= m_rng >> 17;
    m_rng ^= m_rng << 5;
    return static_cast<float>(m_rng >> 8) * (1.0f / 16777216.0f);
}

void Game::refreshDerived() {
    // M5: every derived stat recomputed from upgrade levels + meta (03).
    m_damage = config::kDmgBase * std::pow(config::kUpgDamageMult, m_upgDamage) *
               std::pow(config::kUpgExtraOverflow, std::max(0, m_upgExtra - config::kUpgExtraMaxShots)) *
               (1.0f + 0.02f * m_meta.levels[0]);
    m_fireRate = config::kFireRateBase * std::pow(config::kUpgFireRateMult, m_upgFire);
    m_bulletSpeed = config::kProjSpeed * std::pow(config::kUpgHeavyMult, m_upgHeavy);
    m_bulletRadius = config::kProjRadius * std::pow(config::kUpgHeavyMult, m_upgHeavy);
    m_playerSpeed = config::kMoveSpeed * std::pow(config::kUpgBootsSpeed, m_upgBoots) *
                    (1.0f + 0.01f * m_meta.levels[2]);  // Haste meta (was unapplied)
    m_magnetRadius = config::kGemMagnetRadius * std::pow(config::kUpgBootsMagnet, m_upgBoots);
    m_critCh = config::kUpgCritChance * m_upgCrit;
    m_orbCount = m_upgOrb <= 0 ? 0 : std::min(3, (m_upgOrb + 1) / 2);
}

void Game::openDraft() {
    // M5: full 10-upgrade draft (03-upgrades.md). Never offer maxed upgrades
    // or locked skills; anti-snowball: >=3 offensive levels weights
    // defensive/utility x1.5. Fallback fills empty slots.
    auto level = [&](int id) {
        switch (id) {
            case 0:
                return m_upgDamage;
            case 1:
                return m_upgFire;
            case 2:
                return m_upgNova;
            case 3:
                return m_upgExtra;
            case 4:
                return m_upgHeavy;
            case 5:
                return m_upgOrb;
            case 6:
                return m_upgMis;
            case 7:
                return m_upgBoots;
            case 8:
                return m_upgVit;
            case 9:
                return m_upgCrit;
            default:
                return config::kUpgMaxLevel;
        }
    };
    // Skills need a slot: without the slot2 meta only 1 skill per run.
    const int skillsOwned =
        (m_upgNova > 0 ? 1 : 0) + (m_upgOrb > 0 ? 1 : 0) + (m_upgMis > 0 ? 1 : 0);
    const int maxSkills = m_meta.levels[4] > 0 ? 2 : 1;
    // Offensive weight rule (03 anti-snowball).
    const int offenseLv = m_upgDamage + m_upgFire + m_upgExtra + m_upgHeavy + m_upgCrit;
    // Stack candidate table: no heap allocs.
    const char *names[10] = {"Basic damage", "Fire rate",     "Nova",       "Extra projectile",
                             "Heavy bullets", "Orbitals",      "Auto missiles", "Boots+Magnet",
                             "Vitality",      "Critical"};
    const char *fmts[10] = {"+15%% damage (lv %d)",     "+12%% fire rate (lv %d)", "8-bullet ring (lv %d)",
                            "+1 shot (lv %d)",          "+10%% speed+size (lv %d)", "spinning orb (lv %d)",
                            "homing volley (lv %d)",    "+8%% speed +30%% magnet (lv %d)",
                            "+20 maxHP, heal 20 (lv %d)", "+5%% x2 crit (lv %d)"};
    int cand[10];
    float weight[10];
    int nCand = 0;
    for (int id = 0; id < 10; ++id) {
        if (level(id) >= config::kUpgMaxLevel) continue;  // never offer maxed
        const bool isSkill = (id == 2 || id == 5 || id == 6);
        if (isSkill && level(id) == 0 && skillsOwned >= maxSkills) continue;  // locked skill
        float w = 1.0f;
        if (offenseLv >= 3 && (id == 7 || id == 8)) w = 1.5f;  // defensive/utility x1.5
        cand[nCand] = id;
        weight[nCand] = w;
        ++nCand;
    }
    char descs[3][96];
    int filled = 0;
    for (int slot = 0; slot < 3; ++slot) {
        float total = 0.0f;
        for (int i = 0; i < nCand; ++i) total += weight[i];
        if (total <= 0.0f) break;
        float r = rand01() * total;
        int pick = 0;
        while (pick < nCand - 1 && r >= weight[pick]) {
            r -= weight[pick];
            ++pick;
        }
        const int id = cand[pick];
        weight[pick] = 0.0f;  // distinct picks
        snprintf(descs[slot], sizeof(descs[slot]), fmts[id], level(id) + 1);
        m_draft[static_cast<std::size_t>(slot)] = {names[id], descs[slot], id};
        ++filled;
    }
    // Fallback fills any remaining slots (all three when fully maxed).
    for (; filled < 3; ++filled)
        m_draft[static_cast<std::size_t>(filled)] = {"Overcharge", "Heal 30% + 10 Fragments", 10};
    m_state = UiState::Draft;
    m_input.releaseCapture();
}

void Game::applyDraft(int idx) {
    // M5: ids 0-9 = the 10 upgrades (03), 10 = fallback. openDraft never
    // offers a maxed/locked upgrade, so the switch trusts the id.
    const int id = (idx >= 0 && idx < 3) ? m_draft[static_cast<std::size_t>(idx)].id : 10;
    if (id >= 0 && id <= 9) {
        switch (id) {
            case 0:
                ++m_upgDamage;
                break;
            case 1:
                ++m_upgFire;
                break;
            case 2:
                ++m_upgNova;
                m_novaTimer = std::min(m_novaTimer, 1.0f);  // first ring comes out quickly
                break;
            case 3:
                ++m_upgExtra;
                break;
            case 4:
                ++m_upgHeavy;
                break;
            case 5:
                ++m_upgOrb;
                break;
            case 6:
                ++m_upgMis;
                m_missileTimer = std::min(m_missileTimer, 1.0f);
                break;
            case 7:
                ++m_upgBoots;
                break;
            case 8:
                ++m_upgVit;
                m_run.maxHp += config::kUpgVitalityHp;
                m_run.hp = std::min(m_run.maxHp, m_run.hp + config::kUpgVitalityHp);
                break;
            case 9:
                ++m_upgCrit;
                break;
            default:
                break;
        }
        if (m_upgDamage > config::kUpgMaxLevel) m_upgDamage = config::kUpgMaxLevel;
        if (m_upgFire > config::kUpgMaxLevel) m_upgFire = config::kUpgMaxLevel;
        if (m_upgNova > config::kUpgMaxLevel) m_upgNova = config::kUpgMaxLevel;
        if (m_upgExtra > config::kUpgMaxLevel) m_upgExtra = config::kUpgMaxLevel;
        if (m_upgHeavy > config::kUpgMaxLevel) m_upgHeavy = config::kUpgMaxLevel;
        if (m_upgOrb > config::kUpgMaxLevel) m_upgOrb = config::kUpgMaxLevel;
        if (m_upgMis > config::kUpgMaxLevel) m_upgMis = config::kUpgMaxLevel;
        if (m_upgBoots > config::kUpgMaxLevel) m_upgBoots = config::kUpgMaxLevel;
        if (m_upgVit > config::kUpgMaxLevel) m_upgVit = config::kUpgMaxLevel;
        if (m_upgCrit > config::kUpgMaxLevel) m_upgCrit = config::kUpgMaxLevel;
        refreshDerived();
    } else {
        // Fallback (02-level-progression.md): heal 30% + fragments at run end.
        m_run.hp = std::min(m_run.maxHp, m_run.hp + 0.3f * m_run.maxHp);
        m_run.fragsDraft += config::kDraftFallbackFrags;
    }
    // Chain pending debug level-ups first, then leftover XP chain.
    if (m_pendingDrafts > 0) {
        --m_pendingDrafts;
        openDraft();
        return;
    }
    const int need = config::xpNeed(m_run.level);
    if (m_run.xp >= static_cast<float>(need)) {
        openDraft();  // stays in Draft with fresh options
        return;
    }
    m_state = UiState::Run;
    m_input.requestCapture();  // draft pick click -> try to re-lock at once
}

void Game::addXp(float v) {
    v *= 1.0f + 0.03f * m_meta.levels[1];  // XP hunger meta
    m_run.xp += v;
    int need = config::xpNeed(m_run.level);
    if (m_run.xp < static_cast<float>(need)) {
        m_run.xp01 = m_run.xp / static_cast<float>(need);
        return;
    }
    // Queue-friendly multi-level loop: the first level-up opens a draft
    // immediately (if not already drafting); further levels queue up so
    // a spammed debug "+1 level" during Draft no longer overwrites the
    // current 1-of-3 options — the extra level appears after the pick.
    bool opened = false;
    while (m_run.xp >= static_cast<float>(config::xpNeed(m_run.level))) {
        const int curNeed = config::xpNeed(m_run.level);
        if (m_run.xp < static_cast<float>(curNeed)) break;
        m_run.xp -= static_cast<float>(curNeed);
        m_run.level += 1;
        if (m_state == UiState::Draft || opened) {
            ++m_pendingDrafts;
        } else {
            opened = true;
            const int nxt = config::xpNeed(m_run.level);
            m_run.xp01 = nxt > 0 ? m_run.xp / static_cast<float>(nxt) : 0.0f;
            openDraft();
        }
    }
    need = config::xpNeed(m_run.level);
    m_run.xp01 = need > 0 ? m_run.xp / static_cast<float>(need) : 0.0f;
}

void Game::fireNova() {
    // 8-bullet ring around the player (03-upgrades.md), uses the shared
    // player bullet pool so caps/skip rules still apply.
    const glm::vec2 pp = m_player.pos();
    const glm::vec3 center(pp.x, pp.y, config::kProjSpawnZ);
    for (int i = 0; i < config::kNovaBullets; ++i) {
        const float a = 6.2831853f * static_cast<float>(i) / static_cast<float>(config::kNovaBullets);
        m_projectiles.spawnAt(center + glm::vec3(std::cos(a) * 0.6f, std::sin(a) * 0.6f, 0.0f),
                              glm::vec3(std::cos(a), std::sin(a), 0.0f) * m_bulletSpeed, m_damage);
    }
}

void Game::fireBasic() {
    // M5: 1 + Extra shots in a tight fan (max 3 bonus, then +10% damage
    // via refreshDerived). Shares the bullet pool with nova/missiles.
    const int n = 1 + std::min(m_upgExtra, config::kUpgExtraMaxShots);
    const glm::vec2 pp = m_player.pos();
    for (int i = 0; i < n; ++i) {
        const float yaw = m_camera.yaw + (i - (n - 1) * 0.5f) * 0.12f;
        const glm::vec3 dir(std::cos(yaw), std::sin(yaw), 0.0f);
        m_projectiles.spawnAt(glm::vec3(pp.x + dir.x * config::kProjForwardOffset,
                                        pp.y + dir.y * config::kProjForwardOffset, config::kProjSpawnZ),
                              dir * m_bulletSpeed, m_damage);
    }
}

void Game::fireMissiles() {
    // M5: homing volley at the nearest enemy in range (03#7: alternate
    // +1 missile / +damage per level).
    if (m_enemies.size() == 0) return;
    const glm::vec2 pp = m_player.pos();
    std::size_t best = 0;
    float bestD2 = config::kMissileAcquire * config::kMissileAcquire;
    bool found = false;
    for (std::size_t i = 0; i < m_enemies.size(); ++i) {
        const glm::vec2 d = m_enemies.data()[i].pos - pp;
        const float d2 = glm::dot(d, d);
        if (d2 < bestD2) {
            bestD2 = d2;
            best = i;
            found = true;
        }
    }
    if (!found) return;
    const int count = (m_upgMis + 1) / 2;  // lv1:1, lv3:2, lv5:3
    const float dmgMult = std::pow(config::kSkillDmgStep, m_upgMis / 2);
    const glm::vec2 tp = m_enemies.data()[best].pos;
    for (int k = 0; k < count; ++k) {
        const float a = std::atan2(tp.y - pp.y, tp.x - pp.x) + (k - (count - 1) * 0.5f) * 0.2f;
        const std::size_t before = m_projectiles.size();
        m_projectiles.spawnAt(glm::vec3(pp.x, pp.y, config::kProjSpawnZ),
                              glm::vec3(std::cos(a), std::sin(a), 0.0f) * config::kMissileSpeed,
                              m_damage * dmgMult);
        if (m_projectiles.size() > before) m_projectiles.data()[before].homing = true;
    }
}

void Game::steerMissiles(float dt) {
    // M5: homing bullets turn toward the nearest enemy (limited turn rate).
    if (m_enemies.size() == 0) return;
    for (std::size_t bi = 0; bi < m_projectiles.size(); ++bi) {
        Projectile &p = m_projectiles.data()[bi];
        if (!p.homing) continue;
        const glm::vec2 pp(p.pos.x, p.pos.y);
        float bestD2 = config::kMissileAcquire * config::kMissileAcquire;
        bool found = false;
        glm::vec2 tp(0.0f);
        for (std::size_t ei = 0; ei < m_enemies.size(); ++ei) {
            const glm::vec2 d = m_enemies.data()[ei].pos - pp;
            const float d2 = glm::dot(d, d);
            if (d2 < bestD2) {
                bestD2 = d2;
                tp = m_enemies.data()[ei].pos;
                found = true;
            }
        }
        if (!found) continue;
        const float speed = std::sqrt(p.vel.x * p.vel.x + p.vel.y * p.vel.y);
        if (speed < 1e-4f) continue;
        const float cur = std::atan2(p.vel.y, p.vel.x);
        const float want = std::atan2(tp.y - pp.y, tp.x - pp.x);
        float diff = want - cur;
        while (diff > 3.14159265f) diff -= 6.2831853f;
        while (diff < -3.14159265f) diff += 6.2831853f;
        const float maxTurn = config::kMissileTurn * dt;
        const float turn = diff < -maxTurn ? -maxTurn : (diff > maxTurn ? maxTurn : diff);
        const float a = cur + turn;
        p.vel = glm::vec3(std::cos(a) * speed, std::sin(a) * speed, 0.0f);
    }
}

void Game::updateOrbitals(float dt) {
    // M5: spinning orbs that damage on contact (03#6). Orb count/damage
    // alternate per level; per-orb hit cooldown 0.35s.
    if (m_orbCount <= 0) return;
    m_orbAngle += config::kOrbSpeed * dt;
    const float dmgMult = std::pow(config::kSkillDmgStep, m_upgOrb / 2);
    const glm::vec2 pp = m_player.pos();
    for (int k = 0; k < m_orbCount; ++k) {
        m_orbCd[k] -= dt;
        const float a = m_orbAngle + 6.2831853f * k / m_orbCount;
        m_orbPos[k] = pp + glm::vec2(std::cos(a), std::sin(a)) * config::kOrbRadius;
        if (m_orbCd[k] > 0.0f) continue;
        for (std::size_t ei = 0; ei < m_enemies.size(); ++ei) {
            Enemy &e = m_enemies.data()[ei];
            const glm::vec2 d = m_orbPos[k] - e.pos;
            const float rr = config::kOrbRadiusHit + e.radius;
            if (glm::dot(d, d) < rr * rr) {
                e.hp -= m_damage * dmgMult;
                m_orbCd[k] = config::kOrbHitCd;
                if (e.hp <= 0.0f && !onEnemyKilled(ei)) return;  // victory ended the run
                break;  // one hit per orb per cooldown
            }
        }
    }
}

bool Game::onEnemyKilled(std::size_t ei) {
    // M5: shared kill rewards for bullets/orbitals. Returns false when the
    // final-boss kill ended the run (callers must stop touching the world).
    const Enemy e = m_enemies.data()[ei];  // copy: killAt moves memory
    const bool isBoss = e.type == EnemyType::Boss;
    m_gems.spawn(e.pos, isBoss ? config::kBossGemValue : (e.elite ? config::kEliteGem : config::kGemValue));
    m_enemies.killAt(ei);
    if (isBoss) {
        ++m_run.bossKills;
        const int idx = e.bossTier >= 15 ? 2 : (e.bossTier >= 10 ? 1 : 0);
        m_run.fragsBoss += config::kBossFrags[idx];
        if (e.bossTier >= 15) {
            // Victory: final boss down (01). Big bonus + difficulty unlock.
            m_run.fragsVictory += config::kVictoryFrags;
            m_run.won = true;
            gameOver(true);
            return false;
        }
    } else {
        ++m_run.kills;
        if (e.elite) {
            ++m_run.elitesKilled;
            m_run.fragsElite += config::kEliteFrags;
        }
        m_fragAccum += config::kFragPerKill;
    }
    return true;
}

void Game::collectGems() {
    // 2D pickup radius; collected gems grant XP via addXp (level-up chain).
    const glm::vec2 pp = m_player.pos();
    const float rr = config::kGemCollectRadius;
    for (std::size_t i = 0; i < m_gems.size();) {
        const glm::vec2 d = pp - m_gems.data()[i].pos;
        if (glm::dot(d, d) < rr * rr) {
            const int v = m_gems.data()[i].value;
            m_gems.collectAt(i);
            addXp(static_cast<float>(v));
            if (m_state != UiState::Run) return;  // leveled up -> Draft paused the world
        } else {
            ++i;
        }
    }
}

void Game::togglePause() {
    if (m_state == UiState::Run) {
        m_state = UiState::Paused;
        m_input.releaseCapture();
    } else if (m_state == UiState::Paused) {
        m_state = UiState::Run;
        m_input.requestCapture();  // P key / Resume button gesture -> re-lock
    }
}

void Game::gameOver(bool won) {
    // M4/M5: real fragment economy (02-run-economy): time 2/min + kills
    // 0.1/kill + boss tiers 15/40/100 + elites + draft fallback + victory 100,
    // all x NG+ frag mult. `won` is true only via the Boss-15 kill.
    m_run.won = won;
    m_run.fragsTime = static_cast<int>(config::kFragPerMin * (m_run.timerSec / 60.0f));
    m_run.fragsKills = static_cast<int>(m_fragAccum);
    m_run.fragmentsEarned =
        static_cast<int>((m_run.fragsTime + m_run.fragsKills + m_run.fragsBoss + m_run.fragsDraft +
                          m_run.fragsElite + m_run.fragsVictory) *
                         m_fragMult);
    m_meta.fragments += m_run.fragmentsEarned;
    m_meta.recordRun(m_run.timerSec, won);
    m_meta.save();  // run end is a save point (06)
    m_state = UiState::GameOver;
    m_input.releaseCapture();
}

void Game::frame() {
    // t0 marks the start of real work; t1 the end (before swap,
    // which on web only queues the present). The difference is CPU
    // time in update+render, WITHOUT the browser vsync idle wait.
    const double t0 = glfwGetTime();
    // Only Run may capture the mouse; menus keep a visible cursor.
    m_input.setCaptureAllowed(m_state == UiState::Run);
    m_window.pollEvents();

    // ESC in Chrome exits Pointer Lock at browser level: it never reaches
    // Game as a key press, so without this there is no releaseCapture call
    // and the game just sits unlocked in Run. Auto-pause instead, which is
    // what calls releaseCapture() and shows a clickable menu.
    // Always drain; act only in Run.
    if (m_input.consumeExternalUnlock() && m_state == UiState::Run) {
        std::cout << "Pointer Lock exited (ESC) -> auto-pause" << std::endl;
        m_state = UiState::Paused;
        m_input.releaseCapture();
    }

    const double now = glfwGetTime();
    float dt = static_cast<float>(now - m_lastTime);
    m_lastTime = now;
    // Avoid large jumps when switching tabs / first frame.
    dt = std::clamp(dt, 0.0f, 0.05f);

    // Global hotkeys (edge-triggered, Run only except P-to-resume).
    {
        GLFWwindow *win = m_window.handle();
        auto edge = [&](int key, bool active) {
            const bool down = glfwGetKey(win, key) == GLFW_PRESS && active;
            const size_t i = static_cast<size_t>(key) & 511;
            const bool pressed = down && !m_prevKeys[i];
            m_prevKeys[i] = down;
            return pressed;
        };
        const bool inRunLike = m_state == UiState::Run || m_state == UiState::Paused;
        // ESC while locked never arrives here (browser consumes it to exit
        // the lock -> handled above); ESC while unlocked (or in menus with
        // a visible cursor) arrives as a normal key.
        if (edge(GLFW_KEY_P, inRunLike) || edge(GLFW_KEY_ESCAPE, inRunLike)) togglePause();
        if (edge(GLFW_KEY_L, m_state == UiState::Run)) grantLevel();
        if (edge(GLFW_KEY_K, m_state == UiState::Run)) gameOver();
        if (m_state == UiState::Hub && edge(GLFW_KEY_ENTER, true)) startRun();
    }

    if (m_state == UiState::Run) {
        try {
            update(dt);
        } catch (const std::exception &e) {
            std::cerr << "update: " << e.what() << std::endl;
        } catch (...) {
            std::cerr << "update: unknown error" << std::endl;
        }
    } else {
        // Frozen world: keep FPS/MS window alive so Hub HUD doesn't stall.
        ++m_fpsFrames;
        if (now - m_fpsLast >= 0.5) {
            const double elapsed = now - m_fpsLast;
            m_run.fps = std::clamp(static_cast<int>(m_fpsFrames / elapsed), 0, 999);
            m_run.workMs = std::clamp(static_cast<float>(m_workMsSum / m_fpsFrames), 0.0f, 999.9f);
            m_fpsFrames = 0;
            m_workMsSum = 0.0;
            m_fpsLast = now;
        }
    }

    m_window.applyViewport();
    try {
        renderScene();
    } catch (const std::exception &e) {
        std::cerr << "render: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "render: unknown error" << std::endl;
    }
    try {
        drawUi();
    } catch (const std::exception &e) {
        std::cerr << "ui: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "ui: unknown error" << std::endl;
    }
    m_workMsSum += (glfwGetTime() - t0) * 1000.0;
    m_window.swapBuffers();
}

void Game::update(float dt) {
    GLFWwindow *win = m_window.handle();
    const bool uiEatsKeys = ImGuiLayer::wantsKeyboard();

    // WASD relative to the camera yaw (gated while typing/clicking UI).
    // M5: Boots/Haste-scaled speed (m_playerSpeed via refreshDerived).
    if (!uiEatsKeys) {
        const glm::vec2 fwd(std::cos(m_camera.yaw), std::sin(m_camera.yaw));
        const glm::vec2 right(fwd.y, -fwd.x);
        if (glfwGetKey(win, GLFW_KEY_W) == GLFW_PRESS) m_player.addDisplacement(fwd * m_playerSpeed * dt);
        if (glfwGetKey(win, GLFW_KEY_S) == GLFW_PRESS) m_player.addDisplacement(-fwd * m_playerSpeed * dt);
        if (glfwGetKey(win, GLFW_KEY_D) == GLFW_PRESS) m_player.addDisplacement(right * m_playerSpeed * dt);
        if (glfwGetKey(win, GLFW_KEY_A) == GLFW_PRESS) m_player.addDisplacement(-right * m_playerSpeed * dt);
        if (glfwGetKey(win, GLFW_KEY_Q) == GLFW_PRESS) m_fill -= dt * config::kFillSpeed;
        if (glfwGetKey(win, GLFW_KEY_E) == GLFW_PRESS) m_fill += dt * config::kFillSpeed;
        m_fill = std::clamp(m_fill, 0.0f, 1.0f);
        if (glfwGetKey(win, GLFW_KEY_R) == GLFW_PRESS) m_curveK -= dt * config::kCurveSpeed;
        if (glfwGetKey(win, GLFW_KEY_F) == GLFW_PRESS) m_curveK += dt * config::kCurveSpeed;
        m_curveK = std::clamp(m_curveK, 0.0f, config::kCurveMax);
    }

    m_player.updateYaw(m_camera.yaw, dt);

    // Look: relative browser deltas (movementX/Y via WebExt),
    // drained once per frame. No lock, no look.
    {
        double dx = 0.0, dy = 0.0;
        if (m_input.consumeLookDelta(dx, dy)) m_camera.onMouseMove(dx, dy);
    }

    // Locked pointer = game owns every click, even if the virtual cursor
    // drifted over an ImGui window (same rule as InputManager).
    // M1/M5: hold-click fires with fireRate; Extra projectiles fan out.
    m_fireTimer -= dt;
    const bool uiOwnsClick = !m_input.isCaptured() && ImGuiLayer::wantsMouse();
    if (m_input.isFiring() && !uiOwnsClick && m_fireTimer <= 0.0f) {
        fireBasic();
        m_fireTimer = 1.0f / m_fireRate;
    }
    steerMissiles(dt);  // M5: homing turn before straight-line integration
    m_projectiles.update(dt);
    // M3/M5: Director (budget/interval/bosses, NG+ mults) -> enemy AI -> bullets.
    const float timeMin = m_run.timerSec / 60.0f;
    m_director.update(dt, m_run.timerSec, m_player.pos(), m_enemies, m_ngHp, m_ngDmg);
    m_enemies.update(dt, m_player.pos(), m_enemyBullets, timeMin);
    m_enemyBullets.update(dt, m_player.pos());
    collideBulletsEnemies();
    if (m_state != UiState::Run) return;  // victory ended the run mid-collision
    collideEnemiesPlayer(dt);
    collideEnemyBulletsPlayer();
    refreshBossBar();
    m_gems.update(dt, m_player.pos(), m_magnetRadius);
    collectGems();  // may open the draft; the rest of this frame still runs once (harmless)

    // M2: Nova auto-skill — ring every (6 - 0.5*lv)s once unlocked.
    if (m_upgNova > 0) {
        m_novaTimer -= dt;
        if (m_novaTimer <= 0.0f) {
            fireNova();
            const float cd = config::kNovaBaseCd - config::kNovaCdPerLevel * m_upgNova;
            m_novaTimer = std::max(config::kNovaMinCd, cd);
        }
    }

    // M5: auto-missiles + orbitals + vitality regen.
    if (m_upgMis > 0) {
        m_missileTimer -= dt;
        if (m_missileTimer <= 0.0f) {
            fireMissiles();
            m_missileTimer = config::kMissileCd;
        }
    }
    updateOrbitals(dt);
    if (m_state != UiState::Run) return;  // orbital kill scored victory
    if (m_upgVit >= 3 && m_run.hp > 0.0f && m_run.hp < m_run.maxHp)
        m_run.hp = std::min(m_run.maxHp, m_run.hp + config::kUpgVitalityRegen * dt);

    m_camera.update(dt, m_player.pos());

    m_run.timerSec += dt;
    // M4/M5: live fragment counter for the HUD (finalized in gameOver).
    m_run.fragmentsEarned =
        static_cast<int>((m_run.fragsDraft + m_run.fragsBoss + m_run.fragsElite + m_run.fragsVictory +
                          static_cast<int>(m_fragAccum) +
                          static_cast<int>(config::kFragPerMin * (m_run.timerSec / 60.0f))) *
                         m_fragMult);
    // M5: meta revive (once per run at 50% HP) before death.
    if (m_run.hp <= 0.0f) {
        if (m_meta.levels[5] > 0 && !m_reviveUsed) {
            m_reviveUsed = true;
            m_run.hp = 0.5f * m_run.maxHp;
            m_invulnTimer = 1.0f;
        } else {
            gameOver();
        }
    }

    // FPS (wall-clock, includes vsync) + average WORK ms per frame
    // (update+render only, no idle). Same 0.5 s window, dirty at 2 Hz.
    const double now = glfwGetTime();
    ++m_fpsFrames;
    if (now - m_fpsLast >= 0.5) {
        const double elapsed = now - m_fpsLast;
        m_run.fps = std::clamp(static_cast<int>(m_fpsFrames / elapsed), 0, 999);
        m_run.workMs = std::clamp(static_cast<float>(m_workMsSum / m_fpsFrames), 0.0f, 999.9f);
        m_fpsFrames = 0;
        m_workMsSum = 0.0;
        m_fpsLast = now;
    }
}

void Game::collideBulletsEnemies() {
    // 2D circle collisions (xy only, curved z is visual). O(n*m) is fine
    // for n,m < 300 at 60fps (see 05-architecture). No allocs.
    // M5: heavy-sized bullets (m_bulletRadius) + crit rolls (x2).
    auto *bullets = m_projectiles.data();
    auto *enemies = m_enemies.data();
    for (std::size_t bi = 0; bi < m_projectiles.size();) {
        const glm::vec2 bp(bullets[bi].pos.x, bullets[bi].pos.y);
        bool consumed = false;
        for (std::size_t ei = 0; ei < m_enemies.size(); ++ei) {
            const glm::vec2 d = bp - enemies[ei].pos;
            const float rr = m_bulletRadius + enemies[ei].radius;
            if (glm::dot(d, d) < rr * rr) {
                float dmg = bullets[bi].damage;
                if (rand01() < m_critCh) dmg *= 2.0f;
                enemies[ei].hp -= dmg;
                consumed = true;
                if (enemies[ei].hp <= 0.0f) {
                    if (!onEnemyKilled(ei)) return;  // victory ended the run
                    enemies = m_enemies.data();      // swap-remove moved memory
                }
                break;  // one bullet hits one enemy
            }
        }
        if (consumed) {
            m_projectiles.killAt(bi);  // swapped-in bullet still needs testing
            bullets = m_projectiles.data();
        } else {
            ++bi;
        }
    }
}

void Game::collideEnemiesPlayer(float dt) {
    m_invulnTimer = std::max(0.0f, m_invulnTimer - dt);
    if (m_invulnTimer > 0.0f) return;
    if (m_godMode) return;  // debug cheat
    const glm::vec2 pp = m_player.pos();
    for (std::size_t i = 0; i < m_enemies.size(); ++i) {
        const Enemy &e = m_enemies.data()[i];
        const glm::vec2 d = pp - e.pos;
        const float rr = config::kPlayerRadius + e.radius;  // M3: per-type radius
        if (glm::dot(d, d) < rr * rr) {
            m_run.hp -= e.damage;
            m_invulnTimer = config::kPlayerInvulnSec;
            break;  // one hit per iframe window
        }
    }
}

void Game::collideEnemyBulletsPlayer() {
    // M3: enemy bullets vs player (shared iframes with contact hits).
    if (m_invulnTimer > 0.0f) return;
    if (m_godMode) return;  // debug cheat
    const glm::vec2 pp = m_player.pos();
    auto *bullets = m_enemyBullets.data();
    for (std::size_t i = 0; i < m_enemyBullets.size(); ++i) {
        const glm::vec2 d(pp.x - bullets[i].pos.x, pp.y - bullets[i].pos.y);
        const float rr = config::kPlayerRadius + config::kEnemyBulletRadius;
        if (glm::dot(d, d) < rr * rr) {
            m_run.hp -= bullets[i].damage;
            m_invulnTimer = config::kPlayerInvulnSec;
            m_enemyBullets.killAt(i);
            break;  // one hit per iframe window
        }
    }
}

void Game::refreshBossBar() {
    m_run.bossHp01 = -1.0f;
    m_run.bossTier = 0;
    for (std::size_t i = 0; i < m_enemies.size(); ++i) {
        const Enemy &e = m_enemies.data()[i];
        if (e.type == EnemyType::Boss && e.maxHp > 0.0f) {
            m_run.bossHp01 = std::clamp(e.hp / e.maxHp, 0.0f, 1.0f);
            m_run.bossTier = e.bossTier > 0 ? e.bossTier : 5;
            return;
        }
    }
}

void Game::refreshEdgeMarkers() {
    // M5: project enemies to NDC; off-screen ones become border markers.
    // Bosses first so they are never pushed out of the 64 cap.
    // Curve fix: the entity shader sinks verts by curveK*|rel|^2, so the CPU
    // projection must apply the same drop (anchored at the cube-center height
    // the renderer draws, radius*1.6). A flat z=1 projection disagrees by
    // 12+ units at spawn-ring range.
    const glm::vec2 pp = m_player.pos();
    m_edgeCount = 0;
    for (int pass = 0; pass < 2 && m_edgeCount < m_edge.size(); ++pass) {
        for (std::size_t i = 0; i < m_enemies.size() && m_edgeCount < m_edge.size(); ++i) {
            const Enemy &e = m_enemies.data()[i];
            const bool isBoss = e.type == EnemyType::Boss;
            if ((pass == 0) != isBoss) continue;
            const glm::vec2 rel = e.pos - pp;
            const float z = e.radius * 1.6f - m_curveK * glm::dot(rel, rel);
            const glm::vec4 clip = m_projView * glm::vec4(e.pos.x, e.pos.y, z, 1.0f);
            if (clip.w < 1e-4f) continue;
            float nx = clip.x / clip.w;
            float ny = clip.y / clip.w;
            if (nx > -0.95f && nx < 0.95f && ny > -0.9f && ny < 0.9f) continue;  // on screen
            if (nx < -0.93f || nx > 0.93f || ny < -0.88f || ny > 0.88f) {
                const float sx = 0.93f / (std::abs(nx) > 1e-4f ? std::abs(nx) : 1e-4f);
                const float sy = 0.88f / (std::abs(ny) > 1e-4f ? std::abs(ny) : 1e-4f);
                const float s = sx < sy ? sx : sy;
                nx *= s;
                ny *= s;
            }
            ui::EdgeMarker &mk = m_edge[m_edgeCount++];
            mk.x = nx;
            mk.y = ny;
            mk.boss = isBoss;
            mk.elite = e.elite;
        }
    }
}

void Game::renderScene() {
    const int fbW = m_window.fbWidth();
    const int fbH = m_window.fbHeight();

    const glm::mat4 view = m_camera.getView();
    const float aspect = static_cast<float>(fbW) / static_cast<float>(fbH > 0 ? fbH : 1);
    const glm::mat4 proj = glm::perspective(glm::radians(60.0f), aspect, 0.1f, 500.0f);
    m_projView = proj * view;  // M5: edge arrows project with this matrix

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Planet (grid re-centered under the player with per-cell snap).
    const float snapX = std::floor(m_player.pos().x / config::kCell + 0.5f) * config::kCell;
    const float snapY = std::floor(m_player.pos().y / config::kCell + 0.5f) * config::kCell;
    m_planet.draw(proj * view, m_player.pos(), {snapX, snapY}, m_curveK, m_fill, {0.2f, 1.0f, 0.4f},
                  config::kFogDensity);

    // Player + nose + projectiles + enemies (same curved shader).
    m_entities.begin(proj * view, m_player.pos(), m_curveK);
    glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(m_player.pos().x, m_player.pos().y, 0.0f));
    model = glm::rotate(model, m_player.yaw(), glm::vec3(0.0f, 0.0f, 1.0f));
    m_entities.drawPlayer(model, {1.0f, 0.25f, 0.2f});
    m_entities.drawNose(model, {1.0f, 0.85f, 0.2f});
    m_entities.drawProjectiles(m_projectiles.data(), m_projectiles.size(), {0.3f, 0.8f, 1.0f});
    // Gems as small green cubes (stack array: no heap allocs per frame).
    {
        const std::size_t n = m_gems.size();
        glm::vec2 gpos[300];
        for (std::size_t i = 0; i < n; ++i) gpos[i] = m_gems.data()[i].pos;
        if (n > 0) m_entities.drawGems(gpos, n, {0.3f, 1.0f, 0.4f});
    }
    // Enemies as colored cubes (stack arrays: no heap allocs per frame).
    // Scale derives from the per-type radius (chaser 0.5 -> 0.8 to match M1).
    // M5: elites read yellow (radius already x1.3 from make()).
    {
        const std::size_t n = m_enemies.size();
        // Fixed cap 256: bounded stack copies, refreshed every frame.
        glm::vec2 epos[256];
        float escale[256];
        glm::vec3 ecolor[256];
        for (std::size_t i = 0; i < n; ++i) {
            const Enemy &e = m_enemies.data()[i];
            epos[i] = e.pos;
            escale[i] = e.radius * 1.6f;
            ecolor[i] = e.elite ? glm::vec3(1.0f, 0.85f, 0.2f) : enemyColor(e.type);
        }
        if (n > 0) m_entities.drawEnemies(epos, escale, ecolor, n);
    }
    // M5: orbitals as cyan cubes reusing the gem mesh.
    if (m_orbCount > 0) m_entities.drawGems(m_orbPos, static_cast<std::size_t>(m_orbCount), {0.3f, 1.0f, 1.0f});
    // M3: enemy bullets (magenta), same curved shader + projectile mesh.
    if (m_enemyBullets.size() > 0)
        m_entities.drawEnemyBullets(m_enemyBullets.data(), m_enemyBullets.size(), {1.0f, 0.2f, 0.5f});
    refreshEdgeMarkers();
}

void Game::drawUi() {
    m_imgui.beginFrame(m_window.fbWidth(), m_window.fbHeight());
    const DebugSnapshot snap = buildDebugSnapshot();
    DebugActions actions = debugActions();
    switch (m_state) {
        case UiState::Hub:
            ui::drawHub(m_meta, [this] { startRun(); }, [this] { m_meta.save(); });
            ui::drawDebugPanel(m_run, m_meta, snap, actions, m_curveK, m_fill, m_input.isCaptured());
            break;
        case UiState::Run:
            ui::drawHud(m_run, m_meta, snap, actions, m_curveK, m_fill, m_input.isCaptured());
            ui::drawEdgeArrows(m_edge.data(), m_edgeCount);
            break;
        case UiState::Draft:
            ui::drawHud(m_run, m_meta, snap, actions, m_curveK, m_fill);
            ui::drawEdgeArrows(m_edge.data(), m_edgeCount);
            ui::drawDraft(m_draft, [this](int i) { applyDraft(i); });
            break;
        case UiState::Paused:
            ui::drawHud(m_run, m_meta, snap, actions, m_curveK, m_fill);
            ui::drawEdgeArrows(m_edge.data(), m_edgeCount);
            ui::drawPause([this] { togglePause(); }, [this] { quitToHub(); });
            break;
        case UiState::GameOver:
            ui::drawGameOver(m_run, [this] { startRun(); }, [this] { quitToHub(); });
            ui::drawDebugPanel(m_run, m_meta, snap, actions, m_curveK, m_fill, m_input.isCaptured());
            break;
    }
    m_imgui.endFrame();
}
