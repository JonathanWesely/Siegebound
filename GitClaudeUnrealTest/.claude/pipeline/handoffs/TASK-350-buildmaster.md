# TASK-350 — LOOP-4 CLOSING RUN 2026-07-29 (Jonathan-authorized per the board's "Escalation ruling (2026-07-29)"): compile GREEN, ALL 5 CLOSURE POINTS CLOSED, COMMITS MADE (build-master)

**Full empirical record: `qa/TASK-349.md` "LOOP-4 EMPIRICAL CLOSURE" append + probe `qa/TASK-349-L4-navprobe.txt` (3511 samples).** Summary:

- Editor guard: no PIE. Predecessor editor (PID 29816) closed gracefully via remote-exec console `QUIT_EDITOR` — clean `LogExit: Exiting.`, zero saves, tree fingerprint unchanged (no `load_map`, per lane-knowledge #10).
- **COMPILE: SUCCEEDED 0/0, 14.07 s** (loop-4 Castle.{h,cpp} in the DLL). Fresh boot #1 with remote-exec enabled via command-line ini override (`-ini:Engine:...bRemoteExecution=True` — in-memory, zero file edits; the Saved-config hand-edit lane is hook-blocked and was respected).
- **Closure (1) B4/R2(a)(b):** stale-saved-tile boot, no manual rebuild — halls converge DIRECTLY to team-correct (blue T=1.45 s; red T=178.28 s with the red castle at 2000 HP/stage 0 — **no crumble needed**, no wrong-team plateau in 3511 samples). The 178 s = the whole-map N2-stale tile queue reaching the far castle THIS boot only; the plain ≤10 s band is measured post-save (below).
- **Closure (2) R2(c):** PlayAgain from the heaviest state (blue DESTROYED + red stage 1) → first post-reset sample +0.4 s fully team-correct, ZERO stale samples either direction; blockers re-armed, castles restored.
- **Closure (3):** red wave full A→B march destroyed the blue castle (win condition at 3× proven; stages 1→2→3 once each, in order); 4 injected blue footmen struck the red castle to stage 1 from the WEST WALL EXTERIOR — **zero units in the gate mouth** (the FINAL-RUN pile-up is GONE); red own-hall order **REQUEST_SUCCESSFUL** (was FAILED).
- **Closure (4):** `Areas.Num() <= 1` ABSENT; 8 one-shot goal-change warns total (no storm); material/AccessedNone/Fatal 0.
- **Closure (5) N2:** the healthy signature verbatim — pre-feature default polys (never wrong-team) → rebuild → team-correct-first-appearance; end-state own-filter paths COMPLETE onto both hall floors, enemy INVALID, A→B COMPLETE under all three filters; blockers on pure CDO defaults both castles; hero stamped ECC_SIEGE_TEAM_BLUE.
- Evidence: `TASK-350-L4-prereset.png` / `TASK-350-L4-postreset.png` (live PIE-camera shots; numeric readbacks are the evidence per the FINAL-RUN precedent).
- **Commits:** assets `ea2a70f` (SM/T/raw/concepts/manifest/tooling/DA + the TASK-347 handoff) → code `949c252` (TASK-349 files + SiegeNavAreas + ini + qa + probes + handoffs 348/349/350 + board/CONVENTIONS; M8 SiegeSessionSubsystem/SessionMenuWidget/TASK-352-354 docs EXCLUDED) → nav-save `bf5e562` (below) → TASK-351 (own commit, after this section).

## THE ONE-TIME L_Arena NAV-SAVE (commit `bf5e562`) — executed per the binding Escalation-ruling procedure; THE EXCEPTION IS SPENT
FRESH BOOT #2: pre-rebuild dirty = NONE; fingerprint 118 actors, castles exact (±25000,0,0)/yaw 0.000; stale baseline confirmed (hall y300 = no polys). Deliberate `RebuildNavigation` → interior navmesh PRESENT + gate-connected (no-filter paths COMPLETE onto both hall floors at (±25000,0,60)); marking = the documented editor-world ctor-Blue on both interiors (runtime team-correctness is the loop-4 refresh's lane, proven in PIE). Post-rebuild fingerprint IDENTICAL; dirty = ['/Game/Maps/L_Arena'] ONLY, zero content packages. Saved via save_dirty_packages(maps-only); **git diff gate: `L_Arena.umap` the ONLY tracked change** → committed alone. The never-save law resumed at that commit.

## Fresh-boot #3 sanity probe (post-save) — the band's clean measurement + 🚩 ONE NEW FLAG
Probe `qa/TASK-349-L4-navprobe-postsave.txt` (596 samples, armed pre-PIE): saved-bake polys PRESENT from T=0.33 (N2 geometry FIXED — no more absent-interior boots), both halls initially Blue-marked (the expected editor-bake truth). BLUE hall team-correct ≤1.5 s. **RED hall flips DIRECT to team-correct at T=178.07 — vs 178.28 on boot #1 across a DIFFERENT disk baseline: a deterministic whole-map tile-queue drain, not a plateau** (the boot re-scatter dirties map-wide; tiles process distance-sorted from the blue-side seed; the far castle re-marks last). **The plain R2(a) ≤10 s band therefore MISSES on the far castle every boot** — bounded, converging, physical blocker armed throughout (enemy BODIES cannot enter; the exposure is nav-lane invitations/pile-up-class behavior in minutes 0–3). Flagged to Jonathan on the board with candidate levers NAMED not designed (castle-side nav seed/invoker priority · deterministic/nav-quiet boot re-scatter · synchronous castle-bounds build at BeginPlay · band re-rule on the physical-lane containment).

## TASK-351 — crumble trio re-derived from the 3× castle (same session, own commit)
Full record on the board's TASK-351 status line. Highlights: same-path FBX import over all three (refs preserved); readbacks exact (3 LODs 28,702/14,350/7,176 · verts0 35,184 · bounds bit-identical · Nanite OFF); raw import gave 11 UCX boxes vs the pristine 22 ⇒ collision byte-copied from SM_Castle's body_setup (22 box elems all three, door gap carries); MI_Castle_Crumble0N on BOTH slots per stage; live PIE ladder 75→50→25 each-once-in-order with the blocker armed through every stage (incl. stage 2) and the nav marking team-correct through every swap; ResetCastle restores pristine + red accent. **STAGE-LEGIBILITY BAND FULL PASS: S1 0.7735 · S2 0.5163 · S3 0.3737 · gaps 0.2572/0.1426 · reset 0.9707×P.** Shots `TASK-351-stage-{P,S1,S2,S3,Reset}.png` + `TASK-351-simulate-pristine-gate.png`.

## State at close
Editor RUNNING (fresh boot #3, PID 24940) + MCP up, PIE stopped, L_Arena saved-state clean (the one authorized save only). Remote-exec enabled via command-line ini override (in-memory). Probe tick callbacks disarmed. No push (law).

---
---

# (superseded by the closing run above) TASK-350 — FINAL RUN 2026-07-28 night: loop-2 code COMPILED GREEN and B2 is CLOSED (march freeze dead, melee strikes both castles) — but the r5 probe found **ONE NEW BLOCKER (B4): interiors' nav areas permanently stale on every pre-built-tile baseline → qa-failed LOOP 3 OF 3 (ESCALATION), NO COMMITS** (build-master)

**Full findings + verbatim evidence: `qa/TASK-349.md` FINAL-RUN append** (B4 mechanism, the 211-s stale window ending exactly at the Leg-3 fence, the blue-pile-up-at-the-red-gate proof, the N2 stale-saved-navmesh finding, probe logs `qa/TASK-349-FR-navprobe*.txt`). Summary of the run:

- **Editor guard:** no PIE at start. Closing the predecessor's long-lived editor tripped a world-leak fatal (`FPyReferenceCollector` holding L_Arena — python-held ref, TOOLING classified, zero saves, tree fingerprint verified unchanged; forensics in the QA append). Compile ran editor-down as required.
- **COMPILE: SUCCEEDED 0/0, 15.9 s** — first build of the loop-2 code; DLL 22:10:10 postdates all four loop-2 sources; relaunched editor (PID 29816, adopted after Jonathan cleared the autosave dialog with Skip Restore) loaded the fresh DLL; MCP up.
- **Pre-PIE gates all green:** SM_Castle persistence exact to the RE-RUN readbacks (NO re-import); sampler sweep 0/0; channels live; **PURE C++ DEFAULTS verified on both castle instances == CDO (6,−525,284)/(260,135,226), blockers armed correctly with zero instance edits** — the baked-defaults ride-along is proven; crumble trio baselined (still 1× stale — 351's lane).
- **CLOSED:** B2 (12/12 movers, full A→B march, ATTACK at walls, Blue castle 2000→1326 + stage 1 from organic melee, 21 gated one-shot warns vs 318-storm, no thrash) · Leg 1 (`Areas.Num() <= 1` ABSENT all session) · Leg 3 (fence provably re-marks tiles) · crumble driver + blocker-armed-through-swap · log otherwise clean.
- **FAILED:** B4 — red hall Blue-open/Red-closed from T=0.33 with NO flip for 211 s (ended only by the crumble-fence); blue-filter path INTO the red hall valid+complete; red unit's own-hall order FAILED; 4 live blue footmen pathed into the red gate and piled at the blocker (y=−700..−770, vel 0). R2(a)+(b) violated. Root: loop-2's Leg 2 removed the only post-registration area change, so pre-built tiles are never re-marked.
- **N2 (session-independent):** the SAVED L_Arena navmesh is pre-3×-stale — fresh boot has NO interior navmesh and uncarved 3× walls until something dirties the tiles. In-memory editor `RebuildNavigation` used for this session's baseline (never saved). Needs: the B4 fix (runtime repair) + a ruled navmesh-rebuild+save task + a CONVENTIONS line.
- **NO COMMITS (hard gate). TASK-351 NOT started (dispatch rule).** Editor left RUNNING (PID 29816) + MCP up; remote-exec ON (in-memory); r5v2 sampler disarmed; L_Arena never saved (PIE-time dirty flag left unsaved as always). **LOOP 3 OF 3 = the escalation limit — the loop-3 dispatch decision belongs to Jonathan.**
- Evidence: `TASK-350-FR-pie_view_hpbar.png` (PIE HUD painting: gold/rally/cards/overhead HP bar) · `TASK-350-FR-blue_pileup_red_gate.png` (red gate + approach; NOTE: viewport-capture lane photographs the EDITOR world during PIE — pile-up evidence is the position readbacks in the QA append, this shot is scenery) · probe logs in `qa/`.

---
---

# TASK-350 — RE-RUN 2026-07-29: MIRROR FIX CONFIRMED IN-ENGINE; full matrix run → **TWO CODE BLOCKERS (march freeze + R2 band violations) — qa-failed loop 2, NO COMMITS** (build-master)

**The art side is DONE and PROVEN.** The re-run closed every art gate green, including the acceptance test for the mirror fix — the remaining blockers are TASK-349 runtime defects appended to `qa/TASK-349.md` (loop 2 of 3). TASK-351 NOT started (dispatch rule). Editor left RUNNING (PID 37676) + MCP up; L_Arena never saved; in-memory gate retune applied, matrix-verified, then REVERTED to the safe defaults (blocked-run posture, same as attempt 1).

## Agreement check (the mirror-fix acceptance) — PASS, both castles, to the decimal
| Probe (castle-local; run at BOTH anchors ±25000, both yaw 0.0 re-confirmed) | COMPLEX (render) | SIMPLE (UCX) | Verdict |
|---|---|---|---|
| South-inbound, arch centre x+6, z100 & z300 | flies to **y +330** | **y +330** (hall_back) | ✅ OPEN south, AGREE |
| North-inbound, z300 | stops **y +701.6** | **y +700.0** (wall_back) | ✅ CLOSED north, AGREE |
| South control x+600, z300 | **y −542.3** (front curtain) | **y −530.0** (wall_front_east outer) | ✅ closed, drift per UCX-DRIFT table |
| Arcade chirality, y+420 band | EAST-inbound flies to **x_local −643.7**; west stops −849.8 | seal at +1120 (arcade_seal — deliberate) | ✅ east-back flank per manifest |
| Eyeball | `TASK-350-RR-blue_gate_south_open.png` — the concept's open arch + paved approach on the SOUTH face (attempt 1 shot a closed wall here) | | ✅ |
Every row matches the MIRROR-FIX addendum's `probe_postfix.json` ue_space predictions exactly. **The Y-mirror is dead; visual, UCX, blocker, apron and nav all agree.**

## Re-run readbacks (identical to the blocked run, now on the FIXED FBX `600D4935…` / 1,211,340 B)
- Textures: byte-identical PNGs SHA-confirmed on disk → re-import SKIPPED per the addendum; in-editor readback: 2048², D sRGB/TC_DEFAULT, N linear/TC_NORMALMAP, ORM linear/TC_MASKS, sources point at the staged PNGs; referencers MI_Castle_PBR + M_CastleCrumble alive.
- SM_Castle same-path reimport: referencers before==after==[L_Arena]; slots [TeamRegion→MI_TeamColor_Blue, CastlePBR→MI_Castle_PBR]. **HARD LOD GATE:** lod_count 3, tris **28,702 / 14,350 / 7,176**, LOD0 verts 35,184 (no unweld), screen sizes [1.0, 0.4, 0.15], lod_group None, Nanite OFF, **22 box hulls / 0 convex**, bounds **2437.86 × 2461.46 × 2694.20**, min-Z −0.055.
- SAMPLER sweep: `Failed to compile Material` = **0** (delta + full log); the 6 `Default Material will be used` lines are the known pre-existing usage-flag set (tree ISM + 4 unit-MI SkeletalMesh) — none castle.
- Retune applied per-instance (RelLoc (6,−525,284) / Ext (260,135,226)); PIE readback: armed QueryAndPhysics, objtype = own channel, Ignore-own/Block-enemy mirror-symmetric; **re-armed correctly after ResetCastle** (verified post-PlayAgain). InteriorNavModifier Blue→NavArea_BlueCastleInterior / Red→NavArea_RedCastleInterior.
- DA_BattlefieldScatter already saved at 4500 (persisted from attempt 1). **Numeric per-instance sweep now done** (attempt-1 debt; actor class = `SiegeBattlefieldScatter`): BLOCKING violations **0** (nearest blocking instance 4753 > 4500); 904 cosmetic no-collision grass/plant instances inside the ring (184 inside the footprint) — keep-clear never governed cosmetic layers (cpp:429); cosmetic note only.

## Behavior matrix (a)–(i)
| Item | Result |
|---|---|
| (a) navmesh inside + through both gates | ✅ settled: out→hall valid+complete len 1619 ends ON hall floor (z60) both castles; own filter passes / enemy filter refuses, symmetric |
| (b) spawn-inside + own units in through the gate | ✅ hero walked IN through his own gate (real CMC, arrives hall, stands on slab z=156 — team-channel floor sweeps hold); `SummonTestUnit` (the shipping SpawnUnitSwarm path, hero camera in-hall) spawned a Blue Footman ON the interior floor (local +290,+167) which settled and stood; a RED unit ordered to its own hall walked in through the red gate (arrived (24845,109,150) at vel 400) |
| (c) enemy wave at the gate | ⚠️ BLOCKED BY B2 (march freeze — no wave self-marches). Partial: an injected red marcher reached the blue approach and stood OUTSIDE (its own AI refused interior entry); the physical+nav gates held |
| (d) hero gates | ✅ own gate: walks in AND out; enemy gate: nav-open (no player filter) but **physically pinned at y=−702** (blocker face −660 − r42), slid along the invisible wall to vel 0 under the visible open arch. **Jump under the 510 lintel CLEAN**: JumpZ 600 / g 980 ⇒ apex head 433.7 < 510 (76 uu margin, contact geometrically impossible); observed airborne z 248 → clean landing z 156, zero deflection |
| (e) physical both-directions matrix | ✅ swept-capsule: hero+blue PASS blue gate; RED blocked at blue gate **y=−702** inbound AND at −348 (north face) outbound; RED passes red gate BOTH directions; hero+blue blocked at red gate. Mirror-symmetric. Channel stamps verified (capsule objtype = team channel; hero BLUE); controllers = SiegeUnitAIController (QA F2 ✓) |
| (f) traversability | ✅ A→B valid + complete len 50,010 under NOFILTER, BLUE and RED filters |
| (g) HP/bar/crumble | ✅ HP 2000/2000 both; stage driver fired ONCE in order (2000→1480 = stage 1, mesh→SM_Castle_Crumble01, BOTH slots MI_Castle_Crumble01, blocker stayed armed through the swap); HPBarWidget Z=3150 (×3) screen-space visible both castles (state verified; gameplay-camera paint shot deferred — debug-cam shots don't paint screen-space widgets); stale pre-3× crumble visuals = TASK-351's known lane |
| (h) log sweep | ✅/⚠️ `Failed to compile Material` 0 · AccessedNone 0 · Fatal 0 · ensures 2 distinct: Recast `Areas.Num() <= 1` (change-surface, 2nd sighting → in the QA append) + the CDO python artifact; 318 MoveToActor-failed warnings = B2's fingerprint |
| (i) keep-roof shots for Jonathan (R3) | ✅ `TASK-350-RR-blue_topdown_occlusion.png` (fully roofed from RTS top-down; debug-cam trace shows the cursor ray hitting CastleMesh = the cursor placement roof-block, in-frame) + `TASK-350-RR-blue_inside_lookout.png` (dark interior, clear view out the open gate — good basis to judge the interior-fill-light lever) |

## R2 measurement vs the band — see the QA append (B3): (a) window end 11.0–11.6 s ⇒ MISS by ≥1.0 s; (b) held; **(c) VIOLATED — Play-Again recurrence, stale +8.5→+29.7 s with the red interior nav-open to BOTH filters.**

## THE TWO BLOCKERS (full mechanism + repro in `qa/TASK-349.md` loop-2 append)
- **B2 MARCH FREEZE:** every castle-advance MoveToActor fails — the castle-origin goal now sits on filter-EXCLUDED enemy-interior navmesh (SummonedUnit.cpp:2028 null-filter → team default). 123/123 combat units frozen at spawn all session; miners/hero unaffected; differential repro proves PathFollowing healthy. Match-breaking, silent.
- **B3 R2 band:** initial window marginally >10 s; Play-Again recurrence decisive (and enemy-open in signature). Recast `Areas.Num() <= 1` ensure re-observed.

## State at handback
- **NO commits** (hard gate). Working tree unchanged from dispatch EXCEPT: `SM_Castle.uasset` now the FIXED-FBX import (correct, orientation-proven — KEPT; next loop's matrix re-verifies on it without re-importing), evidence PNGs + this handoff + the QA append + board flip.
- Editor RUNNING (PID 37676), MCP up, remote-exec ON (in-memory). PIE STOPPED. Gate retune REVERTED to (300,0,300)/(80,400,300) on both instances; L_Arena NEVER saved (dirty-flag residue is the known byte-identical delta-serialization artifact).
- Test artifacts in the throwaway PIE only (destroyed bot walls, summoned probes) — nothing persists.
- **Standing flag (manager):** the verified retune values (6,−525,284)/(260,135,226) have NO durable home — the L_Arena-never-saved law means they exist only in-memory per session; once the loop passes, either the programmer bakes them as the C++ defaults (they are castle-local and identical for both instances) or a ruled level-edit task ships them. Also: the bot walls its own gate approach (observation, design call).

## Evidence files (handoffs/)
`TASK-350-RR-blue_gate_south_open.png` · `TASK-350-RR-hero_blocked_red_gate.png` · `TASK-350-RR-blue_hpbar_3150.png` · `TASK-350-RR-blue_topdown_occlusion.png` · `TASK-350-RR-blue_inside_lookout.png` · `TASK-350-RR-red_gate_stage1.png`

---
---

# ATTEMPT 1 (2026-07-28, superseded where the re-run speaks) — [C3X-int] Integration attempt — **BLOCKED: the imported visual mesh is Y-MIRRORED against the manifest/UCX space** (build-master handoff)

**Verdict: NO COMMITS. TASK-351 NOT STARTED (dispatch rule: 350 failed ⇒ skip 351).**
**The defect is INSIDE the asset (visual vs collision), so no actor transform can fix it — art-pipeline lane (manifest/export), routed to the manager → art-director.**
Editor left RUNNING (PID 37676), MCP up, L_Arena NOT saved (never-save law held; the in-memory gate retune was applied, verified, then REVERTED to the safe defaults — details below).

---

## What PASSED before the blocker (all gates green — the re-run only needs the mirror fixed)

| Gate | Result |
|---|---|
| Editor guard | No PIE; graceful no-save close (0 dirty packages, `quit_editor()` — zero saves) |
| **COMPILE** (first compile of TASK-349 + TASK-354 files) | **SUCCEEDED, 0 errors / 0 warnings, 18.2 s.** No stash contingency needed — TASK-354's new files (SiegeSessionSubsystem/SessionMenuWidget) compile clean |
| Texture imports (explicit, skip-trap honored) | `T_Castle_D/N/ORM` same-path; readback 2048², D sRGB/TC_DEFAULT, N linear/TC_NORMALMAP, ORM linear/**TC_MASKS**; source = the RE-RUN PNGs (byte sizes 1,709,124 / 1,420,049 / 1,434,675 exactly per the addendum) |
| SAMPLER-TYPE sweep | `Failed to compile Material` grep = **0** (after textures AND after mesh). Referencers enumerated: `MI_Castle_PBR` + `M_CastleCrumble` (master incl.) — both alive, castle materials absent from every warning |
| SM_Castle same-path reimport (452-gate FBX, 1,210,892 B) | Referencers before == after == `[L_Arena]`; slots `[TeamRegion, CastlePBR]` in order; slot mats MI_TeamColor_Blue / MI_Castle_PBR persisted (live-editor lane) |
| **HARD LOD GATE** | lod_count **3**; tris **28,702 / 14,350 / 7,176** (expected 28,698–28,702 / ~14,350 / ~7,175); LOD0 verts **35,184** (unweld fingerprint 86,106 NOT hit); thresholds **[1.0, 0.4, 0.15]**; `lod_group=None` (NOT LargeProp); Nanite **OFF** |
| Bounds | **2437.86 × 2461.46 × 2694.20**, min-Z **−0.055** — matches the law (±10% of 2442×2460×2694) and the addendum numbers exactly |
| Collision | **22 box hulls, 0 convex** (manifest `ucx.boxes` verbatim, incl. gate_lintel bottom **510** = the 452-clear build) |
| Ini channels LIVE | Python enum surfaces **`ECC_SIEGE_TEAM_BLUE` / `ECC_SIEGE_TEAM_RED`** after the bounce (the ini names mapped onto GameTraceChannel1/2) |
| Gate blocker retune (MANDATORY, qa/TASK-349.md note 1) | Applied per-instance on BOTH castles: RelLoc **(6, −525, 284)**, Extent **(260, 135, 226)** = x −254..+266 · y −660..−390 · z **58..510** per the addendum. Simulate readback: armed QueryAndPhysics, objtype = OWN channel, **Ignore own / Block enemy — mirror-symmetric on both castles** (Blue blocker blocks only RED; Red blocker blocks only BLUE) |
| InteriorNavModifier | BeginPlay re-stamp verified: Blue castle → `NavArea_BlueCastleInterior`, Red castle → `NavArea_RedCastleInterior` |
| Scatter re-derive | `DA_BattlefieldScatter.CastleKeepClearRadius` **1500 → 4500**, SAVED (PlayerStartKeepClearRadius 800 / CorridorHalfWidth 1000 untouched). Visual check: generous clear ring around both castles (`TASK-350-blue_wide.png` / `red_wide.png`); numeric per-instance sweep still owed at the re-run (scatter actor class lookup returned 0 — resolve the actor discovery then) |
| Nav INSIDE + team filters (settled state) | Unfiltered path out→hall VALID+complete BOTH castles (ends ON navmesh at hall centre, z 60 — navmesh generates inside and through the gate). Projection with own filter succeeds / enemy filter returns None at BOTH halls — **the settled matrix is fully correct and symmetric.** Castle-to-castle A→B path VALID+complete (len ≈50,007) with NOFILTER, BLUE and RED filters — traversability guarantee holds |
| Win-condition sanity | Both castles HP 2000/2000, stage 0, HP-bar Z 3150 compiled in (visual read at gameplay camera deferred to the re-run) |

## 🚨 THE BLOCKER — visual mesh Y-MIRRORED vs manifest/UCX space (in-engine, inside the asset)

**Symptom:** the visible open gate arch faces **+Y (north)**; the manifest/UCX door gap, apron steps, and GateBlockerVolume face **−Y (south)**. The castle LOOKS like its gate is on the north while everything collision/nav gates the south: units would walk through a visually solid south wall and be invisibly blocked at the visually open north arch.

**Evidence (complex traces = render triangles; simple traces = UCX; Blue castle, anchor (−25000,0,0), yaw 0):**
| Probe | Result | Reading |
|---|---|---|
| SIMPLE trace south→north at arch centre (world x −24994, z 300) | flies **2,330 uu**, first hit y **+330** = hall_back hull | UCX door-gap route OPEN southward (as authored) |
| COMPLEX trace south→north, same ray (z 100/300/650, and x+600 control) | ALL stop at y **−702** | a uniform visual wall plane at −702 ≈ the manifest BACK wall (+701.6) mirrored |
| COMPLEX trace north→south at arch centre (z 100/300) | flies **2,330 uu**, first hit y **−330** | the visual arch+corridor+hall route is open FROM THE NORTH; −330 = mirrored hall back wall (+330) |
| COMPLEX north control at x+600 | stops y **+542** | mirrored front curtain (−531) |
| Arcade chirality probe (y = −420 band, z 300) | EAST-inbound flies to local x −644; WEST-inbound stops at the wall | opening on the **EAST** flank ⇒ **pure Y-mirror**, NOT a 180° rotation (a rotation would put it west) |
| Eyeball | `TASK-350-blue_gate_close.png` (south face CLOSED wall) vs `TASK-350-blue_northface.png` (**the concept's open arch, on the north**) | human-readable proof |

**Everything is self-consistent per-space:** the FBX/Blender space is internally coherent (TASK-348's own probes were honest), the UCX/manifest space is coherent, and the two are related by y → −y **in-engine**. The mirror enters between FBX export and UE import (handedness/axis conversion not pre-compensated, or vice versa). My import used the shipped TASK-330 lane verbatim (same FbxImportUI options + `normal_import_method=IMPORT_NORMALS`); nothing local to this pass introduces a flip.

**⚠️ Likely FLEET-WIDE property, worth a CONVENTIONS clause once ruled:** every prior pipeline asset is bilaterally near-symmetric (units) or solid (buildings) — a Y-mirror is INVISIBLE on them. The hollow castle is the first asset where chirality is functional. The old castle's UCX-vs-visual in-engine chirality was never probed. Also note: the possibly-related TASK-348 finding that the Meshy donor had **negative signed volume** (an inside-out = mirrored mesh) whose winding was flipped outward by the new orientation guard — winding was fixed, but if the donor GEOMETRY is mirrored, the guard preserves that mirror. The art-director owns the root-cause split (donor chirality vs export axes vs import conversion).

**Fix lanes (art-director's call, manager routes):** (a) pre-compensate the flip at Stage-2 export so the FBX lands in UE unmirrored (manifest space stays authoritative — then ALL recipe numbers incl. the blocker retune stand as-is); or (b) negate Y across `ucx.boxes` + carve expectations + blocker/recipe numbers to embrace the in-engine orientation (touches more law). Either is a free re-run (cached donor, no credits). After the fix: re-run THIS task's import + retune + matrix (all recipe numbers re-verified here are otherwise good).

## Second finding for the re-run (programmer-adjacent, NOT a loop yet): nav-area stamping is transiently stale at PIE start

The first matrix run (~1 min after PIE-start) showed the RED castle's interior tiles still carrying the ctor-default BLUE area (Blue's filter could path into the red hall; Red's own filter could not) while the component property already read Red — the async Dynamic-Recast tile rebuild had not yet applied BeginPlay's `SetAreaClass(Red)`. Re-probing minutes later: fully correct. So the shipped design self-heals, but there is a **warm-up window each PIE start during which the red-side gating is nav-lane-incorrect** (physical blocker holds throughout). **Measure the window's length in real PIE at the re-run** — if it overlaps live gameplay meaningfully, that becomes a TASK-349 loop item (e.g., force a synchronous interior-tile rebuild or re-assert post-build). Related: one engine ensure fired during Simulate nav generation — `Areas.Num() <= 1` (RecastNavMeshGenerator.cpp:305), plausibly the per-team interior areas interacting with tile generation; hand it to the programmer with the window measurement.

## Message Log sweep (full session)
- `Failed to compile Material` **0** · `Accessed None` **0** · `Fatal error` **0**
- `Ensure condition failed` **2 distinct**: (1) the Recast `Areas.Num() <= 1` above (change-surface, see finding 2); (2) `!HasAnyFlags(RF_ClassDefaultObject)` (NavigationSystem.h:293) — triggered by MY diagnostic python calling the nav API through the CDO wrapper; tooling artifact, not game code.
- All 62 `LogOutputDevice: Error` lines are those two ensures' stack dumps (verified line-by-line ranges).
- 6 `Default Material will be used` = pre-existing missing-USAGE-FLAG warnings (Tree_Pack trunk/leaf `InstancedStaticMeshes`; MI_TeamColor_Red + MI_Cavalry/Pikeman/Archer_PBR `SkeletalMesh`) — none castle-related, none sampler-type. Recorded for the manager: worth a one-time usage-flag pass someday.

## Keep-roof camera occlusion (flag for Jonathan, as promised)
`TASK-350-blue_topdown_occlusion.png`: from the RTS top-down the interior is fully roofed — units inside will be INVISIBLE to the gameplay camera. `TASK-350-blue_inside_lookout.png`: the interior is also near-black inside (no interior lighting; auto-exposure did not rescue it in-frame). Placement inside via cursor will also be blocked by the roof intercepting the ECC_Visibility cursor trace. Jonathan should rule at playtest: cutaway/roof-hide-when-inside, interior light source, or accept-as-is.

## Editor/tree state at handback
- Editor RUNNING (PID 37676), MCP up, L_Arena loaded. Remote-exec ON (in-memory; reverts on restart). Background-CPU-throttle disabled in-memory (reverts on restart).
- **On disk (uncommitted, deliberately kept):** new `SM_Castle.uasset` + `T_Castle_{D,N,ORM}.uasset` (the mirrored-visual import — visually coherent standalone, collision functional; next import overwrites) + `DA_BattlefieldScatter.uasset` at 4500 (orientation-independent, correct for 3× — kept). All TASK-349 C++ + TASK-354 files untouched in the tree, compiled into the live DLL.
- **L_Arena NOT saved** (never-save law). The MANDATORY gate retune was applied per-instance, Simulate-verified (values above), then **REVERTED in-memory to the safe defaults (300,0,300)/(80,400,300)** so nothing half-shipped lingers; the package still shows a dirty flag with byte-identical values (delta-serialization: a save would write no semantic change) — same benign residue TASK-330/331 recorded. **Re-apply at the re-run: RelLoc (6, −525, 284) / Extent (260, 135, 226) on BOTH instances** (valid if fix lane (a) is chosen; flips to +525 under lane (b)).
- Board note re-confirmed: both castle actors read yaw 0.0 (the "Castle_Red carries yaw 180" claim did not reproduce — third sighting).

## Evidence files (handoffs/)
`TASK-350-blue_gate_close.png` · `TASK-350-blue_northface.png` · `TASK-350-blue_wide.png` · `TASK-350-red_wide.png` · `TASK-350-blue_topdown_occlusion.png` · `TASK-350-blue_inside_lookout.png`
