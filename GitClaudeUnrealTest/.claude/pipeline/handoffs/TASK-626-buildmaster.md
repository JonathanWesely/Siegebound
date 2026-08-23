# TASK-626 — [GH-1] THE ENTRY RISER-SEQUENCE DIAGNOSIS — live, read-only (build-master handoff)

**Status: COMPLETE — the instrument ran; the verdict is the GH-R2 "mechanism is elsewhere" branch.** Date: 2026-08-17. Editor: Jonathan's live session (PID 12492 per dispatch), MCP `http://127.0.0.1:8000/mcp` green throughout. **STRICTLY READ-ONLY honoured:** no property write, no save, no PIE/SIE, no console/`M`/`DumpAssistantPrompt`, no pawn input, no compile, no git write, TASKBOARD not edited. `L_Arena.umap` SHA256 **ENTRY** `B3DBC5D9AE484A7BD02CAFAD52B4681DA68B011477479B65EE7781AE459F8268` = **EXIT** (re-hashed after all engine work) = the canonical ledger value. Laws: GH-R2 · CF-R3/R4 · the never-save law · the MCP tell-don't-fake law.

---

## 0. THE ONE-LINE RESULT

**There is NO blocking face on the legitimate entry route, at either castle.** The full riser-sequence audit (contiguous ≤35-uu samples, complex down-trace + live occupancy face probes, both castles, five lanes + the spawn-side approach routes) measures the centre entry chain **arena ground 0 → 29 → 43.5 → 58 → 72.5 → 87 → 101.5 → 116 → 130.5 → 145 → 174 (hall)** — ten risers, **worst riser 29.0 uu vs the measured hero MaxStepHeight 50** — hero-walkable end-to-end, capsule volume clear at every station, live hulls == manifest at every probed face. **Every face a straight-line or wall-hugging approach from the spawn hits IS a named >50 design seal** (table §4) — correct under the mount-proof law, and exactly what Jonathan's habitual pre-618 walk-in route now runs into. Per the 626 spec's explicit branch: **the mechanism is elsewhere — TASK-627 owes ZERO hull edits for the entry chain; STOP for re-spec.** The two live non-manifest candidates are named with evidence in §5.

## 1. MEASURED HERO CONFIG (the named instrument gap, closed — measured, not assumed)

| quantity | measured (live CDO reads) | where |
|---|---|---|
| `HeroMaxStepHeight` | **50.0** | C++ CDO `/Script/GitClaudeUnrealTest.Default__HeroCharacter` AND BP CDO `/Game/Blueprints/BP_HeroCharacter.Default__BP_HeroCharacter_C` — identical (no BP override) |
| `HeroWalkableFloorAngle` | **50.0°** | both CDOs (applied via `SetWalkableFloorAngle`, `HeroCharacter.cpp:869`; re-applied at BeginPlay) |
| Hero capsule | **radius 42 / half-height 96** (Ø84, height 192) | both CDOs, `CollisionCylinder` — ⚠️ **the board's "Ø68 / ~88" anchor is the UNIT/agent capsule, not the hero's.** The hero is bigger. No entry-route consequence (narrowest gap on the route is 1,260 uu); recorded so no future gate reuses the wrong capsule. |
| WalkSpeed / JumpZ | 500 / 600 | both CDOs |
| Castle mesh live | `SM_Castle` (pristine stage) both castles; castles at (∓25000, 0, 0) yaw 0 | live reads |

## 2. THE INSTRUMENT (TASK-598 lineage, upgraded to sequence)

Per route, per castle: complex down-trace (`trace_world`) every ≤35 uu for the visual profile; analytic support/identity from the shipped 61-hull manifest (exact for axis-aligned `KBoxElem`); **live occupancy probe-pairs at every riser face** (12×12×2-uu boxes, ObjectTypeQuery1–10: occupied at top−3..−1 AND empty at top+1.5..+3.5 — 28 faces probed, **28/28 perfect**, i.e. the live BodySetups still carry the manifest exactly, re-confirming 619 §1 at the faces that matter); **live capsule-volume overlaps** (84×84×184-uu standing boxes) every 105 uu along the centre lanes — **zero hits, both castles**. Scene census (native physics overlap, channels 1–10) over both approach corridors: **the only collision actors are ArenaGround, BackdropApron and the castle itself** — no rogue blocker exists in-editor.

## 3. THE RISER SEQUENCE — measured, identical at BOTH castles (local coords)

