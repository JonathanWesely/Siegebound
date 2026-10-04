<!-- MOVED from .claude/pipeline/CONVENTIONS.md on 2026-10-04 by Tools/split_conventions.py. Sections are byte-identical to the original; this comment is the only addition. Cite clauses by tag (e.g. `VER-§3 cl. 6`); CONVENTIONS.md '## Law index' maps tags to files. -->
## ⚖️ THE VISUAL PUNCHLIST — VID-004's six cosmetic rows + the three UNTESTED paths (2026-09-02) — namespace **VIS-§**

*Source: `.claude/pipeline/footage/VID-004-contact-climb-confirmed-visual-punchlist.md`. Jonathan's words were **"there are some minor visual things to fix but I think it works good enough for now"** — ⛔ he did **not** itemise them; the analyst itemised six (V1–V6). ⭐ **Every claim in `VIS-§1`..`§3` below was RE-VERIFIED AT SOURCE by the manager before this section was written, and ⛔ two of the analyst's routing recommendations did ⛔ NOT survive that check.** Sections cite as `VIS-§N`, ⛔ never bare `§N`.*

### VIS-§0. ⭐⭐ THE HEADLINE IS ⛔ NOT A PUNCHLIST ROW — IT IS A **CLOSE**, AND IT IS THE PIXEL EVIDENCE FOR `TOWER-§10` `L-1`

**The brick test is CONFIRMED ON PIXELS, ≥ 6 TIMES.** At **01:20.5** a red-hooded Archer emerges through the deck plane at the ladder head **while the hero is already fully up ~2 m away** — hero-up and second-climber-ascending **in one frame** — and by **02:14.5** there are **seven pawns on the deck** (`HOLD set: 6 unit(s)`). ⇒ ⭐ **the occupancy slot CLAIMS AND RELEASES AND CYCLES.** `TASK-787`'s two-seam widening is corroborated in the world, ⛔ not merely in the suite.
⚖️ **AND THE EVIDENCE CLASSES STAY SEPARATE, BECAUSE THAT IS THE WHOLE VALUE:** Jonathan **reported** it (*"yes, units were able to climb the ladder after the hero did"*); the frames are an **independent pixel corroboration**. ⭐ **The analyst kept his claim and its frames apart, and this board does too — ⛔ a report that agrees with a measurement is ⛔ two facts, ⛔ not one.**

### VIS-§1. ⭐⭐ **V4 — THE VERDICT: THE BLUE IS THE ⛔ TEAM SLOT WORKING AS SPECIFIED. IT IS ⛔ NOT A MISSING MATERIAL.** *(the source check the dispatch demanded, and it lands DECISIVELY)*

⛔ **This row was boarded as a MEASUREMENT and ⛔ NOT as an art fix, and here is exactly why — every line cited:**

| # | fact | source, verified |
|---|---|---|
| 1 | `SM_WatchTower` ships **two** slots, `[TeamRegion, WatchTowerPBR]` | `Tools/ArtPipeline/build_watchtower.py:1694,1715` · manifest `WatchTower.team_region` |
| 2 | slot 0 `TeamRegion`'s authored base colour is **`(0.05, 0.30, 1.00)`**, flat, **⛔ no texture node at all** | `build_watchtower.py:1699` — the comment on that very line reads `# MI_TeamColor_Blue` |
| 3 | that is **byte-identical** to the Team contract's `MI_TeamColor_Blue` = linear `(0.05, 0.30, 1.00)` | this file, **Team contract** |
| 4 | ⭐ at runtime `AClimbableTower` **is** a building, so `ABuilding::ApplyTeamVisuals` **overwrites slot 0** with `MI_TeamColor_Blue`/`_Red` | `ClimbableTower.h:219` (`: public ABuilding`) · `Building.cpp:166-172` |
| 5 | `M_TeamColor` exposes a **flat vector parameter** `TeamColor` — ⛔ there is **no** texture in that path **by design** | this file, **Team contract** |

