# TASK-663 — [ROT-2] THE POST-ROTATION MEASUREMENT BATTERY (build-master handoff)

**Status: COMPLETE — read-only honoured; every mandate measured live. This record is the ACTIVATION RULING's sole input (RELAYED-DIAGNOSIS law — 664/665 recompute from HERE).** Date: 2026-08-27.
Editor: UP throughout, PID **28772** (as left by TASK-662 — never bounced, never closed). MCP green (raw JSON-RPC lane, session scratchpad `mcp_client.py`). Level `/Game/Maps/L_Arena` loaded at entry and exit.
Fences honoured: ⛔ no property write, no save (dirtiness 0 at exit, zero `LogSavePackage` Content events after 662's 22:05:16 — the log tail is pasted in §9) · ⛔ no console sentence / no `M` (latch UNSPENT) · ⛔ no pawn input (CF-R3 — three PIE/SIE occupancies, all input-free, all reads inside the SIE-§1 window) · ⛔ no compile · git READ-only · TASKBOARD untouched · the user's viewport camera never moved (all posed captures via `CaptureViewport.captureTransform`; the PIE pawn-view capture used the live game viewport without moving anything).
Laws: ROT-§2..§4 · WR-§2b · SIE-§1 · CF-R3 · the RELAYED-DIAGNOSIS law · the never-save law (RESUMED — under the ROT-§2 ledger).

**HASH GATE (ROT-§2): `L_Arena.umap` SHA256 ENTRY == EXIT == the NEW canonical ledger, exact:**
`9ccd54efeb0459df9ed15204fd7e5274797e5f6093f5504a5730c3c9d5ea0e58`
Git HEAD at entry and exit: `bd222c2` (no drift; porcelain unchanged apart from this task's own handoff+PNG writes).

---

## 0. THE ONE-LINE RESULT

**The hero spawns at world (−21007.8, 0, 98.2) — 292 uu OUTSIDE his own gate mouth, on the gate axis — facing yaw 0, i.e. 180° AWAY from the gate (JR2: he does NOT see it; the 665 conditional fix is LIVE, target facing = toward the own castle, ≈ yaw 180 Blue / 0 Red). The rotated approach walks clean end-to-end on BOTH castles (20/20-station battery, risers 29/101.5/148/174 byte-true, worst ≤ 50). Banners frame the mouth from centre-field but sit outside the FOV at the spawn line itself; TramplePath and ToeRocks landed on flank faces no approach uses. GateBlocker armed at the rotated centres; nav definitive ×2 sessions, 0 pending, 0 culls, 6/6 mines; the bot's wave anchor resolved LIVE at x 19965 — in front of Red's GATE.**

## 1. MANDATE 1 — THE WR-§2b SPAWN RESOLVER, LIVE (PIE, no input)

Three sessions this task: PIE [921] 22:19:39 · PIE [604] 22:21:02 (same numbers, reproduced) · SIE [630] 22:24:42 (captures/nav/bot). All measurements in each session's first minute.

**Branch record (log lines, session [921], reproduced byte-identical in [604]):**
- Branch-2 REFUSAL FIRED: `Warning: PlayerStart 'PlayerStart_0' at (-23800, 0, 100) lies INSIDE the Blue castle's colliding bounds (centre X -24999, half-extent 3692 x 3657 x 4041) — REFUSED, falling through to the castle-relative resolver.`
- Branch 3 resolved: `Castle-relative hero start for Blue: castle X -25000, measured colliding half-extent 3692 + clearance 300 => spawn distance 3992 (authored floor 1500) -> (-21008, 0, 100).`
- **The extent swap at the resolver itself:** pre-rotation sessions in the same log (21:24:07, 21:27:37) read half-extent **3657** → spawn distance 3957 → (−21043, 0, 100); post-rotation reads **3692** → 3992 → (−21008, 0, 100). The colliding X/Y halves swapped 3657↔3692 exactly as ROT-§4 predicts, measured by the shipped resolver.

**The pawn, read live (PIE world):** `BP_HeroCharacter_C_0` at world **(−21007.816, 0, 98.15)**, rotation **(pitch 0, yaw 0, roll 0)**. Castle-local (Blue frame, yaw +90): **(0, −3992.2)** — dead ON the gate axis, 292 uu outside the mouth plane (local y −3700 / world x −21300). z 98.15 = capsule settled on grass (feet ≈ 2.15). Capsule live-read: `CollisionCylinder` CapsuleRadius **42** / CapsuleHalfHeight **96** (Ø84×192, the anchor values).

**JR2 — WHAT HE FACES: yaw 0 = world +X = centre-field/enemy — his own gate is at bearing ≈180° DIRECTLY BEHIND him. He does NOT spawn seeing the gate.** Facing delta for the 665 conditional fix: **180°**. Live derivation for 665 (never a hardcode): target yaw = atan2(OwnCastle.Y − Spawn.Y, OwnCastle.X − Spawn.X) → Blue **180**, Red **0** — equivalently the branch-3 `TowardCenterline` sign negated. Standalone-safe note: with no castle, branch 3 never runs and nothing changes.
Pixels: `TASK-663-pie-spawn-pawnview.png` (the literal player camera at spawn: open field, scatter, RED's distant gate on the horizon dead-centre — no own castle in frame) · `TASK-663-sie-blue-spawn-eyeline-fwd.png` (posed at the spawn point, yaw 0) · `TASK-663-sie-blue-spawn-eyeline-back.png` (same point turned 180: the OPEN mouth fills the frame 292 uu ahead — treads, torch-lit passage, war table visible in the hall). Red mirror pair: `TASK-663-sie-red-spawn-eyeline-{fwd,back}.png` (Red hero prediction: branch 3 mirrored → (+21008, 0, 100) yaw 180, gate likewise at his back; no Red client exists in single-player PIE — Red rows are the same resolver arithmetic on the mirrored transform, not a live pawn read).

**Overlap-proof at the spawn (live, hero scale Ø84×192):** capsule box at the pawn contains ONLY `BP_HeroCharacter_C_0` itself — not GateBlocker (armed centre 2,417 uu away inside the passage), not tread/seal geometry. Underfoot slabs (feet −4..0 / −8 / −20): `StaticMeshActor_1`/`StaticMeshActor_33` (ground/skirt meshes) — **standable floor under him**. Spawn = CLEAN.

**Castle colliding AABBs, pasted (the swap):** editor `get_actor_bounds`: Castle_0 x −28690.91..−21306.54 (half **3692.18**), y −3657.15..+3656.43 (half **3656.79**), z −32..9450; Castle_1 the exact mirror. Resolver's colliding-only read (in-log): half-extents **3692 × 3657 × 4041**. Pre-rotation halves 3656.79↔3692.18 swapped byte-exact (656 §1 vs today).

## 2. MANDATE 2 — THE NEW-APPROACH WALK PROOF (both castles; the reusable battery)

Instrument: **`handoffs/TASK-663-battery.py`** (ProgrammaticToolset script; native physics overlaps, ObjectTypeQuery1–10; castle-local→world mapping for yaw ±90 derived in-file). **667 re-runs it byte-for-byte**; its raw output this run: `TASK-663-battery-result.json`. Editor-world run (level geometry; GateBlocker inert there by construction — its PIE-armed state is §4).

| probe (castle-local) | Blue (world = (−25000−ly, +lx)) | Red (world = (+25000+ly, −lx)) | verdict |
|---|---|---|---|
| pre-mouth stations y −3992 (the spawn lane) / −3850 / −3775, feet 8..192 | EMPTY ×3 | EMPTY ×3 | **spawn → mouth is open grass** |
| the 656 17-station centre lane y −3700..+1300 (feet on manifest support) | 16/17 EMPTY; y −3600 returns `Castle_0` | 16/17 EMPTY; y −3600 returns `Castle_1` | **the known 656 §2 2-uu probe-skin graze of tread_02's front corner, reproduced at the same station on BOTH castles; both neighbours EMPTY — not a blocker** |
| riser tops (slab bisection) at y −3600 / −2500 / −1750 / −1300 | occupied ≤ [27,30) / [99.5,102.5) / [146.5,149.5) / [172.5,175.5); empty above | identical | **tread_01 29 · tread_06 101.5 · tread_09 148 · hall_floor_02 174 — byte-true to manifest v3 in the rotated frame; worst chain riser 29 ≤ HeroMaxStepHeight 50** |
| old d1 seal face bisection (x 3665/3660/3657/3650, z 200..300) | EMPTY at 3665, occupied ≤3660 | identical | **`skirt_toe_01` face 3657.5 rode the rotation intact — now on the world-Y flank (§3)** |
| forward-of-spawn field y −4300/−4800/−5500 | EMPTY ×3 | EMPTY ×3 | **the direction he FACES is open field — no misleading walk-up face on the live approach** |

**Verdict: mouth → ramp → treads → gate passage → hall is hero-walkable end-to-end on BOTH rotated castles, and the spawn now sits ON that axis.** The 656/659 batteries are orientation-stale per ROT-§4; this is the new walkability record.

## 3. MANDATE 3 — F1 FURNISHING EVIDENCE (measurement only; dispositions are the manager's ruling)

Furnishing log, byte-form intact, BOTH castles, all three sessions: `F1 discoverability set — 2/2 banners, 13/13 path segments, 10/10 toe rocks spawned` + `furnished — 6 of 6 torch anchors spawned (cap 6), commander spawned`. CDO anchor arrays read live off `Castle_0` == the 661 §2 tables exactly (2/13/10 entries, values byte-equal). All castle-local ⇒ world positions under yaw ±90 (arithmetic verified on the plan-view pixels):

- **GateBanners (2) — anchor local (±1400, −3675):** world Blue **(−21325, ∓1400)** / Red **(+21325, ±1400)** — flanking each mouth on the NEW gate face. Pixels: `TASK-663-sie-{blue,red}-wide.png` (from the centre-field family pose (∓19200, 0, 1050)): **both crimson chevron banners frame the open mouth cleanly on the live approach** — correct by construction, confirmed. **Datum for the ruling:** from the spawn point ITSELF (292 uu out), ±1400 lateral ≈ ±77° off-axis — **outside the eye-line frame** (`-sie-blue-spawn-eyeline-back.png` shows the mouth without the banners); they read from approach/centre-field distance, not from the spawn line.
- **TramplePath (13 segments) — first segment local (3699.5, 0), last local (0, −3700):** world Blue first **(−25000, +3699.5)** (the NORTH flank, the old spawn-line grass), corner seg-6 **(−21300, +3699.5)** (NE corner), last/turn-in seg-12 **(−21300, 0)** — AT the mouth foot, its arrow (local yaw +90 → world 180) still pointing INTO the gate. Red the mirror: first **(+25000, −3699.5)** (SOUTH flank) → last **(+21300, 0)**. Pixels: `-sie-{blue,red}-plan.png` + `-sie-blue-northflank.png`/`-sie-red-southflank.png`. **Measured state for the ruling:** the chain's premise-origin (the old east-face stop lane) now lies on a flank no route touches; segments 0–5 decorate that flank; segments 6–12 happen to run along the new gate face toward the mouth; nothing about the chain starts at the ACTUAL spawn (which is 292 uu from the mouth on the axis — a guide path there would be ~zero-length).
- **ToeRocks (10) — local x 3650, y −1000..+800, z 95:** world Blue **y ≈ +3650, x −25800..−24000** (north flank); Red **y ≈ −3650, x +24000..+25800** (south flank). Pixels: the flank captures show the grey picket hugging the skirt on that flank. **Measured state for the ruling:** the picket dresses the old d1 seal line — a face the live spawn approach never presents (§2: the forward stations are open field; the seal face is 90° off every route). No misleading walk-up face remains on the live approach for it to warn about.

## 4. MANDATE 4 — GATEBLOCKER AT RUNTIME (PIE, property reads + armed-centre probes; no pawn input)

- Editor-world (inert ctor state, read pre-session): RelativeLocation (0,0,0), RelativeRotation zero, BoxExtent 32³ — both castles.
- **PIE (armed at BeginPlay via `ConfigureTeamGating`), read live on both instances:** RelativeLocation **(18, −1575, 852)** · RelativeRotation zero · BoxExtent **(900, 405, 678)** — the authored gate values, riding each castle's rotated transform.
- **Armed world centres (castle transform × relative):** Blue **(−23425, +18, 852)**, Red **(+23425, −18, 852)** — both on the NEW gate axis, centre-side. **Live probe proof:** an 80-uu probe box at each armed centre returns the castle (the blocker's team object channel answers the object-type overlap); the control box at the Blue mouth plane (z 260..340, outside the blocker band) returns EMPTY — the occupancy is the blocker, localized, not castle mesh.
- **Own-team clear / enemy-side armed:** the response matrix (object type = OWN team channel, Ignore all + Block ENEMY channel only) is authored in the same guarded `ConfigureTeamGating` block whose two other observables (relative location flip + extent flip) are read live above — plus the standing TASK-350 PIE-verified record and 659 §3b. Instrument residual, declared: `BodyInstance` is not readable through this MCP lane (returned null; §9) — the matrix itself was not property-dumped. No contrary evidence anywhere: own-team Red units path through their own gate in this log's sessions.

## 5. MANDATE 5 — NAV DEFINITIVE GATE (ROT-§3), reproduced in TWO fresh cycles this task

- PIE session [604-adjacent, BeginPlay 22:23:03] definitive at **22:23:06**: `Nav generation FINISHED … DEFINITIVE post-settle` → `Traversability CONFIRMED (nav settled: 0 pending) — 0 tile task(s) pending; Blue→Red castle path + 6 mine path(s) exist (after 0 cull(s)) [definitive: OnNavigationGenerationFinished]` ×2, with `nav-build [at-confirmation]: remaining=0 running=0 dirtyAreas=0 hasDirty=false activeTiles=446`.
- SIE session [630, BeginPlay 22:24:42] definitive at **22:24:44**: the identical pair, again `activeTiles=446`, again **0 culls, 6/6 mine paths**.
- **The parked mine-reachability row (659 §5.1): re-observed, did NOT fire** in any post-rotation session (the `Mine reachability unconfirmed` Error appears only in pre-rotation boots 20:57/21:09 in this log). Reported as the PARKED item it is — class unchanged.
- Timing note for 667: the deferred definitive re-check lands ~2–4 s after BeginPlay; my first two PIE cycles were stopped at ~8–11 s and one of them pre-empted the line — hold a session ≥10 s before grepping (the two reproductions above are both in-session observations).

## 6. MANDATE 6 — THE BOT ANCHOR (live)

- `ResolveCastleFaceDistance(FVector2D(1,0))` = Red colliding half-extent **X** = **3692.18** post-rotation (the ±X ray-exit; live AABB §1) + `BotCastleSpawnOffset` **1343.15** ⇒ **`ResolveCastleFrontAnchorOffset` = 5035.33** ⇒ anchor world x = 25000 − 5035.33 = **19964.67**.
- **Measured live in MY SIE session:** `Rule 4 (Attack): played unit 'Knight' (cost 18) castle-front (19965, 418, 20) — marching` (22:25:10, session [630]). Pre-rotation sessions in the same log placed at x **20000** (= 3656.85 + 1343.15 band); post-rotation placements read **19965** — the anchor moved with the swapped extent, and with Red's gate now on −X it materializes **in front of Red's GATE face**, exactly the 662 §8 prediction. (Wave-anchor truth-change datum for 665's SiegeBotController comment adjudication.)

## 7. MANDATE 7 — FREE EVIDENCE (one property read each; NOT diagnosed — the parked F2 lane)

- `Castle_0.HPBarWidget` (PIE instance): RelativeLocation **(0, 0, 9450)**, DrawSize **256×32** ⇒ world anchor (−25000, 0, 9450) — on the rotation axis, unmoved by yaw.
- Hero `HPBarWidget` (PIE instance): RelativeLocation **(0, 0, 120)**, DrawSize **90×22** — rides the pawn.
- Unsolicited observation, logged not diagnosed: the PIE viewport carried the dev overlay `Video memory has been exhausted (450.516 MB over budget)` (visible in `TASK-663-pie-spawn-pawnview.png`) — the known standing VRAM condition on this machine, editor-session-scoped; no new class of finding.

## 8. SANITY (mandate 5's tail)

Commander + 6/6 torches spawned per the §3 log lines, both castles; torch glow visible in the passage in `-sie-blue-wide.png` (Blue sunlit) and the war table in both `-eyeline-back` frames; Red's mouth self-shadowed exactly as the 662 record says a −X-facing mouth under the SE sun must be. One wide capture per castle: `TASK-663-sie-{blue,red}-wide.png`. Castles pristine in every frame (captures at T+3..T+7 of session [630]).

## 9. INSTRUMENT RESIDUALS / DEVIATIONS — declared

1. `ControlRotation` is not readable through this MCP lane (SiegePlayerController property read refused). The pawn's actor rotation (yaw 0) IS the applied `StartRotation`, and `RestartPlayer`/`SetControlRotation` consume the same value by code — recorded as the facing datum. 667 re-verifies the same way.
2. `BodyInstance` not readable (null) — see §4; the response matrix rests on its two live observables + the code tie + the standing PIE-verified record.
3. Two PIE cycles ended before the deferred nav re-check fired (see §5 timing note) — both later cycles observed it in-session; no gap in the record.
4. The three PRE-rotation sessions visible in this log (21:09/21:24/21:27, plus 20:57) are Jonathan's/662's, not this task's; cited only as before/after comparators.
5. Save-event scan at exit (`LogSavePackage`, `Moving output`): last Content event remains 662's `22:05:16 /Game/Maps/L_Arena`; the only other is the 21:59:24 periodic autosave into `Saved/Autosaves/` (outside Content, gitignored).

## 10. EXIT STATE + FOR THE ACTIVATION RULING / 664 / 665 / 667

- Editor **UP PID 28772**, MCP green, level `/Game/Maps/L_Arena` loaded, `IsPIERunning` false, **dirtiness 0 — decline any save prompt**. Latch UNSPENT; no console sentence, no `M`, no pawn input ever issued; user camera untouched.
- Hash: entry == exit == ROT-§2 ledger `9ccd54efeb0459df9ed15204fd7e5274797e5f6093f5504a5730c3c9d5ea0e58`.
- Writes this task: this handoff + 11 PNGs + `TASK-663-battery.py` + `TASK-663-battery-result.json` (all in `handoffs/`) — nothing else.
- **For the ruling, the measured cores:** (a) banners CONFIRMED framing the mouth from the live approach (and NOT visible from the spawn line itself — distance datum §3); (b) TramplePath: premise-origin on a dead flank, only its final mouth segments coincide with the live route, the actual spawn stands 292 uu from the mouth; (c) ToeRocks: dress a flank face 90° off every live route; no misleading walk-up face exists on the live approach (§2 forward stations EMPTY); (d) spawn-facing: JR2 condition MET (facing away, delta 180°) — 665's conditional fix is live, derivation in §1.
- **For 667:** re-run `TASK-663-battery.py` byte-for-byte and diff against `TASK-663-battery-result.json`; hold any nav-observing session ≥10 s; the furnishing-log grep surface changes to whatever 664 declares.

Captures beside this file (11): `TASK-663-pie-spawn-pawnview` · `-sie-blue-spawn-eyeline-{fwd,back}` · `-sie-red-spawn-eyeline-{fwd,back}` · `-sie-{blue,red}-wide` · `-sie-{blue,red}-plan` · `-sie-blue-northflank` · `-sie-red-southflank`.
