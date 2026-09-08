# TASK-1106 — [ROLL-UPSTREAM] diagnosis (gameplay-programmer)

**Marker:** `TASK-1106-ROLL-UPSTREAM` · **DIAGNOSE-ONLY, READ-ONLY** · 2026-09-07
**Law:** `SC-§39` · `SC-§87` (incl. the *"four facts, no mechanism"* conduct clause) · `SC-§90` · `SC-§93` · `SC-§94` cl. B · `SC-§95` · `FIELD-§1`
**HEAD at time of work:** `e1ba2c4` · **editor** PID 24532, MCP `127.0.0.1:8000` answering
**Files written by this row:** THIS FILE, and one `- status:` line on `TASKBOARD.md`. Nothing else.

---

## 0. THE ANSWER IN ONE PARAGRAPH

**Clause (4): the hero capsule is NOT rolled. `GetActorRotation().Roll` read `−0.0000` at every
sample across a real, driven death — alive, at the instant of death, and through the ghost phase.**
**Clause (5) WARN-1 at `SiegeGameMode.cpp:961`: the transport is REAL and I measured it firing
(a corpse I deliberately rolled to `+89.9` put exactly `+89.9` into the control rotation and the
camera through `PC->Possess(Hero)`), but it is NOT REACHABLE in practice because its input never
occurs — no rolled corpse was produced by any death I could drive.**
**Clause (3)'s two `UE_LOG` probes are NOT needed and I did not write them** — MCP + the editor's
Python remote-execution channel read the values live, which is a strictly better instrument than a
log line (`SC-§94` cl. B: it reads the state, it does not echo a request).
**Clause (7): I did NOT find the mechanism. This is the licensed *"four facts, no mechanism"*
outcome** — but the search space is now much smaller, and **one of the three facts the row handed me
is WRONG**, which is §5 below and is the most useful thing on this page.

---

## 1. THE POSITIVE CONTROLS (clause 1 · `SC-§39` · `SC-§95`)

Three separate instruments were used; each carries its own control. **A silent instrument is not a
zero reading**, and this project has already paid for that once (the climb probe, 2026-09-02).

### 1a. The log channel — the trap the row leads with

**Before the run**, `LogsToolset.GetVerbosity("LogGitClaudeUnrealTest")` returned **`"Log"`** — i.e.
`Verbose` was **OFF**, exactly the state that would have produced a false pass. I set it and
re-read it:

```
SetVerbosity  category=LogGitClaudeUnrealTest verbosity=Verbose   ->  null
GetVerbosity  category=LogGitClaudeUnrealTest                     ->  "Verbose"
```

**The positive-control line — the one that proves the channel actually speaks.** I triggered a
deterministic emitter (`ASiegePlayerController::HandleMatchEnd` called a second time, the
`UE_LOG(..., Verbose, ...)` at `SiegePlayerController.cpp:2039`) and read it back:

```
[2026.09.08-01.38.22:612][352]LogGitClaudeUnrealTest: Verbose: ASiegePlayerController
'SiegePlayerController_0': HandleMatchEnd ignored — match already ended.
```

⚠️ **AND THE TRAP WAS LIVE, NOT THEORETICAL.** Across **two full PIE sessions** — a running match
with bots, a driven hero death, a ghost phase, a match end, and ~5 minutes of wall clock — the
`LogGitClaudeUnrealTest` `Verbose` channel emitted **exactly ONE line in total, and it was mine.**
Every other Verbose site in the module (53 of them, censused below) is behind a conditional none of
this exercised. A run that had emitted nothing would have looked **identical** to a working channel
reporting a clean zero. `SC-§95`, precisely.

### 1b. The reader — proving `_snap` can see a non-zero roll

The reader that printed `R −0.0000` for every natural sample had to be shown capable of printing
something else. I injected a known roll onto the **PIE-transient** corpse and read it back through
the identical code path:

```
CONTROL: injecting roll 89.9 onto the transient PIE corpse BP_HeroCharacter_C_0
SNAP[F] hero.actorRot         = (P +0.0000  Y +180.0000  R +89.9000)
        hero.capsule.worldRot = (P +0.0000  Y +180.0000  R +89.9000)
```

⛔ **DECLARED, so it is not discovered later:** this is a runtime mutation of a **transient PIE actor**
that is discarded when the session stops. It is not a `Source/**` edit, not a `Content/**` write, and
`L_Arena` was never opened for edit and never saved. It is the `SC-§39` control the row's own law
requires, and without it the string of zeros would have been unpublishable.

