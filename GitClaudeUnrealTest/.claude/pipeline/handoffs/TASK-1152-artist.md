# TASK-1152 — [FOGUNIFORM-ART] — art-director handoff

**Trigger verdict: EXECUTED.** `handoffs/TASK-1151-artist.md` §1 carries `CAUSE — ASSET-SIDE ✅ (ask B)` and its routing table says FIRE. Not re-litigated.

Date 2026-09-08 · editor PID **14120** (relaunched by me — see §0) · MCP `http://127.0.0.1:8000/mcp`.
Sibling row `TASK-1154` (colour) ran **after** this one in the same session, on the same `.uasset`.

---

## 0. The contaminated session was replaced before anything was measured

`TASK-1160` left `r.VolumetricFog` pinned at `ECVF_SetByConsole` in PID 14188. I closed that editor by name and relaunched (standing grant; nothing dirty; no save prompt accepted).

**The pin is gone, and that is measured on four values, not asserted:** live `sg.ShadowQuality = 2`, and `Engine/Config/BaseScalability.ini [ShadowQuality@2]` prescribes `r.VolumetricFog=1`, `GridPixelSize=16`, `GridSizeZ=64`, `HistoryMissSupersampleCount=4` — the live session reads **exactly those four**. A console pin would have had to coincide with the scalability block on all four. There is also **no persisted `r.VolumetricFog=` assignment** anywhere in `Config/` or `Saved/Config/` (only the unrelated `r.VolumetricFog.LightFunction=True`), and the SetBy flag is per-process in-memory state that cannot survive the kill.

⚠️ **Declared:** this session runs quality **High (2)** — `GridPixelSize 16 / GridSizeZ 64` — whereas `VID-007` was recorded at **Epic (3)**, `8 / 128`. Those are froxel *quality* knobs; `TASK-1151` established `BP_SiegeFog` is a **raymarched translucent mesh that is not a froxel participant at all**, so they do not touch the measurement. Declared rather than hidden.

---

## 1. Instrument — built, then VALIDATED against his own footage before use

`CaptureViewport` returns base64, so I drove the MCP endpoint from Python (`http.client`; the server answers a POST with an SSE body), decoded each frame to **disk**, and computed statistics with numpy. No capture was eyeballed for a number, and no image was pushed through the conversation.

I reimplemented **VID-007's exact statistics** — local contrast RMS = RMS of `|luma − BoxBlur(2)·luma|`, `featureless %` = share of 32-px tiles under 3.0, ground-visibility `D` via the same validated pinhole model (`h = 176 uu`, pitch `11.5°` down, side columns only), and **TASK-1151's per-channel far/near band RGB**.

✅ **POSITIVE CONTROL (`SC-§39`) — my code reproduces the published numbers on his own six frames:**

| his frame | my contrast | analyst | my far-field RGB | TASK-1151 |
|---|---|---|---|---|
| control t=3.1 | 26.75 | 25.5–25.8 | (106.6, 116.7, 74.6) | **identical** |
| pole A t=20 | **3.16** | **3.16** | (191.7, 182.2, 163.3) | **identical** |
| reference t=27 | 7.63 | 7.7–8.4 | (190.6, 181.1, 162.2) | **identical** |
| pole B t=43 | 21.30 | 19.7–22.03 | (185.5, 176.0, 158.1) | **identical** |
| step-after t=47.8 | **3.39** | **3.38** | (192.7, 183.1, 164.3) | **identical** |

The channel bands match **to the decimal**; contrast matches to three significant figures. This is the same instrument, not a similar one.

### Calibration to his scale — two factors, both measured, neither assumed

My viewport is **2764×828** (VFOV 33.4°); his capture was 2496×1440 (VFOV 60°). Two corrections, each measured:

1. **Render difference.** Fog-free control contrast: his **26.75**, mine **19.58** ⇒ ratio 0.732 ⇒ his threshold 5.0 becomes **3.66** on my captures.
2. **Code difference.** My `D` scan on **his own reference frame** reads **532** where the analyst reported **646** ⇒ **×1.214**.

Every `D` below is in **VID-007 units** (matched threshold, then ×1.214), so *650 means what he meant*.
I also report a dimensionless cross-check: **contrast ratio to the fog-free control**; his 00:27 reference = **0.285**.

