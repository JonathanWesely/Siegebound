# TASK-942 — THE SPLIT. `CanStackHeight()` IS BORN; THE WHEEL KEEPS `CanScaleFootprint()`.

**Author:** gameplay-programmer · **Date:** 2026-09-03 · **Gate:** `TASK-943`
**Mode:** ⛔ No compile · ⛔ no editor · ⛔ no MCP · ⛔ no Git · ⛔ **zero `Content/**` writes** · ⛔ no art,
mesh or re-export.

---

## ⭐⭐ THE SENTENCE THAT IS NOT OPTIONAL, VERBATIM, FIRST

> ### 🧑 **HIS SPEC SAID ×2 → ×5. ×2 IS A MEASURED SHORTFALL AGAINST IT, NOT A DESIGN CHOICE.**

`n ≥ 3` jams the deck-breach sweep **32 / 72 / 112 uu below** where the window opens ⇒ watchdog drop,
deck unreachable. The cap is **not** dressed as balance anywhere — not in `ClimbableTower.cpp`'s
constructor comment, not in `Building.h`, not in this handoff, not in the Slack post. The number comes
from `STACK-§10`; this row measured nothing and re-derived nothing.

---

## 1. ⭐ WHAT JONATHAN GETS

Hovering his own WatchTower with a WatchTower card in hand now resolves **`Ready`** instead of
`NotStackable`. The shipped three-state ternary paints `UpgradeGhostColor` on `Ready`, so **the ghost
turns BLUE and the click upgrades** — with ⛔ **zero tint edits** (item (5), confirmed in §7).

One upgrade: **×2 height, ×1.5 health**. The second and subsequent clicks still land, still buy health,
still show the shipped cap note — they simply stop adding height. `J-6`'s cap behaviour needed no new UI.

---

## 2. ⛔ THE SPLIT — WHAT IS NOW WHOSE

| question | virtual | `ABuilding` | `AClimbableTower` |
|---|---|---|---|
| may the **wheel** scale X/Y? | `CanScaleFootprint()` | `true` | ⛔ **`false`** — unchanged, **not reopened** |
| may the **stack** grow Z? | ⭐ **`CanStackHeight()`** (new) | `true` | ⭐ **`true`**, ceiling `2` |

⛔ **Not a wrapper.** Two virtuals, two literals, and they **DISAGREE on `AClimbableTower`** — which is the
only observable an alias can never produce. `{ return CanScaleFootprint(); }` is asserted absent from both
bodies at source *and* refuted behaviourally by the disagreement row.

### ⛔ THE THREE-GATE CENSUS — **MEASURED, WITH CONTROLS**

Located **by symbol** (`SC-§38`), never by the diagnosis's line numbers:

| # | gate | file | consults |
|---|---|---|---|
| 1 | `ResolvePlacementUpgradeState` gate (5) | `SiegePlayerController.cpp` | `CanStackHeight()` ×1 |
| 2 | `ConfirmStackUpgrade`'s re-ask | `SiegePlayerController.cpp` | `CanStackHeight()` ×1 |
| 3 | `ABuilding::ApplyStackUpgrade`'s guard | `Building.cpp` | `CanStackHeight()` ×1 |

> ### ⛔ **STACK-SITE CONSULTS OF `CanScaleFootprint()`: `0`.**
> ### ⭐ **A FOURTH CONSULT: `0`.** ⛔ **WHEEL CONSULTS SURVIVING: exactly `1`** — `CanCardActorScaleFootprint`
> (`SiegePlayerController.cpp`), which is the wheel's own seam and was left byte-untouched.

