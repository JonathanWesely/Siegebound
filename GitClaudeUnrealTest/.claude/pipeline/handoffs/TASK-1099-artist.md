# TASK-1099 — [TREE-LOD-PROOF] art-director handoff (2026-09-06)

Marker `TASK-1099-TREE-LOD-PROOF` · editor PID 20564, MCP `127.0.0.1:8000` answering · law cited: `FIELD-§7` · `SC-§88a` (incl. cl. 6) · `SC-§88` · `SC-§94` cl. A · `SC-§85` · `SC-§91`.

---

# ⛔ VERDICT: **REFUTED.**

**The near black slabs do NOT become birches under `ForceLOD 0`. They do not change at all.**
Same silhouette, same coverage, same darkness, at LOD0 as shipped. `TASK-1100` is **VOID, not delayed.**

This is not a partial confirmation and I am not softening it. **(b) — "a LOD / geometry effect" — is eliminated at the lane vantage.** The `TREE-DARK-diagnosis.md` reasoning was sound and its LOD table is accurate; the inference from it is what the frames refute. The one thing that diagnostic could not observe is now observed, and it says no.

---

## 1. The instrument (`SC-§88a` cl. 1 — mode named beside the pose)

**Simulate-In-Editor**, `StartPIE {bSimulate: true, playMode: PlayMode_Simulate, warmupSeconds: 8}` — **not** a PIE-posed capture.
Pose **`(-24000, 0, 1400)` pitch −4 yaw 0**, FOV 90, **2764×828**, `bShowUI=false`, annotations off (grid 0 / labels 0).
`EditorAppToolset.CaptureViewport` with an explicit `captureTransform`; **camera read back == requested on all 10 burst frames** (`loc.x = -24000`, `pitch = -4`, `fov = 90` asserted per frame).
One Simulate session, started and stopped by me; `IsPIERunning` **false** before I started and **false** after I stopped.

## 2. The pair, and the numbers

Convergence per **`SC-§88a` cl. 6** — *stationary series* + *converged metric with spread*, **not** `|Δ|→0`.
CTI is `TASK-1083` §13.4's method verbatim: canopy mask HSV hue 40–110°, S ≥ 0.15, V ≥ 0.05; metric = mean Rec.709 linear luminance of sRGB-linearised masked pixels × 100. ROI rects in full-res pixels.

| | **Frame A — `r.ForceLOD 0` AND `foliage.ForceLOD 0`** | **Frame B — SHIPPED, both `-1`** |
|---|---|---|
| cvar read-back, before and after the burst | `r.ForceLOD 0` / `foliage.ForceLOD 0` | `r.ForceLOD -1` / `foliage.ForceLOD -1` |
| consecutive mean \|ΔRGB\|/255 | **2.94 → 2.70 → 3.01 → 2.82** | **1.67 → 0.53 → 2.45 → 0.52** |
| series verdict | **stationary**, no monotone trend, no drift | **stationary**, no monotone trend, no drift |
| tree-line CTI per frame | 5.940 / 5.817 / 5.943 / 5.803 / **5.950** | 5.921 / 5.916 / 5.917 / 5.937 / **5.932** |
| **converged metric (burst)** | **5.891 ± 0.148** (spread 2.51 %) | **5.925 ± 0.021** (spread 0.35 %) |
| promoted frame (5 of 5), ALL tree line `(0,0,2764,185)` | **CTI 5.950**, 113,275 px, meanSRGB (66.0, 66.3, 42.8) | **CTI 5.932**, 113,311 px, meanSRGB (65.9, 66.1, 42.6) |
| L dark cluster `(330,0,1000,185)` | 5.335 | 5.251 |
| C castle pair `(1410,0,1680,185)` | 5.685 | 5.660 |
| R gold pair `(1725,0,2070,185)` | 6.720 | 6.790 |

### ⇒ SIGNED DELTA, promoted frames, **A − B = +0.018 CTI (+0.30 %)**. Burst means: **−0.034 (−0.6 %)**.
Both are **inside Frame A's own burst spread of 0.148**. There is no signal. (`TASK-1085`'s reading at this ROI was 5.477 / burst 5.459 ± 0.045; today's 5.93 differs from it because the world was re-seeded, not because of anything this row did — both cvar states read 5.93, which is the point.)

⚠️ **Instrument caveat, stated because it matters more than the number:** the `TASK-1083` §13 ROI is rows **0–185**, the far horizon band. **The near black slabs live at rows ~300–700 and are not in that ROI at all.** The mandated number is reported above and is null; the silhouette test below is what actually carries the verdict, exactly as `FIELD-§7` prescribes.

