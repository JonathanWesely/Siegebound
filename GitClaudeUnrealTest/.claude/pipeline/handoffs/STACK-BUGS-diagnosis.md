# STACK-BUGS — diagnosis of Jonathan's two playtest reports

**Author:** gameplay-programmer · **Date:** 2026-09-03 · **Mode:** DIAGNOSE ONLY — ⛔ no source, tests, board rows, compile, editor, MCP or Git writes were touched. Read-only `git log`/`show` only.

> *"the first bug is that for some reason I am not able to stack towers on top of each other in the way that I described that you should be able to, instead I just get "building cannot be stacked", secondly, I get a strange error that says "card actor unavailable"."*

---

## ⭐ VERDICT IN ONE PARAGRAPH

**Two defects, two independent root causes. They are NOT one bug.** Bug 1 is the **WatchTower**, excluded from stacking by `CanScaleFootprint()` — the orchestrator's hypothesis 1 **measures TRUE**, with a correction: the stack gate does not consult an *adjacent* predicate, it consults the **identical virtual**, deliberately, at three independent sites. Bug 2 is **`BP_Unit_Witch.uasset` — it does not exist and never has**, while her DT_Cards row shipped live in `1aa0fee`. The orchestrator's hypothesis 2 ("a missing or unresolvable Blueprint") is **correct in substance but wrong in attribution**: it is **not** a TASK-815 regression, TASK-815 does not touch this path for unit cards at all, and it is **not** "once per session, building cards only" — it fires on **every confirm click** of the Witch card.

⭐ **On Jonathan's correction (the blue is a THIRD TINT ON THE GHOST, not an outline on the target): the shipped code already does exactly that, three-state, from ONE resolver — see §5. The blue state is implemented and works; it simply does not apply to the WatchTower.** ⇒ **"no blue" and "cannot be stacked" ARE the same defect** — two faces of one decision, `AClimbableTower::CanScaleFootprint() == false`. Fix that one predicate and the blue appears *and* the click works. ⛔ **But that unification does NOT extend to bug 2** (§3).

---

## 1. `"That building cannot be stacked"` — located, every door enumerated

**The string:** `ASiegePlayerController::StackNotStackableRefusalText()`
`Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.cpp:5272-5281`
`NSLOCTEXT("Siegebound", "CardRefused_NotStackable", "That building cannot be stacked")`

### 1.1 All four call sites (⛔ not just the likely one)

| # | Site | Condition that reaches it | Can Jonathan hit it in normal play? |
|---|------|---------------------------|-------------------------------------|
| A | `TryConfirmPlacement` **:2260** | `PlacementInvalidReason == Upgrade` **and** `PlacementUpgradeState != Unaffordable` ⇒ `NotStackable` | ✅ **YES — this is the one he hit** |
| B | `ConfirmStackUpgrade` **:2484** | target `!IsValid` **or** `IsBuildingDestroyed()` — the building **died between the ghost frame and the click** | Rare race; ⚠️ **wrong wording** (see 5.3) |
| C | `ConfirmStackUpgrade` **:2499** | `!Target->CanScaleFootprint()` re-asked at confirm | Unreachable in practice — requires `Ready` at hover (predicate true) and false at click; the predicate is class-fixed |
| D | `ConfirmStackUpgrade` **:2535** | `ApplyStackUpgrade()` returned false **after** the spend (gold refunded, `Warning` logged) | Only the `!HasAuthority()` M8 guard (`Building.cpp:390`); ⚠️ **wrong wording** |

### 1.2 Door A traced to its single origin

`EPlacementInvalidReason::Upgrade` is **written at exactly one site** — `:2775`, inside the `NotStackable`/`Unaffordable` switch arm of `UpdatePlacementGhost`. (Grep census: 2 hits total in the .cpp/.h — one `case` label at :2236, one assignment at :2775.)

`EPlacementUpgradeState::NotStackable` is **produced at exactly one site** — `:5253`, inside `ResolvePlacementUpgradeState`, reached only after **all four preceding gates pass**:

```
(1) bPendingCardIsBuilding == true && !PendingCard.IsNone()      :5209
(2) IsValid(HoveredBuilding) && !IsBuildingDestroyed()           :5219
(3) HoveredBuilding->GetTeamId() == OwnTeam   (own team only)    :5228
(4) HoveredBuilding->GetCardID() == PendingCard  (SAME card!)    :5236
(5) !HoveredBuilding->CanScaleFootprint()   ⇒ NotStackable       :5251
```

### 1.3 What `CanScaleFootprint()` actually is — measured

- `Building.h:233` — `virtual bool CanScaleFootprint() const { return true; }` (every building says **yes** by default)
- `ClimbableTower.h:503` — `virtual bool CanScaleFootprint() const override { return false; }` (**the only override in the codebase**)
- **BP census over all 8 building Blueprints:** only `BP_Building_WatchTower.uasset` names `ClimbableTower` (3 hits). Negative control: `BP_Building_ArrowTower` → 0 hits. Cross-inheritance control: no building BP references any other building BP.

> ⚠️ **Instrument note:** my first binary probe used `strings`, which **is not installed** in this shell and printed an empty result for all 8 files. That empty output was the *instrument*, not a finding. Re-run with `grep -a`, positive control (`ArrowTower` names itself → 6 hits) and negative control (`ArrowTower` vs `ClimbableTower` → 0) both clean.

### ⭐ 1.4 CONCLUSION

**`"That building cannot be stacked"` is reachable in normal play by exactly one card acting on exactly one building: a WatchTower card hovering the player's own WatchTower.** Gate (4) forbids cross-card hovers, gate (3) forbids enemy buildings, and `CanScaleFootprint()` is false on nothing else in the project. Jonathan was stacking **WatchTowers**.

### ⚖️ 1.5 The correction to hypothesis 1

The orchestrator wrote: *"stacking and footprint-resizing are DIFFERENT features — but if the stack eligibility check consults that same predicate, or an adjacent one, it would produce exactly this refusal."*

**It consults the same predicate, and that is deliberate and documented, not a slip.** `CanScaleFootprint()` is asked **three independent times** for the stack feature:

1. `ResolvePlacementUpgradeState` :5251 (hover/ghost)
2. `ConfirmStackUpgrade` :2494 (click)
3. `ABuilding::ApplyStackUpgrade` **Building.cpp:411** (the mutator itself)

…and a **fourth** time for the wheel, via `CanCardActorScaleFootprint` :5384. TASK-813 spec (5) and TASK-815 spec (6) both name it as *the* single structural exclusion, and the comments state the reason explicitly (`:5241-5250`, `Building.cpp:405-410`): a scaled `SM_WatchTower` moves the `LadderFoot`/`LadderTop` sockets and the rung plane, fires **TOWER-§8.5a**'s voiding condition, and *"the climb stops working ENTIRELY while every readback still reports correct."*

⇒ **This is a design conflict for Jonathan to rule on, not a bug to quietly patch.** The stated justification is real for the **Z** axis too — arguably *more* so, since stacking is a pure Z scale and a ladder is a vertical feature.

---

## 2. `"Card actor unavailable"` — located, every door enumerated

**The string:** `SiegePlayerController.cpp:2324`, inside `TryConfirmPlacement`
`NSLOCTEXT("Siegebound", "CardRefused_NoActorClass", "Card actor unavailable")`

Reached by **one condition**: `ResolveCardActorClass(PendingCardID, PendingCardType)` returned `nullptr`. Followed by `ExitPlacementMode()` — the mode drops, card stays in hand, **no gold moves**.

> ⚠️ Distinguish from the *similar* string `"Card data unavailable"` (`CardRefused_NoData`, :983 / :1673 / :3037) — a **missing DT_Cards row**, a different defect. Jonathan's words match `:2324` exactly.

### 2.1 Every way `ResolveCardActorClass` (:4594-4642) returns null

| # | Condition | Reachable from placement? |
|---|-----------|---------------------------|
| a | `CardType` is not Building/Unit/Economy → `Error` log, null (:4626) | ⛔ No — `PlayHandSlot`'s type switch gates it |
| b | `LoadSynchronous()` on the composed CONVENTIONS path returns **null** — the asset is not at that path | ✅ **YES — this is it** |
| c | Class loads but `!IsChildOf(RequiredBase)` — wrong parent class | Possible; not the case here |

