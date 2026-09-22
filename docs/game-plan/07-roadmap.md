# 07 — Roadmap by milestones

## M1 — Minimal playable loop (1-2 sessions)
- [ ] Hold-click fires with `fireRate` (change in `mouse_button_callback` + `frame()`).
- [ ] Player bullet pool 256 (migrate current `vector`).
- [ ] 1 Chaser enemy (pool 256, chase AI, contact damage).
- [ ] Player HP + death + run restart + timer in HUD.
- [ ] Verify: 60 fps, 2D collisions, no per-frame allocs.
- **Out:** you can kill and die. No XP yet.

## M2 — XP and draft
- [ ] Gems + magnet + `xpNeed(n)` + levels.
- [ ] Pause + 3-option DOM overlay + 3 upgrades (damage / fire rate / nova).
- [ ] "Heal + fragments" fallback when everything is maxed.
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

## "Ready to balance" bar
15-min run at stable 60 fps with 200+ bullets on screen and draft working.
Everything else is tuning the numbers in this plan.
