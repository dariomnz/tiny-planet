#include "Game.h"

#include <cmath>

#include "Config.h"

// Debug panel support + cheats. Lives here (not in Game.cpp) so gameplay
// code stays free of debug clutter. All functions are Game members, so
// they can read/touch run state directly.

DebugSnapshot Game::buildDebugSnapshot() const {
    DebugSnapshot s;
    s.playerPos = m_player.pos();
    s.playerYaw = m_player.yaw();
    s.damage = m_damage;
    s.fireRate = m_fireRate;
    s.fireTimer = m_fireTimer;
    s.bulletSpeed = m_bulletSpeed;
    s.bulletRadius = m_bulletRadius;
    s.playerSpeed = m_playerSpeed;
    s.magnetRadius = m_magnetRadius;
    s.critCh = m_critCh;
    s.novaTimer = m_novaTimer;
    s.missileTimer = m_missileTimer;
    s.orbAngle = m_orbAngle;
    s.orbCount = m_orbCount;
    s.invulnTimer = m_invulnTimer;
    s.fragAccum = m_fragAccum;
    s.reviveUsed = m_reviveUsed;
    s.upgLevels = {m_upgDamage, m_upgFire, m_upgNova, m_upgExtra, m_upgHeavy,
                   m_upgOrb,    m_upgMis,  m_upgBoots, m_upgVit,  m_upgCrit};
    s.xpNeed = config::xpNeed(m_run.level);

    s.enemyCount = m_enemies.size();
    s.enemyCap = config::kEnemyCap;
    s.elitesCount = 0;
    s.bossAlive = false;
    for (std::size_t i = 0; i < m_enemies.size(); ++i) {
        const Enemy &e = m_enemies.data()[i];
        int idx = 0;
        switch (e.type) {
            case EnemyType::Chaser:
                idx = 0;
                break;
            case EnemyType::Swarm:
                idx = 1;
                break;
            case EnemyType::Shooter:
                idx = 2;
                break;
            case EnemyType::Tank:
                idx = 3;
                break;
            case EnemyType::Spinner:
                idx = 4;
                break;
            case EnemyType::Boss:
                idx = 5;
                break;
        }
        s.enemyByType[static_cast<std::size_t>(idx)] += 1;
        if (e.elite) ++s.elitesCount;
        if (e.type == EnemyType::Boss) s.bossAlive = true;
    }

    s.projCount = m_projectiles.size();
    s.projCap = config::kProjMax;
    s.projHoming = 0;
    for (std::size_t i = 0; i < m_projectiles.size(); ++i)
        if (m_projectiles.data()[i].homing) ++s.projHoming;

    s.enemyBulletCount = m_enemyBullets.size();
    s.enemyBulletCap = config::kEnemyBulletCap;
    s.enemyBulletHoming = 0;
    for (std::size_t i = 0; i < m_enemyBullets.size(); ++i)
        if (m_enemyBullets.data()[i].homing) ++s.enemyBulletHoming;

    s.gemCount = m_gems.size();
    s.gemCap = config::kGemCap;
    s.edgeCount = m_edgeCount;

    const float t = m_run.timerSec / 60.0f;
    s.timeMin = t;
    s.targetAlive = config::targetAlive(t);
    s.spawnInterval = config::spawnInterval(t) * (s.bossAlive ? 3.3333333f : 1.0f);
    s.hpMult = config::hpMult(t);
    s.dmgMult = config::dmgMult(t);
    s.enemyBulletSpeed = config::bulletSpeed(t);
    s.spawnTimer = m_director.spawnTimer();
    s.bossesSpawned = m_director.bossesSpawned();
    s.ngHp = m_ngHp;
    s.ngDmg = m_ngDmg;
    s.fragMult = m_fragMult;

    s.camYaw = m_camera.yaw;
    s.camPitch = m_camera.pitch;
    s.camDist = m_camera.distance;
    s.camTargetDist = m_camera.targetDistance;
    s.camTarget = m_camera.target;
    s.gridSnap = {std::floor(m_player.pos().x / config::kCell + 0.5f) * config::kCell,
                  std::floor(m_player.pos().y / config::kCell + 0.5f) * config::kCell};

    s.pendingDrafts = m_pendingDrafts;
    s.captured = m_input.isCaptured();
    s.firing = m_input.isFiring();
    s.wantsMouse = ImGuiLayer::wantsMouse();
    s.wantsKeyboard = ImGuiLayer::wantsKeyboard();
    s.fbW = m_window.fbWidth();
    s.fbH = m_window.fbHeight();
    s.state = m_state;

    s.perfUpdate = m_perfAvgUpdate;
    s.perfSim = m_perfAvgSim;
    s.perfCollide = m_perfAvgCollide;
    s.perfPlanet = m_perfAvgPlanet;
    s.perfEntSubmit = m_perfAvgSubmit;
    s.perfEntFlush = m_perfAvgFlush;
    s.perfUi = m_perfAvgUi;
    s.perfTotal = m_perfAvgTotal;
    s.perfUiPanels = m_perfUiAvg;
    // Unwrap ring (oldest -> newest) for the graph.
    s.histN = m_perfHistCount;
    static constexpr int kHist = DebugSnapshot::kPerfHist;
    for (int i = 0; i < m_perfHistCount; ++i) {
        const int idx = (m_perfHistHead - m_perfHistCount + i + kHist * 2) % kHist;
        s.histTotal[static_cast<std::size_t>(i)] = m_perfHistTotal[static_cast<std::size_t>(idx)];
        s.histUpdate[static_cast<std::size_t>(i)] = m_perfHistUpdate[static_cast<std::size_t>(idx)];
        s.histRender[static_cast<std::size_t>(i)] = m_perfHistRender[static_cast<std::size_t>(idx)];
        s.histUi[static_cast<std::size_t>(i)] = m_perfHistUi[static_cast<std::size_t>(idx)];
    }
    return s;
}

