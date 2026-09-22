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

    std::cout << "Controls: Hub = mouse, WASD = move, click = fire, "
                 "P = pause, L = draft, K = die\n";

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
    m_run = RunStats{};
    m_run.maxHp = 100.0f + 10.0f * m_meta.levels[3];
    m_run.hp = m_run.maxHp;
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
    m_draft = {{{"Basic damage", "+15% damage (mock)"},
                {"Fire rate", "+12% fire rate (mock)"},
                {"Vitality", "+20 max HP, heal 20 (mock)"}}};
    m_state = UiState::Draft;
    m_input.releaseCapture();
}

void Game::applyDraft(int idx) {
    if (idx == 2) {
        m_run.maxHp += 20.0f;
        m_run.hp = std::min(m_run.maxHp, m_run.hp + 20.0f);
    }
    if (idx == 0) m_run.fragmentsEarned += 1;  // mock visible effect
    m_run.level += 1;
    m_run.xp01 = 0.0f;
    m_state = UiState::Run;
    m_input.requestCapture();  // draft pick click -> try to re-lock at once
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

void Game::gameOver() {
    m_meta.fragments += m_run.fragmentsEarned;
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
        if (edge(GLFW_KEY_L, m_state == UiState::Run)) openDraft();
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
    const bool fireClick = m_input.consumeFireRequest();
    const bool uiOwnsClick = !m_input.isCaptured() && ImGuiLayer::wantsMouse();
    if (fireClick && !uiOwnsClick) m_projectiles.spawn(m_player.pos(), m_camera.yaw);
    m_projectiles.update(dt);

    m_camera.update(dt, m_player.pos());

    // Mock run clock / xp so HUD moves until real game/Run lands.
    m_run.timerSec += dt;
    m_run.xp01 += dt * 0.05f;
    if (m_run.xp01 >= 1.0f) openDraft();
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

void Game::renderScene() {
    const int fbW = m_window.fbWidth();
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

    // Player + nose + projectiles (same curved shader).
    m_entities.begin(proj * view, m_player.pos(), m_curveK);
    glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(m_player.pos().x, m_player.pos().y, 0.0f));
    model = glm::rotate(model, m_player.yaw(), glm::vec3(0.0f, 0.0f, 1.0f));
    m_entities.drawPlayer(model, {1.0f, 0.25f, 0.2f});
    m_entities.drawNose(model, {1.0f, 0.85f, 0.2f});
    m_entities.drawProjectiles(m_projectiles.list(), {0.3f, 0.8f, 1.0f});
}

void Game::drawUi() {
    m_imgui.beginFrame(m_window.fbWidth(), m_window.fbHeight());
    switch (m_state) {
        case UiState::Hub:
            ui::drawHub(m_meta, [this] { startRun(); });
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
