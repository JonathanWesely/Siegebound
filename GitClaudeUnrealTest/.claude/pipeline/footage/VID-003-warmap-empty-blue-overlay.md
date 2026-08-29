# Footage Review — VID-003
Video: testvideo/GitClaudeUnrealTest (64-bit Development PCD3D_SM6)  2026-08-29 12-38-23.mp4  ·  25.69 s 2496x1440 @ 29.174 avg fps (probe; VFR flagged — all timestamps ±interval/2, sheets ±0.27 s)
Reviewed: 2026-08-29 · Jonathan: "look into the latest video dropped in, you can see that when you open the map, you only get a blue box with nothing in it, when you try to click around there is some text that comes up, but its really hard to make out what is going on."

## Symptoms (his words → what the pixels show)
- S1 "blue box with nothing in it" → the war map renders as a FULL-SCREEN semi-transparent dark blue-grey tint laid over the live 3D scene (world, player, table remain visible through it). No bounded panel, no border/frame, no parchment/background brush, no grid, no place markers, no labels. The only map content anywhere on screen is ONE small blue square dot (~8×8 px + dark outline) at the far-left edge, orig ≈ (72, 733) = 2.9% width / 50.9% height.
- S2 "when you try to click around there is some text that comes up" → on the first empty-area click (between 19.4 and 19.8) the bottom-center status line changes from "War map" to, verbatim: **"Click a marked place to add its name to the console."** It never changes again through end of clip despite continued cursor movement (frame-delta inference: cursor at different positions each ~0.5 s tile, 20.3→24.6).
- S3 "really hard to make out" → the status line is small white text drawn DIRECTLY OVER the unit deck bar (it overlaps the "Footman/Archer" card titles at ~y 1015/1154 displayed); the top-right button label is clipped: large text "Rev" spills OUTSIDE the left edge of its grey button, the button text reads "…eal Enemies (30 G…" cut off by the overlapping "Close" button, and the pair sits on top of the Rally HUD (the "0/6" shows beneath the buttons).
- (Bonus, pre-map) At 12.2 a centered white line reads verbatim: **"Walk up to your commander in the castle to use the war map"** — a refused open while away from the commander. Player then walks to the war table and the map opens at ~17.1.

## Timeline of findings
| # | t | frame evidence | measured observation (pixels, not conclusions) |
|---|---|----------------|-------------------------------------------------|
| 1 | 00:00.0–05.9 | sheet_01 | Outside castle; Gold 34→39 at +1/s; deck bar 5×Footman 9g + Archer 12g; Rally: Ready +1/s 0/6. No console box visible in any tile. |
| 2 | 00:06.4–11.8 | sheet_02 | Player enters castle interior. No console box, no typed sentence anywhere on screen. |
| 3 | 00:12.2 | f00012_20s (promoted t00m12s) | Centered text verbatim "Walk up to your commander in the castle to use the war map". Gold: 46. Commander (green pawn, blue overhead bar) seated at distant war table. No overlay, no console. |
| 4 | 00:16.8–17.0 | f00016_80s (promoted t00m17s) | Player at table beside commander. The PHYSICAL table top is a blank tan rectangle (no painted map). Gold: 50. |
| 5 | 00:17.2 | f00017_20s (promoted t00m17s map-first-open-empty) | FIRST-OPEN STATE: full-screen translucent blue-grey tint over live scene; bottom-center status "War map"; top-right "…eal Enemies (30 G…" + "Close" buttons; exactly one blue square dot at orig ≈(72,733); zero place markers, zero labels. Gold: 51 (dimmed but legible). |
| 6 | 00:17.2 | …-reveal-button-clipped crop ×2 | Button label overflow: "Rev" rendered outside the button chrome left edge; right side truncated at "(30 G" by the "Close" button; "0/6" of the Rally HUD visible under the pair. |
| 7 | 00:17.2 | …-lone-ally-dot crop ×4 | The lone dot is a solid blue square with dark outline, ~8×8 px, isolated — no other dots or markers on the entire screen. |
| 8 | 00:19.4 | f00019_40s (read, not promoted) | Status still "War map"; arrow cursor at orig ≈(1325,591); Gold: 53. |
| 9 | 00:19.8 | f00019_80s (promoted t00m20s) | Status now verbatim "Click a marked place to add its name to the console."; Gold STILL 53 → the click moved no gold and inserted nothing visible. No console box appeared. |
| 10 | 00:20.3–24.6 | sheet_04 | Cursor at a different position in nearly every 0.54 s tile (frame-delta inference: repeated clicking/moving); status line unchanged; no marker, no console, no gold discontinuity visible. |
| 11 | 00:25.4 | f00025_40s (promoted t00m25s) | End of clip: map still open, same status line, cursor hovering near the Reveal/Close pair (orig ≈2142,185), Gold: 59. Gold sampled 51→53→53→59 across 17.2→25.4 = exactly +1/s accrual; NO 30-gold deduction at any sampled point (sheet tiles show no discontinuity either — low-confidence interpolation between samples). |