### 1c. The asset census instrument — controlled before its zeros were believed

My first attempt (`strings` over the `.uasset`) returned **zero hits AND zero control hits** — a blind
instrument that would have published a false all-clear. I replaced it with an ASCII-run extractor and
gated every zero on a control: six names that **must** be present in any Blueprint-shaped asset
(`SimpleConstructionScript`, `BlueprintGeneratedClass`, `EdGraph`, `K2Node_Event`, `SCS_Node`,
`/Script/Engine`). All six were found in both hero and ghost Blueprints, and the sweep in §4 reports
its control-passed denominator (`3890 / 4389`) beside its numerator, never a bare zero.

---

## 2. CLAUSE (4) — THE MEASURED `.Roll` SAMPLES, AND WHEN EACH WAS TAKEN

**Method.** Editor PID 24532, `L_Arena`, PIE started and stopped over MCP (`StartPIE` / `StopPIE`;
`IsPIERunning` checked **false** before I started and again after I finished — I started two sessions
and stopped two, and never touched a session I did not start). Values read live through the editor's
Python remote-execution channel (`Config/DefaultEngine.ini:7 bRemoteExecution=True`), the lane
`TASK-1094` §6 recorded. `t` is `UGameplayStatics::GetTimeSeconds` on the PIE world.

### Session 1 — a real, driven death (`ApplyDamage` → `TakeDamage` → `HandleDeath`)

| # | when | `hero.GetActorRotation()` | `hero` capsule world rot | `PC->GetControlRotation()` | possessed pawn |
|---|------|---------------------------|--------------------------|----------------------------|----------------|
| **A** | alive, idle, `t=44.102` | `P +0.0000  Y +180.0000  R −0.0000` | `R −0.0000` | `R −0.0000` | `BP_HeroCharacter_C_0` |
| **C** | **at death**, same call stack as the damage | `R −0.0000` | `R −0.0000` | `R −0.0000` | `BP_SiegeGhostPawn_C_0` |
| **D** | ghost phase, `t=58.870` (+0.6 s) | `R −0.0000` | `R −0.0000` | `R −0.0000` | `BP_SiegeGhostPawn_C_0` |
| **E** | ghost phase, `t=62.004` (+3.6 s) | `R −0.0000` | `R −0.0000` | `R −0.0000` | `BP_SiegeGhostPawn_C_0` |

The ghost pawn's own actor rotation read `R −0.0000` at C, D and E as well, and
`PlayerCameraManager.GetCameraRotation()` read `R −0.0000` at every one of them.

⛔ **A sample I will not quote because I did not retain it:** a `B-pre-kill` snapshot was taken in the
same block as C and its lines scrolled out of my captured buffer. It is not in the table. `A` covers
the alive case.

**⇒ CLAUSE (4) ANSWER: the capsule reads ~0 at every sample. The `89.9` `TASK-1094` §5.6 recorded
lived in the CONTROL ROTATION only. `eeb29c4` closed the transport that could carry it, and
`ResetHero()` levels it.**

### Session 1 continued — the deliberate injection (see §3)

| # | when | `hero` actor roll | control roll | camera roll |
|---|------|-------------------|--------------|-------------|
| **F** | `t=141.091`, roll injected onto the dead, unpossessed corpse | **`+89.9000`** | `−0.0000` | `−0.0000` |
| **G** | `t=142.733` (+1.6 s of ticks, still dead) | **`+89.9000` — HELD** | `−0.0000` | `−0.0000` |
| **H** | `t=144.415`, after match end ⇒ `RetireGhostFor(:565)` ⇒ `PC->Possess(Hero)` `(:961)` | `−0.0000` | **`+89.9000`** | **`+89.9000`** |

### Session 2 — the same injection on a LIVING, possessed hero (this is §5)

| # | when | `hero` actor roll |
|---|------|-------------------|
| **I** | `t=39.841`, roll injected | **`+89.9000`** |
| **J** | `t=39.960` — **Δ 0.119 s of world time, a handful of ticks** | **`−0.0000`** — gone |
| **K** | `t=41.045` (+1.2 s) | `−0.0000` |
| **L** | `t=45.130` (+5.3 s) | `−0.0000` |

---

## 3. CLAUSE (5) — THE WARN-1 RESIDUAL AT `SiegeGameMode.cpp:961`

