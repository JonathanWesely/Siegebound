# TASK-923 — [VEIL-SWAP] WIRE THE VEIL MATERIAL TO THE VEIL STATE — programmer handoff

**Status:** `ready-for-qa` · **Gate:** `TASK-924` · **Integration:** `TASK-925`
**Law:** ⭐⭐ `WITCH-§5` (application clause + swap-row clause) · ⭐⭐ `WITCH-§6` (swap seam + restore rows) · `WITCH-§0`/`§2`/`§3` · `GHOST-§5` · `SC-§38` · `SC-§39.1` cl. 7 + cl. 8 · `SC-§40` cl. 3/9 · `SC-§47` · `SHIP-§9` · `TL-§5c` · `HIGH-§1`
**Fence honoured:** ⛔ `Source/**` only. ⛔ Zero `Content/` writes · ⛔ no geometry/ORM/FBX/rig/`MI_Witch_PBR` · ⛔ `CombatantHealthBarComponent.*` untouched · ⛔ `SiegeInvisibilityStatics.*` untouched · ⛔ no `DT_Cards` · ⛔ **no compile, no editor, no MCP, no Git.**

---

## 1. WHAT SHIPPED, IN ONE PARAGRAPH

`MI_Unit_Invisible` now has a caller. `ASummonedUnit::GrantInvisibility()` paints it over **every** material slot of `GetActiveVisualMesh()` on `ApplyVeil`'s **false→true** edge; `ASummonedUnit::BreakInvisibility()` takes it off inside its **post-early-out edge branch** (`ApplyBreak`'s **true→false** transition, which fires exactly once per veil) by clearing the override on every slot and then calling the **shipped `ApplyTeamMaterial()`** so slot 0 comes back in the unit's **current** team colour. Two new **private** helpers do the painting — `ApplyVeilMaterial()` / `ClearVeilMaterial()` — each with **exactly one caller**, and neither reads `bIsInvisible`: they consume the doors' **return values**, so the swap added ⛔ no state, ⛔ no cache, ⛔ no tick, ⛔ no timer and ⛔ no third site. One new automation test (**+1**, 13 assertions) is built against seven **named wrong implementations**, each of which compiles and reviews plausibly.

---

## 2. FILES TOUCHED — ⛔ EXACTLY THREE

