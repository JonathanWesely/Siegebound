# TASK-619 — [CR-2] THE FLOOR RE-TRACE GATE + THE WAVE COMMIT (build-master handoff)

**Status: COMPLETE — ALL THREE GATES PASS (identity · re-trace · nav); the wave commit is this file's carrier commit (CF-R1).** Date: 2026-08-17. Editor: relaunched THIS task under Jonathan's session grant ("go, and I give you permission to close and reopen the editor as needed") — the one batched bounce serving 619's fresh-asset load AND 622's post-ini boot. MCP `http://127.0.0.1:8000/mcp` green throughout. No console/`M`/`DumpAssistantPrompt` (CF-R3); no map save; TASKBOARD not edited (orchestrator's flip).

## 0. THE BOUNCE RECORD (the granted editor bounce, both tasks' precondition)

| step | measured |
|---|---|
| Pre-close editor | PID **4764** (started 2026-08-17 13:27:23, Jonathan's/618's session) |
| Pre-close dirtiness | **ZERO measured live**: `is_dirty` false on L_Arena + SM_Castle + Crumble01/02/03 + BP_Torch; PIE not running; level = `/Game/Maps/L_Arena` |
| `L_Arena.umap` SHA256 ENTRY (pre-close) | `B3DBC5D9AE484A7BD02CAFAD52B4681DA68B011477479B65EE7781AE459F8268` (= the canonical ledger value) |
| Close | graceful `CloseMainWindow`, exited in **4 s**, NO save modal (as the zero-dirtiness predicted) |
| `L_Arena.umap` SHA256 post-exit | `B3DBC5D9…F8268` — **UNCHANGED** |
| Relaunch | GUI `UnrealEditor.exe` + uproject, detached, new PID **14604** (17:49:43); MCP re-registered; fresh session booted straight onto `L_Arena` |
| Post-ini boot proof (622's precondition) | `r.DefaultFeature.LocalExposure.HighlightContrastScale` = **0.70**, `…ShadowContrastScale` = **0.65** (SearchCVars, live) — the D3 keys read at boot |
| `L_Arena.umap` SHA256 mid-occupancy (post-619 engine work, post-SIE) | `B3DBC5D9…F8268` — **UNCHANGED**, `is_dirty` false |

(The final exit hash of the whole occupancy lands in `handoffs/TASK-622-buildmaster.md` — same batched session, per the dispatch. The editor stays UP for TASK-625.)

## 1. IDENTITY GATE (the 612 pattern) — PASS, fresh load

Live `ObjectTools.get_properties` on `…:BodySetup_0`, all four meshes, full 61-hull comparison against `pipeline_manifest.json` `assets.Castle.ucx.boxes` (center xyz + size xyz per hull, rotation checked to zero):

| mesh | boxElems | other elems | trace flag | max deviation (any component, any hull) | max rotation dev | canonical invariance vs SM_Castle |
|---|---|---|---|---|---|---|
| SM_Castle | **61** | 0/0/0/0/0/0/0/0 | CTF_UseDefault | **0.000** | 0.000 | (reference) |
| SM_Castle_Crumble01 | **61** | all 0 | CTF_UseDefault | **0.000** | 0.000 | **IDENTICAL** |
| SM_Castle_Crumble02 | **61** | all 0 | CTF_UseDefault | **0.000** | 0.000 | **IDENTICAL** |
| SM_Castle_Crumble03 | **61** | all 0 | CTF_UseDefault | **0.000** | 0.000 | **IDENTICAL** |

`W6-R2` invariance: canonical serialisations (61 × [center xyz, size xyz, rotation pyr], 6 dp) string-identical across all four stages, on the FRESH load — the on-disk assets carry the 618 apply.

## 2. RE-TRACE (the TASK-598 instrument: complex down-trace + WorldStatic overlap bisection ±0.5) — BOTH CASTLES, ALL PASS

Castle_0 = Blue (−25000, 0, 0) · Castle_1 = Red (+25000, 0, 0). **Every column below measured IDENTICALLY at both castles to the hundredth** (one table serves both; support tops carry the instrument's ±0.5 bisection tolerance; the −1.0 probe-half-height correction is applied). occ50/150/300 = castle-hull occupancy at those z.

| column (local) | vis z | support top | §5 expected | verdict |
|---|---|---|---|---|
| (−2030, −1610) | 513.97 | **517.59** (Castle) | `skirt_toe_14` top 518, −4 float, sealed | ✅ |
| (2765, −2030) | 518.27 | **518.06** (Castle) | `skirt_toe_10` top 518.5, −0.2 | ✅ |
| east bay (3255, −1120) | 511.32 | **2429.62** + occ 50/150/300 all true | BLOCKED by `bay_seal_east_01` (top 2430) | ✅ |
| ⭐ chest seam (−1295, −1155) | 164.71 | **2429.62** + occ all true | BLOCKED by `bay_seal_west_01` | ✅ SEALED |
| ⭐ chest seam (1295, −1155) | 163.86 | **2429.62** + occ all true | BLOCKED by `bay_seal_east_01` | ✅ SEALED |
| ⭐ berm (−1600, −2900) | 100.47 | **152.44** (toe pokes above vis) | BLOCKED by `skirt_toe_13` (top 152.5) | ✅ CR-R2 discharged |
| ⭐ berm (1600, −2900) | 98.78 | **152.44** | BLOCKED by `skirt_toe_09` (top 152.5) | ✅ CR-R2 discharged |
| gate-mouth (−700, −2100) | 174.00 | **174.00** | `gate_plateau_01` 174/174 FLUSH (was Δ+58) | ✅ |
| gate-mouth (700, −2100) | 174.00 | **174.00** | `gate_plateau_02` FLUSH | ✅ |
| corridor flank (−315, −1120) | 174.00 | **144.94** | unchanged +29 (declared class, < 50) | ✅ |
| shoulder (−1225, −2450) | 299.75 | **299.62** | `shoulder_step_01` top 300 (−0.25) | ✅ |
| S6 control (0, −2900) | 85.82 | **72.28** | unchanged +13.3 (tread_04) | ✅ |
| HALL control (0, 700) | 174.00 | **174.00** | flush, unchanged | ✅ |
| F1 control (0, −1120) | 128.48 | **144.94** | −16.5 float, unchanged (polarity control) | ✅ |

Residual gate: worst measured residual on a reachable walkable column = **+29** (corridor flank, declared channel class) — all three declared tread classes < 50 = below `HeroMaxStepHeight`. Zero unexpected residuals, zero unexpected floats.

**Entry walk-trace y = −1890 (the sunk walk-in), both castles:** the old −2990/−2510/−2030 ground-0 walk-in band now reads **castle support ~517.6 (inside `skirt_toe_14`) — SEALED**. Open approach confirmed at local −3700 (knee-clear, ArenaGround only, Red-verified). ⭐ **Declared amendment to §5's face note:** the OUTERMOST blocking face at y −1890 is `skirt_toe_03`'s west face at local **−3657.5** (top 138.5, unmountable: 138.5 rise > 50), with §5's named `skirt_toe_14` face (top 518) behind it at −3552.5 — a STRICTER seal than predicted, not a violation (toe_03's y-span [−1929, 2170] covers the walk line; the §5 row named only toe_14). At BLUE both faces lie beyond ArenaBoundary_West — measured live: boundary actor present at local −2510 (z 400–600 occupancy), exactly the "boundary hit first" prediction. At RED: NO boundary at local −2510 (open, castle-only), `ArenaBoundary_East` present at local +2510 — the mirror fencing measured. Red east-bay outer ground identity: the apron actor (StaticMeshActor_33), not ArenaGround — 617 B1's amendment reproduced. Local −1460 reads inside `gatetower_west` (z 0–2430) — pre-existing hull, expected.

**Instrument note (declared):** on columns solid at the scan start (boundary wall at −2510; gatetower at −1460) the bisection's "support top" is an artifact (~818) because the upper bracket is never empty — occupancy probes, not the top number, carry those columns' verdicts. All open-column tops are genuine.

## 3. NAV REGRESSION (`NAV-§4`/`NAV-§12`) — PASS, settled-only, definitive

SIE session (bSimulate, in-viewport), reads at T+~15 s (`SIE-§1` honoured; zero decay lines — no crumble/match-over in the log). Seed 185734689, mirror=rot180, layers=7, corridorHalfY=1000. The navmesh regenerated over the NEW 61-hull collision and settled:

```
Traversability CONFIRMED (nav settled: 0 pending) — 0 tile task(s) pending;
Blue→Red castle path + 6 mine path(s) exist (after 0 cull(s)) [definitive: OnNavigationGenerationFinished]
```

Both the definitive (OnNavigationGenerationFinished) and the standard confirm lines fired. **0 pending tiles · Blue→Red path ✓ · 6/6 mine paths ✓ · 0 culls** — the bay seals + toe ring moved the navmesh ring without breaking a single guarantee. (MinesPass "culls=2" entries are that pass's own candidate retries, not nav culls; the NAV-§4 gate is the traversability line's 0.)

## 4. §25b LFS LANE (oid = worktree sha256; never size)

All four mesh uassets LFS-tracked (`filter: lfs` verified); worktree sha256 == the TASK-618 §6a record byte-for-byte:

| file | worktree SHA256 | == 618 §6a |
|---|---|---|
| SM_Castle.uasset | `28131FE6F0F1750420CE620D35676BE2D73000AFE1E2A1B22B61005B6103AAE4` | ✅ |
| SM_Castle_Crumble01.uasset | `A7E070250C15220CDE9059EC60C8599D29249FC461CC5619765950AD6818266D` | ✅ |
| SM_Castle_Crumble02.uasset | `E822C51CA2F25F335F57C0C179C18073FE08C894D20FEA8B69F00484E4F27D84` | ✅ |
| SM_Castle_Crumble03.uasset | `0867BC6D2B3402D6BAB6E9196CFF3492416EEAAC7722B4B556623EFF215A86D9` | ✅ |

(BP_Torch worktree `451924C9…7AA0` also verified == the 620 §3 record — banked for the 622 commit, NOT staged here.)

## 5. THE WAVE COMMIT (CF-R1 carrier)

HEAD verified `67b30ab` immediately pre-stage (nothing swept — the orchestrator's refutation of QA WARN-1 re-confirmed by my own `git status`: the worktree carried exactly the expected 11 modified + untracked pipeline files). Staged BY EXPLICIT PATHSPEC (the FLOOR lane + the diagnosis record): the 4 mesh uassets · `Tools/ArtPipeline/pipeline_manifest.json` · TASKBOARD.md · CONVENTIONS.md (the SIE-§1 diff) · `handoffs/TASK-613..619*` (incl. the 617 PNGs + the 615 grid CSV) · `playtest-evidence/2026-08-17/*` · `qa/TASK-620.md` · `qa/TASK-623.md`. ⛔ NOT staged (the 622/625 lanes): `Config/DefaultEngine.ini` · `Content/Blueprints/BP_Torch.uasset` · `BattlefieldScatter.cpp` · `Castle.h` · `handoffs/TASK-620-programmer.md` · `handoffs/TASK-623-programmer.md`. ⛔ No push. Commit hash in the orchestrator report + the Slack post.

## 6. FENCES / LEDGER

- **Writes this task:** this handoff + the wave commit. No asset touched, no save issued (all engine work read-only), no compile, no PIE-with-pawn (Simulate only, no input surface — the TASK-571+552 latch untouched by construction).
- `L_Arena`: hash-identical at entry, post-close, and mid-occupancy; `is_dirty` false at every checkpoint; no save prompt ever appeared.
- SIE stopped cleanly before commit work; editor left UP (PID 14604) on `L_Arena` for TASK-622 (same session) and TASK-625's later close under the same grant.
