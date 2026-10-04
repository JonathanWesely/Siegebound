<!-- ARCHIVED from .claude/pipeline/TASKBOARD.md on 2026-10-04 by Tools/archive_board.py. Every row below was in a terminal state when moved; bytes are unchanged and this comment is the only addition. Law: TASKBOARD.md '## Archive'. -->
#### TASK-358 — [AG-T0] 180° rotational symmetry rewrite of the scatter + delete the dead hill-parity rollback (gameplay-programmer)
- assignee: gameplay-programmer
- status: **done** (2026-08-02 — INTEGRATED by TASK-378, commit A `6e79a24`. Gate satisfied: `qa/TASK-365-report.md` PASS 7/7, 0 BLOCKER; compile re-verified GREEN at TASK-378 (`Result: Succeeded`, target up to date); Jonathan's PIE ship gate TASK-377 PASSED.) ← was: **qa-passed** (2026-08-01 — TASK-365 PASS, 0 BLOCKER / 1 NIT; report `qa/TASK-365-report.md`. Zero RNG draws in the twin block confirmed; primary X draw RANGE narrowed only, count+order unchanged; layer/mine streams untouched. All SIX mine (−X,−Y) sites confirmed — the plan's list of five omitted the twin SpawnActor (:1553), the line that moves the actor. Rollback deletion RULED SOUND. Unconditional twin yaw +180 ACCEPTED; halved pair loop ACCEPTED (old mirror path was a latent count-doubler). NIT: odd targets ship `placed` 1 over `target` — documented, do not file.) ← was: ready-for-qa. **BUILD-MASTER PIE NOTE (carried from the implementer):** new log token `mirror=rot180`; the MinesPass `inj=` token was REMOVED with the deleted injection — its absence is expected, not a regression. Existing seeds now produce NEW layouts (ruling 8 / TASK-140 precedent).
- blocked-by: none
- parallel-safe: yes (file-disjoint from 359/360/362/363/364/369/374) — but **EXCLUSIVE owner of `BattlefieldScatter.{h,cpp}` + `ScatterConfig.h`; TASK-361 serializes behind this task**
- spec: >
    Implement CONVENTIONS §1 of "Ancient Grounds + Sorcerer + 180° terrain symmetry". **Read plan §1 first — it is the spec.**
    **(1) THE TRANSFORM:** replace the per-instance `if (bMirror)` twin block (`BattlefieldScatter.cpp:601-643`) with the ROTATIONAL twin —
    `TwinPoint = (−X, −Y)` (was `(−X, Y)`), `TwinYaw = Fmod(Yaw + 180, 360)` (unchanged), scale UNCHANGED. Narrow the primary draw X range to
    `[−ArenaHalfExtent.X, 0]`. **Everything else — biased-Y sampling, mesh index, scale, yaw — is unchanged.**
    **(2) KEEP IT INLINE, NOT A BULK POST-PASS** — a post-pass breaks three shipped invariants: `VisualToProxy` index parallelism
    (`:585-599`, `:631-641` — tree visual + collision proxy must be added in LOCKSTEP or the cull paths orphan visible trees),
    `SpacingGrid` registration of the twin (`:628`, so later primaries respect it), and `HillSurfaceComponents` registration for
    pass-1 blockers (`:450-458`).
    **(3) ⚠️ GENERATE THE BLUE HALF (X ≤ 0) — NOT NEGOTIABLE:** `PlayerStart` exists only at (−23800, 0, 100) and there is NO Red-side
    PlayerStart (`SiegeGameMode.cpp:684-687` confirms the fallback IS the design). Generating Red would rotate a legally-placed prop
    ONTO the hero spawn. The mine pass already draws `X < 0`, so this is consistent.
    **(4) MINES:** convert `PlaceMines` to the same law — `Y → −Y` at the five sites the plan names (`:1192`, `:1331`, `:1379`, `:1391`,
    `:1425`); the mine twin yaw is ALREADY 180, so no yaw change.
    **(5) DELETE THE DEAD HILL-PARITY ROLLBACK:** under a TRUE rotation the twin lands on the geometrically identical point of the
    rotated hill (same Z, same slope, always), so `TryResolveMinePair`'s clone/re-trace/rollback path (`:1241-1265`) is provably
    unreachable. Delete it and say in the handoff WHY it is unreachable (that argument is what QA checks). Keep the parts of
    `TryResolveMinePair` that are still live.
    **(6) Z HANDLING:** keep RE-TRACING the twin's ground Z via `GroundZAt(−X, −Y)` — do NOT copy the primary's Z. It preserves the
    null-safe code shape, and under exact rotation it MUST return the same Z, which is a free QA assertion. **Log any mismatch.**
    **(7) CONFIG:** replace the `bool bMirrorSymmetric` toggle (`ScatterConfig.h:282-290`) with an explicit single global symmetry-mode
    field. **Keep the `GenerateScatter seed=… mirror=…` log token grep-able** (`:319-322`) — record the new mode value there.
    **(8) LOG THE ASYMMETRY ESCAPES when they fire:** `CullCorridorBlockers` (`:1666`) and `RemoveBlockingInstancesInDisc` (`:1712`) stay
    side-agnostic (mirroring a cull would delete more geometry for zero traversability gain; shipped runs report 0 culls across every
    recorded PIE — TASK-287/291/295), but each MUST log when it fires because it locally breaks symmetry.
    **ACCEPTANCE:** ZERO RNG draws in the rotation step (the twin is computed, never sampled) · per-layer `TargetCount` semantics
    preserved (target 340 ⇒ ~170 pairs; `Placed` already increments for primary + twin at `:628`) · the traversability guarantee
    intact · **NO compile, NO Git, NO editor** — files only. Handoff `handoffs/TASK-358-programmer.md` must state: the unreachability
    argument for the deleted rollback, the new log-token value, and that existing seeds now produce new layouts (EXPECTED, ruling 8).
    Post in ⚙️ Dev & QA.
- names: >
    Edit: `Source/GitClaudeUnrealTest/Siegebound/BattlefieldScatter.{h,cpp}` · `Source/GitClaudeUnrealTest/Siegebound/ScatterConfig.h`.
    Read-only: everything else. Law: CONVENTIONS "Ancient Grounds + Sorcerer + 180° terrain symmetry (2026-08-01)" §1 + the amended
    ":130" placement-symmetry law + the amended mines law. Plan: `C:\Users\wesel\.claude\plans\there-is-one-new-glittery-bentley.md` §1.

