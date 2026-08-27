# TASK-659 — [F1-4] THE VID-001-F1 INTEGRATION: one compile · live verification · THE F1 COMMIT (build-master handoff)

**Status: COMPLETE — compile PASS, suite 127/127, every live-verify mandate discharged with numbers, THE F1 COMMIT cut (hash in §7). ⛔ Not pushed.** Date: 2026-08-27.
Verdict lineage: VID-001 → TASK-656 branch (i) EAST-FACE DISCOVERABILITY → TASK-657 (assets, ready-for-integration) + TASK-661 (anchors, qa-passed `qa/TASK-661.md` PASS 0 blockers) → this task.
**Grant record:** Jonathan's editor grant, verbatim relay 2026-08-27: *"I give you permission to close and reopen the editor as needed for the rest of this session."* — session-wide close/reopen; graceful-close-only, PIE-guard-first, save-modal law, never-save `L_Arena`, editor left UP at end, all honoured. One bounce spent (PID 15772 → 28772); no extra bounce needed.

---

## 1. PRE-FLIGHT (editor up, before the bounce)

- HEAD `6ece2a0` (the footage-pipeline commit) — exactly as dispatched; main 1 ahead of origin (Jonathan's push cadence, untouched). No self-commit collision.
- Porcelain reconciled line-for-line against the expected F1 tree — **zero unexplained lines**: Source delta EXACTLY `Siegebound/Castle.h` + `Castle.cpp` · 657's 16 Content uassets + 4 raw FBX + 9 raw PNGs · `pipeline_manifest.json` + `build_entry_dressing_props.py` · docs set (TASKBOARD + handoffs 655/656(+5 PNGs)/657/661 + `qa/TASK-661.md`).
- `Tools/ArtPipeline/Cache/**` proven gitignored (`.gitignore:27 Tools/ArtPipeline/Cache/*`); **`testvideo/` proven unstageable** (`git check-ignore` hits root `.gitignore:12` for both root and project paths; `git ls-files | grep testvideo` = 0; porcelain grep = 0).
- `L_Arena.umap` sha256 == ledger `b3dbc5d9ae484a7bd02cafad52b4681da68b011477479b65ee7781ae459f8268`, `is_dirty` false.
- PIE guard: `IsPIERunning` false (PID 15772 idle). Graceful `CloseMainWindow` → clean exit in ~15 s, **no save modal** (dirtiness 0 held). Hash re-verified identical post-close.

## 2. COMPILE + SUITE (the wave's one compile — QUIET-MODULE discharged)

- CLAUDE.md Build.bat line verbatim. **Log-parsed `Result: Succeeded`** (never the exit code), 16.27 s, 10 actions, `Castle.cpp` adaptive non-unity, 0 errors / 0 warnings.
- Suite (the TASK-470/593/606 recipe, cmd-redirect lane): **`...Automation Test Queue Empty 127 tests performed.`** Census: **127 `Test Completed`, 127 `Result={Success}`, 0 non-success, 0 `: Error:` severity lines**. Expected 127 (661 declared no tests) — expected == actual, no furnishing-count pin broke.
- Instrument note for the next build-master: `UnrealEditor-Cmd` twice bailed ~21 s after the Turnkey UBT step with NO abslog when invoked directly from PowerShell in the seconds after an editor shutdown; the identical command via `cmd /c` with output redirect (abslog in `Saved/Logs/`) ran clean. Transient, not reproducible at will; the passing run's log is `Saved/Logs/suite-659.log`.

## 3. LIVE VERIFICATION (editor relaunched PID 28772; SIE = the TASK-623..625 Simulate lane, no pawn, no console, latch UNSPENT)

**3a. The F1 furnishing log — BOTH castles, TWICE (two SIE sessions = the Play-Again edge live):**
`ACastle 'Castle_0': F1 discoverability set - 2/2 banners, 13/13 path segments, 10/10 toe rocks spawned (TASK-661; every component forced NoCollision - GH-R9).` — and the identical line for `Castle_1` (Red symmetry CONFIRMED, QA §6.7), at 20:57:45 (session 1) and 21:09:23 (session 2). Torch line byte-form intact both castles both sessions: `furnished - 6 of 6 torch anchors spawned (cap 6), commander spawned`. Commanders present: `BP_CommanderNpc_C_0` Blue (−25465, 810, 174) / `_C_1` Red (24535, 810, 174). No `family skipped` line ever fired (all four soft refs resolve).

**3b. The capsule battery (656's `overlap_script.py`, byte-for-byte the same script):**
- **Editor world (656's exact context): BYTE-IDENTICAL, all 68 rows** — east-face bisection (EMPTY ≥3665 / Castle_0 ≤3660), seal top (occupied ≤507, empty ≥508), bay seal, 6 walk stations (FREE 3710 / BLOCKED 3690), all 17 centre-lane stations, 4 riser-top bisections. F1-R3 clean: nothing lowered, live==manifest everywhere 656 measured.
- Live SIE world (props spawned): identical at EVERY station within reach of an F1 prop (stop lane, trample lanes, rock line, east face). Sole deltas: `lane0_y-1300/-1550/-1700/-1950` + `top_y-1300` upper slabs — **the BeginPlay-armed `GateBlockerVolume`** (castle-local y −1980..−1170, z 174..1530, x −882..+918; ctor-inert so 656's editor-world battery could never see it; boundary agreement exact — y −2200 and y −1000 stations outside its box stayed EMPTY). Not a prop, not a regression — the castle's own designed gate seal.
- **Props-collisionless verdict: PROVEN** — belt (AggGeom EMPTY ×4), braces (code-side NoCollision), and now the live probe record.

**3c. Nav delta:** `nav-config` standard, `Nav generation FINISHED` settled, **castle lane CONFIRMED (nav settled: 0 pending)**; no prop-attributable tile/cull line. `SetCanEverAffectNavigation(false)` held. (Mine finding → §5.)

**3d. The east visual profile (656's `east_script.py`, complex traces): BYTE-IDENTICAL to 656 in the live session** — the 29-row lane profile (grass 0 to x 3675, rim 95.01 at 3650 rising to 144.35), the 19-row cross-section, the west rays (244.84 / 463.05). The berm silhouette is untouched by the wave, and the props are trace-invisible exactly as NoCollision predicts.

**3e. QA §6 extras, all discharged:**
- **W1/D1 banner bases — now PROBED, not bracketed:** ground z = 0.00 flat at castle-local (±1400, −3675) AND at all four ±30-uu offsets, Blue both poles AND Red both mirrors. Riser 0 vs 50. Visual: both plinths ground-contacted, clear of knoll toe (capture set).
- **Ribbon edges at real width (348):** south/east edges flat 0.0 everywhere; north wrap edge tucks under the berm toe on local x 0..1800 (ground 53–78) and leg-1 west edge into the rim at y 0..−1200 (ground 98–110) — the QA-anticipated **trail-hugging-the-mound lap; NO segment visibly buried** in any capture; both trails read continuous.
- **Rock crest read:** rocks ON the crest across the 92–101 spread, no visible floater; at M1/M2 the line reads as an emphatic rubble wall at the exact refusal line (arcade doorway visible over the top). Final read = Jonathan at 660.

**3f. Captures (beside this handoff):** `TASK-659-before-M1/M2.png` (editor world == 656's frames: chartreuse rim + arcade + skirt mounds) · `TASK-659-after-M1/M2.png` (the video's exact views, props in-frame) · `TASK-659-south-approach-banners.png` (**both crimson chevron banners frame the mouth from the field, torches visible in the arch — "the one door" reads**) · `TASK-659-banner-west-base.png`/`-east-base.png` · `TASK-659-wrapleg-ribbon.png` · `TASK-659-rockline-crest.png` · `TASK-659-red-mouth-symmetry.png` (Red's full set mirrored) · `TASK-659-eastface-wide.png` (trail + rocks + banners coexist at the SE corner). Exterior sanity: no unintended visual delta vs the 656 references beyond the new props (3d is the byte-proof).

## 4. FINDING — the session-1 self-siege (evidence hygiene, on the record)

The first SIE session was left running ~11 min while instruments ran; **the Red AI besieged and razed the undefended Blue castle** (`Castle_0` read back `bHidden=true`, `CastleMesh=SM_Castle_Crumble03`). First-capture attempts caught the dead state and were re-taken in a fresh short session (captures inside the first ~90 s; no unit can finish the 46,000-uu march that fast). Bonus verification banked from the dead state: **the teardown edge is real** — the razed castle's component census carried no F1 component and no orphaned banner/ribbon/rock floats anywhere in the dead-castle frames ("a fallen castle sheds its signage"; WR-§4 orphan hazard absent live). Live CDO readback while dead-alive cycling: `GateBannerAnchors` = (±1400, −3675) yaw −90 exact.

## 5. FOLLOW-UPS FOR THE MANAGER (report-only, none blocks F1)

1. **Mine-reachability terminal fallback fired in SIE:** `LogSiegeTerrain: Error: Mine reachability unconfirmed after 5/6 culls; unreachable-mine approach discs force-cleared (best-effort economy guarantee - castle lane itself is CONFIRMED (nav settled: 0 pending))` + repeated `SymmetryEscape: mine-approach repair culled 4-6 instance(s) around 2 unreachable mine(s)`. Structurally NOT F1 (BattlefieldScatter byte-untouched per QA §1; props collision- and nav-inert, proven §3b/3c); no baseline SIE log exists at HEAD~1 to date it. Suspect: standing terrain state around 2 mines. Worth its own diagnosis row.
2. **AI razes an undefended castle in ≲6 min of unattended simulate** (§4) — balance data point only.
3. **M1/M2 rock dominance:** at the stop line the picket fills the frame (authored "rubble barrier, not a door", delivered emphatically). If Jonathan's eye wants it thinner, it is an `EditDefaultsOnly` anchor retune + recompile — a 2-line defaults change.
4. Standing plaza sighting (J7 pavement shards) visible again in the south-approach capture — unchanged, parked.

## 6. §25b — LFS oid == worktree sha256 (never by size)

All **31 rows of 657's table verified OK** against the worktree pre-stage (16 uassets + 4 FBX + 9 PNGs + manifest + `L_Arena` unchanged-row). Post-stage, every staged LFS pointer oid re-verified == its worktree sha256 (30 binaries of the 657 set + the 5 TASK-656 capture PNGs + the 11 TASK-659 capture PNGs) — see the commit block below; zero mismatches.

## 7. THE F1 COMMIT

- HEAD re-verified `6ece2a0` immediately before staging (no drift). Explicit `GitClaudeUnrealTest/`-prefixed paths only; TASKBOARD.md staged LAST (expected cargo: the F1 wave + older orchestrator flips — the docs-debt ride precedent). Check-and-carry: VID-001 report + playtest-evidence PNGs + FR-§ CONVENTIONS delta + SLACK.md registry row are already IN `6ece2a0` — nothing to carry.
- **Commit: see `git log -1` — recorded by the orchestrator report; ⛔ NOT pushed** (main 2 ahead after the commit).

## 8. EXIT STATE

Editor **UP PID 28772**, MCP green, level `/Game/Maps/L_Arena`, PIE **not running**, viewport camera restored to where the session found it. `L_Arena.umap` sha256 == ledger `b3dbc5d9...8268` at entry, after the bounce, and at exit; `is_dirty` false; **expected dirtiness 0 — decline any save prompt**. Latch UNSPENT; no console sentence, no `M`, no pawn input ever issued (SIE only). **TASK-660 (Jonathan's walk → VID-002) is next and the editor is ready for it.**
