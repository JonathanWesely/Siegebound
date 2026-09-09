# TASK-1178 — [FOGPROSE-TRUTH] — gameplay-programmer handoff

**Base `4a3da63`. Two files edited, both `Source/`. ⛔ No compile, no suite, no editor, no MCP, no Git,
no `Content/`, no `Config/`, no `.uasset`, `L_Arena` never opened, `CONVENTIONS.md` untouched.**

| | |
|---|---|
| W1 branch taken | **(b) LABEL IT** — with the real releaser named by symbol **and** by line |
| cl. (5) census verdict | **branch (b): REPORT AND STOP.** UNIVERSAL claims exist **outside** `SiegeFogVisualTest.cpp` |
| declared suite delta | **`0` — a DERIVATION, not a measurement** (`SC-§95`); the derivation is *measured* (§5) |
| did any behaviour move | **NO — shown in §5, not asserted** |

Files edited:
- `Source/GitClaudeUnrealTest/Siegebound/FogVolume.cpp` — **the `:99-104` warning string ONLY** (`+7/−3`)
- `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeFogVisualTest.cpp` — **comments only** (`+79/−14`)

`git diff --numstat` at the git root (**one level above the project dir**, `SC-§102`), with a
deliberate-bad-path negative control run first — the mis-anchored pathspec answered with **silence**,
exit `0`, which is why the anchor was verified rather than assumed:

```
7   3   GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/FogVolume.cpp
79  14  GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeFogVisualTest.cpp
```

---

## 1. W5 — `FogVolume.cpp:99-104`, the shared editor-world warning

The manager's ruling in `SC-§109` cl. 4(d) was read and **not re-litigated**: it is a **false statement**,
conservative in direction, and it rides this compile. Remedy taken = the first of the two the ruling
offers — **state what this command MAY do** — because the second (move the spawn clause onto the `Raise`
path) requires editing `ExecRaiseFog` or the `IsWorldUsable` signature, and **the row's `names:` fence
says `FogVolume.cpp` — the `:99-104` warning string ONLY.** A parameterised gate is a call-site change at
three sites; that is outside the fence and would also be a (tiny) behaviour change. Reported here rather
than done.

**BEFORE** (verbatim, `:100-102`):
```cpp
                TEXT("[%s] ⚠️⚠️ ACTING ON AN ⛔ EDITOR WORLD ('%s'), not a game/PIE world. The fog-state actor is spawned ")
                TEXT("into the map you currently have OPEN, which ⛔ MARKS IT DIRTY. ⛔ DO NOT SAVE THE MAP (`GFX-§11`): ")
                TEXT("discard, or close the editor without saving. Prefer PIE for anything you intend to look at."),
```

**AFTER** (verbatim):
```cpp
                TEXT("[%s] ⚠️⚠️ ACTING ON AN ⛔ EDITOR WORLD ('%s'), not a game/PIE world. ⛔ ANYTHING THIS ")
                TEXT("COMMAND CREATES OR DESTROYS LANDS IN THE MAP YOU CURRENTLY HAVE OPEN, which can leave it ")
                TEXT("MODIFIED. ⛔ THIS GATE IS SHARED BY EVERY Siege.Fog COMMAND, so it states the HAZARD and ⛔ NEVER ")
                TEXT("THE OUTCOME: only the RAISE command can CREATE a fog-state actor — CLEAR and STATUS only ever ")
                TEXT("look one up, and CLEAR destroys the visual it finds. ⛔ READ THE PER-COMMAND LINE PRINTED BELOW ")
                TEXT("THIS ONE for what actually happened. ⛔ DO NOT SAVE THE MAP (`GFX-§11`): discard, or close the ")
                TEXT("editor without saving. Prefer PIE for anything you intend to look at."),
```

**What changed, claim by claim:**
- *"The fog-state actor **is spawned** into the map"* — an **event**, asserted unconditionally on a gate
  that `ExecClearFog` (`:166`, `AFogVolume::Find`) and `ExecLogFogState` also pass. ⇒ replaced by the
  **predicate** *"anything this command creates or destroys lands in the map you currently have open"*,
  which is true on all three paths, plus the **discriminator** naming which path can create.
- *"which ⛔ MARKS IT DIRTY"* — a conclusion about an outcome. ⇒ *"which can leave it MODIFIED"*: what the
  operator should **observe** (`SC-§109`), not what the code concluded.
- **The line now says out loud that it is a HAZARD and never an OUTCOME, and points at the per-command
  line below it** — which is the one place the actual result is printed (`:135-146` raise, `:172-176`
  clear, `:198-206` status). That is the durable half: the gate can never again be read as a report.
- ✅ The *"do not save"* half is **kept**, exactly as QA W5 asked — destroying an actor in an editor world
  can modify the level too, so that instruction is correct on the Clear path.

⚠️ **LEFT UNTOUCHED, DECLARED RATHER THAN SWEPT — `FogVolume.cpp:89-96`.** The design comment directly
above the gate still opens *"Spawning into the world the editor currently has open marks that MAP
DIRTY"*. It is **not** false the way the literal was (it is the justification for **acting rather than
refusing**, and the Raise path really does spawn), but it carries the same Raise-only framing on a
shared gate. It sits **outside `:99-104`** and the row's fence is explicit. ⇒ **routed to the manager**,
not edited.

---

## 2. W1 — `SiegeFogVisualTest.cpp`, the dead teardown net · **BRANCH (b), LABEL IT**

### 2.1 Why (b) and not (a) — the evidence, stated before the remedy

