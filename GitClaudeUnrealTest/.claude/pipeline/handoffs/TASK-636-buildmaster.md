# TASK-636 — [GH-11] THE FULL GATE + THE REDESIGN COMMIT (build-master handoff)

**Status: COMPLETE — every gate PASS; the GRAND-HALL wave (TASK-626..636) is committed by this file's carrier commit.** Date: 2026-08-23. Editor: Jonathan's boot PID 5456 (09:53) bounced under his explicit this-session G4 grant (graceful close, clean exit in <15 s, ZERO save prompts — the dirtiness-zero prediction held); relaunched PID 14984, MCP `http://127.0.0.1:8000/mcp` green, left UP for the checkpoint. No console/`M`/`DumpAssistantPrompt`/pawn input (CF-R3 — the TASK-571+552 latch untouched); no map save; TASKBOARD statuses not edited (orchestrator flips).

Access-lane note (the 633 §12 lane carried): the unreal-mcp tools were not in this session's registered tool list — the SAME endpoint was driven directly over streamable-HTTP JSON-RPC (`call_tool` → `editor_toolset.toolsets.*` / `EditorToolset.*`). tools/call responses arrive as SSE on the POST stream (read incrementally — a plain body read comes back empty).

## 0. HARD-FENCE LEDGER (CF-R4)

| | SHA256 `Content/Maps/L_Arena.umap` |
|---|---|
| **ENTRY** (pre-flight, PID 5456 up) | `b3dbc5d9ae484a7bd02cafad52b4681da68b011477479b65ee7781ae459f8268` |
| post-close (bounce) | same |
| **EXIT** (post-compile, post-suite, post-identity, post-battery, post-SIE) | `b3dbc5d9ae484a7bd02cafad52b4681da68b011477479b65ee7781ae459f8268` |

**BYTE-IDENTICAL end-to-end; never saved.** `is_dirty` false at entry AND exit on: `L_Arena` + all four castle meshes + `MI_Castle_Interior_PBR`. SIE stopped clean (`IsPIERunning` false).

## 1. PRE-FLIGHT (step 0)

- HEAD `7142839`, main 3-ahead-0-behind origin ✅ (re-verified again immediately before the commit — HEAD had not moved; Jonathan-self-commit check clean).
- **Source-delta deviation, ruled by measurement:** the dispatch (and qa/TASK-634 note 1) expected FOUR Source files incl. `BattlefieldScatter.cpp`. The tree carries exactly THREE: `Castle.cpp` + `ScatterConfig.h` (TASK-634) + `Castle.h` (5 lines, verified = ONLY the 634-addendum slot-contract comment). Mechanism: **the TASK-623 pair (`BattlefieldScatter.cpp` +28 / `Castle.h` rider +7) already landed in HEAD `7142839`** (2026-08-17, `git show --stat` proof) — the QA note (2026-08-18, no git instrument) predates that knowledge. Zero UNREVIEWED files — the gate's intent holds; the STOP condition (a fifth file) never fired.
- §25b (oid==worktree-sha law, never byte size): **all 20 wave files hashed; 20/20 MATCH** the 633 §8 table + the §12 addendum supersessions (`SM_Castle.uasset` `237b36b0…319d`→ full `237b36b0ac62984fcb96383bf5a0b20493cc319268236f951c74609bd38f0b3d`; `MI_Castle_Interior_PBR.uasset` `1405e0b1…ce24`; manifest `ee8e2357…3254` byte-intact; L_Arena entry pasted above). Post-stage, `git ls-files -s` LFS pointer oids re-verified == worktree sha256 (see §7).
- `SIE-§1` read-early: editor log tailed BEFORE the bounce — quiet (only the §12 repair saves at 17:09 + EOSSDK heartbeats; zero anomalies).
- The §10→§12 repair CONFIRMED live: MI on disk, `SM_Castle` slot 2 resolves (§5) — the commit fence stays LIFTED.

## 2. THE WAVE'S ONE COMPILE (G4) + SUITE — step 1

