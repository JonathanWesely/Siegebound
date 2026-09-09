# TASK-1160 — [FOGEXPLOIT-CAPTURE] — art-director handoff

# ⭐ THE VERDICT, IN ONE SENTENCE

🚨 **THE EXPLOIT IS REAL AND IT IS DEMONSTRATED ON PIXELS: with a Fog card's `BP_SiegeFog` up and Shadows set to Low, the card's fog does NOT render — the frame becomes, to within 0.2 %, the frame with no fog actor in the world at all.**

And the mechanism is isolated to **one cvar**: `r.VolumetricFog`. Shadows is only the delivery route.

> ⚖️ **The correction this forces on our own record:** `TASK-1147`'s floor pins exactly `r.VolumetricFog` — so **the floor IS the fix for 🧑 Jonathan's complaint**, not merely "defensive depth over a different fog". The comment shipped in the uncommitted `FogVolume.cpp` — *"THIS FLOOR IS CORRECT FOR THE AMBIENT FOG AND **DOES NOT REACH THE CARD'S FOG**"* — is now **measured FALSE**. See §4; it must be corrected before `TASK-1149` compiles.

**DIAGNOSE/CAPTURE ONLY. ZERO WRITES EXECUTED.** One editor session, PID 14188, MCP `http://127.0.0.1:8000/mcp` answered. Date 2026-09-08.

---

## 0. FENCE REPORT — verify me first

| Fence | Result |
|---|---|
| `L_Arena.umap` SHA-256 **BEFORE** | `1F78419D888940739C949A60B29EE43451C464915F7AB37942B921FE0AF15622` |
| `L_Arena.umap` SHA-256 **AFTER** | `1F78419D888940739C949A60B29EE43451C464915F7AB37942B921FE0AF15622` — **IDENTICAL**; mtime still `2026-09-05 00:42:58` |
| `git status --porcelain Content/` | **empty**, before and after — zero `.uasset` / `.umap` touched |
| Editor dirty packages at teardown | `get_dirty_content_packages()` = **[]** · `get_dirty_map_packages()` = **[]** ⇒ **no save prompt was ever raised, so none had to be declined** |
| `BP_SiegeFog.uasset` | untouched, mtime still `2026-09-05 02:37:01`, 37,947 bytes |
| `set_properties` / `save_actor` / `save_assets` / material recompile / import / commit / push | **never called** (`TASK-239` respected) |
| `Content/FogArea/**` edited · `BP_FogArea` opened | **NO** · **NO** — no asset editor was opened at any point |
| `Source/**` · `Config/**` | **zero** writes |
| PIE/Simulate | `IsPIERunning` = **false** before I started; I started **one** Simulate session and **I stopped it**. None was running that I did not start. |
| Viewport camera | moved for the capture, **restored** to its entry pose `(-20607.816, -0, 98.15)` / `(0, 180, 0)` |
| cvars | **restored** — see §6. ⚠️ **One residue, and it matters: read §6 before anyone re-tests.** |

**Writes actually made:** this file, five promoted PNGs, and one `- status:` line on the board. Nothing else.

---

## 1. 🚨 THE BINARY'S FLOOR STATE — established, not assumed

**`TASK-1147`'s floor is NOT in the running binary. This is therefore a clean PRE-floor measurement and NO DEFEAT WAS NEEDED.**

Established three independent ways, because this is exactly the shape the project keeps getting burned by:

| # | evidence | reading |
|---|---|---|
| 1 | `git log -1` = **`61702e14b0a774553599bb580a9b583130a849e9`** (`TASK-1124`). `git status`: `Source/…/Siegebound/FogVolume.cpp` and `.h` are **modified and UNCOMMITTED** (+801 lines) | the floor exists only in the working tree |
| 2 | ⭐ **The decisive one — timestamps.** `Binaries/Win64/UnrealEditor-GitClaudeUnrealTest.dll` built **`2026-09-08 01:32:13`**. Editor PID 14188 `StartTime` = **`2026-09-08 01:36:07`** ⇒ it loaded that DLL. `FogVolume.cpp` last written **`2026-09-08 14:15:26`** — **12 h 43 m AFTER the DLL was compiled.** No `*.patch_*.dll` exists in `Binaries/Win64` ⇒ **no Live Coding patch could have carried it in.** | the loaded code **cannot** contain the floor |
| 3 | ⭐⭐ **The runtime behavioural test, which is the one that actually proves it** (`SC-§94` cl. B): I set `sg.ShadowQuality 0` and **read `r.VolumetricFog` back. It read `0`.** A post-floor binary would have **refused** that scalability write (`CanChange`: `NewPri >= OldPri`, our layer at `ECVF_SetByCode`) and left it at `1`. | the write **landed** ⇒ **no floor was holding it** |

