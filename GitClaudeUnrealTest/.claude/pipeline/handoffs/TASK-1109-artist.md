# TASK-1109 — [TREE-USAGE-FLAG] — art-director handoff

# ✅ FIXED — the usage flag. ⛔ NOT DONE — the trunk textures, and **not because I skipped them**: the two settings the row names were **already set**. See §4.

**Both instruments agree.** Tree-line CTI **4.540 → 4.940** same-session (Δ **+0.400**, ≈13× the measured between-run noise floor of 0.029), and the engine's own
`missing usage flag InstancedStaticMeshes` warning is **PRESENT** in the pre-fix session log and **ABSENT** from a fresh post-fix session that reached the identical
trigger point. The battlefield scatter trees are gold birches with cut leaf silhouettes instead of near-black opaque slabs, and that is visible without any metric.

**Date:** 2026-09-06 · **Editor:** closed PID **31996**, relaunched PID **24532**, MCP `127.0.0.1:8000` answering · **Assets saved: 2** (not 4 — §4).

---

## 0. THE AUTHORIZATION, QUOTED NOT PARAPHRASED

🧑 Jonathan, verbatim (orchestrator terminal, 2026-09-06):

> ***"ok, I went ahead and disabled revision control by doing option A in "2", and I am giving you permission to proceed with proceed with "1" and "3", so that should take care of all three of those things"***

Licence of record: `CONVENTIONS.md` `FIELD-§3`, the **broadened dated exception** (amended the same day), items **2** (the usage flag on the two Mobile materials) and
**3** (the compression settings on the two `T_Trunk_Pack1*` textures). Nothing else under `Content/Tree_Pack_1/**` was touched. Cause of record:
`handoffs/TREE-DARK-diagnosis-2.md` — cited, not re-derived.

---

## 1. READ-BACK — THE MATERIALS (`SC-§94` cl. A: the value read **off the asset**, not the setter's return)

| | `M_Pack1_Leaf_Mobile` | `M_Pack1_Trunk_Mobile` |
|---|---|---|
| `bUsedWithInstancedStaticMeshes` **BEFORE** | `false` | `false` |
| `set_properties` returned | `true` | `true` |
| **`bUsedWithInstancedStaticMeshes` read back after save** | **`true`** ✅ | **`true`** ✅ |
| `is_dirty` after save | `false` | `false` |
| **Re-read in a FRESH editor process (PID 24532), i.e. off the `.uasset` on disk** | **`true`** ✅ | **`true`** ✅ |
| `bAutomaticallySetUsageInEditor` (unchanged) | `true` | `true` |
| `BlendMode` (unchanged) | `BLEND_Masked` | `BLEND_Opaque` |
| on disk, before → after | 20,939 → **17,826 B** | 23,457 → **23,142 B** |

The fresh-process re-read is the one that matters: it proves the boolean went to the `.uasset` and not merely to the in-memory object.
The engine had said so itself, in the pre-fix log, one line under the warning: *"The material will recompile every editor launch until resaved."*

**No editor wedge.** Both `set_properties` calls and the `save_assets` returned immediately and the editor stayed responsive throughout —
the `TASK-239` risk was accepted as low-but-nonzero and it did not materialise. No `RecompileShaders`, no batch recompile, no Custom-HLSL path was gone near.

---

## 2. INSTRUMENT 1 — PIXELS

**Mode (named beside the pose, `SC-§88a` cl. 1):** Simulate-In-Editor, `StartPIE {bSimulate: true, playMode: PlayMode_Simulate, warmupSeconds: 8}`.
**Pose:** lane vantage `(-24000, 0, 1400)` pitch `−4` yaw `0`, **FOV 90** — `cameraLocation`/`cameraRotation`/`cameraFOV` read back == requested on every capture.
**Metric:** `TASK-1083` §13.4 verbatim — canopy mask HSV hue 40–110°, S ≥ 0.15, V ≥ 0.05; CTI = mean Rec.709 linear luminance of sRGB-linearised masked pixels × 100.
**Tree-line ROI:** full width × rows 0–185, `TASK-1099`'s rect on a 2764×828 frame.

### 2a. Stationary series, not "|Δ| → 0" (`SC-§88a` cl. 6) — consecutive whole-frame mean `|ΔRGB|`/255

