# QA Report — TASK-878 (gate over TASK-876; TASK-874 NOT DELIVERED)

Verdict: **PASS** — 0 BLOCKER · 7 WARN · 5 NIT
Date: 2026-09-03 · Reviewer: qa-reviewer
Law: `SC-§39.1` (incl. cl. 4 + cl. 6) · `SC-§39` · `SC-§40` · `SC-§38` · `SHIP-§9` · `SC-§27` · `SC-§29` · `TL-§5b` · `TL-§5c` cl. 5

---

## SC-§29 coverage ledger — ⛔ THIS GATE COVERS ONE OF ITS TWO NAMED TASKS

| Task | Covered? | Artefacts read |
|---|---|---|
| **TASK-876** | ✅ **YES — gated in full** | `Tools/ArtPipeline/trellis_generate.py` (1,335 lines, whole file) · `Tools/ArtPipeline/meshy_generate.py` (1,678 lines, whole file) · `Tools/ArtPipeline/refine_trellis_glb.py` (audited at the named sites) · `handoffs/TASK-876-programmer.md` · `handoffs/TASK-864-programmer.md` · `qa/TASK-865.md` |
| **TASK-874** | ⛔ **NO — NOT DELIVERED.** Board status is still `boarded`; there is no `handoffs/TASK-874-programmer.md` and no diff to `Tests/SiegeAssistantSelectionTest.cpp`. | — |
| TASK-877 / TASK-879 | ❌ out of scope by the row's own text | — |

⛔ **`TASK-878` therefore CANNOT be closed `done` on this sitting.** Its spec item (a) — *"is the roster DERIVED, or was `13` just bumped to `14`?"* — has nothing to rule on. **A second sitting is owed** once TASK-874 lands, or the manager splits the row. ⚖️ *Recording this as a measurement rather than letting a half-covered gate read as a full one is `SC-§29`'s whole point; `TASK-876` is not held up by it.*

⛔ **This gate does NOT cover:** any C++, any art asset, `concept_generate.py` (that is `TASK-879`), the README (`TASK-877`), or a compile.

---

## 0. ⛔ DECLARED vs EXECUTED — read this BEFORE the verdict (`TL-§5c` cl. 5(a))

⛔ **I did NOT execute either `--check`, either `run_guard_self_test()`, the mutation sweep, the corpus sweep or the behavioural repro. No shell was exposed to this session** — my tool set is Read / Grep / Glob / Write + Slack. The dispatch invited me to run them if I had a shell. **I do not, and reporting a green I did not watch would be this chain's own defect one level up.**

### ⛔ DECLARED — the programmer's, unwatched by me
`trellis --check` and `meshy --check` exit codes · `run_guard_self_test()` = 0 on both, 11 fixtures · **the 12-mutation sweep (12 RED / 0 GREEN)** · the **126/126** corpus acceptance and its three worst-real values (11,820 verts / 9,461 tris / 0.2652 aspect) · the **4-run behavioural repro** (2 REPRODUCED pristine, 2 NOT fixed; 23,434,424 → 132 bytes) · the healthy-artefact end-to-end regression (rc 0, sha matches, no stray temp) · *"the real `Cache/` is untouched, no `.part`, no `.tmp-*`, no `_rejected/`"* · `MESHY_TOKEN` set / `MESHY_API_KEY` unset · `api.meshy.ai` Norton-intercepted · `refine_trellis_glb.py` mtime still 2026-07-28.

### ✅ EXECUTED — my own static measurements, every one re-derived rather than relayed (`TL-§5c` cl. 5(b))

| Claim | Its source | My independent re-derivation | Agrees? |
|---|---|---|---|
| the meta-control yields **7** disagreements on a never-rejects guard | handoff §4 | walked the fixture table: **8** must-REJECT fixtures, but `missing-file` is rejected by `validate_glb_file`'s `OSError` branch **before** `assess_degeneracy` is called, so neutering `assess_degeneracy` cannot reach it ⇒ 8 − 1 = **7** | ✅ **7** |
| **10** on an always-rejects guard | handoff §3 | `lambda: list(ALL_GLB_CHECKS)` agrees with `missing-file` only (same reason) ⇒ 11 − 1 = **10** | ✅ **10** |
| `os.replace` onto `dest` exists **once per tool** | handoff §7 | census: trellis **660** (`commit_staged`) + **690** (`quarantine_staged` → `_rejected/`); meshy **689** + **720** (ditto). Second site targets `_rejected/` in both | ✅ |
| `commit_staged` has **one** caller each, below a `return`ing `except DegenerateArtefactError` | handoff §7 | trellis **1173**, guarded by the `except` at **1140** returning `EXIT_DEGENERATE` at **1168**; meshy **1367**, guarded by the `except` at **1333** returning at **1362** | ✅ |
| exit **6** collides with nothing and is never a literal | handoff §7 | census: `EXIT_DEGENERATE = 6` at trellis:91, meshy:135, concept:257; returned only via the constant (trellis:1168, meshy:1362, concept:929/937). `refine_trellis_glb.py` uses `sys.exit(0/1/2)` only | ✅ |
| the corpus is **126** GLBs | handoff §5 | glob `Cache/**/*.glb` → **126** | ✅ **126** (`SC-§40` cl. 9 — reported although it matches) |
| **19** are trellis's own output | source comment :109 | glob `Cache/*/trellis_raw.glb` → **19** (incl. `Witch/`) | ✅ **19** |
| **33** are meshy's own output (dead agent said 34) | handoff §0, source :150 | glob `Cache/**/meshy_r*.glb` → **34** paths, **one of which is `Cache/Ogre/meshy_raw_task316_42d2ab2.glb`** — a preserved backup, not a canonical output name ⇒ **33** canonically named | ✅ **33 — and I found WHY 34 was written** (§F4) |
| `refine_trellis_glb.py` has **zero** post-write validation | handoff §6 | ran the same positive-control grep myself: `.exists()\|.is_file()\|st_size\|stat()\|getsize` → **exactly 2 hits, :210 and :1433, both input-side** | ✅ **2** |
| the fence held | handoff §2 | `TASK-876` census → **15 hits in exactly 2 files** (trellis 4, meshy 11); wider needle `TASK-876\|SC-39.1\|DegenerateArtefactError\|assess_degeneracy` → **112 hits in 4 code files + 5 pipeline docs**, ZERO in `Source/`, `Content/`, `refine_trellis_glb.py`, `README.md`. **The instrument could see** | ✅ |
| `cardart_render.py` does not exist | board item (5) | glob `Tools/ArtPipeline/*.py` → **14 files**, none of them it | ✅ **14** |

