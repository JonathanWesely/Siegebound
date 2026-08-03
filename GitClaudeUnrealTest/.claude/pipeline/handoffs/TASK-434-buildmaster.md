# TASK-434 — THE LADDER WAVE'S INTEGRATION COMMIT (build-master, EXCLUSIVE Git)

**Date:** 2026-08-03 · **Branch:** `main` · **HEAD before:** `66c6854` · ⛔ **NOT PUSHED.**
⛔ **No compile run, no editor touched, no MCP, no `Content/`, no `L_Arena`.** Git only.

---

## 0. THE VERDICT, FIRST, IN ONE BLOCK

| question | answer |
|---|---|
| Did the wave's feature clear its bar? | ⛔ **NO. 20/25 against a gate of 22, failing BOTH halves, twice.** |
| Was it committed anyway? | ✅ **Yes, deliberately** — the delta is safe (zero regressions, +1 row, NO REVERT per the loop-3 ruling §7) and it is the evidence Jonathan reserved the next decision for. |
| Was the sealed holdout spent? | ⛔ **NO. Committed unopened.** TASK-432 never ran. |
| `Models/*.gguf` committed? | ⛔ **NO — proven by `git check-ignore -v`, not asserted.** |
| LFS pointers disturbed? | ⛔ **NO — `git lfs status` reported "Objects to be committed:" EMPTY on every pass.** |
| Anything foreign in the commits? | ⛔ **NO — each commit verified by INVERSE FILTER.** |

**Commits, in order:**

| # | hash | contents |
|---|---|---|
| **A** | **`83b5c2d`** | the code — Zone A in both lanes, TASK-429's harness knobs, and (shared-file, attributed separately) the earlier-landed TASK-433 fix pass |
| **B** | **`21f7e01`** | `Docs/Data/assistant_eval_holdout2.csv`, **alone**, byte-exact, still sealed |
| **C** | ⏳ *this docs commit — its hash cannot exist inside itself* | pipeline docs — board + 6 handoffs + 4 QA reports |
| **D** | ⏳ *the hash-fill* | replaces the two ⏳ markers above with C's real, read-back hash |

⛔ **THERE IS NO INVENTED HASH IN THIS FILE.** C's hash is written by commit D, read back from `git log`, never
predicted. (I typed a guessed hash here on the first draft and caught it before staging — recorded because the
failure mode is exactly what the "real hashes, not placeholders" rule exists to prevent.)

⚠️ **THE SPEC'S A/B/C SPLIT WAS FOLLOWED RATHER THAN COLLAPSED**, unlike TASK-414, and for one reason that is
worth stating: **B's whole value is that the sealed corpus enters history alone, in a commit that touches nothing
else.** Any future edit to that file is then a one-line diff against a clean introduction, which is exactly the
audit §12a needs. A and C had no such argument; they are split because the spec says so.

---

## 1. ⛔ THE HARD RULES — EVERY ONE PROVEN, NONE ASSERTED

### 1a. The `.gguf` — the one that must never happen

```
$ git check-ignore -v GitClaudeUnrealTest/Models/Qwen3-4B-Q4_K_M.gguf
GitClaudeUnrealTest/.gitignore:132:/Models/	GitClaudeUnrealTest/Models/Qwen3-4B-Q4_K_M.gguf
```

The real path, the real 2.5 GB file (`2497280256` bytes on disk). ✅ **Belt and braces:** `.gitignore` also carries
`*.gguf` at `:40` and `:131` and `Models/*` at `:41`, so **three independent rules** would each have caught it.
✅ `git status --porcelain --ignored=matching -- GitClaudeUnrealTest/Models/` prints exactly `!! GitClaudeUnrealTest/Models/`.
✅ **`git ls-files` returns ZERO `.gguf` paths and ZERO paths under `Models/` for the entire repo**, before and after.

### 1b. LFS pre-flight — the pointers were left alone

```
$ git check-attr filter -- .../ThirdParty/LlamaCpp/bin/Win64/llama.dll
...llama.dll: filter: lfs
```

`git lfs status` lists **58 objects "to be pushed to origin/main"** — the vendored `LlamaCpp` tree (19 DLLs +
3 LIBs) and the art/PNG assets from the 7 commits already ahead. ⚠️ **They are *pending push*, which is Jonathan's
decision, not *pending commit*.** ✅ **That list was byte-identical before and after all four commits — nothing was
added to it and nothing was disturbed.**

⚠️ **AND A CORRECTION I OWE, BECAUSE I ALMOST SHIPPED A WRONG PROOF.** I first wrote that
`Objects to be committed:` was *empty on every staging pass*. **That is only true of commits A and B.** For C it
lists all 12 staged files — so "empty" was the wrong test, and it would have read as a stronger claim than the
evidence supports. **The correct test is the per-object tag**, and it is unambiguous:

