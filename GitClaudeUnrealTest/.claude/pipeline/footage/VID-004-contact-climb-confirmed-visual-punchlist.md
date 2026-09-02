# Footage Review — VID-004

Video: `testvideo/Siegebound Preview [NetMode_ Standalone 0]  (64-bit_PC D3D SM6) 2026-09-02 14-19-47.mp4`
· 134.9 s · 2512×1440 · h264 · avg **29.299 fps** (probe.json) · ⚠️ **VFR capture (Xbox Game Bar)** — every timestamp below carries **±interval/2** (±0.5 s on coarse-pass rows, ±0.25 s on fine-pass rows)
Autocrop: **none applied** (full-window capture, no black canvas corners; the Unreal window chrome is part of the frame)
Reviewed: 2026-09-02 · Image Reads spent: **32 / ~40**

Jonathan: *"ok, I dropped a video in testvideo folder, it should be the latest one. It seems to work fine, there are some minor visual things to fix but I think it works good enough for now"*
Jonathan (follow-up, on the headline question): *"yes, units were able to climb the ladder after the hero did."*

---

## ⭐⭐ HEADLINE — does a unit climb after the hero? **HIS REPORT IS CORROBORATED ON PIXELS.**

⚖️ **Evidence-class note (kept separate, per this pipeline's rule):** Jonathan *reported* it; the frames below are an *independent pixel corroboration* of the same event. Both agree. The report is his; the frames are mine.

| step | t | frame | what the pixels show |
|---|---|---|---|
| Slot **claimed** | 01:17.0 | `f00077_00s` | The hero (grey Manny) stands alone on the brick deck, between the ladder's two protruding top-rails. No other pawn on the deck. |
| Slot **still sole-occupied** | 01:19.0 / 01:20.0 | `f00079_00s`, `f00080_00s` | Hero alone on the deck across 1.0 s. Deck otherwise empty of pawns. |
| ⭐ **Slot RELEASED — second climber ascending** | **01:20.5** | **`f00080_50s`** (+ 3× zoom crop) | **A red-hooded Archer is emerging through the deck plane at the ladder top: its hood and one raised arm are above the brick surface, its torso is still occluded below the deck, and its own blue health bar floats above it. The hero is simultaneously fully on the deck, ~2 m away.** ⇒ **hero up AND a second pawn mid-ascent in one frame.** |
| Second climber **arrives** | 01:21.0 | `f00081_00s` | Same Archer now fully above the deck, standing beside the ladder head, bow visible. Deck population 1 → 2. |
| ⭐ Slot **cycles repeatedly** | 02:08 | `f00128_00s` | Deck population **6** (hero + 5 units). |
| ⭐ Slot **cycles repeatedly** | **02:14.5 (last second)** | **`f00134_50s`** | Deck population **7** — hero + **six** units (4 blue-capped Footmen, 1 red-hooded Archer, 1 spearman). HUD reads `HOLD set: 6 unit(s)`. |

**Answer: OBSERVED — and not once but ≥ 6 times.** The ladder is **not** bricked. The occupancy slot is claimed by the hero at ~01:16 and demonstrably released; at least six subsequent pawns traverse it in the remaining 58 s.

⚠️ One qualifier worth boarding: the **first** post-hero ascent (01:20.5) happened **before** any HOLD zone was placed on the deck (the first `HOLD set: 6 unit(s)` banner appears at **01:32.7**, `sheet 01:32.7`). So that first ascent was **not** a commanded move — see the abduction row (S4) for the two readings.

---

## Symptoms (his words → what the pixels show)

**S1 — "It seems to work fine"** → **Confirmed.** The contact-climb runs end-to-end: approach (01:11.0) → attach (~01:12–01:13) → ascent (01:14.5, 01:16.5) → deck (01:17.0). No stall, no re-trigger, no pawn left hanging. The hero remains on the deck through the final frame.

**S2 — "some minor visual things to fix"** → **He did not itemise them. Six are itemised below (V1–V6).** One of them (V1, the video-memory banner) is a *build/perf* item, not a cosmetic one, and I'd argue it is the least "minor" thing in the capture.

**S3 — trigger fires by walking, not by a key** → **Consistent with the pixels, with one honest limit.** Across the whole approach (`f00070_50s` → `f00073_50s`) **no interaction prompt, keybind glyph, or "press" affordance is drawn anywhere on the HUD**, and the ascent begins continuously out of the locomotion — there is no frame in which the hero is stopped-and-waiting at the ladder foot. ⛔ A key press leaves **no** pixel trace (FR-§5: no audio, no input overlay), so this is *absence of a prompt* + *continuity of motion*, not proof of the input path.

**S4 — abduction (±247 uu)** → **NONE OBSERVED.** See the limitation note — the camera makes this question largely unanswerable in this capture.

**S5 — descent** → **NOT PRESENT IN THIS CAPTURE.** The hero mounts at 01:17.0 and is still standing on the deck at 02:14.5 (final frame). No pawn is observed descending the ladder at any point. **The descent path is untested by this footage.**

---

## Timeline of findings

| # | t (±) | frame evidence | measured observation (pixels, not conclusions) |
|---|---|---|---|
| 1 | 01:09.0 | `sheet 01:09.0` | Full-screen green translucent placement ghost; Gold reads **30**. |
| 2 | 01:10→01:11 | `sheet`, `f00071_00s` | Gold **31 → 2** across one sheet interval ⇒ the Watch Tower is **placed at ≈70.4 s**. ⭐ **The ladder therefore exists for only the last 64.5 s of a 134.9 s capture** — nothing before 01:10 can bear on any ladder question. |
| 3 | 01:11.0 | `f00071_00s` | Hero mid-stride on the sand path, facing the tower ~1 tower-width away; a red-hooded Archer stands immediately to his right. Tower ladder fully visible on the near face. |
| 4 | 01:11.5 | `f00071_50s` | Tower's stone base subtends ~9 % more screen width than at 01:11.0 ⇒ hero closing. Gait: long stride, trailing heel lifted, arms counter-swinging across the torso — **reads as jog-or-faster, not a slow walk**. ⛔ Cannot separate `WalkSpeed 500` from `SprintSpeed 750` on stills (no speed readout on HUD). |
| 5 | 01:12.0 – 01:13.5 | `f00072_00s`, `f00073_50s` | **The third-person camera collapses into the hero's body for ≈2 s** — the pawn's back/hips fill 60–70 % of the frame, world view lost. This is the approach + attach window; it is the reason the attach instant itself is not directly observable. |
| 6 | 01:14.5 | `f00074_50s` | Hero mid-ladder, **upright idle/walk pose, feet flat, arms at sides — no climb pose** (⭐ known-expected, waived; see the calibration section — it does **not** look worse than described). |
| 7 | 01:14.5 | `f00074_50s` | Hero's body centre sits **left of the ladder's centre-line by ~20 % of the ladder's clear width**; his left arm tracks over the dark recess beside the left rail rather than over the rungs. |
| 8 | 01:16.5 | `f00076_50s` + crop | Hero at the parapet: head/shoulders above the blue slab's top face, hips/legs on the rungs below. **Occlusion is CORRECT — no polygon of the hero intersects the parapet, the stone, or the deck.** Feet planted on rungs. |
| 9 | 01:17.0 | `f00077_00s` | Hero fully on the deck, upright, standing **between** the ladder's two protruding top-rails. **No clipping in this frame either.** |
| 10 | 01:17.4 | `f00077_40s` | Hero walking clear of the ladder head. Mount complete. Total climb ≈ 01:12.5 → 01:17.0 ≈ **4.5 s**. |
| 11 | **01:20.5** | **`f00080_50s`** | ⭐⭐ **Second climber (Archer) emerging through the deck at the ladder top while the hero is already up.** |
| 12 | 01:32.7 | `sheet 01:32.7` | First `HOLD set: 6 unit(s)` banner — i.e. the *commanded* deck occupation begins **12 s after** the first post-hero ascent. |
| 13 | 02:08.0 | `f00128_00s` | Deck population 6. Ladder rungs visible on the near-right tower face. |
| 14 | 02:14.5 | `f00134_50s` | Deck population **7** (hero + 6). `HOLD set: 6 unit(s)`. Hero has **not** descended. |

---

## ⭐ The "minor visual things", itemised (he did not)

| id | t | pixel description | lane |
|---|---|---|---|
| **V1** | **01:11.0 – 01:11.5** (present from at least 01:11; not visible after ~01:12) | ⭐ **A red engine banner across the left-centre of the viewport reading — verbatim, from a 3× crop (`f00071_00s-crop-x0y250`): `Video memory has been exhausted (0.922 MB over budget). Expect extremely poor performance.`** ⛔ I asserted this text only after cropping (never from a sheet tile). | build-master |
| **V2** | 01:12.0 – 01:13.5 | **Third-person camera collapses into the pawn for ≈2 s** during the ladder approach — the hero's back/hips fill 60–70 % of the frame and the world is not visible (`f00072_00s`, `f00073_50s`). Recurs briefly at the deck approach. This is the most *player-visible* defect in the clip. | gameplay-programmer |
| **V3** | 01:14.5, 01:19.0, 01:20.0, 02:08.0 | **Unit health bars render with no depth occlusion.** Blue bars are drawn **on top of** the tower's solid stonework and the wooden ladder (`f00074_50s`), and **four blue bars float over an empty deck** with no pawn beneath them (`f00079_00s`) — they belong to units standing on the ground ~1 tower-height below and punch straight up through the deck. Also visible at 02:08 (`f00128_00s`, bars over the parapet underside). | gameplay-programmer |
| **V4** | 01:16.5 – 02:14.5 (whole deck phase) | **The tower deck platform reads as an untextured flat cornflower-blue slab.** It has real 3-D thickness (a visible side face), it is **walkable** (the hero stands on it at 02:08, `f00128_00s`), and it **overhangs the stone tower body on all sides with no supporting geometry** — at `f00128_00s` the blue edge projects ~180 px past the stone wall into open air. It also **interpenetrates the brick deck**: at 01:17.0 blue is drawn over brick along the near ~15 % of the deck while brick is drawn over blue elsewhere (`f00077_00s`). A pale, hard-edged wash covers the slab's left half at 01:17.0/01:17.4 that does not match the rest of the surface. | art-director |
| **V5** | 01:17.0 (clearest), 01:19.0, 01:20.5, 02:08.0 | **The ladder's two side-rails protrude above the deck surface as two isolated brown planks**, staggered rather than paired, with the hero standing between them (`f00077_00s`). From directly overhead (`f00079_00s`) they read as two disconnected wooden slabs sitting on the brickwork. | art-director |
| **V6** | throughout; worst at **02:14.5** | **Foliage renders as large flat untextured shards.** Distant tree-lines appear as solid-black angular silhouettes in most wide shots (sheets at 00:00, 00:36.5, 01:14.5-left) while a correctly-lit autumn tree is visible in the same frame (`f00074_50s`, right side) — so it is not a global lighting state. At 02:14.5 the camera is close to one and **~30 % × 35 % of the frame is a mass of intersecting flat tan / black / pale-blue polygons with hard silhouette edges** (`f00134_50s-crop-x1730y30`). ⚠️ **Low-to-medium confidence on cause** — h264 compression can exaggerate flat-shaded polygon edges (FR-§5), and this could equally be leaf-card shading or a broken/exploded mesh. | art-director |

**Also noted, deliberately NOT called a defect (colour semantics are not determinable from stills):** at 01:14.5 (`f00074_50s-crop-x980y300`) three bar styles coexist — a white-outlined rounded crimson bar, a plain flat salmon-red bar, and plain flat blue bars. I can confirm the *rendering* fact (V3) but not what red vs blue means here. → **needs-Jonathan** if team-colour consistency matters.

---

## ⛔ The two known-expected items — calibration, not defect reports

**KE1 — NO CLIMB ANIMATION (waived by his ruling).** Confirmed present, and it **does not look worse than described**. At 01:14.5 (`f00074_50s`) the hero ascends in an upright idle/walk pose. Crucially the pose reads as *standing on a rung* — feet flat and in contact with the ladder, body plumb, hands at the rails' depth — rather than a T-posed or floating pass-through. Ascent is smooth and continuous across 01:14.5 → 01:16.5 → 01:17.0. **Cosmetically this reads as "unanimated", not as "broken".** No change to the deferral is indicated by the footage.

**KE2 — CLIPPING NEAR THE TOP / THROUGH THE DECK. ⭐ SEVERITY READ: MINOR — in fact I could not reproduce a visible body-clip at all.** This is the load-bearing judgement, so here is exactly what I did and what I saw:

- I sampled the **entire mount transition** at 76.5 / 77.0 / 77.4 s (0.4–0.5 s steps, spanning the full ladder-top → deck handover) and cropped 76.5 at 2×.
- **In none of those three frames does any part of the hero's mesh penetrate the parapet, the stone body, or the deck.** At 01:16.5 the hero is cleanly *outside* the parapet with correct occlusion (torso in front of the slab's face, legs below on the rungs). At 01:17.0 he is standing upright on the brick between the two rails. At 01:17.4 he is walking clear.
- The one **measurable** geometric anomaly during the ascent is **lateral, not width-related**: at 01:16.5 the hero's silhouette is **narrower than the ladder's clear opening** but sits **~20 % of that width left of the ladder centre-line**, so his left shoulder/arm tracks over the dark recess beside the left rail (`VID-004-t01m16s-deck-transition-clearance.png`).

