Verdict: PASS
# QA Report — TASK-1349 [AGENT-CONTROL-CENSUS-GATE]

**Subject:** `qa/TASK-1348-verify.md` — the instrument, **not** the answer.
**Counts: 0 BLOCKER · 4 WARN · 3 NIT.**
**Central ruling: every claim-making probe carries a control that genuinely had the power to fail.** 8 of 8 probes that assert something carry a **discriminating** control; P8b asserts nothing and is labelled `unobs`; P10 is a sweep and owes none. **No probe measured its own instrument.**

⇒ **`TASK-1350` MAY RUN.** The census is trustworthy enough to board implementation rows on, subject to the four WARN carry-forwards in §6, which are scoping constraints on what the numbers *entitle* a row to say — not doubts about the numbers.

---

## §0 — WHAT I EXECUTED vs WHAT IS `ACCEPTED-AS-DECLARED` (`SC-§71b`)

I hold **no `Bash`**, no shell, no compiler, no PIE, no git. **I also did not use the `unreal_inspector` at all** — this row's `names:` says `⛔ NO EXECUTION OF ANY KIND` and does not name the inspector as a read source, so I declined it deliberately rather than stretching a read-only grant into a fence I was not given. That is a real limit on this verdict's provenance and it is stated, not buried: **I did not independently reproduce a single runtime number.**

| # | claim in the census | my instrument | status |
|---|---|---|---|
| 1 | `TraceCursorToGround` — one definition, exactly three callers (M2) | source read, by quoted text | ✅ **RE-DERIVED** — definition `bool ASiegePlayerController::TraceCursorToGround(FHitResult& OutHit) const`, body `return GetHitResultUnderCursor(ECC_Visibility, /*bTraceComplex=*/ false, OutHit) && OutHit.bBlockingHit;`; callers in `UpdatePlacementGhost`, the spell-reticle updater, the group-pick updater. **Three. Exactly.** |
| 2 | `DeprojectMousePositionToWorld` — one site, hero targeting, not routed through M2 (M3) | source read | ✅ **RE-DERIVED** — single site, `if (!IsValid(TargetingHero) \|\| !DeprojectMousePositionToWorld(RayOrigin, RayDirection))` |
| 3 | P5's refusal line, quoted verbatim | source read | ✅ **RE-DERIVED** — the format string byte-matches the census's quote, including `(W1-PREP additions 3, TASK-261; …)` which the census elided with `…` |
| 4 | "mode survived the refusal, exactly as the code says it must" | source read | ✅ **RE-DERIVED** — the refusal arm ends `RefuseCardPlay(...); break;` then `return;` — **no `ExitPlacementMode()` on that path** |
| 5 | P5's control (same press, no placement mode) can only be silent | source read | ✅ **RE-DERIVED** — `if (WasInputKeyJustPressed(EKeys::LeftMouseButton)) { TryConfirmPlacement(); }` sits **inside** the placement-mode branch ⇒ with no mode active the refusal is **unreachable**. The control's silence is *code-guaranteed*, not merely observed |
| 6 | Finding 1 — the wheel is a raw poll, so `binding_found:false` is expected | source read | ✅ **RE-DERIVED** — `WasInputKeyJustPressed(EKeys::MouseScrollUp/Down)` under the comment *"the wheel is globally unbound and stays INERT outside its three named consumers"* |
| 7 | Finding 3 — `AClimbableTower` is why the first P8 number was frozen | source read | ✅ **RE-DERIVED** — `if (!bPendingCardCanScaleFootprint) { return; }` is the **first statement** of `ApplyPlacementFootprintWheel`, and the resolve comment reads *"whose CDO answers `ABuilding::CanScaleFootprint()` — false on `AClimbableTower`"* |
| 8 | P8 (scale moved) and P9 (ghost hidden at origin) are consistent | source read | ✅ **RE-DERIVED — and this one could have failed.** `SetActorScale3D(...)` runs under a bare `if (GhostActor)`, **unconditionally every frame**, while `SetActorHiddenInGame(!bGroundHit)` and `SetActorLocation(PlacementLocation)` are gated on the hit. ⇒ a scale that moves while the ghost stays hidden at `(0,0,0)` is **exactly** what this code produces when the trace never hits. Not a contradiction |
| 9 | P9's observable is a live per-frame readout of the trace | source read | ✅ **RE-DERIVED** — ghost is spawned `SetActorHiddenInGame(true); // shown on the first ground hit` and re-hidden every frame from `!bGroundHit`. `bHidden:true` + `RelativeLocation (0,0,0)` is the **code signature of `TraceCursorToGround` returning false**, not a stale initial value |
| 10 | M5 — `SetIgnoreLookInput` twice, both on the `IA_UICursor` hold | source read | ✅ **RE-DERIVED** — `SetIgnoreLookInput(true)` under `bUICursorHeld = true`, `SetIgnoreLookInput(false)` inside `ClearUICursorHold()`. **Two sites, both the hold.** P4's control is *code-predicted*, which is the strongest kind |
| 11 | M1 — 13 raw `WasInputKeyJustPressed(EKeys::` sites | source read | ✅ **RE-DERIVED** — RMB/`Escape` ×4 (group-pick · targeting · war-map · placement), LMB ×3, `MouseScroll*` ×4 in the two wheel helpers. Count corroborated |
| 12 | Both evidence PNGs exist at the quoted paths | session-start `git status` + direct image read | ✅ **RE-DERIVED** — both appear as untracked files, and I **opened both** (see §5) |
| 13 | a3 frame: `Gold: 38`, `60 FPS / 16.7 ms`, **no ghost rendered** | image read | ✅ **RE-DERIVED — the description is accurate, cell for cell** |
| 14 | a2 frame: placement mode up for `WatchTower`, ghost nonetheless invisible | image read | ✅ **RE-DERIVED, and it is a SECOND independent corroboration of P9** (§5) |
| 15 | **All PIE runtime values** — rotations, scales, `GhostActor` ids, `bHidden`, tool reply texts | none | ⚠️ **ACCEPTED-AS-DECLARED** |
| 16 | The `SC-§118` editor census (PID 27484, log ages, empty command line) | none | ⚠️ **ACCEPTED-AS-DECLARED** — internally coherent and the substitute is named, not hidden |
| 17 | `L_Arena.umap` sha256 and the five `.sav` hash/mtime pairs | none | ⚠️ **ACCEPTED-AS-DECLARED** — within its read-only Python grant, so the claim is at least *capability-consistent* (`SC-§71b`) |

