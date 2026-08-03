# TASK-445 — [W1-B4] `IA_AssistantConsole` + the `IMC_Hero` `Enter` mapping — artist handoff

**Agent:** art-director · **Date:** 2026-08-03 · **Status on exit:** `ready-for-integration`
**Law implemented:** CONVENTIONS "In-match LLM command assistant" §2 (strictly-additive) + §5 (naming/folder law) · flagged item (d) FLAG-never-stomp.
**Asset-only. No C++, no compile, no Git write. `L_Arena` never opened, never saved.**

---

## 1. ⚖️ THE ENTER VERDICT — `Enter` WAS FREE. NOTHING WAS STOMPED.

**`Enter` is not bound anywhere in this project.** It was mapped as a genuinely new binding, not over anything.

Evidence, gathered **before** the write:

| Mapping context | `defaultKeyMappings` | legacy `mappings` | profile overrides | Enter/Return? |
|---|---|---|---|---|
| `/Game/Input/IMC_Hero` | 23 | 0 | 0 | **NO** |
| `/Game/Input/IMC_Default` | 12 | 0 | 0 | **NO** |
| `/Game/Input/IMC_MouseLook` | 1 | 0 | 0 | **NO** |
| `/Game/Variant_Combat/Input/IMC_Combat` | 16 | 0 | 0 | **NO** |
| `/Game/Variant_Platforming/Input/IMC_Platforming` | 14 | 0 | 0 | **NO** |
| `/Game/Variant_SideScrolling/Input/IMC_SideScroller` | 9 | 0 | 0 | **NO** |

`IMC_Hero` keys BEFORE (23): `A C D E Escape F Five Four LeftAlt LeftMouseButton LeftShift Mouse2D One Q R RightMouseButton S Six SpaceBar T Three Two W`. No `Enter`, no `Return`, no `NumPadEnter`.

Also checked, both clean:
- `Config/DefaultInput.ini` — **no** `ActionMappings`/`AxisMappings` at all. Only `bAltEnterTogglesFullscreen=True` (engine **Alt+Enter** fullscreen toggle — a *modified* chord, not a bare-`Enter` binding, so **not** a conflict).
- Code-side grep over `Source/` + `Plugins/*/Source/` for `EKeys::Enter` / `EKeys::Return` / `EKeys::Virtual_Accept` / `NumPadEnter` → **zero hits**.

**No escalation was needed. Jonathan's key decision was not required, because there was no collision to decide.**

---

## 2. Assets delivered — exact object paths

| Asset | `/Game/` object path | Disk | Action |
|---|---|---|---|
| `IA_AssistantConsole` | `/Game/Input/Actions/IA_AssistantConsole.IA_AssistantConsole` | `Content/Input/Actions/IA_AssistantConsole.uasset` (1 214 B) | **CREATED** |
| `IMC_Hero` | `/Game/Input/IMC_Hero.IMC_Hero` | `Content/Input/IMC_Hero.uasset` (12 702 → **13 179 B**) | **EDITED** (one mapping appended) |

Both saved by **explicit 2-path `save_assets`** — never save-all. Both read back `is_dirty == false`.

### Path equality vs what the C++ composes — character-for-character

`SiegePlayerController.cpp:180` composes
`TEXT("/Game/Input/Actions/IA_AssistantConsole.IA_AssistantConsole")`; `load_asset` returns
`"/Game/Input/Actions/IA_AssistantConsole.IA_AssistantConsole"`. **Both 59 chars, compared programmatically — IDENTICAL.**
Location verified against the shipped `IA_Cmd*` fleet (same `Content/Input/Actions/` folder), not inferred.

Reference graph closes the loop both ways:
- `get_referencers("/Game/Input/Actions/IA_AssistantConsole")` → `["/Game/Input/IMC_Hero"]`
- `get_dependencies("/Game/Input/IMC_Hero")` → 21 entries, **including** `/Game/Input/Actions/IA_AssistantConsole`

---

## 3. `IA_AssistantConsole` settings — donor and verification

**Built by DUPLICATING `/Game/Input/Actions/IA_CmdFollow`** (newest shipped sibling, TASK-399), so it mirrors the fleet by construction rather than by re-typing.

I first read all **11** UPROPERTYs off **all six** shipped siblings (`IA_CmdFollow`, `IA_CmdAmbush`, `IA_CmdAttack`, `IA_CmdHold`, `IA_CmdDefend`, `IA_Rally`). **They are unanimous** — every property agrees across all six, so there was no fleet-vs-spec disagreement to resolve:

