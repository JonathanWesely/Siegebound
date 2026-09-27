# TASK-1508 — [IA-MENU-SECONDARY-ASSET] — art-director handoff

marker: `TASK-1508-IA-MENU-SECONDARY-ASSET`
status on completion: `done` (per the orchestrator's dispatch; this satisfies the `ready-for-integration` gate that `TASK-1510` / `TASK-1514` name)
law: `KBD-§1` · `KBD-§2a` cl. 5 · `HELP-§4` · `SC-§68` · `SC-§118` · `SC-§125` · `VER-§2` cl. 2 · `AS-§6 A-2`
ruling consumed: `DECK-§3` marker `DECK-3-KEYBOARD-SET-ACTIVE-RULED` (1 hit in `CONVENTIONS.md` at dispatch). The key is Jonathan's first-hand pick: `Home` + gamepad Y.

---

## (5) SAVE DECLARATION — FIRST, AS OWED (`SC-§125` / `VER-§8` cl. 2 twin)

🧑 **NO HUMAN SAVE KEYSTROKE IS OWED.** The MCP writes reached disk on their own, the same outcome as `TASK-1408`.

The digest is the gate, not the return value: `IMC_MainMenu.uasset` sha256 **CHANGED** and `IA_MenuSecondary.uasset` exists on disk with its own bytes. The dirty-package census after the last write:

```
unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages()  ->  []
unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages()      ->  []
```

The census read `[]` / `[]` BEFORE the first write too, so nothing was dirty going in and nothing is left unsaved in PID 19396. `L_Arena` was never dirtied and never saved. If a later reader finds these mappings absent, the cause is not an unsaved package.

---

## (4) THE DIGEST GATE — `Content/Input/IMC_MainMenu.uasset`

| | sha256 | bytes | mtime |
|---|---|---|---|
| BEFORE | `6d09ac2541042612f52e84104db2c42cdc636ab72b70a8eaf67fc287076fd2aa` | 6571 | 2026-09-24 20:31:40.747210000 -0700 |
| AFTER  | `1a1ee5aff7be1b3485510ab88c2097de6120e34f7d182289132b60eb1db8dbaa` | 7379 | 2026-09-26 16:23:49.419875000 -0700 |

**ASSERTION: CHANGED.** `6d09ac25… ≠ 1a1ee5af…`, +808 bytes. The BEFORE digest equals `TASK-1408`'s AFTER digest byte-for-byte, so nothing had touched this asset between the two rows. (`SC-§68`: the digest is the gate. The byte delta and the three `success` returns are reported but are **not** the evidence.)

### The new asset (created this row)

| path on disk | sha256 | bytes | mtime |
|---|---|---|---|
| `Content/Input/Actions/IA_MenuSecondary.uasset` | `e3195fc4729d223d5de22e8678673345d38d6bdbfc4afa3628b046b8696432a9` | 1142 | 2026-09-26 16:23:42.584521800 -0700 |

Absent before this row. Measured pre-state:

```
$ ls Content/Input/Actions/ | grep -i secondary
IA_MenuSecondary: NONE (as measured)
EditorAssetLibrary.does_asset_exist('/Game/Input/Actions/IA_MenuSecondary') -> False
```

---

## (1) THE ASSET PATH — READ BACK FROM THE EDITOR

⛔ `TASK-1507` loads this by literal string, so a typo resolves to null and nothing errors. Read back after creation:

```
EditorAssetLibrary.does_asset_exist('/Game/Input/Actions/IA_MenuSecondary') -> True
load_asset(...).get_path_name() -> /Game/Input/Actions/IA_MenuSecondary.IA_MenuSecondary
```

That object path is character-identical to the pinned constant `TEXT("/Game/Input/Actions/IA_MenuSecondary.IA_MenuSecondary")` on `TASK-1507`.

### Shape: matched against `IA_MenuAccept`, read first rather than assumed

| property | `IA_MenuAccept` (existing) | `IA_MenuSecondary` (new) |
|---|---|---|
| class | `InputAction` | `InputAction` |
| `ValueType` | `BOOLEAN` (Digital) | `BOOLEAN` (Digital) |
| `bConsumeInput` | `True` | `True` |
| `bTriggerWhenPaused` | `False` | `False` |
| `Triggers` | len 0 | len 0 |
| `Modifiers` | len 0 | len 0 |

Empty `Triggers` is the correct encoding of the spec's `Started`. With no explicit trigger, the default pressed-state trigger supplies `Started` / `Triggered` / `Completed`, the same as the six sibling actions. `Started` is selected on the **binding** side (`TASK-1507`).

---

## (2) THE MAPPINGS — APPENDED TO `/Game/Input/IMC_MainMenu`

**`mapping_count: 12 → 14`**, re-measured live at this row (`SC-§138`: the board's `12` was a sighting and it held). Read back with `get_input_mapping_context_keys` and cross-checked through `DefaultKeyMappings.Mappings` with per-row trigger/modifier counts.

Rows **0–11 unchanged and in their original array order** (identical to the pre-state read in §(3), row for row, all `trg=0 mod=0`):

```
 0  IA_MenuUp        Up                          trg=0 mod=0
 1  IA_MenuUp        Gamepad_DPad_Up             trg=0 mod=0
 2  IA_MenuDown      Down                        trg=0 mod=0
 3  IA_MenuDown      Gamepad_DPad_Down           trg=0 mod=0
 4  IA_MenuAccept    Enter                       trg=0 mod=0
 5  IA_MenuAccept    Gamepad_FaceButton_Bottom   trg=0 mod=0
 6  IA_MenuLeft      Left                        trg=0 mod=0
 7  IA_MenuLeft      Gamepad_DPad_Left           trg=0 mod=0
 8  IA_MenuRight     Right                       trg=0 mod=0
 9  IA_MenuRight     Gamepad_DPad_Right          trg=0 mod=0
10  IA_MenuBack      Backspace                   trg=0 mod=0
11  IA_MenuBack      Gamepad_FaceButton_Right    trg=0 mod=0
```

Rows **12–13 appended** by this row:

```
12  IA_MenuSecondary Home                        trg=0 mod=0
13  IA_MenuSecondary Gamepad_FaceButton_Top      trg=0 mod=0
```

Both stored key names match the engine's canonical spelling exactly (`InputCoreTypes.cpp`), so the `TASK-1408` `Backspace` casing question does not arise here:

```
45:  const FKey EKeys::Home("Home");
188: const FKey EKeys::Gamepad_FaceButton_Top("Gamepad_FaceButton_Top");
```

Two independent append calls, one key each (transactions `C62943F0408D309E47097C8B9F05BB4D`, `A0223E51466543D221D02D8F8CC4F276`). No `UnmapKey`, no `UnmapAll`, no whole-array rewrite (`KBD-§1`).

⚠️ Reminder carried from `TASK-1408`: in UE 5.8 `InputMappingContext.mappings` is deprecated and reads back **empty** in Python. The live array is `DefaultKeyMappings.Mappings`. A reader who checks the old property will measure `0` and wrongly conclude nothing was written.

---

## (3) THE FREE-KEY PROOF — QUOTED, TAKEN BEFORE ANY WRITE (`HELP-§4`)

Keys this row claims = { `Home`, `Gamepad_FaceButton_Top` }.

**`IMC_MainMenu`**, `get_input_mapping_context_keys('/Game/Input/IMC_MainMenu.IMC_MainMenu')`, run first:

```
IA_MenuUp      Up                          | IA_MenuUp      Gamepad_DPad_Up
IA_MenuDown    Down                        | IA_MenuDown    Gamepad_DPad_Down
IA_MenuAccept  Enter                       | IA_MenuAccept  Gamepad_FaceButton_Bottom
IA_MenuLeft    Left                        | IA_MenuLeft    Gamepad_DPad_Left
IA_MenuRight   Right                       | IA_MenuRight   Gamepad_DPad_Right
IA_MenuBack    Backspace                   | IA_MenuBack    Gamepad_FaceButton_Right
mapping_count: 12
```

Occupied set ∩ claimed set = **∅**.

**`IMC_Hero`** (read-only), `get_input_mapping_context_keys('/Game/Input/IMC_Hero.IMC_Hero')`, `mapping_count: 28`. Keys present:
`SpaceBar, W, S, A, D, Mouse2D, LeftShift, LeftMouseButton, One, RightMouseButton, Escape, Two, Three, Four, Five, Six, LeftAlt, Q, T, R, E, F, C, Enter, M, Tab, B, H`.
**`Home`: absent. `Gamepad_FaceButton_Top`: absent.**

Extra sightings beyond the spec's two IMCs (read-only):
- `IMC_Default` (12 mappings: `SpaceBar, Gamepad_FaceButton_Bottom, W, S, A, D, Up, Down, Right, Left, Gamepad_Left2D, Gamepad_Right2D`): neither key present.
- `IMC_MouseLook` (1 mapping: `Mouse2D`): neither key present.
- `Grep '\bHome\b|Gamepad_FaceButton_Top'` over `Config/`: no matches.
- `Grep 'EKeys::Home|Gamepad_FaceButton_Top'` over `Source/` at ~16:20: no matches. This is a sighting only, since `TASK-1507` is writing `Source/` in parallel and is expected to add exactly these references.

**No conflict. Nothing was stomped, nothing was flagged.**

---

## (6) FENCES — HELD

- ✅ No `UnmapKey` / `UnmapAll` / whole-array rewrite.
- ✅ `IMC_Hero` untouched: `168dbf4f1523f2e6…da313`, 15013 bytes, mtime `2026-09-02 20:01:54.807069400`, identical before and after.
- ✅ `IMC_Default` (`133bda65…`, 7787), `IMC_MouseLook` (`6a00157c…`, 2521), `IA_MenuAccept` (`9c891897…`, 1130), `IA_MenuUp` (`d0efdfa8…`, 1114) and `IA_MenuBack` (`ef78c051…`, 1122) were re-hashed after the writes and all are byte-identical to pre-state.
- ✅ `Escape` not bound (`AS-§6 A-2`). `Home` + gamepad Y only.
- ✅ No code, no C++, no Blueprint logic, no compile, no git.
- ✅ No PIE. `is_pie_active -> is_active: false` checked before the first write (`VER-§2` cl. 2).
- ✅ No save of `L_Arena` or any asset other than the two named. `get_dirty_map_packages() -> []` before and after.
- ✅ No editor lifecycle action. PID 19396 was never closed or restarted.
- ✅ Editor identified **by command line** (`SC-§118`): PID 19396, created 2026-09-26 14:32:34, `"C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor.exe" "C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\GitClaudeUnrealTest.uproject"`. That is the GUI editor, **not** a `-game` instance of 🧑 his, and the only `UnrealEditor*.exe` running.

---

## FOR INTEGRATION — `TASK-1507` / `TASK-1510` / `TASK-1514`

- `TASK-1507`: the target exists and is a non-null load target. Copy the literal character-for-character: `/Game/Input/Actions/IA_MenuSecondary.IA_MenuSecondary`. The action is **authored but unbound**. No code references it yet, which is `TASK-1507`'s deliverable.
- `TASK-1510`: "confirm `IA_MenuSecondary` resolves (read-only)". The expected object path is the one above; this row already measured `does_asset_exist -> True`.
- `TASK-1514` (ship host): the two files this row produced are `Content/Input/Actions/IA_MenuSecondary.uasset` (new) and `Content/Input/IMC_MainMenu.uasset` (modified). Verify each by oid-vs-sha256 against the AFTER digests above (`SC-§68`), never by size. The UE Git plugin may already have auto-staged both on save. Commit by pathspec and verify on `git show --stat HEAD`, never the index. This row ran no git and did not check the index.

## Not examined / limitations

- **The keys were never pressed.** This row authored asset bytes. It ran no PIE and injected no input. That `IA_MenuSecondary` *fires* is `TASK-1511`'s to observe (A1), and it cannot be claimed from anything in this handoff.
- **The gamepad row is unexercised.** No controller was attached and no gamepad input was injected. `Gamepad_FaceButton_Top` is asserted only as correctly stored asset data with canonical spelling.
- **The `Source/` free-key grep is a sighting at one instant**, taken while `TASK-1507` edits `Source/` in parallel. The binding proof this row owes (`HELP-§4`) is the asset read in §(3), which does not depend on it.
- **Slate-side collisions: one sighting, not a proof.** This row proves the keys free in the Enhanced Input assets and `Config/`. An asset read cannot see whether a Slate widget consumes a real `Home` press first. The one known engine consumer is `SListView` ("jump to first item", `SListView.h:599` `if ( InKeyEvent.GetKey() == EKeys::Home )`), and at source neither deck-builder container is a list view: the bar is `TObjectPtr<class UHorizontalBox> DeckBar` (`DeckBuilderWidget.h:149`) and the card grid is a `WrapBox` (`DeckBuilderWidget.h:330/647`). Any other ancestor (e.g. a scroll box) was not examined. The Slate door belongs to `TASK-1507` / `TASK-1509`, and the real key closes on 🧑 his hand check (`TASK-1511` A5).