**(a) is structurally unsatisfiable inside this row's own fences, and I will not ship an unproven
connection.** Its acceptance test is *"PROVE `EndPlay` RAN WITH A QUOTED LOG LINE (`SC-§113` cl. 3(c) —
an empty log is NOT a pass)"*, and cl. (6) of the same row says **`YOU DO NOT COMPILE, DO NOT RUN THE
SUITE`**. There is no lane from here to that quoted line. This is the same bind `qa/TASK-1174-report.md`
§0 recorded when it deferred BLOCKER-0 rather than bouncing a diff to an agent barred from satisfying it.

**And (a) is not cheap, which is the condition the row attached to "PREFERRED".** Measured, not assumed:

| what (a) would do | why it is not a comment change |
|---|---|
| `World->GetWorldSettings()->NotifyBeginPlay()` iterates **every actor** and calls `DispatchBeginPlay` | the fixture's actors would start receiving `BeginPlay` |
| it sets `bBegunPlay`, so `AActor::PostActorConstruction` dispatches `BeginPlay` to **actors spawned after that point** | test 10 spawns `AExponentialHeightFog` (`:1622`), `AFogVolume` (via `FindOrSpawn`) and — through `SpawnFogVisual` — the **`BP_SiegeFog` Blueprint**, whose own event graph would then run |
| `RouteEndPlay` would then really route | `AFogVolume::EndPlay` runs at teardown **in addition to** the `ResetFog()` step (5) already performs |

⇒ (a) is a **real test-harness behaviour change** whose blast radius includes running a Blueprint's event
graph, and the row forbids the only instrument that could show it is safe. **A comment claiming a
connected net without a log line is the exact defect W1 is about** — I am not replacing one unproven
promise with another. **(b), with the truth measured and written down, and the connection left to a row
that can run it.**

⛔ **The loop was NOT deleted.** Silent deletion is forbidden and would also be wrong: the loop becomes
correct the instant the rig grows a game mode.

### 2.2 The justification comment (`:1491-1496` before)

**BEFORE** (verbatim, last three lines are the false half):
```
 *  ⚠️ `BeginPlay()` IS CALLED, and the reason is measured rather than assumed: this project has
 *  ⛔ ZERO `UWorldSubsystem`s (all six of its subsystems are `UGameInstanceSubsystem`s, and this
 *  world has no game instance), so `UWorld::BeginPlay` runs ⛔ no project code — it is null-safe
 *  on the absent game mode (`World.cpp`, `GetAuthGameMode()` branch). ⛔ Without it the world
 *  never sets `bBegunPlay`, so `AActor::RouteEndPlay` would be a ⛔ NO-OP at teardown and
 *  `AFogVolume::EndPlay` — which is what RELEASES the integrity floor — would ⛔ never run.
```

**AFTER** — the false sentence is **struck (`~~…~~`) and kept verbatim** (`SC-§53` cl. 3, the same
handling `FogVolume.h:305-346` used), and the measured truth is appended below it. Load-bearing lines:
```
 *  🚨🚨 ⛔ THE STRUCK SENTENCE IS ⛔ FALSE. It is kept verbatim rather than deleted (`SC-§53`
 *  cl. 3) because the belief it records is the one a reader arrives with. ⛔ MEASURED off the
 *  installed 5.8 source (`TASK-1174` W1, corrected by `TASK-1178`):
 *    • `UWorld::BeginPlay()` does ⛔ NOT set `bBegunPlay`. The ⛔ ONLY writer is
 *      `AWorldSettings::NotifyBeginPlay` (`WorldSettings.cpp`, `World->SetBegunPlay(true)`),
 *      reached ⛔ only through `AGameModeBase::StartPlay` → `AGameStateBase::HandleBeginPlay`.
 *    • `InitializeActorsForPlay(FURL())` spawns ⛔ NO game mode, and this world has none.
 *  ⇒ ⛔ `World->HasBegunPlay()` IS FALSE FOR THIS WORLD'S WHOLE LIFETIME, so `AActor::RouteEndPlay`
 *  returns ⛔ without dispatching and ⛔ `AFogVolume::EndPlay` ⛔ NEVER RUNS HERE. …
 *  ⭐ `BeginPlay()` is ⛔ KEPT: it is `FActorTestSpawner`'s own shape, it runs no project code
 *  (measured above), and it is half of what a future game-mode-bearing rig needs. …
 *  ⚖️ ⛔ WHY THIS IS ⛔ LABELLED RATHER THAN ⛔ CONNECTED (`TASK-1178` cl. 1, branch (b)): …
```
(The (b)-vs-(a) reasoning from §2.1 is written into the comment itself, so the next reader does not have
to find this handoff.)

### 2.3 The loop's own comment (`:1541-1545` before)

**BEFORE** (verbatim):
```
            // ⛔ `RouteEndPlay` FIRST, and it is the reason `BeginPlay` was called above: this is
            // what fires `AFogVolume::EndPlay`, which RELEASES the integrity floor. ⛔ A teardown
            // that skipped it would leave `r.VolumetricFog` pinned at `SetByCode` for the ⛔ REST
            // OF THE SUITE PROCESS — 553 other tests running under a console variable this one
            // stranded, with nothing red anywhere.
```

**AFTER** (verbatim, 18 lines; the four load-bearing claims):
```
            // ⛔⛔ INERT UNDER THIS WORLD — ⛔ MEASURED, ⛔ NOT A READING (`TASK-1174` W1; the full
            // derivation is in the `FScopedPlayWorld` doc comment above). `bBegunPlay` is written
            // ⛔ ONLY by `AWorldSettings::NotifyBeginPlay`, which needs a game mode this world does
            // not have. ⇒ `AreActorsInitialized()` is TRUE, the loop runs, and `AActor::RouteEndPlay`
            // returns ⛔ WITHOUT dispatching ⇒ ⛔ `AFogVolume::EndPlay` NEVER RUNS HERE.
            // ⭐⭐ ⛔ THE RELEASE THAT ACTUALLY HAPPENS IS `Volume->ResetFog();` IN STEP (5) OF
            // `FSiegeFogRaisePathActuallyExecutesTest::RunTest` (line 1764 as this was
            // written — ⛔ the SYMBOL is the durable name, the number is a courtesy that goes stale).
            // ⛔ IT IS UNCONDITIONAL, with ⛔ no `return` between it and the raise that engaged the
            // floor. ⛔ THAT is the line that must never be deleted — ⛔ this loop is not.
            // ⛔ THE LOOP IS ⛔ KEPT, ⛔ NOT DELETED: it becomes ⛔ LIVE the instant this world is given
            // a game mode (or `SetBegunPlay(true)`), it is `FActorTestSpawner`'s own teardown shape,
            // and deleting it means re-inventing it blind on the day the rig grows one.
            // ⚠️ WHAT IT WOULD BUY WHEN LIVE, as a predicate rather than a tally (`SC-§104`): a
            // teardown that released nothing would leave `r.VolumetricFog` pinned at `SetByCode` for
            // ⛔ EVERY REMAINING TEST IN THIS PROCESS — a whole suite running under a console variable
            // this one test stranded, with nothing red anywhere. (The count that stood here said 553
            // and was already drifting; ⛔ the hazard does not depend on the number.)
