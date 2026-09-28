# TASK-1541 — gameplay-programmer handoff (HELP-PROSE-CODE-IDENTIFIERS, rescoped to the fix)

- **Row:** `TASKBOARD.md` `#### TASK-1541 ` (marker `TASK-1541-RESCOPED-TO-FIX-2026-09-27`). Scope = EVERY hit, per 🧑 his two relayed answers ("Yes, plain words (Recommended)", then "Fix every hit (Recommended)").
- **Status set:** `ready-for-qa` → gate `TASK-1546` → 5a folded into `TASK-1538` → 5b `TASK-1547` → host `TASK-1540` (commit G, together with `TASK-1480`).
- **Size valve:** not used. Every hit in every row was rewritten in this session; no row is half-done and no row is left untouched.
- **Summary:** 63 code-name hits on 46 player-facing lines in 19 of the 27 rows, all inside `Detail` strings. All 63 fixed. 50 literal lines changed: the 46 hit lines, plus 4 continuation lines rewrapped because a rewritten sentence ran onto them. **One test pin moved** (`SiegeControlsHelpTest.cpp:1818-1819`, `"DiscardAllCost"` → `"charged once for the whole hand"`), so `TASK-1538` owes a mutation arm for it.

## 0. Start state (spec (6)) and declared tooling (`SC-§71a`)

- **Blocker met:** `TASK-1481` PASS (`qa/TASK-1481-loop1.md`), as relayed in the dispatch.
- **Start bytes, measured at my first read, before any edit:** `Source/GitClaudeUnrealTest/Siegebound/SiegeControlsHelpWidget.cpp` sha256 `f407d0b4c1d1df474fe2c0303cbe2eed6cb9636fb33526ffc2afe308ade96137`, 273661 B, LF, 4439 lines. This **equals** `TASK-1480`'s after-sha256, so I built on the reviewed bytes.
- `Tests/SiegeControlsHelpTest.cpp` start: sha256 `9870e40fa10ca7f9e5c3380f582fc17cdc2e48d5cbbab579e4f3149dbcd025cc`, 150453 B, LF. It is unmodified vs `HEAD` in the session-start git status.
- **Scratch copies of the start bytes** (outside the repo; my diffs are taken against these, NOT against `HEAD`): `C:\Users\wesel\AppData\Local\Temp\claude\C--GitProjects-GitHub-GitClaudeUnrealTesting-GitClaudeUnrealTest\93b0d17f-722a-4ad5-9615-dc48a30aa715\scratchpad\SiegeControlsHelpWidget.start.cpp` and `...\SiegeControlsHelpTest.start.cpp`.
- **Read-only git, declared (all with `--no-optional-locks`):**
  - `git diff --stat HEAD -- <widget .cpp/.h, help test>`
  - `git diff -U0 HEAD -- SiegeControlsHelpWidget.cpp`, hunk headers only. I used it to locate `TASK-1480`'s hunks so I could stay off them. They are at start-copy lines 178-193, 212, 214, 217-222, 340-348, **413-434 (R-02 citations)**, **584-587 (R-08 citations)**, 4128 and 4132-4141.
  - `git diff --no-index -U0 <start copy> <file>`, once per touched file (§5 below).
  - One `git hash-object` and one `git rev-parse HEAD:<path>` probe, informational only.
- **Hashing:** `sha256sum`. **Lexing:** Python scripts in the scratchpad (`lex.py`, `census.py`, `verify.py`, `pins.py`). They read the repo and write only to the scratchpad.
- **Nothing else ran:** no compile, no PIE, no asset, no `CONVENTIONS.md`, no mutating git.
- **Line endings:** both files are LF with no BOM after the edits, checked by byte count (zero `\r` bytes).

## 1. The census (spec (1)–(3))

**Method.** A lexer read the start copy and tracked five states: code, `//` comment, `/* */` comment, `"…"` string, and char literal. Raw strings were handled too, but the file has none. Every string literal inside `FSiegeControlsHelpRegistry::GetActions()` in the **code** state was attributed to its row (27 `AddRow` calls) and to its field (`DisplayName`, `OneLine` or `Detail`). Each literal was flagged by these token classes:

- `X::Y`
- `Name(…)`
- lowerCamel (e.g. `bDead`)
- UpperCamel with two or more humps
- an identifier containing an underscore
- `file:line`
- an `A`/`U`/`F`/`E`/`I` type prefix

I then did a manual sweep of mid-sentence capitalised words, to catch single-word code names the classes miss (`Started`, `Negate`, `Super`). The player-facing strings outside the registry were scanned too: the title, both hint strings, `Close`, `Back to the controls list`, the related-controls header, `Mouse click`, `(not bound)`, the key separator, `(undocumented — TODO)` and the five category headers. They have **zero hits**. The `[ControlsHelp]` `UE_LOG` strings and the widget object names (`RowButton`, `DetailScrollBox`, …) are excluded by name: they are not player-facing.

**Result: 63 hits on 46 lines, in 19 of 27 rows.**

- Every hit is inside a `Detail` string. One-liners, display names and chrome have zero.
- 55 hits are in the spec's classes. **8 are borderline tokens that I counted as code:** `Negate`, `Super`, `Started`, `Completed` ×2, `Canceled` ×2, and the bare argument list `(true, 0)`.
- **Lexer state of every hit: a player-facing literal**, i.e. code state, inside `TEXT("…")`, inside a `Row.Detail = FText::FromString(FString(…))` concatenation. None is in a comment.

**Excluded, with the reason:** `{Interface.WarMap}` (start line 1562), and `{Interface.ControlsHelp}` (1623, 1629). These are `{ActionId}` tokens, which `ResolveDetailTokens` replaces with the live key chip, so the player never sees the id text.

**The 8 rows with zero hits:** `Cards.Play`, `Cards.Cancel`, `Orders.Hold`, `Orders.Ambush`, `Orders.Follow`, `PickMode.Cancel`, `Interface.AssistantAccept`, `Interface.WarMapMarker`.

| # | Row | Start line | Hit tokens, quoted | State |
|---|---|---|---|---|
| 1 | R-01 `Hero.Move` | 399 | `Negate` (borderline), `SwizzleAxis` | literal |
| 2 | R-01 `Hero.Move` | 401 | `AHeroCharacter::WalkSpeed` | literal |
| 3 | R-01 `Hero.Move` | 402 | `GetEffectiveWalkSpeed()` | literal |
| 4 | R-02 `Hero.Look` | 436 | `Negate_2` | literal |
| 5 | R-02 `Hero.Look` | 437 | `OnUICursorPressed` | literal |
| 6 | R-02 `Hero.Look` | 438 | `SetIgnoreLookInput(true)` | literal |
| 7 | R-02 `Hero.Look` | 441 | `FInputModeGameAndUI`, `DoNotLock` | literal |
| 8 | R-03 `Hero.Jump` | 456 | `AHeroCharacter::FellOutOfWorld` | literal |
| 9 | R-03 `Hero.Jump` | 457 | `Super` (borderline), `Destroy()` | literal |
| 10 | R-04 `Hero.Sprint` | 475 | `Started`, `Completed`, `Canceled` (borderline; trigger-event names) | literal |
| 11 | R-04 `Hero.Sprint` | 476 | `SprintSpeed` | literal |
| 12 | R-04 `Hero.Sprint` | 477 | `WalkSpeed`, `GetEffectiveSprintSpeed()` | literal |
| 13 | R-04 `Hero.Sprint` | 478 | `GetEffectiveWalkSpeed()` | literal |
| 14 | R-04 `Hero.Sprint` | 479 | `StartSprint`, `bDead`, `StopSprint` | literal |
| 15 | R-04 `Hero.Sprint` | 482 | `DoMeleeAttack` | literal |
| 16 | R-05 `Hero.Attack` | 499 | `MeleeRange` | literal |
| 17 | R-05 `Hero.Attack` | 500 | `±MeleeHalfAngleDegrees`, `MeleeCooldown` | literal |
| 18 | R-05 `Hero.Attack` | 502 | `GetEffectiveMeleeDamage()` | literal |
| 19 | R-05 `Hero.Attack` | 505 | `SetMeleeSuppressed(true)`, `DoMeleeAttack` | literal |
| 20 | R-06 `Hero.Rally` | 519 | `ASummonedUnit`, `RallyRadius`, `RallySpeedBonus` | literal |
| 21 | R-06 `Hero.Rally` | 520 | `RallyDuration` | literal |
| 22 | R-06 `Hero.Rally` | 521 | `AMinerUnit` | literal |
| 23 | R-06 `Hero.Rally` | 522 | `OnRallyStateChanged(false, remaining)` | literal |
| 24 | R-06 `Hero.Rally` | 523 | `RallyCooldown`, `OnRallyReady` | literal |
| 25 | R-06 `Hero.Rally` | 524 | `(true, 0)` (borderline; bare argument list) | literal |
| 26 | R-08 `Cards.CursorHold` | 589 | `GameAndUI` | literal |
| 27 | R-08 `Cards.CursorHold` | 591 | `Completed`, `Canceled` (borderline) | literal |
| 28 | R-09 `Cards.Discard` | 665 | `DiscardAllCost` — ⛔ PINNED (§4) | literal |
| 29 | R-25 `Cards.StackUpgrade` | 792 | `MaxStackHeightMultiplier` | literal |
| 30 | R-25 `Cards.StackUpgrade` | 793 | `StackHealthStep` | literal |
| 31 | R-26 `Cards.PlacementResize` | 875 | `PlacementFootprintWheelStep`, `PlacementFootprintMin` | literal |
| 32 | R-26 `Cards.PlacementResize` | 876 | `PlacementFootprintMax` | literal |
| 33 | R-11 `Orders.Attack` | 922 | `bHasIssuedCommand` | literal |
| 34 | R-12 `Orders.Defend` | 945 | `DefendRadius` | literal |
| 35 | R-12 `Orders.Defend` | 948 | `ASummonedUnit::ResolveDefendEngagementRadius` | literal |
| 36 | R-16 `PickMode.Confirm` | 1130 | `GroupSelectRadiusDefault` | literal |
| 37 | R-16 `PickMode.Confirm` | 1137 | `GroupPositionRadiusDefault` | literal |
| 38 | R-16 `PickMode.Confirm` | 1140 | `GroupAttackRadiusDefault` | literal |
| 39 | R-17 `PickMode.Resize` | 1171 | `GroupRadiusWheelStep` | literal |
| 40 | R-17 `PickMode.Resize` | 1172 | `GroupRadiusMin`, `GroupRadiusMax` | literal |
| 41 | R-19 `Interface.AssistantConsole` | 1257 | `CancelPressed()` | literal |
| 42 | R-19 `Interface.AssistantConsole` | 1258 | `SetConsoleEnabled(false)` | literal |
| 43 | R-21 `Interface.WarMap` | 1403 | `ACommanderNpc::IsPlayerInRange`, `InteractRadius` | literal |
| 44 | R-22 `Interface.WarMapReveal` | 1454 | `EnemyRevealCost` | literal |
| 45 | R-27 `Interface.MapMarks` | 1581 | `MaxMapMarks` | literal |
| 46 | R-24 `Interface.ControlsHelp` | 1631 | `ApplyCursorInputState()` | literal |