### ⭐ 2.2 ROSTER CENSUS — the measurement

Every spawnable `cards.csv` row checked against the CONVENTIONS path its `CardType` composes:

**21 of 22 present. Exactly one missing:**

```
MISSING  Witch (Unit) -> Content/Blueprints/Units/BP_Unit_Witch.uasset
```

Corroborating evidence, all controlled:

- `find Content -iname "*witch*"` → `SM_Witch`, `MI_Witch_PBR`, `T_Witch_D/N/ORM`, `T_CardArt_Witch`, the RawAssets — **everything except the actor Blueprint.** Positive control: the same find for `Sorcerer` returns `BP_Unit_Sorcerer.uasset` first.
- `git log --all -- Content/Blueprints/Units/BP_Unit_Witch.uasset` → **empty. It was never tracked, in any commit, ever.** Positive control: `BP_Unit_Sorcerer.uasset` → `80c47e8`.
- `DT_Cards.uasset` **contains the Witch row** (name-table probe: `Witch` 4, `Sorcerer` 3, `WatchTower` 3, bogus control `Nonexistentcardxyz` **0**).
- `cards.csv` Witch row: `Unit`, cost 50, **`DeckCount = 2`** ⇒ `UDeckComponent` builds 2 copies into the curated default draw pile (`DeckComponent.cpp:90-93`), and `UDeckBuilderWidget`'s reset-to-default seeds her too (`:617-621`). **She is drawable.**
- There is **no `AWitchUnit` C++ class** — the veil is implemented inside `ASummonedUnit`, so her actor is a plain `BP_Unit_Witch` deriving `ASummonedUnit`. Nothing can substitute for the missing asset.

### 2.3 It was declared in writing, then shipped anyway

`.claude/pipeline/qa/TASK-849.md` **N-5**, verbatim:

> *"`BP_Unit_Witch` and `T_CardArt_Witch` do not exist yet (`TASK-835` / `TASK-834`; the card face degrades to text-only, never a crash)."*

`T_CardArt_Witch` was subsequently authored (TASK-834 landed). **`BP_Unit_Witch` was not.** Commit `1aa0fee` (TASK-835, "Lane W lands") shipped `DT_Cards.uasset`, `cards.csv` and all six Witch art assets — **and no Blueprint.** The card went live without its actor.

### ⛔ 2.4 Where hypothesis 2 is wrong

The orchestrator relayed `qa/TASK-816.md`'s note that TASK-815 made `EnterPlacementMode` call `ResolveCardActorClass`, *"once per session, building cards only — one extra Warning line for a missing BP."*

**That note is accurate and it is NOT this bug.** `EnterPlacementMode:1773-1774`:

```cpp
bPendingCardCanScaleFootprint =
    bPendingIsBuilding && CanCardActorScaleFootprint(ResolveCardActorClass(CardID, Row->CardType));
```

`&&` short-circuits. **For a Unit card `bPendingIsBuilding` is false, so `ResolveCardActorClass` is never called** — the comment at `:1769-1772` says so explicitly and it is correct. The Witch is a Unit. TASK-815's line is not on her path at all.

The refusal Jonathan saw comes from the **shipped M2 confirm path** at `:2324`, which has existed since `aafd968` (TASK-021..030) and is behaving **exactly as designed**: a missing BP refuses the play with no gold spent and no crash. **The code is right; the content is missing.**

---

## 3. ⭐ WHICH IS UPSTREAM? — NEITHER. Measured in both directions.

The orchestrator asked whether "card actor unavailable" fires first and leaves placement in a bad state, making "cannot be stacked" a symptom. **The call graph refutes it.**

**Direction 1 — can a stack refusal reach the class resolve?** No. Site A (`:2260`) sits inside the `if (!bPlacementValid)` block that `break`s to a `return` at **`:2271`**. `ResolveCardActorClass` is at **`:2321`**, fifty lines *below* that return. A stacking refusal **never reaches** the class resolve.

**Direction 2 — can a Witch refusal poison a later stack attempt?** No. `:2324` calls `ExitPlacementMode()`, which clears (`:1826-1833`):