```

**The row required the real releaser NAMED BY LINE, and the row also forbids re-arming numeric traps.
Both are honoured, and the tension is stated in the comment:** the durable name is the **symbol +
enclosing function** (`Volume->ResetFog();` in step (5) of `FSiegeFogRaisePathActuallyExecutesTest::RunTest`);
the line number rides alongside, explicitly labelled *"as this was written … a courtesy that goes stale."*

🚨 **The line number is MEASURED after every insertion in this row, not carried over from the QA report.**
The patch script resolved it programmatically and the verifier re-read it back off the written file:

```
  SiegeFogVisualTest.cpp:1764 = '\tVolume->ResetFog();'
  comment text says            = MATCH
```
(QA's `:1699` was correct pre-edit; my own edits above it moved it by 65 lines. **A number copied from
the report would have shipped stale on the day it was typed** — the exact failure this row exists to end.)

⚠️ **For QA: the `553` in the struck-count sentence is deliberately still visible**, as the record of what
stood there. It is inside a clause that says the number was drifting and the hazard does not depend on it.

---

## 3. W2 — `SiegeFogVisualTest.cpp:97-100`, the header that denied its own file

**BEFORE** (verbatim):
```
 *    • ⛔ **WHAT NEITHER LANE COVERS, STATED SO NO GREEN IS MISTAKEN FOR IT:** there is not one
 *      `SpawnActor` anywhere in `Siegebound/Tests/`, so ⛔ NOTHING HERE RUNS THE ACTUAL SPAWN. The
 *      end-to-end claim — play `Fog`, a box appears; wait 300 s, it goes — is ⛔ NOT EXECUTED by
 *      this file and is ⛔ NOT executed by the suite.
```

**AFTER** — struck verbatim, then three bullets: the **forward pointer** the QA finding asked for, the
**predicate** that replaces the census, and the half that is **still** uncovered:
```
 *    • ⛔ **WHAT NEITHER LANE COVERS…:** ~~there is not one
 *      `SpawnActor` anywhere in `Siegebound/Tests/`, … is ⛔ NOT executed by the suite.~~
 *      🚨 **⛔ STRUCK — ⛔ FALSE, AND THE FILE THAT FALSIFIED IT IS ⛔ THIS ONE.** Kept verbatim
 *      rather than deleted (`SC-§53` cl. 3) so the mistake stays readable. ⭐ **LANE C EXISTS NOW —
 *      TEST 10 at the bottom of this file creates a real `UWorld` and spawns real actors, so the
 *      SPAWN and the DESPAWN ⛔ ARE executed by the suite** (`TASK-1173`; the correction is
 *      `TASK-1178`, from `TASK-1174` W2).
 *    • ⭐⭐ **AND THE REPLACEMENT IS A ⛔ PREDICATE, ⛔ NOT A CENSUS** (`SC-§104`): a count of
 *      `SpawnActor` sites in a directory is true the day it is typed and false the day after —
 *      which is precisely how the struck sentence came to lie in its own file. ⛔ The durable form:
 *      **MOST tests under `Siegebound/Tests/` are headless — pure statics, CDOs and reflection —
 *      and ⛔ THIS FILE IS ONE OF THE ONES THAT IS NOT.** ⛔ Do ⛔ NOT write a fresher number here;
 *      if you need to know whether some OTHER file is headless, ⛔ read that file.
 *    • ⛔ **WHAT IS STILL ⛔ NOT COVERED, WHICH IS THE HALF THAT MATTERED:** the 300-second WAIT is
 *      executed ⛔ nowhere, and ⛔ nothing here says the fog ⛔ LOOKS right — 🧑 his eye remains the
 *      ⛔ ONLY instrument for legibility (`AS-§6 A(e)`).
