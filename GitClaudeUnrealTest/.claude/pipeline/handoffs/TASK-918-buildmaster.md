# TASK-918 — [HOST-TOOLS] build-master handoff

**Commit: `44a8710`** (`44a87108d8a96f6e9c4d7543a6797fe400df4ce2`) · main **11 ahead** of `origin/main` · **NOT PUSHED**
Date: 2026-09-03 · Host row: `TASK-918` · Adopted riders: **`TASK-866`** + **`TASK-890`**
No compile · no automation suite · no `QUIET-MODULE` slot · no editor · no MCP. Editor left **UP (PID 7076), untouched.**

---

## 1. THE FOUR CONTROLS — the duty transferred by name, now EXECUTED

`TL-§5c` cl. 5. Both gates ran **without a shell and said so**. Every `--check` exit code and fixture
result on this chain was, until this commit, **declared by the authors and observed by nobody.**
I am the first party who ran them. **All four passed, all four exit 0.**

| # | command | exit code **I observed** | verdict |
|---|---|---|---|
| 1 | `uv run concept_generate.py --check` | **0** | PASS |
| 2 | `uv run test_concept_guard.py` | **0** | PASS — `46/46` |
| 3 | `uv run trellis_generate.py --check` | **0** | PASS |
| 4 | `uv run meshy_generate.py --check` | **0** | PASS |

⭐ **Neither expected-pass trap had to be invoked.** No trailing exit `1` (Norton TLS / Space
unreachable), no exit `2` (no key), and **no exit `5` VOID** on the guard self-test. Graded on the
guard line regardless, per `TASK-890` item (0a) — but the exit codes happened to agree with it.

### The guard lines, verbatim

**(1) `concept_generate.py --check`** — the control fired RED on all 7 degenerate fixtures and green
on all 3 legitimately-dark ones, then:
```
[concept] --check: degeneracy guard control PASSED - RED on every degenerate fixture, green on every legitimately-dark one.
[concept] --check: HF_TOKEN is present in the environment (value not read/echoed).
[concept] --check PASSED: tool wires up (model constant: black-forest-labs/FLUX.1-dev).
```
⭐ **The tool redacts its own secret** — it asserts `HF_TOKEN` presence without reading or echoing
the value. Nothing token-shaped entered any transcript, this file, or the commit.

**(2) `test_concept_guard.py`** — `46/46`:
```
  [PASS] 49 real concept PNGs assessed, 0 rejected  -- corpus worst p99.9=0.7412 (Footman.png), worst stddev=0.0840 (CrystalTower.png)
  margin: p99.9  floor 0.1200 vs corpus worst 0.7412  (6.2x headroom)
  margin: stddev floor 0.0100 vs corpus worst 0.0840  (8.4x headroom)
...
  [PASS] a guard that NEVER rejects is caught by the control  -- 7 disagreement(s) reported
  [PASS] a guard that ALWAYS rejects is caught by the control  -- 8 disagreement(s) reported
  [PASS] `--check` FAILS (exit 1) while the guard is broken  -- got 1
  [PASS] `--check` passes again once the real guard is restored  -- got 0
========================================================================
46/46 assertions passed.
ALL GREEN - the guard rejects degenerate frames, accepts every real concept, and never overwrites a good roll with a bad one.
```
⚠️ **Corpus resolved at 49 — NOT exit 5 (VOID).** That `49` is **MACHINE-LOCAL** (25 gitignored
`Inbox/` + 24 tracked). A clean checkout sees 24. **No row may be boarded on `49`.**

**(3) `trellis_generate.py --check`** — guard line **FIRST**, exact numbers:
```
[trellis] --check: degeneracy guard control PASSED - 11 fixtures (8 rejected, 3 accepted), all 5 checks fire in isolation, and the control itself goes RED when the guard is broken (7 disagreements with a never-rejects guard, 10 with an always-rejects one).
[trellis] --check: offline signature assert OK - Client.__init__ accepts 'token'.
[trellis] --check: connecting to microsoft/TRELLIS.2 (tokenless; no GPU call, no quota spend)
Loaded as API: https://microsoft-trellis-2.hf.space
[trellis]   /preprocess_image - OK (params: input)
[trellis]   /image_to_3d - OK (params: image, seed, ...)
[trellis]   /extract_glb - OK (params: decimation_target, texture_size)
[trellis] --check PASSED: Space reachable, all three endpoints present.
```