⇒ **Read for the pending 10 uu move:** what this footage shows is a **lateral offset toward one rail**, which is the kind of error a translation fix addresses — so the fix looks aimed at the right axis. ⛔ **I cannot tell you whether 10 uu is *enough*,** because converting my ~20 %-of-clear-width measurement into uu requires the ladder's true clear width, which is not measurable from footage. ⚠️ **Two caveats that keep this from being a clean bill of health:** (a) my sampling is 0.4–0.5 s and a single-frame interpenetration could hide between samples — a `--run` burst at 29.3 fps across 76.8–77.2 would close that; (b) this is **one** ascent by **one** pawn class at **one** approach angle. The six unit ascents happen while the camera looks down at the deck, so their ladder-top handovers are largely off-frame.

---

## Abduction (±247 uu) — NOT OBSERVED, and largely **unobservable** in this capture

- **No pawn is seen being yanked upward off a trajectory tangential to the ladder** at any point in the 64.5 s the tower exists.
- **The one candidate** is the 01:20.5 Archer. It ascended **before** any HOLD zone existed on the deck (first `HOLD set` = 01:32.7), so it was **not** a commanded move. Two readings, neither decidable on pixels: (i) benign — the same red-hooded Archer is standing at the hero's right shoulder at 01:11.0 and 01:11.5 (`f00071_00s`, `f00071_50s`), i.e. it is escorting/following him and went up after him on purpose; (ii) incidental grab — it was near the ladder for other reasons and the contact radius took it. **I record the ambiguity rather than resolve it.**
- ⛔ **The real limitation:** from 01:16 onward the camera is locked to the hero **on top of the tower and pointed down at the deck**. The ground at the tower base — precisely where a pass-by abduction would happen — is off-screen or extremely foreshortened for essentially the whole window in which the ladder is live. **An abduction could have occurred repeatedly and left no pixel in this capture.** This question is **not answered**, not "answered no".

