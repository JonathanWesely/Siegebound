# Footage Review — VID-007

Video: `testvideo/Siegebound (64-bit Development PCD3D_SM6)  2026-09-08 12-34-21.mp4`
· 75.47 s · 2496×1440 · h264 · avg 27.447 fps (probe.json)
· ⚠️ **VFR capture (Game Bar)** — sheet labels are ±interval/2 (±0.79 s); `frames --at` seeks are exact, but the *game* rendered at 4–17 FPS, so two samples <0.1 s apart can be the same rendered frame.
· Autocrop: **none applied** (content filled the capture; no black canvas).
Reviewed: 2026-09-08 · Analyst: footage-analyst (diagnose-only, FR-§0.3)

🧑 **Jonathan, verbatim:**
> *"Another thing I want to fix is with how the fog is scattered. If you take a look at the video that I just most recently put in "testvideo", you can see how the fog in some areas gives you a more clear visual a short distance around the player, and then at some other areas you cannot see much at all. I want to make it to where regardless of where the player walks the amount they can see around them is the roughly same. The amount that I want that to be is something similar to how it looks in the latest video at the 27 second mark. You are able to see a little bit around you and the units slowly fade away as they get farther. It doesnt have to be perfectly consistent, we can allow for a little bit of variation, but as you see in the video they are some areas where you cannot see at all and some areas where the fog looks like its not even there."*

**Video identity verified before extraction** (the folder holds 8 recordings): newest mtime `2026-09-08 12:35:34.817`, 156,311,095 bytes — matches the dispatch exactly.

---

## ⭐ HEADLINE

**His 27 s reference = ground stays legible to ≈ 650 uu (21 ft / 6.5 m) from the camera.**

The clip swings from **< 243 uu** (his own character, 400 uu away, fully extinguished) to **≈ 970 uu** — **a ≥ 4× spread in visible distance, 7× in scene contrast.** 30 % of the fogged clip is at or near total whiteout; only 24 % is at or better than the standard he named.

⚠️ **And the 27 s number he chose by eye lands within 7 % of the game's own `FogVisionCeilingUU = 609.6 uu`** — the unit-acquisition radius the Fog card already imposes (`Siegebound/FogVolume.h`). His eye picked the number the gameplay is already using.

---

## Measurement method (identical for every frame — this is what makes the three moments comparable)

- **Local contrast RMS** = RMS of `|luma − BoxBlur(2)·luma|`. Fog is a low-frequency wash: it destroys local contrast while *raising* luminance.
- **Scene region** = frame minus the top 10 % and bottom 9 % (gold / FPS / VRAM banner / card bar), so no HUD element is ever counted as scene detail.
- **`featureless %`** = share of 32-px scene tiles whose contrast RMS < 3.0.
- **Ground-visibility distance `D`** = the screen row where contrast collapses below 5.0, scanning from the bottom (near) upward, measured **in the left/right side columns only** so the hero's own silhouette never drives the number; converted through a pinhole model `D(y) = h / tan(θ + atan((y−720)/f))`.
- **The projection model was validated on pixels, not assumed:** `f = 1248 px` (HFOV 90° over 2496 px), boom `TargetArmLength = 400.0f` (`GitClaudeUnrealTestCharacter.cpp:41`), capsule 192 uu ⇒ predicted hero on-screen height **599 px**; **measured ≈ 575 px** (4 % agreement). The hero's vertical midpoint sits within 4 px of frame centre, confirming zero socket offset and that the camera aims at the capsule centre. Derived: pitch **θ ≈ 11.5° down**, camera height **h ≈ 176 uu**, horizon row **y ≈ 466** — and y ≈ 466 is where the far field meets the treeline in the fog-free frame, an independent check.
- ⚠️ **Uncertainty:** ±1 band = ±22 px, which near the horizon is large. The 27 s figure is **650 (+48 / −41) uu**. Treat as **~650 ± 50 uu**.

**Positive control — a true fog-free baseline exists inside this same clip, on the same terrain, in the same session:** t = 3.1–8.0 s, *before the Fog card was played*. Contrast RMS **25.5–25.8**, mean luma **102**, featureless **2.9 %**, **no contrast collapse anywhere** (ground legible to the horizon). Every number below is measured against that control.

---

## Symptoms (his words → what the pixels show)

