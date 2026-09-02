# TASK-733 — `A_SiegeBiped_Climb` (art-director) — LADDER-REDESIGN [LADDER-2]

**Law cited:** `TOWER-§0` · `TOWER-§8.1` (the promotion clause) · `TOWER-§8.3` (the pinned climb line) ·
CONVENTIONS *"Skeletal rig & animation workstream (M7)"* → the shared-skeleton `A_SiegeBiped_<Action>` clause
and the **sanctioned Blender-side animation export path** (`TOWER-§`/TASK-229: UE 5.8's own retarget exporter
is root-only, so animation FBXs are exported from Blender for direct import onto the existing skeleton).

---

## ⛔ READ FIRST — WHAT THIS TASK DID **NOT** DO

1. ⚠️ **The `/Game/` asset does NOT exist yet. The Unreal Editor is DOWN.**
   PID 5584 is gone, no `Unreal*` process is running, nothing is listening on `:8000`, and
   `list_toolsets` fails with *"Unable to connect"*. I did **not** boot the editor or a headless
   commandlet — the orchestrator holds Jonathan's grant and owns that process's lifecycle, and
   `CLAUDE.md`'s hard gate says to report an unreachable editor rather than route around it.
   ⇒ **The import is one step, fully specified in §6 below, and it is the first editor action of
   whichever task next opens the editor (TASK-739 or TASK-742).**
2. ⛔ **`SK_Footman_Skeleton` was NOT opened, modified or saved.** Nothing in `/Game/` was touched at all.
3. ⛔ **`ABP_Footman` was NOT touched.** Wiring the clip into the locomotion machine is **TASK-739's**.
4. ⚠️ **DELIVERING THIS CLIP DOES NOT MAKE UNITS CLIMB.** The traversal (TASK-738/734) and the ABP
   state (TASK-739) do. A landed clip is **not** a landed feature.
5. ⛔ No `.umap` was opened or saved. No C++, no compile, no Git.

---

## 1. WHAT SHIPPED

| Thing | Path / value |
|---|---|
| **Source FBX** (the deliverable) | `Content/RawAssets/Characters/Anims/SiegeBiped_Climb.fbx` — 167,660 bytes |
| **Target UE asset** (⚠️ not yet created) | `/Game/Characters/Anims/A_SiegeBiped_Climb` |
| **Skeleton it binds** | `/Game/Characters/SK_Footman_Skeleton` — the single shared 21-bone SiegeBiped rig |
| **Covers** | `SK_Archer` · `SK_Longbowman` · `SK_Wizard` (and the whole rigged fleet) — ⛔ **ONE clip, not three** |
| **Authoring tool** (new, checked in) | `Tools/ArtPipeline/author_climb_anim.py` |
| **Verification artefacts** | `Tools/ArtPipeline/Cache/SiegeBipedClimb/climb_report.json` · `climb_fbx_readback.json` · 5 preview strips (`Cache/` is gitignored) |

**FBX contents, read back off the file cold (⛔ not trusted from the export call):**
21 bones, names exactly `root · pelvis · spine_01..03 · neck_01 · head · {clavicle,upperarm,lowerarm,hand,thigh,calf,foot}_{l,r}` ·
one action · 17 keys (FBX frames 1–17) · 219 fcurves · **60 fps** · **zero mesh objects** (animation-only,
matching the shipped `retarget_meshy_to_siegebiped.py` export block verbatim except `object_types={'ARMATURE'}`).

---

## 2. ⭐ THE NUMBER TASK-739 CANNOT PROCEED WITHOUT

| Quantity | Value |
|---|---|
| **Advance per loop, ALONG the climb line** | ⭐ **90.00 uu** |
| Advance per loop, **VERTICAL component** | **87.31 uu** (= 90 × sin 75.9638°) |
| Loop length | **16 frames @ 60 fps = 0.266667 s** |
| **Native climb speed along the line** | ⭐ **337.50 uu/s** |
| Native vertical speed | 327.42 uu/s |
| Strokes per loop | 2 (contralateral: left hand + right foot, then the mirror) |
| Advance per stroke | 45.00 uu |

### ⛔ The RateScale formula — use the ALONG-LINE pair, not the vertical pair

```
RateScale = LadderClimbSpeedUU / 337.5
```