---

## Evidence frames (promoted)

- `.claude/pipeline/playtest-evidence/2026-09-02/VID-004-t01m11s-approach-vram-warning.png` — hero mid-approach to the tower; red engine banner across the left-centre; blue unit bars drawn over the tower's solid stonework.
- `.claude/pipeline/playtest-evidence/2026-09-02/VID-004-t01m14s-hero-midladder-no-climb-anim.png` — hero mid-ladder in an upright idle pose, feet on a rung, offset left of the ladder centre-line; correctly-lit tree at right vs black shard trees at left.
- `.claude/pipeline/playtest-evidence/2026-09-02/VID-004-t01m16s-deck-transition-clearance.png` — 2× crop of the ladder-top handover: head/shoulders above the blue parapet, legs on the rungs, **zero mesh interpenetration**.
- `.claude/pipeline/playtest-evidence/2026-09-02/VID-004-t01m17s-ladder-rails-through-deck.png` — hero upright on the brick deck between the two protruding ladder side-rails; blue slab interpenetrating the brick along the near edge.
- ⭐⭐ `.claude/pipeline/playtest-evidence/2026-09-02/VID-004-t01m20s-second-climber-ascending.png` — **the single strongest frame: hero already on the deck AND a second pawn mid-ascent at the ladder head.**
- ⭐⭐ `.claude/pipeline/playtest-evidence/2026-09-02/VID-004-t01m20s-second-climber-ascending-zoom.png` — 3× crop of the same: the Archer's hood and raised arm above the deck plane, torso still occluded below it.
- `.claude/pipeline/playtest-evidence/2026-09-02/VID-004-t02m14s-seven-pawns-on-deck.png` — final second: seven pawns on the deck (hero + six units) ⇒ the slot cycled ≥ 6 times.
- `.claude/pipeline/playtest-evidence/2026-09-02/VID-004-t02m14s-foliage-shard-artifact.png` — 2× crop: ~a third of the frame filled with intersecting flat tan/black/pale-blue polygons.

