# TASK-823 — [ST-4] THE CONTROLS MENU, THE TOWER HALF. Three new rows, and the hole `STACK-§4` could not satisfy is closed.

**Status: `ready-for-qa`** → gate is **TASK-816** (row (e)).
**Suite delta: `+2`.** Fresh census at hand-off: **378 across 29 files** (376 when I started; `376 + 2 = 378`, reconciles exactly).
⚠️ `TL-§5b` / TASK-815's warning applies to me too: **I own the `+2`, not the absolute.** `TASK-824` should reconcile deltas, not trust my total.

---

## 0. FILES TOUCHED — three, exactly the fence

| file | what |
|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/SiegeControlsHelpWidget.cpp` | **3 NEW rows appended** + 2 comment-only corrections (§6) |
| `Source/GitClaudeUnrealTest/Siegebound/SiegeControlsHelpWidget.h` | **comment only** — the stale "24 rows" claim (§6) |
| `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeControlsHelpTest.cpp` | **2 NEW tests** (15, 16), appended before `#endif` |

⛔ **No shipped row is edited, re-indented, reordered or deleted.** ⛔ `Cards.Discard`, `Cards.Play` and `Cards.Cancel` are **byte-identical** — `TASK-821`'s commit rebases clean over mine. ⛔ No gameplay code, ⛔ no WBP, ⛔ no asset, ⛔ no compile, ⛔ no editor, ⛔ no MCP, ⛔ no Git.

---

## 1. THE THREE ROWS

| id | category | headline | lane | keys |
|---|---|---|---|---|
| **`Cards.StackUpgrade`** | Cards | "Stack a tower taller" | `RawNonLetter` | `LeftMouseButton` |
| **`Cards.PlacementResize`** | Cards | "Resize what you are placing" | `RawNonLetter` | `MouseScrollUp`, `MouseScrollDown` |
| **`Interface.MapMarks`** | Interface | "Draw circles on the map" | `RawNonLetter` | `LeftMouseButton`, `RightMouseButton`, `MouseScrollUp`, `MouseScrollDown` |

**Placement in the file:** the two card rows sit **after `Cards.Cancel`, at the end of the Cards category**; the marks row sits **between `Interface.WarMapMarker` and `Interface.ControlsHelp`**, so the three war-map rows are contiguous. Both are pure insertions between blocks.

⭐ **Each row says WHEN its gesture applies before it says what it does** (spec (4)): *"While you are placing a building…"* · *"While a building's placement outline is up…"* · *"On the war map…"*.

⛔ **No tunable's value is typed anywhere.** `MaxStackHeightMultiplier`, `StackHealthStep`, `PlacementFootprintWheelStep`, `PlacementFootprintMin`, `PlacementFootprintMax` and `MaxMapMarks` are **NAMED**; the card's upgrade price is described as *"the card's own cost"*. **Measured: zero digit characters in all six strings** (test 15(e) asserts it, with a self-checked scanner).
⚠️ **Declared jargon cost:** these pages read `MaxStackHeightMultiplier` to a player, exactly as `PickMode.Resize` reads `GroupRadiusWheelStep`. That is the shipped precedent and its **F-3 flag for Jonathan is inherited, not re-argued** — I did **not** invent a number to avoid it.

---

## 2. `HELP-§2` MECHANISM 3 — THE CITATION TABLE. ⛔ Every sentence traces to a symbol I opened and read.

⛔ **Nothing below is taken from the task board, from `CONVENTIONS.md`, or from `TASK-813`/`TASK-815`'s handoffs.** Those told me *where to look*; the sentences are from the source. Per **`SC-§38` I cite the SYMBOL** — line numbers are hints and I do not give them (`CONVENTIONS.md`'s own numbers shifted **under me, mid-task**; see §8).

