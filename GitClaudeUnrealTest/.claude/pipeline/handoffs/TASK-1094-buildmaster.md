# TASK-1094 — [CHAR-SHIP] the hero is a knight, and he was watched WALKING and FIGHTING before a byte was committed

**build-master · 2026-09-06 · marker `TASK-1094-CHAR-SHIP`**
**Status → `done` · commit `9daa6418b73531fe4d4fa220ea25d463749f99e4` (`9daa641`) · main 5 ahead of `origin/main`, ⛔ NOT pushed**

⚠️ **Born OUTSIDE its own commit (`TL-§5e` cl. 7):** this file did not exist when `9daa641` was made and is therefore
not in it. `handoffs/TASK-1091-artist.md` and `handoffs/TASK-1093-artist.md` — the two reports that describe the bytes —
ARE in it, deliberately, so the report and the asset cannot drift apart.

---

## 1. THE SWAP — ONE PROPERTY, READ BACK THREE WAYS

`Content/Blueprints/BP_HeroCharacter.uasset` · `CharacterMesh0.SkeletalMeshAsset`:
**`SKM_Quinn_Simple` → `/Game/Characters/SK_MainCharacter`**. Saved by explicit package path
(`EditorLoadingAndSavingUtils.save_packages([BP_HeroCharacter], False)` — never the empty-list save-all).

### (A) Read back from the ASSET after the save (`SC-§94` cl. A)

| property | value | |
|---|---|---|
| `SkeletalMeshAsset` | **`/Game/Characters/SK_MainCharacter.SK_MainCharacter`** | ✅ swapped |
| `AnimClass` | `/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed.ABP_Unarmed_C` | ⛔ untouched |
| `AnimationMode` | `AnimationBlueprint` | ⛔ untouched |
| `RelativeLocation` | `(0, 0, −89)` | ⛔ untouched |
| `RelativeRotation` | `pitch 0, yaw −90, roll 0` | ⛔ untouched |
| `RelativeScale3D` | `(1, 1, 1)` | ⛔ untouched |
| `OverrideMaterials` | `[]` | ⛔ untouched |

The pre-swap read was byte-for-byte the same on every row except the mesh — i.e. the values match `TASK-1093` §6 exactly.

### (B) Read back from the LIVE PIE ACTOR (`BP_HeroCharacter_C_0` in `UEDPIE_0_L_Arena`)

Same seven values, plus: component world scale `(1,1,1)`, resolved material `MI_MainCharacter_PBR`
(from the slot, not an override), anim instance `ABP_Unarmed_C_0`, capsule `r 42 / half-height 96`.

### (C) A BYTE SCAN OF THE SAVED PACKAGE — the instrument that does not consult the editor's memory

`Content/Blueprints/BP_HeroCharacter.uasset` on disk, sha256 `9b9f3624…` → **`e8c8adf1d17a5f1b…`**:

```
SK_MainCharacter   ×2      SKM_Quinn_Simple   ×0
ABP_Unarmed        ×2      AM_ComboAttack     ×2
```

⭐ The old mesh name is **gone from the file**, and both animation references are **still in it**. A property read-back can
be satisfied by an object the editor is holding; this cannot.

---

## 2. RUNG 4 — THE ROW. ⛔ HE WALKS AND ⛔ HE FIGHTS.

**How he was driven, stated first so the limit is not buried.** MCP has **no key/LMB injection lane**. The hero was driven
through his own shipped entry points via the editor's Python remote-execution channel (`bRemoteExecution=True`,
`Config/DefaultEngine.ini:7`) — a per-tick `Pawn::AddMovementInput(+X, 1.0)` and `AHeroCharacter::DoMeleeAttack()`, which
are **the exact functions the WASD and LMB bindings call**. ⛔ **What was therefore NOT exercised is the input-binding
layer itself** (`IMC_Hero` → `IA_Move` / `IA_Attack`). That layer is unchanged by this task and was already shipped; but
"a key press reaches the hero" is **NOT OBSERVED** here, in those words, and it is named for 🧑 his sitting.