```
PlacementUpgradeState = None
PlacementUpgradeTarget = nullptr
PlacementFootprintScale = PlacementFootprintMin
bPendingCardCanScaleFootprint = false
```

…and `EnterPlacementMode` clears **the same four members again** on entry (`:1753-1762`, `:1773`) — the comment at `:1749-1752` states this belt-and-braces intent outright. **No state survives between placement sessions.** Additionally `UpdatePlacementGhost` recomputes `PlacementUpgradeState` **every frame** (`:2744`) and nulls `PlacementUpgradeTarget` before the switch (`:2756`).

**Direction 3 — could one card cause both?** No. Gate (1) of `ResolvePlacementUpgradeState` requires `bPendingCardIsBuilding`. The Witch is a **Unit**; the WatchTower is a **Building**. They cannot be the same card.

⇒ ⭐ **This batch's "two reported defects were one root cause" pattern does NOT apply here. They are genuinely two.**

---

## 4. REGRESSION vs GAP — settled with `git log -S`, not inferred from code shape

| Bug | Provenance | Verdict |
|-----|-----------|---------|
| **"cannot be stacked"** | `StackNotStackableRefusalText` first appears in **`d287102`** (TASK-814, Lane B) — the commit that *introduced tower stacking* | ⛔ **NOT a regression.** Stacking did not exist before `d287102`, so nothing broke. It is a **feature that shipped with a deliberate exclusion** Jonathan did not know about. |
| **"card actor unavailable"** | The *string* is from **`aafd968`** (M2, TASK-021..030) and has been correct for months. What changed is the **Witch DT_Cards row**, added by **`1aa0fee`** (TASK-835, Lane W) | ✅ **REGRESSION — but from `1aa0fee`, ⛔ NOT from `d287102` or `239ca77`.** Neither of last night's two named commits is implicated. It is a **content gap shipped past a QA declaration**, not a code defect. |

---

## 5. THE GHOST TINT vs THE COMMIT PATH — ⭐ addressing Jonathan's correction

> *"I wanted it to be entirely blue when hovering over a potential stacking case (instead of entirely green when the placement is valid and entirely red when the placement is invalid)."*

### ⭐ 5.0 THE BLUE STATE **IS IMPLEMENTED**, AND IT IS EXACTLY WHAT HE DESCRIBES

⛔ **The "maybe blue was never built" hypothesis is FALSIFIED.** The ghost tint is a **three-state whole-ghost tint**, not a two-state valid/invalid path, and not an outline on the target. `SiegePlayerController.cpp:2804-2824`:

```cpp
static const FName GhostColorParamName(TEXT("GhostColor"));
const FLinearColor ActiveGhostColor =
    (PlacementUpgradeState == EPlacementUpgradeState::Ready)
        ? UpgradeGhostColor                                    // ⭐ BLUE
        : (bPlacementValid ? ValidGhostColor                   //    GREEN
                           : InvalidGhostColor);               //    RED
GhostMID->SetVectorParameterValue(GhostColorParamName, ActiveGhostColor);
```

The three tunables, measured at `SiegePlayerController.h:2145-2167`:

| Member | Default | Reads as |
|--------|---------|----------|
| `ValidGhostColor` | `FLinearColor(0.f, 1.f, 0.f)` | **GREEN** — placement valid |
| `InvalidGhostColor` | `FLinearColor(1.f, 0.f, 0.f)` | **RED** — placement invalid |
| `UpgradeGhostColor` | `FLinearColor(0.f, 0.4f, 1.f)` | ⭐ **BLUE** — stack case (TASK-813, STACK-§0) |

It is applied to **the whole ghost**: one `GhostColor` vector param on the `GhostMID`, set on **every material slot** of the ghost mesh (`:2936-2941`). **Needle controlled:** `M_Ghost.uasset` contains `GhostColor` (1 hit); positive control `Opacity` → 2; negative control `ZZZnotaparamZZZ` → **0**.

### ⭐ 5.1 THE TINT PREDICATE AND THE COMMIT PREDICATE ARE **THE SAME FUNCTION**