DebugActions Game::debugActions() {
    DebugActions a;
    a.godMode = &m_godMode;
    a.showDebug = &m_showDebug;
    a.onHealFull = [this] { healFull(); };
    a.onKillAll = [this] { killAllNonBoss(); };
    a.onClearEnemyBullets = [this] { clearEnemyBullets(); };
    a.onGrantLevel = [this] { grantLevel(); };
    a.onSpawnBoss = [this] { spawnBoss5(); };
    a.onAddMinute = [this] { addMinute(); };
    a.onSpawnEnemy = [this](int typeIdx) { spawnEnemy(typeIdx); };
    a.onSpawnBossTier = [this](int tier) { spawnBoss(tier); };
    return a;
}

void Game::healFull() { m_run.hp = m_run.maxHp; }

void Game::killAllNonBoss() {
    for (std::size_t n = m_enemies.size(); n-- > 0;) {
        if (m_enemies.data()[n].type == EnemyType::Boss) continue;
        if (!onEnemyKilled(n)) return;  // run ended (victory can't come from here, be safe)
    }
}

void Game::clearEnemyBullets() { m_enemyBullets.clear(); }

void Game::grantLevel() {
    // Top up exactly the missing XP so one press = one level, reusing the
    // existing progress. addXp(0) then runs the normal level-up path
    // (exact, no float-mult rounding issues from dividing by the hunger bonus).
    const int need = config::xpNeed(m_run.level);
    if (m_run.xp < static_cast<float>(need)) m_run.xp = static_cast<float>(need);
    addXp(0.0f);
}

void Game::spawnBoss5() { spawnBoss(5); }

void Game::spawnBoss(int tier) {
    const float t = m_run.timerSec / 60.0f;
    const glm::vec2 p(m_player.pos().x + config::kBossSpawnDist, m_player.pos().y);
    m_enemies.spawn(EnemySystem::make(EnemyType::Boss, p, m_ngHp, m_ngDmg * config::dmgMult(t), false, tier));
}

void Game::spawnEnemy(int typeIdx) {
    // Debug spawner: one enemy of the requested type in front of the player,
    // scaled like a natural Director spawn at the current clock.
    EnemyType type = EnemyType::Chaser;
    switch (typeIdx) {
        case 0:
            type = EnemyType::Chaser;
            break;
        case 1:
            type = EnemyType::Swarm;
            break;
        case 2:
            type = EnemyType::Shooter;
            break;
        case 3:
            type = EnemyType::Tank;
            break;
        case 4:
            type = EnemyType::Spinner;
            break;
        default:
            return;
    }
    const float t = m_run.timerSec / 60.0f;
    const glm::vec2 dir(std::cos(m_camera.yaw), std::sin(m_camera.yaw));
    const glm::vec2 p = m_player.pos() + dir * 10.0f;
    m_enemies.spawn(
        EnemySystem::make(type, p, config::hpMult(t) * m_ngHp, config::dmgMult(t) * m_ngDmg));
}

void Game::addMinute() { m_run.timerSec += 60.0f; }

