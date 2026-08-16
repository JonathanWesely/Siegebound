# TASK-558 — [WR-4] `ATorch`, the interior light prop — programmer handoff

- **assignee:** gameplay-programmer
- **status on completion:** `ready-for-qa`
- **gate:** ⛔ **TASK-565** (`qa/TASK-565.md`) — this task is one of the nine that gate names.
- **law worked to:** CONVENTIONS **`WR-§4`** · **`WR-§8`** · **`SC-§34`** · **`SC-§33`** · **`SC-§15`** · **`SC-§18c`** · **`SC-§22`** + "C++ layout — Siegebound gameplay" (plain gameplay-actor names) + the shadow law.
- **compile / editor / Git / MCP / PIE:** ⛔ **NONE PERFORMED.** File-only, as dispatched. The batch's only compile is TASK-566, its only gate TASK-565, its only commit TASK-570.

---

## 1. What changed — ⭐ TWO NEW FILES, ZERO EXISTING FILES TOUCHED

| file | disposition |
|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/Torch.h` | **NEW** |
| `Source/GitClaudeUnrealTest/Siegebound/Torch.cpp` | **NEW** |

⛔ **Nothing else.** No `Castle.{h,cpp}`, no `Build.cs`, no `Content/`, no `.csv`, no `Tests/`, no existing file of any kind.
⭐ **The diff is one new file pair, which is the shape the spec asked for** (the `FSiegeNavDiagnostics` / `FSiegeAssistantRegionStatics` precedent): **this task adds NO call site and edits NOTHING.** `ATorch` currently has exactly one intended caller, **TASK-562**, which has not run yet.

**Read-only working-tree evidence** (`git status --porcelain`, run read-only for evidence — ⛔ no Git operation performed):

```
 M GitClaudeUnrealTest/.claude/pipeline/CONVENTIONS.md
 M GitClaudeUnrealTest/.claude/pipeline/TASKBOARD.md
 M GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/AncientGround.h
 M GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/Castle.h
 M GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/SiegeAssistantConsoleWidget.cpp
 M GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/SiegeAssistantConsoleWidget.h
 M GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/SiegeGameMode.cpp
 M GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.h