**Bound by symbol, not by line** (`SC-§93` cl. 4 — I opened the file). At `e1ba2c4`/`e12907c`:
`ASiegeGameMode::RetireGhostFor` opens at **`:915`**; `PC->Possess(Hero)` is at **`:961`**; the
match-end caller is at **`:565`**; the respawn-timer caller at **`:1015`**; Play Again at **`:1350`**.
⭐ **And there is a FOURTH `PC->Possess(Hero)` the WARN did not name — `:1070`, inside
`RestoreHeroAtStart`'s defensive path.** It is the same engine-side copy and inherits the same verdict.

### The verdict, in the two halves `SC-§90` demands

**(a) THE MECHANISM IS REAL AND I WATCHED IT FIRE. MEASURED, NOT REASONED.**
Rows F→H above are one causal experiment. With the corpse standing at `+89.9` roll and the control
rotation at `−0.0000`, I ended the match — which runs `:565` → `RetireGhostFor` → `:961`
`PC->Possess(Hero)` → `AController::OnPossess` → `ClientSetRotation(GetPawn()->GetActorRotation())`.
One frame later the control rotation read **`+89.9000`** and the camera read **`+89.9000`**, and the
control rotation had held `−0.0000` for the entire 3.3 s before that. The WARN is **not** hypothetical
and the word does not appear in this verdict.

**(b) ITS INPUT DOES NOT OCCUR ⇒ `NOT REACHABLE`.**
`:961` copies whatever roll the corpse carries. The corpse carried `−0.0000` at every sample of a real
death (§2, rows A/C/D/E). The `+89.9` in row F was **put there by me**; no death produced it.
⇒ **`:961` is NOT REACHABLE as a defect on the measured death path**, with the reachability decided by
the corpse's roll and not by an argument about the code.

**⇒ `NOT REACHABLE` — with the caveat in §6 about which deaths I was able to drive.**

---

## 4. CLAUSE (6) — THE CENSUS OUTSIDE `Source/**`

Read live off the **possessed PIE instance** wherever possible (`SC-§94`: read back the result, not
the setting). Verdict column answers only *"can this write roll onto the hero capsule?"*

| # | candidate | measured value | can it write roll? |
|---|-----------|----------------|--------------------|
| 1 | `/Game/Blueprints/BP_HeroCharacter` — the hero pawn IS a Blueprint (`BP_HeroCharacter_C`) | graph name-table carries **no** rotation-write node name; only `RelativeRotation` (a component default) and `Rotator` (a struct type) | **NO** |
| 2 | `bUseControllerRotationPitch / Yaw / Roll` on the live pawn | `False / False / False` | **NO** — `APawn::FaceRotation` early-outs entirely when all three are false |
| 3 | `CharacterMovement.bOrientRotationToMovement` | `True` | **NO** — `ComputeOrientToMovementRotation` returns `Acceleration.Rotation()`, whose roll is always 0 |
| 4 | `CharacterMovement.RotationRate` | `(P 0, Y 500, R 0)` — confirms `GitClaudeUnrealTestCharacter.cpp:27` | **NO**, and see §5 — this value does the OPPOSITE of what the row was told |
| 5 | `CharacterMovement.bUseControllerDesiredRotation` | `False` | **NO** |
| 6 | `CharacterMovement.bIgnoreBaseRotation` (movement-base rotation inheritance) | `False` ⇒ the path is **live** on the hero | **NO** — ruled out by reading `CharacterMovementComponent.cpp:2580-2612` and `UpdateBasedRotation` at `:2718-2742`: the capsule branch sets `TargetRotator.Roll = 0.f` explicitly, and the controller branch **saves `ControllerRoll` and writes it back**, so roll is preserved, never injected |
| 7 | **Physics asset / ragdoll** — `PA_Mannequin` on `SK_MainCharacter`, `physics_asset_override = None` | `mesh.is_simulating_physics() = False`, `body_instance.simulate_physics = False`; `SetSimulatePhysics(true)` appears **0×** on any Siegebound path (only `Variant_Combat/*` and `CombatDummy`, all unused template variants) | **NO** — measured, not assumed |
| 8 | **AnimBP / root motion** — `ABP_Unarmed_C` | `root_motion_mode = ROOT_MOTION_FROM_MONTAGES_ONLY`; the hero plays exactly one montage (`AttackMontage`, `HeroCharacter.cpp:533`) and `HandleDeath()` plays none | **NO** on the death path |
| 9 | **Spring arm** `CameraBoom` | `bInheritRoll = True`, `bInheritPitch/Yaw = True`, `bUsePawnControlRotation = True` | **NO — but it is the SYMPTOM path.** The arm takes the control rotation wholesale, so control-rotation roll *is* camera roll. Measured at row H: control `+89.9` ⇒ camera `+89.9` |
| 10 | Camera `FollowCamera` | `bUsePawnControlRotation = False` (inherits the arm) | **NO** |
| 11 | Hero mesh component | world rot `(P 0, Y 90, R 0)` vs actor yaw 180 ⇒ relative `(0, −90, 0)`, `absolute_rotation = False` | **NO** |
| 12 | `/Game/Blueprints/BP_SiegeGhostPawn` | name table contains **no** string matching `rotat` at all; C++ side `SiegeGhostPawn.cpp:99-106` sets all three `bUseControllerRotation*` false and `RotationRate (0,500,0)` | **NO** |
| 13 | **Every Blueprint in the project** — full `Content/` sweep | **4389** `.uasset` read, **3890** passed the Blueprint-shape control, **6** carry a rotation-write node name: `CR_Mannequin_Body`, `CR_Mannequin_Procedural` (control rigs — they drive **bones**, not the capsule root), and four vendor magic-pack demo assets (`BP_Fire_Magic_Character`, `BP_Ice_Magic_Character`, `BP_IceBreak_Multiple_Burst`, `BP_IceBreak_Multiple_Loop`) | **NO** — none is on the hero, the ghost, or any death path |
| 14 | `Source/**` actor-rotation writes (my own re-derivation, not a relay) | 14 sites total; the only non-variant ones are `Projectile.cpp:294`, `SummonedUnit.cpp:462/4821`, `SiegePlayerController.cpp:3631/4265` (decal reticles), `CommanderNpc.cpp:129`, `AncientGround.cpp:52`, `CaptureZone.cpp:41` | **NO** — not one targets `AHeroCharacter`. Corroborates the row's inherited fact (2)(a) by an independent needle |

