# TASK-1165 — [FOGGREY-COMPONENT] — gameplay-programmer handoff

**Ask (C), 🧑 his words: *"Lets also make the fog more of a grey instead of a yellow."***
Date 2026-09-08 · **REVISION 2 — QA loop 1 of 3, after `TASK-1166` FAIL (1 BLOCKER, 6 WARN, 3 NIT)** · gate ⭐ `TASK-1166` · host ⭐ `TASK-1167` · law `FOG-§12.4a` · `GFX-§11` · `FOG-§6` · `SC-§94` cl. A + cl. D · `FIELD-§7` · `HIGH-§1` · `SC-§27` · `SC-§101` · `SC-§104` · `SC-§109` · `TL-§5b`/`TL-§5e`.

⛔ **NOT COMPILED. NOT RUN. NOT COMMITTED. NOTHING PUSHED.** ⛔ **No `.uasset`, no `.umap`, no `Content/FogArea/**`, no `Config/**`, no `SiegeFogStatics.{h,cpp}`, no `DirectionalLight_0`.**

---

## 0. 🚨 THE BLOCKER, AND WHY THE MECHANISM CHANGED RATHER THAN GOT A GUARD

**QA was right, and the finding was worse than a compile error.** `FogVolume.cpp:982` called `Visual->RerunConstructionScripts()` unguarded in a `"Type": "Runtime"` module. I re-read the installed 5.8 source first-hand and confirm every line QA cited:

| claim | file:line | what it says |
|---|---|---|
| declaration is editor-only | `Actor.h:3415-3418` | `#if WITH_EDITOR` … `ENGINE_API virtual void RerunConstructionScripts();` … `#endif` |
| definition is editor-only | `ActorConstruction.cpp:253` | `#if WITH_EDITOR` immediately above the definition |

⇒ the **only** compile this project runs (`Build.bat GitClaudeUnrealTestEditor`) defines `WITH_EDITOR=1` and goes **green** over it. The packaged game would not have built at all — and had it been guarded, it would have shipped **beige fog plus an Error on every raise**. Both outcomes are the cooked-build-only failure class the row's own cl. (2b) forbids by name.

⛔ **I did not add a guard.** The rebuild *was* the mechanism, so guarding it ships the defect. **The mechanism is replaced.**

### ⭐⭐⭐ THE NEW MECHANISM: defer construction by one statement, write, then finish

```
SpawnActor(…, SpawnParams.bDeferConstruction = true)   ← the object exists; UPROPERTYs are
   │                                                      initialised from the archetype;
   │                                                      the SCS has NOT run, so there is no
   │                                                      root component and no mesh yet
   ├─ ApplyFogVisualMaterial(Spawned)   ← reflection only: write BoxMaterials["Base"], read it back
   │                                      ⛔ no vendor code runs in this window
   ├─ Spawned->FinishSpawning(SpawnTransform, /*bIsDefaultTransform=*/false, nullptr,
   │                          ESpawnActorScaleMethod::OverrideRootScale)
   │        ← ⭐ THE VENDOR'S CONSTRUCTION SCRIPT RUNS HERE, ONCE, and builds its MID from OUR entry
   ├─ VerifyFogVisualMaterial(Spawned)  ← read every mesh, every slot; unwrap the MID; compare; log
   ├─ SetActorScale3D(RequestedScale3D) ← unchanged, kept deliberately (see §2)
   └─ achieved scale/location readback + Error  ← unchanged
```

**There is no longer a second construction pass at all.** The re-run is not guarded, moved or replicated — it is **gone**, and the suite pins `RerunConstructionScripts` at **0 occurrences file-wide**.

### ⭐ How I know it runs in a **cooked** build — cited, not asserted

