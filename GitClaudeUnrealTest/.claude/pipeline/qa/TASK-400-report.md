# QA Report — TASK-400 (FOLLOW + MINER lane: TASK-395 · 396 · 397 · 398)
Reviewer: qa-reviewer · Date: 2026-08-02 · Pre-compile review, no build attempted.

## VERDICT — PER TASK

| Task | Verdict | Blockers |
|---|---|---|
| **TASK-395** — controller plumbing (`UnitCommand.h`, `SiegePlayerController.{h,cpp}`) | **PASS** | 0 |
| **TASK-396** — unit body / eligibility split / spawn auto-enroll (`SummonedUnit.{h,cpp}`) | **PASS** | 0 |
| **TASK-397** — miner command SEAM (`MinerUnit.{h,cpp}`) | **PASS** | 0 |
| **TASK-398** — miner SEMANTICS (`MinerUnit.{h,cpp}`, `GoldNode.{h,cpp}`, `Castle.{h,cpp}`) | **PASS** | 0 |

**LANE VERDICT: PASS. 0 BLOCKERS · 3 WARN · 6 NIT.** Nothing goes back to a programmer.
Every WARN and NIT is either a doc-only staleness or a named playtest/PIE watch item —
none of them is a code change I am asking for before the compile.

⚠️ **THE LANE MUST BE COMPILED AS ONE UNIT.** By design, no task compiles alone:
TASK-395 calls `ASummonedUnit::IsFollowCommandEligible()` (TASK-396's symbol) and TASK-397
overrides `CanFollowHero`/`CanTakeZoneOrders` (TASK-396's virtuals). That is the pinned-registry
posture (the ANCIENT-GROUNDS precedent), not a defect — build-master must not open a loop over it.

---

## THE FIVE HIGH-RISK CHECKS

### ⚠️ 1. THE ANTI-REPATH COMPOSITION — **THEY COMPOSE. THEY DO NOT FIGHT.** (top target)

Three tasks touched re-path behaviour. The structural answer is that **no unit is ever driven
by two of them**, which is what makes the TASK-280/282 two-driver mill impossible in this lane:

| Contributor | What it is | Who it drives |
|---|---|---|
| TASK-395 `FollowRepathTolerance = 250.f` (`SiegePlayerController.h:337`) | a tunable + contract text | **no code path at all** — it is read, never executed |
| TASK-396 latch + `bStillWalking` (`SummonedUnit.cpp:1866-1877`) | the base follow body's band | base followers, driven ONLY by `UpdateState` (0.25 s state timer) |
| TASK-398 `DriveToPoint` band (`MinerUnit.cpp:850-872`) | the miner's band | miners, driven ONLY by `UpdateMining` (0.25 s poll) |

- **A miner can never run `UpdateStateFollow`.** `StateCheckInterval = 0` (`MinerUnit.cpp:61`) and the
  ONE arming call is `SummonedUnit.cpp:1208` with that rate — `SetTimer` with rate ≤ 0 CLEARS.
  Belt: `ClearAllTimersForObject(this)` (`MinerUnit.cpp:149`). So `UpdateState` executes **exactly once**
  per miner, synchronously at `SummonedUnit.cpp:1207`, and the hoisted follow dispatch is skipped there
  because the enroll was refused (see check 3). ⇒ one driver on the miner's `UPathFollowingComponent`.
- **A base follower has no second driver**: `UpdateStateFollow` `return`s out of `UpdateState` at
  `SummonedUnit.cpp:1317`, so the profile dispatch, the group dispatch, the stance gate and the legacy
  body are all unreachable on a follow tick.
- **The two bands are the same shape**, as TASK-398 claimed — I diffed them by hand:
  skip iff `latch && still-walking && goal-to-goal drift ≤ tolerance`.
  `SummonedUnit.cpp:1869-1874` ≡ `MinerUnit.cpp:851-856`. Drift is measured **goal-to-goal**, never
  goal-to-unit, in both. A miner therefore follows exactly like a Footman.

**THE GOAL-LATCH INVALIDATION — TASK-398's named "subtlest item" — VERIFIED CORRECT, AND VERIFIED
NOT SAFE TO TIDY.** The invariant is:

> `bHasIssuedPointGoal == true` ⇒ the last move request this class issued **was** a `DriveToPoint` request.

It holds because the only *other* issuer, `EnsureWalkingToNode`, clears the latch **unconditionally at
entry, above its own early-outs** (`MinerUnit.cpp:895`), and `StandInPlace` clears it (`:804`). So the
hazard TASK-398 described (an in-flight *node* walk reading as "in flight" while a hero near a stale
latched goal reads as "within band") cannot occur: an in-flight node move implies the latch was false
when it was issued, and only `DriveToPoint` — which itself replaces the move — can set it true again.

**`LeaveMining` deliberately does NOT clear the latch (`MinerUnit.cpp:784-788`) and MUST NOT.** It runs on
*every* poll of a non-mining order; clearing there would make `bWithinBand` false forever and re-issue
`MoveToLocation` at 4 Hz at a moving hero — **that is the mill, exactly**. This is the "plausible tidy-up
that would silently reintroduce the mill" TASK-398 flagged, and it is correctly reasoned. It is documented
in place; **build-master and any future task: do not simplify `LeaveMining`.**

Base-side latch invalidation is complete too: `AssignCommandGroup:1905`, `ClearCommandGroup:1912`,
anchor-null `:1823`, arrival `:1838` — a new order, a re-station, a hero respawn and every idle each
force one fresh path.

**Measured cadence** (hero walk 500 uu/s, sprint 750 uu/s, tick 0.25 s, band 250 uu): re-path every
3rd tick walking, every 3rd tick sprinting ⇒ ≥2 full ticks of uninterrupted path-following per request,
0.75 s wall clock. Stationary hero ⇒ never. That is the mill closed by construction, not by tuning.

### ⚠️ 2. THE LINK SURFACE — **CLEAN. Access-specifier check RE-RUN and CONFIRMED.**

`SummonedUnit.h` has **exactly three** access specifiers: `public: 121` · `protected: 574` · `private: 862`
— TASK-396's claim verified independently. TASK-379's getters are at
`GetPermanentDamageBonusPerStack` **:557** and `GetMaxPermanentDamageStacks` **:572**, both inside
`[121, 574)` ⇒ still in the ORIGINAL `public:` block. **No specifier was inserted above them.**
Every outside call site links. (This was TASK-380's raised risk; it is closed.)

**§7 PINNED REGISTRY — 13/13 character-for-character, access levels included:**

| §7 pin | Shipped at | Access | ✓ |
|---|---|---|---|
| `enum class ESiegeGroupCommandType : uint8 { Hold, Ambush, Follow }` | `UnitCommand.h:76-81` | — | Follow APPENDED; Hold==0/Ambush==1 preserved |
| `int32 EnsureDefaultFollowGroup();` | `SiegePlayerController.h:273` | public (182–558) | ✓ |
| `void EnrollInDefaultFollowGroup(ASummonedUnit* Unit);` | `:294` | public | ✓ |
| `AActor* GetFollowAnchor() const;` | `:316` | public | ✓ |
| `void OnCmdFollowPressed();` | `:1142` | **private** (>964) | ✓ |
| `int32 DefaultFollowGroupId = INDEX_NONE;` | `:1458` | **private** | ✓ |
| `virtual bool CanFollowHero() const;` | `SummonedUnit.h:404` | public | ✓ |
| `virtual bool CanTakeZoneOrders() const;` | `:413` | public | ✓ |
| `bool IsFollowCommandEligible() const;` | `:423` | public | ✓ |
| `bool IsGroupCommandEligible() const;` | `:453` | public, **name+signature KEPT** | ✓ |
| `void UpdateStateFollow(const FSiegeUnitGroup& Group);` | `:860` | **protected** (574–862) | ✓ |
| `virtual bool CanEverAttack/CanFollowHero/CanTakeZoneOrders() const override` | `MinerUnit.h:353/392/413` | public | ✓ |
| `static AGoldNode* FindBestMineInDisc(UWorld*, ETeamId, const FVector&, float, const FVector&)` | `GoldNode.h:181` | public static | ✓ |
| `FVector GetInteriorAnchorLocation() const;` | `Castle.h:169` | public (61–195) | ✓ |

No signature was "improved". `ASiegePlayerController`'s consumed surface is const-correct for the
`const ASiegePlayerController* const` callers: `FindUnitGroup` (`:236`), `GetCurrentCommand` (`:222`),
`HasIssuedCommand` (`:226`), `GetFollowRepathTolerance` (`:350`), `GetFollowAnchor` (`:316`) are all `const`.

### 3. THE MINER ENROLL — **TASK-398's defect analysis is CORRECT, and the two-gate refusal closes it.**

Verified independently:
- `EnrollInDefaultFollowGroup`'s idempotence early-out really is `if (FollowGroup->Members.Contains(WeakUnit)) return;`
  (`SiegePlayerController.cpp:3075-3078`) and it sits **above** `AssignCommandGroup` (`:3098`). So a miner left
  in `Members` with a cleared id would indeed have swallowed **every** future C press — **Follow permanently
  unreachable for miners**, while every "does a spawned miner mine?" test still passed. TASK-397's flagged
  `ClearCommandGroup()` lever was genuinely wrong and TASK-398 was right to refuse it.
- **The refusal holds at TWO independent gates**, as claimed: the unit-side
  `if (!IsFollowCommandEligible()) return;` (`SummonedUnit.cpp:1248`) and the controller-side
  `if (!Unit->IsFollowCommandEligible())` (`SiegePlayerController.cpp:3044`). Either alone suffices;
  the proof does not depend on where TASK-396 put its call.
- **The window is exactly one synchronous stack wide, and I verified the stack.**
  `ASummonedUnit::BeginPlay:228 → LoadStatsAndStart → TryAutoEnrollInFollowGroup (:1204)`.
  `AMinerUnit::BeginPlay` calls `Super::BeginPlay()` (`MinerUnit.cpp:107`) and closes the window on the
  **very next statement** (`:127`), unconditionally, above every early-out. ✓
- **`bFollowOnSpawn = true` genuinely works as the no-compile flip.** Traced: `CanFollowHero()` is true from
  construction ⇒ the enroll succeeds ⇒ the `BeginPlay` tripwire (`:135`) is correctly skipped (`!bFollowOnSpawn`
  is false) so it does NOT undo the flip ⇒ the miner is in the follow group. The shipped `SeekBestMine()` +
  `EnsureWalkingToNode()` at `:168-171` still start a mine walk, which the first `UpdateMining` poll
  (≤0.25 s) redirects via `LeaveMining()` + `DriveToPoint(hero station)`. A ≤0.25 s cosmetic wobble at spawn,
  no leak (`LeaveMining` on a never-arrived miner is a clean no-op), and the flip lands.
  **`EditDefaultsOnly` on `MinerUnit.h:482-483` ⇒ Jonathan can flip it on `BP_Unit_Miner` with no compile.** ✓
- **KNOWN RESIDUAL, correctly disclosed:** the late-`InitUnit` path (`SummonedUnit.cpp:588-591`) reaches
  `LoadStatsAndStart` *after* `AMinerUnit::BeginPlay` closed the window, so a plain-`SpawnActor` miner
  would auto-enroll. Unreachable today (no shipped spawner uses plain `SpawnActor` for a unit) and the fix
  belongs in `AMinerUnit`. Accepted as disclosed.

### 4. THE NO-ATTACK SEAL — **HOLDS, and it is genuinely per-STANCE, not per-class.**

`UpdateStateFollow` (`SummonedUnit.cpp:1758-1884`) — every statement read. Its complete call set is:
`UpdateSupportHealTargeting` (Support only), `FindControllerForTeam`, `GetFollowAnchor`, `EnterIdle`,
`GetAIController`/`GetMoveStatus`, `EnterAdvanceToLocation`. **It reaches `AcquireTarget` / `AcquireEnemyNearPoint`
/ `EnterAttack` on NO path.** `CurrentTarget = nullptr` is forced at `:1789` before anything else runs.

- **ENTERING Follow mid-fight stands the unit down.** `AssignCommandGroup:1897` nulls `CurrentTarget` and
  `:1905` clears the goal latch; on the first follow tick `EnterAdvanceToLocation:2549-2554` (or
  `EnterIdle:2609-2611`) clears `AttackTimerHandle` + `StopAttackLunge()`. The band cannot suppress that
  first tick — a unit in Attack has `StopMovement()`'d (`EnterAttack:2383`), so `bStillWalking` is false,
  **and** the latch was just cleared. Belt: `PerformAttack:2633` re-validates `CurrentTarget`.
- **LEAVING Follow restores a normal attacker.** `ClearAllUnitGroups:3246-3254` clears each member's id ⇒
  next `UpdateState` skips the hoist (`:1306`) ⇒ profile dispatch ⇒ stance gate (`:1385`) ⇒
  `UpdateStateStandardCommanded`. Nothing persists on the unit but two scalars that only the follow body reads.
- **Hero death does not release the seal or the group.** `HandleHeroDied` (`:1018-1051`) cancels the pick and
  the two modes but **does NOT call `ClearAllUnitGroups`** — verified: its only three callers are
  `OnCmdAttackPressed:970`, `OnCmdDefendPressed:1014`, `HandleMatchReset:1418`. So followers keep the group,
  `GetFollowAnchor()` returns null (`:3119`/`:3132`), and the body idles (`:1823-1825`). **Ruling 8 upheld.**

**MINER, all five commands:** four seals, and I cite the three shipped guard points at their **current**
lines (TASK-396 shifted them; TASK-397/398's docs still quote the old ones — NIT-1):
`EnterAttack` **`SummonedUnit.cpp:2371`** (stands down to Idle) · `UpdateStateGrouped` **`:1713`** (acquires
nothing) · `PerformAttack` **`:2628`** (refuses). `AMinerUnit::CanEverAttack() → false` at `MinerUnit.h:353`.
Plus: no acquisition or attack surface is called anywhere in `MinerUnit.cpp`, and
`Group->AttackCenter`/`AttackRadius` are never read (grep-verified) — "the attacking portion is skipped
entirely" is true structurally, not by a zero radius.

**`Profile == None` fall-through, verified independently as the spec demands:** approach (B) was chosen, so
the criterion is "is the timer seal genuinely intact". It is — see check 1. `UpdateState` runs **once** per
miner and that one call takes the legacy body's `Goal = FindNearestEnemyCastle()` + `EnterAdvance` exactly
as it does today, replaced by `EnsureWalkingToNode` (`MinerUnit.cpp:170`) in the same call stack. **Shipped
behaviour, byte-unchanged.** Nothing in this lane re-arms that timer.

### 5. ELIGIBILITY SPLIT — **matches the §3 table exactly, and the Cleric is genuinely reachable.**

`CanFollowHero()` = `Standard || Support` (`SummonedUnit.cpp:1922`); `CanTakeZoneOrders()` = `Standard`
(`:1931`); miner overrides both (`MinerUnit.h:392/413`); Siege excluded from both ⇒ **Ogre and Sapper still
auto-march**, unchanged. The sweep splits at `SiegePlayerController.cpp:2603`
(`bFollowPick ? IsFollowCommandEligible() : IsGroupCommandEligible()`).

**The hoist is Follow-only — diff-reasoned, not taken on the handoff's word.** The block at
`SummonedUnit.cpp:1306-1325` `return`s **only** for `Type == Follow`:
- *Standard unit in a live Hold/Ambush group*: falls through the hoist untouched ⇒ profile dispatch ⇒
  the shipped group dispatch (`:1352`) ⇒ `UpdateStateGrouped`. **Identical**; cost is one extra
  `FindControllerForTeam` + `FindUnitGroup` per tick (both allocation-free, silent).
- *Dead id*: hoist clears it and falls through in the same tick — exactly what `:1370` did. **Identical.**
- *Siege / bot / Red / miner*: `CommandGroupId` is `INDEX_NONE`, so the hoist is one integer compare.
  **Byte-identical.** (Miners can now hold an id but never run `UpdateState` — check 1.)
- `UpdateStateSiege`, `UpdateStateStandardCommanded`, `UpdateStateGrouped`, the legacy body and the stance
  gate: **no line changed.** `UpdateStateSupport` (`:2107-2145`) is behaviourally identical — the four
  heal-targeting statements moved verbatim into `UpdateSupportHealTargeting` (`:2147-2166`) in the same
  order, and `FaceTarget` correctly stayed at the call site (`:2124`).
- **The Cleric works because of the hoist and only because of it**: Support `return`s at `:1337-1341`,
  which is *below* the hoist and *above* the group dispatch. A following Cleric heals via `:1798-1801`
  at the shipped rate on its own `HealTimerHandle`. ✓ And it is **not** granted zone orders. ✓

---

## RULINGS ON EVERY FLAGGED DECISION

| # | Flagged by | Ruling |
|---|---|---|
| **1** | TASK-395 — monotonic enrollment ordinal instead of the spec's literal "by member index" | **UPHELD — DO NOT REVERT.** Verified the premise: `PruneUnitGroups` compacts `Members` (`SiegePlayerController.cpp:3182-3189`), so array indices are recycled; with Follow as the spawn default that group churns on every spawn/death, making **two live followers sharing one station the common case**. The ordinal preserves what the spec actually wants (stable, distinct, once-computed, O(1), one `int32`) and is reset with the group (`:3019`, `:3239`). The literal reading would ship a defect. |
| **2** | TASK-396 — enroll in `LoadStatsAndStart` rather than the `BeginPlay` tail | **UPHELD.** `Profile`'s constructor default is `Standard` and `LoadStatsAndStart` binds it ~50 lines above the call, so the literal placement would enroll an **Ogre** before its row said Siege — a real bug, not a style preference. Also verified the placement keeps TASK-398's contract: `ASummonedUnit::BeginPlay:228` calls `LoadStatsAndStart`, so the enroll is still inside `BeginPlay`'s synchronous stack. And it runs **before** the synchronous `UpdateState` (`:1204` vs `:1207`), so a follower's first decision is already the follow body. |
| **3** | TASK-396 — drafted `ShouldSpawnFollowing()` carve-out then DELETED it | **UPHELD, and this is the best call in the batch.** With `bFollowOnSpawn = true` the base gate would still have refused and **Jonathan's designated flip would have silently done nothing**. Two levers for one decision where one silently wins is precisely the defect class §7 exists to prevent. Verified removed (zero hits) and replaced by a contract comment at `SummonedUnit.h:425-439`. One decision, one owner. |
| **4** | TASK-397 — approach (B), zero base-class delta | **UPHELD, and (A) was genuinely unreachable.** Verified (D1) independently: `SummonedUnit.h`'s `private:` block starts at **:862** and contains `UpdateState`, `UpdateStateGrouped`, `UpdateStateStandardCommanded`, `EnterAdvance`, `EnterAdvanceToLocation`, `EnterIdle`, `GetAIController`, `FindOwnCastle`, `State`, `CommandGroupId`, `GroupStationOffset`. (A) required a promote-to-`protected` edit to a file this batch assigns exclusively to TASK-396 **and modified in the tree at the time**. (B) is also the reason there is only ever ONE driver on the miner's movement component — see check 1. |
| **5** | TASK-398 — **zero** lines of `SummonedUnit.*` / `SiegePlayerController.*` / `UnitCommand.h` | **ACCEPTED, with independent corroboration.** QA has no Git tool, so I did not re-run `git diff --numstat`; instead: (a) every §7 symbol `MinerUnit.cpp` consumes exists at the pinned access level in the landed base files; (b) the **stale** `AMinerUnit::ResolveStationOffset` reference surviving at `SummonedUnit.h:364` is exactly the artefact you would expect if TASK-398 deleted its own duplicate and never touched the base header — a task that *had* edited `SummonedUnit.h` would have fixed its own comment. **build-master: run `git diff --numstat` at TASK-401 as the mechanical confirmation.** |
| **6** | TASK-398 → TASK-402 — reinforcement miners inherit the **stance** (a miner played after **E** hides in the castle instead of mining until **T**) | **RULED ACCEPTABLE TO SHIP — this is not a defect and must not be "fixed" here.** It is the pre-existing Shield-Wall latch (`bHasIssuedCommand`, `SiegePlayerController.h:226`) read live by every unit, now reaching a new unit type. A miner-only carve-out would be a **second silent lever on the same decision** — the exact class of defect ruling 3 just eliminated. Consistency with every other unit beats convenience. **Conditions:** (i) it stays a named item on TASK-402 (it already is, board line under TASK-398), and Jonathan meets it at playtest; (ii) if he dislikes it, the lever is CONVENTIONS §2's "new spawns inherit the last issued order" toggle applied uniformly, **not** a miner special case. The economic sting is real (E is a defensive panic button and it silently zeroes mining income) — that is exactly why it belongs on a human gate rather than in a programmer's judgement. |
| **7** | TASK-398 — `ACastle::FindNearestCastleForTeam` is a NEW public static not in §7 | **ACCEPTED as additive.** It alters no pinned signature, and the `names:`-block alternative (`ASummonedUnit::FindOwnCastle`) is **private** at `SummonedUnit.h` in another task's file — genuinely unreachable. The mirror is faithful: same-team, `IsValid`, **skips destroyed** (`Castle.cpp:672`), nearest wins, strict-`<` first-found tiebreak. The squared-2D vs bounds-aware metric deviation cannot change the winner with exactly one standing own castle. **📌 MANAGER: add it to §7 for the record.** |
| **8** | TASK-398 — `CanFollowHero()` returns a member rather than a literal `true` | **ACCEPTED.** §7 pins **signatures**, not bodies. `virtual bool CanFollowHero() const override` is character-for-character; only the value is data-driven, and that value **is** the §5 semantic this task owns. |
| **9** | TASK-396 — the `bStillWalking` term, beyond the letter of the spec | **KEEP IT.** Verified it can only fire when path following is Idle, i.e. when there is **no in-flight request to restart** — so it cannot re-open the mill. Without it a follower that completes or fails its leg inside the band stalls permanently up to a full 250 uu short of station on a stationary hero. It is the same discipline `EnterAdvance`/`EnterAdvanceToLocation` already use (`\|\| GetMoveStatus() == Idle`, `:2583`). Deleting it would be a regression. |
| **10** | TASK-395 — the two tunables are `public` UPROPERTYs, plus BlueprintPure mirrors | **ACCEPTED — and do NOT "tidy" them after the compile.** Both access shapes are consumed by the parallel lanes as written: `SummonedUnit.cpp:1866` reads the **member**, `MinerUnit.cpp:646` reads the **getter**. Shipping both is what made the blind parallel write safe. Moving them to `protected` now would break `SummonedUnit.cpp`. |

---

## FINDINGS

### BLOCKERS — none.

### WARN

- **[WARN-1] `SummonedUnit.cpp:1836-1841` + `:1883` — arrival-idle / re-advance stop-go at a MOVING anchor.**
  The body idles inside the shipped 150 uu `HoldArrivalTolerance` and re-advances once outside it. With a
  *stationary* anchor (the shipped Hold tier-3 case) that fires once; with a **moving** hero a follower that
  is faster than the hero will cross that boundary repeatedly, producing a ~0.5–0.75 s stop/go hitch.
  **This is NOT the TASK-280/282 mill** (every request produces motion; nothing cancels an in-flight request)
  and the 150 uu idle test is **mandated by CONVENTIONS §4**, so I am not failing it. But it is the single
  most likely source of a "stutter" report at playtest.
  *Route:* TASK-401 check (b) "no visible per-tick re-path stutter" and TASK-402's "no stutter, no mill-in-place"
  row must be judged against this specifically. *If it reads badly, the fix is hysteresis, not the band:*
  idle at 150 uu but do not re-advance until `150 + FollowRepathTolerance`, or skip `EnterIdle` on a tick
  where the station itself moved. **Do not lower `FollowRepathTolerance` in response — that is the mill floor.**

- **[WARN-2] `MinerUnit.cpp:867-883` — a stranded Defend miner re-issues a failed `MoveToLocation` at 4 Hz forever.**
  With `RepathTolerance = 0` and a `Failed` result, path following stays `Idle`, so `bMoveInFlight` is false and
  the band never suppresses. The Warning is one-shot (`bWarnedPointMoveFailed`), so the retry storm is **invisible
  in the log**. CONVENTIONS §5 explicitly accepts a gate-stranded miner and the base `EnterAdvanceToLocation:2583`
  has the same `\|\| Idle` shape, so this is precedented and not a blocker — but it is one pathfinding query per
  0.25 s per stranded miner (cap 6).
  *Route:* TASK-401 must read back the `DEFEND — hiding inside …; interior anchor resolves to …` log line
  (`MinerUnit.cpp:725-728`) and confirm no `MoveToLocation toward … failed` Warning appears. If the interior
  anchor does not nav-project, the fix is the **tunable** `InteriorAnchorRelativeLocation` (`Castle.h:373`), not code.

- **[WARN-3] `SummonedUnit.h:363-371` — a shipped doc comment describes a function that no longer exists and a
  state that is no longer true.** It says `GetGroupStationOffset()` is "CURRENTLY UNUSED, ON PURPOSE" and that
  `AMinerUnit::ResolveStationOffset` re-derives its own sunflower. TASK-398 deleted `ResolveStationOffset` and
  **does** consume the getter (`MinerUnit.cpp:605`, `:640`). Doc-only, zero compile/runtime effect — but it is
  load-bearing documentation about a cross-task contract, and it currently misleads.
  *Route:* fold into the next `SummonedUnit.h` touch. Not worth a QA loop now.

### NIT

- **[NIT-1] `MinerUnit.h:146-148` and `:339`** cite the pre-TASK-396 guard-point lines `2050 / 1583 / 2307`.
  Current: **`2371 / 1713 / 2628`**. TASK-397 flagged the drift in advance and TASK-398 correctly switched to
  citing function names. Update on the next touch.
- **[NIT-2] `handoffs/TASK-398-programmer.md` §4** states "the auto-enroll runs *after*" the synchronous
  `UpdateState`. It runs **before** (`SummonedUnit.cpp:1204` vs `:1207`). The conclusion is unaffected (the miner
  is refused, so `CommandGroupId` is `INDEX_NONE` either way), but the sentence must not be quoted downstream.
- **[NIT-3] `MinerUnit.cpp:498-519` wait-mode branch does not `StopMovement`.** A `MineInDisc` miner that queues
  at an enemy-claimed mine while a `DriveToPoint(Station)` request is still in flight can be pulled a short way
  off the ring for one or two polls before `EnsureWalkingToNode` re-issues `MoveToActor`. Self-correcting,
  cosmetic, and only reachable in a narrow configuration (station move in flight + the finder returning a
  wait target the miner is already inside 150 uu of).
- **[NIT-4] `SiegePlayerController.cpp:2977-2981` — "FOLLOW set: N" counts already-following units.** TASK-395
  flagged it; **ruled correct** — "5 units are following you" is the honest readout of the command issued.
  Recorded so Jonathan is not surprised.
- **[NIT-5] `SiegePlayerController.cpp:3108` — `PruneUnitGroups()` now runs on EVERY unit spawn** (via the enroll),
  not just on picks and the 1 s timer. O(groups × members) with a handful of each — trivial, but new traffic on a
  hot path. Recorded for the perf capstone.
- **[NIT-6] `SiegePlayerController.cpp:3169` — `FindUnitGroupMutable` uses `const_cast`.** Reviewed and accepted:
  the pointee genuinely aliases this controller's own non-const `UnitGroups`, so it is well-defined, and it keeps
  exactly one search implementation. The "never cache it" comment is correct and necessary.

---

## THE NAMED CRITERIA (board spec, 1–11)

| # | Criterion | Result |
|---|---|---|
| 1 | §7 registry character-for-character, access included | **PASS** — 13/13, table above |
| 2 | A following unit can NEVER attack; seal is per-BODY | **PASS** — check 4 |
| 3 | Miner can never attack under any of the five; `Profile == None` fall-through verified independently | **PASS** — guards at `2371`/`1713`/`2628`; (B) chosen and the timer seal is genuinely intact |
| 4 | Anti-repath IMPLEMENTED, not described | **PASS** — `SummonedUnit.cpp:1866-1877` gates the `EnterAdvanceToLocation` call; `MinerUnit.cpp:850-872` gates `MoveToLocation`. No unconditional per-tick re-path exists on any path |
| 5 | Anchor resolved LIVE, never cached; hero-death idles | **PASS** — `GetFollowAnchor` re-reads `GetPawn()` (`:3118`), returns null on missing/pending-kill/`IsDead()`; body idles at `:1823-1825`; `HandleHeroDied` does NOT release groups |
| 6 | Dispatch hoist is Follow-ONLY; everything else byte-identical | **PASS** — diff-reasoned per branch, check 5 |
| 7 | `GetFirstPlayerController()` appears nowhere | **PASS** — grep over the whole `Siegebound/` tree: 8 hits, **all comments naming it as BANNED**. Every resolve is `FindControllerForTeam` |
| 8 | M8 declaration duty discharged in EVERY handoff | **PASS** — TASK-395 §"M8 DECLARATION DUTY", TASK-396 §"M8 DECLARATION DUTY", TASK-397 §7, TASK-398 §9. All four state it explicitly. Verified: **no new replicated property, no new replicated class, no new tier.** The four new `UPROPERTY`s (`FollowRepathTolerance`, `FollowFormationRadius`, `bFollowOnSpawn`, `InteriorAnchorRelativeLocation`) are all `EditDefaultsOnly` design-time data — the shipped `GateBlocker*` precedent |
| 9 | Miner bookkeeping untouched; `FindBestMineFor` unmodified | **PASS** — `EndPlay(Destroyed)`, `FreezeAI`, `EndMineTenure`, `NotifyMineDepleted`, `TryRegisterArrivedMiner`, the `AddMinerIncome`/`RemoveMinerIncome` pairing and every latch read as shipped. `FindBestMineFor` (`GoldNode.cpp:187-236`) is unmodified; `FindBestMineInDisc` (`:238-315`) is a deliberate write-out beside it. **`bOutOfOrderDisc` and the finder's gate AGREE** — both 2D, both mine-location-to-centre, both boundary-inclusive (`MinerUnit.cpp:363-365` `> Square(R)` ⇔ `GoldNode.cpp:279` `> RadiusSq`), so the oscillation TASK-398 flagged cannot occur. `LeaveMining` correctly runs mine-side release **before** `EndMineTenure` (the `EndPlay(Destroyed)` order) |
| 10 | House compile traps | **PASS** — (a) regex scan for `*/` inside doc comments across all 8 files: **zero**; (b) every `UE_LOG`/`Printf` format is a string literal — the one runtime ternary (`SiegePlayerController.cpp:3218`) is an **argument**, two `const TCHAR[]` literals decaying to `const TCHAR*` in the conditional, well-formed, and `TCheckedFormatString` constrains only the format parameter; (c) **no shadowing** — every new member name (`LastFollowGoalLocation`, `bHasFollowGoalLocation`, `LastIssuedPointGoal`, `bHasIssuedPointGoal`, `bSpawnFollowEnrollWindowClosed`, `bFollowOnSpawn`, `InteriorAnchorRelativeLocation`, the four log latches) resolves to exactly one class; (d) **complete-type include law** — `MinerUnit.cpp` adds `Castle.h:12`, has `SiegePlayerController.h:16`, `UnitCommand.h:18`, `PathFollowingComponent.h:11`, `AIController.h:5`; `Castle.cpp` adds `EngineUtils.h:12`; `SummonedUnit.cpp` already had `AIController.h:5`, `PathFollowingComponent.h:25`, `SiegePlayerController.h:41`, `UnitCommand.h:42`; `SiegePlayerController.cpp` has `HeroCharacter.h:31` (for the `Cast<AHeroCharacter>` in `GetFollowAnchor`), `SummonedUnit.h:40`, `EngineUtils.h:17`. **No task added an include it did not need, and none omitted one it did.** `AAIController::GetMoveStatus()` is `const` in UE 5.8, so both `const AAIController*` calls compile |
| 11 | Flagged-decision hygiene | **PASS** — the miner flip line exists, is `EditDefaultsOnly`, and is named in both the code (`MinerUnit.h:459-483`) and the handoff; a following Cleric heals (`SummonedUnit.cpp:1798-1801`); the Cleric is **not** granted zone orders (`CanTakeZoneOrders()` = Standard only) |

**Deprecated / removed UE 5.8 APIs: none.** Everything used is current
(`FTimerManager`, `AAIController::MoveToLocation`/`MoveToActor`, `UPathFollowingComponent::GetMoveGoal`,
`UNavigationSystemV1::ProjectPointToNavigation`, `TActorIterator`, `UEnhancedInputComponent::BindAction`,
`TSoftObjectPtr`, `TWeakObjectPtr`). No raw `UObject*` UPROPERTY was introduced.

---

## NOTES FOR BUILD-MASTER (TASK-401)

1. **Compile the whole lane together.** Nothing compiles alone by design — do not open a loop over an
   unresolved `IsFollowCommandEligible` / `CanFollowHero` if you try a partial build.
2. **Run `git diff --numstat`** and confirm TASK-398's zero-delta claim on
   `SummonedUnit.*` / `SiegePlayerController.*` / `UnitCommand.h`. QA corroborated it structurally but has no Git tool.
3. **The castle interior anchor's LIVE nav-projection is TASK-401's duty, explicitly NOT claimed by TASK-398**
   and correctly disclosed as an honest gap. The probe is already built: `MinerUnit.cpp:722-729` logs the
   resolved world point **once per miner**, so read it back from the PIE log — no editor probe needed.
   Expected: **Blue (−25000, 0, 0) · Red (+25000, 0, 0)**. Also watch for
   `MoveToLocation toward … failed` (WARN-2). A gate-stranded miner is ACCEPTABLE per §5; a crash is not.
4. **Four named PIE watch items** beyond the board's (a)–(i): WARN-1 (follow stop-go hitch → check (b)),
   WARN-2 (Defend nav failure), the `bFollowOnSpawn = true` flip path (only if Jonathan asks at TASK-402),
   and the `MineInDisc` wait-mode wobble (NIT-3, cosmetic).
5. **Do NOT "tidy" any of these after a green compile** — each is load-bearing and reasoned:
   `LeaveMining` must NOT clear `bHasIssuedPointGoal`; the `bStillWalking` term stays;
   `FollowRepathTolerance`/`FollowFormationRadius` stay `public`; the enrollment ordinal stays monotonic.
6. **TASK-399 (`IA_CmdFollow` + the `C` mapping) is still outstanding.** Until it lands, C is inert by design
   (`SiegePlayerController.cpp:363`, `:432-435`) — one log line, no crash. Board check (a) needs a **real key
   press**; if the desktop is locked, leave it OPEN for TASK-402 rather than claiming it.

## NOTES FOR THE MANAGER

- **📌 Pin `static ACastle* ACastle::FindNearestCastleForTeam(UWorld*, ETeamId, const FVector&)` (public) in
  CONVENTIONS §7** — shipped, additive, ruled acceptable (ruling 7).
- **📌 Pin `FVector ASummonedUnit::GetGroupStationOffset() const` (public)** — TASK-396 asked; it is now
  consumed by `MinerUnit.cpp:605`/`:640`, so it is live API, not dead code.
- **Ruling 6 (reinforcement miners inherit the stance) needs to reach Jonathan as a NAMED playtest item**, with
  the economic consequence spelled out: pressing **E** silently zeroes mining income until **T**.

---

## BOARD STATUS (applied by the orchestrator — QA has no Edit tool)

TASK-395 · TASK-396 · TASK-397 · TASK-398 ⇒ **`qa-passed`** · TASK-400 ⇒ **`done`**.
TASK-401 is unblocked on the QA side (still gated on TASK-399 for its input-asset checks).

---

# 🔧 BUILD-MASTER ADDENDUM — TASK-401 compile attempt (2026-08-02)

## ⚠️ THIS IS **NOT** A LANE FAILURE. NO QA LOOP WAS CONSUMED.

`Result: Failed (OtherCompilationError)` — but **zero** of the diagnostics belong to this lane.
**TASK-395 · 396 · 397 · 398 remain `qa-passed` and were NOT set `qa-failed`.** Nothing goes back
to a programmer on account of this build. Recorded here only as the audit trail TASK-403 will want.

### The complete diagnostic set (3 lines, all foreign)

```
SiegeAssistantCommand.cpp(58) : error C4172: returning address of local variable or temporary : $S1837
   while compiling `anonymous namespace'::FindFieldExact
SiegeAssistantSnapshot.cpp(338,58): error C2228: left of '.LoadSynchronous' must have class/struct/union
SiegeAssistantSnapshot.cpp(338,31): error C2737: 'CardTable': const object must be initialized
```

Both files are **untracked, never committed**, and belong to the **Siege Assistant / Llama batch
(TASK-416 · TASK-417)** whose own compile gate **TASK-420 has not run**. A parallel agent was
**actively writing them during the build** (`SiegeAssistantSnapshot.cpp` grew 33,245 → 33,799 B at
14:02:36; board marker `▶ NOW TASK-417`). They were therefore **left untouched** — not parked, not
renamed. They share the UBT module with this lane, which is how they broke a gate they have nothing
to do with.

### THE LANE COMPILED CLEAN

All 7 lane TUs + all 4 unity blobs compiled with **no diagnostic**: `GoldNode.cpp` [2/18],
`MinerUnit.cpp` [4/18], `Castle.cpp` [6/18], `DeckBuilderWidget.cpp` [7/18], `SiegeCheatManager.cpp`
[9/18], `SiegePlayerController.cpp` [10/18], `SummonedUnit.cpp` [14/18].
A grep of the full log for every lane file **and** every §7 pinned symbol, intersected with
`error|warning|unresolved`, returns **zero rows** ⇒ **no cross-task registry mismatch; §7 held.**

⚠️ **LINK NOT REACHED** (UBT aborted at compile), so the link surface — **WARN-2's** concern —
remains unproven. The *static* half was re-run and **PASSES**: `SummonedUnit.h` has exactly three
specifiers (`public:` **121** · `protected:` **574** · `private:` **862**) with TASK-379's getters at
**:557** / **:572**, both inside the original `public:` block.

### Ruling 5 — `git diff --numstat`, the confirmation QA asked for

- **TASK-398's six owned files match its claimed table 6/6 exactly**: `Castle.h` 73/0 · `Castle.cpp` 55/0 ·
  `GoldNode.h` 28/0 · `GoldNode.cpp` 79/0 · `MinerUnit.h` 429/0 · `MinerUnit.cpp` 549/12.
- **Honest limit:** `HEAD` (`5fa10eb`) predates all four tasks with no intermediate commit, so numstat
  **cannot attribute lines per-task** on the shared base files.
- **So the decisive check was a content check:** every added/removed *executable* line in
  `SummonedUnit.{h,cpp}` / `SiegePlayerController.{h,cpp}` / `UnitCommand.h` was grepped for miner
  implementation. **Every hit is a COMMENT** (`+\t//`, `+\t *`, `+\t//~`) — **zero miner executable code**
  in the base files. **Ruling 5 stands, now mechanically backed.**

### PIE: NOT RUN — zero items claimed

No binary contains the lane (compile red), **and** the workstation is **locked** (session idle 3d 15:28,
`GetForegroundWindow()` = 0, desktop capture black), so the real key presses checks (a)–(i) require are
impossible; MCP exposes no function-invoke tool, so `SummonTestUnit` has no route either.
**(a)–(i) all left OPEN for TASK-402, per the board's own instruction.**
`L_Arena` was never saved — **535,522 B / 7/29/2026 3:53:38 AM, unchanged.**
Full detail: `handoffs/TASK-401-buildmaster.md`.