### (a) ⛔ HE WALKS — ✅ OBSERVED

| instrument | reading |
|---|---|
| ticks sampled | **400** |
| ground speed | **500.0 uu/s, min = max = mean** (zero spread — sustained locomotion, not a spike) |
| distance travelled | **3,337 uu** across the arena, actor yaw rotating to face travel |
| gait, mid-stride | `foot_l` world z **38.5** (lifted) vs `foot_r` **8.8** (planted) — ⛔ a T-pose has both at the same height |
| lean | `head` world z **169.4 idle → 158.1 running** |

**Captures:** `TASK-1094-A-walks-knight-mid-stride-front.png` · `TASK-1094-B-walks-second-frame-different-pose.png`
— two frames, two visibly different gait poses, knight walking toward the camera in front of the blue castle.

### (b) ⛔ HE FIGHTS — ✅ OBSERVED, and the montage is the NEW mesh's

| instrument | reading |
|---|---|
| active montage on the new mesh's anim instance | **`AM_ComboAttack` on 257/257 sampled frames** of a continuous swing (and 261/261 on an earlier run) |
| swing arc | `hand_r` world z **98.4 → 215.8 uu** (a **117 uu** overhead arc) |
| body follow-through | `head` world z dips to **154.9** |
| synchronous check at the instant of the call | `get_current_active_montage()` = `/Game/Variant_Combat/Anims/AM_ComboAttack.AM_ComboAttack`, `montage_is_playing` = **True**, position 0.0 |

**Captures:** `TASK-1094-C-fights-AM_ComboAttack-swing1.png` · `TASK-1094-D-fights-AM_ComboAttack-swing2.png`.

### (b2) ⭐ AND THE MELEE LANDS — ✅ OBSERVED, and it is the shipped number

One swing at **110 uu** into a `BP_Unit_Footman_C` (hero `TeamId.BLUE`, target `TeamId.RED`):

```
target current_hp   80.0  ->  60.0      on the frame of the DoMeleeAttack() call
```

**Exactly −20**, which is `TASK-017`'s shipped `MeleeDamage` (`HeroCharacter.h:1208`). Read off the target actor's own
property, not off a log line.

⚠️ **One later swing did NOT damage** a `BP_Unit_MilitiaMob_C` at 115 uu even though the montage played. Not chased —
`DoMeleeAttack`'s sweep runs `GatherHostileAgents(..., ESiegeVeilPolicy::SuppressVeiled, &Vision)`, so a veiled/fog-hidden
target is legitimately skipped, and the hero had just been teleported there. **Declared rather than hidden**; it is a
combat-visibility question, not a mesh question, and nothing about it involves `SK_MainCharacter`.

### (c) ⛔ NO T-POSE · NO EXPLOSION · NO SHRINK · NO FLOOR SINK — ✅ OBSERVED

`TASK-1093` §4 caught a **100× root-bone scale** before handoff, and §4 says plainly that it would have surfaced *here* as
a 2 cm figurine the moment an animation wrote `root` back to scale 1. It did not:

| | reading | expected |
|---|---|---|
| `root` bone, world z | **9.15** | actor z 98.15 + component −89 = **9.15** ✅ (a 100× survivor reads ~915 or ~0.09) |
| `pelvis` / `head`, world z | **102.1 / 168.9** | upright, ~190 uu figure ✅ |
| component world scale | **(1,1,1)** | ✅ |
| capsule | **r 42 / half-height 96** | unchanged ✅ |
| feet | mesh feet-at-z0, same convention as Quinn | plant where Quinn's planted ✅ |
| skinning | captures A–D: cloak, surcoat, cross, sabatons all coherent; no stretched verts, no inverted normals | ✅ |

