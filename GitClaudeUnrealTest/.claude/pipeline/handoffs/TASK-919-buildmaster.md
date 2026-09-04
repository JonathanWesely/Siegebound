# TASK-919 — [HOST-CPP] build-master handoff

**Commit: `239ca77`** · 7 paths · `2081 insertions(+), 131 deletions(-)`
**`main` is 14 ahead of `origin/main`. NOT PUSHED.**
Date: 2026-09-03 · base `8e74596`

**Rows adopted and CLOSED by this commit: `TASK-857` · `TASK-873` · `TASK-871` · `TASK-874`.**
`TASK-906` is **superseded** — see §3, it must be re-pointed to its remaining duty `§7′(e)`, not to a file already shipped.

---

## 1. THE HEADLINE MEASUREMENTS

| | |
|---|---|
| **Compile** | `Result: Succeeded` — **verbatim, the sole `Result:` line in the log** |
| `Result: Failed` | **0** · `error C####` 0 · `error LNK` 0 · `fatal error` 0 |
| Exit code | `0` — ⛔ **given no weight** (the Build.bat-exit-code law) |
| Scope | `[32/32]`, a real build — **not** a `Target is up to date` no-op |
| **Suite (EXECUTED BY ME)** | ⭐ **`432 Result={Success}` / `0 Result={Fail}`** |
| Census | **HEAD `425` → `431` declared across 31 files.** Commit delta **`+6`** |

`-ExecCmds="Automation RunTests Siegebound;Quit" -nullrhi -unattended -nopause -log`

### The positive control on the zero (`SC-§39`)

A `Result={Fail}` count of `0` is a null reading, indistinguishable from an unreadable
instrument. Discharged three ways:

1. **The needle is proven live.** The identical `grep -c 'Result={Fail}'` returns **1**
   against a synthetic `Test Completed. Result={Fail} Name={Synthetic}` row and **0**
   against the real log. ⇒ **a measured zero, not an empty log.**
2. **The vocabulary census returns exactly ONE variant tree-wide** —
   `grep -oE "Result=\{[A-Za-z]+\}" | sort | uniq -c` ⇒ `432 Result={Success}` and nothing
   else. There is no third state hiding.
3. **Two independent instruments agree on 432** — the *executed* `Result={Success}` count
   (432) and the *source-text* declaration census (432 across 31 files). Different tools,
   different questions, same number.

The same discipline was applied to the compile: the `Result: Failed` needle returns 1 on a
synthetic row and 0 on the real log.

### ⭐ THE FIRST COMPILE OF `SiegePlayerController.{h,cpp}` SINCE `TASK-871`'s DIFF

That duty was transferred to me by name (`TL-§5c` cl. 5). `qa/TASK-914.md` §4 read for
compile hazards and found none — **a read is not a compile.** This build is the first event
that could have found a syntax or link error in those files. **It found none.** Both files
compiled inside `[32/32]`; `SiegePlacementTest.cpp` at `[24/32]`.

---

## 2. THE NAMED ACCEPTANCE ROWS — BY NAME, NOT BY TOTAL (item 4a)

A bare total cannot show a cut loop. All seven were read out of the log individually:

```
Result={Success} Name={EveryRelatedActionIdResolvesToARealRow}                    TASK-852/857 TEST 18
Result={Success} Name={EveryRowHasAuthoredDetail}                                 test 9 — STILL GREEN
Result={Success} Name={RightClickIsTaughtOnlyByTheRowsThatOwnIt}                  TASK-870/873 TEST 17
Result={Success} Name={TheSlopeProbeConsumesTheOneFootprintMeasurementAndTheGateChainIsUnreordered}
Result={Success} Name={TheSlopeProbeRefusesACornerOnAFlankWhileAFlatPadAtTheMaximumWheelScaleStillPasses}
Result={Success} Name={TheSlopeProbeSampleOffsetsAreDerivedFromTheFootprintRadiusAndNeverTranscribed}
Result={Success} Name={RosterCapCollapsesTheTail}                                 TASK-874
```

⭐ **`TASK-852` cut a loop out of a shipped test and BOTH halves are green** — the extracted
walk (TEST 18) *and* test 9, which kept its own subject.

