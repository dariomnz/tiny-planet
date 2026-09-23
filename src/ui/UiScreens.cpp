#include "ui/UiScreens.h"

#include <imgui.h>

#include <chrono>
#include <cmath>
#include <cstdio>

#include "Config.h"

namespace ui {

// Self timer for per-panel ms (UI layer stays free of GLFW).
struct PanelClock {
    std::chrono::steady_clock::time_point t0 = std::chrono::steady_clock::now();
    float ms() const {
        return std::chrono::duration<float, std::milli>(std::chrono::steady_clock::now() - t0).count();
    }
};

const char *metaName(int i) {
    static const char *kNames[kMetaCount] = {"Base edge",   "XP hunger",      "Haste",
                                             "Extra heart", "2nd skill slot", "Revive"};
    return (i >= 0 && i < kMetaCount) ? kNames[i] : "?";
}

const char *metaDesc(int i) {
    static const char *kDescs[kMetaCount] = {"+2% base damage / level",
                                             "+3% XP / level",
                                             "+1% speed / level",
                                             "+10 starting max HP / level",
                                             "Unlock 2nd auto skill (one-time)",
                                             "Auto-revive at 50% HP (one-time)"};
    return (i >= 0 && i < kMetaCount) ? kDescs[i] : "";
}

int metaMax(int i) {
    static const int kMax[kMetaCount] = {50, 30, 20, 10, 1, 1};
    return (i >= 0 && i < kMetaCount) ? kMax[i] : 1;
}

static int costFor(int index, int level) {
    // Mirrors 03-upgrades.md formulas (simplified): 10*1.8^n, 15*1.9^n,
    // 25*2.0^n, one-time 200 / 500.
    switch (index) {
        case 0:
        case 1: {
            int c = 10;
            for (int k = 0; k < level; ++k) c = static_cast<int>(c * 1.8f);
            return c;
        }
        case 2: {
            int c = 15;
            for (int k = 0; k < level; ++k) c = static_cast<int>(c * 1.9f);
            return c;
        }
        case 3: {
            int c = 25;
            for (int k = 0; k < level; ++k) c *= 2;
            return c;
        }
        case 4:
            return 200;
        case 5:
            return 500;
        default:
            return 10;
    }
}

void drawHud(const RunStats &run, const Meta &meta, const DebugSnapshot &snap, DebugActions actions,
             float &curveK, float &fill, bool mouseCaptured, UiPanelMs &timers) {
    PanelClock clk;
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.35f);
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoNav |
                             ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_AlwaysAutoResize |
                             ImGuiWindowFlags_NoSavedSettings;
    ImGui::Begin("HUD", nullptr, flags);
    ImGui::Text("%d FPS  %.1f MS", run.fps, static_cast<double>(run.workMs));
    ImGui::ProgressBar(
        run.hp / (run.maxHp > 0 ? run.maxHp : 1.0f), ImVec2(200, 0),
        ("HP " + std::to_string(static_cast<int>(run.hp)) + "/" + std::to_string(static_cast<int>(run.maxHp))).c_str());
    const int mm = static_cast<int>(run.timerSec) / 60;
    const int ss = static_cast<int>(run.timerSec) % 60;
    ImGui::Text("Time %02d:%02d  Lv %d  Frags +%d", mm, ss, run.level, run.fragmentsEarned);
    ImGui::ProgressBar(run.xp01, ImVec2(200, 0), "XP");
    if (run.bossHp01 >= 0.0f) {
        char label[16];
        snprintf(label, sizeof(label), "BOSS-%d", run.bossTier > 0 ? run.bossTier : 5);
        ImGui::ProgressBar(run.bossHp01, ImVec2(200, 0), label);
    }
    ImGui::End();

    if (!mouseCaptured) {
        const ImVec2 center(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.85f);
        ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowBgAlpha(0.7f);
        ImGuiWindowFlags hintFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs |
                                     ImGuiWindowFlags_NoNav | ImGuiWindowFlags_AlwaysAutoResize |
                                     ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing;
        ImGui::Begin("CaptureHint", nullptr, hintFlags);
        ImGui::Text("Click the scene to capture the mouse (camera + fire)");
        ImGui::End();
    }

