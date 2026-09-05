# Footage Review — VID-005

Video: `testvideo/Siegebound (64-bit Development PCD3D_SM6)  2026-09-03 18-15-04.mp4` (note: `testvideo/` lives at the **git root**, i.e. `C:\GitProjects\GitHub\GitClaudeUnrealTesting\testvideo\`, NOT inside the project dir) · 42.87 s · 2560x1440 · h264 · avg 28.494 fps (probe.json) · ⚠️ **VFR (Game Bar)** — sheet timestamps are ±interval/2 = **±0.45 s**; `frames --at` times are exact decode targets. Autocrop: none required (full-screen capture, no black canvas).
Reviewed: 2026-09-03 · Image Reads spent: **22 of ~40**.

Jonathan: *"in the latest video, there are 2 bugs I want to point out, the first bug is that for some reason I am not able to stack towers on top of each other in the way that I described that you should be able to, instead I just get "building cannot be stacked", secondly, I get a strange error that says "card actor unavailable"."*

Feature intent (his earlier words, as relayed): select tower card → hover an existing tower → ghost goes **entirely BLUE** (vs entirely GREEN = valid placement, entirely RED = invalid) → click upgrades that tower. Height ×2→×5 capped; health ×1.5 compounding uncapped. Mouse wheel resizes width/length during placement (cap 1.5×); WatchTower deliberately excluded from resizing.

---

## ⚠️ A retracted question, recorded on purpose

My dispatch first told me to look for **"a blue outline on the target tower."** That was the wrong object, and the orchestrator corrected it mid-pass: the blue signal is the **pending ghost's own tint**, not an outline on the tower. I had already been recording the ghost's tint, so the correction cost little — but **1 image Read** (the 00:08.4 tower-silhouette crop, `f00008_40s-crop-x380y320`) was spent specifically hunting an outline on the tower's stonework. That read is not wasted: it doubles as the proof that the red ghost volume is *coincident with* the existing tower. **For the record: there is no outline of any colour on the existing tower at any point in this video.**

---

## Symptoms (his words → what the pixels show)

**S1 — "not able to stack towers … I just get 'building cannot be stacked'".**
Confirmed, and the pixels add the decisive detail he asked for: while he hovers the existing Watch Tower with a Watch Tower card in placement mode, **the ghost is RED — never BLUE.** It is red at every sampled instant of both hover episodes, including across the whole window in which the click must have landed. The ghost only ever showed **two** of its three states in this video: green (open ground) and red (on the tower).

**S2 — "a strange error that says 'card actor unavailable'".**
Confirmed, twice, and it is a **different card from S1**: a **Witch** (50 g). Its ghost renders **GREEN (valid)**, it disappears on the click, **no gold is deducted**, and nothing spawns.

**S3 — (not reported by him, same root card as S2)** "Not enough gold" fires 4 sampled times at Gold 45–48, all below the Witch's 50 g. This is **correct behaviour**, and it is the thing that dates the S2 attempts: he was trying the Witch card from ~26 s and only crossed its price at ~31 s.

---

## Timeline of findings

Tint figures below are a whole-video census: % of pixels in the upper 60% of frame that are strongly red-/green-/blue-dominant, sampled every 0.5 s (plus the fine frames). The instrument was **validated on positive controls before use** — 09.20 s → red 58.06 (matches a visually-read red ghost), 10.90 s → green 33.00 (visually-read green ghost), 07.60 s → all-baseline (visually-read no ghost).

| # | t | frame evidence | measured observation (pixels, not conclusions) |
|---|---|---|---|
| 1 | 00:07.6 | `f00007_60s` | No ghost (red 0.03 / green 0.27 / **blue 1.61** = baseline). Gold 56. Hotbar: 1 Witch 50g · 2 Watch Tower 30g · 3 Archer 12g · 4 Watch Tower 30g · 5 Watch Tower 30g · 6 Witch 50g; "Next: Cleric 18g". A stone **Watch Tower** (ladder rungs, two blue health bars) stands directly ahead of the hero. |
| 2 | 00:07.8–00:10.5 | `f00008_40s`, **`f00009_20s`** (promoted) | Ghost present and **RED** continuously: red 30.9 → 44.6 → **58.2 peak** → 55.8. Green ≤0.19. **Blue flat at 1.16–1.45 — no excursion whatsoever.** The red volume is *coincident with the existing Watch Tower*: the tower's own ladder rungs read through it as darker red bands. Cursor sits inside the ghost, over the tower. |
| 3 | ~00:09.4 (bracket 9.2 < t ≤ 9.8) | `f00009_20s` toast **absent**; sheet 00:09.8 toast **present** | "That building cannot be stacked" first appears **while the ghost is RED** (red 52.18 at 09.40). |
| 4 | 00:10.6–00:11.6 | **`f00010_90s`** (promoted) | Ghost flips **GREEN** (green 28.0 → 33.0 → 21.0; red ≤0.03) when moved onto **open ground to the right of the same tower**. The tower itself is untinted and un-outlined. Stack toast still displayed. |
| 5 | 00:11.8–00:12.6 | **`f00012_20s`** (promoted) | Ghost **RED again** (red 44.1 → 42.9 → 14.2) back over the tower, cursor over it; toast still displayed. **Second** hover-the-tower episode. |
| 6 | 00:12.8–00:13.2 | census | Ghost **GREEN again** (green 21.1 → 31.4) over open ground. |
| 7 | ~00:13.25 | **`f00013_30s`** (promoted) | **A placement SUCCEEDS** — on open ground, not on the tower. Gold **61 → 32** (−30 net of +1/s accrual = the 30 g Watch Tower). Ghost gone (red 0.01 / green 0.02). A new Watch Tower stands at frame right. Hotbar slot 2 "Watch Tower" has become **"Cleric"**, "Next: Watch Tower" ⇒ the card played was **slot 2, Watch Tower**. The stack toast is *still on screen* through this success. |
| 8 | ~00:14.1 | `f00014_00s-crop` toast **present**; sheet 00:14.3 **absent** | Stack toast ends. Continuous span ≈ **09.4 → 14.1 ≈ 4.7 s**. |
| 9 | 00:26.5–00:29.9 | **`f00026_80s-crop`** (promoted) | "Not enough gold" (crop-verified) with **Gold: 45** (crop-verified). Present at 26.8/27.7/28.6/29.5 → Gold 45/46/47/48; absent 25.9 and 30.4. Every value **< 50 = the Witch's cost**. |
| 10 | 00:32.0–00:32.6 | **`f00032_20s-crop`** (promoted) | A small **GREEN** ghost, ~hero height: **pointed conical hat, robed, arms out — a Witch silhouette** (green 1.69 → 0.80). Gold **50**, exactly the Witch's price. Toast **absent** at 32.6 (crop-verified). Hotbar now 1 Witch · 2 Cleric · 3 Archer · 4 WT · 5 WT · 6 Witch. |
| 11 | ~00:32.8–00:34.6 | **`f00034_00s-crop`** (promoted), `f00034_40s-crop` | Ghost **vanishes** (green 0.80 → 0.09 at 33.0) and **"Card actor unavailable"** displays (crop-verified at 34.0 and 34.4; absent 32.6 and at sheet 34.8). Gold runs **50 → 52 → 53 with no deduction**. Burst ≈ **1.8 s**. |
| 12 | 00:38.0 | census | Witch ghost **GREEN** again (green 2.22); toast absent at 37.5 (frame-read). |
| 13 | ~00:38.2–00:39.8 | sheets 38.4 & 39.3 present; 37.5 & 40.2 absent | **Second** "Card actor unavailable" burst, ≈ **1.8 s**. Gold climbs monotonically 56 → 61 to end of video: **no deduction, nothing spawned**. |

### ⭐ The headline measurement — the ghost's colour at the click

Across the whole 42.87 s census the pending ghost renders in exactly **two** tints: **red** and **green**. During both tower-hover episodes the **blue channel never moves off its scene baseline (1.16–1.66%) while red swings to 58.2%.** A blue ghost of comparable screen area would have registered in the same 20–58% band that red and green do. **The ghost was RED for the entire 7.8–10.5 s window and the entire 11.8–12.6 s window**, so whichever instant inside those windows the click landed on, **the ghost was red at the moment of the click.** No pinpointing of the click frame is required for that claim to hold.

⚠️ **One blue excursion was checked and rejected.** Blue jumps 1.5% → 9.26% at 13.30 s. Reading the frame shows it is the **newly placed tower's blue-lit surfaces plus two extra health bars** — red and green are both ≈0 there, i.e. no ghost exists in that frame at all. Reported here because a naive read of the blue column would have manufactured a false "blue state observed."

---

## Answers to the five questions asked

1. **Ghost colour while hovering the tower:** **RED**, at every sampled instant of both episodes, and red at the click. Never blue. (`f00009_20s`, `f00012_20s`, census.) ⇒ the blue/stack state was never displayed.
2. **What was he hovering:** an **existing Watch Tower** — stone shaft, climbable ladder rungs, two blue health bars — the same building type as the card he was holding (slot 2, "Watch Tower", 30 g). (`f00007_60s`, `f00009_20s`.)
3. **In placement mode?** **Yes** — a ghost is rendered throughout 07.8–13.2. **Wheel-resized first?** **Cannot be determined from pixels.** The ghost's screen coverage does grow 30.9% → 58.2% between 07.8 and 09.0, but the camera translates in that same interval (the hero is standing at 07.6 and running at 09.2), so growth is confounded by camera motion and is **not** evidence of wheel use. See limitations.
4. **Which error when:** "That building cannot be stacked" ≈ **09.4 → 14.1**. "Card actor unavailable" ≈ **32.8 → 34.6** and ≈ **38.2 → 39.8**. (Plus "Not enough gold" ≈ 26.5 → 29.9.) The two errors he reported are **on different clicks, minutes apart in game-time terms, and on different cards** — they never co-occur.
5. **How many times, anything succeed between:** stack refusal — toast held continuously for 4.7 s against a measured ≈1.8 s toast lifetime ⇒ **at least 3 overlapping triggers**, corroborated by the ghost returning to RED for a distinct second episode. "Card actor unavailable" — **exactly 2 bursts**, each ≈1 lifetime, separated by ~3.6 s of clear screen ⇒ ≈2 fires. **And yes, one thing succeeded in between: a Watch Tower was placed on open ground at ~13.25 s (Gold 61→32), i.e. the card and the placement system work — only the stack-onto-a-tower case refuses.**

---

## Evidence frames (promoted)

- `.claude/pipeline/playtest-evidence/2026-09-03/VID-005-t00m09s-ghost-red-on-stack-target.png` — ghost RED (58.06% red pixels) coincident with the existing Watch Tower, cursor on it; tower's ladder rungs visible through the red.
- `.claude/pipeline/playtest-evidence/2026-09-03/VID-005-t00m10s-ghost-green-open-ground.png` — same ghost GREEN (33.00%) one second later on open ground beside that tower; stack toast still up. The green/red pair is the whole two-state tint.
- `.claude/pipeline/playtest-evidence/2026-09-03/VID-005-t00m12s-ghost-red-second-attempt.png` — ghost RED again (42.94%) on the tower, second refusal episode.
- `.claude/pipeline/playtest-evidence/2026-09-03/VID-005-t00m13s-placement-succeeds-gold-61-to-32.png` — the one success: Gold 61→32, new tower at right, hotbar slot 2 Watch Tower → Cleric, stack toast still displayed.
- `.claude/pipeline/playtest-evidence/2026-09-03/VID-005-t00m32s-witch-ghost-green-valid.png` — the Witch ghost, GREEN/valid, pointed hat and robe legible at 3×.
- `.claude/pipeline/playtest-evidence/2026-09-03/VID-005-t00m34s-card-actor-unavailable-toast.png` — "Card actor unavailable", crop-verified at 2×.
- `.claude/pipeline/playtest-evidence/2026-09-03/VID-005-t00m26s-not-enough-gold-at-45.png` — "Not enough gold" crop-verified; the companion Gold:45 crop confirms 45 < 50.

---

## Suspected mechanism — HYPOTHESIS, NOT VERDICT

- **Ghost never entered the blue state** → `ASiegePlayerController::UpdatePlacementGhost`, `Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.cpp:2819-2823`. The tint is one ternary: `UpgradeGhostColor` is selected **only** when `PlacementUpgradeState == EPlacementUpgradeState::Ready`, else green/red on `bPlacementValid`. Observing red-not-blue is consistent with that state never reaching `Ready` on this hover. **Confidence: high that the state was not Ready; the *reason* is not visible in pixels.** Confirmed by: an instrument logging `PlacementUpgradeState` and the resolved hover target each frame during a WatchTower hover.

- **⭐ The eligibility predicate appears to be the same flag as the wheel-resize exclusion** → `ConfirmStackUpgrade` refuses with `StackNotStackableRefusalText()` when `!Target->CanScaleFootprint()` (`SiegePlayerController.cpp:2494-2501`), and `ApplyPlacementFootprintWheel` goes inert on the same predicate (`:2839`). The comment at `:2829-2842` names this case **explicitly**: *"The WatchTower's refusal already has a voice — the RED ghost plus 'That building cannot be stacked' on the click."* ⇒ **Hypothesis: the WatchTower's exclusion from width/length footprint scaling is doing double duty as the height-stack eligibility test, so a WatchTower can never be a stack target.** That predicts *exactly* the pixels: always red, never blue, always that message, on every attempt. **Confidence: high.**
  ⚠️ **This looks deliberate, not accidental, and it collides with what Jonathan wants.** `:2491` records the reason: a scaled `SM_WatchTower` fires TOWER-§8.5a's voiding condition and *"the climb stops working ENTIRELY, with every readback still reporting correct."* A fix that simply flips the predicate could silently brick tower climbing. **This needs a ruling before it is boarded as a code change** — the likely shape is *splitting* height-only stacking from footprint scaling, not relaxing one flag.

- **"Card actor unavailable"** → `SiegePlayerController.cpp:2321-2327`: `ResolveCardActorClass(...)` returned null → that exact `NSLOCTEXT` → refuse with **no gold spent** and exit placement mode (matches the observed monotonic gold and the vanishing ghost). Corroborated off-pixels, read-only: `SummonedUnit.cpp:73-80` names `/Game/Blueprints/Units/BP_Unit_Witch` as the Witch's spawn class, and **`Content/Blueprints/Units/BP_Unit_Witch.uasset` does not exist** (Archer, Cavalry, Cleric, Footman, Knight, Longbowman, MilitiaMob, Miner, Ogre, Pikeman, Sapper, Sorcerer, Wizard all do). **`Content/Meshes/SM_Witch.uasset` DOES exist** — which is why the *preview* draws a witch while the *commit* finds no actor class. **Confidence: high.** Confirmed by: resolving the Witch row in `DT_Cards` and checking its actor class soft-pointer.

---

## Routing recommendation

| finding | lane | suggested fix one-liner (hypothesis) |
|---|---|---|
| Ghost is RED, never BLUE, on a WatchTower stack hover; every stack click refused | **needs-Jonathan** (ruling) → then **gameplay-programmer** | Split stack eligibility from `CanScaleFootprint()` so **height-only** stacking is permitted on WatchTower while **footprint** scaling stays excluded — needs his call first because `:2491` warns a scaled `SM_WatchTower` silently voids the climb |
| Blue/upgrade ghost state never observed in play | gameplay-programmer | Once eligibility is split, verify `PlacementUpgradeState` reaches `Ready` on hover so `UpgradeGhostColor` (`:2819-2823`) actually paints |
| "Card actor unavailable" on the Witch card (2 fires, no gold spent) | **art-director** (author asset) + **gameplay-programmer** (verify row) | `BP_Unit_Witch` is missing from `Content/Blueprints/Units/` though `SM_Witch` exists — author/import the Blueprint, or repoint the `Witch` `DT_Cards` row at a shipped class |
| "Not enough gold" at Gold 45–48 | **no action — correct behaviour** | Witch costs 50 g; the gate is working. Recorded only to date the S2 attempts |

---

## Not examined / limitations this pass

- **No audio** (FR-§5) — no click sounds to time inputs against.
- **Click instants are not directly observable** (no input overlay). All click times are inferred from toast onset brackets (±0.3–0.6 s) and ghost-state transitions, and are labelled as brackets throughout.
- **Toast lifetime ≈1.8 s was measured on "Card actor unavailable"** (two independent bursts agreeing) and **assumed** equal for the stack toast. If the HUD uses per-message timers, the "≥3 triggers" inference for the stack error weakens to **≥2** — which the two distinct RED hover episodes support independently.
- **Whether the mouse wheel was used is not determinable from pixels** — ghost growth is confounded by camera motion. (Note: per `:2839` a WatchTower ghost would be inert to the wheel regardless, so this likely does not bear on S1.)
- **Selected hotbar slot is not directly readable** — the hotbar shows **no selected-slot highlight** in any frame (the white ellipse near it is a world-space rally ring seen behind the HUD). Card identity was inferred from three converging lines: the ghost silhouette, the cost-vs-gold threshold (50 g), and which slot was consumed on the one success. Not read directly.
- **Small-ghost blue detection is below the noise floor.** Scene blue baseline varies 0.1–9.3% with lighting, so the "never blue" finding is **conclusive for the large tower ghost** (blue flat 1.16–1.66% while red/green swung 20–58%) but **cannot exclude a faint blue tint on the small Witch ghost** (which registered only 0.8–2.2% green). This does not affect S1.
- **00:14–00:26 reviewed at sheet resolution only** (0.89 s tiles): the hero climbs and walks the towers, no placement activity, no toasts. Not fine-sampled.
- **VFR capture**: sheet-derived timestamps carry ±0.45 s; all bracket endpoints quoted from sheets inherit that.
- **Not attempted:** any editor/engine/MCP inspection, any code change, any git operation (FR-§0.3). The `Source/` references above are read-only Greps for naming suspects, and the two asset-existence checks are read-only directory listings.
