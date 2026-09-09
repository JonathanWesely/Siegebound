# TASK-1177 — [FOG-ASKB-VISIBILITY-CYCLE] — art-director

**2026-09-09.** Ask (B) re-opened as a question, answered with a measured series.

> ## 🚨 THE VERDICT, AGAINST THE RULE THAT WAS WRITTEN BEFORE THE ANSWER
>
> **`MIN/MAX VISIBILITY RATIO = 0.340` (card path) and `0.351` (probe).**
> **BOTH ARE `< 0.50` ⇒ DECISION RULE BRANCH (b) FIRES: THE DEFECT IS LIVE IN TIME.**
> The spatial fix held — ⛔ nothing about it is contradicted here — but at a **fixed
> camera with every value frozen**, how far the player can see swings **2.94×** on a
> cycle of ≈200 s wall / ≈180 s world. 🧑 His *"some areas you cannot see much at
> all… some areas the fog looks like it's not even there"* is reproducible **without
> moving**, and the 122 s window that closed ask (B) could not have seen it (`SC-§115`).

⛔ **This row measured. It did not author.** ZERO `.uasset` writes, zero `Source/`, zero
`Config/`, zero Git, zero compile. `L_Arena` never saved. The repair is the manager's to
board (`SC-§100`).

---

## 0. THE HEADLINE NUMBERS

| arm | `ACT=` | `WIN=` | n | window | **V₅ min** | **V₅ max** | **V₅ mean** | median | **min/max** |
|---|---|---|---|---|---|---|---|---|---|
| **CARD** (`Siege.Fog.Raise`) | `ship` | `cycle` | **101** | **220.0 s wall / 202.0 s world** | **1099.8 uu** | **3235.8 uu** | **2063.6 uu** | 2121.2 | **0.3399** |
| **PROBE + floor grid** | `ship` (hand-spawned construction) | `cycle` | **101** | **220.0 s wall / 195.4 s world** | **1250.0 uu** | **3557.3 uu** | **2137.8 uu** | 1735.7 | **0.3514** |
| **NO-FOG** (negative control) | `ship` (nothing raised) | `cycle` | **101** | **220.1 s wall / 201.2 s world** | 42845.7 | 42845.7 | 42845.7 | — | **1.0000** (censored beyond the frame on **all 101 frames**) |

Corroborating estimator (`V_fit`, the fitted Koschmieder range over the well-resolved
window): CARD **1002.7 → 3779.2 uu, ratio 0.2653**. PROBE is ill-conditioned in its thin
phase (dividing by a σ near zero) — **declared, not quoted as a result**; `V₅` is primary
and is the **more conservative** of the two.

⛔ **The window exceeds 180 s on BOTH clocks.** World time runs at ~90 % of wall here
(measured: 9.85/9.88/9.98 s of world per 10.03 s of wall), so a 189 s wall window would
have been **170.6 s world** — under the duty. The first NOFOG series was re-taken for
exactly that reason; the short one is kept on disk and is not quoted.

---

## 1. THE PINNED VANTAGE — quoted character-for-character from `FOG-§12.5`

> band `y ∈ [0.42, 0.48]`, `x ∈ [0.30, 0.70]` · camera **`(-20000, 0, 176)`** · pitch
> **`-11.5°`** · yaw **`0`** · FOV **`90`** · **`2764×828`** · **SIMULATE-IN-EDITOR** ·
> **8 s WARMUP**

**Echoed by the engine on EVERY ONE of the 303 series frames**, and the series runner
**aborts** on any drift:

```
CAMECHO loc=(-20000.0,0.0,176.0) roll=0.0000 pitch=-11.5000 yaw=0.0000 world=L_Arena t=…
```

⛔ **`unreal.Rotator` was built with KEYWORDS** — `unreal.Rotator(pitch=-11.5, yaw=0.0,
roll=0.0)` — so the positional `(roll, pitch, yaw)` trap never had a chance to fire. Every
echo reads `roll 0.0000 / pitch -11.5000`.

