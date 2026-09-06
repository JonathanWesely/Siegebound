# TASK-1102 — DEATH-CAM-ROLL — gameplay-programmer handoff

**Marker:** `TASK-1102-DEATH-CAM-ROLL` · 2026-09-06 · gate `TASK-1103` · host `TASK-1104`
**Status:** `ready-for-qa`
**Law cited:** `SC-§79` · `SC-§83` · `SC-§87` · `SC-§90` · `SC-§94` · `SC-§36.1`

---

## 1. ⛔ THE WRITE SITE, FIRST, AS `file:line`

```
Source/GitClaudeUnrealTest/Siegebound/SiegeGameMode.cpp:862      PC->SetControlRotation(GhostRotation);
Source/GitClaudeUnrealTest/Siegebound/SiegeGameMode.cpp:833      const FRotator GhostRotation = DeadHero->GetActorRotation();
```

⚠️ **Both line numbers are PRE-FIX** — the tree I was handed, so the diagnosis can be checked against
the evidence it came from. Post-fix the same two statements are at **`:896`** and **`:867`** (the fix
adds comment lines above them, nothing else moves).

`:862` is the write. `:833` is the value it writes. **`:833` is where I fixed it** — the argument for
that is §3(a).

### ⛔ It is the ONLY candidate, and that is measured, not asserted

`SetControlRotation` appears at exactly **three** sites in the whole module —
`SiegeGameMode.cpp:514` (recall teleport), **`:862` (death→ghost)**, `:1051` (respawn) — and the other
two are both fed by `ASiegeGameMode::GetHeroStartTransform`, which is **provably yaw-only at every one
of its three returns** (`:1171` `FRotator(0, Start->GetActorRotation().Yaw, 0)` · `:1254`
`FRotator(0.0, FacingYawDeg, 0.0)` · `:1268` `FRotator::ZeroRotator`). Nothing else in `Source/**`
writes a controller's rotation, and **no site in `Siegebound` writes roll into an actor rotation at
all** (`grep` for `SetActorRotation|AddActorLocalRotation|AddActorWorldRotation|SetRelativeRotation|
SetWorldRotation` over `Siegebound/` returns 13 hits, every one of them either a decal's `-90` pitch,
a projectile's `Direction.Rotation()`, or a unit's own facing — none of them the hero, none of them a
controller).

---

## 2. THE CALL GRAPH: death → that site → why the reset does not undo it

```
hero takes lethal damage  (HeroCharacter.cpp:808)   or   FellOutOfWorld (:836)
  └─ AHeroCharacter::HandleDeath()                  HeroCharacter.cpp:864
       ├─ bDead = true; EndRecall; AbortLadderClimb; StopMovement; DisableMovement;
       │  StopWarBannerAura; SetActorHiddenInGame(true); SetActorEnableCollision(false); DisableInput
       │       ⛔ ZERO rotation writes here. ⛔ ZERO ragdoll (see §2a).
       └─ OnHeroDied.Broadcast(this)                HeroCharacter.cpp:~926
            └─ ASiegeGameMode::HandleHeroDied       SiegeGameMode.cpp:~735
                 ├─ arms the 180 s respawn timer
                 └─ ASiegeGameMode::SpawnAndPossessGhost   SiegeGameMode.cpp:829
                      ├─ :833  GhostRotation = DeadHero->GetActorRotation()   ← ⛔ THE ROLL ENTERS HERE
                      ├─ :836  SpawnActor<ASiegeGhostPawn>(…, GhostRotation, …)   (read #1)
                      ├─ :861  PC->Possess(Ghost)                                  (read #2 — see below)
                      └─ :862  PC->SetControlRotation(GhostRotation)               (read #3 — THE WRITE SITE)
                                   └─ USpringArmComponent CameraBoom (bUsePawnControlRotation = true,
                                      bInheritRoll never set ⇒ engine default TRUE)
                                        └─ the camera is rolled. The frame is on its side.
```

