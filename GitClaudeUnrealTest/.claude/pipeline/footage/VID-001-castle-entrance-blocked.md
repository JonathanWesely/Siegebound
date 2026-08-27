# Footage Review — VID-001

Video: testvideo/GitClaudeUnrealTest (64-bit Development PCD3D_SM6)  2026-08-26 18-16-21.mp4  ·  11.46 s 2496x1440 @ 29.53 fps avg (probe; **VFR capture — all timestamps ±0.25 s**; game content occupies a 992x576 window, autocrop applied)
Reviewed: 2026-08-26 · Jonathan: "Review this capture; flag anything that looks visually broken or off; there is no single known issue — this is a general pass."

DRY RUN — first dispatch of the footage-review lane. General pass, no target symptom given.

## Symptoms (his words → what the pixels show)

- S1 "anything visually broken" → the hero walks to the castle entrance, then spends 5.6–7.7 s repeatedly jumping at/on a hip-height green berm lip that crosses directly in front of the gateway, and ends the clip standing outside. No frame shows him past the archway threshold.
- S2 → a solid light-blue bar floats in mid-air inside the dark archway in **every frame** of the clip, with no visible owner, border, background, or fill boundary.
- S3 → at ~9.4 s a white mouse cursor appears mid-gameplay; at ~9.8 s a small blue ring marker appears on the grass at the cursor position; nothing else observable changes in the remaining ~1.6 s.
- S4 → the berm/skirt reads as untextured blockout: flat uniform chartreuse, faceted low-poly silhouette, hard zigzag lit/shadow seams — in strong contrast to the detailed grass texture beside it.
- S5 (minor UI) → adjacent card titles in the hand bar collide with zero gap ("Arrow TowerPikeman"); the hand holds two identical "Archer 12g" cards; a tiny white dash sits at the exact top-center of the viewport in every frame.

## Timeline of findings

