# TASK-1585 — [HELP-TRUTH-FIXES] — programmer handoff

Marker `TASK-1585-HELP-TRUTH-FIXES`. 2026-09-28. gameplay-programmer. Status → `ready-for-qa`. Gate: `TASK-1586`.

Text only, plus one new test. ⛔ No compile, no PIE, no editor call, no mutating git. The only shell calls were read-only: `sha256sum`, `git --no-optional-locks hash-object` / `ls-files -s` / `diff`, and Python text simulations that read files in the scratchpad.

## §0 Start state (spec (0)): measured, all equal

| file | expected | measured |
|---|---|---|
| `SiegeControlsHelpWidget.cpp` | `419cefc8…3764` (`qa/TASK-1575.md` §9) | `419cefc896081b86aae6b03402ebf90967152a8a08733b6fefc03a646d893764` ✓ (295138 B, 4666 lines) |
| `SiegeControlsHelpWidget.h` | `31c98693…bf4e` | `31c986935a49b859e20212c164d148f0865993417ed39c4be89d150adcd7bf4e` ✓ (81619 B, 1409 lines) |
| `Tests/SiegeControlsHelpTest.cpp` | `41e7d944…7304` | `41e7d944ac87486bf2c0973ea0e17d880f49ba307dede0c7abb0400a58ad7304` ✓ (150755 B, 2628 lines) |
| `SiegePlayerController.cpp` | its `9b82e8d` blob | `hash-object` = `ls-files -s` = `rev-parse 9b82e8d:…` = `94ad60424fb5393413f286676c590ac11435808c` ✓ (sha256 `8f33a5de…b4fb`, 387770 B, 7770 lines) |

This is the first independent re-hash of `qa/TASK-1575.md`'s declared after-anchors. All three match. I copied them byte-for-byte to the scratchpad as `W.start.cpp` / `T.start.cpp` / `W.start.h` / `PC.start.cpp` (hashes re-measured equal); the diffs in §8 are taken against those copies.

## §1 Each change, old beside new (sites found by text; line numbers are hints only)

### (A) `Cards.StackUpgrade`, the player literal

- **Deleted, whole**, the paragraph "ONE KIND OF BUILDING REFUSES TO BE STACKED, AND IT IS THE ONE YOU CAN CLIMB. Its ladder is fixed to the building's exact shape, … Hovering one shows RED with "That building cannot be stacked"." Nothing replaces it. No building is singled out, and no per-building limit or number is added (`Q-STACK-CAP-2026-09-28` belongs to `TASK-1576`).
- **The dangling sentence.** Old: "A refusal of either kind costs nothing and leaves you in placement mode, so another building — or another patch of ground — still works. Back out entirely with {Cards.Cancel}." → New: "Being refused for gold costs nothing and leaves you in placement mode. Back out entirely with {Cards.Cancel}."
  - "either kind" pointed at nothing once the stack refusal was gone.
  - "another building — or another patch of ground — still works" is **false for the gold refusal that remains**. Upgrading any other building of the same card, or placing on open ground, costs the same `PendingCost`, so neither works either. I dropped it rather than keep a false clause.
  - Source for the new sentence: `TryConfirmPlacement`'s `if (!bPlacementValid)` branch ("refuse, spend NOTHING, STAY in placement mode … Every branch runs BEFORE any gold moves"). The `Upgrade` case with `Unaffordable` refuses with "Not enough gold", and the block `return`s without `ExitPlacementMode`. `ConfirmStackUpgrade`'s `SpendGold` refusal also returns without exiting.
  - The `{Cards.Cancel}` token is kept.
- ⛔ Banned words check on the new prose: no "balance", "design choice" or "design decision", no claim that a building cannot be stacked, no hover colour, no digit, no `::`, no `()`. Measured by script (§5).

### (A2) The comment block beside `Cards.StackUpgrade` (R-25), each item re-read at source and dated `TASK-1585 (2026-09-28)`

| comment | old | new / source |
|---|---|---|
| gate list | "… the SAME CardID -> CanScaleFootprint() -> gold -> Ready" | "… -> CanStackHeight() -> gold -> Ready". Source: `ResolvePlacementUpgradeState`'s gate (5) `if (!HoveredBuilding->CanStackHeight())`, repointed by STACK-§8 on 2026-09-03. Every shipped class answers true, so gate (5) turns nothing red. |
| castle case | "gate (2)'s `Cast<ABuilding>`" | `UpdatePlacementGhost`'s `Cast<ABuilding>(Hit.GetActor())`, which hands gate (2) a null. The resolver takes `const ABuilding*` and never casts. (Imprecise rather than stale; corrected in passing and declared.) |
| HEIGHT series | "Cap read off the CDO's MaxStackHeightMultiplier" | `ABuilding::StackHeightMultiplier(UpgradeCount, MaxMultiplier)`: the cap is handed in by the caller as the building's own `MaxStackHeightMultiplier`, per class since STACK-§10 cl. 2. The function's own comment records the one-argument `GetDefault<ABuilding>()` form it replaced. |
| Z only | "`StackHeightMultiplier(StackUpgradeCount);`" | "`StackHeightMultiplier(StackUpgradeCount, MaxStackHeightMultiplier);`", as in `ApplyStackUpgrade` (`Building.cpp`). |
| cap notice | "`StackHeightMultiplier(UpgradesAfter) <= StackHeightMultiplier(UpgradesBefore)`" | the two-argument form with `TargetHeightCap`, plus `const int32 TargetHeightCap = Target->GetMaxStackHeightMultiplier();`, as in `ConfirmStackUpgrade`. |
| the two refusals | (true in code) | Annotated: only the gold refusal is reachable. NotStackable's doc in `SiegePlayerController.h` says "NO SHIPPED CLASS PRODUCES THIS STATE". Both return before gold moves and without exiting placement. |
| "THE EXCLUSION IS TAUGHT AS A BEHAVIOUR … ABuilding::CanScaleFootprint() (AClimbableTower overrides it false)" | stale since 2026-09-03 | Rewritten as "THERE IS NO STACK EXCLUSION TO TEACH". All three stack gates ask `CanStackHeight()`: the resolver gate (5), `ConfirmStackUpgrade` and `ApplyStackUpgrade`, each read. `AClimbableTower` answers true, and `CanScaleFootprint()` is the wheel's predicate only. The paragraph and the "it is the one you climb" reason are deleted. The STACK-§2 no-CardID-compare law survives. It points at Q-STACK-CAP / `TASK-1576`. |
| `TASK-1574`'s "REPORTED AND NOT FIXED HERE" note | kept as history | Followed by "⭐ FIXED BY TASK-1585 (2026-09-28), which superseded TASK-973", with the old dangling sentence quoted and the reason it was replaced. The `TASK-1574` "exact shape" note now says its sentence was in the deleted paragraph. |

### (B) `PickMode.Resize`, the player literal

- Old: "The game reads the wheel directly rather than through its control setup, and it was checked that no control anywhere else is set to the wheel — it is inert everywhere except inside a pick."
- **New, quoted in full:** "The game reads the wheel directly rather than through its control setup, and it was checked that no control anywhere is set to the wheel. During a pick the wheel resizes only the circle you are drawing. It has two other jobs elsewhere: while you are placing a building it can resize that building, and on the war map it resizes one of your own map circles."
- Each clause traced at source:
  - **"no control anywhere is set to the wheel".** `MARK-§4`'s quoted shipped clause says "The wheel is verified globally unbound (no camera zoom exists)". I tightened "anywhere else" to "anywhere" because none of the three jobs is a bound control: two are `WasInputKeyJustPressed` polls and one is a Slate event. `qa/TASK-1575.md` §2 ruled it "true in the binding sense".
  - **"During a pick the wheel resizes only the circle you are drawing".**
    - `ApplyGroupPickWheel` writes `GroupPickRadius` and the active decal only.
    - The pick branch of `PlayerTick` `return`s before the placement branch.
    - The war map cannot be open during a pick: `BeginGroupPick` ignores while `bWarMapOpen`, and `SetWarMapOpen` refuses through `CanOpenWarMap` while `GroupPickStage != None` (its log line names "group pick").
  - **"while you are placing a building it can resize that building".** `ApplyPlacementFootprintWheel`, called only in the placement branch. The word is "can" because the wheel is inert for a card whose class answers `CanScaleFootprint()` false (`AClimbableTower`, `ClimbableTower.h`).
  - **"on the war map it resizes one of your own map circles".** `UWarMapWidget::NativeOnMouseWheel` → `TryResizeMarkAtLocal`. Same words as `Cards.PlacementResize`'s page.
