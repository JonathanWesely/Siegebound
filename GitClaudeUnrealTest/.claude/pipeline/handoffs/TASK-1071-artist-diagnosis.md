# TASK-1071 — DIAGNOSIS: why the Fog card draws nothing

**Agent:** art-director · **Date:** 2026-09-05 · **Status:** diagnosis only, ⛔ nothing fixed
**Editor:** opened by me under the standing grant (PID 29128), MCP `127.0.0.1:8000` live, **left UP**

---

## 0. THE ANSWER IN ONE LINE

**`AFogVolume::SpawnFogVisual()` asks for scale `(640, 360, 260)` and the engine silently gives it
`(20, 20, 5)` — the vendor component template's own scale.** His fog box is a **2,000 × 2,000 × 500 uu
slab floating 6,750 uu above the battlefield**, not a 64,000 × 36,000 × 26,000 uu volume around it.
It is outside the volumetric-fog froxel grid in every direction, so it renders nothing, for anyone,
always. **The log prints the REQUESTED scale, which is why the log looks perfect.**

⛔ **It is not the wiring, not `StartDistance`, not the camera, not the material, not scalability.**

---

## 1. THE SINGLE MEASURED DIFFERENCE

| | renders | does NOT render |
|---|---|---|
| what | `BP_SiegeFog` at root scale **(640, 360, 260)** | `BP_SiegeFog` at root scale **(20, 20, 5)** |
| box | 64,000 × 36,000 × 26,000 uu, Z −6,000 → +20,000 | 2,000 × 2,000 × 500 uu, Z **6,750 → 7,250** |
| mean luma (linear) | **0.449952 – 0.452790** | **0.261480 – 0.261546** |
| vs zero-control | **+73.2 %** | **+0.04 %** |
| detail std | 0.0727 – 0.0866 (collapsed) | 0.1198 – 0.1202 (**intact**) |
| PNG bytes | 1.47 – 1.72 MB | 4.398 – 4.405 MB |

**Zero-control (no fog actor in the level at all): mean luma `0.261408`, std `0.120017`, 4,403,611 B.**

⇒ The "does not render" state is **`+0.000096` mean luma over the control — +0.037 % relative**, inside
the ±0.05 % pixel noise floor TASK-841 measured. **It is not "faint fog". It is indistinguishable
from no fog whatsoever**, and the frame is pixel-for-pixel what 🧑 Jonathan describes: crisp grass,
legible enemy gate. Everything else — camera, material, level, lighting, CVars, the actor itself —
was held identical between those two rows. **The only variable was the root scale.**

---

## 2. THE MECHANISM, READ FROM ENGINE SOURCE (not inferred)

`BP_SiegeFog`'s root component is **`Mesh`** — read back live:
`/Game/Maps/L_Arena.L_Arena:PersistentLevel.BP_SiegeFog_C_0.RootComponent → …:BP_SiegeFog_C_0.Mesh`.
It is an **SCS** component inherited from the vendor `BP_FogArea`, **not a native root**. So
`AActor::PostSpawnInitialize`'s `FixupNativeActorComponents()` returns `nullptr` and the root
transform is applied by the SCS path instead:

**`Engine/Source/Runtime/Engine/Private/SCS_Node.cpp:129-147`**

```cpp
FTransform WorldTransform = *RootTransform;
switch(TransformScaleMethod)
{
case ESpawnActorScaleMethod::OverrideRootScale:
case ESpawnActorScaleMethod::SelectDefaultAtRuntime:
    break;                                                     // keeps the spawn scale
case ESpawnActorScaleMethod::MultiplyWithRoot:
    WorldTransform = NewSceneComp->GetRelativeTransform() * WorldTransform;
    break;
}

if(bIsDefaultTransform)
{
    // Note: We use the scale vector from the component template when spawning (to match what
    // happens with a native root). This does NOT occur when this component is instanced as part
    // of dynamically spawning a Blueprint class in a cooked build (i.e. 'bIsDefaultTransform'
    // will be 'false' in that situation).
    WorldTransform.SetScale3D(NewSceneComp->GetRelativeScale3D());   // ⛔ THE CLOBBER
}
```

and **`Actor.cpp:4358-4360`** — the non-deferred spawn path passes `true`:

```cpp
if (!bDeferConstruction)
{
    FinishSpawning(UserSpawnTransform, true);   // ⛔ bIsDefaultTransform = true
}
```

