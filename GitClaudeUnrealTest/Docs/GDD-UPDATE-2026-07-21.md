# GDD As-Built Update — Review Sheet (2026-07-21)

For Jonathan. This is the complete list of amendments made to `Docs/GDD.md` in the as-built update pass you directed ("read through all the tasks we have completed since the creation of GDD and add any details worth mentioning"). Your review is the acceptance gate; the commit rides the next build-master window (TASK-244) only after your approval.

**Rules the pass followed:**
- Design-level only — engineering law (naming, tool contracts, import mechanics) stays in `.claude/pipeline/CONVENTIONS.md` and is referenced in one line where relevant, never inlined.
- One consistent marker style: every change is tagged `*[as-built YYYY-MM-DD: …]*` (tables use a short `*(rev. …)*` where a tag would break the cell).
- Nothing aspirational was deleted. Where reality contradicts an unbuilt aspiration, the original text stands and the tag records the delta.
- Your section numbering is untouched (other docs and code comments reference §-numbers).

---

## Amendments by section

### Header
- **Added a v3 revision comment** (next to your v2/v2.1 notes) explaining the pass, the tag style, and the no-deletion rule.
- Why: the header is where the GDD's revision history already lives.

### §1 Overview
- **Pitch line tagged:** the arena is castle-symmetric but the battlefield terrain is a procedural random scatter regenerated each match (points to §5).
- Why: "symmetric 3D arena" is no longer literally true — you ruled asymmetric organic terrain for PvE at M6.5.

### §3.1 Hero Movement & Combat
- **Added one as-built bullet:** the M6.6 terrain-climb comfort retune (step height 50, walkable slope 50°, jump velocity 600). Base speeds unchanged.
- Why: shipped hero-movement values now differ from engine defaults; speeds/damage in the section are still accurate.

### §3.2 Gold Economy — IN-PLACE NUMBER CORRECTIONS
- **Start 50 → 10; base +2/s → +1 per 2 s; overtime +4/s → +1/s.** Acceptance criteria updated to match. Tagged with your 2026-07-08 urgent rebalance directive; notes the HUD shows the rounded-up +1/s average and that miner/Deep Mine income was untouched.
- Why: these were the most factually-wrong numbers in the document — the code has carried the new defaults since M4-era.

### §3.3 Miners
- **Tag on the "~10 s walk" line:** gold nodes moved WITH the castles at every arena scale-up, so the short walk assumes placement near your castle; mid-half is now a long hike.
- Why: the walk-time claim silently depended on the 4,000-unit arena.

### §3.4 Deck & Hand
- **Note under the default-deck table:** superseded as the default by the curated 50 in `cards.csv` (full DeckCount spread listed: Footman 12, Archer 8, Wall 4, Knight 3, Miner 3, Arrow Tower 3, Militia Mob 3, Pikeman 3, Cavalry 3, Longbowman 2, Cleric 2, Ogre 2, Fireball 2). Original table kept for history.
- Why: M6 shipped the deck-builder and re-authored the default; the v2 6-card-type deck no longer exists in the data.

### §3.5 Playing Cards
- **Tag on the Spell bullet:** reticle-at-point is still true for Lightning/Battle Cry; Fireball + Frost Nova use the reticle as an AIM point (§3.11); Pickpocket resolves instantly (M5 ruling).
- Why: three of five spells no longer follow the literal v2 targeting sentence.

### §3.8 Unit AI
- **Tag on the attack-telegraph line:** the skeletal pass shipped — real full-body animations; the procedural lunge is now only the null-safe fallback.
- Why: the "blockout tier until M7" caveat is done.

### §3.9 Castle & Win Condition
- **Tag on the crumble line:** shipped as progressive material-char stages + debris VFX (material-based, not geometry fracture); team-neutral while crumbling.
- Why: records the implementation choice at design level.

