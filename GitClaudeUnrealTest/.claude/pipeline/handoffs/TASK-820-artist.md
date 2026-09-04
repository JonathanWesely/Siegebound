# TASK-820 — `IA_DiscardAll` + the ONE `IMC_Hero` row at the `H` position (art-director)

**Status:** COMPLETE, all proofs PASS → `ready-for-integration`
**Date:** 2026-09-02/03 · **Editor:** PID **30748**, left RUNNING, never closed/bounced/restarted
**Law applied:** `HELP-§4` · `KBD-§2a` (editor-time authoring carve) · `KBD-§4` · `KBD-§5` · `CARDBAR-§6` · `CARDBAR-§7` · `AS-§6` A-2 · `§25b`
**Route:** in-editor Python over the PythonScriptPlugin remote-execution lane (`bRemoteExecution` was **already `true`**) + Unreal MCP as the second instrument.
⛔ No C++ · no `WBP_CardHand` · no controls-menu registry · no compile · no Git · no `.umap` save.

---

## 1. ⭐⭐ MY OWN PROOF THAT `H` WAS FREE — READ IN THE ASSET, BEFORE ANY MUTATION

⛔ **I did not take TASK-818's word for it.** `HELP-§4` is explicit that a `Source/` grep is *necessary, not sufficient*, because `IMC_Hero` is binary. I read the live asset first, then re-checked the same gate a second time **inside the authoring script**, so a conflict could only ever have been **flagged**, never overwritten.