`LadderClimbSpeedUU` is the speed **along the pinned line** (TASK-738 interpolates Foot→Top at it;
1236.93 / 350 = 3.53 s, which is exactly `TOWER-§9.3`'s exposure figure) — so it pairs with the
**along-line** 337.5, ⛔ **not** with the vertical 327.42. Mixing the two produces a silent 2.99 % skate.

**At the shipped `LadderClimbSpeedUU = 350.f`: `RateScale = 350 / 337.5 = 1.03704`.**

⚠️ If TASK-738 ships a rate other than 350, recompute — ⛔ do not carry 1.037 forward.

---

## 3. ⭐ HOW IT HOLDS UP ACROSS A RANGE OF PLAYBACK RATES

**The contract is the FORMULA, not a baked duration.** Three properties make the rate-decoupling real
rather than asserted:

1. **The root track is CONSTANT** (lean + standoff only — no cyclic bob, no cyclic surge). The clip
   contributes **zero translation and zero bob at any rate**, so nothing strobes when it is sped up.
2. **A gripping limb's LOCAL travel is exactly −90 uu per loop along the line.** That is precisely what
   the traversal's +90 uu per loop cancels — at **any** RateScale satisfying the formula. Contact is
   therefore exact at every rate, not tuned for one.
3. **No inertial or ballistic secondary motion.** Nothing in the clip only reads at one speed.

**Measured cadence across the plausible band** (strokes/s = Speed / 45; ascent = 1236.93 / Speed):

| `LadderClimbSpeedUU` | RateScale | Loop | Strokes/s | Ascent | Reads as |
|---|---|---|---|---|---|
| 150 | 0.444 | 0.600 s | 3.3 | 8.25 s | brisk human climb |
| 225 | 0.667 | 0.400 s | 5.0 | 5.50 s | fast scramble ✅ |
| 300 | 0.889 | 0.300 s | 6.7 | 4.12 s | hard scramble ✅ |
| **350 (shipped)** | **1.037** | **0.257 s** | **7.8** | **3.53 s** | urgent siege scramble ✅ |
| 450 | 1.333 | 0.200 s | 10.0 | 2.75 s | at the edge of legibility |
| 600 | 1.778 | 0.150 s | 13.3 | 2.06 s | ⚠️ limbs stop resolving as strokes |
| 900 | 2.667 | 0.100 s | 20.0 | 1.37 s | ⚠️ blur |

⇒ **Declared readable band: RateScale ≈ 0.4 – 1.4 (≈ 135 – 470 uu/s).** The shipped 350 sits comfortably
inside it. Contact stays exact above 1.4; what degrades is only whether the eye can resolve individual
strokes.

### ⚠️ THE MEASURED FINDING HANDED OVER RATHER THAN HIDDEN

**At 350 uu/s the climber makes 7.8 limb strokes per second — and ⛔ no authoring choice can change that.**
Strokes/s = Speed / stroke-advance, and the stroke advance is capped by the rig's own reach
(measured: arm 64.8 uu, leg 78.1 uu ⇒ a hard ceiling near 47 uu/stroke; I shipped 45). **Any** clip
authored for 350 uu/s runs at ~7.5 strokes/s. This matters because `TOWER-§9.3`'s survivability problem
pushes the rate **upward**: at 600 uu/s the ascent survives 1 Longbowman shot instead of 2, but the
animation stops reading. ⇒ **If the exposure has to shrink, shortening the ladder is visually free and
raising the rate is not.** ⛔ That is a flag for the manager/Jonathan, ⛔ not a decision I took.

⛔ **Do NOT "fix" a high rate by capping RateScale below the formula** — the skate is then
`Speed − 337.5 × RateScale` uu/s (e.g. 428 uu/s of skate at Speed 900 capped to 1.4). The formula is the
only correct setting.

---

## 4. HOW IT WAS BUILT ON THE PINNED LINE (⛔ NOT A VERTICAL CLIMB TRANSLATED DIAGONALLY)

Derived from `TOWER-§8.3`'s own endpoints, ⛔ not from a restated literal:
`LadderFoot (−450, 0, 0)` → `LadderTop (−150, 0, 1200)` ⇒ Δ = (300, 0, 1200), **length 1236.932 uu**,
**lean 75.9638°**, **tilt from vertical 14.0362°** — all three recomputed in-script and logged.

- **The body leans 14.0362° forward**, so the spine is **parallel to the climb line**. That lean is a
  constant `root` rotation: the capsule cannot pitch (CharacterMovement keeps it upright), so the lean
  **must** come from the clip, and it is the single largest reason this reads as a ladder.
- **A rung plane is constructed from the line** (`d` = climb direction, `n` = line→rung normal,
  `e` = character-left). Every grip point is `u·d + lateral·e + perp·n`.
- **Hands and feet are placed by analytic 2-bone IK onto that plane**, per frame, with the shoulder/hip
  read from the *already-posed* parent chain. **Max IK residual: 0.0000 uu** — every contact is exact,
  not approximated by eye-tuned FK angles.
- **The cycle:** each limb **grips for half the loop and swings for half**, hands and feet contralateral.
  Grip is **linear** in `u` (that is what makes contact exact); the swing is a **cubic Hermite whose end
  derivatives match the grip's**, so grip→swing is C¹. The swing's natural −4.4 % dip / +4.4 % overshoot
  reads as release-anticipation and reach-overshoot.
- **Frame 0 sits mid-grip / mid-swing — never on a plant or release** — which is what puts the loop seam
  away from the cycle's only genuine corners.

---

## 5. ⭐ HOW I VERIFIED THE LOOP IS SEAMLESS (three gates, all off the exported file)

| Gate | Result | What it proves |
|---|---|---|
| **Seam pose identity** — every fcurve's key at frame N vs frame 0 | **0.00000 uu** (exact) | C⁰: the wrap is pose-identical, ⛔ no pop |
| **Seam corner ratio** — wrapped 2nd difference of bone world positions at the seam ÷ the max elsewhere in the cycle | **0.420** (worst limb) | The seam is **not** the sharpest frame. The genuine corners are at the plant/release quarter-phases (frames 5 and 13), which is where they belong. `hand_r` and `foot_l` — the limbs mid-grip at the seam — score **exactly 0.000** |
| ⭐ **Contact across the wrap** — walk the cycle **twice** (pose is periodic, advance is not), add the traversal advance back, measure how far a gripping limb moves in world | **≤ 0.0001 uu**, all four limbs | The decisive one. `hand_r` and `foot_l` grip **through** the loop point; if the loop were not seamless their contact would jump by up to a full 45 uu stroke. It does not |

A naive `v(N) − v(0)` velocity comparison was tried first and **rejected as invalid** — on a periodic
sampled curve it measures *curvature*, not discontinuity, and it flagged 2.78 uu/frame on a limb that is
provably continuous. The corner-ratio test replaced it.

**Also measured:** elbows **43°–112°**, knees **41°–141°** — never locked straight, never collapsed —
and **bilaterally identical to the digit**, which independently proves the half-cycle mirror is exact.

**Eyeball gate (nothing ships unseen):** 5 preview strips in
`Tools/ArtPipeline/Cache/SiegeBipedClimb/` — `climb_hero_strip.png` (900 px, 3 angles),
`climb_side_strip.png`, `climb_back34_strip.png`, `climb_traversal_strip.png`, and
⭐ `climb_contact_strip.png`, which pins a red marker at the grip point and advances the rig along the
pinned line exactly as TASK-738 will: **the hand stays on the marker while the body climbs past it.**
(The preview mesh is the Footman, so its shield/sword appear — preview only; the clip carries no mesh.)

---

## 6. ⛔ THE IMPORT — THE ONE OUTSTANDING STEP (for TASK-739 / TASK-742)

Import `Content/RawAssets/Characters/Anims/SiegeBiped_Climb.fbx` as an **Animation Sequence**:

| Setting | Value | Why |
|---|---|---|
| Destination | `/Game/Characters/Anims/` | naming clause |
| Asset name | **`A_SiegeBiped_Climb`** | ⛔ exactly this — TASK-739 binds it by name |
| Import type | **Animation only** (`bImportMesh = false`, `bImportAnimations = true`) | the FBX carries no mesh |
| **Skeleton** | **`/Game/Characters/SK_Footman_Skeleton`** | ⛔ **must be set, or UE creates a NEW skeleton and every unit's ABP binding silently diverges** |
| **`bImportAsSkeletal` / create new skeleton** | **OFF** | same reason |
| ⛔ **Root motion (`bEnableRootMotion`)** | ⭐ **FALSE** | the shipped fleet convention — the traversal drives the translation, the clip supplies the pose |
| **`bLoop` / looping** | **TRUE** | it is a cycle |
| Frame import range | full (1–17) | frame 17 is pose-identical to frame 1; UE's wrap lands on it exactly, ⛔ no duplicate-frame hitch |
| Material import | none | ⛔ no mesh, no materials |

⭐ **Verify by read-back, ⛔ not by the call returning success** (this lane has produced three silent
defects): after import, read back **(a)** `GetSkeleton()` resolves to `SK_Footman_Skeleton`,
**(b)** `bEnableRootMotion == false`, **(c)** `SequenceLength ≈ 0.2667 s` and frame rate `60`,
**(d)** the number of bone tracks is **21**.

⚠️ **If `SequenceLength` reads ≈ 0.533 s, the clip imported at 30 fps** — the RateScale would then be
wrong by exactly 2×. Check it.

---

## 7. DEVIATIONS AND JUDGEMENT CALLS — DECLARED, ⛔ NOT SILENTLY TAKEN

1. ⭐ **60 fps, not the rig lane's 30.** At the shipped rate this clip runs a 0.267 s loop; 30 fps would
   give it **8 source keys per cycle** and 60 fps gives 16. Nothing in the project couples to this clip's
   fps (RateScale arithmetic is fps-free, and CONVENTIONS' 24-vs-30 note is about Idle/Walk *durations*
   staying consistent between the Sorcerer and the Wizard — a pair this clip is not part of).
   ⚠️ Scene fps is pinned **before** export, so `rig_character.py`'s known "exported before
   `render_fps` was set" trap is avoided — read-back confirms **60** in the file.
2. ⚠️⚠️ **A constant `root` translation of 24.0 uu, backward along the rung-plane normal — please read
   this one, it is the only thing here another lane can trip over.**
   `TOWER-§8.3` puts the ladder's centreline and the climber's own path on **the same line**, so a
   climber posed on-axis would have the rungs passing **through its torso**. The clip therefore holds
   the body **24 uu behind the rung plane** and reaches the limbs forward onto it.
   - It is **constant** — ⛔ no drift, ⛔ no bob, ⛔ no rate dependence.
   - 24 uu is **inside the 34 uu capsule radius**, so the mesh does not leave its own capsule.
   - ⚠️ **Consequence for TASK-739:** blending Walk → Climb moves the mesh 24 uu backward. Over a normal
     0.15–0.2 s blend this reads as the unit *leaning into the ladder* — desirable — but it is a real
     mesh offset and you should know it is there rather than discover it.
   - ⚠️ **Consequence for TASK-737:** if the re-authored mesh puts the ladder's **near face** (rather
     than its centreline) on the pinned line, this 24 uu may want re-tuning. It is one constant
     (`BODY_STANDOFF_M`) in `author_climb_anim.py` and a 30-second re-export. ⛔ I did not guess at
     TASK-737's mesh — I built to the pinned line, as the law instructs.
