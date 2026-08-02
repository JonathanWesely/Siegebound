# TASK-399 — [FC-5] `IA_CmdFollow` + the `C` mapping in `IMC_Hero` — artist handoff

**Agent:** art-director · **Date:** 2026-08-02 · **Status on exit:** `ready-for-integration`
**Law implemented:** CONVENTIONS **"FOLLOW command + the DEFAULT-STANCE law + the MINER command rework (2026-08-02)"** §1 (the input-asset + conflict-check clause).
**Asset-only. No C++, no Git, no compile. `L_Arena` was never opened and never saved.**

---

## 1. Assets delivered — exact object paths

| Asset | `/Game/` object path | Disk | Action |
|---|---|---|---|
| `IA_CmdFollow` | `/Game/Input/Actions/IA_CmdFollow.IA_CmdFollow` | `Content/Input/Actions/IA_CmdFollow.uasset` | **CREATED** |
| `IMC_Hero` | `/Game/Input/IMC_Hero.IMC_Hero` | `Content/Input/IMC_Hero.uasset` | **EDITED** (one mapping appended) |

Both saved by **explicit path** (`save_assets` with a 2-element list — never save-all). Both read back `is_dirty == false` after the save. `Content/Maps/L_Arena.umap` on-disk mtime is unchanged at **Jul 29 03:53** — the diagnosis agents' in-memory dirt did **not** reach disk.

### ⚠️ Path readback vs. what the code composes — character-for-character

TASK-395 soft-references this asset null-safely, so a typo would be silent. Verified literally, not by eye:

- `SiegePlayerController.cpp:166` composes `TEXT("/Game/Input/Actions/IA_CmdFollow.IA_CmdFollow")`
- `AssetTools.load_asset` returns `refPath: "/Game/Input/Actions/IA_CmdFollow.IA_CmdFollow"`
- Both 45 chars, string-equality **IDENTICAL** (compared programmatically, not visually).

Reference graph closes the loop the other way: `get_referencers("/Game/Input/Actions/IA_CmdFollow")` → `["/Game/Input/IMC_Hero"]`. The asset exists, is named right, and is genuinely wired into the context.

---

## 2. `IA_CmdFollow` — settings, and the donor they came from

**Built by DUPLICATING `/Game/Input/Actions/IA_CmdAmbush`** (the newest shipped sibling, TASK-345) rather than authoring from scratch — a duplicate mirrors the donor by construction instead of by my re-typing it.

I first read all **eleven** UPROPERTYs off **all five** shipped command siblings. **They are unanimous** — `IA_CmdAttack` (T), `IA_CmdHold` (R), `IA_CmdDefend` (E), `IA_CmdAmbush` (F) and `IA_Rally` (Q) return byte-identical property sets, so there was no fleet-vs-spec disagreement to resolve on this one:

| Property | All 5 siblings | `IA_CmdFollow` (readback) |
|---|---|---|
| `valueType` | `Boolean` (Digital) | **`Boolean`** ✅ |
| `triggers` | `[]` (none) | **`[]`** ✅ |
| `modifiers` | `[]` (none) | **`[]`** ✅ |
| `bConsumeInput` | `true` | `true` ✅ |
| `accumulationBehavior` | `TakeHighestAbsoluteValue` | `TakeHighestAbsoluteValue` ✅ |
| `bTriggerWhenPaused` | `false` | `false` ✅ |
| `bConsumesActionAndAxisMappings` | `false` | `false` ✅ |
| `bReserveAllMappings` | `false` | `false` ✅ |
| `triggerEventsThatConsumeLegacyKeys` | `0` | `0` ✅ |
| `actionDescription` | `""` | `""` ✅ |
| `playerMappableKeySettings` | `None` | `None` ✅ |

Asset class readback: **`InputAction`** (matches `IA_CmdAmbush`).

**Note for the programmer, not a defect:** the siblings carry **no explicit trigger**. Enhanced Input's default for a Boolean action with an empty `triggers` array is a *Down* evaluation that still emits `ETriggerEvent::Started` on the press edge — which is exactly what `SiegePlayerController.cpp:363` binds. Adding an explicit `Pressed` trigger would have *deviated* from the fleet, so I did not. C will behave identically to T/E/R/F/Q.

---

## 3. ⚠️ `IMC_Hero` mapping enumeration — BEFORE and AFTER