## 3. The silhouette test — the primary, answered in words (`FIELD-§7`)

**Do the near canopies become recognisable leafy birches under `ForceLOD 0`? No. They stay coarse dark triangular slabs, and the slabs are pixel-for-pixel the same slabs.**

Measured on the near scatter tree at screen `(1876, 555)` — `SM-Mobile_Tree_8`, instance scale 0.92, 4,483 uu from camera — crop `x 1650–2050, y 150–560`, canopy mask = pixels with max(RGB) < 95 (114,926 px):

| state | mean \|Δ\| vs AUTO, canopy mask | % masked px > 20/255 | canopy coverage (silhouette area) | edge density |
|---|---:|---:|---:|---:|
| **noise floor** (AUTO frame 3 vs frame 4, 1.5 s apart) | **0.22** | 0.01 % | — | — |
| AUTO / shipped (burst of 5) | — | — | **70.13 ± 0.09 %** | 23.04 ± 0.35 % |
| `r.ForceLOD 0` + `foliage.ForceLOD 0` (burst of 5) | 5.73 | 2.40 % | **71.38 ± 0.12 %** | 25.67 ± 1.97 % |
| `foliage.ForceLOD 0` alone | 9.73 | 8.18 % | **72.77 %** | 23.93 % |
| `foliage.ForceLOD 1` alone | 11.19 | 11.02 % | **72.70 %** | 23.64 % |
| `foliage.ForceLOD 4` alone | **28.88** | **30.31 %** | **65.54 %** | 29.31 % |
| `r.ForceLOD 0` alone | 10.24 | 8.77 % | 71.50 % | 21.44 % |

The 5–11 band on the single captures is **wind**, not LOD: those frames are ~30–60 s apart and the world ticks (`SC-§88a` cl. 6). LOD 4 at 28.88 / 30.3 % / −4.6 pp coverage is far outside it.

## 4. Which cvar moved the pixels — the knob every later row will need

**`foliage.ForceLOD` is the one that binds on the scatter trees.** Proven by isolation, not assumed:
- With `r.ForceLOD` held at `-1`, **`foliage.ForceLOD 4` alone visibly collapses the near scatter tree** (28.88 mean \|Δ\| on the canopy mask, coverage 70.1 → 65.5 %). So the cvar reaches the tree HISMs.
- With `foliage.ForceLOD` held at `-1`, **`r.ForceLOD 0` alone leaves the scatter tree at the wind level.** `r.ForceLOD` governs the `StaticMeshActor` trees / rocks / castle, not the HISM scatter.
- `foliage.ForceLOD` also visibly re-densifies the **grass** (obvious between Frames A and B) — side-finding, not this row's business, but it means the knob is live and there is grass-LOD headroom at this vantage.

⇒ **Because `foliage.ForceLOD 4` demonstrably moves the trees, `foliage.ForceLOD 0` demonstrably reached them too. The null result is a real null, not a knob that missed.** That was the one way this test could have lied, and it is closed.

## 5. cl. 6 — the LOD index actually chosen at this vantage

**The MCP exposes NO per-instance or per-component LOD read-out.** I checked every toolset; there is no such tool, and writing properties to live PIE HISMs is **void, not evidence** (`SC-§88a` cl. 4). **I did not read an index and I am not presenting one as a reading.** What I have instead, both labelled for what they are:

- **(a) Empirically, from the ladder above:** AUTO is indistinguishable from `foliage.ForceLOD 0` **and** from `foliage.ForceLOD 1`, and clearly distinguishable from `foliage.ForceLOD 4`. ⇒ **AUTO is LOD0 or LOD1 at this vantage — it is not LOD3/LOD4, and LOD0 and LOD1 are the same picture on this mesh.**
- **(b) Computed, not observed** — UE's `ComputeBoundsScreenSize = 2 · ScreenMultiple · SphereRadius · Scale / Dist`, `ScreenMultiple = 0.5·max(M₀₀, M₁₁)` = **1.669** at 2764×828 / FOV 90. Nearest in-frame scatter tree `SM-Mobile_Tree_8` (r = 1224.6, scale 0.92, 4,483 uu) ⇒ **screen size 0.842 ⇒ LOD1** against the chain `[2.0, 0.1191, 0.0263, 0.001447, 0.001237]`. 273 tree instances project into frame; the nearest sixteen compute to 0.22–0.84, i.e. **all LOD1**.