#### TASK-359 — [AG-T1] NEW `AAncientGround` actor — zone box, decal, 1 Hz friendly-only boost tick (gameplay-programmer)
- assignee: gameplay-programmer
- status: **done** (2026-08-02 — INTEGRATED by TASK-378, commit A `6e79a24`. Gate satisfied: `qa/TASK-365-report.md` PASS 7/7, 0 BLOCKER; compile re-verified GREEN at TASK-378 (`Result: Succeeded`, target up to date); Jonathan's PIE ship gate TASK-377 PASSED.) ← was: **qa-passed** (2026-08-01 — TASK-365 PASS, 0 findings; report `qa/TASK-365-report.md`. HasAuthority count in AncientGround.cpp = 0 VERIFIED; bAuthoritativeBoost fail-closed; InitAncientGround its only writer; Tier C declared in header + handoff. Unconditional timer arm / gated tick body RULED CORRECT (spawn-order argument verified — `SpawnActor` runs `BeginPlay` INSIDE the call, so arming on the flag would leave a ground that never boosts on the server too). CaptureZone.h edit CONFIRMED comment-only — two /** */ blocks, zero symbols — NOT scope creep.) ← was: ready-for-qa. Decal `SetSortOrder(10)` present (UE 5.8 `UMaterial` has NO SortOrder field — it is a `UDecalComponent` property; prevents a Z-fight with the capture-zone decal near the centerline).
- blocked-by: none (all cross-task signatures are PINNED in CONVENTIONS §7)
- parallel-safe: yes (NEW files only)
- spec: >
    Create `AncientGround.h/.cpp` per CONVENTIONS §2 + plan §2. **Read plan §2 first.**
    **(1) ⚠️ MUST NOT derive from `ACaptureZone`.** `TActorIterator<ACaptureZone>` is a UNIT-SPAWN-ELIGIBILITY GATE
    (`SiegePlayerController.cpp:3559`, `SiegeBotController.cpp:1183/1380` — the bot takes the FIRST instance) plus the play-again reset
    (`SiegeGameMode.cpp:835`); subclassing would make ancient grounds spawnable-in and let one win the bot's first-instance race.
    **Duplicate** the ~80 lines of decal/box boilerplate from `CaptureZone.cpp:59-216` and **record the 2-mirror as debt in BOTH headers.**
    **(2) SHAPE:** plain `AActor`; `SceneRoot` + `UDecalComponent GroundDecal` (relative pitch −90, no collision primitive);
    `FVector2D ZoneHalfExtent = (840,840)` — **PAIRED TUNABLE with `ACaptureZone::ZoneHalfExtent`, both headers cross-note**;
    `bool IsPointInZone(const FVector&) const` (2D XY box, Z ignored — byte-copy of `CaptureZone.cpp:112`);
    `float BoostTickInterval = 1.0f` (`// GDD §x.x` mechanic rule, NOT a CSV column); `EndPlay` clears the timer.
    **NO `Reset…()` and NO `SiegeGameMode` edit** — the ground latches no state and PlayAgain step 7 already calls `ClearScatter()` +
    `GenerateScatter()`, which re-places the pair for free. Say that in the handoff so QA does not file a missing reset.
    **(3) ⚠️ AUTHORITY IS PUSHED, NEVER READ (ruling 4 — the most dangerous spot in the feature):** `void InitAncientGround(bool bAuthoritative)`
    stores the flag; the tick gates on the STORED `bAuthoritativeBoost` ONLY. **`HasAuthority()` must NOT appear anywhere in this file** —
    the actor is spawned LOCALLY ON CLIENTS from the replicated seed and keeps `ROLE_Authority` there, so a `HasAuthority()` guard would
    silently run a rogue client-side boost sim. Mirrors `RunScatterPasses(Seed, bAuthoritativeGenerate)`.
    **(4) ⚖️ DECLARE THE NET TIER IN THE HEADER AND THE HANDOFF: TIER C — NOT REPLICATED (`bReplicates` stays false).** Undeclared = QA FAIL.
    **(5) THE TICK (1 Hz TIMER, NEVER per-tick — TASK-004 law):** ONE `TActorIterator<ASummonedUnit>` sweep, two team buckets. Skip
    invalid/dead and out-of-zone; `IsAncientGroundEmpowerer()` ⇒ `++SorcererCount[team]` and `continue` (**a sorcerer NEVER self-boosts**);
    else require `CanReceiveDamageBoost()` to be an occupant. Second pass: each occupant gets `AddPermanentDamageStacks(SorcererCount[itsOwnTeam])`
    — **FRIENDLY-ONLY (Jonathan ruling iii)** and **per-sorcerer stacking (ruling iv)**. Zero grant ⇒ no call (no spurious broadcast).
    **(6) MATERIAL:** soft-load `/Game/Materials/M_AncientGround` NULL-SAFE — missing ⇒ log ONCE, no visual, **the mechanic still runs**.
    `DecalSize` after the −90 pitch: `.X` = projection half-DEPTH `1024`; `.Y`/`.Z` = `ZoneHalfExtent` (840/840) — the decal MATCHES the
    mechanic box, never shrinks. Set **`FadeScreenSize = 0.001`** explicitly (the 0.01 default culls decals at this arena's zoom-out).
    **ACCEPTANCE:** compiles against the PINNED signatures character-for-character · no `HasAuthority()` · tier declared · no per-tick work ·
    **NO compile, NO Git, NO editor.** Handoff `handoffs/TASK-359-programmer.md`. Post in ⚙️ Dev & QA.
- names: >
    NEW: `Source/GitClaudeUnrealTest/Siegebound/AncientGround.{h,cpp}` → class `AAncientGround`. Material contract
    `/Game/Materials/M_AncientGround` (authored by TASK-374). Read-only donors: `CaptureZone.{h,cpp}`, `BattlefieldScatter.cpp`
    (`RunScatterPasses` authority-threading pattern). Pinned API: CONVENTIONS §7. Law: CONVENTIONS §2. Plan §2.

#### TASK-360 — [AG-T2] `ASummonedUnit` boost state + compose points + NEW `ASorcererUnit` with the 3-point attack seal (gameplay-programmer)
- assignee: gameplay-programmer
- status: **done** (2026-08-02 — INTEGRATED by TASK-378, commit A `6e79a24`. Gate satisfied: `qa/TASK-365-report.md` PASS 7/7, 0 BLOCKER; compile re-verified GREEN at TASK-378 (`Result: Succeeded`, target up to date); Jonathan's PIE ship gate TASK-377 PASSED.) ← was: **qa-passed** (2026-08-01 — TASK-365 PASS, 0 BLOCKER / 1 NIT; report `qa/TASK-365-report.md`. All THREE guards cited (cpp:2050/:1583/:2307); both virtuals confirmed public: (h:390/:400 vs protected: at :450); guard-2 unreachability argument independently verified against all 15 CurrentTarget writes. PermanentDamageStacks has exactly TWO writers module-wide, both broadcasting. Boundary exactness verified in single precision — (100.f × 0.05f) rounds to exactly 5.0f, so 20/40/60/80 stacks land on exactly 100/200/300/400%. NIT: CardID ctor line RULED IN (AMinerUnit precedent).) ← was: ready-for-qa. **⚠️ Compiles only WITH TASK-362** — it consumes `FOnCombatantDamageBoostChanged` + the two defaulted interface virtuals from `HealthBarProvider.h`, by design.
- blocked-by: none (signatures PINNED)
- parallel-safe: yes — but **EXCLUSIVE owner of `SummonedUnit.{h,cpp}` for this batch** (TASK-362 must not touch it)
- spec: >
    Implement CONVENTIONS §3 + §4. **Read plan §3 and §4 first.**
    **(1) TWO NEW VIRTUALS ON `ASummonedUnit`, IN THE `public:` BLOCK (`SummonedUnit.h:121-345`) — ⚠️ NOT beside `ShouldHoldDeathAnim()`
    at `:582`, which is `protected:`; `AAncientGround` calls these from OUTSIDE, so protected would fail to link (manager ruling 2):**
    `virtual bool CanEverAttack() const` (base `true`) and `virtual bool IsAncientGroundEmpowerer() const` (base `false`).
    **(2) THREE ATTACK-SEAL GUARD POINTS — ALL THREE, ALL VERIFIED (ruling 6):** `EnterAttack()` (`:1964`, the structural chokepoint all
    four attack entries funnel through) ⇒ `if (!CanEverAttack()) { EnterIdle(); return; }` (**stand down, do not silently return** — it keeps
    the state machine honest); `UpdateStateGrouped()` (`:1464`) ⇒ force `CurrentTarget = nullptr` INSTEAD of the two `AcquireEnemyNearPoint`
    calls (`:1518-1525`), falling through to tier-3 station-keeping (without this a grouped sorcerer walks to an enemy and stands there);
    `PerformAttack()` (`:2207`) ⇒ add `|| !CanEverAttack()` to the existing `bAIFrozen`/`bSpellFrozen` gate (defense in depth).
    **WHY three:** `AttackCadence = FMath::Max(Row->Cadence, 0.05f)` (`:1069`) means a Cadence-0 row that ever reached Attack fires **20×/s**.
    **In the handoff, state what is NOT needed so QA does not hunt:** the legacy Standard body and DEFEND stance are already dead via the
    ctor's `AggroRadius = 0` / `DefendRadius = 0`, and Siege/Detonate paths are unreachable (`Standard`, `bSuicide false`).
    **(3) BOOST STATE — INTEGER STACKS, NEVER A FLOAT** (the UI must distinguish EXACTLY 100/200/300% from just-past-them; 80 additions of
    0.05 makes "exactly 100%" epsilon-dependent). `int32 PermanentDamageStacks = 0` (VisibleInstanceOnly, Transient) ·
    `float PermanentDamageBonusPerStack = 0.05f` (`// GDD §x.x`, EditAnywhere) · `int32 MaxPermanentDamageStacks = 80` (`// GDD §x.x` — the
    +400% cap) · `float GetPermanentDamageMultiplier() const` **BlueprintPure** = `1 + PerStack * Stacks`.
    **(4) MUTATION + ELIGIBILITY:** `AddPermanentDamageStacks(int32)` clamps to Max and **broadcasts ONLY on an actual change**;
    `ClearPermanentDamageStacks()` **broadcasts UNCONDITIONALLY** (reset path); `CanReceiveDamageBoost() const` =
    `!bDead && CanEverAttack() && AttackDamage > 0 && Profile != Support` (excludes Sorcerer/Miner/Cleric — units whose damage routes
    through neither compose point — which also keeps their boost row hidden). **Authority is by construction** (the sole caller is the
    gated ancient-ground tick) — **COMMENT it, do NOT bolt on a guard that buys nothing in P1.**
    **(5) COMPOSE POINTS — TWO, and never mutate `AttackDamage` in place (house buff law):** (a) `ComputeOutputDamage` (`:2339`), ONE line
    immediately after the War Banner aura at `:2366`: `Output *= GetPermanentDamageMultiplier();` — multiplicative, exactly 1.0 at zero
    stacks, and it covers BOTH melee and ranged and every keyword unit; (b) `ApplyDetonation` (`:2650`) currently passes RAW `AttackDamage`,
    bypassing the chokepoint ⇒ `const float BlastDamage = AttackDamage * GetPermanentDamageMultiplier();`. **(b) is a DELIBERATE change to a
    shipped unit** (dying IS how a Sapper attacks; an unboosted blast would be a visible lie). Charge/Slayer/Aura are deliberately NOT
    retro-applied there — a separate pre-existing gap, recorded, not fixed here.
    **(6) RESET:** `ClearPermanentDamageStacks()` in `HandleDeath` (`:2654`) next to the existing HP broadcast at `:2677` — required because
    the rigged-death path defers `Destroy()` up to 2 s, during which a boosted corpse would show a full boost bar. **Play Again needs ZERO
    work** (its step 2 destroys every unit). **Match-end freeze deliberately does NOT reset** — say so in the handoff so QA does not file it.
    **(7) `IHealthBarProvider` OVERRIDES LIVE HERE (not in TASK-362):** the `FOnCombatantDamageBoostChanged OnDamageBoostChanged` member,
    `GetDamageBoostPercent()` and `GetDamageBoostChangedDelegate()`. **Broadcast on EVERY mutation** — the same discipline as `OnHPChanged`'s
    four sites; miss one and the bar is stale forever.
    **(8) M8 P2 DUTY — record it in the header, do not silently omit:** `PermanentDamageStacks` becomes
    `UPROPERTY(ReplicatedUsing = OnRep_PermanentDamageStacks)` when the fleet replicates, the OnRep re-broadcasting so the client's
    seed-then-bind path is identical.
    **(9) NEW `SorcererUnit.{h,cpp}` (~70 lines):** `ASorcererUnit : public ASummonedUnit`; ctor sets `AggroRadius = 0`, `DefendRadius = 0`;
    `CanEverAttack() → false` (THE SEAL); `IsAncientGroundEmpowerer() → true`. **`StateCheckInterval` STAYS at the base 0.25 s** — unlike the
    Miner, this unit MUST keep its state timer because it is commandable. **No spawn-path change at all** (`IsChildOf(ASummonedUnit)` at
    `SiegePlayerController.cpp:3084` passes by inheritance). `Profile` stays `Standard` in CSV — that is what makes it commandable via
    `IsGroupCommandEligible()` (`:1576`).
    **ACCEPTANCE:** the pinned signatures character-for-character · `public:` placement of the two virtuals · all three guards present ·
    the two compose points present · **NO compile, NO Git, NO editor.** Handoff `handoffs/TASK-360-programmer.md`. Post in ⚙️ Dev & QA.
- names: >
    Edit: `Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.{h,cpp}`. NEW: `Source/GitClaudeUnrealTest/Siegebound/SorcererUnit.{h,cpp}`
    → class `ASorcererUnit`. Consumes `FOnCombatantDamageBoostChanged` from `HealthBarProvider.h` (TASK-362 declares it — PINNED).
    BP contract: `/Game/Blueprints/Units/BP_Unit_Sorcerer` reparents to `ASorcererUnit` (TASK-375). Law: CONVENTIONS §3 + §4 + §7. Plan §3 + §4.

#### TASK-361 — [AG-T3] `PlaceAncientGrounds` scatter pass + `USiegeScatterConfig` ancient-ground fields (gameplay-programmer)
- assignee: gameplay-programmer
- status: **done** (2026-08-02 — INTEGRATED by TASK-378, commit A `6e79a24`. Gate satisfied: `qa/TASK-365-report.md` PASS 7/7, 0 BLOCKER; compile re-verified GREEN at TASK-378 (`Result: Succeeded`, target up to date); Jonathan's PIE ship gate TASK-377 PASSED.) ← was: **qa-passed** (2026-08-01 — TASK-365 PASS, 0 BLOCKER / 2 WARN / 1 NIT; report `qa/TASK-365-report.md`. Both InitAncientGround(bAuthoritativeGenerate) calls confirmed (:1831/:1841); exactly 2 GroundStream draws/attempt X-then-Y, everything downstream draw-free; pass sits at :395, BEFORE the authority early-out at :401, so host==client holds. RULINGS: two-arg signature ACCEPTED (private, absent from §7, zero cross-task surface) — manager: amend CONVENTIONS §2's one-arg wording; no keep-clear disc ACCEPTED; SymmetryMode-independence ACCEPTED (fairness law) — manager: record in §2. WARN-1: the code comment + plan §2 cite castle r=4500; shipped `CastleKeepClearRadius` is 1500 (`ScatterConfig.h:354`) — the DECISION is right (at real values the disc bites at |X| ≥ 23500, so a test would be a no-op clearing the 21000 ceiling by 2500 uu) but the derivation is wrong; the real binding constraint on raising `AncientGroundMaxAbsX` is `SpawnBoxHalfExtent 2460` (box edge 22540, 700 uu past the 21840 footprint edge). WARN-2: handoff §1's "HasAuthority = 0 in BattlefieldScatter.cpp" is wrong — it is 2 (:210 BeginPlay / :237 GenerateScatter), both correct shipped M8 D9 guards on a genuinely replicated actor, neither in or near the new pass; build-master's grep returning 2 is NOT a regression.) ← was: ready-for-qa
- blocked-by: **TASK-358** (SAME FILE — hard serialization, ruling 3) · **TASK-359** (needs the `AAncientGround` type)
- parallel-safe: no — shares `BattlefieldScatter.{h,cpp}` + `ScatterConfig.h` with TASK-358
- spec: >
    Add the ancient-ground placement pass per CONVENTIONS §2 ("Placement") + plan §2. **Read plan §2 first.**
    **(1) `void PlaceAncientGrounds(int32 Seed)`** on `ASiegeBattlefieldScatter`, called from `RunScatterPasses` (`:306`) **immediately after
    `PlaceMines(Seed)` (`:358`)**. It follows the TASK-358 law — **draw on the BLUE half, emit the ROTATED twin `(−X, −Y)` yaw+180** — so it
    needs NO exception. Spawn both actors and call `InitAncientGround(bAuthoritativeGenerate)` on each, threading the SAME flag
    `RunScatterPasses` already carries (ruling 4 — the ground never reads `HasAuthority()`).
    **(2) SEED DISCIPLINE (HARD QA CRITERION):** dedicated `FRandomStream GroundStream(Seed ^ 0x41474E44)` ("AGND"; mines use `^0x4D494E45`),
    **EXACTLY TWO DRAWS PER ATTEMPT IN FIXED X-THEN-Y ORDER**, everything downstream draw-free — the discipline `PlaceMines` already enforces.
    **Layer streams are UNTOUCHED.**
    **(3) NEW `USiegeScatterConfig` FIELDS, category `Scatter|AncientGrounds`** (defaults ARE the law; all flagged tunable):
    `AncientGroundHalfExtent` **(840,840)** ≡ `ACaptureZone` · `AncientGroundMinAbsX` **4000** (leaves 2320 uu clear between the capture zone
    and the nearest ground) · `AncientGroundMaxAbsX` **21000** (castle keep-clear is r=4500 at ±25000; spawn boxes start at |X|=22540) ·
    `AncientGroundMaxAbsY` **10800** (`ArenaHalfExtent.Y 12000 − 1200` margin; box edge 11640 < ground 12500 < navmesh 13888) ·
    `AncientGroundMineClear` **1800** (tested at **BOTH** P and P′) · `AncientGroundClearRadius` **1200**
    (`RemoveBlockingInstancesInDisc` at **BOTH** P and P′).
    **(4) SLOPE: REJECT HILLS OUTRIGHT — do NOT parity-clone a hill.** A 1680² gathering box needs FLAT ground. Use `FindHillSurfaceAt` to
    reject, not to accommodate.
    **(5) THE RESERVED CORRIDOR (`|Y| ≤ 1000`) IS **NOT** EXCLUDED** — the same ruling as the mines: the actor is no-collision/no-nav so it
    cannot break the traversability guarantee, and a lane objective is good contested design. Do not "fix" this.
    **(6) 48 ATTEMPTS, then a DETERMINISTIC FALLBACK at `P = (−12000, +6000)` logged at `Error`** (the mines' "never ships short" discipline).
    **(7) GREP-ABLE LOG LINE (QA reads it):** `[BattlefieldScatter] AncientGroundsPass seed=%d P=(...) M=(...) fb=%s culls=%d`.
    **(8) LIFECYCLE:** track in `TArray<TObjectPtr<AAncientGround>> SpawnedAncientGrounds`, destroyed in `ClearScatter` **exactly like
    `SpawnedMines`**. No `SiegeGameMode` edit — Play Again's `ClearScatter()` + `GenerateScatter()` re-places the pair for free.
    **ACCEPTANCE:** zero draws outside the two-per-attempt X-then-Y pair · both grounds spawn every match (fallback proves it) ·
    `OnRep_GenerationIndex` re-runs the pass on the client identically · **NO compile, NO Git, NO editor.**
    Handoff `handoffs/TASK-361-programmer.md`. Post in ⚙️ Dev & QA.
- names: >
    Edit: `Source/GitClaudeUnrealTest/Siegebound/BattlefieldScatter.{h,cpp}` (after TASK-358 lands) · `ScatterConfig.h`. Spawns
    `AAncientGround` (TASK-359). Pinned API: `InitAncientGround(bool)`. Law: CONVENTIONS §2. Plan §2.

#### TASK-362 — [AG-T4] Boost-bar plumbing: `IHealthBarProvider` defaulted virtuals + widget BIE + C++ banding (gameplay-programmer)
- assignee: gameplay-programmer
- status: **done** (2026-08-02 — INTEGRATED by TASK-378, commit A `6e79a24`. Gate satisfied: `qa/TASK-365-report.md` PASS 7/7, 0 BLOCKER; compile re-verified GREEN at TASK-378 (`Result: Succeeded`, target up to date); Jonathan's PIE ship gate TASK-377 PASSED.) ← was: **qa-passed** (2026-08-01 — TASK-365 PASS, 0 findings; report `qa/TASK-365-report.md`. All 8 CeilToInt bands recomputed correct; unconditional-seed-then-conditional-bind confirmed (cpp:120 then :125-131); SummonedUnit/Building/HeroCharacter confirmed untouched by module-wide grep. RULINGS: the defensive FMath::Min(Frac, 1.f) — **KEEP IT, do NOT delete** (it is the only guard between a future >400% source and SetPercent > 1); no EndPlay unbind ACCEPTED as parity with the shipped HP binding.) ← was: ready-for-qa. Handoff `handoffs/TASK-362-programmer.md` carries the exact 5-arg `SetDamageBoost` signature + the four band RGB triplets + the `BoostBar` track color for TASK-368.
- blocked-by: none (signatures PINNED; the `ASummonedUnit` half belongs to TASK-360 — **do not touch `SummonedUnit.{h,cpp}`**)
- parallel-safe: yes (file-disjoint)
- spec: >
    Implement CONVENTIONS §5. **Read plan §5 first — including WHY a second WidgetComponent is broken by construction** (screen-space
    layout at constant pixel `DrawSize` ⇒ the world-ΔZ→screen-gap mapping changes with zoom; no fixed `BarHeightZ` works, per-frame
    correction violates the never-per-tick law, and it would double the screen-space registration surface TASK-130 spent five attempts on).
    **The TASK-131 corruption fear does NOT apply** — that was a Blueprint CLASS REPARENT that orphaned BIE overrides; re-nesting `Bar`
    under a `VerticalBox` is a slot/outer change and the parent class is never touched.
    **(1) `HealthBarProvider.h`:** declare `DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCombatantDamageBoostChanged, float, BoostPercent);`
    and add TWO **DEFAULTED** virtuals to `IHealthBarProvider` — `virtual float GetDamageBoostPercent() const { return 0.f; }` and
    `virtual FOnCombatantDamageBoostChanged* GetDamageBoostChangedDelegate() { return nullptr; }` (**a POINTER, so "not boostable" is
    expressible**). **Defaulted ⇒ `ABuilding` and `AHeroCharacter` need ZERO changes** — do not edit them.
    **(2) `UCombatantHealthBarWidget`:** ONE atomic BlueprintImplementableEvent —
    `void SetDamageBoost(float FillFraction, float R, float G, float B, float RowOpacity);` — **ONE event, not two**, because unlike the team
    tint the COLOR CHANGES WITH THE VALUE, and splitting fill from color creates a frame where band-3 purple paints at a band-4 fill.
    **All banding math is C++; the widget is a dumb pipe with ZERO conditionals.**
    **(3) `UCombatantHealthBarComponent`:** `DrawSize (90,12) → (90,22)`. **Seed UNCONDITIONALLY in BeginPlay** (this drives a non-boostable
    actor's row to opacity 0 rather than leaving the design-time state — the `qa/TASK-005` seed-then-bind law), **THEN** bind only if
    `GetDamageBoostChangedDelegate()` is non-null. **BANDING:** `Band = Clamp(CeilToInt(B / 100), 1, 4)` ·
    `Frac = Max((B − (Band−1)*100) / 100, MinBoostFillFraction /*0.04*/)`. `CeilToInt` is upper-inclusive BY DESIGN: **100% = full light blue,
    100.1% = nearly-empty dark blue, 400% = full black**, with no special case. **Row opacity 0 when BoostPercent <= 0.**
    **(4) BAND COLORS = `EditDefaultsOnly FLinearColor` on the component (tint is DATA, never hardcoded in the WBP):** band 1
    `(0.55, 0.80, 1.00)` · band 2 `(0.010, 0.020, 0.350)` · band 3 `(0.200, 0.010, 0.420)` · band 4 `(0.010, 0.010, 0.014)`. Plus the
    `BoostBar` track color `(0.22, 0.22, 0.24, 0.85)` as an EditDefaultsOnly value **for the record** (the artist authors the brush at
    TASK-368 from THIS number). The ramp darkens monotonically so "deeper = stronger" reads without a tooltip; band 3 is deliberately a
    DEEP purple (a bright violet computes to ~1.06:1 and vanishes).
    **ACCEPTANCE:** `SummonedUnit.{h,cpp}` **UNTOUCHED** · `Building`/`HeroCharacter` **UNTOUCHED** · pinned signature character-for-character ·
    no per-tick work · **NO compile, NO Git, NO editor.** Handoff `handoffs/TASK-362-programmer.md` must include the exact 5-arg
    `SetDamageBoost` signature and the four band RGB triplets **for TASK-368 to consume verbatim.** Post in ⚙️ Dev & QA.
- names: >
    Edit: `Source/GitClaudeUnrealTest/Siegebound/HealthBarProvider.h` · `CombatantHealthBarWidget.{h,cpp}` · `CombatantHealthBarComponent.{h,cpp}`.
    Widget asset contract (authored later, names are LAW): `/Game/UI/WBP_CombatantHealthBar` → `BarStack` (VerticalBox) ▸ `BoostOutline`
    (Border) ▸ `BoostBar` (ProgressBar) + the EXISTING `Bar` (ProgressBar). Law: CONVENTIONS §5 + §7. Plan §5.

#### TASK-363 — [AG-T5] `USiegeCheatManager::SetTestDamageBoost` — the deterministic PIE lever (gameplay-programmer)
- assignee: gameplay-programmer
- status: **done** (2026-08-02 — INTEGRATED by TASK-378, commit A `6e79a24`. Gate satisfied: `qa/TASK-365-report.md` PASS 7/7, 0 BLOCKER; compile re-verified GREEN at TASK-378 (`Result: Succeeded`, target up to date); Jonathan's PIE ship gate TASK-377 PASSED.) ← was: **qa-passed** (2026-08-01 — TASK-365 PASS, 0 BLOCKER / 1 NIT; report `qa/TASK-365-report.md`. All ten PIE rows independently recomputed in double — all land exactly. RULINGS: CEIL + 1e-4 epsilon ACCEPTED and **LOAD-BEARING** (round-to-nearest maps 101→100%, making the human gate's most important row unperformable); eligibility filtering on both paths ACCEPTED; reflection read ACCEPTED as shipped (loud failure, instance value, pinned name); ResolveNearestSearchOrigin extraction ACCEPTED — semantics diffed identical on the only reachable path, **do NOT revert**. NIT: sub-resolution positive Percent yields 0 stacks — recorded only.) ← was: ready-for-qa
- blocked-by: none (signatures PINNED)
- parallel-safe: yes (file-disjoint)
- spec: >
    Add `UFUNCTION(exec) void SetTestDamageBoost(float Percent, bool bAllFriendly);` to `USiegeCheatManager` per CONVENTIONS §6.
    **Without this lever the human PIE gate (TASK-377) is "walk a unit onto a ground and hope you hit exactly 100.0%" — that is why this
    is a real task and not a nicety.**
    **ROUTE IT THROUGH THE SHIPPING PATH:** convert `Percent` to an integer stack count using the unit's OWN
    `PermanentDamageBonusPerStack` (never a hardcoded 0.05) and apply via `ClearPermanentDamageStacks()` then
    `AddPermanentDamageStacks(n)` — **NEVER a raw field write**, so the `FOnCombatantDamageBoostChanged` delegate still broadcasts and the
    bar is exercised end-to-end. `Percent <= 0` ⇒ clear only. Values above the cap must CLAMP (500 behaves identically to 400).
    `bAllFriendly == true` ⇒ every live friendly `ASummonedUnit`; `false` ⇒ the unit under the crosshair / nearest friendly (mirror the
    existing `ApplyTestDamage` targeting). Null-safe everywhere; never a crash on an empty field.
    **ACCEPTANCE:** matches the existing exec-cheat idiom (`SummonTestUnit` / `ApplyTestDamage` / `AddTestGold`) · non-shipping by
    construction · **NO compile, NO Git, NO editor.** Handoff `handoffs/TASK-363-programmer.md`. Post in ⚙️ Dev & QA.
- names: >
    Edit: `Source/GitClaudeUnrealTest/Siegebound/SiegeCheatManager.{h,cpp}`. Pinned API consumed: `AddPermanentDamageStacks(int32)`,
    `ClearPermanentDamageStacks()`, `PermanentDamageBonusPerStack`. Law: CONVENTIONS §6 + "Debug exec cheats". Plan §7 step 2.

#### TASK-364 — [AG-desc] Sorcerer deck-builder description — the CardID-keyed glossary rule line (gameplay-programmer)
- assignee: gameplay-programmer
- status: **done** (2026-08-02 — INTEGRATED by TASK-378, commit A `6e79a24`. Gate satisfied: `qa/TASK-365-report.md` PASS 7/7, 0 BLOCKER; compile re-verified GREEN at TASK-378 (`Result: Succeeded`, target up to date); Jonathan's PIE ship gate TASK-377 PASSED.) ← was: **qa-passed** (2026-08-01 — TASK-365 PASS, 0 findings; report `qa/TASK-365-report.md`. All 14 truth claims verified
  against code ON DISK, each landed on a file:line; the narrowing to "every friendly unit that fights" is the CORRECT call
  (unqualified would be false, since `CanReceiveDamageBoost` excludes miners/Support/other sorcerers). else-if placement cannot
  shadow or be shadowed; string literals ASCII-only. RULING: qualitative magnitudes ACCEPTED under CONVENTIONS §8 —
  interpolation confirmed non-compiling (`SummonedUnit.h:652`/`:662` inside `protected:` at `:450`). ⚠️ See the report's
  SIMPLIFICATION verdict: two public getters would retire this workaround AND stop the text going stale when 5%/400% is
  retuned — scoped follow-up, not a blocker.) ← was: ready-for-qa
- blocked-by: none
- parallel-safe: yes (file-disjoint)
- spec: >
    **WHY THIS IS A CODE TASK AND NOT A CSV CELL (manager finding, verified — the plan's framing needed correcting):**
    `UDeckBuilderWidget::GetCardDescription` (`DeckBuilderWidget.cpp:424-471`, composers at `:734-926`) **does NOT read the `Notes` column** —
    it composes from row FIELDS plus CardID-keyed glossary strings. For the Sorcerer's Damage-0/Range-0/Cadence-0 row it emits **no melee
    lie** (the melee branch at `:806-809` requires `Row.Damage > 0` INSIDE a `Row.Range > 0` block, and both are 0) — but it also says
    **NOTHING about what the card does**, which fails the TRUTH LAW in the other direction: a 60-gold card whose panel reads only
    "Unit · Cost 60 gold · Max 2 per deck / Health: 70 / Move speed: 350".
    **THE FIX:** add a Sorcerer branch to `AppendRuleLines` using the EXACT shipped idiom of `MinerRole` / `DeepMineRole` / `MasonsRole` —
    a `SorcererRole` glossary string in the `SiegeboundCardGlossary` namespace + `const FName GlossaryCardID_Sorcerer(TEXT("Sorcerer"));`
    mirroring the play path's keying (comment it like its neighbours at `:145-151`).
    **THE LINE MUST STATE, and must be TRUE of the shipped code:** it cannot attack; while it stands in an ancient ground it PERMANENTLY
    strengthens every FRIENDLY unit standing there; each sorcerer adds its own share every second; the boost STACKS, is KEPT when the unit
    leaves the ground, is LOST when the unit dies, and is CAPPED.
    **MAGNITUDES:** interpolate from the owning UPROPERTY mechanic rules where the widget can reach them, or state them QUALITATIVELY —
    **never hardcode a number that can drift** (the anti-drift ruling that created this generator in the first place). Player-facing text
    carries **no GDD refs, no class names, no property names** (the shipped house style — read the glossary block before writing).
    **ACCEPTANCE:** no other card's description changes by one character · the truth claims match TASK-360's shipped behavior ·
    **NO compile, NO Git, NO editor.** Handoff `handoffs/TASK-364-programmer.md` quotes the final composed Sorcerer panel text verbatim so
    QA can diff it against the code. Post in ⚙️ Dev & QA.
- names: >
    Edit: `Source/GitClaudeUnrealTest/Siegebound/DeckBuilderWidget.{h,cpp}`. New symbols: `SiegeboundCardGlossary::SorcererRole`,
    `GlossaryCardID_Sorcerer` (FName `Sorcerer` — character-for-character the cards.csv row name). Law: CONVENTIONS §8 (TRUTH LAW clause)
    + "Deck-builder card details". Row already shipped in `Docs/Data/cards.csv`.

#### TASK-365 — [AG-QA] Pre-compile review of the whole C++ lane, determinism + draw-order focused (qa-reviewer)
- assignee: qa-reviewer
- status: **done** (2026-08-01 — VERDICT **PASS**, 7 of 7 tasks; 0 BLOCKER / 2 WARN / 4 NIT. Report `qa/TASK-365-report.md` ⚠️ **NOTE THE FILENAME** — TASK-366's spec says `qa/TASK-365.md`; the `-report` file is the authoritative one, append compile errors THERE. All named criteria (A)-(I) addressed by name; 13 flagged judgement calls RULED, none rejected. Batch cleared for TASK-366 compile. Manager follow-ups: 2 CONVENTIONS §2 amendments, the plan's r=4500 correction, and the two-public-getters simplification.)
- blocked-by: TASK-358, TASK-359, TASK-360, TASK-361, TASK-362, TASK-363, TASK-364
- parallel-safe: no
- spec: >
    Pre-compile review of all seven code tasks. Write `qa/TASK-365.md`. **These are the NAMED criteria — a report that does not address
    each one BY NAME is incomplete:**
    **(A) DETERMINISM / DRAW ORDER (focus on TASK-358 + TASK-361):** **ZERO RNG draws in the rotation step** (the twin is COMPUTED, never
    sampled) · `PlaceAncientGrounds` makes **exactly two draws per attempt in fixed X-then-Y order** from its own
    `FRandomStream(Seed ^ 0x41474E44)` and nothing downstream draws · **layer streams untouched** · a same-seed re-run must reproduce every
    log line **byte-identically** · host == client (the pass is re-run from the replicated seed on the client). ⚠️ **EXISTING SEEDS
    PRODUCING NEW LAYOUTS IS EXPECTED — DO NOT FILE IT** (manager ruling 8; TASK-140 precedent).
    **(B) THE AUTHORITY INVERSION:** **`HasAuthority()` must NOT appear anywhere in `AncientGround.cpp`** — the boost tick gates on the
    PUSHED `bAuthoritativeBoost` from `InitAncientGround(bool)`, and TASK-361 must thread `RunScatterPasses`' own flag into it. This is
    the single most dangerous spot in the feature (manager ruling 4).
    **(C) NET TIER:** `AAncientGround` declares **TIER C — not replicated** in its header comment AND its handoff. **Undeclared = FAIL.**
    **(D) THE THREE ATTACK GUARDS — cite ALL THREE by file:line** (`EnterAttack`, `UpdateStateGrouped`, `PerformAttack`). Two out of three
    is a FAIL: a Cadence-0 row that reaches Attack fires 20×/s.
    **(E) PINNED SIGNATURES:** every symbol in CONVENTIONS §7 matches character-for-character across the tasks that declare and consume it
    (UBT compiles the whole module — a drifted signature means the batch does not link). **Explicitly verify `CanEverAttack()` and
    `IsAncientGroundEmpowerer()` are in `ASummonedUnit`'s `public:` block**, not `protected:`.
    **(F) BOOST CORRECTNESS:** integer stacks (never a float) · clamp at `MaxPermanentDamageStacks` · `AddPermanentDamageStacks` broadcasts
    ONLY on change, `ClearPermanentDamageStacks` broadcasts UNCONDITIONALLY · **both** compose points present (`ComputeOutputDamage` AND
    `ApplyDetonation`) · `AttackDamage` **never mutated in place** · `HandleDeath` clears · **match-end freeze deliberately does NOT reset
    (not a miss)** · seed-then-bind is UNCONDITIONAL-seed-then-conditional-bind.
    **(G) FILE OWNERSHIP:** TASK-362 did not touch `SummonedUnit.{h,cpp}`; `Building`/`HeroCharacter` untouched (the defaulted-virtual
    promise); TASK-361's edits sit on top of TASK-358's, not beside them.
    **(H) THE STANDING COMPILE TRAPS (restate + scan):** the **shadow law** (no local/param/loop var shadowing an inherited reflected
    UPROPERTY — C4457/C4458/C4459 are HARD ERRORS) · the **complete-type include law** (any `.cpp` that dereferences or `Cast<>`s a
    forward-declared pointer must `#include` the full header — C2027/C2227) · **no literal `*/` inside doc comments** ·
    `FString::Printf` format strings literal/`constexpr` (UE 5.8 `TCheckedFormatString`, the TASK-268 C7595 lesson).
    **(I) TRUTH:** TASK-364's composed Sorcerer text matches TASK-360's shipped behavior claim for claim.
    **NO edits, NO engine, NO Git.** Verdict per task. Post the verdict + report path in ⚙️ Dev & QA.
- names: >
    Report `qa/TASK-365.md`. Reviews: `BattlefieldScatter.{h,cpp}`, `ScatterConfig.h`, `AncientGround.{h,cpp}`, `SummonedUnit.{h,cpp}`,
    `SorcererUnit.{h,cpp}`, `HealthBarProvider.h`, `CombatantHealthBar{Widget,Component}.{h,cpp}`, `SiegeCheatManager.{h,cpp}`,
    `DeckBuilderWidget.{h,cpp}`. Law: CONVENTIONS "Ancient Grounds + Sorcerer + 180° terrain symmetry" §1–§8 + the M8 NET RELEVANCY LAW.

#### TASK-366 — [AG-B1] Compile the batch + boot-PIE smoke + nav-settle measurement (build-master)
- assignee: build-master
- status: **done** (2026-08-01 18:45 local — **COMPILE GREEN**, boot-PIE smoke GREEN, nav settle measured, determinism PARTIAL with one characterized finding. Report `handoffs/TASK-366-buildmaster.md`. **NO COMMIT — TASK-378 owns it, gated on TASK-377.**)
    **(1) COMPILE: `Result: Succeeded`, 17 actions, 0 errors / 0 WARNINGS.** All ten edited TUs were excluded from the unity blob by adaptive non-unity, so **every changed file compiled standalone**; no CONVENTIONS §7 signature drift. Real link: DLL **2,779,136 → 2,907,136 bytes** (+128 KB), UHT emitted `AncientGround.generated.h` + `SorcererUnit.generated.h`. 🔧 **TWO TRAPS RECORDED FOR THE PIPELINE: (a) `Build.bat` EXITS 0 ON FAILURE — parse the log for `Result: Succeeded`/`Result: Failed`, treat a missing `Result:` line as failure, NEVER trust `$?`.** (b) Attempt 1 died on the **Live-Coding mutex** (4.87 s, zero TUs compiled) — NOT Smart App Control (`VerifiedAndReputablePolicyState=0`, and the signature is a mutex check not `0x800711C7`) and NOT a code error; **Ctrl+Alt+F11 is not a substitute** because UBT invalidated the makefile on `source file added` and Live Coding cannot introduce the two new UCLASSes. **No QA loop was ever opened — the 3-loop budget is untouched.**
    **(2) BOOT-PIE: GREEN.** Match boots and starts (`SiegeGameMode`, player seated Blue, Red bot spawned). **0 Error / 0 Fatal / 0 Ensure / 0 AccessedNone.** `mirror=rot180` ✅ · **`inj=` GONE (grep → 0)** ✅ · `AncientGroundsPass P=(-4743,1860,0) M=(4743,-1860,0)` **exact antipodes** ✅ · **two** `AAncientGround` actors, **both `authoritativeBoost=true`** ✅ · all 3 mine pairs exact antipodes ✅ · `twinSkipped=0` on every layer ✅. Runtime extras: **`bReplicates=false` read live ⇒ Tier C confirmed at RUNTIME**; `ZoneHalfExtent(840,840)` == config (paired tunable agrees, divergence guard correctly silent); `DA_BattlefieldScatter` shows **`SymmetryMode=Rotational180` with `bMirrorSymmetric` gone** — the silent default flip that IS this batch's behavior change. Decal verified **structurally** (material resolved, `bVisible`, no soft-load warning). ⚠️ **NOT a visual pass: I could NOT confirm the jade rune rings render** (capture path does not composite decals; full-arena shots are HISM distance-culled) — **TASK-377 is the pixel gate.** Taller health bars expected/not filed.
    **(3) DETERMINISM: PARTIAL — and it is NOT an RNG defect.** Seed `OverrideSeed=20260801`, three runs. **run 2 == run 3 BYTE-IDENTICAL** ✅; **run 1 (the first PIE after editor boot) differs** ❌ ⇒ a **systematic COLD-vs-WARM split, bistable not chaotic** (run 3 reproducing run 2 exactly rules out a random race). `GenerateScatter` and `AncientGroundsPass` are **identical in all three**; only `MinesPass` differs and only in the **redistribution of per-pair `culls` (`0/4/0` → `0/2/2`, TOTAL = 4 both)**, plus decorative `Grass` (12246→12244, zMismatch 27→28) and `Plants` (zMismatch 6→1). **Every fairness-critical value is stable in ALL runs including cold** — both mine pairs and both ancient grounds at byte-identical exact antipodes, and all five `blocking=true` layers identical with `zMismatch=0`. Mechanism (evidenced): `GroundZAt`'s `ECC_WorldStatic` line trace runs at `BeginPlay` while first-boot static collision is still registering, so a few traces miss → the `0.f` fallback → one diverged placement shifts every later sampled point (run 1 and run 2 mismatch at *different coordinates*, not just different counts). RNG streams provably clean (`mineStream=1283221892` identical everywhere). ⚠️ **OPEN QUESTION FOR THE MANAGER — is the COLD path reproducible cold-to-cold? In a packaged build EVERY boot is cold, so that is the path players get.** I attempted an editor bounce to test it; the close was blocked by the permission classifier and I did not work around it. **Needs a task: two fresh-boot first-PIE runs at one seed, diffed.**
    **(4) NAV SETTLE MEASURED: 216.1 s** (run 2, `LogNavigation=Verbose` from t=0, **326 tiles**, PIE start 01:33:31.949 → last tile 01:37:08.053); run 1 agreed at ~214.2 s. **vs the ~178 s TASK-349 baseline ⇒ +38 s / +21%.** ⚠️ Honest caveat: the 178 s baseline used a *different probe definition* (far-castle team-correct convergence) vs mine (queue fully drained) — same underlying distance-sorted drain, so **indicative, not exact**. Per spec this is a measurement, **not a gate**.
    **(5) NEW FINDING FOR THE MANAGER (no task blocked) — THE ARENA TERRAIN IS NOT 180°-SYMMETRIC.** `zMismatch` fires ONLY on the two `blocking=false` decorative layers, never on the five blocking ones. Confirmed independently of the scatter with two `trace_world` probes at an antipodal pair that mismatched identically in every run: **`(-25731,-623)` → Z 87.5 vs `(25731,623)` → Z 660.1, a 572-unit difference in the authored static terrain.** The scatter is correct — it places the twin at the exact antipode and honestly reports the ground disagreeing. **Perfect symmetry of ground-hugging decoration is capped by level geometry, not by this code** (~0.2 % of instances). Accept-and-document or task a terrain pass — manager's call.
    **STATE:** `HEAD=10f14de`, `origin/main…main = 0`, **nothing committed, nothing pushed**. **`L_Arena` NEVER SAVED** (`git status Content/Maps/` empty); the `OverrideSeed` edit was in-memory only and was **restored to 0**. Working tree = the same 33 entries as at session start. Editor running (PID 23372), MCP up, PIE stopped. ← was: backlog
- blocked-by: TASK-365 **PASS** (cleared)
- parallel-safe: no (EXCLUSIVE editor)
- spec: >
    **(1) COMPILE** with the CLAUDE.md build command. Editor-bounce discipline as usual. Compile failure ⇒ append the errors to
    `qa/TASK-365.md` and route back to gameplay-programmer (**this counts as a QA loop**; max 3, then escalate).
    **(2) BOOT PIE SMOKE (no commit yet — nothing here is a ship gate, TASK-377 is):** confirm the scatter generates; grep the
    `GenerateScatter seed=… mirror=…` token for the NEW mode value; grep the `AncientGroundsPass seed=… P=… M=… fb=… culls=…` line and
    confirm **P and M are exact `(−X,−Y)` antipodes**; confirm **two** `AAncientGround` actors exist; Message Log / ensure / AccessedNone /
    Fatal all 0.
    **(3) DETERMINISM PROOF:** run the SAME `OverrideSeed` twice and diff the two log captures — **byte-identical scatter lines** is the
    pass. Record the seed used.
    **(4) ⚠️ MEASURE NAV SETTLE TIME (flagged item vii):** the boot re-scatter took ~178 s to fully settle at the TASK-349 loop-4
    measurement because tiles process distance-sorted. A symmetry rewrite changes total dirty area — **measure the new number and record
    it; do NOT assume it is unchanged.** A large regression is a finding for the manager, not a silent accept.
    **(5) `L_Arena` IS NEVER SAVED. NO COMMIT IN THIS TASK** (TASK-378 owns the commit). `reset --hard` / `clean -fd` BANNED.
    Report `handoffs/TASK-366-buildmaster.md` + post the compile result, the two log grabs, the diff verdict and the nav number in
    🔧 Build & Git.
- names: >
    Build: `"C:/Program Files/Epic Games/UE_5.8/Engine/Build/BatchFiles/Build.bat" GitClaudeUnrealTestEditor Win64 Development -project="C:/GitProjects/GitHub/GitClaudeUnrealTesting/GitClaudeUnrealTest/GitClaudeUnrealTest.uproject" -waitmutex`.
    Grep tokens: `GenerateScatter seed=`, `[BattlefieldScatter] AncientGroundsPass seed=`. Law: the hard gate + CONVENTIONS §1.

#### TASK-367 — [AG-H1] 🧑 JONATHAN — `WBP_CombatantHealthBar` widget-tree edit (~90 s, PIE stopped)
- assignee: **Jonathan (human — irreducible, manager ruling 9)**
- status: **done** (2026-08-02 — recorded by build-master at TASK-378 from ARTIFACT EVIDENCE, not from a status report: `Content/UI/WBP_CombatantHealthBar.uasset` is modified on disk (the `BarStack` Vertical Box wrap) and it ships in commit B, and Jonathan's TASK-377 ship gate — whose entire boost-bar checklist depends on this wrap — PASSED. ⚠️ **Manager: confirm this reading**; Jonathan never posted a completion for TASK-367 itself.) ← was: backlog
- blocked-by: TASK-366
- parallel-safe: no (EXCLUSIVE editor; nothing else runs during it)
- spec: >
    **WHY A HUMAN: MCP cannot instantiate a UMG widget into a WidgetTree** — no widget toolset exists, verified live against the running
    editor. This is 90 seconds of clicking and it unblocks the whole UI half.
    **RECOVERY IF ANYTHING GOES WRONG:** `git checkout 61a1e72 -- Content/UI/WBP_CombatantHealthBar.uasset`
    **STEPS (copy-paste from plan §7 step 1):**
    1. Open `Content/UI/WBP_CombatantHealthBar`.
    2. Right-click `Bar` → **Wrap With… → Vertical Box**; rename it **`BarStack`**; leave all properties default.
    3. Drag a **Progress Bar** from the Palette onto `BarStack` **ABOVE** `Bar`; rename it **`BoostBar`**.
    4. Right-click `BoostBar` → **Wrap With… → Border**; rename the Border **`BoostOutline`**.
    5. Confirm the Hierarchy reads exactly: `BarStack (Vertical Box)` ▸ `BoostOutline (Border)` ▸ `BoostBar (Progress Bar)`, then
       `Bar (Progress Bar)` ← unchanged.
    6. `BoostOutline`: tick **Is Variable**; **Padding = 1.5** on all four sides; slot **Size = Fill 1.0**, HAlign/VAlign = Fill, Pad Bottom 1.
    7. `BoostBar`: tick **Is Variable**.
    8. `Bar`: slot **Size = Fill 2.0**, HAlign/VAlign = Fill — **change NOTHING else on `Bar`.**
    9. Compile, Save. **Do NOT set any brush/color/opacity and do NOT touch the Event Graph** — the agent does all of that over MCP next.
- names: >
    `/Game/UI/WBP_CombatantHealthBar` → new widgets `BarStack` (VerticalBox), `BoostOutline` (Border), `BoostBar` (ProgressBar); existing
    `Bar` (ProgressBar) untouched except its slot fill. Law: CONVENTIONS §5. Plan §7 step 1.

#### TASK-368 — [AG-A1] MCP: brushes/tints on `BoostBar` + `BoostOutline`, author `SetDamageBoost` as a VERIFIED `K2Node_Event` (art-director)
- assignee: art-director
- status: **done** (2026-08-02 — INTEGRATED by TASK-378, commit B `80c47e8`. Jonathan's PIE ship gate TASK-377 PASSED.) ← was: **ready-for-integration** (2026-08-02) — ✅ **THE `K2Node_Event` GATE IS PASSED WITH A HARD CLASS ASSERTION:** `SetDamageBoost` authored via `add_event` → node `EventGraph.K2Node_Event_3`, and `ObjectTools.get_class` returns **`/Script/BlueprintGraph.K2Node_Event`** — **NOT `K2Node_CustomEvent`** — re-asserted a 2nd time AFTER the compile, plus two independent corroborations (`type_id = AddEvent|Siegebound|UI|EventSetDamageBoost`; the engine's own probe error echoing `(K2Node_Event)`). **`bIsImplemented` was NEVER used as evidence** (TASK-131 round 1 proved it lies). Signature verified pin-by-pin: exactly 5 floats `FillFraction/R/G/B/RowOpacity` in contract order. ✅ **GRAPH matches the contract exactly** — one linear exec chain `SetPercent(BoostBar) → SetFillColorAndOpacity(BoostBar) → SetBrushColor(BoostOutline) → SetRenderOpacity(BoostOutline) → SetRenderOpacity(BoostBar)`, **ONE** `MakeLinearColor` genuinely fanned to both consumers (DSL `bind _linearcolor` proves a single shared node), **ZERO conditionals**. **Alpha literal read back = `1.0`** via `get_pin_value` (trap 4). ✅ **ORPHAN CHECK CLEAN: `find_nodes` 14 → 23, delta exactly +9 = the 9 nodes I created**, all 14 pre-existing nodes present with **identical refPaths**; `OnHPChanged`/`SetTeamColor` DSL **character-for-character identical** to the pre-work read. **I deliberately did NOT use `write_graph_dsl`** (it would have had to re-emit the working events — the exact operation TASK-131 recorded as silently orphaning); used purely additive `create_node`/`connect_pins`/`set_pin_value` instead. ✅ **BRUSHES — NO MATERIAL ANYWHERE** (trap 2, TASK-131's real root cause, closed by construction): `BoostBar` fill = `WhiteSquareTexture`/`Image`/white-identity-tint, track = **`(0.22,0.22,0.24,0.85)`**/`RoundedBox`/resourceObject `None`, `BarFillStyle` `Mask`→**`Scale`**, `LeftToRight`, `Percent 0`, `HitTestInvisible`; `BoostOutline` brush = `WhiteSquareTexture`/`Image`/white, `brushColor` white identity, `HitTestInvisible`. Both white tints are deliberate identity multiplies so the C++-pushed band colour lands undimmed. ✅ **`RenderOpacity = 0` applied LAST on both** (after everything else was verified) — a unit that never receives a push shows health-bar-only, exactly like today. ✅ **`Bar` PROVABLY UNTOUCHED** — full before/after property capture identical on every field (near-black track `(0.03,0.03,0.03,0.7)`, fill, `Scale`/`LeftToRight`, `Percent 1`, `fillColorAndOpacity {0,0.5,1,1}`, `Visible`, opacity 1). Compiled clean twice (zero `LogBlueprint` errors), saved single-asset, **`is_dirty = false`**. PIE stopped throughout; touched ONLY `WBP_CombatantHealthBar`; no save-all, `L_Arena` never opened; no Git, no C++. ⚠️ **RENDERING IS UNVERIFIED AND I DO NOT CLAIM IT** — `CaptureAssetImage` refuses WidgetBlueprints and screen-space Slate is uncapturable headless; every claim above is structural readback, and TASK-131 is the record of readback passing on a visually-broken widget. **TASK-377 IS THE ONLY PIXEL GATE** (7-point eyeball checklist in the handoff §7). ✅ **ONE TREE DEVIATION FOUND, FLAGGED, THEN FIXED ON AUTHORIZATION — TASK-367 step 8 had not landed: `Bar`'s slot was `Fill 1.0`, spec `Fill 2.0`** (`BoostOutline`'s slot was already correct at Fill 1.0 + Pad Bottom 1). I flagged rather than patched (per "touch NOTHING on `Bar`" + "don't silently work around a wrong tree"); **the orchestrator then authorized it on the reasoning that a `VerticalBoxSlot` is a layout property of the CONTAINER, not of `Bar` — the prohibition protects `Bar`'s brushes/colours/fill-style/`Percent`, none of which were touched.** One `set_properties` on `BarStack.VerticalBoxSlot_5` → **readback `{value:2, sizeRule:Fill}`** ✅; the 22 px box now splits **~7 px boost / ~15 px health** as designed instead of 11/11. **`Bar` re-verified byte-identical a THIRD time after the slot edit** (both brushes, `Percent 1`, `Scale`, `LeftToRight`, `{0,0.5,1,1}`, `Visible`, opacity 1); recompiled + re-saved, `is_dirty = false`. ⚠️ **Honest limit: child ORDER is NOT machine-readable** — `UPanelWidget::Slots` is not exposed over MCP, so parentage is proven but "boost row ABOVE health bar" is not; **add it to the TASK-377 checklist.** Both new widgets' **Is Variable CONFIRMED** via `find_node_types` publishing `GetBoostBar`/`GetBoostOutline` (`bIsVariable` itself is unreadable over MCP). Full evidence in `handoffs/TASK-368-artist.md`. ← was: backlog
- blocked-by: TASK-367 — **CLEARED** (Jonathan's widget-tree step landed; hierarchy machine-verified via slot parentage)
- parallel-safe: no (EXCLUSIVE editor)
- spec: >
    **(1) BRUSHES — `WhiteSquareTexture`, `DrawAs = Image`, tint WHITE on BOTH new widgets.** ⚠️ **A *material* fill brush swallowing a
    runtime tint was TASK-131's actual root cause — do NOT reopen that door.** `BoostBar` gets its OWN medium-grey TRACK
    **`(0.22, 0.22, 0.24, 0.85)`**; `Bar` keeps its near-black track **UNTOUCHED**. `BoostBar`'s `fillImage.tintColor` must be neutral white
    (identity multiply) so the pushed band color shows through undimmed.
    **(2) THE EVENT GRAPH — ONE linear exec chain, ONE `MakeLinearColor` fanned to both consumers:**
    `SetPercent(BoostBar, FillFraction)` → `SetFillColorAndOpacity(BoostBar, $C)` → `SetBrushColor(BoostOutline, $C)` →
    `SetRenderOpacity(BoostOutline, RowOpacity)` → `SetRenderOpacity(BoostBar, RowOpacity)`, where `$C = MakeLinearColor(R, G, B, 1.0)`.
    **The alpha literal MUST be 1.0 — READ IT BACK.** **ZERO conditionals in the widget** — all banding is C++ (TASK-362).
    **(3) ⚠️ THE DEFECT THAT HID THE HEALTH-BAR BUG FIVE TIMES: `SetDamageBoost` landing as a `K2Node_CustomEvent` NEVER fires from C++,
    silently.** Author via `add_event`; **assert the node's object CLASS via `get_node_infos`**; **NEVER trust `bIsImplemented`.**
    Report the asserted class string in the handoff.
    **(4) HIDE VIA `SetRenderOpacity(0.0)`, NEVER `SetVisibility`** — a float pin (widget-param law) that keeps the health bar pinned at a
    constant head offset for every unit, boosted or not. Set it on BOTH `BoostOutline` and `BoostBar` (opacity propagates to children —
    belt-and-braces). **Set render opacity LAST** so the design-time state is not what ships.
    **(5) ORPHAN CHECK:** no orphaned/duplicate nodes; no stray pins; the existing `OnHPChanged` / `SetTeamColor` graphs **byte-untouched**.
    **(6) Compile + Save the WidgetBlueprint. NO Git. NO C++.**
    ⚠️ **Do NOT claim the bar renders correctly — you cannot self-verify UMG rendering here** (`CaptureAssetImage` refuses WidgetBlueprints;
    MCP readback has repeatedly passed on visually-broken UMG in this project). **TASK-377 is the pixel gate.** Report structure, not appearance.
    Handoff `handoffs/TASK-368-artist.md`. Post in 🎨 Art.
- names: >
    `/Game/UI/WBP_CombatantHealthBar` → `BoostBar` (ProgressBar), `BoostOutline` (Border), `BarStack` (VerticalBox), `Bar` (existing).
    BIE consumed: `SetDamageBoost(float FillFraction, float R, float G, float B, float RowOpacity)` — the 5-arg signature verbatim from
    `handoffs/TASK-362-programmer.md`. Law: CONVENTIONS §5 + "Widgets with C++ bases". Plan §5.

#### TASK-369 — [AG-A2] Sorcerer concept (Stage 0) — the LEY-WARDEN identity + the two reject gates (art-director)
- assignee: art-director
- status: **done** (2026-08-02 — recorded by build-master at TASK-378 from ARTIFACT EVIDENCE, not from a recorded ruling: the GATE-A escalation was resolved as **option (1) ACCEPT the overshoot + per-asset `proportions` override**, which is the artist's own recommendation — proven by `rig_manifest.json` now carrying the `Sorcerer.proportions` block (QA-reviewed in `qa/TASK-372-tooling-report.md`) and by `Content/RawAssets/Concepts/Sorcerer.png` now EXISTING on disk (the status text below says it was deliberately ABSENT — that clause is STALE). The whole 370→375 chain ran on it and Jonathan's ship gate passed. Concept PNG + `concept_prompts.json` ship in commit B. ⚠️ **Manager: the ruling itself was never written to the board** — please record it properly.) ← was: **DECISION-NEEDED — ✅ 402 CLEARED, 8 images generated; Gate B PASS + TeamRegion ANSWERED; ⛔ Gate A (antlers) blocked by a MODEL-LEVEL constraint conflict needing ONE ruling (2026-08-02).** ✅ **Jonathan's credits work — 8 generations, 8 successes, ZERO 402s** (~5–7 s each; CA triplet used every call, handshakes clean, verification never disabled). ✅ **GATE B (Wizard distinctness): PASS, decisively, on every roll** — no hood, no beard, stone ritual mask, cool jade/teal key vs the Wizard's warm fire; a stranger would never call them the same unit. ✅ **The `TeamRegion` slab-mantle question is ANSWERED ON REAL PIXELS and the selector is VIABLE** — on roll 5 (seed 71022) §6 points (1) distinct rigid plate and (2) genuinely UP-facing flat top both **PASS**, (4) separable **PASS**, (3) is **two large boxy plates (~17 % image width each) rather than one continuous band — acceptable, decimates fine**. Per my own §6 law only (1)/(2) force a Stage-0 reroll, so **no mantle reroll is required and TASK-370's manifest can be written — it should target TWO symmetric islands, not one band.** ⛔ **GATE A splits: the MONOLITH half PASSES** (planted stone top well below the shoulder line on rolls 5/6/8 — the approved shoulder-not-head judgement call worked) **but the ANTLER half FAILS on every roll that renders the mandated antler crown** (overshoot by roll: +143, +115, +23, +65, +88, +117, n/a, 0 px). **Systematic, not seed noise — 8 rolls across 6 prompt formulations**; the only two rolls that cleared it did so by destroying the identity (crown ate the head, antlers gone) or the composition (pillar occluding the torso, fatal for image-to-3D). Verified in code, not assumed: `rig_manifest.json` says `*_z are fractions of measured mesh HEIGHT`, so antlers skew **every** vertical anchor — at 9.2 % inflation `neck_top_z` 0.865 lands at ≈0.94 of true body height. **STOPPED at 8 rolls rather than burn more of Jonathan's newly-purchased credits on a conflict a seed cannot resolve.** ⚠️ **NOTHING ARCHIVED — `Content/RawAssets/Concepts/Sorcerer.png` deliberately ABSENT; no unearned accept recorded.** **RULING NEEDED (one line):** (1) **ACCEPT the overshoot** + per-asset `proportions` override in `rig_manifest.json` (already supported — *"per-asset keys override"*; Ogre `skeleton:"bespoke"` is precedent) — **my recommendation, concept = roll 5 / seed 71022**; (2) **RELAX the identity** (drop/shrink the antler crown — manager call); or (3) **TRIM the antlers in Stage 2 Blender** (TASK-370 scope, leaves texture artifacts). **`MODEL_ID` NOT swapped off FLUX.1-dev and will not be unprompted** — shared style key for all 18 shipped concepts. 📌 **Two reusable findings for CONVENTIONS: (a) on FLUX.1-dev in-prompt negations SUMMON their tokens** — *"not rounded pauldrons"* → got pauldrons, *"absolutely NO branching antler rack"* → got a rack; rolls 3+ rewritten with ZERO negations (asserted in code); **(b) prompt length has a style cliff at ~1.6 k chars** — at 2,007 chars the tail-mounted style block diluted and the roll lost roster style, chunky proportions AND two identity props; keep entries ≲1,600. `concept_prompts.json` `Sorcerer` entry rewritten positive-form (1,573 chars) and **seed pinned to 71022** so `--force` reproduces the recommendation exactly; `Inbox/Sorcerer.png` staged with roll 5. **TASK-370 is blocked on the ruling ONLY — its TeamRegion input is settled.** Full evidence, 8-roll table + contact sheet in `handoffs/TASK-369-artist.md`. ← was: BLOCKED-ESCALATED (HF 402 ×2) — ⛔ **The one Jonathan-authorized free re-run is SPENT and the renewal-propagation hypothesis is DISPROVEN** — ~8 h after the first 402 the same call returned the same account-level stop (`Request ID Root=1-6a6e7f25-…`, exit 3), and a free `whoami` re-probe shows the account **unchanged**: `{isPro: true, canPay: false, periodEnd: 2026-09-01}`. The included budget did NOT reset on the period boundary. **Cost of the retry: ZERO credits** (bare attempt died at TLS before reaching a provider; bundled attempt was rejected before inference). 🔒 **TLS is CONFIRMED FIXED in the field** — the bare run still fails `CERTIFICATE_VERIFY_FAILED` (so the CA triplet is now MANDATORY, not optional, for every HF/Meshy call), but with the rebuilt `Cache/_certs/win-ca-bundle.pem` the handshake is clean and the request reaches the provider. Verification never disabled. **STOPPED ON THE 402 as instructed — no second retry, no workaround, no MODEL_ID/provider swap to dodge a paid quota.** ⚠️ **ONLY ONE PATH REMAINS AND IT IS JONATHAN-ONLY: add pre-paid HF Inference credits** (`canPay: false` = no payment method; no agent can, and waiting no longer helps). A `MODEL_ID` swap off FLUX.1-dev is the sole alternative but needs a manager/Jonathan ruling — it would break style consistency with all 18 shipped concepts. **BOTH REJECT GATES REMAIN UNRUN, reroll count 0** (seeds 71019/71020 untouched), and the **`TeamRegion` slab-mantle question is UNANSWERED — it needs pixels, and answering it from the prompt text was explicitly ruled out**; the exact 4-point test to apply on first image is recorded in the handoff §6. `Inbox/Sorcerer.png`, `Content/RawAssets/Concepts/Sorcerer.png`, `Cache/Sorcerer/` all re-verified ABSENT — no partials. ⚠️ **TASK-370 STILL MUST NOT BE DISPATCHED.** ← was: BLOCKED-ESCALATED (first attempt). Prompt work is DONE and survives: the `Sorcerer` entry is authored and live in `concept_prompts.json` (seed **71018**, LEY-WARDEN identity, both gates pre-defended in the prompt text; `--check` sees 19 entries). The FLUX.1-dev call never reached the model — verbatim provider message: *"You have depleted your monthly included credits. Purchase pre-paid credits to continue using Inference Providers."* (`router.huggingface.co/fal-ai/fal-ai/flux/dev`, exit 3 = the tool's documented quota pause, never retried by design). **BOTH REJECT GATES ARE UNRUN — there is no image to judge; reroll count 0.** `Inbox/Sorcerer.png` and `Content/RawAssets/Concepts/Sorcerer.png` are both ABSENT (no partial files left behind). ~~⏳ TIMING — a plain re-run in a few hours may simply succeed at ZERO cost~~ **← THIS HYPOTHESIS IS DEAD.** It was the whole justification for the authorized retry above; the retry was taken and returned the identical 402, so the period boundary was a red herring. `canPay: false` ⇒ buying pre-paid credits is **Jonathan-only**; no agent can clear it. **🔧 SIDE-FIX THAT UNBLOCKS THE WHOLE HF/MESHY LANE: the committed CA bundle was STALE and failed too** — Norton regenerated its interception root since 2026-07-26, so `Cache/_certs/win-ca-bundle.pem` no longer validated `router.huggingface.co`. Rebuilt from the live Windows trust store (certifi + 115 machine roots incl. the current Norton root); both HF hosts now VERIFY OK and the call reached the provider. **Verification was never disabled.** Old bundle kept as `.bak-20260801`. ⚠️ **TASK-370 MUST NOT BE DISPATCHED** — it consumes the nonexistent `Inbox/Sorcerer.png`. Resume recipe + the escalation ask in `handoffs/TASK-369-artist.md`. ← was: backlog — **dispatchable NOW**
- blocked-by: none (upstream deps clear; the HF-credit block is CLEARED) — **HELD on a one-line GATE-A RULING (accept overshoot + rig `proportions` override / relax the antler identity / trim in Stage 2), not on a pipeline task and not on credits**
- parallel-safe: yes (no editor, no code)
- spec: >
    **(1) AUTHOR the `concept_prompts.json` entry for `Sorcerer` to the CONVENTIONS §3 identity — and it is a REJECTION-DRIVEN spec.**
    ⚠️ The shipped `Wizard` prompt literally reads *"battle-mage fire **sorcerer** … hooded bearded wizard … fireball"*, so a generic
    "sorcerer" prompt returns a **Wizard reskin with ~90% probability.** The identity is the **LEY-WARDEN / GEOMANCER** — a ritualist who
    CONSECRATES ground, not a caster who throws things: carved **stone ritual mask** + antler crown (**no face, no hood, no beard**);
    **BOTH hands on a rune-carved monolith-staff planted in the ground** (no weapon presented — "never attacks" must be legible from the
    POSE); stone-and-moss vestments with a **flat slab mantle** across the shoulders (the best `TeamRegion` target on the roster —
    reuse the Cleric/Wizard `shoulder_caps` selector, **no `helm_dome`**: a mask is not a helmet); cool **jade/teal** ley-light keying to
    the ancient-ground decal; reads at 15 m as **"a standing stone with legs."**
    **(2) RUN** `uv run concept_generate.py Sorcerer` from `Tools/ArtPipeline` (seed 71018). **Norton TLS: run BARE first**; the CA-bundle
    fallback is only for a verbatim cert failure.
    **(3) ⚠️ GATE A — THE HEIGHT GATE, AT THE CONCEPT, BEFORE ANY MESHY CREDITS:** the **monolith top must sit at ~HEAD height, NOT above
    it.** `rig_character.py` fits SiegeBiped anchors as FRACTIONS of measured mesh height, so an overshooting staff slides the whole rig
    down the body. Reject and re-roll at the concept — this is the cheap place to catch it.
    **(4) ⚠️ GATE B — THE WIZARD SIDE-BY-SIDE:** set the Sorcerer preview next to `Content/RawAssets/Concepts/Wizard.png`.
    **If a stranger would call them the same unit, REROLL Stage 0.** Report the comparison honestly — a marginal pass is a fail.
    **(5) COMMIT THE ACCEPTED CONCEPT** to `Content/RawAssets/Concepts/Sorcerer.png` (working input stays at
    `Tools/ArtPipeline/Inbox/Sorcerer.png`, PascalCase = CardID). No approval gate on the concept itself — **post it in 🎨 Art for
    visibility, do not wait.**
    Handoff `handoffs/TASK-369-artist.md` records the prompt text, the seed, and BOTH gate verdicts.
- names: >
    `Tools/ArtPipeline/concept_prompts.json` (new `Sorcerer` entry) · `Tools/ArtPipeline/Inbox/Sorcerer.png` ·
    `Content/RawAssets/Concepts/Sorcerer.png`. Reference (read-only): `Content/RawAssets/Concepts/Wizard.png`.
    Law: CONVENTIONS §3 (art identity) + "Textured mesh law" Stage 0. Plan §6.

#### TASK-370 — [AG-A3] Sorcerer Meshy image-to-3D + Blender refine + the mandatory pre-import gate (art-director)
- ⚠️ **AMENDED 2026-08-02 — STAGE 2 RE-RUN (`handoffs/TASK-375-facing-fix.md`).** `pipeline_manifest.json` `Sorcerer.pre_rotate_z_deg: 0.0 → 180.0` to fix a 180° facing error that only manifests AFTER export (root cause: the TASK-348 `_ue_handedness_precomp` MIRROR-FIX — the Sorcerer is the first UNIT exported after it). Re-run from the cached GLB: **11.7 s, ZERO Meshy credits.** **FULL PRE-IMPORT GATE RE-RUN AND RE-PASSED:** UV-norm albedo **0.4403** (floor 0.2536) · luma retention **0.9811** (band 0.85–1.25) · chroma **0.9292** · dominant hue shift **2.55°** · 15,000 tris · `UVMap` · feet-centre · slots `[TeamRegion, SorcererPBR]` · TeamRegion **3.12%, two islands, 0 faces at |x|<5, no head paint** · report warnings `[]`. `albedo_delight` still PINNED to the locked fleet values; mask at the corrected `neighbor_tol=0.010` (concept fg fraction reproduces **0.3377 exactly**). Antler factor **1.0545 unchanged**. ⚠️ Conformed space now fronts **+Y**, so `preview_back.png` shows the character's FRONT — that inversion IS the compensation, do NOT reset `pre_rotate_z_deg` to 0.
- assignee: art-director
- status: **done** (2026-08-02 — INTEGRATED by TASK-378, commit B `80c47e8`. Jonathan's PIE ship gate TASK-377 PASSED.) ← was: **ready-for-integration** (2026-08-01) — ✅ **PRE-IMPORT GATE PASSED ON EVERY CRITERION, none marginal.** Stage 0 housekeeping → Stage 1 Meshy → Stage 2 headless refine, all clean. ✅ **CONCEPT ARCHIVED FIRST** as instructed: roll 5 / seed 71022 → `Content/RawAssets/Concepts/Sorcerer.png`, **byte-identical** to `Inbox/Sorcerer.png` (sha256 `bc268ccb7449…a0ea`, 612,298 B); Meshy's `state.json` independently records the same input sha, so the mesh provably derives from the archived concept. ✅ **TLS: `--check` ran BARE and PASSED — `api.meshy.ai` does NOT need the CA triplet** (unlike `router.huggingface.co`, which still does); verification never disabled, `MESHY_TOKEN` env-only/redacted, **ZERO 402s**. ✅ **Stage 1: task `019fc042-8cc4-7d1c-9b8f-28feb63f8f16`, 30 CREDITS (2416 → 2386), 3 m 48 s**, Wizard-identical params. ✅ **Stage 2: 10.8 s** — CONFORM 91.4×78.7×182.0 `dims_within_tolerance=True` **on the first run**, remesh 71,540 → **15,000 tris**, 1024² bakes, **slots `[TeamRegion, SorcererPBR]`**. 🔬 **GATE NUMBERS: UV-normalised albedo 0.4377 (floor 0.2536 — 73% headroom) · luma retention 0.9983 vs the ALPHA-MASKED concept (band 0.85–1.25, target ≈1.0 — 0.17% off target; per-view 1.1185/0.9976/0.8788, all in band) · anti-bleach guard NOT raised · chroma retention 0.8596 (Cleric ref 0.28×) · dominant-cluster hue shift 0.75° · UVMap present · min_z 0.059 feet-center, XY centred · ORM AO 0.4773 / metallic 0.0035.** ✅ **TeamRegion TWO SYMMETRIC ISLANDS asserted ON THE EXPORTED FBX (not on intent):** left 168 faces `x −38.50..−9.37` / right 185 faces `x 8.63..40.09`, mirror ratio **0.9798**, **ZERO faces within |x|<5** (neck + scarf knot + jaw spared), **97.73% of faces up-facing at dot≥0.55**, **ZERO head/antler paint** (region tops out at z 157.87, ~15 UE BELOW the skull apex 172.42 — the Archer bare-HEAD lesson holds), area **3.08%** (fleet band 2.1–4.1%). No `helm_dome`, `max_fraction` 0.35, **selectors needed NO gate tune**. 📐 **THE TASK-372 DELIVERABLE — ANTLER `INFLATION FACTOR = 1.0545`, MEASURED ON THE CONFORMED MESH:** total height 181.805 UE, skull dome apex 172.42 UE (front/back agree to 0.3%), true body height 172.42 UE, overshoot 9.39 UE = **5.45% of body**, skull top at z-fraction 0.9484. **`neck_top_z` 0.865 uncorrected lands at 0.9121 of true body height → corrected 0.8203; general rule `corrected_z = nominal_z / 1.0545`.** ⚠️ **THE CONCEPT-STAGE ESTIMATE OVERSTATED IT — TASK-369's ~9.2% (predicting ≈0.94) is SUPERSEDED; TASK-372 must divide by 1.0545, NOT ~1.09.** Two rejected methods recorded (vertex Z-slice = sampling noise at 7,496 verts → 21.78%; silhouette width = fooled by converging tines → 7.76%); the authoritative method is colour segmentation (skull green vs antler tan) on the self-calibrating ortho previews, eyeball-confirmed. 🔬 **METHOD DEFECT FOUND AND CORRECTED BEFORE IT COULD CORRUPT A NUMBER:** the shipped TASK-342 `derived_alpha_mask` default `neighbor_tol=0.03` **LEAKS THROUGH the left granite plate** and flood-fills it as background (plate coverage 0.100), deleting a large light region from the retention DENOMINATOR — it read the concept **0.9242×, i.e. 7.6% too DARK, inflating every retention number ~8%**. Used `tol=0.010` (tightest clean value; its fg fraction 0.3377 agrees with the INDEPENDENT legacy corner mask 0.3383 to **0.06%** — cross-method validation), mask eyeballed; `measure_fidelity.py` left **byte-untouched** so its fleet-anchor validation stays meaningful. Harness re-validated first: **6/6 albedo anchors to 4 dp**, Ogre retention anchors to 0.04%. ⚠️ **FLAG 1 — the brief's "(omit the block)" mechanism is FACTUALLY WRONG and I measured it rather than argued it:** omitting yields the script's older CONSERVATIVE in-script default `{0.6,0.35,0.85,1.0}`, NOT the locked fleet profile; CONVENTIONS calls the locked profile "the fleet-wide DEFAULT" but **the script does not implement it as the default**. Counterfactual run in the `--smoke` sandbox: **omit-the-block → UV-norm 0.1801 = FLOOR FAIL (29% under); locked profile pinned → 0.4377 PASS.** Followed the NAMED INTENT; values are the locked fleet numbers **verbatim — NOT a per-asset tune, no `albedo_delight` override needed**. **Tooling/manager follow-up: make `ALBEDO_DELIGHT_DEFAULTS` the locked profile OR amend the CONVENTIONS wording — doc and script currently disagree.** ⚠️ **FLAG 2 — spec path:** the Stage-2 static FBX is script-hardcoded to `Content/RawAssets/Sorcerer.fbx` (where all 24 shipped assets live); `Content/RawAssets/Characters/Sorcerer.fbx` in `names`/CONVENTIONS:281 is the **RIGGED** FBX from `rig_character.py` (TASK-372). Not a defect; **NOT "fixed" by moving the file.** ⚠️ **FLAG 3 (reported, not escalated):** 2nd hue cluster (darkest, L≈24) shifted 23.1° with chroma 9.4→5.63 — the de-light lifting shadow greens; the ≤20° criterion binds concept-fidelity REWORK tasks, not ordinary builds, and every gating metric passes. First place to look if the Sorcerer ever reads flat in shadow at playtest. ✅ **Gate A monolith half RE-CHECKED post-refine: monolith top ≈88.5 UE vs shoulder line 157.9 UE — far below, comfortable PASS.** ✅ **Gate B re-checked on the 3D result: no hood/beard/face/fireball, cool jade key, wholly different silhouette — decisively not a Wizard.** 📝 Honest note: the flat-lit previews *look* pale and my first read was "bleached" — **that read was wrong** (flat-lit albedo carries no shading); the Cycles beauty render sits on the concept palette and the metric agrees at 0.9983. **I did not tune on the wrong impression.** Manifest `Sorcerer` entry authored + **retuned to measured dims (Wizard `_tuned` precedent)**, `team_region` promoted `_pending` → `_verified`; 22 assets, JSON valid, CRLF preserved, 0 bare LF. **NO UE import (TASK-371), NO rigging (TASK-372), NO editor, NO Git.** Full evidence in `handoffs/TASK-370-artist.md`. ← was: backlog
- blocked-by: TASK-369 — **CLEARED** (Jonathan's 2026-08-01 Gate-A ruling: KEEP THE ANTLERS + rig `proportions` override; accepted source roll 5 / seed 71022; TeamRegion = two symmetric islands, settled)
- parallel-safe: yes (headless — no editor)
- spec: >
    **(1) `uv run meshy_generate.py --check` FIRST — it is FREE and it is the standing rule.** Quota/credit failure ⇒ 🚨 Blockers, do not retry blind.
    **(2) `uv run meshy_generate.py --mode image3d Sorcerer`** (~30 credits).
    **(3) `blender --background … refine_trellis_glb.py -- --card-id Sorcerer`** (free, re-runnable). Heavy Blender runs HEADLESS via
    `blender.exe --background --python` through Bash — **the live Blender MCP bridge has a 30 s socket cap and is for <30 s inspection only.**
    **(4) ⚠️ MANDATORY PRE-IMPORT GATE — NOTHING ENTERS `/Game/` UNSEEN.** Eyeball the refined result: the monolith top at ~head height
    (re-check — the refine can change proportions), the mask/antler/slab-mantle silhouette intact, the jade key present, and **it still
    does not read as the Wizard.** A fail here re-rolls at TASK-369, it does not proceed.
    **(5) TARGETS:** ≤15k tris, feet-center origin, `UVMap`, Nanite OFF, two material slots in order **[`TeamRegion`, `SorcererPBR`]**,
    ≤4 simple hulls, 1024² bakes. Export the FBX to `Content/RawAssets/Characters/Sorcerer.fbx` (raw-asset rule — the FBX is checked in
    alongside the .uasset).
    **NO UE import in this task** (TASK-371 owns the editor). Handoff `handoffs/TASK-370-artist.md` with the credit spend, the gate
    verdict and the measured tri/hull/slot numbers. Post in 🎨 Art.
- names: >
    `Tools/ArtPipeline/Cache/Sorcerer/` (gitignored) · `Content/RawAssets/Characters/Sorcerer.fbx` ·
    `Content/RawAssets/Textures/Sorcerer/` (PNG sources). Slots `[TeamRegion, SorcererPBR]`. Law: CONVENTIONS "Textured mesh law" +
    "Meshy second engine" + §3 asset set. Plan §6.

#### TASK-371 — [AG-A4] UE import: `SM_Sorcerer` + `T_Sorcerer_{D,N,ORM}` + `MI_Sorcerer_PBR` (art-director)
- ⚠️ **AMENDED 2026-08-02 — ASSETS RE-IMPORTED IN PLACE (`handoffs/TASK-375-facing-fix.md`).** §6's facing finding was CORRECT as a *relative* measurement; the absolute cause is the TASK-348 MIRROR-FIX, not the asset's authored rotation. `SM_Sorcerer` + `T_Sorcerer_{D,N,ORM}` were **reimported over the same paths via a headless commandlet** (MCP `import_file` refuses existing paths) — **never delete+recreate**; `SM_Sorcerer`'s referencer `[BP_Unit_Sorcerer]` is identical before AND after. Textures imported **explicitly**, so §7a's texture-skip trap was avoided. Nanite OFF · `LargeProp` · 4 LODs · 4 convex hulls · slots + MIs verified. §8's numbers superseded by the re-measured gate (UV-norm **0.4403**, luma **0.9811**, chroma **0.9292**). `MI_Sorcerer_PBR` NOT modified.
- assignee: art-director
- status: **done** (2026-08-02 — INTEGRATED by TASK-378, commit B `80c47e8`. Jonathan's PIE ship gate TASK-377 PASSED.) ← was: **ready-for-integration** (2026-08-02) — ✅ **ALL FIVE ASSETS IMPORTED, CONFIGURED, HARD-READBACK-VERIFIED AND SAVED** (`is_dirty=false` on each; saved by explicit path list — never save-all, `L_Arena` never opened). 🔴 **THE HEADLINE IS A TASK-375 FINDING, NOT THE IMPORT: THE FLEET FACING CONVENTION DOES NOT HOLD FOR THE SORCERER — MEASURED, NOT INFERRED.** `CaptureAssetImage` uses the SAME fixed default camera for every StaticMesh, so I ran a controlled 4-way A/B: **`SM_Footman` FRONT** (face/spear/shield) · **`SM_Cleric` FRONT** (hooded face/staff/tome) · **`SM_Wizard` FRONT** (face/fireball) · **`SM_Sorcerer` 🔴 BACK** (back of skull, antlers from behind, NO mask, monolith HIDDEN, sash down the back). **3 of 3 shipped units face front; the Sorcerer is the ONLY one rotated ~180° about Z relative to the entire fleet.** ⚠️ **The brief's premise that "+Y front is consistent with the fleet convention, so yaw −90 should be right" is NOT SUPPORTED** — whatever yaw makes the Cleric/Footman/Wizard face correctly leaves the Sorcerer BACKWARDS. **TASK-375 should treat +90 as the LEADING candidate and −90 as SUSPECT, and must still confirm in the viewport.** Independently corroborates TASK-373's back-of-skull card render (same root cause, different lens). Honest scope: this proves the RELATIVE rotation vs the fleet; it does NOT independently establish the absolute world axis, and I do not claim "+Y" as measured. 📌 Related: **`SM_Wizard`'s MESH renders front-facing**, so the Wizard's historical facing bug lived at the Blueprint yaw, not in its FBX — the Sorcerer's is the opposite and lives in the mesh. ✅ **VISUAL GATE ACTUALLY RUN** (unlike TASK-368's WBP — `CaptureAssetImage` DOES support StaticMeshes): textures live and on-palette (jade/teal robe, cream cowl, tan antlers — **not grey/white/checkerboard**, so all three MI params genuinely resolve through `M_AssetPBR`); **`TeamRegion` renders BLUE ON THE TWO SHOULDER PLATES**, visually confirming both slot-0 assignment and that TASK-370's two-symmetric-island claim survived into UE; **ZERO team paint on head or antlers** (Archer bare-HEAD lesson holds); identity intact. 🔬 **MESH READBACK: 15,000 tris (exactly at budget) · Nanite `false` · `lODGroup LargeProp` · `lod_count` 4, thresholds `[2.0, 0.3127, 0.1690, 0.0992]` strictly descending · slots `["TeamRegion","SorcererPBR"]` IN ORDER (read off the imported asset BEFORE assigning — verified, not assumed; FBX matched 1:1, zero renaming) · slot 0 ← `MI_TeamColor_Blue`, slot 1 ← `MI_Sorcerer_PBR` · collision exactly 4 `convexElems` @16 verts, 0 sphere/box/sphyl/capsule, tiling base `−1.3…24` / legs `10…95` / torso `87…123` / head+antlers `117…183` · bounds min Z 0.0586 feet-centre.** **Bounds cross-checked against TASK-370's independent Blender measurement: X and Z agree to 3 dp; Y is MIRRORED (−39.21/+39.16 ↔ −39.157/+39.212) = expected FBX handedness conversion, noted alongside the facing finding.** 🔬 **TEXTURES: `_D` sRGB **ON** / `TC_Default` / `TEXTUREGROUP_World` · `_N` sRGB off / `TC_Normalmap` / `WorldNormalMap` · `_ORM` sRGB **OFF** / `TC_Masks` / `World`; all 1024².** Settings were **read off the shipped `T_Wizard_*` first and matched exactly**, not invented; every value quoted is a POST-SET readback. Textures imported EXPLICITLY, one call each (texture-skip trap). ✅ **`MI_Sorcerer_PBR` parent `/Game/Materials/M_AssetPBR`; `BaseColor`/`Normal`/`ORM` all readback-confirmed.** `list_parameters(M_AssetPBR)` = exactly those three, so no 4th param left unset and no misnamed one silently ignored. ✅ **FIRST IMPORT CONFIRMED BY PRE-FLIGHT** (`find_assets` returned `[]` before starting) — no same-path overwrite to preserve. ✅ **ZERO JUNK ASSETS:** FBX imported `import_materials=false, import_textures=false`, so the importer created no Material/Texture strays despite the FBX naming slots after non-existent materials; `find_assets("/Game","Sorcerer")` returns **exactly 5**, `get_dependencies(SM_Sorcerer)` = `[MI_TeamColor_Blue, MI_Sorcerer_PBR]` only. ⚠️ **FLAG A — the `reimport_meshes.py` texture-skip trap does NOT apply today but IS NOW ARMED:** `_ensure_textures_and_mi` early-returns when the MI exists; `MI_Sorcerer_PBR` now exists, so **any future Sorcerer re-bake will SILENTLY skip re-importing `T_Sorcerer_{D,N,ORM}` and reuse stale bakes — re-import the three textures explicitly (or delete the MI first).** ⚠️ **FLAG B — `SM_Wizard` is a fleet OUTLIER and I followed the fleet, not it:** Cleric/Ogre/Longbowman/MilitiaMob are all `LargeProp` (Cleric `lod_count` 4) but **`SM_Wizard` is `lODGroup None` with `lod_count` 1 — no classic-LOD chain at all**, the same M7.6 gap class that produced its facing follow-up. Checked before applying the board's `LargeProp` precisely because the most obvious reference asset disagreed. **Not fixed here (shipped asset, outside scope) — cheap perf/consistency defect needing a manager decision.** ⚠️ **FLAG C (carried from TASK-370 §7b, unchanged):** `SM_Sorcerer` came from `Content/RawAssets/Sorcerer.fbx` (static); `Content/RawAssets/Characters/Sorcerer.fbx` is the RIGGED FBX = TASK-372's input, a DIFFERENT file — do not reconcile them. **NO skeletal mesh, NO anims, NO card art, NO `ABP_Sorcerer`, nothing placed in any level, no Blueprint edited, no Git, no C++.** Concept-fidelity numbers quoted forward from TASK-370 (same FBX): UV-norm 0.4377 · luma 0.9983 · chroma 0.8596 · hue 0.75°. Full evidence in `handoffs/TASK-371-artist.md`. ← was: backlog
- blocked-by: TASK-370 — **CLEARED**
- parallel-safe: no (EXCLUSIVE editor)
- spec: >
    Import the static path per the "Textured mesh law". **This is a FRESH asset (no same-path overwrite trap applies — there is no prior
    `SM_Sorcerer`), but every OTHER clause of the law does apply.**
    **(1)** `SM_Sorcerer` at `/Game/Meshes/SM_Sorcerer` — **the placement-ghost string contract resolves this path**, so the name is law.
    Nanite OFF · `UVMap` · feet-center origin · ≤15k tris · `lod_group='LargeProp'` per the M7.6 classic-LOD law.
    **(2)** Textures **imported EXPLICITLY** (the texture-skip trap): `T_Sorcerer_D` (sRGB ON), `T_Sorcerer_N` (normal),
    `T_Sorcerer_ORM` (**LINEAR — sRGB OFF**), all at `/Game/Textures/`. **Run the SAMPLER-TYPE sweep after the import** (standing lane law).
    **(3)** `MI_Sorcerer_PBR` at `/Game/Materials/Instances/`, an instance of `/Game/Materials/M_AssetPBR`, params `BaseColor` / `Normal` / `ORM`.
    **(4)** Slot assignment: slot 0 **`TeamRegion`** ← `MI_TeamColor_Blue` (design-time placeholder — the BeginPlay team recolor overwrites
    it at runtime); slot 1 **`SorcererPBR`** ← `MI_Sorcerer_PBR`.
    **(5) HARD READBACK, not vibes:** report tri count, LOD chain, Nanite flag, slot names in order, texture sRGB flags, and the M7.5
    chroma/luma retention numbers vs the accepted concept.
    **NO Git.** Handoff `handoffs/TASK-371-artist.md`. Post in 🎨 Art.
- names: >
    `/Game/Meshes/SM_Sorcerer` · `/Game/Textures/T_Sorcerer_{D,N,ORM}` · `/Game/Materials/Instances/MI_Sorcerer_PBR` (from
    `/Game/Materials/M_AssetPBR`) · slots `[TeamRegion, SorcererPBR]`. Law: CONVENTIONS "Textured mesh law" + §3 asset set.

#### TASK-372 — [AG-A5] Rig + anims: `SK_Sorcerer` on the SHARED skeleton + `A_Sorcerer_{Idle,Walk,Attack,Death}` + LODs (art-director)
- ⚠️ **AMENDED 2026-08-02 — RIG RE-RUN, AND A DEEPER DEFECT FOUND (`handoffs/TASK-375-facing-fix.md`).** **`rig_character.py` hard-codes "Front is -Y" (line 383) and drives every forward motion toward −Y; the OLD Sorcerer FBX fronted +Y in that space, so the shipped rig had its toe offset, leg-swing direction, Attack lunge and `_l`/`_r` bone sides ALL INVERTED relative to the mesh.** That was never a cosmetic yaw problem and no Blueprint/C++ yaw fix would have caught it. The source fix corrects it for free. Stage 3b re-run: exit 0, 40.7 s, 21 bones, `auto_heat` **0.00% unweighted**, zero warnings; `proportions` override **unchanged**, all 16 keys active (`head_top_z` → **172.48 UE** vs skull apex ~172.42 = 0.03% shift). `SK_Sorcerer` + all four `A_Sorcerer_*` reimported in place on the **EXISTING** `SK_Footman_Skeleton` (no new skeleton), root motion OFF, `bForceRootLock` ON. `rig_manifest.json` / `rig_character.py` **byte-untouched**. 🔴 **§14's `lod_count == 1` BLOCKER — ✅ NOW CLOSED 2026-08-02 by build-master (TASK-376 session), `lod_count == 3`; see the ✅ line below.** *(Historical: it was unchanged by the facing-fix pass — 1 before, 1 after, no regression — and deliberately NOT closed there even though its commandlets could have, because it is this task's acceptance criterion and had been escalated for Jonathan's decision. It was ultimately closed WITHOUT needing that decision: the headless commandlet lane required neither the denied `bRemoteExecution` flip nor any manual editor step.)*
- assignee: art-director
- ✅ **LOD BLOCKER CLOSED 2026-08-02 by build-master (TASK-376 session) — `lod_count` 1 → 3, ALL ACCEPTANCE CRITERIA NOW MET.**
  **The denied `bRemoteExecution` flip was NOT used and NOT routed around** (no plugin config edited on disk); the work ran on the
  project's OTHER sanctioned lane — the **headless `-run=pythonscript` commandlet** with the editor closed (`Tools/reimport_meshes.py`
  is the shipped precedent, and TASK-289 names the commandlet as the explicit alternative to remote-exec).
  🔬 **ACCEPTANCE `lod_count == 3` PROVEN FROM A FRESH EDITOR PROCESS after a full close/reopen — verts `15572 / 9845 / 5482`**
  (vertex fractions 0.632/0.352 against the 0.5/0.2 **triangle** targets — vertex fractions legitimately run higher, and this tracks
  the `SK_Ogre` precedent 0.62/0.34 from TASK-341). Recipe exactly as staged: transient `SkeletalMeshLODSettings` ×3
  (LOD1 **50%@0.4** / LOD2 **20%@0.15**, `num_of_triangles_percentage` + `SMOT_/SMTC_NUM_OF_TRIANGLES`) → `regenerate_lod(sk,3)` →
  **`lod_settings` restored to `None` and ASSERTED `None`** → save. **NO companion `SkeletalMeshLODSettings` asset created.**
  ✅ **FLEET NORM HOLDS: all 13 units (12 shipped + Sorcerer) measure `lod_count == 3`** — TASK-289's "SK fleet is LOD0-only" claim
  is confirmed **STALE** once more, measured live both before and after.
  ✅ **`SK_Sorcerer` INTEGRITY UNCHANGED:** skeleton still `SK_Footman_Skeleton` (no new skeleton), **22 bones**, slots
  `["TeamRegion","SorcererPBR"]`, deps exactly `[MI_TeamColor_Blue, MI_Sorcerer_PBR, SK_Footman_Skeleton]`, `is_dirty=false`,
  `find_assets("/Game","Sorcerer")` = **exactly 12**, zero strays. **URO untouched** (no home on `USkeletalMesh`; set in the
  `ASummonedUnit` constructor).
  ⚠️ **EXIT-CODE LAW APPLIED:** the commandlet returned **raw exit 0**, which proves nothing — the verdict came from the log
  (`[SKLOD] RESULT: Succeeded` + `LogInit: Display: Success - 0 error(s), 2 warning(s)`, and **0** hits for `LogPython: Error` /
  `Ensure condition failed` / `Fatal error`). 🧹 **PRE-CLOSE HYGIENE: full project sweep of 3,160 probed assets → `dirty_count == 0`.**
  `MI_Sorcerer_PBR` and `L_Arena` were both confirmed **present in the probe set** (not silently absent) and **both clean** — so
  `MI_Sorcerer_PBR` needed no discard, **`L_Arena` was never opened or saved**, and because nothing was dirty the graceful close
  (WM_CLOSE, **not** a force-kill) raised **no save prompt at all** — the `SK_Footman_Skeleton` "never Don't Save" hazard never arose.
  **NO Git, NO commit, NO push.** Evidence: `handoffs/TASK-376-buildmaster.md`. ← the item below is now HISTORICAL
- status: **done** (2026-08-02 — INTEGRATED by TASK-378, commit B `80c47e8`. Jonathan's PIE ship gate TASK-377 PASSED.) ← was: Tooling half (`rig_character.py` + `rig_manifest.json`) ships in commit A under `qa/TASK-372-tooling-report.md` PASS. **ready-for-integration** (2026-08-02; LOD item CLOSED 2026-08-02 — see the line above. Historical text follows.) — 🔵 **UE IMPORT HALF NOW COMPLETE** (editor released by Jonathan; PIE confirmed stopped + no asset editors open before touching anything; **`L_Arena` never opened or saved, no save-all — every save an explicit path list, no Git, no gameplay code, no `ABP_Sorcerer`, editor never closed**). ✅ **`SK_Sorcerer` at `/Game/Characters/SK_Sorcerer` BOUND TO THE EXISTING `/Game/Characters/SK_Footman_Skeleton`** — the import returned **exactly ONE asset**, and a **project-wide Skeleton scan confirms `SK_Footman_Skeleton` is STILL THE ONLY Siegebound skeleton** (no `SK_Sorcerer_Skeleton` anywhere); nothing orphaned. **22 bones** (21 rig + the `Footman_Rig` object node), bone list **character-for-character identical to `SK_Footman`**. ✅ **SLOTS READ OFF THE ASSET BEFORE ASSIGNING (verified, not assumed):** `["TeamRegion","SorcererPBR"]` → slot 0 `MI_TeamColor_Blue`, slot 1 `MI_Sorcerer_PBR`, both confirmed by readback; 2 sections; deps exactly `[SK_Footman_Skeleton, MI_TeamColor_Blue, MI_Sorcerer_PBR]`, no dangling refs. Bounds `(45.49, 39.18, 90.90)` extent, origin z 90.96 → **min z ≈ 0.06, feet-at-origin — matches the Blender export exactly**. ✅ **ALL FOUR `A_Sorcerer_{Idle,Walk,Attack,Death}` at `/Game/Characters/Anims/`, all on the SHARED skeleton, root motion OFF + `bForceRootLock` ON.** The TASK-165 recipe reproduced exactly (each anim import emits a spurious `SkeletalMesh` at the target name + the real `AnimSequence` at `<name>_Anim`); the spurious meshes were deleted **guarded on `get_asset_class == "SkeletalMesh"`** and the sequences renamed into place **guarded on `== "AnimSequence"`** — never a blind delete; **zero strays left** (exactly 5 Sorcerer assets under `/Game/Characters`), all 5 saved (`is_dirty=false`). 🔴 **THE ONE OUTSTANDING ITEM — LOD CHAIN NOT APPLIED, AND NOT FAKED: `lod_count == 1`, acceptance is 3.** ⚠️ **The blocker is NOT "MCP can't reach it" as the spec anticipated — I checked, and the framing changed twice.** First: **TASK-289's "SK fleet is LOD0-only" is STALE** — measured live, **all 12 shipped units report `lod_count == 3`** (TASK-297 applied them), so `SK_Sorcerer` is the **ONLY** unit without a chain — a real gap, NOT a fleet norm. Second: a **proven method exists that needs NO editor close** (TASK-297's UE Python Remote Execution lane), but it requires flipping `bRemoteExecution` on `/Script/PythonScriptPlugin.Default__PythonScriptPluginSettings` — **and that flip was DENIED BY THE PERMISSION SYSTEM.** I did **not** route around it (e.g. by editing the plugin config on disk) — that would circumvent the intent of the denial. **JONATHAN'S DECISION NEEDED.** Everything else is staged: the recipe is already on disk at `Content/RawAssets/Characters/Sorcerer.lod.json`; remaining = enable remote-exec (or apply manually / headless commandlet) → transient `SkeletalMeshLODSettings` ×3 (LOD1 **50%@0.4** / LOD2 **20%@0.15**) → `regenerate_lod(sk, 3)` → restore `lod_settings` to None → readback + save. ⚠️ **Use `num_of_triangles_percentage`, NOT `number_of_triangles_percentage`** (does not exist, errors — TASK-297's correction to the TASK-288 recipe), with `SMOT_NUM_OF_TRIANGLES` + `SMTC_NUM_OF_TRIANGLES`. **URO needs NOTHING** — those flags have no home on `USkeletalMesh` and are already set in the `ASummonedUnit` constructor (TASK-285/297). 🔵 **FACING CARRY-FORWARD CORROBORATED FROM THE SKELETAL SIDE — `SK_Sorcerer` renders BACK-facing, confirming TASK-371's static-side finding.** Not judged by impression: a thumbnail alone is weak evidence for a unit whose concept has a *faceless* stone mask, so I rendered the **exported rigged FBX** from known −Y/+Y with real albedo and matched cue-by-cue — the UE thumbnail has **no red central sash** (present only in FRONT), **has the tan diagonal strap** (present only in BACK), and shows the ribbed cowl mass with no mask → **matches BACK**. Control: `SK_Footman` in the same thumbnail camera is unambiguously front-facing. Expected rather than surprising: `rig_character.py` exports with an axis contract *deliberately identical* to the static pipeline. **Scope limit kept narrow (TASK-371's caution): this establishes rotation RELATIVE TO THE FLEET, not an absolute world axis. TASK-375 → treat +90 as the LEADING candidate and −90 as SUSPECT — now backed by TWO independent readings.** ⚠️ **THREE FINDINGS REPORTED, NOT ACTED ON:** (a) **the clips import at 24 fps, not the authored 30** (Idle 2.0 s → 2.5 s, ~20% slow) — cause found in my own tooling: `rig_character.py::main` exports the anim FBXs **before** `render_previews` sets `scene.render.fps`, so the manifest `fps: 30` never reaches them; **fleet-consistent with the unit it should match** (`A_Wizard_*` is also 24 fps — same rig lane; Footman/Cleric are 30 fps only because they came via the **Meshy retarget** lane, Footman Idle being 120 f/4 s, not this script's 60-frame Idle), so **nothing new was introduced** and "fixing" it would desync the Sorcerer from the Wizard + force a re-export/re-import → **tooling follow-up**; (b) **`A_Wizard_*` shipped with `bForceRootLock=false`** while Footman/Cleric are `true` — the Sorcerer was set `true` per instruction, flagging the **Wizard as the odd one out** and a candidate for the same follow-up; (c) **`import_file` exposes NO normals-method parameter**, so "Import Normals (not compute)" could not be set explicitly — the FBX carries face smoothing and this is the identical call shape used for all 12 shipped units, so `SK_Sorcerer` matches the fleet (stated plainly rather than claiming a setting I could not control). Full evidence in `handoffs/TASK-372-artist.md` PART 2. ← was: headless-complete, import-pending
- status-history: **headless-complete, import-pending** (2026-08-01) — ⚠️ **HALF THE TASK WAS DONE. Stage 3b (headless Blender) COMPLETE; the UE-editor half NOT STARTED, owing `spec` items (2)(3)(4)(6).** Jonathan was hand-editing a widget, so the editor was OFF LIMITS and the orchestrator explicitly scoped this pass to headless only: **no `SK_Sorcerer` import, no anim imports, no LOD regeneration, no `ABP_Sorcerer` (never author one), no Git, editor never opened.** ✅ **RIG RAN CLEAN: exit 0 in 40.5 s — 21 bones (20 deform), armature root `Footman_Rig` (the shared-skeleton constant), skinning `auto_heat` with 0.00% unweighted (BETTER than the Wizard, which fell back to envelope), 15,000 tris / 7,496 verts, `UVMap`, slots `[TeamRegion, SorcererPBR]`, ZERO warnings.** ✅ **ALL FOUR CLIPS AUTHORED** — `Idle` 60f / `Walk` 30f / `Attack` 40f / `Death` 48f @ 30 fps. **The Attack clip is produced ON PURPOSE for a unit that never attacks** (Cleric precedent: free, and it is the ready-made hook for a consecration gesture — "Attack" is a SLOT NAME, not a semantic claim). 📐 **JONATHAN'S ANTLER RULING IMPLEMENTED WITH THE MEASURED FACTOR: per-asset `proportions` override in `rig_manifest.json`, `corrected = nominal / 1.0545`** (TASK-370's mesh measurement — **TASK-369's concept-stage ~9.2% was correctly IGNORED**; dividing by ~1.09 would have put `head_top` ~14 UE BELOW the skull, inside the cowl). 15 anchors + `foot_forward_frac` corrected; the six `*_x_frac` keys deliberately NOT (they scale measured HALF-WIDTHS, which antlers do not inflate). ⚠️ **THE RULING COULD NOT BE DONE BY EDITING THE MANIFEST ALONE — TWO `rig_character.py` FIXES WERE REQUIRED (read this before reviewing anything else):** (1) `merged_params()` had **NO per-asset `proportions` support at all** — an asset-level block would have been **SILENTLY IGNORED**, shipping an uncorrected rig while the manifest claimed otherwise (the worst possible outcome); now overlaid onto a COPY with unknown keys hard-failing exit 2. (2) The head anchor was a **bare hard-coded `0.905` literal** in two places — un-overridable, so the head bone would have stayed stranded at antler-inflated height while all 14 other anchors moved; now the key `head_base_z` with the SAME 0.905 default. ✅ **PROVEN A BIT-EXACT NO-OP FOR THE 12 SHIPPED RIGS — not argued, RUN:** a `--smoke` Footman re-rig compared bone-for-bone against the shipped `Characters/Footman.fbx` gives **max head delta 0.0 UE across all 21 bones**, names identical; `Cache/Footman/rig/` was backed up before and restored byte-for-byte after (TASK-370 `shipped_locked` pattern). ⚠️ **THE STOCK PREVIEWS RENDER THE MESH ONLY — ARMATURES DO NOT RENDER IN BLENDER, so no shipped preview contains a single bone and eyeballing them would have proven NOTHING about anchor placement.** Built a purpose-made gate instead, loading the **EXPORTED** FBX (the artifact that will actually be imported, not in-memory state) and rendering **ORTHOGRAPHIC** views (image row maps linearly+exactly to Z — no perspective ambiguity) with corrected anchors ruled GREEN, uncorrected RED, bones cyan. 🔬 **EYEBALL VERDICT: PASS, NOT MARGINAL — the anchors landed on the BODY, not the antlers.** Every shipped anchor sits at its NOMINAL fraction **of true body height** to 4 dp (`head_top` **0.9999** vs target 1.0 · `head_base` 0.9049 vs 0.905 · `neck_top` **0.8649** vs 0.865 · `shoulder` 0.7999 · `hip` 0.5000 · `knee` 0.2700), and mesh occupancy confirms it physically: at the corrected `head_top` the mesh has **106 verts of solid skull**, at the uncorrected height only **18 verts of thin antler tine**. In `ruler_head.png` the green rule lands ON THE SKULL DOME and the red one ON THE ANTLER TIPS. The uncorrected `neck_top` reproduces TASK-370's **0.9121 exactly** — independent cross-validation of the handed-over measurement. **No retune was warranted and none was invented.** Clips + bind pose eyeballed: clean A-pose, identity fully intact (antler crown, cream cowl, **blue `TeamRegion` reading strongly off the plate tops**, moss robe, sash, boots, monolith). ⚠️ **FLAG 1 — THE BOARD'S OWN TASK-369 GATE TEXT STATES THE FAILURE DIRECTION BACKWARDS:** it says an overshooting prop "slides the whole rig **down** the body"; measured, it slides it **UP** (anchors are fractions of the inflated bbox, so `neck_top` rides at 0.9121 of body height instead of 0.865). TASK-370's numbers are self-consistent and the divide-by-factor fix is unaffected — **nothing changed in what shipped** — but flagging it so nobody "fixes" a correct override after reasoning from the wrong direction. ⚠️ **FLAG 2 (quantified, deliberately NOT acted on):** `measure_anchors()` samples its half-width bands as hard-coded fractions of the INFLATED bbox (shoulder band runs 138.17–152.72 UE instead of 131.04–144.83). **Measured rather than assumed: Δ shoulder_half 1.65 UE → 1.42 UE at the joint; Δ hip_half 0.80 UE → 0.42 UE.** Left untouched on purpose — blast radius would be all 13 assets for a sub-2 UE effect, and it is not what the ruling asked for. ⚠️ **FLAG 3 (fleet-wide, NOT an antler artifact):** arm/leg bones ride at the outer edge of the robe because `*_x_frac` multiplies MEASURED half-width (shoulders x ±33.71 vs silhouette ±40.03; legs ±21.73 vs mesh legs ~±29) — identical in kind to the shipped Wizard (sho_x 52.3), bone-heat still 0.00% unweighted so every bone is inside the volume. The `*_x_frac` lane is unaffected by the antler factor; "fixing" it would be an unmeasured tune. Playtest note: the rune monolith is body geometry, so it swings with the arm in `Walk` (same class as the Wizard staff / Ogre maul). 🔑 **READBACK FOR THE IMPORT PASS: top-level `"skeleton"` = `SiegeBiped` (THE AUTHORITATIVE FIELD); `armature.skeleton` = `Sorcerer` = the CARD ID, NOT an asset name (the recorded myth-source that cost a correction round); bind to the EXISTING `/Game/Characters/SK_Footman_Skeleton` — NEVER create a skeleton.** `Sorcerer.lod.json` emitted, schema-identical to `Wizard.lod.json` (LOD1 50%@0.4 / LOD2 20%@0.15 + both URO flags). Full evidence in `handoffs/TASK-372-artist.md`; gate artifacts in `Cache/Sorcerer/rig/anchor_check/`. — 🔍 **TOOLING CODE LANE qa-passed** (2026-08-01, `qa/TASK-372-tooling-report.md`) — 0 blockers, 4 WARN, 4 NIT. Bit-exact-no-op-for-the-12-shipped-rigs claim **UPHELD** (verified structurally *and* against the surviving `cmp_footman.py` + Footman cache backup); `head_base_z` default `0.905` confirmed identical; unknown-key exit-2 hard-fail confirmed real and correctly scoped; all 16 Sorcerer override values independently recomputed at `nominal / 1.0545`; the `*_x_frac` exclusion confirmed **in code** (they multiply measured half-widths `sh`/`hh`, never `H`). Anchor-gate method accepted. Open WARNs: malformed (non-dict) `proportions` still silently ignored; the new code comment + manifest `_doc` state the inflation direction **backwards** (it slides the rig UP, not down); no range/ordering validation on override values; and `rig_character.py` is the only `.py` under `Tools/` with CRLF — build-master must run `git diff --numstat` on it before staging. **The editor half of TASK-372 remains un-reviewed and un-done.** — ⚠️ **SCOPE CAVEAT ON THAT PASS (QA's own note 4): it covers the TOOLING CODE LANE ONLY and must NOT be read as "TASK-372 done."** The UE-editor half (`SK_Sorcerer` import, the four `A_Sorcerer_*` clips, LOD chain + `lod_count == 3` readback) is **un-started**; the `.fbx`/`.lod.json` artifacts are an art-integration check, not a code review. Status stays **import-pending**. ✅ **WARN-1 and WARN-2 are now FIXED (2026-08-01, same pass):** WARN-1 — a present-but-malformed `proportions` (list / string / number / null / bool / **empty object**) now **hard-fails exit 2 naming the offending value**, closing the type-door re-entry of the exact defect the feature exists to prevent; proven by an 18-check harness (`test_override_guard.py`, ALL PASS) that also re-confirms the shared `SiegeBiped` dict is unmutated after all 13 resolves, unknown-key exit 2 still fires, a valid override on a second asset applies without leaking, and an absent key still takes the untouched default path. WARN-2 — the backwards direction is corrected in **both** places a maintainer reads (the `merged_params` comment and the manifest `_doc.proportions_override`): an inflated bbox pushes anchors **UP** the body (`neck_top` 0.865 → lands at 0.9121 of true body height; `head_top` 1.0 → lands on the antler tips), each now carrying an explicit MIND-THE-SIGN warning that correcting from the wrong direction would DOUBLE the error. **WARN-3 (no type/range/ordering validation on override values) and WARN-4 (CRLF EOL question) remain OPEN as recorded follow-ups — not done this pass.** EOLs re-verified after the fixes: `rig_character.py` 1,211 CRLF / **0 bare LF**, `rig_manifest.json` **0 CRLF** / 147 LF — neither file's EOL style was changed by this work. ← was: backlog
- blocked-by: TASK-371 — **still blocking the EDITOR half only** (the headless half needed only TASK-370's `Content/RawAssets/Sorcerer.fbx`, which exists)
- parallel-safe: no (EXCLUSIVE editor)
- spec: >
    **(1)** `blender --background … rig_character.py -- --card-id Sorcerer` (from repo root).
    **(2) ⚠️ BIND TO THE EXISTING SHARED `SK_Footman_Skeleton` (`/Game/Characters/SK_Footman_Skeleton`) — NEVER CREATE A SKELETON.**
    There is exactly ONE skeleton asset in `/Game/Characters`; a bespoke per-unit skeleton needs an explicit manager ruling and does not
    have one. ⚠️ **When reading the rig report, the AUTHORITATIVE field is the TOP-LEVEL `"skeleton"`; `armature.skeleton` is the CARD ID,
    not an asset name** (the recorded myth-source that cost a correction round).
    **(3)** `SK_Sorcerer` at `/Game/Characters/SK_Sorcerer`, SAME two-slot material contract as `SM_Sorcerer`.
    **(4)** Anim clips `A_Sorcerer_{Idle,Walk,Attack,Death}` at `/Game/Characters/Anims/`. **PRODUCE the Attack clip** — the Cleric
    precedent: it is free, and it is the ready-made hook for a consecration gesture even though this unit never attacks.
    **(5) NO `ABP_Sorcerer`** — the runtime falls back to `ABP_Footman`. Do not author one.
    **(6) LOD CHAIN per the M7.6 SK-unit law:** LOD1 50% @ screen 0.4 / LOD2 20% @ 0.15 (`regenerate_lod`), plus
    `VisibilityBasedAnimTickOption = OnlyTickPoseWhenRendered` and `bEnableUpdateRateOptimizations = true` on `SkeletalVisualMesh`.
    Readback `lod_count == 3`.
    **(7)** ⚠️ **The SKELETAL yaw is C++-owned (`SkeletalVisualYawOffset = -90.f`, ABSOLUTE assignment) — do NOT hand-author it on any asset.**
    **HARD READBACK:** bound skeleton name, bone count, the four clips resolving, `lod_count`. **NO Git.**
    Handoff `handoffs/TASK-372-artist.md`. Post in 🎨 Art.
- names: >
    `/Game/Characters/SK_Sorcerer` bound to `/Game/Characters/SK_Footman_Skeleton` · `/Game/Characters/Anims/A_Sorcerer_{Idle,Walk,Attack,Death}` ·
    NO `ABP_Sorcerer` (falls back to `ABP_Footman`). Law: CONVENTIONS §3 asset set + "Skeletal rig & animation workstream (M7)" +
    the M7.6 SK-LOD law + the shared-skeleton binding law.

#### TASK-373 — [AG-A6] `T_CardArt_Sorcerer` — the hand/deck-builder card face (art-director)
- ⚠️ **AMENDED 2026-08-02 — CARD RE-RENDERED + RE-IMPORTED (`handoffs/TASK-375-facing-fix.md`).** Camera re-derived: **`MODEL_YAW_DEG` 196 → 16** (the 180° compensation is removed now that the source fronts −Y; only the ¾ kick remains). **Jonathan's approved key is HELD EXACTLY** — rgb8 **(29, 79, 69)**, hue 168.0 / sat 0.633 / val 0.310, Lab (30.1, −19.6, 0.9), **ΔE2000 10.14 to Archer** (runners-up 10.18 / 10.55 / 12.53) — TASK-373's own `accept.py` re-run VERBATIM. Halo 7.17× · headroom 10.16% / floor 12.11% · 512×512 RGB opaque · reads at 128/96 px. Re-import verified by pixels with flip controls (as-is **0.937** vs h-flip 8.459 / v-flip 34.385 / rot180 34.692; key in-engine (30,79,68) vs source (29,79,69)); `TEXTUREGROUP_UI` + sRGB ON + `TC_Default` re-asserted. ⚠️ **TWO DECLARED DEVIATIONS:** render engine is **CYCLES** (headless Blender 5.1 EEVEE renders this scene flat — measured spread 0.060 vs 0.281), and **the original render script was DELETED BY THE FIX PASS** (my error, reported in the handoff §7d) so the scene was rebuilt from PART 1 §2's recorded parameters — **TASK-385 loses its head start**. `accept.py` survives intact. ℹ️ `TextureTools.get_size` returns the RESIDENT MIP (32×32 on a cold editor, for the shipped Wizard/Footman/Archer too) — the authoritative field is the registry `Dimensions` tag.
- assignee: art-director
- status: **done** (2026-08-02 — INTEGRATED by TASK-378, commit B `80c47e8`. Jonathan's PIE ship gate TASK-377 PASSED.) ← was: **ready-for-integration** (2026-08-02) — ✅ **IMPORT COMPLETE: `/Game/UI/CardArt/T_CardArt_Sorcerer`
  imported, configured, readback-verified, VISUALLY GATED and saved** (`is_dirty=false`, saved by explicit
  single-path list — never save-all, `L_Arena` never opened, PIE never started, editor never closed).
  ⚠️ **THE LIVE TRAP: the importer defaults `LODGroup` to `TEXTUREGROUP_World`, NOT `TEXTUREGROUP_UI`** —
  caught by reading the asset immediately after import BEFORE setting anything. `SRGB` and
  `CompressionSettings` landed correct on their own, but **Texture Group did not**; anyone assuming
  "import defaults are fine" ships a card face in the World group (wrong streaming/mip behaviour), and it is
  invisible both in the content browser and on the card. 🔬 **POST-SET READBACK (re-read after the save, not
  assumed from the set call): `TEXTUREGROUP_UI` · `SRGB true` · `TC_Default` · `TMGS_FromTextureGroup` ·
  `TF_Default` · `NeverStream false` · `CompressionNoAlpha false` · `PowerOfTwoMode None` · `TA_Wrap/TA_Wrap` ·
  `MaxTextureSize 0` · `TCQ_Default` · `bUseLegacyGamma false` · `VirtualTextureStreaming false` — 14/14
  IDENTICAL to the shipped fleet.** Settings were **read off THREE shipped cards first** (`T_CardArt_Wizard`
  TASK-303 · `T_CardArt_Footman` TASK-077 · `T_CardArt_Pickpocket` TASK-081); all three agree with each other
  AND with the spec, so **no fleet-vs-spec conflict existed here** (unlike TASK-372). **512×512 confirmed by
  `get_size` readback.** ✅ **PATH CONTRACT VERIFIED BYTE-EQUAL, not eyeballed:** the `cards.csv` cell and the
  engine object path compare equal as raw strings AND as hex — `/Game/UI/CardArt/T_CardArt_Sorcerer.T_CardArt_Sorcerer`;
  on-disk casing checked with a real directory listing. **`cards.csv` NOT edited** (already correct). 📊 **BONUS
  ROSTER SWEEP: all 30 rows now resolve — 0 missing card art, 0 malformed cells; card-art coverage is COMPLETE
  for the first time (30 PNGs / 30 uassets).** ✅ **VISUAL GATE ACTUALLY RUN AND MEASURED — `CaptureAssetImage`
  DOES work on Textures:** the thumbnail is the authored render (antler crown · faceless stone mask · granite
  shoulder slabs · cream cowl · rust sash · moss robe · rune monolith at screen-right · jade rune rings), and
  rather than trust "looks right" I diffed it against the source PNG with wrong-orientation controls —
  **as-is MAE 1.093 (DXT noise) vs h-flip 10.913 (10.0× worse) vs v-flip 31.342 (28.7× worse)**, so the texture
  is **provably not mirrored/flipped/rotated**. Worth proving on THIS asset specifically, since the Sorcerer is
  the fleet's one known ~180° facing anomaly (TASK-371/372). **Colour space confirmed by pixels: mean luma
  0.3960 vs source 0.3954 (Δ0.0006) and backdrop key rgb8 (30,79,68) vs (29,79,69) — Δ≤1/255**, so PART 1's
  hard-won ΔE2000 10.14 separation from Archer is intact IN-ENGINE, not just in the source; sRGB-off would have
  dragged luma to ~0.13. ✅ **ZERO STRAYS:** `find_assets("/Game","Sorcerer")` = exactly **11** (10 pre-existing
  TASK-371/372 assets + this one). 📌 **For build-master: TWO files to commit** — `Content/RawAssets/CardArt/Sorcerer.png`
  + `Content/UI/CardArt/T_CardArt_Sorcerer.uasset`; **no code/Blueprint/widget wiring needed** (the resolver seam is
  data-driven and the path already matches), though **`DT_Cards` may want a CSV reimport** to pick the row up.
  **NOT done deliberately: the render script stays scratchpad-only — making it durable at
  `Tools/ArtPipeline/cardart_render.py` is TASK-385's QA-gated tooling lane.** Handoff `handoffs/TASK-373-artist.md`
  PART 2. ← was: PNG-complete, import-pending
- status-history: **PNG-complete, import-pending** (2026-08-01) — `Content/RawAssets/CardArt/Sorcerer.png` authored,
  verified 512×512 RGB opaque, no baked text. **UE IMPORT DELIBERATELY NOT DONE** (Jonathan holds the editor;
  TASK-372 running in parallel) — re-dispatch for the editor-gated import to `/Game/UI/CardArt/T_CardArt_Sorcerer`.
  Handoff `handoffs/TASK-373-artist.md`.
  ⚠️ **THE PLAN'S JADE-UNIQUENESS NOTE WAS WRONG** — it checked only violet-family keys. Measured against all 29
  shipped keys, the green-teal arc is CROWDED (Pickpocket 124.1 · Archer 134.8 · Cleric 178.2 · BallistaTower 188.1 ·
  CrystalTower 190.1 · Footman 198.0); the real constraint is **Archer/Pickpocket**, not the violets. Shipped key is a
  grid-search argmax: **hue 168.0 / sat 0.633 / val 0.310, rgb8 (29,79,69)**, nearest shipped ΔE2000 **10.14** (Archer)
  against a roster nearest-neighbour median of **7.85** (~83rd pct). `M_AncientGround`'s `GroundColor` is hue **156** —
  used VERBATIM for the floor rune rings; the backdrop sits 12° off it to buy separation.
  ⚠️ **CARRIED FORWARD (for any future render of this asset): `Sorcerer.fbx`'s FRONT faces +Y, NOT −Y** — the pipeline's
  "front faces Blender −Y" convention does not hold here; a −Y camera renders the back of the skull.
  ✅ **RULED BY JONATHAN — RESOLVED (was "§4 ruling wanted"):** this face renders the REAL `SM_Sorcerer` (per the brief)
  while all 29 shipped card arts are primitive "board-game token" renders, so Sorcerer is visibly higher-fidelity than the
  roster. **Jonathan ACCEPTED it as the new M7-tier standard**; the other 29 get re-rendered later, boarded as
  **TASK-385..388**. Sorcerer was NOT rebuilt as a token.
- blocked-by: TASK-369 (needs the accepted concept)
- parallel-safe: yes for the render; **editor import serializes** with the other editor tasks
- spec: >
    Author the card illustration per the "Card artwork (hand UI)" law. **512×512 exactly**, Texture Group = UI, sRGB ON, default compression.
    Source PNG committed at `Content/RawAssets/CardArt/Sorcerer.png` (casing character-for-character from the cards.csv row name), imported
    to `/Game/UI/CardArt/T_CardArt_Sorcerer`.
    **STYLE BAR:** must read at ~150 px — ONE dominant subject, strong silhouette, **jade/teal key color** (distinct from every other card),
    team-agnostic palette, **NO baked-in text** (DisplayName + cost overlay at runtime).
    The CSV `CardArt` cell already points at `/Game/UI/CardArt/T_CardArt_Sorcerer.T_CardArt_Sorcerer` — **the asset must land at exactly
    that path** or the card face silently falls back to text-only.
    **NO Git.** Handoff `handoffs/TASK-373-artist.md`. Post in 🎨 Art.
- names: >
    `Content/RawAssets/CardArt/Sorcerer.png` → `/Game/UI/CardArt/T_CardArt_Sorcerer`. Law: CONVENTIONS "Card artwork (hand UI)" + §3 asset set.

#### TASK-374 — [AG-A7] `M_AncientGround` — jade rune-ring pulsing decal (art-director)
- assignee: art-director
- status: **done** (2026-08-02 — INTEGRATED by TASK-378, commit B `80c47e8`. Jonathan's PIE ship gate TASK-377 PASSED.) ← was: **ready-for-integration** (2026-08-01) — `/Game/Materials/M_AncientGround` authored, compiles clean, saved.
  Handoff `handoffs/TASK-374-artist.md` (+ captures in `handoffs/TASK-374/`). ⚠️ **CARRIED FORWARD TO TASK-359 (code):
  `SortOrder 10` is NOT a `UMaterial` property in UE 5.8** — verified by full property dump; it lives on `UDecalComponent::SortOrder`,
  so `AAncientGround` must call `SetSortOrder(10)`. `ACaptureZone` leaves it at default 0 (repo-wide grep for `SortOrder` = 0 hits),
  so any positive value wins. `DecalSize (1024, 840, 840)` + `FadeScreenSize 0.001` are likewise component-side (TASK-359).
- blocked-by: none
- parallel-safe: yes (no code dependency; **editor slot serializes**)
- spec: >
    Author `M_AncientGround` per CONVENTIONS §2 ("Material") + plan §6. **Clone `M_CaptureZone`'s STRUCTURE, NOT its look.**
    **(1) STRUCTURE:** `MD_DeferredDecal` · `BLEND_Translucent` · **Emissive + Opacity only, BaseColor UNCONNECTED** ·
    ⚠️ **STOCK NODES ONLY — the Custom-HLSL BAN is absolute on this project** (a Custom node detonates shader permutations and wedges the
    editor under the Substrate + HW-RT stack; TASK-239 lost ~4 h to it, the stock-node rebuild compiled in seconds).
    **(2) SHAPING — it must NOT read as a recolored capture zone** (they meet near the centerline): concentric **rune rings** inside the
    square footprint + a slow **`Sine(Time)` pulse**.
    **(3) PARAM:** vector parameter named exactly **`GroundColor`**, default jade **(0.10, 0.85, 0.55)** — maximally far from Blue
    (0.05,0.30,1.00), Red (1.00,0.10,0.05) and neutral grey.
    **(4) `SortOrder 10`** so it draws ABOVE `M_CaptureZone`.
    **(5) ⚠️ The DECAL SIZING is C++-side (TASK-359) but you must SANITY-CHECK the look at `840×840` ground half-extents with a `1024`
    projection half-depth**, and **`FadeScreenSize = 0.001`** (the 0.01 default culls decals at this arena's zoom-out).
    **(6)** Verify it compiles clean — grep `Failed to compile Material` = 0. **NO Git. NO C++.**
    Handoff `handoffs/TASK-374-artist.md` with the node list and a capture. Post in 🎨 Art.
- names: >
    `/Game/Materials/M_AncientGround` (Content/Materials/) · vector param `GroundColor` default (0.10, 0.85, 0.55). Donor structure
    (read-only): `/Game/Materials/M_CaptureZone`. Consumed null-safe by `AAncientGround` (TASK-359). Law: CONVENTIONS §2 + "Material &
    Niagara lane laws" (Custom-HLSL BAN). Plan §6.

#### TASK-375 — [AG-A8] `BP_Unit_Sorcerer` — reparent to `ASorcererUnit` + the static `VisualMesh` ritual (art-director)
- ⛔ **SUPERSEDED IN PART 2026-08-02 by TASK-375-FACING-FIX (`handoffs/TASK-375-facing-fix.md`) — §3's `+90` and §4's three backwards seams are CLOSED.** The defect was fixed **at the source**, not at any consumer: `pre_rotate_z_deg = 180` in `pipeline_manifest.json`. **`BP_Unit_Sorcerer`'s `VisualMesh` yaw is back to the fleet `−90`** (Z still `−88.0` = −(READ capsule half-height **88.0**), scale 1, `[MI_TeamColor_Blue]`, compiled `warnings_as_errors=True`, saved). **`SkeletalVisualYawOffset` remains the C++ `−90`, NOT overridden — no exception hatch was authored, no C++ touched, no law amended.** ⇒ **the spec text below and CONVENTIONS §294/§493 saying `−90` are CORRECT again**; this entry's earlier claim that they were "now WRONG" is itself superseded. §2, §3b (incl. the still-open `88/34` vs mesh-matched ≈91/40 capsule question), §5, §5a and §7 remain accurate.
- assignee: art-director
- status: **done** (2026-08-02 — INTEGRATED by TASK-378, commit B `80c47e8`. Jonathan's PIE ship gate TASK-377 PASSED.) ← was: **ready-for-integration** (2026-08-02) — ✅ **`/Game/Blueprints/Units/BP_Unit_Sorcerer` CREATED FRESH,
  parent `ASorcererUnit`, compiled `warnings_as_errors=True` (0 errors AND 0 warnings), saved (`is_dirty=false`,
  explicit single-path list — never save-all, `L_Arena` NEVER saved, PIE stopped throughout, no Git, no C++).**
  **No duplicate AND no reparent at all** — `BlueprintTools.create` accepted the actor class directly as `asset_type`,
  so the recorded duplicate+reparent corruption pattern was never approached.
  🎯 **THE FACING RITUAL — SHIPPED YAW IS `+90`, NOT `−90`, AND IT IS PROVEN BY A POSITIVE CUE IN THE VIEWPORT.**
  ⚠️ **This CONTRADICTS the spec text below and CONVENTIONS §294/§493, which both say `−90`; those lines are now WRONG
  for this one asset and should be amended.** Three independent lines agree: (1) TASK-371/372's prior readings;
  (2) **an independent axis DERIVATION neither prior task ran** — the C++ fleet contract is front on Blender −Y →
  UE-local +Y ⇒ yaw −90, but TASK-373 recorded `Sorcerer.fbx` fronts Blender **+Y** and TASK-371 §4 independently
  measured that FBX import **mirrors Y**, so this asset's front arrives at UE-local **−Y** ⇒ `Rot(θ)·(0,−1,0)=(1,0,0)`
  ⇒ **θ = +90**; (3) **THE VIEWPORT CHECK.** `CaptureAssetImage` **does NOT support Blueprints** ("Asset type does not
  support image capture"), so temp actors were placed at **actor yaw 0** (actor-forward = world +X) and captured via
  explicit `captureTransform` (Jonathan's viewport camera never moved). **Control first:** `BP_Unit_Footman` (yaw −90)
  in that camera shows face/blue helm/spear/shield/lion tabard. **Then the Sorcerer at +90: the faceless STONE MASK
  (two eye sockets + muzzle) faces the direction of travel — a POSITIVE cue, not an absence — corroborated by the
  RUNE MONOLITH (front-only) and the CENTRAL KNOTTED SASH (front-only); the reciprocal capture shows a maskless dome
  with the monolith hidden.** That reciprocal image IS what −90 would have put in the travel direction, so
  **−90 is not merely "suspect", it is measured WRONG here.** 📏 **CAPSULE HALF-HEIGHT READ OFF THIS BP = `88.0`
  (radius 34) → `VisualMesh.RelativeLocation.Z = −88.0`** — both numbers reported, "== 90" never assumed.
  🔬 **HARD READBACK (post-save):** parent `SorcererUnit` · `CardID` `Sorcerer` · `VisualMesh` = `SM_Sorcerer`,
  rot `(0,+90,0)`, loc `(0,0,−88)`, scale `(1,1,1)`, slot 0 `MI_TeamColor_Blue`, visible+castshadow ·
  **`SkeletalVisualMesh` UNTOUCHED at `(0,0,0)`/`(0,0,0)`, `SkeletalVisualYawOffset` left at the C++ `−90`, NO `+=`
  anywhere** · deps exactly `[module, SM_Sorcerer, MI_TeamColor_Blue]` · 13 unit BPs, `find_assets("/Game","Sorcerer")`
  = exactly 12, **zero strays**, no `ABP_Sorcerer`. ✅ **SPAWN PATH VERIFIED, NOT ASSUMED:**
  `/Game/Blueprints/Units/BP_Unit_Sorcerer.BP_Unit_Sorcerer_C` **loads** AND appears in
  `search_subclasses(ASummonedUnit)` ⇒ `IsChildOf(ASummonedUnit)` passes ⇒ **NO C++/spawn change needed.**
  📊 **All 12 shipped BPs were dumped first and matched:** the Z==−CapsuleHalfHeight ritual is exact on **12/12**;
  `OverrideMaterials=[MI_TeamColor_Blue]` on 11/12 (Wizard `[]`) — followed the 11. **CONVENTIONS §493's "Wizard is
  the ONE outlier at yaw 0 / Z 0" is now STALE — TASK-334 fixed it; it measures `−90` / `−88` today.**
  🔴 **FLAGGED, NOT FIXED (C++ concern → needs a manager ruling + a separate task): the SKELETAL visual AND the
  PLACEMENT GHOST will both face BACKWARDS.** Proven, not asserted: a `SkeletalMeshActor` of `SK_Sorcerer` placed at
  world yaw **−90** (exactly what `ResolveSkeletalVisual` forces at runtime) shows its **BACK** to the travel
  direction — featureless dome, no mask — while `SK_Footman` at the same yaw shows its FRONT; the reciprocal capture
  shows the mask + eye sockets, so it is a genuine 180° established by a positive cue on **both** sides (TASK-372
  correctly warned "no face" alone is non-decisive for a faceless-mask unit). **Consequence: once `SK_Sorcerer`
  resolves at BeginPlay it becomes the runtime visual and the static mesh is hidden, so in normal play the Sorcerer
  marches/casts/dies backwards** — the static ritual is correct, the runtime result still needs the C++ task.
  **THIRD SEAM, same root cause:** `ASiegePlayerController::GhostYawOffset = −90.f` (`SiegePlayerController.h:825`)
  ⇒ **the drag-to-place GHOST also shows the back** (§486's "the ghost has never mis-faced" assumes the fleet bake,
  which this asset does not follow). **Two candidate fixes, both deliberately NOT taken:** override the documented
  `SkeletalVisualYawOffset` `EditDefaultsOnly` exception hatch (§489 makes that a NON-DEFAULT requiring an explicit
  manager ruling), or re-export `Sorcerer.fbx` 180° so it joins the fleet bake (fixes all three seams at once and
  reverts this BP to −90, but invalidates `SM_Sorcerer`, `SK_Sorcerer`, the 4 anim clips and the card render).
  ⚠️ **ALSO RECORDED, NOT ACTED ON: the capsule `88/34` is the raw `ACharacter` default** — neither `ASummonedUnit`
  nor `ASorcererUnit` resizes it, and **11/12 shipped units hand-author theirs to match their mesh** (only the Wizard
  ships at the default, which is what a fresh BP inherits). `SM_Sorcerer` is 181.81 tall / 90.98 wide ⇒ a mesh-matched
  capsule would be ≈ **91/40** (the Cleric's exact profile). Not set: out of spec scope, collision/nav-adjacent, no
  ruling — and the BP is self-consistent either way since Z tracks whatever the capsule is. **One-line follow-up if
  the manager wants it: set the capsule, re-derive Z.** 🧹 **LEVEL HYGIENE:** 4 temp actors placed for the captures,
  **all 4 removed** (`remove_from_scene` true ×4; `find_actors` sweep on `TMP_`/`Sorcerer`/`SkeletalMeshActor` = `[]`
  ×3). **`L_Arena` left dirty-in-memory and NEVER saved — do NOT save it to "clean it up".**
  ⚠️ **ONE ASSET I NEVER TOUCHED READS DIRTY — reported, deliberately NOT saved: `MI_Sorcerer_PBR`.** Checked whether
  it was a general artifact — it is not: **exactly 1 of the 21 `MI_*_PBR` instances is dirty, this one.** Verified by
  readback that it is **derived state, not an edit** — parent `M_AssetPBR`, `BaseColor`/`Normal`/`ORM` → the three
  `T_Sorcerer_*`, deps clean, **identical to TASK-371's recorded values**. Almost certainly the shader map cached on
  first *in-level render* (my §3/§4 captures were the first time `SM_/SK_Sorcerer` were rendered this session; every
  other MI has rendered in `L_Arena` many times — fits the 1-of-21 pattern). **DISCARD IT — do not save, do not commit
  `MI_Sorcerer_PBR.uasset`.** Flagged only so the dirty marker isn't later mistaken for a TASK-375 edit. All other
  watched assets clean (`BP_Unit_Sorcerer`/`BP_Unit_Footman`/`SM_Sorcerer`/`SK_Sorcerer`/`SK_Footman`/`MI_TeamColor_Blue`
  all `is_dirty false`); PIE false, zero asset editors open, editor never closed.
  📌 **For build-master: ONE file to commit** — `Content/Blueprints/Units/BP_Unit_Sorcerer.uasset` (+ this handoff and
  4 `TASK-375-*.png` captures); **no code/CSV/widget wiring needed**; expect the flagged backwards skeletal facing in
  any verify pass and treat it as KNOWN, not a new regression. Handoff `handoffs/TASK-375-artist.md`. ← was: backlog
- blocked-by: TASK-366 (the `ASorcererUnit` class must exist compiled) · TASK-371 (`SM_Sorcerer`) · TASK-372 (`SK_Sorcerer`)
- parallel-safe: no (EXCLUSIVE editor)
- spec: >
    Create `BP_Unit_Sorcerer` at `/Game/Blueprints/Units/BP_Unit_Sorcerer`, **parent class `ASorcererUnit`** (NOT `ASummonedUnit`).
    **Build it FRESH — never duplicate+reparent** (the recorded runtime-repaint corruption lesson). CardID = `Sorcerer`;
    `VisualMesh` = `/Game/Meshes/SM_Sorcerer`.
    **⚠️ ACCEPTANCE CRITERION, EXPLICIT AND NON-NEGOTIABLE (manager ruling 10): hand-author the STATIC `VisualMesh` transform as
    yaw −90 and Z = −(the capsule half-height YOU READ from this Blueprint).** **"Half-height == 90" is BANNED** — shipped values span
    −74.5 to −145. **READ the value in-editor, negate it, and REPORT BOTH NUMBERS (the read half-height and the applied Z) in the handoff.**
    `BP_Unit_Wizard` is the unit that missed exactly this and needed a follow-up task; the Sorcerer is the next one that can.
    **The SKELETAL yaw is C++-owned (`SkeletalVisualYawOffset = -90.f`, absolute assignment) — do NOT hand-author it here** (`+=` anywhere
    is an automatic QA FAIL).
    **VERIFY:** the spawn path needs NO change (`IsChildOf(ASummonedUnit)` passes by inheritance) — confirm the composed path
    `/Game/Blueprints/Units/BP_Unit_Sorcerer.BP_Unit_Sorcerer_C` resolves. Compile + Save. **NO Git.**
    Handoff `handoffs/TASK-375-artist.md`. Post in 🎨 Art.
- names: >
    `/Game/Blueprints/Units/BP_Unit_Sorcerer` (parent `ASorcererUnit`) · `VisualMesh` = `/Game/Meshes/SM_Sorcerer` · CardID `Sorcerer`.
    Law: CONVENTIONS "Blueprint subclasses of C++ classes" + "Per-card visual assets" + §3 (the VisualMesh ritual). Plan §6.

#### TASK-375-FACING-FIX — Sorcerer 180° facing defect: fix AT SOURCE + re-land downstream (art-director)
- assignee: art-director
- status: **done** (2026-08-02 — INTEGRATED by TASK-378, commit B `80c47e8`. Jonathan's PIE ship gate TASK-377 PASSED.) ← was: **ready-for-integration** (2026-08-02) — 🎯 **ROOT CAUSE FOUND AND FIXED AT SOURCE. It is NOT "the
  Sorcerer's geometry is rotated 180°" — it is the TASK-348 `_ue_handedness_precomp` MIRROR-FIX (2026-07-28), and the
  Sorcerer is simply the FIRST UNIT exported after it.** Every other unit FBX predates it (Footman/Cleric/Wizard/
  Archer/Knight/Cavalry/Longbowman/Miner/Sapper/Pikeman/MilitiaMob 2026-07-26, Ogre 2026-07-27), so the fleet lands in
  UE Y-mirrored (conformed front −Y → UE-local +Y, which is exactly why C++ `−90` is right for them) while the
  pre-compensated Sorcerer landed un-mirrored (→ UE-local −Y) and read 180° wrong at the same `−90`.
  **Proof:** shipped `Footman.fbx` renders FRONT from −Y; old `Sorcerer.fbx` renders BACK from −Y and FRONT from +Y;
  numerically, the FBX measures Y −39.212…+39.157 on disk and TASK-371 read it back in UE at Y −39.157…+39.212 — an
  exact negation, with X matching identically.
  ✅ **FIX: `pre_rotate_z_deg = 180.0` on the Sorcerer manifest entry.** Sign is a non-issue (`Rz(180) ≡ Rz(−180)`, and
  it commutes with the export mirror), and the net is a **pure rotation** so chirality is preserved (monolith stays on
  the figure's own right, as in the concept). **NO C++ exception hatch, NO `SkeletalVisualYawOffset` override, NO law
  amended** — all three seams resolve at once and `−90` is correct everywhere again.
  ✅ **Stage 2 re-run 11.7 s, ZERO Meshy credits; FULL pre-import gate re-passed:** UV-norm albedo **0.4403** (floor
  0.2536) · luma retention **0.9811** (band 0.85–1.25) · chroma **0.9292** · hue shift **2.55°** · 15,000 tris ·
  `UVMap` · feet-centre · slots `[TeamRegion, SorcererPBR]` · TeamRegion **3.12%, two islands, 0 faces at |x|<5, no
  head paint** · warnings `[]`. `albedo_delight` PINNED to the locked fleet values; mask at `neighbor_tol=0.010`
  (concept fg fraction reproduces **0.3377 exactly**); harness re-validated 6/6 anchors to 4 dp first.
  ✅ **Stage 3b re-run** (exit 0, 21 bones, `auto_heat` 0.00% unweighted, zero warnings); `proportions` override
  unchanged, antler factor **1.0545** confirmed (`head_top_z` → 172.48 UE vs skull apex ~172.42).
  ✅ **Re-imported IN PLACE over the same paths via headless commandlets** (MCP `import_file` refuses existing paths) —
  **never delete+recreate**; `SM_Sorcerer`'s referencer `[BP_Unit_Sorcerer]` identical before AND after; textures
  imported **explicitly** so the armed texture-skip trap was avoided; `SK_Sorcerer` on the **EXISTING**
  `SK_Footman_Skeleton` (no new skeleton); 4 anims root-motion OFF / `bForceRootLock` ON. **Exactly 12 Sorcerer
  assets, ZERO strays; `ABP_Sorcerer` still absent; 3160-asset sweep = ZERO dirty.**
  🎯 **FACING PROVEN IN THE VIEWPORT — three tests, each with a Footman control, all by POSITIVE cues (mask + eye
  sockets + muzzle, knotted sash, planted monolith), never absence-of-cue, every capture with an explicit
  `captureTransform` (Jonathan's viewport camera never moved):** (A) at actor yaw 0 from a **+Y camera** `SK_Sorcerer`
  presents its FRONT — the exact invariant `SkeletalVisualYawOffset`'s doc derives `−90` from, so **12/12 becomes
  13/13**; (B) at the runtime `−90`, viewed from the direction of travel, it presents its FRONT — **the precise seam
  TASK-375 §4 proved was backwards**; (C) `BP_Unit_Sorcerer` at its reverted static `−90` presents its FRONT;
  (D) the ghost uses the same constant on the same mesh, so (C) is its configuration.
  ✅ **`BP_Unit_Sorcerer` `VisualMesh` yaw `+90 → −90`**, Z `−88.0` = −(READ capsule half-height 88.0), compiled
  `warnings_as_errors=True`, saved. `SkeletalVisualMesh` transform still untouched `(0,0,0)/(0,0,0)`.
  ✅ **Card art re-rendered + re-imported; Jonathan's approved key reproduces EXACTLY** — rgb8 (29, 79, 69), hue 168.0 /
  sat 0.633 / val 0.310, **ΔE2000 10.14 to Archer**; camera re-derived `MODEL_YAW_DEG` 196 → 16.
  ⚠️ **SYSTEMIC FLAG (manager/Jonathan, NOT taken): every FUTURE unit through Stage 2 inherits the same +180 need**
  until `_ue_handedness_precomp` is reconsidered for the unit lane or the fleet is re-exported — a 25-asset blast
  radius that would disturb TASK-348/350's castle collision/gate alignment.
  ⚠️ Two editor bounces were required (MCP cannot overwrite in place); before **each** kill a full 3160-asset dirty
  sweep showed the dirty set was **exactly `{L_Arena, MI_Sorcerer_PBR}`** — precisely the two the brief orders
  discarded — so both discards were measured lossless. **`L_Arena` NEVER saved and `MI_Sorcerer_PBR` NEVER saved;
  both are byte-untouched on disk.**
  ⚠️ **Self-reported error:** the fix pass deleted TASK-373's original card-render script from the shared scratchpad
  before reading it (handoff §7d) — unrecoverable; **TASK-385 loses its head start**. `accept.py` survives.
- blocked-by: none — supersedes the two fixes TASK-375 §4 proposed
- parallel-safe: no (editor + two editor bounces)
- handoff: `handoffs/TASK-375-facing-fix.md` (+ amendments appended to TASK-370/371/372/373/375 artist notes)
- files: `Tools/ArtPipeline/pipeline_manifest.json` · `Content/RawAssets/{Sorcerer.fbx, Textures/Sorcerer/*.png,
  Characters/Sorcerer.fbx, Characters/Sorcerer.lod.json, Characters/Anims/Sorcerer_*.fbx, CardArt/Sorcerer.png}` ·
  `Content/{Meshes/SM_Sorcerer, Characters/SK_Sorcerer, Characters/Anims/A_Sorcerer_*, Textures/T_Sorcerer_*,
  UI/CardArt/T_CardArt_Sorcerer, Blueprints/Units/BP_Unit_Sorcerer}.uasset`.
  **DO NOT COMMIT** `MI_Sorcerer_PBR.uasset` (discarded) or `L_Arena.umap` (never saved). No Git command was run.

#### TASK-376 — [AG-B2] `DT_Cards` reimport — Sorcerer row live + Footman DeckCount 9 (build-master)
- assignee: build-master
- status: **done** (2026-08-02) — ✅ **`Sorcerer` row LIVE, `Footman` DeckCount 11 → 9, `DT_Cards` saved (`is_dirty=false`). NO COMMIT (TASK-378 owns it), no push, `Docs/Data/cards.csv` NOT edited.**
  🔬 **HARD READBACK from a FRESH editor process (post close/reopen), not from the in-memory state that produced it:**
  `Sorcerer` = `CardType Unit` · `Cost 60` · `MaxCopies 2` · `HP 70` · `Damage/Range/Cadence 0/0/0` · `Speed 350` ·
  **`Profile Standard`** · `DeckCount 2` · `CardArt /Game/UI/CardArt/T_CardArt_Sorcerer.T_CardArt_Sorcerer`.
  **`Profile == Standard` checked explicitly** — `IsGroupCommandEligible()` gates on it, so `Support` would have silently made the
  unit uncommandable. ✅ **`sum(DeckCount) == 50` EXACTLY across all 30 rows** (recomputed from a fresh post-restart dump);
  row count 29 → 30, `Footman Cost 9` unchanged. ✅ **`CardArt` PROVEN to be a real object reference, not a stored string:**
  `get_dependencies(DT_Cards)` returned **29** CardArt textures pre-save and **30** post-save, `T_CardArt_Sorcerer` newly present.
  ✅ **NO-DRIFT PROVEN BY MACHINE DIFF, not asserted:** a full 30-column dump of all rows before vs after gives **exactly 2 deltas**
  (`+ Sorcerer`, `~ Footman.deckCount 11→9`), and post-state vs `cards.csv` gives **0 mismatches** across 30 rows × 30 columns.
  ⚠️ **METHOD — `add_rows` + `set_rows`, NOT `import_file`, and this was a MEASURED decision** (the spec's own note names
  `set_rows` as what sidesteps the import-factory enum quirk). **The column-count trap did NOT fire — the CSV is clean**
  (31 header fields, 30 data rows, **zero width mismatches**, 31 CRLF / 0 bare LF, 30 named columns mapping 1:1 onto `FCardRow`).
  **But a REAL adjacent hazard was found and avoided: 28 of 30 CSV rows carry an EMPTY `SpellDelivery` cell** while the table holds
  the enum's index-0 default `Auto` — and **`Fireball`/`FrostNova` carry a genuine non-default `HeroLine`** (matching the table).
  A wholesale reimport would have pushed 28 empty cells at an `ESpellDelivery` column whose empty-input parse behaviour is
  unverified, risking a silent revert of that real data. `set_rows` makes drift on the other 28 rows **structurally impossible**
  rather than merely verified-after-the-fact. Benign `LogCSVImportFactory "missing CardType"` warnings not chased, per spec.
  Handoff `handoffs/TASK-376-buildmaster.md` (covers BOTH jobs). ← was: backlog
- blocked-by: TASK-373 (so the `CardArt` path resolves on the first reimport instead of needing a second)
- parallel-safe: no (EXCLUSIVE editor)
- spec: >
    Reimport `Docs/Data/cards.csv` into `/Game/Data/DT_Cards` (the manager already wrote the CSV — **do not edit it**).
    **HARD READBACK, spot-check by value:** `Sorcerer` row present with **Cost 60 · MaxCopies 2 · HP 70 · Damage 0 · Range 0 · Cadence 0 ·
    Speed 350 · Profile Standard · DeckCount 2 · CardArt `/Game/UI/CardArt/T_CardArt_Sorcerer.T_CardArt_Sorcerer`** · `Footman` **DeckCount 9**
    (Cost 9 unchanged) · **`sum(DeckCount) == 50` recomputed and reported as a number** · **no other column on any other row drifted.**
    Save `DT_Cards.uasset`. **Benign, expected, do not chase:** stale `LogCSVImportFactory "missing CardType"` warnings on load — the
    import-factory enum quirk that `set_rows` sidesteps.
    **NO commit here** (TASK-378 owns it). Report `handoffs/TASK-376-buildmaster.md` + post the readback table in 🔧 Build & Git.
- names: >
    `Docs/Data/cards.csv` (READ-ONLY for this task) → `/Game/Data/DT_Cards` (row struct `FCardRow`). Law: CONVENTIONS "Data-driven card
    stats (GDD §3.0)" + §8 of the new section.

#### TASK-377 — [AG-H2] 🧑 JONATHAN — the PIE verification gate (THE SHIP GATE)
- assignee: **Jonathan (human — irreducible, manager ruling 9)**
- status: **done** (2026-08-02 — ✅ **PASSED BY JONATHAN**, the batch's last hard gate. His words, verbatim: *“I can confirm all the features listed work as expected.”* That clears the boost-bar checklist, the terrain/gameplay pass, and both flagged judgement calls (the rotationally-symmetric look and the permanent stacking boost) — the flags were ACCEPTED, not waived. TASK-378 unblocked and executed.) ← was: backlog
- blocked-by: TASK-368, TASK-375, TASK-376
- parallel-safe: no
- spec: >
    **WHY A HUMAN: agents cannot self-verify UMG rendering here** — `CaptureAssetImage` refuses WidgetBlueprints, screen-space Slate is
    uncapturable headless, and MCP readback has repeatedly PASSED on visually-broken UMG in this project. **Nothing commits until this
    passes (TASK-378).**
    **BOOST-BAR CHECKLIST (console; `SetTestDamageBoost` from TASK-363 is the deterministic lever):**
    | Console | Must be true |
    |---|---|
    | `SummonTestUnit Footman false` | **ONE bar only.** No boost row. |
    | `SetTestDamageBoost 50 true` | Second bar above, HALF full, **light blue**, medium-grey track, **light blue frame** |
    | `SetTestDamageBoost 100 true` | FULL, still light blue |
    | `SetTestDamageBoost 101 true` | Snaps near-empty; fill **AND outline** flip to dark blue. **If the outline did not change, the feature has FAILED.** |
    | `200 / 250 / 300` | full dark blue / half purple / full purple, frame matching |
    | `350` | half **black** — *is the fill clearly distinguishable from the grey track, and the frame from the world, at normal zoom?* |
    | `400` then `500` | full black; 500 IDENTICAL (clamped) |
    | `0` | boost bar **and frame** vanish; **health bar does NOT jump** |
    | `ApplyTestDamage 30` at 250 | health drops, boost row unaffected |
    | Zoom fully out, then fully in | both rows stay stacked and touching |
    | Look at a building and the hero | health bar ONLY, no boost row |
    **TERRAIN + GAMEPLAY PASS (same session):** the field is visibly **rotationally** symmetric (pick a distinctive hill or tree cluster and
    confirm its twin at the ANTIPODE, not the X-mirror) · two ancient-ground decals at `(−X,−Y)`-mirrored positions **matching the log line** ·
    Play Again ×3 re-rolls both · a Sorcerer is circle-selectable and obeys a group order · **a Sorcerer parked next to an enemy for 30 s
    NEVER enters Attack** · a Footman boosted 4 s deals exactly `12 × 1.20 = 14.4` · the boost SURVIVES leaving the ground · a same-seed
    re-run reproduces every log line **byte-identically** · units path normally through the rotated field (no nav regression).
    **TWO JUDGEMENT CALLS THAT ARE YOURS, and both are flagged, not defects:** (a) **the terrain now looks dramatically different** — ~7,500
    of ~15,000 instances are exact rotated copies and every cluster has a visible twin (flag viii); (b) **the boost is permanent, stacking
    and has no removal** — 80 s of uncontested ground time is a permanently 5× army (flag iii; the levers are `MaxPermanentDamageStacks`,
    `BoostTickInterval`, or a per-unit accrual cap).
    **ESCALATION if band 4's near-black OUTLINE disappears over dark world geometry** (documented residual, the fill still carries the
    magnitude at 4.5:1): the levers are thicken the Border padding to 2.5 px, or `DrawAs = RoundedBox` with a 1 px light halo.
    **DO NOT SHIP A BLIND CHANGE — that ordering is the whole lesson of TASK-131.** Route the verdict to the manager.
- names: >
    Exec cheats: `SummonTestUnit`, `SetTestDamageBoost`, `ApplyTestDamage`. Log grep: `[BattlefieldScatter] AncientGroundsPass seed=`.
    Law: CONVENTIONS §5 + "Overhead combatant health bars" (verification-is-law clause). Plan §7 step 2.

#### TASK-378 — [AG-B3] Integration commit (build-master)
- ⚠️ **SPEC AMENDED 2026-08-02 (manager, wave-2 findings) — THE FOUR COMMIT HAZARDS BELOW ARE PART OF THE SPEC, NOT ADVICE.** Sources: `handoffs/TASK-376-buildmaster.md` "🔴 FOR TASK-378", `handoffs/TASK-375-facing-fix.md` §8, `handoffs/TASK-373-artist.md` §12.
- assignee: build-master
- status: **done** (2026-08-02 — ✅ **COMMITTED. Commit A `6e79a24`** (code + tooling + data + pipeline docs, 49 files) **and commit B `80c47e8`** (art assets + `DT_Cards`, 28 files). `main` is **UNPUSHED — 3 commits ahead of `origin/main`: A `6e79a24`, B `80c47e8`, and this hash-recording docs commit** — Jonathan's push, standing law. Compile re-verified GREEN (`Result: Succeeded`, target up to date; judged on output text, not exit code). **All four commit hazards defeated and PROVEN, not assumed:** the auto-staged index was cleared with `git restore --staged` (index only) and every path re-added explicitly · `SK_Sorcerer` ships **post-LOD** (`git diff` on the path EMPTY, index blob `46814f12` == `git hash-object` of the working file, `lod_count == 3`) · `DT_Cards` explicitly staged (Sorcerer row + Footman DeckCount 9 landed) · **`MI_Sorcerer_PBR` UN-STAGED and in NO commit** · **`L_Arena.umap` never saved, never staged, in NO commit** — verified by a name scan of `10f14de..HEAD` returning zero hits for both. **QA WARN-4 RULED** (see handoff): `rig_character.py` numstat is **65/3**, NOT ~1191/1191, and `core.autocrlf=true` means HEAD already stores the file as LF — zero EOL churn landed, so **TASK-383 must NOT renormalise**. Handoff `handoffs/TASK-378-buildmaster.md`.) ← was: backlog
- blocked-by: TASK-377 **PASS**
- parallel-safe: no (EXCLUSIVE Git)
- spec: >
    Final compile (must be GREEN), then commit **on `main`, NO PUSH** (Jonathan's push, standing law).
    **PER-DELIVERABLE COMMITS (the shipped house pattern):** commit A = C++ + `Docs/Data/cards.csv` + `qa/` + `handoffs/` + board +
    CONVENTIONS; commit B = assets (`SM_/SK_/T_/MI_` Sorcerer set + `A_Sorcerer_*` + `T_CardArt_Sorcerer` + raw FBX + concept PNG +
    `M_AncientGround` + `WBP_CombatantHealthBar` + `BP_Unit_Sorcerer` + `DT_Cards`) **+ the tooling DATA file
    `Tools/ArtPipeline/pipeline_manifest.json`** (the `pre_rotate_z_deg: 180.0` + `_pre_rotate_source` entry — data, NOT code, so it
    needs no QA gate; CRLF preserved, 0 bare LF).
    **🔴 HAZARD 0 — THE EDITOR'S GIT PROVIDER AUTO-STAGES SAVED ASSETS. STAGE BY EXPLICIT PATHSPEC, ALWAYS.**
    `Provider=Git` in `Saved/Config/WindowsEditor/SourceControlSettings.ini` means saving an asset in-editor **`git add`s it with nobody
    running `git add`** — **13 files were already staged before you started.** A bare `git commit` (or any `-a` / pathspec-less form)
    **sweeps all of them into commit B, including `MI_Sorcerer_PBR`.** Every `git add` and every `git commit` in this task names its
    paths explicitly. Re-run `git status --porcelain` after staging and BEFORE committing, and read the two-column codes, not the names.
    **🔴 HAZARD 1 — `Content/Characters/SK_Sorcerer.uasset` is `AM` (staged-add AND further modified in the worktree). THE LOD
    REGENERATION SITS IN THE *UNSTAGED* HALF.** Committing the stale index entry **ships pre-LOD bytes** — i.e. it silently reverts
    TASK-372's last acceptance criterion (`lod_count == 3`). **Re-`add` the explicit path, then prove it: `git diff --cached --stat`
    lists `SK_Sorcerer.uasset`, and `git diff -- Content/Characters/SK_Sorcerer.uasset` is EMPTY afterwards.** Check every other `AM`
    file the same way.
    **🔴 HAZARD 2 — `Content/Data/DT_Cards.uasset` is `M` and NOT staged.** Without an explicit `git add` the **entire Sorcerer row +
    the `Footman DeckCount 9` change silently miss the commit** — the batch would ship a card nobody can draw. Verify post-stage that
    `DT_Cards.uasset` is in `git diff --cached --name-only`.
    **🔴 HAZARD 3 — TWO FILES THAT MUST *NOT* LAND.** `Content/Materials/Instances/MI_Sorcerer_PBR.uasset` is **discardable derived
    shader-map state** (it went dirty twice purely from rendering the Sorcerer in a level, was discarded both times, and is
    byte-untouched on disk) — it is currently sitting in the index from an earlier save, so **un-stage it deliberately; do not inherit
    it.** `Content/Maps/L_Arena.umap` **must NEVER be committed and never saved** — it is byte-untouched (mtime 2026-07-29) and the
    one-time save exception is SPENT.
    **📍 REPO STATE, VERIFIED 2026-08-02: `main == origin/main` at `10f14de`, 0 ahead / 0 behind — Jonathan has pushed.** Any board or
    memory text claiming *"main 9 ahead, unpushed"* is **STALE**. Verify the ahead-count yourself before asserting one, and **do not
    push** (standing law).
    **Also carried in from the batch handoffs:** both `Sorcerer.fbx` files are legitimate and distinct — `Content/RawAssets/Sorcerer.fbx`
    (611,068 B, static lane) and `Content/RawAssets/Characters/Sorcerer.fbx` (820,348 B, rigged lane) — **commit both, do not
    "de-duplicate"**; and `rig_character.py`'s CRLF warning is a REAL content diff (`git diff --numstat` 65/3), safe to stage.
    **`git diff --stat` clean on each; nothing foreign; `L_Arena` NEVER saved; `reset --hard` / `clean -fd` BANNED.**
    Record BOTH hashes on the board and in the handoff — **and put the REAL hashes in, not placeholders** (the recorded lesson from
    TASK-355/357).
    Report `handoffs/TASK-378-buildmaster.md`. Post both hashes in 🔧 Build & Git.
- names: >
    Commit on `main`. Law: the hard gate (no commit without a PASS gate) + CONVENTIONS "PIPELINE-DOCS COMMITS ARE ALWAYS PERMITTED"
    (docs-only commits are separately allowed at any time, and are never a substitute for this one).

---

#### TASK-380 — [AG-FU1-QA] QA review of TASK-379 (qa-reviewer)
- assignee: qa-reviewer
- status: **done** (2026-08-02 — VERDICT **PASS**. Report `qa/TASK-380-report.md` ⚠️ **NOTE THE FILENAME** — the spec says `qa/TASK-380.md`; the `-report` file is authoritative, append compile errors THERE. Criteria (a)-(e) all addressed by name; 2 spec problems ruled. ⚠️ **BUILD-MASTER DUTY carried from WARN-2:** `SummonedUnit.h`'s access specifiers are `121:public:` / `484:protected:` / `745:private:` and TASK-379's getters sit at `:467`/`:482` — **re-run the specifier grep after ANY later header edit and before compiling**; a specifier inserted above `:482` makes every call site fail to LINK. Manager follow-up: 1 CONVENTIONS §4 note — `DeckBuilderWidget.cpp:82`'s "for each second" still hardcodes `AAncientGround::BoostTickInterval`, the third §4 lever.)
- blocked-by: TASK-379
- parallel-safe: no (gates TASK-389)
- spec: >
    Pre-compile review. **NAMED CRITERIA:** (a) both getters are in `ASummonedUnit`'s **`public:`** block (the §7 access-level trap that
    already bit this batch once — `ShouldHoldDeathAnim()` at `:582` is `protected:` and is NOT the model) and match the pinned shape
    character-for-character; (b) the `SiegeCheatManager` deletion removed the reflection block, the constant AND the include, with **no
    dangling reference** and no other behavior touched — `SetTestDamageBoost` still routes through `AddPermanentDamageStacks` /
    `ClearPermanentDamageStacks`, **NEVER a raw field write** (CONVENTIONS §6); (c) the `DeckBuilderWidget` line still satisfies the §8
    TRUTH LAW and the format string cannot emit a malformed number (check the `%` escaping and the float→percent arithmetic by hand);
    (d) no other card's description changes by one character; (e) shadowing + complete-type-include scans (the two standing pre-compile laws).
    Report `qa/TASK-380.md`. **PASS/FAIL verdict + report path in ⚙️ Dev & QA.**
- names: >
    Reviews `SummonedUnit.h`, `SiegeCheatManager.{h,cpp}`, `DeckBuilderWidget.cpp`. Report `qa/TASK-380.md`.

