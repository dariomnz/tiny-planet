#include "game/Meta.h"

#include <cstdio>
#include <iostream>
#include <sstream>

#include "platform/WebExt.h"

// M4 fix: plain .ini-style lines (one key=value per line) instead of JSON —
// trivial to read in devtools -> Application -> Local Storage, and the
// parser below tolerates missing/reordered lines (keeps defaults).
std::string Meta::serialize() const {
    char buf[256];
    std::snprintf(buf, sizeof(buf),
                  "v=1\nfragments=%d\nedge=%d\nxp=%d\nhaste=%d\nheart=%d\nslot2=%d\nrevive=%d\n"
                  "best=%.1f\nwins=%d\nng=%d\n",
                  fragments, levels[0], levels[1], levels[2], levels[3], levels[4], levels[5],
                  static_cast<double>(bestTimeSec), wins, ngPlus);
    return std::string(buf);
}

namespace {

int parseInt(const std::string &v, int fallback) {
    try {
        std::size_t pos = 0;
        const int r = std::stoi(v, &pos);
        return pos == v.size() ? r : fallback;
    } catch (...) {
        return fallback;
    }
}

float parseFloat(const std::string &v, float fallback) {
    try {
        std::size_t pos = 0;
        const float r = std::stof(v, &pos);
        return pos == v.size() ? r : fallback;
    } catch (...) {
        return fallback;
    }
}

}  // namespace

Meta Meta::deserialize(const char *s) {
    Meta m;
    if (!s || !*s) return m;
    std::istringstream in(s);
    std::string line;
    bool versionOk = false;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();  // CRLF saves
        const std::size_t eq = line.find('=');
        if (eq == std::string::npos) continue;
        const std::string key = line.substr(0, eq);
        const std::string val = line.substr(eq + 1);
        if (key == "v") {
            versionOk = parseInt(val, 0) == 1;
        } else if (key == "fragments") {
            m.fragments = parseInt(val, 0);
        } else if (key == "edge") {
            m.levels[0] = parseInt(val, 0);
        } else if (key == "xp") {
            m.levels[1] = parseInt(val, 0);
        } else if (key == "haste") {
            m.levels[2] = parseInt(val, 0);
        } else if (key == "heart") {
            m.levels[3] = parseInt(val, 0);
        } else if (key == "slot2") {
            m.levels[4] = parseInt(val, 0);
        } else if (key == "revive") {
            m.levels[5] = parseInt(val, 0);
        } else if (key == "best") {
            m.bestTimeSec = parseFloat(val, 0.0f);
        } else if (key == "wins") {
            m.wins = parseInt(val, 0);
        } else if (key == "ng") {
            m.ngPlus = parseInt(val, 0);
        }  // unknown keys ignored (forward compat)
    }
    if (!versionOk) {
        std::cout << "Meta::deserialize: bad save (no v=1), using defaults" << std::endl;
        return Meta::defaults();
    }
    // Clamp to sane ranges so a hand-edited save can't break the shop.
    if (m.fragments < 0) m.fragments = 0;
    if (m.fragments > 999999) m.fragments = 999999;
    for (int &l : m.levels) {
        if (l < 0) l = 0;
        if (l > 99) l = 99;
    }
    if (m.bestTimeSec < 0.0f) m.bestTimeSec = 0.0f;
    if (m.wins < 0) m.wins = 0;
    if (m.ngPlus < 0) m.ngPlus = 0;
    return m;
}

void Meta::recordRun(float timeSec, bool won) {
    if (timeSec > bestTimeSec) bestTimeSec = timeSec;
    if (won) {
        ++wins;
        ngPlus = wins;
    }
}

void Meta::save() const {
    const std::string s = serialize();
    std::cout << "Meta::save: " << s.size() << " bytes -> localStorage [tiny_meta]" << std::endl;
    web::metaSave(s.c_str());
}

Meta Meta::load() {
    const std::string s = web::metaLoad();
    std::cout << "Meta::load: " << s.size() << " bytes <- localStorage [tiny_meta]" << std::endl;
    return Meta::deserialize(s.c_str());
}
