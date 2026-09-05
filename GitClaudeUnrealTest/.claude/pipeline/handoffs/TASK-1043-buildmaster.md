# TASK-1043 — the integration check + ship host for TASK-841

**Agent:** build-master · **Date:** 2026-09-05 · **Status:** `done`
**Commit:** `ef2c901` on `main` · **NOT PUSHED** · new ahead-count **23** (was 22)
**Subject:** `/Game/Blueprints/BP_SiegeFog` · **Gate:** WAIVED — *this row IS the gate, for art*

---

## 0. THE HEADLINE, BECAUSE ONE ITEM MUST NOT BE BURIED

The integration check **PASSED on every clause**. The art shipped.

🚨 **But the suite did not finish, and the cause is not this diff.** A test belonging to
**`TASK-1044`** — `Siegebound.Fog.TheUnitSideCeilingIsOneDoorAndAmbushIsExemptByConstruction` —
**hangs indefinitely**. Reproduced twice, isolated with a positive control. **`TASK-1044` must not
ship until it is resolved, and `TASK-1045` is blocked behind it.** Full evidence in §4.

---

## 1. THE INTEGRATION CHECK — clauses 1–3, all measured, none assumed

Performed against the **live editor session `TASK-841` left standing** (PID 13600, window title
read `GitClaudeUnrealTest - Unreal Editor`, so no `Restore Packages` modal — checked by **title**,
not by the port, per `SC-§68`). The editor was closed only afterwards, for my compile.

| # | claim | how it was checked | result |
|---|---|---|---|
| 1 | `/Game/Blueprints/BP_SiegeFog` exists | MCP resolved the asset | ✅ |
| 1 | parent is `Content/FogArea/Blueprints/BP_FogArea` | `get_parent` ⇒ `/Game/FogArea/Blueprints/BP_FogArea.BP_FogArea_C` | ✅ |
| 1 | vendor pack byte-unchanged | **all 27** `Content/FogArea/**` files, **oid-vs-sha256** | ✅ **0 mismatches, 0 raw blobs** |
| 1 | no `_Auto` package created | `Saved/Autosaves/**` timestamps + `git status` | ✅ **zero** |
| 2 | **vendor-dirt check** — `BP_FogArea` dirties its own package on load | `git status --porcelain -uall -- Content/FogArea/` | ✅ **EMPTY** |
| 3 | handoff prints its camera coordinates | read at source | ✅ **six poses printed** |
| 3 | none is the forbidden vantage | compared field by field | ✅ **absent** |

### 1a. The vendor-dirt result, stated plainly — this was the point of the row

**`Content/FogArea/**` is CLEAN. Nothing to report, nothing staged, nothing reverted.**

⛔ I did **not** settle this on `git status` alone. `git status` can answer from the stat cache, and
the artist's own warning — *verify by oid, never by size* — applies to a clean answer too. I walked
**all 27 tracked files**, extracted each committed **LFS pointer oid** and compared it to the
**disk `sha256`**:

```
FogArea vendor oid audit: TOTAL=27  NON-LFS=0  MISMATCH=0
```

⇒ The load-dirties-package hazard **did not fire**, and the artist's autosave-disable is why.

### 1b. Read-back of the shipped asset, and the leak check on the parent

| property | `BP_SiegeFog` (child) | `BP_FogArea` (vendor) |
|---|---|---|
| `general Data.density` | **5.0** | **1** |
| `bBoxShape` | **true** | **false** |
| `maxDrawDistance` | **0** (never cull) | **25000** |
| `general Data.wInd World Space` | **true** | **false** |
| `mode` / `material Mode` | Base / Dynamic | — |

⇒ Every override landed on the **child**, and **none leaked upward**. This corroborates the oid
audit from the other side: the pack is unchanged in **bytes** *and* in **behaviour**.

`list_variables` ⇒ **`[]`**. The visual declares no state, so it cannot own the fog timer, the
duration, or a mirrored "is fog up" flag. The artist's architecture claim is confirmed independently.

