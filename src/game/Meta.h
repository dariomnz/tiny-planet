#pragma once

#include <array>
#include <string>

// M4: permanent meta progression (docs/game-plan/06-persistence.md).
// Meta only — run state is never persisted. Saved on run end + each
// purchase, never per frame. `Meta::load()` falls back to defaults
// (fresh localStorage, parse failure) and logs to stdout.
struct Meta {
    int fragments = 0;
    // levels per meta upgrade, same order as UiScreens::kMetaNames.
    std::array<int, 6> levels{0, 0, 0, 0, 0, 0};
    float bestTimeSec = 0.0f;
    int wins = 0;
    int ngPlus = 0;  // == difficultyUnlocked (06); M5 wires the victory hook

    static Meta defaults() { return Meta{}; }

    // Plain key=value lines (one per line, .ini-style). Returns "" when
    // there is no save. Unknown lines are ignored, missing keys keep
    // defaults, wrong version (no "v=1") resets to defaults.
    std::string serialize() const;
    static Meta deserialize(const char *s);

    // Best/wins bookkeeping at run end. `won` lands in M5 (final boss);
    // until then every run records won=false.
    void recordRun(float timeSec, bool won);

    void save() const;  // -> localStorage "tiny_meta"
    static Meta load();  // <- localStorage "tiny_meta" or defaults
};