### §3.11 Spells — NEW AS-BUILT BLOCK (the spell delivery overhaul)
- **Fireball + Frost Nova = hero-origin cursor-aimed LINE spells** (your 2026-07-21 playtest directive; you confirmed cursor aim). Recorded tunables: line range **900**, half-width **100**, sweep front ~3,000 u/s — all flagged playtest numbers. Magnitudes/costs/castle-50%/no-friendly-fire unchanged; line-vs-circle coverage is a playtest watch.
- **Bot origin = its castle** (it has no hero) — flagged design default, with the far-cluster whiff watch item.
- **Lightning: radius 400 → 700** (data change), taller/higher sky-strike read, visual + hitbox scale honestly from the same data.
- Why: the single biggest post-v2 mechanics change; the GDD said nothing about it.

### §4 Card Compendium
- **Lightning row: 400 → 700** with a `*(rev. 2026-07-21)*` marker (matches `cards.csv`).
- **Note under the Set III table:** Fireball/Frost Nova keep magnitudes but deliver along the hero-origin line.
- **Bot section tag:** bot line spells fire from its castle; bot attack waves also MARCH from its castle on the 10× field (M7.6 ruling #1 — no mid-field materialization).
- Why: table cells and bot rules must match the shipped data/behavior.

### §5 Levels / World — LARGEST AS-BUILT BLOCK
- **First bullet corrected in place:** ~4,000 apart → 50,000 (with pointer to the block).
- **New 5-bullet as-built block:**
  1. **Scale:** castles ±25,000 (50,000 apart; field ≈ 52k × 25k) — true 10× area; speeds/ranges deliberately NOT scaled (your slow-epic pacing ruling, accepted "for now", W1 watch pending and owns the live verdict); forward structures gain importance as the counter-lever; 10× currently on its own branch, mainline stays ±8,000 until the merge gate.
  2. **Gold nodes:** still exactly 800 in front of each castle through every scale-up.
  3. **Terrain:** runtime procedural scatter (trees/rocks/climbable hills/grass), asymmetric organic random per match (mirror-symmetric toggle reserved), obstacles block + carve navmesh, hills climbable by hero AND units, high ground physical-only, terrain blocks projectiles.
  4. **Corridor & keep-clears:** guaranteed castle-to-castle corridor (half-width 1,000) + keep-clear radii — traversability enforced every match.
  5. **Vista & POIs (M7.6 plan):** far cliff vista ring outside play bounds (the one Nanite exception), 6–10 POIs, 2 pre-seeded NEUTRAL gold-node props (visual-only; capture mechanic = recorded future hook).
- Why: §5 was the most out-of-date section in the document — three arena generations happened since v2.

### §6 Art Style & Audio
- **Tri budgets corrected IN PLACE:** ≤8k units / ≤15k buildings+castle → **≤15k units / ≤20k buildings / ≤40k castle** (as actually enforced by the pipeline manifest).
- **New as-built block "how the art is actually made"** (design summary; one-line pointer to CONVENTIONS for the law):
  - Concept-art-first: FLUX.1-dev generated concepts, your optional review/replace; **concept-fidelity color law** (baked result must read as the concept's palette — your 2026-07-21 bar).
  - Dual 3D engines: TRELLIS.2 default + Meshy Pro retexture/image-to-3D (the dark-look fix); 16/16 roster + castle + gold node textured.
  - Two-slot team-color system (team-region slot recolored at spawn + per-asset PBR slot) — replaces the v2 "one master material with a team-color parameter"; gold node = team-neutral emissive exception.
  - LOD chains + per-layer scatter cull bands/shadow flags — what pays for the 10× arena.
  - Nanite posture: OFF for gameplay; the scoped vista-ring exception (2026-07-18 ruling).
  - Animation system: shared 21-bone SiegeBiped skeleton; Meshy preset clips retargeted via the Blender lane (UE 5.8 export defect noted in one line); height-normalized amplitude gate. **Per-unit status:** 8 units live, Archer + Ogre in flight, **Cavalry HELD (quadruped needed)**; buildings don't animate; hero = template character.
  - Known open items: **Lightning strike material rework** (custom-HLSL node wedged the editor — re-author with stock nodes), Cavalry quadruped, 7 uncovered audio cues.
- **Juice checklist tagged shipped** (M7, C++, null-safe).
- **Perf budget tagged:** no measured baseline yet; perf verdicts are human watches; W1 = first real number.
- **Audio tagged:** hooks for all 14 cues shipped null-safe; 7 covered from packs (4 solid + 3 best-effort), 7 missing and silent+logged until you source them.
- Why: §6 described a pipeline that no longer exists; this is the section your directive specifically called out.

### §7 UI / UX
- **Health-bar line tagged (the rebuild saga outcome):** every combat actor, both teams, ALWAYS-visible overhead bar — red enemy / blue friendly / grey track, hidden only on death; rebuilt once from scratch on the castle bar's working model.
- **New input-model bullet:** hotkeys 1–6 + Left-Alt cursor (M2 law); card faces carry real data-driven illustrations.
- **Deck-builder line tagged:** shipped M6; physical-card tiles + above-card copy count; cross-session saved decks; bot picks 1 of 2 decks.
- Why: three shipped UX realities the v2 text contradicted or omitted.

### §8 Progression & Economy
- **Currency line corrected in place** to the rebalanced numbers (mirrors §3.2).
- Why: same stale numbers as §3.2.

### §9 Milestones — STATUS PASS
- **M1–M6 marked SHIPPED** with dates + a one-line what-shipped delta each (M1 legibility fixes; M2 input model; M3 sandbox mode; M4 balance-note backlog; M5 later delivery overhaul; M6 physical tiles).
- **M7 marked SHIPPED (art-complete 2026-07-18)** with the two checkpoint items explicitly OPEN: Sequencer flythrough + 60 fps perf watch (deferred so the flythrough captures final art).
- **M8 marked future, unchanged.**
- **New inserted-milestones note:** M4.5 (superseded), M5.5 (shipped), M6.5 (shipped), M6.6 (shipped), M7.5 (shipped in substance; FAB purchases still your gate), **M7.6 IN PROGRESS** (Phase 0 on branch; W1 gate + phase ladder pending; merge only at the final gate). Notes each shipped milestone has an `mN-testable` branch.
- Why: your directive — mark M1–M7.5 shipped, M7.6 in progress, M8 future.

### §10 Out of Scope
- **One added bullet:** the four recorded future hooks (neutral-node capture, adaptive bot spawns, spawn-forward/waypoint, RTS overview camera) — still explicitly out of scope until you schedule them.
- Why: they were flagged during M7.6 rulings and belong on the out-of-scope ledger, not lost in the task board.

### NEW — "## Design Change Log (as-built appendix)"
- 14 dated entries, 1–2 lines each, covering: M1 legibility (v2.1), economy rebalance, TRELLIS.2 pipeline stand-up, M4.5 conceive/park, M5.5 health bars, the health-bar rebuild saga, M6 deck-builder, M6.5 battlefield pivot, M6.6 climbable terrain, M7 premium pass + concept generation, M7.5 Meshy second engine, M7.6 arena 10×, the animation-lane pivot (2026-07-19), and the 2026-07-21 spell overhaul + fleet retexture GO.
- Why: your "major changes to the core structure" deserve a single chronological record inside the design document itself.

---

## What was deliberately NOT changed
- Section numbering and ordering (referenced by §-number elsewhere).
- All still-accurate mechanics: hero combat numbers, keywords, targeting profiles, castle 2000 HP + damage model, discard, placement rules, hero upgrades, Crystal Tower, bot rule order, deck legality rules.
- All aspirational/unbuilt content: §6 audio wish-list, §7 specifics not yet built, M8, §10 — kept verbatim, tagged only where reality diverges.
- The closing "numbers over adjectives" rule — untouched.

## Open questions surfaced by this pass (no action taken; your calls)
1. §3.4's original default-deck table is now historical — keep it (current choice) or replace it outright with the curated table?
2. Frost Nova is DeckCount 0 in the curated default — the new ice line-spell VFX is never seen in normal play (standing D-FROSTNOVA-DECK flag).
3. The v2 line "deals 50 if the blast overlaps the castle" (§3.11 acceptance) still describes circle behavior for Fireball — left as-is since the 50%-vs-castle law is unchanged, but the example could be rewritten for line delivery if you prefer.
