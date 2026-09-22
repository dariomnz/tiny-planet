# 06 — LocalStorage persistence

## What is stored (meta only, never run state)

```json
{
  "version": 1,
  "fragments": 1234,
  "upgrades": { "edge": 12, "xp": 5, "haste": 3, "heart": 2, "slot2": 1, "revive": 0 },
  "bestTime": 812.5,
  "wins": 2,
  "difficultyUnlocked": 1
}
```

- Key: `"tiny_meta"`. Save on: run end + each purchase. Never per frame.
- Best/wins only to show progress and unlock difficulty.

## How (Emscripten, no libraries)

```cpp
// Meta.cpp
EM_JS(char*, js_load, (), {
  const s = localStorage.getItem("tiny_meta");
  if (!s) return 0;
  const n = lengthBytesUTF8(s) + 1;
  const p = _malloc(n);
  stringToUTF8(s, p, n);
  return p;
});
EM_JS(void, js_save, (const char* s), {
  localStorage.setItem("tiny_meta", UTF8ToString(s));
});
```

- Minimal hand-rolled JSON parse/serialize (the struct is flat, ~7 fields).
- `version` to migrate if the schema changes. On parse failure → defaults + log.
- Menus/shop and overlays (draft, run end) in DOM (`shell.html` + JS),
  not OpenGL. C++ exposes `applyUpgrade(id)`, `buyMeta(id)`, `startRun()`.

## Difficulty unlock (after victory)

- NG+1: `hpMult x1.5`, `dmgMult x1.2`, Fragments x1.5. NG+n stacks.
- Stored in `difficultyUnlocked`.