### ⛔ REPORTED AS **UNMEASURED**, never as unlikely

- **`SK_MainCharacter`'s post-process AnimBP / control rig.** `CR_Mannequin_Procedural` carries a
  `SetRotation` node. I reasoned it cannot reach the capsule (a control rig writes bone transforms,
  and `GetActorRotation()` reads the capsule root) but I did **not** measure whether a post-process
  graph is even bound on this mesh. Cheap to settle; not settled here.
- **The name-table census is a NAME instrument.** It sees a node or property *name* present in an
  asset. It cannot see a rotation written through a renamed local, a variable-driven
  `SetActorTransform`, or a native component's own tick. Its 3890-of-4389 control makes its zeros
  publishable; it does not make them exhaustive.
- **Camera modifiers / `APlayerCameraManager::ProcessViewRotation`.** Not censused. This is a live
  hook that can add roll to the view rotation before `SetControlRotation`, and it sits **outside**
  every census `TASK-1103` and I have run. See the row spec in §7 — this is where I would look next.
- **Deaths I could not drive.** See §6.

---

## 5. ⭐⭐ THE FINDING: INHERITED FACT **(2)(c)** IS **WRONG**, AND IT IS WRONG IN THE DIRECTION THAT MATTERS

The row handed me this as a given:

> **(c)** `RotationRate = (0, 500, 0)` means `PhysicsRotation` corrects **yaw only** ⇒ a roll that
> **LANDS** is **HELD FOREVER**. There is no self-healing path.

**That is false for a living hero, and the truth is closer to its exact opposite.** Two independent
instruments agree.

**Instrument 1 — live measurement (rows I→J).** An `89.9°` roll injected onto the living, possessed,
`MOVE_Walking` hero was **gone within 0.119 s of world time** — a handful of ticks — with no code of
ours running in between.

**Instrument 2 — the engine source, read at
`Engine/Source/Runtime/Engine/Private/Components/CharacterMovementComponent.cpp:6698-6710`:**

```cpp
// If we'd be prevented from becoming vertical, override the non-yaw rotation rates to allow the character to snap upright
if (CharacterMovementCVars::bPreventNonVerticalOrientationBlock && bWantsToBeVertical)
{
    if (FMath::IsNearlyZero(DeltaRot.Pitch)) { DeltaRot.Pitch = 360.0; }
    if (FMath::IsNearlyZero(DeltaRot.Roll))  { DeltaRot.Roll  = 360.0; }
}
```