**Mapping-context priority is unchanged.** `IMC_Hero` is applied by `AHeroCharacter` at priority **1** (`HeroCharacter.cpp:37`, `constexpr int32 HeroMappingContextPriority = 1`). I added a mapping *inside* the existing context; I did not touch how or when it is applied.

### BEFORE — 22 mappings

| # | Action | Key | Modifiers |
|---|---|---|---|
| 1 | `IA_Jump` | SpaceBar | — |
| 2 | `IA_Move` | W | SwizzleAxis_0 |
| 3 | `IA_Move` | S | SwizzleAxis_1, Negate_0 |
| 4 | `IA_Move` | A | Negate_1 |
| 5 | `IA_Move` | D | — |
| 6 | `IA_Look` | Mouse2D | Negate_2 |
| 7 | `IA_Sprint` | LeftShift | — |
| 8 | `IA_Attack` | LeftMouseButton | — |
| 9 | `IA_Card1` | One | — |
| 10 | `IA_CancelPlace` | RightMouseButton | — |
| 11 | `IA_CancelPlace` | Escape | — |
| 12 | `IA_Card2` | Two | — |
| 13 | `IA_Card3` | Three | — |
| 14 | `IA_Card4` | Four | — |
| 15 | `IA_Card5` | Five | — |
| 16 | `IA_Card6` | Six | — |
| 17 | `IA_UICursor` | LeftAlt | — |
| 18 | `IA_Rally` | **Q** | — |
| 19 | `IA_CmdAttack` | **T** | — |
| 20 | `IA_CmdHold` | **R** | — |
| 21 | `IA_CmdDefend` | **E** | — |
| 22 | `IA_CmdAmbush` | **F** | — |

### AFTER — 23 mappings

**Entries 1–22 are byte-identical to the table above** (verified by full readback + diff, not by assertion). One entry appended:

| # | Action | Key | Triggers | Modifiers | settingBehavior | playerMappableKeySettings |
|---|---|---|---|---|---|---|
| **23** | **`/Game/Input/Actions/IA_CmdFollow.IA_CmdFollow`** | **`C`** | `[]` | `[]` | `InheritSettingsFromAction` | `None` |

That row's shape is copied from rows 19–22 (the four `IA_Cmd*` siblings), which are themselves identical to each other.

### ✅ CONFIRMATION THAT `C` WAS PREVIOUSLY UNBOUND

**`C` appears nowhere in the BEFORE table.** I did not stop at `IMC_Hero` — I enumerated **every** `InputMappingContext` asset in the project (`find_assets` on `/Game` for `IMC_`, 6 hits) and checked each for a `C` binding:

| Mapping context | In play? | Binds `C`? |
|---|---|---|
| `/Game/Input/IMC_Hero` | ✅ yes (priority 1, `AHeroCharacter`) | **NO** |
| `/Game/Input/IMC_Default` | template leftover (priority 0) | **NO** (SpaceBar, WASD, arrows, gamepad only) |
| `/Game/Input/IMC_MouseLook` | ✅ yes | **NO** (Mouse2D only) |
| `/Game/Variant_Combat/Input/IMC_Combat` | template map only | **NO** (uses R for `IA_ToggleCameraSide`) |
| `/Game/Variant_Platforming/Input/IMC_Platforming` | template map only | **NO** |
| `/Game/Variant_SideScrolling/Input/IMC_SideScroller` | template map only | **NO** (uses F for `IA_Interact`) |

Legacy input was checked too: `Config/DefaultInput.ini` has **no** `C` action/axis mapping and no `Crouch` binding — consistent with the programmer's code-side grep (`EKeys::C` / `Crouch` → zero hits in `Source/GitClaudeUnrealTest/`).

**Conclusion: `C` was genuinely free. Nothing was stomped, and there is no double-binding to diagnose at the playtest gate.** No escalation was needed.

---

## 4. 🔍 ENGINE NOTE FOR WHOEVER TOUCHES AN `IMC_` NEXT — the empty-`mappings` trap

**UE 5.8 has moved the live mapping array.** `UInputMappingContext` now exposes **two** array-shaped properties:

- `mappings` — **legacy/deprecated, and it reads back EMPTY (`[]`) on every one of the six contexts in this project.**
- `defaultKeyMappings.mappings` — **this is where all 22 shipped mappings actually live, and it is what I wrote.**

An agent that queries `mappings`, sees `[]`, and concludes "`IMC_Hero` has no bindings" would (a) wrongly report C as free for the *right* reason by accident, and (b) if it *wrote* to `mappings`, produce an asset that looks correct in a property dump and **binds nothing at runtime**. Recording this so the next input task does not lose a loop to it.

