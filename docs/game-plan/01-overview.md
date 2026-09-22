# 01 — Overview and core loop

## Fantasy
You survive alone on a tiny planet against growing waves. You dodge, position,
and build your loadout. Every run makes you stronger even when you lose.

## Core loop (30 seconds)
```
Dodge bullets/enemies → kill → collect XP gems → level up →
pick 1 of 3 upgrades → more DPS/survivability → survive next wave
```

## Meta loop (between runs)
```
Die or kill final boss → earn Fragments → buy permanent upgrade →
next run goes further
```

## Conditions

- **Defeat:** HP <= 0. End of run, you keep earned Fragments.
- **Victory:** kill the final Boss (min 15). Big Fragment bonus + difficulty unlock.
- **Target duration:** 10-15 min. Timer visible in HUD.

## Design pillars

1. **Dodging > aiming.** Basic fire is hold-click, skills are automatic.
   Player skill is positioning.
2. **Visible builds.** Few upgrades but ones that change bullet patterns (extra
   projectile, nova, orbitals, missiles).
3. **Death by density, not by one-shot.** Enemy damage scales slowly, enemy
   count and bullet density scale fast.
4. **60 fps on web.** Fixed pools, zero per-frame allocs, entity caps.

## Target controls

| Input | Action |
|---|---|
| WASD | Move (camera-relative, as now) |
| Mouse | Rotate camera |
| Hold left click | Basic fire (new: currently click-only) |
| Wheel | Zoom (already exists) |
| ESC | Release mouse / pause |
| Nothing | Skills: fire automatically when cooldown ends |
