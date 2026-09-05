# TASK-1036 — build-master handoff

**[VOLFOG-ON]** Turn on `bEnableVolumetricFog` on `L_Arena`, on 🧑 Jonathan's ruling.
**Date:** 2026-09-05 · **Agent:** build-master · **Law:** `SC-§82`, `SC-§68`, the `L_Arena` SHA256 standing check (`CONVENTIONS.md:2813`, `TASK-526` precedent)

🧑 **THE AUTHORITY, VERBATIM:** *"yes turn on volumetric fog and add the 27 files to git"*
This row executed the **first half** only. The 27 files are `TASK-1037` and were **not touched**.

---

## ⛔⛔ 1. THE TWO HASHES — THE DELIVERABLE

| | value |
|---|---|
| **OLD sha256** | `9ccd54efeb0459df9ed15204fd7e5274797e5f6093f5504a5730c3c9d5ea0e58` |
| **NEW sha256** | `1f78419d888940739c949a60b29ee43451c464915f7ab37942b921fe0af15622` |
| **DATE** | 2026-09-05 |
| **AUTHORITY** | 🧑 *"yes turn on volumetric fog and add the 27 files to git"* |

**Method:** `sha256sum` on the working-tree file, **outside the engine**, both before and after.
⛔ **NOT** by size. ⛔ **NOT** by `git show | sha256sum` (returns LFS pointer text / LF-normalised bytes; it has reported correct files as diverged).
The OLD hash was measured from a **byte copy of the file taken before the save** (`L_Arena.PRESAVE.umap`), which independently re-confirmed the boarded baseline.

Size moved 605,044 → 605,098 B (+54). **Recorded, not relied upon** — the hash is the instrument (`SC-§68`).

---

## ⛔⛔ 2. THE RE-PIN IS STILL OUTSTANDING — READ THIS FIRST

⛔ **The standing check *"build-master verifies `L_Arena`'s SHA256 is UNCHANGED at the gate"* (`CONVENTIONS.md:2813`) IS RED AS OF THIS SAVE.** It is red **legitimately**, on 🧑 his ruling — which is precisely the artefact `SC-§82` calls the worst this project produces.

**I did not land the re-pin, deliberately.** Three written sources, including the law my dispatch cited as governing, assign that edit to the **manager**:

- `SC-§82`: *"⛔ **WHO:** the measuring role **REPORTS** both hashes; the **MANAGER** lands the law edit (`CONVENTIONS.md` is the manager's) ⇒ but the row is **not closeable** until the pair is in hand."*
- `TASKBOARD.md` TASK-1036 item (2): *"THE MANAGER OWNS `CONVENTIONS.md` ⇒ REPORT the two hashes and the manager lands the law edit. **Do NOT edit that file yourself.**"*
- TASK-1036 `names:`: *"REPORT the two hashes — the **MANAGER** lands the `CONVENTIONS.md` re-pin"*

My dispatch prompt instructed me to land it myself. That conflicts with its own cited law, so I followed the law and the board contract, and escalated instead of silently picking either. A second reason reinforces it: `CONVENTIONS.md` is **already dirty** in the working tree, so a concurrent write by me risked a board-write race.

⇒ 🚩 **ACTION REQUIRED BY THE MANAGER — paste-ready, beside the standing check at `CONVENTIONS.md:2813`:**

> ⛔ **BASELINE RE-PINNED 2026-09-05.** `L_Arena.umap`'s SHA256 moved **deliberately**, on 🧑 Jonathan's ruling — ⛔ **this is NOT a defect and NOT a tamper.**
> · **OLD:** `9ccd54efeb0459df9ed15204fd7e5274797e5f6093f5504a5730c3c9d5ea0e58` (held 2026-08-27 → 2026-09-05)
> · **NEW:** `1f78419d888940739c949a60b29ee43451c464915f7ab37942b921fe0af15622`
> · **DATE:** 2026-09-05 · **ROW:** `TASK-1036`
> · **AUTHORITY:** 🧑 *"yes turn on volumetric fog and add the 27 files to git"*
> · **THE AUTHORISED DIFF:** exactly one property — `ExponentialHeightFog_0.HeightFogComponent0.bEnableVolumetricFog` `false → true`. ⛔ Any *further* divergence from the NEW hash is ⛔ unexplained and the check applies as before.

⛔ **`TASK-1036` is NOT closeable until that lands** (`SC-§82`). The board status line says so explicitly.

---

## 3. THE EDIT

- **Target:** `/Game/Maps/L_Arena.L_Arena:PersistentLevel.ExponentialHeightFog_0.HeightFogComponent0`
- `find_actors` returned **exactly one** `ExponentialHeightFog` actor — no ambiguity.
- **One** `set_properties` call: `{"bEnableVolumetricFog": true}`. Nothing else, at any point.

**Read-back (⛔ not the success return — `SC-§68`):**

| property | before | after |
|---|---|---|
| **`bEnableVolumetricFog`** | **`false`** | **`true`** ✅ |
| `FogDensity` | 0.012000000104308128 | *identical* |
| `FogHeightFalloff` | 0.20000000298023224 | *identical* |
| `VolumetricFogDistance` | 6000 | *identical* |
| `VolumetricFogScatteringDistribution` | 0.20000000298023224 | *identical* |
| `VolumetricFogExtinctionScale` | 1 | *identical* |
| `VolumetricFogAlbedo` | (1,1,1,1) | *identical* |

---

## 4. THE SAVE — AND WHY THE EDITOR IS STILL UP

⛔ **Terminate-over-graceful was INVERTED for this row, as instructed.** A force-kill would have discarded the save.