**(4) `meshy_generate.py --check`** — guard line **FIRST**, exact numbers:
```
[meshy] --check: degeneracy guard control PASSED - 11 fixtures (8 rejected, 3 accepted), all 5 checks fire in isolation, and the control itself goes RED when the guard is broken (7 disagreements with a never-rejects guard, 10 with an always-rejects one).
[meshy] API key resolved from process env MESHY_TOKEN (value redacted everywhere).
[meshy] --check: probing https://api.meshy.ai (free read endpoints; no credit spend)
[meshy]   /openapi/v1/balance - OK (credits remaining: 3200)
[meshy]   /openapi/v1/retexture - OK (list endpoint reachable, 0 task(s) in first page)
[meshy]   /openapi/v1/image-to-3d - OK (list endpoint reachable, 0 task(s) in first page)
[meshy] --check PASSED: key valid, API reachable, documented endpoints present.
```

✅ **`11 · 8 · 3 · 5 · 7 · 10` reproduced EXACTLY from both mesh tools**, and the guard line printed
**before** any exit in both. `TASK-890` item (0a)'s pass criterion is met to the digit.

### ⚠️ REPORTED AS INSTRUCTED — two of the four DID reach the network

The brief said these should need no network and to say so if one reached for it. **Two did:**

- **`trellis --check` reached `microsoft-trellis-2.hf.space`** to probe the three endpoint schemas.
  Tokenless, no GPU call, no quota spend. It **succeeded** — no Norton interception observed.
- **`meshy --check` reached `https://api.meshy.ai`** for three free read endpoints. It **succeeded**
  and reported **3200 credits remaining**, no credits spent.

⭐ **This contradicts the standing expectation that `router.huggingface.co` and `api.meshy.ai` are
Norton-intercepted** — on this machine tonight, both were reachable bare, so the documented
expected-pass traps never fired. **Verification was never disabled.** I did not investigate further;
recorded as a live observation, not a ruling. **The guard-line half of both runs is pure stdlib and
offline regardless** — the network half only probes endpoint schemas and never gates the guard.

⚠️ `trellis --check` **writes** `Tools/ArtPipeline/Cache/api_schema.json`. Confirmed gitignored:
`git check-ignore -v` → `.gitignore:27 Tools/ArtPipeline/Cache/*`. **It did not and cannot enter the commit.**

### Fence: the controls changed nothing

`git status --porcelain -- Tools/` is **byte-identical before and after** all four runs. No stray
`.part`, no `.tmp-*`, no `_rejected` leakage into git (`Inbox/*` is gitignored at `.gitignore:25`).
All test artefacts were written to `%TEMP%\task864-*` scratch dirs.

---

## 2. THE PATHSPEC — five paths, each verified against `git status`, never against a `names:` line

`§25b` cl. R. **Ruled pathspec vs what the tree actually held — every path present and accounted for:**

| # | path | owner row | `git status` state | in commit |
|---|---|---|---|---|
| 1 | `Tools/ArtPipeline/concept_generate.py` | `TASK-866` | ` M` modified | ✅ `M` |
| 2 | `Tools/ArtPipeline/test_concept_guard.py` | `TASK-866` | `??` **untracked** | ✅ **`A` (new)** |
| 3 | `Tools/ArtPipeline/README.md` | `TASK-866` | ` M` modified | ✅ `M` *(see §3)* |
| 4 | `Tools/ArtPipeline/trellis_generate.py` | `TASK-890` | ` M` modified | ✅ `M` |
| 5 | `Tools/ArtPipeline/meshy_generate.py` | `TASK-890` | ` M` modified | ✅ `M` |

**`git show --name-status 44a8710` returns exactly these five and nothing else.** 2545 insertions, 52 deletions.

⭐ **The new file was the one at risk and it landed.** `test_concept_guard.py` was **untracked** —
invisible to `HEAD`, and nothing would ever have gone red to say so (`TL-§5d`). A guard shipped
without its control is literally the defect this chain exists to fix.

**Not double-counted:** `concept_generate.py` and `test_concept_guard.py` are `TASK-866`'s and were
committed under `qa/TASK-865.md`, **not** under `TASK-890`'s verdict. `TASK-890` contributed
**exactly two** paths, as its row rules.

**Named exclusions, verified still dirty and left alone:**
`concept_prompts.json` + `pipeline_manifest.json` (art-director `TASK-833`) · `refine_trellis_glb.py`
(`TASK-886`, ungated — **not modified at all**, nothing to exclude) · `Cache/**` (gitignored).

