# TASK-926 — [HOST-ARTDATA] the host for the orphaned Witch JSONs and the TASK-832 evidence renders — build-master handoff

**Commit: `8e74596`** — `TASK-926: the host for TASK-833's Witch generation data and the TASK-832 evidence renders`
**Status:** ✅ **done.** ⛔ **NOT PUSHED** — `main` is now **13 ahead** of `origin/main`.
**Law:** `TL-§5e` cl. 4 + cl. 5 · `§25b` cl. R + cl. S · `SC-§29` · `SC-§40` cl. 1 · `TL-§5c` · `TL-§5d` · `SHIP-§9`.
⛔ **No compile · zero `Source/` diff · zero `Content/` diff · suite delta `0` · no `QUIET-MODULE` slot · no editor · no MCP.**
Editor left **UP (PID 7076), untouched** — `TASK-919` needs it down and that is the orchestrator's to sequence.

**7 files changed, 277 insertions(+), 0 deletions(−).**

---

## 1. ⭐⭐ PROVENANCE — RE-MEASURED BY ME, NOT INHERITED. AND THE BOARD WAS THE THING THAT WAS WRONG

`SC-§40` cl. 1: *a relayed provenance is a citation.* I ran `git diff` on both JSONs myself before staging either.

| file | `git status` | diff shape | every hunk's owner | verdict |
|---|---|---|---|---|
| `Tools/ArtPipeline/concept_prompts.json` | ` M` modified | **+5 / −0**, one `"Witch"` key | `TASK-833`, seed **71035** | ✅ **PURE `TASK-833`** |
| `Tools/ArtPipeline/pipeline_manifest.json` | ` M` modified | **+31 / −0**, one self-contained `"Witch"` block | `TASK-833` — **all 5 `_source` fields stamped `"TASK-833 (2026-09-02)"`** | ✅ **PURE `TASK-833`** |

**Task-ID census over both diffs** (`grep -oE "TASK-[0-9]{3}"`): **`TASK-833` ×8 · `TASK-348` ×1 · nothing else.**
⭐ The single `TASK-348` hit is **prose quoted inside `_pre_rotate_source`** (the 2026-07-28 MIRROR-FIX, cited as the reason `pre_rotate_z_deg = 180.0`) — it is **an explanation, not another lane's data**.

### ⛔ THE `TASK-899` STOP-CONDITION **DID NOT FIRE** — measured two independent ways

| instrument | result | required |
|---|---|---|
| `TASK-899` occurrences in the staged diff | **`0`** | `0` ✅ |
| added `"metallic"` / `"orm"` / `"roughness"` keys | **`0`** | `0` ✅ |

⛔ `TASK-899` is Jonathan's `J-W15` ruling on `SM_Witch`'s 95.2%-metallic ORM, assigned to **him**, fenced *"no Git"*, and possibly never dispatched. **None of its data rode here.** The manifest block I committed is *generation input* (engine, tri budget, delight levels, team-region selectors); a `TASK-899` remedy would write **textures**, and would touch this file only as a *remedy fence*.

### ⭐⭐ THE FINDING THE BRIEF PREDICTED, CONFIRMED — AND IT IS A ROW-vs-HANDOFF SPLIT

The brief warned that `TASK-918` **mis-attributed these exclusions** and that its fence therefore *"held for a FALSE REASON, which is luck"*. **Measured, and it is more precisely diagnosed than that:**

- ⛔ **`TASK-918`'s BOARD ROW** attributed `pipeline_manifest.json` to `TASK-899` and `concept_prompts.json` to no open row. **Wrong on both.**
- ✅ **`TASK-918`'s HANDOFF** (§2, "Named exclusions") says: *"`concept_prompts.json` + `pipeline_manifest.json` (art-director **`TASK-833`**)"*. **Correct.**

