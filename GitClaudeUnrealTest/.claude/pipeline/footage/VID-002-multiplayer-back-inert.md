# Footage Review — VID-002
Video: testvideo/GitClaudeUnrealTest (64-bit Development PCD3D_SM6)  2026-08-28 17-29-24.mp4  ·  9.53s 2496x1440 @ 29.209 avg fps (probe; VFR flagged — all timestamps ±interval/2). Autocrop: game canvas 992x576 at offset (4,0) — Game Bar corner capture, manifest records crop=992:576:4:0.
Reviewed: 2026-08-28 · Jonathan: "I just dropped a new video, the 'back' button in the multiplayer screen doesn't work, so lets fix that."

## Symptoms (his words → what the pixels show)
S1 — "the 'back' button in the multiplayer screen doesn't work": the Multiplayer panel (title "Multiplayer", hint `127.0.0.1:7777`, Host / Join / Back, status "Ready - host a match or join an address." — all text verified via 2x crop) opens at ~6.3s and is still on screen at the last sampled frame (~9.40s; clip ends 9.53s). The Back button renders BOTH hover and pressed visual states — three full press-release cycles measured — yet the panel never closes and the main menu never returns. This is the MULTIPLAYER screen, not the deck builder's renamed Exit (d8bfd23): panel title reads "Multiplayer" with Host/Join/IP field.

## Timeline of findings
Method note: beyond the ≤40-Read image passes, a 102-consecutive-frame run (6.00s→clip end, tool `--run`) was measured numerically (mean luminance per button region per frame, scratchpad script — measurement only, no project writes). Baselines: button idle 187–188, panel-bg probe 87.6, status line 135.8. r# = run frame, t ≈ 6.0 + (r−1)/29.209 s, VFR ±.

| # | t | frame evidence | measured observation (pixels, not conclusions) |
|---|---|---|---|
| 1 | 0.0–6.0 | sheet_01 | Main menu up: Play (vs Bot) / Sandbox (No Bot) / Deck Builder / Multiplayer / Settings / Login / Quit; hover tints cycle across entries — main-menu hover rendering works. |
| 2 | ~6.31→6.34 | run r10→r11 | Panel-bg probe drops 154.5→87.6, status line 224.4→135.8: Multiplayer panel appears, main-menu buttons no longer visible anywhere in frame. The main menu's own Multiplayer click WORKED. |
| 3 | ~6.34–6.58 | r11 (promoted, full frame) | Join region +32.0 above idle; visually the brighter hover fill — cursor landed on Join's row the instant the panel opened. Panel hit-testing/hover alive from frame one. |
| 4 | ~6.68–7.20 | r21–23, r29–36; r33 crop (promoted) | Back region +32.0 — Back hover fill renders (2x crop: visibly brighter than Host/Join). |
| 5 | ~7.23–7.37 | r37–41; r38 crop (promoted) | Back region −20.0 below idle for 5 consecutive frames — darker pressed fill (2x crop confirms). **Click #1.** |
| 6 | ~7.71–7.81 | r51–54 | Back −20.0 again (4 frames), bracketed by +32.0 hover. **Click #2.** |
| 7 | ~8.12–8.19 | r63–65 | Back −20.0 again (3 frames). **Click #3.** |
| 8 | ~8.50→9.46 | r74–r102; r90 crop (promoted) | Cursor leaves Back (region returns to 187 idle). Panel-bg probe 87.6 and status line 135.8 UNCHANGED on every one of the remaining 29 frames — no close, no error text, no status change after three clicks. |
| 9 | ~9.40 | f00009_40s-c (promoted) | Last sampled frame: panel fully present, all buttons idle, status still "Ready - host a match or join an address." Clip ends 9.53s with no main-menu return. |

## Evidence frames (promoted)
- .claude/pipeline/playtest-evidence/2026-08-28/VID-002-t00m06s-panel-open-join-hover.png — panel just opened, main menu gone, Join showing bright hover fill (hit-testing alive).
- .claude/pipeline/playtest-evidence/2026-08-28/VID-002-t00m07s-back-hover.png — 2x crop: Back with bright hover fill (region +32 vs idle).
- .claude/pipeline/playtest-evidence/2026-08-28/VID-002-t00m07s-back-pressed.png — 2x crop: Back with darkened pressed fill (region −20 vs idle), click #1.
- .claude/pipeline/playtest-evidence/2026-08-28/VID-002-t00m09s-back-idle-after-clicks.png — 2x crop: Back back to idle gray after three clicks, panel unchanged.
- .claude/pipeline/playtest-evidence/2026-08-28/VID-002-t00m09s-panel-still-open-at-end.png — last sampled frame, panel still up, status line unchanged.