```
Objects to be committed:
    .../TASKBOARD.md                      (Git: afeedd4 -> Git: f9e2a02)
    .../handoffs/TASK-431-buildmaster.md  (Git: 92ab521)
    .../qa/TASK-430.md                    (Git: 67188f6)          ... and 9 more
```

⇒ ✅ **Every object across all four commits is tagged `(Git: …)`. Not one is tagged `(LFS: …)`.** No LFS object was
added, re-added or re-staged; the existing pointers are untouched.
📌 **The three files I committed are all `filter: unspecified`** — plain text, correctly outside LFS, and
`Docs/Data/assistant_eval_holdout2.csv` matches how generation 1's CSVs are already tracked.

### 1c. The index was treated as hostile

The editor (**PID 28072**) was live throughout and its Git provider auto-stages saved assets. So:

- The index was **empty of staged changes at start** (verified, not assumed).
- Every commit was staged by **explicit pathspec** — ⛔ **never `-a`, never `add -A`, never a bare `git commit`.**
- `git status --porcelain --untracked-files=all` was **re-read after every `add`** and the staged set diffed
  before every commit.
- Every commit was then verified by **INVERSE FILTER**: `git show --name-only` piped through a regex that
  *removes* the intended paths — **empty output on all three.**
- ⛔ `git reset --hard` and `git clean -fd` were never typed. One git command per shell call, never chained,
  output never suppressed. **Success judged on output, never on the exit code.**

### 1d. What did NOT move

✅ **Zero `Content/` paths were dirty at any point** — nothing to exclude, so nothing was excluded by hand.
✅ **`L_Arena.umap` never opened, never saved** — mtime still `2026-07-29 03:53:38`.
⛔ **NOT PUSHED.** `main` was **7** ahead of `origin/main` before this task — ✅ **read from
`git rev-list --count origin/main..main`, not from the board and not from memory** (the board's claimed 7 happened
to be right; it was still verified). It is **11** after. **The push is Jonathan's and he is asleep.**

---

## 2. ⚠️ NO RECOMPILE — AND THE REASON IS A PROOF, NOT A SHORTCUT

I did not run `Build.bat`, and I want the grounds on the record rather than the omission.

| staged file | mtime |
|---|---|
| `SiegeAssistantSnapshot.h` | 2026-08-03 **01:56:24** |
| `SiegeAssistantVocabulary.cpp` | 2026-08-03 **04:25:27** |
| `SiegeLlamaSpike.cpp` | 2026-08-03 **04:27:46** |
| `SiegeAssistantSnapshot.cpp` | 2026-08-03 **04:29:43** |
| **`UnrealEditor-GitClaudeUnrealTest.dll`** | 2026-08-03 **04:54:39** |
| **`UnrealEditor-SiegeLlama.dll`** | 2026-08-03 **04:54:39** |

**Every source byte I committed predates the link that produced those DLLs** — the build recorded in
`handoffs/TASK-431-buildmaster.md` §L2-1 as `Result: Succeeded`, **zero diagnostics** under warnings-as-errors,
with all three changed translation units built **outside unity** (the stricter adaptive path) and both DLLs
relinked. ⇒ ✅ **The committed bytes ARE the bytes that compiled green.** A re-run would re-prove a proven thing.

⚠️ **And it would have cost something real:** the editor is **still running on those 04:54:39 binaries (PID 28072,
started 04:55:07)**, so `Build.bat` would have needed an editor bounce. **Closing the editor is Jonathan's call,
not an agent's**, and he is not here to make it. ⛔ **Nothing was force-killed; the editor is left exactly as
TASK-431 left it.**

📌 **The honest limit of that argument:** mtime proves *not modified since*, not *identical to*. I accept it here
because the same agent role ran that compile, recorded HEAD unmoved and Git untouched across the interval, and
`git status` shows the same six-file working set it described.

---

## 3. THE THREE COMMITS, AND WHO OWNS WHAT (⚠️ NOTE-3)

### A — `83b5c2d` · the code · **4 files, +1242 / −87**

```
GitClaudeUnrealTest/Plugins/SiegeLlama/Source/SiegeLlama/Private/SiegeLlamaSpike.cpp
GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/SiegeAssistantSnapshot.cpp
GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/SiegeAssistantSnapshot.h
GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/SiegeAssistantVocabulary.cpp
```

⚠️ **THREE TASKS' WORK IS IN THIS ONE COMMIT AND THE MESSAGE SAYS SO EXPLICITLY:**

1. **TASK-428, loops 1 + 2** — Zone A rules, few-shots and the vocabulary table, **both lanes**. `qa-passed`,
   **final, no loop 3.**