⇒ ⭐⭐ **"UNTEXTURED FLAT CORNFLOWER-BLUE" IS ⛔ EXACTLY WHAT A CORRECTLY-ASSIGNED TEAM SLOT LOOKS LIKE.** ⛔ **No art task may be raised to "assign the missing material" — there is ⛔ none missing.**

⚠️⚠️ **AND THE NEAR-MISS IS RECORDED, BECAUSE IT ALMOST BECAME A LAW:** a grep of `ClimbableTower.cpp` for `SetMaterial`/`MI_TeamColor`/`ApplyTeam` returns **ZERO matches**, and I was one step from boarding *"the tower never applies its team colour"* as a defect. ⛔ **It applies it — in its BASE CLASS.** ⇒ ⭐ **STANDING RULE: a grep of the LEAF class is ⛔ NOT evidence about an INHERITED behaviour. Resolve the base chain first, ⛔ or do not make the claim.**

**⛔ WHAT THE TEAM SLOT ⛔ CANNOT EXPLAIN, AND THIS IS THE PART THAT IS STILL OPEN:** `TeamRegion` is **MEASURED at 12 of 341 faces = `team_area_fraction` 0.0283 — ⛔ 2.83 % of surface area** (manifest `_verified`, TASK-737), and its selector is **the deck-top 75 uu border ring (a FLUSH INLAY) + the deck slab's 40 uu outer rim, and ⛔ NOTHING ELSE** (`build_watchtower.py:1615-1638`). ⇒ ⛔ **2.83 % ⛔ cannot paint a whole deck.** **The deck's centre panel (`|x|,|y| ≤ 225`) is `FAM_PAVING` on slot 1 `WatchTowerPBR` and ⛔ must read as STONE.** ⭐ **The analyst's own report contains the likely reconciliation and he flagged it without naming it: *"a pale, hard-edged wash covers the slab's left half … that does not match the rest of the surface."* ⇒ that is plausibly the paving centre panel, bounded by the ring's hard inlay edge.** ⛔ **PLAUSIBLE IS ⛔ NOT MEASURED — it is `TASK-792`'s single question.**

### VIS-§1a. ⛔ THE OVERHANG IS **AUTHORED AND EXACTLY 24 uu** — AND THE "NOTHING BENEATH" IS THE **DELIBERATE WEST-BAY GAP**

- `SHAFT_HALF = 276.0` vs `DECK_HALF = 300.0` ⇒ the deck overhangs the wall by **exactly 24 uu per side**, and the reason is written on the line: **"walls inset 24 uu so the deck reads as a cap"** (`build_watchtower.py:185`).
- Beneath it sits the **corbel band**, `z ∈ [1090, 1160]`, out to `DECK_HALF` — present on **N / S / E and both west piers**, and ⛔ **GAPPED across the west bay `|y| < 130` (260 uu)** because a band drawn across the opening *"would sit exactly where the climbing capsule passes"* (`:424-433`).
- ⭐⭐ **THE WEST BAY IS THE LADDER FACE — i.e. ⛔ the exact face the camera watches for the entire climb.** ⇒ **"overhangs the stone body with nothing beneath" is ⛔ REAL, is ⛔ 24 uu, and is ⛔ the standoff budget being spent on purpose.** ⛔ **Closing that gap is ⛔ NOT a free cosmetic fix — it spends clearance `CONTACT-§7a` bought at the cost of a whole art lane.**
- ⚠️ **`~180 px` is ⛔ NOT `~180 uu` and the conversion is ⛔ not derivable from the capture.** ⛔ **Do not board a fix against a pixel count.**

### VIS-§1b. ⚠️⚠️ **THE CAVEAT THAT BINDS ⛔ ALL OF V4 AND V5: THE CAPTURE PREDATES THE REIMPORT.**

