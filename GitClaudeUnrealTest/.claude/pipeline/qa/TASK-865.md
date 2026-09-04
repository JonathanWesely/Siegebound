# QA Report — TASK-865 (gate over TASK-864)

Verdict: **PASS** — 0 BLOCKER · 5 WARN · 6 NIT
Date: 2026-09-02 · Reviewer: qa-reviewer · Law: `SC-§39.1` · `SC-§39` · `SC-§40` · `SHIP-§9` · `SC-§29` · `TL-§5c`

## SC-§29 coverage ledger

| Task | Covered by this gate | Artefacts read |
|---|---|---|
| **TASK-864** | ✅ **YES — the only task this gate covers** | `Tools/ArtPipeline/concept_generate.py` (1,319 lines, whole file) · `Tools/ArtPipeline/test_concept_guard.py` (570 lines, whole file) · `Tools/ArtPipeline/README.md` (measured) · `handoffs/TASK-864-programmer.md` |
| TASK-866 (build-master rider) | ❌ not gated — it commits, it does not code | — |
| everything else on the board | ❌ out of scope | — |

⛔ **This gate does NOT cover:** any C++, any art asset, the sibling tools' own repair (they are a FINDING, §6 below), or a compile. Explicitly per the row's OUT OF SCOPE clause.

---

## 0. ⛔ DECLARED vs EXECUTED — read this before trusting anything below (`TL-§5c`)

⛔ **I did NOT execute `--check` and I did NOT execute `test_concept_guard.py`.** No shell/Bash tool was exposed to me in this session (my tool set was read/grep/glob/write + Slack only). The dispatch invited me to run them if I had a shell; I did not have one, and reporting a green I did not watch would be the exact defect this task exists to fix, one level up again.

⇒ **These remain DECLARED by the programmer, not EXECUTED by the gate:**
`--check` EXIT 0 · `test_concept_guard.py` 46/46 · the 49/49 corpus acceptance · the `[repro] PREMISE NOT REPRODUCED` re-run · the "real `Inbox/Witch.png` untouched" timestamp claim.

✅ **What I did instead, and what it is worth.** Rather than relay, I re-derived the load-bearing numbers by hand from the source. **Every one reproduced.** These are my own measurements:

| Claim | Source of the claim | My independent re-derivation | Agrees? |
|---|---|---|---|
| `os.replace` onto the target has **exactly one call site** | handoff §2(2) | symbol census: `os.replace` at **588** (`commit_staged`) and **618** (`quarantine_staged`, targets `_rejected/`). `commit_staged` appears at 582 (def), 568/585 (docstrings), **908 (the ONE call)** | ✅ |
| that call site is inside `if not failed_checks:` | handoff §2(2) | line 906 `if not failed_checks:` → 908 `commit_staged(...)`. `generate_one` has exactly **two** `return 0` — line 833 (skip, generates nothing) and line 912 (post-commit) | ✅ |
| the control yields **7** disagreements on a never-rejecting guard | handoff §3 | fixture census: **7 must-REJECT** (pure-black, near-black-noise, dark-noise-unlit, pure-white, flat-mid-grey, fully-transparent, tiny-glitch) + **3 must-ACCEPT**. A `lambda: []` guard disagrees with exactly the 7 | ✅ **7** |
| **8** disagreements on an always-rejecting guard | handoff §3 | `lambda: [(CHECK_FLAT,…)]` agrees only with pure-white and flat-mid-grey → **8** | ✅ **8** |
| **46** assertions | handoff §4 | `check()` census on the happy path: 1+11+5+4+2+2+6+3+2+3+3+4 = **46** | ✅ **46** |
| **49** real concepts in the corpus | handoff §2(1) | glob: `Tools/ArtPipeline/Inbox/*.png` = **25**, `Content/RawAssets/Concepts/*.png` = **24**, total **49** | ✅ **49** (`SC-§40` cl. 9 — reported even though it matches) |
| exit **6** collides with nothing | handoff §2(4) | return-code census: `trellis_generate.py` = {0,1,2,3,4,5} + `self.exit(64)`:592 · `meshy_generate.py` = {0,1,2,3,4,5} + `self.exit(64)`:883. **No `6` anywhere in either** | ✅ |
| `very-dark-thin-lit-edge` mean **0.023** and must be ACCEPTED | handoff §3 | hand-computed from the fixture generator (ITU-601 luma; base L=2, silhouette L=12 over π·65²=13,273 px, lit edge L=110 over 13×97=1,261 px of 65,536): **mean = 5.911/255 = 0.02317**, **stddev = 0.0591**, **p99.9 = 110/255 = 0.4314** ⇒ p99.9 clears the 0.12 floor by 3.6× ⇒ **ACCEPTED** | ✅ **0.0232** |

