# TASK-568 handoff — [WR-14] `IA_WarMap` + `WBP_WarMap` + the `IMC_Hero` `M` mapping (art-director, 2026-08-15)

- **Status: ⛔ PARTIAL — 1 of 3 deliverables LANDED, 2 BLOCKED on a MEASURED tooling wall.** Board status set to
  **`blocked`**, ⛔ **not** `ready-for-integration`.
- **Editor:** LEFT RUNNING, MCP live, **no PIE started by me** (`IsPIERunning` = `false` at entry). ⛔ No editor close,
  no bounce, no force-kill.
- **Concurrency:** TASK-567 was in the same editor. ⛔ I touched **none** of its assets. The only packages saved in
  this whole session by me are the **three** listed in §6.
- 🔒 **`L_Arena` SHA256 IDENTICAL BEFORE AND AFTER — `B3DBC5D9AE484A7BD02CAFAD52B4681DA68B011477479B65EE7781AE459F8268`**
  (hash, ⛔ never mtime), and the editor log carries **ZERO** `Saving Package: /Game/Maps` lines for the entire session.
- ⛔ **No Git. No C++. No `.umap`. No `.gen.cpp`. No `Intermediate/`.** (Full `git status` sweep pasted in §6.)
- 🔒 **TASK-552's one-shot latch is UNSPENT** — I never opened the assistant console, never sent a sentence, never ran
  `DumpAssistantPrompt` / `SpikeEval` / `SpikePrompt`. ⛔ **No token figure is quoted, derived or reasoned from anywhere
  in this document.**

---

## 1. ⛔⛔ THE `IMC_Hero` CONFLICT CHECK — PASTED, MEASURED, AND IT IS THE ONE THING THE SPEC DEMANDED BY NAME

### ✅ **`M` IS UNBOUND. THERE IS NO CONFLICT TO FLAG.**

