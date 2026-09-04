# QA Report — TASK-924 (VEIL-SWAP GATE)

**Verdict: PASS** — **0 BLOCKERS · 5 WARN · 6 NIT.**

| task | verdict | blockers |
|---|---|---|
| **TASK-923** — wire the veil material to the veil state | ✅ **PASS** | 0 |

## `SC-§29` COVERAGE LEDGER

This gate covers **exactly one task, named: `TASK-923`** — its three files and nothing else.

⛔ **NOT covered by this gate:** `TASK-922` (the asset commit) · `TASK-919` (its build is running; its executed suite figure supersedes every number below) · the other lane's dirty tree (`SiegeControlsHelpWidget.{h,cpp}`, `SiegePlayerController.{h,cpp}`, `Tests/SiegeAssistantSelectionTest.cpp`, `Tests/SiegeControlsHelpTest.cpp`, `Tests/SiegePlacementTest.cpp`) · `J-W15` / `J-W16` (**Jonathan's**, not adjudicated here) · the compile · the pixel.

⛔ **Method (`SC-§38`): every location below was found by opening the named SYMBOL and reading the quoted EXPRESSION. Not one line number from the board or from the handoff was trusted** — and the handoff was right that they had already moved (§N-1). Coordinates are my own reads, dated 2026-09-03, and are annotations.

---

## ⛔ THE DECLARED LIMIT, RESTATED FIRST SO NOTHING BELOW CAN BE READ AS MORE THAN IT IS

⛔⛔ **THIS PASS DOES ⛔ NOT SAY "THE VEIL IS VISIBLE IN PLAY." IT SAYS THE SWAP IS WIRED, ON BOTH EDGES, ON EVERY SLOT, WITH NO CACHE AND NO THIRD SITE.**

The row is **DECLARED PIXEL-GATED and the declaration is correct.** I re-derived the reason rather than accepting it: there is no `UWorld::CreateWorld` and no `SpawnActor` anywhere in `Siegebound/Tests/`, and `TASK-832` measured that this exact effect is invisible to every instrument except a region-restricted frame diff against a strength-0 control. ⇒ **not one of the new assertions touches a pixel; every one of them reads source text.**

⛔ **The skeletal-mesh PIE condition is RESTATED VERBATIM in the handoff (§6) and carried BY NAME to `TASK-907`, exactly as item (6) demanded.** ⭐ `bUsedWithSkeletalMesh = true` is **mitigation, not proof** — `GHOST-§5`'s defect is invisible in-editor and appears only in a packaged build. ⛔ **Failing this row for that would be failing it for the absence of `SK_Witch`** (the gate's own item (8) forbids it). It is not a blocker; it is an open condition with an owner.

⭐ **`SC-§44`'s blindness case closed tonight only on a human eye. Treat this the same way: the last word on TASK-923 is Jonathan's, not the suite's.**

---

## (1) ⛔ EVERY SLOT, NEVER SLOT 1 — ✅ **CONFIRMED, MECHANICALLY**

`ASummonedUnit::ApplyVeilMaterial()` (`SummonedUnit.cpp:2951`):

```cpp
const int32 SlotCount = ActiveMesh->GetNumMaterials();
for (int32 SlotIndex = 0; SlotIndex < SlotCount; ++SlotIndex)
{
    ActiveMesh->SetMaterial(SlotIndex, ResolvedVeil);
}
```

- ✅ Driven by `GetNumMaterials()` on `GetActiveVisualMesh()`. ⛔ **No literal `0`, no literal `1`, no assumed count of `2` anywhere in either helper.** I grepped `SetMaterial\(` across the whole of `SummonedUnit.cpp`: **exactly three write sites** — `:354` (`ApplyTeamMaterial`, slot 0, pre-existing), `:2999` (apply, index-free), `:3029` (clear, index-free). There is no fourth.
- ✅ `GetActiveVisualMesh()` (`:358`) returns `SkeletalVisualMesh` when `bUsingSkeletalVisual && SkeletalVisualMesh`, else `VisualMesh` — both `UMeshComponent`, one code path covers both, and the null guard's shape is copied from `ApplyTeamMaterial` rather than reinvented.
- ✅ **Slot 0 is covered.** The gate's headline risk — an opaque chrome hat on 18% of the Witch, on her brim — is closed by construction, not by comment.
- ✅ The idiom matches both named precedents **exactly**, which I opened: `SiegeGhostPawn.cpp:326-331` (whole-body, count hoisted) and `SiegePlayerController.cpp:2939-2941`. No third idiom was invented.

## (2) ⛔ THE RESTORE — ✅ **CALL GRAPH TRACED CLOSED, AND IT RE-DERIVES**