**S1 — "the amount you can see … at the 27 second mark" (his TARGET).**
At t = 26.4–27.2 s the ground stays legible to **D ≈ 646 uu**; contrast RMS **7.7–8.4** (30 % of the fog-free control); featureless **56–62 %**; mean luma **166–169**. The hero at 400 uu is clearly readable but desaturated; grass tufts and his cast shadow resolve; the ground washes out smoothly rather than cutting off. This is a *graded* falloff — it matches his description.

**S2 — "some areas where you cannot see at all."**
Worst sustained example **t = 19.0–21.0 s** (and a 5 s repeat at **48.0–52.0 s**). Contrast RMS **3.16** (12 % of control); featureless **98.5 %**; mean luma **176.3**. ⭐ **The player's own character is invisible.** On a 400 uu boom with `bUsePawnControlRotation`, the hero is *always* screen-centre — he cannot leave frame by camera rotation — so a man-sized, high-contrast object at 400 uu is being fully extinguished. Ground contrast is 0.2–0.5 even at the **bottom** scene row (D ≈ 243 uu), so the ground-visibility figure is **censored: D < 243 uu**. All that renders is HUD plus two blue unit health bars floating over nothing.

**S3 — "some areas where the fog looks like it's not even there."**
Best example **t = 43.0–47.6 s**. Contrast RMS **19.7–22.0** — i.e. **76–85 % of the fog-free control**; featureless **47.6–49.2 %**; mean luma **145.8** (control 102, pole A 176); ground legible to **D ≈ 833–971 uu**. Individual grass blades, the red cross on the tabard, the archer's blue tabard and red hair, and a sharp directional cast shadow all resolve.

**S4 — "regardless of where the player walks" — ⚠️ the variation is faster than walking.**
At **t = 47.6 → 47.8 s** the scene goes from S3 to S2 in **≤ 0.2 s**: contrast **20.19 → 3.38**, luma **149.1 → 175.7**, featureless **48.5 % → 92.9 %** (settled at 98.2 % by 48.0 s). Both frames carry the **same crop-verified HUD readout `14 FPS · 70.4 ms` and the same `Gold: 989`**. At `SprintSpeed = 750.f` (`HeroCharacter.h:1107`) that is **≤ 150 uu of hero travel**. ⇒ **This transition is a step, not a gradient walked into.** Any purely positional explanation requires a fog boundary sharper than 150 uu.

**S5 — ⛔ the first ~7.6 s is NOT a "fog is missing" symptom.**
The clip opens fully clear because **the Fog card had not been played yet.** Session log: `USpellLibrary: resolved 'Fog' for Blue` at **12:34:27.017**, visual `BP_SiegeFog_C_0` spawned **12:34:26.948**. Recording start = mtime − duration = **12:34:19.35** ⇒ **video t ≈ 7.6 s**, and the measured onset sits between t = 8.0 (clear, featureless 2.9 %) and t = 9.0 (fogged, 83.6 %). A reader skimming the contact sheets would mistake this for pole S3; it is the control.

---

## Timeline of findings

