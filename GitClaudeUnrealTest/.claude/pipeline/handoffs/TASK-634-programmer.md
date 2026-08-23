# TASK-634 — [GH-9] THE CODE-SIDE ANCHOR AUDIT + THE HOMED RIDERS (gameplay-programmer handoff)

Date: 2026-08-18 · Status requested: ready-for-qa (board flip left to the orchestrator per dispatch — I did not edit TASKBOARD.md) · **ZERO compile (QUIET-MODULE — the diff sits unbuilt until TASK-636, G4)** · no editor · no git · no console/`M`/DumpAssistantPrompt.

Input of record: `handoffs/TASK-629-artist.md` (the as-built readback, §2/§3/§5). Law: GH-R7/R8 · W8-R1 · the trailing-defaulted-parameter law · the compile-trap restatements.

## Files touched (complete list)

| file | nature |
|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/Castle.cpp` | 3 constant re-derives, 2 constant deletions, 2 anchor-expression re-bases (1 value-changing, 1 value-preserving), comment corrections at every touched site |
| `Source/GitClaudeUnrealTest/Siegebound/ScatterConfig.h` | the GH-R8(i) rider — **comment-only, zero behaviour** (lines in the `bBlocking` doc block) |

`Castle.h`, `CommanderNpc.{h,cpp}`, `Torch.{h,cpp}`: **zero edits** (audited read-only; see the verdict table and §5).

## 1. THE AUDIT TABLE — every GH-R7 constant, per-constant verdict

Every "629" citation below is a MEASURED number from `handoffs/TASK-629-artist.md`, not the spec projection.

| constant (site) | old value | new geometry fact (629) | verdict | new value + derivation |
|---|---|---|---|---|
| `InteriorFloorZ` (`Castle.cpp` anon-ns) | 174.0 | floor measured flat 174.00–174.69 end-to-end (1,612 probes, §4) | **HOLDS** | — (comment annotated) |
| `HallClearHeightZ` | 1560.0 | as-built clear 1986 (ceiling flat 2160, §2) | **HOLDS** (as the WR-§1 design MINIMUM; 1986 ≥ 1560) | — (comment now states it is the design minimum, not an as-built readback) |
| `TorchWallMountZ` | 174 + 0.5×1560 = **954** | "mount z 954 has flat wall everywhere (walls are vertical 174→2160)" (§3); corridor spring z 1410 unchanged (§2) | **HOLDS** (GH-R7 design pin, 629-verified) | — (954 is now below the as-built mid-height 1167 — declared as FEEL, Jonathan's TASK-571 tunable, not a derivation error) |
| `HallMinX` / `HallMaxX` | −1920 / 990 | hall x −1920 … 990 (§2 — X span retained) | **HOLDS** | — (`HallMaxX` is now ALSO the east WALL plane — consumed by re-derived anchor 5) |
| `HallMinY` | 270 | south wall plane measured **y 240** (§2) | **RE-DERIVED** | **240** (the measured plane, verbatim) |
| `HallMaxY` | 990 | north wall plane measured **y 1380** (§2 — the y-990 wall is GONE) | **RE-DERIVED** | **1380** (the measured plane, verbatim) |
| `HallCentreX` (derived) | −465 | X span unchanged | **HOLDS** | — (0.5×(−1920+990) = −465, unchanged) |
| `HallCentreY` (derived) | 630 | follows the Y re-derive | follows | **810** = 0.5×(240+1380) — exact in float |
| `HallThirdX` (derived) | 970 | X span unchanged | **HOLDS** | — (2910/3 = 970, unchanged) |
| `AnnexMaxX` / `AnnexCentreY` | 1680 / 780 | the annex void was FILLED SOLID (§5.3, H1 one-volume default) | **DELETED** (the constants' referent no longer exists; the sole consumer — torch anchor 5 — re-derived) | — |
| `CorridorWestWallX` | −732 | corridor wall planes measured **x −735 / +771** (§2 — old arch grown +3/side, §5.2) | **RE-DERIVED** | **−735** (the measured west plane, verbatim) |
| `CorridorCentreY` | −315 | corridor span y −1140 … +510 UNCHANGED (§2) | **HOLDS** | — (0.5×(−1140+510) = −315) |
| **Torch anchors 1–3** (north wall) | (−1435/−465/+505, **990**, 954) yaw −90 | all six anchors measured OFF-WALL (§3: anchors 1–3 are 390 uu from any wall — the y-990 wall died); suggested sites: north wall y 1380 at x −1435/−465/+505 yaw −90 | **RE-DERIVED** | (−1435/−465/+505, **1380**, 954) — the SAME third-centre expressions (`HallMinX + k·HallThirdX`, k = 0.5/1.5/2.5) reproduce 629's suggested x's exactly because the X span held; only `HallMaxY` moved |
| **Torch anchor 4** (south wall) | (−1435, **270**, 954) yaw +90 | §3: 46.8 uu off-wall (wall moved 270→240); suggested: south wall y 240 at x −1435 yaw +90 | **RE-DERIVED** | (−1435, **240**, 954) — same expression, `HallMinY` moved. Still the ONLY surviving third-centre on that wall: the corridor punch-through is now x −735 … +771 (swallows −465 and +505) |
| **Torch anchor 5** (was annex east wall) | (**+1680, 780**, 954) yaw 180 | §3: 60 uu off-wall → then the annex FILLED (§5.3) — the old anchor is inside masonry; suggested: east wall x 990 at (990, 810) yaw 180 | **RE-DERIVED** (site moved, not nudged) | (**990, 810**, 954) yaw 180 = `(HallMaxX, HallCentreY, TorchWallMountZ)` — 629 §3's suggested site verbatim, wall verified planar at z 954 |
| **Torch anchor 6** (corridor west wall) | (**−732**, −315, 954) yaw 0 | §3: 41.4 uu off-wall (wall −732 → −735); suggested: corridor west wall x −735 at (−735, −315) yaw 0 | **RE-DERIVED** | (**−735**, −315, 954) — `CorridorWestWallX` moved; y/z expressions unchanged. Mirrored-candidate comment updated to (+771, −315, 954) |
| `MaxTorchesPerCastle` (`Castle.h:538`) | 6 | 6 anchors in the re-derived set (629 suggested exactly 6 sites) | **HOLDS** | — (WR-§4 cap law; untouched) |
| **`CommanderNpcAnchor`** (`Castle.cpp:313` region) | value (−465, 810, 174) yaw −90; expression `(HallCentreX, 0.5×(HallCentreY + HallMaxY), InteriorFloorZ)` | §3: anchor interior, floor 174 under it, **min horizontal clearance 570** (24 dirs × z 300/600/954) ✅ ≥ 300; war table at +200 → y ≈ 610, 370 clear | **VALUE HOLDS — EXPRESSION RE-BASED** (⚠️ the load-bearing subtlety of this whole audit): under the new bounds the OLD expression would compute 0.5×(810+1380) = **1095** — silently moving him 285 uu north. Re-based to `(HallCentreX, HallCentreY, InteriorFloorZ)`, which under the new bounds = (−465, **810**, 174), byte-identical to the shipped transform. Zero behaviour change, proven by arithmetic in the code comment | robustness window re-derived: table stays north of the corridor mouth (y 510) for offsets < 300 (unchanged); stays inside the hall (y > 240) for offsets < **570** (was 540 vs the old y-270 wall) |
| `CommanderWarTableForwardOffset` (`CommanderNpc.cpp:96`) | 200 | table lands y ≈ 610: 370 clear of the south wall, 100 north of the corridor mouth (§3) | **HOLDS** | — (file read-only per spec; no edit) |
| `GetInteriorAnchorLocation` (`Castle.cpp:1056`) + `InteriorAnchorRelativeLocation` ZeroVector (`Castle.h:498`) | actor-transform of (0,0,0) | §3: local (0,0) interior — floor 174.0 below, ceiling 2162 above ✅ | **HOLDS** | — (the value is a navmesh DESTINATION; the mover projects onto the floor poly — the shipped rationale survives verbatim) |
| **`GateBlockerExtent` (900, 405, 678)** / **`GateBlockerRelativeLocation` (18, −1575, 852)** (`Castle.h:409/464`) | as shipped | §3: "gate aperture reproduced as-built; the blocker still spans it ✅"; §4: z300 open span −672…668 **identical to pre-redesign**; §5.2: arch grown ≤ +3/side, jamb planes −885/+921, "clearance arguments still cite TASK-555's collision numbers — unchanged" | **HOLDS — NO DIFF** (the spec's "verify no diff needed" case, verified) | Cross-checks performed, all pass: X span −882…+918 still ⊇ the retained collision gap −762…+798 (GH-R7) and still embeds in jamb hull space; Y span −1980…−1170 ⊂ the tunnel (arch cutter y −2100…−1020); Z band [174, 1530] unchanged. ⚠️ ONE NEW FACT, DECLARED NOT DIFFED: 629's threshold lowers the local floor to 148/161 under part of the blocker's Y band (landings, y mouth…−1460, x −462…498), leaving a ≤ 26-uu slit below the blocker's z-174 bottom. No capsule on the field fits it (hero r42/hh96 — GH-R9's measured Ø84×192; every unit capsule is far taller than 26), so the gate contract holds. Flagged for 636's live hall-hollowness/entry walk anyway |
| `InteriorNavModifier` (bounds-derived, `Castle.h:334`) | — | §4: mesh bounds 7313.576 × 7384.367 × 8082.610 and min-Z **byte-equal Δ0** | **HOLDS — follows by construction** | the modifier's marked volume derives from the owner-attached component bounds, which did not move; area semantics untouched |
| `SpawnBoxHalfExtent` (7380, 7380) (`Castle.h:377`) | — | bounds Δ0 | **HOLDS** | — (derived from full castle width, unchanged) |
| HP-bar Z 9450 (`Castle.cpp:174`) + `CastleDamageNumberHeightZ` 9450 (`Castle.cpp:50`) | — | mesh height 8082.610 Δ0 | **HOLDS** (the paired transcription moves together or not at all — not at all here) | — |

**Independent-enumeration note for QA (SC-§34):** my sweep for further interior-coordinate constants (`annex|corridor|hall|954|174\.|1530|1575|InteriorAnchor|TorchAnchors|CommanderNpcAnchor` over `Source/`) found NO code-side interior coordinate outside `Castle.{h,cpp}` — `BattlefieldScatter`/`ScatterConfig` "corridor" is the ARENA scatter corridor (unrelated); `MinerUnit`/`SummonedUnit` consume `GetInteriorAnchorLocation()` as a function, baking no coordinate; **no test in `Source/.../Tests/` pins any touched value** (suite expectation stays 118 for 636).

## 2. THE POOL-COVERAGE MATH (the 616 method at the LIVE values — QA re-computes per spec (c))

Inputs (all cited, none invented): live tuning surface = **BP_Torch class defaults, TASK-620 post-save readback: 90 cd / attenuation 1900**; live light centre = anchor + `TorchLightRelativeOffset` (50, 0, 75) (TASK-617 amendment) ⇒ 50 uu into the room along facing, **z 1029 = 855 above the 174 floor**.

- Floor-pool radius: √(1900² − 855²) = √(3,610,000 − 731,025) = √2,878,975 ≈ **1696.7 uu** (620 §4's own figure, reproduced).
- Nadir illuminance: 90 / 8.55² ≈ **1.23 lux** (620 §4) — the 622 pixel pass is the feel baseline; this task gates GEOMETRY, not feel.
- **Hall (2910 × 1140, x −1920…990, y 240…1380):** anchors A1(−1435,1380) A2(−465,1380) A3(505,1380) A4(−1435,240) A5(990,810). Worst point = the south-edge equidistant point of A4/A2 at (x ≈ −280, y 240): nearest-anchor distance ≈ **1,155 uu** (A5 there ≈ 1,392). Every hall point therefore sits ≤ ~1,240 uu (the code comment's conservative bound) from an anchor, deep inside the 1,697 pool ⇒ **100% floor coverage with continuous overlap**; the full 1,140-uu depth is spanned from the walls (1,697 > 1,140).
- **Corridor (x −735…771, y −1140…510):** single pool at light centre (−685, −315). Far corners (771, ±extreme y): √(1456² + 825²) ≈ **1,673.5 ≤ 1,696.7** ⇒ the corridor is covered corner-to-corner AT CUTOFF for the first time (the old 616 §2 east-half hole closes; margin ≈ 23 uu, brightness out there is falloff + hall spill — the mirrored 7th anchor stays the flagged FEEL addition, unchanged).
- **Declared residual (not a regression):** the threshold's east mouth corner (498, −1780) is ≈ 1,883 from the corridor light — outside cutoff, exactly as it was pre-redesign; it is lit by spill and by daylight through the mouth.

## 3. THE TRAILING-DEFAULTED-PARAMETER LAW — discharged

**ZERO function/method signatures changed** in this diff (anonymous-namespace `constexpr` values, constructor-authored defaults, and comments only). No parameter added, removed, defaulted, or re-ordered ⇒ **no call-site audit is owed**; stated per the law rather than skipped silently.

## 4. M8 DECLARATION — replication impact of every touched symbol

- All touched values feed `SpawnCastleFurnishings()` exclusively: `TorchAnchors` / `CommanderNpcAnchor` / the anon-ns constants spawn **ATorch / ACommanderNpc — NET RELEVANCY TIER C, not replicated** (their own headers declare it). The furnishing BeginPlay runs on the server AND every client BY DESIGN (no authority guard — the Castle.h class doc's ruled posture, untouched); both machines compile the SAME C++ defaults into their CDOs, so both build byte-identical local sets. **Anchors are server-spawn-side AND client-spawn-side cosmetics — verified: no replicated property, no RPC, no authority path, no `GetLifetimeReplicatedProps` entry is touched by this diff.**
- `GateBlockerExtent`/`GateBlockerRelativeLocation` (the one replication-adjacent surface via team gating): **NOT modified** (HOLDS row).
- `ScatterConfig.h`: comment-only — generate-time scatter config, zero runtime/replication surface.
- `W8-R1` honored: `ACommanderNpc` remains `AActor`; nothing in this diff touches its class shape.

## 5. RIDERS — done, and found-but-declared (NOT touched)

1. **DONE — `ScatterConfig.h` `bBlocking` doc (old :113-115):** the falsified "non-blocking GRASS layer ignores keep-clear (lush everywhere)" corrected to the TASK-623 disc-test truth (non-blocking layers — grass AND plants — honor the keep-clear DISCS via `IsInKeepClearDiscs`, footprint-inflated, but NOT the reserved corridor band; the lane stays lush). Comment-only, zero behaviour, cites qa/TASK-623.md rider 1. (GH-R8(i), the TASK-596 precedent.)
2. **DECLARED, not touched — `Torch.h:180-189`:** the `TorchAttenuationRadius` doc still says "the grand hall is ≈2910 × 720 uu with ≈1560 uu clear" (now 2910 × 1140 / 1986 as-built) and "1200 by law" (the C++ default — the LIVE surface is BP_Torch's 1900 since TASK-620). **Out of my fence** (the names block scopes me to `Castle.{h,cpp}` + `CommanderNpc` read + `ScatterConfig.h:113-115`) — the exact posture qa/TASK-623.md took with ScatterConfig.h. Manager: board a comment rider on the next task owning `Torch.h`.
3. **DECLARED, not touched — `Castle.cpp:~1058/·~549` "Castle_Red is placed at yaw 180":** stale by the TASK-617 C1 MEASUREMENT (both castles yaw 0), not by 629's geometry; TASK-623's rider corrected only the `Castle.h` copy (its fence). Not a 634 matter (my mandate is geometry-invalidated text); candidate for the same future rider batch.
4. **DECLARED, not touched — `Castle.h:474`** "local (0,0) is the interior floor's CENTRE in XY": pre-existing imprecision (the hall's centre is (−465, 810)); the doc's load-bearing claim — (0,0) lands in the open hall and MoveToLocation projects to the floor — was re-VERIFIED true by 629 §3. Not newly falsified, left as is.

## 6. WHAT QA SHOULD SCRUTINIZE

- **The commander expression re-base** (the one value-preserving edit): confirm `(HallCentreX, HallCentreY, InteriorFloorZ)` = (−465, 810, 174) exactly under `HallMinY 240 / HallMaxY 1380`, and that the OLD expression would indeed have drifted to 1095 (the defect this diff prevents).
- **Anchor 5's site change** is the only anchor that MOVED site rather than tracking a wall plane — check it against 629 §3's suggested-site line verbatim.
- The float exactness claims: 0.5×(240+1380) = 810, 2910/3 = 970, −1920 + 2.5×970 = 505 — all representable, no drift.
- §2's coverage math per spec (c) — recompute; the corridor-corner margin is only ≈ 23 uu, so the arithmetic deserves the check.
- Compile-trap restatements: no `*/` inside any new doc text (all new block comments verified), no new `Printf`, no new includes/symbols/shadowing — the diff introduces NO new code, only values and comments.
- QUIET-MODULE: no build artifact exists; nothing under `Binaries/`/`Intermediate/` was produced by this task.

---

## 7. ADDENDUM (2026-08-18, post-QA, orchestrator-routed) — THE SLOT-COUNT AUDIT (TASK-630 §3's flagged code dependency)

**Trigger:** TASK-630 appended material slot 2 `CastleInteriorPBR` — the castle slot table goes `[TeamRegion, CastlePBR]` → `[TeamRegion, CastlePBR, CastleInteriorPBR]` at TASK-633's import (630 §3/§7). 630 verified at source that both `Castle.cpp` runtime writes are slot-0-only and that the old board claim "ApplyCrumbleStage writes both slots by index" does NOT match the code; this addendum is the module-wide audit it assigned to 634.

**Sweep instrument (QA re-runs it):** `GetNumMaterials|GetMaterials|StaticMaterials|SetMaterialByName|GetMaterialIndex|GetNumSections` and `SetMaterial\(|GetMaterial\(|CreateDynamicMaterialInstance|CreateAndSetMaterialInstanceDynamic` over all of `Source/`, plus a `slot|material` sweep of `Source/.../Tests/` and the two `Tools/reimport_*.py` import scripts.

### 7.1 Per-site verdict table — every enumeration/index/count site in the module

| site | what it does | castle-relevant? | verdict vs 3 slots |
|---|---|---|---|
| `Castle.cpp` `ApplyTeamVisuals` (`SetMaterial(0, …)`) | team recolor | YES | **SAFE — slot 0 only** (630's own source check, re-confirmed). ⚠️ Its comment claimed "SM_Castle has a single material slot (TASK-013 spec)" — stale since the two-slot TeamRegion split, falsified again by slot 3 → **corrected (comment-only)** and the 3-slot contract recorded there |
| `Castle.cpp` `ApplyCrumbleStage` (`SetMaterial(0, …)`) | crumble MI swap | YES | **SAFE — slot 0 only.** Slots ≥ 1 on every stage render the SAVED asset's design-time bindings (630 §7.4 puts that duty on TASK-632). **The 3-slot contract is now RECORDED beside the write** (+ a pointer in the `Castle.h` `ApplyCrumbleStage` doc) — 630 §3's named duty, discharged |
| `SiegeHitFlashComponent.cpp:83/102` (`SetOverlayMaterial`) | §6 hit flash on the castle (and units) | YES | **SAFE by construction — slot-agnostic**: the engine overlay renders across ALL sections whatever the count; base slots never touched (its own doc already states this). A 3rd slot simply flashes too — correct |
| `SiegePlayerController.cpp:2130-2133` (ghost `GetNumMaterials` loop) | placement-ghost material override | NO (ghost meshes are `SM_<CardID>` — the castle is not a card) | **SAFE twice over**: not reachable for the castle, and count-derived anyway (self-adapts to any slot count) |
| `BattlefieldScatter.cpp:937-940 / 1069-1072` (`GetNumMaterials` loops) | scatter donor overrides | NO (scatter donors only) | SAFE — count-derived |
| `Building.cpp:172` · `SummonedUnit.cpp:335` · `Projectile.cpp:177` · `GoldNode.cpp:453` (`SetMaterial(0/…)` / MID on 0) | other actors' slot-0 writes | NO (different meshes) | SAFE — no castle reference |
| `Torch.h:74/239-240` slot-1 flame contract | SM_Torch's OWN 2-slot layout | NO (torch mesh, not castle) | SAFE — unrelated slot table, untouched by 630 |
| **Tests** (`Source/.../Tests/*`) | — | — | **ZERO material-slot assertions on castle/crumble** — every "slot" hit is a SaveGame slot (`SiegeAccountTest`) or a hand slot (`SiegePlayerController` docs). Suite expectation stays 118 |
| `Tools/reimport_apply_materials_mcp.py:83-90` (WARNs "expected 2 slots", sets slots 0/1 only) + `reimport_meshes.py` (same lane) | the TASK-086/087/151 UNIT-FLEET import tooling (`SM_<CardID>`) | not castle-lane (and GH-R6 already bans `reimport_meshes.py::main()` on the crumbles) | **NO C++ diff owed — but FLAGGED to the orchestrator for TASK-633**: if 633 reuses this script pattern on the castle it would under-bind slot 2 and WARN on 3 slots; 633's own contract (630 §7.3: bind slot 2 → `MI_Castle_Interior_PBR`, read back all three by name+index) is the authority — reported so it is checked, not discovered |

### 7.2 The diff (comment-only, zero behaviour — flagged for QA pass-through)

| file | edit |
|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/Castle.cpp` (`ApplyTeamVisuals`) | stale "single material slot" claim corrected; slot contract recorded (comment-only) |
| `Source/GitClaudeUnrealTest/Siegebound/Castle.cpp` (`ApplyCrumbleStage`) | the 3-slot contract recorded beside the write, incl. the correction of the board's falsified "writes both slots" line (comment-only) — 630 §3's named duty |
| `Source/GitClaudeUnrealTest/Siegebound/Castle.h` (`ApplyCrumbleStage` doc) | 4-line slot-contract pointer (comment-only) |

Zero functional bytes changed: no expression, value, signature, or UPROPERTY touched by this addendum — `SetMaterial(0, …)` at both sites is character-identical. No compile run (QUIET-MODULE holds; 636 owns the wave's one). M8: comments only — the §4 declaration above is unchanged.

### 7.3 Verdict

**No functional diff is needed for the 3-slot mesh** — the runtime is slot-0-only everywhere it touches the castle, the one whole-mesh path (hit-flash overlay) is slot-agnostic, every count-derived loop self-adapts, and no test pins a count. The comment-only recording diff above is 630 §3's explicitly assigned duty; since TASK-634 already carries a PASS QA report, these three comment sites go back through TASK-635's reviewer before 636 compiles (orchestrator-routed).
