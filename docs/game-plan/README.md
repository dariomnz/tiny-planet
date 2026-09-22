# Game Plan — Bullet Hell Incremental (tiny-planet)

> Design source of truth. If code contradicts this plan, the plan wins
> until the plan is updated.

## Base decisions (agreed)

- **Session:** short 10-15 min runs, Vampire Survivors style.
- **Arena:** the current curved tiny-planet. No scenery change.
- **Combat:** hybrid. Basic fire on click / hold-click + auto skills on cooldown.
- **Persistence:** LocalStorage via Emscripten, no backend.
- **Platform:** web-only (Emscripten + WebGL2 / ES 3.0), C++17.

## Index

1. [Overview and core loop](01-overview.md)
2. [In-run level progression](02-level-progression.md)
3. [In-run and meta upgrades](03-upgrades.md)
4. [Enemies and scaling](04-enemies-scaling.md)
5. [Technical architecture](05-architecture.md)
6. [LocalStorage persistence](06-persistence.md)
7. [Roadmap by milestones](07-roadmap.md)

## Current prototype state

- Player = cube on curved grid (`src/main.cpp`, `src/Config.h`).
- Manual click-to-shoot, 1 projectile type, `kProjMax = 100`.
- No enemies, no XP, no upgrades, no saving.
