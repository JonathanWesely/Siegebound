# TASK-1408 — [IA-MENU-LRB-ASSETS] — art-director handoff

marker: `TASK-1408-IA-MENU-LRB-ASSETS`
status on completion: `ready-for-integration`
law: `KBD-§2a` cl. 5 · `HELP-§4` · `VER-§8` cl. 2 · `SC-§68` · `SC-§118`

---

## (6) THE `VER-§8` cl. 2 TWIN — DECLARED FIRST, AS OWED

🧑 **NO HUMAN KEYSTROKE IS OWED.** The MCP writes reached disk on their own.

The gate was the digest, not the return value. `IMC_MainMenu.uasset`'s `sha256`
**CHANGED** and all three new `IA_` assets exist on disk with their own bytes.
Dirty-package census after the last write returned `[]` — nothing is sitting
unsaved in PID 5728 waiting for a `Ctrl+S`.

```
unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages()  ->  []
```

This row therefore carries **no** deferred human save. If a later reader finds
these mappings absent, the cause is not an unsaved package.

---

## (5) THE DIGEST GATE — `IMC_MainMenu.uasset`

| | sha256 | bytes | mtime |
|---|---|---|---|
| BEFORE | `c0da00d3a5b5de4731369cc8a506f8f3b39c3439f52faf22ee68dc5d218cd1d9` | 4176 | 2026-09-14 14:08:58.655194300 -0700 |
| AFTER  | `6d09ac2541042612f52e84104db2c42cdc636ab72b70a8eaf67fc287076fd2aa` | 6571 | 2026-09-24 20:31:40.747210000 -0700 |

**ASSERTION: CHANGED.** `c0da00d3… ≠ 6d09ac25…`. +2395 bytes.
(Per `SC-§68` the digest is the gate; the +2395 bytes are reported but are
**not** the evidence, and the six `status: success` returns are **not** the
evidence either.)

### The three new assets (created this row)

| path on disk | sha256 | bytes | mtime |
|---|---|---|---|
| `Content/Input/Actions/IA_MenuLeft.uasset`  | `7ae96d2272ad16f642cb0692d36802e6c78cdc64f0a4d7827fee82ea8fbe4238` | 1122 | 2026-09-24 20:30:47.851243800 -0700 |
| `Content/Input/Actions/IA_MenuRight.uasset` | `3fe8abbebb67971332d1189bda9f1d39a68872988ad88e0534b72b92aace5dca` | 1126 | 2026-09-24 20:30:48.174336500 -0700 |
| `Content/Input/Actions/IA_MenuBack.uasset`  | `ef78c0511285d7d392d8079187b7d74c01f675b677cc6c4826821fbb9fc6c5ee` | 1122 | 2026-09-24 20:30:48.449074300 -0700 |

All three were **absent** before this row. Measured pre-state:

```
$ ls Content/Input/Actions/ | grep -iE 'MenuLeft|MenuRight|MenuBack'
NONE (as measured)
```

---

## (1) THE THREE ASSET PATHS — READ BACK FROM THE EDITOR

⛔ These are the literals `TASK-1409` references. Character-for-character:

```
/Game/Input/Actions/IA_MenuLeft
/Game/Input/Actions/IA_MenuRight
/Game/Input/Actions/IA_MenuBack
```

Object paths as the mapping context stores them:

```
/Game/Input/Actions/IA_MenuLeft.IA_MenuLeft
/Game/Input/Actions/IA_MenuRight.IA_MenuRight
/Game/Input/Actions/IA_MenuBack.IA_MenuBack
```

### Shape — matched against the existing sibling, not assumed

`IA_MenuUp` was read first and the three new assets were created to match it:

| property | `IA_MenuUp` (existing) | `IA_MenuLeft` / `IA_MenuRight` / `IA_MenuBack` (new) |
|---|---|---|
| class | `InputAction` | `InputAction` |
| `value_type` | `BOOLEAN` (Digital) | `BOOLEAN` (Digital) |
| `consume_input` | `True` | `True` |
| `trigger_when_paused` | `False` | `False` |
| `triggers` | `[]` | `[]` (len 0) |
| `modifiers` | `[]` | `[]` (len 0) |