**VID-004 was shot BEFORE `TASK-780`'s same-path reimport of `/Game/Meshes/SM_WatchTower` + the three re-baked `T_WatchTower_{D,N,ORM}`** (`CONTACT-§13`). ⇒ ⛔ **the in-engine asset those frames show may be an ⛔ OLDER BUILD ENTIRELY.** ⭐ **And there is an eerie corroboration of exactly that: run 1 of the build DID paint the corbel band team-blue and it was ⛔ REJECTED ON THE PREVIEW for reading as *"a solid blue slab hanging under the deck"* (`:1623-1626`) — which is ⛔ nearly the analyst's words.** ⇒ ⛔⛔ **NO V4/V5 FIX MAY BE AUTHORED UNTIL THE PUNCHLIST IS RE-CONFIRMED AGAINST THE ⛔ REIMPORTED MESH.** ⚖️ *Fixing a defect that a pending reimport already removed is the most expensive kind of nothing.*

### VIS-§2. ⭐⭐ **V3 — THE HEALTH BARS ARE ⛔ NOT A BUG. `EWidgetSpace::Screen` HAS ⛔ NO DEPTH TEST ⛔ BY ENGINE DESIGN.**

**`CombatantHealthBarComponent.cpp:30` — `SetWidgetSpace(EWidgetSpace::Screen);`** A Screen-space `UWidgetComponent` is composited as a **Slate overlay**; it ⛔ **never participates in the depth buffer**. ⇒ ⭐ **"bars drawn over solid stonework" and "four bars floating over an empty deck while their pawns stand a tower-height below" are the ⛔ DEFINED behaviour of the widget space this code chose — ⛔ not a regression, ⛔ not a broken setting.**
⚠️ **AND IT IS ⛔ THREE CALL SITES, ⛔ NOT ONE** — a blanket widget-space switch is ⛔ roster-wide, ⛔ not local: `CombatantHealthBarComponent.cpp:30` · `Castle.cpp:287` (`HPBarWidget`) · `DamageNumberActor.cpp:31` (`NumberWidget`).

⚖️⚖️ **MANAGER RULING — `VIS-R1`, a PROCEEDING DEFAULT, ⛔ overrulable by Jonathan in one word:** the fix is a **camera-occlusion VISIBILITY CULL on the existing Screen-space component**, ⛔ **NOT** a switch to `EWidgetSpace::World`.
- **Why:** `World` space costs **a render target per bar** across a 20+ pawn roster, changes every bar's scale/legibility law, and would have to be paid at all three call sites — ⛔ **that is a rendering-architecture change bought to fix a polish item.**
- **The cull is local, cheap, and reversible:** one trace from the active camera to the bar's world anchor; blocked ⇒ hide. ⛔ **Scope is `CombatantHealthBarComponent.{h,cpp}` ONLY** — ⛔ `Castle.cpp` and `DamageNumberActor.cpp` are ⛔ OUT (the castle bar sits at `z = 9450` and has never been reported; a damage number is transient by nature).
- 🧑 **FLAGGED FOR JONATHAN, ⛔ NOT ASKED AS A BLOCKER:** if he wants genuinely 3-D bars that sort in the world, that is the `World`-space change and it is a **separate, larger** batch.

**PINNED NAMES (`VIS-§2` is the naming law for this row — ⛔ the assignee may ⛔ not rename them):**
| thing | pinned name |
|---|---|
| toggle | `bOccludeHealthBarWhenBlocked` — `UPROPERTY(EditDefaultsOnly, Category="Siegebound|HealthBar")`, default **`true`** |
| trace channel | `HealthBarOcclusionChannel` — `TEnumAsByte<ECollisionChannel>`, default **`ECC_Visibility`** |
| poll period | `HealthBarOcclusionIntervalSeconds` — `float`, default **`0.15f`** ⛔ (**⛔ NOT per-frame** — `CombatantHealthBarComponent.cpp:28-29` records that this component has **no** poll timer and updates arrive by delegate; a per-frame trace per pawn is a roster-wide cost) |
| the query | `bool IsHealthBarOccluded() const` — **`BlueprintPure`**, the QA readback hook (the `GetPermanentDamageMultiplier` precedent) |

