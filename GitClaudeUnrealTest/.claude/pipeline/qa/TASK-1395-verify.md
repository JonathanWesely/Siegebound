Verdict: UNOBSERVABLE
# Verification — TASK-1395

**Headline, stated first so it is not lost inside an `UNOBSERVABLE`:** ⭐ **the (1) binary-is-live
control arm FIRED.** `TASK-1394`'s instrument — shipped in `40c824b` and, until this run, **never once
read at runtime** — emitted **shape 4** (entry line + `focus moved` exit line), **twice**, with
independent UI corroboration. ⇒ **The instrument is LIVE and a future silence from it is a REAL
silence.** What is `UNOBSERVABLE` is **not** the instrument — it is the row's actual research
question, which needs 🧑 **his** keypress (limb B) and did not get one this sitting.

Editor/Aura state: **connected y** · editor **PID 5728**, GUI, **untouched lifecycle** (not started,
not closed, not relaunched by me) · map `/Game/Maps/L_MainMenu.L_MainMenu` before **and** after ·
PIE mode **standalone, 1 client, 1280×725**, **started by me and stopped by me** (no pre-existing
session existed to protect: `is_pie_active` read `is_active: false` before I began) · **attempts used
1 of 3** · wall time ≈ 9 min · PIE clock spent ≈ 98 s · Aura credit not surfaced by any tool this run.

🚨 **`SC-§118` identification — the route is DECLARED because the usual one is UNAVAILABLE.**
`unreal.SystemLibrary.get_command_line()` returned **`''` (empty)** on this editor, and the log
carries no `LogInit: Command Line:` line — **exactly what `TASK-1402` measured.** The command-line
route to identification is therefore dead here, and I substitute a named one rather than skip it:
in-process `os.getpid()` = **5728**, matching the PID the dispatch censused, executed *inside* the
same process that owns the world and the log I read. No `-game` instance exists; none was touched.

---

## 🚨 (2b) BUILD CONFIGURATION — DECLARED BY NAME, BEFORE ANY ABSENCE IS READ

Binding precondition (`VER-§9`, `SHIPPING-ABSENCE-RULED-2026-09-24`). Established **in-process**, by
**two independent routes that agree**:

| route | value | why it discriminates |
|---|---|---|
| `unreal.SystemLibrary.get_build_configuration()` | **`Development`** | the engine's own answer |
| executable path (`sys.executable`) | `C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\**UnrealEditor.exe**` | **unsuffixed** — a Shipping binary is `UnrealEditor-Win64-Shipping.exe`; the suffix is the configuration |
| world + editor-only API reachable | `/Game/Maps/L_MainMenu.L_MainMenu` via `UnrealEditorSubsystem` | it is an **Editor** target, not `-game`, not packaged |

⇒ **CONFIGURATION OF THE SESSION READ: `Development (Editor)` — PIE, in-editor.**
⇒ `UE_LOG(LogSiegeMenuInput, Log, …)` is **compiled IN** here. `USE_LOGGING_IN_SHIPPING` / `NO_LOGGING`
(`bUseLoggingInShipping`, documented in-tree in `SiegeAssistantGrammar.cpp`) **does not apply to this
session.** An absence in **this** log would have been readable as a real absence.

⛔ **And the honest corollary, stated so the declaration is not over-claimed:** my `UNOBSERVABLE`
below is **NOT** a log absence at all. It is a **leg that was never run**. `VER-§9` cl. 6's asymmetry
(*presence survives an undeclared configuration; absence does not*) is satisfied twice over: my
positive is a **presence**, and my gap is **not an absence** — no line was read as missing.

---

## Acceptance lines → observations

