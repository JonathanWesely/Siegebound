# Siegebound — Game Design Document

<!-- v2 (2026-07-02). Changes from v1: 50-card deck with duplicates + per-card copy caps; 6-card hand + next-card preview; card pool expanded 6 → 28 (new towers, hero upgrades, spells, spawners); Miner rebalanced (economy payback was too fast); unit targeting profiles replace hero-first priority (kiting exploit); overtime income; premium stylized art bar; milestones restructured 6 → 8. Written for the agent team: concrete numbers over adjectives, acceptance criteria per mechanic, a tiny Milestone 1, and an explicit Out of Scope. Numbers are first-pass and meant to be tuned during playtests. -->
<!-- v2.1 (2026-07-03). M1 playtest round-1: baseline combat legibility pulled forward from M7 into M1 — visible hero swing animation + per-hit impact feedback (§3.1), units telegraph attacks (§3.8), floating castle HP bars (§3.9). The full §6 juice checklist remains M7. -->
<!-- v3 (2026-07-21). AS-BUILT UPDATE PASS (Jonathan-directed). In-place corrections wherever shipped reality diverged from v2, each marked with a consistent *[as-built YYYY-MM-DD: …]* tag; §9 gains milestone statuses; a Design Change Log appendix records the major pivots. Nothing aspirational was deleted — where reality contradicts an unbuilt aspiration, the aspiration stands and the tag records the delta. Engineering law (naming, pipeline contracts, tool mechanics) lives in .claude/pipeline/CONVENTIONS.md and is only referenced here, never inlined. Section numbering unchanged (other docs and code comments reference §-numbers). -->

## 1. Overview
- **Genre:** Real-time 3D card-battler / hero-action / lane-strategy hybrid.
- **Platform:** PC (Windows), built on UE 5.8 (third-person template as the starting point).
- **Camera:** Third-person over the player's hero (default template camera to start).
- **Players:** 1v1. **Prototype through Milestone 7 is single-player / vs-bot on one machine. Networked 1v1 is Milestone 8** — all mechanics are built and tuned locally first, then made network-authoritative.
- **Pitch:** Two players each control a hero and defend a castle in a symmetric 3D arena. Gold accrues automatically; you spend it from a 6-card hand (drawn from a 50-card deck) to summon units, build towers and walls, grow your economy, upgrade your hero, and cast spells. You also fight directly as your hero. First to destroy the enemy castle wins. *[as-built 2026-07-21: the arena is castle-symmetric, but the battlefield terrain is a procedural random scatter regenerated each match — see §5.]*
- **Quality bar:** this game should *look premium* (see §6). Timeline is flexible; visual quality is not.

## 2. Core Gameplay Loop
Target loop length 30–60 seconds:
Gold accrues → play cards from the 6-card hand (spend gold) to spawn units / build defenses / add economy / upgrade the hero / cast spells → summoned units auto-path toward the enemy castle and fight using built-in AI → the player pushes or defends directly as the hero → economy grows → enemy castle HP hits 0 → win.

## 3. Mechanics
One subsection per mechanic. Acceptance criteria are how QA verifies it.

### 3.0 Global Rules (read first)
- **Data-driven stats:** every card/unit/building/spell stat in this document lives in one data table: `Docs/Data/cards.csv` (source of truth, checked into Git) imported as `DT_Cards` in `/Content/Data/`. Balance changes are CSV edits — **agents must never hardcode a stat that exists in the table.**
- **No friendly fire** anywhere: units, towers, spells, and the hero cannot damage friendly actors or their own castle.
- **Projectiles** (Archer, Longbowman, towers): homing, travel at **1500 units/s**, destroyed on impact.
- **Damage vs castle:** melee deals **100%**, `Siege` units deal **200%**, projectiles and spells deal **50%** (anti-sniping rule — castles must die to committed pushes, not chip from range).
- **Keywords** (used in the §4 tables):
  - **Siege** — targets buildings/castle only; ignores units and the hero; deals 200% damage to buildings and castle.
  - **Charge** — first attack after 2+ seconds of uninterrupted movement deals 200% damage.
  - **Slayer** — deals 200% damage to units with 150+ max HP.
  - **Swarm** — card spawns multiple copies of the unit in a 300-unit circle.
  - **Aura** — passive effect applied to friendly units within the stated radius.
  - **Chain** — attack bounces to additional nearby targets (damage per bounce stated on card).
  - **Instant** — card resolves immediately on play; no placement targeting (hero upgrades, Masons).
- **Refund rule:** any card whose effect is refused (miner cap, upgrade stack cap) refunds its full cost and shows a HUD message stating the reason.