**⚠️ Read #2 is the one a downstream clamp would have missed.** `AController::OnPossess` performs
`ClientSetRotation(GetPawn()->GetActorRotation())` **itself**, and `PC->Possess(Ghost)` at `:861` runs
**before** the explicit write at `:862`. The ghost was spawned at `GhostRotation` at `:836`. So the
rolled value reaches the control rotation by **two independent routes**, one of them inside the engine.
Clamping `:862` alone would have left the ghost lying on its side and the engine's own copy still
rolled. That is why the fix is at `:833`.

### 2a. ⛔ THE LEADING HYPOTHESIS IN THE ROW IS **REFUTED** — and the row asked me to say which

The row named "a mesh going ragdoll and the camera copying its rotation" as the leading hypothesis and
required me to report which it was. **It is not ragdoll.** `SetSimulatePhysics` /
`SetAllBodiesBelowSimulatePhysics` / `Ragdoll` appear at **four** sites in the whole `Source/` tree and
**all four are in the read-only `Variant_*` template donors** (`CombatEnemy.cpp:244`,
`CombatCharacter.cpp:432`, `CombatDamageableBox.cpp:20`, `CombatDummy.cpp:25`). ⛔ Zero in `Siegebound`.
`AHeroCharacter::HandleDeath` hides the actor and disables collision; it never simulates anything.

**It is the *other* half of that sentence:** the camera copies the *pawn's* rotation, roll included.

### 2b. WHY A ROLL, ONCE PRESENT ON THE HERO, IS NEVER SELF-CORRECTED

`AGitClaudeUnrealTestCharacter` ctor `:27` — `RotationRate = FRotator(0.0f, 500.0f, 0.0f)`.
`UCharacterMovementComponent::PhysicsRotation()` turns each axis toward its desired value **at that
axis's rate**, so with Pitch-rate and Roll-rate both `0` it corrects **YAW ONLY**. Any roll that ever
reaches the hero capsule is held indefinitely, with nothing in any log. The death path is therefore
handed a value that nothing upstream will ever clean, and it has to clean it itself.

### 2c. ⛔ AND WHY THE RESET DOES NOT UNDO IT — the recorded fact, explained

`AHeroCharacter::ResetHero()` (`HeroCharacter.cpp:928`) restores HP, the overhead bar, visibility,
collision, movement mode, walk speed, input, melee/regen/Rally cooldowns, the War Banner aura and the
upgrade broadcast — and **touches zero rotation state**. Read the whole function: there is not one
`FRotator` in it.

The **only** place the roll is levelled today is `ASiegeGameMode::RestoreHeroAtStart:1051`
(`PC->SetControlRotation(StartRotation)`), which is a **different function, one caller away, 180 s
downstream of the death** (`GHOST-§0`).

---

## 3. ⛔ LATENT vs LIVE — MEASURED, NOT ASSUMED (`SC-§90`). READ THIS BEFORE §4.

This is the one place my finding **differs from the framing in the row**, and I am declaring it rather
than quietly shipping around it.

| | verdict | why |
|---|---|---|
| **(a) the roll on death** | 🚨 **LIVE, player-visible, up to 180 s of play** | Introduced at `:833`, rendered from the moment the ghost is possessed until the respawn timer fires. The whole ghost phase is played with the horizon vertical. **This is the real defect.** |
| **(b) `ResetHero()` not levelling roll** | ⚠️ **LATENT today** | `ResetHero()`'s only shipped caller is `RestoreHeroAtStart:1040`, and `:1051` — eleven lines later — levels the roll anyway. So on the shipped 180 s respawn the camera *does* come back level. |

**⇒ The recorded observation "it STAYS rolled after `ResetHero()`" is true and reproducible, but it was
produced by the host's own §4(c) *forced, direct* `ResetHero()` — which is precisely the caller that
gets no levelling.** It is not evidence that the shipped respawn leaves the camera rolled; it is
evidence that the **reset primitive does not do what its name promises**, which is a real latent defect
one refactor away from being live (any second caller of `ResetHero()` — a revive, a cheat, a test
harness, a future Play-Again path — inherits a sideways camera).