| # | acceptance line | observable chosen | observed (quoted values, evidence path) | pass/fail/unobs |
|---|---|---|---|---|
| 1 | line 1 `Verdict:` byte-literal | this file's first line | `Verdict: UNOBSERVABLE` | pass |
| 2 | **(2b) build configuration declared by name BEFORE any absence** | `get_build_configuration()` + unsuffixed exe path | **`Development (Editor)`** — section above, declared ahead of every reading | pass |
| 3 | **(1) binary-is-live control printed FIRST, with its PIE stamp** | `TASK-1394`'s new **entry line** after an injected `IA_MenuDown` | `L4852` `[2026.09.25-03.47.52:884][490]` `IA_MenuDown -> HandleMenuDown() entered; calling MoveFocus(+1).` — PIE stamp `pie_time_seconds 22.767` | **pass** |
| 4 | arming line printed (`TASK-1393` (0b)) | the `IMC_MainMenu applied` success line | `L4829` `[2026.09.25-03.47.28:797][208]` `L_MainMenu: IMC_MainMenu applied at priority 0 on 'PlayerController_0'; IA_MenuUp / IA_MenuDown / IA_MenuAccept bound (Started).` — the `… NOT armed.` warning appears **nowhere** in this session | **pass** |
| 5 | **(2) 🧑 his limb — keyboard-only `Down` once, verbatim answer** | 🧑 his own keypress + his sentence | **NOT TAKEN THIS SITTING — declared pending** (acceptance permits *"or limb B declared pending"*). No sentence from him exists to quote; I quote none. | **unobs** |
| 6 | both arms side by side **with the same-log statement** | two arms in one log file | **only ONE arm exists** — see the same-log statement below | **unobs** |
| 7 | **(3) the outcome named by letter** | (a) / (b) / (c) | **NO LETTER APPLIES** — (a) and (b) are both predicated on *"his press"*; (c) needs a **decline** reason line and I got the **focus-moved** line instead. Per (3)'s own closing sentence — *"Any leg missing ⇒ `UNOBSERVABLE`, not a negative"* — the missing leg governs. | **unobs** |
| 8 | `## Not examined / limitations` present | this document | present below | pass |
| 9 | `VER-§7` cl. 2 declaration for any census §5 name reached through the runner | the runner's step vocabulary | declared below — **no census §5 mutation name was reached** | pass |

---

## THE CONTROL ARM — WHAT FIRED, QUOTED VERBATIM

**Same process (PID 5728), same PIE session, same log file**
`C:/GitProjects/GitHub/GitClaudeUnrealTesting/GitClaudeUnrealTest/Saved/Logs/GitClaudeUnrealTest.log`
(5,002 lines at read time). ⚠️ Quoted in full here **because this log rotates** — five of 🧑 his
sessions rotated in one afternoon on 2026-09-23, and a citation by filename alone would decay.

```
L4829: [2026.09.25-03.47.28:797][208]LogSiegeMenuInput: [USiegeMenuInputSubsystem] L_MainMenu: IMC_MainMenu applied at priority 0 on 'PlayerController_0'; IA_MenuUp / IA_MenuDown / IA_MenuAccept bound (Started).
L4835: [2026.09.25-03.47.28:821][208]LogSiegeMenuInput: [USiegeMenuInputSubsystem] ApplyInitialFocus: focus placed on the TOP option 'Button_0' ("Play (vs Bot)"), index 0 of 7, in menu instance 'WBP_MainMenu_C_0'.
L4852: [2026.09.25-03.47.52:884][490]LogSiegeMenuInput: [USiegeMenuInputSubsystem] IA_MenuDown -> HandleMenuDown() entered; calling MoveFocus(+1).
L4853: [2026.09.25-03.47.52:884][490]LogSiegeMenuInput: [USiegeMenuInputSubsystem] MoveFocus(+1): focus moved 0 -> 1 of 7 ('Button_1').
L4854: [2026.09.25-03.47.53:934][553]LogSiegeMenuInput: [USiegeMenuInputSubsystem] IA_MenuDown -> HandleMenuDown() entered; calling MoveFocus(+1).
L4855: [2026.09.25-03.47.53:934][553]LogSiegeMenuInput: [USiegeMenuInputSubsystem] MoveFocus(+1): focus moved 1 -> 2 of 7 ('Button_2').
```