⇒ **The static half of this gate is strong; the executed half is absent.** I am comfortable passing because *every* claim I could check without a shell reproduced exactly, including two that would have been easy to fudge (7/8 and 46). ⚠️ **build-master (TASK-866) should run `uv run concept_generate.py --check` once before it adds the path to a pathspec** — that is ten seconds and it converts the whole first column from DECLARED to EXECUTED. Recorded in §7.

---

## 1. ⭐ THE ENTIRE POINT OF THIS GATE (spec item 1): did the guard ship with a control that proves it can go RED?

✅ **YES, and the design is better than the spec asked for.** Three independent reasons, each verified at source:

**(a) The control's HOME is right, and I am ruling on it as asked.** `run_guard_self_test()` (788-807) is wired into `run_check()` as **step 4/5** (1055-1071), and a disagreement **returns 1** — it does not warn, it does not log a note, it **fails the smoke test**. ⇒ a broken guard **blocks generation**. ✅ **This is the correct home and I would have flagged its absence.** The reasoning that decides it: the fixtures live in `concept_generate.py` (`build_guard_fixtures`, 671-785), *not* in the test file, and `test_concept_guard.py:222` re-imports the **same** function. So the two tiers cannot drift apart — there is exactly one fixture set and the cheap tier runs on every invocation. ⚖️ *A control that lives only in a file someone has to remember to run is a control with a known failure mode; `SC-§39.1` exists because `TASK-833` performed this very check once, by hand, outside the tool, and it died with the task.* Putting it in `--check` is the structural answer to that sentence.

**(b) A dead check really would be caught — VERIFIED, not accepted.** The assertion is `fired != expected` on the **exact set** (799), not a boolean "was it rejected". I checked isolation by hand-computing every fixture's statistics against `assess_degeneracy` (453-486):

| check | its isolating fixture | hand-derived stats | fires alone? |
|---|---|---|---|
| `no-lit-content` | `dark-noise-unlit` (LCG uniform 0..20) | stddev ≈ 6.06/255 = **0.0238** (clears the 0.010 flat floor), p99.9 ≈ 20/255 = **0.0784** (< 0.12) | ✅ `{no-lit-content}` |
| `no-structure` | `pure-white` (255,255,255) | stddev **0.000**, p99.9 **1.000** — uniform but BRIGHT, so the guard is provably not a darkness test | ✅ `{no-structure}` |
| `degenerate-size` | `tiny-glitch` (16×16 checker) | 16 < 64 px; stddev **0.431**, p99.9 **0.941** — perfect statistics, rejected on size alone | ✅ `{degenerate-size}` |
| `fully-transparent` | `fully-transparent` (RGBA checker, alpha 0) | `convert("L")` ignores alpha ⇒ stddev **0.431**, p99.9 **0.941**; `alpha_max` **0.0** | ✅ `{fully-transparent}` |

⇒ **delete any one of the four and its isolating fixture returns `set()` ≠ `expected`** ⇒ disagreement ⇒ `--check` exits 1. **A dead check cannot hide behind a live one.** Confirmed by derivation, not by reading the comment that says so.

**(c) Both directions are controlled, and the accept direction is the one that matters.** `very-dark-thin-lit-edge` (mean **0.0232** — measurably darker than a "near-black" degenerate frame at mean 0.01–0.03) **must be ACCEPTED**, and my hand-derivation above confirms it is: it survives *only* because a 1,261-px lit edge pushes p99.9 to 0.431. ⇒ the guard is forced to key on **"is anything lit"** rather than on mean brightness, and a `return [everything]` guard fails 8 of 10 fixtures. ✅ **The dispatch was right that this is the direction that matters — a guard that rejects everything is the same defect in a different hat — and it is genuinely controlled.**

**(d) The control is proven able to go red.** `test_the_control_itself_can_go_red` (469-527) injects both broken guards, asserts `--check` exits **1** while broken, **and** re-asserts it exits **0** once restored — the last assertion is what stops the first two from being satisfied by a permanently-red instrument. ⛔ I could not *run* it (§0), but I re-derived its two counts (**7** and **8**) exactly, which is the strongest static evidence available that the assertions are live rather than decorative.