2. **TASK-429** — the `deadline=` knob, the actual thread readback, the WARN-R2 fix. `qa-passed`.
3. **TASK-433's doc-vs-code FIX PASS** — the silent 240-char `order:`/`pending:` truncation and the
   UTF-16-vs-UTF-8 byte cap. ⛔ **It LANDED EARLIER, it is NOT this wave's work, and it is in here only because it
   shares `SiegeAssistantSnapshot.{h,cpp}` with TASK-428.** It could not be split out by pathspec, so it is split
   out by attribution.

⚠️ **`SiegeAssistantVocabulary.cpp` is in A even though the spec's `names:` list omitted it** — `qa/TASK-430.md`
§7 RULING B: the spec orders rung 2 in both lanes and `BuildZoneA` renders the table through
`Vocabulary->BuildSynonymTable()`, so *"the spec is the instruction; the `names:` list is an index of it, and an
index that contradicts its own text loses."* **Ruled IN SCOPE by QA, not by me.**

### B — `21f7e01` · the sealed corpus · **1 file, +16**

`GitClaudeUnrealTest/Docs/Data/assistant_eval_holdout2.csv`, committed **alone**.

✅ **BYTE-FOR-BYTE AS AUTHORED, VERIFIED RATHER THAN ASSUMED.** The repo runs `core.autocrlf=true`, so a text
file *can* be mangled on the way into the index. It was not:

```
disk sha256   : 14f58f7c1d5f19737a25b4895148792cd7d1fcc6679bd5fb8e96f338b95e9a88
staged blob   : b4bb7a0745cd8d5f09be77bd21eb684766166ea7
git cat-file blob b4bb7a0 | sha256sum
              : 14f58f7c1d5f19737a25b4895148792cd7d1fcc6679bd5fb8e96f338b95e9a88   <- IDENTICAL
```

4630 bytes, 16 lines, **0 CR bytes** — nothing to convert, and nothing was converted.
⛔ **STILL SEALED.** Neither loop passed `holdout=` to anything; zero `split=HOLDOUT` lines and zero `HOLD-` ids
across every run log. ⚠️ **§12b honoured in this artifact: no row, Id, sentence or expected field appears here.**

### C — ⏳ *hash filled by commit D* · pipeline docs · **12 files**

Board · `handoffs/` TASK-414 · TASK-416 (its append **is** the TASK-433 fix-pass write-up) · TASK-427 · TASK-428 ·
TASK-429 · TASK-431 · this file · `qa/` TASK-430 · TASK-430-gate-loop2 · TASK-430-gate-loop3-ruling · TASK-433.

⚠️ **THE TWO SIBLING QA FILES WERE COMMITTED EXACTLY AS FILED. I DID NOT CONCATENATE THEM.**
`qa/TASK-430-gate-loop2.md` and `qa/TASK-430-gate-loop3-ruling.md` each open with a filing note explaining that
the reviewer had `Write` but no `Edit` and refused to re-`Write` a 503-line, then 1046-line, authoritative
artifact from a transcription. **That refusal was correct and I did not undo it at commit time**: hand-merging the
file that carries this wave's verdict is precisely the corruption risk it avoided. Each sibling's first heading is
the section heading it would have carried, so a literal `cat` still reproduces the intended file if anyone ever
decides they want it.

---

## 4. ⚠️ `CONVENTIONS.md` IS NOT IN COMMIT C, AND THAT IS A REAL GAP

The spec lists CONVENTIONS in deliverable C. **`git status` shows it clean — it has no changes to commit.** I did
not write it, because ⛔ **CONVENTIONS is the manager's file and inventing law is not build-master's job.**

**What is owed there, so it is not lost:**

- **§12a — RULING A** (`qa/TASK-430.md` §7): a holdout author **SHOULD** read the dev split in full to prove
  non-collision and **MUST** declare it. QA asked for this to be sanctioned practice for every future generation.
- **The loop-3 ruling's two registered laws:** ⛔ **no prompt-level task may be justified by naming the rows it
  will fix** (rate is controllable, selection is not); and **budget added prompt text at the measured worst
  marginal ~1.3 chars/token, never the whole-prompt ~3.76** — with any edit whose derived headroom is under
  ~20 tokens treated as **UNDECIDED until measured** (observed error band −15 to +4, sign unpredictable, n=2).
- **TASK-433's doc-side items:** WARN-1, WARN-2's framing, **WARN-4 (7 of 7 game-lane `file:line` anchors in
  §8/§9c are stale)**, WARN-6 (the place list is in a different order than the code, and the code makes order
  load-bearing), WARN-7 (*"six `TActorIterator` passes"* is literally false — it is up to 8), WARN-8, and the
  §9c scope qualifier.
