# TASK-349 — [C3X-gating] Team-gated interior + spawn-inside + tunable re-derivations — programmer handoff

Status: **ready-for-qa** (file-only; NOT compiled — TASK-350 compiles). Law followed: TASKBOARD "CASTLE-3X" rulings 1–4/8 + CONVENTIONS "Castle 3× HOLLOW (2026-07-28)". Names are character-for-character per the naming law.

Everything is inert-or-harmless against the CURRENT solid SM_Castle — **by measured values, not assumption (corrected in loop 1; see the "Loop-1 fixes" section)**: the blocker's DEFAULT position/extents embed its whole box inside the shipped 814.5×820.6×894.9 mesh's solid collision (X [220,380] < 407, Y ±400 ≤ 410, Z [0,600] < 895), the nav areas are cost-1 walkable (generation unchanged), the filters only exclude regions enemy AI could never enter anyway, and the wider spawn boxes admit points that nav projection / collision still arbitrate. (Loop-0 shipped a 3×-derived blocker default that sat ~734 uu OUTSIDE the shipped mesh — QA B1; fixed.)

---

## Per-file delta map (post-edit line refs)

### 1. `Config/DefaultEngine.ini` — NEW `[/Script/Engine.CollisionProfile]` section (lines 297–307)
- `ECC_GameTraceChannel1` → `Name="SiegeTeamBlue"`, `ECC_GameTraceChannel2` → `Name="SiegeTeamRed"` (lines 306–307). Object channels (`bTraceType=False`), `DefaultResponse=ECR_Block` — chosen so every EXISTING preset (Pawn, BlockAll, …) responds to a re-typed capsule exactly as it did to ECC_Pawn; the gate discrimination lives ONLY on GateBlockerVolume's own response matrix.
- QA: check this block character-by-character (a malformed ini bricks editor load). Verified: exactly one `[/Script/Engine.CollisionProfile]` section exists in the file; no prior GameTraceChannel definitions anywhere in Config/ or Source/.

