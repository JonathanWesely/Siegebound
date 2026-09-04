# TASK-891 — the artefact census: `SM_Witch`'s source GLB + the cached fleet — handoff

Date: 2026-09-03 · Status set: **ready-for-qa**
Law: **`SC-§39.1`** · `SC-§40` cl. 1 + cl. 9 + cl. 10 · **`TL-§5c` cl. 5** · `SHIP-§9`
Gates: **`TASK-835` item (2c)**

---

## ⭐ VERDICT — **ACCEPTED**, and here are the five check names, individually

> ⛔ **This is a MEASUREMENT, not the word "fine"** (`SC-§40` cl. 1). The row asked for
> the five check names with their own verdicts; that table is §2, and the instrument
> that produced it is controlled in both directions in §3.

`Tools/ArtPipeline/Cache/Witch/trellis_raw.glb` — **`validate_glb_file` returned stats
and raised nothing. 0 of 5 checks fired. All 5 were EVALUATED and all 5 were proven
LIVE over her own statistics.**

⇒ **`TASK-835` is UNBLOCKED on this row's account.** The Witch's *source GLB* is a real,
non-degenerate mesh; her assets may enter a commit with evidence rather than hope.
⚠️ **Read §6 before treating that as blanket clearance** — this row covers the **GLB**,
not the Stage-2 FBX/PNGs, and that boundary is deliberate.

---

## 0. ⛔ `TL-§5c` cl. 5 — EXECUTED vs DECLARED, stated BEFORE the verdict

### ✅ EXECUTED — by me, on this machine, offline, this session

| # | thing | result |
|---|---|---|
| 1 | `validate_glb_file(Cache/Witch/trellis_raw.glb)` | **ACCEPTED** (§2) |
| 2 | The 5 checks read individually off the shipped `assess_degeneracy` | **0 fired / 5 evaluated** (§2) |
| 3 | 5 stat-level mutations of **her own** stats, one field each | **5/5 fire ⇒ 5/5 LIVE** (§2.2) |
| 4 | The shipped `run_guard_self_test()`, verbatim | **rc=0**, the exact 11·8·3·5·7·10 line (§3.1) |
| 5 | All 11 fixtures driven **individually** through the same call I ran on her | **11/11 agree** (§3.2) |
| 6 | The accept-side pair (4-vert tetra ACCEPT vs 6-vert plane REJECT) | **reproduced** (§3.3) |
| 7 | A hand-built empty-container GLB | **REJECTED `no-mesh`** (§3.4) |
| 8 | Meta-control: never-rejects / always-rejects / restored | **7 / 10 / 0** (§3.5) |
| 9 | **The fleet sweep — `Cache/**/*.glb`** | **126 swept · 126 ACCEPTED · 0 REJECTED** (§4) |
| 10 | Corpus name census, re-measured (`SC-§40` cl. 9) | 126 · 19 trellis · 33 meshy · **34** for `meshy_r*` (§4.1) |
| 11 | `state.json` schema census across the whole cache | **19 pre-fix · 0 post-fix** — §5, **this is the new finding** |
| 12 | Pre/post byte-identity of all 126 GLBs + all 14 `*.py` | **drift = 0** (§7) |

### ⛔ DECLARED — NOT executed by me, and I will not imply otherwise

- **No network call of any kind was made, and none was needed.** `validate_glb_file`,
  `measure_glb`, `assess_degeneracy`, `describe_glb` and every fixture are pure stdlib
  (`struct`/`json`/`pathlib`). ⇒ **zero HF quota, zero Meshy credits, no token read, no
  TLS path touched** — so the Norton interception of `router.huggingface.co` /
  `api.meshy.ai` and the `MESHY_TOKEN`-vs-`MESHY_API_KEY` alias question **were both
  irrelevant to every number above**. I never reached for the network; the dispatch's
  prediction that I would not need it is **confirmed by execution**.
- **`meshy_generate.py`'s copy of the guard was NOT run here.** `TASK-876` reports the
  two copies as behaviourally identical; **I did not re-verify that**, because the
  artefact under test is a trellis output and the trellis copy is the one that would
  have judged it. That verification remains **`TASK-890`'s** (`qa/TASK-878.md` §6).
