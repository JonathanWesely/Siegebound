# TASK-864 — `concept_generate.py` reports SUCCESS on a black frame and overwrites the good roll — handoff

Date: 2026-09-02 · Status set: **ready-for-qa** (gate **TASK-865**)
Law: **`SC-§39.1`** (tool authorship) · `SC-§39` · `SC-§40` cl. 3 · `SHIP-§9`

---

## 0. Premise verified FIRST, and the measurement is reported even though it confirmed

`SC-§40` cl. 3. I did **not** take this on `TASK-833`'s word. I drove the **unmodified**
`generate_one()` with a stand-in client that returns a fully black 1024×1024 frame, into a
scratch Inbox pre-seeded with a **copy** of the real 837,664-byte `Witch.png`:

```
[concept] Witch: SUCCESS -> ...\Inbox\Witch.png (3,129 bytes) in 0.0s
[repro] generate_one returned : 0
[repro] good roll preserved   : False      (837,664 -> 3,129 bytes; sha c27b94df -> d6911226)
```

**PREMISE CONFIRMED — deterministically, with zero HF quota spent.** Exit **0**, the word
`SUCCESS`, and the 837 KB good roll replaced by a 3 KB black frame.

⭐ **The sharpest detail: the tool printed the very number that would have exposed it.**
`(3,129 bytes)` was on screen, next to `SUCCESS`. The evidence was in the output and nothing
was looking at it — which is `SC-§39.1` in one line.

**Root cause, from the control flow rather than the symptom** (old lines 393-399): between
`client.text_to_image(...)` and `return 0` there were exactly two statements —
`atomic_write_png(image, dest)` and `dest.stat().st_size`. `atomic_write_png` ended in
`os.replace(tmp, dest)`. So the destruction of the prior artefact was **unconditional and
upstream of every check**, because there were no checks. Not a threshold that was too loose:
**there was no guard at all.**

⛔ **I did NOT reproduce it against the live HF router**, and that is deliberate:
(a) doing so on the *unfixed* tool would have destroyed a real roll for a third time —
the exact harm this task exists to stop; (b) the defect under repair is **ours**, not
HF's — "seed 71031 returns black" is a fact about FLUX, while "the tool accepts what
comes back" is the bug, and a scripted client isolates it exactly and repeatably.
No quota spent, and the `router.huggingface.co` Norton interception was never in the path.

---

## 1. Files touched — three, and no more

| File | Change |
|---|---|
| `Tools/ArtPipeline/concept_generate.py` | The fix: artefact validation, stage→validate→swap, exit 6, `--check` positive control |
| `Tools/ArtPipeline/test_concept_guard.py` | **NEW** — the gate (46 assertions) |
| `Tools/ArtPipeline/README.md` | **One token**: Stage-2 example `--asset` → `--card-id` |

⛔ **Fence held.** No sibling tool, no `.venv`, no game C++, no compile, no editor, no MCP,
no Git. `concept_prompts.json` / `pipeline_manifest.json` show as modified in `git status` —
those are **the art-director's TASK-833 edits**, present before I started; I never opened them.

---

## 2. The fix

### (1) The artefact is validated before success is reported

`assess_degeneracy()` runs **four independent checks** on the 8-bit sRGB luminance channel
(PIL only — no new dependency):

| check | fires when | catches |
|---|---|---|
| `no-lit-content` | 99.9th-pct luminance < **0.12** | the black frame — *nothing in the image is lit* |
| `no-structure` | luminance stddev < **0.010** | any flat fill, black **or white or mid-grey** |
| `degenerate-size` | either edge < **64 px** | a stub/glitch return |
| `fully-transparent` | alpha is 0 everywhere | a file that exists with nothing visible in it |

Plus `unreadable-artefact` from `validate_png_file()`, which **re-reads the PNG off disk and
forces a full decode** — so a write that "succeeded" while producing a zero-byte or
undecodable file is caught too (the `save_asset` failure mode named beside this one in
`SC-§39.1`).

**Thresholds are measured, not guessed.** Over the 49 real shipped concepts
(`Inbox/*.png` + `Content/RawAssets/Concepts/*.png`) vs synthesised fixtures:

| | real corpus (worst of 49) | degenerate (worst = least obviously bad) | legitimately-dark (worst) | floor |
|---|---|---|---|---|
| p99.9 luminance | 0.7412 (`Footman`) | 0.031 | 0.435 | **0.12** |
| luminance stddev | 0.0840 (`CrystalTower`) | 0.0069 | 0.035 | **0.010** |

