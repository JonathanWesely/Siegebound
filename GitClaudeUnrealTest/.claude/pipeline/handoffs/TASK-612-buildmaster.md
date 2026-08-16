# TASK-612 — [WR-46] THE INDEPENDENT INTEGRATION CHECK + THE COMMIT — floor-sink repair R1 landed (build-master handoff)

**Verdict: ALL FOUR GATES GREEN, INDEPENDENTLY MEASURED — COMMITTED.**
Date: 2026-08-16. Editor: Jonathan's live session (PID 17704), MCP `http://127.0.0.1:8000/mcp`, left OPEN on `L_Arena` exactly as found. No `Build.bat` anywhere in this lane (W10-R6). No push. No branch cut.
Laws: `W6-R2` · W10-R5..R7 · GIT HAZARD LAWS (a)-(f) · `§25`/`§25b` · `SC-§29b` · `NAV-§4`/`NAV-§12` · the never-save law.

**Independence note (§17):** every number below is from MY instruments run this task — TASK-611's §3/§4 tables were treated as claims to test, not premises. Zero discrepancies beyond instrument resolution were found (detail in §5).

---

## 0. HARD-FENCE LEDGER

| | SHA256 `Content/Maps/L_Arena.umap` |
|---|---|
| ENTRY (before any editor touch) | `B3DBC5D9AE484A7BD02CAFAD52B4681DA68B011477479B65EE7781AE459F8268` |
| EXIT (after all probes + the Simulate leg) | `B3DBC5D9AE484A7BD02CAFAD52B4681DA68B011477479B65EE7781AE459F8268` |

**BYTE-IDENTICAL** — the map was traced and simulated, never dirtied, never saved. No save prompt appeared; none was accepted. PIE discipline: **Simulate-In-Editor only** (`bSimulate=true` — no player pawn ever spawned, so no input surface existed by construction): ⛔ no `M`, ⛔ no console sentence, ⛔ no input of any kind — **the TASK-571+552 latch is UNSPENT and TASK-579's first-open instrument is untouched.** `IsPIERunning=false` and level `/Game/Maps/L_Arena` re-verified at exit.

Worktree content ledger at commit time (all unchanged across the Simulate leg — measured before AND after):

| file | worktree SHA256 |
|---|---|
| `Content/Meshes/SM_Castle.uasset` | `063a120e4a07cd2847fd54d3dbcf67ef450bd4761b0cc40b9c20820c2bba4b88` |
| `Content/Meshes/SM_Castle_Crumble01.uasset` | `2e8b9a46c888b7af36cfbf832b719109efc392fa15b29179036eaa1d7cffc583` |
| `Content/Meshes/SM_Castle_Crumble02.uasset` | `9b9f3bc947a1fd637ac387340ca6ff143f6e11fad0ea05d6e174b8ef98936b0a` |
| `Content/Meshes/SM_Castle_Crumble03.uasset` | `4a809741b652f9c0a320f8f656ce3c167c23af04a1e032eb6de145a064fe5b84` |
| `Tools/ArtPipeline/pipeline_manifest.json` | `ddf497b13ad432cb71c4a66b7f9ae2a2e00f083bd3e1ab78d705b64a2c6ec786` |

Commit hash: a file cannot carry the hash of the commit that contains it — the hash is recorded in the Slack completion post (🔧 Build & Git thread) and the orchestrator report. Parent at commit time: `7a4bf39`, main 0/0 vs origin at dispatch.

## 1. GATE (1) — INDEPENDENT READBACK + PER-STAGE INVARIANCE (`W6-R2`) — ✅