| # | t (video) | frame evidence | measured observation (pixels, not conclusions) |
|---|---|---|---|
| 1 | 0.0–8.0 | `f00003_10s`, `f00006_00s`, `f00008_00s` | **Fog-free control.** contrast 25.5–25.8 · luma 102 · featureless 2.9 % · no collapse: castle, treeline, deployment line all legible to the horizon |
| 2 | 8.0 → 9.0 | `f00008_00s` → `f00009_00s` | Fog onset. featureless 2.9 % → **83.6 %**, luma 102.5 → **177.1** in ≤1.0 s. Log-corroborated: visual spawned at video t ≈ 7.6 s |
| 3 | 9.4–11.0 | sheet_01 tiles | Ice-spike + rune-circle summon VFX overlays the fog — ⛔ not a fog reading; excluded from the poles |
| 4 | **19.0–21.0** | **`f00020_00s` (promoted)** | ⭐ **POLE A.** contrast **3.16** · featureless **98.5 %** · luma **176.3** · **D < 243 uu (censored)** · **hero at 400 uu not visible at all**; two health bars float over nothing |
| 5 | 24.0–25.0 | `f00025_00s` | Pole A repeat: featureless 98.3 %, D < 243 uu |
| 6 | **26.4–27.2** | **`f00027_00s` (promoted)** | ⭐ **THE REFERENCE.** **D ≈ 646 uu (~650 ± 50)** · contrast 7.7–8.4 · featureless 56–62 % · luma 166–169. Hero legible, ground fades smoothly, one unit visible + one unit's bar with no visible body |
| 7 | 27.4–28.0 | `f00027_40s`, `f00028_00s` | Reference degrading within 1 s: D 646 → **526 uu**, contrast 8.0 → 5.0. Even his chosen moment is not stable |
| 8 | 31.0 | `f00031_00s` | Single-second whiteout spike inside an otherwise mid-range run: featureless 62.9 % (30 s) → **98.3 %** → 75.7 % (32 s) |
| 9 | **43.0–47.6** | **`f00043_00s`, `f00047_60s` (promoted)** | ⭐ **POLE B.** contrast **19.7–22.03** (76–85 % of fog-free) · featureless 47.6–49.2 % · luma 145.8 · **D ≈ 833–971 uu** |
| 10 | **47.6 → 47.8** | **`f00047_60s` → `f00047_80s` (promoted pair)** | ⭐⭐ **THE STEP.** contrast **20.19 → 3.38** in ≤0.2 s. **Same `14 FPS · 70.4 ms`, same `Gold: 989`** on both sides. ≤150 uu of possible travel. Residual clear patch in the lower-right with a soft diagonal edge |
| 11 | 48.0–52.0 | `f00048_00s`…`f00052_00s` | Pole A sustained **5 s**: featureless 98.2 / 96.1 / 98.2 / 92.1 / 97.7 % |
| 12 | 58, 64, 71–72 | 1 s sweep | Further whiteouts: 98.0 %, 98.2 %, 91.7 %, 96.4 % |
| 13 | 66.0, 73.0–75.0 | `f00066_00s`, `f00073_00s` | Near-clear returns: D ≈ 897 uu and 971 uu; contrast 7.7 and 18.1–18.6 |
| 14 | whole clip | 67 samples at 1 s, t = 9–75 | **30 % of the fogged clip is >90 % featureless (his "cannot see at all"); 24 % is <60 % featureless (at or better than his 27 s target); 46 % sits between.** |

### Distribution across the fogged clip (t = 9–75 s, 1 s samples, n = 67)

| state | criterion | samples | share |
|---|---|---|---|
| near-total whiteout | featureless > 90 % | 20 | **30 %** |
| mid-range | 60–90 % | 31 | 46 % |
| at/better than his target | featureless < 60 % | 16 | **24 %** |

### The spread, three ways

| scale | Pole A (worst) | **Reference (his target)** | Pole B (clearest) | spread |
|---|---|---|---|---|
| ground visibility `D` | **< 243 uu** (censored) | **≈ 650 ± 50 uu** | **≈ 970 uu** | **≥ 4.0×** |
| contrast RMS | 3.16 | 7.98 | 22.03 | **7.0×** |
| featureless % | 98.5 % | 58.3 % | 47.6 % | 51 points |
| mean luma (control = 102) | 176.3 | 166.9 | 145.8 | — |
| vs. his target | ≤ 0.37× | 1.00× | ≈ 1.5× | — |

---

## Recording conditions — ⚠️ ESTABLISHED, AND IT REVERSES THE DISPATCH'S WORRY

The dispatch warned that at Shadows Low/Medium there would be **no volumetric fog at all**, which would change what this footage is evidence of. **Measured: volumetric fog was ON, at Epic.**

Source of record — `Saved/Logs/GitClaudeUnrealTest_2-backup-2026.09.08-19.35.42.log`, the standalone game process, **open 12:32:23 → closed 12:35:42**, which fully contains the 12:34:19–12:35:34 recording:

- `LogConfig: Applying CVar settings from Section [ShadowQuality@3] File [Scalability]` — **the ONLY `[ShadowQuality@…]` application in the entire session.** No mid-session downgrade.
- `Set CVar [[r.VolumetricFog:1]]` · `GridPixelSize:8` · `GridSizeZ:128` · `HistoryMissSupersampleCount:4`
- `LogSiegeGraphics: Subsystem initialized — Overall=3 (Epic), ResolutionScale=100.0%, 3200x1800, WindowMode=1, VSync=off`
- `Set CVar [[r.SupportExpFogMatchesVolumetricFog:0]]` (project config)
- Fog visual read back live: `ACHIEVED Z=7000, ACHIEVED scale (640, 360, 260)` ⇒ **TASK-1074's repair held in this session** — this is not the old (20,20,5) defect.
- One `Fog` cast (12:34:27.017, 50 gold); **no `BrightSun` was played** during the clip; fog destroyed 12:35:39.945, *after* the video ends. ⇒ fog was continuously up from t ≈ 7.6 s to the end.

