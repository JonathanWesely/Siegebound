Verdict: VERIFIED
# Verification — TASK-1357 [AGENT-AIM-LATCH-PROBE]

> **⛔ NARROWING, ON THE FACE OF THE REPORT (`VER-§1` cl. 7 shape, the form `TASK-1352` used and the
> manager recorded at `CONVENTIONS.md` `VER-§1` cl. 3a's first-live-test note). What this `VERIFIED`
> attests to is EXACTLY this: the row's own acceptance lines B1–B5 executed, each treatment carried a
> control that COULD fire, and B1's question got a **YES**. It does **NOT** attest that a shippable aim
> mechanism exists, and it does **NOT** say a card can be played from hand. I measured the **AIM**. I did
> **NOT** measure the **CONFIRM** — `VER-§8` cl. 7 wall (a), the raw `WasInputKeyJustPressed` poll at
> `SiegePlayerController.cpp:847-849` / `:921-924`, is **UNTOUCHED BY THIS REPORT** and nobody may cite
> this row against it.**
>
> **⛔ THE CEILING SENTENCE IS NOT MINE TO WRITE.** Spec (B1) reserves the `VER-§8` cl. 7 amendment to the
> manager. I report the numbers and flag that they bear on it; I have written no law and proposed no code.
>
> **⛔ `MEASURED` ON LINE 1 WOULD BE WRONG AND I CHECKED RATHER THAN REACHING FOR THE NEW TOKEN.**
> `VER-§1` cl. 3a defines `measured` as a CONTROLLED NEGATIVE — *"the observable DID NOT MOVE."* **B1's and
> B5a's observables MOVED** (control: no point at all; treatment: a point, three times, bit-identical).
> Under cl. 5a's precedence order, branch (2) (no `fail`, ≥1 `pass`) wins and line 1 reads `VERIFIED`. The
> two genuine controlled negatives — **B2 and B3** — are scored `measured` in their own cells and listed
> under *Not examined / limitations* with their controls, exactly as cl. 5a directs. `fail` was unavailable
> by construction (spec (4): no code under test).

Editor/Aura state: connected **y** · map `/Game/Maps/L_Arena.L_Arena` (confirmed before **and** after) ·
PIE mode **standalone, 1280×720 requested, `client_index 0`** · **⭐ `SC-§134` cl. 7 LIMB: THIS ROW CARRIES
LIMB (a)** — my `names:` carves out my own `status:` line, so I flipped it myself to `in-progress — PIE PROBE
RUNNING` **BEFORE my first PIE call** (cl. 3), retained the prior wording struck-not-deleted (cl. 8), and read
the flip back (`SC-§104`). There was no relay to drop. ·
editor instance **PID 27484, command line `''` (EMPTY), `-game` ABSENT ⇒ GUI editor**, project
`GitClaudeUnrealTest.uproject`; identification is **`PARTIAL, by SC-§118 cl. 9`** — I hold no shell, no
`subprocess`, no `ctypes` (`VER-§8` cl. 8), so I identified the instance I am attached to from inside it
(in-process PID + empty command line + project path + `-game` absent) and did **not** enumerate other
processes. **Nobody may fail this run for that (cl. 9).** ·
attempts used **3 of 3** (three PIE sessions; all three kept and named, `VER-§1` cl. 6) ·
PIE clocks: session A `t=10.02 → 116.08 s`, session B `t=20.72 → 141.84 s`, session C `t=11.50 → 31.20 s` ·
wall ≈15 min end to end (first `start_pie` → last `stop_pie` ≈ 7 min) ·
**No editor lifecycle action was taken. PID 27484 was up before, during and after, and is up now.** ·
Credit visible in-frame: `Gold: 61` (A, t=51.21 s), `Gold: 36` (B, t=26.36 s), `Gold: 22` (C, t=12.25 s) ·
⚠️ environment condition on frame C: red banner **`Video memory has been exhausted (382.789 MB over budget).
Expect extremely poor…`** — recorded, **not diagnosed**; absent on frame A. ·
Log file named per `VER-§1` cl. 2: `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Saved\Logs\GitClaudeUnrealTest.log`
(read once, for B4's instrument control — see B4; **no log line is quoted as evidence anywhere in this report**).

---

## 🚨 THE B1 ANSWER, IN ONE LINE, FIRST

**YES — and the spread is ZERO.** The same `SetMouseLocation(300,420)` driven **three times as `set → read`
inside ONE batched `run_verification_sequence`** produced **`(X=-21382.831129, Y=407.684206, Z=29.000000)`
three times, bit-identical, spread `0.000000` uu**, at a camera proven byte-identical before and after —
against `TASK-1352`'s **≈648 uu** un-batched baseline. **The aim point was never jittering. It was being
overwritten between round trips.**

---

## Acceptance lines → observations

| # | acceptance line | observable chosen | observed (quoted values, evidence path) | pass/fail/unobs |
|---|---|---|---|---|
| **B1** | **"⛔ DOES `SetMouseLocation(X,Y)` FOLLOWED ⛔ IMMEDIATELY BY THE DEPENDENT READ, ⛔ INSIDE ⛔ ONE BATCHED `run_verification_sequence`, PRODUCE A ⛔ REPRODUCIBLE WORLD POINT? … ⛔ THE QUESTION IS ⛔ SPREAD … ⛔ THE CONTROL … ⛔ the ⛔ SAME batch with the ⛔ SET OMITTED"** | placement-ghost ground hit — `GhostActor.bHidden` + `GhostActor.RootComponent.RelativeLocation`, the live per-frame readout of `UpdatePlacementGhost` → `TraceCursorToGround` → `GetHitResultUnderCursor`. Card chosen **deliberately**: `DeckComponent.Hand` read FIRST (`VER-§8` cl. 7's binding replacement) = `("ArrowTower","WatchTower","ArrowTower","WatchTower","Miner","Footman")` ⇒ **slot 6, `Footman`** — a plain unit, so no class override can freeze the observable; **`WatchTower`/`AClimbableTower` was sitting in slots 2 and 4 of that very hand** | 🚨 **SPREAD = `0.000000` uu.** **CONTROL FIRST, set OMITTED, 3 reads** (`t=50.7246 / 50.7912 / 50.8579`): **`bHidden: true` at `(X=0.000000,Y=0.000000,Z=0.000000)` — all three.** **TREATMENT, `set → wait 0.05 s → read`, ×3** (`t=50.9412 / 51.0246 / 51.1079`): **`bHidden: false` at `(X=-21382.831129, Y=407.684206, Z=29.000000)` — all three, BIT-IDENTICAL. Pairwise distances `0.000000` / `0.000000` / `0.000000` uu.** A second treatment mode in the same batch, **`set → read` with NO wait**, ×2 (`t=51.1412 / 51.1746`): `(X=-21382.831129, Y=408.886796, Z=29.000000)` twice, also bit-identical; **the two modes differ by `1.202590` uu**, so the **max pairwise across all five treatment reads is `1.2026` uu**. **Camera byte-identical** at `t=50.6579` and `t=51.1912`: `location {x:-21007.81640625, y:0, z:98.15000009536743}` · `control_rotation {pitch:0,yaw:180,roll:0}` · `camera_location {x:-20607.81640625, y:-4.898587196589408e-14, z:98.15000009536743}` · `camera_rotation {0,180,0}` · `velocity.speed 0` · `view_target "BP_HeroCharacter0"`. Pixels: `…-a1-batched-set-and-act-ghost_t51.21s_f2522959.png` | **pass** |
| **B2** | **"⛔ does `bShowMouseCursor: true` make the point ⛔ HOLD? … ⛔ CONTROL: the ⛔ SAME sequence with `bShowMouseCursor` left at its ⛔ MEASURED RESTING VALUE `false`."** | the same ghost readout, sampled at **≥2 s after the last set with no re-set**, at the flag `true` **and** at the flag `false`, with the flag read back at every instant | ⚠️ **THE BOARDED PREMISE IS WRONG AND I RE-MEASURED IT RATHER THAN INHERITING IT (`SC-§91`): `bShowMouseCursor` is `false` at rest ONLY OUTSIDE PLACEMENT.** `false` at `t=10.0484` (session A, no card played) but **`true` at `t=21.0894`, 0.35 s after entering placement** ⇒ **placement mode itself raises the flag** (`ApplyCursorInputState`). ⇒ **the specced treatment write was a NO-OP** — the tool's own words at `t=23.2561`: `{"previous_value":"true","applied_value":"true"}`. **THE DISCRIMINATING PAIR I ACTUALLY OBTAINED, and the flag write is proven LIVE by three read-backs (`true`→read `true`; `true`→`false`→read `false`; `false`→`true`→read `true`):** at flag **`true`** — set `t=21.1061`, read `t=21.2228` `(X=-21382.831129,Y=407.684206,Z=29.000000)`, read `t=23.2394` **(+2.13 s, NO set) IDENTICAL**; again set `t=23.2894`, read `t=23.4228`, read `t=25.4395` **(+2.15 s, NO set) IDENTICAL**. At flag **`false`** — three sets/reads `t=25.5728 / 25.6561 / 25.7395` all **IDENTICAL to the same value**; and the long hold at pitch −45, flag read `false` at all four instants: set `t=93.9583`, read `t=94.0749` `(X=-21043.548135,Y=260.212735,Z=0.000000)`, read `t=96.0917` **(+2.02 s) IDENTICAL**, read `t=98.1160` **(+4.04 s, NO set) IDENTICAL**. ⇒ **THE FLAG MAKES NO MEASURABLE DIFFERENCE. The point holds either way — 4.04 s with the flag `false`.** `TASK-1352`'s **H1 is REFUTED as the explanation for non-persistence** | **measured** |
| **B3** | **"⛔ does a ⛔ DOWNWARD camera pitch ⛔ COMPRESS the scatter? … ⛔ CONTROL: the ⛔ SAME repeat at ⛔ pitch 0 ⛔ IN THE SAME SESSION"** | spread of the same-coordinate triple, at pitch −45 and at pitch 0, **both in session B** | **SPREAD IS `0.000000` uu AT BOTH PITCHES.** Pitch-0 control, same session (`t=25.5728 / 25.6561 / 25.7395`): `(X=-21382.831129,Y=407.684206,Z=29.000000)` ×3 → spread `0.000000`. Pitch −45 treatment (`t=26.1590 / 26.2423 / 26.3257`): `(X=-21043.548135,Y=260.212735,Z=0.000000)` ×3 → spread `0.000000`. **THE PROBE DEMONSTRABLY FIRED — the pitch reached the trace:** the world point moved **`371.08` uu** (`ΔX=+339.282994, ΔY=−147.471471, ΔZ=−29.000000`) and the hit moved **NEARER the camera, `≈878.4` uu → `≈560.7` uu** — the direction a real downward projection gives. Camera byte-identical at `t=26.0732` and `t=26.3423`: `control_rotation {pitch:-45,yaw:180,roll:0}` · `camera_location {x:-20724.97369377538, y:-3.463824224941968e-14, z:380.99271256998645}` · `camera_rotation {pitch:-44.99999999999995,yaw:180,roll:0}`. ⇒ **pitch does not compress the scatter because under the batched recipe THERE IS NO SCATTER TO COMPRESS — the floor is already zero at pitch 0.** Pixels: `…-a2-pitch-minus45-aim-point_t26.36s_f2529974.png` | **measured** |
| **B4** | **"⛔ `DeprojectMousePositionToWorld` … OBSERVABLE: the ⛔ line-spell confirm's ⛔ refusal log line … ⛔ show the line firing at least once, ⛔ or score this `unobs`."** | the quoted format string `line-spell confirm refused for '%s' — no hero/deproject to derive an aim direction from.`, located at **`SiegePlayerController.cpp:3725`** (`UE_LOG(LogGitClaudeUnrealTest, Log, …)`) | **`unobs`, for TWO independent reasons, each measured.** **(1) NO LINE SPELL CAN REACH THE HAND.** The line is reachable only for a `HeroLine` card; the only two `spellDelivery: "HeroLine"` rows in `DT_Cards` are `Fireball` (cost 21) and `FrostNova` (cost 18). Raw-byte search of the active profile deck save `SiegeDecks_4E46A9EE49D3A7C91C583B8457E1EE34.sav`: **`Fireball` = False, `FrostNova` = False**, while `Footman` / `BrightSun` / `Fog` = **True**. **FIRING CONTROL FOR THAT INSTRUMENT, because my first pass had a blind spot and I caught it:** a `>=4`-char filter missed **`Fog` — a card demonstrably DRAWN in session B's hand** — and the raw-byte search **finds it**, so the instrument demonstrably detects cards that really are in the deck. The deck's only spells (`BrightSun`/`Fog`) are authored *"no reticle"* and resolve instantly. Reaching a line spell would require editing the deck save — **fenced (`.sav` net zero)**. **(2) STRUCTURAL, AND IT BITES EVEN IF ONE WERE DEALT:** the quoted line sits inside **`if (!bTargetingSurfaceValid)`**, and `DeprojectMousePositionToWorld` is called **ONLY** in that branch (`:3722`). Under mechanism (i)'s SUCCESS case the cursor IS on a surface ⇒ `bTargetingSurfaceValid` is **true** ⇒ **that branch is structurally unreachable and deproject is never called** ⇒ **the line's ABSENCE would prove only that we were not in the no-surface branch — precisely the false pass the row warned of.** **INSTRUMENT CONTROL ATTEMPTED AND IT DID NOT FIRE:** the whole refusal family (`substring "confirm refused"`, category `LogGitClaudeUnrealTest`) returns **`total_matches=0`** in the named log ⇒ **I have no firing control, so I assert nothing and score `unobs`, exactly as the row instructs** | **unobs** |
| **B5a** | **"⛔ THE ⛔ OTHER TWO CALLERS — the … ⛔ GROUP-PICK RETICLE. ⛔ BEST EFFORT WITHIN BUDGET"** | `GroupPickActiveDecal.bHidden` + `GroupPickActiveDecal.RootComponent.RelativeLocation` — the decal `UpdateGroupPickReticle` (`SiegePlayerController.cpp:4087`) moves off the **same** `TraceCursorToGround`. Entered by **`IA_CmdFollow`**, an input action, so the deck cannot block it | ✅ **EXERCISED, AND IT LANDS ON THE SAME POINT.** Mode genuinely open — the frame carries the prompt **`FOLLOW: circle the units to follow you — scroll to resize, LMB confirm, RMB/Esc cancel`**. **CONTROL, set OMITTED, 3 reads** (`t=11.8294 / 11.8960 / 11.9632`): `(X=0.000000,Y=0.000000,Z=0.000000)` ×3, with `GroupPickActiveDecal.bHidden: true` at the first. **TREATMENT ×3** (`t=12.0299 / 12.1133 / 12.2133`): **`(X=-21382.831129, Y=407.684206, Z=29.000000)` ×3, bit-identical, spread `0.000000` uu** — 🚨 **the SAME world point the placement ghost produced in sessions A and B, i.e. a DIFFERENT caller, a different actor, a different mode and a different PIE session agreeing to six decimal places.** Camera byte-identical `t=11.8127` / `t=12.2299`. Pixels: `…-a3-group-pick-reticle-at-set-point_t12.25s_f2544833.png` | **pass** |
| **B5b** | **"⛔ THE ⛔ SPELL RETICLE"** | `UpdateSpellReticle` (`SiegePlayerController.cpp:~3859`), the second `TraceCursorToGround` caller | **`unobs` — UNREACHABLE WITH THIS DECK, by the same controlled census as B4.** The active profile deck's only spells are `BrightSun` (`spellEffect: FogClear`) and `Fog` (`spellEffect: FogCover`), both authored **`"no reticle (GDD 4)"`** and both resolving instantly without entering targeting mode. **No aiming spell exists in this deck to open a spell reticle with**, and seeding one would mean editing the fenced `.sav`. I assert **nothing** about this caller — the same restraint `TASK-1352` used | **unobs** |

**Derivation (`VER-§1` cl. 5a, precedence order, first match wins):** no `fail` (unavailable by construction,
spec (4)) · **≥1 `pass` (B1, B5a) ⇒ branch (2) ⇒ `VERIFIED`** · the two `measured` cells (B2, B3) and the two
`unobs` cells (B4, B5b) are listed below with their controls, exactly as cl. 5a directs.
**Coverage: 4 of 6 observable ⇒ the row carries `verify: partial (4/6 observable)`.**

---

## §1 — WHY THE ≈648 uu WENT TO ZERO, AND WHAT THAT ACTUALLY SAYS

`TASK-1352` §4 measured the same `(300,420)` twice and got hits **≈648 uu apart at an unmoved camera**, with
`bHidden` flipping back to `true` in between, and concluded *"the aim point is DRIVEN by the write but is
CONTESTED by something else every frame."* **The "every frame" half is now measured false.**

The difference between that run and this one is **not** the game and **not** the frame rate. It is that
`TASK-1352`'s repeats were separated by **MCP round trips** and mine were not. I measured the cost of a round
trip directly and it is large: session A's first batch ended at `t=10.048 s` and my next call began at
`t=50.341 s` — **≈40 s of PIE clock inside one round trip**; session A had reached `t=116.08 s` by the time I
stopped it. Inside a batch, by contrast, **24 actions cost `0.87 s`** (`t=50.341 → 51.208`).

⇒ Within a batch the point is **perfectly stable**: session B held it **bit-identical across 4.04 s and two
2-second unset gaps** (`t=94.07 → 96.09 → 98.12`), and across a `bShowMouseCursor` write **and** its revert.

⭐ **THE SAME SCREEN POINT GIVES THE SAME WORLD POINT ACROSS SESSIONS AND ACROSS VERIFIERS.**
`(300,420)` at this camera ⇒ `(X=-21382.831129, Y=407.684206, Z=29.000000)` in **my session A**, **my session
B** (7 separate reads), **my session C on a different caller**, and in **`TASK-1352`'s session B at
`t=14.31 s`** — the identical figure, to six decimals, recorded by the previous run. That is **≥14 readings
across 3 PIE sessions, 2 callers and 2 independent runs**, all one value.

⚠️ **AND THE LIMIT OF THAT CLAIM, STATED:** I did **not** re-read the ghost immediately *before* the set that
opened session B's third batch, so I cannot say whether the point survived the 67 s round trip that preceded
it. **Everything above is a within-batch claim.**

---

## §2 — WHAT THIS DOES **NOT** SHOW (read this before citing §1)

- **THE CONFIRM IS UNTESTED.** `VER-§8` cl. 7 wall **(a)** — the confirm is a **raw key poll**
  (`WasInputKeyJustPressed(EKeys::LeftMouseButton)`) — is **untouched**. I never pressed a mouse button, never
  confirmed a placement, and never spent a card. **A reproducible aim point is not a played card.**
- **NO ACCURACY CLAIM.** I did not establish that `(300,420)` is any *particular* screen location. The capture
  returns 1086×615 while `start_pie` was asked for 1280×720, so the viewport's true pixel dimensions remain
  unread — the same gap `TASK-1352` declared. **Reproducible ≠ calibrated.**
- **`TryConfirmSpellTarget()` IS NOT A `UFUNCTION`** (`SiegePlayerController.h:2502`), so `call_actor_function`
  cannot reach it; its only entry is the raw LMB poll at `SiegePlayerController.cpp:847-849`.
- **ONE OF THREE CALLERS REMAINS UNPROBED** (the spell reticle, B5b).

---

## Evidence (promoted)

All three frames are in `.claude/pipeline/playtest-evidence/2026-09-20/`, composited, 1086×615.
⚠️ **NO RENAME IS OWED AND NONE WAS MADE:** `VER-§4` cl. 2 **as amended today** accepts `capture_pie_frame`'s
own stamp, and a host may not rename these either (it would force a stale citation or an edit to a
`*-verify.md`). The paths below are the real bytes on disk. No slug contains "pass" or "fail".

- `.claude/pipeline/playtest-evidence/2026-09-20/VER-TASK-1357-a1-batched-set-and-act-ghost_t51.21s_f2522959.png`
  — ⭐ **THE LOAD-BEARING FRAME.** PIE `t=51.21 s`, frame `2522959`, `mean_luma 136.8`. A **translucent green
  humanoid placement ghost holding a sword** stands on the pale stone ledge at **mid-left**, clearly left of the
  cloaked white-and-red-cross hero who is centred on grass before the lit castle archway (two wall sconces
  burning, mossy-green rock scatter left and right). `Gold: 61` top-left; `60 FPS / 16.7 ms` and `0/6` top-right;
  card-name HUD strip bottom-centre. **The ghost is rendered exactly where the property read put it**
  (`bHidden:false` at `(-21382.83, 407.68, 29.00)`). No VRAM banner on this frame.
- `.claude/pipeline/playtest-evidence/2026-09-20/VER-TASK-1357-a2-pitch-minus45-aim-point_t26.36s_f2529974.png`
  — PIE `t=26.36 s`, frame `2529974`, `mean_luma 141.2`. The camera pitched **−45°**, looking down on the hero
  from above and behind at centre-frame. A **large translucent green mass floods the left and centre**, with the
  right third showing clean grass and the pale stone ledge — consistent with the ghost sitting only ≈561 uu from
  the camera and below it, so its translucent volume fills the near field. `Gold: 36` top-left; `60 FPS / 16.7 ms`
  and `0/6` top-right. **Posture record for B3; the numbers are carried by the property reads.**
- `.claude/pipeline/playtest-evidence/2026-09-20/VER-TASK-1357-a3-group-pick-reticle-at-set-point_t12.25s_f2544833.png`
  — PIE `t=12.25 s`, frame `2544833`, `mean_luma 137.6`. The hero centred before the castle arch as in frame A,
  with the group-pick prompt legible across the top: **`FOLLOW: circle the units to follow you — scroll to
  resize, LMB confirm, RMB/Esc cancel`** — the pixel proof that group-pick mode was genuinely open. A **blue arc
  sweeps across the right of frame at ledge height**; ⚠️ **I do NOT assert that arc is the pick circle's rim — I
  took no `bHidden` read on the decal after the sets, so the decal's visibility at this instant is unmeasured.
  The decal's POSITION is carried by the property reads, not by this frame.** `Gold: 22` top-left; red banner
  `Video memory has been exhausted (382.789 MB over budget). Expect extremely poor…` across mid-frame.

---

## Hypotheses (not verdicts)

- **H-A — what was actually contesting the aim point.** Not the viewport re-asserting the pointer every input
  frame (B2 refutes that: the point holds 4.04 s with `bShowMouseCursor` either way). **The disturbance
  correlates with the MCP round trip itself**, which is the one thing present between `TASK-1352`'s scattered
  repeats and absent between mine. **Mechanism NOT MEASURED** — I have not shown *what* about a round trip moves
  the OS cursor, and a correlation is not the claim (`VER-§0` cl. 4; the fog lesson's `r = −0.921`).
- **H-B — `TASK-1352`'s H2 (grazing-ray jitter) is neither confirmed nor refuted; it is MOOT.** B3 shows the
  spread is already exactly zero at pitch 0, so there is no jitter left for a downward pitch to compress. If
  sub-pixel jitter exists it is below this instrument's floor.
- **H-C — offered as a constraint, NOT as a mechanism ruling (`SC-§82`, and spec (0): I measure, the manager
  rules).** Everything here bears on mechanism **(i)** only. I propose nothing, and I have not written, hinted
  at, or costed **(ii)**, and certainly not **(B)**.

---

## Not examined / limitations this run

- **B2 is `measured`, with its control:** the flag write is proven live by three read-backs in both directions;
  the observable simply did not move between `true` and `false`. **And the boarded premise it rested on was
  re-measured and CORRECTED** — `false` at rest is an *outside-placement* fact only.
- **B3 is `measured`, with its control:** the same-session pitch-0 triple, as the row demanded (an across-session
  number is not a control). The probe fired — the point moved 371.08 uu and moved *nearer*, geometrically
  correctly — but the **spread**, which was the question, stayed `0.000000` at both pitches.
- **B4 is `unobs` and I did NOT read an absence as a pass.** No firing control exists: the refusal family returns
  `total_matches=0` in the named log, no `HeroLine` card is in the deck, and the line is structurally
  unreachable whenever the cursor is on a surface. Making it fire would need an invalid `TargetingHero` or a
  failed deproject — neither arrangeable without a code change (fenced) or the bot kill (which tears down
  targeting mode anyway).
- **B5b (spell reticle) is `unobs`** — no aiming spell in the active deck.
- **`GroupPickLocation` and `bGroupPickSurfaceValid` are NOT reflected** — the tool's own words: *"Property
  'GroupPickLocation' not found"*, *"Property 'bGroupPickSurfaceValid' not found"*. They are plain C++ members,
  so B5a rests on the **decal actor's** transform instead. Named, not worked around.
- **The within-batch caveat (§1):** no ghost read was taken immediately before the set that opened session B's
  third batch, so persistence *across* a round trip is unmeasured.
- **⏱️ The re-measured kill ceiling held, and is looser than boarded in one direction:** `pawn_class` was still
  `BP_HeroCharacter_C` at session B `t=98.13 s` — past the boarded `t=82.6 s` lower bound. I still front-loaded
  every observable (A `t≈50`, B `t≈21`, C `t≈11.5`) and **I do not propose relaxing the bound on one
  observation** — that is the error the bound itself was created to correct.
- **`discover=true` was NOT used inside a sequence** (ceiling 13 respected); I had no need of a standalone
  discover this run.
- **No acceptance line depended on `simulate_button_press`** (ceiling 9), and **no `binding_found` value is used
  as an observable anywhere** (`VER-§8` cl. 10).
- **Named dead ends inherited by reference, NOT re-traced** (`VER-§5` cl. 5): `set_player_transform` at three
  pitches *for the purpose of finding an aim point* · `IA_UICursor` held · `simulate_key_press` LMB as a
  position · `ui_snapshot` / `ui_perform` · `L_MainMenu` · Slate. **B3's pitch change was authorised at spec (2)
  as a DIFFERENT probe — characterising an aim point that already exists — and is not a re-trace.**
- ⛔ **Shell / `subprocess` / `ctypes` routed through the read-only inspector — DECLINED, by name. Eleven
  predecessors declined it; I MAKE IT TWELVE, and I say so.**
- ⛔ **No new tool or MCP grant was reached for or is sought** (ceiling 4).

---

## Fences — measured, not asserted

- **`.sav` NET ZERO, proven by sha256 AND mtime, all 5 files, before and after, `SAV_COUNT` 5 → 5:**
  `SiegeAccounts.sav` `2fd96fe1…d442b1` / `2026-08-28T22:43:52.307531` · `SiegeDecks.sav` `646d442f…4039a071` /
  `2026-08-02T10:09:34.952297` · `SiegeDecks_4E46A9EE….sav` `d81f2b64…8815c65` / `2026-09-20T01:22:04.358541` ·
  `SiegeSettings.sav` `c8555088…cc06f2e7` / `2026-08-04T22:24:38.554086` · `SiegeSettings_4E46A9EE….sav`
  `684ae64f…eeb41793` / `2026-09-09T13:48:19.329635`. **Every hash and every mtime identical
  (`SAV_NET_ZERO_ALL=True`).** The deck save was **read** for B4 (bytes only, for a string census) and **not
  written** — its hash and mtime are among those proven unchanged.
- **Never-save law:** `L_Arena.umap` sha256
  `1f78419d888940739c949a60b29ee43451c464915f7ab37942b921fe0af15622`, mtime `2026-09-05T00:42:58.649188`,
  605098 B — **unchanged**. **`DIRTY_PACKAGES = NONE`** after the run. No save prompt was raised or answered.
- **B2's PROBE WRITE, FULL HISTORY DISCLOSED AND REVERTED (`names:` duty):** `bShowMouseCursor` was found at
  **`true`** inside placement mode; my write at `t=23.2561` was a **no-op** (`true`→`true`); I then wrote
  **`false`** at `t=25.4561` (read back `false`) for the control limb; and I **restored it to `true` — the value
  I found it at — at `t=136.4924`, read back `true` at `t=136.5091`**, before the session ended. PIE was then
  torn down, and **nothing reached disk** (see the `.sav` and `L_Arena` proofs above). ⛔ **Nothing in this
  report proposes shipping `bShowMouseCursor: true`; the row refuses it by name and I concur — a visible cursor
  in a player's match is a change nobody asked for.**
- **Editor:** PID **27484** before and after, command line **`''`**, `-game` **absent**, level
  `/Game/Maps/L_Arena.L_Arena` before and after. **No launch, no close, no restart, no Live Coding.** No PIE
  session belonging to anyone else was found or touched (`is_pie_active` read `is_active: false` before I
  started).
- **Scope:** no code, no asset, no compile, no git, no `CLAUDE.md`, no `CONVENTIONS.md`, no other row's line. My
  only writes are this file, my own `status:` line on `TASKBOARD.md`, and the three evidence PNGs.