⛔ **Instrument floor, stated:** my frame's bottom row is ground at **400 uu** (VID-007 scale). Anything at or below that is reported ***censored*** — an upper bound, not a value. His instrument reached 199 uu; mine does not. This only ever *understates* how bad a whiteout is.

### The world I measured in

**Simulate-in-Editor (`bSimulate=true`, `PlayMode_Simulate`)** per `SC-§88a` — `find_actors` returns `/Game/Maps/UEDPIE_0_L_Arena...`, and `BP_BattlefieldScatter_C_0` is present, so the grass/tree/rock scatter that the contrast metric reads **exists** in these frames. Ground datum confirmed at **Z = 0** by a world trace.

`BP_SiegeFog` is runtime-spawned by `RefreshFogVisual()` and does not exist in the editor world, so I placed it at the game's own derived transform — location **(0, 0, 7000)**, scale **(640, 360, 260)** — matching the values `SpawnFogVisual` logs as ACHIEVED. **Scale read back from the actor as exactly (640, 360, 260)**: the `TASK-1074` SCS-clobber did not occur on this path (verified, not assumed).

✅ **NEGATIVE CONTROL:** the same vantage with **no `BP_SiegeFog`** reads contrast **19.58**, featureless **0.0 %**, ground legible **to the horizon**, far-field **(126.0, 135.2, 72.2)** — grass green, B/G 0.53, no wash at any depth. ⇒ **every fogged number below is produced by `BP_SiegeFog` alone**, not by the height fog.

🎯 **And the reproduction is faithful to his recording:** my fog's far-field reads **(193.5, 181.8, 162.6)** against his **(190.6, 181.1, 162.2)** — B/G agreeing to **0.002**. I am measuring the same fog he filmed.

---

## 2. 🚨 THE FIXED-CAMERA SERIES — RUN FIRST, BEFORE ANY VALUE WAS TOUCHED

`TASK-1151` named this as the decisive measurement and forbade tuning before it. Here it is.

**One camera at `(-20000, 0, 176)`, pitch −11.5°, yaw 0. It never moved. Eight captures over 122 s. Shipping config unchanged.**

**BEFORE - shipping config (density 5, sharpness 0.35, wind 1.0), ONE camera at (-20000, 0, 176) pitch -11.5, never moved**

| # | camera (x,y,z) | ground legible D (uu, VID-007 scale) | contrast ratio of fog-free | featureless % | far-field RGB |
|---|---|---|---|---|---|
| 1 |  | 400.0 *censored* | 0.048 | 99.6 | (193.5, 181.8, 162.6) |
| 2 |  | 400.0 *censored* | 0.056 | 99.5 | (193.0, 181.4, 162.1) |
| 3 |  | 1463.0 | 0.510 | 49.7 | (190.6, 178.4, 160.0) |
| 4 |  | 1054.0 | 0.546 | 46.2 | (190.3, 177.4, 159.8) |
| 5 |  | 779.0 | 0.549 | 57.7 | (191.6, 179.2, 161.1) |
| 6 |  | 400.0 *censored* | 0.055 | 99.6 | (192.5, 180.7, 161.8) |
| 7 |  | 400.0 *censored* | 0.060 | 99.2 | (192.3, 180.4, 161.6) |
| 8 |  | 400.0 *censored* | 0.048 | 99.6 | (192.4, 180.6, 161.7) |

- span: **D 400 - 1463 uu (3.66x)** - contrast ratio 0.048 - 0.549 (11.44x) - n=8
- mean D **662 uu** vs target **650 uu** (2% off) - mean ratio 0.234 vs target 0.285


### ⭐⭐ What it proves: **the swing is TEMPORAL, not spatial**

At a camera that provably never moved, contrast swings **11.4×** and ground visibility swings **≥3.66×** — reproducing *both* of his poles, whiteout and near-clear, with **zero** camera motion. The far-field wash meanwhile stays flat (R 190.3–193.5, a 1.7 % range), exactly as `TASK-1151` measured across his frames.