3. **Stroke advance 45 uu / loop advance 90 uu was MEASURED from the rig, ⛔ not chosen for roundness.**
   The script computes the reachable band from the actual bone lengths (arm 64.8 uu, leg 78.1 uu) and
   the standoff; the leg is the binding constraint at 50.5 uu of usable band against 49.0 uu required —
   **+1.5 uu of headroom**. A larger stride is not available on this rig.
4. **The low-grip bound is a minimum shoulder-to-hand DISTANCE, not a minimum height.** Distance is
   minimised at shoulder level, so a band centred across the shoulder collapses the elbow wherever it is
   placed. Fixing this (and widening the standoff to 24 uu) took the worst elbow from ~28° to 43°.
5. ⛔ **No per-unit clips.** `A_Archer_Climb` / `A_Longbowman_Climb` / `A_Wizard_Climb` were refused as
   redundant, per `TOWER-§0`.

**Nothing in `TOWER-§8.3` was adjusted.** The lean, the line length and the tilt are all *derived* in
code from the two pinned socket coordinates, so if that law is ever amended the clip re-derives rather
than disagreeing.

---

## 8. RE-RUN / RE-EXPORT

```
"C:/Program Files/Blender Foundation/Blender 5.1/blender.exe" --background --factory-startup \
  --python-exit-code 1 --python Tools/ArtPipeline/author_climb_anim.py -- --advance 90 --frames 16
```
`--probe` prints the reach budget only · `--verify-fbx` re-imports the **exported** file cold and re-runs
the seam + contact gates · `--no-previews` skips rendering · `--save-blend` writes a debug `.blend`.
Deterministic: same inputs ⇒ byte-identical numbers.