### VIS-§3. ⭐ **V2 — THE CAMERA COLLAPSE IS THE SPRING ARM'S ⛔ DEFAULT COLLISION PROBE, PROVED BY ABSENCE**

**`bDoCollisionTest`, `ProbeSize` and `ProbeChannel` have ⛔ ZERO occurrences in ⛔ ALL of `Source/`** — verified by grep. ⇒ they sit at their **engine defaults** (`bDoCollisionTest = true`, `ProbeSize = 12`, `ProbeChannel = ECC_Camera`). `CameraBoom` is built at `GitClaudeUnrealTestCharacter.cpp:39-42` with `TargetArmLength = 400.0f` and ⛔ **nothing else configured**; `AHeroCharacter : public AGitClaudeUnrealTestCharacter` (`HeroCharacter.h:398`) inherits it unmodified.
⇒ ⭐ **walking a 400 uu boom into a 1200 uu tower puts the probe inside the tower's hull and the arm collapses to ≈ 0 — which is ⛔ precisely the 01:12.0–01:13.5 observation (back/hips filling 60–70 % of frame for ≈ 2 s).** ⚠️ **And it cost the analyst the attach instant itself: the ~01:12–01:13.5 window is ⛔ occluded by this defect, so the dwell-to-attach transition in VID-004 is ⛔ INFERRED, ⛔ not observed.**

⚖️ **MANAGER RULING — `VIS-R2`, ⛔ a FENCE, not a technique:** the fix is authored **on `AHeroCharacter` only**, configuring the **inherited** `CameraBoom`. ⛔⛔ **`GitClaudeUnrealTestCharacter.{h,cpp}` is ⛔ OUT OF BOUNDS — it is the UE TEMPLATE base and is also inherited by `Variant_Combat` / `Variant_Platforming`.** ⚖️ *A camera tuning for one game's hero may ⛔ never be paid by three unrelated template characters.*

**PINNED NAMES:** `HeroCameraProbeSize` (`float`, `EditDefaultsOnly`) · `bIgnoreClimbableGeometryForCamera` (`bool`, default **`true`**) · `MinCameraArmLengthUU` (`float`) — ⛔ **all three flagged for Jonathan's feel pass, ⛔ none of them invented as final.**

### VIS-§4. ⛔⛔⛔ **THE LAW THIS REPORT BUYS: A PATH THE FOOTAGE DID ⛔ NOT EXERCISE IS ⛔ OWED — ⛔ IT IS ⛔ NEVER "PASSED".**

⚠️ **VID-004 reads as a success, and that is ⛔ exactly what makes this dangerous: *"it seems to work fine"* + a corroborated headline is the state in which an untested path gets quietly filed as a working one.**

**THE THREE, RECORDED AS UNANSWERED QUESTIONS AND ⛔ NOT AS CLEAR ROWS:**

1. ⭐⭐ **THE DESCENT IS ⛔ ENTIRELY ABSENT — AND IT IS THE HIGHEST-VALUE UNTESTED ROW IN THE CAPTURE.** The hero mounts at **01:17.0** and is **still on the deck at 02:14.5** (the final frame). ⛔ **No pawn descends at any point.** ⚠️⚠️ **Climbing DOWN runs the same path with its ⛔ OWN failure mode — and the descent is ⛔ precisely why `TOWER-§8.5a`'s non-swept window is resolved by ⛔ `Z` rather than by "the last stretch": ⛔ on the way down, "the last stretch" is at the ⛔ OTHER END OF THE LINE.** ⇒ ⛔ **the `K-C` re-arm latch, the deck→ladder handover and the `§8.5a` window are ⛔ ALL untested downward.**
2. **SPRINT — THE STATED WORST CASE, AND IT IS UNTESTED.** Gait at 01:11.5 reads jog-or-faster, ⛔ **but `WalkSpeed 500` vs `SprintSpeed 750` is ⛔ NOT separable on stills** (no velocity readout; `FR-§5` — Claude reads stills, not motion). ⚠️ **Sprint is what drove `K-6.1` to `350` (937.5 uu/s with Swift Boots ⇒ `R/v = 0.320 s` against a `0.35 s` dwell), so ⛔ the number the batch changed is the number the footage ⛔ cannot confirm.**
3. **ABDUCTION (±247 uu) IS ⛔ LARGELY UNOBSERVABLE IN THIS CAPTURE — ⛔ "not answered", ⛔ NOT "answered no".** From **01:16** the camera is locked to the hero **atop the tower pointing down**, so the tower BASE — ⛔ exactly where a pass-by grab happens — is off-frame or extremely foreshortened for ⛔ essentially the whole 64.5 s the ladder is live. ⇒ ⛔ **an abduction could have occurred repeatedly and left ⛔ no pixel.**