### `Cards.StackUpgrade`
| claim on screen | read at |
|---|---|
| hover your own building of the same card ⇒ BLUE | `ASiegePlayerController::ResolvePlacementUpgradeState` — seven ordered gates: building card → live building under cursor → own team → **same `CardID`** → `CanScaleFootprint()` → gold → `Ready` |
| blue makes the click upgrade, nothing is built | `UpdatePlacementGhost`'s `case EPlacementUpgradeState::Ready:` arm (`bPlacementValid = true;`) + `TryConfirmPlacement`'s `if (PlacementUpgradeState == EPlacementUpgradeState::Ready) { ConfirmStackUpgrade(*SiegeState); return; }` — it returns **before every spawn rule** |
| the colour is one more value on the shipped parameter | the ordered ternary `(PlacementUpgradeState == ...::Ready) ? UpgradeGhostColor : (bPlacementValid ? ValidGhostColor : InvalidGhostColor)` into the same `"GhostColor"` param |
| an enemy building never turns blue and keeps its old message | gate (3) `HoveredBuilding->GetTeamId() != OwnTeam` → `None` (⇒ the shipped clearance refusal, **no new vocabulary**) |
| the castle is not a target | gate (2)'s `Cast<ABuilding>` (`ACastle` is class-disjoint) |
| **height adds one more copy of the original and saturates** | `ABuilding::StackHeightMultiplier` — `FMath::Min(1 + Upgrades, Cap)`, `Cap` off the CDO's `MaxStackHeightMultiplier` |
| **health multiplies, compounding, uncapped** | `ABuilding::StackHealthMultiplier` — repeated `Multiplier *= Step` with **no ceiling term**; `Step` is the CDO's `StackHealthStep` |
| width/length untouched by an upgrade | `ApplyStackUpgrade`'s `Scale.Z = AuthoredHeightScaleZ * StackHeightMultiplier(StackUpgradeCount);` — X and Y never written |
| health is **granted, not healed** | same function: `MaxHP = OldMaxHP * StackHealthMultiplier(1);` then `CurrentHP += (MaxHP - OldMaxHP);` |
| at the cap the click still buys health + one HUD line at confirm | `ConfirmStackUpgrade`'s `if (ABuilding::StackHeightMultiplier(UpgradesAfter) <= ABuilding::StackHeightMultiplier(UpgradesBefore))` → `BroadcastRefusal(StackHeightCapNoticeText())` — **at confirm, never per frame** |
| the cost is the card's own | `PendingCost = Row->Cost;` in `EnterPlacementMode`, spent by `ConfirmStackUpgrade`'s `SiegeState.SpendGold(PendingCost)` |
| can't afford ⇒ RED + "Not enough gold" | gate (6) `CurrentGold < UpgradeCost` → `Unaffordable`, and `TryConfirmPlacement`'s `case EPlacementInvalidReason::Upgrade:` |
| the not-stackable refusal wording | `StackNotStackableRefusalText()` = *"That building cannot be stacked"* — **and it never names a card** |
| the card leaves your hand at confirm | `ConfirmStackUpgrade`'s `DeckComponent->ConfirmPlayFromHand(PendingHandSlot)` block |
| the wheel sizes what you **place**, not what you **grow** | `ConfirmStackUpgrade` never reads `PlacementFootprintScale`; `ApplyStackUpgrade` writes Z only |

### `Cards.PlacementResize`
| claim on screen | read at |
|---|---|
| wheel, during placement **only** | the `ApplyPlacementFootprintWheel();` call in `PlayerTick`'s placement branch, reached only past `if (!bInPlacementMode) { return; }`; the pick / targeting / war-map branches all `return` above it |
| one notch = one step, clamped both ends | `ASiegePlayerController::StepPlacementFootprintScale` — `SafeCurrent + SafeStep * NotchDelta` then `FMath::Clamp(..., SafeMin, SafeMax)` |
| **no shrinking, and it is a ruling** | `PlacementFootprintMin`'s own header comment (`J-3`), including the *"hide a building in a gap its mesh was never meant to fit"* reason |
| **width and length only, never height** | `MakePlacementFootprintScale3D` returns `FVector(SafeScale, SafeScale, 1.f)` |
| every placement starts at the floor | `EnterPlacementMode`'s `PlacementFootprintScale = PlacementFootprintMin;` |
| what you see is what you get | `UpdatePlacementGhost`'s `GhostActor->SetActorScale3D(MakePlacementFootprintScale3D(PlacementFootprintScale));` and `TryConfirmPlacement`'s `FTransform SpawnTransform(..., MakePlacementFootprintScale3D(PlacementFootprintScale))` — **one member, one function** |
| the room checks measure the size on screen | `TryGetPlacementFootprintRadius`'s `CalcBounds` through the component's live transform (`STACK-§6`), consumed by the clearance / obstacle / own-unit gates |
| unit and spell cards ignore it | `bPendingCardCanScaleFootprint = bPendingIsBuilding && CanCardActorScaleFootprint(...)` |
| the climbable one is **inert**, not nagged at | `ApplyPlacementFootprintWheel`'s leading `if (!bPendingCardCanScaleFootprint) { return; }` and its own comment (*"the refusal already has a voice … on the click"*) |