| link | measured |
|---|---|
| `ClearVeilMaterial` **declared** | `SummonedUnit.h:1611`, inside the `private:` block that opens at `:1483` — the same block as `ApplyTeamMaterial` (`:1524`) and `GetActiveVisualMesh` (`:1559`) |
| **defined** | `SummonedUnit.cpp:3003` |
| **reached** | `SummonedUnit.cpp:2938`, inside `BreakInvisibility`, **below** the `ApplyBreak` early-out at `:2914-2917` |
| slot 0 | `:3043` — the **shipped** `ApplyTeamMaterial()`, called **unconditionally**, outside the clear loop's own null guard |

- ✅ **It runs once per veil, never per swing.** `ApplyBreak` returns true only on the true→false edge; the early-out at `:2916` returns for an unveiled unit before anything is painted. `BreakInvisibility` has **eight** call sites (Attack ×3, Heal, Death, Cast, Mine, Empower) and several sit on per-cadence attack and per-tick mining paths — hoisting the restore above that early-out would have re-stamped materials fleet-wide every swing. It is not hoisted.
- ✅ **NOTHING IS REMEMBERED ACROSS THE EDGE.** I checked for the shape, not for the word: `TArray<UMaterialInterface` appears **twice tree-wide and both are prose** (`SummonedUnit.h:1597`, a `* ` doc-continuation line; `SummonedUnit.cpp:3008`, a `//` line). **Zero new members, zero new `UPROPERTY`s, zero new bools.** The clear reads no saved state — it re-derives the slot count from the **currently** active mesh and re-derives slot 0 from the **current** `Team`. ⇒ the non-obvious ban holds: were `GetActiveVisualMesh()` ever to hand back a different component, the restore still lands on the right one.
- ✅ **Slot 0 comes back in the unit's own colour.** `ApplyTeamMaterial` (`:326`) resolves `MI_TeamColor_Red`/`Blue` from `Team` at call time and writes `SetMaterial(0, …)` on `GetActiveVisualMesh()`. A red bot unit cannot come back blue.
- ✅ **The clear is the apply's exact mirror** — same accessor, same count source, same loop bounds — so no slot the apply painted can be stranded.

## (3) ⛔ THE STATIC READ-BACK — ✅ **I REPRODUCED IT INDEPENDENTLY IN UE 5.8 SOURCE, AND IT SUFFICES FOR WHAT IT CLAIMS**

The handoff labelled this a **static source read, not an editor observation.** Held to that standard, and re-measured at my own instant against the installed engine:

| step | my read | result |
|---|---|---|
| the write | `Engine/Source/Runtime/Engine/Private/Components/MeshComponent.cpp:63`, assignment at **`:99`** | `SetMaterial(i, nullptr)` sets `OverrideMaterials[i] = nullptr`; it does **not** remove the entry, and it grows the array (`:74-77`) to reach `i` |
| the read, **static** | `Engine/Source/Runtime/Engine/Public/StaticMeshComponentHelper.h:140` | `if (OverrideMaterials.IsValidIndex(i) && OverrideMaterials[i])` → a null entry fails the second test → falls through to `Component.GetStaticMesh()->GetMaterial(i)` (`:145-147`) |
| the read, **skinned** | `Engine/Source/Runtime/Engine/Public/SkinnedMeshComponentHelper.h:110` | identical test → falls through to `SkinnedAsset->GetMaterials()[i].MaterialInterface` (`:116-118`) |
| the rejected alternative | `MeshComponent.cpp:246-281` | `EmptyOverrideMaterials()` does `OverrideMaterials.Reset()` (`:262`) **and** `MaterialSlotsOverlayMaterial.Reset()` (`:268-270`) — the side effect §4c names is **real** |

✅ **VERDICT ON SUFFICIENCY: YES, FOR THIS CLAIM.** The claim is *"`GetMaterial(i)` after the clear returns the asset's authored material on either component type."* That is a **branch in engine source with no runtime input** — it is decided by a null test, not by scene state — so a source read is the *right* instrument and an editor observation would have been **weaker**, because one observation covers one component type. ⛔ **It does not, and does not claim to, prove anything renders.** ⭐ Naming the instrument instead of dressing it up is why this passes; the same claim asserted as "I verified in the editor" would have been a false pass.

## (4) ⛔ THE FAILURE DIRECTION — ✅ **RESOLVE-BEFORE-WRITE, AND THE UNIT STAYS VISIBLE**