---

## §1 — CHECK ONE: THE PER-PROBE CONTROL TABLE (the row's reason for existing)

Measured against **`VER-§8` cl. 1's own firing pair** — `Tab` (`binding_found:false`) moved focus **not at all** while `Down` (`binding_found:true`) **did** — i.e. *one input that should move the observable and one that should not, same posture, back to back.*

| probe | control present | control **discriminated** | the pair, and why it could have failed |
|---|---|---|---|
| **P1** `IA_Look` | **Y** | **Y** | treatment `yaw 180 → 222.5` vs control (zero vector, same call) `180 → 180`. A latched injector, or a cached/drifting transform read, would have moved the control. It did not |
| **P2** `IA_MouseLook` | **Y** | **Y** | `222.5 → 265` vs zero vector `222.5 → 222.5`. **Reported separately from P1 as the spec demanded** — different asset, and a reader *would* have assumed they behave alike |
| **P3** `simulate_right_stick` | **Y** | **Y — but via the *instrument* control, not the stated one** | ⭐ **The subtlest cell in the table, and the census got it right.** Its stated control (neutral stick) returned *the same null as the treatment* ⇒ **that pair alone discriminates nothing.** The census saw this and supplied the real discriminator: *"the same `control_rotation` was moved 3× in this same posture by P1/P2 ⇒ the null is a real negative, not a dead observable."* That is `VER-§8` cl. 1's second-run shape (*"re-took the control STRONGER"*) and it is sufficient. **Credit, not a finding** |
| **P4** look inside cursor modes | **Y** | **Y — a FIRING NEGATIVE, the strongest control in the report** | in placement `180 → 222.5`; under the `IA_UICursor` hold, the **identical** call `222.5 → 222.5`. The two postures **read differently**, so the probe measured the game. And the suppression is **code-predicted** (§0 row 10) — the control was not merely observed to differ, it differed *where the source says it must* |
| **P5** LMB in placement | **Y** | **Y** | refusal line emitted + `GhostActor` survives, vs the same press with no mode: **no refusal line at all**, `GhostActor` `None → None`. §0 row 5 shows the silence is code-guaranteed. **This is the cleanest pair in the report** |
| **P6** RMB / `Escape` | **Y** | **Y** | `GhostActor StaticMeshActor_34 → None` vs `F9` (unbound) in the same posture: **mode STAYED**. An unbound key that ended the mode would have exposed the tool as firing something else |
| **P7** `IA_CancelPlace` | **Y** | **Y** | same observable, same `F9` control taken **immediately before, same posture** |
| **P8a** the wheel | **Y — two of them** | **Y** | (a) `F9` unbound: `1.200 → 1.200` **unchanged**; (b) ⭐ **a *signed* control** — `MouseScrollDown` moved it the **other way** (`1.200 → 1.100`). (b) is the best control in the document: it rules out *"any input nudges it"*, which a null control cannot |
| **P8b** `simulate_button_press` | **N** | **n/a** | ⚠️ **NOT A BLOCKER, and the distinction is the whole point:** the spec's BLOCKER fires on *a probe with no control, or a control that returned the same thing* — i.e. on a probe that **makes a claim** it cannot support. P8b **makes no claim.** It is labelled `UNOBS`, its surface is explicitly called *"inferred, not measured"*, and it is listed under `## Not examined`. An unprobed item owes an honest label, not a control. It got one. **See §6 WARN-B for what `TASK-1350` must do with it** |
| **P9** the cursor | **Y** | **Y** | `set_player_transform` **provably fired** (reply echoed `"applied_rotation"`) at pitches `0° / −45° / −80°` — **a call that demonstrably worked moved the trace not at all.** This is the correct control for a negative: it proves the *lane* was live while the *observable* stayed dead. Plus two independent pixel frames (§5) |
| **P10** the sweep | **n/a** | **n/a** | A census of surface names is not an experiment. The report writes `—` in the control cell rather than inventing one. **Correct** |