### `Interface.MapMarks`
| claim on screen | read at |
|---|---|
| left-click empty map places, at the lowest free number | `UWarMapWidget::TryPlaceMarkAtLocal` ← the left-button arm of `NativeOnMouseButtonDown`; `USiegeMapMarkSubsystem::FindLowestFreeNumber` |
| scroll on a circle resizes; the wheel elsewhere does nothing | `TryResizeMarkAtLocal` ← `NativeOnMouseWheel`; the `INDEX_NONE` arm changes nothing (*"not the nearest circle, not the last-touched one, not a map zoom"*) |
| right-click a circle deletes; right-click on empty map does nothing | the `GetEffectingButton() == EKeys::RightMouseButton` arm → `TryDeleteMarkAtLocal`, whose miss arm returns without writing anything |
| left-click a circle writes its name into the chat box, opening it first, and **submits nothing** | the mark arm's `OnPlacePicked.Broadcast(PickedMarkSymbol)` — **the same delegate the seven place markers use**, so the shipped open-then-append order (already documented on `Interface.WarMapMarker`) applies unchanged |
| **the map's place markers win a contested click** | `NativeOnMouseButtonDown`: the mark lane is entered only on `HitIndex == INDEX_NONE` from the marker hit test |
| deleting leaves a hole; numbers are permanent | `RemoveMark(Number)` + `FindLowestFreeNumber` (`M-1`), and the shipped deleted-status line that says so to the player |
| the cap refuses out loud and names the count | `MaxMapMarks` on the subsystem; the `AddMark`-failed arm reads the count **from the store at the moment of refusal** |
| yours only, never replicated | `class USiegeMapMarkSubsystem : public ULocalPlayerSubsystem` + its M8 declaration |
| survives closing the map; cleared at match reset; never saved | `WarMapWidget`'s own note that reset is the store's job + **`ASiegeGameMode`'s single `ClearMarks()` call site**, in the match-reset path |
| the number is a **place your commander understands** | `USiegeAssistantSnapshot` reads the mark store and publishes `FSiegeMapMark::MakeSymbol(Number)` into the place list |

---

## 3. ⚖️ THE RULINGS — all proceeding defaults, ⛔ none blocked