EMPTY `Triggers` is the correct encoding of the spec's `ETriggerEvent::Started`:
with no explicit trigger the Enhanced Input default pressed-state trigger
supplies `Started` / `Triggered` / `Completed`, exactly as the existing three
do. `Started` is selected on the **binding** side, which is `TASK-1409`'s row.

---

## (2) THE MAPPINGS — APPENDED TO `/Game/Input/IMC_MainMenu`

**`mapping_count: 6 → 12`**, read back live from the editor with
`get_input_mapping_context_keys`.

Rows **0–5 unchanged and in their original array order** (compare against the
pre-state read quoted in §(4) below — identical, row for row):

```
 0  IA_MenuUp      Up                          Boolean
 1  IA_MenuUp      Gamepad_DPad_Up             Boolean
 2  IA_MenuDown    Down                        Boolean
 3  IA_MenuDown    Gamepad_DPad_Down           Boolean
 4  IA_MenuAccept  Enter                       Boolean
 5  IA_MenuAccept  Gamepad_FaceButton_Bottom   Boolean
```

Rows **6–11 appended** by this row:

```
 6  IA_MenuLeft    Left                        Boolean
 7  IA_MenuLeft    Gamepad_DPad_Left           Boolean
 8  IA_MenuRight   Right                       Boolean
 9  IA_MenuRight   Gamepad_DPad_Right          Boolean
10  IA_MenuBack    Backspace                   Boolean
11  IA_MenuBack    Gamepad_FaceButton_Right    Boolean
```

Each of the six new rows carries EMPTY `Triggers` and EMPTY `Modifiers`,
matching the existing six (verified per-row, `trg=0 mod=0`).

### ⚠️ Note for whoever reads this asset in Python

In UE 5.8 the `InputMappingContext.mappings` property is **deprecated and reads
back as an empty array**:

```
WARN: Property 'mappings' on 'InputMappingContext' is deprecated:
      Use the DefaultKeyMappings struct instead.
count = 0
```

The live array is `default_key_mappings.mappings` (a struct field, not a
top-level array). A reader who checks the old property will measure `0` and
wrongly conclude nothing was written. The MCP tool
`get_input_mapping_context_keys` reads the correct one.

---

## (4) THE FREE-KEY PROOF — QUOTED, NOT ASSUMED

Per `HELP-§4` and the `IA_CmdAmbush`/`F` precedent (third application), the six
keys were **proven free in the asset before anything was written**.
`get_input_mapping_context_keys('/Game/Input/IMC_MainMenu')`, run **first**:

```
mappings:
  IA_MenuUp      Up
  IA_MenuUp      Gamepad_DPad_Up
  IA_MenuDown    Down
  IA_MenuDown    Gamepad_DPad_Down
  IA_MenuAccept  Enter
  IA_MenuAccept  Gamepad_FaceButton_Bottom
mapping_count: 6
```

Occupied set = { `Up`, `Gamepad_DPad_Up`, `Down`, `Gamepad_DPad_Down`, `Enter`,
`Gamepad_FaceButton_Bottom` }.

Keys this row claims = { `Left`, `Gamepad_DPad_Left`, `Right`,
`Gamepad_DPad_Right`, `Backspace`, `Gamepad_FaceButton_Right` }.

**Intersection = ∅. No conflict. Nothing was stomped, nothing was flagged.**
No `UnmapKey`, no `UnmapAll`, no whole-array rewrite was used — six independent
append calls, one key each (`KBD-§1`, the `TASK-445` defect avoided).

---

## (3) WHY NOT `Escape` — FLAGGED, NOT TAKEN

⛔ **`IA_MenuBack` binds `Backspace` + `Gamepad_FaceButton_Right`. It does NOT
bind `Escape`, and this row did not re-open that question.**

`AS-§6 A-2` keeps `Escape` **permanently unabsorbed project-wide**
(`AccountMenuWidget.h:170-171`). Binding it from an asset edit would silently
overturn a written law. Two independent reasons it stays unbound here:

1. It is 🧑 **his call**, not an agent's — and an `.uasset` edit is exactly the
   kind of quiet change that would overturn it without anyone noticing.
2. `Escape` is the editor's **PIE-stop key**. Absorbing it globally would take
   the agent lane's own escape hatch away in the very sitting whose goal is
   agent parity on the menus.