`ApplyVeilMaterial` returns at `:2983` — **before a single slot has been touched** — when `LoadSynchronous()` yields null, after one `Warning` naming the path. ⛔ **No `SetVisibility`, no `SetHiddenInGame`, no blanked slot anywhere in either helper** (the only three `SetVisibility` calls in the file are `:200`/`:509`/`:512`, the pre-existing M7 skeletal swap). ⇒ **an unkillable invisible solid is unreachable** (`WITCH-§0`). And `GrantInvisibility` returns the **flag's** edge (`:2905`), never the material's — the material stays a consequence, never a second source of truth.

## (5) ⛔ TWO PAINTERS, ONE CALLER EACH, NEITHER READS THE FLAG — ✅ **AND THE PIN IS STILL 2. RE-MEASURED.**

⭐ **The count-pin the brief told me to re-measure, measured:**

| needle | file | code lines | raw lines | verdict |
|---|---|---|---|---|
| `(bIsInvisible` | `SummonedUnit.cpp` | **2** (`:2890`, `:2914`) | **2** | ✅ **the closure pin is UNMOVED at 2**, and prose-immune |
| `FSiegeInvisibilityStatics::ApplyVeil(` | `SummonedUnit.cpp` | **1** | **1** | ✅ |
| `FSiegeInvisibilityStatics::ApplyBreak(` | `SummonedUnit.cpp` | **1** | **1** | ✅ |
| `bIsInvisible =` on a **code** line | tree-wide | **0** | — | ✅ (the two `bIsInvisible = false` texts at `:2910`/`:5233` are `//` lines) |

⇒ **the helpers genuinely consume the doors' return values.** `ApplyVeilMaterial` is guarded by `if (bNewlyVeiled)`; `ClearVeilMaterial` sits inside the post-early-out branch. Neither names the flag. **That is what keeps the pin at 2** — a helper that consulted `bIsInvisible` would have been a third parenthesised mention and turned `SiegeAcquisitionFunnelTest`'s closure row red, and the brief is right that a class-2 pin going red *because a new caller exists* has bitten twice tonight. It did not bite here.

⭐ **And the no-third-site property is real, measured by symbol, on code lines only:**

| symbol | declaration | definition | calls | total |
|---|---|---|---|---|
| `ApplyVeilMaterial` | `SummonedUnit.h:1589` | `.cpp:2951` | `.cpp:2898` (`GrantInvisibility`) | **3** ✅ |
| `ClearVeilMaterial` | `SummonedUnit.h:1611` | `.cpp:3003` | `.cpp:2938` (`BreakInvisibility`) | **3** ✅ |

⛔ No tick, no timer, no `BeginPlay` branch, no second bool, no new member, no `UPROPERTY`. **A grep for those two names is the complete list of ways a unit's look changes for the veil, and it is now a suite row rather than a promise.**

## (6) ⛔ NO WITCH-SPECIFIC COMPENSATION — ✅ **CONFIRMED, AND THE RULING IS UPHELD**

⛔ **There is no per-`CardID` branch, no second material, no compensating strength, and no `Witch` literal anywhere in the two helpers.** `CardID` appears in them **only** inside log text (`:2982`). The shared `MI` is applied directly — **no per-actor MID** — which is right: the veil drives no parameter, and `WITCH-§5` clause 2 already ruled that losing the team read on a veiled unit is acceptable.

⚖️ **The metallic weakness is `TASK-899`'s consequence and is ruled WITH `J-W15`, never separately.** I checked the numbers **at their source** rather than re-deriving them (`TASK-924` item (7)): `handoffs/TASK-832-artist.md:112-115` records footman **20.05 → 8.66 (−57%)** and witch **10.84 → 11.53**. The board's `20.1 → 8.7` and `10.8 → 11.5` are correct roundings of the artist's own figures. ⛔ **These are the artist's measurements, not mine.** ✅ **A compensation written into this diff would have survived the re-bake and then been wrong** — the exact waste `TASK-899`'s sequencing clause exists to prevent. Correctly refused.

## (7) ⛔ THE PRESENT-TENSE PROSE (`SC-§39.1` cl. 7) — ✅ **CORRECTED, AT ALL THREE SITES**

The sentence the gate named — *"The MATERIAL swap … is ⛔ NOT TASK-829's"* — is **gone**. `SummonedUnit.cpp:2872-2879` now describes the wired state and says why the material is a consequence of the flag. The block header at `:2866` reads *"ZERO RULES"* (not *"zero logic"*), and `SummonedUnit.h:439-450` keeps *"three functions, no fourth"* **true** and annotates it with the two private painters. ⛔ **Shipping "not wired" inside the artefact that wires it would have been a WARN at minimum. It was not shipped.**

---

## ⚖️🧑 RULING 1 — `J-W17` (the health bar over a veiled unit): ⛔ **BOARD IT.**

### First, the number. ⛔ **I re-measured rather than relayed, and the relayed figure was wrong.**