`DeltaRot.Roll` comes from `RotationRate.Roll` — which is **`0`**. A zero roll rate is `IsNearlyZero`,
so the engine **overrides it to 360**, and `FMath::FixedTurn` with a rate `>= 360` returns the desired
value outright ⇒ **instant snap upright, in one tick.** The CVar
`p.PreventNonVerticalOrientationBlock` **defaults to `1`** (`:337`) and its own help text says it in
these words: *"allows a character that's supposed to remain vertical to snap to a vertical orientation
even if RotationRate settings would block it."*

**⇒ `RotationRate.Roll = 0` does not FREEZE the roll. It ARMS the snap-upright override.** The
reasoning that produced fact (c) — "rate 0 ⇒ `FixedTurn` holds current" — is the pre-5.x behaviour and
was overtaken by this CVar.

### Why this is the most useful line on the page

It explains rows F/G vs I/J exactly, and it **narrows the hunt by an order of magnitude**:

- `PhysicsRotation` bails at its own top if the character has **no controller**
  (`:6636-6640`), and `ShouldRemainVertical()` is **false** for `MOVE_None`.
- `AHeroCharacter::HandleDeath()` calls `GetCharacterMovement()->DisableMovement()` (⇒ `MOVE_None`)
  **and** the controller is handed to the ghost. **Both** guards therefore fail on a corpse — which is
  why row G held `+89.9` for 1.6 s while row J lost it in 0.119 s.

⇒ ⭐ **A ROLL CANNOT SURVIVE ON A LIVING HERO. It is erased inside one tick.** Any candidate mechanism
that rolls the capsule *before* `DisableMovement()` is therefore **self-refuting** — the roll would be
gone long before anyone could see it. **The only window in which a capsule roll can persist is
after `DisableMovement()` and before repossession**, and nothing in that window writes rotation.
Combined with §2's measured zeros, that is a strong argument that **the capsule was never the source
at all** and the `89.9` was born in the control rotation.

⚠️ **This also means clause (3)'s recipe would have been a false-negative trap.** Logging
`Hero->GetActorRotation().Roll` at `HandleDeath()` and at `SpawnAndPossessGhost` would have printed
`0.0` twice — truthfully — and told the next agent nothing, because the living-hero snap makes those
two sites blind to any roll that arrived earlier. Good that the row put clause (4) first.

---

## 6. CLAUSE (7) — WHAT I DID **NOT** ESTABLISH, STATED PLAINLY

**I did not find the mechanism.** Per `SC-§87`'s conduct clause I am not going to name one, because a
named mechanism gets acted on before it is checked. The honest fact list:

1. The hero capsule reads roll `≈ 0` at every point of a driven death (§2).
2. Nothing in `Source/**`, in `BP_HeroCharacter`, in `BP_SiegeGhostPawn`, in the physics asset, in the
   AnimBP's root-motion configuration, or in any of the 3890 control-passed Blueprints in `Content/`
   writes roll onto that capsule (§4).
3. The capsule **cannot** hold a roll while the hero is alive — it is snapped upright within one tick
   by an engine override that `RotationRate.Roll = 0` **arms** rather than disables (§5).
4. `SiegeGameMode.cpp:961` **will** copy a corpse roll into the control rotation and the camera — I
   made it do so — but no death I could drive ever produced a rolled corpse (§3).

⛔ **THE DECLARED LIMIT ON ALL OF THIS, AND IT IS THE ONE THING THAT COULD OVERTURN §2.**
`TASK-1094` §5.6 saw its `89.9` after walking the hero into the **RED army at 500 uu/s** and being
**killed twice by melee**, at a pre-`eeb29c4` build. **My death was a scripted `ApplyDamage` on a hero
standing still at his own spawn on flat ground.** The `HandleDeath` path is identical and carries no
impulse, no ragdoll and no montage, so I do not expect the damage source to matter — **but I did not
measure that, and "I do not expect" is not a measurement.** Four things differed and each is
`UNMEASURED`: the hero was **moving**, he was on **mid-field terrain rather than the spawn pad**, the
killer was a **live melee unit**, and the build was **pre-transport-fix**. A follow-on row that drives
a melee death mid-field would close this properly.

---

## 7. CLAUSE (8) — INSTRUMENTATION, AND WHY I STOPPED WHERE I DID

**The two `UE_LOG` probes of clause (3) are NOT necessary, so I did not write them, and I am not
asking for a lane to write them.** MCP + the Python remote-execution channel read
`GetActorRotation().Roll` and `GetControlRotation().Roll` **live, on the possessed instance, across
the whole death timeline**, which is what the probes were a proxy for — and §5 shows those two
specific insertion points would have printed a truthful, useless `0.0`.