⛔⛔ **TRAP, RECORDED SO NOBODY REPEATS IT:** `Saved/Config/WindowsEditor/GameUserSettings.ini` currently reads `sg.ShadowQuality=1` and `sg.ResolutionQuality=71`, which *would* mean no volumetric fog. **Its mtime is 12:37:23 — about two minutes AFTER this session closed.** It was written by a *later* session and is **not** the recording's state. Reading that file naively produces a confident, wrong story of exactly the kind this project has been burned by three times this week.

**Also crop-verified (3× scale, never asserted from a sheet tile), present in every frame:**
`Video memory has been exhausted (2798.546 MB over budget). Expect extremely poor performance.`
⚠️ For scale: the earlier lane finding on record was `0.922 MB over budget`. This is ~3000× worse and is a **separate issue**, flagged here because it is a live confound for anything texture- or streaming-related.

---

## Where on the map — ⛔ COULD NOT ESTABLISH (and one bound that survives)

- **Absolute position: not established for any fogged moment.** There is no minimap, no coordinate readout, and no legible landmark on the HUD. Every landmark that would fix position (castle, treeline, rock scatter) is extinguished by the fog in precisely the frames where I would need it. I am not guessing.
- **Pre-fog only:** at t = 3.1 s the hero stands on the blue deployment-zone line with a red-roofed castle ahead at mid-distance. That is the only positional fact the footage supports.
- ⚠️ **The terrain correlation is selection-biased and I will not launder it as a finding.** The clear frames (43.0, 47.6, 66.0, 73.0) *do* consistently show the hero on a **crest with the ground falling away**. But terrain is *only observable when the fog is thin* — I cannot see the terrain in a whiteout frame at all. So "clear = high ground" **cannot be tested against** "dense = hollow" from this footage. It is a hypothesis for someone with engine access, not an observation.
- **What the footage DOES bound:** the 47.6 → 47.8 step allows **≤ 150 uu** of hero travel. A spatial fog field can only explain that with a boundary sharper than 150 uu. This does not rule out positional variation over the *longer* runs (e.g. the 9 s clear stretch at 39–47 s), but it does rule position out as the *whole* story.

---

## Evidence frames (promoted)

- `.claude/pipeline/playtest-evidence/2026-09-08/VID-007-t00m03s-control-no-fog-before-card.png` — **the control.** t = 3.1 s, before the Fog card: castle, treeline, deployment line and grass all crisp to the horizon; contrast 26.78, featureless 1.0 %, luma 104.3.
- `.claude/pipeline/playtest-evidence/2026-09-08/VID-007-t00m20s-pole-a-total-whiteout.png` — **pole A.** t = 20.0 s: a uniform beige field. No ground, no grass, no shadow, **no player character**; only HUD and two blue health bars over nothing. contrast 3.16, featureless 98.5 %.
- `.claude/pipeline/playtest-evidence/2026-09-08/VID-007-t00m27s-reference-standard.png` — **⭐ the reference.** t = 27.0 s: hero legible at 400 uu, grass tufts and cast shadow resolve, ground washes out at **D ≈ 646 uu**; contrast 7.98, featureless 58.3 %.
- `.claude/pipeline/playtest-evidence/2026-09-08/VID-007-t00m43s-pole-b-near-clear.png` — **pole B.** t = 43.0 s: grass blades, red cross, archer's red hair and blue tabard, sharp cast shadow; **D ≈ 833 uu**, contrast 22.03 = 85 % of the fog-free control.
- `.claude/pipeline/playtest-evidence/2026-09-08/VID-007-t00m48s-step-before-clear.png` — **the step, before.** t = 47.6 s: `Gold: 989`, `14 FPS · 70.4 ms`, contrast 20.19, featureless 48.5 %.
- `.claude/pipeline/playtest-evidence/2026-09-08/VID-007-t00m48s-step-after-opaque.png` — **the step, after.** t = 47.8 s: **identical `Gold: 989` and `14 FPS · 70.4 ms`**, both characters reduced to ghosts, contrast 3.38, featureless 92.9 %. ≤150 uu of possible travel between these two.