Routes (a) x=0 and (c) x=±315: **0 →(+29 @ y −3630)→ 29 →(+14.5 @ −3560)→ 43.5 →(+14.5 @ −3447)→ 58 →(+14.5 @ −3161.5)→ 72.5 →(+14.5 @ −2876.5)→ 87 →(+14.5 @ −2614)→ 101.5 →(+14.5 @ −2344.5)→ 116 →(+14.5 @ −2072.5)→ 130.5 →(+14.5 @ −1796.5)→ 145 →(+29 @ +270)→ 174.0 hall floor.** No riser > 29. No sunk column > 55 on the walk line. Capsule stations all clear (gate clearance 174→1530 lintel = 1356 ≥ 192 needed).

Routes (b) x=±700 (plateau lanes): same chain to tread_06, then **gate_plateau south face +72.5 @ y −2344.5 (> 50 — BY DESIGN)**; the intended side-mounts measured live: **tread_08 → plateau +43.5** (@ x ±630, y −1900) and **tread_09 → plateau +29** (@ x ±630, y −1500) — both ≤ 50 ✓ (the 618 §2 mount notes reproduce exactly). Notes: the 25-uu 174→145→174 dip strip at y 245..270 on the plateau lanes is narrower than the capsule (84) — bridged, non-issue; the plateau walk at |x| = 700, y −2225..−2120 reads visually buried (visual 303..354 over support 174 — the berm-knoll visual; cosmetic, lane-R territory); the |x|=700 lanes clip berm/spine faces by 4–22 uu (the practical plateau walk strip is |x| ≈ 630..678 beside the berms) — none of this touches the primary |x| < 630 channel.

## 4. THE BLOCKING-FACE TABLE (the spec deliverable) — identical both castles unless noted

| route | first blocker | hull key / face | face height over approach support | hero limit | verdict |
|---|---|---|---|---|---|
| (a) centre x=0, ground→hall | **NONE** | — | worst riser **29.0** | 50 | ✅ **WALKABLE end-to-end** |
| (c) flanks x=±315 | **NONE** | — | worst riser **29.0** | 50 | ✅ WALKABLE |
| (b) plateau lanes x=±700, from south | gate_plateau_01/02 | south face y = −2344.5, x ∓918..∓630 / ±630..±918 | **+72.5** (101.5→174) | 50 | ⛔ blocked from south — **BY DESIGN**; side-mount from tread_08/09 measured +43.5 / +29 ✓ |
| (d1) head-on from Blue spawn line (y=0, walking west→castle) | skirt_toe_01 | east face local x **+3657.5** | **+506.5** over ground 0 (bay_seal_east_01 z2430 behind it) | 50 | ⛔ **DESIGN SEAL** — the pre-618 de-facto "door"; this is the face a straight walk from spawn hits |
| (d1-Red mirror) head-on from arena centre (y=0, walking east→castle) | skirt_toe_02 | west face local x **−3657.5** | **+515.5** over ground 0 | 50 | ⛔ DESIGN SEAL |
| (d2) wall-hugging wrap y=−3650 | skirt_toe_07 (Blue E) / skirt_toe_11 (Red W mirror) | face at local x **±2047.5** / span to ±1470 | **+109** over ground 0 | 50 | ⛔ DESIGN SEAL (mount-proof); the wrap MUST swing south of y **−3692.5** — at y=−3720 the field is measured **completely clear** to the channel mouth |
| (d3) west band at y −1890 (the old Red walk-in) | skirt_toe_03 | west face −3657.5, top 138.5 (toe_14 518 behind at −3552.5) | **+138.5** | 50 | ⛔ DESIGN SEAL — GH-R8(ii) record stands, re-confirmed by probe |
| gate passage / corridor / hall thresholds | **NONE** | lintel 1530 headroom 1356 ≥ 192; pinch 1,470 wide ≥ Ø84 | — | — | ✅ clear |

**Mouth geometry:** the one legal entry is the south channel, opening x −1470..+1470 (2,940 uu) between skirt_toe_07/11, reachable from anywhere on the open field south of y −3692.5.

## 5. WHY JONATHAN "CANNOT WALK IN AT ALL" — the mechanism-elsewhere candidates, with evidence

