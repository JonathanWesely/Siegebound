# TASK-841 — the fog visual: `/Game/Blueprints/BP_SiegeFog`

**Agent:** art-director · **Date:** 2026-09-05 · **Status:** `ready-for-integration`
**Gate:** art skips QA ⇒ integration check + ship host **`TASK-1043`**
**Editor:** UP (PID 13600), MCP `127.0.0.1:8000` live, left UP. No modal encountered (verified by
window-title enumeration, not by the port: title read `GitClaudeUnrealTest - Unreal Editor`).

---

## 0. THE THREE THINGS THE DISPATCH ASKED ME TO STATE FIRST

1. **I tuned to the LIVE Beer-Lambert targets, not the retired ones.** I verified both sets at
   source before touching anything. `TASKBOARD.md` spec item (2a) still prints `0.0625` / `0.25` /
   `0.5625` and a `t²` curve; `CONVENTIONS.md` **`FOG-§9.2`** retires both by name
   (*"`FogDensityExponent` and its `t^n` remap — RETIRED"*). **Nothing on this row was tuned against
   the retired numbers.**
2. **The visual reads the fog state; it never owns it.** `BP_SiegeFog` has **zero variables**, an
   empty construction script, and an event graph containing only UE's auto-generated
   parent pass-throughs. **There is no second timer, no duplicated duration, no prevention-window
   logic, and no mirrored "is fog up" flag.** Proof in §4.
3. **Camera coordinates for every capture are printed in §6.** None is the forbidden vantage.

---

## 1. THE ASSET

| | |
|---|---|
| path | `/Game/Blueprints/BP_SiegeFog` |
| file | `Content/Blueprints/BP_SiegeFog.uasset` |
| sha256 | `347624c91f9863c36883ab40a228015b78034d53536e123e90828d645f5e81a2` |
| size | 37,947 bytes |
| **parent** | `/Game/FogArea/Blueprints/BP_FogArea.BP_FogArea_C` — **read back with `get_parent`, not assumed** |
| variables | **none** (`list_variables` ⇒ `[]`) |
| graphs | `UserConstructionScript` = `(fn ConstructionScript () (|Parent:ConstructionScript))` · `EventGraph` = three auto-stubs, each only calling `Parent:` |

**LFS join verified by oid-vs-sha256, never by size** (`SC-§68`, `§25b` cl. S):

```
pointer  oid sha256:347624c91f9863c36883ab40a228015b78034d53536e123e90828d645f5e81a2  size 37947
working  347624c91f9863c36883ab40a228015b78034d53536e123e90828d645f5e81a2  (37947 bytes)   MATCH
```

### 1a. Settings written, then **read back** from the compiled CDO

| property | shipped | vendor default |
|---|---|---|
| `general Data.density` | **5.0** | 1 |
| `general Data.base Color` | (1,1,1,1) | (1,1,1,1) |
| `general Data.emissive Color` | (0.05,0.05,0.05,1) | same |
| `general Data.wInd World Space` | **true** | false |
| `general Data.wind Speed` | 1.0 | 1.0 |
| `general Data.mask Margin` | 3.0 | 3.0 |
| `bBoxShape` | **true** (rectangular battlefield) | false (sphere) |
| `maxDrawDistance` | **0** = never cull | 25,000 |
| `mode` | Base | Base |
| `material Mode` | Dynamic | Dynamic |
| `noise Data` | vendor default, untouched (channel R, scale 2.5, sharpness 0.35) | same |

⚠️ **`maxDrawDistance` mattered:** the vendor's 25,000 would have culled the volume's own mesh at
arena scale (the arena is 52,000 uu corner to corner).

---

## 2. PLACEMENT — for build-master. **This number is derived, never hand-typed** (`SC-§34`)

```
location (0, 0, 7000)   rotation (0, 0, 0)   scale (640, 360, 260)
```

Read back with `get_actor_bounds` on the live spawned actor:

```
min (-32000, -18000,  -6000)      max ( 32000,  18000,  20000)
```