### Index integrity — `§25b` cl. S, DISK-vs-INDEX, all five MATCH

| file | index oid == worktree oid |
|---|---|
| `concept_generate.py` | `1f9d81b4…41dd0` **MATCH** |
| `test_concept_guard.py` | `4879bd79…0473c` **MATCH** |
| `README.md` | `42c77636…66394` **MATCH** |
| `trellis_generate.py` | `60d6cc65…87aa6` **MATCH** |
| `meshy_generate.py` | `35d52b6b…96b0a` **MATCH** |

**Nothing stale.** Re-verified **after** the commit: all five **committed blob oids equal the
worktree oids I ran the controls against** ⇒ the tree that passed **is** the tree at `HEAD`
(`TL-§5d`'s subject question, answered by digest rather than by hope).
**None of the five is LFS-tracked** (`git check-attr filter` → `unspecified` on all five), so
`§25b`'s oid-vs-sha256 clause does not bind here.

### `TL-§5d` — no post-gate drift

Every committed file's mtime **predates its own gate**, so both gates read the tree I shipped:

| file | mtime | its gate | gate written |
|---|---|---|---|
| `concept_generate.py` | 09-02 22:56 | `qa/TASK-865.md` | 09-02 **23:11** ✅ |
| `test_concept_guard.py` | 09-02 22:57 | `qa/TASK-865.md` | 09-02 **23:11** ✅ |
| `README.md` | 09-02 22:57 | `qa/TASK-865.md` | 09-02 **23:11** ✅ |
| `trellis_generate.py` | 09-03 12:02 | `qa/TASK-878.md` | 09-03 **12:21** ✅ |
| `meshy_generate.py` | 09-03 12:03 | `qa/TASK-878.md` | 09-03 **12:21** ✅ |

---

## 3. THE CONTESTED FILE — `README.md`, RESOLVED, TAKEN

`TASK-918` item (2a) / `TASK-866` item (2c) vs `TASK-881` item (2) + `TASK-877`.

**The whole test is `git diff --stat` on that one path, and I did not eyeball the file. Result:**

```
 GitClaudeUnrealTest/Tools/ArtPipeline/README.md | 2 +-
 1 file changed, 1 insertion(+), 1 deletion(-)
```

The entire diff:
```diff
-& "...blender.exe" --background --python refine_trellis_glb.py -- --asset Footman
+& "...blender.exe" --background --python refine_trellis_glb.py -- --card-id Footman
```

✅ **ONE token — `--asset` → `--card-id`. This is the `TASK-864` one-token edit exactly.**
⛔ **It is NOT a Stage-0 rewrite.** `TASK-877` has **NOT** run early and **NOT** run ungated.
⇒ **The stop condition did not fire; `README.md` was taken under `TASK-866`, as ruled.**
⇒ ⭐ **`TASK-877` is now UNBLOCKED** — its `blocked-by` was `TASK-866`'s commit, which is `44a8710`.

---

## 4. COVERAGE LEDGER (`SC-§29`) — what this commit is covered by, and what it is NOT

| adopted row | gate report | verdict | covers |
|---|---|---|---|
| **`TASK-866`** | `.claude/pipeline/qa/TASK-865.md` | **PASS** — 0 BLOCKER · 5 WARN · 6 NIT | `TASK-864` only |
| **`TASK-890`** | `.claude/pipeline/qa/TASK-878.md` | **PASS** — 0 BLOCKER · 7 WARN · 5 NIT | **`TASK-876` ONLY** |

⛔ **`TASK-874` is NOT in either verdict and did NOT ride here.** It was never delivered; its gate is
`TASK-889`, its file is `TASK-906`'s, and that belongs to `TASK-919` — not to me. **Nothing in this
commit is covered by a gate that did not name it.**

⛔ **Still DECLARED and NOT owed before commit** (recorded as such, deliberately **not** re-run):
the 12-mutation sweep · the 126/126 corpus sweep · the 4-run behavioural repro · the healthy-artefact
regression. *(The 126/126 was separately EXECUTED by `TASK-891`; that is that row's evidence, not mine.)*

---

## 5. `WARN-1` CARRIED FORWARD BY NAME — NOT FIXED

⛔ **`meshy_generate.py --help` still omits exit 6.** Its module docstring carries it; both siblings'
epilogs carry it; meshy's epilog does not. **This shipped in `44a8710` unfixed, on purpose.**
It belongs to **`TASK-892` item (1)**. I commit; I do not author.

⭐ **I reproduced WARN-1 independently rather than relaying it**, driving all three tools through the
same predicate the shipped assertion at `test_concept_guard.py:460` uses
(`f"{EXIT_DEGENERATE} DEGENERATE" in build_parser().epilog`):

```
concept  epilog_has_6=True   docstring_has_6=True
trellis  epilog_has_6=True   docstring_has_6=True
meshy    epilog_has_6=False  docstring_has_6=True
```

⇒ ⚠️ **Confirmed at exactly one site, and the tripwire is live:** `test_concept_guard.py:460` is a
**shipped, passing** assertion for `concept_generate.py` — **`meshy_generate.py` would fail that same
assertion today.** The fix is one string. `TASK-877` has already been instructed in writing to take
its exit table from the **module docstring** (which is correct in all three) and never from
meshy's `--help`.

---

## 6. WHAT THIS COMMIT ACTUALLY CLAIMS — every claim grepped before it was written

Following the precedent set by a predecessor host that grepped its files first and found three of
seven briefed subjects were not in its commit. **I verified all four claims in the message against
the diff and the handoffs. All four hold:**

1. **`concept_generate.py` reported SUCCESS over a fully black frame while destroying an
   837,664-byte good roll** — `handoffs/TASK-864-programmer.md:17`
   (`good roll preserved: False (837,664 -> 3,129 bytes; sha c27b94df -> d6911226)`).
   The number that would have exposed it printed **beside the word SUCCESS**.
2. **Both mesh tools shared it: a 132-byte GLB replaced a 23,434,424-byte model, rc=0** —
   `handoffs/TASK-876-programmer.md:72-75`, reproduced for BOTH tools. The payload is a
   syntactically valid GLB 2.0 container with an **empty `meshes` array**, so every
   "did the file arrive" check passed.
3. **stage → validate → swap, ONE overwrite call site per tool** — measured by me in the committed
   files: `commit_staged()` has **exactly one caller each** at `concept_generate.py:908`,
   `trellis_generate.py:1173`, `meshy_generate.py:1367`. The second `os.replace` in each tool is the
   **quarantine** path, not an overwrite of the destination.
4. **Rejects quarantined, never deleted** — `Inbox/_rejected/` (concept),
   `Cache/<Asset>/_rejected/` (both mesh tools). Exit **6** = DEGENERATE ARTEFACT, distinct from
   the family's `0/1/2/3/4/5/64`.

⭐ **Context that justifies the row but is NOT claimed in the commit message** (it is not in this
diff): the mesh generator is how every unit in this game was made — `TASK-891` measured 126 GLBs,
19 of them pre-fix trellis artefacts. That is `TASK-891`'s finding, recorded there, not re-claimed here.

---

## 7. `§25b` cl. R — THE STANDING RECONCILIATION

`git status --porcelain` against the **union of all pending board pathspecs**, naming every file that
appears in **neither**.

### Files that ARE claimed

| file / group | owning row | gated? |
|---|---|---|
| `concept_generate.py` · `test_concept_guard.py` · `README.md` | `TASK-866` → host `TASK-918` | ✅ `qa/TASK-865.md` PASS — **committed `44a8710`** |
| `trellis_generate.py` · `meshy_generate.py` | `TASK-890` → host `TASK-918` | ✅ `qa/TASK-878.md` PASS — **committed `44a8710`** |
| `SiegeControlsHelpWidget.{h,cpp}` | `TASK-873` → host `TASK-919` (1a) | ✅ `qa/TASK-872.md` PASS |
| `Tests/SiegeControlsHelpTest.cpp` | **co-claimed** `TASK-857` + `TASK-873` → `TASK-919` (1a) | ✅ both PASS |
| `SiegePlayerController.{h,cpp}` · `Tests/SiegePlacementTest.cpp` | `TASK-871` via `TASK-873` → `TASK-919` (1b) | ✅ `qa/TASK-914.md` **PASS, 0 blockers — condition MET** |
| `Tests/SiegeAssistantSelectionTest.cpp` | `TASK-906` (carries `TASK-874`) → `TASK-919` (1c) | ⛔ **UNGATED — `qa/TASK-889.md` DOES NOT EXIST (verified absent)** |
| `MI_Unit_Invisible.uasset` · `M_HeroSpirit.uasset` | `TASK-832`, `ready-for-integration` | art row — **no commit host exists** |

### ⛔ UNCLAIMED — in no row's pathspec (STOP-THE-LINE report to the manager, per cl. R)

1. ⛔ **`Content/FogArea/`** — untracked, **27 files, 28 MB**. Three rows name it (`TASK-836`,
   `TASK-841`, `TASK-859`) and **all three mark it READ-ONLY vendor pack**; `TASK-843` (Lane F
   integration) does **not** list it in its `names:`. ⇒ **no row claims it for commit.**
   ⚠️ It **dirties its own package on load** — expect recurring churn.
2. ⛔ **`Tools/ArtPipeline/concept_prompts.json`** — **claimed by no open row.**
3. ⛔ `.claude/pipeline/TASKBOARD.md` · `CONVENTIONS.md` · 43 `handoffs/*.md` + 4 `TASK-832-*.png` ·
   16 `qa/*.md`. **Systematic, not a one-off:** none of the last four commits (`44a8710`, `1aa0fee`,
   `e9df584`, `d287102`) contains a single `.claude/pipeline/**` path.

### ⚠️ A CORRECTION TO MY OWN ROW'S TEXT — found by reconciling instead of trusting the line

⛔ **`TASK-918` item (2) attributes BOTH `pipeline_manifest.json` and `concept_prompts.json` to the
art-director's `TASK-833`. That attribution is wrong on both counts:**
- `pipeline_manifest.json`'s actual named claimant is **`TASK-899`** (`names:` line 15836, Witch
  entry), status **awaiting-jonathan (`J-W15`)**, **UNGATED**. `TASK-833`'s `names:` does not list it.
- `concept_prompts.json` is named by **no open row at all** (the only `names:` hit is `TASK-300`, long shipped).

✅ **No harm to this commit — both were EXCLUDED either way, which is the correct outcome under both
attributions.** But the exclusion held for the right reason by luck, not by the row's text, and a
future host reading item (2) would look for an owner that does not claim the file. **Reported, not fixed.**

### 🪤 A `§25b` FIRE-SHAPE ON THE `TASK-832` PAIR — reported, untouched

⛔ **`MI_Unit_Invisible.uasset` is `A ` (STAGED) while its parent `M_HeroSpirit.uasset` is ` M`
(UNSTAGED).** The editor's revision-control integration staged the **new** asset and left the
**modified** one behind — precisely the split `§25b` records. ⇒ **a bare index-wide `git commit` by
any host would ship the veil instance WITHOUT the master's `RefractionMethod = RM_IndexOfRefraction`
change, and the instance would resolve against an unrefracting parent.**
⭐ **This is exactly why I committed by explicit pathspec: the hazard was in the index the whole
time and my commit could not touch it.** Both are `Content/**`, fenced out of `TASK-918` item (7)
**and** `TASK-919` item (1d) ⇒ **neither host may take them; they need an art-integration host row.**

⛔ **NO `git reset` WAS RUN, and that is deliberate** — two recorded reasons, both still true:
(a) the commit was made **by explicit pathspec**, so nothing unclaimed *could* be swept in regardless
of what sits in the index; (b) a reset would **destroy index oids another row still owes a digest
against.**

✅ **The index was left exactly as found.** `Content/Materials/MI_Unit_Invisible.uasset` was staged
by the editor's revision-control integration before I started and **is still staged, unmodified,
after my commit** — verified by `git diff --cached --name-only` post-commit. It is not mine and I
did not touch it.

⛔ **Untouched as instructed:** the long-standing dirty art files, `L_Arena`, all `Source/**`
(`TASK-919`'s tree — we ran concurrently and disjoint), all `Content/**`.

---

## 8. STATUS FLIPS OWED (board is the manager's; recorded here, not edited by me)

- `TASK-918` → **done** (`44a8710`)
- `TASK-866` → **done** — adopted by host `44a8710`, gate `qa/TASK-865.md`
- `TASK-890` → **done** — adopted by host `44a8710`, gate `qa/TASK-878.md`

**Unblocked by this commit:**
- ⭐ **`TASK-877`** (README Stage-0 rewrite) — its blocker was `TASK-866`'s commit. **Now clear**, and
  the file is clean at `HEAD` with the one-token fix already in.
- ⭐ **`TASK-879`** (the WARN sweep on `concept_generate.py`) — blocked on this commit. **Now clear.**
  The ungated-sweep hazard is gone: the file is committed, so a later edit is visible as a diff.

**NO PUSH.** main is **11 ahead** of `origin/main`. Jonathan pushes his own milestones.