(Log stamps are **UTC**; local date is 2026-09-24. `[2026.09.25-03.47]` UTC = 2026-09-24 evening local.)

### Attribution — these lines are MINE, proven by two independent clocks

| | injected (PIE clock) | logged (UTC wall clock) |
|---|---|---|
| press 1 | `22.767405` s | `03.47.52:884` |
| press 2 | `23.817776` s | `03.47.53:934` |
| **Δ** | **1.05037 s** | **1.050 s** |

The two deltas agree to the millisecond. These are not inherited lines from an earlier session.

### 🚨 WHICH OF THE FOUR SHAPES — **SHAPE 4**, twice, unambiguously

| shape | meaning | observed? |
|---|---|---|
| 1 — **no entry line** | handler never ran (incl. Slate's own `FNavigationConfig` taking the key) | ⛔ **no** |
| 2 — entry + *declined: menu covered* | ran, declined | ⛔ no |
| 3 — entry + *declined: no menu buttons* | ran, declined | ⛔ no |
| 4 — **entry + `focus moved X -> Y of N ('Button_N')`** | ran, moved focus | ✅ **YES — ×2** |

Both `Delta` fields print `+1`, and the exit line names index, count and button
(`0 -> 1 of 7 ('Button_1')`, then `1 -> 2 of 7 ('Button_2')`). The four shapes are **distinguishable
in practice**, not merely in the diff — that is `TASK-1394`'s own deliverable, now read at runtime.

### The independent (non-log) arm — `ui_snapshot`, agreeing on every step

| step | `ui_snapshot` result | PIE stamp |
|---|---|---|
| before any press | `Button_0` `focused: true`, text **"Play (vs Bot)"** | 22.751 s |
| after press 1 | `Button_1` `focused: true`, text **"Sandbox (No Bot)"** | 23.801 s |
| after press 2 | `Button_2` `focused: true`, text **"Deck Builder"** | 24.835 s |

⇒ **The log's claim and the widget tree's focus flag agree independently at all three points.** The
instrument is not merely emitting text; the text is **true**.

---

## 🚨 THE SAME-LOG STATEMENT — SAID PLAINLY RATHER THAN GLOSSED

**Are both arms in the same log file? ⛔ NO — because the second arm does not exist.**
This sitting produced **one** arm (mine, injected). 🧑 His limb was not taken. The row's binding
rider wants *the `Down` leg and the `Enter` leg in the SAME sitting, SAME log file, from the SAME
pair of hands* — and **"the same pair of hands" means his.** I did not manufacture a substitute.

⛔ **I deliberately did NOT inject an `IA_MenuAccept` to manufacture a second leg**, and the reason is
the point of the rider rather than a shortcut: the within-process control pair the rider describes is
*a positive `Down` beside an **absent** `Accept`* — a shape that **only his session can produce**. In
an injected session **both** legs come back positive, which discriminates nothing; an injected Accept
has already been positive 13+ times on this disk (`TASK-1274`, `TASK-1390`, `TASK-1393`'s control,
`TASK-1399` ×12). Adding a 14th would have added an Accept line to this log that a later reader could
easily mistake for leg 2 being closed. ⇒ **Leg 2 of `VER-§8` cl. 11 remains OPEN and remains this
row's**, exactly as the board already records for `TASK-1399`'s output.

*(A same-process precedent does exist and I name it rather than hide it: this **same log file** holds
an injected `Down`+`Accept` pair from an **earlier PIE session of this same process** —
`[2026.09.25-00.46.00:835]` / `00.46.01:252` Down, `00.46.01:685` Accept on `'Button_2' ("Deck
Builder")`. **Agent-injected, not his hands. It is NOT leg 2** and must never be cited as such.)*

---

## 🚨 WHY THE INJECTED ARM CANNOT ANSWER THE ROW'S QUESTION — THE REASONING, NOT AN EXCUSE

The row asks whether **a real `Down`** reaches `HandleMenuDown`, i.e. route (ii) *the subsystem
handles the key* versus route (i) *Slate's own `FNavigationConfig` consumes it first*. **That
competition happens at the KEY → ACTION layer.** `inject_input_action` injects the **action**, not the
key — it enters **downstream of the very fork the row exists to resolve.** ⇒ A positive from my arm
is **guaranteed by construction** and is therefore evidence about the **instrument**, never about the
**route**. Reporting shape 4 from an injected action as outcome **(a)** would be
`VER-§10` cl. 4's *"`UNOBSERVABLE` in a better suit"* — a control that could not have come out any
other way, dressed as a measurement. **I decline to do that**, and that declension is the reason
line 1 reads `UNOBSERVABLE` rather than a letter.

⚠️ **This disagrees with my dispatch**, and I flag it rather than quietly resolve it: the dispatch
states *"this row needs **only** an injected `IA_MenuDown` … No click, no return."* The **row** —
which the same dispatch instructs me to *"follow exactly"* — pre-writes all three outcomes against
🧑 *"his press"* and closes with *"Any leg missing ⇒ `UNOBSERVABLE`."* **I followed the row.** If the
manager intends the injected arm alone to close this row, that is a **spec amendment and it is the
manager's** (`SC-§101`, `SC-§100`) — ⛔ I amended nothing, and I did not amend `VER-§8` cl. 11 on
either outcome, as (4) forbids.

---

## Evidence (promoted)

⛔ **NONE — and that is correct for this row, not an omission.** This is a **log-line measurement**;
the dispatch fences the pixel limb out (*"`VER-§11` cl. 9(b) governs any capture and this row specs
none"*) and the row's acceptance requires no PNG. **Promoting nothing is the discipline**
(*promote what the report cites*), and what the report cites is quoted **verbatim above** precisely
so it survives this log rotating.

- **Cited (gitignored, `Saved/`, not promotable):**
  `C:/GitProjects/GitHub/GitClaudeUnrealTesting/GitClaudeUnrealTest/Saved/Logs/GitClaudeUnrealTest.log`
  — lines **4829** (arming), **4852–4855** (the four instrument lines). Quoted in full above.
- **Exists on disk but cited by NO claim in this report:** a background film auto-armed at
  `start_pie` into `Saved/AuraVerify/rec_1790308048451780700_36`. I attached and read **no frame**
  from it. ⛔ No claim here rests on a pixel. I mention it only so its existence is not a surprise.
- **Nothing is owed to a host row.** No promotion was attempted and none is pending.

---

## Hypotheses (not verdicts)

1. **HYPOTHESIS, not measured:** since an injected `IA_MenuDown` reaches `HandleMenuDown` cleanly and
   the mapping context **is** applied and bound (`L4829` names `IA_MenuDown` explicitly), the most
   economical expectation is that 🧑 his hardware `Down` **also** routes there (outcome (a)). ⛔ **This
   is a prediction, not a finding** — it is exactly the proposition `TASK-1389` says is in doubt, and
   the injected arm **structurally cannot** test it (section above). ⛔ Do not let its plausibility be
   read as its having been measured.
2. **HYPOTHESIS:** shapes **2** and **3** (*menu covered* / *no menu buttons*) are presumably
   reachable, but I forced neither. Shape 2 would need a sub-screen over the menu — and `TASK-1402`
   measured that **no agent can close one**, so forcing it would have stranded the session. Their code
   paths are therefore **unexercised at runtime**, and I claim nothing about them beyond their
   presence in the diff.
3. **OBSERVATION, offered without a mechanism:** six `ApplyInitialFocus` lines appear earlier in this
   same log (`00.56.27:825`, `00.56.28:025`, `00.58.04:518`, `00.58.04:812` and two paired with arming
   lines) with **no** intervening `IA_MenuDown`. Not my session, not this row's question, **mechanism
   not measured** — recorded only so a later reader does not mistake them for `Down` traffic.

---

## Not examined / limitations this run

- 🧑 **LIMB B IS PENDING — the single reason line 1 is not a letter.** No keypress of his, no sentence
  of his. The verbatim prompt he still owes an answer to is the row's (2), unchanged. **One PIE
  sitting with his hands on the keyboard closes this row**, and the instrument is now **proven live**,
  so that sitting is cheap and its silence (if any) would be a **real** silence.
- **`VER-§5` cl. 4 tension, declared rather than resolved silently:** cl. 4 says a *partial* row is
  **not** `UNOBSERVABLE` and takes `verify: partial (n/m observable)`. A partial reading of this run is
  defensible (5 of 9 acceptance items passed). I went with `UNOBSERVABLE` because **the row's own (3)
  closes with the specific instruction** *"Any leg missing ⇒ `UNOBSERVABLE`, not a negative"*, and a
  row-specific pre-written rule beats my choosing after the fact. ⚖️ **If the manager reads cl. 4 as
  governing, the correction is his to make** — the observations above are unchanged either way.
- **`VER-§7` cl. 2 — runner step-vocabulary declaration (required by this row's acceptance).** Names
  reached **through** `run_verification_sequence`: **`wait_pie_seconds`**, **`inject_input_action`**,
  **`ui_snapshot`**. ⛔ **No census §5 (PIE-world mutation / staging) name was reached** — no
  `pie_scene_edit`, no `spawn_actor`, no `delete_actor`, no `set_actor_property`, no
  `set_actor_transform`, no `set_actor_enabled`, no `teleport_player`, no `call_actor_function`.
  ⚠️ `ui_snapshot` is a **read** (widget tree + geometry, no pixels), and it is named here because
  `TASK-1390` measured it **absent from the runner's prose whitelist while present in its JSON action
  enum** — it was accepted, consistent with that finding.
- **`start_pie`'s response exceeded the tool's token ceiling** (635,270 chars / 15,814 lines — a
  recovered *earlier* nvenc film's `time_index`, spilled to
  `…/tool-results/mcp-unreal_editor-start_pie-1790308049005.txt`). **I read only its first 25 lines**
  (status header) and **did not read the remainder**; I state that rather than imply I reviewed it.
  Session truth came from `is_pie_active` (`is_active: true`, 1280×725, 1 instance), not from it.
  ⛔ No claim in this report rests on that file.
- **Attempts: 1 of 3 used.** Two remain. I did not spend a second attempt because the control arm was
  unambiguous on the first and the missing leg is **not** something a retry can supply.
- **Not examined:** `HandleMenuUp` / `IA_MenuUp` (untouched by `TASK-1394`, outside this row);
  `IA_MenuAccept` this sitting (declined on purpose, reasoned above); the working-tree source of
  `SiegeMenuInputSubsystem.cpp` — ⛔ **deliberately NOT read as though it were what is running**, since
  `TASK-1406` is authoring it **uncompiled** right now. The running binaries are **`40c824b`'s**, and
  the instrument strings I matched came from `handoffs/TASK-1394-programmer.md`, not from the
  working tree.
- **Serialization (`VER-§2` cl. 1) honoured:** no board row read `integrating` at my instant (grep for
  `^- status: .*integrating` → **0 matches**); no compile, assemble or import was live; exactly one
  verification (this one) ran.

---

## Editor state left behind — re-measured after PIE, not assumed

| | before | after |
|---|---|---|
| PID | 5728 | **5728** (unchanged — no lifecycle action taken) |
| map | `/Game/Maps/L_MainMenu.L_MainMenu` | **`/Game/Maps/L_MainMenu.L_MainMenu`** |
| PIE | stopped | **stopped** (mine, started and stopped by me) |
| dirty packages | `[]` | **`[]`** |

⛔ No code, no asset, no compile, no git, no editor lifecycle action, no save. ⛔ No clause amended,
⛔ no row boarded, ⛔ no other row's line touched. ⛔ **NO COMMIT.**
✅ The editor is left **exactly as handed to me, already on `L_MainMenu`** — so 🧑 he can press Play and
take limb B without any setup.