void Game::perfPushFrame() {
    static constexpr int kHist = DebugSnapshot::kPerfHist;
    const float renderMs =
        static_cast<float>(m_perfPlanetMs + m_perfEntSubmitMs + m_perfEntFlushMs);
    m_perfHistTotal[m_perfHistHead] = static_cast<float>(m_perfTotalMs);
    m_perfHistUpdate[m_perfHistHead] = static_cast<float>(m_perfUpdateMs);
    m_perfHistRender[m_perfHistHead] = renderMs;
    m_perfHistUi[m_perfHistHead] = static_cast<float>(m_perfUiMs);
    m_perfHistHead = (m_perfHistHead + 1) % kHist;
    if (m_perfHistCount < kHist) ++m_perfHistCount;

    m_perfWinUpdate += m_perfUpdateMs;
    m_perfWinSim += m_perfSimMs;
    m_perfWinCollide += m_perfCollideMs;
    m_perfWinPlanet += m_perfPlanetMs;
    m_perfWinSubmit += m_perfEntSubmitMs;
    m_perfWinFlush += m_perfEntFlushMs;
    m_perfWinUi += m_perfUiMs;
    m_perfWinTotal += m_perfTotalMs;
    m_perfUiWin.snap += m_perfUiLast.snap;
    m_perfUiWin.newFrame += m_perfUiLast.newFrame;
    m_perfUiWin.uiRender += m_perfUiLast.uiRender;
    m_perfUiWin.uiGL += m_perfUiLast.uiGL;
    m_perfUiWin.hub += m_perfUiLast.hub;
    m_perfUiWin.hud += m_perfUiLast.hud;
    m_perfUiWin.debug += m_perfUiLast.debug;
    m_perfUiWin.draft += m_perfUiLast.draft;
    m_perfUiWin.pause += m_perfUiLast.pause;
    m_perfUiWin.over += m_perfUiLast.over;
    m_perfUiWin.edge += m_perfUiLast.edge;
    ++m_perfWinFrames;
}

void Game::perfTickWindow(double now) {
    if (m_perfWinLast <= 0.0) m_perfWinLast = now;
    if (now - m_perfWinLast < 0.5 || m_perfWinFrames <= 0) return;
    const double n = static_cast<double>(m_perfWinFrames);
    m_perfAvgUpdate = static_cast<float>(m_perfWinUpdate / n);
    m_perfAvgSim = static_cast<float>(m_perfWinSim / n);
    m_perfAvgCollide = static_cast<float>(m_perfWinCollide / n);
    m_perfAvgPlanet = static_cast<float>(m_perfWinPlanet / n);
    m_perfAvgSubmit = static_cast<float>(m_perfWinSubmit / n);
    m_perfAvgFlush = static_cast<float>(m_perfWinFlush / n);
    m_perfAvgUi = static_cast<float>(m_perfWinUi / n);
    m_perfAvgTotal = static_cast<float>(m_perfWinTotal / n);
    m_perfUiAvg.snap = m_perfUiWin.snap / static_cast<float>(m_perfWinFrames);
    m_perfUiAvg.newFrame = m_perfUiWin.newFrame / static_cast<float>(m_perfWinFrames);
    m_perfUiAvg.uiRender = m_perfUiWin.uiRender / static_cast<float>(m_perfWinFrames);
    m_perfUiAvg.uiGL = m_perfUiWin.uiGL / static_cast<float>(m_perfWinFrames);
    m_perfUiAvg.hub = m_perfUiWin.hub / static_cast<float>(m_perfWinFrames);
    m_perfUiAvg.hud = m_perfUiWin.hud / static_cast<float>(m_perfWinFrames);
    m_perfUiAvg.debug = m_perfUiWin.debug / static_cast<float>(m_perfWinFrames);
    m_perfUiAvg.draft = m_perfUiWin.draft / static_cast<float>(m_perfWinFrames);
    m_perfUiAvg.pause = m_perfUiWin.pause / static_cast<float>(m_perfWinFrames);
    m_perfUiAvg.over = m_perfUiWin.over / static_cast<float>(m_perfWinFrames);
    m_perfUiAvg.edge = m_perfUiWin.edge / static_cast<float>(m_perfWinFrames);
    m_perfUiWin = UiPanelMs{};
    m_perfWinUpdate = m_perfWinSim = m_perfWinCollide = 0.0;
    m_perfWinPlanet = m_perfWinSubmit = m_perfWinFlush = 0.0;
    m_perfWinUi = m_perfWinTotal = 0.0;
    m_perfWinFrames = 0;
    m_perfWinLast = now;
}
