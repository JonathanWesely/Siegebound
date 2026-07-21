# TASK-239 handoff — NS_Spell_Lightning tall sky strike (art-director) — WIP, BLOCKED on editor restart

Status: **blocked / ~70% complete** · 2026-07-21 21:20 · All finished work is SAVED ON DISK. Blocker: editor hard-deadlock (below). No Git touched.

## Blocker (needs Jonathan)

At 20:56:46 the editor's game thread froze (log frame [92], last line "Compacting FUObjectHashTables" — a GC entered during `MaterialEditingLibrary.recompile_material` on `M_Spell_LightningStrike` never returned; 20+ min with zero log progress, audio-mixer h/w-timeout spam every 5 s, MCP + python remote-exec both dead, process alive burning render/audio threads). This is a deadlock, not a compile wait.

- **Jonathan: kill the UnrealEditor process (PID was 4088) and relaunch.** I did not force-kill per the editor-close law (last human input was ~21:00 — he was active this evening).
- **Nothing valuable is unsaved.** If any prompt appears: do NOT save `L_MainMenu` (it carries only my transient `VFXPREVIEW_*` capture-stage actors: Floor/Capture/Light — a restart discards them, which is the desired cleanup).
- TASK-238's assets are complete + saved and unaffected.

## Design (settled + verified up to the freeze)

Directive: strike from MUCH higher in the sky, much more detailed, ground circle honestly reading the new 700-uu AoERadius (audit row 4: NO radius param from code — authored in-system; spawn is the ground reticle point at ZeroRotator, scale 1, no user params).

Because the automation lanes expose NO Niagara module/value editing (see Limitations), the build is: **donor system duplicate + renderer-level reflection edits + one custom master material that rebuilds the sprite geometry in WPO**:

1. **Base donor**: `/Game/sA_ArcheryVfxPack/FX/NS_LightningShoot_Hit` (one-shot ~0.5 s cyan electric burst; 1 CPU emitter `Electric_Sprite_3`, 1 sprite renderer; `Particles.MaterialRandom` confirmed written — the per-particle band selector works). Looping donors (`NS_LightningShoot`, `_Arrow`, `NS_ArrowShower`) are DISQUALIFIED for the fire-and-forget spawn (would live forever).
2. **`M_Spell_LightningStrike`** (`/Game/VFX/`, additive/unlit/two-sided, Niagara-sprite usage): per-particle `Particle Random` splits the burst population into three layers, each rebuilt in WPO from the UV corners:
   - r < 0.28 → **tall bolts**: vertical jagged ribbons ground → +3400 uu, camera-billboarded about Z, per-bolt horizontal jitter/bend, procedural zigzag core+glow (white core ~14x HDR, cyan glow), 80 Hz flicker.
   - 0.28–0.5 → **ground ring**: flat 1700-uu quad at ground; procedural ring band at r=0.875 ⇒ **exactly 700 uu radius on a 800-uu half-size quad** (honest-scaling law), plus spokes + edge glow + faint fill.
   - r ≥ 0.5 → **core flash**: camera-facing sprite scaled 5x around its center, soft glow + hot core, flicker.
   - Everything multiplied by ParticleColor ⇒ inherits the donor's 0.5 s strike fade envelope.
3. **Renderer reflection edits (applied + saved)**: material → `M_Spell_LightningStrike`, `subImageSize` → (1,1) (donor was a 2x3 flipbook atlas — sub-rect UVs would break the WPO corner math).
4. **System-level (applied + saved)**: `bFixedBounds=true`, box (-1000,-1000,-80)..(1000,1000,3700) — required so the WPO-extended geometry isn't culled (particle bounds alone are ~200 uu).

## Exact state on disk (resume ledger)

| Asset | State |
|---|---|
| `/Game/VFX/NS_Spell_Lightning_NEW` (`Content/VFX/NS_Spell_Lightning_NEW.uasset`) | SAVED: duplicate of donor, compiled ("System successfully compiled"), renderer→my material, subImageSize 1x1, fixed tall bounds. NOT yet swapped to the canonical path |
| `/Game/VFX/M_Spell_LightningStrike` (`Content/VFX/M_Spell_LightningStrike.uasset`) | SAVED but carries ONE KNOWN BUG: pixel custom node ends `return col * PCol;` — float3×float4 HLSL type error (material very likely renders nothing / fails to translate). The one-line fix (`return col * PCol.rgb * PCol.a;`) was mid-apply when the editor deadlocked |
| `/Game/VFX/NS_Spell_Lightning` (placeholder) | UNTOUCHED, still live at the canonical path — the game keeps working meanwhile |

