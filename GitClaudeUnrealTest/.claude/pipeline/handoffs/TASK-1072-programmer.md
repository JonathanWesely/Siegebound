# TASK-1072 — the fog-visual SCALE CLOBBER, fixed; and the instrument that hid it, corrected

**Agent:** gameplay-programmer · **Date:** 2026-09-05 · **Marker:** `TASK-1072-FOGSCALE-CLOBBER`
**Status:** `ready-for-qa` · **Gate:** `TASK-1073` · **Host:** `TASK-1074` · **Outcome (rung 4):** `TASK-1075`

---

## 0. PRECONDITION CHECK — the row's named finding condition, discharged FIRST

`HEAD` = `c6bb376344c8c2d6c3871017dda4aaa14591f74f` ✅ (matches the row).

Before my first write, `git status --porcelain`:

```
 M .claude/agents/qa-reviewer.md
?? .claude/pipeline/handoffs/TASK-1071-artist-diagnosis.md
```

⇒ ✅ **`FogVolume.cpp` and `FogVolume.h` read CLEAN.** The row's one named FINDING condition
("either reads DIRTY before your first write ⇒ STOP AND REPORT") **did not fire**. `TASK-1070`
committed that lane; I absorbed nobody's uncommitted work.

⚠️ The board and `CONVENTIONS.md` went `M` *during* my run (the manager boarding this lane
concurrently) — named as expected-dirty on the row, not a finding, and **not mine**.

---

## 1. WRITES — the authoritative list (the host's pathspec derives from this)