⇒ **A shell-less gate is not a weak gate; it is a gate with a named hole.** The hole is named in §6 and the duty transfers there by name.

---

## 1. ⭐ SPEC ITEM (b) — `SC-§39.1` cl. 4: DOES EACH GUARD GO RED ON A SYNTHESISED DEGENERATE ARTEFACT?

✅ **YES — and I verified it by walking the mutation myself for all SIX raise-paths, in both tools, rather than accepting the 12/12 table.**

`assess_degeneracy` is a **pure function of the stats**, and `measure_glb` **records** malformation instead of raising — that split is what makes each check independently drivable. Deleting each in turn:

| check | isolating fixture | what the fixture yields if the check is DELETED | control goes |
|---|---|---|---|
| `unreadable-artefact` (**OSError** branch, `validate_glb_file`) | `missing-file` (payload `None` ⇒ the file is never created) | `path.read_bytes()` raises `FileNotFoundError` → caught by `_run_fixture_suite`'s generic `except` → **named disagreement** | **RED** ✅ |
| `unreadable-artefact` (**bytes** branch) | `empty-file` / `not-a-glb` / `json-chunk-garbage` | early-return gone ⇒ `empty-file` falls through to `primitives == 0` ⇒ `{no-mesh}` ≠ `{unreadable}` | **RED** ✅ |
| `truncated-container` | `truncated-container` (`declared_delta=64`) | 64 verts / 32 tris / aspect 1.0 all pass ⇒ `ACCEPT` ≠ `{truncated}` | **RED** ✅ |
| `no-mesh` | `no-mesh` (`meshes=0`) | `primitives == 0` ⇒ verts 0 < 4 ⇒ `{no-geometry}` ≠ `{no-mesh}` | **RED** ✅ |
| `no-geometry` | `single-triangle` (3 verts / 1 tri) | aspect 1.0, primitives 1 ⇒ `ACCEPT` ≠ `{no-geometry}` | **RED** ✅ |
| `degenerate-bounds` | `flat-plane` (6 verts / 4 tris, extent 1×1×**0**) | verts 6 ≥ 4, tris 4 ≥ 4 ⇒ `ACCEPT` ≠ `{degenerate-bounds}` | **RED** ✅ |

**Both tools' guard regions are behaviourally identical** — same constants (`MIN_MESH_VERTICES` 4, `MIN_MESH_TRIANGLES` 4, `FLAT_ASPECT_FLOOR` 1e-4), same `ALL_GLB_CHECKS` order, same `measure_glb` / `assess_degeneracy` / `validate_glb_file` / `_synth_glb` / `guard_fixtures` / `_run_fixture_suite` / `run_guard_self_test`. The derivation above therefore holds for both.

**The control lives in the ALWAYS-RUN tier in both**, ahead of all network I/O — trellis `run_check()` **STEP 1** (:941-946), meshy `run_check()` **STEP 1** (:1230-1235) — and a disagreement **returns 1**, i.e. it *blocks generation* rather than logging a note.

**The control has its own control**, injected in both directions (`never-rejects` → 7, `always-rejects` → 10, `restored` → 0, all three asserted at :884-888 / :1204-1208). ⭐ **A guard that rejects everything fails just as loudly as one that accepts everything** — and I confirmed the accept-side design is real, not decorative:
- **`minimal-tetrahedron` has FEWER vertices (4) than the REJECTED `flat-plane` (6)** ⇒ nothing keyed on size or count *alone* can pass one and fail the other. Re-derived from `_synth_glb`'s arguments, not from the comment.
- **`thin-but-legitimate`** is `bounds_max=(1.0, 1.0, 0.01)` ⇒ aspect **0.010**, and 0.2652 / 0.010 = **26.5×** thinner than the thinnest real asset — and it must be **ACCEPTED**. That is what forces the bounds check to key on **collapsed** (aspect *exactly* 0.0) rather than on **thin**. ✅ The dispatch's claim re-derives exactly.

⇒ **Spec item (b): SATISFIED.** No guard here has only ever been seen passing.

## 2. ⭐⭐ SPEC ITEM (c) — IS THE ORDER WRITE-THEN-VALIDATE-THEN-SWAP? (read as control flow, not as prose)

✅ **YES, in both tools, and it is structurally enforced rather than merely sequenced.**

**trellis** (`run_generate`):
```
1135  staged_glb = stage_glb(source_glb, dest_glb)      # PHASE 1 — dest UNTOUCHED
1139  glb_stats  = validate_glb_file(staged_glb)        # PHASE 2 — re-read FROM DISK
1140-1168  except DegenerateArtefactError: quarantine → state_failed.json → return EXIT_DEGENERATE
1173  commit_staged(staged_glb, dest_glb)               # PHASE 3 — the ONLY write to trellis_raw.glb
```
**meshy** (`_finish_task_common`): PHASE 1+2 are inside `download_file` — `open(staged,"wb")` at :755 streams to `<dest>.part`, `validate_glb_file(staged)` at :758, and the function **returns the staged path only from the success branch at :760**. The `except DegenerateArtefactError` at :1333 returns `EXIT_DEGENERATE` at :1362; `commit_staged` is at :1367.

Three independent reasons this cannot be defeated:
1. **`download_file` no longer returns a byte size** — it returns `(staged_path, validated_stats)`. Returning a size *was* the entire evidence base of the defect, and removing the return type removes the temptation.
2. **`os.replace` onto `dest` exists at exactly one site per tool, with exactly one caller each**, and both callers sit **below** a `return`ing `except`. Measured, §0.
3. **Both are routed through `_require_inside_cache()`**, so neither can write outside `Cache/` even on the success path.

⭐ **And the SUCCESS line is no longer decoration.** Both now print `describe_glb(glb_stats)` — *"validated: 22,559,448 bytes, 1 mesh(es)/1 primitive(s), 460,425 verts, 475,801 tris, bounds …, aspect 0.7642"* — i.e. **the measurement beside the word SUCCESS is the measurement that was CHECKED.** That is `SC-§39.1`'s durable sentence discharged in code.

⇒ **Spec item (c): SATISFIED.**

### ⚖️ RULING THE DISPATCH ASKED FOR — the board's *"meshy's destructive half is genuinely already absent"* **UNDERSTATES THE HARM. I UPHOLD THE PROGRAMMER'S CORRECTION, AND I CAN SHARPEN IT.**