⚠️ **One reading that looked like a defect and is NOT one**, recorded so nobody re-finds it: immediately after a forced
`ResetHero()` **in the same frame as a death**, one bone probe read `head z 29.1` *below* `foot z 178.4` — an upside-down
figure. Re-read two seconds later: `root 9.1 / pelvis 102.1 / head 168.9`, upright. It is a same-frame ragdoll-transition
artefact of my own instrumentation, not the mesh.

### (d) VRAM (`FIELD-§3`) — reported even though the honest answer is "the instrument cannot see it"

**The knight's own cost, computed from format facts** (the only instrument with the resolution):

| item | resident |
|---|---|
| `T_MainCharacter_D` 2048² DXT1 + mips | 2.67 MiB |
| `T_MainCharacter_ORM` 2048² DXT1 + mips | 2.67 MiB |
| `T_MainCharacter_N` 2048² BC5 + mips | 5.33 MiB |
| skeletal render data (28,086 verts × ~32 B + 23,990 tris × 6 B) | 0.99 MiB |
| **total added** | **≈ 11.7 MiB** — inside `TASK-1093`'s 12–14 MB prediction |

**The `TASK-1081` instrument** (`nvidia-smi`, whole-GPU DXGI budget, ⛔ NOT the streaming pool), 6–10 samples at 2 s each,
all converged with zero or near-zero spread:

| state | used | free |
|---|---|---|
| another agent's `Simulate` running (my first sample — I had read `IsPIERunning` as false moments before) | 7040 MiB | 852 MiB |
| **this row's PIE #1, converged** | **6715–6725 MiB** | 1167–1177 MiB |
| **this row's PIE #2 (fresh), converged** | **7327–7343 MiB** | 549–565 MiB |
| editor idle after, converged | 4456–4521 MiB | 3371–3436 MiB |

⭐ **Two PIE sessions minutes apart on the same content differ by 615 MiB — 53× the quantity being measured — and PIE #1
read LOWER than the pre-swap sample.** ⛔ **No delta is claimed from this instrument.** `TASK-1081`'s conclusion holds
unchanged: it measures whole-machine contention. The number to carry forward is the computed **≈ 11.7 MiB**.

⚠️ One caveat on the pre-swap sample: `TASK-1093` left the Skeletal Mesh editor tab for `SK_MainCharacter` **open**, so
the knight's textures were **already resident** before the swap. A true before/after was therefore never available in
this editor session even in principle.

---

## 3. EVIDENCE — every file, with what it shows