---

## 9. FILES

**Added:** `Content/RawAssets/Characters/Anims/SiegeBiped_Climb.fbx` ·
`Tools/ArtPipeline/author_climb_anim.py`
**Modified:** none. **Deleted:** none. **`/Game/` touched:** none (editor down).
`Tools/ArtPipeline/Cache/` is gitignored.

---
---

# TASK-733b — THE **−22 uu** RIGID SHIFT ONTO THE SHIPPED RUNG PLANE (art-director)

**APPENDED 2026-09-01. ⛔ The record above is TASK-733's and is UNCHANGED — this section supersedes only
the grip/root plane numbers in §7.2.** Everything else above still stands as written.

**Law:** `TOWER-§8.3` (the rung-plane row) · `TOWER-§8.1` · `TOWER-§0`.
**Executes the manager's ruling on TASK-739's F5:** *the clip should match the mesh; the law should not
match the gap.*

---

## 0. ⛔ READ FIRST — WHAT THIS TASK DID AND DID NOT DO

| | |
|---|---|
| FBX re-exported | ✅ `Content/RawAssets/Characters/Anims/SiegeBiped_Climb.fbx` (167,660 bytes) |
| Authoring script changed | ✅ `Tools/ArtPipeline/author_climb_anim.py` — **one new constant** |
| `/Game/` touched | ⛔ **NONE.** ⛔ The import is still not mine (spec (5)) |
| `SK_Footman_Skeleton` | ⛔ **NOT opened, NOT modified** |
| `ABP_Footman` | ⛔ **NOT touched** — ⚠️ it is DIRTY IN MEMORY from an editor autosave; ⛔ decline saving it |
| `SM_WatchTower` / `WatchTower.fbx` | ⛔ **READ-ONLY — parsed, never written.** TASK-742 owns its reimport |
| C++ / compile / Git / `.umap` / editor lifecycle | ⛔ **NONE** |