### 1c. LFS join, and `L_Arena`

```
committed pointer  oid sha256:347624c91f9863c36883ab40a228015b78034d53536e123e90828d645f5e81a2  size 37947
working tree                  347624c91f9863c36883ab40a228015b78034d53536e123e90828d645f5e81a2  37947 B   JOINED
```

Committed as a **3-line pointer**, not a raw blob (`git cat-file -p HEAD:<path>` re-read after the
commit).

⛔ **`L_Arena` was dirty in memory and was NEVER saved.** On-disk hash checked **before and after**
the editor close: **`1f78419d…5622`** both times — the pinned *new* baseline, not the retired
`9ccd54ef…0e58`. The close discarded the in-memory state, which is the correct outcome.

### 1d. Camera coordinates found in the handoff (clause 3)

Six poses, all with FOV 90, all 2764 × 828:

| id | location | rot (pitch, yaw, roll) |
|---|---|---|
| PNG-control A/B | `(0, 0, 1200)` | `(-5, 0, 0)` |
| MEASURE rig | `(0, 0, 1200)` | `(30, 0, 0)` |
| **E1** + E1-control | `(0, 0, 250)` | `(-2, 0, 0)` |
| **E2** (into the sun) | `(-6000, 6000, 400)` | `(8, -35, 0)` |
| **E3** (arena corner) | `(24000, 11000, 1200)` | `(-6, 200, 0)` |

✅ **CONFIRMED: none is the forbidden vantage `x −20607.8, y 0, z 98.15` / yaw `180` / FOV `90`.**
The nearest in `x` is E3 at `+24000` — opposite sign and 44,000 uu away.

⛔ **I took NO fog measurement of my own, deliberately.** The artist recorded consecutive
identical-setting captures returning **55.8%** and **100.8%**; convergence is monotone from below,
so a single-shot reading is indistinguishable from a converged one. A one-shot number from me would
have been a *fresh* instrument lie dressed as corroboration. Clause 3 asks whether the evidence is
**checkable**, and it is: the coordinates are printed, the control was run first, and the residuals
are declared. That is what I certified — not the pixel values.

---

## 2. COMPILE

```
Result: Succeeded
```

⛔ Parsed **from the log**, never from `$LASTEXITCODE` (the raw exit was `0`, which it also is on a
failed build). `Result: Failed` occurs **0** times; `error C`/`error LNK`/`warning C` occur **0**
times.

⚠️ **Worth naming:** the compile's first three units were
`SiegeFogReachSeamTest.cpp`, `SiegeFogRetentionWiringTest.cpp`, `SiegeBrightSunTest.cpp` — i.e. it
necessarily built **`TASK-1044`'s unstaged edits**, because they are dirty in the tree and a compile
reads the tree, not the index. That is unavoidable and not a defect; it is also the reason §4 exists.

---

## 3. THE SUITE — EXECUTED, AND HONESTLY REPORTED

Baseline is **`489 / 0`** at `12b8707`.

**Static census first, so I would know what to expect rather than discover it:**

| census | value |
|---|---|
| `IMPLEMENT_SIMPLE_AUTOMATION_TEST(` at `HEAD` (`12b8707`) | **489** |
| same, working tree | **489** |
| delta on each of the three dirty `Tests/` files | **0 / 0 / 0** |

⇒ `TASK-1041` changed test **bodies**, not test **counts**. So `489 / 0` was the correct expectation
and **any** move is a real finding, not an accounting artefact.

**What actually happened:**

| measure | value |
|---|---|
| `Result={Success}` | **223** |
| `Result={Fail…}` | **0** |
| distinct `Result=` values in the whole log | **`223 Success`, and nothing else** |
| `Test Started` / `Test Completed` | **224 / 223** |
| ⛔ **outcome** | **HUNG at test 224 — never finished** |

---

## 4. 🚨 THE BLOCKER — `Siegebound.Fog.TheUnitSideCeilingIsOneDoorAndAmbushIsExemptByConstruction` HANGS

