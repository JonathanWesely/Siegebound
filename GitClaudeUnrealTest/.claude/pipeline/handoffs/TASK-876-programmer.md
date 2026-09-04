# TASK-876 — the sibling tools carry the same defect — handoff

Date: 2026-09-03 · Status set: **ready-for-qa** (gate **TASK-878**)
Law: **`SC-§39.1`** (incl. **cl. 6**) · `SC-§39` · `SC-§40` cl. 1 + cl. 3 + cl. 10 · `SHIP-§9` · `SC-§38`

> ⚠️ **This task was RESUMED after a previous agent was killed mid-work by a network
> outage.** §0 states exactly what I found on disk and what I did with it, because
> "the file compiles" was very nearly mistaken for "the file is finished" — and that
> distinction is this task's entire subject.

---

## 0. ⛔ WHAT THE DEAD AGENT LEFT, AND WHAT I DID WITH IT

Its last words were *"Trellis is fixed and proven by the same instrument. Now
`meshy_generate.py` — validation half only."* Both files compiled. **Only one of
those two claims survived contact.**

| file | found on disk | verdict | action |
|---|---|---|---|
| `trellis_generate.py` | **complete** — guard, stage/validate/swap, quarantine, fixtures, self-test, exit 6, call site all present | **genuinely done**, and good work | **KEPT**, re-verified from scratch, **+1 defect of my own finding fixed** (§4) |
| `meshy_generate.py` | **docstring, exit-code prose, 2 imports and 27 constants — and NOT ONE FUNCTION** | ⛔ **dangerous partial** | **completed** (§3) |

### ⛔ The meshy partial was worse than an untouched file, and this is the reusable lesson

The dead agent's docstring said, in the present tense:

```
    The staging was correct and is UNCHANGED. What was added is PHASE 2:
        PHASE 2  validate_glb_file(.part)         (NEW; dest still the old file)
        PHASE 3  commit_staged()                  os.replace, only if PHASE 2 passed
```

**`validate_glb_file` did not exist. `commit_staged` did not exist. No function was
added at all.** `struct` and `tempfile` were imported and unused; twenty-seven
constants were defined and referenced by nothing.

⇒ `py_compile` was **clean**, because prose is not a syntax error and an unused
constant is not a syntax error. A reader grepping the docstring — or trusting the
dispatch note that said "it compiles" — would have concluded meshy was fixed.
**An untouched file is honestly broken; this one documented a guard it did not have.**

⭐ **That is `SC-§39.1` one level further out than the law currently reaches: the tool
lied about its artefact, the guard could have been dead, the control could have been
partly dead — and here the *source file itself* claimed a capability it did not
possess, while passing the only automated check anyone had run on it.** Recommend a
clause: **a compile is a syntax measurement, never a completeness measurement.**

⛔ I did **not** take the dead agent's numbers on trust either. Its meshy comment
claimed the corpus held **"34"** of this script's own outputs; I swept and measured
**33** (§5). Its docstring claimed a repro of *"a 428-byte 0-vertex GLB replaced a
20,380,016-byte mesh"* — I cannot reproduce those figures and mine differ (132 bytes
/ 23,434,424 bytes, §1). **Both were rewritten to what I measured** (`SC-§40` cl. 1;
this is the `qa/TASK-865.md` WARN-1 shape — a wrong number in a source comment is
what the next editor re-tunes from).

---

## 1. ⭐ THE BEHAVIOURAL REPRO — what was still owed, and what I watched happen

Nobody had yet driven either tool and watched it destroy a good artefact; the premise
was measured at source only. I drove **both unmodified tools** with a scripted client
into a scratch Cache pre-seeded with a **copy** of the real 23,434,424-byte
`Cache/Knight/trellis_raw.glb`. **Zero HF quota, zero Meshy credits, no network.**

The degenerate payload is a **132-byte, syntactically valid GLB 2.0 container with an
empty `meshes` array** — synthesised, never a roll. This is the realistic failure: the
container is well-formed, so every "did the file arrive" check passes, and there is no
geometry in it.