⇒ ⚖️ ***The same agent recorded the right answer in its evidence file and the wrong one on the board, and the board is what the next row copies.*** The fence held either way — but a reader inheriting the **row** would have concluded these files belong to a Jonathan-fenced ruling and could never be committed at all, which is how an orphan becomes permanent. **This is why item (2) demanded re-measurement, and the demand paid.**

---

## 2. THE PATHSPEC vs THE `names:` LINE — reconciled against `git status`, ⛔ never the line

`§25b` cl. R — proven load-bearing repeatedly tonight. **Ruled 7 paths; the tree agreed on all 7.**

| # | path | owner row | `git status` as found | in commit |
|---|---|---|---|---|
| 1 | `Tools/ArtPipeline/concept_prompts.json` | `TASK-833` | ` M` | ✅ `M` |
| 2 | `Tools/ArtPipeline/pipeline_manifest.json` | `TASK-833` | ` M` | ✅ `M` |
| 3 | `.claude/…/TASK-832-A-gameplay-distance-veiled-vs-control.png` | `TASK-832` | `??` untracked | ✅ **`A`** |
| 4 | `.claude/…/TASK-832-B-refraction-closeup-castle-warped.png` | `TASK-832` | `??` untracked | ✅ **`A`** |
| 5 | `.claude/…/TASK-832-C-control-strength0-no-refraction.png` | `TASK-832` | `??` untracked | ✅ **`A`** |
| 6 | `.claude/…/TASK-832-D-ghost-unchanged-beside-veil.png` | `TASK-832` | `??` untracked | ✅ **`A`** |
| 7 | `.claude/pipeline/handoffs/TASK-832-artist.md` | `TASK-832` | `??` untracked | ✅ **`A`** |

`git show --name-status 8e74596` returns **exactly these seven and nothing else.**

**Fence, measured on the staged set:** `Source/` paths = **0** · `Content/` paths = **0** · `FogArea` = **0**.
✅ Spec (1)'s *"expect a ZERO-LINE source diff"* is satisfied **by measurement, not by intent**.

⛔ **`handoffs/TASK-833-artist.md` was NOT taken.** It is untracked and it *is* the evidence document for the two JSONs I committed — but the `names:` line names only `TASK-832-artist.md`, and improvising a pathspec at the terminal is precisely what cl. R's boarding-time bullet forbids. **Named in §5 and left**, exactly as `TASK-918` and `TASK-922` did with the JSONs.

---

## 3. ⭐⭐ THE FOUR RENDERS — THE SIZE GATE WAS MEASURED, AND THE LFS DOUBT RESOLVED THE OTHER WAY

### (a) The size gate **did not fire**

| | |
|---|---|
| four PNGs, total | **`4,247,911` bytes = 4.05 MB** |
| gate | *"if the four total more than **a few MB**… take only `TASK-832-D`"* |
| ruling | **4.05 MB is within "a few MB", not over it ⇒ the reduce-to-one branch does not fire.** All four taken. |

**Reported plainly as instructed, rather than resolved silently.**

### (b) ⛔⛔ THE ROW'S OWN LFS QUESTION — ANSWERED, AND THE ANSWER IS THE OPPOSITE OF ITS GUESS

The row said: *"`Content/`-style LFS rules do **not** apply under `.claude/` — **VERIFY which, do not assume**."* ✅ **Verified — and they DO apply:**

```
$ git check-attr filter diff -- .claude/pipeline/handoffs/TASK-832-D-…png
  filter: lfs      diff: lfs
$ grep -n png <repo-root>/.gitattributes
  4: *.png filter=lfs diff=lfs merge=lfs -text
```

⭐ **The rule is `*.png` — UNANCHORED, at the REPO ROOT — so it reaches every directory, `.claude/` included.**
⇒ **the git tree carries four ~130-byte pointers; the 4 MB lives in LFS.** Confirmed by reading the staged blob:

```
version https://git-lfs.github.com/spec/v1
oid sha256:96f8ac3ac70e012c200375454a28604320bf2dcbcb0a175260eee2965787c5b9
size 1039116
```

