Verdict: UNOBSERVABLE
# Verification — TASK-1393 — [MENU-ACCEPT-ROUTE-DISCRIMINATOR]

**OUTCOME LETTER: (c)** — the row's own pre-written third outcome. The absence in 🧑 his sessions is
**NOT a negative**; limb B (his hands) is **STILL OWED**. But the archaeology did real work: it has
**narrowed the four candidate explanations to two**, and the one remaining question is now a **single bit**
(*did he touch the mouse?*).

Editor/Aura state: Aura **connected** (`editor_connected`). Editor identified **by its own command line and
open-stamp**, not by a borrowed sighting: `Saved/Logs/GitClaudeUnrealTest.log` line 1 = `Log file open,
09/21/26 13:00:14` and line 538 = `LogInit: Command Line: ` (**empty** ⇒ a GUI editor, not `-game`,
not a commandlet) — the open-stamp matches the dispatch's censused **PID 26992 up since 2026-09-21 13:00**,
so the live editor and the log I am reading are **the same process**. ⛔ A full `Win32_Process` census is
**outside my grant** (`VER-§8` cl. 8 — the read-only Python lane rejects `subprocess` and `ctypes`); this
log-header identification is what I could measure myself, and it is declared as such.
**PIE was NOT active when I looked** (`is_active: false`) ⇒ there was **no session of his to protect** and
nothing was ever stopped that I did not start. Map: `L_Arena` on arrival ⇒ **`load_level` → `/Game/Maps/L_MainMenu`**
(`discarded_unsaved: false`; dirty packages were **`[]` BEFORE** the load, so `discard_unsaved` was **never passed** —
see Limitations). PIE mode: standalone, 1 client, 1280×725. Attempts used: **2 of 3**. Wall time ≈ 12 min.
Nothing reached disk: dirty packages **`[]` after** (`DIRTY_CONTENT=[] DIRTY_MAPS=[] DIRTY_TOTAL=0`).

---

## (0) / (0b) — The two facts the dispatch asked me to re-verify myself

| fact | my own measurement | agrees? |
|---|---|---|
| category not suppressed | `SiegeMenuInputSubsystem.h:18` `DECLARE_LOG_CATEGORY_EXTERN(LogSiegeMenuInput, Log, All)`; `.cpp:22` `DEFINE_LOG_CATEGORY(LogSiegeMenuInput)`. Grep of **all of `Config/`** for `LogSiegeMenuInput` **and** `Core.Log` ⇒ **no matches** ⇒ **no override** | ✅ confirmed — **no verbosity step spent** |
| the Accept instrument exists | `.cpp:349-351`, emitted **after both guards and immediately before** `Focused->OnClicked.Broadcast()` (`.cpp:356`) | ✅ confirmed |

**(0b) ARMING PRECONDITION — quoted before anything is interpreted.** The success line is present in
**every** session discussed below, byte-for-byte:
`[USiegeMenuInputSubsystem] L_MainMenu: IMC_MainMenu applied at priority 0 on 'PlayerController_0'; IA_MenuUp / IA_MenuDown / IA_MenuAccept bound (Started).`
The `Warning: … menu input actions NOT armed.` failure line appears in **no** session I read. ⇒ **The subsystem armed
everywhere.** No absence below is the "never armed" artefact.

---

## (1) ARCHAEOLOGY — every Accept hit on disk, with attribution

Swept `Saved/Logs/*.log` (**all 84 files**, current + every rotated `-backup-*`) for `IA_MenuAccept -> OnClicked.Broadcast()`.
**64 `LogSiegeMenuInput` lines across 34 files. 31 of them are Accept hits. Attribution is COMPLETE — no `PARTIAL` needed.**

| # hits | where | attribution | driven? |
|---|---|---|---|
| **29** | `run_suite_bounded_suite_2026091*/2026092*.log`, all `'Button_2' ("Deck Builder")`, 2026-09-14 → 2026-09-20 | the **automated test suite** runner (`run_suite_bounded`), incl. ⭐ `TASK-1274`'s 2026-09-14 hit at `[2026.09.14-21.32.47:557]` | 🤖 **agent/automation** |
| **1** | `GitClaudeUnrealTest.log:4110` `[2026.09.22-21.21.35:057][737]` `'Button_4' ("Settings")` | ⭐ `TASK-1390`'s **READER CONTROL** — its own report names `171.05s (Accept)` via `inject_input_action`; PIE tore down 47 s later at `21.22.22` | 🤖 **agent** |
| **1** | `GitClaudeUnrealTest.log:6252` `[2026.09.23-19.12.52:154][957]` `'Button_1' ("Sandbox (No Bot)")` | **MY OWN control arm, today** — see (2) | 🤖 **agent (me)** |
| **0** | 🧑 **any undriven session** | — | — |

