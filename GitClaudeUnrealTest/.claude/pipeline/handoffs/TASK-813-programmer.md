# TASK-813 — [ST-2] THE BLUE UPGRADE STATE + THE CONFIRM PATH

**Agent:** gameplay-programmer · **Status → `ready-for-qa`** · 2026-09-02
**Law:** `STACK-§0` (what already ships) · `STACK-§2` (the exclusion) · `STACK-§3` (the muzzle finding) · `STACK-§5` (`J-2`/`J-5`/`J-6`/`J-7`/`J-8`/`J-9`/`J-10`) · `STACK-§7` (names + M8) · `SC-§33` · `SC-§37`
**Consumes:** TASK-812 (the series + the predicate + `ApplyStackUpgrade`) · TASK-735 (the footprint placement frame)
**Gate:** TASK-814 (QA) · **Compile:** TASK-816 owns Lane B's only one — ⛔ nothing compiled here.

---

## 1. FILES TOUCHED — three, and they are the three the fence names

| File | Change |
|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.h` | +1 fwd decl · +1 public nested enum · +3 public pure statics · +1 `EditDefaultsOnly` colour · +1 `EPlacementInvalidReason` value · +1 private method · +2 private members |
| `Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.cpp` | the additive ghost block · the ternary on the shipped MID line · one new `case` · one new early-return branch · `ConfirmStackUpgrade()` · the three statics · 2 reset lines |
| `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegePlacementTest.cpp` | **EXTENDED** — tests 11..17, ⛔ no second placement frame |

⛔ **Nothing else opened for edit.** `Building.{h,cpp}` · `ClimbableTower.*` · every WidgetBlueprint · `cards.csv` · `M_Ghost` · any `TOWER-§8.*` symbol (no socket, no climb line, no rung plane, no standoff) · `HIGH-§` — untouched. ⛔ No compile, ⛔ no editor, ⛔ no MCP, ⛔ no Git. Airlock: ⛔ no `Capture()`/`EnsureSnapshot()`, Zone A untouched, 🔒 the 552 latch unspent, ⛔ no token figure.

⚠️ `git diff --stat` on the two controller files reports **1,069 insertions** — that is **TASK-735's diff plus mine**, both uncommitted in the same working tree. Mine is the symbol list above.

---

## 2. ⭐ THE THREE-STATE GHOST LOGIC — and how little of it is new

**`STACK-§0` was right and it saved the expensive half of this task.** The ghost actor, the per-frame cursor trace, the five validity gates, the `M_Ghost` MID and the `"GhostColor"` parameter all already ship. ⇒ **blue is a third `FLinearColor` written by the SAME line into the SAME parameter.** ⛔ No new material, ⛔ no new MID, ⛔ no second ghost actor, ⛔ no new render path, ⛔ no second trace.

### The decision — one pure static, six values in, one state out

```cpp
static EPlacementUpgradeState ASiegePlayerController::ResolvePlacementUpgradeState(
    const ABuilding* HoveredBuilding, ETeamId OwnTeam, FName PendingCard,
    bool bPendingCardIsBuilding, int32 CurrentGold, int32 UpgradeCost);