⇒ **Spec item (1): SATISFIED.**

## 2. `--force` non-destructive-until-validated (spec item 2) — read as ORDER, not as intent

✅ **PASS, measured three ways.**

1. **Single write site.** `os.replace` onto `dest` exists at **exactly one** place (588), called from **exactly one** place (908), and 908 sits under `if not failed_checks:` (906). The only other `os.replace` (618) targets `Inbox/_rejected/` and is itself confined by `_require_inside_inbox(target, allow_subdir=True)` **before** the move (616→618).
2. **No path reaches it otherwise.** `stage_png` (565-579) writes `.<CardID>.png.tmp-<pid>` **beside** dest and never to dest; `CARD_ID_RE` forbids a leading dot so a staged name can never collide with an artefact name; `_require_inside_inbox` is applied on stage (571), commit (587) and quarantine (616). Failure paths (`_discard_staged` 591-596) delete only the temp.
3. **The behavioural proof is the right one.** Test **3b** snapshots the target's sha256 *inside* a spy on `validate_png_file` and does it on a **PASSING** roll — the only case where "validate→overwrite" and "overwrite→validate" are otherwise indistinguishable. And its companion assertion (307-309) requires the target to have **changed** afterwards, so the test cannot be satisfied by a guard that simply refuses everything. ⭐ **That is the correct experiment; I would have designed the same one.**
4. **Rejection preserves evidence.** `quarantine_staged` (599-624) collision-suffixes within the same second (612-615) so a retry loop cannot overwrite its own evidence, and `Inbox/` is gitignored — **verified at source, `.gitignore:25` `Tools/ArtPipeline/Inbox/*` with only `.gitkeep` negated (26)** ⇒ `_rejected/` never enters git.

⇒ **Spec item (2): SATISFIED.** Losing existing work was the worse half of this bug and it is structurally closed.

## 3. Exit code (spec item 3)

✅ **PASS.** `EXIT_DEGENERATE = 6` (257). **Census measured, not relayed:** `trellis_generate.py` returns {0,1,2,3,4,5} (373,382,385,395,409,423,426,549,560,567,574) + `self.exit(64)` at 592; `meshy_generate.py` returns {0,1,2,3,4,5} (623,648,651,656,661,668,670,742,749,833,840,847,854,866) + `self.exit(64)` at 883. **`6` appears in neither.** No collision. Documented in the module docstring (58-62), in `--help`'s epilog (1208-1210), and asserted by the gate (457-461). The taxonomy reasoning is also correct: this is not 5 (nothing to read) and not 4 (the endpoint moved) — the call succeeded and the model is fine.

## 4. Fence (spec item 4)

✅ **HELD — measured with a positive control, per `SC-§39`.** A symbol census for `TASK-864|SC-§39.1|degenerac|assess_degeneracy` across `Tools/` returns **37 hits in exactly 2 files** (`concept_generate.py` 22, `test_concept_guard.py` 15) and **zero** in `trellis_generate.py`, `meshy_generate.py`, `refine_trellis_glb.py`, `rig_character.py`. **The instrument could see (37 hits); it saw nothing in the siblings.** ⚠️ Stated honestly: this is a symbol census, **not a `git diff`** — I have no Git by design, so "one README token and no more" is DECLARED. The census is strong corroboration, not a diff.
✅ **README:** current line 75 reads `--card-id Footman`, and it is verified correct against `refine_trellis_glb.py:191` (`--card-id`, `required=True`) — and against the **absence** of any `--asset` argument in that file's `add_argument` list (191-204). The programmer's correction of the spec's own item (5) is **upheld**: that README line documents Stage 2, not Stage 0.

## 5. Secrets (spec item 5)

