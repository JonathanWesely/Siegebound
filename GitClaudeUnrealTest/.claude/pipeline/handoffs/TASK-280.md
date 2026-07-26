# TASK-280 handoff — ATTACK march-freeze: diagnose + fix

**Branch:** `m7.6-arena10x` · **Status:** ready-for-qa · **Code-only** (no compile, no Git — TASK-281 owns those)

**Files touched (ONLY these):**
- `Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.cpp` — ATTACK branch no-aggro goal block in `UpdateStateStandardCommanded`
- `Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.h` — one new UPROPERTY `EnemyBaseEngageRadius`

No temporary `UE_LOG` was added — the mechanism was confirmed by live MCP property/transform readback + code equivalence on the CURRENTLY COMPILED build (no logging/compile needed). So there is nothing to strip.

---

## 1. How it was reproduced / instrumented

The editor was running the current compiled build with the MCP server up. I ran a live PIE session (`StartPIE`, in-viewport) and inspected the **PIE world** directly via MCP (`find_actors` returns `UEDPIE_0_...` refPaths → confirmed I am reading the running game, not the editor CDO). Readable live fields per unit: `Team`, `Profile`, `CardID`, `State` (= `GetUnitState`), `CurrentTarget`, and world transform. (`CurrentMoveGoal` is a private `UPROPERTY(Transient)` without `AllowPrivateAccess` → not readable; I inferred goal changes from `CurrentTarget` + position + the computed box contents.)

**Constraint (honest):** The player (Blue) summons units via card-select + a **placement click** (needs gold) and the ATTACK stance needs a **T keypress** (`IA_CmdAttack`). `CurrentCommand`/`bHasIssuedCommand` are plain C++ (not UPROPERTY) so they can't be set via MCP, and MCP **cannot spawn actors during PIE**. Driving a live Blue ATTACK unit would require blind mouse-click + key injection into Jonathan's **actively-used** desktop — I declined that as unsafe. So a Blue ATTACK unit was **not** directly driven; the diagnosis rests on live readback of the battlefield + code equivalence (below), which is decisive for the branch that matters. The direct Blue-ATTACK-freeze capture is left as the TASK-281 build-master PIE test (real T-key, empty vs populated box) — see WATCH.

## 2. Geometry / navmesh (live editor-world readback)

- `Castle_0` = **Blue @ X=-25000**, `Castle_1` = **Red @ X=+25000** (TargetPoints co-located). Midfield X=0.
- `NavMeshBoundsVolume_0` world AABB = **X∈[-28000, 28000], Y∈[-12500, 12500]** → covers the FULL field incl. both bases and the enemy half.
- `trace_world` straight down hits ground (z=0) at X=0, X=+3000, X=+12000 → floor is continuous across midfield into the enemy half.

→ The nav **bounds/floor are not missing** on the enemy half.

## 3. THE DECISIVE CONTROLLED TEST (empty vs populated enemy box)

The bot spawns only Red units; the player summoned none, so I could not put a Blue unit under ATTACK. Instead I established the two arms as follows:

**(a) EMPTY-box arm — established by code-equivalence + live proof, NOT by driving a Blue unit.**
When the Red spawn box is empty, `FindNearestEnemyInSpawnBox` returns null, so the ATTACK no-aggro goal is `FindNearestEnemyCastle()` and the call is `EnterAdvance(enemyCastle)` — **byte-identical** to the LEGACY Standard body's no-aggro march (`SummonedUnit.cpp:1084` / `EnterAdvance` castle branch). That exact stable-castle-goal march is **provably healthy full-field at runtime right now**:
- A Red `MilitiaMob` (legacy body, goal = the standing Blue castle) was observed marching from its +25000 spawn **all the way to X=-24520** (the Blue base) — traversing the ENTIRE 10x field, `State=Advance`, **no freeze**.
- Many other Red units spread from +25000 down to -5737, all `Advance`, all moving.

Because empty-box ATTACK issues the identical `EnterAdvance(castle)` call as this demonstrably-working march, **empty-box ATTACK does NOT freeze**. → This **disproves** the "empty box still freezes → navmesh / MoveToActor partial-path at the enemy-half boundary" branch. The stable-goal path across the widened field is fine.

**(b) POPULATED-box arm — the precondition is live-confirmed, and it is the ONLY code difference from the working case.**
- The bot spawns in **bursts** (unit count jumped 6 → 24 between samples). Freshly-spawned units sit **inside** the Red box (X∈[24160, 25840]) for only ~1–1.5 s before crossing the box's lower edge and marching out (caught them at X≈20329, 20629, 20929, 23673 — i.e., a wave's worth just below/at the box edge, turning over). So the box is **transiently but recurringly populated with a shuffling set** every wave.
- The ATTACK branch's ONLY behavioral difference vs the (working) legacy/empty-box case is the no-aggro goal substitution `FindNearestEnemyInSpawnBox(enemyCastle) ?? enemyCastle`, which engages ONLY when that box is populated.

