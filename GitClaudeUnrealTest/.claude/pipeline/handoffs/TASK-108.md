# TASK-108 Handoff — Spell Niagara VFX set: NS_Spell_* ×5 + NS_ChainZap + M_SpellReticle (art-director)

Status: **COMPLETE — 6 Niagara systems duplicated from the template donor, restyled per spell, saved clean; M_SpellReticle authored (DeferredDecal emissive ring), compiled with zero shader errors, saved clean. All 7 paths readback-verified character-exact.** Editor was FREE — `IsPIERunning` → false before any mutation; editor left UP (TASK-103's compile bounce can proceed after me). No Git run (read-only `git status` only). No TASKBOARD edit. Donor `NS_Damage` verified untouched (not dirty, absent from git status).

## Deliverables (all exist, `is_dirty` = false post-save)

| Asset | Path | Class | Donor |
|---|---|---|---|
| NS_Spell_Fireball | `/Game/VFX/NS_Spell_Fireball` | NiagaraSystem | duplicate of `/Game/Variant_Combat/VFX/NS_Damage` |
| NS_Spell_FrostNova | `/Game/VFX/NS_Spell_FrostNova` | NiagaraSystem | same donor |
| NS_Spell_Lightning | `/Game/VFX/NS_Spell_Lightning` | NiagaraSystem | same donor |
| NS_Spell_BattleCry | `/Game/VFX/NS_Spell_BattleCry` | NiagaraSystem | same donor |
| NS_Spell_Pickpocket | `/Game/VFX/NS_Spell_Pickpocket` | NiagaraSystem | same donor |
| NS_ChainZap | `/Game/VFX/NS_ChainZap` | NiagaraSystem | same donor |
| M_SpellReticle | `/Game/Materials/M_SpellReticle` | Material | authored from scratch (M_CenterlineStripe recipe lineage) |

New files on disk: `Content/VFX/NS_*.uasset` ×6, `Content/Materials/M_SpellReticle.uasset` (UE's Git provider auto-staged them on save — observed prior behavior, left as-is; build-master owns the commit at TASK-109).

## Donor choice note (spec deviation, recorded)

The spec said "Variant_Combat Niagara donors" (plural) — **Variant_Combat contains exactly ONE Niagara system, `NS_Damage`**; the only other template systems are `NS_JumpPad` (LevelPrototyping) and `NS_Jump_Trail` (Variant_Platforming). All six spells were therefore duplicated from `NS_Damage`. This turned out to be the strongest possible donor: it is a **stateless (lightweight) emitter** system, whose modules are plain reflected UObjects — fully editable via MCP property tools (shape/velocity/color/lifetime/burst are data, not compiled graph). Emitter subobject in every duplicate: `:DirectionalBurst_1` (NiagaraStatelessEmitter).

## Restyle recipes (per system; donor baseline: 300 white 1×3–2×5 velocity-aligned sparks, cone-up 100–500, gravity −980, drag 0.8–1.2, life 0.4–0.8, one 1 s loop, alpha fade-out curve, DefaultSpriteMaterial)

Colors are HDR linear (values > 1 bloom under the tonemapper; hue survives at the fringes). All keep the donor's alpha fade-out (ScaleColor curve), Once-loop 1 s, burst @ t=0, DefaultSpriteMaterial (vertex-color-driven, so no material swaps needed).

| System | Burst | Color (RGB) | Life (s) | Sprite | Shape | Velocity | Gravity | Align | Emitter fixedBounds |
|---|---|---|---|---|---|---|---|---|---|
| Fireball | 500 | 8, 1.4, 0.08 (red-orange) | 0.5–0.9 | 6×14–10×26 streaks | Sphere r60 core | FromPoint 500–900 radial | −300 | VelocityAligned | ±350 |
| FrostNova | 600 | 2, 6, 12 (ice blue-white) | 0.7–1.1 | 10–18 round | **Ring r350** (ground plane, edge-exact) | FromPoint 60–160 outward drift | 0 | Unaligned | ±450 xy, −50..250 z |
| Lightning | 320 | 7, 4, 14 (white-violet) | 0.25–0.5 | 4×60–8×140 vertical streaks | Cylinder r350, h700, midpoint 0 (0..700 up) | Linear (0,0,−1400) plunge | 0 (drag 0.1–0.3) | VelocityAligned | ±500 xy, −50..900 z |
| BattleCry | 500 | 10, 7, 1 (gold) | 0.8–1.2 | 8–16 round | **Ring r400** (ground plane) | Linear (0,0,+200) rising rally | 0 | Unaligned | ±500 xy, −50..400 z |
| Pickpocket | 40 | 8, 5.6, 0.8 (coin gold) | 0.7–1.0 | 7–12 round | Sphere r25 | InCone 35°, 250–450 up-toss | −980 (coins arc down) | Unaligned | ±150 |
| ChainZap | 120 | **0.8, 6.4, 8.0** = TASK-105 crystal cyan (0.10, 0.80, 1.00) × 8 | 0.3–0.55 | 1.5×8–3×16 sparks | origin (module off, donor default) | FromPoint 200–500 crackle | −300 | VelocityAligned | ±150 |

## Radius-read notes (§3.11: visual ≈ gameplay radius)

- **Fireball (AoERadius 300):** radial speed 500–900 under drag ~1.0 → travel ≈ v·(1−e^(−d·t))/d ≈ 250–380 over the 0.5–0.9 life. Dome read ≈ 300. Match.
- **FrostNova (350):** particles spawn **exactly on** the ring edge at r=350 (engine-source-verified, see below). Exact match.
- **Lightning (400 / top-3 targets):** strikes fall **inside** r350 of the 400 zone — deliberate: strike-rain inside the area reads better than an edge ring for a "strikes the top targets anywhere in the zone" spell; the reticle (below) carries the exact 400 boundary during targeting.
- **BattleCry (400):** exact ring at r=400. Match.
- **Pickpocket:** no gameplay radius (instant global GoldSteal, manager ruling — no reticle); small local coin toss at the spawn point.
- **NS_ChainZap:** per-hit impact (spawned at each chain hit by ATower per TASK-101, soft path `/Game/VFX/NS_ChainZap.NS_ChainZap`); compact ±150 read, no AoE claim. Cyan matches M_CrystalGlow so tower and zap speak one language.

## M_SpellReticle (graph + scaling contract for TASK-100)

- Domain **MD_DeferredDecal**, blend **BLEND_Translucent** (M_CenterlineStripe recipe lineage). Compiled clean via MaterialTools.recompile — zero shader errors. Wiring readback-verified.
- Graph: `TexCoord` + `Constant2Vector(0.5,0.5)` → two `SphereMask`s (A=UV, B=center; outer radius **0.48**, inner **0.42**, hardness 92%) → `Subtract` = annulus band → **Opacity**; band × `Constant3Vector(0.4, 2.5, 8.0)` (HDR spell-blue) → **EmissiveColor**.
- **Spell-blue vs centerline gold:** centerline stripe is (5, 4, 0.5) warm gold; reticle is (0.4, 2.5, 8.0) blue — unmistakable apart. (BattleCry's burst is gold, but it is a 1 s VFX, not the targeting reticle.)
- **Scaling read (IMPORTANT for TASK-100):** the ring band is authored in decal-UV space at 0.42–0.48 UV-radius. On an ADecalActor whose DecalSize half-extents = AoERadius, the visible ring sits at **0.84r–0.96r** (band center ~0.90 of the gameplay radius, deliberately just inside so the band never clips the decal edge). If a pixel-exact outer edge is wanted, scale DecalSize by ~1.06; the current read is within §3.11 "roughly matches".
- Decal-domain thumbnails don't preview meaningfully (known engine limitation; the sphere preview is blank-ish) — the in-level proof-of-recipe is M_CenterlineStripe itself, live since M1.

## Verification performed + limits (honest record)

- **Readback sweep (all 6 systems + material):** every value in the recipe tables above read back from the saved assets in one programmatic pass — burst counts, colors, lifetimes (min/max AND LUT store), sizes, shapes/radii, velocity types/speeds, gravity, alignment, bounds, renderer material, domain/blend, output wiring. All character-exact. All assets `is_dirty` false after save.
- **Live visual proof (Fireball):** Niagara editor preview captured mid-burst — restyled wide orange dome confirmed live (proves the whole duplicate → subobject-edit → stateless rebuild → render chain).
- **Ring math proof (FrostNova/BattleCry):** UE 5.8 engine source (`NiagaraStatelessModule_ShapeLocation.cpp`): with DiscCoverage=0, RadiusScale=0 / RadiusBias=RingRadius ⇒ particles at **exactly** ringRadius on the AxisX/AxisY (ground) plane, full circle. Zero-particle risk ruled out at the source level.
- **Preview-capture limitation (flagged):** the Niagara editor preview plays a one-shot system ONCE at tab-open (often behind a "Compiling..." shader overlay) and does NOT restart on MCP property writes (no PostEditChange path) — so FrostNova/Lightning/BattleCry/Pickpocket/ChainZap could not be re-caught on camera after their tab-open burst was consumed. Their correctness rests on the readback sweep + the two proofs above. **In-PIE visual confirmation rides TASK-109's exit-criteria run**, which drives actual spell plays via SendInput — the natural checkpoint for §6 one-frame readability in context.
- BattleCry was temporarily given a 10 s lifetime / r60 ring during preview debugging — **restored to final values and re-verified in the readback sweep** (life 0.8–1.2, ring 400 confirmed).
- Scratch captures written to `Saved/MCPCaptures/` (gitignored) — disposable.

## Flags for M7 (ruling 11 — at/below the §6 bar but shippable for M5)

1. **All six are single-emitter spark-burst restyles** — they clear one-frame readability (HDR color + 120–600 particles + radius-matched shapes) but have no textured flipbooks, ribbons, meshes, or ground decals. M7 premium VFX pass replaces them at the same paths.
2. **NS_Spell_Lightning** reads as violet strike-rain, not branched bolts — a stateless sprite emitter cannot do beams/ribbons. Flagged as the weakest §6 read of the set; M7 wants a ribbon/beam bolt.
3. **NS_Spell_Pickpocket** coins are round gold sprites, not disc meshes — reads "gold flourish" more than "coins". Acceptable for an instant-cast flourish; M7 coin mesh upgrade.
4. **M_SpellReticle** is a flat emissive band — no rotation, tick marks, or pulse. M7 candidate: panner rotation + edge pulse.

## Integration notes (TASK-109 / build-master)

- Code contracts already in place: `USpellLibrary::ResolveSpell` composes `/Game/VFX/NS_Spell_<CardID>` (null-safe, TASK-098); `ATower` defaults `ChainZapEffect` to `/Game/VFX/NS_ChainZap.NS_ChainZap` (TASK-101); TASK-100 soft-references `/Game/Materials/M_SpellReticle`. Names here are character-exact to the TASK-108 names block — no wiring needed from me, and none performed (integration is build-master's).
- Systems auto-complete (emitter Once + inactiveResponse Complete) — `SpawnSystemAtLocation` defaults auto-destroy the component; no lingering actors.
- All six have emitter-level fixed bounds set (table above) — no dynamic-bounds cost, no culling surprises at radius extremes.
- An editor popup "7 changes to source content files detected — import?" was up during my session (CardArt PNG watcher, pre-existing, not mine). I did not touch it; whoever owns the editor next should dismiss with Don't Import unless TASK-106 says otherwise.
