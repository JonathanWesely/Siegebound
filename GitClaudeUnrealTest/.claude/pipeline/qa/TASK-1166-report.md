# QA Report — TASK-1166 (gate over TASK-1165, ask (C): the grey fog material)

**Verdict: FAIL** — **1 BLOCKER**, 6 WARN, 3 NIT.
Date 2026-09-08 · subject `TASK-1165` · host `TASK-1167` · law `FOG-§12.4a` · `GFX-§11` · `SC-§94` cl. D · `SC-§101` · `SC-§104` · `SC-§109` · `SC-§71b` · `TL-§5b`/`TL-§5e`.

> ⚠️ **PATH NOTE, flagged rather than silently resolved:** the board's `names:` field for this row says *"WRITES: `.claude/pipeline/qa/TASK-1165.md` ONLY"* and `TASK-1167`'s `STAGES:` list names that same path. My dispatch named **`.claude/pipeline/qa/TASK-1166-report.md`**, and that is the file you are reading — the **only** verdict file for this gate. ⛔ I did not write a second copy: two verdict files is two sources of truth. **`TASK-1167` must stage THIS path**, or its pathspec answers with silence (`SC-§102`).

## What I hold and what I do not (`SC-§71b`)
⛔ **No `Bash`, no MCP, no engine, no Git.** Every **pixel**, **compile**, **suite-run** and **git/fence** claim below that originates in the handoff is **ACCEPTED-AS-DECLARED** and is labelled where it matters.
✅ What I **did** verify first-hand, by reading: the four diff-surface files; the **installed UE 5.8 engine source** (`C:/Program Files/Epic Games/UE_5.8/Engine/Source/...`) for every reflection and actor API this diff uses; the caller graph inside `Source/`; `CONVENTIONS.md`'s `FOG-§12.4a`; the presence of `Content/Materials/Instances/MI_SiegeFog_Grey.uasset` on disk (existence only — **not** its LFS materialisation, which is `TASK-1167`'s oid-vs-sha256 duty, `SC-§68`).

---

## Findings

### 🚨 BLOCKER-1 — `Source/GitClaudeUnrealTest/Siegebound/FogVolume.cpp:982` — `RerunConstructionScripts()` is an **EDITOR-ONLY** API. This diff does not compile in a cooked build, and the mechanism it depends on does not exist there.

```cpp
	Visual->RerunConstructionScripts();     // FogVolume.cpp:982 — unguarded, in a Runtime module
```

**Measured against the installed 5.8 engine, not remembered:**

| evidence | file:line | what it says |
|---|---|---|
| declaration | `Engine/Classes/GameFramework/Actor.h:3415-3418` | `#if WITH_EDITOR` … `ENGINE_API virtual void RerunConstructionScripts();` … `#endif` — **one** declaration, editor-only |
| definition | `Engine/Private/ActorConstruction.cpp:253-254` | `#if WITH_EDITOR` immediately above `void AActor::RerunConstructionScripts()` |
| module type | `GitClaudeUnrealTest.uproject:9` | `"Type": "Runtime"` — this module is compiled into the cooked game |
| guard census | `FogVolume.cpp` | **zero** occurrences of `WITH_EDITOR` / `#if` / `#endif` anywhere in the file (my grep) |

⇒ **Two consequences, one root cause:**

1. **The cooked target will not compile.** `Build.bat GitClaudeUnrealTestEditor` (the only command `TASK-1167` runs) defines `WITH_EDITOR=1` and will go **green** — this defect is **invisible to the gate downstream of me**. The `GitClaudeUnrealTest` (non-editor) target fails at `FogVolume.cpp:982` with *"'RerunConstructionScripts': is not a member of 'AActor'"*. Packaging is a **live lane in this project** — `PKG-§`/`SHIP-§`/`BuildCookRun` appear **95 times** in `CONVENTIONS.md`, and the packaged zip is an outstanding deliverable — so this is a real break, not a hypothetical one.
2. **A `#if WITH_EDITOR` guard is NOT the fix.** The whole design routes the colour through *the vendor's own construction re-run* so the vendor re-pushes ask (B)'s scalars. Guarded, that re-run **cannot happen in the packaged game**: the map entry would be written and read back green, and Error #5 would then fire on **every** fog raise in the shipped product while the box renders the vendor's beige. That is **exactly** the failure class the row's own cl. **(2b)** forbids by name — *"a defect could return ONLY IN THE SHIPPED PRODUCT, where nobody is watching"* — arriving through a different door than the one the fence was built across.
3. **The header's coverage claim is false in its load-bearing half.** `FogVolume.h:1246-1249` states: *"the visual has EXACTLY ONE spawn site … so PIE, Simulate, standalone **and the packaged game** all arrive here on the same line."* The **write** does; the **rebuild that makes the write visible** does not. The same sentence is in the handoff §2. This must be corrected in the same diff as the mechanism, whichever mechanism lands.

**Candidate directions — offered as HYPOTHESES, not a prescription (`SC-§101`: a prescribed remedy is a claim, and I cannot compile):**
- `AActor::ExecuteConstruction(...)` is **public and NOT editor-gated** (`Actor.h:3439`, inside the `public:` region that runs `3122`→`3470`) — it is what `UWorld::SpawnActor` itself calls at runtime. ⚠️ Unverified by me: whether it needs `DestroyConstructedComponents()` first to avoid duplicating SCS components, and what it does to the actor mid-frame.
- Re-running **only** the vendor's user construction script by name (`GetClass()->FindFunctionByName(TEXT("UserConstructionScript"))` + `ProcessEvent`) — this would not destroy/rebuild the SCS components at all, which would also retire the whole `TASK-1071` ordering hazard. ⚠️ Unverified: whether the vendor's script creates components (it would then duplicate them). ⛔ `AActor::ProcessUserConstructionScript()` is **protected** (`Actor.h:3526`→`3534`) and is therefore *not* reachable from `AFogVolume`.
- Or an explicit ruling that the grey is editor-only — ⛔ which I would expect the manager to refuse, because 🧑 he plays the packaged build.

⚠️ Whatever lands, **`SiegeFogVisualTest.cpp:1643` pins `RerunConstructionScripts()` at exactly 1**, and mutation **M3** in the handoff is named against it. The suite delta and the mutation list move **with** the fix.

---

### WARN-1 — `FogVolume.cpp:745-754` (and handoff §6.1) — the stated engine mechanism is **WRONG**, in the direction that would invite a future editor to invert the load-bearing ordering. The *conclusion* is right; the *reason* is not.