🧑 **If he expects `Escape` to back out of a menu, that is an `AS-§6`
amendment and his decision.** The binding ships usable either way — `Backspace`
and the gamepad face button both work today.

---

## ⚠️ ONE MEASURED NOTE: the `Backspace` spelling

The row specifies `Backspace`. The engine's canonical spelling is `BackSpace`
(capital S) — `InputCoreTypes.cpp:34`:

```
const FKey EKeys::BackSpace("BackSpace");
```

The stored value therefore reads back as `Backspace`, which differs in case
from the engine constant. **This resolves correctly and is not a defect.**
Measured, not assumed:

`InputCoreTypes.h:109,112` — key identity is `FName`-based, and `FName`
comparison and hashing are case-insensitive:

```
friend bool   operator==(const FKey& KeyA, const FKey& KeyB) { return KeyA.KeyName == KeyB.KeyName; }
friend uint32 GetTypeHash(const FKey& Key)                   { return GetTypeHash(Key.KeyName); }
```

Empirical confirmation in the live editor, **with a controlled negative**:

```
hash(stored 'Backspace') == hash('BackSpace')        -> True    (same key)
hash(stored 'Backspace') == hash('Bogus_ZZZ_NotAKey')-> False   (control: the probe discriminates)
```

⛔ **Do not use Python `==` on an `FKey` as a probe — it is not a value
comparison.** It returned `False` even when comparing the shipped, known-working
`Up` key against an exact-cased copy of itself:

```
stored 'Up' vs 'Up'   -> == False | hash True
stored 'Up' vs 'up'   -> == False | hash True
stored 'Up' vs 'Down' -> == False | hash False
```

`hash()` tracks the engine's real `GetTypeHash(FKey)` and discriminates
(`Up` ≠ `Down`); the Python `==` is useless here and would have produced a
false alarm. Recording this so the next reader does not "fix" a working key.

No further write was made to normalise the casing: correcting a single stored
row in place is not an append, and the fence is append-only.

---

## (7) FENCES — HELD

- ✅ No `UnmapKey` / `UnmapAll` / whole-array rewrite.
- ✅ `IMC_Hero` untouched — `168dbf4f1523f2e6…`, 15013 bytes, mtime
  `2026-09-02 20:01:54` (unchanged).
- ✅ `IMC_Default`, `IMC_MouseLook`, `IA_MenuUp`, `IA_MenuDown`, `IA_MenuAccept`
  all byte-identical to pre-state (digests + mtimes re-read after the writes).
- ✅ No code, no C++, no Blueprint logic, no git, no compile.
- ✅ No PIE. `is_pie_active -> false` checked before the first write
  (`VER-§2` cl. 2 — no verification was live).
- ✅ No editor lifecycle action. PID 5728 was never closed or restarted.
- ✅ Editor identified **by command line** (`SC-§118`): PID 5728,
  `UnrealEditor.exe … GitClaudeUnrealTest.uproject` — the GUI editor, **not** a
  `-game` instance of 🧑 his.

---

## FOR INTEGRATION — `TASK-1409`

The three action assets exist and are non-null soft-reference targets. Bind
them beside the existing three. **Copy these literals character-for-character;
a typo resolves `null` and nothing errors:**

```
IA_MenuLeft
IA_MenuRight
IA_MenuBack
```

The actions are **authored but unbound** — no code references them yet. That is
`TASK-1409`'s deliverable and was deliberately not done here.

## Not examined / limitations

- **The keys were never pressed.** This row authored asset bytes; it ran no PIE
  and injected no input. That the mappings *fire* is `TASK-1413`'s to observe,
  and it cannot be claimed from anything in this handoff.
- **`Gamepad_*` bindings are unexercised.** No controller was attached and no
  gamepad input was injected; the gamepad rows are asserted only as correctly
  stored asset data.
- **No validity API was available.** `KismetInputLibrary` is not exposed to
  Python in this build, so no direct `Key_IsValid` call was possible. The
  `Backspace` argument above rests on engine source plus a hash probe with a
  control — strong, but it is not the engine answering "this key is registered".
- The remaining five keys (`Left`, `Right`, `Gamepad_DPad_Left`,
  `Gamepad_DPad_Right`, `Gamepad_FaceButton_Right`) match engine canonical
  spelling exactly and raise no casing question.
