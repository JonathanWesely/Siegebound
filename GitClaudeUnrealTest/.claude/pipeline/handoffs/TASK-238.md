# TASK-238 handoff — NS_Spell_Fireball + NS_Spell_FrostNova as hero-fired projectile/line effects (art-director)

Status: **ready-for-integration** · 2026-07-21 · Editor MCP + python remote-exec lanes, L_MainMenu loaded (see editor-state notes). No Git touched.

## What shipped (in-place re-author at the CardID-composed code-contract paths)

| Code-contract path | Now contains | Donor system (READ-ONLY pack) | Method |
|---|---|---|---|
| `/Game/VFX/NS_Spell_Fireball.NS_Spell_Fireball` | Hero-fired FIRE BOLT: orange muzzle ring at the cast origin, ember-trail bolt traveling along +X, terminal starburst + lingering embers (7 emitters: 4 sprite, 2 mesh, 1 ribbon; CPU, one-shot, life ≈ 2.5 s) | `/Game/Fire_Magic/VFX_Niagara/NS_Fire_Magic_Projectile3` | duplicate → delete placeholder → rename into place (TASK-187 method; zero referencers verified pre-delete) |
| `/Game/VFX/NS_Spell_FrostNova.NS_Spell_FrostNova` | Hero-fired FROST CONE: ice-crystal spikes erupt instantly forward covering the line + frost mist + sparks residue (4 emitters: 2 mesh, 2 sprite; CPU, one-shot, life ≈ 2 s) | `/Game/Ice_Magic/VFX_Niagara/NS_Ice_Magic_FrontSpike` | same swap method |

- Both verified `NiagaraSystem` class; object name == package name == the composed `LoadSynchronous` path in `USpellLibrary::SpawnSpellVFX`. **No rename, no data column, no C++ change.**
- Both systems are authored by the pack to fire along **+X local** — the TASK-236 muzzle spawn (`SpawnSystemAtLocation(muzzle, aimRotation)`) orients them down the aim line with no adjustment needed. Verified in PIE (below).
- No orphan .uasset files this time (disk listing verified before/after the swap).

## Donor selection — measured, not guessed

All 15 Fire_Magic + 16 Ice_Magic candidate systems were simulated deterministically (solo-mode `advance_simulation` harness, 60 Hz, bounds sampled at 8 checkpoints) and frame-captured. Tables + contact sheets in `Tools/ArtPipeline/Cache/TASK-238/`. Key numbers:

- **Fireball ← Projectile3**: bolt front edge ≈ 189 uu @0.2 s → 450 @0.35 → 819 @0.6 → 1464 @1.0; terminal starburst ≈ t1.2–1.5 s at ~1300–1500 uu; dead by 4 s. Chosen over Projectile2 (faster, ~2100 uu/s, but reads as a sparse ember trickle — much weaker look) and Flamethrower (stream look, ~4 s duration, wrong shape for a bolt).
- **FrostNova ← FrontSpike**: instant forward cone, front ≈ 780 uu within 0.2 s (matches the 0.3 s sweep timing almost exactly), extents ~550×470; dead by ~2.5 s. Chosen over IceSpike (traveling spike wave — great motion but ~2.5 s crawl, too slow vs the sweep) and SpearSplash (huge 2100-uu shatter — oversized/washy).

## Timing honesty (FLAGGED for TASK-240 + playtest)

- Sweep contract: 900 uu / 0.3 s = 3000 uu/s (`ASpellLineSweep` defaults).
- **FrostNova visual ≈ in sync** (instant cone to ~780 uu inside 0.3 s; short of 900 by ~13% — reads correct).
- **Fireball visual lags the sweep ~2×**: bolt front ~1500–1700 uu/s, reaches 900 uu at ~0.6 s (sweep finishes at 0.3 s), and the terminal starburst lands at ~1300–1500 uu — i.e. ~400–600 uu PAST the 900 line end. Damage is never outside the visual; the visual overshoots/lags the damage. No module-value access exists in the automation lanes (see Limitations), so this is baked donor behavior. If Jonathan wants exact sync, the clean lever is code-side: raise `TravelDuration` toward ~0.55 s or `LineRange` toward ~1300 (gameplay-programmer lane, playtest numbers anyway).

## Verification

- Editor-world deterministic frame series (solo advance + SceneCapture, side + gameplay-angle): `SHEET_AFTER_Fireball_side.png`, `SHEET_AFTER_Fireball_game.png`, `SHEET_AFTER_FrostNova_side.png`, `SHEET_AFTER_FrostNova_game.png`.
- **PIE, real L_Arena match** (PIE from L_MainMenu → in-PIE travel to L_Arena; time-dilated 0.15 for frame capture): both systems spawned at the LIVE hero's capsule center with a horizontal aim rotation via the exact `NiagaraFunctionLibrary::SpawnSystemAtLocation` call signature the resolver uses. Fireball ring+bolt forms at the hero and streams toward the enemy side; FrostNova spike cone erupts forward. Reads clearly at a gameplay-style camera over arena grass: `SHEET_PIE_Fireball.png`, `SHEET_PIE_FrostNova.png` (frames `PIE_*_f0*.png`).
- **End-to-end resolver cast NOT exercised by me**: `ResolveSpell` is deliberately not scriptable (C++-only pin, qa/TASK-098) and the bot did not draw into a rule-3a Fireball during my observation window (rules 1/2/4 fired normally — log verified). The full resolver-path PIE spell suite is TASK-240's item as specced.
- Perf sanity: both donors are CPU sims, 4–7 emitters, one-shot, dynamic bounds; no editor log Niagara errors/warnings during any run; no visible hitch at PIE cast.

## Captures for Jonathan (durable)

`Tools/ArtPipeline/Cache/TASK-238/` — BEFORE sheets (`SHEET_BEFORE_Fireball/FrostNova` — the old stationary ground bursts), candidate sheets (`SHEET_CAND_*`), AFTER sheets (side + gameplay angle), and PIE sheets. Full-res frames alongside (`*_t*.png`, `PIE_*_f*.png`).

## Integration notes (build-master)

1. **Commit**: `Content/VFX/NS_Spell_Fireball.uasset` + `Content/VFX/NS_Spell_FrostNova.uasset` (modified, saved).
2. **Dependency gate**: the new systems hard-reference `Content/Fire_Magic/` + `Content/Ice_Magic/` pack assets (Projectile3/FrontSpike materials, meshes, textures, curl-noise vector field). Both packs are already tracked in Git since TASK-187/M7 — verify `git status` shows no untracked pack files before committing.
3. Spawn transform: system origin = muzzle, +X = aim. Scale (1,1,1). No user parameters. Dynamic bounds (CPU-exact).
4. D-FROSTNOVA-DECK still stands: FrostNova is DeckCount 0 — not seen in normal play until Jonathan bumps it.
5. Do NOT save `L_MainMenu` (see TASK-239 handoff editor-state section — shared cleanup ledger).

## Limitations hit (recorded)

- The editor lanes expose NO Niagara module/value editing (no python API for emitter graphs; MCP property reflection hides `EmitterHandles`; rapid-iteration stores unreachable). Donor-system choice IS the value dial — hence the measured-selection approach and the timing flag above.
- Freshly duplicated Niagara systems do not finish their async recompile while the editor idles in background — simulation stays empty until the asset is opened once in the Niagara editor (forces compile). Both systems were opened, compiled ("System successfully compiled"), closed, and re-saved.
