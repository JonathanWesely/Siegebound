# TASK-1574 — gameplay-programmer handoff (HELP-REGISTER-PASS, with the `TASK-1581` re-scope: spec (6), the `qa/TASK-1561.md` residue)

- **Row:** `TASKBOARD.md` `#### TASK-1574 ` (marker `TASK-1574-HELP-REGISTER-PASS`; re-scope marker `TASK-1574-RESCOPED-1561-RESIDUE-2026-09-28`). One handoff covers (1)–(6), as the re-scope asks.
- **Status set:** `ready-for-qa` → gate `TASK-1575` → `TASK-1576` builds on the reviewed bytes → 5a `TASK-1578` → 5b `TASK-1579` → host `TASK-1580`.
- **Size valve (4):** not used. Every hit in every row was rewritten in this session. No row is half-done.
- **Summary:** **164 developer-register hits in 25 of the 27 rows**, all inside `Detail` strings (one-liners, display names: 0). All 164 rewritten in plain words, same rule, no number typed, no `::`, no `()`. **134 `Detail` literals changed**: the hit literals plus the continuation literals a rewritten sentence ran onto. **Moved pins: 0.** The spec (6) residue (W1 a/b/c, W2, N1) is done. **The `::` / `()` guard arm is re-anchored** (the old `TASK-1562` anchor literal is gone) and named in §6 with the new label. **Five false claims found, kept, reported (§9)** — one of them is the `Cards.StackUpgrade` "ONE KIND OF BUILDING REFUSES TO BE STACKED" paragraph, whose fix row `TASK-973` was boarded 2026-09-03 and never ran.

## 0. Start state (spec (0)) and declared tooling (`SC-§71a`)

- **Blocker met:** `TASK-1562` committed `9b82e8d99e8b09447f2bcdb13a31db5c0f2e88e4` (HEAD at my instant).
- **Start bytes = `TASK-1562`'s committed bytes, measured before any edit** (`git --no-optional-locks hash-object <file>` vs `git --no-optional-locks rev-parse HEAD:./<file>`, both equal; plus `sha256sum`):

| File | blob (worktree = HEAD) | sha256 | bytes | lines |
|---|---|---|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/SiegeControlsHelpWidget.cpp` | `9a5d7ff6…` | `6f511582cee9929893daf85d1b51ff98389ac70e62b8f3ab84308db400ddc990` | 279507 | 4503 |
| `Source/GitClaudeUnrealTest/Siegebound/SiegeControlsHelpWidget.h` | `488df9df…` | `249277ed46601d16bb8f717ead2d76e7486efb76f60983662e5f6968917d2720` | 81005 | 1400 |
| `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeControlsHelpTest.cpp` | `ac3d292c…` | `33fd21e74d9378a4b8b28a39d79fd2b72b7e37bf1b8d031988dc3138f2a58c5f` | 150550 | 2625 |

  All three equal the dispatch's anchors and `qa/TASK-1561.md` §4's gated bytes.
- **Scratch copies of the start bytes** (my diffs and checks run against these and against `9b82e8d`, never against a working copy): `scratchpad\W.start.cpp`, `W.start.h`, `T.start.cpp` in my session scratchpad (`…\35e683bf-936e-4e1a-93dd-6b0127c2c1b8\scratchpad\`).
- **Read-only git, declared (all `--no-optional-locks`):** `log -1`, `hash-object`, `rev-parse HEAD:./<path>`, `ls-files -s`, `status --porcelain -- Source/`, `diff --stat` and `diff -U0 9b82e8d -- <the three files>`, and one `log -- handoffs/TASK-973-programmer.md` (empty). No mutating git.
- **Instruments (read-only Python in the scratchpad; they read the repo and write only to the scratchpad):**
  - `lex.py` — a C++ lexer tracking code, `//` and `/* */` comments, `"…"` strings with escapes, char literals (digit-separator aware), raw strings (none found). It composes each row's `OneLine` and `Detail` from the `GetActions()` registry. On the start bytes it reports 610 literals and 27263 composed `Detail` chars, exactly `qa/TASK-1561.md` §2's figures. **I had read those figures before building it, so this is a claimed agreement (`SC-§92`), not an unclaimed one.**
  - `verify.py` — skeleton, literal-by-literal and comment-sequence diffs, and every pin constraint listed in §4.
  - `hits.py` — the census as data: each quoted hit must be present in the start row's composed `Detail` and absent from the after row's.
  - `pins.py` — every test literal (≥ 4 chars) that is a substring of any row's composed prose, before and after.
  - `arm.py` — the §6 anchor count over every file under `Source/`, and the simulated injection.
  - `census.py` — a vocabulary sweep over the composed prose (the classes in §1).