⭐ **Precedent, measured rather than assumed: `262` PNGs are ALREADY tracked under `.claude/pipeline/`.** Committing evidence renders here is established practice, not a novel act.

### (c) ⭐ ALL FOUR ARE **ARGUED FROM** — and each carries an argument no other frame carries

The gate's own standard is *"evidence is committed because it is **ARGUED FROM**, not because it exists."* **Census of citations across `.claude/pipeline/`:**

| frame | citations | the argument it and only it carries |
|---|---|---|
| **A** | **×5** | four figures at ~1600 uu — the *"can the owner tell at a glance, without it reading as dead?"* answer, **and** the side-by-side that shows the veil is nearly a no-op on the metallic Witch |
| **B** | **×2** | the up-close proof that what ships is **distortion/smear, not the blur Jonathan asked for** — the one place his word and the pixels diverge |
| **C** | **×5** | ⭐⭐ **the NEGATIVE CONTROL that caught the IOR-neutral bug** |
| **D** | **×9** | ⭐ the ghost rendering **clean** beside the scrambling veil — the non-regression itself |

⛔⛔ **Why reducing to `D` alone would have been wrong even if the gate had fired:** `TASK-832-C` is the frame that was *shot to prove the default was safe and disproved it instead*. `VeilRefractionStrength` was defaulted to `0` so shipped instances would be untouched — but in `RM_IndexOfRefraction` the neutral is **`1.0`**, and `0` is violently refractive. **Discarding the control and keeping the positive result is exactly what `SHIP-§9` forbids** (*validate a gate against the failure it detects*). `D` carries the conclusion; `C` carries the reason anyone should believe it.

### (d) Why these renders are load-bearing **for a commit that already shipped**

`a8b97b3` edited **`M_HeroSpirit`, a SHIPPED master material**. `MI_Ghost_Translucent` — the hero's death ghost — **inherits** from it. ⛔ **A shared-master edit can regress a child whose own bytes never move**, so the child's byte-identity proves nothing about how the child *renders*. These four frames are the only evidence that did not rot untracked. **`TASK-922`'s own verification layer 4 was "I opened the evidence render and LOOKED"** — that instrument was living in the working tree only, one `git clean` from gone.

---

## 4. ⛔ `§25b` cl. S — EVERY CHANGED BINARY SWEPT, BY OID-vs-`sha256`, NEVER BY SIZE

The only binaries in play this sitting are the four PNGs (all other dirty paths are text). **Swept at three points:**

| frame | pre-stage worktree `sha256` | staged pointer oid | post-commit `HEAD` pointer oid | verdict |
|---|---|---|---|---|
| A | `7627c7b7…f434` | `7627c7b7…f434` | `7627c7b7…f434` | ✅ **MATCH ×3** |
| B | `f657256c…db6b` | `f657256c…db6b` | `f657256c…db6b` | ✅ **MATCH ×3** |
| C | `c759ef1a…c17a` | `c759ef1a…c17a` | `c759ef1a…c17a` | ✅ **MATCH ×3** |
| D | `96f8ac3a…c5b9` | `96f8ac3a…c5b9` | `96f8ac3a…c5b9` | ✅ **MATCH ×3** |

**Nothing stale.** For the three text files, index-vs-worktree was proven by the complementary instrument: after staging, `git diff --name-only` over the full 7-path pathspec returned **empty**.

⭐ **Integrity beyond the digest:** each PNG was verified to be a **real, complete** image before staging — `\x89PNG` magic, parsed `IHDR` dimensions (`1150×820`, `900×817`, `900×817`, `1150×650`), and a terminal `IEND` chunk. ⚖️ *Given that this whole chain exists because tools reported SUCCESS over empty artefacts, committing an evidence render without confirming it decodes would have been the same defect one layer up.*

### ⚠️ BRIEFING EXPECTATION CORRECTED: **NOTHING WAS STAGED**

