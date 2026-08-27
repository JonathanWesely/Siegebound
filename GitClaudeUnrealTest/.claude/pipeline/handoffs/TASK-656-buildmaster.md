# TASK-656 — [F1-1] VID-001 LOCALIZATION + MEASUREMENT DIAGNOSIS — live, read-only (build-master handoff)

**Status: COMPLETE — the site is LOCATED and MEASURED; the verdict is branch (i) EAST-FACE DISCOVERABILITY.** Date: 2026-08-27.
Editor: launched by this task (was measured DOWN; launching is not closing — the close law stays Jonathan's). PID **15772**, MCP `http://127.0.0.1:8000/mcp` green throughout (raw JSON-RPC lane, scratchpad `mcp_client.py`).
**STRICTLY READ-ONLY honoured:** no property write, no save, **no PIE** (never needed — see §5), no console/`M`, no pawn input, no compile, no git write, TASKBOARD untouched. The user's viewport camera was never moved (all captures via `CaptureViewport.captureTransform`).
`L_Arena.umap` SHA256 **ENTRY == EXIT == ledger**: `b3dbc5d9ae484a7bd02cafad52b4681da68b011477479b65ee7781ae459f8268`. Editor log: zero save events. PIE: never started (`IsPIERunning` false at exit). Level: `/Game/Maps/L_Arena`.
Laws: F1-R2/R3/R4/R5 · GH-R9/R11 · SIE-§1 (log read early, hash pasted) · the never-save law · CF-R3 · the latch law (UNSPENT) · the MCP tell-don't-fake law.

---

## 0. THE ONE-LINE RESULT

**The VID-001 site is the BLUE castle's EAST face — the d1 head-on line from spawn — NOT the south gate.** The hero stood on grass at castle-local **x ≈ +3699.5, y ≈ 0** (world **(−21300.5, 0, 0)**), stopped by the **`skirt_toe_01` east face at local x +3657.5 (world x −21342.5), top +506.5** — the DESIGN SEAL standing exactly where his pre-618 de-facto door used to be (626 §5 candidate 1, now CONFIRMED BY REAL INPUT). The "hip-height berm lip" in the frames is the **95-uu visual rim** of the berm skirt at that line; the actual stop is the invisible 506.5-uu seal above it (riser **+506.5 vs HeroMaxStepHeight 50, jump apex ≈ 183** — unmountable BY DESIGN). The legitimate south route was re-verified live: **16/16 hero-capsule stations EMPTY, riser tops 29/101.5/148/174 manifest-true** — had he found the south channel, he walks in. **Verdict: branch (i) discoverability. TASK-628 stays OPEN (F1-R5).**

## 1. LOCALIZATION — pixels → world (the camera-match evidence)

**Scene ground truth (live reads):** Castle_Blue `Castle_0` at (−25000, 0, 0) yaw 0 · Castle_Red `Castle_1` at (+25000, 0, 0) yaw 0 · PlayerStart (−23800, 0, 100) · DirectionalLight pitch −38, yaw 145 → **sun in the SE sky, shadows cast WNW**.

**Which castle: BLUE, by arithmetic before any capture.** Clip gold 68→79 at exactly +1/s over 11.46 s. Whatever the starting gold, the elapsed game time at clip start cannot fund the ≥46,000-uu walk from the Blue-side spawn to Red's gate at WalkSpeed 500. The hero was at his OWN castle.

**Which face — three independent lines of evidence, all naming the EAST face:**

1. **The lip profile is unique in the world.** Complex down-traces (≤50-uu steps) across the whole south approach and the east spawn line:
   - South approach, mouth line (y −3650→−3600, full width x −1500..+1500): one **~48–52-uu** visual step (knee height), then a smooth tan ramp 48→142 to the plaza. Flank lanes: knoll/crescent walls 245–468 (over head). **Nowhere a hip-height lip.** The gate-front is tan apron + paved plaza + brick steps — not the frames' green field.
   - East face, spawn line y=0: grass 0 out to x 3675, then a chartreuse rim stepping to **95–96**, rising 95→144 westward. The rim runs 92–101 uu tall across the face width (x 3600 line: y −1000..+800). **Grass underfoot + hip-height green lip + full width = the frames, exactly, and only here.**
2. **Shadow geometry.** Airborne-hero shadow in `VID-001-t00m07s` falls below-LEFT. Shadows cast WNW (light yaw 145). Convention verified against the plan capture's axis gizmo (UE left-handed: facing north → screen-right = WEST): a NORTH-facing camera (south gate) renders the WNW shadow below-RIGHT; a WEST-facing camera (east face) renders it below-LEFT. **Only the east face matches.**
3. **Composition.** Match captures from the hero's stop line reproduce the frame family: chartreuse berm field ahead, dark arcade openings in stone with brick coursing screen-left (south), green skirt mounds screen-right (north), grass at the hero's feet. The south-gate captures (tan ramp, symmetric knolls, paved plaza) do not resemble the frames.

**Matched camera transforms (world):** M1 `(−20900, 0, 260) pitch −10 yaw 180` · M2 `(−20850, −350, 270) pitch −10 yaw 195`. Captures beside this file:
`TASK-656-match-eastlip-M1.png` · `TASK-656-match-eastlip-M2.png` (the site) · `TASK-656-eastface-wide-P3.png` (wide) · `TASK-656-southgate-ref-P1.png` + `TASK-656-plan-south-approach.png` (the refuted candidate, for the record). Clip targets: `playtest-evidence/2026-08-26/VID-001-t00m05s-entrance-berm-lip.png` + `VID-001-t00m07s-entry-jump-blocked.png`.
(Editor viewport is ultra-wide 2758×796 and editor-exposed — darker than the game's auto-exposure; geometry, not palette, is the match basis.)

**THE NAMED SITE:** Blue castle EAST face, castle-local **(+3699.5, 0, 0)** hero-capsule stop centre (feet on grass z 0), facing the `skirt_toe_01` east face plane at local **x +3657.5** = world **x −21342.5**, y span of the engagement ≈ local −200..+400 (the lip rim he jumped at: local x ≈ 3645–3655, top ≈ 95). J1 is answered from pixels: **straight from spawn (east face)** — he never entered the south channel.

## 2. MEASUREMENT AT THE SITE (the 626 instrument, live)

All simple-collision results via native physics overlap (`find_actors` bounds + ObjectTypeQuery1–10); visual via complex `trace_world`. Hero anchors: capsule **Ø84×192 (r42/hh96)**, `HeroMaxStepHeight` 50, JumpZ 600 (apex ≈ 183).

| probe | result | manifest says | verdict |
|---|---|---|---|
| face bisection, 6-uu slabs z 200..300, y ±30 | EMPTY at x ≥ 3665 · **Castle_0 at x ≤ 3660** | `skirt_toe_01` east face **3657.5** | **live == manifest** |
| seal top, z slabs at x 3400..3600 | occupied ≤ 507 · empty ≥ 508 | ztop **506.5** | **live == manifest** |
| deep seal behind | `bay_seal_east_01` east face between x 3358..3370 at z 1000 | face **3360**, ztop 2430 | **live == manifest** |
| hero capsule walk-in, y=0, feet z 8..192 | FREE at centre x 3710 · **BLOCKED at 3690** | theoretical stop 3699.5 | the invisible wall, confirmed |
| the lip he jumped at | visual rim 95–96 at (3645..3655, −200..+400); collision at that height = the SOLID seal column 0→506.5 | — | **riser +506.5 vs 50 — unmountable by design** |
| skirt-top standability (the `BattlefieldScatter.cpp:1313` question) | **NO standable simple collision exists at the visual-rim height** — the column is solid to 506.5; the hero NEVER stood on the berm. The "standing/jumping ON it" pixels are jump arcs over the 95-uu visual rim (apex 183 > 95) plus his WNW shadow projected onto the berm surface, landing back on grass each time. | — | **pixels' standable read REFUTED; the 1313 comment is not contradicted at this site — rider stays PARKED (F1-R4; branch (i) opens no code lane)** |

**The legitimate route, re-proven live (the F1-R2 reconciliation):** centre lane x=0, capsule stations (84×84×184, feet on manifest support) at y −3700/−3600/−3500/−3300/−3000/−2700/−2500/−2200/−1950/−1700/−1550/−1300/−1000/−500/0/+700/+1300: **16/16 EMPTY** (the −3600 station's single Castle_0 return is a 2-uu probe-skin graze of tread_02's front corner — geometry of the probe, not a blocker; both neighbours EMPTY). Riser-top bisections: tread_01 **29** · tread_06 **101.5** · tread_09 **148** · hall_floor_02 **174** — byte-true to manifest v3. Worst chain riser 29 ≤ 50. **Grass → treads → sill → corridor → hall is hero-walkable end-to-end in the live world.**

**Pixels vs instruments — ON RECORD:** VID-001 does NOT contradict the 626/631/636 record — it **confirms** it. 626 §4 row d1 predicted this exact stop ("this is the face a straight walk from spawn hits", +506.5); 626 §5 named the experience ("every route he has ever used reads 'invisible wall'"). The first real-input attempt on record ran the predicted line and got the predicted refusal. The instruments were right; what failed is what they always said would fail: nothing tells the player the door is south.

## 3. VERDICT (F1-R2) — branch (i): EAST-FACE DISCOVERABILITY

- **(i) discoverability — YES.** The stop is a >50 DESIGN SEAL from the 626 table (d1), live-exact to the manifest, working as the mount-proof law intends. The south route is live-clear. The hero never found it: he spawned east, saw a friendly 95-uu green rim + an arch-like arcade shadow ahead (with the HP bar floating in it), and pushed the sealed line for the whole clip.
- **(ii) gate-blocker runtime — NO.** The hero never reached any gate; `GateBlockerVolume` never touched him. The GH-R11 PIE instrument was therefore out of scope (spec step 4 is gated on the site being the gate) and was not run — PIE was never started.
- **(iii) genuine lip/collision defect — NO.** Every probed face is byte-true to manifest v3; no rogue collision; the legit route measures clear at hero scale.

**F1-R5 adjudication — TASK-628:** the clip is at the EAST face ⇒ **TASK-628 stays OPEN.** VID-001 does not exercise the south-route walk and discharges none of GH-R11's (i)/(ii)/(iii) outcomes; the 30-second disambiguation walk protocol passes to TASK-660 (Jonathan's retry: open field south of the mound → south channel → ramp centre → straight north through the gate).

## 4. PRESCRIPTION → TASK-657 (INPUT, not authorization — F1-R2; owner pin = manager's activation ruling, branch (i) ⇒ art-director per the board)

**⛔ F1-R3 hard fence restated: NO seal is lowered.** Nothing on the east face (or any non-entry face) may become mountable; the gate stays the only opening. Nothing below asks for a collision or Source change.

Minimal-fix directions, cheapest-sufficient first (visual/UX only; zero Source diff expected ⇒ TASK-658 likely closes no-op and TASK-659 is commit-only, no compile):
1. **Signpost the south route from the spawn line.** The gap is between spawn and the channel mouth: path dressing (road/trample decal or material strip on the grass) sweeping from the spawn area south around the toe ring (the wrap must stay south of y −3692.5 — measured clear) to the channel mouth, plus a gate-ward marker readable from spawn (banner poles / torch pair at the mouth, x ±1470 line). The plaza/doorway already reads as a door once seen; it is never seen from spawn.
2. **Make the east-face refusal readable.** The 95-uu chartreuse rim visually invites a walk-up the collision refuses. Dress the d1 seal line so the visible cause matches the invisible wall (steeper silhouette / rock or masonry toe dressing along local x ≈ 3650, y −1000..+800). Scope note for the activation ruling: this touches the exterior mesh/material surface — the manager rules whether it rides 657 or parks to M7.
3. **J2 rides to Jonathan unchanged:** turning/moving the SPAWN to face the south approach is an `L_Arena` level edit = his own save grant (never-save law); default proceeds without it.

## 5. J3 FREE EVIDENCE (one property read, no diagnosis — the parked F2 row)

`Castle_0.HPBarWidget` (UWidgetComponent): RelativeLocation **(0, 0, 9450)** on the castle root ⇒ world anchor **(−25000, 0, 9450)**; Space **Screen**; DrawSize **256×32**; instance WidgetClass None (class supplied by the BP CDO — `WBP_CastleHealthBar`, Castle.h:614); bHiddenInGame false. Logged as-is for the J3/D-BAR lane.

## 6. FENCES / LEDGER / EXIT STATE

- **Writes this task:** this handoff + 5 capture PNGs beside it (`TASK-656-*.png`, ~11 MB). Nothing else — no asset, no board, no config, no git. Pre-existing dirt untouched: TASKBOARD.md (orchestrator) + `handoffs/TASK-655-buildmaster.md` (untracked).
- **Engine work was queries only:** ~700 complex traces · ~60 physics overlaps · actor/component/property reads · viewport captures from explicit poses (user camera untouched). No PIE, no console sentence, no `M` (latch UNSPENT), no pawn input.
- `L_Arena.umap` entry == exit == ledger `b3dbc5d9…e459f8268`; editor log shows zero save/SavePackage events; expected dirtiness **0** — decline any save prompt.
- **Editor left UP as required: PID 15772**, MCP green, level `/Game/Maps/L_Arena`, PIE not running — ready for the 657 fix lane and Jonathan's TASK-660 retry.
- Instrument lane note: MCP driven via raw JSON-RPC (scratchpad `mcp_client.py`, chunked SSE reader added this task); `trace_world` is COMPLEX (visual) — every collision claim above is from native physics overlaps, never from trace_world.