Each floor sits near the geometric midpoint of the gap it straddles: **6.2× / 8.4× headroom
below the real corpus**, ~3.5× above the worst degenerate case. `SC-§39`'s two-directions
clause, applied to a threshold instead of a count.

⚠️ **Two obvious signals were deliberately REJECTED as gates**, and the reasoning is in the
source so nobody re-adds them:
- **PNG byte size** (the 12,444-vs-800,000 tell `TASK-833` used by eye) **does not
  generalise in either direction**: a near-black *noise* frame compresses to **610 KB**
  (100% degenerate, passes any size floor) and a legitimately dark frame compresses to
  **7 KB** (perfectly usable, fails one). Reported as context only.
- **Distinct luminance levels** is **inverted** on the hardest case: the degenerate noise
  fixture occupies **13** levels, a legitimate very-dark frame occupies **3**.

### (2) `--force` cannot destroy a good roll — the order is the fix

```
PHASE 1  stage_png(image, dest)      -> writes .<CardID>.png.tmp-<pid> BESIDE dest; dest untouched
PHASE 2  validate_png_file(staged)   -> re-reads from disk, assesses; dest STILL the old roll
PHASE 3  commit_staged(staged, dest) -> os.replace, reached ONLY when failed_checks is empty
```

`os.replace` onto `dest` exists at **exactly one call site** (`commit_staged`, line 588),
reached from **exactly one place** (line 908) — inside `if not failed_checks:`. There is no
path that writes `dest` without passing validation.

A rejected frame is **quarantined**, not deleted: `Inbox/_rejected/<CardID>.<utc>.png`
(collision-suffixed, so a retry loop cannot overwrite its own evidence). `Inbox/` is
gitignored (`.gitignore:25`), so quarantined frames never enter git — verified with
`git check-ignore`.

### (3) Retry policy, from the evidence rather than by default

A **pinned** seed is **not** retried — identical input reproduces identically, which is
precisely what `TASK-833` measured (71031 went black **twice**); retrying would burn quota
for a guaranteed repeat. An **unpinned** seed **is** retried, because each attempt draws a
fresh one. An `unreadable-artefact` is retried **even when pinned**, because a disk/write
failure says nothing about the seed.

### (4) Exit code 6 — distinct and documented

`0/1/2/3/4/5/64` are unchanged and still 1:1 with `trellis_generate.py`. **6 = DEGENERATE
ARTEFACT** is new and unused by either sibling (both stop at 5 + 64 — checked). It is
deliberately **not** 5 ("input missing" — there was nothing to read) and **not** 4 ("API
drift" — the endpoint changed shape). Here the call **succeeded** and the model is **fine**;
the roll is not. Documented in the module docstring table, in `--help`, and asserted by the
gate. `--all` treats it as **per-card, not systemic**: the batch continues and the bad
CardIDs are **named** in the summary with a ready-to-paste re-roll command.

### (5) README

`refine_trellis_glb.py`'s real flag is `--card-id` (`refine_trellis_glb.py:191`,
`required=True`); **there is no `--asset` flag at all**, so the documented command would have
died with a usage error. Re-measured against the script rather than relayed — the script's
own docstring (line 11) already had it right; only the README was wrong.

---

## 3. ⭐ The positive control — the part TASK-865 exists to check

**The guard ships with its control wired into `--check` (step 4/5), so it runs every time
anyone smoke-tests the tool** — not only when someone remembers the test file.

Ten fixtures, and the assertion is the **exact set of checks that fired**, not merely
"rejected". That matters: with a boolean assertion, a dead check hides behind a live one and
the control stays green. **Every check therefore owns a fixture where it is the ONLY one
firing** — `dark-noise-unlit` isolates `no-lit-content`, `pure-white` isolates `no-structure`,
`tiny-glitch` isolates `degenerate-size`, `fully-transparent` isolates the alpha case.

**Both directions are controlled.** A guard proven only against black frames could be
`return True`, so three fixtures must be **ACCEPTED**, including
`very-dark-thin-lit-edge` — mean luminance **0.023**, *darker than some black frames*, saved
only by a thin lit edge. That fixture is what forces the guard to key on **"is anything
lit"** rather than on mean brightness. A guard that rejects everything fails the suite.

⭐ **And the control itself is proven able to go RED** (`test_the_control_itself_can_go_red`),
because "an unverified verifier" is the same defect one level up:
- inject a guard that **never** rejects (the original defect, re-introduced) → the control
  reports **7 disagreements**;
- inject a guard that **always** rejects → **8 disagreements**;
- with the guard broken, **`--check` exits 1**, so a broken guard **blocks generation**
  rather than logging a note;
- restore the real guard → `--check` exits **0** again, so the two failures above are not
  just a permanently-red instrument.