---

## Suspected mechanism — ⛔ HYPOTHESIS, NOT VERDICT

Named from **read-only** `Grep` of `Source/`. Every line here is a hypothesis until an implementer's instrument confirms it.

- **Slot released ≥ 6 times (headline)** → `Source/GitClaudeUnrealTest/Siegebound/ClimbableTower.h:776` `IsLadderSlotOccupied()` / `:755` `ReleaseClimber()`; the header's own note at **`ClimbableTower.h:833–836`** records that TASK-787 made the slot *"⛔ NOT ONLY OCCUPIABLE BY A HERO BUT RELEASABLE"* and that before it *"a hero could be admitted and ⛔ never released"*. — **Confidence HIGH** that the footage is consistent with that change having landed. **What would confirm it:** a log/`stat` trace of `ReleaseClimber` firing on the hero's `OnLadderClimbEnded`, or a scripted two-climber PIE test. ⛔ The pixels show the *outcome*, never the delegate.
- **Trigger by proximity + dwell, not input** → `ClimbableTower.h:653` `LadderContactRadiusUU = 350.f` and `:685` `LadderContactDwellSeconds = 0.35f`; polled by `SiegePlayerController.cpp:4970` per the header's note at `:414–416`. The header's own worked example at `:602` — *"At 350 uu and a 300 uu/s walk, a pawn aimed at the ladder crosses ~105 uu of approach during the 0.35 s dwell"* — matches the smooth, non-stopping approach observed at 01:11–01:13. — **Confidence MEDIUM** (absence of a prompt is weak evidence).
- **Approach speed / sprint worst case** → `HeroCharacter.h:345` *"Walks at 500 u/s, sprints at 750 u/s while IA_Sprint is held"*, `:1061` `SprintSpeed = 750.f`. The gait at 01:11.5 reads jog-or-faster but **I cannot resolve 500 vs 750 on stills.** — **Confidence LOW on which speed was used.** **What would confirm it:** a HUD speed readout (or a one-line `stat`/on-screen debug of `Velocity.Size2D()`) during a re-test.
- **±247 uu accidental-grab window** → `ClimbableTower.h:639` *"R = 150 → ±58 · R = 300 → ±202 · R = 350 → **±247**"* and `:644` *"a pawn marching past its own tower is now grabbed from up to ~247 uu to the [side]"*. The header at `:646–649` already names a **free lever**: raising `LadderContactDwellSeconds` 0.35 → 0.50 s narrows the window at zero radius cost, *for the unit side only*. — **Confidence N/A: the footage neither supports nor refutes this**, because the camera never watches the tower base while the ladder is live.
- **Camera collapse into the pawn (V2)** → the spring-arm/collision setup on `Source/GitClaudeUnrealTest/Siegebound/HeroCharacter.cpp` (camera boom); the collapse coincides exactly with the hero entering the tower's collision footprint. — **Confidence MEDIUM.** **What would confirm it:** watching `SpringArm->TargetArmLength` vs the probe hit during an approach.
- **Health bars without depth occlusion (V3)** → `Source/GitClaudeUnrealTest/Siegebound/CombatantHealthBarComponent.h` (its `:94` note distinguishes World-space vs Screen-space Slate paths). — **Confidence MEDIUM-HIGH** that this is a widget-space / depth-test setting rather than a gameplay bug. **What would confirm it:** toggling the component's widget space and re-shooting the same deck view.
- **Video memory exhausted (V1)** → not a `Siegebound` source item; an engine/renderer budget report. Prior VRAM findings are already on record in this project. — **Confidence HIGH on the observation, none on the cause.**