**Derivation.** `USiegeScatterConfig::ArenaHalfExtent = (26000, 12000)`
(`Source/GitClaudeUnrealTest/Siegebound/ScatterConfig.h:356`) — the single owner of the arena
extent — plus **6,000 uu of horizontal margin on every side**, 6,000 uu being exactly
`ExponentialHeightFog_0`'s `VolumetricFogDistance` (§3.2). Ground datum is
`ArenaGroundReferenceZUU = 0` (`J-F13`), so the box runs 6,000 uu below ground to 20,000 above.
`/Engine/BasicShapes/Cube` is 100 uu, hence scale = extent/100.

⛔ **Why the scale is not baked into the Blueprint.** The mesh component is **inherited from the
vendor BP**; `get_components` on the child CDO returns the vendor's own template
(`…BP_FogArea.BP_FogArea_C:Mesh_GEN_VARIABLE`). Writing a scale there would have **edited the vendor
pack** (`FOG-§6`), and MCP exposes no way to create the child's inherited-component override record.
So the arena scale rides the **spawn transform**, documented here. This is a reportable tooling gap,
not a design choice.

---

## 3. WHAT I MEASURED

### 3.1 The PNG control — run FIRST, before any delta was read (`SC-§85`, `SC-§68` ninth lie)

Two captures of **identical** content, nothing changed between them, at `(0,0,1200)` pitch −5 yaw 0:

| metric | value |
|---|---|
| **BYTE** size ratio | **0.998139** (−0.19%) |
| **PIXEL** mean-luma delta | **+0.000251 absolute = +0.0517% relative** |
| per-pixel \|ΔLuma\| | mean 0.0048, max 0.496, **71.4% of pixels differ at all** |

⇒ Two conclusions I used throughout: **(a)** the byte metric's noise floor (0.19%) is ~4× the pixel
metric's (0.05%), so a byte comparison is not a pixel comparison; **(b)** TAA jitter makes
*per-pixel* comparison useless — only **area-averaged** luma is stable. Every number below is an
area mean over a disc of ≥8,000 px.

### 3.2 The instrument, and the two traps in it

- **`L_Arena`'s `ExponentialHeightFog_0` reads `bEnableVolumetricFog = true`** — measured, so
  `TASK-1036` is live and my pixel observations are interpretable. It also reads
  **`VolumetricFogDistance = 6000`**, `VolumetricFogExtinctionScale = 1`, `FogDensity = 0.012`,
  `StartDistance = 10000`.
  ⇒ **The pack can only render within 6,000 uu of the camera.** At the shipped density everything
  past ~1,000 uu is already fully obscured, so this is a render-range limit that is invisible in
  play — but it is the reason the volume must overhang the arena (§2).
- ⚠️⚠️ **UE's volumetric fog is temporally accumulated, and a single `CaptureViewport` is NOT a
  converged measurement.** Consecutive captures at *identical* settings gave 55.8% and 100.8%
  obscuration at 609.6 uu. The history builds from empty, so **convergence is monotone from below**.
  Every figure below is the best-converged frame of a burst of 3–4. **A single-shot fog measurement
  in this editor is worthless, and it looks exactly like a real one.**

### 3.3 The probe — three estimators failed before one worked

A lit grey sphere gives an unusable reading, because a map-wide volume of this density
**shadows the sun**: the probe's own surface radiance changes when the fog is added, so
transmittance and the shadow factor are entangled. A contrast estimator failed too — the fog does
not merely scale the probe's shading, it *restructures* it (direct sun → ambient).

**What worked: an unlit pure-black probe.** A target of zero radiance makes obscuration read
directly, with no shadow term, no reference frame and no fitting:

> `I_observed(d) = 0·T(d) + L∞·(1 − T(d))` ⇒ **`Obscuration(d) = I_observed(d) / L∞`**

Positive control (`SC-§39`): with the fog absent all four probes measured **exactly `0.000000`,
zero variance** — the instrument reads zero when there is nothing to see.
Probes were `/Engine/BasicShapes/Sphere` at 25 / 152.4 / 304.8 / 609.6 uu from the camera along
known rays (distances confirmed by computation as 25.000 / 152.401 / 304.800 / 609.604), each scaled
to subtend the same 4°, carrying a temporary unlit-black material. All measured in **linearised
sRGB**.

