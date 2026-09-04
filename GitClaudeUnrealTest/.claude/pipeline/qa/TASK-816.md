# QA Report — TASK-816

**Gate over five diffs.** `SC-§29` coverage ledger below. `SC-§27`: each diff gets its own verdict.

Verdict: **PASS** — 0 BLOCKERS, 6 WARN, 4 NIT.

| # | task | subject | verdict | blockers |
|---|---|---|---|---|
| 1 | **TASK-812** | the two series + the scale-exclusion predicate | ✅ **PASS** | 0 |
| 2 | **TASK-735** | footprint-aware placement | ✅ **PASS** | 0 |
| 3 | **TASK-813** | the blue upgrade state + the confirm path | ✅ **PASS** | 0 |
| 4 | **TASK-815** | the placement footprint wheel | ✅ **PASS** | 0 |
| 5 | **TASK-823** | the three controls-menu rows | ✅ **PASS** | 0 |

⛔ **`TASK-852` is NOT in this ledger** (row (f) struck, re-homed to `TASK-856`). I did not read it and I make no claim about it.

⚠️ **My instruments, declared up front (`SC-§40`): I have Read/Grep/Glob only — ⛔ no Bash and ⛔ no git.** Every claim below says how it was measured. The three claims that require a *diff* rather than a *file* are named in W-5 and handed to build-master, which has git. Twice during this review `Grep -A` rendered a `//` as `\` (at `SiegePlayerController.cpp:4586`, `:5075`, `:5108` and `SiegeControlsHelpWidget.cpp:1105`); each was re-read with `Read` and each was a **display artefact, not a defect**. Recorded because a reviewer trusting that output would have filed four false blockers.

---

## ⛔ ROW (g1) — THE FINDING, RE-MEASURED AT SOURCE WITH MY OWN EYES

> `SC-§40` cl. 3: the manager re-scoped a task on a relayed measurement, so the gate re-measures it.

✅ **`TASK-823`'s finding is CONFIRMED. The registry-wide `RelatedActionIds` walk ALREADY SHIPS.**

**How I measured it:** located by symbol (`SC-§38`), not by line — `grep` for `^IMPLEMENT_SIMPLE_AUTOMATION_TEST` in `Tests/SiegeControlsHelpTest.cpp` gave 16 tests; test 9 is `FSiegeControlsHelpAuthoredDetailTest` / `"Siegebound.ControlsHelp.EveryRowHasAuthoredDetail"`. I then read its body in full (`:1085-1136`). The walk is at `:1126-1132`:

```cpp
for (const FSiegeControlsHelpAction& Row : FSiegeControlsHelpRegistry::GetActions())   // :1098
{
    ...
    for (const FName RelatedId : Row.RelatedActionIds)                                  // :1126
    {
        TestNotNull(..., FSiegeControlsHelpRegistry::FindAction(RelatedId));            // :1128
        TestNotEqual(..., RelatedId, Row.ActionId);                                     // :1130
    }
}
```

**Three properties checked, not assumed:**
1. ⭐ **It covers EVERY row** — the outer loop is `GetActions()`, the live registry, not a subset. (Contrast `RequiredIds[]` in test 1, which *is* a subset — that is the array the law named, and naming the wrong test is precisely how "no test does it" got written.)
2. ⭐ **It is DERIVED, not transcribed** — the ids come from `Row.RelatedActionIds` (the live field) and resolve through `FindAction` against the registry. There is no hand-typed edge list anywhere in it.
3. ⭐ **It also forbids self-reference**, which `TASK-852` §(2) asked for.

⇒ **`HELP-§7`'s correction is right. `TASK-852`'s re-scope is right. A second walk would be a duplicate and is correctly an automatic fail.** What it genuinely lacks is the **negative control** — nothing in test 9 proves the walk can go red — and that remains `TASK-852`'s. (⭐ Note for `TASK-852`: `TASK-823` already shipped the precedent it can copy, at `SiegeControlsHelpTest.cpp:2061` — `TestNull(FindAction(FName("Cards.NoSuchUpgradeRow")))`.)

⚖️ **Two independent readers concluded this did not exist because it lives in a test named for something else.** I add a third data point in the same direction: I found it in ten seconds *only because I was told the symbol*. `HELP-§7`'s closing clause — a test's name is its discoverability surface — is the correct generalisation.

## ⛔ ROW (g2) — THE STALE COUNTS

✅ **27 is correct.** Measured by counting `AddRow(` call sites in `SiegeControlsHelpWidget.cpp`: **6 Hero + 6 Cards + 5 Orders + 3 PickMode + 7 Interface = 27.** `24 + 3 = 27` reconciles exactly, and the three appends are `Cards.StackUpgrade` (`:665`), `Cards.PlacementResize` (`:752`), `Interface.MapMarks` (`:1394`).

✅ **The two source corrections are comment-only.** Read at `SiegeControlsHelpWidget.cpp:212-263` (the registry banner — the count is *removed* and replaced with a note naming `GetActions().Num()`, and *"THE ROW SET"* is reworded to *"THE ORIGINAL ROW SET"*) and the `GetActions()` docstring in the header. ⛔ No executable line in either file was changed by that correction.

✅ **The law's fix is right, not merely present:** `CARDBAR-§9` and `STACK-§4` now carry `GetActions().Num()` with `27` surviving only as a dated observation. That is the correct shape — the previous text would have rotted again on the next append.

## ⛔ ROW (g3) — RULING ON H-1 (the marks row's lane). **UPHELD.**

⚖️ **`Interface.MapMarks` stays `ESiegeInputLane::RawNonLetter`. `PointerOnly` would be a defect, and I verified that at source rather than accepting the argument.**

| what I read | where | what it proves |
|---|---|---|
| `case ESiegeInputLane::PointerOnly: return DisplayKeys;` — returning an **empty** array | `ResolveRowDisplayKeys`, `SiegeControlsHelpWidget.cpp:1495-1497` | on Lane D the row has no keys to render |
| `if (Row.bPointerOnly \|\| Row.Lane == PointerOnly) return PointerChip;` — **before** `DisplayKeys` is ever inspected | `ComposeKeyChipLabel`, `:1567-1570` | ⭐ the chip is the pointer affordance **regardless of the keys handed in** |
| `case RawNonLetter: DisplayKeys = Row.QwertyReferenceKeys; return;` — **zero** `GetPositionalKey` calls | `:1499-1506` | Lane B *is* "label the reference keys verbatim" |

⇒ On `PointerOnly` the wheel would be **structurally unable to appear on the row** — the one thing the row was boarded for. The lane encodes the **translation rule**, and Lane B's rule is exactly the one this row needs; the identity is provable because the translation table is A–Z only. **The programmer's argument is correct in both halves.** The cost of disagreeing would be the wheel vanishing from the list, and I do not disagree.

⚠️ See **N-1** for the one thing the ruling leaves slightly untidy.

## ⛔ ROW (g4) — THE DECLARED GREP HIT

✅ **Confirmed exactly as declared.** `GetPositionalKey` in `SiegeControlsHelpWidget.cpp`: **2 calls** — `:1524` (the Lane-C derive arm) and `:1560` (the Lane-A fallback), **both pre-existing**; plus **1 new occurrence at `:1355`, which is prose inside the comment block arguing H-1.** ⇒ **zero calls added to the label path.** The criterion holds as written.

---

## Findings

### BLOCKER
**None.**

### WARN

- **[WARN] W-1 — `SiegeControlsHelpWidget.cpp:1268` vs `:1410` — TWO SHIPPED HELP PAGES NOW CONTRADICT EACH OTHER ABOUT THE SAME GESTURE IN THE SAME MODE, AND ONE OF THE TWO SENTENCES IS FALSE.** (TASK-823)
  - `Interface.WarMap`'s detail says the map is *"closed by right-click or Escape, polled every frame"*.
  - `Interface.MapMarks`'s detail (new) says *"A right-click that hits no circle does nothing at all"*.
  - **What I measured:** `WarMapWidget.cpp:2629-2635` — while `bMapOpen`, **every** right-click takes the mark arm, calls `TryDeleteMarkAtLocal` (return value deliberately discarded) and returns `FReply::Handled()`. That is consistent with the widget consuming the gesture, which would mean the `PlayerTick` poll at `SiegePlayerController.cpp:746` never fires and **`Interface.WarMap`'s sentence is the false one**. ⛔ **I could not settle it by reading** — it turns on Slate event routing under the live input mode, which is an observation, not a file.
  - ⚖️ **Why this is a WARN and not a blocker on `TASK-823`:** the contradicting sentence is on a **shipped row `TASK-823` was fenced out of editing** (its spec (6) scope fence, and `HELP-§7`'s ownership rule), and the cross-consumer right-click reconciliation is **already recorded as owed law** — `STACK-§4`: *"`CARDBAR-§8` adds the same obligation for a second gesture … that half is STILL OWED and is NOT satisfied by the above."* Failing a task for not doing what its fence forbids would be a false fail.
  - **Suggested fix — two actions, neither in this diff:** (1) `TASK-814` gets a PIE row: *open the war map, right-click empty ground — does the map close?* (2) the answer goes to the manager, who boards the one-sentence repair on whichever page is wrong. ⛔ Do not let Lane B's commit be called "help complete" until that row is observed — `HELP-§2`: a wrong control is worse than no help screen.

- **[WARN] W-2 — gate row (e), the right-click half — NOT SATISFIED, and by law not `TASK-823`'s to satisfy.** Row (e) asks that the *"FIVE right-click meanings"* be told apart. Measured against `CARDBAR-§8`'s registry (consumers 1–4 + 4b; row 5 struck when Jonathan cut right-click discard): the help screen has rows for the group-pick cancel (`PickMode.Cancel`), the placement cancel (`Cards.Cancel`), the map close (`Interface.WarMap`) and now the mark delete (`Interface.MapMarks`) — but **spell-targeting cancel has no row at all**, and none of the four states its exclusivity against the others. `TASK-823` *did* satisfy its own share: `Interface.MapMarks` opens with *"all four work on the war map and nowhere else"* and scopes its right-click to *"A CIRCLE"*. **Suggested fix:** the owed half stays owed and belongs on the board next to `CARDBAR-§9`'s *"`Cards.Play` and `Cards.Cancel` detail pages must disambiguate right-click"* — which is Lane A's row, not this gate's diff.

- **[WARN] W-3 — `Building.h:437-438` / `Building.cpp:437-442` — the STACKED HEIGHT IS NOT REPLICATED, and the M8 declaration does not say so.** (TASK-812)
  `StackUpgradeCount` is `UPROPERTY(VisibleInstanceOnly, **Transient**)` with no `Replicated` specifier, and `ApplyStackUpgrade` writes `VisualMesh->SetRelativeScale3D(...)` on a component that is not replicated. `STACK-§7`'s M8 paragraph correctly says the **HP** result rides the shipped `OnHPChanged` push — but it is silent on the **height**, which is the visible half. In a listen-server build a remote client would see an un-stacked tower carrying stacked HP. **Suggested fix:** ⛔ no code change now — M8 is deferred project-wide and the authority guard is correct. Add one line to the M8 ledger naming *"stacked height is server-local until M8"* so the next reader does not have to re-derive it. (`SC-§38` cl. 4: a law that describes behaviour rots silently.)

- **[WARN] W-4 — `SiegePlayerController.cpp:2676` (`IsGroundSlopePlaceable`) — TASK-735's declared survivor (b) gets WORSE with the wheel, and it should now be BOARDED rather than left as prose.** (TASK-735 §5(b), the ruling it explicitly asked for)
  The ghost is now *known* to be wide **and player-adjustable up to ×1.5**, while the slope is still a single straight-down trace at the cursor. A structure whose centre sits on a flat crown can overhang a 40° flank and pass — and the wheel widens the overhang by up to half again. **RULED: board it as a follow-on** (four corner traces at `FootprintRadius`, which now exists as a value one line above). ⛔ Not this wave's, ⛔ not a blocker: it fails open in the same direction it always has, and nothing that passes today starts failing.

- **[WARN] W-5 — THREE BYTE-IDENTITY CLAIMS ARE OUTSIDE MY INSTRUMENTS. Hand them to build-master, which has git.**
  I cannot run `git diff`. The following are **claimed** and **not measured by me**: (a) `Cards.Discard`/`Cards.Play`/`Cards.Cancel` byte-identical so `TASK-821` rebases clean; (b) `TASK-819`'s discard-all block untouched; (c) no shipped gate line in `UpdatePlacementGhost` re-indented.
  **What I *did* measure, and it is real corroborating evidence, not a substitute:** I read all **16** `RelatedActionIds` blocks in the registry and **no existing row names any of the three new ids** ⇒ `HELP-§7`'s ownership rule is honoured and no shipped row's edges were re-pointed. The shipped card rows' content is coherent with `TASK-821`'s recorded end state (`:450`, `:500-502`, `:567`, `:590`). The gate chain at `:2670-2700` is intact and the `TASK-813` block sits **after** `bPlacementValid = bValid;`, purely additive, exactly as declared.
  **Suggested fix:** `TASK-814` runs `git diff -- Source/.../SiegeControlsHelpWidget.cpp` and confirms the three card-row hunks are absent before committing.

- **[WARN] W-6 — `SiegePlayerController.cpp:2363-2368` — a `BP_Building_*` construction script could override the spawn scale.** (TASK-815 §9.3, declared not fixed)
  `FinishSpawning(SpawnTransform)` applies the transform and *then* runs the construction script; a BP that sets its root's relative scale wins. This is the same exposure the shipped *location* has carried since `TASK-027`, and checking `.uasset`s is outside a code fence. **Correctly declared as a risk, not a defect.** **Suggested fix:** it is already covered by `TASK-814` §(3) — *"the wheel visibly resizes the ghost and the SPAWNED building matches"*. Keep that row; it is the instrument for this exact risk.

### NIT

- **[NIT] N-1 — `SiegeControlsHelpWidget.h:83-89` — `ESiegeInputLane::RawNonLetter`'s docstring now under-describes its own membership.** It reads *"A RAW-polled key that is NOT a letter — a mouse button, Escape, or the wheel"*, and `Interface.MapMarks` joins it with gestures that arrive as **Slate events**. The ruling (g3) is right; the docstring is now the only place a reader could be misled. **Suggested fix:** one additive clause — *"…or a Slate-delivered mouse gesture: the lane selects the VERBATIM-label algorithm, not the delivery route."* ⛔ Not `TASK-823`'s to write (the enum is a shipped surface outside its fence, and its own comment at `:1349-1364` documents the judgment in place). For whoever next edits the enum.

- **[NIT] N-2 — `SC-§38` FIRED AGAIN, ON THIS WAVE'S OWN HANDOFF.** `handoffs/TASK-815-programmer.md` §0 states `DiscardAllCost = 20;` is at **`SiegePlayerController.h:1496`**. **Measured: it is at `:1586`** — a 90-line drift from parallel lanes. ⛔ **Nothing rode the bad number** (nobody edited on it) and the **symbol is intact and unique**: `int32 DiscardAllCost = 20;` occurs exactly once. **Suggested fix:** `TASK-844`'s dispatch must carry the **symbol** (`DiscardAllCost = 20;` → `SiegeCardEconomy::DiscardAllGold`), with the line as a hint only. `SC-§38` cl. 5 exempts handoffs — but cl. 6 binds the *prompt* that relays one.

- **[NIT] N-3 — `SiegePlayerController.cpp:1774` — a building card now resolves its BP class TWICE per placement session** (once in `EnterPlacementMode`, once in `TryConfirmPlacement`). The second `LoadSynchronous` hits the already-loaded asset, so the cost is a soft-path lookup, not a load. ⛔ No change wanted; recorded so nobody "optimises" it by caching the class on a member and creating a second source of truth.

- **[NIT] N-4 — `SiegePlayerController.cpp:1762` — the seed `PlacementFootprintScale = PlacementFootprintMin;` takes the tunable raw**, i.e. it is the one read of the scale that is not clamped. Harmless and self-consistent: `StepPlacementFootprintScale` collapses `SafeMin`/`SafeMax` to the same misconfigured `Min`, so ghost and spawn still agree — and agreement is the property that matters. ⛔ No change; see ruling R-4c below.

---

## ⚖️ THE RULINGS THE HANDOFFS ESCALATED — all answered, none deferred

| # | task | question | ⚖️ ruled |
|---|---|---|---|
| **R-1a** | 812 §4/§9.1 | is the **height half** of `ApplyStackUpgrade` in scope? | ✅ **IN. Keep it.** `MaxHP`/`CurrentHP`/`StackUpgradeCount` are private and `VisualMesh` is protected; splitting would put the additive series in two places and the "recompute from the baseline" property — the one thing that lets the cap land *exactly* — would then depend on the controller getting it right too. One upgrade, one place. |
| **R-1b** | 812 §9.2 | the **CDO read** inside the pinned pure statics | ✅ **ACCEPTED.** `STACK-§7` pins the seam at one parameter *and* pins the tunables `EditDefaultsOnly`; the conjunction leaves no other implementation. Costs no world and no instance, so the seam stays headless. The game-wide-not-per-card consequence is written at `Building.h:339-343`, which is where it belongs. |
| **R-1c** | 812 §9.3 | is `AuthoredHeightScaleZ` a "second copy of the height scale"? | ✅ **NO.** It is the *baseline the series is defined against*, not a multiplier — an additive series has no transform expression without it. Captured lazily at `StackUpgradeCount == 0` **before** the increment (`Building.cpp:420-427`), which is what makes it independent of spawn ordering. |
| **R-1d** | 812 §9.4 | `MaxHP ×= StackHealthMultiplier(1)` vs `×= StackHealthStep` | ✅ **KEEP THE SERIES.** Identical value, one expression — so a HUD preview and the granted number cannot disagree. |
| **R-2a** | 813 §9.1 | the upgrade block **overrides** the gate verdict rather than wrapping it | ✅ **CORRECT AS BUILT.** The five gates answer *"may a NEW building stand here"*; a blue click places nothing, and hovering a building fails §3.5 clearance **by construction** (distance ≈ 0), so composing could only ever refuse the state being added. The cost — five side-effect-free gates running and being discarded — buys a zero-line diff for `TASK-815`, and `TASK-815` did in fact land on top of it without touching a gate line. |
| **R-2b** | 813 §9.2 | does anything else read `bPlacementValid` and assume *"a new building may stand here"*? | ✅ **NO — RE-MEASURED INDEPENDENTLY.** Project-wide grep: `bPlacementValid` has exactly **two readers** — `TryConfirmPlacement`'s `if (!bPlacementValid)` (`:2198`) and the colour ternary (`:2811`). Both are correct under the coupling. Writers: `:1740`, `:1817`, `:2700`, `:2752`, `:2763`. ⛔ No third consumer. |
| **R-2c** | 813 §9.4 | the **`J-5`** reading — does blue require gold? | ✅ **YES, AS BUILT.** `STACK-§5` `J-5`'s cell opens *"NO"* and then says *"Can't afford ⇒ ⛔ RED + the shipped 'Not enough gold' line"*. The second sentence is operative and unambiguous, and the row's own cited standard settles it: *"what the player sees is what the click does."* A blue that refuses would be the first place that stopped being true. |
| **R-2d** | 813 §9.7 | the **public nested** `EPlacementUpgradeState` | ✅ **ACCEPTED.** It is nested on the controller (no namespace pollution) and the resolver *is* the feature — a private return type makes it uncallable from a test, which would delete the five state assertions in tests 11–14. |
| **R-3** | 735 §9.7 | board the single-trace slope gate? | ⚖️ **YES — see W-4.** |
| **R-4a** | 815 §9.1 | `EnterPlacementMode` now calls `ResolveCardActorClass` | ✅ **ACCEPTED.** Once per session, building cards only (`bPendingIsBuilding` short-circuits at `:1773`), on a soft path the confirm resolves anyway. The one visible consequence is **one extra `Warning` line for a missing BP** (`ResolveCardActorClass:4627`) in an already-broken case; placement still enters, the confirm still refuses and exits. ⭐ It also moves a possible first-load hitch from the *confirm click* to the *ghost spawn*, which is the better of the two places for it. Both rejected alternatives (a second soft-path source of truth; a per-tick CDO resolve) were correctly rejected. |
| **R-4b** | 815 §9.2 | the **one widened shipped line** | ✅ **ACCEPTED.** Read at `:2361-2362`: `const FTransform SpawnTransform(FRotator::ZeroRotator, PlacementLocation, MakePlacementFootprintScale3D(PlacementFootprintScale));` — a third argument on an existing statement. Spec (4) is unsatisfiable without it. Nothing around it is reordered or re-indented. |
| **R-4c** | 815 §9.6 | the scale is **not re-clamped at confirm** | ✅ **ACCEPTED — do not add a second clamp.** One writer (`ApplyPlacementFootprintWheel`, always through `StepPlacementFootprintScale`), reset to `PlacementFootprintMin` in **both** `EnterPlacementMode` (`:1762`) and `ExitPlacementMode` (`:1832`), and `MakePlacementFootprintScale3D` sanitises non-finite/non-positive at **both** consumers (`:5278`). A second clamp would be a second copy of the range — the `HIGH-§1` booby trap. See N-4 for the one edge I checked and cleared. |
| **R-5** | 823 H-1 | the marks row's lane | ✅ **UPHELD — see row (g3).** |

---

## Gate rows (a)–(e) — each measured, with its instrument named

**(a) ⛔ NO `CardID == "WatchTower"` COMPARE ANYWHERE IN THE PLACEMENT OR STACK PATH.** ✅ **PASS.**
Grep for `WatchTower` across `SiegePlayerController.{h,cpp}`, `Building.{h,cpp}`, `SiegeControlsHelpWidget.{h,cpp}` returned **18 occurrences — every one inside a comment** (`//` or ` * `). ⭐ **Positive control (`SC-§39`): the instrument found 18, so it is not blind — the zero on code lines is separation, not an unreadable probe.** The exclusion is `virtual bool ABuilding::CanScaleFootprint() const` (`Building.h:233`) overridden `false` on `AClimbableTower` (`ClimbableTower.h:503`), asked at **three** independent sites: `ResolvePlacementUpgradeState` gate (5) (`:5186`), `ConfirmStackUpgrade` (`:2494`), and `ApplyStackUpgrade` itself (`Building.cpp:411`) — plus `CanCardActorScaleFootprint` for the wheel (`:5320`), which asks the **class CDO** via `Cast<ABuilding>(GetDefaultObject())` and ⛔ never the templated form. ⛔ Nothing else.

**(b) ⛔ NO `TOWER-§8.*` SYMBOL IS TOUCHED — verified in the diff, not in the prose.** ✅ **PASS.**
Grep for `LadderFoot|LadderTop|RungPlane|Standoff|A_SiegeBiped_Climb|SM_WatchTower|ClimbLine` across the five diffs' files: **10 occurrences, all comments.** ⛔ Zero code lines. `ClimbableTower.h` gained exactly one line of code — the `false` override at `:503`; `ClimbableTower.cpp` is not in any handoff's file list and shows no new symbol. ⭐ `STACK-§2`'s argument holds *because the geometry never changes*, and that is now structural: the predicate is a virtual on the base, so the next climbable building inherits the protection.

**(c) ⛔ `SC-§37` ON THE SERIES TESTS — reject any transcription.** ✅ **PASS, and this is the strongest part of the wave.**
`Tests/SiegeBuildingStackTest.cpp` test 2 (`TheHeightCapIsReachedExactlyAndTheDoublingReadingCanNeverReachIt`, `:428-521`): the cap is read off `MaxStackHeightMultiplier` **by reflection** (`:439`), every index is derived from it, and ⛔ **`2, 3, 4, 5` and `1.5, 2.25, 3.375` appear nowhere as expectations.** Specifically —
- ⭐ **The cap is reached EXACTLY** at `n = Cap-1` with tolerance **`Exact`** (`:477-478`), the term below it is **strictly** below (`:474`), and it saturates for six terms beyond (`:480-484`). That is the property the doubling reading cannot satisfy.
- ⭐ **The falsifier is independently constructed** — a separate `2ⁿ` walked 24 terms, asserting no term equals the cap and that it steps *over* it (`:493-513`), with a self-check that the doubling series is live (`2² = 4`, `:507`).
- ⭐ **The blind spot is asserted rather than hidden:** `:454-457` proves the two readings *agree* at n=0 and n=1, so "every discriminating row is n ≥ 2" is an auditable fact rather than a comment.
- ⚠️ **And the power-of-two retune is guarded with an `AddWarning` rather than a silent red** (`:517`) — correct: a legitimate retune must not read as a defect.
- The shipped arithmetic backs it: `FMath::Min(1 + FMath::Clamp(n, 0, Cap), Cap)` (`Building.cpp:322-334`) — integer, then **one** widening. `INT32_MAX` cannot overflow the `1 +` because the clamp runs first. Health is repeated `*=` (`:371-379`) with a NaN-safe `!(Step >= 1.f)` guard and ⛔ no ceiling term.

**(d) ⭐ `MARK-§4`'s AMENDMENT HONOURED.** ✅ **PASS.**
`ApplyPlacementFootprintWheel()` occurs on exactly **2** code lines — its definition (`:2816`) and its **one** call (`:785`), inside `PlayerTick`'s placement branch, reached only past `if (!bInPlacementMode) { return; }` at `:757`. ⛔ No new `InputAction` (the body has zero `BindAction`/`UInputAction`); it polls `WasInputKeyJustPressed(EKeys::MouseScrollUp/Down)`, `ApplyGroupPickWheel`'s shape exactly. ⛔ `GroupRadiusWheelStep`/`Min`/`Max` reused **zero** times, and test 27(f) pairs each zero with a **positive control** proving the scanner finds all three elsewhere in the same file. ✅ **No guard was added for a state that cannot exist**, and I confirmed the state genuinely cannot: `:696`, `:724`, `:754` are unconditional `return`s above the placement gate.

**(e) `TASK-823`'s three rows.** ✅ **PASS** (with W-1/W-2 on the right-click half).
Three rows exist, all non-blank, all with an authored `Detail` — ⛔ none renders `(undocumented — TODO)` (test 15(b) asserts both halves). ⭐ **Zero digit characters across all six strings — I verified this by reading all six in full, not by trusting the test**; every tunable is NAMED (`MaxStackHeightMultiplier`, `StackHealthStep`, `PlacementFootprintWheelStep`/`Min`/`Max`, `MaxMapMarks`) and the upgrade price is *"the card's own cost"*. ⭐ **The three wheel meanings are told apart** — three ids, three headlines, three one-liners, three detail pages, **three different categories**, each stating *when* its gesture applies before what it does; test 16 asserts that as a property by **scanning** the registry for wheel keys, with `MARK-§4`'s ceiling of three enforced. ⛔ **No card row was touched** (see W-5 for the limit of my instrument). ⛔ **No `GetPositionalKey` on the label path** — see row (g4). ✅ **The `WatchTower` is never named in player prose:** the exclusion is taught as *"the game asks each building whether it may be scaled at all, rather than checking it against a list of names"* with the player's reason (*"it is the one you climb"*), and ⛔ no socket, rung plane or standoff reaches the screen.

## "Also gate" rows

| row | verdict | measured at |
|---|---|---|
| **`J-5`** — blue never promises a refused click | ✅ | `ResolvePlacementUpgradeState` gate (6), `:5195`; `Unaffordable` → RED via `EPlacementInvalidReason::Upgrade`, `:2244-2251` |
| **`J-10`** — adds HP, does not heal | ✅ | `Building.cpp:453-455`: `MaxHP = OldMaxHP * StackHealthMultiplier(1); CurrentHP += (MaxHP - OldMaxHP);` — the delta, never the new max. Test 7 asserts the missing HP is *identical* before and after, on a damaged instance, at n=1 **and** n=2 |
| **the new `EPlacementInvalidReason` value is NEW, not `Clearance` reused** | ✅ | `SiegePlayerController.h:2159` — `Upgrade`, appended **last**, after `Units`. `Clearance` untouched. Enum still `private` |
| **`TASK-735`'s footprint reads SCALED bounds (`STACK-§6`)** | ✅ | `TryGetPlacementFootprintRadius`, `:5348` — `GhostMesh->CalcBounds(GhostMesh->GetComponentTransform())`. ⛔ **Not** `UStaticMesh::GetBounds()`. This is the load-bearing line of the whole wave and it is correct |
| **the M8 declaration (`StackUpgradeCount` server-set)** | ✅ (see W-3) | `Building.cpp:390-396` — `!HasAuthority()` refuses with a `Warning`; ⛔ deliberately **not** a `UFUNCTION`, so no Blueprint entry point exists. Belt *and* braces |
| **`max` not sum (735's whole non-regression argument)** | ✅ | `EffectiveBuildingClearance`, `:5090` — `FMath::Max(SafeBase, SafeRadius)`. Every footprint ≤ `BuildingClearance` returns `BuildingClearance` **identically** |
| **the `Units` gate is evaluated LAST** | ✅ | `:2691`, after slope/obstacle/clearance ⇒ first-failing-rule-wins preserved; ⛔ no shipped refusal can change which message it shows |
| **degrade asymmetry** | ✅ | building clearance takes radius `0` ⇒ `max(200,0)=200`, the shipped rule byte-for-byte; the unit gate is **skipped** via `bFootprintKnown` (`:2668`, `:2691`) |
| **`bWarnedNoFootprintBounds` cannot go stale** | ✅ | set only inside the degrade arm (`:5363`), cleared in `EnterPlacementMode` (`:1747`). A stale `true` can persist only while **not** in placement mode, where it is unreachable |

## ⭐ THE TWO ORDERING FACTS — verified, and verified to DISCRIMINATE

`TASK-815`'s strongest claim, and the dispatch asked whether the assertions actually bite. **They do.**

`CheckPrecedes` (`Tests/SiegePlacementTest.cpp:1694-1701`) requires `EarlierAt != INDEX_NONE && LaterAt != INDEX_NONE && EarlierAt < LaterAt`. ⇒ **inverting the pair makes it red, and deleting either token makes it red.** It is not a tautology. It runs over `CodeLinesOnly`, so the shipped comments that name every symbol it looks for cannot satisfy it.

| fact | assertion | ⭐ **my own independent re-measurement** |
|---|---|---|
| the wheel polls **before** `UpdatePlacementGhost` | test 27, `:2794` | `ApplyPlacementFootprintWheel();` at **`:785`** < `UpdatePlacementGhost();` at **`:788`** ✅ |
| the ghost is **sized** before its footprint is **measured** | test 28, `:2909` | `SetActorScale3D(MakePlacementFootprintScale3D(...))` at **`:2605`** < `TryGetPlacementFootprintRadius(...)` at **`:2668`** ✅ |
| the two wheel polls sit on **opposite sides** of the placement gate | test 27, `:2786-2787` | `ApplyGroupPickWheel();` **`:683`** < `if (!bInPlacementMode)` **`:757`** < `ApplyPlacementFootprintWheel();` **`:785`** ✅ |

⭐ **And the extraction fails SAFE:** `ExtractControllerFunctionBody` (`:1640-1651`) errors by name if the signature is gone, and its self-checks (27(b), 28's two) prove the extracted body is the right function before anything is counted in it. A stale or empty probe reports red, not green.

**The structural-agreement claim, re-measured rather than accepted:** `MakePlacementFootprintScale3D(` occurs on exactly **3** code lines in `SiegePlayerController.cpp` — the definition (`:5273`) and its two consumers (`:2362` spawn, `:2605` ghost). The two other occurrences of the name (`:2360`, `:2589`) are comments. ⛔ **Zero** `FVector(PlacementFootprintScale`. **The programmer's count of exactly three is correct.** ⭐ And `MakePlacementFootprintScale3D` returns `FVector(SafeScale, SafeScale, 1.f)` — X == Y is load-bearing, not a simplification: the ghost's only rotation is `GhostYawOffset`'s exact quarter turn, which swaps X and Y, so `max(|X|,|Y|)` is invariant only while they are equal.

**The CDO seam's one assumption, verified:** test 26(f) (`:2712-2727`) compares `CanScale(AClimbableTower::StaticClass())` against a **live** `AClimbableTower` asked through an **`ABuilding*`**, in **both** directions (both false for the tower, both true for a plain building — so it is not two falses agreeing by accident). ⭐ That doubles as the virtual-dispatch check: a shadowed non-virtual override passes every other row in the file and fails this one.

## ⛔ THE TASK-735 COMMENT-TERMINATOR SPECIES — SWEPT, NOTHING FOUND

The build-breaking defect was a `*/` inside a `/** */` block (14 errors from one character). **I swept for the species across all seven files in the five diffs** by censusing `/*` against `*/`:

| file | `/*` | `*/` | adjudication |
|---|---|---|---|
| `Building.h` · `Building.cpp` · `ClimbableTower.h` · `ClimbableTower.cpp` · `SiegeControlsHelpWidget.{h,cpp}` · `SiegePlayerController.h` | balanced | balanced | ✅ |
| `SiegePlayerController.cpp` | 19 | **21** | ✅ the +2 are `Enter*/BeginGroupPick` in **`//` line comments** at `:5618` and `:6314`, both in ordinary function bodies **outside any block comment** (read and confirmed). Both pre-existing, from the `TASK-440`/controls-help lanes. Inert |
| `Tests/SiegePlacementTest.cpp` · `Tests/SiegeControlsHelpTest.cpp` | balanced | balanced | ✅ |
| `Tests/SiegeBuildingStackTest.cpp` | 18 | **19** | ✅ the +1 is `TEXT("*/")` at `:303` — a **string literal** inside `CountOccurrencesInCode`'s skip rule. The `//` at `:298` also carries one; both sit after the enclosing `/** */` closed at `:280`. Inert |

⭐ **Positive control (`SC-§39`): the instrument returned 542 and 544 hits, so it can see the token — the balance is a measurement, not a blind zero.** ⇒ **No further defect of this species survives anywhere in the five diffs.**

## ⛔ `SC-§39` — THE BLIND-INSTRUMENT SWEEP

Per the dispatch, I checked every probe that reports a **zero** for a positive control. All of them have one:

| probe | zero claimed | ⭐ its positive control |
|---|---|---|
| `WatchTower` on code lines, `Building.{h,cpp}`/`ClimbableTower.h` | 0 | test 6, `:772` — the scanner **finds** the socket-diagnostic `UE_LOG` in `ClimbableTower.cpp` |
| `WatchTower` in the wheel body | 0 | test 27, `:2825` — proves the token **is** in the file's prose |
| `GroupRadiusWheelStep`/`Min`/`Max` in the wheel body | 0 | test 27, `:2813` — all three **findable** elsewhere in the same file |
| `FVector(PlacementFootprintScale` in ghost/confirm | 0 | test 28, `:2886` — `FVector(` **is** findable on code lines |
| digits in the six new help strings | 0 | test 15, `:1946` — the digit scanner **answers true** on `"up to 5 times taller"` |
| a fabricated `RelatedActionIds` id | null | test 15, `:2061` — `TestNull(FindAction("Cards.NoSuchUpgradeRow"))` |
| the wheel-row scanner | 3 rows | test 16, `:2183-2186` — answers **both** ways on synthetic rows |
| "unchanged on Dvorak" | identity | test 15, `:1903` — the injected map **really moves** `F` |

⚠️ **The amended clause (`CountOccurrencesInCode` skips lines whose trimmed form starts with `/*`) does not bite here.** I checked: the counted needles land on ordinary code lines — `:2362`'s `MakePlacementFootprintScale3D(` line begins `FRotator::ZeroRotator, …`, not `/*`. The named-argument comments in this diff (`/*Owner=*/`, `/*Instigator=*/` at `:2366-2367`) carry **none** of the counted tokens. And the digit scanner is a bare character loop with **no** skip rules at all, so its control needs no matching formatting.

---

## ⛔ CENSUS — `TL-§5b` / `TL-§5c`

> ⛔ **`392 declared`. ⛔ NOT `392/392`.** I ran ⛔ no suite. A pass count is `TASK-814`'s to write, and only with a `Result={Success}`/`Result={Fail}` pair in hand.

**Fresh, taken by me at review time:**
`392 declared across 29 files` — pattern `^IMPLEMENT_SIMPLE_AUTOMATION_TEST`, scoped to `Source/GitClaudeUnrealTest/Siegebound/Tests/*.cpp`.

⚠️ **The `TL-§5b` trap reproduced exactly, so nobody has to take it on trust:** the bare `^IMPLEMENT_` pattern returns **`393 across 30 files`** — the extra is `IMPLEMENT_PRIMARY_GAME_MODULE` in `Source/GitClaudeUnrealTest/GitClaudeUnrealTest.cpp`, outside the Tests scope. ⛔ **The off-by-one lands in the FILE count, which is exactly where it mimics an undeclared test file landing.** Use the scoped pattern.

**Per-subject deltas, verified in place (`TL-§5b` part 3 — the delta is the only trustworthy figure a task produces):**

| task | file | measured | declared delta |
|---|---|---|---|
| **812** | `SiegeBuildingStackTest.cpp` | **10** (new file) | `+10` ✅ |
| **735** | `SiegePlacementTest.cpp` tests 1–10 | 10 | `+10` ✅ |
| **813** | `SiegePlacementTest.cpp` tests 11–17 | 7 | `+7` ✅ |
| **815** | `SiegePlacementTest.cpp` tests 24–28 | 5 | `+5` ✅ |
| **823** | `SiegeControlsHelpTest.cpp` | **16** | `+2` (14 → 16) ✅ |

⭐ **`SiegePlacementTest.cpp` totals 28, and it reconciles exactly: `10 (735) + 7 (813) + 6 (819) + 5 (815) = 28`.** ⛔ No undeclared file in these three.

⚠️ **`392 − 376` (the last figure recorded in `TL-§5b`) = `+16` from lanes outside this gate.** ⛔ Not mine to attribute and ⛔ not a discrepancy — `TASK-814` reconciles it against **declared deltas**, ⛔ never against any absolute in any document, **including this one.** The absolute in `handoffs/TASK-815-programmer.md` (376) and `handoffs/TASK-823-programmer.md` (378) were both right when taken and are both stale now; the deltas are still right.

---

## Notes for build-master (`TASK-814`)

1. ⛔ **Parse the log for `Result: Failed` — never `$LASTEXITCODE`.** Standing law.
2. ⛔ **Take your own fresh census; do not reconcile *to* my 392.** It will have moved. Reconcile `fresh == prior + Σ(declared deltas)`, and if it does not balance that is a **reportable finding** (an undeclared test file), not something to narrate away.
3. ⭐ **W-5 — the one thing I could not measure and you can:** run `git diff -- Source/GitClaudeUnrealTest/Siegebound/SiegeControlsHelpWidget.cpp` and confirm there is **no hunk** inside the `Cards.Discard`, `Cards.Play` or `Cards.Cancel` blocks. If one appears, `TASK-821`'s commit will not rebase clean and that is a stop condition.
4. ⭐⭐ **ADD ONE PIE ROW to your §(3b) list — it settles W-1, and it is one click:** open the war map and **right-click on empty ground, away from any circle**. Report, as an observation: *does the map close, or does nothing happen?* ⛔ Either answer is fine; what is **not** fine is not knowing, because two help pages currently claim opposite things (`SiegeControlsHelpWidget.cpp:1268` vs `:1410`) and one of them is teaching a wrong control (`HELP-§2`). Route the answer to the manager.
5. ⭐ **Your §(3) wheel row is the instrument for W-6** — *"the wheel visibly resizes the ghost and the SPAWNED building matches"* is exactly the BP-construction-script risk `TASK-815` declared. Keep it and report it explicitly.
6. ⚠️ **Your `TASK-814` §(3) ×5 row:** the cap is reached at **4** upgrades (`min(1+n, 5)`), so *"repeating four times reaches ×5"* is right, and the **fifth** upgrade is the one that must still buy health with the ghost still blue. The HUD note (*"Maximum height reached - the upgrade added health only"*) fires **at confirm, once** — ⛔ not per frame — so watch for it on the click, not on the hover.
7. ⚠️ **The wheel's diagnostic is `Verbose`.** `SiegePlayerController.cpp:2871` prints nothing without `Log LogGitClaudeUnrealTest Verbose`. ⛔ **An empty log here reads as "the wheel never moved"** — the same false-pass shape that has bitten this project before. Enable it before the wheel row, or report the row as un-instrumented.
8. 📌 **`TASK-844`'s anchor:** `int32 DiscardAllCost = 20;` is at `SiegePlayerController.h:` **`1586`**, ⛔ not `:1496` as `handoffs/TASK-815-programmer.md` says (N-2). The **symbol is unique and intact**; relay the symbol, not the number.
9. 📌 Deferred to the M8 ledger, ⛔ not a fix: **the stacked height is server-local** (W-3).

## Notes for the manager

- **(g1)/(g2)/(g3) all confirmed** — `HELP-§7`'s correction, `TASK-852`'s re-scope and `CARDBAR-§9`/`STACK-§4`'s count fix are all **correct as landed**. ⛔ Nothing to reverse.
- **W-1 is the one item that needs a decision after `TASK-814`'s PIE row lands.** One sentence on one help page is wrong; the observation names which.
- ~~**W-2** — the right-click half of `CARDBAR-§8`/`STACK-§4` is **still owed**, correctly, and is Lane A's (`Cards.Play`/`Cards.Cancel` disambiguation) plus a possible new row for the spell-targeting cancel, which has **none**.~~
  > ### ⛔⛔ MANAGER RIDER — **STRUCK AND CORRECTED 2026-09-03.** ⛔ **W-2's second clause is MEASURED FALSE and this report must not be cited for it again.**
  >
  > ⛔ **The spell-targeting cancel does NOT have "none" — it has a shipped row, in both halves.** `Cards.Cancel`'s **one-liner** (📌 `:579`) reads *"Back out of whatever you are placing, **targeting** or circling"* and its **detail** (📌 `:593`) reads *"it exits placement mode, **exits spell targeting**, or aborts a group-order pick"*; the **code agrees at ONE handler** (📌 `:1343`) — `OnCancelPlacePressed`, branching placement → targeting → group-pick.
  > ⇒ ⛔ **A new row would DUPLICATE a shipped page** (the `HELP-§2` test-16 conflation, against the one-action-one-row rule, 704 D-1) — and de-duplicating would require editing `Cards.Cancel`, which was **fenced**.
  > ✅ **`TASK-870` DECLINED to build it and reported the measurement instead. That refusal is UPHELD** — re-measured independently by `qa/TASK-872.md` **R-1**, which is the *second* reader of the same premise.
  > ⭐ **The first clause of W-2 stands:** the `Cards.Play`/`Cards.Cancel` right-click disambiguation was real and shipped under `TASK-821`/`TASK-870`.
  > ⚖️ *`SC-§40` cl. 1 cuts toward QA reports and manager specs alike: **a verdict is a citation, not a fact.** A WARN that names an absence must be measured, not inferred from a registry table — `CARDBAR-§8` enumerates **right-click consumers**, and `Cards.Cancel` teaches the gesture on the **Escape/cancel** page, so the row was invisible to the instrument used.* ⛔ **This is `SC-§46`'s neighbour: an absence measured in the wrong index reads as a real absence.**
- **W-4** — board the four-corner slope trace as a follow-on; the wheel widened its exposure. ⛔⛔ **RIDER 2026-09-03: boarded as `TASK-871` — and `TASK-871` was DISPATCHED AND NEVER DELIVERED (no diff, no handoff), which `qa/TASK-872.md` discovered by arriving at an empty subject. RE-DISPATCHED with its own gate `TASK-914`. ⛔ The exposure is STILL OPEN.**

---

## Board status lines

⚠️ `TASKBOARD.md` is under concurrent write — returning the lines rather than editing the file.

```
TASK-816 → status: ✅ qa-passed (2026-09-02) — PASS on all five subjects, 0 blockers, 6 WARN, 4 NIT. Report: .claude/pipeline/qa/TASK-816.md. ⭐ Row (g1) RE-MEASURED AT SOURCE AND CONFIRMED: the registry-wide RelatedActionIds walk DOES already ship inside test 9 (SiegeControlsHelpTest.cpp:1098-1132). Census: 392 declared across 29 files (⛔ NOT a pass count, TL-§5c). ⚠️ W-1 owed: two help pages contradict each other on right-click over the war map — one PIE row settles it (added to TASK-814 §3b).
TASK-812 → status: ✅ qa-passed (2026-09-02) — 0 blockers. Height half of ApplyStackUpgrade RULED IN. WARN W-3 (stacked height server-local until M8).
TASK-735 → status: ✅ qa-passed (2026-09-02) — 0 blockers. SCALED bounds + max-not-sum confirmed at source. Comment-terminator species swept clean across all 7 files. WARN W-4 (board the 4-corner slope trace).
TASK-813 → status: ✅ qa-passed (2026-09-02) — 0 blockers. All four escalated rulings answered; bPlacementValid has exactly TWO readers, both correct.
TASK-815 → status: ✅ qa-passed (2026-09-02) — 0 blockers. All three flagged items ACCEPTED; both ordering assertions verified to DISCRIMINATE; the 3-occurrence count re-measured independently.
TASK-823 → status: ✅ qa-passed (2026-09-02) — 0 blockers. H-1 UPHELD (measured, not accepted). 27 rows confirmed. Zero digits confirmed by reading all six strings. WARN W-1/W-2.
TASK-814 → blocked-by: ~~TASK-816 (PASS)~~ ✅ CLEARED — gate PASSED 2026-09-02.
```