---

## Routing recommendation

| finding | lane | suggested fix one-liner (⛔ hypothesis) |
|---|---|---|
| ⭐⭐ Headline: slot releases and cycles ≥ 6× | **no fix needed — close it** | Corroborates TASK-787; record VID-004 as the pixel evidence for `TOWER-§10` L-1 release. |
| V1 — `Video memory has been exhausted (0.922 MB over budget)` | **build-master** | Capture a `stat streaming` / texture-pool baseline for this map; 0.922 MB over is a *hair* over budget — likely a pool-size setting, not an asset blowout. |
| V2 — camera collapses into the pawn for ≈2 s at the ladder | **gameplay-programmer** | Add the tower/ladder to the spring-arm probe ignore set, or raise the probe size / min arm length during a climb. |
| V3 — health bars render through world geometry | **gameplay-programmer** | Review `CombatantHealthBarComponent` widget space / depth handling; bars for pawns below the deck should not draw over it. |
| V4 — deck platform is an untextured oversized blue slab, overhangs the tower, interpenetrates the brick | **art-director** | Assign the intended parapet/deck material and re-fit the slab to the tower footprint; resolve the near-edge coplanar overlap with the brick deck. |
| V5 — ladder side-rails protrude through the deck as two loose planks | **art-director** | Trim/cap the rails at the deck plane (likely folds into the pending ladder-mesh reimport). |
| V6 — foliage renders as flat black / pale-blue shards | **art-director** | Inspect the tree asset's leaf cards / two-sided-lighting; ⚠️ verify in-editor first — capture compression could be exaggerating this. |
| KE2 — deck clearance | **gameplay-programmer** (already boarded) | The pending 10 uu translation targets the right axis; ask for a `--run` burst re-capture at the handover to confirm no sub-sample interpenetration. |
| Approach speed 500 vs 750 | **needs-Jonathan** | Re-run the contact-climb **holding sprint** and say so, or enable a velocity readout — sprint is the worst case and is **untested here**. |
| Descent | **needs-Jonathan** | Not in this capture at all. Please climb **down** once on the next pass. |
| Abduction (±247 uu) | **needs-Jonathan** | Walk a unit *past* the ladder, tangentially, with the camera on the **tower base**, not on the deck. |
| Red vs blue bar semantics | **needs-Jonathan** | Confirm whether the hero/structure bars are meant to read red while unit bars read blue. |