✅ **CLEAN — enumerated rather than asserted.**
- `HF_TOKEN` read **only** via `os.environ.get` at **972** (`require_token`) and **1074** (`--check`, presence only, value never read into a message). No `--token`/`--key` argument exists in `build_parser` ⇒ **never on argv**.
- Registered once (981) so `redact()` (209-213) scrubs the live value from **every** output path; `say`/`warn`/`fail` (216-225) all redact, and the generic `hf_[A-Za-z0-9]{15,}` pattern (199) covers output emitted *before* registration — including `_Parser.error` (1193), which redacts a token accidentally pasted on the command line.
- Exception text is double-redacted on the risky path (882 then 887/894).
- **The only file this tool writes is a PNG** (575) — no state file, no log file ⇒ no secret can reach disk.
- The new failure report (`_print_degenerate_report`, 944-963) prints **image statistics and file paths only** — no prompt, no token.
- `test_concept_guard.py`: no secret; the one token-shaped literal is `"not-a-real-token"` (427), a stub return consumed by a stand-in client.
- **Write confinement:** every write is inside `INBOX_DIR` or `INBOX_DIR/_rejected` and guarded (571/587/616); the test redirects `INBOX_DIR` to `tempfile.mkdtemp()` for every end-to-end case and only ever *reads* the real corpus (`Image.open`, `shutil.copy2` source). No write escapes into another chain's territory.

⇒ **Spec item (5): SATISFIED.**

---

## Findings

### WARN

- **[WARN-1] `concept_generate.py:168-171` (and handoff §2(1)) — the threshold ledger's margin sentence is FALSE for `MIN_LUMA_STDDEV`, and the ledger is exactly what the next editor will re-tune from.** The comment says *"each floor sits near the geometric mid-point of the gap it straddles, so the guard has a comparable margin against BOTH failure directions"*, and the handoff says *"~3.5× above the worst degenerate case"*. **Re-derived from the table printed immediately above it:**
  | floor | vs worst degenerate | vs worst legit-dark | vs corpus worst | geometric mid-point of the straddled gap |
  |---|---|---|---|---|
  | `MIN_HIGHLIGHT_P999` 0.12 | **3.9×** above 0.031 | 3.6× below 0.435 | 6.2× below 0.741 | 0.152 — **claim holds** |
  | `MIN_LUMA_STDDEV` 0.010 | **1.45×** above 0.0069 | 3.5× below 0.035 | 8.4× below 0.084 | 0.024 — **claim does NOT hold** |
  The "3.5×" is a real number attached to the **wrong side**: it is the margin *below the legitimate-dark case*, not *above the degenerate case*. And 1.45× vs 8.4× is not "comparable". This is `SC-§38` cl. 8 in miniature — the snippet is normative, the sentence rotted.
  ⚖️ **Ruling on the thresholds themselves, as asked: DEFENSIBLE, and the thin stddev margin is not load-bearing.** The hard degenerate case (`near-black-noise`) is rejected **twice over** — `no-lit-content` fires at p99.9 = 0.0196 with a 6× margin, so even if the stddev floor were slid up or down by 1.5× the frame is still rejected. `no-structure` is the *supporting* check (it exists to catch flat white/grey, where it has effectively infinite margin), not the black-frame check. The composite is sound.
  **Fix:** correct the comment at 168-171 to the four-column table above, and have `test_concept_guard.py` print the **degenerate-side** margin alongside the corpus-side one (it currently prints only 207-210, i.e. only the half that looks generous).

- **[WARN-2] `concept_generate.py:450, 500-532, 904` — `unreadable-artefact` is the one check with NO control inside `--check`.** `run_guard_self_test` drives `measure_image` + `assess_degeneracy` on in-memory fixtures; `CHECK_UNREADABLE` is raised only by `validate_png_file`, which nothing in `--check` calls. ⇒ **delete the zero-byte/decode guard (518-530) and `--check` stays green.** Its only control is `test_concept_guard.py:366-393` — the second tier, i.e. exactly the "file nobody remembers to run" the design correctly rejected for the other four. This also softens the handoff's claim that the guard "catches the `save_asset` wrote-nothing shape": the *capability* is real (verified by reading 512-530 — `stat()` → `size <= 0` → forced `image.load()`), but inside `--check` it is **uncontrolled**.
  **Fix (cheap):** add two disk fixtures to `run_guard_self_test` via `tempfile` — a zero-byte `.png` and a truncated/garbage `.png` — asserting `DegenerateArtefactError` from `validate_png_file`. That makes all **five** checks controlled in the tier that always runs.