---

## 4. Evidence — all green, all reproducible offline

```
uv run concept_generate.py --check      -> EXIT 0   (incl. the 10-fixture guard control)
uv run test_concept_guard.py            -> EXIT 0   46/46 assertions
--help / --list                         -> EXIT 0
--all Knight (usage error)              -> EXIT 64  (unchanged)
```

The premise script re-run against the **fixed** tool now reports:

```
[repro] generate_one returned : 6
[repro] file after run        : 837,664 bytes  sha=c27b94df03213d4d   (unchanged)
[repro] good roll preserved   : True
[repro] PREMISE NOT REPRODUCED
```

**Real `Inbox/Witch.png` is untouched** — still 837,664 bytes, still its original 22:10
timestamp. Every end-to-end case redirected `INBOX_DIR` to a temp directory; no stray temp
or `_rejected/` entry exists in the real Inbox.

---

## 5. 🔍 What QA should scrutinise

1. **The order of operations, read as control flow — not as intent.** `TASK-865` cl. 2 is
   right that "validate then overwrite" and "overwrite then validate" look nearly identical.
   The mechanical check: `grep -n "os.replace" concept_generate.py` returns **two** sites —
   line 588 (`commit_staged`, guarded) and line 618 (`quarantine_staged`, which targets
   `_rejected/`, never `dest`). Then confirm `commit_staged`'s only caller (line 908) sits
   inside `if not failed_checks:`. Test **3b** proves it *behaviourally* by snapshotting the
   target's sha256 at the instant validation is invoked — on a **passing** roll, where both
   orderings end with the new file in place and only the snapshot can tell them apart.
2. **Whether the thresholds are defensible or merely convenient.** They are the one part of
   this that is a judgement call. The corpus sweep (49/49 accepted) and the margin table are
   in `test_concept_guard.py`, which prints the headroom on every run.
3. **`--check` exit 0 is now load-bearing for more than wiring.** If someone edits a
   threshold and the fixtures disagree, `--check` returns **1**. That is intended.
4. **Secrets:** unchanged. `HF_TOKEN` is env-only; every new print goes through `say`/`warn`/
   `fail`, all of which redact. The new failure report prints **image statistics and file
   paths only** — never a prompt, never a token. No secret on argv, in a log, or in a file.

---

## 6. 🪤 FINDINGS for the manager — reported, NOT fixed (fence)

**These are code reads, not behavioural repros. I did not run them.** Stated that way on
purpose: `SC-§40` forbids me relaying an unmeasured claim as a fact.

1. **`trellis_generate.py` shares BOTH halves of the defect** (Stage 1, line 541):
   `shutil.copyfile(source_glb, dest_glb)` overwrites the previous good
   `Cache/<Asset>/trellis_raw.glb` **in place, with no staging**, then reports `SUCCESS`
   with the **byte size as the only evidence**. A GLB with zero meshes, zero vertices or a
   degenerate bbox would pass. Its one mitigation is a presence check on the *response*
   (`/extract_glb` returned no path → raise), which is not a check on the *artefact*.
2. **`meshy_generate.py` has half of it** (line 352 `download_file`): it **does** stage
   (`.part` → `tmp.replace(dest)`), so the prior artefact survives a failed download — but
   there is **no validation between the completed download and the swap**, and `SUCCESS` is
   again reported on byte size alone.
3. **`cardart_render.py` does not exist yet** but is already named in `CONVENTIONS.md:83`.
   ⭐ It should be **born** with this law rather than retrofitted — the cheapest moment to
   fix this family is before the tool is written.
4. **`refine_trellis_glb.py` was NOT audited** for this. I am not claiming it is clean; I am
   claiming I did not look. (`SC-§40` cl. 1.)
5. **The README documents no Stage 0 at all** — `concept_generate.py` is absent from the
   pipeline diagram, the Stage-command list, and the directory-layout table, even though it
   is the tool that produces every `Inbox/<CardID>.png` the diagram starts from. Outside my
   fence (one README line), so **not fixed**. Worth a small row.
6. 🧑 **Environment, unchanged and not mine to fix:** `router.huggingface.co` remains
   Norton-intercepted while apex `huggingface.co` passes bare; `MESHY_API_KEY` is unset.
   Neither was in this task's path — no live call was made.

---

## 7. Assets referenced

**None.** No `/Game/...` asset, no mesh, no material, no texture. This task is tooling only.
The 49 concept PNGs under `Tools/ArtPipeline/Inbox/` and `Content/RawAssets/Concepts/` are
**read** by the test as the negative-control corpus and never written.