| Path | What |
|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/FogVolume.cpp` | the fix + the readback + the new predicate's body |
| `Source/GitClaudeUnrealTest/Siegebound/FogVolume.h` | the predicate's declaration + one tolerance constant |
| `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeFogVisualTest.cpp` | **tests 6 and 7 (NEW)** + **one existing count corrected** |
| `.claude/pipeline/handoffs/TASK-1072-programmer.md` | this file |
| `.claude/pipeline/TASKBOARD.md` | **`TASK-1072`'s `status:` line ONLY** |

⛔ **No other `Source/` path.** ⛔ No new test file — I took the row's PREFERRED option and extended
`SiegeFogVisualTest.cpp`, the shipped home of this seam, so the host's staging set is unchanged.

**Fences held:** ⛔ `Content/FogArea/**` byte-unchanged (never opened) · ⛔ `BP_SiegeFog` never
opened, re-parented or re-tuned · ⛔ `L_Arena` **never saved** (I never opened the editor at all —
see §7) · ⛔ `CONVENTIONS.md` untouched by me · ⛔ the three other-lane `Tests/` files untouched ·
⛔ nothing staged, ⛔ no commit, ⛔ no push, ⛔ no `git checkout`/`restore`/`stash`/`reset`/`clean`.

---

## 2. THE FIX — the form the row required, and I did not deviate

`AFogVolume::SpawnFogVisual()`, after the spawn and after the existing null check:

```cpp
const FVector RequestedScale3D = SpawnTransform.GetScale3D();
Spawned->SetActorScale3D(RequestedScale3D);
```

⚠️ The expression is **spent once into a named local** and then referred to by name — that is not
cosmetic, it is what makes §3's "the log never re-spells the request" assertion mechanically
checkable. Semantically identical to the row's literal `Spawned->SetActorScale3D(SpawnTransform.GetScale3D());`.

**I did NOT use `SpawnParams.TransformScaleMethod`** (the row makes it a blocker on sight, correctly).
The test file now **reds on that reach by name**: `TransformScaleMethod` must be **0** in
`FogVolume.cpp`, so a future "simplification" into that flag cannot land quietly.

**Why the explicit set and NOT `SpawnActorDeferred` + `FinishSpawning`** — answering the row's
sentence directly, because it is the whole argument: `Actor.h:3117` defaults `FinishSpawning`'s
`bIsDefaultTransform` to `false`, so the deferred path dodges the clobber **in the editor** — but
the engine's own comment at `SCS_Node.cpp:144-146` says that flag is **`false` in a cooked build**
too, i.e. the two paths **differ between uncooked and cooked**. A fix that relies on which spawn
overload we called is a fix whose correctness is a function of the build configuration, and the
failure direction is the worst one available: **the bug returns only in the packaged game, where
nobody is watching and no automation runs.** `SetActorScale3D` runs unconditionally, after
construction, in both. ⇒ **I took the explicit set. No deviation to argue.**

### 2a. A hazard the diagnosis did not cover, which I checked before trusting the fix

`SetActorScale3D` on a component whose **Mobility is Static** could plausibly have been refused at
runtime — which would have made this fix a no-op and produced 🧑 a **fourth** "no fog". I did not
assume; I read it, on this machine:

- `Actor.cpp:5078` — `AActor::SetActorScale3D` → `RootComponent->SetWorldScale3D(NewScale3D)`
- `SceneComponent.cpp:1982` — `SetWorldScale3D` → `SetRelativeScale3D` (no parent ⇒ pass-through)
- `SceneComponent.cpp:1865` — `SetRelativeScale3D` writes the field and calls
  `UpdateComponentToWorld()`. **There is no mobility gate on this path** — the
  *"Mobility has to be Movable"* refusal lives in `MoveComponentImpl` (location/rotation), which
  scale never enters.

⇒ ✅ **The correction applies regardless of the vendor component's mobility.** Recorded so QA does
not have to re-derive it, and so a future reader knows it was checked rather than hoped.

### 2b. The mechanism, independently re-verified (the artist's one un-observed link)

The artist declared honestly that they could not *watch* the substitution happen. **I re-read the
engine source on this machine and confirm their quotation is exact**, line for line:

- `Actor.cpp:4358-4360` — non-deferred spawn: `if (!bDeferConstruction) { FinishSpawning(UserSpawnTransform, true); }` ⇒ `bIsDefaultTransform = **true**`
- `SCS_Node.cpp:131-140` — the `ESpawnActorScaleMethod` switch
- `SCS_Node.cpp:142-147` — **`if (bIsDefaultTransform) { WorldTransform.SetScale3D(NewSceneComp->GetRelativeScale3D()); }`, AFTER the switch**

⇒ the substitution is **unconditional on the scale method**, exactly as diagnosed. This is still
source-reading, not observation — see §5 for what remains unobserved and who owns it.

---

## 3. THE READ-BACK CORRECTION — the half I consider the more important one

**The old line was:**

```cpp
SpawnTransform.GetLocation().Z,
SpawnTransform.GetScale3D().X, SpawnTransform.GetScale3D().Y, SpawnTransform.GetScale3D().Z,
```

— the value it **asked for**, on a line that reported what it **got**. Every sentence it wrote was
true. The one number that mattered was never taken. That is why it read clean, why the gate passed,
and why 🧑 Jonathan was told the actor had spawned "at the right transform" while his screen did not
change. **It is gone, not joined by a second line** (`SpawnTransform.GetScale3D()` now appears
**once** in that whole function — the derivation — and `SpawnTransform.GetLocation()` **zero** times).

**The new line prints ACHIEVED values first, unmistakably labelled**, with the request kept in
parentheses so a human can see a disagreement on one line:

```
Fog VISUAL spawned: '<name>' — ACHIEVED Z=…, ACHIEVED scale (…, …, …), read back from the actor
with GetActorLocation/GetActorScale3D (requested (…, …, …), derived from ArenaHalfExtent + a … uu margin; …)
```

**And a disagreement is LOUD** — a third `Error` site in this function:

```cpp
if (!FogVisualScaleMatches(RequestedScale3D, AchievedScale3D))
{
    UE_LOG(LogGitClaudeUnrealTest, Error, /* names the cause, the consequence, and what to check */);
}
```

⚖️ **`Error`, never a refusal.** A wrong scale is an ART failure, and the boundary the two existing
loaders already hold binds this branch too: the fog MECHANIC keeps working, the 50-gold card is not
refused, no clamp changes, the visual is still HELD and destroyed on schedule. The spawn path still
has exactly its **three original early exits** — pinned at 3, so a future edit cannot teach the
mismatch branch to abort and leave an opaque box unheld and undestroyable.

### 3a. `SC-§94`'s own falsifiable test, applied to my new line — the row asks for this in writing

> *"If I deleted the spawn, would this line still print the same text?"*

**NO — and it fails in two independent ways, which is the answer I want:**

1. **Delete the spawn** ⇒ `Spawned` is null ⇒ the existing `if (!Spawned)` early-returns and
   **the line does not print at all.** It cannot report on an actor that does not exist.
2. **Keep the spawn, delete the correction** (`Spawned->SetActorScale3D(...)`) ⇒ the line still
   prints, but its numbers **change**, from `(640, 360, 260)` to `(20, 20, 5)`, because they are
   read off the actor. **The old line printed byte-identical text in both worlds.**

⇒ the new line's text is a **function of the world**, not of the request. That is the property the
old one lacked, and it is the whole deliverable.

### 3b. `GetActorBounds()` — CONSIDERED, and deliberately NOT added (my call, argued)

The row invites it. I declined, for one reason: **bounds carry no information about this defect that
the scale does not already carry, and they carry noise the scale does not.** `GetActorBounds` is
`scale × the mesh's own bounds`, computed render-side and dependent on component registration order
— a *derived, weaker* second representation of the exact fact the readback already measures
directly. Adding it would put two numbers in the log that must agree, on a class whose entire
doctrine is one source of truth, and the failure it would uniquely catch (scale applied, bounds not
refreshed) is not the measured defect and has no evidence behind it. **If QA wants it, it is one
line and I will add it on a FAIL** — I would rather be told to add it than smuggle in a second
instrument nobody asked to maintain.