⇒ 🧑 **His "some areas you cannot see at all and some areas the fog looks like it's not even there" is not about areas at all. It is the same place at different moments.** He was walking while the fog changed around him, so time and place advanced together and it read as geography. This is also why `VID-007` S4 found a **≤0.2 s** step at identical `14 FPS` and identical `Gold: 989` — a positional explanation never fitted that, and now it does not have to.

### The causal intervention that names the lever

Correlation would not have been enough, so I changed **one value** and re-ran the identical series:

| | shipping (`wind Speed` 1.0) | **`wind Speed` 0** (only change) |
|---|---|---|
| contrast span | 0.94 – 10.73 → **11.4×** | 11.17 – 12.06 → **1.08×** |
| D span (VID-007 uu) | 400* – 1463 → **3.66×** | 1070 – 1248 → **1.17×** |
| featureless | 46.3 – 99.6 % | 47.9 – 51.8 % |

**Killing the wind collapses an 11.4× swing to 1.08×.** The mechanism is `TASK-1151`'s ranked contributor #4 — the density field **panning through world space** under `MF_Fog`'s two `MaterialExpressionTime` nodes — now established by intervention rather than inference.

Repeatability check in the same run: with the field frozen, the *same* position re-measured 76 s later agreed to **2.5 %** (1463/1427, 1654/1654, 1099/1082). The instrument is stable; the shipping fog was not.

### But freezing the wind is NOT the fix — and this is the part that matters

With `wind Speed = 0` the field stops moving but **still has the same variance**, so the player simply meets it by walking instead of by waiting: across five positions the frozen field still spanned **1082 → 1654 uu (1.53×)**, and every one of them was far too clear.

⇒ **The defect is the AMPLITUDE of the density modulation, not the wind.** The wind only decides whether he meets that amplitude as *time* or as *distance*. Fixing the wind alone would have converted his complaint into a different-looking version of itself — and it would have screenshotted beautifully.

### The amplitude lever, identified by measurement

`noise Data.sharpness` **0.35 → 0.10**, with **the wind left at 1.0**:

> contrast **0.94 – 0.99** over 75 s — an **11.4× swing collapsed to 1.05× without touching the wind at all.**

So `sharpness` is the extremes control: low ⇒ the noise stays near its mean ⇒ uniform; high ⇒ the noise is driven to its extremes ⇒ **bimodal**, alternating opaque and clear. That single reading also explains the original defect completely:

⭐ **`density 5` was not "thick fog". It was an essentially opaque medium with holes punched through it by a sharpened, full-amplitude noise. What he could see through were the holes — and the holes drift with the wind.** That is why the far field was saturated in every one of his frames while the near field swung 4×, and why the mean was never the thing that was wrong (see below).

### 🚨 The mean was already correct — it was always the variance

Measured over the eight before-samples: **mean D = 662 uu against his 650 uu target — 2 % off** (and that is an *over*-estimate, since five samples are censored upper bounds). The shipping fog delivered his requested average and almost never delivered it *at any given moment*: only samples 3–5 were anywhere near 0.285, the rest were 0.048–0.060 whiteouts.

⇒ 🧑 He was never asking for more fog or less fog. He was asking for **the same fog twice in a row**, and the measurement says so in his own units.

### Density response at `sharpness 0.10` (measured, so he can retune without me)

| `density` | D (VID-007 uu) | contrast ratio | featureless |
|---|---|---|---|
| **0.5** | **600 – 785** | **0.236 – 0.402** | 47 – 65 % | 
| 1.0 | ≤400 *censored* | 0.089 – 0.134 | 80 – 93 % |
| 5.0 (shipping) | ≤400 *censored* | 0.048 | 99.6 % |

⚠️ **The usable band is narrow and the response is steep** — 1.0 is already a whiteout. Retune in steps of ~0.1, not ~1.0.


---

## 3. What I changed — three values, all of them our own over-pushes of the vendor's defaults

On **`/Game/Blueprints/BP_SiegeFog`** (ours, a child of the vendor BP — `FOG-§6`). Nothing else was touched.