⇒ **The answer to the diagnosis's blind spot: LOD1. And LOD1 is not the defect** — the reducer's 50 % pass on this mesh does not destroy the leaf cards, because LOD0 looks exactly the same.

## 6. cl. 6b — the free control, and it resolves AGAINST the hoped-for corroboration

I measured it. All three sub-questions, from the live world:

| | answer |
|---|---|
| **(i) same asset?** | **YES.** All 15 hand-placed `StaticMeshActor`s use `SM-Mobile_Tree_{1,4,5,7,8,11,12}` out of `/Game/Tree_Pack_1/Meches/Mobile_Tree_1/`, material slots exactly `["M_Pack1_Trunk_Mobile", "M_Pack1_Leaf_Mobile"]` — **the same two D1 materials**, the same 5-LOD chain. Variants **overlap** the scatter set (4, 7, 8, 12 are in both), so it is **not** a variant split. |
| **(ii) scale?** | **7.53× – 10.94× uniform** (scatter is 0.80–1.20). **But their distances are 12,357–65,013 uu**, and the net computed screen sizes of the in-frame ones are **0.86 (VistaFill_59) / 1.04 (VistaFill_47) / 1.23 (VistaFill_57)** — **all below the 2.0 LOD0 threshold. Still LOD1.** |
| **(iii) per-actor override?** | **NONE.** `ForcedLodModel = 0` (auto) and `MinLOD = 0` on **all 15**. No forced LOD anywhere. |

### ⇒ The loose end closes the wrong way, and that is the finding
The up-scaling does **not** buy the praised trees a better LOD. **Same mesh, same materials, same light, same frame, and the same LOD band (LOD1) — one set reads as leafy light birches and one set reads as black slabs.** The control the project did not have to build exists, and it **eliminates LOD as the differentiator** independently of the forced frames. Two instruments, one conclusion.

Two corrections to the record while I was in there:
- **Only 3 of the in-frame `VistaFill_*` actors are trees** (`VistaFill_47` @ screen (2484,388), `_57` @ (−53,411) off-frame-left, `_59` @ (437,370)). Most in-frame `VistaFill_*` are **`SM_Hill_*` / `SM_Vista_*` ground and rock props** on `MI_BattlefieldGround` / `MI_Rock_*`. The "15 praised trees" are 15 in the level, not 15 in this frame.
- **The four nearest hand-placed trees (`VistaFill_51/52/53/55`) are BEHIND the camera** at the lane vantage and appear in neither frame. The diagnosis's note about "a correct-looking branch closest to camera" cannot have been one of them at this pose.

### Corroboration taken after `StopPIE` (read-only, no session running)
`CaptureAssetImage("/Game/Tree_Pack_1/Meches/Mobile_Tree_1/SM-Mobile_Tree_8")` — **the exact variant that is the near black slab at screen (1876, 555)** — renders as a **bright yellow birch with a cleanly cut leaf silhouette**, post-D1. So the content-browser-vs-match divergence `FIELD-§7` names is **real**. It is simply **not LOD**: the match at LOD0 does not close it. (Same for `SM-Mobile_Tree_12`.)

## 7. What `TASK-1100` should do

**Nothing. It is VOID.** Do not run `set_lod_thresholds`, do not `remove_lods`, do not regenerate a chain. All three of the diagnosis's proposed routes target a cause that is now measured absent: **LOD0 is already what the near trees render as, and LOD0 is the slab.** Making LOD0 "reachable" would cost draw calls on 340 instances and change nothing a player can see.

`FIELD-§7`'s *rule* survives intact and was correctly applied — silhouette-grading-with-distance is still the right first suspicion for one asset family. Its *licence clause* simply did not fire here.

## 8. The re-diagnosis question I hand back to the manager

**Given that LOD and material *assignment* are both now eliminated: why does `SM-Mobile_Tree_8` cut a leaf silhouette in its own asset render but render its leaf cards as solid, straight-edged triangles in the match at the same LOD0?** Candidates, in the order I would board them, none of them tested by me (this row proves, it does not fix):

