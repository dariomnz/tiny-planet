#include "ui/UiScreens.h"

#include <imgui.h>

namespace ui {

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

void drawHud(const RunStats &run, float &curveK, float &fill, bool mouseCaptured) {
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
    if (run.bossHp01 >= 0.0f) ImGui::ProgressBar(run.bossHp01, ImVec2(200, 0), "BOSS-5");
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

    // Debug / tuning panel (replaces Q/E/R/F keys discoverability).
    ImGui::SetNextWindowPos(ImVec2(10, 150), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowBgAlpha(0.6f);
    ImGui::Begin("Tuning", nullptr, ImGuiWindowFlags_NoSavedSettings);
    ImGui::SliderFloat("curveK", &curveK, 0.0f, 0.2f, "%.3f");
    ImGui::SliderFloat("fill", &fill, 0.0f, 1.0f, "%.2f");
    ImGui::TextDisabled("WASD move | Hold-click fire | P pause | L level | K die");
    // Live capture diagnostics (remote debugging aid).
    {
        const ImGuiIO &io = ImGui::GetIO();
        ImGui::TextDisabled("Lock:%s ImGuiMouse:%d Mouse:(%.0f,%.0f) FB:(%.0fx%.0f)", mouseCaptured ? "ON" : "OFF",
                            io.WantCaptureMouse ? 1 : 0, static_cast<double>(io.MousePos.x),
                            static_cast<double>(io.MousePos.y), static_cast<double>(io.DisplaySize.x),
                            static_cast<double>(io.DisplaySize.y));
    }
    ImGui::End();
}

void drawHub(Meta &meta, const std::function<void()> &onStart, const std::function<void()> &onBuy) {
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
}

void drawDraft(const std::array<DraftOption, 3> &opts, const std::function<void(int)> &onPick) {
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
}

void drawPause(const std::function<void()> &onResume, const std::function<void()> &onQuit) {
    const ImVec2 center(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);
    ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::Begin("PAUSED", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings);
    if (ImGui::Button("Resume (P)", ImVec2(200, 0))) onResume();
    if (ImGui::Button("Quit to Hub", ImVec2(200, 0))) onQuit();
    ImGui::End();
}

void drawGameOver(const RunStats &run, const std::function<void()> &onRetry, const std::function<void()> &onHub) {
    const ImVec2 center(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);
    ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::Begin("RUN OVER", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings);
    const int mm = static_cast<int>(run.timerSec) / 60;
    const int ss = static_cast<int>(run.timerSec) % 60;
    ImGui::Text("Survived %02d:%02d  — level %d, %d kills", mm, ss, run.level, run.kills);
    // M4: fragment breakdown (02 economy) so the incremental loop is visible.
    ImGui::Text("Fragments +%d  (time %d, kills %d, boss %d, draft %d)", run.fragmentsEarned, run.fragsTime,
                run.fragsKills, run.fragsBoss, run.fragsDraft);
    if (ImGui::Button("Retry", ImVec2(200, 0))) onRetry();
    if (ImGui::Button("Hub", ImVec2(200, 0))) onHub();
    ImGui::End();
}

}  // namespace ui
