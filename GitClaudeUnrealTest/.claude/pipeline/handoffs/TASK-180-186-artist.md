# Handoff — M7 audio SoundCues (TASK-180 + TASK-186)

**From:** art-director · **Status:** both → `ready-for-integration` · **Date:** 2026-07-16

The §6 audio hooks (TASK-179, commit f313253) resolve a SoundCue by SOFT path at `/Game/Audio/S_<Event>` (null-safe — silent until the asset exists). This task created the `/Game/Audio/` folder (did not exist) and authored the cues the pack can genuinely cover. Every authored cue is a **duplicate-into-place of a donor SoundCue** (CONVENTIONS "Audio event cues (M7)" law-sanctioned path; the donor packs are READ-ONLY and were left untouched).

## Authored cues — 6 total (all verified valid SoundCues at the exact contract paths)

| `/Game/Audio/` path | Task | Donor cue duplicated | SoundWave wrapped | Dur | Type |
|---|---|---|---|---|---|
| `S_HeroSwing` | 180 | `MedievalWeaponsSFX/WeaponsSFXCue/WhooshCue/S_Sword_Whoosh_1_Cue` | `S_Sword_Whoosh_1` | 0.59s | world SFX, mono, one-shot |
| `S_HeroHit` | 180 | `.../HitCue/S_Hit_Body_Mono_1_Cue` | `S_Hit_Body_Mono_1` | 0.69s | world SFX, mono, one-shot |
| `S_ProjectileFire` | 180 | `.../BowCue/S_Bow_Mono_1_Cue` | `S_Bow_Mono_1` | 0.75s | world SFX, mono, one-shot |
| `S_ProjectileImpact` | 180 | `.../BowCue/S_Arrow_Hit_Body_Mono_1_Cue` | `S_Arrow_Hit_Body_Mono_1` | 1.13s | world SFX, mono, one-shot |
| `S_UnitSpawn` | 186 | `.../WhooshCue/S_Whoosh_Mono_1_Cue` | `S_Whoosh_Mono_1` | 0.59s | world SFX, mono, one-shot |
| `S_CastleHit` | 186 | `.../HitCue/S_Hitting_Wall_Mono_1_Cue` | `S_Hitting_Wall_Mono_1` | 1.50s | world SFX, mono, one-shot — **WEAK placeholder** |

**Verification (structural readback, per cue):** class == `SoundCue`; `firstNode` is a valid `SoundNodeWavePlayer` pointing at a real, loaded donor `SoundWave` (non-zero `duration` proves the wave resolved); `bLooping == false` (correct — all 6 are `SpawnSoundAtLocation` world one-shots per the TASK-179 loop/2D contract); `volumeMultiplier` 0.75 (donor default). `find_assets /Game/Audio` returns exactly these 6, nothing else. NOTE: I cannot hear audio headlessly — aural confirmation ("plays on its event in PIE") is the build-master TASK-183 check.

### MONO choice (deliberate)
All 6 are `SpawnSoundAtLocation` (3D world) per TASK-179, so I duplicated the **mono** donor variants — mono sources spatialize correctly; stereo sources do not. Donor cues carry no attenuation override, so they play positioned but without distance falloff; if a playtest wants falloff, build-master can assign a shared `SoundAttenuation` (non-blocking, cosmetic).

## Best-effort outcome (TASK-186): 2 filled, 1 left silent
- **S_UnitSpawn — FILLED.** Spec explicitly allowed `MedievalWeaponsSFX WhooshCue`. Used the *generic* `S_Whoosh_Mono_1` (not the sword whoosh) so a unit deploy reads distinct from `S_HeroSwing`.
- **S_CastleHit — FILLED (WEAK placeholder) + a documented deviation.** The convention/coverage-map named `S_Hit_Wood_*` / `S_Axe_Wood_Hit_*` ("NO stone/masonry impact in-pack"). I instead used **`S_Hitting_Wall_Mono_1`** — the pack DOES contain a wall-impact cue, and a structural "hitting wall" thud is a strictly-better masonry stand-in than a wood chop. Still weak (no true stone impact); flagged for upgrade at TASK-188. Orchestrator/manager may veto in favor of the named wood source — a 1-line re-duplicate if so.
- **S_SpellCast — LEFT SILENT (rolls to TASK-188).** `Content/sA_StylizedWizardSet/` ships **NO audio** (only `Blueprints/`, `Fx/NiagaraEmitters/`, `Levels/`, `Materials/`, `Models/`). No fitting cast/muzzle SFX exists in ANY imported pack (checked sA_ArcheryVfxPack, Fire_Magic, Ice_Magic, IceAttack, Prickly_Knight, MedievalCastleEnvironmentAndSiegeWeaponProps — none ship SoundWaves/Cues). Forcing a weapon whoosh onto a spell would be a mismatch, so per "do not force it" the hook stays silent + logged.

## Confirmed SILENT-GAP list for Jonathan (needs a future pack — TASK-188 gate)
These stay soft-ref no-ops (the hook logs once + plays nothing). NOT fabricated, per Jonathan's "silence the rest" call:

| Cue | Needs | Contract flag |
|---|---|---|
| `S_SpellCast` | a magic cast/whoosh SFX | world SFX |
| `S_MinerClink` | a **LOOPING** mining/pick sound (author `bLooping` on the asset) | LOOP |
| `S_CardPlay` | a UI card-play click | 2D |
| `S_CardDiscard` | a UI card-discard click | 2D |
| `S_CastleDestroyed` | a destruction stinger | world SFX |
| `S_VictoryMusic` | victory music | 2D |
| `S_DefeatMusic` | defeat music | 2D |
| `S_OvertimeSting` | an overtime stinger | 2D |

(`S_CastleHit` above is a *weak* placeholder that should also be upgraded when a real stone/masonry impact lands.)

## Notes for integration (build-master)
- **No new soft paths / no code touched.** These cues satisfy the EXISTING TASK-179 contract paths character-for-character. Nothing to wire — the hooks resolve them by string.
- **Dependencies:** each cue soft-references a donor `SoundWave` under `/Game/MedievalWeaponsSFX/WeaponsSFXWav/...` (already imported + in-repo). Commit `Content/Audio/S_*.uasset` (the 6 cues); the referenced donor waves are already tracked.
- **No `Content/RawAssets/Audio/`** — sources are the imported pack, not loose files (CONVENTIONS raw-source rule only applies to loose files).
- **PIE audio check (TASK-183):** drive HeroSwing/HeroHit (hero melee), ProjectileFire/Impact (archer/tower), UnitSpawn (summon), CastleHit (batter a castle) and confirm each cue is audible on its event. The 8 silent cues above should stay silent with no crash.
- **Known limitation (flagged):** Random-node variety + pitch/volume modulation was NOT authored — the MCP toolset exposes no `unreal`-Python / console-exec route and `set_properties` rejects constructing new `SoundNode` subobjects (`/Script/Engine.SoundNodeRandom is not valid SoundNode for property 'FirstNode'`). Single-variant per cue. If repetition grates at playtest, a 2-minute in-editor pass per cue (insert a Random node over the `*_1/_2/_3` variants) is the upgrade — all variant waves are present in the pack.