⛔ **The "hover tints from one predicate, click refuses from another" hypothesis is FALSIFIED.** There is exactly **one** resolver, `ResolvePlacementUpgradeState`, called at exactly **one** site (`:2744`), whose result is stored in **one** member, `PlacementUpgradeState`. Both consumers read that member and nothing else:

- **tint** → `:2820`
- **commit** → `:2244` (Unaffordable) and `:2289` (Ready), plus `PlacementInvalidReason` written at the single site `:2775`

There is no second expression to drift. This is the same one-resolver discipline the orchestrator noted elsewhere in the batch — here it was designed in from the start (`:2237-2241` states the intent, and the code honours it).

### ⭐⭐ 5.2 THE ELIGIBILITY CHECK **DOES** RUN AT HOVER TIME — and his error message PROVES it

⛔ **The third hypothesis ("the check is correct and simply never runs at hover time") is FALSIFIED on two independent grounds.**

**(i) Direct measurement of the tick path.** `ASiegePlayerController::PlayerTick` (`:663`) runs, in this order, **every frame while in placement mode**:

```
:785  ApplyPlacementFootprintWheel()   — wheel
:788  UpdatePlacementGhost()           — ⭐ trace, ResolvePlacementUpgradeState, tint
:795  if (WasInputKeyJustPressed(LMB)) TryConfirmPlacement()
```

The eligibility check runs **before** the click is polled, **in the same tick**. The click cannot read a stale or unset value.

**(ii) ⭐ The refusal he received is itself the evidence.** `"That building cannot be stacked"` at `:2260` is reachable **only** through `PlacementInvalidReason == EPlacementInvalidReason::Upgrade`, and that value is written at **exactly one site in the entire codebase** — `:2775`, **inside `UpdatePlacementGhost`'s switch**, reached only when `ResolvePlacementUpgradeState` returned `NotStackable`.

⇒ **If the eligibility check had never run at hover time, `PlacementUpgradeState` would have stayed `None`, `PlacementInvalidReason` would never have been `Upgrade`, and Jonathan would have seen `"Too close to another building"` or `"Invalid placement location"` instead.** The fact that he saw *this specific sentence* is proof the hover-time check **ran, recognised the stack case, and deliberately refused it.**

### ⭐ 5.3 So what did he actually see, and why "no blue" and "cannot be stacked" are ONE defect

For a WatchTower-on-WatchTower hover, `ResolvePlacementUpgradeState` reaches gate (5), returns `NotStackable`; the switch at `:2767-2776` sets `bPlacementValid = false`; the ternary therefore paints **RED**. **`UpgradeGhostColor` is reached only on `Ready`.**

⇒ **He saw the ghost turn RED and then got the refusal — and both came from the same single decision:** `AClimbableTower::CanScaleFootprint() == false`. **The missing blue and the refusal sentence are two symptoms of one root cause, not two bugs.** Change that one predicate for the height case and the WatchTower turns **blue** on hover *and* the click upgrades — no tint code, no resolver, no commit-path change required. **The plumbing is entirely correct and already agrees with his spec.**

### ⚠️ 5.4 Two genuine usability findings on this path

1. **The `NotStackable` red is pixel-identical to ordinary "invalid location" red.** Both resolve to `InvalidGhostColor`. There is no distinct tint for *"this particular building refuses you"*, so the player gets no warning until the click — which is precisely why this reads as a bug rather than a rule. ⚠️ **This one survives Jonathan's ruling either way** (see §7.3).
2. **Sites B (`:2484`) and D (`:2535`) reuse the wrong sentence.** B fires when the target *died between the ghost frame and the click*; D fires on the *M8 authority guard after a refunded spend*. Neither is a "cannot be stacked" condition, and both would tell the player something false.

### ⚠️ 5.3 Two genuine usability findings on this path

1. **The NotStackable red is pixel-identical to ordinary "invalid location" red.** Both resolve to `InvalidGhostColor`. There is no distinct tint for *"this particular building refuses you"*, so the player gets no warning until the click — which is precisely why this reads as a bug rather than a rule.
2. **Sites B (`:2484`) and D (`:2535`) reuse the wrong sentence.** B fires when the target *died between the ghost frame and the click*; D fires on the *M8 authority guard after a refunded spend*. Neither is a "cannot be stacked" condition, and both would tell the player something false.