**⇒ CHECK ONE VERDICT: PASS. Zero uncontrolled probes. Zero BLOCKERs under spec (1).**

---

## §2 — CHECK TWO: THE P5 INVERSION, RULED EXPLICITLY

**Ruled CORRECT.** The census scored the refusal **`PASS — the button ARRIVES; the AIM POINT is what fails`**. It did **not** score the refusal as a `fail`, which is the defect this check exists to catch.

And the **converse escalation did not trigger, which I checked rather than assumed**: no unit was placed. `GhostActor` remained `StaticMeshActor_34` after the press, and §0 rows 3–5 show the refusal arm is reached only when the placement validity gate rejects the point — with `bGroundHit` false at its head. **Ceiling 1 stands unrefuted.** Had a unit been placed, the cursor *would* have been over the viewport and the whole wave's design premise would need re-opening; it was not, and the report does not hint otherwise.

⭐ The inversion is also **independently corroborated by law**: `VER-§8` cl. 7 already records this exact refusal string and this exact mechanism (*"`simulate_key_press "LeftMouseButton"` DOES reach the poll … the placement then fails one layer deeper"*). P5 reproduces a measured ceiling rather than discovering one — and the census did **not** re-trace the named dead ends around it (`ui_perform` was inherited by reference, `VER-§5` cl. 5). **Budget spent correctly.**

---