## Resume plan (next session, in order)

1. Editor up → re-enable python remote execution if off (MCP ObjectTools set `/Script/PythonScriptPlugin.Default__PythonScriptPluginSettings` `bRemoteExecution=true`; runner recipe = scratchpad `ue_exec` client, TASK-221 §2 / TASK-231 one-shot pattern). `bThrottleCPUWhenNotForeground=false` if background captures needed (reverts on restart).
2. Apply the material one-line fix via `MaterialEditingLibrary` (load `.../M_Spell_LightningStrike.M_Spell_LightningStrike:MaterialExpressionCustom_1`, replace the return line, `recompile_material`, save). **CAUTION — the deadlock happened exactly here with the Niagara editor window OPEN on the system: do the recompile with NO Niagara editor windows open, and save immediately.**
3. Preview-sim + capture (solo `advance_simulation` + SceneCapture harness — scripts in the session scratchpad: `capture_lib.txt`, `cap_lightning_wip.txt`), iterate material numbers (bolt count/width, ring intensity, flash brightness) via the custom-node `code` property.
4. Swap into place (TASK-187 method, referencers were verified ZERO earlier): delete `/Game/VFX/NS_Spell_Lightning` placeholder → rename `_NEW` → `NS_Spell_Lightning`; open once in the Niagara editor to force compile; close; save.
5. PIE verify at gameplay camera (PIE on whatever map is loaded → in-PIE `open_level L_Arena`; `summon /Script/Engine.SceneCapture2D` for the capture rig — the proven lane; slow-mo dilation 0.15 for frames). Bot rule 3b casts Lightning in real matches for end-to-end observation.
6. AFTER captures → `Tools/ArtPipeline/Cache/TASK-239/` (BEFORE sheets + donor candidate sheets + `WIP1_*` already there); board flip to ready-for-integration; Slack 🎨.

## Donor→derived mapping (reuse ledger)

| Donor (READ-ONLY) | Derived asset (mine, /Game/VFX/) | Relationship |
|---|---|---|
| `sA_ArcheryVfxPack/FX/NS_LightningShoot_Hit` | `NS_Spell_Lightning_NEW` (→ will become `NS_Spell_Lightning`) | full-system duplicate; renderer re-pointed; system bounds overridden |
| (Fire_Magic pack) `NS_Fire_Magic_Projectile3` | `NS_Spell_Fireball` | TASK-238, see its handoff |
| (Ice_Magic pack) `NS_Ice_Magic_FrontSpike` | `NS_Spell_FrostNova` | TASK-238, see its handoff |
| — | `M_Spell_LightningStrike` | authored from scratch (procedural; no donor texture dependencies) |

## Limitations hit (recorded for the pipeline)

- **No Niagara module/value editing exists in any automation lane** (no python graph API in 5.8; converter-plugin contexts absent; MCP reflection hides `EmitterHandles`/rapid-iteration stores). Sanctioned fallback used per dispatch: donor-system duplicate at the canonical path + renderer/system-level reflection edits + material-side authoring. Renderer property surface IS fully writable via MCP ObjectTools (material, subImageSize, bindings, per-renderer MID parameter overrides) — recorded as the reusable adaptation lever.
- **Freshly duplicated Niagara systems don't finish async compile while the editor idles** — open once in the Niagara editor to force it (both TASK-238 systems + the Lightning duplicate needed this).
- **`recompile_material` on a material live on an OPEN Niagara editor's renderer can deadlock the editor** (this task's blocker; reproduced once). Mitigation in resume step 2.
- Editor-side captures: solo-mode `advance_simulation` + SceneCapture2D export (TASK-225 lane) is deterministic and works headless; `summon` console cheat is the way to get a capture actor into a PIE world (no python actor-spawn door there).

## Editor-state / cleanup ledger (for whoever is in the editor next)

- `L_MainMenu` was ALREADY LOADED (and possibly already dirty) at session start; my `VFXPREVIEW_Floor`/`_Capture`/`_Light` transient actors dirtied it further. NEVER save it — a restart discards them (desired). If resuming preview work, `stage_setup.txt` in scratchpad rebuilds the rig; it also deletes stale `VFXPREVIEW_*` actors.
- In-memory (lost on restart, re-apply as needed): `bThrottleCPUWhenNotForeground=false`, python remote execution enabled.
- PIE was started/stopped once (L_MainMenu → L_Arena travel); global time dilation restored to 1.0 before StopPIE.
