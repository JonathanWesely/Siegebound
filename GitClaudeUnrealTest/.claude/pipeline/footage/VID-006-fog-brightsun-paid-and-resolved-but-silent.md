# Footage Review — VID-006

Video: `testvideo/Siegebound (64-bit Development PCD3D_SM6)  2026-09-04 21-45-11.mp4`
· 73.1 s · 2496x1440 · h264 · avg 28.5 fps · 158.5 MB (probe.json)
· ⚠️ **VFR capture (Game Bar)** — contact-sheet timestamps are **±0.76 s** (interval/2, interval = 1.52 s). Every `frames --at` / `--run` time below is a decoder seek and is frame-accurate; only times marked "(sheet)" carry the ±0.76 s band.
· autocrop: **none applied** (cropdetect found no black canvas — the capture is full-bleed client area).
Reviewed: 2026-09-04 · Image Reads spent: **18** of ~40.

Jonathan, verbatim:
> *"in the lastest video in "testvideo" you can see me first summon an archer, which I then summon a witch, and then the witch is able to make the archer go invisible, that is all good. After that, I try to summon fog twice and nothing seems to happen. Then I try to summon bright sun and nothing seemed to happen either. Also, bright sun and fog seemed to have a placement circle for them, which is totally unneccesary, playing fog or bright sun should be instant and not have any placement circle (like the pickpocket card), see if you can figure out why these fog and bright sun cards seem to not be working and also make them be instantly activated like the pickpocket card."*

---

## ⭐ HEADLINE

**All three casts were confirmed, charged, and — by the shipped confirm contract — RESOLVED: gold went 985→935, 940→891, 893→834 (exactly −50 / −50 / −60), with NO refund and NO on-screen message at any point.** "Nothing happened" is therefore **cause (a) — the card resolved and its effect is invisible/absent** — not a refusal, not a refund, and **not** the reticle bug.

The three casts produce **zero perceptible output** because (i) `NS_Spell_Fog` and `NS_Spell_BrightSun` **do not exist** in `Content/VFX/` while every other shipped spell has its Niagara, (ii) the fog has no rendered volume by design (TASK-841 unbuilt) on a level whose volumetric grid is off (TASK-858), (iii) there is **no HUD element anywhere on screen** naming fog or the prevention window, and (iv) **no enemy unit is on screen for the entire 00:47–01:13 window**, so the one gameplay effect fog has (the 609.6 uu acquisition clamp) had nothing to act on.

---

## Build provenance — he was **NOT** on the stale packaged build

⛔ Refuted on pixels, not inherited.

| evidence | frame | reading |
|---|---|---|
| The **Fog (50g)** and **Bright Sun (60g)** cards are **in his hand with card art** | `f00058_50s` (00:58.5), hand crop `x930y1300` @3× | Slots read `1 Bright Sun 60g · 2 Footman 9g · 3 Archer 12g · 4 Bright Sun 60g · 5 Witch 50g · 6 Fog 50g`, `Next: Fog 50g`. `T_CardArt_Fog/BrightSun.uasset` were **created 2026-09-04 01:48 / 01:51** and `DT_Cards.uasset` was **written 19:50 tonight**. `packagedZIPofGame\Windows\GitClaudeUnrealTest.exe` is **2026-08-30 00:07** — it cannot contain either card. |
| Costs match tonight's `DT_Cards` **exactly** | gold deltas −12 / −50 / −50 / −50 / −60 | `cards.csv`: Archer `12`, Witch `50`, Fog `50`, BrightSun `60`. All confirmed on the counter (timeline below). |
| The **veil renders** | `f00051_00s` (00:51.0) | A translucent, refractive shimmer humanoid replaces the solid Archer. `MI_Unit_Invisible` was wired **2026-09-03** (`d101b1e`) — after the packaged build. |
| No fizzle/refund on a FogCover cast | 00:58.92 → 01:12.5 | If the running binary lacked the `ESpellEffect::FogCover` resolver arm, `ResolveSpell` would return false ⇒ full refund + **"Spell fizzled"**. Neither occurred ⇒ **the fog resolvers are compiled into the binary he ran.** |