- **The comments beside it:**
  - The R-17 header "it is inert everywhere except inside a pick (…:2787-2791, call site :599)" now names `ApplyGroupPickWheel` and `MARK-§4`'s three consumers, with the old text quoted and dated.
  - The citation "polled-not-bound and inert outside a pick = :2787-2791, call site :599" loses the "inert" claim, with a dated note. (The line numbers themselves stay ⛔ UNVERIFIED under the file-level `TASK-1480` (a) rule; I did not re-anchor them.)
  - `TASK-1574`'s "REPORTED AND NOT FIXED HERE" note is followed by "⭐ FIXED BY TASK-1585", which quotes the old text and gives every new clause with its source.

### (B) fence widening: `SiegePlayerController.cpp`, `PlayerTick`'s header comment only

- Old: "(the CONVENTIONS wheel law: the wheel is inert everywhere but inside this branch)"
- New: "(the CONVENTIONS wheel law, MARK-§4 as amended by STACK-§4: "the wheel has exactly THREE consumers — (1) the controller's group-pick poll · (2) UWarMapWidget while the map is open and the cursor is over it · (3) the controller's PLACEMENT-mode footprint poll (ApplyPlacementFootprintWheel, STACK-§4). It stays inert everywhere else"; this branch is consumer 1. TASK-1585, 2026-09-28: this read "the wheel is inert everywhere but inside this branch")"
- MARK-§4 is cited by its text, with emoji and backticks stripped. The quote is truncated after "everywhere else"; the omitted tail is "and no FOURTH consumer may be added …".
- It is a `//` comment with no braces and no `ApplyPlacementFootprintWheel()` / `();` call shape. `SiegePlacementTest`'s `CountOccurrencesInCode` skips `//` lines and `CheckPrecedes` runs on `CodeLinesOnly(TickBody)`, so no census moves.
- **Nothing else in the file moved** (§8: 1 hunk, −2 / +7).

### (C) The registry comment (`.cpp`, "THE ACTION REGISTRY" header)

- Old: "⛔ THE ORIGINAL ROW SET AND THE LANE COLUMN ARE 704's. Every lane assignment below traces to that file's §1.1 audit, and every one-liner is its §4 text VERBATIM."
- New: the original row set is 704's, and so are its lanes and one-liners, **except `Cards.Discard`'s**. `TASK-821` rewrote that one-liner in place and moved its lane from PointerOnly to MappedAction, per its own R-09 block. Rows 25–27 carry `TASK-823`'s own lanes and prose. Dated, and it says it now matches the `.h` `OneLine` doc.
- Measured, not assumed:
  - I compared `handoffs/TASK-704-programmer.md` §4's 24 lane tags against the registry's `ESiegeInputLane::` values. Cards.Discard is the only one that changed (D → MappedAction).
  - One-liners: `qa/TASK-1575.md` §4 measured 23 of 24 identical, all but Discard.

### (D) Test 9's docstring

- Old: "across 24 strings".
- New: "across every row's string (twenty-seven rows as of TASK-1585, 2026-09-28; it read "24 strings" until TASK-823 appended three, and the loop walks GetActions(), so it covers whatever the registry holds)".
- The count is spelled in words and dated. The historical "on all 24 rows" is untouched, per `qa/TASK-1575.md` §6 (e). The live count is 27, measured by parser (§5).

### (E) QA's wording NITs, taken

