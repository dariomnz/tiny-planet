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
    m_fragAccum = 0.0f;
    m_damage = config::kDmgBase * (1.0f + 0.02f * m_meta.levels[0]);
    m_fireRate = config::kFireRateBase;
    m_fireTimer = 0.0f;
    m_novaTimer = 0.0f;
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

void Game::openDraft() {
    // M2: first real build choice — damage / fire rate / nova (max 5 each).
    // Only non-maxed upgrades are offered; when everything is maxed all
    // three slots become the "Heal 30% + 10 Fragments" fallback (02-draft).
    const bool maxDmg = m_upgDamage >= config::kUpgMaxLevel;
    const bool maxFire = m_upgFire >= config::kUpgMaxLevel;
    const bool maxNova = m_upgNova >= config::kUpgMaxLevel;
    int slot = 0;
    auto offer = [&](int id, const char *name, char *descBuf, std::size_t n, const char *fmt, int lvl) {
        snprintf(descBuf, n, fmt, lvl + 1);
        m_draft[static_cast<std::size_t>(slot++)] = {name, descBuf, id};
    };
    // Stack buffers: no heap allocs for the formatted descriptions.
    char d0[96], d1[96], d2[96];
    if (!maxDmg && slot < 3) offer(0, "Basic damage", d0, sizeof(d0), "+15%% damage (lv %d)", m_upgDamage);
    if (!maxFire && slot < 3) offer(1, "Fire rate", d1, sizeof(d1), "+12%% fire rate (lv %d)", m_upgFire);
    if (!maxNova && slot < 3) offer(2, "Nova", d2, sizeof(d2), "8-bullet ring (lv %d)", m_upgNova);
    // Fallback fills any remaining slots (all three when fully maxed).
    for (; slot < 3; ++slot)
        m_draft[static_cast<std::size_t>(slot)] = {"Overcharge", "Heal 30% + 10 Fragments", 3};
    m_state = UiState::Draft;
    m_input.releaseCapture();
}

void Game::applyDraft(int idx) {
    const int id = (idx >= 0 && idx < 3) ? m_draft[static_cast<std::size_t>(idx)].id : 3;
    if (id == 0 && m_upgDamage < config::kUpgMaxLevel) {
        ++m_upgDamage;
        m_damage = config::kDmgBase * std::pow(config::kUpgDamageMult, m_upgDamage) *
                   (1.0f + 0.02f * m_meta.levels[0]);
    } else if (id == 1 && m_upgFire < config::kUpgMaxLevel) {
        ++m_upgFire;
        m_fireRate = config::kFireRateBase * std::pow(config::kUpgFireRateMult, m_upgFire);
    } else if (id == 2 && m_upgNova < config::kUpgMaxLevel) {
        ++m_upgNova;
        m_novaTimer = std::min(m_novaTimer, 1.0f);  // first ring comes out quickly
    } else {
        // Fallback (02-level-progression.md): heal 30% + fragments at run end.
        m_run.hp = std::min(m_run.maxHp, m_run.hp + 0.3f * m_run.maxHp);
        m_run.fragsDraft += config::kDraftFallbackFrags;
    }
    // Level-up chain: leftover XP from a big pickup can afford another level.
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
    m_run.xp -= static_cast<float>(need);
    m_run.level += 1;
    need = config::xpNeed(m_run.level);
    m_run.xp01 = m_run.xp / static_cast<float>(need);
    openDraft();
}