**Pages touched (19):**

- Hero: move, look, jump, sprint, attack, rally
- Cards: cursor hold, discard, stack upgrade, placement resize
- Orders: attack, defend
- Pick mode: confirm, resize
- Interface: AI chat, war map, reveal, map marks, controls

**Left in place, not code names (not hits):**

- `LMB` in `Cards.Play` ("at LMB confirm"). It is gamer shorthand, not a code token by the spec's classes.
- `2D` in `PickMode.Confirm`'s "(2D)".
- `(undocumented — TODO)`. It is pinned by law (`HELP-§2` mechanism 2), and it only renders on a row with no text; no row is in that state.
- General developer register such as "state tick", "idempotent", "input asset", "mapping context", "polled … not bound", "ignore-look counter" and "trigger setup". These are ordinary words, so the row does not cover them. See the limitations section.

## 2. Every replacement, old beside new

This covers 50 literal lines: the 46 hit lines above, plus the 4 continuation lines 440, 480, 501 and 923. Those four carry no hit; they were rewritten only because the sentence being fixed ran onto them. Line numbers are start → after. The text is the bytes between the quotes, with `\n` and `\"` shown as in the source.

| # | Page (row id) | start line -> after line | OLD (bytes inside `TEXT("…")`) | NEW |
|---|---|---|---|---|
| 1 | R-01 `Hero.Move` Detail | 399 -> 399 | `The back and left rows carry Negate modifiers and the forward/back rows carry SwizzleAxis, ` | `The back and left rows carry a modifier that reverses the direction, and the forward and back rows carry one that turns the input onto the forward axis, ` |
| 2 | R-01 `Hero.Move` Detail | 401 -> 401 | `Base walk speed is AHeroCharacter::WalkSpeed; the speed actually applied composes the Swift Boots ` | `Your base walk speed is set on the hero; the speed you actually move at applies the Swift Boots ` |
| 3 | R-01 `Hero.Move` Detail | 402 -> 402 | `upgrade on top and is GetEffectiveWalkSpeed() — the base is never mutated.\n\n` | `upgrade on top of that base, and the base itself is never changed.\n\n` |
| 4 | R-02 `Hero.Look` Detail | 436 -> 439 | `Bound as a 2D mouse axis with a Negate_2 modifier on the Y channel. ` | `Bound to the mouse's up-down and left-right movement, with a modifier that reverses the up-down direction. ` |
| 5 | R-02 `Hero.Look` Detail | 437 -> 440 | `Look is suspended while you hold the interface-cursor key — OnUICursorPressed calls ` | `Look is suspended while you hold the interface-cursor key — pressing the key switches camera look off ` |
| 6 | R-02 `Hero.Look` Detail | 438 -> 441 | `SetIgnoreLookInput(true), paired 1:1 with its release, so a click-drag on the HUD cannot ` | `and its release switches it back on, one release for every press, so a click-drag on the HUD cannot ` |
| 7 | R-02 `Hero.Look` Detail | 440 -> 443 | `It is not suspended in placement, targeting or a group pick: those modes use ` | `It is not suspended in placement, targeting or a group pick: those modes let the game and the interface ` |
| 8 | R-02 `Hero.Look` Detail | 441 -> 444 | `FInputModeGameAndUI with DoNotLock, so the mouse steers the cursor while movement keys ` | `both take input without locking the mouse to the window, so the mouse steers the cursor while movement keys ` |
| 9 | R-03 `Hero.Jump` Detail | 456 -> 462 | `Falling out of the world is a death, not a despawn — AHeroCharacter::FellOutOfWorld ` | `Falling out of the world is a death, not a despawn — the hero deliberately skips the engine's ` |
| 10 | R-03 `Hero.Jump` Detail | 457 -> 463 | `deliberately does not call Super (which would Destroy() the pawn) and routes into the same ` | `default handling (which would delete your hero outright) and goes down the same ` |
| 11 | R-04 `Hero.Sprint` Detail | 475 -> 483 | `Bound on Started, Completed and Canceled, so the sprint can never stick on if the press is ` | `It listens for the press, the release and a cancelled press, so the sprint can never stick on if the press is ` |
| 12 | R-04 `Hero.Sprint` Detail | 476 -> 484 | `interrupted. Pressing raises the max walk speed to SprintSpeed and releasing returns it to ` | `interrupted. Pressing raises your top speed to your sprint speed and releasing returns it to ` |
| 13 | R-04 `Hero.Sprint` Detail | 477 -> 485 | `WalkSpeed; both compose the Swift Boots move-speed bonus live via GetEffectiveSprintSpeed() ` | `your walk speed; both include the Swift Boots move-speed bonus, worked out fresh each time, ` |
| 14 | R-04 `Hero.Sprint` Detail | 478 -> 486 | `and GetEffectiveWalkSpeed() rather than mutating the base.\n\n` | `rather than changing either base speed.\n\n` |
| 15 | R-04 `Hero.Sprint` Detail | 479 -> 487 | `A dead hero cannot start a sprint — StartSprint early-outs on bDead — but StopSprint is ` | `A dead hero cannot start a sprint — starting checks for death first — but stopping has no such ` |
| 16 | R-04 `Hero.Sprint` Detail | 480 -> 488 | `unguarded so the state always releases.\n\n` | `check, so the sprint always releases.\n\n` |
| 17 | R-04 `Hero.Sprint` Detail | 482 -> 490 | `another, and nothing in DoMeleeAttack reads the sprint flag.` | `another, and the melee swing never looks at whether you are sprinting.` |
| 18 | R-05 `Hero.Attack` Detail | 499 -> 514 | `One swing damages all enemy team agents within MeleeRange and inside a ` | `One swing damages everything on the enemy team within your melee reach and inside a ` |
| 19 | R-05 `Hero.Attack` Detail | 500 -> 515 | `±MeleeHalfAngleDegrees forward cone, rate-limited to one swing per MeleeCooldown seconds. ` | `cone in front of you, and you can swing at most once per melee cooldown. ` |
| 20 | R-05 `Hero.Attack` Detail | 501 -> 516 | `Damage per swing is composed live — base plus the Sharpened Blade stacks — through ` | `Damage per swing is worked out fresh each time — the base damage plus the Sharpened Blade ` |
| 21 | R-05 `Hero.Attack` Detail | 502 -> 517 | `GetEffectiveMeleeDamage(). No friendly fire.\n\n` | `stacks. No friendly fire.\n\n` |
| 22 | R-05 `Hero.Attack` Detail | 505 -> 520 | `SetMeleeSuppressed(true) makes DoMeleeAttack a no-op that does not even consume the cooldown.` | `the swing is skipped entirely and does not even use up the cooldown.` |
| 23 | R-06 `Hero.Rally` Detail | 519 -> 534 | `Buffs every same-team ASummonedUnit within RallyRadius by RallySpeedBonus for ` | `Speeds up every friendly summoned unit within the rally radius by the rally speed bonus for ` |
| 24 | R-06 `Hero.Rally` Detail | 520 -> 535 | `RallyDuration seconds — units only, never the hero, never enemy units. Friendly miners are ` | `the rally duration — units only, never the hero, never enemy units. Friendly miners are ` |
| 25 | R-06 `Hero.Rally` Detail | 521 -> 536 | `included, since AMinerUnit is a summoned-unit subclass.\n\n` | `included, since a miner is a kind of summoned unit.\n\n` |
| 26 | R-06 `Hero.Rally` Detail | 522 -> 537 | `On cooldown the press is a no-op but still broadcasts OnRallyStateChanged(false, remaining) ` | `On cooldown the press does nothing, but it still tells the HUD how much cooldown is left ` |
| 27 | R-06 `Hero.Rally` Detail | 523 -> 538 | `so the HUD can flash the time left; the cooldown length is RallyCooldown and OnRallyReady ` | `so the HUD can flash the time left; when the rally cooldown runs out, the HUD is told ` |
| 28 | R-06 `Hero.Rally` Detail | 524 -> 539 | `re-broadcasts (true, 0) when it elapses. A dead hero cannot rally.` | `that Rally is ready again. A dead hero cannot rally.` |
| 29 | R-08 `Cards.CursorHold` Detail | 589 -> 608 | `A hold, not a toggle. Holding puts the game in GameAndUI with the cursor visible and camera ` | `A hold, not a toggle. Holding lets the game and the HUD both take input, with the cursor visible and camera ` |
| 30 | R-08 `Cards.CursorHold` Detail | 591 -> 610 | `Bound on Completed and Canceled so the hold can never stick regardless of the action's ` | `The release fires on a normal release and on a cancelled press, so the hold can never stick regardless of the action's ` |
| 31 | R-09 `Cards.Discard` Detail | 665 -> 688 | `The fee is DiscardAllCost and it is charged once for the whole hand, flat. Dumping a ` | `The fee is a single set amount and it is charged once for the whole hand, flat. Dumping a ` |
| 32 | R-25 `Cards.StackUpgrade` Detail | 792 -> 815 | `height, and it stops at MaxStackHeightMultiplier times that original. Its width and length are ` | `height, and it stops at a set maximum multiple of that original. Its width and length are ` |
| 33 | R-25 `Cards.StackUpgrade` Detail | 793 -> 816 | `not touched. Health: each upgrade multiplies the building's maximum health by StackHealthStep, ` | `not touched. Health: each upgrade multiplies the building's maximum health by a set step, ` |
| 34 | R-26 `Cards.PlacementResize` Detail | 875 -> 898 | `One notch changes the size by PlacementFootprintWheelStep, between PlacementFootprintMin and ` | `One notch changes the size by a set step, and the size always stays between a set floor and a set ` |
| 35 | R-26 `Cards.PlacementResize` Detail | 876 -> 899 | `PlacementFootprintMax. It will not go below the floor: you can make a building bigger than it ` | `ceiling. It will not go below the floor: you can make a building bigger than it ` |
| 36 | R-11 `Orders.Attack` Detail | 922 -> 948 | `The stance is latched — it persists until replaced — and bHasIssuedCommand flips true on ` | `The stance is latched — it persists until replaced — and the game records your first command ` |
| 37 | R-11 `Orders.Attack` Detail | 923 -> 949 | `your first command and stays true for the match, so the pre-command legacy behaviour never ` | `and keeps that record for the rest of the match, so the pre-command legacy behaviour never ` |
| 38 | R-12 `Orders.Defend` Detail | 945 -> 973 | `What \"the band\" means CHANGED: DefendRadius is no longer a disc centred on the castle — it ` | `What \"the band\" means CHANGED: the defend range is no longer a disc centred on the castle — it ` |
| 39 | R-12 `Orders.Defend` Detail | 948 -> 976 | `ASummonedUnit::ResolveDefendEngagementRadius.` | `each unit for itself.` |
| 40 | R-16 `PickMode.Confirm` Detail | 1130 -> 1163 | `1. SELECT — opens at GroupSelectRadiusDefault. At confirm, every eligible unit inside it ` | `1. SELECT — opens at its own default size. At confirm, every eligible unit inside it ` |
| 41 | R-16 `PickMode.Confirm` Detail | 1137 -> 1170 | `2. POSITION — opens at GroupPositionRadiusDefault. This is the ground the squad stands on: ` | `2. POSITION — opens at its own default size. This is the ground the squad stands on: ` |
| 42 | R-16 `PickMode.Confirm` Detail | 1140 -> 1173 | `3. ATTACK — opens at GroupAttackRadiusDefault. This is the first-priority engage trigger: an ` | `3. ATTACK — opens at its own default size. This is the first-priority engage trigger: an ` |
| 43 | R-17 `PickMode.Resize` Detail | 1171 -> 1206 | `One notch changes the active circle's radius by GroupRadiusWheelStep, clamped between ` | `One notch changes the active circle's radius by a set step, and the radius always stays between ` |
| 44 | R-17 `PickMode.Resize` Detail | 1172 -> 1207 | `GroupRadiusMin and GroupRadiusMax. Each stage opens at its own default and resizing one ` | `a set smallest and largest size. Each stage opens at its own default and resizing one ` |
| 45 | R-19 `Interface.AssistantConsole` Detail | 1257 -> 1292 | `CLOSING — the complete list: (1) press the open key again; (2) CancelPressed() — public API ` | `CLOSING — the complete list: (1) press the open key again; (2) a spare close route the game ` |
| 46 | R-19 `Interface.AssistantConsole` Detail | 1258 -> 1293 | `with no caller today, kept deliberately; (3) the fault latch SetConsoleEnabled(false); ` | `keeps on purpose, though nothing uses it today; (3) the box switching itself off when the assistant faults; ` |
| 47 | R-21 `Interface.WarMap` Detail | 1403 -> 1442 | `both belong to the commander — ACommanderNpc::IsPlayerInRange reading InteractRadius — and ` | `both belong to the commander — he checks whether you are inside his own interaction range — and ` |
| 48 | R-22 `Interface.WarMapReveal` Detail | 1454 -> 1496 | `The price is EnemyRevealCost — Jonathan's own number: \"You can pay 30 gold to reveal all ` | `The price is a fixed reveal fee — Jonathan's own number: \"You can pay 30 gold to reveal all ` |
| 49 | R-27 `Interface.MapMarks` Detail | 1581 -> 1624 | `You can hold MaxMapMarks circles at once. At the limit a further click refuses out loud and ` | `You can hold a limited number of circles at once. At the limit a further click refuses out loud and ` |
| 50 | R-24 `Interface.ControlsHelp` Detail | 1631 -> 1674 | `Cursor posture is added to ApplyCursorInputState()'s one composition and nowhere else — the ` | `Its cursor is handled in the one place the game decides who owns the cursor, and nowhere else — the ` |

