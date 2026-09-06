# Handoff — TASK-1070 — [FOGVIS-CALLER-SHIP], the commit host for TASK-1068

**Marker:** `TASK-1070-HOST` · build-master · 2026-09-05
**Gate:** `qa/TASK-1069.md` — PASS-WITH-ONE-BINDING-CONDITION · 0 BLOCKERS · 3 WARN · 4 NIT.
**The binding condition was DISCHARGED BY EXECUTION. Both reds were witnessed. The PASS stands.**

---

## 0. THE HEADLINE

The visual asset `/Game/Blueprints/BP_SiegeFog` shipped in `ef2c901`, correct in every property and
**invoked by nothing**. Every mention of it anywhere in `Source/` was a **comment**. This commit
supplies the spawn/despawn wiring and the five tests that would have caught its absence.

**Measured by me, at my own instant, with the same script run twice:**

| census (house `CountOccurrencesInCode` rule, shipping non-`Tests/` code) | executable hits |
|---|---|
| at `b6a44b8` — **pre-diff** | **0** |
| working tree — **post-diff** | **30** |

---

## 1. HEAD AS A RELATION (cl. 1, `SC-§89` rule 1)

```
git merge-base --is-ancestor b6a44b8 HEAD   ->  exit 0   PASS
```

**`HEAD` measured at my instant = `b6a44b81fff800ba9445210bc152661ee2cd9167`.** Recorded, not pinned.
Git root is `C:/GitProjects/GitHub/GitClaudeUnrealTesting`; every repo path below is therefore
prefixed `GitClaudeUnrealTest/` in `git status` output.

---

## 2. THE BINDING CONDITION — BOTH MUTATION REDS, WITNESSED (`qa/TASK-1069.md` W-1)

### The safety net that did not exist (W-2), and what I did instead

`FogVolume.h`, `FogVolume.cpp` and `Tests/SiegeFogVisualTest.cpp` were **uncommitted** at `b6a44b8`.
`git checkout --` / `git restore` would not have undone a mutation — it would have **deleted the
entire TASK-1068 diff**. Before touching anything I copied all three to the session scratchpad and
recorded `sha256`:

| file | sha256 (pre-mutation, and after every restore) |
|---|---|
| `FogVolume.h` | `53f0075e916664ba25cc3ed690f6d6f0f7df08e1390ca135932b02470615b0e3` |
| `FogVolume.cpp` | `febc2d6eb01785c16e6d91bad6a103d0aaf974bc605e39e14a825cf2601d4fd4` |
| `Tests/SiegeFogVisualTest.cpp` | `7e0dc8c4d97367e7e8b46ca2a0623675a0aec70cbe93da2dd8e09e3ed16f1899` |

**Every restore was made FROM THOSE COPIES and hash-verified.** No git restore gesture was used on
these files at any point.

### MUTATION A — THE SPAWN RED ✅ WITNESSED

Deleted `\tRefreshFogVisual();` from `RaiseFog`, `FogVolume.cpp:160` (1 tab — verified against the
real bytes before deleting). `diff` vs backup: **`160d159`, exactly one line.**

- **Compile:** `Result: Succeeded`. `[1/14] Compile SiegeFogVisualTest.cpp`, `[2/14] Compile
  FogVolume.cpp`, `[13/14] Link UnrealEditor-GitClaudeUnrealTest.dll` — a real build, not a no-op.
- **Suite:** `493 / 0`… **`493 / 1`** — verdict `EXITED`, 40.1 s, bounds `1500 / 420 / 180`.
- **FAILING TEST:** `Siegebound.Fog.EveryExitFromFoggedReconcilesTheVisualAndThereAreExactlyThreeOfThem`

**Both predicted errors appeared, verbatim from the log:**

```
Error: Expected '(entry + refresh) RaiseFog - the fog rises, the box must appear calls
RefreshFogVisual() exactly once' to be 1, but it was 0.   [SiegeFogVisualTest.cpp(479)]

Error: Ordering marker 'RefreshFogVisual();' is gone - the ordering probe is stale, so it FAILS.
                                                          [SiegeFogVisualTest.cpp(483)]
```

Terminus: `**** TEST COMPLETE. EXIT CODE: -1 ****`, `Setting GIsCriticalError due to test failures`.

**Restored from copy. Hash re-verified: MATCH. `diff` vs backup: identical.**

### MUTATION B — THE DESPAWN RED ✅ WITNESSED