?? GitClaudeUnrealTest/.claude/pipeline/handoffs/TASK-554-programmer.md
?? GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/CommanderNpc.cpp
?? GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/CommanderNpc.h
?? GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/Torch.cpp
?? GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/Torch.h
?? GitClaudeUnrealTest/Tools/ArtPipeline/build_warroom_props.py
```

⚠️ **Everything in that list other than `Torch.{h,cpp}` belongs to a parallel task** (554 · 557 · 559 · 561 · 556). ✅ **My two files are `??` (untracked, new) — confirming zero existing-file edits from this task.** ⛔ The snapshot is a moment in time; 557/559/560/561 were writing concurrently.

---

## 2. ⚠️⚠️ FLAGGED DECISION **D3** — WHAT I DEFAULTED TO, WHY, AND ⛔ WHAT I DID **NOT** DECIDE

⛔ **I did not rule D3 and this handoff does not ask anyone to treat it as ruled.** Jonathan's call stands open.

**What ships:** `TorchLight` mobility **Movable**, `CastShadows` **false** — because that is what **`WR-§4` states as law**, not because I weighed the perf question. The reason is structural and is pasted into the header comment as the spec required:

1. A **runtime-spawned** light does not exist when lighting is built ⇒ **no lightmap entry, no static shadow to sample** ⇒ Static mobility would give a torch that emits nothing at all.
2. The only Static/Stationary path needs a lighting build **plus a save of `L_Arena`**, which **`WR-§3` forbids outright** (the one-time nav-data-only save exception was granted for TASK-350 and **expired at that commit**).
3. **Shadows off is what makes Movable cheap** — the shadow-depth pass is the expensive half, and it is the half that would scale with `MaxTorchesPerCastle` × 2 castles.

⛔ **NO FRAME-RATE FIGURE IS QUOTED ANYWHERE IN THE CODE OR IN THIS DOCUMENT** (`WR-§4` perf clause: no FPS baseline exists for this project — M7's TASK-183 never ran). ⛔ **No token figure is quoted either** (`AS-§12g`, batch-wide ban).

**How I kept the flip cheap, concretely — three single lines, all locatable by symbol:**

| what | where | how big is the flip |
|---|---|---|
| mobility | `ATorch::ATorch()` → `TorchLight->SetMobility(EComponentMobility::Movable);` | **one line** |
| shadow casting | `ATorch::ApplyTorchLightTuning()` → `TorchLight->SetCastShadows(false);` | **one line** |
| intensity / radius / colour | three `EditDefaultsOnly` UPROPERTYs | **no recompile at all** — `BP_Torch` defaults |

### ⭐ AND THE PART D3 SHOULD BE RULED **WITH**, BECAUSE IT MAKES THE FLIP NOT FREE

⚠️ **`ULocalLightComponent::SetAttenuationRadius` calls `AreDynamicDataChangesAllowed` with `bIgnoreStationary = false`, so on a REGISTERED Stationary *or* Static component it SILENTLY NO-OPS.** (Verified by reading `Engine/Source/Runtime/Engine/Private/Components/LocalLightComponent.cpp` and `Classes/Components/SceneComponent.h:1363`.)

⇒ **If D3 rules Stationary or Static, the `BeginPlay` re-apply of `TorchAttenuationRadius` stops working** — the constructor-time apply still lands (the component is not registered yet), so a designer would tune the radius, see it apply in the editor, and see **nothing change at runtime**, with **no warning and no error**. **Movable is the only mobility under which every tunable in this class is honestly live.** That is a fact about the engine, not an argument for a ruling; it is recorded in the header so the ruling is made with it rather than against it.

---

## 3. 📌 M8 DECLARATION — `WR-§8`, STATED EXPLICITLY

> ⚖️ **`ATorch` IS NET RELEVANCY TIER C — NOT REPLICATED.** Declared in **both** the class doc comment in `Torch.h` and the constructor comment in `Torch.cpp`.

- **`bReplicates` stays at the `AActor` default (`false`) and is never set** — the `AAncientGround` precedent verbatim ("declared, not touched").
- ⛔ **THERE IS NOTHING ELSE TO DECLARE, AND I AM STATING IT RATHER THAN IMPLYING IT:** this task adds **NO replicated property · NO new replicated class · NO new relevancy tier · NO RPC · NO authority-guarded mutation · NO `GetFirstPlayerController` call.**
- **Rationale:** a torch holds no gameplay truth. `ACastle` (TASK-562) spawns an identical set on both machines from its own `EditDefaultsOnly` anchors, so a torch is a **pure local projection of already-replicated castle state** — replicating the prop would buy nothing and cost bandwidth. Same argument the mine pair and the ancient-ground pair already ship.

**Verification grep (raw hit count 9, ALL IN COMMENTS, ZERO IN CODE):**

```
grep -nE "Niagara|ITeamAgent|bReplicates|GetLifetimeReplicatedProps|UFUNCTION\(Server|UFUNCTION\(Client|GetFirstPlayerController|HasAuthority" \
     Source/GitClaudeUnrealTest/Siegebound/Torch.h Source/GitClaudeUnrealTest/Siegebound/Torch.cpp