1. **⭐ ROUTE-DISCOVERABILITY REGRESSION (fits every measurement).** The hero spawns EAST of Castle_Blue at ≈ local (+3957, 0, 100): L_Arena's only PlayerStart (−23800, 0, 100 = local +1200,0 — now INSIDE `bay_seal_east_01`) is **refused** by the `SiegeGameMode` WR-§2b resolver (verified live: castle colliding bounds x-extent 3,657 contains it) → castle-relative spawn at colliding-extent + 300 clearance. Pre-618 his straight westward walk from spawn passed through the hull-free east bay into the castle — **that was the habitual door**. Post-618 that exact line is `skirt_toe_01`'s +506.5 face, the wall-hug is +109, the west band +138.5: every route he has ever used reads "invisible wall". The ONLY entry is the south apron ramp → gate — walkable (§3) but reachable only by wrapping south of y −3692.5. That experience IS "I cannot walk into the castle at all". The seals are contract-correct (RB-1/RB-2); nothing here is a manifest defect.
2. **⭐ THE ENEMY-GATE TEAM BLOCKER (if the castle he pushed was RED).** `GateBlockerVolume` at PIE (BeginPlay-applied; editor-state read shows the constructor NoCollision stub, so this is code-pinned, not live-measured): local x −882..+918, y −1980..−1170, z **174..1530** — a full-aperture invisible seal across the gate, object type own-team channel, **blocks the enemy team channel only** (`Castle.cpp:404-416`). A Blue hero walking up Red's (perfectly walkable) treads is stopped dead at the gate mouth BY DESIGN. Own gate ignores him (`SiegeTeamObjectChannel` folds any non-Red team to Blue — even a mis-timed team read cannot invert this into an own-gate block).
3. **Ruled out by measurement:** boundary walls (south boundary at world y −12450, far off the approach; W/E boundaries at local ∓2450 fence only the far flank bands, 619's record reproduced) · rogue collision actors on the approach (census: none) · in-editor mines (none placed) · scatter rocks at PIE (`CastleKeepClearRadius` 4500 ≥ the farthest route point 3,974 from castle centre — the whole approach ring is keep-clear; caveat: the C++ default's DA override state is TASK-569's still-open verify) · spawn entombment (resolver refuses the sealed PlayerStart; fallback spawn ground is clear, capsule-checked) · crumble-stage mesh swap (both castles live on pristine `SM_Castle`; W6-R2 makes stages collision-identical anyway) · hero tunable regressions (§1: 50/50° confirmed on both CDOs).

**What read-only law prevented me from measuring:** the PIE-time blocker matrix and hero team typing in the act, and the actual walk. **The 30-second human disambiguation (for the orchestrator to route):** Jonathan walks at his OWN (Blue) castle via the south ramp — centre of the ramp, straight north through the gate. (i) If he gets in → mechanism was discoverability and/or he had been pushing the Red gate; (ii) if the OWN gate also stops him at the arch line (y ≈ −1980) → the blocker/team-typing is misbehaving at runtime → **gameplay-programmer lane, not a manifest task**; (iii) if he is stopped on the RAMP itself, the stop coordinate names the face — but §3/§4 measured none to find.

## 6. PRESCRIPTION FOR TASK-627 (the input it was blocked on)

**Zero hull edits are owed for the entry chain — the shipped 61-hull set is entry-correct as designed, and this handoff is NOT a license to trim any seal.** Specifically do NOT lower `skirt_toe_01/02/03/07/11` or the bay seals (re-opens the sealed-flank defect classes and violates the mount-proof law), and do NOT touch the plateau south faces (+72.5 is the designed anti-frontal-mount; the legal mounts measured 43.5/29). If the orchestrator still wants a collision-side kindness for discoverability, the ONLY edit compatible with the seals' contract would be widening nothing — the channel is already 2,940 wide — so there is none; the discoverability fix is visual/UX (lane R: make the south ramp READ as the door — 629's doorway directive already owns exactly this) and/or the Red-gate expectation is a design fact to surface to Jonathan. Recommend: 627 re-specs to no-op/closes against this finding, or re-targets to whatever branch §5's human check selects. Cosmetic items for lane R's ledger (non-blocking): the |x|=700 plateau visual-bury strip (visual −180 vs support, y −2225..−2120) and the hall-threshold float (~45 uu, y 260..310 — known F-class).

## 7. FENCES / LEDGER

- **Writes this task: this handoff file. Nothing else** — no asset, no board, no config; `git status` carries only the orchestrator's pre-existing TASKBOARD edit besides this file.
- `L_Arena.umap` hash entry == exit == ledger (`B3DBC5D9…F8268`); no save issued or prompted; all engine work was queries (traces, overlap probes, property/label/bounds reads).
- Instrument counts: ~1,450 down-traces · 28 face probe-pairs (28/28 perfect) · ~100 capsule-volume overlaps (0 unexpected hits) · 2 corridor censuses · CDO/component/bounds reads. Editor left exactly as found, Jonathan's hands never contended.