```

`enum class EPlacementUpgradeState : uint8 { None, Ready, NotStackable, Unaffordable }` — **public nested**, unlike `EPlacementInvalidReason`, because the resolver *is* the feature and a private return type makes it uncallable from a test. It follows TASK-735's four pinned pure seams exactly: plain C++ static, ⛔ not a `UFUNCTION`, ⛔ no world, ⛔ no member state, ⛔ no parameter defaulted (`SC-§33`).

**Decision order, every step a ruling:**

| # | Condition | Answer | Why |
|---|---|---|---|
| 1 | not a Building card / `NAME_None` | `None` | `J-8` — ⛔ not units, ⛔ not spells. Reuses the SAME `bPendingIsBuilding` flag that exempts unit cards from the slope/obstacle/clearance/footprint gates — one source of truth, not a second opinion |
| 2 | nothing hovered, not an `ABuilding`, or destroyed | `None` | `ACastle` is class-disjoint from `ABuilding` (the fact `HasBuildingClearance` is built on) ⇒ the castle can never be a target |
| 3 | ⛔ **ENEMY** building | `None` | `J-7` — and ⭐ **no new refusal is invented**: `None` hands the frame back to the shipped gates, which already refuse enemy ground on §3.5 clearance |
| 4 | a **different** `CardID` | `None` | spec (2) — an ArrowTower in hand does not grow a BombTower |
| 5 | ⛔ `!CanScaleFootprint()` | `NotStackable` (RED) | `STACK-§2` — ⛔ **structural**, ⛔ never a `CardID` compare |
| 6 | `CurrentGold < UpgradeCost` | `Unaffordable` (RED) | `J-5` |
| 7 | otherwise | ⭐ `Ready` (**BLUE**) | |

⚠️ **5 is asked BEFORE 6 deliberately** — "this building can never be stacked" is a permanent, actionable truth; "you are 4 gold short" would be a misleading thing to say about a building that refuses at any price. Asserted, not just stated (test 12(e)).

⭐ **The height cap is deliberately absent from that list** — see ruling 3 below.

### Where it plugs in — ⛔ ZERO shipped gate lines edited

The block is **purely additive** and sits **after** `bPlacementValid = bValid;` in `UpdatePlacementGhost`, overriding the verdict rather than joining the chain:

- **CORRECTNESS:** the five gates answer *"may a NEW building stand at this point"*. A blue click puts no new building anywhere, and hovering an existing building fails the §3.5 clearance gate **by construction** (the distance to it is ~0) — so composing with them could only ever refuse the very state being added.
- **DIFF SIZE:** ⛔ not one of TASK-735's gate lines is edited or re-indented. That was TASK-735's explicit request and it is what TASK-815 needs next.

The hover target is `Cast<ABuilding>(Hit.GetActor())` off **the trace the ghost already ran** — buildings root a `BlockAll` `VisualMesh` (`Building.h:43-51`) so they answer the Visibility trace, and the ghost's own collision is fully disabled. ⛔ No second trace, ⛔ no sweep, ⛔ no proximity search.

**The colour line, as shipped:**
```cpp
const FLinearColor ActiveGhostColor =
    (PlacementUpgradeState == EPlacementUpgradeState::Ready)
        ? UpgradeGhostColor
        : (bPlacementValid ? ValidGhostColor : InvalidGhostColor);