Deleted `\t\tDestroyFogVisual();` from `RefreshFogVisual`, `FogVolume.cpp:499` (2 tabs — verified).
`diff` vs backup: **`499d498`, exactly one line.**

- **Compile:** `Result: Succeeded`. `[1/4] Compile FogVolume.cpp`, `[3/4] Link …dll`.
- **Suite:** **`493 / 1`** — verdict `EXITED`, 40.1 s, same bounds.
- **FAILING TEST:** `Siegebound.Fog.TheVisualHasOneSpawnSiteAndOneDespawnSiteAndTheStateActorStillNeverTicks`

```
Error: Expected 'THE DESPAWN HALF: the reconciler destroys in exactly one place. "A seam that can
be entered and not left is HALF A SEAM" - a spawn-only build is PERMANENT FOG with a green suite'
to be 1, but it was 0.                                    [SiegeFogVisualTest.cpp(559)]
```

**Restored from copy. All three hashes re-verified: MATCH.**

⇒ **The tests DISCRIMINATE. A build with the spawn call deleted, or the despawn call deleted, goes
RED. This is the check that `TASK-1043` never had.**

---

## 3. ⚠️ A FINDING THE MUTATION PROTOCOL ITSELF PRODUCED — `cp -p` DEFEATS UBT

**After restoring mutation B I ran the clean compile and it returned `Result: Succeeded` in
0.91 s with an EMPTY `[n/N] Compile` list.** UBT built **nothing**.

**Cause:** I restored with `cp -p`, which **preserves the original mtime**. The restored (clean)
`FogVolume.cpp` was therefore *older* than the object file built from **mutation B**, so UBT
considered it up to date. **The linked DLL at that moment was still the MUTANT.**

⇒ Had I trusted that green `Result:` and run the suite, the source-text probes would have read the
clean file and gone **green while the compiled binary was missing `DestroyFogVisual()`** — a false
green of exactly the class this row exists to prevent, produced by the *restore*, not the diff.

**Fix applied:** `touch`ed all three files (content hash re-verified unchanged), rebuilt, and
confirmed the `[n/N]` list. ⇒ **`SHIP-§9` again: validate a gate against the failure it detects.
A green `Result:` with an empty compile list is not a compile.** This belongs in the mutation
protocol as a standing step for whoever runs the next one.

---

## 4. THE CLEAN TREE — COMPILE AND SUITE, EXECUTED (cl. 3)

**Compile (`build-clean2.log`), parsed for `Result:`, never `$LASTEXITCODE`:**

```
[1/12] Compile [x64] SiegeFogVisualTest.cpp
[2/12] Compile [x64] FogVolume.cpp
...
[11/12] Link [x64] UnrealEditor-GitClaudeUnrealTest.dll
Result: Succeeded          (16.19 s, zero warnings, zero errors)
```

Both subject translation units are **in the list** — the build is real.

**Suite — `SC-§87` bounds ARMED (`1500` overall / `420` boot / `180` stall), nothing skipped,
filtered or excluded:**

```
UnrealEditor-Cmd.exe <uproject> -ExecCmds="Automation RunTests Siegebound;Quit"
  -nullrhi -unattended -nopause -nosplash -NoLiveCoding -log -abslog=<scratchpad>/suite-clean.log
```

### EXECUTED: **`494 / 0`** · verdict **`EXITED`** at **40.1 s** — the bound never fired

| runner verdict field | value (**verbatim, uncurated**) |
|---|---|
| `VERDICT` | `EXITED` |
| `TEST_STARTED` | **494** |
| `TEST_COMPLETE` | **494** |
| `SUCCESS` | **494** |
| `FAIL` | **0** |
| `OTHER_RESULT` | 0 |
| **`N / M`** | **`494 / 0`** |
| `QueueEmpty` | **`NO`** |
| `TESTCOMPLETE` | `YES (exit code 0)` |

**`QueueEmpty: NO` recorded uncurated as `SHIP-§9f` / `TASK-1060` evidence.** It is the same
probe-wording artefact `TASK-1056` declared: this run ends on `**** TEST COMPLETE. EXIT CODE: 0 ****`
and never prints the phrase `Automation Test Queue Empty`. The decisive evidence is independent of
it — **494 started = 494 completed = 494 Success, 0 Fail**, self-terminated, explicit exit code 0.