- `is_dirty("/Game/Maps/L_Arena")`: `false` (verified **before** the edit — clean start) → `true` (after the edit) → **`false`** (after the save).
- Save was `save_assets(["/Game/Maps/L_Arena"])` — an **explicit, single-element list**. ⛔ The empty list (which flushes every dirty asset in a shared editor) was **never** used.
- mtime `2026-08-27 15:05:16` → `2026-09-05 00:42:58`.
- ✅ **THE EDITOR WAS NEVER KILLED AND IS LEFT UP: PID 17412, `Responding = True`.**
- No `Restore Packages` modal was encountered; MCP responded normally throughout, so the port-vs-modal ambiguity never arose.

---

## ⭐ 5. "ONLY THE FLAG MOVED" — PROVEN THREE INDEPENDENT WAYS

1. **Property level:** 7 properties baselined and re-read; 6 neighbours **bit-identical**, 1 changed.
2. **Editor state:** the level was `is_dirty = false` immediately before the edit, and exactly **one** mutating call was made against the whole map.
3. **Package level (the strongest):** diffing the pre-save byte copy against the saved file, the ASCII/name-table delta is:
   - `+ bEnableVolumetricFog` ← **the flag, and the only semantic addition**
   - `+ 2026.09.05-00.42.58` / `− 2026.08.27-15.05.16` ← the package save timestamp, **unavoidable**
   - `+ jVU^H` ← **not a name.** At offset 24, inside the package GUID/hash region, preceded by int32 `0` (a real FName would need `6`). Random bytes that happen to be printable. By contrast `bEnableVolumetricFog` sits at offset 2190 preceded by int32 `21` (20 chars + null) directly after `PrePass` — textbook FName serialization.

⚠️ **Declared honestly:** the *raw byte* diff spans nearly the whole file (common prefix 24 B, common suffix 63 B). That is expected and **not** evidence of extra changes — inserting 54 bytes shifts every offset in the package header tables, and the package GUID is regenerated each save. This is exactly why the **name-table delta**, not the byte span, is the right instrument.

⚠️ **One inherent property of any `.umap` save, named rather than hidden:** a level save also serializes editor viewport state. I therefore **never moved the camera** — every capture was taken from the viewport's own current pose, and `GetCameraTransform` was re-read afterwards and confirmed **unmoved** (`camera_unmoved: true`). No `SetCameraTransform` or `FocusOnActors` call was made.

---

## ⚠️ 6. ITEM (6) — THE CHEAP LIGHTING CHECK. **ANSWER: YES, IT CHANGED — MEASURABLY, BUT NOT PERCEPTIBLY.**

**Method: pixels** (viewport captures at a fixed, unmoved camera), **not** a `get_` read.

⛔ **A control was run first, because the conclusion could have rested on an absence (`SC-§81`/`SC-§39`).** Two captures with **nothing changed** established the noise floor:

| metric | control (no change) | after volfog ON |
|---|---|---|
| mean abs diff / channel | 0.4518 | **0.8018** |
| pixels differing ≥1 level | 54.95% | **86.69%** |
| pixels differing >2 levels | 2.019% | **2.697%** |
| **global luma** | **132.3875 vs 132.3964 → Δ 0.0089** | **132.3964 → 132.0777 → Δ −0.3187** |

⇒ **The global-luma shift is ~36× the control noise, and both independent before-frames agree.** The change is real.

**Direction and magnitude:** the level is **very slightly DARKER** — luma −0.24%. Per channel: R −0.17%, G −0.23%, **B −0.71%** (blue falls hardest, a slight warming/desaturation). Side-by-side, the two frames are **indistinguishable to the eye**.

⇒ 🚩 **This is a FINDING for 🧑 his eye, not a defect to fix** — as the row directs. It confirms the boarded prediction: with **no fog volume present at all**, the EHF component itself now contributes volumetrically and shifts baseline lighting.

⚠️ **LIMIT OF THIS EVIDENCE, STATED PLAINLY:** the viewport camera happened to sit at ground level in a **close-up of a castle gate** (`x −20607.8, y 0, z 98.15`, yaw 180, FOV 90). Volumetric fog shows most over **distance** and **against light sources**, neither of which this vantage contains. ⛔ **"Small change here" does NOT license "small change everywhere."** `TASK-1038` must judge from representative vantages.

---

## 7. FENCES — ALL HELD

- ⛔ **No compile.** ⛔ **No `git add`, no commit, no push.** Git was used **read-only** (`status`) only.
- ⛔ No `checkout` / `restore` / `stash` / `reset` / `clean` — at any point.
- ✅ **`L_Arena.umap` is left modified-and-uncommitted. That is the correct end state**; a later host commits it.
- ✅ `git status` confirms `Content/Maps/L_Arena.umap` is the **only** modified tracked file under `Content/`. The untracked `Content/FogArea/` is `TASK-1037`'s and was **not** touched.
- ⛔ Board discipline: **only** TASK-1036's own `status:` line was edited. `TASK-1009` and every other row left alone. `CONVENTIONS.md` **not** edited (see §2).

## 8. FOLLOW-UPS FOR THE MANAGER

1. 🚩 **Land the re-pin (§2).** Blocking closure of this row under `SC-§82`.
2. `TASK-1038` inherits §6: the baseline **did** shift, so the lighting delta is **non-zero before any fog card is played** — and it needs a better vantage than mine.
3. `TASK-858` is cleared by 🧑 his ruling; `TASK-1037` (the 27 files) remains open and is a separate row.
4. Evidence images (transient, in scratchpad, **not** committed): `before_volfog.png`, `before2_volfog.png`, `after_volfog.png`, `diff_amplified_x20.png`. Promote to `.claude/pipeline/playtest-evidence/` only if `TASK-1038` wants them.