All under `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\.claude\pipeline\playtest-evidence\2026-09-06\`.
All 1920×1080, captured by `HighResShot` from the live PIE viewport (`SC-§88` — bursts, not one frame).

| file | shows |
|---|---|
| `TASK-1094-A-walks-knight-mid-stride-front.png` | ⭐ **the money frame** — the knight walking toward the camera in front of the blue castle, great helm, red Templar cross, tattered cloak, left leg forward mid-stride, correct scale against castle and grass |
| `TASK-1094-B-walks-second-frame-different-pose.png` | same run, a different gait pose (proof of motion, not one lucky frame) |
| `TASK-1094-C-fights-AM_ComboAttack-swing1.png` | mid-swing at the castle gate, arm raised, cloak flaring |
| `TASK-1094-D-fights-AM_ComboAttack-swing2.png` | the swing one beat later — a different arm position |
| `TASK-1094-E-OBSERVATION-death-camera-roll-90deg.png` | ⚠️ the death-camera roll (§5.6), kept because the observation needs its frame |

Also committed in `9daa641` (`TASK-1091`/`TASK-1093`'s, promoted here so they ship with the asset):
`MainCharacter_preview_{front,side,back}.png` · `MainCharacter_rig_bind_{front,side,back}.png` ·
`MainCharacter_rig_bind_{front,side}_bones_xray.png` · `MainCharacter_rig_testpose_{front,threequarter}.png` ·
`MainCharacter_ue_persona_refpose.png` · `MainCharacter_ue_thumbnail_SK_MainCharacter.png`
(the last one is the **empty checkerboard** of the pre-fix 100×-scale asset — `TASK-1093` §4's own exhibit, kept
on purpose as the record of the defect).

---

## 4. THE COMMIT

**`9daa6418b73531fe4d4fa220ea25d463749f99e4`** · 31 files · 598 insertions, 2 deletions · on top of `TASK-1085`'s
`e37c8996` (which landed WHILE this row was in PIE — `git log -1` was re-read immediately before staging, per the brief).

### The pathspec — DERIVED from the two handoffs' WRITES lists, ⛔ never from the index (`§25c`)

`git commit -F <msg> -- <31 explicit paths>`. Staged set **BEFORE**: **empty** (`git diff --cached --name-only` returned
nothing — the orchestrator's earlier unstage held). Committed set **AFTER** = the 31 paths, verified by
`git show --stat HEAD`, **zero strays**:

```
Content/Blueprints/BP_HeroCharacter.uasset                              (mine — the swap)
Content/Characters/SK_MainCharacter.uasset                              (TASK-1093 WRITES)
Content/Textures/T_MainCharacter_{D,N,ORM}.uasset                       (TASK-1093 WRITES)
Content/Materials/Instances/MI_MainCharacter_PBR.uasset                 (TASK-1093 WRITES)
Content/RawAssets/Characters/MainCharacter.fbx                          (TASK-1093 raw-source rule)
Content/RawAssets/Concepts/MainCharacter.png                            (TASK-1093 raw-source rule)
Content/RawAssets/Textures/MainCharacter/T_MainCharacter_{D,N,ORM}.png  (TASK-1091 §2)
Tools/ArtPipeline/pipeline_manifest.json                                (TASK-1091 §2, manager-ratified, +25 lines)
.claude/pipeline/handoffs/TASK-109{1,3}-artist.md
.claude/pipeline/playtest-evidence/2026-09-06/  ×17 PNG                 (12 theirs + 5 mine)
```

### LFS — verified **oid vs sha256**, ⛔ never by size (`SC-§68`)

28 of the 31 files carry `filter: lfs` (`git check-attr`); the other 3 are text (`pipeline_manifest.json`, the two `.md`).
For each of the 28, the index blob's `oid sha256:` was compared against `sha256sum` of the working file.
**28 / 28 MATCH · 0 mismatches.** The load-bearing rows:

| file | index oid = working sha256 (first 16) | why it mattered |
|---|---|---|
| `BP_HeroCharacter.uasset` | `e8c8adf1d17a5f1b` | ✅ the **post**-swap bytes, not the pre-swap `9b9f3624…` |
| `SK_MainCharacter.uasset` | `d6e4dfab6a9c7e1a` | ✅ ⭐ **the stale-index worry in `TASK-1093` §6 is REFUTED by measurement** — the editor's Git provider had auto-staged this file earlier today and the orchestrator unstaged it; the oid now equals the working **cm-scale** file, ⛔ not the pre-fix 100×-scale blob |
| `MainCharacter.fbx` | `d4582d7d1c1ec309` | ✅ = `TASK-1093`'s declared sha256, and md5 `90526b41…` = the asset's own `AssetImportData` |
| `MainCharacter.png` (concept) | `77f845dbd1e043b1` | ✅ = `TASK-1093`'s declared sha |
| `MI_MainCharacter_PBR.uasset` | `c69e2ea40f82bbec` | |
| `T_MainCharacter_{D,N,ORM}.uasset` | `20842c09…` / `a6a0a72a…` / `eecd23f0…` | |

### ⭐ THE TWO FBX FILES — what happened to the other one

Both exist on disk and they are **different files**:

| path | md5 | size | disposition |
|---|---|---|---|
| `Content/RawAssets/Characters/MainCharacter.fbx` | **`90526b4193054d7e4ba4ea02e10cb73a`** | 1,374,924 B | ✅ **COMMITTED** — it is the file `SK_MainCharacter.AssetImportData` names, and it is `TASK-1093`'s cm-scale rigged export |
| `Content/RawAssets/MainCharacter.fbx` | `d605148553b3c231c9f155c08cbf3256` | 986,684 B | ⛔ **LEFT UNTRACKED, on purpose** — `TASK-1091`'s pre-rig **static** Stage-2 export at the old flat path; superseded, no asset imports from it |

⛔ **I did not delete it.** Reasons for leaving it out rather than adding it: it is not any asset's import source, and a
second tracked file with the same basename at a different path is exactly the ambiguity that would bite the next
same-path reimport (`reimport_meshes.py`). 🙋 **For the manager:** if the raw Stage-2 static output is wanted in history,
that is a boardable one-liner — the Stage-2 donor (`Cache/MainCharacter/meshy_raw.glb`) is gitignored, so re-deriving it
costs 0 credits but needs the cache to survive.

### What stayed dirty, and whose it is

```
 M .claude/pipeline/TASKBOARD.md                        shared — edited by me (§6 below), ⛔ never staged by a lane