| # | question | ⚖️ ruled | why |
|---|---|---|---|
| **H-1** | **the map-marks row's LANE** — the gestures are **Slate events**, and `ESiegeInputLane::RawNonLetter`'s comment says *"a RAW-polled key"* | ✅ **`RawNonLetter`, declared** | ⭐ **The lane encodes a TRANSLATION rule, not a plumbing route:** what this row needs is *"label the reference keys verbatim, zero `GetPositionalKey` calls"*, which is Lane B's algorithm exactly, and the identity is **provable** (the table is A–Z only). ⛔⛔ **`PointerOnly` would be actively wrong: `ComposeKeyChipLabel` answers the single "Mouse click" affordance for that lane REGARDLESS of the keys it is handed** — so the **wheel would be structurally unable to appear on the row**, and the row would silently fail the one requirement it was boarded for. ⚖️ Lane D stays correct for `Interface.WarMapReveal`/`WarMapMarker`, which really are one click on one button; **neither is touched.** 🧑 One word moves it if QA disagrees — the cost is the wheel disappearing from the list. |
| **H-2** | the **upgrade** row's lane/key | ✅ **`RawNonLetter` + `LeftMouseButton`** | The upgrade IS the placement confirm, which is a **raw poll** (`WasInputKeyJustPressed(EKeys::LeftMouseButton)` → `TryConfirmPlacement`) — the identical lane and key as the shipped `PickMode.Confirm`. ⛔ Naming an `IA_*` here would put a raw key on the Enhanced Input query lane. |
| **H-3** | three separate rows, or one "placement" row? | ✅ **THREE** | `STACK-§4` requires the **three wheel meanings** be told apart, and two of them are mine. One row would put a wheel meaning and a click meaning under one chip. **Test 16 asserts the separation as a property.** |
| **H-4** | where the disambiguation lives | ✅ **in MY rows' prose + MY outbound edges — ⛔ NOT by editing the shipped pick pages** | `PickMode.Resize`/`PickMode.Confirm` are **correct today**; editing a correct page is how a help screen acquires its next falsehood, and they are not my rows. My wheel row names all three meanings in its **first paragraph** and links to the other two. ⚠️ **`TASK-751` §3 asks for the pick page itself to be amended — see §7, that task is still `backlog`.** |
| **H-5** | do I add my ids to test 1's `RequiredIds[]`? | ⛔ **NO — a new test instead** | That array is a shipped assertion two other tasks are serialised against, and `CARDBAR-§9` makes editing it to accommodate a row change an automatic fail. A new test costs it nothing and keeps a red legible. |

---

## 4. `HELP-§7` — EVERY EDGE CHANGE, DECLARED

**I added 9 outbound edges, all on rows I own. ⛔ I edited NO existing row's `RelatedActionIds`.**

| my row | → | why |
|---|---|---|
| `Cards.StackUpgrade` | `Cards.Play` · `Cards.PlacementResize` · `Cards.Cancel` | getting into placement, sizing what you put down, backing out — Jonathan's *"all the controls with it"* for this gesture |
| `Cards.PlacementResize` | `Cards.StackUpgrade` · `PickMode.Resize` · `Interface.MapMarks` | ⭐ **the other two wheel meanings, rendered underneath this page** — `STACK-§4`'s instruction satisfied by *linking* rather than by a second copy of their prose |
| `Interface.MapMarks` | `Interface.WarMap` · `Interface.WarMapMarker` · `PickMode.Resize` | you cannot reach any of it without opening the map; the place markers are the **other** clickable thing on the same screen and the one that **wins** a contested click; the pick wheel is the meaning most easily confused |

✅ **Verified mechanically: all 9 resolve, and registry-wide there are ⛔ ZERO dangling `RelatedActionIds` and ⛔ ZERO dangling `{ActionId}` detail tokens across all 27 rows.** ⭐ **No row lists itself.**

---

## 5. ⛔⛔ FINDING 1 — **`HELP-§7`'s CENTRAL HAZARD CLAIM IS MEASURED FALSE, AND `TASK-852` AS BOARDED WOULD ADD A DUPLICATE**

> ### ⚠️ **This is the most important thing in this handoff. I am blocking `TASK-852` and I did not write it.**

**`HELP-§7` states:** *"⛔ THERE IS ⛔ NO REFERENTIAL-INTEGRITY CHECK ON THIS FIELD … `SiegeControlsHelpTest.cpp`'s `RequiredIds[]` asserts a required SUBSET of rows; it does ⛔ not validate edges."*
**`TASK-852` §(1) is boarded on that premise:** *"WALK every registered row's `RelatedActionIds` and RESOLVE each id against the REGISTRY."*

**MEASURED AT SOURCE — that walk ALREADY SHIPS.** It is inside **test 9, `FSiegeControlsHelpAuthoredDetailTest`** (`"Siegebound.ControlsHelp.EveryRowHasAuthoredDetail"`), in its per-row loop:

```cpp
for (const FName RelatedId : Row.RelatedActionIds)
{
    TestNotNull(..., FSiegeControlsHelpRegistry::FindAction(RelatedId));
    TestNotEqual(..., RelatedId, Row.ActionId);
}
```

It walks **every** row from `GetActions()`, resolves **every** id **against the registry**, and is **derived, not transcribed** — i.e. it is `TASK-852` §(1) and §(2), already green, today.

