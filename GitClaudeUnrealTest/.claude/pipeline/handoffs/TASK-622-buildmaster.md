# TASK-622 — [CR-5] THE LIGHTING PIXEL RE-CHECK + INTEGRATION + COMMIT (build-master handoff)

**Status: COMPLETE — VERDICT: PASS on all four acceptance gates; the lighting lane is committed.** Date: 2026-08-17. Same batched session as TASK-619 (editor PID 14604, the post-ini boot — bounce record in `handoffs/TASK-619-buildmaster.md` §0). No console/`M` (CF-R3), no map save, `SIE-§1` honoured (all SIE reads at T+≤30 s, fresh worlds, zero decay lines).

## 0. HARD-FENCE LEDGER (CF-R4) — the WHOLE batched occupancy (619+622)

| | SHA256 `Content/Maps/L_Arena.umap` |
|---|---|
| **ENTRY** (pre-bounce, Jonathan's session PID 4764) | `B3DBC5D9AE484A7BD02CAFAD52B4681DA68B011477479B65EE7781AE459F8268` |
| **EXIT** (after 619's traces/SIE + 622's SIE captures) | `B3DBC5D9AE484A7BD02CAFAD52B4681DA68B011477479B65EE7781AE459F8268` |

**BYTE-IDENTICAL.** `is_dirty` false on L_Arena, BP_Torch, SM_Castle at exit; PIE stopped; no save prompt ever appeared. Editor left UP on `L_Arena` for TASK-625 (closes later under the same session grant).

## 1. LIVE READBACK (the fresh post-ini boot)

- **Boot cvars:** `r.DefaultFeature.LocalExposure.HighlightContrastScale` = **0.70** · `…ShadowContrastScale` = **0.65** (SearchCVars, live — the D3 keys read at boot; the capture session is a legal post-ini session per the 620/621 boot law).
- **All 12 spawned torches** (SIE, `BP_Torch_C_0..11`, PointLightComponent readback): Intensity **90** · AttenuationRadius **1,900** · IntensityUnits **Candelas** · LightColor **(1.0, 0.8667, 0.6784)** = the exact sRGB round-trip of linear (1, 0.72, 0.42) — colour unchanged. 12/12 identical.
- BP_Torch.uasset worktree SHA256 `451924C9B59B5D8EAACCCB737EEF4EF47E06EAB3E4CFBD0914A19A5DC4A17AA0` == the TASK-620 §3 record (LFS-tracked ⇒ this is the committed oid — `§25b` discharged).

## 2. CAPTURES — the 617 poses reproduced (pose-override CaptureViewport, FOV 90, viewport 2751×792 = the baseline's exact raster)

Poses calibrated against the baseline PNGs in the editor world first, then captured in a fresh SIE world (seed 709317761, rot180, layers=7; 12 torches present; zero crumble/match-over lines — the freshness proof). Files beside this handoff:

| file | pose (world, Blue castle) |
|---|---|
| `TASK-622-entrance.png` | (−24900, −4650, 260) yaw 90 pitch 0 |
| `TASK-622-hall-eye.png` | (−25000, −1500, 334) yaw 90 pitch 0 — the corridor-mouth eye-334 pose |
| `TASK-622-hall-above.png` | (−25465, 1050, 3200) pitch −90 yaw 0 |

Declared pose note: 617 recorded no numeric transforms — poses were reconstructed from its PNGs + handoff anchors (commander offset, gizmo axes, table position); every metric below is computed over IDENTICAL pixel rects on baseline and new images, so residual pose deviation cannot manufacture a pass. Baseline floor readings from my rects (L 27.3 / 21.1 at hall-eye) bracket 617's recorded (36, 31, 24) region mean — instrument agreement.

## 3. THE PIXEL GATES — ALL PASS

Region stats (mean RGB, luminance L = 0.2126R+0.7152G+0.0722B, hi250 = fraction of pixels with L ≥ 250, clip = fraction with all channels ≥ 254), same rect on both images:

| pose · region (rect x0,y0→x1,y1) | 617 BASE | 622 NEW | ratio / verdict |
|---|---|---|---|
| hall-eye · floor centre (1000,620→1750,780) | (31.6, 26.7, 20.4) L 27.3 | (101.9, 88.9, 71.2) L 90.4 | **×3.31 ≥ 2 ✅** |
| hall-eye · floor wide (700,560→2050,780) | L 21.1 | L 89.2 | **×4.22 ✅** |
| hall-above · floor W of table (700,300→1100,600) | L 13.6 | L 112.6 | **×8.3 ✅** |
| hall-above · floor S of table (1150,550→1500,750) | L 8.8 | L 123.3 | **×13.97 ✅** |
| hall-eye · commander (1600,380→1680,450) | L 11.2 | L 56.7 | **×5.05 — DISCERNIBLE ✅** (green skin + blue pads readable at the table from eye 334; table legs, parchment distinct) |
| hall-eye · torch pool L (1020,30→1180,220) | R/B 1.49 | (149.9, 134.0, 109.0), **R/B 1.375** | **> 1.3 warm ✅** |
| hall-eye · torch pool R (1560,30→1720,220) | R/B 1.57 | (149.9, 133.1, 102.4), **R/B 1.464** | **> 1.3 warm ✅** |
| entrance · sun treads L (300,350→1050,600) | L **182.1**, hi250 0.00%, clip 0.000% | L **143.1**, hi250 0.00%, clip 0.000% | **sun faces DARKER, zero clip — no new blowout ✅** |
| entrance · sun treads R (1800,350→2500,600) | L 194.4, clip 0.000% | L 169.3, clip 0.000% | **✅** |
| entrance · whole frame | clip 0.001% | clip 0.007% | rise = interior torch-pool cores now visible THROUGH the gate (gate region clip 0.075%, hi250 0.98%) — not sun-face clipping; sun regions strictly darker |

**Reading:** D1+D2 (×7.5 cd, radius 1,900) lift the interior floor 3–14× while D3's LocalExposure compresses the sun-lit exterior DOWN (~−20% on the treads) — "black floor beside blown walls" was attacked from both ends exactly as CR-R3 designed. The corridor's own torch light is now visible from outside the gate (the entrance capture shows lit sconces + warm corridor walls where the baseline had a black hole). Wall pool cores peak ~1–3% hi250 in-pool with sub-0.01% hard clip — the 620 §4 hotspot watch item lands inside the gates.

**Floor-repair capture deltas (619 landed first, noted per spec):** the manifest repair is collision-only — no visual geometry change in any frame; tuft placement differs per-seed (709317761 vs the baseline's 1943988097), which shifts the entrance tread-band green content slightly. Nothing else.

**G2 note:** these gates are the floor of acceptability, not the ceiling — Jonathan's eye sits in judgment after this wave (the A-PPV question stays closed unless HIS verdict reopens it; on these pixels Wave 2 is not indicated by the instrument).

## 4. THE COMMIT (lighting lane)

HEAD verified = `1b66c4e` (my own TASK-619 wave commit — the wave record is already carried; no check-and-carry needed). Staged BY EXPLICIT PATHSPEC: `Config/DefaultEngine.ini` (±2 LocalExposure lines) · `Content/Blueprints/BP_Torch.uasset` (§25b oid `451924C9…7AA0`) · `handoffs/TASK-620-programmer.md` · `handoffs/TASK-622-entrance.png` / `-hall-eye.png` / `-hall-above.png` · this handoff. ⛔ Not staged: the two TASK-623 source files (the 625 lane), `handoffs/TASK-623-programmer.md`. ⛔ No push. Hash in the orchestrator report + Slack.

## 5. LEDGER

- **Writes:** this handoff + the 3 PNGs beside it + the commit; transient `Saved/TASK-622/*.b64.txt` capture dumps deleted at exit. No asset edited, no save issued, no compile, Simulate-only (no pawn, no input surface — the TASK-571+552 latch untouched by construction).
- Two SIE sessions this occupancy (619's nav + 622's captures), both read within their first ~30 s; both stopped cleanly.