**Re-census of the after file:** the same lexer and classes, plus the borderline sweep, now find **0 hits**. The only flags left are the three `{ActionId}` tokens excluded above.

## 3. Comments beside the strings

These are allowed by the row: "If a code name is worth keeping, move it into the comment beside the string, cited by text".

- **Added one `// TASK-1541 (2026-09-27): …` note after the `Detail` statement in 12 rows.** It names, by symbol, the code names that prose used to print: R-01, R-02, R-03, R-04, R-06, R-08, R-26, R-11, R-12, R-19, R-21 and R-24. There are no line numbers in them (`CITE-BY-TEXT-RULED-2026-09-24`).
- **Amended the existing comment that became false the moment its string changed (`SC-§53`)**, in 7 rows:
  - R-05: "the three tunables stay NAMED"
  - R-09: "THE FEE IS NAMED AND NEVER TYPED". It now also records the moved pin.
  - R-25: "MaxStackHeightMultiplier and StackHealthStep are NAMED"
  - R-16: "the three defaults stay NAMED"
  - R-17: "ALL THREE TUNABLES STAY NAMED" and the F-3 jargon flag
  - R-22: "the sentence around the quote still names EnemyRevealCost"
  - R-27: "MaxMapMarks is NAMED"

  Each amendment keeps the rule the comment states (no number typed) and records the change.