Live `BodySetup_0` on all four assets, read via my own ObjectTools script (not TASK-611's), compared hull-by-hull IN ORDER against `pipeline_manifest.json` `assets.Castle.ucx.boxes` (35 entries, read this task):

| mesh | box | convex | sphere | sphyl | tapered | levelSet-family | trace flag | max dev vs manifest (center+size, every hull) | max abs rotation |
|---|---|---|---|---|---|---|---|---|
| SM_Castle | **35** | 0 | 0 | 0 | 0 | 0/0/0/0 | CTF_UseDefault | **0.000** | 0.000 |
| SM_Castle_Crumble01 | **35** | 0 | 0 | 0 | 0 | 0/0/0/0 | CTF_UseDefault | **0.000** | 0.000 |
| SM_Castle_Crumble02 | **35** | 0 | 0 | 0 | 0 | 0/0/0/0 | CTF_UseDefault | **0.000** | 0.000 |
| SM_Castle_Crumble03 | **35** | 0 | 0 | 0 | 0 | 0/0/0/0 | CTF_UseDefault | **0.000** | 0.000 |

**PER-STAGE INVARIANCE: IDENTICAL** — canonical-JSON serialisation (35 × [center xyz, size xyz, rotation pyr], 6 dp) compared by full string equality: Crumble01 == Crumble02 == Crumble03 == pristine. Not a hash — direct content equality. Elem names `None` by design (order = manifest order).

Independent anchors from the live set: tread_01 top/bottom **29 / −60** · gate_lintel `(18,−1560,1980)/(1560,780,900)` **unchanged** · floor_slab_hall `(−120,735,57)/(3960,930,234)` ⇒ y-span **270…1200** (F3 rider live) · berm_01 top **497** · tread_09 y-span **−1796.5…+270**.

## 2. GATE (2) — THE RE-TRACE, TASK-598's INSTRUMENT, BOTH CASTLES — ✅

Instrument pair re-run from scratch: complex down-trace (`trace_world`, z 1000→−200) = visual; WorldStatic overlap probe (4×4×1 slab, `ObjectTypeQuery1`) with MY OWN coarse-scan + 11-step bisection top-finder (resolution ±0.03) + the TASK-598 ±0.6 bracket pair = support. **Castle_0 (Blue, −25000) and Castle_1 (Red, +25000) returned IDENTICAL numbers at every column** — one table serves both, beside the TASK-598 baseline:

| # | (x,y) local | visual z | support top (actor) | **live Δ** | TASK-598 shipped Δ | pinned W10-R5 acceptance | verdict |
|---|---|---|---|---|---|---|---|
| S1 | (1050,−2350) | 496.73 | 497.00 (Castle_N) | **−0.27** | +496.73 SINK | supported-at-visual | ✅ |
| S2 | (−1050,−2350) | 490.18 | 497.00 (Castle_N) | **−6.82** | +490.18 SINK | structural wall | ✅ |
| S3 | (990,−2350) | 467.59 | 497.00 (Castle_N) | **−29.41** | +380.59 SINK | blocked | ✅ |
| S4 | (810,−2170) | 410.74 | 411.01 (Castle_N) | **−0.27** | +294.74 SINK | supported-at-visual | ✅ |
| S5 | (1050,−2920) | 116.33 | 72.51 (Castle_N) | **+43.82** | +116.33 SINK | supported (declared D5 residual) | ✅ |
| S6 | (0,−2900) | 85.82 | 72.51 (Castle_N) | **+13.31** | +27.82 SINK | **≤ 14.5** | ✅ **PASS** |
| F1 | (0,−500) | 127.34 | 145.00 (Castle_N) | **−17.66** | −46.66 FLOAT | **≈ −17** (F3 rider) | ✅ **PASS** |
| HALL | (0,700) | 174.00 | 174.00 (Castle_N) | 0.00 | — | flush | ✅ |
| GND | (0,−4000) | 0.00 | 0.00 (`StaticMeshActor_1` = ArenaGround) | — | — | instrument control | ✅ |

All 18 bracket pairs (±0.6) verified at both castles: above-empty / below-hit, every column. **Centre-route (|x| < 720) max residual = +13.31 ≤ 14.5 — no STOP condition exists.** Visual values reproduce TASK-598's baseline to the hundredth ⇒ no visual byte changed (R1 manifest-only, re-proven at my instrument).

Negative probes, both castles, all as declared: old slab position (0,−500,173.4) **EMPTY** · widened shelf (1050,−2920,60) **hits Castle_N** · mid-berm (1050,−2350,300) **hits Castle_N**.

## 3. GATE (3) — NAV + ENTRY SANITY (Simulate leg) — ✅, with one declared, pre-approved deviation

**Declared deviation from the spec's "PIE from a fresh boot", approved by the orchestrator BEFORE the run:** an editor restart contradicts the session fence (Jonathan's live editor, editor-close-is-Jonathan's-choice) and carries the Slate save-modal coin-flip hazard with `L_Arena` loaded under the never-save law. Freshness was instead isolated by **baseline-delta**: the settled-only grep was counted BEFORE the run (baseline = exactly 2 lines, both stamped 18.30.36, pre-Simulate) and only lines timestamped after `StartPIE` count.

**The falsifiable criterion, pinned BEFORE the run (the orchestrator's hard condition):** FAIL = (a) no new `CONFIRMED (nav settled:` line after StartPIE (leg unobserved); or (b) a new line with pending > 0, a missing `Blue→Red castle path`, < 6 mine paths, or culls > 0; or (c) new spawn/encroachment errors. In the fail world (blockers sealing the corridor / a riser breaking walkableClimb) the castle-to-castle path cannot exist on the settled navmesh and the CONFIRMED clause cannot print — the trigger does NOT fire identically in pass and fail worlds.

**Result — PASS on all three:** grep count 2 → **4**; the two NEW lines, verbatim:

```
[2026.08.16-19.50.05:339][405]LogSiegeTerrain: [BattlefieldScatter 'BP_BattlefieldScatter_C_0'] Traversability CONFIRMED (nav settled: 0 pending) — 0 tile task(s) pending; Blue→Red castle path + 6 mine path(s) exist (after 0 cull(s)) [definitive: OnNavigationGenerationFinished].
[2026.08.16-19.50.05:555][418]LogSiegeTerrain: [BattlefieldScatter 'BP_BattlefieldScatter_C_0'] Traversability CONFIRMED (nav settled: 0 pending) — 0 tile task(s) pending; Blue→Red castle path + 6 mine path(s) exist (after 0 cull(s)).
```

⇒ The navmesh settled over the NEW 35-hull collision in the running world and the castle-to-castle path plus all 6 mine paths exist with 0 culls — the `NAV-§4`/`NAV-§12` settled-only, definitive form. Error sweep over the Simulate window: **zero** spawn-collision/encroachment/`does NOT exist` lines (only benign editor-boot dll probes, pre-dated). **Interior spawning observed on PIXELS**: a viewport capture (pose-override — Jonathan's camera untouched) inside Castle_0's hall during the live Simulate world shows the commander NPC standing at the war-map table at correct floor height — not sunk, not floating — with the table and interior scatter intact.

Honest scope note (`W8-R4`): no unit was marched through the gate on screen (no input lane exists, by design of this leg). The entry claim rests on (a) the path-level proof above on the settled navmesh, (b) the measured riser chain 29 + 8×14.5 + 29 (all ≤ 29 < 35 nominal AgentMaxStepHeight) and the blocker inner faces at exactly |x| = 720 from gate (1), and (c) zero regressions at every corridor column in gate (2). The 11 unobserved PIE-matrix rows remain unobserved; nothing here claims them.

## 4. GATE (4) — `L_Arena` HASH — ✅ entry = exit = `B3DBC5D9…F8268` (§0), byte-identical.

## 5. DEVIATIONS FROM TASK-611's EXPECTATION TABLE — NONE MATERIAL

Instrument-resolution differences only: my bisection tops read 411.01/72.51 where the hull tops are 411/72.5 (±0.03 resolution), giving S4 −0.27 (vs −0.26), S5 +43.82 (vs +43.83), S6 +13.31 (vs +13.32). Every classification, every support identity, every pass/fail identical. TASK-611's declared departures D1–D7 stand as declared; the D1 lip strip contains no S-column and was not re-litigated.

## 6. GIT — THE COMMIT

- Pre-verified with my own instruments: HEAD `7a4bf39`, `origin/main...main` = 0/0, porcelain re-read after the Simulate leg (composition unchanged).
- **`§25b` digest lane (LFS):** all four `.uasset` = `filter: lfs`; each HEAD pointer oid ≠ worktree sha256 (real byte change on all four — never a size check); staged pointer oids verified == the §0 worktree sha256s before committing. Docs/manifest = `filter: unspecified` ⇒ diff lane.
- **Whitelist (derived from the ledger, explicit pathspecs only — no `git add .`, no `-a`):** the five W10-R7 artifacts + `handoffs/TASK-597-artist.md` + `handoffs/TASK-598-buildmaster.md` + `handoffs/TASK-611-artist.md` + this file + `TASKBOARD.md`.
- **EXCLUDED (the in-flight ACCOUNTS code lane — not this commit's):** all `Source/**/Siege*` new/modified files, `handoffs/TASK-599..604-programmer.md`, `qa/TASK-605.md`, `CONVENTIONS.md` (its diff is the ACC-§ block — ACCOUNTS-lane law text).
- **Named rider:** `TASKBOARD.md` is one shared file; its uncommitted state includes the ACCOUNTS batch's board text (headline + decomposition + a manager status annotation) alongside Wave 10. Board text is documentation — no code rides this commit, and `git diff --cached --name-only` was pasted to the log before committing (`SC-§29b` checked: no `.cpp`/`.h`/`.gen.cpp`/`Intermediate/`/`.umap` staged).
- **This lane compiled NOTHING** (W10-R6): no `Build.bat`, no C++ byte in the commit. TASK-606 owns the next compile gate; TASK-596 rides there.

## 7. NEXT / OPEN ITEMS (for the manager)

1. **R2's trigger is Jonathan's eye** (W10-R5): does the residual ≤ 14.5 saw-tooth still read on screen? A playtest item for the next feedback round — not a task today.
2. **TASK-571 remains owed by Jonathan in one sitting, order load-bearing** — untouched by this lane (latch unspent, `M` unpressed, no console sentence ever typed).
3. The D1 lip strip (y −3616…−3560, residual 14.5–22.1) is the R1 floor by measurement; R2 (visual re-derivation) is the held escape.