---

## 4. THE TEST — and the SCS-root question answered PLAINLY, because it is the one that sinks this row

### 4a. ⛔ THE DECLARED INABILITY — read this before reading any green

**My tests do NOT spawn an actor. I could not construct an SCS-rooted subject in an automation
test, and I am saying so plainly rather than shipping a native-root test that would be green with
the fix and green without it.**

The reason is specific and is not laziness: the only SCS-rooted subject available is `BP_SiegeFog`
itself (or its vendor parent), and **loading it is forbidden from inside a test.** `TASK-841` §5.3
measured that loading `BP_SiegeFog` pulls in its vendor parent `BP_FogArea`, **which dirties its own
package on load** — and dirtying a vendor package inside an editor that has autosave on is exactly
how `FOG-§6`'s read-only vendor rule gets broken *by a test*. The shipped test 2 already declines to
load it for precisely this reason and says so. Authoring a **fixture Blueprint** with an SCS root
would mean creating a new `/Game/` asset from a C++ row — new content, a new asset to name, convey
and maintain, and outside this row's WRITES list.

⇒ **This is a SCOPE STATEMENT (`SC-§79`), not a failure**, and it routes the end-to-end question to
`TASK-1075`, where it already lives.

### 4b. What the two new tests DO prove — stated exactly

**TEST 6 — `Siegebound.Fog.TheScaleReadbackRejectsTheEngineSubstitutionThatMadeTheFogInvisibleThreeTimes`
(LANE A, ⭐ GENUINELY EXECUTED).**
It calls the shipped `AFogVolume::FogVisualScaleMatches` — a pure static, no world, no actor, no
asset load, the `BrightSunWindowSeconds` testability-seam precedent — and hands it **the measured
vendor substitute `(20, 20, 5)`** against the scale **derived** from
`FogVisualTransform(ShippedArenaHalfExtent(), 0.f)`.