| needle | file | **my measurement** |
|---|---|---|
| `invisib` (case-insensitive) | `CombatantHealthBarComponent.cpp` | ⛔ **2** — `:79`, `:327` |
| `invisib` (case-insensitive) | `CombatantHealthBarComponent.h` | ⛔ **3** — `:321`, `:577`, `:585` |
| **positive control** `health` | `CombatantHealthBarComponent.cpp` | **80 lines** ⇒ the instrument is alive |
| `IsInvisible` / `IsAgentVisibleTo` / `bIsInvisible` / `ESiegeVeil` | **both** files | ⛔ **0** |

⇒ ✅ **The board's *"returns ZERO hits"* is wrong; the true figure is 2 (5 across the pair).** All five hits are ordinary prose. ⛔ **The RULING is unaffected — there is still no veil predicate — but the EVIDENCE offered to this gate was a citation, not a fact.** ⚖️ *A gate asked to rule on "an ABSENCE, measured" must not be handed a count that is wrong by five.* **Recorded, as asked: the zero was relayed by the orchestrator and was not measured by the relayer.** `SC-§40` cl. 9 caught its own section, and the programmer — not the gate — caught it first. That is `SC-§47` working exactly as written.

### The ruling: ⛔ **BOARD, and board it NOW rather than deferring.**

**Why now and not later:** before tonight this defect was **unobservable** — nothing turned a unit translucent, so a bar over a solid unit was just a bar. ⛔ **`TASK-923` is what makes it visible.** The moment a veiled body ghosts out, a fully-opaque bar hovering over nothing becomes **the single most legible positional tell in the game** — better than a silhouette, because it is UI-bright, camera-facing and unoccluded. ⇒ **it is now the largest remaining gap between "the veil is wired" and "the veil works", and it will read to Jonathan as *"invisibility is broken"*.**

