# TASK-1068 — [FOGVIS-CALLER] the fog visual gets a caller: spawned when fog rises, destroyed when fog ends

**Agent:** gameplay-programmer · **Date:** 2026-09-05 · **Status:** `ready-for-qa`
**Gate:** `TASK-1069` (qa-reviewer) · **Host:** `TASK-1070` (build-master)
**Base:** `b6a44b8`, index empty. ⛔ **Nothing staged, nothing committed, nothing pushed.**
**Marker:** `TASK-1068-FOGVIS-CALLER`

---

## 0. 🚨 READ THIS FIRST — THE HONEST SCOPE OF EVERY GREEN THIS ROW CAN PRODUCE

⛔⛔ **AN AUTOMATED TEST CAN PROVE THE SPAWN HAPPENS. ⛔ IT CANNOT PROVE THE FOG LOOKS RIGHT.**
🧑 **Jonathan's eye is the only instrument for legibility** (`AS-§6 A(e)`), and the stake is
concrete: under fog every unit on both sides is **87.8% blind and drops its target**, so a visual
that reads as **light haze** is a mismatch between what he sees and what the simulation is doing.
⇒ **A fully green suite does not close that question.** It closes "is there a caller, and does
every exit reconcile?".

⚠️ And **do not try to settle it with a screenshot**: `TASK-841` §3.2 measured **55.8% vs 100.8%**
obscuration at **identical settings** — UE's volumetric fog is temporally accumulated, so a single
capture is not converged (`SC-§88`). A fog number needs a **convergence series**, never a shot.

🚨 **SECOND DECLARATION, `SC-§83`: I could not compile and could not run the suite.** Every red
below is **named and mechanically demonstrable**, not witnessed. §8 hands the host the exact
mutations. **A declared red is not a witnessed one, and I am not claiming one.**

---

## 1. FILES TOUCHED