- ⭐⭐⭐ **THE ONE:** `TestFalse` — the detector must answer **NO** to the engine's substitution.
- ⭐ **positive control** — it answers YES to the derived scale (a judgement, not a blanket refusal).
- **premise asserted, not assumed** — the two really differ on every axis, so the rejection is not vacuous.
- tolerance is real and **tight** in both directions; per-axis (Y alone, Z alone) mismatches caught; NaN is a mismatch, with a self-check that the fixture's NaN really is one.

⛔ **What test 6 does NOT prove:** that the substitution is actually *corrected in a running game*.
It proves the **detector** is sound. The correction itself is pinned structurally by test 7 and
observed only by 🧑 his eye (`TASK-1075`). **I will not let one stand in for the other** — that
conflation is the entire subject of this row, and the file header now says so.

**TEST 7 — `Siegebound.Fog.TheSpawnSiteForcesTheScaleAndLogsWhatItGotRatherThanWhatItAsked`
(LANE B, source-text).** Pins, on `SpawnFogVisual`'s real body: the correction line (×1); the
`TransformScaleMethod` no-op reach (×0, whole file); the two readbacks; **`SpawnTransform.GetScale3D()`
exactly once and `SpawnTransform.GetLocation()` zero times** — i.e. the old echo cannot come back;
the comparison and its negated branch; the three early exits; **both orderings** (correction before
measurement — else it screams on every cast forever; spawn before correction); and that the detector
stays a reachable public static rather than an inline expression no test could run.

### 4c. `(c)` RELATION not literal — held

Test 1's overhang relations are **untouched** and still bind. Test 6 derives its intended scale from
the shipped function; **no `640` is typed anywhere.** The one literal I introduced is the **vendor
donor value `(20, 20, 5)`, in the TEST file only**, carrying its provenance
(`BP_FogArea.BP_FogArea_C:Mesh_GEN_VARIABLE`, read live under `TASK-1071` §2). It is not our
geometry and is not derivable from our code; if the vendor pack is ever updated it goes stale in the
**safe** direction (the detector is then asserted to reject some *other* wrong scale, which it still
must — the claim degrades, it does not invert).

### 4d. ⚠️ ONE EXISTING ASSERTION MOVED, DECLARED IN THE OPEN (`TL-§5b`)

`Siegebound.Fog.TheVisualHasOneSpawnSiteAndOneDespawnSiteAndTheStateActorStillNeverTicks`:
`LogGitClaudeUnrealTest, Error,` in `SpawnFogVisual` **2 → 3**. This is **a third error site this
diff deliberately adds**, not a probe drifting off its subject. ⭐ Note it *could* have been hidden
from that count by moving the mismatch into a private helper — **which is exactly what a probe that
counts error sites exists to stop.** It stays inline and the number is corrected in daylight, with
the reason written beside it.

### 4e. `SC-§91` census — DECLARED

⛔ **No new `CountOccurrencesInCode` site.** I extended an existing file that already hosts the
helper, so `TASK-1052`'s census stays at **19 sites / 16 files** — my row does **not** move it.
⚠️ I did add one **verbatim copy of the bit-pattern `MakeQuietNaN()` helper** (from
`SiegeBrightSunTest.cpp` / `SiegeCastBarTest.cpp:129`) to this file's fixture. Different helper,
declared for completeness.

⚠️ **And a fixture hazard I nearly shipped, recorded because it is a real trap:** my first draft used
the bare `NAN` macro. Two problems — it is constant-foldable (the house forbids it for that reason),
and this build runs with `ENABLE_NAN_DIAGNOSTIC == 1`, where **arithmetic** on a NaN vector raises an
engine error and fails a test *on its own fixture*. The shipped form **constructs and reads only**,
never arithmetic — matching `SiegeLadderClimbTest.cpp`'s `NaNLocation`, which is proven green in
this very suite — and `FogVisualScaleMatches` short-circuits on `ContainsNaN()` **before**
`FVector::Equals` would subtract anything.

---

## 5. ⛔ THE MUTATIONS — AND I DECLARE: **NO WITNESSED RED**