### 2. NEW `Source/GitClaudeUnrealTest/Siegebound/SiegeNavAreas.h` / `.cpp` — the team-gating one-home
- h:55–58 — C++ channel aliases `ECC_SiegeTeamBlue` = GameTraceChannel1, `ECC_SiegeTeamRed` = GameTraceChannel2 (constexpr; lockstep note pointing at the ini).
- h:60–70 — `SiegeTeamObjectChannel(ETeamId)` / `SiegeEnemyTeamObjectChannel(ETeamId)` constexpr helpers (the one team→channel mapping).
- h:72–100 — `UNavArea_BlueCastleInterior` / `UNavArea_RedCastleInterior` (UNavArea subclasses, GENERATED_UCLASS_BODY; cpp ctors set `DefaultCost = 1` + team DrawColor — cost-1 so navmesh GENERATION and own-team pathing are byte-identical to plain navmesh).
- h:86–100 — `UNavFilter_TeamBlue` / `UNavFilter_TeamRed` (UNavigationQueryFilter subclasses; cpp ctors call the engine's protected `AddExcludedArea(<ENEMY interior area>)` — Blue excludes Red's interior, Red excludes Blue's; cpp:24–41).
- h:102–106 — `SiegeTeamInteriorAreaClass(ETeamId)` / `SiegeTeamNavFilterClass(ETeamId)` free functions (cpp:43–55).
- h:108–129 — `ASiegeUnitAIController : AAIController` with public `ApplyTeamNavigationFilter(ETeamId)` (cpp:57–63). **Why it exists:** `AAIController::DefaultNavigationFilterClass` is PROTECTED in UE 5.8 (verified in the engine header, AIController.h:145–154) — there is no public setter, so the sanctioned wiring is this minimal subclass. It adds ZERO behavior beyond the setter. Every existing `MoveToActor`/`MoveToLocation` in ASummonedUnit passes `FilterClass = null` and therefore picks the default filter up with NO call-site edits.

### 3. `Castle.h` / `Castle.cpp`
- h:13/16 — fwd decls `UBoxComponent`, `UNavModifierComponent`.
- h:150–186 — components `GateBlockerVolume` (UBoxComponent) + `InteriorNavModifier` (UNavModifierComponent), Category "Siegebound|Castle|Gating", full doc.
- h:197–211 — `SpawnBoxHalfExtent` **(840,840) → (2460,2460)** + rewritten 3-way pairing cross-note (site 1 of 3).
- h:213–242 — NEW tunables `GateBlockerRelativeLocation` (default **(300, 0, 300) — the loop-1 SAFE-EMBEDDED value**, was (1221, 0, 300) in loop 0) + `GateBlockerExtent` (default (80, 400, 300)), EditAnywhere (per-instance adjustable at TASK-350).
- h:253–263 — `ConfigureTeamGating()` decl.
- cpp:31–32 — `CastleDamageNumberHeightZ` 1050 → **3150** (see flagged decision F4).
- cpp:49–56 — HP bar relative Z 1050 → **3150** (spec item 5: C++-authored, scaled ×3).
- cpp:58–80 — ctor: both gating components created inert (blocker `NoCollision` + `SetCanEverAffectNavigation(false)` + no overlap events; nav modifier ctor `AreaClass = UNavArea_BlueCastleInterior` — NEVER UNavArea_Null, so nav generation is untouched in every state incl. editor/no-BeginPlay).
- cpp:119–124 — BeginPlay calls `ConfigureTeamGating()` (after the HP-bar init).
- cpp:126–171 — `ConfigureTeamGating()`: derives OWN/ENEMY channels from `Team` (symmetric, zero hardcoded Blue/Red branches); blocker gets tunable size/position, object type = OWN channel, `SetCollisionResponseToAllChannels(ECR_Ignore)` + `Block` ONLY the enemy channel, then `QueryAndPhysics`; nav modifier `SetAreaClass(SiegeTeamInteriorAreaClass(Team))`. Both branches null-checked.
- cpp:435–441 — `IsPointInSpawnBox` comment refreshed to (2460,2460).
- Untouched (byte-identical): TakeDamage scaling, crumble thresholds/`UpdateCrumbleStages`/`ApplyCrumbleStage`, `HandleDestroyed`, `ResetCastle`, heal-over-time, team visuals, delegates. Note: `HandleDestroyed`'s existing `SetActorEnableCollision(false)` also drops the gate blocker (a fallen castle gates nothing) and `ResetCastle` restores it — response matrix persists across both; no new code needed.

### 4. `SummonedUnit.h` / `SummonedUnit.cpp`
- h:52–56 — class doc: controller is now `ASiegeUnitAIController` (behaviorally plain AAIController).
- h:354–363 — `PossessedBy` override decl (ref corrected in loop 1 — QA NIT). **Why:** AutoPossessAI possession can land AFTER BeginPlay (proven in this codebase — the miner's controller poll at MinerUnit.cpp:393 exists for exactly this gap), so a BeginPlay-only filter push can miss the controller.
- h:585–611 — `ApplyTeamGatingProfile()` decl (the ONE unit-side stamping site, full doc).
- cpp:38 — include SiegeNavAreas.h (ref corrected in loop 1 — QA NIT).
- cpp:98–104 — ctor: `AIControllerClass = ASiegeUnitAIController::StaticClass()` (was plain `AAIController`).
- cpp:218–224 — BeginPlay: `ApplyTeamGatingProfile()` right after `ApplyTeamMaterial()` (same team-set discipline).
- cpp:229–239 — `PossessedBy`: Super + re-apply (idempotent).
- cpp:241–262 — `ApplyTeamGatingProfile()`: capsule `SetCollisionObjectType(SiegeTeamObjectChannel(Team))` (OBJECT TYPE ONLY — response matrix untouched, so all `ActorGetDistanceToCollision(…, ECC_Pawn, …)` damage/range/spell math is byte-identical) + `Cast<ASiegeUnitAIController>(GetController())` → `ApplyTeamNavigationFilter(Team)`. Null-safe both halves; a non-Siege controller degrades to pre-feature pathing with the physical gate still holding.
- cpp:559–568 — `InitUnit` post-BeginPlay branch also re-applies the gating profile (late team update swaps gate sides, mirroring the material re-apply).
- Untouched (byte-identical): the whole state machine, group orders (TASK-344), Shield Wall bodies, keywords, buffs, freeze, death/anim paths, all MoveTo call sites. AMinerUnit inherits everything.

### 5. `HeroCharacter.cpp` (h untouched)
- :29 — include SiegeNavAreas.h.
- :83–96 — BeginPlay stamps the hero capsule by `GetTeamId()` (ITeamAgent contract) — hero ruling 2: blocked at the ENEMY gate, passes his own. No nav filter (player-driven, never pathfinds). The revert-to-hero-raids path is this single line.

### 6. `SiegePlayerController.h` / `.cpp`
- h:94–100 — class placement doc: plinth clause replaced with the retirement note.
- h:637–655 — `SpawnBoxHalfExtent` **(840,840) → (2460,2460)** + full pairing cross-note (site 2 of 3) + the mid-zone-stays-840 flag.
- h:709–717 — `CastlePlinthClearance` UPROPERTY **deleted**, replaced by a `//~` retirement note (supersede-not-silent-delete).
- h:1023–1025 — `IsPointInsideCastlePlinth` decl **deleted**, retirement note in place; IsPointInOwnSpawnBox doc cleaned of the dangling reference (h:1027–1037).
- cpp:1427–1456 — placement-validity comment rewritten to v4; the gate itself is now `bValid = IsPointOnNavmesh(PlacementLocation);` (line 1456) — the `&& !IsPointInsideCastlePlinth(...)` refusal removed.
- cpp:3168–3170 — HasObstacleClearance comment: dangling reference removed.
- cpp:3191–3196 — `IsPointInsideCastlePlinth` body **deleted**, `//~` retirement note in place.
- cpp:3225–3233 — IsPointInOwnSpawnBox comment refreshed (2460 + spawn-inside-by-construction).
- Untouched: capture-zone clause, slope/obstacle/building clearances, spells, economy, group orders, input.

### 7. `SiegeBotController.h` / `.cpp`
- h:301–308 — BotCastleSpawnOffset appendix-3a cross-note refreshed: with the 2460 box the 1,750 castle-front offset now lands INSIDE the box, so ruling #1's knob is ACTIVE (it was clamp-inert at 840). No value/logic change — the anchor-clamp law reads the tunables (structurally untouched, per ruling 3).
- h:349–355 — bot `CastlePlinthClearance` UPROPERTY **deleted**, `//~` retirement note in place.
- h:365–377 — `SpawnBoxHalfExtent` **(840,840) → (2460,2460)** + pairing cross-note (site 3 of 3).
- cpp:451–456 — rule-1 tower standoff: `MinStandoff = CastlePlinthClearance + 150.f` → **literal `570.f`** with derivation comment (see flagged decision F5 — bot decision output preserved byte-for-byte).
- cpp:1282–1297 — `IsBotHalfPointClear`: the castle plinth keep-out LOOP (the bot-side placement refusal) **deleted** with an in-place retirement note; box/captured-zone gate, unit spawn clearance, and building clearance all byte-identical.
- Untouched: all §4 rules and decision logic (outside the two constants above), wave/anchor code, deck logic.

### 8. `BattlefieldScatter.cpp` (CONSEQUENTIAL — outside the names block, mandatory; see F1)
- :20 — include SiegeNavAreas.h.
- :614–619 — real-geometry scatter blocker (rocks/slabs/hills): added `Block` for `ECC_SiegeTeamBlue/Red` next to the existing `ECC_Pawn` block.
- :737–741 — invisible tree-trunk collision proxy: same two `Block` lines.

## The 3-way pairing proof
`SpawnBoxHalfExtent = FVector2D(2460.f, 2460.f)` at exactly three sites, cross-notes updated at each (Castle.h ref shifted post-loop-1):
- `Castle.h:211`
- `SiegePlayerController.h:655`
- `SiegeBotController.h:377`
`ACaptureZone::ZoneHalfExtent` (840,840) byte-untouched (`CaptureZone.h/.cpp` not modified). `SpawnBoxAnchorInset` 40 untouched.

## Plinth call-site table (retirement completeness)
| # | Site (pre-edit) | Kind | Disposition |
|---|---|---|---|
| 1 | SiegePlayerController.h:712 `CastlePlinthClearance` UPROPERTY | tunable | Deleted; `//~` retirement note at h:709–717 |
| 2 | SiegePlayerController.h:1019 `IsPointInsideCastlePlinth` decl | refusal API | Deleted; note at h:1023–1025 |
| 3 | SiegePlayerController.cpp:1453 `&& !IsPointInsideCastlePlinth(...)` in placement validity | PLACEMENT REFUSAL | Removed (cpp:1456); validity = nav projection + collision + existing clearances (v4 comment) |
| 4 | SiegePlayerController.cpp:3189–3217 function body | refusal impl | Deleted; note at cpp:3191–3196 |
| 5 | SiegePlayerController.cpp:3170 comment ref | comment | Reference removed |
| 6 | SiegePlayerController.cpp:3231 comment ref (IsPointInOwnSpawnBox) | comment | Reference removed |
| 7 | SiegePlayerController.h:98 class-doc clause | comment | Rewritten (retirement note) |
| 8 | SiegePlayerController.h:642 SpawnBoxHalfExtent doc ref | comment | Rewritten with the new derivation |
| 9 | SiegeBotController.h:349 `CastlePlinthClearance` UPROPERTY | tunable | Deleted; `//~` note at h:349–355 |
| 10 | SiegeBotController.cpp:451 `MinStandoff = CastlePlinthClearance + 150.f` | standoff DERIVATION (not a refusal) | Literal `570.f` + comment — behavior byte-identical (F5) |
| 11 | SiegeBotController.cpp:1300–1315 keep-out loop in `IsBotHalfPointClear` | SPAWN REFUSAL | Deleted (note at cpp:1290–1293); Blue-castle branch was already unreachable (Blue castle can never be in the Red box/zone) |
| 12 | SiegeBotController.h:359 SpawnBoxHalfExtent doc "= 2× CastlePlinthClearance" | comment | Rewritten with the new derivation |

No other reference to either symbol exists in Source/ (grep-verified; remaining hits are the retirement notes themselves). No placement path refuses the own interior anymore.

## Flagged decisions (beyond the rulings — QA please scrutinize each)
- **F1 — BattlefieldScatter.cpp touched (outside the task's names block).** MANDATORY consequence of the capsule re-type: those two bodies are ignore-all + explicit `Block ECC_Pawn` only (M6.6 climbable-terrain + tree-proxy laws). Once a capsule's object type is no longer ECC_Pawn, the pairwise test fails and re-typed units/hero would walk and FALL THROUGH hills and tree trunks — a hard shipped-behavior regression. Two response lines per site, nothing else touched. If QA rules this out-of-lane, the alternative is a real regression — please escalate rather than strike the lines.
- **F2 — `ASiegeUnitAIController` (new class, not in the names block).** `DefaultNavigationFilterClass` is protected on AAIController in 5.8 with no public setter (engine header verified); the ruling's "set as DefaultNavigationFilterClass on unit AI controllers" is only reachable via a subclass (or reflection, rejected). Housed in the SiegeNavAreas one-home per the one-home spirit. Risk noted for TASK-350: any BP_Unit_* that explicitly SERIALIZED the old `AIControllerClass = AAIController` default would silently keep it — degrades to physical-lane-only gating (null-safe), and PIE matrix (c) would surface it.
- **F3 — `PossessedBy` override on ASummonedUnit (stamping-site extension).** The codebase's own miner poll proves AutoPossessAI possession can land after BeginPlay; without the hook the nav filter could be missed on that path. Idempotent re-apply; Team is authoritative at possession on every spawn path.
- **F4 — `CastleDamageNumberHeightZ` 1050 → 3150.** The spec's item 5 names only the HP bar, but this constant is documented in-code as "HP-bar Z parity" and at 1050 the damage number would spawn INSIDE the 2694-tall mesh. Scaled ×3 with the bar to preserve the authored parity.
- **F5 — Bot tower-standoff floor kept at literal 570.** Site 10 is a heuristic anchor derivation, not a refusal; the byte-identical bot-decision guarantee outranks cosmetically re-deriving it. Reads identically to the shipped build.
- **F6 — GateBlocker geometry: tunable design ACCEPTED by QA; the loop-0 default value REJECTED (B1) and replaced.** The default is now the SAFE-EMBEDDED (300, 0, 300) — inert vs the SHIPPED mesh by measurement — and deliberately NOT pre-derived for the 3× model. TASK-350 sets the authored gate offset per instance (a +X-face-centered gate lands near (1221, 0, 300); each instance's facing may differ) via the EditAnywhere tunables. This remains the most likely thing TASK-350 has to adjust, and a forgotten adjust fails LOUDLY (ungated 3× door in PIE matrix (c)/(d)), never silently.
- **F7 — Blocker object type = the OWN team's channel** (not WorldStatic/WorldDynamic): keeps it invisible to AProjectile's terrain-impact OBJECT query (WorldStatic+WorldDynamic list, Projectile.cpp:415–417) and to every trace channel (ignore-all base) — the "projectiles/spells unaffected" guarantee is structural, not incidental.
- **F8 — InteriorNavModifier marks the castle's whole COLLISION BOUNDS** (UNavModifierComponent derives bounds from the owner's collision), not a hand-drawn interior box. Correct for the law (enemy paths must not enter the footprint) and inert for generation (cost-1 area), but on the CURRENT solid castle small navmesh slivers at the AABB corners get area-flagged: enemy partial paths can end a few uu earlier there. Harmless now (nothing walkable inside the solid AABB to speak of); TASK-350's matrix (e) castle-to-castle march + (c) no-pile-up proofs cover it.
- **F9 — `ACastle::IsPointInSpawnBox` left in place.** It is genuinely orphaned (zero callers — its W1 TASK-275 caller was superseded by TASK-344), but the board block does not order its removal, so it stays as a consistent reader of the updated tunable. Flagging so its orphan status is on the record.
- **F10 — CaptureZone.h stale comments not touched.** Its doc still says the mid zone is "the SAME size as each side's spawn box" — now false (box 2460, zone 840). Value must stay 840 (ruling 3); the file is outside the names block, so I left it byte-untouched and recorded the truth in the two spawn-box cross-notes. QA may rule a comment-only touch-up into a later pass.

## What TASK-350's integration must verify (beyond its own spec)
1. Ini channels live after the editor bounce: Project Settings → Collision shows SiegeTeamBlue/SiegeTeamRed object channels.
2. GateBlocker placement vs the ACTUAL authored gate (F6/B1): **re-positioning is now MANDATORY, not conditional** — the C++ default is the safe-embedded (300, 0, 300), which does NOT cover the 3× gate. TASK-350 must set `GateBlockerRelativeLocation` (and extents if needed) per castle instance against the authored gate + facing, then prove: hero blocked at the ENEMY gate, passes his OWN, both instances (PIE matrix (d)). If skipped, the door is simply ungated — loud in matrix (c)/(d), no old-mesh hazard.
3. Filter uptake: sample a few BP_Unit_* instances in PIE — possessing controller class should be ASiegeUnitAIController (F2 serialization risk).
4. Navmesh generates INSIDE both castles and through the gates; enemy waves show NO door pile-up; own units path in; placement succeeds on the interior floor (spawn-inside both teams).
5. Hills/trees regression watch (F1): units and hero still climb hills and collide with tree trunks.
6. HP bar + damage numbers read at the 3× height (3150) at the gameplay camera.
7. Scatter keep-clear ×3 re-derive is 350's own step — unrelated to this code but same session.

## Byte-identical set (verified untouched)
Group orders (TASK-344, commit 3068286) — no group/order/command file or code path modified; spells (SpellLibrary/SpellLineSweep untouched); economy (GoldNode/MinerUnit/DeepMine/player-state untouched); win condition / castle HP / crumble thresholds / ApplyCrumbleStage / team visuals; all unit AI outside the stamping sites; bot §4 decision logic outside the spawn-box constant and the two documented plinth sites; `ZoneHalfExtent`; `SpawnBoxAnchorInset`.

---

## Loop-1 fixes (QA `qa/TASK-349.md`: 1 BLOCKER, 2 WARN, 4 NIT — all addressed)

### B1 (BLOCKER) — GateBlocker default: chose QA option (a), the safe-embedded default
- `Castle.h:230` — `GateBlockerRelativeLocation` default **(1221, 0, 300) → (300, 0, 300)**. With the unchanged extents (80, 400, 300) the armed box spans X [220, 380] × Y ±400 × Z [0, 600] — entirely inside the shipped 814.5×820.6×894.9 solid mesh (half-depth ≈ 407, half-width ≈ 410, height ≈ 895), exactly QA's recommended envelope. Both B1 symptoms die together: no mid-corridor invisible wall, and the blocker's box no longer adds anything beyond the footprint to `UNavModifierComponent::CalcAndCacheBounds` (the F8 addendum).
- **Why (a) over (b) (the arming bool):** (a) fixes the danger at its source with zero new API surface — the arming path stays unconditional and simple, and there is no second flag whose forgotten flip is its own failure mode. The failure profile of a forgotten TASK-350 retune is identical under both options (an ungated 3× door, loud in PIE matrix (c)/(d): hero walks into the enemy keep, enemy units path inside at the physical lane) — but (b) additionally risks a world where the tunables were retuned and the bool forgotten, a two-knob inconsistency (a) cannot have. QA leaned (a); concur.
- **Corrected claims the old value falsified:** (1) this handoff's intro inertness premise (now states inertness BY MEASURED VALUES with the numbers, and records the loop-0 error); (2) `Castle.h` `GateBlockerVolume` component doc (old :164–166 claim) — now states the embedded-default mechanism and the loud-failure property (post-edit Castle.h:163–171); (3) the `GateBlockerRelativeLocation` doc itself (Castle.h:213–228) — full safe-embedded rationale + the (1221, 0, 300) guidance preserved as TASK-350's target value, not as the default; (4) `GateBlockerExtent` doc notes it co-defines the embedded envelope (Castle.h:232–241); (5) F6 and integration-verify item 2 above rewritten — the TASK-350 retune is now MANDATORY.

### WARN 1 — stale "plinth keep-out covers them" claim
- `SiegePlayerController.h:687–697` (`BuildingClearance` doc) — rewritten: castles are class-disjoint AND since TASK-349 carry no placement clearance at all (keep-out retired; the enemy side is fenced by team gating, not placement rules).
- **Same-defect-class twin fixed and declared (was not in QA's list):** `SiegePlayerController.cpp:3090–3096` (`HasBuildingClearance` body comment) carried the identical false claim "(the plinth keep-out covers them, TASK-027 handoff)" — corrected the same way. Comment-only; the retirement law ("never a silent violation") is why it could not stand.

### WARN 2 — stale TowerDefenseStandoff doc
- `SiegeBotController.h:341` — "(clamped outside the plinth keep-out ...)" → "(clamped above the 570-uu standoff floor — the retired plinth keep-out's numeric legacy, TASK-349 ...)". Matches the F5 literal at cpp:456.

### NITs
- **NIT 1 (fixed):** `SiegePlayerController.h:698–711` (`NavProjectionExtent` doc) + cpp `IsPointOnNavmesh` comment (cpp:3077–3081) — plinth-anchored rationale replaced with the current truth (projection IS the placement truth; tight Z refuses elevated non-walkable hits, ground-level interior-floor hits validate).
- **NIT 2 (fixed):** handoff line-ref drift corrected in place (SummonedUnit.cpp include :38; `PossessedBy` decl h:354–363) + Castle.h refs re-baselined after the loop-1 edits (SpawnBoxHalfExtent :211).
- **NIT 3 (acknowledged, no action):** HP bar / damage number at Z 3150 float above the CURRENT castle in the interim window — cosmetic-only, rides the same TASK-350 sequencing as the mesh itself; QA concurred no separate action.
- **NIT 4 (acknowledged, deferred stands):** `CaptureZone.h:44/:154` stale "same size as the spawn box" prose — value verified byte-untouched at (840,840); comment-only fix deferred to a later pass per F10 (QA recorded it so it is not lost).

### Additional same-class comment sites fixed and declared (retirement-law completeness; none listed by QA, all comment/log-literal only, zero logic)
The loop-1 sweep (`grep -i plinth` across Source/) surfaced further prose still asserting the plinth rule as LIVE. All fixed; every remaining "plinth" hit in Source/ is now a retirement note, historical narrative, or the standoff-legacy annotation:
- `SiegePlayerController.cpp:1244` — the placement-refusal LOG LITERAL still listed "or on a castle plinth" as a live refusal reason → reworded (Printf literal only, no format specifiers touched).
- `SiegePlayerController.h:776` — `EPlacementInvalidReason::Point` enum comment → plinth dropped.
- `SiegePlayerController.h:806` — `UpdatePlacementGhost` doc "validity v3 (... plinth keep-out ...)" → v4 without it.
- `SiegeBotController.cpp:1236–1243` — degrade-open comment + warn LOG LITERAL "using the half/plinth rule only" → "spawn box/zone + clearance rules only" (literal only).
- `SiegeBotController.cpp:1148–1150` + `SiegeBotController.h:507–509` — "the plinth is deliberately NOT special-cased" pair → reworded as ring-walk-out ownership with the retirement noted.
- `SiegeBotController.cpp:1255` — ring-walk comment "plinth / clearance / box failure" → "clearance / box failure".
- `SiegeBotController.h:479–491` (`ComputeValidBotSpawnPoint` doc) and `:532` (`IsBotHalfPointClear` doc) — plinth removed from the listed live rules, retirement cited.

### QA's non-blocking CONVENTIONS recommendation
The "any Pawn-blocking ignore-all body MUST also Block ECC_SiegeTeamBlue/Red" scatter-channel-law line (QA report, mandate-1 sharpest finding) is a CONVENTIONS edit — manager's file, out of my lane; endorsing it here for the manager's next CONVENTIONS touch.

**Loop-1 files touched:** `Castle.h`, `SiegePlayerController.h`, `SiegePlayerController.cpp`, `SiegeBotController.h`, `SiegeBotController.cpp`, this handoff. Everything QA passed is untouched: the ini block, `SiegeNavAreas.{h,cpp}`, `Castle.cpp`, `SummonedUnit.{h,cpp}`, `HeroCharacter.cpp`, `BattlefieldScatter.cpp`, all values outside `GateBlockerRelativeLocation`, and all logic everywhere (loop 1 is one default value + comments/log-literals).

---

## Loop-2 fixes (integration findings, TASK-350 re-run — `qa/TASK-349.md` top append: B2 + B3 + the baked-defaults ride-along)

All three items are runtime findings against code the pre-compile reviews passed; every engine claim below was re-verified against the INSTALLED 5.8 source, not assumed.

### B2 — MARCH FREEZE: filter-aware structure-goal resolution (chosen shape: project the PER-UNIT near-wall point under the mover's own filter, then `MoveToLocation`)

**Mechanism (build-master's differential, confirmed in code):** `EnterAdvance`'s structure branch issued `MoveToActor(Castle, …, FilterClass=nullptr)` — null resolves to the controller's team default filter (by design), and the goal was the castle ACTOR ORIGIN, which the hollow 3× castle puts ON enemy-interior navmesh that the mover's filter EXCLUDES. Goal-poly resolution under the filter finds no poly ⇒ the request fails outright — `bAllowPartialPath` never engages because a partial path still needs a resolved goal poly. 123/123 combat units froze; miners (mine goals) and the hero (unfiltered) were untouched.

**Fix (`SummonedUnit.cpp:2072–2120` `ResolveStructureMarchPoint` + the `EnterAdvance` structure branch `:2024–2060`; decl `SummonedUnit.h:743–764`; tunable `StructureGoalProjectionExtent` (800,800,600) `SummonedUnit.h:470–483`):**
1. Take the nearest point on the structure's `ECC_Pawn`-blocking collision to THIS unit (`ActorGetDistanceToCollision` — the same closest-point convention every range check uses, so the march target is the very wall face the attack math measures; origin fallback when no blocking collision, the shared convention).
2. Project it to the nearest poly ALLOWED by the possessing controller's `DefaultNavigationFilterClass` (`UNavigationSystemV1::ProjectPointToNavigation` filter overload — engine-verified at NavigationSystem.h:718; `UNavigationQueryFilter::GetQueryFilter` null-class returns null ⇒ navdata default filter, engine-verified at NavigationQueryFilter.cpp:107–117). For an enemy castle that lands on the wall-base ring / gate apron OUTSIDE the excluded interior; for the OWN castle the interior itself is allowed (own units still walk in).
3. March there via `MoveToLocation(…, bProjectDestinationToNavigation=false, FilterClass=nullptr, bAllowPartialPath=true)` — **the PATH still runs under the team default filter, so the no-enemy-pathing-inside guarantee is byte-untouched; only the GOAL moved from an unreachable poly to a reachable one.** Structures are static — losing `MoveToActor`'s moving-goal tether costs nothing.
4. Failure ladder: no nav system/data or nothing allowed within the extent ⇒ `return false` ⇒ the caller falls back to the LEGACY `MoveToActor` (pre-feature behavior + the existing warn). Null-safety law intact.

**Why the other candidate shapes lost:** a permissive per-request filter override would let enemy paths enter the interior and pile at the physical gate — it breaks the verified guarantee; allow-partial-path semantics cannot work at all here (verified: the failure is goal-RESOLUTION, upstream of path truncation); a geometry-derived gate-front anchor needs per-instance gate/facing knowledge C++ doesn't reliably have (the authored gate is a −Y corridor mouth, not a wall-face formula).

**Why it can't thrash (TASK-280/282 law):** the resolver is called ONLY inside the existing goal-changed-or-idle re-path gate — never per tick; the projected point is deterministic for a static goal, so idle re-issues re-resolve to the same point (`AlreadyAtGoal` class, the same cheap pattern the pre-3× partial-path flow produced); `CurrentMoveGoal` bookkeeping (actor-keyed) is unchanged, so the gate itself is byte-identical.

**Why melee ends in attack range:** the resolved point is the projection of the unit's nearest wall-face point — agent-radius (~34 uu) plus hull margin off the wall, acceptance 50 — the unit stops where `ActorGetDistanceToCollision ≤ melee Range` against the same wall, exactly the pre-3× flush-to-the-wall endpoint. Per-unit resolution preserves the natural around-the-perimeter spread (no single-point pile-up).

**Known edge (documented, accepted):** a goal buried DEEP inside the enemy interior (e.g. an enemy building at the hall center) fails the 800-extent projection by design — unreachable-by-design targets degrade to the legacy path (unit holds, warn on goal change) rather than resolving to a wall point they cannot fight from. The extent is deliberately NOT half-diagonal-sized for this reason (doc on the tunable).

### B3 — nav-area staleness: three deterministic legs (root cause found in engine source)

**Root cause (engine-verified):** `UNavRelevantComponent` defaults to `bAttachToOwnersRoot=true` (NavRelevantComponent.cpp:9), so `InteriorNavModifier`'s area data rode the root **CastleMesh GEOMETRY element** in the nav octree. Consequences: (i) the raw-geometry path resolves collision areas via `FRecastRawGeometryElement::GetCollisionAreaClass`, which carries the `ensureMsgf(Areas.Num() <= 1)` at RecastNavMeshGenerator.cpp:305 and honors ONLY `Areas[0]` — our modifier emits one area PER cached collision box (22 UCX hulls; NavModifierComponent.cpp `GetNavigationData` iterates `ComponentBounds`), which is exactly the observed ensure; (ii) area marking was COUPLED to every rebuild/re-registration of the geometry element — the crumble→pristine swap + `SetActorEnableCollision(true)` churn at Play-Again is precisely when the hall went nav-open to both filters.

**Leg 1 — decouple (`Castle.cpp:82–94`):** the ctor calls `InteriorNavModifier->ForceNavigationRelevancy(true)` — engine-verified (NavRelevantComponent.cpp:116–124) to set `bAttachToOwnersRoot=false` + force relevancy, and CTOR-SAFE (`RefreshNavigationModifiers` no-ops pre-registration via the `bRegistered` guard, cpp:136–143). Because the flag lands before `OnRegister` ever caches a nav parent, the FIRST registration is already a standalone octree element — the canonical NavModifierVolume shape, whose dynamic-area marking path handles multi-box area lists properly and survives geometry churn. This removes our actor from the `Areas.Num()<=1` ensure path entirely.

**Leg 2 — team area BEFORE first generation (`Castle.h:137–146`, `Castle.cpp:123–140`):** new `PostInitializeComponents` override selects the TEAM's interior area — after the serialized `Team` is authoritative, before Dynamic Recast's first generation pass — so interior tiles build correct-first-time. This deletes the mechanism of the (a) window (tiles built ctor-Blue, then rebuilt after the BeginPlay flip — the measured 9.8–11.6 s signature): with no flip there is no double-build, and the band's "initial-load-only" residue is just first-build latency with the CORRECT area, never a wrong-team read. `ConfigureTeamGating`'s BeginPlay `SetAreaClass` stays as a free re-assert (engine-verified early-out on unchanged class, NavModifierComponent.cpp:249–256) for spawned-castle paths.

**Leg 3 — same-frame fence at every mesh swap (`Castle.cpp:428–439` in ApplyCrumbleStage, `:466–481` in ResetCastle):** both swap sites now call `InteriorNavModifier->RefreshNavigationModifiers()` in the SAME frame as the swap (ResetCastle's also covers its `SetActorEnableCollision(true)`). Determinism argument: tile rebuilds are asynchronous and gather octree data AT BUILD TIME; a same-frame octree re-assert guarantees the area data is present before ANY tile dirtied by the swap can build — the +8.5→+29.7 s enemy-open window depended on the area's return being incidental (a later dirtying event); it is now ordered. Belt to Leg 1's braces: with the decoupled element the entry isn't even disturbed by the swap; the fence makes the ordering explicit rather than incidental.

**Acceptance mapping:** (a) ≤10 s initial-load-only — no flip ⇒ correct-from-first-build (the prior probe's first possible sample, 9.82 s, should now read correct); (c) never enemy-open — Legs 1+3 remove both the mechanism and the ordering dependence. Verify with the same timestamped projection probe (r5 pattern).

### Ride-along — PIE-verified gate defaults baked (`Castle.h:229–274`)
- `GateBlockerRelativeLocation` (300,0,300) → **(6, −525, 284)**; `GateBlockerExtent` (80,400,300) → **(260, 135, 226)** — the TASK-350 re-run's per-instance-verified values (hero blocked at the enemy gate / passes his own, both castles, in-engine).
- **Co-commit reasoning (in the doc comment, per the flag):** TASK-350 commits this code and the 3× mesh in the same session — the defaults and the mesh they were measured against land together, so no committed world pairs them with the old solid mesh the way loop-0's uncoordinated 1221 default did.
- **Honest restore-path residue (named + accepted, in the doc comment):** a future MESH-ONLY revert to the old 814.5×820.6×894.9 castle re-creates a mis-placed blocker — the box spans Y [−660, −390] vs that mesh's ±410 half-width, i.e. a 520×250×452 enemy-only bump ~250 uu proud of its −Y face, holding enemy melee on that one strip out of range (localized stall, not match-breaking; every other face unaffected). Any such revert must retune the two tunables with the mesh. This supersedes loop-1's safe-embedded default and its rationale; the loop-1 sections above stand as the audit record.

### Loop-2 files touched
`SummonedUnit.h` (tunable + resolver decl + EnterAdvance doc), `SummonedUnit.cpp` (2 includes, EnterAdvance structure branch, resolver), `Castle.h` (PostInitializeComponents decl, component-doc rewrites, baked gate defaults), `Castle.cpp` (ctor decoupling, PostInitializeComponents, ConfigureTeamGating comment, 2 same-frame refreshes), this handoff. **Untouched:** the ini block, `SiegeNavAreas.{h,cpp}`, `HeroCharacter.cpp`, `SiegePlayerController.{h,cpp}`, `SiegeBotController.{h,cpp}`, `BattlefieldScatter.cpp`; all stamping sites; the blocker's response matrix; the pawn-chase branch; every state-machine body. The in-engine-verified gating chain (channels, stamping, blocker symmetry, filter exclusion) is byte-identical.

### For the loop-2 QA / TASK-350 re-verify
1. B2: full wave march both directions (matrix (e) 0-cull A→B), melee striking castle walls, no per-tick re-path in the log cadence, own units still entering their own hall.
2. B3: the r5 timestamped probe — initial ≤10 s correct-first-time, Play-Again with NO stale sample after T0 (both filter directions), and the `Areas.Num() <= 1` ensure GONE from the session log.
3. Gate defaults: fresh PIE with NO per-instance override should now pass matrix (d) as-is (the baked values ARE the verified ones); confirm per-instance overrides in L_Arena (if any were saved) agree or are cleared.
4. The interior-building edge (B2 known edge) is expected behavior — do not file it as a freeze recurrence.

---

## Loop-4 fix (B4 — Jonathan-authorized loop; `qa/TASK-349.md` FINAL-RUN append + `qa/TASK-349-FR-navprobe.txt`)

### Mechanism (probe-proven; my loop-2 Leg 2 caused it)
Leg 2 moved the team-area selection to PostInitializeComponents so the FIRST octree registration already carries the team class — correct-first-time for freshly GENERATED tiles. In doing so it removed the only POST-registration area CHANGE the old code had (the pre-loop-2 BeginPlay `SetAreaClass` flip, whose ~11 s repair the RE-RUN had measured). Pre-built tiles — editor-built and SAVED, i.e. every real boot lane, because the editor world never runs PostInitializeComponents/BeginPlay on level actors and its tiles are therefore always ctor-Blue on BOTH interiors — see no change, get no dirty, and are NEVER re-marked. The r5 probe: `RHb=OK RHr=NO` from T=0.33 s, 626 consecutive stale samples over 211 s, unbounded, flipping only when enemy melee crumbled the red castle and tripped the ApplyCrumbleStage Leg-3 fence; path-level and live-unit proofs (Blue units invited into the red gate mouth → piled at the physical blocker; Red units refused their OWN hall) in the QA append. Loop 2's model was true for fresh builds and false-in-consequence for the only baselines that exist in practice — every prior session masked it because its own imports dirtied the tiles.

### Fix (per the documented direction, no deviation) — `Castle.cpp` ConfigureTeamGating, nav-lane block
One unconditional call added after the (still-early-out) `SetAreaClass` re-assert:
- `InteriorNavModifier->RefreshNavigationModifiers();` — the Leg-3 fence applied once at startup: octree re-assert + bounds-dirty, forcing the castle-bounds tiles to rebuild against the already-correct team area (correct since PostInitializeComponents; within ConfigureTeamGating the `SetAreaClass` re-assert precedes the refresh in program order, so the refresh can never propagate a non-team class).
- Runs at BeginPlay — after all team-dependent state is final (Team authoritative pre-PIC; the area class set at PIC; the blocker matrix configured lines above in the same function).
- **Why both legs coexist (documented in three places for QA — the ConfigureTeamGating comment, the InteriorNavModifier component doc leg-list (now legs 1–4), the PostInitializeComponents doc):** Leg 2 = GENERATION-TIME correctness — fresh tiles build team-correct-first-time, no flip window on the fresh-build lane; the loop-4 refresh = the PRE-BUILT-TILE re-mark — the one post-registration dirty the load lane otherwise lacks. Different lanes; folding either into the other re-opens one of the two windows.

### Why this cannot regress B3's determinism (the reset-recurrence argument)
1. **The reset path is byte-identical to the FINAL-RUN-verified behavior.** The new refresh runs at BeginPlay only; Play-Again does NOT re-run BeginPlay (ResetCastle is a plain method call) — the reset window stays owned by the UNTOUCHED, empirically confirmed Leg-3 fence in ResetCastle (and ApplyCrumbleStage's, likewise untouched and this-run-proven).
2. **The refresh is monotone.** It only ADDS a dirty/re-assert of data that is already team-correct — it never changes the area class, never removes octree data, and never touches the geometry element (Leg-1 decoupling untouched). At no instant after PostInitializeComponents does the modifier carry a non-team class, so no rebuild it triggers can ever mark a wrong-team area — the stale-open direction (enemy-open) is unreachable by construction.
3. **Ordering-insensitive endgame.** For any interior tile relative to the BeginPlay refresh: built-before ⇒ dirtied ⇒ rebuilt team-correct; built-after ⇒ gathers the already-correct area at build time. Either order converges on team-correct; the only cost is one redundant rebuild of just-built tiles on the fresh-build lane, bounded and at level start.
4. **The residual quantity is latency, not correctness:** the pre-built-lane stale window is now bounded by the castle-bounds tile-rebuild latency (seconds — the FINAL-RUN's fence-triggered rebuild demonstrated the same rebuild class), which is exactly what the R2 ≤10 s band measures.

### What the build-master's r5 probe must show (re-verify)
1. **Pre-built/saved-tile boot (the B4 lane, no in-session import):** both halls read team-correct (`RHb=NO RHr=OK` / `BHb=OK BHr=NO`) within the R2 ≤10 s band from the first sample; NO unbounded plateau; the flip must NOT require any crumble/damage event.
2. **Play-Again leg (untested in the FINAL-RUN):** no stale sample after T0 in either filter direction — the Leg-3 fences, unchanged, own this.
3. **B2 stays dead:** units march + strike walls; the 4-footmen red-gate-mouth pile-up must be GONE (once the red hall is Red-marked, the Blue filter refuses it — the nav lane stops inviting them in); Red units enter their OWN hall (the FAILED own-hall order flips to success).
4. `Areas.Num() <= 1` ensure still absent; warn count stays in the one-shot `bGoalChanged` class.
5. N2 disk-side residue is EXPECTED until the Jonathan-authorized one-time navmesh rebuild+save lands (build-master's lane): the very first boot on the stale SAVED tiles may lack interior polys entirely — the runtime refresh dirties and rebuilds them this session and every session after; the rebuild+save then makes the disk baseline sane.

### Loop-4 files touched
`Castle.cpp` (ONE new call + the ConfigureTeamGating rationale comment + a two-line ctor cross-note), `Castle.h` (InteriorNavModifier leg-list doc → legs 1–4; PostInitializeComponents doc scoped to the generation-time half), this handoff. **Byte-preserved per the dispatch:** the B2 resolver and its EnterAdvance seam, Leg 1 (ctor decoupling), Leg 3 (both fences), the baked gate defaults, all stamping sites, SiegeNavAreas, the ini, both controllers, HeroCharacter, BattlefieldScatter.