- **Her FBX and PNGs under `Content/RawAssets/**` were NOT validated** — out of fence
  by the row's own item (3). See §6.
- I did **not** run `--check` as a CLI (it can reach the network after the control
  passes). I called the **same control function** `--check` calls, in-process. §3.1.

---

## 1. What I ran — the exact command

```
cd Tools/ArtPipeline
PYTHONDONTWRITEBYTECODE=1 TEMP=<scratch> TMP=<scratch> python <scratch>/t891/census.py
```

`census.py` does `sys.path.insert(0, Tools/ArtPipeline)` then `import trellis_generate as T`
and calls **`T.validate_glb_file(...)`**. The module has **no side effects at import** —
every `mkdir` in the file sits inside a function body (`:235`, `:643`, `:689`, `:917`,
`:1006`, `:1007`), and `main()` is behind `if __name__ == "__main__":` (`:1333`).
`PYTHONDONTWRITEBYTECODE=1` was set so not even a `.pyc` was written into the tree.
Harness lives in the scratchpad, is **not** committed, and **edits nothing**.

---

## 2. ⭐ THE WITCH — the full `describe_glb` line and the five checks

```
CALL: trellis_generate.validate_glb_file(
          Path(r'…\Tools\ArtPipeline\Cache\Witch\trellis_raw.glb'))

VERDICT: ACCEPTED (returned stats, raised nothing)

describe_glb ->
19,050,252 bytes, 1 mesh(es)/1 primitive(s), 379,909 verts, 480,750 tris,
bounds 0.656x1.002x0.653, aspect 0.6518, 2 image(s)
```

Full stats dict as returned:

| field | value | field | value |
|---|---|---|---|
| `bytes` | **19,050,252** | `declared_bytes` | **19,050,252** |
| `version` | 2 | `meshes` | 1 |
| `primitives` | 1 | `vertices` | **379,909** |
| `triangles` | **480,750** | `images` | 2 |
| `materials` | 1 | `bounds_known` | **True** |
| `extent` | `[0.6558980941772461, 1.0016180276870728, 0.6528786420822144]` | `aspect` | **0.6518239728470501** |
| `max_extent` | 1.0016180276870728 | `min_extent` | 0.6528786420822144 |
| `unreadable_reason` | **None** | `truncated_reason` | **None** |

### 2.1 The five checks, named, each with its own verdict

`assess_degeneracy(witch_stats)` → **`[]`** — 0 checks fired.

| # | check name | evaluated? | verdict | the value it judged |
|---|---|---|---|---|
| 1 | **`unreadable-artefact`** | ✅ yes | ✅ **PASS** | `unreadable_reason is None` — the file was read from disk and the container parsed |
| 2 | **`truncated-container`** | ✅ yes | ✅ **PASS** | header declares **19,050,252**, file **is** 19,050,252 — **equal**; no chunk short |
| 3 | **`no-mesh`** | ✅ yes | ✅ **PASS** | `primitives = 1` (> 0) |
| 4 | **`no-geometry`** | ✅ yes | ✅ **PASS** | `379,909 verts ≥ 4` **and** `480,750 tris ≥ 4` |
| 5 | **`degenerate-bounds`** | ✅ yes | ✅ **PASS** | `bounds_known=True`, `max_extent = 1.0016 > 0`, `aspect 0.6518 ≥ 1e-4` |

⛔ **"Evaluated" is not a courtesy word here.** `assess_degeneracy` carries **two early
returns** — `unreadable` short-circuits everything, and `no-mesh` short-circuits
geometry+bounds. A degenerate artefact can therefore have three checks *never reach their
domain*. **Hers reached all five**, and §2.2 proves it rather than asserting it.

### 2.2 ⭐ Proving the five were LIVE over HER stats, not merely believed to be

A check that is skipped and a check that passes look identical in a `[]` return. So I
mutated **one field of her own stats dict at a time** and fed it back to the **shipped**
`assess_degeneracy`. If a check is dead over her data, its mutation cannot make it fire.