⛔ **A fourth probe that I ran and am DISCARDING as blind, recorded so nobody counts it as agreement** (`SC-§96`): I asked Python whether `unreal.FogVolume` exposes any floor symbol. It returned `[]`. **That is blindness, not absence** — the positive control is that `dir(unreal.FogVolume)` returns **219 members and every single one is inherited from `AActor`**; the class exposes **zero** project functions of any kind. A reader that returns the same empty list for everything cannot testify to anything.

⇒ ⚖️ **Achieved `r.VolumetricFog` at Shadows=Low was `0` by the ordinary scalability path. I never typed `r.VolumetricFog 0` to obtain the exploit condition. The defeat instruction was read, was ready, and was not needed.**

---

## 2. THE INSTRUMENT — and its positive control

- **Mode: Simulate-in-Editor** (`SC-§88a`), `bSimulate=true`, `PlayMode_Simulate`, 8 s warmup. Game world = `/Game/Maps/UEDPIE_0_L_Arena.L_Arena`, 155 actors, both castles, torches, gold nodes, and the runtime scatter present.
- **Capture: `HighResShot 1920x1080`** on the Simulate viewport — the real scene render, written to `Saved/Screenshots/WindowsEditor/` (git-ignored), then copied out. Chosen over `CaptureViewport` because it lands on disk as pixels I can measure numerically.
- **Vantage, held byte-identical across every frame:** `(-18000, 0, 176)`, pitch `-11.5°`, yaw `0`, HFOV 90°, 1920×1080. Derived from the game's own geometry, not chosen by eye: `PlayerStart_0` reads `(-23800, 0, 100)` and the hero rides a `400 uu` boom, so `176 uu` eye height and `11.5°` down is `VID-007`'s own validated pinhole camera, placed out in open field.
- **How the fog got up:** the editor spawner **refuses** during Simulate, so I used the console `summon BP_SiegeFog_C`, which spawns into the **game** world, then reproduced `AFogVolume::FogVisualTransform` exactly.
  - ⭐ **The `TASK-1074` trap reproduced live, unprompted:** the actor arrived at scale **`(20, 20, 5)`** — the vendor template's substitution, character-for-character the value named in `05c3fc0`. Corrected with `SetActorScale3D` **after** the spawn, exactly as `SpawnFogVisual` does.
  - **Read back (`SC-§94` cl. B):** ACHIEVED loc `(0, 0, 7000)` · ACHIEVED scale `(640.0, 360.0, 260.0)` · bounds extent `(32000, 18000, 13000)` ⇒ box `64,000 × 36,000 × 26,000 uu`, X span `−32,000…+32,000`, **Z span `−6,000…+20,000`** — matching `TASK-1151`'s derivation exactly.

### ✅ POSITIVE CONTROL (`SC-§39`) — PASSED, and taken BEFORE the series

Same vantage, same session, Shadows at the session baseline; the only difference is whether `BP_SiegeFog` exists:

| | far-field RGB | contrast RMS | featureless | ground visibility D |
|---|---|---|---|---|
| `BP_SiegeFog` **ABSENT** | (97.7, 99.1, 55.6) | **19.21** | **2.5 %** | no collapse — castle legible at ~43,000 uu |
| `BP_SiegeFog` **PRESENT** | (193.6, 182.1, 164.7) | **0.40** | **100 %** | **241 uu, censored** (whiteout) |

⇒ **My instrument demonstrably sees this fog appear and disappear at this exact vantage.** A null result at Low would therefore have meant something. (It did not come out null.)

⭐ This also independently re-measures `TASK-1151`'s §3 on **my own** pixels: with the `ExponentialHeightFog` live and `BP_SiegeFog` absent, at Shadows=**High** with `r.VolumetricFog = 1`, the enemy castle is **crisp at ~43,000 uu**. The ambient system's rendered contribution at the player's vantage is ≈ 0. **The wash is `BP_SiegeFog` and nothing else.**