⚠️ **FR-§6:** these six PNGs **and this report** need a **named host row** on the board, or they sit in the tree unclaimed (the failure recorded for VID-005).

---

## Suspected mechanism — ⚠️ HYPOTHESIS, NOT VERDICT

⛔ I did not open the editor, any material, any Blueprint, or any MCP surface (FR-§0.3). Every asset claim below is from a **read-only directory listing** plus `FogVolume.h`'s own prose — **never from the asset itself.** These are candidates ranked by what the pixels support, not a diagnosis.

- **C1 — the vendor fog material's density is noise-driven, and the forced non-uniform scale may be stretching that noise.** `BP_SiegeFog`'s parent is the vendor `BP_FogArea_C` (`FogVolume.h`), and `Content/FogArea/` ships `Data/S_FogAreaNoise.uasset`, `Materials/M_FogArea.uasset` and volume-noise textures `T_Volume_Curl_01`, `T_Volume_Noises_01`, `VT_Curl_Low`, `VT_Noises`. A noise-modulated density field is *literally* "scattered fog". The actor is force-scaled to **(640, 360, 260)** (`FogVolume.cpp:504 RefreshFogVisual` → `SpawnFogVisual`; ACHIEVED value logged live this session) — **if** the material samples noise in object/local space, that scale stretches the noise cells by 640×/360×/260× and turns fine mist into arena-scale dense and clear blobs. — **Confidence: MEDIUM-HIGH that the density field is noisy** (it is what the pack is built to do and it matches the picture); **the object-space-UV half is UNVERIFIED.** **Confirms it:** art-director opens `M_FogArea` + `BP_SiegeFog`'s `S_FogAreaNoise` and reports whether density noise is enabled, its tiling/contrast, and whether its UVs are world- or local-space.
- **C2 — the noise pans, so density varies with TIME at a fixed point.** This is the only candidate that comfortably explains a step at ≤150 uu of travel. Same confirming test: is the noise animated, and at what speed? — **Confidence: MEDIUM.**
- **C3 — volumetric-fog temporal reprojection starved at 8–17 FPS** (`HistoryMissSupersampleCount:4`, grid 8 px / 128 Z). ⚠️ **PARTIALLY REFUTED ON PIXELS:** the 47.6/47.8 pair carries the **same crop-verified `14 FPS · 70.4 ms`** on both sides of the step, and pole B's clearest frame (t = 43.0) runs at **8 FPS** while pole A (t = 20.0) runs at **11 FPS** — the correlation runs the *wrong way*. Framerate alone is not sufficient. **Not fully excluded** (history misses track camera *motion*, not fps). — **Confidence: LOW as a primary cause.**
- **C4 — VRAM exhaustion degrading the fog's VIRTUAL textures.** The banner is crop-verified at **2798.546 MB over budget** in every frame; two of the pack's noise sources (`VT_Curl_Low`, `VT_Noises`) are virtual textures, and VT tile streaming under a 2.8 GB overdraft at 8–17 FPS feedback resolves and evicts **in steps**. — **Confidence: LOW-MEDIUM, flagged as such.** **Confirms/falsifies it cheaply:** re-record with headroom (lower resolution or texture quality) and see whether the steps survive. ⛔ Per the standing lane law, **no asset may be shrunk and no `.ini` edited until a VRAM baseline is measured.**
- **C5 — height falloff / far-field mismatch.** `r.SupportExpFogMatchesVolumetricFog:0` was applied this session, so the non-volumetric height fog is **not** matched to the volumetric fog beyond `VolumetricFogDistance` (6000 uu, transcribed in `FogVolume.h`). Candidate for the *far-field* look only — ⛔ **it cannot explain extinction of an object at 400 uu.** — **Confidence: LOW for these symptoms.**
- **C6 — lighting: considered and NOT supported by the pixels.** Scene mean luma **rises** with the fog (102 → 176) while contrast falls; a lighting failure would not raise luminance uniformly while erasing texture. The sun is stable — a sharp, consistently-directional cast shadow is present in every low-fog frame (3.1, 43.0, 47.6). **I found no pixel evidence for a lighting cause**, and record that explicitly because the dispatch asked.