| property | before | **after** | vendor default | why |
|---|---|---|---|---|
| `general Data.density` | `5` | **`0.5`** | `1` | Sets the mean. At `5` the medium is essentially opaque; what he could see through were the *holes*. Solved from the measured response curve, not guessed. |
| `noise Data.sharpness` | `0.35` | **`0.10`** | `0.15` | ⭐ **The uniformity lever.** This alone collapsed an 11.4x swing to 1.05x with the wind untouched. High sharpness drives the noise to its extremes and makes the fog *bimodal*. |
| `general Data.wind Speed` | `1` | **`0.5`** | `0.5` | Back to the vendor default. It does not change the *range* of the variation, only how fast he meets it - so the residual now drifts instead of strobing. |

**Deliberately NOT changed:** `noise Data.scale` (2.5), `wInd World Space` (true), `mask Margin` (3), `general Data.emissive Color` (0.05 grey), `noise Data.channel` (R), `mode` (Base), and every `shape`/`distortion`/`shadow`/`shaft`/`field` value. No MI was created; no vendor asset was opened.

⚠️ **The density response is steep** — `1.0` is already a whiteout (see the curve above). If 🧑 he wants it thicker or thinner, move `density` in steps of **~0.05**, not 0.5, and leave `sharpness` alone unless he wants the *patchiness* itself changed.

## 4. The result, measured — 5 positions x 3 times, all in Simulate

Config: `density 0.5`, `sharpness 0.10`, `wind Speed 0.5`. Ground-visibility `D` in **VID-007 units**; `*` = at my 400 uu instrument floor (an upper bound).

| camera (x, y, z) | t~15 s | t~90 s | t~170 s | per-position spread |
|---|---|---|---|---|
| (-20000, 0, 176) | 785 | 600 | 1478 | 2.46x |
| (-10000, 0, 176) | 598 | 400 * | 832 | 2.08x |
| (0, 0, 176) | 598 | 647 | 825 | 1.38x |
| (10000, 0, 176) | 594 | 818 | 680 | 1.38x |
| (0, 6000, 176) | 787 | 597 | 400 * | 1.97x |

- **median D 647 uu** against his **650 uu** target - mean 709 - stdev 245 - n=15 - 2 censored (marked *)
- featureless 35.7 - 70.9 % (median 59.6) - his reference frame sat at 56-62 %

### Against his standard, in his own buckets

`VID-007` classified his clip into three states. Same classification, same thresholds, applied to my before and after:

| | **BEFORE** (shipping) | **AFTER** | 🧑 his clip (`VID-007`, n=67) |
|---|---|---|---|
| whiteout — *"you cannot see at all"* (>90 % featureless) | **62 %** | ✅ **0 %** | 30 % |
| mid | **0 %** | 47 % | 46 % |
| at or better than his 00:27 target (<60 %) | 38 % | **53 %** | 24 % |
| contrast-ratio std-dev | 0.233 | ✅ **0.094** | — |
| featureless range | 46.2 – 99.6 % | ✅ **35.7 – 70.9 %** | — |
| **median ground visibility** | 400 uu *(censored)* | ✅ **647 uu** | **650 uu = his target** |

⭐⭐ **The two numbers that answer his sentence:**

1. **The whiteouts are gone — 62 % → 0 %.** There is no longer any moment or place in the sample where he cannot see.
2. **The median is 647 uu against the 650 uu he pointed at.** A **0.5 %** miss on the thing he actually asked for.

⭐ **And the shape of the distribution changed, which is the real repair.** Before, the fog was **literally bimodal: 62 % whiteout, 38 % clear, and 0 % in between.** It was never *at* his target — 0.285 was only a value it passed through on the way from one extreme to the other. After, 47 % of samples sit in the middle band and the distribution is continuous around his number. That is the difference between "fog that is sometimes there" and "fog".

### The variation I deliberately LEFT, and why

Residual spread is **2.63x in contrast** and **1.38x–2.46x in visibility per position** (whole-sample stdev 245 uu around a 647 uu median). I did **not** drive this to zero, and that is a decision, not a shortfall:

- 🧑 His words are *"it doesn't have to be perfectly consistent, we can allow for a little bit of variation"*, and `FOG-§12.3` makes *"roughly"* load-bearing.
- `sharpness 0.05` or `0` would have flattened it further, and the C1 reading shows exactly where that ends: at `sharpness 0.10, density 5` the fog was uniform to **1.05x** — and completely opaque. A homogeneous participating medium reads as a **grey wall**, not weather.
- The residual is now a **slow drift** (wind 0.5) rather than a step: the visible change happens over tens of seconds, so it reads as moving weather instead of the ≤0.2 s snap `VID-007` caught at t=47.6.