⚖️⭐ **STANDING RULE — `VIS-§4`:** when a footage report names a path as **not exercised**, the manager boards it as an ⛔ **OWED row with a named instrument**, ⛔ never as a closed one, and ⛔ never folded into a neighbouring row that *was* observed. **The checkable tell: an owed row states ⛔ WHAT WOULD ANSWER IT.** ⚖️ *Absence of evidence gets a task; it does ⛔ not get a tick.*

### VIS-§4a. ⛔⛔ **AMENDED 2026-09-02 BY `TASK-799` — ⛔ THIS SECTION'S ORIGINAL INFERENCE IS ⛔ FALSE. ⭐ COMMAND STATE IS ⛔ NOT OBSERVABLE FROM BANNERS.**

> ### ⚖️⭐⭐ **THE ERROR WAS ⛔ MINE AND THE ANALYST'S BOTH, AND IT IS RECORDED AS AN ERROR RATHER THAN QUIETLY DELETED.**

**⛔ WHAT THIS SECTION USED TO SAY (retained struck-through so a reader who remembers it finds its refutation instead of re-deriving it):**

> ~~**The first post-hero ascent (01:20.5) ⛔ PRECEDED any HOLD zone on the deck** — the first `HOLD set: 6 unit(s)` banner is at **01:32.7**, **12 s later**. ⇒ ⛔ **that ascent was ⛔ NOT a commanded move.**~~

⛔⛔ **REFUTED AT SOURCE, POSITIVELY** (`TASK-799`; every cite re-verified by the manager on disk before this was written):

| # | fact | source, verified |
|---|---|---|
| 1 | `ASummonedUnit::BeginPlay` calls `TryAutoEnrollInFollowGroup()` on **every** spawn path | `SummonedUnit.cpp:1311` |
| 2 | that enrolment is **UNCONDITIONAL** for every follow-eligible Blue unit — *"every follow-eligible Blue unit spawns FOLLOWING, on EVERY spawn path"* | the default-stance law, `SummonedUnit.cpp:1320-1329` |
| 3 | `EnrollInDefaultFollowGroup` emits ⛔ **ZERO** `BroadcastCommandPrompt` | `SiegePlayerController.cpp:3552`; the **eight** banner sites are `:2919 · :2924 · :3084 · :3104 · :3170 · :3358 · :3505 · :3817`, ⛔ **none inside it** (`:3505` is the explicit C-press confirm, and it sits **above** `:3552`) |

⇒ ⭐⭐ **THE 01:32.7 `HOLD set` BANNER IS THE FIRST ⛔ EXPLICIT ORDER OF THE MATCH. IT IS ⛔ NOT THE FIRST ORDER.** The units carried a standing **FOLLOW** order from the moment they spawned, and **that order never prints.** ⇒ ⛔ **"no HOLD banner until 01:32.7" proves ⛔ NOTHING about whether the move was commanded.**

### ⚖️⭐ **THE STANDING RULE THIS BUYS — ⛔ GENERAL, ⛔ not specific to ladders. Cite it from anywhere.**