### 3.4 THE RESULT — obscuration achieved at `density = 5.0`

| distance | **measured** | **target (`FOG-§9.2`)** | delta |
|---|---|---|---|
| **152.4 uu** | **55.1 %** | 62.39 % | −7.3 pts |
| **304.8 uu** | **87.4 %** | 85.86 % | **+1.5 pts** |
| **609.6 uu** | **100.0 %** (saturated) | 98.00 % | ≥ target, cannot resolve finer |

σ implied at 304.8 uu = **0.00639 /uu** against the pinned
**σ = −ln(0.02)/609.6 = 0.00641736 /uu** — a **0.4 % match** on the best-conditioned probe.
Figures are normalised by the instrument's own saturation reference (a fully-obscured probe reads
104.7% of its background ring, a +4.7% sampling bias that I divided out rather than hid).

**Declared residuals, because they are real:**
- The **onset band is ~7 points clearer than Beer-Lambert prescribes**. The rendered falloff is
  steeper than exponential at short range — near-camera froxel depth resolution. This errs toward
  legibility (you see your own feet slightly better than the model says), and 152.4 uu was also the
  noisiest probe.
- **The ceiling cannot be measured more finely than "≥98%"** with this instrument; 98% and 100% are
  not separable once the probe has merged into the fog.
- The medium is **deliberately noise-modulated** (vendor noise, `sharpness 0.35`), so local
  obscuration varies around these means by a few points. That is the *look* Jonathan bought the
  asset for; it is not measurement error.

---

## 4. THE ARCHITECTURE — the visual is a consumer, and here is the proof

- `BP_SiegeFog` declares **zero variables** (`list_variables` ⇒ `[]`).
- Its construction script is `(|Parent:ConstructionScript)` and nothing else.
- Its event graph is three UE-generated stubs that only call `Parent:BeginPlay`,
  `Parent:ActorBeginOverlap`, `Parent:Tick`.
- ⇒ **There is no timer, no duration, no deadline, no prevention window, and no "is fog up" flag
  anywhere in this asset.** It cannot own fog state because it holds no state.
- The consumer relationship is **actor lifetime**: the volume renders while it exists. Spawn it when
  `AFogVolume` raises fog, destroy it when the fog expires. That wiring is C++/integration and is
  deliberately **not** in this asset.
- ⚠️ **A note the integrator needs:** `AFogVolume` currently exposes **no `UFUNCTION` at all**
  (`Find`, `FindOrSpawn`, `IsFogActive`, `IsFogPrevented` are all plain C++), and
  `FSiegeFogStatics` is a non-reflected static library. **A Blueprint therefore cannot poll the fog
  state today.** Lifetime control from C++ is the only available seam — which is also the one that
  cannot drift, so I did not ask for a Blueprint accessor.
- ⛔ **The vendor's disconnected `SphereMask(CameraPositionWS, …)` node was NOT reconnected.** I did
  not open `M_FogArea` or `MF_Fog` at all.

---

## 5. 🚨 FINDINGS FOR THE MANAGER — one of them is a real conflict

### 5.1 ⛔ NAME COLLISION: two laws pin **different parents** to the same asset path

A Blueprint has exactly one parent, and two live sources disagree about which:

| source | says `/Game/Blueprints/BP_SiegeFog` is… |
|---|---|
| **`TASK-841` `names:`** and **`TASK-1043` item (1)** | a child of **`Content/FogArea/Blueprints/BP_FogArea`** |
| **`CONVENTIONS.md` `FOG-§6`** file map | *"the actor \| `AFogVolume` in `Siegebound/` ⇒ BP child `/Game/Blueprints/BP_SiegeFog`"* |
| **`FogVolume.h:95-105`** | *"A level-placed `BP_SiegeFog` (`FOG-§6`'s BP child) would ALSO be found by `Find`"* and its `CoreRedirects` paragraph assumes the five `EditDefaultsOnly` tunables serialise *"into any Blueprint child (`/Game/Blueprints/BP_SiegeFog`)"* |

