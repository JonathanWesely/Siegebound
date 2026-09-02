# TASK-758 — the ghost's initialisation ordering — gameplay-programmer handoff

**Status:** ready-for-qa · **Date:** 2026-09-01
**Files touched (SOLE, and nothing else):**
- `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Source\GitClaudeUnrealTest\Siegebound\SiegeGhostPawn.h`
- `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Source\GitClaudeUnrealTest\Siegebound\SiegeGhostPawn.cpp`
- `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Source\GitClaudeUnrealTest\Siegebound\Tests\SiegeGhostPawnTest.cpp`
- `.claude/pipeline/TASKBOARD.md` (status row only — see "board" below)

**Suite delta: +2 tests added.** (A count of tests added by this task — ⛔ deliberately **not** an on-disk tree total; other batches are in flight.)

---

## 1. The defect, re-measured at the source before touching anything

`ASiegeGameMode::SpawnAndPossessGhost` (`SiegeGameMode.cpp`):

| line | call |
|---|---|
| `:823` | `World->SpawnActor<ASiegeGhostPawn>(...)` — **`BeginPlay` fires inside this** |
| `:842` | `Ghost->InitializeGhost(DeadHero->GetTeamId())` |
| `:848` | `PC->Possess(Ghost)` |

⇒ `GhostTeam` is still its declaration default (`ETeamId::Blue`) for the whole of `BeginPlay`. Any team-dependent line written there is **silently wrong for Red 100 % of the time** — it compiles, it reviews as correct, and nothing fails. TASK-756's row (c) is confirmed exactly as written.

## 2. The fix I chose, and why it is not one of the other two

**Chosen: a single gated seat, driven from both ends.**

- New private `ASiegeGhostPawn::ApplyGhostTeamAppearance()` — documented in the header as **the one place team-dependent initialisation may live**.
- It is **gated on `bGhostTeamAssigned`** (a new plain bool set only by `InitializeGhost`) ⇒ with no team assigned it does **nothing at all**, rather than doing the wrong thing quietly. ⛔ The gate is *not* `GhostTeam == Blue`: `ETeamId` has no unset value (`TeamId.h` declares `Blue`/`Red` only), and that ambiguity **is** the defect.
- It is **called from BOTH `BeginPlay` (tail) AND `InitializeGhost`** ⇒ whichever runs **last** drives it. Correct under the shipped ordering *and* under a future deferred spawn (`SpawnActorDeferred` → `InitializeGhost` → `FinishSpawning` → `BeginPlay`).
- It is deliberately **re-runnable — there is no "already applied" early-out**, and the header says so in bold. The re-run is what makes the second ordering correct: the mesh whose material slots it stamps does not exist until `BeginPlay` has assigned it. A sensible-looking idempotence guard would freeze the first, mesh-less run.

**Why not "move the work into `InitializeGhost` only":** that fixes the shipped ordering and breaks the deferred one — the mesh does not exist yet at that point, so a tint would stamp zero slots.

**Why not "make the team a spawn parameter":** it is a legitimate shape, but (a) it requires editing `ASiegeGameMode` to spawn deferred, which is TASK-750's file and outside this task's fence — per the dispatch I would have had to stop and say so; and (b) it only fixes **the one caller that was edited** — a second caller written later with a plain `SpawnActor` walks straight back into the trap. The chosen shape is correct for every caller, present and future. **Nothing outside my fence was needed.**

**Why not a comment:** the dispatch is right that it is the weakest option; the code now makes the wrong thing structurally inert rather than merely discouraged.