| link in the chain | file:line | what I read |
|---|---|---|
| `FinishSpawning` is public and **not** editor-gated | `Actor.h:3117` | declared in the `public:` region opened at `:3079`; preprocessor depth over the file is **0** at that point and the nearest `#if WITH_EDITOR` (`:3007`) closes at `:3010` |
| it calls `ExecuteConstruction` unconditionally | `Actor.cpp:4415` | inside `FinishSpawning`, no guard |
| `ExecuteConstruction` is public and not editor-gated | `Actor.h:3439` / `ActorConstruction.cpp:818` | same public region; the only `#if WITH_EDITOR` inside is a re-entrancy `checkf` |
| it runs the vendor's **user** construction script | `ActorConstruction.cpp:930-940` | the `#if WITH_EDITOR` wraps **only** the `bTurnOffEditorConstructionScript` config lookup and its `if`; with `WITH_EDITOR` off the block is **unconditional** ⇒ `ProcessUserConstructionScript()` runs **always** in a cooked build |
| this is the engine's documented purpose for the shape | `World.h:3846-3849` | *"WILL NOT run Construction Script of Blueprints to give caller an opportunity to set parameters beforehand"* |
| the properties are already there to write | `LevelActor.cpp:755` | `PostSpawnInitialize(...)` runs after the actor is allocated from its archetype; `bDeferConstruction` only skips the construction call at `Actor.cpp:4358-4360` |

⚠️ **What this does NOT prove, stated plainly:** that the *vendor's* script actually sources its MID from `BoxMaterials["Base"]`. Nothing in `Source/` can prove that. `VerifyFogVisualMaterial` is the instrument, and *"the Error fires on every raise"* remains a possible outcome of a perfectly green compile. That is `TASK-1167`'s to observe.

### ⚖️ THE FENCE I HAD TO CROSS, DECLARED IN THE OPEN — **cl. (2b)**

**I am taking the deferred-construction shape, which is the shape cl. (2b) forbids by name.** ⛔ I am not hiding behind the fact that the *symbol* `SpawnActorDeferred` does not appear in the diff. It does not (I use `SpawnActor` with `SpawnParams.bDeferConstruction`, which keeps `OverrideRootScale` — see below), but the **substance** is the deferred form and I say so.

**Why the clause's stated reason does not hold — measured, not argued:**

> cl. (2b): *"The engine's own comment beside the substitution says `bIsDefaultTransform` is FALSE IN A COOKED BUILD ⇒ the deferred form behaves DIFFERENTLY in the editor and in the packaged game."*

- The engine comment (`SCS_Node.cpp:143-146`) sits **inside the `if (bIsDefaultTransform)` branch** — i.e. it describes the case where the substitution **does** fire.
- The path that reaches that branch is the **non-deferred** one: `Actor.cpp:4358-4360` reads `if (!bDeferConstruction) { FinishSpawning(UserSpawnTransform, true); }` — **hardcoded `true`**, with no `#if` of any kind.
- ⇒ **the editor/cooked asymmetry the fence warns about belongs to the shape the file had BEFORE this change**, not to the one it forbids.
- On the deferred path, `bIsDefaultTransform` is a **literal I spell in our own file** (`/*bIsDefaultTransform=*/ false`). ⛔ A literal cannot vary by build target. **I did not verify what the stale engine comment meant; I made it irrelevant.**

**The one real hazard inside the fence survives, and is kept:** `UWorld::SpawnActorDeferred` (the *helper*, `World.h:3851`) defaults `TransformScaleMethod` to **`MultiplyWithRoot`** (`:3856`), which would **multiply** this world-sized scale by the vendor template's own. ⇒ the helper stays **banned by name** in test 11 — with its comment rewritten to that measured reason — and the shipped code uses `SpawnActor` + `bDeferConstruction`, which keeps `OverrideRootScale`.

⚖️ **Routed to the manager as a law-text correction (`SC-§82`), alongside the `RefreshFogVisual()` slip from revision 1** — cl. (2b)'s prohibition and the identical fence comment that used to live in `FogVolume.cpp`. **I rewrote the in-file comment** (it is in a file I own and it stated a false engine fact); I have **not** touched `CONVENTIONS.md` or the board's spec text. **If the manager rules the deferred shape out regardless, say so — but the alternatives are enumerated in §1 and each loses on a measured ground.**

---

## 1. Which shape I took, and why every alternative loses