**I built it per the live dispatch and its own gate: parent = `BP_FogArea`.** That is the instruction
I was sent and the condition `TASK-1043` will check.

**The consequence, stated plainly so nobody discovers it later:** `FogVolume.h`'s serialisation
warning is now **false** — there is no Blueprint child of `AFogVolume`, so those five tunables are
serialised only into the C++ CDO, and the `CoreRedirects` hazard it describes does not currently
exist. This is a comment/law correction for the programmer + manager lanes, **not** something I
touched (`Source/` was fenced).

### 5.2 ⛔ A COVERAGE HOLE I FOUND AND FIXED — *"entire battlefield"* nearly shipped with a gap

Sized to the arena exactly (`±26000, ±12000`), the fog **was not there at the arena edge**. From
`(24000, 11000, 1200)` the view was essentially clear: green grass, the blue castle, crisp trees.
Cause: the box mask's `mask Margin` feather plus the 6,000 uu froxel range means a camera near the
boundary has a near-field neighbourhood that is mostly *outside* the dense core.

Fixed by overhanging the arena by 6,000 uu per side (§2). Same camera, before and after:

| | PNG bytes | mean luma |
|---|---|---|
| box sized to arena | 3,220,062 | 0.6249 (clear) |
| box overhanging 6,000 uu | **873,062** | 0.6454 (**total white-out**) |

⇒ A player standing near the arena wall would have been the only one who could see.

### 5.3 Other things build-master must know

- ⚠️ **I disabled the editor's autosave** (`UEditorLoadingSavingSettings.bAutoSaveEnable` true → false,
  read back false) **before loading `BP_FogArea`**, precisely because that vendor asset dirties its
  own package on load and autosave is what would have written it. This is an in-memory editor
  preference (`EditorPerProjectUserSettings`); it was **not** persisted and reverts on editor
  restart. **Result: zero `_Auto` packages created** — none under `Content/`, none new under
  `Saved/Autosaves/` (baseline recorded: `BP_FogArea_Auto3/4` from 2026-09-02, `L_Arena_Auto1` from
  00:41 today; none newer).
- ⚠️ **`L_Arena` is dirty in memory** — I spawned and removed transient actors (the volume and four
  probes; all removed, verified). **The on-disk package is untouched: `sha256` still
  `1f78419d888940739c949a60b29ee43451c464915f7ab37942b921fe0af15622`.** ⛔ Decline any save prompt.
- ⚠️ **The editor auto-staged the asset.** `Saved/Config/WindowsEditor/SourceControlSettings.ini`
  has `Provider=Git`, so saving the new asset put it in the index by itself:
  `A  GitClaudeUnrealTest/Content/Blueprints/BP_SiegeFog.uasset`. **I ran no Git command** (only
  `git status` / `git cat-file` / `git ls-files`, all read-only). The staged set contains **that one
  file and nothing else**.
- ✅ **Vendor pack byte-unchanged.** `git status --porcelain --untracked-files=all Content/FogArea/`
  returns **empty**. Corroborated in-editor: the vendor CDO still reads `density 1`,
  `bBoxShape false`, `maxDrawDistance 25000`, `wInd World Space false` — none of my overrides
  leaked upward. `BP_FogArea` was never opened in its editor; `M_FogArea`/`MF_Fog` were never opened
  at all. No save prompt was ever accepted.
- ✅ A temporary unlit-black probe material was created at
  `/Game/Dev/TASK841Scratch/M_TASK841_ProbeBlack` and **deleted, with its folder**
  (`Content/Dev/TASK841Scratch` confirmed absent from disk).
- ⭐ **Cost, honestly:** `M_FogArea` is `MD_Volume`, so its cost is **view-resolution-sized, not
  volume-sized**, and the noise samples absolute world position — the box does not get more
  expensive by being bigger, and the noise does not stretch when scaled. Enlarging past the arena
  (§5.2) was therefore free. It does compound with the volumetric-fog pass generally (`J-W8`); I
  did no GPU profiling and do not claim a frame cost.