## 5. 🔍 The instanced-subobject risk I checked, and why the result is trustworthy

The MCP `set_properties` API takes the **whole** array, so adding one mapping means rewriting all 23 — and six of those entries reference **instanced modifier subobjects owned by the `IMC_Hero` package** (`IMC_Hero:InputModifierSwizzleAxis_0`, `…_1`, `…Negate_0`, `…_1`, `…_2`). The tool's own docs note that instanced sub-object properties can be constructed from a class path, so a round-trip that *re-created* those modifiers with defaults would have silently broken **WASD movement and mouse-look Y-inversion** while the mapping table still looked perfect.

So I snapshotted every modifier's values **before** the write and re-read them **after**:

| Modifier | Used by | Before | After |
|---|---|---|---|
| `InputModifierSwizzleAxis_0` | `IA_Move` / W | `order: YXZ` | `order: YXZ` ✅ |
| `InputModifierSwizzleAxis_1` | `IA_Move` / S | `order: YXZ` | `order: YXZ` ✅ |
| `InputModifierNegate_0` | `IA_Move` / S | X✓ Y✓ Z✓ | X✓ Y✓ Z✓ ✅ |
| `InputModifierNegate_1` | `IA_Move` / A | X✓ Y✓ Z✓ | X✓ Y✓ Z✓ ✅ |
| `InputModifierNegate_2` | `IA_Look` / Mouse2D | X✗ **Y✓** Z✗ | X✗ **Y✓** Z✗ ✅ |

All five are the **same instances** (identical `refPath`s in the after-readback) with **identical values**. `Negate_2` is the load-bearing one — a default-constructed Negate would have flipped X as well and inverted horizontal mouse-look. It is intact.

`IMC_Hero` was also confirmed **clean on disk before I started** (`is_dirty == false`), so the pre-write snapshot is a true baseline and a bad write would have been revertible by reload. It was not needed.

---

## 6. What this unblocks / notes for integration

- **No integration work is required of build-master beyond committing the two files.** These are pure data assets; nothing needs attaching, and I wired nothing into a Blueprint (not my lane).
- **The C key is now live data-side, but still inert game-side until the code lane compiles.** `CmdFollowAction` is resolved in `SetupInputComponent` (`SiegePlayerController.cpp:363`) — with the asset now present, `ResolveInputAction` will succeed and bind `ETriggerEvent::Started` → `OnCmdFollowPressed()` on the **next compile of TASK-395 + TASK-396 together**. Until then C does nothing, which is the designed pre-compile state, not a defect.
- **Files for build-master to commit by explicit path:**
  - `Content/Input/Actions/IA_CmdFollow.uasset` (new)
  - `Content/Input/IMC_Hero.uasset` (modified)
  - `.claude/pipeline/handoffs/TASK-399-artist.md` (this file)
  - the `TASK-399` status line in `.claude/pipeline/TASKBOARD.md`
- **Do NOT let a `git add -A` sweep in `L_Arena`.** It is dirty in memory from the diagnosis agents' temp actors. Its on-disk copy is untouched (Jul 29 03:53) and must stay that way; if the editor is closed later, **decline the save prompt for `L_Arena`**.
- **No `.uasset` other than the two above was created, edited, or saved by this task.**

## Acceptance self-check

| Criterion | Status |
|---|---|
| `IA_CmdFollow` exists at the pinned path, Digital/bool | ✅ `valueType: Boolean` |
| Settings mirror a shipped sibling rather than a guess | ✅ duplicated from `IA_CmdAmbush`; all 11 properties verified against all 5 siblings |
| Mapped to `C` in the existing `IMC_Hero` | ✅ entry 23 |
| Additive — not one existing mapping disturbed | ✅ entries 1–22 diffed identical, all 5 modifier subobjects value-verified |
| `C` confirmed free BEFORE mapping, across all contexts | ✅ 6 contexts + `DefaultInput.ini` enumerated; zero hits |
| Path matches what the code composes, character-for-character | ✅ programmatic string equality, 45/45 chars |
| Mapping-context priority unchanged | ✅ still 1, applied by `AHeroCharacter` |
| Only the two assets saved; never save-all | ✅ explicit 2-path `save_assets` |
| `L_Arena` never opened, never saved | ✅ on-disk mtime unchanged (Jul 29 03:53) |
| No C++, no Git, no compile | ✅ |