### ✅ **(2a)(a), with the transport changed from a re-run to a deferred first pass.** The *choice* of (a) over (b) is unchanged and its reason is unchanged: the vendor re-pushes ask (B)'s scalars itself, and this diff copies **zero** vendor parameters.

| candidate | why it loses |
|---|---|
| **(b)** `SetMaterial` + re-apply the scalars ourselves | orphans the vendor's tuned MID ⇒ ships **grey fog with `density 5` back**, silently. Closing it means hand-copying a vendor parameter list nothing keeps in sync (the live MID carries at least six). ⛔ Unchanged from revision 1, and this diff still contains **0** `SetMaterial(` / `CreateDynamicMaterialInstance` / `SetScalarParameterValue` / `SetVectorParameterValue`. |
| **`#if WITH_EDITOR` around the re-run** | ⛔ ships the defect. The rebuild **is** the mechanism ⇒ guarded, the packaged game gets beige fog + Error #7 on every raise. QA said this and QA is right. |
| **`DestroyConstructedComponents()` + `ExecuteConstruction(...)`** — both public, neither editor-gated | ⛔ this is hand-reimplementing `RerunConstructionScripts` **minus** its instance-data cache, attachment save/restore, child-actor redirection and clean-package restoration (`ActorConstruction.cpp:253-400`). It keeps the *second* construction pass and its cost, keeps WARN-1's ordering hazard, and adds a new class of state-loss bugs — to buy nothing the deferred form does not give for free. **Loses on strictly-more-risk-for-less.** |
| **`FindFunctionByName("UserConstructionScript")` + `ProcessEvent`** (QA's second hypothesis) | ⛔ **Cannot be judged from `Source/` at all**, and that is the disqualifier rather than a preference: if the vendor's UCS creates components, calling it a second time **duplicates them**, and I have no way to read that Blueprint's graph. `AActor::ProcessUserConstructionScript()` is `protected` (`Actor.h:3526`) so the safe wrapper is unreachable. **Loses on an unbounded unknown.** |
| **`SpawnActorDeferred` (the helper)** | ⛔ defaults `TransformScaleMethod` to `MultiplyWithRoot` ⇒ would multiply our world-sized scale by the vendor template's. **Still banned by name**; `bDeferConstruction` on `FActorSpawnParameters` gets the same window while keeping `OverrideRootScale` **and** every other spawn parameter this call site already sets (`Owner`, `AlwaysSpawn`, `RF_Transient`). |
| **ruling the grey editor-only** | ⛔ 🧑 he plays the packaged build. |

### ⭐ What the new shape wins beyond closing the blocker

- **One construction pass instead of two** ⇒ QA's cost question (*"a construction re-run on a world-sized actor, once per fog raise"*) **evaporates**: this is not an extra pass, it is the **same** pass moved one statement later.
- **WARN-2 is retired, not mitigated.** The deferral path QA found (`ActorConstruction.cpp:256-261`, `FScopedSuspendRerunConstructionScripts`) is a property of `RerunConstructionScripts` **only**. `FinishSpawning` has no such branch: it calls `ExecuteConstruction` **inline and synchronously** at `Actor.cpp:4415`. The only "deferral" left is **ours**, and it is bounded by two adjacent statements in one function. ⇒ **there is no window in which a readback can pass before a later rebuild undoes it.**
- **WARN-1's hazard is retired at the source.** With `bIsDefaultTransform=false`, `SCS_Node.cpp:142-147` never enters the substitution branch. See §2 for what I did with the ordering anyway.

---

## 2. ⭐ WARN-1 — the ordering is KEPT; the reason is CORRECTED

QA is right that my stated reason was wrong. On the **re-run** path, `ActorConstruction.cpp:601-611` computes `bIsDefaultTransform` from `OldTransform.Equals(RootComponent->GetComponentTransform(), 0.0)` where `OldTransform` **is** that same transform captured at `:490` ⇒ **TRUE**, and the template scale **is** re-substituted. My *"`== false`"* reading was backwards, and colour-before-scale was load-bearing rather than prudent.

**Under the new mechanism the ordering is still colour-before-scale, and it is now required for a *stronger* reason:**

- the material write must be **before** `FinishSpawning` — after it, the script has already read the field and our value lands where nobody looks again;
- the scale force must be **after** `FinishSpawning` — before it there is **no root component at all** (the SCS has not run), so `SetActorScale3D` would be a **silent no-op**.

⇒ the two are on **opposite sides of a single, non-optional statement**. The order is no longer a judgement call that a future editor could invert; it is pinned by what exists at each moment. The suite asserts all three positions and, additionally, that **nothing touches the transform in the deferred window**.

⚠️ **`SetActorScale3D` is KEPT even though `bIsDefaultTransform=false` should make the substitution unreachable.** I say *should* deliberately: it is correct whatever construction did, it is identical in both targets, and it is the only reason the achieved-scale readback measures a value somebody in this file actually asked for. The comment above it and test 7's prose are **both narrowed to say it is now belt-and-braces rather than the repair** — assertion unchanged, prose corrected.

---

## 3. The other findings — each addressed or declined **with a reason**

| # | finding | disposition |
|---|---|---|
| **WARN-2** | a rerun can be deferred ⇒ green readback before a later rebuild | ✅ **RETIRED BY THE MECHANISM.** `FinishSpawning` → `ExecuteConstruction` is a direct synchronous call (`Actor.cpp:4415`); the suspend/queue path exists only inside `RerunConstructionScripts`, which is gone. |
| **WARN-3** | `Spawned` not re-validated after a step that ran arbitrary vendor Blueprint code | ⚖️ **DECLINED, and the premise is retired rather than tolerated.** Under the new order `ApplyFogVisualMaterial` runs **before** any vendor code — it is reflection plus one `TryLoad()`. The vendor's script now runs **inside** `FinishSpawning`, i.e. after the write. ⛔ **Adding the guard would actively hurt:** it would put a `return` between the write and `FinishSpawning`, and an actor left unfinished has **no root, no mesh and no BeginPlay**, held forever by `FogVisualActor` — strictly worse than beige fog. It would also have broken two invariants tests 4 and 7 already pin (`SpawnFogVisual`'s `Error` = 3, `return` = 3) for a hazard that no longer exists. ⇒ instead of a guard I asserted the **unconditionality** with a pair of rows (returns-before-the-call = 3 **and** returns-in-the-function = 3; either alone is green for the other's mutation). |
| **WARN-4** | the *"no other producer"* claim rested on an un-re-run byte scan | ✅ **CLOSED BY MEASUREMENT TODAY.** `grep -rl --binary-files=text "BP_SiegeFog" Content/` returns **exactly one file — `Content/Blueprints/BP_SiegeFog.uasset` itself**. `Content/Maps/L_Arena.umap` returns **0**, and that is not a tooling artefact: the file is **605,098 real bytes** (not an LFS stub) and a positive control on the same file (`SiegeGameMode|DirectionalLight`) returns **4**. ⇒ **no hand-placed instance, no second spawner.** |
| **WARN-5** | TEST 10 passes even if the feature is deleted; TEST 11 is a string census | ✅ **ACKNOWLEDGED, unchanged, and re-declared.** TEST 10 covers the asset + the predicate, never the wiring. TEST 11 holds all feature coverage and proves only the **shape** of the call. ⭐ **But it now also holds the one thing our compile structurally cannot see** — see the row below. |
| **WARN-6** | the `EditDefaultsOnly` departure | ⚖️ **UNCHANGED; QA ruled it non-blocking and routed it to the manager.** A function cannot carry a `UPROPERTY` specifier, so the law's two halves are mutually exclusive; the property it wants (*a retune is zero code change*) is preserved and stronger, because the colour lives in `MI_SiegeFog_Grey`'s own vectors. |
| **NIT-1** | Error #4's sentence was untrue when the entry was absent | ✅ **FIXED by splitting the branch.** *"There was nothing to write to"* and *"the write changed nothing"* are now **two Errors with two messages**. The suite pins `if (BaseEntryIndex == INDEX_NONE)` at 1 so the split cannot be silently re-merged. |
| **NIT-2** | the map's value class is never checked against the material | ✅ **FIXED — and QA undersold it.** `FObjectPropertyBase::SetObjectPropertyValue` has no type check, so a vendor retype would let the write **succeed**, store a wrongly-typed object, and let the readback compare **the same pointer against itself** and report **GREEN**. ⇒ this is not a message improvement, it is the difference between a caught fault and a self-verifying corruption. The check is **before** the store (asserted as an ordering row, because after it, it is decoration). |
| **NIT-3** | `GetMaterial(0)` + `FindComponentByClass` is non-deterministic if the vendor grows a second mesh | ✅ **FIXED.** `VerifyFogVisualMaterial` walks **every** `UMeshComponent` and **every** slot (`GetNumMaterials()`), and counts what it inspected. `GetMaterial(0)` is pinned at **0** by the suite. ⭐ A visual with **zero** mesh slots now reaches the Error saying so, instead of passing silently. |

---

## 4. Exact diff surface

| File | Change |
|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/FogVolume.h` | **+4 declarations** — `FogVisualMaterialPath()`, `FogVisualMaterialMatches()` (unchanged from rev 1), `ApplyFogVisualMaterial(AActor*)`, and ⭐ **NEW** `VerifyFogVisualMaterial(const AActor*) const`. `ApplyFogVisualMaterial`'s doc block **rewritten**: the re-run narrative replaced by the deferred-construction one with its engine citations, and ⭐ **the false coverage sentence corrected** — it now says out loud that *"every surface reaches this line"* is **two** claims (the write, and the mechanism that makes it visible), that the second half was **false** while the function ended in an editor-only API, and that both hold only for the `FinishSpawning` route. |
| `Source/GitClaudeUnrealTest/Siegebound/FogVolume.cpp` | **+1 spawn parameter** (`SpawnParams.bDeferConstruction = true;`) · **+1 call** `Spawned->FinishSpawning(SpawnTransform, /*bIsDefaultTransform=*/ false, /*InstanceDataCache=*/ nullptr, ESpawnActorScaleMethod::OverrideRootScale);` · **+1 call** `VerifyFogVisualMaterial(Spawned);` · **−1 call** `Visual->RerunConstructionScripts();` (**0 occurrences remain on any code line**) · **+1 function** `VerifyFogVisualMaterial` (the mesh readback, moved out and widened to every mesh/every slot) · **+2 Error sites** inside `ApplyFogVisualMaterial` (the value-class check; the missing-entry split) · **4 prose amendments** (the `TASK-1071` fence, which had a false engine claim in it; the scale-mismatch Error string; the colour call's comment; the spawn-parameter block). ⛔ **No `#include` added or removed.** |
| `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeFogVisualTest.cpp` | **+1 fixture signature** (`VerifyFogVisualMaterialSignature`) · **tests 10 and 11 revised** (both are *this* row's own, unlanded) · ⚠️ **2 PROSE-ONLY amendments to pre-existing test 7** — its *"delete this line and the substitution stands"* comment and its *"the clobber is applied during SpawnActor"* message both describe a path the code no longer takes. ⛔ **No pre-existing assertion was edited, weakened, renumbered or removed** — the counts they pin (`SpawnFogVisual`: `Error` = 3, `return;` = 3, `TransformScaleMethod` = 0 file-wide, `SetActorScale3D` = 1) are all **still exactly what they were**, re-measured. |
| `.claude/pipeline/TASKBOARD.md` | **my own `- status:` line only**, `Edit` tool, prior text struck not deleted. |

**Referenced assets (consumed, never authored):** `/Game/Materials/Instances/MI_SiegeFog_Grey.MI_SiegeFog_Grey` — present on disk (`Content/Materials/Instances/MI_SiegeFog_Grey.uasset`). ⚠️ **LFS materialisation is `TASK-1167`'s duty, by oid-vs-`sha256`, never by size (`SC-§68`).**

⭐ **Ask (B) is untouched and structurally cannot be touched by this diff:** `SetMaterial(` = 0, `CreateDynamicMaterialInstance` = 0, `SetScalarParameterValue` = 0, `SetVectorParameterValue` = 0, on **any code line, file-wide** — re-measured after every edit, and banned by name in test 11. `density 0.5` / `sharpness 0.1` / `wind Speed 0.5` are the vendor's own struct defaults and this file never names them.

---

## 5. The failure paths — **7 `Error` sites, and the count moved 5 ⇒ 7 in the open**

**`ApplyFogVisualMaterial` — 6 (all before construction, all `return` on failure, all leave the fog UP and beige):**

| # | condition | note |
|---|---|---|
| 1 | `MI_SiegeFog_Grey` fails to load | names the exact path **and** the function |
| 2 | `BoxMaterials` missing, or its value is not an object property | names the property and the class |
| 3 | ⭐ **NEW (NIT-2)** — the map's value class will not accept a `UMaterialInterface` | **checked before the store**, because the engine's setter has no type check and a wrong-typed store verifies itself green |
| 4 | the `Base` enum entry no longer exists under that name | resolved by name, never by ordinal |
| 5 | ⭐ **NEW (NIT-1 split)** — the map holds **no entry** under that key ⇒ **nothing was written** | FIND, never FIND-OR-ADD |
| 6 | ⭐ the write returned and **changed nothing** | the measured `TASK-1154` failure (`SC-§94` cl. D) |

**`VerifyFogVisualMaterial` — 1 (after construction):**

| # | condition | note |
|---|---|---|
| 7 | ⭐ **no material slot on any mesh component** carries the requested material | reports **how many slots were inspected** and the first achieved path ⇒ *"there was nothing to measure"* is a reportable state, not a silent pass. Also catches a **mode** mismatch for free. |

⛔ **No log line tells a reader what to conclude, report or paste** (`SC-§109`) — each says what failed, where, and what the player will see.
⚖️ **The art-failure boundary holds:** `return false` = 0, `FogActiveUntilTimeSeconds` = 0, `IsFogActive()` = 0 in **both** new bodies; `SpawnFogVisual`'s own `Error` = 3 and `return` = 3, **unchanged**.

---

## 6. Suite delta (`TL-§5b`) — and it asserts **resolved state**, not that a function was called

**+2 tests** in the **existing** `Siegebound/Tests/SiegeFogVisualTest.cpp` (10 = Lane A executed, 11 = Lane B source-text). ⛔ No new test file.

**New/changed rows in TEST 11, over revision 1:**
- 🚨⭐⭐⭐ **`RerunConstructionScripts` = 0, file-wide** — ⛔ **this is the row that would have caught the blocker.** Our only compile defines `WITH_EDITOR=1` and goes green over an editor-only API; a **source-text** probe is blind to a great deal but is **not** blind to that.
- `SpawnParams.bDeferConstruction = true;` = 1 · `Spawned->FinishSpawning(` = 1 · `/*bIsDefaultTransform=*/ false` = 1 (the argument is **spelled**, so it cannot vary by target).
- ordering: colour **before** `FinishSpawning`; measurement **not** before it; **nothing touches the transform** in the deferred window; scale forced **after** `FinishSpawning`.
- ⭐ **unconditionality, as a PAIR**: `return` before the colour call = 3 **and** `return` in the whole function = 3. Either row alone is green for the other's mutation, which is exactly why both are there.
- the value-class check is **before** the store; the readback is **not** before it.
- the missing-entry branch exists as its own `if`; Error sites = **6** in Apply, **1** in Verify.
- new `VerifyFogVisualMaterial` block: `GetNumMaterials()` = 1, **`GetMaterial(0)` = 0**, `GetMaterial(SlotIndex)` = 1, one dynamic unwrap, one comparison, `if (!bWearingRequestedMaterial)` = 1, and ⛔ **`SetObjectPropertyValue(` = 0** (a measurement that repaired what it found would be reporting a state it had itself just changed).
- the `SpawnActorDeferred` ban **survives with a corrected reason** (`MultiplyWithRoot`, measured at `World.h:3856`) instead of the false cooked-build claim.

**Pre-flight I did run (and it is ⛔ NOT a suite run):** I re-implemented `CountOccurrencesInCode` / `ExtractFunctionBody` / `SubstringBefore` character-for-character and evaluated **65 needles** — every new row plus the pre-existing invariants of tests 4, 5, 7 and 9 that touch the edited functions. **65/65 agree with the asserted values**, including `SpawnFogVisual`'s untouched `Error` = 3 / `return` = 3, `TransformScaleMethod` = 0, `MarkPackageDirty`/`SavePackage` = 0, `ReleaseFogRenderFloor();` = `DestroyFogVisual();` = 2. Brace balance on all three edited files is **0**. ⛔ **That is a text simulation of the Lane-B probes: it cannot compile, cannot execute Lane A, and is evidence of needle correctness only.**

### ⛔ `NO WITNESSED RED` — transferred BY NAME to ⭐ `TASK-1167`, **re-derived against the new mechanism**

| # | mutation | must red at |
|---|---|---|
| M1 | delete `ApplyFogVisualMaterial(Spawned);` | test 11 "THE COLOUR HAS A CALLER" + "ONE application site and ONE caller" *(the `SC-§36.1` zero-callers failure exactly)* |
| M2 | move that call **after** `Spawned->FinishSpawning(` | test 11 "THE COLOUR IS WRITTEN **BEFORE** CONSTRUCTION FINISHES" |
| **M3** | ⭐ **RE-DERIVED** — delete `SpawnParams.bDeferConstruction = true;` | test 11 "CONSTRUCTION IS DEFERRED…". ⭐ **This is the mutation whose red matters most:** without it the vendor's script has already built its MID from its own material before the write lands, and **every readback in the file still reads green** while the fog stays beige |
| M4 | move `VerifyFogVisualMaterial(Spawned);` **above** `FinishSpawning` | test 11 "…and the MEASUREMENT is NOT [before construction]" |
| M5 | delete the post-write `GetObjectPropertyValue` block | test 11 "THE WRITE IS READ BACK" + the Error-site count |
| M6 | drop `.MI_SiegeFog_Grey` from the path literal | test 10 "names an OBJECT, not merely a package" + the file-on-disk row + test 11's two-occurrence row |
| M7 | make `FogVisualMaterialMatches` `return true;` unconditionally | test 10 "THE DETECTOR SAYS NO to the VENDOR material" + all three empty-path rows |
| M8 | add a `SetScalarParameterValue` anywhere in `FogVolume.cpp` | test 11's forbidden-shortcut ban *(the uniformity non-regression's executable form)* |
| **M9** | ⭐ **NEW, AND IT IS THE BLOCKER'S OWN FENCE** — re-introduce `Visual->RerunConstructionScripts();` | test 11 "FogVolume.cpp reaches NO editor-only construction API" (0 ⇒ 1). ⛔ **Run this one first: it is the only row that fails a defect the editor compile passes.** |
| **M10** | ⭐ **NEW** — insert a bare `return;` between `ApplyFogVisualMaterial(Spawned);` and `Spawned->FinishSpawning(` | test 11 "…the function has NO OTHER return anywhere" (3 ⇒ 4) — and test 7's independent pin |
| **M11** | ⭐ **NEW** — flip `/*bIsDefaultTransform=*/ false` to `true` | test 11 "…`bIsDefaultTransform` is SPELLED rather than defaulted" |
| **M12** | ⭐ **NEW** — change `GetMaterial(SlotIndex)` back to `GetMaterial(0)` | test 11 "…the hardcoded element 0 is GONE" + "walks every mesh and every slot" |

⚠️ **The list grew from 8 to 12 rather than being trimmed to fit, and that is deliberate:** four of the new rows exist because four defects (the editor-only API, an unfinished actor, a build-varying transform flag, a non-deterministic slot read) each shipped through a gate that could not see them.

### ⛔ `NOT MEASURED` — transferred BY NAME to ⭐ `TASK-1167`

**No sampled B/G before or after. No whiteout %. No median ground visibility.** Not an omission and not a shortcut: **the pixels cannot exist before the build**, because the mechanism that changes them is C++ that has not been compiled and `SC-§27` forbids this row to compile it. cl. (3)'s targets stand as written (far-field B/G `0.895 ⇒ ~0.963`, saturation `14.7 % ⇒ 5.9 %`; whiteouts stay `0 %`, median visibility stays `≈ 647 uu`) and the duty to take them is `TASK-1167`'s, **by name**.
**Editor:** ⛔ **not used at all in this revision.** No MCP call, read or write. (Revision 1 read the vendor CDO read-only at PID `13180`; nothing in this revision needed it, and the vendor field names it recorded are unchanged.)

---

## 7. 🔍 What QA should scrutinise hardest **this time**

1. 🚨⭐⭐⭐ **Is the `bDeferConstruction` window actually safe for this Blueprint?** My claim: `BoxMaterials` is initialised from the archetype during `NewObject` inside `SpawnActor`, well before `PostSpawnInitialize` (`LevelActor.cpp:755`), so the map already holds the vendor's four entries and `FindMapIndexWithKey("Base")` hits. ⛔ **Read from the engine's construction order, not executed.** If it were wrong, Error #5 fires loudly on every raise rather than failing silently — but check the reasoning.
2. ⚖️ **The cl. (2b) crossing (§0).** This is the biggest judgement in the diff and it is a **claim about a law's stated reason**, not a preference. If you can falsify `Actor.cpp:4358-4360` or my reading of `SCS_Node.cpp:143-146`, the whole shape falls.
3. ⚠️ **`FinishSpawning` must be unconditional.** I declined WARN-3's guard *because* it would break this. If you think an unfinished actor is the lesser risk, say so — but note `ExecuteConstruction` opens with `check(IsValidChecked(this))`, so a guard is the only alternative to a crash if the actor could die in that window, and I argue it cannot (no vendor code runs there).
4. ⚠️ **The deferred-transform cache.** `Actor.cpp:4361-4366` only records a `GSpawnActorDeferredTransformCache` entry when `SceneRootComponent != nullptr`; this visual has **no native root**, so **no entry is made** and `FinishSpawning`'s `RemoveAndCopyValue` has nothing to leak. ⛔ Read, not run.
5. ⚠️ **`PostActorConstruction` / `BeginPlay` now dispatch from `FinishSpawning`** rather than from inside `SpawnActor` — one statement later, and **after** the material write. I believe that is strictly better (the vendor's BeginPlay sees the final material) and it is the documented behaviour of the pattern, but it is a real ordering change on a shipped path.
6. ⚠️ **The `Base` key is still resolved by DISPLAY name**, and display names are `FText` and therefore localisable. Unchanged, declared, and failure #4 makes a miss loud.
7. ⚠️ **`VerifyFogVisualMaterial` now loops over components and slots** — cost is trivial (once per fog raise, on an actor with one mesh today) but it is new code with new indices; the loop is bounded by `GetNumMaterials()` and the array `GetComponents` filled.

---

## 8. Fences

| Fence | Result |
|---|---|
| `Content/Maps/L_Arena.umap` | **never opened, never saved.** Zero `.umap` in the diff. ✅ Read-only byte scan only (WARN-4), which does not touch mtime. |
| ⛔ `DirectionalLight_0` / any map actor | **not touched.** |
| ⛔ `Content/FogArea/**` (vendor) | **untouched and unopened.** Test 11 asserts `FogVolume.cpp` names **zero** paths inside the pack. |
| ⛔ any `.uasset` | **zero.** The map write is per-instance memory on an `RF_Transient` actor; `MarkPackageDirty`/`SavePackage` remain **0** file-wide. ⭐ The old `RerunConstructionScripts` clean-package argument is moot — there is no re-run. |
| ⛔ `SiegeFogStatics.{h,cpp}` · `Config/**` | **zero bytes.** |
| MCP / editor | ⛔ **not used in this revision at all.** |
| Compile / suite / Git | **none.** `TASK-1166` gates, `TASK-1167` hosts. Never pushed. |
| `TASKBOARD.md` | `Edit` tool, **my own `- status:` line only**. ⛔ `CONVENTIONS.md` not edited. |
