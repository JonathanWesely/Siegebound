<!-- ARCHIVED from .claude/pipeline/TASKBOARD.md on 2026-10-04 by Tools/archive_board.py. Every row below was in a terminal state when moved; bytes are unchanged and this comment is the only addition. Law: TASKBOARD.md '## Archive'. -->
## M4 tasks (decomposed 2026-07-04)

### M4 COMPLETE — 2026-07-04 (done; committed, not pushed; read this first)
All M4 tasks integrated + committed on `main`, **NOT pushed**: C++ batch TASK-053..060 at **65861ce** (via TASK-068, parent 56247c9, clean compile+link first try); editor/art TASK-061..067 + assembly TASK-069 at **e586699** (parent 65861ce). Milestone preserved on branch **m4-testable @ e586699** = the FULL-GAME superset M2+M3+M4 (earlier slices on **m3-testable @ 56247c9** + **m2-testable @ f903cf0**). TASK-053..069 all `done`; blocks left in place (not relocated to ## Done) to preserve the M4 audit trail.
- **Slice verified (TASK-069):** §9-4 expanded-roster slice — ZERO M4-feature FAILs. LIVE PASS: the bot self-drives Set II (repeated Ogre push, discards upgrades/Instants, never unaffordable / never Blue-half, Siege army killed the Blue castle 2000→0 in ~41 s), DT_Cards 22-row reconfirmed warm+cold (DeckCount=50, save-clean), and a real-PIE Defeat screen fired (closes the M2 "no PlayerController end-screen" watch).
- **DEFERRED to Jonathan's playtest** (each backed by a code-QA PASS; MCP cannot inject card-play): keyword fires (Charge/Slayer/Swarm/Suicide/Siege), Support heal, new-tower behaviors (Bomb AoE, Ballista blind spot), Barracks spawn+expire, Deep Mine +2/s, and hero-upgrade apply/stack/cap/refund + persist-through-death + reset-on-PlayAgain.
- **FOLLOW-UP found → TASK-070 (backlog, in ## Active tasks):** 3 stray verification actors committed into L_Arena.umap since M2 (BP_Unit_Footman_C_1, BP_Unit_Miner_C_2, BP_Building_ArrowTower_C_1) — present on ALL branches; cause the "stray Blue unit at match start" + a bot t=0 defend. Root cause in handoffs/TASK-069.md.

### M4 — COMPLETE (Jonathan greenlit M4 2026-07-04; developed on `main`; branches `m2-testable` @ f903cf0 + `m3-testable` @ 56247c9 + `m4-testable` @ e586699 preserve the milestone slices)
M4 = GDD §9-4 + §4 Set II (16 cards) + §3.0 keywords (Siege/Charge/Slayer/Swarm) + §3.8 Siege & Support profiles + §3.10 hero upgrades + §4 Bot Set II extension. **Slice:** expanded-roster combat clip — Ogre push vs Bomb Tower defense. Statuses are all `done` (code TASK-053..060 @ 65861ce, editor/art TASK-061..067 @ e586699, not pushed). **Every code task (053–060) implied a QA review** (standard qa loop); the chain ended at build-master integration (TASK-068 compile, TASK-069 final assembly).

**Dispatch order (from blocked-by):**
- **Wave 1 (parallel, start immediately):** TASK-053 (card data, files) + the three art mesh tasks TASK-065 / TASK-066 / TASK-067 (Blender modeling runs parallel with code; the editor-IMPORT step of each art task serializes with all editor-mutating tasks — one editor instance).
- **Wave 2 (after 053 qa-passed):** TASK-054 (new profiles: Siege+Support) and TASK-057 (new buildings) in parallel (different files). Then TASK-055 (keywords) serializes AFTER 054 (shared SummonedUnit files).
- **Wave 3:** TASK-056 (new towers, after 055's radial-AoE helper) + TASK-058 (hero upgrades, after 055's unit damage-buff hook) in parallel; then TASK-059 (play v3, after 055 + 058).
- **Wave 4:** TASK-060 (bot v2) after 054/055/056/057/059 (needs every card behavior + the play/spawn paths).
- **Build:** TASK-068 (M4 code batch compile + commit) after 053–060 all qa-passed.
- **Editor wave (after 068 compiles, one editor task at a time):** TASK-061 (DT_Cards reimport) → TASK-062 (BP units, after unit meshes) / TASK-063 (BP buildings, after building meshes) → TASK-064 (HUD upgrade row).
- **Final:** TASK-069 (M4 final assembly PIE verify + commit) last.
- **First parallel wave to launch NOW:** TASK-053 + TASK-065 + TASK-066 + TASK-067.

### M4 design rulings (binding for all M4 tasks)
- **M4 test deck (supersedes §3.4 core-only default until M6):** the DeckCount column is redistributed across all 22 rows into an expanded 50-card deck so the full roster is reachable in the hand for BOTH the player and the bot before the M6 deck-builder. Must sum to exactly 50, each ≤ MaxCopies, ≥1 of every card. First-pass (tunable): Footman 6, Archer 4, Wall 3, ArrowTower 2, Knight 2, Miner 3 (core = 20); MilitiaMob 2, Pikeman 2, Sapper 2, Cavalry 2, Longbowman 2, Cleric 2, Ogre 2 (= 14); BombTower 2, BallistaTower 2, Barracks 2, DeepMine 2 (= 8); Masons 2, SharpenedBlade 2, PlateArmor 2, SwiftBoots 1, WarBanner 1 (= 8). This changes M3's exact core-copy-count acceptance — a completed-milestone artifact M4 supersedes by design.
- **FCardRow columns** grow per the CONVENTIONS registry (added there 2026-07-04): `bCharge`, `bSlayer`, `bSuicide`, `SwarmCount`, `AoERadius`, `MinRange`, `SpawnCardID`, `SpawnInterval`, `Lifetime` — CSV header must map 1:1 for reimport. Siege/Support are the existing `Profile` column (targeting); Instant is derived from `CardType` (HeroUpgrade / Utility) — no play-placement step.
- **Siege (keyword + profile):** new `USiegeDamageType_Siege`; Profile=Siege units ignore units/hero and target buildings then the castle (§3.8); their attacks tag Siege damage → 200% vs castle AND buildings (§3.0). Sapper is a Siege unit with `bSuicide` (explode-on-contact/death AoE); Ogre is a Siege tank.
- **Keyword magnitudes are mechanic RULES → UPROPERTY // GDD, not CSV:** Charge (2 s move / 2×), Slayer (150-HP / 2×), Swarm (300-unit circle), Deep Mine (+2 gold/s), Masons (300 HP over 10 s), hero-upgrade effects (§3.10). Per-card numbers that DO vary live in CSV columns (SwarmCount, AoERadius, MinRange, spawner triple).
- **Support/Cleric:** Profile=Support unit never attacks; follows the nearest damaged friendly within Range (400) and heals `Damage`/sec (8); if none damaged, follows the nearest friendly combat unit (§3.8). Heal = negative damage / AddHP, no friendly-fire issue.
- **Hero upgrades (§3.10):** applied via a hero-side upgrade component/state; persist through hero death; reset on match end / Play Again; stack cap = MaxCopies; over-cap play refused + FULL refund (§3.0 refund rule); a `FOnHeroUpgradesChanged` delegate drives the §7 HUD icon row. War Banner is an aura (600, +20% friendly-unit damage) applied through the unit damage-buff hook (TASK-055). The bot has no hero → it treats HeroUpgrade/Utility/Instant/Spell cards as discards (§4 M4 extension).
- **Bot v2:** plays Set II by the same §4 ordered rules (rule 3 now reaches Ogre/Cavalry/etc.); spawns honor SwarmCount + Team=Red + team material; **folds the TASK-046 WARN-2 hardening** — the rule-4 "discard most-expensive unplayable card" path now goes live (Set II adds hero-upgrade/Instant cards the bot can't play) and must guard the discard fee / DiscardFromHand return.
- **Art stays blockout tier** (premium art DEFERRED to M7 per Jonathan 2026-07-04). New units/buildings get `SM_<CardID>` blockout meshes; Militia Mob MAY reuse SM_Footman and Longbowman MAY reuse SM_Archer to save art (set the BP VisualMesh accordingly; the ghost-preview falls back to a sphere for a missing SM_<CardID>). Masons + the four hero upgrades are Instant — no actor, no mesh.
- Unchanged laws still in force: stats in DT_Cards/cards.csv (never hardcode a table stat); `Variant_*` donors READ-ONLY (UMG duplicate-and-rewire); `L_Arena` is the arena; local player always Blue / bot always Red; spawn by composed BP path `/Game/Blueprints/Units|Buildings/BP_*_<CardID>` (null-safe); seed-then-bind for UI; delegate `FOn<Owner><Event>` naming; **no shadowing inherited reflected UPROPERTYs** (C4457/58/59 hard errors — QA must scan every code task); only ONE editor-mutating task at a time.

### M4 WATCH items (confirm at playtest / carry as needed)
- **Transient Blue unit at match start** (M3 carry-forward a): cold-boot reconfirm at Jonathan's M4 playtest; becomes a PlayAgain/world-teardown task only if it recurs.
- **M2 TASK-041 visual hand UI** still open — shares WBP_HUD with TASK-064; do 041 first or fold to avoid a colliding UMG pass.

### TASK-053 — Card data: FCardRow keyword columns + cards.csv Set II rows + M4 test deck (files)
- assignee: gameplay-programmer
- status: done (committed 65861ce via TASK-068, not pushed). qa-passed (qa/TASK-053-report.md — 0 blk/0 warn/0 nit). CONFIRMED: header↔FCardRow 1:1 name+order (TASK-061 reimport will warn zero); DeckCount = exactly 50 (each ≥1 ≤MaxCopies); all 16 Set II rows §4 character-exact; 6 core rows byte-unchanged; keyword-column sparsity clean; no enum adds/shadowing. Data foundation for TASK-054-060. handoffs/TASK-053.md.
- blocked-by: none
- parallel-safe: yes
- spec: >
    Files only, no editor/no compile. (1) FCardRow (CardRow.h) — add nine UPROPERTYs (names must match CSV
    headers 1:1 for reimport; CONVENTIONS registry): bool bCharge=false; bool bSlayer=false; bool
    bSuicide=false; int32 SwarmCount=0; float AoERadius=0.f; float MinRange=0.f; FName SpawnCardID=NAME_None;
    float SpawnInterval=0.f; float Lifetime=0.f. The ECardType (Unit/Building/Economy/Spell/HeroUpgrade/
    Utility) and ECardProfile (None/Standard/Siege/Support) enums ALREADY exist — do not add enum values.
    (2) Docs/Data/cards.csv — append the nine columns to the header AND all six existing rows (defaults
    false/0/None; no behavior change to core cards). Then add the sixteen Set II rows EXACTLY per GDD §4:
    MilitiaMob (DisplayName "Militia Mob"): Unit, Cost 5, Max 6, HP 25, Dmg 6, Range 120, Cadence 1.0, Speed
    400, Standard, SwarmCount 4 (spawns 4 × 25 HP for one cost).
    Pikeman: Unit, 5, 6, HP 100, Dmg 30, Range 120, Cadence 1.5, Speed 350, Standard, bSlayer true.
    Sapper: Unit, 5, 4, HP 60, Dmg 80, Range 120, Cadence 1.0, Speed 500, Siege, AoERadius 250, bSuicide true.
    Cavalry: Unit, 7, 4, HP 140, Dmg 20, Range 120, Cadence 1.0, Speed 600, Standard, bCharge true.
    Longbowman: Unit, 6, 4, HP 70, Dmg 18, Range 1200, Cadence 1.5, Speed 300, Standard, bRanged true.
    Cleric: Unit, 6, 3, HP 90, Dmg 8, Range 400, Cadence 1.0, Speed 350, Support (Dmg 8 = heal HP/sec, §3.8;
    no attack).
    Ogre: Unit, 12, 2, HP 500, Dmg 35, Range 120, Cadence 1.5, Speed 250, Siege.
    BombTower (DisplayName "Bomb Tower"): Building, 8, 4, HP 180, Dmg 25, Range 800, Cadence 2.5, Speed 0,
    None profile, bRanged true, AoERadius 250.
    BallistaTower (DisplayName "Ballista Tower"): Building, 7, 4, HP 120, Dmg 45, Range 1400, Cadence 3.0,
    Speed 0, None, bRanged true, MinRange 300.
    Barracks: Building, 10, 3, HP 250, Dmg 0, Range 0, Cadence 0, Speed 0, None, SpawnCardID Footman,
    SpawnInterval 8, Lifetime 60.
    DeepMine (DisplayName "Deep Mine"): Economy, 15, 2, HP 200, Dmg 0, Range 0, Cadence 0, Speed 0, None
    (+2 gold/s income is a UPROPERTY on ADeepMine, not a column).
    Masons: Utility, 8, 3, all stats 0, None (Instant; 300 HP / 10 s is a UPROPERTY on the effect).
    SharpenedBlade (DisplayName "Sharpened Blade"): HeroUpgrade, 6, 2, all stats 0.
    PlateArmor (DisplayName "Plate Armor"): HeroUpgrade, 6, 2, all stats 0.
    SwiftBoots (DisplayName "Swift Boots"): HeroUpgrade, 5, 1, all stats 0.
    WarBanner (DisplayName "War Banner"): HeroUpgrade, 8, 1, all stats 0.
    (3) Redistribute the DeckCount column across all 22 rows into the M4 test deck (design ruling above) —
    must sum to exactly 50, each ≤ MaxCopies, ≥1 per card. Acceptance: CSV header maps 1:1 onto FCardRow
    (reimport at TASK-061 must warn zero); exactly 22 rows; DeckCount sums to exactly 50; every §4 value
    character-exact; keyword columns set ONLY where §4 specifies; no stat invented outside the GDD.
- names: >
    Source/GitClaudeUnrealTest/Siegebound/CardRow.h (FCardRow — new UPROPERTYs bCharge, bSlayer, bSuicide,
    SwarmCount, AoERadius, MinRange, SpawnCardID, SpawnInterval, Lifetime); Docs/Data/cards.csv. Row CardIDs
    (PascalCase, no spaces): MilitiaMob, Pikeman, Sapper, Cavalry, Longbowman, Cleric, Ogre, BombTower,
    BallistaTower, Barracks, DeepMine, Masons, SharpenedBlade, PlateArmor, SwiftBoots, WarBanner. Import
    target (TASK-061): /Game/Data/DT_Cards.

### TASK-054 — New targeting profiles: Siege + Support + Siege damage type & castle/building 200% (files)
- assignee: gameplay-programmer
- status: done (committed 65861ce via TASK-068, not pushed). qa-passed (qa/TASK-054-report.md — 0 blk/0 warn/3 nit; both interp calls ACCEPTED [Siege nearest-building map-wide, Support fallback follows combat units]). NON-REGRESSION CONFIRMED: Standard/melee/ranged/miners byte-identical M3 (Profile dispatch returns before untouched Standard body); castle Siege×2 before Projectile×0.5-castle-only; building Siege×2 branch, ScaledDamage==ActualDamage for non-Siege. Cleric heals 8/s≤400 clamped, never attacks. FreezeAI stops healing. C4458 clean, TASK-055 seam clean. handoffs/TASK-054-programmer.md. Ready for TASK-068 batch.
- blocked-by: TASK-053
- parallel-safe: yes (parallel with TASK-057 — different files; TASK-055 serializes AFTER it on SummonedUnit)
- spec: >
    Files only. GDD §3.8 (Siege + Support profiles) + §3.0 (Siege 200%). Large but coherent — if the session
    runs long, hand off Siege first, then Support. (1) DamageTypes.h/.cpp: add USiegeDamageType_Siege
    (UDamageType subclass, no logic — CONVENTIONS damage-type registry). (2) ACastle::TakeDamage AND
    ABuilding::TakeDamage: read DamageEvent.DamageTypeClass — USiegeDamageType_Siege = 200%; keep
    Projectile 50% / Melee/default 100% (castle only had this from M2 — ADD the Siege branch to both; buildings
    otherwise still take listed damage). Leave the TODO(Spell 50%) marker for M5. (3) ASummonedUnit — implement
    the two profiles the M2 state machine stubbed (only Standard shipped): SIEGE profile (Ogre, Sapper): ignore
    units and the hero entirely in Acquire; target the nearest enemy ABuilding in path, else the enemy castle
    (§3.8); tag its dealt damage with USiegeDamageType_Siege. SUPPORT profile (Cleric): never attacks; in
    Acquire, follow the nearest DAMAGED friendly ASummonedUnit within Range (row 400) and heal row Damage
    (8) HP/sec continuously while in range (heal = clamp to MaxHP, no friendly-fire); if no friendly is
    damaged, follow the nearest friendly combat unit (§3.8). Standard profile byte-unchanged. FreezeAI must
    also stop Siege pathing and Support following/healing. Do NOT disturb the -90 yaw VisualMesh, the M1
    melee/lunge, or the M2 ranged path. Acceptance: an Ogre walks past enemy units without engaging and hits
    the first wall/tower then the castle, dealing 200% to castle/buildings; a Cleric follows and heals the
    nearest damaged friendly at 8 HP/s within 400, heals no enemies, never attacks; Standard units unchanged.
- names: >
    USiegeDamageType_Siege in Source/GitClaudeUnrealTest/Siegebound/DamageTypes.h/.cpp. ACastle
    (Siegebound/Castle.h/.cpp), ABuilding (Siegebound/Building.h/.cpp) — Siege 200% scaling in TakeDamage.
    ASummonedUnit (Siegebound/SummonedUnit.h/.cpp) — Siege + Support profile branches; reads ECardProfile
    (CardRow.h). Data rows: Ogre, Sapper (Siege), Cleric (Support).

### TASK-055 — Standard keywords: Charge / Slayer / Swarm + damage-multiplier infra + AoE radial helper + Sapper suicide (files)
- assignee: gameplay-programmer
- status: done (committed 65861ce via TASK-068, not pushed). qa-passed (qa/TASK-055-report.md — 0 blk/0 warn/3 nit). CONFIRMED: non-keyword ComputeOutputDamage bit-for-bit AttackDamage; ONE-hit charge (bChargePrimed consumed once); SINGLE detonation (bDetonated+bDead guards); drift-free aura (multiplier separate, restore 1.0f, FreezeAI/EndPlay clear); NO friendly-fire radial (GetTeamId==Team sole authority, closest-point reaches fortifications, routes through TakeDamage so Siege fires). Downstream signatures verified: FSiegeCombatStatics::ApplyRadialDamage (→056), ASummonedUnit::SetAuraDamageBonus (→058). No Build.cs. C4458 clean. NIT: fold the 3 closest-point mirrors into SiegeCombatStatics later (qa/TASK-026 NIT-4). handoffs/TASK-055.md. Ready for TASK-068 batch.
- blocked-by: TASK-053, TASK-054 (serialize — shares SummonedUnit files; needs USiegeDamageType_Siege)
- parallel-safe: no (SummonedUnit serial hub; TASK-056 and TASK-058 depend on symbols added here)
- spec: >
    Files only. GDD §3.0 keywords + §4. Centralize ASummonedUnit damage OUTPUT so every modifier composes in
    one place: dealt = base row Damage × Charge × Slayer × AuraBonus (Siege 200% is applied fortification-side
    by TASK-054's damage type, NOT here). (1) CHARGE (bCharge, Cavalry): track uninterrupted-movement seconds;
    the first attack after ≥ ChargeMoveSeconds (2.f // GDD §3.0) of continuous movement deals ×ChargeMultiplier
    (2.f); reset the timer when the unit is blocked/attacking. (2) SLAYER (bSlayer, Pikeman): ×SlayerMultiplier
    (2.f) vs any target whose MaxHP ≥ SlayerHPThreshold (150.f // GDD §3.0). (3) AURA HOOK (for War Banner,
    TASK-058): SetAuraDamageBonus(float Bonus, float Duration) (BlueprintCallable) — a temporary additive
    output multiplier applied by a friendly hero's War Banner; refresh-not-stack; restore exactly on expiry;
    FreezeAI clears it (same drift-free discipline as ApplyMoveSpeedBuff). (4) AoE radial helper: a shared
    static ApplyRadialDamage(World, InstigatorController, Team, Center, Radius, Damage, DamageTypeClass) that
    damages all ENEMY combat actors within Radius (no friendly fire) — authored once here, reused by Bomb Tower
    (TASK-056). (5) SUICIDE (bSuicide, Sapper): a Siege+bSuicide unit, on reaching attack range OR on death,
    calls ApplyRadialDamage(AoERadius 250, Damage 80, Siege type) at its location then destroys itself — a
    single detonation, no repeat attacks. Acceptance: a Cavalry's first post-charge hit deals 40 (2×20) then
    reverts to 20; a Pikeman deals 60 to a 200-HP Knight but 30 to an 80-HP Footman; a Sapper explodes once on
    contact for 80 AoE in 250 (2× vs castle/buildings via Siege type) and dies; the aura hook adds then cleanly
    removes its bonus; nothing stat-like hardcoded that lives in DT_Cards.
- names: >
    ASummonedUnit (Siegebound/SummonedUnit.h/.cpp) — Charge/Slayer output multipliers, SetAuraDamageBonus,
    bSuicide detonation; UPROPERTYs ChargeMoveSeconds, ChargeMultiplier, SlayerMultiplier, SlayerHPThreshold.
    Shared ApplyRadialDamage static (Siegebound/ — e.g. SiegeCombatStatics or on DamageTypes.cpp). Reads
    bCharge/bSlayer/bSuicide/AoERadius [TASK-053]; uses USiegeDamageType_Siege [TASK-054]. Data rows: Cavalry,
    Pikeman, Sapper, Militia Mob (SwarmCount consumed spawn-side in TASK-059/060).

### TASK-056 — New towers: Bomb Tower (AoE) + Ballista Tower (min-range blind spot) (files)
- assignee: gameplay-programmer
- status: done (committed 65861ce via TASK-068, not pushed). qa-passed (qa/TASK-056-report.md — 0 blk/0 warn/2 nit). CONFIRMED: single-target/ArrowTower byte-unchanged (trailing default InAoERadius=0, sole 4-arg Archer caller binds, AoE branch gated AoERadius>0, ArrowTower MinRangeSq==0 inert); Bomb AoE 25-in-250 via ApplyRadialDamage (7-arg sig matches, closest-point catches primary); Ballista MinRangeSq skip ring [300,1400] max-gate intact; NO friendly fire (Bomb carries tower Team). Row-driven ATower (both BPs parent ATower, TASK-063). No Build.cs. C4458 clean. handoffs/TASK-056.md. Ready for TASK-068 batch.
- blocked-by: TASK-053, TASK-055 (reuses ApplyRadialDamage; AProjectile AoE)
- parallel-safe: yes (parallel with TASK-057/058 — different files, once 055 lands)
- spec: >
    Files only. GDD §3.7/§4. Both are ATower-family buildings driven by DT_Cards; nothing stat-like on the BP.
    (1) AProjectile: add AoERadius support — an InitProjectile overload (or param) carrying AoERadius; on impact,
    if AoERadius > 0 call ApplyRadialDamage (TASK-055) at the impact point instead of single-target, then
    destroy + spawn the NS_Damage impact (existing donor). (2) Bomb Tower behavior: ATower firing every Cadence
    (row 2.5 s) at the nearest enemy unit/hero within Range (800); fires an AoE projectile (row Damage 25,
    AoERadius 250, Projectile type) — anti-swarm splash (§3.7 "damages all units within its 250-unit impact
    radius"). (3) Ballista Tower behavior: long-range single-target — nearest valid enemy within Range (1400)
    but OUTSIDE MinRange (row 300 — the blind spot, §4); fires every Cadence (3.0 s) for row Damage 45
    (Projectile type). A target inside 300 is ignored (re-scan next cadence). Both: no friendly fire; reuse the
    M2 ATower acquire/idle loop; stats all from DT_Cards. Decide in-code whether Bomb/Ballista are ATower with
    row-driven branching (AoERadius>0 / MinRange>0) or thin ATower subclasses — prefer row-driven ATower so no
    new class is needed unless behavior demands it; record the choice in the handoff. Acceptance: a Bomb Tower
    hits a 4-unit cluster with one 25-damage blast in 250 every 2.5 s; a Ballista hits a target at 1200 for 45
    but ignores one at 250 and fires every 3.0 s; both ignore friendlies and read all stats from the table.
- names: >
    AProjectile (Siegebound/Projectile.h/.cpp) — AoERadius on InitProjectile, reuses ApplyRadialDamage
    [TASK-055]. ATower (Siegebound/Tower.h/.cpp) — Bomb/Ballista behavior (row-driven AoERadius/MinRange), or
    new subclasses if required (record in handoff). Data rows: BombTower, BallistaTower. BP children (TASK-063):
    /Game/Blueprints/Buildings/BP_Building_BombTower, BP_Building_BallistaTower.

### TASK-057 — New buildings: Barracks spawner + Deep Mine economy (files)
- assignee: gameplay-programmer
- status: done (committed 65861ce via TASK-068, not pushed). qa-passed (qa/TASK-057-report.md — 0 blk/1 warn/3 nit). M2/M3 miner economy BYTE-PRESERVED (symbol-traced: FlatIncomePerTick isolated, GetGoldRate base+miner unchanged, flat not overtime-doubled, ResetEconomy zeroes it, miner cap never gates DeepMines). Freeze step-2b CLEAN (additive, no double-handle, synchronous). WARN: mis-teamed DeepMine idle retry timer (bounded, never in designed flows). C4458 clean. CARRY→TASK-059: route Economy-typed DeepMine down building spawn path. handoffs/TASK-057-programmer.md. Ready for TASK-068 batch.
- blocked-by: TASK-053
- parallel-safe: yes (parallel with TASK-054/055 — different files; new class pairs + a PlayerState income addition)
- spec: >
    Files only. GDD §4/§8 + §3.3 economy pattern. Two ABuilding subclasses, DT_Cards-driven, team-resolved
    economy via GetPlayerStateForTeam (TASK-043). (1) ABarracks : ABuilding — on BeginPlay start a repeating
    timer of row SpawnInterval (8 s); each tick spawn the composed BP for row SpawnCardID (Footman) —
    /Game/Blueprints/Units/BP_Unit_Footman — Team = own team (team material via TASK-044), at a small offset in
    front of the Barracks on the navmesh; after row Lifetime (60 s) destroy self (its spawned units persist).
    FreezeAI/match-end must stop the spawn timer (hook the existing freeze sweep). (2) ADeepMine : ABuilding —
    raidable economy building (§8): on BeginPlay register +DeepMineIncome (UPROPERTY 2 // GDD §8, +2 gold/s) on
    the owning team's ASiegePlayerState IMMEDIATELY (no walk, unlike Miner); on death unregister it. Reuse/extend
    the PlayerState income API (AddMinerIncome pattern → a generic AddIncome(int32)/RemoveIncome(int32) or a
    DeepMine-specific pair) so GetGoldRate composes base + miners + deep mines; the miner cap does NOT apply to
    Deep Mines. 200 HP, destructible, blocks pathing like a building. Acceptance: a Barracks spawns one Footman
    every 8 s of its own team, stops + is destroyed at 60 s, and freezes at match end; a Deep Mine raises the
    owner's rate by +2/s the instant it is placed and drops it by 2 when destroyed; the miner cap is unaffected;
    all values from DT_Cards / the documented UPROPERTY.
- names: >
    ABarracks in Source/GitClaudeUnrealTest/Siegebound/Barracks.h/.cpp; ADeepMine in
    Source/GitClaudeUnrealTest/Siegebound/DeepMine.h/.cpp — UPROPERTY DeepMineIncome. ASiegePlayerState
    (Siegebound/SiegePlayerState.h/.cpp) — income API extension for non-miner income. Uses GetPlayerStateForTeam
    [TASK-043]. Spawns /Game/Blueprints/Units/BP_Unit_Footman. BP children (TASK-063):
    /Game/Blueprints/Buildings/BP_Building_Barracks (ABarracks), BP_Building_DeepMine (ADeepMine).

### TASK-058 — Hero upgrade system + War Banner aura (files)
- assignee: gameplay-programmer
- status: done (committed 65861ce via TASK-068, not pushed). qa-passed (qa/TASK-058-report.md — 0 blk/0 warn/1 nit). Hero non-regression byte-preserved @ 0 stacks: sprint 500/750 (ApplyMovementSpeed single live-writer + bSprinting; SwiftBoots composes 625/937.5), melee 20 (getter not hardcoded, TASK-016/017 feedback intact), HP 200+regen (all clamps route through GetEffectiveMaxHP, PlateArmor heals 100 clean), Rally untouched. 4 upgrades work, DRIFT-FREE (only 4 int32 stacks mutate), refund contract clean, caps from MaxCopies. WarBanner aura signature matches TASK-055. PlayAgain-reset gap VERIFIED clean 059 carry (ResetHero is shared respawn+PlayAgain path). NIT: 2 direct MaxWalkSpeed writes (HandleDeath/ctor) provably consistent. handoffs/TASK-058-programmer.md. Ready for TASK-068 batch. ⚠️CARRY→TASK-059: add Hero->ResetUpgrades() in SiegeGameMode::PlayAgain.
- blocked-by: TASK-053, TASK-055 (uses the SetAuraDamageBonus unit hook)
- parallel-safe: yes (edits HeroCharacter + a new component — no other M4 task edits HeroCharacter)
- spec: >
    Files only. GDD §3.10 (hero upgrades) + §4 (War Banner aura). Implement the four Instant hero upgrades as a
    hero-side upgrade state (a UHeroUpgradeComponent on AHeroCharacter, or upgrade state on the hero — choose,
    record in handoff). ApplyUpgrade(FName UpgradeCardID) (BlueprintCallable, called by TASK-059's Instant path):
    (1) SharpenedBlade → +MeleeDamageBonus (10 // GDD §3.10) per stack, cap 2; (2) PlateArmor → +MaxHPBonus
    (100) per stack AND heal 100 immediately, cap 2; (3) SwiftBoots → +MoveSpeedBonus (25% // to base move AND
    sprint) per stack, cap 1; (4) WarBanner → enable an aura (600 radius, +20% friendly-unit damage // GDD §4),
    cap 1 — periodically (or on a timer) call SetAuraDamageBonus(0.20, ...) on friendly ASummonedUnit within
    RallyRadius-style 600 (like Rally's speed buff, TASK-042). Stack caps = MaxCopies (read from DT_Cards).
    Return a bool/enum so the caller can refund an over-cap play (§3.0 refund). Upgrades PERSIST through hero
    death (re-apply cumulative mods on respawn) and RESET on match end / Play Again. Broadcast
    FOnHeroUpgradesChanged (active upgrades + stack counts) for the §7 HUD icon row (TASK-064). Values are
    UPROPERTY // GDD §3.10 defaults, never CSV. Do NOT regress M1/M2 hero movement, the 20-dmg cone melee, the
    200 HP base, regen, or M3 Rally. Acceptance: one Sharpened Blade makes the cone melee deal 30, a second 40,
    a third refused + refunded; Plate Armor takes maxHP 200→300 and heals 100 now; Swift Boots raises move/
    sprint 25%; War Banner gives friendly units within 600 +20% damage; all survive respawn and reset on Play
    Again; the delegate reports the upgrade row.
- names: >
    AHeroCharacter (Siegebound/HeroCharacter.h/.cpp) + UHeroUpgradeComponent (Siegebound/
    HeroUpgradeComponent.h/.cpp, if used) — ApplyUpgrade, ResetUpgrades; UPROPERTYs MeleeDamageBonus,
    MaxHPBonus, MoveSpeedBonus, WarBannerAuraRadius, WarBannerDamageBonus; delegate FOnHeroUpgradesChanged
    (member OnHeroUpgradesChanged). Uses ASummonedUnit::SetAuraDamageBonus [TASK-055]; reads MaxCopies (stack
    cap) from DT_Cards. Upgrade CardIDs: SharpenedBlade, PlateArmor, SwiftBoots, WarBanner. HUD consumer:
    TASK-064.

### TASK-059 — Play v3: Instant/upgrade routing + Masons castle-heal + Swarm multi-spawn + stack-cap refund (files)
- assignee: gameplay-programmer
- status: done (committed 65861ce via TASK-068, not pushed). qa-passed (qa/TASK-059-report.md — 0 blk/2 warn/2 nit; both flagged items ACCEPTED). CONFIRMED: melee-suppression intact all exit paths; net-zero refund all branches (Swarm unwinds copies on fail); existing Unit/Building/Miner/placement-v2 byte-preserved; apply-then-spend SAFE (CanAfford pre-check, ApplyUpgrade first, SpendGold only on Applied); SpawnUnitSwarm clean static (TASK-060 consumes as-is); IsBuildingCard routes DeepMine→building path (not miner-capped), Miner still unit; Masons 300/10 clamp+cancel clean. WARNs benign (post-match Masons tick on winner castle out-of-scope; off-navmesh ring fallback). C4458 clean. handoffs/TASK-059.md. Ready for TASK-068 batch.
- blocked-by: TASK-053, TASK-055 (SwarmCount), TASK-058 (ApplyUpgrade)
- parallel-safe: yes (edits SiegePlayerController + a Castle heal-over-time; serialize after TASK-058 only for the symbol)
- spec: >
    Files only. GDD §3.5/§3.6/§3.10 + §3.0 refund. Extend ASiegePlayerController's play path (built in
    TASK-023/030). (1) INSTANT routing: CardType HeroUpgrade or Utility resolves immediately with NO placement
    step (§3.5) — spend row Cost, apply the effect, then ConfirmPlayFromHand(Slot) to draw a replacement. Hero
    upgrades → ApplyUpgrade (TASK-058); if over stack cap, refuse with NO spend and OnCardRefused("… at max
    stacks") (full-refund rule, §3.10). (2) MASONS (Utility Instant): restore 300 castle HP over 10 s to the
    friendly castle — add ACastle::HealOverTime(float Total, float Duration) (or a heal component) clamped to
    MaxHP, broadcasting the existing FOnCastleHPChanged; Masons values are UPROPERTY // GDD §4. (3) SWARM
    multi-spawn: when confirming a Unit card with SwarmCount > 0, spawn SwarmCount copies in a 300-unit circle
    (UPROPERTY SwarmSpawnRadius // GDD §3.0) around the placement point (navmesh-projected each), for ONE Cost —
    reuse a shared spawn path so the bot (TASK-060) matches. (4) Preserve the M2 melee-suppression law on every
    exit path, and the net-zero refund discipline. Acceptance: playing Sharpened Blade deducts 6 and buffs the
    hero with no placement; a 3rd Sharpened Blade is refused with a message and unchanged gold; Masons deducts 8
    and heals the friendly castle 300 over 10 s (never over MaxHP); Militia Mob spawns 4 units in a 300 circle
    for one cost of 5; unaffordable/over-cap plays never move gold.
- names: >
    ASiegePlayerController (Siegebound/SiegePlayerController.h/.cpp) — Instant routing, Swarm multi-spawn,
    stack-cap refund via OnCardRefused; UPROPERTY SwarmSpawnRadius. ACastle (Siegebound/Castle.h/.cpp) —
    HealOverTime. Uses ApplyUpgrade [TASK-058], SwarmCount/AoERadius rows [TASK-053], ConfirmPlayFromHand
    [TASK-022]. Instant CardIDs: SharpenedBlade, PlateArmor, SwiftBoots, WarBanner, Masons.

### TASK-060 — Bot v2: play Set II + treat upgrades/instants as discard + rule-4 discard hardening (files)
- assignee: gameplay-programmer
- status: done (committed 65861ce via TASK-068, not pushed). qa-passed (qa/TASK-060-report.md — 0 blk/1 warn/1 nit). Invariants CONFIRMED (never unaffordable, never Blue-half center); M3-core byte-preserved; rule-4 fix GENUINE (closes TASK-046 WARN-2 — no double/0 charge); SwarmCount via SpawnUnitSwarm (4 Red copies, unwind on fail). Type-unplayable-only discard RULED ACCEPTABLE (no deadlock — 50g start, income accrues; excluding banked units preserves "growing Ogre waves"). WARN: swarm-fan never-Blue-half contingent on Center.X≥radius (holds in L_Arena centerline 350; same shared-helper property as player/TASK-059; hard-clamp optional). handoffs/TASK-060.md. Ready for TASK-068 batch.
- blocked-by: TASK-054, TASK-055, TASK-056, TASK-057, TASK-059 (needs every card behavior + the play/spawn paths)
- parallel-safe: yes (bot-internal — only edits SiegeBotController)
- spec: >
    Files only. Extend ASiegeBotController::EvaluateDecisions (TASK-046) for Set II by the SAME §4 ordered rules
    — no new fuzzy logic. (1) Rule 3 "most expensive affordable UNIT" now naturally reaches Ogre/Cavalry/
    Pikeman/etc.; rule 1 defensive plays may now use the new towers; rule 2 economy may use Deep Mine as well as
    Miner where the rule fits (keep it simple — Deep Mine counts as an economy play, no miner-cap interaction).
    Bot spawns honor SwarmCount (Militia Mob → 4 copies) via the shared spawn path (TASK-059), Team=Red, team
    material (TASK-044), on a navmesh-projected own-half (X≥0) clearance-valid point. (2) The bot has NO hero →
    HeroUpgrade / Utility(Masons) / Spell / Instant cards are treated as DISCARDS (§4 M4 extension) — never
    "played". (3) HARDEN rule 4 (folds M3 TASK-046 WARN-2, now live because Set II adds cards the bot cannot
    play): guard the 1-gold discard fee (gold ≥ 1) and check the DiscardFromHand result before charging;
    "most-expensive unplayable card" selection must correctly classify hero-upgrade/Instant/unaffordable cards
    as unplayable. Keep the invariants: NEVER play a card it can't afford; NEVER spawn on the Blue half; exactly
    one LogSiegeBot line per fired rule. Acceptance (§4): player idle → bot builds economy then attacks with
    growing Set II waves (incl. Ogres); player pushes → a defensive play within 2 s; the bot discards
    hero-upgrade/Instant cards it draws (logged) and never plays them; it never plays an unaffordable card,
    never spawns on the Blue half, and the discard fee is never double-charged or charged at 0 gold.
- names: >
    ASiegeBotController (Siegebound/SiegeBotController.h/.cpp) — EvaluateDecisions Set II extension + rule-4
    hardening; log category LogSiegeBot. Uses UDeckComponent [TASK-022], economy [TASK-024/043], placement
    validity + shared Swarm spawn path [TASK-030/059], team material [TASK-044]. Spawns Team=Red BP_Unit_*/
    BP_Building_* by CardID.

### TASK-061 — DT_Cards reimport: 22-row Set II + M4 test deck (editor)
- assignee: gameplay-programmer
- status: done (committed e586699 via TASK-069, not pushed). Integrated 2026-07-04 (editor UP PID 6172). DT_Cards rebuilt to 22 rows (reference-safe add/set_rows from cards.csv, GUID+linkage preserved). VERIFIED: 22 rows no legacy/dupes, DeckCount sum=50, keyword cols set only where §4 (swarmCount 4 MilitiaMob; aoERadius 250 Sapper+BombTower; minRange 300 Ballista; bSlayer Pikeman/bCharge Cavalry/bSuicide Sapper; Barracks spawn triple; profiles Siege Sapper/Ogre, Support Cleric). BOOT NOTE: hung on stale Saved/Autosaves PackageRestoreData modal — declined restore + relaunched clean (~13min, no git residue). WATCH: live CSV auto-reimport → TASK-069 re-confirm 22 rows + save-clean before commit. handoffs/TASK-061.md.
- blocked-by: TASK-053; TASK-068 (FCardRow columns compiled)
- parallel-safe: no (editor)
- spec: >
    Editor/MCP work. Reimport Docs/Data/cards.csv into the existing /Game/Data/DT_Cards (FCardRow now carries
    the nine M4 columns). Zero import warnings. Verify all 22 rows (6 core + 16 Set II) match GDD §4 exactly,
    keyword columns present and set only where specified, and DeckCount sums to exactly 50 (the M4 test deck).
    Keep the CSV as the import source (§3.0). If MCP lacks a reimport verb (M3/TASK-031 lesson), use the same
    reference-safe in-place row rebuild that preserves the asset GUID + CSV linkage. Acceptance: GetRow on each
    of the 22 CardIDs returns exact §4 values incl. the new columns; no legacy/extra rows; DeckCount sum = 50;
    no warnings.
- names: >
    /Game/Data/DT_Cards (source Docs/Data/cards.csv, row struct FCardRow). 22 rows: Footman, Archer, Knight,
    Miner, ArrowTower, Wall + MilitiaMob, Pikeman, Sapper, Cavalry, Longbowman, Cleric, Ogre, BombTower,
    BallistaTower, Barracks, DeepMine, Masons, SharpenedBlade, PlateArmor, SwiftBoots, WarBanner.

### TASK-062 — BP_Unit_* for the 7 Set II units (editor)
- assignee: gameplay-programmer
- status: done (committed e586699 via TASK-069, not pushed). Integrated 2026-07-04 (editor UP PID 6172). 7 BPs in /Game/Blueprints/Units/ (MilitiaMob/Pikeman/Sapper/Cavalry/Longbowman/Cleric/Ogre), all ASummonedUnit, Footman recipe (-90 yaw, slot0 MI_TeamColor_Blue, no stats on BP), compiled clean. SIE stat verify PASS vs cards.csv: Ogre 500/250 Siege, Cavalry 140/600 bCharge, Pikeman 100/350 bSlayer, Sapper 60/500 Siege bSuicide, Longbowman 70/300 bRanged, Cleric 90/350 Support, MilitiaMob 25/400. Custom SM_MilitiaMob + SM_Longbowman confirmed wired (not fallbacks). Deferred to TASK-069: Siege/heal/Charge/Slayer/Swarm combat behavior. handoffs/TASK-062.md.
- blocked-by: TASK-061; TASK-065, TASK-066 (unit meshes); TASK-068 (unit code compiled)
- parallel-safe: no (editor)
- spec: >
    Editor/MCP work. Seven BPs in /Game/Blueprints/Units/, cloning the BP_Unit_Footman recipe (Team default
    Blue, capsule sized to mesh, VisualMesh with the -90 yaw import fix per handoffs/TASK-014.md, slot 0 =
    MI_TeamColor_Blue, NOTHING stat-like on the BP — all from DT_Cards): BP_Unit_MilitiaMob (ASummonedUnit,
    CardID MilitiaMob, SM_MilitiaMob or SM_Footman per art), BP_Unit_Pikeman (ASummonedUnit, Pikeman,
    SM_Pikeman), BP_Unit_Sapper (ASummonedUnit, Sapper, SM_Sapper), BP_Unit_Cavalry (ASummonedUnit, Cavalry,
    SM_Cavalry), BP_Unit_Longbowman (ASummonedUnit, Longbowman, SM_Longbowman or SM_Archer per art),
    BP_Unit_Cleric (ASummonedUnit, Cleric, SM_Cleric), BP_Unit_Ogre (ASummonedUnit, Ogre, SM_Ogre). Acceptance
    (SIE/PIE in L_Arena): each reports its §4 HP/speed/profile; Ogre/Sapper ignore units and path to buildings/
    castle (Siege); Cleric heals a damaged friendly and never attacks; Pikeman/Cavalry show Slayer/Charge
    2× against the right targets; Militia Mob's card yields 4 units; none hit friendlies.
- names: >
    /Game/Blueprints/Units/BP_Unit_MilitiaMob, BP_Unit_Pikeman, BP_Unit_Sapper, BP_Unit_Cavalry,
    BP_Unit_Longbowman, BP_Unit_Cleric, BP_Unit_Ogre (all ASummonedUnit, CardID = suffix). Meshes
    /Game/Meshes/SM_MilitiaMob, SM_Pikeman, SM_Sapper, SM_Cavalry, SM_Longbowman, SM_Cleric, SM_Ogre (per
    TASK-065/066; MilitiaMob/Longbowman may reuse SM_Footman/SM_Archer). Material
    /Game/Materials/Instances/MI_TeamColor_Blue.

### TASK-063 — BP_Building_* for the 4 Set II buildings (editor)
- assignee: gameplay-programmer
- status: done (committed e586699 via TASK-069, not pushed). Integrated 2026-07-04 (editor UP PID 6172). 4 BPs in /Game/Blueprints/Buildings/: BombTower+BallistaTower (parent ATower, row-driven), Barracks (ABarracks), DeepMine (ADeepMine); Team-default, slot0 MI_TeamColor_Blue, BlockAll + bCanEverAffectNavigation=true pinned (§3.7). Compiled clean. SIE verify: stats bind from DT_Cards (BombTower 180/25/800/2.5/AoE250, Ballista 120/45/1400/3.0/MinRange300, Barracks 250+spawn-triple, DeepMine 200/income2); bonus — match-end freeze log "1 barracks frozen, 2 towers silenced" confirms 056/057 freeze hooks. Deferred to TASK-069: AoE splash/blind-spot/Barracks spawn+self-destruct/DeepMine rate. MCP stable. handoffs/TASK-063.md.
- blocked-by: TASK-061; TASK-067 (building meshes); TASK-068 (building code compiled)
- parallel-safe: no (editor)
- spec: >
    Editor/MCP work. Four BPs in /Game/Blueprints/Buildings/, cloning the BP_Building_ArrowTower/Wall recipe
    (Team default, slot 0 MI_TeamColor_Blue, VisualMesh collision blocks Pawns + affects navigation, nothing
    stat-like on the BP): BP_Building_BombTower (ATower or its subclass per TASK-056, CardID BombTower,
    SM_BombTower), BP_Building_BallistaTower (ATower/subclass, BallistaTower, SM_BallistaTower),
    BP_Building_Barracks (ABarracks, Barracks, SM_Barracks), BP_Building_DeepMine (ADeepMine, DeepMine,
    SM_DeepMine). Match the parent class each task chose (record any subclass from TASK-056 in the handoff).
    Acceptance (PIE): Bomb Tower AoE-splashes a cluster every 2.5 s; Ballista single-targets at long range with
    a 300 blind spot; Barracks spawns a Footman every 8 s and self-destructs at 60 s; Deep Mine raises the
    owner rate +2/s on placement; all from DT_Cards.
- names: >
    /Game/Blueprints/Buildings/BP_Building_BombTower, BP_Building_BallistaTower, BP_Building_Barracks,
    BP_Building_DeepMine (CardID = suffix). Meshes /Game/Meshes/SM_BombTower, SM_BallistaTower, SM_Barracks,
    SM_DeepMine (TASK-067). Material /Game/Materials/Instances/MI_TeamColor_Blue.

### TASK-064 — HUD v4: hero-upgrade icon row + stack pips (editor)
- assignee: gameplay-programmer
- status: done (committed e586699 via TASK-069, not pushed). Integrated 2026-07-04 (editor UP PID 6172). Additive to WBP_HUD: 4 TextBlocks (Blade/Plate/Boots/Banner) + UpdateUpgradeEntry/UpdateUpgradeRow/SetupUpgradeRow fns; renders "Label cur/cap", collapses at 0; caps from GetUpgradeStackCap (not guessed); seed-then-bind FOnHeroUpgradesChanged on Tick do-once. VERIFIED: M1 gold Construct byte-INTACT + Rally indicator untouched (read_graph_dsl, zero EventGraph write); compile clean; full-match PIE zero errors, row attached. Live upgrade-pip → TASK-069/Jonathan (no MCP play-inject). MCP survived ~60 calls (1 ICE cascade handled). handoffs/TASK-064.md.
- blocked-by: TASK-058 (FOnHeroUpgradesChanged); TASK-068 (delegate compiled)
- parallel-safe: no (editor; shares WBP_HUD with the open M2 TASK-041 — do TASK-041 first or fold, per the M3 TASK-050 lesson)
- spec: >
    Editor/MCP work, ADDITIVE to WBP_HUD (guard the protected M1 gold Construct + M2/M3 additions — additive
    only, do NOT round-trip the Construct, per TASK-033/050). Add the §7 hero-upgrade icon row: one entry per
    active upgrade (SharpenedBlade / PlateArmor / SwiftBoots / WarBanner) with stack pips showing current/cap;
    seed from the hero's current upgrade state, THEN bind FOnHeroUpgradesChanged (seed-then-bind law).
    Placeholder styling is fine (premium icons are M7 — text/labels acceptable at blockout tier). Acceptance
    (PIE): playing an upgrade adds/updates its icon + pip within a frame; a 2-stack upgrade shows 2 pips; the
    row clears on Play Again; the M1 gold counter and M2/M3 HUD elements are unchanged.
- names: >
    /Game/UI/WBP_HUD (additive). Delegate FOnHeroUpgradesChanged [TASK-058]. Upgrade CardIDs: SharpenedBlade,
    PlateArmor, SwiftBoots, WarBanner.

### TASK-065 — Unit blockout meshes wave 1: SM_Cavalry, SM_Pikeman, SM_Sapper, SM_MilitiaMob (art)
- assignee: art-director
- status: done (committed e586699 via TASK-069, not pushed). Integrated 2026-07-04. 4 meshes in /Game/Meshes/ (Cavalry 804tris/208u, Pikeman 532/190, Sapper 620/169, MilitiaMob 452/149), feet-center, slot0 MI_TeamColor_Blue, ≤8k tris, distinct silhouettes, ZERO import warnings. MilitiaMob = CUSTOM (not reused Footman) → TASK-062 BP_Unit_MilitiaMob VisualMesh = /Game/Meshes/SM_MilitiaMob. Editor was free (no PIE) — no playtest disruption. FBX in Content/RawAssets/. Left untracked for TASK-069 commit. handoffs/TASK-065.md.
- blocked-by: none (Blender MCP required — confirm UP before dispatch, per the M2 protocol)
- parallel-safe: yes (Blender modeling parallel with code; the editor-IMPORT step serializes with other editor-mutating tasks — one editor instance)
- spec: >
    Blender MCP + editor import. Blockout tier only (premium art DEFERRED to M7): SM_Footman-family style
    (GDD §6 — chunky 2.5–3 heads tall, oversized weapons/hands, silhouette-readable at 15 m, beveled, ≤ 8k
    tris, no textures, team material does the color). SM_Cavalry (~200 units, mounted/charging silhouette —
    reads "fast heavy"); SM_Pikeman (~185, long pike/spear — reads "anti-tank reach"); SM_Sapper (~170,
    carrying a bomb/keg — reads "expendable demolisher"); SM_MilitiaMob (~150, small ragged peasant — reads
    "weak swarm"; MAY instead reuse SM_Footman scaled if time-constrained — note the choice in the handoff so
    TASK-062 sets the BP VisualMesh accordingly). ALL: feet-center origin, SM_Footman export orientation
    (BPs apply the -90 yaw fix), capsule-friendly, slot 0 = MI_TeamColor_Blue on import, FBX to
    Content/RawAssets/<Name>.fbx, import to /Game/Meshes/. Acceptance: meshes at the named paths, ≤ 8k tris,
    distinct silhouettes at 15 m, zero import warnings, handoff records dims + tri counts.
- names: >
    /Game/Meshes/SM_Cavalry, SM_Pikeman, SM_Sapper, SM_MilitiaMob; FBX Content/RawAssets/Cavalry.fbx,
    Pikeman.fbx, Sapper.fbx, MilitiaMob.fbx; material slot 0 /Game/Materials/Instances/MI_TeamColor_Blue.

### TASK-066 — Unit blockout meshes wave 2: SM_Ogre, SM_Cleric, SM_Longbowman (art)
- assignee: art-director
- status: done (committed e586699 via TASK-069, not pushed). Integrated 2026-07-04. SM_Ogre (1572tris, 290 tall — ROSTER's LARGEST), SM_Cleric (920, 182), SM_Longbowman (1984, 184). Feet-center, slot0 MI_TeamColor_Blue, ≤8k tris, ZERO import warnings, editor free (no disruption). Longbowman = CUSTOM (not reused Archer) → TASK-062 BP_Unit_Longbowman VisualMesh = /Game/Meshes/SM_Longbowman. FBX in Content/RawAssets/. Left untracked for TASK-069. handoffs/TASK-066.md.
- blocked-by: none (Blender MCP required)
- parallel-safe: yes (see TASK-065 note)
- spec: >
    Blender MCP + editor import. Blockout tier (premium art DEFERRED to M7), SM_Footman-family style, ≤ 8k
    tris, no textures: SM_Ogre (~280 units, massive hulking siege brute + club — the win-condition card, must
    read HUGE next to a Footman); SM_Cleric (~180, robed with a staff/holy symbol — reads "support/healer",
    NOT a fighter); SM_Longbowman (~185, tall longbow — reads "long-range archer"; MAY reuse SM_Archer if
    time-constrained — note the choice for TASK-062's VisualMesh). ALL: feet-center origin, SM_Footman export
    orientation, capsule-friendly, slot 0 = MI_TeamColor_Blue, FBX to Content/RawAssets/, import to
    /Game/Meshes/. Work priority: Ogre first (it anchors the slice). Acceptance: meshes at the named paths,
    ≤ 8k tris, distinct silhouettes at 15 m (Ogre clearly the largest unit in the roster), zero import
    warnings, handoff records dims + tri counts.
- names: >
    /Game/Meshes/SM_Ogre, SM_Cleric, SM_Longbowman; FBX Content/RawAssets/Ogre.fbx, Cleric.fbx,
    Longbowman.fbx; material slot 0 /Game/Materials/Instances/MI_TeamColor_Blue.

### TASK-067 — Building blockout meshes: SM_BombTower, SM_BallistaTower, SM_Barracks, SM_DeepMine (art)
- assignee: art-director
- status: done (committed e586699 via TASK-069, not pushed). Integrated 2026-07-04. SM_BombTower (580tris/250×250×451), SM_BallistaTower (512/250×270×500), SM_Barracks (312/400×419×349), SM_DeepMine (1064/300×305×300). Ground-center, slot0 MI_TeamColor_Blue, ≤15k tris, TIGHT authored UCX = base footprint (overhangs outside hull, plinth-lesson honored), distinct silhouettes (DeepMine headframe ≠ GoldNode crystal), ZERO import warnings, editor free. ALL M4 ART DONE (065/066/067 = 11 meshes). FBX in Content/RawAssets/. Left untracked for TASK-069. handoffs/TASK-067.md.
- blocked-by: none (Blender MCP required)
- parallel-safe: yes (see TASK-065 note)
- spec: >
    Blender MCP + editor import. Four structure blockouts (GDD §6, ≤ 15k tris each, beveled, blockout tier,
    no textures; UCX hulls kept tight to the footprint — the M1 castle-plinth lesson: oversized collision =
    placement dead zones), ground-center origin, slot 0 = MI_TeamColor_Blue: SM_BombTower (~250×250 footprint,
    ~450 tall, mortar/cauldron top — reads "lobs bombs"); SM_BallistaTower (~250×250, ~500 tall, big
    horizontal bolt-thrower — reads "long-range sniper"); SM_Barracks (~400×400 footprint, ~350 tall, tent/
    hall with a doorway — reads "spawns troops"); SM_DeepMine (~300×300, ~300 tall, mine-shaft/ore-cart — reads
    "economy building", visually distinct from the SM_GoldNode ore cluster). FBX to Content/RawAssets/, import
    to /Game/Meshes/. Acceptance: four meshes at the named paths with stated dims (±10%), tight UCX collision
    verified, distinct silhouettes, zero import warnings, handoff records dims/tris/collision per mesh.
- names: >
    /Game/Meshes/SM_BombTower, SM_BallistaTower, SM_Barracks, SM_DeepMine; FBX Content/RawAssets/BombTower.fbx,
    BallistaTower.fbx, Barracks.fbx, DeepMine.fbx; material slot 0 /Game/Materials/Instances/MI_TeamColor_Blue.

### TASK-068 — M4 code batch: compile + residue adjudication + commit (build)
- assignee: build-master
- status: done (commit 65861ce on main, parent 56247c9, NOT pushed; 47 files +4825/-257). Clean compile+LINK FIRST TRY (~16.6s, zero C4456/57/58/59 + zero C4244, 14 TUs incl. 3 new pairs SiegeCombatStatics/Barracks/DeepMine). No donor re-saves; 11 M4 art meshes+FBX unstaged→untracked for TASK-069. Editor left DOWN on 65861ce DLL (TASK-061 boots it — path tracing already disabled → fast boot). TASK-053..060 code committed here (→done; manager wraps at M4 finish). handoffs/TASK-068.md. FOLLOW-UPS (manager): optional swarm own-half hard-clamp; off-navmesh swarm fallback.
- blocked-by: TASK-053, TASK-054, TASK-055, TASK-056, TASK-057, TASK-058, TASK-059, TASK-060 (all qa-passed)
- parallel-safe: no
- spec: >
    Build-master. Compile the accumulated M4 C++ batch (TASK-053..060) with the standard Build.bat command;
    editor-bounce protocol (editor must release the DLL). **Pre-compile: scan the batch for inherited-reflected-
    member shadows (CONVENTIONS coding law — cost 2 loops in M2, held clean in M3).** Any compile error →
    append to the failing task's QA report, set qa-failed, stop (counts as a QA loop; build-master never edits
    code). Adjudicate any working-tree residue. Commit the M4 code batch + Docs/Data/cards.csv + pipeline docs
    (TASKBOARD/CONVENTIONS/SLACK deltas) with task IDs. Do NOT push. Acceptance: clean build; git status clean
    of unexplained residue; commit hash on the board + posted in 🔧 Build & Git.
- names: >
    Build command per CLAUDE.md. Commit pattern: "TASK-053..060: M4 Set II C++ batch (keywords, Siege/Support
    profiles, new towers/buildings, hero upgrades, play v3, bot v2)".

### TASK-069 — M4 final assembly: expanded-roster PIE verification + commit (build)
- assignee: build-master
- status: done (commit e586699 on main, parent 65861ce, NOT pushed; 44 files editor/art). ZERO M4-feature FAILs. LIVE bot Set II verified (repeated Ogre push; discards upgrades/Instants; never unaffordable/never-Blue-half; Siege army killed Blue castle 2000→0 ~41s; deck 50/6). DT_Cards 22-row reconfirmed warm+cold (DeckCount=50, save-clean). Keywords/upgrades/new-buildings DEFERRED-to-playtest (MCP no card-play inject), each backed by code QA PASS. BONUS: real-PIE Defeat screen fired → CLOSES M2 "no PlayerController end-screen" watch. Editor UP PID 10932 (cold-booted, clean). handoffs/TASK-069.md.
  - **FOLLOW-UP FOUND → TASK-070:** 3 stray verification actors (BP_Unit_Footman_C_1, BP_Unit_Miner_C_2, BP_Building_ArrowTower_C_1) are COMMITTED in L_Arena.umap since M2 (5bb9507/40b69ef) — present on ALL branches. Cause the "stray Blue unit at match start" + bot t=0 defend. Cleanup = remove 3 actors, re-save L_Arena, PIE-verify clean start.
- blocked-by: TASK-061, TASK-062, TASK-063, TASK-064 (done); TASK-068 (committed)
- parallel-safe: no
- spec: >
    Build-master. Final M4 integration in editor + Git. (1) Verify all M4 assets resolve with zero load
    warnings (DT_Cards 22 rows, 7 unit BPs, 4 building BPs, HUD upgrade row). (2) PIE-verify the §9-4 slice +
    §4 acceptance: keywords fire (Cavalry Charge 2×, Pikeman Slayer 2×, Militia Mob spawns 4, Sapper suicide
    AoE, Ogre/Sapper Siege 200% vs castle/buildings); Support Cleric heals; new towers behave (Bomb AoE,
    Ballista blind spot); Barracks spawns + expires; Deep Mine +2/s; hero upgrades apply/stack/cap/refund +
    persist through death + reset on Play Again; the bot plays Set II (incl. an Ogre push) and discards
    upgrades/Instants; the M4 test deck deals 50/6 with Set II reachable. Record pass/fail per line; fails route
    back per the QA loop. (3) Confirm the slice is RECORDABLE (Ogre push vs Bomb Tower defense clip). (4)
    Cold-boot reconfirm the M3 transient-Blue-unit WATCH. (5) Commit all M4 editor/art assets with task IDs;
    hash on the board; post in 🔧 Build & Git. Do NOT push. Flag the M4 checkpoint ready for Jonathan's
    playtest.
- names: >
    /Game/Maps/L_Arena, /Game/Data/DT_Cards, /Game/Blueprints/Units/BP_Unit_* (7), /Game/Blueprints/Buildings/
    BP_Building_* (4), /Game/UI/WBP_HUD. Handoff: .claude/pipeline/handoffs/TASK-069.md.

---