---

## 6. ⭐ THE SHIPPED SERIES vs HIS SPEC — no divergence found

| Quantity | Spec as relayed | What ships | Match |
|----------|-----------------|-----------|-------|
| Height | ×2 → ×3 → ×4 → ×5, **capped** | `StackHeightMultiplier(n) = min(1 + n, MaxStackHeightMultiplier)`, `MaxStackHeightMultiplier = 5` (`Building.h:346`) ⇒ n=1→**2**, 2→**3**, 3→**4**, 4→**5**, ≥5→**5** | ✅ **exact** |
| Health | ×1.5 **compounding, uncapped** | `MaxHP = OldMaxHP * StackHealthMultiplier(1)` with `StackHealthStep = 1.5f` (`Building.h:365`) — mutated **in place** ⇒ base × 1.5ⁿ. No ceiling term in the loop (`Building.cpp:371-379`) | ✅ **exact** |
| Wheel | capped at **1.5×** default | `PlacementFootprintMin = 1.0f`, `PlacementFootprintMax = 1.5f`, step `0.1f` (`SiegePlayerController.h:2194/2213/2229`) | ✅ **exact** |

**No third finding here.** Two notes, neither a defect:

- `Building.cpp:324-328` records **ruling J-0**: Jonathan's word *"double"* was read as **additive** (`+1×` per upgrade), **not** `2ⁿ`, because under a doubling reading his own ×5 ceiling is unreachable (2, 4, 8, 16…). The spec as relayed to me (×2→×3→×4→×5) **is** the additive series, so his stated expectation and the shipped code agree. The comment explicitly says *"⛔ Do not 'fix' this toward his summary sentence."*
- `CurrentHP += (MaxHP - OldMaxHP)` (`:455`) — the upgrade grants the *new* hit points and does **not** repair old damage (ruling J-10, by design; a full heal would make upgrading a Masons substitute).
- The height cap is surfaced as a HUD note at confirm only when it actually bit (`:2548-2551`), derived from the series rather than the literal `5`. Correct.

---

## 7. ⛔ RECOMMENDATIONS — stated as recommendations, ⛔ nothing applied

### 7.1 Bug 2 — the real blocker, and it is **not a code fix**

**R1 (art / integration, highest priority).** Author `Content/Blueprints/Units/BP_Unit_Witch.uasset`:
- parent class **`ASummonedUnit`**
- `VisualMesh` = `/Game/Meshes/SM_Witch` (component name must be exactly `VisualMesh` — CONVENTIONS)
- ⚠️ **hand-author the static `VisualMesh` transform: yaw `-90`, Z `= -CapsuleHalfHeight`.** Per `handoffs/TASK-833-artist.md` item 3 this is a **known latent trap** with no C++ owner (parked debt TASK-335), and **`BP_Unit_Wizard` is the one unit that missed the ritual.** ⛔ **Read the real capsule half-height — the "half-height == 90" assumption is banned** (shipped values 74.5–145).
- ⛔ **No C++ change is needed or wanted.** `ResolveCardActorClass` already does the right thing.

**R2 (defence, separate task — I did not write it, fenced).** The gap was declared in `qa/TASK-849.md` and shipped regardless. A headless test that walks every `DT_Cards` row of a spawnable `CardType` and asserts its composed CONVENTIONS path resolves would have **failed at `1aa0fee`**. That is the census in §2.2, automated. Cheap, and it closes the whole class.

**Interim, if the Witch cannot be authored immediately:** setting her `DeckCount` to `0` removes her from the curated default deck and stops the refusal, without touching code. ⚠️ She would still be selectable in the Deck Builder, so this is mitigation, not a fix.

### 7.2 Bug 1 — ⛔ needs Jonathan's ruling BEFORE any code moves

**The question is his, not ours: may the WatchTower grow in HEIGHT even though it may not be wheel-resized in WIDTH?** The code currently answers *no* to both through one predicate, on a stated climb-rig justification.