`AFogVolume::SpawnFogVisual` (`FogVolume.cpp:586`) uses exactly that non-deferred path:
`World->SpawnActor<AActor>(VisualClass, SpawnTransform, SpawnParams)`.

⇒ `WorldTransform.SetScale3D(Mesh_GEN_VARIABLE->GetRelativeScale3D())`.

**Measured live from the vendor template** —
`/Game/FogArea/Blueprints/BP_FogArea.BP_FogArea_C:Mesh_GEN_VARIABLE` reads
**`RelativeScale3D = (20, 20, 5)`**, `RelativeLocation = (0,0,0)`.

⚠️⚠️ **THE TRAP FOR WHOEVER FIXES THIS: `SpawnParams.TransformScaleMethod` DOES NOT HELP.** The
`bIsDefaultTransform` block runs **after** the switch and overwrites the scale **regardless** of which
`ESpawnActorScaleMethod` was chosen. Setting `OverrideRootScale` is the obvious reach and it changes
nothing. (`FActorSpawnParameters::TransformScaleMethod` already defaults to `MultiplyWithRoot`.)

### 2a. Why the box is invisible, arithmetically

`/Engine/BasicShapes/Cube` is 100 uu, so `(20,20,5)` = **2,000 × 2,000 × 500 uu**, centred on
`(0, 0, 7000)` ⇒ spans **X ±1,000 · Y ±1,000 · Z 6,750 → 7,250**.

His hero sat at `(-21194.15, -43.63, 98.15)` (his own log). So the slab was:
- **~21,200 uu away laterally**, and
- **~6,550 uu above the camera**,

against `ExponentialHeightFog_0.VolumetricFogDistance = 6000` — the froxel grid only exists within
6,000 uu of the camera. **The volume never intersects the grid at any camera angle from the ground,
even looking straight up.** Two independent reasons it contributes exactly zero.

Intended footprint 64,000 × 36,000; achieved 2,000 × 2,000 = **0.17 % of the plan area, 0.0033 % of
the volume.**

---

## 3. THE THREE LEADS — ALL THREE KILLED, AND HOW

### ① `StartDistance = 10000` gating the volumetric path — **DEAD, twice over**

- **Engine source.** The froxel grid's Z range is
  `GetVolumetricFogGridZParams(FogInfo.VolumetricFogStartDistance, NearClip, FogInfo.VolumetricFogDistance, …)`
  (`VolumetricFog.cpp:1426, 1460, 1604`) — it reads **`VolumetricFogStartDistance`**, a *separate*
  property, **never `StartDistance`**. `StartDistance` is consumed only in
  `HeightFogCommon.ush:286-288` under `PERMUTATION_SUPPORT_FOG_START_DISTANCE`, i.e. the **analytic
  (non-volumetric) height-fog** path. The `FogInstance.StartDistance` in `VolumetricFog.usf:212`
  belongs to **Local Fog Volumes** (`USE_LOCAL_FOG_VOLUMES`) — a different feature; `L_Arena`
  contains **zero** of them (name-table count 0).
- **Property read, fresh from disk.** `ExponentialHeightFog_0.HeightFogComponent0` reads
  **`VolumetricFogStartDistance = 0`**, `VolumetricFogDistance = 6000`, `StartDistance = 10000`,
  `bEnableVolumetricFog = true`, `FogDensity = 0.012`, `Mobility Movable`, `bVisible true`.
  ⇒ The window is `[near clip → 6000]`, **not** empty. My TASK-841 sentence *"the pack can only
  render within 6,000 uu of the camera"* was right; the *"begins at 10,000"* fear was wrong.

### ② The `SphereMask(CameraPositionWS, …)` in `MF_Fog` — **DEAD, and my own note was wrong**

I opened `MF_Fog` (read-only). ⚠️ **Correcting the record: the node is NOT disconnected.**
`MaterialExpressionSphereMask_0` reads
`A = MaterialExpressionCameraPositionWS_0`, `B = MaterialExpressionWorldPosition_7 (XYZ)`,
`Radius = MaterialExpressionScalarParameter_22`, `Hardness` unwired (default).
TASK-841 §4's claim that it *"was NOT reconnected"* was **hearsay I carried forward without opening
the asset** — it is repaired here.

But it **cannot be the cause**: the "renders" and "does not render" rows in §1 use the **same
material, same camera, same frame**. A material expression that is identical in both states cannot be
the difference between them. Killed by the experiment, not by the graph.

### ③ Editor viewport vs the gameplay camera — **DEAD, measured not assumed**