- **[WARN-3] `concept_generate.py:908` — `commit_staged` is the one unguarded I/O call in the fixed path.** On Windows `os.replace` raises `PermissionError` if `dest` is open in another process (an image viewer on `Inbox/Witch.png` is an entirely ordinary state on this machine). That escapes `generate_one` → `run_generate`/`run_all` → `main` as an **uncaught traceback**, bypassing the clean exit-code contract and leaving `.{CardID}.png.tmp-<pid>` in the Inbox. Not destructive — `dest` survives — but ungraceful in a tool whose whole thesis is graceful non-destruction, and `--all` would abort the whole batch on one locked file.
  **Fix:** `try: commit_staged(...) except OSError as exc: fail(...); _discard_staged(staged); return 1`.

- **[WARN-4] `concept_generate.py:830-833` — the skip path returns 0 without ever looking at the artefact it is blessing.** `if dest.exists() and not args.force: return 0`. This is not hypothetical: per `TASK-833`, a black frame **was** written into the Inbox by the pre-fix tool, so a degenerate `Inbox/<CardID>.png` can already exist on disk. A later `--all` prints *"already exists - skipping"*, returns **0**, and Stage 1 consumes the black frame. `SC-§39.1` cl. 1 is scoped to the artefact the tool *produces*, so this is not a spec violation — but it is the same family, it is inside the fence, and it is ~4 lines.
  **Fix:** on the skip path call `validate_png_file(dest)` and `warn` (never `fail`, never delete) if it is degenerate, naming the `--force --seed <new>` re-roll. Deliberately a warning: the existing file is Jonathan's to keep.

- **[WARN-5] `concept_generate.py:1084` — `model_info(MODEL_ID)` under `--probe` has no explicit timeout**, while every other remote call does (`build_client` 819 forwards `--timeout-minutes`). An unbounded metadata call can hang a "smoke test" indefinitely. ⛔ **PRE-EXISTING and OUTSIDE TASK-864's fence — do NOT fix it under this gate.** Recorded so it rides the family task (§6). Fix when it is boarded: `model_info(MODEL_ID, timeout=30)`.

### NIT

- **[NIT-1] `concept_generate.py:1229-1232`** — `--check`'s `--help` text still describes only *"deps import, client signature, prompts schema, and HF_TOKEN presence"*. The degeneracy-guard control is now step **4/5** and the load-bearing part; `--help` is where an operator looks. Add *"and the degeneracy guard's positive control"*.
- **[NIT-2] `concept_generate.py:753`** — comment says the fixture's mean is *"~0.018"*; I derive **0.0232**, and the handoff/board say **0.023**. One of the two numbers rotted; the derivation says the comment did.
- **[NIT-3] corpus provenance** — the "49" is **25 gitignored** (`Inbox/`, `.gitignore:25`) + **24 tracked** (`Content/RawAssets/Concepts/`). On a clean checkout the negative control is **24**, not 49, and the printed headroom changes. The design handles the degenerate case correctly (empty ⇒ exit **5 VOID**, never green — ⭐ that is `SC-§39` applied properly and I want it on the record as a good decision). Just record "49" as machine-local so nobody boards a task on it.
- **[NIT-4] `test_concept_guard.py:515, 525`** — the meta-control calls the real `run_check`, which parses the live `concept_prompts.json`. That file is currently under art-director edit (`TASK-833`); a malformed entry makes `run_check` return 5 and turns **both** meta-control assertions red for a reason that has nothing to do with the guard. Consider pointing `cg.PROMPTS_PATH` at a temp stub for those two calls.
- **[NIT-5] `concept_generate.py:1165`** — `worst = worst or rc` means an earlier generic `1` masks a later `6` in `--all`'s exit code. Documented behaviour and the degenerate CardIDs are still **named** in the summary (1171-1175), so nothing is lost operationally. Noted only.
- **[NIT-6] `run_guard_self_test` cost** — the fixtures synthesise ~330k pixels in pure Python on every `--check` (~1-2 s). ⭐ **Worth it; flagged only so a future reader does not "optimise" the control out of the always-run tier.**

### ⭐ Rulings the dispatch asked for, stated plainly