**Read via MCP `ObjectTools.get_properties` on `/Game/Input/IMC_Hero.IMC_Hero`, property `defaultKeyMappings`** (⚠️ `mappings` is `UE_DEPRECATED(5.7)` and returned **`[]`** — measured again this session, confirming TASK-568's finding still holds).

**The complete pre-append key list, verbatim, in array order — 27 rows:**

```
 1 SpaceBar          IA_Jump
 2 W                 IA_Move            modifiers: [InputModifierSwizzleAxis_0]
 3 S                 IA_Move            modifiers: [InputModifierSwizzleAxis_1, InputModifierNegate_0]
 4 A                 IA_Move            modifiers: [InputModifierNegate_1]
 5 D                 IA_Move
 6 Mouse2D           IA_Look            modifiers: [InputModifierNegate_2]
 7 LeftShift         IA_Sprint
 8 LeftMouseButton   IA_Attack
 9 One               IA_Card1
10 RightMouseButton  IA_CancelPlace
11 Escape            IA_CancelPlace
12 Two               IA_Card2
13 Three             IA_Card3
14 Four              IA_Card4
15 Five              IA_Card5
16 Six               IA_Card6
17 LeftAlt           IA_UICursor
18 Q                 IA_Rally
19 T                 IA_CmdAttack
20 R                 IA_CmdHold
21 E                 IA_CmdDefend
22 F                 IA_CmdAmbush
23 C                 IA_CmdFollow
24 Enter             IA_AssistantConsole
25 M                 IA_WarMap
26 Tab               IA_ControlsHelp
27 B                 IA_Recall
```

⇒ ✅ **`H` appears ZERO times in the asset. No conflict. Nothing was stomped, and no `FLAG` row is owed to Jonathan.**
⇒ ✅ Independently reproduces TASK-818's byte-parse **27/27** — now three sources agreeing (its offline parse, its help-registry cross-check, my live asset read).

✅ **`mappingProfileOverrides` = `{}` (EMPTY)** — recorded because `KBD-§5` makes `RetargetContextKeys` refuse **wholesale** if it is not. The positional remap is not sitting on a latent refusal.

---

## 2. ASSETS

| Asset | /Game/ path | Disk path | sha256 | bytes |
|---|---|---|---|---|
| `IA_DiscardAll` (**NEW**) | `/Game/Input/Actions/IA_DiscardAll` | `Content/Input/Actions/IA_DiscardAll.uasset` | `bfb14877d73f5c1dac875b52afd20b28b9afbe4642aa638528d9b810c0b8b688` | 1,184 |
| `IMC_Hero` (**append only**) | `/Game/Input/IMC_Hero` | `Content/Input/IMC_Hero.uasset` | `168dbf4f1523f2e65c58157d2cc39d65bfe2f81466aaf3c332ecb01ffacda313` | 15,013 |

`IMC_Hero` **pre-append** was `88f7f6e8…332fa1` / 14,552 B — an exact match to TASK-747's recorded post-append hash, so the chain of custody is unbroken.

⛔ **No other asset was created, modified or saved.** Saves were explicit and per-path. **There is no `save_assets([])` anywhere.**

### `IA_DiscardAll` — matches the shipped digital template exactly

Built by `AssetTools.duplicate` of the shipped **`IA_AssistantConsole`** (the `KBD-§2a`/TASK-568 precedent: an `InputAction` is a leaf `UDataAsset` with no parent class and no widget tree, so this is **not** the prohibited UMG duplicate-and-reparent). Duplicating the shipped template makes parity **provable by equality** rather than by my eye.

**All 11 properties read back IDENTICAL to the template — nothing invented:**

| property | `IA_AssistantConsole` | **`IA_DiscardAll`** |
|---|---|---|
| `valueType` | `Boolean` | ✅ **`Boolean`** (Digital/bool) |
| `bConsumeInput` | `true` | ✅ `true` |
| `bTriggerWhenPaused` | `false` | ✅ `false` |
| `bReserveAllMappings` | `false` | ✅ `false` |
| `triggers` / `modifiers` | `[]` / `[]` | ✅ `[]` / `[]` |
| `accumulationBehavior` | `TakeHighestAbsoluteValue` | ✅ same |
| `actionDescription` | `""` | ✅ `""` |
| `bConsumesActionAndAxisMappings` | `false` | ✅ `false` |
| `triggerEventsThatConsumeLegacyKeys` | `0` | ✅ `0` |
| `playerMappableKeySettings` | `None` | ✅ `None` |

Class readback: **`InputAction`**.

---

## 3. `KBD-§2a` — THE FOUR CONDITIONS, EACH DISCHARGED

**1. The `MapKey` call lives in a SCRATCHPAD script, never in `Source/`.** ✅
`<scratchpad>/t820_author.py`. `git status` shows **zero** `Source/` paths from me.

**2. ONE call, ONE appended row; the removal APIs stay banned.** ✅
Exactly one `imc.map_key(ia, key)`. The driver **mechanically refuses to send** any payload containing the removal symbols and printed `banned-token scan: CLEAN` on every send — the ban is enforced by the tool, not by my care.
⛔ The mappings array was **never** rewritten. That is the TASK-445 defect and the one operation MCP *would* have accepted (`DefaultKeyMappings` is `EditAnywhere`, so it is genuinely reachable — it may not be used).

**3. ⭐ SURVIVORS PROVEN BY NAMING THE MODIFIER OBJECTS, NOT THE KEYS.** ✅

| row | modifier objects AFTER the append |
|---|---|
| `IA_Move` / `W` | `InputModifierSwizzleAxis_0` |
| `IA_Move` / `S` | `InputModifierSwizzleAxis_1`, `InputModifierNegate_0` |
| `IA_Move` / `A` | `InputModifierNegate_1` |
| `IA_Move` / `D` | *(none — and none before)* |
| `IA_Look` / `Mouse2D` | `InputModifierNegate_2` |

- `proof_survivors_identical: true` (before-dict == after-dict, whole-structure compare)
- All five required objects present **by name**: `true ×5`
- MCP cross-check confirms the same objects by **refPath**, e.g. `/Game/Input/IMC_Hero.IMC_Hero:InputModifierSwizzleAxis_0`
- ⭐ **COUNT: `N` = 27 → `N+1` = 28**, with `N` **measured in the asset in the same session as the append** — `KBD-§2a`'s **RELATIVE** invariant. ⛔ I am deliberately not quoting a literal as the law's expected value; the measured `N` is the contract.
- **First 27 keys identical IN ARRAY ORDER** ✅ — and the first 27 **actions** identical in array order too (an extra check I added).
- **All 28 keys distinct** ✅ — no duplicate introduced.

**4. The appended row carries EMPTY `Triggers` and EMPTY `Modifiers`.** ✅

```
index 27 → action=IA_DiscardAll  key=H
          triggers=[]  modifiers=[]
          settingBehavior=InheritSettingsFromAction
          playerMappableKeySettings=None
```

Identical in shape to the shipped `IA_AssistantConsole` / `IA_ControlsHelp` / `IA_Recall` rows. Its emptiness is the **specification**, which is why the append cannot express the ban's failure mechanism.

`ALL_PROOFS_PASS: true`

---

## 4. ✅ `Escape` IS UNTOUCHED (`AS-§6` A-2)

Rows 10/11 were never addressed by the script — `map_key` is a pure `Add_GetRef` at the **end** of the array.

| | before | after |
|---|---|---|
| `Escape` row index (0-based) | `[10]` | `[10]` — ⛔ **unmoved** |
| `Escape` row content | `IA_CancelPlace`, triggers `[]`, modifiers `[]` | ✅ **identical** |
| `RightMouseButton` row | `IA_CancelPlace` | ✅ **identical** |

`proof_escape_rows_unmoved: true`. ⛔ The deliberate RMB/`Escape` double cover for `IA_CancelPlace` is intact.

---

## 5. ⭐⭐ READ-BACK ON FOUR INDEPENDENT PATHS (two were required)

The dispatch warned of five prior silent asset defects where the call returned success and the property was wrong. **That warning earned its keep this session — see §6.**

| # | path | mechanism | result |
|---|---|---|---|
| **A** | remote-exec Python | `default_key_mappings.mappings` walked in-process | 28 rows; index 27 = `IA_DiscardAll` / `H`, empty triggers+modifiers |
| **B** | Unreal MCP `ObjectTools.get_properties` | property system → JSON, a **different code path** | 28 rows; final row `{"action":"…/IA_DiscardAll","key":"H","triggers":[],"modifiers":[]}` |
| **C** | **offline byte scan of the saved `.uasset`** | ⛔ **no editor involved at all** | `IA_DiscardAll` present in the on-disk name table alongside all 23 pre-existing `IA_` imports |
| **D** | asset registry | `get_referencers` / `get_dependencies` | `IA_DiscardAll` ⇄ `IMC_Hero` hard reference exists in the saved package |

Plus: `is_dirty("/Game/Input/IMC_Hero")` = **`false`** ⇒ memory and disk agree.

---

## 6. ⛔⛔ THE DEFECT I CAUGHT — `MapKey` DOES NOT DIRTY THE PACKAGE, SO THE FIRST SAVE SILENTLY DID NOTHING

> ### ⛔ **`unreal.EditorAssetLibrary.save_asset(path)` RETURNED `True` AND WROTE ⛔ NOTHING. The in-memory array had 28 rows; the on-disk bytes were ⛔ byte-identical to the pre-append state.**

**How it was caught:** the disk sha256 after the "successful" save was still `88f7f6e8…332fa1` at exactly **14,552 bytes** — unchanged. Had I trusted the `True` return, or compared only the in-memory readback, **this task would have reported success while shipping `H` UNBOUND.**

**Root cause, measured:** `is_dirty("/Game/Input/IMC_Hero")` returned **`false`** *after* the append. `UInputMappingContext::MapKey` is a pure `DefaultKeyMappings.Mappings.Add_GetRef(...)` + a control-mapping rebuild request — it calls neither `Modify()` nor `MarkPackageDirty()`. `save_asset` defaults to `only_if_is_dirty=True`, so it **no-opped and returned `True` anyway.**

**The fix (applied):** `imc.modify()` followed by `save_asset(IMC_PATH, only_if_is_dirty=False)`. Disk hash then moved to `168dbf4f…cda313` at 15,013 B (+461).

⚠️ **`imc.mark_package_dirty()` does not exist** on the Python binding (`AttributeError`); `modify()` is the working route.

⭐ **Worth writing into `KBD-§2a` as a fifth condition:** *the append is not done until the on-disk bytes change.* A `save_asset` return value is **not** evidence, and `is_dirty` is the diagnostic. This is the same family as the `PKG-§9a-3` inert-key and the stale-literal traps: **a confident success signal covering a no-op.** ⛔ I have not edited `CONVENTIONS.md`.

📌 **Not a contradiction of TASK-705/747** — both landed their bytes (my pre-read hash proves TASK-747's `B` row is on disk). Whatever dirtied the package for them did not happen here. ⇒ **The hash check is the only reliable gate; do not rely on the append path happening to dirty the asset.**

---

## 7. ⚠️ TWO ENGINE-API SIGNATURES THAT DIFFER FROM THE OBVIOUS GUESS (probed, never assumed — TASK-747 `D4` again)

1. ⛔ **`unreal.Key` takes NO constructor arguments.** `unreal.Key(key_name="H")` and `unreal.Key("H")` both raise `TypeError: call() takes at most 0 arguments (1 given)`. The working route is `k = unreal.Key()` then `k.set_editor_property("key_name", "H")`. Attribute access `k.key_name` also fails — `key_name` is **reflection-only**.
2. ⚠️ **`Key.__eq__` is NOT a value comparison for a freshly constructed key.** Two *independently constructed* keys both named `B` compare **`False`**, as does constructed-`B` vs live-`B`, while live-vs-live compares `True`. ⇒ ⛔ **A struct-equality check here proves nothing and would have sent me chasing a phantom defect.** I confirmed this with controls before proceeding rather than treating the first `False` as a finding. The field that actually serializes is `key_name`, and `B`/`Tab` in this very asset were authored by this identical route and work in the shipped game.

---

## 8. ✅ THE BINDING RESOLVES THROUGH THE LAYOUT SYSTEM — ZERO EXTRA CODE, AND IT IS EXACTLY WHAT JONATHAN ASKED FOR

**His words:** *"when I said 'H', I am talking about 'H' on QWERTY, on Dvorak it would be 'D'."* ⇒ he means the **physical key position**.

`IA_DiscardAll` rides `IMC_Hero`, and `USiegeKeyboardLayoutSubsystem` retargets **only the `.Key` field, index-by-index, on a transient duplicate** of the whole context. `KBD-§4` tables **all 26 letters**, so **`H` inherits Dvorak with zero extra code**, resolving to the physical `H` position (printed `D` on US-Dvorak). The append landing at the **END** of the array is exactly why the index-by-index retarget is unaffected.

- ⛔ **I authored the QWERTY reference key `H` only.** No Dvorak variant, no second row, no profile override.
- ⛔ **No `EKeys::H` literal exists on any shipped path** — I wrote no C++ at all. `CARDBAR-§7`'s one-way-to-get-this-wrong (a bare `EKeys::H` raw poll, which fires on the physical **J** position on Dvorak) is structurally avoided because this is a **mapped** Enhanced Input action.
- ✅ **The `H` letter row keeps `IMC_Hero` punctuation-free**, so TASK-818's `§5.2` latent hole (the injectivity guard seeds `ClaimedKeys` from the 26 letter probes only and cannot see non-letter rows) is **not** approached. ⛔ Nothing in this task pushed toward a non-letter key.

---

## 9. ⭐ WHAT TASK-821 NEEDS TO RENDER THE LABEL

⛔ **Not mine** — `FSiegeControlsHelpRegistry` is C++ and is TASK-821's sole-owned surface. Stated precisely so it is one lookup, not a re-derivation:

- **Lane: `MappedAction` (lane A).** `IA_DiscardAll` is an **IMC-context** action, so its label comes from the **already-remapped context** via `QueryAppliedKeysForRow` → `ComposeKeyChipLabel` — the **same** resolver the card digits use.
- ⛔⛔ **NEVER `GetPositionalKey`** — that is the raw-polled-key API and would **double-translate** (`CARDBAR-§7`; the shipped header quantifies it: on US-Dvorak `F` → `U` → `G` and the help screen teaches the wrong key). ⛔ **Never a typed `"H"`** either.
- ⛔ **Write ZERO conditional layout logic.** `CARDBAR-§7` is explicit: any `if (bIsDigit)` or per-key special case is an **automatic QA FAIL**. The digit does not move because the subsystem's table has no digit entry; the letter moves because it does. One code path.
- **Action asset:** `/Game/Input/Actions/IA_DiscardAll` · **ActionId:** `DiscardAll` (or the registry's existing convention for an `IA_`-backed row).
- ⭐ **This feature supplies BOTH operands of `HELP-§6`'s test for the first time:** `H` must **CHANGE** under a simulated layout flip and a card digit must **HOLD**, in one test. Seam: `USiegeKeyboardLayoutSubsystem::SetTranslationMapForAutomationTests` (⚠️ it **latches** by design — a test wanting a probing subsystem constructs a fresh one). ⛔ `SC-§37`: a test asserting the label `== "H"` is a transcription and proves nothing.
- ⛔ **The fee is `DiscardAllCost`.** `HELP-§2`/`CARDBAR-§6`: if the fee is ever shown to the player it is **READ** from `DiscardAllCost`, ⛔ **never typed** — a literal `20` anywhere rots the moment the fee is retuned (the M7.7 `Notes` "in 400" vs `AoERadius` 700 precedent).

---

## 10. ⚠️ `§25b` — WHAT THE EDITOR'S SCC AUTO-STAGED, AND THE STALE-INDEX TRAP FIRED AGAIN

⛔ **I ran ZERO Git commands that mutate anything.** `git status` / `git ls-files` / `git cat-file` are read-only reporting. Both paths are LFS-tracked.

| path (repo-relative) | worktree sha256 | index state | verdict |
|---|---|---|---|
| `GitClaudeUnrealTest/Content/Input/Actions/IA_DiscardAll.uasset` | `bfb14877…c0b8b688` (1,184 B) | `A ` (auto-staged) | ✅ index LFS pointer oid **matches** the worktree — correctly staged |
| `GitClaudeUnrealTest/Content/Input/IMC_Hero.uasset` | `168dbf4f…facda313` (15,013 B) | ` M` | ⛔⛔ **STALE — index still holds the PRE-append pointer `88f7f6e8…332fa1` / 14,552 B. Re-`git add` REQUIRED.** |

⛔⛔ **A commit as-found would ship `H` UNBOUND while looking fully staged** — the exact failure TASK-709 caught and TASK-747 recorded. **Re-`git add` both and verify by oid-vs-sha256, ⛔ never by byte size** (a size check "notices" the change while certifying the wrong blob).

**Not mine, present in `git status`, listed so nobody attributes them to me:** `Source/.../CardHandWidget.cpp`, `CardHandWidget.h`, and the untracked `Tests/SiegeCardHandKeyLabelTest.cpp` (TASK-809/819 surfaces).

---

## 11. STATE LEDGER — MEASURED, NOT ASSERTED

**Packages saved by me this session — exactly two, by explicit single path, ⛔ never a save-all:**

```
[2026.09.03-02.57.42] LogFileHelpers: Saving Package: /Game/Input/Actions/IA_DiscardAll
[2026.09.03-03.01.54] LogFileHelpers: Saving Package: /Game/Input/IMC_Hero
```

- 🔒 **`L_Arena` NEVER-SAVE LAW HELD, MEASURED TWICE (entry and exit):** `Content/Maps/L_Arena.umap` sha256 = `9ccd54efeb0459df9ed15204fd7e5274797e5f6093f5504a5730c3c9d5ea0e58` — an **exact match to the `ROT-§2` ledger `9ccd54ef…0e58`**. **`Saving Package: /Game/Maps` count in the editor log: `0`.**
- 🔒 **`WBP_CardHand` was NOT saved** — `Saving Package: /Game/UI/WBP_CardHand` count: **`0`**. ⚠️ It reads dirty in memory from TASK-808's read-only measurement (which declined the save); I left that alone and did not touch the asset. ⛔ **Do not let a "save all" run.**
- ⛔ **The editor was NOT closed, restarted or bounced.** PID **30748** throughout — every payload asserted `os.getpid() == 30748` and would abort before touching an asset otherwise; every run printed `pid 30748 confirmed`. No PIE was started. No modal was encountered, so no click of any kind was made.
- **Node identity bound before any editor-shaped call:** exactly one remote-exec node, `project_root = C:/GitProjects/GitHub/GitClaudeUnrealTesting/GitClaudeUnrealTest/`. The driver refuses if more than one node answers.
- ⛔ **No `Config/` file written** — `bRemoteExecution` was **already `true`** on arrival; I changed no setting.
- ⛔ **Untouched by me:** every other `IA_*` · every other `IMC_Hero` row · `WBP_CardHand` · `WBP_HUD` · `DT_Cards` · `L_Arena` · any `.cpp`/`.h` · the controls-help registry.

---

## ⚠️ DEVIATIONS / FINDINGS (`SC-§15`)

**D1 — ⭐ THE HEADLINE: `MapKey` does not dirty the package and the first `save_asset` silently no-opped while returning `True`.** Full detail in §6. Caught only by the on-disk hash. **Recommend a fifth `KBD-§2a` condition: the append is not done until the on-disk bytes change.** ⛔ No `CONVENTIONS.md` edit made.

**D2 — `unreal.Key` construction and `Key.__eq__` both differ from the obvious guess.** §7. Recorded so the next agent does not lose time, and specifically so nobody treats a `False` struct-equality as a defect.

**D3 — `Package.is_dirty()` and `Object.mark_package_dirty()` are not exposed** on the Python bindings here (`AttributeError` on both). Use MCP `AssetTools.is_dirty` for the query and `imc.modify()` for the mark.

**D4 — `KBD-§2a` condition 3's struck-through literals are stale again**, now reading `26 → 27` in TASK-747's history while the live measurement this session was **27 → 28**. ⇒ I satisfied the **RELATIVE** invariant with `N` measured in-session, which is what the repaired law actually requires. **Flagging only so a reviewer reading the struck-through prose does not raise a false FAIL — no amendment is owed; the repaired law is correct.** This is the second consecutive task to have to say this.

**D5 — TASK-818's §5.2 punctuation rule is still unwritten in `KBD-§11`'s known-limitations list.** Not mine to board, and `H` (a letter) does not approach it — confirming independently that the hole remains unrecorded in law.

⛔ **AND THE STANDING ONE: ON-SCREEN CORRECTNESS IS JONATHAN'S PIXEL CHECK.** ⛔ **Nothing in this document claims anything looks or feels right — I rendered ZERO pixels and started no PIE.** Every claim above is a property readback, an on-disk byte scan, a file hash, an asset-registry query, or an editor log line. **Whether `H` actually discards the hand in play is TASK-819's code plus a playtest, not this task.**

---

## Reproduction artifacts (scratchpad, not repo)

- `ue_remote_driver.py` — remote-exec driver (banned-token scan · sole-node + project-root check · PID self-assertion)
- `t820_author.py` — **the `KBD-§2a` scratchpad authoring script, the one `map_key`**, with the internal `H`-free gate and all six proofs
- `t820_probe.py` / `t820_probe2.py` / `t820_probe3.py` — read-only API probes (§7); `probe3` is the `__eq__` control experiment
- `t820_save.py` — the forced-save repair for the §6 defect