**Both halves shipped.** The row is explicit that (b) ships even if (a) is deferred, never the reverse;
I shipped (a) *and* (b) and I am flagging the latency of (b) for the gate rather than letting it be
discovered later. ⛔ I did **not** describe (b) as "no longer reachable" — `TASK-1103` cl. 2 forbids
that, and it would be wrong: it is reachable the moment anyone calls the primitive directly.

---

## 4. WHAT SHIPPED, PER HALF

### ⭐ NEW FILE PAIR — the two pure rules
- `Source/GitClaudeUnrealTest/Siegebound/SiegeDeathCameraStatics.h` *(new)*
- `Source/GitClaudeUnrealTest/Siegebound/SiegeDeathCameraStatics.cpp` *(new)*

`struct FSiegeDeathCameraStatics` — no `UCLASS`, no `GITCLAUDEUNREALTEST_API`, `FRotator` and nothing
else, matching the recorded `SiegeInvisibilityStatics.h:302-307` shape (`FSiegeLadderClimbStatics` /
`FSiegeStuckStatics` idiom).

```cpp
static FRotator MakeDeathViewRotation(const FRotator& DeadPawnRotation); // (a) → FRotator(0, Yaw, 0)
static FRotator LevelViewRoll(const FRotator& CurrentViewRotation);      // (b) → (Pitch, Yaw, 0)
```

**⛔ Why statics and not two inline expressions — this is the `SC-§79` decision.** An inline
`FRotator(0.f, X.Yaw, 0.f)` can only ever be asserted by a **name census**, and this project has been
bitten twice by a census that could not see a site which recomputes a value. A pure function can be
**called** by the suite with the measured `roll 89.9` and made to go genuinely red.

### (a) THE APPLICATION — `SiegeGameMode.cpp:833` (pre-fix) / `:867` (post-fix)
```cpp
const FRotator GhostRotation = FSiegeDeathCameraStatics::MakeDeathViewRotation(DeadHero->GetActorRotation());
```
plus `#include "Siegebound/SiegeDeathCameraStatics.h"` (alphabetical, after `SiegeBotController.h`).

**⛔ THE DECISION, ARGUED (clause 3a asked for it): the roll is SUPPRESSED, not kept as an effect.**
A deliberate death tilt is a few degrees about a *level* horizon; ≈90° with the horizon *vertical* is a
broken frame. Nothing in the GDD, in `GHOST-§`, or in any handoff asks for a tilt. The ghost's own
shipped intent — *"At the death location, facing the way the hero was facing … the swap is meant to
read as continuous"* (`SiegeGameMode.cpp:828-830`) — is a claim about **facing**, i.e. **yaw**. Pitch
and roll were never part of it.

**Pitch is dropped too, deliberately and not as scope creep:** this same rotation is the **ghost's spawn
rotation**, and a walking `ACharacter` spawned pitched is as wrong as one spawned rolled. It is the
identical yaw-only contract this very file already states in these words at `:1247` — *"Yaw-only on
purpose (pitch/roll 0 …)"*.

**⛔ Not a behaviour change for the healthy case:** an upright hero has pitch 0 / roll 0, so the
expression returns exactly what the shipped line returned. Asserted by test (a3).

### (b) THE RESET — `HeroCharacter.cpp`, inside `ResetHero()`, immediately after the shipped `EnableInput`
```cpp
if (AController* const OwningControllerForCamera = GetController())
{
    OwningControllerForCamera->SetControlRotation(
        FSiegeDeathCameraStatics::LevelViewRoll(OwningControllerForCamera->GetControlRotation()));
}
```
plus `#include "Siegebound/SiegeDeathCameraStatics.h"` (alphabetical, after `SiegeCombatStatics.h`).
`GameFramework/Controller.h` was already included (`:20`), so no new engine include.

**⛔ Roll only.** Pitch and yaw pass through **exactly**. A reset that also snapped the player's look
direction would be a new defect wearing this one's name — and test (b1) asserts *both* sides, so a lazy
`return FRotator::ZeroRotator;` fails just as loudly as a no-op.

