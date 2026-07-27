# TASK-337 — [CASTLE-crumble-tune] BLOCKED: the band is unreachable by MI scalars — `M_CastleCrumble` FAILS TO COMPILE and every crumble stage renders the ENGINE DEFAULT MATERIAL

> **⚠️ SUPERSEDED 2026-07-27 (same day): the BLOCKED return below is the FIRST run's finding and led to the re-adjudicated chain TASK-339 → 337 → 338. The MEASUREMENT-FIRST RE-RUN (§ "RE-RUN" at the bottom of this file) executed after the TASK-339 sampler fix: the shipped values FAIL the band on S1/S2 (too BRIGHT — the opposite of the withdrawn "ScorchAmount dominates" direction), the conditional retune RAN and PASSES all five criteria. Final shipped values: MI01 `Darken 0.38 / ScorchAmount 0.18`, MI02 `Darken 0.24 / ScorchAmount 0.52`, MI03 untouched. §§ below are the first run's record, kept verbatim.**

**Status: blocked — returned to orchestrator/manager for re-adjudication.** Date: 2026-07-27 · art-director.
**Nothing saved. No MI value shipped. No Git. No board edit. Simulate STOPPED. `L_Arena` untouched (no save prompt answered).**

## The one-line finding

`M_CastleCrumble` has failed shader translation since TASK-330's same-path texture reimport — **compile error, read off the material editor Stats panel:**

> `SM6 (Node TextureSampleParameter2D) Sampler Type is Linear Color, should be Masks for /Game/Textures/T_Castle_ORM`