### What actually moved
The `GhostMaterial` slot-stamping moved out of `BeginPlay` into `ApplyGhostTeamAppearance()`, because that material is **the seat of any future team tint** — so it must be stamped when the team is known. **⛔ No behaviour change in the shipped path:** both instants are within the same `SpawnAndPossessGhost` call and both precede `Possess`, and `GhostMaterial` is unset today anyway (TASK-756's `BP_SiegeGhostPawn` is still hard-blocked and `GhostPawnClassAsset` ships unset, so the raw C++ class is the shipped ghost).

### The silent-omission guard (`SC-§36`)
A step that refuses to run until told is correct — and a step that is *never* told runs never, quietly. `NotifyControllerChanged` now warns **once, loudly**, if the pawn is possessed by a real `APlayerController` while `bGhostTeamAssigned` is false. Silent in the shipped path (`:842` precedes `:848`), loud the day a caller forgets. ⛔ It does not return, branch or otherwise touch the `IMC_Hero` re-add below it.

## 3. TASK-749's four load-bearing properties — verified after the change

| # | property | verdict |
|---|---|---|
| 1 | **⛔ Does NOT implement `ITeamAgent`** | ✅ **HOLDS.** Class declaration untouched (`: public ACharacter`). No interface added, no `GetTeamId()` added. The new accessor is deliberately named `HasAppliedTeamAppearance` — the same "distinct name so nobody completes the pattern" discipline `GetGhostTeam` uses. All eight acquisition enumerations, including `FSiegeCombatStatics::ApplyRadialDamage`, still cannot see it. `FSiegeGhostPawnNotATeamAgentTest` unmodified and still passes both lanes. |
| 2 | **⛔ Capsule collision OBJECT TYPE unchanged** | ✅ **HOLDS.** ⛔ **I did not touch one line of the constructor.** `SetCollisionObjectType(ECC_Pawn)` is byte-identical, so the ghost stays out of `AProjectile::FindTerrainHit`'s WorldStatic/WorldDynamic object trace — no projectile shield. `FSiegeGhostPawnCollisionTest` row (c) unmodified. |
| 3 | **⭐ The ghost re-adds `IMC_Hero` itself, incl. the positional layout resolve** | ✅ **HOLDS.** The guard chain (`GhostMappingContext` → `APlayerController` → `ULocalPlayer` → `UEnhancedInputLocalPlayerSubsystem`), the `KBD-§5`/`§6` resolve **in the innermost scope**, the priority constant and the `AddMappingContext` call are **all byte-identical**. My addition sits *above* the existing `if (!GhostMappingContext)` block, logs only, and never returns. |
| 4 | **⭐ The published API keeps its signatures** | ✅ **HOLDS.** `void InitializeGhost(ETeamId)` · `ETeamId GetGhostTeam() const` · `void RetireGhost()` — unchanged, in that order. `RetireGhost()` was **not edited at all**: still idempotent via `bRetired`, still does not un-possess, still owns no timer. `HasAppliedTeamAppearance()` is added in a **separately labelled block** marked *"⛔ NOT part of the three-call lifecycle API"*, and TASK-750 neither calls it nor needs to. |

## 4. The test that fails against the old ordering (+2)

**⛔ The trap I deliberately avoided** (the one that cost two QA loops this week): asserting `GetGhostTeam() == Red` after `InitializeGhost(Red)` **would have passed against the defective code** — the *field* was always assigned correctly; what was wrong was **when the work that reads it happened.** So the tests measure **the team the team-dependent step actually observed**, via `HasAppliedTeamAppearance(OutTeam)`.

**`FSiegeGhostPawnTeamKnownBeforeTeamWorkTest`** — `Siegebound.Ghost.TeamDependentInitialisationObservesTheRealTeamNotTheDefault`
- SELF-CHECK: the declaration default **is Blue** — asserted, because the whole test's discriminating power depends on Red ≠ default.
- (a) a freshly constructed ghost has **not** run the step (it is gated on the team being known).
- (b) `InitializeGhost()` **drives** the step. ⛔ **Red against the old code**, where `InitializeGhost` assigned a field and drove nothing.
- (c) ⭐⭐ **the step observed `Red`.** ⛔ **This is the row that goes red against the old ordering**, where the step ran in `BeginPlay` and observed `Blue` on every Red ghost.
- (d) …and the observed team is **not** the declaration default (the same claim phrased against the value the bug produced).
- (e) the record agrees with `GetGhostTeam()` — it is a record of the real field, not a parallel value that could drift.
- SELF-CHECK: a **Blue** instance records Blue and the two instances **differ** ⇒ the record tracks the argument and cannot be a hard-coded constant.

**`FSiegeGhostPawnTeamStepIsRerunnableTest`** — `Siegebound.Ghost.TeamDependentInitialisationRerunsAndHasNoAlreadyAppliedEarlyOut`
- `InitializeGhost(Blue)` then `InitializeGhost(Red)`; the observed team must follow to Red. **This goes red the day someone adds an "already applied" early-out** — which reads like harmless idempotence and would break the deferred-spawn ordering. It also puts an assertion behind the API's published *"safe to call more than once"* promise.

**Mechanism:** transient `NewObject<ASiegeGhostPawn>` instances (GC-rooted with `TStrongObjectPtr`, `RF_Transient`, transient package). ⛔ **No world, no `SpawnActor`, no PIE, no asset load, no CDO mutation** — `GhostMaterial`/`GhostMesh` ship unset in C++ so no soft pointer resolves and no log line is emitted. The file's "CDOs and reflection only" mechanism note has been **amended** so it stays true.

## 5. ⚠️ Things QA should scrutinise (I am flagging them rather than hoping)

1. **`NewObject` on an `AActor` subclass is new to `Siegebound/Tests/`.** The directory's standing claim is "no `UWorld::CreateWorld` and no `SpawnActor`" — both still hold, and I amended the file's mechanism note rather than leave the claim stale. But an **ordering cannot be observed on a CDO** (a CDO never runs an initialisation sequence), so a transient instance is the minimum instrument. If QA rules `NewObject<AActor>` out of bounds for this directory, say so — I have no cheaper instrument that can actually fail against the old code, and a CDO-only assertion would be exactly the "passes under both orderings" test the dispatch forbids.
2. **Stated limit of the new tests, on purpose:** they cannot *run* `BeginPlay` (an actor outside a world never begins play). ⇒ they prove the step is **driven by and observes** the team hand-off; they do **not** prove `BeginPlay` stays free of team-dependent lines. That second claim rests on the design (one gated seat, documented at both call sites) and on **QA's diff review**. It is written into the test file so a green bar is not over-read.
3. **The material moved.** Confirm you are happy that `GhostMaterial` is now stamped in `ApplyGhostTeamAppearance()` rather than `BeginPlay`. My argument: it is the tint seat, both instants precede `Possess`, and it is unset today so the shipped path is byte-identical in effect.
4. **New members are un-reflected** (`bGhostTeamAssigned`, `bTeamAppearanceApplied`, `AppliedAppearanceTeam`, plus the two functions), matching `bRetired`. None contains a banned token, so `FSiegeGhostPawnNoCombatSurfaceTest`'s scan is unaffected; its two name self-checks (`GhostTeam`, `GhostMappingContext`) still find their targets.
5. **`SC-§36` compliance of the tint seat comment.** `ApplyGhostTeamAppearance()` marks where a team tint *would* be decided. It explicitly states it **binds no task, schedules nothing and awaits nothing**, and no code references a team material. ⚠️ **If a team tint is actually wanted, it must be BOARDED as its own task** — TASK-756 flagged it as an undecided ruling for TASK-755 and authored two **unwired** proposal instances (`MI_HeroSpirit_Blue`/`_Red`, zero referencers). **This handoff does not claim that work is scheduled.**

## 6. Board

⚠️ **TASK-758 had no row on `TASKBOARD.md`** — it was dispatched directly. I added a compact row after TASK-756 in the *MARKS + RECALL + GHOST* section, **explicitly labelled as programmer-authored for status tracking, not manager law**, carrying `status: ready-for-qa` and the +2 suite delta. **The manager should correct or absorb it.**

## 7. Not done, by fence

⛔ No compile, no editor, no MCP, no Git. ⛔ No `SiegeGameMode`/`SiegePlayerController` (TASK-750), no `HeroCharacter` (TASK-748), no materials (TASK-756).