| check | mutation applied to HER stats | fired? | ⇒ |
|---|---|---|---|
| `unreadable-artefact` | `unreadable_reason = "MUTANT"` | **YES** | **LIVE** |
| `truncated-container` | `truncated_reason = "MUTANT"` | **YES** | **LIVE** |
| `no-mesh` | `primitives = 0` | **YES** | **LIVE** |
| `no-geometry` | `vertices = 3` | **YES** | **LIVE** |
| `no-geometry` (2nd input) | `triangles = 3` | **YES** | **LIVE** |
| `degenerate-bounds` | `aspect = 0.0` | **YES** | **LIVE** |
| `degenerate-bounds` (2nd input) | `max_extent = 0.0` | **YES** | **LIVE** |

**5/5 checks live, via 7 mutations covering both inputs of the two compound checks.**

### 2.3 The measurement read AGAINST AN EXPECTATION (`SC-§39.1`)

A number without an expectation is decoration:

| | her value | floor | headroom |
|---|---|---|---|
| vertices | **379,909** | 4 | **94,977×** |
| triangles | **480,750** | 4 | **120,188×** |
| aspect | **0.6518** | 1e-4 | **6,518×** |

⭐ And the reading that actually matters, because it is the one a floor cannot give you:
**her aspect 0.6518 is not merely above the floor, it is well inside the real corpus's
own range** (fleet worst = **0.2652**). She is not a marginal pass hugging a threshold —
she sits in the middle of the distribution of assets already shipped in this game.
**480,750 triangles against a `decimation_target` of 500,000** is likewise the shape a
successful TRELLIS extract should have. ⇒ *the stats are consistent with a real
character, not merely with "not provably broken".*

---

## 3. ⛔ CONTROLLING THE INSTRUMENT IN BOTH DIRECTIONS — because a validator that accepts everything looks identical to one that works

### 3.1 The shipped control, run verbatim

```
[trellis] --check: degeneracy guard control PASSED - 11 fixtures (8 rejected, 3 accepted),
all 5 checks fire in isolation, and the control itself goes RED when the guard is broken
(7 disagreements with a never-rejects guard, 10 with an always-rejects one).
run_guard_self_test() returned 0
```
⛔ **11 · 8 · 3 · 5 · 7 · 10** — the exact line and the exact numbers `qa/TASK-878.md` §6
sets as the pass criterion. ⚠️ **That does NOT discharge `TASK-890`'s duty:** QA owes the
*CLI* `--check` on **both** tools at the commit; I ran the trellis control **in-process**
and did not touch meshy at all.

### 3.2 All 11 fixtures, driven individually through **the same call I made on the Witch**

| fixture | expected | `validate_glb_file` → | agree |
|---|---|---|---|
| `missing-file` | `unreadable-artefact` | REJECT `unreadable-artefact` | ✅ |
| `empty-file` | `unreadable-artefact` | REJECT `unreadable-artefact` | ✅ |
| `not-a-glb` | `unreadable-artefact` | REJECT `unreadable-artefact` | ✅ |
| `json-chunk-garbage` | `unreadable-artefact` | REJECT `unreadable-artefact` | ✅ |
| `truncated-container` | `truncated-container` | REJECT `truncated-container` | ✅ |
| `no-mesh` | `no-mesh` | REJECT `no-mesh` | ✅ |
| `single-triangle` | `no-geometry` | REJECT `no-geometry` | ✅ |
| `flat-plane` | `degenerate-bounds` | REJECT `degenerate-bounds` | ✅ |
| `minimal-tetrahedron` | ACCEPT | **ACCEPT** | ✅ |
| `thin-but-legitimate` | ACCEPT | **ACCEPT** | ✅ |
| `dense-healthy` | ACCEPT | **ACCEPT** | ✅ |

**11/11.** ⇒ the function that said ACCEPT to the Witch says **REJECT** to eight
degenerate artefacts, each by the correct name. **My ACCEPTED verdict means something.**

### 3.3 ⭐ The accept side is the one that matters — reproduced with the numbers