⇒ **He was on a current editor build.** Corroborating (off-pixel, read-only): `Binaries/Win64/UnrealEditor-GitClaudeUnrealTest.dll` = **2026-09-04 20:58**, and every load-bearing source predates it (`SiegeCombatStatics.cpp` 17:01, `SummonedUnit.cpp` 17:58, `FogVolume.cpp` 18:49, `SpellLibrary.cpp` 18:50, `SiegePlayerController.cpp` 20:26). Recording started **21:45**. ⛔ Not a build-provenance defect.

---

## Symptoms (his words → what the pixels show)

**S1 — "first summon an archer, which I then summon a witch"** → ✅ Reproduced. Green placement ghost at ~00:09.1 (sheet), then Gold **999 → 988** by ~00:10.7 (sheet) = −12 +1/s = the **Archer**. Second green ghost ~00:12.2 (sheet), Gold **989 → 941** by ~00:13.7 (sheet) = −50 + regen = the **Witch**; the blue-hat Witch model is visible from 00:15.2. **Both summons turned their hand slot over**: slot 1 `Archer 12g` (00:03.5) → `Bright Sun 60g` (00:55.5); slot 5 `Witch 50g` → a second `Witch 50g`. Hold that comparison — it is the control for S4.

**S2 — "the witch is able to make the archer go invisible, that is all good"** → ✅ Reproduced and time-boxed. The Archer (red plume) renders **fully opaque** at 00:47.5, 00:48.5, 00:49.0, 00:49.5 and 00:50.0; at **00:51.0** it is replaced by a **translucent refractive shimmer silhouette** in the same place, which persists unbroken through 01:10.1. ⇒ the veil lands in the bracket **50.0 < t ≤ 51.0**. ⚠️ Two friendly blue health bars are on screen at 00:58.5 with only **one** resolvable shimmer body — I cannot say from pixels whether the Witch is also veiled or merely out of frame (**low confidence, not asserted**).

**S3 — "bright sun and fog seemed to have a placement circle"** → ✅ Confirmed on pixels, three times. A thin white/cyan **ground ring with the mouse cursor inside it** is up for **Fog #1** (00:56.3 sheet; frames 00:57.5, 00:58.0, 00:58.5, and every frame of the 00:58.70 +6-frame run through ≈00:58.91), for **Fog #2** (01:02.5, 01:03.0, 01:03.5, 01:04.0), and for **Bright Sun** (absent at 01:06.0 / 01:06.2 / 01:06.5 → **present at 01:06.8**, still present 01:07.0 and 01:07.2). ⛔ Already diagnosed and boarded as **TASK-1018**; nothing here contradicts it. ⚠️ Pickpocket is **not in this deck**, so the video contains no instant-cast control to compare against.

**S4 — "I try to summon fog twice and nothing seems to happen … bright sun and nothing seemed to happen either"** → the four causes, separated per cast below.

⚠️ **Do not confuse two ground circles.** The large persistent **teal ring** from 00:44 onward is the **HOLD position zone** (`HOLD set: 2 unit(s)`, top-centre, from the hold command at 00:44.2–00:47.2). The spell reticle is a **small thin white ring under the cursor**. Both are on screen simultaneously during every cast.

---

## Timeline of findings

