# TASK-282 handoff — ATTACK "stops just short of the enemy castle": diagnose + fix

**Branch:** `m7.6-arena10x` · **Status:** ready-for-qa · **Code-only** (no compile, no Git — TASK-283 owns those)

**Files touched (ONLY these):**
- `Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.cpp` — ATTACK no-aggro goal now = the stable enemy castle (legacy parity); removed the dead `FindNearestEnemyInSpawnBox` definition.
- `Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.h` — removed the now-dead `EnemyBaseEngageRadius` UPROPERTY and `FindNearestEnemyInSpawnBox` declaration; updated the `UpdateStateStandardCommanded` doc-comment ATTACK bullet.

No temporary `UE_LOG` was added (see §1) — the mechanism was confirmed by live MCP readback on the CURRENTLY COMPILED build + code equivalence, so there is nothing to strip.

---

## 1. How it was instrumented (what I actually observed)

Editor was running the current compiled build with MCP up; PIE was not running, level = `/Game/Maps/L_Arena`. Following the TASK-280 method (I CANNOT drive a live Blue-ATTACK unit headlessly — ATTACK needs the player's T-key `IA_CmdAttack` + a gold-gated summon click, and `CurrentCommand`/`bHasIssuedCommand` are plain C++, not UPROPERTY, so MCP cannot set them; and Jonathan is asleep — no blind key injection), I ran a **Simulate-In-Editor** session (no player pawn, no input hijack) so the bot's Red units run the **LEGACY** body and I could read the running world via MCP `find_actors`/`get_properties`/`get_actor_transform` (all refPaths came back `UEDPIE_0_...` → confirmed I was reading the live PIE world, not the editor CDO). Session was stopped (`StopPIE`) when done.

Instead of a per-tick `UE_LOG`, the state fields I needed are directly readable as UPROPERTYs (`State` = `GetUnitState`, `CurrentTarget`, `Team`, `Profile`, `CardID`, `AttackRange`, `AggroRadius`) plus `get_actor_transform` for position and the castle's `CurrentHP`/`bDestroyed`/`crumbleStage`. That is the same evidence a log line would print, read live.

## 2. Geometry (live PIE readback)

- `Castle_0` = **Blue @ (-25000, 0, 0)**, `Castle_1` = **Red @ (+25000, 0, 0)**. Full 10× field.
- `ACastle::SpawnBoxHalfExtent` = **(840, 840)** → the Red spawn box spans X∈[24160, 25840], Y∈[-840, 840] around the Red castle. `EnemyBaseEngageRadius` (the removed gate) was 3500 uu closest-point from the castle collision.

## 3. THE DECISIVE runtime observation — legacy final approach DOES reach + destroy the enemy castle on the 10× field

Tracked one bot unit (`BP_Unit_Ogre_C_0`, **Team=Red, Profile=Siege**, `AttackRange=120`, `AggroRadius=600`) across the march:

| t (approx) | X position | State | CurrentTarget |
|---|---|---|---|
| ~30 s | **+8392** | Advance | `Castle_0` (Blue enemy castle) |
| ~115 s | **-24730** (the Blue castle wall) | **Idle** | **None** |

At the later sample I read the enemy castle directly:

- `Castle_0`: **`CurrentHP = 0`, `bDestroyed = true`, `crumbleStage = 3`** (fully destroyed).

So a bot unit marched the **entire** 10× field (+8392 → -24730, i.e. from midfield to the far wall) and the enemy castle was driven to 0 HP / destroyed. The Ogre is `Idle`/`CurrentTarget=None` at the wall **because the castle is destroyed** → `FindNearestEnemyCastle()` returns null → `EnterIdle` (match over) — exactly the code path.

**What this proves (and refutes):**
- The shared `EnterAdvance(castle)` → `MoveToActor(castle, StructureMoveAcceptanceRadius=50, bAllowPartialPath)` path brings a unit **flush to the castle wall**, and `EnterAttack` fires there and damages the castle to destruction. → **Refutes hypothesis (a)'s "the castle standoff/acceptance radius stops the unit OUTSIDE AttackRange so EnterAttack never triggers."** The castle final approach + attack is healthy on this build. (The distance metric is closest-point-on-collision — `ActorGetDistanceToCollision`, `GetDistanceToTarget` — so a unit at the wall is ~0 uu from the castle, well inside `AttackRange` and `AggroRadius(600)`.)
- The only reason the Siege Ogre reaches the castle by a DIFFERENT route than a Standard unit is that Siege sets `CurrentTarget=castle` unconditionally; but the **march + attack primitives it used (`EnterAdvance`, `EnterAttack`, `GetDistanceToTarget`) are the SAME ones the legacy Standard body and the ATTACK body use.** The legacy Standard body's no-aggro goal is likewise the stable castle (`FindNearestEnemyCastle`), and TASK-280 already runtime-proved a Standard `MilitiaMob` marches the full field on that stable goal without freezing.

(Only 4 bot units existed the whole session — 2 Miners [Profile None, economy, don't attack], 1 Ogre [Siege], 1 Cleric [Support] — the Simulate economy spawns slowly, so no Standard `MilitiaMob` happened to be in this sample. The castle-kill primitive is nonetheless proven, and it is shared.)

## 4. Confirmed root cause (the legacy-vs-ATTACK divergence)

The ATTACK branch (`UpdateStateStandardCommanded` case `Attack`) was **byte-identical to the legacy body EXCEPT one thing**: its no-in-aggro march **goal**. Legacy: `Goal = CurrentTarget ?? FindNearestEnemyCastle()` (the **stable** castle). ATTACK (pre-fix):

```cpp
if (EnemyCastle && GetDistanceToTarget(MyLocation, EnemyCastleActor) <= EnemyBaseEngageRadius) // 3500
    BoxDefender = FindNearestEnemyInSpawnBox(EnemyCastle);
Goal = BoxDefender ? BoxDefender : EnemyCastleActor;
```

i.e. **within 3500 uu of the enemy castle, the goal is the nearest enemy in the castle's spawn box, not the castle.** That single divergent line is the bug, in two compounding ways, both isolated by §3 (the castle path itself is fine):

1. **Goal thrash → "stops just short."** `FindNearestEnemyInSpawnBox` returns the nearest-**to-self** box occupant. The bot continually spawns into its box (waves — TASK-280 recorded the count jumping 6→24 as the box turns over), so the nearest-to-self identity **flips tick-to-tick**. On the 0.25 s state timer that flips `Goal` every tick → `EnterAdvance`'s `bGoalChanged` is true every tick → `MoveToActor` is torn down and re-issued to a different box unit every 0.25 s → the unit never follows one path to completion and **mills near the 3500 ring** = "marches up fine, then halts near Castle_Red." (This is the SAME re-path mechanism TASK-280 fixed mid-field; TASK-280's gate merely **relocated** the residual thrash into the 3500 final-approach band.)
2. **Castle never becomes the sustained target → never attacked.** While ANY enemy sits in the box, `Goal` is that box unit and never the castle, so `EnterAdvance(castle)` / an `EnterAttack` on the castle never happen. Because the bot **endlessly repopulates its own spawn box**, the box is essentially never empty, so an ATTACK unit that reaches the band **never advances to the wall and never attacks the castle** — precisely Jonathan's "halt near Castle_Red without attacking it."

The natural in-aggro path is NOT a rescue here: `AcquireTarget` only sets `CurrentTarget` within `AggroRadius(600)`; in the band the unit is >600 from the (flipping, and often outward-marching) box units, so nothing latches, and the box-goal branch keeps firing.

**Legacy escapes both** because its no-aggro goal is the stable castle: one `MoveToActor(castle)` followed to the wall, `AcquireTarget` picks up the castle at the wall (dist ~0 < 600), `EnterAttack` fires (§3 proof). The divergence is entirely the box-defender substitution.

## 5. The fix (why it follows from the evidence)

**Remove the box-defender goal substitution** so the ATTACK no-aggro march goal is the **stable enemy castle** — making the ATTACK case byte-identical to the legacy body's no-aggro path (which §3 proves reaches and destroys the castle):

```cpp
AActor* Goal = CurrentTarget;
if (!Goal)
{
    Goal = FindNearestEnemyCastle();
}
```

- **Kills the "stops just short":** the goal is now a single stable actor across the whole approach → `EnterAdvance` issues one path to the wall, no per-tick re-path. Subsumes (and strengthens) the TASK-280 anti-freeze — the goal is the stable castle over the ENTIRE approach, not just mid-field.
- **Makes ATTACK units attack the castle:** at the wall `AcquireTarget` returns the castle as `BestOther` (dist ~0 < AggroRadius) → `CurrentTarget=castle` → `dist<=AttackRange` → `EnterAttack` (the exact §3-proven castle-kill). Any defender that enters `AggroRadius(600)` on the way is still engaged first (unchanged self-defense / "engage the castle when no closer target remains").
- **Null-safe / unchanged edge:** null or destroyed enemy castle ⇒ `FindNearestEnemyCastle()` null ⇒ `Goal` null ⇒ `EnterIdle` (match over) — identical to before and to legacy.

Because `FindNearestEnemyInSpawnBox` and `EnemyBaseEngageRadius` were referenced ONLY by that deleted block (verified: repo-wide grep shows no other `.cpp` referrer; no Blueprint/`Content` asset serializes `EnemyBaseEngageRadius` — `Content` grep clean, and TASK-280 was code-only so no BP overrode its default), both are removed as dead code rather than left dangling.

## 6. Non-regression argument

- **The change is entirely inside the ATTACK `case` (no-aggro block).** DEFEND and HOLD cases are untouched.
- **Legacy Standard body (`SummonedUnit.cpp` ~:1064–1101) is byte-for-byte untouched.** Bot/Red units, miners (Profile None), Siege (Ogre — see §3), Support (Cleric), and pre-first-command player units all still run it unchanged (the command gate `Profile==Standard && Team==Blue && HasIssuedCommand()` is unchanged). After the fix the ATTACK no-aggro path now MATCHES that legacy body (only `break;` vs `return;` differs, as the switch requires) — the reference behavior the task named.
- **`EnterAdvance` / `EnterAttack` / `AcquireTarget` / `FindNearestEnemyCastle` are all untouched** — every other caller (legacy, DEFEND, HOLD, Siege, Support) is unaffected. No shared-helper WATCH.
- **TASK-280 anti-freeze is preserved/strengthened,** not regressed: the mid-field goal is still the stable castle (now the whole approach is).
- **`EnemyBaseEngageRadius` removal is safe:** `EditDefaultsOnly` default never overridden in any BP (Content grep clean), no other referrer.

## 7. Records for the manager / QA / build-master

- **RETIRED identifier (record for CONVENTIONS fold-out by manager at integration):** `EnemyBaseEngageRadius` (added TASK-280) is REMOVED. The CONVENTIONS "Unit commands (Shield Wall stances) — ATTACK / HOLD / DEFEND (W1, 2026-07-23)" entry's ATTACK clause should drop the `EnemyBaseEngageRadius` tunable line and the "clear spawn-box defenders first" wording — ATTACK now = legacy full-aggression march-to-enemy-castle. **FLAGGED design change** (retires the TASK-275 box-defender-FIRST behavior): it is incompatible with the task requirement "actually ATTACK the enemy castle," since the bot endlessly repopulates its box — box-FIRST means the castle is never attacked. Surfaced here for QA/manager ruling.
- **Out-of-scope stragglers I could NOT touch (task = SummonedUnit only) — for a future Castle-scoped pass:** removing `ASummonedUnit::FindNearestEnemyInSpawnBox` orphans `ACastle::IsPointInSpawnBox` (now unused; still compiles — public method, reads `SpawnBoxHalfExtent`, which remains a live 3-way paired tunable used by the two controllers for spawning). Two now-slightly-stale doc-comment references remain in `Castle.h` (~:122 names `ASummonedUnit::FindNearestEnemyInSpawnBox`; ~:159 "read by IsPointInSpawnBox"). All harmless; noted for cleanup.

## 8. QA scrutiny + WATCH

- **Scrutinize:** that the ATTACK no-aggro path is now truly legacy-equivalent (leash → `AcquireTarget` → `Goal=CurrentTarget ?? FindNearestEnemyCastle()` → Idle-if-null → attack-if-in-range else advance); null-safety of the destroyed-castle path; that no dangling reference to the two removed identifiers remains in `SummonedUnit.{h,cpp}` (grep: only the explanatory comment at cpp ~:1285 names them); complete-type include scan (`Castle.h` still included; `ACastle` still used by `FindNearestEnemyCastle`/`FindOwnCastle`); no unused-variable/shadow warnings introduced.
- **WATCH (for TASK-283 build-master PIE test — the direct capture I could not drive headlessly):** with real T-key ATTACK, confirm Blue Standard units march the FULL 10× field AND enter `EnterAttack` on `Castle_Red` (no halt short), in BOTH conditions — (a) enemy units in/near the Red box, and (b) Red box empty. Regression-confirm DEFEND/HOLD and Siege/Support/miners/bot unaffected.
