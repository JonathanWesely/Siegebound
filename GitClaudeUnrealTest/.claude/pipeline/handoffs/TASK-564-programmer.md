# TASK-564 — [WR-10] TESTS, and ⭐ THE AIRLOCK PROOF — programmer handoff

**Agent:** gameplay-programmer · **Date:** 2026-08-15 · **Status on the board:** `ready-for-qa`
**Gate:** `qa/TASK-565.md` (the batch's ONLY QA gate) · **Compile + suite run:** TASK-566 · **Commit:** TASK-570
⛔ **No compile, no editor, no MCP, no PIE, no Git write, no model, no inference was performed by this task.**
The editor was up this session and was **not touched**.

---

## 0. ONE-PARAGRAPH SUMMARY

One new file — `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeWarMapTest.cpp`, **22 tests** under the
`Siegebound.WarMap.` prefix — covering TASK-560's four pure projection statics end to end, the marker
hit test, the map's outbound pick surface, the `WR-§7` reveal state machine on a world-less widget,
the `ACommanderNpc` reveal price, and TASK-561's `AppendToInput` refusal contract. **Zero existing
files were modified.** ⭐ **The headline deliverable is a NEGATIVE one and it is verified:
`Tests/SiegeAssistantZoneATest.cpp` is BYTE-UNTOUCHED and no second Zone A assertion was added
anywhere** — §1 below turns "still green" from a hope into a structural argument. **Two spec lines
could not be honoured as written and are declared as findings rather than coded around (`SC-§15`),
one of which — the whitespace rule — is genuinely unreachable through the shipped public API and
carries a concrete recommendation for the manager.**

---

## 1. ⭐⭐ THE AIRLOCK PROOF — AND IT IS EVIDENCE, NOT AN ASSERTION I WROTE

### (a) The file is untouched. Verified, not claimed.

```
$ git status --porcelain -- Source/GitClaudeUnrealTest/Siegebound/Tests/
?? GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeWarMapTest.cpp
```

**The `Tests/` tree contains exactly ONE pending change: my new, untracked file.**
⛔ `Tests/SiegeAssistantZoneATest.cpp` does not appear — it is not modified, not staged, not renamed.
⛔ **I added no second assertion about Zone A**, in that file or in mine. My file's header comment
says so in terms and points a reader at the existing one by name.

**The assertion that must stay green, cited by symbol (`SC-§18c`) — and the citation is REAL, I
checked rather than relayed it:**

| what | where | present? |
|---|---|---|
| `ShippedZoneAChars = 5658` | `Tests/SiegeAssistantZoneATest.cpp`, symbol `SiegeAssistantZoneATestFixture` | ✅ line 370 |
| test path `Siegebound.Assistant.ZoneA.MeasuredCharCount` | same file | ✅ line 809 |
| the **chars** claim | same file | ✅ line 849 |
| the **UTF-8 bytes** claim, asserted separately rather than inferred | same file | ✅ line 859 |

⇒ TASK-566 item (3) names this test explicitly. **It is the batch's airlock proof and I did not write it.**

### (b) ⭐ WHY IT IS STILL GREEN — a structural argument, not an optimistic one

`Siegebound.Assistant.ZoneA.MeasuredCharCount` measures `Snapshot->BuildZoneA(Vocabulary.Get())` on a
headless `NewObject` snapshot plus a default vocabulary. **Its entire input surface is:
`SiegeAssistantSnapshot.{h,cpp}` (the `BuildZoneA` composer + the file-local `PlaceVocabulary` table +
the roster fixture) and `SiegeAssistantVocabulary.{h,cpp}`.**

The batch's COMPLETE change set is 20 modified + 8 new files (`git status --porcelain`, read-only).
**Not one of them is in that input surface:**

- ⛔ `SiegeAssistantSnapshot.{h,cpp}` — **NOT in the diff.**
- ⛔ `SiegeAssistantVocabulary.{h,cpp}` — **NOT in the diff.**
- ⛔ `SiegeAssistantGrammar.{h,cpp}` · `SiegeAssistantCommand.h` · `SiegeAssistantComponent.{h,cpp}` — **NOT in the diff.**
- ⛔ No `.csv`, no `DT_Cards`, no `DA_AssistantVocabulary`. 🔒 `assistant_eval_holdout2.csv` untouched and still sealed.
- The two assistant-adjacent files that ARE in the diff — `SiegeAssistantConsoleWidget.{h,cpp}`
  (TASK-561) and `SiegePlayerController.{h,cpp}` (TASK-563) — are **UI/controller surfaces that author
  no prompt zone**. TASK-561's own handoff records `+255 / −0` with zero existing lines modified.

⇒ **Every byte `BuildZoneA` reads is unchanged, therefore its output length is unchanged, therefore
`5658` still holds.** That is a proof by input invariance rather than by re-measurement, and it is
falsifiable: if TASK-566 reports that test red, one of the four bullets above is wrong and the
handoff is wrong with it.

⇒ ⭐ **THE WAR MAP SPENT ZERO PROMPT CHARACTERS.** No new `who` shape, no grammar alternation, no
schema key, no place symbol, no coordinate field. The whole feature's outbound surface is one
delegate carrying one `FName`.

### (c) ⛔ NO TOKEN FIGURE APPEARS ANYWHERE

`AS-§12g` / `WR-§6` binds. **Every size claim in my file and in this handoff is in CHARACTERS or
BYTES.** The word "token" appears in my file three times and every one of them means *"the exact
string the model must emit"* — ⛔ never a count. The shipped `zoneA_tok` has still never been printed
and **TASK-552 still owes it**; nothing here quotes, derives or reasons from one.

### (d) 🔒 NOTHING HERE RUNS THE MODEL

⛔ No inference, no `Capture()`, no live prompt build, no eval. **`ReportFirstCapture` is untouched and
its one-shot latch is UNSPENT** — the reading TASK-552 owes and TASK-528 blocks on is still available.

---

## 2. FILES

| file | change | lines |
|---|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeWarMapTest.cpp` | ⭐ **NEW** | 1,700 (22 `IMPLEMENT_SIMPLE_AUTOMATION_TEST` + 22 `RunTest` bodies = 44 anchors) |

**⛔ ZERO existing files modified.** ⛔ No `Content/`, no `.csv`, no `.uasset`, no `Build.cs` (SlateCore,
Slate and UMG are already public dependencies — `FGeometry` needs nothing new).

**⚠️ TASK-566 SUITE-COUNT DELTA, stated so the build-master has a number to check against:** the
suite was **88** tests (counted: Grammar 12 · Guard 9 · Selection 28 · ZoneA 5 · KeyboardLayout 7 ·
Settings 7 · StuckStatics 20 — matching TASK-551's recorded `88/88` baseline). **It should now report
110.** A total that is not 110 means a test failed to register, and that is itself a finding.

### ⛔ Spec item (6) — "EXTEND, DO NOT PROLIFERATE": I checked for a home first, and there isn't one

All seven existing files were examined. **None is a home for war-map geometry:**

| file | subject | why not |
|---|---|---|
| `SiegeAssistantZoneATest.cpp` | the Zone A prompt lane | ⛔ **THE FILE I MUST NOT TOUCH.** Adding here would destroy the proof outright. |
| `SiegeAssistantGrammarTest.cpp` | GBNF + JSON parse | the map authors no grammar and no JSON |
| `SiegeAssistantSelectionTest.cpp` | `who` / selection / exclusion | the map has no `who` shape |
| `SiegeAssistantGuardTest.cpp` | assistant guardrails | the map reaches no guardrail |
| `SiegeKeyboardLayoutTest.cpp` | `KBD-§` positional remap | unrelated |
| `SiegeSettingsTest.cpp` | the persisted settings store | unrelated |
| `SiegeStuckStaticsTest.cpp` | `FSiegeStuckStatics` | ⭐ **the STRUCTURAL precedent I followed** — one pure-statics library, one file |

⇒ **`SiegeWarMapTest.cpp` follows the shipped one-subject-one-file law** set by
`SiegeStuckStaticsTest.cpp` (`FSiegeStuckStatics`) and `SiegeKeyboardLayoutTest.cpp`
(`FSiegeKeyboardLayoutStatics`). ⚠️ **This is a judgement call and I am flagging it as one:** if QA
reads "extend, do not proliferate" as forbidding an eighth file regardless of subject, the merge
target would be `SiegeAssistantSelectionTest.cpp` and I will move it — but I believe filing war-map
geometry inside the assistant's selection tests is the worse outcome, and last batch's finding was
about a **second file for the same subject**, which this is not.

---

## 3. EVERY ASSERTION'S SUBJECT — the 22 tests

### Group A — `FSiegeWarMapProjection`, the four pure statics (tests 1-10)

| # | test | subject | ⭐ the mutation it kills |
|---|---|---|---|
| 1 | `ProjectionCentreAndCorners` | `WorldToMapUV` | centre→centre; the four `ArenaHalfExtent` corners→the four UV corners, run **twice** (clean 2:1 arena AND the shipped CDO extent, so a projection that accidentally assumed a square aspect fails) |
| 2 | `ProjectionOrientationIsPinned` | `WorldToMapUV` | ⭐⭐ **the Y FLIP.** 33-sample monotonic sweep: UV.X strictly **increases** with world X, UV.Y strictly **decreases** with world Y. **Deleting the flip renders the whole battlefield mirrored about the castle axis while still looking plausible** — this is the assertion that catches it. Plus axis independence (X alone must not move V) |
| 3 | `ProjectionClampsOutOfBoundsInsteadOfDropping` | `WorldToMapUV` | pins to the edge, never drops, never escapes `[0,1]²` — verified across **nine decades** of distance in both directions |
| 4 | `ProjectionSurvivesADegenerateArenaExtent` | `WorldToMapUV` + `ComputeMapRectLocal` | ⛔ **the zero-divide.** `(0,0)` and NEGATIVE extents both stay finite (`FMath::IsFinite`, which is false for NaN and Inf alike) and both land at centre; the negative case is asserted as **equality with the zero case** so the two paths cannot drift |
| 5 | `ShippedArenaFallbackIsNeverDegenerate` | `USiegeScatterConfig` CDO ↔ `MinArenaHalfExtentUu` | the fallback `ResolveArenaHalfExtent` actually takes is strictly **above** the divisor floor on both axes, and yields a positive rect. ⛔ **Deliberately does NOT pin `(26000, 12000)`** — see §5 |
| 6 | `MapRectPreservesTheArenaAspect` | `ComputeMapRectLocal` | letterboxing across four panels (width-bound / height-bound / tall / exact-fit): rect aspect == **arena** aspect, fits inside the padded box, and is the **largest** such rect |
| 7 | `MapRectIsCentredInsideThePadding` | `ComputeMapRectLocal` | centring asserted as **"left margin == right margin"**, ⛔ not by re-deriving the formula (a re-derivation agrees with the implementation even when both are wrong). Negative padding asserted as **equality with zero padding** |
| 8 | `MapRectFailsClosedAndIsNeverNegative` | `ComputeMapRectLocal` | ⛔ **a NEGATIVE rect size would INVERT every hit rect** and make `FindMarkerIndexAtLocal` answer for points nowhere near a marker. Five degenerate panels: size exactly zero, ⛔ never negative, origin at panel centre |
| 9 | `MapUvToLocalSpansExactlyTheRect` | `MapUVToLocal` | (0,0)→origin, (1,1)→far corner, (0.5,0.5)→centre; affine and axis-independent |
| 10 | `FullChainMapsWorldCornersToRectCorners` | all three composed | ⭐ the composition `NativePaint` and `BuildMarkerRects` actually perform — a link can be individually correct and still be composed in the wrong order or against the wrong rect |

### Group B — the hit test and ⭐ the airlock (tests 11-15)

| # | test | subject | ⭐ the mutation it kills |
|---|---|---|---|
| 11 | `MarkerHitTestBoundaryIsInclusive` | `FindMarkerIndexAtLocal` | 11 cases: centre, all four edges **exactly** (inclusive `<=`), the corner, one pixel past on each axis, the **axis-pair trap** (inside on X, outside on Y), and a zero-size hit box that degenerates to a point rather than to "everything" |
| 12 | `MarkerHitTestLastMatchWinsOnOverlap` | `FindMarkerIndexAtLocal` | ⭐ **NOT an edge case — the ordinary case** (`hero` standing at `own_castle`). Markers paint in vocabulary order, so the LAST is on top; picking the first hands the player **the symbol of a marker they cannot see** — a silent wrong answer. Two-marker overlap **and** three exactly coincident markers |
| 13 | `MarkerHitTestOnEmptyMapReturnsNothing` | `FindMarkerIndexAtLocal` | ⛔ `WR-§9` outcome 1. Empty array, origin, negative point, and — the important one — **a click between two markers returns NOTHING, with ⛔ NO nearest-marker fallback**. It does not invent a coordinate, a grid cell or a snap radius |
| 14 | `PickReturnsTheSymbolPlacedAtThatRect` | the symbol channel | all **seven** canonical symbols, each hit at its own centre, each compared with ⛔ **`TestEqualSensitive`** — plus explicit byte-exact pins on `ancient_ground_near` / `ancient_ground_far` (underscores and case intact) and an assertion that the vocabulary is **exactly 7** |
| 15 | ⭐⭐ `AirlockPickSetIsExactlyTheMarkerSetOrNothing` | **the whole outbound surface** | see below |

**⭐ Test 15 is the headline test and it is a MEASUREMENT rather than an argument.** It sweeps
**120,000 click points** (400 × 300) across a full 1920×1080 panel carrying the seven canonical
markers, and asserts three things:

1. ⛔ **ZERO answers outside the marker array** — the map can only ever name a marker it was
   *handed*, never one it *constructed*.
2. ⭐ **Exactly seven distinct markers are reachable and ⛔ NO EIGHTH ANSWER EXISTS** anywhere on the
   panel — at any click precision.
3. ⭐ **The overwhelming majority of the panel answers NOTHING** — empty map emits no symbol at all.

⇒ **However precisely the player clicks, the map emits one of the symbols it was given, or nothing.
It cannot emit a coordinate, a grid cell, an interpolation between two markers, or an eighth place,
because there is no code path that constructs a symbol.** That is the airlock, measured.

⚠️ **The sweep collects `int32` INDICES, ⛔ not `FName`s or `FString`s, and that is deliberate:**
`TSet<FName>` hashes case-**insensitively** and `GetTypeHash(FString)` is `Strihash`, so either
container would silently absorb a symbol whose case had drifted. Indices are exact; **test 14** pins
index → symbol byte-exactly with `TestEqualSensitive`. The two together give a byte-exact claim about
the entire reachable set **with no case-insensitive container anywhere in the argument.**

**Test 15 also asserts its own PRECONDITION** (no two fixture markers overlap) before sweeping —
because an overlap would engage the last-match-wins rule and turn a *reachability failure into a
pass*. Same "did the guard actually run" discipline `SC-§21` applies to guard placement.

### Group C — `UWarMapWidget` state, headless (tests 16-20)

| # | test | subject |
|---|---|---|
| 16 | `BuildMarkerRectsYieldsNothingWithoutASnapshot` | ⭐ encodes **TASK-560's named finding**: no snapshot ⇒ **empty, ⛔ never partial, never stale** (a partial list is far worse — the player clicks a marker resolved against a survey that no longer exists). The out-array is **RESET, not appended to** — asserted by pre-populating it with junk |
| 17 | `RevealReplacesNeverAppendsAndStartsEmpty` | ⛔ no dots before payment · a second purchase **REPLACES** (5 then 2 gives **2, not 7** — an appending list would become a heat map of everywhere the enemy has ever been) · an **empty payload is a real answer** and clears |
| 18 | ⭐ `RevealIsClearedOnCloseAndStaysClearedOnReopen` | **`WR-§7`'s state machine, all four legs**: (1) nothing on a fresh open → (2) dots held after a push → (3) ⛔ **cleared on close, unconditionally** → (4) ⛔ **still cleared on re-open** — then repeated over **three full cycles**, plus a redundant `OpenMap` that must NOT wipe a live purchase and a redundant `CloseMap` that must be a no-op |
| 19 | `ClearEnemyRevealIsIdempotent` | reachable from `CloseMap`, `NativeDestruct` and a match reset, so it is called repeatedly on the same empty state as a matter of course; and clearing is **not a one-way latch** |
| 20 | `ToggleMapMirrorsTheOpenState` | ⭐ **the key path the player actually uses.** `ToggleMap` is `IA_WarMap`'s binding; if it had been written as its own state machine the clear-on-close would be reachable one way and not the other. Nine toggles, no drift |

**⚠️ HOW TESTS 16-20 RUN WITH NO WORLD — verified at the source, not assumed** (QA should re-check
this, it is the file's one unusual mechanism):
`OpenMap` / `CloseMap` / `ToggleMap` are safe on a bare `NewObject` widget because every world-dependent
leg null-guards and early-returns — `UWarMapWidget::SetAllyRefreshTimerEnabled` (`GetWorld()` null ⇒
`return`) and `UWarMapWidget::RefreshAllyDots` (same). `SetVisibility` no-ops without a realized Slate
widget: `UWidget::SetVisibilityInternal` guards on `GetCachedWidget()` (Widget.cpp:430) and
`UWidget::BroadcastFieldValueChanged` guards on an empty `EnabledFieldNotifications`
(Widget.cpp:2049). `UUserWidget::GetWorld()` walks the outer chain and returns null off the transient
package (UserWidget.cpp:476-517). The `BlueprintImplementableEvent`s are the same no-op they are on
the shipped **"no `WBP_WarMap` yet"** path this class was explicitly designed to survive.

### Group D — the price and the console seam (tests 21-22)

| # | test | subject |
|---|---|---|
| 21 | `EnemyRevealCostIsThirtyOnTheCommanderCdo` | `ACommanderNpc` CDO: `EnemyRevealCost == 30` (Jonathan's own number, a `UPROPERTY` default per `WR-§7`, ⛔ never a `cards.csv` column) · **a free reveal would delete the mechanic silently** (a zero cost "spends" successfully on every balance) · `InteractRadius` positive **and still human-scale** — asserted `< 2000` because `WR-§1` / `SC-§34`'s human-scale exemption says it did **NOT** grow with the 9× castle |
| 22 | ⭐ `AppendToInputRefusesAndNeverOpensTheConsole` | empty / whitespace-only / tab-newline all refused with ⛔ **no stray separator** · a real symbol into a **CLOSED** console is refused, loudly · ⭐⭐ **and the console is STILL CLOSED afterwards** — TASK-561's pinned contract to TASK-563, publicly observable via `IsConsoleOpen()` |

---

## 4. ⚠️⚠️ FINDING 1 — THE WHITESPACE RULE IS NOT TESTABLE THROUGH THE SHIPPED PUBLIC API

**This is a departure on a checkable spec line and it is declared rather than papered over
(`SC-§15`).** Spec item (4) asks for the TASK-561 whitespace rule (empty box / mid-sentence /
trailing space) to be asserted by string equality. **I did not assert it, and it cannot be asserted
from a test file.** Three reasons, all verified at the source:

1. **The composition is INLINE inside `AppendToInput`** (`SiegeAssistantConsoleWidget.cpp`, the block
   commented *"THE WHITESPACE RULE (header comment, rules 1-3). TASK-564 tests each case."*), not
   extracted into a pure helper.
2. **Reaching that composition requires a Slate-REALIZED widget.** `InputBox` is constructed by
   `ConstructConsoleTree()`, which runs only from `RebuildWidget()`, which runs only from
   `TakeWidget()`. It additionally requires `bConsoleOpen == true`, i.e. `OpenConsole()`, which takes
   keyboard focus. **That is a PIE/Slate dependency, not a headless unit.**
3. ⭐ **Even with all of that, the result is UNREADABLE.** `InputBox` is `protected`
   (`SiegeAssistantConsoleWidget.h:694`, after `protected:` at 581) and **the class exposes no public
   getter for the input text** — grepped for `GetInput` / `InputText` / `GetText` on the header:
   **zero hits.** The only public observable of a successful append is the returned `bool`.

⛔ **AND I MAY NOT FIX (1).** TASK-564's own `names:` block lists **"any non-test source file"** as
NOT TOUCHED. Extracting the helper is a `SiegeAssistantConsoleWidget.{h,cpp}` edit.

⛔ **I did NOT write a replica test.** Transcribing the rule into my file and asserting it against my
own copy would assert **nothing whatsoever about shipped code** — precisely the failure mode this
batch keeps warning about, and precisely how a shipped QA-passed test in this project once ended up
asserting nothing. **A green test that proves nothing is worse than a stated gap.**

**⚠️ TASK-561 PRE-AUTHORISED THE REVERSAL AND ASKED TO BE TOLD:** its handoff §8.7 reads *"TASK-564
can call it against a widget with a code-authored `InputBox`, or **extract the composition into a
testable helper if it prefers** — I left the composition inline… **that is a call TASK-564 is free to
reverse; say so if you want the split**."* ⇒ **I am saying so.**

### 🚩 RECOMMENDATION FOR THE MANAGER — three candidates, ⛔ I chose none of them

| # | candidate | cost | consequence |
|---|---|---|---|
| **A** ⭐ **preferred** | a **new task** giving TASK-561's author a `static FString ComposeAppendedInput(const FString& Existing, const FString& Symbol)` on `USiegeAssistantConsoleWidget`, with `AppendToInput` calling it. TASK-564 then gets a second test asserting all five table rows with `TestEqualSensitive` | one small refactor + ~1 test; **⛔ it is a `SC-§27` code diff and would need the gate** | the rule becomes permanently, headlessly testable. **⚠️ `SC-§33` would bind on the new signature** — it must take both parameters explicitly |
| **B** | add a PIE row to TASK-569: *"click a marker mid-sentence and confirm the box reads `send 10 footmen to ancient_ground_near ` with exactly one space and no `nearand` collision"* | one observation line | ✅ **the cheapest honest coverage, and it is a real measurement** — ⚠️ but it is a human eye, not a regression guard |
| **C** | accept the gap for this batch and record it | zero | the rule ships **unwatched**. ⛔ `SC-§32` is explicit that a mechanism never observed to function is not known to function |

⚠️ **I lean A + B.** ⛔ **But this is a scope decision and it is not mine to take.**

---

## 5. ⚠️ FINDING 2 / DECLARED DEPARTURE — I REFUSED TO PIN THE ARENA EXTENT'S VALUE

Spec item (2) says *"the zero/absent-config path takes the named fallback rather than dividing by
zero."* Test 5 asserts the **path** and the **property** — that the fallback is non-degenerate,
strictly above `MinArenaHalfExtentUu`, and yields a positive rect. ⛔ **It deliberately does NOT
assert `(26000, 12000)`.**

⚖️ **Reason, and it is `SC-§34` applied to a test rather than to shipped code:** pinning the number
re-creates the exact stale-derived-constant defect that clause exists to prevent. The day Jonathan
resizes the arena, **a test named for the WAR MAP would fail for a reason that has nothing to do with
the war map** — and the next reader would "fix" it by editing my number. `USiegeScatterConfig` owns
that value; its correctness is that class's business, and **TASK-569 reads the ASSET lane back**
(`WR-§2` row 4: the saved `DA_BattlefieldScatter` overrides the C++ default, so the CDO is only ever
half the story — a limit test 5 states in its own comment rather than glossing).

**Corollary, stated so it is not mistaken for inconsistency:** the seven place symbols in my fixture
**are** transcribed. That is the opposite choice and it is deliberate — `PlaceVocabulary` is a
file-local `static constexpr` table in `SiegeAssistantSnapshot.cpp` with **no public publisher**, so
the only runtime route to it is `GetPlaceNames()` on a `Capture()`d snapshot, which needs a world.
**More importantly it is the RIGHT choice for a vocabulary: an eighth place appearing in the shipped
table SHOULD break a test that spells seven.** Shipped precedent: `SiegeAssistantGrammarTest.cpp:65`
and `SiegeAssistantSelectionTest.cpp:159` transcribe place symbols the same way.

---

## 6. ⛔ `SC-§33` — DISCHARGED, WITH THE GREP

**ZERO defaulted parameters were added.** Every fixture helper takes all of its parameters
explicitly — `MakeMarker(FName, const FVector2D&, double)`, `MakePanel(float, float)`,
`MakeDotList(int32)`, `BuildCanonicalMarkers(const FVector2D&, const FVector2D&)`.

```
$ grep -cE "\w+\s+\w+\s*=\s*\w+\s*[,)]" Source/.../Tests/SiegeWarMapTest.cpp
0
```

⇒ **`SC-§33`'s enumerated call-site sweep does not fire: there is no new defaulted parameter and
therefore no call site to classify.** (⚠️ It *would* bind on candidate **A** in §4 — flagged there.)

**Other batch-wide checks, run on my file:**

| check | result |
|---|---|
| `TestEqual` on any `FString` / `ToString()` (⛔ case-INSENSITIVE in UE 5.8) | ✅ **zero hits** — every string claim uses `TestEqualSensitive` |
| any token **figure** quoted or derived (`AS-§12g`) | ✅ **zero** — chars/bytes only |
| test-class name collision with the other 7 test files | ✅ **zero** — `FSiegeWarMap*` appears only in my file |
| test-path collision | ✅ **zero** — `Siegebound.WarMap.` appears only in my file; all 22 paths unique |
| any wave-2 (TASK-573..576) test | ✅ **zero, by ruling** — see §7 |

---

## 7. ⛔ WHAT IS DELIBERATELY NOT HERE

- ⛔ **NO WAVE-2 TESTS (TASK-573 · 574 · 575 · 576).** By manager ruling: a unit test there would have
  to build a world with a castle in it and would then be asserting **the engine's `GetActorBounds`**,
  not our logic. **TASK-569's PIE rows (n)(o)(p) are the acceptance instrument** and those tasks'
  handoffs already state what a human must observe. I was not blocked on them and did not read into
  their scope.
- ⛔ **THE GOLD.** `ServerRequestEnemyReveal_Implementation`, the authority branch, `SpendGold`, the
  **NET-ZERO refusal on a short balance**, and `ClientReceiveEnemyReveal` all need a world, a player
  state, a castle and an own-team `ACommanderNpc`. I assert the **price** (test 21) and the widget's
  **sink** behaviour (tests 17-20) and nothing between them. ⇒ TASK-569 rows (i) and (j).
- **The painter.** `NativePaint` needs an `FSlateWindowElementList`. Every geometry it draws comes out
  of `BuildMarkerRects` / `FSiegeWarMapProjection`, which **are** covered.
- **`RefreshAllyDots`, the refresh timer, `CreateAndAddToViewport`, `NativeConstruct`/`NativeDestruct`,
  `NativeOnMouseButtonDown` itself, the proximity gate, `IA_WarMap`, every cursor/posture concern** —
  all need a world, a viewport or a pawn.
- **`BuildMarkerRects`' degenerate-panel early return.** ⚠️ **Named as UNCOVERED rather than
  decorated with a test that cannot see it:** the snapshot is tested first, so with no snapshot the
  function returns before the rect is ever computed. A "degenerate panel yields no markers"
  assertion would be green **because of the null snapshot** and would stay green if the panel guard
  were deleted outright. Test 16 says this in its own comment. The guard's *arithmetic* is covered by
  test 8; its *consumption* needs PIE.

---

## 8. ⚠️⚠️ SPEC ITEM (7) — THE MUTATION CHECK, AND WHY I COULD NOT RUN ONE

Item (7) asks that where a mutation check is cheap, I break one thing deliberately, **watch** the
right test go red, and report it. ⛔ **I could not do that, and I am not going to describe a
derivation as an observation** — `AS-§12g`'s discipline and `SC-§32`'s.

**This task is FILE-ONLY: no compile, no suite run.** TASK-566 is the batch's only compile. A
mutation check requires *two* runs of a suite I am not permitted to build, and mutating shipped
source would violate my `names:` block twice over.

**What I did instead, and it is weaker — say so in the QA report if it is not enough:** every
assertion in §3's tables carries a named mutation in its "⭐ the mutation it kills" column, derived by
reading the finished implementation. The three I would most want watched, in priority order, are:

1. ⭐ **Delete the Y inversion** in `WorldToMapUV` (`MapUV.Y = (HalfY - WorldXY.Y) / (2*HalfY)` →
   `(WorldXY.Y + HalfY) / (2*HalfY)`). **Expected red: test 2** (`ProjectionOrientationIsPinned`,
   the strict-decrease line) **and test 1** (the four corners) **and test 10**. This is the defect
   that renders the battlefield mirrored while looking perfectly plausible.
2. ⭐ **Scan `FindMarkerIndexAtLocal` forwards instead of backwards.** **Expected red: test 12 only**
   — and nothing else, which is exactly why test 12 exists as its own test.
3. ⭐ **Remove the `FMath::Max(..., MinArenaHalfExtentUu)` floor.** **Expected red: test 4** (the
   `IsFinite` lines go red on NaN).

⇒ **A green run of this file means the map's geometry and its outbound surface are right. It does not
mean the feature works.** The batch's real gates remain TASK-569's PIE matrix and Jonathan's playtest.

---

## 9. ⚠️ WHAT QA SHOULD SCRUTINISE (ranked)

1. ⭐⭐ **§1(b), the airlock argument.** Re-run `git status --porcelain -- Source/` and confirm
   `SiegeAssistantSnapshot.*` and `SiegeAssistantVocabulary.*` really are absent. **If either is in
   the diff, my proof is wrong and this comes back to me.**
2. ⭐ **§4, the whitespace finding.** Re-check the three legs: composition inline · `InputBox`
   `protected` at `SiegeAssistantConsoleWidget.h:694` · no public text getter. **If a public read
   exists that I missed, the finding is wrong and I owe the test.**
3. ⭐ **The headless-widget mechanism** behind tests 16-20 (end of §3, Group C). This is the file's
   one unusual claim. Four engine citations are given by file and line; **if any is wrong, tests
   16-20 may crash the suite rather than fail it**, which is a much worse outcome than a red line.
4. **Test 15's precondition block.** Confirm you agree that asserting non-overlap *before* sweeping
   is what stops a reachability failure passing silently.
5. **§5, the refusal to pin `(26000, 12000)`.** ⚠️ If you read `SC-§34` as *requiring* the pin, that
   is a legitimate disagreement — but note that pinning it makes a war-map test fail on an arena
   resize, and TASK-569 reads the asset lane anyway.
6. **§2, the eighth-file judgement call.** If "extend, do not proliferate" forbids a new file
   regardless of subject, say so and I will merge into `SiegeAssistantSelectionTest.cpp`.
7. ⚠️ **Compile risk, stated honestly (`SC-§32`): nothing here was compiled.** The overloads I leaned
   on were each verified against the installed UE 5.8 headers rather than assumed —
   `TestEqual(const TCHAR*, double, double, double)` (`AutomationTest.h:1989`, and note there is **no
   `FVector2D` overload**, which is why every vector claim is split into X and Y),
   `TestEqualSensitive(const TCHAR*, const FString&, const FString&)` (`:2016`),
   `TestNotNull` (`:2449`), `FGeometry::MakeRoot` (`Geometry.h:197`),
   `FMath::Pow(double, double)` and `FMath::IsFinite(double)`
   (`GenericPlatformMath.h:556` / `:587`), and the non-explicit `FName(const WIDECHAR*)`
   (`NameTypes.h:1079`). **A compile error here is mine and I expect to fix it, not TASK-566.**

---

## 10. 📌 M8 DECLARATION (`WR-§8` — ⛔ NOT the last three batches' boilerplate)

**This task adds NO replicated property, NO new replicated class, NO new relevancy tier and NO RPC.**
It is a test file: **it adds no shipped surface at all** and links only into
`WITH_DEV_AUTOMATION_TESTS` builds. ⚠️ The batch's two RPCs (`ServerRequestEnemyReveal` /
`ClientReceiveEnemyReveal`) are TASK-563's, on the controller, and are **not exercised here** — they
need an authority (§7).

---

## 11. ⚠️ NOT MINE, FLAGGED NOT FIXED

- ⚠️ **TASK-569's PIE matrix has no row for the SHORT-BALANCE refusal.** Row (i) covers *"paying 30
  gold shows red dots and the gold actually decrements"* and row (j) covers the close-clears rule,
  but ⛔ **nothing observes `WR-§7`'s net-zero refusal — "insufficient gold ⇒ the button refuses
  BEFORE any gold moves"**. That is a shipped guardrail with **no test and no PIE row**, i.e. exactly
  `SC-§32`'s "never observed to trip". 🚩 **Suggested new row: spend down to under 30, click reveal,
  confirm a HUD line, ⛔ ZERO dots, and the gold total UNCHANGED.** ⛔ Board edits are the manager's.
- **TASK-560's snapshot residual** (a map opened before the first console sentence has no markers)
  is **encoded** by test 16 but **not resolved** — its three candidate owners are still open in
  `handoffs/TASK-560-programmer.md`.
- 🔒 The sealed holdout, `ReportFirstCapture`'s unspent latch, and the unscheduled spike file are all
  untouched by this task.

---
---

# ⛔ QA-FAILED REPAIR — QA LOOP 1 of 3 — 2026-08-15

**Trigger:** the COMPILE-GATE APPENDIX on `qa/TASK-565.md` (B1) + `handoffs/TASK-566-buildmaster.md` §4.
**Error:** `Tests/SiegeWarMapTest.cpp(8,1): fatal error C1083: Cannot open include file:
'Layout/SlateLayoutTransform.h': No such file or directory`
**Constraint honoured:** FILE-ONLY. ⛔ No compile, no Git (see §R6), no editor, no MCP, no PIE. The
editor was left CLOSED by build-master and I did **not** relaunch it. ⛔ I stayed inside
`Tests/SiegeWarMapTest.cpp` — TASK-560 is being repaired in parallel and I touched nothing of its.

---

## R1. THE FIX — the corrected include, pasted

```diff
  #include "Layout/Geometry.h"
- #include "Layout/SlateLayoutTransform.h"
  #include "Math/UnrealMathUtility.h"
  #include "Math/Vector2D.h"
+ #include "Rendering/SlateLayoutTransform.h"
  #include "Siegebound/CommanderNpc.h"
```

The corrected block, verbatim as it now stands (`SiegeWarMapTest.cpp:3-17`):

```cpp
#include "Misc/AutomationTest.h"

#include "Containers/Array.h"
#include "Containers/Set.h"
#include "Layout/Geometry.h"
#include "Math/UnrealMathUtility.h"
#include "Math/Vector2D.h"
#include "Rendering/SlateLayoutTransform.h"
#include "Siegebound/CommanderNpc.h"
#include "Siegebound/ScatterConfig.h"
#include "Siegebound/SiegeAssistantConsoleWidget.h"
#include "Siegebound/WarMapWidget.h"
#include "UObject/NameTypes.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UObjectGlobals.h"
```

⚠️ **It MOVED as well as changed, and that is deliberate, not drift.** This block is strictly
alphabetically sorted; `Rendering/` sorts after `Math/` and before `Siegebound/`. Changing the path
in place would have left the block unsorted. **One include line changed. That is the whole diff of
this repair.**

📌 **Confirmed at the engine source, not assumed:** `class FSlateLayoutTransform` is defined at
`Runtime/SlateCore/Public/Rendering/SlateLayoutTransform.h:19`. ⚠️ And note
`Layout/Geometry.h:16` **already includes** `Rendering/SlateLayoutTransform.h`, so the type would
have resolved transitively even with line 8 deleted outright — the explicit include is kept because
`MakePanel` names `FSlateLayoutTransform` **directly** (IWYU), not because it is load-bearing.

---

## R2. ⛔ THE FULL INCLUDE AUDIT — ALL 14, EACH RESOLVED AT THE INSTALLED ENGINE

⛔ **I did not trust a path because it looked plausible — that is exactly what produced this
failure.** Every non-project header was resolved by locating the file on disk.

**Command run** (from `C:\Program Files\Epic Games\UE_5.8\Engine\Source`):
```sh
for h in <each include>; do find Runtime Developer Editor -path "*/Public/$h" -o -path "*/Classes/$h" | grep -i "/$h$"; done
```

| # | include | resolved to | verdict |
|---|---|---|---|
| 1 | `Misc/AutomationTest.h` | `Runtime/Core/Public/Misc/AutomationTest.h` | ✅ |
| 2 | `Containers/Array.h` | `Runtime/Core/Public/Containers/Array.h` | ✅ |
| 3 | `Containers/Set.h` | `Runtime/Core/Public/Containers/Set.h` | ✅ |
| 4 | `Layout/Geometry.h` | `Runtime/SlateCore/Public/Layout/Geometry.h` | ✅ ⛔ untouched, as instructed |
| 5 | ~~`Layout/SlateLayoutTransform.h`~~ | **NO MATCH — 0 results** | ⛔ **THE BUG** |
| 5′ | `Rendering/SlateLayoutTransform.h` | `Runtime/SlateCore/Public/Rendering/SlateLayoutTransform.h` | ✅ **THE FIX** |
| 6 | `Math/UnrealMathUtility.h` | `Runtime/Core/Public/Math/UnrealMathUtility.h` | ✅ |
| 7 | `Math/Vector2D.h` | `Runtime/Core/Public/Math/Vector2D.h` | ✅ |
| 8 | `UObject/NameTypes.h` | `Runtime/`**`Core`**`/Public/UObject/NameTypes.h` | ✅ ⚠️ note it is in **Core**, not CoreUObject — the path is still correct as written |
| 9 | `UObject/StrongObjectPtr.h` | `Runtime/CoreUObject/Public/UObject/StrongObjectPtr.h` | ✅ |
| 10 | `UObject/UObjectGlobals.h` | `Runtime/CoreUObject/Public/UObject/UObjectGlobals.h` | ✅ |
| 11-14 | `Siegebound/{CommanderNpc,ScatterConfig,SiegeAssistantConsoleWidget,WarMapWidget}.h` | all four present in `Source/GitClaudeUnrealTest/Siegebound/` | ✅ |

✅ **Every include is USED** — no dead include, and therefore no risk of the fix masking a second
one. `Containers/Set.h` → `TSet<int32>` (test 15); `Rendering/SlateLayoutTransform.h` → `MakePanel`;
`Layout/Geometry.h` → `FGeometry`; `UObject/StrongObjectPtr.h` → the six `TStrongObjectPtr` locals.

---

## R3. ⛔ THE FULL API AUDIT — EVERY ENGINE CALL READ AT THE SOURCE

⛔ **Nothing below is from memory.** Each row was read out of the installed UE 5.8 headers.

| symbol | read at | verdict |
|---|---|---|
| `TestEqual(const TCHAR*, double, double, double Tolerance)` | `AutomationTest.h:1989` | ✅ the double overload EXISTS; every tolerance call binds to it |
| `TestEqual(const TCHAR*, int32, int32)` | `:1985` | ✅ every count/index claim binds here (non-template beats the `:2186` template) |
| `TestEqualSensitive(const TCHAR*, const FString&, const FString&)` | `:2016` | ✅ **exact match** — beats the `FStringView` (`:2015`) and `const TCHAR*` (`:2014`) candidates |
| `TestTrue` / `TestFalse(const TCHAR*, bool)` | `:2603` / `:2367` | ✅ |
| `TestNotNull(const TCHAR*, const ValueType*)` | `:2449` (template) | ✅ accepts the `const ACommanderNpc*` CDO pointer |
| `AddInfo(const FString&, ...)` | `:1681` | ✅ |
| `EAutomationTestFlags::EditorContext \| EAutomationTestFlags::EngineFilter` | — | ✅ **byte-identical to the form used by all 7 already-compiling test files in this folder** (88 tests' worth of proof). Not a guess |
| `FMath::IsFinite(double)` | `GenericPlatformMath.h:587` | ✅ the **double** overload exists (not just `float`) |
| `FMath::Pow(double, double)` | `:556` | ✅ |
| `FGeometry::MakeRoot(const FDeprecateVector2DParameter&, const FSlateLayoutTransform&)` | `Geometry.h:197` | ✅ exact 2-arg shape `MakePanel` calls |
| `GetTransientPackageAsObject()` | `UObjectGlobals.h:276` | ✅ real, and the idiom **4 other shipped test files already use** |
| `FString::Contains(const TCHAR*, ESearchCase::Type, …)` | `UnrealString.h.inl:1177` | ✅ |
| `ESearchCase` | `Misc/CString.h:19` | ✅ reachable — pulled by `UnrealStringIncludes.h.inl` |

### ⚠️ THE WARNINGS-AS-ERRORS CLASS QA NAMED — CHECKED, AND IT IS CLEAN

- ⭐ **`FVector2f`, NOT `FVector2D`, at the Slate boundary.** `SlateVector2.h:490`
  `FDeprecateVector2DParameter(const FVector2f&)` carries **NO** `UE_SLATE_VECTOR_DEPRECATED_DEFAULT()`;
  the three deprecated ctors are `:498/:503/:508` (the double-precision ones). ⇒ `MakePanel`'s
  `FVector2f(WidthPx, HeightPx)` is the **non-deprecated** path. **The file's own comment on this is
  accurate.** Had it passed `FVector2D`, this module would have failed on the deprecation.
- ✅ **No unused `static` (MSVC C4505).** ⛔ I checked this specifically because a dead fixture in a
  warnings-as-errors module is a build failure, and `MakePanel` was the natural suspect (it is the
  *only* consumer of the header that was broken). **It is used twice — `:1307` and `:1327`.** All 13
  fixture helpers/constants have live call sites.
- ✅ **No narrowing.** Every `float`-parameter call site casts explicitly
  (`static_cast<float>(ShippedMapPaddingPx)`, `MakePanel(1920.f, 1080.f)`, `0.f`, `-250.f`), and
  `MinArenaHalfExtentUu` (a `constexpr float`) is `static_cast<double>` before every `double`
  comparison. Same class of thing as QA's `GetDefaultFontStyle(float)` check — **clean here too**.

### ⛔ THE ACCESSIBILITY AUDIT — the exact error class that killed TASK-560, run against MY file

⚠️ **This is the check the file-only gate structurally could not perform, and TASK-560 died on it.
So I ran it on every member this file touches.**

| member called | declared | region | ✅ |
|---|---|---|---|
| `UWarMapWidget::OpenMap/CloseMap/ToggleMap/IsMapOpen` | `WarMapWidget.h:428/436/440/443` | `public:` @383 | ✅ |
| `…::ReceiveEnemyReveal / ClearEnemyReveal` | `:465` / `:469` | `public:` | ✅ |
| `…::GetEnemyRevealDotCount / GetAllyDotCount` | `:473` / `:477` | `public:` | ✅ |
| `…::BuildMarkerRects(const FGeometry&, TArray<FSiegeWarMapMarker>&) const` | `:512` | `public:` | ✅ |
| `ACommanderNpc::GetEnemyRevealCost() const` → `int32` | `CommanderNpc.h:188` | `public:` @133 | ✅ (backing field `:293` is protected — **only the accessor is used**) |
| `ACommanderNpc::GetInteractRadius() const` → `float` | `:184` | `public:` | ✅ |
| `USiegeAssistantConsoleWidget::AppendToInput(const FString&)` | `…ConsoleWidget.h:554` | `public:` @292 | ✅ |
| `…::ComposeAppendedInput(const FString&, const FString&)` **static** | `:601` | `public:` | ✅ |
| `…::IsConsoleOpen() const` | `:337` | `public:` | ✅ |

⇒ ⛔ **`protected:` opens at `WarMapWidget.h:554`, `CommanderNpc.h:219`, `…ConsoleWidget.h:640`.
EVERY member this file touches is strictly above its class's first `protected:`. There is no second
C2248 waiting in this file.** ✅ Both `GetDefault<>` calls are on `const` accessors, so the CDO's
constness is satisfied.

---

## R4. ⛔ AN AUDIT THAT FOUND NOTHING ELSE IS A RESULT — AND HERE IT IS AS ONE

⛔ **STATED PLAINLY, AS REQUIRED: beyond the single wrong include path, the full include and API
audit of all 1,901 lines found ZERO further defects.** That is a finding, not an absence of effort —
14 includes resolved on disk, 13 engine APIs read at their declaration, 9 member accesses checked
against their class's access regions, and the three named warnings-as-errors traps (deprecated Slate
vector, unused static, narrowing) each checked and cleared.

⚠️ **AND THE HONEST BOUND ON THAT CLAIM (`SC-§32`):** this is still a **file-and-header audit**, not
a compile. It can prove a path exists, a signature matches and a member is reachable; it **cannot**
prove template overload resolution, UHT interaction, or link. ⛔ **I do not claim this file will
compile — I claim every defect of the class that killed it is now checked.** **TASK-566's re-run is
the authority, exactly as it was for TASK-560's C2248.**

---

## R5. 🔒 WHAT I DID NOT DISTURB — the batch's headline evidence

| protected thing | state |
|---|---|
| `Tests/SiegeAssistantZoneATest.cpp` | ✅ **NOT OPENED, NOT EDITED.** No second Zone A assertion added or removed |
| `SiegeAssistantSnapshot.{h,cpp}` · `SiegeAssistantVocabulary.{h,cpp}` | ✅ **NOT OPENED, NOT EDITED** — the four Zone A input-surface files are untouched by this repair |
| Zone A byte-freeze | ✅ **Unmoved.** This repair edits ⛔ **zero prompt-zone bytes** — it changes one `#include` in a test file that adds no shipped surface. ⛔ **No token figure is quoted anywhere in this handoff** (`AS-§12g`) |
| 120,000-point click sweep | ✅ **INTACT** — `SamplesX = 400` (`:1196`), `SamplesY = 300` (`:1197`) |
| 33-sample Y-flip sweep | ✅ **INTACT** — `SampleCount = 33` (`:394`) |
| `Siegebound.WarMap.ComposeAppendedInputWhitespaceRule` | ✅ **INTACT** — `:1730`, all its calls into the shipped static |
| test count | ✅ **23**, unchanged. ⛔ **No test was deleted, weakened, skipped or relaxed** |
| `TestEqualSensitive` discipline | ✅ Re-verified across all 23: **every** `FString` claim is Sensitive; every `TestEqual` is on `int32`/`double`; prefix claims use `Left()` + `TestEqualSensitive` (`:1839`), never `==`/`StartsWith` |
| TASK-560's files (`WarMapWidget.{h,cpp}`) | ✅ **READ ONLY, NEVER WRITTEN** — its parallel repair is untouched |

### ⭐ SUITE TOTAL — RE-COUNTED, AND IT IS UNCHANGED AT **111**

```sh
grep -c "^IMPLEMENT_\(SIMPLE\|COMPLEX\)_AUTOMATION_TEST" *.cpp
  SiegeAssistantSelectionTest 28 · SiegeWarMapTest 23 · SiegeStuckStaticsTest 20 ·
  SiegeAssistantGrammarTest 12 · SiegeAssistantGuardTest 9 · SiegeSettingsTest 7 ·
  SiegeKeyboardLayoutTest 7 · SiegeAssistantZoneATest 5          →  TOTAL 111
```
⇒ ✅ **My repair does NOT change the count.** 111 remains the expectation for TASK-566.

---

## R6. ⚖️ ONE DECLARED DEPARTURE (`SC-§15`) — I USED NO GIT AT ALL

⚖️ `qa/TASK-565.md` §11 answer 1 **rules read-only Git permitted** and names TASK-564 as right to
have used it. **My dispatch for THIS repair says FILE-ONLY, "no Git", with no exception.** ⇒ **I took
the stricter of the two and ran ⛔ ZERO git commands**, including read-only ones. **Declared rather
than silently assumed**, because it changes what evidence I can offer: I make **no diff-based claim**
in §R5. The airlock's byte-level proof remains **build-master's** (`TASK-566-buildmaster.md` §6,
blob-SHA equality at `f205eb5`) and I neither reproduce nor extend it. ⚠️ **What I can state is
narrower and checkable: this repair wrote to exactly ONE file, `Tests/SiegeWarMapTest.cpp`, changing
exactly ONE line.** ⛔ Re-establishing byte-equality of the five airlock files is TASK-566's to
re-confirm.

---

## R7. ➡ FOR QA AND TASK-566

1. ⛔ **Scrutinise first: is `Rendering/SlateLayoutTransform.h` right?** — `SlateCore/Public/Rendering/SlateLayoutTransform.h:19`, and `Layout/Geometry.h:16` includes it too. Two independent confirmations.
2. ⛔ **The re-run buys the right to discover the NEXT error, not a green build.** ⚠️ **None of this file's 1,901 lines or 23 tests has EVER been compiled.** I have audited them; **audited ≠ compiled.**
3. ✅ **The one thing TASK-566 still owes and I cannot supply:** `Siegebound.Assistant.ZoneA.MeasuredCharCount` green at **5658**. No binary containing it exists yet.
4. 📌 **Unchanged and still open from the original handoff:** §11's short-balance PIE row gap (`WR-§7`'s net-zero refusal has no test and no PIE row) and TASK-560's snapshot residual. ⛔ Board edits are the manager's.
5. ⚠️ **W3 stands:** `BuildMarkerRects`' degenerate-panel guard remains uncovered and test 16 **says so in its own assertion text** (`:1328`). ⛔ Not repaired — repairing it needs a snapshot fixture that does not exist headlessly.

**STATUS → `ready-for-qa`.**
