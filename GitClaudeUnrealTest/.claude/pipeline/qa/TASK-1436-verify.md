Verdict: MEASURED
# Verification — TASK-1436 (WAVE-D2-VERIFY, the milestone's final sitting)

**Per-subject verdicts — seven, one line each (the roll-up above is `MEASURED` only because two subjects are; 5 VERIFIED · 2 MEASURED · 0 VERIFY-FAILED · nothing was observed failing, so nothing blocks the commit — routing rule 5c).**

1. `TASK-1429` in-match arming — **VERIFIED**
2. `TASK-1434` assistant console — **MEASURED**
3. `TASK-1471` Row B self-driving flag — **VERIFIED**
4. `TASK-1474` deck-bar descent — **VERIFIED**
5. `TASK-1478` controls-help navigable — **VERIFIED**
6. `TASK-1482` victory screen — **VERIFIED**
7. `TASK-1484` detail scroll — **MEASURED**

Editor/Aura state: Aura connected (`editor_connected`). Editor **PID 24652**, identified per `SC-§118` by the in-process probe `os.getpid() = 24652` returned by the MCP server answering on `:8000` ⇒ the answering server **is** the owning process. Development build (`Log`-level lines print with no console verb). Maps: `L_Arena` (open at dispatch), `L_MainMenu` (loaded for the deck builder), `L_Arena` **restored at close**. PIE mode standalone, 1280×720 (viewport reports 1280×725, DPI scale 0.670639). Three PIE sessions across **2 attempts of 3**. Wall time ≈ 22 min (23:50 → 00:12). **PID 24652 left UP. No code, no asset, no compile, no git, no commit, no editor lifecycle action.** All seven subject rows read `built` before I started; Jonathan's go ("go when the build lands") was already given and the build had landed.

---

## 0. THE CONTROLLED NEGATIVE — FIRST, BEFORE ANY POSITIVE

**In a live match with nothing open, injection moved nothing, and the lane was demonstrably open while it did so.**

At `L_Arena` PIE `t=18.43 s`, `t=19.04 s` and `t=19.68 s` — a live match (`SiegeGameMode` / `SiegeGameState`, pawn `BP_HeroCharacter0`, 155 actors, 2 players) with **no screen open** — I injected `IA_MenuDown` twice and `IA_MenuAccept` once.

- `LogSiegeMenuInput` **total_matches = 0** for the entire session up to that point.
- Player transform byte-identical across the three injections (`deltas_vs_prev: {}`; location `-21007.816, 0, 98.150`, velocity 0, control rotation unchanged) at `t=18.41 s` and `t=20.29 s`.
- The first `LogSiegeMenuInput` line of the whole session is timestamped `23.52.07:483` — the instant the controls-help overlay opened, **3.5 minutes later**.

**The control that makes that zero mean something (this is the law the dispatch bound me to — a negative control proves the instrument can say NO; only a positive control proves it can say YES):**

| instrument | negative control | positive control | discriminated? |
|---|---|---|---|
| `get_unreal_output_logs`, category filter | `LogSiegeMenuInput` → 0 lines | unfiltered tail → **2,961** lines; `LogSiegeAssistant` → **3** lines, quoted | ✅ YES |
| `get_unreal_output_logs`, substring filter | `card-tile class` → 0; `DeckBar` → 0 | `Deck Builder` → **1** line, quoted | ✅ YES |
| same category, after the screen opened | — | `LogSiegeMenuInput` → **22 → 91** lines | ✅ YES |
| `ui_snapshot` `focused` field | most nodes `false` | `DetailScrollButton` → `focused: true` | ✅ YES (for `UButton`) |
| `capture_pie_frame` (composited) | scroll frames unchanged | overlay close → mean_luma **50 → 136**, pct_near_black **3.7e-05 → 0.0444** | ✅ YES |