- **No `TASK-1480` hunk was touched.** My nearest hunks start at start-lines 436 (R-02) and 589 (R-08). `TASK-1480`'s end at 434 and 587, and the untouched lines 435 and 588 (`Row.Detail = …`) sit between them.

## 4. Pins (`HELP-§2` mechanism 4) — listed BEFORE editing

**Method:** `pins.py` lexed `Tests/SiegeControlsHelpTest.cpp`, `Tests/SiegeCardHandKeyLabelTest.cpp` and `Tests/SiegeMenuInputTest.cpp`. It then reported every test literal of 6 or more characters that is a substring of any row's composed `OneLine`/`Detail`. After that, I read every prose assertion in `SiegeControlsHelpTest.cpp` by hand (every `.Contains(` and every fragment array).

**⛔ MOVED PIN (1): `Tests/SiegeControlsHelpTest.cpp:1818-1819`** in `FSiegeControlsHelpDiscardAllLayoutTest` (`Siegebound.ControlsHelp.DiscardAllLetterMovesWhileCardDigitsHold`, section (e)). It pinned the code name itself:

```
-	TestTrue(TEXT("⭐ The page NAMES its fee property instead of restating its value"),
-		DiscardDetail.Contains(TEXT("DiscardAllCost"), ESearchCase::CaseSensitive));
+	TestTrue(TEXT("⭐ The page states the flat-fee rule in words instead of restating its value"),
+		DiscardDetail.Contains(TEXT("charged once for the whole hand"), ESearchCase::CaseSensitive));
```

- **The new phrase carries the claim:** the fee is one flat charge for the whole hand (`CARDBAR-§9`).
- **The phrase was already in the prose.** Only `DiscardAllCost` in front of it changed, to "a single set amount". So the pin now guards the rule sentence rather than a symbol name.
- **⚖️ Judgment call, declared for QA to rule on:** I changed the assertion's **label literal** (:1818) along with the pinned literal (:1819). Left alone, the label would have told every future reader that "the page NAMES its fee property", which is false now. My fence says "a pin literal that must move, only", and I read the label as part of the pin. If QA reads it more narrowly, the revert is one literal, but the label would then be false.
- **⇒ `TASK-1538` owes a mutation arm for this pin, seen red.** Recipe: change "charged once" in the `Cards.Discard` `Detail` literal (after line 688), expect the (e) `TestTrue` to fail, then revert.

**Pins over changed strings that did NOT move.** Each is byte-identical and still holds on the after file, checked by `verify.py` on the composed text:

- **`{Cards.Discard}` token** (:1805-1807) holds; the token is untouched.
- **Digit scans:**
  - `Cards.Discard` one-liner and detail (:1848-1850): no digit.
  - `Cards.StackUpgrade`, `Cards.PlacementResize` and `Interface.MapMarks` (the loop around :1967): no digit in either field.
- **Retired vocabulary** (`DiscardCost` / `DiscardHandSlot` / `RequestDiscardSlot`, :1824-1829): absent.
- **Scrapped-route fragments on `Cards.Discard`** (:1856): absent.
- **Refuted-route fragments on `Interface.WarMap`** (:2372): absent.
- **The positive control on `Interface.MapMarks`** (:2393, "right-click"): still present.
- **`ForbiddenInPlayerProse`** (:1111-1115: `.cpp:`, `.h:`, `handoffs/`, `TASK-`, `SPC:`, `**`, backtick, `§`): absent from every row. I also checked `::` and `()`: absent from every row.
- **Detail longer than one-liner:** true for every row.

**No other test pins any phrase in a changed string.** `SiegeCardHandKeyLabelTest.cpp` and `SiegeMenuInputTest.cpp` only read chips and ids.

**Moved-pin count: 1.**

## 5. Hashes and the diff