1. **Is `--check` the right home for the positive control? — YES.** See §1(a). It is the difference between a control that runs and a control that exists.
2. **Would a dead check really be caught? — YES, verified by hand-derivation of all four isolating fixtures, not by reading the comment.** See §1(b). ⚠️ With one exception: the **fifth** check (`unreadable-artefact`) is not controlled in `--check` — WARN-2.
3. **Was byte-size / distinct-levels correctly rejected as a gate? — YES, and the reasoning is better than the alternative.** Both fail in **both** directions, which makes them worse than useless as gates: a 610 KB near-black noise frame passes any size floor while being 100% degenerate, and a 7 KB legitimate dark frame fails one. Distinct-levels is outright **inverted** on the hardest case (13 degenerate vs 3 legitimate). ⭐ The right call was to keep both as **printed diagnostics** — the operator still sees the 12,444-byte tell that caught this by eye in `TASK-833`, it just no longer decides anything. ⚖️ *`TASK-833` found the bug with byte size; the fix correctly declined to enshrine the symptom that found it.*
4. **Was the root cause structural? — CONFIRMED.** Between the generate call and `return 0` there were two statements and no check of any kind, with the overwrite unconditional and upstream of everything. This was never a threshold that was too loose. The premise verification (`SC-§40` cl. 3) was done first, reported even though it confirmed, and driven with a **scripted client** rather than live quota — ⭐ the right instrument: it isolates *our* defect from FLUX's seed behaviour, spends nothing, and is repeatable. ⭐ And its sharpest observation is correct and worth keeping: **the tool printed `(3,129 bytes)` immediately beside the word `SUCCESS`** — the evidence was already on screen and nothing was looking at it.

---

## 6. 🪤 FINDINGS FOR THE MANAGER — ruled on, not fixed

**(1) ⭐ THE FAMILY DEFECT IS REAL — I confirmed the diagnosis AT SOURCE, so the manager can board it without re-deriving.** The programmer stated these as unmeasured code reads (`SC-§40` cl. 1, correctly). **I opened the files. They hold:**

| tool | staged? | validated before SUCCESS? | evidence |
|---|---|---|---|
| `trellis_generate.py` | ❌ **NO** | ❌ **NO** | **541** `shutil.copyfile(source_glb, dest_glb)` overwrites `Cache/<Asset>/trellis_raw.glb` **in place**, no temp, no swap; **545-547** report `SUCCESS` with `output_glb_bytes` (byte size) as the **only** evidence. **BOTH halves of TASK-864's defect.** A GLB with zero meshes/vertices or a degenerate bbox passes. |
| `meshy_generate.py` | ✅ yes | ❌ **NO** | **362-367** stages `.part` → `tmp.replace(dest)`, so a *failed download* cannot destroy the prior artefact — **the destructive half is already absent**; but nothing inspects the file between the completed download and the swap, and **739** reports `SUCCESS: {dest} ({size:,} bytes)`. **HALF of it.** |
| `refine_trellis_glb.py` | — | — | ⛔ **NOT AUDITED by the programmer and NOT by me.** Neither of us is claiming it is clean; we are both claiming we did not look. |

⇒ ⚖️ **`TASK-864` was RIGHT not to touch them, and its absence from the diff is CORRECT** (spec item 4 anticipated exactly this). ⇒ **Recommend one family task**, because the fix is now a shipped, reviewed, controlled pattern that can be lifted verbatim: `stage → validate → swap`, a degeneracy assessment appropriate to the artefact type (for a GLB: mesh count > 0, vertex count > 0, non-degenerate bbox), a distinct exit code, and **its positive control wired into that tool's own `--check`**. `trellis_generate.py` is the priority — it is the only one that can still destroy a good artefact. Fold **WARN-5** (`model_info` timeout) into that row.
⭐ **And I endorse the sharpest suggestion in the handoff: `cardart_render.py` does not exist yet (`CONVENTIONS.md:83`) and should be BORN with this law rather than retrofitted.** The cheapest moment to close a family defect is before the next member is written — recommend the law text be quoted into that tool's spec row *now*, while it is still free.

**(2) The README earns a row — the absence is MEASURED, with a positive control (`SC-§40` cl. 1).** I grepped `README.md` for `concept_generate|--asset|--card-id|Inbox|Stage 0|refine_trellis_glb`: the instrument returned **11 hits** (so it could see), including `refine_trellis_glb` at 75/83/143 and the pipeline diagram at 9-13 — and **`concept_generate` returned ZERO**. Line 9 still reads `Inbox/<AssetName>.png (Jonathan drops the concept)` and the layout table (139) still calls Inbox drops `(human)`. ⇒ **The README genuinely documents no Stage 0 at all**, even though `concept_generate.py` is the tool that produces the `Inbox/<CardID>.png` the diagram starts from — and it now also owns a **new exit code and a new non-destructive `--force` contract** that appear nowhere an operator will look. ✅ **Ruling: it earns a small row.** Correctly out of `TASK-864`'s one-line fence — do not backfill it into this diff.