| # | t (frame-accurate unless marked) | frame evidence | measured observation (pixels, not conclusions) |
|---|---|---|---|
| 1 | 00:03.5–00:08.5 | `f00003_50s`…`f00008_50s` | Gold **999** flat. Hand `1 Archer 12g · 2 Footman 9g · 3 Archer 12g · 4 Bright Sun 60g · 5 Witch 50g · 6 Fog 50g`. Both new cards present from the first seconds of play. |
| 2 | ~00:09.1 → ~00:10.7 (sheet, ±0.76) | sheet_01 tiles 7–8 | Green unit ghost, then Gold **999 → 988**. Archer summoned. |
| 3 | ~00:12.2 → ~00:13.7 (sheet, ±0.76) | sheet_01 tiles 11–12 | Second green ghost, then Gold **989 → 941**. Witch summoned. |
| 4 | 00:44.2–00:47.2 | sheet_03 | HOLD command: *"HOLD: circle your units — scroll to resize, LMB confirm, RMB/Esc cancel"* → *"HOLD: 2 unit(s) selected — place the POSITION zone"* → **`HOLD set: 2 unit(s)`**, which then occupies the top-centre HUD slot for the **rest of the video**. |
| 5 | 50.0 < t ≤ **00:51.0** | **`f00051_00s`** (promoted) | Archer solid at 00:50.0 → translucent shimmer at 00:51.0. **The veil works.** |
| 6 | 00:55.5–**00:58.91** | **`f00058_50s`** (promoted) + `f00058_70s-r01..r06` | **Fog #1 targeting.** Gold climbs 982→**985** on the +1/s income alone. White ground reticle up, cursor inside it. Hand slot 6 = `Fog 50g`, `Next: Fog 50g`. Gold is still **985** on the last frame of the 58.70 run (≈**00:58.91**). |
| 7 | **00:58.92** | `f00058_92s-r01` | ⭐ **Gold 985 → 935. Exactly −50 = Fog's cost. The reticle is GONE.** ⇒ he **left-click CONFIRMED**; the click lands in **58.91 < t ≤ 58.92**. |
| 8 | 00:58.92 → 00:59.38 (**14 consecutive frames**) | `f00058_92s-r01..r14` | ⛔ **NO on-screen message of any kind, anywhere on the full frame.** No "Spell fizzled". No toast. Gold ticks 935→936 (income). **No screen fog, no colour shift, no exposure change, no VFX at the reticle point, no unit reaction.** |
| 9 | 00:59.0 | **`f00059_00s`** (promoted) | Gold **935**. Hand **unchanged**: `Bright Sun · Footman · Archer · Bright Sun · Witch · Fog`. `Next: Fog 50g` — **unchanged**. |
| 10 | 00:59.5–01:04.0 | `f00059_50s`…`f00064_00s` (top-strip crops @2×) | Gold rises **monotonically** 936→940 at +1/s. ⛔ **No refund** — the 50 is gone for good. |
| 11 | 01:02.5–01:04.0 | **`f00064_00s`** (promoted) | **Fog #2 targeting.** Ground reticle up again, cursor inside. Gold **940**. Hand slot 6 still `Fog 50g`, `Next: Fog`. |
| 12 | 64.00 < t ≤ **01:04.30** | `f00064_30s-r01` | ⭐ **Gold 940 → 891** (−49 net = −50 + 1 s of income). **Reticle gone ⇒ confirmed.** |
| 13 | 01:04.30 → 01:04.48 (6 consecutive) + 01:05.0 / 01:05.5 / 01:06.0 | `f00064_30s-r01..r06`, `f00065_00s`, `f00065_50s`, `f00066_00s` | ⛔ **No message. No refund. No visual change.** Gold rises 891→892. |
| 14 | 01:04.5 | `next` crop @4× | ⭐ **`Next:` flips `Fog` → `Witch`** — the FIRST change to the Next widget since 00:55.5. Hand slot 6 still reads `Fog 50g`. |
| 15 | 01:06.5 → **01:06.8** | **`f00066_80s`** (promoted) | **Bright Sun targeting.** No ring at 01:06.0 / 01:06.2 / 01:06.5; **ring present with cursor inside at 01:06.8**, still present 01:07.0 and 01:07.2. Gold 893. |
| 16 | 67.20 < t ≤ **01:07.40** | `f00067_40s-r01` | ⭐ **Gold 893 → 834** (−59 net = −60 + 1 s income). **Reticle gone ⇒ confirmed.** |
| 17 | 01:07.40 → 01:07.68 (8 consecutive) | `f00067_40s-r01..r08` | ⛔ **No message. No brightness/exposure change. No VFX. No refund.** |
| 18 | 01:08.0 | **`f00068_00s`** (promoted) | ⭐ **Hand slot 4 `Bright Sun 60g` → `Witch 50g`** — the Bright Sun card **left his hand and was replaced**. Gold **834**. `Next: Witch`. |
| 19 | 01:08.0 → 01:12.5 (end) | `f00070_00s`, `f00072_50s`, sheet_04 | Gold 834→836→839 (+1/s, clean). No fog, no sun, no message, no unit behaviour change. **No enemy unit appears anywhere in 00:47–01:13** (the only red bar on screen is the distant enemy castle's). |

---

## ⭐⭐ VERDICT PER CAST — which of the four causes the pixels support

### Cast 1 — **Fog**, confirmed at **00:58.92**

| question | pixel answer |
|---|---|
| **1. Gold deducted?** | ✅ **YES — 985 → 935, exactly −50.** Never refunded (936, 937, 938, 939, 940 = pure +1/s income through 01:04.0). |
| **2. On-screen message?** | ❌ **NONE.** 14 consecutive full frames spanning 00:58.92–00:59.38, plus full frames at 00:59.5, 01:00.5, 01:01.5, 01:02.5. The only text on screen is the persistent `HOLD set: 2 unit(s)`, `Gold:`, and `Rally: Ready / +1/s / 0/6`. Against the **≈1.8 s** toast lifetime measured in VID-005, a toast fired at this click could not have been missed. |
| **3. Card left hand?** | ⚠️ **CANNOT BE DECIDED FROM PIXELS.** Slot 6 reads `Fog 50g` both before and after — a consumed Fog replaced by the deck's other Fog looks identical. The one asymmetry: **`Next:` did NOT advance** (still `Fog` at 00:59.0, 01:00.5, 01:02.5, 01:04.0), whereas it *did* advance on cast 2, and the hand slot *did* turn over on both unit summons and on Bright Sun. That is **consistent with** no draw having occurred here, but it is **not proof**: I cannot see his deck list, and the hand at 01:08.0 shows **three simultaneous Witches** (slots 4+5 and `Next`) against a `MaxCopies` of 2 — so the deck either recycles its discard or exceeds the cap. **Stated as ambiguous rather than resolved.** |
| **4. Reticle confirmed?** | ✅ **YES.** Ring + cursor up through 00:58.91; ring gone and 50 g spent at 00:58.92. ⛔ **This was NOT an unconfirmed placement — the reticle bug is not the explanation here.** |
| **5. Any visual change?** | ❌ **NONE** — no fog, no colour/exposure shift, no VFX at the reticle point, no unit reaction. (Expected per the briefing for the *volume*; the absence of any **cast** VFX is finding F1.) |

⇒ **Cause: (a) the card resolved and its effect is invisible/absent.** Given the shipped confirm contract (deduct → resolve → refund + "Spell fizzled" on false), a **permanent** deduction with **no fizzle** can only mean `ResolveSpell` returned **true**. ⚠️ The single loose thread is question 3 — see **F3**.

### Cast 2 — **Fog**, confirmed in **64.00 < t ≤ 01:04.30**

Identical to cast 1 on questions 1, 2, 4 and 5: **−50 g permanent (940 → 891)**; **no message** across 6 consecutive frames plus 01:05.0 / 01:05.5 / 01:06.0; reticle up 01:02.5–01:04.0 then gone at the deduction; **no visual change**. **Question 3 differs:** `Next:` **flips `Fog` → `Witch` at 01:04.5**, which is the deck advancing ⇒ **a draw happened** ⇒ this cast **did consume its card**. ⇒ **Cause: (a).**

### Cast 3 — **Bright Sun**, confirmed in **67.20 < t ≤ 01:07.40**

| question | pixel answer |
|---|---|
| **1. Gold?** | ✅ **YES — 893 → 834, exactly −60.** Never refunded (836, 839 = income). |
| **2. Message?** | ❌ **NONE** across 8 consecutive frames 01:07.40–01:07.68, plus 01:08.0, 01:10.1, 01:12.5. |
| **3. Card left hand?** | ✅ **YES, unambiguously — slot 4 `Bright Sun 60g` becomes `Witch 50g` between 01:07.0 and 01:08.0**, and `Next:` stays `Witch` (another one drawn behind it). |
| **4. Reticle confirmed?** | ✅ **YES** — ring appears 01:06.8, still up 01:07.2, gone with 60 g spent by 01:07.40. |
| **5. Any visual change?** | ❌ **NONE** — no brightness lift, no exposure change, no VFX. (Bright Sun's job is to *clear* fog and open a prevention window; with no fog visual there is nothing to see it remove.) |

⇒ **Cause: (a), with full certainty on all four questions. Bright Sun WORKED**: paid, consumed, resolved, redrawn. Every observable the game gives a player said nothing.

---

## Evidence frames (promoted)

- `.claude/pipeline/playtest-evidence/2026-09-04/VID-006-t00m51s-veil-live-build-provenance.png` — the Archer rendered as a translucent refractive shimmer (solid at 00:50.0), proving the 2026-09-03 veil is in the running binary.
- `.claude/pipeline/playtest-evidence/2026-09-04/VID-006-t00m58s-fog1-reticle-up-gold-985.png` — Fog #1 targeting: white ground ring with the cursor inside it, `Gold: 985`, hand slot 6 `Fog 50g`, `Next: Fog 50g`.
- `.claude/pipeline/playtest-evidence/2026-09-04/VID-006-t00m59s-fog1-confirmed-gold-935-no-message.png` — 0.1 s later: `Gold: 935` (−50), reticle gone, **no text anywhere on screen**, hand and `Next:` unchanged.
- `.claude/pipeline/playtest-evidence/2026-09-04/VID-006-t01m04s-fog2-reticle-up-gold-940.png` — Fog #2 targeting ring up at `Gold: 940`, immediately before the second −50.
- `.claude/pipeline/playtest-evidence/2026-09-04/VID-006-t01m06s-brightsun-placement-circle.png` — **the Bright Sun placement circle**: the ring appears between 01:06.5 and 01:06.8 with the cursor inside it. This is the TASK-1018 pixel.
- `.claude/pipeline/playtest-evidence/2026-09-04/VID-006-t01m08s-brightsun-consumed-gold-834.png` — `Gold: 834` (−60) and hand slot 4 turned over from `Bright Sun 60g` to `Witch 50g` ⇒ the card was consumed.

---

## Suspected mechanism — HYPOTHESIS, NOT VERDICT

**F1 — ⭐ THE STRONGEST CANDIDATE FOR "nothing happened": both cards are missing their cast VFX asset, which is the ONLY per-cast feedback the shipped spell path has.**
`SpellLibrary.cpp:81-96` — the M5 ruling-11 contract spawns `/Game/VFX/NS_Spell_<CardID>` on **every** successful resolve, **null-safe and log-once**. `Content/VFX/` contains `NS_Spell_BattleCry`, `NS_Spell_Fireball`, `NS_Spell_FrostNova`, `NS_Spell_Lightning`, `NS_Spell_Pickpocket` — and **no `NS_Spell_Fog`, no `NS_Spell_BrightSun`**. A successful Fog/BrightSun resolve therefore spawns nothing, and looks *byte-for-byte identical to a card that did not fire*. Predicts the pixels exactly: gold moves, screen does not, at all three casts. **Confidence: high.** Would confirm it: point those two paths at any placeholder Niagara and re-cast — a flash appears at the reticle point with **no code change**.

**F2 — the fog's own visual and the level's volumetric grid are both absent (already boarded; restated only so the manager does not double-board it).** `AFogVolume` is state-only (**TASK-841** owns the visual, unbuilt) and `L_Arena`'s `ExponentialHeightFog_0` has `bEnableVolumetricFog = false` (**TASK-858**, awaiting Jonathan). ⛔ **"I cast fog and the screen did not change" is correct current behaviour and is NOT the defect.** **Confidence: high** (inherited + corroborated: zero screen change 00:58.92–01:12.5).

**F3 — ⚠️ THE ONE UNRESOLVED ANOMALY: Fog cast #1 spent 50 g but did not visibly advance the deck, while Fog cast #2 did.**
`SiegePlayerController.cpp:3428-3495` is `deduct → ResolveSpell → (false ⇒ full refund + "Spell fizzled" + card kept) → (true ⇒ ConfirmPlayFromHand + redraw)`. Both fog casts show the *deducted-and-not-refunded, no-fizzle* signature of a **true** return, so both should have reached `ConfirmPlayFromHand`. Yet `Next:` sat on `Fog` across cast 1 (00:59.0 → 01:04.0) and flipped only at cast 2. Two readings the pixels **cannot** separate: (i) cast 1 legitimately drew the deck's other Fog and the card behind *that* was also a Fog; (ii) cast 1 resolved and charged but the hand did not turn over. Reading (ii) already has a named tripwire in the code — the `UE_LOG(Error, "ConfirmPlayFromHand(%d) refused … hand mutated mid-targeting (should be impossible)")` at `SiegePlayerController.cpp:3487-3492`. **Confidence: low — flagged, NOT asserted.**

**F4 — the reticle (his second complaint), confirmed on pixels; already boarded as TASK-1018.** `SiegePlayerController.cpp:1169` (`PlayHandSlot`'s `case ECardType::Spell:`) routes to `ResolveSpellInstant` only when `Row->SpellEffect == ESpellEffect::GoldSteal`; everything else falls into `EnterTargetingMode`. Same literal at `:3244` (`EnterTargetingMode`'s head) and in the `:4663-4705` comment block. **Confidence: high** — pixels: Fog #1, Fog #2 and Bright Sun all opened a ground ring and all required an LMB confirm. ⛔ Not re-derived; recorded because the fix must not break **F5**.

**F5 — ⚠️ A COUPLING THE TASK-1018 FIX MUST NOT BREAK, and it is not obvious from the diff.** Tonight's `PlayHandSlot` already carries **two refusal gates that run BEFORE the routing switch**: the `FogCover`-refused-while-shielded gate (`SiegePlayerController.cpp:1040-1058`, *"Bright Sun is still up for {0}"*) and the `FogClear`-would-shorten gate (`:1119-1140`). Their own comments state they are **routing-agnostic** and fire identically whether the card goes instant or targeted. **Confidence: high (source-read, not pixel).** ⇒ whoever lands TASK-1018 must route Fog/BrightSun through `ResolveSpellInstant` **without** relocating or duplicating those gates. ⚠️ Note the asymmetry: `ResolveSpellInstant`'s fizzle refund lives at `:4715`, a **different** call site from the targeted one at `:3463` — both must stay reachable once these two effects change lanes.

**F6 — nothing in this video could have shown fog's gameplay effect even if it landed perfectly.** Fog's only mechanical output is the 609.6 uu acquisition clamp in `FSiegeCombatStatics::GatherHostileAgents`, whose seam `ReadFogState` (`SiegeCombatStatics.cpp:120-179`) is now genuinely wired to `AFogVolume::Find` + `IsFogActive()` (TASK-998). **There is no enemy unit on screen anywhere in 00:47–01:13**, and his own two units are parked on a HOLD zone. **Confidence: high (pixel census).** ⇒ *the test he ran cannot pass or fail this feature.*

---

## Routing recommendation

| finding | lane | suggested fix one-liner (hypothesis) |
|---|---|---|
| **F1** `NS_Spell_Fog` / `NS_Spell_BrightSun` absent ⇒ a successful cast is visually indistinguishable from a dead click | **art-director** (author the assets) + **gameplay-programmer** (verify the ruling-11 path is reached) | Create `/Game/VFX/NS_Spell_Fog` and `/Game/VFX/NS_Spell_BrightSun` — the code already looks for them **by name** and no-ops silently when absent. **The cheapest single change that answers his complaint.** |
| **F1b** zero HUD feedback that fog / the prevention window is live | **needs-Jonathan** (design call) → **gameplay-programmer** | A HUD readout — *"Fog 4:52"* / *"Bright Sun 2:00"* — from `AFogVolume::GetFogPreventionSecondsRemaining()`, read live at paint. He already asked for that number inside a **refusal** message (TASK-989/991); the same accessor gives him a **status** line. |
| **F3** Fog cast #1 charged 50 g but the `Next:` card did not advance | **gameplay-programmer** — ⛔ **instrument first, do not fix blind** | Run with `Log LogGitClaudeUnrealTest Verbose`, cast Fog twice, and check for **two** `cast spell 'Fog' for 50 gold` lines and for the `ConfirmPlayFromHand … should be impossible` Error at `:3487`. ⚠️ **An empty log reads as a false pass — set the verbosity FIRST.** |
| **F4** Fog + Bright Sun open a ground circle instead of resolving instantly | **gameplay-programmer** — ⛔ **already boarded as TASK-1018**; confirmed here on pixels only | Replace the `== ESpellEffect::GoldSteal` blacklist-of-one with a data-driven predicate at all three sites (the `SpellDelivery` column already exists in `cards.csv` and is empty for all three rows). |
| **F5** the two pre-switch fog refusal gates must survive the instant-routing change | **gameplay-programmer** (rider on TASK-1018) | Keep `:1040-1058` and `:1119-1140` where they are (before the switch) and re-verify the fizzle-refund path at `:4715` is reachable for `FogCover`/`FogClear` once they route instant. |
| **F6** the fog's only gameplay effect is untestable in the scenario he played | **needs-Jonathan** (playtest recipe) | Next fog test: park two ranged units ~1500 uu from an enemy, cast Fog, watch whether they stop shooting. That is the only observable that separates "fog state landed" from "fog state absent". |
| **S3 side-note** no Pickpocket in this deck | **needs-Jonathan** | His comparison card was not in the deck he played, so this video contains no side-by-side of the instant path. Not a defect. |

---

## Not examined / limitations this pass

- **No audio** (FR-§5). `USiegeFeedbackLibrary::PlayWorldSound(SpellCastSoundPath, …)` fires on a successful resolve — if a cast sound played, it would be *positive* evidence the spell resolved, and I cannot hear it. **Anyone with speakers can settle F3's ambiguity in ten seconds.**
- **Click instants are brackets, not points.** Every confirm is bounded by the last frame at the old gold and the first at the new: Fog #1 `58.91 < t ≤ 58.92` (tight), Fog #2 `64.00 < t ≤ 64.30`, Bright Sun `67.20 < t ≤ 67.40`. **Key gap: I have no full frame inside 64.00–64.30 or 67.20–67.40**, so a toast lasting **under ~0.15 s** at those two clicks would be invisible to me. The 00:58.92 run has no such gap and is clean — and a toast that short would be unreadable to Jonathan anyway.
- **The card-consumption question for Fog cast #1 is genuinely undecidable on pixels** (F3). I have deliberately not picked a side. Deck contents are not observable, and the three-simultaneous-Witches reading at 01:08.0 proves the deck's copy behaviour is not a naive `MaxCopies = 2`.
- **Whether the fog STATE actually landed is not directly observable in this video at all** — no enemies, no HUD indicator, no volume. The gold + no-fizzle evidence is a *contract* inference (F3/F6), not a measurement of `AFogVolume`.
- **00:16.8 – 00:44.2 reviewed at sheet resolution only** (1.52 s tiles, ±0.76 s): hero walking, gold rising monotonically 944 → 970, no card play, no toasts. Not fine-sampled.
- **Whether the Witch is also veiled** at 00:58.5 (two friendly health bars, one resolvable shimmer) — **not determined**; flagged low-confidence, not claimed.
- **Capture compression** (FR-§5): the veil's refractive shimmer and the thin reticle ring are both low-contrast and h264-mangled at sheet scale. Every reticle and veil claim above is made from a **full-resolution frame**, never a sheet tile; every gold and hand-text reading is from a **2×–4× crop**, never a tile.