### 🚨 The finding: 🧑 HIS `-game` sessions armed the menu, OPENED SCREENS, and NEVER emitted an Accept line

`SC-§118` is explicit that a `-game` instance is 🧑 **his**. There are **four** such sessions retained on disk, all
launched `LogInit: Command Line: -windowed -ResX=3200 -ResY=1800 -game` (3200×1800 = his display):

| his session (log) | opened (local) | closed (local) | armed? | **deck builder opened?** | **Accept line?** |
|---|---|---|---|---|---|
| `GitClaudeUnrealTest_2-backup-2026.09.19-00.31.58.log` | 09/18 17:31:10 | 09/18 17:31:58 | ✅ | no | ❌ **none** |
| `GitClaudeUnrealTest_2-backup-2026.09.20-06.57.15.log` | 09/19 22:52:17 | 09/19 23:57:15 | ✅ | no | ❌ **none** |
| `GitClaudeUnrealTest_2-backup-2026.09.20-08.26.53.log` | 09/20 01:21:16 | 09/20 01:26:53 | ✅ | ✅ `[2026.09.20-08.21.32:302]` | ❌ **none** |
| **`GitClaudeUnrealTest_2-backup-2026.09.21-23.34.02.log`** ← 🧑 **the 2026-09-21 sitting** | **09/21 16:33:41** | 09/21 16:34:02 | ✅ `[23.33.50:535]` | ✅ **`[2026.09.21-23.33.55:800]`** | ❌ **none** |
| `GitClaudeUnrealTest_2.log` | 09/22 11:40:25 | 09/22 11:46:48 | ✅ | ✅ `[2026.09.22-18.41.00:218]` | ❌ **none** |

**Each of those five logs contains EXACTLY ONE `LogSiegeMenuInput` line — the arming line. Never an Accept.**
(Clock note, derived not assumed: log **headers** are local, **in-line stamps are UTC**, offset **UTC−7** —
cross-checked on the 09-21 file, header `16:33:41` vs first stamp `23.33.41`.)

**The 2026-09-21 sitting, reconstructed from its own log — it matches his account exactly:**
- `23.33.50:283` `LogLoad: LoadMap: /Game/Maps/L_MainMenu?Name=Player`
- `23.33.50:535` arming line (above)
- `23.33.50:535` `LogViewport: Display: Player bShowMouseCursor Changed, False -> True` ← **load-bearing, see (5)**
- `23.33.55:782` `UDeckBuilderWidget::AcquireBuilderFocus PRE-FLUSH read-back: IDENTITY=NO-MATCH … focused SWidget 0x000001EAA666DC10 ('SButton') vs this builder's SWidget 0x000001EB4C335790 ('SObjectWidget')`
- `23.33.55:800` `… POST-FLUSH read-back: IDENTITY=MATCH (THIS is the verdict …)` ⇒ **THE DECK BUILDER DEMONSTRABLY OPENED**
- `23.33.59:739` `LogSlate: Window 'Siegebound (64-bit Development PCD3D_SM6) ' being destroyed` ⇒ he closed it

**5.2 seconds** from menu-up to deck-builder-open — a human cadence. And the menu's own layout corroborates his
words: the suite logs prove **`Button_2` == "Deck Builder"**, and `Down`×2 from the initially-focused `Button_0`
lands exactly on `Button_2`. His account is **internally consistent with the real menu**. (Corroboration, **not** proof
of keyboard — see (5).)

---

## (2) THE FIRING CONTROL — taken LIVE, today, in the live editor's own log ✅ **IT FIRED**