```
TRELLIS [PRISTINE]  SUCCESS: ...trellis_raw.glb (132 bytes).      rc=0
                    victim 23,434,424 -> 132 bytes   preserved: False   >>> DEFECT REPRODUCED
MESHY   [PRISTINE]  SUCCESS: ...meshy_raw.glb (132 bytes).        rc=0
                    victim 23,434,424 -> 132 bytes   preserved: False   >>> DEFECT REPRODUCED

TRELLIS [FIXED]     DEGENERATE ARTEFACT ... Failed checks: no-mesh.     rc=6
                    victim 23,434,424 bytes UNCHANGED  preserved: True  quarantined
MESHY   [FIXED]     DEGENERATE ARTEFACT ... Failed checks: no-mesh.     rc=6
                    victim 23,434,424 bytes UNCHANGED  preserved: True  quarantined
```

⭐ **The `concept_generate.py` signature, reproduced exactly — and 28× larger.** That
defect printed `SUCCESS -> Witch.png (3,129 bytes)` while destroying 837,664 bytes.
This one prints `SUCCESS: ...trellis_raw.glb (132 bytes)` while destroying
**23,434,424** bytes of real GPU quota. **The number that would have exposed it was on
screen, beside the word `SUCCESS`, in both tools.**

### ⛔⛔ THE FINDING THAT SHARPENS THE BOARD'S OWN SCOPING — meshy's `.part` staging did NOT save it

The board says meshy's **destructive half is "genuinely already absent"** because it
stages `.part` → `tmp.replace(dest)`. **That scoping is CORRECT and I obeyed it — I did
not re-plumb the staging.** But the reason it is correct is not the reason stated, and
QA should have the precise version:

- `.part` staging protects against a **FAILED transfer**. ✅ True, and it works.
- The repro above ran through meshy's **own real `download_file`**, `.part` staging and
  all — only the CDN socket was scripted. **The transfer SUCCEEDED.** `tmp.replace(dest)`
  then destroyed the 23 MB artefact exactly as completely as trellis's bare `copyfile`.

⇒ ⚖️ ***Staging protects against a failed transfer. Only validation protects against a
SUCCESSFUL transfer of a degenerate payload.*** meshy was missing **one** of the two
halves but was **fully as destructive** as trellis in practice. The board's instruction
(fix validation only, do not re-plumb staging) produces the right diff; the phrase
"the destructive half is absent" understates the harm and should not be carried forward
as a severity ranking.

Repro harness: `…/scratchpad/t876/repro.py` (throwaway; not committed — it imports the
pristine `HEAD` copies of both tools alongside the working-tree ones).

---

## 2. Files touched — TWO, and no more

| File | Change |
|---|---|
| `Tools/ArtPipeline/meshy_generate.py` | **The task's real work**: the whole validation half — guard, PHASE 2, single-call-site swap, quarantine, exit 6, positive control wired into `--check` |
| `Tools/ArtPipeline/trellis_generate.py` | Inherited complete; **verified**, plus **one uncontrolled branch I found and closed** (§4) + two stale doc numbers |