`LogSiegeMenuInput` is declared `(LogSiegeMenuInput, Log, All)` (`SiegeMenuInputSubsystem.h:37`) and this is a Development build ⇒ **an absent line here is a ZERO, not an `UNOBSERVABLE`.** I am not claiming a void where the lane was open.

⇒ **P0 holds: a live match is NOT hijacked by the menu keys.** This is `TASK-1429`'s P1 signature (a) — *a fresh match prints no line at all* — which the row declared in advance as one of two correct signatures.

---

## 1. Acceptance lines → observations

| # | acceptance line | observable chosen | observed (quoted, with PIE `t=` / wall) | verdict |
|---|---|---|---|---|
| 1a | `TASK-1429` P0/P1/P4 — a live match must NOT be hijacked | `LogSiegeMenuInput` line count + player transform with nothing open | **0 lines**, transform unchanged, `t=18.4–20.3 s` (§0 above) | pass |
| 1b | `TASK-1429` the arming transition in the log (rising edge) | `IN-MATCH MENU VOCABULARY ARMED` | `ARMED (RegisterMenuNavTarget): IMC_MainMenu applied at priority 0 on 'SiegePlayerController_0' for registered screen 'SiegeControlsHelpWidget_0'; 1 screen(s) registered. Below IMC_Hero (priority 1) — shadows NO hero key; 'Enter' stays with IA_AssistantConsole.` — `23.52.07:483` | pass |
| 1c | `TASK-1429` the removal transition (falling edge) | `IN-MATCH MENU VOCABULARY DISARMED` | `DISARMED (UnregisterMenuNavTarget): IMC_MainMenu REMOVED from 'SiegePlayerController_0'; 0 screen(s) on the stack, live nav target now 'None'. The match has its keys back.` — `23.57.27:469`; then **ARMED again** on reopen `23.58.00:610` | pass |
| 1d | `TASK-1429` **P2/P3 — were they unlocked?** (declared `UNOBSERVABLE` at authoring because no in-match screen registered) | the positive limb: does a registered in-match screen now arm and drive? | **YES — UNLOCKED.** `SiegeControlsHelpWidget_0` registers in-match, arms the context, and the keys then drive its ring (28 stops walked). P2/P3 are **no longer unobservable and they PASS.** | pass (unlock) |
| 2a | `TASK-1434` console opens in-match on `IA_AssistantConsole` | `LogSiegeAssistant` | `[AssistantConsole] Opened.` `00.04.54:052` (PIE `t=19.5 s`), and again `00.05.37:191`; `[AssistantConsole] Closed.` `00.05.33:913` | pass |
| 2b | `TASK-1434` the declared one-stop tree | live tree | `Code-authored tree built (ruling A). Children: RootPanel=1 TranscriptText=1 StatusText=1 InputBox=1. No buttons are constructed` — `ui_snapshot` confirms 8 nodes, `InputBox` = `EditableTextBox`, `Visible`, `hit_testable: true`, `StatusText` = "Ready", **zero buttons** | pass |
| 2c | `TASK-1434` **deliberately does NOT register a nav target** (the ruled-correct behaviour) | `LogSiegeMenuInput` across the whole session | **zero** `menu nav target registered -> 'SiegeAssistantConsoleWidget…'` lines; `IN-MATCH … ARMED` fired **only** for `SiegeControlsHelpWidget_0`. The console never arms a menu context over live combat. | pass |
| 2d | `TASK-1434` **focus returns to the game on close** | functional: drive the hero after the close | close at `t=59.4 s`; hero at `x=-21007.816`; injected `IA_Move (0,1)` held 1.5 s; hero at `x=-21709.628` ⇒ **Δx = −701.81 uu**. The game has its input back. | pass |
| 2e | `TASK-1434` "text box focused on open" | `ui_snapshot` `focused` + a typed-character probe | `InputBox.focused: false`; `ui_perform` type "focusprobe" → `Text` still empty (`HintText` read back "Type an order, then press Enter", so the property reader discriminates). **BUT neither instrument is valid here** — see §3 | **UNOBSERVABLE** |
| 2f | `TASK-1434` "Accept via `IA_MenuAccept`" | — | **structurally impossible in this binary**, pre-declared by `qa/TASK-1435.md` WARN-3 (the tree builds no `UButton`). Not tested; reading the spec literally would bounce a correct build. | **UNOBSERVABLE** (pre-declared) |
| 3a | `TASK-1471` the generic ring must not drive a self-driving screen | the gate's own log line, by name | `FocusFirstNavStop declined: the active nav target 'WBP_DeckBuilder_C_0' (WBP_DeckBuilder_C) drives its OWN navigation (TASK-1471) — the generic ring does not walk its tree, press its buttons or step its controls. It stays registered and its focus stops are still counted.` `00.08.50:246`; **same decline on `MoveFocus(+1)`** `00.09.39:337` and **`MoveFocus(-1)`** `00.09.39:854` ⇒ **three of the four gates fired, by name** | pass |
| 3b | `TASK-1471` `IA_MenuAccept` **CANNOT** land on "Reset to Default" | the same gate | the gate's own sentence — *"does not … press its buttons"* — is the subsystem refusing **every** builder button from **every** focus state (`TASK-1472` RULING 1: *closes, not narrows*). **I did not press Accept on a tile** — see §3, I declined to mutate his `deck1`. | pass (by the gate) |
| 3c | `TASK-1471` the screen **stays registered** and its stops **stay counted** | the retarget line | `menu nav target registered -> 'WBP_DeckBuilder_C_0' (registered screen), 14 focus stop(s), 1 screen(s) registered.` — registered, counted, and gated. Both halves. | pass |
| 3d | `TASK-1471` **ABORT CONDITION** | `card-tile class … did not resolve` | **0 occurrences**, on a positively-controlled substring reader. Abort did **not** fire. | n/a — clear |
| 4 | `TASK-1474` the descent ran (3/4 = didn't run · **13/14 = PASS** · 47+ = leaked) | the retarget count | **`14 focus stop(s)`** = 4 pre-existing + 10 `SlotButton`s ⇒ **PASS**. `WARN-2`'s confound closed explicitly: the `DeckBar`-unbound **Error** printed **0** times (positively-controlled reader), so the 14 is not a coincidence of a different path. | pass |
| 5a | `TASK-1478` list stop count = **28** (27 rows + `CloseButton` last) | retarget line | `menu nav target registered -> 'SiegeControlsHelpWidget_0' (registered screen), **28 focus stop(s)**, 1 screen(s) registered.` `23.52.07:483`; `[ControlsHelp] Rebuilt 27 of 27 registry rows` | pass |
| 5b | `TASK-1478` **no double-step** (P6-a: two `focus moved` lines for one press ⇒ `VERIFY-FAILED`) | one line per press | **10 presses → exactly 10 `focus moved` lines**, `0→1→2→3→4→5→6→7→8→9→10 of 28 ('RowButton')`, each preceded by exactly one `HandleMenuDown() entered`. **No index repeated twice in a row.** Reproduced identically in attempt 2. | pass |
| 5c | `TASK-1478` detail stop count = **2** = `{BackButton (stop 0), DetailScrollButton}` | retarget on entering detail | `menu nav target registered -> 'SiegeControlsHelpWidget_0' (registered screen), **2 focus stop(s)**` then `MoveFocus(+1): focus moved **0 -> 1 of 2** ('DetailScrollButton')` ⇒ `BackButton` **still stop 0**, new stop is last. Exactly `TASK-1484`'s predicted delta, not a regression. | pass |
| 5d | `TASK-1478` 🚨 **BLOCKER-class: `IA_ControlsHelp` must still close the overlay FROM THE DETAIL PAGE** | log + pixels | pressed from the detail page at PIE `t=403.3 s`: `[ControlsHelp] Overlay closed.` `23.57.27:469`, and the frame shows the live match restored (hero, castle gate, HUD "Gold: 136") — mean_luma **50 → 136**. **The documented promise holds.** | pass |
| 5e | `TASK-1478` the transient `registered → unregistered` pair on close-from-detail | — | observed, **expected, not filed** (per P6). | n/a |
| 6a | `TASK-1482` victory screen registers with **1 stop**, `Btn_Jump` | retarget line, read for the **parenthetical** not just the integer | `menu nav target registered -> 'WBP_VictoryScreen_C_0' (**registered screen**), **1 focus stop(s)**, 2 screen(s) registered.` `23.54.48:043`. **NOT** `(default: WBP_MainMenu)` ⇒ the registration **took**. Independently repeated: `WBP_VictoryScreen_C_1 (registered screen), 1 focus stop(s)` `23.59.24:720`. | pass |
| 6b | `TASK-1482` the **affirmative** press discriminator | `IA_MenuAccept -> OnClicked.Broadcast()` | `IA_MenuAccept -> OnClicked.Broadcast() on '**Btn_Jump**' ("**Play Again**").` `23.55.21:570`, followed by the teardown `menu nav target unregistered -> …`. **No `HandleMenuAccept` decline line ⇒ the self-driving flag did NOT come back.** The match did reset. | pass |
| 7a | `TASK-1484` the machine limb — the offset moves | the shipped `Log` line, read **whole** | `[ControlsHelp] Detail scroll on '**Cards.StackUpgrade**': offset **0.0 -> 591.0** (end **1166.4**, step **591.0**, view **695.2**) - **MOVED**.` `23.52.46:670`; **reproduced byte-identically** in attempt 2 at `23.58.06:616`. `end 1166.4 ≠ 0` ⇒ this is **not** the short-page correct-negative. Reached entirely by agent verbs (`IA_MenuDown` → `DetailScrollButton`, `IA_MenuAccept` → press). | pass |
| 7b | `TASK-1484` 🚨 **P8-b — the PHOTOGRAPH, not the predictor** | before/after frames of the same overflowing page | **the displayed prose did not change.** See §2. | **not witnessed** |

---

## 2. P8-b — the photograph, and what it actually shows

`SScrollBox` keeps `DesiredScrollOffset` and `ScrollPanel->PhysicalOffset` in two fields and `GetScrollOffset()` returns the first (`SScrollBox.cpp:496-499`). So the sitting owed a picture. Here it is, and it is the most important single finding of this run.

**Two independent runs, same page, same result.** At `DesiredScrollOffset` **0.0** and at **591.0** the `Cards.StackUpgrade` detail page is visually indistinguishable — the first paragraph of `DetailBodyText` ("*Hover one of your OWN buildings while holding the card that built it and the placement outline turns BLUE…*") sits at the top of the box in **both** frames. Attempt 2's two captures returned **bit-identical** statistics (mean_luma 50, pct_near_black 3.743131353965473e-05 on both).

**But the reason is not what P8-b anticipated, and it is checkable from source rather than from any instrument.** The source's `Cards.StackUpgrade` `Row.Detail` string ends at `SiegeControlsHelpWidget.cpp:785-786` with *"The size you dial in with the wheel applies to what you PLACE, not to what you GROW: an upgrade keeps the building's existing width and length."* — **and that is the last visible body line in every frame.** Its three `RelatedActionIds` (`Cards.Play`, `Cards.PlacementResize`, `Cards.Cancel`) are all three rendered, ending with *"Nothing has been spent at the moment you cancel…"*, with blank space beneath before the two buttons.

⇒ **The entire page is displayed. On a 1280×725 viewport this page has no fold.** There is no hidden text for a scroll to reveal, so the press could not produce a visible difference — the stimulus was void, not the feature.

The box's geometry corroborates that the *arithmetic* claims an overflow that is not painted: `DetailScrollBox` measures `abs_h = 466.26` screen px ÷ DPI `0.670639` = **695.3 logical px**, matching the log's `view 695.2` exactly; `end 1166.4` therefore asserts content of ~1861.6 logical px in that window — yet nothing is clipped or hidden on screen.

**Controls, and whether they discriminated:**
- **Camera — discriminated ✅.** The same tool, same layer, same screen family reported mean_luma 50 → 136 when the overlay actually closed, and cleanly showed the list→detail transition. It can see UI change; it saw none here.
- **Mouse wheel (the human path) — did NOT discriminate ❌.** Two wheel events delivered at screen point (1927.5, 1230.0) — inside the box rect (1325.06–2529.94, 998.42–1464.68) — reported `handled: true`, and the prose still did not move. So the non-movement is **not specific to the agent path**. ⚠️ The `ui_perform`/`ui_snapshot` **selector did not narrow**: it resolved to the root `SiegeControlsHelpWidget_0`, not to `DetailScrollBox`, so this control is weaker than intended and I am not resting a verdict on it.
- **`ui_snapshot` geometry — ambiguous ❌.** At offset 591 `DetailBodyText.abs_y = 602.099`, which is *exactly* `998.42 − 591×0.670639 = 602.1` (i.e. consistent with the paint having followed). But sibling nodes in the same box report `abs_x: 15.02`, which cannot be an absolute screen coordinate ⇒ these numbers are panel-local for scroll-box children and cannot settle the question either way.

⇒ Because the mandated page turned out to have **no text below the fold**, this sitting **could not witness** Jonathan's acceptance sentence. That is why `TASK-1484` is `MEASURED` and **not** `VERIFY-FAILED`: every number the row shipped is confirmed and reproducible, the ring change 1→2 is verified, nothing is observed broken — but the pixel claim remains unwitnessed, and I will not fail a row on a test whose premise did not hold.

---

## 3. Two instruments I refused to trust, and one press I refused to make

**(a) `TASK-1434`'s "focused on open" — instrument not validated for this class.** `ui_snapshot`'s `focused` field has a positive control in this very session (`DetailScrollButton: focused: true`), **but that control does not transfer**: a `UButton`'s focus lives on the `SButton` itself, whereas a `UEditableTextBox` delegates focus to an inner `SEditableText`, so `UWidget::HasKeyboardFocus()` on the outer widget reads `false` even when the box has focus. My typed-character fallback is also void — `ui_perform`'s `type` step delivered at pointer (2898, 1289), **outside** the console backdrop (1330–2524, 1466–1560). And `TASK-1434`'s own row header warns explicitly against inheriting a late-sampled negative here; my samples were ~40 s and ~100 s after open. **I could not prove the instrument can say YES for this widget class ⇒ `UNOBSERVABLE`, scored as neither pass nor fail.**

**(b) The deck-builder tile press — declined on purpose.** The abort condition did not fire, so I was permitted to proceed. I still did not press `IA_MenuAccept` with a tile focused, because `AcceptFocusedCard` **mutates Jonathan's `deck1` and auto-saves it to his profile**. The gate lines already establish `TASK-1471`'s deliverable directly (the generic ring presses nothing, on three verbs, by name, while the screen stays registered with 14 stops counted), so the mutation would have bought nothing the log did not already say. I exercised the gate with `IA_MenuDown`/`IA_MenuUp` only. **His deck is untouched.**

---

## 4. Evidence (promoted)

All under `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\.claude\pipeline\playtest-evidence\2026-09-25\`:

- `VER-TASK-1436-controls-help-list-page.png` — the controls-help list open over a live match; 27 rows in category groups, ring live.
- `VER-TASK-1436-a1-detail-scroll-before.png` — attempt 1, `Cards.StackUpgrade` detail page at offset **0.0**; body starts "Hover one of your OWN buildings…"; **"Back to the controls list"** with the **new** "Scroll down — back to the top at the end" button **directly beneath it**.
- `VER-TASK-1436-a1-detail-scroll-after.png` — attempt 1, same page at offset **591.0**; **prose identical, first paragraph still at top.**
- `VER-TASK-1436-a1-detail-scroll-at-end.png` — attempt 1 third press; ⚠️ **contaminated** — the press was stolen by the victory screen that had opened, so this frame is a second view at offset 591, not at the end. Kept per `VER-§1` cl. 6.
- `VER-TASK-1436-a2-detail-scroll-before.png` / `...-a2-detail-scroll-after.png` — attempt 2, clean re-run, offsets 0.0 → 591.0; **bit-identical luma statistics.**
- `VER-TASK-1436-a2-detail-scroll-mousewheel-control.png` — the human-wheel control at full 1280×725; prose still unmoved; the "Defeat"/"Play Again" screen visible behind the overlay.
- `VER-TASK-1436-controls-help-toggle-close-from-detail.png` — **the BLOCKER-class limb**: after `IA_ControlsHelp` from the detail page, the live match is back (hero at the castle gate, "Gold: 136", card hand).
- `VER-TASK-1436-assistant-console-open.png` — the console open in a clean live match.
- `VER-TASK-1436-assistant-console-open-focus.png` — ⚠️ **named before the capture returned**: this frame actually shows the console **failing to open** in the contaminated post-match state (Defeat screen still latched). Cited here for accuracy, not as a pass.
- `VER-TASK-1436-deck-builder-open.png` — the deck builder opened from the main-menu ring.

Not promoted (stays in `Saved/`, gitignored): `Saved/AuraVerify/mainmenu-labels.png` — used only to read the seven main-menu labels.

---

## 5. 🧑 FOR JONATHAN — the one thing on this row that is yours and not mine (P9 / cl. 3(b))

> **Open the Tab screen and go into a row's detail page — there is a new button under Back now; does it look right to you, and does pressing it page the text down? Two things you should know before you answer: a SMALLER affordance (e.g. a compact `▼`) is STILL AN OPEN OPTION, and the button COSTS you about one button-height plus 34 px of visible prose on every detail page.**
>
> (The handoff `§3` line 110 told you glyph-only is *"barred by the stepper-pair discriminator"* — **that is false and you are owed the correction**: only the **bare** `<` and `>` are barred, so a compact `▼` would pass both discriminators. And the box's slot is `Fill`, so that prose cost applies to **every** detail page — a trade nobody had shown you, and a trade you cannot decline if you are not shown it.)

**The machine limb is the OFFSET; the APPEARANCE is yours alone.** My reading on the offset says **nothing** about whether you want that button there. The new button is clearly visible in `VER-TASK-1436-a1-detail-scroll-before.png` and `VER-TASK-1436-a2-detail-scroll-before.png`, directly below "Back to the controls list", reading "Scroll down — back to the top at the end".

Second, separate ask arising from §2: **on `Cards.StackUpgrade` the whole page already fits on screen with room to spare.** If that is true of every detail page at 1280×725, the scroll affordance is paying its prose cost for a fold that does not exist — which may change your answer.

---

## 6. How many of your ten screens are now runtime-verified — plainly

**Eight of the ten carry a `VERIFIED` runtime verdict.** Six from the earlier sittings (main menu, Settings, Graphics, Login, deck builder, Session — `TASK-1413` / `1421` / `1427`), plus **two earned today**: the **in-match regression fence** and the **victory screen**. Of the six prior ones I incidentally re-observed two live in this sitting and both still hold — the main menu (`ApplyInitialFocus … 'Button_0' ("Play (vs Bot)"), index 0 of 7`, then a clean forward walk 0→6 and a clean reverse walk 6→2) and the deck builder (14 stops).

**The ninth, controls help, is `VERIFIED` for navigation** — 28 list stops, 2 detail stops, one focus move per press with no double-step, and the toggle key still closing from the detail page — **but its scroll residual (`TASK-1484`) is `MEASURED`, not verified.** You ruled *"it isn't navigable until an agent can scroll"*, so **whether that counts screen 7 is your call and I have not made it for you.** A runtime verdict now exists, which is the condition your ruling attached.

**The tenth, the assistant console, is `MEASURED`**: it opens, closes, correctly refuses to register a nav target, and hands focus back to the game (proven by a hero that moved 701.8 uu on the next injected input) — but "text box focused on open" could not be read by any instrument I could positively control for that widget class.

**Nothing in this sitting was observed failing.** No subject returned `VERIFY-FAILED`, so no row bounces and the commit is not blocked by me.

---

## 7. Hypotheses (NOT verdicts)

- **H1 — the scroll box may not be clipping.** `DetailScrollBox` reports `end 1166.4` (≈1861.6 logical px of content in a 695.2 px window) while painting its whole content unclipped with blank space below. If so, a genuinely long page would draw *past* the box rather than become scrollable, and `AdvanceBodyScroll`'s truthful `MOVED` would stay invisible. **This is a hypothesis about a pre-existing widget, not about `TASK-1484`'s diff** — `AdvanceBodyScroll`'s clamp, wrap, step (0.85 × 695.2 = 591.0) and read-back all behaved exactly as specified.
- **H2 — the mouse-wheel path may share H1.** The human wheel produced the same non-movement. If H1 holds, the row's founding premise (*"a human wheel-scrolls it to read it"*) may itself never have been pixel-tested.
- **H3 — `NIT-5`'s ordering is real but did not bite the clean case.** The first victory screen registered *and* took the press. The second (`WBP_VictoryScreen_C_1`) declined with `IA_MenuAccept declined: focused stop 'None' (none focused)` — but I had a controls-help overlay open over it at the time, i.e. **my own contamination**, not a clean defect. Worth a look by whoever owns `TASK-1480 (g)`.

---

## 8. Not examined / limitations this run

- **Settings, Graphics, Login, Session** were not re-tested; they stand on `TASK-1413` / `1421` / `1427`. `R4a`/`R4b`/`R7`'s Graphics ring discriminators were therefore **not exercised** — no Graphics screen was opened, and I pressed **no** `IA_MenuLeft`/`IA_MenuRight` anywhere, so `R4b`'s stepper trap was never sprung and no `of 21` appears in this run.
- **`TASK-1434` "focused on open"** and **"Accept via `IA_MenuAccept`"** — `UNOBSERVABLE`, reasons in §3(a) and §1 row 2f.
- **`TASK-1471`'s positive half** (tile focused → Accept picks the card) — deliberately not pressed; §3(b).
- **`TASK-1484`'s pixel limb** — not witnessed; §2. A future sitting wanting this should first find a detail page that **visibly** overflows at the test resolution, or reduce the window height, rather than trusting `end > 0`.
- **The automation suite is NOT cited for controls help or for the victory screen.** Coverage was re-measured at **0 occurrences** across the whole `Tests/` tree for both. `564 started / 0 failed` is a regression check on everything else and says nothing about either screen; **these verdicts are the only runtime evidence either screen has.** The suite *is* citable for limbs 2/3 as mechanisms on synthetic fixtures, the four-class vocabulary, and the live main menu's seven.
- **Match-end fired twice on its own** while the overlay work was in progress (I was idle in the UI for minutes). That gifted the victory subject two clean registrations, but it also contaminated attempt 1's third scroll press and the first assistant-console attempt. Both were re-run clean.
- **Tooling defects found, worth recording:** `ui_snapshot` / `ui_perform` **selectors do not narrow** on this build (`by:name`, `index_path` and `name_path` all resolved to the root) — this forced full-tree snapshots that overflowed the tool limit and weakened the wheel control. `unreal_inspector.grep` silently returned empty for patterns containing escaped parens/alternation; caught by a positive control, not by the tool.
- **PIE was run 3 times; the editor was never restarted, never closed, and `L_Arena` was restored.** No commit, no stage, no push.