⛔⛔ **I did not compile and I did not run the suite (`SC-§83` — my role cannot). I have NOT
witnessed a single red. Nothing below is a red I saw; every one is a red I predict.** The duty to
run these, witness the reds, restore byte-exact and only then commit **transfers by name to
`TASK-1074`.**

### ⭐ MUTATION A — THE ROW'S NAMED ONE: revert the `SetActorScale3D` line

**File:** `Source/GitClaudeUnrealTest/Siegebound/FogVolume.cpp`, in `AFogVolume::SpawnFogVisual()`.
**Delete this exact line** (one line, leading tab preserved):

```cpp
	Spawned->SetActorScale3D(RequestedScale3D);
```

**Predicted:** `Siegebound.Fog.TheSpawnSiteForcesTheScaleAndLogsWhatItGotRatherThanWhatItAsked`
goes **RED**, on three assertions at once — the correction count (`got=0 want=1`); the
correction-before-measurement ordering (`got=0 want=1`); and the ordering probe's own stale-marker
`AddError` ("Ordering marker … is gone"). Suite `495 / 1`.

### ⭐ MUTATION B — the INSTRUMENT: put the echo back

**Same file, same function**, in the final `UE_LOG(…, Log, …)` argument list. **Replace:**

```cpp
		AchievedLocation.Z,
```

**with:**

```cpp
		SpawnTransform.GetLocation().Z,
```

**Predicted:** the same test goes **RED** on two assertions — `SpawnTransform.GetLocation()`
(`got=1 want=0`) and the achieved-Z-reaches-the-log `TestTrue`. Suite `495 / 1`.
⭐ This is the *original defect itself*, re-inserted in its smallest possible form. If it does not
red, the readback correction is not actually pinned and this row has failed its main purpose.

### ⭐ MUTATION C — the DETECTOR: make it answer YES to everything

**Same file**, in `AFogVolume::FogVisualScaleMatches`. **Replace the final line:**

```cpp
	return AchievedScale3D.Equals(RequestedScale3D, static_cast<double>(FogVisualScaleTolerance));
```

**with:**

```cpp
	return true;
```

**Predicted:** `Siegebound.Fog.TheScaleReadbackRejectsTheEngineSubstitutionThatMadeTheFogInvisibleThreeTimes`
goes **RED** on four `TestFalse` rows (the vendor-scale rejection, the outside-tolerance row, and
both per-axis rows). The NaN rows still pass — the early `ContainsNaN` return is untouched — which
is itself informative. Suite `495 / 1`.