**WHAT IS GENUINELY MISSING, and it is not nothing:**
1. ⛔ **the NEGATIVE CONTROL** (`SC-§39` / `SHIP-§9`) — nothing proves that walk can go **red**;
2. ⚠️ **DISCOVERABILITY** — it is buried in a test named for *detail authoring*, which is precisely why **two separate readers** (the law, then the task) concluded it did not exist;
3. ⛔ the failure message does not name the missing id distinctly from the owning row's other assertions.

⇒ **`TASK-852` should be RE-SCOPED, not cancelled**: the deliverable is the **control + the naming + the extraction into a test that says what it checks**, ⛔ not a second walk. **Its premise (0)(1) is stale.** ⚖️ This is `SC-§38` clause 4 exactly — *a law that describes BEHAVIOUR rots silently and instructs the next task to re-introduce the defect*, here as a duplicate.
✅ **The other half of `HELP-§7`'s claim is TRUE and I confirmed it:** `ComposeDetailContent`'s related loop does `const FSiegeControlsHelpAction* const RelatedRow = FindAction(RelatedId); if (RelatedRow == nullptr) { continue; }` — **dropped, with no log.** The runtime failure really is silent by construction.
⭐ **And my test 15(f) already ships the negative control for the lookup** (`TestNull` on a fabricated id), so `852` can point at a precedent rather than inventing one.
⛔ **I did not edit `HELP-§7` or the `TASK-852` row** — law and board are the manager's. **This is reported, not repaired.**

---

## 6. FINDING 2 — TWO COMMENTS I MADE FALSE, AND FIXED IN THE SAME EDIT

Adding rows made three shipped comments untrue. ⛔ **I broke them, so I fixed them, and they are comment-only:**

- `SiegeControlsHelpWidget.cpp`'s registry banner said **"24 rows"** → the count is **removed**, with a note saying why (`GetActions().Num()` is the answer that cannot rot) and naming the three rows `TASK-823` added. It also said *"THE ROW SET AND THE LANE COLUMN ARE 704's"* → reworded to *"the ORIGINAL row set"*.
- `SiegeControlsHelpWidget.h`'s `GetActions()` docstring said **"The 24 rows of …§4"** → same treatment.
- `ResolveDetailTokens`'s comment said *"all 24 registry rows even though 20 of the 24 pages contain no token"* → the counts are removed rather than re-typed.

⚠️ **`CARDBAR-§9` also says "24 rows measured today" and `STACK-§4` says "the 24 shipped rows".** Those are **law**, they are now stale, and **`SC-§38` clause 3 puts them on the gate over this diff** — ⛔ I did not edit `CONVENTIONS.md`. **Flagged for QA/manager: the registry is 27.**

---

## 7. ⛔ NAMED, ⛔ NOT BOARDED (the spec's own fence) — **and one of them is a live duplicate**

> The fence says *name what else shipped undocumented, do not board it.* **Measured, not relayed:**

1. ⛔⛔ **`TASK-751` — [HELP-12] "THE CONTROLS MENU LEARNS THREE NEW THINGS" — IS STILL `status: backlog`, AND IT OWNS THE MAP-MARKS ROW I JUST WROTE.** Its §(1) is *"Recall (`IA_Recall`) · **Map marks** · the death/ghost state"* and its §(3) is the circle disambiguation. ⇒ ⭐ **`CARDBAR-§9` recorded the marks row as "MEASURED MISSING" without noticing a task for it already existed and had never run.** ⚖️ **The manager must adjudicate:** `751` is now **partly delivered** — its remaining scope is the **Recall** row, the **ghost/death** row, and (if still wanted) §(3)'s amendment of the *shipped* pick pages, which I deliberately did not touch (ruling H-4). ⛔ **If `751` is dispatched unamended it will write a second map-marks row.**
2. **Still undocumented, exactly as the fence predicted:** **Recall** (`RECALL-§`'s `B`) and the **death/ghost state** — no row exists for either (measured: the 27 ids carry neither). ⛔ **Named, not boarded.**
3. 📌 Corroborating: **`SiegeMapMark.h`'s own header names `TASK-751` as the task that would write these help rows.** The MARKS batch believed it had covered this; the task simply never ran.

---

## 8. ⚠️ `SC-§38` FIRED **DURING** THIS TASK, AND I AM REPORTING IT AGAINST MYSELF

**`CONVENTIONS.md`'s line numbers moved under me, inside one session.** I read `MARK-§4` at `:5771`; ~20 minutes later `grep` put **`MARK-§3`** at `:5776` — a parallel edit had inserted ~5 lines above that region. ⛔ **Nothing rode the bad number** (I re-located `MARK-§3` by `awk`-ing between its symbol and `MARK-§4`'s), but it is the second instance of the citation-rot class in two days.
⇒ ⭐ **Every citation in §2 above is a SYMBOL — a class, a function, or a quoted expression. I have deliberately given no line numbers**, so this table cannot rot the way the ones it replaces did.

