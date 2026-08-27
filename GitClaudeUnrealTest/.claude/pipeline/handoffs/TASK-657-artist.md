# TASK-657 — [F1-2] VID-001-F1 branch (i): the castle-entry DISCOVERABILITY asset set (art-director handoff)

**Status: COMPLETE — ready-for-integration (orchestrator flips the board).** Date: 2026-08-27.
Three furnishing families authored PROCEDURALLY (zero Meshy/TRELLIS spend — GH-R4), full texture-bake lane, imported LIVE into the editor (PID 15772, MCP raw JSON-RPC lane, scratchpad `mcp_client.py`), explicit per-asset saves only.
**⛔ Fences held:** no placement in any level (TASK-661 owns it) · `L_Arena` NEVER saved — SHA256 entry == exit == ledger `b3dbc5d9ae484a7bd02cafad52b4681da68b011477479b65ee7781ae459f8268`, `is_dirty` false at entry, after save, and at exit · no git · no compile · no TASKBOARD edit · no PIE/console/`M` · Content/ + manifest only (the 661 parallel-safety contract).
Laws: the F1-R2 activation ruling (binding scope) · F1-R3 (no seal touched — these are DRESSING) · GH-R4/R9 · the pivot law (ground contact, +X forward — the 661 contract) · the SM_Torch collisionless precedent · the pinned LOCKED delight profile (DEFAULT-TRAP law) · `SC-§15` (departures §6) · cp1252/ASCII tooling law.

**Branch record (spec COMMON clause):** BRANCH (ii) CLOSED NO-OP 2026-08-27 (hero never reached any gate — 656 §3). BRANCH (iii) CLOSED NO-OP 2026-08-27 (live == manifest at every probed face; no rogue collision — 656 §2/§3). Both were closed by the activation ruling on the board; restated here for the record.

---

## 1. THE DELIVERABLE SET (all 16 assets on disk, saved, dirty=false)

| family | mesh(es) | MI (parent `M_AssetPBR`) | textures (1024²) |
|---|---|---|---|
| gate-ward marker | `/Game/Meshes/SM_Castle_GateBanner` | `/Game/Materials/MI_Castle_GateBanner` | `/Game/Textures/T_Castle_GateBanner_{D,N,ORM}` |
| road ribbon | `/Game/Meshes/SM_Castle_TramplePath` | `/Game/Materials/MI_Castle_TramplePath` | `/Game/Textures/T_Castle_TramplePath_{D,N,ORM}` |
| seal-line dressing | `/Game/Meshes/SM_Castle_ToeRock01` + `SM_Castle_ToeRock02` | `/Game/Materials/MI_Castle_ToeRock` (SHARED by both rocks) | `/Game/Textures/T_Castle_ToeRock_{D,N,ORM}` (shared atlas: 01 = left half, 02 = right) |

Raw sources: `Content/RawAssets/Castle_{GateBanner,TramplePath,ToeRock01,ToeRock02}.fbx` + `Content/RawAssets/Textures/Castle/T_Castle_*_{D,N,ORM}.png` (9).
Generator (deterministic, re-runnable, ~15 s/family): `Tools/ArtPipeline/build_entry_dressing_props.py` (`--family GateBanner|TramplePath|ToeRock`). Machine-readable readbacks: `Tools/ArtPipeline/Cache/EntryDressing/props_report.json`; previews + debug bakes beside it.

## 2. PER-ASSET MEASURED READBACK (Blender report == UE `get_bounds` to float dust; roundtrip probe with the diag(1,−1,1) UE-handedness emulation IDENTICAL)

| mesh | dims (uu) X×Y×Z | bbox | min-Z | tris | slot (exactly 1) | Nanite | LODs | simple collision |
|---|---|---|---|---|---|---|---|---|
| `SM_Castle_GateBanner` | **88.0 × 544.4 × 1168.0** | x −39..+49, y ±272.2 | 0.0 | 1,912 | `GateBannerPBR` → `MI_Castle_GateBanner` | **OFF** | 1 (LOD0) | **0 elements — every AggGeom array EMPTY** |
| `SM_Castle_TramplePath` | **600.0 × 348.0 × 2.98** | x ±300 exact, y ±174 | 0.0 | 1,976 | `TramplePathPBR` → `MI_Castle_TramplePath` | **OFF** | 1 | **0 elements** |
| `SM_Castle_ToeRock01` | **299.1 × 327.4 × 314.4** | y −125.7..+201.8 (side-boulder at +Y) | 0.0 | 714 | `ToeRockPBR` → `MI_Castle_ToeRock` | **OFF** | 1 | **0 elements** |
| `SM_Castle_ToeRock02` | **400.5 × 268.3 × 209.7** | low-wide variant | 0.0 | 480 | `ToeRockPBR` → `MI_Castle_ToeRock` | **OFF** | 1 | **0 elements** |

