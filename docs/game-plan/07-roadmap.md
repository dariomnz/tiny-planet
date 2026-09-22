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
- [x] `Director` (`world/Director.h/.cpp`: `targetAlive(t)` budget, `spawnInterval(t)`,
  time-gated roster chaser -> +swarm(1') -> +shooter(2') -> +tank(3'), Boss-5 at min 5,
  normal spawns at 30% while boss lives).
- [x] Shooter + Swarm + Tank + enemy bullets (`world/EnemyBullets.h/.cpp` pool 400,
  skip-spawn + hold-fire when full; shooter holds ~12u; swarm groups of 8-12;
  separation pass, tanks/bosses push through).
- [x] Patterns: aimed (shooter), 5-fan + 12-ring (Boss-5, +1 fan bullet / 3 min).
  Boss HP bar in HUD, 25-XP gem + 15 fragments on kill.
- **Out:** it is a bullet hell now.

## M4 — Meta + persistence
- [x] `Meta.h/.cpp` (`src/game/Meta.h/.cpp`: one `key=value` line per save field
  under key `tiny_meta`, NOT the JSON of 06 — easier to inspect in devtools and
  to parse; round-trip proven by a native test) + EM_JS load/save (bridge in
  `platform/WebExt`, try/catch for private mode; save on run end + each purchase,
  never per frame) + shop (stays the ImGui Hub, same deviation as M2 — no
  DOM/`shell.html` bridge) + run-end breakdown screen (time/kills/boss/draft +
  kills + level).
- [x] Fragment economy (02): time 2/min + kills 0.1/kill (accumulator) + Boss-5
  tier 15 + draft fallback 10. Live counter in HUD, finalized at run end.
- [x] Best time / wins persisted and shown in Hub; NG+ counter + `recordRun(won)`
  ready, but no victory path yet — `gameOver(won=true)` lands with the M5 final
  boss (until then every run records `won=false`).
- **Out:** it is incremental now.

## M5 — Content and balance
- [x] Spinner (spiral volley, 2 arms → 3 from min 8, slow chase) + elites
  (5% from min 5: x6 HP, x5 XP, +2 frags, yellow) + Boss-10 (fan+ring / 4s) /
  Boss-15 (double counter-rotating spiral + slow homing missiles) + full 10
  upgrades (spread/Heavy/orbitals/missiles/boots/vitality/crit) + slot2 (2nd
  skill cap) / revive (50% once per run).
- [x] Tuned with the 02/03/04 formulas: `hpMult(t)`/`dmgMult(t)` on every normal
  spawn (bosses take NG+ only), `bulletSpeed`/`targetAlive`/`spawnInterval`/
  `xpNeed` live, boss tiers 15/40/100 + victory +100, NG+ x1.5/x1.2/x1.5.
  Expected DPS ~17x at full build (2 x 1.76 x 4 shots x 1.25 crit + skills).
  Edge arrows for off-screen enemies (boss red > elite yellow > white, 64 cap;
  fixed: markers replicate the shader curve drop `curveK*|rel|^2`, parity-proven
  by a native test — flat projection was 12+ units off at spawn range).
- [ ] Perf test with full caps on large canvas — MANUAL: serve `build/`, force
  late-game (survive to min 10+ or seed bosses), watch workMs in the HUD.
  Code side is ready: zero per-frame allocs held everywhere (fixed pools,
  stack arrays), worst case ~65k separation + ~65k bullet×enemy checks/frame.
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
(M3 notes: M1 inline spawner removed — Director owns all spawning on the 25-35u ring.
Contact uses per-type `kPlayerRadius + e.radius`. Spinner/elites/Boss-10/15 stay in
M5; boss schedule currently fires once at min 5.)

## "Ready to balance" bar
15-min run at stable 60 fps with 200+ bullets on screen and draft working.
Everything else is tuning the numbers in this plan.