⛔ **Fence held.** `refine_trellis_glb.py` **audited, not modified** (mtime still
2026-07-28 — §6). No `concept_generate.py` (mtime still 09-02 22:56 — its 653-line diff
in `git status` is **TASK-864's own uncommitted work**, present before I started). No
README (that is `TASK-877`). No `Source/`, no `Content/`, no `cardart_render.py`, no
compile, no editor, no MCP, no Git.

---

## 3. The meshy fix — the validation half only

**PHASE 1 (staging) is UNCHANGED**, per the board. What I added:

```
PHASE 1  download_file()      stream -> <dest>.part          (PRE-EXISTING; dest untouched)
PHASE 2  validate_glb_file()  re-read the .part FROM DISK    (NEW; dest STILL the old artefact)
PHASE 3  commit_staged()      os.replace  <- reached ONLY when PHASE 2 raised nothing
```

`download_file` no longer performs the swap and **no longer returns a byte size** — it
returns `(staged_path, validated_stats)`. Returning a size was the entire evidence base
of the defect, and moving the swap to the caller is what lets it exist at one guarded
site. A rejected GLB is **quarantined** to `Cache/<CardID>/_rejected/<name>.<utc>.glb`
(collision-suffixed), **never deleted** — it already cost Meshy credits, so deleting it
deletes the evidence of what those credits bought. `Cache/*` is gitignored
(`.gitignore:27`, verified with `git check-ignore`), so quarantined artefacts never
enter git.

**Retry policy, from the failure's shape rather than by default.** Only
`unreadable-artefact` and `truncated-container` are retried: they describe the
**transport**, and a CDN can serve the same signed URL correctly on a second attempt.
`no-mesh` / `no-geometry` / `degenerate-bounds` are **not** retried — the same URL
returns the same bytes, so a retry spends time for a guaranteed repeat.

### ⭐ The `--check` placement that is load-bearing on THIS machine

meshy's `--check` **requires an API key and returns 2 without one**. Had the control
gone after `require_api_key()`, it would have been dead on any machine without a
credential. It is **STEP 1, ahead of the key check and ahead of all network I/O**.
Measured live:

```
$ python meshy_generate.py --check
[meshy] --check: degeneracy guard control PASSED - 11 fixtures (8 rejected, 3 accepted),
        all 5 checks fire in isolation, and the control itself goes RED when the guard
        is broken (7 disagreements with a never-rejects guard, 10 with an always-rejects one)
[meshy][ERROR] API unreachable: URLError: <urlopen error [SSL: CERTIFICATE_VERIFY_FAILED] ...>
exit 1
```

⇒ **the guard's control ran and passed even though the network leg failed.** That is
`SC-§39.1` cl. 6's always-run tier doing its job.

---

## 4. ⛔ THE DEFECT I FOUND IN THE INHERITED TRELLIS WORK — cl. 6, one level down

`TASK-864`'s gap was that `unreadable-artefact` had **no fixture inside `--check`**.
The dead agent's trellis guard **fixed that** — every fixture is a real file on disk.
**But `validate_glb_file` raises `CHECK_UNREADABLE` from TWO distinct sites**, and only
one was controlled:

- the **bytes** branch (`measure_glb` sets `unreadable_reason`) — controlled by
  `empty-file` / `not-a-glb` / `json-chunk-garbage` ✅
- the **`except OSError`** branch (file missing/unreadable) — ⛔ **reached by NO fixture**,
  because every fixture wrote a file before validating it.

**Measured, not reasoned:** I deleted that `except OSError` clause and re-ran the
control. It did not stay green — but it **crashed with an uncaught `FileNotFoundError`
naming a temp path**, rather than reporting. A control that dies is a control that will
be "fixed" by whoever is unlucky enough to hit it.

**Both fixed, in both tools:**
1. a **`missing-file` fixture** (payload `None` ⇒ the file is deliberately never
   created) now exercises the `OSError` branch — **11 fixtures**, up from 10;
2. `_run_fixture_suite` catches an unexpected exception and records it as a
   **disagreement naming the fixture**, so the guard must *reject*, never *explode*.

⚠️ **Note for QA on a number that looks wrong and is not:** the never-rejects injection
reports **7** disagreements against **8** rejecting fixtures. That is correct and
informative — `missing-file` is rejected by the `OSError` branch **before**
`assess_degeneracy` is ever called, so neutering `assess_degeneracy` cannot affect it.
The meta-control is measuring exactly what it should.

Also corrected: `--check`'s `--help` in trellis still advertised **"10 synthetic GLB
fixtures"** after my 11th. Both tools' help now describes the *property* and lets the
runtime print the live count, so the number cannot rot again (`SC-§38`).

---

## 5. Control coverage — PER CHECK, BOTH DIRECTIONS, proven by mutation

⛔ **Not asserted from the fixture table — measured by neutering each check in turn and
confirming the control goes RED.** `12/12 RED, 0 GREEN`:

| check | fires ALONE in fixture | mutation → trellis | mutation → meshy |
|---|---|---|---|
| `unreadable-artefact` (OSError branch) | `missing-file` | **RED** | **RED** |
| `unreadable-artefact` (bytes branch) | `empty-file`, `not-a-glb`, `json-chunk-garbage` | **RED** | **RED** |
| `truncated-container` | `truncated-container` | **RED** | **RED** |
| `no-mesh` | `no-mesh` | **RED** | **RED** |
| `no-geometry` | `single-triangle` | **RED** | **RED** |
| `degenerate-bounds` | `flat-plane` | **RED** | **RED** |

**BOTH DIRECTIONS.** Three fixtures must be **ACCEPTED**, so a reject-everything guard
fails just as loudly:
- **`minimal-tetrahedron` has FEWER vertices (4) than the REJECTED `flat-plane` (6)** —
  so nothing keyed on size or count *alone* can pass one and fail the other;
- **`thin-but-legitimate`** (aspect **0.010**) is **26.5× thinner than the thinnest real
  asset in the corpus** (`Footman_gameready`, aspect **0.2652**) and must still pass —
  this is what forces the bounds check to key on **collapsed**, not on **thin**;
- `dense-healthy`.

Plus the control's **own** control (both tools): a never-rejects guard → 7
disagreements, an always-rejects guard → 10, restored → 0, and a broken guard makes
`--check` exit **1**, blocking generation.

### The degeneracy check is appropriate to a MESH, not copied from the image tool

A GLB is not a PNG — "mean pixel value" is meaningless here. The checks are a
**container parse** (magic/version/declared length/JSON chunk), **mesh presence**
(`primitives > 0`), **geometry presence** (vertices ≥ 4 **and** triangles ≥ 4) and
**non-degenerate bounds** (min/max extent aspect ≥ 1e-4). They detect the failure I can
actually produce: the 132-byte well-formed empty container of §1.

**Thresholds are DERIVED, not picked** (`SC-§40` cl. 10). `MIN_MESH_VERTICES` /
`MIN_MESH_TRIANGLES` = **the tetrahedron**, the smallest closed solid in 3-space — a
floor of *"is it a solid at all"*, never a quality bar (quality is Stage 2's tri
budget). `FLAT_ASPECT_FLOOR` = a relative epsilon; a collapsed mesh has aspect
**exactly 0.0**, and 1e-4 sits ~3 orders of magnitude above float32 noise at unit
scale. A derived floor cannot go stale when the data moves.

**My own corpus sweep** (not the dead agent's — re-measured): **126 real GLBs** under
`Cache/`, **126/126 accepted by BOTH guards, 0 rejected**. Worst real values, i.e. the
headroom that matters:

| | worst real asset | floor | headroom |
|---|---|---|---|
| vertices | **11,820** (`rerig_t234/Sapper_gameready_nobomb`) | 4 | **2,955×** |
| triangles | **9,461** (`attempt1/Sapper_Attack`) | 4 | **2,365×** |
| aspect | **0.2652** (`meshy_anim/Footman_gameready`) | 1e-4 | **2,652×** |

⛔ **BYTE SIZE IS DELIBERATELY NOT A GATE**, and the source says so in both tools so
nobody re-adds it: it is the symptom that *found* this bug, not a test for it — a
degenerate noise frame measured 610 KB and a legitimate dark frame 7 KB in `TASK-864`,
and here a **132-byte** degenerate GLB and a **22.5 MB** healthy one differ by size in
the *convenient* direction only by luck. Size stays a printed **diagnostic** and
decides nothing.

### Accept-direction regression, end-to-end (not just fixture-level)

A **healthy 22,559,448-byte** GLB driven through both fixed tools: **rc=0**, the
artefact **replaced correctly** (sha matches the new roll), **no stray `.tmp`/`.part`
left behind**. And the SUCCESS line now reads:

```
SUCCESS: ...trellis_raw.glb - validated: 22,559,448 bytes, 1 mesh(es)/1 primitive(s),
         460,425 verts, 475,801 tris, bounds 0.762x0.997x0.786, aspect 0.7642, 2 image(s)
```

⇒ **the measurement beside the word `SUCCESS` is now the measurement that was CHECKED**,
not decoration.

---

## 6. ⛔ `refine_trellis_glb.py` — THE AUDIT. Verdict: **DEFECTIVE, BOTH HALVES — REPORTED, NOT FIXED (scope valve invoked)**

It was an **unknown, not a pass**. I opened it. **It carries both halves, and its
validation half is WEAKER than either sibling's.**

| half | verdict | evidence |
|---|---|---|
| **destructive** | ⛔ **PRESENT** | `bpy.ops.export_scene.fbx(filepath=str(out))` (**:1302**) writes **directly** to `Content/RawAssets/<CardID>.fbx` — no temp, no staging, no swap. `save_png` (**:721-726**) sets `image.filepath_raw = str(out)` and calls `image.save()` — likewise in place, onto `Content/RawAssets/Textures/<CardID>/T_<CardID>_D\|_N\|_ORM.png`. |
| **validation** | ⛔ **ABSENT — and weaker than the siblings had** | After the FBX export it logs `EXPORT: {out}` and returns; after `image.save()` it logs `wrote {out}`. **Positive control on the instrument:** grepping the whole file for `.exists()\|.is_file()\|st_size\|stat()\|getsize` returns exactly **two** hits — **:210** (`manifest not found`) and **:1433** (`input mesh not found`) — **both INPUT-side. There is not one post-write check of any output.** Success = *"the bpy operator did not raise"*, i.e. weaker than trellis's byte-size check, which at least read the file back. |

⚠️ **`OutputGuard.check()` (:159-180) is easy to mistake for a guard and is NOT one** —
it is *write confinement* (path allow-listing + a CardArt lane block). It validates
**where** you may write, never **what** you wrote.

⚖️ **Severity, stated honestly in both directions:** its targets live under
`Content/RawAssets/**`, which is **git-tracked** — so a destroyed FBX is recoverable
from git, a mitigation neither sibling has (they write to gitignored `Cache/`).
**But** work done between commits is still lost, and — worse than the siblings — the
operator gets **no signal at all**, because nothing looks at the output.

⛔ **SCOPE VALVE INVOKED (row item 2a), deliberately.** I did **not** fix it:
1. it runs **inside Blender's bundled Python** (`bpy`/`bmesh`, `--background`), a
   different runtime from anything else in this task — its guard **and its positive
   control** would both have to be authored and proven headless in Blender;
2. it has **no offline `--check` tier at all** to hang a control on — one would have to
   be built;
3. this fence forbids the editor/MCP, and a Blender launch is a separate lane.

⇒ **A half-finished third tool is worse than a named one.** Recommend its own row:
stage the FBX + the three PNGs to temp, validate (re-import the FBX / re-read the PNGs),
swap, quarantine rejects, exit 6, control in a new offline `--check`.

---

## 7. Single-call-site proof + exit-code collision check

**Every destination-write primitive in both tools, enumerated:**

| tool | site | what it writes |
|---|---|---|
| trellis | **:660 `os.replace(staged, dest)`** | ⭐ **`commit_staged` — the ONLY write to `trellis_raw.glb`** |
| trellis | :690 `os.replace(staged, target)` | `quarantine_staged` → `_rejected/`, **never `dest`** |
| trellis | :646 `shutil.copyfile(source, staged)` | the staged temp |
| trellis | :236 / :919 `dest.write_text` | `api_schema.json` / `state.json` (different `dest`, JSON) |
| meshy | **:689 `os.replace(staged, dest)`** | ⭐ **`commit_staged` — the ONLY write to the output GLB** |
| meshy | :720 `os.replace(staged, target)` | `quarantine_staged` → `_rejected/` |
| meshy | :755 `open(staged,"wb")` | the `.part` |
| meshy | :980 / :1001 / :1008 | state JSON only |

**`commit_staged` has exactly ONE caller in each tool** — trellis **:1173**, meshy
**:1367** — and in both it sits **after** the `except DegenerateArtefactError:` block
that `return`s, so it is reachable only when PHASE 2 raised nothing. Both also route
through `_require_inside_cache()`, so neither can write outside `Cache/`.

**Exit code 6 = DEGENERATE ARTEFACT — collides with neither sibling's family:**

| tool | codes |
|---|---|
| `trellis_generate.py` | 0 1 2 3 4 5 **6** 64 |
| `meshy_generate.py` | 0 1 2 3 4 5 **6** 64 |
| `concept_generate.py` | 0 1 2 3 4 5 **6** 64 |
| `refine_trellis_glb.py` | 0 2 (no degeneracy contract at all) |

`6` is **never a literal** anywhere — it is `EXIT_DEGENERATE` in all three tools, same
value, same meaning. Verified: `grep -nE "return\s+6\b|sys\.exit\(6\)" *.py` → **none**.
Documented in each module docstring's exit table and in `--help`.

---

## 8. Evidence — all green, all offline, zero quota

```
py_compile trellis_generate.py meshy_generate.py    -> OK
trellis --help / meshy --help                       -> exit 0
meshy --check                                       -> guard control PASSED, then exit 1 (TLS, §9)
trellis run_guard_self_test()                       -> exit 0, 11 fixtures, 5 checks, all isolated
meshy   run_guard_self_test()                       -> exit 0, 11 fixtures, 5 checks, all isolated
per-check mutation sweep (12 mutations, both tools) -> 12 RED, 0 GREEN
corpus sweep, 126 real GLBs, both guards            -> 126/126 ACCEPTED
behavioural repro, 4 runs                           -> 2 REPRODUCED (pristine), 2 NOT (fixed)
healthy-artefact regression, both tools             -> rc 0, replaced, no stray temp files
```

**The real corpus is untouched** — `Cache/Knight/trellis_raw.glb` still 23,434,424
bytes at its original 2026-07-16 mtime, `Cache/Ogre/trellis_raw.glb` still 22,559,448
at 2026-07-14. Every run redirected `INBOX_DIR`/`CACHE_DIR` into scratch. No `.part`,
no `.tmp-*`, no `_rejected/` exists anywhere in the real `Cache/`.

---

## 9. 🔍 What QA should scrutinise

1. ⭐ **The order, read as control flow — not as prose.** `grep -n "os.replace"` gives
   two sites per tool; confirm the second targets `_rejected/`, then confirm
   `commit_staged`'s single caller sits **below** a `return`ing
   `except DegenerateArtefactError`. §7. The repro proves it behaviourally on a
   **passing** roll too (§5), where both orderings end with the new file in place.
2. ⛔ **The meshy `--check` ordering is load-bearing, not stylistic.** If the guard
   control is ever moved below `require_api_key()`, it goes dead on every machine
   without a Meshy key — including this one. §3.
3. **The "7 vs 8 disagreements" in the meta-control is CORRECT** — see the note in §4
   before flagging it.
4. **`refine_trellis_glb.py` is deliberately unmodified.** Its verdict is
   **defective, both halves** (§6), not clean, and the scope valve was invoked on
   purpose. Please confirm you agree the valve was the right call rather than treating
   the absent diff as an omission.
5. **Thresholds** are the one judgement call. They are derived (tetrahedron / relative
   epsilon) rather than corpus-fitted, with ~2,400–2,950× headroom measured over 126
   real assets. §5.
6. **Duplication, declared:** the guard is **duplicated** between the two tools rather
   than shared. Deliberate — creating `Tools/ArtPipeline/artefact_guard.py` is a new
   file outside this row's named fence. Both copies carry a comment saying so. ⇒
   **recommend a row to extract it**, before a third caller makes it three copies.
7. **Secrets:** unchanged. Every new print goes through `say`/`warn`/`fail`, all of
   which redact. The failure report prints **mesh statistics and file paths only** —
   never a URL query string (`_strip_query`), never a token, never a prompt.

---

## 10. 🪤 FINDINGS for the manager — reported, NOT fixed

1. ⛔⛔ **`refine_trellis_glb.py` needs its own row.** Both halves, validation weaker
   than either sibling's, writing into **git-tracked** `Content/RawAssets/**`. Full
   measurement in §6. **This is a measured verdict, not a code-read hunch** — I opened
   it and ran a positive-control grep on the instrument.
2. ⛔ **Recommend a clause under `SC-§39.1`: a compile is a SYNTAX measurement, never a
   COMPLETENESS measurement.** §0's partial passed `py_compile` while documenting, in
   the present tense, two functions that did not exist. The existing law makes an agent
   distrust a *tool's* success return; this case is a **source file's own docstring**
   asserting a capability it lacked, and the only automated check anyone had run on it
   agreed. Cheap detector, used here: **grep the docstring's named symbols against
   `def`** — it takes one command and it is what caught this.
3. ⚠️ **Environment correction for the board:** the dispatch says *"`MESHY_API_KEY` is
   unset"*. True for that **alias** — but the **canonical `MESHY_TOKEN` IS set** in this
   environment and `--check` resolved a key from it. I spent **no credits** (the guard
   control is offline; the only calls attempted were free read endpoints, and they died
   at TLS). Anyone boarding meshy work on "no key available" should re-measure.
4. 🧑 **`api.meshy.ai` is Norton-TLS-intercepted, same as `router.huggingface.co`.**
   Measured: `CERTIFICATE_VERIFY_FAILED: Basic Constraints of CA cert not marked
   critical`. The tool already prints the right guidance. Not fixed, not worked around,
   verification never disabled — and it was **not in this task's path** (no live call
   was needed for any result above).
5. **The guard should be extracted to one shared module** once a third caller appears
   (`cardart_render.py`, `TASK-385`) — see §9(6).

---

## 11. Assets referenced

**None.** No `/Game/...` asset, no mesh, no material, no texture. Tooling only. The 126
GLBs under `Tools/ArtPipeline/Cache/` are **read** as the accept-direction corpus and
were never written; two of them were **copied** into scratch directories as repro
victims.