- Bounce: `CloseMainWindow()` on PID 5456 → clean exit <15 s, **no Save Content modal** (predicted: none). `L_Arena` hash unchanged post-close.
- Pinned Build.bat line; **log-parsed `Result: Succeeded`** (exit code never trusted), 18.19 s wall, UBA 13 actions, adaptive build compiled **`Castle.cpp` + `ScatterConfig.cpp`** by name, relinked `UnrealEditor-GitClaudeUnrealTest.dll`. `warning|error` grep over the full log: **0 lines**. SAC 2s-fail: did not fire. No compile-fix diff ⇒ no SC-§27 item.
- Suite (TASK-593/606 command + Entry-map ini override): **"Automation Test Queue Empty 118 tests performed." — ACTUAL 118, `Result={Success}` ×118, non-Success 0** (expectation 118 held). `L_Arena` mentions in the suite log: 0. Fresh-binary proof: DLL relinked minutes earlier.
- Relaunch → MCP up in ~2.5 min (PID 14984).

## 3. IDENTITY GATE ×4 (612 pattern) — step 2 — PASS

Per-mesh `BodySetup_0` readback (`ObjectTools.get_properties`, camelCase wire):

| mesh | boxElems | convex/sphere/sphyl/tapered | trace | vs manifest v3 (66 rows) | canonical |
|---|---|---|---|---|---|
| SM_Castle | 66 | 0/0/0/0 | CTF_UseDefault | element-exact 66/66, max dev **0.000000** | `ebdd54ff…65ed` **MATCH** |
| Crumble01 | 66 | 0/0/0/0 | CTF_UseDefault | 0.000000 | **MATCH** |
| Crumble02 | 66 | 0/0/0/0 | CTF_UseDefault | 0.000000 | **MATCH** |
| Crumble03 | 66 | 0/0/0/0 | CTF_UseDefault | 0.000000 | **MATCH** |

**`W6-R2`: the four canonical serialisations are string-IDENTICAL.** The 631 type-template instrument reproduced file-side first (manifest → `ebdd54ff7888b38ba7f803c4f27228ebad776a2b4af2fb6c5a963ecc570565ed` exact); wire note for future gates: the readback JSON drops `.0` on whole floats — manifest-typed canonicalisation must coerce int-typed literals to int AND float-typed literals to float.

## 4. LIVE VERIFICATION, BOTH CASTLES — step 3 — PASS (162-check battery + corrections, `t636_live_verify.json` in session scratchpad; every number in this file)

Castles verified live at (∓25000, 0, 0) yaw 0 ×2.