?? .claude/pipeline/handoffs/TASK-1085-buildmaster.md   ⛔ TASK-1085's build-master
?? .claude/pipeline/handoffs/TASK-1098-buildmaster.md   ⛔ another lane's
?? Content/RawAssets/MainCharacter.fbx                  the superseded FBX above
```

⛔ Nothing belonging to `TASK-1083/1084/1085` was staged, restored or tidied. Their files
(`Tree_Pack_1` Mobile materials, `DA_BattlefieldScatter.uasset`, `RosterSheet_Trees.png`, `TASK-108{3,4,5}-*.png`,
`CONVENTIONS.md`) had already gone in with their own `e37c8996`; I touched none of them.

**`main` is 5 ahead of `origin/main`. ⛔ NOT PUSHED.**
**No C++ changed in this lane ⇒ ⛔ nothing was compiled and no test suite was run.** Saying it rather than leaving a gap.

---

## 5. RESIDUALS — 🧑 FOR HIS EYE, reported and ⛔ NOT fixed

1. **`TeamRegion` slot ABSENT** on `SK_MainCharacter` (slots = `["MainCharacterPBR"]`). Zero faces by the
   `TASK-1091` §5b / `TASK-1092` no-team-tint decision; UE creates no slot for a material with no faces. The hero is not
   a `SummonedUnit` and never runs the slot-0 recolour, and `overrideMaterials` is `[]`. **A declared residual, not a defect.**
2. **Helmet height.** The knight is 191.75 uu vs Quinn 180.1 — crown ~**12 uu** higher. Capsule top sits at actor z + 96;
   the crown lands just under it and did not visibly poke through in any of A–D. Cosmetic; 🧑 his call.
3. **Foot IK, `TASK-1093` §5.4 — the thing that could only be seen here.** The `ik_foot_*` bones retarget in `Animation`
   mode, so at runtime they carry the anim's Manny-sized values, ~8 uu inboard of the knight's wider stance. **Across the
   walk captures nothing reads wrong at gameplay distance** — feet plant, no skating, no visible pull-in. A close-camera
   judgement is 🧑 his; I did not get a boot-level crop.
4. **Gold is gone and the rear cross roundels are absent** (`TASK-1091` §3 — 3 HIT / 8 PARTIAL / 1 MISS, his approval
   given with that report in hand). Nothing changed here; restated so it is not rediscovered as new.
5. **`PA_Mannequin`** is the physics asset — Quinn's, a read-only vendor donor, Quinn-sized on a 6 % larger body. Kept
   deliberately: a skeletal mesh with no physics asset has no mesh collision, and changing it would silently change
   whatever traces the hero's mesh today. One property, reversible.
6. ⚠️ **OBSERVATION, ⛔ not a finding of this lane (capture E).** When the hero dies, the player camera ends up
   **rolled ≈ 90°** (`control rotation roll 89.9`) and **stays rolled after `ResetHero()`** — the whole frame is on its
   side. Seen only because this row deliberately walked the hero into the RED army at 500 uu/s and he was killed twice.
   ⛔ It reproduces from the death path, ⛔ not from the mesh swap, and I did not chase it. 🙋 Boardable if his own
   playtest ever shows it.
7. **`importedMaterialSlotName = None`** on the one slot (`TASK-1093` §5.7) — only matters for a future same-path
   reimport's slot matching; with one slot it cannot mis-order.

---

## 6. WHAT I DID TO THE BOARD AND THE EDITOR

- `TASKBOARD.md`, `Edit` tool only, one line each: **`TASK-1094` → done** · **`TASK-1093` → `done — shipped 9daa641`** ·
  **`TASK-1091` → `done — shipped 9daa641`** · **`TASK-1092`** got `— shipped 9daa641` appended. ⛔ `TASK-1083/1084/1085`
  lines untouched.
- **Editor:** left **UP** (PID 20564), MCP `127.0.0.1:8000` answering, **PIE stopped** (`StopPIE` called on both of my
  sessions; I started two and stopped two, and ⛔ never stopped one I did not start).
- I **waited** for `TASK-1085`'s session before starting mine. ⚠️ Their `Simulate` was up during my first
  `EditorAssetSubsystem` call, which returned `does_asset_exist = False` for a file that is plainly on disk — exactly
  `TASK-1093` §8.1. **The save went through anyway** because `EditorLoadingAndSavingUtils.save_packages()` is **not**
  behind that subsystem. ⭐ Worth keeping: that is a save path that survives another agent's PIE.
- **`L_Arena` never opened, never saved, not staged.** No actor was placed. Nothing of `TASK-1093`'s five assets was
  re-saved. The `SK_MainCharacter` Persona tab was left open and untouched.
- The revision-control provider was **not** changed.

### ⭐ ONE OPERATIONAL FINDING WORTH THE NEXT AGENT'S TIME

**The editor's Python remote-execution channel is live and it is the missing PIE-driving lane.**
`Config/DefaultEngine.ini:7` carries `bRemoteExecution=True`; connecting with the engine's own
`Engine/Plugins/Experimental/PythonScriptPlugin/Content/Python/remote_execution.py` gives full in-editor Python
**during PIE**, which MCP's toolsets do not. Two idioms did all the work here and neither is obvious:

1. **A per-tick driver**: `unreal.register_slate_post_tick_callback(fn)` keeps running **between** remote calls, so a
   Python-driven `AddMovementInput` / `DoMeleeAttack` actually ticks the game. A `sleep` loop inside one call cannot —
   it blocks the game thread and nothing advances.
2. **Screenshots to disk**: `execute_console_command(world, "HighResShot 1920x1080")` writes straight to
   `Saved/Screenshots/WindowsEditor/`. MCP's `CaptureEditorImage` returns ~1.36 M characters of base64 and blows the
   tool-result budget on a single frame.

⚠️ **And one trap**: `unreal.Vector` values returned by `get_actor_location()` are **pooled — successive calls can hand
back the same buffer**, so a "saved" location silently becomes another actor's. Copy immediately
(`unreal.Vector(v.x, v.y, v.z)`) or your distances are fiction. It cost me two bad placements before I saw it.

---

**Ladder (`SC-§94` cl. B):** rung 1 (imported) and rung 2 (referenced) were `TASK-1093`'s. **This handoff reaches rung 4 —
he walks, he fights, and the swing takes 20 HP off a real enemy.** Rung 5 — *does it look right* — is 🧑 his.