## §3 — CHECK THREE: THE P9 ENUMERATION, RULED EXPLICITLY

**Ruled SUFFICIENT — with one named gap (WARN-B).** Seven attempts, each with what it returned, and the count stated. This is emphatically **not** a bare *"no route exists"*: attempt 1 (`set_player_transform` across three pitches) is called out by the census itself as **the load-bearing one**, because it is what rules out the cheap alternative explanation *"the aim is merely pointing at sky"*. That is the difference between a negative result and a shrug.

**The other direction — checked, and clean.** No ungranted name was reached for. No shell, `subprocess` or `ctypes` was routed through the read-only inspector; item (6) declines it by name and records itself as **the ninth consecutive refusal**. Per my dispatch: *do not treat that as a gap in diligence* — I do not, and I record that **I make it the tenth.** I held a read-only inspector this entire review and did not route anything through it (§0).

**Completeness for the row that must design around it:** the enumeration covers the camera lane, the game's own cursor mode, the button lane, the widget lane and the shell lane. It does **not** cover the property-write lane — see WARN-B, which is a constraint on `TASK-1350`'s mechanism ranking, not a defect in the measurement taken.

---

## §4 — CHECKS FOUR & FIVE: SCOPE, AND THE TABLE

**CHECK FOUR — SCOPE: CLEAN.** No code, no asset, no compile, no git, no `CONVENTIONS.md`, no other row's line, no tool grant sought (*"No ungranted tool was reached for, and no new grant is sought (ceiling 4)"*). All three mechanism guesses sit under `## Hypotheses (not verdicts)`, labelled. **The (A)/(B) ruling is not re-opened, not nudged, and not commented on.** The closest the report comes to a remedy is *"the blocker is the AIM POINT, not the button"* — that is a **conclusion from a measurement**, not a prescribed mechanism, and it is precisely what the row was asked to establish. The `DeckComponent.Hand` discovery is instrumentation for future probes, not gameplay design. **No `SC-§100` finding.**

**CHECK FIVE — TABLE COMPLETE AND HONEST: PASS.** Eleven rows for ten probes (P8 correctly split a/b). Every `observed` cell carries a quoted number, a verbatim tool/log string, or an evidence path. **No `works` / `looks right` / `as expected` anywhere.** The sole `unobs` (P8b) is also listed under `## Not examined`, first. Attempts 3 of 3, and the superseded attempt's evidence was **kept** rather than quietly dropped (`VER-§1` cl. 6). The `SC-§118` census is declared **PARTIAL by cl. 9** with the substitute performed in full and named — the honest shape.

---

## §5 — THE PIXEL CHECK I RAN MYSELF (and it could have sunk P9)

I opened both evidence frames. P9's entire negative rests on `bHidden:true`, so a frame showing a visible ghost would have falsified it.

- **`…-a3-t00m06s-wall-ghost-wheel-resize.png`** — `Gold: 38` top-left, `60 FPS / 16.7 ms` top-right, the cloaked knight on grass before the lit castle arch with both wall sconces burning. **No placement ghost anywhere in frame.** The census's one-line description is accurate cell for cell.
- **`…-a2-t00m20s-placement-ghost-up.png`** — `Gold: 64`, `31 FPS / 32.5 ms`, hero beside the castle wall, red-and-gold banner, autumn trees. Taken at an instant the report says `GhostActor` was **non-None for `WatchTower`** — and **there is still no ghost rendered.** ⭐ **This is a second, independent corroboration of P9**: different session, different camera, different card, different gold, same result — placement mode provably up, ghost provably invisible. It does not contradict P9; it strengthens it.

Both frames also show a small HUD feedback string at bottom-centre, consistent with `RefuseCardPlay` reaching the player-facing layer.

---

## §6 — FINDINGS

### BLOCKER — none (0)

### WARN (4)