The comment says the re-run gets *"the actor's CURRENT root transform **rather than a default one**"*, and the handoff says *"…passes it to `ExecuteConstruction` with `bIsDefaultTransform == false`, so the `SCS_Node.cpp` substitution does **not** fire."*

**Measured:** `ActorConstruction.cpp:601-611` computes `bIsDefaultTransform = OldTransform.Equals(RootComponent->GetComponentTransform(), 0.0)`. On this path `OldTransform` **is** `RootComponent->GetComponentTransform()`, captured at `:490`, and nothing moves the actor in between ⇒ the two are bit-equal ⇒ **`bIsDefaultTransform` is `TRUE`** and `ExecuteConstruction` is called with it at `:614`. `SCS_Node.cpp:142-148` then does exactly what `TASK-1071` measured: `WorldTransform.SetScale3D(NewSceneComp->GetRelativeScale3D())` — **the vendor template's scale is re-substituted on the re-run.**

⭐ **So the author's ordering decision is not merely prudent — it is REQUIRED, and it is the single thing standing between this diff and a re-run of `TASK-1071`.** Colour-before-scale is **correct**; the reverse order would have shipped invisible fog with a green suite. ✅ And the location claim survives too, by a different mechanism than the one written down: `SCS_Node.cpp:147` substitutes **`Scale3D` only** — location and rotation come from the passed `RootTransform`. ⛔ Fix the sentence, keep the order.