- **Nothing else ran:** no compile, no PIE, no editor or Unreal process touched (PID 12112 is the verifier's), no asset, no `CONVENTIONS.md`, no mutating git.
- **Line endings:** all three files are LF, no BOM, 0 CR bytes after the edits.

## 1. The census (spec (1)) — **164 hits, 25 rows, all `Detail`**

**Method.** I read every `Detail` string and every one-liner of all 27 rows in full, by reading the registry top to bottom (not by regex alone), and flagged wording a player would not use. Classes, declared so QA can check the line I drew:

- **Input-system vocabulary:** bound / binding / unbound, input action, input asset, "action" in the input-action sense, mapping context, modifier, axis, trigger setup, polled / poll, payload, handler, input surface, input mode.
- **Code and engine vocabulary:** tick / state tick, function, implementation, predicate, counter, idempotent, monotone, early-out, broadcasts, state machine, controller (the code class), subsystem, widget, posture, authority, fails closed, data table, property default, class, validation branch, opaquely, symbol / append (as code objects), logged, decal, material, visual (noun), scratch, teardown / funnels, template, the engine's default handling, preview / tunnels / focus path, positional key, re-derives, owners compose, cursor owner, re-implements, structurally unreachable, transient visual, acquisition radius, colliding half-width, zone-valid, re-acquisition, acquires, drop-test, sunflower offset, oscillate, null, cached, pawn, unpossess, player state, actor, UI, mesh, "the art", `(2D)`, "slot 0" (a zero-based index).
- **Pipeline and board register:** law, load-bearing, legacy, latch / latched, shipped, byte-identically, re-typed, double-cover, pinned, "the flag", "the cancel surface", "a silent ignore", in-flight, mid-flow, published, committing, claims (Escape), precedence.

**Lexer state of every hit: a player-facing literal** — code state, inside `TEXT("…")`, inside that row's `Row.Detail = FText::FromString(FString(…))` statement. None is in a comment, none in a one-liner, display name or `UE_LOG`. 32 hits span a literal join; the table says so. The `[ControlsHelp]` `UE_LOG` strings are excluded, as the spec says.

**The 2 rows with zero hits:** `Hero.Attack`, `Hero.Rally` (both are `TASK-1576` pages; neither was touched).

**One-liners and display names: 0 hits** (all 27 read in full). Chrome strings outside the registry (title, hints, `Close`, `(not bound)`, `(undocumented — TODO)`, headers) are outside spec (1)'s "Detail and one-liner" scope and were not edited; see limitations.

**Line numbers are as of the start bytes (`9b82e8d`) and are hints only. Find each hit by its quoted text.**

| # | Row | Hit, quoted from the start bytes | Start line(s) | Lexer state |
|---|---|---|---|---|
| 1 | `Hero.Move` | "bindings on one action" | 404 | literal |
| 2 | `Hero.Move` | "The back and left rows carry a modifier" | 405 | literal |
| 3 | `Hero.Move` | "turns the input onto the forward axis" | 405 | literal |
| 4 | `Hero.Move` | "these four rows must never be rewritten as a block" | 406 | literal |
| 5 | `Hero.Move` | "the layout subsystem retargets the mapping context's keys" | 409-410 | literal (spans a join) |
| 6 | `Hero.Look` | "Bound to the mouse's" | 445 | literal |
| 7 | `Hero.Look` | "with a modifier that reverses" | 445 | literal |
| 8 | `Hero.Jump` | "Inherited from the character template" | 467 | literal |
| 9 | `Hero.Jump` | "bound on both press and release" | 467 | literal |
| 10 | `Hero.Jump` | "skips the engine's default handling" | 468-469 | literal (spans a join) |
| 11 | `Hero.Jump` | "goes down the same path as lethal damage" | 469-470 | literal (spans a join) |
| 12 | `Hero.Sprint` | "a hold on one action" | 495 | literal |
| 13 | `Cards.Play` | "bound with the slot index as the payload" | 583 | literal |
| 14 | `Cards.Play` | "read from the data table and never from code" | 584 | literal |
| 15 | `Cards.Play` | "placement's sibling on the same input surface" | 588 | literal |
| 16 | `Cards.Play` | "same \"card leaves the hand only at LMB confirm\" law" | 588-589 | literal (spans a join) |
| 17 | `Cards.Play` | "the affordability refusal outranks the type refusal" | 591-592 | literal (spans a join) |
| 18 | `Cards.Play` | "legacy quirk" | 593 | literal |
| 19 | `Cards.Play` | "hand slot 0" | 593 | literal |
| 20 | `Cards.CursorHold` | "the action's trigger setup" | 624-625 | literal (spans a join) |
| 21 | `Cards.CursorHold` | "the release is guarded so a double release cannot unbalance the ignore-look counter" | 625-626 | literal (spans a join) |
| 22 | `Cards.CursorHold` | "placement mode is live" | 627 | literal |
| 23 | `Cards.CursorHold` | "the two owners compose" | 627-628 | literal (spans a join) |
| 24 | `Cards.Discard` | "half-way through committing" | 710 | literal |
| 25 | `Cards.Cancel` | "One action, two keys" | 744 | literal |
| 26 | `Cards.Cancel` | "polled directly every frame" | 747 | literal |
| 27 | `Cards.Cancel` | "input asset is missing" | 748 | literal |
| 28 | `Cards.Cancel` | "the deliberate double-cover" | 748 | literal |
| 29 | `Cards.Cancel` | "re-firing is harmless because the exits are idempotent" | 748-749 | literal (spans a join) |
| 30 | `Cards.Cancel` | "a shipped, bound cancel key" | 751 | literal |
| 31 | `Cards.StackUpgrade` | "the shape of the mesh" | 844 | literal |
| 32 | `Cards.PlacementResize` | "along with the art" | 918 | literal |
| 33 | `Cards.PlacementResize` | "the shape of the mesh" | 929 | literal |
| 34 | `Orders.Attack` | "The sequence is one function" | 956-957 | literal (spans a join) |
| 35 | `Orders.Attack` | "in-flight pick" | 957 | literal |
| 36 | `Orders.Attack` | "latch the stance" | 958 | literal |
| 37 | `Orders.Attack` | "BEFORE the latch" | 959 | literal |
| 38 | `Orders.Attack` | "that ordering is load-bearing" | 959 | literal |
| 39 | `Orders.Attack` | "re-read the stance on their next state tick" | 959-960 | literal (spans a join) |
| 40 | `Orders.Attack` | "after latching" | 960 | literal |
| 41 | `Orders.Attack` | "re-assert its station for one tick" | 961 | literal |
| 42 | `Orders.Attack` | "The stance is latched" | 964 | literal |
| 43 | `Orders.Attack` | "pre-command legacy behaviour" | 965 | literal |
| 44 | `Orders.Defend` | "same guard, same two calls" | 986 | literal |
| 45 | `Orders.Defend` | "same final latch" | 986 | literal |
| 46 | `Orders.Defend` | "through the same one implementation" | 987 | literal |
| 47 | `Orders.Defend` | "the acquisition radius is derived" | 990 | literal |
| 48 | `Orders.Defend` | "the castle's live colliding half-width" | 991 | literal |
| 49 | `Orders.Hold` | "ladder every state tick" | 1016 | literal |
| 50 | `Orders.Hold` | "dropped that tick" | 1019 | literal |
| 51 | `Orders.Hold` | "dropped for both types" | 1020 | literal |
| 52 | `Orders.Hold` | "a live, zone-valid target" | 1022 | literal |
| 53 | `Orders.Hold` | "re-acquisition runs only when target-less" | 1022 | literal |
| 54 | `Orders.Hold` | "flipping every tick" | 1023 | literal |
| 55 | `Orders.Hold` | "One monotone upgrade" | 1023 | literal |
| 56 | `Orders.Hold` | "cannot oscillate" | 1025 | literal |
| 57 | `Orders.Hold` | "a sunflower offset" | 1025 | literal |
| 58 | `Orders.Ambush` | "skips the zone drop-test entirely while a live target exists" | 1062 | literal |
| 59 | `Orders.Ambush` | "Ambush acquires through" | 1063 | literal |
| 60 | `Orders.Ambush` | "the exemption governs only when an already-held target is released" | 1064 | literal |
| 61 | `Orders.Ambush` | "dropped, for both types" | 1065 | literal |
| 62 | `Orders.Ambush` | "The single monotone" | 1065 | literal |
| 63 | `Orders.Ambush` | "does not run for Ambush" | 1066 | literal |
| 64 | `Orders.Follow` | "the pick enters at Select" | 1103 | literal |
| 65 | `Orders.Follow` | "structurally unreachable" | 1103-1104 | literal (spans a join) |
| 66 | `Orders.Follow` | "carries zero radii, zero centres" | 1107 | literal |
| 67 | `Orders.Follow` | "no marker decals" | 1108 | literal |
| 68 | `Orders.Follow` | "your live position" | 1109 | literal |
| 69 | `Orders.Follow` | "own sunflower offset" | 1109 | literal |
| 70 | `Orders.Follow` | "resolved every tick and never cached" | 1109-1110 | literal (spans a join) |
| 71 | `Orders.Follow` | "makes hero respawn work for free" | 1110 | literal |
| 72 | `Orders.Follow` | "The target is forced null every tick" | 1111 | literal |
| 73 | `Orders.Follow` | "the follow body calls none of the acquire/attack functions" | 1111-1112 | literal (spans a join) |
| 74 | `Orders.Follow` | "a live pawn exists again" | 1115 | literal |
| 75 | `Orders.Follow` | "on every spawn path" | 1120-1121 | literal (spans a join) |
| 76 | `Orders.Follow` | "nothing player-side auto-engages" | 1121 | literal |
| 77 | `Orders.Follow` | "the eligibility predicate excludes them" | 1122-1123 | literal (spans a join) |
| 78 | `PickMode.Confirm` | "Polled directly every frame" | 1176 | literal |
| 79 | `PickMode.Confirm` | "while a pick is live" | 1176 | literal |
| 80 | `PickMode.Confirm` | "not bound to an input action" | 1176 | literal |
| 81 | `PickMode.Confirm` | "(2D)" | 1180 | literal |
| 82 | `PickMode.Confirm` | "the narrower zone-order predicate" | 1182 | literal |
| 83 | `PickMode.Confirm` | "a transient pick visual" | 1184 | literal |
| 84 | `PickMode.Confirm` | "traces to the surface under the cursor" | 1192 | literal |
| 85 | `PickMode.Confirm` | "refuses on the same flag" | 1194 | literal |
| 86 | `PickMode.Confirm` | "died mid-flow" | 1197 | literal |
| 87 | `PickMode.Resize` | "The decal resizes" | 1224 | literal |
| 88 | `PickMode.Resize` | "The wheel is polled, not bound" | 1225 | literal |
| 89 | `PickMode.Resize` | "verified globally unbound elsewhere" | 1225 | literal |
| 90 | `PickMode.Resize` | "the circle material is missing" | 1227 | literal |
| 91 | `PickMode.Resize` | "you just get no visual" | 1228 | literal |
| 92 | `PickMode.Cancel` | "Polled every frame at the top of the pick branch" | 1255 | literal |
| 93 | `PickMode.Cancel` | "the bound cancel action" | 1256 | literal |
| 94 | `PickMode.Cancel` | "the deliberate double-cover" | 1256 | literal |
| 95 | `PickMode.Cancel` | "The teardown is one function and every exit funnels through it" | 1258 | literal |
| 96 | `PickMode.Cancel` | "unpossess" | 1259 | literal |
| 97 | `PickMode.Cancel` | "releases the melee suppression before any early-out" | 1260 | literal |
| 98 | `PickMode.Cancel` | "all pick scratch" | 1260-1261 | literal (spans a join) |
| 99 | `PickMode.Cancel` | "the flow still owns" | 1261 | literal |
| 100 | `PickMode.Cancel` | "the cursor owners compose" | 1264 | literal |
| 101 | `PickMode.Cancel` | "order key mid-flow" | 1265 | literal |
| 102 | `PickMode.Cancel` | "a silent ignore" | 1265 | literal |
| 103 | `PickMode.Cancel` | "the cancel surface" | 1266 | literal |
| 104 | `Interface.AssistantConsole` | "deliberately un-gated" | 1300 | literal |
| 105 | `Interface.AssistantConsole` | "Opening is gated" | 1301 | literal |
| 106 | `Interface.AssistantConsole` | "a group pick owns the cursor" | 1301-1302 | literal (spans a join) |
| 107 | `Interface.AssistantConsole` | "until the posture is granted" | 1302 | literal |
| 108 | `Interface.AssistantConsole` | "no NPC reference" | 1304 | literal |
| 109 | `Interface.AssistantConsole` | "only a genuine Enter commits" | 1306 | literal |
| 110 | `Interface.AssistantConsole` | "moving focus away" | 1306 | literal |
| 111 | `Interface.AssistantConsole` | "It is left unabsorbed" | 1313 | literal |
| 112 | `Interface.AssistantConsole` | "the shipped placement, targeting and group-pick cancel routes keep firing byte-identically" | 1313-1314 | literal (spans a join) |
| 113 | `Interface.AssistantConsole` | "broadcasts no cancellation" | 1316 | literal |
| 114 | `Interface.AssistantConsole` | "the assistant's own state machine" | 1316-1317 | literal (spans a join) |
| 115 | `Interface.AssistantConsole` | "The console never sets the input mode itself" | 1318 | literal |
| 116 | `Interface.AssistantConsole` | "the controller owns that in one place" | 1318 | literal |
| 117 | `Interface.AssistantAccept` | "The key is caught in preview" | 1378 | literal |
| 118 | `Interface.AssistantAccept` | "it tunnels down the focus path from the root" | 1378 | literal |
| 119 | `Interface.AssistantAccept` | "a plain key handler could never see a printable key the box already ate" | 1379-1380 | literal (spans a join) |
| 120 | `Interface.AssistantAccept` | "consuming them" | 1400 | literal |
| 121 | `Interface.AssistantAccept` | "The grab is as narrow" | 1402 | literal |
| 122 | `Interface.AssistantAccept` | "it fires only while the box is open, enabled" | 1402 | literal |
| 123 | `Interface.AssistantAccept` | "the comparison resolves through the layout subsystem" | 1405-1406 | literal (spans a join) |
| 124 | `Interface.WarMap` | "close is asked first and never gated" | 1453 | literal |
| 125 | `Interface.WarMap` | "The team is resolved from your player state" | 1454-1455 | literal (spans a join) |
| 126 | `Interface.WarMap` | "with no player state" | 1455 | literal |
| 127 | `Interface.WarMap` | "no commander is returned" | 1455-1456 | literal (spans a join) |
| 128 | `Interface.WarMap` | "a wrong default on the wrong side" | 1456 | literal |
| 129 | `Interface.WarMap` | "would gate the map on" | 1456 | literal |
| 130 | `Interface.WarMap` | "price the reveal off the wrong actor" | 1457 | literal |
| 131 | `Interface.WarMap` | "the controller re-implements neither" | 1459 | literal |
| 132 | `Interface.WarMap` | "the cursor posture is touched" | 1460 | literal |
| 133 | `Interface.WarMap` | "until the posture is granted" | 1464 | literal |
| 134 | `Interface.WarMap` | "the widget then fails to create" | 1464 | literal |
| 135 | `Interface.WarMap` | "the posture is rolled back" | 1465 | literal |
| 136 | `Interface.WarMap` | "left as a cursor owner with no UI" | 1465-1466 | literal (spans a join) |
| 137 | `Interface.WarMap` | "polled every frame; that poll exists" | 1469 | literal |
| 138 | `Interface.WarMapReveal` | "a mechanic rule, so it is a property default" | 1516 | literal |
| 139 | `Interface.WarMapReveal` | "never a card-table column" | 1516-1517 | literal (spans a join) |
| 140 | `Interface.WarMapReveal` | "the commander class only holds the number" | 1517 | literal |
| 141 | `Interface.WarMapReveal` | "it never reads a balance" | 1517 | literal |
| 142 | `Interface.WarMapReveal` | "never re-typed" | 1518-1519 | literal (spans a join) |
| 143 | `Interface.WarMapReveal` | "the purchase fails closed" | 1519 | literal |
| 144 | `Interface.WarMapReveal` | "again on the authority" | 1522 | literal |
| 145 | `Interface.WarMapReveal` | "no early-out" | 1522-1523 | literal (spans a join) |
| 146 | `Interface.WarMapMarker` | "through the proper open path" | 1541 | literal |
| 147 | `Interface.WarMapMarker` | "then appends the symbol" | 1541-1542 | literal (spans a join) |
| 148 | `Interface.WarMapMarker` | "That order is pinned" | 1542 | literal |
| 149 | `Interface.WarMapMarker` | "the append never opens the box and never submits" | 1542 | literal |
| 150 | `Interface.WarMapMarker` | "so appending first" | 1543 | literal |
| 151 | `Interface.WarMapMarker` | "The symbol is moved opaquely" | 1545 | literal |
| 152 | `Interface.WarMapMarker` | "the controller never spells it and must not learn which symbols exist" | 1545-1546 | literal (spans a join) |
| 153 | `Interface.WarMapMarker` | "a validation branch there would be a second, drifting copy of a vocabulary it does not own" | 1546-1547 | literal (spans a join) |
| 154 | `Interface.WarMapMarker` | "The append's return value is checked" | 1547 | literal |
| 155 | `Interface.WarMapMarker` | "a refused insert is logged" | 1547 | literal |
| 156 | `Interface.WarMapMarker` | "rather than mis-delivering" | 1548 | literal |
| 157 | `Interface.MapMarks` | "published to him as a place" | 1621 | literal |
| 158 | `Interface.ControlsHelp` | "is a positional key" | 1685 | literal |
| 159 | `Interface.ControlsHelp` | "this row re-derives" | 1686 | literal |
| 160 | `Interface.ControlsHelp` | "read-only on the world" | 1689 | literal |
| 161 | `Interface.ControlsHelp` | "it never claims Escape" | 1692 | literal |
| 162 | `Interface.ControlsHelp` | "Every shipped cancel route keeps firing" | 1692 | literal |
| 163 | `Interface.ControlsHelp` | "who owns the cursor" | 1693 | literal |
| 164 | `Interface.ControlsHelp` | "the existing owners keep their exact shipped precedence" | 1693-1694 | literal (spans a join) |

**Re-census on the after bytes:**

- `hits.py`: all 164 present at start, all 164 absent after, 0 problems.
- `census.py` (the vocabulary classes above as regexes over every composed `Detail` + one-liner): **183 matches at start → 1 after.** The one left is "whose **focused** text field would otherwise swallow the toggle key" (`Interface.WarMap`). I judged "focused text field" to be ordinary UI wording a player meets in any text box and left it; QA may rule otherwise, and the fix would be one literal.

**Considered and NOT flagged (declared, for QA to rule on):**

- **Game vocabulary a player uses:** HUD, ghost, reticle, aggro, despawn, respawn, spawn, stance, station, tier, stage, pick, flow ("the flow ENDS HERE"), confirm (as a noun), overlay, gate / proximity gate (as a noun), frame / every frame, "live" meaning alive, "destroyed" for a group or circle, compounding.
- **Change-history wording**, e.g. `Orders.Defend`'s "What "the band" means CHANGED … no longer a disc", `Cards.Discard`'s "there is no longer any way", `Orders.Follow`'s "any more". These read like patch notes, but they are not developer vocabulary and none is in the spec's seeds. A separate pass is the manager's to board.
- **Design-rationale sentences written in plain words**, e.g. `Cards.StackUpgrade`'s "rather than checking it against a list of names — so any climbable building added later is protected by the same one rule", `Interface.ControlsHelp`'s "a pause would be a new mechanic, not a side effect of a help screen", `Interface.AssistantConsole`'s "(2) a spare close route the game keeps on purpose, though nothing uses it today". They are addressed to a developer more than to a player (T5-shaped), but their words are plain.
- **Jonathan's quotations and his name** ("Jonathan's ruling", "Jonathan's own number", …): quotations of the game's designer, byte-identical.

## 2. Every replacement, old beside new (spec (2))

134 literals, all `Detail`. OLD and NEW are the bytes between the quotes, with `\n` and `\"` as in the source. Line numbers are start → after, hints only. Grouped by row in file order. A row's rewritten sentence often runs onto the next literal; those continuation literals are in the table too, and each belongs to a sentence carrying a §1 hit.

| # | Row | start -> after line | OLD (bytes inside `TEXT("…")`) | NEW |
|---|---|---|---|---|
| 1 | `Hero.Move` | 404 -> 404 | `Four separate bindings on one action — forward, back, strafe left, strafe right. ` | `Four separate keys on one control — forward, back, strafe left, strafe right. ` |
| 2 | `Hero.Move` | 405 -> 405 | `The back and left rows carry a modifier that reverses the direction, and the forward and back rows carry one that turns the input onto the forward axis, ` | `The back and left keys carry a setting that reverses their direction, and the forward and back keys carry one that turns their push onto the forward-and-back line, ` |
| 3 | `Hero.Move` | 406 -> 406 | `which is why these four rows must never be rewritten as a block. ` | `which is why these four keys must never be replaced as one set. ` |
| 4 | `Hero.Move` | 409 -> 409 | `On a non-QWERTY layout these four keep their physical positions: the layout subsystem retargets ` | `On a non-QWERTY layout these four keep their physical positions: the game changes which keys ` |
| 5 | `Hero.Move` | 410 -> 410 | `the mapping context's keys, not your muscle memory.` | `the four answer to, not your muscle memory.` |
| 6 | `Hero.Look` | 445 -> 453 | `Bound to the mouse's up-down and left-right movement, with a modifier that reverses the up-down direction. ` | `Follows the mouse's up-down and left-right movement, with a setting that reverses the up-down direction. ` |
| 7 | `Hero.Jump` | 467 -> 478 | `Inherited from the character template and bound on both press and release.\n\n` | `The jump comes from the basic character the game was built on, and it reacts to both the press and the release.\n\n` |
| 8 | `Hero.Jump` | 468 -> 479 | `Falling out of the world is a death, not a despawn — the hero deliberately skips the engine's ` | `Falling out of the world is a death, not a despawn — instead of simply deleting your hero, which is what ` |
| 9 | `Hero.Jump` | 469 -> 480 | `default handling (which would delete your hero outright) and goes down the same ` | `would happen by default, the game deliberately treats the fall exactly like lethal damage, ` |
| 10 | `Hero.Jump` | 470 -> 481 | `path as lethal damage, so the standard respawn brings you back at your castle.` | `so the standard respawn brings you back at your castle.` |
| 11 | `Hero.Sprint` | 495 -> 512 | `Sprinting and attacking are independent: sprint is a hold on one action, melee is a press on ` | `Sprinting and attacking are independent: sprint is a hold on one control, melee is a press on ` |
| 12 | `Cards.Play` | 583 -> 602 | `Six keys, six hand slots, bound with the slot index as the payload. What happens next depends ` | `Six keys, six hand slots, and each key carries its own slot's number. What happens next depends ` |
| 13 | `Cards.Play` | 584 -> 603 | `on the card's type, read from the data table and never from code:\n\n` | `on the card's type, read from the card list and never hard-wired into the game:\n\n` |
| 14 | `Cards.Play` | 588 -> 607 | `• Spell → targeting mode, placement's sibling on the same input surface; same \"card leaves ` | `• Spell → targeting mode, placement's twin, worked with the same clicks and keys and under the same rule: the card ` |
| 15 | `Cards.Play` | 589 -> 608 | `the hand only at LMB confirm\" law. The one exception is Gold Steal, which resolves instantly ` | `leaves your hand only at the left-click confirm. The one exception is Gold Steal, which resolves instantly ` |
| 16 | `Cards.Play` | 591 -> 610 | `Gold is checked before the type is considered — the affordability refusal outranks the type ` | `Gold is checked before the type is considered — if you are short of gold, that refusal comes ahead of any ` |
| 17 | `Cards.Play` | 592 -> 611 | `refusal. A press is quietly ignored — not refused — after match end, mid-placement or ` | `refusal about the card's type. A press is quietly ignored — not refused — after match end, mid-placement or ` |
| 18 | `Cards.Play` | 593 -> 612 | `mid-targeting. Key 1 has one legacy quirk: with hand slot 0 empty it falls back to the ` | `mid-targeting. Key 1 has one leftover quirk: with the first hand slot empty it falls back to the ` |
| 19 | `Cards.CursorHold` | 624 -> 651 | `The release fires on a normal release and on a cancelled press, so the hold can never stick regardless of the action's ` | `The release fires on a normal release and on a cancelled press, so the hold can never stick, however the key's ` |
| 20 | `Cards.CursorHold` | 625 -> 652 | `trigger setup, and the release is guarded so a double release cannot unbalance the ` | `press is set up, and a release only counts while the key is really held, so a double release cannot upset the ` |
| 21 | `Cards.CursorHold` | 626 -> 653 | `ignore-look counter.\n\n` | `game's count of what is holding camera look off.\n\n` |
| 22 | `Cards.CursorHold` | 627 -> 654 | `Releasing while placement mode is live leaves the cursor to placement mode — the two owners ` | `Releasing while you are in placement mode leaves the cursor to placement mode — the two share ` |
| 23 | `Cards.CursorHold` | 628 -> 655 | `compose rather than fight.` | `the cursor rather than fight over it.` |
| 24 | `Cards.Discard` | 710 -> 743 | `half-way through committing would hand the confirm a different one. Back out first with ` | `half-way through playing would hand the confirm a different one. Back out first with ` |
| 25 | `Cards.Cancel` | 744 -> 780 | `One action, two keys, and it is the same gesture everywhere. It exits placement mode, exits ` | `One control, two keys, and it is the same gesture everywhere. It exits placement mode, exits ` |
| 26 | `Cards.Cancel` | 747 -> 783 | `The same two keys are ALSO polled directly every frame, so cancelling still works even if the ` | `The same two keys are ALSO checked directly every frame, so cancelling still works even if the ` |
| 27 | `Cards.Cancel` | 748 -> 784 | `input asset is missing — the deliberate double-cover, and re-firing is harmless because the ` | `game's setup for this control is missing — a deliberate backup, and a cancel that fires twice is harmless because ` |
| 28 | `Cards.Cancel` | 749 -> 785 | `exits are idempotent.\n\n` | `backing out of something you have already left changes nothing.\n\n` |
| 29 | `Cards.Cancel` | 751 -> 787 | `Escape is a shipped, bound cancel key.` | `Escape is a built-in cancel key.` |
| 30 | `Cards.StackUpgrade` | 844 -> 886 | `fixed to the shape of the mesh, so stretching the building would take the ladder with it and ` | `fixed to the building's exact shape, so stretching the building would take the ladder with it and ` |
| 31 | `Cards.PlacementResize` | 918 -> 969 | `building blocks along with the art.\n\n` | `building blocks along with what you see.\n\n` |
| 32 | `Cards.PlacementResize` | 929 -> 980 | `ladder is fixed to the shape of the mesh, so it cannot be resized, and the wheel is simply dead ` | `ladder is fixed to the building's exact shape, so it cannot be resized, and the wheel is simply dead ` |
| 33 | `Orders.Attack` | 956 -> 1012 | `Immediate, army-wide, and it releases every standing group order. The sequence is one ` | `Immediate, army-wide, and it releases every standing group order. It runs as one fixed ` |
| 34 | `Orders.Attack` | 957 -> 1013 | `function, shared with the assistant's charge: abort any in-flight pick → clear all unit ` | `sequence, shared with the assistant's charge: abort any pick you are part-way through → clear all unit ` |
| 35 | `Orders.Attack` | 958 -> 1014 | `groups → latch the stance.\n\n` | `groups → lock in the stance.\n\n` |
| 36 | `Orders.Attack` | 959 -> 1015 | `The release runs BEFORE the latch and that ordering is load-bearing — units re-read the ` | `The release comes BEFORE the stance is locked in, and that order matters — units check the ` |
| 37 | `Orders.Attack` | 960 -> 1016 | `stance on their next state tick, so releasing after latching would let a group about to be ` | `stance again at their next decision, so releasing afterwards would let a group about to be ` |
| 38 | `Orders.Attack` | 961 -> 1017 | `destroyed re-assert its station for one tick.\n\n` | `destroyed go back to its station for one more decision.\n\n` |
| 39 | `Orders.Attack` | 964 -> 1020 | `The stance is latched — it persists until replaced — and the game records your first command ` | `The stance is locked in — it stays until replaced — and the game records your first command ` |
| 40 | `Orders.Attack` | 965 -> 1021 | `and keeps that record for the rest of the match, so the pre-command legacy behaviour never ` | `and keeps that record for the rest of the match, so the old behaviour from before your first command never ` |
| 41 | `Orders.Defend` | 986 -> 1049 | `The exact mirror of Attack — same guard, same two calls, same order, same final latch, ` | `The exact mirror of Attack — same check, same two steps, same order, same stance locked in at the end, ` |
| 42 | `Orders.Defend` | 987 -> 1050 | `through the same one implementation. Units fall back toward your own castle and engage only ` | `all done by the very same sequence. Units fall back toward your own castle and engage only ` |
| 43 | `Orders.Defend` | 990 -> 1053 | `is the band past the castle's wall face, and the acquisition radius is derived at every ` | `is the band past the castle's wall face, and how far out a unit will pick a target is worked out at every ` |
| 44 | `Orders.Defend` | 991 -> 1054 | `decision from the castle's live colliding half-width plus that band, by ` | `decision from how far the castle's walls actually reach from its centre, plus that band, by ` |
| 45 | `Orders.Hold` | 1016 -> 1085 | `ladder every state tick: enemies in the attack zone first, else enemies in the position zone, ` | `ladder at every decision: enemies in the attack zone first, else enemies in the position zone, ` |
| 46 | `Orders.Hold` | 1019 -> 1088 | `zones is dropped that tick — the unit disengages and returns toward its station. A dead ` | `zones is dropped at that same decision — the unit disengages and returns toward its station. A dead ` |
| 47 | `Orders.Hold` | 1020 -> 1089 | `target is dropped for both types.\n\n` | `target is dropped for both orders.\n\n` |
| 48 | `Orders.Hold` | 1022 -> 1091 | `a live, zone-valid target is kept and re-acquisition runs only when target-less, which is ` | `a live target still inside the zones is kept, and a new target is looked for only when the unit has none, which is ` |
| 49 | `Orders.Hold` | 1023 -> 1092 | `what stops the goal flipping every tick. One monotone upgrade exists, HOLD only: a ` | `what stops the goal flipping back and forth. A single one-way upgrade exists, HOLD only: a ` |
| 50 | `Orders.Hold` | 1025 -> 1094 | `other way, so the two tiers cannot oscillate. Stations are spread by a sunflower offset so ` | `other way, so the two tiers cannot keep swapping. Stations are spread out in a spiral so ` |
| 51 | `Orders.Ambush` | 1062 -> 1137 | `The difference: AMBUSH skips the zone drop-test entirely while a live target exists. It keeps ` | `The difference: while it has a live target, AMBUSH never drops it for leaving the zones. It keeps ` |
| 52 | `Orders.Ambush` | 1063 -> 1138 | `the target until the kill, then the ladder resumes. Ambush acquires through exactly the same ` | `the target until the kill, then the ladder resumes. Ambush picks its targets through exactly the same ` |
| 53 | `Orders.Ambush` | 1064 -> 1139 | `two tiers; the exemption governs only when an already-held target is released.\n\n` | `two tiers; that exception only affects when a target it already has is let go.\n\n` |
| 54 | `Orders.Ambush` | 1065 -> 1140 | `A dead target is still dropped, for both types. The single monotone position→attack upgrade ` | `A dead target is still dropped, for both orders. The single one-way position→attack upgrade ` |
| 55 | `Orders.Ambush` | 1066 -> 1141 | `is HOLD-only and does not run for Ambush.\n\n` | `is HOLD-only and does not happen for Ambush.\n\n` |
| 56 | `Orders.Follow` | 1103 -> 1185 | `the pick enters at Select and confirms there, and the later stages are structurally ` | `the pick opens at Select and confirms there, and the later stages can never be ` |
| 57 | `Orders.Follow` | 1104 -> 1186 | `unreachable. Jonathan's reason: \"There is only one mouse scroll circle used for this, and it ` | `reached. Jonathan's reason: \"There is only one mouse scroll circle used for this, and it ` |
| 58 | `Orders.Follow` | 1107 -> 1189 | `The anchor is you, not a piece of ground: a follow group carries zero radii, zero centres ` | `The anchor is you, not a piece of ground: a follow group stores no circle sizes, no centre points ` |
| 59 | `Orders.Follow` | 1108 -> 1190 | `and no marker decals, and the select circle is destroyed at confirm rather than left on the ` | `and no markers on the ground, and the select circle is destroyed at confirm rather than left on the ` |
| 60 | `Orders.Follow` | 1109 -> 1191 | `map. The station is your live position plus that unit's own sunflower offset, resolved every ` | `map. Each unit's station is wherever you are right now plus that unit's own spot in the spiral, worked out fresh ` |
| 61 | `Orders.Follow` | 1110 -> 1192 | `tick and never cached — which is exactly what makes hero respawn work for free.\n\n` | `moment to moment and never stored — which is exactly why following carries on by itself after your hero respawns.\n\n` |
| 62 | `Orders.Follow` | 1111 -> 1193 | `Followers never attack. The target is forced null every tick and the follow body calls none ` | `Followers never attack. Their target is cleared constantly, and following never uses any ` |
| 63 | `Orders.Follow` | 1112 -> 1194 | `of the acquire/attack functions. A following Cleric still heals — healing is not attacking, ` | `of the target-finding or attacking steps. A following Cleric still heals — healing is not attacking, ` |
| 64 | `Orders.Follow` | 1115 -> 1197 | `a live pawn exists again, including a brand-new one after respawn.\n\n` | `you have a living hero again, including a brand-new one after respawn.\n\n` |
| 65 | `Orders.Follow` | 1120 -> 1202 | `THE SPAWN DEFAULT: every follow-eligible Blue unit spawns already following you, on every ` | `THE SPAWN DEFAULT: every follow-eligible Blue unit spawns already following you, whichever ` |
| 66 | `Orders.Follow` | 1121 -> 1203 | `spawn path, and nothing player-side auto-engages any more — you personally order every ` | `way it spawns, and nothing on your side starts a fight by itself any more — you personally order every ` |
| 67 | `Orders.Follow` | 1123 -> 1205 | `eligibility predicate excludes them. Enrolment is unconditional: a unit spawned after you ` | `rule for who may follow leaves them out. Enrolment is unconditional: a unit spawned after you ` |
| 68 | `PickMode.Confirm` | 1176 -> 1267 | `Polled directly every frame while a pick is live, not bound to an input action. Your hero ` | `Read straight from the mouse button every frame while a pick is open, rather than through the game's control setup. Your hero ` |
| 69 | `PickMode.Confirm` | 1180 -> 1271 | `(2D) joins the group. An empty circle is refused and you STAY in the stage — a different ` | `(measured flat, ignoring height) joins the group. An empty circle is refused and you STAY in the stage — a different ` |
| 70 | `PickMode.Confirm` | 1182 -> 1273 | `differs by order: Hold and Ambush use the narrower zone-order predicate, while Follow is ` | `differs by order: Hold and Ambush use the narrower rule for zone orders, while Follow is ` |
| 71 | `PickMode.Confirm` | 1184 -> 1275 | `(Ogre/Sapper) and the entire enemy side. This circle is a transient pick visual: it is ` | `(Ogre/Sapper) and the entire enemy side. This circle is only there while you pick: it is ` |
| 72 | `PickMode.Confirm` | 1192 -> 1283 | `The circle you are drawing traces to the surface under the cursor — flat floor, hill crown ` | `The circle you are drawing sits on the ground under the cursor — flat floor, hill crown ` |
| 73 | `PickMode.Confirm` | 1194 -> 1285 | `confirm refuses on the same flag, so what you see is what the click does.\n\n` | `confirm refuses in exactly the same case, so what you see is what the click does.\n\n` |
| 74 | `PickMode.Confirm` | 1197 -> 1288 | `members that died mid-flow are dropped.` | `members that died part-way through are dropped.` |
| 75 | `PickMode.Resize` | 1224 -> 1322 | `circle never touches an earlier one. The decal resizes in place as you scroll.\n\n` | `circle never touches an earlier one. The circle on the ground resizes in place as you scroll.\n\n` |
| 76 | `PickMode.Resize` | 1225 -> 1323 | `The wheel is polled, not bound, and it is verified globally unbound elsewhere — it is inert ` | `The game reads the wheel directly rather than through its control setup, and it was checked that no control anywhere else is set to the wheel — it is inert ` |
| 77 | `PickMode.Resize` | 1227 -> 1325 | `If the circle material is missing the radius still changes and the confirm still uses it — ` | `If the circle's graphics are missing the radius still changes and the confirm still uses it — ` |
| 78 | `PickMode.Resize` | 1228 -> 1326 | `you just get no visual.` | `you just cannot see the circle.` |
| 79 | `PickMode.Cancel` | 1255 -> 1362 | `Polled every frame at the top of the pick branch, before anything else runs, and also ` | `Checked every frame, first, before anything else in the pick happens, and also ` |
| 80 | `PickMode.Cancel` | 1256 -> 1363 | `reachable through the bound cancel action — the deliberate double-cover. Cancelling leaves ` | `reachable through the regular cancel control — a deliberate backup. Cancelling leaves ` |
| 81 | `PickMode.Cancel` | 1258 -> 1365 | `The teardown is one function and every exit funnels through it — the final confirm, either ` | `One clean-up step handles every way out of a pick — the final confirm, either ` |
| 82 | `PickMode.Cancel` | 1259 -> 1366 | `cancel route, {Orders.Attack} or {Orders.Defend}, match end, hero death, unpossess and match ` | `cancel route, {Orders.Attack} or {Orders.Defend}, match end, hero death, losing control of your hero and match ` |
| 83 | `PickMode.Cancel` | 1260 -> 1367 | `reset. It releases the melee suppression before any early-out, clears the stage and all pick ` | `reset. It lets your hero swing again before anything can cut the clean-up short, clears the stage and everything the pick ` |
| 84 | `PickMode.Cancel` | 1261 -> 1368 | `scratch, destroys every circle the flow still owns — a cancel at any stage kills all live ` | `was holding, destroys every circle the pick still has — a cancel at any stage kills all live ` |
| 85 | `PickMode.Cancel` | 1264 -> 1371 | `free-look unless the interface-cursor key is still held, because the cursor owners compose.\n\n` | `free-look unless the interface-cursor key is still held, because anything that still needs the cursor keeps it.\n\n` |
| 86 | `PickMode.Cancel` | 1265 -> 1372 | `Re-pressing the same order key mid-flow does nothing — it is a silent ignore; right-click or ` | `Re-pressing the same order key part-way through does nothing — it is quietly ignored; right-click or ` |
| 87 | `PickMode.Cancel` | 1266 -> 1373 | `Escape is the cancel surface.` | `Escape is how you cancel.` |
| 88 | `Interface.AssistantConsole` | 1300 -> 1415 | `deliberately un-gated, because a close that can be refused is a close that can strand your ` | `deliberately never blocked, because a close that can be refused is a close that can strand your ` |
| 89 | `Interface.AssistantConsole` | 1301 -> 1416 | `cursor. Opening is gated: it is refused while placement, spell targeting or a group pick owns ` | `cursor. Opening has conditions: it is refused while placement, spell targeting or a group pick is using ` |
| 90 | `Interface.AssistantConsole` | 1302 -> 1417 | `the cursor, or after match end, and nothing is created or shown until the posture is granted ` | `the cursor, or after match end, and nothing is created or shown until the cursor is handed over ` |
| 91 | `Interface.AssistantConsole` | 1304 -> 1419 | `It works anywhere. Unlike the war map there is no proximity check, no NPC reference and no ` | `It works anywhere. Unlike the war map there is no proximity check, no commander to look for and no ` |
| 92 | `Interface.AssistantConsole` | 1306 -> 1421 | `Sending: type and press Enter — only a genuine Enter commits; moving focus away or clearing ` | `Sending: type and press Enter — only a genuine Enter sends it; clicking away or clearing ` |
| 93 | `Interface.AssistantConsole` | 1313 -> 1428 | `Escape does NOT close the chat box, permanently. It is left unabsorbed so the shipped ` | `Escape does NOT close the chat box, permanently. The box deliberately lets Escape through so the ` |
| 94 | `Interface.AssistantConsole` | 1314 -> 1429 | `placement, targeting and group-pick cancel routes keep firing byte-identically while the box ` | `placement, targeting and group-pick cancels keep working exactly as usual while the box ` |
| 95 | `Interface.AssistantConsole` | 1316 -> 1431 | `A close is not a cancel: closing the window broadcasts no cancellation — only the ` | `A close is not a cancel: closing the window cancels nothing — only the ` |
| 96 | `Interface.AssistantConsole` | 1317 -> 1432 | `assistant's own state machine may turn one into the other.\n\n` | `assistant itself may turn one into the other.\n\n` |
| 97 | `Interface.AssistantConsole` | 1318 -> 1433 | `The console never sets the input mode itself; the controller owns that in one place. A ` | `The chat box never decides by itself who gets the mouse and keyboard; the game decides that in one place. A ` |
| 98 | `Interface.AssistantAccept` | 1378 -> 1501 | `The key is caught in preview — it tunnels down the focus path from the root before the ` | `The key is caught on its way in, before the text box you are typing in gets it, ` |
| 99 | `Interface.AssistantAccept` | 1379 -> 1502 | `focused text box, which is why a plain key handler could never see a printable key the box ` | `which is why it works at all: anything that listened for it after the box would never see a letter ` |
| 100 | `Interface.AssistantAccept` | 1380 -> 1503 | `already ate.\n\n` | `the box had already typed.\n\n` |
| 101 | `Interface.AssistantAccept` | 1400 -> 1523 | `undo and redo, and consuming them would both kill undo and execute an order you never asked ` | `undo and redo, and catching them would both kill undo and execute an order you never asked ` |
| 102 | `Interface.AssistantAccept` | 1402 -> 1525 | `The grab is as narrow as it can be: it fires only while the box is open, enabled, and a ` | `Catching the key is kept as narrow as it can be: it happens only while the box is open, switched on, and a ` |
| 103 | `Interface.AssistantAccept` | 1405 -> 1528 | `On a non-QWERTY layout the game listens at the QWERTY-Z PHYSICAL POSITION — the comparison ` | `On a non-QWERTY layout the game listens at the QWERTY-Z PHYSICAL POSITION — it works out which key ` |
| 104 | `Interface.AssistantAccept` | 1406 -> 1529 | `resolves through the layout subsystem.` | `sits there from your keyboard layout.` |
| 105 | `Interface.WarMap` | 1453 -> 1584 | `A toggle, and close is asked first and never gated, for the same reason as the chat box.\n\n` | `A toggle, and closing is checked first and never blocked, for the same reason as the chat box.\n\n` |
| 106 | `Interface.WarMap` | 1454 -> 1585 | `THE PROXIMITY GATE — and it is proximity to YOUR OWN commander. The team is resolved from ` | `THE PROXIMITY GATE — and it is proximity to YOUR OWN commander. Your team is read from ` |
| 107 | `Interface.WarMap` | 1455 -> 1586 | `your player state and never guessed: with no player state there is no honest answer and no ` | `your own player record and never guessed: with no such record there is no honest answer and no ` |
| 108 | `Interface.WarMap` | 1456 -> 1587 | `commander is returned, because a wrong default on the wrong side would gate the map on the ` | `commander is picked, because a wrong guess on the wrong side would tie the map to the ` |
| 109 | `Interface.WarMap` | 1457 -> 1588 | `enemy's commander and price the reveal off the wrong actor. The distance test and its radius ` | `enemy's commander and take the reveal's price from the wrong one. The distance test and its radius ` |
| 110 | `Interface.WarMap` | 1459 -> 1590 | `the controller re-implements neither.\n\n` | `the map key asks him rather than keeping its own copy of either.\n\n` |
| 111 | `Interface.WarMap` | 1460 -> 1591 | `The gate is checked BEFORE the cursor posture is touched, deliberately, so an out-of-range ` | `The gate is checked BEFORE anything about the cursor changes, deliberately, so an out-of-range ` |
| 112 | `Interface.WarMap` | 1464 -> 1595 | `Nothing appears until the posture is granted, and if the widget then fails to create or ` | `Nothing appears until the cursor is handed over, and if the map then fails to appear or ` |
| 113 | `Interface.WarMap` | 1465 -> 1596 | `fails to report itself open, the posture is rolled back rather than left as a cursor owner ` | `fails to report itself open, the cursor is handed back rather than left claimed by a map ` |
| 114 | `Interface.WarMap` | 1466 -> 1597 | `with no UI.\n\n` | `that is not there.\n\n` |
| 115 | `Interface.WarMap` | 1469 -> 1600 | `polled every frame; that poll exists because a marker click opens the chat box, whose focused ` | `checked directly every frame; that check exists because a marker click opens the chat box, whose focused ` |
| 116 | `Interface.WarMapReveal` | 1516 -> 1655 | `enemy locations\". It is a mechanic rule, so it is a property default and never a card-table ` | `enemy locations\". It is a rule of the game, so it is a fixed setting on the commander and never a line in the card ` |
| 117 | `Interface.WarMapReveal` | 1517 -> 1656 | `column, and the commander class only holds the number: it never reads a balance and never ` | `list, and the commander only holds the number: he never looks at anyone's gold and never ` |
| 118 | `Interface.WarMapReveal` | 1519 -> 1658 | `re-typed; with no commander the purchase fails closed rather than inventing a fallback ` | `copied anywhere else; with no commander the purchase is simply refused rather than inventing a fallback ` |
| 119 | `Interface.WarMapReveal` | 1522 -> 1661 | `produces the HUD message and again on the authority. Past the spend there is deliberately no ` | `produces the HUD message and again where the purchase is actually settled. Past the spend there is deliberately no ` |
| 120 | `Interface.WarMapReveal` | 1523 -> 1662 | `early-out, so a partial spend is impossible — even an empty survey is a legitimate paid-for ` | `way to stop half-way, so a partial spend is impossible — even an empty survey is a legitimate paid-for ` |
| 121 | `Interface.WarMapMarker` | 1541 -> 1686 | `Clicking a marker opens the chat box first, through the proper open path, then appends the ` | `Clicking a marker opens the chat box first, the normal way, then adds the ` |
| 122 | `Interface.WarMapMarker` | 1542 -> 1687 | `symbol. That order is pinned: the append never opens the box and never submits, and opening ` | `place's name. That order is fixed: adding the name never opens the box and never sends, and opening ` |
| 123 | `Interface.WarMapMarker` | 1543 -> 1688 | `clears the input field on every open — so appending first and opening second would silently ` | `clears the input field every time — so adding first and opening second would silently ` |
| 124 | `Interface.WarMapMarker` | 1545 -> 1690 | `The symbol is moved opaquely — the controller never spells it and must not learn which ` | `The name is passed along exactly as the map gave it — nothing on the way checks it against a ` |
| 125 | `Interface.WarMapMarker` | 1546 -> 1691 | `symbols exist, because a validation branch there would be a second, drifting copy of a ` | `list of its own, because a second list there would drift out of step with the real one, ` |
| 126 | `Interface.WarMapMarker` | 1547 -> 1692 | `vocabulary it does not own. The append's return value is checked; a refused insert is logged ` | `which lives elsewhere. Whether the name actually went in is checked; a refused insert is written to the game's log ` |
| 127 | `Interface.WarMapMarker` | 1548 -> 1693 | `and inserts nothing rather than mis-delivering.\n\n` | `and adds nothing rather than delivering the wrong thing.\n\n` |
| 128 | `Interface.MapMarks` | 1621 -> 1774 | `your commander: it carries a number, that number is published to him as a place, and an order ` | `your commander: it carries a number, that number is handed to him as a place, and an order ` |
| 129 | `Interface.ControlsHelp` | 1685 -> 1842 | `A toggle. It documents its own key: {Interface.ControlsHelp} is a positional key like any ` | `A toggle. It documents its own key: {Interface.ControlsHelp} is tied to its spot on the keyboard like any ` |
| 130 | `Interface.ControlsHelp` | 1686 -> 1843 | `other, so on a layout that moves it this row re-derives with everything else.\n\n` | `other key, so on a layout that moves it this entry updates with everything else.\n\n` |
| 131 | `Interface.ControlsHelp` | 1689 -> 1846 | `read-only on the world: opening it issues no order, cancels no group, plays no card and ` | `for reading only and changes nothing in the game: opening it issues no order, cancels no group, plays no card and ` |
| 132 | `Interface.ControlsHelp` | 1692 -> 1849 | `list — it never claims Escape. Every shipped cancel route keeps firing while it is open.\n\n` | `list — it never takes Escape for itself. Every normal cancel keeps working while it is open.\n\n` |
| 133 | `Interface.ControlsHelp` | 1693 -> 1850 | `Its cursor is handled in the one place the game decides who owns the cursor, and nowhere else — the ` | `Its cursor is handled in the one place the game decides who gets the cursor, and nowhere else — ` |
| 134 | `Interface.ControlsHelp` | 1694 -> 1851 | `existing owners keep their exact shipped precedence.` | `everything else that uses the cursor keeps exactly the priority it already had.` |

**The same rule, restated — the rewordings that most deserve a check against source (`SC-§101`), with what I read:**

- **`Hero.Move`** "the game changes which keys the four answer to" for "the layout subsystem retargets the mapping context's keys" — `SiegeKeyboardLayoutStatics.cpp`'s KBD-§1 loop assigns only each mapping's `.Key` ("ONLY `.Key` IS ASSIGNED"). "these four keys must never be replaced as one set" keeps the prescriptive "must never be rewritten as a block" as a prescription.
- **`Hero.Jump`** "instead of simply deleting your hero, which is what would happen by default, the game deliberately treats the fall exactly like lethal damage" — `HeroCharacter.h`'s `FellOutOfWorld` doc: "Super: AActor::FellOutOfWorld() would Destroy() the pawn". The jump binding is `AGitClaudeUnrealTestCharacter`'s `Jump` on Started, `StopJumping` on Completed.
- **`Cards.Play`** "with the first hand slot empty" for "hand slot 0" — `ASiegePlayerController::OnCard1Pressed` plays slot 0 or falls back to `EnterPlacementMode(Card1CardID)`.
- **`Cards.CursorHold`** "a release only counts while the key is really held, so a double release cannot upset the game's count of what is holding camera look off" — `ClearUICursorHold` acts only `if (bUICursorHeld)`, then `SetIgnoreLookInput(false)` (the engine's ignore-look counter).
- **`Orders.Attack` / `Orders.Defend`** "same check, same two steps, same order, same stance locked in at the end, all done by the very same sequence" — `ASiegePlayerController::ApplyArmyWideStance`: `if (bMatchEnded) return; CancelGroupPick(); ClearAllUnitGroups(); SetUnitCommand(NewCommand);`, whose own comment reads "same guard, same two calls, same order, same final latch". "units check the stance again at their next decision" — the same comment: "units re-read CurrentCommand on their next 0.25 s state tick". "Decision" is the word the Defend page already used ("derived at every decision").
- **`Orders.Defend`** "from how far the castle's walls actually reach from its centre, plus that band" for "the castle's live colliding half-width plus that band" — `SummonedUnit.cpp`'s note on `ResolveDefendEngagementRadius()`: "the castle's live colliding half-width + DefendRadius".
- **`Orders.Hold`** "Stations are spread out in a spiral" for "a sunflower offset" — `SiegePlayerController.cpp`: "Per-unit stations: deterministic golden-angle sunflower".
- **`PickMode.Confirm`** "(measured flat, ignoring height)" for "(2D)" — the Select-stage confirm tests `FVector::DistSquared2D(Unit->GetActorLocation(), GroupPickLocation) <= SelectRadiusSq`.
- **`PickMode.Cancel`** "It lets your hero swing again before anything can cut the clean-up short" for "releases the melee suppression before any early-out" — `CancelGroupPick()` calls `GroupPickHero->SetMeleeSuppressed(false)` before its `if (GroupPickStage == EGroupPickStage::None) return;`. "losing control of your hero" for "unpossess" — `ASiegePlayerController::OnUnPossess` calls `CancelGroupPick()`.
- **`Interface.WarMap`** "the map key asks him rather than keeping its own copy of either" for "the controller re-implements neither" — kept to the controller's scope, not widened to the whole game.
- **`Interface.WarMapReveal`** "again where the purchase is actually settled" for "again on the authority".
- **`Interface.WarMapMarker`** "the place's name" for "the symbol" — the row's own one-liner says "drop that place's name into the chat box". The closing ruling is a quotation and keeps "symbol" byte-identical.
- **`Interface.AssistantAccept`**: every `Z` is byte-identical (the count of `Z` in the composed page is unchanged), so the one sanctioned letter literal and the Ctrl carve-out both still hold.

**Digits: none added.** Two removed, both zero-based or dev notation: `Cards.Play`'s "slot 0" and `PickMode.Confirm`'s "(2D)". "Key 1", the stage labels, the chat box's `(1)`–`(4)`, Jonathan's quoted 30 and "stage-1" are byte-identical. `Cards.Discard`, `Cards.StackUpgrade`, `Cards.PlacementResize` and `Interface.MapMarks` carry no digit in either field (checked).

## 3. Comments beside the strings

- **One `// TASK-1574 (2026-09-28, the register pass): …` note after the `Detail` statement in each of the 25 changed rows.** Each names, by quotation, the developer wording the prose used to print, cites any mechanism it keeps by symbol or by quoted text (`CITE-BY-TEXT-RULED-2026-09-24`), and says "The claims are unchanged". No added comment cites a line number (checked: 0 `:NNN` / "line NNN").
- **Two of those notes also carry a `⚠️ SC-§101, REPORTED AND NOT FIXED HERE` block** (`Cards.StackUpgrade`, `PickMode.Resize`), naming the false claim and its evidence by symbol. See §9.
- **The `Orders.Ambush` note records that 707's "T5 repair" comment above quotes wording this pass changed** ("skips the zone drop-test entirely"), and gives the new wording, rather than editing the old comment.
- **Every comment op is a pure insert.** Comment tokens 2068 → 2231, 25 insert ops, 0 existing comment lines changed or removed (the sequence diff in `verify.py`). In `Interface.AssistantConsole` my note sits after the existing `TASK-1541` note, which is byte-identical in its original place.
- **No mechanism moved out of player prose into a comment.** Every hit sentence was rewritten in place; the comments hold the old code names, not removed rules.

## 4. Pins (`HELP-§2` mechanism 4) — listed BEFORE editing

**Method.** At 2026-09-28T17:27:39-07:00, before the first edit, `pins.py` listed every literal of `Tests/SiegeControlsHelpTest.cpp`, `SiegeCardHandKeyLabelTest.cpp`, `SiegeMenuInputTest.cpp`, `SiegeMapMarkTest.cpp` and `SiegePlacementTest.cpp` that is a substring of any row's composed prose (`scratchpad\pins.before.txt`). Then I read every prose assertion in `SiegeControlsHelpTest.cpp` by hand (every `ComposeDetailForDisplay` / `ComposeOneLineForDisplay` / `ComposeDetailContent` / `.Detail` read). Only `SiegeControlsHelpTest.cpp` reads help prose; the other four files read chips, ids or their own subsystems' strings.

**The prose pins, and where each one's phrase sits relative to the strings I changed:**

| Pin (test, by text) | Kind | Its phrase in a literal I changed? |
|---|---|---|
| test 14 (e) `DiscardDetail.Contains(TEXT("charged once for the whole hand"))` | present | **No.** Its literal ("The fee is a single set amount and it is charged once for the whole hand, flat. Dumping a ") is byte-identical. My one `Cards.Discard` change is a later literal ("half-way through playing …"). |
| test 14 `DiscardRow->Detail … MakeActionToken(DiscardRow->ActionId)` (`{Cards.Discard}`) | present | **No.** Token literal byte-identical. |
| test 17 positive control, `Interface.MapMarks` detail contains "right-click" | present | **No.** My one `MapMarks` change is the "published to him" literal. |
| test 10 (a) the Ambush page names the Attack order's chip (`{Orders.Attack}`) | present | **No.** The token literal ("Pressing {Orders.Attack} or {Orders.Defend} destroys this group.") is byte-identical. |
| test 10 (b) no unresolved `{` on any page; ≥ 1 page has a token | absent / present | Token sequences per row are unchanged (checked, 8 rows). |
| test 10 (c) `Hero.Jump` identical on QWERTY and Dvorak | property | Holds: `Hero.Jump` still carries no token. |
| test 9 `ForbiddenInPlayerProse` (10 entries) on every row | absent | 0 hits on all 27 composed pages after. |
| test 9 / test 15 detail ≠ one-liner and longer | property | Holds on all 27. |
| test 14 (e) no digit in `Cards.Discard` one-liner or detail; retired `DiscardCost` / `DiscardHandSlot` / `RequestDiscardSlot` absent | absent | Holds. |
| test 14 (f) no "right-click" / "right click" / "discard button" on `Cards.Discard` | absent | Holds. |
| test 15 (e) no digit on `Cards.StackUpgrade`, `Cards.PlacementResize`, `Interface.MapMarks` | absent | Holds (I changed 1, 2 and 1 literals there; no digit added). |
| test 17 (d2) no "right-click" / "right click" / "right button" on `Interface.WarMap` | absent | Holds (11 WarMap literals changed; none of the three phrases entered). |
| test 16 / test 17 pairwise: the wheel rows and the right-click rows have distinct pages | property | Holds: all 27 composed pages are pairwise distinct. |

**Moved pins: 0.** No pinned phrase sits inside a string I changed, so no pin literal moved and `TASK-1578` owes no moved-pin arm from this row.

**Presence diff after the edit** (`pins.before.txt` vs `pins.after.txt`, 16 = 16 lines): exactly one change, and it is incidental. `SiegePlacementTest.cpp`'s `"cannot"` (its `RefusalWords[]` list, which reads the stack cap-notice text, never help prose) now also matches `PickMode.Resize` ("you just cannot see the circle"). No help test reads that literal.

## 5. Spec (6) — the `qa/TASK-1561.md` residue, each site old beside new, cited by text

**W1 (a)** — `Tests/SiegeControlsHelpTest.cpp`, the comment directly above `const TCHAR* ForbiddenInPlayerProse[] =` (a `//` comment; lexer state: comment):

- OLD: "Every one of these belongs in a C++ comment beside the string (TASK-704 §4's citations, rule T1) or is markdown that only means something in a .md file (rule T2)."
- NEW: "Every one of these belongs in a C++ comment beside the string (TASK-704 §4's citations, rule T1; \`::\` and \`()\`, which are rule T5's "a C++ fragment" in the widget's T-rule list) or is markdown that only means something in a .md file (rule T2)."
- Cited by text: the widget's `T5  Sentences addressed to an IMPLEMENTER rather than to a player — provenance of a ruling, "the header says so on purpose", a C++ fragment, a defect post-mortem — move into the comment beside the string …`.

**W1 (b)** — test 9's docstring (the `/** … */` above `IMPLEMENT_SIMPLE_AUTOMATION_TEST( FSiegeControlsHelpAuthoredDetailTest`; lexer state: comment):

- OLD: "… across 24 strings: ⛔ no \`file:line\` citation and ⛔ no markdown markup may reach the player's screen (rules T1 and T2). Those live in C++ comments."
- NEW: "… across 24 strings: ⛔ no \`file:line\` citation, ⛔ no markdown markup and ⛔ no C++ fragment (\`::\`, \`()\`) may reach the player's screen (rules T1 and T2, and rule T5's "a C++ fragment" in the widget's T-rule list). Those live in C++ comments."
- "across 24 strings" is byte-identical and is stale (27 rows today). It is not a W1 site, so it is reported (§9), not fixed.

**W1 (c)** — the guard loop's executable `TestFalse` label (lexer state: **literal**, the first argument of `FString::Printf` inside `TestFalse(…)`):

- OLD: `"Row '%s' detail carries no developer-only fragment '%s' (T1/T2: citations and markup live in comments)"`
- NEW: `"Row '%s' detail carries no developer-only fragment '%s' (T1/T2/T5: citations, markup and C++ fragments live in comments)"`
- Words only: both `%s` stay, in order (`['%s', '%s']` → `['%s', '%s']`). The `Printf` arguments (`*RowName, Forbidden`), the `TestFalse` call, its predicate (`Detail.Contains(Forbidden, ESearchCase::CaseSensitive)`) and its place in the loop are unchanged. The test file's code skeleton is identical (literals masked); literals 408 = 408, and this label is the only one that changed. `IMPLEMENT_` 18 = 18.
- `SC-§93`: this is an executable literal, so it owes a compile and a suite run. Its burden is an assertion MESSAGE, not a predicate, so it owes no `SC-§83` decoy. It does owe the guard arm re-run with the new label (§6), per the re-scope.

**W2** — `SiegeControlsHelpWidget.h`, the `Detail` field's `/** … */` block (lexer state: comment; UHT reads it as `Comment` / `ToolTip` metadata):

- OLD opening: "⭐ THE FULL-SCREEN DETAIL PROSE. FILLED BY TASK-707 from handoffs/TASK-704-programmer.md §4 — ⛔ transferred, ⛔ never re-authored. §4's \`file:line\` citations ride in a C++ COMMENT above each string …"
- NEW opening: "⭐ THE FULL-SCREEN DETAIL PROSE. TASK-707 filled each original row's detail string from handoffs/TASK-704-programmer.md §4's prose for that row. Not every string is still 704's words: TASK-823 wrote its three appended rows at source, and later rows rewrote others, among them TASK-821 (Cards.Discard, in place), TASK-870 (Interface.WarMap) and TASK-1541 (2026-09-27: the code names the prose printed, put into plain words); ⛔ TASK-707's transfer re-authored, re-derived and invented nothing. §4's \`file:line\` citations ride in a C++ COMMENT above each string …"
- It mirrors the `.cpp`'s TASK-707 opening paragraph (the one beginning "TASK-707 filled each original row's detail string from handoffs/TASK-704-programmer.md §4's prose for that row."), clause for clause. The block's later "since TASK-1541 … plain words" paragraph no longer contradicts its opening. Its three example phrases ("a set step", "once per melee cooldown", "a fixed reveal fee") are still in the prose after this pass (`PickMode.Resize` / `Cards.PlacementResize`, `Hero.Attack`, `Interface.WarMapReveal`, all byte-identical literals).
- I did not add `TASK-1574` to the list: the `.cpp` paragraph it mirrors is outside my fence, and "among them" is non-exhaustive, so both stay true and stay identical.

**N1** — `SiegeControlsHelpWidget.h`, the `OneLine` field's doc (lexer state: comment; UHT metadata):

- OLD: `/** The one-line description shown on the row (TASK-704 §4, verbatim). */`
- NEW: "The one-line description shown on the row. For the original row set it is TASK-704 §4's text verbatim, except Cards.Discard's, which TASK-821 rewrote in place; rows 25-27 (Cards.StackUpgrade, Cards.PlacementResize, Interface.MapMarks) are TASK-823's own prose, authored at source." (a 5-line `/** */` block)
- **Why not a pure mirror of the `.cpp`:** the row says scope it "the way the `.cpp` registry block already does" ("every one-liner is its §4 text VERBATIM"). I measured that claim before copying it. `oneliners.py` compared every shipped one-liner with 704 §4's `*One line:*` entries: **23 of the 24 original rows are byte-identical; `Cards.Discard`'s is not** (704: "Click a hand card's discard button to bin it and draw a replacement — it costs gold."; shipped: "Bin every card in your hand at once and draw a full replacement — one flat fee, however many cards you were holding."). The `.cpp` block itself records TASK-821's rewrite ("the one-liner "Click a hand card's discard button…" — TASK-809 removes …"). So the `.h` now says the true, scoped thing, and the `.cpp`'s unscoped "every one-liner" is reported (§9), not fixed (outside my fence).

**The `.h` fence, proven:** code skeleton identical; literals 47 = 47, 0 changed; comment tokens 189 = 189, exactly 2 replaced (the `OneLine` and `Detail` doc blocks). No other `.h` byte moved.

## 6. The guard's mutation arm for `TASK-1578`, re-named at my after-bytes (re-scope consequence 2)

**The old anchor is gone.** This pass rewrote `Hero.Jump`'s second paragraph, so `TASK-1562`'s anchor `TEXT("default handling (which would delete your hero outright) and goes down the same ")` now occurs **0** times in `Source/` (`arm.py`, fixed-string count over every file). The new anchor is also in `Hero.Jump`: outside `TASK-1576`'s five candidate pages (`Interface.MapMarks`, `Cards.Discard`, `Hero.Rally`, `Cards.StackUpgrade`, `Hero.Attack`), so `TASK-1576` cannot move it.

- **Anchor (a whole literal, fixed string):** `TEXT("Falling out of the world is a death, not a despawn — instead of simply deleting your hero, which is what ")`
  - Occurrences in `Source/`: **exactly 1** (`SiegeControlsHelpWidget.cpp`, the `Hero.Jump` `Row.Detail` statement; line 479 at my after-bytes, a hint only). Counted as bytes over every file under `Source/` by `arm.py`.
- **Fragment (unchanged from `qa/TASK-1561.md` §3):** `AHeroCharacter::FellOutOfWorld() ` (one trailing space), prepended inside the literal:
  - mutant: `TEXT("AHeroCharacter::FellOutOfWorld() Falling out of the world is a death, not a despawn — instead of simply deleting your hero, which is what ")`
- **Simulated at my after-bytes** (`arm.py`; the guard list lexed out of the test file itself): the mutant `Hero.Jump` page hits exactly **`['::', '()']`** of the 10 entries `['.cpp:', '.h:', 'handoffs/', 'TASK-', 'SPC:', '**', '`', '§', '::', '()']`. The unmutated page hits none. No other row is hit. The mutant page carries no digit and no `{`, is 397 chars against a 5-char one-liner, and equals no other row's page. The other readers of `Hero.Jump`'s prose are the ones `qa/TASK-1561.md` §3 listed, unchanged by this row (test 10 (c)'s layout identity still holds because the fragment adds no token).
- **Expected: exactly one test red, `Siegebound.ControlsHelp.EveryRowHasAuthoredDetail`, with exactly two errors, one per list entry, each carrying the NEW label:**
  - `Expected 'Row 'Hero.Jump' detail carries no developer-only fragment '::' (T1/T2/T5: citations, markup and C++ fragments live in comments)' to be false.`
  - `Expected 'Row 'Hero.Jump' detail carries no developer-only fragment '()' (T1/T2/T5: citations, markup and C++ fragments live in comments)' to be false.`
  - The log prefixes each with `LogAutomationController: Error: ` and suffixes `[…\Tests\SiegeControlsHelpTest.cpp(NNNN)]`. The `TestFalse` call is at line 1141 of my after-bytes, but `TASK-1576` edits this file next, so match on the message text, not the number.
  - **Reading rule (the re-scope's):** only one of the two lines ⇒ a list entry is dead. The old "(T1/T2: citations and markup live in comments)" tail ⇒ the label edit is not in the binary. Either ⇒ STOP and report.
- **Procedure (`qa/TASK-1561.md` §3, unchanged):** copy the reviewed widget aside; inject with a fixed-string replace whose count must be 1 (never a regex: `(` and `)` are in the fragment); check the sha256 moved; build; run; quote the red; restore from the copy and re-hash to the gated `.cpp` sha256 (`TASK-1577`'s after-anchor, since `TASK-1576` edits the file next); build; green. **Before injecting, assert the anchor occurs exactly once at `qa/TASK-1577.md`'s bytes** (the re-scope's own STOP).

**Arms owed by this row, complete list: 1** — the re-run `::` / `()` guard arm above. No moved-pin arm (§4: 0 moved pins).

**Readers of the old label (re-scope consequence 3).** Fixed-string grep for `T1/T2: citations and markup`:

- **Positive control:** `Source/` → exactly 1 hit before my edit: the (c) literal itself (`Tests/SiegeControlsHelpTest.cpp`, the `TestFalse(*FString::Printf(TEXT("Row '%s' detail carries no developer-only fragment '%s' (T1/T2: …` line). So the grep can see the label.
- `Tools/` → **0**.
- A wider `T1/T2` grep of the whole repo, excluding `.claude/pipeline/` → the test file only.
- ⇒ **Zero other readers.** No moved pin, nothing to report. The pipeline records that quote the old label (`qa/TASK-1561.md` §3, `handoffs/TASK-1562-buildmaster.md` §6 and §13) are history, not readers.

## 7. The 5a shape (for `TASK-1575` to confirm and `TASK-1578` to measure)

- **Three files. `N` stays 566** from this row: no `IMPLEMENT_` added (18 = 18), and no test's shape changed. (`TASK-1576` may add tests; its handoff states the final `N`.)
- **The `.h` changed** (comments only to the C++ compiler, but UHT reads them — `SC-§93`): expect the `Detail` **and** `OneLine` properties' `Comment` / `ToolTip` metadata to change in `SiegeControlsHelpWidget.gen.cpp`, plus the struct and file-registration CRCs, and the package CRC in `GitClaudeUnrealTest.init.gen.cpp` (the `handoffs/TASK-1562-buildmaster.md` §5 shape).
- **The `.h` grew by 9 lines** (1400 → 1409), all above the three `UCLASS`es and below the struct's `GENERATED_BODY()`. So the `.generated.h` line-number macros shift by **+9**: `GENERATED_BODY()` at 441 / 624 / 924 → **450 / 633 / 933** (`_h_441/_h_624/_h_924` → `_h_450/_h_633/_h_933`), and the `UCLASS()` PROLOG lines 438 / 621 / 921 → **447 / 630 / 930**. The struct's `_h_127` does not move (its `GENERATED_BODY()` is still at 127). Measured on the files, not predicted.
- **Assert the exec-symbol set only, 9 = 9 both ways:** `execHandleRowButtonClicked`, `execHandleBackButtonClicked`, `execHandleCloseButtonClicked`, `execRefreshRows`, `execIsDetailViewActive`, `execGetSelectedActionId`, `execIsHelpOpen`, `execCloseHelp`, `execOpenHelp`. This row adds or removes no `UFUNCTION`.
- **The `.cpp`** changes literals and comments only: code skeleton identical, 610 = 610 literals, 134 changed, all `Detail`. **The test file** changes one label literal and two comments.

## 8. Hashes and the diff against `TASK-1562`'s committed bytes

| File | Before sha256 (bytes, lines) = `9b82e8d` | After sha256 (bytes, lines) |
|---|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/SiegeControlsHelpWidget.cpp` | `6f511582cee9929893daf85d1b51ff98389ac70e62b8f3ab84308db400ddc990` (279507, 4503) | **`419cefc896081b86aae6b03402ebf90967152a8a08733b6fefc03a646d893764`** (295138, 4666) |
| `Source/GitClaudeUnrealTest/Siegebound/SiegeControlsHelpWidget.h` | `249277ed46601d16bb8f717ead2d76e7486efb76f60983662e5f6968917d2720` (81005, 1400) | **`31c986935a49b859e20212c164d148f0865993417ed39c4be89d150adcd7bf4e`** (81619, 1409) |
| `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeControlsHelpTest.cpp` | `33fd21e74d9378a4b8b28a39d79fd2b72b7e37bf1b8d031988dc3138f2a58c5f` (150550, 2625) | **`41e7d944ac87486bf2c0973ea0e17d880f49ba307dede0c7abb0400a58ad7304`** (150755, 2628) |

All three after files are LF only (0 CR), no BOM. `TASKBOARD.md` (my `status:` line only) and this handoff are also written; the board is edited concurrently by other agents, so no stable hash is claimed for it.

`git --no-optional-locks diff --stat 9b82e8d -- Source/`: `SiegeControlsHelpWidget.cpp | 431`, `SiegeControlsHelpWidget.h | 15`, `Tests/SiegeControlsHelpTest.cpp | 13`; 3 files, 317 insertions, 142 deletions. (git prints its usual "LF will be replaced by CRLF" notice for these paths; the files on disk are LF.)

`git --no-optional-locks diff -U0 9b82e8d -- <.h> <test> <.cpp>` (read-only, declared). Hunk headers are git's; line numbers in them are as of these bytes.

```diff
diff --git a/GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/SiegeControlsHelpWidget.cpp b/GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/SiegeControlsHelpWidget.cpp
index 9a5d7ff..de25471 100644
--- a/GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/SiegeControlsHelpWidget.cpp
+++ b/GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/SiegeControlsHelpWidget.cpp
@@ -404,3 +404,3 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-				TEXT("Four separate bindings on one action — forward, back, strafe left, strafe right. ")
-				TEXT("The back and left rows carry a modifier that reverses the direction, and the forward and back rows carry one that turns the input onto the forward axis, ")
-				TEXT("which is why these four rows must never be rewritten as a block. ")
+				TEXT("Four separate keys on one control — forward, back, strafe left, strafe right. ")
+				TEXT("The back and left keys carry a setting that reverses their direction, and the forward and back keys carry one that turns their push onto the forward-and-back line, ")
+				TEXT("which is why these four keys must never be replaced as one set. ")
@@ -409,2 +409,2 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-				TEXT("On a non-QWERTY layout these four keep their physical positions: the layout subsystem retargets ")
-				TEXT("the mapping context's keys, not your muscle memory.")));
+				TEXT("On a non-QWERTY layout these four keep their physical positions: the game changes which keys ")
+				TEXT("the four answer to, not your muscle memory.")));
@@ -413,0 +414,8 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
+			// TASK-1574 (2026-09-28, the register pass): player words replace the developer wording
+			// this prose used to print — "bindings on one action", the "rows" carrying "a modifier",
+			// "turns the input onto the forward axis", "rows must never be rewritten as a block", and
+			// "the layout subsystem retargets the mapping context's keys". The mechanism stays here: each
+			// IMC_Hero mapping's .Key is re-targeted one index at a time and the mappings array is never
+			// rewritten, because rewriting it dropped the instanced Negate / SwizzleAxis modifiers (the
+			// "ONLY `.Key` IS ASSIGNED" clause of the KBD-§1 loop in SiegeKeyboardLayoutStatics.cpp).
+			// The claims are unchanged.
@@ -445 +453 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-				TEXT("Bound to the mouse's up-down and left-right movement, with a modifier that reverses the up-down direction. ")
+				TEXT("Follows the mouse's up-down and left-right movement, with a setting that reverses the up-down direction. ")
@@ -454,0 +463,3 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
+			// TASK-1574 (2026-09-28, the register pass): "Follows" and "a setting" replace "Bound to" and
+			// "a modifier" (the IA_Look Mouse2D binding and its Negate_2 modifier, anchored above). The
+			// claim is unchanged.
@@ -467,4 +478,4 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-				TEXT("Inherited from the character template and bound on both press and release.\n\n")
-				TEXT("Falling out of the world is a death, not a despawn — the hero deliberately skips the engine's ")
-				TEXT("default handling (which would delete your hero outright) and goes down the same ")
-				TEXT("path as lethal damage, so the standard respawn brings you back at your castle.")));
+				TEXT("The jump comes from the basic character the game was built on, and it reacts to both the press and the release.\n\n")
+				TEXT("Falling out of the world is a death, not a despawn — instead of simply deleting your hero, which is what ")
+				TEXT("would happen by default, the game deliberately treats the fall exactly like lethal damage, ")
+				TEXT("so the standard respawn brings you back at your castle.")));
@@ -472,0 +484,6 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
+			// TASK-1574 (2026-09-28, the register pass): player words replace the developer wording this
+			// prose used to print — "Inherited from the character template" (the jump is bound in
+			// AGitClaudeUnrealTestCharacter's input setup, Jump on Started and StopJumping on Completed),
+			// "bound on both press and release", "skips the engine's default handling" (the skipped
+			// AActor::FellOutOfWorld Super call, which would Destroy() the pawn) and "goes down the same
+			// path as lethal damage". The claims are unchanged.
@@ -495 +512 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-				TEXT("Sprinting and attacking are independent: sprint is a hold on one action, melee is a press on ")
+				TEXT("Sprinting and attacking are independent: sprint is a hold on one control, melee is a press on ")
@@ -500,0 +518,2 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
+			// TASK-1574 (2026-09-28, the register pass): "one control" replaces "one action" (IA_Sprint
+			// and IA_Attack, two separate input actions). The claim is unchanged.
@@ -583,2 +602,2 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-				TEXT("Six keys, six hand slots, bound with the slot index as the payload. What happens next depends ")
-				TEXT("on the card's type, read from the data table and never from code:\n\n")
+				TEXT("Six keys, six hand slots, and each key carries its own slot's number. What happens next depends ")
+				TEXT("on the card's type, read from the card list and never hard-wired into the game:\n\n")
@@ -588,2 +607,2 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-				TEXT("• Spell → targeting mode, placement's sibling on the same input surface; same \"card leaves ")
-				TEXT("the hand only at LMB confirm\" law. The one exception is Gold Steal, which resolves instantly ")
+				TEXT("• Spell → targeting mode, placement's twin, worked with the same clicks and keys and under the same rule: the card ")
+				TEXT("leaves your hand only at the left-click confirm. The one exception is Gold Steal, which resolves instantly ")
@@ -591,3 +610,3 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-				TEXT("Gold is checked before the type is considered — the affordability refusal outranks the type ")
-				TEXT("refusal. A press is quietly ignored — not refused — after match end, mid-placement or ")
-				TEXT("mid-targeting. Key 1 has one legacy quirk: with hand slot 0 empty it falls back to the ")
+				TEXT("Gold is checked before the type is considered — if you are short of gold, that refusal comes ahead of any ")
+				TEXT("refusal about the card's type. A press is quietly ignored — not refused — after match end, mid-placement or ")
+				TEXT("mid-targeting. Key 1 has one leftover quirk: with the first hand slot empty it falls back to the ")
@@ -594,0 +614,8 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
+			// TASK-1574 (2026-09-28, the register pass): player words replace the developer wording this
+			// prose used to print — "bound with the slot index as the payload" (the BindAction
+			// VarTypes overload that hands keys 2..6 their slot index), "read from the data table and
+			// never from code" (the card's type comes from its DT_Cards row), "placement's sibling on the
+			// same input surface", the quoted "card leaves the hand only at LMB confirm" law, "the
+			// affordability refusal outranks the type refusal", "legacy quirk" and "hand slot 0" (key 1
+			// is OnCard1Pressed, which plays slot 0 or falls back to the M1 Footman placement). The
+			// `1` in "Key 1" stays literal for the reason given above. The claims are unchanged.
@@ -624,5 +651,5 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-				TEXT("The release fires on a normal release and on a cancelled press, so the hold can never stick regardless of the action's ")
-				TEXT("trigger setup, and the release is guarded so a double release cannot unbalance the ")
-				TEXT("ignore-look counter.\n\n")
-				TEXT("Releasing while placement mode is live leaves the cursor to placement mode — the two owners ")
-				TEXT("compose rather than fight.")));
+				TEXT("The release fires on a normal release and on a cancelled press, so the hold can never stick, however the key's ")
+				TEXT("press is set up, and a release only counts while the key is really held, so a double release cannot upset the ")
+				TEXT("game's count of what is holding camera look off.\n\n")
+				TEXT("Releasing while you are in placement mode leaves the cursor to placement mode — the two share ")
+				TEXT("the cursor rather than fight over it.")));
@@ -630,0 +658,6 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
+			// TASK-1574 (2026-09-28, the register pass): player words replace the developer wording this
+			// prose used to print — "the action's trigger setup", "the release is guarded so a double
+			// release cannot unbalance the ignore-look counter" (ClearUICursorHold acts only
+			// `if (bUICursorHeld)`, so SetIgnoreLookInput(false) runs once per hold), "placement mode is
+			// live" and "the two owners compose" (bWantCursor ORs the cursor owners in
+			// ApplyCursorInputState). The claims are unchanged.
@@ -710 +743 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-				TEXT("half-way through committing would hand the confirm a different one. Back out first with ")
+				TEXT("half-way through playing would hand the confirm a different one. Back out first with ")
@@ -711,0 +745,3 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
+			// TASK-1574 (2026-09-28, the register pass): "half-way through playing" replaces "half-way
+			// through committing". The claim is unchanged, and the pinned "charged once for the whole
+			// hand" sentence and both {…} tokens are untouched.
@@ -744 +780 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-				TEXT("One action, two keys, and it is the same gesture everywhere. It exits placement mode, exits ")
+				TEXT("One control, two keys, and it is the same gesture everywhere. It exits placement mode, exits ")
@@ -747,3 +783,3 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-				TEXT("The same two keys are ALSO polled directly every frame, so cancelling still works even if the ")
-				TEXT("input asset is missing — the deliberate double-cover, and re-firing is harmless because the ")
-				TEXT("exits are idempotent.\n\n")
+				TEXT("The same two keys are ALSO checked directly every frame, so cancelling still works even if the ")
+				TEXT("game's setup for this control is missing — a deliberate backup, and a cancel that fires twice is harmless because ")
+				TEXT("backing out of something you have already left changes nothing.\n\n")
@@ -751 +787,7 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-				TEXT("Escape is a shipped, bound cancel key.")));
+				TEXT("Escape is a built-in cancel key.")));
+			// TASK-1574 (2026-09-28, the register pass): player words replace the developer wording this
+			// prose used to print — "One action", "polled directly every frame", "the input asset is
+			// missing" (IA_CancelPlace), "the deliberate double-cover", "re-firing is harmless because the
+			// exits are idempotent" and "a shipped, bound cancel key" (Escape is IMC_Hero's second
+			// IA_CancelPlace key). The raw RMB / Escape poll is the backup for the bound action (the
+			// "raw double-cover" in the citation block above). The claims are unchanged.
@@ -844 +886 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-				TEXT("fixed to the shape of the mesh, so stretching the building would take the ladder with it and ")
+				TEXT("fixed to the building's exact shape, so stretching the building would take the ladder with it and ")
@@ -852,0 +895,9 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
+			// TASK-1574 (2026-09-28, the register pass): "the building's exact shape" replaces "the shape
+			// of the mesh". The claim is unchanged, and no number is typed.
+			// ⚠️ SC-§101, REPORTED AND NOT FIXED HERE: the paragraph that sentence sits in ("ONE KIND OF
+			// BUILDING REFUSES TO BE STACKED, AND IT IS THE ONE YOU CAN CLIMB" … "Hovering one shows RED
+			// with 'That building cannot be stacked'") is FALSE at source. AClimbableTower answers
+			// CanStackHeight() true (its "THE HEIGHT (Z) ANSWER — **TRUE**" doc), and the NotStackable
+			// state's doc in SiegePlayerController.h reads "NO SHIPPED CLASS PRODUCES THIS STATE". The
+			// truth fix is TASK-973 (boarded 2026-09-03, never dispatched). A register pass keeps a false
+			// sentence's claim; it does not repair it.
@@ -918 +969 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-				TEXT("building blocks along with the art.\n\n")
+				TEXT("building blocks along with what you see.\n\n")
@@ -929 +980 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-				TEXT("ladder is fixed to the shape of the mesh, so it cannot be resized, and the wheel is simply dead ")
+				TEXT("ladder is fixed to the building's exact shape, so it cannot be resized, and the wheel is simply dead ")
@@ -933,0 +985,5 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
+			// TASK-1574 (2026-09-28, the register pass): "along with what you see" replaces "along with
+			// the art", and "the building's exact shape" replaces "the shape of the mesh" (the X/Y
+			// refusal: AClimbableTower answers CanScaleFootprint() false because a sideways scale moves
+			// its ladder sockets off the climb line, per that override's own doc). The claims are
+			// unchanged, and no number is typed.
@@ -956,6 +1012,6 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-				TEXT("Immediate, army-wide, and it releases every standing group order. The sequence is one ")
-				TEXT("function, shared with the assistant's charge: abort any in-flight pick → clear all unit ")
-				TEXT("groups → latch the stance.\n\n")
-				TEXT("The release runs BEFORE the latch and that ordering is load-bearing — units re-read the ")
-				TEXT("stance on their next state tick, so releasing after latching would let a group about to be ")
-				TEXT("destroyed re-assert its station for one tick.\n\n")
+				TEXT("Immediate, army-wide, and it releases every standing group order. It runs as one fixed ")
+				TEXT("sequence, shared with the assistant's charge: abort any pick you are part-way through → clear all unit ")
+				TEXT("groups → lock in the stance.\n\n")
+				TEXT("The release comes BEFORE the stance is locked in, and that order matters — units check the ")
+				TEXT("stance again at their next decision, so releasing afterwards would let a group about to be ")
+				TEXT("destroyed go back to its station for one more decision.\n\n")
@@ -964,2 +1020,2 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-				TEXT("The stance is latched — it persists until replaced — and the game records your first command ")
-				TEXT("and keeps that record for the rest of the match, so the pre-command legacy behaviour never ")
+				TEXT("The stance is locked in — it stays until replaced — and the game records your first command ")
+				TEXT("and keeps that record for the rest of the match, so the old behaviour from before your first command never ")
@@ -968,0 +1025,7 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
+			// TASK-1574 (2026-09-28, the register pass): player words replace the developer wording this
+			// prose used to print — "The sequence is one function" (ApplyArmyWideStance, shared with the
+			// assistant's charge), "in-flight pick", "latch the stance" / "the latch" / "latched", "that
+			// ordering is load-bearing", "re-read the stance on their next state tick", "re-assert its
+			// station for one tick" and "the pre-command legacy behaviour" (the behaviour before
+			// bHasIssuedCommand flips). "Decision" is the word the Defend page already uses for the
+			// unit's periodic state update. The claims are unchanged.
@@ -986,2 +1049,2 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-				TEXT("The exact mirror of Attack — same guard, same two calls, same order, same final latch, ")
-				TEXT("through the same one implementation. Units fall back toward your own castle and engage only ")
+				TEXT("The exact mirror of Attack — same check, same two steps, same order, same stance locked in at the end, ")
+				TEXT("all done by the very same sequence. Units fall back toward your own castle and engage only ")
@@ -990,2 +1053,2 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-				TEXT("is the band past the castle's wall face, and the acquisition radius is derived at every ")
-				TEXT("decision from the castle's live colliding half-width plus that band, by ")
+				TEXT("is the band past the castle's wall face, and how far out a unit will pick a target is worked out at every ")
+				TEXT("decision from how far the castle's walls actually reach from its centre, plus that band, by ")
@@ -995,0 +1059,6 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
+			// TASK-1574 (2026-09-28, the register pass): player words replace the developer wording this
+			// prose used to print — "same guard, same two calls, … same final latch, through the same one
+			// implementation" (the match-end guard, the abort-pick and clear-groups calls and the latch in
+			// ApplyArmyWideStance), "the acquisition radius is derived" and "the castle's live colliding
+			// half-width" (the castle's collision half-width, read live, in
+			// ResolveDefendEngagementRadius). The claims are unchanged.
@@ -1016 +1085 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-				TEXT("ladder every state tick: enemies in the attack zone first, else enemies in the position zone, ")
+				TEXT("ladder at every decision: enemies in the attack zone first, else enemies in the position zone, ")
@@ -1019,2 +1088,2 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-				TEXT("zones is dropped that tick — the unit disengages and returns toward its station. A dead ")
-				TEXT("target is dropped for both types.\n\n")
+				TEXT("zones is dropped at that same decision — the unit disengages and returns toward its station. A dead ")
+				TEXT("target is dropped for both orders.\n\n")
@@ -1022,2 +1091,2 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-				TEXT("a live, zone-valid target is kept and re-acquisition runs only when target-less, which is ")
-				TEXT("what stops the goal flipping every tick. One monotone upgrade exists, HOLD only: a ")
+				TEXT("a live target still inside the zones is kept, and a new target is looked for only when the unit has none, which is ")
+				TEXT("what stops the goal flipping back and forth. A single one-way upgrade exists, HOLD only: a ")
@@ -1025 +1094 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-				TEXT("other way, so the two tiers cannot oscillate. Stations are spread by a sunflower offset so ")
+				TEXT("other way, so the two tiers cannot keep swapping. Stations are spread out in a spiral so ")
@@ -1030,0 +1100,6 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
+			// TASK-1574 (2026-09-28, the register pass): player words replace the developer wording this
+			// prose used to print — "every state tick" / "that tick" / "every tick" (the unit's periodic
+			// state update, called a "decision" as on the Defend page), "for both types" (the Hold and
+			// Ambush group types), "a live, zone-valid target", "re-acquisition runs only when
+			// target-less", "One monotone upgrade", "oscillate" and "a sunflower offset" (the golden-angle
+			// station spread cited above). The claims are unchanged, and the {…} tokens are untouched.
@@ -1062,5 +1137,5 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-				TEXT("The difference: AMBUSH skips the zone drop-test entirely while a live target exists. It keeps ")
-				TEXT("the target until the kill, then the ladder resumes. Ambush acquires through exactly the same ")
-				TEXT("two tiers; the exemption governs only when an already-held target is released.\n\n")
-				TEXT("A dead target is still dropped, for both types. The single monotone position→attack upgrade ")
-				TEXT("is HOLD-only and does not run for Ambush.\n\n")
+				TEXT("The difference: while it has a live target, AMBUSH never drops it for leaving the zones. It keeps ")
+				TEXT("the target until the kill, then the ladder resumes. Ambush picks its targets through exactly the same ")
+				TEXT("two tiers; that exception only affects when a target it already has is let go.\n\n")
+				TEXT("A dead target is still dropped, for both orders. The single one-way position→attack upgrade ")
+				TEXT("is HOLD-only and does not happen for Ambush.\n\n")
@@ -1070,0 +1146,7 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
+			// TASK-1574 (2026-09-28, the register pass): player words replace the developer wording this
+			// prose used to print — "skips the zone drop-test entirely while a live target exists" (the
+			// T5 repair quoted above now reads "while it has a live target, AMBUSH never drops it for
+			// leaving the zones"; the drop-test is the Hold-gated block cited above), "acquires",
+			// "the exemption governs only when an already-held target is released", "for both types",
+			// "monotone" and "does not run". The claims are unchanged, and the {…} tokens that test 10
+			// derives through are untouched.
@@ -1103,2 +1185,2 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-				TEXT("the pick enters at Select and confirms there, and the later stages are structurally ")
-				TEXT("unreachable. Jonathan's reason: \"There is only one mouse scroll circle used for this, and it ")
+				TEXT("the pick opens at Select and confirms there, and the later stages can never be ")
+				TEXT("reached. Jonathan's reason: \"There is only one mouse scroll circle used for this, and it ")
@@ -1107,6 +1189,6 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-				TEXT("The anchor is you, not a piece of ground: a follow group carries zero radii, zero centres ")
-				TEXT("and no marker decals, and the select circle is destroyed at confirm rather than left on the ")
-				TEXT("map. The station is your live position plus that unit's own sunflower offset, resolved every ")
-				TEXT("tick and never cached — which is exactly what makes hero respawn work for free.\n\n")
-				TEXT("Followers never attack. The target is forced null every tick and the follow body calls none ")
-				TEXT("of the acquire/attack functions. A following Cleric still heals — healing is not attacking, ")
+				TEXT("The anchor is you, not a piece of ground: a follow group stores no circle sizes, no centre points ")
+				TEXT("and no markers on the ground, and the select circle is destroyed at confirm rather than left on the ")
+				TEXT("map. Each unit's station is wherever you are right now plus that unit's own spot in the spiral, worked out fresh ")
+				TEXT("moment to moment and never stored — which is exactly why following carries on by itself after your hero respawns.\n\n")
+				TEXT("Followers never attack. Their target is cleared constantly, and following never uses any ")
+				TEXT("of the target-finding or attacking steps. A following Cleric still heals — healing is not attacking, ")
@@ -1115 +1197 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-				TEXT("a live pawn exists again, including a brand-new one after respawn.\n\n")
+				TEXT("you have a living hero again, including a brand-new one after respawn.\n\n")
@@ -1120,2 +1202,2 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-				TEXT("THE SPAWN DEFAULT: every follow-eligible Blue unit spawns already following you, on every ")
-				TEXT("spawn path, and nothing player-side auto-engages any more — you personally order every ")
+				TEXT("THE SPAWN DEFAULT: every follow-eligible Blue unit spawns already following you, whichever ")
+				TEXT("way it spawns, and nothing on your side starts a fight by itself any more — you personally order every ")
@@ -1123 +1205 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-				TEXT("eligibility predicate excludes them. Enrolment is unconditional: a unit spawned after you ")
+				TEXT("rule for who may follow leaves them out. Enrolment is unconditional: a unit spawned after you ")
@@ -1125,0 +1208,9 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
+			// TASK-1574 (2026-09-28, the register pass): player words replace the developer wording this
+			// prose used to print — "enters at Select", "structurally unreachable", "carries zero radii,
+			// zero centres and no marker decals", "your live position plus that unit's own sunflower
+			// offset, resolved every tick and never cached", "makes hero respawn work for free", "The
+			// target is forced null every tick and the follow body calls none of the acquire/attack
+			// functions", "a live pawn exists again", "on every spawn path", "nothing player-side
+			// auto-engages" and "the eligibility predicate" (ASummonedUnit::IsFollowCommandEligible, the
+			// gate TryAutoEnrollInFollowGroup asks). The claims are unchanged, and the {…} tokens are
+			// untouched.
@@ -1176 +1267 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-				TEXT("Polled directly every frame while a pick is live, not bound to an input action. Your hero ")
+				TEXT("Read straight from the mouse button every frame while a pick is open, rather than through the game's control setup. Your hero ")
@@ -1180 +1271 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-				TEXT("(2D) joins the group. An empty circle is refused and you STAY in the stage — a different ")
+				TEXT("(measured flat, ignoring height) joins the group. An empty circle is refused and you STAY in the stage — a different ")
@@ -1182 +1273 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-				TEXT("differs by order: Hold and Ambush use the narrower zone-order predicate, while Follow is ")
+				TEXT("differs by order: Hold and Ambush use the narrower rule for zone orders, while Follow is ")
@@ -1184 +1275 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-				TEXT("(Ogre/Sapper) and the entire enemy side. This circle is a transient pick visual: it is ")
+				TEXT("(Ogre/Sapper) and the entire enemy side. This circle is only there while you pick: it is ")
@@ -1192 +1283 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-				TEXT("The circle you are drawing traces to the surface under the cursor — flat floor, hill crown ")
+				TEXT("The circle you are drawing sits on the ground under the cursor — flat floor, hill crown ")
@@ -1194 +1285 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-				TEXT("confirm refuses on the same flag, so what you see is what the click does.\n\n")
+				TEXT("confirm refuses in exactly the same case, so what you see is what the click does.\n\n")
@@ -1197 +1288,8 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-				TEXT("members that died mid-flow are dropped.")));
+				TEXT("members that died part-way through are dropped.")));
+			// TASK-1574 (2026-09-28, the register pass): player words replace the developer wording this
+			// prose used to print — "Polled directly every frame while a pick is live, not bound to an
+			// input action" (the per-frame poll cited above), "(2D)" (the membership test is
+			// FVector::DistSquared2D against the select radius, in the controller's Select-stage confirm
+			// branch), "the narrower zone-order predicate" (IsGroupCommandEligible), "a transient pick
+			// visual", "traces to the surface", "refuses on the same flag" and "mid-flow". No digit was
+			// added: "(2D)" was removed and the stage labels are byte-identical. The claims are unchanged.
@@ -1224,2 +1322,2 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-				TEXT("circle never touches an earlier one. The decal resizes in place as you scroll.\n\n")
-				TEXT("The wheel is polled, not bound, and it is verified globally unbound elsewhere — it is inert ")
+				TEXT("circle never touches an earlier one. The circle on the ground resizes in place as you scroll.\n\n")
+				TEXT("The game reads the wheel directly rather than through its control setup, and it was checked that no control anywhere else is set to the wheel — it is inert ")
@@ -1227,2 +1325,11 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-				TEXT("If the circle material is missing the radius still changes and the confirm still uses it — ")
-				TEXT("you just get no visual.")));
+				TEXT("If the circle's graphics are missing the radius still changes and the confirm still uses it — ")
+				TEXT("you just cannot see the circle.")));
+			// TASK-1574 (2026-09-28, the register pass): player words replace the developer wording this
+			// prose used to print — "The decal", "The wheel is polled, not bound, and it is verified
+			// globally unbound elsewhere", "the circle material is missing" and "you just get no visual".
+			// ⚠️ SC-§101, REPORTED AND NOT FIXED HERE: "it is inert everywhere except inside a pick" is
+			// kept byte-identical, and it is FALSE at source since TASK-823. The wheel also resizes a
+			// building's footprint in placement (ApplyPlacementFootprintWheel, the Cards.PlacementResize
+			// page) and a map circle on the war map (UWarMapWidget::NativeOnMouseWheel, the
+			// Interface.MapMarks page), and the Cards.PlacementResize page says so ("the game has three
+			// different ones"). A register pass keeps a false claim; it does not repair it.
@@ -1255,2 +1362,2 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-				TEXT("Polled every frame at the top of the pick branch, before anything else runs, and also ")
-				TEXT("reachable through the bound cancel action — the deliberate double-cover. Cancelling leaves ")
+				TEXT("Checked every frame, first, before anything else in the pick happens, and also ")
+				TEXT("reachable through the regular cancel control — a deliberate backup. Cancelling leaves ")
@@ -1258,4 +1365,4 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-				TEXT("The teardown is one function and every exit funnels through it — the final confirm, either ")
-				TEXT("cancel route, {Orders.Attack} or {Orders.Defend}, match end, hero death, unpossess and match ")
-				TEXT("reset. It releases the melee suppression before any early-out, clears the stage and all pick ")
-				TEXT("scratch, destroys every circle the flow still owns — a cancel at any stage kills all live ")
+				TEXT("One clean-up step handles every way out of a pick — the final confirm, either ")
+				TEXT("cancel route, {Orders.Attack} or {Orders.Defend}, match end, hero death, losing control of your hero and match ")
+				TEXT("reset. It lets your hero swing again before anything can cut the clean-up short, clears the stage and everything the pick ")
+				TEXT("was holding, destroys every circle the pick still has — a cancel at any stage kills all live ")
@@ -1264,3 +1371,11 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-				TEXT("free-look unless the interface-cursor key is still held, because the cursor owners compose.\n\n")
-				TEXT("Re-pressing the same order key mid-flow does nothing — it is a silent ignore; right-click or ")
-				TEXT("Escape is the cancel surface.")));
+				TEXT("free-look unless the interface-cursor key is still held, because anything that still needs the cursor keeps it.\n\n")
+				TEXT("Re-pressing the same order key part-way through does nothing — it is quietly ignored; right-click or ")
+				TEXT("Escape is how you cancel.")));
+			// TASK-1574 (2026-09-28, the register pass): player words replace the developer wording this
+			// prose used to print — "Polled every frame at the top of the pick branch", "the bound cancel
+			// action", "the deliberate double-cover", "The teardown is one function and every exit
+			// funnels through it" (CancelGroupPick, cited above), "unpossess" (OnUnPossess calls it),
+			// "releases the melee suppression before any early-out", "all pick scratch", "the flow still
+			// owns", "the cursor owners compose" (bWantCursor ORs every cursor owner), "mid-flow", "a
+			// silent ignore" and "the cancel surface". The claims are unchanged, and the {…} tokens are
+			// untouched.
@@ -1300,3 +1415,3 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-				TEXT("deliberately un-gated, because a close that can be refused is a close that can strand your ")
-				TEXT("cursor. Opening is gated: it is refused while placement, spell targeting or a group pick owns ")
-				TEXT("the cursor, or after match end, and nothing is created or shown until the posture is granted ")
+				TEXT("deliberately never blocked, because a close that can be refused is a close that can strand your ")
+				TEXT("cursor. Opening has conditions: it is refused while placement, spell targeting or a group pick is using ")
+				TEXT("the cursor, or after match end, and nothing is created or shown until the cursor is handed over ")
@@ -1304 +1419 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-				TEXT("It works anywhere. Unlike the war map there is no proximity check, no NPC reference and no ")
+				TEXT("It works anywhere. Unlike the war map there is no proximity check, no commander to look for and no ")
@@ -1306 +1421 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-				TEXT("Sending: type and press Enter — only a genuine Enter commits; moving focus away or clearing ")
+				TEXT("Sending: type and press Enter — only a genuine Enter sends it; clicking away or clearing ")
@@ -1313,2 +1428,2 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-				TEXT("Escape does NOT close the chat box, permanently. It is left unabsorbed so the shipped ")
-				TEXT("placement, targeting and group-pick cancel routes keep firing byte-identically while the box ")
+				TEXT("Escape does NOT close the chat box, permanently. The box deliberately lets Escape through so the ")
+				TEXT("placement, targeting and group-pick cancels keep working exactly as usual while the box ")
@@ -1316,3 +1431,3 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-				TEXT("A close is not a cancel: closing the window broadcasts no cancellation — only the ")
-				TEXT("assistant's own state machine may turn one into the other.\n\n")
-				TEXT("The console never sets the input mode itself; the controller owns that in one place. A ")
+				TEXT("A close is not a cancel: closing the window cancels nothing — only the ")
+				TEXT("assistant itself may turn one into the other.\n\n")
+				TEXT("The chat box never decides by itself who gets the mouse and keyboard; the game decides that in one place. A ")
@@ -1323,0 +1439,8 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
+			// TASK-1574 (2026-09-28, the register pass): player words replace the developer wording this
+			// prose used to print — "un-gated", "Opening is gated", "owns the cursor", "until the posture
+			// is granted", "no NPC reference" (the war map's commander lookup), "only a genuine Enter
+			// commits", "moving focus away", "left unabsorbed so the shipped … cancel routes keep firing
+			// byte-identically", "broadcasts no cancellation", "the assistant's own state machine" and
+			// "The console never sets the input mode itself; the controller owns that in one place"
+			// (ApplyCursorInputState, cited above). Jonathan's two quotations are byte-identical. The
+			// claims are unchanged.
@@ -1378,3 +1501,3 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-				TEXT("The key is caught in preview — it tunnels down the focus path from the root before the ")
-				TEXT("focused text box, which is why a plain key handler could never see a printable key the box ")
-				TEXT("already ate.\n\n")
+				TEXT("The key is caught on its way in, before the text box you are typing in gets it, ")
+				TEXT("which is why it works at all: anything that listened for it after the box would never see a letter ")
+				TEXT("the box had already typed.\n\n")
@@ -1400 +1523 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-				TEXT("undo and redo, and consuming them would both kill undo and execute an order you never asked ")
+				TEXT("undo and redo, and catching them would both kill undo and execute an order you never asked ")
@@ -1402 +1525 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-				TEXT("The grab is as narrow as it can be: it fires only while the box is open, enabled, and a ")
+				TEXT("Catching the key is kept as narrow as it can be: it happens only while the box is open, switched on, and a ")
@@ -1405,2 +1528,10 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-				TEXT("On a non-QWERTY layout the game listens at the QWERTY-Z PHYSICAL POSITION — the comparison ")
-				TEXT("resolves through the layout subsystem.")));
+				TEXT("On a non-QWERTY layout the game listens at the QWERTY-Z PHYSICAL POSITION — it works out which key ")
+				TEXT("sits there from your keyboard layout.")));
+			// TASK-1574 (2026-09-28, the register pass): player words replace the developer wording this
+			// prose used to print — "caught in preview — it tunnels down the focus path from the root
+			// before the focused text box, which is why a plain key handler could never see a printable
+			// key the box already ate" (the NativeOnPreviewKeyDown catch cited above), "consuming them",
+			// "The grab … fires only while the box is open, enabled", and "the comparison resolves
+			// through the layout subsystem" (USiegeKeyboardLayoutSubsystem). Every Z mention, Jonathan's
+			// quotation and the status-line quotation are byte-identical, so the one sanctioned letter
+			// literal and the Ctrl carve-out above both still hold. The claims are unchanged.
@@ -1453,5 +1584,5 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-				TEXT("A toggle, and close is asked first and never gated, for the same reason as the chat box.\n\n")
-				TEXT("THE PROXIMITY GATE — and it is proximity to YOUR OWN commander. The team is resolved from ")
-				TEXT("your player state and never guessed: with no player state there is no honest answer and no ")
-				TEXT("commander is returned, because a wrong default on the wrong side would gate the map on the ")
-				TEXT("enemy's commander and price the reveal off the wrong actor. The distance test and its radius ")
+				TEXT("A toggle, and closing is checked first and never blocked, for the same reason as the chat box.\n\n")
+				TEXT("THE PROXIMITY GATE — and it is proximity to YOUR OWN commander. Your team is read from ")
+				TEXT("your own player record and never guessed: with no such record there is no honest answer and no ")
+				TEXT("commander is picked, because a wrong guess on the wrong side would tie the map to the ")
+				TEXT("enemy's commander and take the reveal's price from the wrong one. The distance test and its radius ")
@@ -1459,2 +1590,2 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-				TEXT("the controller re-implements neither.\n\n")
-				TEXT("The gate is checked BEFORE the cursor posture is touched, deliberately, so an out-of-range ")
+				TEXT("the map key asks him rather than keeping its own copy of either.\n\n")
+				TEXT("The gate is checked BEFORE anything about the cursor changes, deliberately, so an out-of-range ")
@@ -1464,3 +1595,3 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-				TEXT("Nothing appears until the posture is granted, and if the widget then fails to create or ")
-				TEXT("fails to report itself open, the posture is rolled back rather than left as a cursor owner ")
-				TEXT("with no UI.\n\n")
+				TEXT("Nothing appears until the cursor is handed over, and if the map then fails to appear or ")
+				TEXT("fails to report itself open, the cursor is handed back rather than left claimed by a map ")
+				TEXT("that is not there.\n\n")
@@ -1469 +1600 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-				TEXT("polled every frame; that poll exists because a marker click opens the chat box, whose focused ")
+				TEXT("checked directly every frame; that check exists because a marker click opens the chat box, whose focused ")
@@ -1472,0 +1604,8 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
+			// TASK-1574 (2026-09-28, the register pass): player words replace the developer wording this
+			// prose used to print — "never gated", "The team is resolved from your player state" (the
+			// APlayerState read cited above), "no commander is returned", "a wrong default", "gate the
+			// map on", "price the reveal off the wrong actor", "the controller re-implements neither",
+			// "the cursor posture is touched", "the posture is granted", "the widget then fails to
+			// create", "the posture is rolled back rather than left as a cursor owner with no UI" and
+			// "polled every frame; that poll". Jonathan's quotation is byte-identical, and no refuted
+			// mouse-button wording entered the page (test 17's fragment scan). The claims are unchanged.
@@ -1516,2 +1655,2 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-				TEXT("enemy locations\". It is a mechanic rule, so it is a property default and never a card-table ")
-				TEXT("column, and the commander class only holds the number: it never reads a balance and never ")
+				TEXT("enemy locations\". It is a rule of the game, so it is a fixed setting on the commander and never a line in the card ")
+				TEXT("list, and the commander only holds the number: he never looks at anyone's gold and never ")
@@ -1519 +1658 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-				TEXT("re-typed; with no commander the purchase fails closed rather than inventing a fallback ")
+				TEXT("copied anywhere else; with no commander the purchase is simply refused rather than inventing a fallback ")
@@ -1522,2 +1661,2 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-				TEXT("produces the HUD message and again on the authority. Past the spend there is deliberately no ")
-				TEXT("early-out, so a partial spend is impossible — even an empty survey is a legitimate paid-for ")
+				TEXT("produces the HUD message and again where the purchase is actually settled. Past the spend there is deliberately no ")
+				TEXT("way to stop half-way, so a partial spend is impossible — even an empty survey is a legitimate paid-for ")
@@ -1526,0 +1666,6 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
+			// TASK-1574 (2026-09-28, the register pass): player words replace the developer wording this
+			// prose used to print — "a mechanic rule, so it is a property default and never a card-table
+			// column" (ACommanderNpc::EnemyRevealCost, a UPROPERTY default, not a DT_Cards column), "the
+			// commander class", "it never reads a balance", "never re-typed", "fails closed", "on the
+			// authority" (the server-side re-check cited above) and "no early-out". The quoted 30 gold
+			// and both quotations are byte-identical; no digit was added. The claims are unchanged.
@@ -1541,3 +1686,3 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-				TEXT("Clicking a marker opens the chat box first, through the proper open path, then appends the ")
-				TEXT("symbol. That order is pinned: the append never opens the box and never submits, and opening ")
-				TEXT("clears the input field on every open — so appending first and opening second would silently ")
+				TEXT("Clicking a marker opens the chat box first, the normal way, then adds the ")
+				TEXT("place's name. That order is fixed: adding the name never opens the box and never sends, and opening ")
+				TEXT("clears the input field every time — so adding first and opening second would silently ")
@@ -1545,4 +1690,4 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-				TEXT("The symbol is moved opaquely — the controller never spells it and must not learn which ")
-				TEXT("symbols exist, because a validation branch there would be a second, drifting copy of a ")
-				TEXT("vocabulary it does not own. The append's return value is checked; a refused insert is logged ")
-				TEXT("and inserts nothing rather than mis-delivering.\n\n")
+				TEXT("The name is passed along exactly as the map gave it — nothing on the way checks it against a ")
+				TEXT("list of its own, because a second list there would drift out of step with the real one, ")
+				TEXT("which lives elsewhere. Whether the name actually went in is checked; a refused insert is written to the game's log ")
+				TEXT("and adds nothing rather than delivering the wrong thing.\n\n")
@@ -1550,0 +1696,8 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
+			// TASK-1574 (2026-09-28, the register pass): player words replace the developer wording this
+			// prose used to print — "through the proper open path", "appends the symbol", "That order is
+			// pinned", "the append", "on every open", "The symbol is moved opaquely — the controller never
+			// spells it and must not learn which symbols exist, because a validation branch there would
+			// be a second, drifting copy of a vocabulary it does not own", "The append's return value is
+			// checked; a refused insert is logged" and "mis-delivering". "The place's name" is the
+			// one-liner's own word for the symbol. The closing ruling is a quotation and is
+			// byte-identical, "symbol" included. The claims are unchanged.
@@ -1621 +1774 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-				TEXT("your commander: it carries a number, that number is published to him as a place, and an order ")
+				TEXT("your commander: it carries a number, that number is handed to him as a place, and an order ")
@@ -1647,0 +1801,4 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
+			// TASK-1574 (2026-09-28, the register pass): "handed to him as a place" replaces "published to
+			// him as a place" (USiegeAssistantSnapshot publishing FSiegeMapMark::MakeSymbol into the place
+			// list, cited above). The claim is unchanged, no number is typed, and the "right-click" that
+			// test 17 uses as its positive control is untouched.
@@ -1685,2 +1842,2 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-				TEXT("A toggle. It documents its own key: {Interface.ControlsHelp} is a positional key like any ")
-				TEXT("other, so on a layout that moves it this row re-derives with everything else.\n\n")
+				TEXT("A toggle. It documents its own key: {Interface.ControlsHelp} is tied to its spot on the keyboard like any ")
+				TEXT("other key, so on a layout that moves it this entry updates with everything else.\n\n")
@@ -1689 +1846 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-				TEXT("read-only on the world: opening it issues no order, cancels no group, plays no card and ")
+				TEXT("for reading only and changes nothing in the game: opening it issues no order, cancels no group, plays no card and ")
@@ -1692,3 +1849,3 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-				TEXT("list — it never claims Escape. Every shipped cancel route keeps firing while it is open.\n\n")
-				TEXT("Its cursor is handled in the one place the game decides who owns the cursor, and nowhere else — the ")
-				TEXT("existing owners keep their exact shipped precedence.")));
+				TEXT("list — it never takes Escape for itself. Every normal cancel keeps working while it is open.\n\n")
+				TEXT("Its cursor is handled in the one place the game decides who gets the cursor, and nowhere else — ")
+				TEXT("everything else that uses the cursor keeps exactly the priority it already had.")));
@@ -1696,0 +1854,6 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
+			// TASK-1574 (2026-09-28, the register pass): player words replace the developer wording this
+			// prose used to print — "a positional key" (a Lane A key, re-derived through the layout
+			// subsystem), "this row re-derives", "read-only on the world", "it never claims Escape",
+			// "Every shipped cancel route keeps firing", "who owns the cursor" and "the existing owners
+			// keep their exact shipped precedence" (the other terms of bWantCursor, cited above). The
+			// claims are unchanged, and both {…} tokens are untouched.
diff --git a/GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/SiegeControlsHelpWidget.h b/GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/SiegeControlsHelpWidget.h
index 488df9d..016de72 100644
--- a/GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/SiegeControlsHelpWidget.h
+++ b/GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/SiegeControlsHelpWidget.h
@@ -141 +141,6 @@ struct FSiegeControlsHelpAction
-	/** The one-line description shown on the row (TASK-704 §4, verbatim). */
+	/**
+	 *  The one-line description shown on the row. For the original row set it is TASK-704 §4's
+	 *  text verbatim, except Cards.Discard's, which TASK-821 rewrote in place; rows 25-27
+	 *  (Cards.StackUpgrade, Cards.PlacementResize, Interface.MapMarks) are TASK-823's own prose,
+	 *  authored at source.
+	 */
@@ -146,2 +151,6 @@ struct FSiegeControlsHelpAction
-	 *  ⭐ THE FULL-SCREEN DETAIL PROSE. FILLED BY TASK-707 from
-	 *  handoffs/TASK-704-programmer.md §4 — ⛔ transferred, ⛔ never re-authored. §4's
+	 *  ⭐ THE FULL-SCREEN DETAIL PROSE. TASK-707 filled each original row's detail string from
+	 *  handoffs/TASK-704-programmer.md §4's prose for that row. Not every string is still 704's
+	 *  words: TASK-823 wrote its three appended rows at source, and later rows rewrote others,
+	 *  among them TASK-821 (Cards.Discard, in place), TASK-870 (Interface.WarMap) and TASK-1541
+	 *  (2026-09-27: the code names the prose printed, put into plain words); ⛔ TASK-707's
+	 *  transfer re-authored, re-derived and invented nothing. §4's
diff --git a/GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeControlsHelpTest.cpp b/GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeControlsHelpTest.cpp
index ac3d292..52cd50a 100644
--- a/GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeControlsHelpTest.cpp
+++ b/GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeControlsHelpTest.cpp
@@ -1090,2 +1090,4 @@ bool FSiegeControlsHelpChipTest::RunTest(const FString& Parameters)
- *  human would otherwise have to eyeball across 24 strings: ⛔ no `file:line` citation and ⛔ no
- *  markdown markup may reach the player's screen (rules T1 and T2). Those live in C++ comments.
+ *  human would otherwise have to eyeball across 24 strings: ⛔ no `file:line` citation, ⛔ no
+ *  markdown markup and ⛔ no C++ fragment (`::`, `()`) may reach the player's screen (rules T1
+ *  and T2, and rule T5's "a C++ fragment" in the widget's T-rule list). Those live in C++
+ *  comments.
@@ -1109,2 +1111,3 @@ bool FSiegeControlsHelpAuthoredDetailTest::RunTest(const FString& Parameters)
-	// these belongs in a C++ comment beside the string (TASK-704 §4's citations, rule T1) or is
-	// markdown that only means something in a .md file (rule T2).
+	// these belongs in a C++ comment beside the string (TASK-704 §4's citations, rule T1; `::` and
+	// `()`, which are rule T5's "a C++ fragment" in the widget's T-rule list) or is markdown that
+	// only means something in a .md file (rule T2).
@@ -1138 +1141 @@ bool FSiegeControlsHelpAuthoredDetailTest::RunTest(const FString& Parameters)
-			TestFalse(*FString::Printf(TEXT("Row '%s' detail carries no developer-only fragment '%s' (T1/T2: citations and markup live in comments)"),
+			TestFalse(*FString::Printf(TEXT("Row '%s' detail carries no developer-only fragment '%s' (T1/T2/T5: citations, markup and C++ fragments live in comments)"),
```

## 9. False claims found (`SC-§101`) — kept, reported, NOT fixed

1. **`Cards.StackUpgrade`, the "ONE KIND OF BUILDING REFUSES TO BE STACKED, AND IT IS THE ONE YOU CAN CLIMB" paragraph (through "Hovering one shows RED with "That building cannot be stacked"").** FALSE at source. `AClimbableTower` answers `CanStackHeight()` **true** (its doc: "THE HEIGHT (Z) ANSWER — **TRUE**, AND IT IS A ⛔ MEASUREMENT"), and the `NotStackable` state's doc in `SiegePlayerController.h` reads "⚠️⚠️ AS OF 2026-09-03 (STACK-§8/§10) ⛔ NO SHIPPED CLASS PRODUCES THIS STATE". `HELP-§2` mechanism 4 names this exact paragraph as occurrence (ii). **Its fix row, `TASK-973` [HELP-STACK], was boarded 2026-09-03 and never ran:** no `handoffs/TASK-973-programmer.md` and no `qa/TASK-974.md` exist, and its status line still reads "boarded 2026-09-03 — NOT dispatchable until TASK-944 has COMMITTED". This row rewrote only "the shape of the mesh" → "the building's exact shape" inside it and kept its claim; a comment beside the string now flags it. **For the manager: `TASK-973` looks orphaned.** Its spec carries a proceeding default (delete the false rule, add no number) and its own pin requirement. This matters for `TASK-1576` too: `Cards.StackUpgrade` is one of its five pages, and 5b will read this paragraph.
2. **`PickMode.Resize`, "it is inert everywhere except inside a pick".** FALSE as worded since `TASK-823`: the wheel also resizes a building's footprint in placement (`ASiegePlayerController::ApplyPlacementFootprintWheel`) and a map circle on the war map (`UWarMapWidget::NativeOnMouseWheel`). The `Cards.PlacementResize` page says so itself ("the game has three different ones"). Kept byte-identical as a phrase, flagged in a comment beside the string.
3. **`SiegeControlsHelpWidget.cpp`, the registry block's "⛔ THE ORIGINAL ROW SET AND THE LANE COLUMN ARE 704's. … every one-liner is its §4 text VERBATIM".** False for `Cards.Discard`: its one-liner is TASK-821's (measured, §5 N1), and its lane moved from PointerOnly to MappedAction (the R-09 block: "the PointerOnly lane … was falsehood 2 and it is GONE"). This is a file-level comment, not beside a string, so it is outside my fence. The `.h` N1 doc I wrote is scoped correctly.
4. **`Cards.Play`, "bound with the slot index as the payload"** (now "each key carries its own slot's number", same claim). It is true of keys 2–6 only. Key 1 is bound to `ASiegePlayerController::OnCard1Pressed` with no payload, and plays slot 0 itself ("key "1" (via IMC_Hero, TASK-009): hand slot 0, with the M1 Footman fallback"). Minor, and a player-invisible distinction; kept.
5. **`Tests/SiegeControlsHelpTest.cpp`, test 9's docstring "across 24 strings"**: the registry has 27 rows. It sits in the W1 (b) sentence but is not a W1 site, so it is byte-identical.

## 10. For 5b (`TASK-1579`)

- **Pages changed: 25** — every row except `Hero.Attack` and `Hero.Rally`. `TASK-1576` then changes its own pages on top of these bytes, and its handoff names them.
- **Every changed page's own text grew or shrank a little** (composed `Detail` total 27263 → 28281 chars, +1018). The biggest own-page growth is `Cards.Play` +100, `Orders.Follow` +92 and `PickMode.Resize` +90. **The largest composite page (own text plus its related blocks) is still `Cards.PlacementResize`: 6745 → 6847 chars (+102).** Then come `Cards.StackUpgrade` 5698 → 5873 and `Orders.Follow` 5155 → 5465 (+310, the biggest composite growth, because its three related PickMode pages also changed). `TASK-1494`'s "no page overflows" predates this. Those three pages are the ones to scroll to the bottom of.
- The new text of every changed literal is the NEW column of §2.

## What QA (`TASK-1575`) should scrutinize

1. **The census line (§1).** Where I drew the developer-register boundary, and the "considered, not flagged" list: change-history wording, design-rationale sentences in plain words, "focused text field", and game vocabulary.
2. **Same rule, new words (§2)**, especially the rewordings listed there with their source reads, and these judgment calls:
   - `Hero.Move`: "must never be replaced as one set" (kept as a prescription).
   - `Cards.CursorHold`: "the game's count of what is holding camera look off" (the ignore-look counter).
   - `Orders.Attack` / `Hold` / `Defend`: "decision" for "state tick".
   - `PickMode.Resize`: "it was checked that no control anywhere else is set to the wheel" (the "verified globally unbound elsewhere" claim, kept).
   - `Interface.WarMapMarker`: its rationale sentence, kept in plain words rather than moved to a comment.
   - `Interface.WarMapReveal`: "where the purchase is actually settled" (the authority).
3. **N1's deliberate departure from a pure mirror** (§5), with the measurement behind it.
4. **The re-anchored arm (§6):** the anchor is exactly once in `Source/`, the injection hits exactly `['::', '()']`, and the two expected lines carry the new label.
5. **The fence:** the `.cpp` and `.h` skeletons are identical, literal counts are unchanged, every `.cpp` comment op is an insert, and the test file changed one label and two comments only.
6. **§9 item 1 (`TASK-973`)** is the most consequential finding here, and it is outside this row.

## Not examined / limitations

- **Nothing was compiled or run.** "N stays 566", "the arm reds exactly one test with two errors" and every pin claim are byte-level and simulated. `TASK-1578` is where they are first measured.
- **The census is my judgment of register, over a declared class list** (§1). A reviewer may draw the line elsewhere. Every hit is fixed either way, and every fix is text.
- **Truth at source was re-read only for the sentences I reworded where the plain version could drift** (§2's list and §9's findings). The remaining sentences' truth, and the numeric `file:line` citations in the `Citations (T1)` blocks, were not re-examined; `TASK-1480` (a) already declares those numbers unverified.
- **Chrome strings outside the registry** (the title, hints, `(not bound)`, `(undocumented — TODO)`, the headers, the related-controls header) are outside spec (1)'s "Detail and one-liner" scope. They were read but not edited. `(undocumented — TODO)` is pinned by law, and `(not bound)` is mildly technical.
- **Existing comments that paraphrase changed prose were not edited** (only inserted beside). Known instance: test 14 (e)'s comment "Cards.Play names key 1's legacy quirk" still holds as a paraphrase. The `Interface.AssistantAccept` block's line-number cross-references (":948-965", ":927-930", ":934-936") were stale before this row and were not examined.
- **Wrapping and overflow on the real `UTextBlock`** are 5b's to observe (§10).
- **The instruments are mine** (`SC-§92`). The lexer's match with `qa/TASK-1561.md`'s 610 literals and 27263 composed chars is a claimed agreement (I had read those figures first). One agreement I did not target: the `pins.py` presence diff found the `SiegePlacementTest.cpp` `"cannot"` side-match, which no rule of mine predicted. Beyond that, instrument fidelity is **uncorroborated**. The `SC-§71b` reading of my hashes and counts is QA's to take or re-measure.