- **[WARN-A]** `qa/TASK-1348-verify.md`, P3's last cell — **the cell reads `FAIL` while line 1 reads `VERIFIED`.** Under `VER-§1` cl. 5 *any* `fail` mechanically derives `VERIFY-FAILED`, so the report's own table and its own headline disagree, and a grep-based or mechanical consumer resolves that the wrong way. Worse, **the same epistemic object is scored two different ways in one table**: P3's controlled negative is `FAIL`, while P9's controlled negative — the larger of the two — is `NEGATIVE, FULLY CONTROLLED`. — **Not a blocker**: the departure is disclosed in a blockquote immediately under the H1 (the `VER-§1` cl. 7 shape, §7), and no measurement, control or number is affected. — **Suggested fix: not an edit to the shipped report.** That file is committed evidence and I may not touch it; re-scoring a cell after the fact would also destroy the disclosure that makes it honest. The fix belongs to the `MEASURED` mint: whoever mints it should rule **what a census row's last cell may contain**, so a controlled negative is scored consistently. Absent that rule, this exact inconsistency recurs on the next census.
- **[WARN-B]** P9's enumeration, and `TASK-1350`'s `(C2)` mechanism ranking — **the property-write lane is UNENUMERATED, not refuted.** The seven attempts cover camera, cursor-mode, button, widget and shell lanes. They do **not** cover `set_actor_property` / `pie_scene_edit` — which `VER-§8` cl. 7 itself names as the granted substitute lane for exactly this kind of problem. — **Why this is load-bearing:** `(C2)` instructs the row to *"take the HIGHEST fidelity mechanism the census supports"*, and mechanism **(i)** (*move the REAL viewport cursor so `GetMousePosition` is UNTOUCHED — purest (A), zero divergence*) is the top of that ranking. Ranking down to **(ii)** — which carries a **declared residual divergence** (the drawn cursor and the aim point can disagree) — on the strength of P9 would be **ranking down on an incomplete negative.** — **Suggested fix:** `TASK-1350` treats mechanism (i) as **untested**, not closed, and either boards one bounded probe of the property-write lane before choosing, or states on the row's face that (ii) was taken **without** (i) having been measured.
- **[WARN-C]** P6, and the attribution of the "cancel half already shipped" claim — **P6's `Escape`/`RMB` result is OVER-DETERMINED.** In placement mode those keys reach **two** code paths: the raw poll (`if (WasInputKeyJustPressed(EKeys::RightMouseButton) || WasInputKeyJustPressed(EKeys::Escape)) { ExitPlacementMode(); }`) **and** the bound `IA_CancelPlace`. P6 proves *the key ends the mode*; it cannot say *which path carried it*. — **The claim survives, via P7**: `inject_input_action IA_CancelPlace` sets the **action**, bypassing the key poll entirely, and it ended the mode. So the bound action is independently proven live. — **Suggested fix:** `TASK-1350` cites **P7**, never P6, when it closes the cancel half as already-shipped. Both paths are (A)-shaped player routes, so nothing is lost — but a row that cites P6 has cited the ambiguous one.
- **[WARN-D]** `CONVENTIONS.md` `VER-§8` cl. 7 — **the law itself enshrines the slot-index assumption that Finding 2 refutes.** Its first bullet is the worked example *"`inject_input_action IA_Card1` returned 'placement mode entered for card *Cleric* (cost 18)'"*. Given a shuffled hand that redraws after every play, **the card name in that quote is incidental and not reproducible** — which is exactly why it failed to reproduce for the census. — **Not my edit to make**, and I have made none: `⛔ NEVER CONVENTIONS.md` is on my `names:`. Under `VER-§8` cl. 5 a clause is amended **in place, dated**. — **Suggested fix:** this is `TASK-1350` item (5) work — it is the clearest law-shaped fact this wave has produced, and it rides the wave's host.

### NIT (3)