**(a) 631 §5 predicted-trace contract VERBATIM — every row, both castles, PASS:**
- Entry chain x=0: all 13 stations face-probe exact (0→29→43.5→58→72.5→87→101.5→116→130.5→**148→161→174**); **worst riser 29.0 ≤ 50** ×2. Flank lanes x=±315: interior tail exact ×4 lanes.
- Interior flush controls: (0,−1120) F1 **174/174 FLUSH** (float ERASED) · (±315,−1120) flush (was +29 — ERASED) · (0,700) · (−455,805) commander-adjacent · (910,1330) · (−1890,1330) — the NEW north band — all 174/174, visual≤±1 ×2 castles. Sill landing vis 148 · landing2 161 face+vis ✓.
- BLOCKED verdicts: (1300,800) `hall_wall_04` annex solid ✓ · (1295,−1155) bay_seal_east ✓ · (−1295,−1155) bay_seal_west ✓ (BLOCK is the contract).
- Seals byte-true: d1 east face top 506.5 · d1-mirror west 515.5 · d3 (−3630,−1890) 138.5 · d2 wrap toe_07 AND toe_11 top 109 · toe_14 518 · toe_10 518.5 ✓ ×2.
- Carried controls: (0,−2905) vis 85.57 (+13.07) · (−280,−2350) support 101.5 + vis 116.44 (+14.94) ✓.
- Plateau: mouth (±700,−2100) 174 · south approach 101.5 (face +72.5 BY DESIGN) · mounts tread_08→plateau **+43.5** and flank→plateau **0** (was +29 — improved) ✓ ×2. Bury-strip visuals RECORDED: E 226.9 / W 275.5 over support 174 (inside the declared ≤354 cosmetic class, 0-walkable, ledger item for Jonathan's eye).
- Gate passage: jamb bodies at −750/+780 with corridor OPEN at −725/+760 (the −735/+771 planes → gap 1506) · doorway jambs −770/+806 with open −755/+790 (gap 1560) · lintel open ≤1526 / body at ≥1534 (bottom 1530) ✓ ×2.
- Instrument-correction ledger (three first-pass fails, all Blue==Red mirrored, all MINE not the world's): (i) (−280,−2345) probe box straddled the tread_06/07 boundary at y −2344.5 → tight re-probe at −2350 PASS; (ii) toe_11 probed at x −2020 but the hull spans −1872.5..−1470 (manifest) → re-probe at −1700 PASS; (iii) capsule-lattice west column at x −1878 touched the −1920 wall plane exactly → x −1877 PASS 0/6.

**(b) Hall hollowness (GH-R9 HERO capsule Ø84×192, never the Ø68 unit):** envelope overlap sweeps (native physics, ObjectTypeQuery1–10) across the OPEN hall (−1920..990 × 240..1380 × 174..2160), corridor (−735..771 × −1140..240 × 174..2100) and passage channel (−462..498 × −1780..−1140 × 174..1530), ε=0.5 shrink: **ZERO castle hits ×3 envelopes ×2 castles**. 30-point hero-capsule lattice (42,42,96 half-extents, standing + mid-height): **0 hits ×2**. Capsule standing at BOTH threshold landings (148/161): clear ×2.

**(c) The two carried qa/TASK-634 items:**
- **≤26-uu GateBlocker slit:** landings measured live at 148/161; the hero capsule (192 tall) stands clear at both landings and spans far above z 174 — no capsule-relevant gap exists at the threshold (26 ≪ 84/192); three-record chain closed.
- **Corridor east-corner ~23-uu margin:** RECORDED on pixels — `TASK-636-corridor-east.png` whole-frame L 50.4, **east half L 60.7** (brighter than the west half; A6 pool + hall spill). The passage does NOT read dark; ⛔ the 7th mirrored anchor NOT added (Jonathan's TASK-571 feel lane, untouched).

**(d) Commander + war table + torches ON PIXELS (SIE, fresh world seed 280437089, all reads T+<120 s, zero decay/crumble lines):**
- 12/12 `BP_Torch_C` at the SIX RE-DERIVED anchors EXACT ×2 castles (A1..A6 local (−1435,1380)/(−465,1380)/(505,1380)/(−1435,240)/(990,810)/(−735,−315), z 954, yaws −90/−90/−90/+90/180/0). Light readback: **Intensity 90 cd · AttenuationRadius 1900 · Candelas · LightColor (1, 0.8667, 0.6784)** — the 622 §1 record verbatim.
- 2/2 `BP_CommanderNpc_C` at the anchor **(−465, 810, 174) local, yaw −90, EXACT** (the 634 re-base proven live). `WarTableMesh` RelativeLocation (200,0,0) ×2 → table at local y **610** (370 clear of the south wall, 100 north of the mouth); `SM_WarTable` resolves.
- 622-pose captures beside this file: `TASK-636-entrance.png` / `-hall-eye.png` / `-hall-above.png` (+ `-hall-interior.png`, `-corridor-east.png`). Commander DISCERNIBLE at the table on pixels (hall-eye + hall-above + interior close-up). Reference metrics (622 means are reference NOT gate — the hall geometry moved): hall-eye floor centre L **59.1** (well clear of the pre-fix 27.3 black-floor class), torch pools R/B **1.341/1.346 > 1.3 warm**, hi250 3.6-3.9% pool cores; **entrance sun treads L 147.9/170.1 vs 622's 143.1/169.3, hi250 0.00% — no new blowout, D3 behaviour carried**.
- Interior renders on the NEW PBR set everywhere — zero WorldGrid/fallback faces.

**(e) Mid-match crumble stage-swap eyeball (GH-R12 — captured, NOT judged):** in the TRANSIENT SIE world (`UEDPIE_0` paths — persistent level untouched, restored + verified), `CastleMesh` swapped → Crumble01/02/03, hall captured each (`TASK-636-crumble0N-hall.png`): **the hollow hall is present at EVERY stage** — same walls/floor/anchors; interior crumble MIs render (no fallback). Stage-spread record for Jonathan's eye: pristine wall L 94.8 / floor 59.1 → stages wall ~80-81 / floor ~36; the BETWEEN-stage deltas on rect means are subtle (the scorch pattern is spatial) — his checkpoint eye is the boarded arbiter. Declared: the swap keeps the team slot-0 override (runtime `ApplyCrumbleStage` would write `MI_Castle_Crumble0N` there), and Blue's team band shows on some interior arcade faces (design-time slot 0) — checkpoint item.

**(f) Nav (`NAV-§4`/`NAV-§12`) — DEFINITIVE PASS at T+6.9 s:** *"Traversability CONFIRMED (nav settled: 0 pending) — 0 tile task(s) pending; Blue→Red castle path + 6 mine path(s) exist (after 0 cull(s)) [definitive: OnNavigationGenerationFinished]"* — both the standard and definitive lines fired. 6 GoldNodes live at the MinesPass coordinates. (MinesPass `culls=2` entries = that pass's own retries, per the 619 ruling.)

**(g) Exterior spot-renders (GH-R1):** `TASK-636-ext-front-Blue/Red.png` + `-ext-east-Blue/Red.png` (editor world) — exterior masonry/berm/treads read unchanged; the through-gate view now shows the lit stone interior (629's intent). Byte-identity itself rides the upstream proofs (629 vertex-identity, 631's 26,274-column visual identity, this battery's byte-exact seal faces); the renders are the record for Jonathan's eye.

**(h) 632's gate (dims/slots/aperture, NOT tri count):** slots ×4 = `[TeamRegion, CastlePBR, CastleInteriorPBR]` with the exact 630 §7.3/GH-R12 bindings (SM_Castle: TeamColor_Blue / Castle_PBR / **Interior_PBR resolves**; crumbles: Crumble0N ×2 + Interior_Crumble0N). Aperture probed live ((a) above). LOD0 tris 26,515 ×4 RECORDED (632's declared sliver family: source 26,517, UE culled 2 — not a gate).

## 5. THE COMMIT — step 4

HEAD re-verified `7142839` immediately pre-commit (unmoved). Staged by EXPLICIT pathspec only (⛔ no `add -A`): 4 FBX + 3 source PNGs + 4 mesh uassets + 3 texture uassets + 4 MIs (incl. `Instances/MI_Castle_Interior_PBR`) + `Castle.cpp`/`Castle.h`/`ScatterConfig.h` + manifest + handoffs 626/629/630/631(+grid csv)/632/633/634 + `qa/TASK-634.md` + this file + 12 PNGs; **TASKBOARD.md + CONVENTIONS.md staged LAST immediately before the commit** (the manager's concurrent ACCOUNTS-Phase-2 docs content rides as-is — the a49f740 precedent; not diff-policed). `BattlefieldScatter.cpp` from the dispatch list: no diff exists (landed in `7142839`) — nothing to stage. ⛔ Not pushed. Commit hash in the orchestrator report + Slack.

## 6. FOR JONATHAN'S CHECKPOINT EYE (the G2-successor acceptance)

1. The new hollow hall in person: enter via the south ramp → gate; the walk line is machine-proven, the FEEL is yours.
2. Interior stage-spread (crumble 1/2/3 read) — GH-R12's explicitly-yours call (`TASK-636-crumble0*.png` for reference).
3. The team-color band on interior arcade faces (slot 0 design-time) — intended/not.
4. Corridor east-corner brightness (recorded NOT-dark; the 7th anchor remains your feel lane).
5. Plateau bury-strip cosmetic (E 226.9 / W 275.5 visual over 174 support, off-channel, carried class).
6. Torch mount z 954 sits below as-built mid-height 1167 (declared FEEL item from 634, untouched).

## 7. LEDGER

- Writes: this handoff + 12 PNGs + the commit itself. Zero asset/property writes to any PERSISTENT world object (the crumble swap lived and died in `UEDPIE_0`); zero saves; zero board-status edits; scratchpad instruments (`mcp_client.py`, `t636_*.py`, logs) outside the repo.
- Post-stage LFS/oid audit: staged pointer oids == the §1 worktree sha256 table (spot-checked all Content binaries).
- Suite/build logs: session scratchpad `build-636.log` / `suite-636.log` (Result + 118-count lines quoted in §2).
- Editor left UP (PID 14984), MCP live, `L_Arena` loaded, dirtiness zero — ready for the checkpoint.
