# TASK-236 — Fireball + FrostNova → hero-origin line delivery (C++) — programmer handoff

Status: ready-for-qa · 2026-07-21 · gameplay-programmer
Law: CONVENTIONS "Spell delivery overhaul (2026-07-21)" + "Spells & Set III (M5)" (unchanged laws).
Jonathan's directive verbatim: "I want the Fireball and FrostNova spells to shoot out from the player, and I want their hitboxes to go in a line in the air from the player a short distance in front of the player, instead of a circle on the ground."

## Files touched

| File | Change |
|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/CardRow.h` | NEW `ESpellDelivery` enum (`Auto`/`GroundCircle`/`HeroLine`) + NEW FCardRow column `SpellDelivery = Auto` (data flag — see schema note) |
| `Source/GitClaudeUnrealTest/Siegebound/SpellLineSweep.h` / `.cpp` | NEW delivery actor `ASpellLineSweep` (the spec's "optional delivery actor") — fast-travel line sweep, all line tunables live here |
| `Source/GitClaudeUnrealTest/Siegebound/SpellLibrary.h` / `.cpp` | Delivery dispatch in `ResolveSpell` (pinned signature PRESERVED); new statics `GetEffectiveDelivery` + `IsLineDeliverySpell`; file-local `ResolveLineOrigin` + `ResolveHeroLine`; `SpawnSpellVFX` gained a defaulted rotation param |
| `Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.h` / `.cpp` | Aim pass in `TryConfirmSpellTarget` (line spells need only a direction); call-site flags; line-spell cast sound anchored at the hero |
| `Source/GitClaudeUnrealTest/Siegebound/SiegeBotController.cpp` | COMMENT-ONLY call-site flags at Rules 3a/3b (no logic change; LogSiegeBot lines byte-intact) |

NOT touched: `Docs/Data/cards.csv` (TASK-237 owns it this wave — git-diff confinement), Lightning/BattleCry/Pickpocket/ChainZap paths, HUD/cost flow, any Content asset.

## Design decisions (recorded)

1. **Sweep timing — fast-travel, per-segment application.** `ASpellLineSweep` advances a "front" from the origin to `LineRange` over `TravelDuration` (900 uu / 0.3 s = 3000 uu/s, 2× projectile flight speed). Every tick, enemies whose collision lies within `LineHalfWidth` of the already-swept segment receive the effect EXACTLY ONCE (weak-ptr dedupe set). Chosen over instant-sweep so damage lands in sync with the projectile visual TASK-238 attaches; near targets are hit a few frames before far ones.
2. **Tunables (UPROPERTY defaults on ASpellLineSweep, `// GDD §4` comments — mechanic rules, NOT CSV columns, per the board spec):** `LineRange = 900` (comparable to the old comfortable placement reach), `LineHalfWidth = 100`, `TravelDuration = 0.3`, `CastleMuzzleHeight = 150`. ALL FLAGGED PLAYTEST NUMBERS. `CastleMuzzleHeight` is deliberately LOW (hero-chest-comparable) — a battlement-height muzzle would fly the line OVER unit capsules given the 100 half-width.
3. **Origin resolution** (`ResolveLineOrigin`, resolver-side — the pinned `ResolveSpell` signature is untouched; TargetPoint became the AIM-POINT for the two line spells): the caster team's LIVING hero (capsule center ≈ chest height — no magic offset), else the team's standing castle + `CastleMuzzleHeight` (the bot — the law's flagged design default). Neither ⇒ refuse (caller refunds). Live TActorIterator lookups both sides.
4. **Aim direction:** origin → TargetPoint, FLATTENED horizontal (`GetSafeNormal2D`) — the line lives at the origin's height. Degenerate 2D aim refuses (resolver); the PLAYER path pre-checks it FREE + stay-in-mode (see 6) so the resolver refusal stays position-independent per contract.
5. **Bot mapping (NO bot logic change this wave):** Rule 3a passes its cluster centroid as the aim-point; the line fires from the Red castle toward it. A cluster beyond ~LineRange of the castle WHIFFS (spent, zero hits — the whiffed-Fireball rule). This is the law's flagged design default; recorded for the TASK-240 WATCH list ("bot-origin feel"). The bot never casts FrostNova (DeckCount 0 + no rule selects it) — Fireball is the only bot line spell in practice. Rule 3b (Lightning) is byte-untouched.
6. **Targeting UX (aim pass):** reticle kept as-is (sized to AoERadius — now purely an AIM indicator for the two line spells; TASK-238/239 may restyle). Confirm for line spells no longer REQUIRES a surface under the cursor: a sky-click synthesizes the aim-point from the deprojected cursor ray (flattened, hero-anchored, arbitrary 1000 uu reach — the resolver only reads the direction). Free stay-in-mode refusals generalized to "No aim direction" (failed deproject / vertical ray / reticle at zero horizontal offset from the hero). GroundCircle spells keep the M5 confirm gate byte-for-byte.
7. **Effect semantics preserved (magnitudes/costs unchanged):** Fireball line = row Damage per target, `USiegeDamageType_Spell`, enemy-only via the team filter (the ApplyRadialDamage friendly-fire design), castle INCLUDED and scaling itself to 50% (§3.11), liveness gates mirror Lightning's. FrostNova line = `ApplyFreeze(EffectDuration)` on enemy units + buildings only — castle/hero excluded (M5 ruling 5); refresh-not-stack + match-end-freeze precedence stay inside ApplyFreeze (TASK-099). NO line-of-sight blocking — the line passes through walls, consistent with the old circle (which ignored LOS) and the "acquisition stays range-only" posture.
8. **No-hit/refund rule (documented):** a line through empty air is a SUCCESSFUL resolve — spent, no refund (the whiffed-Fireball rule). Refunds happen only on resolver refusal: malformed row, malformed line config (non-positive LineRange/HalfWidth), no origin (no living hero AND no standing castle), degenerate aim, or sweep spawn failure.
9. **Match-end freeze + Play Again:** `FreezeWorldAtMatchEnd` destroys in-flight AProjectiles but predates this class, so the sweep SELF-GATES — every tick it checks `ASiegeGameMode::HasMatchEnded()` and destroys itself with no further application. Lifetime ≤ TravelDuration (2 s failsafe `InitialLifeSpan`), so nothing can straddle a Play Again. No new reset hooks needed.
10. **VFX contract (paths unchanged):** exactly ONE `NS_Spell_<CardID>` spawn per successful resolve, composed path law intact. Line spells spawn it at the MUZZLE with the aim rotation (`SpawnSpellVFX` rotation param; circle spells byte-identical via the ZeroRotator default). Seams for TASK-238: muzzle = the spawn at origin+rotation; travel = the sweep actor's transform (it rides the front every tick — an attached VFX moves with the bolt); impact = per-target `TakeDamage`/`ApplyFreeze` (existing receiver-side feedback). Line length/width scale: read `ASpellLineSweep` defaults (LineRange 900 / LineHalfWidth 100).
11. **Cast audio (small recorded deviation):** a line spell's `S_SpellCast` one-shot plays at the HERO (the muzzle) — the aim-point can be anywhere on the map and would be inaudible. Circle spells keep the reticle point byte-for-byte.

## Data-schema changes

- NEW enum `ESpellDelivery { Auto, GroundCircle, HeroLine }` (CardRow.h).
- NEW FCardRow column `SpellDelivery` (default `Auto`). `Auto` resolves PER-EFFECT in `USpellLibrary::GetEffectiveDelivery`: `AoEDamage`/`Freeze` → HeroLine (today exactly Fireball + FrostNova — verified: no other cards.csv row carries either effect), everything else → GroundCircle. Explicit cells are the per-card data override lever.
- **⚠ CSV header NOT appended this wave (flagged, needs manager follow-up):** cards.csv is frozen outside TASK-237's single Lightning cell (git-diff confinement at TASK-240), so the `SpellDelivery` column exists in C++ only. Every row imports/deserializes to `Auto`, which reproduces the directive with zero cell edits — the CURRENT DT_Cards asset needs NO reimport for the line behavior to go live at compile. Consequences to flag forward: (a) the CONVENTIONS "CSV header must match UPROPERTY names 1:1" registry entry + header append should ride the NEXT cards.csv wave (manager action); (b) TASK-240's DT_Cards reimport (for the Lightning cell) may log a missing-column notice for `SpellDelivery` — benign, rows keep `Auto`; build-master should not treat it as a failure.

## QA should scrutinize

- Shadow scan: new members/locals audited (`Target` param on `ApplyEffectToTarget` does NOT collide — AActor has no reflected `Target`; ASpellLineSweep does not derive AProjectile).
- Include-completeness: SpellLineSweep.cpp includes SiegeGameMode.h (GetAuthGameMode<> upcast), EngineTypes.h (ECC_Pawn), all four target-type headers, DamageTypes.h; SpellLibrary.cpp adds EngineUtils.h (TActorIterator) + SpellLineSweep.h (GetDefault<> complete type).
- `DistanceToTargetCollision` is a THIRD mirror of the closest-point helper (SpellLibrary.cpp + AProjectile precedents) — the qa/TASK-026 NIT-4 mirror debt, knowingly extended; folds into the shared helper on the wave that owns all mirrors.
- M5 law deltas (flag, per spec): targeting-confirm surface gate RELAXED for the two line spells only; TargetPoint semantics shift at the pinned resolver (all 4 call sites carry flag comments); ruling-11 VFX spawn point moved to the muzzle for line spells (path/count unchanged).
- Compile rides TASK-240 on the mixed tree (checkout carries qa-passed m7.6-arena10x branch code — expected, flagged).

## Playtest WATCH (for TASK-240's list)

- Line reach 900 vs the old anywhere-on-map reticle — a large hands-on nerf to spell reach; Jonathan tunes `LineRange`.
- Bot castle-origin Fireball whiffs on far clusters (decision logic unchanged this wave — a range-gated cluster search is the obvious follow-up if playtest confirms).
- Line at origin height over sloped terrain (hero on a hill can overfly units downhill — LineHalfWidth 100 absorbs modest slopes only).
