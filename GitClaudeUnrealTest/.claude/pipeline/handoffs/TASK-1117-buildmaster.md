# TASK-1117 — [GFX-SHIP-1] build-master handoff

⛔ **THIS FILE IS BORN OUTSIDE ITS OWN COMMIT** (`TL-§5e` cl. 7 / the table's *"YOUR OWN
`handoffs/TASK-###-buildmaster.md` ⇒ MINTED ⇒ STRUCTURALLY EXCLUDED"* row). It carries `42734b7`,
which did not exist when the commit was made. **Expected, bounded at one, never amended — the NEXT
host takes it under row 1.** My row names ⭐ `TASK-1124` as that sweep.

**Commit: `42734b7`** · **HEAD before: `a521687`** (⭐ `TASK-937`) · `main` **3 ahead of `origin/main`**
· ⛔ **NOT PUSHED.**

---

## 1. COMPILE

```
Result: Succeeded
```

⛔ **Parsed from the log, never from `$LASTEXITCODE`.** Every run returned `EXITCODE=0` and that fact
carries **no information** (the `Build.bat` exit-code law). **Smart App Control did not fire** — no
`0x800711C7`, no ~2 s death.

**17 builds, all green** (`grep -h "^Result:" build-*.log | sort | uniq -c` ⇒ `17 Result: Succeeded`):
1 baseline + 15 mutation builds + 1 final post-restore build.

🚨 **THE HONEST GAP, AND `SC-§99` FORBIDS ME FROM CLOSING IT THE EASY WAY.** `SC-§99` (ratified in
`CONVENTIONS.md` **today**, and it is in this very commit) says a parser that has only ever seen green
is unproven, and that the cure is to **harvest** an unintended failure — cl. 4: ⛔ *"do not manufacture
one."*

- ⛔ **I got no unintended failure.** All 15 mutation edits applied cleanly on the first attempt
  (see §3's guard), so **no `Result: Failed` was produced this run**, and no failed log survived in
  the session scratchpad from ⭐ `TASK-937` to borrow. **I did not break a build to satisfy the clause.**
- ✅ **What I did instead, named as the weaker instrument it is:** I fed the *parse expression* a
  synthetic `Result: Failed (OtherCompilationError)` log and confirmed it rejects it. **That is a
  control on the READER (`SC-§96`), not on the BUILD (`SC-§99`).** It proves my `grep` can say
  "Failed"; it does **not** prove UBT would have told me.
- ⇒ ⚠️ **The strongest true statement: my 17 greens rest on a parse expression proven alive but on a
  toolchain path never observed failing in this run.** The *identical* expression was harvested
  against a real `Result: Failed` by ⭐ `TASK-937` earlier today (its §1) — **that is a citation, not
  my measurement** (`SC-§40` cl. 1).

**`Source/` was confirmed quiet before the first build** (UBT globs every `.cpp`): newest write
anywhere under `Source/` was `1788838334` (⭐ `TASK-937`'s own post-restore copies), **~15 minutes
before** my first build, and nothing landed during the run.

---

## 2. SUITE — EXECUTED AND BOUNDED (`SC-§87`), NOT CENSUSED

Runner: `UnrealEditor-Cmd.exe <uproject> -ExecCmds="Automation RunTests Siegebound;Quit" -nullrhi
-unattended -nopause -nosplash -NoLiveCoding -log -abslog=…`, wrapped in
`timeout --signal=KILL 900`. **No run was killed.**

| | baseline (shipped tree) | final (post-restore, = what is in `42734b7`) |
|---|---|---|
| `Result={Success}` | **517** | **517** |
| `Result={Fail}` | **0** | **0** |
| `Test Started` / `Test Completed` | 517 / 517 | 517 / 517 |
| log | 6,560 lines / 1,108,025 B | — |

⛔ **`N` is non-zero and the log carries real `Test Started`/`Test Completed` traffic** (`SC-§95`: a
run that starts zero tests is indistinguishable from a green one). `Started == Completed` ⇒ **nothing
hung.**

### RECONCILIATION — ZERO UNEXPLAINED, AND THE POINT OF IT

| source | Δ | evidence |
|---|---|---|
| executed baseline | 498 | ⭐ `TASK-1104` @ `eeb29c4` |
| ⭐ `TASK-931` | **+2** | shipped in `a521687` |
| ⭐ `TASK-1132` | **+1** | shipped in `a521687` |
| ⭐ `TASK-1113` — **mine** | **+16** | 16 distinct `Siegebound.Graphics.*`, all observed **by name** |
| | **= 517** | **measured 517. Residual 0.** |

⭐⭐ **`517` WAS NOT `a521687`'s NUMBER AND IS NOW `42734b7`'s.** ⭐ `TASK-937` measured the same 517
with my 16 tests sitting **in the working tree but outside its commit**, and said so. **This commit is
what earns them.** A future host reconciling against `517` may now read it as `HEAD`'s.

⛔ `306` appears nowhere as a suite number. ⛔ `514` was a **census** (`IMPLEMENT_SIMPLE_AUTOMATION_TEST`
× 16 + 498) and was labelled UNMEASURED by both the author and the gate; the executed answer is **517**,
and it differs from `514` only because `514` was computed against a `498` that predates ⭐ `TASK-931`
and ⭐ `TASK-1132`. **Both numbers were right about their own arithmetic; only one was executed.**

### HERMETICITY (the gate's L1.6 item 6)

`Saved/Config/WindowsEditor/GameUserSettings.ini` = `fba8537bdfbe86c7b071b6dc136e15fff615455d6fd2e6b8660edf7dca2899a9`
**before** the first run, **after** the baseline run, and **after** the final run — byte-identical
across **18 suite runs**. ⇒ **suppression did not leak; no test changed the editor's resolution or
wrote the developer's real ini.**

⚠️ **CORRECTION TO MY OWN COMMIT MESSAGE, RECORDED HERE RATHER THAN REWRITTEN** (`SC-§40` cl. 9 —
records are committed, never re-authored): the message says the ini held *"across all 17 runs."*
**17 is the BUILD count. The suite-run count is 18** (1 baseline full + 1 filter control + 15
mutation runs + 1 final full). The claim is true and *understated* by one; the figure is wrong.

---

## 3. THE MUTATION SET — **15 EXECUTED, EACH ITS OWN COMPILE, EVERY ONE RESTORED BYTE-EXACT**

**Method.** Pristine copies of all three files taken **before** any mutation and `sha256`-recorded.
Every mutation is a **byte-level** edit applied by a helper that **hard-fails unless its marker occurs
exactly once** — so a `sed` that silently no-ops can never masquerade as *"a mutation that did not
redden"* (`SC-§96`: make the instrument prove it spoke). After every mutation the file is restored
**by copy from pristine** and re-hashed. All 15 edits were **dry-run against copies first**, which is
why none of them produced a broken build — and therefore why §1 has no harvested control.

**Pre-mutation baseline hashes (re-verified identical at the end):**

```
ab14f14d892acefe9be09e94f4c93c1aa20746333b75e32da6ad14f16022c694  SiegeGraphicsSettingsSubsystem.cpp
de26d7d4706ad2eb8aa0945a78ab4b21d5fdae5a0b99e05372b6f5accf449faa  SiegeGraphicsSettingsSubsystem.h
b98fb3c97f76f84a4ef44c5bb6edcd2d7d821c623993bf2628b489cb42ef1b85  Tests/SiegeGraphicsSettingsTest.cpp
```

**Final hashes: identical, all three.** Only the `.cpp` was ever mutated; the `.h` and the test were
never written. **`RESTORE OK` printed after every single cycle**, hash-compared, not assumed.

**Instrument.** Each mutation ran `Automation RunTests Siegebound.Graphics` (16 tests) rather than the
full 517 — chosen deliberately so an **over-broad** red inside the Graphics group is visible, which is
the class ⭐ `TASK-937` reported. ✅ **The filter was positive-controlled first**: on the unmutated
tree it returned **16 Started / 16 Success / 0 Fail**, so a `0 Fail` under mutation could not be a
filter that matched nothing.

### ACTUAL vs PREDICTED — 15 of 15 REDDENED. ⛔ NO SILENT SURVIVOR.

| | mutation | predicted RED | **ACTUAL** | verdict |
|---|---|---|---|---|
| **M1** | `EffectsQuality` → `VisualEffectQuality` | `CanonicalNames` | `CanonicalNames` (2 assertions) | ✅ **EXACT** |
| **M2** | swap Shadow/Shading write branches | `GroupRoundTrip` | `GroupRoundTrip` **+ `OutOfRangeWriteClamps` + `DelegateNeverFiresOnNoOp` + `VolumetricFogFollowsShadowNotEffects`** | ⚠️ **BROADER — 4 tests, 1 predicted** |
| **M3** | delete the `CurrentLevel == ClampedLevel` early return | `DelegateNeverFiresOnNoOp` | same, only | ✅ **EXACT** |
| **M4** | `FMath::Clamp(NewLevel,…)` → `NewLevel` | `DelegateNeverFiresOnNoOp`; ⛔ `OutOfRangeWriteClamps` **stays GREEN** | `DelegateNeverFiresOnNoOp` only; **`OutOfRangeWriteClamps` GREEN** | ⭐ **EXACT — the loop-1 correction CONFIRMED BY EXECUTION** |
| **M5** | clamp on **read** in `GetResolutionScalePercent` | `SentinelIsNotClampedOnRead` | same, only (`raw read was 50, expected 0`) | ✅ **EXACT** |
| **M6** | drop the write-side clamp | `ResolutionScaleClampsOnWrite` | same, only (3 assertions; a 0.0 write really passed **0%** through) | ✅ **EXACT** |
| **M7** | delete `RequestSaveSettings`' pending guard | `SaveUnreachableFromProvisionalVideoMode` | same, only (`saved during the unconfirmed window`) | ✅ **EXACT** |
| **M8** | delete `RevertVideoModeChange`'s `ApplyResolutionSettings(false)` | `RevertActuallyAppliesTheResolution` | same, only | ✅ **EXACT** |
| **M9** | delete the `Rung <= 0.0f` skip | `FrameRateLadderSnapsAndLabels` | same, only (a tiny request snapped to **Unlimited**, not 30) | ✅ **EXACT** |
| **M10** | fog reads Effects instead of Shadow | `VolumetricFogFollowsShadowNotEffects` | same, only (4 assertions, incl. **both** cross rows) | ✅ **EXACT** |
| **M11** | Epic foliage `1.00f` → `0.90f` | `TierDFoliageDensityTable` | same, only | ✅ **EXACT** |
| **M12** | `ResolveSettings` ignores the force-null flag | `NullSettingsDegradesToFallbacks` | same, only (12 assertions) | ✅ **EXACT** |
| **M13** 🚨 | delete `AutoDetectQuality`'s refusal block | ⛔ **counter row ONLY; the `TestFalse` stays GREEN** (cl. (6) / `WARN-8`) | `AutoDetectRefusedDuringUnconfirmedVideoMode`, **EXACTLY ONE assertion: `test:977`, the counter.** `TestFalse` at `test:973-974` **GREEN** | ⭐⭐ **EXACT — `WARN-8` CONFIRMED** |
| **M14A** | drop the snapshot + conditional 2nd broadcast | 2 rows: the `+2` count **and** the name | `CustomWhenGroupsDisagree`, **2 assertions**: `test:650` (`6` expected, got `5`) + `test:656` (`ResolutionScale` expected, got `OverallQuality`) | ✅ **EXACT** |
| **M14B** | make the 2nd broadcast unconditional | 1 row: the **real preset write whose scale does not move** | `CustomWhenGroupsDisagree`, **1 assertion**: `test:623` (`4` expected, got `5`) | ✅ **EXACT** |

### THE THREE THINGS WORTH KEEPING

1. ⭐⭐ **`M13` SETTLED THE CORRECTED PREDICTION BY EXECUTION, AND IT SETTLED IT TIGHTLY.** The board's
   cl. (6) said **one** red, not two, and that **two would mean the suppression branch had moved**.
   Measured: **exactly one failed assertion in the whole suite.** The `TestFalse` immediately above it
   stayed green — because under automation the deleted guard's traffic falls into the suppression
   branch, which returns `false` anyway. ⇒ **the guard's placement above the suppression branch is
   confirmed, and the counter really is the only discriminating instrument.** The gate caught this on
   *reading*, one loop after `M4` taught it to; **the cost of the catch was one comment and the value
   was a host that knew what a single red meant instead of reporting a half-failure.**
2. ⭐ **`M4` IS NOW WITNESSED, AND THE RE-POINTING WAS RIGHT.** `OutOfRangeWriteClamps` stayed **green**
   under an unclamped facade — the engine clamps to the identical band one layer down — and
   `DelegateNeverFiresOnNoOp` caught it. **A prediction in this file was false, was corrected on
   reading, and execution has now agreed with the correction.** That is the whole return on writing
   predictions down.
3. ⚠️ **`M2` REDDENED FOUR TESTS WHERE ONE WAS PREDICTED — the same class ⭐ `TASK-937` reported, and
   it is a finding about the PREDICTION, not the code.** Swapping the two write branches does not
   merely break a round-trip: it means `SetShadowQuality` writes the **Shading** group, so
   (a) the clamp test reads a group nobody wrote, (b) the no-op law fires on writes that did land
   elsewhere, and (c) **the fog predicate — which reads `ShadowQuality` — sees a value that was never
   written to it.** ⇒ **a needle-based derivation names the test that *targets* the line; it cannot
   model a suite whose other tests happen to read the same state.** ⛔ **The mutation is caught, and
   the predicted red is among the four.**

⚠️ **CITATION DRIFT, NOT A DEFECT, NAMED SO NOBODY RE-DERIVES IT:** `qa/TASK-1114.md` § L1.2/L1.6 cites
`test:968-969` / `:970-971` (M13), `:648-649` / `:650-651` (M14A) and `:620-621` (M14B). The shipped
file's actual assertion lines are **`973-974` / `976-977`**, **`650` / `656`** and **`623`**. **Every
ROW the gate named is the row that actually reddened** — the line numbers are 2–6 off, from a
pre-final revision. The gate's reading is correct; only its coordinates drifted.

---

## 4. `§25c` — THE PATHSPEC, AND cl. 7a's STANDING CENSUS

### The commit (**8 files**, derived at my own instant, `git show --stat 42734b7`)

```
 .../.claude/pipeline/CONVENTIONS.md                |   38 +
 GitClaudeUnrealTest/.claude/pipeline/TASKBOARD.md  |  143 ++-
 .../pipeline/handoffs/TASK-1113-programmer.md      |  260 ++++
 .../pipeline/handoffs/TASK-937-buildmaster.md      |  412 ++++++
 .../.claude/pipeline/qa/TASK-1114.md               |  322 +++++
 .../Siegebound/SiegeGraphicsSettingsSubsystem.cpp  | 1285 ++++++++++++++++++
 .../Siegebound/SiegeGraphicsSettingsSubsystem.h    |  882 +++++++++++++
 .../Siegebound/Tests/SiegeGraphicsSettingsTest.cpp | 1357 ++++++++++++++++++++
 8 files changed, 4683 insertions(+), 16 deletions(-)
```

**Staging law honoured (`TL-§5e` cl. 7a-iv):** every `??` candidate staged by
`git add -- <explicit path>`. ⛔ **No `-A`, no `.`, no bare directory, no `commit -a`.**
**cl. S:** the index held **0 staged entries** before I staged — no stale oids.
**Verified on the COMMIT, not the index** (`§25c`): `git show --stat HEAD` = the 8 above, exactly.

### 🚨 cl. (4) — THE INTEGRATION CHECK THIS LANE ACTUALLY NEEDED, **SHOWN, NOT ASSERTED**

`git diff --cached --name-only` immediately before the commit:

```
  .uasset hits: 0      Config/ hits: 0      Content/ hits: 0
  Tools/ hits:   0     L_Arena hits:  0
  Source/ hits:  3   <- POSITIVE CONTROL: the filter can find things
```

⇒ ✅ **`GFX-§1`'s whole claim — that this lane writes no assets — HOLDS, and the human-gate premise
did not fail silently.** The `Source/ hits: 3` line is the point: a filter returning five zeros is
indistinguishable from a dead `grep` (`SC-§96`), so one row of it was made to return non-zero.

### 🚨 A REAL ANCHORING TRAP I WALKED INTO AND CAUGHT — worth the next host's minute

**This repo's root is `C:/GitProjects/GitHub/GitClaudeUnrealTesting`; the project lives one level down
in `GitClaudeUnrealTest/`.** My first `git cat-file -e HEAD:.claude/pipeline/…` probes therefore
returned **"NOT in HEAD" for everything**, including files that are plainly in `HEAD` — a **dead
reader reporting maximum alarm**, exactly `SC-§96`'s shape. Caught by running the pair of controls the
gate's L1.6 item 7 demands:

- ✅ **positive:** `HEAD:GitClaudeUnrealTest/.claude/pipeline/qa/TASK-934.md` → **found** (reader alive)
- ✅ **negative:** `HEAD:.claude/pipeline/qa/TASK-934.md` → **not found** (the bare prefix matches
  nothing in this repo, **ever**)

⚠️ **The same trap bit a second time and I am recording it:** `git log --oneline -- <repo-root-path>`
run from the project subdirectory returned **SILENCE** for files that exist, because the pathspec was
re-anchored to the cwd. Re-run as `git log -- ":/GitClaudeUnrealTest/…"` it answered — and I proved
the new form on a file I *knew* landed in `42734b7`. ⇒ ⚖️ ***a path filter in this repo is wrong by
default; every one of them owes a control, and `git`'s answer to a wrong path is silence, not an
error.***

### cl. 7a STANDING CENSUS — command run **verbatim**, every line classified

`git status --short --untracked-files=all --ignored -- .claude/pipeline/` ⇒ **5 lines, non-empty**
(a real derivation, so no zero-census branch is owed). ⛔ **Zero `!!` (ignored) lines — the `(7a-ii)`
class is clean today.**

| line | test run at my instant | verdict |
|---|---|---|
| ` M CONVENTIONS.md` | table default says leave; **my own row's cl. (3) names it** | ✅ **TAKEN** — `(7a-iii)`: an explicit naming in the host's row overrides the default |
| ` M TASKBOARD.md` | same | ✅ **TAKEN** — same clause |
| `?? handoffs/TASK-1113-programmer.md` | **my adopted subject**; I am its named host and I am committing | ✅ **TAKEN** |
| `?? qa/TASK-1114.md` | **my gate**, same subject | ✅ **TAKEN** |
| `?? handoffs/TASK-937-buildmaster.md` | board read: ⭐ `TASK-937` = **`done — committed a521687`** ⇒ **terminal AND already committed** | ✅ **TAKEN** — cl. 7's *"the NEXT host takes it"*; **`(7a-v)`'s two conditions both fail for it** (its host has already run), so it is a **genuine** orphan |

⭐ **`(7a-v)` — THE CLAUSE RATIFIED TODAY — RESOLVES THE ONE JUDGEMENT CALL ⭐ `TASK-937` FLAGGED, AND
IT RESOLVES IT IN ITS FAVOUR.** 937 left `handoffs/TASK-1113-programmer.md` and `qa/TASK-1114.md`
because `TASK-1113`'s row **named me** as host and its source was outside 937's scope — taking the
documents alone would have split one subject across two commits. The new clause makes that the law:
*a `handoffs/`/`qa/` file whose subject row carries a named host that has not yet committed is **not**
an orphan.* ⇒ **937 was right, the delta it offered the manager is closed, and all five files landed
together in one commit as intended.**

### cl. (3) FLOOR ITEMS **ABSENT** FROM MY DERIVATION — classified, never silently skipped

My row's cl. (3) floor named more than my derivation returned. Each absent item was tested, not assumed:

| floor item | test | verdict |
|---|---|---|
| `SettingsMenuWidget.h` / `.cpp` | `git cat-file -e HEAD:…` ⇒ **present** | ✅ **ALREADY SWEPT ⇒ NOT A CANDIDATE** — taken by **`cd5f4ed`** (⭐ `TASK-447`, the Wave-1 settings screen). Clean at my instant because ⭐ `TASK-1115`, which would have edited them, **has not run** |
| `handoffs/TASK-1112-programmer.md` | same ⇒ **present** | ✅ **ALREADY SWEPT ⇒ NOT A CANDIDATE** — taken by **`001b331`** (⭐ `TASK-1131`) |
| `handoffs/TASK-1115-programmer.md` · `qa/TASK-1115.md` · `qa/TASK-1116.md` | **not in `HEAD` AND not on disk** | ⛔ **NEVER MINTED ⇒ not candidates.** ⭐ `TASK-1115` is still `boarded`; ⭐ `TASK-1116` is blocked on it. **An unwritten document and a swept one are different facts** |

### NAMED AND LEFT (outside `.claude/pipeline/`)

- ⛔ **`handoffs/TASK-1117-buildmaster.md`** — **this file. Structurally excluded** (§ top).
- ⛔ Everything under `Content/**` and `Tools/**` — **fenced by my row, never staged.** At my instant
  **neither appeared in my status at all**: `Tools/ArtPipeline/pipeline_manifest.json` is **in `HEAD`
  and clean**, and the `MainCharacter` art was swept by 🧑 Jonathan's **`e1ba2c4`** — its textures are
  **tracked and clean** on disk, and `Content/RawAssets/MainCharacter.fbx` is **neither on disk nor in
  `HEAD`** (removed by the art lane after export). **None of it was ever a candidate; I name it
  because the session-start snapshot listed it and a silent absence reads like an omission.**
- ⛔ `Content/Maps/L_Arena.umap` — **never staged, never saved, never opened for edit.**

### AFTER THE COMMIT (deliberately left dirty)

`TASKBOARD.md`, by **two causes, and only one of them is mine**:
1. **My two status lines** (§5) — a board line cannot name the hash of the commit that contains it,
   so the records follow the commit. Same pattern as `001b331` and `a521687`.
2. ⚠️ **A concurrent manager write.** `TASKBOARD.md` was modified **23 s after my commit** (a
   one-line `SC-§101` promotion) with no agent of mine touching it, and **`CONVENTIONS.md` grew from
   14 to 38 insertions between my census read and my `git add`** — both files were being authored
   while I staged. **I committed the bytes on disk at `git add` time, which is what "derive at your
   own instant" means; the content is the manager's either way.** ⛔ Recorded because a reader
   diffing my census against my commit would otherwise see a discrepancy and suspect the host.

**The next host sweeps all of it** (⭐ `TASK-1124`), together with this handoff.

---

## 5. BOARD (`Edit` tool only, one line each — ⛔ never `Write`, never a shell rewrite)

- ⭐ **`TASK-1117`** → `done 2026-09-07 (TASK-1117) — committed 42734b7`, carrying the `Result:` line,
  the measured `517 / 0` with its reconciliation, the 15/15 mutation tally, the cl. (4) proof, the
  `SC-§99` gap, and `NOT PUSHED`. Prior text kept as struck history — **including the correction that
  its two blockers had *not* landed** (below).
- ⭐ **`TASK-1113`** → `done — shipped 42734b7`, prior `qa-passed` text kept as struck history.
- ⛔ **No other row touched.**

---

## 6. 🚨 FOR THE MANAGER — THE ONE THING THIS COMMIT DOES **NOT** DO, AND MY ROW'S OWN HEADLINE SAYS IT DOES

⛔⛔ **`TASK-1117`'s heading reads *"THE FIRST PLAYABLE CHECKPOINT. THE MENU EXISTS AND THE SLIDERS
MOVE."* — ⛔ THAT IS NOT TRUE OF `42734b7`, AND IT IS NOT CLOSE.**

- My row was **`BLOCKED on ⭐ TASK-1116`**, which is blocked on ⭐ **`TASK-1115`** (the panel). **Neither
  has run** — measured, not assumed: their handoffs and QA reports are *never minted* (§4), and both
  rows still read `boarded`. I was dispatched on the ⭐ `TASK-1113` half alone and shipped exactly that.
- ⭐ **`SC-§36.1` CENSUS, RUN RATHER THAN ASSUMED: `USiegeGraphicsSettingsSubsystem` HAS ZERO CALLERS.**
  A repo-wide grep returns **three files: the header, the cpp, and its own test.** Zero `Content/**`
  Blueprint consumers. And the `SettingsMenuWidget` that already lives in `HEAD` (from `cd5f4ed`,
  ⭐ `TASK-447`) mentions *"graphics"* **zero times** in either file.
- ⇒ ⚠️ **This commit ships a fully built, fully tested, fully mutation-proofed surface that nothing
  calls** — precisely the `SC-§36.1` shape this project has been burned by. **The difference, and it
  is the whole difference: the gap is DECLARED and it HAS A ROW** (⭐ `TASK-1115` → ⭐ `TASK-1116` →
  a second commit host). It is **scheduled, not abandoned** — the same distinction `(7a-v)` just drew
  for documents. ⛔ **But `SC-§50` is only satisfied while that row stays open: if `TASK-1115` is ever
  dropped, this becomes a permanent orphan of the most expensive kind.**
- ⇒ 📌 **A SECOND SHIP ROW IS OWED** for the panel half, and **the `[GFX-SHIP-1]` heading should be
  corrected** so a later reader does not conclude from the board that sliders were playable at
  `42734b7`. ⛔ Not mine to edit — I touch only my own two status lines (`SC-§82`).

### Other findings, homed by ID — ⛔ none of them a hold on this commit

1. ⚠️ **`M2`'s over-broad red (§3)** — a finding about the **prediction method**, not the code.
   **Second instance today** (⭐ `TASK-937` reported the same class on its own `M2`/`M4`). ⇒ 📌 **a
   needle-based derivation should state its red as *"at least X"*, never as *"X and nothing else"*,
   unless the deriver has modelled every test that reads the same state.**
2. ⚠️ **`SC-§99` has an unclosed corner and I am the measurement (§1).** A **careful** host produces
   **no** unintended failure, so the cheapest control is available exactly to the hosts that need it
   least. ⛔ Not asking for the rule to change — cl. 4 is right. 📌 **But a project-owned corpus of
   real failed build logs would let every host control its parser for free.**
3. ⚠️ **The repo-root anchoring trap bit twice in one session (§4)** — once on `cat-file`, once on
   `git log --`, the second time returning **silence**. 📌 Worth a line in `CONVENTIONS.md`: in this
   repo every `git` pathspec is wrong by default and owes a control.
4. ⚠️ **`qa/TASK-1114.md`'s line citations drift 2–6 lines (§3).** Rows all correct. Cheap to fix, and
   the next host will otherwise re-derive it.
5. ⛔ **`WARN-8`'s comment fix is still owed and is `TASK-1118`'s** — `Tests/SiegeGraphicsSettingsTest.cpp:140-141`
   still states M13's over-broad red *"and the return value"* in the shipped bytes. **My row forbade me
   to touch source for it (cl. (6): comment-only, name it, do not edit).** ⭐ **Execution has now proved
   the comment wrong**, so `TASK-1118` can strike it citing a measurement rather than a reading.
6. ⛔ **`SC-§94`'s pixels-and-log rung is entirely unspent** and the gate says so: nothing here has been
   rendered or looked at. No CVar has been observed moving; no display has changed mode. Owed by
   ⭐ `TASK-1118`/`1119`/`1122`.

---

## 7. EDITOR

| event | detail |
|---|---|
| found UP | PID **20088** (⭐ `TASK-937`'s relaunch), idle |
| closed | by name under 🧑 Jonathan's **standing grant** — ⛔ he was not interrupted. Nothing dirty, `L_Arena` **never saved**, **no Restore-Packages modal appeared** |
| compiles | **17**, all with the editor closed |
| relaunched | PID **26708**, detached, with the `.uproject` |
| MCP | `127.0.0.1:8000` **responding — `http_code=200`** on an `initialize` POST |
| PIE | ⛔ **not started** — nothing in this row needs it, and `SC-§94` is owed elsewhere |

---

## 8. GIT

- **`42734b7`** — `main`, **3 ahead of `origin/main`** (behind me: `a521687`, `001b331`, and
  🧑 Jonathan's `e1ba2c4`).
- ⛔ **NOT PUSHED.** No push was requested and none was made.
- ⛔ No `--allow-empty`, no amend, no `git reset`, no force, no branch cut, no `git add -A`.
- Prior `HEAD` re-derived at my own instant (`git log --oneline -1`), never read off the board.
