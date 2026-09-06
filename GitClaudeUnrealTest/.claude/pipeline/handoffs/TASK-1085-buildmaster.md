# TASK-1085 — [FIELD-SHIP] build-master handoff (2026-09-06)

Marker `TASK-1085-FIELD-SHIP` · assignee build-master · editor PID 20564, MCP `127.0.0.1:8000` answering · law cited: `FIELD-§3` (+ its dated 2026-09-06 exception) · `SC-§68` · `SC-§88` · `SC-§88a` · `SC-§94` · `§25c` · `TL-§5e` cl. 7.

**Status: done.** Commit **`e37c8996fa60433440caaa02b2b8ba49a7d17456`** (`e37c899`), 23 files, exactly the row's STAGES pathspec, **not pushed** (main 4 ahead of origin). VRAM gate **passed** — headroom went **up** 359 MiB, the refusal clause did not fire. Two Simulate frames promoted for `TASK-1086`. **No C++ changed ⇒ nothing was compiled**, and no compile was required.

⚠️ **This handoff is born OUTSIDE its own commit (`TL-§5e` cl. 7).** `e37c899` was made before this file existed; the next doc-host sweeps it. Do not amend `e37c899` to include it.

---

## 1. THE VRAM GATE — same instrument, same vantage, same discipline (`SC-§88`, `FIELD-§3` cl. 1)