```

| hit | file:line | classification |
|---|---|---|
| `bReplicates` | `Torch.h:64` | **comment** — the Tier C declaration itself |
| `Niagara` ×3 | `Torch.h:75,76,78` | **comment** — the "no Niagara, recorded as a follow-up" clause |
| `ITeamAgent` | `Torch.h:102` | **comment** — why this actor deliberately does NOT implement it |
| `Niagara` | `Torch.cpp:16` | **comment** — why there is no tick |
| `bReplicates` | `Torch.cpp:20` | **comment** — the Tier C declaration |
| `ITeamAgent` ×2 | `Torch.cpp:28,29` | **comment** — same reason |

✅ **Zero code hits for every one of those eight patterns. That is a RESULT and is reported as one** (`SC-§22`'s closing rule).

---

## 4. ⛔ `SC-§33` DISCHARGE — THE PASTED CALL-SITE GREP

**The claim being audited: this task adds NO trailing defaulted parameter to any function, existing or new.**

⚠️ **`SC-§33`'s own scope clause says it binds "a DEFAULTED parameter added to a function that ALREADY HAS CALL SITES" and does not bind a brand-new function.** ⛔ **I am discharging it anyway rather than citing the exemption, because the dispatch instructed me to and because the gate re-runs it.** Both legs are below.

### Leg 1 — every function this task declares (the complete new signature surface)

```
grep -nE "^\s+(virtual\s+)?(void|bool|FVector)\s+\w+\(|^\s+ATorch\(\)" Source/GitClaudeUnrealTest/Siegebound/Torch.h
```

**raw hit count: 6**

| # | signature | trailing default? |
|---|---|---|
| 1 | `ATorch();` | none — no parameters |
| 2 | `virtual void OnConstruction(const FTransform& Transform) override;` | none — engine override, signature fixed |
| 3 | `virtual void BeginPlay() override;` | none — no parameters |
| 4 | `void ApplyTorchLightTuning();` | none — no parameters |
| 5 | `void ResolveTorchMesh(bool bWarnIfMissing);` | ⛔ **NONE — `bWarnIfMissing` is REQUIRED, deliberately.** See below. |
| 6 | `void PositionLightAtFireBowl();` | none — no parameters |

### Leg 2 — the mechanical sweep for `=` inside any parameter list, in both new files

```
grep -nE "\w+\s*\([^)]*=[^)=]*\)\s*(const)?\s*;" Source/GitClaudeUnrealTest/Siegebound/Torch.h Source/GitClaudeUnrealTest/Siegebound/Torch.cpp
```

**raw hit count: 3 — and ⛔ ALL THREE ARE NAMED-ARGUMENT COMMENTS AT CALL SITES, NOT PARAMETER DECLARATIONS:**

| hit | line | classification |
|---|---|---|
| `ResolveTorchMesh(/*bWarnIfMissing=*/ false);` | `Torch.cpp:80` | **(i) not a declaration** — a `/* */` clarity comment on an argument I pass explicitly |
| `ResolveTorchMesh(/*bWarnIfMissing=*/ true);` | `Torch.cpp:98` | **(i) not a declaration** — same |
| `SetLightColor(TorchLightColor, /*bSRGB=*/ true);` | `Torch.cpp:137` | **(i) not a declaration** — the engine's `SetLightColor` has the default; ⭐ **I pass it EXPLICITLY rather than relying on it**, which is the behaviour `SC-§33` wants |

✅ **ZERO defaulted parameters declared. ZERO existing functions touched, so ZERO existing call sites can have been silently under-called. A sweep that finds nothing is a RESULT and is reported as one.**

📌 **And the structural preference was taken, not just described:** `ResolveTorchMesh(bool bWarnIfMissing)` **could** have shipped as `ResolveTorchMesh(bool bWarnIfMissing = false)` — the shipped `AGoldNode::ResolveNodeMesh(bool)` it mirrors also takes it required. **A required parameter is audited by the compiler, which is precisely the property a default trades away** (`SC-§33`'s own words). The reason is written into the header comment so nobody "tidies" it into a default later.

---

## 5. What the class actually does

**`ATorch : public AActor`** — `Source/GitClaudeUnrealTest/Siegebound/Torch.{h,cpp}`, BP target **`/Game/Blueprints/BP_Torch`** (not created here — no `Content/` work in this task).

| element | shipped |
|---|---|
| `TorchMesh` | `UStaticMeshComponent`, **root**, soft-resolves `/Game/Meshes/SM_Torch` |
| `TorchLight` | `UPointLightComponent`, attached to `TorchMesh`, **Movable**, **`CastShadows = false`** |
| `TorchIntensity` | `float`, `EditDefaultsOnly`, **12.0 candelas** (derivation in §6) |
| `TorchAttenuationRadius` | `float`, `EditDefaultsOnly`, **1200 uu** (per `WR-§4`) |
| `TorchLightColor` | `FLinearColor`, `EditDefaultsOnly`, **(1.00, 0.72, 0.42, 1.0)** (per `WR-§4`) |
| `TorchLightRelativeOffset` | `FVector`, `EditDefaultsOnly`, **zero = derive from the asset** — ⚠️ **a DECLARED addition, §7** |
| `TorchMeshAsset` | `TSoftObjectPtr<UStaticMesh>`, `EditAnywhere`, `/Game/Meshes/SM_Torch.SM_Torch` |
| tick | ⛔ **off** (`bCanEverTick = false`) — no timer either; nothing about a torch changes after `BeginPlay` |
| collision | ⛔ **none** — `NoCollision` profile + `SetCollisionEnabled(NoCollision)` + no overlap events + `SetCanEverAffectNavigation(false)` |
| damage | `SetCanBeDamaged(false)`; deliberately does **not** implement `ITeamAgent` |
| team | ⛔ **none** — environment props are NEUTRAL (the M4.5 precedent). There is no `Team` member. |

**The ctor → `BeginPlay` re-apply pattern** (`ApplyTerrainMovementTuning` / `ApplyMovementSpeed` precedent, as the spec named): **`ApplyTorchLightTuning()` is the SINGLE WRITER of every light tunable** and is called from the **constructor**, from **`BeginPlay`**, and additionally from **`OnConstruction`** for the editor preview (the `AGoldNode::OnConstruction` precedent — additive, no behaviour lost).

⛔ **Mobility is set in the constructor ONLY, deliberately** — it is a registration-time property, not a runtime tunable, so it does not belong in the re-apply helper.

⛔ **`CastShadows = false` is asserted on EVERY apply, not once in the ctor.** It is a **law**, not a designer preference, so a ticked "Cast Shadows" box on `BP_Torch` is deliberately overridden. ⚠️ **QA should confirm it agrees with that reading** — the alternative (honour the BP) would let `WR-§4` be violated silently from a checkbox.

### ⚠️ THE ONE ENGINE TRAP I FOUND AND CLOSED — THE LIGHT UNITS

⛔ **`ULocalLightComponent`'s constructor leaves `IntensityUnits` at `ELightUnits::Unitless`** (`LocalLightComponent.cpp:13-16`), and **`UPointLightComponent::ComputeLightBrightness` scales unitless by ×16 and candelas by ×10000** (`PointLightComponent.cpp:128-155`).

⇒ **A candela number written into a field the engine still believes is unitless renders ~625× too dim**, and it presents as ***"the torches do not work"*** rather than as a units bug — with no log line, no warning and no error. `ApplyTorchLightTuning()` therefore calls **`SetIntensityUnits(ELightUnits::Candelas)` before `SetIntensity`**, and the reason is in the code.

---

## 6. THE ONE NUMBER I HAD TO CHOOSE — `TorchIntensity = 12.0` cd

⚠️ **`WR-§4` names `TorchIntensity` and its unit but gives no default, so a number had to be picked. Here is the arithmetic, so it is CHECKABLE rather than invented:**

1. The engine's own point-light default is **5000 UNITLESS** (`LocalLightComponent.cpp:13`).
2. `ComputeLightBrightness` scales unitless by **×16** and candelas by **×10000** ⇒ render brightness `5000 × 16 = 80000` ⇒ **the engine default is EXACTLY `80000 / 10000` = 8 cd**, at the engine's default **1000 uu** attenuation radius.
3. This torch's radius is **1200 uu** and illuminance falls off as `1/d²` ⇒ preserving the engine default's brightness **at the attenuation edge** takes `8 × (1200/1000)² = 11.52` cd ⇒ **rounded to 12**.

⇒ **"As bright as an engine-default point light, at a 1.2× bigger radius."**

⚠️ **STATED PRECISELY, BECAUSE IT IS EASY TO OVER-READ: the ARITHMETIC is derived; the FEEL IS NOT MEASURED AND IS NOT CLAIMED TO BE.** What the derivation preserves is brightness **at the edge** of the pool — closer in, this torch is 1.44× an engine-default point light, deliberately, since a fire bowl should read as a source rather than as ambient fill. ⛔ **No luminance and no frame rate has been measured. `TorchIntensity` is a FEEL TUNABLE flagged for Jonathan (TASK-571) and it costs no recompile to change.**

⛔ **NOT a `WR-§2` ledger row, and `TorchAttenuationRadius` is not one either.** Both constants are **BORN at the 9× scale**; neither was derived from the old castle's size, so **there is nothing stale in either** and **`SC-§34` produces no row for this task**. **TASK-557 owns the re-derivation ledger and this task adds nothing to it.**

---

## 7. ⚠️ ONE DECLARED DEPARTURE (`SC-§15`) — `TorchLightRelativeOffset`, A **FOURTH** `EditDefaultsOnly` FIELD

⛔ **The spec's `names:` block lists three tunables. I shipped a fourth `EditDefaultsOnly` property. Declaring it, not smuggling it — and it is declared in the header comment as well as here.**

**What forced it.** `WR-§4` requires a `TorchLight` but says nothing about *where on the torch it sits*. Following `SC-§34`'s **structural escape** ("derive at runtime from the asset instead of transcribing a number"), `PositionLightAtFireBowl()` derives the light's local position from the resolved mesh — **XY = the local bounding-box centre** (which is out into the room, because `SM_Torch`'s origin is its wall-mount face and the mesh extends away from the wall), **Z = the top of that box**. No number is invented and nothing goes stale when TASK-556 re-authors the torch.

⚠️ **THEN I FOUND A CROSS-TASK CONTRACT I HAD NOT BEEN TOLD ABOUT, AND IT DISAGREES WITH MY DERIVATION.** `Tools/ArtPipeline/build_warroom_props.py:290` — TASK-556's in-flight script — says in terms:

```
# flame bbox centre = the point ATorch::TorchLight should sit at (TASK-558/562)
```

…and computes `TORCH["flame_centre_uu"]` as a **measured** value.

**The residual, measured off the geometry in that script (`build_torch()`, lines 264-280, coordinates in uu):**

| | whole-mesh bounds | flame-only bounds |
|---|---|---|
| X | `0 … 75` (backplate face → bowl rim) | `35 … 65` |
| Y | `−22 … 22` | `−15 … 15` |
| Z | `−32 … 104` | `46 … 104` |
| **⇒ point** | **my derivation: `(37.5, 0, 104)`** | **the script's `flame_centre_uu`: `(50, 0, 75)`** |

⇒ **Δ ≈ 31.6 uu on a 1200 uu attenuation radius ≈ 2.6 %.** The light lands slightly **above and behind** the flame instead of inside it. ⚠️ **It is a polish item, not a defect:** the flame reads regardless because it is **emissive**, and this light **casts no shadows**, so nothing occludes.

**Why I did NOT just transcribe `(50, 0, 75)`:** ⛔ **it is a measurement owned by a task that has not published its handoff yet.** Reading an in-flight parallel task's working file as a source of truth is exactly the coupling the pipeline forbids — the geometry can still change, and a transcribed number would then be a stale constant that compiles and passes every test, which is the precise hazard class `SC-§34` exists for.

**So the field is a SEAM, not a knob:** default `FVector::ZeroVector` = **derive from the asset** (shipped behaviour unchanged if nobody ever sets it); any non-zero value is used verbatim and wins outright. ⇒ **When TASK-556 publishes `flame_centre_uu`, it lands as a `BP_Torch` default edit at TASK-569 — no recompile, no code change, no C++ constant.**

⚠️ **This also fixes a design flaw I would otherwise have shipped:** because `PositionLightAtFireBowl()` writes the light's relative location at `BeginPlay`, a component transform authored by hand in `BP_Torch` would have been **silently overwritten**. The override field is what makes "just set it in the Blueprint" actually work.

⭐ **NAMED FORWARD TO TASK-569 / TASK-562:** set `BP_Torch → TorchLightRelativeOffset` from TASK-556's published `flame_centre_uu`. ⛔ **Do not edit `Torch.cpp` for it.**

---

## 8. Null-safety — every path, since a missing asset is the EXPECTED state right now

⛔ **`SM_Torch` DOES NOT EXIST YET** (TASK-556 authors it, TASK-566 imports it). This class resolves it by path and **survives its absence**, exactly as dispatched.

| path | behaviour |
|---|---|
| `TorchMeshAsset.IsNull()` (cleared in a child/instance) | **silent no-op** — the supported designer opt-out (the `AttackImpactEffect` pattern, TASK-020) |
| `LoadSynchronous()` returns null at `BeginPlay` | ⛔ **ONE `UE_LOG(Warning)`, guarded by `bWarnedMissingMesh`** — once per actor, never per frame, never per construction. The actor stays **inert and invisible but still lights the room**. ⛔ **No crash, no `check`, no `ensure`.** |
| `LoadSynchronous()` returns null in `OnConstruction` | **silent** — `OnConstruction` re-runs on every editor property tweak; warning there would spam |
| `TorchMesh` / `TorchLight` null | early-return guards at the top of `ApplyTorchLightTuning()`, `ResolveTorchMesh()` and `PositionLightAtFireBowl()` |
| mesh resolved but bounding box degenerate | `FBox::IsValid` checked — the light is left where it is rather than moved somewhere meaningless |
| no mesh at all | light stays at the component default `(0,0,0)` = the wall-mount point, **which still lights the room because shadows are off**. Silent — the missing asset was already reported once, and a second line for the same cause is noise. |

---

## 9. ⚠️ WHAT QA SHOULD SCRUTINISE (ranked — the top three are the ones I would attack)

1. ⚠️⚠️ **THE `SetIntensityUnits` CLAIM — VERIFY IT, DO NOT TAKE MY WORD.** The whole "12 cd" derivation and the ×625 trap both rest on `LocalLightComponent.cpp:13-16` (`IntensityUnits = ELightUnits::Unitless`) and `PointLightComponent.cpp:128-155` (unitless ×16 vs candelas ×10000). **If I read either wrong, the torches ship at the wrong brightness and nothing catches it — the code compiles and no test can see it.** Both files are at `C:\Program Files\Epic Games\UE_5.8\Engine\Source\Runtime\Engine\Private\Components\`.
2. ⚠️⚠️ **THE DECLARED DEPARTURE IN §7 — `TorchLightRelativeOffset`.** It is a fourth `EditDefaultsOnly` field beyond the `names:` block. **I believe it is right and I have given the full reasoning; it is a judgement call and QA is entitled to overrule it.** ⛔ The fallback if overruled is *not* "hardcode `(50, 0, 75)`" — it is "delete the field and accept the ~2.6 % offset", because transcribing an unpublished number from an in-flight task is the worse option.
3. ⚠️ **`CastShadows = false` RE-ASSERTED EVERY APPLY, overriding a `BP_Torch` checkbox.** Deliberate — `WR-§4` is a law, not a preference. **If QA reads the law as "set it once and let designers own it", say so and it is a one-line change.**
4. **Constructor-time calls to component setters.** `ApplyTorchLightTuning()` runs in the ctor, where the component is unregistered. I verified each setter is safe there: `AreDynamicDataChangesAllowed` returns true when `IsRegistered()` is false (`SceneComponent.h:1363`); `MarkRenderStateDirty` is guarded on `IsRegistered() && bRenderStateCreated`; `UpdateColorAndBrightness` early-outs on a null `GetWorld()`/`World->Scene`; `PushRadiusToRenderThread` early-outs on a null `SceneProxy`.
5. **`SetupAttachment(TorchMesh)` with a `TObjectPtr<UStaticMeshComponent>`** — two-step conversion. Shipped precedent: `Castle.cpp:73` (`HPBarWidget->SetupAttachment(CastleMesh)`) and `GoldNode.cpp:34` (`SetRootComponent(NodeMesh)`), both identical shapes that compile today.
6. **Deprecated-API check.** Every engine call used was read in the UE 5.8 headers and **none carries `DeprecatedFunction`**: `SetMobility` · `SetIntensityUnits` · `SetIntensity` · `SetAttenuationRadius` · `SetLightColor` · `SetCastShadows` · `SetStaticMesh` · `SetCollisionProfileName` · `SetCollisionEnabled` · `SetGenerateOverlapEvents` · `SetCanEverAffectNavigation` · `SetCanBeDamaged` · `UStaticMesh::GetBoundingBox` · `SetRelativeLocation`. ⚠️ **`ULightComponentBase::SetCastRaytracedShadow` IS deprecated in 5.8 — it is not used here.**
7. **Ownership discipline.** Files 554/557/559/560/561 were editing concurrently. ⛔ **I opened `Castle.{h,cpp}`, `HeroCharacter.cpp`, `GoldNode.{h,cpp}`, `AncientGround.{h,cpp}`, `SiegeNetLimits.h` and `build_warroom_props.py` READ-ONLY, for precedent and for the §7 finding, and edited none of them.**

---

## 10. Cross-task notes

- **TASK-562** is `ATorch`'s only caller: it spawns from `TorchAnchors`, attaches to `CastleMesh`, caps at `MaxTorchesPerCastle`, and owns the destroy/`ResetCastle` lifecycle. ⛔ **This class holds no lifecycle opinion and no anchor data** — it is a prop.
- **TASK-556** authors `SM_Torch` (slot 0 `M_Torch`, slot 1 emissive `M_TorchFlame`); **TASK-566** imports it to `/Game/Meshes/SM_Torch`. ⚠️ **See §7 — TASK-556's `flame_centre_uu` has a named landing site.**
- **TASK-569** owns `BP_Torch` and any `Content/` work. ⛔ **None was done here.**
- **`NS_TorchFlame` (Niagara) is RECORDED AS A FOLLOW-UP and was not started** (`WR-§4` flame clause). This file references no Niagara type — verified by the §3 grep.
- ⛔ **`WR-§3` (navmesh) is untouched by this task:** the torch carries no collision and no navigation relevance, so it cannot affect the traversability guarantee.

---

## 11. Slack

Posted in **⚙️ Dev & QA** (`C0BF0QZP3CN`, `thread_ts 1783116269.740549`), prefixed `⚙️ PROGRAMMER: ✅ TASK-558`. ⛔ **No blocker raised — nothing in 🚨 Blockers for this task.**