| file | what |
|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/FogVolume.h` | new statics + private lifetime seam + 5 comment repairs the diff made necessary + the 🚩 ruling |
| `Source/GitClaudeUnrealTest/Siegebound/FogVolume.cpp` | the reconciler, the spawn, the despawn, three call sites, the derived transform, the class path |
| `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeFogVisualTest.cpp` | **NEW** — 5 automation tests |
| `.claude/pipeline/handoffs/TASK-1068-programmer.md` | this |
| `.claude/pipeline/TASKBOARD.md` | **TASK-1068's own `status:` line only** |

⛔ **`SpellLibrary.cpp` is BYTE-UNTOUCHED** — I did not need it, and the state owner is the better
home (see §5). ⛔ `Content/FogArea/**` never opened. ⛔ `L_Arena.umap` never saved, never loaded.
⛔ `CONVENTIONS.md` untouched. ⛔ `Tests/SiegeFogVolumeTest.cpp`, `Tests/SiegeBrightSunTest.cpp`,
`Tests/SiegeFogRetentionWiringTest.cpp` never opened.
✅ Pre-flight re-verified per the row: `git status --porcelain` on `FogVolume.{h,cpp}` +
`SpellLibrary.cpp` was **empty** before my first write.

**Asset referenced, never edited:** `/Game/Blueprints/BP_SiegeFog.BP_SiegeFog_C`.

---

## 2. THE MECHANISM I CHOSE — **ONE RECONCILER + ONE DERIVED WAKE-UP**

```
RaiseFog()        ─┐
ApplyBrightSun()  ─┼──► RefreshFogVisual()  ──► IsFogActive() ? SpawnFogVisual() : DestroyFogVisual()
ResetFog()        ─┤                        └──► SetTimer(rate = FogActiveUntilTimeSeconds − now)
expiry wake-up    ─┘                                     (re-enters the same function)
```

**`RefreshFogVisual()` makes the world agree with `IsFogActive()`.** It is the only function in the
project that spawns or destroys the fog visual. Every writer of a deadline calls it **immediately
after it writes** — one identical line, unconditionally, on the success path.

### Why a reconciler rather than a spawn-here / despawn-there pair
- A pair has to be **correct at four sites** (three writers + the timer). A reconciler is
  **idempotent**, so each site is the same line and a **fourth writer added later gets the visual
  right by copying one call** rather than by understanding the mechanic.
- It makes the "half a seam" failure structurally impossible: spawn and despawn are **two branches
  of one `if`**, so a diff cannot delete one without the other becoming obviously unbalanced.

### Rejected alternatives, and why
| rejected | why |
|---|---|
| **`Tick` on `AFogVolume`** | Banned by the row and by `TASK-004`'s never-per-tick law; 60 polls a second re-asking a question whose answer is already known. `PrimaryActorTick.bCanEverTick` **stays `false`** and a test asserts it. |
| **`SetLifeSpan()` on the spawned visual** | Tempting (no timer handle at all) but **wrong in the dangerous direction**: a lifespan fires **unconditionally**, without consulting `IsFogActive()`. If it ever fired while fog was still live, the box would die and **never come back** — permanent clear during live fog, with the mechanic still blinding everyone. It also puts a deadline **on the visual**, i.e. exactly the "the visual owns state" shape the row forbids. |
| **Poll from the acquisition seam / `AFogVolume::Find`** | `FogVolume.h` states the ban outright: the read door is read-only because its caller runs on a **0.25 s gather**, and a finder that spawned would mutate the world from inside a query, forever. |
| **`ASiegeGameMode` tick or a second `PlayAgain` loop** | Leaks fog policy into the game mode and breaks `TASK-998`'s `SC-§62` exception. `PlayAgain` stays **byte-unchanged**, and a test asserts the game mode names none of the visual symbols. |
| **A Blueprint-side "am I still up?" poll on `BP_SiegeFog`** | Impossible today — `AFogVolume` exposes **zero `UFUNCTION`s**, so a Blueprint cannot read fog state at all. Lucky, too: it would have put a second opinion about fog liveness in an asset nobody reviews. |

### ⭐ Why the timer is not a second source of truth (please read this before ruling on it)
`FogActiveUntilTimeSeconds` **remains the single source of truth.** The handle:
- stores **no deadline anybody reads back** — its rate is recomputed as
  `FogActiveUntilTimeSeconds − World->GetTimeSeconds()` at **every** arming;
- **never re-types `FogDurationSeconds`** (a test asserts **0 occurrences** of it in
  `RefreshFogVisual`) — re-typing it would be a second copy of the number and would be wrong on
  every path but the very first cast;
- is a **wake-up, not an authority**: the function it wakes **re-asks `IsFogActive()`** instead of
  acting on the fact that it fired. ⇒ **a timer that fires early re-arms; one that fires late
  destroys a hair late.** Neither can make the machine answer differently in two places, which is
  the property the ban exists to protect.
- `SetTimer` with a rate of `0` **clears** the handle instead of firing, and a cleared expiry
  handle is permanent fog — hence the `FMath::Max(..., UE_KINDA_SMALL_NUMBER)` **float-underflow
  guard**. It is not a policy; the subtraction is provably `> 0` whenever `IsFogActive()` is true.

### ⭐⭐ `TObjectPtr<AActor> FogVisualActor` IS **NOT** A STATE DUPLICATE — stated explicitly so the gate does not read it as a flag
The ban in `FogVolume.h` is on a **second answer to "is fog up?"** (`bool bFogActive` /
`bFogPrevented` / `bFogCleared`). This handle is **not an answer to anything** — it is **the
actor's own presence**, the pointer you need in order to `Destroy()` the thing you spawned.
**Nothing branches on fog state by reading it.** The only question ever asked of it is *"is there
an actor to destroy / do I need to make one?"*, and `IsFogActive()` remains the sole predicate
above it. It is **required**, not stylistic: `BP_SiegeFog`'s parent is `BP_FogArea_C`, so it is
**not** an `AFogVolume` subclass, `TActorIterator<AFogVolume>` **never sees it**, and a `Find`-style
sweep returns **nothing, silently**. ⇒ **HOLD the reference; there is no re-finding this actor.**
A test asserts `TActorIterator<AActor>` appears **0 times** in the file.

---

## 3. THE EXIT ENUMERATION — **EXACTLY THREE**, and here is how completeness is proved

**The proof is structural, not an inspection.** `FogVolume.h`'s three-state block states that
`RaiseFog` · `ApplyBrightSun` · `ResetFog` are the **only writers of either deadline in the
project** and bans a third **by name**. I did not take that on trust — I made it **executable**:

> `Siegebound.Fog.EveryExitFromFoggedReconcilesTheVisualAndThereAreExactlyThreeOfThem` asserts
> `CountOccurrencesInCode(FogVolume.cpp, "FogActiveUntilTimeSeconds =") == 3`.

⇒ **a fourth writer is a fourth exit, and it goes RED.** (Verified: the count is exactly 3 today —
`RaiseFog` stamps, `ApplyBrightSun` zeroes, `ResetFog` zeroes. `IsFogActive` and `RefreshFogVisual`
**read** it and match no `=`.)

| # | exit | handler | consequence of missing it |
|---|---|---|---|
| **(i)** | **natural expiry** — the deadline simply passes | ⛔ **no writer exists** ⇒ **I manufactured it**: the wake-up armed inside `RefreshFogVisual` | **permanent fog, forever, with a green suite** |
| **(ii)** | 🧑 **Bright Sun** — `ApplyBrightSun` zeroes the fog deadline | `RefreshFogVisual()` on the success path | his live `FogClear` card would **pay 60 gold to remove fog that is still on screen** |
| **(iii)** | **match reset** — `ResetFog` zeroes both | `RefreshFogVisual()` at the end of `ResetFog` (**not** in `PlayAgain`) | a **fog corpse survives into match 2** |

🚨 **My enumeration came back THREE, not four.** No third writer was found; nothing to report.

**Teardown is handled and it is NOT a fourth exit** — the state does not change, **the state's
owner ceases to exist**. `EndPlay` clears the wake-up and destroys the visual. **Why it is worth
the override even though a world teardown is free:** `EndPlay` also fires for `Destroyed` and
`LevelTransition`, **where the world survives** — and there, skipping it leaves a world-sized
opaque box behind with **nobody holding its reference** (`Find` cannot see it). That is permanent
fog arriving through the back door of the row whose subject is not shipping permanent fog.
⛔ I deliberately did **not** rely on `SpawnParams.Owner`: UE does **not** cascade `Destroy()` to
owned actors. Owner is set for net relevancy when M8 lands, and the comment says so.

**Refusal paths call nothing, deliberately.** `RaiseFog`'s `J-F19` refusal and `ApplyBrightSun`'s
`J-F18` refusal write **nothing** — "the stored expiry stays bit-identical" is a property their
callers depend on. A reconciler call there would be a no-op that reads like an effect.

---

## 4. THE TRANSFORM — DERIVED, and the test asserts the RELATION

`AFogVolume::FogVisualTransform(ArenaHalfExtentUU, GroundReferenceZUU)` — a **pure static**, the
shipped `BrightSunWindowSeconds` testability-seam precedent, so the geometry is assertable
headlessly.

```
HalfX  = ArenaHalfExtent.X + Margin                    26000 + 6000 = 32000
HalfY  = ArenaHalfExtent.Y + Margin                    12000 + 6000 = 18000
Bottom = GroundReferenceZ  − Margin                        0 − 6000 = −6000
Top    = GroundReferenceZ  + Ceiling                       0 + 20000 = 20000
Centre = (Bottom + Top) / 2                                          = 7000
HalfZ  = (Top − Bottom) / 2                                          = 13000
Scale  = FullExtent / CubeEdge(100)              (640, 360, 260) · loc (0,0,7000) · rot (0,0,0)
```
⇒ **bit-for-bit the transform `TASK-841` §2 read back live** (`min (−32000,−18000,−6000)`,
`max (32000,18000,20000)`), reached by derivation instead of transcription.

- **Derived:** `USiegeScatterConfig::ArenaHalfExtent` (its one owner) · `ArenaGroundReferenceZUU`
  (`J-F13`) · the `/Engine/BasicShapes/Cube` 100 uu edge.
- **Not derivable ⇒ named constants carrying their derivation, never a bare number at a call
  site:** `FogVisualHorizontalMarginUU = 6000.f` (it is `L_Arena`'s `ExponentialHeightFog_0`
  `VolumetricFogDistance` — an actor in a map we may never save, so it cannot be read from code)
  and `FogVisualCeilingAboveGroundUU = 20000.f` (a tuned choice with no owner in code).
  ⛔ I did **not** fake either as derivable. Each is transcribed **once**, in the header, beside its
  source, and a test asserts it appears exactly once there.
- ⭐ The margin is **reused** as the below-ground depth rather than a second invented number.

**The test asserts the relation, never the literal:** *the box's half-extent exceeds
`ArenaHalfExtent` on both axes by at least the margin*, *doubling the arena strictly widens the
box*, *raising the ground datum translates the box by exactly that much and changes its size not at
all*, *a zeroed arena still yields a finite, strictly positive box*. ⇒ **a hand-typed `640` goes
RED.**

⚠️ **DECLARED RESIDUAL (written in the code, not only here):** the arena extent is read from the
**`USiegeScatterConfig` CDO** — the same fallback `UWarMapWidget::ResolveArenaHalfExtent` uses. A
saved `DA_BattlefieldScatter` can override that value **for the scatter actor** without moving this
box. The live route is **closed today**: `ASiegeBattlefieldScatter::ScatterConfig` is `protected`
and its file is outside my `names:` fence. The CDO value is exactly what `TASK-841` derived and read
back, so the two lanes agree today; the comment on `SpawnFogVisual` says where to look the day they
do not. **Routing this rather than widening someone else's class.**

---

## 5. THE CLASS REFERENCE — a named static path, and a **loud** failure

```cpp
const FSoftClassPath& AFogVolume::FogVisualClassPath();   // "/Game/Blueprints/BP_SiegeFog.BP_SiegeFog_C"
```

**Chosen because it converts the hazard the row named into a red.** A hardcoded `/Game/` path is a
content dependency **no test can see break** — and `Tests/SiegeFogVisualTest.cpp` **calls this
function** and asserts the package it names is really on disk. Rename the asset ⇒ **RED**, instead
of silently invisible fog. It is also the **only** place the path is written (asserted: one literal
in the whole project; **zero** `/Game/` at the spawn site; **zero** on any code line of the header).

| rejected | why |
|---|---|
| **`EditDefaultsOnly TSoftClassPtr`** | `EditDefaultsOnly` is **archetype-only**, `AFogVolume` has **zero Blueprint children** (measured), and the per-property `Config` specifier is absent ⇒ **there is nowhere for an override to live.** It buys nothing and **adds** a `CoreRedirects` obligation to this class. |
| **`Config` (`.ini`) path** | A second live wire no test reads and no reviewer sees, on a class whose whole doctrine is one source of truth. |
| **Data-driven (`DT_Cards` column)** | The visual's lifetime is owned by the **state**, not by the card — `ResetFog` and natural expiry have **no card behind them at all**, so a card row would be the wrong owner for **two of the three exits**. |

**How a failed load announces itself:** `UE_LOG(..., Error, ...)` naming **the exact path and the
function that produced it**, then returns. ⛔ Never silently null — the cards' own VFX spawn is
null-safe and *that* is precisely why his missing spell VFX were **invisible rather than erroring**;
I did not reproduce it one layer up. Two `Error` sites (class will not load · world refuses the
spawn), asserted by the test as exactly 2.

⚖️ **The boundary of "loud" is honoured: LOUD IN THE LOG, NEVER IN THE GAMEPLAY.** `SpawnFogVisual`
is `void`, contains **zero** `return false` and **zero** references to a fog deadline (both
asserted). The fog **mechanic** keeps working with no visual; an art failure can never brick a
50-gold card, consume-and-abort, or change one clamp.

### ✅ Clause 6a — the conditional serialisation rider **did not fire**, and here is why
**I added no `EditDefaultsOnly` property.** The class reference is a plain static; the three
geometry constants are `static constexpr`; the visual handle and the timer handle are
`Transient` / non-`UPROPERTY`. ⇒ **`FogVolume.h`'s "FIVE tunables" enumeration stays true.**
I still improved it in the same diff (`SC-§91`): it now states the hazard as a **predicate**
(*every `EditDefaultsOnly` property on this class, whatever their number*) with the five **named**
as today's members, plus a dated `TASK-1068` re-check recording that the list is unchanged **by
measurement rather than by omission**. Verified: still exactly five `EditDefaultsOnly` `UPROPERTY`s.

---

## 6. CLAUSE 7 — the 🚩 OPEN paragraph is now **RULED**

`FogVolume.h`'s `FOG-§6a` question is recorded as **RULED: the two-object split IS the design**,
citing `TASK-1068` cl. 7, with the manager's three grounds preserved so it can be argued with, and
🧑 **an explicit note that Jonathan may overturn it.** ⛔ The question itself is **kept verbatim** —
a ruling that erases its own question teaches nobody. ⛔ Zero edits to `CONVENTIONS.md`.

---

## 7. THE COMMENT REPAIRS MY DIFF MADE NECESSARY (`SC-§38` cl. 3 — the diff owns the laws it invalidates)

| where | was | now |
|---|---|---|
| class doc heading | *"THIS ACTOR IS **STATE ONLY**"* + *"with **no C++ spawner yet**"* + ⚠️ *"a future reader looking for the visual should **stop looking here** — its absence is the design"* | *"THIS ACTOR RENDERS **NOTHING**"*; the class **owns the visual's lifetime, never its look**. ⛔ The old closing sentence was **actively harmful** — it sent the one reader asking the right question ("who spawns the fog?") **out of the only file that could answer**. Named, not deleted. |
| one-way-door ¶ | *"there is no handler, and there is nothing to handle"* | **narrowed**, not contradicted: true **of the state machine**, which needs none. A **visual** does. The timer is a wake-up, not an authority. |
| `CoreRedirects` ¶ | a **count** ("The FIVE tunables") | a **predicate** + the five named + a dated `TASK-1068` re-check (`SC-§91`). |
| BLUEPRINT SEAM ¶ | *"C++ lifetime control … IS THE ONLY AVAILABLE SEAM"* | ✅ that seam is now **TAKEN**, and the measurement is what chose the shape. |
| `RaiseFog` / `ApplyBrightSun` / `ResetFog` docs | — | each names its exit, and says why its **refusal path calls nothing**. |
| ctor comment | *"the timer's home **and nothing else**"* | narrowed — still no geometry of its own; it now also owns a separate actor's lifetime. |

---

## 8. 🚨 THE TEST — AND THE **NAMED MUTATIONS** THE HOST (`TASK-1070`) MUST RUN TO WITNESS RED

**New file:** `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeFogVisualTest.cpp` —
**5 automation tests**, `#if WITH_DEV_AUTOMATION_TESTS`, `IMPLEMENT_SIMPLE_AUTOMATION_TEST`,
`EditorContext | EngineFilter`, all under `Siegebound.Fog.` (the shipped namespace — no fourth
level invented). **Suite delta: 489 → 494 (+5).** ⛔ **DECLARED-NOT-EXECUTED — I could not compile
or run.**

### Which instrument answers which claim (`SC-§83` / clause 8a)
- **LANE A — genuinely EXECUTED:** tests 1 and 2 **call** `FogVisualTransform` and
  `FogVisualClassPath` and assert on the values that come back. A wrong number here is a wrong
  number in game.
- **LANE B — SOURCE-TEXT structure:** tests 3–6 count constructs on **code lines only**.
  ⛔⛔ **A source census proves a caller EXISTS; it cannot prove it is REACHED.** A call inside
  `if (false)` passes every one of them, and the file says so in its own header rather than letting
  the green imply both. What Lane B genuinely buys is that **a future refactor cannot silently drop
  an exit** — which a running test could not see either, because two of the three exits
  (`ResetFog`, natural expiry) have **no card to press**.
- ⛔ **Not covered by either:** there is not one `SpawnActor` anywhere in `Siegebound/Tests/`, so
  **nothing here runs the actual spawn**. Play `Fog` → a box appears → wait 300 s → it goes is
  **not executed by this file or by the suite**.
- ⚠️ Test 2 asserts **package EXISTENCE, not a load**, and the narrowness is deliberate: loading
  `BP_SiegeFog` pulls in its **vendor** parent `BP_FogArea`, which `TASK-841` §5.3 measured
  **dirties its own package on load** — and dirtying a vendor package inside an editor with
  autosave on is exactly how `FOG-§6`'s read-only rule gets broken **by a test**. The parent is
  covered by `TASK-1043`'s live read-back; a failed runtime load is covered by the `Error` log.
  A **negative control** (`SC-§39`) proves the existence probe answers **NO** for an absent
  package, so its YES is evidence rather than noise.

### ⛔ THE MUTATIONS — precise enough to run without asking me
Every one was **simulated against the real files** and the red confirmed by replicating
`CountOccurrencesInCode` exactly. Restore **byte-exact** and hash-verify after each
(`TASK-1041`'s protocol).

| # | mutation | expected RED |
|---|---|---|
| **A — SPAWN** | In `FogVolume.cpp`, inside `bool AFogVolume::RaiseFog()`, **delete the single line `\tRefreshFogVisual();`** (the only one in that body, just after the `FogActiveUntilTimeSeconds = …` assignment). | `Siegebound.Fog.EveryExitFromFoggedReconcilesTheVisualAndThereAreExactlyThreeOfThem` — *"RaiseFog calls RefreshFogVisual() exactly once"* **got=0 want=1** |
| **B — DESPAWN** | In `FogVolume.cpp`, inside `void AFogVolume::RefreshFogVisual()`, **delete the single line `\t\tDestroyFogVisual();`** (inside the `if (!IsFogActive())` branch). | `Siegebound.Fog.TheVisualHasOneSpawnSiteAndOneDespawnSiteAndTheStateActorStillNeverTicks` — *"the reconciler destroys in exactly one place"* **got=0 want=1** |
| **C — exit (ii)** | Delete `\tRefreshFogVisual();` from `bool AFogVolume::ApplyBrightSun(ETeamId CasterTeam)`. | test 3 — *"ApplyBrightSun calls RefreshFogVisual() exactly once"* **got=0 want=1** |
| **D — exit (iii)** | Delete `\tRefreshFogVisual();` from `void AFogVolume::ResetFog()` (it is the **last statement** of that function). | test 3 — *"ResetFog calls RefreshFogVisual() exactly once"* **got=0 want=1** |
| **E — spawn call** | Delete `\t\tSpawnFogVisual();` from `RefreshFogVisual`. | test 4 — *"the reconciler spawns in exactly one place"* **got=0 want=1** |
| **F — the margin** | In `FogVolume.h`, change `FogVisualHorizontalMarginUU = 6000.f;` to `= 0.f;`. | test 1 (**EXECUTED**) — three reds: *"the horizontal margin is strictly positive"*, *"the fog box is strictly wider than the arena on X"*, *"the box STARTS BELOW the ground datum"* |
| **G — a hardcoded extent** | In `FogVolume.cpp` `FogVisualTransform`, replace `ArenaHalfExtentUU.X + static_cast<double>(FogVisualHorizontalMarginUU)` with `32000.0` and the `.Y` line with `18000.0`. | test 1 (**EXECUTED**) — *"doubling the arena's X half-extent makes the fog box strictly wider"*; **and** test 5 — *"no hand-typed '32000' / '18000'"* |
| **H — the asset path** | In `FogVolume.cpp` `FogVisualClassPath`, change the literal to `/Game/Blueprints/BP_SiegeFog_Nope.BP_SiegeFog_Nope_C`. | test 2 (**EXECUTED**) — *"the package the shipped code names is really on disk"* |

⭐ **A and B are the two the row demands** (spawn and despawn). **A red on spawn alone proves half a
seam**, which is why B, C, D and E exist.

### ⚠️ DECLARED — THE **19th** `CountOccurrencesInCode` SITE (`TASK-1052`)
This file adds a **verbatim** copy of the house character-scan helper (plus `LoadProjectFile`,
`ExtractFunctionBody`, `SubstringBefore`). ⇒ `TASK-1052`'s census of **18 sites across 15 files**
becomes **19 across 16** when it re-derives (`SC-§91`: its count is a lower bound). ⛔ I did **not**
pre-adopt its guard — it is not authored yet, and adopting an unwritten fence is inventing one.
The site is **declared** so its author-derived sweep includes this file.

---

## 9. WHAT QA SHOULD SCRUTINISE

1. **The `TObjectPtr` ruling.** §2's last block is the argument that it is presence, not state.
   If the gate disagrees, it should say what *would* be an acceptable handle — because
   **`BP_SiegeFog` cannot be re-found**, so some handle is unavoidable.
2. **`FMath::Max(..., UE_KINDA_SMALL_NUMBER)`** — is the float-underflow guard right, or does it
   deserve a larger floor? My grounds: `SetTimer(rate ≤ 0)` **clears** the handle, and a cleared
   expiry handle is permanent fog.
3. **`RF_Transient` on the spawned visual, while `AFogVolume` itself carries no such flag.** The
   asymmetry is deliberate and commented; the visual is world-sized and **visible**, and `L_Arena`
   is never-save law.
4. **`DestroyFogVisual` inside `ResetFog`, which the game mode calls from a
   `TActorIterator<AFogVolume>` loop.** My grounds (in the code): the destroyed actor is not an
   `AFogVolume` and was never in that iteration, and UE **nulls** the level's actor slot rather
   than compacting it. **Worth a second pair of eyes.**
5. **The banned-literal census in test 5** uses substring matching, so a future code line containing
   `1260` would trip `260`. Declared in the file: that direction is a **false alarm, never a false
   pass** (`SC-§38`).
6. **The CDO-vs-DataAsset residual** in §4 — reported, not swept, and routable as its own row.

---

## 10. BOUNDS, DELTAS, FENCES

- **Suite delta:** 489 → **494** (+5), all in one new file. ⛔ **DECLARED-NOT-EXECUTED.**
- **Bounded runs (`SC-§87`):** none attempted — I ran no suite. If `TASK-1070`'s run hangs, report
  *"hung at test N: `<name>`"* and stop.
- **Unexpected reds are FINDINGS to route (`TL-§5b`)**, never numbers to overwrite.
- ⛔ **No commits. No staging. No push.** `TASK-1070` is the host.
- 🧑 `.claude/agents/qa-reviewer.md` was dirty on arrival and I **never touched it**.