**Instrument (1081 §3's, unchanged):** `nvidia-smi --query-gpu=memory.total,memory.used,memory.free`, i.e. the **whole-GPU DXGI local budget across every process on the machine** — ⛔ NOT the texture-streaming pool. Card `RTX 5070 Laptop`, **8151 MiB total**. **Vantage:** 1081's lane vantage, editor camera `(-24000, 0, 1400)` pitch −4 yaw 0, read back == requested. **Mode:** PIE `PlayMode_InViewPort` (1084 §5 named PIE as the comparable state). **Convergence:** 20 samples at 2 s, **spread 0 MiB** across the whole 40 s window.

| state | used | free | note |
|---|---:|---:|---|
| editor idle, no session (today, 10×2 s) | **4638 MiB** | **3254 MiB** | spread 0 |
| **PIE on `L_Arena`, lane vantage, converged (today)** | **7040 MiB** | **852 MiB** | 20×2 s, spread 0 |
| Simulate converged (today, 6×2 s) | 6967 MiB | 925 MiB | spread 7 |
| Simulate + viewport captures, peak (today) | 7110 MiB | 782 MiB | during the two bursts |
| — *`TASK-1081` baseline: editor idle* | *6455 MiB* | *1437 MiB* | *1081 §3* |
| — ***`TASK-1081` baseline: PIE converged*** | ***7399 MiB*** | ***493 MiB*** | *1081 §3, same vantage/instrument* |
| — *`TASK-1081` baseline: PIE + captures, peak* | *7498 MiB* | *394 MiB* | *91.9 % of the card* |

### ⇒ SIGNED DELTA, PIE-converged, before → after: **−359 MiB used / +359 MiB free.**
Peak-to-peak: **−388 MiB used / +388 MiB free** (7498/394 → 7110/782).

| | expected (declared before the measurement) | measured |
|---|---|---|
| `TASK-1084` grass instances | ≤ **+3 MB** (1084 §2) | — |
| `TASK-1083` D1 normal maps | **+15.5 MiB** resident (`T_Leaf_Pack1_normal` 5.33 + `T_Trunk_Pack1_normal` 10.17, 1081 §3's texture table) | — |
| **lane total** | **≈ +18.5 MB** | **not resolvable by this instrument** (see below) |
| whole-GPU occupancy at the gameplay vantage | — | **−359 MiB (headroom improved)** |

**The honest sentence, and it is the same one 1081 wrote:** the two sessions differ by **−1817 MiB at editor idle**, so the −359 belongs to **whole-machine contention** (browser/Fab/other processes), not to this lane. The lane's own predicted **+18.5 MB is ~20× below this instrument's between-session spread** and cannot be resolved by it. What the gate needed was *"not materially worse than −50 MiB, and free not below the baseline threshold"*: **free is 852 MiB against a 394–493 MiB baseline.** ⇒ **commit, do not refuse.**

**Corroborating fact, independent of the sampler:** 1081's baseline session fired the banner `Video memory has been exhausted (23.391 MB over budget)`. **Today's engine log contains ZERO occurrences of that string** (`grep -ac` = 0) across a PIE session and a Simulate session with both changes live.

⭐ **INSTRUMENT FINDING worth keeping (for the manager).** The *PIE delta* (PIE-minus-idle) reads **2402 MiB today vs 944 MiB in 1081**. That is **not** a 1.5 GB regression: 1081's session started at 6455/1437 and hit the budget ceiling, so its PIE growth was **clamped by eviction** (hence the banner). Today the card had 3254 MiB free at idle, so the match allocated its **true appetite ≈ 2.4 GB** and still left 852 free. ⇒ **the 944 MiB PIE delta in 1081 §3 understates the match's GPU appetite; on this card, headroom at the gameplay vantage is governed mostly by what else is running.**

---

## 2. SIMULATE EVIDENCE FOR `TASK-1086` (`SC-§88a`)

Both frames are **Simulate-in-Editor** (`StartPIE bSimulate=true, PlayMode_Simulate`, warmup 8 s) — ⛔ **not** a PIE-posed `CaptureViewport`, which renders the editor world (1084 §0a). `bShowUI=false`, annotations disabled (grid 0 / labels 0), FOV 90, **2764×828**, camera read back == requested on every frame.

**Label for both, exactly as the row specifies:** *"battlefield scatter trees + grass as they render in a match, post-D1 post-1084."*

| file | pose | sha256 | bytes |
|---|---|---|---|
| `.claude/pipeline/playtest-evidence/2026-09-06/TASK-1085-SIMULATE-lane.png` | 1081's lane vantage `(-24000, 0, 1400)` pitch −4 yaw 0 | `6f5d7d9c…b77c` | 3,129,835 |
| `.claude/pipeline/playtest-evidence/2026-09-06/TASK-1085-SIMULATE-hero.png` | 1084's hero vantage `(-23700, 0, 260)` pitch −10 yaw 0 (1 m in front of `PlayerStart_0`) | `4917ee9c…dc3f` | 2,323,530 |

**These are the first images ever taken of the rostered `Layers[Trees]` scatter trees.** Every earlier tree frame today (1081's lane vantage, 1083's D-BEFORE/AFTER) imaged the **15 hand-placed ring trees** in the editor world. Proof the rostered scatter is in these frames — the session's own log, seed `1182764289`, 20:28:50 UTC:
```
[BattlefieldScatter 'BP_BattlefieldScatter_C_0'] GenerateScatter seed=1182764289 mirror=rot180 layers=7 corridorHalfY=1000
[BattlefieldScatter] Layer 'Trees': placed 340 instances (target 340, sym=rot180, pairs 170, twinSkipped=0, zMismatch=0, blocking=true, meshVariants=12).
[BattlefieldScatter] Layer 'Grass': placed 24226 instances (target 36750, ... meshVariants=10).
```
⇒ the frames show **340 rostered scatter trees across 12 mesh variants** and the **post-1084 grass at 24,226 placed** (1084 measured 24,278 on a different seed — the same saturation cap, `±0.2 %`).

### Burst convergence (`SC-§88`) — reported honestly, including where it differs from 1083/1084
5 frames per pose at 1.5 s; consecutive-frame mean |ΔRGB| /255:
- **lane:** 2.27 → 2.21 → 2.16 → 2.33
- **hero:** 1.86 → 1.79 → 1.90 → 1.89

**The series is STATIONARY, not descending** — unlike 1083's (1.07 → 0.32) and 1084's (2.27 → 0.60). It is **not** an unsettled accumulation: the residual's **median |Δ| is 1.0/255** with p90 = 5–7 and only **1.5–1.9 % of pixels over 20/255**, concentrated in the animated field (lane: spread across every band; hero: rows 185–400, exactly the near-field grass strip). ⇒ **the floor is the world ticking — wind on foliage and moving units — not temporal accumulation.** The image had already settled before frame 1 (8 s warmup + script overhead); what remains does not settle at any interval.

**The measurement itself IS converged**, which is the claim that matters: the tree-line CTI across the 5 lane frames reads **5.504 / 5.420 / 5.479 / 5.415 / 5.477**, spread **0.090 = 1.64 %** of the mean. Frame 5 is the promoted one.

### The tree-line CTI, on 1083 §13's ROI method (so `TASK-1086` has a number)
Method verbatim from 1083 §13.4: canopy mask **HSV hue 40–110°, S ≥ 0.15, V ≥ 0.05**; metric = **mean Rec.709 linear luminance of sRGB-linearised masked pixels × 100** (1082's CTI units); rects in full-res pixels.

| ROI (full-res rect) | masked px | **CTI, Simulate, post-D1 post-1084** | 1083's editor-world reading (ring trees only) |
|---|---:|---:|---:|
| L dark cluster `(330,0,1000,185)` | 58,059 | **4.454** | 3.675 |
| C castle pair `(1410,0,1680,185)` | 17,235 | **5.815** | 4.523 |
| R gold pair `(1725,0,2070,185)` | 33,286 | **6.731** | 4.782 |
| **ALL tree line `(0,0,2764,185)`** | **122,519** | **5.477** (burst mean 5.459 ± 0.045) | 4.242 |

Mean sRGB of the tree line: **(63.2, 63.5, 40.7)**.

⚠️ **Read those two columns as different worlds, not as a before/after.** The right-hand column is the **editor world** (ring trees only, `SC-§88a`); the left is the **match** (ring trees **and** the 340 scatter trees). The match tree line is **+29 % brighter** than the editor-world reading — that is the instrument changing, not the material. **What survives as a number for his eye: at the lane vantage in a real match the canopy band sits at CTI ≈ 5.5, mean sRGB ≈ (63, 64, 41) — still dark, on a dusk sky.** By eye in `TASK-1085-SIMULATE-lane.png` the near ring trees read near-black while the mid-field scatter trees read a lighter olive; both carry the D1 normals.

### 🧑 THE OPTION I DID **NOT** TAKE, NAMED FOR HIM
A **true scatter-tree BEFORE** does not exist and I did not manufacture one. Producing it means **reverting the two D1 samplers → Simulate capture → re-applying them → Simulate capture** — a material-revert capture row. Per the row and `TASK-1086` cl. 2-D that is **boarded only on 🧑 his word, never pre-emptively.** I did not revert anything.

### ⇒ THE ONE-LINE QUESTION FOR `TASK-1086`
**"At the battlefield vantage — `TASK-1085-SIMULATE-lane.png` and `-hero.png`, plus your own match — are the trees still too dark?"** (yes ⇒ the manager boards Option A, re-light the dusk, knowing it also re-lights the praised background trees, `FIELD-§4`; no ⇒ ask 1 closes and cl. 3's *"name the indices to drop / want more of"* still applies.)

---

## 3. THE COMMIT

**Hash `e37c8996fa60433440caaa02b2b8ba49a7d17456`** · one commit · `git commit -F <msg> -- <explicit pathspec>` (`§25c`: never bare, never `-a`, never `add -A`). Message quotes **both** authorization sentences verbatim: *"I approve you to go with your recommendation."* (~09:00 PDT, Option D) and *"ok, I will go with your recommendation, D1"* (~09:45 PDT, D1), and **names the vendor edit** rather than burying it.

**Pre-stage fences:** `git status --short Content/Tree_Pack_1/` = **exactly two lines** (the two materials — no third line, no widen) · `git status --short Content/Maps/` **empty before and after** · index **clean** before staging · no `.git/index.lock` · `git log -1` re-read as `236b0a8` immediately before committing.

**`git show --stat HEAD` (`§25c` cl. 2) — pasted:**
```
 GitClaudeUnrealTest/.claude/pipeline/CONVENTIONS.md                                            |  35 ++-
 GitClaudeUnrealTest/.claude/pipeline/TASKBOARD.md                                              |  48 +--
 GitClaudeUnrealTest/.claude/pipeline/handoffs/TASK-1083-buildmaster.md                         | 327 +++++++++++++++++++++
 GitClaudeUnrealTest/.claude/pipeline/handoffs/TASK-1084-buildmaster.md                         | 112 +++++++
 .../playtest-evidence/2026-09-06/RosterSheet_Trees.png                                         |   4 +-
 .../playtest-evidence/2026-09-06/TASK-1083-D-AFTER.png                                         |   3 +
 .../playtest-evidence/2026-09-06/TASK-1083-D-BEFORE.png                                        |   3 +
 .../playtest-evidence/2026-09-06/TASK-1083-D-SIDE-BY-SIDE.png                                  |   3 +
 .../playtest-evidence/2026-09-06/TASK-1084-AFTER-hero.png                                      |   3 +
 .../playtest-evidence/2026-09-06/TASK-1084-AFTER-lane.png                                      |   3 +
 .../playtest-evidence/2026-09-06/TASK-1084-AFTER-mid.png                                       |   3 +
 .../playtest-evidence/2026-09-06/TASK-1084-BEFORE-hero.png                                     |   3 +
 .../playtest-evidence/2026-09-06/TASK-1084-BEFORE-lane.png                                     |   3 +
 .../playtest-evidence/2026-09-06/TASK-1084-BEFORE-mid.png                                      |   3 +
 .../playtest-evidence/2026-09-06/TASK-1084-INSTRUMENT-editorworld-vs-simulate-lane.png         |   3 +
 .../playtest-evidence/2026-09-06/TASK-1084-SIDE-BY-SIDE-hero.png                               |   3 +
 .../playtest-evidence/2026-09-06/TASK-1084-SIDE-BY-SIDE-lane.png                               |   3 +
 .../playtest-evidence/2026-09-06/TASK-1084-SIDE-BY-SIDE-mid.png                                |   3 +
 .../playtest-evidence/2026-09-06/TASK-1085-SIMULATE-hero.png                                   |   3 +
 .../playtest-evidence/2026-09-06/TASK-1085-SIMULATE-lane.png                                   |   3 +
 GitClaudeUnrealTest/Content/Data/DA_BattlefieldScatter.uasset                                  |   4 +-
 GitClaudeUnrealTest/Content/Tree_Pack_1/Meches/Mobile_Tree_1/M_Pack1_Leaf_Mobile.uasset        |   4 +-
 GitClaudeUnrealTest/Content/Tree_Pack_1/Meches/Mobile_Tree_1/M_Pack1_Trunk_Mobile.uasset       |   4 +-
 23 files changed, 554 insertions(+), 29 deletions(-)
```
**The committed set was diffed line-for-line against the intended pathspec: identical, 23/23, no strays** (`git show --name-only HEAD | sort` vs the pathspec file). Verified **after** the commit, not only on the index before it.

### LFS verification — index oid vs working `sha256`, ⛔ never by size (`SC-§68`)
All 19 binaries (`*.uasset` and `*.png` are both `filter=lfs`) verified **twice**: staged pointer before the commit, and `git show HEAD:<path>` after. **19/19 match, 0 mismatches.**

| file | LFS oid == working sha256 | size |
|---|---|---:|
| `M_Pack1_Leaf_Mobile.uasset` | `58957692…7283` | 20,939 |
| `M_Pack1_Trunk_Mobile.uasset` | `977b4b13…13d2` | 23,457 |
| `DA_BattlefieldScatter.uasset` | `798c3579…1c19` | 9,098 |
| `RosterSheet_Trees.png` | `49d1631d…437b` | 643,895 |
| `TASK-1083-D-BEFORE.png` | `dbb668fc…6d80` | 2,673,655 |
| `TASK-1083-D-AFTER.png` | `10287237…e340` | 2,685,125 |
| `TASK-1083-D-SIDE-BY-SIDE.png` | `c13a935c…1678` | 2,148,918 |
| `TASK-1084-BEFORE-lane.png` | `546d8c8b…271e` | 3,727,443 |
| `TASK-1084-BEFORE-hero.png` | `1b13f0bb…67d2` | 2,917,193 |
| `TASK-1084-BEFORE-mid.png` | `1ee69c5d…15b8` | 4,641,223 |
| `TASK-1084-AFTER-lane.png` | `f3cce064…7a5c` | 3,848,417 |
| `TASK-1084-AFTER-hero.png` | `a1e233a0…7b7c` | 2,986,111 |
| `TASK-1084-AFTER-mid.png` | `f7e6a33b…de82` | 4,754,577 |
| `TASK-1084-SIDE-BY-SIDE-lane.png` | `80cacaec…aa84` | 6,092,104 |
| `TASK-1084-SIDE-BY-SIDE-hero.png` | `c3703785…0e8e` | 4,576,684 |
| `TASK-1084-SIDE-BY-SIDE-mid.png` | `4c9812f4…4e54` | 7,911,639 |
| `TASK-1084-INSTRUMENT-editorworld-vs-simulate-lane.png` | `e9272457…c876` | 5,807,882 |
| `TASK-1085-SIMULATE-lane.png` | `6f5d7d9c…b77c` | 3,129,835 |
| `TASK-1085-SIMULATE-hero.png` | `4917ee9c…dc3f` | 2,323,530 |

**`main` is 4 ahead of `origin/main`. ⛔ NOT PUSHED** (no push was asked for).

**No C++ changed ⇒ no compile was run, and none was required.** Assets, evidence and records only.

---

## 4. WHAT STAYED DIRTY, AND WHOSE IT IS

Nothing of mine, apart from the board line I write after this file. Everything below is another row's and was **not staged, not restored, not tidied** (`§25c`):

- `Content/Blueprints/BP_HeroCharacter.uasset` (M) — appeared **during** my run; ⭐ `TASK-1094`'s.
- `Tools/ArtPipeline/pipeline_manifest.json` (M) — ⭐ `TASK-1091`/`1093`'s.
- `Content/Characters/SK_MainCharacter.uasset`, `Content/Materials/Instances/MI_MainCharacter_PBR.uasset`, `Content/Textures/T_MainCharacter_{D,N,ORM}.uasset`, `Content/RawAssets/**` (untracked) — ⭐ `TASK-1093`/`1094`'s.
- `handoffs/TASK-1091-artist.md`, `handoffs/TASK-1093-artist.md`, 12 × `MainCharacter_*.png` (untracked) — the character lane's.
- `handoffs/TASK-1098-buildmaster.md` (untracked) — the **next doc-host**'s, deliberately left.
- `TASKBOARD.md` — clean at commit time, re-dirtied by my own `done` line + the 1083/1084/1086 line edits immediately after.

The editor's Git provider **did** stage a stray during the earlier killed attempt (`Content/Characters/SK_MainCharacter.uasset`); the orchestrator unstaged it before I started and **the index was clean when I staged**. No stray appeared in mine — verified by the after-the-fact `git show --name-only` diff, not merely by the index.

---

## 5. FENCES — verified after the last engine call

- ✅ **`L_Arena` never saved.** `is_dirty` false; `git status --short Content/Maps/` **empty before and after**; sha `1f78419d…5622` intact (it is not in the commit).
- ✅ **No asset saved by me at all.** `save_assets` never called (neither by path nor `[]`); `DA_BattlefieldScatter`, both `M_Pack1_*_Mobile` and `L_Arena` all read `is_dirty: false` after my last session.
- ✅ **Play sessions:** `IsPIERunning` **false** before I started (no concurrent `TASK-1094` session to wait on). I started and stopped **two** short sessions — PIE 13:25:18–13:26:12 (~55 s) and Simulate 13:26:5x–13:28:3x (~100 s) — and **stopped only my own**; `IsPIERunning` read back **false** at close-out. No `EditorAssetSubsystem` call was made while any session ran.
- ✅ No `checkout` / `restore` / `stash` / `reset` / `clean`; no push; no amend; no force.
- ✅ Editor left **up, PID 20564**, MCP answering, `L_Arena` loaded, no session running, nothing of mine dirty.
- ⚠️ Instrument note for the next scripter: `AssetTools.write_file` resolves a **relative** path against the *engine binaries* directory, not the project — pass the absolute project path or it fails the allowed-root check. Raw base64 bursts are in `Saved/TreeSurvey/t1085_{lane,hero}_0..4.txt` (gitignored).

---

## 6. THE LADDER — the commit does NOT close this lane (`SC-§94` cl. B)

**Rung 1** properties (the DA lists 12 meshes) · **rung 2** reachability (the scatter reads the DA) · **rung 3** *this commit* — the change is live, measured, and in git · **⛔ RUNG 4 = ⭐ `TASK-1086`, 🧑 JONATHAN'S EYE.** Both can be green while the field still looks too dark to him, and *"too dark"* was his judgement in the first place. `TASK-1086` is now **his to sit**, with the two Simulate frames above, the numbered `RosterSheet_Trees.png`, and 1083's index→path table.

## 7. FOR THE MANAGER (not executed, not boarded by me)

1. **`SC-§88`/`SC-§88a` addendum candidate:** in **Simulate**, a burst does not converge to a still frame — the world ticks. The honest discipline is *"the series is stationary and the **metric** is converged"* (CTI spread 1.64 % here), not *"consecutive |Δ| → 0"*. Recorded with numbers in §2.
2. **1081 §3's `944 MiB` PIE delta is a clamped figure** (see §1's instrument finding). If a later row budgets against it, it will under-budget by ~1.4 GB.
3. `T_Trunk_Pack1` and `T_Trunk_Pack1_normal` are **1414² uncompressed** (1414 is not a multiple of 4): ~20.3 MiB for what BC-compressed 2048² would cost ~8 MiB. A resize is a free ~12 MiB — vendor-pack fence, his call.
4. 1084's optional hygiene stands: carrying `24500` instead of `36750` in the DA would drop ~0.4 s of rejected attempts per match start — folds into whatever DA-tune row his `TASK-1086` answer buys, not a row of its own.
