# TASK-613 handoff — LANE A: can a scatter visual legally stand in the castle entrance? (gameplay-programmer, DIAGNOSIS ONLY)

**Date:** 2026-08-17 · **Scope kept:** file-only — no code edit, no compile, no editor/MCP, no git, no asset write. Both evidence PNGs read first-hand. `BattlefieldScatter.{h,cpp}` read in full. Every number below re-verified AT the artifact per the RELAYED-DIAGNOSIS LAW — including a byte-level decode of `Content/Data/DA_BattlefieldScatter.uasset` (method in §2).

---

## 0. TL;DR VERDICT (one paragraph, per spec (5))

Under the shipped placement rules, **no blocking scatter layer can legally stand overlapping the castle approach/threshold**: every real-geometry blocker (Rocks / Boulders / Hill / Slabs) is keep-clear-tested with an auto-derived footprint radius that circumscribes its visual, so its visual edge can come no closer than **4,500 uu** to a castle center, while the entire declared walkable approach lies within **3,903 uu** (≥ 597 uu of guaranteed clearance). The two mechanisms the rules DO allow are: **(a) Grass + Plants (14,250 instances, `bBlocking=false`) skip keep-clear ENTIRELY** (`BattlefieldScatter.cpp:691`) and land in the approach (~104 grass + ~17 plants expected per castle apron, every match) and inside the castle footprint (~530 + ~87 expected, grounding at the first collision-hull top the down-trace hits, else **arena ground z≈0 under the visual floor 174** — this is the interior grass in the second PNG, corroborating CF-R2's named prediction code-side); and **(b) the Trees layer's canopy**, whose explicit `FootprintRadius=150` (trunk-only, verified serialized in the DA) lets foliage overhang the keep-clear disc by design — but neither produces a huge smooth yellow-green berm. **The entrance berm is therefore, on paper, NOT a legal scatter instance.** The best mechanism candidate consistent with the pixels and with the code is a **collision-less L_Arena VISTA actor** (`Vista_01..16` / `VistaFill_01..62`, TASK-291/293): they use the very meshes/look in the shot (`SM_Hill_01..03` at 6–10×, `SM_Vista_02/03` "green ridges" at 9–13×, pale `SM_Vista_01/04` "peaks"), are **measured collision-less** ("no baked collision geometry … never collide", TASK-293 §flags), sit at z=0 on rings whose FRONT band (RX 31,000) passes ~**6,000 uu** from each castle center — placed against the pre-9× castle, before the castle grew into their reach. A collider-less visual intersecting the threshold produces exactly the pixel: the hero stands on the real approach/ground collision while the berm surface crosses him at waist height. This is a hypothesis for TASK-617 to confirm or refute by live transform dump (§6), not a verdict.

---

## 1. Question (1) answered exactly

**Q: by the shipped placement rules, can a scatter VISUAL stand overlapping the castle approach/threshold?**
- Blocking layers with auto footprints (Rocks/Boulders/Hill/Slabs): **NO** (proof §4).
- Trees (blocking via proxy): trunk NO; **canopy MAY overhang the disc** (by design), marginal at the approach corner (§4c) — cannot be the berm.
- Grass/Plants (non-blocking): **YES, anywhere** — keep-clear and corridor are never consulted for them (§4d).

**Q: can a COLLIDER differ from its VISUAL there?** Yes, three distinct ways in this file:
1. **By design (Trees):** the visual HISM ships `NoCollision` (`:964`) and blocking is delegated to an invisible engine-`Cylinder` proxy scaled (1.4, 1.4, 17.0) — trunk-only. Everything outside the trunk cylinder is collider-less visual (`ResolveProxyForVisual`, `:999`).
2. **By degradation:** if `CollisionProxyMesh` fails to resolve, `ResolveProxyForVisual` returns null with a warning — *"this layer's visuals have NO collider (degraded, never a crash)"* (`:1016-1019`) — and the per-instance add silently skips the proxy (`:733-742`): whole layer becomes walk-through. (`/Engine/BasicShapes/Cylinder` resolves in an uncooked Development `-game`, so this is a cooked-build risk, not this session's mechanism.)
3. **By mesh asset:** real-geometry blockers get `QueryOnly` + Pawn/SiegeTeam blocks on the HISM (`:944-957`), but the collision GEOMETRY is the static mesh's own BodySetup. A donor with no simple collision and no complex-as-simple would be a "blocking" layer with EMPTY bodies — silent, no log line anywhere in this file. TASK-289 §4 audited `SM_Hill_01` (and tree donors) as carrying "no baked collision geometry"; counter-evidence that hill collision exists in practice: `FindHillSurfaceAt`'s `LineTraceComponent` hill-surface hits demonstrably work (`hill=yes` tokens in shipped MinesPass logs; props seat on hill tops since TASK-250) and units/hero climb hills. The definitive BodySetup read is TASK-617's (§6 item 5).

---

## 2. The measured leads, re-verified at the artifact (spec (2))

| Lead | Verified? | Where |
|---|---|---|
| `IsInKeepClear` consulted **only when `Layer.bBlocking`** | ✅ verbatim | `BattlefieldScatter.cpp:691` (`if (Layer.bBlocking && IsInKeepClear(Candidate, FootprintR))`); twin re-test same gate `:776` |
| `ResolveProxyForVisual` allows collider-less visuals | ✅ | `:999-1020`; per-instance silent skip `:733-742` |
| `RebuildKeepClearZones` — live castle locations, (±25000, 0) fallbacks | ✅ | `:1111-1180`; castle sweep `:1129-1146`, fallbacks `:1147-1154`; PlayerStart disc r=800 `:1163-1179` |
| `CastleKeepClearRadius` = **4,500 live** | ✅ — but see the ⚠ finding below | `ScatterConfig.h:376` (C++ default 4500) **and** a serialized FloatProperty tag in the DA reading **4500.0** (byte offset 0x2368, value bytes `00 A0 8C 45`) |

**Method for the DA (W8-R3's serialized-delta method, taken one level deeper):** I parsed `Content/Data/DA_BattlefieldScatter.uasset` (9,157 B) directly — name table (70 entries @0x19a), SoftObjectPath table (50 entries @0x850), and the full UE5.4-format tagged-property stream of the export (@0xfda, terminating cleanly at the `None` tag @0x2368+). Every decoded value cross-checks against TASK-287's independently-recorded density table exactly (340/300/30/40/40/12250/2000, cull bands, shadows), which validates the decode.

**⚠ FINDING for the manager (provenance, does not change any live value):** the DA **does** serialize `CastleKeepClearRadius = 4500.0`. W8-R3's *conclusion* (4,500 live) is CONFIRMED, but its *mechanism sentence* ("no override was ever serialized") is contradicted by the bytes. Since UE only writes a tagged property that differs from the CDO at save time, a stored 4500 implies the asset was saved with value 4500 **while the header still said 1500** — i.e. plausibly bumped ×3 in-editor during the CASTLE-3X era (the asset's last write is TASK-350's `ea2a70f`, whose contents list includes "DA"), long before TASK-557 re-derived the header. Consequence that outlives this batch: **if the header default ever moves again, the DA will PIN 4,500** — the "two artifacts, two tasks" trap (ScatterConfig.h:361-365) is live, not moot. Absences also confirmed at byte level: **no** `SymmetryMode` (→ Rotational180 live), **no** `ArenaHalfExtent` (→ 26,000 × 12,000), **no** `CorridorHalfWidth` (→ 1,000), **no** `PlayerStartKeepClearRadius` (→ 800), **no** `Mine*` / `AncientGround*` tags (→ C++ defaults live, re-confirming the TASK-576/W8-R3 fact).

---

## 3. The per-layer table (spec (3)) — LIVE serialized DA values, decoded byte-level 2026-08-17

| # | Layer | Meshes | Count | Scale | bBlocking | Bias | MinSpacing | FootprintR | Collision proxy | bAllowOnHills | OverrideMaterial | Cull | Can enter the 4,500 disc? | Can ship collider-less visuals? |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| 0 | **Trees** | `SM-Mobile_Tree_1..12` (`/Game/Tree_Pack_1/Meches/Mobile_Tree_1/`) | 340 | 0.8–1.2 | ✔ | EdgeBias | 300 | **150 (explicit trunk override)** | `/Engine/BasicShapes/Cylinder`, scale (1.4, 1.4, 17.0), Z+850 | true (35°) | — | 24k→32k | trunk NO · **canopy YES (overhang beyond r=150 is un-tested)** | **YES — everything outside the trunk cylinder; whole layer if proxy resolve fails** |
| 1 | Rocks | `SM_Rock_{1,5,4,7,8,2,6,10,9,19}` | 300 | 0.5–1.5 | ✔ | EdgeBias | 350 | 0 (auto) | none (own geometry) | true (35°) | — | 14k→20k | **NO** (edge ≥ 4,500) | only via empty BodySetup (§1.3) |
| 2 | Boulders | `SM_Rock_{11,20}` | 30 | 0.8–1.3 | ✔ | EdgeBias | 900 | 0 (auto) | none | false | — | never | **NO** | only via empty BodySetup |
| 3 | **Hill** | `SM_Hill_01/02/03` | 40 | **0.4–2.5** | ✔ | WholeField | 2000 | 0 (auto) | none | false | **M_HillGrass** | never | **NO** | only via empty BodySetup (⚠ TASK-289 audited the donors as "no baked collision geometry" — §6 item 5 settles it) |
| 4 | Slabs | `SM_Rock_31..35` | 40 | **3.0–5.0** | ✔ | EdgeBias | 1500 | 0 (auto) | none | false | — | never | **NO** | only via empty BodySetup |
| 5 | **Grass** | `SM_Grass_{1,7,3,5,10,14,27,23,17,9}` | **12,250** | 0.7–1.5 | **✘** | WholeField | 50 | 0 | none | true (35°) | — | 6k→9k | **YES — keep-clear + corridor never consulted (`:691`)** | N/A (never has collision: `:964`, NoCollision) |
| 6 | **Plants** | `SM_Plant_{6,3,8,12,15}` | **2,000** | 0.7–1.5 | **✘** | WholeField | 200 | 0 | none | true (35°) | — | 8k→12k | **YES — same** | N/A (never has collision) |

Hill-surface providers (what `bAllowOnHills` layers trace onto, `:556`): Boulders, Hill, Slabs (bBlocking + no proxy + !bAllowOnHills). Rocks opted onto hills, so rocks are NOT a provider.

---

## 4. The geometry, analytic (spec (4))

**Frame:** castles at (±25,000, 0, 0), yaw 0.000 (TASK-350 fingerprint; `CastleMesh` is the actor ROOT — `Castle.cpp:153-154` — so the keep-clear disc center coincides with the mesh pivot; the live-actor sweep and the (±25000, 0) fallbacks are therefore numerically identical). Gate faces **south (−Y)** (TASK-350 agreement check: "OPEN south"). Walkable approach (TASK-611 record): castle-local |x| ≤ 1,470, y to −3,616 → world X ∈ castleX ± 1,470, Y ∈ [−3,616, gate].

Key distances from a castle center:
- Gate mouth center (castleX, −3,616): **3,616 uu**.
- Approach lip corner (castleX ± 1,470, −3,616): **√(1470² + 3616²) = 3,903.4 uu**.
- Keep-clear disc: **4,500 uu** ⇒ the WHOLE declared walkable approach sits inside the disc with **596.6 uu minimum margin**.
- Castle bbox half-depth 3,656.85; XY half-diagonal ≈ 5,197 (ScatterConfig.h:367-373's honest note) ⇒ derived half-width ≈ 3,693.

**(4a) Blocking layers, auto footprint (Rocks/Boulders/Hill/Slabs).** Accept requires `dist(center, castleCenter) > 4,500 + R` where R = mesh XY half-diagonal × rolled scale (`:671-679`), a circle that CIRCUMSCRIBES the instance's XY AABB at any yaw. ⇒ the visual's nearest point stays **≥ 4,500** from the castle center, i.e. ≥ 596.6 uu clear of the nearest walkable-approach point. **Zero legal overlap, independent of the layer's scale range** — a 2.5× giant hill and a 5× slab are excluded by exactly the same inequality because the footprint scales with them. The corridor band (|Y| ≤ 1,000 + R) contributes nothing at the gate (the approach lies at |Y| ≥ ~2,5xx); the disc alone carries the guarantee.

**(4b) The 7,380 box vs the 4,500 disc.** `SpawnBoxHalfExtent = (7380, 7380)` (`Castle.h:374`). The disc does NOT protect the box: blocking-instance edges legally reach 4,500, i.e. **2,880 uu inside the box half-width** (5,937 inside its corner diagonal). Legal-today consequences: scatter blockers can stand inside the unit spawn box; and since the castle's own bbox corners stick **~697 uu outside the disc** (5,197 > 4,500), a blocker's edge can legally interpenetrate the castle's corner masonry visual by up to that depth along the diagonals. Neither touches the walkable approach (fully inside the disc), but both are real "the radii still assume a smaller footprint" symptoms.

**(4c) Trees.** Explicit `FootprintRadius=150` (trunk) ⇒ tree CENTERS ≥ 4,650 from a castle center; the canopy (untested by any rule) intrudes into the disc by `canopyHalfWidth × scale − 150`. It reaches the approach lip corner only if `canopyHalfWidth × scale ≥ 747` (scale ≤ 1.2 ⇒ donor canopy half-width ≥ ~622 uu). Possible overhead foliage at the corner; never a ground berm. The trunk proxy (cylinder r = 50 × 1.4 × scale ≈ 70 uu) is the ONLY collision — by design most of every tree is collider-less visual.

**(4d) Grass + Plants — the unbounded case.** 14,250 instances, uniform X (blue-half draw + exact twin) and uniform Y (WholeField). Densities: grass 12,250 / (52,000 × 24,000) = 9.82e-6 /uu², plants 1.60e-6 /uu². Expected per castle, per match, EVERY match:
- Approach band (2,940 × 3,616 ≈ 1.06e7 uu²): **~104 grass + ~17 plants** — the tuft at the hero's feet in the entrance PNG is exactly this population (legal by design: "lush everywhere").
- Castle footprint bbox (≈ 7,385 × 7,314 ≈ 5.40e7 uu²): **~530 grass + ~87 plants**. Grounding: `GroundZAt` (`:1213-1232`) line-traces `ECC_WorldStatic` with `bTraceComplex=false` — the castle is `BlockAll` (`Castle.cpp:157`), so the trace hits the **top of the first simple-collision HULL** in the column (roof/wall/floor hulls of the 35-hull manifest). Over a **hull-free column** it passes through the castle VISUAL entirely and hits arena ground ⇒ **grass grounds at z≈0 under the interior visual floor at 174** — visible exactly at seams/gaps and to a sunk eye below the floor plane. **This is the interior-grass pixel, and it is lane B's hull-free-column mechanism observed through a second, independent instrument.** CF-R2's grass lead: corroborated code-side.

**(4e) Runtime randomization caveat (spec ⚠):** all of the above is about the RULES; the specific berm Jonathan saw may not reproduce on the next seed. The bounds (4,500-edge exclusion; grass unrestricted) hold for EVERY seed.

---

## 5. What CAN the berm be, then? (mechanism candidates, ranked)

The berm in `entrance-berm-halfbody-sink.png`: smooth, matte, uniform yellow-green, spanning the full entrance width, surface at ~waist height on the hero, hero's support ~90–100 uu below its surface. Ranked candidates:

1. **L_Arena vista dressing (TASK-291/293) — `VistaFill_01..62` + `Vista_01..16`.** NOT scatter — 78 hand/script-placed level actors: `SM_Hill_01..03` at **6–10×**, `SM_Vista_02/03` "**green ridges**" at **9–13×**, `SM_Vista_01/04` pale "peaks" at 11–15×, all z=0, all **measured collision-less** ("no baked collision geometry … read QueryAndPhysics profile but never collide" — TASK-293 §flags) and nav-off. FRONT band ring RX 31,000 × RY 17,000 passes **~6,000 uu** from each castle center near Y≈0; MID 34,000, BACK 36,500. The ring was placed 2026-07-25 against the pre-9× castle; the 9× rebuild (TASK-555/557) pushed the threshold out to ~3,616 uu from center — INTO the reach of a 6–13× scaled hill/ridge whose donor half-extent toward the castle satisfies `halfExtent × scale ≥ ring-to-threshold gap` (≈ 6,000 + for FRONT-band hills ⇒ donor half-extent ≥ ~670–1,000 uu — plausible for the hill donors; MineMinSpacing's own comment caps a 2.5× scatter hill's diameter below 3,000, i.e. donor diameter ~1,200). The pixel's supporting cast matches too: pale blobby masses flanking the entrance = the vista "peaks" look. Mechanism yields the sink exactly: collider-less visual + real support (approach hulls / arena ground) below it. **Live adjudication required (§6 items 3–4).**
2. **The castle's own 9× berm feature** (the manifest names `berm_block_*` hulls, so the castle mesh HAS berm geometry at the approach): if the visual berm outruns its hulls, the sink is castle-geometry-side. This is lane B's question — TASK-615 step (3) checks the FBX profile against the berm silhouette; I flag the coincidence of names, nothing more.
3. **A scatter Hill instance — REFUTED on paper** (§4a) unless TASK-617 finds a live violation of the keep-clear inequality (which would mean a placement bug, e.g. a keep-clear rebuild against wrong castle locations — code path re-read here and found sound: live sweep and fallbacks are numerically identical at (±25,000, 0)).

---

## 6. TASK-617 live checklist (lane A) — every read is read-only, SIE-safe

Coordinate windows: at BOTH castles, world **X ∈ [castleX − 7,500, castleX + 7,500], Y ∈ [−8,000, +8,000]** (covers approach + disc + box overlap band).

1. **HISM instance dump** per scatter component (keyed by mesh): `SM_Hill_01/02/03`, `SM_Rock_*` (Rocks/Boulders/Slabs), `SM-Mobile_Tree_*` + the `Cylinder` proxy, `SM_Grass_*`, `SM_Plant_*`. For each instance in-window: (x, y, z, scale). For blocking layers report `dist(center, castleCenter) − 4,500 − R` (R = half-diagonal × scale) — **any negative value refutes §4a and means a live placement bug.**
2. **Proxy parity:** Trees visual instance count == Cylinder proxy instance count (a mismatch = collider-less trees via the `:733-742` silent skip).
3. **The berm site trace:** at TASK-615's reconstructed evidence (x, y): trace down `ECC_Pawn`, `ECC_WorldStatic`, `ECC_Visibility` — hit component + hit Z vs the on-screen visual surface. The collider-vs-visual delta at that column IS the half-body number.
4. **Identify the berm actor:** query what renders at the berm site (SIE viewport / hit-proxy or actor-bounds sweep). Candidates in order: `VistaFill_*`/`Vista_*` (report name, mesh, scale, location, collision profile, `BodySetup->AggGeom` element count), castle mesh, Hill HISM. Also dump ALL `Vista*` actor transforms + scales within 15,000 uu of each castle center.
5. **Donor BodySetup reads** (settles §1.3 / the TASK-289 "no baked collision" audit): `SM_Hill_01/02/03`, `SM_Rock_11/20/31`, `SM_Vista_01..04`, one `SM-Mobile_Tree_*`: `AggGeom` element counts + `CollisionTraceFlag` (complex-as-simple?). Also `SM_Hill_01..03` XY bounds (needed to close §5's reach inequality numerically).
6. **Seed law:** record the seed of every generate; if the first shows no intrusion, run ≥ 3 regenerations; report absence as evidence, not proof.
7. **Grass/plants census:** count in-window grass/plant instances inside (a) the approach rect, (b) the castle footprint bbox, with a Z histogram — instances at z≈0 under the 174 floor are the interior-grass mechanism made measurable; compare against §4d's expectations (~104/~17 apron, ~530/~87 footprint).

---

## 7. Costed repair sketch — ⛔ INPUT to the manager's spec ONLY (CF-R1), not started, no ID

- **R-A1 (code, one compile): castle-disc keep-clear for non-blocking layers.** In `ScatterLayer`, additionally test `IsInKeepClearDiscs(Candidate, FootprintR)` (+ twin) when `!Layer.bBlocking` — discs only, NOT the corridor, so the lane stays lush and only the two castle pads lose decoration. Kills the interior/under-floor grass and the approach tufts at the root. Cost: ~6 lines in `BattlefieldScatter.cpp` (primary `:691` region + twin `:776` region), zero draw-sequence change (keep-clear is draw-free), layouts change per seed (allowed — cross-build stability is not a contract). Compile gated on Jonathan's standalone being closed (G4) + QUIET-MODULE.
- **R-A2 (no change recommended): Trees FootprintRadius 150.** The trunk-only footprint is the deliberate M6.6 canopy design; no evidence it produced either pixel. Leave unless TASK-617's dump shows a canopy in the approach.
- **R-A3 (NOT a scatter repair — routed to the manager): the vista ring vs the 9× castle.** If §6 item 4 confirms a vista actor as the berm: the fix is level-side (re-ring the ~6–10 offending FRONT/MID instances outward or rescale — the TASK-293 placement script pattern makes this a bounded edit), which REQUIRES an L_Arena save ⇒ CF-R4 escalation to Jonathan with the evidence, art-director/build-master lane. Giving vista meshes collision is the wrong direction (they are deliberately non-colliding backdrop; collision there would carve nav and block nothing useful).
- **R-A4 (belongs to lane B if it lands): castle berm hull under-coverage** — manifest-hull extension, the R1 pattern; not specced here.

**Standing bookkeeping finding (manager):** §2's ⚠ — the DA pins `CastleKeepClearRadius=4500` as a serialized override; record it so the next header re-derivation doesn't silently diverge, and correct W8-R3's mechanism sentence (its 4,500 conclusion stands).

---

## 8. Cross-lane notes

- **Lane B (TASK-615):** §4d gives you the code-side grounding law for the grid's "support identity" column: first simple-collision hull top from above, else arena ground z≈0, `bTraceComplex=false`, castle profile BlockAll. My §5 item 1 is your step (3)'s strongest cross-check: if no castle FBX surface matches the berm profile AND a vista transform does, both lanes close together.
- **Lane C (TASK-614):** nothing in this file places anything at the doorway that could be the blue bar; `HPBarWidget` rides the castle at relative Z 9,450 (`Castle.cpp:174`) — lane C's to adjudicate.
- **Instrument-protection law (CF-R3) honored:** no console, no `M`, no `DumpAssistantPrompt`, no L_Arena touch, no editor session at all.