- **[NIT-1]** `VER-§1` cl. 3 asks the last cell to be *exactly one of* `pass`/`fail`/`unobs`; most cells carry a token **plus prose** (`PASS — left/right AND up/down both drivable`). This is consistent house style across the project's verify reports and the token is never ambiguous. Noted once, not stacked — it is the surface WARN-A's fix should tidy.
- **[NIT-2]** The evidence filename `VER-TASK-1348-a2-t00m20s-placement-ghost-up.png` reads as *"the ghost was up/visible"* when the frame's actual value is the **opposite** — placement mode up, ghost **not rendered** (§5). The caption is accurate; the filename outlives the report and is what the manager sees in a directory listing.
- **[NIT-3]** Ceiling 6 over-run, **disclosed**: session A ran ≈175 s against a 90 s line. Structurally mitigated and I checked it rather than accepting the reassurance — the load-bearing observables sit at **t≈28.2 s** and **t≈54.4 s**, and a bot-kill PIE restart would have **nulled `GhostActor`**, which was non-None at both readings. ⇒ the claim *"no hero death interfered with any recorded observable"* is corroborated by the data's own shape, not merely asserted.

---

## §7 — RULING ON THE VERDICT TOKEN, AND ON `MEASURED`

**The narrowed `VERIFIED` is ACCEPTABLE AS SHIPPED. I do not ask for it to be changed.**

It is the **`VER-§1` cl. 7 shape, executed correctly and completely**: emit the **true** token, state the departure **on the report's face**, propose the amendment, and write **nothing** into the law file. All four limbs are present — the narrowing blockquote sits immediately under the H1, so it is the first thing any reader past `head -1` meets; the proposal is addressed to the manager; `CONVENTIONS.md` is untouched.

**Its reasoning on the two alternatives is correct, and I checked both rather than taking them:**
- **`VERIFY-FAILED` would be actively wrong.** It blocks a commit and bounces the row to a gameplay-programmer — **there is no code in this row and no defect to fix.** That is a false fail, and this project's own law names the false fail as *the expensive error*: it burns one of the three loops, delays a lane that was ready, and teaches the next author that a measured negative is their defect.
- **`UNOBSERVABLE` would also be wrong.** Ten of ten probes produced runtime signal. `VER-§5` cl. 4 is explicit that a partial row is not `UNOBSERVABLE`, and this row is not even partial.

**The one thing the token genuinely risks** is the `head -1` reader who sees `Verdict: VERIFIED` on a row inside a wave headlined *"all the same controls as an actual player"* and concludes the answer is **yes**. It is **no** (P9). What defuses that is that the row's **own acceptance lines** are about the census executing under control — and against those, every line passed. The report says so in its second sentence. ⇒ the token is true **to the row**, and the narrowing prevents it being read as true **to the wave**.

**`MEASURED` — ENDORSED as a proposal, and I do NOT mint it** (`SC-§82`: the agent flags, the manager rules; `SC-§101`: a prescribed remedy is a claim). It is the right token for a row that returns numbers rather than a pass/fail on a feature, that must never block and never bounce. **The census was also right to leave the minting alone** — this is the same shape as `VER-§1` cl. 7's own origin, where a verifier refused to resolve a conflict in a file that was not its own and proposed the clause instead. That refusal is why the clause exists.

**One design constraint for whoever mints it,** offered as a constraint and not a draft: `MEASURED` needs **two** things, or WARN-A recurs on the next census. (1) A fourth branch in `VER-§1` cl. 5's derivation. (2) **A rule for what a census row's last cell may contain** — because the real defect in this report is not the headline token, it is that a *controlled negative* had no correct cell token available, so P3 borrowed `FAIL` and P9 invented prose. Mint the token without the cell rule and the next census will disagree with itself in the same place.

---

## §8 — THE THREE FINDINGS `TASK-1350` MUST CARRY (verified, and made unmissable)

### ⭐ 1. `"binding_found": false` IS NOT EVIDENCE A KEY FAILED TO REACH THE GAME. — **CONFIRMED, AND RE-DERIVED AT SOURCE.**