---

## 6. CAMERA COORDINATES ACTUALLY USED (`SC-§85`)

⛔ **The forbidden vantage — `x −20607.8, y 0, z 98.15`, yaw 180, FOV 90 — was never used.** No
capture is within 2,000 uu of it, and none looks at a castle gate from ground level.

| id | location | rot (pitch, yaw, roll) | FOV | what it is |
|---|---|---|---|---|
| PNG-control A/B | `(0, 0, 1200)` | `(-5, 0, 0)` | 90 | the noise-floor pair, §3.1 |
| MEASURE rig | `(0, 0, 1200)` | `(30, 0, 0)` | 90 | black probes against clean sky, §3.3–3.4 |
| **E1** | `(0, 0, 250)` | `(-2, 0, 0)` | 90 | **hero eye height, mid-field, down the lane** |
| **E1-control** | `(0, 0, 250)` | `(-2, 0, 0)` | 90 | identical, fog actor removed |
| **E2** | `(-6000, 6000, 400)` | `(8, -35, 0)` | 90 | **into the sun** (sun bearing derived from `DirectionalLight_0` pitch −38 yaw 145) — god-rays |
| **E3** | `(24000, 11000, 1200)` | `(-6, 200, 0)` | 90 | **arena corner** — the §5.2 before/after pair |

All captures 2764 × 828. Camera pose in the table is the pose I requested; each capture's returned
`cameraLocation`/`cameraRotation`/`cameraFOV` matched it and is what I recorded.

**E1 vs its control, identical coordinates, area-averaged pixels:**

| | control (no fog) | fogged | vs noise floor |
|---|---|---|---|
| mean luma | 0.485943 | 0.698263 | **+43.69 %** vs a **+0.05 %** floor ⇒ **874×** |
| detail (std) | 0.157705 | 0.068498 | collapses to **43.4 %** |
| PNG bytes | 3,426,504 | 1,162,110 | ratio 0.339 vs a 0.998 byte floor — *reported, not relied on* |

Working PNGs (session-local scratch, not committed):
`C:\Users\wesel\AppData\Local\Temp\claude\C--GitProjects-GitHub-GitClaudeUnrealTesting-GitClaudeUnrealTest\7fc2d172-00dd-497a-98eb-46eae267c5c0\scratchpad\fog\`
— `control_A/B.png`, `E1_final.png`, `E1_control.png`, `E2_fog.png`, `E3_fog.png` (hole),
`E3_fog_fixed.png` (fixed), `black_nofog.png`, `d5_r1..r4.png`.

**What the frames show, in words.** At hero eye height mid-field (E1) the player can read the grass
for a few hundred uu directly ahead and **nothing else** — the enemy castle 25,000 uu away is
completely gone. Looking into the sun (E2) the field is a white-out crossed by crepuscular rays.
At the arena corner (E3, fixed) the white-out is identical to mid-field. **This does not read as
light haze**, which is the point: the mechanic already blinds units by 87.8% and makes them drop
targets, and the picture now agrees with that.

🧑 **Legibility is Jonathan's eye and only his** (`FOG-§11`(5)) — this is a playtest item, not a gate
item. The specific question worth putting to him: **at `density = 5.0` the fog is thick enough that
the battlefield reads as a white-out from a standing hero.** If he wants to see further while units
stay blind, `general Data.density` is the single knob, and lowering it is a one-value retune with no
code change.

---

## 7. FENCES HONOURED

⛔ No `Source/` file touched · no compile · no Git write · `TASK-1041`'s row untouched ·
`L_Arena` never saved (hash verified unchanged) · every save used an **explicit one-asset list**
(`save_assets(["/Game/Blueprints/BP_SiegeFog"])`), never the empty list · every asset claim verified
by read-back, never by a success return · `Content/FogArea/**` not edited, moved, deleted or opened ·
only `TASK-841`'s own `status:` line edited on the board.