> ⛔⛔ **ABSENCE OF A PRINTED SIGNAL IS ⛔ NOT ABSENCE OF THE STATE.** ⛔ **No footage row, no diagnosis and no board item may infer a pawn's command state — or ⛔ any state — from the ⛔ absence of a banner, a log line or a HUD element.** ⭐ **The default is silent by construction, and a silent default is ⛔ invisible to every instrument that reads output.** ⇒ **the state must be read from ⛔ SOURCE or from an instrument that positively prints ⛔ BOTH branches.**

⚖️ **This is a reading failure of the ⛔ SAME CLASS as `VIS-§1`** — there, a conclusion was nearly drawn from what a frame appeared to show without checking what the source actually specified; here, a conclusion **was** drawn from what a log did not show. ⭐ **Both are cured by the same discipline: ⛔ verify at source before concluding, and treat grep-absence/print-absence as ⛔ evidence of nothing.**

### ⭐ **THE THREE READINGS — ⛔ THERE WERE NEVER TWO. The third is the ⛔ benign, ORDERED one, and it was ⛔ missing from the list.**

- **(i) benign escort-follow** — the same red-hooded Archer stands at the hero's right shoulder at 01:11.0 **and** 01:11.5, i.e. it was following him and went up after him.
- **(ii) incidental grab** — it was near the ladder for other reasons and the **contact radius took it**.
- ⭐⭐ **(iii) THE NAV-LINK PATH — ⛔ NEW, and fully ORDERED.** Hero on the deck ⇒ the follow **station** is on the deck (`SummonedUnit.cpp:2056` — the anchor is the **live hero**, re-resolved every call) ⇒ ⛔ **the deck IS navmesh** (`TOWER-§8.3`: `LadderTop` **must** sit on generated deck navmesh) ⇒ the path routes through the **ladder nav link** into `AClimbableTower::HandleLadderLinkReached` (`ClimbableTower.cpp:444-551`, ruled **permanent** by `CONTACT-§4.2` `K-E`). ⇒ **a genuinely commanded ascent that invokes the contact trigger ⛔ not at all.**

### ⚖️⭐⭐ **THE VERDICT, RECORDED**

> **ORDER-BLINDNESS IS ⛔ TRUE OF THE ⛔ PREDICATE AND ⛔ FALSE OF ⛔ THAT EVENT.**

1. **The predicate requires ⛔ no order.** Six terms — proximity, intent (speed floor + 60° cone), dwell, identity, team, occupancy — and ⛔ **not one reads a command, a goal, a path or a destination.** *(HIGH.)*
2. ✅ **A ⛔ STATIONARY UNIT CANNOT BE TAKEN — the categorically worse case is ⛔ REFUTED.** The predicate requires `vel.Size2D() >= MinContactSpeedUU`; a still pawn yields speed `0` and a zero heading ⇒ `dot = 0 < 0.5`, on every sample, forever. **A unit ordered to hold at a ladder foot is safe.** *(HIGH.)* ⚠️ Residual, named ⛔ not asserted: `MinContactSpeedUU = 1.f` is a **very** weak floor (0.33 % of a 300 uu/s walk); what actually protects a jostling crowd is **T3's 0.35 s of CONTINUOUS in-cone motion**, ⛔ not the speed floor. *(MEDIUM — jostle velocities ⛔ unmeasured.)*
3. ⇒ ⛔⛔ **`VID-004` @ 01:20.5 is ⛔ NOT EVIDENCE OF THE ABDUCTION DEFECT — under ⛔ EITHER reading.** Both paths were armed, both fit every frame, and ⭐ **they share a cause: the follow order walked the archer into the cone.** ⇒ even a contact-path admission there was **escort-caused, ⛔ not incidental**, so the dichotomy the section originally posed was a ⛔ **false one for this event**.
4. **The abduction defect remains ⛔ REAL and remains ⛔ UNOBSERVED.** It is now merged into `CONTACT-§14`; `VID-004` could ⛔ not have seen it.

