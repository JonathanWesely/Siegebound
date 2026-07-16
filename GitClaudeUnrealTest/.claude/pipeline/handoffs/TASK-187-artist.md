# TASK-187 handoff — Fire/Ice spell VFX re-skin (art-director)

Status: **ready-for-integration**. Editor MCP up (127.0.0.1:8000), L_Arena loaded. No Git touched.

## What shipped (in-place re-skin at the CardID-composed code-contract paths)

| Code-contract path (object path) | Now contains | Source system (donor) | Method |
|---|---|---|---|
| `/Game/VFX/NS_Spell_Fireball.NS_Spell_Fireball` | Fire_Magic explosion burst (8 emitters: NSSpellFireball/Crack/Flash/Smoke1-3/Sparks1-2) | `/Game/Fire_Magic/VFX_Niagara/NS_Fire_Magic_Explosion` | duplicate → save-as in place |
| `/Game/VFX/NS_Spell_FrostNova.NS_Spell_FrostNova` | Ice_Magic radial frost shockwave (spiral crystals + mist + sparks + ground crack) | `/Game/Ice_Magic/VFX_Niagara/NS_Ice_Magic_Shockwave` | duplicate → save-as in place |

- Both are valid `NiagaraSystem` assets (verified via `get_asset_class`), and their internal object path matches the composed load path in `USpellLibrary::SpawnSpellVFX` (`SpellLibrary.cpp:77-80`, `LoadSynchronous` on `/Game/VFX/NS_Spell_<CardID>.NS_Spell_<CardID>`). **No rename, no new data column, no C++/cards.csv change** — the swap is a pure look-swap at the exact placeholder paths, so the M5 spell-resolve behavior is untouched.
- Placeholders had **zero referencers** (composed-path load only), so the in-place overwrite is safe.

### Method detail (why it looks the way it does on disk)
Duplicated each source to a temp `_NEW` path, verified class + dependencies, deleted the placeholder, then `move`d `_NEW` → the exact target name (so the object name = package name = the composed contract path). The editor `delete` left the old Jul-8 placeholder `.uasset` files orphaned on disk (registry-deleted but not flushed); I removed those two orphan files so the rename could land. Final disk state: `NS_Spell_Fireball.uasset` (2.6 MB) + `NS_Spell_FrostNova.uasset` (1.5 MB), no `_NEW`/orphan leftovers. Assets saved.

## Source choice rationale
- **Fireball → Explosion** (not `_AOE`): Fireball is an instant 100-dmg AoE **burst at the reticle**; the multi-emitter explosion reads as an impactful "boom" in one frame (§6). `_AOE` is a lingering ground patch — wrong read for an instant blast.
- **Frost Nova → Shockwave** (not `_Frozen`/`_Snowstorm`): a "nova" is literally a radial burst; `NS_Ice_Magic_Shockwave` is a ground-radial ice spiral/crystal ring = the exact Frost-Nova shape, and it is **finite/one-shot** (important — see note below). `_Frozen` reads as a localized ground-freeze; `_Snowstorm` is a voluminous **looping** weather effect (would persist forever under the one-shot `SpawnSystemAtLocation` call — rejected).

## Radius / sizing
- Both effects are authored at low-meters ability scale and read as ground bursts of roughly **~200–300 unit radius** against a 1 m measurement grid in Simulate-In-Editor — a good match for Fireball 300 / Frost Nova 350 AoE. The Frost Nova frost ring sits slightly under the 350 target but reads correctly as the AoE.
- **Sizing caveat (flagged):** the C++ spawns via `UNiagaraFunctionLibrary::SpawnSystemAtLocation` with **no scale arg**, so size is whatever is baked into the system. These premium systems expose **no scale user-parameter** and use dynamic bounds (`bFixedBounds=false`), and the editor MCP has **no Niagara emitter/module editor** — so I could not dial the radius to an exact 300/350 without the interactive Niagara editor. The authored scale is close enough to read as the right-sized AoE; if Jonathan wants exact radius calibration, that's a human Niagara-editor tweak (scale the sprite/mesh-size or spawn-shape modules), not an MCP-doable step. Not blocking.

## Verification (screenshots in gitignored `Tools/ArtPipeline/Cache/TASK-187/`)
One-shot Niagara bursts can't be reliably caught in the non-realtime level viewport; I used the Niagara asset-editor preview (loops) + Simulate-In-Editor (realtime) to confirm play:
- `editor_fire.png` — Fire explosion in the Niagara editor preview: warm-orange multi-emitter fireball burst (§6 warm read confirmed).
- `final_fireball_editor.png` — the FINAL `/Game/VFX/NS_Spell_Fireball` opened: all 8 fire emitters present (proves the shipping asset IS the explosion).
- `sie_fireball_1.png` — Fireball spawned in-world (SIE) plays (caught the smoke/ember tail; the bright flash is ~0.3 s and overshoots MCP round-trip latency).
- `sie_frostnova_1.png` — **best in-world frame:** `/Game/VFX/NS_Spell_FrostNova` spawned in L_Arena, playing a **cyan-white crystalline frost radial burst** at origin over the 1 m grid (§6 cool cyan-white read + AoE sizing confirmed).
- Could not drive a real PIE spell-cast (targeting-mode needs input, input-limited headless); verified by MCP spawn instead, per the task's fallback.

## INTEGRATION NOTES for build-master (important)
1. **Commit the two changed assets:** `Content/VFX/NS_Spell_Fireball.uasset` + `Content/VFX/NS_Spell_FrostNova.uasset` (both show as modified in the working tree).
2. **DEPENDENCY GATE — must also commit the source packs.** The re-skinned systems **hard-reference** pack assets that are currently **UNTRACKED** in Git:
   - `NS_Spell_Fireball` → `/Game/Fire_Magic/Materials/{Smoke/M_Fire_Magic_Smoke, Fragment/M_Fire_Magic_Sparks, Crack/M_Fire_Magic_M_Crack4, Fragment/M_Fire_Magic_Flash}` + `/Game/Fire_Magic/Mesh/SM_Fire_Magic_Plane1` (+ their textures).
   - `NS_Spell_FrostNova` → `/Game/Ice_Magic/Materials/{M_Ice_Magic_Spiral, Smoke/M_Ice_Magic_Smoke1, Fragment/M_Ice_Magic_Sparks, Crack/M_Ice_Magic_Crack}` + `/Game/Ice_Magic/Mesh/{SM_Ice_Magic_Spiral3, SM_Ice_Magic_Plane3}` (+ their textures).
   - `Content/Fire_Magic/` and `Content/Ice_Magic/` are `??` untracked. **If only the two NS_Spell assets are committed, the effects will have unresolved references on a clean clone.** Commit the pack dependencies (or the whole packs) with this change. These are READ-ONLY Fab-style donor packs (template-donor rule) — not edited.
3. Pivot/scale: both systems spawn at their origin = the reticle/`TargetPoint`; no offset. Default component scale (1,1,1) — do not add scale.
4. Do **not** save `L_Arena` on my account — I spawned/removed transient preview NiagaraActors and cleaned them all up (level verified clean: 0 Niagara actors, 0 `PREVIEW_*` actors), but the level may read dirty from the transaction history; its content is unchanged by this task.

## Flags
- **D-FROSTNOVA-DECK (carried from board):** FrostNova is `DeckCount 0` (not in the default deck), so the new ice effect won't be seen in normal play until Jonathan bumps its DeckCount — that's the separate data task the orchestrator is routing, not this one.
- Sizing-calibration caveat above (exact 300/350 radius = human Niagara-editor tweak if desired; current authored size reads correct).
