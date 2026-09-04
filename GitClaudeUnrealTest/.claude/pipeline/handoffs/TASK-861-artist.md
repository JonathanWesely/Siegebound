# TASK-861 — the cast segment on `WBP_CombatantHealthBar` — ART HANDOFF

**Agent:** art-director · **Date:** 2026-09-02 · **Status:** ⛔ **BLOCKED — design ruled + baseline captured, ZERO asset writes made**
**Law:** `WITCH-§9` (`§9.1`–`§9.6`) · `CONVENTIONS` §5 *"The boost bar — `WBP_CombatantHealthBar` is EXTENDED, never duplicated"* · `W4-R5` · `SC-§40` cl. 9
**Editor/MCP:** UP, verified by real round-trips (not by a listening port). PIE `false` before and throughout.
**Asset touched:** ⛔ **NONE. I made zero writes to any asset.** `L_Arena` never opened, never saved. No Git, no C++.

---

## 0. THE HEADLINE — I STOPPED RATHER THAN DUPLICATING

The task's own item (5) and my dispatch both say: *if extension is impossible, STOP and say so rather than duplicating.*
**I am invoking that clause.** I did **not** duplicate, did **not** reparent, did **not** rebuild fresh, and did **not**
author a single node. Two independent blockers are measured below; **neither is a judgement call.**

⭐ **The creative deliverable — the differentiator ruling (spec item 3) — IS DONE and is in §4.** It was the one part of
this task that did not need the editor, and it is the part that decides whether the feature works. §5 carries the exact
pixel arithmetic, and §6 is a copy-pasteable recipe for the human step.

---

## 1. ⛔ BLOCKER 1 — MCP CANNOT INSTANTIATE A WIDGET INTO A `WidgetTree`. RE-MEASURED TODAY, NOT INHERITED.

`SC-§40` cl. 9 says re-measure rather than trusting a recorded count. TASKBOARD.md:2083 records this limit from
2026-08-02. **I re-measured it from scratch today against the live editor rather than citing it.** It still holds:

| probe | result |
|---|---|
| `list_toolsets` | **18 toolsets. No widget/UMG toolset exists.** |
| `ObjectTools` full tool list | `reset_properties`, `set_properties`, `get_properties`, `list_properties`, `get_class`, `search_subclasses` — **6 tools, no instantiation verb of any kind** |
| `AssetTools` full tool list | 21 tools (create_folder, duplicate, move, delete, save…) — **nothing that creates a widget inside a tree** |
| `BlueprintTools` full tool list | 53 tools — graphs, nodes, pins, variables, events. **Zero widget-tree verbs** |
| `ProgrammaticToolset.get_execution_environment` | sandbox is **`execute_tool` over registered tools only**; importable modules = `{datetime, copy, math, re, json, time}`. ⛔ **No `unreal` module** — it is explicitly *"tool orchestration, not general Python execution"* |
| `ObjectTools.list_properties` on `…WBP_CombatantHealthBar:WidgetTree` | **`""` — the empty string.** The WidgetTree exposes **zero** editable properties |
| `ObjectTools.list_properties` on `BarStack` (VerticalBox) | 22 properties returned, and **`Slots` is NOT among them** ⇒ children can be neither added **nor even enumerated** |
| `get_class` on `…WidgetTree.CastBarRoot` / `.CastBarFill` | **`is not valid Object for property 'instance'`** ⇒ neither widget exists |