---

## 3. ⭐ THE TWO SERIES — statistic, stationarity, magnitude

**Channel statistic is `TASK-1151`'s**, so the two runs are comparable by the same method: per-channel RGB in a **far-field** band (scene rows 25–40 %) and a **near-ground** band (rows 80–95 %), plus R/G and B/G. Contrast RMS / featureless % / ground-visibility `D` are **`VID-007`'s**, same formulas, same thresholds (`f = 960 px` for 1920 wide at HFOV 90 — the same `f = W/2` `VID-007` used). ⚠️ One declared difference: my frames carry **no HUD**, but I kept `VID-007`'s scene region (drop top 10 % / bottom 9 %) unchanged so the numbers stay comparable.

🚨 **The blocks were INTERLEAVED — Epic, Low, Epic, Low, 3 frames each — precisely because `TASK-1151` measured that this fog PANS on a timer** (`wind Speed 1`, world-space, 2 `Time` nodes). A single frame per setting, or two un-interleaved blocks, could not separate a setting effect from the pan.

| block | `sg.ShadowQuality` | achieved `r.VolumetricFog` | far-field RGB | far B/G | contrast RMS series | featureless | D (uu) |
|---|---|---|---|---|---|---|---|
| **1 EPIC** | 3 | **1** | (193.4, 182.1, 164.7) ×3 | 0.905 | 0.67 / 0.51 / 0.40 | 99.5 / 100 / 100 % | 241 **censored** |
| **2 LOW** | 0 | **0** | (103.9,104.1,57.6) (103.7,103.8,57.5) (103.8,103.9,57.4) | 0.554 | 19.50 / 19.53 / 19.53 | 2.4 / 2.4 / 2.4 % | **none — no collapse** |
| **3 EPIC** | 3 | **1** | (192.9, 181.1, 164.0) ×3 | 0.905 | 9.35 / 9.87 / 10.22 | 70.9 / 67.4 / 66.3 % | 464 / 408 / 381 |
| **4 LOW** | 0 | **0** | (103.8,103.9,57.6) (104.0,104.1,57.6) (103.9,104.1,57.6) | 0.554 | 19.50 / 19.52 / 19.53 | 2.4 / 2.4 / 2.4 % | **none — no collapse** |

### Stationarity (`SC-§88a` cl. 6 — the right test in Simulate is *stationary series + an error bar*, not convergence to zero)

- **Far field, LOW (n = 6):** R spans `103.7 … 104.1` ⇒ spread **0.4 / 255 = 0.39 %**. Contrast spans `19.50 … 19.53` ⇒ **0.15 %**. **No trend, no drift.** Stationary.
- **Far field, EPIC (n = 6):** R spans `192.9 … 193.4` ⇒ spread **0.5 / 255 = 0.26 %.** Stationary.
- **Near field, EPIC: NOT stationary, and that is the signal, not noise.** Block 1 is a whiteout (contrast 0.5, featureless 100 %) and block 3 is a partial fog (contrast 9.8, featureless 68 %) — **the fog thinned between the two Epic blocks while the camera never moved.** That is `VID-007`'s 4× swing and `TASK-1151`'s wind-pan, reproduced in Simulate.
- ⭐ **And it independently corroborates `TASK-1151`'s central finding:** across the whole Epic swing the **far field is saturated-identical** (193.4 vs 192.9, **0.26 %**) while the near field moves from whiteout to D = 464 uu. `TASK-1151` measured 3.9 % across `VID-007`'s frames; mine is tighter. **One fog whose near-field extinction swings — not two behaviours.**

### ⇒ Between-condition separation vs within-condition spread

**Far-field R: Epic `193.15 ± 0.26`, Low `103.85 ± 0.13`. Difference `89.3 / 255 = 35.0 % of full scale` — roughly `340 ×` the within-condition spread.** This is nowhere near the noise floor `SC-§88` warns about; the two Low blocks, taken ~2 minutes apart with an Epic block between them, agree to **three significant figures.**

### The magnitude, in `VID-007`'s own units