```
minimal-tetrahedron    648 B  ACCEPT  -> 4 verts, 4 tris, bounds 1.000x1.000x1.000, aspect 1.0
flat-plane             672 B  REJECT ['degenerate-bounds']
                              -> 6 verts, 4 tris, bounds 1.000x1.000x0.000, aspect 0.0
thin-but-legitimate 31,316 B  ACCEPT  -> 2,048 verts, 1,024 tris, aspect 0.01
```
⛔ **The 4-vertex file is ACCEPTED while the 6-vertex file is REJECTED, and the accepted
one is the SMALLER of the two.** ⇒ **nothing keyed on byte size, vertex count or
triangle count alone can produce this pair.** A "validator" that accepted everything
fails on `flat-plane`; one that rejected everything fails on `minimal-tetrahedron`.
And `thin-but-legitimate` at aspect **0.010** — **26.5× thinner than the thinnest real
asset in the corpus (0.2652)** — must still pass, which is what forces the bounds check
to key on *collapsed* rather than on *thin*.

### 3.4 The degenerate shape this whole lane exists for

A hand-built, syntactically valid GLB 2.0 container with an **empty `meshes` array**:

```
72 bytes -> REJECT ['no-mesh']
   describe: 72 bytes, 0 mesh(es)/0 primitive(s), 0 verts, 0 tris, bounds unknown, aspect None
```
This is the payload class that the **pre-fix** tool reported as `SUCCESS` — well-formed,
so every "did the file arrive" check passes, and there is no geometry in it.

### 3.5 The control's own control

| injection | disagreements |
|---|---|
| never-rejects (`lambda: []`) | **7** |
| always-rejects (`lambda: list(ALL_GLB_CHECKS)`) | **10** |
| restored | **0** |

The **7-not-8** is correct and I verified why rather than repeating it: `missing-file` is
rejected by `validate_glb_file`'s **`except OSError`** branch *before* `assess_degeneracy`
is ever called, so neutering `assess_degeneracy` cannot affect it.

---

## 4. THE FLEET — the DECLARED sweep converted to an EXECUTED one

```
SWEPT   : 126 GLBs   (1,522,982,700 bytes)
ACCEPTED: 126
REJECTED: 0
```
⛔ **NO REJECTS TO NAME.** Had there been any, each would appear here with its path,
its firing check and its `describe_glb` line; the sweep prints exactly that and printed
`(no rejects to name)`.

⭐ `TASK-876` **declared** 126/126 and `qa/TASK-878.md` could not execute it. **It is now
executed, and it reproduces exactly** (`TL-§5c` cl. 5). Worst real values, re-derived:

| | worst real asset | floor | headroom |
|---|---|---|---|
| vertices | **11,820** — `Sapper/rerig_t234/Sapper_gameready_nobomb.glb` | 4 | **2,955×** |
| triangles | **9,461** — `Sapper/rerig_t234/attempt1/Sapper_Attack.glb` | 4 | **2,365×** |
| aspect | **0.2652** — `Footman/meshy_anim/Footman_gameready.glb` | 1e-4 | **2,652×** |

All three reproduce `TASK-876`'s table to the digit, including the file names.

✅ **And a silent-skip check nobody had run:** `validate_glb_file` **passes** an artefact
whose POSITION accessors carry no `min`/`max` — the bounds check is *skipped*, with only
a `warn`. **Accepted-with-`bounds_known=False`: 0 of 126.** ⇒ the bounds check had a live
domain on **every** file in the sweep; not one of the 126 passes was a silent skip.

### 4.1 Name census, re-measured rather than relayed (`SC-§40` cl. 9)

| glob | my count | prior claim | agrees |
|---|---|---|---|
| `Cache/**/*.glb` | **126** | 126 | ✅ |
| `trellis_raw.glb` | **19** | 19 | ✅ |
| `meshy_raw.glb` + `meshy_retex.glb` | **33** (16 + 17) | 33 | ✅ |
| `Cache/**/meshy_r*.glb` | **34** | 34 | ✅ |

⭐ **And `qa/TASK-878.md` (F4)'s explanation of the 33-vs-34 is CONFIRMED at the file
level**: the one non-canonical name is **`Cache/Ogre/meshy_raw_task316_42d2ab2.glb`** — a
preserved backup. Reported even though it matched, per cl. 9.

### 4.2 ⚠️ `126` IS MACHINE-LOCAL — nobody may board a row on it