**Collisionless on purpose, proven not assumed:** `remove_collisions` returned true ×4 post-import (belt), then `BodySetup_0.AggGeom` read back — sphere/box/sphyl/convex/tapered/levelSet arrays ALL EMPTY ×4 (braces). The manifest DECLARES `ucx: null` for all four (§4), so per the CONVENTIONS acceptance rule this zero is "zero on purpose", not a STOP. TASK-661 adds the third layer: `SetCollisionEnabled(NoCollision)` code-side on every spawned component (GH-R9). **The entry-chain collision record is untouched — no castle mesh, no BodySetup of `SM_Castle*` (castle) was read or written by this task.**

**MI wiring readback (×3 MIs):** created via `MaterialInstanceTools.create`, parent readback `/Game/Materials/M_AssetPBR.M_AssetPBR` **non-None** (the TASK-633 §10 phantom trap explicitly checked); params `BaseColor`/`Normal`/`ORM` set + `get_texture_parameter` readback exact ×9. Texture settings readback: `_D` sRGB=true TC_Default · `_N` sRGB=false **TC_Normalmap** · `_ORM` sRGB=false **TC_MASKS**, all 1024×1024 (`get_size`).

## 3. PIVOT / FORWARD — THE TASK-661 CROSS-LANE CONTRACT (honored + notes)

- **All four meshes: min Z = 0.0000 = ground contact; +X = forward.** Verified in the Blender report AND in the UE bounds readback after the handedness round trip.
- `SM_Castle_GateBanner`: the **pole axis is the pivot** (0,0); the cloth bows +26..+46 uu into +X, so the XY bbox is deliberately asymmetric (x −39..+49). **The banner cloth FACES ±X (readable from +X); spawn it with local +X toward the approaching viewer** (at the mouth: toward the south field). Crossarm/cloth width runs along Y (±272). Height 1168 ≈ 6× the 180-uu human — silhouette-readable at the ~4,300-uu spawn→mouth sightline (preview `GateBanner_front_far.png` is the 2,500-uu check).
- `SM_Castle_TramplePath`: pivot = segment XY centre; **tiles along local X** (the travel axis) at exactly **600-uu spacing**; end faces at x ±300 are straight and profile-identical (ruffle + texture periodic by construction) so butted segments read continuous. Spec'd z +2 over measured support stands (mesh is 3 uu of relief; no z-fight at +2).
- `SM_Castle_ToeRock01/02`: pivot = ground contact under the boulder mass; ~200-uu spacing with yaw jitter overlaps the ~300/400-uu widths into a continuous rubble line (previewed exactly so in `ToeRock_sealline_vs_berm.png`). 01 carries a small side-boulder at +Y (bbox +201.8) — harmless under yaw jitter, collisionless.

## 4. MANIFEST DELTA (`Tools/ArtPipeline/pipeline_manifest.json` — 22 → 26 assets; JSON re-parsed clean)

Four new `assets` entries appended after `GoldNode`: **`Castle_GateBanner` · `Castle_TramplePath` · `Castle_ToeRock01` · `Castle_ToeRock02`** — each `category: "castle-prop"`, `origin: "ground-center"`, measured `target_dims_ue`, `team_region: null` (NEUTRAL props — the M4.5 law, no TeamRegion slot), and **`ucx: null` with an `_ucx_source` declaring COLLISIONLESS ON PURPOSE (the SM_Torch precedent) + the 661 code-side enforcement**. `_comment_task657` on each records authoring lane, pivot law, and placement ownership. **No existing entry touched** (the `Castle` entry and its 66-hull `ucx.boxes` are byte-identical — the collision authority is unmodified).