---

## 9. TESTS — `SC-§37`. **DELTA `+2`.** ⛔ I do not own an absolute.

`Tests/SiegeControlsHelpTest.cpp` **14 → 16**. ⛔ No second test file — this feature has one, by `HELP-§6`. ⛔ **No test above mine is edited**, `RequiredIds[]` included.

| # | test | ⭐ what it would CATCH |
|---|---|---|
| **15** | `Siegebound.ControlsHelp.TowerAndMapMarkRowsAreAuthoredAndRawLaned` | a missing row · a row shipped on the **`(undocumented — TODO)` fallback** (both halves — one-liner AND detail) · a detail page that is just the one-liner again · ⛔ a row flipped to **`PointerOnly`** (which would make its wheel invisible) · a row that grew an `IA_*` and quietly joined the Enhanced Input lane · the wrong keys · ⛔⛔ **a `GetPositionalKey` call introduced on these rows — asserted as a BEHAVIOUR on a simulated Dvorak layout, not as a grep** · a **typed tunable value** in either string · a **dangling outbound edge** |
| **16** | `Siegebound.ControlsHelp.TheThreeWheelMeaningsAreThreeDistinctRows` | ⭐⭐ **two wheel meanings collapsed into one row** · a wheel meaning with **no row** · a **fourth** wheel consumer documented without amending `MARK-§4` · three rows that read the same (id, headline, one-liner, detail and **category** must all be pairwise distinct) · a wheel row on the pointer lane |

**⭐ HOW THE CLAIMS ARE SHAPED (`SC-§37`):** there is **no `TestEqual(Chip, "LMB")` anywhere.** The no-translation claim is made **against what the accessor answers** on a scratch Dvorak subsystem; the wheel-meaning count is obtained by **scanning every row's reference keys**, never from a typed list.
**Every zero/identity claim is paired with its control:** a **fixture self-check** that the injected map really does move a letter (or "unchanged" is vacuous) · a **digit-scanner self-check** that it finds a digit when one is present · a **wheel-scanner self-check** that answers **both** ways on synthetic rows · and a **negative control** proving a fabricated related id does **not** resolve.

⛔ **WHAT THESE CANNOT PROVE (`SC-§32`):** nothing here scrolls a wheel, hovers a tower, opens the map, opens PIE or paints a row. **That the three pages READ well is Jonathan's part of `HELP-§6` and no agent may claim it** — `TASK-814` §(3b) is the pixel row.

---

## 10. 🔍 WHAT QA SHOULD SCRUTINISE HARDEST