| Property | All 6 siblings | `IA_AssistantConsole` |
|---|---|---|
| `valueType` | `Boolean` (Digital) | **`Boolean`** ✅ |
| `triggers` | `[]` | `[]` ✅ |
| `modifiers` | `[]` | `[]` ✅ |
| `bConsumeInput` | `true` | `true` ✅ |
| `accumulationBehavior` | `TakeHighestAbsoluteValue` | same ✅ |
| `bTriggerWhenPaused` | `false` | `false` ✅ |
| `bConsumesActionAndAxisMappings` | `false` | `false` ✅ |
| `bReserveAllMappings` | `false` | `false` ✅ |
| `triggerEventsThatConsumeLegacyKeys` | `0` | `0` ✅ |
| `actionDescription` | `""` | `""` ✅ |
| `playerMappableKeySettings` | `None` | `None` ✅ |

Asset class readback: **`InputAction`**. Empty `triggers` is the fleet norm and still emits `ETriggerEvent::Started` on the press edge — exactly what TASK-449's `BindAction` uses. Adding an explicit trigger would have *deviated* from the fleet, so I did not.

---

## 4. `IMC_Hero` — BEFORE and AFTER

Priority untouched: applied by `AHeroCharacter` at priority **1** (`HeroCharacter.cpp:37`). I added a mapping *inside* the existing context.