**Where it does NOT go:** ⛔ **not in `TASK-923`, and `TASK-923` was correctly fenced out of it.** `CombatantHealthBarComponent.{h,cpp}` is `TASK-898`/`902`'s **ungated** tree and a second writer there is a collision. ⛔ **And it is not a one-line hide:** the correct predicate is **per-viewer** — `FSiegeCombatStatics::IsAgentVisibleTo(ETeamId, const AActor*)` — because the **owning** player must still see their own veiled unit's bar while the enemy must not. That is a design question with an M8 replication tail (`WITCH-§6`'s M8 declaration: the flag is asymmetric per client).

**Boarding shape (manager's to write, mine to recommend):** one row, `gameplay-programmer`, **sequenced after `TASK-902`'s tree is quiet** (`QUIET-MODULE`), gated, consuming the per-viewer predicate — **and see RULING 3: it should be boarded as one row with the other two tells, not as three.**

---

## ⚖️ RULING 2 — `CombatantHealthBarComponent.h:585` was ALREADY RELYING ON THIS DIFF

### The species, named: ⛔ **this is an ANTICIPATORY description, not a stale one — and it is the more dangerous of the two.**

A **stale** comment was true when written and a later diff falsified it. An **anticipatory** comment was **FALSE when written** and a later diff **validated** it. `TASK-860` mitigated its cast-bar residual with *"a COMPLETED cast also turns the target translucent (`MI_Unit_Invisible`)"* — which had **no caller** until tonight, so the declared 1.7% ambiguity had **no second signal** and was simply an unmitigated hole for the whole window between the two commits.

⛔⛔ **Why it is worse than staleness, in one sentence: a stale claim is falsified by a diff a reviewer is looking at; an anticipatory claim is validated by a diff nobody connected to it — so nothing ever draws a reviewer's eye to the window in which it was false.**

⭐⭐ **And the instrument point, which is the part worth keeping: A NAME CENSUS CANNOT CATCH AN ANTICIPATORY CLAIM, BECAUSE EVERY NOUN IN IT IS REAL.** `MI_Unit_Invisible` existed. `TASK-832` shipped it and measured it at 13× the noise floor. A grep for the asset name returns hits in law, in a test and in a header — and **all of them are true statements about an asset with zero callers.** ⚖️ *This is the same shape as the earlier lesson that a name census cannot catch a site that RECOMPUTES a value: the census tests nouns, and the defect lives in the verb.*

### ⛔ AND IT PROPAGATED TO LAW. **That is the finding, not the comment.**

I measured the same claim in **three** places, and the third is the one that matters:

1. `Source/…/CombatantHealthBarComponent.h:585` — the comment the programmer found.
2. `Source/…/Tests/SiegeCastBarTest.cpp:92` — the same sentence, mirrored into a test file's prose.
3. ⛔⛔ **`CONVENTIONS.md`, `WITCH-§9.1` row 4** — *"COMPLETION ⛔ already has a tell — ⛔ for the OWNER only (the target turns translucent, `MI_Unit_Invisible`)"*.

⇒ ⭐⭐ **`WITCH-§9.1` is the requirements table that decided the cast-bar-vs-animation debate, and one of its four rows described a behaviour the tree did not exhibit.** `TASK-860` did not invent the claim — it read it out of law. **The prose propagated the gap instead of closing it.**

### What it means, and what I am asking for

- ✅ **No code change. `TASK-923` RETIRED this defect; it did not introduce it.** All three sentences are **true as of this commit**.
- 📌 **`TASK-925`'s commit message must say so** — the handoff already asks for it, and it is the right place: *before this commit `MI_Unit_Invisible` had zero callers, and this commit closes a residual `TASK-860` had already written into a comment as if it were mitigated.*
- 📋 **A law amendment for the manager** (mine to propose, not to write): ***a comment or a law row describing behaviour the tree does not yet exhibit must be written in the FUTURE tense and must name the row that will make it true (`TASK-### will…`). Present tense is reserved for what a grep can confirm today.*** ⭐ Then the **absence of a task ID** is itself the instrument — which is precisely what was missing here, since `WITCH-§5`'s own new clause records that *"declaring a gap is not boarding it."*
- ⚖️ **The twin of the durable sentence:** `WITCH-§5` says *an edge is nobody's deliverable.* This adds: ⭐ ***an edge that two lanes each assume the other owns gets DESCRIBED into existence — and the description then reads as evidence that it exists.***

### ⛔ The `WITCH-§9.0` cancelled-vs-shipped contradiction — **I CAN adjudicate it, and it is not a contradiction**

The programmer flagged it as unresolvable from its seat. It resolves in law: **`WITCH-§9.0` itself states** that the bar-specific geometry in `UCombatantHealthBarComponent` (`ApplyCastRowGeometry`, the `DrawSize`/`Pivot` recompute, the `OnCastProgressChanged` BIE and the 15 `SiegeCastBarTest` rows) is *"dead weight the moment the bar is cancelled"* and is *"boarded as its own row against the ⛔ NEXT commit, ⛔ never left to rot"* (`CARDBAR-§6`'s no-dead-entry-point rule). ⇒ **the C++ cast row being live is a KNOWN, LAW-ACKNOWLEDGED removal debt, not a contradiction, and `TASK-923` was right to leave it alone.** ⛔ **The provider seam (`IsCastInProgress()` / `GetCastProgressPercent()`) explicitly SURVIVES — an Anim BP reads exactly those two.**

⚠️ **But — `W-5` below.** I searched the board for that removal row and could not find it.

---

## ⚖️ RULING 3 — the HIT FLASH, and ⛔ **A THIRD TELL THE DIFF DID NOT REPORT**

### ✅ The hit flash is real. **Confirmed by my own read, both halves.**

- `ASummonedUnit::TakeDamage` calls `HitFlashComponent->TriggerFlash()` at **`SummonedUnit.cpp:5076`**, on actual damage, **unconditionally** — no veil consult.
- `USiegeHitFlashComponent::TriggerFlash` (`SiegeHitFlashComponent.cpp:53`) applies `M_HitFlash` via **`SetOverlayMaterial`** (`:83`) to every cached mesh where **`Mesh->IsVisible()`** (`:81`), for `HitFlashSeconds`.
- ⛔ **Damage does not break the veil, and that is correct** (`WITCH-§3` — being hit is not acting; `SummonedUnit.cpp:5081-5087` refuses a break there in writing).
- ✅ **No collision with `TASK-923`.** `OverlayMaterial` is a different property from `OverrideMaterials`; the two loops and the flash's two calls cannot overwrite each other, and `ClearFlash` (`:93`) restores independently. Verified, not assumed.
- ⭐⭐ **AND THE IRONY IS LOAD-BEARING: the flash reaches a veiled unit *precisely because* `TASK-923` correctly refused to hide the mesh.** `TriggerFlash` filters on `IsVisible()`, and a veiled unit's component is still visible — the veil is material-only, by law. ⇒ **the ruled failure direction has a cost, and this is it. That is not an argument against the ruling; it is the bill.**

### ⭐⭐ AND A THIRD, ON THE NEXT LINE, WHICH NOBODY REPORTED

`SummonedUnit.cpp:5078` — **immediately after** the flash, in the same block, on the same condition:

```cpp
USiegeFeedbackLibrary::ShowDamageNumber(this, ActualDamage,
    GetActorLocation() + FVector(0.f, 0.f, UnitDamageNumberHeightZ), USiegeFeedbackLibrary::TeamTint(Team));
```

`ShowDamageNumber` (`SiegeFeedbackLibrary.cpp:140-145`) spawns an `ADamageNumberActor` at the unit's world location with a **team tint**. ⇒ ⛔⛔ **a veiled unit caught in AoE floats a team-coloured NUMBER over itself for the rise/fade lifetime.** It is **worse than the flash**: it lasts longer, it is UI-bright, it leaks **whose** unit it is (the tint) and it leaks **how much HP it just lost** (the number).

### The ruling: ⛔ **BOARD — but board ONE row, not three.**

⚖️ ***The finding is not "there is a second tell". It is that three tells were found in one evening by three different people looking at three unrelated things — which means nobody has run the census that would find the rest.*** All three are downstream of one decision that is **correct** (`WITCH-§3`: damage does not break the veil), all three fire in the one lane `WITCH-§2` deliberately leaves un-suppressed (**AoE**, `J-W2`) — i.e. **exactly when a hidden push is being flushed**, which is the moment the veil is doing its most important work.

**What I am recommending to the manager, in place of two one-off rows:**

> ⭐ **ONE row: a census of every system that renders something ATTACHED TO, OVERLAID ON, or DERIVED FROM a unit's position, ruled against the veil one by one, behind the SAME per-viewer predicate (`FSiegeCombatStatics::IsAgentVisibleTo`).** Known members so far: **(1)** the floating health bar (`J-W17`) · **(2)** the hit flash (`SetOverlayMaterial`) · **(3)** the damage number (`ADamageNumberActor`, team-tinted) · **(4)** ⚠️ **the SHADOW — open, see N-6.** ⛔ **Three one-line hides written separately will disagree the first time the predicate moves; one predicate consulted from three sites will not.**

⛔ **None of this is `TASK-923`'s to fix and none of it blocks.** `SiegeHitFlashComponent.*` and `SiegeFeedbackLibrary.*` are not in its names list; it reported the flash correctly under `SC-§47` (*a fence limits the repair, never the report*), and finding the third one was my job, not its.

---

## FINDINGS

- **[WARN] W-1 — `Tests/SiegeInvisibilityTest.cpp:3293` — a NEW pin that a COMMENT EDIT can turn red, in the one file whose doctrine forbids exactly that.** The ordering probe uses `BreakCode.Find(TEXT("return; "))` — **needle with a trailing space**. It matches today only because `SummonedUnit.cpp:2916` carries a trailing `//` comment after the semicolon. ⛔ **Delete that comment and the needle returns `INDEX_NONE`, `EarlyOutAt != INDEX_NONE` fails, and row (2a-ii) goes RED with nothing in the behaviour to explain it** — the same species the file's own IMMUNITY row exists to prevent, and the same species that ate 16 real hits (`SC-§39`) and manufactured a phantom red (`SC-§41`) tonight. ✅ **It fails CLOSED (a red, never a false green), so it is not a blocker.** **Fix:** drop the trailing space — `TEXT("return;")` — or anchor on `TEXT("\treturn;")`.
- **[WARN] W-2 — `J-W17`, the floating health bar. BOARD IT** (RULING 1). Measured myself: 2 hits in the `.cpp`, 3 in the `.h`, **all prose**, and **zero** veil predicates across the pair; control `health` = 80. The relayed *"zero hits"* was wrong. ⛔ Not fixable here — `TASK-898`/`902`'s ungated tree, and the predicate is per-viewer.
- **[WARN] W-3 — the HIT FLASH and the DAMAGE NUMBER paint a veiled unit. BOARD AS ONE ROW WITH W-2** (RULING 3). `SummonedUnit.cpp:5076` and `:5078`, both unconditional on actual damage, both in the AoE lane. The damage number was **not** reported by the diff and is the more legible of the two.
- **[WARN] W-4 — an ANTICIPATORY comment reached LAW and nothing could have caught it** (RULING 2). `CombatantHealthBarComponent.h:585`, `Tests/SiegeCastBarTest.cpp:92` and **`CONVENTIONS.md` `WITCH-§9.1` row 4** all asserted, in the present tense, a behaviour that did not exist until tonight. ⛔ **No code change** — `TASK-923` retired it — but the law amendment in RULING 2 is worth writing, and `TASK-925`'s commit message must record it.
- **[WARN] W-5 — `WITCH-§9.0`'s promised removal row appears NOT to be boarded.** The law says the cancelled cast bar's geometry in `UCombatantHealthBarComponent` is *"boarded as its own row against the NEXT commit, never left to rot"*, but a board search for `ApplyCastRowGeometry` / `CastBarRoot` / `SiegeCastBarTest` returns only `TASK-860`-era rows, no removal row. ⛔ **Not `TASK-923`'s and not this gate's subject — but it is `WITCH-§5`'s own brand-new lesson recurring within one day: *declaring a gap is not boarding it.*** 📋 **Manager call.** ⚠️ It has an operational edge too: `TASK-925`'s spec already warns to **DECLINE** `WBP_CombatantHealthBar` if a `Restore Packages` modal offers it.

- **[NIT] N-1 — the handoff's line numbers are of TWO VINTAGES, and only one is flagged.** §3's seam table is **post-edit** and correct (`GrantInvisibility :2886` ✅, the break edge `:2919` ✅, `ApplyVeilMaterial :2951` ✅, `ClearVeilMaterial :3003` ✅). But §5a's citations are **pre-edit**, uniformly **21 lines low** and *not* marked stale: `LoadStatsAndStart` early-out cited `:1225` → **`:1246`**; `bStatsLoaded = true` cited `:1319` → **`:1340`**; the `ResolveSkeletalVisual` call cited `:1302` → **`:1323`**; `InitUnit`'s `LoadStatsAndStart` cited `:677` → **`:698`**. (Its own +21-line `InitUnit` comment caused the shift.) ⛔ **Every CLAIM is true — I re-verified all four by symbol** — in a document whose headline was `SC-§38`.
- **[NIT] N-2 — §4c's side-effect argument is right for a reason it slightly mis-names.** `EmptyOverrideMaterials()` resets **`MaterialSlotsOverlayMaterial`** (verified, `MeshComponent.cpp:268-270`) — the **per-slot** overlay array. The hit flash writes **`OverlayMaterial`**, a *different* member (`SetOverlayMaterial`). ⇒ the avoided collision is with an array **nothing in this project writes today**. ✅ **The choice stands on its general reason — *clearing state we did not set is how a fix grows a side effect* — and on the mirror-image argument, both of which I accept.** The judgement call was flagged for me; ⚖️ **I agree with it.**
- **[NIT] N-3 — the assertion count is UNDER-stated, which is the safe direction but is still a count.** The handoff and the board both say **13 assertions**. I counted the shipped test: **19 `TestEqual`/`TestTrue` calls — 16 substantive plus 3 self-checks — across 15 labelled rows.** `13` matches its own summary table's row count, not the code. Reported because `TL-§5c`/`SC-§40` are count-discipline laws and a gate that lets a wrong count through on the grounds that it is *pessimistic* has still let a wrong count through.
- **[NIT] N-4 — a null mesh asset veils silently and invisibly.** `GetNumMaterials()` returns 0 for a component with no mesh set, so the apply loop is a no-op and the unit stays visible with **no log** (the `Warning` covers only a failed material resolve). ⛔ Unreachable today — the mesh is bound long before any 3-second cast — and it fails in the ruled direction. Recorded only so the next reader does not mistake a silent no-veil for a broken material.
- **[NIT] N-5 — the first completed cast of a process does a game-thread `LoadSynchronous`.** Precedented exactly by `ApplyTeamMaterial` (`:352`), one small material instance, once per process, on an event telegraphed for 3 seconds. Not worth changing; recorded because it is the only new blocking call in the diff.
- **[NIT] N-6 — an OPEN PIXEL QUESTION nobody has asked: does a veiled unit still cast a SHADOW?** I searched: **no `SetCastShadow` / `bCastShadow` anywhere on the unit path**, and **`TASK-832` never mentions shadows** (zero hits in its handoff). A `BLEND_Translucent` + `MSM_Unlit` material almost certainly drops out of the shadow depth pass — i.e. the shadow probably vanishes, which is the *desirable* answer — but ⛔ **that is a prediction, not a measurement, and a unit-shaped shadow with no unit in it would be the most legible tell of the four on a top-down camera.** ⛔ Not a blocker, ⛔ not `TASK-923`'s to fix (adding `SetCastShadow(false)` unmeasured would be a fourth behaviour needing its own restore). ⇒ **one look, at `TASK-925`'s PIE pass, as a third row beside P-1 and P-2** (see below).

---

## `TL-§5c` — SUITE CENSUS

- **Measured at my own instant**, scoped to `^IMPLEMENT_SIMPLE_AUTOMATION_TEST` over `Siegebound/Tests/*.cpp`: **432 across 31 files** — agreeing with the declared figure, with `SiegeInvisibilityTest.cpp` at **33** (was 32).
- ⛔⛔ **`declared`, ⛔ NOT a pass count. I ran no suite and no compile.** Last **EXECUTED** remains **`427 / 0` at `1aa0fee`**.
- ⚠️⚠️ **`TASK-919`'s commit build is running now and is the first execution since. ITS RESULT SUPERSEDES EVERY FIGURE IN THIS REPORT.**
- ⚠️ **One honesty note on the reconciliation:** my `432` is a census of the **working tree**, which carries another lane's uncommitted `Tests/SiegeAssistantSelectionTest.cpp`, `Tests/SiegeControlsHelpTest.cpp` and `Tests/SiegePlacementTest.cpp`. `432 == 431 + 1` balances **only if the briefed `431` was taken at the same tree state**, which I cannot verify from here. ⛔ **`TASK-925` reconciles against its own fresh census, not against this number** (`TL-§5b`).

---

## NOTES FOR BUILD-MASTER (`TASK-925`)

1. ⛔ **Ordering is HARD, not courtesy: `TASK-922`'s PAIR must be in `HEAD` first** — `MI_Unit_Invisible.uasset` **and** `M_HeroSpirit.uasset`. The code references the instance **by soft path**, which **compiles against an asset that does not exist** and fails only at runtime. Commit the code first and `HEAD` carries a dangling reference in the worst available ordering.
2. **Pathspec is exactly three files:** `SummonedUnit.cpp` · `SummonedUnit.h` · `Tests/SiegeInvisibilityTest.cpp`. ⛔ **Confirm against `git status`, never against the handoff line** — other lanes are dirty in the same tree (§25b cl. R).
3. ⛔ **`SummonedUnit.{h,cpp}` is a HOT FILE — serialise against any other `Source/**` writer** (`QUIET-MODULE`).
4. ⭐⭐ **THE PIE ROWS ARE THE ONLY THING IN THIS PIPELINE THAT CAN SEE A PIXEL. The suite cannot, and this report says so on its first page.** Run P-1 and P-2 as the row specifies (no opaque speck; comes back opaque **and in its own team's colour**), on a **normal unit**, ⛔ **never the Witch** — she is measured as the weakest case and would read as a failure of correct code.
   - ⭐ **Add one row, from N-6: (P-3) DOES THE VEILED UNIT STILL CAST A SHADOW?** It costs one glance and it is unmeasured by anyone.
5. **Commit message must say:** the veil material is **now wired** — apply on grant, restore on break, **every slot** — naming `TASK-923`; **before this commit `MI_Unit_Invisible` had ZERO callers**; and that it **closes a residual `TASK-860` had already written into a comment as if it were mitigated** (RULING 2).
6. ⛔ **Carry forward BY NAME to `TASK-907`:** the skeletal-mesh PIE condition is **still open and is not a pass**. Two riders for that row: **(a)** a rig whose SK asset ships a different slot count veils a different number of slots — silently and correctly, by design; **(b)** ⚠️ **the veil paints `GetActiveVisualMesh()` only.** The shipped C++ class creates exactly two mesh components (`VisualMesh` `:176`, `SkeletalVisualMesh` `:195`) and exactly one is visible at a time, so this is airtight **today** — but ⛔ **I did not measure whether any `BP_Unit_*` subclass adds a third mesh component, and I had no sound instrument for it** (the `.uasset` name-table probe is inconclusive in both directions — `TASK-923` §7.5's own lesson, learned off a dead `strings` binary). ⭐ **A rigged witch carrying her staff as a separate component would ship an opaque staff over a ghostly body — the "chrome hat" defect one level up, at the COMPONENT rather than the SLOT.** That belongs in `TASK-907`'s spec as a check, not here as a blocker.
7. ⚠️ **If a `Restore Packages` modal appears: READ THE LIST. If `WBP_CombatantHealthBar` is offered, DECLINE** — the cancelled cast bar (`WITCH-§9.0`), and see W-5.

---

## VERDICT

✅ **PASS — 0 BLOCKERS.** `TASK-923` does what `WITCH-§5`'s application clause and `WITCH-§6`'s two file-map rows require: **every slot, never slot 1; both edges; re-derived, never cached; slot 0 back through the shipped `ApplyTeamMaterial()`; two private painters with one caller each that never read `bIsInvisible`; failure to a VISIBLE unit; no witch-specific compensation; no third site.** The closure pin is still **2**, both painter counts are **3**, and the engine read-back reproduces independently in UE 5.8 source.

⛔ **AND THE SENTENCE THAT MUST TRAVEL WITH THE PASS: A GREEN SUITE HERE MEANS THE SWAP IS WIRED. IT DOES NOT MEAN THE WITCH LOOKS INVISIBLE. THAT IS JONATHAN'S EYE AND `TASK-907`'s PIE PASS, AND THREE POSITIONAL TELLS (BAR · FLASH · DAMAGE NUMBER) STILL SIT BETWEEN THE WIRING AND THE ILLUSION.**