- The boom is **`TargetArmLength = 400`**, `bUsePawnControlRotation = true`, **no `SocketOffset`, no
  `TargetOffset`** (`GitClaudeUnrealTestCharacter.cpp:39-47`; `HeroCharacter` only pushes `ProbeSize`
  onto the inherited boom). ⇒ his camera sits ~390 uu behind the hero at roughly **Z ≈ 200** — within
  ~150 uu of TASK-841's E1 vantage.
- **Every capture in §1 was taken from his own gameplay vantage**, derived from his log's hero
  position: `(-21580, -44, 201)`, pitch −15, yaw 0. At full scale it white-outs there. So the
  gameplay camera renders fog fine.
- **Arena position eliminated too:** his hero is **10,806 uu** inside the box's X edge — *further*
  in than TASK-841's E3 corner probe (8,000 uu), which was a total white-out after the §5.2 fix.

### ④ Three more I killed on the way (none was the cause)

- **Scalability.** His own log line 841: `r.VolumetricFog:1`, `GridPixelSize:8`, `GridSizeZ:128`,
  from `[ShadowQuality@3]` — Epic, and never re-applied later in the session. (`r.VolumetricFog`
  lives under **`sg.ShadowQuality`**, not `EffectsQuality`; his `GameUserSettings.ini` is all `@3`.)
- **The level flag not being on disk.** `TASK-1036` genuinely saved and committed it (`12b8707`);
  a fresh-from-disk load reads `bEnableVolumetricFog = true`, and there is exactly **one**
  `ExponentialHeightFog` in `L_Arena`, so `Scene->ExponentialFogs[0]` is it
  (`VolumetricFog.cpp:1355-1364`).
- **Runtime-only Blueprint logic (`BeginPlay`/`Tick`).** The vendor BP does implement
  `ReceiveBeginPlay`/`ReceiveTick`, and those **never run in an editor viewport but always run in his
  game** — the right shape, so I tested it: **Simulate-In-Editor, world ticking, mean luma `0.434`.**
  Fog still there. Also full **PIE**, converged burst: `0.365` with detail std collapsed to `0.063` —
  near-total white-out. Killed.
- **`bHiddenInGame`** (the classic editor-yes/game-no trap): absent from both `BP_FogArea.uasset`
  and `BP_SiegeFog.uasset` name tables. **Material usage flags** (`"Default Material will be used in
  game"` — a failure mode his log *does* show, for the tree materials): **no such warning for
  `M_FogArea`** anywhere in his session.

---

## 4. WHY HIS LOG LOOKED PERFECT — the instrument lie

```
Fog VISUAL spawned: 'BP_SiegeFog_C_0' at Z=7000, scale (640, 360, 260)
```

`FogVolume.cpp` logs `SpawnTransform`, **the value it asked for**. It never reads
`Spawned->GetActorScale3D()` — **the value it got**. The actor really did spawn, really was held for
the right duration, and really was destroyed on time; every one of those claims in the log is true.
The one number that mattered was never measured. ⭐ Another instance of the standing law: **a success
return is not evidence.**

---

## 5. THE FIX — property/connection to change (⛔ NOT APPLIED)

**Owner: `gameplay-programmer`. One C++ file, `Source/GitClaudeUnrealTest/Siegebound/FogVolume.cpp`,
in `AFogVolume::SpawnFogVisual()`. No art change, and ⛔ NO VENDOR EDIT.**

**Preferred — force the achieved scale explicitly after the spawn:**

```cpp
AActor* const Spawned = World->SpawnActor<AActor>(VisualClass, SpawnTransform, SpawnParams);
// ... existing null check ...
Spawned->SetActorScale3D(SpawnTransform.GetScale3D());   // SCS_Node.cpp:141 clobbers spawn scale
                                                         // with the vendor template's (20,20,5)
```

**Alternative — the deferred path**, whose `FinishSpawning` defaults `bIsDefaultTransform = false`
(`Actor.h:3117`), so the clobber never runs:
`SpawnActorDeferred<AActor>(...)` then `Spawned->FinishSpawning(SpawnTransform)`.

⛔ **Do NOT "fix" it with `SpawnParams.TransformScaleMethod`** — see the §2 trap.

**And add the instrument that would have caught this in one playtest:** log the **achieved**
`Spawned->GetActorScale3D()` / `GetActorBounds()`, not the requested transform — or assert they match.