| file | hunks | what |
|---|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.cpp` | 6 (+181) | the two seams · the two helper bodies · one prose-only forward-hazard note at `InitUnit` (§7.3) · two present-tense comment corrections (§9) |
| `Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.h` | 2 (+59) | the two **private** declarations (in the same `private:` block as `ApplyTeamMaterial`, beside `GetActiveVisualMesh`) · one 7-line amendment to the public veil block's *"three functions, no fourth"* note |
| `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeInvisibilityTest.cpp` | 1 (+243) | **+1 test**, appended at the tail |

⚠️ **The test file is the one path outside the row's `names` list, and it asked for it:** item (7) makes tests a deliverable, and `WITCH-§6` + `TASK-827`/`829` name `SiegeInvisibilityTest.cpp` as **the frame for this whole batch**. `git diff --unified=0` shows **one hunk**, at the tail — nothing else in that file is mine.

📌 **`TASK-925`, READ THIS BEFORE YOU STAGE.** The working tree carries **another lane's** edits to `SiegeControlsHelpWidget.{h,cpp}`, `SiegePlayerController.{h,cpp}`, `Tests/SiegeAssistantSelectionTest.cpp`, `Tests/SiegeControlsHelpTest.cpp`, `Tests/SiegePlacementTest.cpp`. ⛔ **None of those are mine.** My pathspec is the three rows above and nothing else — confirm against `git status`, never against this line.

---

## 3. ⭐ THE SEAMS — LOCATED BY SYMBOL (`SC-§38`), AND THE ROW'S NUMBERS HAD ALREADY DRIFTED

I used **zero** line numbers from the board. Measured at source:

| seam | symbol | board said | ✅ where it actually was | **now, post-edit** |
|---|---|---|---|---|
| APPLY | `ASummonedUnit::GrantInvisibility()` | `~:2855` | **`:2855`** ✅ | **`:2886`** |
| RESTORE | `ASummonedUnit::BreakInvisibility()`'s edge branch | `~:2873` | **`:2873`** ✅ | **`:2919`** |
| the call the restore must make | `ASummonedUnit::ApplyTeamMaterial()` | `~:326` | **`:326`** ✅ | `:326` |
| the mesh accessor | `ASummonedUnit::GetActiveVisualMesh()` | `~:358` | **`:358`** ✅ | `:358` |
| the latch | `ASummonedUnit::ResolveSkeletalVisual()` | `~:370` | **`:370`** ✅ | `:370` |
| its caller | `ASummonedUnit::LoadStatsAndStart()` | `~:1302` | **`:1302`** ✅ | `:1302` |

📌 **Stated precisely, because an inflated claim is worse than none: every citation in the row was still ACCURATE when I read it.** `SC-§38` did not have to save me this time. ⭐ What it did do is make the method right anyway — I opened each named function and found the quoted expression, so a drift would have been a non-event. ⚠️ **And the two numbers above are already stale for the next reader: my own edits moved both** (`+31` / `+46`). ⛔ `TASK-924` must verify **by symbol**; a gate that checks these numbers will report correct wiring as a defect.

**New symbols (all private, all in `SummonedUnit.cpp`):** `ASummonedUnit::ApplyVeilMaterial()` **`:2951`** · `ASummonedUnit::ClearVeilMaterial()` **`:3003`**.

---

## 4. THE DESIGN DECISIONS THAT ARE MINE (⚠️ QA: these are the rulings to scrutinise)

### 4a. Two private helpers, ⛔ not inline loops in the doors
`WITCH-§6`'s file map names **two** rows — *the swap seam* and *the restore* — and they are genuinely different shapes (one resolves an asset, the other re-derives through a shipped function). Inlining both would have grown the doors from *"two functions, zero rules"* into two 25-line bodies, and would have made the **no-third-site** property uncheckable. As helpers, it is checkable: each symbol appears **exactly 3 times** tree-wide (declaration + definition + one call) and the suite pins that number.

### 4b. ⭐⭐ The helpers read the doors' RETURN VALUES, ⛔ never `bIsInvisible`
This is not a style choice, and it is the reason a pre-existing suite pin survived untouched. `SiegeAcquisitionFunnelTest`'s **closure pin** asserts the raw flag is handed to a call **exactly twice** in `SummonedUnit.cpp` — *"it also fires on a raw parenthesised READ of the flag; that over-catch is DELIBERATE"*. A helper that consulted the flag would have been a third mention and turned that row red. Consuming the edge the door already returned is both the cheaper design and the one the existing gate was built to require. **Measured: the count is still exactly 2.**

### 4c. `SetMaterial(i, nullptr)` for the clear, ⛔ not `EmptyOverrideMaterials()`
Both are *"named candidates, not a ruling"* per the row, so I measured them (§5). `SetMaterial(i, nullptr)` was chosen because `EmptyOverrideMaterials()` **also** does `MaterialSlotsOverlayMaterial.Reset()` (`MeshComponent.cpp:266-271`) — a different array this feature never wrote. ⛔ Clearing state we did not set is how a fix grows a side effect. The chosen loop touches exactly what the apply loop touched and is visibly its mirror image.

### 4d. The shared MI is applied directly, ⛔ not through a per-actor MID
`ASiegeGhostPawn` (the whole-body precedent this row names) sets the **instance** on every slot; `ASiegePlayerController`'s placement ghost creates a **MID** because it drives a colour parameter per validity state. The veil drives **no** parameter — `TASK-832` records `TeamColor` as *"the per-team lever **if** integration ever wants one"*, and `WITCH-§5`'s own clause 2 rules that losing the team read on a veiled unit is **acceptable** (it is the owner's own unit) and says ⛔ *"do not add a second material"*. ⇒ one shared MI, no per-actor allocation, no divergence.

### 4e. `GrantInvisibility()` returns the FLAG's edge, ⛔ never the material's
A missing `MI_Unit_Invisible` leaves a unit that is genuinely veiled to enemy acquisition and merely **looks normal**. Returning `false` there would make the material a **second source of truth** about the veil, which item (4) and `WITCH-§6` forbid — and it would make `CompleteWitchCast`'s log lie about whether the cast landed.

### 4f. The apply is GUARDED by the edge; the clear is UNCONDITIONAL inside its branch
`GrantInvisibility` paints only when `ApplyVeil` reported the transition, so `WITCH-§4`'s *"never target an already-invisible unit"* belt stays a **total** no-op instead of costing a full material re-stamp. `ClearVeilMaterial` calls `ApplyTeamMaterial()` **outside** its own null-mesh guard, because slot 0 restoration is ⛔ not optional and `ApplyTeamMaterial` carries its own identical guard.

---

## 5. ⭐⭐ ITEM (3)'s TWO DEMANDED MEASUREMENTS — BOTH TAKEN, BOTH REPORTED

### 5a. ⛔ *"MEASURE whether `LoadStatsAndStart` can run WHILE VEILED"* — ✅ **IT CANNOT, and the row's reading was right for a reason the row did not give**

The row's citation was *"a veil requires a 3-second cast, long after `LoadStatsAndStart`"*. ⭐ **That is true but it is not the load-bearing reason, and I would not want the gate to rest on it.** The structural reason is stronger:

| link | measured |
|---|---|
| `ResolveSkeletalVisual()` callers, tree-wide | **exactly one** — `LoadStatsAndStart:1302`. It also self-latches (`if (bUsingSkeletalVisual …) return;` at `:375`) ⇒ the component can change **at most once, ever** |
| `LoadStatsAndStart()` callers | **two** — `BeginPlay:285` and `InitUnit:677`. Both are gated by `if (bDead \|\| bStatsLoaded \|\| bAIFrozen) return;` at `:1225` and it sets `bStatsLoaded = true` at `:1319` ⇒ **the body runs at most once per unit** |
| `InitUnit()` callers, tree-wide | **two** — `ABarracks::SpawnUnit` (`Barracks.cpp:152`) and `ASiegePlayerController` (`SiegePlayerController.cpp:4869`). ⛔ **Both are `SpawnActorDeferred → InitUnit → FinishSpawning`** |

⇒ ✅ **`GetActiveVisualMesh()` cannot return a different component while a unit is veiled.** ⛔ **But the cache stays banned anyway** — the ban is about the *shape being unrepresentable*, not about today's call graph, and §7.3 shows the graph is one added function away from moving.

### 5b. ⛔ *"READ BACK `GetMaterial(i)` after and report what it returned"* — ✅ **DONE, and I am naming the instrument because it is NOT the one the row imagined**

⚠️ **I could not run the editor (fenced), so this is a ⛔ STATIC read-back off UE 5.8's own source, ⛔ not a live observation.** Said plainly rather than dressed up. ⭐ In exchange it covers **both** component types at once, which one editor observation would not:

| step | engine source | what it does |
|---|---|---|
| the write | `Engine/Private/Components/MeshComponent.cpp:63`, assignment at **`:99`** | `SetMaterial(i, nullptr)` sets `OverrideMaterials[i] = nullptr` — it **does not remove the entry** (it will even grow the array to reach `i`) |
| the read, **static** mesh | `Engine/Public/StaticMeshComponentHelper.h:140` | `if (OverrideMaterials.IsValidIndex(i) && OverrideMaterials[i])` → a **null** entry fails the second test → falls through to `Component.GetStaticMesh()->GetMaterial(i)` |
| the read, **skeletal** mesh | `Engine/Public/SkinnedMeshComponentHelper.h:110` | identical test → falls through to `SkinnedAsset->GetMaterials()[i].MaterialInterface` |

⇒ ✅ **`GetMaterial(i)` after the clear loop returns the ⛔ ASSET's authored material — ⛔ not `nullptr`, and not the veil — on either component type.** That is the behaviour the restore needs, and it is why step (i) is correct before step (ii) re-stamps slot 0.

⚠️ **`EmptyOverrideMaterials()` would have produced the same *material* result** (`MeshComponent.cpp:246`, `OverrideMaterials.Reset()`), so this is a real choice between two working options — decided on the **side effect**, per §4c.

⛔ **WHAT THIS READ-BACK DOES ⛔ NOT PROVE: that anything is on screen.** See §8.

---

## 6. ⚠️⚠️ THE OPEN CONDITION — ⛔ RESTATED VERBATIM AS ITEM (6) DEMANDS, ⛔ AND CARRIED FORWARD BY NAME TO `TASK-907`

> ⛔⛔ **NOT PROVEN ON A SKELETAL MESH IN PIE.** `TASK-832` tested on **STATIC** actors because **`SK_Witch` DOES NOT EXIST**. `bUsedWithSkeletalMesh = true` was **read back** and the ghost exercises the same master on `SKM_Quinn_Simple` — ⛔ **that is MITIGATION, ⛔ NOT PROOF: `GHOST-§5`'s defect is INVISIBLE IN-EDITOR and appears only in a PACKAGED build.**

⛔ **THIS IS ⛔ NOT A PASS AND I HAVE ⛔ NOT WRITTEN IT AS ONE.** ⭐ **`TASK-907` (the witch-rig integration) is the ⛔ FIRST row at which a skeletal veil can EXIST AT ALL, and it is the row that must close this.** My code makes it *reachable* — `GetActiveVisualMesh()` returns the skeletal component once the swap takes, and **one** code path covers both — but "reachable" is not "rendered".

📌 **A second thing for `TASK-907`, which my diff made true and which is worth one line in its spec:** the first unit in this project ever to receive a **skeletal** veil will do so through `ApplyVeilMaterial`'s `GetNumMaterials()` loop, and a skeletal mesh's slot **count** is authored on the SK asset — ⛔ so a rig that ships with a different slot count than `SM_Witch` veils a **different number of slots**, silently and correctly. That is the loop working as designed; it is recorded so nobody reads it as drift.

---

## 7. ⚠️⚠️ FINDINGS OUTSIDE MY FENCE — ⛔ NOT FIXED, ⛔ REPORTED (`SC-§47`: a fence limits the repair, never the report)

### 7.1 ⛔⛔ `J-W17` — THE BOARDED MEASUREMENT IS ⛔ WRONG. THE CONCLUSION SURVIVES; THE EVIDENCE DOES NOT.

The row states, twice, that ***"`grep -i invisib` over `CombatantHealthBarComponent.cpp` returns ⛔ ZERO hits"*** and offers it to `TASK-924` as **measured input** for a ruling. `SC-§40` cl. 9 says every consumer of a relayed count re-measures. I did:

| needle | file | measured |
|---|---|---|
| `invisib` (case-insensitive) | `CombatantHealthBarComponent.cpp` | ⛔ **2**, ⛔ not 0 |
| `invisib` (case-insensitive) | `CombatantHealthBarComponent.h` | ⛔ **3** |
| **positive control** `health` | `CombatantHealthBarComponent.cpp` | **80** ⇒ the instrument is alive |

✅ **The RULING is unaffected and I want that said first:** all five hits are ordinary prose (*"this must be invisible to them"*, *"a 1.5 px shortfall — invisible"*). ⛔ **There is still no veil predicate, so a veiled unit's floating bar still stays up.** ⇒ `TASK-924` should still rule, and the right predicate is still the per-viewer `FSiegeCombatStatics::IsAgentVisibleTo(ETeamId, const AActor*)`.
⚖️ ***But a gate asked to rule on "an ABSENCE, measured" must not be handed a number that is wrong by five. `SC-§40`'s whole point is that a law's or a manager's count is a CITATION, not a fact — and this is that law catching its own section.***

### 7.2 ⭐⭐ AND THE FIFTH HIT IS ⛔ NOT INCIDENTAL — `CombatantHealthBarComponent.h:585` ALREADY DEPENDS ON MY DIFF

`TASK-860`'s cast row declares a residual and mitigates it **with the thing that had no caller until today**:

> *"an interrupt landing inside the FINAL poll window (≤50 ms of a 3 s cast) still paints as 'nearly full'. That is 1.7% of the window and it is **disambiguated by the other half of the signal — a COMPLETED cast also turns the target translucent (`MI_Unit_Invisible`)**, and a broken one changes nothing."*

⛔⛔ **Before this commit that sentence was FALSE** — a completed cast turned the target translucent **nowhere**, so the cast bar's declared 1.7% ambiguity had **no** second signal and was simply an unmitigated hole. ⇒ ⭐ **`TASK-923` silently closes a residual boarded against a different task, and `TASK-925`'s commit message is the right place for that to become visible.** ⚖️ *This is `WITCH-§5`'s own durable sentence arriving from the other direction: an unwired deliverable was already being **relied upon in prose** by a second lane.*
⚠️ **One thing I cannot adjudicate and am flagging rather than resolving:** `WITCH-§9.0` records the cast bar as **CANCELLED and reversed by Jonathan**, and `TASK-832 §10` calls `WBP_CombatantHealthBar` *"the cancelled cast-bar work"* — yet the C++ cast row is **shipped and live** in `CombatantHealthBarComponent.{h,cpp}`. ⛔ Not mine, not touched. 📋 **Manager/QA call.**

### 7.3 ⚠️ A FORWARD HAZARD ON `InitUnit` — ⛔ UNREACHABLE TODAY, ⛔ MEASURED, ⛔ PROSE-ONLY REPAIR

`InitUnit:655` calls `ApplyTeamMaterial()` behind `if (HasActorBegunPlay())` — the *"re-team a live unit"* belt. ⛔ **On a VEILED unit that would stamp an OPAQUE `MI_TeamColor_<Team>` through the veil — literally `WITCH-§5`'s "opaque chrome hat over a ghostly body", arriving through a door nobody was watching.**
✅ **Measured unreachable: both shipped `InitUnit` callers are `SpawnActorDeferred → InitUnit → FinishSpawning`, so `HasActorBegunPlay()` is FALSE and the branch has ZERO reachable call sites** (§5a).
⇒ ⛔ **I shipped ⛔ NO guard.** A veil consult there would be a **THIRD site** at which a unit's look changes for the veil, which item (1) and `WITCH-§6` refuse — and a third site has to be **ruled**, not slipped in. ⭐ **What I shipped is a comment at that exact line**, so the next person to add a live re-team path is standing on the warning. ⚠️ **That comment is the one edit outside the two seams; QA should judge it as prose, and it is provably prose — `CountOccurrencesInCode` skips comment lines, so it cannot move any pinned count.**

### 7.4 ⚠️⚠️ A NEW POSITIONAL TELL IN THE SAME FAMILY AS `J-W17`: ⛔ THE HIT FLASH PAINTS A VEILED UNIT

`ASummonedUnit::TakeDamage` calls `HitFlashComponent->TriggerFlash()` on **actual damage** (`SummonedUnit.cpp:~4915`), and `USiegeHitFlashComponent::TriggerFlash` applies `M_HitFlash` as an **overlay** to *"every VISIBLE static/skeletal mesh"*. ⛔ **Taking damage does NOT break the veil** (`WITCH-§3` — being hit is not acting). ⇒ ⛔⛔ **a veiled unit under fire FLASHES WHITE for ~0.1 s** — and it fires precisely in the one lane `WITCH-§2` deliberately leaves un-suppressed (**AoE**, `J-W2`), i.e. exactly when a hidden push is being flushed.
✅ **It does ⛔ NOT collide with my code, and I checked rather than assumed:** the flash writes `OverlayMaterial` via `SetOverlayMaterial`, a **different property** from `OverrideMaterials` — my two loops and its two calls cannot overwrite each other, and its own `ClearFlash` restores independently. ⛔ `SiegeHitFlashComponent.*` is not in my names list ⇒ ⛔ **not touched.** 🧑 **Suggest boarding it beside `J-W17`** — same shape, same viewer question, and probably the same one-line answer.

### 7.5 ⚠️ A DECLARED RESIDUAL THE LAW ⛔ FORCES, NOT A DEFECT: a BP-authored override on a slot ≥1 would not survive a veil
The restore drops every slot back to the **ASSET's** material (§5b). ⛔ If a unit Blueprint authored a **component-level** override on a non-zero slot, that override is gone after the unit's first veil-and-break. ⛔ `EmptyOverrideMaterials()` has the **identical** failure, and the only implementation that preserves it is a **cache — which `WITCH-§6` bans, twice over.** ⇒ ⛔ **there is no compliant alternative; this is a law-forced residual and I am declaring it rather than letting QA find it.**
⚠️ **I could ⛔ not settle it headlessly and I am ⛔ not claiming I did.** A binary probe found the literal `OverrideMaterials` in 12 of 13 `BP_Unit_*.uasset` (absent only in `BP_Unit_Wizard`) — ⛔ **but a `.uasset` name table lists every property name a package references, so that is ⛔ inconclusive in both directions.** 🪤 **And the probe that produced the first, cleaner-looking answer was DEAD:** my initial sweep used `strings`, which **does not exist in this shell**, and it returned a confident **0 for every file**. ⛔ Caught only by a positive control. ⚖️ *`SC-§39` again, self-inflicted: a missing binary and a clean result are the same output.* ⭐ **Narrowness that makes this acceptable: slot 0 — the only slot the shipped code writes — is re-derived correctly by construction.**

---

## 8. ⛔⛔ TESTS — AND THE HONESTY IS THE DELIVERABLE, ⛔ NOT THE COUNT

### ⛔ WHAT IS ⛔ NOT HEADLESSLY TESTABLE — ⛔ SAID PLAINLY, AND THIS ROW IS **DECLARED PIXEL-GATED**
⛔ **No automation test in this project can prove the veil LOOKS like anything.** A material swap is a **rendering** fact: it needs a mesh, a compiled material, a scene and a frame. There is ⛔ not one `UWorld::CreateWorld` and ⛔ not one `SpawnActor` anywhere in `Siegebound/Tests/` (the house rule) — and even with one, `TASK-832` measured that **this exact effect is invisible to every instrument except a region-restricted frame diff against a strength-0 control**: a thumbnail could not see it, four configurations returned every green success value while refracting **nothing**, and the artist could not see it by eye either.
⇒ ⛔⛔ **GREEN HERE MUST ⛔ NEVER BE REPORTED AS *"invisibility works"*** (`SC-§32`). It reports: **the swap is wired, on both edges, on every slot, and nobody has quietly removed it.** The pixel belongs to Jonathan's eye and to `TASK-907`'s PIE pass.

### ✅ WHAT ⛔ IS TESTABLE — AND ⛔ WHY IT IS NOT THE SIXTH BLIND INSTRUMENT
Item (7) forbids a test that *"merely asserts the material path string, calls it coverage, and moves on"*. ⛔ **Every row is built against a NAMED wrong implementation that COMPILES and reviews plausibly**, and the path-string row is the **sixth of seven**, carrying a claim the string alone does not (that there is exactly **one** of it):

| # | goes RED when | ⚖️ the defect it catches |
|---|---|---|
| **1a** | ⭐⭐ **the swap is removed** | ⛔ **the exact state `TASK-923` existed to end** — six green tasks and nothing applying the material |
| 1b | the paint is unguarded | a wasted cast costs a full material re-stamp |
| **2a** | ⭐⭐ **the restore is removed** | ⚖️ **worse than no veil** — all six break reasons silently dead |
| 2a-ii | the restore is hoisted above the early-out | every unveiled unit re-stamps its materials on every swing |
| **2b** | `ApplyTeamMaterial()` is dropped | ⚖️ **a RED unit comes back BLUE** — an enemy in your half wearing your colour |
| 2c | the team resolve is re-implemented instead of called | two copies of the paths, disagreeing on the first move |
| **3a/3b/3b-ii** | ⭐⭐ **a slot index is hard-coded** | ⚖️ **the opaque chrome hat on a ghostly witch** |
| 3c/3d | the clear covers fewer slots than the apply | the veil is stranded on the slots the clear missed |
| **4** | `SetVisibility` / `SetHiddenInGame` appears in the apply | ⚖️ **an unkillable invisible SOLID that still blocks placement and still deals damage** (`WITCH-§0`) |
| **5** | a cached `TArray<UMaterialInterface*>` appears | restored onto the **wrong component** once the skeletal swap has taken |
| **6** | ⭐ **a THIRD painting site is added anywhere** | the grep-is-complete property the whole design rests on |
| 7 | the pinned path is duplicated or renamed | a second copy of the path to keep in sync |

⭐ Every extraction carries a **self-check** (a stale signature FAILS rather than reporting a vacuous zero — an empty body and a deleted swap look identical otherwise), the ordering probe runs on **code lines only**, and the tree-wide scans reuse the fixture's `CountAcrossShippingSource`, which already refuses to run on fewer than 20 files.

### ⛔ REGRESSION CHECK ON ⛔ OTHER TESTS' PINS — RUN, NOT ASSUMED
⚠️ I re-implemented `CountOccurrencesInCode` / `CodeLinesOnly` / `ExtractFunctionBody` **exactly** (comment-line skipping included) and ran every pin my diff could touch against the real files. **All green**, including the ones I could plausibly have broken:

- `FSiegeInvisibilityStatics::ApplyVeil(` = **1** · `ApplyBreak(` = **1** · `(bIsInvisible` = **2** (the closure pin) — **unchanged**.
- ⭐ **PROSE-IMMUNITY holds for all three** (with-comments count == code-only count). 🪤 **This nearly bit me:** my first draft of the `InitUnit` note contained the literal `(bIsInvisible` **inside a comment**, which would have turned `SiegeAcquisitionFunnelTest`'s immunity row **red from a comment**, with nothing in the behaviour to explain it. Caught before shipping; the note now names the **accessor** and says why.
- All **13** `PermanentlyIsEnforcedByShapeNotByComment` banned names = **0** on code lines. ⛔ `ApplyVeilMaterial` / `ClearVeilMaterial` were chosen against that list on purpose — `RestoreVeilMaterial` would have contained the banned `RestoreVeil` and turned the suite red.
- Inlined `bIsInvisible =` assignments = **0**. Header `bool GrantInvisibility()` = **1**.

### ⛔ `TL-§5c` — SUITE CENSUS
- **My delta: `+1` test, `+0` files.** (13 assertions, one new `IMPLEMENT_SIMPLE_AUTOMATION_TEST`.)
- **Tree now reads `432` declared / `31` files** — both needles agree, and it is exactly the briefed `431/31` plus my one.
- ⛔ **`declared`, ⛔ NOT a pass count.** ⛔ I ran ⛔ no suite (no compile in my lane). Last **EXECUTED** remains **`427/0` at `1aa0fee`**; `TASK-925` reports the executed figure.

---

## 9. ⭐ PRESENT-TENSE PROSE CORRECTED (`SC-§39.1` cl. 7) — ⛔ THE ROW ASKED FOR ONE, I FOUND THREE

| where | said | now |
|---|---|---|
| `SummonedUnit.cpp:2879` (the 📌 the row named) | *"The MATERIAL swap … hangs off this same edge and is ⛔ NOT TASK-829's"* | ⛔ **deleted and replaced** by the wired call + why it is half the feature. ⛔ Leaving it would have shipped *"not wired"* inside the artefact that wires it |
| `SummonedUnit.cpp:2841` block header | *"TWO FUNCTIONS, ⛔ ZERO LOGIC"* | *"ZERO **RULES**"* + an explicit note that each door now hands its **already-computed** edge to one painter, and that the material is a consequence of the flag, never an input |
| `SummonedUnit.h:439` public veil block | *"THREE FUNCTIONS, AND THERE IS DELIBERATELY ⛔ NO FOURTH"* | **kept true, and annotated**: still no fourth *public* door, still nothing that takes a TIME — plus a pointer to the two **private** painters so the count is not read as stale |

---

## 10. ⛔ WHAT QA SHOULD SCRUTINISE HARDEST

1. **§4b — the helpers not reading `bIsInvisible`.** If you disagree, it is the highest-consequence design line in the diff: it is what keeps the funnel test's closure pin at 2 and what keeps the material a consequence rather than an input.
2. **§4c — `SetMaterial(i, nullptr)` vs `EmptyOverrideMaterials()`.** Both work; I chose on a **side effect** I read out of engine source. ⛔ If you think the overlay-array reset is harmless, say so — it is a real judgement call and it is mine.
3. **§7.1 — please rule on `J-W17` with the CORRECTED measurement** (5 hits, not 0; conclusion unchanged). ⛔ The row hands you a number that is wrong.
4. **§7.2 — the cast bar's live dependency on this diff, and the `WITCH-§9.0` cancelled-vs-shipped contradiction.** ⛔ Not mine to adjudicate.
5. **§7.3 — the `InitUnit` comment**, the one edit outside the two seams. Judge it as prose or tell me to remove it.
6. **§7.4 — the hit flash on a veiled unit.** ⛔ Out of fence; ⛔ board it or declare-and-defer beside `J-W17`.
7. **§7.5 — the BP-override residual**, including that my first probe was a **dead instrument** returning a clean zero.
8. **Item (5) compliance:** ⛔ **there is ⛔ no witch-specific anything in this diff** — no per-`CardID` branch, no second material, no compensating strength. The metallic weakness (`10.8 → 11.5`) is `TASK-899`'s consequence and is ruled **with `J-W15`**, never separately. ⛔ Please confirm by grep: `CardID` appears in my two helpers **only** inside log text.
9. **Compile risks I ⛔ cannot test (⛔ no compile in my lane):** `TSoftObjectPtr<UMaterialInterface>::ToString()` used as `*Ptr.ToString()` (mirrors `SiegePlayerController.cpp:2949`) · `SetMaterial(SlotIndex, nullptr)` resolving unambiguously to `UMeshComponent::SetMaterial(int32, UMaterialInterface*)` · both helpers declared in the **same `private:` block (`:1483`)** as `ApplyTeamMaterial` · ⛔ **no new `#include` was needed** — `Materials/MaterialInterface.h` and `Components/MeshComponent.h` are already at the top of the TU.

---

## 11. FOR `TASK-925` (integration)

- **Pathspec:** the three files in §2. ⛔ Confirm against `git status`; ⛔ other lanes are dirty in the same tree.
- **Prerequisite in `HEAD`:** `/Game/Materials/MI_Unit_Invisible` **and** `/Game/Materials/M_HeroSpirit` — `TASK-922`'s **pair**. ⛔ The instance alone would resolve to a master without the refraction input.
- **Commit message must say the veil material is ⛔ NOW WIRED** — apply on grant, restore on break, every slot — and name `TASK-923`. ⭐ One line for the next reader of `git log`: **before this commit, `MI_Unit_Invisible` had ZERO callers.** ⭐ A second line worth its space (§7.2): **it also closes a residual `TASK-860` had already written into a comment as if it were mitigated.**
- **Carry forward BY NAME to `TASK-907`:** §6 — the skeletal-mesh PIE condition, ⛔ still open, ⛔ not a pass.

---

## 12. ⭐ ADDENDUM 2026-09-03 — `TASK-924`'s **W-1** REPAIRED (one character)

**Fence honoured:** `Tests/SiegeInvisibilityTest.cpp` **ONLY**. ⛔ Zero production edits · ⛔ no compile · ⛔ no editor · ⛔ no MCP · ⛔ no Git.

### 12.1 The change

`Tests/SiegeInvisibilityTest.cpp` (`BreakInvisibility` ordering probe, located by symbol):

```cpp
- const int32 EarlyOutAt = BreakCode.Find(TEXT("return; "), ESearchCase::CaseSensitive);
+ const int32 EarlyOutAt = BreakCode.Find(TEXT("return;"), ESearchCase::CaseSensitive);
```

Plus a short trap comment above it recording the needle discipline, so the space cannot be re-added by the next reader.

### 12.2 ⭐⭐ Why it mattered more than its size

`CodeLinesOnly` drops whole comment **LINES**; it ⛔ does **NOT** strip a trailing `//` from a code line. `SummonedUnit.cpp:2916` reads:

```cpp
return; // the unit was already visible — no edge, no log, no work
```

⇒ the probe was matching **the presence of that unrelated end-of-line comment**, not the return. **Delete the comment and the row goes red with nothing in the behaviour to explain it.** This is `SC-§39`'s trailing-comment hazard appearing **inside the test written to guard against exactly that**, in the one file whose own doctrine forbids it. ⚖️ It **fails closed**, which is why QA correctly did not make it a blocker.

⛔ **`SummonedUnit.cpp:2916`'s comment was NOT touched** — the whole point is that the *test* must not depend on it.

### 12.3 ⛔ Discrimination VERIFIED (the row is not unfalsifiable)

A shorter needle can match somewhere new, so this was measured rather than assumed:

- **`ExtractFunctionBody` window is 2908–2939** — it cuts at the first column-0 `\n}`; lines 2915/2917 are tab-indented, so there is **no over-run**.
- **`grep -n 'return;' SummonedUnit.cpp` → within that window there is EXACTLY ONE hit: `:2916`.** Nearest neighbours are `:2855` (before) and `:2959` (after). ⇒ the match **position is unchanged**; only its *dependence on the comment* is gone.
- **`return;` cannot match a value-returning statement** — `return true;` / `return false;` / `return Count;` all carry a token between `return` and `;`. So the shortened needle is *more* precise in role (it names "a bare void return"), not less.
- **Both failure modes still go RED:** restore hoisted above the early-out ⇒ `ClearAt < EarlyOutAt` ⇒ false; early-out deleted entirely ⇒ `EarlyOutAt == INDEX_NONE` ⇒ false.
- **Same-role positive controls** — the file's other seven ordering probes (`:1105/1106`, `:1155-1157`, `:1229/1230`, `:1296/1297`, `:2008/2009`, `:2307/2308`, `:2652/2653`, `:2730-2732`) all use `Find(A) < Find(B)` over `CodeLinesOnly` with **terminator-anchored** needles (`(`, `;`, `()`). `return;` now conforms to that house pattern; `"return; "` did not.

### 12.4 ⚠️ Sibling sweep — the outlier was alone

**All 21 `.Find(TEXT(` needles in this file** end at a syntactic terminator. ⛔ `:3293` was the **lone** outlier. Swept the whole `Siegebound/Tests/` tree for needles carrying trailing whitespace:

| hit | verdict |
|---|---|
| `TEXT("* ")` in **7** tests + this file `:200` | ⛔ **NOT a defect — deliberate and load-bearing**, documented at `:194-195`: it distinguishes a `* text` doc-continuation from `*GetNameSafe(Foo)`, which starts a CODE line with the same character. |
| `SiegeAssistant*` / `SiegeWarMap` / `SiegeMapMark` (` ::= `, `ZONE   = `, `wizard <- `, `" kinds, "`) | ⛔ **NOT defects** — assertions about **rendered output text**; the spacing *is* the subject being asserted. |

### 12.5 ⚠️ `SC-§47` — out of fence, reported not fixed

`Tests/SiegePlacementTest.cpp:2313` — `CountOccurrencesInCode(HeaderText, TEXT("int32 DiscardCost = "))` is the closest **shape** match (a source-scanning needle ending in a space). ⚖️ **It is NOT the same hazard:** it depends on *initializer style* (`= 3;`), never on a comment, so no comment deletion can break it. Weaker than ideal, ⛔ but not `SC-§39`. **Flagging only — outside this fence.**

### 12.6 Census

⛔ `TL-§5c` — **delta 0, `declared`.** Row count unchanged at **`432` DECLARED**; this is a one-character repair of an existing row, ⛔ not a new one. Last **EXECUTED** = `432 Result={Success}` / `0 Result={Fail}` at `239ca77`.

**Reporting:** `ready-for-qa`, folded into **`TASK-925`'s pre-commit check** — ⛔ **no new gate row**; the gate that found it named the exact fix.