**Delta is EXACTLY the declared one:** baseline `489` (`TASK-1056`, executed at `2c58460`) → `494`,
**`+5`** = the five `IMPLEMENT_SIMPLE_AUTOMATION_TEST` in the new file. **No other movement.**
The author's `489 → 494 (+5)` was DECLARED-NOT-EXECUTED; **it is now EXECUTED and it is correct.**

### ⚠️ TEST 224 — IT PASSED, BUT **NOT** IN POSITION 224. ROUTED, NOT OVERWRITTEN (`TL-§5b`)

The row asks that *"test 224 pass in its original position."* Reported precisely:

- Baseline test 224 = `TheUnitSideCeilingIsOneDoorAndAmbushIsExemptByConstruction`.
- It is now at **position 227**, `Result={Success}`.
- **The shift is fully accounted for:** exactly **3** of the 5 new `Siegebound.Fog.*` tests sort
  *before* it (positions **198**, **222**, **223**); the other 2 land after (**229**, **230**).
  `224 + 3 = 227` — **arithmetic, not drift.** All five new tests `Success`.
- Position 224 now holds `TheHeroLineSpellKeepsItsFullRangeUnderFog`, also `Success`.

⇒ **No hang at 224 or anywhere. `TASK-1048` stays closed.** Any future row pinning *"test 224"* by
**ordinal** should be re-pinned **by symbol** — the ordinal is now stale by 3 and will move again
with the next added test.

### ⚠️ INCIDENTAL: A DUPLICATE TEST **SHORT NAME** IN THE SUITE (pre-existing, not this diff)

494 starts but only **493 distinct `Name={}`** values — because two different tests share a short
name: `Siegebound.Account.SlotContract` and `Siegebound.Settings.SlotContract`. **Distinct
`Path={}` values = 494 of 494**, both `Success`, so nothing ran twice and nothing was lost.
**Latent hazard, worth one follow-up row:** any tally keyed on `Name={}` instead of `Path={}`
silently under-counts by one, which is how a lost test hides.

---

## 5. CLAUSE 4 — THE REACHABILITY RE-DERIVATION, DONE MYSELF (NOT INHERITED)

Script replicating the house `CountOccurrencesInCode` comment rule (`SC-§80`), run over **all** of
`Source/`, across `BP_SiegeFog` + `FogVisualClassPath` + the four lifetime symbols + `FogVisualActor`.

| | shipping (non-`Tests/`) | `Tests/` |
|---|---|---|
| **at `b6a44b8` (pre-diff, via `git show`)** | **0** | 0 |
| **working tree (post-diff)** | **30** | 45 |

**ZERO executable hits was the automatic-BLOCKER condition. It is NOT met — the opposite is
measured, and the pre-diff `0` is measured too, by the same script.**

**On QA's *"should come back 28"* — I got 30, and derived it independently as instructed. The gap
is counting convention, not substance:**
1. `FogVolume.cpp:439` carries `BP_SiegeFog` **TWICE** on one line
   (`/Game/Blueprints/BP_SiegeFog.BP_SiegeFog_C`). QA's table counted **lines**; the house helper
   counts **occurrences** — and the diff's own test at `:667` asserts
   `CountOccurrencesInCode(ClassPathBody, TEXT("BP_SiegeFog")) == 2` for that body, so
   occurrence-counting is the house rule. **+1.**
2. QA's `FogVisualClassPath` enumeration named `h:533, cpp:431, :538, :546, :587` and **omitted
   `cpp:542`** — a code line (a `TEXT(...)` continuation inside the `UE_LOG`) that names the symbol.
   **+1.**

⇒ **28 (lines, one site missed) vs 30 (occurrences, complete). Neither number is near zero and the
verdict is identical.** Recorded so the next census knows which convention it is comparing against.

**The five chains, spot-checked at my instant and agreeing with the gate:** spawn ← `RaiseFog` ←
`SpellLibrary.cpp:690` `FogCover` ← player controller (a human click) · BrightSun ←
`SpellLibrary.cpp:740` `FogClear` ← the same · reset ← `SiegeGameMode::PlayAgain` ← the end-screen
button · expiry ← `FTimerManager` · teardown ← `EndPlay`. **No chain left unclosed.**

`Content/Blueprints/BP_SiegeFog.uasset` present on disk (37 947 bytes) at the path `:439` names.

---

## 6. PATHSPEC — STAGED BY NAMED PATH, NEVER BY DIRECTORY (cl. 2, cl. 6)