Warmup: ≥ 9 s between the raise (or the probe's Simulate entry) and the first captured
pixel, on every arm.

---

## 2. THE METRIC — a DISTANCE in `uu`, and the threshold is a NUMBER

⛔ **`B/G` was NOT used as a visibility proxy.** It is reported below only as a *secondary*
figure and as the link to the prior rows.

**Named candidate (b) from the spec was taken: contrast-vs-distance against the ground
plane.**

- **`d(row)`** — world distance for each screen row, from an **engine line trace** along the
  ray of the pinned camera.
  ⭐ **Cross-checked against an independent computation:** the ground is flat at `z = 0`,
  camera at `z = 176`, so the analytic slant range is `176 / sin(θ)`. Engine trace vs
  analytic at row 800: **`386.0` vs `386.0`**; at row 400: **`927.4` vs `927.3`**. Two
  independent methods, agreement to `0.1 uu`. The frame's ground spans **373 uu → the
  horizon**; the last usable ground row is **42846 uu**.
- **`C(d)`** — RMS of the **horizontally high-pass filtered** luminance
  (`L − boxfilter_x(L, 17 px)`) in a 2-row block over `x ∈ [0.30, 0.70]`.
  ⭐ **The high-pass is load-bearing and it is a measured decision, not a preference:** a
  plain standard deviation counts the **fog's own low-frequency blobs** as scene contrast
  and floors the metric at ≈3.1 luma, censoring everything past ~1500 uu. The high-pass
  drops that floor ~17× and buys two decades of range (measured ratio at 41364 uu:
  **0.0068** with the high-pass vs **0.057** without).
- **`C₀(d)`** — the same quantity from an **atmosphere-off reference** (`ShowFlag.Fog 0` +
  `ShowFlag.VolumetricFog 0`), mean of **8 frames**; frame-to-frame relative sd **1.21 %**
  mean / **3.25 %** max over 347 usable ground blocks.
- **`V₅` = the distance at which `C/C₀` crosses `0.05`**, by linear interpolation in
  `(d, ln ratio)`. ⭐ **PUBLISHED THRESHOLD: `0.05` — 5 % of intrinsic contrast**, the
  standard meteorological visual range. It was written into the instrument
  (`vis.py` header) **before the first series frame was captured.**

### 2a. ⭐ The verdict does not depend on the threshold — measured, not asserted

| contrast threshold | CARD min/max | PROBE min/max | |
|---|---|---|---|
| 0.02 | **0.607** | 0.338 | ⚠️ the one exception, named below |
| **0.05 (published)** | **0.3399** | **0.3514** | ⇒ branch (b) |
| 0.10 | **0.2814** | **0.3199** | ⇒ branch (b) |
| 0.20 | **0.2353** | **0.2710** | ⇒ branch (b) |
| 0.35 | 0.419 | 0.315 | ⛔ 14 / 17 frames censored — invalid |

**9 of the 10 valid (arm × threshold) cells land `< 0.50`. The single exception is CARD at
`0.02`, which reads `0.607` — inside the "routed, not ruled" band.** ⛔ Stated because it
exists, not buried. At `0.02` the crossing sits at 2524–4159 uu in the thick phase, where
the ratio curve has flattened into its tail and the crossing is poorly determined; it is
the *least* reliable of the five, not the most. ⛔ **The threshold was NOT moved after
seeing data** — `0.05` is the pre-registered one and it is what the verdict rests on.

### 2b. ⭐⭐ The strongest evidence needs NO reference, NO fit and NO threshold

Raw high-pass contrast at **fixed ground distances**, min/max across each 220 s series:

| arm | d = 500 uu | d = 900 uu | d = 1500 uu |
|---|---|---|---|
| **NO-FOG** | **0.948** | **0.923** | **0.924** |
| **CARD** | **0.349** | **0.212** | **0.126** |
| **PROBE** | **0.370** | **0.233** | **0.111** |

⇒ **The ground at 900 uu loses 79 % of its contrast between the thinnest and thickest
moment, at a fixed camera, with nothing changed** — while the same pixels in the no-fog
scene hold to within 8 % over the identical window. **The swing is in the fog, not in the
instrument, and it is visible in the raw pixels before any processing.**

---

## 3. 🚨 cl. (9) — THE CONFOUND CONTROL (W6). **REMEDY (i).**

⭐ **I USED REMEDY (i): I re-took the PROBE series WITH the floor's own froxel grid
applied** (`r.VolumetricFog 1` / `GridPixelSize 16` / `GridSizeZ 64`, driven by console and
**read back**). ⛔ **I did NOT use remedy (ii)** — that is a CDO write and cl. (0) forbids
it; it was never attempted and needs no routing.

### 3a. CARD arm — both log lines, captured BEFORE one pixel

```
[2026.09.09-11.24.12:710][124]LogGitClaudeUnrealTest: [FogVolume_0] Fog INTEGRITY FLOOR engaged —
ACHIEVED 'r.VolumetricFog'=1, 'r.VolumetricFog.GridPixelSize'=16, 'r.VolumetricFog.GridSizeZ'=64,
height fog volumetric=ON (view distance 6000), all read back off the machine after the write. …

[2026.09.09-11.24.12:711][124]LogGitClaudeUnrealTest: [FogVolume_0] Fog VISUAL spawned:
'BP_SiegeFog_C_2' — ACHIEVED Z=7000, ACHIEVED scale (640, 360, 260), read back from the actor with
GetActorLocation/GetActorScale3D …
```

Plus the `SC-§113` positive control on the same raise:
`[Siege.Fog.Raise] AFogVolume::RaiseFog() executed on 'FogVolume_0' and returned TRUE. Fog is now UP.`

⛔ **`TASK-1074`'s `(20,20,5)` extent substitution did NOT occur** — asserted against the
**ACHIEVED** scale `(640, 360, 260)`, never against the request.

### 3b. PROBE arm — there are **NO** such lines, and that is the point

A hand-spawned probe runs **no `AFogVolume` code at all**, so it emits neither line. The
grid was therefore supplied and **read back** instead:

```
CVAR-AFTER-RELEASE  r.VolumetricFog = 1 | GridPixelSize = 16 | GridSizeZ = 64
CVAR-PROBE-ARM      r.VolumetricFog = 1 | GridPixelSize = 16 | GridSizeZ = 64
PROBE VISUAL spawned: 'BP_SiegeFog_C_0' - ACHIEVED Z=7000, ACHIEVED scale (640, 360, 260)
PROBE FIELDS density=0.5 sharpness=0.1 windSpeed=0.5
PIE PROBE 'BP_SiegeFog_C_0' ACHIEVED Z=7000 scale (640, 360, 260)   [BP_SiegeFog count in PIE world: 1]
```

**Achieved geometry compared across the arms: `Z=7000` vs `Z=7000`; scale
`(640, 360, 260)` vs `(640, 360, 260)`. Grid compared: `1/16/64` vs `1/16/64`. Identical.**

⭐ **And the floor's own line settles why:** it reports *"The player's own pre-floor values
were 1 / 16 / 64"* — **on this machine the floor is a no-op for the grid**, so the two arms
were never going to differ on that axis. Measured, not assumed.

### 3c. Why the confound cannot be carrying my conclusion

The hazard cl. (9) names is a verdict of ***"the cycle is ABSENT under the card"*** reading
the grid instead of the actuation. **My result is the opposite: the cycle is PRESENT in
both arms at the same amplitude (0.340 vs 0.351).** A confound can manufacture a
*difference*; it cannot manufacture *agreement*.

⚠️ **Residual difference, declared:** the floor also enables the *height fog's* volumetric
component at view distance 6000, which remedy (i) did **not** equalise. At matched `B/G`
the probe reads ~15 % further than the card (e.g. `B/G 0.9276`: probe `V₅ 3376` vs card
`V₅ 2925`). That is a **level** offset; it does not touch either arm's **ratio**, which is
what the rule is written on.

---

## 4. cl. (7) — IS THE CYCLE AN ARTEFACT OF THE PROBE RIG? **NO.** `FOG-§12.5c` cl. 5 is NOT voided

The cheapest discriminator that clause names is exactly what was run, and it comes back
**negative**: the cycle is present when the fog arrives **by the card**, at the **same
amplitude** as under a hand-spawned probe (0.340 vs 0.351), with the same period and the
same waveform shape. ⇒ **`FOG-§12.5c` stands. The window is averaging the game, not the
rig.**

---

## 5. THE CONTROLS

### 5a. Calibrated control (`SC-§112` cl. 9) — measured vs published. **PASSED.** `ACT=archived`, `WIN=single`

| archived frame | published | **measured** |
|---|---|---|
| `VID-007-t00m27s` — 🧑 his beige | **0.891 / 15.5** | **0.8907 / 15.50** |
| `TASK-1167-A` — no-fog grass | **0.548 / 45.2** | **0.5483 / 45.17** |

⭐ **A defect the control caught:** my first saturation implementation (per-pixel mean)
returned `15.65 / 48.13` — *plausible, and wrong*. The published definition is the
saturation **of the band-mean colour**; only that reproduces both figures. ⛔ Without a
published prior number this would have entered the record unnoticed — which is precisely
what cl. 9 exists for.

### 5b. Acquisition + driver control (`SC-§112a` cl. 6 + cl. 8) — four states, predictions written first

Predictions were committed to `PREDICTIONS.txt` **before any of these captures**:

| state | prediction (written first) | measured | |
|---|---|---|---|
| **S1** no siege fog | \|σ\| < 5e-5 /uu; V₅ censored > 40000 uu; B/G 0.55–0.58 | σ **1.26e-5 / 2.77e-5**; V₅ **42846 censored**; B/G **0.5681 / 0.5679** | ✅ |
| **S2** card fog UP | V₅ 900–2000 uu; V_fit 800–1600 uu; B/G > 0.90 | V₅ **3145 / 3127**; V_fit **2246 / 2165**; B/G **0.9276 / 0.9279** | ⚠️ **BG ✅, distances ABOVE my band** |
| **S3** + `ShowFlag.VolumetricFog 0` | V_fit ≥ 3× S2; B/G < 0.80 | V_fit **97677 / 136599 (43–63×)**; B/G **0.5696 / 0.5698** | ✅ |
| **S4** flag restored | V_fit within ±25 % of S2; B/G > 0.90 | V_fit **1622 / 1563 (−28 %)**; B/G **0.9391 / 0.9408** | ⚠️ **just outside ±25 %** |

⛔ **Two predictions missed and I am reporting them as misses, not re-narrating them.**
Both misses are in the *same direction and have the same cause*: I wrote the S2 band from a
**single earlier peak-phase frame** (`B/G 0.9858 → V₅ 1261`), and by the time S2 ran the fog
had breathed to `B/G 0.9276 → V₅ 3145`. ⇒ **My own prediction was falsified by the very
phenomenon this row was sent to measure**, before I had measured it. The S4 −28 % is the
same thing across 12 s.
⭐ **What the control was for still discharges cleanly:** four states, four
mutually-distinguishable results, S3 collapsing to the no-fog value and S4 returning.
**A frozen, cached or dead acquirer returns identical numbers four times. It did not** — and
the driver demonstrably reached the rendered object (`V_fit` moved 43×, `B/G` moved 0.37).

### 5c. `SC-§112a` cl. 7 — what the NULL scores under **this row's** pass condition. **COMPUTED.**

A synthesised all-black `2764×828` frame scores **`V₅ = 373.4 uu`, censored
`below-near-edge`**. A *series* of such frames gives **`min/max = 1.000 ≥ 0.75` ⇒ "ASK (B)
HOLDS"**.

🚨 ⇒ **THE NULL SCORES A PERFECT PASS ON MY PASS CONDITION**, and so does any frozen or
cached acquirer — a ratio-of-extremes gate is maximally satisfied by a constant. ⇒ **The
acquisition gate RAISES, it never returns.** `series.py` refuses any frame failing
`acq_max > 0 AND acq_ncolours > 16`, *and* any frame whose camera echo drifts, and it
aborts the arm rather than scoring it. Measured across all 303 series frames: `acq_max` 237
or 255 everywhere, `ncolours` **310 – 3344**, zero raises.

### 5d. Negative control over the FULL window (cl. 5's explicit demand)

**101 frames / 220.1 s wall / 201.2 s world with no fog present:**

- **`V₅` constant at 42845.7 uu on every frame** (censored beyond the frame edge) — the
  instrument does **not** swing when the subject does not.
- **σ: min `−2.176e-05`, max `+1.944e-05`, mean `1.04e-07` /uu** — it **straddles zero**,
  i.e. pure noise. The card arm's *smallest* σ (`7.93e-04`) is **36×** the no-fog arm's
  largest |σ|.
- **`B/G` flat: 0.5679 → 0.5696 (range 0.0017)**; `sat` 43.04 → 43.21.
- ⚠️ **Honest caveat, declared:** the no-fog arm's `V_fit` min/max prints as `0.0252`. That
  is **`1/σ` where σ ≈ 0 — a reciprocal of noise, not a swing.** ⛔ I am not quoting it as
  an instrument instability, and the three quantities above are why.

---

## 6. cl. (4) — THE ASK-(B) FIELDS, QUOTED BEFORE AND AFTER

| field | BEFORE | AFTER | |
|---|---|---|---|
| `general Data.density` | **0.5** | **0.5** | ✅ |
| `noise Data.sharpness` | **0.1** | **0.1** | ✅ |
| `general Data.wind Speed` | **0.5** | **0.5** | ✅ |

⛔ **A READ-READ pair. Nothing was written.** Read off the `BP_SiegeFog` CDO *and* off both
the card-spawned and hand-spawned instances; all four readings agree. ⛔ `0.005` — the MID
scalar after the vendor's ×0.01 on Wind Speed only — was never written into the field.
`TASK-1172`'s colours also confirmed unmoved: `base (0.86, 1.00, 1.00)` ·
`emissive (0.05, 0.07, 0.325)`.

### `wind Speed` — cl. (4)'s named trap: **`NOT MEASURED`**

⛔ I did **not** discriminate the mechanism. The clean test is setting `wind Speed` to zero
and that is a fenced field. **The test that would settle it, named:** take this exact
`WIN=cycle` visibility series a second time with `general Data.wind Speed = 0` and compare
the min/max ratio — if the ratio goes to ≈1.0 the advection of the noise volume *is* the
breathing; if it stays ≈0.34 it is not. **That is a separate row and it is the manager's to
board.**

⭐ **One lead I can offer without writing anything**, `SC-§101`-labelled as a lead: the
waveform is **not sinusoidal** — it is a flat low plateau, a fast transition of ~10–15 s,
a flat high plateau, and back (see the plot). A smooth advecting noise field would be
expected to read closer to a sinusoid; a plateau-and-step shape is more suggestive of a
**large-scale feature translating through the camera's line of sight** than of a global
density modulation. ⛔ **Lead only. Not a finding.**

---

## 7. cl. (6) — LABELS ON EVERY FIGURE, AND THE `FOG-§12.5c` cl. 2 COMPANION

Every series figure above is `ACT=` and `WIN=cycle` labelled in §0. Secondary `B/G`
figures for the same series (`ACT` as §0, `WIN=cycle`):

| arm | far-field `B/G` (the gate band) | **thick-band `B/G` `y∈[0.05,0.25]`** (mandatory companion) |
|---|---|---|
| CARD | min **0.9255** max **0.9865** mean **0.9643** (range **0.0610**) | min 0.9890 max 0.9921 mean **0.9903** (range **0.0031**) |
| PROBE | min 0.9162 max 0.9798 mean **0.9571** (range 0.0636) | min 0.9831 max 0.9902 mean **0.9852** (range 0.0070) |
| NO-FOG | min 0.5679 max 0.5696 mean **0.5690** | min 0.6060 max 0.6078 mean **0.6068** |
| 🧑 his `VID-007` beige (`ACT=archived`, `WIN=single`) | 0.8907 | **0.9153** |

⭐ **The card arm independently replicates `TASK-1172`'s breathing to the third decimal:**
that row measured `0.9235 → 0.9857` at fixed colours; I measure **`0.9255 → 0.9865`** in a
different session, on a different actuation, with a different instrument author. And
**`r(V₅, B/G) = −0.921`** across the 101 frames.

🚨 **And this is the finding the manager should have: `FOG-§12.5c` cl. 2 ruled that the
far field keeps the gate seat because *"the far field is where 🧑 he looked"*. That ruling
is now corroborated by an instrument that is not `B/G` at all.** The thick band's range is
**0.0031** while the far field's is **0.0610** — and the thing the thick band is stable
*about* is the fog's own body, which is exactly the thing the player does not look at.
**The stable metric is stable because it is blind to the defect.**

---

## 8. WHAT THIS ROW DOES **NOT** CLAIM

- ⛔ **No claim that the spatial uniformity fix (`20c1bea`) regressed or was wrong.** It
  fixed *place*. This is *time*. `SC-§115`'s narrow claim is the correct one: the evidence
  that closed ask (B) **could not have seen this**, because its window was shorter than the
  period.
- ⛔ **No colour / ask-(C) claim in either direction.** Nothing here touches colour;
  `TASK-1159` and 🧑 his eye are untouched.
- ⛔ **No claim about the mechanism.** `wind Speed` is `NOT MEASURED` (§6).
- ⚠️ **The period is soft.** The window covers ≈1.1 cycles. Autocorrelation gives a first
  peak at 193.8 s wall but at `r = 0.09` — ⛔ that is not a period measurement, and I am
  not publishing it as one. What is solid: **both arms completed one full down-and-up
  excursion inside 220 s wall**, consistent with `TASK-1172`'s ~181 s.
- ⚠️ **What the distance means.** `V₅` is the visual range against the **ground plane** at
  the pinned camera. It is not the range at which a *unit* or a *tower* becomes visible —
  a tall high-contrast object will read further. The **ratio** is the transferable part.
- ⚠️ `C₀` is a single 8-frame reference. Its error is common-mode across a series and so
  cannot create a temporal swing; §2b is the C₀-free check that proves it.

---

## 9. HASHES (`SC-§108`) AND FENCES

| asset | BEFORE | AFTER |
|---|---|---|
| **`Content/Maps/L_Arena.umap`** | **`1f78419d888940739c949a60b29ee43451c464915f7ab37942b921fe0af15622`** | **IDENTICAL** — verified at row start, before teardown, and **after the editor was killed** |

| fence | result |
|---|---|
| `L_Arena.umap` | ✅ **never saved.** Dirtied in memory only (the probe was staged in the editor world because the only spawn APIs are editor-world); **discarded by killing the editor.** Hash unmoved at 3 checkpoints. |
| `save_assets` | ⛔ **never called at all**, with `[]` or otherwise. |
| `.uasset` writes | ⛔ **ZERO.** `git status --porcelain -- Content` is **empty** — and the pathspec was proven to resolve first (`git ls-files -- Content` = **4765 files**, both `L_Arena.umap` and `BP_SiegeFog.uasset` listed) so the silence is a **measurement, not a mis-anchor** (`SC-§102`). |
| Probe actors | ✅ `BP_SiegeFog_C_0` destroyed; **final sweep returns `[]`** (state asserted, not a tally — `SC-§104`). `FogVolume_0` lived only in the PIE world and died with it. |
| `Content/FogArea/**` · `Source/**` · `Config/**` · `Tools/**` | ⛔ untouched by me. (`Source/FogVolume.cpp` + `Tests/SiegeFogVisualTest.cpp` are dirty from **`TASK-1178`'s concurrent lane**, not from this row.) |
| `CONVENTIONS.md` | ⛔ not mine — arrived dirty, not touched (`SC-§82`; findings routed in §11). |
| Git index | ✅ **empty** — `git diff --cached --name-only` returns nothing. |
| Git / compile / push | ⛔ **none.** |
| `TASKBOARD.md` | **`Edit` tool only, my own `- status:` line only.** |
| `testvideo/**` | ⛔ never touched. |
| cvar hygiene | `r.VolumetricFog` / `GridPixelSize` / `GridSizeZ` driven **to the values they already held** (`1/16/64`, read back before and after) for remedy (i); `ShowFlag.Fog` and `ShowFlag.VolumetricFog` toggled 0 and **restored to 1**, verified by the picture returning (S4). Session-only; the editor was killed. |
| Editor | I **launched** it (it was down at row start) and **killed** it at the end — standing grant. |

---

## 10. EVIDENCE — promoted for 🧑 his eye, no tooling needed

`.claude/pipeline/playtest-evidence/2026-09-09/`

| file | what |
|---|---|
| ⭐⭐ `TASK-1177-CARD-peak-vs-trough-2.94x-nothing-changed.png` | **the one to look at.** Two frames, **same camera, same frozen values, 134 s apart.** Top: grass, rocks and plants read out to the mid-distance. Bottom: a near-whiteout with only the closest few plants left. **That is his sentence, at one spot, without walking anywhere.** |
| ⭐ `TASK-1177-visibility-swings-3x-at-a-fixed-camera.png` | the full 101-frame series for both arms with the `0.75` and `0.50` decision levels drawn on, and the no-fog control stated |

---

## 11. 🙋 FOR THE MANAGER

1. 🚨 **Branch (b) fired: `0.340` / `0.351`, both `< 0.50`. The repair is yours to board
   (`SC-§100`).** I measured; I did not author, and I did not touch a fenced field.
2. 🚨 **The one row that would name the mechanism, spec'd and ready:** re-run this exact
   `WIN=cycle` visibility series with `general Data.wind Speed = 0`. The instrument, the
   vantage, the C₀ reference and the analyser all exist and are calibrated; the row is
   ~25 minutes of wall clock. It needs the fence lifted on **one field**, and it answers
   `FOG-§12.5c`'s open question (ii) with a number instead of a lead.
3. ⭐ **`FOG-§12.5c` cl. 5 is DISCHARGED, negatively — the pin is NOT void.** The cycle
   survives the card path at the same amplitude. Worth writing into the clause so nobody
   re-runs it.
4. ⭐ **A candidate law, offered not asserted** (`SC-§82` — the file is yours):
   ***a pass condition written as a RATIO OF EXTREMES is maximally satisfied by a
   CONSTANT — so its null output does not merely score inside the band, it scores the
   BEST POSSIBLE VALUE.*** `SC-§112a` cl. 7 caught a black frame scoring `sat 0.0 %`
   *inside* a band; this is one rung worse and the same family: my all-black series scores
   `min/max = 1.000`, a **perfect** pass, and so does a frozen acquirer, a paused world, and
   a screenshot lane that silently returns the same file 101 times. cl. 7's general form
   (*"toward zero / silence / empty / neutral"*) does not name it, because a ratio gate's
   dangerous direction is **toward UNCHANGING**.
5. ⚠️ **A second `SC-§112a` cl. 8 note, from my own miss:** my S2/S4 predictions were
   falsified by the phenomenon under test. ⇒ **when the subject is known to move, a
   four-state prediction must be written in terms the movement cannot invalidate** (I
   should have predicted *"S3 ≫ S2 and S4 ≈ S2 ± the phase spread"*, not absolute bands
   copied off one earlier frame). The control still discharged; the *bands* were the
   defect, not the technique.
6. ⚠️ **Carried forward unsolved, not mine:** `TASK-1175b` §9's W1 / W2 / W3 / W4 / W5 /
   W8 remain open in `TASK-1178`'s lane. I touched none of them.
7. ⚠️ **A residual I could not equalise:** the integrity floor also switches the *height
   fog's* volumetric component on at view distance 6000, and remedy (i) covers only the
   three cvars. It shifts the probe arm's **level** ~15 % but not its **ratio** (§3c).
   If a future row needs the two arms matched on level as well, that is the axis.

---

## 12. ⭐ THE TRANSFERABLE LESSON

`TASK-1172` bought *a calibrated instrument pointed at a moving subject produces a precise,
reproducible number that means nothing*. This row is the other half of it, and it is
sharper:

> ***`TASK-1172` measured the RIGHT THING WITH THE WRONG WINDOW. Ask (B) was closed by
> measuring the WRONG THING with the wrong window — and nobody noticed, because `B/G` and
> visibility distance are correlated at `r = −0.92`, so the proxy AGREED with the truth on
> every frame anyone happened to look at.*** A proxy that tracks the real quantity almost
> perfectly is the hardest kind to catch, because it is only wrong about **what it entitles
> you to say**. Ask (B) is a sentence about **how far he can see**; every number that closed
> it was a sentence about **what colour the far field is**. It took `SC-§115` — a law about
> *time* — to expose a defect that was equally about *units*.