## 5. TEXTURE GATES — MEASURED (in-script, on the shipped PNG data; profile = the pinned LOCKED delight `{ao_divide 1.0, ao_floor 0.25, gamma 0.55, gain 1.2}` + shoulder 0.80/max_out 0.98)

| gate | GateBanner | TramplePath (run 2) | ToeRock | law |
|---|---|---|---|---|
| UV-norm covered mean linear (floor **0.2536**) | **0.2593 ✅** | **0.2783 ✅** | **0.3325 ✅** | fleet albedo floor |
| declared-intent retention (band 0.85–1.25, warn-only) | ⚠️ **1.3029 over-band** (§6.1) | 1.0369 ✅ | 1.1701 ✅ | intent card = authored post-delight palette, area-weighted (no concept PNG exists — same posture as 630 §5.2) |
| operative anti-bleach guard (>1.25 AND UV-norm >0.60) | **NOT tripped** | NOT tripped | NOT tripped | the LAW's trigger |
| chroma C*ab covered vs intent | 39.2 vs 36.8 | 16.1 vs 18.1 | 2.03 vs 2.89 (neutral grey held) | report-only |
| ORM sanity | AO 0.864 · rough ≤0.851 · metal 0 | AO 0.981 · metal 0 | AO 0.795 · metal 0 | R=AO/G=rough/**B=0** (house pattern — gold is albedo+roughness, no metal channel spent) |

Delight ledger: mean linear before→after = GateBanner 0.0508→0.165 · TramplePath 0.0486→0.230 · ToeRock 0.0481→0.2225 (authored palettes are the analytic AO=1 inversion of the intended post-delight palette — the declared TASK-630 posture; the AO-divide's crevice lift rides on top).

**EYEBALL GATE — PASS, 8 previews viewed** (`Cache/EntryDressing/previews/`): the banner reads bold crimson + gold double chevron POINTING DOWN, crisp at the 2,500-uu far view beside the 180-uu human proxy; the path chain reads as tan trampled road with wandering wheel ruts, strong contrast on green, three segments butting continuously; the rock line reads as a crown of pale angular boulders over a chartreuse berm proxy — unmistakably "rubble barrier, not a door" at the hero's eye height (`ToeRock_walkup_view.png`).

## 6. `SC-§15` DECLARED DEPARTURES (none silent)

1. **GateBanner intent retention 1.3029 > 1.25 band.** Mechanism (same as 630 §5.2): the intent card assumes AO=1 while the shipped delight divides by the real AO≈0.86 field, margin texels skew bright at 0.64 coverage, and the card's area weights are estimates. The operative bleach guard is NOT tripped (UV-norm 0.2593 « 0.60) and the eyeball shows saturated crimson, not chalk. Jonathan's eye at TASK-660 stays the final authority.
2. **TramplePath run 1 FAILED the albedo floor (0.2464 < 0.2536)** — the intent palette itself was under-floor (retention 1.036 proved the bake faithful). Palette lifted +12% (recorded in the script + manifest comment), run 2 shipped at 0.2783. Run-1 textures were never imported.
3. **`SM_Castle_ToeRock02` built** — the spec's optional second rock taken, for line variety (identical pattern, shared MI/textures; cost ~0).
4. **No lightmap UVs channel generated** — the live `StaticMeshTools.import_file` lane exposes no such flag (the TASK-566 finding). These props are runtime-spawned cosmetic components with no static-lighting dependency (the torch precedent ships the same way); a future static-light ruling would need a re-import, declared not owed.
5. **Roughness bake min 0.004–0.008 on a handful of covered island-edge texels** (margin antialiasing dust; visually nil — body values 0.80–0.87).
6. **TramplePath segment joins show a subtle transverse shading crease** in the chain preview (end-butt vertex normals). Profiles/textures are periodic-exact; the crease reads as a natural ground seam and 661's placement variation softens it further.
7. Single material slot per mesh, **NEUTRAL** (no TeamRegion) — per the M4.5 environment-prop law and the manifest `team_region: null`; both castles receive identical dressing via 661's castle-local spawns, so team identity comes from context, not recolor.
8. Tooling note: Blender 5.1.2 emits `Material.use_nodes` deprecation warnings (removal in 6.0) from the generator — future-Blender debt on the whole ArtPipeline lane, not this task's behavior.

## 7. ON-DISK SHA256 TABLE (for TASK-659 §25b — every file this task wrote or could have touched)

| file | sha256 | bytes |
|---|---|---|
| `Content/Meshes/SM_Castle_GateBanner.uasset` | `fe1d04c6049a37e1371618866e99d55e7649e9ce0ae270a8470572709231ed11` | 77,335 |
| `Content/Meshes/SM_Castle_TramplePath.uasset` | `ec21750523dc57fee82a55bf03291acd839c7b6e8c4439a8ffcd1ce6f0bd3872` | 66,033 |
| `Content/Meshes/SM_Castle_ToeRock01.uasset` | `6a2ef7ec374444688eddd16e1d8381b7045e7b2d57d5d4202fc28933a88f72a5` | 49,225 |
| `Content/Meshes/SM_Castle_ToeRock02.uasset` | `bf4d544412c7f22e32f96ef0e484db936fd6facf444c3d6260caf9dc7e9e44ba` | 38,462 |
| `Content/Textures/T_Castle_GateBanner_D.uasset` | `f74a875b8879d94caabb9572e4465ba06603896e7d185324fe85df8db80c7dfe` | 524,623 |
| `Content/Textures/T_Castle_GateBanner_N.uasset` | `98fd15ed084763d1b7bd9c69f7ea4da896098bd6125648715ab335b94d5a3588` | 51,843 |
| `Content/Textures/T_Castle_GateBanner_ORM.uasset` | `c7d3d2d3c6cc60f2b300fd985b3c3239ee82dbbed1596c8d3c2b70d9b242f862` | 336,805 |
| `Content/Textures/T_Castle_TramplePath_D.uasset` | `b7eb9f653933bd527f88733ec69f971c591a5b46699e50fcbb92ea55314784ff` | 635,394 |
| `Content/Textures/T_Castle_TramplePath_N.uasset` | `250164056b69307eebadf9ff38ebc1c966eed3cc9b389aebc305a6ab118c5141` | 27,478 |
| `Content/Textures/T_Castle_TramplePath_ORM.uasset` | `94f9691b58bfc0fd124c18f366410f569aef4908c93f4e18ca77d3104ae2319b` | 293,097 |
| `Content/Textures/T_Castle_ToeRock_D.uasset` | `6f4d47262547bd5d7ba3a249d1f3acc60a00217ef77648f17d3db547d02eea13` | 722,959 |
| `Content/Textures/T_Castle_ToeRock_N.uasset` | `1d61d794a9f77cc442021f2428a9d59983190b78a2cbc67d326a69c9cc567b70` | 14,309 |
| `Content/Textures/T_Castle_ToeRock_ORM.uasset` | `2cd9c8acac109b6d7fcc5f8f21afa1c93c783bc86c948a3a36bb4e2cc246a05a` | 481,494 |
| `Content/Materials/MI_Castle_GateBanner.uasset` | `32c7a935eb7ba12d677e85e36fc6838d4c962761ca4e484be2b504fde2dbb275` | 10,782 |
| `Content/Materials/MI_Castle_TramplePath.uasset` | `2d02a5d38755d1ffcee8d3351067fb9cc93e0506b53d1dde3a35117a9cbad602` | 9,934 |
| `Content/Materials/MI_Castle_ToeRock.uasset` | `a31f9fcd0dd9c1309fca50eaf9495a609d51d189b5e6db2eaeac70b8c38f407c` | 12,739 |
| `Content/RawAssets/Castle_GateBanner.fbx` | `393e2f2e1d28ba7889afb6d2db786b07cb3da0d7db22cfd219a90f66d56e417d` | 74,044 |
| `Content/RawAssets/Castle_TramplePath.fbx` | `748c5bd94f5ed18cdd8cfad412e39cd9ae439259a6fe2023c91d115a48bde963` | 62,620 |
| `Content/RawAssets/Castle_ToeRock01.fbx` | `e795e93defaa5768f0187b90322805916995d61484663ed2502578e54c2ec841` | 49,948 |
| `Content/RawAssets/Castle_ToeRock02.fbx` | `ef9972ab331ad091d92fbee83db68e760a7f5e89f0f6304d7f4f6c07c35e4447` | 39,676 |
| `Content/RawAssets/Textures/Castle/T_Castle_GateBanner_D.png` | `cbe163476435e8bc207bf5a2e6925a9e3c06f3c38baec73d14ec9e45470ec932` | 548,009 |
| `Content/RawAssets/Textures/Castle/T_Castle_GateBanner_N.png` | `fda7756ae867d3213b4039a18005eeb0553ecd21d0768f07d380de0ec55bc16b` | 69,272 |
| `Content/RawAssets/Textures/Castle/T_Castle_GateBanner_ORM.png` | `4598b82e8c025507f3ccb3e41d42416bac58d8621462404fd3b426dd568d34ff` | 411,797 |
| `Content/RawAssets/Textures/Castle/T_Castle_TramplePath_D.png` | `4b69b2a022fc5be15418a765b1f44b3bb549193afb3ecddd43217d4d35046c9d` | 702,863 |
| `Content/RawAssets/Textures/Castle/T_Castle_TramplePath_N.png` | `c5fcde8d8d3ec3bf34ac93661b099c04e78cc06d045a7116edd32f923c509ec1` | 34,088 |
| `Content/RawAssets/Textures/Castle/T_Castle_TramplePath_ORM.png` | `a6f79aff73f177b398558b4f39d9fc10575cfb528505c7b02260704923013f94` | 382,389 |
| `Content/RawAssets/Textures/Castle/T_Castle_ToeRock_D.png` | `286c03dfa48b2b770afb8e4bef045573d49b144468541b2ee57374d38be674af` | 757,986 |
| `Content/RawAssets/Textures/Castle/T_Castle_ToeRock_N.png` | `9d635534e475c7dc60f869767dbd9b0f37a94ddd7aa9a7b98642a6bd4febe8bc` | 15,960 |
| `Content/RawAssets/Textures/Castle/T_Castle_ToeRock_ORM.png` | `2e85efcf3450f278757d68c48771ada0b09d65625716046ed73ececf668592da` | 563,353 |
| `Tools/ArtPipeline/pipeline_manifest.json` | `452c5b4bb29e3cff35a9af746c6b0e44ade7623eba4b7474400c0b9d3f48319d` | 84,040 |
| `Content/Maps/L_Arena.umap` | `b3dbc5d9ae484a7bd02cafad52b4681da68b011477479b65ee7781ae459f8268` | 535,522 **(UNCHANGED — ledger match)** |

Also written (uncommitted, 659's carrier decides): `Tools/ArtPipeline/build_entry_dressing_props.py` (the generator) + `Tools/ArtPipeline/Cache/EntryDressing/**` (report, previews, debug bakes — Cache is the standing evidence lane).

## 8. PREVIEW RENDERS (the pre-import eyeball evidence; the LIVE before/after pair at 656's M1/M2 transforms is TASK-659 step (2), not mine)

`Tools/ArtPipeline/Cache/EntryDressing/previews/`:
`GateBanner_{threequarter,front_far,detail}.png` · `TramplePath_{chain_threequarter,walk_view,detail}.png` · `ToeRock_{sealline_vs_berm,walkup_view,pair_detail}.png` — each family staged on grass-green ground with a 180-uu human proxy; the ToeRock scene includes a chartreuse berm proxy replicating the VID-001 rim read. Debug set (pre-delight D, AO, R) in `previews/../debug/`.

## 9. NOTES FOR TASK-661 (placement) AND TASK-659 (integration)

- **661:** soft paths exactly `/Game/Meshes/SM_Castle_GateBanner` · `SM_Castle_TramplePath` · `SM_Castle_ToeRock01` · `SM_Castle_ToeRock02` (all four exist — the optional 02 is real; use it for line variety). Pivots §3. Banner faces +X; path tiles at 600; rocks overlap at ~200 spacing. All collisionless — your `SetCollisionEnabled(NoCollision)` is the second belt, keep it.
- **659:** commit carrier = §7's table + the generator + Cache evidence + this handoff + the manifest delta. Nothing here compiles; the compile expectation on the F1 chain comes from 661's Source diff alone. The four SM assets render textured in-editor (MI-bound at design time; slot-0-only runtime writes never touch these props — they are not on any `ApplyTeamVisuals` path).