**No `git add -A`. No `git add Source/`. No `-a`. No `reset` / `stash` / `clean` / `checkout --` /
`restore` / `--amend` / `--no-verify`.** Derived with `--untracked-files=all`, verified with
`git diff --cached --name-only`.

**`Source/` set — EXACTLY THREE, matching the row's named list:**

- `Source/GitClaudeUnrealTest/Siegebound/FogVolume.h` (M)
- `Source/GitClaudeUnrealTest/Siegebound/FogVolume.cpp` (M)
- `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeFogVisualTest.cpp` (new)

**`SpellLibrary.cpp` is ABSENT — expected and NOT a finding** (row cl. 2 says so explicitly): the
mechanism did not route there, it reuses the existing `FogCover`/`FogClear` call sites unchanged.

**N-4 SETTLED BY MEASUREMENT, not relayed** (`git status --porcelain` at commit time):

- `SpellLibrary.cpp` — **clean, byte-identical to `HEAD`.**
- `SiegeGameMode.cpp` — **clean, byte-identical to `HEAD`.**
- `CONVENTIONS.md` — **MODIFIED (+11 lines)**, and correctly so: it is TASK-1068's own doc
  obligation, the `SC-§36.1` **Instance 2** entry. In scope under `.claude/pipeline/**`.

**MODULE FENCE (parallel-safety release condition) — MET by the trigger, not by hope:**
`git status --porcelain -- Source/` read at **17:35:49** and again at **17:43:08** came back
**IDENTICAL** (the same three paths). No `TASK-1064` / `TASK-1067` torn file. The three other-lane
`Tests/` paths (`SiegeFogVolumeTest.cpp`, `SiegeBrightSunTest.cpp`,
`SiegeFogRetentionWiringTest.cpp`) are **clean/tracked and were NOT staged** — **NAMED-AND-LEFT.**

🧑 **`.claude/agents/qa-reviewer.md` is MODIFIED and was NOT staged — NAMED-AND-LEFT, because
Jonathan has still not ruled.** Confirmed absent from the index after commit.

**Zero untracked `handoffs/TASK-<prev-host>-buildmaster.md` were pending** (cl. 6 expected one; two
or more would have been a finding). Not a finding.

---

## 7. THE FINDINGS I CARRY FORWARD — I FIXED NONE OF THEM (I write no code)

Routed for the manager to board:

1. **W-3 (from the gate)** — `SpawnFogVisual()` has no internal re-entrancy guard; the "one spawn"
   pin only counts *inside* `RefreshFogVisual`'s body. A second call added elsewhere in the file
   passes every assertion and orphans a world-sized opaque box. *Fix: an early-out in
   `SpawnFogVisual`, or widen the pin file-wide.*
2. **⭐ NEW, MINE — `cp -p` restore defeats UBT** (§3). A preserved mtime makes a green `Result:`
   with an empty compile list; the binary stays the mutant. **Belongs in the mutation protocol.**
3. **⭐ NEW, MINE — the "test 224" ordinal is stale by 3** (§4). Re-pin by symbol.
4. **⭐ NEW, MINE — duplicate test short name** `SlotContract` in two paths (§4). Any tally keyed on
   `Name={}` under-counts.
5. **N-1** — the `FogVolume.cpp:265-268` iterator-safety comment states a TRUE fact that is not the
   operative mechanism (`TActorIterator` iterates a private snapshot). **QA settled the conclusion
   SAFE — no crash on match restart.** The wrong *ground* is a comment fix.
6. **N-2** — `SiegeFogVisualTest.cpp:49` says "Tests 3-6"; there are five, so Lane B is tests 3-5.
7. **N-3** — `FogVolume.cpp:568-571` hand-multiplies the box dimensions in prose beside a
   derivation whose point is that nobody types them. Goes stale silently.

---

## 8. 🧑 THE OPEN QUESTION, CARRIED FORWARD RATHER THAN CLOSED BY MY GREEN (cl. 8)

A green suite here closes *"does a caller exist, and is it reached?"* and **closes nothing else.**
Per `SC-§88`, nobody may settle the look with a screenshot: a single-shot volumetric reading is not
converged (`55.8%` vs `100.8%` at identical settings). A fog number needs a convergence series.

***The spawn is PROVEN; the LOOK is NOT — Jonathan's eye is owed a playtest of fog-up,
BrightSun-clear and natural expiry.***

---
*build-master · TASK-1070 · 2026-09-05 · marker `TASK-1070-HOST` · gate `qa/TASK-1069.md`,
whose binding condition was discharged by execution.*