✅ **The two rows the board warned might be RED are GREEN.** All **9** `Siegebound.Acquisition.*`
rows report `Result={Success}`, confirming `TASK-835`'s finding — nothing is owed there.

---

## 3. ⛔⛔ THE UNTRACKED-FILE QUESTION — SETTLED, AND THE ANSWER REVERSES FOUR RECORDS

**Duty `§7′(a)`, run first, before any pathspec or delta:**

```
$ git ls-files --error-unmatch .../Tests/SiegeAssistantSelectionTest.cpp
GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeAssistantSelectionTest.cpp
exit 0
```

⛔ **THE FILE IS TRACKED.** It is present in `HEAD`, last written by `4a03e03`
(`TASK-742`, 2026-09-01).

| | `HEAD` | worktree | delta |
|---|---|---|---|
| `^IMPLEMENT_SIMPLE_AUTOMATION_TEST` | **38** | **39** | ⭐ **`+1`** |

⇒ **The delta is `+1`, NOT `+39`.** The one added row is
`Siegebound.Assistant.Selection.RosterCapCollapsesTheTail` — confirmed by a name-set diff of
`HEAD` against the worktree: **1 added, 0 removed.**

### How the wrong figure propagated — and why the retraction was the error

`handoffs/TASK-814-buildmaster.md:68` — written by the **last agent in this chain who
actually had Git** — records the file as **`SiegeAssistantSelectionTest.cpp 38→39 | +1`**,
and at line 229 says *"It is in my census (the `+1`) and I left it."* **That was correct and
it was measured.**

The `+39 untracked` belief came from **conflation with a different file.** `TASK-814`'s same
handoff lists **8 genuinely new untracked test files** (funnel · stack · keylabel · castbar ·
**fogclamp** · fog · invis · placement). `Tests/SiegeFogClampTest.cpp` really was new and
untracked at `+8`. The board then wrote, of that paragraph, *"`Tests/SiegeAssistantSelectionTest.cpp`
is ⛔ STILL IN EXACTLY THAT STATE"* — a generalisation that was never measured.