**Home:** `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeFogRetentionWiringTest.cpp:624`
**Owner:** ⭐ **`TASK-1044`** (the file is one of its three) · **gated by** `qa/TASK-1042.md`

### How I established it, and the wrong answer I nearly accepted

The first run also filled the log with `LogAudioMixer` device-swap timeouts — a real Windows audio
device change mid-run, spinning at `StreamState=5` every 5 seconds forever. **That is a very
attractive false cause**, and "the machine's audio flipped" would have let me re-run and move on.

⛔ **The timeline refutes it.** The test started at `19:38:08.897`; the audio event is at
`19:39:39` — **90 seconds later**. Neighbouring tests complete in **~20–35 ms**. The test was
already hung for a minute and a half *before* the audio ever misbehaved. The audio spam is a
**second** symptom of an idle machine, not the cause.

### The two experiments that settled it

| # | experiment | result |
|---|---|---|
| 1 | **re-run the same test alone, `-nosound`** (removes the confound entirely) | ⛔ **HUNG again** — started `19:50:22`, nothing by `19:53:08`, **zero** audio lines |
| 2 | **positive control** — a *different* single test, same file's group, same flags | ✅ **`Result={Success}` in 19 ms**, process exited cleanly |

⇒ Single-test invocation works. The framework works. **This one test hangs**, reproducibly.

### Attribution

The test **existed at `12b8707`** (census delta 0) and `TASK-1040` measured it **Success** there.
The only `Source/` change since is `TASK-1041`'s three test files. Its diff on this test adds:

```cpp
const FString Between = (DispatchIndex != INDEX_NONE && ReturnIndex != INDEX_NONE && ReturnIndex > DispatchIndex)
    ? UpdateStateBody.Mid(DispatchIndex, ReturnIndex - DispatchIndex)
    : FString();
…
&& CountOccurrencesInCode(Between, TEXT(";")) == 1);
```

⇒ **The hang arrived with `TASK-1041`'s edit.**

⚠️ **I am NOT naming a mechanism, and that is deliberate.** I read both helpers and neither is an
obvious non-terminating loop — `CountOccurrencesInCode` advances `From` by `NeedleLength ≥ 1` every
iteration, and `ExtractFunctionBody` contains no loop at all. **I could not explain it from
reading, so I am reporting the reproducible symptom rather than shipping a guess** that would send
the programmer to the wrong line. `SC-§39`-shaped: a confident wrong diagnosis is worse than an
honest "reproduces here, cause unknown".

### Why this did not block THIS commit

Every `Source/` path in `ef2c901` is **`HEAD`'s own content** — I staged no code. The hang lives
**entirely in unstaged edits I was ordered to exclude**, and a Blueprint asset cannot influence a
source-text-scanning C++ test. So the tree *this commit describes* is the `489 / 0` tree; the tree
I *measured* additionally carried `TASK-1044`'s work.

⛔ **I did not, and could not, remove those edits to get a clean measurement** — `checkout` /
`restore` / `stash` / `reset` / `clean` are all forbidden, correctly. So I report both facts side by
side and overwrite neither. **I am not claiming an executed `489 / 0` for this commit.**

---

## 5. PATHSPEC

Derived with `--untracked-files=all` and the `GitClaudeUnrealTest/` prefix, intersected against the
**Standing Exclusion Registry**, verified with `git diff --cached --name-only`.

⛔ **`SC-§77a` honoured:** the index already held `BP_SiegeFog.uasset` — the **editor** staged it
(`Provider=Git`), the artist ran no Git write. I **read the index first, treated it as deliberate,
and ADDED to it.** No `git reset` "to start from a clean pathspec".

**STAGED — exactly 3 paths:**

1. `GitClaudeUnrealTest/Content/Blueprints/BP_SiegeFog.uasset` — the subject
2. `GitClaudeUnrealTest/.claude/pipeline/handoffs/TASK-841-artist.md` — its handoff
3. `GitClaudeUnrealTest/.claude/pipeline/handoffs/TASK-1040-buildmaster.md` — clause 6, swept silently

