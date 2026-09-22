# 04 — Enemies and scaling

## Types (MVP)

All reuse the current cube + curvature shader. Only color/size/AI change.
Collisions in 2D (`xy`), ignoring curved `z`.

| Type | Behavior | Base HP | Damage | Speed | Color |
|---|---|---|---|---|---|
| Chaser | Moves toward player | 20 | 10 contact | 3.5 | red |
| Swarm | Groups of 8-12, fast, 1 HP | 1 | 5 contact | 5.5 | orange |
| Shooter | Keeps ~12u distance, 1 slow bullet | 30 | 8 bullet | 2.5 | purple |
| Tank | Slow, pushes through | 160 (x8) | 20 contact | 1.8 | dark green |
| Spinner (elite) | Bullet spiral + slow chase | 300 | 10 bullet | 2.0 | yellow |
| Boss-5/10/15 | HP bar + 2-3 patterns per phase | `800/2500/6000` | 12-20 | 2.2 | white/red |

Enemy bullet patterns:

- Shooter: 1 aimed bullet, speed 6.
- Spinner: spiral `angle += 0.4/frame`, 2 arms → 3 arms from min 8.
- Boss-10: 5-fan + 12-ring every 4s. Boss-15: double spiral + slow missiles.

## Scaling formulas (`t` = run minutes)

```cpp
hpMult(t)  = (1 + t * 0.35) * pow(1.13, t);   // min 15 ≈ x12
dmgMult(t) = (1 + t * 0.12);                  // intentionally slow
bulletSpeed(t) = min(14.0f, 6.0f + t * 0.4f);
targetAlive(t) = 6 + t * 4;                   // director budget
spawnInterval(t) = max(0.25f, 1.2f - t * 0.07f);
bossHpMult(t) = 20.0f * pow(1.18, t);
```

- Pattern extras: +1 bullet in fans every 3 min.
- Elites: from min 5, 5% of spawns (x6 HP, x5 XP, +2 Fragments).

## Director

- Keeps `targetAlive(t)` enemies alive. Spawns on a 25-35u ring around the player,
  off-screen when possible.
- Schedule: mini-boss min 5, boss min 10, final boss min 15. During bosses,
  normal spawns at 30%.
- Hard caps (perf): enemies 256, enemy bullets 400, player bullets 256,
  gems 300. When a pool is full, skip spawning (enemies stop firing before
  the frame can blow up).

## Balance target

- Player with no defense dies in ~3 hits at min 12.
- With average defensive build: 6-8 hits. Death comes from density/mistakes,
  not one-shots.