| leg | value |
|---|---|
| arming line, this PIE session | `[2026.09.23-19.12.20:344][181]` (`GitClaudeUnrealTest.log:6200`) |
| `inject_input_action IA_MenuDown` | PIE **t = 30.71 s** |
| focus read after Down | **attempt 2** (clean pass, session 2): `Button_1` / `TextBlock_1` text **"Sandbox (No Bot)"** ⇒ **`focused: true`**, `hit_testable: true`, `realized: true`, at PIE **t = 18.59 s** |
| `inject_input_action IA_MenuAccept` | PIE **t = 31.49 s** |
| **THE ACCEPT LINE** | **`[2026.09.23-19.12.52:154][957]LogSiegeMenuInput: [USiegeMenuInputSubsystem] IA_MenuAccept -> OnClicked.Broadcast() on 'Button_1' ("Sandbox (No Bot)").`** (`GitClaudeUnrealTest.log:6252`) |

The two arms of the control **agree independently**: `ui_snapshot` says `Button_1` holds focus after one `Down`,
and the log says Accept fired on `Button_1`. ⇒ **The instrument is ALIVE in this build. It is not dead, and his
press will not be read against a dead instrument** (`SC-§54` cl. 1 satisfied).

---

## (4) THE SIDE-BY-SIDE READ — and the same-log question, answered plainly

| arm | log file | arming line | Accept line |
|---|---|---|---|
| **control (me, today)** | `Saved/Logs/GitClaudeUnrealTest.log` | ✅ `[2026.09.23-19.12.20:344]` | ✅ `[2026.09.23-19.12.52:154]` `'Button_1'` |
| 🧑 **his 2026-09-21 press** | `Saved/Logs/GitClaudeUnrealTest_2-backup-2026.09.21-23.34.02.log` | ✅ `[2026.09.21-23.33.50:535]` | ❌ **ABSENT** |

🚨 **ARE BOTH ARMS IN THE SAME LOG FILE? ⛔ NO — AND I AM SAYING SO RATHER THAN GLOSSING IT.**
They are not even the same **process**: his is a dead `-game` standalone, mine is the GUI editor's PIE.
**I cannot re-take the control in his file** — that process exited on 2026-09-21 and launching a `-game`
instance is an editor-lifecycle action I am fenced from. ⇒ Per the row's (5), **leg 2 of outcome (b) is MISSING**,
so outcome (b) is **NOT** available and I do **not** claim route (i).

**What his file DOES carry is a weaker but real control, and the distinction matters:**
- ✅ **category liveness IN HIS OWN PROCESS** — the arming line *is* a `LogSiegeMenuInput` line at `Log`
  verbosity, and it **printed** in his file. His process was **not** running with the category suppressed.
  The zero is therefore **not** the `SC-§39` "instrument wasn't watching" artefact at the *category* level.
- ❌ **Accept-callsite liveness in his process** — never established, and now unestablishable.

⇒ The silence is **informative but not conclusive**, which is precisely outcome **(c)**.

---

## (5) THE OUTCOME — letter (c), with the fourth explanation ARGUED, not waved at

**⛔ THE FOURTH EXPLANATION, TAKEN SERIOUSLY AND THEN DISPOSED OF ON SOURCE TEXT.**
`HandleMenuAccept` (`SiegeMenuInputSubsystem.cpp:334-357`) has exactly two silent exits —
`if (!IsMenuUncovered()) return;` and `UButton* Focused = GetFocusedMenuButton(); if (!Focused) return;`
(the latter carrying its own comment: *"Cold Accept does nothing, deliberately"*). So "the handler ran and
declined" is a genuine candidate for the **silence**. It is **refuted as an explanation of the OPENING**, three ways:

1. **A declining handler opens nothing.** The `UE_LOG` sits *between* the two guards and the broadcast:
   there is **no path through `HandleMenuAccept` that broadcasts `OnClicked` without first logging.**
   ⇒ Whatever opened the deck builder, **it was not this handler.** That conclusion is airtight and does
   not depend on his testimony at all.