The row's **instruction** (fix validation only, do not re-plumb staging) was **correct and was obeyed** — the staging is untouched and there is no diff without a defect behind it. But the **reason** was wrong, and the corrected sentence should replace it in `SC-§39.1`:

> ***Staging protects against a FAILED transfer. Only validation protects against a SUCCESSFUL transfer of a degenerate payload.***

**And the code says so at source, independently of the repro.** `measure_glb`'s own comment at meshy **:496-502** records why `truncated-container` exists: *"the transfer completed but the payload is not the whole file — THIS is the truncation `.part` staging cannot see: the stream simply ended early, `copyfileobj` returned normally and nothing raised."* ⇒ **meshy's staging was blind to at least two of the five failure modes** (`no-mesh`, `truncated-container`), not one, and `tmp.replace(dest)` destroyed the artefact exactly as completely as trellis's bare `copyfile`. ⇒ **"half the defect" was a count of the *code paths*, never of the *harm*. Do not carry it forward as a severity ranking.**

## 3. SPEC ITEM (d) — IS THE DEGENERACY CHECK APPROPRIATE TO A **MESH**?

✅ **YES, and byte size is explicitly and deliberately NOT a gate — in both tools' source, so nobody re-adds it.**

A GLB is not a PNG; "mean pixel value" has no meaning here. The five checks are a **container parse** (magic / version / declared length / JSON chunk), **mesh presence** (`primitives > 0`), **geometry presence** (`vertices ≥ 4` **AND** `triangles ≥ 4`) and **non-degenerate bounds** (`min_extent / max_extent ≥ 1e-4`). They detect the failure that can actually be produced: a well-formed 132-byte container with an empty `meshes` array — the case where *every* "did the file arrive" check passes.

