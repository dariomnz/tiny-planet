# 03 — In-run and meta upgrades

## A. In-run (draft 1-of-3)

Base player stats: HP 100, basic damage 10, fire rate 2/s, bullet speed 20, player speed 8.

| # | Upgrade | Effect per level (max 5 unless noted) |
|---|---|---|
| 1 | Basic damage | +15% damage |
| 2 | Fire rate | +12% fire rate |
| 3 | Extra projectile | +1 spread projectile (max 3, then +10% damage) |
| 4 | Heavy bullets | +10% bullet speed and size |
| 5 | Nova | 8-bullet ring every 6s. -0.5s CD / level |
| 6 | Orbitals | 1 spinning orb (max 3). +1 orb at lv. 1 and 3, +damage otherwise |
| 7 | Auto missiles | 1 homing missile every 3s. Alternate +1 missile / +damage |
| 8 | Boots + Magnet | +8% speed, +30% pickup radius |
| 9 | Vitality | +20 max HP and heal 20. Regen +0.5 HP/s at lv. 3+ |
| 10 | Critical | +5% crit chance, x2 damage |

### Anti-snowball rules

- Draft weighting: if you already have >= 3 offensive upgrades, weight defensive/utility x1.5.
- Never offer maxed upgrades. Never offer a locked skill (see meta unlocks).
- Expected final DPS: 15-20x the starting value around level 30.

## B. Permanent meta (Fragments, LocalStorage)

| Upgrade | Effect / level | Cost | Max |
|---|---|---|---|
| Base edge | +2% base damage | `10 * 1.8^n` | 50 |
| XP hunger | +3% XP | `10 * 1.8^n` | 30 |
| Haste | +1% speed | `15 * 1.9^n` | 20 |
| Extra heart | +10 starting max HP | `25 * 2.0^n` | 10 |
| 2nd skill slot | unlocks equipping a 2nd auto skill | 200 one-time | 1 |
| Revive | 1x auto-revive at 50% HP per run | 500 one-time | 1 |

- Costs in `config::kMetaBaseCost[]`, formulas in `game/Stats.h`.
- Purchases only outside runs (DOM menu). Save immediately (see 06-persistence).