`Tools/ArtPipeline/Cache/*` is gitignored (**`.gitignore:27`**, verified with
`git check-ignore -v`). ⇒ **a clean checkout of this repo contains ZERO cached GLBs and
none of §4 is reproducible by anyone else.** The 126 is a fact about *this machine at
this hour*, not about the project. It must not appear in a task row, a law, or a
convention as though a reviewer could re-derive it.

---

## 5. ⭐⭐ THE FINDING THE ROW DID NOT ASK FOR — the Witch is not the *only* unvalidated artefact, she is the *most recent of nineteen*

The row's premise is that her `state.json` is pre-fix schema. **It is** — verified, and
quoted verbatim below. But I swept the schema across the **whole cache** instead of
reading only hers, and the result reframes the premise:

| `state.json` schema | count |
|---|---|
| **pre-fix** — has `output_glb_bytes`, has **no** `artefact_stats` / `artefact_checks_passed` | **19** |
| **post-fix** — carries `artefact_stats` or `artefact_checks_passed` | **0** |
| meshy-shaped (`engine`/`meshy` keys; different schema entirely) | 4 |
| **total `state.json` under `Cache/`** | **23** |

⇒ ⛔ **19 of 19 trellis provenance records are pre-fix — exactly matching the 19
`trellis_raw.glb` files.** ⭐ **NOT ONE artefact in this project has ever been produced by
the fixed tool.** The Witch is not a lone unvalidated asset; she is simply the **newest
member of a set that is 100% unvalidated at generation time.**

⚖️ **This makes §4 the load-bearing half of this row, not a bonus lap.** Validating only
the Witch would have proved a single file good while nineteen sat in the same condition.
✅ **All 19 pass — and so do the other 107.** The defect was real and the artefacts it
produced are, measurably, all sound.

Her record, verbatim (the four lines the gate quoted, confirmed against the file):
```
"space": "microsoft/TRELLIS.2",   "started_utc": "2026-09-03T05:11:32+00:00",
"finished_utc": "2026-09-03T05:12:58+00:00",   "status": "success",
"gradio_client_version": "2.5.0",   "output_glb_bytes": 19050252,
"output_glb": "C:\\…\\Tools\\ArtPipeline\\Cache\\Witch\\trellis_raw.glb"
```
`output_glb_bytes` present · `artefact_stats` absent · `artefact_checks_passed` absent.
⇒ **pre-fix schema, confirmed by measurement, and now put in its true context.**

⭐ **Byte size proved exactly as worthless as advertised, and I can show it from her own
record:** her `state.json`'s **19,050,252** and my validator's `bytes` field are the
**same number** — the pre-fix tool and the fixed tool agree on the one figure that
decides nothing. What the pre-fix tool never had is the row beneath it: **379,909 verts /
480,750 tris / aspect 0.6518**. *The 19 MB was never evidence; it is evidence now only
because something else was measured beside it.*

---

## 6. ⛔ THE BOUNDARY — declared, and deliberately not crossed

**`Cache/Witch/refine_report.json` EXISTS** (3,666 B, 2026-09-02 22:17 local) ⇒ Stage 2
(`refine_trellis_glb.py`) **already ran on her**, through a path with **zero post-write
validation** (`qa/TASK-878.md` F1). ⇒ **her Stage-2 outputs are NOT covered by this row:**

| file | size | covered by this row? |
|---|---|---|
| `Content/RawAssets/Witch.fbx` | 616,892 B | ⛔ **NO** — `TASK-886` |
| `Content/RawAssets/Textures/Witch/T_Witch_D.png` | 368,354 B | ⛔ **NO** — `TASK-886` |
| `Content/RawAssets/Textures/Witch/T_Witch_N.png` | 892,468 B | ⛔ **NO** — `TASK-886` |
| `Content/RawAssets/Textures/Witch/T_Witch_ORM.png` | 888,974 B | ⛔ **NO** — `TASK-886` |
| `Content/Meshes/SM_Witch.uasset` | 814,393 B | ⛔ **NO** — engine import, no row here |
| `Content/Materials/Instances/MI_Witch_PBR.uasset` | 14,904 B | ⛔ **NO** |
| `Content/Textures/T_Witch_{D,N,ORM}.uasset` | 356,698 / 795,254 / 831,579 B | ⛔ **NO** |