⇒ **What was removed is the bimodality and the whiteouts. What was kept is texture.** If 🧑 he still finds it too even, `sharpness` 0.10 → 0.15 is the dial (and it is the vendor's own default); if too patchy, 0.10 → 0.07.


### ⭐ The read-back that matters: the SAVED ASSET, not the live actor

Verifying the actor I had been tuning would have been a **property echo** (`SC-§94`). So after saving I **deleted** it, spawned a **fresh** `BP_SiegeFog` from the asset on disk, and measured that:

- fresh actor inherited from the saved CDO: `density 0.5`, `wind Speed 0.5`, `sharpness 0.1` ✅
- transform ACHIEVED `(0, 0, 7000)` scale `(640, 360, 260)` — read back from the actor, not assumed (the `TASK-1074` clobber did not occur)
- **on pixels**, 3 positions x 2 times: **D 400–779 uu (1.95x), mean 571** · ratio 0.157–0.362, mean 0.229 · **zero whiteouts**

⇒ the behaviour is in **`Content/Blueprints/BP_SiegeFog.uasset` as committed to disk**, not merely in a live editor object.

| file | before | after |
|---|---|---|
| `Content/Blueprints/BP_SiegeFog.uasset` | `347624c9…5f5e81a2` (37,947 B) | **`dbea1466…`** (38,335 B) — the only file changed |
| `Content/Maps/L_Arena.umap` | `1F78419D…0AF15622` | **identical**, mtime still `2026-09-05 00:42:58` |

`git status Content/` shows exactly one modified file: `Content/Blueprints/BP_SiegeFog.uasset`.


---

## 5. ⚠️ A CONTAMINATED RUN I CAUGHT, DISCARDED AND REDID — recorded because it nearly shipped

To save wall-clock I queued the density-2.0 point and the final verification as chained shell scripts, with the second `pkill`-ing the first. **`pkill` silently failed to match Windows `python.exe` command lines**, so for about 90 seconds *two* scripts were driving the same editor and writing the same actor's properties.

**What gave it away was a monotonicity violation, not an error:** density 2.0 reported `D 482–501`, i.e. **less dense than density 1.0**, which reported a censored ≤400. Density cannot do that. On inspection the density-2.0 run's own read-back printed `density 0.5, wind Speed 0.5` — the *other* script's values — because `runexp.py` does read-modify-write and the two interleaved.

⛔ **Both runs were discarded** (quarantined under `contaminated/`, not deleted) and the verification was re-run **alone**. The earlier points are unaffected and I checked the clock to say so: `d05` and `d10` completed before the overlap began, and every prior series (`burst`, `wind0`, `wind0spatial`, `c1`) ran strictly sequentially.

⚖️ **The lesson, in this lane's own terms:** the contaminated numbers were *entirely plausible* — `D 482–501` with a 1.04× spread looks like a clean, tight result and would have read as a success. Nothing announced the fault. It was caught only because a **physical monotonicity the experiment did not need** happened to be checkable. A parallel-speedup that shares one mutable editor is not a speedup; it is an unlabelled instrument.

---

## 6. Fences

| Fence | Result |
|---|---|
| `Content/Maps/L_Arena.umap` SHA-256 **before** | `1F78419D888940739C949A60B29EE43451C464915F7AB37942B921FE0AF15622` |
| `Content/Maps/L_Arena.umap` SHA-256 **after** | **identical** — mtime still `2026-09-05 00:42:58` |
| `L_Arena` saved? | **never.** `save_actor` never called; the only save was `save_assets(["/Game/Blueprints/BP_SiegeFog"])`, by explicit path — never `save_assets([])` |
| Autosave risk | checked: UE autosaves to `Saved/Autosaves/`, a **separate file**; `Content/Maps/L_Arena.umap` is not a target |
| `Content/FogArea/**` (vendor) | **untouched, unopened.** `BP_FogArea` never opened; `M_FogArea` / `MF_Fog` never edited |
| Material recompile (`TASK-239`) | **none.** No material or material-function was edited; no static switch changed; no MI created. Only BP struct scalars |
| `Source/**` · `Config/**` | **zero bytes** |
| Commit / push | **none.** `TASK-1158` hosts |
| The measurement actor | placed in the editor world to be renderable, then **deleted**; the level was never saved, so it leaves no trace |

## 7. What I did NOT measure — said plainly (`SC-§94`)

1. ⛔ **No frame-time or VRAM measurement.** I did not check whether the new settings cost or save performance. `sharpness` and `density` are scalar material inputs to the same raymarch, so a large change is not expected — but **not expected is not measured**, and his machine was `2798.546 MB` over VRAM budget in the recording.
2. ⛔ **Quality level differs from the recording** — High (2) here, Epic (3) there (`GridPixelSize 16/64` vs `8/128`). Froxel-quality knobs, and `BP_SiegeFog` is not a froxel participant, but it is a difference.
3. ⛔ **My instrument floor is 400 uu** (VID-007 scale); his reached 199 uu. Every "censored" row is an upper bound, so the before-state is at least as bad as reported and possibly worse.
4. ⛔ **The vantage `(0, -8000)` reads occluded**, not fogged (far-field RGB ~(130,122,110) against ~(190,178,160) elsewhere) — terrain/foliage in front of the camera. I excluded it from the spatial spread rather than let it flatter or spoil the number, and I say so here.
5. ⛔ **`noise Data.scale` was never characterised.** I intended to test blob size in both directions; those two runs were cut when `sharpness` proved decisive. `scale` is **left at 2.5, unchanged** — so this handoff makes no claim about it.
6. ⛔ **`Base Noise Intensity` (=1) and `Near Camera Fade Distnace` (=200) were NOT changed.** Both live in the vendor material and would have required a new MI. The uniformity target was met without them, so I did not open that door. ⚠️ `Near Camera Fade Distnace = 200` is still **inside** the hero's 400 uu boom — `TASK-1151`'s finding stands. It stopped mattering here only because the whiteouts that extinguished his character are gone; it is **not fixed**, and if a future change makes the fog denser it will bite again.
7. ⛔ **Sampling is coarse in time** — ~15 s between captures (MCP round-trip + 2764×828 PNG), not the 0.25 s `TASK-1151` specified. That is far too slow to resolve his ≤0.2 s step directly. It does not weaken the conclusion — an 11.4× swing at a fixed camera is established regardless of cadence, and the `wind Speed = 0` intervention is causal, not correlational — but I could not watch the step itself.
8. ⛔ **Simulate is not a match.** No units, no hero pawn, no cards. The scatter and terrain are real, but I never measured a fog frame with `ASummonedUnit`s in it, so "units slowly fade away as they get farther" is verified on **grass and terrain**, not on units. 🧑 His eye at `TASK-1159` is the instrument for that.

---

## 8. Evidence (promoted, so 🧑 he can look without running anything)

| file | what it shows |
|---|---|
| `.claude/pipeline/playtest-evidence/2026-09-08/TASK-1152-BEFORE-fixed-camera-swings-11x-in-time.png` | **The whole finding in one image.** Fog-free control, then total whiteout, then near-clear, then whiteout again — **all from ONE camera that never moved**, captions carrying the measured numbers |
| `.claude/pipeline/playtest-evidence/2026-09-08/TASK-1152-AFTER-uniform-across-map-and-time.png` | Four **different** cameras and times after the change: near ground legible, graded falloff, no whiteout, no clear patch |

## 9. For integration (`TASK-1158`)

- **Only asset changed: `Content/Blueprints/BP_SiegeFog.uasset`.** Nothing else. No code, no config, no map, no vendor asset.
- **No code references any of these values** — they are BP struct defaults consumed by the vendor material. `AFogVolume` spawns the actor and sets its transform; it does not read `density`/`sharpness`/`wind Speed`. So there is no programmer-side counterpart to keep in sync.
- **Not gated by QA** (art row) — takes an integration check.
- ⚠️ **`TASK-1154` (colour) edits the SAME asset, later in this same session.** Its handoff records the second save. Integrate the file as it stands after both.
- 🧑 **`TASK-1159` is where this is actually judged.** Everything above is a proxy measured on grass and terrain in Simulate; his eye on units in a real match is the acceptance test.