void Game::fireNova() {
    // 8-bullet ring around the player (03-upgrades.md), uses the shared
    // player bullet pool so caps/skip rules still apply.
    const glm::vec2 pp = m_player.pos();
    const glm::vec3 center(pp.x, pp.y, config::kProjSpawnZ);
    for (int i = 0; i < config::kNovaBullets; ++i) {
        const float a = 6.2831853f * static_cast<float>(i) / static_cast<float>(config::kNovaBullets);
        m_projectiles.spawnAt(center + glm::vec3(std::cos(a) * 0.6f, std::sin(a) * 0.6f, 0.0f),
                              glm::vec3(std::cos(a), std::sin(a), 0.0f) * config::kProjSpeed, m_damage);
    }
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
    // M4: real fragment economy (02-run-economy): time 2/min + kills 0.1/kill
    // + boss tiers (15/40/100, only Boss-5 exists until M5) + draft fallback.
    m_run.fragsTime = static_cast<int>(config::kFragPerMin * (m_run.timerSec / 60.0f));
    m_run.fragsKills = static_cast<int>(m_fragAccum);
    m_run.fragmentsEarned = m_run.fragsTime + m_run.fragsKills + m_run.fragsBoss + m_run.fragsDraft;
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
        if (edge(GLFW_KEY_L, m_state == UiState::Run)) addXp(static_cast<float>(config::xpNeed(m_run.level)));
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
    if (!uiEatsKeys) {
        const glm::vec2 fwd(std::cos(m_camera.yaw), std::sin(m_camera.yaw));
        const glm::vec2 right(fwd.y, -fwd.x);
        if (glfwGetKey(win, GLFW_KEY_W) == GLFW_PRESS) m_player.addDisplacement(fwd * config::kMoveSpeed * dt);
        if (glfwGetKey(win, GLFW_KEY_S) == GLFW_PRESS) m_player.addDisplacement(-fwd * config::kMoveSpeed * dt);
        if (glfwGetKey(win, GLFW_KEY_D) == GLFW_PRESS) m_player.addDisplacement(right * config::kMoveSpeed * dt);
        if (glfwGetKey(win, GLFW_KEY_A) == GLFW_PRESS) m_player.addDisplacement(-right * config::kMoveSpeed * dt);
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
    // M1: hold-click fires with fireRate (docs 01-overview / 07-roadmap).
    m_fireTimer -= dt;
    const bool uiOwnsClick = !m_input.isCaptured() && ImGuiLayer::wantsMouse();
    if (m_input.isFiring() && !uiOwnsClick && m_fireTimer <= 0.0f) {
        m_projectiles.spawn(m_player.pos(), m_camera.yaw, m_damage);
        m_fireTimer = 1.0f / m_fireRate;
    }
    m_projectiles.update(dt);
    // M3: Director (budget/interval/boss) -> enemy AI + patterns -> bullets.
    const float timeMin = m_run.timerSec / 60.0f;
    m_director.update(dt, m_run.timerSec, m_player.pos(), m_enemies);
    m_enemies.update(dt, m_player.pos(), m_enemyBullets, timeMin);
    m_enemyBullets.update(dt);
    collideBulletsEnemies();
    collideEnemiesPlayer(dt);
    collideEnemyBulletsPlayer();
    refreshBossBar();
    m_gems.update(dt, m_player.pos());
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

    m_camera.update(dt, m_player.pos());

    m_run.timerSec += dt;
    // M4: live fragment counter for the HUD (finalized in gameOver).
    m_run.fragmentsEarned = m_run.fragsDraft + m_run.fragsBoss + static_cast<int>(m_fragAccum) +
                            static_cast<int>(config::kFragPerMin * (m_run.timerSec / 60.0f));
    if (m_run.hp <= 0.0f) gameOver();

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
    auto *bullets = m_projectiles.data();
    auto *enemies = m_enemies.data();
    for (std::size_t bi = 0; bi < m_projectiles.size();) {
        const glm::vec2 bp(bullets[bi].pos.x, bullets[bi].pos.y);
        bool consumed = false;
        for (std::size_t ei = 0; ei < m_enemies.size(); ++ei) {
            const glm::vec2 d = bp - enemies[ei].pos;
            const float rr = config::kProjRadius + enemies[ei].radius;
            if (glm::dot(d, d) < rr * rr) {
                enemies[ei].hp -= bullets[bi].damage;
                consumed = true;
                if (enemies[ei].hp <= 0.0f) {
                    // M4: boss pays 25-XP gem + fragsBoss tier; normals feed
                    // the 0.1/kill accumulator settled at run end.
                    const bool isBoss = enemies[ei].type == EnemyType::Boss;
                    m_gems.spawn(enemies[ei].pos, isBoss ? config::kBossGemValue : config::kGemValue);
                    m_enemies.killAt(ei);
                    enemies = m_enemies.data();  // swap-remove moved memory
                    if (isBoss) {
                        ++m_run.bossKills;
                        m_run.fragsBoss += config::kBossFragments;
                    } else {
                        ++m_run.kills;
                        m_fragAccum += config::kFragPerKill;
                    }
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
    for (std::size_t i = 0; i < m_enemies.size(); ++i) {
        const Enemy &e = m_enemies.data()[i];
        if (e.type == EnemyType::Boss && e.maxHp > 0.0f) {
            m_run.bossHp01 = std::clamp(e.hp / e.maxHp, 0.0f, 1.0f);
            return;
        }
    }
}

void Game::renderScene() {    const int fbW = m_window.fbWidth();
    const int fbH = m_window.fbHeight();

    const glm::mat4 view = m_camera.getView();
    const float aspect = static_cast<float>(fbW) / static_cast<float>(fbH > 0 ? fbH : 1);
    const glm::mat4 proj = glm::perspective(glm::radians(60.0f), aspect, 0.1f, 500.0f);

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
            ecolor[i] = enemyColor(e.type);
        }
        if (n > 0) m_entities.drawEnemies(epos, escale, ecolor, n);
    }
    // M3: enemy bullets (magenta), same curved shader + projectile mesh.
    if (m_enemyBullets.size() > 0)
        m_entities.drawEnemyBullets(m_enemyBullets.data(), m_enemyBullets.size(), {1.0f, 0.2f, 0.5f});
}

void Game::drawUi() {
    m_imgui.beginFrame(m_window.fbWidth(), m_window.fbHeight());
    switch (m_state) {
        case UiState::Hub:
            ui::drawHub(m_meta, [this] { startRun(); }, [this] { m_meta.save(); });
            break;
        case UiState::Run:
            ui::drawHud(m_run, m_curveK, m_fill, m_input.isCaptured());
            break;
        case UiState::Draft:
            ui::drawHud(m_run, m_curveK, m_fill);
            ui::drawDraft(m_draft, [this](int i) { applyDraft(i); });
            break;
        case UiState::Paused:
            ui::drawHud(m_run, m_curveK, m_fill);
            ui::drawPause([this] { togglePause(); }, [this] { quitToHub(); });
            break;
        case UiState::GameOver:
            ui::drawGameOver(m_run, [this] { startRun(); }, [this] { quitToHub(); });
            break;
    }
    m_imgui.endFrame();
}
