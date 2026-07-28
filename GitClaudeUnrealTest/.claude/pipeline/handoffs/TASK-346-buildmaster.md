# TASK-346 handoff — [CMD-int] group-orders integration: compile + host TASK-345 + PIE matrix + ONE feature commit (build-master)

**Date:** 2026-07-27/28 (overnight window, Jonathan's editor grant) · **Verdict: MATRIX PASS — feature committed.**
**Phase A:** TASK-344 C++ compiled GREEN (16 s, 0 errors / 0 warnings) after a graceful no-save editor close (Save Content dialog declined; `quit_editor()` over the TASK-297 remote-exec lane — zero assets saved). Editor relaunched PID 6460; TASK-345 was hosted in this session (its own handoff).
**Phase B:** full PIE matrix on `/Game/Maps/L_Arena` — REAL PIE (player pawn possessed, bot playing), driven by the SummonTestUnit cheat lane + UE python remote-exec readbacks + a PostMessage input driver (desktop was LOCKED — SendInput unavailable per the TASK-076/112 record; posted WM input worked throughout). Verification is numeric-first: pick/marker circles read back as DecalActors (radius = DecalSize.Y, stage identity = the `StageTint` MID param), units via the `CommandGroupId`/`GroupStationOffset`/`State`/`CurrentTarget` instance properties, flow via the prompt/refusal/formation/prune log lines.

## Matrix results, row by row

**(a) FLOW — PASS.**
- R opens the pick (prompt logged); F opens the IDENTICAL flow as AMBUSH (IA_CmdAmbush + F mapping live — the TASK-345 asset works; "AMBUSH:" prompts logged).
- Three sequential cursor-following circles: active decal tracks the cursor trace tick-by-tick (readback loc follows the trace point); each stage seeds its OWN default — Select 1200 / Position 700 / Attack 1500 ✓.
- Wheel resize: exactly ±100/notch (GroupRadiusWheelStep); clamps observed numerically at **200** (min) and **5000** (max); earlier circles stay visible (2-circle and 3-circle states read back + screenshots).
- Sky cursor: trace MISS ⇒ circle `hidden=True` + LMB silently ignored (stage held) ✓.
- Empty stage-1 selection: refused BOTH types with the log+HUD reason — "HOLD/AMBUSH stage-1 select refused — no eligible unit inside the circle (radius 1200; staying in the pick)" ✓.
- Stage prompts ladder logged verbatim: circle-your-units → "6 unit(s) selected — place the POSITION zone" → "place the ATTACK zone" → "HOLD/AMBUSH set: 6 unit(s)" ✓.
- After stage 3 the Position+Attack circles PERSIST as markers; the Select circle is destroyed ✓. Formation log: "HOLD group N formed — 6 unit(s), position (…) r=700, attack (…) r=1500".

**(b) TIERS — PASS.**
- Stations: 6 DISTINCT sunflower offsets matching R·√((i+0.5)/N) golden-angle EXACTLY (202/350/452/~534/606/668 for R=700; one nav-projection nudge) — computed once, pushed as per-unit scalars.
- **30 s no-jitter:** all 6 station positions INTEGER-IDENTICAL across a 30 s window, State=Idle, zero re-path — the TASK-280/282 freeze class is absent.
- Tier-1 attack-zone engage: Red placed in the attack disc → all 6 acquired within one 0.25 s tick, converged, killed, RETURNED to stations (Idle, tgt=None).
- Tier-2 position-zone engage: same, from the position disc.

**(c) LEASHES + ESCALATION — PASS.**
- HOLD leash: engaged live Ogre yanked outside BOTH discs → every unit dropped the target THAT tick (tgt=None readback with the Ogre alive in the same frame) and returned to stations. No chase.
- AMBUSH leash-exemption: identical yank with an AMBUSH group → all 6 KEPT the target and pursued far outside both zones, chased to the kill, then returned to stations ✓.
- Monotone escalation: with a LIVE position-tier target held (tanky Ogre), a Red dropped into the attack disc flipped all 6 to it within 1 s while the Ogre stayed alive; no flip-back observed; on the attack-tier target's death the units re-acquired the position-tier Ogre (ladder re-acquire). No target ping-pong in any sample all session.

**(d) RELEASE — PASS.**
- T: groups cleared + BOTH markers destroyed + `CommandGroupId=-1` + offsets zeroed, instantly.
- STEAL: R pick circling an already-grouped 6 → new group formed, old group synchronously pruned at the confirm ("unit group 2 emptied — group and markers removed (TASK-344 prune)"); GroupIds strictly increasing (1→2→3 — never reused).
- All-members-dead: markers reaped by the +0.5 s poll (≤ 1 s law met; prune log present).

**(e) CANCELS / TEARDOWN — PASS (see Esc note).**
- RMB cancel verified at stage 1, stage 2, and stage 3: every pick circle destroyed, live-group markers untouched each time.
- Hero-death teardown: observed ORGANICALLY in match 1 — the bot killed the hero mid-pick; log: "hero died during a group-order pick — cancelling the pick (groups/stance unchanged)". Clean.
- Match end (winner Red, real castle kill, match 1): controller teardown ran clean. Direct `HandleMatchEnd` with an IN-FLIGHT pick + live group: pick torn down, **group + markers SURVIVED match end** (flagged decision 7 behavior) ✓.
- Play-Again (`HandleMatchReset`, called twice this session): groups + markers fully cleared, units released to the legacy body, prompts cleared ✓.
- PIE stop with a LIVE stage-2 pick + live group (accidental, see Esc note): EndPlay teardown clean — no ensure/crash.
- **Esc note:** in-viewport PIE the EDITOR consumes Escape as End-PIE before the game's poll sees it — the Esc cancel lane is untestable (and unusable) in PIE; RMB is the proven cancel. Esc matters only for packaged builds. Flagged below for Jonathan.

**(f) NO-REGRESSION — PASS.**
- Legacy pre-command: fresh summons march A→B at full speed (observed repeatedly).
- Stances: T latch + release-then-march verified TWICE (mid-advance AND station-idle-group shapes) — units march the enemy castle after T. E/Defend not re-run this session (bodies byte-identical per QA; Defend near own castle is indistinguishable from idle at the staging spot).
- Bot/Red untouched: bot played real matches throughout (rushed and WON match 1 — killed the hero, crumbled and destroyed the Blue castle; kept summoning Knights/Longbowman/Miners in match 2). Red units never grouped (`grp=-1` always).
- Miners + Siege/Support: Red miners ran their mining path; Blue Miner/Ogre/Cleric each ran their own profile AI ungrouped.
- **Eligibility negative test:** a select circle containing ONLY Blue Miner+Ogre+Cleric (all inside R=1200) REFUSED — "no eligible unit inside the circle" — Standard-only law enforced live.
- Card placement intact: "placement mode entered for card 'Longbowman'" via the One key; **R during placement silently ignored** (no prompt, no decal — mutual exclusion both directions; BeginGroupPick-while-placement ignore verified live, the reverse direction by the same session's pick+One... [R-side observed; placement-side enforced by the same guard set QA verified]).
- Wheel INERT outside the flow (3 notches, no pick: zero decals, zero errors).
- WASD: W moved the hero +123 uu forward — the TASK-345 IMC edit did not break move/look (the `InputModifier*` load warnings at IMC save time are benign: 21 original entries byte-identical per the artist readback, movement live in PIE).

**(g) LOGS — CLEAN.** Whole-session sweeps: `Ensure condition failed` = 0 · `Accessed None` = 0 · `Fatal error` = 0 · `Failed to compile Material` = 0. Deleted-getter grep (`GetHoldLocation|GetHoldRadius`) = 0 on load (TASK-345's check, re-confirmed by zero BP errors all session). Only noise: pre-existing audio-device churn, TASK-265 SpawnZ diagnostics, TASK-015-class `MoveToActor … failed` nav warnings produced by MY harness yanking a target to an off-navmesh spot (the TASK-328 precedent — not a feature defect; re-staged on-lane and the chase ran).

## WATCH (non-blocking, for Jonathan's playtest)
1. **One-time non-reproduced idle:** in the FIRST PIE session, after a T release of a station-idle group, the 6 units sat Idle (~2 min) instead of marching under stance-Attack. TWO dedicated reproduction probes in session 2 (T mid-advance AND T on a station-idle group — the exact original shape) both MARCHED correctly, and every subsequent order always re-engaged the units. Recorded as a watch: if units ever sit after T/E, grab the log.
2. **Esc stops PIE** (editor-owned) — in-PIE playtests must cancel picks with RMB.
3. **Stage-tint readability:** the tints are numerically live (white / 0.2,1,0.3 / 1,0.35,0.2 readback on the MIDs) but the multiply against the blue-dominant M_SpellReticle ring renders SUBTLY — green reads cyan-ish, red dims the ring. Screenshots attached. If Jonathan wants unmistakable per-stage colors, a small material tweak (e.g. desaturated/white base ring so the tint owns the hue) is a TASK-345-followup-sized change.

## Feel-pass items for Jonathan (from the board spec + ruling 12)
- Wheel sensitivity: step 100/notch (GroupRadiusWheelStep) — felt right under the harness; real-mouse feel is his call.
- Circle readability: base ring EXCELLENT up close (see select-ring shot); grazing angles at eye-level camera make distant rings thin — see the tint WATCH above.
- Spread feel: sunflower spread fills the position circle evenly (spread shot); 150 uu arrival tolerance leaves a natural loose formation.
- The six tunables (GroupRadiusWheelStep 100 / Min 200 / Max 5000 / Select 1200 / Position 700 / Attack 1500) all live on the controller for his pass.
- AMBUSH-vs-building semantics: default shipped = besiege until destroyed (chase-to-the-kill has no structure carve-out). NOT explicitly exercised vs a building this session — worth one playtest look.
- HUD prompt pixels: prompts are broadcast + logged and WBP_HUD binds them (TASK-345, compiled clean); the on-screen pixel look closes on Jonathan's eye per the UMG lesson.

## For the manager
- QA WARN doc-sync (from `qa/TASK-344.md`): amend CONVENTIONS "Group orders" eligibility wording to "not match-end frozen; a resumable spell freeze does NOT exclude" (spell-frozen units ARE select-eligible — accepted ruling 8). Doc action only, still open.
- The two optional NITs (prune-timer clear in EndPlay; seed-radius clamp) remain optional polish — not routed back.

## Verify shots (this folder)
- `TASK-346-verify-select-ring.png` — SELECT stage, white ring R=700 around the 6 Footmen, hero at the edge.
- `TASK-346-verify-two-circles.png` — dropped Select + active Position circle.
- `TASK-346-verify-spread-stations.png` — group formed; units dispersing to sunflower stations inside the green position marker.
- `TASK-346-verify-tier1-engage.png` — attack-zone engage (units breaking toward the fresh Red spawns).
- `TASK-346-verify-ambush-chase.png` — AMBUSH pursuit after the out-of-zone yank.

## Commit
ONE feature commit on main (ruling 11): the five TASK-344 source files + `IA_CmdAmbush` + `IMC_Hero` + `WBP_HUD` + `M_SpellReticle` (TASK-345 did the optional StageTint) + board/CONVENTIONS + TASK-344/345/346 handoffs + `qa/TASK-344.md` + the five verify shots. Explicit pathspecs; TASK-342 art-lane files (`Tools/ArtPipeline/pipeline_manifest.json`, `Content/RawAssets/`) deliberately excluded; `L_Arena` never saved; NO push. Hash in the 🔧 Build & Git post.

## Harness notes (reusable)
- Input on the locked desktop: PostMessage WM_KEYDOWN/WM_LBUTTONDOWN/WM_MOUSEWHEEL to the editor HWND works (SendInput does not — TASK-076/112 confirmed again). Wheel notches merge under background throttle — pace ≥400 ms/notch for exact counts.
- Cursor aiming fully in-engine: `project_world_location_to_screen` → `set_mouse_location` → `get_hit_result_under_cursor_by_channel` round-trip (aim at GROUND z, not actor z — elevated aim points overshoot the trace). Viewport→desktop mapping: two-point calibration (this session: desktop = viewport + (19,343), 1:1 physical px; PS must SetProcessDPIAware).
- `Shot` console command captures the real PIE view to `Saved/Screenshots` (MCP CaptureViewport renders the EDITOR camera even during PIE — wrong lane for PIE evidence).
- Remote-exec `bRemoteExecution` reverts on editor restart — re-flip via MCP ObjectTools set_properties on `/Script/PythonScriptPlugin.Default__PythonScriptPluginSettings`.
