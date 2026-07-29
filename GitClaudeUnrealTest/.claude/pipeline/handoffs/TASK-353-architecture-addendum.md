# TASK-353 Architecture Addendum — CASTLE-3X gating vs networking, resolved against the SHIPPED code

- **Author:** gameplay-programmer · 2026-07-29 · STEP 0 of TASK-356 (closure ruling 2026-07-29: this task writes the owed addendum; TASK-356-QA verifies it and 356's 349-lane compliance with it).
- **Evidence base:** the SHIPPED CASTLE-3X code — commit `949c252` as landed at HEAD `08c4a24` (working tree clean for all cited files; `git status` verified). Every claim below is file:line against that tree, read this session. Engine claims verified against the installed UE 5.8 source, never memory.
- **Scope:** resolves TASK-353 §7 items 1–3 (+ the residuals §7 listed) and pins the §3.1 member names. TASK-356's own edits MUST comply with the verdicts here; each verdict states what 356 does (or must not do).

---

## Verdict summary (one line each)

| # | §7 hazard | Verdict |
|---|---|---|
| 1 | Client-proxy Team-at-BeginPlay ordering vs the landed stamping sites | UNITS: no P1 hazard (no unit proxies exist); the P2 hook is named and ready. HERO: ordering is safe on the normal path but not guaranteed on every path ⇒ **an `OnRep_Team` capsule re-stamp hook IS needed — TASK-356 implements it** |
| 2 | Client-side gate-collision truth for the hero | HOLDS by construction: the landed blocker config is NOT authority-gated, castle `Team` is level-authored, channels are ini-shipped — identical matrices on both machines. **356 must not authority-gate `ConfigureTeamGating`** (compliance rule) |
| 3 | Pin the crumble member/apply names for §3.1 | `int32 CrumbleStage` (Castle.h:399, private+AllowPrivateAccess) / `ApplyCrumbleStage(int32)` (Castle.cpp:440) — **absolute-apply CONFIRMED**; stage 0 (pristine) is `ApplyTeamVisuals()`, NOT ApplyCrumbleStage — the OnRep must branch |
| 4 | Nav filters on clients (D1) | Safe by construction in P1 — enforced by the fact that NO unit exists client-side, not by a guard; P2 obligation recorded |
| 5 | Crumble swaps vs gating; ini | Swap touches ONLY CastleMesh mesh/material (verified) — cannot strip the gating components; ini carries the channels + zero net config (D13 holds) |

---

## 1. Client-proxy Team-at-BeginPlay ordering (§7.1)

**The landed stamping sites (shipped code):**
- **Units:** `ASummonedUnit::ApplyTeamGatingProfile()` (SummonedUnit.cpp:243–264; capsule `SetCollisionObjectType(SiegeTeamObjectChannel(Team))` at :254, nav-filter push via `Cast<ASiegeUnitAIController>(GetController())` at :258–262). Called from THREE team-set sites: `BeginPlay` (:226), `PossessedBy` (:241 — the override at SummonedUnit.h:363), and the post-BeginPlay `InitUnit` team branch (:569). `AIControllerClass = ASiegeUnitAIController` (:106).
- **Hero:** `AHeroCharacter::BeginPlay` stamps the capsule ONCE (HeroCharacter.cpp:83–96; the stamp at :94, reading `GetTeamId()` → `Team`, HeroCharacter.h:336 — default Blue, NEVER assigned at HEAD; the audit's §1b#4 stands in the shipped code). There is NO PossessedBy override and NO OnRep on the hero today.

**Units — P1 verdict: no hazard, because no unit proxy can exist.** `ASummonedUnit` never sets `bReplicates` (grep: zero replication surface in SummonedUnit.{h,cpp} at `949c252`), and P1 deliberately does not add it (doc §3/§6: the two poll-site edits ONLY). No client-side unit instance ⇒ no client-side BeginPlay ⇒ no mis-stamp. **P2 obligation (named for the P2 decomposition):** when unit replication lands, `Team` must ride the initial bunch (InitialOnly/spawn-data per doc §2 matrix) AND `OnRep_Team → ApplyTeamGatingProfile()` is the ready-made re-stamp seam — the function is already idempotent and degrades correctly on proxies: `GetController()` is null on a client proxy (AIControllers never replicate) ⇒ the `Cast<ASiegeUnitAIController>` half skips ⇒ capsule-only stamp, which is exactly right client-side.

**Hero — P1 verdict: the OnRep_Team re-stamp hook IS needed; TASK-356 implements it.**
- The hero replicates in P1 without any 356 edit: `APawn`'s constructor sets `bReplicates = true` (engine: Pawn.cpp:86) — both heroes exist as proxies on both machines the moment a P1 session runs.
- Normal-path ordering is SAFE: 356 assigns `Team` in `AHeroCharacter::PossessedBy` (server-side; doc §2.3). `AGameModeBase::RestartPlayer` spawns the pawn and possesses it in the same server frame, and the NetDriver replicates at frame end — so the pawn's FIRST bunch already carries the correct Team, and the client proxy's BeginPlay (which for a dynamically spawned replicated actor runs AFTER the initial bunch applies — engine spawn contract) stamps the right channel at :94. Note the SERVER-side ordering quirk: `SpawnDefaultPawnFor` runs BeginPlay (stamp with default Blue) BEFORE `Possess` — so the server-side stamp must be REPEATED after the possess-time team assign. 356's PossessedBy therefore re-stamps the capsule after assigning Team (idempotent).
- Why the hook is still needed: any path where the proxy's BeginPlay beats the correct Team value (PIE login-edge timing is engine-internal per the doc §3.2 divergence note; a future repossess/team correction; hot-join races) leaves the CLIENT hero carrying the WRONG object channel — and because CharacterMovement predicts locally, that is a live gate-truth defect (walks into the enemy gate locally → rubber-band; §7.2's exact failure). The doc reserved the `OnRep_Team` seam for exactly this (§2.3 "P2 hangs … CASTLE-3X capsule-channel stamping off it — §7"; §7.1 "an OnRep_Team hook may be needed there").
- **Resolution (implemented by 356, extends the doc's "log-only" P1 OnRep with the hook §7 reserved):** `AHeroCharacter::OnRep_Team()` = log + capsule re-stamp (`SetCollisionObjectType(SiegeTeamObjectChannel(GetTeamId()))`). Idempotent, cheap, closes the ordering class unconditionally: whichever of {proxy BeginPlay, Team rep} lands second, the channel ends correct.

## 2. Client-side gate-collision truth for the hero (§7.2)

**Verdict: HOLDS on the shipped code, by construction — provided 356 does not break it (compliance rules below).** Evidence:
1. **The blocker config is NOT authority-gated.** `ACastle::BeginPlay` (Castle.cpp:143–158) unconditionally calls `ConfigureTeamGating()` (:160–230): blocker sized/positioned from the tunables, object type = own channel, `SetCollisionResponseToAllChannels(ECR_Ignore)` + `Block` ONLY the enemy channel (:182), armed `QueryAndPhysics` (:188). No `HasAuthority()` anywhere in that path (verified by read) — it runs identically on the client's level-placed castle instances.
2. **Every input to that config is identical on both machines:** castle `Team` is a level-authored per-instance EditAnywhere property (Castle.h:229) baked into the level package both machines load; the gate tunables are C++ CDO defaults (Castle.h:277/:288 — the loop-2 baked values, PIE-verified); the channels are `DefaultEngine.ini` config shipped with the game (ini:306–307). Same matrix, both machines. (§3.1's `Team` COND_InitialOnly replication is belt-and-braces on top of an already-identical value — correct as designed.)
3. **Hero-side:** the capsule stamp (HeroCharacter.cpp:94) runs on every machine's copy of every hero (BeginPlay is not authority-gated either) — with §1's OnRep re-stamp guaranteeing the value it stamps is the replicated truth. Client prediction therefore collides exactly as the server does: own gate pass, enemy gate block, no rubber-band.
4. **Cost of client-side nav legs (harmless):** `PostInitializeComponents` area select (Castle.cpp:125–141), the ctor `ForceNavigationRelevancy(true)` (:96), and the B4 BeginPlay `RefreshNavigationModifiers()` (:228) also run client-side → the client rebuilds its castle-bounds nav tiles once at boot. Nothing consumes client navmesh in P1 (no client AI — §3 below); pure idle cost.

**TASK-356 compliance rules derived here (QA-checkable):**
- `ACastle::BeginPlay`/`ConfigureTeamGating`/`PostInitializeComponents` must stay authority-UNgated. 356's Castle authority guards go ONLY on the mutation surfaces the doc names (`TakeDamage`, `HealOverTime`, `ResetCastle`).
- `OnRep_Destroyed → ApplyDestroyedState` toggles actor collision — on the client this legitimately drops/restores the blocker with the castle (a fallen castle gates nothing), mirroring the server's `HandleDestroyed`/`ResetCastle` collision cycle (:377–409/:482+). Symmetric on both machines via the same OnRep — no divergence.
- `OnRep_CrumbleStage → ApplyCrumbleStage` (client) must not disturb the gating — it cannot (see §3/§5).

## 3. The crumble members, pinned for §3.1 (§7.3 / task item c)

| §3.1 table item | Shipped name | Evidence | Networked note |
|---|---|---|---|
| Crumble stage member | `int32 CrumbleStage = 0` | Castle.h:399 — **private**, `UPROPERTY(VisibleInstanceOnly, Transient, meta=(AllowPrivateAccess))` | Private is fine for `ReplicatedUsing` (handler private too). Monotonic 0→3 via `UpdateCrumbleStages` (Castle.cpp:411–438, called only from TakeDamage — authority-gated by 356); reset to 0 only in `ResetCastle` (:482+) |
| Stage apply | `void ApplyCrumbleStage(int32 Stage)` | Castle.cpp:440–480 | **ABSOLUTE — confirmed:** composes `/Game/Meshes/SM_Castle_Crumble0%d` + `/Game/Materials/MI_Castle_Crumble0%d` directly from `Stage` and swaps mesh+material outright; no incremental dependency. Join-in-progress receives the final stage and one call lands the right look. Guards `Stage < 1 || Stage > 3` (:442) — **stage 0 is NOT its domain** |
| Pristine restore (stage 0) | `ApplyTeamVisuals()` | Castle.cpp:482+ (ResetCastle path) | **The OnRep must branch:** `CrumbleStage > 0 → ApplyCrumbleStage(CrumbleStage)`, `== 0 → ApplyTeamVisuals()` — otherwise a client never un-crumbles on Play-Again. 356 implements exactly this |
| Destroyed latch | `bool bDestroyed = false` | Castle.h:395 (private, AllowPrivateAccess) | OnRep name per the ratified bool law: `OnRep_Destroyed`. The visual/collision half of HandleDestroyed (:377–409: hide, collision off, HP-bar hide) + ResetCastle's restore is the `ApplyDestroyedState` refactor seam; the server-only half (StopHealOverTime, the S_CastleDestroyed sting, `OnCastleDestroyed` broadcast) stays out of the OnRep — the doc's "never broadcasts OnCastleDestroyed" holds |
| Client side effects of ApplyCrumbleStage | debris VFX + LogGitClaudeUnrealTest line + the B3 leg-3 `RefreshNavigationModifiers` fence (Castle.cpp:466) | read | All client-legal: debris = desirable cosmetic (free P2-grade juice in P1), nav refresh = idle cost (no client AI), log = noise-free (once per stage). NO gameplay mutation anywhere in the function |

## 4. Nav filters / AI on clients (§7 item 3, D1)

- In P1 no unit replicates ⇒ no client-side unit ⇒ no client-side `ASiegeUnitAIController` (AIControllers never replicate; `AutoPossessAI` spawning only happens where the pawn exists — server) ⇒ no client MoveTo ⇒ the filter/area lane simply has no client consumer. The client's castle still MARKS its local navmesh (legs 1/2/4 run client-side, §2.4) — inert.
- **P2 obligation (recorded):** when units replicate, client proxies must NEVER run the AI (the audit §3.3 authority-gate class of change) — otherwise a proxy-side MoveTo would path on client navmesh and fight replicated movement. The B2 march resolver (`ResolveStructureMarchPoint`, SummonedUnit.cpp) reads the possessing controller's filter — on proxies there is no controller, so it degrades to the legacy path; acceptable only because P2 will authority-gate `UpdateState` wholesale.

## 5. Residuals swept (§7 items 4–5)

- **Crumble/mesh swaps cannot strip the gating** — verified in the shipped code, not just claimed: `ApplyCrumbleStage` touches ONLY `CastleMesh` (SetStaticMesh :455, SetMaterial :477) plus the nav re-assert; `GateBlockerVolume` and `InteriorNavModifier` are sibling ACTOR components created in the ctor (Castle.cpp:67–96) and never referenced by the swap. Replication-friendly exactly as CASTLE-3X designed.
- **`DefaultEngine.ini`:** carries the two team channels (:306–307) and `RuntimeGeneration=Dynamic` (:271); ZERO netcode config. D13 confirmed against the landed file — P1 needs no ini edit, and 356 makes none (parallel law d).
- **`SiegeNavAreas.{h,cpp}` (349's new files):** pure types (areas/filters/channel aliases) + the minimal `ASiegeUnitAIController`. Nothing in them is machine-specific or replication-relevant; 356 does not touch them (doc §6 "explicitly NOT touched").
- **Line-ref re-verification of the doc's 349-lane refs (§7's last duty):** the doc's HEAD-`8a903c7` refs have shifted: hero melee `ApplyDamage` now HeroCharacter.cpp:332 (was :317); the two unit poll sites now SummonedUnit.cpp:1172/:1199 (were :1118/:1145); the PC placement-Blue literals shifted by the TASK-349 plinth edits (e.g. `IsPointInOwnSpawnBox` Blue filter now SiegePlayerController.cpp:3237–3247). The DESIGN is unaffected — same sites, same shapes; TASK-356 uses the current refs above.

---

**Bottom line for TASK-356:** one genuine code consequence beyond the signed doc's letter — `AHeroCharacter::OnRep_Team` is a re-stamp hook, not log-only (the doc's own reserved seam, resolved YES) — plus two compliance fences (no authority gate on the castle's gating config path; the crumble OnRep branches to `ApplyTeamVisuals` at stage 0). Everything else in the signed P1 design lands against the shipped code without modification.