### 3.1 Hero Movement & Combat
- WASD moves the hero; mouse controls camera/facing. Move speed **500 units/s**, sprint (hold Shift) **750 units/s**.
- LMB performs a melee attack: **20 damage** to **all** enemies in a **150-unit** forward arc (**60°** cone), **0.5 s** cooldown.
- **Attack feedback (M1+, playtest 2026-07-03):** every swing plays a visible attack animation, and every damaging hit spawns an impact effect — damage must be readable without logs. (Baseline only; the full §6 juice checklist remains M7.)
- Hero has **200 HP** base (modifiable by upgrades, §3.10). Out-of-combat regeneration: **5 HP/s** after 8 s without taking or dealing damage.
- On death, the hero respawns at its own castle after **5 s** (hero death does not lose the match — only castle destruction does). Upgrades persist through death.
- *[as-built 2026-07-14 (M6.6): terrain-climb comfort retune — step height 50, walkable slope 50°, jump velocity 600 — so the hero climbs the §5 hills. Base move/sprint speeds unchanged.]*
- *[as-built 2026-08-04 (keyboard-layout batch): every LETTER binding in the game — WASD movement, the Q rally, the T/R/E/F/C unit commands — is **positional**, not literal. On Windows the game detects the active OS keyboard layout at runtime and remaps all 26 letters so they land in the same **physical** positions as US-QWERTY; a Dvorak player presses the key printed "," where a QWERTY player presses "W" and both move forward. It is always on, has no player-facing setting, and follows a mid-session layout switch (Win+Space) within ~1 s with no reload. **Deliberately NOT remapped: the §7 hotkeys 1–6, Left Alt, Shift, Space, Enter, Escape, punctuation and the mouse** — those stay literal on every layout, so the §7 line about hotkeys 1–6 and Left Alt reads exactly as written. Non-Windows hosts pass through unchanged.]*
- **Acceptance:** hero moves at 500 u/s (750 sprinting); an LMB swing damages every enemy unit within 150 units and inside the 60° cone for 20 HP; a target at 200 units is unaffected; hero at 0 HP disappears and respawns at its castle within 5–6 s with all purchased upgrades intact; a hero at 150/200 HP untouched for 8 s begins regenerating 5 HP/s.

