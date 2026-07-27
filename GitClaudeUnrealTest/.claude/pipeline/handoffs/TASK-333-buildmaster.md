# TASK-333 — [GOLD-glow-int] DONE: two-point gauge verify in `L_Arena` Simulate + commit of the TASK-332 dial-back

**Status: done.** Date: 2026-07-27 · build-master. Verified in **Simulate** on `L_Arena` (never PIE-in-viewport), two sessions, exclusive editor. Committed on main (hash in the board line / Slack), **no push**. `L_Arena` never saved; every save-free — the only asset in the commit is TASK-332's already-saved `M_GoldGlow.uasset`.

## (a) FULL RESERVE — PASS

- **Independent blown-fraction re-measure** (fresh `CaptureAssetImage` on `SM_GoldNode`, TASK-332's exact mask method — per-row background model from L/R edge strips, subject = summed |ΔRGB| > 36, **mean-RGB luma** per the handoff's floor note): **9.1 % blown past luma 0.85** (subject coverage 27.3 %).
  - Method validation: my implementation re-measures TASK-332's own PNGs at **85.3 % before / 7.0 % after** vs their recorded 86.0 / 6.6 (audit anchor 86.3) — reproduces the anchor to < 1 pt, audit-comparable.
  - Gate: ≤ 15 % ✅ · CrystalTower reference 6.5 % · before-state 86.0–86.3 %. (Rec.709-raw deliberately NOT quoted — the 37.5 % lit-BaseColor floor makes it meaningless on this asset, per the TASK-332 floor note.)
- **In-arena** (`TASK-333-mine-full.png`, real arena sun, MID = 1.0): warm-yellow, unmistakably self-lit at gameplay distance, chunk boundaries / facets / dark crevices / base rubble all legible — **no cream blob**. `TASK-333-blue-side-full.png` frames a full mine directly against a large grey boulder: the grey-rock vs gold separation is unmistakable in-game.

## (b) DEPLETED — PASS (ember visible; no C++ remedy needed)

Reached through the REAL `Deplete()` path (drain tick hit 0): `IsDepleted=True`, MID `GlowIntensity = 0.0500` exactly. `TASK-333-mine-depleted-ember.png` — the depleted mine is **dimmer but plainly visible**, still reads as a warm-yellow formation, not black. Quantified (same pose, exposure-consistent — shadow-grass control 16.70–16.77 across all three shots): mine-region Rec.709 luma **58.3** at ember vs 80.4 (GI 0.23) vs 98.5 (GI 0.37) — the ember sits 3.5× above the shadow-floor. The TASK-332 watch is CLOSED: the 0.45×0.05=0.0225 effective emissive plus the lit BaseColor keeps the zero-UI signal readable; the sanctioned `GlowIntensityDepleted` C++ raise is NOT required. Jonathan's playtest eye stays final on the read.

## (c) GAUGE CONTINUITY — PASS (exact lerp at every sampled point)

Real runtime path end-to-end: bot-summoned RED miner registered at a mine (`GoldNode_3`, occupied), mine-side 1 s drain ticking; I stepped `GoldReserve` (the spec's "direct reserve set") and read the slot-0 MID after each tick. Predicted = `0.05 + 0.95 × Reserve/300`:

| Reserve (readback) | MID GlowIntensity | Predicted |
|---|---|---|
| 133 | 0.4712 | 0.4712 |
| 102 | 0.3730 | 0.3730 |
| 237 (up-drive) | 0.8005 | 0.8004 |
| 177 | 0.6105 | 0.6105 |
| 57 | 0.2305 | 0.2305 |
| 17 | 0.1038 | 0.1038 |
| 0 (Deplete latch) | **0.0500** | 0.0500 |

Exact match at 7 points in both directions + the depletion latch ⇒ the `GlowIntensity` param is live, found by name by `AGoldNode::UpdateGlowGauge`'s MID, and the dial-back did not touch the wiring. On-screen luma tracked monotonically (98.5 → 80.4 → 58.3, constant exposure).

## (d) ALL SIX MINES / TEAM-NEUTRAL — PASS

Two independent Simulate sessions (scatter randomizes mine positions per match; mirror law held both times — pairs at ±X): **6/6 mines** carry slot-0 `MID(parent=M_GoldGlow)`, `GlowIntensity 1.0000` at full reserve, identical readbacks. Both team sides glow identically: `TASK-333-blue-side-full.png` / `TASK-333-red-side-full.png` (mirrored full pair, same framing). Message Log clean across both sessions: ensure / Accessed None / Fatal / `LogOutputDevice: Error` = **0**; `Failed to compile Material` = **0** in-session (last hit in the log remains 22.33.20 — the pre-TASK-339-fix window of the prior art session).

## Follow-up observations (reported, NOT fixed — manager's call)

1. **Miner walk-stall on a corner-rise mine (session 1):** the bot's miner stalled at **713 uu 2D** from its target mine (that session's `GoldNode_5`, on the z≈232 corner rise), velocity 0, well before match-end froze it — it never reached the 150-uu arrival ring, so the mine was never mined. Looks like walk-acceptance/navmesh vs the rise (mine origin z 232–349 in some scatter layouts). Gameplay-lane follow-up candidate; NOT touched by this task.
2. Both unattended Simulate sessions ended with the RED bot destroying the undefended Blue castle in ~4–7 min — expected for Simulate (no player defense); noted as context only. The gauge drive ran in the post-match window via the documented "occupied mines keep draining after match end" accepted quirk — the mine-side drain/gauge tick is the real runtime path either way.
3. Transient sim-world nudges used (all died with the session): bot `StopDecisionTimer()`, destroy of 15 non-miner summons, one miner teleport. No editor-world change, no level save prompt appeared.

## Commit scope (this task's commit)

`Content/Materials/M_GoldGlow.uasset` (TASK-332's one-Constant dial-back 6.0 → 0.45) + `handoffs/TASK-332-artist.md` + `TASK-332-GoldNode-{before,after}.png` + this handoff + the five `TASK-333-*.png` + TASKBOARD/CONVENTIONS doc updates. Crumble materials deliberately EXCLUDED (TASK-338's commit).
