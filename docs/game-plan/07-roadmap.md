# 07 — Roadmap by milestones

## M1 — Minimal playable loop (1-2 sessions)
- [x] Hold-click fires with `fireRate` (`Input::isFiring/m_firingHeld` + `Game::m_fireTimer` in `update()`).
- [x] Player bullet pool 256 (fixed `array<Projectile,256>` + swap-remove, skip when full).
- [x] 1 Chaser enemy (`world/Enemies.h/.cpp` pool 256, chase AI, contact damage + iframes).
- [x] Player HP + death + run restart + timer in HUD (`startRun` resets, `collideEnemiesPlayer`, `gameOver`, `RunStats.timerSec`).
- [x] Verify: build passes (`emcmake` + `cmake --build`); 2D circle collisions `dist2D < r1+r2`; no per-frame allocs (fixed pools, stack-only render copies).
- **Out:** you can kill and die. No XP yet.

## M2 — XP and draft
- [x] Gems + magnet + `xpNeed(n)` + levels (`world/Pickups.h/.cpp` pool 300, magnet
  2.5u @10u/s, despawn 20s, kill drops 1-XP gem, `config::xpNeed(n)=floor(5+n^1.6*3)`).
- [x] Pause + 3-option overlay + 3 upgrades (damage +15% / fire rate +12% / nova
  8-ring, max 5 each). Overlay is the ImGui `Draft` modal, not a DOM overlay:
  same behavior (world frozen, pick 1 of 3), no `shell.html`/JS bridge needed.
- [x] "Heal 30% + 10 Fragments" fallback fills empty slots when maxed (all 3 when fully maxed).
- **Out:** first real build choice.

## M3 — Director + bullet hell
- [ ] `Director` (budget, interval, boss schedule).
- [ ] Shooter + Swarm + Tank + enemy bullets (pool 400).
- [ ] Patterns: aimed, fan, ring. Boss at min 5.
- **Out:** it is a bullet hell now.

## M4 — Meta + persistence
- [ ] `Meta.h/.cpp` + EM_JS load/save + DOM shop + run-end screen.
- [ ] Fragment economy (time/kills/bosses).
- [ ] Best time / wins / NG+.
- **Out:** it is incremental now.

## M5 — Content and balance
- [ ] Spinner + elites + Boss-10/15 + full 10 upgrades + slot2/revive.
- [ ] Tune with the 02/03/04 formulas. Edge arrows for off-screen enemies.
- [ ] Perf test with full caps on large canvas.
- **Out:** 15-min run winnable with a decent build, losable with a bad one.

## Suggested code order
`Config.h` → bullet pool → `g_firing` → Chaser → HP/death → gems/XP → DOM draft →
Director → enemy bullets → Meta → rest.
(M1 done in this order. `g_firing` lives as `InputManager::m_firingHeld`/`isFiring()`.
M1 spawner is a minimal inline director in `EnemySystem::trySpawn`; M3 replaces it
with the real `Director`. Kill reward is a temporary +1 fragment/kill; M4 wires the
real time/kill/boss economy.)
(M2 notes: draft overlay stayed ImGui instead of DOM — same pause/pick flow without
the `emscripten_pause_main_loop` + `shell.html` bridge. `L` is now a debug "grant
level" key. XP-hunger meta (+3%/lv) already applies to gem values; nova shares the
player bullet pool so caps/skip rules hold.)

## "Ready to balance" bar
15-min run at stable 60 fps with 200+ bullets on screen and draft working.
Everything else is tuning the numbers in this plan.