The brief warned *"other assets are staged by the editor's revision-control integration, staged by no task; leave the index exactly as you find it."*
⛔ **Measured: `git diff --cached --name-only` returned `0` paths on arrival. The index was completely clean.** `TASK-922`'s auto-stage fire (`M_HeroSpirit` holding a pre-edit oid) **did not recur** this sitting. I left the index as found — which happened to mean adding only my own 7 paths to an empty index.
⭐ **The sweep still ran in full.** A clause that has fired 4–7 times is not owed less diligence on the night it comes back clean; *"clean"* is a **result**, and it is only a result if you measured.

### `TL-§5c` / `TL-§5d` — no post-gate drift

Post-commit, **`HEAD`'s blob/pointer oid equals the worktree digest on all four binaries** ⇒ the tree I inspected **is** the tree at `HEAD`. `main` verified at **12 ahead before** / **13 ahead after** — ⭐ **counted myself**, as instructed, because the brief's own count had gone stale once already tonight.

---

## 5. ⛔ `§25b` cl. R — THE FULL RECONCILIATION. ⛔ REPORTED, ⛔ NO `git reset`, NOTHING UNSTAGED

⛔ **No `git reset` was run. Committed by explicit pathspec, so nothing *could* be swept in.** Everything below is **left exactly as found.**

### (a) ⚠️ `Content/FogArea/` — **NOT MINE, named and left** (`TASK-927`)

**Re-measured: `27` files, `28 MB`, still `??` untracked.** ⛔ No commit · ⛔ no `.gitignore` edit · ⛔ no delete · ⛔ no move · ⛔ `BP_FogArea` never opened. Awaiting Jonathan's `J-F11` ruling.

### (b) 7 modified `Source/` files — **`TASK-919`'s**, left dirty and untouched

`SiegeControlsHelpWidget.{h,cpp}` · `SiegePlayerController.{h,cpp}` · `Tests/SiegeControlsHelpTest.cpp` · `Tests/SiegePlacementTest.cpp` · `Tests/SiegeAssistantSelectionTest.cpp`. ⛔ **Not a finding** — this matches `TASK-919`'s pathspec exactly, as `TASK-922` also recorded. My commit's `Source/` diff is **zero lines**.

### (c) `CONVENTIONS.md` + `TASKBOARD.md` — modified, no commit row

Standing pipeline-doc dirt. `TASKBOARD.md` carries my own `done` flip. **Not adopted** — historically Jonathan sweeps these into his own commits.

### (d) 🚨⭐⭐ THE FINDING THAT IS **LARGER THAN MY OWN ROW**: the orphan class is **61 files**, not three

I was dispatched to close *"the third orphan of the sitting."* **The reconciliation says the class is far bigger:**

| | count |
|---|---|
| untracked `handoffs/*.md` | **44** |
| untracked `qa/*.md` | **17** |
| **total un-hosted pipeline documents** | **⛔ 61** |

⛔⛔ **The tracked backlog STOPS AT `TASK-805`.** The repo holds **816** handoffs and **162** QA reports — and **nothing from `TASK-807` onward**. `1aa0fee` (`TASK-835`, 39 files) carried **zero** `.md` files; so did `44a8710` and `a8b97b3` beyond their own scope.

⛔⛔ **THE SHARPEST INSTANCE, AND IT SHOULD BE READ TWICE: `qa/TASK-865.md` — the PASS verdict that GATED commit `44a8710` — IS NOT IN THE REPOSITORY.**
⇒ ⚖️ ***A commit shipped under a QA report that a clean clone cannot read.*** The hard gate (*nothing commits without a PASS report*) was **honoured in the session and is unverifiable from the repo**. The same is true of `qa/TASK-878.md`, `TASK-847`, `TASK-849`, and 13 others.

📋 **MANAGER: this wants ONE doc-host row, not sixty-one.** It is cheap (text, no LFS, no gate), and it is the same `TL-§5e` cl. 5 shape as my own row — **a detector firing reliably at nobody in particular.**