**⚠️ DECLARED LIMIT (🔍 for the gate):** it resolves the controller with `GetController()` — the same
resolution the `EnableInput` line directly above already uses. **While the death ghost is possessed the
hero is UNPOSSESSED and this is a no-op.** That is correct rather than convenient: from inside the hero
there is no way to know which player controller owns a controller-less corpse, and guessing would be
worse than doing nothing. On the shipped path the ghost is retired and `PC->Possess(Hero)` has already
run before `ResetHero()` (`RestoreHeroAtStart`'s own comment: *"Repossess BEFORE ResetHero"*), so the
controller is present.

---

## 5. THE TESTS — extended the EXISTING suite, no new test file

**File:** `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeRespawnLifecycleTest.cpp`
(the fitting existing file — it already owns the death → ghost → 180 s → respawn lifecycle; ⛔ no new
test file was created). Two tests appended as sections **10** and **11**, plus a
`SiegeDeathCameraRollTestFixture` namespace carrying `LoadProjectSource` / `ExtractFunctionBody` copied
in shape from `SiegeHeroLadderClimbTest.cpp:107/143`.

| test | automation name |
|---|---|
| **10** `FSiegeDeathPathNeverTransportsRollTest` | `Siegebound.RespawnLifecycle.TheDeathPathHandsTheControllerAYawOnlyRotation` |
| **11** `FSiegeResetHeroLevelsTheCameraRollTest` | `Siegebound.RespawnLifecycle.ResetHeroReturnsControlRotationRollToZero` |

**Both are headless, zero-world, zero-PIE, zero-asset-load, zero-write** — the discipline this file
states in its own header. Each has **two halves that fail for different reasons**:

- **BEHAVIOUR half** — calls the shipped rule with the **measured** `roll 89.9` (constant
  `MeasuredDeathRollDegrees = 89.9`, deliberately the observed number and not a round 90, so nobody can
  claim the case was invented) and with a probe pitch `-17.5` / yaw `133.25` that are non-zero and
  unlike each other. Comparison is **exact** (`FMath::IsNearlyEqual(A, B, 0.0)`) because "preserved
  EXACTLY" is the claim. Failures print all six numbers.
- **CALL-SITE CENSUS half** (`SC-§36.1`) — reads the shipped source and requires the rule to actually be
  **called**. ⚠️ Every census token is a **whole statement** (trailing semicolon, or an argument list no
  comment repeats), *because both call sites' own comment blocks name the functions in prose* and a
  loose `Contains` would stay green with the code deleted. ⛔ Do not relax these to bare identifiers.

Extra tripwires worth the gate's attention:
- **(a5)** counts `SetControlRotation(` in `SiegeGameMode.cpp` and asserts **exactly 3**. A fourth
  control-rotation write is a new roll transport and must be reviewed, not absorbed. *(I reworded my own
  comment at `:851` to avoid the literal token precisely so this tripwire stays honest — noted in the
  comment itself.)*
- **(b4)** `TestFalse` that `ResetHero()`'s body contains `MakeDeathViewRotation` — i.e. the two halves
  must stay in **different files**, so the reset's levelling cannot quietly migrate into the game mode's
  death path and leave the primitive bare. This is `TASK-1103` cl. 2 asked by the suite, every run.
- Each census runs a **positive control first** (`PC->Possess(Ghost);` / the shipped `EnableInput`
  statement) so a broken extraction fails loudly instead of passing vacuously.

---

## 6. 🚨 THE NAMED MUTATIONS (`SC-§83`) — AND: **NO WITNESSED RED**

**NO WITNESSED RED.** I could not execute them. Clause (6) of my row and my dispatch both forbid
compiling (the editor is open and holds the DLL), and the suite cannot be run against this diff without
a compile — running it against the currently-loaded DLL would exercise the **old** code and report a
number that means nothing (`SC-§94`: a success return is not evidence). ⛔ I did not run the suite, I
did not open the editor, and I touched no MCP. **`TASK-1104` owns the compile and its spec item (1)
already requires it to run the mutation and witness the red.**

Three mutations, each a **one-line** edit, each with the exact expected result. Restore **byte-exact
from a scratch copy** and verify against a `sha256` taken **before** mutating (`SC-§68`) — ⛔ never
`git restore`, the diff is uncommitted.

| # | file | the exact edit | expected |
|---|---|---|---|
| **M1** *(half b — the primary)* | `Siegebound/SiegeDeathCameraStatics.cpp`, body of `LevelViewRoll` | replace `return FRotator(CurrentViewRotation.Pitch, CurrentViewRotation.Yaw, 0.f);` with `return CurrentViewRotation;` | `Siegebound.RespawnLifecycle.ResetHeroReturnsControlRotationRollToZero` goes **RED** — 2 of the 3 `CheckRotator` rows fail, each printing `roll 89.9000` where `0.0000` was expected. **307 / 308.** |
| **M2** *(half a)* | `Siegebound/SiegeDeathCameraStatics.cpp`, body of `MakeDeathViewRotation` | replace `return FRotator(0.f, DeadPawnRotation.Yaw, 0.f);` with `return DeadPawnRotation;` | `Siegebound.RespawnLifecycle.TheDeathPathHandsTheControllerAYawOnlyRotation` goes **RED** — the (a1) and (a3) rows fail. **307 / 308.** |
| **M3** *(the zero-caller trap — `SC-§36.1`)* | `Siegebound/HeroCharacter.cpp`, inside `ResetHero()` | delete the whole `if (AController* const OwningControllerForCamera = GetController()) { … }` block (leave the comment) | test **11** goes **RED on its census half only** — the two `(b3)` assertions fail while every behaviour row stays green. ⛔ This is the mutation that proves the suite can tell a landed *file* from a landed *fix*. **307 / 308.** |

The opposite-direction check is already permanent, no mutation needed: a lazy
`return FRotator::ZeroRotator;` in `LevelViewRoll` fails `(b1)`/`(b2)` on **pitch and yaw**, not on roll.

---

## 7. SUITE BEFORE / AFTER (`SC-§87`) — declared honestly

- **before:** **306 / 306** — the shipped baseline as recorded on the board. ⚠️ **I did not run it.** I
  am reporting the inherited number, not a measurement of my own; per `SC-§94` treat it as such.
- **after:** **expected 308 / 308** — 306 + the two new tests. ⚠️ **Not measured.** No compile, no run.
- ⛔ **`TASK-1104` owes the real `N / M` in both states**, plus the three mutation runs above.

---

## 8. 🔧 FOR `TASK-1104` (the host) — compile notes

1. **Two NEW source files** (`SiegeDeathCameraStatics.h/.cpp`). ⛔ There is **no new `UCLASS`**, so no
   UHT-generated header and no `.generated.h` — but a new `.cpp` still needs a **full Build.bat pass**
   with the **editor CLOSED**; Live Coding / `Ctrl+Alt+F11` is not a substitute for adding a
   translation unit.
2. **⛔ Build.bat returns exit 0 on a FAILED build** (Live Coding mutex). Parse the log for `Result:` —
   ⛔ never trust `$LASTEXITCODE`.
3. **⚠️ If Smart App Control is ENFORCED on this machine** (`VerifiedAndReputablePolicyState = 1`) every
   UE `Build.bat` fails in **~2 s** with **`0x800711C7`**, because it blocks UBT's unsigned
   `ModuleRules.dll`. **That is Jonathan's Windows setting, ⛔ NOT a code error** — the fix is
   Windows Security → Smart App Control → Off, and it is **his** call. ⛔ Do not route that back to me
   as a QA loop.
4. Files to compile-check: the two new files, `SiegeGameMode.cpp`, `HeroCharacter.cpp`,
   `Tests/SiegeRespawnLifecycleTest.cpp`.

---

## 9. 🔍 FLAGGED DECISIONS FOR THE GATE (`TASK-1103`)

1. **⛔ `MakeDeathViewRotation` drops PITCH as well as roll.** Argued in §4(a) — the same rotation is the
   ghost's spawn rotation, and the in-repo yaw-only precedent is `SiegeGameMode.cpp:1247`. If the gate
   wants pitch preserved, that is a one-word change in the static and one line in test (a1).
2. **⛔ I fixed at `:833` (the value), not at `:862` (the write).** §2 argues the value is read three
   times and two of the reads are *ahead* of the explicit write, one of them **inside the engine**
   (`AController::OnPossess`). Rule against me if you think `:862` is the only legitimate site — but
   note that a `:862`-only fix leaves the ghost spawned on its side.
3. **⚠️ (b) is LATENT today, and I say so in §3 rather than claiming it is live.** I did **not** claim it
   was unreachable (cl. 2 forbids that) and I shipped it anyway. If the gate reads the row as asserting
   (b) is live on the shipped respawn path, my §3 table is the contrary measurement and the gate should
   adjudicate which reading governs.
4. **⚠️ (b) is a no-op while the ghost is possessed** (§4(b), declared limit). Accepted deliberately.
5. **⛔ I could not measure what put ~90° of roll onto the hero capsule in the first place.** No site in
   `Source/**` writes it (§1), and I am fenced out of the editor, MCP and `Content/`, so I cannot probe
   the live pawn. The upstream is therefore **outside `Source/`** — a Blueprint, the physics asset, an
   animation, or an engine path. ⭐ **The fix does not depend on knowing which:** sanitising at `:833`
   makes the death path *structurally incapable* of transporting roll, whatever the upstream is. 🙋 If
   Jonathan or the gate wants the upstream identified, that is **a different row with editor access**
   (probe `Hero->GetActorRotation().Roll` at the moment of death) — and it is worth boarding, because a
   hero capsule that is genuinely rolled 90° is also a mis-oriented body for traces and for the mesh.
6. **⚠️ Declared edge in `LevelViewRoll`:** an input with roll `±180` is an upside-down view whose
   pitch/yaw are the mirrored pair; zeroing its roll levels the horizon but leaves the mirrored
   pitch/yaw. The measured defect is ≈90 and the contract is "roll returns to 0", which holds for every
   input. Flagged rather than special-cased — a normalisation branch would be untestable guesswork about
   an input never observed.
7. **NO WITNESSED RED** (§6) and **no measured suite numbers** (§7). Both handed to `TASK-1104`.

---

## 10. FILES TOUCHED — the complete list

| file | change |
|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/SiegeDeathCameraStatics.h` | **NEW** — the two pure rules + the whole diagnosis |
| `Source/GitClaudeUnrealTest/Siegebound/SiegeDeathCameraStatics.cpp` | **NEW** — two bodies, nothing else |
| `Source/GitClaudeUnrealTest/Siegebound/SiegeGameMode.cpp` | half (a): 1 statement changed at `:833`, 1 include, comment block |
| `Source/GitClaudeUnrealTest/Siegebound/HeroCharacter.cpp` | half (b): 5-line `if` added to `ResetHero()`, 1 include, comment block |
| `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeRespawnLifecycleTest.cpp` | 4 includes + fixture namespace + tests 10 and 11 |
| `.claude/pipeline/handoffs/TASK-1102-programmer.md` | this file |
| `.claude/pipeline/TASKBOARD.md` | **`Edit` tool, my row's single `status:` line only** |

**⛔ ZERO `Content/`. ⛔ ZERO editor. ⛔ ZERO MCP. ⛔ ZERO Blender. ⛔ ZERO git** — no `add`, no `commit`,
no `push`, no `restore`. `L_Arena` never opened. `CONVENTIONS.md` and `Tools/ArtPipeline/**` untouched.

⚠️ **Foreign dirt I saw and left alone** (`git diff --stat` at the start of my lane): `TASKBOARD.md`
(+195, the boarding of this wave) and `.claude/pipeline/CONVENTIONS.md` (+23) were **already modified
before I started** and are **not mine**; the untracked `Content/RawAssets/MainCharacter.fbx`,
`Content/RawAssets/Textures/MainCharacter/`, the three `playtest-evidence/2026-09-06/MainCharacter_*.png`
and `handoffs/TASK-1091-artist.md` belong to the art lane. Reported per the hosts'-census rule; ⛔ I
touched none of them.