From there: `qa/TASK-889.md` loop-1 **WARN-2** told the programmer his `git show HEAD:`
provenance *"cannot be true if the file is untracked, and two dated project records say it
is."* He is fenced from Git, could not check, and **retracted a claim that was true.** The
retraction hardened into loop-2 §5 (*"treat the commit delta as `+39` — THE WHOLE FILE —
never `+1`"*), into `TASK-919`'s row, and into my dispatch.

⚖️ **The durable lesson: the retraction was reasoned from a premise nobody had measured, and
a correct record was withdrawn because two incorrect ones outnumbered it.** `§25b` cl. R is
right that `git status` is the measurement and a `names:` line is a citation — **this extends
it: a QA WARN is also a citation.** The first party with the instrument must re-measure even
when the direction of travel is a *retraction*, because a retraction feels like the cautious
option and here it was the wrong one.

✅ **Nothing was lost either way** — the file is committed, the `+1` is in `HEAD`, and had I
staged it believing `+39` the outcome would have been identical. **The cost was purely to the
record.** Four records now need correcting: `qa/TASK-889.md` WARN-2/WARN-2′/§5, `TASK-919`'s
row item (1c)(a), `TASK-906`, and TASKBOARD's *"STILL IN EXACTLY THAT STATE"* line.

### ⚠️ `TASK-906` — RE-POINT IT, DO NOT LET IT CLAIM THIS FILE

**I took the file. `TASK-906` must not also claim it** — that is the `857`/`873` collision
shape. Its remaining live duty is **`§7′(e)`**: board the *enforceable* form of the WARN-1
rider — a structural pin on `Capture()`'s filter set that goes RED when a filter is added and
names `BuildDerivedRoster` as the mirror, with its own gate proving it goes red on a
synthesised violation. **That needs a compile and touches production source; it is not this
diff and it was not in this commit.**

---

## 4. THE PATHSPEC vs THE `names:` LINE (`§25b` cl. R)

⚠️ **It is SEVEN paths, not six.** The row's own arithmetic is `(1a) 3 + (1b) 3 + (1c) 1 = 7`;
the "six" in my dispatch counts `{h,cpp}` pairs as one. I staged **7 explicit paths** and
`git diff --cached --name-only | wc -l` returned **7**.

| # | path | row | gate | delta |
|---|---|---|---|---|
| 1 | `Siegebound/SiegeControlsHelpWidget.h` | `TASK-873` ← 870 | `qa/TASK-872.md` PASS | — |
| 2 | `Siegebound/SiegeControlsHelpWidget.cpp` | `TASK-873` ← 870 | `qa/TASK-872.md` PASS | — |
| 3 | `Tests/SiegeControlsHelpTest.cpp` | ⭐ **`TASK-873` AND `TASK-857`** | `872` + `856`, both PASS | **+2** |
| 4 | `Siegebound/SiegePlayerController.h` | `TASK-873` (1b) ← 871 | `qa/TASK-914.md` PASS | — |
| 5 | `Siegebound/SiegePlayerController.cpp` | `TASK-873` (1b) ← 871 | `qa/TASK-914.md` PASS | — |
| 6 | `Tests/SiegePlacementTest.cpp` | `TASK-873` (1b) ← 871 | `qa/TASK-914.md` PASS | **+3** |
| 7 | `Tests/SiegeAssistantSelectionTest.cpp` | `TASK-906` → **taken here** ← 874 | `qa/TASK-889.md` loop 2 PASS | **+1** |

**Path 3 is the co-claimed one** (`qa/TASK-856.md` W-4). Both tests were swept in together,
as intended — **`TASK-857` and `TASK-873` are BOTH named and BOTH closed by this handoff.**

### The cl. R reconciliation — run as an instrument, not a formality

`git status --porcelain` against the union of all pending pathspecs, **every dirty path
classified, nothing unaccounted:**

```
   7  MINE — TASK-919 (committed)
   3  TASK-925  — SummonedUnit.{h,cpp} + Tests/SiegeInvisibilityTest.cpp  (TASK-923's veil
                  wiring, gate TASK-924 HAS NOT RETURNED)          LEFT DIRTY, correctly
   1  TASK-927  — Content/FogArea/  (27 assets, awaiting Jonathan) LEFT UNTRACKED, correctly
  65  pipeline docs — CONVENTIONS.md, TASKBOARD.md, 40 handoffs/, 17 qa/, + this file
   0  ⛔ IN NEITHER
```

⛔ **Zero files in neither.** The pipeline docs are named rather than staged: the last three
build-master commits (`8e74596`, `a8b97b3`, `44a8710`) all committed work artefacts only and
left `TASKBOARD.md`/`CONVENTIONS.md`/their own handoffs to the doc sweep. I followed that.

**Nothing was staged when I arrived** — `git diff --cached` was empty, index == `HEAD`. I ran
no `git reset` at any point; the commit was made by explicit pathspec.

### `§25b` cl. S — the full binary sweep, run although the index was clean

⭐ **The fires are intermittent, not continuous, so a clean index is not proof the hazard is
gone.** I swept **all 4971 tracked binaries** (`*.uasset *.umap *.fbx *.png *.wav *.ttf`),
comparing each file's **content hash against its index oid** — not `git status`, which can be
fooled by an unchanged mtime+size:

```
git update-index --really-refresh
git ls-files -s -- <binary globs> | (batched) git hash-object --stdin-paths
⇒ 4971 hashed · 0 MISMATCH · 0 missing from disk
```

✅ **Zero stale index entries.** No editor-staged asset, nothing to unstage. Like `TASK-926`,
I found it clean on arrival and ran the sweep anyway.

---

## 5. `SC-§43` cl. 4 — THE TRAP THAT STOPPED `TASK-814`

⛔ **Neither my build nor the suite measures whether the intermediate `HEAD` compiles — both
build the WORKING TREE.** So I checked it symbolically instead.

**(a) The excluded lane's new symbols, and whether I reference them.** `TASK-923` adds exactly
two functions to the excluded `SummonedUnit.h` — `ApplyVeilMaterial()` and
`ClearVeilMaterial()`. Grepped across all seven committed paths: **zero hits.**

**(b) Every identifier my diff ADDS that occurs nowhere in `HEAD`'s `Source/**`.** 2002 added
identifiers vs 24052 in `HEAD`; the new ones are overwhelmingly comment prose. Every
**code-shaped** new symbol was located and **all resolve inside my own seven paths**:

| symbol | defined/used in |
|---|---|
| `IsSurfaceNormalWithinSlopeLimit` · `PlacementSurfaceSlopeDegrees` · `PlacementSlopeSampleOffset` · `NumPlacementSlopeSamples` | `SiegePlayerController.{h,cpp}` + `Tests/SiegePlacementTest.cpp` — **all mine** |
| `BuildDerivedRoster` · `CommandableKindsInCardRowOrder` · `FDerivedRoster` · `BoardWithinRosterCap` · `bSpawnsSummonedUnit` | `Tests/SiegeAssistantSelectionTest.cpp` only — **test-local** |
| `SiegeSlopeProbeFixture` | `Tests/SiegePlacementTest.cpp` only |
| `FogClamp` | ⚠️ chased: **comment-only** in my file (`:184`, `:321`, `:323`). `SiegeFogStatics.h` and `Tests/SiegeFogClampTest.cpp` are **already in `HEAD` and clean** — `TASK-913`'s subject landed earlier. No dependency. |

✅ **No symbol I committed resolves into a file I excluded.** The residual risk is the
inverse — `HEAD` keeps the *pre-*`TASK-923` `SummonedUnit`, which is the state that shipped in
`8e74596` — so no new edge is created.

---

## 6. WHAT THE COMMIT MESSAGE CLAIMS — EACH SUBJECT GREPPED BEFORE IT WAS WRITTEN

Following the predecessor who found three of seven briefed subjects were not in its commit and
refused to write them in. All four subjects **verified present in the diff** before writing:

1. **Four-corner slope trace + the NaN fail-open repair** — `SiegePlayerController.cpp`:
   `FMath::IsFinite(FootprintRadius)`, `if (!FMath::IsFinite(NormalZ))`,
   `if (!FMath::IsFinite(MaxSlopeDegrees))`, and the comment recording that a NaN previously
   read as flat and was **admitted**, laundered by two engine clamps. ✅ present.
2. **The war-map help repair** — the false close-sentence removed, `Interface.MapMarks`
   appended. ✅ present.
3. **The edge-integrity extraction** — TEST 18 present, test 9 retained. ✅ present.
4. **The derived assistant roster** — `BuildDerivedRoster`, `RosterCapCollapsesTheTail`. ✅ present.

⚠️ **ONE CORRECTION I MADE TO MY OWN BRIEF.** My dispatch described the repair as removing a
sentence *"Jonathan personally refuted by observation"* — true, but only of the **right-click**
half. Reading the diff: **the ESCAPE half was KEPT and rewritten** (*"Escape closes it too,
polled every frame"*), deliberately not re-derived. `qa/TASK-872.md` W-4/R-2 records Escape as
**unobserved and still shipping.** The commit message therefore says the right-click half is
gone **and explicitly disclaims any Escape behaviour** rather than implying the page is now
reconciled. **Half of that row still rests on an assumption.**

⛔ **The mandatory `ECC_Visibility` line is in the message**, in its own paragraph: the probe
is **not** terrain-only, a corner can hit another building/rock/the castle, *"Too close to
buildings"* may now read *"Too steep"*, it **fails closed**, and the question is boarded as
`TASK-920`.

---

## 7. THE PIXEL DUTY (item 7) — ⛔ NOT DONE, AND `TASK-920` STAYS OPEN

⛔ **I ran NO PIE session. I have not observed P-1 or P-2.** The compile requires the editor
closed, and I left it closed.

⛔ **I am recording no observation, and the arithmetic is not one** (`SC-§44` cl. 3a, `VIS-§4`).
**`TASK-920` remains OPEN — do not close it on this handoff.** P-2 (place a building hard
against an interior castle wall, then step it away) is still the **only** instrument that
decides whether `qa/TASK-914.md` W-1 is a documentation fix or a real balance change.

---

## 8. FOLLOW-UPS FOR THE MANAGER — reported, not fixed (item 9 fences me from source edits)

**(F-1) ⛔ A SECOND stale back-pointer, and this commit SHIPPED it.**
`qa/TASK-856.md` WARN-1 named **one** site whose prose is now false —
`Tests/SiegeControlsHelpTest.cpp:2404`, *"Test 9 walks the whole registry for this"*.
**There is a second, and no gate named it:**

> `SiegeControlsHelpWidget.cpp:1306` — *"…the registry-wide walk that would catch it lives in
> **test 9, EveryRowHasAuthoredDetail**."*

⭐ **That line is in production source and was ADDED BY `TASK-870`'s own diff — in this very
commit.** So the commit that *moves* the walk to TEST 18 also ships a *fresh* pointer to its
old home. Neither is executable and both self-correct in one hop (`:1096`, `:1144` and `:2453`
all point at TEST 18 correctly), so **neither is a blocker** — but WARN-1's suggested fix
(*"one-word repair on whichever row next legitimately edits that file — naturally the same
host as `TASK-857`, so it costs nothing"*) **assumed the host could edit source. It cannot:**
item (9) fences me, and a comment edit is still CODE owing an `SC-§27` verdict. **It needs a
row: two sites, `Test 9` → `TEST 18`.**

**(F-2)** `qa/TASK-856.md` WARN-3's F-1 rider is still unhomed —
`SiegeControlsHelpWidget.cpp:492` says *"the **13** `RelatedActionIds` assignments in this file
were read"*; measured **16**. I re-confirmed **16** blocks in the committed file. The repair
must carry the derivation, never a re-typed number.

**(F-3)** The four records carrying the `+39`/untracked claim need correcting — §3 above.

**(F-4)** `qa/TASK-872.md` W-4 / R-2: **Escape on the war map is still unobserved** and the
page now ships that claim alone. One PIE click for Jonathan: *"open the war map, press Escape —
does it close?"*

---

## 9. LEDGER — `SC-§29`, verdict + report path per row

| row | subject | gate | verdict | in this commit |
|---|---|---|---|---|
| **TASK-857** | `TASK-852`'s edge-integrity extraction (TEST 18) | `qa/TASK-856.md` | ✅ **PASS**, 0 blockers | ✅ **CLOSED** |
| **TASK-873** | `TASK-870`'s war-map help repair (TEST 17) | `qa/TASK-872.md` | ✅ **PASS**, 0 blockers | ✅ **CLOSED** |
| **TASK-871** | four-corner slope trace + NaN repair (1b) | `qa/TASK-914.md` | ✅ **PASS**, 0 blockers | ✅ **CLOSED** |
| **TASK-874** | derived assistant roster (1c) | `qa/TASK-889.md` loop 2 | ✅ **PASS**, 0 blockers | ✅ **CLOSED** |
| **TASK-906** | was to host path 7 | — | — | ⚠️ **SUPERSEDED — re-point to `§7′(e)`** |
| **TASK-920** | the two pixel rows | — | — | ⛔ **OPEN, unobserved** |

**Every row that entered this commit holds a PASS in hand. No ungated code shipped.**
I verified all four gate files myself at my own instant (`SC-§40` cl. 3) rather than
inheriting the dispatch's assertion.

## 10. STATE LEFT BEHIND

- **`main` = `239ca77`, 14 ahead of `origin/main`. ⛔ NOT PUSHED** — Jonathan pushes his own.
- Editor **left CLOSED**, port 8000 clear. I never opened it; that is the orchestrator's row.
- Working tree: `TASK-925`'s 3 files dirty, `TASK-927`'s `Content/FogArea/` untracked, the
  pipeline docs dirty. **Index clean. Nothing reset, nothing stashed.**
- Worktree suite census **432**; `HEAD` **431**. ⚠️ **The `+1` gap is `TASK-923`'s
  `SiegeInvisibilityTest` row and it is `TL-§5d`'s live case** — it counts in the green suite
  above and is invisible to `HEAD`. **`TASK-925` must actually take it**, or `HEAD` silently
  loses a test.
- `TASK-844` is the next writer of `SiegePlayerController.h` — **serialise.**