| run | series | verdict |
|---|---|---|
| **run 1 — pre-fix (promoted BEFORE)** | 1.462 / 1.454 / 1.467 / 1.398 | stationary, no monotone trend |
| run 2 — pre-fix control | 1.491 / 1.425 | stationary |
| **run 3 — post-fix (promoted AFTER)** | 1.812 / 1.690 / 1.712 / 1.815 | stationary, no trend |
| run 4 — post-fix, fresh process | 3.885 / 1.891 | frame 1 is a warm-up outlier, dropped; 2–3 stationary |

### 2b. Tree-line CTI + the noise floor I actually measured

| burst | per frame | mean | burst spread |
|---|---|---:|---:|
| **run 1, PRE-FIX (BEFORE)** | 4.547 / 4.537 / 4.542 / 4.537 / **4.540** | **4.540** | 0.011 |
| run 2, PRE-FIX again, new Simulate run | 4.573 / 4.564 / **4.571** | 4.569 | 0.008 |
| **run 3, POST-FIX (AFTER)** | 4.949 / 4.939 / 4.938 / 4.951 / **4.925** | **4.940** | 0.026 |

⇒ **BETWEEN-RUN NOISE FLOOR (run 1 vs run 2, identical asset state) = `0.029`.**
⇒ ### SIGNED DELTA, burst means, **BEFORE → AFTER = `+0.400` CTI (+8.8 %)**; promoted frames `4.540 → 4.925` = **`+0.385`**; against the control run, **`+0.371`**.
**That is ≈ 13× the between-run floor and ≈ 15× the within-burst spread.** It dwarfs `TASK-1099`'s `±0.148` baseline spread, which the row asked it to beat.

### 2c. ⚠️ THE CAVEAT I OWE YOU, AND IT LIMITS THE NUMBER ABOVE

Run 4 — the **fresh process**, post-fix, same pose, same metric — reads tree-line CTI **4.372 ± 0.003**, i.e. **BELOW both pre-fix readings.**
⇒ **This ROI carries a between-*session* offset that is larger than the effect.** The `+0.400` is only load-bearing as a **same-session** comparison
(runs 1/2/3 are one editor process, one lighting state, one streaming state). `TASK-1099` saw the same thing across sessions (5.46 → 5.93 for no reason)
and I am not going to quote a cross-session tree-line delta as if it meant something.

There is a second reason not to lean on it, and `TASK-1099` §3 already wrote it down: **rows 0–185 is the far horizon band and the black slabs were at rows ~300–700 —
the mandated ROI barely contains the defect.** So I measured the band that does.

### 2d. The field band, rows 185–700 — where the trees actually are, and it survives a process restart

| metric (full width, rows 185–700) | pre run 1 | pre run 2 | **post run 3** (same session) | **post run 4** (fresh process) |
|---|---:|---:|---:|---:|
| dark-pixel fraction, max(RGB) < 95 | 51.730 % | 59.231 % | **44.564 %** | **45.515 %** |
| CTI of those dark pixels | 1.279 | 1.005 | **2.674** | **2.630** |
| **mean HSV saturation of those dark pixels** | **0.3849** | **0.4057** | **0.6168** | **0.6050** |
| CTI of the whole band | 13.656 | 12.060 | **15.431** | **15.117** |

Every row separates, and — the part that matters — **the two post-fix readings agree with each other across a process restart by an order of magnitude
more tightly than the pre/post gap.** Saturation is the cleanest discriminator and it is the one the diagnosis predicts: `WorldGridMaterial` is neutral grey
(pre: 0.385–0.406), the birch albedo is not (post: 0.605–0.617). The dark fraction falling 51.7/59.2 → 44.6/45.5 is the opaque quads becoming cut leaf cards.

**Independent corroboration, free:** PNG entropy. Pre-fix frames encode to 4.81–4.97 MB of base64; post-fix frames to 5.77–5.88 MB — **+18…22 % more image detail**
at an identical pose, which is what replacing flat slabs with foliage does.

### 2e. And the plain visual