1. **The opacity mask.** `TREE-DARK-diagnosis.md` §① lists the whole shipped `M_Pack1_Leaf_Mobile` graph as **four** connections — `MP_BaseColor`, `MP_Metallic (0)`, `MP_Roughness (1)`, `MP_Normal` — and **no `MP_OpacityMask`** on a `BLEND_Masked` material. An unconnected OpacityMask defaults to 1.0 ⇒ **every leaf card renders fully opaque**, which is precisely a straight-edged dark slab. Read the graph again *for the OpacityMask pin specifically* and check `T_Leaf_Pack1`'s alpha channel + `CompressionNoAlpha`. **Observation supporting it:** the canopy's dark coverage over its own bounding crop is **70 %**; a cut leaf canopy would be a fraction of that, and the canopy edges against the sky are straight polygon edges, not ragged foliage. **Caveat:** I did not read the graph myself, and this does not yet explain why the 256² asset render cuts correctly — that asymmetry is the actual question.
2. **The dusk lighting** — is the slab a *correctly shaped* canopy that only reads as a slab because it is unlit at CTI ≈ 5.9? The straight silhouette edges argue against it, but it is cheap to settle and it is the one candidate `TASK-1086` cl. 2-D (Option A, withdrawn) was aimed at.
3. **The HISM render path vs `StaticMeshActor`** — the two sets differ in that and little else; but note the *hand-placed* set is not obviously clean either at this pose, so weight this below (1).

⚖️ **And one thing to carry forward:** the diagnosis's "far away → sub-pixel → mips to green" and "the correct-looking branch is closest to camera" could not both be true, the manager was right to catch it, and the resolution is that **neither was describing what is in this frame** — the praised trees at this vantage are 30–45 km away and the nearest hand-placed trees are behind the camera.

---

## 9. Evidence — promoted, exact pinned names (`TASK-1101` stages by this list)

All three under `.claude/pipeline/playtest-evidence/2026-09-06/`:

| file | bytes | px | caption |
|---|---:|---|---|
| **`TASK-1099-FORCELOD0-lane.png`** | 3,226,994 | 2764×828 | *Frame A. **Simulate-In-Editor (`bSimulate=true`, `PlayMode_Simulate`)**, lane vantage `(-24000, 0, 1400)` pitch −4 yaw 0 FOV 90. **cvars: `r.ForceLOD 0` AND `foliage.ForceLOD 0`, both read back `0`.** Frame 5 of a 5-frame burst; tree-line CTI 5.950, burst 5.891 ± 0.148.* |
| **`TASK-1099-FORCELOD-AUTO-lane.png`** | 3,092,423 | 2764×828 | *Frame B, the shipped state. **Simulate-In-Editor (`bSimulate=true`, `PlayMode_Simulate`)**, identical pose. **cvars: `r.ForceLOD -1` AND `foliage.ForceLOD -1`, both read back `-1`.** Frame 5 of a 5-frame burst; tree-line CTI 5.932, burst 5.925 ± 0.021.* |
| **`TASK-1099-SIDE-BY-SIDE-lane.png`** | 7,242,295 | 2764×2258 | *A over B at full res, plus the LOD ladder on the identical crop `x1650–2050, y150–560` (near scatter `SM-Mobile_Tree_8`, 0.92×, 4,483 uu): **AUTO ｜ `foliage.ForceLOD 0` ｜ `foliage.ForceLOD 1` ｜ `foliage.ForceLOD 4`.** Same instrument mode, pose and burst as above; every cvar state captioned in the image.* |

The two lane frames are the **unaltered** instrument output (no caption baked in, no resize, no recompression of the pixels); the captions live here and in the sheet. Raw base64 bursts are in `Saved/TreeSurvey/t1099_{flod0,auto}_0..4.txt` + `t1099_{fol0,fol1,fol4,r0,ann,asset_*}_0.txt` (gitignored).

## 10. Fences — verified after the last engine call

- ✅ **ZERO asset writes.** No `set_lod_thresholds`, no `remove_lods`, no `generate_lods`, no material edit, no DataAsset edit, no import/reimport, **no `save_assets` in any form**, no level save.
- ✅ **Both cvars restored to `-1` before I finished** — read back `{"r.ForceLOD": -1, "foliage.ForceLOD": -1}` after `StopPIE`. `r.ForceLODShadow` and `r.SkeletalMeshForceLOD` were never touched and read `-1`.
- ✅ **`L_Arena` never saved** — `is_dirty` **false** after the session; hash `1f78419d…5622` untouched.
- ✅ **One Simulate session, mine, stopped by me.** `IsPIERunning` false before start and false after stop. No other agent's session was touched. No `EditorAssetSubsystem` call was made while it ran.
- ✅ Vendor packs read-only; the two D1 materials untouched — **`e37c899` stands**. No commit, no push, no Git of any kind.
- ✅ Editor left **up, PID 20564**, MCP answering, `L_Arena` loaded, no session running, nothing of mine dirty.
- ℹ️ Board: my own `- status:` line only, via the `Edit` tool.