## The three dispatch-required observations (recorded, not judged — manager rules the TASK-579 instrument)
- **(a) Exact first-open appearance (17.2 s):** full-screen translucent dark blue-grey tint over the live world; NO bounded box/frame/background brush; ONE blue ally-colored square dot at far-left mid-height (≈2.9% W, 50.9% H); ZERO place markers and zero marker labels; status line bottom-center reads "War map"; top-right shows the clipped "Reveal Enemies (30 G…" + "Close" buttons overlapping the Rally HUD; Gold: 51 still ticking +1/s under the tint.
- **(b) Console sentence before the first M-press:** WITHIN THE CLIP — none. No assistant-console box, input field, or typed sentence is visible in any examined frame, before or after either open attempt (checked full-res at 12.2, 16.8, 17.0, 17.2, 19.4, 19.8, 25.4 and all 48 sheet tiles). CAVEAT the manager needs: the clip starts MID-SESSION (Gold: 34 at t=0.0, accruing +1/s), so a pre-recording console sentence cannot be excluded from pixels — and the status-line branch actually observed (see mechanism) is the code's WITH-snapshot branch, which per WarMapWidget.cpp exists only after the snapshot is allocated on the turn path.
- **(c) Status-line text at open:** "War map" (verbatim, 17.2–19.4). It is NOT TASK-579's no-snapshot line ("No place markers yet - send your commander one order in the console…", WarMapWidget.cpp:87) — that string never appears anywhere in the clip.

## Evidence frames (promoted)
- .claude/pipeline/playtest-evidence/2026-08-29/VID-003-t00m12s-proximity-refusal-line.png — centered "Walk up to your commander in the castle to use the war map", Gold: 46, no overlay.
- .claude/pipeline/playtest-evidence/2026-08-29/VID-003-t00m17s-commander-table-preopen.png — pre-open: player + commander at blank-topped war table, Gold: 50.
- .claude/pipeline/playtest-evidence/2026-08-29/VID-003-t00m17s-map-first-open-empty.png — first-open: full-screen blue tint, "War map" status, one dot, zero markers, Gold: 51.
- .claude/pipeline/playtest-evidence/2026-08-29/VID-003-t00m17s-reveal-button-clipped.png — ×2 crop: "Rev" spilling outside button chrome, label truncated at "(30 G", pair overlapping Rally "0/6".
- .claude/pipeline/playtest-evidence/2026-08-29/VID-003-t00m17s-lone-ally-dot.png — ×4 crop: the single blue square dot with dark outline.
- .claude/pipeline/playtest-evidence/2026-08-29/VID-003-t00m20s-empty-click-hint.png — "Click a marked place to add its name to the console." over the deck bar, Gold: 53 (unchanged across the click).
- .claude/pipeline/playtest-evidence/2026-08-29/VID-003-t00m25s-end-state-no-spend.png — clip end: map still open, Gold: 59, cursor near Reveal/Close, no spend ever registered.