## Suspected mechanism — HYPOTHESIS, NOT VERDICT
- Back renders hover AND pressed (findings 4–7) → Slate is receiving and processing mouse press/release on that button; the failure is downstream of hit-testing. Named system: `Source/GitClaudeUnrealTest/Siegebound/SessionMenuWidget.cpp:142-145` — `HandleBackClicked()` → `BackPressed()`; auto-wired at `SessionMenuWidget.cpp:28-31` via `BackButton->OnClicked.AddUniqueDynamic` (BindWidgetOptional name match, `SessionMenuWidget.h:133-135`).
- `BackPressed()` standalone branch (`SessionMenuWidget.cpp:125-128`, contract at `SessionMenuWidget.h:62-68`): with no net session and no pending connection it logs "no session active; WBP handles panel dismissal" and returns — **no UI action**. No `RemoveFromParent`/`SetVisibility` exists anywhere in the file. Dismissal was a flagged decision assigned to the WBP (`handoffs/TASK-354-programmer.md:52`).
- But TASK-355 shipped `WBP_Multiplayer`... correction, `/Game/UI/WBP_SessionMenu` via route (A): named widgets, "the C++ base then auto-wires everything with zero graph work" (`handoffs/TASK-355-artist.md:122`). Zero graph work ⇒ the WBP has NO Back-dismissal graph either. **Hypothesis: contract gap — C++ deliberately punts standalone dismissal to the WBP; the WBP shipped with no dismissal graph; nobody closes the panel.** Confidence: high (pixels + both handoffs + code agree), but delegate firing itself is an inference from the pressed-state render — one instrumented click showing the `[SessionMenu] Back pressed - no session active` line on `LogSiegeNet` confirms the whole chain.
- The fix must be the INVERSE of the opening transition: the main-menu Multiplayer entry runs `RemoveFromParent(self)` → `CreateWidget(WBP_SessionMenu_C)` → `AddToViewport` (`handoffs/TASK-355-artist.md:138,360` — main menu is fully REMOVED from the viewport, not hidden), so Back must remove WBP_SessionMenu and re-create/re-add the main-menu WBP (same pattern Deck Builder's transition already uses).

## Routing recommendation
| finding | lane | suggested fix one-liner (hypothesis) |
|---|---|---|
| Back visually clicks, panel never dismisses (S1, findings 4–9) | gameplay-programmer (with manager ruling on WHERE) | Implement the standalone-dismissal half of the TASK-354 flagged decision: either graph-side in WBP_SessionMenu (Back → RemoveFromParent + CreateWidget/AddToViewport of the main-menu WBP, inverse of the TASK-355 open transition) or revisit the flagged decision and let `BackPressed()`'s standalone branch (SessionMenuWidget.cpp:125-128) drive dismissal from C++ (e.g. RemoveFromParent + a BIE the WBP answers). The 669 finding (anonymous AssignDelegate Back buttons) does NOT apply here — this Back is a named BindWidgetOptional button and its delegate path is real; the gap is the empty standalone branch + zero-graph WBP. |
| Confirmation instrument | gameplay-programmer | One PIE click on Back watching `LogSiegeNet` for `[SessionMenu] Back pressed - no session active; WBP handles panel dismissal` — if it prints, the hypothesis is confirmed end-to-end. |

## Not examined / limitations this pass
- No audio (FR-§5); no log overlay in frame — OnClicked firing is inferred from the rendered pressed state, not observed directly.
- 0–6s main-menu segment examined only at sheet level (hover cycling noted; no defect claimed there).
- VFR capture: all timestamps ±one frame interval; the three click windows are measured in run-frame indices, wall-clock is approximate.
- Whether Host/Join click-through works was not exercised in this clip (only their idle/hover states observed); the clip never enters a session, so the net-active Back branch (`LeaveMatch`) is unobserved.
- Capture compression can mimic rendering defects; irrelevant here — all claims rest on ≥3-frame stable region deltas, not single-frame texture.

Image Reads spent: 11 of ~40.