### 3.2 Gold Economy (passive accrual + overtime)
- Each player has a **gold** integer, starting at **10**, accruing **+1 gold per 2 s** at base rate, capped at **999**. *[as-built 2026-07-08: rebalanced from start 50 / +2 gold/s on Jonathan's urgent directive — passive income was way too high. The HUD shows the per-second average rounded up (+1/s); miner and Deep Mine per-second income untouched.]*
- **Overtime:** at match time **7:00**, base accrual doubles to **+1 gold/s** for both players (miner bonuses unchanged) and a HUD indicator appears. This forces long matches toward a conclusion. *[as-built: the doubling is a multiplier on the rebalanced base (was "doubles to +4/s" pre-rebalance).]*
- Gold is spent to play cards (§3.5) and to discard (§3.6).
- **Acceptance:** gold starts at 10, increases by ~1 per 2 s, never exceeds 999; at 7:00 the base rate becomes ~1/s (+ miner bonuses) and the overtime indicator shows; the HUD counter matches the underlying value at all times.

### 3.3 Miners (economy scaling)
- A miner is a non-combat unit spawned by the Miner card (**cost 8**). It walks to the owner's gold node and begins mining; **+1 gold/s activates only when it arrives** (~10 s walk from a mid-half placement). *[as-built: the gold nodes moved WITH the castles at every arena scale-up (§5), so the walk stays short when the miner is placed near your castle — a mid-half placement is now a much longer hike on the widened field.]*
- Miners are destructible: **30 HP**. If killed, its +1/s is removed. Economy is a raidable investment — the payback window (~18 s effective) is the point.
- Active cap **6 miners** per player. A 7th Miner card is refused per the refund rule (§3.0).
- **Acceptance:** playing a Miner deducts 8 gold and accrual rises from +2/s to +3/s only after the miner reaches the node; killing that miner returns accrual to +2/s; a 7th miner is refused with "Miner limit reached" and 8 gold refunded.

### 3.4 Deck & Hand
- A **deck is exactly 50 cards**. **Duplicates are allowed** up to each card's **Max Copies** value (per-card column in the §4 tables). The default deck (used until the deck-builder ships in M6) is:

| Card | Copies |
|---|---|
| Footman | 12 |
| Archer | 10 |
| Wall | 10 |
| Arrow Tower | 8 |
| Knight | 6 |
| Miner | 4 |
| **Total** | **50** |

*[as-built 2026-07-09 (M6): the deck-builder shipped, and this table is superseded as the DEFAULT by the curated 50 in `cards.csv` (DeckCount column): Footman 12, Archer 8, Wall 4, Knight 3, Miner 3, Arrow Tower 3, Militia Mob 3, Pikeman 3, Cavalry 3, Longbowman 2, Cleric 2, Ogre 2, Fireball 2. Players build/save their own 50-card decks (§7); the bot carries 2 curated decks.]*

- The player has a visible **hand of 6 cards**. Playing or discarding a card immediately draws the next card from the **draw pile**; played/discarded cards go to a **discard pile**; when the draw pile empties, the discard pile is shuffled into a new draw pile.
- **Next-card preview:** the HUD shows the top card of the draw pile (Clash Royale rule — enables planning).
- **Acceptance:** exactly 6 cards show in the hand; playing one immediately draws a replacement; the preview slot always shows the actual next draw; after 50 plays/discards the deck has reshuffled and keeps dealing; the default deck contains exactly the copy counts above.

### 3.5 Playing Cards
- Each card shows a **gold cost** (top-right). Cards whose cost exceeds current gold are greyed out and unplayable.
- Card categories and play behavior:
  - **Unit / Building / Economy** — clicking a playable card enters **placement mode**; clicking a valid location on the player's half spawns the actor there and deducts the cost. Units spawn at the placement point.
  - **Hero Upgrade / Utility (`Instant`)** — the card resolves immediately on click; no placement step.
  - **Spell** (Milestone 5) — clicking enters **targeting mode**; a reticle can be placed **anywhere on the map**; the spell resolves at that point. *[as-built 2026-07-21: still true for Lightning and Battle Cry; Fireball + Frost Nova now use the reticle as an AIM point for a hero-origin line (§3.11); Pickpocket resolves instantly with no reticle (global effect, M5 ruling).]*
- **Placement rules:** units/buildings/economy only on the owner's half (centerline-bounded); buildings additionally require **200 units** of clearance from any other building; invalid placement shows a red ghost and refuses the click without cost.
- **Acceptance:** a card with cost 3 is unplayable at 2 gold (greyed), playable at 3 (deducts 3 and spawns at the clicked point); placement on the enemy half is refused with no gold spent; a building placed 100 units from another building is refused; a Fireball reticle on the enemy half is accepted; an Instant card deducts gold and applies with a single click.

### 3.6 Discard
- The player may discard a card for a fixed **1 gold**; the card goes to the discard pile and a replacement is drawn. Discard is refused if gold < 1.
- **Acceptance:** clicking discard on a card with ≥1 gold removes it, deducts 1 gold, and draws a replacement; at 0 gold, discard is refused.

### 3.7 Towers & Walls (buildings)
- General tower rules: stationary; auto-target the **nearest** valid enemy in range; fire per their cadence; no friendly fire. Individual tower stats live in the §4 tables.
- **Arrow Tower** (core set): **150 HP**, targets units/hero within **900**, **15 damage** every **1.5 s**.
- **Wall:** **300 HP**, no attack; blocks unit pathing and physically collides; units path around walls; dynamically updates the navmesh.
- **Acceptance:** an Arrow Tower fires at the nearest enemy inside 900 every 1.5 s (15 dmg/hit) and ignores anything beyond 900; a wall stops units from walking through and forces rerouting; both are destructible; a Bomb Tower (M4) damages all units within its 250-unit impact radius.

### 3.8 Summoned Unit AI — targeting profiles
Every unit runs the same state machine (**Advance → Acquire → Attack → Reacquire**) with one of three **targeting profiles**:

| Profile | Valid targets | Behavior |
|---|---|---|
| **Standard** | units, hero, buildings, castle | Advance toward enemy castle; acquire **nearest** enemy within **600** aggro radius; if two targets are within 100 units of each other, prefer units/hero over buildings. |
| **Siege** | buildings and castle only | Ignores units and the hero entirely; walks to the nearest enemy building in its path, else the castle. |
| **Support** | friendly units | Follows the nearest damaged friendly unit and applies its effect (e.g., Cleric heal); never attacks. Follows the closest friendly combat unit if none are damaged. |

- **Advance:** move toward the enemy castle along the shortest valid path (UE navigation).
- **Attack:** in range, deal damage on the unit's cadence.
- **Reacquire/leash:** if the target dies or moves beyond **900 units**, resume Advance. (Deliberate change from v1: no hero-first priority — a sprinting hero must not be able to kite entire waves off-lane.)
- **Attack telegraph (M1+, playtest 2026-07-03):** every unit visibly telegraphs its attack plus an impact effect on hit. Blockout tier: a procedural lunge toward the target is acceptable until the M7 skeletal/animation pass. *[as-built 2026-07-19: the M7/M7.5 skeletal pass shipped — units play real full-body attack/walk/death/idle animations (§6); the procedural lunge survives only as the automatic null-safe fallback for any unit without a resolvable rig.]*
- **Acceptance:** a Standard unit walks toward the enemy castle, engages the nearest enemy entering 600, and resumes advancing when it dies; a Siege unit walks past enemy units without engaging and attacks the first tower/wall in its path; a Support unit follows friendlies and heals the nearest damaged one; a unit whose target sprints 900+ units away disengages and resumes Advance.

### 3.9 Castle & Win Condition
- Each castle has **2000 HP**, is destructible, and does not attack. Damage model per §3.0 (melee 100% / Siege 200% / projectiles+spells 50%).
- **Castle crumble states:** visible damage stages at **75% / 50% / 25%** HP (mesh/material swap + debris FX) — legibility and drama. *[as-built 2026-07-18 (M7): shipped as three progressive material-char stages on mesh-swap duplicates plus debris VFX — material-based damage, not geometry fracture; the castle stays team-neutral while crumbling.]*
- **Castle HP bar (M1+, playtest 2026-07-03):** each castle shows a floating HP bar above its mesh — live-updating on every HP change, hidden on destruction, restored full on Play Again. (The §7 top-of-HUD castle bars remain the M2+ HUD spec; this floating bar is the in-world baseline.)
- When a castle reaches 0 HP the match ends: Victory/Defeat screen with **Play Again** (full state reset: gold, deck, hand, units, buildings, upgrades, castle HP, match clock).
- **Acceptance:** dealing 2000 damage destroys the castle and triggers the correct end screen; an Archer volley (projectile) deals half its listed damage to the castle while an Ogre (Siege) deals double; crumble visuals appear at each threshold; Play Again restores every value in this document to its starting state; a match cannot end any other way.

### 3.10 Hero Upgrades (Instant cards, permanent for the match)
- Hero Upgrade cards apply immediately on play, persist through hero death, and reset only at match end.
- Each upgrade has a **stack cap** (= its Max Copies value). Playing one beyond its stack cap is refused per the refund rule.
- The HUD shows an icon row of active upgrades with stack pips (§7).

| Upgrade | Cost | Effect | Stack cap |
|---|---|---|---|
| Sharpened Blade | 6 | +10 melee damage | 2 |
| Plate Armor | 6 | +100 max HP, and heals 100 on play | 2 |
| Swift Boots | 5 | +25% move & sprint speed | 1 |
| War Banner | 8 | Aura (600): friendly units deal +20% damage | 1 |

- **Acceptance:** one Sharpened Blade makes the hero swing deal 30; a second makes it 40; a third is refused and refunds 6 gold; Plate Armor raises max HP 200→300 and heals 100 immediately; upgrades survive hero respawn; all reset on Play Again.

### 3.11 Spells (Milestone 5)
- Spells use targeting mode (§3.5): reticle anywhere on the map, resolve at the point, no friendly fire, **50% damage vs castle**.
- Spell VFX are a first-class deliverable (§6) — every spell gets a Niagara effect readable in one frame.
- Individual spells in the §4 Set III table.
- **Acceptance (example):** Fireball at a cluster of 3 enemy Footmen (80 HP) kills all three (100 dmg each), leaves an adjacent friendly Footman untouched, and deals 50 (not 100) if the blast overlaps the castle.

*[as-built 2026-07-21 — spell DELIVERY overhaul (Jonathan playtest directive; magnitudes, costs, castle-50% and no-friendly-fire unchanged):]*
- *Fireball + Frost Nova are HERO-ORIGIN LINE spells: targeting mode is kept, but the reticle sets the aim direction — confirmed by Jonathan, cursor aim is the delivery. The cast fires from the hero toward the cursor point and applies along a swept air line: range **900**, half-width **100**, sweep front ~3,000 u/s (all flagged playtest tunables). Line-vs-circle coverage is a playtest watch, not a balance change.*
- *The bot has no hero, so its line spells fire from its own castle toward its chosen target point (flagged design default — distant clusters beyond the 900 reach can whiff; watch item).*
- *Lightning stays a reticle-placed sky strike, bigger and taller: radius **400 → 700** (a `cards.csv` data change), the strike reads from high in the sky, and the ground-circle visual and the hitbox both scale honestly from the same data — no visual-only lying.*

## 4. Characters, Units & Card Compendium
**Player Hero** (one fixed hero for the prototype): 200 HP base, 500/750 u/s, 20-damage cleave melee (§3.1), upgradeable via §3.10. One active ability (Milestone 3+): **Rally** — nearby friendly units (600) gain +25% move speed for 5 s, 20 s cooldown, key: Q.

All stats below live in `cards.csv` (§3.0). **Cost** = gold. **Max** = max copies per 50-card deck (also the stack cap for hero upgrades).

### Core Set — Milestone 2 (6 cards)

| Card | Type | Cost | Max | HP | Dmg | Range | Cadence | Speed | Profile / notes |
|---|---|---|---|---|---|---|---|---|---|
| Footman | Unit | 3 | 12 | 80 | 12 | 120 | 1.0 s | 400 | Standard. Frontline. |
| Archer | Unit | 4 | 10 | 45 | 10 | 700 | 1.2 s | 350 | Standard. Ranged. |
| Knight | Unit | 6 | 6 | 200 | 15 | 120 | 1.2 s | 300 | Standard. Tank. |
| Miner | Economy | 8 | 4 | 30 | — | — | — | 350 | §3.3. +1 gold/s on arrival. |
| Arrow Tower | Building | 5 | 8 | 150 | 15 | 900 | 1.5 s | — | Anti-unit tower. |
| Wall | Building | 4 | 10 | 300 | — | — | — | — | Blocks pathing. |

### Set II — Milestone 4 (16 cards)

| Card | Type | Cost | Max | HP | Dmg | Range | Cadence | Speed | Profile / notes |
|---|---|---|---|---|---|---|---|---|---|
| Militia Mob | Unit | 5 | 6 | 25 ×4 | 6 | 120 | 1.0 s | 400 | Standard, **Swarm** (spawns 4). Dies to AoE. |
| Pikeman | Unit | 5 | 6 | 100 | 30 | 120 | 1.5 s | 350 | Standard, **Slayer** (2× vs 150+ HP). Anti-tank. |
| Sapper | Unit | 5 | 4 | 60 | 80 AoE (250) | contact | once | 500 | **Siege**. Explodes on contact or death. |
| Cavalry | Unit | 7 | 4 | 140 | 20 | 120 | 1.0 s | 600 | Standard, **Charge** (first hit 2×). |
| Longbowman | Unit | 6 | 4 | 70 | 18 | 1200 | 1.5 s | 300 | Standard. Outranges towers (900). |
| Cleric | Unit | 6 | 3 | 90 | heals 8/s | 400 | — | 350 | **Support**. No attack. |
| Ogre | Unit | 12 | 2 | 500 | 35 | 120 | 1.5 s | 250 | **Siege** tank. The win-condition card. |
| Bomb Tower | Building | 8 | 4 | 180 | 25 AoE (250) | 800 | 2.5 s | — | Anti-swarm splash. |
| Ballista Tower | Building | 7 | 4 | 120 | 45 | 1400 | 3.0 s | — | Long range; **blind spot inside 300**. |
| Barracks | Building | 10 | 3 | 250 | — | — | spawns | — | Spawns 1 Footman every 8 s; expires after 60 s. |
| Deep Mine | Economy | 15 | 2 | 200 | — | — | — | — | Building: +2 gold/s immediately; raidable. |
| Masons | Utility | 8 | 3 | — | — | — | — | — | **Instant**: restores 300 castle HP over 10 s. |
| Sharpened Blade | Hero Upg. | 6 | 2 | — | — | — | — | — | §3.10. |
| Plate Armor | Hero Upg. | 6 | 2 | — | — | — | — | — | §3.10. |
| Swift Boots | Hero Upg. | 5 | 1 | — | — | — | — | — | §3.10. |
| War Banner | Hero Upg. | 8 | 1 | — | — | — | — | — | §3.10. |

### Set III — Milestone 5 (6 cards)

| Card | Type | Cost | Max | Effect |
|---|---|---|---|---|
| Fireball | Spell | 7 | 3 | 100 damage in a 300 radius at the reticle. |
| Frost Nova | Spell | 6 | 3 | Freezes enemy units and towers in a 350 radius for 4 s (castle unaffected). |
| Lightning | Spell | 8 | 2 | 200 damage to the 3 highest-HP enemies in a 700 radius *(rev. 2026-07-21, was 400)*. The tower-killer. |
| Battle Cry | Spell | 5 | 3 | Friendly units in a 400 radius: +50% attack speed, +25% move speed for 8 s. |
| Pickpocket | Spell | 6 | 2 | Steal 10 gold from the opponent (up to what they have). |
| Crystal Tower | Building | 9 | 3 | 150 HP; **Chain** attack every 1.5 s, 800 range: 15 dmg primary, 10 second, 5 third. |

*[as-built 2026-07-21: Fireball + Frost Nova keep these magnitudes but deliver along a hero-origin line instead of a reticle-placed ground circle — see §3.11.]*

### Bot Opponent (Milestone 3, extended in M4/M5)
Through Milestone 2 there is no opponent (target-practice castle). The M3 bot follows explicit rules — no fuzzy "plays well":
- Accrues gold identically to the player; uses the default deck; controls **no hero**.
- **Every 2 s, evaluate in order** (play the first rule that fires):
  1. If enemy units are on the bot's half and gold ≥ cheapest affordable *defensive* play → play a unit at its centerline or a tower between the intruders and the castle.
  2. If miners < 3 and no enemy units on the bot's half and gold ≥ 8 → play Miner.
  3. If gold ≥ 12 → play the most expensive affordable unit at its centerline.
  4. If hand has an unplayable card and gold ≥ 1 → discard the most expensive card in hand.
- **M4 extension:** bot plays Set II cards by the same rules; plays hero upgrades... *(n/a — bot has no hero)* → bot treats Hero Upgrade cards as discards. **M5 extension:** bot casts Fireball at 3+ clustered player units; Lightning at a tower adjacent to 2+ units. *[as-built 2026-07-21: the bot's line spells (Fireball/Frost Nova) fire from its castle (§3.11); it also marches attack waves FROM its castle on the 10× field — no mid-field materialization (M7.6 ruling).]*
- Difficulty is a single fixed level for now.
- **Acceptance:** with the player idle, the bot reaches 3 miners and attacks with progressively larger waves; when the player pushes, the bot responds with a defensive play within 2 s; the bot never plays a card it cannot afford; a logged decision trace shows which rule fired for every play.

## 5. Levels / World
- **One symmetric arena.** Two castles at opposite ends (~4000 units apart *[as-built: 50,000 — see below]*), a mostly open battlefield, a clear centerline dividing placement halves.
- **Gold node** per side, **800 units** from each castle, glowing (emissive) — where miners gather.
- Navigation mesh covers the battlefield; walls dynamically obstruct it.
- Layout supports the §6 showcase pass: leave silhouette room for set dressing (banners, braziers, siege debris) that doesn't alter gameplay collision.
- **Win/lose:** destroy the enemy castle to win; lose if yours is destroyed.

*[as-built 2026-07-21 — the arena as it exists today, after three deliberate scale-ups (M6.5 4× → M6.6 widen → M7.6 10×):]*
- ***Scale:** castles at X = ±25,000 (**50,000 apart**; play field ≈ 52,000 × 25,000 units — a true 10× of the previous area). Unit speeds, ranges, and aggro radii were deliberately NOT scaled: long, slow, epic marches are the accepted design (the "slow-epic" pacing ruling — Jonathan's call, accepted "for now"; his W1 perf/feel watch owns the live verdict and is still pending), and forward structures (Barracks, towers, walls placed up-field) gain importance as the pacing counter-lever. The 10× field currently lives on its own branch behind a human-watch phase ladder; the mainline carries the previous ±8,000 field until the final merge gate.*
- ***Gold nodes:** still exactly 800 units in front of each castle — they moved WITH the castles at every scale-up, preserving the §3.3 miner-economy geometry.*
- ***Terrain:** a runtime PROCEDURAL scatter of trees, rocks, climbable hills, and grass, re-seeded fresh each match — organic ASYMMETRIC placement (a PvE fairness ruling; a mirror-symmetric toggle is reserved if playtests ever read unfair). Obstacles physically block movement and carve the navmesh; hills are climbable by the hero AND units (convex ≤30° faces with flat crowns — high ground is physical only, no stat bonuses); terrain blocks projectiles (arrows die on rocks/hills/trunks — an accepted balance change).*
- ***Corridor & keep-clears:** a guaranteed clear castle-to-castle corridor (half-width 1,000) plus keep-clear radii around castles, gold nodes, and spawns. A match where units cannot reach the enemy castle is a hard failure — traversability is enforced every match, by design.*
- ***Vista & POIs (M7.6 plan, in progress):** a far ring of cliff/mountain silhouettes OUTSIDE the play bounds (visual-only; the one Nanite exception — §6), 6–10 mid-field points of interest, and 2 pre-seeded NEUTRAL gold-node props mid-field — visual-only today; a neutral-node capture mechanic is a recorded future hook (§10), not designed.*

## 6. Art Style & Audio — the "premium stylized" bar
**Direction: polished stylized** (Warcraft Rumble / Fortnite quality tier, low-poly base). Chosen deliberately: achievable by the Blender→UE agent pipeline *and* premium-looking with the right materials, lighting, and juice. Timeline is flexible; this bar is not.

**Asset standards (every shipped asset):**
- Chunky readable proportions: units ~2.5–3 heads tall, oversized weapons/hands; silhouette identifiable at 15 m.
- No flat single-color materials: gradient ramps (darker feet → lighter shoulders) or hand-painted-style textures, plus rim light. One master material with a **team-color parameter** (blue/red accents).
- Beveled/chamfered edges (low-poly still catches specular); uniform texel density across the set; ≤ 15k tris per unit, ≤ 20k buildings, ≤ 40k castle *[as-built 2026-07-21: the budgets as actually enforced by the art pipeline — the v2 "8k/15k" line went stale when the generated-mesh bakes landed]*.
- Every ability, impact, spawn, death, and spell gets a **Niagara effect**; every card play has audio.

*[as-built 2026-07-21 — how the art is actually made (design summary; the full engineering law lives in `.claude/pipeline/CONVENTIONS.md`):]*
- ***Concept-art-first:** every asset starts as a generated 2D concept (FLUX.1-dev, per-card art-direction prompts; Jonathan may review/replace any concept before production). The concept is the color/style contract: the baked 3D result must READ AS THE CONCEPT'S PALETTE (the concept-fidelity color law, 2026-07-21) — judged per asset at acceptance, not merely "brighter".*
- ***Dual 3D engines:** TRELLIS.2 (default image-to-3D) + Meshy Pro (second engine: retexturing existing meshes — the fix for the early "dark TRELLIS look" — and per-asset image-to-3D for stubborn shapes), both feeding one Blender refine step and same-path UE imports so no code reference ever breaks. Status: the full 16-card roster + castle + gold node are textured through this pipeline (2026-07-21).*
- ***Team color:** every card mesh carries exactly TWO material slots — a minority "team region" slot (trim/banners/accents) recolored blue/red at spawn, plus the asset's own PBR slot. This replaced the v2 "one master material with a team-color parameter" idea. Gold nodes are the deliberate exception: team-neutral warm emissive.*
- ***LODs & culling:** the whole gameplay fleet ships classic LOD chains (auto 4-LOD on static meshes; castles keep hand-set landmark LODs; skeletal units add LODs plus render-driven animation throttling), and every terrain-scatter layer has per-layer draw-distance cull bands and shadow flags. This structure is what pays for the 10× arena.*
- ***Nanite:** OFF for everything gameplay (standing ruling). The ONE scoped exception (2026-07-18): the far vista-ring cliffs (§5) — Nanite ON, no collision, never gameplay actors, never inside the play bounds.*
- ***Animation:** all humanoid units share ONE skeleton ("SiegeBiped", 21 bones); motion comes from Meshy auto-rig preset clips (idle/walk/attack/death) retargeted through a Blender-side lane (a UE 5.8 retarget-export defect makes the in-editor export route unusable — detail in CONVENTIONS), with a hard height-normalized foot-amplitude gate so root-only "sliding" clips can never ship. Per-unit status (2026-07-21): Footman, Knight, Pikeman, Cleric, Longbowman, Miner, Militia Mob, Sapper LIVE on full-body clips; Archer + Ogre in flight; **Cavalry HELD** — biped clips deform the horse, needs a quadruped source (known open item). Buildings do not animate (tower recoil stays procedural); the hero remains the template skeletal character.*
- ***Known open items:** the Lightning sky-strike material rework (first attempt used a custom-HLSL node that wedged the editor; re-author with stock material nodes), the Cavalry quadruped source, and the 7 uncovered audio cues below.*

**Scene standards:**
- Lighting: one strong key light + Lumen GI; warm-vs-cool team framing; stylized gradient skybox.
- Post stack: bloom, subtle vignette, color-grade LUT (saturated characters over slightly desaturated environment — Ori rule).
- Palette: player = cool blue accents, enemy = warm red, neutral stone/green battlefield, gold glows warm yellow (emissive).

**Game-feel ("juice") checklist — required, not optional:**
- Hit flash (0.1 s white material swap) on every damage event; spawn squash-and-stretch (0.15 s); floating damage numbers; tower recoil on fire; castle crumble stages (§3.9); gold coin burst on unit kills; screen shake ≤ 0.2 s on castle hits. *[as-built 2026-07-18 (M7): the full checklist shipped in C++, soft-referenced and null-safe.]*

**Performance budget:** 60 fps at 1440p on a mid-range GPU (RTX 3060 class) with 60+ units on screen. Profile at every milestone checkpoint. *[as-built: no measured baseline exists yet — the M7 perf capstone is still open, and every perf verdict is a HUMAN watch (Jonathan), not a machine stat; the M7.6 W1 watch will be the first real number.]*

**Audio:** hero swing + hit, unit spawn, projectile fire/impact, miner "clink" loop, card play/discard clicks, spell casts, castle-hit and castle-destroyed stingers, victory/defeat music, overtime sting. *[as-built 2026-07-16: the C++ trigger hooks for all 14 event cues shipped null-safe; 7 of the 14 cues are covered from imported packs (4 solid weapon/bow cues + 3 best-effort), and 7 (mining loop, 2 UI clicks, castle-destroy stinger, victory/defeat music, overtime sting) exist in no owned pack — they play silent+logged until Jonathan sources them.]*

## 7. UI / UX
- **In-match HUD:** hero health bar (+ regen state); gold counter with live **gold/s rate**; overtime indicator (from 7:00); miner count ("3/6"); **hand bar of 6 cards** (art + cost, greyed when unaffordable) + **next-card preview slot**; **discard button** (shows 1-gold cost); **hero upgrade icon row** with stack pips; both castles' HP bars (top). Enemy hero/units show floating health bars when damaged. *[as-built 2026-07-10: broadened + reversed by Jonathan's directives — EVERY combat actor (units incl. miners, all buildings, the hero, both teams) shows a floating overhead health bar, ALWAYS visible while alive: red fill = enemy, blue fill = friendly, grey track; hidden only on death. Rebuilt once from scratch on the castle bar's working model after the first implementation wouldn't render.]*
- *[as-built 2026-07-03 (M2 input model): hand slots also play via hotkeys **1–6**; holding **Left Alt** shows the cursor (camera look suspended) so cards/discard are mouse-clickable — free-look is otherwise preserved. Card faces carry real illustrations (one per card, data-driven).]*
- **Placement mode:** ghost preview + legal-half highlight; building clearance violations show red. **Targeting mode (spells):** ground reticle, full map.
- **End screen:** Victory / Defeat + "Play Again" (full reset per §3.9).
- **Deck-builder screen (Milestone 6):** browse the 28-card collection; add/remove copies with per-card **Max Copies** enforced; live count "x/50"; save/load named decks; a deck is playable only at exactly 50. *[as-built 2026-07-10: shipped in M6; browser tiles render as PHYSICAL cards (same face treatment as the in-match hand) with a prominent above-card in-deck copy count — both from Jonathan's playtest feedback. Saved decks persist cross-session and feed matches; the bot picks one of 2 curated decks.]*
- **Main menu (Milestone 3+):** Play (vs Bot), Deck Builder (M6), Quit.

## 8. Progression & Economy
- **In-match currency:** gold only — start **10**, base **+1 per 2 s**, **+1/s in overtime** (7:00), **+1/s per active miner**, **+2/s per Deep Mine**, cap **999**. Discard costs **1**. *[as-built 2026-07-08: rebalanced from start 50 / +2/s / +4/s overtime — see §3.2.]*
- **Economy design rule:** economy cards must have a **meaningful payback window** (Miner ≈ 18 s effective, Deep Mine ≈ 7.5 s but a 15-gold tempo hit and a stationary raid target). If a playtest shows "always max economy first" is dominant, raise costs — do not shorten paybacks.
- **Deck cost curve:** default deck averages ~4.3 gold/card; decks that average < 4 tend to spam, > 7 tend to brick — the deck-builder shows average cost as a guide (no hard rule).
- **Card costs and copy caps:** single source of truth is the §4 tables / `cards.csv`. Tune numbers there during playtests.
- **Meta progression:** none in the prototype. The deck-builder (M6) assembles decks from the fixed 28-card pool. Card unlocks / collection / ranked are future (§10).

## 9. Milestones
Ordered playable increments. The manager decomposes only the current milestone. Each milestone ends in a **recordable portfolio slice**.

1. **Core loop, local, one card.** Walkable arena with two castles; hero moves and melee-attacks; gold accrues and shows on the HUD; a single default Footman card can be played to spawn a unit that auto-paths to the enemy castle and attacks it; enemy castle has HP and can be destroyed → Victory screen. No opponent, no deck system yet. *(Slice: core-loop + unit-pathing clip.)* — **SHIPPED 2026-07-03** *[as-built: playtested + signed off; round-1 combat-legibility findings (swing/impact feedback, castle HP bar, unit lunge) fixed and confirmed same day.]*
2. **Economy + deck/hand + core set + defenses.** Full deck system (50-card default deck from `cards.csv`, 6-card hand, next-card preview, draw/discard/reshuffle); discard; Miner economy incl. arrival rule; Archer + Knight with §3.8 profiles; Arrow Tower + Wall; placement rules; overtime income. *(Slice: card-hand UI clip + unit targeting/combat clip.)* — **SHIPPED 2026-07-04** *[as-built: plus the hotkeys 1–6 / Alt-cursor input model (§7) and arena boundary hardening.]*
3. **Bot opponent = real 1v1 match.** The §4 rule-based bot; hero Rally ability; main menu (Play vs Bot); full win/lose flow; bot decision logging. *(Slice: full match-vs-AI clip + AI dev-pipeline case study.)* — **SHIPPED 2026-07-04** *[as-built: plus a Sandbox (No Bot) test mode added 2026-07-05 (dev affordance, not GDD content).]*
4. **Card Set II.** 16 new cards: keywords (Siege/Charge/Slayer/Swarm), Support profile, hero upgrade system + HUD row, Barracks spawner, Deep Mine, Masons, two new towers; bot plays Set II. *(Slice: expanded-roster combat clip — Ogre push vs Bomb Tower defense.)* — **SHIPPED 2026-07-08** *[as-built: signed off with a standing "balancing changes later" note — the balance pass is open backlog awaiting Jonathan's specific notes.]*
5. **Spell system + Set III.** Targeting mode, 5 spells + Crystal Tower, spell Niagara VFX at the §6 bar; bot casts spells per its M5 rules. *(Slice: spell VFX showcase reel.)* — **SHIPPED 2026-07-08** *[as-built: delivery later overhauled 2026-07-21 — hero-origin lines + Lightning 700 (§3.11).]*
6. **Deck-builder meta.** Deck-builder screen per §7 (copy caps, x/50 counter, save/load named decks); decks feed into matches; replace default deck with a legal curated one; give the bot 2 distinct decks. *(Slice: UI/UX + save-load systems clip.)* — **SHIPPED 2026-07-09** *[as-built: plus physical-card tiles + above-card copy counts from playtest feedback.]*
7. **Premium art & feel pass.** Bring the whole game to the §6 bar: materials/lighting/post stack, full juice checklist, castle crumble, arena set dressing, Lumen showcase lighting; Sequencer cinematic flythrough + gameplay b-roll. *(Slice: environment/archviz flythrough + game-feel before/after reel.)* — **SHIPPED 2026-07-18 (art-complete)** *[as-built: juice checklist, skeletal-animation workstream, 16/16 textured roster, Lumen/post/skybox, audio hooks all live. STILL OPEN from the M7 checkpoint: the Sequencer flythrough slice + the 60 fps perf watch — deliberately deferred so the flythrough captures the final M7.5/M7.6 art.]*
8. **Networked 1v1 multiplayer.** Server-authoritative gold, spawns, combat, spells, upgrades; lobby/host-join; replicate units/heroes/economy for a real online 1v1. *(Slice: multiplayer replication systems clip.)* — **future (unchanged, not started)**

*[as-built — milestones INSERTED between the v2 numbers during development (each shipped one is preserved on an `mN-testable` branch):*
- ***M4.5** gameplay-terrain pass (2026-07-08) — conceived as hand-placed mirror-symmetric terrain, parked on asset availability, then SUPERSEDED by M6.5's procedural approach; its climbable-terrain intent shipped as M6.6.*
- ***M5.5** overhead health bars (2026-07-09) — SHIPPED (see §7).*
- ***M6.5** battlefield & procedural terrain (2026-07-10) — SHIPPED: 4× castle spacing + the runtime random scatter (§5).*
- ***M6.6** climbable terrain (2026-07-14) — SHIPPED: convex climbable hills, units climb too, terrain blocks projectiles, arena widened.*
- ***M7.5** art-quality upgrade (2026-07-18) — SHIPPED in substance: Meshy Pro second engine, fleet retexture to the concept-fidelity bar (16/16), the animation fleet (§6); FAB pack purchases remain Jonathan-gated.*
- ***M7.6** arena 10× + LOD/perf structure (2026-07-18) — IN PROGRESS: Phase 0 (the 10× scale spike) is integrated on its own branch; the hard-gated phase ladder (scatter culls → density → LOD batch → vista/POIs → capstone playtest) waits on Jonathan's W1 perf/feel watch, and nothing merges to main before the final gate.]*

## 10. Out of Scope
Not yet — do not build until scheduled or explicitly requested:
- **Networking / replication until Milestone 8** (build and tune everything locally first).
- **Direct unit command / RTS control** (selecting and ordering summoned units) — post-GDD future feature; units are fully AI-driven.
- More heroes / class selection, more than one arena, spectator mode, replays.
- Card unlocks, collection/gacha, ranked/matchmaking, player accounts, monetization.
- Mobile/controller support, save systems beyond deck save/load, cosmetics.
- *[as-built 2026-07-21: recorded FUTURE HOOKS — flagged during development, still explicitly out of scope until Jonathan schedules them: neutral gold-node capture mechanic (§5 props are visual-only), adaptive bot spawn positioning by strategy, a spawn-forward/waypoint mechanic (the march-pacing lever), an RTS overview camera.]*

> One rule that matters most: **numbers over adjectives.** Every value above is implementable and testable, and lives in `cards.csv` where applicable. Tune numbers in playtests; don't replace them with vibes.

## Design Change Log (as-built appendix, added 2026-07-21)

Dated record of the major design pivots since v2. One entry per pivot, rationale in one line; details live in the tagged sections above and in the pipeline records.

- **2026-07-03 — Combat legibility pulled into M1 (v2.1).** First playtest: the core loop worked but damage was unreadable — swing/impact feedback, castle HP bars, and attack telegraphs became M1 baseline.
- **2026-07-08 — Economy rebalance.** Start 10 gold, base +1 per 2 s (was 50 / +2 per s): passive income was too high and made economy cards uninteresting.
- **2026-07-07 → ongoing — Art pipeline, generation 2: blockouts → TRELLIS.2.** Automated concept→3D→UE pipeline stood up and pilot-proven (Footman/Archer/Castle), replacing hand-modeled blockouts as the production path.
- **2026-07-08 — M4.5 terrain pass conceived, then parked.** Hand-placed mirror-symmetric hills/trees; parked on asset availability — its intent survived (see 07-10 and 07-14).
- **2026-07-09 — Overhead health bars on everything (M5.5).** Jonathan directive; later reversed to always-visible after playtest.
- **2026-07-09/10 — Health-bar rebuild saga.** Five failed fixes on the first implementation; feature rebuilt from scratch on the castle bar's proven model with the red-enemy/blue-friendly law. Outcome: bars work; lesson institutionalized (rebuild fresh instead of patching corrupted UI).
- **2026-07-09 — Deck-builder shipped (M6)** — physical-card tiles and above-card copy counts added straight from playtest notes.
- **2026-07-10 — Battlefield pivot (M6.5).** Arena 4× wider + RUNTIME procedural asymmetric scatter re-seeded each match — replaced the hand-placed mirror-symmetric plan because Jonathan wanted organic variety, and PvE fairness tolerates asymmetry.
- **2026-07-14 — Climbable terrain (M6.6).** Hills became real gameplay high ground: purpose-built convex meshes the hero AND units climb; terrain blocks projectiles. Delivered the parked M4.5 intent on top of the M6.5 scatter.
- **2026-07-15/16 — M7 premium pass + concept generation.** Skeletal rig/animation workstream stood up from scratch; concepts auto-generated (FLUX) instead of hand-dropped — the concept-art-first pipeline became the standard.
- **2026-07-18 — Art pipeline, generation 3: + Meshy second engine (M7.5).** Meshy Pro added for retexture (killing the accepted-dark TRELLIS look), image-to-3D alternatives, and auto-rig/preset animation clips.
- **2026-07-18 — Arena 10× + LOD/perf structure (M7.6).** True 10× area with speeds/ranges deliberately unscaled — the slow-epic pacing ruling; LOD chains + cull bands + the scoped Nanite vista exception make it affordable.
- **2026-07-19 — Animation lane pivot.** UE 5.8's retarget export proved defective (root-only motion); the Blender-side retarget became the sanctioned path and the unit fleet went live on full-body clips.
- **2026-07-21 — Spell delivery overhaul + fleet retexture GO.** Fireball/Frost Nova became hero-origin cursor-aimed line spells and Lightning a bigger (700) taller sky strike — spells now read as HERO actions; the whole roster was retextured under the new concept-fidelity color law (16/16).