All four wheel presses reported `false` **and the wheel still fired** (`1.000 → 1.100 → 1.200`, and back down to `1.100` on the reverse notch). I re-derived the mechanism in `SiegePlayerController.cpp`: the wheel is read by `WasInputKeyJustPressed(EKeys::MouseScrollUp/Down)` under the comment *"⛔ NO new InputAction — the wheel is globally unbound and stays INERT outside its three named consumers."* ⇒ **`binding_found:false` is the correct and expected Enhanced-Input answer for a deliberately unbound key that a raw poll reads anyway.**

🚨 **Any future probe that treats that field as a verdict yields a FALSE NEGATIVE.** It would report *"the wheel is unreachable"* about a wheel that demonstrably works — and `TASK-1350` would then board a `(C3)` code row to fix nothing. **This is the most reusable item in the report.**

⭐ **The precise scoping, which matters because the field is not worthless:** `VER-§8` cl. 1's own canonical firing pair *uses* `binding_found` (`Tab` false vs `Down` true). Finding 1 does **not** overturn that pair — there the **observable** was focus and `binding_found` merely corroborated. The rule that survives both is: **`binding_found` may corroborate an observable; it may never *be* the observable.** ⇒ **the census did not commit its own warned-against error at P3** — P3's negative rests on the rotation not moving, with P1/P2 proving the rotation was movable in that posture. `binding_found` is quoted there as supporting text only. Credit.

### 2. THE HAND IS SHUFFLED — A SLOT INDEX DOES NOT IDENTIFY A CARD. — **CONFIRMED.**

Slot 1 held `BrightSun`, then `Footman`, then `WatchTower` across three sessions. The boarded claim *"`IA_Card1` → placement mode for 'Cleric'"* **did not reproduce**, and now it is clear it never could have reliably. The deterministic fix the census found: read `DeckComponent.Hand` via `get_actor_property_in_pie` — it returned `("Footman","Footman","WatchTower","Footman","BrightSun","Archer")`.

⇒ **Any row specced against a slot index is specced against noise.** A row needing a *specific* card reads the hand first and picks the slot that currently holds it. **And see WARN-D: this assumption is currently written into `VER-§8` cl. 7 itself**, which is where it will bite next if it is not amended.

### ⭐ 3. IT DISCARDED ITS OWN FIRST P8 NUMBER AS UNCONTROLLED AND DISCLOSED IT. — **CONFIRMED AT SOURCE. CREDITED, NOT MARKED DOWN.**

It had measured the wheel against `WatchTower` and seen scale frozen at `1.000000`. I re-derived why that number was junk: `ApplyPlacementFootprintWheel` opens with `if (!bPendingCardCanScaleFootprint) { return; }`, and the flag is resolved from the card class's CDO where *"`ABuilding::CanScaleFootprint()` — false on `AClimbableTower`"*. The WatchTower **is** `AClimbableTower` — **the one building that overrides it to `false`** — so the poll is never reached. Re-run against `Wall`, the wheel fired.

🚨 **Had it published that first number**, the census would have asserted *"the wheel does not fire"* about a wheel that works, and `TASK-1350` would have boarded a `(C3)` row to repair a shipped feature — **the `TASK-1286` shape exactly, one stage earlier, which is the precise failure this gate exists to catch.** It caught itself, named the mechanism, kept the superseded evidence per `VER-§1` cl. 6, and wrote the lesson down. **That self-catch is the single strongest signal in this report that the instrument is trustworthy**, and it is why WARN-A does not become a blocker: an author who discards their own convenient number is not an author whose disclosures need policing.

---

## §9 — ALSO RULED (P8b, P10, and the budget)