---

## 7. Notes for build-master (TASK-866)

1. ✅ **PASS report is `.claude/pipeline/qa/TASK-865.md` (this file).** It gates **TASK-864 only**.
2. ⛔ **NO COMPILE.** Python, not in the module, cannot break a build. Do not burn a `QUIET-MODULE` slot.
3. **Pathspec — exactly three paths:** `Tools/ArtPipeline/concept_generate.py` · `Tools/ArtPipeline/test_concept_guard.py` (**new file — do not omit it; the gate is worthless if only the guard lands**) · `Tools/ArtPipeline/README.md` (one token). ⛔ Nothing else in `Tools/ArtPipeline/` — `concept_prompts.json` and `pipeline_manifest.json` are the art-director's `TASK-833` edits and belong to that task's commit, not this rider.
4. ⭐⭐ **DO THIS FIRST, it is the one thing this gate could not do (§0):** run `uv run concept_generate.py --check` (expect **exit 0**) and `uv run test_concept_guard.py` (expect **exit 0**, `46/46`) from `Tools/ArtPipeline/`, and **record the two exit codes verbatim in your handoff**. Both are tokenless, offline, and cost no quota. ⛔ **If either is non-zero, STOP and route back — do not commit**, and note that `test_concept_guard.py` exiting **5** means VOID (the corpus was not found), **not** a failure of the guard. That ten seconds converts this gate's whole first column from DECLARED to EXECUTED.
5. Name the host commit that adopted the rider (a rider nobody records gets committed twice or never). **NO PUSH.**

---

## Board status lines (⛔ returned, NOT applied — I have no line-editing tool and will not whole-file `Write` a board other agents are editing)

`#### TASK-864` line 14758 →
```
- status: ✅ **qa-passed (2026-09-02, `qa/TASK-865.md` — 0 BLOCKER · 5 WARN · 6 NIT) — ready-for-integration (rider TASK-866)**
```

`#### TASK-865` line 14781 →
```
- status: ✅ **done (2026-09-02) — VERDICT: PASS. `qa/TASK-865.md`. 0 BLOCKER · 5 WARN · 6 NIT.** ⭐ **The control is the right shape: it lives in `--check` (runs every smoke test), asserts the EXACT SET of checks fired (a dead check cannot hide behind a live one — VERIFIED by hand-deriving all four isolating fixtures), controls BOTH directions (`very-dark-thin-lit-edge` mean **0.0232** re-derived independently and must be ACCEPTED), and is itself proven able to go RED (7/8 disagreement counts **re-derived and matched**).** ⛔ **`--check` and the 46 assertions were NOT EXECUTED by the gate — no shell was available (`TL-§5c`); every claim checkable statically was re-derived and ALL reproduced (single `os.replace` call site at 588 with its ONE caller at 908 inside `if not failed_checks:` · exit 6 absent from both siblings' {0,1,2,3,4,5,64} · corpus 25+24=**49** · **46** assertions). **TASK-866 must run `--check` before committing** (item 4 of the build notes).
  ⚠️ **WARNs (none blocking):** the stddev margin ledger is wrong in the source comment AND the handoff (**1.45×** above the worst degenerate, not "~3.5×" — thresholds still DEFENSIBLE, the black frame is rejected twice over) · **`unreadable-artefact` is the one check with no control inside `--check`** · `commit_staged` is unguarded (a Windows locked-file `PermissionError` escapes as a traceback) · the skip path returns **0** without validating a PRE-EXISTING artefact (a black frame from the old tool is already possible on disk) · `--probe`'s `model_info` has no timeout (pre-existing, out of fence).
  🪤 **FINDINGS CONFIRMED AT SOURCE for the manager: `trellis_generate.py:541`+`547` has BOTH halves** (in-place `shutil.copyfile`, SUCCESS on byte size alone) · **`meshy_generate.py:367`+`739` has HALF** (stages `.part`, never validates) · `refine_trellis_glb.py` **NOT audited by either of us** · **the README documents NO Stage 0 at all** (measured with a positive control: 11 hits returned, `concept_generate` **zero**) — **both earn rows.** ⭐ **Endorsed: `cardart_render.py` should be BORN with this law, not retrofitted.**
```
