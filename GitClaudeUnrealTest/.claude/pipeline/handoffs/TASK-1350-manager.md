# TASK-1350 — [AGENT-CONTROL-DECOMPOSE] — manager handoff

**Date:** 2026-09-20 · **Agent:** manager · **Row:** `TASK-1350`, marker `TASK-1350-AGENT-CONTROL-DECOMPOSE`
**Shape:** board writes + `CONVENTIONS.md` amendments. Zero code, zero asset, zero engine, zero git.
**Law applied:** `SC-§100` · `SC-§101` · `SC-§104` · `SC-§120` · `SC-§121` · `SC-§126` cl. 10/11 · `SC-§127` · `SC-§133` · `SC-§134` cl. 7 · `SC-§137` · `VER-§1` cl. 1/3/3a/5a/7 · `VER-§8` cl. 2/3/5/7/10 · `TL-§5e` cl. 1/7a/7b/7d.

---

## 1. The headline

🧑 **Three of his four asks were already built.** The census measured them, the gate certified the census, and the correct number of code rows for those three is **zero**.

| ask | verdict | evidence |
|---|---|---|
| **(C1)** camera up/down/left/right | ✅ **ALREADY SHIPPED — the answer is YES** | `IA_Look` yaw `180 → 222.5`, pitch `0 → 317.5`; `IA_MouseLook` yaw `222.5 → 265`. Zero-vector controls unchanged both times. Look **survives** placement (P4); suppressed only under the `IA_UICursor` hold (`SetIgnoreLookInput`: exactly two sites, both that hold) |
| **(C3)** the wheel | ✅ **ALREADY SHIPPED** | `1.000 → 1.100 → 1.200`, reverses to `1.100`. Two controls, one of them **signed**. `AClimbableTower` exclusion is designed, not broken |
| **(C2)** cancel half (RMB/`Escape`) | ✅ **ALREADY SHIPPED — cited to `P7`, never `P6`** | `inject_input_action IA_CancelPlace` ⇒ `GhostActor` `StaticMeshActor_34 → None`, `F9` control left the mode standing |
| **(C4)** gamepad | ⛔ **NO REACH — settled cheaply** | `simulate_right_stick` moved nothing; instrument control = the same rotation moved 3× by P1/P2 in the same posture |
| **(C2)** the **AIM POINT** | 🚨 **THE WHOLE REMAINING WAVE** | ghost `bHidden: true` at `(0,0,0)` across pitches 0°/−45°/−80°, with `set_player_transform` provably firing |

**The button arrives. The aim point is the gap.** P5's refusal line is the expected positive, with a control that was code-guaranteed silent.

## 2. WARN-B — how the ranking was resolved, and why

The gate found that `P9`'s seven attempts **never covered the property-write lane** (`set_actor_property` / `pie_scene_edit`) — the very lane `VER-§8` cl. 7 names as the granted substitute. ⇒ mechanism **(i)** (move the real viewport cursor; `GetMousePosition` untouched; purest (A), zero divergence) is **UNENUMERATED, not REFUTED**.

**I took the probe, not the fallback.** The trade, written on `TASK-1352` (0):

> Mechanism (ii) carries a **permanent, shipped divergence** — the drawn cursor and the aim point can disagree — paid by every player-facing trace, forever. **One bounded probe is cheaper than a permanent divergence.**

⇒ **`TASK-1352`** closes the enumeration; **`TASK-1353`** rules the mechanism on the result. I deliberately did **not** board a code row carrying two candidate mechanisms with "assignee picks" — that hands a design ruling to an assignee, which this wave's own header forbids. One extra manager row is the price of not doing that.

`TASK-1352` carries a **positive instrument control taken before the treatment**: if no property write can be shown to take at all, every null in the row is a dead instrument and the row is `unobs`, not `measured`. It also forbids guessing a property name and reporting "not found" as a finding — *a missing name is a naming failure, not a lane failure*, which is the same incomplete-negative error one level down.

## 3. `MEASURED` — minted, with the cell rule

`VER-§1` cl. 1 (token added) · cl. 3 (fourth cell token) · **NEW cl. 3a** (the cell rule) · **NEW cl. 5a** (the derivation branch).