GhostMID->SetVectorParameterValue(GhostColorParamName, ActiveGhostColor);
```
`UpgradeGhostColor = (0, 0.4, 1)`, `EditDefaultsOnly`, with its `HIGH-§1` consequence written beside it. A **third hue**, not a third shade — asserted (test 15(b)).

---

## 3. ⭐ HOW I CALL `ApplyStackUpgrade` — and what I would need if its height half moves

`ConfirmStackUpgrade(ASiegePlayerState&)` is a small private helper called from **one place**: `TryConfirmPlacement`, immediately after the `SiegeState` null-check and **before** the miner cap, the BP-class resolve and both spawn branches — because ⛔ nothing on this path spawns, so those rules are not skipped, they are *inapplicable*.

**The order, which is the shipped confirm's order rather than a new one:**
1. re-validate the weak target (`IsValid` + `!IsBuildingDestroyed`) — ⛔ **not** belt-and-braces: a building can die between the frame that painted blue and the click;
2. re-ask `CanScaleFootprint()`;
3. `SpendGold(PendingCost)` — gold last before commit, the shipped discipline;
4. ⭐ **one call:** `Target->ApplyStackUpgrade()`;
5. the `J-6` cap note;
6. the card leaves the hand (`ConfirmPlayFromHand`), then `ExitPlacementMode()`.

⛔ **NO ARITHMETIC HAPPENS IN MY FILE.** The two series, the cap, the `MaxHP`/`CurrentHP` delta and the `StackUpgradeCount` increment are all TASK-812's, because those three members are **private** on `ABuilding` and are meant to stay so. I read exactly two things off the building — `GetStackUpgradeCount()` and the two public series statics — and only to decide whether the cap note fires and what to log.

**⛔ Net-zero on failure (§3.0).** `ApplyStackUpgrade` cannot be un-applied (`StackUpgradeCount` has no decrement, by design), so gold is spent **before** it and a `false` return **refunds through `AddGold`** with a `Warning`. Today the only uncovered refusal inside it is the `!HasAuthority()` guard.

### ⚠️ WHAT I WOULD NEED IF QA STRIPS THE HEIGHT HALF (TASK-812 §4/§9.1)

**My call site survives it with zero edits.** I call one bool-returning mutator and never touch the mesh, the scale or the baseline. If QA rules the height half out of `ApplyStackUpgrade`, I need **exactly one thing**, and it is not a change to my code:

> ⭐ **A second `ABuilding` mutator that applies the height, callable with no arguments and returning `bool`** — e.g. `bool ApplyStackHeightScale();` — so `ConfirmStackUpgrade` becomes two calls instead of one.

⛔ **What I must NOT be asked to do instead:** reach `VisualMesh` (protected), read or store the authored baseline Z, or multiply a scale in the placement path. All three would put the height series in two places, and the "recompute from the baseline, never multiply in place" property that lets the cap land *exactly* would then depend on the controller getting it right too. **I would rather the height half stay where it is; if it moves, it needs a seam, not a caller rewrite.**

📌 Also load-bearing for that ruling: **my cap note reads the series, not the mesh.** `StackHeightMultiplier(after) <= StackHeightMultiplier(before)` is true iff the cap bit — it keeps working whether the height is applied inside `ApplyStackUpgrade` or beside it.

---

## 4. ⚖️ THE FOUR RULINGS — all proceeding defaults, ⛔ none blocked

| # | Question | ⚖️ RULED | Reasoning |
|---|---|---|---|
| **R-1** | does BLUE require enough gold? | ✅ **YES — unaffordable ⇒ RED + the shipped "Not enough gold"** | `J-5`'s second half and the codebase's own standard (`:2946-2947`: *"what the player sees is what the click does"*). A blue that refuses would be the first place that stopped being true. ⚠️ `J-5`'s bare "NO" reads as answering *"may blue appear regardless of gold"* — both halves of the row agree once the sentence after it is read, and I built the sentence. |
| **R-2** | do **ENEMY** towers show blue? | ✅ **NO — `None`, and ⛔ no new refusal is invented for them** | `J-7`. Returning `None` (not a refusal state) means the enemy case keeps the message it has had for months — the §3.5 clearance line — at a cost of ⛔ zero new code and ⛔ zero new vocabulary. |
| **R-3** | what shows at the **height cap**? | ✅ **STILL BLUE, the click still succeeds, and a one-line HUD note fires ⛔ at confirm** | `J-6`, his own sentence. The colour ⛔ does not change (a silent recolour at ×5 is a bug report); the note is `"Maximum height reached - the upgrade added health only"` — it says **health**, ⛔ never "taller", so the feedback cannot claim a height gain it will not deliver. ⚠️ **At confirm, ⛔ not per frame** — a ghost-driven note would spam the HUD for as long as the cursor rested on a maxed tower. The condition is derived from the series, so the number 5 appears nowhere. |
| **R-4** | a **non-stackable** hovered building? | ✅ **NOT-A-TARGET, ⛔ not an error** — plain shipped RED + `"That building cannot be stacked"` on the click | `STACK-§2` demands a visible reason (*"a feature that quietly does nothing on one card is a bug report waiting to happen; one that says why is a design"*). "Not an error" is honoured mechanically: **`Log` severity, ⛔ never `Warning`/`Error`**, the shipped `RefuseCardPlay` vocabulary, the player **stays in placement mode**, ⛔ no gold moves, and ⛔ no exceptional path anywhere. The wording ⛔ **never names "WatchTower"** — the exclusion is structural, so the next climbable building inherits the message unchanged (asserted, test 16(c)). |

### 🧑 THE ONE THING FOR JONATHAN — the upgrade **cost**, and a number worth his eye

⛔ **He never specified an upgrade cost.** Proceeding default per `J-2`: **the card's own `DT_Cards` `Cost`, ⛔ never a literal** — so retuning the card retunes the upgrade with no code change.

⚠️ **Measured from `Docs/Data/cards.csv`, because "presume the card's 30g" points at the wrong card:**

| Card | Cost | Stackable? |
|---|---|---|
| `ArrowTower` | **15** | ✅ |
| `BallistaTower` | **21** | ✅ |
| `BombTower` | **24** | ✅ |
| `CrystalTower` | **27** | ✅ |
| `WatchTower` | **30** | ⛔ **NO** (`J-9`) |

⇒ ⭐ **30 gold is the WatchTower's — the one card that can never be upgraded.** The towers he can stack cost **15–27**, and an ArrowTower stack therefore prices at **15 per upgrade**. 🧑 If he wants an upgrade to cost a *fraction* or a *multiple* of the card, that is a one-line change at the `ResolvePlacementUpgradeState` call site plus the same expression in `ConfirmStackUpgrade`'s `SpendGold` — but it would be a **new lever he did not ask for**, so I did not invent one.

---

## 5. ✅ SPEC (6) — THE PROJECTILE MUZZLE, MEASURED FROM THE SOURCE (`STACK-§3`)

> **⛔ NO TOWER'S PROJECTILE SPAWN POINT DERIVES FROM A MESH SOCKET. ⇒ a `Z` scale moves the muzzle ⛔ not at all, and range is unchanged.**

**The two lines, quoted:**
```
Tower.cpp:318   const FVector MuzzleLocation = GetActorLocation() + MuzzleOffset;
Tower.h:103     FVector MuzzleOffset = FVector(0.f, 0.f, 200.f);
```

- It is an **actor-relative constant offset**, ⛔ not `GetSocketLocation`, ⛔ not a component transform. A `Z` scale on `VisualMesh` moves **no actor origin** and **no `UPROPERTY` vector** ⇒ a ×5 ArrowTower fires from exactly the same world point it does today.
- ⭐ **And it would not matter if it did:** `Tower.cpp:315-317`'s own comment — *"Purely cosmetic — the projectile re-aims at the target every tick (TASK-026 homing), so where it starts never changes what it hits."*
- **Project-wide socket audit:** `GetSocketLocation` / `DoesSocketExist` appear in exactly **two** files — `ClimbableTower.cpp` (the `TOWER-§8.3` ladder sockets) and `HeroCharacter.cpp`. ⛔ **Zero** socket reads in `Tower.cpp`, `Projectile.cpp` or `Building.cpp`.
- ⇒ **Nothing to fix, and I fixed nothing.** ✅ And the `ClimbableTower` sockets — the one place a scale *would* bite — are exactly what `CanScaleFootprint() == false` protects.

**Spec (7), verified rather than re-derived:** `HIGH-§4`'s selector is `bRanged && CardType == Unit` ⇒ a ×5 ArrowTower gains ⛔ zero damage from height (⛔ no `HIGH-§` edit). Collision + navmesh scale for free via `Building.h:43-51`.

**Also checked and found to need nothing:** the card-play sound (`SiegePlayerController.cpp:970`) fires in `PlayHandSlot`, ⛔ not at confirm — so the upgrade path inherits it with no new audio hook.

---

## 6. ⚠️ WHAT `TASK-815` MUST KNOW ABOUT MY DIFF

It serializes behind me on the same two files. **Everything, in one place:**

1. **⛔ I DID NOT REFACTOR THE `TryConfirmPlacement` SWITCH.** One `case EPlacementInvalidReason::Upgrade:` added **in place**, directly after TASK-735's `Units` case and before `default:`. **⛔ Nothing else in that switch is touched.**
2. **⭐ `EPlacementInvalidReason` STAYS `private`, and `Upgrade` is appended LAST — after `Units`.** **Append yours after `Upgrade`**; ⛔ do not insert in the middle. There is a test that fails if anyone does (test 17(b) pins `Clearance → Units → Upgrade`).
3. **⛔ I DID NOT EDIT ONE LINE OF THE GATE CHAIN** in `UpdatePlacementGhost` (`:2400-2432`). My block is **purely additive**, placed *after* `bPlacementValid = bValid;`. Your wheel poll goes in `PlayerTick`'s placement branch and never meets it.
4. **⭐⭐ THE GHOST SCALE IS STILL ENTIRELY YOURS. I set ⛔ no scale anywhere.** `UpdatePlacementGhost` still moves the ghost by `SetActorLocation` only, and TASK-735's `CalcBounds(GetComponentTransform())` read is untouched — so scaling the ghost component still makes the footprint follow with zero edits, exactly as 735 promised you.
5. **⭐ The upgrade inherits the wheel's X/Y verbatim** (`J-4`). `ApplyStackUpgrade` scales **Z only**; if the player wheels the ghost and then upgrades an existing building, the *existing* building's X/Y are unchanged — the wheel sizes what you **place**, not what you **grow**. That is his sentence ("keeping the same width and length"), and it needs ⛔ no coordination between us.
6. **New private members:** `PlacementUpgradeState` and `PlacementUpgradeTarget` (weak), declared after `PlacementInvalidReason`, reset in **both** `EnterPlacementMode` (beside `bWarnedNoFootprintBounds`) and `ExitPlacementMode` (beside `bPendingIsBuilding`). ⚠️ **If your wheel adds session state, reset it in the same two places** — that is now the pattern in both functions.
7. **New public statics** (plain C++, ⛔ not `UFUNCTION`s), declared as one block **after** TASK-735's four: `ResolvePlacementUpgradeState` · `StackNotStackableRefusalText` · `StackHeightCapNoticeText`. Plus the public nested `EPlacementUpgradeState`.
8. **New `EditDefaultsOnly`:** `UpgradeGhostColor`, in the `Siegebound|Placement` block after `InvalidGhostColor`. **⛔ Your three wheel tunables go after it** — and ⛔ never reuse `GroupRadiusWheelStep/Min/Max` (`MARK-§4`).
9. **⭐ `Tests/SiegePlacementTest.cpp` now holds 17 tests** (735's 1–10, mine 11–17) in **two** fixture namespaces — `SiegePlacementTestFixture` (735's, untouched) and `SiegePlacementUpgradeFixture` (mine). **Add an 18th; ⛔ do not add a third file.** My fixture already gives you `MakeScratchBuilding<T>(Team, CardID)`, `TryReadShippedColor`, `LoadProjectSource` and `CountOccurrencesInCode`.

---

## 7. TESTS — `SC-§37`. **MY DELTA: `+7`.**

⚠️⚠️ **⛔ I AM NOT ASSERTING AN ABSOLUTE — the baseline moved under me exactly as briefed.** Measured just now with `grep -rh "^IMPLEMENT_.*_AUTOMATION_TEST(" | wc -l` across **26 files**: **331 → 338**. The 331 already contained TASK-807 (+5), TASK-735 (+10) and TASK-812 (+10). ⭐ **My contribution is unambiguously `+7`; TASK-815 will move it again, and whoever declares the number last owns it.**

**Frame check (⛔ "check first"):** `Tests/SiegePlacementTest.cpp` **exists and is the placement frame** — TASK-735 named it for the mechanic precisely so this task would land in it. ⭐ **EXTENDED, ⛔ no second placement file.**

| # | Test | ⭐ What it would CATCH |
|---|---|---|
| 11 | `TheUpgradeStateIsBlueOnlyForAnOwnTeamSameCardScalableBuilding` | blue on an enemy tower · blue on a different card · blue while holding a **unit** card · blue on a **corpse** · **blue on nothing at all** — and the fixture self-checks that team+CardID were actually planted, so a row cannot pass by comparing a building to itself |
| 12 | `AClimbableTowerNeverYieldsTheBlueUpgradeStateAndTheTowerFamilyStillDoes` | ⛔ **a stackable WatchTower** (the `TOWER-§8.5a` void) · a **shadowed non-virtual** `CanScaleFootprint` (asked through an `ABuilding*`) · an over-broad exclusion that also caught the Arrow/Bomb/Ballista/Crystal family · the ordering slip that says "not enough gold" about a tower that refuses at any price |
| 13 | `InsufficientGoldYieldsRedNotBlueAndTheBoundaryTracksTheCardsOwnCost` | ⛔ **a blue that refuses** · a `<=`/`<` slip that eats the player's exact change · ⭐ **a hardcoded price** — three different costs each flip at their own boundary, so 15/24/27/30/37 all die · a gold check placed before the team check |
| 14 | `AtTheHeightCapTheStateIsStillBlueAndTheClickStillBuysHealth` | a **refusal at the cap** (`J-6` violated) · a cap that also freezes health or the count · ⭐ a cap note that would fire on a **first** upgrade. ⛔ The number 5 appears nowhere — the cap is **walked** until the series stops moving |
| 15 | `TheUpgradeGhostColourIsAThirdDistinctValueOnTheShippedGhostParameter` | ⭐ **blue that is actually green** (invisible third state) · a third *shade* rather than a third *hue* · ⛔ a quiet retint of the shipped green/red · a deleted `UpgradeGhostColor` (the existence check **guards** the value checks) |
| 16 | `TheUpgradeRefusalAndCapNoticeAreTheirOwnLinesAndNeverNameTheExcludedCard` | ⛔ reusing "Too close to another building" for a hover the player made on purpose · a **success** line that reads as a **refusal** · ⛔ **"WatchTower" leaking into player-facing text** (the name check, moved into a string) · a cap note that does not say what the click bought |
| 17 | `ThePlacementPathNamesNoCardAndTheNewReasonIsAppendedAfterUnits` | ⛔⛔ **`STACK-§2`'s automatic fail** — a `CardID`/class-name compare on the placement path (code lines only; prose skipped) · an enum value **inserted** rather than appended · a `case` that falls into `Clearance` |

### ⛔⛔ EVERY ASSERTION CAN FAIL — and the trap you named is guarded explicitly

> ⚠️ *"A 'shows blue' test passes trivially if nothing was hovered."*

**Every claim moves exactly ONE term of a fixed fixture and pins both sides:**
- ⭐ **the control that must come out GREEN** (⇒ `None`, the shipped path decides): **nothing hovered** (11b) · a unit card (11e) · an enemy building (11c) · a different card (11d) · a destroyed building (11g) · an enemy climbable tower (12f);
- ⭐ **the control that must come out RED**: a climbable tower ⇒ `NotStackable` (12a) · one gold short ⇒ `Unaffordable` (13a);
- ⭐ **and every RED claim is paired with the SAME fixture coming out BLUE** when the one term under test is restored: 12(b) the identical call on a plain `ABuilding`, 13(b) exact change.

⇒ **A resolver that answers `None` for everything fails 11(a), 12(b), 12(c), 13(b), 14(a)/(b). A resolver that answers `Ready` for everything fails 11(b)..(g), 12(a), 12(e), 12(f), 13(a), 13(e). ⛔ Neither can pass this file.**

**Instruments are self-checked before they are trusted:** the fixture's team/CardID are proven planted before any row about them · `bDestroyed`'s seed is proven to have taken · the cap **walk** is proven to have taken ≥ 2 upgrades and to have stopped on the cap rather than the loop guard · the colour reader is proven to return **false** for a property that does not exist · `Contains` is proven to find a word that IS present before four absences are claimed · and the source scanner is proven to find `WatchTower` **in prose** before it reports **zero on code lines** — the one way test 17 could report SAFE while the name check had been added.

⛔ **No expectation is transcribed from the spec.** The gold boundaries are `cost ± 1` for three costs, the cap is walked rather than numbered, the colours are read off the CDO by reflection, and the fixture's card names and cost (37) match ⛔ nothing in `cards.csv`.

---

## 8. 📌 M8 DECLARATION (`STACK-§7`) — explicit, ⛔ not boilerplate

- **Everything this task adds is client-local PRE-gate presentation:** the hover cast, `PlacementUpgradeState`, `PlacementUpgradeTarget`, the colour. ⛔ **No replicated property, ⛔ no RPC, ⛔ no relevancy change.**
- **The one authoritative write is ⛔ not mine.** `StackUpgradeCount` is authoritative game state (it drives `MaxHP`) and is written **only** inside `ABuilding::ApplyStackUpgrade`, which refuses on `!HasAuthority()`. This controller **asks**; the building **decides**.
- The resulting HP rides the **already-shipped** `OnHPChanged` push. ⛔ No second push, ⛔ no direct widget call.

---

## 9. 🔍 WHAT QA SHOULD SCRUTINISE HARDEST

1. **⭐⭐ THE OVERRIDE PLACEMENT.** My block runs **after** the gate chain and overwrites `bPlacementValid` / `PlacementInvalidReason`. Confirm that is what you want versus wrapping the chain in an `else`. My argument is in §2 (correctness + a zero-line diff for TASK-815); the cost is that five side-effect-free gates run and are then discarded in the upgrade state.
2. **The `Ready` ⇒ `bPlacementValid = true` coupling.** It is what makes the confirm reach my branch. Verify nothing else downstream reads `bPlacementValid` and assumes "a new building may stand at `PlacementLocation`". I traced `TryConfirmPlacement` as the only reader.
3. **The spend-then-refund order in `ConfirmStackUpgrade`.** `ApplyStackUpgrade` cannot be un-applied, so gold moves first and `AddGold` refunds on `false`. The alternative (duplicate its preconditions before spending) would put the authority check in two places.
4. **⭐ The `J-5` reading.** `STACK-§5`'s `J-5` cell literally says "NO" and then describes the RED behaviour. I built **blue requires gold**. If you read the row the other way, say so — it is a one-line deletion in the resolver.
5. **The card is consumed by an upgrade.** Gold moves and the hand slot is spent, exactly as a placement. Implied by `J-2` + the card-leaves-hand-at-CONFIRM law, but ⛔ he never said it in so many words.
6. **`ConfirmStackUpgrade` is untested headlessly** (it needs a live `ASiegePlayerState` and a `UDeckComponent`). Test 14 covers the two facts it is built on. Its real instruments are your diff read and Jonathan's playtest.
7. **The public nested `EPlacementUpgradeState`.** TASK-735 deliberately kept `EPlacementInvalidReason` private; I made the **new** enum public so the resolver is callable from a test. Rule on it — if you want it private, the five state assertions in tests 11–14 go with it.

## 10. ⚠️ DECLARED, ⛔ NOT FIXED (out of fence)

- **The ghost still renders at the cursor's surface point while blue** — i.e. sitting on the hovered tower, tinted blue. That is his "the outline instead appears blue"; ⛔ I did not move, hide or re-pose it.
- **TASK-735's three declared misleading survivors** (point-slope gate under a wide ghost, point spawn-box test, the other building's footprint as a point) are unchanged and still theirs.
- **`J-11`'s ×5 UV stretch** is real, shipped as-is and named. ⛔ No task, ⛔ no re-author.
- **`OnCard1Pressed`'s empty-slot Footman fallback** (TASK-807's item 5) is still declared-not-fixed and is not mine either.
