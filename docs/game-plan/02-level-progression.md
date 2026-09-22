# 02 — In-run level progression

## Target pacing

| Minute | Expected level | What happens |
|---|---|---|
| 0-2 | 1-5 | Slow chasers, low HP. Learn to move and collect |
| 2-5 | 6-12 | + Shooters (1 bullet), + swarms. Mini-boss at min 5 |
| 5-10 | 13-22 | + Spinners, + tanks, x2 density. Boss at min 10 |
| 10-15 | 23-35 | Elites, combined patterns. Final boss at min 15 |

## XP formula

```cpp
// n = current level (1-based). XP needed to go from n to n+1:
xpNeed(n) = floor(5 + pow(n, 1.6) * 3)
```

- Approx. cumulative total for level 30: ~1100 XP.
- Normal gem = 1 XP, elite = 5 XP, boss = 25 XP.
- Tune via `config::kXpBase`, `kXpPow` in `Config.h` (see 05-architecture).

## Level rules

- On level-up: pause run (`emscripten_pause_main_loop`), DOM overlay with 3 options.
- If all upgrades are maxed: offer "Heal 30% + 10 Fragments" fallback.
- Base magnet: 2.5u radius. Gems fly to the player inside the radius.
- Out-of-range gems despawn after 20s (avoids unbounded lists).

## Run economy

- Fragments (meta) earned from: time (`2/min`), kills (`0.1/kill`),
  elites (`2`), bosses (`15/40/100`), victory (`+100`).
- Typical run dying at min 8: ~40-60 Fragments.
- Winning run (min 15): ~180-220 Fragments.