⚠️ **Rider the fixer must know:** the engine comment quoted in §2 states `bIsDefaultTransform` is
**`false` in a cooked build**, so a packaged build may honour the scale where his uncooked standalone
did not. **I did not test a cooked build.** That divergence is itself an argument for the explicit
`SetActorScale3D` — it makes cooked and uncooked behave identically.

**Does the fix touch vendor content?** ⛔ **No.** `Content/FogArea/**` needs no edit. (The vendor
template's `(20,20,5)` is the *donor* value, not a defect — the pack is fine; our spawn call is what
must stop relying on the spawn transform's scale surviving.)

---

## 6. HOW I MEASURED (`SC-§88`, `SC-§79`, control)

- **Zero-control first**, before any delta was read: the level with **no fog actor at all**,
  captured at the same vantage ⇒ `0.261408`. Reproduced at the end by the (20,20,5) rows, which land
  on it to 4 decimal places.
- **Bursts, never single shots.** 4 frames at full scale (0.4500 / **0.4528** / 0.4472 / 0.4423),
  3 in Simulate (0.4345 / 0.4343 / 0.4338), 4 in PIE (0.3702 unconverged → **0.3643** converged,
  std 0.1231 → 0.0634), 3 at runtime scale (0.2615 / 0.2615 / 0.2615). Best-converged frame quoted.
- **Area-averaged linearised-sRGB luma over the whole 2764 × 828 frame**, plus detail std and PNG
  bytes as corroboration — `SC-§85`/the ninth lie: the byte metric is reported, never relied on.
- Every capture's returned `cameraLocation` was checked to equal the requested pose.
- ⚠️ **Declared:** all captures are **editor-viewport** renders (the axis gizmo is visible in each),
  including during PIE. I never observed his standalone process's own framebuffer.
- ⚠️ **The one link I did NOT observe directly:** the achieved scale of a *runtime-spawned*
  `BP_SiegeFog`. `AFogVolume` exposes **no `UFUNCTION`** and `FogActiveUntilTimeSeconds` is
  `VisibleInstanceOnly, Transient`, so I could not drive the production spawn from outside. The
  mechanism is read from engine source, the substituted value is measured live from the vendor
  template, and the visual consequence of that exact value is reproduced against a control — but the
  substitution itself is inferred, not watched. The §5 instrument closes that gap permanently.
- 🧑 **His context, established from his own log:** `LogInit: Game Engine Initialized` +
  `UGameEngine::Tick.ViewportClosed` + window title `Siegebound (64-bit Development PCD3D_SM6)` ⇒
  **a standalone game run, not PIE and not the editor viewport.** (Log timestamps are UTC:
  his local 17:53 = `2026.09.06-00.53`.)

---

## 7. FENCES

✅ `L_Arena` **never saved** — `sha256` verified **before and after**, still
`1f78419d888940739c949a60b29ee43451c464915f7ab37942b921fe0af15622`. No save prompt was accepted.
✅ `Content/FogArea/**` **byte-unchanged** — `git status --porcelain --untracked-files=all` on it is
**empty**, re-checked *after* loading `BP_FogArea` and `MF_Fog`. **Nothing went `M`.** Read-only
tools only (`get_expressions`, `get_expression_inputs`); ⛔ no `recompile`, no connect/disconnect.
✅ `BP_SiegeFog` **not modified** — only spawned as a level actor and removed.
✅ ⛔ **No Git operation** (only read-only `status` / `log` / `sha256sum`). Nothing staged.
✅ ⛔ No `Source/` file touched. No compile. No board row created.
⚠️ I disabled editor **autosave** in memory (`bAutoSaveEnable` true → false, read back `false`)
before loading the vendor BP, exactly as TASK-841 did. It is an `EditorPerProjectUserSettings`
in-memory preference and **reverts on editor restart**. No `_Auto` package was created under
`Content/`.
⚠️ **`L_Arena` is dirty in memory** — I spawned and removed two transient probe actors
(`BP_SiegeFog_C_0`, `BP_SiegeFog_C_1`; both removed, verified). **On-disk package untouched.**
⛔ **Decline any save prompt.** Editor left **UP**, PID 29128.

Working PNGs (session scratch, not committed):
`…\scratchpad\fog2\` — `CTRL_A_nofog.png`, `EDITOR_fog_r1..r4.png`, `SIM_fog_r1..r3.png`,
`PIE_gameview_r1..r4.png`, `RUNTIMESCALE_r1..r3.png`.