### (e) ⭐ THE STRUCTURAL CAUSE, RECORDED SO THE SWEEPER ROW IS WRITTEN CORRECTLY

⛔ **A build-master handoff cannot contain its own commit hash *and* be inside that commit.** Amending only moves the hash again.
⇒ `TASK-918`'s, `TASK-922`'s and **now this file** are untracked for exactly that reason. ⛔ **Every host row mints one new orphan by construction.**
⚖️ ***A convention that guarantees a leftover needs a sweeper row, not more diligence from the next agent.*** The fix is a trailing doc-host that runs **after** a batch of commits and takes the handoffs they produced.

---

## 6. COVERAGE (`SC-§29`) — what gates this, and what it explicitly does not

⛔ **No QA report gates this commit, and none is owed.** `TASK-926` is an **asset/data host**: zero `Source/`, zero `Content/`, zero executable change. The code-QA gate does not apply to generation inputs or evidence renders.

- `Tools/**/*.py` **counts as code** under the 2026-07-07 rule — ⭐ **and I committed none.** Both `Tools/` paths here are **`.json` data**, and no `.py` file was touched or staged.
- The Witch **art** these JSONs describe was gated by `TASK-833` (`ready-for-integration`, all five assets read back from the editor) and shipped under `TASK-835`/`1aa0fee`. **This commit adds her reproducibility inputs after the fact; it ships no new art.**
- The renders are gated by `TASK-832`'s own artist verification and by `TASK-922`'s independent pixel read.
- ⛔ **No secret entered this commit, this file, or any log.** No `HF_TOKEN` / `MESHY_TOKEN` value was read or echoed; neither JSON contains a credential field.

---

## 7. WHAT THIS COMMIT IS NOT — stated in the message itself, not only here

⛔ **Nothing here is gameplay.** No behaviour changes, no `Source/`, no `Content/`, no compile, suite delta `0`.
The commit message states plainly that these are **generation inputs and evidence renders**, that the JSONs were **orphaned when `TASK-835`/`1aa0fee` took `Concepts/Witch.png` and not the JSONs that produced it**, and that **the renders exist because a shipped master material was edited and its child had to be proven unchanged**. It names **`TASK-833`**, **`TASK-926`**, **`TASK-832`**, **`TASK-899`** (as excluded), **`TASK-918`** and **`TASK-922`**.

⭐ **Grepped before writing, per the predecessor's discipline:** every subject named in the message was verified present in the staged diff first — seed `71035` ✅, `TASK-833` stamps ✅, four renders ✅, `TASK-832-artist.md` ✅ — and every subject verified **absent** was stated as absent, not as present: `TASK-899` content `0` ✅, metallic/ORM keys `0` ✅, `Source/` `0` ✅, `Content/` `0` ✅.

---

## 8. FOLLOW-UPS FOR THE MANAGER

1. 🚨 **A pipeline-doc host row for the 61 untracked `handoffs/` + `qa/` files** — with `qa/TASK-865.md` (gate of `44a8710`) called out as the reason it is not cosmetic. **§5(d).**
2. ⭐ **Write that row to run LAST in a batch**, and note in `TL-§5e` that a build-master handoff **structurally cannot self-host**. **§5(e).**
3. ⚠️ **`TASK-918`'s board row still carries the wrong attribution** for these two JSONs (`pipeline_manifest.json`→`TASK-899`). It is now harmless — the files are committed — but the **wrong sentence is still there to be copied**. Worth a one-line annotation rather than a rewrite (`TL-§5c` cl. 4). **§1.**
4. ⛔ **`handoffs/TASK-833-artist.md` is still untracked** — the evidence document for the very JSONs this row committed. Folds into (1).
5. ⛔ **`Content/FogArea/` still awaits `J-F11`** — 27 files / 28 MB re-measured, reported for the **third** time. **§5(a).**