    // Full debug panel (collapsible sections + cheats, visible in every state).
    // Timed separately: hud records its window only, excl. the debug panel.
    const float hudOnlyMs = clk.ms();
    drawDebugPanel(run, meta, snap, actions, curveK, fill, mouseCaptured, timers);
    timers.hud = hudOnlyMs;
}

void drawDebugPanel(const RunStats &run, const Meta &meta, const DebugSnapshot &snap, DebugActions actions,
                    float &curveK, float &fill, bool mouseCaptured, UiPanelMs &timers) {
    PanelClock clk;
    ImGui::SetNextWindowPos(ImVec2(10, 150), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowBgAlpha(0.6f);
    ImGui::Begin("Debug", nullptr, ImGuiWindowFlags_NoSavedSettings);

    if (ImGui::CollapsingHeader("Tuning", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::SliderFloat("curveK", &curveK, 0.0f, 0.2f, "%.3f");
        ImGui::SliderFloat("fill", &fill, 0.0f, 1.0f, "%.2f");
        ImGui::TextDisabled("WASD move | Hold-click fire | P pause | L level | K die");
        // Live capture diagnostics (remote debugging aid).
        const ImGuiIO &io = ImGui::GetIO();
        ImGui::TextDisabled("Lock:%s ImGuiMouse:%d Mouse:(%.0f,%.0f) FB:(%.0fx%.0f)", mouseCaptured ? "ON" : "OFF",
                            io.WantCaptureMouse ? 1 : 0, static_cast<double>(io.MousePos.x),
                            static_cast<double>(io.MousePos.y), static_cast<double>(io.DisplaySize.x),
                            static_cast<double>(io.DisplaySize.y));
    }

    if (ImGui::CollapsingHeader("Frame / App", ImGuiTreeNodeFlags_DefaultOpen)) {
        const int mm = static_cast<int>(run.timerSec) / 60;
        const int ss = static_cast<int>(run.timerSec) % 60;
        ImGui::Text("State %s  %d FPS  %.1f ms work", uiStateName(snap.state), run.fps,
                    static_cast<double>(run.workMs));
        ImGui::Text("Run time %02d:%02d (%.1fs, %.2f min)  FB %dx%d", mm, ss,
                    static_cast<double>(run.timerSec), static_cast<double>(snap.timeMin), snap.fbW, snap.fbH);
    }

    if (ImGui::CollapsingHeader("Perf (CPU ms, 0.5s avg)", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text("total %.2f  update %.2f (sim %.2f collide %.2f)", static_cast<double>(snap.perfTotal),
                    static_cast<double>(snap.perfUpdate), static_cast<double>(snap.perfSim),
                    static_cast<double>(snap.perfCollide));
        ImGui::Text("planet %.2f  submit %.2f  flush %.2f  ui %.2f", static_cast<double>(snap.perfPlanet),
                    static_cast<double>(snap.perfEntSubmit), static_cast<double>(snap.perfEntFlush),
                    static_cast<double>(snap.perfUi));
        if (snap.histN > 1) {
            char totalLbl[48], updateLbl[48], renderLbl[48], uiLbl[48];
            snprintf(totalLbl, sizeof(totalLbl), "total %.2f ms", static_cast<double>(snap.perfTotal));
            snprintf(updateLbl, sizeof(updateLbl), "update %.2f ms", static_cast<double>(snap.perfUpdate));
            const float renderMs = snap.perfPlanet + snap.perfEntSubmit + snap.perfEntFlush;
            snprintf(renderLbl, sizeof(renderLbl), "render %.2f ms", static_cast<double>(renderMs));
            snprintf(uiLbl, sizeof(uiLbl), "ui %.2f ms", static_cast<double>(snap.perfUi));
            ImGui::PlotLines("ms total", snap.histTotal.data(), snap.histN, 0, totalLbl, 0.0f, FLT_MAX,
                             ImVec2(-1, 60));
            ImGui::PlotLines("ms update", snap.histUpdate.data(), snap.histN, 0, updateLbl, 0.0f, FLT_MAX,
                             ImVec2(-1, 40));
            ImGui::PlotLines("ms render", snap.histRender.data(), snap.histN, 0, renderLbl, 0.0f, FLT_MAX,
                             ImVec2(-1, 40));
            ImGui::PlotLines("ms ui", snap.histUi.data(), snap.histN, 0, uiLbl, 0.0f, FLT_MAX,
                             ImVec2(-1, 40));
        } else {
            ImGui::TextDisabled("collecting frame history...");
        }
        if (ImGui::CollapsingHeader("UI panels")) {
            if (ImGui::BeginTable("ui_panels", 2,
                                  ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedFit |
                                      ImGuiTableFlags_NoSavedSettings)) {
                ImGui::TableSetupColumn("ui panel");
                ImGui::TableSetupColumn("ms");
                ImGui::TableHeadersRow();
                const UiPanelMs &p = snap.perfUiPanels;
                const char *kNames[9] = {"snap", "frame", "hub", "hud", "debug",
                                         "draft", "pause", "over", "edge"};
                const float kVals[9] = {p.snap, p.frame, p.hub, p.hud, p.debug, p.draft, p.pause, p.over, p.edge};
                for (int i = 0; i < 9; ++i) {
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted(kNames[i]);
                    ImGui::TableNextColumn();
                    ImGui::Text("%.2f", static_cast<double>(kVals[i]));
                }
                ImGui::EndTable();
            }
        }
    }

    if (ImGui::CollapsingHeader("Player", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text("Pos (%.1f, %.1f)  yaw %.2f", static_cast<double>(snap.playerPos.x),
                    static_cast<double>(snap.playerPos.y), static_cast<double>(snap.playerYaw));
        ImGui::Text("HP %.0f/%.0f  speed %.2f  magnet %.2f", static_cast<double>(run.hp),
                    static_cast<double>(run.maxHp), static_cast<double>(snap.playerSpeed),
                    static_cast<double>(snap.magnetRadius));
        ImGui::Text("iframes %.2fs  firing %d  revive %s", static_cast<double>(snap.invulnTimer),
                    snap.firing ? 1 : 0, snap.reviveUsed ? "used" : "ready");
    }

    if (ImGui::CollapsingHeader("Combat (derived)")) {
        ImGui::Text("dmg %.2f  fire %.2f/s (timer %.2f)  crit %.0f%%", static_cast<double>(snap.damage),
                    static_cast<double>(snap.fireRate), static_cast<double>(snap.fireTimer),
                    static_cast<double>(snap.critCh * 100.0f));
        ImGui::Text("bullet spd %.1f  radius %.2f", static_cast<double>(snap.bulletSpeed),
                    static_cast<double>(snap.bulletRadius));
        ImGui::Text("nova %.1fs  missile %.1fs  orbs %d (%.1f rad)", static_cast<double>(snap.novaTimer),
                    static_cast<double>(snap.missileTimer), snap.orbCount, static_cast<double>(snap.orbAngle));
        ImGui::Text("NG+ hp x%.2f  dmg x%.2f  frag x%.2f", static_cast<double>(snap.ngHp),
                    static_cast<double>(snap.ngDmg), static_cast<double>(snap.fragMult));
        if (actions.godMode) ImGui::Text("god %s", *actions.godMode ? "ON" : "OFF");
    }

    if (ImGui::CollapsingHeader("Upgrades (draft levels)")) {
        static const char *kUpg[10] = {"dmg", "fire", "nova", "extra", "heavy",
                                       "orb", "mis",  "boots", "vit",  "crit"};
        for (int i = 0; i < 10; ++i) {
            ImGui::Text("%-5s %d/%d%s", kUpg[i], snap.upgLevels[static_cast<size_t>(i)],
                        config::kUpgMaxLevel, i < 9 ? " |" : "");
            if (i % 2 == 0) ImGui::SameLine();
        }
        ImGui::NewLine();
    }

    if (ImGui::CollapsingHeader("XP / Level")) {
        ImGui::Text("Lv %d  xp %.0f/%d (%.0f%%)%s", run.level, static_cast<double>(run.xp), snap.xpNeed,
                    static_cast<double>(run.xp01 * 100.0f),
                    snap.pendingDrafts > 0 ? "  (queued drafts!)" : "");
        if (snap.pendingDrafts > 0)
            ImGui::Text("queued draft levels: %d", snap.pendingDrafts);
        ImGui::Text("kills %d  elites %d  bosses %d", run.kills, run.elitesKilled, run.bossKills);
        if (run.bossHp01 >= 0.0f)
            ImGui::Text("boss tier %d  hp %.0f%%", snap.enemyByType[5] > 0 ? run.bossTier : 0,
                        static_cast<double>(run.bossHp01 * 100.0f));
        else
            ImGui::TextDisabled("no boss alive");
    }

    if (ImGui::CollapsingHeader("Enemies", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text("alive %d/%d  elites %d  edge arrows %d", static_cast<int>(snap.enemyCount),
                    static_cast<int>(snap.enemyCap), snap.elitesCount, static_cast<int>(snap.edgeCount));
        ImGui::Text("chaser %d  swarm %d  shooter %d", snap.enemyByType[0], snap.enemyByType[1],
                    snap.enemyByType[2]);
        ImGui::Text("tank %d  spinner %d  boss %d", snap.enemyByType[3], snap.enemyByType[4],
                    snap.enemyByType[5]);
    }

    if (ImGui::CollapsingHeader("Bullets / Gems / Frags")) {
        ImGui::Text("shots %d/%d (homing %d)", static_cast<int>(snap.projCount),
                    static_cast<int>(snap.projCap), snap.projHoming);
        ImGui::Text("enemy shots %d/%d (homing %d)", static_cast<int>(snap.enemyBulletCount),
                    static_cast<int>(snap.enemyBulletCap), snap.enemyBulletHoming);
        ImGui::Text("gems %d/%d  fragAccum %.1f", static_cast<int>(snap.gemCount),
                    static_cast<int>(snap.gemCap), static_cast<double>(snap.fragAccum));
        ImGui::Text("frags +%d (time %d kill %d boss %d elite %d draft %d win %d)", run.fragmentsEarned,
                    run.fragsTime, run.fragsKills, run.fragsBoss, run.fragsElite, run.fragsDraft,
                    run.fragsVictory);
    }

    if (ImGui::CollapsingHeader("Director / Scaling")) {
        ImGui::Text("target %d  interval %.2fs (timer %.2f)%s", snap.targetAlive,
                    static_cast<double>(snap.spawnInterval), static_cast<double>(snap.spawnTimer),
                    snap.bossAlive ? "  BOSS: spawns x0.3" : "");
        ImGui::Text("hpMult x%.2f  dmgMult x%.2f  shotSpd %.1f", static_cast<double>(snap.hpMult),
                    static_cast<double>(snap.dmgMult), static_cast<double>(snap.enemyBulletSpeed));
        ImGui::Text("bosses spawned %d/3", snap.bossesSpawned);
    }

    if (ImGui::CollapsingHeader("Camera / World")) {
        ImGui::Text("yaw %.2f  pitch %.2f  dist %.1f->%.1f", static_cast<double>(snap.camYaw),
                    static_cast<double>(snap.camPitch), static_cast<double>(snap.camDist),
                    static_cast<double>(snap.camTargetDist));
        ImGui::Text("target (%.1f,%.1f,%.1f)  snap (%.0f,%.0f)", static_cast<double>(snap.camTarget.x),
                    static_cast<double>(snap.camTarget.y), static_cast<double>(snap.camTarget.z),
                    static_cast<double>(snap.gridSnap.x), static_cast<double>(snap.gridSnap.y));
        ImGui::Text("curveK %.3f  fill %.2f  fog %.3f  cell %.0f", static_cast<double>(curveK),
                    static_cast<double>(fill), static_cast<double>(config::kFogDensity),
                    static_cast<double>(config::kCell));
    }

    if (ImGui::CollapsingHeader("Meta (persistent)")) {
        const int bmm = static_cast<int>(meta.bestTimeSec) / 60;
        const int bss = static_cast<int>(meta.bestTimeSec) % 60;
        ImGui::Text("fragments %d  best %02d:%02d  wins %d  NG+%d", meta.fragments, bmm, bss, meta.wins,
                    meta.ngPlus);
        ImGui::Text("meta lv: %d %d %d %d %d %d", meta.levels[0], meta.levels[1], meta.levels[2],
                    meta.levels[3], meta.levels[4], meta.levels[5]);
    }

    if (ImGui::CollapsingHeader("Input")) {
        ImGui::Text("captured %d  firing %d  allowed-state %s", snap.captured ? 1 : 0,
                    snap.firing ? 1 : 0, uiStateName(snap.state));
        ImGui::Text("imgui mouse %d  kbd %d", snap.wantsMouse ? 1 : 0, snap.wantsKeyboard ? 1 : 0);
    }

    if (ImGui::CollapsingHeader("Cheats", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (actions.godMode) ImGui::Checkbox("God mode (no damage)", actions.godMode);
        if (ImGui::Button("Heal full")) {
            if (actions.onHealFull) actions.onHealFull();
        }
        ImGui::SameLine();
        if (ImGui::Button("Kill non-boss")) {
            if (actions.onKillAll) actions.onKillAll();
        }
        if (ImGui::Button("Clear enemy bullets")) {
            if (actions.onClearEnemyBullets) actions.onClearEnemyBullets();
        }
        ImGui::SameLine();
        if (ImGui::Button("+1 level (XP)")) {
            if (actions.onGrantLevel) actions.onGrantLevel();
        }
        ImGui::SameLine();
        if (ImGui::Button("+1 min (time)")) {
            if (actions.onAddMinute) actions.onAddMinute();
        }
        if (ImGui::Button("Spawn Boss-5")) {
            if (actions.onSpawnBoss) actions.onSpawnBoss();
        }
        ImGui::TextDisabled("Spawn enemy:");
        static const char *kSpawnNames[5] = {"Chaser", "Swarm", "Shooter", "Tank", "Spinner"};
        for (int i = 0; i < 5; ++i) {
            if (i > 0) ImGui::SameLine();
            if (ImGui::Button(kSpawnNames[i])) {
                if (actions.onSpawnEnemy) actions.onSpawnEnemy(i);
            }
        }
        static const int kBossTiers[3] = {5, 10, 15};
        for (int i = 0; i < 3; ++i) {
            if (i > 0) ImGui::SameLine();
            char label[16];
            snprintf(label, sizeof(label), "Boss-%d", kBossTiers[i]);
            if (ImGui::Button(label)) {
                if (actions.onSpawnBossTier) actions.onSpawnBossTier(kBossTiers[i]);
            }
        }
    }

    timers.debug = clk.ms();
    ImGui::End();
}

void drawHub(Meta &meta, const std::function<void()> &onStart, const std::function<void()> &onBuy,
             UiPanelMs &timers) {
    PanelClock clk;
    const ImVec2 center(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);
    ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(460, 0), ImGuiCond_FirstUseEver);
    ImGui::Begin("TINY PLANET — HUB", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings);

    ImGui::Text("Fragments: %d", meta.fragments);
    // M4: progress line (best run, victories, NG+ when unlocked by a win).
    {
        const int bmm = static_cast<int>(meta.bestTimeSec) / 60;
        const int bss = static_cast<int>(meta.bestTimeSec) % 60;
        if (meta.ngPlus > 0)
            ImGui::Text("Best %02d:%02d  Wins %d  NG+%d", bmm, bss, meta.wins, meta.ngPlus);
        else
            ImGui::Text("Best %02d:%02d  Wins %d", bmm, bss, meta.wins);
    }
    ImGui::Separator();

    for (int i = 0; i < kMetaCount; ++i) {
        const int lvl = meta.levels[static_cast<size_t>(i)];
        const int mx = metaMax(i);
        ImGui::PushID(i);
        ImGui::Text("%s  (%d/%d)", metaName(i), lvl, mx);
        ImGui::TextDisabled("%s", metaDesc(i));
        const bool maxed = lvl >= mx;
        const int cost = maxed ? 0 : costFor(i, lvl);
        const bool afford = !maxed && meta.fragments >= cost;
        ImGui::BeginDisabled(!afford);
        char btn[64];
        if (maxed)
            snprintf(btn, sizeof(btn), "MAX");
        else
            snprintf(btn, sizeof(btn), "Buy — %d", cost);
        if (!maxed && ImGui::Button(btn)) {
            meta.fragments -= cost;
            meta.levels[static_cast<size_t>(i)] = lvl + 1;
            if (onBuy) onBuy();  // M4: each purchase is a save point (06)
        }
        ImGui::EndDisabled();
        ImGui::Separator();
        ImGui::PopID();
    }

    if (ImGui::Button("START RUN", ImVec2(-1, 40))) onStart();
    ImGui::TextDisabled("START RUN, then click the scene to capture the mouse.");
    ImGui::TextDisabled("ESC/P pauses. After ESC, wait ~1s before re-clicking.");
    ImGui::End();
    timers.hub = clk.ms();
}

void drawDraft(const std::array<DraftOption, 3> &opts, const std::function<void(int)> &onPick,
               UiPanelMs &timers) {
    PanelClock clk;
    ImGui::OpenPopup("LEVEL UP — pick 1 of 3");
    const ImVec2 center(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal("LEVEL UP — pick 1 of 3", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        for (int i = 0; i < 3; ++i) {
            ImGui::PushID(i);
            if (ImGui::Button(opts[static_cast<size_t>(i)].name.c_str(), ImVec2(220, 60))) onPick(i);
            ImGui::SameLine();
            ImGui::Text("%s", opts[static_cast<size_t>(i)].desc.c_str());
            ImGui::PopID();
        }
        ImGui::EndPopup();
    }
    timers.draft = clk.ms();
}

void drawPause(const std::function<void()> &onResume, const std::function<void()> &onQuit,
               UiPanelMs &timers) {
    PanelClock clk;
    const ImVec2 center(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);
    ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::Begin("PAUSED", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings);
    if (ImGui::Button("Resume (P)", ImVec2(200, 0))) onResume();
    if (ImGui::Button("Quit to Hub", ImVec2(200, 0))) onQuit();
    ImGui::End();
    timers.pause = clk.ms();
}

void drawGameOver(const RunStats &run, const std::function<void()> &onRetry, const std::function<void()> &onHub,
                  UiPanelMs &timers) {
    PanelClock clk;
    const ImVec2 center(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);
    ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    // M5: victory header when the final boss died.
    ImGui::Begin(run.won ? "VICTORY — FINAL BOSS DOWN" : "RUN OVER", nullptr,
                 ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings);
    const int mm = static_cast<int>(run.timerSec) / 60;
    const int ss = static_cast<int>(run.timerSec) % 60;
    ImGui::Text("Survived %02d:%02d  — level %d, %d kills (%d elites, %d bosses)", mm, ss, run.level,
                run.kills, run.elitesKilled, run.bossKills);
    // M4/M5: fragment breakdown (02 economy) so the incremental loop is visible.
    ImGui::Text("Fragments +%d  (time %d, kills %d, elite %d, boss %d, draft %d, victory %d)",
                run.fragmentsEarned, run.fragsTime, run.fragsKills, run.fragsElite, run.fragsBoss,
                run.fragsDraft, run.fragsVictory);
    if (run.won) ImGui::Text("Difficulty unlocked: future runs hit harder, pay more.");
    if (ImGui::Button("Retry", ImVec2(200, 0))) onRetry();
    if (ImGui::Button("Hub", ImVec2(200, 0))) onHub();
    ImGui::End();
    timers.over = clk.ms();
}

void drawEdgeArrows(const EdgeMarker *markers, std::size_t count, UiPanelMs &timers) {
    PanelClock clk;
    // M5: triangles at the screen border pointing at off-screen enemies.
    if (!markers || count == 0) return;
    const ImVec2 size = ImGui::GetIO().DisplaySize;
    const ImVec2 center(size.x * 0.5f, size.y * 0.5f);
    ImDrawList *dl = ImGui::GetForegroundDrawList();
    for (std::size_t i = 0; i < count; ++i) {
        const EdgeMarker &mk = markers[i];
        const ImVec2 p((mk.x * 0.5f + 0.5f) * size.x, (1.0f - (mk.y * 0.5f + 0.5f)) * size.y);
        ImVec2 dir(p.x - center.x, p.y - center.y);
        const float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
        if (len < 1e-4f) continue;
        dir.x /= len;
        dir.y /= len;
        const float s = mk.boss ? 16.0f : 10.0f;
        const ImVec2 tip(p.x + dir.x * s, p.y + dir.y * s);
        const ImVec2 base(p.x - dir.x * s * 0.6f, p.y - dir.y * s * 0.6f);
        const ImVec2 perp(-dir.y * s * 0.6f, dir.x * s * 0.6f);
        const ImU32 col = mk.boss ? IM_COL32(255, 60, 60, 255)
                                  : (mk.elite ? IM_COL32(255, 210, 60, 255) : IM_COL32(255, 255, 255, 140));
        dl->AddTriangleFilled(tip, ImVec2(base.x + perp.x, base.y + perp.y),
                              ImVec2(base.x - perp.x, base.y - perp.y), col);
    }
    timers.edge = clk.ms();
}

}  // namespace ui