## Suspected mechanism — HYPOTHESIS, NOT VERDICT
- Empty map + both observed strings ("War map" at open, "Click a marked place…" on empty click) → `UWarMapWidget::OpenMap()` WarMapWidget.cpp:413-420 and `NativeOnMouseButtonDown` WarMapWidget.cpp:876-883 both discriminate on `GetReadOnlySnapshot()`: null → the TASK-579 hint (line 87), non-null → exactly the two strings observed. ⇒ the assistant snapshot was NON-NULL for this whole open, yet `BuildMarkerRects` (WarMapWidget.cpp:667-716) produced ZERO markers — the documented "markers empty WITH a snapshot present" case (comment at :871-873): every `Snapshot->ResolvePlace()` failed or `GetPlaceNames()` was empty. Consistent with TASK-580 (marker seeding) being deliberately held — but note this is NOT the WR-§9 row 12 shape (row 12 is the NO-snapshot state; CONVENTIONS.md:3417). Either a pre-clip sentence allocated the snapshot and the row-12 "self-heals the moment one sentence is sent" claim did not hold on markers, or something allocates the snapshot without a sentence. Medium confidence; confirmed by one log pull: presence/absence of `[WarMap] No assistant snapshot yet` (WarMapWidget.cpp:524-527) and of `[AssistantConsole] War map inserted` (SiegeAssistantConsoleWidget.cpp:1302) in this session's log would split the branch with certainty.
- "Blue box with nothing in it" appearance itself → WBP_WarMap (Content/UI/WBP_WarMap.uasset exists; binary, layout unverifiable in files) supplies ALL visual chrome; the C++ widget paints only dots/markers/labels (NativePaint, WarMapWidget.cpp:722-826) and no background. Hypothesis: the WBP's background is a full-screen low-alpha solid tint with no map-panel art, so with zero markers the player sees only tint + chrome. Art/UX gap rather than logic fault. High confidence on "the emptiness is content-absence, not a failed draw" (the one ally dot and all chrome DID draw); low confidence on WBP internals.
- The lone blue dot → ally-dot painter (WarMapWidget.cpp:767-774) drawing `AllyDotsWorldXY` in the shipped health-bar blue (`GetDefaultBlueBarColor`, :759). Hero + commander stand at near-identical world XY inside the castle at the arena's west edge → one (overlapping) dot at left edge, mid-height. Hypothesis: the dot is CORRECT projection, not a defect. Rally 0/6 = no summoned units = no other dots. Confirmable by summoning one unit and watching a second dot.
- "Reveal Enemies (30 Gold)" label clipped + pair overlapping Rally HUD → WBP_WarMap top-right layout: button too narrow for its label and anchored over the Rally readout. Cosmetic/layout defect in the binary WBP. High confidence it is layout, cannot name the slot from files.
- Status line drawn over the deck bar → WBP_WarMap bottom-center placement collides with the always-visible deck bar. Legibility defect matching "really hard to make out". High confidence on the collision as pixels; layout fix is in the WBP.
- 12.2 refusal line → `BroadcastRefusal(… "Walk up to your commander in the castle to use the war map")` SiegePlayerController.cpp:4797 (WarMapRefused_OutOfRange), the WR-§5 proximity gate working as designed. Not a defect.
- No 30-gold spend → gold 51→59 strictly +1/s; either the Reveal button was never successfully clicked (its clipped label may have read as decoration) or a click failed silently. Cannot distinguish from pixels; the clip ends with the cursor near the button (25.4) before any observable outcome.

## Routing recommendation
| finding | lane | suggested fix one-liner (hypothesis) |
|---|---|---|
| Map is content-empty despite with-snapshot status branch (markers=0 with snapshot present; row-12 shape NOT observed) | needs-Jonathan → manager rules TASK-579 instrument + TASK-580 hold; then gameplay-programmer | Pull the session log for `[WarMap] No assistant snapshot yet` / `[AssistantConsole] War map inserted`; if snapshot existed sentence-less, instrument the allocator; else board TASK-580 (seed/resolve places) as the real repair. |
| No map background/panel art — "blue box" reading | art-director | Give WBP_WarMap a bounded map panel (parchment/arena outline) so the projection rect is visible even with zero markers. |
| "Reveal Enemies (30 Gold)" label overflows/clipped by Close; pair covers Rally HUD | art-director (WBP layout) | Widen/auto-size the button, move the pair off the Rally readout. |
| Status line overlaps deck bar (legibility — his core complaint) | art-director (WBP layout) | Re-anchor status text above the deck bar or give it a backing plate. |
| Proximity refusal line + empty-click hint + lone ally dot | none — record as working-as-coded (WR-§5, WR-§9 rows 1/12-adjacent) | No action; cite frames on the playtest sheet. |
| Reveal spend never observed (gold +1/s throughout) | needs-Jonathan | Ask whether he tried the Reveal button; a follow-up clip clicking it would test WR-§7 end-to-end. |

## Not examined / limitations this pass
- No audio (FR-§5); key presses (M, clicks) are inferred from UI responses, never observed directly.
- The clip starts mid-session (Gold: 34 at t=0) — everything before recording, including any console sentence, is outside pixel evidence.
- Whether the final cursor position near Reveal/Close (25.4) became a click — clip ends first.
- WBP_WarMap.uasset internals (binary); the exact tint alpha/color values (capture compression); gold values between full-res samples (sheet-tile interpolation only, low confidence).
- Extracted but not read: f00011_90s, f00015_80s, f00019_60s (superseded by adjacent frames).
- Image-Read budget used: 13 of ~40.