2. **Both guards would have PASSED at that instant.** `IsMenuUncovered()` — the deck builder had not yet
   opened, the menu was uncovered. `GetFocusedMenuButton()` tests `Button->HasUserFocus(PC)`, and his log's
   PRE-FLUSH read-back records the then-focused Slate widget as **`SButton` `0x000001EAA666DC10`** — a button
   held focus. ⇒ `Focused` would have been **non-null**. So had an `IA_MenuAccept` trigger reached the
   subsystem, **the line would have printed.** The silence means the **trigger never arrived**, not that the
   handler declined. *(Caveat kept honest: "an SButton had focus" is strong but not airtight evidence that it
   was one of the menu's `UButton`s.)*
3. The same silence-with-an-opening reproduces across **three independent sittings on three different days**
   (09-20, 09-21, 09-22). A one-off timing artefact does not replicate three times.

**⇒ WHAT SURVIVES — two explanations, not four:**

| # | explanation | status |
|---|---|---|
| A | 🖱️ **He used the MOUSE.** `UButton::SlateHandleClicked` is *itself* `OnClicked.Broadcast()` (noted in the source comment at `.cpp:353-355`) ⇒ a click opens the screen and **bypasses `HandleMenuAccept` entirely**, emitting nothing. | ⛔ **NOT EXCLUDED** — and his log proves the cursor was **shown** (`bShowMouseCursor … False -> True`), so clicking was physically available |
| B | ⌨️ **Route (i)** — he used the keyboard and **Slate's own focused-widget Accept path** carried it, i.e. Enhanced Input never saw the key | ⛔ **NOT EXCLUDED** |
| C | ⌨️ **Route (ii)** — keyboard, and Enhanced Input **did** deliver it | ✅ **REFUTED** *for these sessions* — conditional on the press having been a key at all |
| D | the handler ran and **declined** | ✅ **REFUTED** (argument above) |

🚨 **A AND B PRODUCE A BYTE-IDENTICAL LOG SIGNATURE.** This is *exactly* the false positive the row's
verbatim parenthesis was written to prevent: reading this silence as "route (i)" would **manufacture** a
conclusion that a mouse click could equally have produced. ⇒ **I do not claim route (i).**

⇒ **OUTCOME (c) — `UNOBSERVABLE`. `VER-§8` cl. 11's `SetIgnoreInput` premise is NEITHER refuted NOR confirmed
by this run, and it STANDS UNRELAXED.** ⛔ I amend no clause — that is the manager's act on this report (`SC-§101`).

### ✅ What this run nevertheless bought (the row is not a dry hole)
1. The firing control is **banked in the live editor's log** — limb B no longer needs one.
2. Explanations **C and D are eliminated in advance**, so his answer cannot be argued away afterwards.
3. **Limb B is now a ONE-BIT question.** "Keyboard only?" → **yes** ⇒ route **(i)** measured (A falls, B stands).
   → **"I touched the mouse"** ⇒ redo, costs him 15 seconds.

---

## 🧑 (3) LIMB B — STILL OWED. The sentence, VERBATIM, parenthesis INTACT

⛔ Returned to the orchestrator to put to him — I do not assume I may ask him myself (`VER-§8` cl. 3(b)/3(c)).
⛔ **Not trimmed, not improvised, not paraphrased:**

> ***"🧑 On the main menu, ⛔ KEYBOARD ONLY — ⛔ don't touch the mouse, ⛔ don't move it, ⛔ don't click — press `Down` twice and then press `Enter`. ⛔ Did a ⛔ NEW SCREEN actually ⛔ OPEN, and ⛔ which one? (⛔ An outline moving on the menu is ⛔ NOT a yes — ⛔ something has to ⛔ OPEN. ⛔ And if you touched the mouse at ⛔ any point the answer ⛔ does not count — ⛔ just say so and we'll redo it. ⛔ Roughly what time, so the line can be found.)"***

🚨 **ONE OPERATIONAL RIDER THE NEXT HOST MUST CARRY WITH IT — it is what fixes the missing leg 2:**
his press must land in **`GitClaudeUnrealTest.log`**, i.e. in **PIE inside the ALREADY-RUNNING editor (PID 26992)**,
where my control arm now sits. I have left that editor **UP and already on `L_MainMenu`** so he can simply press Play.
⛔ **If the editor is restarted first, my control rotates into a `-backup-` file and leg 2 breaks again.**
⛔ **If he instead launches a fresh `-game` window** (his habit — all four sittings above were `-game`), his press
lands in a **new `GitClaudeUnrealTest_2.log` that contains NO control arm**, and the result is **another (c)**.

---

## Evidence (promoted)
**NONE — and no promotion is owed.** Every observable on this row is **log text**, not pixels; the report cites
file + line + timestamp, which is stronger and independently re-greppable. No screenshot is cited, so per
`FR-§1`/`VER-§4` ("promote what the report cites") there is nothing to promote.
⚠️ **Declared discrepancy, not silently resolved:** my `names:` line grants writes to
`.claude/pipeline/playtest-evidence/**2026-09-22**/`, but this row ran on **2026-09-23**. Writing today's frames
into a 2026-09-22 folder would **misdate** evidence. Since no PNG is required, I wrote none rather than
choose between a misdated path and an ungranted one. **Flagging it for the manager** — if a future attempt of
this row needs pixels, that grant needs re-dating.

Re-greppable primary sources (all read-only, none copied):
- `Saved/Logs/GitClaudeUnrealTest.log` — lines **6200** (arming) and **6252** (control Accept)
- `Saved/Logs/GitClaudeUnrealTest_2-backup-2026.09.21-23.34.02.log` — lines **1957** (arming), **2058-2059** (deck builder opened), **516** (`-game` command line)
- `Source/GitClaudeUnrealTest/Siegebound/SiegeMenuInputSubsystem.cpp:334-357` · `.../SiegeMenuInputSubsystem.h:18`

## Hypotheses (not verdicts)
- **HYPOTHESIS, not measured:** the consistent pattern — **five** of his `-game` sessions armed the menu and
  **not one** ever produced an Accept line, while three of them opened screens — is *suggestive* that real keys
  never reach Enhanced Input on `L_MainMenu` (which would **support** `VER-§8` cl. 11's premise). ⛔ It is equally
  explained by "he navigates that menu with the mouse," and the cursor is shown there. **Not a verdict. Do not
  cite this row as having measured it.**
- **HYPOTHESIS:** `Button_2` == "Deck Builder" and `Down`×2 from `Button_0` reaches `Button_2`, which matches
  his account arithmetically. Corroborates his *recollection*; says nothing about the input *device*.

## Not examined / limitations this run
- ⛔ **The `Down` leg was NOT attempted** — deliberately, per dispatch. `HandleMenuDown` is `{ MoveFocus(+1); }`
  and every exit of `MoveFocus` is silent; a silence there today is a **guaranteed false negative**. That is
  ⭐ `TASK-1394`'s subject, not mine. *(My `IA_MenuDown` injections were used only to place focus for the control;
  no claim of any kind is made about the Down leg.)*
- ⛔ **Mouse-vs-keyboard for 2026-09-21 is UNDECIDABLE from disk.** No log line distinguishes a pointer click
  from a Slate key-accept on this path. This is the sole reason limb B survives.
- ⛔ **Leg 2 is permanently unsatisfiable for the archaeology arm** — his process is dead; I cannot inject into it,
  and launching `-game` is outside my fence.
- ⛔ **Rotation:** older `-game` sessions than 2026-09-18 have rotated out. Absence *there* is `SC-§39` absence,
  not emptiness. I make no claim about them.
- ⛔ **Editor identification** is by log header + empty command line, **not** by a `Win32_Process` census
  (`VER-§8` cl. 8 — outside my grant). Stated, not glossed.
- ⛔ The dispatch relayed that the **current** `GitClaudeUnrealTest.log` held **zero** `LogSiegeMenuInput` lines.
  **My own measurement contradicts that**: it held **3** before I touched anything (lines 4069 / 4110 / 4192,
  all 2026-09-22), and **5** after my control arm. I acted on my measurement, not the relayed one. The relayed
  figure most likely described `GitClaudeUnrealTest_2.log` (which does hold exactly one project-category line).
- ⛔ **No** code, asset, compile, git, or editor-lifecycle action was taken. `CONVENTIONS.md` untouched;
  no other row's line and no other agent's file was written.

### `VER-§7` cl. 2 declaration (census §5 names reached through the runner)
Names used: **`inject_input_action`** ×3, **`ui_snapshot`** ×2, `wait_pie_seconds`, `load_level`, `start_pie`,
`stop_pie`, `is_pie_active`. ⛔ **`binding_found` was NEVER the observable and is not cited anywhere in this
report** — `inject_input_action` bypasses bindings by design, and my observables were a **log line** and a
**`focused: true`** widget read. `simulate_button_press` was **not** used (cl. 2(a): its surface is unmeasured).
**Budget (cl. 2(b)):** every observable landed **inside the `t≈60 s` fence** — Accept at **t=31.49 s**, focus read at
**t=18.59 s** — by batching into a single `run_verification_sequence` per PIE session (2 sequences, 2 sessions,
zero round trips spent inside a fence).