- 🧑 His reference (27 s mark): ground legible to **≈ 650 ± 50 uu**; the game's own `FogVisionCeilingUU = 609.6`.
- **At Shadows = EPIC:** `D = 241 uu` (censored — collapse at the nearest sampled row, i.e. *worse* than `VID-007`'s pole A of `< 243 uu`) in block 1, and `381–464 uu` in block 3. Both **at or below** his target.
- **At Shadows = LOW:** ⛔ **no contrast collapse anywhere in the scene band.** `D` is unbounded within the frame — the treeline, the mown grass stripes, individual grass blades and the **enemy castle at ~43,000 uu** are all legible.

⇒ **Visible distance goes from ≤ 464 uu to ≥ 43,000 uu — a lower bound of ≈ 93 ×** — while the fog's *mechanical* penalty stays at `FogVisionCeilingUU = 609.6` and is server-authoritative and identical for both players (`TASK-1146`, measured). **That is the `GFX-§12` asymmetry, demonstrated rather than inferred.**

⇒ ⚖️ **The cleanest single sentence: at Shadows=Low the fogged frame IS the unfogged frame.** Low far-field `(103.9, 104.1, 57.6)` vs the fog-absent control `(97.7, 99.1, 55.6)`; the residual ~6 units of luma is **shadows being off** (`r.ShadowQuality` 5 → 0), not residual fog — and it is ~15× smaller than the ~89-unit fog effect.

---

## 4. 🚨⭐⭐⭐ THE MECHANISM — a 2×2 that isolates it to ONE cvar

`sg.ShadowQuality 0` writes a *whole group*. So I broke the confound apart, holding the vantage and the actor fixed:

| # | Shadows | `r.VolumetricFog` (achieved) | far-field RGB | contrast | featureless | fog? |
|---|---|---|---|---|---|---|
| **A** | EPIC (3) | 1 | (192.9, 181.1, 164.0) | 9.35 | 70.9 % | ✅ **UP** |
| **B** | LOW (0) | 0 | (103.8, 103.9, 57.6) | 19.50 | 2.4 % | ⛔ **GONE** |
| **C** | **EPIC (3)** | **0** ← forced by hand | **(97.6, 99.3, 55.4)** | **19.33** | **2.4 %** | ⛔ **GONE** |
| **D** | **LOW (0)** | **1** ← forced by hand | **(192.9, 181.1, 164.0)** | **9.66** | **68.0 %** | ✅ **UP** |

- **C holds Shadows at EPIC and the card's fog still vanishes.**
- **D holds Shadows at LOW and the card's fog comes straight back.**

⇒ 🚨 **The effect tracks `r.VolumetricFog` and is INDEPENDENT of `sg.ShadowQuality`. Shadows matters only because `[ShadowQuality@0]`/`@1` set `r.VolumetricFog = 0` (`BaseScalability.ini:143` / `:178`).**

### ⭐⭐⭐ What this does to `TASK-1147` / `TASK-1149` — read before that compile

**Row D is a hand-simulation of `TASK-1147`'s floor** — `r.VolumetricFog` forced to `1` while the player's Shadows setting sits at Low — **and it restores the fog completely.** The floor pins exactly `r.VolumetricFog` plus the two grid axes at `ECVF_SetByCode`.

⇒ ✅ **`TASK-1147`'s floor is the correct AND sufficient fix for the exploit 🧑 Jonathan reported.** It is not "defensive depth over a different fog".

⇒ ⛔ **Two things in our own record are now measured FALSE and must be corrected before `TASK-1149` ships:**
1. The comment in the uncommitted `FogVolume.cpp` — *"THIS FLOOR IS CORRECT FOR THE AMBIENT FOG AND **DOES NOT REACH THE CARD'S FOG**"*. It reaches it. It is the whole fix.
2. `GFX-§9`'s 2026-09-08 correction, which rules the exploit **"UNEXPLAINED — a third state"** and says the floor is *"DEFENSIVE DEPTH OVER A REAL VIOLATION, not the fix for his complaint"*. The third state is now resolved to **DEMONSTRATED**. (Everything else in that correction stands, including that 🧑 he found it and that `GFX-§12` holds on its own merits.)

⚠️ **What I did NOT establish is the material-level *why*.** I measured **that** `r.VolumetricFog 0` deletes this translucent raymarched mesh; I did not open the material to find the coupling. `TASK-1147`'s own header comment names the likely candidate — **`bUsedWithVolumetricFog`** on `M_FogArea` — and `TASK-1152`/`TASK-1149` should confirm it at source. `TASK-1147`'s scan was not wrong: it looked for `DetailMode` / `QualitySwitch` / draw-distance routes and correctly found none. **The route was a fourth kind it did not enumerate.**

---

## 5. ⛔ WHAT REMAINS UNTESTED — said plainly (`SC-§94`)

1. ⛔ **`r.SceneColorFormat` and `r.TranslucencyLightingVolume` were NEVER MOVED, so they are individually UNTESTED.** I ran `sg.EffectsQuality 0` (exploratory, cl. 4) and the fog was **unchanged**: far `(193.5, 182.2, 165.1)`, featureless 96.9–100 %. But the cvar read-back shows `[EffectsQuality@0]` in *this project's config* left `r.SceneColorFormat` at `3` and `r.TranslucencyLightingVolume` at `1` — **identical to baseline**. ⇒ the **Effects GROUP is refuted as a route**; **two of `GFX-§9`'s three named candidates are not.** Reporting them as refuted would be exactly the `SC-§96` error.
2. ⛔ **`sg.TextureQuality 0` — tested, fog unchanged** (far `(193.4, 182.0, 164.6)`, featureless 99.9 %). ⇒ the third candidate, `sg.TextureQuality` vs the virtual `VT_Noises`, **is refuted** as a route. (It did move `r.DetailMode` to 3 as a side effect; still no change.)
3. ⛔ **Shadows = MEDIUM (`sg.ShadowQuality 1`) not tested.** `BaseScalability.ini:178` sets `r.VolumetricFog=0` there too, so I expect it identical — but expectation is not measurement.
4. ⛔ **The Graphics MENU path was not exercised.** I drove `sg.ShadowQuality` at the console; 🧑 he uses the shipped Settings → Graphics slider, which goes through `GameUserSettings`. MCP has no input lane, so whether the menu produces the same `r.VolumetricFog` write is **untested here**. This is the one thing `TASK-1159` cl. 1 can settle that I cannot.
5. ⛔ **Editor only.** No packaged/Shipping build was measured.
6. ⛔ **No hero pawn.** Simulate spawns none, so `VID-007`'s "his own character is extinguished at 400 uu" was not reproduced. My subject is the wash.
7. ⛔ **No frame-time measurement of any kind**, so nothing here says what the floor costs.
8. 🧑 ⛔ **cl. (5): I did not see an answer from Jonathan on which fog he saw disappear.** If one has landed since, compare it to §3 — my frames say **the card's fog**, unambiguously, and would **agree** with him if that is what he reported.

### Declared deviations (each one held constant across the comparison, so none can manufacture the result)

- **The summoned actor's root `Mesh` component was `Static`; I set it `MOVABLE`** to place the box at `(0,0,7000)` (a Static component refuses a runtime move). It is a **PIE-world instance** — the asset is untouched (0 dirty packages; `.uasset` mtime unchanged). ⚠️ It was **identical in all four 2×2 cells**, so it cannot produce or hide the Low/Epic difference.
- **The fog was `summon`ed, not played as a card.** Transform, scale and bounds were read back and match `AFogVolume::FogVisualTransform` exactly; the BP's own density parameters are its defaults, which is what the card spawns.
- **Grid quality differs by setting, as the engine intends:** baseline/High = `16/64`, Epic = `8/128` (the recording behind `VID-007` also ran `8/128`). Quality, not density — declared.

---

## 6. ⚠️🚨 CVARS RESTORED — AND ONE RESIDUE THAT WILL FAKE A "NO EXPLOIT"

**Restored, verified by read-back:** `sg.ShadowQuality 2` · `sg.EffectsQuality 2` · `sg.TextureQuality 2` · `r.VolumetricFog 1` · grid back to `16/64` — **identical to the session baseline I recorded on entry.** Every other `sg.*` group still reads `2`.

🚨 **BUT — and this is the single most important operational line in this file:**

Because I typed `r.VolumetricFog 1` at the console to restore it, that cvar now sits at **`ECVF_SetByConsole`**, which **outranks scalability for the rest of this editor process.** I **measured** this rather than assuming it:

```
before                                r.VolumetricFog=1  sg.ShadowQuality=2
after  sg.ShadowQuality 0             r.VolumetricFog=1  sg.ShadowQuality=0   ← DID NOT DROP
after  sg.ShadowQuality 2 (restore)   r.VolumetricFog=1  sg.ShadowQuality=2
```

⇒ ⛔ **In editor PID 14188, `sg.ShadowQuality 0` can no longer drive `r.VolumetricFog` to 0. Anyone who re-runs this test in THIS editor session — including 🧑 Jonathan on `TASK-1159` cl. 1 — will read a FALSE *"no exploit"*, whether or not one exists.**

✅ **THE FIX IS ONE LINE: RESTART THE EDITOR before re-testing.** A fresh process drops the console layer and `sg.ShadowQuality 0` reaches `r.VolumetricFog` again.

⚖️ *This is exactly the confound cl. (0) predicted a post-floor binary would create — and my own restore write created it by hand, in a pre-floor binary, in the same session. It is also, incidentally, a live proof that the floor's chosen mechanism works: a higher-priority write does make `sg.ShadowQuality 0` unable to move the cvar.*

---

## 7. EVIDENCE — every file, named

All under `.claude/pipeline/playtest-evidence/2026-09-08/`:

| file | what it shows |
|---|---|
| `TASK-1160-shadows-epic.png` | Shadows **EPIC**, achieved `r.VolumetricFog 1`, fog card up. far (193, 181, 164) · B/G 0.905 · contrast 9.35 · featureless 70.9 % · **D = 464 uu** |
| `TASK-1160-shadows-low.png` | Shadows **LOW**, achieved `r.VolumetricFog 0`, **same actor still in the world**. far (104, 104, 58) · B/G 0.554 · contrast 19.50 · featureless 2.4 % · **no collapse anywhere** |
| `TASK-1160-shadows-low-vs-epic-side-by-side.png` | the two above at one glance, captioned with the statistic |
| ⭐ `TASK-1160-mechanism-2x2-r-volumetricfog.png` | **the most valuable frame here** — the §4 2×2. Cell C holds Shadows at EPIC and still deletes the fog; cell D holds Shadows at LOW and brings it back |
| `TASK-1160-positive-control-fog-absent-vs-present.png` | `SC-§39` — `BP_SiegeFog` absent vs present at the identical vantage, taken **before** the series |

Camera on every frame: `(-18000, 0, 176)` pitch `-11.5` yaw `0`, HFOV 90°, 1920×1080, **Simulate-in-Editor (`bSimulate=true`)** — stated per `SC-§88a` cl. 1 so no promoted PNG here is an unlabelled instrument.

---

## 8. WHAT FIRES DOWNSTREAM

| row | effect of this measurement |
|---|---|
| ⭐ `TASK-1149` (the ship) | ✅ **UNBLOCKED — the before-picture now exists.** ⚠️ It must also carry the §4 correction: the floor **is** the fix, and the `FogVolume.cpp` comment saying it *"does not reach the card's fog"* is measured false. |
| ⭐ `TASK-1147` | ✅ **Vindicated and PROMOTED** from defensive depth to *the* fix. Its scan was not wrong — the route was a kind it did not enumerate. Confirm `bUsedWithVolumetricFog` at source. |
| ⭐ `TASK-1159` (🧑 his walk) | ⚠️ cl. 1 is still worth doing **through the menu**, which I could not reach (§5.4) — but ⛔ **he must be on a RESTARTED editor** (§6) or the console pin gives him a false pass. |
| ⭐ `GFX-§9` 2026-09-08 correction | ⛔ *"UNEXPLAINED — a third state"* is **superseded: DEMONSTRATED.** The rest of that correction stands. |
| ⭐ `TASK-1152` / `TASK-1154` | unaffected — they tune density and colour. This is the last clean look at the shipped fog before they change it. |

**Law honoured:** `GFX-§9` (2026-09-08) · `GFX-§11` · `GFX-§12` · `FOG-§12.1` · `FOG-§12.5` · `FOG-§12.6` · `SC-§39` · `SC-§88` · `SC-§88a` (cl. 1, cl. 6) · `SC-§90` · `SC-§94` (cl. B) · `SC-§96` · `SC-§102`.