**If he rules YES**, the minimal shape is to **split the virtual**:
- keep `CanScaleFootprint()` for the **X/Y wheel** (stays `false` on `AClimbableTower`) — sites `:5384`, `:2839`
- add a sibling `CanStackHeight()` on `ABuilding` (default `true`), overridable independently
- repoint the **three** stack gates at it: `ResolvePlacementUpgradeState:5251`, `ConfirmStackUpgrade:2494`, `ApplyStackUpgrade` (`Building.cpp:411`)

⭐ **That single change also delivers the blue ghost he asked for, with ⛔ zero tint-code edits** — gate (5) would return `Ready` instead of `NotStackable`, and the existing ternary at `:2820` paints `UpgradeGhostColor` automatically (§5.3). **⛔ Do not board a separate "add the blue state" task: it already ships and is already correct.**

⛔ **But I would not board that as a one-liner.** `AClimbableTower` carries `LadderFoot`/`LadderTop` sockets and a rung plane on the mesh. A Z scale of ×2…×5 moves `LadderTop` to 2–5× its authored height and stretches the rung spacing with it. **TOWER-§8.5a's voiding condition must be re-read and the climb re-measured at each multiplier before this ships** — otherwise the WatchTower becomes stackable and **unclimbable**, which is worse than the bug being fixed, and (per the memory record) fails silently with every readback still reporting correct. **That measurement is a task in itself and should precede the code task.**

**If he rules NO**, the fix is purely communicative — see below, which is worth doing **either way**.

### 7.3 Cheap improvements independent of the ruling

- **Give `NotStackable` its own ghost colour.** `:2819-2822` already has the ternary chain to hang it on; today the player cannot distinguish "refuses to stack" from "can't build here". One new `FLinearColor` UPROPERTY beside `UpgradeGhostColor`.
- **Stop reusing the not-stackable sentence for two unrelated conditions** at `:2484` (target died mid-click) and `:2535` (M8 authority refusal after refund). Distinct `NSLOCTEXT` keys.
- **Surface the exclusion in the controls help screen.** `SiegeControlsHelpWidget.cpp:690` already carries the line *"ONE KIND OF BUILDING REFUSES TO BE STACKED, AND IT IS THE ONE YOU CAN CLIMB"* — worth checking with Jonathan whether that screen was reachable/read, since the rule **was** documented in-game.

---

## 8. ⚠️ CONFIDENCE AND WHAT WOULD SETTLE IT DEFINITIVELY

**Bug 2 — certainty: very high.** The missing asset is a filesystem and git fact, controlled in both directions against a positive control.

**Bug 1 — certainty: high, but by exhaustive elimination, ⛔ not by observation.** ⛔ I could not run PIE, so I did not observe which card he held. I established that the message is **unreachable** for any building other than a WatchTower given gates (1)–(4) and the measured class census. **One line in his log settles it in seconds** — both refusals already log the CardID by name in shipped code:

- `SiegePlayerController.cpp:2257-2259` → `"upgrade click refused for '<CardID>' — the hovered building refuses footprint scaling (CanScaleFootprint() is false...)"`
- `SiegePlayerController.cpp:4638-4640` → `"card class '/Game/Blueprints/Units/BP_Unit_Witch.BP_Unit_Witch_C' missing or not a SummonedUnit (built in TASK-010/034) — play refused, no gold spent"`

⇒ **Recommend the footage-analyst / Jonathan grep `LogGitClaudeUnrealTest` for those two lines.** They name the cards outright and would convert both findings from inferred to observed.

---

## Files read (⛔ none modified)

- `Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.cpp` / `.h`
- `Source/GitClaudeUnrealTest/Siegebound/Building.cpp` / `.h`
- `Source/GitClaudeUnrealTest/Siegebound/ClimbableTower.h`
- `Source/GitClaudeUnrealTest/Siegebound/DeckComponent.cpp`, `DeckBuilderWidget.cpp`, `SummonedUnit.h`
- `Docs/Data/cards.csv`, `Content/Data/DT_Cards.uasset` (binary probe), `Content/Blueprints/**`
- `.claude/pipeline/qa/TASK-849.md`, `.claude/pipeline/handoffs/TASK-833-artist.md`, `TASKBOARD.md`