---

## Not examined / limitations this pass

1. **Descent — entirely absent.** The hero never comes down; no pawn descends. The descent failure mode is **untested**, not "passed".
2. **Abduction — unobservable, not absent.** From 01:16 to 02:14.5 the camera looks *down at the deck*; the tower base is off-frame. Re-shoot with the camera on the ground.
3. **Sprint approach — not exercised (or not distinguishable).** The gait reads jog-or-faster but `WalkSpeed 500` vs `SprintSpeed 750` cannot be separated on stills. ⭐ Since sprint is the stated worst case, **the worst case is untested by this footage.**
4. **The attach instant itself (~01:12–01:13.5) is occluded** by the camera collapsing into the pawn (V2). The dwell-to-attach transition is inferred from before/after states, not directly observed.
5. **The six unit ascents' ladder-top handovers are mostly off-frame** — the camera looks down at the deck, so I can confirm *arrival* and (at 01:20.5) one *emergence*, but not each unit's full traversal or its clearance at the deck.
6. **First 70.4 s not analysed in fine detail** — the Watch Tower does not exist until then, so nothing there can bear on any ladder question. Coarse-pass only (2.81 s sheets); no anomaly noted.
7. **Sampling bound:** coarse pass 2.81 s → refined to 1.0 s over 60–135 s; fine pass 0.4–0.5 s. **No frame-adjacent (`--run`) burst was taken** — a 1-frame flicker or a single-frame interpenetration would not appear in this report.
8. **VFR capture** (Game Bar): all timestamps ±interval/2. **No audio** (FR-§5). **h264 compression** can mimic rendering defects — V6 is explicitly flagged low-to-medium confidence on that basis.
9. **Budget:** 32 of ~40 image Reads spent. Nothing was dropped for budget reasons; the gaps above are camera/content gaps, not budget gaps.
10. ⛔ **No code, art, engine, or Git action was taken.** `testvideo/` was never staged. Only the eight promoted PNGs under `.claude/pipeline/playtest-evidence/2026-09-02/` are repo-bound.