⇒ **The `CastBarRoot` / `CastBarFill` instantiation is an irreducible human step**, exactly as `TASK-367` was for the
boost bar — and the board already budgets that shape ("*THE TWO HUMAN STEPS ARE JONATHAN'S AND ARE IRREDUCIBLE — they
are budgeted as REAL TASKS, not footnotes*", TASKBOARD.md:2083). §6 is the recipe.

---

## 2. ⛔ BLOCKER 2 — `OnCastProgressChanged` DOES NOT EXIST YET, AND BINDING IT NOW WOULD RE-CREATE THE FIVE-TIMES DEFECT

`BlueprintTools.list_events` on `WBP_CombatantHealthBar` returns **exactly three** project BIEs:

```
SetTeamColor   (bIsImplemented true)
SetDamageBoost (bIsImplemented true)
OnHPChanged    (bIsImplemented true)
```

**`OnCastProgressChanged` is absent.** Corroborated one level further down the chain:

```
grep -n "GetCastProgressPercent|IsCastInProgress" Source/GitClaudeUnrealTest/Siegebound/HealthBarProvider.h
  → no output
```

⇒ **`TASK-830` item (8) has not landed either**, so `TASK-860` is itself still blocked on its own premise gate (its
spec item (0a) tells it to STOP if those virtuals are absent — they are).

⚠️ **Why I did not author the event anyway.** `CONVENTIONS` §5 records, in bold: *"`SetDamageBoost` landing as a
`K2Node_CustomEvent` never fires from C++, silently — this exact defect hid the health-bar bug five times."*
`add_event` can only produce a true `K2Node_Event` override **if the BIE already exists on the parent C++ class.**
Authoring `OnCastProgressChanged` today would necessarily produce a `K2Node_CustomEvent` that is DSL-indistinguishable
from the real thing, reports `bIsImplemented = true`, and **never fires.** That is the single most expensive mistake
available in this file, and it is available *right now*.

### ⚖️ 2.1 THE BOARD ROW IS WRONG ON ONE POINT, AND IT IS WORTH CORRECTING

TASK-861 is boarded `blocked-by: none` / `parallel-safe: yes`, on the reasoning that *"the C++↔widget contract is PINNED
IN LAW (`WITCH-§9.3`), so you do NOT wait for `TASK-860`."`

**That reasoning is correct about the contract and wrong about the mechanism.** I do not need to *negotiate* names with
`TASK-860` — the law pins them, and I have honoured them character-for-character. But I cannot **bind** a
`BlueprintImplementableEvent` that the parent class does not yet declare. Naming independence ≠ build independence.

⇒ **TASK-861 is genuinely three phases, and only phase A was runnable today:**

| phase | owner | gate |
|---|---|---|
| **A — design ruling + baseline + recipe** | art-director | ✅ **DONE today** (§4, §5, §6, §7) |
| **B — instantiate the two widgets in the designer** | 🧑 **Jonathan** (~90 s) | needs nothing; can happen any time |
| **C — brushes, tints, visibility, and the event graph** | art-director | needs **B done** *and* **`TASK-860` compiled** |

---

## 3. ✅ WHAT I MEASURED — THE COMPLETE "BEFORE" BASELINE (spec item 4 — *capture before and after, do not assert*)

Spec item (4) requires the pixel-identical guarantee to be **proven by comparison, not asserted.** The before-half is
captured now, in full, so phase C has something real to diff against. This is the artifact, recorded verbatim.

**Structure (parentage proven by each widget's own `slot` refPath — a hard structural read):**

```
BarStack      /Script/UMG.VerticalBox    slot = None (root)
 ├─ BoostOutline  /Script/UMG.Border       slot = …WidgetTree.BarStack.VerticalBoxSlot_4
 │    └─ BoostBar /Script/UMG.ProgressBar  slot = …WidgetTree.BoostOutline.BorderSlot_0
 └─ Bar           /Script/UMG.ProgressBar  slot = …WidgetTree.BarStack.VerticalBoxSlot_5
```

Parent class: `/Script/GitClaudeUnrealTest.CombatantHealthBarWidget` ✓ · Graphs: `EventGraph` only ·
Is-Variable set (via `find_node_types`): **`GetBoostOutline`, `GetBoostBar`, `GetBar`** — three, no more.

**Slots:**

| slot | size | padding | HAlign / VAlign |
|---|---|---|---|
| `BarStack.VerticalBoxSlot_4` (BoostOutline) | `{value 1, Fill}` | `{0,0,0,1}` | Fill / Fill |
| `BarStack.VerticalBoxSlot_5` (Bar) | `{value 2, Fill}` | `{0,0,0,0}` | Fill / Fill |

**`Bar` (must be byte-identical after phase C):**
`percent 1` · `fillColorAndOpacity {0, 0.5, 1, 1}` · `barFillStyle Scale` · `barFillType LeftToRight` ·
`visibility Visible` · `renderOpacity 1` · track `{0.03, 0.03, 0.03, 0.7}` `RoundedBox` `resourceObject None` ·
fill `WhiteSquareTexture` / `Image` / white `{1,1,1,1}` / `imageSize {32,32}`

**`BoostBar`:** `percent 0` · `fillColorAndOpacity {0,0.5,1,1}` · `Scale` · `LeftToRight` · `HitTestInvisible` ·
`renderOpacity 0` · track `{0.22, 0.22, 0.24, 0.85}` `RoundedBox` · fill `WhiteSquareTexture` / `Image` / white

**`BoostOutline`:** `brushColor {1,1,1,1}` · background `WhiteSquareTexture` / `Image` / white / `{32,32}` ·
`padding {1.5,1.5,1.5,1.5}` · `HAlign_Fill` / `VAlign_Fill` · `HitTestInvisible` · `renderOpacity 0`

**EventGraph DSL (baseline — must be character-for-character intact after phase C):**

```
(event UserInterface|EventPreConstruct (IsDesignTime))
(event Siegebound|UI|EventOnHPChanged (CurrentHP MaxHP)
  (if (> MaxHP 0.0)
    (Progress|SetPercent (Variables|WBP_CombatantHealthBar|GetBar) (/ CurrentHP MaxHP))))
(event UserInterface|EventConstruct)
(event UserInterface|EventTick (MyGeometry InDeltaTime))
(event Siegebound|UI|EventSetTeamColor (R G B)
  (Progress|SetFillColorAndOpacity (Variables|WBP_CombatantHealthBar|GetBar) (Utilities|Struct|MakeLinearColor R G B 1.0)))
(event Siegebound|UI|EventSetDamageBoost (FillFraction R G B RowOpacity)
  (bind _linearcolor (Utilities|Struct|MakeLinearColor R G B 1.0))
  (bind _boostbar (Variables|WBP_CombatantHealthBar|GetBoostBar))
  (bind _boostoutline (Variables|WBP_CombatantHealthBar|GetBoostOutline))
  (Progress|SetPercent _boostbar FillFraction)
  (Progress|SetFillColorAndOpacity _boostbar _linearcolor)
  (Appearance|SetBrushColor _boostoutline _linearcolor)
  (Widget|SetRenderOpacity _boostoutline RowOpacity)
  (Widget|SetRenderOpacity _boostbar RowOpacity))
```

### ⚠️ 3.1 `SC-§40` cl. 9 — TWO THINGS THE LAW SAYS THAT THE MEASUREMENT DOES NOT

Reported **even though nobody asked and one of them is only cosmetic**, because that is the clause.

1. **`CONVENTIONS` §5's pixel split is stale.** It says *"boost bar ~5 px inside a 1.5 px frame (~8 px total) and health
   bar ~12 px, i.e. today's health-bar height exactly."* **Measured from the live slots**, 22 px with a 1 px bottom pad
   leaves 21 px split Fill 1:2 ⇒ **BoostOutline 7.00 px · Bar 14.00 px**, and BoostBar is **4.00 px** inside the 1.5 px
   frame. The law's numbers were written before `TASK-368` fixed `Bar`'s slot from Fill 1.0 to Fill 2.0 and were never
   re-derived. **Nothing is broken** — but the health bar is 14 px, not 12, and anyone budgeting from the law's numbers
   will be 2 px out.
2. **`TASK-861`'s own `names:` line and `blocked-by: none` are accurate on names, wrong on buildability** — §2.1.

---

## 4. ⭐⭐ THE DIFFERENTIATOR RULING (spec item 3) — *"say which one and why"*

**CHOSEN: FILL GEOMETRY, primary — `CastBarFill.barFillType = FillFromCenter`** (confirmed present in this engine:
the live enum is `[LeftToRight, RightToLeft, FillFromCenter, FillFromCenterHorizontal, FillFromCenterVertical,
TopToBottom, BottomToTop]`).

### Why the other three were refused — each for a measured reason, not a taste one

- ⛔ **Hue, as the primary cue: refused.** The two shipped bars already occupy the usable hue space. `Bar` is
  **team-tinted** — blue `(0, 0.5, 1)` on your units, red `(1, 0.1, 0.05)` on theirs — so *any* choice collides with one
  team or the other. `BoostBar` already ramps light-blue → navy → purple → black. Hue is used here, but only as the
  **secondary** cue (§4.1), because it cannot carry the read alone.
- ⛔ **Height: refused.** The entire stack is **22 px**. After a 7 px boost row and a 14 px health bar there is no
  headroom for a height difference large enough to resolve at gameplay distance.
- ⛔ **Position: refused as primary.** Three rows inside 22 px sit ~7 px apart. At the ~150 px on-screen unit size this
  project designs for (`CONVENTIONS`, the DrawSize comment), 7 px of vertical offset is not a reliable discriminator.

### ⭐ Why fill geometry wins, and it is specific to *this* mechanic

1. **It encodes the right thing.** Both shipped bars fill `LeftToRight` and both mean *magnitude* — how much HP, how much
   boost. The cast bar means something categorically different: **time**. A different fill geometry says "this is not a
   resource" before you have read a single pixel of colour.
2. **It is pre-attentive, which is exactly the test item (3) sets** (*"at the gameplay camera, at real distance, on a
   moving unit"*). With `FillFromCenter` the fill is symmetric about the midpoint with empty track on **both** sides, and
   its **left edge travels leftward while every other bar's fill edge travels rightward.** Opposite motion is detectable
   at spatial frequencies well below those needed to resolve 4 px of hue.
3. ⭐⭐ **It delivers requirement 4 — completed vs BROKEN — better than any colour could.** `WITCH-§9.1` row 4 is the
   requirement with no tell at all today. With a centre-out fill, a **completed** cast is the two halves reaching the
   ends *together*, and a **broken** cast is the spread *vanishing mid-flight*. Both are large, central, symmetric
   events. A left-anchored bar signals its ending with a small edge stopping somewhere — the weakest possible signal for
   the single perception the player most needs.
4. ⭐⭐ **It survives the exact problem that killed the animation option.** `WITCH-§9.2` ruled out the rig because the
   witch's team colour had to go on the **brim at 18%** — the shoulders are occluded from the top-down camera. A
   geometry cue in a screen-space overlay is **immune to that class of failure**: it is composited as a Slate overlay
   and never occluded by her own hat, by stonework, or by anything else.
5. ⭐ **And the strongest cue is free and is already the design: two-endedness.** Two centre-out bars appearing
   **simultaneously on two different actors** is a pattern nothing else in this game produces. `WITCH-§9.2` is right
   that the UI is the shape of the mechanic.

### 4.1 Secondary cue — hue, and the ⛔ ruling that it must be TEAM-NEUTRAL

**`CastBarFill.fillColorAndOpacity` = linear `(1.00, 0.55, 0.02, 1)` — warm amber/gold.** Track = `(0.03, 0.03, 0.03, 0.7)`,
reusing `Bar`'s own near-black. Computed contrast of amber on that track ≈ **8.2 : 1** — the highest on the widget
(the boost bar's best band is 4.50 : 1).

⛔⛔ **THE CAST BAR IS DELIBERATELY *NOT* TEAM-TINTED, AND THIS IS A RULING, NOT AN OVERSIGHT.** The obvious move is to
reuse `GetDefaultBlueBarColor()` / `GetDefaultRedBarColor()` per `W4-R5`. **It must not be done here:** an enemy witch's
cast bar would then be red `(1, 0.1, 0.05)` — **the exact colour of the enemy health bar sitting 1 px below it.** That is
the single worst collision available on this widget, and it would be introduced *in the name of* a convention.

⇒ `W4-R5`'s rule ("reuse the shipped team palette; never a new hardcoded colour") governs anything that **expresses
allegiance**. A cast bar expresses **time**, and it must read identically on a friendly and an enemy witch precisely so
that requirement 2 (*on WHOM*) is answered the same way in both directions. Amber is chosen as the one band neither the
team palette nor the boost ramp uses.

⚠️ **Where the amber lives — and why it is design-time, unlike the boost bands.** `CONVENTIONS` §5 says tint is DATA,
never hardcoded in the WBP — but that rule exists because **the boost colour changes with the value** and therefore needs
a runtime channel. The pinned cast contract is `OnCastProgressChanged(float CastPercent, bool bCasting)` — **float/bool
only, with no R/G/B channel at all** — and the cast colour is constant. ⇒ **A design-time constant on `CastBarFill` is
the only option the contract permits, and it is also the correct one.** Flagged explicitly so QA does not read it as a
`W4-R5` violation.

---

## 5. ⭐ THE PIXEL BUDGET (spec item 4) — EXACT ARITHMETIC, NO EYEBALLING

**`CastBarRoot`'s slot mirrors `BoostOutline`'s exactly: `Fill 1.0`, padding bottom `1`, inserted at index 0 (top).**
That choice makes the arithmetic exact in both states, with no fractional pixels:

| state | `DrawSize` | CastBarRoot | pad | BoostOutline | pad | Bar | total |
|---|---|---|---|---|---|---|---|
| **not casting** (`CastBarRoot` **Collapsed**) | **(90, 22)** | **0** (slot skipped entirely) | 0 | **7.00** | 1 | **14.00** | **22** |
| **casting** | **(90, 30)** | **7.00** | 1 | **7.00** | 1 | **14.00** | **30** |

Per-segment: `CastBarFill` = 7 − (2 × 1.5 padding) = **4.00 px**, identical to `BoostBar`'s 4.00 px.

⭐⭐ **Note what this buys: `BoostOutline` and `Bar` keep their exact current pixel heights (7.00 and 14.00) in *both*
states.** The health bar does not shrink when a cast starts. That is stronger than item (4) requires — item (4) only
demands that a *non-casting* bar be identical.

### ⛔⛔ 5.1 A MEASURED CONFLICT WITH `TASK-860` SPEC ITEM (4) — READ THIS BEFORE COMPILING `860`

`TASK-860` item (4) says: *"`DrawSize` grows to fit the new segment, following the boost bar's own arithmetic."*
**Read literally — as a constructor change from `(90,22)` to `(90,30)` — that ships a fleet-wide regression and breaks
`WITCH-§9.6`'s pixel-identical guarantee.**

The mechanism, measured: `BoostOutline` and `Bar` are **`Fill`** slots. Fill slots absorb *every* spare pixel. With
`CastBarRoot` collapsed and `DrawSize` at `(90,30)`, the remaining 29 px split Fill 1:2 ⇒ **BoostOutline 9.67 px and Bar
19.33 px** on **every building, the hero, and all 20+ units in the game.** That is exactly the *"a 2-px shift here is a
fleet-wide regression"* failure item (4) warns about — arriving via `860`'s side of the fence, from an instruction that
looks harmless.

⇒ ⭐ **"`DrawSize` grows" must mean grows AT CAST TIME** — a runtime `SetDrawSize((90,30))` paired to `bCasting` true and
`(90,22)` on false. Twice per cast, never per tick, so the never-per-tick law is untouched. That is the only reading of
`860` item (4) compatible with `WITCH-§9.6`.

⚠️ **And a rider `860` must handle if it takes that route: `UCombatantHealthBarComponent` never sets `Pivot`** (grepped:
the constructor sets `SetWidgetSpace`, `SetDrawSize`, and `SetRelativeLocation(0,0,BarHeightZ)` — no `Pivot`). It is
therefore the engine default **`(0.5, 0.5)`**, so a +8 px `DrawSize` grows the box **4 px up and 4 px down** — dropping
the bottom of the health bar 4 px into the unit's head at cast start. `860` must either set `Pivot = (0.5, 1.0)` or
compensate `BarHeightZ` by half the delta.

**If `860` prefers to avoid dynamic `DrawSize` entirely**, the fallback is: keep `(90,22)` and give `CastBarRoot` slot
`Auto` with a fixed 5 px `SizeBox`. Cost: the health bar shrinks 14 → 10 px for the 3 s of the cast. Legible, but it
degrades HP readability at the exact moment the player is deciding whether to attack. **I recommend the dynamic
`DrawSize`.**

---

## 6. 🧑 THE HUMAN STEP — copy-pasteable, ~90 s, `TASK-367`'s shape exactly

⚠️ **PIE must be stopped. Open ONLY `/Game/UI/WBP_CombatantHealthBar`. Do not save any other asset.**

1. In the Designer hierarchy, select **`BarStack`** (the root VerticalBox).
2. Add a **`Border`** as a child of `BarStack`, and **drag it ABOVE `BoostOutline`** so it is child **index 0** (topmost).
3. Rename it **`CastBarRoot`** — character-for-character; `TASK-860` binds it by string.
4. Tick **Is Variable** on `CastBarRoot`.
5. Its slot: **Size = `Fill`, value `1.0`** · **Padding Bottom = `1`** (left/top/right `0`) · HAlign **Fill** / VAlign **Fill**.
6. Set `CastBarRoot`'s **Padding = `1.5` uniform** (mirrors `BoostOutline`).
7. Set `CastBarRoot`'s **Visibility = `Collapsed`**. ⛔ **`Collapsed`, not `Hidden`** — a Hidden widget still occupies its
   slot and would move the health bar on every unit in the game.
8. Add a **`ProgressBar`** as the child of `CastBarRoot`. Rename it **`CastBarFill`**. Tick **Is Variable**.
9. **Compile** and **Save** — ⛔ *this asset only*. Decline any other save prompt, especially `L_Arena` and `BP_FogArea`.

⛔ **Do NOT set any colours or brushes** — phase C does that over MCP with readback, per §7.

---

## 7. WHAT PHASE C WILL DO (blocked on §6 **and** on `TASK-860` compiling)

- `CastBarFill`: fill brush **`WhiteSquareTexture`** / `DrawAs = Image` / tint **white `(1,1,1,1)`** (identity multiply);
  track `(0.03, 0.03, 0.03, 0.7)`; `barFillStyle = Scale`; **`barFillType = FillFromCenter`**;
  `fillColorAndOpacity = (1.00, 0.55, 0.02, 1)`; `percent 0`; `visibility HitTestInvisible`.
  ⛔ **No material anywhere** — `TASK-131`'s actual root cause was a material fill brush swallowing a runtime tint.
- `CastBarRoot`: background `WhiteSquareTexture` / `Image` / white; `brushColor = (0.02, 0.02, 0.03, 0.9)` (a static
  near-black 1.5 px frame, so bright amber survives being drawn over pale terrain — the residual case `CONVENTIONS` §5
  flagged for boost band 4). ⛔ Never pushed from C++; no contract needed for it.
- **EventGraph — one linear exec chain, authored additively** (`create_node` / `connect_pins` / `set_pin_value`;
  ⛔ **never `write_graph_dsl`**, which would re-emit the three working events — the operation `TASK-131` recorded as
  silently orphaning nodes):

  ```
  (event Siegebound|UI|EventOnCastProgressChanged (CastPercent bCasting)
    (Widget|SetVisibility CastBarRoot (Select bCasting HitTestInvisible Collapsed))
    (Progress|SetPercent  CastBarFill (/ CastPercent 100.0)))
  ```

  ⚠️ **`CastPercent` is `0..100` and `SetPercent` takes `0..1` — the widget MUST divide by 100.** Same shape as the
  `OnHPChanged` divide already in this graph. Omitting it ships a bar pinned full from frame one.
  ⚠️ A **`Select`** (not a `Branch`) keeps the single linear exec chain of `CONVENTIONS` §5. The enum appears only as a
  literal *inside* the graph — the widget-param law bans enums in the **event signature**, which this contract honours.
- ⛔ **THE GATE, before anything else is believed:** `ObjectTools.get_class` on the new event node must return
  **`/Script/BlueprintGraph.K2Node_Event`**, ⛔ **not `K2Node_CustomEvent`**, re-asserted *after* the compile.
  ⛔ **`bIsImplemented` is NEVER evidence** — `TASK-131` round 1 proved it lies.
- **Orphan check:** `find_nodes` before/after, delta must equal exactly the nodes created, all pre-existing nodes
  present with **identical refPaths**, and the three baseline events' DSL character-for-character intact vs §3.
- **Pixel-identical proof:** re-capture every property in §3 and diff. `Bar` and `BoostOutline` must be byte-identical.

---

## 8. ⚠️ AN ANOMALY I MUST REPORT — THE ASSET IS DIRTY AND I DID NOT EDIT IT

**`AssetTools.is_dirty("/Game/UI/WBP_CombatantHealthBar")` → `true`.** ⛔ **I made zero writes.** Every call I made was a
read: `exists`, `list_events`, `list_graphs`, `get_parent`, `list_properties`, `get_properties`, `get_class`,
`find_node_types`, `read_graph_dsl`, `is_dirty`, `IsPIERunning`, `find_assets`.

I swept the whole folder to see whether this was ambient: **of 44 assets under `/Game/UI`, exactly ONE is dirty — this
one.** So it is not a global editor state; it correlates perfectly with the one asset I read.

**Two candidate causes, and I did not run a destructive test to separate them** (that would have meant deliberately
dirtying a second widget in a project with this widget-corruption history):

1. ⭐ **Most likely — `find_node_types` dirties the package.** `TASK-368`'s handoff already recorded that these probe
   APIs *"report pins against transient probe nodes"*; it verified they leave **no node residue**, but never checked the
   **dirty flag**, and it saved the asset afterwards, which would have masked this.
2. The new DLL load reinstancing this natively-parented Blueprint (the same species as the `BP_FogArea` self-dirtying
   the dispatch warned about) — weaker, since 43 other UI assets stayed clean.

⛔ **I DECLINED TO SAVE IT.** I made no edits, so saving would persist an unknown delta into a widget every combatant in
the game carries. **Guidance for whoever is next in this file: dirty-on-arrival here is EXPECTED and is NOT evidence
that someone edited it — do not "clean it up" with a blind save.**

---

## 9. 🙋 WHAT NEEDS JONATHAN'S EYE

1. ⛔⛔ **THE 90-SECOND WIDGET STEP (§6).** Nothing else in this task can proceed without it. It is the same action he
   already performed once for the boost bar in `TASK-367`.
2. ⭐⭐ **THE ONE THE DISPATCH ASKED ABOUT — is a 4 px amber centre-out bar legible at true gameplay distance on a unit
   whose own team colour sits at 18% on the brim?** ⛔ **I cannot answer this and I will not pretend to.**
   `CaptureAssetImage` refuses WidgetBlueprints and screen-space Slate is uncapturable headless — the same wall
   `TASK-368` hit, and `TASK-131` is this project's record of structural readback passing on a **visually broken**
   widget. Specifically worth his eye once it is live:
   - Does **centre-out** actually read as different from the two left-to-right bars in motion, at real distance?
   - Is **4.00 px** enough vertical height for the amber to register, or does the cast row need to be Fill `1.5`
     (≈ 9 px, with `DrawSize` (90, 32))? **This is the number I am least confident in.**
   - ⭐ **The two-ended read — the thing the whole design exists for:** with a cast running, can he tell *at a glance*
     that the bar on the witch and the bar on her target are the **same** event, rather than two unrelated units each
     doing something?
   - Does the amber hold up against the **enemy red** health bar 1 px below it (§4.1's deliberate refusal to team-tint)?
3. ⚖️ **§5.1 — the `DrawSize` ruling.** Dynamic (`SetDrawSize` at cast start/end, my recommendation) vs static-22 with a
   transient HP-bar shrink. This is a `TASK-860` decision but it is a **design** call, not a code one.

---

## 10. FENCE — HELD

⛔ No C++ (`TASK-860` untouched) · ⛔ no `SK_Witch`, no rig, no montage (`TASK-863`) · ⛔ no mesh, material or card-art
change · ⛔ no gameplay code · ⛔ no Git · ⛔ `L_Arena` never opened, never saved · ⛔ **no duplicate, no reparent, no
rebuild** · ⛔ **no asset writes of any kind, and nothing saved.**