---

## 1. ⭐ I VERIFIED THE −22 MEASUREMENT MYSELF — ⛔ NOT RELAYED FROM TASK-739

I wrote **my own binary FBX reader** (FBX 7400 node walker, independent of TASK-739's) and measured the
landed `Content/RawAssets/WatchTower.fbx` cold.

⚠️ **One trap worth recording:** the file's `GlobalSettings` say **`UpAxis = 1` (Y-up)** and every `Model`
carries Blender's **−90° X rotation** with **`Lcl Scaling` 100**. A parse that reads x/y/z naively measures
the *lateral* axis as height. The correct remap is **UE (X, Y, Z) = (fbx X, −fbx Z, fbx Y)**.
✅ Validated against two figures the parse could not fudge: render **x → −436.418 uu** and lateral
**±66 / ±86 uu**, both exactly TASK-737's declared numbers.

**Frame:** `LadderFoot (−450,0,0)` → `LadderTop (−150,0,1200)`; length **1236.9317 uu**, lean **75.96376°**
— re-derived, ✅ matching `TOWER-§8.3`. **m** = in-plane normal pointing **away from the tower**
= `(−0.970143, 0, 0.242536)`.

| Measured (TASK-733b, own reader) | Value | TASK-739 said |
|---|---|---|
| Ladder slab **near face** (climber side) | **−12.000 uu** | −12 ✅ |
| Ladder slab **far face** | **−32.000 uu** | −32 ✅ |
| Slab thickness | **20.000 uu** | 20 ✅ |
| ⭐ **Rung MID-PLANE** | ⭐ **−22.000 uu** | −22.0 ✅ |
| Lateral | **±66 / ±86 uu** | ±66/±86 ✅ |
| `UCX_` hull count / min x | **8 hulls, min x = −300.0 uu** | 8, −300 ✅ |

> ### ⭐ **CONFIRMED INDEPENDENTLY. The −22.0 uu figure is correct.**

### 1.1 ⭐ THE EXTRA CHECK THE −22 FIX ACTUALLY DEPENDS ON — AND IT PASSES

A **rigid** shift is only the right instrument if the rung plane is **constant along the ladder**. If the
slab fanned or stepped, one translation would be right at one height and wrong everywhere else.
⚠️ **Neither TASK-739 nor the dispatch checked this.** I did.

Selecting by **proximity to the climb line** (⛔ not by `x < −300`, which silently clips the ladder's
upper half at u ≈ 618):

```
u-bucket    n     min      max      mid          <- m-offset, uu
      0    16   -32.0    -12.0    -22.0
    100    24   -31.0    -13.0    -22.0
    ...    ..     ...      ...      ...
   1100    24   -31.0    -13.0    -22.0
```

**All 12 buckets over u = 8 → 1199 uu: mid-plane −22.0, thickness 20.0.** ⇒ ⭐ **The plane is constant to
the digit over the ladder's whole run. A rigid translation is provably the correct fix.**

✅ Also measured: **nothing sits behind the ladder** — no vertex at all in m ∈ [−60, −32) within the
±95 uu lateral band. So limbs that project past the far face meet **open air, ⛔ not tower geometry.**

---

## 2. THE CHANGE — ONE NEW CONSTANT

⚠️ **Deviation from spec (1), declared:** the spec named `BODY_STANDOFF_M` as the knob. **I did not change
it, deliberately.** `BODY_STANDOFF_M` **is the authored reach** — the distance every IK solve runs against.
Editing it would change the reach and **re-solve every joint**, which is the one thing spec (2) requires
not happen. Instead I added a **separate** constant and left the reach untouched:

```python
RUNG_PLANE_OFFSET_M = 0.22   # rung plane sits this far along +n from the pinned line
```

Applied at exactly **three** placement sites — `rung_point()` (all grip targets), `build_pose()` and
`build_pose_static()` (the root) — so grips and body translate **together**:

| | before | after |
|---|---|---|
| Grip plane | m = 0 | ⭐ **m = −22** (on the shipped rung mid-plane) |
| `root` / body | m = +24 | ⭐ **m = +2** |
| **Body → rung distance** | 24 uu | ⭐ **24 uu — UNCHANGED** |

The reach budget was rewritten to measure against the plane's new coordinate rather than `0`, so it is
**invariant by construction** rather than by luck. Everything else in the script uses `n` only as a
**direction** (IK poles, grip/foot aims), which a translation cannot affect.

**Previews:** the ladder proxy now stands on the shipped plane with the measured **20 uu thickness** and
**±66/±86 uu** span. ⛔ A preview drawn at the pinned line would have shown hands meeting a ladder that
is not there — the eyeball gate would have been lying.

---

## 3. ⭐⭐ PROOF IT IS A RIGID TRANSLATION — ⛔ NOT AN ASSERTION

I kept the pre-shift FBX and **diffed the two files curve-by-curve** with my own reader (198 animation
curves, labelled bone|channel|axis via the `OP` connection graph).

```
MAX delta across all 195 NON-ROOT curves  (float32 export quantisation only):
  rotation    : 9.155e-05 deg   <- hand_r|Lcl Rotation|d|Z
  translation : 4.470e-05 uu    <- lowerarm_l|Lcl Translation|d|Y
  scaling     : 7.153e-07

ROOT Lcl Translation delta  --  THE ONLY MATERIAL CHANGE:
  |delta|           = 21.999999 uu
  component along n = 21.999999 uu
  off-axis residual =  0.000000000 uu
  root track CONST  : pre=True  post=True
```

⇒ ⭐ **Every joint rotation is identical to within a few float32 ULPs. The sole material change is the
root's constant translation: exactly 22.000 uu along `n`, with a ZERO off-axis component.**
⭐ **Nothing re-solved.** The root track is still **CONSTANT** — ⛔ no cyclic translation was introduced.

---

## 4. THE ORIGINAL VERIFICATION, RE-RUN (spec (4)) — ⛔ NO REGRESSION

| Gate | TASK-733 | **TASK-733b** | |
|---|---|---|---|
| **Seam pose identity** | 0.00000 uu | **0.0 uu (exact)** | ✅ |
| ⭐ **Contact drift through the wrap**, all four limbs | ≤ 0.0001 uu | **≤ 0.0000477 uu** — `hand_l` 0.000048 · `hand_r` 0.000012 · `foot_l` 0.000003 · `foot_r` 0.000012 | ✅ |
| **IK residual** | 0.0000 uu | **0.0000 uu** | ✅ |
| **Elbows / knees** | 43–112° / 41–141° | **43.4–112.1° / 41.2–140.8°**, bilaterally identical | ✅ |
| Seam corner ratio | 0.420 | **0.4199** | ✅ |
| Bones / fps / keys / meshes | 21 / 60 / 17 / none | **21 / 60 / 17 / none** | ✅ |

**Cold re-import read-back** (`--verify-fbx`, re-imports the *exported* file): `bones=21 keys=[1,17]
fcurves=219 fps=60 meshes=[]`, `seam=0.00000 uu`, `contact skate max=0.0001 uu`. ✅
**Reach budget, bit-identical:** arm **64.81 uu** · leg **78.14 uu** · need **48.99 uu** · hand headroom
**+6.75 uu** · ⭐ leg headroom **+1.52 uu**. **Warnings: none.**

⛔ **The rejected velocity-based seam test was NOT reintroduced.** It measures curvature, not
discontinuity, on a periodic sampled curve. The corner-ratio test remains its replacement.

### 4.1 THE THREE RATE FIGURES — ⛔ CONFIRMED UNCHANGED, ⛔ NOT ASSUMED

| | TASK-733 | **TASK-733b** |
|---|---|---|
| Advance per loop **along the line** | 90.00 uu | ⭐ **90.00 uu** |
| **Native speed along the line** | 337.50 uu/s | ⭐ **337.50 uu/s** |
| Declared readable band | 0.4 – 1.4 | ⭐ **0.4 – 1.4** |

Also unchanged: 16 frames @ 60 fps = **0.266667 s** · vertical **87.3128 uu / 327.4231 uu/s** · stroke
**45.00 uu** · lean **75.96376°** · **`RateScale = LadderClimbSpeedUU / 337.5` = 1.037037 at 350.**
⭐ **The shift is perpendicular to travel, and the numbers confirm it rather than being asserted to.**

---

## 5. ⭐ THE IMPORT PATH — WHY THE ERROR WAS MISLEADING (spec: so the next reader meets the REASON)

> ## ⭐⭐ **THIS FBX IS ANIMATION-ONLY BY DESIGN. IT MUST BE IMPORTED VIA THE *ANIMATION* PATH.**

Jonathan's Message Log showed:

```
Failed to find any bone hierarchy. Try disabling the "Import As Skeletal" option to import as a rigid mesh
Import failed.
```

⛔ **That message is false about this file.** TASK-739 parsed it cold at the byte level: **22 `Model` nodes
— one `Null` (`Footman_Rig`) + 21 `LimbNode`** — a **22/22 exact, in-order match** to
`SK_Footman_Skeleton`; `CustomFrameRate 60.0`; `LocalStop 0.266667 s`; 17 keys/curve. A control scan of the
known-good `Footman_Walk.fbx` returned **identical structural markers**. ✅ Still true of this re-export:
21 bones + the rig null, 60 fps, 17 keys, **zero meshes**.

**Why the engine says it anyway:** UE's **skeletal-mesh** import path discovers bones by walking
`Skin` → `Cluster` deformer links **off a `Mesh`**. This file has no mesh, so it finds no clusters and
reports *"no bone hierarchy"* — **it never looks at the `LimbNode` chain at all.** The message is the
skeletal-mesh path describing its own failure, ⛔ not a defect in the file.

| Setting | Value |
|---|---|
| ⭐ **`MeshTypeToImport`** | ⭐ **`FBXIT_Animation`** — ⛔ **THE ONE FIELD THAT DECIDES SUCCESS** |
| **`bImportMesh`** | **false** |
| **`bImportAsSkeletal`** | **false** |
| ⛔ **`Skeleton`** | ⛔ **`/Game/Characters/SK_Footman_Skeleton`** — mandatory, and what stops UE creating a NEW skeleton |
| `bImportAnimations` | true |
| **`bEnableRootMotion`** | ⭐ **false** — the traversal drives translation, the clip supplies pose |
| Loop | true · **Asset name `A_SiegeBiped_Climb`** → `/Game/Characters/Anims/` |

⛔ **Do NOT take the engine's suggested fix.** *"Import as a rigid mesh"* yields a **StaticMesh** — it would
"succeed" and deliver nothing to drive `ABP_Footman`.
⛔ **Do NOT "fix" it by adding a mesh to the export.** That would create a SkeletalMesh **bound to
`SK_Footman_Skeleton`** — writing to the one shared asset the fleet depends on.

**Read-back (⛔ never trust the call returning success):** (a) `GetSkeleton()` → `SK_Footman_Skeleton` ·
(b) `bEnableRootMotion == false` · (c) `SequenceLength ≈ 0.2667 s` and **frame rate 60** — ⚠️ **0.533 s
means a 30 fps import and a silent 2× `RateScale` error** · (d) bone tracks = **22**, ⛔ **not 21**
(TASK-739's F2 correction).

⚠️ **MCP has no animation-import entry point** (TASK-739 §2), so this needs a Python/commandlet lane or
Jonathan's hand-drag. ⛔ Not this task's.

---

## 6. ⚠️⚠️ A RESIDUAL I MEASURED THAT NOBODY ANTICIPATED — DECLARED, ⛔ NOT SILENTLY DECIDED

**The clip solves against a ZERO-THICKNESS rung plane. The shipped rung is a 20 uu SLAB.** Putting the
grips on its **mid-plane** (−22) necessarily puts the body **10 uu closer to the ladder** than putting them
on its **near face** (−12) would. ⚠️ **Nobody's arithmetic covered this — TASK-739 and the dispatch both
treat the rung plane as a plane.** So I measured the skinned mesh against the real slab, per frame.

✅ **The grip contract is exact** — bone heads, straight off the pose:

```
hand_l / hand_r   m = -22.00 uu  CONSTANT through the whole grip window   <- dead on the rung mid-plane
foot_l / foot_r   m = -14.00 uu  CONSTANT   <- mid-plane + the 8 uu ankle standoff, exactly as designed
```

⚠️ **The residual — two vertex groups now overlap the rung's NEAR FACE (−12 uu):**

| Group | deepest m | vs near face |
|---|---|---|
| `neck_01` | **−14.29 uu** | ⚠️ **2.29 uu inside** |
| `pelvis` | **−13.92 uu** | ⚠️ **1.92 uu inside** |
| `spine_03` / `spine_01` / `clavicle_r` / `spine_02` / `head` | −10.95 … −4.66 uu | ✅ **all clear/outboard** |

⇒ **The torso proper is clear. Only the neck and pelvis dip ~2 uu into the near face** — the clip clears
its own authored plane by 7.7 uu, but the rung carries 10 uu of material outboard of that plane.

> ### ⚖️ **THE TRADE, STATED PLAINLY: this converts a 12 uu VISIBLE grip miss (hands closing on air, the
> exact "reads as broken" the clip exists to prevent) into a ~2 uu overlap at the neck and pelvis.**
> **That is a clearly good trade, and ~2 uu on a ~180 uu character is at or below the visual threshold.**

🙋 **FOR-MANAGER — ⛔ NOT MY RULING.** If the ~2 uu is judged visible, the fix is **one constant, no
re-solve**: `RUNG_PLANE_OFFSET_M = 0.22 → 0.12` grips the rung's **near face** instead of its centre
(defensible: a hand wraps the front of a bar). That buys **7.7 uu of neck clearance** at the cost of
standing the body 10 uu further out. ⛔ **I delivered the −22 the ruling specified and did not re-decide it.**

⚠️ **Also observed, ⛔ PRE-EXISTING and ⛔ unchanged by this shift:** the knee pole aims **+n**, so at full
flexion the shin/knee project past the rung plane (deepest `calf_l` **−54.6 uu**). In body-relative terms
this is **identical** to TASK-733's approved clip — the rigid shift moved it, it did not create it. ✅ It
meets **open air, not tower geometry** (§1.1). Flagged for the QA eye, ⛔ not re-authored by me.

---

## 7. EYEBALL GATE — ⛔ NOTHING SHIPS UNSEEN

5 strips re-rendered in `Tools/ArtPipeline/Cache/SiegeBipedClimb/` (gitignored). Reviewed:
⭐ `climb_contact_strip.png` — **the red marker stays pinned on a rung while the body climbs past it** ·
`climb_side_strip.png` — body rides **outboard** of the slab, limbs reach in · `climb_hero_strip.png` —
reads as a ladder climb from all three angles. *(The preview mesh is the Footman, so its shield/sword
appear — preview only; the clip carries no mesh.)*

---

## 8. RE-RUN

```
"C:/Program Files/Blender Foundation/Blender 5.1/blender.exe" --background --factory-startup \
  --python-exit-code 1 --python Tools/ArtPipeline/author_climb_anim.py -- --advance 90 --frames 16
```
⚠️ **`--verify-fbx` RETURNS EARLY and only re-checks the file already on disk — it does ⛔ NOT export.**
Run the export first, then `--verify-fbx` separately. *(This cost me one run; recorded so it costs nobody else.)*

## 9. FILES

**Modified:** `Content/RawAssets/Characters/Anims/SiegeBiped_Climb.fbx` (167,660 bytes) ·
`Tools/ArtPipeline/author_climb_anim.py` · this handoff.
**Added / deleted:** none. **`/Game/` touched:** ⛔ **none.** **Code / compile / Git:** ⛔ **none.**
