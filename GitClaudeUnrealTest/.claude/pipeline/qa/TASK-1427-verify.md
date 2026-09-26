Verdict: VERIFIED
# Verification — TASK-1427 [WAVE-D1-VERIFY]

**Editor/Aura state:** Aura `editor_connected`. Editor identified **IN-PROCESS**: `os.getpid()` returned
**18236** from inside the editor that answered the MCP call (matches the dispatch); `sys.executable` =
`C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe` (the **GUI** editor, no `-game`);
`get_game_world()` was **None** before I started, so no `-game` sitting of 🧑 Jonathan's existed.
⚠️ **`FCommandLine::Get()` returned the EMPTY STRING again**, so the `SC-§118` identification rests on
in-process PID + exe path + editor-world presence, **not** on a command-line string — declared, not glossed
(the same limit `TASK-1413` and `TASK-1421` hit).
**Binary confirmed in-process:** `Binaries/Win64/UnrealEditor-GitClaudeUnrealTest.dll` =
**9,890,816 B @ 2026-09-25 02:42:30** — the post-Row-A link, exactly as the dispatch declared.
Map: editor world was already `/Game/Maps/L_MainMenu` (left there by `TASK-1421`); **I loaded no level.**
PIE mode: standalone, 1 client, 1280×720 requested → viewport **1280×725**. `is_pie_active` read
**`is_active: false` before I started**, so the session was mine to start and mine to stop; stopped at
`pie_time_seconds: 420.91`, `is_active: false` confirmed after. **Editor PID 18236 left UP.**
**Attempts used: 1 of 3.** PIE wall time ≈ 6.9 min (t=5.63 s → t=420.91 s), 2026-09-25 ≈17:03–17:11 local.
🧑 Jonathan's explicit go was carried in the dispatch (*"Run both now."*). I sat the **second** of the two
sittings; Settings / Graphics / Login were **not touched** (they are `TASK-1421`'s and are out of scope).
Residue declared by `TASK-1421` (`ViewDistanceQuality` / `ShadingQuality` = 3) was left alone.

Pre-registered falsifiers were written to disk **before PIE started** (while `is_pie_active: false`):
`C:\Users\wesel\AppData\Local\Temp\claude\C--GitProjects-GitHub-GitClaudeUnrealTesting-GitClaudeUnrealTest\d36961fb-c9ad-4c45-b8a7-d2b80c7a159c\scratchpad\TASK-1427-prereg.md`.
Nothing below was decided after a number was seen.

---

## 🚨 INSTRUMENT LIVENESS — MEASURED FROM THE DECLARATION FIRST, BECAUSE AN EMPTY LOG READS AS A ZERO

R7 asked for three `Log … <verbosity>` console writes. **They were not issued and could not be: I hold no
console-command verb, and `execute_unreal_python_readonly` is read-only.** So I measured the compiled
defaults, as `TASK-1413` did:

| category | declaration | default verbosity | consequence |
|---|---|---|---|
| `LogSiegeMenuInput` | `SiegeMenuInputSubsystem.h` (`, Log, All`) | `Log` | ✅ printed — 156 lines in this log file |
| `LogGitClaudeUnrealTest` | `Source/GitClaudeUnrealTest/GitClaudeUnrealTest.h:8` — `DECLARE_LOG_CATEGORY_EXTERN(LogGitClaudeUnrealTest, Log, All)` | **`Log`** | ⛔ **`Verbose` lines DID NOT PRINT** |

⛔ **THE DECK BUILDER'S PER-PRESS INSTRUMENT IS `UNOBSERVABLE` THIS RUN, AND IS NOT REPORTED AS A ZERO.**
`DeckBuilderWidget.cpp:2078` (`RouteMenuNavKey: %s -> '%s' (FocusedCardIndex=%d, grid live=%s)`) is
**`Verbose`**; the category's *declared default is `Log`*, so the line cannot fire and I could not raise it.
**Zero `RouteMenuNavKey` lines were observed and that number means nothing.**
✅ **The once-per-open arm line is at `Log` and it printed**, so the lane's existence is proven by a line
rather than by an absence:

```
[2026.09.25-17.07.25:911][986]LogGitClaudeUnrealTest: UDeckBuilderWidget::BindMenuNavActions: 6 IA_Menu*
  action(s) bound (Started) on 'PlayerController_0' [IA_MenuUp, IA_MenuDown, IA_MenuLeft, IA_MenuRight,
  IA_MenuAccept, IA_MenuBack]; every IA_Menu* asset resolved. …
```

⇒ **The deck-builder criterion was measured through a LIVE REFLECTION READ of `FocusedCardIndex`
(`get_widget_property_in_pie`), not through the log.** That instrument is the one
`DeckBuilderWidget.cpp:2107-2112` itself names as "what the verifier reads".

---

## 🚨🚨 (R1) THE ABORT CONDITION — **IT DID NOT FIRE.** NO `IA_MenuAccept` WAS EVER INJECTED IN THE BUILDER.

- **Pre-flight, before PIE:** `unreal.load_object(None, "/Game/UI/WBP_DeckCardTile.WBP_DeckCardTile_C")`
  returned `/Game/UI/WBP_DeckCardTile.WBP_DeckCardTile_C` — the class resolves offline.
- **In PIE:** the whole `LogGitClaudeUnrealTest` category produced **exactly 3 lines** this session and
  **none** of them is `card-tile class '…' did not resolve`. The Warning is at `Warning` verbosity, which
  prints at the declared default, so **its absence is a real absence.**
- ⛔ **I injected `IA_MenuAccept` exactly three times all session — on `Button_3` ("Multiplayer"), on the
  session menu's `BackButton`, and on `Button_2` ("Deck Builder"). ZERO Accepts were injected while the
  deck builder was open.** 🧑 His deck was not touched; `deck1` still reads `Deck: 51/50` at
  `Overlay_19/VerticalBox_0/TextBlock_6`, unchanged.

---

## §1 — PER-SUBJECT VERDICT TABLE (the commit host reads THIS, not line 1)

| TASK | subject | verdict | the one sentence |
|---|---|---|---|
| **TASK-1423** | Deck builder | **VERIFIED** | `FocusedCardIndex` read **`-1` at PIE t=245.057 s → `0` at PIE t=245.790 s** across one injected `IA_MenuDown` at t=245.074 s — `TASK-1306`'s recorded `-1` is overturned — and the grid→bar crossing landed focus on `deck1`'s `SlotButton`. ⚠️ Its **per-press log limb is `UNOBSERVABLE`** (Verbose, un-raisable). |
| **TASK-1425** | Session menu | **VERIFIED** | Registered **4 focus stop(s)** exactly as predicted, walked `Host → Join → Back → Address → wrap`, and `Back` re-armed a **fresh** `WBP_MainMenu_C_1` on its **TOP option** 183 ms later, still there 60.8 s later. |
| **TASK-1469** | Row A (limb 1) | **VERIFIED** | 🚨 `MoveFocus(+1): focus moved **3 -> 0** of 4 ('**HostButton**')` — the walker read `Current = 3` **while the ring was inside a `UEditableTextBox`**. Limb 1's text-box payout has now been exercised at runtime for the first time in this project's history. |

**Row-level line 1 = `VERIFIED`**: every acceptance line with a reachable runtime signal was observed
passing; nothing was observed failing. The two `UNOBSERVABLE` sub-limbs (the deck-builder per-press log,
and the pixel limb) are carved out explicitly rather than folded into a pass.

---

## §2 — ACCEPTANCE LINES → OBSERVATIONS

| # | acceptance line | observable chosen | observed (quoted values, evidence path) | pass/fail/unobs |
|---|---|---|---|---|
| 1423-a | `FocusedCardIndex != -1` after `inject_input_action IA_MenuDown` (direct re-run of `TASK-1306`'s `-1`) | live reflection read of `UDeckBuilderWidget::FocusedCardIndex` on `WBP_DeckBuilder_C_0` | `{"name":"FocusedCardIndex","found":true,"type":"int32","value_number":-1}` at **PIE t=245.057 s**; `IA_MenuDown` injected at **PIE t=245.074 s**; `{"value_number":0}` at **PIE t=245.790 s**. Evidence: `…/VER-TASK-1427-deckbuilder-focusedcardindex-after-first-injected-down.png` | **pass** |
| 1423-b | the 2-D grid ring actually steps | same property, four more presses | `0` → `IA_MenuRight` (t=265.330) → **`1`** (t=266.063) → `IA_MenuDown` (t=266.080) → **`14`** (t=266.813) → `IA_MenuUp` (t=266.830) → **`1`** (t=267.547). Row width **13**, clean round trip | **pass** |
| 1423-c | "cross to the deck bar and read a stop there" | `IA_MenuBack` then `ui_snapshot` | `IA_MenuBack` at t=277.566 ⇒ `FocusedCardIndex` back to **`-1`** (t=278.483) **and** `WBP_DeckBuilder_C_0 → Overlay_19/DeckBar/DeckSlotEntryWidget_0/OutlineBorder/SlotButton` (label `"deck1"`) reads **`focused: true`** while the other nine `SlotButton`s read `false` (t=278.499). Evidence: `…-deckbuilder-grid-to-deckbar-crossing-slotbutton-deck1-focused.png` | **pass** |
| 1423-d | (R2) subsystem stop count is **3–4**, not `1 + A` | `LogNavTargetRetarget` line + census | `menu nav target registered -> 'WBP_DeckBuilder_C_0' (registered screen), **4 focus stop(s)**, 1 screen(s) registered.` — inside the predicted band. **`Btn_Jump` measured excluded**, anchored by path: `WBP_DeckBuilder_C_0 → Overlay_19/SizeBox_0/Btn_Jump` reads `visibility: Collapsed`, `realized: false` | **pass** |
| 1423-e | the per-press instrument | `LogGitClaudeUnrealTest` at `Verbose` | category default is **`Log`**; no console verb held ⇒ the line cannot fire. **Zero lines seen and the zero is meaningless** | **unobs** |
| 1425-a | 4 stops, ring order `Host → Join → Back → Address → wrap` | `MoveFocus` walk lines | all four lines read `of 4` and named, in order, `JoinButton`, `BackButton`, `AddressTextBox`, `HostButton` (§3) | **pass** |
| 1425-b | `Back` ⇒ ring on the **TOP option** of the **NEW** main-menu instance, sampled **≥ 0.5 s** after Back (R3) | `ApplyInitialFocus` line + a walk read 60.8 s later | `ApplyInitialFocus: focus placed on the TOP option 'Button_0' ("Play (vs Bot)"), index 0 of 7, in menu instance '**WBP_MainMenu_C_1**'` at **+183 ms**; then at **+60.8 s** (PIE t=181.258) a Down read `focus moved **0** -> 1 of 7 ('Button_1')`, i.e. the ring was **still on stop 0** at a sample 121× the 0.5 s floor. Evidence: `…-mainmenu-reentry-top-option-after-session-back.png` | **pass** |
| 1425-c | negative control: a **byte-identical** tree after an injected Down is a `VERIFY-FAILED` (R5) | `ui_snapshot` before/after | **NOT byte-identical.** Open: `CanvasPanel_52/HostButton` `focused:true`, all others `false` (t=36.978). After one injected Down: `CanvasPanel_52/JoinButton` `focused:true`, `HostButton` `false` (t=61.151). **The control discriminated.** | **pass** |
| 1425-d | ring order vs visual order | `geometry.abs_y` in the same census | visual top→bottom is `AddressTextBox` (1100.57) · `HostButton` (1162.27) · `JoinButton` (1222.62) · `BackButton` (1289.69); ring order is `Host(0) → Join(1) → Back(2) → Address(3)`. ⇒ **they still differ ⇒ `TASK-1473`'s reorder has NOT landed and the witness is intact.** Known/accepted, **not re-filed** | **pass (as accepted)** |
| 1469-a | limb 1: `Down` ×4 must end `3 -> 0 ('HostButton')`, not `0 -> 1 ('JoinButton')` (R4) | `MoveFocus` walk line, verbatim | `MoveFocus(+1): focus moved **3 -> 0** of 4 ('**HostButton**').` — **the sound-walker shape** | **pass** |
| 1469-b | limb 1 diagnostic half: no false `'None' (none focused)` / `declined` at a ring that is visibly in a field | log census | **`'None' (none focused)`: 0 occurrences in the whole log file.** `declined: no focus stop holds focus`: **exactly 1**, on the deck builder with a **card tile** focused — see §5, it is correct, not the false negative Row A fixed | **pass** |

---

## §3 — 🚨 THE MEASUREMENT THREE ATTEMPTS OWED: THE FIRST REACHABLE TEXT-BOX STOP IN THIS PROJECT'S HISTORY

The session menu opened with the ring already on stop 0. Four injected `IA_MenuDown`, quoted **verbatim**:

```
[2026.09.25-17.04.57:060][963]LogSiegeMenuInput: [USiegeMenuInputSubsystem] MoveFocus(+1): focus moved 0 -> 1 of 4 ('JoinButton').
[2026.09.25-17.04.57:499][973]LogSiegeMenuInput: [USiegeMenuInputSubsystem] MoveFocus(+1): focus moved 1 -> 2 of 4 ('BackButton').
[2026.09.25-17.04.58:673][981]LogSiegeMenuInput: [USiegeMenuInputSubsystem] MoveFocus(+1): focus moved 2 -> 3 of 4 ('AddressTextBox').
[2026.09.25-17.04.59:138][999]LogSiegeMenuInput: [USiegeMenuInputSubsystem] MoveFocus(+1): focus moved 3 -> 0 of 4 ('HostButton').
```

**The fourth line is the whole measurement.** The 4th press started with the ring inside
`AddressTextBox` (a `UEditableTextBox`, stop 3). `GetFocusedNavStop()` returned that box, so `MoveFocus`
read `Current = 3` and wrapped to `HostButton`. The **defective** shape `0 -> 1 of 4 ('JoinButton')` — the
cold branch that `qa/TASK-1426.md` WARN-1 predicted would make `HostButton` keyboard-unreachable —
**did not occur.** Corroborated by census, anchored by object path:

| instant | `CanvasPanel_52/HostButton` | `…/JoinButton` | `…/BackButton` | `…/AddressTextBox` |
|---|---|---|---|---|
| open, PIE t=36.978 | **true** | false | false | false |
| after Down #1, t=61.151 | false | **true** | false | false |
| after Down #3, t=62.598 | false | false | false | **false** ⚠️ |
| after Down #4, t=63.245 | **true** | false | false | false |

⚠️ **THE ALL-FALSE ROW IS THE PREDICTED INSTRUMENT ARTEFACT, NOT A FAILURE — AND THE DISPATCH WARNED ME
ABOUT EXACTLY THIS.** With the ring in the box, **every node censuses `focused: false`**, because
`SEditableTextBox::OnFocusReceived` forwards focus to its inner `SEditableText` and `UWidget::HasUserFocus`
is exact-widget. That is the very bug Row A repaired *inside the subsystem* — `ui_snapshot` does not route
through `GetFocusedNavStop()` and so still reads the old way. ⇒ **I discriminated with the walk, not with
the census field**, exactly as instructed, and the walk says the ring was on stop 3.
⛔ **This all-false reading is NOT a `VERIFY-FAILED` and must not be cited as one.**

---

## §4 — THE SESSION RETURN LEG (spec (2)), AND ONE THING THE CENSUS COULD NOT DO

```
[2026.09.25-17.05.58:815][194] IA_MenuAccept -> OnClicked.Broadcast() on 'BackButton' ("Back").
[2026.09.25-17.05.58:816][194] menu nav target unregistered -> 'WBP_MainMenu_C_1' (default: WBP_MainMenu), 7 focus stop(s), 0 screen(s) registered.
[2026.09.25-17.05.58:816][194] UnregisterMenuNavTarget('WBP_SessionMenu_C_0'): not registered (already unregistered, or never was) — no change.
[2026.09.25-17.05.58:998][205] ApplyInitialFocus: focus placed on the TOP option 'Button_0' ("Play (vs Bot)"), index 0 of 7, in menu instance 'WBP_MainMenu_C_1'.
```

The instance is **named in the log** (`WBP_MainMenu_C_1`), which is a stronger anchor than a name in a
snapshot. `TASK-1400`'s 0.2 s re-arm fired at **+183 ms** and nothing displaced it.

🚨 **AND THE ONE THING I COULD NOT DO, SAID PLAINLY.** At PIE t=121.947 — ≈1.5 s after Back — `ui_snapshot`
**refused**: `'WBP_MainMenu' matched 2 widgets: WBP_MainMenu_C, WBP_MainMenu_C. Pass a more specific class
name.` ⇒ **two `WBP_MainMenu_C` objects were alive at that instant.** I therefore have **no census-side
read of the fresh instance**; the re-entry verdict rests on the two log lines above plus the t=181.258
walk line. Three read-only Python attempts to resolve either instance by object path failed (widgets are
not outered where I guessed). **Whether `WBP_MainMenu_C_0` was still parented or merely an orphan awaiting
GC is UNRESOLVED this run** — see §7.

---

## §5 — THE ONE `declined` LINE, AND WHY IT IS NOT A ROW-A REGRESSION

```
[2026.09.25-17.08.27:724][374] IA_MenuLeft/IA_MenuRight -> StepFocusedStop(+1) entered.
[2026.09.25-17.08.27:724][374] StepFocusedStop(+1) declined: no focus stop holds focus.
```

Sitting one reported **zero** `declined` lines. This one fired on the deck builder, on the `IA_MenuRight`
press, while a **card tile** held Slate focus. A card tile is a `UUserWidget` — **not** one of the four
admitted stop classes, and **not** a descendant of any stop — so `GetFocusedNavStop()` correctly returns
null and the refusal is **honest**. Row A's limb 1 repairs the case where a *stop's own descendant* holds
focus (the text box); it does not, and should not, claim a tile. ⛔ **Do not read this line as the false
negative Row A fixed.** The proof it is not: `FocusedCardIndex` moved `1 → 14 → 1` across the very presses
either side of it, so the grid was demonstrably armed and being driven.

---

## §6 — PREDICTED vs ACTUAL STOP COUNTS, AND THE DECK-BUILDER RING IDENTITY

| screen | predicted (source) | actual, measured | agree? |
|---|---|---|---|
| Main menu | **7** (`TASK-1469` §2/§3 non-regression) | **7** — every main-menu line reads `of 7`, `Button_0`…`Button_6`; census shows all seven `Visible`/`enabled` | ✅ |
| **Session** (`WBP_SessionMenu_C_0`) | **4** (`TASK-1469` count table; `qa/TASK-1426.md` read it live off the `.uasset`) | **4** at registration **and** `of 4` on all six walk lines; census node_count **13**, matching `qa/TASK-1426.md` exactly | ✅ |
| **Deck builder** (`WBP_DeckBuilder_C_0`) | **3–4** (`TASK-1469` count table: 3 details closed / 4 details open) | **4** at registration **and** `of 4` on all three walk lines | ✅ within prediction |

**Deck-builder ring identity — two indices MEASURED, two DERIVED, and the difference is marked.**
The subsystem named two of its four stops in the walk lines: `focus moved 0 -> 1 of 4 ('**Button_1**')`
(twice) and `MoveFocus(-1): focus moved 0 -> 3 of 4 ('**Button_3**')`. Against the live tree those bare
names resolve as: `Overlay_19/VerticalBox_0/HorizontalBox_1/Button_1` — label **`"Reset to Default"`** —
and `Overlay_19/Button_3` — label **`"Exit"`**. Depth-first pre-order over the builder's own tree then
**derives** stop 2 as `Overlay_19/VerticalBox_0/Button_2` and stop 0 as a `UButton` inside
`Overlay_19/VerticalBox_0/HorizontalBox_0` (consistent with `qa/TASK-1424.md` WARN-3's `Btn_DetailsClose`,
but ⛔ **I did not open that subtree and did not measure it — stops 0 and 2 are DERIVED, not observed**).
⇒ `qa/TASK-1424.md` **WARN-2 is CONFIRMED AT RUNTIME, ANCHORED BY PATH THIS TIME**: `Btn_Jump` is
`Collapsed`/`realized: false` at `WBP_DeckBuilder_C_0 → Overlay_19/SizeBox_0/Btn_Jump`, so the `1` of
`1 + A` is genuinely absent. ⚠️ Note honestly that **`4` is numerically indistinguishable from `1 + A`
with `A = 3`** — the count alone never separated them (`qa/TASK-1424.md` NIT-1 said so); it is the
**path-anchored census of `Btn_Jump`** that separates them, not the number.

---

## ⛔ ANCHORING DECLARATION — SAID IN THE REQUIRED WORDS (R2b)

**Anchored by object path** (root + full `name_path`, read from `ui_snapshot`):
- `WBP_SessionMenu_C_0` → `CanvasPanel_52/{HostButton, JoinButton, BackButton, AddressTextBox,
  StatusTextBlock, ErrorTextBlock, BackdropBorder, TitleText}` — 13 nodes, root included.
- `WBP_MainMenu_C_0` (the pre-session instance) → `Overlay_19/VerticalBox_0/Button_0` … `Button_6`,
  and `Overlay_19/SizeBox_0/Btn_Jump` (`Collapsed`).
- `WBP_DeckBuilder_C_0` → `Overlay_19/SizeBox_0/Btn_Jump` (`Collapsed`) ·
  `Overlay_19/DeckBar/DeckSlotEntryWidget_0…9/OutlineBorder/SlotButton` ·
  `Overlay_19/VerticalBox_0/HorizontalBox_1/Button_1` ("Reset to Default") ·
  `Overlay_19/VerticalBox_0/Button_2` · `Overlay_19/Button_3` ("Exit") ·
  `Overlay_19/VerticalBox_0/TextBlock_6` (`"Deck: 51/50"`).

⛔ **UNANCHORED — stated in those words:**
1. **The four `MoveFocus` stop names on the session screen are `UNANCHORED` at the object-path level.**
   `MoveFocus` prints a bare `GetName()`. What they *are* anchored by is weaker but explicit: the
   registration line naming the active target `'WBP_SessionMenu_C_0'` 26 s before the walk, the invariant
   `of 4` on every line, and a path-anchored `ui_snapshot` of that same root whose four candidate nodes
   carry exactly those four names with exactly those four classes.
2. **The fresh main-menu instance `WBP_MainMenu_C_1` is `UNANCHORED` by census** — see §4. It is anchored
   by the log, which names the instance explicitly, and by nothing else.
3. **Deck-builder stops 0 and 2 are `UNANCHORED` and DERIVED** — see §6.
- ⭐ Collision-free and cited bare by permission: `HostButton`, `JoinButton`, `AddressTextBox`,
  `Btn_DetailsClose`, `DeckBar`, `SlotButton`. ⛔ **`BackButton` is NOT collision-free** (three classes
  carry one) and every use of it in this report is either inside a quoted log line whose active target was
  named one line earlier, or carries its `CanvasPanel_52/` path. ⛔ **`Btn_Jump` is NOT collision-free**
  (nine design-time trees, plus one in `WBP_DeckCardTile`, so 34+1 sit in the builder's rendered tree at
  once) and is cited **only** with its full path `Overlay_19/SizeBox_0/Btn_Jump` under a named root.

---

## §7 — THE PIXEL LIMB (`TWO-LIMB-CONDITIONED-BY-VER-11-CL-9-2026-09-24`)

**STATE limb and PIXEL limb are recorded side by side and are NOT merged.**

- 🤖 **STATE limb — the gate — PASSED** on all three subjects. See §1–§6.
- 🧑 **PIXEL limb — `UNOBSERVABLE`, and the reason is that the four-part bar is NOT SPECCED ON THIS ROW.**
  The row header says so in its own words; cl. 4 forbids me deciding at the keyboard that my capture
  qualifies, and cl. 9(b) makes a ringless capture `UNOBSERVABLE`, never a `VERIFY-FAILED`. **Saying this
  is the deliverable, not a shortfall** (`SC-§50`).
- 🚨 **AND THE STRONGER HONEST LIMIT: I captured four frames to disk but NO IMAGE CONTENT REACHED MY
  CONTEXT.** `capture_pie_frame` returned dimensions and luma statistics only. ⇒ **I make NO claim about
  what is drawn in them** — not "a ring is visible", not "the ring is on `HostButton`". The Evidence
  section reports only what I can attest: the file, its dimensions, and its measured statistics.
  🧑 **Their adjudication is Jonathan's.**
- 🧑 **(2b) *"does it look right?"* remains his, forever.**

---

## Evidence (promoted)

All four written **directly** by `capture_pie_frame` to the dated evidence folder (verified on disk by
size after PIE stopped). ⚠️ **The `-t<time>` token is deliberately omitted** per the dispatch.

- `.claude/pipeline/playtest-evidence/2026-09-25/VER-TASK-1427-session-ring-wrapped-to-hostbutton.png`
  — composited layer, **1280×725**, 613,120 B, PIE `game_time_seconds` **107.522**, frame 1579557,
  `mean_luma` 189, `pct_near_black` 0. Taken with the session menu open and the ring having just wrapped
  `3 -> 0` onto `HostButton`. **I did not see this image.**
- `.claude/pipeline/playtest-evidence/2026-09-25/VER-TASK-1427-mainmenu-reentry-top-option-after-session-back.png`
  — composited, **1280×725**, 626,503 B, PIE t **121.964**, frame 1580287, `mean_luma` 195,
  `pct_near_black` 0. The fresh `WBP_MainMenu_C_1`, ≈1.53 s after `Back`. **I did not see this image.**
- `.claude/pipeline/playtest-evidence/2026-09-25/VER-TASK-1427-deckbuilder-focusedcardindex-after-first-injected-down.png`
  — composited, **1280×725**, 531,286 B, PIE t **246.190**, frame 1587366, `mean_luma` 151,
  `pct_near_black` 4.09e-05. Taken 0.40 s after the read that returned `FocusedCardIndex = 0`.
  **I did not see this image.**
- `.claude/pipeline/playtest-evidence/2026-09-25/VER-TASK-1427-deckbuilder-grid-to-deckbar-crossing-slotbutton-deck1-focused.png`
  — composited, **1280×725**, 521,209 B, PIE t **307.045**, frame 1590693, `mean_luma` 151,
  `pct_near_black` 4.09e-05. After `IA_MenuBack`, with `deck1`'s `SlotButton` holding focus.
  **I did not see this image.**

---

## Hypotheses (NOT verdicts)

1. **Why `MoveFocus` read `Current = 3` in the text box:** limb 1(a)'s second pass
   (`HasUserFocusedDescendants`) answered where `HasUserFocus` could not, because `SEditableTextBox`
   forwards focus to its inner `SEditableText`. **MECHANISM, NOT MEASURED** — I observed the *outcome*
   (`3 -> 0`), not the branch taken. A single-pass fused `||` would produce the same line, so this run
   does **not** discriminate the two-pass shape from a fused one.
2. **Why the census reads all-false with the ring in the box:** `ui_snapshot`'s `focused:` field most
   likely derives from `UWidget::HasUserFocus`, which is exact-widget. **HYPOTHESIS.** It fits every row
   of the §3 table, but I did not read the tool's implementation.
3. **The two live `WBP_MainMenu_C` objects:** most likely the pre-session instance surviving as an orphan
   until GC, since `SessionMenuWidget`'s open path removes it from the parent rather than destroying it,
   and since the subsystem resolved its default target to `WBP_MainMenu_C_1` (not `_0`) one millisecond
   after the Back. **HYPOTHESIS — the alternative (two parented menus stacked) was not excluded** and
   would be a real defect. See "Not examined".
4. **The deck builder's cold branch is load-bearing on purpose.** Every `IA_MenuDown` in the builder made
   the subsystem move its own ring onto stop 1 = `Button_1` ("Reset to Default") *within the press*, and
   `TASK-1423`'s repair then pulled Slate focus back to the card tile — the evidence being that the very
   next press read `Current = 0` (cold) again rather than `1`. ⇒ the repair demonstrably works.
   ⚠️ **But it means the ring transits a destructive button on every press**, which is `qa/TASK-1424.md`
   WARN-1's hazard observed live rather than reasoned about. **I did not test Accept there and I am not
   filing a new defect** — `TASK-1471` owns the structural fix. Routed to the manager, not self-adjudicated.

---

## Not examined / limitations this run

- ⛔ **The pixel limb** — `UNOBSERVABLE`; the four-part bar is not specced on this row, and no image
  content reached my context (§7).
- ⛔ **The deck builder's per-press `Verbose` instrument** — `UNOBSERVABLE`; category default is `Log` and
  I hold no console verb. **Never reported as a zero.**
- ⛔ **`IA_MenuAccept` inside the deck builder** — deliberately never injected (R1). ⇒ **whether Accept on
  a builder stop is safe is NOT tested by this run**, in either direction.
- ⛔ **Deck-builder stops 0 and 2** — derived from depth-first order, not measured; I did not open
  `HorizontalBox_0`, so `Btn_DetailsClose`'s admission is consistent-with, not confirmed.
- ⛔ **The fresh main menu was never censused** (two-instance ambiguity, §4); the re-entry verdict rests on
  log lines. Three read-only object-path lookups failed to resolve any live widget by guessed path.
- ⛔ **Whether `WBP_MainMenu_C_0` was still parented at t=121.95** — unresolved (Hypothesis 3).
- ⛔ **`IA_MenuBack` on the session menu** — not exercised. The subsystem logged it **INERT by design** on
  the deck builder (`implements no ISiegeMenuNavCloseTarget — Back is INERT for it, and no teardown is
  guessed`), which is `TASK-1454`'s row. ⛔ **Not filed as a defect** (R6).
- ⛔ **Sitting one's screens (Settings / Graphics / Login)** — out of scope, not re-verified, not touched.
- ⛔ **The automation suite was NOT cited** anywhere in this report (`TASK-1475` is closing its divergence
  from the walker).
- ⛔ **No code, no asset, no compile, no git, no editor lifecycle action.** Editor **PID 18236 left UP**;
  PIE started by me and stopped by me; `execute_unreal_python_readonly` used read-only only.
  `ViewDistanceQuality`/`ShadingQuality` = 3 left exactly as `TASK-1421` declared.