### ⭐ **THE DISCRIMINATOR — ⛔ FREE, ⛔ ZERO CODE, ALREADY IN THE SHIPPED LOG**

The two paths log **asymmetrically** — accidental, and decisive:

| path | on successful start | cite |
|---|---|---|
| **CONTACT** | `"… CONTACT climb started for '%s' (ascending)."` | `ClimbableTower.cpp:884-886` |
| **LINK** | ⛔ **nothing** — only a refusal or a declined `BeginLadderClimb` | `ClimbableTower.cpp:444-551` |
| **both** | `"climb ended for '%s' (reached top: …)"` | `ClimbableTower.cpp:570-572` |

⇒ ⭐ **at Verbose `LogGitClaudeUnrealTest`: a `climb ended` with ⛔ NO preceding `CONTACT climb started` is the ⛔ ORDERED LINK path.** **One PIE row, and it settles this section outright with ⛔ no diff** — boarded in `TASK-802`.

### VIS-§5. ⭐⭐ **KE2 — THE DECK-CLIP VERDICT IS BETTER THAN EXPECTED, ⛔ AND THE AXES MAY ⛔ NOT MATCH. ⛔ THIS IS THE ONE ROW WHERE I DISAGREE WITH THE ANALYST — AT SOURCE.**

✅ **FIRST, THE GOOD NEWS, UNQUALIFIED: ⛔ NO FRAME SHOWS ⛔ ANY BODY-CLIP.** Not the parapet, not the stone, not the deck. Occlusion reads **CORRECT** at 01:16.5 (torso in front of the slab's face, legs below on the rungs) and at 01:17.0. **The measured anomaly is ⛔ LATERAL, ⛔ not width:** the hero's silhouette is **NARROWER than the ladder's clear opening** yet sits **~20 % of that width LEFT of the centre-line**, so his left shoulder tracks over the recess beside the left rail.

⛔⛔ **AND HERE IS THE SOURCE CHECK THE ANALYST COULD ⛔ NOT PERFORM, WHICH IS WHY HE CORRECTLY HEDGED:**
- **The pending translation is `δ = (−10, 0, 0)` — ⛔ X-ONLY** (`CONTACT-§7a` `K-1` = option `A`; `build_watchtower.py:36-66`). **X is TOWARD/AWAY FROM the tower body** — it is the **standoff** axis.
- **The ladder's clear opening is along ⛔ Y:** stiles at `STILE_CY = 76.0`, `STILE_HY = 10.0` ⇒ **inner faces at `|y| = 66`, clear width `132` uu** (`:214-215`; manifest confirms `132`).
- ⇒ ⭐⭐ **AN OFFSET MEASURED *ACROSS THE LADDER'S CLEAR OPENING* IS A ⛔ Y OFFSET, AND A PURE X TRANSLATION MOVES IT BY ⛔ EXACTLY ZERO.** ⇒ ⛔ **the report's read — *"the pending 10 uu translation targets the right axis"* — is ⛔ NOT ESTABLISHED AT SOURCE.** ⚖️ *He judged the axis from a screen direction; the screen direction at the west face is ⛔ not the axis the fix moves. ⛔ Neither of us can settle it from the capture — so it is ⛔ measured, ⛔ not assumed.*
- ⚠️ **AND THE ARITHMETIC IS ⛔ WORTH STATING BECAUSE IT IS ⛔ NOT REASSURING, ⛔ but it is ⛔ NOT a finding:** `20 % × 132 = 26.4 uu` of Y offset, against a hero capsule `r = 42` (`GitClaudeUnrealTestCharacter.cpp:18 InitCapsuleSize(42,96)`, TASK-775-measured) in a `±66` half-opening ⇒ a centred hero has **`66 − 42` = 24 uu** of side margin, so **`26.4 > 24` ⇒ a ~2.4 uu capsule/stile overlap.** ⭐ **That is CONSISTENT with both observations at once — the shoulder over the recess, ⛔ AND no visible clip (2.4 uu is sub-pixel at that range, and ⛔ the ladder has ZERO collision so nothing pushes back).** ⛔⛔ **BUT IT IS BUILT ON AN EYEBALLED "~20 %" AND IS THEREFORE A ⛔ HYPOTHESIS, ⛔ NOT A MEASUREMENT — `SC-§20`, and `CONTACT-§13`'s own closing clause: ⛔ a manager reconstruction is a PREDICTION and ⛔ never the acceptance.**
- ⇒ ⛔ **`TASK-800` measures the pawn's tower-local `Y` during a live climb. ⛔ Until it reports, ⛔ no task may claim the offset is fixed, and ⛔ no task may claim `10 uu` was aimed at it.**

**⚠️ THE TWO CAVEATS THE ANALYST DECLARED, KEPT VERBATIM IN FORCE — ⛔ they are why this is not a clean bill of health:** **(a)** sampling was **0.4–0.5 s** with ⛔ **no `--run` burst**, so a ⛔ single-frame interpenetration could hide between samples (`FR-§3`: `--run` is the flicker instrument); **(b)** it is **ONE ascent, by ONE pawn class, at ONE approach angle** — the six unit ascents happen while the camera looks down at the deck, so ⛔ their ladder-top handovers are largely off-frame.

### VIS-§6. ✅ **KE1 — JONATHAN'S ANIMATION WAIVER IS ⛔ HOLDING UP ON PIXELS, AND THAT IS RECORDED AS A RESULT**

The climb ships unanimated on his `K-3` waiver. **Confirmed present, and it does ⛔ NOT look worse than described.** At 01:14.5 the pose reads **standing on a rung** — feet flat and in contact, body plumb, hands at the rails' depth — ⛔ **not T-posed, ⛔ not floating, ⛔ not a pass-through.** Ascent is smooth and continuous across 01:14.5 → 01:16.5 → 01:17.0. ⇒ ⭐ **cosmetically "unanimated", ⛔ NOT "broken". ⛔ No change to the deferral is indicated, and ⛔ no task is raised.** ⚖️ *A waiver that survives contact with the pixels is worth recording as loudly as one that fails.*

### VIS-§7. ⚠️ **V6 — A LOW-CONFIDENCE FOOTAGE FINDING GETS A ⛔ VERIFY TASK, ⛔ NEVER A FIX TASK**

The analyst flagged the foliage shards **low-to-medium confidence** himself, and named the reason: **h264 compression can exaggerate flat-shaded polygon edges** (`FR-§5`). ⚠️ **And his own frame contains the counter-evidence: a correctly-lit autumn tree sits in the SAME frame as the black shard silhouettes (`f00074_50s`) ⇒ ⛔ it is NOT a global lighting state.**
⚖️ ⭐ **STANDING RULE:** a footage finding the analyst grades below **high** confidence is boarded as an **in-editor VERIFICATION** whose deliverable is a ⛔ **verdict**, ⛔ not a change. **A fix task is raised ⛔ only if the verification confirms it.** ⚖️ *Authoring art against a compression artefact is how a clean asset acquires a defect.*

### VIS-§8. **V1 — THE ⛔ LEAST "MINOR" THING IN THE CAPTURE, AND IT IS A ⛔ MEASUREMENT, ⛔ NOT A FIX**

Verbatim, from a 3× crop (⛔ the analyst refused to assert this text from a sheet tile — `FR-§3`): **`Video memory has been exhausted (0.922 MB over budget). Expect extremely poor performance.`** ⭐ **`0.922 MB` over is a ⛔ HAIR over budget — that is the signature of a ⛔ POOL-SIZE SETTING, ⛔ not an asset blowout**, and prior VRAM findings are already on record in this project. ⛔ **No asset may be shrunk, no texture downsized, and no `.ini` edited until a baseline is measured.**

---