| NIT | row | old | new | source |
|---|---|---|---|---|
| N2 | `Cards.CursorHold` | "a release only counts while the key is really held" | "each hold is let go only once" (QA's exact form) | `ClearUICursorHold`: `if (bUICursorHeld) { bUICursorHeld = false; SetIgnoreLookInput(false); }`. `OnUICursorPressed` sets the record only when it is not already set. Four other `ClearUICursorHold()` callers can end a hold while the key is down, counted by grep (`SiegePlayerController.cpp`). |
| N3 | `Interface.AssistantConsole` | "clicking away or clearing the box" | "clicking or tabbing away or clearing the box" | `HandleTextCommitted` returns on every commit method except `ETextCommit::OnEnter`. `OnUserMovedFocus` is any loss of focus. |
| N4 | `Interface.AssistantConsole` | "A close is not a cancel: closing the window cancels nothing — only the assistant itself may turn one into the other." | "Closing the box does not cancel anything by itself — the assistant decides what a close means, and if it was waiting for you to confirm an order, it discards that order." | `SiegeAssistantConsoleWidget.h` "A CLOSE IS NOT A CANCEL *IN THIS WIDGET*": every close route reuses `CloseConsole`, which broadcasts `OnConsoleOpenChanged(false)` and no cancellation. `USiegeAssistantComponent::HandleConsoleOpenChanged` → `NotifyConsoleClosed`: `AwaitConfirm` → `ClearConfirmPreview`, `ClearPendingIntent`, `Idle`. It now agrees with the `Interface.AssistantAccept` block ("or just close the box to discard it"). The other outcomes (Thinking aborts, Deferred keeps its latch) sit under "the assistant decides". |

N1, N5 and N6 were not taken, as ruled on `TASK-1584`. The `Hero.Jump` guard anchor is byte-identical.

### (F→A) One further false clause fixed under (A)'s rules: `Cards.PlacementResize`

- Old last sentence: "… and the wheel is simply dead on it rather than nagging you once per notch — that refusal speaks once, at the click."
- New: "… and the wheel is simply dead on it rather than nagging you once per notch."
- **Why it was false.** No refusal speaks at the click for a climbable building's size. `TryConfirmPlacement` spawns it at `MakePlacementFootprintScale3D(PlacementFootprintScale)`, the floor the wheel never moved. The "voice" this clause leaned on is `ApplyPlacementFootprintWheel`'s comment "the RED ghost plus "That building cannot be stacked" on the click". That is the NotStackable state, which no shipped class produces, and it was a stack refusal, never a footprint one.
- **Scope.** It is one clause of one sentence, and the only `Cards.PlacementResize` literal byte that moved. The R-26 citation bullet that cited the stale comment is corrected and dated, and a TASK-1585 note beside the string gives old beside new.
- ⚠️ **For QA:** this executable change is authorised by (F)(i), "fix it under (A)'s rules if it is one sentence". It is not one of the literals (A), (B) or (E) name by row, so I declare it here.

### (G) The pins: new test 19

`Siegebound.ControlsHelp.NoPageTeachesARefutedStackOrWheelRule` (`FSiegeControlsHelpRefutedRulesTest`), appended after test 18. There is also a 5-line index entry in the file header.

- **(a) Fixture self-checks.** The refuted prose is copied literal for literal from the start bytes: the whole deleted stack paragraph and the whole old `PickMode.Resize` page. The same predicate must find "refuses to be stacked", "cannot be stacked" and "inert everywhere" in them, and must **not** find "placing a building" or "war map" on the old pick page. This is the in-test proof that each pin is red on the text it replaces.
- **(b) Negative pins, registry-wide.** For every row in `GetActions()`, `ComposeOneLineForDisplay` and `ComposeDetailForDisplay` carry none of "refuses to be stacked", "cannot be stacked" or "inert everywhere" (case-insensitive, one `TestFalse` each). There is also a vacuity guard.
  - Every rendering is covered: `ComposeDetailContent` renders each related block from that row's own `ComposeDetailForDisplay` text, read at source.
  - The (A) pin reads the whole page, so it depends on nothing in "WHAT AN UPGRADE BUYS" or "AT THE HEIGHT LIMIT", and its arm anchor sits in the page's last paragraph.
- **(c) Positive pin for (B).** The `PickMode.Resize` detail contains "placing a building" and "war map".
- **The pins against both byte sets.** I simulated them with a parser that composes each row's literals from the source (`sim.py`, scratchpad).
  - On `qa/TASK-1575.md`'s bytes: **5 failures**. Cards.StackUpgrade detail ×2 (both (A) phrases); PickMode.Resize detail 'inert everywhere'; and both positive jobs missing.
  - On the new bytes: **0 failures**. 27 rows parsed; test 9's guard fragments: 0 hits.

## §2 The arms `TASK-1578` owes (`SHIP-§9`: each pin seen red)

⚠️ Run them one at a time, as byte replaces on BOM-less UTF-8. ⛔ No `Get-Content` / `Set-Content`, ⛔ no `-replace`; see `qa/TASK-1575.md` §5 for the method. Both anchors are **ASCII-only**. Before injecting, assert the anchor count == 1 in `Source/` **at `qa/TASK-1577.md`'s bytes**, since `TASK-1576` edits this file next. Restore by byte copy and re-hash equal.

| arm | file | anchor (occurs once in `Source/` at my bytes, measured) | replace once with | bytes | expect |
|---|---|---|---|---|---|
| **1585-A** | `SiegeControlsHelpWidget.cpp` | `The size you dial in with the wheel applies to what you PLACE, not to what you GROW: an ` (the `Cards.StackUpgrade` last paragraph, outside both `TASK-1576` splice paragraphs) | `That building cannot be stacked. ONE KIND OF BUILDING REFUSES TO BE STACKED. The size you dial in with the wheel applies to what you PLACE, not to what you GROW: an ` | **+77** | exactly one red test, test 19, with exactly 2 errors (below) |
| **1585-B** | `SiegeControlsHelpWidget.cpp` | `It has two other jobs elsewhere: while you are placing a building it can resize that building, and on the war map it resizes one of your own map circles.` (the `PickMode.Resize` literal) | `It is inert everywhere except inside a pick.` | **−109** | exactly one red test, test 19, with exactly 3 errors (below) |

**The red test:** `Siegebound.ControlsHelp.NoPageTeachesARefutedStackOrWheelRule`.

Arm 1585-A expected error lines (UE's `TestFalse`: `Expected '%s' to be false.`):
- `Expected 'Row 'Cards.StackUpgrade' detail teaches the refuted rule 'refuses to be stacked' (TASK-1585 (A): no shipped building refuses the stack, STACK-§10)' to be false.`
- `Expected 'Row 'Cards.StackUpgrade' detail teaches the refuted rule 'cannot be stacked' (TASK-1585 (A): no shipped building refuses the stack, STACK-§10)' to be false.`

Arm 1585-B expected error lines:
- `Expected 'Row 'PickMode.Resize' detail teaches the refuted rule 'inert everywhere' (TASK-1585 (B): the wheel has three jobs, MARK-§4)' to be false.`
- `Expected 'PickMode.Resize names the wheel's other job 'placing a building' (TASK-1585 (B): MARK-§4's three consumers)' to be true.`
- `Expected 'PickMode.Resize names the wheel's other job 'war map' (TASK-1585 (B): MARK-§4's three consumers)' to be true.`

**Reading rule.**
- 1585-A: exactly 2 errors, one per (A) phrase. One error means a dead phrase entry.
- 1585-B: exactly 3 errors. 1 error means the positive pin is dead; 2 means the negative pin is dead.
- Any other red test means the arm leaked. Either case ⇒ STOP.

**Why no other test can go red.** Checked by simulation and by reading:
- The injected text carries no digit (test 15 (e)) and no `.cpp:` `.h:` `handoffs/` `TASK-` `SPC:` `**` `` ` `` `§` `::` `()` (test 9's guard: 0 hits on both mutants).
- It adds no `{` token (test 10).
- Both details stay longer than their one-liners and pairwise distinct (tests 9, 15, 16).
- Test 11 compares `PickMode.Resize`'s block to its own text, which the mutant changes identically.
- No other test pins either row's prose.

Measured mutant hashes at MY bytes, which are informational only because `TASK-1578` runs on `qa/TASK-1577.md`'s bytes:
- 1585-A: 304435 → 304512 B, `365453ad…71b9`.
- 1585-B: 304435 → 304326 B, `40d6df76…dc0a`.

**Still owed from before, unchanged:** the `TASK-1574` guard arm (`qa/TASK-1575.md` §5). Its anchor `Falling out of the world is a death, not a despawn` still occurs **exactly once** in `Source/`, and so does the ASCII form `TEXT("Falling out of the world is a death, not a despawn ` (measured, 1 = 1). The old anchor `default handling (which would delete your hero outright)` is at 0.

**Arms owed by `TASK-1585`: 2 (1585-A, 1585-B), plus the one `TASK-1574` arm = 3 for this file set before `TASK-1576` adds its own.**

Optional, not owed: the one-liner `TestFalse`s use the same `CarriesPhrase` lambda on a different field, and no arm exercises that field. If `TASK-1578` wants one: inject `That building cannot be stacked. ` at the start of a one-liner literal. Expect 1 error: `Row '<id>' one-liner teaches the refuted rule 'cannot be stacked' …`.

## §3 `N`

**566 → 567.** One test is **added** (test 19). No existing test changed shape: test 9 changed in its docstring only, and the header gained an index comment. `IMPLEMENT_SIMPLE_AUTOMATION_TEST` in this file: 18 → 19. The final `N` is `TASK-1576`'s.

**5a shape:**
- The `.h` is unchanged (`31c98693…bf4e`), so there is no UHT/`.generated.h` change from this row.
- The controller change is one comment.
- Three `.cpp` files recompile. The new test compiles into the module's test TU as it already does.

## §4 (F) The stack-truth census, and the war-map ledger line

**Pages:** `Cards.StackUpgrade`, `Cards.PlacementResize` and `Cards.Play`, one-liners and display names included. Each stacking, height or footprint rule was checked at source against the shipped stack lane: `ResolvePlacementUpgradeState`, `UpdatePlacementGhost`, `TryConfirmPlacement`, `ConfirmStackUpgrade`, `ABuilding::ApplyStackUpgrade` / `StackHeightMultiplier` / `StackHealthMultiplier`, `ApplyPlacementFootprintWheel`, `StepPlacementFootprintScale`, `MakePlacementFootprintScale3D`, `EnterPlacementMode`, and `AClimbableTower`'s two overrides.

| # | page | rule | verdict |
|---|---|---|---|
| 1 | StackUpgrade | blue needs a building card, a live own building, and the same card | ✓ gates (1)–(4) |
| 2 | StackUpgrade | an enemy building stays red on the ordinary clearance rule | ✓ gate (3) → None → shipped gates (by citation, not re-derived) |
| 3 | StackUpgrade | +1× original height per upgrade, stops at "a set maximum multiple" | ✓ `Min(1+Upgrades, Cap)`, cap per instance; the wording is still true with per-class caps |
| 4 | StackUpgrade | width and length untouched | ✓ `Scale.Z` only |
| 5 | StackUpgrade | health × a set factor, compounding, no ceiling | ✓ `StackHealthMultiplier`, CDO `StackHealthStep` |
| 6 | StackUpgrade | granted, not repaired | ✓ `CurrentHP += (MaxHP - OldMaxHP)` |
| 7 | StackUpgrade | at the limit: still buys health, stays blue, HUD line at confirm | ✓ gate (7) ignores the cap; the cap notice fires in `ConfirmStackUpgrade` |
| 8 | StackUpgrade | the card's own cost; the card leaves the hand at confirm | ✓ `PendingCost = Row->Cost`; `ConfirmPlayFromHand` |
| 9 | StackUpgrade | unaffordable ⇒ red + "Not enough gold"; "Blue never promises a click that will be refused" | ✓ `UpdatePlacementGhost` recomputes in the same `PlayerTick`, immediately before `TryConfirmPlacement` |
| 10 | StackUpgrade | "ONE KIND OF BUILDING REFUSES TO BE STACKED … RED …" | ✗ **FALSE → deleted (A)** |
| 11 | StackUpgrade | "A refusal of either kind … another building — or another patch of ground — still works" | ✗ **FALSE (dangling, and false for gold) → rewritten (A)** |
| 12 | StackUpgrade | wheel size applies to what you place, not what you grow | ✓ |
| 13 | PlacementResize | three wheel meanings, one mode at a time | ✓ PlayerTick branch `return`s; the war map's mutual exclusion |
| 14 | PlacementResize | set step, floor, ceiling; never below the floor | ✓ `StepPlacementFootprintScale` clamp; `PlacementFootprintMin = 1.0f` (J-3) |
| 15 | PlacementResize | width and length only | ✓ `FVector(SafeScale, SafeScale, 1.f)` |
| 16 | PlacementResize | every placement starts at the floor | ✓ `PlacementFootprintScale = PlacementFootprintMin;` (both sites) |
| 17 | PlacementResize | ghost = spawned size; room checks on the scaled ghost | ✓ `SetActorScale3D(MakePlacementFootprintScale3D(…))`, `SpawnTransform`, `TryGetPlacementFootprintRadius` |
| 18 | PlacementResize | unit and spell cards ignore the wheel | ✓ `bPendingIsBuilding && CanCardActorScaleFootprint(…)`; spells never enter placement |
| 19 | PlacementResize | the climbable building can't be resized; the wheel is dead rather than nagging | ✓ `CanScaleFootprint()` false; the early `return` |
| 20 | PlacementResize | "— that refusal speaks once, at the click" | ✗ **FALSE → clause deleted (F→A)** |
| 21 | Cards.Play | building card → placement; card leaves the hand only at confirm | ✓ (also true of upgrades). No stacking, height or footprint rule stated. |

**Count: 3 false sentences found on these pages, 3 fixed, 0 left unfixed.** (#10 and #11 are (A)'s own; #20 is the one further sentence.)

Flagged as judgment calls, **not** counted false and **not** changed (`SC-§101`, for QA or the manager to rule):
- **`Cards.StackUpgrade`'s one-liner** "…the outline turns blue and the click makes that one taller instead of building a new one." It is a happy-path summary. At the height limit the click adds health only, and when unaffordable the outline is red. The same page's "AT THE HEIGHT LIMIT" and "WHAT IT COSTS" paragraphs state both. I treated it like `qa/TASK-1575.md` §6 (d): a summary, not a false rule. If ruled false, a one-word fix ("upgrades that one") is available, but it collides with the display name below.
- **The display name "Stack a tower taller".** Not a sentence. Every `ABuilding` subclass can be stacked: `ABarracks`, `ADeepMine`, `ATower` and `AClimbableTower`, with `CanStackHeight()` defined only in `Building.h` (true) and `ClimbableTower.h` (true). So "tower" is narrower than the truth. It is outside (A)'s literal set; reported only.

**(ii) War-map right-click ledger line.** `Interface.WarMap` teaches **no** right-click, so the refuted "closed by right-click or Escape" claim (`HELP-§2` mech. 4, occurrence (i)) is **absent**. Measured on the new bytes:
- "right-click", "right click" and "right button": 0 of 3 on its one-liner and 0 of 3 on its detail.
- The only close sentence left is "Escape closes it too, checked directly every frame".

Evidence is test 17, `RightClickIsTaughtOnlyByTheRowsThatOwnIt`:
- (d1): no RMB reference key on the row.
- (d2): 3 fragments × one-liner and detail, 6 `TestFalse`, with the same-role positive control on `Interface.MapMarks` (which does carry "right-click", measured).
- (d3): delegation to `Interface.MapMarks` through `RelatedActionIds`.

**Nothing changed.**

## §5 Measurements (scripts in the scratchpad `t1585/`: `sim.py`, `arms.py`, `pages.py`)

- **Parser.** 27 rows on both byte sets. Test-19 simulation: old = 5 failures, new = 0. Test 9's guard fragment scan: 0 hits on the old, new, 1585-A and 1585-B bytes.
- **The new player prose** (the seven changed literal runs): 0 digits, 0 `::`, 0 `()`, 0 of "balance", "design choice", "design decision", "cannot be stacked", "red", "blue" or "green".
- **Arm anchors.** Counted in `Source/`: arm A = 1, arm B = 1, guard = 1 (both forms).
- **Page lengths, for `TASK-1579`'s page-fit check** (`qa/TASK-1575.md` §7, W2). This is my own composition count: display + one-liner + detail + related display + detail. It is not QA's method, but the deltas carry over.
  - **Grew +168**, because the `PickMode.Resize` block renders on each: `Orders.Follow` (5599 → 5767), `Orders.Hold`, `Orders.Ambush`, `Interface.MapMarks` (5176 → 5344), `PickMode.Confirm`, `PickMode.Cancel` and `PickMode.Resize`.
  - **Grew +64:** `Interface.AssistantConsole`, `Interface.AssistantAccept` and `Interface.WarMapMarker`.
  - **Shrank:** `Cards.StackUpgrade` −584, `Cards.PlacementResize` −416 (still the longest page), `Cards.Play` and `Cards.CursorHold` −21.
  - ⇒ **`Orders.Follow` is the page to watch.** It was already on §7's list, and its last sentence is `PickMode.Cancel`'s "Escape is how you cancel."

## §6 Found outside the fence, reported and not edited (`SC-§101`)

1. **`SiegePlayerController.cpp`, `ApplyPlacementFootprintWheel`'s leading comment:** "The WatchTower's refusal already has a voice — the RED ghost plus "That building cannot be stacked" on the click (TASK-813)". Stale since STACK-§8/§10: NotStackable is unreachable. It is the source the `Cards.PlacementResize` clause leaned on. The fence ("Nothing else in that file moves") keeps it out.
2. **`WarMapWidget.cpp`, the comment above `NativeOnMouseWheel`:** "⛔ THE WHEEL NOW HAS EXACTLY TWO CONSUMERS AND ⛔ NO THIRD MAY BE ADDED WITHOUT AMENDING MARK-§4 BY NAME". `MARK-§4` names three since `STACK-§4`.
3. **`SiegePlayerController.cpp` `ApplyGroupPickWheel` comment, and `SiegePlayerController.h`'s doc on the same function:** "the wheel is verified globally unbound and must stay INERT outside the pick". This is the pre-`MARK-§4` wording of the law. It is still true of this function's own poll, but not of the wheel.
4. **`Cards.StackUpgrade` one-liner and display name:** see §4.

## §7 Files touched, sha256 before → after

| file | before | after | size / lines |
|---|---|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/SiegeControlsHelpWidget.cpp` | `419cefc896081b86aae6b03402ebf90967152a8a08733b6fefc03a646d893764` | `9b078d0ec6a51f4439ce8d0bfa9ab1bb39fdcc24dfdd52a8c426de3ea8b4020d` | 295138 → 304435 B · 4666 → 4771 |
| `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeControlsHelpTest.cpp` | `41e7d944ac87486bf2c0973ea0e17d880f49ba307dede0c7abb0400a58ad7304` | `c45412c9cd2597fdb2f836b749fc0753c5686b8833a7c5f9d7a43e914748b38b` | 150755 → 160530 B · 2628 → 2790 |
| `Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.cpp` | `8f33a5de3c8e06fe39ef3a484756d93f40facd8c0cadaea81116d40078db7fb4` (blob `94ad604` = `9b82e8d`) | `d2dfbf5021b1594162d8e0a50d2b6cced2e7327c7833b912dd324d121995b1fd` (blob `cc420f5`) | 387770 → 388165 B · 7770 → 7775 |
| `Source/GitClaudeUnrealTest/Siegebound/SiegeControlsHelpWidget.h` | `31c98693…bf4e` | **unchanged** `31c986935a49b859e20212c164d148f0865993417ed39c4be89d150adcd7bf4e` | — |

All files are LF and BOM-less, as found; the `Edit` tool preserved both. Git warns "LF will be replaced by CRLF the next time Git touches it" for the start copies too, so that is the repo's `autocrlf`, not something this row introduced.

**Start anchors for `TASK-1576`** (for `TASK-1586` to confirm): the `.cpp`, the test file and `SiegePlayerController.cpp` at the "after" values above. The `.h` stays at `qa/TASK-1575.md`'s value.

**Note for `TASK-1576`:** test 19 now bans "cannot be stacked" and "refuses to be stacked" (case-insensitive) on every page. If option A's per-building numbers are phrased as "cannot be stacked past …", the suite goes red. Phrase them positively, for example "stacks up to …".

## §8 `git --no-optional-locks diff -U0`

- The help files are diffed against `qa/TASK-1575.md`'s bytes: the scratch copies, whose sha256 equal §0. They are uncommitted, so I used `git diff --no-index`.
- The controller is diffed against `9b82e8d`.
- Hunks: `.cpp` 22 · test 3 · controller 1.
- The diff itself is the fenced block at the very end of this file, after "Not examined / limitations".

## Not examined / limitations

- ⛔ **No compile, no suite run, no PIE, no pixels.** "N = 567", "each arm reds exactly one test with 2 / 3 errors" and "0 failures on the new bytes" are text-level predictions from a Python parser that composes the `TEXT("…")` runs. `TASK-1578` measures them. The parser's C-unescape covers `\"` `\n` `\\` only, which is all these literals use.
- The census re-read the rules on the three named pages. **Other pages** were read only where a changed literal lives (`Cards.CursorHold`, `Interface.AssistantConsole`, `PickMode.Resize`) or where `PickMode.Resize` renders. The truth of untouched sentences elsewhere was not examined.
- **Row 2 (enemy stays red on the clearance rule)** is accepted by citation (gate (3) → None → shipped gates). I did not re-trace which shipped gate fires first when the cursor sits on an enemy building's roof, for example the slope or obstacle gate.
- The `file:line` numbers in the `Citations (T1)` blocks I touched stay ⛔ UNVERIFIED under the file-level `TASK-1480` (a) rule. I corrected claims and quotations, not line numbers.
- The `MARK-§4` quote in the `PlayerTick` comment drops emoji and backticks and ends at "everywhere else".
- The page-length figures (§5) are my own counting method, not `qa/TASK-1575.md`'s and not pixels.
- I copied test 19's `OldStackParagraph` / `OldResizePage` fixtures from `W.start.cpp`'s literal lines, then checked them by parser against the old registry:
  - the composed `OldStackParagraph` (477 chars) is an exact substring of the old `Cards.StackUpgrade` detail;
  - the composed `OldResizePage` (592 chars) equals the old `PickMode.Resize` detail exactly.

  This is a text-level check, not a compiled one.

```diff
# SiegeControlsHelpWidget.cpp vs qa/TASK-1575.md bytes (scratch W.start.cpp, sha 419cefc8…3764)
diff --git a/C:/Users/wesel/AppData/Local/Temp/claude/C--GitProjects-GitHub-GitClaudeUnrealTesting-GitClaudeUnrealTest/35e683bf-936e-4e1a-93dd-6b0127c2c1b8/scratchpad/t1585/W.start.cpp b/Source/GitClaudeUnrealTest/Siegebound/SiegeControlsHelpWidget.cpp
index de25471..6a25ca9 100644
--- a/C:/Users/wesel/AppData/Local/Temp/claude/C--GitProjects-GitHub-GitClaudeUnrealTesting-GitClaudeUnrealTest/35e683bf-936e-4e1a-93dd-6b0127c2c1b8/scratchpad/t1585/W.start.cpp
+++ b/Source/GitClaudeUnrealTest/Siegebound/SiegeControlsHelpWidget.cpp
@@ -299,2 +299,8 @@ namespace
-//  ⛔ THE ORIGINAL ROW SET AND THE LANE COLUMN ARE 704's. Every lane assignment below
-//     traces to that file's §1.1 audit, and every one-liner is its §4 text VERBATIM.
+//  ⛔ THE ORIGINAL ROW SET IS 704's, AND SO ARE ITS LANES AND ONE-LINERS, WITH ONE EXCEPTION.
+//     For the original rows, every lane assignment below traces to that file's §1.1 audit and
+//     every one-liner is its §4 text verbatim, EXCEPT Cards.Discard's: TASK-821 rewrote that
+//     one-liner in place and moved its lane from PointerOnly to MappedAction (its own R-09
+//     block says why). Rows 25-27 (Cards.StackUpgrade, Cards.PlacementResize,
+//     Interface.MapMarks) carry TASK-823's own lanes and prose, authored at source.
+//     TASK-1585 (2026-09-28): scoped the way the .h's OneLine doc already is. This line used to
+//     claim 704's lanes and verbatim one-liners for every row, which the R-09 block refutes.
@@ -652 +658 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-				TEXT("press is set up, and a release only counts while the key is really held, so a double release cannot upset the ")
+				TEXT("press is set up, and each hold is let go only once, so a double release cannot upset the ")
@@ -663,0 +670,6 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
+			// TASK-1585 (2026-09-28, qa/TASK-1575.md N2, taken): "a release only counts while the key is
+			// really held" is now "each hold is let go only once". The guard is the game's hold record,
+			// not the physical key: ClearUICursorHold acts only `if (bUICursorHeld)` and clears it, so
+			// SetIgnoreLookInput(false) runs once per hold, and OnUICursorPressed sets the record only
+			// when it is not already set. ClearUICursorHold has four other callers that can end a hold
+			// while the key is still down, which is why "really held" overstated it. Same rule.
@@ -813 +825,5 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-			//     the SAME CardID -> CanScaleFootprint() -> gold -> Ready;
+			//     the SAME CardID -> CanStackHeight() -> gold -> Ready;
+			//     ⚠️ TASK-1585 (2026-09-28): gate (5) read "CanScaleFootprint()" here. STACK-§8
+			//     repointed it to ABuilding::CanStackHeight() on 2026-09-03 (the resolver's own
+			//     "(5) THE EXCLUSION" comment), and every shipped class answers that one true,
+			//     AClimbableTower included (STACK-§10) — so gate (5) turns nothing red today;
@@ -824,4 +840,11 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-			//   • the castle case = gate (2)'s `Cast<ABuilding>` (ACastle is class-disjoint);
-			//   • the HEIGHT series = ABuilding::StackHeightMultiplier, whose whole answer is
-			//     `FMath::Min(1 + Upgrades, Cap)` with Cap read off the CDO's
-			//     MaxStackHeightMultiplier ⇒ ⭐ ADDITIVE and SATURATING, ⛔ never doubling;
+			//   • the castle case = UpdatePlacementGhost's `Cast<ABuilding>(Hit.GetActor())`, which
+			//     hands gate (2) a null for the castle (ACastle is class-disjoint); TASK-1585
+			//     (2026-09-28): this read "gate (2)'s Cast", but the resolver takes an ABuilding
+			//     pointer and never casts;
+			//   • the HEIGHT series = ABuilding::StackHeightMultiplier(UpgradeCount, MaxMultiplier),
+			//     whose whole answer is `FMath::Min(1 + Upgrades, Cap)` with Cap the ceiling the
+			//     CALLER hands in: the building's OWN MaxStackHeightMultiplier, per class since
+			//     STACK-§10 cl. 2 (read through GetMaxStackHeightMultiplier() at the cap notice)
+			//     ⇒ ⭐ ADDITIVE and SATURATING, ⛔ never doubling. TASK-1585 (2026-09-28): this read
+			//     "Cap read off the CDO's MaxStackHeightMultiplier", which was the one-argument form
+			//     that read GetDefault<ABuilding>() until 2026-09-03 (the function's own comment);
@@ -831 +854,3 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-			//     StackHeightMultiplier(StackUpgradeCount);` with X and Y untouched (`J-4`);
+			//     StackHeightMultiplier(StackUpgradeCount, MaxStackHeightMultiplier);` with X and Y
+			//     untouched (`J-4`) — TASK-1585 (2026-09-28): the second argument, the instance's
+			//     own ceiling, was missing from this quotation;
@@ -835,2 +860,4 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-			//     UpgradesAfter) <= ABuilding::StackHeightMultiplier(UpgradesBefore))` ->
-			//     BroadcastRefusal(StackHeightCapNoticeText()) — at CONFIRM, ⛔ never per frame;
+			//     UpgradesAfter, TargetHeightCap) <= ABuilding::StackHeightMultiplier(UpgradesBefore,
+			//     TargetHeightCap))` -> BroadcastRefusal(StackHeightCapNoticeText()), with
+			//     `const int32 TargetHeightCap = Target->GetMaxStackHeightMultiplier();` — at CONFIRM,
+			//     ⛔ never per frame. TASK-1585 (2026-09-28): the quotation was one-argument;
@@ -842 +869,6 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-			//     StackNotStackableRefusalText() ("That building cannot be stacked");
+			//     StackNotStackableRefusalText() ("That building cannot be stacked"). ⚠️ TASK-1585
+			//     (2026-09-28): only the gold one is reachable. The second is the NotStackable
+			//     state's, whose doc in SiegePlayerController.h reads "NO SHIPPED CLASS PRODUCES
+			//     THIS STATE", so the page teaches the gold refusal alone. Both refusals `return`
+			//     before any gold moves and without ExitPlacementMode (the `if (!bPlacementValid)`
+			//     branch's "refuse, spend NOTHING, STAY in placement mode");
@@ -846,7 +878,14 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-			// ⛔⛔ THE EXCLUSION IS TAUGHT AS A BEHAVIOUR, ⛔ NEVER AS A NAME LIST — and that is the
-			// same law the CODE obeys: the shipped path asks ABuilding::CanScaleFootprint()
-			// (AClimbableTower overrides it false) and a CardID string compare there is an
-			// automatic QA fail. ⇒ naming a card in this prose would teach a rule the game does not
-			// have and would go stale the day a second climbable building ships. ⭐ The PLAYER'S
-			// reason is given ("it is the one you climb"); ⛔ no socket, ⛔ no rung plane and ⛔ no
-			// standoff appears anywhere on screen.
+			// ⛔⛔ THERE IS NO STACK EXCLUSION TO TEACH, AND THIS PAGE TEACHES NONE (TASK-1585,
+			// 2026-09-28). This paragraph used to say the exclusion was taught as a behaviour because
+			// "the shipped path asks ABuilding::CanScaleFootprint() (AClimbableTower overrides it
+			// false)". That stopped being true on 2026-09-03: STACK-§8 split the height question
+			// into its own virtual, ABuilding::CanStackHeight(), which ALL THREE stack gates ask
+			// (the resolver's gate (5), ConfirmStackUpgrade and ApplyStackUpgrade), and
+			// AClimbableTower answers it TRUE with a per-class ceiling (STACK-§10, `J-13`,
+			// TASK-944). CanScaleFootprint() is now the placement WHEEL's predicate only (the
+			// Cards.PlacementResize row). ⇒ the "REFUSES TO BE STACKED" paragraph, and the player's
+			// reason it gave ("it is the one you climb"), are DELETED by TASK-1585. The law that
+			// survives is the code's: a CardID string compare on the stack path is an automatic QA
+			// fail (STACK-§2), and ⛔ no building is named here. ⛔ Whether the page shows each
+			// building's height limit as a number is 🧑 Jonathan's open question
+			// Q-STACK-CAP-2026-09-28, owned by TASK-1576 — ⛔ this row adds no per-building limit.
@@ -885,8 +924,2 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-				TEXT("ONE KIND OF BUILDING REFUSES TO BE STACKED, AND IT IS THE ONE YOU CAN CLIMB. Its ladder is ")
-				TEXT("fixed to the building's exact shape, so stretching the building would take the ladder with it and ")
-				TEXT("the climb would stop working. The game therefore asks each building whether it may be scaled at ")
-				TEXT("all, rather than checking it against a list of names — so any climbable building added later is ")
-				TEXT("protected by the same one rule. Hovering one shows RED with \"That building cannot be ")
-				TEXT("stacked\".\n\n")
-				TEXT("A refusal of either kind costs nothing and leaves you in placement mode, so another building — ")
-				TEXT("or another patch of ground — still works. Back out entirely with {Cards.Cancel}.\n\n")
+				TEXT("Being refused for gold costs nothing and leaves you in placement mode. Back out entirely with ")
+				TEXT("{Cards.Cancel}.\n\n")
@@ -896 +929,2 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-			// of the mesh". The claim is unchanged, and no number is typed.
+			// of the mesh". The claim is unchanged, and no number is typed. (That sentence was in the
+			// paragraph TASK-1585 deleted; see below.)
@@ -903,0 +938,14 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
+			// ⭐ FIXED BY TASK-1585 (2026-09-28), which superseded TASK-973. The code was right and the
+			// text was wrong (`J-13`, TASK-944, STACK-§10), so the paragraph is DELETED, whole, and
+			// nothing replaces it: no building is singled out and no height limit is stated here (the
+			// cap's number, if any, is TASK-1576's on 🧑 Q-STACK-CAP-2026-09-28). The sentence after
+			// it read "A refusal of either kind costs nothing and leaves you in placement mode, so
+			// another building — or another patch of ground — still works." With the stack refusal
+			// gone, "either kind" pointed at nothing; and "another building or another patch of
+			// ground still works" is FALSE for the gold refusal that remains, because an upgrade of
+			// any other building of this card, and a placement on open ground, cost the same
+			// PendingCost. It now reads "Being refused for gold costs nothing and leaves you in
+			// placement mode", which is the `if (!bPlacementValid)` branch of TryConfirmPlacement
+			// (refuses before any gold moves and returns without ExitPlacementMode), and the same
+			// holds for ConfirmStackUpgrade's SpendGold refusal. The {Cards.Cancel} token is kept.
+			// ⛔ Pinned by test 19 (NoPageTeachesARefutedStackOrWheelRule) in the help test file.
@@ -949 +997,12 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-			//     return; }` and its comment ("the refusal already has a voice ... on the click").
+			//     return; }`. ⚠️ TASK-1585 (2026-09-28): this also cited that function's comment,
+			//     "the refusal already has a voice ... on the click", as the source of the page's
+			//     "that refusal speaks once, at the click". ⛔ Both are stale: the voice that comment
+			//     names is the RED ghost plus "That building cannot be stacked", which is the
+			//     NotStackable state that no shipped class produces since STACK-§8/§10 (2026-09-03),
+			//     and it was a STACK refusal, never a footprint one. A climbable card placed on open
+			//     ground is never refused at the click for its size; TryConfirmPlacement spawns it
+			//     at MakePlacementFootprintScale3D(PlacementFootprintScale), the floor the wheel
+			//     never moved.
+			//     ⇒ the clause is deleted from the prose below. The stale comment in
+			//     ApplyPlacementFootprintWheel is ⛔ outside TASK-1585's fence and is reported in
+			//     handoffs/TASK-1585-programmer.md, not edited.
@@ -981 +1040 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-				TEXT("on it rather than nagging you once per notch — that refusal speaks once, at the click.")));
+				TEXT("on it rather than nagging you once per notch.")));
@@ -989,0 +1049,4 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
+			// TASK-1585 (2026-09-28, the (F) stack-truth census): the last sentence ended "…rather than
+			// nagging you once per notch — that refusal speaks once, at the click." The trailing
+			// clause is deleted because no refusal speaks at the click for a climbable building's
+			// size (the citation block above says why). Everything before it is unchanged.
@@ -1301,2 +1364,8 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-			// R-17. Lane B. ⛔ The wheel is polled, not bound, and it is inert everywhere except
-			// inside a pick (SiegePlayerController.cpp:2787-2791, call site :599).
+			// R-17. Lane B. ⛔ The wheel is polled, not bound: ASiegePlayerController::ApplyGroupPickWheel,
+			// called only from PlayerTick's `GroupPickStage != EGroupPickStage::None` branch. It is
+			// ONE of the wheel's three consumers `MARK-§4` names (as amended by `STACK-§4`): this pick
+			// poll, the placement branch's footprint poll (ApplyPlacementFootprintWheel) and
+			// UWarMapWidget::NativeOnMouseWheel while the map is open; it stays inert everywhere else.
+			// TASK-1585 (2026-09-28): this read "it is inert everywhere except inside a pick
+			// (SiegePlayerController.cpp:2787-2791, call site :599)", which the war-map wheel
+			// (TASK-745) and the placement wheel (TASK-815) made false.
@@ -1311,2 +1380,3 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-			// polled-not-bound and inert outside a pick = :2787-2791, call site :599; the
-			// no-material degradation = :2808-2810.
+			// polled-not-bound = :2787-2791, call site :599 (⚠️ TASK-1585, 2026-09-28: this also cited
+			// those lines for "inert outside a pick", which is stale — see the R-17 note above and
+			// the TASK-1585 note below the string); the no-material degradation = :2808-2810.
@@ -1323,2 +1393,3 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-				TEXT("The game reads the wheel directly rather than through its control setup, and it was checked that no control anywhere else is set to the wheel — it is inert ")
-				TEXT("everywhere except inside a pick.\n\n")
+				TEXT("The game reads the wheel directly rather than through its control setup, and it was checked that no control anywhere is set to the wheel. ")
+				TEXT("During a pick the wheel resizes only the circle you are drawing. ")
+				TEXT("It has two other jobs elsewhere: while you are placing a building it can resize that building, and on the war map it resizes one of your own map circles.\n\n")
@@ -1335,0 +1407,19 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
+			// ⭐ FIXED BY TASK-1585 (2026-09-28). Old: "…and it was checked that no control anywhere else
+			// is set to the wheel — it is inert everywhere except inside a pick." New, every clause
+			// read at source:
+			//   • "no control anywhere is set to the wheel": `MARK-§4`'s quoted shipped clause, "The
+			//     wheel is verified globally unbound (no camera zoom exists)". "anywhere else" became
+			//     "anywhere" because none of the three wheel jobs is a bound control: two are polls
+			//     (WasInputKeyJustPressed on MouseScrollUp / MouseScrollDown) and one is a Slate event.
+			//   • "During a pick the wheel resizes only the circle you are drawing": ApplyGroupPickWheel
+			//     writes GroupPickRadius and the ACTIVE decal only; the pick branch `return`s before the
+			//     placement branch is reached; and the war map cannot be open during a pick
+			//     (BeginGroupPick is ignored while bWarMapOpen, and CanOpenWarMap refuses a pick).
+			//   • "while you are placing a building it can resize that building":
+			//     ApplyPlacementFootprintWheel, the placement branch's poll. "can", because it is
+			//     inert for a card whose class answers CanScaleFootprint() false (AClimbableTower).
+			//   • "on the war map it resizes one of your own map circles": UWarMapWidget::
+			//     NativeOnMouseWheel -> TryResizeMarkAtLocal, the Interface.MapMarks page's own words.
+			// ⛔ No number is typed. ⛔ Pinned by test 19 (NoPageTeachesARefutedStackOrWheelRule): the
+			// refuted phrase may appear on no page, and this page must name "placing a building" and
+			// "war map".
@@ -1421 +1511 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-				TEXT("Sending: type and press Enter — only a genuine Enter sends it; clicking away or clearing ")
+				TEXT("Sending: type and press Enter — only a genuine Enter sends it; clicking or tabbing away or clearing ")
@@ -1431,2 +1521,2 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-				TEXT("A close is not a cancel: closing the window cancels nothing — only the ")
-				TEXT("assistant itself may turn one into the other.\n\n")
+				TEXT("Closing the box does not cancel anything by itself — the assistant decides what a close means, ")
+				TEXT("and if it was waiting for you to confirm an order, it discards that order.\n\n")
@@ -1446,0 +1537,15 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
+			// TASK-1585 (2026-09-28, qa/TASK-1575.md N3 and N4, taken):
+			//   N3 — "clicking away" is now "clicking or tabbing away". USiegeAssistantConsoleWidget::
+			//   HandleTextCommitted returns on every commit method but ETextCommit::OnEnter, and
+			//   OnUserMovedFocus is ANY loss of focus, so a click was one case of it, not the rule.
+			//   N4 — "A close is not a cancel: closing the window cancels nothing — only the assistant
+			//   itself may turn one into the other." read, in plain words, against the
+			//   Interface.AssistantAccept block rendered under it ("or just close the box to discard
+			//   it"). It is now "Closing the box does not cancel anything by itself — the assistant
+			//   decides what a close means, and if it was waiting for you to confirm an order, it
+			//   discards that order." Source: SiegeAssistantConsoleWidget.h's "A CLOSE IS NOT A CANCEL
+			//   *IN THIS WIDGET*" (every close route reuses CloseConsole and none broadcasts a
+			//   cancellation; CloseConsole broadcasts OnConsoleOpenChanged(false)), then
+			//   USiegeAssistantComponent::HandleConsoleOpenChanged -> NotifyConsoleClosed, whose
+			//   AwaitConfirm case clears the preview and the pending order and returns to Idle. The
+			//   same rule, with the one case the Accept block names spelled out.
# Tests/SiegeControlsHelpTest.cpp vs qa/TASK-1575.md bytes (scratch T.start.cpp, sha 41e7d944…7304)
diff --git a/C:/Users/wesel/AppData/Local/Temp/claude/C--GitProjects-GitHub-GitClaudeUnrealTesting-GitClaudeUnrealTest/35e683bf-936e-4e1a-93dd-6b0127c2c1b8/scratchpad/t1585/T.start.cpp b/Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeControlsHelpTest.cpp
index 52cd50a..c265a03 100644
--- a/C:/Users/wesel/AppData/Local/Temp/claude/C--GitProjects-GitHub-GitClaudeUnrealTesting-GitClaudeUnrealTest/35e683bf-936e-4e1a-93dd-6b0127c2c1b8/scratchpad/t1585/T.start.cpp
+++ b/Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeControlsHelpTest.cpp
@@ -28,0 +29,5 @@
+ *  ⭐⭐ TEST 19 = TASK-1585: `HELP-§2` mechanism 4's pins for the two false rules the pages taught
+ *  until 2026-09-28 — that a building refuses to be stacked, and that the wheel is inert outside
+ *  a pick. Registry-wide negative scans, plus a positive pin that the pick-resize page names the
+ *  wheel's two other jobs.
+ *
@@ -1090 +1095,3 @@ bool FSiegeControlsHelpChipTest::RunTest(const FString& Parameters)
- *  human would otherwise have to eyeball across 24 strings: ⛔ no `file:line` citation, ⛔ no
+ *  human would otherwise have to eyeball across every row's string (twenty-seven rows as of
+ *  TASK-1585, 2026-09-28; it read "24 strings" until TASK-823 appended three, and the loop walks
+ *  GetActions(), so it covers whatever the registry holds): ⛔ no `file:line` citation, ⛔ no
@@ -2627,0 +2635,155 @@ bool FSiegeControlsHelpRelatedEdgeIntegrityTest::RunTest(const FString& Paramete
+// ════════════════════════════════════════════════════════════════════════════════════════
+//  TEST 19 — Siegebound.ControlsHelp.NoPageTeachesARefutedStackOrWheelRule   ⭐⭐
+// ════════════════════════════════════════════════════════════════════════════════════════
+
+/**
+ *  ⭐⭐ TASK-1585 (2026-09-28). `HELP-§2` MECHANISM 4 FOR THE TWO FALSE RULES THE HELP PAGES
+ *  TAUGHT, and each pin is shaped so the OLD text fails it and the new text passes (`TASK-974`
+ *  (3)'s test: a pin the old sentence would pass pins nothing).
+ *
+ *  (A) `Cards.StackUpgrade` taught that one kind of building "REFUSES TO BE STACKED" and that
+ *      hovering one shows RED with "That building cannot be stacked". FALSE at source:
+ *      AClimbableTower answers CanStackHeight() true (`STACK-§10`, `J-13`, TASK-944), the
+ *      NotStackable state's doc in SiegePlayerController.h reads "NO SHIPPED CLASS PRODUCES THIS
+ *      STATE", and SiegePlacementTest asserts an own-team, same-card, affordable climbable tower
+ *      resolves to Ready (blue). The fix was a DELETION ⇒ the pin is NEGATIVE: neither phrase
+ *      may appear in any row's one-liner or detail.
+ *  (B) `PickMode.Resize` taught that the wheel "is inert everywhere except inside a pick". FALSE
+ *      since the war-map wheel (TASK-745) and the placement wheel (TASK-815): `MARK-§4` names
+ *      three consumers. The fix was a REWRITE ⇒ the pin is BOTH: the refuted phrase may appear
+ *      on no page, AND the pick-resize page must name the wheel's other two jobs.
+ *
+ *  ⛔ REGISTRY-WIDE, ON EACH ROW'S OWN COMPOSED TEXT (ComposeOneLineForDisplay /
+ *  ComposeDetailForDisplay). That covers every RENDERING as well: a related block is some row's
+ *  own detail, read by ComposeDetailContent and never re-worded, so the stack row under
+ *  Cards.PlacementResize and the pick-resize row under the order pages are scanned here once,
+ *  at their source.
+ *
+ *  ⚠️ `HELP-§2` mechanism 4's own caveat binds this test: the pins make the prose HARD TO CHANGE,
+ *  ⛔ not TRUE. The truth came from reading the code (mechanism 3; the TASK-1585 notes beside both
+ *  strings in SiegeControlsHelpWidget.cpp carry the reads). This test only makes a later edit
+ *  that brings either rule back go red. Like every test in this file it paints nothing (`SC-§32`).
+ */
+IMPLEMENT_SIMPLE_AUTOMATION_TEST(
+	FSiegeControlsHelpRefutedRulesTest,
+	"Siegebound.ControlsHelp.NoPageTeachesARefutedStackOrWheelRule",
+	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
+
+bool FSiegeControlsHelpRefutedRulesTest::RunTest(const FString& Parameters)
+{
+	// ⭐ ONE PREDICATE for every scan below, fixtures included, so "the scan can go red" is a claim
+	// about the code that actually scans (`SC-§39`). Case-insensitive on purpose: the shipped false
+	// sentence was in capitals.
+	auto CarriesPhrase = [](const FString& Prose, const TCHAR* Phrase) -> bool
+	{
+		return Prose.Contains(Phrase, ESearchCase::IgnoreCase);
+	};
+
+	// ⭐ EACH PHRASE IS THE ONE THAT CARRIES ITS CLAIM (mechanism 4: "never a decorative
+	// fragment"), paired with why it is refuted, which a failure line prints.
+	struct FRefutedPhrase
+	{
+		const TCHAR* Phrase;
+		const TCHAR* Refutation;
+	};
+
+	const FRefutedPhrase RefutedPhrases[] =
+	{
+		{ TEXT("refuses to be stacked"), TEXT("TASK-1585 (A): no shipped building refuses the stack, STACK-§10") },
+		{ TEXT("cannot be stacked"),     TEXT("TASK-1585 (A): no shipped building refuses the stack, STACK-§10") },
+		{ TEXT("inert everywhere"),      TEXT("TASK-1585 (B): the wheel has three jobs, MARK-§4") }
+	};
+
+	// ⭐ (B)'s POSITIVE HALF: the wheel's two other jobs, which the pick-resize page must name —
+	// the placement footprint (ApplyPlacementFootprintWheel) and the war map's own circles
+	// (UWarMapWidget::NativeOnMouseWheel).
+	const TCHAR* OtherWheelJobs[] = { TEXT("placing a building"), TEXT("war map") };
+
+	// ── (a) FIXTURE SELF-CHECK: THE OLD TEXT FAILS EVERY PIN ─────────────────────────────
+	// ⭐ The refuted prose, copied literal for literal from qa/TASK-1575.md's bytes of
+	// SiegeControlsHelpWidget.cpp (sha256 419cefc8…3764): the whole stack paragraph TASK-1585
+	// deleted, and the whole pick-resize page as it stood before TASK-1585. ⛔ Without this, the
+	// zeros in (b) would be indistinguishable from a scanner that matches nothing, and a green (c)
+	// from a page that always carried the words. (Claims about the SCANNER against the OLD prose,
+	// ⛔ not about the registry.)
+	const FString OldStackParagraph(
+		TEXT("ONE KIND OF BUILDING REFUSES TO BE STACKED, AND IT IS THE ONE YOU CAN CLIMB. Its ladder is ")
+		TEXT("fixed to the building's exact shape, so stretching the building would take the ladder with it and ")
+		TEXT("the climb would stop working. The game therefore asks each building whether it may be scaled at ")
+		TEXT("all, rather than checking it against a list of names — so any climbable building added later is ")
+		TEXT("protected by the same one rule. Hovering one shows RED with \"That building cannot be ")
+		TEXT("stacked\".\n\n"));
+	const FString OldResizePage(
+		TEXT("One notch changes the active circle's radius by a set step, and the radius always stays between ")
+		TEXT("a set smallest and largest size. Each stage opens at its own default and resizing one ")
+		TEXT("circle never touches an earlier one. The circle on the ground resizes in place as you scroll.\n\n")
+		TEXT("The game reads the wheel directly rather than through its control setup, and it was checked that no control anywhere else is set to the wheel — it is inert ")
+		TEXT("everywhere except inside a pick.\n\n")
+		TEXT("If the circle's graphics are missing the radius still changes and the confirm still uses it — ")
+		TEXT("you just cannot see the circle."));
+
+	TestTrue(TEXT("FIXTURE SELF-CHECK (A): the scan finds 'refuses to be stacked' in the stack paragraph as it shipped"),
+		CarriesPhrase(OldStackParagraph, RefutedPhrases[0].Phrase));
+	TestTrue(TEXT("FIXTURE SELF-CHECK (A): ...and 'cannot be stacked'"),
+		CarriesPhrase(OldStackParagraph, RefutedPhrases[1].Phrase));
+	TestTrue(TEXT("FIXTURE SELF-CHECK (B): the scan finds 'inert everywhere' on the pick-resize page as it shipped"),
+		CarriesPhrase(OldResizePage, RefutedPhrases[2].Phrase));
+	for (const TCHAR* Job : OtherWheelJobs)
+	{
+		TestFalse(*FString::Printf(TEXT("FIXTURE SELF-CHECK (B): the pick-resize page as it shipped does NOT name '%s', so (c) would have been red on it"), Job),
+			CarriesPhrase(OldResizePage, Job));
+	}
+
+	// ── (b) ⛔ THE NEGATIVE PINS: NO ROW, ONE-LINER OR DETAIL, TEACHES A REFUTED RULE ──────
+	// ⛔ Every row from GetActions(), never a hand-typed list (`SC-§37`), so a refuted sentence
+	// pasted onto a row added later is caught too.
+	int32 RowsScanned = 0;
+	for (const FSiegeControlsHelpAction& Row : FSiegeControlsHelpRegistry::GetActions())
+	{
+		++RowsScanned;
+
+		const FString RowName = Row.ActionId.ToString();
+		const FString OneLine = FSiegeControlsHelpRegistry::ComposeOneLineForDisplay(Row).ToString();
+		const FString Detail  = FSiegeControlsHelpRegistry::ComposeDetailForDisplay(Row).ToString();
+
+		for (const FRefutedPhrase& Refuted : RefutedPhrases)
+		{
+			TestFalse(*FString::Printf(TEXT("Row '%s' one-liner teaches the refuted rule '%s' (%s)"),
+				*RowName, Refuted.Phrase, Refuted.Refutation), CarriesPhrase(OneLine, Refuted.Phrase));
+			TestFalse(*FString::Printf(TEXT("Row '%s' detail teaches the refuted rule '%s' (%s)"),
+				*RowName, Refuted.Phrase, Refuted.Refutation), CarriesPhrase(Detail, Refuted.Phrase));
+		}
+	}
+
+	// ⛔ THE VACUITY GUARD: a walk over zero rows is green and proves nothing.
+	TestTrue(TEXT("The walk scanned at least one registry row, so (b) is a measurement and not a loop over an empty list"),
+		RowsScanned > 0);
+
+	// ── (c) ⭐ THE POSITIVE PIN FOR (B): THE PICK-RESIZE PAGE NAMES THE WHEEL'S OTHER JOBS ───
+	const FSiegeControlsHelpAction* const ResizeRow =
+		FSiegeControlsHelpRegistry::FindAction(FName(TEXT("PickMode.Resize")));
+
+	if (!TestNotNull(TEXT("The pick-resize row is in the registry (it is (B)'s subject, and test 1 names it in RequiredIds)"), ResizeRow))
+	{
+		return false;
+	}
+
+	const FString ResizeDetail = FSiegeControlsHelpRegistry::ComposeDetailForDisplay(*ResizeRow).ToString();
+	int32 JobsNamed = 0;
+	for (const TCHAR* Job : OtherWheelJobs)
+	{
+		const bool bNamed = CarriesPhrase(ResizeDetail, Job);
+		if (bNamed)
+		{
+			++JobsNamed;
+		}
+		TestTrue(*FString::Printf(TEXT("PickMode.Resize names the wheel's other job '%s' (TASK-1585 (B): MARK-§4's three consumers)"), Job),
+			bNamed);
+	}
+
+	AddInfo(FString::Printf(TEXT("Scanned the one-liner and detail of %d registry row(s) for every refuted phrase; the pick-resize page names %d of the wheel's other jobs."),
+		RowsScanned, JobsNamed));
+
+	return true;
+}
+
# SiegePlayerController.cpp vs 9b82e8d
diff --git a/GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.cpp b/GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.cpp
index 94ad604..cc420f5 100644
--- a/GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.cpp
+++ b/GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.cpp
@@ -798,2 +798,7 @@ void ASiegePlayerController::PlayerTick(float DeltaTime)
-	// cancel + POLLED wheel resize (the CONVENTIONS wheel law: the wheel is
-	// inert everywhere but inside this branch) + per-frame trace + polled LMB
+	// cancel + POLLED wheel resize (the CONVENTIONS wheel law, MARK-§4 as amended
+	// by STACK-§4: "the wheel has exactly THREE consumers — (1) the controller's
+	// group-pick poll · (2) UWarMapWidget while the map is open and the cursor is
+	// over it · (3) the controller's PLACEMENT-mode footprint poll
+	// (ApplyPlacementFootprintWheel, STACK-§4). It stays inert everywhere else";
+	// this branch is consumer 1. TASK-1585, 2026-09-28: this read "the wheel is
+	// inert everywhere but inside this branch") + per-frame trace + polled LMB
```