| # | t (±0.25 s) | frame evidence | measured observation (pixels, not conclusions) |
|---|------|----------------|------------------------------------------------|
| 1 | 0:00–0:04.5 | sheet_01 tiles 1–10 | Hero idle on grass facing the entrance. Gold ticks 68→72 at exactly +1/s (matches the HUD's "+1/s"). Rally widget "Rally: Ready / +1/s / 0/6" static. |
| 2 | 0:04.6 | f00004_60s | Hero runs toward the archway. Green berm strip spans the frame in front of the castle wall. Blue bar visible floating in the archway opening. |
| 3 | 0:05.0–0:05.3 | f00005_00s, f00005_30s (promoted) | Hero stationary at the threshold; the sunlit berm top crosses at his hip height **directly in front of the gateway**, spanning its full width — no cut/ramp visible through it at this camera angle. |
| 4 | 0:05.6–0:06.2 | f00005_60s, f00005_90s, f00006_20s | Hero airborne three consecutive samples, over the berm top / tan apron in front of the arch. Airborne pose shows arms out, legs tucked; shadow shows a splayed flail pose (jump/fall anim present). |
| 5 | 0:06.5–0:06.8 | f00006_50s, f00006_80s | Hero descends and stands **back at grass level**, the berm's shadowed face rising to his hip in front of him. |
| 6 | 0:07.1–0:07.7 | f00007_10s, f00007_40s (promoted), f00007_70s | Two more airborne samples over the berm at increasing height. Still no frame past the threshold. |
| 7 | 0:08.0–0:11.0 | sheet_02 tiles 5–11 | Hero stands on the grass outside for the final 3 s, facing the berm/entrance. Gold reaches 79 (+11 over 11 s — income exact). |
| 8 | whole clip | f00005_30s-crop (promoted) | Solid-blue rectangle floats in the archway shadow at roughly second-story height; at 2x scale no owner, frame, or fill boundary is visible beneath/around it. |
| 9 | 0:09.4–0:09.8 | f00009_40s, f00009_60s, f00009_80s (promoted) | White cursor appears at 9.4; blue ring appears at the cursor spot at 9.8. Gold never dips (no card purchase), Rally widget unchanged, hero does not move before clip end. |
| 10 | 0:06.8 | f00006_80s-crop-x325y485 (promoted) | Hand bar text at 2x: "Fireball 21g / Footman 9g / Archer 12g / Archer 12g / Arrow Tower 15g / Pikeman 15g", each with "Play" + "1"; "Next: Archer 12g" chip. Titles "Arrow Tower" and "Pikeman" render with zero gap. |

## Evidence frames (promoted)

- .claude/pipeline/playtest-evidence/2026-08-26/VID-001-t00m05s-entrance-berm-lip.png — hero at the threshold; sunlit berm lip crosses the gateway at hip height, full width.
- .claude/pipeline/playtest-evidence/2026-08-26/VID-001-t00m07s-entry-jump-blocked.png — hero airborne over the berm in front of the arch, one of five airborne samples across 5.6–7.7 s.
- .claude/pipeline/playtest-evidence/2026-08-26/VID-001-t00m05s-floating-hp-bar.png — 2x crop: bare solid-blue bar hanging in the archway air, no visible owner.
- .claude/pipeline/playtest-evidence/2026-08-26/VID-001-t00m09s-click-ring-no-response.png — blue ring on the grass at the just-appeared cursor's position; hero idle.
- .claude/pipeline/playtest-evidence/2026-08-26/VID-001-t00m06s-card-label-collision.png — 2x crop of the hand bar: duplicate Archer cards and colliding "Arrow TowerPikeman" titles.

## Suspected mechanism — HYPOTHESIS, NOT VERDICT

- F1 entrance not entered → the castle skirt/berm mesh (SM_Castle skirt, GH-wave redesign, commit 1025160) may lack a gate cutout, and/or collision blocks the gate corridor. Note `BattlefieldScatter.cpp:1313` documents the skirt as "buried skirt, not a standable surface" — yet the pixels show the hero standing and jumping on it, so its collision is live and standable. Confidence: medium that entry was attempted and failed (behavioral inference — I cannot see input); confirm by asking Jonathan whether he was trying to enter, then a build-master in-scene collision trace through the gate corridor. **Hypothesis, not verdict.**
- F2 floating blue bar → prime suspect `ACastle::HPBarWidget` (UWidgetComponent, `Source/GitClaudeUnrealTest/Siegebound/Castle.h:275`; class `WBP_CastleHealthBar`, `Castle.h:614`): after the interior redesign its anchor location may now sit in the open air of the archway, reading as a disembodied bar. Alternative: a `CombatantHealthBarComponent` bar of an interior actor (e.g. the commander at `CommanderNpcAnchor` — board law says the NPC "must not stand inside the gate corridor") whose body is indiscernible in the dark corridor at capture exposure. Confirm via the widget component's world transform in-scene. **Hypothesis, not verdict.**
- F3 click ring, no visible response → no click-marker/ping system surfaced in a bounded grep of `Siegebound/*.h` (AncientGround/CaptureZone decals are world features). Could be a card-targeting reticle (no purchase followed — gold monotonic) or engine/default cursor feedback. Observation window after the click is only ~1.6 s, so "no response" is weakly supported. Confidence: low. **Hypothesis, not verdict.**
- F4 berm reads as blockout → consistent with known project state: premium art is deferred (M7/Fab), and GH-R1 preserves the exterior. Possibly intentional/known; flagging because Jonathan asked for anything that looks off. Capture compression can exaggerate flat-shading banding (FR-§5 caveat). Confidence: low as a defect, high as a pixel description. **Hypothesis, not verdict.**
- F5 UI polish trio → card-title collision suggests missing padding/auto-size in the hand-bar widget (`CardHandWidget.cpp`); duplicate Archer may be legitimate deck content; the top-center white dash appears widget-like (persistent, screen-anchored) but is unidentified. Confidence: high on the pixel facts, low on mechanism. **Hypothesis, not verdict.**

## Routing recommendation

| finding | lane | suggested fix one-liner (hypothesis) |
|---------|------|--------------------------------------|
| F1 entrance berm lip / no entry | needs-Jonathan first (intent), then art-director + build-master | Confirm entry was intended; cut/ramp the skirt at the gate or clear corridor collision, then walk-through verify in-scene. |
| F2 floating HP bar in archway | gameplay-programmer (with build-master scene check) | Re-anchor `ACastle::HPBarWidget` to a sensible castle-relative height post-redesign (Castle.h:275/614); verify owner actor if it proves to be a combatant bar. |
| F3 click ring without response | needs-Jonathan | Ask what was clicked/expected at ~9.5 s; only board a task if he expected a response. |
| F4 blockout berm material | art-director (backlog, likely known) | Fold into the M7 premium-art pass unless Jonathan wants an interim texture. |
| F5 card-title collision (+ dash, dup Archer) | gameplay-programmer (low priority) | Add padding/spacing between card title slots in the hand widget; identify the top-center dash widget; confirm duplicate Archer is intended. |

## Not examined / limitations this pass

- No audio; input is invisible — "tried to enter" is a behavioral inference from approach + repeated jumps, not a fact.
- VFR capture: all timestamps ±0.25 s; sub-frame flicker between samples would be missed.
- The clip never shows the castle interior, minimap regions, combat, unit spawning, or any card being played — none of those systems were exercised.
- Only ~1.6 s of footage follows the 9.8 s click — the "no response" observation is bounded by clip end.
- Tool quirk (dry-run note): the frame extracted at t=5.0 came out uncropped (full 2496x1440 black canvas, game in the top-left corner) while all other frames were autocropped — content still legible; worth a look in `Tools/VideoReview/extract_frames.py` autocrop-per-frame behavior.
- Image-Read budget used: 20 of ~40 (2 sheets, 15 full frames, 3 crops).