1. ⭐⭐ **§5's finding.** It changes another boarded task. Please verify the walk in test 9 with your own eyes before `TASK-852` is dispatched.
2. ⭐ **Ruling H-1 (the marks row's lane).** It stretches `ESiegeInputLane::RawNonLetter`'s docstring ("raw-polled") to cover Slate-delivered mouse gestures. I argue the lane encodes the **translation rule**, not the delivery mechanism, and that Lane D would defeat the row's purpose. **Rule on it.** If you disagree the fix is one enum value plus `bPointerOnly = true` — and the wheel leaves the list.
3. ⚠️ **The prose is mine, and it is the thing a test cannot check.** Every factual sentence is in §2's table with the symbol I read it from. **Spot-check any row of it** — that is the only real gate on `HELP-§2` mechanism 3.
4. ⚠️ **The `WatchTower` is never named** in any of the three pages (nor in any comment as a *rule*). The exclusion is taught as *"the game asks each building whether it may be scaled"* with the player's reason (*"it is the one you climb"*), and ⛔ no socket / rung plane / standoff reaches the screen. **Check I did not leak `TOWER-§8.5a`'s machinery at the player.**
5. ⚠️ **Category choice.** The two placement rows are filed under **Cards** ("Cards and the HUD") because they only exist while a card is being placed; the marks row under **Interface** beside the war map. Test 16 leans on those three categories being **different** — if you move one, that assertion is what tells you.
6. ⛔ **I could not compile** (fence). Balanced-delimiter and prose checks were run mechanically: braces/parens/brackets **+0** on all three files with strings and comments stripped; **no `*/` inside any block comment I wrote** (`TASK-824`'s 14-error lesson, checked deliberately).
7. ⚠️ **`TestNotEqual` on `FName`** is used in test 15(f) — that exact shape already ships in test 9, so it compiles; every other equality claim I added uses `FString`.

---

## 11. 📌 M8 DECLARATION (`HELP-§5`, verbatim shape)

⛔ **No replicated property, ⛔ no new replicated class, ⛔ no new relevancy tier, ⛔ no RPC.** Three data rows in a client-local, code-authored overlay that **reads** the local player's input mappings and keyboard layout and **writes nothing anywhere**. ⛔ **The overlay stays read-only on the world** — nothing I added issues an order, moves gold, or touches `ApplyCursorInputState()`. ⛔ **`Escape` is untouched:** I added no key handler of any kind; `Escape` appears in my diff **zero** times.

---

## 12. ⛔ WHAT I DID NOT DO

⛔ No card row edited (`Cards.Discard`/`Play`/`Cancel` byte-identical) · ⛔ no other row's `RelatedActionIds` touched · ⛔ no `RequiredIds[]` edit · ⛔ **no `GetPositionalKey` call added to any production path** (see the precise statement below) · ⛔ no gameplay code, no `SiegePlayerController`, no `ABuilding`, no `UWarMapWidget` · ⛔ no WBP, no asset · ⛔ no `CONVENTIONS.md` or law edit · ⛔ no compile, no editor, no MCP (the editor is live and an art task is driving it — I left it alone) · ⛔ no Git.

### ⚠️ THE `GetPositionalKey` CLAIM, STATED EXACTLY — ⛔ because "zero" would have been a lie

- ✅ **`SiegeControlsHelpWidget.{h,cpp}`: ⛔ ZERO CALLS added.** My three rows are Lane B, the branch that returns `Row.QwertyReferenceKeys` **verbatim and calls nothing** — that is the whole point of the lane choice, and the label path is untouched. **The only two calls in the file remain the pre-existing pair in `ResolveRowDisplayKeys` (the Lane-C derive arm and the Lane-A fallback), neither of which my rows can reach.**
  - ⚠️ **ONE occurrence of the NAME is mine, and it is a comment, not a call:** the marks row's lane block quotes the requirement *"label the reference keys VERBATIM, with ZERO `GetPositionalKey` calls"* while arguing ruling H-1. ⛔ **A grep counting occurrences will see 8 where it saw 7 — that one is prose.**
- ⚠️ **`Tests/SiegeControlsHelpTest.cpp`: my test 15 DOES call `Dvorak.Layout->GetPositionalKey(...)` — as the ORACLE, ⛔ never as the thing under test.** It asks the accessor to **agree** that each of my keys is an identity on a Dvorak layout, which is what turns *"we skip the call because it would be a no-op"* into a measured fact instead of an assertion. ⭐ **This is the shipped idiom, not a new one:** test 4 (`RawLanesAreIdentity`) and test 14 both do exactly this, in the same file, for the same reason.
- ⇒ **`TASK-816` row (e) should read the criterion as "no `GetPositionalKey` on the label path", which holds.** ⛔ A bare grep of the diff will hit the test file — **that hit is intended, and here is why**, stated in advance rather than defended after a finding.