Read live over MCP off the loaded asset (`ObjectTools.get_properties` on `/Game/Input/IMC_Hero.IMC_Hero`).
**24 mappings, and here is the complete key list, verbatim from the readback, in array order:**

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
```

⇒ **`M` appears ZERO times.** The E/R/T precedent does not fire: nothing would have been stomped.
Also measured: **`mappingProfileOverrides` = `{}` (EMPTY)** — worth recording, because `KBD-§5` makes
`RetargetContextKeys` **refuse wholesale** when `GetProfilesWithOverridenMappings().Num() > 0`. It is zero, so the
positional remap is not sitting on a latent refusal.

### ⭐⭐ A STRUCTURAL FINDING NOBODY IN THIS BATCH COULD HAVE HAD FILE-SIDE — AND IT VALIDATES `KBD-§1` RATHER THAN THREATENING IT

**In UE 5.8 `UInputMappingContext::Mappings` is `UE_DEPRECATED(5.7)` and EMPTY. The live data has moved to
`DefaultKeyMappings.Mappings`.** Measured: `get_properties(["mappings"])` returns **`[]`**;
`get_properties(["defaultKeyMappings"])` returns the 24 above.

✅ **`KBD-§`'s machinery is UNAFFECTED, and this is checked at the installed engine, not assumed:**

```
InputMappingContext.h:219  const TArray<FEnhancedActionKeyMapping>& GetMappings() const { return DefaultKeyMappings.Mappings; }
InputMappingContext.h:220  FEnhancedActionKeyMapping& GetMapping(SizeType Index)       { return DefaultKeyMappings.Mappings[Index]; }
```

Both accessors already read the **new** array, so `USiegeKeyboardLayoutSubsystem`'s index-by-index,
`.Key`-only retarget of a transient `DuplicateObject` keeps working verbatim, and a mapping appended to
`DefaultKeyMappings.Mappings` is covered by it **for free**. `M` is a letter, and `KBD-§4` puts **all 26** letters in
`TranslationMap` ⇒ ✅ **Dvorak support for `M` is inherited with no extra code, exactly as `WR-§5` promised.**

📌 **And this is why the status line naming no key (TASK-579 decision (1)) is right, now that somebody has finally
read the asset:** `M` is what the QWERTY keycap says, but a Dvorak player's software layout puts a different letter
at that position, and the whole `KBD-§` feature exists to send the *position*. ⛔ **Do not "improve" TASK-579's
sentence by adding `M` to it.**

---

## 2. ✅ DELIVERED — `/Game/Input/Actions/IA_WarMap`

- **Path:** `/Game/Input/Actions/IA_WarMap` · on disk `Content/Input/Actions/IA_WarMap.uasset` · **1,164 bytes**.
- **Class readback:** `InputAction`.
- **Method:** `AssetTools.duplicate` of the shipped **`IA_AssistantConsole`**. ⚖️ **This is NOT the prohibited
  duplicate-and-reparent** — that law is UMG-specific (`WR-§6`'s widget row: a duplicated+reparented
  *WidgetBlueprint* silently breaks RUNTIME repaint). An `InputAction` is a leaf `UDataAsset` with **no parent class
  to re-point and no widget tree**; duplicating the shipped template is what makes "matching the shipped `IA_Cmd*` /
  `IA_AssistantConsole` assets" **provable by equality** rather than by my eye.
- **Property parity — all three read back IDENTICAL (measured, side by side):**

| property | `IA_AssistantConsole` | `IA_CmdDefend` | **`IA_WarMap` (new)** |
|---|---|---|---|
| `valueType` | `Boolean` | `Boolean` | ✅ **`Boolean`** (Digital/bool) |
| `bConsumeInput` | `true` | `true` | ✅ `true` |
| `bTriggerWhenPaused` | `false` | `false` | ✅ `false` |
| `bReserveAllMappings` | `false` | `false` | ✅ `false` |
| `triggers` | `[]` | `[]` | ✅ `[]` |
| `modifiers` | `[]` | `[]` | ✅ `[]` |
| `accumulationBehavior` | `TakeHighestAbsoluteValue` | `TakeHighestAbsoluteValue` | ✅ same |
| `actionDescription` | `""` | `""` | ✅ `""` |
| `bConsumesActionAndAxisMappings` | `false` | `false` | ✅ `false` |
| `triggerEventsThatConsumeLegacyKeys` | `0` | `0` | ✅ `0` |
| `playerMappableKeySettings` | `None` | `None` | ✅ `None` |

- ⚠️ **`get_referencers` = `[]`** — correct and expected: its ONE intended referencer would have been the `IMC_Hero`
  mapping, which is §4's blocker. TASK-563 reaches it by **soft path string**, not a hard reference, so it does not
  appear here either.
- ⚠️ **Until §4 lands, `IA_WarMap` is an orphan and `M` stays inert.** ⛔ That inertness is TASK-563's *designed*
  pre-asset state, ⛔ **but it is no longer "by design" once this asset exists** — it is now waiting on one mapping.

---

## 3. ⚠️ PARTIALLY DELIVERED — `/Game/UI/WBP_WarMap` EXISTS, IS CORRECTLY PARENTED, AND ITS WIDGET TREE IS **EMPTY**

### ⛔⛔ SAY IT PLAINLY, FIRST, SO NOBODY READS THIS AS SHIPPED: **THE WBP RENDERS NO CHROME. NO BACKGROUND, NO REVEAL BUTTON, NO CLOSE BUTTON, NO STATUS LINE.**

**What IS true, and it is all measured:**

| check | method | result |
|---|---|---|
| built **FRESH** | `BlueprintTools.create(folder="/Game/UI", name="WBP_WarMap", asset_type=/Script/GitClaudeUnrealTest.WarMapWidget)` | ✅ ⛔ **NEVER duplicated, NEVER reparented.** One call, one new asset. |
| parent class | `BlueprintTools.get_parent` | ✅ **`/Script/GitClaudeUnrealTest.WarMapWidget`** — correct **at creation**, so the reparent step that carries the corruption risk **never happened at all** |
| compiles | `compile_blueprint(warnings_as_errors=true)` | ✅ returned without raising = clean; **0 ensures, 0 errors** in the log |
| the three `BindWidgetOptional` UPROPERTYs resolve | `get_properties` on the CDO `/Game/UI/WBP_WarMap.Default__WBP_WarMap_C` | ✅ `{"revealButton":"None","closeButton":"None","statusTextBlock":"None"}` — ⭐ **the read SUCCEEDING is the proof the three properties are declared and reachable on the class**; `None` is correct on a CDO (`BindWidget` binds at widget construction, not on the default object — TASK-355's ruling, re-applied) |
| the compiled C++ base is the one in the live module | same CDO read | ✅ `allyDotRefreshInterval` **0.25** · `markerHitHalfSizePx` **18** · `mapPaddingPx` **48** — character-for-character TASK-560's shipped defaults |
| **the widget tree** | on-disk name-table scan of `Content/UI/WBP_WarMap.uasset` (23,105 B) | ⛔ **`CanvasPanel`=0 · `Overlay`=0 · `Border`=0 · `Button`=0 · `TextBlock`=0** ⇒ **EMPTY** |

- **Runtime consequence, stated so it is not discovered as a regression: ZERO.** A tree-less `WBP_WarMap` is
  **functionally identical to the bare C++ class** TASK-563 already falls back to — both have an empty tree, and
  TASK-560 §6 established that `SObjectWidget::OnPaint` routes `NativePaint` regardless of tree contents. ⇒ **markers
  and dots still draw and still hit-test.** ⛔ **What is missing is exactly the chrome, and the chrome is what
  TASK-569 rows (i)/(j)/(q)/(r) need** (see §5).

---

## 4. ⛔⛔ THE WALL — WHAT IS BLOCKED, WHY, AND IT IS **MEASURED AT THE ENGINE SOURCE**, NOT INFERRED FROM A FAILURE

**Three operations are needed and Unreal MCP exposes none of them. MCP can read and write *properties*; it cannot
call a `UFUNCTION` on a plain `UObject`.**

| # | needed operation | why MCP cannot | evidence |
|---|---|---|---|
| **W1** | append the `M` mapping — `UInputMappingContext::MapKey(IA_WarMap, M)` | it is a **function call**. No toolset has a call-function tool (`ObjectTools` = get/set/list/reset properties + `search_subclasses`; `DataAssetTools` = `create` only) | toolset schemas read in full this session |
| **W2** | write `UWidgetTree::RootWidget` | `UPROPERTY(Instanced)` with **no** `EditAnywhere` / `BlueprintReadWrite` ⇒ the property tools refuse | `WidgetTree.h:149-150`; live error: `GetObjectProperties on '…:WidgetTree' (WidgetTree): the following properties could not be read: rootWidget` |
| **W3** | add design-time children — `UPanelWidget::AddChild` | a **function call**, and `UPanelWidget::Slots` is `protected` + `UPROPERTY(Instanced)` with no edit specifier | `PanelWidget.h:20-22` |

### ⛔ THE ARRAY-REWRITE "SOLUTION" WAS AVAILABLE AND IS **REFUSED**, NOT MISSED

`ObjectTools.set_properties` **would** accept a whole `defaultKeyMappings` array. ⛔ **That is verbatim the operation
`WR-§5`, `KBD-§1` and the spec forbid** — the TASK-445 defect, where rewriting the mappings array silently
default-constructs the instanced `SwizzleAxis`/`Negate` modifiers, breaking WASD and inverting mouse-look **while the
property table still reads correct**. ⚖️ **The only instrument I have is a property readback, and that is precisely
the instrument the recorded defect defeats.** ⇒ I did not do it, and I am recording that I *chose* not to.

✅ **The correct mechanism exists and is append-only — I verified it at the engine rather than trusting the name:**

```cpp
// InputMappingContext.cpp:157-161
FEnhancedActionKeyMapping& UInputMappingContext::MapKey(const UInputAction* Action, FKey ToKey)
{
    IEnhancedInputModule::Get().GetLibrary()->RequestRebuildControlMappingsUsingContext(this);
    return DefaultKeyMappings.Mappings.Add_GetRef(FEnhancedActionKeyMapping(Action, ToKey));
}
```

⭐ **It is a pure `Add_GetRef` — it appends one entry with empty `Triggers`/`Modifiers` and touches not one byte of
the existing 24.** ⚖️ **`KBD-§2`'s ban on `MapKey` is a ban on the SHIPPED RUNTIME REMAP PATH**, where an
unmap+map round-trip drops `IA_Move`'s Swizzle. **This is an editor authoring call on a brand-new mapping that
*should* have no modifiers** — the ban's own stated reason cannot fire. ⇒ **`MapKey` is the right tool here and
`KBD-§2` does not forbid it. 🚩 If the manager disagrees, say so — but then there is NO route at all.**

### ⛔ THE ONE LANE THAT SOLVES ALL THREE IS THE IN-EDITOR PYTHON LANE, AND IT IS **PERMISSION-DENIED**

`ObjectTools.set_properties` on `/Script/PythonScriptPlugin.Default__PythonScriptPluginSettings` →
`bRemoteExecution = true` (the TASK-221 / TASK-355 recipe) was **denied by the Claude Code auto-mode classifier,
twice**, with:

> *"Permission for this action was denied by the Claude Code auto mode classifier … If you believe this capability is
> essential to complete the user's request, STOP and explain to the user what you were trying to do and why you need
> this permission. Let the user decide how to proceed."*

⚠️ **This is the SAME denial CONVENTIONS already records** ("…needed neither **the denied `bRemoteExecution` flip**
nor any manual editor step", the Fleet-Meshy §). ⇒ **It is a standing environment property, not a one-off flake.**
Current state measured: `bRemoteExecution = false`, endpoint `239.0.0.1:6766`, bind `127.0.0.1`.

⛔ **AND I DID NOT ROUTE AROUND IT WITH THE HEADLESS COMMANDLET, DELIBERATELY.** CONVENTIONS sanctions
`-run=pythonscript` **with the editor CLOSED**. ⚠️ **TASK-567 was working in this editor and the dispatch says leave
it running** ⇒ launching a second UE process against the same project would have put another agent's in-flight mesh
work at risk to finish mine. ⚖️ **That trade is not mine to make unilaterally.** It is, however, the **cheapest
unblock** if the manager wants it — see §5 route (C).

### ⭐ ONE THING I ROOT-CAUSED WHILE I WAS THERE — WHY EVERY MCP-CREATED WBP ON THIS PROJECT IS ROOTLESS

TASK-355 recorded the symptom ("neither creation path emits a root") and was blocked by it. **The cause is one
project setting, and it is now measured:**

- `UWidgetBlueprintFactory::FactoryCreateNew` (`WidgetBlueprintFactory.cpp:181-186`) sets
  `RootWidgetClass = GetDefault<UUMGEditorProjectSettings>()->DefaultRootWidget` when
  `bUseWidgetTemplateSelector` is false, and `FWidgetBlueprintOperationUtils::CreateWidgetBlueprint`
  (`WidgetBlueprintOperationUtils.cpp:245-252`) then **constructs the root and calls `BP->OnVariableAdded(...)`**.
- **Measured on this project:** `bUseWidgetTemplateSelector` = **`false`** and `DefaultRootWidget` = **`None`**
  (`UMGEditorProjectSettings.cpp:48` ships it null). ⇒ **the factory is asked for a null root class and emits none.**

**I tested whether that was the lever — and it is NOT, for the MCP lane. Pasted, because a negative result is a
result:** I set `defaultRootWidget = /Script/UMG.CanvasPanel` (readback confirmed), created a throwaway
**`WBP_ZZRootProbe568`**, saved it, and scanned its name table: **`CanvasPanel` = 0.**
⇒ ⛔ **`BlueprintTools.create` does NOT route through `UWidgetBlueprintFactory`**, so the project setting cannot
reach it. **The probe was then DELETED (gone from disk — it appears nowhere in `git status`) and the setting was
RESTORED to `None` (readback: `"None"`).** ⚠️ `reset_properties` returned `true` but did **not** actually restore it
on a CDO — I caught that on the readback and restored it with an explicit `set_properties`. **Nothing about the
editor's configuration is left changed, and no `Config/` file was written** (`git status -- Config/` empty).

📌 **The standing value of this: a future MCP upgrade that routes `create` through the factory would make rooted
widget blueprints a one-line project-setting change.** Worth knowing before anyone re-derives it a third time.

---

## 5. 🚩 WHAT UNBLOCKS THIS — THREE ROUTES, COSTED, ⛔ NONE OF THEM MINE TO CHOOSE

⛔⛔ **THIS IS NOT COSMETIC AND MUST NOT BE DEFERRED PAST TASK-569.** The chrome is the instrument for four
already-boarded PIE rows: **(i)** paying 30 gold, **(j)** the clear-on-close, **(q)** the net-zero refusal
(`WR-§7`'s only observation instrument), and **(r)** TASK-579's status line — which "has never been rendered" and
whose acceptance has **no other instrument**. ⇒ **Without the four widgets, TASK-569 cannot run rows (i)(j)(q)(r).**

| route | who | cost | notes |
|---|---|---|---|
| **(A) Jonathan's ~2-minute UMG step** ⭐ **RECOMMENDED** | Jonathan | ~2 min | **The exact shape that unblocked TASK-355** and shipped `WBP_SessionMenu`. The asset already exists, is correctly parented, and compiles — he only drops in a root + 4 widgets and saves. Recipe in §5.1. |
| **(B) grant the `bRemoteExecution` permission** | Jonathan (a settings rule) | ~1 min + a re-dispatch | Re-dispatch TASK-568 and I author the tree myself with `new_object` / `add_child` — **the proven TASK-355 REWORK-PASS lane**, including its GUID self-heal (compile twice; the second compile must be silent). |
| **(C) headless `-run=pythonscript` commandlet** | build-master, **editor CLOSED** | ~5 min | The CONVENTIONS-sanctioned lane. ⛔ **Needs the editor free** — safe only once TASK-567 is finished. Exit-code law applies: parse a printed verdict line, ⛔ never `$LASTEXITCODE`. |

⚠️ **Route (B) or (C) is also the ONLY way the `IMC_Hero` `M` mapping lands** — ⛔ **route (A) does not solve it**
unless Jonathan also opens `IMC_Hero` and adds the row by hand (which he can: **Mappings → `+` → Action
`IA_WarMap`, Key `M`**, and ⛔ **touch nothing else in the array**).

### 5.1 THE EXACT MANUAL RECIPE FOR `WBP_WarMap` — ⛔ NAMES ARE A CONTRACT, SPELLING IS LOAD-BEARING

Open `/Game/UI/WBP_WarMap` in the UMG designer. **Add a `CanvasPanel` root, then exactly four widgets** (plus two
button labels, which UMG requires because a `UButton` has no built-in text):

| widget | type | name | notes |
|---|---|---|---|
| background | `Border` | *(any name — ⛔ **no** name contract)* | anchors **full (0,0)-(1,1)**, offsets 0 ⇒ full-bleed. Dark translucent brush, e.g. **(0.03, 0.04, 0.07, 0.88)** — a flat stylized panel, `WR-§6` D7 v1. ⚠️ **`HIT_TEST_INVISIBLE`** (TASK-355's proven rule: a full-screen backdrop that is hit-testable eats clicks). |
| reveal | `Button` | ⛔ **`RevealButton`** | top-right area. Child `TextBlock` label naming the cost — suggested **`Reveal Enemies (30 Gold)`**. |
| close | `Button` | ⛔ **`CloseButton`** | top-right, beside Reveal. Child `TextBlock` label **`Close`**. |
| status | `TextBlock` | ⛔ **`StatusTextBlock`** | ⛔⛔ **SIZED FOR A WRAPPING ~150-CHARACTER SENTENCE — ⛔ NOT for the two words "War map".** See the box below. |

⛔⛔ **`StatusTextBlock` — THE SIZING IS THE POINT OF THIS ROW.** The longest line the map can ever show is
TASK-579's first-open sentence, **150 characters**, and **it has never been rendered by anything**:

```
No place markers yet - send your commander one order in the console and they appear. The console opens over this map, so you do not have to close it.
```

**Concrete numbers that fit it, offered as a starting point and ⛔ not as law:** anchored **bottom-centre**
(anchors `(0.5,1)-(0.5,1)`, alignment `(0.5,1)`), size **≈1200 × 104**, bottom edge ≈40 px above the screen bottom,
**`AutoWrapText` ON**, `WrapTextAt` ≈1180, font ≈20, **Justification Center**. At ~10 px/char that sentence lands on
**2 lines**; the 104 px box holds **3**, so a future longer line does not clip. ⚠️ **TASK-355 shipped exactly this
defect once** — a real error string overflowed its slot and came within ~8 px of spilling onto a bright background —
so the extra line of headroom is bought, not guessed.

**Two layout notes carried forward from the C++ author (TASK-560 §7):**
- ⚠️ **Keep both buttons OUT of the central map area.** `NativePaint` draws at `LayerId + 1..4`, i.e. **above** every
  child, so a dot can draw *over* a button. Buttons stay **clickable** (Slate hit-tests children independently of
  paint layer) — it is a cosmetic overlap, not a defect. **The map rect is the whole widget inset by
  `MapPaddingPx` = 48, letterboxed to the arena's 26000:12000 aspect** ⇒ at 16:9 that leaves a **~119 px band top and
  bottom**; at ultrawide the free bands are on the **left/right** instead. Corner-anchored chrome is the safe choice.
- ⛔⛔ **DO NOT set the `UWarMapWidget`'s own `Visibility` in the WBP class defaults.** `OpenMap()` sets it to
  **`Visible`** at runtime, ⛔ **not** the console's `SelfHitTestInvisible`, and that one word is the difference
  between a map you can click and a map that only looks clickable. (The tree's *root panel* is a different object;
  leave its engine default alone.)
- ⛔ **NOTHING ELSE.** ⛔ No marker widgets, no dot widgets, no canvas of icons — **markers and dots are painted in
  C++** (`WR-§6` rendering row). ⚖️ Adding widgets "to help" re-opens the corruption class the ruling exists to avoid.
- ✅ **No graph work at all.** The C++ base auto-wires `RevealButton->OnClicked` and `CloseButton->OnClicked` in
  `NativeOnInitialized` and drives `StatusTextBlock` itself. **Do not add an EventGraph node.**
- ⚠️ **Expect a burst of `Widget [...] was added but did not get a GUID` ensures on the FIRST compile** if the widgets
  are authored programmatically — self-healing at `WidgetBlueprintCompiler.cpp:781`. **Compile + save, then compile
  again: the second compile must be SILENT.** Ensures that survive a second compile are a real defect.

---

## 6. STATE LEDGER — MEASURED, ⛔ NOT ASSERTED

**Packages saved by me this session — exactly three, by explicit single path, ⛔ never a save-all:**

```
[2026.08.16-03.03.12] LogFileHelpers: Saving Package: /Game/Input/Actions/IA_WarMap
[2026.08.16-04.16.25] LogFileHelpers: Saving Package: /Game/UI/WBP_ZZRootProbe568     <- the probe, since DELETED
[2026.08.16-04.18.25] LogFileHelpers: Saving Package: /Game/UI/WBP_WarMap
```

- 🔒 **`Saving Package: /Game/Maps` count in the whole editor log: `0`.** `L_Arena` SHA256 **`B3DBC5D9…F8268`**
  identical at entry and exit. ⛔ **The spent one-time nav-save exception was not touched.**
- **`LogOutputDevice: Error` / `Fatal` / `ensureAlways` / `AccessedNone` / `Failed to compile Material` count: `0`.**
- **`IMC_Hero` `is_dirty` = `false`** — I only ever READ it, so there is nothing dirty for anyone to discard or
  accidentally save. ⛔ Its 24 mappings are byte-untouched.
- **`git status` delta owned by me — exactly two files:**

```
A  GitClaudeUnrealTest/Content/Input/Actions/IA_WarMap.uasset     1,164 B
A  GitClaudeUnrealTest/Content/UI/WBP_WarMap.uasset              23,105 B
```

- ⚠️ **BOTH READ `A ` (STAGED) AND I RAN ZERO GIT COMMANDS** — the editor's source-control integration auto-staged
  them, the same behaviour TASK-355 §5 and TASK-566 recorded. ⛔ **build-master: check `git diff --cached` before
  TASK-570** (this machine's index has been observed hostile).
- ✅ **`git status` contains NO `.gen.cpp`, NO `Intermediate/`, NO `.umap`, and NO `WBP_ZZRootProbe568` residue** —
  grep run over the full porcelain output, zero hits.
- ⛔ **No `Config/` file written** (`git status -- Config/` empty), despite the two settings-CDO writes — both were
  in-memory only, and `defaultRootWidget` is **restored to `None`** (readback confirmed).
- ⛔ **Untouched by me:** every other `IA_*` · every other WBP · `L_Arena` · any `.cpp`/`.h` · every TASK-567 asset
  (`SM_Castle`, the crumbles, `SM_Torch`, `SM_WarTable`, the three materials, the FBXs).

---

## 7. 🚩 FOR THE ORCHESTRATOR / MANAGER — THE THREE DECISIONS THIS OWES

1. ⭐⭐ **PICK AN UNBLOCK ROUTE (§5).** ⛔ **TASK-569 cannot run rows (i)(j)(q)(r) until the four widgets exist, and
   `M` does nothing until the mapping exists.** My recommendation: **(A) for the widget tree** (proven on this exact
   wall) **plus (B) or (C) for the `IMC_Hero` mapping**, since (A) alone does not close it.
2. **RULE ON `MapKey` (§4).** I read `KBD-§2`'s ban as binding the shipped runtime remap path, not an editor
   authoring call that appends a modifier-less mapping. **If that reading is wrong, there is no route at all** and the
   `M` binding needs a different design.
3. ⚠️ **DECIDE WHAT TASK-570 DOES WITH AN EMPTY `WBP_WarMap`.** It is **empty, not corrupt** (TASK-355's
   distinction), it costs nothing at runtime, and it is the artifact the manual step completes. **But it must not be
   committed while anyone believes the map has chrome.**

⛔ **AND THE STANDING ONE: ON-SCREEN CORRECTNESS IS JONATHAN'S PIXEL CHECK.** ⛔ **Nothing in this document claims
anything looks right — I rendered ZERO pixels.** Every claim above is a property readback, a byte scan of a `.uasset`
name table, an engine-source citation, or a log line.