⚠️ **Restore from scratch copies and `sha256`-verify a byte-exact match — ⛔ never `git restore`;
this diff is uncommitted and a restore would delete the row.** (`TASK-1070`'s own recorded practice.)

---

## 6. SUITE DELTA — DECLARED, ⛔ NOT EXECUTED

Baseline (last **executed**, `TASK-1070`): **`494 / 0`**.
This row adds **2** tests ⇒ **`496`**. `IMPLEMENT_SIMPLE_AUTOMATION_TEST` count in
`SiegeFogVisualTest.cpp`: **5 → 7**, verified by count.

⛔ **`494 → 496 (+2)`, DECLARED-NOT-EXECUTED.** I ran no suite and no build; no bound was set or
consumed, because nothing was run to bound.

**What I did instead, and its exact weight:** I re-implemented `CountOccurrencesInCode`'s
comment-skipping semantics and `ExtractFunctionBody` / `SubstringBefore` faithfully in a throwaway
script and ran **every** assertion in both new tests, plus **every existing assertion in every test
file that scans `FogVolume.{h,cpp}`** (`SiegeFogVisualTest` · `SiegeBrightSunTest` ·
`SiegeFogVolumeTest` · `SiegeFogRefusalTest`), against the real bytes on disk. All green, including
the eight banned geometry literals, the six trace needles, the seven banned state flags, and the
three writer-census counts. Braces and parens balance on all three edited files.
⚠️ **That is a text simulation, not a compile and not a test run.** It cannot see a type error, a
missing include, or a link failure, and **it is not evidence of a green suite.** Two of my own
assertions were wrong when first written (`return;` count, and an `AchievedScale3D` needle that
matched the seam's own parameter name) and the simulation is what caught them — which is the honest
measure of what it is worth: it catches my arithmetic, not the compiler's judgement.

---

## 7. 🧑 THE HONEST SCOPE OF EVERY GREEN THIS ROW CAN PRODUCE — in my own words

**A green suite here proves that the code asks the engine to apply the scale, that the log now
measures what came back, and that the detector rejects the exact value the engine substituted. It
does NOT prove Jonathan sees fog.**

Concretely, the three things a full green still leaves open:

1. **Nothing here spawns an actor** (§4a) — so nothing executes the SCS path where the clobber
   lives. The correction is pinned structurally, not observed.
2. **Nothing here renders a pixel.** Even a correctly-scaled box is a *claim* about geometry, not
   about what a volumetric froxel grid does with it at his camera.
3. **A residual I cannot close from code:** the vendor `BP_FogArea` implements `ReceiveTick`. My
   readback is taken *immediately* after the correction — so if the vendor's own Tick were to write
   the scale back on a later frame, **this row's log would still read clean.** I judge it unlikely
   and I have no evidence for it; I flag it because it is precisely the class of thing that would
   produce a fourth "no fog" with a green suite, and the first thing to check if `TASK-1075`
   measures no change. (The mismatch `Error` names it explicitly in its "what to check" text.)

⇒ **RUNG 4 — a PIXEL MEASUREMENT against a ZERO-CONTROL at his real vantage, target ≈ `+73 %` mean
luma vs control — is `TASK-1075`, and it is the row that closes this lane. The commit does not.**
🧑 His eye remains the only instrument for whether the fog *looks* right, and a fully green suite
says nothing whatsoever about that.

---

## 8. WHAT QA SHOULD SCRUTINISE (`TASK-1073`)

1. **§3a's answer** — is my new log line genuinely a function of the world? Try to construct a
   deletion under which it prints identical text. I could not.
2. **§4a** — is the declared inability honest, or did I duck a fixture Blueprint I could have built?
   I believe the vendor-dirtying argument is decisive; disagree loudly if not.
3. **§4d** — the `2 → 3` count move. Confirm it is a real new error site and not a probe I loosened.
4. **The tolerance, `0.01f`.** It is the one number I chose rather than derived. It is ~1.6e-5
   relative against the smallest shipped axis and the defect it catches is a factor of 18–52, so I
   believe it cannot hide anything — but it is the first place a future "flaky test" edit would go,
   which is why both directions are pinned by test 6.
5. **§4e's NaN hazard** — if `TASK-1074` sees test 6 fail on the *fixture* (an engine NaN
   diagnostic) rather than on its subject, the remedy is to **delete the two NaN rows and their
   self-check**, ⛔ never to change the shipped `ContainsNaN` guard. Routing that decision here in
   advance so nobody has to guess under a red.
6. **§7.3** — the vendor-Tick residual. If you think it should be measured rather than declared,
   say so; it is a `TASK-1075` observation, not a code change.

---

## 9. FENCES — final state

✅ `HEAD` `c6bb376`, unchanged · ✅ nothing staged, no commit, no push, no destructive git
✅ `Content/FogArea/**` never opened · ✅ `BP_SiegeFog` never opened · ✅ `CONVENTIONS.md` untouched
✅ **`L_Arena` never saved — I never opened the editor and never contacted MCP.** The artist left it
UP (PID 29128) with the map dirty in memory; I neither saved nor closed it, and no save prompt
reached me. The pinned hash `1f78419d…5622` is untouched by anything I did.
✅ `.claude/agents/qa-reviewer.md` — named-and-left, unstaged, not read, not edited.
✅ Other lanes' three `Tests/` files — not opened for writing (read-only, to prove my diff cannot
red their assertions).