Clause 6 satisfied exactly: **exactly one** untracked `*-buildmaster.md` was present. Two or more
would have been a finding.

### 🚨 NAMED-AND-LEFT — never silently omitted

| path | why |
|---|---|
| `…/Tests/SiegeBrightSunTest.cpp` | ⛔ **clause 5a** |
| `…/Tests/SiegeFogReachSeamTest.cpp` | ⛔ **clause 5a** |
| `…/Tests/SiegeFogRetentionWiringTest.cpp` | ⛔ **clause 5a** |

All three are **`qa-passed` under `TASK-1042` and belong to `TASK-1044`**. My gate is **WAIVED**
because this row is the gate *for art*. Sweeping them here would have shipped `qa-passed` code
**ungated** and left `qa/TASK-1042.md` **cited by no commit at all** — `TASK-964` reproduced by the
one route nobody watches: **a correct derivation**. My derivation *did* reach all three, exactly as
the row predicted.

| path | why |
|---|---|
| 🧑 `.claude/agents/qa-reviewer.md` | **Jonathan has not ruled** (`TASK-1035`). A permission change is neither code nor art and **never** rides a code commit. Staging it is a hard failure of this row. |
| `testvideo/**` | never committed, ever |

### Left for `TASK-1044`, its designated host (clause 4) — declared, not dropped

`CONVENTIONS.md` · `TASKBOARD.md` · `handoffs/TASK-1041-programmer.md` · `qa/TASK-1042.md`

⭐ **`qa/TASK-1042.md` is left deliberately, and it is the same argument as clause 5a read forwards:**
a verdict belongs in the commit that ships the code it gates. Taking the *report* here while leaving
its *subject* behind would have split the gate from the gated — the mirror image of the failure 5a
names. `TASK-1044`'s clause 4 provisions for exactly this ("whichever are STILL dirty at your
instant"), so nothing is lost.

⚠️ Board dirt remains after this commit (my own `status:` edit). Per `TASK-1044` clause 4a that is
the **expected end state**, not a defect.

---

## 6. FINDING FOR THE MANAGER — the documentation contradiction (NOT fixed here)

`FOG-§6` and `FogVolume.h:95-105` pin `/Game/Blueprints/BP_SiegeFog` as a Blueprint child of
**`AFogVolume`**. The asset was built as a child of **`BP_FogArea`**, per this row's clause 1 and the
live dispatch — and **clause 1's expectation is the correct one; it passed.**

A Blueprint has **one** parent, so `FogVolume.h`'s `CoreRedirects` paragraph is **now false**: there
is no Blueprint child of `AFogVolume`, those five `EditDefaultsOnly` tunables serialise only into the
C++ CDO, and the hazard it warns about does not currently exist.

⛔ I edited **nothing** — not `FogVolume.h`, not `CONVENTIONS.md`, not `FOG-§6` — and I did **not**
treat the header's claim as evidence against the asset. Routed to the manager.

---

## 7. FENCES HONOURED

⛔ No `Source/` file edited · no art authored · no `git push` · no `--no-verify` (no hook fired) ·
no `checkout --` / `restore` / `stash` / `reset` / `clean` · `Content/FogArea/**` read-only and
untouched · `L_Arena` never saved · `qa-reviewer.md` untouched and unstaged · the three `Tests/`
paths untouched and unstaged · exit codes never trusted over logs.

⚠️ **Disclosed against myself:** I **closed the editor** (standing grant, 2026-08-30) after the
integration check, which is what `TASK-1044`'s clause 6 expects to find. The close **discarded**
`L_Arena`'s dirty in-memory state — verified harmless by hashing the package before and after.
I also added **`-nosound`** to the two diagnostic probes in §4 (not to the main run), solely to
remove the audio confound; it changes nothing these source-text tests assert.