The real defect WARN-A found was **not** the headline token: a *controlled negative* had **no correct last-cell token**, so `P3` borrowed `FAIL` and `P9` invented prose — the same epistemic object scored two ways, with `VER-§1` cl. 5 mechanically deriving `VERIFY-FAILED` from that borrowed `FAIL`.

- `fail` — **reserved** for *the feature under test did not do what the acceptance line required*. A statement about **our code**. The only token that bounces a row and burns a loop.
- `measured` — a **controlled negative**: probe fired, control discriminated, observable did not move, and *"it did not move"* is the answer the row went looking for. A statement about **the world or the rig**.
- `unobs` — the probe never fired or the observable was never read. **A negative without a discriminating control is `unobs`, never `measured`** — `measured` is *earned by the control*.
- Derivation, first match wins: any `fail` ⇒ `VERIFY-FAILED` · no `fail` + ≥1 `pass` ⇒ `VERIFIED` · no `fail`/`pass` + ≥1 `measured` ⇒ **`MEASURED`** · none ⇒ `UNOBSERVABLE`. `pass` + `measured` together ⇒ `VERIFIED` wins (the `head -1` reader is asking about the feature).
- `MEASURED` **never blocks, never bounces**; 5c proceeds. It is **not** a pass and is never written as one.
- Retrospective application **refused**: `qa/TASK-1348-verify.md` is shipped evidence and is not re-scored.

⚠️ **Orchestrator-facing consequence, flagged not actioned:** `CLAUDE.md`'s routing rule 5 knows `verified` / `verify-failed` / `UNOBSERVABLE`. `MEASURED` routes like `UNOBSERVABLE` (proceed to 5c) but **is not** it. `CLAUDE.md` is not on my `names:` line — this is a flag, not an edit.

## 4. `VER-§8` cl. 7 corrected — the law's own worked example was refuted

The clause enshrined *"`IA_Card1` → placement mode entered for card **'Cleric'**"*. The census proved **the hand is shuffled and redraws after every play** — slot 1 held `BrightSun`, then `Footman`, then `WatchTower` across three sessions. The example is struck in place, dated, not deleted. Replacement: read **`DeckComponent.Hand`** via `get_actor_property_in_pie` and pick the slot that *currently* holds the card.

Second-order hazard folded into the same amendment: **a card's class decides whether the observable can move at all.** The WatchTower is `AClimbableTower`, the one building overriding `CanScaleFootprint()` to false — which is why the first wheel number was frozen. ⇒ *pick the card for the observable, not for convenience, and say why.*

## 5. `VER-§8` NEW cl. 10 — the most reusable thing this wave produced

**`"binding_found": false` is not evidence a key failed to reach the game.** All four wheel presses reported `false` and the wheel fired. Scoped so it does not over-correct: **`binding_found` may corroborate an observable; it may never *be* one.** cl. 1's canonical `Tab`/`Down` pair survives — there the observable was focus and `binding_found` merely agreed.

Two rig facts boarded in the same clause so rows inherit them by reference:
- **`simulate_button_press`'s surface is UNMEASURED** ⇒ no acceptance line on any row may depend on it.
- **Batching is a method instruction, not a number:** one MCP round trip ≈30 s of PIE clock, a batched `run_verification_sequence` ≈7 s ⇒ batching is the only way inside the 90 s ceiling. A row that writes *"stay under 90 s"* has given the assignee the score and not the technique.

## 6. `TL-§5e` NEW cl. 7d — the trailing ledger, fixed structurally

**Four consecutive hosts** — `TASK-1340` (`d9a98d1`), `TASK-1343` (`4e388d6`), `TASK-1347` (`ce4947d`), `TASK-1351` (`6052dca`) — each minted the same two-file tail, each behaving correctly. *That is a property of the clause, not of the agents.* And `TASK-1347` proved the obvious fix is wrong: the regress does not terminate, because a ledger commit's hash cannot be inside itself either.

⇒ **A mandatory `TAIL-TAKER:` field on every commit-host row**, taking exactly one of two values: **(i)** a boarded, *dispatchable* row ID, or **(ii)** the literal `STANDING — next commit host under TL-§5e cl. 7a` — permitted **only** when the tail is bounded and every file in it is named by path. Value (ii) terminates the regress honestly, because cl. 7a's standing sweep already binds every future host without being copied onto its row.