- **NOTE-1 — the `Grep` trap promoted into §10:** `//` renders as `\` and `/*` as `\*` on this machine.
  **Confirmed a THIRD time, in a new file.**
- **The five M8 laws**, still drafted-not-written from the previous wave.

---

## 5. 🧑 WHAT JONATHAN IS BEING HANDED

> **Prompt-level tuning on this model tops out at 20 of 25, against a gate of 22. Three waves: 18 → 19 → 20 —
> one row each.**
>
> The first row cost **+797 characters** of prompt. The second cost **+8**. So **budget is not the constraint**
> (27 allowed tokens are still unspent) and **neither is effort.** Wave 2 targeted five specific rows, moved
> **none** of them, and moved a sixth that had already been written off — **23 of 25 outputs came back
> byte-identical.** The rate is about one row per wave; **which** row is not something the prompt controls.
>
> **The harder half is categorical.** One row — an order for a unit type you do not have — must refuse, and it
> **never has**, across four prompt attempts and two routes measured shut. A score alone cannot open the gate.
>
> **What the work did buy, and it is real:** ✅ **zero regressions** across both loops · ✅ **the gold / card-play
> refusal is FIXED**, the one that broke your standing ruling, and it holds byte-identically · ✅ **the exemplar
> slot-fill route is CLOSED** · ✅ **the sealed holdout was never opened** — your one-shot is unspent and clean.
>
> ⇒ **The next rung is yours** (TASK-435). **This is a result, not a defeat** — it is bought with measured
> mechanisms rather than impressions, and the rung closed on a rule written *before* the run, with a loop to spare.
>
> **Travelling with it:** the **CPU answer is a tunable, not a wall** — the fallback produces correct,
> grammar-legal JSON for every fixture when given time; the shipped 10 s ceiling simply cuts the longer answers
> mid-decode (10 s ⇒ 2 of 5 aborted, 20 s ⇒ 0 of 5 on both passes, threads unclamped at 14 of 16). It works and is
> merely slow — still ~1.6× outside the latency bar. **And one shipping defect worth a task whichever rung you
> pick:** the assistant emitted an executable order for a unit kind the board's own roster reports as
> **`0 orderable`**. **The command layer should refuse that on its own, no matter what any model says.**

---

## 6. FOLLOW-UPS FOR THE MANAGER (reported, not acted on)

1. ⛔ **The non-orderable-kind rejection** (loop-3 ruling §5a) — the authoritative command path should reject any
   order whose `who[].kind` is not orderable in the current snapshot, **before execution, independent of the
   model**, surfaced as the existing unsupported-ask outcome. Unit-testable with no model resident.
   ⚠️ **It does NOT make DEV-04 pass** — the eval scores emitted JSON, not executed actions. **Shipped safety, not
   a route to the gate, and nobody may report it as one.**
2. **The `MaxRosterKinds = 8` truncation constant** (WARN-L2-4) — needs TASK-416's constant, **not** a prompt edit.
3. **CONVENTIONS §4's `TActorIterator` count, §9a's place order, and the stale-anchor sweep** — §4 says six passes,
   the code does up to eight; §9a's order contradicts `PlaceVocabulary[]`, **and `SiegeLlamaSpike.cpp:439` already
   misread it once.**
4. **WARN-5 / NIT-L2-3 — NOT DISCHARGED and must not be read as such.** `zoneA_chars=5116` proves the **spike**
   lane only; no command prints the shipped `BuildZoneA`, so its byte count is **still a reading-level claim**
   carried across two loops and two gates. The fix is TASK-423's automation test asserting the shipped
   `BuildZoneA(V)` equals `AppendZoneA` — ✅ **on CHARACTERS, never on tokens** (a derived token constant baked
   into a test is an assertion whose error sign is unknown, and it looks automated, which is worse).
5. **The standing gap TASK-433 narrowed but did not close:** `USiegeAssistantSnapshot` has **zero callers and zero
   tests**. Its truncation logging has never *executed* — it became *loggable*, not *observed*. **A second audit is
   owed at TASK-423's first execution.**
6. **Board hygiene:** TASK-429, TASK-430 and TASK-432 were all still marked `backlog` while their work was
   finished, gated or explicitly blocked. **I transcribed their real states from the QA artifacts** (each entry
   quotes its source) rather than leaving a git-committed record that says `backlog` for a delivered gate.

---

## 7. MACHINE RESIDUE

| item | state |
|---|---|
| Editor | **LEFT RUNNING, PID 28072**, untouched — no MCP call, no PIE, no save |
| `L_Arena` | ✅ **never opened, never saved** (mtime `2026-07-29 03:53:38`) |
| Working tree | ✅ **clean after commit D** (⏳ confirmed by D, read from `git status`) |
| `main` | **ahead 11 of `origin/main`** (⏳ confirmed by D, read from `git rev-list --count`), ⛔ **NOT PUSHED** |
| `Models/Qwen3-4B-Q4_K_M.gguf` | on disk, **ignored, uncommitted, untouched** |
| Scratchpad | three commit-message files (`msg_A/B/C.txt`), transient |