**BEFORE — 23 mappings** (matches TASK-399's exit state exactly).
**AFTER — 24 mappings.** Entries 1–23 verified **byte-identical** by programmatic diff of the full readback (not by assertion). One entry appended:

| # | Action | Key | Triggers | Modifiers | settingBehavior | playerMappableKeySettings |
|---|---|---|---|---|---|---|
| **24** | `/Game/Input/Actions/IA_AssistantConsole.IA_AssistantConsole` | **`Enter`** | `[]` | `[]` | `InheritSettingsFromAction` | `None` |

Row shape copied from entry 23 (`IA_CmdFollow`/`C`), itself identical to the other `IA_Cmd*` rows.

### Instanced modifier subobjects — the TASK-399 hazard, re-checked

`set_properties` rewrites the **whole** array, and six rows reference instanced modifiers owned by the `IMC_Hero` package. A round-trip that re-created them with defaults would silently break WASD movement and mouse-look Y-inversion while the table still looked perfect. Snapshotted before, re-read after:

| Modifier | Used by | Before | After |
|---|---|---|---|
| `InputModifierSwizzleAxis_0` | `IA_Move`/W | `order: YXZ` | `YXZ` ✅ |
| `InputModifierSwizzleAxis_1` | `IA_Move`/S | `order: YXZ` | `YXZ` ✅ |
| `InputModifierNegate_0` | `IA_Move`/S | X✓ Y✓ Z✓ | X✓ Y✓ Z✓ ✅ |
| `InputModifierNegate_1` | `IA_Move`/A | X✓ Y✓ Z✓ | X✓ Y✓ Z✓ ✅ |
| `InputModifierNegate_2` | `IA_Look`/Mouse2D | X✗ **Y✓** Z✗ | X✗ **Y✓** Z✗ ✅ |

`Negate_2` is the load-bearing one — a default-constructed Negate would have flipped X too and inverted horizontal mouse-look. **Intact.** `IMC_Hero` was confirmed `is_dirty == false` before I started, so the snapshot is a true baseline.

---

## 5. ⚠️ `L_Arena` — UNCHANGED, PROVEN BY HASH

| Checkpoint | mtime | sha256 |
|---|---|---|
| Session start (baseline) | `2026-07-29 03:53:38.9198809` | `b3dbc5d9ae484a7bd02cafad52b4681da68b011477479b65ee7781ae459f8268` |
| Immediately pre-editor-kill | same | same |
| Immediately post-editor-kill | same | same |
| Immediately before `save_assets` | same | same |
| Immediately after `save_assets` | same | same |
| **Final** | **`2026-07-29 03:53:38.9198809`** | **`b3dbc5d9…459f8268`** |

Size `535 522 B` throughout. **Never opened, never saved, never PIE'd.** Verified by hash, not just mtime.

---

## 6. 🛑 THE EDITOR DEADLOCKED MID-TASK AND WAS RESTARTED — full disclosure

**`AssetTools.duplicate` hard-deadlocked the editor (PID 26700).** Sequence from `Saved/Logs/GitClaudeUnrealTest.log`:

```
19:32:28:543  Dispatching toolset tool: 'AssetTools.duplicate'
19:32:28:886  LogFileHelpers: InternalPromptForCheckoutAndSave started...
19:32:29:001  Saving Package: /Game/Input/Actions/IA_AssistantConsole
19:32:29:008  Moving '…/Saved/IA_AssistantConsole….tmp' to '…/Content/Input/Actions/IA_AssistantConsole.uasset'
<log ends — no further output, ever>
```

The duplicate **succeeded and wrote the asset to disk**, then the editor wedged. Diagnosis before acting:
- `Responding: False`; log **not growing** for 5+ minutes
- **108 of 108 threads in `Wait`**, ~3.4 % of one core → hard deadlock, not a busy operation
- **No modal dialog existed** — window enumeration for PID 26700 returned only the main window (Visible=True, Enabled=True). So this was *not* the "Save Content" trap, and there was nothing to click.

**Action:** terminated PID 26700 under the session-scoped grant, then relaunched. Termination was also the *safest* option for `L_Arena` — it discards the in-memory dirt rather than risking a save prompt. Hashes above prove nothing reached disk.

- **New editor: PID 7844**, launched 12:39:49, `Responding: True`, owns `127.0.0.1:8000`. **Left RUNNING.**
- The old PID **26700 is dead** — any doc or task referencing it is stale.
- ⚠️ **The resident LLM model is gone** (the restart unloaded it). `Siege.Llama.SpikeUnload` was never needed. Anything expecting a warm model must reload it.
- The gitignored `Saved/Config/WindowsEditor/Engine.ini` Python remote-exec block was **not** re-appended — it was absent before and I used MCP, not Python remote exec, so it was never needed. **Flagging in case a later task wants it.**

**After the restart I re-verified the asset the wedged editor wrote rather than trusting it:** class `InputAction`, `is_dirty false`, path identical (59/59), and **all 11 properties matching the donor**. `IMC_Hero` re-read as 23 mappings, **byte-identical to the pre-crash baseline**. The deadlock corrupted nothing.

📌 **For whoever next uses this MCP: `AssetTools.duplicate` triggers `InternalPromptForCheckoutAndSave` and deadlocked this editor once.** It is not obviously reproducible (TASK-399 used `duplicate` successfully), but budget for it and check the log's last line before assuming a hang is your own code.

---

## 7. 🔍 MCP FINDING — `set_properties` silently returns `false` unless `values` is a JSON **string**

This cost real time and **will** bite the next agent, so it is recorded as law-grade detail.

`ObjectTools.set_properties`' declared schema is `values: {"type": "string"}` — **"A JSON formatted string of the properties to set"**. Passing a normal JSON **object** (the intuitive shape, and the shape `get_properties` *returns*) makes the call return:

```json
{"returnValue": false}
```

— **with no error message, no log line, and no mutation.** I burned three shape-variants and a full diagnostic pass on this before reading the schema.

- ⚠️ **`get_properties` is asymmetric with `set_properties`:** the getter returns a JSON string of the values, and the setter needs `values` as a JSON string too — but the getter accepts a bare path for `instance` while `set_properties` needs `{"refPath": …}`.
- ⚠️ **A `false` return is the ONLY signal.** There is no thrown error. **Any agent that does not check the return value AND re-read the data will believe it wrote and it did not.**
- Proof the fix is real, not assumed: `actionDescription` `""` → `"PROBE"` → readback `"PROBE"` → reverted to `""` → readback `""`, with `returnValue: true` each time.

Also re-confirming TASK-399's **empty-`mappings` trap**: the live array is `defaultKeyMappings.mappings`; the legacy top-level `mappings` reads `[]` on **all six** contexts. Writing the legacy array would produce an asset that dumps correctly and **binds nothing**.

---

## 8. ⚠️ THINGS I COULD NOT INDEPENDENTLY CONFIRM — read this before trusting section 2

**MCP readback has passed on broken assets on this project before, so readback is NOT my evidence of record.** What I can and cannot stand behind:

**Independently corroborated (non-MCP evidence):**
- `IMC_Hero.uasset` grew **12 702 → 13 179 B** on disk, and a raw byte scan of the file finds `IA_AssistantConsole` (×2), the full string `/Game/Input/Actions/IA_AssistantConsole` (×1), and `Enter` (×1) in its name/import table, with `IA_CmdFollow` still present. The mapping is genuinely **in the file**, not just in the editor's memory.
- `L_Arena` unchanged — by **sha256**, not mtime alone.

**MCP-only, NOT independently verified — treat as claims, not facts:**
- That entries 1–23 are semantically unchanged **at runtime**. I diffed the full property readback and the five modifier subobjects, but a readback is exactly the thing that has lied here before.
- That the `Enter` mapping actually **fires** `ETriggerEvent::Started`. **Nothing has been compiled or played.** ⛔ **This closes only at Jonathan's playtest**, and it is the criterion that matters.
- Trigger/modifier semantics of the new `InputAction` — asserted from property equality with the fleet, never executed.

**Anomaly I did NOT cause and cannot fully explain — flagged, not silently fixed:**
- `Content/Input/Actions/IA_AssistantConsole.uasset` is **STAGED in Git** (`git diff --cached` lists it; `.git/index` mtime `12:45:15`, ~17 s after my save). **I ran zero Git write commands** — only read-only `git status` / `git diff --cached` / `rev-parse` / reflog reads. **GitHub Desktop is running** (PIDs 11748/12236/3060/8980 since 10:07) and is the most plausible cause. **No commit was created** (HEAD still `c8c780c`).
- ⛔ **I deliberately did NOT unstage it** — `git reset` is itself a Git write and this task is Git-forbidden. **TASK-447 owns the commit and should be aware the file may already be in the index.**

---

## 9. Notes for integration (build-master / TASK-447)

- **No integration work beyond committing.** These are pure data assets; nothing to attach, and I wired nothing into a Blueprint (not my lane).
- **`Enter` is live data-side but still inert game-side until the module compiles.** `AssistantConsoleAction` resolves in `SetupInputComponent` (`SiegePlayerController.cpp:399`) and binds `ETriggerEvent::Started` → `OnAssistantConsolePressed` (`:481`). With the asset present, `ResolveInputAction` now succeeds. **Until TASK-447's compile, Enter does nothing — the designed pre-compile state, not a defect.**
- **Files to commit by explicit path:**
  - `Content/Input/Actions/IA_AssistantConsole.uasset` (new — ⚠️ already staged, see §8)
  - `Content/Input/IMC_Hero.uasset` (modified)
  - `.claude/pipeline/handoffs/TASK-445-artist.md` (this file)
  - the `TASK-445` status line in `.claude/pipeline/TASKBOARD.md`
- ⛔ **Do NOT let a `git add -A` sweep in `L_Arena`.** Its disk copy is pristine (§5) and must stay that way. If the editor is closed later, **decline the save prompt for `L_Arena`.**
- **No `.uasset` other than the two above was created, edited, or saved.**

### 🚩 For the manager / QA — one interaction I observed but did NOT decide

`Enter` now both **opens** the console (Enhanced Input → `OnAssistantConsolePressed`) and **submits** the typed line: `USiegeAssistantConsoleWidget` calls `InputBox->SetKeyboardFocus()` (`:844`) and commits on `ETextCommit::OnEnter` (`:625`). While the box holds focus, Slate should consume `Enter` before Enhanced Input sees it, so the toggle should not double-fire — **but that is reasoning, not a measurement, and I did not test it.** It sits next to flagged item **(t)** (Escape left unabsorbed, Jonathan's call at TASK-448). **Recorded for the playtest gate; not a blocker and not my ruling.**

---

## Acceptance self-check

| Criterion | Status |
|---|---|
| `IA_AssistantConsole` exists at the pinned path, Digital/bool | ✅ `valueType: Boolean` |
| Settings mirror a shipped sibling rather than a guess | ✅ duplicated from `IA_CmdFollow`; 11 properties verified against all 6 siblings |
| Mapped to `Enter` in the existing `IMC_Hero` | ✅ entry 24 |
| `Enter` confirmed free BEFORE mapping, FLAG-never-stomp honoured | ✅ 6 contexts + `DefaultInput.ini` + code grep; zero hits ⇒ no stomp, no escalation |
| Additive — not one existing mapping disturbed | ✅ entries 1–23 diffed identical, all 5 modifier subobjects value-verified |
| Path matches what the code composes, character-for-character | ✅ programmatic equality, 59/59 |
| Mapping-context priority unchanged | ✅ still 1, applied by `AHeroCharacter` |
| Only the two assets saved; never save-all | ✅ explicit 2-path `save_assets` |
| `L_Arena` never opened, never saved | ✅ sha256 identical at 6 checkpoints |
| No C++, no compile, no Git write | ✅ (⚠️ external staging anomaly disclosed in §8) |
| Editor handed back RUNNING, PID reported | ✅ **PID 7844** (⚠️ restarted mid-task — §6) |