**A host row with no `TAIL-TAKER:` is defective at boarding — an `SC-§100` blocker against the *manager*,** who is the only party who can board the successor. `TASK-1355` is the first row to carry the field, with value (ii) and both tail files named.

⛔ Not closed, and said rather than glossed: cl. 7d makes the tail **owned**, not **instant**. The between-waves quiet window still needs the protective docs-only row.

## 7. The four housekeeping items

1. **`TASK-1231` → `done`.** Both repo artefacts shipped in `6052dca`. Gate **waived per its own `names:` line** — recorded, not granted by me. `TASK-1351`'s host correctly refused to flip it and named it as owed; this is that flip.
2. **Both scope stretches RULED KEPT.** The §7 ticks: leaving them would have put an unticked open question three lines from a callout answering it — a doc that contradicts itself is worse than one merely incomplete. The `footage-analyst` roster row: `.claude/agents/` holds **7** files, the table listed **5**; adding only the specced one would have shipped a *newly written* wrong number under a count line the row had to touch anyway. The spec was under-written; the deliverable was right. Both are the good shape of `SC-§100` — taken small, declared loudly, each revertible alone.
3. **F4 BOARDED as `TASK-1354`** — the lifecycle line, hard-gates line and mermaid flowchart. Every token must be traced to a **quoted** line of `CLAUDE.md`/`CONVENTIONS.md`, not to my paraphrase (`SC-§97`). The `verify-failed → gameplay-programmer` loop-back edge is named explicitly: a diagram that draws only the happy path is the same defect one notch smaller.
4. **Trailing ledger — fixed as law** (§6), not as a remembered favour.

## 8. Standing obligations, discharged

- **(A) everywhere; (B) refused by name** on `TASK-1353` (2), with the escape hatch written so it cannot be taken quietly.
- **`SC-§134` cl. 7 on every row:** `TASK-1352` and `TASK-1355` carve out their own `status:` lines; `TASK-1354`'s flipper is **named** (the manager) because its `names:` excludes the board; `TASK-1353` is a manager row (owns every field, no fence to carve — stated, not omitted).
- **Capability ceilings declared at boarding:** ceilings 1–6 inherited by reference + the four the census added (7 `binding_found` · 8 shuffled hand · 9 `simulate_button_press` unmeasured · 10 batching).
- **`SC-§121` key census:** owed only if a row binds a key. **None does** — (C2) is an aim source, not a keybind. Declared rather than silent, with the `Escape`/`AS-§6 A-2` constraint carried to `TASK-1353` in case its mechanism changes that.
- **H1/H2 written verbatim**, parentheses intact, on `TASK-1353` for copying onto the code row. **H2 is not optional even though (C1) closed** — the fix touches the same controller and the same input frame, and a camera regression shipped inside an aim fix is exactly what a closure makes invisible.
- **`DeprojectMousePositionToWorld` linkage preserved** — `TASK-1353` (3), fold-in-or-declare-out, never silently left behind.
- **`SC-§126` cl. 10/11:** no line address handed as fact anywhere; `SiegePlayerController.cpp` is cited by quoted text only.
- **`SC-§129`:** does not fire — no row in this wave reads, writes or runs `Tools/run_suite_bounded.ps1`.

## 9. Anchor discipline

`Edit` only (`SC-§120`). **`replace_all` never used** (`SC-§127`). Counts measured at my own instant, not inherited: bare `^- status:` = **1348** (which is why no bare anchor was used anywhere); `BOARDED, ⛔ NOT DISPATCHED (⭐ TASK-1350)` = **1**; the `TASK-1230` blocked-by line = **1**; the insertion header = **1**. Every write read back as **state** (`SC-§104`): both `done` flips confirmed at their own rows, four new `#### TASK-135[2-5]` headers present, all five `CONVENTIONS.md` amendments confirmed by content.

## 10. Fences honoured

⛔ No code · no asset · no compile · no suite · no git · no push · no engine call · **no editor lifecycle action — PID 27484 untouched, which `TASK-1352` depends on** · no other row's `status:` line beyond the two housekeeping flips this row was dispatched to make.

## 11. The tail I leave

`handoffs/TASK-1350-manager.md` (this file) and the board/law writes are **all dirty and all named** in `TASK-1355`'s expected pathspec — which that host **re-derives at its own instant** and does not inherit from this list (`SC-§133`; `TASK-1351` predicted two and measured four).