so **every crumble stage in game renders the engine DEFAULT MATERIAL** (the grey mottle TASK-331's close-up interpreted as "char/speckle"). The three MIs' scalar values never reach the screen at all. The TASK-331 defect table (P 77.5 → 28.0/28.9/29.2 "flat stages") is three measurements **of the same Default Material render**, and the adjudication's premise — "the root cause is arithmetic (stage params vs the brighter base)" — is **falsified by measurement**. No `Darken`/`ScorchAmount` value can move a material that is not rendering. Capture: `TASK-337-M_CastleCrumble-compile-error.png`.

## Root-cause chain (each link verified this session)

1. `T_Castle_ORM` ships `TC_Masks` + sRGB off (correct per the M7 fleet import law; set by TASK-330's same-path reimport).
2. `M_CastleCrumble`'s ORM node (`TextureSampleParameter2D_2`, param `ORM`) was authored at TASK-157 with `SAMPLERTYPE_LinearColor` **and `T_Castle_ORM` assigned as the node default**. UE validates the node-assigned texture at translation: `TC_Masks` demands `SAMPLERTYPE_Masks` → hard SM6 error → whole material + all three MIs fall back to Default Material in game. Log (repeats on every load/compile attempt): `LogMaterial: Warning: [AssetLog] ...M_CastleCrumble.uasset: Failed to compile Material for platform PCD3D_SM6, Default Material will be used in game.` (21:15:38, 22:22:27, 22:27:42, 22:30:22 this session; also fires as `Failed to compile Material Instance with Base M_CastleCrumble` per MI).
3. **Why `M_AssetPBR`/`MI_Castle_PBR` are fine with the same texture:** `M_AssetPBR`'s ORM node default is `T_AssetPBR_NeutralORM` = `TC_Default` (verified) → LinearColor is valid → master compiles; MI-level texture overrides (the fleet's `T_*_ORM` masks textures) are NOT re-validated. Only `M_CastleCrumble` embeds `T_Castle_ORM` at the node.
4. At TASK-157 the OLD castle ORM predated the TC_Masks import law, so the material compiled then — the crumble stages Jonathan approved were real. The TASK-330 reimport silently broke it; TASK-331 was the first in-level render since.

## Measurements (TASK-331 protocol, reproduced and validated first)

Method validation: my region/formula reproduces TASK-331's own PNGs to ~1-4% (their captures re-measured: P 81.5 / S1 27.0 / S2 28.4 / S3 28.9 vs recorded 77.5 / 28.0 / 28.9 / 29.2; grass 12.6-13.7 vs 11.4-11.6) — formula = **mean Rec.709 luma on 8-bit sRGB, 0-255**. My session: Simulate on `L_Arena`, ONE castle (Castle_0, found by class + `Team==Blue`, never by label) driven 2000→1480→980→480 HP by instigator-less `apply_damage` (each stage fired once, in order, correct mesh+MI readbacks). Frozen camera pose (loc −23200, −1300, 550 / rot pitch −4.4, yaw 144.2), sun-lit east wall region (800,980,615,695 @1920×1080), grass control (300,650,850,1000). Exposure-consistent: grass 149.4–149.8 across ALL captures.

| State | Wall luma | Grass | ×P | Band | Capture |
|---|---|---|---|---|---|
| P pristine (`SM_Castle`, TeamBlue+PBR) | **173.2** | 149.6 | 1.000 | — | `TASK-337-P-pristine.png` |
| S1 shipped (01: Darken .80/Scorch .12) | **89.4** | 149.5 | **0.516** | FAIL (needs 0.55–0.80) | `TASK-337-S1-shipped-defaultmaterial.png` |
| S2 shipped (02: .50/.45) | **89.4** | 149.4 | **0.516** | in 0.35–0.55 but S1−S2 = 0.00 → FAIL | `TASK-337-S2-shipped-defaultmaterial.png` |
| S3 shipped (03: .28/.82) | **89.3** | 149.5 | **0.516** | FAIL ≤0.40 — **TASK-331's "stage 3 PASSES 0.38×P" is VOID** (it measured Default Material at dusk exposure) | `TASK-337-S3-shipped-defaultmaterial.png` |
| **Probe** MI01 Darken **1.0** / Scorch **0.0** (ceiling) | **90.3** | 149.5 | **0.521** | scalar ceiling < 0.55 floor ⇒ **band unreachable** | `TASK-337-probe-darken1-scorch0.png` |
| **Cross-test** `SM_Castle_Crumble01` + `MI_Castle_PBR` both slots (transient, Simulate-world only) | **165.8** | 149.7 | **0.957** | — | `TASK-337-xtest-pbr-on-crumblemesh.png` |

Reading the table: three different MIs render bit-identically (89.3–89.4); the extreme-value probe moves the wall <1 luma; the same crumble MESH with a compiling material renders ≈ pristine. Mesh/UVs exonerated (TASK-331's derivation genuinely closed); the master material is the sole defect. Also ruled out en route: LOD chain (`r.ForceLOD 0` no change), hit-flash overlay (None), MI texture overrides (all same-path correct), metallic/ORM content (B channel ≈ 0), UV tiling (all samplers on default UV0), `MP_FrontMaterial` (unwired). `RecompileShaders Material M_CastleCrumble` re-fails in 46 ms with the same error — it is deterministic, not a stale cache.

## Visual read (observation)

Stage 1/2/3 all render as an identical dense grey-black sparkle-mottle (the Default Material pattern) — no scorch tinting, no stage progression, nothing of TASK-157's intent on screen. Pristine and the cross-test render the bright rebuilt castle correctly.

## The fix is a ONE-ENUM change — but it is outside this task's fence

`M_CastleCrumble` ORM node `SamplerType: LinearColor → Masks` (the exact combination `M_AssetPBR` effectively runs — Masks-compressed texture, linear channels; `.rgb` swizzles `R`/`G`/`B` keep meaning O/R/M). Zero graph topology change, zero param change, stock nodes. But TASK-337's scope is **"scalar parameter OVERRIDE VALUES on MI 01/02 ONLY — do NOT touch the `M_CastleCrumble` master graph"**, and both alternative lanes are also fenced (any `T_Castle_*` edit; MI texture params are not scalars). Per the standing "flag it, don't fix unilaterally" doctrine I stopped here.

**Recommendation to manager:** (a) open a micro-task authorizing the sampler-type fix on `M_CastleCrumble` (+ resave of master, which also clears the "recompiles every launch" debt) — art-director lane, minutes; (b) THEN re-run this TASK-337 band measurement against the first real render of the stage params since the rebuild — the TASK-157 values may or may not need the re-spread once they actually render (my cross-test suggests real headroom: compiled-material wall ≈ 0.96×P before Darken/Scorch). TASK-338's independent verify stays downstream. Note for whoever fixes it: after the sampler change the master needs its own save (it currently also warns "will recompile every editor launch until resaved").

## Session hygiene / editor state

- MI_Castle_Crumble01 probe values RESTORED bit-exact to shipped (Darken 0.8, ScorchAmount 0.12 — float32-identical readback confirmed); asset left dirty-in-memory but **NOT saved** — on-disk `.uasset` untouched (verify: `git status` shows no Content/Materials change). MI02/MI03 never touched (not dirty).
- Transient Simulate-world edits (cross-test slots, `r.ForceLOD`) died with the session; Simulate STOPPED cleanly; material editor window closed; `LogMaterial` verbosity restored to Log.
- In-memory-only editor tweaks that revert on restart: `bThrottleCPUWhenNotForeground=false` (TASK-239 recipe, needed for background screenshots), remote-exec flag (was already on).
- `L_Arena` never saved; pre-existing dirty state (from TASK-330/331) left as found. No TASKBOARD edit. Nothing published externally.

---

# RE-RUN (2026-07-27, same day — MEASUREMENT-FIRST, after the TASK-339 sampler fix landed): band FAIL at shipped values → conditional RETUNE ran → PASS

**Status: complete — retune executed.** art-director, same editor session as TASK-339. **Saved (Simulate STOPPED first): `MI_Castle_Crumble01.uasset` + `MI_Castle_Crumble02.uasset` ONLY. MI03 untouched. `L_Arena` never saved. No Git (TASK-338 commits master + both MIs). No board edit.**

## Protocol (TASK-331/337 protocol, reproduced)

Simulate on `L_Arena` (never PIE-in-viewport), ONE castle — `Castle_0`, found **by class** (`unreal.Castle`) + `Team==TeamId.BLUE`, never by label — driven 2000 → 1480 → 980 → 480 HP by instigator-less `apply_damage` (520/500/500; each stage readback-verified to fire ONCE in order with the correct mesh + MI on BOTH slots). Frozen camera pose loc (−23200, −1300, 550) / rot (−4.4, 144.2, 0), re-asserted and readback-verified. 1920×1080 HighResShot; wall region (800,980,615,695), grass control (300,650,850,1000); formula = mean Rec.709 luma on 8-bit sRGB (0–255) — the exact `measure.py` validated against TASK-331's PNGs in the first run. Pre-fix continuity check: pristine wall measured **173.13** vs the first run's 173.2 — protocol reproduces to 0.04 %.

**Interference incident + control (recorded):** the first sweep of this re-run was DISCARDED — the RED `SiegeBotController` summoned units during the long protocol and a RED Cavalry reached the Blue castle mid-sweep (drove it 980→180→0, win-state risk + hit-flash contamination). Clean re-run: fresh Simulate, bot neutralized via its public `StopDecisionTimer()` (BlueprintCallable — the TASK-047 hook) + all `SummonedUnit` actors destroyed (all transient sim-world edits, died with the session; verified 0 units respawned). Every capture then verified HP-stable between damage call and shot. (The discarded sweep's wall ratios matched the clean run to 3 dp anyway — the contamination never actually entered the wall region — but the clean set is the deliverable.)

## The measured table — SHIPPED TASK-157 values (first real render since `fcb1ec0`)

MI values readback-verified live before measuring: MI01 0.80/0.12 · MI02 0.50/0.45 · MI03 0.28/0.82 (float32-exact; the first run's MI01 restore held).

| State (shipped values) | Wall | Grass | ×P | Gate | Verdict |
|---|---|---|---|---|---|
| P pristine (`SM_Castle`, TeamBlue+PBR) | **173.57** | 151.71 | 1.000 | — | `TASK-337-rerun-P-pristine.png` |
| S1 (01: Darken .80 / Scorch .12) | **157.33** | 151.64 | **0.906** | 0.55–0.80 | **FAIL — too BRIGHT** · `TASK-337-rerun-S1-shipped-FAIL.png` |
| S2 (02: .50 / .45) | **111.53** | 151.67 | **0.643** | 0.35–0.55 | **FAIL — too BRIGHT** · `TASK-337-rerun-S2-shipped-FAIL.png` |
| S3 (03: .28 / .82) | **58.33** | 151.66 | **0.336** | ≤ 0.40 | PASS · `TASK-337-rerun-S3-shipped-PASS.png` |
| gaps | | | S1−S2 = 0.264 · S2−S3 = 0.307 | ≥ 0.10 / ≥ 0.05 | PASS / PASS |

Exposure-consistent: grass 151.55–151.71 across ALL captures (spread 0.16). **Band verdict at shipped values: FAIL (2 of 5 criteria)** — S1 and S2 both read far too bright against the rebuilt base. Note for the record: the withdrawn adjudication direction ("Darken UP toward 0.85–0.90, Scorch DOWN") was exactly BACKWARDS for the real render — measurement-first was the right call. The real-render transfer curve is strongly compressive (screen-ratio ≈ (Darken×(1−Scorch))^~0.37 under the tonemapper), so shipped Darken 0.80 lands at 0.906×P, not ~0.7.

## The retune (authorized by the band FAIL — values my call inside the band, param names law kept)

| MI | Param | Shipped | **Final** | Basis |
|---|---|---|---|---|
| MI_Castle_Crumble01 | Darken | 0.80 | **0.38** | target S1 ≈ 0.67×P (band middle), fit from the three real-render points |
| MI_Castle_Crumble01 | ScorchAmount | 0.12 | **0.18** | keep the scorch a TINGE (stage-1 intent), most of the drop from Darken |
| MI_Castle_Crumble02 | Darken | 0.50 | **0.24** | target S2 ≈ 0.46×P (band middle) |
| MI_Castle_Crumble02 | ScorchAmount | 0.45 | **0.52** | stage 2 reads MORE charred than stage 1, not just dimmer |
| MI_Castle_Crumble03 | (all) | 0.28/0.82 | **untouched** | S3 passes as shipped; gap rule satisfied without a nudge |

`RoughBoost`/`CharColor`/`EmberColor`/`EmberAmount`: untouched on all three (non-gating). No parameter renamed/deleted. Master graph untouched (TASK-339's one enum is the only master change, and it precedes this run).

## Verification at FINAL values (same session, same protocol — one iteration sufficed)

| State (final values) | Wall | Grass | ×P | Gate | Verdict |
|---|---|---|---|---|---|
| P (same-session) | 173.57 | 151.71 | 1.000 | — | — |
| S1 (0.38/0.18) | **115.18** | 151.62 | **0.664** | 0.55–0.80 | **PASS** · `TASK-337-rerun-S1-tuned.png` |
| S2 (0.24/0.52) | **76.20** | 151.55 | **0.439** | 0.35–0.55 | **PASS** · `TASK-337-rerun-S2-tuned.png` |
| S3 (0.28/0.82, untouched) | **58.30** | 151.56 | **0.336** | ≤ 0.40 | **PASS** · `TASK-337-rerun-S3-final.png` |
| gaps | | | S1−S2 = **0.225** · S2−S3 = **0.103** | ≥ 0.10 / ≥ 0.05 | **PASS / PASS** |

**All five band criteria PASS.** Visual reads at gameplay distance: S1 = dimmed dusty tan with scorch patches, unmistakably "battle-worn but standing" and clearly closer to alive than dead; S2 = blackened charcoal-grey-brown, clearly worse; S3 = near-dead dark charred silhouette. Stage progression is legible at a glance — three distinct states, strictly darkening.

## Session hygiene

- Each stage fired once in order on every sweep (readbacks in-line); `ResetCastle` restored pristine (hp 2000, stage 0, `SM_Castle`) before Simulate stop — twice (between sweeps and at end).
- Simulate STOPPED (verified `IsPIERunning == false`) BEFORE saving the two MIs. `L_Arena` never saved. Message-Log sweep clean: ensure/AccessedNone/Fatal/`LogOutputDevice: Error` = 0; **`Failed to compile Material` = 0 post-TASK-339-fix across both Simulate sessions**.
- On-disk delta this session (git): `M_CastleCrumble.uasset` (TASK-339) + `MI_Castle_Crumble01.uasset` + `MI_Castle_Crumble02.uasset` (this run). Nothing else (the `M_GoldGlow`/board/CONVENTIONS diffs pre-date this session — TASK-332/333 lane).
- Jonathan's playtest eye remains the FINAL authority over these numbers (two-tier ruling); TASK-338 carries that WATCH.