**Thresholds are DERIVED, not corpus-fitted** (`SC-§40` cl. 10), and I accept the derivation: `MIN_MESH_VERTICES` / `MIN_MESH_TRIANGLES` = **the tetrahedron**, the smallest closed solid in 3-space — a floor of *"is it a solid at all"*, never a quality bar (quality is Stage 2's tri budget). `FLAT_ASPECT_FLOOR` is a **relative epsilon**: a collapsed mesh has aspect *exactly* 0.0, so any value in (0, corpus-worst) separates them, and 1e-4 sits ~3 orders of magnitude above float32 noise at unit scale. ⭐ **A derived floor cannot go stale when the data moves; a fitted one goes stale silently.**

⛔ **The original bug WAS a size check, and the fix correctly declines to enshrine the symptom that found it.** Recorded in both tools (trellis :119-125, meshy :160-162): a degenerate frame measured **610 KB** against a legitimate **7 KB** in `TASK-864`, and here a 132-byte degenerate vs a 22.5 MB healthy one differ by size in the *convenient* direction only by luck. **Size stays a printed diagnostic and decides nothing.** ✅ Correct call.

⇒ **Spec item (d): SATISFIED.**

## 4. SPEC ITEM (e) — FENCE

✅ **HELD — measured with a positive control (`SC-§39`), not asserted.**
- `TASK-876` census across `Tools/`: **15 hits in exactly TWO files** — `trellis_generate.py` (4) and `meshy_generate.py` (11). **ZERO** in `concept_generate.py`, `test_concept_guard.py`, `refine_trellis_glb.py`, `README.md`, or any other of the 14 `.py` files.
- Wider needle `TASK-876|SC-39.1|DegenerateArtefactError|assess_degeneracy` across the whole repo: **112 hits in 9 files** — 4 code files (meshy 35, trellis 26, concept 11, test_concept_guard 9 — the last two are **TASK-864's own** symbols, and they carry **no `TASK-876` token**) + 5 pipeline docs. **ZERO in `Source/`. ZERO in `Content/`.** The instrument returned 112 hits, so it could see.
- `cardart_render.py` **still absent** — re-measured against the filesystem: **14** `.py` files in `Tools/ArtPipeline/`, none of them it. **Not created here.** ✅
- `refine_trellis_glb.py` **untouched** — corroborated two ways: zero `TASK-876`/guard symbols in it, and a modification-time-ordered glob puts it **third among the OLDEST** of the 14 files while `concept_generate` / `test_concept_guard` / `trellis_generate` / `meshy_generate` occupy the recent tail in exactly that order. ⚠️ Stated honestly: that is a **census**, not a `git diff` — I have no Git by design.

⇒ **Spec item (e): SATISFIED.**

## 5. ⭐⭐ THE `SC-§39.1` cl. 6 GAP THE PROGRAMMER FOUND IN THE INHERITED WORK — VERIFIED, AND THE LAW **DOES** NEED THE CRASH CASE WRITTEN IN

✅ **The finding is real and the fix lands.** `CHECK_UNREADABLE` is raised from **two structurally distinct sites** — the bytes branch (`measure_glb` sets `unreadable_reason`) and `validate_glb_file`'s `except OSError`. Every pre-existing fixture wrote a file before validating it, so **no fixture ever reached the `OSError` branch.** The `missing-file` fixture (payload `None` ⇒ *ensure the file does NOT exist*, `_run_fixture_suite` :826-828 / :1143-1146) closes it. **11 fixtures, up from 10, in both tools.** And `--help` no longer hard-codes the count in either tool (both now describe the *property* and let the runtime print the live number) — `SC-§38` handled at the point where the number would otherwise rot.

⚖️ **RULING THE DISPATCH ASKED FOR: YES — cl. 6 needs the crash case written in, and it is a genuinely NEW shape, not a restatement.**

Clause 6 as written says a check with no isolating fixture is **UNCONTROLLED and must be DECLARED as such**. It assumes the failure mode is *the control stays GREEN*. **This task measured a second failure mode: deleting the guarded branch did not leave `--check` green — it made `--check` CRASH**, with an uncaught `FileNotFoundError` naming a temp path.

⇒ **A control that DIES is worse than a control that passes**, for a reason that is operational rather than aesthetic: **the obvious response to a crashing control is to "fix" the control**, and the person who does that is the person least equipped to know they are deleting a guard. A green control at least fails silently; a crashing one *actively recruits* the next reader into removing it.

✅ **Proposed rider to `SC-§39.1` cl. 6, and it would be describing SHIPPED code rather than prescribing new work:**
> *A fixture suite CATCHES an unexpected exception and records it as a disagreement NAMING THE FIXTURE. The guard must REJECT, never EXPLODE — a control that dies is a control that will be "fixed" by whoever is unlucky enough to hit it.*

Both tools now implement exactly that (`_run_fixture_suite`'s generic `except Exception` → `f"{name}: … but the guard RAISED {type(exc).__name__}"`, trellis :836-844, meshy :1154-1162).

⚠️ **And the "7 vs 8" number is CORRECT — I re-derived it before flagging it, per the handoff's request.** The never-rejects injection reports **7** disagreements against **8** rejecting fixtures because `missing-file` is rejected by the `OSError` branch *before* `assess_degeneracy` is ever called. The meta-control is measuring exactly what it should. ⭐ *That asymmetry is itself evidence the two raise sites are genuinely distinct — a single-site guard would have given 8.*

---

## Findings

### ⛔ BLOCKER — none.

### ⚠️ WARN

- **[WARN-1] `meshy_generate.py:1570-1579` — `--help`'s exit-code table OMITS exit 6, and the handoff asserts otherwise.** The epilog reads *"Exit codes: 0 success, 1 failure, 2 key unset, 3 credits exhausted (expected pause), 4 API drift, 5 input missing, 64 CLI usage error."* — **there is no 6.** The module docstring (:73-78) has it; `trellis_generate.py`'s epilog (:1256-1260) has it; `concept_generate.py`'s (:1210) has it. ⇒ **handoff §7's *"Documented in each module docstring's exit table and in `--help`"* is FALSE at one of the three sites.**
  ⭐ **And the family already enforces this property on one member: `test_concept_guard.py:460` is a SHIPPED, PASSING assertion — `check(f"{cg.EXIT_DEGENERATE} DEGENERATE" in epilog, …)`.** meshy would fail that exact assertion today.
  ⚠️ **Live consequence:** `TASK-877` is boarded to put exit 6 in the README with the contracts *"read out of the tool, never transcribed from any spec"*. An author who reads meshy's `--help` — the operator-facing surface — will find no 6 and propagate the gap.
  **Fix (one string):** insert `"6 DEGENERATE ARTEFACT (the returned GLB is unreadable/truncated/mesh-less/geometry-less/flat - the previous output GLB is UNTOUCHED and the rejected GLB is kept in Cache/<CardID>/_rejected/), "` between `"5 input missing, "` and `"64 CLI usage error."`, and lift `test_concept_guard.py`'s epilog assertion to both siblings.

- **[WARN-2] `trellis_generate.py:121-122` and `meshy_generate.py:161` — the UNREPRODUCIBLE repro figures SURVIVED, and the handoff says they were rewritten.** handoff §0: *"Its docstring claimed a repro of 'a 428-byte 0-vertex GLB replaced a 20,380,016-byte mesh' — I cannot reproduce those figures and mine differ (132 bytes / 23,434,424 bytes). **Both were rewritten to what I measured.**"* **Measured with a needle that demonstrably works** (the corrected pair returns hits at meshy :50-52 and :666-667):
  - `trellis_generate.py:121-122` still reads *"The TASK-876 repro destroyed a **20,380,016**-byte good mesh with a **428**-byte one"* — **both dead-agent numbers, intact.**
  - `meshy_generate.py:161` still reads *"reported a destroyed **20 MB** mesh"* while its own docstring at :51 says **23,434,424** (= 22.35 MB).
  ⇒ **The two tools' comments now disagree with each other about the same repro**, and the surviving number is the one that *argues against a size gate* — i.e. the load-bearing half of the paragraph. This is `qa/TASK-865.md` WARN-1's exact shape: **a wrong number in a source comment is what the next editor re-tunes from.** Fix: replace both with 132 / 23,434,424, or drop the figures and cite the handoff.

- **[WARN-3] `meshy_generate.py:1334` — `quarantine_staged(exc.path or dest, …)` is a DESTRUCTIVE fallback in the one handler whose entire contract is "the prior artefact is untouched".** If `exc.path` were ever `None`, this **moves `dest` itself** — the prior good artefact — into `_rejected/`, and then line **:1356** prints *"No previous {name} existed; nothing was written"*, because `dest.is_file()` is now False. **A false reassurance printed by the code path that just destroyed the thing it is reassuring you about.**
  ⚖️ **NOT reachable today** — both raise sites in `validate_glb_file` pass `path` (:647, :655), and a `Path` is always truthy — so this is **latent, not live**, which is why it is a WARN and not a blocker. But trellis's equivalent (:1141) passes the explicit `staged_glb` with no fallback, so the two tools differ, and this is precisely the species of default the whole chain exists to remove.
  **Fix:** `quarantined = quarantine_staged(exc.path, asset_dir.name, dest) if exc.path else None`.

- **[WARN-4] `meshy_generate.py:1230-1233` and `:1174-1177` — the load-bearing placement is justified by a premise the same handoff corrects.** The comment reads *"MESHY_API_KEY is unset on this machine, so a control placed after `require_api_key()` would exit 2 and NEVER RUN"*; the docstring reads *"an unset MESHY_API_KEY — which is the state this machine is actually in"*. **The ALIAS is unset; the CANONICAL `MESHY_TOKEN` is set** (handoff FINDINGS (c)), and `resolve_api_key()` reads the canonical **first** (:288-291) ⇒ on *this* machine `--check` does **not** exit 2.
  ⚖️ **RULING THE DISPATCH ASKED FOR: THE ORDERING IS CORRECT AND MUST NOT MOVE.** Ahead of the key check is the right home, because the real case is a **keyless machine** — a clean checkout, a CI runner, a machine after an HKCU rollback (the documented `MESHY_TOKEN` precedent). ⛔ **But the reason as written is falsifiable in ten seconds, and the next reader who falsifies it may conclude the constraint is stale and move the control below `require_api_key()`, killing it on every keyless machine.** A comment that defends a correct decision with a wrong fact is worse than no comment.
  **Fix:** re-word to *"…on any machine with no Meshy credential — a clean checkout, CI, or an HKCU rollback. (The canonical `MESHY_TOKEN` happens to be set on this one; the ALIAS `MESHY_API_KEY` is not.)"*

- **[WARN-5] `meshy_generate.py:748-784` — `download_file` can raise a STALE degenerate error, mismatching the exit code AND the quarantined evidence.** If a *retryable* degenerate on attempt *k* is followed by a **generic transport failure** on the final attempt *n*, the loop exits with `last_degenerate` still set from attempt *k*, and `:778-779 raise last_degenerate` fires. Consequences: **exit 6 (DEGENERATE) is reported when the last thing that actually happened was a transport failure (exit 1)**, and the `.part` on disk carries attempt *n*'s bytes while `exc.stats` — the block written into `state_failed.json` and printed by `describe_glb` — describes attempt *k*'s. ⇒ **the quarantined artefact and its stated measurement can disagree, in the one artefact whose declared purpose is preserving evidence.** Not destructive (`dest` untouched throughout).
  **Fix:** clear `last_degenerate` when a later attempt fails for a different reason, or re-validate the staged file immediately before `raise last_degenerate`.

- **[WARN-6] ⛔ PRE-EXISTING, BOTH TOOLS, OUT OF FENCE — DO NOT FIX UNDER THIS GATE. Write confinement is applied at commit, not at first write, and `asset` is unvalidated argv.** `concept_generate.py` has a `CARD_ID_RE`; **neither sibling does.** `asset_dir = CACHE_DIR / asset` → `asset_dir.mkdir(parents=True)` (meshy :1427-1428, trellis :1004-1007), and in meshy the `.part` (`open(staged,"wb")`, :755) and `state_failed.json` are written **before** any `_require_inside_cache` runs. A traversal-shaped `asset` therefore creates a directory and writes two files outside `Cache/` before `commit_staged`'s check raises and the run exits 1. ⚖️ **The COMMIT itself is confined — the new code is what added that — so this is a residual, not a regression, and it needs a hostile or badly mistyped argv.** Fix when boarded: apply the sibling's `CARD_ID_RE` at argv and call `_require_inside_cache(dest)` at the top of `download_file` / `stage_glb`.

- **[WARN-7] ⛔ PRE-EXISTING, trellis only, OUT OF FENCE — `trellis_generate.py:214, :968, :1050` have no explicit network timeout.** `Client(SPACE_ID)`, `Client(SPACE_ID, token=token)` and `client.view_api(...)` carry none, while every GPU call does (`job.result(timeout=timeout_seconds)`, :334). An unbounded connect/metadata call can hang a "smoke test" indefinitely — the **same shape** as `qa/TASK-865.md` WARN-5 (`model_info` with no timeout). 📌 **That one was re-homed to `TASK-879` because it lives in `concept_generate.py`; this one lives in `trellis_generate.py` and therefore belongs to a trellis-owning row, NOT to `TASK-879`** — noted so nobody re-derives the departure. ✅ **`meshy_generate.py` is CLEAN here:** every remote call passes an explicit timeout (`CREATE_HTTP_TIMEOUT` 600 / `POLL_HTTP_TIMEOUT` 60 / `DOWNLOAD_HTTP_TIMEOUT` 600).

### NIT

- **[NIT-1] `trellis_generate.py:1136-1139`** — if `validate_glb_file` raises anything that is *not* `DegenerateArtefactError` (a `MemoryError` on a very large file, say), the outer generic `except` at :1217 returns 1 and the `.trellis_raw.glb.tmp-<pid>` is **left behind**. meshy cleans up in the same situation (`_discard_staged` at :780). One `finally` away; `dest` is safe either way.
- **[NIT-2] control cost, flagged so nobody "optimises" it.** `run_guard_self_test` calls `_run_fixture_suite` **four** times plus a fifth `guard_fixtures()` for the verbose counts, regenerating every payload each pass — `dense-healthy` alone is a 1.56 MB buffer, so `--check` synthesises and writes roughly 6 MB per invocation. ⭐ **Worth it. This is `qa/TASK-865.md` NIT-6's ruling applied to the mesh guard: DO NOT move the control out of the always-run tier to save a second.**
- **[NIT-3] `describe_glb` (trellis :585, meshy :622)** — `stats.get('aspect') if stats.get('aspect') is None else round(stats['aspect'], 4)` is a triple-lookup way of writing `None if … else round(…)`. Correct; unreadable.
- **[NIT-4] corpus provenance — record "126" as MACHINE-LOCAL.** `Cache/` is gitignored, so a clean checkout sees **zero** GLBs and none of the headroom table (2,955× / 2,365× / 2,652×) is reproducible by anyone else. Same shape as `qa/TASK-865.md` NIT-3. Add one clause to the comment so nobody boards a row on 126.
- **[NIT-5] the guard is duplicated between the two tools** — declared in both files and in the handoff §9(6). ✅ **Correct for this fence** (a shared `artefact_guard.py` is a new file outside the row's `names:`), and both copies say so. **Board the extraction row before a third caller makes it three copies** — `TASK-385`'s `cardart_render.py` is that third caller, and it is already instructed to be born with this law.

---

## 6. ⛔ DUTY TRANSFER — `TL-§5c` cl. 5(c): THE EXECUTION THIS GATE COULD NOT PERFORM, WITH ITS EXIT-CODE TAXONOMY

**Owed by:** the **build-master row that takes `Tools/ArtPipeline/trellis_generate.py` + `meshy_generate.py` into a pathspec.** **The COMMIT is the deadline** — a declared green may travel through a gate; it may not travel through a commit.

```
cd Tools/ArtPipeline
uv run trellis_generate.py --check
uv run meshy_generate.py  --check
```

⛔⛔ **THE EXIT CODE IS NOT THE GATE HERE — THE GUARD LINE IS.** Both `--check`s can legitimately end non-zero **on this machine** *after* the control has already passed: trellis **1** if the HF Space is unreachable, meshy **1** on the Norton TLS interception of `api.meshy.ai`, meshy **2** if no key resolves. **Routing any of those back as a guard failure would burn a QA loop on a machine-local network condition.**

✅ **PASS CRITERION — this exact line must appear FIRST, with these exact numbers, from BOTH tools:**
```
--check: degeneracy guard control PASSED - 11 fixtures (8 rejected, 3 accepted), all 5
checks fire in isolation, and the control itself goes RED when the guard is broken
(7 disagreements with a never-rejects guard, 10 with an always-rejects one)
```
⛔ **11 · 8 · 3 · 5 · 7 · 10.** Any other numbers, or `GUARD CONTROL FAILED`, or an exit **1** printed *before* that line ⇒ **STOP, do not commit, route back to the programmer.**

**Also still DECLARED and not owed before commit** (record as such, do not re-run): the 12-mutation sweep, the 126/126 corpus sweep, the behavioural repro, the healthy-artefact regression.

---

## 7. 🪤 FINDINGS FOR THE MANAGER — ruled on, not fixed

**(F1) ⛔⛔ `refine_trellis_glb.py` EARNS ITS OWN ROW — DIAGNOSIS CONFIRMED AT SOURCE, BY ME, NOT RELAYED.** I opened it and re-ran the positive-control grep myself:

| half | verdict | my own evidence |
|---|---|---|
| **destructive** | ⛔ **PRESENT** | `save_png` (**:721-727**) sets `image.filepath_raw = str(out)` and calls `image.save()` — **in place**, no temp, no swap, onto `Content/RawAssets/Textures/<CardID>/T_<CardID>_D\|_N\|_ORM.png`. `bpy.ops.export_scene.fbx(filepath=str(out))` (**:1302**) writes **directly** to `Content/RawAssets/<CardID>.fbx`. |
| **validation** | ⛔ **ABSENT — and WEAKER than either sibling ever had** | `.exists()\|.is_file()\|st_size\|stat()\|getsize` returns **exactly 2 hits: :210 (`manifest not found`) and :1433 (`input mesh not found`) — BOTH INPUT-SIDE.** After the export, **:1319-1326** stores a dict of **constants** into `report["export"]` and logs `EXPORT: {out}`; after `image.save()` it logs `wrote {out}`. **Nothing reads either file back.** Success = *"the bpy operator did not raise"* — weaker than trellis's old byte-size check, which at least read the file. |

⚠️ **`OutputGuard.check()` (:159-180) is easy to mistake for a guard and IS NOT ONE** — it is *write confinement*: an allow-list of roots plus a CardArt-lane block, followed by `rp.parent.mkdir(...)`. **It validates WHERE you may write, never WHAT you wrote.**
✅ **THE SCOPE VALVE WAS CORRECTLY INVOKED, and I want that on the record as a good call rather than an omission.** It runs inside Blender's bundled `bpy` under `--background`, a different runtime from everything else in this task; it has **no offline `--check` tier at all** to hang a control on, so one would have to be built and proven headless; and this fence forbade a Blender lane. ⚖️ *A half-finished third tool would have been worse than a named one.*
⚠️ **And it is not theoretical: `Cache/Witch/refine_report.json` EXISTS** ⇒ Stage 2 ran for the newest asset in the game through the unvalidated path. Its mitigation is real but partial — `Content/RawAssets/**` is **git-tracked**, so a destroyed FBX is recoverable from the last commit (neither sibling has that; they write to gitignored `Cache/`) — **but work between commits is still lost, and the operator gets NO SIGNAL AT ALL.**

**(F2) ⭐⭐ THE WITCH — THE ORCHESTRATOR'S EXPLANATION TO JONATHAN IS **REFUTED**, AND I CAN NAME THE REAL DIFFERENCE.** The dispatch asked me to check the environment claim because *"I told Jonathan his missing key was why the witch was generated on a different system than the rest of the fleet."* **`Cache/Witch/state.json` is `trellis_generate.py`'s own provenance record and it settles it:**

```
"space": "microsoft/TRELLIS.2",  "started_utc": "2026-09-03T05:11:32+00:00",
"finished_utc": "2026-09-03T05:12:58+00:00",  "status": "success",
"gradio_client_version": "2.5.0",  "output_glb_bytes": 19050252,
"output_glb": "C:\\GitProjects\\GitHub\\GitClaudeUnrealTesting\\GitClaudeUnrealTest\\Tools\\ArtPipeline\\Cache\\Witch\\trellis_raw.glb"
```

1. ⛔ **The Witch was generated HERE, on THIS machine, TODAY (2026-09-03, 05:11→05:12 UTC)** — the record carries **this repo's absolute path**. Not another system.
2. ⛔ **It was generated on TRELLIS / `HF_TOKEN`, not on Meshy** ⇒ **a Meshy key had nothing to do with it**, present or absent.
3. ✅ **THE REAL DIFFERENCE, measured:** `Cache/Witch/` holds exactly four files — `trellis_raw.glb`, `state.json`, `api_schema.json`, `refine_report.json` — and **none of them is `meshy_raw.glb` or `meshy_retex.glb`.** Across the rest of the fleet there are **33** such artefacts, **including `Sorcerer/meshy_raw.glb` and `Wizard/meshy_raw.glb`.** ⇒ ***The Witch is TRELLIS-only; the fleet is TRELLIS → Meshy-retex. The Witch simply never got the Meshy pass.***
4. ⚠️⚠️ **AND THE SHARP COROLLARY NOBODY HAS NOTICED: that `state.json` carries `output_glb_bytes` and NO `artefact_stats` / `artefact_checks_passed` block ⇒ it is the PRE-FIX SCHEMA ⇒ the Witch is the LAST ASSET THIS PROJECT GENERATED WHILE THE DEFECT WAS LIVE**, accepted on **byte size alone**, hours before the fix landed. The 126/126 corpus sweep covers it — **but that sweep is DECLARED, not executed by me.** ✅ **Cheapest possible closure: run `validate_glb_file(Path("Cache/Witch/trellis_raw.glb"))` once. It is offline, tokenless and costs nothing.**

**(F3) ENVIRONMENT CORRECTION — UPHELD, but as CORROBORATION, not measurement (`SC-§40` cl. 1).** ⛔ **I did not read the environment; I have no shell.** What I *can* measure is that the handoff's transcript describes a state that is **unreachable without a resolved key**: `run_check()` returns **2** and prints the six-line setup block when `require_api_key()` yields nothing (:1237-1239), and the message *`API unreachable: URLError: CERTIFICATE_VERIFY_FAILED`* + exit **1** is emitted **only** from the `except` at :1278-1284, **inside the block guarded by that key check**. ⇒ **either a key resolved, or the transcript is fabricated.** Combined with `CONVENTIONS.md`'s own record (TASK-197: Jonathan set the real HKCU var as `MESHY_TOKEN`, *"his actual setup is law"*, `MESHY_API_KEY` deprecated alias), I rule: **`MESHY_TOKEN` is set; `MESHY_API_KEY` — the alias the board named — is not. Anyone boarding meshy work on "no key available" must re-measure.** 🧑 **`api.meshy.ai` Norton TLS interception: DECLARED, not verified by me** — the tool already prints the right guidance and never disables verification (:1280-1283, :1536-1540). ✅ **Zero credits spent: the guard control is pure-stdlib offline, and every fixture is synthesised.**

**(F4) ⭐ THE "34 vs 33" DISCREPANCY HAS A CAUSE, AND NEITHER AGENT FOUND IT.** `Cache/**/meshy_r*.glb` returns **34** paths. **One of them is `Cache/Ogre/meshy_raw_task316_42d2ab2.glb`** — a preserved backup, not a canonical output name. ⇒ **33 is correct as the comment words it** (*"meshy_raw.glb / meshy_retex.glb"*), **34 is correct for "files this script produced"**, and **the dead agent was not simply wrong.** ⚖️ *Recorded so the number is not re-litigated a third time — and as a small vindication of `SC-§40` cl. 9: the programmer was right to re-measure, and right to report the disagreement rather than quietly adopting a number.*

**(F5) ⛔ THE FAMILY MAY BE LARGER THAN FOUR — THE AUDIT COVERS 4 OF 14 TOOLS.** `fix_rig_root.py:388` also does `os.replace(tmp_out, dst)` onto a **`Content/`** path. ⭐ **That is a POSITIVE datapoint, not a new defect:** its own docstring step 6 reads *"REPLACE — `os.replace` the **verified** temp file onto the Content path"*, i.e. it appears to already carry stage → verify → swap, independently. **But eight other artefact-producing tools have never been looked at** (`rescale_refined_fbx.py`, `build_watchtower.py`, `build_warroom_props.py`, `build_entry_dressing_props.py`, `author_climb_anim.py`, `retarget_meshy_to_siegebiped.py`, `rig_character.py`, `verify_watchtower_fbx.py`). ⇒ **an UNKNOWN, not a pass** (`SC-§40` cl. 1). Recommend either widening the `refine_trellis_glb.py` row or boarding a cheap census: *which of the 14 produce an artefact, and which validate it before reporting success* — one grep per file, and it is how you find out whether this is a four-tool defect or a directory-wide one.

**(F6) ✅ ENDORSED — the recommended `SC-§39.1` clause, WITH the detector attached.** *A compile is a SYNTAX measurement, never a COMPLETENESS measurement.* ⭐ **Include the cheap detector, because it is the operational half and it is what actually caught this: GREP THE DOCSTRING'S NAMED SYMBOLS AGAINST `def`.** The dead agent's `meshy_generate.py` documented `validate_glb_file` and `commit_staged` **in the present tense** while defining **neither**, and `py_compile` agreed with it, because prose is not a syntax error and an unused constant is not a syntax error. ⚖️ ***An untouched file is honestly broken; that one documented a guard it did not have*** — which is strictly worse, because the next reader greps the docstring.
📌 **And the second clause this task bought: the crash rider on cl. 6 (§5 above) — a control that DIES is worse than one that passes, because a crash recruits the next reader into deleting the guard.**

**(F7) ⛔ `TASK-878` IS NOT CLOSED.** `TASK-874` was never delivered (still `boarded`; its blocker `TASK-849` and the `TASK-868` lane adjacency are unchanged). **Spec item (a) — *"is the roster DERIVED, or was 13 just bumped to 14?"* — has nothing to rule on, and I decline to imply otherwise.** Split the row or re-open it; `TASK-876` must not wait on it.

---

## 8. Notes for build-master

1. ✅ **The PASS report is `.claude/pipeline/qa/TASK-878.md` (this file). It gates `TASK-876` ONLY** — `TASK-874` is not in this verdict and must not be committed under it.
2. ⛔ **NO COMPILE, NO SUITE.** Python, not in the module; it cannot break a build and cannot move a test count. Do not burn a `QUIET-MODULE` slot.
3. ⭐⭐ **DO §6 FIRST — the two `--check` invocations, and grade them on the GUARD LINE (11 · 8 · 3 · 5 · 7 · 10), NOT on the exit code.** A trailing exit 1 (Norton TLS / Space unreachable) or 2 (no key) **after** a printed `control PASSED` is an expected pass on this machine. Record both transcripts verbatim in your handoff — that converts this gate's whole first column from DECLARED to EXECUTED.
4. **Pathspec — exactly two paths:** `Tools/ArtPipeline/trellis_generate.py` · `Tools/ArtPipeline/meshy_generate.py`.
   ⛔ **NOTHING ELSE in `Tools/ArtPipeline/`.** In particular: `concept_generate.py` + `test_concept_guard.py` are **`TASK-864`/`TASK-866`'s uncommitted work** (present before this task started and outside this verdict — committing them here creates the ungated-sweep hazard that holds `TASK-877` and `TASK-879`); `pipeline_manifest.json` and `concept_prompts.json` are the art-director's; `README.md` is `TASK-877`'s.
   ⛔ **`Cache/` is gitignored** — verified in the tools' own contract; no quarantined artefact, `.part` or `state*.json` can enter git.
5. **Carry WARN-1 forward by name.** meshy's `--help` still omits exit 6, and `TASK-877` is boarded to transcribe that table into the README. Either the one-string fix rides a follow-up row **before** `TASK-877` runs, or `TASK-877` must be told in writing to take the exit table from the **module docstring**, not from meshy's `--help`.
6. **No push.** Name the host commit in your handoff.

---

## Board status lines — ⛔ RETURNED, NOT APPLIED (I have no line-editing tool and will not whole-file `Write` a board other agents are editing)

`#### TASK-876` line 15030 →
```
- status: ✅ **qa-passed (2026-09-03, `qa/TASK-878.md` — 0 BLOCKER · 7 WARN · 5 NIT) — ready-for-integration.** ⭐⭐ **THE GATE RE-DERIVED EVERY STATICALLY CHECKABLE CLAIM AND ALL OF THEM REPRODUCED:** ⛔ the meta-control's **7** and **10** disagreement counts (⛔ and **WHY 7 not 8** — `missing-file` is rejected by the `OSError` branch *before* `assess_degeneracy` runs) · ⛔ `os.replace` **twice per tool**, second site → `_rejected/` · ⛔ `commit_staged` **ONE caller each** (trellis `:1173`, meshy `:1367`) **below a `return`ing `except DegenerateArtefactError`** · ⛔ exit **6** = `EXIT_DEGENERATE` in all three tools, **never a literal** · ⛔ corpus **126** GLBs / **19** trellis / **33** meshy · ⛔ **14** `.py` files, `cardart_render.py` **still absent** · ⛔ **FENCE HELD, positive control: `TASK-876` = 15 hits in EXACTLY 2 files, ZERO in `Source/`, `Content/`, `concept_generate.py`, `refine_trellis_glb.py`, `README.md`.**
  ✅ **`SC-§39.1` cl. 4 SATISFIED — the gate walked ALL SIX raise-paths' mutations ITSELF rather than accepting the 12/12 table: every one goes RED.** ✅ **cl. 3 SATISFIED — stage → validate → swap, structurally** (`download_file` **no longer returns a byte size**; the swap moved to the caller so it exists at one guarded site). ✅ **The check is MESH-appropriate and byte size is DEMOTED TO A DIAGNOSTIC IN SOURCE in both tools.**
  ⚖️ **THE BOARD'S OWN SCOPING IS CORRECTED AND THE GATE UPHOLDS IT: *"meshy's destructive half is genuinely already absent"* UNDERSTATES THE HARM.** ⛔ **The instruction was right; the reason was wrong.** ⇒ ***STAGING protects against a FAILED transfer; ONLY VALIDATION protects against a SUCCESSFUL transfer of a degenerate payload.*** ⭐ **Corroborated at SOURCE independently of the repro: `measure_glb`'s own comment (meshy `:496-502`) says the stream can end early while `copyfileobj` returns normally and nothing raises** ⇒ ⛔ **meshy's staging was blind to TWO of the five checks, not one. "Half" counted CODE PATHS, never HARM — do NOT carry it forward as a severity ranking.**
  ⚠️ **WARNs (none blocking):** ⛔ **meshy's `--help` epilog OMITS exit 6** while the docstring and BOTH siblings carry it — and `test_concept_guard.py:460` is a **SHIPPED PASSING ASSERTION of exactly that property** ⇒ ⛔ **`TASK-877` must NOT transcribe meshy's `--help`** · ⛔ **the dead agent's unreproducible `428` / `20,380,016` SURVIVE at `trellis_generate.py:121-122`** (and "20 MB" at `meshy:161`) **despite handoff §0 saying both were rewritten** — the two tools now disagree with each other · ⛔ `meshy:1334` `exc.path or dest` is a **latent DESTRUCTIVE fallback** (would quarantine `dest` itself, then print "nothing was written") · ⛔ the `--check` placement comment defends a **CORRECT** decision with a **wrong fact** ("MESHY_API_KEY unset on this machine" — the CANONICAL `MESHY_TOKEN` IS set) · ⛔ `download_file` can raise a **STALE** degenerate ⇒ exit 6 for a transport failure, with quarantined bytes that do not match the reported stats · pre-existing/out-of-fence: write confinement applied at commit rather than first write + unvalidated `asset` argv (both tools) · trellis's `Client()`/`view_api()` have **no explicit timeout** (⛔ belongs to a TRELLIS row, **NOT** `TASK-879`).
  🪤 **FINDINGS CONFIRMED BY THE GATE AT SOURCE:** ⛔ **`refine_trellis_glb.py` DEFECTIVE BOTH HALVES — the gate ran the positive-control grep ITSELF: exactly `2` hits, `:210` + `:1433`, BOTH input-side; `:1319-1326` stores CONSTANTS and logs `EXPORT:` — nothing reads the file back; `OutputGuard.check()` is write-CONFINEMENT.** ✅ **SCOPE VALVE CORRECTLY INVOKED — endorsed, not an omission.** ⚠️ **`Cache/Witch/refine_report.json` EXISTS ⇒ it already ran on the newest asset.**
  ⭐⭐ **AND THE GATE MEASURED THE WITCH: `Cache/Witch/state.json` shows TRELLIS.2, `2026-09-03T05:11:32Z`, status success, 19,050,252 bytes, at THIS MACHINE'S ABSOLUTE PATH** ⇒ ⛔ **"generated on a different system because the Meshy key was missing" is REFUTED on BOTH counts.** ⭐ **The REAL difference: the Witch has NO Meshy pass at all** (4 files, none of them `meshy_*`; the fleet has 33 incl. `Sorcerer` + `Wizard`). ⚠️⚠️ **And that state.json has `output_glb_bytes` and NO `artefact_stats` ⇒ PRE-FIX SCHEMA ⇒ THE WITCH IS THE LAST ASSET GENERATED WHILE THE DEFECT WAS LIVE — accepted on BYTE SIZE ALONE.** ✅ **Owed: one offline `validate_glb_file` over `Cache/Witch/trellis_raw.glb`.**
```

`#### TASK-878` line 15076 →
```
- status: ⚠️ **PARTIALLY DONE (2026-09-03) — VERDICT: PASS over `TASK-876`. `qa/TASK-878.md`. 0 BLOCKER · 7 WARN · 5 NIT.** ⛔⛔ **`TASK-874` WAS NEVER DELIVERED (still `boarded`) ⇒ spec item (a) HAS NOTHING TO RULE ON AND THE GATE DECLINES TO IMPLY OTHERWISE** (`SC-§29`: the ledger names what was covered AND what was not). ⇒ ⛔ **SPLIT THIS ROW or RE-OPEN it when `TASK-874` lands — `TASK-876` must NOT wait on it.**
  ⛔ **`TL-§5c` cl. 5 DECLARED FIRST, ABOVE THE VERDICT: the gate had NO SHELL** (read/grep/glob/write + Slack). ⛔ **NOT EXECUTED:** both `--check`s · the 12-mutation sweep · the 126/126 corpus sweep · the 4-run behavioural repro · the healthy-artefact regression · the env/Norton claims. ✅ **EXECUTED STATICALLY AND ALL REPRODUCED:** 7/10 meta-control counts · both `os.replace` censuses · both single call sites and their guard clauses · the exit-6 census across three tools · **126**/**19**/**33** corpus globs · **14** `.py` files · the `refine` 2-hit positive control · the `TASK-876` fence census (**15 hits, 2 files**) · `Cache/Witch/state.json`.
  ⛔ **DUTY TRANSFERRED BY NAME to the build-master row that takes the two tools into a pathspec, with the COMMIT as the deadline — and with its EXIT-CODE TAXONOMY:** ⛔ **the exit code is NOT the gate, the GUARD LINE is** (`11 fixtures (8 rejected, 3 accepted), all 5 checks ... 7 ... 10`). ⛔ **A trailing exit 1 (Norton TLS / Space unreachable) or 2 (no key) AFTER a printed `control PASSED` is an EXPECTED PASS on this machine — routing it back would burn a loop on a machine-local network condition.**
  ⚖️ **RULINGS THE DISPATCH ASKED FOR:** ⛔ **(1) the "destructive half absent" phrasing UNDERSTATES the harm — UPHELD, and corroborated at source independently of the repro** · ⛔ **(2) `SC-§39.1` cl. 6 DOES need the crash case written in — a control that DIES is WORSE than one that PASSES, because a crash RECRUITS the next reader into deleting the guard; proposed rider already describes SHIPPED code** · ✅ **(3) the `refine_trellis_glb.py` scope valve was CORRECTLY invoked — endorsed as a good call** · ✅ **(4) meshy's `--check` ordering (control AHEAD of the key check) is CORRECT and MUST NOT MOVE — but its justifying comment is FALSE about this machine** · ⛔ **(5) the "generated on another system" explanation for the Witch is REFUTED — measured from the tool's own provenance record.**
```