**Zero edits made:** `git status --short` at the end of this row shows **no** `Source/**` and **no**
`Content/**` entries (the dirt present is `TASKBOARD.md`, `CONVENTIONS.md`, and the untracked handoffs
of the concurrent `TASK-1105` / `TASK-1111` lanes — none of it mine). No compile, no test, no commit,
no push. `L_Arena` never opened for edit, never saved. Both PIE sessions were started by me and
stopped by me; `IsPIERunning` reads `false` now.

**Baselines recorded so a follow-on row can prove nothing moved under it** (`SC-§68`):

```
55ce13f38ab3b472c802e5ca58b602c92866bf15ef494474692c28fa14a61bfd  Source/GitClaudeUnrealTest/Siegebound/HeroCharacter.cpp
3f60416e73e37e4539b66f36326b7f178b2621da9f15d81fc8e2a478cf433d09  Source/GitClaudeUnrealTest/Siegebound/SiegeGameMode.cpp
```

### 📋 ROW SPEC THE MANAGER CAN BOARD WITHOUT RE-DERIVING ANYTHING

**Row A — `[ROLL-UPSTREAM-2]` the control-rotation hunt (gameplay-programmer, diagnose-only,
read-only, no gate needed by the `TASK-1105`/`TASK-1106` precedent).**
- Premise, already measured here: **the capsule is not the source** — it physically cannot hold a roll
  while alive (§5). ⇒ **look where the roll can persist: the CONTROL ROTATION**, which has no
  restoring force of any kind.
- Census target, explicitly `UNMEASURED` by this row: `APlayerCameraManager::ProcessViewRotation` and
  any `UCameraModifier` registered on `ASiegePlayerController`'s camera manager; plus
  `APlayerController::UpdateRotation` overrides. **These sit outside every `SetControlRotation` census
  run so far, which is exactly why four censuses have come back clean.**
- Second target: `SK_MainCharacter`'s post-process AnimBP binding (is `CR_Mannequin_Procedural`
  reachable at all?).
- **Clause 1 of TASK-1106 carries over verbatim: set `Log LogGitClaudeUnrealTest Verbose` FIRST and
  quote the positive-control line.** It was measured **OFF** at the start of this row.
- Reproduction it must add, because this row could not: **a melee death mid-field after a walk**, the
  `TASK-1094` §5.6 conditions. The remote-exec per-tick driver idiom
  (`unreal.register_slate_post_tick_callback`) recorded in `TASK-1094` §6 is what makes that drivable.

**Row B — `[LAW]` correct the record on `RotationRate` (manager, doc-only).**
`CONVENTIONS.md` and the `TASK-1102`/`1103`/`1104`/`1106` board rows all assert *"`RotationRate =
(0, 500, 0)` ⇒ a roll that lands is held forever."* §5 falsifies it with a live measurement **and** the
engine source. It is load-bearing — it is the stated reason the upstream "matters even though the
symptom is hidden" — and leaving it standing will send the next reader down the same wrong corridor.
⛔ **Not absorbed silently here; this row does not write `CONVENTIONS.md`.**

**Row C — optional, `[HARDEN]` `SiegeGameMode.cpp:961` + `:1070`.**
`PC->Possess(Hero)` is an **engine-side** copy of the corpse's rotation into the control rotation, and
§3 measured it firing. It is currently harmless because the corpse is never rolled — but it is the
one remaining uninstrumented transport, and `:1070` was never named by WARN-1 at all. Cheap belt;
**not** urgent, and **not** to be smuggled into a diagnosis row.

---

## 8. WHAT THIS DIAGNOSIS DOES **NOT** MEAN (`SC-§94` cl. B)

It does **not** mean the death camera is level in Jonathan's hands — I drove one scripted death at a
spawn pad, not his match. It does **not** mean the upstream is closed: **nothing on this page explains
where `TASK-1094`'s `89.9` came from**, and §6 lists four measured-different conditions between his
death and mine. It does **not** clear `ProcessViewRotation` or the post-process rig — both are
`UNMEASURED`, and `UNMEASURED` is not `clean`. What I can defend is exactly this: *across a driven
death on `e1ba2c4`, the hero capsule's roll was zero at every sample; no asset or source site in this
project writes roll to it; a living hero's capsule cannot retain roll for even one tick; and the
`:961` residual is a real transport with no input.*