```

**The header now points FORWARD** (it named nothing before; the pointer at `:1432-1439` only pointed
back), which is what W2 said a reader stopping at `:100` was missing. **The predicate replaces the count
without introducing a new one** — the row's own rule, and the reason "most tests here are headless, this
one is not" survives the next test that grows a world.

✅ Checked and **left as-is**: the quotation at `:1432-1439` (now `:1447-1454`) is framed *"Verbatim, and
**true until this test landed**"* and already says the sentence *"IS NOW HALF FALSE"*. With the header
struck, the two now agree. No edit needed and none made.

---

## 4. W3 / W4 — the two stale counts that ride the same compile (cl. 4)

**W3 — `:1450`, "SEVEN `Error` sites" (measured: SIX).**
BEFORE: `` `FogVolume.cpp` already contains ⛔ SEVEN `Error` sites, and the automation framework routes… ``
AFTER: `` `FogVolume.cpp` already ships ⛔ ITS OWN `Error` sites — ⛔ A PREDICATE, ⛔ NEVER A COUNT
(`SC-§104`). The number that stood here said ⛔ SEVEN and was ⛔ SIX on the day it was typed … a fresher
number would go stale on the next added or reverted site and lie again, silently. ⛔ IF YOU NEED THE SET,
GREP THE FILE for `UE_LOG(LogGitClaudeUnrealTest, Error,` — ⛔ do not trust a tally in a comment, this one
included. ``
⇒ **no number shipped.** The remedy hands the reader the grep that regenerates the answer.

**W4 — `:1474` / `:1479`, "Nine files" (measured: many more, and not confined to `Tests/`).**
BEFORE: `` ⛔ Nine files under `Siegebound/Tests/` currently assert … a nine-file sweep inside a
capability row is how a capability row becomes a refactor. ``
AFTER: `` ⛔ OTHER FILES assert … ⛔⛔ HOW MANY IS DELIBERATELY ⛔ NOT WRITTEN HERE (`SC-§104`): the number
that stood in this sentence said ⛔ NINE and was ⛔ already wrong … replaced it with the ⛔ GREP THAT
REGENERATES THE ANSWER: search `Source/` for ``not one `SpawnActor` `` and ``not one `UWorld::CreateWorld` ``.
⛔ THAT CENSUS WAS RUN (`TASK-1178` cl. 5) and its two load-bearing results are: the set is ⛔ MANY files,
⛔ not nine — and it is ⛔ NOT confined to `Tests/`: `Siegebound/HeroCharacter.h` and
`Siegebound/SiegeLadderClimbStatics.h` carry the same universal claim. ⛔ The list went to the manager;
⛔ sweeping it is a row of its own (`SC-§100`). … an N-file prose sweep inside a capability row is how a
capability row becomes a refactor. ``
⇒ again **no number shipped**, and the one durable fact the census produced (*it escapes `Tests/`*) is
recorded where the next reader is standing.

---

## 5. ⭐ ZERO BEHAVIOUR CHANGE — **SHOWN, NOT ASSERTED**

Three independent instruments, all run against a byte-copy of the pre-edit files
(`scratchpad/FogVolume.BEFORE.cpp`, `scratchpad/SiegeFogVisualTest.BEFORE.cpp`). Scripts:
`scratchpad/task1178_patch.py` (each old block asserted to match **exactly once** or abort) and
`scratchpad/task1178_verify.py`.

### (a) Comment-stripped equivalence — the strongest one
A real C++ comment stripper (state machine: tracks string literals, char literals, `//`, `/* */`) was run
over before and after. **String literals are KEPT**, so a changed literal cannot hide.

```
SiegeFogVisualTest.cpp  comment-stripped IDENTICAL : True
FogVolume.cpp           comment-stripped IDENTICAL : False
```
⇒ **`SiegeFogVisualTest.cpp` has ZERO non-comment change of any kind** — every one of its 79 added and 14
removed lines is a comment. Not "I only meant to edit comments": **the compiler-visible text is
byte-identical.**

⇒ `FogVolume.cpp`'s **entire** non-comment diff, in the whole task, is the one `UE_LOG` argument:
```
-  TEXT("[%s] ⚠️⚠️ ACTING ON AN ⛔ EDITOR WORLD ('%s'), not a game/PIE world. The fog-state actor is spawned ")
-  TEXT("into the map you currently have OPEN, which ⛔ MARKS IT DIRTY. ⛔ DO NOT SAVE THE MAP (`GFX-§11`): ")
-  TEXT("discard, or close the editor without saving. Prefer PIE for anything you intend to look at."),
+  (the 7-line replacement quoted in §1)
```
**No control flow, no call, no signature, no declaration, no `#if`, no include** — the `if
(!World->IsGameWorld())` guard, the `return true;`, and `CommandName, *World->GetName()` are untouched,
and the diff hunk shows them as context on both sides.

**And nothing in the project asserts on that string.** Grepped `Source/`, `Tools/` and `CONVENTIONS.md`
for `MARKS IT DIRTY` / `ACTING ON AN` / `not a game/PIE world`: the **only** hits are the two lines being
replaced. The two `IsWorldUsable` call sites (`:120`, `:161`) test the **`bool`**, which is unchanged on
every path.

### (b) Structural-census regression — 42 needles, 0 drift
`CountOccurrencesInCode` (`SiegeFogVisualTest.cpp:188`) was **ported to Python line for line**, including
its comment-line skip rule, and every needle any test in the suite counts against `FogVolume.cpp` was run
before and after — the 8 banned geometry literals, the banned companion flags, the `ExtractFunctionBody`
signatures, the call-count pins, `TEXT("r.VolumetricFog`, `SpawnActor<AFogVolume>`, the forbidden
shortcuts:

```
>>> census needles that DRIFTED: 0
```
(full 42-row table in the verifier's stdout; every row `before == after`.)

⚠️ This was **designed for, not discovered**: the replacement string was written with **no digits at all**
precisely because `SiegeFogVisualTest.cpp:784` substring-matches `640|360|260|32000|18000|7000|26000|12000`
on **code lines**, and a `UE_LOG` literal **is** a code line. It also avoids every
`AFogVolume::…(`-shaped signature so no `ExtractFunctionBody` first-match can be hijacked by a string that
sits **above** the real definitions.

### (c) Suite delta — `0`, and it is a **DERIVATION** (`SC-§95`)
```
SiegeFogVisualTest.cpp   IMPLEMENT_SIMPLE_AUTOMATION_TEST  before=10  after=10
SiegeFogVisualTest.cpp   IMPLEMENT_ (any)                  before=10  after=10
FogVolume.cpp            IMPLEMENT_ (any)                  before=0   after=0
```
⛔ **Labelled honestly: I did not run the suite (cl. 6 forbids it), so `0` is DERIVED.** What is
**measured** is the input the derivation rests on: the `IMPLEMENT_` count is unchanged, and (a) proves the
test file's compiler-visible text is byte-identical — a file whose non-comment bytes did not move cannot
register, deregister or rename a test. **Branch (1)(a) was not taken, so the "still 0 even under (a)"
clause does not arise** — but for the record it would also have been 0, since (a) adds no macro either.
**`TASK-1180` owes the EXECUTED `N/M`, reconciled against its own previous run, never a published
absolute.**

### (d) Line endings — CRLF round-trip
Both files are `UTF-8, CRLF`. The patch read with `newline=''` and wrote with `newline=''` (no
translation) at every step:
```
bare LF in FogVolume.cpp   : 0
bare LF in SiegeFogVisual  : 0
FogVolume.cpp   lines: 1550 -> 1554
SiegeFogVisual  lines: 1731 -> 1796
```
⇒ no whole-file reflow, so the numstat above is the real edit and not a line-ending artefact.

---

## 6. ⭐ cl. (5) — THE CENSUS, AND ITS STOP RULE FIRES: **BRANCH (b), REPORT AND STOP**

**The prescribed census, run exactly as written** (`grep -rn -i "headless"` over
`Source/GitClaudeUnrealTest/Siegebound/Tests/`):

| | |
|---|---|
| occurrences | **47** (the manager's sample said 46 — same order, minor grep-form difference; I report **my** measurement) |
| files | **18** — matches the manager's 18 exactly |
| **SCOPED** (a claim about the file's own reasoning — ⛔ STILL TRUE, ⛔ NOT TOUCHED) | **42** |
| **UNIVERSAL** (a claim about the whole tree — now FALSE) | **5** |

**The 5 UNIVERSAL ones, by file and line:**

| # | site | text | in scope? |
|---|---|---|---|
| 1 | `SiegeFogVisualTest.cpp:1475` | *"every automation test in this project is HEADLESS — there is not one `UWorld::CreateWorld`…"* | ✅ **MINE — corrected by cl. (4)/W4 above** |
| 2 | `SiegeHeroCameraTest.cpp:43` | *"Every automation test in this project is HEADLESS (`SiegeLadderClimbTest.cpp:39`: not one `UWorld::CreateWorld`, not one `SpawnActor` in `Siegebound/Tests/`)"* | ⛔ **OUTSIDE** |
| 3 | `SiegeHeroLadderClimbTest.cpp:34` | *"Every automation test in this project is HEADLESS: there is not one `UWorld::CreateWorld`…"* | ⛔ **OUTSIDE** |
| 4 | `SiegeLadderClimbTest.cpp:41` | *"Every automation test in this project is HEADLESS. There is not one `UWorld::CreateWorld`…"* — ⭐ **THE ORIGIN; the other three cite it as `SiegeLadderClimbTest.cpp:39`** | ⛔ **OUTSIDE** |
| 5 | `SiegeRecallTest.cpp:51` | *"Every automation test in this project is HEADLESS: there is not one `UWorld::CreateWorld`…"* | ⛔ **OUTSIDE** |

⚖️ **THE STOP RULE, APPLIED TO MY OWN NUMBERS AS IT WAS WRITTEN BEFORE THE ANSWER:**
> *(a) UNIVERSAL ≤ 2 **and** all inside `SiegeFogVisualTest.cpp` ⇒ cl. (2) discharges it, say so and close.*
> *(b) UNIVERSAL > 2 **or** any outside that file ⇒ **REPORT THE LIST AND STOP.***

**UNIVERSAL = 5 (> 2), and 4 of the 5 are outside the file.** ⇒ **BRANCH (b). I stopped. Nothing outside
`SiegeFogVisualTest.cpp` and the one `FogVolume.cpp` literal was edited.**
⇒ **The manager's own suspicion — *"possibly a one-file job that cl. (2) already covers"* — is REFUTED by
the measurement, and the refutation is the useful output of the clause.**

### 6.1 ⚠️ AND THE PRESCRIBED NEEDLE UNDER-COUNTS — declared, not hidden (`SC-§101`)

**The word `headless` is not the load-bearing needle.** `SiegeFogVisualTest.cpp:97-100` — the site W2
exists about — is a **universal claim that never says "headless"**. Running the census on the actual
sentence (``not one `SpawnActor` `` / ``not one `UWorld::CreateWorld` `` / ``anywhere in
`Siegebound/Tests/` ``) over **all of `Source/`**, the universal set is:

`SiegeAcquisitionFunnelTest.cpp:43` · `SiegeBrightSunTest.cpp:56` · `SiegeCastBarTest.cpp:65` ·
`SiegeFogClampTest.cpp:60` · `SiegeFogReachSeamTest.cpp:48` · `SiegeFogRefusalTest.cpp:62,428,1031` ·
`SiegeFogVolumeTest.cpp:46` · `SiegeHeroCameraTest.cpp:44` · `SiegeHeroLadderClimbTest.cpp:34-35` ·
`SiegeInvisibilityTest.cpp:154,2054,2108,2809,3216` · `SiegeLadderClimbTest.cpp:41-42` ·
`SiegeRecallTest.cpp:51-52` · `SiegeRespawnLifecycleTest.cpp:53` · `SiegeUnitNoticeRangeTest.cpp:77` ·
`SiegeWarMapTest.cpp:3384` — **plus two files OUTSIDE `Tests/` entirely:**
🚨 **`Siegebound/HeroCharacter.h:96-97`** and 🚨 **`Siegebound/SiegeLadderClimbStatics.h:155`**.

⇒ **~16 files, and the stale claim has escaped the test tree into production headers.** `TASK-1173`'s
"~15 files" was **closer to right than the `headless` grep suggests**, and the manager's honest
downgrade of it (`SC-§101`) was the correct call — the derivation was sound, the sampling needle was not.
**This list is the cl. (5) deliverable, routed to the manager. ⛔ I did not sweep it** (`SC-§100`).

**A note for whoever boards that sweep:** it is **one edit and N citations** — four of the five `headless`
sites and most of the phrase sites cite `SiegeLadderClimbTest.cpp:39` as the origin. Fixing the origin
and pointing the citations at it is a smaller row than 16 independent rewrites. **And the remedy shape is
already settled by this row: a PREDICATE, never a fresher count** — the correction that ships a new
number is the one that gets boarded again in a month.

---

## 7. ⛔ WHAT QA SHOULD SCRUTINISE (`TASK-1179`)

1. 🚨 **The first duty, and it is right:** *did any behaviour move?* §5(a) is the instrument I would use —
   **re-run the comment-strip diff yourself**; `SiegeFogVisualTest.cpp` must come back **byte-identical**
   and `FogVolume.cpp` must differ in **exactly one `UE_LOG` argument**. If either is false, fail me.
2. **The W1 ruling is mine to defend, not to assume.** I took **(b)**. If you judge that (a) was
   reachable inside cl. (6)'s fence, that is a real finding — but §2.1 argues no lane exists to the
   quoted log line, and that (a) would run a Blueprint's `BeginPlay` unmeasured.
3. **The line number `1764`.** It is the one re-armed trap I deliberately shipped, because the row ordered
   it. Verify it points at `Volume->ResetFog();` **in the written file** (I did: it does). It is labelled
   as perishable and the symbol carries the meaning.
4. **`553` survives inside the struck-count clause** at the loop. Deliberate. Overturn it if you read it
   as a fresh assertion rather than a record.
5. **`FogVolume.cpp:89-96` was NOT edited** (§1) — fence, not oversight. Confirm the fence reading.
6. **cl. (5) branch (b) — check my classification**, especially that the 42 I called SCOPED really are
   about their own files. The four outside-file UNIVERSAL sites are quoted in §6 with line numbers.
7. **The census under-count in §6.1** is a claim about the *manager's needle*, not about her judgement.
   It is offered as evidence for the sweep row's scope, and it is a **grep**, not an exhaustive read.

## 8. FENCES — DISCHARGED
`Content/**` 0 · `Config/**` 0 · `.uasset` 0 · `L_Arena` never opened (no editor started) · **no MCP
call made** (🧑 the editor stayed his) · no compile · no suite · **no Git command that writes** (two
read-only `git diff --numstat` / `git status` only) · never pushed · `CONVENTIONS.md` untouched
(findings routed here) · `FogVolume.h` untouched · **no other `Tests/*.cpp` touched** ·
`Tools/**` untouched (⚠️ `TASK-1181`'s concurrent lane — the `?? Tools/…` entries in `git status` are
**its**, not mine).


---
---

# TASK-1178 — **rev-2** (`TASK-1179` loop 1 → WARN-1)

> ⛔ **Everything above this line is rev-1 and is PRESERVED VERBATIM.** Nothing in it was edited,
> struck or renumbered. This section only *corrects* rev-1 where rev-1 was wrong, and it says so.

| | |
|---|---|
| what this rev fixes | **`TASK-1179` WARN-1** — my rev-1 replacement literal repeated the very defect the row exists to end |
| files edited this rev | **`FogVolume.cpp` ONLY** (`SiegeFogVisualTest.cpp` untouched — re-proved below) |
| `:89-96` decision | ⭐ **FIXED IN THE SAME BREATH** — reasoning in §R2.3 |
| did any behaviour move | **NO — shown in §R2.5, not asserted**: the code outside the one `UE_LOG` literal is **byte-identical, in order** |
| new digits shipped in the literal | **ZERO** (§R2.5 d) |
| `WARN-2` (`SiegeFogVisualTest.cpp`) | ⛔ **NOT TOUCHED** — out of this rev's scope, declared in §R2.6 |

---

## R2.1 The defect I shipped, restated as measurement

My rev-1 literal said **"⛔ THIS GATE IS SHARED BY EVERY `Siege.Fog` COMMAND"** and then named a
roster (*"only the RAISE command can CREATE … CLEAR and STATUS only ever look one up"*).

**Measured, post-edit, off the written file (section E of the verifier):**

```
  IsWorldUsable definition       FogVolume.cpp:77
  IsWorldUsable CALL SITE        FogVolume.cpp:127   (inside ExecRaiseFog,     :123)
  IsWorldUsable CALL SITE        FogVolume.cpp:168   (inside ExecClearFog,     :164)
  command entry point            FogVolume.cpp:200   ExecLogFogState  <-- NO CALL
```

`ExecLogFogState` carries its own inline `if (!World)` and **no editor-world warning whatsoever**, so
`Siege.Fog.Status` never prints that line. **QA is right and the claim was wider than the truth.**
⚠️ Worse than QA's own summary: my roster did not merely *over-scope the gate*, it **described the
behaviour of a command that never reaches the gate**. It was the W5 shape, inside the W5 fix.

⛔ **I did not take QA's suggested wording.** Its suggestion (*"SHARED BY THE TWO WRITING COMMANDS"*)
names a **count**, which is a re-armed trap true only until the next command is added — the same class
of defect as the `553`, the `SEVEN` and the `Nine` this row already replaced with predicates.
**The rule this row itself established was applied to it: state the PREDICATE, never the roster.**

---

## R2.2 EDIT 1 — the literal (`FogVolume.cpp`, now `:102-109`)

**BEFORE** (rev-1, then at `:100-106`):
```cpp
                TEXT("[%s] WARN ACTING ON AN [X] EDITOR WORLD ('%s'), not a game/PIE world. [X] ANYTHING THIS ")
                TEXT("COMMAND CREATES OR DESTROYS LANDS IN THE MAP YOU CURRENTLY HAVE OPEN, which can leave it ")
                TEXT("MODIFIED. [X] THIS GATE IS SHARED BY EVERY Siege.Fog COMMAND, so it states the HAZARD and [X] NEVER ")
                TEXT("THE OUTCOME: only the RAISE command can CREATE a fog-state actor - CLEAR and STATUS only ever ")
                TEXT("look one up, and CLEAR destroys the visual it finds. [X] READ THE PER-COMMAND LINE PRINTED BELOW ")
                TEXT("THIS ONE for what actually happened. [X] DO NOT SAVE THE MAP (GFX-11): discard, or close the ")
                TEXT("editor without saving. Prefer PIE for anything you intend to look at."),
```
*(emoji rendered as `WARN` / `[X]` in this quotation only, so a copy-paste out of this handoff can
never be mistaken for the source; the shipped bytes are the ones in the file and in the diff below.)*

**AFTER** — the middle sentence, and only it, is replaced. The shipped text of the replaced sentence:
```
MODIFIED. THIS SENTENCE IS THE HAZARD AND NEVER THE OUTCOME: it is the SAME text for
every command routed through this gate, and it is printed BEFORE that command acts - so it
can tell you NOTHING about what was created, found or destroyed. THE PREFIX NAMES THE
COMMAND; THE PER-COMMAND LINE PRINTED BELOW THIS ONE IS WHAT ACTUALLY HAPPENED. Read that
line - do not infer an outcome from this one.
```
The opening (`ACTING ON AN EDITOR WORLD … can leave it MODIFIED`) and the closing (`DO NOT SAVE THE
MAP … Prefer PIE`) are **carried over unchanged** — QA passed both.

**Every claim in the new sentence, and how it is checkable without trusting me:**

| clause | why it is true, and why it stays true when a command is added |
|---|---|
| *"it is the SAME text for every command routed through this gate"* | there is **one** literal in **one** function; a new caller gets this exact string by construction. ⛔ It says nothing about **which** commands route here. |
| *"printed ⛔ BEFORE that command acts"* | `IsWorldUsable` is called at the **top** of each caller (`:127`, `:168`), before `FindOrSpawn` / `Find`. |
| *"can tell you ⛔ NOTHING about what was created, found or destroyed"* | the gate has **no knowledge** of the outcome — it runs before any of it. This is the durable half. |
| *"THE PREFIX NAMES THE COMMAND"* | `[%s]` ← `CommandName`, still the single-source `CommandName*` constant (`:66-68`; one-home law verified below). |
| *"THE PER-COMMAND LINE PRINTED BELOW THIS ONE IS WHAT ACTUALLY HAPPENED"* | every continuation path in both callers prints a line (`ExecRaiseFog` on both the null-volume and the executed branch; `ExecClearFog` likewise). Unchanged from rev-1; QA did not contest it. |

⭐ **What is deliberately absent: any command name, any count, any "shared by N".** The sentence is
now a statement about **this line's own position in time** — which cannot go stale — instead of a
statement about a roster, which goes stale the moment `SiegeFogDevTrigger` grows a fourth command.
⭐ `SC-§109` — every verb is something the reader can **observe** (*read that line*, *the prefix
names*), and the one imperative is a prohibition on **concluding** (*do not infer an outcome*).

---

## R2.3 ⚖️ EDIT 2 — `:89-96` — **FIXED, NOT ROUTED. The decision, and why it reversed.**

**Decision: I fixed it, in the same breath.** The rev-1 fence reading is superseded — not because it
was a bad reading, but because the orchestrator **re-opened it in the dispatch that ordered this rev**,
which is the only authority that can (`SC-§100`: a rescope is a board edit, and the board's owner made
it).

The rev-1 reasoning I record as **defeated**: I read the row's `names:` fence as *"`FogVolume.cpp` —
the `:99-104` warning string ONLY"*, and `TASK-1179` §1.3 **upheld** that reading. Both were right on
the day. What changed is not the fence but the **fact on the ground**: after Edit 1, the design comment
eight lines above the gate carried the **same false framing as the clause I had just removed** — and
QA's WARN-1 is precisely about a fix that leaves its own shape behind. ⛔ Shipping a corrected literal
under an uncorrected comment reading *"Spawning … marks that MAP DIRTY"* would have taught the next
reader the roster framing anyway, from four lines higher up.

**BEFORE** (rev-1 `:89-91`, the false half only):
```
        // THE EDITOR-WORLD HAZARD, SAID OUT LOUD RATHER THAN REFUSED. Spawning into the world
        // the editor currently has open marks that MAP DIRTY, and this project's standing law is
        // that L_Arena is NEVER saved (GFX-11). Refusing here would be worse: it would
```

**AFTER** (now `:89-93`; the remainder of the block — the commandlet justification and the
`RF_Transient` star — is **reflowed only, its words unchanged**):
```
        // THE EDITOR-WORLD HAZARD, SAID OUT LOUD RATHER THAN REFUSED. Creating or destroying
        // actors in the world the editor currently has open can leave that MAP MODIFIED, and this
        // project's standing law is that L_Arena is NEVER saved (GFX-11). The hazard is a
        // PREDICATE, never one command's behaviour: it holds for whatever command is routed through
        // this gate, and it does not depend on which of them can spawn. Refusing here would be
```

The same two corrections the literal took, plus one:
- *"**Spawning** into the world …"* — a **Raise-only event** on a shared gate ⇒ *"**Creating or
  destroying actors** in the world …"*, the predicate that holds on every path that reaches here.
- *"**marks that MAP DIRTY**"* — a **conclusion about an outcome** ⇒ *"**can leave that MAP
  MODIFIED**"*: what the operator should **observe** (`SC-§109`), matching the literal word for word.
- ➕ one added sentence states the rule out loud, so the next editor does not re-introduce a roster.

⛔ **This edit is a `//` comment and is therefore census-inert by construction**, not by luck:
`CountOccurrencesInCode` (`SiegeFogVisualTest.cpp:188`) **skips any line whose trimmed start is `//`**
— I re-read that skip rule off the shipped helper rather than trusting rev-1's note about it. And the
comment stripper below erases the whole block from **both** sides, which is exactly why §R2.5 (a)
shows the literal as the **only** difference.

---

## R2.4 ⚠️ RE-CHECKED LINE CITATIONS (the rev-1 lesson, applied to rev-2)

rev-2 adds **+3 net lines** to `FogVolume.cpp` (1555 → 1558), so **every FogVolume citation below the
gate moved.** Measured off the **written file**, not carried over:

| what | rev-1 / QA cited | **rev-2 MEASURED** |
|---|---|---|
| `IsWorldUsable` definition | `:77` | **`:77`** (above the edit — unmoved) |
| `IsWorldUsable` call site — `ExecRaiseFog` | `:124` | **`:127`** |
| `IsWorldUsable` call site — `ExecClearFog` | `:165` | **`:168`** |
| `ExecRaiseFog` entry | `:123` | **`:123`** |
| `ExecClearFog` entry | `:164` | **`:164`** |
| `ExecLogFogState` entry (⛔ the non-caller) | `:197` | **`:200`** |
| `if (!World->IsGameWorld())` — the gate | `:97` | **`:99`** |
| the edited literal | `:100-106` | **`:102-109`** (args on `:110`) |
| the edited design comment | `:89-96` | **`:89-98`** |
| ⭐ **`SiegeFogVisualTest.cpp:1764`** — QA's confirmed releaser | `:1764` | **`:1764`, re-read back: `Volume->ResetFog();` — MATCH** |

🚨 **`:1764` is in the OTHER file, which rev-2 did not touch at all** — so it could not have moved;
it was **re-read off disk anyway** rather than assumed, because *"it can't have moved"* is exactly the
reasoning that ships a stale number. **QA's `:1764` stands, post-rev-2.**

⚠️ **A correction to rev-1's own prose, recorded rather than silently fixed** (`TASK-1179`'s
"handoff prose only" note): rev-1 §5(a) cited the call sites as `:120, :161`; they were `:124, :165`
at the time, and are **`:127, :168`** now. Rev-1's body is preserved above with that error intact —
this row is the correction. ⛔ Nothing shipped ever depended on it.

---

## R2.5 ⭐ ZERO BEHAVIOUR CHANGE — **SHOWN FOR THE NEW DELTA, NOT ASSERTED**

Instruments: `scratchpad/task1178_rev2_patch.py` (every anchor must match **exactly once** or the
script **aborts** — ⭐ **it did abort on the first run**: `CommandName, *World->GetName());` occurs
**twice** in the file, the second inside `ExecLogFogState`, so a blind replace would have rewritten
the wrong command's log line; the anchor was scoped to the block before proceeding) and
`scratchpad/task1178_rev2_verify.py`. Baseline byte-copy: `scratchpad/FogVolume.REV1.cpp`
(`sha256 16a970f7…`, 97 572 bytes, 1554 CRLF, 0 bare LF).

### (a) Comment-stripped equivalence — rev-1 → rev-2
A real C++ stripper (string- and char-literal aware). **Comments are removed; string literals are
KEPT**, so a changed literal **cannot** hide and a changed `//` comment **must** vanish.

```
unified-diff lines : 18      <- ONE hunk
```
and that one hunk is, in full, the five old `TEXT(...)` lines out and six new ones in — quoted in
§R2.2. ⛔ **The `:89-98` comment edit contributes ZERO lines to this diff**, which is the proof that
Edit 2 is a comment and not code.

### (b) ⭐ THE POSITION CENSUS — offset-aware, because rev-2 changes the line count
A naive positional zip is **worthless** across a `+1` code-line shift — it reported 637 "changes", all
of them alignment noise. ⛔ I am declaring that rather than quoting it as a result. The instrument that
actually answers the question, run two ways:

```
CODE OUTSIDE THE ONE LITERAL, ORDERED, BYTE-IDENTICAL : True   (664 vs 664 lines)
code-line MULTISET entries that differ                : 11
   ...of which NOT a TEXT() literal line              : 0
```
⇒ excise the one `UE_LOG` literal block and **the remaining 664 compiler-visible lines are identical
in content AND in order**. No control flow, no call, no signature, no declaration, no `#if`, no
include, no brace moved. The `if (!World->IsGameWorld())`, the `return true;` and
`CommandName, *World->GetName())` appear as **context on both sides** of the hunk.

### (c) Needle census — the suite's own counter, ported line for line
56 needles (the 8 banned geometry literals, the banned companion flags, the `ExtractFunctionBody`
signatures, the call-count pins, `TEXT("r.VolumetricFog`, `SpawnActor<AFogVolume>`, plus **the tokens
this edit removed**):

```
census needles checked: 56    DRIFTED: 1
DRIFT  Siege.Fog        rev1=4  rev2=3
```
⭐ **The one drift is declared, chased to ground, and is inert:**
- it is the **bare** `Siege.Fog` I deleted from the phrase *"EVERY `Siege.Fog` COMMAND"*;
- the **three full command names are unchanged and each still appears EXACTLY ONCE in code** —
  `Siege.Fog.Raise` = 1, `Siege.Fog.Clear` = 1, `Siege.Fog.Status` = 1 — so the **one-home law** at
  `:60-65` (*a name typed at both the registration and the log line is two copies that can drift*)
  **is intact, and this census is the proof of it**;
- ⛔ **nothing counts it.** Grepped `Source/`, `Tools/` and `Config/`: the only consumers of these
  strings outside `FogVolume.cpp` are `FogVolume.h:930` (prose) and `Tools/run_suite_bounded.ps1`
  (`:60`, `:548`, `:552`), which **invokes the commands by their full names** — names this edit did
  not touch. ⚠️ `Tools/` is another lane's file and was **read only**. **No test in
  `Siegebound/Tests/` mentions `Siege.Fog` at all**, so no assertion can see this drift.

### (d) ⛔ NO DIGITS ADDED — the rev-1 finding still binds
```
DIGITS in the new literal : ['1']       <- the 1s of the GFX-11 law tag, present in rev-1 too
banned geometry literals as substrings : []
%s in literal : 2   (args: CommandName, *World->GetName() = 2)   other % : 0
```
`SiegeFogVisualTest.cpp:784` substring-matches `640|360|260|32000|18000|7000|26000|12000` on **code
lines**, and a `UE_LOG` literal **is** a code line. ⛔ **No new digit was introduced**, format arity is
unchanged (2 specifiers, 2 arguments), and the new text contains no `AFogVolume::…(`-shaped token that
an `ExtractFunctionBody` first-match could latch onto.

### (e) Suite delta — `0`, still a **DERIVATION** (`SC-§95`), never a measurement
```
FogVolume.cpp            IMPLEMENT_ (any)  rev1=0     rev2=0
SiegeFogVisualTest.cpp   IMPLEMENT_ (any)  before=10  after=10   (file untouched this rev)
```
⛔ I did not compile and did not run the suite. **`TASK-1180` still owes the EXECUTED `N/M`.**

### (f) Line endings + numstat
`UTF-8, CRLF` throughout; read and written with `newline=''`. **0 bare LF** in the written file;
`1555 → 1558` lines. `git diff --numstat` at the **git root, one level above the project dir**
(`SC-§102`) — with the mis-anchored pathspec run **first** as a negative control, which answered with
**silence and exit 0**, which is why the anchor is verified rather than assumed:

```
(mis-anchored)  ->  <no output>, exit 0
16   9   GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/FogVolume.cpp    <- was 7/3 at rev-1
79  14   GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeFogVisualTest.cpp  <- UNCHANGED
```
`git status` over `Source/`, `Content/` and `Config/` lists **exactly those two files and nothing else.**

---

## R2.6 ⛔ WHAT THIS REV DELIBERATELY DID **NOT** DO

- **`TASK-1179` WARN-2** (`SiegeFogVisualTest.cpp:1505` — the *"the claim they each make is about their
  OWN reasoning"* clause that contradicts the census three lines above it). ⛔ **Not touched.** The
  dispatch that ordered this rev scoped it to *"one clause"* in `FogVolume.cpp` and said *"straight
  back to gate"*, and `SiegeFogVisualTest.cpp` is byte-identical this rev (§R2.5 a). ⚠️ **QA's finding
  is CORRECT and I am not disputing it** — it is live and unaddressed, and it should be boarded or
  folded into `TASK-1180`. ⛔ I am flagging it, not quietly dropping it.
- ⛔ No compile, no suite, no editor, no MCP, no Git write, no `Content/`, no `Config/`, no `.uasset`;
  `L_Arena` never opened; `Tools/**` **read-only** (another lane owns it); `FogVolume.h` untouched;
  `CONVENTIONS.md` untouched; no other `Tests/*.cpp` touched.

---

## R2.7 ⛔ WHAT QA SHOULD SCRUTINISE THIS LOOP

1. 🚨 **Re-run §R2.5 (a)+(b) yourself.** The claim is exact: strip comments from
   `scratchpad/FogVolume.REV1.cpp` and from the shipped file, and the **only** difference must be the
   one `UE_LOG` literal; excise that block and the remaining **664** code lines must be identical **in
   order**. If either is false, fail me.
2. **Does the new sentence over-claim anywhere?** It is deliberately narrower than QA's own suggested
   wording. Check especially *"printed BEFORE that command acts"* against both call sites
   (`:127`, `:168`) — that is the one clause a future caller could falsify.
3. ⚖️ **The `:89-96` reversal is mine to defend.** `TASK-1179` §1.3 upheld leaving it; I edited it on
   the orchestrator's explicit re-open. If you read that as a scope breach rather than a board edit,
   that is a real finding — the edit is a `//` comment and reverting it costs nothing.
4. **The `Siege.Fog` census drift (4→3)** — confirm the one-home law is intact (each full command name
   still exactly 1) and that no consumer counts the bare token.
5. **The moved citations in §R2.4.** Every FogVolume line at or below `:89` shifted. Re-read them off
   the written file, not off this table.
6. **WARN-2 is still open** (§R2.6). I want that stated in your loop-2 report rather than closed by
   silence.
