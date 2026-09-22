# 05 — Technical architecture

## Principles

- `src/Config.h` = balance single source of truth. No magic numbers in `Game/`.
- Fixed pools (`std::array`), zero `new`/`erase` per frame. Swap-remove.
- Enemies/bullets/gems live in 2D (`xy`). Curved `z` is visual only (current shader).
- `main.cpp` gets thinner: currently monolithic (~850 lines). Extract `game/` in phases.

## Target structure

```
src/
  Config.h            // + balance: XP, scaling, cooldowns, caps, meta costs
  game/
    Stats.h           // PlayerStats, EnemyDef, UpgradeDef + inline formulas
    Director.h/.cpp   // budget, wave timer, boss schedule
    Enemies.h/.cpp    // fixed pool, chaser/shooter/spinner AI, contact
    Bullets.h/.cpp    // player/enemy pools, patterns (fan/ring/spiral)
    Pickups.h/.cpp    // XP gems + magnet + despawn
    Skills.h/.cpp     // nova/orbital/missile cooldowns, auto-fire
    Meta.h/.cpp       // Meta struct, load/save (uses 06 persistence)
    Run.h/.cpp        // run state: timer, level, draft, earned fragments
  ui/
    Hud.h/.cpp        // HP bar, timer, level, boss bar (reuse current bitmap HUD)
```

## Concrete changes to current code

1. **Basic fire (`main.cpp:63-74, 322-334`):**
   `mouse_button_callback` sets `g_firing=true/false` (down/up).
   In `frame()`: `fireTimer -= dt; if (g_firing && fireTimer<=0) spawn; fireTimer=1/fireRate`.
2. **Projectiles:** migrate `vector<Projectile>` to fixed pool `array<PlayerBullet,256>`
   + new `array<EnemyBullet,400>`. Same cube VAO, different color per side.
3. **Collisions:** 2D circles. `dist2D < r1+r2`. O(n*m) is fine with n<300 at 60fps.
   No spatial grid in M1-M3.
4. **Level draft:** `emscripten_pause_main_loop()` + DOM overlay in `shell.html`
   with 3 buttons → C++ callback applies upgrade and `emscripten_resume_main_loop()`.
   Simpler than clickable OpenGL HUD.
5. **Curvature:** enemies and bullets use the same `curveK`/`playerPos` as the player.
   No shader changes in M1.

## New constants in `Config.h` (proposal)

```cpp
kXpBase=5.0f; kXpPow=1.6f; kXpMult=3.0f;
kHpBase=100.0f; kDmgBase=10.0f; kFireRateBase=2.0f;
kEnemyCap=256; kEnemyBulletCap=400; kPlayerBulletCap=256; kGemCap=300;
kSpawnBase=1.2f; kSpawnMin=0.25f;
kNovaCd=6.0f; kMissileCd=3.0f;
```