### WARN-2 — `FogVolume.cpp:982` → `755-793` — the re-run can be **DEFERRED**, and in that case the scale readback passes green and the box is resized afterwards with nothing in any log.
`ActorConstruction.cpp:256-261`: if `FScopedSuspendRerunConstructionScripts::IsSuspended()`, the call **queues the actor and returns immediately**. The sequence then becomes: write ✓ → readback ✓ → *(no rebuild)* → mesh read reports the vendor material (Error #5, loud — fine) → `SetActorScale3D` → scale readback **passes** → and the deferred rebuild lands **later**, re-substituting the template scale (WARN-1) with **no further correction and no further readback**. That is `TASK-1071` returning **silently**. Low likelihood inside PIE (nothing in this project opens such a scope), but it is the *"rebuilds asynchronously"* case, it is real, and it must be re-judged against whatever replaces BLOCKER-1's call.

### WARN-3 — `FogVolume.cpp:720` → `755`, `768-769`, `799` — `Spawned` is not re-validated after the material step, which executes **arbitrary vendor Blueprint** code.
`ApplyFogVisualMaterial(Spawned)` runs a construction script; the caller then dereferences `Spawned` three more times and stores the handle without an `IsValid` re-check. The engine's own re-entrancy guards make actor destruction from a CS unlikely, so this is not a blocker — but the cheapest possible close (`if (!IsValid(Spawned)) { return; }` after the call) removes a dangling-pointer class from a line that now calls into content nobody here reviews.

### WARN-4 — the *"exactly one spawn site"* claim is **verified in `Source/` and ACCEPTED-AS-DECLARED for `Content/`**.
✅ My own census, first-hand: `SpawnFogVisual()` has exactly one caller (`FogVolume.cpp:626`, inside the one reconciler); `FogVisualClassPath()` is consulted **only** inside `SpawnFogVisual` (`:657`, `:665`, `:706`); `ApplyFogVisualMaterial(` appears once as a call (`:720`) and once as a definition (`:818`); `SpawnActor<AActor>(` appears once (`:700`). Test 9 already pins `BP_SiegeFog` at zero occurrences in `SiegeGameMode.cpp` and `SpellLibrary.cpp`.
⛔ **What I cannot see:** my `Grep` does not read `.uasset`/`.umap` bytes (a search of `Content/` for `BP_SiegeFog` returned *no files*, which is a **tooling artefact, not evidence** — the asset demonstrably exists). So *"no hand-placed instance, and no other asset spawns the visual"* rests on the header's byte-scan **dated 2026-09-05 and not re-run**. Per this project's own scar — *a name census cannot catch a site that recomputes a value* — a second producer would collapse the coverage claim, and neither the author nor I have re-measured that today.

### WARN-5 — the two new tests are correct probes, but **neither fails on behaviour**, and one does not fail at all if the feature is deleted.
- **TEST 10** (`SiegeFogVisualTest.cpp:1450-1562`, Lane A, genuinely executed) exercises `FogVisualMaterialPath()` and `FogVisualMaterialMatches()`: negative control **before** the positive probe (`:1507-1517`), the detector fed the **measured vendor material** and required to answer NO (`:1538`), YES to its own (`:1541`), all three empty-path forms mismatched (`:1547-1552`), `_C`-suffix rejected (`:1559`). ⭐ This is the right shape and it is the only genuinely executable row in the delta. ⛔ **But delete `ApplyFogVisualMaterial` entirely and TEST 10 still passes** — it covers the asset and the predicate, never the wiring. Declared by the author; recorded here so nobody counts it as feature coverage.
- **TEST 11** (`:1569-1777`, Lane B, source-text) is where all feature coverage lives. I checked its needles against the edited source by hand: the five `Error` sites really are **5** (`FogVolume.cpp:841/862/908/963/1007`), `SetObjectPropertyValue(`/`GetObjectPropertyValue(` really are 1 each and both really precede `:982`, `GetMaterial(`/`FogVisualMaterialMatches(` really are 0 before it, `FindOrAdd` really is 0 on code lines, and the five forbidden shortcuts (`SetMaterial(`, `CreateDynamicMaterialInstance`, `SetScalarParameterValue`, `SetVectorParameterValue`, `SpawnActorDeferred`) really are **0 on code lines** file-wide — the only two textual hits (`:738`, `:941`) are comment lines, which `CountOccurrencesInCode` (`:211-221`) skips. ✅ M1-M8 would each redden a named row. ⛔ **It is still a string census**: it proves the *shape* of the call, never that the vendor rebuilt anything or that a pixel changed.
- ⚠️ Per the author's own words, **56/56 needles were evaluated in a throwaway re-implementation and the suite was NOT RUN**. ACCEPTED-AS-DECLARED; `TASK-1167` owes the executed `N/M`.

### WARN-6 — ⚖️ `FOG-§12.4a`'s **`EditDefaultsOnly`** pin was **not** taken, and the departure is declared (`FogVolume.h:629-638`). I rule it **non-blocking** and route the ruling to the manager.
The law asks for `EditDefaultsOnly` *and* *"the existing `FogVisualClassPath()` house pattern"* in one breath; a **function cannot carry a `UPROPERTY` specifier**, so the two are mutually exclusive and the prescription is self-falsifying (`SC-§101`). The property the law actually wants — *a retune is zero code change* — **is preserved and is stronger**: the colour lives in `MI_SiegeFog_Grey`'s own vectors, so an artist retunes it by editing that asset and this file does not change. The row's check **(f)** is therefore **satisfied in substance**, and its load-failure half is satisfied literally: `FogVolume.cpp:839-849` logs `Error` naming the exact path **and** the function, then `return`s — the function is `void`, so no refusal can propagate.

### NIT-1 — `FogVolume.cpp:961-971` — Error #4's sentence is untrue in one of the two cases it catches.
If `BaseEntryIndex == INDEX_NONE` at `:944` the write is **skipped**, and the message then reads *"still holds 'None' **after being set to** 'MI_SiegeFog_Grey'"* — describing an attempt that never happened. ✅ The branch is **reachable and does fire** (that is the case that keeps a missing map entry from becoming a silent success, and it is why `FindOrAdd` is banned), so the safety property holds; only the wording conflates *"the write did nothing"* with *"there was nothing to write to"*. ✅ `SC-§109` clean: none of the five lines tells a reader what to conclude, report or paste.

### NIT-2 — `FogVolume.cpp:838` / `:946` — the map's value class is never checked against the material being written.
Error #2 (`:860`) proves the value property is an object property; nothing proves it **accepts a `UMaterialInterface`**. ✅ Not a crash risk: `FObjectPropertyBase::SetObjectPropertyValue` in 5.8 (`PropertyBaseObject.cpp:682-692`) contains **no type `check`** — it only guards nullability. So a vendor retype would produce a wrong-typed store caught one line later by the readback, not an assert. Recorded because the code's comment implies a stronger validation than the engine performs.

### NIT-3 — `FogVolume.cpp:992-993` — `GetMaterial(0)` assumes element 0, declared by the author but unguarded, and the mesh is found by class rather than by name (`FindComponentByClass<UMeshComponent>`), so a vendor that ever grows a second mesh component makes the readback non-deterministic. Loud-not-silent either way (Error #5), and cheap to narrow later.

---

## The `RerunConstructionScripts()` judgement (the question this gate was told to interrogate)

| the author's question | answer, measured |
|---|---|
| Can the re-run **reset the property just written** (CDO/template reasserted over an instance write)? | **No.** `RerunConstructionScripts` destroys and rebuilds **components** and restores **component** instance data (`ActorConstruction.cpp:329-390`); it never re-serialises the actor's own `UPROPERTY`s, and `FPrimitiveComponentInstanceData` (`PrimitiveComponent.h:3347-3377`) caches transform / visibility id / LOD parent and **no material state**. The `BoxMaterials` write is an actor-instance property and survives. |
| Can it **drop ask (B)'s parameters** instead of re-pushing them? | **No, by the same fact.** `Density` / `Base Noise Sharpness` / `Wind Speed` live in the visual's own struct properties (class defaults copied to the instance at spawn); the re-run does not touch them, and the vendor's script pushes them again from the same struct into the new MID. ⭐ Additionally this diff is **structurally incapable** of regressing ask (B): zero `SetMaterial(`, zero `CreateDynamicMaterialInstance`, zero `SetScalarParameterValue`/`SetVectorParameterValue` on any code line, and test 11 bans all four file-wide. **Ask (B) is not at risk from this diff.** |
| Does the ordering survive a **no-op** re-run? | **Yes, loudly** — Error #5 fires and the scale is still forced afterwards. |
| Does it survive an **asynchronous** one? | ⛔ **No — see WARN-2.** The engine has a real deferral path, and in it the scale correction lands *before* the rebuild that undoes it. |
| Does the re-run itself re-substitute the scale (`TASK-1071`)? | ⛔ **YES — see WARN-1.** `bIsDefaultTransform` is computed **TRUE**, not false. The author's mitigation (colour before scale) is what saves it; his stated reason for why it was safe is wrong, and if a later reader trusts the reason and swaps the order, the fog goes invisible again. |
| Does it dirty a package (`GFX-§11`)? | **No.** The actor carries `RF_Transient` (`FogVolume.cpp:698`) and `UObject::MarkPackageDirty` early-outs on transient objects; the engine also explicitly restores clean-package state after a rerun (`ActorConstruction.cpp:314-327`). ✅ Fence holds — for the editor lane, which is the only lane the call exists in. |
| **Is it legal to call at all?** | ⛔ **NO — BLOCKER-1.** It is `#if WITH_EDITOR`. Everything above describes what happens in the *editor* lane; in the cooked lane the call does not compile and the mechanism does not exist. |

## What only a runtime can settle (owed to `TASK-1167`, by name)
1. ⭐ **Whether the vendor's construction script actually rebuilds its MID from `BoxMaterials["Base"]`** at all. Nothing here proves it; the mismatch Error at `FogVolume.cpp:1007-1015` is the instrument, and *"Error #5 fires on every fog raise"* is a perfectly possible outcome of a green compile.
2. **cl. (3)'s pixels**: far-field B/G `0.895 ⇒ ~0.963`, saturation `14.7 % ⇒ 5.9 %`, same vantage, before **and** after.
3. 🚨 **cl. (2)'s uniformity non-regression, in numbers**: whiteouts stay **0 %**, median ground visibility stays **≈ 647 uu**. (See the ruling below — this is *transferred*, not *missing*.)
4. **The eight named mutations M1-M8** — `NO WITNESSED RED` is inherited and must be discharged **by name** (`TASK-987` cl. 6g/6j). ⚠️ M3 is written against the defective call and will need re-naming with the fix.
5. **The cost** of a construction re-run on a world-sized actor, once per fog raise, in whatever form survives.
6. **`MI_SiegeFog_Grey.uasset` LFS materialisation** — I confirmed the file exists on disk; I cannot confirm it is not a pointer stub. Verify by **oid-vs-`sha256`, never by size** (`SC-§68`).

## The row's own checklist, answered
| check | verdict |
|---|---|
| **(a)** uniformity non-regression in two numbers | ✅ **NOT A BLOCKER — and I say so deliberately.** The row's cl. (3) and cl. (7) grant an explicit escape: *"write `NOT MEASURED` in those words and transfer the duty BY NAME"*. The handoff does exactly that, names `TASK-1167`, and the physical argument is sound — **the pixels cannot exist before a compile, and this row is forbidden to compile (`SC-§27`)**. That is a transfer, ⛔ not silence. The numbers are now **`TASK-1167`'s debt, by name.** |
| **(b)** which of (2a)(a)/(b), with reason | ✅ (a), with the reason measured rather than asserted, and the losing shape's cost enumerated. |
| **(c)** zero `SpawnActorDeferred` | ✅ verified by me — 0 on code lines (the single textual hit, `:738`, is the pre-existing fence comment). |
| **(d)** verification is sampled pixels, not a read-back | ⚠️ transferred under (h); the diff's own read-backs are correctly framed as *necessary and insufficient* (`FogVolume.cpp:984-987`). |
| **(e)** zero `.umap` / `.uasset` / `Config/**` / `SiegeFogStatics.*`, `DirectionalLight_0` untouched | ⚠️ **ACCEPTED-AS-DECLARED** (no Git, no engine). The three files I read contain nothing that writes an asset, and `MarkPackageDirty`/`SavePackage` are 0 on code lines. |
| **(f)** `EditDefaultsOnly` + load-failure logs-and-continues | ⚠️ departure declared — see WARN-6; the substance (zero-code retune) and the whole failure boundary hold. |
| **(g)** no second seam / actor / subsystem | ✅ one private method on the existing class, one caller, inside the existing spawn path. |
| **(h)** pixels **or** the words *"NOT MEASURED"* + named transfer | ✅ verbatim, to `TASK-1167`. |

## Law-text correction for the manager (⛔ NOT edited by me — `SC-§82`)
✅ **The author's §0 claim is CONFIRMED, measured.** `CONVENTIONS.md:11103` (`FOG-§12.4a`) states *"`AFogVolume::RefreshFogVisual()` **already forces the fog's SCALE after spawn**"*. It does not: `RefreshFogVisual()` (`FogVolume.cpp:581-642`) contains **no** `SpawnActor`, **no** spawn null-check and **no** `SetActorScale3D` — it *calls* `SpawnFogVisual()` (`:626`), which holds all three (`SetActorScale3D` at `:755`). The `TASK-1165` row's cl. (1) carries the same slip. ⇒ **the landmark wins over the function name**, the author put the write in the right function, and the law text — **not** the code — is what needs the correction.

## Notes for build-master
⛔ **None — this is a FAIL and nothing may be staged, compiled-for-commit or committed.** When it returns:
- the fix must be judged against the **cooked** target, not only `GitClaudeUnrealTestEditor` — ⚠️ **the editor compile cannot see BLOCKER-1**, which is the entire reason it reached this gate;
- the inherited debts travel with it: `NOT MEASURED` (whiteout % + median visibility + B/G before/after) and `NO WITNESSED RED` (M1-M8, re-named as needed), both **by name**;
- stage **this** report path (see the note at the top of this file), and verify the **commit**, never the index.

## Suite delta (declared, `TL-§5b`)
+2 tests in the existing `Siegebound/Tests/SiegeFogVisualTest.cpp` (10 = Lane A executed, 11 = Lane B source-text); no new test file; no existing assertion edited or renumbered — verified by reading, ⛔ not by running.

---
---

# 🔍 LOOP 2 — re-gate of `TASK-1165` **REVISION 2** (2026-09-08)

**Verdict: PASS** — **0 BLOCKERS**, 4 WARN (all new), 5 NIT.
⚠️ **Loop 1 above is preserved intact.** Its BLOCKER-1 is **CLOSED**; its six WARN and three NIT are each re-judged below. This is loop **2 of 3** on `TASK-1165`.

⛔ **What I still do not hold (`SC-§71b`):** no `Bash`, no MCP, no engine, no Git. Every **pixel**, **compile**, **suite-run**, **byte-scan** and **git** claim originating in the handoff is **ACCEPTED-AS-DECLARED** and labelled. ✅ What I verified **first-hand by reading**: the three diff files, and the **installed UE 5.8 source** (`C:/Program Files/Epic Games/UE_5.8/Engine/Source/...`) — including a **re-derivation of preprocessor depth from the raw directive list**, not a re-reading of the author's citations.

---

## 1. ⚖️ THE RULING ON cl. (2b) — **LEGITIMATE DEPARTURE**

**cl. (2b), verbatim (`TASKBOARD.md:1346`):** *"FORBIDDEN, BY NAME … `SpawnActorDeferred` + `FinishSpawning`. The engine's own comment beside the substitution says `bIsDefaultTransform` is FALSE IN A COOKED BUILD ⇒ the deferred form behaves DIFFERENTLY in the editor and in the packaged game, so a defect could return ONLY IN THE SHIPPED PRODUCT."*

**Ruled: the departure is legitimate. The clause's stated reason is FALSE in UE 5.8, and no cooked-only hazard survives the shape as written.** Measured, first-hand, in four steps:

| # | what I measured | file:line | result |
|---|---|---|---|
| 1 | Where the engine comment actually sits | `SCS_Node.cpp:142` opens `if(bIsDefaultTransform)`; the comment is `:144-146`; the substitution `WorldTransform.SetScale3D(NewSceneComp->GetRelativeScale3D())` is `:147` | ✅ The comment is **inside the `TRUE` branch** — it describes the case where the substitution **fires**, not the deferred one. |
| 2 | Which path reaches it with `true` | `Actor.cpp:4358-4360`: `if (!bDeferConstruction) { FinishSpawning(UserSpawnTransform, true); }` — **hardcoded**, no `#if` of any kind (directive tally: 4339/4345 closed before it) | ✅ The **non-deferred** path — the shape this file had **before** this change. In 5.8 it passes `true` in **both** targets, so the engine's own comment is **stale**. |
| 3 | What the deferred path passes | `FogVolume.cpp:765-766`: `Spawned->FinishSpawning(SpawnTransform, /*bIsDefaultTransform=*/ false, nullptr, ESpawnActorScaleMethod::OverrideRootScale);` | ✅ A **literal in our own file**. It cannot vary by target, and test 11 (`SiegeFogVisualTest.cpp:1677-1678`) pins it at 1 so it cannot silently become defaulted. |
| 4 | Whether **any other** editor/cooked asymmetry exists on the new path | `ActorConstruction.cpp:932-936` — the `bTurnOffEditorConstructionScript` lookup | ⚠️ One exists, and it runs in the **safe direction**: cooked **always** runs the UCS; only the **editor** can skip it. It is **shared with the old shape** (it lives inside `ExecuteConstruction`, which both reach), and the key is **absent from this project's `Config/**`** (my grep). ⇒ not a hazard this diff introduces. |

⭐ **And the departure is strictly better than compliance would have been.** With `bIsDefaultTransform=false` + `OverrideRootScale`, `SCS_Node.cpp:133-136` uses the provided transform and `:142` is **never entered** ⇒ **`TASK-1071` is fixed at its source**, rather than repaired one line later. `Spawned->SetActorScale3D` is now genuinely belt-and-braces, exactly as the author says.

⇒ **Recommendation to the manager (`SC-§82`, ⛔ not edited by me):** cl. (2b)'s *reason* is void under `SC-§110`. Its *prohibition on the `SpawnActorDeferred` helper* should be **kept** — but re-grounded on the hazard measured in **WARN-7** below, because the replacement reason the author wrote is also wrong.

---

## 2. THE COOKED-BUILD CLAIM — **RE-DERIVED, NOT ACCEPTED AS READ**

⛔ I did not take the citations. I extracted every `#if/#ifdef/#ifndef/#else/#elif/#endif` line from each file and tallied the nesting from line 1.

| link | file:line | derivation |
|---|---|---|
| `FinishSpawning` **declaration** | `Actor.h:3117` | Directive pairs from `:20` to `:3117` all balance; the last is **3007/3010**, and there is **no directive between 3010 and 3117** ⇒ **depth 0**. `ENGINE_API`, in the `public:` region opened at `:3079`. ✅ |
| `FinishSpawning` **definition** | `Actor.cpp:4374` | Last pair before it is **4339/4345** ⇒ **depth 0**. ✅ |
| the `ExecuteConstruction` call | `Actor.cpp:4415` | The only directive inside the body is **4376/4379** (`ENABLE_SPAWNACTORTIMER`), closed ⇒ **depth 0**. Its only enclosing condition is `if (ensure(!bHasFinishedSpawning))` (`:4381`). ✅ |
| `ExecuteConstruction` **definition** | `ActorConstruction.cpp:818` | **253** `#if WITH_EDITOR` → **590/592** nested → **811 `#endif`**. ⇒ the *entire* re-run machinery (including `RerunConstructionScripts`) is editor-only, and `:818` is **depth 0**. The `#if` at 823/827 is only the re-entrancy `checkf`. ✅ |
| the vendor script runs in a cooked build | `ActorConstruction.cpp:932-940` | **932 `#if WITH_EDITOR` → 936 `#endif`** wraps **only** `bool bDoUserConstructionScript;` + `GConfig->GetBool(...)` + the `if (!GIsEditor \|\| !bDoUserConstructionScript)` line. With `WITH_EDITOR=0` the braced block at **937-940** is left **unconditional** ⇒ **`ProcessUserConstructionScript()` at `:939` runs ALWAYS.** ✅ |

⇒ **The blocker's own claim is CONFIRMED.** `FinishSpawning` genuinely runs the vendor's construction script in a cooked build, and no editor-only API remains anywhere in `FogVolume.cpp` (`RerunConstructionScripts` appears **only** on `//` comment lines `:708-710`, which `CountOccurrencesInCode` skips — so the suite's `== 0` row is accurate and passes).

**`SC-§111` is satisfied in substance, not merely cited:** cl. 3(c) was obeyed literally — the API is **gone, not guarded** — and the cooked-build property is now pinned by *two executable rows* (`RerunConstructionScripts` == 0 at `SiegeFogVisualTest.cpp:1625-1626`; the spelled literal at `:1677-1678`) in the only lane that can see it, since this project compiles **only** the editor target.

⭐ **A second cooked-only trap I went looking for and did NOT find — recorded because it nearly was one.** `UEnum::GetDisplayNameTextByIndex` (`Class.h:3021`) carries the warning *"If called from a cooked build this will normally return the short name as Metadata is not available."* For a **Blueprint** enum whose internal names are generated placeholders, that would have made the key at `FogVolume.cpp:979` resolve in PIE and **fail in the packaged game only** — the exact class this chain exists to stop. It does **not**, and here is why, measured: `UUserDefinedEnum::DisplayNameMap` is a plain `UPROPERTY()` at `UserDefinedEnum.h:41-42`, **outside** the `#if WITH_EDITORONLY_DATA` block that closes at `:35` ⇒ it is **serialized into the cook**, and `UUserDefinedEnum::GetDisplayNameTextByIndex` (`UserDefinedEnum.cpp:195-206`) reads it identically in both targets. And for a **native** enum the base-class fallback returns the short name — which *is* the authored name — so the `GetNameStringByIndex` branch matches instead. ⇒ **the `||` of the two lookups (`FogVolume.cpp:978-979`) is target-safe in both enum shapes.** ✅

---

## 3. FINDINGS — LOOP 2

### WARN-7 — ⭐ **the sharp one.** `FogVolume.cpp:804-808` and `SiegeFogVisualTest.cpp:1699-1711` — **the fence's replacement reason is ALSO false.** The ban is right; the reason written beside it is not, and it is the second false engine claim to be recorded in this same fence.

Both comments say the helper *"defaults `TransformScaleMethod` to `MultiplyWithRoot` … what the shipped path uses is `SpawnActor` with `bDeferConstruction`, which keeps `OverrideRootScale`."* **Measured:**

- `FActorSpawnParameters::TransformScaleMethod` **also** defaults to `MultiplyWithRoot` — `World.h:457` — and `FogVolume.cpp` never overrides it. ⇒ the shipped shape carries the **same** default as the helper.
- On the deferred path that field is consulted **only** at `Actor.cpp:4306-4324`, inside `if (SceneRootComponent != nullptr)` — i.e. **only when a native root exists**. This visual has none. ⇒ it is **inert in both shapes**.
- `OverrideRootScale` reaches the SCS **solely** through the fourth argument of `FinishSpawning` (`FogVolume.cpp:766`) — which the helper shape could pass identically. (Note the non-deferred path never forwards `SpawnParams.TransformScaleMethod` to construction at all: `Actor.cpp:4360` calls `FinishSpawning(T, true)` with two arguments, taking the header default `OverrideRootScale` at `Actor.h:3117`.)

⇒ **the stated distinction between the two shapes does not exist.** ⭐ **The real disqualifier — and it is a good one:** `UWorld::SpawnActorDeferred` (`World.h:3851-3858`) has **no `ObjectFlags` parameter**, so it cannot carry `SpawnParams.ObjectFlags |= RF_Transient` (`FogVolume.cpp:698`). A world-sized opaque box without `RF_Transient` is the never-save hazard the file's own comment at `:692-698` spells out. The handoff §1's table names this; the two shipped comments do not. **Suggested fix (hypothesis, `SC-§101`): replace the scale-method sentence in both places with the `ObjectFlags`/`RF_Transient` one.** ⛔ Not a behavioural defect — behaviour is correct.

### WARN-8 — `FogVolume.cpp:1096`, `:1127-1134` — `VerifyFogVisualMaterial` **walks** every mesh and every slot, but its **verdict is ANY, not ALL**. It can pass while something else renders.
`bWearingRequestedMaterial` is set true by the **first** matching slot and never re-cleared, so a visual with two slots where only one carries `MI_SiegeFog_Grey` reports **green**. ✅ This is strictly better than loop 1's `FindComponentByClass` + element 0 (NIT-3 **is** fixed — the non-determinism is gone, `GetMaterial(0)` is pinned at 0, and zero slots now reaches the Error). ✅ The Error text (`:1137`) is accurate to the ANY semantics (*"none of the %d … slot(s) … carries it"*). ⛔ But the row this diff is answering — `SC-§104`, resolved **state** — is only partially met: the state resolved is *"at least one slot wears it"*, not *"the mesh renders it"*. Low risk today (one mesh, and the vendor builds from one map entry); it becomes a false-pass the day the vendor grows a slot. Record it so nobody reads the walk as the verdict.

### WARN-9 — `FogVolume.cpp:979` — the display-name branch is `FText`, therefore **culture-dependent**, and the culture-invariant twin exists one line away in the engine.
For a Blueprint enum the internal name is a generated placeholder, so the **display** branch is the one that hits — and `DisplayNameMap` holds `FText`. Under a non-English culture it can resolve to a translated string ≠ `"Base"` ⇒ Error #4 on every raise, **loudly**, and beige fog. The author declares this (§7.6) and the failure is loud, so it is not a blocker; English is the only shipped culture. ⭐ **Cheap hardening (hypothesis):** `UEnum::GetAuthoredNameStringByIndex` — `UUserDefinedEnum` overrides it at `UserDefinedEnum.cpp:208-221` to return the **source string** via `FTextInspector::GetSourceString`, which is culture-invariant and serialized in the same `DisplayNameMap`.

### WARN-10 — `FogVolume.cpp:898` — production **loads** an asset chain the file's own test **refuses to load**, on a stated `FOG-§6`/`GFX-§11` hazard.
`FogVisualMaterialPath().TryLoad()` runs on every fog raise. `SiegeFogVisualTest.cpp:1523-1527` declines to load the very same asset and says why: *"`MI_SiegeFog_Grey` is parented to the VENDOR box material, and `TASK-841` §5.3 measured that loading a vendor package DIRTIES IT — which is how `FOG-§6`'s read-only rule gets broken by a test rather than by a person."* ⇒ **the test and the shipped code disagree about whether that load is safe.** ⭐ Mitigating, and why this is a WARN rather than a blocker: `BP_SiegeFog`'s own mesh already references the vendor parent material, so the vendor package is **resident before this line ever runs** — the incremental load is our own child asset. ⛔ I cannot measure a dirty flag. **Transferred to `TASK-1167` by name:** after one fog raise, confirm no package is dirty.

---

### NIT-4 — `SiegeFogVisualTest.cpp:1625-1626` — the cooked-build fence is a **whole-file** property asserted **inside** the `if (ExtractFunctionBody(… SpawnFogVisualSignature …))` block opened at `:1604`. It fails closed (a stale signature calls `AddError`), so this is a placement nit, not a hole — but the single most important row in the file should not depend on an unrelated extraction succeeding.

### NIT-5 — **no mutation covers the scale ordering, and that gap is invisible to every runtime instrument.** Moving `Spawned->SetActorScale3D` **above** `FinishSpawning` would be a **silent no-op** (`Actor.cpp:5078-5084`: the call early-outs on `if (RootComponent)`, and there is no root in that window) — and the achieved-scale readback would then still read **green**, because on the deferred path the SCS already delivers the correct scale. ⇒ it is caught **only** by source text: `SiegeFogVisualTest.cpp:1670-1671` and `:1638-1639`, plus test 7's `:1049-1053`. ⭐ **Suggest an M13** (*move the scale set above `FinishSpawning`*) on the transferred list, precisely because no `Error` can see it.
⭐ Note the converse, which is a genuine strength: if construction ever produced **no** root at all, `GetActorScale3D` returns `(1,1,1)` (`Actor.cpp:5087-5094`) against a world-sized request ⇒ the existing scale Error fires **loudly**. The unfinished-actor failure mode the author feared is instrumented, not silent.

### NIT-6 — `FogVolume.cpp:1115-1118` — the MID unwrap is a `while` loop over `Parent`. A cyclic parent chain would hang the game thread. Not producible by any engine path (and `Parent` is public at `MaterialInstance.h:647`, region `public:` at `:620`, so the read compiles); recorded only because the loop has no depth bound.

### NIT-7 — `FogVolume.cpp:940` — the `!MaterialValueProperty->PropertyClass` half of Error #3 is effectively unreachable (an `FObjectProperty` always carries one). Fail-closed and free; noted so nobody counts it as a live branch.

### NIT-8 — ⚖️ **two sources of truth for a shipped number, routed to the manager (⛔ not edited by me).** `TASKBOARD.md:1344` (cl. 2) records ask (B) as *"`Density 0.5`, `Base Noise Sharpness 0.1`, **`Wind Speed 0.005`**"*; the handoff §4 and my dispatch both say **`wind Speed 0.5`**. ⛔ Nothing in this diff names any of them, so **ask (B) is unaffected either way** — but one of those two documents is wrong about a value that shipped in `20c1bea`, and the next reader will inherit whichever he reads first.

---

## 4. LOOP 1's FINDINGS, EACH RE-JUDGED

| loop 1 | disposition, measured |
|---|---|
| 🚨 **BLOCKER-1** (editor-only API) | ✅ **CLOSED — and closed the right way.** The API is **gone**, not guarded (`SC-§111` cl. 3(c) obeyed). 0 occurrences on any code line; the only textual hits are `//` comments at `:708-710`. The replacement chain is **re-derived** in §2 above. |
| **WARN-1** (the `bIsDefaultTransform` prose was backwards) | ✅ **CLOSED, and better than claimed.** The order is kept; the reason is corrected; and under the new shape the substitution branch (`SCS_Node.cpp:142`) is **never entered at all**, so `TASK-1071` is fixed at its source. The comment at `FogVolume.cpp:788-795` states this accurately, including that the line is now belt-and-braces. |
| **WARN-2** (a deferred re-run could land after a green readback) | ✅ **RETIRED — confirmed.** `FScopedSuspendRerunConstructionScripts` (`ActorConstruction.cpp:256-261`) lives **inside** the `253→811` `#if WITH_EDITOR` block and is reachable **only** from `RerunConstructionScripts`. `FinishSpawning` calls `ExecuteConstruction` **inline and synchronously** (`Actor.cpp:4415`). There is no window. |
| **WARN-3** (re-validate `Spawned` after the material step) | ✅ **DECLINE UPHELD, and the reasoning holds — I checked the part he asserted.** The window now contains reflection plus one synchronous `TryLoad()`. The actor **cannot** die in it: it is referenced by the level's actor array from `LevelActor.cpp:739`, and synchronous loading holds the GC lock. And the guard would genuinely hurt: a `return` between the write and `FinishSpawning` strands an actor with no root, no mesh and no `BeginPlay`. ⭐ The **pair** of assertions he substituted (`return` before the call = 3 **and** in the whole function = 3, `SiegeFogVisualTest.cpp:1656-1659`) is the correct structural answer — I verified both counts are really 3 (`FogVolume.cpp:649/666/734`; the `//` line at `:751` that contains the word is skipped by the counter). |
| **WARN-4** (the "one producer" byte scan) | ⚠️ **ACCEPTED-AS-DECLARED.** His `grep -rl --binary-files=text` result, the 605,098-byte size and the positive control are all **Bash claims** I cannot run. The *code-side* half I re-verified myself: one spawn site, one caller, one apply site. |
| **WARN-5** (test 10 passes with the feature deleted) | ✅ **UNCHANGED AND RE-DECLARED**, correctly. ⭐ But the dispatch's question — *would the tests fail if the feature were removed?* — is **YES for the feature as a whole**: deleting `ApplyFogVisualMaterial` reds `:1606`, `:1915` **and** the `ExtractFunctionBody` guard at `:1721`; deleting `VerifyFogVisualMaterial` reds `:1862`, `:1864` and `:1817`; deleting `bDeferConstruction` reds `:1615`. Test 10 alone is the part that survives. |
| **WARN-6** (`EditDefaultsOnly` departure) | ⚖️ **UNCHANGED** — ruled non-blocking in loop 1, routed to the manager, nothing in rev 2 disturbs it. |
| **NIT-1** (Error #4 described an attempt that never happened) | ✅ **FIXED.** `FogVolume.cpp:1026-1042` is now its own branch with its own message, pinned at `SiegeFogVisualTest.cpp:1794-1795` so it cannot be silently re-merged. |
| **NIT-2** (no type check on the map's value class) | ✅ **FIXED — and his sharper reading is CORRECT, measured.** `FObjectPropertyBase::SetObjectPropertyValue` (`PropertyBaseObject.cpp:682-692`, public at `UnrealType.h:2899`) guards **nullability only**; there is no type check. A wrong-typed store would therefore succeed and the readback at `FogVolume.cpp:1058` would compare **the pointer it had just written against itself** ⇒ **green**. The pre-store check at `:940` closes exactly that, and its position is pinned at `SiegeFogVisualTest.cpp:1742-1745`. ⭐ **And I checked the rest of the verify path for the same self-comparison shape: it does not recur.** `VerifyFogVisualMaterial` compares `FogVisualMaterialPath()` against a path read off `Mesh->GetMaterial(SlotIndex)` — an **independent** source — and `FogVisualMaterialMatches` refuses two empty paths (`FogVolume.cpp:571-574`), which is the degenerate self-comparison that would otherwise certify a mesh rendering nothing. |
| **NIT-3** (`GetMaterial(0)` + `FindComponentByClass`) | ✅ **FIXED** — every mesh, every slot, count reported, `GetMaterial(0)` pinned at 0. ⚠️ See **WARN-8** for the ANY-vs-ALL residual. |

---

## 5. COUNTS, COVERAGE, AND THE MUTATIONS

✅ **Error sites 5 ⇒ 7 is REAL, and I counted them off the source, not the handoff:** `ApplyFogVisualMaterial` = **6** (`FogVolume.cpp:901`, `922`, `942`, `990`, `1034`, `1060`) and `VerifyFogVisualMaterial` = **1** (`:1136`). The suite pins 6 and 1 separately (`:1788-1789`, `:1844-1845`).
✅ **Every Error branch is reachable** — (1) asset gone · (2) vendor rename/retype of the map · (3) vendor retype of the value class · (4) mode renamed/removed · (5) entry absent under the key · (6) the measured `TASK-1154` write-that-did-nothing · (7) the mesh wearing something else, which the author says is a live possibility. (NIT-7 notes the one dead half-condition.)
✅ **`SC-§109` clean.** No log line tells a reader what to conclude, report or paste; each names what failed, where, and what the player sees. Error #7's *"either … or …"* is a disjunction of causes, not a conclusion.
✅ **Mutations 8 ⇒ 12 is REAL** (M1-M12 enumerated). ⭐ **M9 as written WOULD red**, verified against the assertion it targets: re-introducing `Visual->RerunConstructionScripts();` on a code line moves `CountOccurrencesInCode(FogCpp, "RerunConstructionScripts")` from 0 to 1 at `SiegeFogVisualTest.cpp:1626`. **His nomination of M9 to run first is correct** — it is the only row in the whole list that fails a defect the editor compile passes, and this project compiles no other target. ⚠️ See **NIT-5** for the one mutation the list is missing.
⚠️ **Pre-flight was 65/65 needles in a re-implementation and a text simulation — ⛔ NOT a suite run.** ACCEPTED-AS-DECLARED. I independently re-checked the load-bearing needles against the edited source by hand: `bDeferConstruction` = 1 (`:725`), `Spawned->FinishSpawning(` = 1 (`:765`), `/*bIsDefaultTransform=*/ false` = 1, `ApplyFogVisualMaterial(Spawned);` = 1 (`:745`), `VerifyFogVisualMaterial(Spawned);` = 1 (`:771`), `return`/`return;` = 3 both scoped and whole-body, `RerunConstructionScripts` = 0, `SpawnActorDeferred` = 0, `TransformScaleMethod` = 0 (`ESpawnActorScaleMethod::OverrideRootScale` does not contain the needle), `GetNumMaterials()` = 1, `GetMaterial(0)` = 0, `GetMaterial(SlotIndex)` = 1, `SetObjectPropertyValue(` = 1 in Apply / 0 in Verify. **All three fixture signatures match the shipped definitions** (`SiegeFogVisualTest.cpp:145`, `:157`, `:158` vs `FogVolume.cpp:644`, `:878`, `:1071`), so no probe is scanning nothing (`SC-§38`). ⛔ Read, not run — `TASK-1167` owes the executed `N/M`.
✅ **Compile-surface sanity (read, not compiled):** every include the new code needs is already present (`FogVolume.cpp:6`, `12`, `13`, `14`, `19`, `20`, `21`); `AActor::GetComponents(TArray<T*, Alloc>&) const` exists at `Actor.h:4023` so the `const AActor*` call is legal; `FProperty::GetElementSize()` is the **modern** accessor (`UnrealType.h:293` — the deprecation at `:179` is on the old `ElementSize` member, which this code does not use); `UMaterialInstance::Parent` is public.

---

## 6. THE ROW'S CHECKLIST — LOOP 2

| check | verdict |
|---|---|
| **(a)** uniformity non-regression in two numbers | ⚠️ **Still transferred, still not a blocker.** The manager ruled loop 1's exercise of the cl. (3)/(7) escape correct; nothing changed. Duty remains `TASK-1167`'s, by name. |
| **(b)** which of (2a)(a)/(b), with reason | ✅ (a), unchanged; only the **transport** moved, and the losing alternatives are each refuted on a measured ground. |
| **(c)** zero `SpawnActorDeferred` | ✅ **Satisfied literally** — 0 on code lines file-wide (`FogVolume.cpp:796` and `:804` are `//` comments). The departure from the clause's **substance** is ruled legitimate in §1. |
| **(d)** verification is sampled pixels, not a read-back | ⚠️ Transferred under (h). The diff frames its own read-backs as *necessary and insufficient* (`FogVolume.h:1306-1310`) — correctly. |
| **(e)** zero `.umap`/`.uasset`/`Config/**`/`SiegeFogStatics.*`, `DirectionalLight_0` untouched | ⚠️ **ACCEPTED-AS-DECLARED** (no Git). The three files I read contain no asset write; `MarkPackageDirty`/`SavePackage` = 0 on code lines. ⚠️ See **WARN-10** for the one *load* that touches vendor territory. |
| **(f)** `EditDefaultsOnly` + load-failure logs-and-continues | ⚠️ Departure unchanged (WARN-6). The failure boundary holds on **all seven** sites: `return false` = 0, `FogActiveUntilTimeSeconds` = 0, `IsFogActive()` = 0 in both new bodies, all pinned. |
| **(g)** no second seam / actor / subsystem | ✅ Two private methods on the existing class, one caller each, inside the existing spawn path. |
| **(h)** pixels **or** *"NOT MEASURED"* + named transfer | ✅ Verbatim, to `TASK-1167`. |
| ⭐ **ask (B) re-confirmation** | ✅ **STRUCTURALLY SAFE, and STRONGER than in rev 1.** `SetMaterial(` / `CreateDynamicMaterialInstance` / `SetScalarParameterValue` / `SetVectorParameterValue` = **0** on any code line, banned file-wide at `SiegeFogVisualTest.cpp:1693-1717`. ⭐ The new shape removes the last theoretical window: the vendor's script builds its MID from **our** entry on its **first** pass, so there is never a tuned MID to orphan. `density 0.5` / `sharpness 0.1` / wind speed are the vendor's own struct defaults and this file names none of them. ⚠️ See **NIT-8** on the two documents disagreeing about the wind value. |

---

## 7. WHAT ONLY `TASK-1167`'s RUNTIME CAN SETTLE (⛔ carried + new)

1. ⭐⭐ **Whether the vendor's construction script actually sources its MID from `BoxMaterials["Base"]`.** ⛔ Nothing in `Source/` can prove it; the author says so plainly. **Error #7 (`FogVolume.cpp:1136`) is the instrument, and *"it fires on every raise"* is a perfectly possible outcome of a green compile and a green suite.** This is the single largest residual in the whole row.
2. **cl. (3)'s pixels:** far-field B/G `0.895 ⇒ ~0.963`, saturation `14.7 % ⇒ 5.9 %`, same vantage, before **and** after.
3. 🚨 **cl. (2)'s uniformity non-regression, in numbers:** whiteouts stay **0 %**, median ground visibility stays **≈ 647 uu**.
4. **M1-M12 by name** — `NO WITNESSED RED` is inherited. ⭐ **Run M9 first** (confirmed it reds). ⭐ **Add M13** per NIT-5.
5. **The `Base` key resolving at runtime** — Error #4 staying silent is the evidence that the vendor's enum lookup hit.
6. ⭐ **NEW (WARN-10):** after one fog raise, confirm **no package is dirty** — the production `TryLoad()` walks a chain into the vendor pack the file's own test refuses to load.
7. **`MI_SiegeFog_Grey.uasset` LFS materialisation** — by **oid-vs-`sha256`, never by size** (`SC-§68`).
8. ⛔ **The cooked target is still never compiled by this project.** `Build.bat GitClaudeUnrealTestEditor` cannot see a `WITH_EDITOR` divergence; M9 and the two source-text pins are the only proxies that exist.

---

## 8. NOTES FOR BUILD-MASTER (`TASK-1167`)

- ✅ **PASS. Compile, run the suite, and commit** — the hard gate is met.
- 🚨 **Stage `.claude/pipeline/qa/TASK-1166-report.md`** (this file, both loops). The board's `names:` field has been reconciled to this path; a pathspec aimed at `qa/TASK-1165.md` answers with **silence** (`SC-§102`).
- ⛔ Both inherited debts travel with the commit **by name**: `NOT MEASURED` (whiteout % + median visibility + B/G before/after) and `NO WITNESSED RED` (**M1-M12**, M9 first).
- ⚠️ **The editor compile going green proves nothing about the cooked target.** That is the whole lesson of loop 1. If a cooked/packaged build is ever run, this is the diff to run it against.
- ⚠️ Four WARN and five NIT are **open by design, not overlooked** — WARN-7 (the fence's second false reason) and NIT-8 (the wind-speed disagreement) are **manager routes**, not build blockers.

## 9. LAW-TEXT CORRECTIONS FOR THE MANAGER (⛔ NOT edited by me — `SC-§82`)

1. ⭐ **`TASK-1165` cl. (2b) — its stated reason is void (`SC-§110`).** Keep the prohibition on the `SpawnActorDeferred` **helper**; re-ground it on **`World.h:3851-3858` has no `ObjectFlags` parameter ⇒ it cannot carry `RF_Transient`**, not on `bIsDefaultTransform` and not on `TransformScaleMethod` (see §1 and WARN-7). The engine comment at `SCS_Node.cpp:144-146` that the clause quotes is **itself stale** in 5.8.
2. **NIT-8** — reconcile `Wind Speed 0.005` (`TASKBOARD.md:1344`) against `0.5` (handoff §4) against what actually shipped in `20c1bea`.
3. Loop 1's `FOG-§12.4a` / cl. (1) correction is already **discharged** by the manager (`SC-§110`); nothing further owed.