*(Existence and size only — I did not open, parse, validate or judge any of them.)*

⭐ **Their partial mitigation is real and I am stating it rather than alarming:**
`Content/RawAssets/**` is **git-tracked**, so a destroyed FBX is recoverable from the
last commit — a mitigation the gitignored `Cache/` artefacts do not have.
⛔ **I did not extend the row across this boundary and I did not repair anything.**

---

## 7. ⛔ THE REAL CORPUS IS BYTE-IDENTICAL — I checked, in both directions

Pre-state manifest (size + **`st_mtime_ns`**) captured **before** the first read; the
identical manifest recaptured **after** the last:

```
GLBs compared : 126 -> 126        DRIFT: 0
*.py compared :  14 ->  14        DRIFT: 0
stray .part / .tmp-* / _rejected under Cache/: 0
Witch BEFORE : size=19,050,252  mtime_ns=1788412378931266500
Witch AFTER  : size=19,050,252  mtime_ns=1788412378931266500   IDENTICAL: True
sha256(Witch) = 55c59ae392e37225b9d14d3d4e718501d52800716f49a49a31b41b01d4e11937
```

Tool mtimes, unchanged and re-checked at the end — `trellis_generate.py` **2026-09-03
12:02:57**, `meshy_generate.py` **12:03:02** (both `TASK-876`'s, untouched by me),
`refine_trellis_glb.py` **2026-07-28 20:42** (`TASK-886`'s, untouched),
`concept_generate.py` **2026-09-02 22:56** (`TASK-879`'s, untouched).

**Fence held:** zero `.py` edits · zero `Source/` · zero `Content/` · nothing
regenerated, repaired, re-imported, quarantined or deleted · no quota · no network · no
token read · no compile · no editor · **no MCP** (editor left alone, PID 22940) · no Git
command except read-only `git check-ignore -v` to substantiate §4.2. Only two files
written: this handoff and my board row.

---

## 8. ⛔ TWO NUMBERS OF MY OWN WENT WRONG. I am reporting both, because catching them is the point of the standard.

**(a) I labelled a fixture "the 132-byte repro payload". It measured 500 bytes.**
`_synth_glb(0, 0, meshes=0)` produces **500** bytes — it is **not** `TASK-876`'s 132-byte
payload, which was a different, hand-made minimal container. ⛔ **I had carried a number
out of a handoff into my own instrument's label without measuring it — the exact
`SC-§40` cl. 1 shape.** Corrected by building the minimal container myself: **72 bytes,
REJECT `no-mesh`** (§3.4). **The check result never changed; only my label was false.**
⚖️ *And the irony is load-bearing: I mis-stated a **byte count** in a task whose entire
thesis is that byte counts prove nothing. It changed no verdict — which is precisely
because the verdict is not keyed on bytes.*

**(b) My meta-control reported 9 always-rejects disagreements; the shipped one prints 10.**
I did **not** wave this through. Measured: the shipped injection is
`lambda stats: list(ALL_GLB_CHECKS)` (`:878`, all five names); mine was
`[CHECK_NO_MESH]`. Under mine, the **`no-mesh` fixture accidentally AGREES** (it expects
exactly `no-mesh` and got exactly `no-mesh`) ⇒ 9. Re-ran with the shipped injection and
got **10**, and diffed the two disagreement sets: the single separating fixture is
`no-mesh`, exactly as predicted. ⇒ **both counts are correct for their own injection;
the shipped control has no defect here.** ⭐ A weaker always-rejects injection is a
*weaker* meta-control — worth knowing, and it argues the shipped choice is the right one.

**(c) A third, smaller one, same family:** I first reported
`Content/Materials/MI_Witch_PBR.uasset` as **absent**. It exists — at
`Content/Materials/Instances/MI_Witch_PBR.uasset`. **An absence is a measurement**
(`SC-§40` cl. 1); I had measured the wrong path. Re-globbed `Content/**/*Witch*` and
found **13** Witch-named files (§6).

---

## 9. 🔍 What QA should scrutinise

1. ⭐ **The ACCEPT is only as good as §3.** Please check the accept-direction control
   before the verdict: if `flat-plane` (6 verts, **larger** file) were not REJECTED while
   `minimal-tetrahedron` (4 verts, **smaller** file) is ACCEPTED, my §2 would be
   worthless. That pair, not the fixture count, is what makes it evidence.
2. ⭐⭐ **§2.2 is the claim most worth attacking.** "All five evaluated" is exactly the
   sentence a skip-rule can make false silently. I mutated her real stats to prove each
   check live — **satisfy yourself the mutation is of the STATS and not of the CODE**,
   i.e. that I did not neuter a check and then declare it live.
3. ⛔ **§5 is a re-scoping of the row's premise and deserves an independent ruling.**
   19 pre-fix / 0 post-fix means the "last asset generated while the defect was live"
   framing is true but under-states it: **every** trellis artefact was. All pass — but
   whether that changes anything downstream is a manager/QA call, **not mine**, and I
   have deliberately not boarded anything on it.
4. ⚠️ **Do not let §4's `126` escape into a row or a law.** §4.2 — gitignored, machine-local.
5. ⚠️ **This row does NOT clear the Witch's FBX/PNGs** (§6). If `TASK-835`'s item (2c) is
   read as "the witch's assets are validated", that is broader than what I measured.
   **I validated her GLB.** `TASK-886` owns Stage 2.
6. **§8 (a) and (b) are self-reported errors, not findings against anyone.** Read them as
   the instrument's calibration rather than as defects in the shipped tools.
7. **`TASK-890`'s duty is untouched by this row** — QA still owes the two **CLI**
   `--check` runs graded on the guard line, on **both** tools, at the commit. §0.

---

## 10. 🪤 For the manager — reported, not fixed, not boarded

1. ⭐⭐ **§5: 19/19 trellis artefacts carry the pre-fix schema; 0 carry the post-fix one.**
   **All 126 GLBs pass**, so there is nothing to repair — but *"the Witch is the last
   asset generated while the defect was live"* should be recorded as **"the Witch is the
   most recent of nineteen, all of which now measure clean."**
2. ⛔ **The Meshy-retex remedy is still NOT boarded and I am not asking for it.** The row
   named it as quota-costing and deliberately withheld pending this answer. **My answer
   is that her GLB is sound**, so the retex question is now purely about the **darker
   bake** (`J-W13` taste/appearance), **not about artefact integrity.** Those are
   different questions and should not be merged.
3. ⚠️ **§4.2 belongs in `CONVENTIONS.md` if the 126 is ever cited again**: `Cache/` is
   gitignored, so *any* corpus figure from it is machine-local and irreproducible by a
   reviewer. This is `TL-§5c`'s cousin — **a number nobody else can re-derive is a
   declaration wearing a measurement's clothes.**
4. ⭐ Cheap and not done here: `state.json` could carry the `artefact_stats` block going
   forward, which would make "was this artefact validated at birth?" a **file read**
   instead of a **1.5 GB sweep**. `TASK-892`'s WARN sweep is the natural home; I did not
   touch it (it is fenced).

---

## 11. Files touched · assets referenced

**Written — two, both mine:**
- `.claude/pipeline/handoffs/TASK-891-programmer.md` (this file)
- `.claude/pipeline/TASKBOARD.md` — **the `TASK-891` status line only**, edited surgically

**Read-only:** `Tools/ArtPipeline/trellis_generate.py` (imported, never modified) ·
`Tools/ArtPipeline/Cache/Witch/{trellis_raw.glb,state.json,refine_report.json}` ·
all 126 `Tools/ArtPipeline/Cache/**/*.glb` · 23 `Cache/**/state.json` ·
existence+size only of the 13 `Content/**/*Witch*` files.

**Assets referenced:** none authored, none imported, none modified. `SM_Witch`,
`MI_Witch_PBR`, `T_Witch_{D,N,ORM}` are **named** in §6 as the boundary of this row and
were **not** validated by it.

**Scratch (not committed):** `…/scratchpad/t891/census.py` + `census-run.txt`.