---

## Routing recommendation

| finding | lane | suggested fix one-liner (hypothesis) |
|---|---|---|
| ⭐ The target is now a number: **~650 ± 50 uu** of ground visibility | manager → art-director | Tune the fog to hold **600–700 uu**; the game already names that distance as `FogVisionCeilingUU = 609.6 uu`, so visual and gameplay reach would finally agree |
| ≥4× spread; 30 % of the clip is a whiteout | art-director | Inspect `BP_SiegeFog` → `S_FogAreaNoise` + `M_FogArea`: reduce density-noise **contrast/amplitude**, and check the noise **UV space** against the forced `(640, 360, 260)` scale |
| Step change in ≤0.2 s / ≤150 uu | art-director + gameplay-programmer | If the noise pans, slow or disable the pan; verify whether density is object-space-scaled by `RefreshFogVisual`'s forced scale (`FogVolume.cpp:504`) |
| Hero invisible at 400 uu (pole A) | art-director | Whatever the density fix, add a **floor on near-camera clarity** so the player's own body can never be extinguished |
| Health bars render at full strength over invisible units | gameplay-programmer | Bars are **fixed screen size** and unfogged — verified (bar widths ~120 px for a near unit and a castle-distance unit alike at t = 3.1). Decide whether they should attenuate with the fog |
| `2798.546 MB over budget`, every frame | build-master | **Measure a VRAM baseline first** — no asset shrunk, no `.ini` edited before that. Also the control run that falsifies C4 |
| "regardless of where the player walks" — is the fog *meant* to be spatially uniform? | **needs-Jonathan** | The card is world-global; a uniform-density fog is a design ruling, not a bug fix. One sentence from him settles whether *any* spatial variation is wanted |
| Host row for 6 PNGs + this report | build-master | FR-§6 — promoted evidence and the report both need a **named** host row |

---

## Not examined / limitations this pass

- **No audio** (FR-§5).
- **Image Reads used: 15 of the ~40 budget** (4 sheets, 8 full frames, 3 crops). Under budget; the numeric sweep did the heavy lifting without spending Reads.
- **Sampling:** 1.0 s across the whole clip; 0.2 s **only** at 25.4–28.0 s and 46.6–48.6 s. The 1 s data shows equally sharp transitions at **25→26, 30→31, 52→53, 63→64, 72→73** that were **NOT** resolved at 0.2 s. `--run` (single-frame flicker) was **not** used at all — a one-frame fog pop between samples would be invisible to this pass.
- **Absolute map position: not established.** See the section above; the terrain/crest correlation is selection-biased and is flagged as a hypothesis, not a finding.
- **The "units fade with distance" half of his reference is only partly measured.** At t = 27.0 exactly one unit is visible and a second unit's health bar floats with no visible body, so the fade boundary at that instant sits between them — but I could **not** convert that to a distance, because the health bars are **fixed screen size** and therefore not a distance ruler. My 650 uu is measured on **ground texture**, not on unit silhouettes.
- **Geometry assumptions:** FOV 90° is the engine default and no `FieldOfView` assignment exists anywhere in `Source/` — but a Blueprint could override it and I cannot open Blueprints. The 4 % hero-height agreement is the evidence that it is not overridden. Camera pitch was measured at 11.5° in frames where the hero is visible and **assumed constant** for the whiteout frames, where he is not; for those the figure is reported as a censored bound (`D < 243 uu`) rather than a value, which does not depend on the assumption.
- **VFR + 4–17 FPS:** the `--at` seeks are exact but the game's own frame cadence is coarser than the sampling in places; "≤0.2 s" for the step means "within one sampling interval, ≈2 rendered frames".
- **Capture compression** (h264, 156 MB / 75 s) can flatten low-contrast detail. This biases the *featureless %* **upward** in dark or low-contrast regions — i.e. it makes the fog look slightly worse than it is. The **relative** comparison between the three moments is unaffected because all were measured identically from the same encode; the fog-free control inside the same file bounds the effect at **2.9 %**.
- ⛔ **No code, asset, editor, MCP or git action was taken** (FR-§0.3). TASKBOARD untouched — the manager boards all fixes.