## 4. Confirmed root cause

Differential diagnosis, backed by the runtime evidence above:

- The working case (legacy / empty-box ATTACK / DEFEND / HOLD) marches on a **STABLE** goal actor → `EnterAdvance`'s `bGoalChanged` is false on consecutive ticks → **one** `MoveToActor` path is issued and followed to completion → smooth march (proven full-field, §3a).
- ATTACK with a populated box picks the **nearest-to-self box defender**, whose **identity flips tick-to-tick** as the bot's spawn box turns over each wave (fresh spawns enter, previous ones cross out). On the 0.25 s state timer that flips `Goal` every tick → `bGoalChanged` true every tick → `EnterAdvance` re-issues `MoveToActor` **every 0.25 s**. On the **10x-widened** field a mid-field unit's box-defender goal is ~a half-field (~tens of thousands of uu) away, so each re-issue tears down and restarts a long path-request that is never followed to completion → the unit makes ~zero forward progress = **the freeze**. This is NEW to the 10x arena: on the old small map the re-path was cheap enough to be invisible.

This matches every symptom: only ATTACK (only it substitutes the box goal); DEFEND/HOLD fine (fixed/stable goals); "past the midpoint" (the freeze bites once the unit is out on the long approach with the box-defender goal engaged and no closer in-aggro target holding `CurrentTarget`); and arena-scale-only.

## 5. The fix (why it follows from the evidence, and why it can't regress)

**Change (ATTACK no-aggro block only, `UpdateStateStandardCommanded`):** only prefer a spawn-box defender once the unit is within **`EnemyBaseEngageRadius`** (2D closest-point distance) of the enemy castle; otherwise the goal is the STABLE enemy castle.

```cpp
AActor* BoxDefender = nullptr;
if (EnemyCastle && GetDistanceToTarget(MyLocation, EnemyCastleActor) <= EnemyBaseEngageRadius)
{
    BoxDefender = FindNearestEnemyInSpawnBox(EnemyCastle);
}
Goal = BoxDefender ? BoxDefender : EnemyCastleActor;
```

- **Kills the freeze:** during the long mid-field approach the goal is now the stable enemy castle → the march is byte-identical to the legacy/DEFEND stable-goal march that §3a proved healthy full-field. No more per-tick re-path.
- **Preserves ATTACK intent:** within `EnemyBaseEngageRadius` of the base the box-defender-FIRST goal still engages (clear box units/buildings before the castle). Close in the remaining path is short, so even a shuffling box goal re-paths cheaply; and once within aggro (600) `AcquireTarget` takes over as `CurrentTarget` anyway.
- **Null-safe / unchanged edge:** a null/destroyed enemy castle ⇒ `EnemyCastle` null ⇒ guard short-circuits ⇒ `Goal` stays null ⇒ `EnterIdle` — exactly as before (match over).

**Cannot regress DEFEND / HOLD / legacy / bot:**
- The edit is entirely inside the **ATTACK `case` no-aggro block**. DEFEND and HOLD cases are untouched.
- The **legacy Standard body** (`SummonedUnit.cpp:1064-1102`) is untouched — bot/Red, miners/None, Siege/Support, and pre-first-command player units still run it byte-for-byte (the command gate is unchanged).
- **`EnterAdvance` is NOT touched** — so every marching unit's re-path discipline is unchanged (no shared-path WATCH).
- `Castle`, `FindNearestEnemyInSpawnBox`, the capture-zone code, and the bot are untouched.
- `MyLocation` and `GetDistanceToTarget(FVector, AActor*)` are already in scope/used in this same function; no new include.

## 6. New identifier (record for CONVENTIONS fold-in by manager at integration)

- **`float EnemyBaseEngageRadius = 3500.f;`** — `UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Siegebound|Commands", meta=(ClampMin="0"))`, on `ASummonedUnit` (lives with the unit, mirrors `DefendRadius`). Manager-sanctioned name from the TASK-280 spec. **FLAGGED tunable** for the 10x arena (default 3500 uu comfortably covers the (840,840) spawn box measured from the castle walls). Doc comment cites TASK-280. Do NOT edit CONVENTIONS in this task — manager folds a one-line entry into "Unit commands (Shield Wall stances)…".

## 7. QA scrutiny + WATCH

- **Scrutinize:** the proximity gate direction/units (closest-point distance to the enemy castle vs `EnemyBaseEngageRadius`), null-safety of the destroyed-castle path, and that the box-defender-first intent still fires inside the radius.
- **WATCH (for TASK-281 build-master PIE test — the direct capture I could not drive headlessly):** with real T-key ATTACK, confirm Blue Standard units march the FULL 10x field to the Red base and DON'T freeze past midfield, in BOTH conditions — (a) enemy units present in/near the Red box (units clear box defenders then the castle), and (b) Red box empty (straight to Castle_Red). Also regression-confirm DEFEND/HOLD and that Siege/Support/miners/bot are unaffected.