- **P8b is `UNOBS`, named and not dressed up — CONFIRMED HONEST.** It says *"Not separately invoked"*, calls its surface *"inferred, not measured"*, and appears first under `## Not examined`. **⇒ `TASK-1350` MUST TREAT `simulate_button_press`'S SURFACE AS UNMEASURED.** Nothing is lost by this — ceiling 2 already puts Slate out of reach by construction, and gamepad parity is out of scope as a feature `(C4)`. The one thing the gap forbids: **no row may carry an acceptance line whose observable depends on `simulate_button_press`**, because nobody has established what it presses.
- **P10's sweep — COMPLETE AND CORRECTLY SCOPED.** It names what was not probed: hero locomotion (`IA_Move`/`Jump`/`Sprint`), the five `IA_Cmd*` stances, `IA_Rally`/`Recall`/`DiscardAll`, `IA_WarMap`/`ControlsHelp`/`AssistantConsole`, and group-pick + spell-targeting — **the other two `TraceCursorToGround` callers**, where it explicitly declines to assert the blocker it believes applies *by construction*. That restraint is correct.
- ⭐ **`DeprojectMousePositionToWorld` — FLAGGED, NOT BURIED, AND I AM RAISING IT FURTHER.** It is **a SECOND cursor read that a fix at the single choke point would NOT cover** (M3; I re-derived the single site in the hero-targeting path). It appears in the P10 table **and** under `## Not examined`, and `TASK-1350`'s `(C2)` already carries the matching instruction — *M3's lone `DeprojectMousePositionToWorld` is **EITHER** folded into the same source **OR** declared out, never silently left behind.* **That linkage is live and must not be lost in the re-spec**: a `(C2)` row that fixes `TraceCursorToGround` and says nothing about this site has left half the cursor surface unaddressed while looking complete.
- **The budget constraint, for the next verifier spec:** a single MCP round trip costs **≈30 s of PIE clock**; a batched `run_verification_sequence` costs **≈7 s**. ⇒ **batching is the only way to stay inside the 90 s ceiling**, and session A over-ran it (NIT-3). `TASK-1350` should write this into any verifier row it boards — as a *method instruction*, not merely a budget number, because the number alone does not tell an assignee what to do differently.

---

## §10 — NOTES FOR `TASK-1350` (and for the wave's commit host)

1. **Read this file first, then the verify report** — as the row already instructs. Nothing in the census is marked uncontrolled; **all ten probes are facts.** The four WARNs constrain *what those facts entitle a row to say*, not whether they are true.
2. **`(C1)` CAMERA — the census supports CLOSING IT as already-shipped.** P1 and P2 both fired with discriminating controls, and P4 proves look survives placement mode. Quote the rotation numbers on the board and tell Jonathan the answer to his camera question is **YES**. **P3's negative does not qualify this** — it says gamepad does not reach the camera *under the applied context measured*; the keyboard/mouse lane he actually plays on is proven live.
3. **`(C3)` THE WHEEL — the census supports CLOSING IT as already-shipped**, on `Wall`, with the `AClimbableTower` exclusion named as **designed behaviour, not a defect** (§8 finding 3).
4. **The cancel half — CLOSE IT, citing P7** (WARN-C), not P6.
5. **`(C2)` IS THE WHOLE REMAINING WAVE, and it is where the WARNs bite.** Honour WARN-B before ranking mechanisms, and honour `(C2)`'s own standing instruction on `DeprojectMousePositionToWorld` (§9).
6. **Law-shaped candidates for item (5):** §8 finding 1 (`binding_found` may corroborate, never be, the observable) is the strongest — it is a *general* instrument law, not a Siegebound fact. WARN-D (amending `VER-§8` cl. 7's worked example) is the most urgent, because the stale assumption sits in the law an assignee will copy from.
7. **Commit host:** this report (`qa/TASK-1349-report.md`) and `qa/TASK-1348-verify.md` are both held-for the wave's host per `TASK-1348`'s `names:`; the host derives its pathspec at its own instant (`SC-§133`), never from a filename predicted here. **The two evidence PNGs under `playtest-evidence/2026-09-20/` are untracked and belong in the same pathspec** — they are the promoted evidence for the wave's load-bearing negative, and I opened both to gate it (§5).
8. **Editor untouched.** PID 27484 was up when the census ran and I took no lifecycle action, drove no PIE, and ran nothing. I did not use the `unreal_inspector` at all (§0).