**Positive control for the needle:** `CanScaleFootprint(` still returns `> 0` on code lines of the
controller (the wheel's real call), so each per-gate zero is a finding and not a blind scanner.
**Negative control:** a synthetic macro name returns `0` across all 33 test files.

---

## 3. ⛔⛔ THE `TASK-956` FINDING — AND WHY THE AMENDED SIGNATURE IS LOAD-BEARING

`TASK-956` measured that a per-class ceiling would have been **silently ignored**: `Building.cpp:302`
read `GetDefault<ABuilding>()` — the **base** CDO, whatever instance was calling — and capped on it.
Setting `MaxStackHeightMultiplier = 2` on `AClimbableTower` would have been **read straight past** and the
tower would have kept stacking to `5`: the exact deck-unreachable state `STACK-§10` bans, with every
readback of the tunable still reporting `2`.

**Shipped:**

```cpp
static float ABuilding::StackHeightMultiplier(int32 UpgradeCount, int32 MaxMultiplier);
```

- ⛔ **Zero `GetDefault<>` reads** in the height resolver now — asserted, with the **health** resolver as
  the negative control (it *still* reads the CDO, deliberately, so the zero is a property of the height
  function and not of a blind scanner).
- ⛔ **Zero copies of the ceiling.** `MaxStackHeightMultiplier` remains the one `EditDefaultsOnly`
  storage; callers reach it through the new public `ABuilding::GetMaxStackHeightMultiplier()`.
- ⛔ Still a plain static: **not** a `UFUNCTION`, no defaults (`SC-§33`). Asserted in the reflection tables.
- ⭐ Side benefit that is really the point: the seam is now testable at ceilings the project does not
  ship, so the test asserts the **shape** rather than restating a value (`SC-§37`).

### ✅ THE FALSE COMMENT IS FIXED, AND SAID SO

`Building.cpp:296`'s *"THE CAP IS READ FROM THIS CLASS'S CDO"* was **false as written**. It is replaced
with a paragraph that names the old behaviour, why it was wrong, and what it cost. The mirror claim in
`Building.h`'s `MaxStackHeightMultiplier` comment (*"a BP child that re-authored this value would be
IGNORED by the series"*) is likewise rewritten — it was **true by accident**, for the wrong reason.

---

## 4. ⛔ ITEM (5a) — THE NAV-LINK RE-ARM

**Shipped:** a protected virtual `ABuilding::OnStackUpgradeApplied()` (empty base, the `OnStatsLoaded`
precedent), called by `ApplyStackUpgrade` **only on the success path, after the transform and the HP push**.
`AClimbableTower` overrides it and calls `ConfigureLadderLink()`.

- ⛔ **`ABuilding` holds no navigation call and names no navigation type** — measured: `ConfigureLadderLink`
  appears **0** times in `Building.cpp`. The knowledge lives on the class that owns the component.
- ⛔ **`ApplyStackUpgrade` kept its exact signature** (`bool ApplyStackUpgrade()`, non-virtual, not a
  `UFUNCTION`) so its M8 discipline is untouched *and* so `SiegePlacementTest`'s `J-4` body probe, which
  keys on that literal signature, keeps reading the same window.
- ⛔ **`OnStackUpgradeApplied`'s definition is placed ABOVE `ApplyStackUpgrade` in `Building.cpp`** —
  deliberately. That probe takes "the text from the signature to the next `\nfloat ABuilding::`", so a
  definition inserted *below* would have silently widened its window. Noted in the code.

✅ **The refuted hazard is NOT re-raised.** `ConfigureLadderLink` stores `RTS_Actor` relatives and
`NavLinkCustomComponent.cpp:518-525` re-applies the live owner transform on every read; four read sites,
zero caches. What is re-armed is the **registration**, not the geometry — stated in the override's comment.

⚠️ **Consequence I introduced and am declaring rather than letting QA find:** a successful upgrade now runs
`ConfigureLadderLink()`, which on a **world-free scratch tower** (no `SM_WatchTower`, no sockets) degrades
open to the pinned literals and logs **one Warning** — by design (`TOWER-§8.4(A)`). The new test therefore
carries a **scoped** `AddExpectedMessagePlain` for that one message. ⛔ Not a blanket warning suppression:
the M8 authority refusal and the statless-building path must stay loud in this file.

---

## 5. ⚠️ ITEM (5b) — THE JUICE TRAP: **ONE COMMENT, ZERO CODE**

Parked beside `ApplyStackUpgrade` (the function somebody would edit to set it off). It names that
`USiegeMeshJuiceComponent`'s squash terminal branch writes `SetRelativeScale3D(BaseScale)` **verbatim**, so
a future "upgrade squash" would silently **un-stack** the tower — height *and* climb line — with every
readback correct. **Measured safe today** (one call, at `ABuilding::BeginPlay`; `AClimbableTower : public
ABuilding`, not `ATower`) ⇒ ⛔ nothing was "fixed", and the comment says not to harden the juice component,
because the hazard is a call that does not exist.

---

## 6. ⛔ ITEM (6) — THE TESTS, AND THE **RED TRANSCRIPTS**

`SiegeBuildingStackTest.cpp` **10 → 14** tests. `SiegePlacementTest.cpp` **31 → 31** (one test inverted and
renamed, none added). ⇒ ⭐ **`TL-§5c` DECLARED DELTA: `+4`.** ⛔ `declared`, ⛔ not a pass count.

**Tree census, `declared` and stale by construction:** `439` across `33` files, scope
`Source/GitClaudeUnrealTest/Siegebound/Tests/*.cpp`, needle `^IMPLEMENT_SIMPLE_AUTOMATION_TEST`, with the
positive and negative controls above. ⛔ **Re-census; do not reconcile to it** — `TASK-957` and `TASK-964`
are writing in the same tree right now. `TL-§5b`: ⛔ no suite absolute appears as an expectation anywhere.

### ⛔ THE INSTRUMENT — AND ⭐ THE DEFECT IT CAUGHT IN MY OWN GATE

Mutations were applied **in memory** to the exact text the probe reads, so the source tree was never
written and nothing could be left mutated. For a source-scanning gate that is equivalent to editing and
reverting, and strictly safer.

⛔⛔ **THE HARNESS IMMEDIATELY FAILED ONE OF ITS OWN MUTATIONS, AND THE FINDING WAS REAL.** `MUT B2`
(delete gate 2's consult outright) came out **GREEN**: my new `UE_LOG` literal quoted `CanStackHeight()`
**with its parentheses** on a code line, so the census counted the *message* and not the *decision*.

> ⚖️ **A gate cannot honestly scan a file whose MESSAGES quote the thing it counts.**

**Fixed twice over:** the shipped refusal messages now name the predicate **without** its parens (`SC-§41`'s
open-paren discriminator, used the way it is meant to be used — the diagnostic value is kept, the false hit
is not), and the assertion was tightened from `>= 1` to **`== 1`**, which is what holds them to it.

### THE TRANSCRIPT

```
BASELINE (the working tree as it stands)
  BASELINE gates    => GREEN (expected GREEN) OK
  BASELINE re-arm   => GREEN (expected GREEN) OK
  BASELINE alias    => GREEN (expected GREEN) OK
  BASELINE ceiling  => GREEN (expected GREEN) OK

ITEM (6)(a) -- REMOVE THE CanStackHeight() CONSULT FROM EACH GATE, ONE AT A TIME
  MUT A1: gate1 repointed back at CanScaleFootprint() (the EXACT shipped bug)   => RED OK
          gate1 :: consults CanStackHeight( EXACTLY once ....... RED  <== fired
          gate1 :: zero CanScaleFootprint( ..................... RED  <== fired
          gate2, gate3 ......................................... GREEN (isolated)
  MUT A2: gate1 consult DELETED outright                                        => RED OK
  MUT B1: gate2 repointed back at CanScaleFootprint()                           => RED OK
  MUT B2: gate2 consult DELETED outright                                        => RED OK  (was GREEN before the fix above)
  MUT C1: gate3 repointed back at CanScaleFootprint()                           => RED OK
  MUT C2: gate3 consult DELETED outright                                        => RED OK

ITEM (6)(a2) -- THE NAV-LINK RE-ARM'S OWN RED
  MUT D1: ConfigureLadderLink() DELETED from the re-arm override                => RED OK
  MUT D2: the HOOK CALL deleted from ApplyStackUpgrade (override = dead code)   => RED OK
  MUT D3: SC-41 -- call replaced by a code line that MENTIONS without CALLING   => RED OK

ITEM (6)(b) -- THE WRAPPER/ALIAS DEFECT
  MUT E1: CanStackHeight() rewritten as `{ return CanScaleFootprint(); }`       => RED OK

ITEM (6)(c) -- THE PER-CLASS CEILING (source half)
  MUT F1: the resolver goes back to reading GetDefault<ABuilding>()             => RED OK

NEGATIVE CONTROLS -- must stay GREEN, proving the harness is not RED for every edit
  NEG G1: the wheel seam left intact                                            => GREEN OK
  NEG G2: `CanScaleFootprint()` added to gate3 as a COMMENT line                => GREEN OK
  NEG G3: an unrelated function renamed elsewhere in Building.cpp               => GREEN OK

HARNESS VERDICT: every mutation moved the gate in the DIRECTION IT WAS SUPPOSED TO
```

⭐ Each gate is asserted **individually**, so a red names the gate that broke rather than a total.

### ⛔⛔ CAVEAT, DECLARED **ABOVE** THE CLAIM (`TL-§5c` cl. 5(a))

**I did NOT compile and did NOT run the compiled suite** (fence: no compile, no editor). The transcript
above is an **independent shell mirror** of the predicates — it reimplements `CountOccurrencesInCode`,
`ExtractFunctionBody` and `ExtractInlineBody` byte-for-byte against the same files — ⛔ **it is not the
suite**, and it cannot exercise any behavioural row (`(6)(c)`'s per-class walk, the disagreement row, the
ceiling arithmetic). ⛔ **The execution duty transfers by name to the integration row.**

### THE FOUR NEW TESTS

| # | name | what only it can see |
|---|---|---|
| 11 | `CanStackHeightIsASiblingOfCanScaleFootprintAndTheTwoDisagreeOnTheClimbableTower` | the **wrapper** defect — behaviourally (they disagree) and at source (neither body delegates) |
| 12 | `AllThreeStackGatesConsultCanStackHeightAndNoneConsultsTheWheelPredicate` | a gate silently repointed back at the wheel, **per gate** |
| 13 | `TheClimbableTowerReArmsItsLadderLinkOnTheStackUpgradePath` | the re-arm — the **only** automated instrument that can see item (5a) at all |
| 14 | `TheHeightSeriesSaturatesAtTheCeilingItIsHandedRatherThanAtTheBaseClassCDOs` | the ceiling being **obeyed**, at ceilings the project does not ship |

And test **10 was INVERTED**, not deleted:
`AClimbableTowerRefusesTheUpgradeAtTheBuildingItselfNotOnlyInThePlacementPath` →
`AClimbableTowerAcceptsTheHeightUpgradeAndSaturatesAtItsOwnPerClassCeiling`.

> ### ⭐⭐ THE FINDING THAT DESERVES ITS OWN LINE
> **Two tests asserted the defect and were GREEN for the entire time Jonathan could not stack a tower** —
> this one and `SiegePlacementTest`'s test 12. ⚖️ *A passing suite is a statement about the code, never
> about the design being right.* Both are kept and inverted, with the history written into their banners,
> so the next reader sees the refusal was **deliberate and was reversed by a measurement** rather than
> thinking it was never there.
>
> ⚠️ And a corollary I had to act on: **when a test's polarity flips, its anti-fake argument has to be
> re-derived, not re-signed.** `SiegePlacementTest` test 12's old red control *was* the climbable tower;
> the new (a) needs a different partner, so (b)/(f) — the fixtures the resolver must **still** turn away —
> now carry that weight, and the file's own "a resolver that answers Ready for everything fails …" ledger
> was rewritten to match.

---

## 7. ✅ ITEM (5) — **ZERO TINT EDITS**, CONFIRMED ON THE DIFF

⛔ `UpgradeGhostColor` · ⛔ `ValidGhostColor` · ⛔ `InvalidGhostColor` · ⛔ the ternary at the ghost-tint site
· ⛔ `M_Ghost` · ⛔ `GhostColor` — **not one of them appears in this diff.** No second source of truth for
the tint was created. The blue ships because gate (5) now returns `Ready`, exactly as `STACK-§9` said it
would.

---

## 8. ⭐ RIDER — `STACK-§9`(2) refusal-vocabulary

> ### ✅ **DONE. Not dropped.**

Both branches of `ConfirmStackUpgrade` that reused *"That building cannot be stacked"* now have their own
`NSLOCTEXT` keys in the shipped `RefuseCardPlay` vocabulary:

| branch | new key | new sentence |
|---|---|---|
| target died between the ghost frame and the click | `CardRefused_StackTargetGone` | *"That building is gone"* |
| M8 authority guard after a refunded spend | `CardRefused_StackUpgradeFailed` | *"The upgrade could not be applied"* |

⚖️ Both old sentences were **false**, and each in its own way: the first told the player a **permanent
rule** about a building whose only problem was **transient**; the second implied they had picked a bad
target when nothing they did or could see produced it. ⛔ Neither new line promises the refund in words the
other refusals do not — every refusal here is net-zero, so saying it once would imply the others are not.

⛔ **This ledger line is separate and non-blocking and it cannot change this row's verdict** (`SC-§29`).

---

## 9. ⚠️⚠️ THINGS QA SHOULD SCRUTINISE — INCLUDING TWO I AM HANDING OVER RATHER THAN FIXING

### (a) ⛔ TWO FILES OUTSIDE THE ROW'S NAMED PATHSPEC — **FORCED, NOT CHOSEN**

| file | why it was unavoidable |
|---|---|
| `ClimbableTower.cpp` | the per-class ceiling must be set **in the constructor** (item (4)) and the re-arm override needs a definition. The row named `ClimbableTower.h` only. |
| `Tests/SiegePlacementTest.cpp` | its test 12 **asserted the defect** and its test 14 calls `StackHeightMultiplier`. Leaving either would ship a **red suite**. |

⛔ **`TASK-944`'s hand-named pathspec must take both, or the commit is incomplete.** ⚠️ Its list currently
names neither. **The git root is one level above the `.uproject`, so repo-relative pathspecs carry a
`GitClaudeUnrealTest/` prefix** — I confirmed this the hard way by using the prefix on `git show` and
getting real content back rather than the silent empty result `SC-§56` describes.

### (b) 📌 A FINDING THE ROW DID NOT ASK FOR — **THE IN-GAME HELP NOW TEACHES A FALSE RULE**

`SiegeControlsHelpWidget.cpp`'s `Cards.StackUpgrade` row tells the player, on screen:

> *"ONE KIND OF BUILDING REFUSES TO BE STACKED, AND IT IS THE ONE YOU CAN CLIMB. … Hovering one shows RED
> with \"That building cannot be stacked\"."*

⛔ **That is now false**, and its citation block also names `CanScaleFootprint()` as the gate the shipped
path asks. ⚠️ **No test goes red** — `SiegeControlsHelpTest` pins the row's existence, category and key,
never its prose — so this would ship silently, which is precisely `SC-§50`'s orphan shape.

⛔ **I did NOT edit it**, and the reason is not timidity: it is out of the row's named files, and the
replacement text is a **player-facing UX decision** (does the help now teach the ×2 ceiling? name the
climb reason at all?) that belongs to the manager, not to a refactor. ⇒ ⭐ **recommending a manager row**,
and declaring it here so it is a named obligation rather than a footnote. ⚖️ *Shipping a help screen that
says the tower cannot be stacked while it can is the mirror image of the mistake this whole batch exists
to correct.*

### (c) ⚠️ A HAZARD THE PER-CLASS SHAPE **OPENS**, DECLARED RATHER THAN DEFENDED WITH A SECOND LITERAL

`MaxStackHeightMultiplier` stays `EditDefaultsOnly`, so a **Blueprint child can now RAISE it and be
OBEYED** — which was not true before (the base-CDO read discarded it). For an ordinary building that is a
balance knob; for a climbable one it re-opens the deck-unreachable state.

⛔ **I did not add a clamp**, because a clamp would be a **second copy of the ceiling** and item (4) makes
that an automatic fail. It is declared in the tunable's own comment (`HIGH-§1`) and again in
`AClimbableTower`'s constructor: *the ceiling is derived from `STACK-§10`'s arithmetic, never bumped by
hand and never raised in a `.uasset`.*

✅ **And I measured that no override exists today**, read-only, with controls — `TASK-956`'s FName-table
technique, re-derived here:

```
BP_Building_WatchTower.uasset   entries=177
   MaxStackHeightMultiplier  ABSENT   (no delta saved)   <== the ctor's 2 governs
   RelativeScale3D           ABSENT   (corroborates TASK-956's live MCP read of (1,1,1))
   CTRL+ VisualMesh / StaticMesh / CardID   all present  (the probe is not blind)
   CTRL- ZzNoSuchPropertyZz                 absent       (the probe does not fabricate)
BP_Building_ArrowTower.uasset   entries=164   -- same result, fleet-consistent
```

⭐ Two instruments with nothing in common now agree on that asset: a headless byte probe and `TASK-956`'s
live MCP read. ⛔ **Zero `Content/**` writes** — every access was `rb`.

### (d) THINGS I DELIBERATELY DID NOT TOUCH

⛔ The wheel and its `1.5` cap · ⛔ the two series (`STACK-§1`) · ⛔ the `NotStackable` distinct tint
(`STACK-§9`(1), conditionally deferred — and note it is now **unreachable in normal play**, which is
exactly the condition under which it was deferred) · ⛔ any mesh, socket, clip or material · ⛔ the
`EPlacementUpgradeState::NotStackable` enum value itself, which is **kept** and documented as the state a
future outright-refusing building would land in.

### (e) CONCURRENT-LANE HYGIENE — MEASURED, NOT ASSUMED

`git status --porcelain -- Source/` shows `SiegeCardRosterTest.cpp` modified and `SiegeCardArtRosterTest.cpp`
untracked. ⛔ **Neither is mine** — they are `TASK-964`'s and `TASK-957`'s. I opened neither for writing.
⛔ I added **no** sixth path-composer copy (`TASK-959`'s row is untouched). `Content/FogArea/` is untracked
and predates my session — already declared by `TASK-956` §6(2).

⚠️ **`SC-§55`:** my session-start `gitStatus` snapshot was stale; every statement above comes from a `git
status` I ran myself at the end of this row.

---

## 10. FILES TOUCHED

```
Source/GitClaudeUnrealTest/Siegebound/Building.h                       (+ CanStackHeight, + GetMaxStackHeightMultiplier,
                                                                       + OnStackUpgradeApplied, amended static signature,
                                                                       rewritten cap + health-step comments)
Source/GitClaudeUnrealTest/Siegebound/Building.cpp                    (resolver takes the cap; gate -> CanStackHeight;
                                                                       + empty hook; + juice-trap comment; false comment fixed)
Source/GitClaudeUnrealTest/Siegebound/ClimbableTower.h                (+ CanStackHeight override, + OnStackUpgradeApplied,
                                                                       CanScaleFootprint doc scoped to the WHEEL)
Source/GitClaudeUnrealTest/Siegebound/ClimbableTower.cpp              (+ MaxStackHeightMultiplier = 2 in the ctor,
                                                                       + the ConfigureLadderLink re-arm)
Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.h         (docs; + two refusal-text declarations)
Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.cpp       (gates 1 + 2 repointed; cap notice takes the target's
                                                                       ceiling; + two refusal texts; log wording)
Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeBuildingStackTest.cpp (10 -> 14 tests; test 10 inverted)
Source/GitClaudeUnrealTest/Siegebound/Tests/SiegePlacementTest.cpp     (test 12 inverted; test 14 signature; banner ledger)
```

**Assets referenced:** none written. Read-only byte probes of
`/Game/Blueprints/Buildings/BP_Building_WatchTower` and `BP_Building_ArrowTower` (control).

**Instruments (scratch, ⛔ not in the repo):** `probe.py` (predicate mirror + controls), `red.py`
(`SHIP-§9` mutation harness), `fname.py` (FName-table probe), `balance.py` (brace/paren balance across all
eight edited files — all `+0/+0`).