| File | Before sha256 (bytes) | After sha256 (bytes) |
|---|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/SiegeControlsHelpWidget.cpp` | `f407d0b4c1d1df474fe2c0303cbe2eed6cb9636fb33526ffc2afe308ade96137` (273661, 4439 lines) | `ada3609ff7115fa6c3a202f2998e1a42d381e8d7bbe60cfb1f0de14def1debcb` (277821, 4484 lines) |
| `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeControlsHelpTest.cpp` | `9870e40fa10ca7f9e5c3380f582fc17cdc2e48d5cbbab579e4f3149dbcd025cc` (150453) | `525b5886779d1e346162f3d1e04e226ef69cfffab6ed6714705341385fc591d7` (150481) |

`TASKBOARD.md` (only my `status:` line) and this handoff are also written. The board is edited concurrently by the manager, so no stable hash for it is claimed.

**Structural proof that only literals and comments changed** (`verify.py`):

1. Mask every string literal.
2. Delete every comment.
3. Collapse whitespace.
4. Compare the start and after "code skeletons".

Result: the skeletons are **identical** for both files. The literal count is unchanged: **610 → 610** in the widget and **406 → 406** in the test. **50** literals changed in the widget and **2** in the test.

**`git --no-optional-locks diff --no-index -U0 <start copy> SiegeControlsHelpWidget.cpp`** (221 lines):

```diff
diff --git a/SiegeControlsHelpWidget.start.cpp b/C:/GitProjects/GitHub/GitClaudeUnrealTesting/GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/SiegeControlsHelpWidget.cpp
index 954052f..af1ce2d 100644
--- a/SiegeControlsHelpWidget.start.cpp
+++ b/C:/GitProjects/GitHub/GitClaudeUnrealTesting/GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/SiegeControlsHelpWidget.cpp
@@ -399 +399 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-				TEXT("The back and left rows carry Negate modifiers and the forward/back rows carry SwizzleAxis, ")
+				TEXT("The back and left rows carry a modifier that reverses the direction, and the forward and back rows carry one that turns the input onto the forward axis, ")
@@ -401,2 +401,2 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-				TEXT("Base walk speed is AHeroCharacter::WalkSpeed; the speed actually applied composes the Swift Boots ")
-				TEXT("upgrade on top and is GetEffectiveWalkSpeed() — the base is never mutated.\n\n")
+				TEXT("Your base walk speed is set on the hero; the speed you actually move at applies the Swift Boots ")
+				TEXT("upgrade on top of that base, and the base itself is never changed.\n\n")
@@ -404,0 +405,3 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
+			// TASK-1541 (2026-09-27): player words replace the code names this prose used to print —
+			// the Negate and SwizzleAxis input modifiers, AHeroCharacter::WalkSpeed and
+			// AHeroCharacter::GetEffectiveWalkSpeed(). Cited by symbol; the claims are unchanged.
@@ -436,3 +439,3 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-				TEXT("Bound as a 2D mouse axis with a Negate_2 modifier on the Y channel. ")
-				TEXT("Look is suspended while you hold the interface-cursor key — OnUICursorPressed calls ")
-				TEXT("SetIgnoreLookInput(true), paired 1:1 with its release, so a click-drag on the HUD cannot ")
+				TEXT("Bound to the mouse's up-down and left-right movement, with a modifier that reverses the up-down direction. ")
+				TEXT("Look is suspended while you hold the interface-cursor key — pressing the key switches camera look off ")
+				TEXT("and its release switches it back on, one release for every press, so a click-drag on the HUD cannot ")
@@ -440,2 +443,2 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-				TEXT("It is not suspended in placement, targeting or a group pick: those modes use ")
-				TEXT("FInputModeGameAndUI with DoNotLock, so the mouse steers the cursor while movement keys ")
+				TEXT("It is not suspended in placement, targeting or a group pick: those modes let the game and the interface ")
+				TEXT("both take input without locking the mouse to the window, so the mouse steers the cursor while movement keys ")
@@ -442,0 +446,3 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
+			// TASK-1541 (2026-09-27): player words replace the code names this prose used to print —
+			// Negate_2, OnUICursorPressed, SetIgnoreLookInput(true), FInputModeGameAndUI and DoNotLock,
+			// every one anchored by text in the citation block above. The claims are unchanged.
@@ -456,2 +462,2 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-				TEXT("Falling out of the world is a death, not a despawn — AHeroCharacter::FellOutOfWorld ")
-				TEXT("deliberately does not call Super (which would Destroy() the pawn) and routes into the same ")
+				TEXT("Falling out of the world is a death, not a despawn — the hero deliberately skips the engine's ")
+				TEXT("default handling (which would delete your hero outright) and goes down the same ")
@@ -458,0 +465,2 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
+			// TASK-1541 (2026-09-27): player words replace the code names this prose used to print —
+			// AHeroCharacter::FellOutOfWorld, its skipped Super call, and Destroy(). The claim is unchanged.
@@ -475,6 +483,6 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-				TEXT("Bound on Started, Completed and Canceled, so the sprint can never stick on if the press is ")
-				TEXT("interrupted. Pressing raises the max walk speed to SprintSpeed and releasing returns it to ")
-				TEXT("WalkSpeed; both compose the Swift Boots move-speed bonus live via GetEffectiveSprintSpeed() ")
-				TEXT("and GetEffectiveWalkSpeed() rather than mutating the base.\n\n")
-				TEXT("A dead hero cannot start a sprint — StartSprint early-outs on bDead — but StopSprint is ")
-				TEXT("unguarded so the state always releases.\n\n")
+				TEXT("It listens for the press, the release and a cancelled press, so the sprint can never stick on if the press is ")
+				TEXT("interrupted. Pressing raises your top speed to your sprint speed and releasing returns it to ")
+				TEXT("your walk speed; both include the Swift Boots move-speed bonus, worked out fresh each time, ")
+				TEXT("rather than changing either base speed.\n\n")
+				TEXT("A dead hero cannot start a sprint — starting checks for death first — but stopping has no such ")
+				TEXT("check, so the sprint always releases.\n\n")
@@ -482 +490,5 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-				TEXT("another, and nothing in DoMeleeAttack reads the sprint flag.")));
+				TEXT("another, and the melee swing never looks at whether you are sprinting.")));
+			// TASK-1541 (2026-09-27): player words replace the code names this prose used to print —
+			// the Started / Completed / Canceled trigger events, SprintSpeed, WalkSpeed,
+			// GetEffectiveSprintSpeed(), GetEffectiveWalkSpeed(), StartSprint's bDead early-out,
+			// StopSprint and DoMeleeAttack. Cited by symbol; the claims are unchanged.
@@ -497 +509,4 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-			// ⛔ NO NUMBER RESTATED: the three tunables stay NAMED (704 U-5, the M7.7 lesson).
+			// ⛔ NO NUMBER RESTATED (704 U-5, the M7.7 lesson), and since TASK-1541 (2026-09-27) no
+			// code name either: the prose says "melee reach", "a cone in front of you" and "melee
+			// cooldown" where it printed MeleeRange, MeleeHalfAngleDegrees and MeleeCooldown, and it no
+			// longer prints GetEffectiveMeleeDamage(), SetMeleeSuppressed(true) or DoMeleeAttack.
@@ -499,4 +514,4 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-				TEXT("One swing damages all enemy team agents within MeleeRange and inside a ")
-				TEXT("±MeleeHalfAngleDegrees forward cone, rate-limited to one swing per MeleeCooldown seconds. ")
-				TEXT("Damage per swing is composed live — base plus the Sharpened Blade stacks — through ")
-				TEXT("GetEffectiveMeleeDamage(). No friendly fire.\n\n")
+				TEXT("One swing damages everything on the enemy team within your melee reach and inside a ")
+				TEXT("cone in front of you, and you can swing at most once per melee cooldown. ")
+				TEXT("Damage per swing is worked out fresh each time — the base damage plus the Sharpened Blade ")
+				TEXT("stacks. No friendly fire.\n\n")
@@ -505 +520 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-				TEXT("SetMeleeSuppressed(true) makes DoMeleeAttack a no-op that does not even consume the cooldown.")));
+				TEXT("the swing is skipped entirely and does not even use up the cooldown.")));
@@ -519,6 +534,10 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-				TEXT("Buffs every same-team ASummonedUnit within RallyRadius by RallySpeedBonus for ")
-				TEXT("RallyDuration seconds — units only, never the hero, never enemy units. Friendly miners are ")
-				TEXT("included, since AMinerUnit is a summoned-unit subclass.\n\n")
-				TEXT("On cooldown the press is a no-op but still broadcasts OnRallyStateChanged(false, remaining) ")
-				TEXT("so the HUD can flash the time left; the cooldown length is RallyCooldown and OnRallyReady ")
-				TEXT("re-broadcasts (true, 0) when it elapses. A dead hero cannot rally.")));
+				TEXT("Speeds up every friendly summoned unit within the rally radius by the rally speed bonus for ")
+				TEXT("the rally duration — units only, never the hero, never enemy units. Friendly miners are ")
+				TEXT("included, since a miner is a kind of summoned unit.\n\n")
+				TEXT("On cooldown the press does nothing, but it still tells the HUD how much cooldown is left ")
+				TEXT("so the HUD can flash the time left; when the rally cooldown runs out, the HUD is told ")
+				TEXT("that Rally is ready again. A dead hero cannot rally.")));
+			// TASK-1541 (2026-09-27): player words replace the code names this prose used to print —
+			// ASummonedUnit, RallyRadius, RallySpeedBonus, RallyDuration, AMinerUnit,
+			// OnRallyStateChanged(false, remaining), RallyCooldown, and OnRallyReady's (true, 0)
+			// re-broadcast. Cited by symbol; the claims are unchanged.
@@ -589 +608 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-				TEXT("A hold, not a toggle. Holding puts the game in GameAndUI with the cursor visible and camera ")
+				TEXT("A hold, not a toggle. Holding lets the game and the HUD both take input, with the cursor visible and camera ")
@@ -591 +610 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-				TEXT("Bound on Completed and Canceled so the hold can never stick regardless of the action's ")
+				TEXT("The release fires on a normal release and on a cancelled press, so the hold can never stick regardless of the action's ")
@@ -595,0 +615,2 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
+			// TASK-1541 (2026-09-27): player words replace the code names this prose used to print —
+			// the GameAndUI input mode and the Completed / Canceled trigger events. The claims are unchanged.
@@ -653 +674,3 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-			// ⛔⛔ THE FEE IS NAMED AND NEVER TYPED. DiscardAllCost's own header comment pins this
+			// ⛔⛔ THE FEE IS NEVER TYPED — and since TASK-1541 (2026-09-27) it is not NAMED either: the
+			// prose says "a single set amount" where it printed DiscardAllCost, and the suite's pin
+			// moved with it to "charged once for the whole hand". DiscardAllCost's own header comment pins this
@@ -665 +688 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-				TEXT("The fee is DiscardAllCost and it is charged once for the whole hand, flat. Dumping a ")
+				TEXT("The fee is a single set amount and it is charged once for the whole hand, flat. Dumping a ")
@@ -775,4 +798,4 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-			// ⛔ NO TUNABLE'S VALUE IS TYPED — MaxStackHeightMultiplier and StackHealthStep are
-			// NAMED, exactly as PickMode.Resize names its three radii (the M7.7 "in 400" lesson).
-			// ⚠️ The jargon cost F-3 already flags for that row applies here too and is declared in
-			// this task's handoff rather than solved by inventing a number.
+			// ⛔ NO TUNABLE'S VALUE IS TYPED — MaxStackHeightMultiplier and StackHealthStep were
+			// NAMED here, as PickMode.Resize named its three radii (the M7.7 "in 400" lesson), until
+			// TASK-1541 (2026-09-27) put both in player words: "a set maximum multiple" and "a set
+			// step". ⚠️ The jargon cost F-3 flagged is paid by the wording, still without inventing a number.
@@ -792,2 +815,2 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-				TEXT("height, and it stops at MaxStackHeightMultiplier times that original. Its width and length are ")
-				TEXT("not touched. Health: each upgrade multiplies the building's maximum health by StackHealthStep, ")
+				TEXT("height, and it stops at a set maximum multiple of that original. Its width and length are ")
+				TEXT("not touched. Health: each upgrade multiplies the building's maximum health by a set step, ")
@@ -875,2 +898,2 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-				TEXT("One notch changes the size by PlacementFootprintWheelStep, between PlacementFootprintMin and ")
-				TEXT("PlacementFootprintMax. It will not go below the floor: you can make a building bigger than it ")
+				TEXT("One notch changes the size by a set step, and the size always stays between a set floor and a set ")
+				TEXT("ceiling. It will not go below the floor: you can make a building bigger than it ")
@@ -891,0 +915,3 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
+			// TASK-1541 (2026-09-27): player words replace the code names this prose used to print —
+			// PlacementFootprintWheelStep, PlacementFootprintMin and PlacementFootprintMax, all three
+			// cited by symbol above. Still no value typed.
@@ -922,2 +948,2 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-				TEXT("The stance is latched — it persists until replaced — and bHasIssuedCommand flips true on ")
-				TEXT("your first command and stays true for the match, so the pre-command legacy behaviour never ")
+				TEXT("The stance is latched — it persists until replaced — and the game records your first command ")
+				TEXT("and keeps that record for the rest of the match, so the pre-command legacy behaviour never ")
@@ -924,0 +951,2 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
+			// TASK-1541 (2026-09-27): player words replace the code name this prose used to print —
+			// bHasIssuedCommand, cited above. The claim is unchanged.
@@ -945 +973 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-				TEXT("What \"the band\" means CHANGED: DefendRadius is no longer a disc centred on the castle — it ")
+				TEXT("What \"the band\" means CHANGED: the defend range is no longer a disc centred on the castle — it ")
@@ -948 +976,4 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-				TEXT("ASummonedUnit::ResolveDefendEngagementRadius.")));
+				TEXT("each unit for itself.")));
+			// TASK-1541 (2026-09-27): player words replace the code names this prose used to print —
+			// DefendRadius and ASummonedUnit::ResolveDefendEngagementRadius, both cited above. The
+			// claim is unchanged.
@@ -1125 +1156,3 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-			// ⛔ NO RADIUS VALUE RESTATED: the three defaults stay NAMED (704 U-5, the M7.7 lesson).
+			// ⛔ NO RADIUS VALUE RESTATED (704 U-5, the M7.7 lesson). The three defaults were NAMED
+			// here until TASK-1541 (2026-09-27); the prose now says "its own default size" for each of
+			// GroupSelectRadiusDefault, GroupPositionRadiusDefault and GroupAttackRadiusDefault.
@@ -1130 +1163 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-				TEXT("1. SELECT — opens at GroupSelectRadiusDefault. At confirm, every eligible unit inside it ")
+				TEXT("1. SELECT — opens at its own default size. At confirm, every eligible unit inside it ")
@@ -1137 +1170 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-				TEXT("2. POSITION — opens at GroupPositionRadiusDefault. This is the ground the squad stands on: ")
+				TEXT("2. POSITION — opens at its own default size. This is the ground the squad stands on: ")
@@ -1140 +1173 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-				TEXT("3. ATTACK — opens at GroupAttackRadiusDefault. This is the first-priority engage trigger: an ")
+				TEXT("3. ATTACK — opens at its own default size. This is the first-priority engage trigger: an ")
@@ -1166 +1199 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-			// ⛔ ALL THREE TUNABLES STAY NAMED, ⛔ never re-typed as numbers. This is 704's U-5 / D-6
+			// ⛔ NONE OF THE THREE TUNABLES IS ⛔ ever re-typed as a number. This is 704's U-5 / D-6
@@ -1168,2 +1201,4 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-			// while the real radius was 700. ⚠️ It reads as jargon on screen and I have FLAGGED that
-			// for Jonathan (F-3 in handoffs/TASK-707-programmer.md) rather than invent a number.
+			// while the real radius was 700. ⚠️ Naming them read as jargon on screen (F-3 in
+			// handoffs/TASK-707-programmer.md); TASK-1541 (2026-09-27) answered F-3 with player words
+			// ("a set step", "a set smallest and largest size") in place of GroupRadiusWheelStep,
+			// GroupRadiusMin and GroupRadiusMax — still without inventing a number.
@@ -1171,2 +1206,2 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-				TEXT("One notch changes the active circle's radius by GroupRadiusWheelStep, clamped between ")
-				TEXT("GroupRadiusMin and GroupRadiusMax. Each stage opens at its own default and resizing one ")
+				TEXT("One notch changes the active circle's radius by a set step, and the radius always stays between ")
+				TEXT("a set smallest and largest size. Each stage opens at its own default and resizing one ")
@@ -1257,2 +1292,2 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-				TEXT("CLOSING — the complete list: (1) press the open key again; (2) CancelPressed() — public API ")
-				TEXT("with no caller today, kept deliberately; (3) the fault latch SetConsoleEnabled(false); ")
+				TEXT("CLOSING — the complete list: (1) press the open key again; (2) a spare close route the game ")
+				TEXT("keeps on purpose, though nothing uses it today; (3) the box switching itself off when the assistant faults; ")
@@ -1268,0 +1304,4 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
+			// TASK-1541 (2026-09-27): player words replace the code names this prose used to print —
+			// USiegeAssistantConsoleWidget::CancelPressed() (close route 2) and SetConsoleEnabled(false)
+			// (route 3, the fault latch), per the enumerated close-route contract cited above. The
+			// claims are unchanged.
@@ -1403 +1442 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-				TEXT("both belong to the commander — ACommanderNpc::IsPlayerInRange reading InteractRadius — and ")
+				TEXT("both belong to the commander — he checks whether you are inside his own interaction range — and ")
@@ -1415,0 +1455,2 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
+			// TASK-1541 (2026-09-27): player words replace the code names this prose used to print —
+			// ACommanderNpc::IsPlayerInRange reading ACommanderNpc::InteractRadius. The claim is unchanged.
@@ -1450,3 +1491,4 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-			// tunable is NAMED (704 U-5, the M7.7 "in 400"/AoERadius-700 lesson) — and note that the
-			// sentence around the quote still names EnemyRevealCost, so the mechanism, not the
-			// number, is what the page teaches.
+			// tunable is described, never typed (704 U-5, the M7.7 "in 400"/AoERadius-700 lesson; they
+			// were NAMED until TASK-1541, 2026-09-27) — and the sentence around the quote calls it "a
+			// fixed reveal fee" where it printed EnemyRevealCost, so the mechanism, not the number, is
+			// what the page teaches.
@@ -1454 +1496 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-				TEXT("The price is EnemyRevealCost — Jonathan's own number: \"You can pay 30 gold to reveal all ")
+				TEXT("The price is a fixed reveal fee — Jonathan's own number: \"You can pay 30 gold to reveal all ")
@@ -1549 +1591,2 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-			// ⛔ NO NUMBER IS TYPED: MaxMapMarks is NAMED. ⛔ And ⛔ no coordinate, radius or count
+			// ⛔ NO NUMBER IS TYPED: the cap is "a limited number" in the prose (it printed MaxMapMarks
+			// until TASK-1541, 2026-09-27, 🧑 his "plain words"). ⛔ And ⛔ no coordinate, radius or count
@@ -1581 +1624 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-				TEXT("You can hold MaxMapMarks circles at once. At the limit a further click refuses out loud and ")
+				TEXT("You can hold a limited number of circles at once. At the limit a further click refuses out loud and ")
@@ -1631 +1674 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-				TEXT("Cursor posture is added to ApplyCursorInputState()'s one composition and nowhere else — the ")
+				TEXT("Its cursor is handled in the one place the game decides who owns the cursor, and nowhere else — the ")
@@ -1632,0 +1676,2 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
+			// TASK-1541 (2026-09-27): player words replace the code name this prose used to print —
+			// ASiegePlayerController::ApplyCursorInputState(), cited by name above. The claim is unchanged.
```

**`git --no-optional-locks diff --no-index -U0 <start copy> Tests/SiegeControlsHelpTest.cpp`:**

```diff
diff --git a/SiegeControlsHelpTest.start.cpp b/C:/GitProjects/GitHub/GitClaudeUnrealTesting/GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeControlsHelpTest.cpp
index b78ee60..fe6cc09 100644
--- a/SiegeControlsHelpTest.start.cpp
+++ b/C:/GitProjects/GitHub/GitClaudeUnrealTesting/GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeControlsHelpTest.cpp
@@ -1818,2 +1818,2 @@ bool FSiegeControlsHelpDiscardAllLayoutTest::RunTest(const FString& Parameters)
-	TestTrue(TEXT("⭐ The page NAMES its fee property instead of restating its value"),
-		DiscardDetail.Contains(TEXT("DiscardAllCost"), ESearchCase::CaseSensitive));
+	TestTrue(TEXT("⭐ The page states the flat-fee rule in words instead of restating its value"),
+		DiscardDetail.Contains(TEXT("charged once for the whole hand"), ESearchCase::CaseSensitive));
```

## 6. The statement

**Only bytes inside player-facing `TEXT(…)` literals and the comments beside them changed** in `SiegeControlsHelpWidget.cpp`. In `Tests/SiegeControlsHelpTest.cpp`, only the one moved pin's two literals changed. No executable token, no `TEXT(` wrapper, no statement and no line of code was added, removed or reordered (§5's skeleton proof). No number was typed into prose. The only digits removed were in `Hero.Look` (`2D`, `Negate_2` and `1:1`) and `Hero.Rally` (the `0` in `(true, 0)`). The digits already in the changed lines (the `1.`/`2.`/`3.` stage labels, the chat box's `(1)`–`(3)` list, and Jonathan's quoted `30`) are byte-identical, and no digit was added. No derivation code and no `FText::Format` was added.

## 7. False claims found (`SC-§101`)

**None found.** Every claim was kept as it stood. While rewording, I spot-checked these at source:

- `StartSprint` refuses on `bDead`; `StopSprint` has no such guard.
- `DoMeleeAttack` returns on `bMeleeSuppressed` **before** the `LastMeleeTime` cooldown stamp. So "does not even use up the cooldown" holds.
- `AHeroCharacter::FellOutOfWorld` deliberately skips `Super`.
- `USiegeAssistantConsoleWidget::CancelPressed()` is close route 2 in the widget header's enumerated list, and it has **no caller** in `Source/`. The component's own `CancelPressed` bound to `OnConsoleCancelled` is a different function.
- `ACommanderNpc::IsPlayerInRange` and `InteractRadius` exist on the commander.
- `ASummonedUnit`'s `ResolveDefendEngagementRadius` exists.
- `USiegeMapMarkSubsystem::MaxMapMarks` exists.

Every other sentence's truth is **not re-examined** by this row (see limitations).

## 8. Follow-up number candidates (listed, ⛔ NOT built — spec (3))

**No derivation exists in the file today:** there is no `FText::Format`, `FText::AsNumber` or `GetDefault<>` in `SiegeControlsHelpWidget.cpp`. `ResolveDetailTokens` splices key chips only. Any of the items below would be new code: a number token or a format step. Ranked by how much a player would plausibly gain:

1. `Interface.MapMarks` — the circle cap (`USiegeMapMarkSubsystem::MaxMapMarks`): "how many can I hold?" The refusal line already states the count at the limit.
2. `Cards.Discard` — the fee (`DiscardAllCost`). Its own header comment already says that if it is ever shown, it is READ from the property.
3. `Hero.Rally` — rally radius, speed bonus, duration and cooldown (`AHeroCharacter::RallyRadius` / `RallySpeedBonus` / `RallyDuration` / `RallyCooldown`).
4. `Cards.StackUpgrade` — the height cap and the health step (the building CDO's `MaxStackHeightMultiplier` / `StackHealthStep`).
5. `Hero.Attack` — melee reach, cone half-angle and cooldown.
6. Low value: `Interface.WarMap` (the interaction range), `Hero.Move` / `Hero.Sprint` (the speeds), `Cards.PlacementResize`, and `PickMode.Confirm` / `PickMode.Resize` (steps, floors, ceilings, default radii).

## 9. Other follow-ups, outside my fence (`SC-§53`), NOT edited

These comments now describe the pre-1541 state. They sit outside the `names:` fence, so I report them rather than touch them:

- **`SiegeControlsHelpWidget.cpp:336-337`** (after numbering; the file-level TASK-707 transfer block, not beside any string): "704 §4 deliberately NAMES tunables instead of restating numbers … and the names are carried through unchanged." This is no longer true of the `Detail` column. The same block's "Every detail string below is handoffs/TASK-704-programmer.md §4's prose" was already stale before this row (821 / 823 / 870 rewrote rows).
- **`SiegeControlsHelpWidget.h:162-166`** (the `Detail` field's doc): "TASK-704 §4 deliberately NAMES tunables (GroupRadiusWheelStep, MeleeCooldown, EnemyRevealCost …) … and this task keeps that." Now stale. The header is not in my fence.
- **`Tests/SiegeControlsHelpTest.cpp:1800-1804`**: "the prose also names DiscardAllCost, so that letter is present whatever the implementation does". The reason is stale, but the conclusion still holds, because the page still contains a capital `D` ("Dumping").
- **`Tests/SiegeControlsHelpTest.cpp:1809`**: the section header "(e) ⛔ THE FEE IS NAMED, ⛔ NEVER TYPED". The "NAMED" half is stale. My fence allows only the moved pin's literals.
- **A guard against recurrence (not built):** adding `TEXT("::")` and `TEXT("()")` to `ForbiddenInPlayerProse` (:1111-1115) would make this defect class go red registry-wide. It would need a new mutation arm. It is a new assertion, so it is the manager's to board.

## What QA (`TASK-1546`) should scrutinize

1. **Same rule, new words (`SC-§101`).** These rewordings deserve the closest check:
   - R-01: the two modifier descriptions.
   - R-02: "one release for every press", which was "paired 1:1 with its release".
   - R-04: "It listens for the press, the release and a cancelled press", for `Started` / `Completed` / `Canceled`.
   - R-05: "everything on the enemy team", for "all enemy team agents".
   - R-08: "The release fires on a normal release and on a cancelled press", for `Completed` / `Canceled`.
   - R-12: "by each unit for itself", for `ASummonedUnit::ResolveDefendEngagementRadius`.
   - R-19: close route (2), "a spare close route the game keeps on purpose, though nothing uses it today".
   - R-24: "handled in the one place the game decides who owns the cursor".
2. **The moved pin and my label change** (§4).
3. **The fence:** the skeleton proof (§5), and that no `TASK-1480` hunk bytes changed (§3).
4. **The comment amendments in §3** each state the same rule as before; only the "NAMED" half changed.

**For 5b (`TASK-1547`), the verifier read:** the 19 pages listed in §1. Every one is a `Detail` page. The one-liners on the list view did not change.

## Not examined / limitations

- **Scope was code identifiers.** The pages still read in a developer register in places:
  - "Polled directly every frame … not bound to an input action" (`PickMode.Confirm`, `PickMode.Cancel`)
  - "idempotent", "state tick", "input asset", "the mapping context's keys", "the controller", "ignore-look counter", "trigger setup", "monotone upgrade", "eligibility predicate"

  These are ordinary words, not code names, so this row does not cover them. A register pass would be a separate ask to 🧑 him.
- **The census classes are a regex plus one manual sweep.** A code name written as plain lowercase words would evade both, e.g. "the fault latch", which I did rewrite because it sat in a hit sentence. I read every changed row in full; I did **not** read the 8 zero-hit rows line by line beyond the automated sweep.
- **The truth of the sentences** beyond the §7 spot checks was not re-examined. The mechanism-3 citation blocks were not re-anchored. `TASK-1480` (a) declares their numbers unverified, and I added no numbers.
- **Nothing was compiled or run.** Rendering on the real `UTextBlock` (wrapping, length) is 5b's to observe. Every changed `Detail` is still far longer than its one-liner. The largest growth of any single literal is 62 characters (R-01, line 399). The next largest are 39 (R-02) and 32 (R-08).
- **The borderline classification** (8 tokens) is my judgment: capitalised single words and a bare argument list that a player reads as code. QA may rule any of them not a hit. They are fixed either way, and the fixes are text-only.

## QA loop 1 (fix for `qa/TASK-1546.md`, 2026-09-27)

- **Status set:** `ready-for-qa` (re-gate `TASK-1546`), marker `TASK-1541-LOOP1-READY-FOR-QA-2026-09-27`.
- **Start bytes, measured before any edit:** `SiegeControlsHelpWidget.cpp` sha256 `ada3609ff7115fa6c3a202f2998e1a42d381e8d7bbe60cfb1f0de14def1debcb` (277821 B). This equals the loop-0 after-sha256 QA re-measured (§8). A scratch copy was taken before editing: `scratchpad\SiegeControlsHelpWidget.loop0.cpp` (same hash).
- **After bytes:** `SiegeControlsHelpWidget.cpp` sha256 **`a6bf281fd83d4db65461fe8a831e93b632faa1aca9af80ef6785b6cfbc235d8b`**, 278984 B, 4497 lines, LF only (0 CR bytes), no BOM.
- **Test file untouched:** `Tests/SiegeControlsHelpTest.cpp` is still `525b5886779d1e346162f3d1e04e226ef69cfffab6ed6714705341385fc591d7`, re-hashed after the edits.
- **Declared tooling:**
  - `sha256sum`, `wc`, `tr` and `od` for byte checks.
  - My loop-0 scratchpad scripts (`lex.py`, `verify.py`, `census.py`), plus two new read-only ones (`loop1/delta.py`, `loop1/pinpresence.py`).
  - One `git --no-optional-locks diff --no-index -U0` between the scratch copy and the file.
  - No other git, no compile, no editor, no asset.

### Findings, one by one

| # | Finding | Taken? | Change |
|---|---|---|---|
| B1 | R-05 `Hero.Attack` `Detail`: "everything on the enemy team" was false at source | **Fixed** | `"One swing damages everything on the enemy team within your melee reach and inside a "` → **`"One swing damages every enemy unit, hero, building and castle within your melee reach and inside a "`**. The next literal ("cone in front of you, …") is unchanged. |
| B1 (optional) | R-05 comment: record the exclusion | **Taken** | 8 comment lines added before `Row.Detail`, after the existing R-05 "no code name either" block. |
| W1.1 | R-22 comment beside the one-liner: "or leaves the name" went stale | **Fixed** | The comment's last line now continues with the loop-0 change: the page leaves no name either, the one-liner types no number and names no property, and the detail says "a fixed reveal fee" where it printed `EnemyRevealCost`. The two lines above it are byte-identical. So the old `CommanderNpc.h` citation stays on an unchanged line, and no changed line cites a line number. |
| W1.2 | `Tests/SiegeControlsHelpTest.cpp:2058` label "the tunables are NAMED" | **Not taken, as ruled** | Out of my fence; it goes to the manager. The test file is byte-identical. |
| N1 | R-25 `Cards.StackUpgrade`: "by a set step" | **Taken** | → **"by a set factor"**. The R-25 comment quoted "a set step", so it was amended to match (`SC-§53`). |
| N2 | R-06 `Hero.Rally`: says "the HUD … the time left" twice | **Taken** | `"so the HUD can flash the time left; when …"` → **`"so the HUD can flash it; when …"`**. The line before is unchanged ("…it still tells the HUD how much cooldown is left "), so "it" refers to that remaining time. I kept "the HUD can" instead of QA's "so it can", because "it" there could read as the press. |

### Truth at source for the changed sentences (`SC-§101`)

**B1: the set named equals the set the swing hits.**

- **Where the candidates come from:** `AHeroCharacter::DoMeleeAttack` gets them from `FSiegeCombatStatics::GatherHostileAgents`, beside the comment "hit ALL enemy team agents within MeleeRange…". That gatherer calls `GatherTeamAgentsFiltered(…, bWantHostile=true, …)`.
- **The implementers:** a grep of `public ITeamAgent` finds exactly four `ITeamAgent` implementers in `Source/`: `ACastle`, `ABuilding`, `AHeroCharacter` and `ASummonedUnit`.
- **The set is closed:** `UTeamAgent` is declared `UINTERFACE(MinimalAPI, NotBlueprintable)` in `TeamId.h`. No Blueprint class can add a fifth kind.
- **"building" is exact:** every card-placed structure class is an `ABuilding` subclass: `ABarracks`, `ATower`, `AClimbableTower` and `ADeepMine`.
- **"unit" covers miners:** `AMinerUnit` is a summoned-unit subclass, the same fact R-06 already states.
- **Excluded, and the sentence no longer implies otherwise:**
  - `ACommanderNpc` is a plain `AActor`. Its class doc says it deliberately does not implement `ITeamAgent`, and it lists the melee cone among the sites.
  - `AGoldNode` is a plain `AActor` and opts out the same way.
  - Capture zones, ancient grounds and torches are not agents, and they are not "buildings" in the player's words.
- **Not claimed, as before:** this gather skips veiled units (`SuppressVeiled`). Neither the old sentence nor the new one mentions it, and QA ruled that silence not owed.

**The other changed sentences:**

- **N1:** `StackHealthStep` compounds as `StackHealthStep ^ UpgradeCount` (the `ABuilding` header doc). It is a multiplier, so "factor" is the right word. The rest of the sentence ("multiplies … compounding … no ceiling") is unchanged.
- **N2:** the sentence says the same thing as before, with the repetition removed: on cooldown the press still reports the remaining time so the HUD can flash it.
- **W1.1 (a comment):**
  - The one-liner is "Pay gold on the war map to reveal every enemy position — and they vanish again the moment you close it." It has no number and no property name.
  - The widget code reads no reveal-cost property: `EnemyRevealCost` appears in `SiegeControlsHelpWidget.cpp` only in comments.

### The fence, re-proven on the new bytes

- **Code skeleton: identical.** Literals were masked, comments removed and whitespace collapsed, then the loop-0 copy was compared with the new file.
- **Literals: 610 = 610. Changed: 3**, all `Detail` literals: R-05 (after-line 522), R-06 (546) and R-25 (826). Zero `DisplayName` or `OneLine` bytes changed.
- **Comment ops: 3**, each inside its own row block. The comment count went 2049 → 2062 (+13).
  - R-05: one insert of 8 lines, before `Row.Detail`.
  - R-25: one replace, 1 line → 3.
  - R-22: one replace, 1 line → 4.
- **New or changed comment lines citing `:NNN` or "line NNN": 0.**
- **`TASK-1480`'s hunks are untouched.** The nearest hunk here is at loop-0 line 512, and the R-02 and R-08 citation ranges sit well clear of every hunk.
- **No other file touched**, except this handoff and my `status:` line on `TASKBOARD.md`.

### Re-census and pins

- **Census on the new file, same classes as loop 0: 0 code-name hits.** The only flags are the three `{ActionId}` key-chip tokens: `{Interface.WarMap}` in MapMarks and `{Interface.ControlsHelp}` ×2. These are the same exclusions as loop 0.
- **The changed literals, swept by hand and by regex:** no digit (old and new both `[]`), no `::`, no call, no `()`, no camelCase, no underscore identifier and no type prefix. The only capitals are "HUD" ×2 in R-06's literal, unchanged from loop 0.
- **`verify.py` constraint checks on the composed prose: 0 failures.** They cover:
  - `ForbiddenInPlayerProse`, plus `::` and `()`;
  - detail longer than one-liner, in all 27 rows;
  - the no-digit rows;
  - the WarMap refuted-route fragments;
  - the Discard scrapped-route fragments and retired vocabulary;
  - the Discard pin phrase "charged once for the whole hand", present;
  - the `{Cards.Discard}` token, present;
  - the MapMarks "right-click" positive control, present.
- **Pin presence diff: 0 presence changes.** Every literal in three test files was checked against the loop-0 prose and the new prose: `SiegeControlsHelpTest.cpp` (406), `SiegeCardHandKeyLabelTest.cpp` (63) and `SiegeMenuInputTest.cpp` (140).
- **No test literal contains any of the 3 old or 3 new phrases** (grep of `Source/`).
- **The moved-pin count stays 1.** `TASK-1538`'s mutation arm (QA §10, the `Cards.Discard` literal) is unaffected, because that literal is byte-identical.
- **Composed prose length:** 30401 → 30407 chars (+15 for B1, −11 for N2, +2 for N1).

### `git --no-optional-locks diff --no-index -U0` against the loop-0 bytes (`ada3609f…`)

```diff
diff --git a/SiegeControlsHelpWidget.loop0.cpp b/C:/GitProjects/GitHub/GitClaudeUnrealTesting/GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/SiegeControlsHelpWidget.cpp
index af1ce2d..d07fa5c 100644
--- a/SiegeControlsHelpWidget.loop0.cpp
+++ b/C:/GitProjects/GitHub/GitClaudeUnrealTesting/GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/SiegeControlsHelpWidget.cpp
@@ -512,0 +513,8 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
+			// ⛔ THE TARGET SET IS NAMED KIND BY KIND ("every enemy unit, hero, building and castle")
+			// because those are exactly the team-agent kinds FSiegeCombatStatics::GatherHostileAgents
+			// hands the swing: ACastle, ABuilding and its subclasses, AHeroCharacter, and ASummonedUnit
+			// including miners. UTeamAgent is NotBlueprintable, so no asset can join the set. ⛔ NEVER
+			// widen it to "everything on the enemy team": the enemy commander HAS a team but
+			// deliberately does not implement ITeamAgent and names the melee cone among the sites it
+			// stays out of (ACommanderNpc's class doc), and AGoldNode opts out the same way. The
+			// "everything on the enemy team" wording was loop 0 of TASK-1541 and QA TASK-1546 B1 caught it.
@@ -514 +522 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-				TEXT("One swing damages everything on the enemy team within your melee reach and inside a ")
+				TEXT("One swing damages every enemy unit, hero, building and castle within your melee reach and inside a ")
@@ -538 +546 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-				TEXT("so the HUD can flash the time left; when the rally cooldown runs out, the HUD is told ")
+				TEXT("so the HUD can flash it; when the rally cooldown runs out, the HUD is told ")
@@ -801 +809,3 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-			// step". ⚠️ The jargon cost F-3 flagged is paid by the wording, still without inventing a number.
+			// factor" (loop 1: "factor", not "step", because StackHealthStep compounds as a power of the
+			// upgrade count, and a step reads as an addition). ⚠️ The jargon cost F-3 flagged is paid by
+			// the wording, still without inventing a number.
@@ -816 +826 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-				TEXT("not touched. Health: each upgrade multiplies the building's maximum health by a set step, ")
+				TEXT("not touched. Health: each upgrade multiplies the building's maximum health by a set factor, ")
@@ -1477 +1487,4 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-			// the live property or leaves the name (`HELP-§2`, the M7.7 lesson).
+			// the live property or leaves the name (`HELP-§2`, the M7.7 lesson). Since TASK-1541
+			// (2026-09-27) the page leaves no name either: the one-liner types no number and names no
+			// property, and the detail calls the price "a fixed reveal fee" where it printed
+			// EnemyRevealCost (the amendment below records the same change).
```

### For the re-gate

- Review only the delta above: 3 literals and 3 comment ops.
- The 5a shape is unchanged: `.cpp`-only, no header, `N` = 566, 1 mutation arm.
- `TASK-1538` must compile the **re-gated** bytes (`a6bf281f…` if this passes), never `ada3609f…`.
- **For 5b (`TASK-1547`):** the changed pages are `Hero.Attack`, `Hero.Rally` and `Cards.StackUpgrade`. The first sentence of `Hero.Attack` grew by 15 characters, so its wrap on the real `UTextBlock` may shift.