The BEFORE frame shows the battlefield trees as **near-black, straight-edged polygon slabs**, against light-yellow correct-looking trees on the skyline
(Jonathan's praised background ring — untouched, and still there in the AFTER). The AFTER frame shows **gold/amber birches with ragged cut canopies**, matching
the skyline trees. This does not need a metric to adjudicate.

---

## 3. INSTRUMENT 2 — THE ENGINE'S OWN WARNING, IN A FRESH SESSION

**How I obtained a fresh log:** copied the live session log aside, then **closed the editor** (`Stop-Process -Id 31996 -Force`; standing grant from Jonathan;
`L_Arena` `is_dirty` = `false`, both materials `is_dirty` = `false`, so nothing of any lane was discarded), **relaunched** with the identical command line
(`UnrealEditor.exe "…/GitClaudeUnrealTest.uproject"`) as **PID 24532**, polled MCP on `127.0.0.1:8000` until it answered, and confirmed `L_Arena` loaded
(`Cmd: MAP LOAD FILE=".../L_Arena.umap"` at `23:05:53`).

### 3a. ⭐ A CORRECTION TO THE DIAGNOSIS, AND IT IS LOAD-BEARING FOR THIS INSTRUMENT

`TREE-DARK-diagnosis-2.md` reads the warning as *"frame 487 = level load, first ISM proxy build."* In the session I inherited it fired at **frame 167**, and the
three lines directly above it are:

```
[2026.09.06-22.57.13:885][167]LogSiegeNavDiag: nav-build [post-scatter]: remaining=0 running=0 dirtyAreas=2 hasDirty=true activeTiles=446 poolCap=1024
[2026.09.06-22.57.13:886][167]PIE: Server logged in
[2026.09.06-22.57.13:886][167]PIE: Play in editor total start time 1.678 seconds.
[2026.09.06-22.57.13:897][167]LogMaterial: Warning: Material /Game/Tree_Pack_1/Meches/Mobile_Tree_1/M_Pack1_Trunk_Mobile.M_Pack1_Trunk_Mobile missing usage flag InstancedStaticMeshes! Default Material will be used in game.
[2026.09.06-22.57.13:897][167]LogMaterial: Warning:      The material will recompile every editor launch until resaved.
[2026.09.06-22.57.13:897][167]LogMaterial: Warning: Material /Game/Tree_Pack_1/Meches/Mobile_Tree_1/M_Pack1_Leaf_Mobile.M_Pack1_Leaf_Mobile missing usage flag InstancedStaticMeshes! Default Material will be used in game.
[2026.09.06-22.57.13:897][167]LogMaterial: Warning:      The material will recompile every editor launch until resaved.
```

⇒ **The trigger is the SCATTER'S ISM PROXY BUILD AT SIMULATE START, not editor level load.** That is not pedantry: it means **a fresh session that merely boots and
loads `L_Arena` never reaches the check**, so a clean scan of such a log would have read "absent" for a reason that has nothing to do with the fix. I would have
had a false pass. **So I ran a Simulate in the fresh session before scanning.**

### 3b. The scan, and its positive control

**Fresh session (PID 24532), full log, `pattern: "missing usage flag"`, `maxEntries: 0` (unlimited, the way the diagnosis did it):**

```
returnValue: []          ← ZERO occurrences
```

**Positive control that the identical code path actually ran** — the same triple that the pre-fix warning followed, in the fresh session's log:

```
[2026.09.06-23.12.14:748][112]LogSiegeNavDiag: nav-build [post-scatter]: remaining=0 running=0 dirtyAreas=2 hasDirty=true activeTiles=446 poolCap=1024
[2026.09.06-23.12.14:749][112]PIE: Server logged in
[2026.09.06-23.12.14:752][112]PIE: Play in editor total start time 1.636 seconds.
```

⇒ **The scatter's ISM proxies were built at frame 112 of the fresh session, and nothing followed them.** Present before / absent after, at the same trigger,
with the warn-once latch defeated by a genuinely new process. Pre-fix count in that session: **2** (both materials, by full path). Post-fix count: **0**.

**Both instruments agree. Nothing was rounded.**

---

## 4. 🚨 THE TRUNK TEXTURES — NOT CHANGED, AND THE REASON IS A MEASUREMENT, NOT A REFUSAL

The row's clauses (1)(c)/(d) name two settings. **Both were already set to exactly those values before I arrived.** Setting them again would have been a no-op
that produced a `.uasset` diff and a claimed fix. Read back live:

| | `T_Trunk_Pack1` | `T_Trunk_Pack1_normal` | row's target | **CONTROL: `T_Leaf_Pack1`** |
|---|---|---|---|---|
| `CompressionSettings` | **`TC_Default`** | **`TC_Normalmap`** | `TC_Default` / `TC_Normalmap` — **already met** | `TC_Default` |
| `SRGB` | `True` (correct for a colour map) | **`False`** | `SRGB false` on the normal — **already met** | `True` |
| **`Format` (the RESULT)** | **`B8G8R8A8`** ⛔ | **`B8G8R8A8`** ⛔ | BC | **`DXT5`** ✅ |
| `Dimensions` | **`1414×1414`** | **`1414×1414`** | — | `2048×2048` |
| `LODGroup` | `TEXTUREGROUP_World` | `TEXTUREGROUP_WorldNormalMap` | — | `TEXTUREGROUP_World` |
| `MipGenSettings` | `TMGS_NoMipmaps` | `TMGS_NoMipmaps` | — | `TMGS_FromTextureGroup` |
| `PowerOfTwoMode` | `None` | `None` | — | `None` |
| **on disk (unchanged)** | **2,476,960 B** | **2,785,590 B** | — | — |

**Cause: `1414` is not divisible by 4, and UE's BCn encoder requires a top mip that is a multiple of 4.** The engine falls back to `B8G8R8A8` silently — no error,
no warning, and every compression *setting* reads back green. This is `SC-§94` cl. A in its purest form: **the settings echo the request; `Format` measures the result.**

**I did not assert that from memory. Census of all `Texture2D` in `/Game` — 647 assets, 0 unreadable:**

| | multiple-of-4 dims | NOT multiple-of-4 |
|---|---:|---:|
| BC/DXT compressed | **609** | **0** |
| not compressed | 36 (all report `Format: unknown` — unbuilt/unloaded, not counterexamples) | **2** |

**The only two non-multiple-of-4 textures in the entire project are these two, and they are the only two that failed to compress. Zero non-multiple-of-4 textures
anywhere in the project are compressed.** The control `T_Leaf_Pack1` settles the other direction: identical `TC_Default`, identical `TEXTUREGROUP_World`,
identical `SRGB`, `PowerOfTwoMode None` — but `2048²` — and it is `DXT5`.

### What would actually reach BC, and why I did not do it

The only routes are **build-time resizes**: `powerOfTwoMode = StretchToPowerOfTwo` (→ 2048², UVs preserved) or `ResizeToSpecificResolution`.
That resamples vendor art and changes the texture's stored resolution — it is **not on the row's property list** ("EXACTLY THESE PROPERTIES"), and it is a
different *kind* of change from "a compression setting". **`FIELD-§3` item 3's own wording names the target — *"what BC at 2048² costs ~8"* — so the ~8 MiB figure
Jonathan was quoted only exists if these become 2048².** That reading is plausible enough that I will not act on it unilaterally on a vendor asset.
⇒ **One sentence from Jonathan closes it.** Numbers for that sentence:

- `StretchToPowerOfTwo` → 2048²: BC1 ≈ 2.0 MiB + BC5 ≈ 4.0 MiB (≈ 2.7 + 5.6 MiB with mips) vs **7.63 MiB each / 15.3 MiB resident today**. Slight resample softening; fully reversible (a build setting; source data untouched).
- ⚠️ **And the on-disk `.uasset` will barely move either way.** These files store the **PNG source** (`SourceCompression: PNG`); the compressed platform data lives in the DDC and is generated at cook. The saving is **resident/cooked memory, not repository bytes** — so nobody should expect `2,476,960 B` to shrink to ~1 MB.

**This is a stop-and-report, not a half-finished row.** The tree fix — the thing Jonathan complained about — is complete and proven.

---

## 5. VRAM — THE READING AND THE CAVEAT, IN THE SAME BREATH

Instrument: `nvidia-smi --query-gpu=memory.used,memory.free` (whole-GPU, every process; card **8151 MiB** total), Simulate at the lane vantage, converged.

| state | used | free | samples |
|---|---:|---:|---|
| pre-fix, Simulate run 1 | 7592 MiB | 300 MiB | 8 × 2 s, spread **0** |
| post-fix, same session, Simulate run 3 | 6497 MiB | 1395 MiB | 6 × 2 s, spread **0** |
| post-fix, fresh process, Simulate run 4 | 7271 MiB | 621 MiB | 6 × 2 s, spread **0** |

**⛔ NO SAVING IS CLAIMED AND NONE IS OBSERVABLE.** The three readings span **1095 MiB** while each is internally rock-steady — that spread is whole-machine
contention, exactly as `TASK-1085` §1 recorded (~±600 MiB between sessions). **No texture was changed in this row, so nothing here should have moved VRAM at all**,
and the ~12 MiB the trunk pair would have saved sits roughly **90× below** this instrument's between-session spread. Reporting a number here would be inventing one.
What is real is in §4: the **format and on-disk facts**, unchanged.

---

## 6. CAPTURES — PROMOTED, CAPTIONED, PINNED NAMES

| file | bytes | dims | sha256 (12) | caption |
|---|---:|---|---|---|
| `.claude/pipeline/playtest-evidence/2026-09-06/TASK-1109-BEFORE-lane.png` | 3,045,614 | 2764×828 | `d59cca0481af` | **Simulate-In-Editor (`bSimulate=true`, `PlayMode_Simulate`)**, lane vantage `(-24000, 0, 1400)` pitch −4 yaw 0 FOV 90. **PRE-FIX**, `bUsedWithInstancedStaticMeshes = false` on both tree materials. Frame 5 of 5; tree-line CTI **4.540**, burst 4.540 ± 0.011. |
| `.claude/pipeline/playtest-evidence/2026-09-06/TASK-1109-AFTER-lane.png` | 3,656,858 | 2764×828 | `856516b07ef6` | Same instrument, same pose, new Simulate run in the same editor session. **POST-FIX**, flag `true` on both, saved. Frame 5 of 5; tree-line CTI **4.925**, burst 4.940 ± 0.026. |
| `.claude/pipeline/playtest-evidence/2026-09-06/TASK-1109-SIDE-BY-SIDE.png` | 6,709,682 | 2764×1808 | `f4d2b6333b4e` | **TOP = BEFORE (flag false), BOTTOM = AFTER (flag true).** Both captions printed into the image with mode, pose and CTI; red box = the `TASK-1083` §13 tree-line ROI, rows 0–185. |

---

## 7. WHAT I SAVED, AND THE FENCES

**Saved — exactly two assets, by explicit path (never `save_assets([])`, never save-all, never save-dirty):**
- `Content/Tree_Pack_1/Meches/Mobile_Tree_1/M_Pack1_Leaf_Mobile.uasset` — 20,939 → **17,826 B**
- `Content/Tree_Pack_1/Meches/Mobile_Tree_1/M_Pack1_Trunk_Mobile.uasset` — 23,457 → **23,142 B**

**Not saved, unchanged on disk:** `T_Trunk_Pack1.uasset` (2,476,960 B), `T_Trunk_Pack1_normal.uasset` (2,785,590 B) — §4.

| fence | state |
|---|---|
| `git status --short Content/Tree_Pack_1/` | **exactly 2 lines** (the two materials). **Not 4** — and the two missing ones are §4's measured no-op, not a scope breach in the other direction. |
| `git status --short Content/Maps/` | **empty** |
| `L_Arena.umap` sha256, before and after | `1f78419d888940739c949a60b29ee43451c464915f7ab37942b921fe0af15622` — **identical**; `is_dirty` `false` at every check |
| Simulate sessions | 4 started, **4 stopped, all mine**; `IsPIERunning` `false` before the first and after the last |
| C++ / `CONVENTIONS.md` / `DA_BattlefieldScatter` | untouched |
| commit / push | **none** — `TASK-1110` is the host |
| console cvars | none set, nothing to restore |
| revision-control provider | left as Jonathan set it (`None`); not changed back, nothing restaged |

**Editor left UP, PID 24532, MCP `127.0.0.1:8000` answering, `L_Arena` loaded, no play session, nothing dirty.**

---

## 8. WHAT SURPRISED ME (five things, all of them cost something)

1. **The trunk textures' compression settings were already correct.** The row's prescribed writes were no-ops; the actual blocker is a dimension that is not a
   multiple of 4. Had I "executed the row", I would have shipped two spurious `.uasset` diffs and a claimed ~12 MiB saving that never happened.
2. **The warning fires at Simulate start, not level load.** A fresh session that only boots would have scanned clean while proving nothing. This is the
   `SHIP-§9` shape — validate a gate against the failure it detects — and it nearly cost this row its second instrument.
3. **Simulate re-seeds the world between runs**, hard: two bursts of the *identical* asset state differ by mean `|ΔRGB|` **26.0/255**, with **38.5 %** of pixels
   apart by >10. Any fixed-crop metric is therefore worthless across runs — my near-canopy crop read 1.407 in one pre-fix run and 0.902 in the next. **Take a
   same-state control burst before believing any Simulate A/B.**
4. **The mandated tree-line ROI carries a between-session offset bigger than the effect** (post-fix fresh session reads *below* pre-fix). It is a valid
   *same-session* instrument and a misleading cross-session one.
5. **No wedge.** The `TASK-239` recompile hazard did not fire — the saves returned immediately. Worth recording so the risk keeps its true, low weight.

---

## 9. FOR THE HOST (`TASK-1110`)

Stage, from this lane: the **two** material `.uasset`s (not four), this handoff, and the three pinned `TASK-1109-*.png`.
`Content/Tree_Pack_1/` shows exactly two modified lines; if a third appears it did not come from me.
