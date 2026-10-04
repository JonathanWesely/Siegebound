<!-- ARCHIVED from .claude/pipeline/TASKBOARD.md on 2026-10-04 by Tools/archive_board.py. Every row below was in a terminal state when moved; bytes are unchanged and this comment is the only addition. Law: TASKBOARD.md '## Archive'. -->
#### TASK-436 — [SET-1] `USiegeSettingsSaveGame` + `USiegeSettingsSubsystem` — the persisted settings store (gameplay-programmer)
- assignee: gameplay-programmer
- status: **done — COMMITTED 2026-08-03 in `cd5f4ed`** (Wave 1 code. Build GREEN: both DLLs linked, **zero errors, zero warnings** — the link step ran for the first time, so the 22-seam pre-flight became a measurement. ⚠️ **Green means it BUILDS, not that it works**: no automation test has ever run (33 compile; compiling is not passing), WARN-5 stays instrumented-not-discharged, and every playable check belongs to TASK-448. Flip applied by the ORCHESTRATOR after manager handed the board back.) ← prior: **qa-passed** (2026-08-03 — `qa/TASK-439.md`: **PASS, 0 blockers**, 2 warns, 2 nits. Flip applied by the ORCHESTRATOR because qa-reviewer has no partial-edit tool; the verdict is QA's, the edit is mine. ⚠️ **WARN-436-2:** the `true`-direction save/load assertion cannot distinguish *loaded true* from *never loaded* — does NOT block TASK-447. ⛔ **QA EXPLICITLY DID NOT CERTIFY THE TOGGLE'S OFF-PATH** — it verified only the scope fact that this task references no assistant/guard/executor code, which is NOT the sentence *"every check still runs with the toggle off."* **That trace is TASK-443's and closes at TASK-446; this task may not be cited as evidence for it.** Handoff `handoffs/TASK-436-programmer.md`; ⛔ NOT compiled, TASK-447 is the one gate)
- blocked-by: **none — DISPATCHABLE NOW**
- parallel-safe: yes (new files only); **EXCLUSIVE owner of `SiegeSettingsSaveGame.{h,cpp}` + `SiegeSettingsSubsystem.{h,cpp}`**
- spec: >
    **Read CONVENTIONS "Settings screen + the assistant CONFIRM STEP…" §2 and §8 first — the signatures are binding character-for-character**,
    because TASK-437 and TASK-443 both compile against them and neither will exist when you start.
    **(1) `USiegeSettingsSaveGame : USaveGame`** carrying exactly one field in v1: **`bool bAssistantConfirmBeforeExecute = true`**.
    ⚠️ **`USaveGame` AND NOT `UGameUserSettings` — RULED, and the reason matters because the other answer looks more idiomatic:**
    `UGameUserSettings` is the engine's home for video/audio/input and adopting it means a **`GameUserSettingsClassName` edit in
    `DefaultEngine.ini`** — a shipped-config change plus a new engine coupling, to store one gameplay bool. **`USiegeDeckSaveGame` + a fixed
    slot is this project's proven, package-safe, runtime-writable idiom** and needs no config edit. ⛔ **Do NOT edit any `.ini`.**
    **(2) `USiegeSettingsSubsystem : UGameInstanceSubsystem`** — ⚠️ **the class MUST be a GameInstance subsystem and that is the whole
    requirement, not a style choice: the value is WRITTEN in `L_MainMenu` and READ in `L_Arena`, mid-match.** A widget- or controller-owned
    value does not survive `OpenLevel` (the Input-mode ownership law is this same lesson wearing different clothes). Precedents:
    `USiegeSessionSubsystem`, `USiegeLlamaSubsystem`.
    **(3) LOAD ONCE, SAVE ON CHANGE, AND NEVER TOUCH THE DISK FROM A GAMEPLAY PATH.** `Initialize()` loads slot **`"SiegeSettings"`**,
    user index **0**. **Missing / unreadable / wrong class ⇒ fall back to the C++ defaults, log ONCE on `LogSiegeSettings`, never crash**
    (the `USiegeDeckSaveGame` null-safety contract — copy it, do not reinvent it). `SetAssistantConfirmEnabled` writes the in-memory value
    **and** saves the slot. ⛔ **`IsAssistantConfirmEnabled()` is a pure in-memory getter — a `LoadGameFromSlot` on the order path would put a
    disk hit inside a mid-battle interaction.**
    **(4) `OnSettingsChanged` BROADCASTS ON EVERY ACTUAL VALUE CHANGE, NEVER ON A NO-OP WRITE** (the delegate law + the qa/TASK-005 major-2
    lesson: a UI that binds without seeding goes stale, and a delegate that fires on refused mutations trains consumers to ignore it).
    **(5) AUTOMATION TESTS — cheap here and genuinely worth it.** `IMPLEMENT_SIMPLE_AUTOMATION_TEST` under `#if WITH_DEV_AUTOMATION_TESTS`.
    Cover at minimum: **default is `true` on a fresh/absent slot** · **set → get round-trips** · **save → load round-trips the value**
    · **a missing slot yields defaults and does not crash** · **the delegate fires on a real change and does NOT fire on a same-value write.**
    ⛔ **NO widget code, NO assistant code, NO `Content/` asset, NO `.ini`, NO Git, NO compile.** New files only.
    **M8 DECLARATION DUTY: state verbatim in the handoff — "adds no replicated property, no new replicated class, no new relevancy tier."**
    ⚠️ **And say WHY it is true here rather than only asserting it: this value is client-local by construction — it governs a LOCAL review
    step and never an authoritative outcome.**
    Handoff `handoffs/TASK-436-programmer.md`. Post in ⚙️ Dev & QA.
- names: >
    `SiegeSettingsSaveGame.{h,cpp}` · `USiegeSettingsSaveGame` · `bAssistantConfirmBeforeExecute` (default **`true`**) ·
    `SiegeSettingsSubsystem.{h,cpp}` · `USiegeSettingsSubsystem` · `SettingsSlotName` == **`"SiegeSettings"`** (a const,
    character-for-character — ⚠️ **a changed slot name silently orphans every player's saved settings**) ·
    `IsAssistantConfirmEnabled` / `SetAssistantConfirmEnabled` / `OnSettingsChanged` / `FOnSiegeSettingsChanged` ·
    `LogSiegeSettings`. **§8 pinned registry, character-for-character.**
    Law: CONVENTIONS "Settings screen + the assistant CONFIRM STEP…" §2, §8, §10 · "Deck-builder & saved decks (M6)" (the SaveGame idiom) ·
    "Delegates (C++)" · "Logging (C++)".

#### TASK-437 — [SET-2] `USettingsMenuWidget` — the settings screen, CODE-AUTHORED TREE (gameplay-programmer)
- assignee: gameplay-programmer
- status: **done — COMMITTED 2026-08-03 in `cd5f4ed`** (Wave 1 code. Build GREEN: both DLLs linked, **zero errors, zero warnings** — the link step ran for the first time, so the 22-seam pre-flight became a measurement. ⚠️ **Green means it BUILDS, not that it works**: no automation test has ever run (33 compile; compiling is not passing), WARN-5 stays instrumented-not-discharged, and every playable check belongs to TASK-448. Flip applied by the ORCHESTRATOR after manager handed the board back.) ← prior: **qa-passed** (2026-08-03 — `qa/TASK-439.md`: **PASS, 0 blockers**, 2 warns, 4 nits. Flip applied by the ORCHESTRATOR (qa-reviewer has no partial-edit tool). ✅ **QA RULED THE BEFORE-`Super` DEPARTURE CORRECT AND THE SPEC TEXT WRONG**, verified at the engine source: `UserWidget.cpp:1214` returns an `SSpacer` when `RootWidget` is null, and `BindWidget*` members resolve inside `Initialize()` at `WidgetBlueprintGeneratedClass.cpp:275`, so *"only if still null"* is fully determined before Super runs. ⇒ ⛔ **`CONVENTIONS.md:1195` STILL CARRIES THE WRONG ORDERING AND WILL SHIP THE NEXT CODE-AUTHORED WIDGET BLANK — fix owed to manager.** ✅ **The `NativeOnInitialized` trap is a CONFIRMED NEGATIVE here** — all eight children wire from `NativeConstruct` (`SettingsMenuWidget.cpp:301-315`). ⚠️ Note the asymmetry QA recorded: the shipped `USessionMenuWidget` *does* use `NativeOnInitialized` and is CORRECT to, because it is a WBP — **copying that precedent into a code-authored widget is the bug.** ⚠️ **WARN-437-1:** `ConstructSettingsTree()` runs before Super's defensive `Initialize()` — unreachable via `CreateWidget`, one-line fix, non-blocking. Handoff `handoffs/TASK-437-programmer.md`. New files only: `Source/GitClaudeUnrealTest/Siegebound/SettingsMenuWidget.{h,cpp}`. Built under CONVENTIONS "Settings screen…" **§3** / board **ruling 8** — ⚠️ **NOT §6 ruling A**, which is console-scoped. No `.uasset` authored, `/Game/UI/WBP_SettingsMenu` still RESERVED, no editor/MCP/PIE, no compile, no Git. ⚠️ **QA read §5.1 of the handoff first: the tree is built BEFORE `Super::RebuildWidget()`, a deliberate departure from the spec's literal wording** — the engine reads `WidgetTree->RootWidget` as it stands and returns an `SSpacer` when null, so building after Super yields a silently EMPTY widget that still passes every readback (the TASK-411 probe's recorded finding). ⚠️ **Nothing on screen is verified — pixel checklist for TASK-448 is handoff §7, and the backdrop click-through test is the blocker criterion.**)
- blocked-by: **none — DISPATCHABLE NOW** (compiles against TASK-436's pinned signatures; ⚠️ **it will not compile ALONE and is not expected to** — the TASK-416/417 precedent, do not open a QA loop over it)
- parallel-safe: yes; **EXCLUSIVE owner of `SettingsMenuWidget.{h,cpp}`**
- spec: >
    **Read CONVENTIONS "Settings screen + the assistant CONFIRM STEP…" §3 (the ruling), §4 (navigation) and §8 (the pin) first.**
    **(0) ⛔ YOU ARE AUTHORING NO `.uasset`. THE TREE IS BUILT IN C++.** This is a **NEW, NAMED, SCOPED ruling (§3)** — ⚠️ **it is NOT §6
    ruling A, which is scoped to the console widget only and says citing it elsewhere is a misuse.** The grounds: with no asset there is
    nothing for a name to drift from; ⛔ **the alternative — a `WBP_` born as a duplicate+reparent — has SILENTLY BROKEN RUNTIME REPAINT on
    this project (design-time fine, ~9 wasted fixes)**; and the one route avoiding both is a **Jonathan hand-step**, which is not owed for a
    two-control panel.
    **(1) BUILD THE TREE IN `RebuildWidget()` via `WidgetTree->ConstructWidget<>`** — and **construct each child ONLY IF that member is still
    null after `Super::RebuildWidget()`**. Every child is a `UPROPERTY(meta=(BindWidgetOptional))` with the **exact pinned names**, so a
    `WBP_SettingsMenu` authored later **wins automatically with ZERO C++ change.** That escape hatch is a QA criterion, not a nicety.
    **(2) ⚠️ `BackdropBorder` IS HIT-TEST **VISIBLE**, AND THIS IS CORRECTNESS, NOT STYLING — IT IS THE OPPOSITE OF `WBP_SessionMenu`'s
    BACKDROP.** The panel is added **ON TOP of `WBP_MainMenu`** (§4), so a hit-test-INVISIBLE plate lets clicks fall through to
    Play / Sandbox / Deck Builder / Quit **while the panel looks modal.** TASK-355's backdrop was deliberately `HIT_TEST_INVISIBLE` because it
    sat over nothing clickable; **copying that choice here ships a live click-through into "Quit".**
    ⇒ **QA CRITERION: with the panel open, a click on the backdrop reaches NOTHING behind it.**
    **(3) THE ONE SETTING ROW.** `ConfirmToggleCheckBox` seeded from `USiegeSettingsSubsystem::IsAssistantConfirmEnabled()` **and THEN bound**
    (seed-then-bind — a bind-only widget created at a pinned value stays stale, qa/TASK-005 major-2). `ConfirmTogglePressed(bool)` forwards to
    `SetAssistantConfirmEnabled`. Player-facing text is **game-authored** and lives here:
    label **"Confirm AI orders before they execute"**, hint **"Shows the parsed order and its target circles for review. Recommended — the
    assistant can pick the wrong place or the wrong number."** ⚠️ **That hint is the measured truth from ruling 2's table, stated plainly to the
    player; do not soften it into marketing.**
    **(4) `BackPressed()` CALLS `RemoveFromParent(self)` AND NOTHING ELSE.** ⛔ **It does NOT re-create or re-open the main menu** — the menu was
    never removed, it is alive underneath, and re-creating it would put navigation state in the leaf plus a soft asset path to get wrong.
    **(5) NULL-SAFE THROUGHOUT.** No subsystem resolvable ⇒ the row renders **disabled** with the hint text explaining it, logs once, and
    **never crashes** (the `USessionMenuWidget::ShowLocalError` shape). A settings screen that hard-faults on a missing subsystem is worse than
    no settings screen.
    **(6) THE CONTRACT IS CLONED FROM `USessionMenuWidget` EXACTLY:** `BindWidgetOptional` members · `BlueprintCallable` wrappers ·
    **FString/int32/bool/uint8-only `BlueprintImplementableEvent`s.** ⛔ **Never an enum or a struct in a BIE param.**
    **(7) ⚠️ VERIFICATION IS A HUMAN PIXEL CHECK AND YOU CANNOT CLOSE IT YOURSELF (§3 condition (e), ruling 9).** There is no `.uasset` to
    read back and **MCP readback has repeatedly passed on visually-broken UMG here.** **Write down what Jonathan must look at** — it becomes
    TASK-448's checklist. **Do not report "verified" for anything on-screen.**
    ⛔ **NO settings-subsystem edits (TASK-436 owns it), NO assistant code, NO `Content/`, NO Git, NO compile.** New files only.
    **M8 DECLARATION DUTY: state verbatim in the handoff — "adds no replicated property, no new replicated class, no new relevancy tier."**
    Handoff `handoffs/TASK-437-programmer.md`. Post in ⚙️ Dev & QA.
- names: >
    `SettingsMenuWidget.{h,cpp}` · `USettingsMenuWidget` · `BackPressed` / `ConfirmTogglePressed` / `OnSettingsValueChanged`
    (**§8 registry, character-for-character**). **Pinned children:** `RootPanel` (`UVerticalBox`) · `BackdropBorder` (`UBorder`,
    **hit-test VISIBLE**) · `TitleText` (`UTextBlock`) · `ConfirmToggleCheckBox` (`UCheckBox`) · `ConfirmToggleLabelText` (`UTextBlock`) ·
    `ConfirmToggleHintText` (`UTextBlock`) · `BackButton` (`UButton`) · `BackLabelText` (`UTextBlock`).
    🔒 **RESERVED, NOT AUTHORED: `/Game/UI/WBP_SettingsMenu`** — the zero-C++-change fallback; nothing else may take that path.
    Law: CONVENTIONS "Settings screen…" §3, §4, §8 · "Widgets with C++ bases" · the template-donor / duplicate+reparent prohibition.

#### TASK-438 — [SET-3] `Btn_Settings` — the main-menu entry, ADDITIVE (art-director, EXCLUSIVE editor)
- assignee: art-director
- status: ✅ **done — COMMITTED `87c4784` (2026-08-03): `TASK-438: Btn_Settings on WBP_MainMenu — the main-menu entry`.** 📋 **MANAGER FLIP 2026-09-07, ⛔ 35 DAYS LATE, ⛔ ON GIT EVIDENCE** — `handoffs/BOARD-STALENESS-audit.md` §2/§5.2 (build-master's git-resolved sweep; ⛔ the manager holds ⛔ NO `Bash`, `SC-§71b`). ⛔ **LOGGED EXCEPTION TO WRITE-DISCIPLINE RULE 1** — `status:` is the ⛔ assignee's field; the ⛔ commit host never flipped it, so this row asserted a falsehood for ⛔ 35 days. 🚨🚨 **AND THIS ROW IS ⛔ HALF OF THE PROJECT'S SHARPEST BOARD EXHIBIT: ⛔ `TASK-438` WAS FENCED BEHIND `TASK-447` ⛔ AND `TASK-447` WAS FENCED BEHIND `TASK-438` — ⛔ A LITERAL DEADLOCK ⛔ THAT GIT RESOLVED ON 2026-08-03, ⛔ THE SAME DAY, ⛔ IN TWO COMMITS.** ⇒ ⛔ **⛔ NEITHER ROW COULD BE UNBLOCKED BY CONSULTING THE OTHER; the pair is ⛔ UNRESOLVABLE FROM BOARD STATE ALONE and is the ⛔ cleanest proof of *"⛔ the board cannot audit the board"* (rule 7).** ⚠️ **The row's ⛔ own text already admitted *"PLACED + WIRED, done inside TASK-447's session"* — ⛔ the truth was ⛔ IN THE FIELD, ⛔ one line below the status that denied it.** ⛔⛔ **A FLIP IS ⛔ NOT A GO** — the human pixel-check this row defers to ⛔ TASK-448 is ⛔ still owed. Marker `BOARD-STALE-FLIP-2026-09-07`. ← was: ~~✅ **ready-for-integration**~~ (2026-08-03 — handoff `handoffs/TASK-438-artist.md`. **PLACED + WIRED**, done inside TASK-447's session (**PID 9828, handed back RUNNING**). `SettingsBtnClicked` → `CreateWidget("/Script/GitClaudeUnrealTest.SettingsMenuWidget")` → `AddToViewport(ZOrder 10)`, ⛔ **no `RemoveFromParent`** per §4. Pre-flight confirmed the compile landed: the C++ class resolves live. **`add_event`-first pattern used; the `AssignOnClicked` auto-rename trap FIRED and was defeated** (auto-stub displaced off the single-link `Delegate` pin). ⭐ **RULING 3 PROVEN BY NODE IDENTITY, not by DSL text: 101→117 nodes, 0 removed, 96/101 pre-existing byte-identical, and NOT ONE PIN VALUE CHANGED ANYWHERE** — the 5 differing nodes are exactly the splice points; `BuildSandboxButton` diff EMPTY. Two compiles `warnings_as_errors=true`, second silent. **Saved `/Game/UI/WBP_MainMenu` ONLY**; 🔒 **`L_Arena` SHA256 `B3DBC5D9…F8268` / 535,522 B UNCHANGED** before+after; ⛔ zero Git, staging untouched.
  - ⛔ **ON-SCREEN CORRECTNESS IS NOT CLOSED AND WAS NEVER CLAIMED (ruling 9).** A readback cannot see geometry. **TASK-448 owes it**, and TWO NEW human checks are owed beyond the existing list: **(a) clicking Settings must NOT remove the main menu** (Deck Builder/Multiplayer both do `RemoveFromParent`; this one deliberately must not — the likeliest silent defect), and **(b) after Back, every main-menu button must still work**, since the menu was never removed and stayed live underneath.
  - ⚠️ **TRAP FOR THE NEXT READER, recorded on the board so it is not re-derived:** the raw DSL diff *looks* like the Quit button was relabelled "Settings". It was not — the DSL printer renumbers its sequential `_returnvalue_N` binds on any mid-chain insert. **Prove ruling-3 by `get_node_infos` identity diff, never by DSL text.**
  - ⚠️ **2 new inert stub events** (`OnClicked_Event_13`/`_14`, both 0 connections) joined the 9 existing → **the deferred stub sweep now has 11.** Deliberately NOT deleted: TASK-355 measured that deleting an auto-stub mints a replacement, so the delete buys zero cleanup and spends a graph reconstruction next to the five bindings under proof.
  - ⚠️ **`Btn_Settings` names the ENTRY, not an object** — this widget builds menu buttons at RUNTIME (no `Btn_Multiplayer`/`Btn_Quit` variable exists either). The greppable identifier shipped is the handler `SettingsBtnClicked`. Nothing in code binds to it.
- blocked-by: ✅ **DISCHARGED 2026-09-07 BY THE MANAGER — `TASK-447` COMMITTED `cd5f4ed` on 2026-08-03 (⛔ 35 days). ⛔ A DISCHARGE IS ⛔ NOT A GO** (`handoffs/BOARD-STALENESS-audit.md` §5.2). ← was: ~~**TASK-447's COMPILE STEP — and it runs INSIDE that session**~~ (the TASK-355/357 precedent). ⚠️ **The reason is mechanical, not procedural: the `CreateWidget` node cannot pick a C++ class that has not been compiled yet.** Also serializes with TASK-445 (one editor). — ✅ **CLEARED: the class resolved live before any graph edit.**
- parallel-safe: no (EDITOR-GATED, and Jonathan owns the hand-over)
- spec: >
    **Add ONE entry to the EXISTING `/Game/UI/WBP_MainMenu`. Nothing else.**
    **(1) FOLLOW THE SHIPPED IDIOM CHARACTER-FOR-CHARACTER** — it is already proven on this exact asset by TASK-355's "Multiplayer" entry:
    font **28**, `MakeMargin(24,12,24,12)`, `HAlign_Fill`; **granular ops only, `write_graph_dsl` NEVER called**; and the **`add_event`-first
    pattern** that defeats the `AssignOnClicked` auto-rename trap. **Lift the geometry off the shipped buttons — do not invent it.**
    **(2) ORDER: Play (vs Bot) → Sandbox (No Bot) → Deck Builder → Multiplayer → Settings → Quit.** ⚠️ **Splice BEFORE the Quit block so Quit
    stays last** (exactly how Multiplayer was spliced).
    **(3) HANDLER:** `CreateWidget(USettingsMenuWidget)` → **`AddToViewport(ZOrder 10)`**. ⛔ **Do NOT `RemoveFromParent` the main menu** — the
    settings panel sits ON TOP and `Back` removes only itself (§4). ⚠️ **There is no `WBP_SettingsMenu` asset and there must not be one** — you
    are picking the **C++ class** in the node's class field. **If the class does not appear in the picker, the compile did not land: STOP and
    report — do not author a WidgetBlueprint to work around it.**
    **(4) ⛔ THE FIVE PRE-EXISTING ENTRIES ARE UNTOUCHABLE, AND YOU PROVE IT RATHER THAN ASSERT IT: full-graph DSL diff showing all five are
    character-identical** (the ruling-3 idiom TASK-355 used). Do not disturb the Play-vs-Bot / Sandbox / Deck Builder / Multiplayer bindings.
    **(5) COMPILE THE WIDGET with `warnings_as_errors=true`, then COMPILE AGAIN AND CONFIRM SILENCE** — the design-time GUID self-heal is an
    `ensureAlwaysMsgf`, so **silence on the second compile is the proof it persisted** (the lane law earned at TASK-355).
    **(6) SAVE `/Game/UI/WBP_MainMenu` ONLY.** ⛔ **`L_Arena` is NEVER saved or opened.** No PIE. **Zero Git — TASK-447 commits.**
    **Hand the editor back RUNNING and say so with its PID.** ⚠️ **Never force-close it — that is Jonathan's call and he is at the keyboard.**
    **(7) ⚠️ ON-SCREEN CORRECTNESS IS NOT YOURS TO CLOSE (ruling 9).** A binding/tree readback **cannot see geometry** — that is exactly how
    TASK-355 shipped six controls stacked in a 165×48 px corner box with a 6/6 green readback. **Report what you built; list what Jonathan must
    look at.**
    Handoff `handoffs/TASK-438-artist.md`. Post in 🎨 Art.
- names: >
    `Btn_Settings` on `/Game/UI/WBP_MainMenu` (label **"Settings"**) → `CreateWidget(USettingsMenuWidget)` → `AddToViewport(ZOrder 10)`.
    Law: CONVENTIONS "Settings screen…" §4 · "Dev / test tooling" (the `Btn_Sandbox` additive-entry precedent) · the UMG verification law.

#### TASK-439 — [SET-QA1] QA gate covering TASK-436 + TASK-437 (qa-reviewer)
- assignee: qa-reviewer
- status: **done** (2026-08-03 — report `qa/TASK-439.md`. **PASS on both**, 0 blockers. Slack posted in ⚙️ Dev & QA. Board flips applied by the ORCHESTRATOR, not by QA. Two findings escalated beyond this gate: (i) ⛔ **`CONVENTIONS.md:1195` carries a wrong widget-construction ordering** that QA re-derived from the engine source and that will blank the next code-authored widget — routed to manager; (ii) the `NativeOnInitialized` asymmetry between WBP and code-authored trees, owed the same section. ⛔ **This gate did NOT certify the toggle's off-path** — TASK-443 owes that trace and TASK-446 closes it.)
- blocked-by: TASK-436 + TASK-437 (both `ready-for-qa`) — **both cleared**
- parallel-safe: yes vs the assistant lane's QA (different report, read-only)
- spec: >
    **Pre-compile review. Read the landed files, not the handoffs describing them** (the RELAYED-DIAGNOSIS LAW: a relayed diagnosis is a lead,
    and **QA re-derives the root cause from the source before accepting it**).
    **Verify at minimum:** the **§8 pinned signatures character-for-character** · the **slot name `"SiegeSettings"` exactly** ·
    **default `true`** · **`Initialize` degrades to defaults on a missing/wrong-class slot and logs once, never crashes** · **no
    `LoadGameFromSlot` on any read path** · **`OnSettingsChanged` does not fire on a same-value write** · **no `.ini` edit anywhere** ·
    **seed-then-bind on the checkbox** · ⚠️ **`BackdropBorder` is hit-test VISIBLE** (ruling: a hit-test-invisible backdrop is a live
    click-through into "Quit" and is a **BLOCKER**, not a nit) · **every child is `BindWidgetOptional` and constructed only when null** (the
    escape hatch is a criterion) · **BIE params are FString/int32/bool/uint8 only** · **the M8 declaration is present VERBATIM in both handoffs**.
    ⚠️ **`Grep` MANGLES COMMENT SYNTAX ON THIS MACHINE** (`//` renders as `\`; three occurrences, two files, two lanes). ⇒ **Comment-syntax and
    structural analysis is done on RAW `Read` output. `Grep` LOCATES; it does not READ.** Say which tool produced any text you quote.
    ⚠️ **STATE WHAT YOU DID NOT RUN.** Nothing here has been compiled and no pixel has been rendered. **"Reviewed by reading" is legitimate,
    useful and INCOMPLETE — report it as incomplete and name the parser you did not run** (§9c's promoted law). ⛔ **Reporting it as a PASS is
    how a grammar that had never once loaded reached a measurement run.**
    Report `qa/TASK-439.md`. Post the verdict in ⚙️ Dev & QA.
- names: > Law: CONVENTIONS "Settings screen…" §2, §3, §8 · the RELAYED-DIAGNOSIS LAW · §9c (the parser is the reviewer of record) · §10's `Grep` trap.

#### TASK-440 — [W1-B1] `CreateUnitGroup` extraction + the assistant component subobject + the mutual-exclusion guards (gameplay-programmer) — ⛔ CRITICAL PATH
- assignee: gameplay-programmer
- status: **done — COMMITTED 2026-08-03 in `cd5f4ed`** (Wave 1 code. Build GREEN: both DLLs linked, **zero errors, zero warnings** — the link step ran for the first time, so the 22-seam pre-flight became a measurement. ⚠️ **Green means it BUILDS, not that it works**: no automation test has ever run (33 compile; compiling is not passing), WARN-5 stays instrumented-not-discharged, and every playable check belongs to TASK-448. Flip applied by the ORCHESTRATOR after manager handed the board back.) ← prior: ✅ **qa-passed → ready-for-integration** (2026-08-03 — `qa/TASK-446.md`: **PASS · 0 BLOCKERS · 9 WARN · 6 NIT** across all eleven tasks. Flip applied by the manager, not QA — QA has no partial-edit tool and the board was being written concurrently.)
- status: **ready-for-qa** (2026-08-03 — handoff `handoffs/TASK-440-programmer.md`; ⛔ NOT compiled, see that handoff §7)
- blocked-by: **none — DISPATCHABLE NOW** (⚠️ see ruling 7: the file is DIRTY with uncommitted FOLLOW work and that is CORRECT)
  - ⚠️ **RULING 7 IS STALE AS OF THIS TASK'S PRE-FLIGHT — THE FILES CAME BACK CLEAN, NOT DIRTY.** Jonathan committed the FOLLOW lane himself as **`e70f5ba` "follow command, and new miner behavior rework"** (Aug 2). ⇒ ✅ **TASK-447 NO LONGER OWES AN ATTRIBUTION SPLIT on `SiegePlayerController.{h,cpp}`** — they were clean when TASK-440 started, so their entire working diff is TASK-440 and the two lanes are already separated **by commit**, which is stronger than by attribution. ⚠️ **Flag (p) still stands: TASK-402's FOLLOW/miner playtest gate has still NOT run, and the assistant plumbing now sits on top of it in the same build.**
- parallel-safe: yes to WRITE (single owner); ⛔ **never beside a compile gate.** **EXCLUSIVE owner of `SiegePlayerController.{h,cpp}`**
- spec: >
    **(0) ⚠️ PRE-FLIGHT, AND THE EXPECTED RESULT IS "DIRTY".** `git status --porcelain` on both files. **TASK-395..398's Follow/Miner work is
    qa-passed, compiled green and UNCOMMITTED** (TASK-403 waits on Jonathan's TASK-402 playtest). ⛔ **Do NOT revert it, rebase it, "clean it
    up", or merge it. Edit ADDITIVELY on top of it.** ⛔ **`git reset --hard` / `clean -fd` are BANNED** — they are what destroyed a whole
    session's board work once already. **If anything looks wrong, STOP and report; do not repair another lane's file.**
    **(1) EXTRACT `ConfirmGroupPickStage`'s stage-3 body into a public
    `CreateUnitGroup(Type, PosCenter, PosRadius, AtkCenter, AtkRadius, Members, PosMarker=nullptr, AtkMarker=nullptr)`** so the cursor pick
    calls it too and **there is ONE implementation**. ⚖️ **QA ACCEPTANCE IS "PROVABLY ZERO BEHAVIOR CHANGE" BY READ-THROUGH (the TASK-397
    idiom) — that is the whole deliverable and it is why this task is alone in this file.** An extraction that "tidies" one line while moving it
    is not an extraction; it is an untested change wearing a refactor's clothes.
    **(2) THE `USiegeAssistantComponent` DEFAULT SUBOBJECT** (created in the constructor; the class is TASK-442's — you create it, you do not
    author it).
    **(3) ONE `ApplyCursorInputState` TERM** for the console's cursor posture. ⚠️ **`ApplyCursorInputState` composes the ONLY cursor owners
    (placement, Alt-held `IA_UICursor`, match-end) — add a term, never a parallel path**, or the level-travel input law starts leaking again.
    **(4) FOUR MUTUAL-EXCLUSION GUARDS** against placement mode / targeting mode / group-pick / the console. **They must be symmetric** — the
    console refuses to open during the other three, and the other three refuse to start while it is open.
    **(5) EXPOSE WHAT THE CONFIRM STEP NEEDS, MINIMALLY.** `SpawnGroupCircleDecal(float Radius)` must be reachable by the component for the
    ghost circles. **Widen access no further than required and prove zero behavior change.**
    ⛔ **NOTHING ELSE. No unit edits, no component body, no `Content/`, no Git, no compile.**
    ⚠️ **§2 IS THE LAW THIS TASK IS MOST ABLE TO BREAK: every keyboard command must still work BYTE-IDENTICALLY** — `IA_CmdFollow` /
    `IA_CmdAttack` / `IA_CmdHold` / `IA_CmdDefend` / `IA_CmdAmbush` / `IA_Rally` / cards 1–6 keep their exact shipped behaviour and their exact
    shipped code paths. **The assistant calls the same public APIs the keys call — never a parallel implementation, never a "better" one.**
    **M8 DECLARATION DUTY: state verbatim in the handoff — "adds no replicated property, no new replicated class, no new relevancy tier."**
    Handoff `handoffs/TASK-440-programmer.md` — including **a read-through argument for the extraction, function-by-function**, and an explicit
    **list of which lines in the file are YOURS vs the FOLLOW lane's** (TASK-447 splits the commit BY ATTRIBUTION because pathspec cannot).
    Post in ⚙️ Dev & QA.
- names: >
    `SiegePlayerController.{h,cpp}` · `CreateUnitGroup` · `ConfirmGroupPickStage` · `ApplyCursorInputState` · `SpawnGroupCircleDecal` ·
    `USiegeAssistantComponent` (subobject only). Law: CONVENTIONS "In-match LLM command assistant" §2 (strictly additive) ·
    "Input-mode ownership (level-travel law)" · "Settings screen…" §9 (the attribution split) · the QUIET-MODULE LAW.

#### TASK-441 — [W1-GUARD] Orderability accessors + the NON-ORDERABLE-KIND GUARD + automation tests (gameplay-programmer)
- assignee: gameplay-programmer
- status: **done — COMMITTED 2026-08-03 in `cd5f4ed`** (Wave 1 code. Build GREEN: both DLLs linked, **zero errors, zero warnings** — the link step ran for the first time, so the 22-seam pre-flight became a measurement. ⚠️ **Green means it BUILDS, not that it works**: no automation test has ever run (33 compile; compiling is not passing), WARN-5 stays instrumented-not-discharged, and every playable check belongs to TASK-448. Flip applied by the ORCHESTRATOR after manager handed the board back.) ← prior: ✅ **qa-passed → ready-for-integration** (2026-08-03 — `qa/TASK-446.md`, PASS · 0 blockers)
- status: **ready-for-qa** (2026-08-03 — `handoffs/TASK-441-programmer.md`). ⛔ **NOT COMPILED, NOT RUN — one compile gate only (TASK-447). "The tests are written" is the only true statement; "the tests pass" is not claimable yet.** ⛔ **AND THE HEADLINE STAYS THE HEADLINE: THIS GUARD DOES NOT MAKE `DEV-04` PASS THE EVAL AND IS NOT PROGRESS ON BAR #5** — the eval scores EMITTED JSON, this refuses an EXECUTED ACTION; **bar #5 stands at 20/25 against 22 + zero refuse-class failures, uncleared.** ✅ **PUBLISHED FOR TASK-443, character-for-character:** `bool IsKindOrderable(FName) const` · `int32 GetOrderableCount(FName) const` · `enum class ESiegeAssistantRejectReason : uint8 { None, KindNotOrderable, KindUnknown }` · `bool ValidateCommandAgainstSnapshot(const FSiegeAssistantCommand&, const TArray<FSiegeAssistantRosterEntry>&, ESiegeAssistantRejectReason&, FName&)`. **Both out-params written on EVERY path incl. success** (the deliberate opposite of `ResolvePlace`); a multi-kind order with one bad kind is refused **as a whole**, naming the **first** offender. ⚠️ **TWO THINGS QA MUST RULE ON, DECLARED NOT SLIPPED IN: (1) `FSiegeAssistantRosterEntry` GAINED ONE FIELD — `int32 Orderable = 0` (appended).** The §9 struct pin and the §8 validator pin are **not simultaneously satisfiable** otherwise: the three pinned fields answer *"is it here"* and cannot answer *"may it be ordered"*, yet the signature must tell `KindNotOrderable` from `KindUnknown` **from the roster array alone** — and the three alternatives each break something explicitly forbidden (a `const USiegeAssistantSnapshot*` param is *"rejected on sight"*, a 5th param breaks the pin TASK-443 links against, one merged reason breaks the pinned 3-value enum). ✅ **Strictly additive, PROMPT-BYTE-NEUTRAL** — no zone prints `Roster` (the roster block prints from the parallel arrays), so **`zoneB_chars=68` / `zoneC_chars=887` cannot move and the t0 tripwire cannot fire** — and **zero external consumers exist** (`FSiegeAssistantRosterEntry`/`GetRoster()` have no reference anywhere else in `Source/` or `Plugins/`). Filled by **one line inside the branch that already calls `IsGroupCommandEligible()`** — no re-tally, no extra traversal, **no registry** (§4 cited). **(2) ONE NEW FILE outside the `names:` line — `Siegebound/Tests/SiegeAssistantGuardTest.cpp`**, under the shipped `Siegebound/Tests/` precedent (TASK-436 landed `Tests/SiegeSettingsTest.cpp` in the same wave); owned by no other task, collides with nothing, **a file move if QA rules otherwise.** ⚖️ **THE SCOPING DECISION THAT KEPT THIS FROM SHIPPING A REGRESSION: `KindNotOrderable` is scoped to `Send`/`Guard`/`Ambush` ONLY.** `IsGroupCommandEligible()` gates the ZONE orders, so the **Cleric** is `Count>0, Orderable==0` — **a guard refusing every 0-orderable kind for every verb would have refused *"clerics follow me"*, a legal shipped order and verbatim the eval's own `DEV-20` utterance.** `KindUnknown` is NOT scoped (a hallucinated unit is one whatever the verb). ⚠️ **DECLARED v1 GAP:** a `Follow` naming a non-followable kind is not refused here — the pinned enum has no such reason and the executor's `IsFollowCommandEligible()` selector sends it to the **shortfall** path; a test pins it so it is the assertion that flips if `KindNotFollowable` is ever added. **9 tests, all model-free / world-free / PIE-free** (hand-populated roster), incl. the **DEV-04 reproduction** and the **Cleric regression test**. Guard logs at **`Log`, never `Warning`** — a refusal is the MODEL being wrong, and a logged Warning fails the automation runner. 📌 **OWED TO TASK-447's FIRST-EXECUTION AUDIT (the one live proof this task cannot self-serve): confirm `sum(Roster[kind].Orderable)` equals the `orderable=` figure the printed roster line reports for that kind — same per-unit call, so they must agree exactly.** M8: *"adds no replicated property, no new replicated class, no new relevancy tier."*
- blocked-by: **none — DISPATCHABLE NOW**
- parallel-safe: yes to WRITE; **EXCLUSIVE owner of `SiegeAssistantSnapshot.{h,cpp}`**
- spec: >
    **Read CONVENTIONS "Settings screen…" §6 and §8 first.**
    **(1) WHY THIS EXISTS — AND THE HONEST LIMIT COMES FIRST, BEFORE THE WORK.** §1 says *"grammar guarantees existence, **executor guarantees
    legality**, FSM owns the conversation"*, and §12f **measured that the executor half was asserted in the document and absent from the code**:
    `DEV-04` emitted a live, well-formed order for a kind the roster line itself prints as **`0 orderable`**. Orderability is per-match **STATE**,
    not identity; §1 forbids the grammar from encoding it and four prompt attempts have not taught it ⇒ **only the command layer can refuse it.**
    ⛔ **AND IT DOES NOT MAKE `DEV-04` PASS THE EVAL. THE EVAL SCORES EMITTED JSON, NOT EXECUTED ACTIONS.** ⚠️ **If your handoff reports this as
    progress on bar #5 — under any wording — it is failed at the gate.** This is shipped safety. Say so in those words.
    **(2) EXPOSE WHAT THE SNAPSHOT ALREADY COMPUTES.** `KindOrderable` is tallied today and is **private** (`SiegeAssistantSnapshot.cpp` around
    `:490` / `:559` / `:565`, printed at `:1131`). Add **`bool IsKindOrderable(FName) const`** and **`int32 GetOrderableCount(FName) const`**.
    ⛔ **Do NOT recompute, re-tally, or "optimize" the existing pass, and do NOT add a unit registry** (§4 — a registry is rejected on sight and
    you cite that clause). ⚠️ **Verify the tally's real semantics from the CODE before you expose it** — the header's own comment distinguishes
    `orderable` (*"can I send these?"*) from `followable` (*"can these follow?"*), and **exposing the wrong one is a silent wrong-answer defect.**
    **(3) THE GUARD IS A PURE FREE FUNCTION, AND ITS SHAPE IS THE POINT:**
    `bool ValidateCommandAgainstSnapshot(const FSiegeAssistantCommand&, const TArray<FSiegeAssistantRosterEntry>& Roster,
    ESiegeAssistantRejectReason& OutReason, FName& OutOffendingKind)`.
    ⚠️ **IT TAKES THE ROSTER ARRAY, NOT THE SNAPSHOT OBJECT — deliberately, for the same reason `USiegeAssistantGrammar::Build` takes
    `TArray<FName>`: a pure function over plain data needs NO `UWorld`, NO `Capture()` and NO engine state to test.** ⛔ **A later "tidy-up"
    that changes it to take a `const USiegeAssistantSnapshot*` deletes that property and is rejected on sight.**
    **(4) AUTOMATION TESTS — this is the cheap half and it is mandatory.** Under `#if WITH_DEV_AUTOMATION_TESTS`, against a **hand-populated
    roster**, no model, no PIE: a kind with `Orderable > 0` **passes** · a kind present but `0 orderable` **is rejected with
    `KindNotOrderable` and names the offending kind** · a kind absent entirely **is rejected with `KindUnknown`** · a **multi-kind selection
    where only ONE kind is bad is rejected as a whole** (⚠️ **never silently dropped — a silently-dropped kind is the valid-shaped-wrong-command
    failure this architecture exists to prevent**) · an empty roster · and `who:"none"` army-wide verbs, which have **no kind to validate and
    must NOT be rejected**.
    **(5) ⛔ THE REFUSAL REUSES THE EXISTING UNSUPPORTED-ASK OUTCOME AND INVENTS NO NEW PLAYER-FACING SURFACE.** `{"ask":"unsupported"}` already
    exists in the grammar and already has a template. You return a **reason code**; TASK-443 routes it. ⛔ **No player-facing string is produced
    in this file** (§3).
    ⛔ **NO component code, NO controller edits, NO grammar edits** (⚠️ **`SiegeAssistantGrammar.cpp` is off limits — tightening the grammar to
    encode board state is introducing the defect §1 names, not fixing one**), **NO `Content/`, NO Git, NO compile.**
    **M8 DECLARATION DUTY: state verbatim in the handoff — "adds no replicated property, no new replicated class, no new relevancy tier."**
    Handoff `handoffs/TASK-441-programmer.md`. Post in ⚙️ Dev & QA.
- names: >
    `SiegeAssistantSnapshot.{h,cpp}` · `IsKindOrderable` / `GetOrderableCount` · `ValidateCommandAgainstSnapshot` ·
    `ESiegeAssistantRejectReason` { `None`, `KindNotOrderable`, `KindUnknown` } · `FSiegeAssistantRosterEntry` · `LogSiegeAssistant`
    (**§8 registry, character-for-character**).
    Law: CONVENTIONS "Settings screen…" §6, §8 · "In-match LLM command assistant" §1, §3, §4 (the unit-registry rejection), §9.

#### TASK-443 — [W1-B2b] The EXECUTOR + the CONFIRM STEP + the toggle + the guard call + the deferred intent (gameplay-programmer)
- assignee: gameplay-programmer
- status: **done — COMMITTED 2026-08-03 in `cd5f4ed`** (Wave 1 code. Build GREEN: both DLLs linked, **zero errors, zero warnings** — the link step ran for the first time, so the 22-seam pre-flight became a measurement. ⚠️ **Green means it BUILDS, not that it works**: no automation test has ever run (33 compile; compiling is not passing), WARN-5 stays instrumented-not-discharged, and every playable check belongs to TASK-448. Flip applied by the ORCHESTRATOR after manager handed the board back.) ← prior: ✅ **qa-passed → ready-for-integration** (2026-08-03 — `qa/TASK-446.md`, PASS · 0 blockers). ⚖️ **THE OFF-PATH DEBT IS CLOSED BEHAVIOURALLY, NOT BY SCOPE** — `IsConfirmBeforeExecuteEnabled` has ONE definition and ONE call site, at the LAST routing step, with the guard at step 1; every machine check is upstream of the branch or downstream of both arms, and it **fails safe to ON**. ✅ **Discharged by a reviewer who TRACED it rather than accepting the author's account** — the one debt no other task could close.
- status: **ready-for-qa** (2026-08-03 — `handoffs/TASK-443-programmer.md`; reviewed under TASK-446). ⛔ **NOT COMPILED, NOT RUN** — one gate only (TASK-447). **All EIGHT TASK-442 seams filled IN PLACE**, no name/signature/call-site moved. ✅ **THE OFF-PATH TRACE IS DISCHARGED — the one duty TASK-436 could not certify and TASK-439's QA explicitly refused to certify.** Verified from the delivered file by grep, not asserted: the toggle is read in **exactly one function** (`IsConfirmBeforeExecuteEnabled`, defined `:1987`) from **exactly one call site** (`RouteParsedCommandInternal` `:1200`, its LAST step), and **`ValidateCommandAgainstSnapshot` runs at `:1134` — 66 lines EARLIER**. ⇒ **All 20 machine checks lie strictly before the branch or downstream of BOTH arms; the only difference between ON and OFF is whether a human looks.** ⛔ **NO BLOCKER FOUND — stated as a finding, not an absence.** Fails safe (`Settings ? … : true`), and `bForceConfirmReview` (the deferred fire) ORs in so it **can only force review ON**. ✅ **`ExecuteAndReport()` is ONE body with TWO callers** (`ConfirmPressed` ON, `RouteParsedCommandInternal` OFF) so ON/OFF cannot drift — ⚠️ **that required a DECLARED edit to TASK-442's finished `ConfirmPressed`** (statement-for-statement identical), plus `EndPlay += DetachConsoleWidget()` and the seam-map comment; **all three revert mechanically if QA rules against them.** ✅ **`Build.cs`: `+ "SiegeLlama"` ONLY** (⛔ not HTTP, not Sockets; `LlamaCpp` stays private so no `llama.h` leak) **and the stale `SlateCore` example RE-POINTED** from the deleted probe to `SiegeAssistantConsoleWidget.{h,cpp}` — ⛔ **the dependency was NOT touched** (§12). ✅ **Mid-task ruling honoured: `AttachConsoleWidget(USiegeAssistantConsoleWidget*)` implemented for TASK-453 to call** — null-safe, idempotent, re-targetable, safe at ANY point in the widget's life; binds all 8 forwards (1 adapter, for the one-delegate→two-entry-points arity); **SEEDS BEFORE BINDING via the new `GetPendingConfirmSummary()`, which closes the lazy-creation trap** (a deferred fire can enter `AwaitConfirm` before the widget exists). ⛔ **Did NOT call it and did NOT touch the controller.** ⚠️ **TWO NEW DELEGATES declared for QA to rule on** — `OnAssistantConfirmPromptShown/Hidden`, the channel TASK-442's forward table listed as mine but left with no wire; deliberately NOT folded into `OnAssistantMessage` (transcript = history, confirm prompt = live modal state gating Accept). 🚩 **FOUR FLAGS, the first consequential: (a) `charge`/`fallback` DO LESS THAN THE T/E KEYS** — §8 pins `SetUnitCommand` and that runs, but the keys also call `CancelGroupPick()` + **`ClearAllUnitGroups()`** (the TASK-344 release law) and **both are `private`**, so standing group orders survive an assistant "charge"; re-implementing is the parallel implementation §2 forbids ⇒ **fix is one public entry point on the controller, another task's file**. **(b)** an assistant-formed group owns **no persistent ground markers** (ghosts are torn down before execution by TASK-442's correct ordering, so there is nothing to transfer). **(c)** two **paired tunables** mirror `protected` controller radii (700/1500) — named, not hard-coded; keep in lockstep. **(d)** `IntentTakesZoneOrder` now exists file-locally in **two** files (mine + TASK-441's) and they must agree. ⚖️ **DEV-11 RULED:** an already-satisfied deferred trigger **RE-ASKS** (existing `AskHowMany`) rather than firing — the relative "N MORE" is indistinguishable from absolute in the JSON, and firing converts a known ambiguity into a confident action; the alternative is one branch if QA rules otherwise. ⛔ **A deferred fire re-`Capture()`s, re-runs the guard, and returns to `AwaitConfirm` EVEN WITH THE TOGGLE OFF — it never executes blind.** ⛔ **No new player-facing string anywhere** — every outcome routes through an existing reason code (§3/§6). ⛔ **No registry/cache/dirty-flag/subscription (§4), no multi-turn loop, no request queue, sealed holdout untouched.** ⚠️ **`#include "SiegeLlamaSubsystem.h"` references a file that DOES NOT EXIST — `USiegeLlamaSubsystem` is TASK-450 (`backlog`), verified against the repo not the board; the ordering is already enforced (TASK-447 ⊃ TASK-424 ⊃ TASK-450) but if 450 has not landed the gate fails naming my include — dispatch it, do not loop QA.** ⛔ **Nothing here is accuracy progress: the eval scores EMITTED JSON, this changes what the game DOES. Bar #5 stands at 20/25 vs 22 + zero refuse-class failures, UNCLEARED.** M8 duty stated verbatim, with the reason.
- blocked-by: **TASK-442** (same file, sequential — ⛔ never concurrent) **+ TASK-441** (its validator)
- parallel-safe: no; **EXCLUSIVE owner of `SiegeAssistantComponent.{h,cpp}` and of `Source/GitClaudeUnrealTest/GitClaudeUnrealTest.Build.cs`**
- spec: >
    **Read CONVENTIONS "Settings screen…" §5 and §6, and "In-match LLM command assistant" §1–§3, first.**
    **(0) PRE-FLIGHT:** `git status --porcelain Source/GitClaudeUnrealTest/GitClaudeUnrealTest.Build.cs` — **if dirty, STOP and report.**
    Add the **`SiegeLlama` plugin dependency** (this is the commit where the game lane first calls the subsystem — TASK-417's spec deliberately
    deferred it to here). ⛔ **Still NOT `HTTP`, NOT `Sockets`** — in-process llama.cpp needs neither and the sidecar was rejected (a Defender
    firewall prompt on first launch of a shipped game is unacceptable). **Re-opening that is out of scope.**
    **(1) THE EXECUTOR AND THE SELECTOR — AND THE ONE RULE THAT GOVERNS THEM: CALL THE SAME PUBLIC APIs THE KEYS CALL.** Route group creation
    through TASK-440's `CreateUnitGroup`. ⛔ **Never a parallel implementation, never a "better" one** (§2). Every keyboard command must still
    work byte-identically with the console closed — that is Jonathan's own playtest criterion.
    **(2) ⛔ THE GUARD RUNS BEFORE EXECUTION, ALWAYS, AND INDEPENDENTLY OF THE MODEL.** Call TASK-441's `ValidateCommandAgainstSnapshot`
    against the **live** snapshot and route a rejection to the **existing unsupported-ask outcome**. ⚠️ **A multi-kind selection with one bad
    kind is rejected AS A WHOLE — never silently dropped.** ⛔ **This does not make `DEV-04` pass the eval and may not be reported as accuracy
    progress.**
    **(3) THE CONFIRM STEP — `AwaitConfirm`, AND IT IS A HARD ACCEPTANCE CRITERION, NOT POLISH (§1).** Show (i) a **game-authored** one-line
    summary of the parsed order from the reason-code template table — ⛔ **never a model-produced string** — and (ii) **the ghost circles on the
    ground** via `SpawnGroupCircleDecal`. **Accept executes. Cancel discards, returns to `Idle`, and leaves NOTHING partially executed.**
    ⚖️ **Constrained decoding guarantees SYNTAX, never SEMANTICS — it cannot prevent a valid-shaped wrong command, and the preview is what
    guarantees the rest.** ⚠️ **Ruling 2's table is the measured evidence for this: 4 of 5 stable failures are wrong-place / wrong-count.**
    **(4) THE TOGGLE — READ IT LIVE FROM `USiegeSettingsSubsystem::IsAssistantConfirmEnabled()` AT CONFIRM TIME** (in-memory getter; the
    subsystem is on the GameInstance so it survives the menu → arena travel). **OFF ⇒ skip `AwaitConfirm` and execute.**
    ⛔ **THE LAW THAT MAKES THE TOGGLE SAFE, AND IT IS THE MOST IMPORTANT SENTENCE IN THIS SPEC: THE TOGGLE REMOVES A HUMAN REVIEW STEP AND
    NEVER A MACHINE CHECK.** OFF does **not** skip the parse, the `Kinds.Num() == Counts.Num()` invariant, the shortfall/clarification path,
    eligibility, the authority refusal, or the guard in (2). ⚠️ **The tell: every check that runs with the toggle ON runs with it OFF.**
    ⚠️ **Subsystem unresolvable ⇒ behave as if confirm is ON.** **Failing safe means MORE review, never less.**
    **(5) THE DEFERRED INTENT:** latched, kind+count trigger, **120 s TTL**, **1 Hz timer armed ONLY while latched**, **re-resolves on fire and
    returns to `AwaitConfirm`** — ⛔ **it NEVER executes blind**, and ⚠️ **that is true with the confirm toggle OFF as well**: a deferred order
    fires on a board the player has not looked at since they typed it, which is the one case where the review is worth more, not less.
    **(6) MODEL CALL:** exactly one `USiegeLlamaSubsystem::RequestCompletion` per turn, **queue depth 1** (a second request returns false —
    that is the design, not a bug), completion **always on the game thread**. ⛔ **No multi-turn loop, no conversation state, no feeding output
    back in** (§1).
    ⛔ **No controller edits, no snapshot edits, no grammar edits, no `Content/`, no Git, no compile.**
    **M8 DECLARATION DUTY: state verbatim in the handoff — "adds no replicated property, no new replicated class, no new relevancy tier."**
    ⚠️ **AND SAY WHY: the settings value is client-local and governs a LOCAL review step, never an authoritative outcome.**
    Handoff `handoffs/TASK-443-programmer.md` — with **the toggle's ON and OFF paths traced side by side, check for check**, so QA can verify
    (4) by reading rather than by trusting. Post in ⚙️ Dev & QA.
- names: >
    `SiegeAssistantComponent.{h,cpp}` · `GitClaudeUnrealTest.Build.cs` (**adds `SiegeLlama` ONLY**) · `CreateUnitGroup` ·
    `SpawnGroupCircleDecal` · `ValidateCommandAgainstSnapshot` / `ESiegeAssistantRejectReason` · `USiegeSettingsSubsystem::IsAssistantConfirmEnabled` ·
    `USiegeLlamaSubsystem::RequestCompletion` / `IsReady` / `IsBusy` / `CancelActiveRequest` · `FSiegeLlamaCompletionSignature` ·
    `DeferredIntentTTLSeconds` **120** · `DeferredIntentPollHz` **1** (**§9 + §8 registries, character-for-character**).
    Law: CONVENTIONS "Settings screen…" §5, §6, §8 · "In-match LLM command assistant" §1, §2, §3, §9, §10.

#### TASK-444 — [W1-B3] `USiegeAssistantConsoleWidget` — the in-game text input (gameplay-programmer)
- assignee: gameplay-programmer
- status: **done — COMMITTED 2026-08-03 in `cd5f4ed`** (Wave 1 code. Build GREEN: both DLLs linked, **zero errors, zero warnings** — the link step ran for the first time, so the 22-seam pre-flight became a measurement. ⚠️ **Green means it BUILDS, not that it works**: no automation test has ever run (33 compile; compiling is not passing), WARN-5 stays instrumented-not-discharged, and every playable check belongs to TASK-448. Flip applied by the ORCHESTRATOR after manager handed the board back.) ← prior: **ready-for-qa** — 2026-08-03, **QA LOOP 1 FIX LANDED** (supersedes the `qa-failed` opened by TASK-447's compile gate; the source-review PASS below still stands — the gate found **compile** defects, not review defects). **All 7 diagnostics in `SiegeAssistantConsoleWidget.cpp` addressed: 2 × `C2445` ambiguous ternary (`.Get()` collapses both arms to one type) + 5 × `C4458` locals named `Slot` shadowing the inherited `UWidget::Slot` (renamed to the file's own `<Purpose>Slot` idiom, which 3 sibling sites already used).** ⛔ **§22 SWEEP RUN ON BOTH SHAPES, AND IT IS THE RESULT, NOT THE PATCH:** all **9** conditionals in the file classified — **the 2 cited are the only ambiguous pair**, the other 7 are same-type arms; and a scripted intersection of **123** identifiers I declare against **163** inherited data members across `UWidget`/`UUserWidget`/`UVisual`/`UObject` returns **`Slot` and nothing else** — run with a **positive control** (the probe was proven able to find `Slot` before its zero was trusted, per the TASK-454 pattern). ⚠️ **NOT claimed: that the TU now compiles, or that the link resolves** — the link step has never run, and a failing TU reports only its own errors. ⛔ No compile, no Git, no editor/MCP/PIE. Handoff appended: `handoffs/TASK-444-programmer.md` §9.
- status: ✅ **qa-passed → ready-for-integration** (2026-08-03 — `qa/TASK-446.md`, PASS · 0 blockers) — ⛔ **then `qa-failed` at the TASK-447 compile gate, 7 errors, loop 1 of 3** (`qa/TASK-446.md` build-master appendix)
- status: ready-for-qa — 2026-08-03. `SiegeAssistantConsoleWidget.{h,cpp}` NEW (code-authored tree, ruling A, all five conditions); `SiegeAssistantInputProbe.{h,cpp}` **DELETED** with its three `Siege.Assistant.InputProbe*` commands — **zero compiled references survive** (two comment-only mentions remain in files I do not own: `GitClaudeUnrealTest.Build.cs:30` and the plugin's `VERSION.md:160`). ⛔ **`SlateCore` MUST STAY in `Build.cs`** — the console widget is now the module's only `ETextCommit`-in-a-`UFUNCTION` / `FSlateApplication` user, so the dep is still load-bearing and only that comment's *example* is stale (TASK-443 owns the file). 🚩 **GAP FLAGGED: nothing in Wave 1 opens the console** — no key binding, no creation site, and no `ApplyCursorInputState()` posture owner (a `SiegePlayerController` edit outside this task); as the wave stands there is nothing for TASK-448 to type into. ⚠️ **UI rendering is NOT self-verifiable and is not claimed** — ruling A(e), checklist in the handoff. No compile, no Git, no editor/MCP/PIE. Handoff: `handoffs/TASK-444-programmer.md`
- blocked-by: **none — DISPATCHABLE NOW** (compiles against the §9 pin; will not compile alone and is not expected to)
- parallel-safe: yes to WRITE; **EXCLUSIVE owner of `SiegeAssistantConsoleWidget.{h,cpp}`; DELETES `SiegeAssistantInputProbe.{h,cpp}`**
- spec: >
    **Read CONVENTIONS "In-match LLM command assistant" §6 ruling A (its five conditions are QA criteria) and §9 first.**
    **(1) THE TREE IS CODE-AUTHORED IN `RebuildWidget()` AND THERE IS NO `.uasset`** — ruling A, whose scope is **this widget** (⚠️ the settings
    widget has its **own separate** ruling; neither inherits from the other). Every child is `UPROPERTY(meta=(BindWidgetOptional))` with the
    pinned names and is **constructed only if still null after `Super::RebuildWidget()`**, so `/Game/UI/WBP_AssistantConsole` — **RESERVED, not
    authored** — wins later with **zero C++ change**.
    **(2) CLONE `USessionMenuWidget`'s SHIPPED CONTRACT EXACTLY:** `BindWidgetOptional` members · `BlueprintCallable` wrappers ·
    **FString/int32/bool/uint8-only BIEs.** ⛔ **FSM state is pushed as a `uint8`, never an enum.**
    **(3) ⛔ DELETE `SiegeAssistantInputProbe.{h,cpp}`.** It is throwaway by construction (manager ruling 4 of the Wave-0 block) and
    **must not be preserved as a "useful dev tool"** — the input fallback it measured is now a decision, not an open question.
    **(4) THE CONSOLE IS NEVER A REQUIREMENT FOR ANY ACTION (§2).** Opening/closing it must not disturb any key, and a faulted assistant simply
    leaves it disabled.
    **(5) ⚠️ VERIFICATION IS A HUMAN PIXEL CHECK AND YOU CANNOT CLOSE IT (ruling 9, §6 condition (e)).** No `.uasset` exists to read back and
    MCP readback has repeatedly passed on visually-broken UMG here. **Write the checklist for TASK-448; report nothing on-screen as verified.**
    ⛔ **No component edits, no controller edits, no `Content/`, no Git, no compile.**
    **M8 DECLARATION DUTY: state verbatim in the handoff — "adds no replicated property, no new replicated class, no new relevancy tier."**
    Handoff `handoffs/TASK-444-programmer.md`. Post in ⚙️ Dev & QA.
- names: >
    `SiegeAssistantConsoleWidget.{h,cpp}` · `USiegeAssistantConsoleWidget` · pinned children `InputBox` (`UEditableTextBox`) ·
    `TranscriptText` (`UTextBlock`) · `StatusText` (`UTextBlock`) · `ConfirmButton` (`UButton`) · `CancelButton` (`UButton`) ·
    `RootPanel` (`UVerticalBox`). 🔒 **RESERVED, NOT AUTHORED: `/Game/UI/WBP_AssistantConsole`.** **DELETES** `SiegeAssistantInputProbe.{h,cpp}`.
    Law: CONVENTIONS "In-match LLM command assistant" §2, §6 ruling A, §9.

#### TASK-445 — [W1-B4] `IA_AssistantConsole` + the `IMC_Hero` binding + the Enter-key conflict check (art-director, EXCLUSIVE editor)
- assignee: art-director
- status: **done — COMMITTED 2026-08-03 in `97c7c8d`** (input action + IMC_Hero mapping; committed as **LFS pointers, not binaries** — per-kind rule `§25`. ⚠️ That the key actually FIRES is unverified: it is MCP-readback-only and closes at TASK-448. Flip applied by the ORCHESTRATOR.) ← prior: **ready-for-integration** (2026-08-03 — handoff `handoffs/TASK-445-artist.md`)
- ✅ **`Enter` WAS FREE — NOTHING STOMPED, NO ESCALATION NEEDED.** All **6** `IMC_` contexts enumerated (`defaultKeyMappings.mappings`, legacy `mappings`, AND `mappingProfileOverrides`) + `Config/DefaultInput.ini` + a code grep for `EKeys::Enter`/`Return`/`Virtual_Accept`/`NumPadEnter` → **zero hits**. (`bAltEnterTogglesFullscreen=True` is the engine **Alt+Enter** chord, not a bare-`Enter` binding.) Delivered: `/Game/Input/Actions/IA_AssistantConsole.IA_AssistantConsole` (duplicated from `IA_CmdFollow`; 11 properties verified unanimous across all 6 shipped siblings) + `IMC_Hero` **entry 24 = `Enter`**, entries **1–23 byte-identical** and all **5 instanced modifier subobjects** value-verified (incl. the load-bearing `Negate_2`). Path matches `SiegePlayerController.cpp:180` **59/59 chars, compared programmatically**.
- 🛑 **THE EDITOR HARD-DEADLOCKED MID-TASK AND WAS RESTARTED — PID 26700 IS DEAD, THE LIVE EDITOR IS `PID 7844` (running, owns `127.0.0.1:8000`).** `AssetTools.duplicate` → `InternalPromptForCheckoutAndSave` wrote the asset then wedged the process: **108/108 threads in `Wait`**, log frozen 5+ min, **no modal dialog existed** (window enumeration proved it — this was NOT the Save-Content trap). Terminated under the session grant; termination was also the safest option for `L_Arena` (discards in-memory dirt). ⚠️ **THE RESIDENT LLM MODEL IS GONE** — the restart unloaded it; anything expecting a warm model must reload. After restart the asset was **re-verified, not trusted**: nothing was corrupted.
- 🔍 **MCP LAW-GRADE FINDING — `ObjectTools.set_properties` SILENTLY RETURNS `false` UNLESS `values` IS A JSON *STRING*.** Passing a JSON object (the intuitive shape, and the shape `get_properties` *returns*) yields `{"returnValue": false}` with **no error, no log line, and no mutation**. ⚠️ **The `false` return is the ONLY signal — an agent that does not check it AND re-read the data will believe it wrote and it did not.** Also note `instance` must be `{"refPath": …}` for the setter though the getter accepts a bare path. Re-confirms TASK-399's empty-`mappings` trap: the live array is `defaultKeyMappings.mappings`; legacy `mappings` reads `[]` on all 6 contexts.
- ⚠️ **`Content/Input/Actions/IA_AssistantConsole.uasset` IS ALREADY STAGED IN GIT AND THE ARTIST DID NOT DO IT** (`.git/index` touched ~17 s after the save; **GitHub Desktop is running**, PIDs 11748/12236/3060/8980). **No commit was created** (HEAD still `c8c780c`). ⛔ **Deliberately NOT unstaged — `git reset` is itself a Git write and this task is Git-forbidden. TASK-447 must know the file may already be in the index.**
- ⚠️ **NOTHING IS COMPILED OR PLAYED.** That `Enter` actually fires `ETriggerEvent::Started` is **MCP-readback-only and closes at Jonathan's playtest** — readback has passed on broken assets here before. Independent non-MCP corroboration that the mapping is really in the file: `IMC_Hero.uasset` grew **12 702 → 13 179 B** and a raw byte scan finds `/Game/Input/Actions/IA_AssistantConsole` + `Enter` in its name table.
- ✅ **`L_Arena` UNTOUCHED — sha256 `b3dbc5d9…459f8268`, mtime `2026-07-29 03:53:38.9198809`, 535 522 B, IDENTICAL at 6 checkpoints** (incl. before/after the editor kill and before/after the save). Verified by hash, not mtime alone.
- 🚩 **New flag for the manager (recorded, NOT decided):** `Enter` now both **opens** the console and **submits** the line (`SetKeyboardFocus` at `SiegeAssistantConsoleWidget.cpp:844`, `ETextCommit::OnEnter` at `:625`). Slate should consume it before Enhanced Input while the box has focus, so the toggle should not double-fire — **but that is reasoning, not a measurement.** Sits beside flagged item (t). For the playtest gate.
- blocked-by: **none — DISPATCHABLE NOW**, but ⚠️ **EDITOR-GATED: waits on Jonathan's hand-over and serializes with TASK-438**
- parallel-safe: no (one editor instance)
- spec: >
    **(1) CREATE `IA_AssistantConsole`** at `/Game/Input/Actions/IA_AssistantConsole` and **map it in `/Game/Input/IMC_Hero`.**
    **(2) ⚠️ `Enter` IS THE *PROPOSED* OPEN KEY, NOT AN APPROVED ONE** (the arena has no chat, so it reads free). ⛔ **`IMC_Hero` is a BINARY
    `.uasset` THE MANAGER CANNOT TEXT-VERIFY, so the key is NOT assumed free.** **Run the same in-editor conflict check TASK-399 ran for `C`,
    and FLAG — NEVER STOMP.** If `Enter` is taken, **report it and stop**; the alternative key is Jonathan's call, not yours.
    **(3) ⛔ NO EXISTING BINDING IS DISTURBED.** `IA_CmdFollow` (`C`) / `IA_CmdAttack` / `IA_CmdHold` / `IA_CmdDefend` / `IA_CmdAmbush` /
    `IA_Rally` / cards 1–6 keep their exact shipped mappings — **§2 is law and this asset is where it would break first.** Prove it by readback
    of the full mapping list, before and after.
    **(4) `L_Arena` NEVER opened, NEVER saved. No PIE. Zero Git — TASK-447 commits.** Hand the editor back **RUNNING** and report its PID.
    ⚠️ **Never force-close it — Jonathan is at the keyboard and that is his call.**
    ⚠️ **Re-append the gitignored `Saved/Config/WindowsEditor/Engine.ini` Python remote-exec block if you needed it — the editor's shutdown
    rewrite DROPS it every time (confirmed standing behaviour, not a one-off).**
    Handoff `handoffs/TASK-445-artist.md`. Post in 🎨 Art.
- names: >
    `IA_AssistantConsole` at `/Game/Input/Actions/IA_AssistantConsole`, mapped in `/Game/Input/IMC_Hero` (**CONVENTIONS §5 naming law,
    character-for-character**). Law: CONVENTIONS "In-match LLM command assistant" §2, §5 · the flagged item (d) FLAG-never-stomp ruling.

#### TASK-446 — [W1-QA2] QA gate covering TASK-440 · 441 · 443 · 444 (and 442 through 443) (qa-reviewer)
- assignee: qa-reviewer
- status: ✅ **done** (2026-08-03 — `qa/TASK-446.md`: **PASS · 0 BLOCKERS · 9 WARN · 6 NIT**, eleven tasks cleared: 440 · 441 · 443 · 444 · 449 · 451 · 453 · 454 · 455 · 456 · 457. ✅ **All four §15 departures ratified, each on a mechanism QA RE-DERIVED ITSELF.**) ← was: backlog
- blocked-by: TASK-440 + TASK-441 + TASK-443 + TASK-444 **+ TASK-449 + TASK-451 + TASK-453 + TASK-454 + TASK-455 + TASK-456** (all `ready-for-qa`) — ⚠️ **SCOPE WIDENED 2026-08-03, THREE TIMES.** ⛔ **THIS GATE RUNS LAST, AFTER EVERY C++ TASK IN THE BATCH IS FINISHED — and that is deliberate, not drift: 455 and 456 edit `SiegeAssistantComponent`, 454 edited `SiegePlayerController`, and reviewing files that are about to move produces a report whose citations are stale on arrival** (WARN-4's *7 of 7 stale anchors* is what that costs).
- parallel-safe: no (single report)
- ⚠️ **TWO ADDITIONS TO THIS GATE, 2026-08-03:**
    **(f) TASK-449 — THE OPEN-KEY SEAM.** Verify the handler calls **`SetAssistantConsoleOpen(true)` BEFORE creating or showing anything** and
    shows **nothing at all** on refusal · that `OnConsoleOpenChanged` is bound so a close by **any** route clears the posture (⚠️ **a posture
    flag stuck true is a cursor soft-lock, and `ApplyCursorInputState` is where this project's input bugs live**) · that the input asset is
    resolved through the shipped **`ResolveInputAction`** idiom and is **inert-null-safe** when absent · and ⛔ **that no existing `BindAction`
    was re-routed** (§2: a key may never pass through the assistant).
    **(g-0) ⛔ THE SENTENCE THAT MUST NOT APPEAR IN YOUR REPORT: "THE TESTS PASS."** ⚠️ **NO TEST IN THESE FOUR FILES HAS EVER EXECUTED — nothing
    has been built this batch.** ⇒ **Every claim about test behaviour is a claim about SOURCE, and must be labelled as one.** Discharge is
    **TASK-447's compile PLUS a green run**, claimed against that run and no other. ⚖️ **This is §9c exactly: where an artifact has a parser, the
    parser is the reviewer of record — and here the parser is the test runner.** ⛔ **A converted assertion is not a passing assertion.**
    **(g-1) ⚠️ RESIDUAL RISK, CONTAINED BUT UNPROVABLE WITHOUT RUNNING — carry it forward rather than closing it.** ⛔ **COUNT CORRECTED 2026-08-03: it is 18 SITES, NOT 12** — the source report's headline figure disagreed with its own enumeration and QA confirmed a sample are genuine `FName::ToString()` comparisons. **FIFTH wrong count this wave (§22 — a count is a hint, never a contract).** ⛔ **TASK-447 must not quote the 12.** ~~the **12 converted
    `FName::ToString()` sites depend on `WITH_CASE_PRESERVING_NAME`** (= `WITH_EDITORONLY_DATA` = 1 under these `EditorContext` tests, **0 in a
    packaged build**). ⇒ **Those assertions test the EDITOR, not the shipped game.** ✅ Correct for their purpose; ⛔ **a green editor run must not
    be reported as a shipped-build guarantee** (CONVENTIONS §13's `FName` caveat).
    **(g-2) VERIFY WARN-436-2's CONTAINMENT** — the ratified overrun must be confined to `FSiegeSettingsSaveLoadRoundTripTest`'s body
    (+1 statement, +1 pre-condition, 1 moved statement). ⛔ **Confirm it against the diff, not against the handoff.**
    **(g) TASK-451 — THE COMPARATOR SWEEP.** ⛔ **Do not accept a raw conversion count as the deliverable** (and note **113 is the true
    denominator, not my 117** — four were prose mentions in a comment block; ✅ **58 of 113 converted, 50 integer sites deliberately untouched,
    ZERO lines added or removed so the live TASK-441/423/436 reviews still resolve by line number**). ✅ **`SiegeAssistantZoneATest.cpp` needed
    ZERO conversions — all five of its string claims were ALREADY `*Sensitive`. The scope implied work there and there was none: TASK-423 had
    got it right unprompted, and that is recorded as a credit, not a gap.** Verify the **triage table** (site →
    claim shape → converted/left → reason), that **integers were NOT churned**, that the engine-header `file:line` for the case-insensitivity
    mechanism is **cited and not taken from the spec**, and — the one that matters — **that at least one conversion was PROVEN to bite**
    (pre-fix passes on an input it should reject, post-fix fails on it). ⚠️ **A sweep that changes 117 lines and demonstrates nothing is the same
    defect one level up.** ⚖️ **And apply the new law to everything else you read: for each `FString` assertion, name the claim shape and confirm
    the comparator can detect it — `TestEqual` cannot see casing or bytes** (CONVENTIONS "Settings screen…" §13). **A vacuous test passes hardest.**
- spec: >
    **Pre-compile review, from the LANDED FILES. ⚠️ THE RELAYED-DIAGNOSIS LAW IS BINDING: a diagnosis arriving via a spec, a handoff or an
    orchestrator relay is a LEAD, not a finding — re-derive it from the source.** A report that repeats a relayed root cause has forwarded, not
    reviewed. ⚠️ **A wrong root cause is worse than none: it ships a wrong FIX and a wrong LESSON, and it burns the loop that would have caught it.**
    **THE FIVE THINGS THIS GATE EXISTS FOR — everything else is secondary:**
    ⛔ **READ THIS FIRST: TASK-439's PASS DOES *NOT* COVER THE TOGGLE'S OFF-PATH, AND QA SAID SO EXPLICITLY.** It verified only the **scope
    fact** that neither TASK-436 nor TASK-437 references the assistant, the guard, the parser or the executor. ⚠️ **That is NOT the sentence
    *"every check still runs with the toggle off"*** — those are different claims and only one of them has been checked. ⇒ **THE OFF-PATH IS
    THIS GATE'S DEBT, owed against TASK-443's code, and it is criterion (a) below.** ⚖️ **A scope fact from one lane must never be read as a
    behavioural guarantee about another** — that substitution is how a green gate ends up covering nothing.
    **(i) ⚖️ RATIFY TWO PROGRAMMER RULINGS AGAINST MY OWN SPEC TEXT — BOTH LOOK LIKE DEVIATIONS AND BOTH ARE CORRECT.**
    **(i-1) `ToggleConsole()` IS DELIBERATELY NOT CALLED** despite sitting in TASK-449's `names:` list: it shows the widget **before** the
    controller can gate it, i.e. **exactly the ordering the ⛔ guard-first clause forbids.** ⇒ **The ⛔ clause wins; I have removed it from the
    `names:` list.** Confirm the shipped path is `SetAssistantConsoleOpen(true)` → **on refusal show NOTHING** → only then create/open.
    **(i-2) TASK-437 and TASK-444 BOTH BUILD THE TREE *BEFORE* `Super::RebuildWidget()`**, against CONVENTIONS' then-wording. ✅ **Both are right
    and the LAW was wrong** (`UserWidget.cpp:1214`); it is corrected. ⛔ **Do not file either as a deviation.**
    **(i-3) `AttachConsoleWidget` IS CALLED *BEFORE* `OpenConsole()`, AGAINST MY SPEC'S *"immediately after a successful create/open"* — AND
    THE DEPARTURE IS CORRECT.** TASK-453 settled it by **reading the widget source**: `OpenConsole()` **ENDS** with
    `OnConsoleOpenChanged.Broadcast(true)`, and the component subscribes to that delegate **INSIDE `AttachConsoleWidget`.** ⇒ ⛔ **Attaching
    afterwards would miss the ONLY "opened" broadcast of the first press — the FSM would sit in `Idle` while the player types into a box it does
    not know is open, recovering only on the SECOND open, with no error, no log and no crash.** ⚠️ **That is the `BeginPlay` trap one level
    deeper, and it would have reached Jonathan as *"the first time I press Enter, nothing happens."*** **Ratify it; do not file it as a deviation.**
    ⚖️ **The standing principle behind all three: a `names:` list or a condition clause that contradicts its own spec's reasoning LOSES** — and an
    agent that refuses a spec line on a stated, **checkable** mechanism is doing the job, not exceeding it (now law: CONVENTIONS "Settings
    screen…" §15). ⛔ **The common property that makes these ratifiable rather than licence is that EACH CITES A MECHANISM QA CAN CHECK WITHOUT
    TRUSTING THE REFUSER** — an engine line, a broadcast at the end of a named function, a clause in the same spec. **A departure justified by
    preference or *"this seemed better"* is NOT this and is still a deviation.**
    ⚠️ **AND DO NOT OPEN A QA LOOP ON TASK-453 FOR AN UNRESOLVED `AttachConsoleWidget`** — it was declared but not yet defined while TASK-443 ran.
    **That is a schedule fact, not a defect**; the definition is 443's (see TASK-447 step (0-PRE)).
    **(j) ⛔ TASK-453 — THE WIDGET→COMPONENT JOIN. Verify `AttachConsoleWidget` is called AT OPEN TIME, once per widget, and that the component's
    implementation is IDEMPOTENT and NULL-SAFE.** ⚠️ **A `BeginPlay`-time binder would bind NOTHING and no-op FOREVER** (widget creation is lazy),
    with no error and no log — **if you find one, it is a BLOCKER, not a nit.** Confirm all four outbound delegates (`OnConsoleSubmitted` /
    `Confirmed` / `Cancelled` / `OpenChanged`) actually reach the FSM, and that **TASK-453 touched only the controller and TASK-443 only the
    component** — ⛔ **a second writer in either file is a single-owner violation.**
    **(h) ⚠️ THE CODE-AUTHORED-TREE ORDERING — CHECK IT IN `USiegeAssistantConsoleWidget`, AND KNOW THE ANSWER IS ALREADY RIGHT.** ✅ **TASK-444
    ships `ConstructConsoleTree(); return Super::RebuildWidget();` — the CORRECT order** (`WidgetTree->RootWidget` is set at `:163`, before
    Super). ⛔ **CONVENTIONS' condition (b) said the OPPOSITE until 2026-08-03 and has been corrected; the defect was in the LAW, never in the
    code.** Confirm the shipped order and that child wiring lives in **`NativeConstruct`** (`:402`), never `NativeOnInitialized`
    (CONVENTIONS "Settings screen…" §3(b) + §11). ⚠️ **`UserWidget.cpp:1214` returns an `SSpacer` when `RootWidget` is null — a widget built in
    the wrong order renders EMPTY while passing every property readback.**
    **(i-5) ⚖️ RATIFY TASK-455's THREE REFUSALS — ALL THREE ARE §15 INSTANCES AND ALL THREE ARE CORRECT.**
    **(i-5a) ⛔ IT REFUSED TO RE-POINT THE RETIRED CONSTANT, AND RE-POINTING WOULD HAVE BEEN A REAL DEFECT.** `MaxSnapshotChars`' **only real code
    reader was `BuildZoneC`'s ROSTER TRIMMER** — neither the authority nor the pre-filter. **Both plugin bounds REJECT**, so aiming the trimmer at
    the pre-filter's `3000` would have converted ***"gracefully narrow the roster"*** into ***"REFUSE the player's order"*** — **a graceful
    degradation silently promoted to a hard failure, on a mid-battle interaction.** ✅ **It introduced `SnapshotTrimBudgetChars = 1085` — a NEW
    NAME with the value deliberately unmoved (§12c freezes Zone B/C bytes this wave).** ⚖️ **Now law (§19): the family has THREE roles — authority ·
    pre-filter · TRIMMER — and the safe direction AND the failure mode differ in each. A constant's safe direction is a property of its ROLE, never
    of its VALUE.** ⛔ **Verify the trimmer does NOT read either plugin constant.**
    **(i-5b) ✅ IT SHIPPED THREE `static_assert`s INSTEAD OF GAME-LANE COPIES — STRICTLY STRONGER THAN THE SPEC ASKED.** `MaxSnapshotTokens` and
    `SnapshotPreFilterMaxChars` **already existed** (`SiegeLlamaSubsystem.h:149`/`:170`); **duplicating them game-side would have created constants
    nothing in the game lane enforces — §17's exact failure, a guardrail that reports safe.** ⇒ **Confirm the asserts exist, that they bind the
    real symbols, and that NO game-lane duplicate was added.** ⚖️ **A `static_assert` cannot report safe falsely, cannot drift, and costs nothing at
    runtime — this is the enforcement pattern to prefer wherever one is available.**
    **(i-5c) ⚠️ `ZoneBCharReserve` STAYS 192, AND THE REASONING IS WHY IT RATIFIES.** My dispatch was ***true about the INSTRUMENT and false about
    the READING***: `ReportFirstCapture` **is** the right instrument, but **it has never executed**, and the 68/71 figures on record were printed by
    the **spike's `AppendZoneB` — a DIFFERENT LANE** (§9c/§12g's same-authorship-different-lane trap, which is what my dispatch walked into).
    ✅ **It invoked the spec's own fallback and made the reading TAKEABLE rather than guessing a value.** ⛔ **A derived number substituted here would
    have been the precise defect §12g exists to prevent.** **The reading is now pinned on TASK-447.**
    **(r) ⛔ A COUPLING YOU MUST KNOW BEFORE YOU RULE: TASK-455's CALLER AND TASK-450's `SetStaticPrefix` STAND OR FALL TOGETHER.** ⚠️ **If you rule
    against `SetStaticPrefix`, §8's MANDATORY budget assertion is left with NO IMPLEMENTABLE FORM AT ALL** — Zone A is built in the game lane, the
    plugin may not include a game-lane header, so there is no other moment the assembled string can be tokenized. ⛔ **That is NOT a reason to
    ratify it** — rule on the merits. **But a ruling against it is a §8 REWRITE routed to me, not a code fix routed to a programmer**, and it must
    be reported that way rather than as a defect finding.
    **(s) ✅ RATIFY THE DECLARED RACE — IT IS §15's DISCIPLINE TURNED ON ITS OWN CLAIM, WHICH IS RARER AND BETTER.** 455's *"authority live on
    turn 1"* is **a RACE, not a guarantee, and it wrote that into the CODE as a race** rather than asserting an ordering it could not lock.
    **Worst case the budget goes live one turn later, and the context budget holds in EVERY interleaving.** ⚖️ **An agent that declines to
    over-claim about its OWN work is doing the thing this pipeline has spent the whole wave asking of reviewers.**
    **(t) 📌 THE COUNT IS SETTLED AT 15 BY THREE INDEPENDENT DERIVATIONS — and the residue is instructive.** 15 lines / 16 occurrences, of which
    **only 5 are real code**; 3 prose in log strings, 7 prose in comments. ⚠️ **TASK-450's larger total had folded in `h:274 ZoneBCharReserve` — a
    DIFFERENT SYMBOL.** ⇒ **§14 gained a fourth variant this wave: a search that matched a different symbol entirely.** ✅ **0 `MaxSnapshotChars`
    code references remain — verify that yourself.**
    **(q) ⚠️ ONE PLUGIN-LANE APPENDIX — TASK-457, AND IT IS FOLDED HERE DELIBERATELY.** It closes three `qa/TASK-424.md` warns in
    `SiegeLlamaSubsystem.cpp`, a file that already passed 424. ⚖️ **A post-QA fix pass owes a re-look (the TASK-417 precedent), and folding one
    file with three named checks into this gate is cheaper than standing up a fifth QA task for it** — but it is a **cross-lane appendix, so scope
    it explicitly and do not let it dilute the game-lane review.** Check: ⛔ **the deadline is now a real between-slice term so the hard timeout
    backstops EVERY tier** (the header at `:130-135` claimed a guarantee the code did not provide — §17's shape, in the place a reviewer goes to
    check) · **`HardTimeoutSeconds` = 10.0 is UNCHANGED** · `MarkBusy`/`ClearBusy` are symmetric **or the asymmetry is documented in code** ·
    the VRAM citation now points at `TASK-413:625` (*"+2.0 to +4.6 ms on every tier"*) rather than the full-tier `:638`. ⛔ **And confirm the
    scope fence held: no queue, no threading change, no game-lane edit, and 🔒 `SiegeLlamaSpike.cpp` UNTOUCHED (§16).**
    **(0-STALE) ⛔ EVERY LINE NUMBER IN THIS GATE'S CRITERIA IS AS-OF-AUTHORING AND SOME HAVE ALREADY MOVED. VERIFY BY SYMBOL, NEVER BY OFFSET
    (CONVENTIONS §18c).** Measured: **TASK-453's `AttachConsoleWidget` call moved `4324 → 4348`** when TASK-454 inserted ~72 lines above it, and
    **nothing about it changed.** TASK-455 and TASK-456 then edit the component again. ⇒ ⛔ **A criterion that fails ONLY because a number moved
    has found NOTHING — and it manufactures a finding, which is the expensive direction.** ✅ **Assert RELATIVE ORDER, which is durable and is what
    these checks actually mean:** *"`AttachConsoleWidget` precedes `OpenConsole()`"* survives every insertion; *"`:4324` precedes `:4357`"* does not.
    ✅ **Already re-verified by symbol after 454 and holding:** attach still precedes open · 449's guard-first `SetAssistantConsoleOpen(true)`
    intact · **exactly ONE `bAssistantConsoleOpen` writer.**
    **(i-4) ⚖️ RATIFY TASK-454's REFUSAL OF THE SPEC'S PREFERRED SHAPE — THE FOURTH §15 INSTANCE, AND THE BEST-EVIDENCED ONE.** My spec preferred a
    shape needing no component edit; **the only such shape is folding the release into `SetUnitCommand` itself.** It refused on three CITED
    mechanisms: `SetUnitCommand` is documented as a **pure latch** · `ClearAllUnitGroups`' own doc (*"Called by T/E (before SetUnitCommand)"*)
    would become **self-referential** · and the P2 RPC `ServerSetUnitCommand` is **already named against the pure-latch contract**
    (`SiegePlayerController.cpp:981`). ✅ **RATIFY. Do not file as a deviation.**
    ✅ **AND RATIFY *HOW* IT CLEARED THE SAFETY QUESTION FIRST, BECAUSE IT IS THE §14 DISCIPLINE WORKING AS DESIGNED: IT USED A POSITIVE CONTROL.**
    Before trusting a zero-hit search for Blueprint callers of `SetUnitCommand`, it confirmed the search *could* find such a hit — `OnUnitCommandChanged`
    **does** appear in `Content/UI/WBP_HUD.uasset`. ⇒ ⚖️ **That converts a negative result from a §14 blind spot into EVIDENCE.** ⛔ **This is the
    pattern every future "nothing references X" claim must follow.**
    ✅ **ALSO RATIFY THE `HasAuthority()` GATE IT DECLINED TO ADD.** A client's T today runs the release locally and is refused inside
    `SetUnitCommand`; adding a gate would have **silently changed SHIPPED KEY BEHAVIOUR under cover of a consistency fix.** ⚖️ **Recognising that a
    "harmless hardening" was actually a behaviour change on the shipped path is exactly the restraint §2 asks for.**
    **(o) ⛔ TASK-456 — VERIFY THE CALL SIDE ACTUALLY LANDED.** 454 built `ApplyArmyWideStance`; **456 is what makes the assistant USE it.**
    Confirm **no `SetUnitCommand` call remains on the charge/fallback paths**, that **both key handlers and the assistant reach ONE
    implementation**, and that the **two now-false divergence-warning comments are DELETED, not softened** (a stale warning makes the next reader
    either "restore" the divergence or waste a pass disproving it).
    **(p) ⛔ TASK-454's DIFF: CHECK THE ITEMISATION, NOT THE NUMBER (CONVENTIONS §18a).** `757/94 → 836/108` is **CORRECT for an EXTRACTION**:
    the header is 100 % additive (deletions unchanged at 10) and **all +14 `.cpp` deletions are MOVES, itemised line-by-line in the handoff.**
    ⚠️ **An unchanged-deletion-count demand is UNSATISFIABLE alongside the pinned one-implementation contract** — two handlers cannot both keep
    intact copies *and* there be exactly one implementation. ⛔ **A reviewer checking the NUMBER fails this wrongly; a reviewer checking the
    ITEMISATION passes it correctly.**
    **(k) ✅ THE OFF-PATH TRACE IS DISCHARGED STRUCTURALLY — VERIFY THE STRUCTURE, NOT THE CONCLUSION.** TASK-443 reports the toggle is read in
    **one function** (`IsConfirmBeforeExecuteEnabled`, `:1987`) from **one call site** (`RouteParsedCommandInternal:1200`, its **last** step),
    while `ValidateCommandAgainstSnapshot` runs at **`:1134`, 66 lines earlier**, and all 20 machine checks sit strictly **before** the branch or
    **downstream of both arms**. ⇒ **Confirm those line facts yourself**; if they hold, criterion (a) is met **structurally**, which is the strong
    form — *the only difference between ON and OFF is whether a human looks.* ✅ **It fails safe (`Settings ? … : true`) and the deferred-fire flag
    ORs in, so it can only ever force review ON, never off.** ⚖️ **Note that it reported this as a FINDING rather than as an absence of findings —
    that is the distinction this gate was told to police, and it landed on the right side of it.**
    **(l) ⛔ RULE ON EACH OF THE THREE DECLARED EDITS TO TASK-442's FINISHED FILE INDIVIDUALLY — DO NOT WAVE THEM THROUGH AS A GROUP.** All three
    are declared and reported mechanically reversible; the load-bearing one makes `ExecuteAndReport()` **one body with two callers** via a
    **statement-for-statement-identical** change to `ConfirmPressed`. ⚖️ **That is what keeps the off-path guarantee NON-DRIFTING rather than
    merely true today — a shared body cannot diverge, two copies can.** ✅ **Sequential re-ownership of a finished task's file is legitimate; an
    UNDECLARED edit would not be.** **Rule each on its own merits and say so per edit.**
    **(m) 📌 DEV-11's DEFERRED-TRIGGER BRANCH IS DECLARED, NOT A DEFECT — AND IT IS THE WORST POSSIBLE ROW TO TUNE AGAINST.** An
    already-satisfied deferred trigger **re-asks** rather than firing, because *"2 **more** knights"* is indistinguishable from an absolute count
    in the emitted JSON. **One branch to reverse if you rule the other way.** ⛔ **But `DEV-11` IS PRECISELY THE ROW THAT FLIPS BETWEEN EVAL RUNS
    (§12h) — the batch's least stable evidence.** ⇒ **Rule on the BRANCH's logic from the code, never from an eval observation, and ⛔ do not
    cite a single run in either direction.** ⚖️ **Re-asking is also the fail-safe direction** — it costs a keypress; firing blind on a
    misread count costs an army.
    **(n) ⚠️ TASK-443's HANDOFF SAYS TASK-450 IS `backlog`. IT IS NOT — IT IS RUNNING. It read a stale board.** No action owed and **not a
    finding against 443** — but the `#include "SiegeLlamaSubsystem.h"` it added **will not resolve until 450 lands**, which the
    **447 ⊃ 424 ⊃ 450** ordering already covers. ⛔ **Do not open a loop on it** (same class as the `AttachConsoleWidget` link note).
    ⚖️ **Recorded because it is §14's cousin: a board read at time T is a snapshot of a moving target, exactly like a search result.**
    **(a) ⛔ THE TOGGLE'S OFF-PATH SKIPS ONLY THE HUMAN REVIEW.** Trace ON and OFF **side by side, check for check**, and confirm the parse, the
    `Kinds.Num() == Counts.Num()` invariant, the shortfall path, eligibility, the authority refusal and the non-orderable guard **all still run
    with confirm OFF.** ⚠️ **Any check that runs only on the ON path is a BLOCKER** — that is a UX preference silently converted into a safety
    regression. Confirm too that **an unresolvable settings subsystem behaves as ON** (fail safe = MORE review).
    **(b) ⛔ NO MULTI-TURN LOOP ANYWHERE.** Any code path that feeds a model's own previous output back into the model is an **automatic FAIL**,
    whatever it is called. Each clarification turn must be a **fresh single-turn call** with context carried as one game-authored Zone-C line.
    **(c) `CreateUnitGroup` IS A PROVABLY ZERO-BEHAVIOR-CHANGE EXTRACTION** (the TASK-397 read-through idiom) — and ⚠️ **`SiegePlayerController`
    also contains the FOLLOW lane's uncommitted work: attribute every diagnostic to the file AND the lane, and do NOT count foreign findings
    against this batch's loop budget.**
    **(d) THE GUARD REJECTS A MIXED SELECTION AS A WHOLE, never silently dropping a kind**, and its tests actually exercise the `0 orderable`
    case rather than only the absent-kind case. ⚠️ **Check that `IsKindOrderable` exposes the `orderable` tally and NOT the `followable` one** —
    the header itself distinguishes them and exposing the wrong one is a silent wrong answer.
    **(e) NO PLAYER-FACING STRING IS PRODUCED BY THE MODEL** (§3) — every one is a game-authored template filled from a reason code.
    **Also verify:** the §8/§9 pinned signatures character-for-character · `Build.cs` adds **`SiegeLlama` only** (⛔ not `HTTP`, not `Sockets`) ·
    the input probe is **deleted** · `BindWidgetOptional` + null-construct escape hatches present in both widgets · BIE params
    FString/int32/bool/uint8 only · **the M8 declaration VERBATIM in all four handoffs** · no unit registry / actor cache / dirty flag (§4).
    ⚠️ **`Grep` MANGLES COMMENT SYNTAX ON THIS MACHINE — comment-level and structural findings come from RAW `Read` output only. `Grep`
    LOCATES; it does not READ.** It has manufactured two false blockers and can equally MASK the `*/`-in-comment trap.
    ⚠️ **STATE WHAT YOU DID NOT RUN — nothing is compiled and nothing has executed. "Reviewed by reading" is legitimate and INCOMPLETE; name
    the parser you did not run** (§9c: **where an artifact has a parser, THE PARSER IS THE REVIEWER OF RECORD**, and *"a method that compares an
    artifact to another artifact of the same authorship cannot detect an error they share"*).
    Report `qa/TASK-446.md`. Post the verdict in ⚙️ Dev & QA.
- names: > Law: CONVENTIONS "Settings screen…" §5, §6, §8, §9 · "In-match LLM command assistant" §1, §2, §3, §4, §9, §9c · the RELAYED-DIAGNOSIS LAW · §10's `Grep` trap.

#### TASK-447 — [W1-INT] QUIESCE + COMPILE + the snapshot FIRST-EXECUTION AUDIT + host TASK-438 + integration commit (build-master)
- assignee: build-master
- status: ✅ **done — COMMITTED `cd5f4ed` (2026-08-03): `TASK-447: Wave 1 code — settings screen + in-match LLM command assistant (compile-gated)`, plus `56acf10` (pipeline record) and `cf8ef8e` (closing record) — ⛔ THREE commits carry this id.** 📋 **MANAGER FLIP 2026-09-07, ⛔ 35 DAYS LATE, ⛔ ON GIT EVIDENCE** — `handoffs/BOARD-STALENESS-audit.md` §2/§5.1/§5.2 (build-master's git-resolved sweep; ⛔ the manager holds ⛔ NO `Bash`, `SC-§71b`). ⛔ **LOGGED EXCEPTION TO WRITE-DISCIPLINE RULE 1** — `status:` is the ⛔ assignee's field; the ⛔ commit host never flipped it, so this row asserted a falsehood for ⛔ 35 days. 🚨🚨 **⛔ THIS IS THE ⛔ SINGLE MOST EXPENSIVE STALE ROW ON THE BOARD: ⛔ `backlog` for ⛔ 35 DAYS with ⛔ THREE commits carrying its name and ⛔ FOUR OPEN ROWS FENCED BEHIND IT (⛔ TASK-438, TASK-448, TASK-458, TASK-460).** ⛔ **And it is the ⛔ OTHER HALF OF THE `438`⇄`447` DEADLOCK — ⛔ each row fenced behind the other, ⛔ both shipped the same day, ⛔ neither knowable from the board.** ⛔⛔ **A FLIP IS ⛔ NOT A GO** — ⛔ do ⛔ NOT re-run this integration; `main` has moved ⛔ 35 days past it. Marker `BOARD-STALE-FLIP-2026-09-07`. ← was: ~~backlog~~
- blocked-by: ✅ **DISCHARGED — ⛔ ALL THREE SPENT; the row itself COMMITTED `cd5f4ed`.** ⛔ **`TASK-438` (hosted in-session) also COMMITTED, `87c4784`, ⛔ same day — the mutual fence is dead in both directions (`handoffs/BOARD-STALENESS-audit.md` §5.2).** ← was: ~~**TASK-439 PASS + TASK-446 PASS + TASK-424 PASS**~~ (⚠️ **and TASK-424 now covers TASK-450, so the subsystem must LAND first**) · hosts **TASK-438** in-session
- parallel-safe: no — ⛔ **this IS the compile gate**
- ⚠️ **THREE CORRECTIONS TO THE SPEC BELOW, 2026-08-03 — READ THESE FIRST:**
    **(i) ⛔ THE SMOKE TEST INHERITED FROM TASK-425 TESTS A SUBSYSTEM THAT DID NOT EXIST WHEN I WROTE IT.** TASK-423 delivered **only** the Zone-A
    equality test and **correctly declared the shortfall**; `USiegeLlamaSubsystem` is written by **TASK-450**. ⇒ **Every clause of step (5) —
    `Siege.Llama.Info`, async load never blocking match start, one parseable JSON, the second concurrent call returning false, `CancelActiveRequest()`
    aborting mid-prefill, the absent-model-file path, the partial-tier hitch re-measure — REQUIRES TASK-450 TO HAVE LANDED.** ⛔ **Do not run this
    gate before it does, and do not quietly drop the smoke test to make the gate runnable.** ⚖️ **My error, not the programmer's: I folded TASK-425's
    spec into this task while its subject was still unwritten.**
    **(ii) ⛔ THE ATTRIBUTION SPLIT IN STEP (5) IS RETIRED — ruling 7 was WRONG.** `SiegePlayerController.{h,cpp}` was **CLEAN**; Jonathan had
    already committed the FOLLOW lane himself (`e70f5ba`). **The lanes are separated BY COMMIT, which is stronger than a split argued in a message.**
    The whole working diff in those files is TASK-440 + TASK-449. ⚠️ **Still run the `git status --porcelain` pre-flight and branch on the ANSWER —
    what was wrong was PREDICTING the answer.** 📌 **TASK-440's handoff states `425 insertions`; git measured `430 / 94`. Trust the command, not the
    handoff** — a number that merely *sounds* measured is the defect this batch keeps finding (the 58-vs-59 LFS precedent).
    **(iii) ⚠️ WARN-5 IS INSTRUMENTED, NOT DISCHARGED — AND MUST NOT BE REPORTED AS DISCHARGED.** TASK-423's Zone-A equality test **has never
    run**. ⇒ **Discharge requires BOTH your compile landing AND `Siegebound.Assistant.ZoneA.*` going GREEN, claimed against that run and no
    other.** ⚠️ **The spike lane inside that test is a frozen 98-line verbatim copy (text-diffed clean, 5116 chars) and its offline agreement is
    SAME-AUTHORSHIP — it discharges nothing** (§9c: *a method that compares an artifact to another artifact of the same authorship cannot detect
    an error they share*). ⛔ **"The test exists" is not "the test passed."**
- spec: >
    **(0-SPIKE) 🔒⛔ THE INHERITED *"DELETE THE SPIKE HARNESS IN THE SAME COMMIT"* CLAUSE IS STRUCK. DO NOT DELETE `SiegeLlamaSpike.cpp`.**
    It arrived here by absorbing TASK-425's spec and is **SUPERSEDED BY CONVENTIONS §16** — the file is **reclassified from throwaway to
    LOAD-BEARING TEST INFRASTRUCTURE** and its deletion is **UNSCHEDULED, not deferred.** ⛔ **Deleting it would spend Jonathan's one honest
    measurement by destroying the only instrument that can take it:** `assistant_eval_holdout2.csv` is authored against the **`t0` fixture inside
    that file** and scored by **`Siege.Llama.SpikeEval`, also inside it** — and it is **also** the only `zoneA_tok` printer and the **only**
    partial-tier sampler. ⚠️ **TASK-424's own item (10) carries the same struck clause; it is superseded identically.**
    **(0-DEADLINE) ⛔ REWRITTEN 2026-08-03 — MY EARLIER CRITERION ASKED YOU TO MEASURE SOMETHING NO SINGLE TIER CAN SHOW. THE OLD WORDING IS
    STRUCK.** ~~The ONLY tier that can exercise the deadline is `tier=cpu`; exercise it there or record NOT EXERCISED.~~
    ⚠️ **THE TWO FACTS COLLIDE, BOTH ARE TRUE, AND TOGETHER THEY DEFINE THE PROCEDURE:**
    · ⛔ **On `tier=cpu` — where the ceiling IS reachable (TASK-413: 2 of 5)** — the deadline exits via `abort_callback` **MID-GRAPH**,
    `llama_decode` returns **2**, and the code takes **a DIFFERENT BRANCH WITH NO ELAPSED FIELD.** ⇒ ⛔ **THE FIXED LINE DOES NOT PRINT ON CPU.**
    · ⚠️ **On the GPU tiers — where the fixed line DOES print** — the ceiling is only **marginally** reachable (worst prefill **417–2914 ms**;
    partial's worst **decode** wall **8926 ms of 10000**, i.e. within ~1.1 s).
    ⇒ ⇒ ⛔ **THEREFORE: ON THE TIER WHERE THE CEILING IS REACHABLE THE LOG DOES NOT PRINT, AND ON THE TIER WHERE THE LOG PRINTS THE CEILING IS
    BARELY REACHABLE.**
    **WHAT TO DO, IN ORDER:**
    **(a)** ⛔ **NEVER read the ABSENCE of a `HARD TIMEOUT` line as *"no timeout occurred."*** On CPU a real timeout is **silent in that field** —
    detect it by its own signature (`llama_decode` returning **2** / the abort path), **not by the log line.**
    **(b)** **If a GPU-tier run does fire the deadline, the printed elapsed is now a REAL measurement (TASK-459) — record the number.**
    **(c)** ⛔ **If neither tier fires it, RECORD `NOT EXERCISED`. Do NOT synthesize a procedure at the gate, and do NOT report a pass.**
    ⚖️ **An honest *"not exercised"* is a RESULT; an invented exercise is how a gate starts producing the false greens this batch exists to catch.**
    **(d)** ⛔ **`SiegeLlamaSpike.cpp` CARRIES THE SAME UNFIXED SHAPE AND IS UNTOUCHABLE UNDER §16 — AND TASK-413's CPU NUMBERS CAME FROM IT.**
    ⇒ **You may still read a CONFIGURED number there. NEVER treat a spike-printed timeout figure as measured** (CONVENTIONS §23: a value that
    varies is not thereby correct; and §16 keeps the file alive precisely because it is the sealed corpus's instrument, not because it is right).
    **(0-ANCHORS) ⚠️ `qa/TASK-424.md`'s LINE NUMBERS ARE STALE — ~200 LINES WERE ADDED TO THAT FILE.** §18c's rot, now in the **plugin** lane.
    ⛔ **Verify by SYMBOL, never by offset**, in the plugin exactly as in the game lane.
    **(0-ZONEB) ⛔ AN OWED READING, PINNED HERE SO IT IS NOT LOST — TAKE IT AT THE FIRST EXECUTION.** `ZoneBCharReserve` **stays 192** and
    TASK-455 was right to leave it: it charges **192** for a Zone B measuring **68 (t0) / 71 (t1)**, worst case ~85 — **but every one of those
    figures was printed by the SPIKE's `AppendZoneB`, a DIFFERENT LANE from the shipped builder** (§9c/§12g). ⇒ **`ReportFirstCapture` is the
    right instrument and it has NEVER EXECUTED. Your snapshot first-execution audit (step 3) is its first run.**
    ⇒ **RECORD THE SHIPPED BUILDER'S PRINTED `zoneB_chars` AND HAND IT TO THE MANAGER.** ⛔ **Do NOT change the constant yourself** — you are
    taking the measurement, not spending it. ⚠️ **If the reading does not appear, say so plainly; a derived substitute here is the exact defect
    §12g exists to prevent.**
    **(0-BACKENDS) ⛔ A HARD REPORTING DUTY, NOT A PREFERENCE — QA ACCEPTED THE SYNCHRONOUS LOAD *WITH BINDING CONDITIONS*.**
    `EnsureBackendsLoaded()` is **synchronous on the game thread** (`bBackendsLoaded` is a plain bool, `SiegeLlamaModule.h:83`;
    `SiegeLlamaInfo.cpp:81` calls the same function). ⇒ **You MUST report the MEASURED `N ms`**, and ⛔ ***"async load never blocks match start"*
    IS A QUALIFIED-PASS ONLY — it may NOT be recorded as a plain pass.** ⚖️ **A latency claim with no number is the shape this batch has paid for
    repeatedly: it reads as a measurement and is an assurance.**
    **(0-SMOKE) ⛔ THE INHERITED SMOKE TEST IS CORRECTED — ONE CHECK IS IMPOSSIBLE, NOT MERELY HARD, AND IT MAY NOT BE SILENTLY REDUCED TO
    WHATEVER HAPPENS TO RUN.** TASK-450 stated exactly what its code can and cannot support; **honour the distinctions:**
    | check | verdict |
    |---|---|
    | `Siege.Llama.Info` (⚠️ **registered in `SiegeLlamaInfo.cpp`, NOT the subsystem — corrected 2026-08-03**) · one parseable JSON · **queue depth 1** (deterministic — run `Siege.Llama.Test` twice) · absent-model path | ✅ **SUPPORTED — run all four** |
    | *async load never blocks match start* | ⚠️ **QUALIFIED** — `EnsureBackendsLoaded()` is **synchronous on the game thread**; it is declared, timed and logged. **Report the timing, not the slogan.** |
    | *cancel mid-prefill* | ⚠️ **genuinely mid-graph ONLY on `tier=cpu`** — say which tier you exercised |
    | **partial-tier frame-time hitch re-measure** | ⛔ **IMPOSSIBLE FROM THE SUBSYSTEM — that sampler exists ONLY in `SiegeLlamaSpike.cpp`.** Run it **there**, or record **NOT MEASURED**. ⛔ **It may NOT be dropped silently.** |
    ⚠️ **THE MODEL IS NO LONGER RESIDENT** — the editor was restarted (deadlock during TASK-445, see below) and the restart unloaded it.
    **Any smoke test needing inference must load it first; budget for that rather than discovering it.**
    **(0-LFS) ⛔ THE LFS CHECK INVERTS BY FILE KIND, AND THE RULE THIS BOARD INHERITED IS THE *DOCS* ONE. THIS COMMIT CARRIES ASSETS.**
    `handoffs/TASK-434-buildmaster.md` §1b proved a **docs-only** commit clean with *"every object `(Git: …)`, not one `(LFS: …)`."*
    ⛔ **APPLIED TO AN ASSET COMMIT THAT TEST IS BACKWARDS.** `.uasset` routes through the **`lfs` filter** ⇒ **`(LFS: …)` IS CORRECT**, and
    ⛔ **a `.uasset` tagged `(Git: …)` IS THE DEFECT** — the binary went in raw, and **git history is append-only: a raw blob stays raw forever.**
    | this commit contains | expect | ⛔ defect |
    |---|---|---|
    | `.md` `.cpp` `.h` `.csv` `.cs` `.ini` | **`(Git: …)`** | any `(LFS: …)` |
    | **`.uasset`** (`IA_AssistantConsole`, `IMC_Hero`, `WBP_MainMenu`) | **`(LFS: …)`** | **any `(Git: …)`** |
    ✅ **Run `git check-attr filter -- <path>` PER KIND before committing and paste it** (§7 *acceptance, not assertion*). Law: CONVENTIONS §25.
    **(0-INDEX) ⛔ THE INDEX IS ALREADY DIRTY AND IT WAS NOT DIRTIED BY AN AGENT — TREAT IT AS HOSTILE.**
    ```
    A   Content/Input/Actions/IA_AssistantConsole.uasset   <- STAGED, not by any agent
     M  Content/Input/IMC_Hero.uasset                      <- modified, unstaged
    ```
    **GitHub Desktop is running and the editor's Git provider auto-stages saved assets.** ✅ **No commit was created — `HEAD` verified still
    `c8c780c`.** ✅ **TASK-445 deliberately did NOT unstage it, on the correct grounds that `git reset` is itself a Git WRITE and Git is YOUR
    exclusive tool** — that restraint is the single-owner law working, not an omission.
    ⇒ ⛔ **Re-read the index; never trust it. Stage by EXPLICIT PATHSPEC only — no `-a`, no `add -A`, no bare `git commit` — and re-check
    `git status --porcelain` AFTER every `add`. Verify each commit by INVERSE FILTER** (`git show --name-only` piped through a regex that
    *removes* the intended paths ⇒ empty output). **`handoffs/TASK-434-buildmaster.md` §1c is the procedure that already works — follow it.**
    **(0-PRE) ⛔ VERIFY THE CROSS-TASK DEFINITION EXISTS *BEFORE* YOU COMPILE — OR THE GATE FAILS ON THE WRONG LINE.**
    `USiegeAssistantComponent::AttachConsoleWidget` is the pinned seam between **TASK-443** (implements it) and **TASK-453** (calls it).
    ⚠️ **At the time TASK-453 landed it was DECLARED (`SiegeAssistantComponent.h:723`) but NOT YET DEFINED in the component `.cpp`, because 443
    was still running. That is the ANTICIPATED state, not a defect.** ⇒ **Before `Build.bat`, confirm the DEFINITION is present on disk.**
    ⛔ **If it is absent, the unresolved external is TASK-443's, NEVER TASK-453's — and it must NOT open a QA loop on 453.**
    ⚖️ **THE MISATTRIBUTION IS THE EXPENSIVE PART, NOT THE FAILURE:** a linker names the *calling* translation unit, so an un-implemented pin
    reads on the console exactly like a bad call. **DIAGNOSTIC ATTRIBUTION applies to LINK errors, not only to compile errors** — attribute to
    **the owner of the missing definition**, and route it there as early information.
    ⚠️ **`git grep` CANNOT SEE ANY OF THIS BATCH'S FILES — they are all untracked (`??`) until your commit. Check with `Read`, never with a
    negative search** (CONVENTIONS "Settings screen…" §14).
    **(0) ⛔ QUIESCE THE MODULE FIRST AND SAY SO IN THE HANDOFF.** No programmer task in `Source/GitClaudeUnrealTest/` may be in flight —
    **every C++ task finished with its handoff written, not merely non-conflicting.** ⚠️ **FILE-DISJOINTNESS IS NOT BUILD-DISJOINTNESS:** a
    half-written new file owned by an unrelated batch is the worst case, and nothing in any board flags it. **This is the exact defect that
    failed TASK-401 on three diagnostics from a lane it had nothing to do with.**
    **(1) COMPILE — the hard gate.** ⛔ **`Build.bat` RETURNS EXIT 0 ON A FAILED BUILD. PARSE THE LOG FOR `Result:`; NEVER TRUST
    `$LASTEXITCODE`** (it has been wrong in BOTH directions on this project — four failed builds returned 0, and one success returned 1).
    ⚠️ **If the failure is `0x800711C7` in ~2 s, that is enforced Smart App Control blocking UBT's unsigned `ModuleRules.dll` — a JONATHAN-ONLY
    fix (Windows Security → Smart App Control → Off), NOT a code error. Do not loop QA over it.**
    **DIAGNOSTIC ATTRIBUTION IS PART OF THE JOB:** a failure belongs to **the file the diagnostic names**, never to the lane that ran the build.
    ⚠️ **`SiegePlayerController.{h,cpp}` carries the FOLLOW lane's uncommitted work — foreign diagnostics route to THAT batch as early
    information and are NEVER counted against this batch's QA loop.**
    **(2) HOST TASK-438 IN-SESSION, AFTER THE COMPILE** (the TASK-355/357 precedent — the `CreateWidget` node cannot pick an uncompiled class).
    **(3) ⛔ THE SNAPSHOT FIRST-EXECUTION AUDIT — A REAL DELIVERABLE, NOT A NOTE (ruling 5).** `USiegeAssistantSnapshot` has had **ZERO callers
    and ZERO tests** and its truncation logging has **never emitted a line**; every *"now observable"* clause in §8/§10 describes code that has
    never run. **Wave 1 is its first execution.** In PIE on `L_Arena` **with units on the field** (⚠️ never an empty map), record:
    **the real captured snapshot pasted in FULL with its character count** · **live `zoneB_chars` / `zoneC_chars`** against the 68 / 887 fixture
    figures and the 738 shipped-builder figure · **whether the truncation log fired — and if not, that it was not supposed to** · **whether
    `MaxRosterKinds = 8` truncated on a real board.** ⛔ **An audit reporting "no problems" without naming which of these it OBSERVED has not
    been performed.** ⚠️ **"Observable" is not "observed" and "reviewed" is not "executed."**
    **(4) THE PLAYABLE CHECKS (machine-verifiable half only; the rest is TASK-448's):** the settings screen opens from the main menu and
    **the backdrop swallows clicks** · the toggle **persists across a full relaunch** (set it OFF, relaunch, confirm it is still OFF — this is
    the SaveGame's only real test) · a typed order reaches `AwaitConfirm` with ghost circles · **Cancel leaves nothing partially executed** ·
    a non-orderable kind is refused **before execution** · and ⛔ **every keyboard command still works identically with the console closed.**
    **(5) COMMIT — `main`, NO PUSH, PER-DELIVERABLE.** ⚠️ **`main` is 13 ahead of origin; verify the ahead-count with
    `git rev-list --count origin/main..main`, NEVER from a memory note or from this board** (placeholder hashes and remembered counts have both
    been wrong here). ⛔ **THE FOLLOW LANE'S WORK IS INTERMIXED IN `SiegePlayerController.{h,cpp}` AND CANNOT BE SPLIT BY PATHSPEC — SPLIT IT BY
    ATTRIBUTION IN THE COMMIT MESSAGE** (the TASK-434 §3 precedent), and ⛔ **do not commit the FOLLOW lane's other files: TASK-403 is still
    Jonathan's, gated on his TASK-402 playtest.**
    ⚠️ **The editor's Git provider auto-stages saved assets — stage by EXPLICIT PATHSPEC and re-check `git status --porcelain` AFTER staging.
    If anything foreign is dirty or staged, STOP and report rather than committing it.** ⛔ `git reset --hard` / `clean -fd` **BANNED**.
    **Real hashes, read back from `git log` — never predicted.** `git ls-files` must show **no `.gguf` and nothing under `/Models/`**; run
    **`git check-ignore -v`** and paste it (**acceptance, not assertion**). **`git lfs status` per-object tags checked** — the correct test is
    the per-object `(Git: …)` tag, **not** an empty "Objects to be committed" list.
    **⛔ `L_Arena` IS NEVER SAVED.** 🔒 **`Docs/Data/assistant_eval_holdout2.csv` IS NOT OPENED, NOT READ, NOT SCORED** — un-gating Wave 1 does
    not spend it.
    ⚠️ **THE EDITOR IS RUNNING WITH THE MODEL LOADED AND JONATHAN IS AT THE KEYBOARD. CLOSING IT IS HIS CHOICE** — if the compile needs a
    bounce, **ASK; do not force-kill.** The unattended-gate exception does not apply while he is present.
    Handoff `handoffs/TASK-447-buildmaster.md`. Post hashes in 🔧 Build & Git.
- names: >
    Build: `"C:/Program Files/Epic Games/UE_5.8/Engine/Build/BatchFiles/Build.bat" GitClaudeUnrealTestEditor Win64 Development -project="…/GitClaudeUnrealTest.uproject" -waitmutex`.
    Law: the hard gate (no code commit without a PASS QA report) · CONVENTIONS "⛔ THE QUIET-MODULE LAW" · "GIT HAZARD LAWS" (LFS, checksum-not-`git diff`,
    repo-relative `git show`) · "UE Build.bat exit code lies" · "Settings screen…" §7, §9, §10.

#### TASK-449 — [W1-WIRE] The open-key seam: bind `IA_AssistantConsole`, create/show the console, drive the posture (gameplay-programmer)
- assignee: gameplay-programmer
- status: **done — COMMITTED 2026-08-03 in `cd5f4ed`** (Wave 1 code. Build GREEN: both DLLs linked, **zero errors, zero warnings** — the link step ran for the first time, so the 22-seam pre-flight became a measurement. ⚠️ **Green means it BUILDS, not that it works**: no automation test has ever run (33 compile; compiling is not passing), WARN-5 stays instrumented-not-discharged, and every playable check belongs to TASK-448. Flip applied by the ORCHESTRATOR after manager handed the board back.) ← prior: ✅ **qa-passed → ready-for-integration** (2026-08-03 — `qa/TASK-446.md`, PASS · 0 blockers). 🔒 **THE BLOCKER-GRADE LAZY-CREATION TRAP IS CLEAR: no `BeginPlay` binder exists.** Verified **by relative order at the SYMBOL, not by offset** (§18c): `SetAssistantConsoleOpen(true)` → `GetOrCreateAssistantConsoleWidget()` → `AttachConsoleWidget` → `OpenConsole()`.
- status: **ready-for-qa** (2026-08-03 — handoff `handoffs/TASK-449-programmer.md`; reviewed under TASK-446 item (f). ⛔ **NOT compiled** — TASK-447 is the one gate. **Additive only: `git diff --stat` on `SiegePlayerController.{h,cpp}` went 425/94 → 715/94 — +290 insertions, deletion count UNCHANGED, so nothing of TASK-440's was reverted or reformatted.** ✅ **The board's correction is confirmed AT THE ARTIFACT: the `|| bAssistantConsoleOpen` posture term IS present at `:4118`** — TASK-444's contrary claim is wrong. Delivered: `AssistantConsoleActionAsset` (ctor, `/Game/Input/Actions/IA_AssistantConsole.IA_AssistantConsole`) · `ResolveInputAction` + **one guarded `BindAction`** appended last, ⛔ **zero shipped bindings re-routed** (§2) · `OnAssistantConsolePressed` with the **guard-first/UI-second** order — `SetAssistantConsoleOpen(true)` runs BEFORE any creation, and on refusal **nothing is constructed, added, focused or flashed** · lazy `CreateAndAddToViewport` on first successful open + `GetAssistantConsoleWidget()` · `OnConsoleOpenChanged` bound to a posture handler (+ `RemoveDynamic`/close/`RemoveFromParent` in `EndPlay`); `bAssistantConsoleOpen` still has exactly ONE writer. ⚠️ **`ToggleConsole()` DELIBERATELY NOT CALLED — it shows the widget itself and would produce the exact forbidden ordering; the ⛔ clause wins over the names list (handoff §4, QA should rule).** ⚠️ **A silent `OpenConsole()` refusal (disabled/faulted console) is detected via `IsConsoleOpen()` and the posture is rolled back.** 🚩 **RESIDUAL GAP, NOT MINE: the widget→component SUBMIT forwards are still unbound (TASK-443), and lazy creation means `GetAssistantConsoleWidget()` is NULL until the first open — a BeginPlay binder would no-op forever. Needs a manager ruling since TASK-443's spec forbids controller edits (handoff §6).** ⚠️ **The key stays INERT until TASK-445's asset + `IMC_Hero` mapping land — TASK-448 cannot test this without it.** No Git, no editor/MCP/PIE, no `Content/`.)
- blocked-by: **none — DISPATCHABLE NOW.** (Sequential re-ownership of `SiegePlayerController.{h,cpp}` after **TASK-440, which is FINISHED with its handoff written** — never concurrent with it.)
- parallel-safe: **yes vs TASK-443** (different files) — ⛔ **never beside a compile gate**; **EXCLUSIVE owner of `SiegePlayerController.{h,cpp}`**
- spec: >
    ⛔ **WHY THIS TASK EXISTS: A REAL WAVE GAP. NOTHING OPENS THE CONSOLE.** Verified repo-wide, not relayed — **zero call sites** for
    `CreateAndAddToViewport`, `ToggleConsole` or `SetAssistantConsoleOpen` outside their own definitions, and **zero `BindAction` for any
    assistant action.** As the board stood, **TASK-448 would have handed Jonathan a feature with nothing to type into**, and it would have
    surfaced at his playtest.
    ⚠️ **BOTH SIDES OF THE SEAM ARE ALREADY BUILT AND CORRECT — DO NOT REBUILD EITHER.** TASK-440 shipped the posture owner
    (`bAssistantConsoleOpen` in `ApplyCursorInputState` at `:4118`, `CanOpenAssistantConsole()` `:4139`, `SetAssistantConsoleOpen()` `:4158`,
    and all four mutual-exclusion mirrors at `:1107` / `:1989` / `:2474`). TASK-444 shipped the widget (`CreateAndAddToViewport`,
    `OpenConsole`, `CloseConsole`, `ToggleConsole`, `OnConsoleOpenChanged`). ⛔ **TASK-444's handoff claimed the posture term was missing —
    THAT CLAIM IS WRONG; it read TASK-440's SPEC, not TASK-440's delivered CODE.** ⚖️ **Recorded because it is the relayed-diagnosis law in its
    purest form: a report about another task's file is a LEAD, and the artifact settles it.** **You are writing the JOIN, nothing else.**
    **(1) FOLLOW THE SHIPPED INPUT IDIOM CHARACTER-FOR-CHARACTER — it is fully self-contained in C++ and needs NO editor step.** Mirror
    `CmdFollowAction` exactly: a `TSoftObjectPtr<UInputAction> AssistantConsoleActionAsset` set in the constructor by path
    (`/Game/Input/Actions/IA_AssistantConsole.IA_AssistantConsole`), an `UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")`
    `TObjectPtr<UInputAction> AssistantConsoleAction`, resolved via **`ResolveInputAction(...)`** in `SetupInputComponent`, then a guarded
    `if (AssistantConsoleAction) { BindAction(..., ETriggerEvent::Started, ...); }`.
    ✅ **INERT-NULL-SAFE IS THE DESIGNED STATE AND IT IS WHY YOU ARE NOT BLOCKED ON TASK-445:** a missing asset skips the binding, logs ONE
    line, and leaves the key inert — **never a crash, every other key untouched.** Same pattern as `IA_CmdAmbush` and `IA_CmdFollow` before
    their assets landed.
    **(2) THE HANDLER'S ORDER IS LOAD-BEARING — GUARD FIRST, UI SECOND.** Call **`SetAssistantConsoleOpen(true)` BEFORE showing anything**;
    it returns **false** when `CanOpenAssistantConsole()` refuses (placement / targeting / group-pick active). ⛔ **On a refusal, show
    NOTHING** — do not create the widget, do not flash it. **A console that appears and then cannot be used is worse than one that does not
    appear.** On close: `CloseConsole()` **and** `SetAssistantConsoleOpen(false)` so the cursor posture is released.
    **(3) OWN THE WIDGET INSTANCE, LAZILY.** Hold a `TObjectPtr<USiegeAssistantConsoleWidget>`; create it on first successful open via the
    widget's own `CreateAndAddToViewport`; expose a `GetAssistantConsoleWidget()` accessor. ⚖️ **The controller owns creation, visibility and
    posture; the COMPONENT (TASK-443) binds the widget's delegates and owns the FSM.** That split is deliberate: it keeps this task to ONE file
    and adds no new coupling inside the component.
    **(4) ⛔ BIND `OnConsoleOpenChanged` AND KEEP THE POSTURE HONEST.** If the widget closes by any route other than the key, the posture must
    still clear. ⚠️ **A posture flag that can be left stuck true is a soft-lock of the cursor** — and `ApplyCursorInputState` is exactly where
    this project's level-travel input bugs have lived before.
    **(5) ⛔ §2 IS THE LAW THIS TASK CAN MOST EASILY BREAK: every keyboard command must still work BYTE-IDENTICALLY.** You are ADDING a binding,
    never re-routing one. **No key may be made to pass through the assistant.**
    ⚠️ **A `UInputAction` reference is not the input MAPPING — TASK-445 still owns creating the asset and mapping it in `IMC_Hero`, and still
    FLAGS-never-stomps if `Enter` is taken.**
    ⛔ **No component edits, no widget edits, no `Content/`, no Git, no compile.**
    **M8 DECLARATION DUTY: state verbatim in the handoff — "adds no replicated property, no new replicated class, no new relevancy tier."**
    Handoff `handoffs/TASK-449-programmer.md`. Post in ⚙️ Dev & QA.
- names: >
    `SiegePlayerController.{h,cpp}` · `AssistantConsoleAction` / `AssistantConsoleActionAsset` / `ResolveInputAction` ·
    `OnAssistantConsolePressed` · `SetAssistantConsoleOpen` / `CanOpenAssistantConsole` / `IsAssistantConsoleOpen` ·
    `USiegeAssistantConsoleWidget::CreateAndAddToViewport` / `OpenConsole` / `CloseConsole` / `OnConsoleOpenChanged` ·
    `/Game/Input/Actions/IA_AssistantConsole.IA_AssistantConsole`.
    ⛔ **`ToggleConsole` REMOVED FROM THIS LIST 2026-08-03 — MANAGER RATIFICATION, AND THE PROGRAMMER WAS RIGHT.** It shows the widget **before**
    the controller can gate it, which produces **exactly the ordering the spec's ⛔ guard-first clause forbids** (*call `SetAssistantConsoleOpen`
    first; on refusal show NOTHING*). ⇒ **The ⛔ clause WINS and `ToggleConsole()` is deliberately NOT called.** ⚖️ **My `names:` list contradicted
    my own spec text, and the spec text is the instruction — an index that contradicts what it indexes loses** (the `qa/TASK-430.md` §7 RULING B
    precedent). **Corrected here so the next reader does not "fix" it back;** `ToggleConsole()` stays on the widget as unused public API.
    Law: CONVENTIONS "In-match LLM command assistant" §2, §5 · "Input-mode ownership (level-travel law)" · the RELAYED-DIAGNOSIS LAW.

#### TASK-459 — [LLM-WARN3] The new timeout log always prints `10.0s` — one argument (gameplay-programmer)
- assignee: gameplay-programmer
- status: **done — COMMITTED 2026-08-03 in `cd5f4ed`** (Wave 1 code. Build GREEN: both DLLs linked, **zero errors, zero warnings** — the link step ran for the first time, so the 22-seam pre-flight became a measurement. ⚠️ **Green means it BUILDS, not that it works**: no automation test has ever run (33 compile; compiling is not passing), WARN-5 stays instrumented-not-discharged, and every playable check belongs to TASK-448. Flip applied by the ORCHESTRATOR after manager handed the board back.) ← prior: **qa-passed / ready-for-integration** (2026-08-03 — `qa/TASK-461.md`: **PASS, 0 blockers**, 2 warns, 2 nits. Flip applied by the ORCHESTRATOR; qa-reviewer has no partial-edit tool. ✅ **THE SECOND-SITE DEPARTURE IS RATIFIED, verified at the code rather than the handoff:** both sites are the identical defect (same `HardTimeoutSeconds`, same `"%.1fs elapsed"`, both in `FSiegeLlamaWorker::RunRequestUnguarded`) and both are now correct — prefill prints `FPlatformTime::Seconds() - StartSeconds`, decode prints `Now - StartSeconds` reusing the clock read that already tripped its own test. ⚠️ **QA's own words: "Declining the departure would have been the defect"** — worst TTFT on ANY tier is 2913.8 ms (29% of the ceiling) and TTFT *upper-bounds* prefill, while partial's worst wall was **8926.3 of 10000**, one slow token from the decode branch. ✅ **§23 base verified at the symbol:** `HardDeadlineSeconds = StartSeconds + HardTimeoutSeconds` ⇒ `printed − 10.0` IS the overshoot; `PrefillStart`/`DecodeStart` were both live at both sites and **either would have been wrong**. ✅ **§22 sweep by QA found NOTHING FURTHER, and that is a result** — the three other `HardTimeoutSeconds` prints are ceilings *labelled as ceilings*; the only surviving instance of the shape is the §16-pinned `SiegeLlamaSpike.cpp`, correctly untouched. ✅ Fences held: `10.0` verbatim, header never opened, 457's guards/placement/`bAborted` byte-intact. ⚠️ The `%.1f` restraint was ruled RIGHT — `10.0s` now means *"overshoot under 50 ms"*, which the line could not previously carry) ← was: backlog
- delivered: >
    ⛔ **THE DEFECT WAS AT TWO SITES, NOT ONE — AND THE UNNAMED ONE IS THE ONE THAT CAN ACTUALLY FIRE.** Both fixed in
    `SiegeLlamaSubsystem.cpp`, both **one argument**, both in `RunRequestUnguarded`: the **PREFILL** timeout detail string (the site WARN-3 cited)
    now passes `FPlatformTime::Seconds() - StartSeconds`, and the **DECODE** timeout detail string — **identical defect, same constant, same
    `"%.1fs elapsed"` field, cited by nobody** — now passes `Now - StartSeconds` (the pre-existing clock read that trips its own test, so **no new
    read**). ⚖️ **DECLARED DEPARTURE (§15), separable in one line if ruled out:** the spec's *"change nothing else"* enumerates flag / guard /
    placement / `HardTimeoutSeconds` and **I touched none of the four** — but **measured prefill is 417-2914 ms worst-case on ALL THREE tiers**
    (`TASK-413:944-948`), so **the prefill ceiling is barely reachable**, while **partial's worst DECODE wall was 8926 ms of 10000 ms.**
    ⇒ ⛔ **Fixing only the named site would have left TASK-447 quoting a constant anyway — this task's own failure mode, achieved by a task that
    reported success.**
    **✅ HOW I KNOW IT VARIES (the spec's test):** the substituted expression is the **same subtraction over the same two symbols** that already
    computes `TtftMs` and `WallMs` in this function — and **TASK-413 published both VARYING across tiers and iterations** (TTFT 417.3/1655.4/2716.1
    ms; wall worsts 2937.5/8926.3/10000.6 ms). **Empirical, not argued.** ⚖️ **And the BASE is the check QA should make, not the argument:**
    `HardDeadlineSeconds = StartSeconds + HardTimeoutSeconds`, so **`printed - 10.0` IS the overshoot**; ⚠️ **`PrefillStart`/`DecodeStart` are both
    in scope at both sites and are the plausible WRONG answer** — they vary too, but cannot be compared with the ceiling.
    ⚠️ **HONEST LIMIT:** `%.1f` floors at ~50 ms, so a tiny overshoot still renders `10.0s` — **a measurement that rounds to the ceiling, not a
    constant that ignores it.** ⛔ **`%.1f`→`%.2f` NOT taken (not the argument); flagged for the manager.**
    ⛔ **FENCES HELD:** `HardTimeoutSeconds` **still 10.0** · **header never opened** (mtime 13:25 vs the `.cpp`'s 13:49) · **no header claim
    softened** · TASK-457's **guards, placements and `bAborted` byte-intact** · 🔒 **`SiegeLlamaSpike.cpp` UNMODIFIED.**
    **§14 INSTRUMENT:** the `.cpp` is **UNTRACKED** (positive control run first: `CLAUDE.md` exit 0, target exit 1), so **`git diff` shows NOTHING
    and would read as "changed nothing"** ⇒ used **`git diff --no-index`** vs a byte-exact pre-edit snapshot: **28 insertions / 2 deletions** =
    the 2 arguments + 26 comment lines. 📌 **The spike IS tracked, so plain `git diff --stat` is valid THERE and returned empty.**
    ⚠️ **TWO FINDINGS FOR THE MANAGER / TASK-447, DECLARED NOT FIXED:** **(a)** 🔒 `SiegeLlamaSpike.cpp`'s `DEADLINE %.1fs elapsed` has the **same
    shape** (§16 — not mine), and **TASK-413's CPU numbers came from the spike**, so **447 may still read a configured number there.**
    **(b)** ⛔ **THE FIXED LINE IS THE GPU-TIER LINE.** On `tier=cpu` the deadline exits via `abort_callback` mid-graph → `llama_decode` returns 2 →
    a **different branch with NO elapsed field**, so **the HARD TIMEOUT line usually does not print on CPU at all.** ⇒ **447 must not read its
    absence as "no timeout"** (§21, pointed at the instrument).
    **M8 DECLARATION, verbatim:** *adds no replicated property, no new replicated class, no new relevancy tier.*
    ⛔ **No compile, no editor, no MCP/PIE, no Git, no `Content/`.**
- blocked-by: **none — DISPATCHABLE NOW.** ⛔ **MUST LAND BEFORE TASK-447.** Sequential re-ownership after the FINISHED TASK-457. ⚠️ **PLUGIN module — parallel-safe against everything game-side.**
- parallel-safe: yes; **EXCLUSIVE owner of `Plugins/SiegeLlama/Source/SiegeLlama/Private/SiegeLlamaSubsystem.cpp`**
- spec: >
    ⛔ **`SiegeLlamaSubsystem.cpp:1557-1559` PRINTS `HardTimeoutSeconds` INTO A `"%.1fs elapsed"` FIELD ⇒ IT ALWAYS PRINTS `10.0s`.** The
    overshoot the header promises is bounded **can therefore never be measured**, and a large overshoot would be **invisible**.
    ⚖️ **THIS IS THE §17 SHAPE APPEARING INSIDE THE FIX THAT WAS WRITTEN TO CLOSE A §17 DEFECT — a stated bound standing in for a measurement.**
    ⚠️ **RECORDED WITHOUT BLAME: TASK-457's control flow, flag choice and guard placement are all CORRECT and QA confirmed it. This is the log
    line only** — which is precisely why it is a separate one-argument task and not a re-open.
    **(1) THE WHOLE FIX IS ONE ARGUMENT: pass `FPlatformTime::Seconds() - StartSeconds`** instead of the constant. ⛔ **Change nothing else** —
    not the flag, not the guard, not its placement (bottom-of-body is load-bearing, §21), not `HardTimeoutSeconds` (**stays 10.0**).
    **(2) ⚠️ WHY IT BLOCKS TASK-447 RATHER THAN FOLLOWING IT:** 447's **only reachable exercise of the deadline is the CPU tier**, and **this is
    the number it would quote.** ⛔ **Left unfixed, 447 would faithfully report a constant and the report would look like a measurement** — the
    exact false green this batch has spent its whole length hunting.
    **M8 DECLARATION DUTY: state verbatim in the handoff — "adds no replicated property, no new replicated class, no new relevancy tier."**
    Handoff `handoffs/TASK-459-programmer.md`. Post in ⚙️ Dev & QA.
- names: >
    `Plugins/SiegeLlama/Source/SiegeLlama/Private/SiegeLlamaSubsystem.cpp` · `FPlatformTime::Seconds` · `HardTimeoutSeconds` (⛔ **unchanged, 10.0**).
    Law: CONVENTIONS "Settings screen…" §17, §21.

#### TASK-469 — [QA-VOCAB] QA gate over TASK-463 + TASK-465 (+466 if landed) (qa-reviewer)
- assignee: qa-reviewer
- status: **done** (2026-08-03 — report `qa/TASK-469.md`. **PASS / PASS, 0 blockers**, 2 WARN, 2 NIT. Slack posted in ⚙️ Dev & QA. Flip by the ORCHESTRATOR. ⚠️ **WARN-2 INDEPENDENTLY FOUND THE SECOND PREPOSITION FRAME** — `Assistant_Order_Place` (*"Guard to mine_near"*), reachable because `who:"all"` is a legal parse for a selection-bearing verb — **without knowing TASK-471 had already fixed it. Two reviewers, same uncited frame, arrived at separately: corroboration, not duplication.** ⚠️ **WARN-1** — the `BeginPlay` log: QA-464's WARN-3 was false in **both** clauses; TASK-463 repaired the first and *"a model fault can only ever disable the console"* **survives verbatim while the latch has zero callers**. Log-only, safe direction ⇒ routes to **TASK-466**. ⛔ **COVERAGE LEDGER (§29): this gate names 463 + 465 ONLY. TASK-466 has NOT landed — `MarkAssistantFaulted` still has zero call sites — and TASK-468 MUST NOT treat it as gated.**)
- blocked-by: TASK-463 + TASK-465 (`ready-for-qa`) · ⛔ **BLOCKS TASK-468**
- parallel-safe: yes (read-only, own report)
- spec: >
    **Scoped gate, §27 shape — the diffs, not a re-review.** ⛔ **§29: every code task names a gate, and these are the ones that do not yet.**
    **(1) ⛔ THE ONE THAT MATTERS MOST — TASK-463's SEAM FIX.** The diagnosis is **(a) the asset is genuinely absent** (positive control: the same
    search shape finds `DA_BattlefieldScatter.uasset`, so the negative is about the repo, not the search) **and the C++ defaults were NEVER empty —
    27 rows since TASK-417.** The defect was a seam: `GetVocabulary()` returned the raw `LoadSynchronous()` result, so `BuildZoneA(nullptr)` took
    its documented `else` branch. ✅ **Verify the closed-form proof rather than accept it: `5116 − 2092 + Len("none\n") = 3029` — the EXACT figure
    PIE printed**, from an offline re-implementation of `BuildSynonymTable()` with no engine resident. ⚖️ **That is a proof, not an inference, and
    it is the standard to hold the fix to.**
    **(2) ⚖️ CONFIRM THE FIX SATISFIES §12a's AMENDED (a): the shipped lane must render the SAME Zone A as the measured lane, HOWEVER ACHIEVED.**
    ✅ **`NewObject<USiegeAssistantVocabulary>(this)` is deliberately the SAME construction `TwoLaneByteEquality` uses**, so that test's PASS
    becomes a statement about **the object the shipped lane actually renders.** ⛔ **Asset-resolution is NOT required and must not be demanded** —
    §12a(a) named a mechanism where it meant a property, and it is amended, not waived.
    **(3) §15 DEPARTURE TO RULE: the missing-asset log drops Warning → Log**, on the grounds that **after the fallback a missing asset IS the
    measured lane**, so a Warning would flag the *correct* state. ⚖️ **Sound on its face — rule it, do not leave it open.**
    **(4) ✅ RATIFY TASK-463's ITEM (3) RESTRAINT: `EnsureStaticPrefixRegistered` is BYTE-UNMODIFIED.** It closes the window on **match 2+ / level
    travel** and is a **deliberate no-op on cold start because the residual window is UNREACHABLE** (`RequestCompletion` returns false while
    `!IsReady()`). ⛔ **No tick, no timer — verify none was added.** ⚖️ **Adding machinery for an unreachable window is how a benign race becomes a
    maintenance burden.**
    **(5) TASK-465 — the shortfall template:** the verb is **parameterised from the parsed intent**, ⛔ **not forked into three copies**; every
    player-facing string stays **game-authored** (§3); and **the §22 sweep for other templates hard-coding a verb/kind/place is reported — a sweep
    that finds nothing else is a RESULT.**
    ⚠️ **STATE THE LIMITS: nothing here has been compiled since `cf8ef8e`, and NO AUTOMATION TEST HAS EVER RUN.** ⛔ **A PASS here is a PASS on the
    source.**
    Report `qa/TASK-469.md`. Post the verdict in ⚙️ Dev & QA.
- names: >
    `SiegeAssistantVocabulary.{h,cpp}` · `GetVocabulary` · `BuildSynonymTable` · `SiegeAssistantSnapshot.{h,cpp}` (`BuildZoneA`) ·
    `SiegeAssistantComponent.{h,cpp}` (`EnsureStaticPrefixRegistered`, `ShortfallCount`) · `ZoneA.TwoLaneByteEquality` ·
    `ZoneA.NullVocabularyIsNotTheMeasuredLane`. Law: CONVENTIONS §12a (amended (a)) · "Settings screen…" §15, §22, §27.

#### TASK-470 — [W1-TESTRUN] 💡 RUN THE 33 AUTOMATION TESTS HEADLESSLY — the standing caveat's only real discharge (build-master, DIAGNOSE-FIRST)
- assignee: build-master
- status: **done — THE ROUTE WORKS. 33/33 EXECUTED, 33 Success, 0 fail, 0 warn, 0 not-run, 0 skipped (2026-08-03).** ⛔ ***"NO AUTOMATION TEST HAS EVER RUN" IS DISCHARGED — WARN-5 CLOSED.*** Report `index.json`: `succeeded=33 / failed=0 / notRun=0 / succeededWithWarnings=0`; suite wall time **0.070 s** after a ~19 s editor boot. ✅ **THE §12a(b) UNKNOWN IS ANSWERED AND THE ANSWER IS GOOD: `ZoneA.TwoLaneByteEquality` PASSED — shipped 5116 chars / 5116 UTF-8 bytes == spike 5116 == measured reference 5116.** ⇒ **TASK-463's vocabulary-fallback change did NOT move Zone A; the two lanes still agree and `zoneA_chars=5116` is NOT stale.** ✅ **`ZoneA.NullVocabularyIsNotTheMeasuredLane` PASSED** — the test that already asserted this morning's broken identity has now actually been run. ⚠️ **FILTER WIDENED, DELIBERATELY: the spec's `Siegebound.Assistant` matches only 26 of 33 and would have silently omitted all 7 `Siegebound.Settings.*` tests. Ran `Siegebound` (33) instead — a WIDENING, never a narrowing (fence 2).** ⛔ **`-nullrhi` §20 QUESTION TRACED AND CLOSED: compatible. The `SiegeLlama` plugin startup is delay-load ONLY (`StartupModule` resolves DLL handles; `llama_backend_init` is lazy inside `EnsureBackendsLoaded`), so it never touches UE's RHI — it loaded cleanly, 4 modules, zero warnings. No test needs a `UWorld` or rendering.** 🔒 **`L_Arena` UNTOUCHED AND NEVER EVEN LOADED — SHA256 `B3DBC5D9AE484A7B…F8268` IDENTICAL before/after, 535,522 B, mtime still 2026-07-29; ZERO occurrences of `L_Arena` in the whole 110 KB log.** ⚠️ **AND THAT WAS NOT FREE — `EditorStartupMap=/Game/Maps/L_Arena.L_Arena`, so the spec's bare invocation WOULD HAVE OPENED IT.** It was prevented by `-ini:Engine:[/Script/EngineSettings.GameMapsSettings]:EditorStartupMap=/Engine/Maps/Entry.Entry`; the editor loaded `Engine/Content/Maps/Entry.umap` instead. ⛔ **THE TRAP THE NEXT RUNNER MUST KNOW: the belt-and-braces positional arg `/Engine/Maps/Entry` was MANGLED BY GIT BASH MSYS PATH TRANSLATION into `C:/Program Files/Git/Engine/Maps/Entry` and silently failed to resolve — the `-ini:` override is the ONLY thing that actually worked.** ⚖️ **A leading-slash UE object path CANNOT be passed positionally from Git Bash.** Raw `$?` was **0** and **was NOT used as the verdict** (verdict parsed from `index.json` + `Test Completed. Result={…}` lines). ⛔ **NO code, NO `Content/`, NO Git, NO commit — nothing in the repo changed.**
- blocked-by: **TASK-468** (needs the fresh binaries) · ⚖️ **does NOT block TASK-448 — Jonathan's feel test is independent of whether our suite runs**
- parallel-safe: yes vs TASK-448
- spec: >
    ⛔ ***"NO AUTOMATION TEST HAS EVER RUN"* HAS BEEN THE CAVEAT ON EVERY GATE TODAY.** 33 tests compile, **and compiling is not passing.** ⇒ **This
    task is the only thing that can turn *"33 compile"* into *"N pass, M fail"*, discharge **WARN-5**, and — decisively — **EXECUTE
    `ZoneA.TwoLaneByteEquality` and `ZoneA.NullVocabularyIsNotTheMeasuredLane`, the two tests §12a(b) turns on.**
    ⚠️ **THE STING THAT MAKES THIS URGENT RATHER THAN TIDY: `NullVocabularyIsNotTheMeasuredLane` ALREADY ASSERTED THE EXACT IDENTITY THAT BROKE IN
    TASK-463. The test that would have caught the empty-synonym-table defect was written, and simply never run.** ⇒ ⚖️ **A written test is not a
    guardrail; a RUN test is.**
    **(1) ⚖️ DIAGNOSE FIRST — THE ROUTE IS PROPOSED, NOT PROVEN, AND IT IS LABELLED THAT WAY DELIBERATELY (§20 binds the proposer too).**
    Candidate: `UnrealEditor-Cmd.exe <project> -ExecCmds="Automation RunTests Siegebound.Assistant" -unattended -nopause -nullrhi -testexit="Automation Test Queue Empty"`.
    ✅ **Commandlets demonstrably work on this machine** — the same pattern drives `Tools/reimport_meshes.py`. ⛔ **BUT NOBODY HAS TRACED WHETHER
    `-nullrhi` IS COMPATIBLE WITH THESE TESTS, OR WHETHER THE `SiegeLlama` PLUGIN'S STARTUP TOLERATES IT.** **Establish that first and report it.**
    **(2) ⛔ IF THE ROUTE DOES NOT WORK, SAY SO AND STOP. DO NOT INVENT A SUBSTITUTE PROCEDURE.** ⚖️ **An honest *"this route is unavailable, here
    is why"* is a RESULT** — and it is worth more than a partial run whose scope nobody can state. ⚠️ **Do NOT weaken, skip or filter tests to
    obtain a green.**
    **(3) REPORT PER-TEST: name, pass/fail, and the failure text for every failure.** ⛔ **A count alone is not a result** (§22/§29's family: a
    number without its members cannot be audited).
    **(4) ⚠️ A FAILURE HERE IS A FINDING, NOT AN EMERGENCY.** These tests have never run, so **a first-run failure may be a defect in the TEST
    rather than in the code** — ⛔ **do not "fix" either side; report and route.** ⚖️ **The point of a first execution is to learn what is true, not
    to produce a green.**
    **(5) ⛔ NO CODE CHANGES, NO `Content/`, NO Git.** 🔒 `L_Arena` never opened. 🔒 The gen-2 holdout is not touched. 🔒 `SiegeLlamaSpike.cpp` stays (§16).
    ⚠️ **The editor close/reopen grant is SESSION-SCOPED (2026-08-03) and expires** — if this runs later, **ask, never force-kill** (§24).
    Handoff `handoffs/TASK-470-buildmaster.md`. Post the per-test results in 🔧 Build & Git.
- names: >
    `UnrealEditor-Cmd.exe` · `-ExecCmds="Automation RunTests Siegebound.Assistant"` · `-unattended -nopause -nullrhi` ·
    `-testexit="Automation Test Queue Empty"` · `ZoneA.TwoLaneByteEquality` · `ZoneA.NullVocabularyIsNotTheMeasuredLane` ·
    `Siegebound.Assistant.Grammar.RuleNameCharset`. Law: CONVENTIONS §12a(b) · "Settings screen…" §16, §20, §24 · §9c (the parser is the reviewer of record).

#### TASK-468 — [W1-INT2] ⛔ THE SECOND COMPILE + COMMIT RUN — boarded now as a PLANNED step (build-master)
- assignee: build-master
- status: **done — SECOND COMPILE GATE GREEN, COMMITTED 2026-08-03** (`b24edb5` code · `0cf98f2` record · `8ccb0d5` handoff; `main` **18 → 21 ahead, NOT pushed**). `Result: Succeeded`, 11/11 actions, **zero error / zero LNK / zero warning lines in the whole log**. `\0` was 0 and **was not used as the verdict** — on this project that value has now been **6 for an environment block, 6 for a real code failure, and 0 for success**. **Carries EXACTLY TASK-463 + TASK-465 + TASK-471**, gated by `qa/TASK-469.md` and `qa/TASK-472.md`, whose self-declared limits were kept **separate rather than merged into a wave-level claim**. ⛔ **TASK-466 HAS NOT LANDED — verified at the artifact, not repeated:** `MarkAssistantFaulted` has 1 declaration, 1 definition, **6 prose mentions in comments, and ZERO call sites**, so the fault latch remains inert. Makes no claim about TASK-473. ⚠️ **TWO THINGS THE GREEN DOES NOT MEAN, recorded because a second green invites the opposite reading:** (1) TASK-463 arming the prefix earlier *should* change the first capture — **but that is a prediction about unexecuted code**, the same shape TASK-455 labelled and the first execution then falsified; (2) ⛔ **TASK-463 CHANGED ZONE A, which is exactly the input `ZoneA.TwoLaneByteEquality` compares across lanes ⇒ whether the two lanes still agree is now UNKNOWN and unknowable without running the test.** Nothing executed this run.)
- blocked-by: **TASK-463 + TASK-465 (both `qa-passed` ✅) + TASK-472 (⚠️ RUNNING)** · ⛔ **MUST COMPLETE BEFORE TASK-448** — Jonathan cannot playtest binaries that predate the fixes he is being asked to judge.
- ⛔ **`blocked-by` CORRECTED 2026-08-03 — IT READ `463 + 465` AND WAS STALE.** ⚠️ **TASK-471 landed AFTER I wrote that line, and its gate TASK-472 is still running** — so `qa/TASK-469.md` correctly reported 468 *"unblocked on both prerequisites"* against a list that no longer described the batch. ⇒ ⛔ **Building without 472 would put uncommitted, ungated code into a green build — precisely what §27b exists to prevent.** ⚖️ **The board is read by three agents at once; a stale `blocked-by` is not a bookkeeping slip, it is an instruction to do the wrong thing.**
- ⛔ **AND CARRY `qa/TASK-469.md`'s OWN LEDGER NOTE, VERBATIM: *"this gate names 463 + 465 ONLY. TASK-466 has NOT landed — `MarkAssistantFaulted` still has zero call sites — and TASK-468 must not treat it as gated."*** ✅ **That is §29 working exactly as designed: a gate declaring what it did NOT cover.** ⇒ **468 commits 463/465/471 and must NOT imply coverage of 466 or 473.**
- parallel-safe: no — ⛔ **this is a compile gate; the QUIET-MODULE LAW applies exactly as it did at TASK-447**
- spec: >
    ⛔ **WHY THIS EXISTS AND WHY IT IS BOARDED NOW RATHER THAN REMEMBERED LATER (CONVENTIONS §27b): A COMPILE-GATE PASS ATTACHES TO A COMMIT,
    NEVER TO A LANE.** ✅ **TASK-447's green describes `cd5f4ed` and the four commits around it — it does NOT extend to TASK-463, TASK-465, or
    anything else landing after `cf8ef8e`.** ⚠️ **Build-master flagged `SiegeAssistantComponent.{h,cpp}` as modified-and-unstaged at hand-off and
    said its PASS did not cover them — correct, and the reason this task exists as a PLANNED step instead of an end-of-batch scramble.**
    **(1) ⛔ QUIESCE FIRST, exactly as at TASK-447** — every C++ task in `Source/GitClaudeUnrealTest/` finished with its handoff written.
    **(2) COMPILE.** ⛔ **`Build.bat` RETURNS EXIT 0 ON A FAILED BUILD — parse the log for `Result:`, never `$LASTEXITCODE`** (it has been wrong
    in both directions here). ⚠️ **Live Coding must be OFF: this batch adds reflected types and `Ctrl+Alt+F11` cannot introduce them (§26) — a
    full editor restart is MANDATORY, not preferred.** ⚠️ **Rule out the two false-attribution traps BY MEASUREMENT, never by assumption** (a
    `0x800711C7` in ~2 s is Smart App Control and is Jonathan-only; foreign diagnostics route to the owning lane and never count against this one).
    **(3) COMMIT on `main`, ⛔ NO PUSH** — the push is Jonathan's, standing law. ⚠️ **`main` is 18 ahead at this task's start; VERIFY the count with
    `git rev-list --count origin/main..main`, never from this board or from memory.**
    **(4) ⛔ THE LFS CHECK IS PER FILE KIND (§25).** If this commit is code-only, expect **`(Git: …)`** on every object and ⛔ **any `(LFS: …)` is
    the defect**; if any `.uasset` rides along, that one must be **`(LFS: …)`**. **Run `git check-attr filter -- <path>` per kind and paste it.**
    **(5) TREAT THE INDEX AS HOSTILE** — GitHub Desktop and the editor's Git provider both auto-stage. **Explicit pathspec only, re-check
    `git status --porcelain` after every `add`, verify by INVERSE FILTER.** ⛔ `reset --hard` / `clean -fd` **BANNED.** **Real hashes, read back.**
    **(6) 🔒 `L_Arena` NEVER SAVED — verify by SHA256 before and after, not mtime.** 🔒 **The gen-2 holdout is not opened.** 🔒 **`SiegeLlamaSpike.cpp`
    is NOT deleted (§16).**
    **(7) ⛔ SAY WHAT THIS GREEN DOES AND DOES NOT MEAN, in the same words TASK-447 used:** compiling is **not** passing · no smoke row has run ·
    **WARN-5 stays instrumented** · and ⛔ **this PASS attaches to THIS commit, not to the lane.**
    Handoff `handoffs/TASK-468-buildmaster.md`. Post the hashes in 🔧 Build & Git.
- names: >
    Build: `"C:/Program Files/Epic Games/UE_5.8/Engine/Build/BatchFiles/Build.bat" GitClaudeUnrealTestEditor Win64 Development -project="…/GitClaudeUnrealTest.uproject" -waitmutex`.
    Law: CONVENTIONS "Settings screen…" §25 (LFS per kind), §26 (Live Coding), §27b (a PASS attaches to a commit) · "⛔ THE QUIET-MODULE LAW" ·
    "GIT HAZARD LAWS" · "UE Build.bat exit code lies".

#### TASK-464 — [QA-442] ⛔ RETROACTIVE scoped gate on TASK-442 — the one task no gate ever named (qa-reviewer)
- assignee: qa-reviewer
- status: **done** (2026-08-03 — report qa/TASK-464.md. **PASS on TASK-442, 0 blockers**, 6 warns, 3 nits. Slack posted in ⚙️ Dev & QA. Flip applied by the ORCHESTRATOR. **§29's coverage debt is DISCHARGED.** ⚠️ **Top findings, severity as found, disposition owed to manager:** **WARN-1** SiegeAssistantComponent.cpp:944 — MarkAssistantFaulted has **ZERO callers**, so AssistantFaulted can never be true (7 matches = 1 decl + 1 def + **5 prose mentions in comments**, §14 instance 2; positive control confirmed the negative is about the code). ⛔ The fix is NOT the obvious one — :1098-1117 deliberately refuses to latch there and that refusal is correct. **WARN-2** FailedTimeout is **unreachable** ⇒ every 10 s hard timeout prints as FailedModelError. **WARN-3** THREE artifacts assert *"holds no reference into the plugin"* and **all three are false**, including a UE_LOG printed at every BeginPlay — §22's exact ledger shape. **WARN-4** the ShortfallCount template hard-codes **"Send"** on a path serving Guard and Ambush — ⚠️ **the ONLY finding a playtester can see without a log; worth closing before TASK-448.**)
- blocked-by: **none — DISPATCHABLE NOW.** ⛔ **Must complete BEFORE TASK-448** — Jonathan should not playtest FSM behaviour that no gate has named.
- parallel-safe: yes (read-only, own report)
- spec: >
    ⛔ **RULING, ON THE RECORD RATHER THAN BY SILENT FLIP — AND IT GOES AGAINST THE CHEAP ANSWER DELIBERATELY.** TASK-442's code shipped in
    **`cd5f4ed`** and **no gate ever named it** (`qa/TASK-439` = 436·437 · `TASK-424` = 423·450 · `TASK-446` = 440·441·443·444·449·451·453·454·455·456
    (+457) · `TASK-461` = 459 · `TASK-462` = the build-fix diffs). ⚠️ **By the letter of the hard gate — *nothing is committed without a PASS QA
    report* — that was not met.**
    ⚖️ **WHY NOT *"COVERED IN SUBSTANCE BY 446"*, WHICH IS THE TEMPTING ANSWER: A FILE REVIEW IS SCOPED TO ONE TASK'S CHANGES.**
    ✅ **The mitigation is REAL — `SiegeAssistantComponent.{h,cpp}` was genuinely read at 446 through 443/455/456, and 443's
    statement-for-statement edit to 442's own `ConfirmPressed` was explicitly ruled on.** ⛔ **But 442's OWN criteria were never applied by
    anyone.** ⚖️ **And CONSISTENCY decides it: at TASK-461 and TASK-462 I ruled that code changing after a PASS needs a verdict, with no exception
    for *"it only fixes the build."* A task whose review was NEVER SCOPED has a WEAKER claim than one whose review merely predated a diff — it
    cannot receive a MORE generous ruling.** ⇒ ✅ **The mitigation SCOPES this gate; it does not replace it. Keep it small.**
    **⛔ IN SCOPE — ONLY WHAT NO PRIOR GATE NAMED. Check 442's own deliverables:**
    **(1) ⛔ THE CENTRAL LAW — *"one utterance + one snapshot → one constrained JSON; a multi-turn model loop is a QA FAIL."*** ⚠️ **446 already
    checked this file-wide as its criterion (b) — CONFIRM rather than re-derive, and say you are confirming.**
    **(2) THE FSM STATES:** `Idle → Composing → Thinking → {AwaitConfirm | Clarify | Failed}` + `Deferred`, with **state pushed to the widget as a
    `uint8`, never an enum** (the widget-param law).
    **(3) THE REASON-CODE TEMPLATE TABLE — ⛔ every player-facing string is game-authored** (§3). **No model-produced text reaches the player.**
    **(4) THE ~15-LINE CLARIFICATION SHORT-CIRCUIT** — handles most clarification turns **with no model call at all**. ⚠️ **Verify it exists and
    that it cannot silently swallow a turn that SHOULD reach the model.**
    **(5) THE FAULT POSTURE:** `bAssistantFaulted` is a **session latch** disabling **the console and nothing else**; ⛔ **a model-load failure,
    GGML fault, timeout or missing GGUF must NEVER block match start or degrade any key** (§2).
    **(6) AUTHORITY:** refuses on `!HasAuthority()` **with the same approved wording as the keys** — ⛔ **never more capable than the keys.**
    **(7) THE M8 DECLARATION, VERBATIM** in `handoffs/TASK-442-programmer.md`.
    **(8) ⛔ NO UNIT REGISTRY / actor cache / dirty flag / subscription list** (§4 — rejected on sight).
    ⛔ **OUT OF SCOPE: everything 446 already ruled on** — the toggle off-path, the guard call, the executor, the confirm step, the deferred
    intent. **A retroactive gate that re-litigates a passed review is not being thorough; it is being unable to stop** (§27).
    ⚠️ **STATE THE STANDING LIMITS: the code is ALREADY COMMITTED (the commit stands — reverting green, in-file-reviewed code would be worse), and
    NO SMOKE ROW HAS RUN, so every behavioural claim here is about SOURCE.** ⛔ **A finding does NOT trigger a revert — it routes to a fix task.**
    Report `qa/TASK-464.md`. Post the verdict in ⚙️ Dev & QA.
- names: >
    `SiegeAssistantComponent.{h,cpp}` · `USiegeAssistantComponent` · `bAssistantFaulted` · `ESiegeAssistantIntent` ·
    `ParseSiegeAssistantCommand` · `LogSiegeAssistant`. Law: CONVENTIONS "In-match LLM command assistant" §1, §2, §3, §4, §9 ·
    "Settings screen…" §27 (scope), §29 (the coverage ledger).

#### TASK-462 — [QA-FIX] Compile-fix gate over the TASK-450 + TASK-444 diffs ONLY (qa-reviewer)
- assignee: qa-reviewer
- status: **done** (2026-08-03 — gate closed, PASS, 0 blockers. Its verdict covered the DIFFS, never a prediction that the build would be green; the build has since gone green independently. Flip applied by the ORCHESTRATOR.) ← prior: backlog
- blocked-by: **TASK-450 (`ready-for-qa`) + TASK-444's compile-fix pass** (both must land) · ⛔ **BLOCKS TASK-447**
- parallel-safe: yes (read-only, own report)
- spec: >
    ⚖️ **RULING, ASKED DIRECTLY AND ANSWERED DIRECTLY: YES, AND YOUR PROPORTIONALITY READ IS RIGHT.** `qa/TASK-446.md`'s PASS **cannot carry
    these diffs, because the code changed after 446 closed** — identical to the TASK-461/TASK-459 precedent, and the hard gate admits no
    exception for *"it only fixes the build."* ⛔ **BUT THIS IS A GATE OVER THE TWO DIFFS, NOT A RE-REVIEW OF ELEVEN TASKS.** **16 TUs compiled
    clean and nothing else moved.** Law: CONVENTIONS §27.
    ⚖️ **BOTH HALVES MATTER: skip the gate and an unreviewed change rides in on an old PASS; re-review the whole batch and the gate becomes
    expensive enough that someone argues it away next time — and it dilutes attention away from the only thing that actually moved.**
    **IN SCOPE — and keep it to this:**
    **(1) THE CHANGED LINES.** TASK-450's `C2555` fix (**three behavioural lines**) and TASK-444's 7-error fix pass.
    **(2) ⛔ THE FENCES, VERIFIED AT THE ARTIFACT — build-master already checked these; CONFIRM rather than assume** (relayed diagnosis is a
    lead): **`HardTimeoutSeconds` still `10.0` verbatim** · **TASK-457's bottom-of-loop deadline test, `bAborted`, and the extended guard
    BYTE-INTACT** (§21 — placement is load-bearing) · **TASK-459's BOTH elapsed arguments still based on `StartSeconds`** (§23 — the base is the
    measurement) · the plugin **header never opened** · 🔒 **the spike unmodified** (§16 — **it is TRACKED, so `git diff` IS valid here and
    returned empty; that is not a §14 blind spot**).
    **(3) ⛔ DID THE FIX INTRODUCE A NEW WAY TO LEAVE A FUNCTION OR LOOP (§20)?** ✅ **The trace says no and it is worth confirming: `Run()` has
    exactly ONE exit** — zero `return`s in the body, the single `break` leaves **the while loop, not the function**, and SEH cannot propagate
    because `SehInvoke`'s `__except` swallows the fault and returns `false` normally; both loop exits converge on one tail
    (`UnloadModel()` → `State.Set(Stopped)`).
    ⛔ **AND CONFIRM THE ANTI-REVERT NOTE IS PRESENT AT THE LINE:** converting that `break` into a `return` **reads as equivalent** and would
    **skip `UnloadModel()` and the `Stopped` transition ⇒ LEAK THE ~2.5 GB MODEL** and hand `JoinAndDestroy`'s `WaitForCompletion` a worker still
    claiming Busy. ⚖️ **The trap is the plausible repair, not the error — so the warning must live where the instinct strikes, not only in a handoff.**
    **(4) 📌 ONE DECLARED OPEN QUESTION TO RULE ON: should `Run()`'s return stay `0`?** The argument offered is that **nothing reads
    `FRunnable::Run`'s exit code** and faults surface through `EState::Faulted` plus the completion delegate, making a non-zero code
    **a claim no caller could act on.** ✅ **That reasoning is sound on its face — rule it, don't leave it open.**
    **⛔ OUT OF SCOPE: everything the earlier PASS already covered.** A build-fix gate that re-litigates a passed design is not being thorough;
    it is being unable to stop.
    ⚠️ **STATE THE TWO STANDING LIMITS PLAINLY: the LINK STEP HAS NEVER RUN, and this is NOT NECESSARILY THE COMPLETE ERROR SET.** ⛔ **Neither
    may be assumed away, and a PASS here is a PASS on the diffs — not a prediction that the build is green.**
    Report `qa/TASK-462.md`. Post the verdict in ⚙️ Dev & QA.
- names: >
    `Plugins/SiegeLlama/Source/SiegeLlama/Private/SiegeLlamaSubsystem.cpp` (`FSiegeLlamaWorker::Run`, `SehInvoke`, `UnloadModel`,
    `JoinAndDestroy`) · `SiegeAssistantConsoleWidget.{h,cpp}` · `HardTimeoutSeconds` **10.0** · `StartSeconds` · `bAborted`.
    Law: CONVENTIONS "Settings screen…" §20 (trace before typing; the plausible repair is the trap), §21, §22, §23, §27 (scope), §28.

#### TASK-461 — [LLM-QA4] QA pass on TASK-459 — small, and owed (qa-reviewer)
- assignee: qa-reviewer
- status: **done** (2026-08-03 — gate closed, PASS, 0 blockers. Its verdict covered the DIFFS, never a prediction that the build would be green; the build has since gone green independently. Flip applied by the ORCHESTRATOR.) ← prior: **qa-passed** (2026-08-03 — report `qa/TASK-461.md`. **PASS, 0 blockers**, 2 warns, 2 nits. Slack posted in ⚙️ Dev & QA. Flip applied by the ORCHESTRATOR. **⇒ TASK-447 IS UNBLOCKED AND DISPATCHED.** ⚠️ **Two findings TASK-447 must carry:** **WARN-1** — the comment at the decode site **mislabels** TTFT and total-wall figures as *"prefill"* and *"DECODE wall"*; **TASK-413 never published a prefill figure**, so 447 must quote *"worst TTFT (an upper bound on prefill) 434/1706/2914 ms"* and *"partial's worst total wall 8926 of 10000"*. **WARN-2** — the decode `llama_decode=2` string lacks the prefill twin's three-cause sentence, **and that is the branch 447 lands in on `tier=cpu`**; pre-existing, and 459 was right not to touch it. Disambiguator: `bCancelled = bAborted && bCancelRequested`, so code 2 with `bCancelled` false is **never** a player cancel. ✅ **CPU carry confirmed by tracing three call sites:** on `tier=cpu` the abort exits via `llama_decode=2`, a branch with **NO elapsed field** ⇒ ⛔ **absence of a `HARD TIMEOUT` line is NOT "no timeout".** 📌 QA explicitly did **not** certify: the compile (447's gate), the mtime/`git diff` proofs (no shell in that role — it verified the underlying properties at the files instead), or that either line ever prints.)
- blocked-by: TASK-459 (`qa-passed`) — **cleared**
- parallel-safe: yes (read-only, own report)
- spec: >
    ⚖️ **RULING ON WHY THIS EXISTS AT ALL — asked directly, answered directly: `qa/TASK-446.md`'s PASS CANNOT CARRY TASK-459, because the CODE
    CHANGED AFTER 446 CLOSED.** ⛔ **The hard gate is *"no code commit without a PASS QA report"*, and 459's diff did not exist when 446 ran.**
    ⚠️ **AND THIS BATCH HAS SPENT ITS ENTIRE LENGTH PROVING THAT *"small and obviously right"* IS EXACTLY WHERE SILENT DEFECTS LIVE** — the
    `Super::RebuildWidget()` ordering, the case-insensitive comparator, the `10.0s` constant itself. **The pass is expected to be quick; it is not
    optional.** ✅ **Keep it proportionate: this is a two-line semantic change plus comments, not a re-review of the subsystem.**
    **(1) ⛔ RATIFY THE §15 DEPARTURE — THE SECOND SITE. This is the substantive item.** WARN-3 cited the **prefill** timeout string; TASK-459
    also fixed the **decode** timeout string, **which nobody cited.** ⚠️ **Verify from the code that the two sites are genuinely the SAME defect
    (same `HardTimeoutSeconds` constant, same `"%.1fs elapsed"` field, both in `RunRequestUnguarded`) and that BOTH are now correct.**
    ⇒ ⚖️ **The measured case for the departure is strong and should be checked, not assumed: worst prefill is 417–2914 ms across all tiers while
    partial's worst DECODE wall was 8926 ms of 10000 — so the UNCITED site is the only one that can realistically fire.** ⛔ **Fixing only the
    named site would have left TASK-447 quoting a constant anyway.**
    **(2) ⚠️ VERIFY THE *BASE*, NOT MERELY THAT THE VALUE VARIES (CONVENTIONS §23).** The ceiling is
    `HardDeadlineSeconds = StartSeconds + HardTimeoutSeconds`, so **only a `StartSeconds`-based elapsed can be compared against it.**
    ⛔ **`PrefillStart` / `DecodeStart` were both in scope at both sites, both vary, and both would have been WRONG.** **Confirm the shipped
    expression is the `StartSeconds` one.**
    **(3) SCOPE-FENCE CHECK:** `HardTimeoutSeconds` **still 10.0** · the flag choice, the guard and its **bottom-of-body placement** (§21)
    **unchanged** · the `%.1f` **format deliberately NOT changed** (its ~50 ms floor is a **declared** residue — ✅ *a measurement that rounds to
    the ceiling* beats *a constant that ignores it*, and a fix that quietly widens its own scope is harder to review than one that states its
    residue) · **26 added comment lines against a "change nothing else" spec — rule whether they are proportionate.**
    ⚠️ **The file is UNTRACKED, so `git diff --stat` shows NOTHING** (§14 instance 3 / §18b): 459 correctly used **`git diff --no-index`**
    (28 insertions / 2 deletions). ⛔ **Do not ask for a `--stat` proof.**
    Report `qa/TASK-461.md`. Post the verdict in ⚙️ Dev & QA.
- names: >
    `Plugins/SiegeLlama/Source/SiegeLlama/Private/SiegeLlamaSubsystem.cpp` · `RunRequestUnguarded` · `HardDeadlineSeconds` / `StartSeconds` /
    `HardTimeoutSeconds` · `FPlatformTime::Seconds`. Law: CONVENTIONS "Settings screen…" §15 (declared departures), §17, §21, §22 (sweep the
    SHAPE, not the site), §23 (the base is part of the measurement).

#### TASK-457 — [LLM-WARN] The hard timeout is not a backstop on GPU tiers + two smaller TASK-424 warns (gameplay-programmer)
- assignee: gameplay-programmer
- status: **done — COMMITTED 2026-08-03 in `cd5f4ed`** (Wave 1 code. Build GREEN: both DLLs linked, **zero errors, zero warnings** — the link step ran for the first time, so the 22-seam pre-flight became a measurement. ⚠️ **Green means it BUILDS, not that it works**: no automation test has ever run (33 compile; compiling is not passing), WARN-5 stays instrumented-not-discharged, and every playable check belongs to TASK-448. Flip applied by the ORCHESTRATOR after manager handed the board back.) ← prior: ✅ **qa-passed → ready-for-integration** (2026-08-03 — `qa/TASK-446.md` plugin appendix, PASS · 0 blockers). ⛔ **ONE RESIDUAL, FIXED BY TASK-459 BEFORE TASK-447: WARN-3 — the new timeout log prints `HardTimeoutSeconds` into a `"%.1fs elapsed"` field, so it ALWAYS prints `10.0s` and the overshoot can never be measured.** ← was: ready-for-qa
- delivered: >
    **2026-08-03 — `handoffs/TASK-457-programmer.md`.** ⛔ **BOTH FENCES HELD, verified verbatim at the artifact after the last edit:**
    `HardTimeoutSeconds = 10.0` (`SiegeLlamaSubsystem.h:161`) **UNCHANGED** · the header's **`BACKSTOP`** promise on Partial/FullOffload
    **STANDS and was not softened** — what moved is *what makes it one* (the loop, not the callback), because **the CODE now implements it.**
    **(1) THE FIX:** a deadline test at the **BOTTOM of the prefill loop body** in `RunRequestUnguarded`, setting **`bAborted`** (never
    `bCancelled` — a timeout completing as *"cancelled"* would have the FSM tell the player their order was cancelled when nobody cancelled it).
    ⇒ **Enforced on ALL THREE tiers**, because the test reads a **clock on the worker thread**, not a device.
    ⭐ **BOTTOM, NOT TOP, AND THAT IS THE REVIEW POINT:** from turn two the prefill is ~347 tok against a 512-tok `n_batch` (§8, `DROP = 77.1 %`),
    so the loop body runs **ONCE** — a top-of-body test would look right and **be inert on the hot path.**
    **(2) ⛔ THE SECOND-ORDER BUG A ONE-LINE FIX WOULD HAVE CREATED:** the post-prefill guard read `if (bCancelled || bDecodeFailed)` and the new
    exit sets **neither** — it would have **fallen through into the sampler with a half-prefilled prompt**, generating a fluent grammar-valid
    command from a truncated one. **This assistant ORDERS UNITS**; that is §1's valid-shaped-wrong-answer, strictly worse than the defect closed.
    ⇒ guard extended to `|| bAborted`, **provably inert on every pre-existing path** (`bAborted` was previously set only alongside `bDecodeFailed`).
    **(3) WARN-1 — FIXED, NOT JUSTIFIED, and it STOPS BEING INERT WHEN TASK-455 LANDS.** `MarkBusy()` now refuses unless `Idle` (symmetric with
    `ClearBusy`) · the run loop **re-tests `Busy`** and **COMPLETES the request on the refusal branch** (`RequestCompletion` already returned
    `true`; dropping it would strand the FSM in "thinking" with no error and no log). ⚠️ **DECLARED DEPARTURE — I ALSO REORDERED
    `MarkBusy()` BEFORE `bWorkPending = true`, and QA's literal "two lines" is UNSAFE WITHOUT IT:** the run loop's wait is **bounded at 200 ms**,
    so the worker ticks unprompted, and with the flag raised first it could observe pending work, find the state still `Idle`, and **refuse a
    PERFECTLY HEALTHY request.** 📌 `SubmitRequest`'s own comment already documented the order the code did not implement — **§17's shape, one
    function away.** ✅ **Cross-lane order PRESERVED:** `ApplyPendingStaticPrefix()` still precedes the `bWorkPending` branch (TASK-455 depends on it).
    **(4) WARN-3 — citation only, `768` UNCHANGED.** Re-pointed to `TASK-413:625` (*"+2.0 to +4.6 ms on every tier"*); the full-tier `:638` figure
    is kept as the VRAM operand it actually is, and **both** partial-scoped bar-#1 rows (`:599`, `:1103`) were verified at the artifact.
    **(5) ⚠️ A THIRD OVER-CLAIM OF THE SAME SHAPE, FOUND AND DECLARED (§15):** the *"at most one ubatch"* bound was **wrong by 8x in the
    FLATTERING direction** — the between-slice test runs once per **`llama_decode` CALL** (up to `llama_n_batch(ctx)` = 512 tok), and llama splits
    that into `n_ubatch` graphs **internally, without returning to this loop.** ⛔ **A loop cannot bound a boundary it never returns through.**
    Corrected in both files I own; **`handoffs/TASK-450-programmer.md` §4 carries the same wrong figure and is NOT mine to edit.**
    📌 **NON-FINDING, recorded so nobody files it:** 🔒 `SiegeLlamaSpike.cpp` says **"ADVISORY"**, not "BACKSTOP" — **it is not over-claiming**;
    the shipped subsystem is simply now STRONGER. **§16 respected — read only, UNTOUCHED.**
    ⚠️ **UNVERIFIABLE WITHOUT TASK-447's COMPILE, and one of them is a scoring trap:** measured prefill is **~1.7 s on partial**, so the new
    timeout **will not fire in normal play** — ⛔ **absence of the log line is NOT evidence the fix works.** The reachable exercise is the **CPU
    tier**, where TASK-413 measured **2 of 5 generations hitting the ceiling.**
    ⛔ **SCOPE FENCE HELD:** no queue · no threading-model change · no game-lane edit · no `Content/` · no Git · no compile · no editor/MCP/PIE.
    ⚠️ **EVERY LINE NUMBER IN THIS SPEC AND IN `qa/TASK-424.md` IS NOW STALE — this task added ~200 lines to the `.cpp`. `:1344` is not where the
    between-slice test lives any more. VERIFY BY SYMBOL (§18c).** ⛔ **No `git diff` exists: both files are UNTRACKED**, so §18a is discharged by
    the handoff's line-by-line itemisation, not by arithmetic. **M8: adds no replicated property, no new replicated class, no new relevancy tier.**
- blocked-by: **none — DISPATCHABLE NOW.** ⚠️ **PLUGIN module — genuinely parallel-safe against TASK-455 / TASK-456 and against a game-module gate.** ⛔ **Must land BEFORE TASK-447**, whose smoke test exercises cancel/timeout behaviour.
- parallel-safe: yes; **EXCLUSIVE owner of `Plugins/SiegeLlama/Source/SiegeLlama/Private/SiegeLlamaSubsystem.cpp`**
- spec: >
    ⛔ **WARN-2 IS A FUNCTIONAL GAP, NOT A DOC NIT, AND THAT IS WHY IT IS A TASK RATHER THAN A WARNING THAT AGES.**
    **`SiegeLlamaSubsystem.cpp:1344`** — the between-slice check is **`if (bCancelRequested || bStopRequested)`** and **OMITS THE DEADLINE.**
    ⇒ ⛔ **On GPU tiers the hard timeout is NOT the backstop the header claims at `:130-135`.** ⚠️ **That is a header asserting a guarantee the
    code does not provide — CONVENTIONS §17's shape exactly: a claim about the call rather than about the world** — and it is the **worst**
    version of it, because the reassurance is written in the place a reviewer goes to check.
    **(1) MAKE THE DEADLINE A REAL BETWEEN-SLICE TERM** so a runaway generation is cut on **every** tier, not only where a cancel or stop happens
    to arrive. ⚠️ **`HardTimeoutSeconds` = 10.0 is the shipped value and is NOT yours to change** (§10, and Jonathan's feel pass owns it) — you are
    making the existing ceiling **effective**, not choosing a new one.
    **(2) ⚖️ IF THE FIX AND THE HEADER STILL DISAGREE, THE HEADER IS WHAT MOVES *ONLY* WHERE THE CODE IS RIGHT — NEVER THE OTHER WAY ROUND.**
    Say which you changed and why. ⛔ **Do not "fix" this by softening the header's guarantee into something the current code already does** —
    the guarantee is the correct behaviour and a truncated-or-runaway command is unusable, not merely slow (§12d).
    **(3) WARN-1, same file:** **`MarkBusy()` (`:349`) does not guard the fault latch although its twin `ClearBusy()` does**, and the run loop
    (**`:549`**) dispatches on **`bWorkPending` alone** without re-testing state. ⚠️ **An asymmetric pair is a latent defect even when no current
    caller can reach it** — make them symmetric, or **document in code why the asymmetry is correct.**
    **(4) WARN-3 — a CITATION fix, and the conclusion SURVIVES:** the VRAM reserve's derivation (**`:83-87`**) cites a **full**-tier figure
    (`TASK-413:638`) for a bar scoped to **partial** (`:599`). ✅ **The reserve is still justified — via `TASK-413:625` (*"+2.0 to +4.6 ms on every
    tier"*), which actually covers the tier in question.** ⇒ **Re-point the citation so the number stops carrying a subject it never had.**
    ⚖️ **This is the §12-family defect in miniature: a correct conclusion resting on a figure measured against a different noun.**
    **(5) ⛔ SCOPE FENCE — THIS IS A THREE-WARN CLOSURE PASS, NOT A REDESIGN.** ⛔ **No `HardTimeoutSeconds` change · no queue · no threading
    model change · no game-lane edits · no `SiegeLlamaSpike.cpp` (🔒 §16 — its deletion is UNSCHEDULED and it is not yours to touch) · no
    `Content/`, no Git, no compile.**
    **M8 DECLARATION DUTY: state verbatim in the handoff — "adds no replicated property, no new replicated class, no new relevancy tier."**
    Handoff `handoffs/TASK-457-programmer.md`. Post in ⚙️ Dev & QA.
- names: >
    `Plugins/SiegeLlama/Source/SiegeLlama/Private/SiegeLlamaSubsystem.cpp` · `MarkBusy` / `ClearBusy` · `bCancelRequested` / `bStopRequested` /
    `bWorkPending` · `HardTimeoutSeconds` **10.0** (⛔ **unchanged**) · `bAssistantFaulted`.
    Law: CONVENTIONS "In-match LLM command assistant" §1, §9, §10 · §12d (a truncated command is unusable, not slow) · "Settings screen…" §17.

#### TASK-456 — [W1-RELEASE-b] Route the assistant's `charge`/`fallback` through `ApplyArmyWideStance` — the call side (gameplay-programmer)
- assignee: gameplay-programmer
- status: **done — COMMITTED 2026-08-03 in `cd5f4ed`** (Wave 1 code. Build GREEN: both DLLs linked, **zero errors, zero warnings** — the link step ran for the first time, so the 22-seam pre-flight became a measurement. ⚠️ **Green means it BUILDS, not that it works**: no automation test has ever run (33 compile; compiling is not passing), WARN-5 stays instrumented-not-discharged, and every playable check belongs to TASK-448. Flip applied by the ORCHESTRATOR after manager handed the board back.) ← prior: ✅ **qa-passed → ready-for-integration** (2026-08-03 — `qa/TASK-446.md`, PASS · 0 blockers). ✅ **WARN-4 containment verified INDEPENDENTLY: every `bMatchEnded` occurrence brackets `CreateUnitGroup` (`:2497`, `:4162`) WITHOUT entering it**, so `send`/`guard`/`ambush` genuinely already behave that way ⇒ **its refusal to patch two arms is FULLY RATIFIED and TASK-458 stays ONE uniform follow-up.**
- status: **ready-for-qa** (2026-08-03 — handoff `handoffs/TASK-456-programmer.md`; reviews under TASK-446. ⛔ **NOT compiled** — TASK-447 is the one gate. ✅ **THE DEFECT IS CLOSED: the assistant now CALLS the entry point 454 built.** 🚩 **STOP CONDITION DISCHARGED — EXACTLY TWO CALL SITES, found BY SYMBOL (§18c; the board's `:1413`/`:1420` were stale as predicted — TASK-455 shifted this file ~93 lines and I never used those offsets).** ⚠️ **7 matching `SetUnitCommand` LINES, 2 CALL SITES** — the §14-instance-2 shape, stated so a reviewer counting matches does not think five sites went missing. Both sit in `USiegeAssistantComponent::ExecutePendingCommand` on the `Charge`/`Fallback` arms; post-edit `grep -c "Controller->SetUnitCommand"` = **0**. ✅ **DELETED, NOT SOFTENED — and it was FOUR artifacts, not two:** the two `//` divergence blocks **plus the two `UE_LOG(…, Warning, …)` statements carrying the same false sentence into the RUNTIME log (the worse of the two — a stale Warning misleads Jonathan mid-battle, not just the next reader).** The words *"divergence"*, *"does NOT run"*, *"standing group orders survive"* now appear **nowhere** in the file. ⚖️ **DECLARED ADDITION (§15) — QA MUST RULE:** against a spec saying *"NOTHING ELSE"* I added **12 lines** — a comment defending against the revert (**three artifacts still pin the old spelling: `CONVENTIONS.md:823` · `SiegeAssistantCommand.h:59-60` · `SiegeAssistantSnapshot.cpp:674`** ⇒ a future reader diffing code against §8's seam "fixes" it back) and a `Log`-level success line in **this file's own house idiom** (`:1577`/`:1621`/`:1644` all read *"EXECUTED X through … - the SAME API the <key> calls"*); severity `Warning`→`Log` is deliberate, since `Warning` was correct only while the branch was defective. **If QA rules the literal reading, stripping to a bare swap is two minutes.** ⚠️ **DIFF EVIDENCE — `git diff --stat` IS THE WRONG INSTRUMENT AND QUOTING IT WOULD HAVE BEEN THE §14 TRAP: this file is UNTRACKED (`??`), so plain `git diff` shows NOTHING and reads as "changed nothing."** Used `git diff --no-index` against a byte-exact pre-edit snapshot: **`17 insertions(+), 18 deletions(-)`, ONE HUNK `@@ -1491,25 +1491,24 @@`, file 2821→2820 lines.** All 18 deletions itemised (12 = deliberate destruction of false text, 6 = the two swaps); no line lost by accident. ✅ **DISTURBED NOTHING — PROVEN BY BEFORE/AFTER SYMBOL COUNTS, not asserted:** 455's `SnapshotTrimBudgetChars` 8→8 · `static_assert` 4→4 · `EnsureStaticPrefixRegistered` 3→3 · `SetStaticPrefix` 5→5; 443's `ExecuteAndReport` 5→5 · `IsConfirmBeforeExecuteEnabled` 3→3 · `bForceConfirmReview` 6→6 · `ClearConfirmPreview` 9→9. **`SiegeAssistantComponent.h` UNTOUCHED** (mtime 13:06:44 vs my 13:18:16); **TASK-454's controller CALLED, NOT EDITED.** ⛔ **443's OFF-PATH HOLDS STRUCTURALLY, NOT BY INSPECTION: `ExecutePendingCommand` has EXACTLY ONE CALLER — `ExecuteAndReport()` (`:1324`) — which is the convergence point of BOTH toggle arms (`:768` ON, `:1314` OFF)** ⇒ my edit is strictly downstream of both and **cannot** be toggle-conditional; no branch, no early return, no FSM read/write added. ⚠️ **ONE BEHAVIOUR CHANGE + ONE RULING OWED (handoff §6):** `bMatchEnded` now gates the assistant's charge/fallback (component had **ZERO** match-end references before) — post-match `charge` is now inert like T, the intended consistency — **BUT `ExecutePendingCommand` still returns `true` unconditionally, so a match-ended no-op reports `Executed`** (reachable: the deferred-intent path can fire after match end). ⛔ **NOT fixed on purpose:** *"no guard changes"* per spec, **and it is not confined to my arms — `CreateUnitGroup` has ZERO `bMatchEnded` references, so `send`/`guard`/`ambush` already do this.** Remedy is cheap and in-file whenever ruled (`HasMatchEnded()` is **public**, `SiegePlayerController.h:647`); 📌 **wants ONE uniform follow-up across all executor arms, not a two-line patch.** 📌 **STALE SEAM-SPELLINGS LEFT ALONE, LISTED FOR THE OWNER:** `CONVENTIONS.md:823` (**manager's — highest value**), `SiegeAssistantCommand.h:59-60`, `SiegeAssistantSnapshot.cpp:674`, plus `Component.h:182`/`:957` and `Component.cpp:19` in my own file — **none is FALSE** (`ApplyArmyWideStance`'s last statement IS `SetUnitCommand`), merely imprecise, a different class from the deleted divergence claims. **M8: adds no replicated property, no new replicated class, no new relevancy tier.** No Git, no editor/MCP/PIE, no `Content/`, no controller edit, no header edit.)
- blocked-by: **TASK-455** (same file, sequential — ⛔ never concurrent). ⛔ **Must land BEFORE TASK-446.** ✅ **SATISFIED — 455 landed first; I read its handoff and re-verified its four symbols by count.**
- parallel-safe: no; **EXCLUSIVE owner of `SiegeAssistantComponent.{h,cpp}`**
- spec: >
    ⛔ **THE DEFECT TASK-454 EXISTS TO FIX IS STILL LIVE. 454 BUILT THE ENTRY POINT; NOTHING CALLS IT.** In its own words:
    ***"I built the entry point the assistant CAN call; 'can' is not 'does'."*** ✅ **It correctly did not touch this file — it is not its.**
    **(1) THE WHOLE DELIVERABLE, AND IT IS TWO CALLS.** In the **`charge` and `fallback` execution paths**, replace the **bare
    `SetUnitCommand` latch calls** with **`ApplyArmyWideStance(...)`** (`ASiegePlayerController::ApplyArmyWideStance(ESiegeUnitCommand)`,
    `BlueprintCallable`, declared `SiegePlayerController.h:474`, implemented `.cpp:1006`). **Both key handlers already route through it** — after
    you, so does the assistant, and there is **one implementation reached by both paths**, which is the §2 contract.
    **(2) DELETE THE TWO DIVERGENCE-WARNING COMMENTS IMMEDIATELY ABOVE THOSE CALLS.** ⚠️ **They document a divergence that will no longer exist,
    and a stale warning is worse than none: the next reader will either "restore" the divergence to match the comment or waste a pass proving it
    is gone.** ⛔ **Delete them; do not soften them.**
    **(3) ⚠️ LINE NUMBERS ARE AS-OF-AUTHORING HINTS ONLY AND WILL HAVE MOVED — TASK-455 EDITS THIS FILE BEFORE YOU (CONVENTIONS §18c).** The
    sites read `:1413` / `:1420` with the comments at `:1412` / `:1419` when TASK-454 found them. ⛔ **VERIFY BY SYMBOL, NEVER BY OFFSET** —
    find the `SetUnitCommand` calls on the charge/fallback paths. **If you find a number of call sites other than two, STOP and report.**
    **(4) ⛔ NOTHING ELSE. NO controller edits** (`SiegePlayerController.{h,cpp}` is TASK-454's and is FINISHED — ⛔ **you may CALL it, you may
    not EDIT it**), **no FSM changes, no executor changes, no guard changes, no `Content/`, no Git, no compile.**
    ⚠️ **This is the LAST thing standing between the assistant path and the shipped key path behaving identically**, which is Jonathan's own
    hard playtest criterion — so keep it exactly this small and let QA see it in one glance.
    **M8 DECLARATION DUTY: state verbatim in the handoff — "adds no replicated property, no new replicated class, no new relevancy tier."**
    Handoff `handoffs/TASK-456-programmer.md`. Post in ⚙️ Dev & QA.
- names: >
    `SiegeAssistantComponent.{h,cpp}` · `ASiegePlayerController::ApplyArmyWideStance(ESiegeUnitCommand)` (**PINNED — call it, do not edit it**) ·
    `SetUnitCommand` (⛔ **no longer called directly from the charge/fallback paths**) · `ESiegeUnitCommand { Attack, Hold, Defend }`.
    Law: CONVENTIONS "In-match LLM command assistant" §2 · "Group orders — 3-zone HOLD + AMBUSH (2026-07-27)" (the T/E release law) ·
    "Settings screen…" §18c.

#### TASK-455 — [W1-BUDGET] The game-lane half of the budget wiring + the missing `SetStaticPrefix` caller (gameplay-programmer)
- assignee: gameplay-programmer
- status: **done — COMMITTED 2026-08-03 in `cd5f4ed`** (Wave 1 code. Build GREEN: both DLLs linked, **zero errors, zero warnings** — the link step ran for the first time, so the 22-seam pre-flight became a measurement. ⚠️ **Green means it BUILDS, not that it works**: no automation test has ever run (33 compile; compiling is not passing), WARN-5 stays instrumented-not-discharged, and every playable check belongs to TASK-448. Flip applied by the ORCHESTRATOR after manager handed the board back.) ← prior: ✅ **qa-passed → ready-for-integration** (2026-08-03 — `qa/TASK-446.md`, PASS · 0 blockers). ✅ **AND THE COUPLING I FLAGGED DID NOT BIND: `SetStaticPrefix` was ruled ON THE MERITS at `qa/TASK-424.md` ruling 1, and 455's caller is INDEPENDENTLY correct** ⇒ ⛔ **no §8 rewrite is owed.** ← was: ready-for-qa
- handoff: `handoffs/TASK-455-programmer.md` (reviews under TASK-446)
- ⚠️ **THREE DECLARED DEPARTURES (§15) — QA MUST RULE, THEY ARE NOT SLIPS:**
    **(a) `MaxSnapshotChars` RETIRED, and a THIRD constant `SnapshotTrimBudgetChars` = 1085 introduced in its place** (name new, value deliberately
    unchanged). ⛔ **The one real code reader was `BuildZoneC`'s roster TRIMMER, which is neither the authority nor the pre-filter** — both plugin
    bounds **REJECT** the turn, and only a trimmer degrades gracefully. Pointing it at the pre-filter's 3000 would have converted a narrowed roster
    into a REFUSED TURN. Value not re-derived, because §12c freezes Zone B/C bytes this wave (the `t0` tripwire).
    **(b) DELIVERABLES 2 + 3 REFUSED AS WRITTEN.** `MaxSnapshotTokens` = 400 and `SnapshotPreFilterMaxChars` = 3000 **already exist** at
    `SiegeLlamaSubsystem.h:149`/`:170` (public `static constexpr`, verified on disk per §14 — the file is untracked). ⛔ **Game-lane copies would be
    constants NOTHING IN THE GAME LANE ENFORCES — a guardrail that reports safe.** Shipped instead: **3 `static_assert`s** in
    `SiegeAssistantComponent.cpp` (the only TU seeing both lanes) turning §8's *"a pre-filter that can reject what the authority would accept is a
    second hidden authority"* from prose into a **compile error**, at zero new header coupling.
    **(c) `ZoneBCharReserve` LEFT AT 192 — the spec's OWN fallback clause, fired.** ⛔ **No printed figure exists:** the 68/71 on record were printed
    by the **spike's** `AppendZoneB` (other lane, §12g's standing WARN), and `ReportFirstCapture` prints the shipped builder's `zoneB_chars` but
    **HAS NEVER EXECUTED** (batch uncompiled; TASK-447 is the gate). **A derived worst case is not a measurement** (§12g) — and the 192 it would
    replace was itself eyeballed. ⇒ The audit line now **prints the reserve, the live `zoneB_chars` and the over-charge** so the reading is one
    console-open away. **Lowering it only ever WIDENS the roster, so 192 is the safe end.** 📌 One-line follow-up task, needs a compiled editor.
- 📌 **COUNT CORRECTION, AGREEING WITH QA's 15 AGAINST TASK-450's "10":** `MaxSnapshotChars` had **15 matching LINES / 16 occurrences** —
    but only **5 lines (6 occurrences) were REAL CODE**; 3 were prose in `TEXT()` log strings and **7 were prose in comments** (§14 instance 2).
    450's total also mis-counted `h:274 ZoneBCharReserve` — a different symbol — into the figure. **Post-edit: 0 code references remain.** Four
    comment mentions survive **deliberately**, as this file's own established dead-spelling idiom (cf. `llama_kv_cache_seq_rm`) — flagged for ruling.
- ✅ **GUARDS NOW LIVE:** `MaxSnapshotTokens` authority · `SnapshotPreFilterMaxChars` pre-filter · the §8 ZONE-A SIZE BOUND assertion (now with a
    **MEASURED** `ZoneA_tokens`). **Context budget unchanged — always enforced.** ⛔ **NONE remain inert, but this is a prediction about unexecuted
    code:** TASK-447 confirms it by 450's three inertness Warnings CEASING plus a `STATIC PREFIX REGISTERED` line. ✅ That line is also the **shipped
    lane's first ever measured `zoneA_tok`**, discharging §12g's standing WARN without the spike.
- ⚠️ **DECLARED GAPS:** the turn-1 authority claim is a **RACE, not a guarantee** (worker can sit between `ApplyPendingStaticPrefix` and the
    `bWorkPending` branch; worst case the budget goes live one turn later, and the CONTEXT budget holds in every interleaving) · the char pre-filter
    is a **certain one-turn lag** if the console-open registration was skipped · `bStaticPrefixRegistered` records a **submission, not an outcome**.
- ⚠️ **BOARD `names:` CORRECTION:** the list says `MaxUtteranceChars`; the shipped symbol is **`MaxUtteranceBytes`** (renamed by TASK-433, *"the
    rename IS the fix"*). A task working from the board's spelling would grep a symbol that does not exist and conclude the cap is missing.
- blocked-by: **none — DISPATCHABLE NOW.** Sequential re-ownership of `SiegeAssistantSnapshot.{h,cpp}` (TASK-441, finished) + `SiegeAssistantComponent.{h,cpp}` (TASK-443, finished). ⛔ **Must land BEFORE TASK-446** — it edits two files that gate reviews.
- parallel-safe: **yes vs TASK-454** (disjoint files) — ⛔ never beside a compile gate
- spec: >
    ⚠️ **WHY THIS EXISTS: TASK-450's spec item (9) IS ONLY HALF-DELIVERABLE FROM THE PLUGIN, AND 450 SAID SO RATHER THAN FAKING IT.**
    `MaxSnapshotChars`'s retirement and `ZoneBCharReserve` live in the **GAME module**; the plugin cannot reach them. ✅ **The good news, and it is
    what makes this task small: the reading no longer needs the spike at all — `ReportFirstCapture` prints `zoneB_chars` from the SHIPPED
    builder.**
    **(1) RETIRE `MaxSnapshotChars`; INTRODUCE `MaxSnapshotTokens` = 400 AND `SnapshotPreFilterMaxChars` = 3000.**
    ⚠️ **COUNT CORRECTED 2026-08-03: the game lane carries `MaxSnapshotChars` on 15 matching lines, NOT 10.** ⛔ **My 10 came from a search count
    and conflated PROSE with CALL SITES — §14 instance 2, firing on me for the second time this wave.** ⇒ **Work from the SYMBOL and report what
    you actually find; if the true number is neither 10 nor 15, that is the finding and the spec is what is wrong.** ⛔ **RETIRED, NOT RE-POINTED**
    — 1085 is an **AUTHORITY** value that assumes the **SMALLEST** plausible chars/token ratio, and a **PRE-FILTER** must assume the **LARGEST**.
    ⚖️ **The safe direction INVERTS with the role, which is exactly why carrying the old constant across would be a silent defect wearing a
    familiar name.** (Floor for the pre-filter: `400 × 3.56 = 1424`; **3000 is generous by ruling** — erring high costs one wasted tokenize,
    erring low silently truncates.)
    **(2) SET `ZoneBCharReserve` FROM THE SHIPPED BUILDER'S PRINTED WORST CASE.** ⛔ **Do NOT eyeball 96 or 128** — the header says
    *"re-measure it, do not eyeball it."* It currently charges **192** for a Zone B measuring **68 (t0) / 71 (t1)**, worst case ~85.
    ⚠️ **If you cannot obtain a printed figure (no editor), STATE THAT AND LEAVE THE CONSTANT ALONE** — ⛔ **a derived number is not a
    measurement, and substituting one here is the exact defect §12g exists to prevent.**
    **(3) ⛔ ADD THE MISSING `SetStaticPrefix` CALLER — TWO OF THE THREE BUDGET GUARDS ARE INERT TODAY.** No game-lane caller invokes it, so they
    **cannot fire**. ✅ **450 logs this at Warning rather than presenting it as a pass — the correct handling of exactly the failure class this
    wave keeps finding — and the CONTEXT budget is always enforced, so there is NO unguarded path to `llama_decode`.** **Wire the caller so all
    three guards are live.**
    **(4) ⚠️ THE OBSERVABLE-TRUNCATION CONDITION IS RE-ANCHORED ONTO *THE ACT* AND IT BINDS YOU:** **any mechanism that truncates the snapshot,
    under any name, must LOG OBSERVABLY.** ⛔ **Your NEW cut sites are not covered by the snapshot builder's existing log.** A cap that silently
    degrades the prompt is **worse than a loose one that does not**.
    **(5) ⚠️ `MaxUtteranceChars = 240` BOUNDS *BOTH* THE `order:` AND `pending:` LINES, AND NEITHER IS TRUNCATED BY THE BUDGET — ONLY THE ROSTER
    IS.** Your enforcement must account for **both unbounded lines**, not just roster width.
    ⛔ **No plugin edits, no controller edits, no `Content/`, no Git, no compile.**
    **M8 DECLARATION DUTY: state verbatim in the handoff — "adds no replicated property, no new replicated class, no new relevancy tier."**
    Handoff `handoffs/TASK-455-programmer.md`. Post in ⚙️ Dev & QA.
- names: >
    `SiegeAssistantSnapshot.{h,cpp}` · `SiegeAssistantComponent.{h,cpp}` · ⛔ **`MaxSnapshotChars` (RETIRED — must not exist after this task)** ·
    `MaxSnapshotTokens` **400** · `SnapshotPreFilterMaxChars` **3000** · `ZoneBCharReserve` · `MaxUtteranceChars` **240** ·
    `USiegeLlamaSubsystem::SetStaticPrefix` · `ReportFirstCapture`.
    Law: CONVENTIONS "In-match LLM command assistant" §8, §10 · §12g · "Settings screen…" §7.

#### TASK-454 — [W1-RELEASE] The T/E release law must fire on the assistant path too — ONE public entry point (gameplay-programmer)
- assignee: gameplay-programmer
- status: **done — COMMITTED 2026-08-03 in `cd5f4ed`** (Wave 1 code. Build GREEN: both DLLs linked, **zero errors, zero warnings** — the link step ran for the first time, so the 22-seam pre-flight became a measurement. ⚠️ **Green means it BUILDS, not that it works**: no automation test has ever run (33 compile; compiling is not passing), WARN-5 stays instrumented-not-discharged, and every playable check belongs to TASK-448. Flip applied by the ORCHESTRATOR after manager handed the board back.) ← prior: ✅ **qa-passed → ready-for-integration** (2026-08-03 — `qa/TASK-446.md`, PASS · 0 blockers). ✅ **Its POSITIVE CONTROL is now the required pattern for every future *"nothing references X"* claim** (CONVENTIONS §14): before trusting a zero-hit search it proved the search COULD find a hit — `OnUnitCommandChanged` does appear in `WBP_HUD.uasset`.
- status: **ready-for-qa** (2026-08-03 — handoff `handoffs/TASK-454-programmer.md`; reviewed under TASK-446. ⛔ **NOT compiled** — TASK-447 is the one gate. **DELIVERED: ONE public entry point, `ASiegePlayerController::ApplyArmyWideStance(ESiegeUnitCommand)` — `SiegePlayerController.h:474`, `UFUNCTION(BlueprintCallable)`, impl at `.cpp:1006`.** ✅ **VERIFIED MYSELF, NOT RELAYED — 443 WAS RIGHT:** access-specifier map is `public:184 · protected:736 · protected:802 · private:1177`, and `CancelGroupPick` (**:1341**) + `ClearAllUnitGroups` (**:1458**) sit after `private:` ⇒ both private **and both STILL private after my edit — not widened, as ⛔ required.** ✅ **ZERO BEHAVIOUR CHANGE ON THE KEY PATH, BY READ-THROUGH:** the two handlers were byte-identical mirrors apart from the stance arg; each is now a single call whose body inlines to **the identical four statements in the identical order** — `bMatchEnded` guard **first**, then `CancelGroupPick()` → `ClearAllUnitGroups()` → `SetUnitCommand()`, release **BEFORE** latch exactly as the header documents. **No guard added or removed — in particular NO `HasAuthority()` gate was added**, since a client's T today runs the release locally and is refused only inside `SetUnitCommand` (:982); adding one would have been a silent change to shipped key behaviour. ⚖️ **DECLARED DEPARTURE (§15) — I REFUSED THE "no component edit at all" PREFERENCE, ON MECHANISM.** The only shape achieving it is folding the release into `SetUnitCommand`; I checked it was *safe* (**no BP callers: `grep -rl SetUnitCommand Content/` → zero hits, and I ran a POSITIVE CONTROL — `OnUnitCommandChanged` DOES hit `Content/UI/WBP_HUD.uasset` — so the negative is evidence, not a §14 blind spot**; no recursion; all 4 C++ callers want the law) **and refused it anyway:** `SetUnitCommand`'s header defines it as a pure latch · `ClearAllUnitGroups`' doc *"Called by T/E (before SetUnitCommand)"* would become self-referential · **the P2 RPC `ServerSetUnitCommand` is already named against the pure-latch contract (`.cpp:981`, M8 doc §4.3#9)** · and it recreates the half-sequence hazard from the other side. ⇒ **The primitive stays a primitive.** 🚩 **STOP-AND-REPORT, exactly as spec item (3) instructs: `SiegeAssistantComponent.cpp:1413`/`:1420` still call the bare `SetUnitCommand` and need a TWO-LINE swap to `ApplyArmyWideStance` (plus deletion of the now-false divergence `Warning` logs at `:1412`/`:1419`). ⛔ I did NOT touch the component — still `??`, mtimes unchanged 12:41/12:36. ⚠️ THE DEFECT IS THEREFORE STILL LIVE: my deliverable is the entry point the assistant *CAN* call, and "can" is not "does" — this needs one more task against the component.** ⚠️ **ADDITIVE PROOF + DECLARED DELETION DELTA: `git diff --stat` 757/94 → 836/108.** The header is **100% additive (264→305 insertions, deletions UNCHANGED at 10 — not one existing header line edited)**; the **+14 deletions are all in the `.cpp` and all MOVES, itemised line-by-line in the handoff** — 1 signature + 5 ATTACK comment lines re-emitted **verbatim** at `:1037-1043`, 1 latch line parameterised, and Defend's 7 statement lines which **ARE** the shared body at `:1028-1034`. ⚖️ **The brief's "deletions must still read 94" is unsatisfiable alongside the pinned contract** — two handlers cannot keep intact copies AND there be *"exactly ONE implementation"*; **board spec item (4) states it conditionally and I followed the board.** ✅ **440/449/453 UNDISTURBED, re-grepped not assumed:** 453's `AttachConsoleWidget` (**:4348**) still precedes `OpenConsole()` (**:4357**) — *absolute lines shifted 4324→4348 because I inserted ~72 lines above; the ORDER is the contract and it is unchanged* · 449's `SetAssistantConsoleOpen(true)` (**:4298**) still guard-first above `CreateAndAddToViewport` (**:4421**) · exactly ONE `bAssistantConsoleOpen =` writer (**:4240**). **M8: adds no replicated property, no new replicated class, no new relevancy tier.** No Git, no editor/MCP/PIE, no `Content/`.)
- blocked-by: **none — DISPATCHABLE NOW.** Sequential re-ownership of `SiegePlayerController.{h,cpp}` after the **FINISHED** TASK-453.
- parallel-safe: yes to WRITE (single owner) — ⛔ **never beside a compile gate**; **EXCLUSIVE owner of `SiegePlayerController.{h,cpp}`**
- spec: >
    ⚖️ **MANAGER RULING FIRST, BECAUSE IT DETERMINES THE SHAPE: THIS IS A CONSISTENCY FIX, NOT A DESIGN CHANGE, AND IT IS NOT JONATHAN'S CALL.**
    ⛔ **The divergence:** assistant `charge` / `fallback` run `SetUnitCommand` as §8 pins, **but the shipped T/E key path ALSO calls
    `CancelGroupPick()` + `ClearAllUnitGroups()` first.** ⇒ **Standing group orders SURVIVE an assistant "charge" and are cleared by the T key.
    Same words, two different outcomes.**
    ✅ **THE LAW ALREADY EXISTS AND THE CODE NAMES IT — verified at the artifact, not relayed:** `SiegePlayerController.h:1414` documents
    `ClearAllUnitGroups` as ***"Called by T/E (before SetUnitCommand) and by HandleMatchReset (Play Again)"***, and `:337` calls it
    ***"the T/E release law."*** ⇒ **This is a NAMED, SHIPPED rule that the assistant path is silently not honouring.**
    ⛔ **AND THE "IT WAS A TARGETED ORDER" DEFENCE DOES NOT SURVIVE CONTACT WITH THE STANCE SYSTEM:** a stance is a **LATCHED GLOBAL** state
    applying to all currently-alive **and future-spawned** player Standard units. **`SetUnitCommand` is army-wide whatever the sentence said** —
    so an assistant `charge` that leaves Hold/Ambush groups pinned to their zones produces **exactly the incoherent state the release law exists
    to prevent**: a global "charge" stance with squads still frozen on their ground, and no log naming why.
    ⇒ ⚖️ **§2 is the governing law and it is unambiguous: *"the assistant calls the same public APIs the keys call — never a parallel
    implementation, never a 'better' one."* The assistant is an alternative INPUT to the same action, not an alternative IMPLEMENTATION with its
    own semantics.** ⛔ **Jonathan's hard playtest criterion is that the two paths behave identically; this is the last thing standing between
    them.**
    **(1) ONE IMPLEMENTATION, TWO CALLERS — THE `CreateUnitGroup` IDIOM AGAIN (TASK-440 / TASK-397).** Extract the shared
    release-then-command sequence the T and E handlers perform into **ONE public entry point** on `ASiegePlayerController`, and have **both the
    key handler and the assistant call it.** ⛔ **Do NOT re-implement `CancelGroupPick` / `ClearAllUnitGroups` anywhere, and do NOT simply widen
    them to public** — two public teardown primitives invite a future caller to do half the sequence. **The entry point is the unit of
    correctness; the primitives stay private.**
    ⚠️ **THE EXACT SHAPE IS YOURS AND I AM DELIBERATELY NOT PINNING IT** (CONVENTIONS §15: a spec that asserts an ordering or a structure it has
    not executed is borrowing authority it never earned). **443 reports both methods are `private` — VERIFY that yourself**, then choose one
    entry point or two. **What is pinned is the CONTRACT: after this task there is exactly ONE implementation of the release-then-command
    sequence, and both the key path and the assistant path reach it.**
    **(2) ⛔ QA ACCEPTANCE IS "PROVABLY ZERO BEHAVIOUR CHANGE" ON THE KEY PATH, BY READ-THROUGH** (the TASK-397 / TASK-440 idiom). ⚠️ **The keys
    are the shipped, playtested behaviour and the assistant is the newcomer — if the two disagree about ORDER, the KEY PATH WINS and the
    assistant conforms.** The release must still happen **BEFORE** `SetUnitCommand`, exactly as the header documents.
    **(3) THE ASSISTANT SIDE IS A CALL, NOT A REIMPLEMENTATION.** ⛔ **You own the controller ONLY.** If the component needs a one-line change to
    call the new entry point, **STOP and report** — `SiegeAssistantComponent.{h,cpp}` is TASK-442/443's and a second writer is the failure this
    batch has serialized to avoid. ⚖️ **Prefer a shape where the component's existing call site needs no edit at all.**
    **(4) PRE-FLIGHT `git status --porcelain` on both files and BRANCH ON THE ANSWER — ⛔ do not predict it.** ⚠️ **Inherit the additive duty:
    your edit should leave the DELETION COUNT unchanged in `git diff --stat` unless the extraction genuinely moves lines — and if it does, SAY SO
    and account for every moved line** (449 and 453 both proved their claim this way).
    ⛔ **No component edits, no widget edits, no `Content/`, no Git, no compile.**
    **M8 DECLARATION DUTY: state verbatim in the handoff — "adds no replicated property, no new replicated class, no new relevancy tier."**
    Handoff `handoffs/TASK-454-programmer.md` with the read-through argument. Post in ⚙️ Dev & QA.
- names: >
    `SiegePlayerController.{h,cpp}` · `CancelGroupPick` / `ClearAllUnitGroups` (⛔ **stay private**) · `SetUnitCommand` ·
    `ESiegeUnitCommand { Attack, Hold, Defend }`. Law: CONVENTIONS "In-match LLM command assistant" §2 (**strictly additive — same public APIs
    the keys call**) · "Group orders — 3-zone HOLD + AMBUSH (2026-07-27)" (the T/E release law) · "Settings screen…" §15.

#### TASK-453 — [W1-JOIN] Hand the live console widget to the component at open time (gameplay-programmer)
- assignee: gameplay-programmer
- status: **done — COMMITTED 2026-08-03 in `cd5f4ed`** (Wave 1 code. Build GREEN: both DLLs linked, **zero errors, zero warnings** — the link step ran for the first time, so the 22-seam pre-flight became a measurement. ⚠️ **Green means it BUILDS, not that it works**: no automation test has ever run (33 compile; compiling is not passing), WARN-5 stays instrumented-not-discharged, and every playable check belongs to TASK-448. Flip applied by the ORCHESTRATOR after manager handed the board back.) ← prior: ✅ **qa-passed → ready-for-integration** (2026-08-03 — `qa/TASK-446.md`, PASS · 0 blockers). ✅ `AttachConsoleWidget` verified **idempotent (detach-before-bind), null-safe, re-targetable, and SEEDS BEFORE IT BINDS** — so a deferred prompt raised while the console was shut survives.
- status: **ready-for-qa** (2026-08-03 — handoff `handoffs/TASK-453-programmer.md`; reviewed under TASK-446 item **(j)**. ⛔ **NOT compiled** — TASK-447 is the one gate. ✅ **HARD STOP CHECKED AND IT DID NOT FIRE:** `AttachConsoleWidget` **exists** at `SiegeAssistantComponent.h:723`, `public:`, matching the pinned signature **character-for-character**; ⛔ **the component was NOT touched — not one byte.** ⚠️ **My first TWO greps returned NOTHING and were WRONG** — the component files are **untracked** (so `git grep` cannot see them) **and TASK-443 was writing them mid-session**; a reviewer repeating that probe may reproduce the false negative. **Delivered: ONE call, `Assistant->AttachConsoleWidget(Console)` at `SiegePlayerController.cpp:4324`** — resolved via `GetAssistantComponent()`, `if`-scoped and null-checked. ⛔ **PLACED BEFORE `Console->OpenConsole()`, AND THAT ORDERING IS THE TASK:** `OpenConsole()` **ENDS** with `OnConsoleOpenChanged.Broadcast(true)` and the component subscribes to it **inside** `AttachConsoleWidget` ⇒ attaching afterwards would miss the only "opened" broadcast of the first press and leave the FSM in `Idle` while the player types — **silent, recovering only on the SECOND open.** ✅ **449's guard-first ordering UNDISTURBED** — `SetAssistantConsoleOpen(true)` still runs before any creation and its refusal returns above my call, so a refusal still constructs nothing, shows nothing, moves no focus. ✅ **Called on EVERY open (not once at creation), so close-and-reopen re-establishes all eight forwards** — safe because the pinned contract is IDEMPOTENT + SEED-THEN-BIND; a creation-time attach would fire once with no recovery. ✅ **Null component ⇒ the console still opens, INERT** (one Warning, posture NOT rolled back — never refuse the console over an assistant defect). ✅ **ADDITIVE PROOF: `git diff --stat` went 715/94 → 757/94 — +42 insertions, deletion count UNCHANGED at 94**, and the header's 274 is unchanged too (⇒ `SiegePlayerController.h` **not touched at all**; the deliverable needed no header change). ⛔ **LINK-ORDER CONSTRAINT ON TASK-447, STATED LOUDLY: `USiegeAssistantComponent::AttachConsoleWidget` is DECLARED but has NO DEFINITION in `SiegeAssistantComponent.cpp` yet — this compiles but will NOT LINK until TASK-443 lands its `.cpp`. Expected (the TASK-416/417 precedent — do not open a QA loop); but TASK-447 MUST verify the definition exists BEFORE compiling, or the gate fails on my line for a defect that is not mine.** ⚠️ **My design DEPENDS on 443's idempotence contract being real — QA item (j) already owns that check and it is not optional.** No Git, no editor/MCP/PIE, no `Content/`.)
- blocked-by: **none for WRITING — DISPATCHABLE NOW, in parallel with TASK-443**, against the pinned signature below. ⚠️ **It will not LINK alone and is not expected to** (the TASK-416/417 precedent — do not open a QA loop over it). Sequential re-ownership of `SiegePlayerController.{h,cpp}` after the **FINISHED** TASK-449.
- parallel-safe: **yes vs TASK-443** (different files) — ⛔ **never beside a compile gate**; **EXCLUSIVE owner of `SiegePlayerController.{h,cpp}`**
- spec: >
    ⛔ **THE GAP: THE KEY OPENS A CONSOLE WHOSE ENTER GOES NOWHERE.** TASK-444 shipped the outbound seam — `OnConsoleSubmitted` /
    `OnConsoleConfirmed` / `OnConsoleCancelled` / `OnConsoleOpenChanged`, declared under the comment ***"Outbound seam. The owning
    `USiegeAssistantComponent` binds these."*** TASK-442/443 own the component. **Nothing joins them.**
    ⚖️ **WHY THIS IS A NEW TASK AND NOT AN AMENDMENT — THE RULING, SO IT IS NOT RE-LITIGATED.** ⛔ **TASK-443 is RUNNING**, its spec forbids
    controller edits, and **widening a live task's file ownership mid-flight is precisely the churn the single-owner law exists to prevent.**
    ⛔ **TASK-449 is FINISHED with its handoff written** — re-opening a closed task to add scope muddies the record it exists to be.
    ⇒ **A new, tiny, single-file task is the cheapest correct answer, and it costs nothing in wall-clock because it runs in PARALLEL with 443.**
    ⛔ **AND THE OBVIOUS FIX IS BARRED — IT FAILS SILENTLY, WHICH IS THIS PROJECT'S WORST CLASS.** Widget creation is **LAZY by TASK-449's spec**
    (first successful open), so `GetAssistantConsoleWidget()` is **NULL until then**. **A `BeginPlay` binder would bind nothing and no-op FOREVER
    — no error, no log, no crash.** ⇒ **The hand-off must happen AT OPEN TIME, in the thing that owns the open, which is the controller.**
    **(1) THE WHOLE DELIVERABLE, AND IT IS ONE CALL.** In TASK-449's open path, **immediately after a successful create/open**, call
    **`Component->AttachConsoleWidget(Widget)`** exactly once per widget. ⛔ **Nothing else. No FSM logic, no delegate bodies, no executor, no
    posture changes** — TASK-449's guard-first ordering and posture handling are **already correct and are not touched.**
    **(2) THE PINNED CROSS-TASK SIGNATURE — TASK-443 IMPLEMENTS IT, YOU CALL IT, character-for-character:**
    `void USiegeAssistantComponent::AttachConsoleWidget(USiegeAssistantConsoleWidget* InWidget);`
    **It is IDEMPOTENT and NULL-SAFE by contract** — you may call it on every open without double-binding, and the component owns all four
    delegate bindings inside it (that is what the shipped comment means by *"the owning component binds these"*).
    **(3) ⛔ IF `AttachConsoleWidget` DOES NOT EXIST WHEN YOU ARRIVE, STOP AND REPORT. DO NOT EDIT THE COMPONENT** — that file is TASK-443's and
    a second writer is exactly the failure this batch has been serializing to avoid. **A missing method is a routing problem, not yours to fix.**
    **(4) ⚠️ RESOLVE THE COMPONENT FROM YOURSELF, NULL-SAFE** (it is a default subobject created by TASK-440, so it exists from construction —
    but check anyway; a missing component must leave the console open and inert, never crash). ⛔ **§2 still governs: no keyboard command changes
    behaviour, and no key is re-routed through the assistant.**
    **(5) PRE-FLIGHT `git status --porcelain` on both files and BRANCH ON THE ANSWER** — ⛔ **do not predict it** (ruling 7's correction).
    ⚠️ **TASK-449's additive property is load-bearing and you inherit the duty: your edit must leave the DELETION COUNT unchanged** in
    `git diff --stat` on those two files. **That is the checkable half of "provably additive" and it is how 449 proved its own claim.**
    ⛔ **No component edits, no widget edits, no `Content/`, no Git, no compile.**
    **M8 DECLARATION DUTY: state verbatim in the handoff — "adds no replicated property, no new replicated class, no new relevancy tier."**
    Handoff `handoffs/TASK-453-programmer.md`. Post in ⚙️ Dev & QA.
- names: >
    `SiegePlayerController.{h,cpp}` · `USiegeAssistantComponent::AttachConsoleWidget(USiegeAssistantConsoleWidget*)` (**PINNED — TASK-443
    implements, TASK-453 calls**) · `GetAssistantConsoleWidget` · `OnConsoleSubmitted` / `OnConsoleConfirmed` / `OnConsoleCancelled` /
    `OnConsoleOpenChanged`. Law: CONVENTIONS "In-match LLM command assistant" §2, §9 · "Settings screen…" §8, §9.

#### TASK-450 — [LLM-A2b] `USiegeLlamaSubsystem` — the RUNNABLE subsystem (worker thread, queue depth 1, abort, timeouts, tiering, KV reuse) (gameplay-programmer)
- assignee: gameplay-programmer
- status: **done — COMMITTED 2026-08-03 in `cd5f4ed`** (Wave 1 code. Build GREEN: both DLLs linked, **zero errors, zero warnings** — the link step ran for the first time, so the 22-seam pre-flight became a measurement. ⚠️ **Green means it BUILDS, not that it works**: no automation test has ever run (33 compile; compiling is not passing), WARN-5 stays instrumented-not-discharged, and every playable check belongs to TASK-448. Flip applied by the ORCHESTRATOR after manager handed the board back.) ← prior: **ready-for-qa** (2026-08-03, **QA LOOP 1 of 3 — C2555 FIXED, re-submitted**. `qa/TASK-424.md`'s build-master appendix: **1 error**, `SiegeLlamaSubsystem.cpp(285,15)` — `FSiegeLlamaWorker::Run` declared `virtual void Run() override` against `FRunnable`'s `virtual uint32 Run() = 0`. **FIXED at both the declaration and the definition, plus ONE `return 0;`.** ⚖️ **§20 TRACED BEFORE TYPED, AND THE TRACE IS THE FINDING: `Run()` has EXACTLY ONE EXIT — it contains ZERO other `return` statements, and the single `break` leaves the WHILE LOOP, not the function.** Both exits converge on the same tail **after** `UnloadModel()` and the `Stopped` transition ⇒ ✅ **the conversion introduced NO new way to leave a loop, so no guard downstream of an early exit needed re-checking, and TASK-457's `MarkBusy()`/`bWorkPending` ordering is untouched.** ⛔ **The trap that was NOT taken: converting the `break` into a `return` would skip `UnloadModel()` + `State.Set(Stopped)` — leaking the ~2.5 GB model and letting `WaitForCompletion` return on a worker still reading Busy.** Recorded in an anti-revert comment. 📌 **§22 SWEEP — A RESULT, NOT A BLANK:** all 4 `FRunnable` overrides re-read against `HAL/Runnable.h:32-61` (`Init` `bool` ✅ · `Stop` `void` ✅ · `Exit` `void` ✅ · **`Run` was the only divergence**); the 2 header overrides (`Initialize`/`Deinitialize`) match `Subsystem.h:59/:62` ✅ **and were already compile-proven by the 16 clean TUs that include the header**; and `FSiegeLlamaWorker` is **the only `FRunnable` subclass in the entire repo** (the spike deliberately uses `AsyncThread`) ⇒ **the shape cannot recur elsewhere.** ✅ Fences verified at the artifact: `HardTimeoutSeconds` **verbatim 10.0**, TASK-457's bottom-of-loop deadline test + `bAborted` + the extended post-prefill guard **byte-intact**, TASK-459's **both** elapsed args still based on `StartSeconds`, header **not opened** (mtime 13:25 = 457's), 🔒 **spike unmodified** (tracked ⇒ `git diff` empty, 188,736 B). Braces 215/215; parens 938→955, **+17 fully accounted as comment prose, 0 in code**. ⛔ **NOT compiled — TASK-447 is the single gate and it re-runs.** ⚠️ **The link step has still never run, so unresolved externals remain unproven in either direction, and this is not necessarily the complete error set.** Handoff appended at `handoffs/TASK-450-programmer.md` §11. — PRIOR: **qa-passed** `qa/TASK-424.md`: **PASS, 0 blockers**, 8 warns, 7 nits. Flip applied by the ORCHESTRATOR. ✅ QA **independently re-read the vendored header** rather than resting on a negative search (§14): `LLAMA_LOAD_MODE_MMAP` = `llama.h:207`, `llama_memory_seq_rm` = `llama.h:735`, and it read the **whole** `llama_model_params` struct (`:306-341`) to confirm `use_mmap` is absent. No deprecated API. ✅ **No unguarded path to `llama_decode`** — the context budget at `:1265` is unconditional on every request. **Three rulings ratified:** `SetStaticPrefix` (⚠️ **§8 is genuinely self-contradictory — manager owes a one-line amendment**, the 4th correctly-refused spec line this wave), reject-not-truncate (**strictly stronger** than the clause), and synchronous `EnsureBackendsLoaded()` (**ACCEPTED with binding conditions — TASK-447 MUST report the `N ms` figure**, and *"async load never blocks match start"* is **QUALIFIED-PASS only**). ⚠️ Top warns, none blocking: **WARN-1** `:349` `MarkBusy()` does not guard the fault latch though its twin `ClearBusy()` does, and the run loop `:549` dispatches on `bWorkPending` alone · **WARN-2** `:1344` the between-slice check omits the **deadline**, so on GPU tiers the hard timeout is **NOT** the backstop the header claims at `:130-135` · **WARN-3** `:83-87` the reserve cites a **full**-tier figure for a **partial**-tier bar; the conclusion survives via `TASK-413:625` instead. ⛔ NOT compiled, NOT run — TASK-447 is the one gate)
- ⛔ **READ THE HANDOFF BEFORE THE QA GATE — IT DECLARES FOUR THINGS THAT CHANGE WHAT "DONE" MEANS HERE.**
    **(a) DELIVERED:** two new files only (`Plugins/SiegeLlama/Source/SiegeLlama/{Public,Private}/SiegeLlamaSubsystem.{h,cpp}`), zero edits to any existing file
    anywhere in the repo. `UGameInstanceSubsystem` · ONE `FRunnable` at `TPri_BelowNormal` owning model/context/vocab · queue depth 1 by early return with **no queue** ·
    `abort_callback` cancel (+ between-slice checks that bound a GPU-tier cancel to one `n_ubatch`) · the three timeouts · measured tiering + `llama_memory_seq_rm` KV reuse ·
    SEH + `bAssistantFaulted` latch + the completion delegate **structurally** game-thread-only · the absent-model path as exactly one log line · the §8 startup budget
    assertion with `ZoneA_tokens` **tokenized**, never a constant · `MaxSnapshotTokens` 400 + `SnapshotPreFilterMaxChars` 3000 with the role-change re-derivation declared.
    **(b) ⛔ THE SPIKE DELETION IS DEFERRED A SECOND TIME — ON A *NEW* REASON, AND TASK-423's OLD REASON IS EXPLICITLY DISCHARGED.** The decisive new one: the **generation-2
    sealed holdout (committed UNSPENT at `21f7e01`) is authored against the `t0` fixture INSIDE `SiegeLlamaSpike.cpp` and is scored by `Siege.Llama.SpikeEval`, also inside
    it** — deleting the file **spends a sealed artifact by destroying it**. Also: no `zoneA_tok` reading could be taken (editor forbidden this task) and the partial-tier
    **hitch sampler lives only in the spike**. ⚠️ **`SpikeCorpus.inl` DOES NOT EXIST** — the board's `names` line has named a nonexistent file since TASK-423. Nothing outside
    the spike references any spike symbol, so the deletion stays link-safe whenever it happens. Mitigation shipped: the subsystem logs a Warning at load naming both command
    families. **Unblock condition is spelled out in the handoff §7 as a 5-step single-session task.**
    **(c) ⛔ SPEC ITEM (9) IS ONLY HALF DELIVERABLE FROM THE PLUGIN.** `MaxSnapshotChars`'s retirement (10 sites) and `ZoneBCharReserve`'s re-measurement live in the **GAME
    module** (`SiegeAssistantSnapshot.h:266/:274` + `SiegeAssistantComponent.cpp`), which a plugin-scoped, parallel-safe task may not touch — and 9b additionally needs a
    **printed** reading no compile-free task can take. ✅ **The reading no longer needs the spike:** `USiegeAssistantComponent::ReportFirstCapture` prints `zoneB_chars` from
    the **SHIPPED** builder. ⇒ **MANAGER OWES ONE SMALL GAME-LANE TASK.**
    **(d) ⚠️ TWO OF THE THREE BUDGET GUARDS ARE INERT TODAY AND THE CODE SAYS SO AT WARNING.** `MaxSnapshotTokens` and `SnapshotPreFilterMaxChars` both need the Zone-A/B+C
    split, which needs the game lane to call `SetStaticPrefix` — and no game-lane caller exists yet. **The CONTEXT budget is always enforced, so there is no unguarded path to
    `llama_decode`.** ⛔ **Do not read a clean log as those budgets passing.**
- ⚠️ **DECLARED ADDITIONS TO THE PLUGIN SURFACE — QA MUST RULE, NOT SILENTLY ACCEPT OR DELETE.** The §9 four methods + delegate are untouched, character-for-character.
    Added: **`SetStaticPrefix(const FString&)`** (opaque string; exists because §8 states BOTH "the surface is `(prompt, gbnf) -> string`" AND that the budget assertion is
    mandatory with `ZoneA_tokens` tokenized from the assembled Zone-A string — **which the plugin cannot build**, so as written the two clauses cannot both hold; if QA rules
    it out, **§8's mandatory assertion has no implementable form and the clause must be re-opened**) · **`OnRequestSoftTimeout`** (parameterless multicast — §10 specifies the
    soft timeout as "tells the FSM to say something", and a constant the FSM polls is not a signal) · `GetOffloadTier` / `DescribeState` · **three `#if !UE_BUILD_SHIPPING`
    console commands** (`Siege.Llama.Status` / `.Test` / `.Cancel`) — ⛔ **without them TASK-447's inherited gate is UNRUNNABLE, because there is no game-lane caller.**
- ⛔ **TASK-447: THE GATE'S OWN SCOPE IS ANSWERED IN THE HANDOFF §6, ITEM BY ITEM, INCLUDING TWO IT CANNOT HAVE.** ✅ satisfiable: `Siege.Llama.Info` · one parseable JSON ·
    queue depth 1 (deterministic, one command: `Siege.Llama.Test twice` — **pass = call 1 TRUE, call 2 FALSE; two FALSEs proves nothing**) · the absent-model path.
    ⚠️ **qualified:** *async load never blocks match start* — the 2.5 GB read IS async, **but `EnsureBackendsLoaded()` is synchronous on the game thread** (declared, with the
    data-race reason, timed and logged — **TASK-447 reports that number**); *cancel mid-prefill* — **genuinely mid-graph ONLY on `tier=cpu`**; on GPU tiers `abort_callback` is
    documented CPU-only and the cancel lands at the next ubatch boundary, **so the run must state its tier**. ⛔ **NOT satisfiable: the partial-tier frame-time hitch
    re-measure — the frame sampler and histogram exist ONLY in the spike; the subsystem has no frame instrumentation and never was specced to. Run it on the spike in its own
    session or record it NOT MEASURED — do not substitute a number from a different instrument.**
- blocked-by: **none — DISPATCHABLE NOW** (⚠️ **PLUGIN module — genuinely parallel-safe against the game-module lane and against a game-module compile gate**)
- parallel-safe: yes; **EXCLUSIVE owner of `Plugins/SiegeLlama/Source/SiegeLlama/{Public,Private}/SiegeLlamaSubsystem.{h,cpp}`**
- spec: >
    ⚠️ **WHY THIS TASK EXISTS — AND THE CAUSE IS AN ORCHESTRATION ERROR, NOT A PROGRAMMER ERROR. RECORDED AS SUCH.** TASK-423 was dispatched
    against its **headline** (the Zone-A equality test) with the rest of its board spec omitted. ✅ **The programmer delivered the test correctly
    AND DECLARED THE SHORTFALL rather than letting a partial pass as `done`** — which is the behaviour this pipeline wants and is the only reason
    this was caught before TASK-447's gate instead of inside it. ⇒ **`USiegeLlamaSubsystem` IS NOT WRITTEN. This task writes it.**
    **⛔ TASK-423's ENTIRE ORIGINAL SPEC IS YOURS — read it on the board (items 1–9) and treat it as this task's spec, with the four
    manager amendments (A)–(D) applied.** Short form of what is owed and absent today: the `UGameInstanceSubsystem` that survives level travel ·
    **ONE dedicated `FRunnable` at `TPri_BelowNormal`** (`llama_context` is not thread-safe; a long-lived runnable makes single-thread ownership
    **structural rather than a comment** — ⛔ not the task graph, not `Async()`, not a context per request) · **queue depth 1 enforced by
    returning false, never by a queue** (⛔ **do not add a request queue "for robustness" — that reintroduces exactly the out-of-order /
    stale-context bug class this design deletes**) · **cancellation via `abort_callback`** (⛔ a post-generation poll is far too late during a
    long prefill) · **three timeouts** (soft 4 s, hard 10 s, 96 output tokens) · **offload tiering + KV reuse from TASK-413's measured numbers** ·
    **SEH + the `bAssistantFaulted` latch + the completion delegate ALWAYS on the GAME THREAD** · and **spec item (9)'s budget work**
    (9a: `MaxUtteranceChars` bounds BOTH the `order:` and `pending:` lines, neither truncated by the budget; 9b: set `ZoneBCharReserve` from
    Zone B's **printed** worst case — ⚠️ **the header says "re-measure it, do not eyeball it"**).
    **⛔ THE PLUGIN STILL KNOWS NOTHING ABOUT SIEGEBOUND.** No game-lane `#include`, no Siegebound type in any signature. The entire API surface
    between the lanes is `(prompt, gbnf) → string`. **A Siegebound include here is a QA FAIL.** ⚠️ **And NO multi-turn loop — one single-shot
    call, zero conversation state** (§1).
    **⚠️ THE TWO DEAD SPELLINGS — the vendored header wins, and both are RENAME-ONLY with the requirement unchanged:** it is
    **`llama_memory_seq_rm`** (`llama.h:735`), **NOT** `llama_kv_cache_seq_rm` — that symbol **does not exist anywhere in the vendored header**;
    and **`load_mode = LLAMA_LOAD_MODE_MMAP`**, **NOT** `use_mmap = true` — that member no longer exists. ⚠️ **Memory-mapped loading is
    load-bearing for the 8 GB claim** (weights are page-cache-backed, not resident).
    **✅ NOW DELETE THE SPIKE — AND ONLY NOW.** TASK-423 **correctly DEFERRED and DECLARED** the deletion under Amendment A on a decisive
    argument: **with no subsystem, deleting the spike leaves ZERO model-load paths rather than one.** ⇒ **Once your subsystem exists, delete
    `SiegeLlamaSpike.cpp` + its corpus file** so two load paths never coexist. ⛔ **Before you delete it, take any final `zoneA_tok` reading you
    need — it is the ONLY command that prints one** (Amendment A: *do not delete the measuring instrument and then quote a number from memory*).
    **M8 DECLARATION DUTY: state verbatim in the handoff — "adds no replicated property, no new replicated class, no new relevancy tier."**
    Handoff `handoffs/TASK-450-programmer.md`: the tiering policy **with the numbers it came from**, the abort path traced end to end, and the
    thread-ownership argument. Post in ⚙️ Dev & QA.
- names: >
    `Plugins/SiegeLlama/Source/SiegeLlama/{Public,Private}/SiegeLlamaSubsystem.{h,cpp}` · `USiegeLlamaSubsystem` ·
    `FSiegeLlamaCompletionSignature` · `IsReady` / `IsBusy` / `RequestCompletion` / `CancelActiveRequest` (**§9 registry,
    character-for-character** — ⚠️ TASK-443 compiles against these) · `LogSiegeLlama` · `llama_memory_seq_rm` · `LLAMA_LOAD_MODE_MMAP` ·
    `SoftTimeoutSeconds` 4.0 · `HardTimeoutSeconds` 10.0 · `MaxOutputTokens` 96 · `MaxSnapshotTokens` 400 · `SnapshotPreFilterMaxChars` 3000 ·
    `ContextTokens` 2048 (⛔ **FROZEN**). **DELETES** `SiegeLlamaSpike.cpp` + `SpikeCorpus.inl`.
    Law: CONVENTIONS "In-match LLM command assistant" §1, §5, §8, §9, §10 · §12c (the `ContextTokens` freeze) · §12g.

#### TASK-451 — [TEST-FIX] ⛔ `TestEqual` IS CASE-INSENSITIVE ON `FString` — the byte/casing assertion sweep (gameplay-programmer)
- assignee: gameplay-programmer
- status: **done — COMMITTED 2026-08-03 in `cd5f4ed`** (Wave 1 code. Build GREEN: both DLLs linked, **zero errors, zero warnings** — the link step ran for the first time, so the 22-seam pre-flight became a measurement. ⚠️ **Green means it BUILDS, not that it works**: no automation test has ever run (33 compile; compiling is not passing), WARN-5 stays instrumented-not-discharged, and every playable check belongs to TASK-448. Flip applied by the ORCHESTRATOR after manager handed the board back.) ← prior: ✅ **qa-passed → ready-for-integration** (2026-08-03 — `qa/TASK-446.md`, PASS · 0 blockers). ⚠️ **WARN-1 CORRECTION: the `WITH_CASE_PRESERVING_NAME` residual is 18 sites, NOT the 12 stated** — the report's own enumeration disagreed with its headline figure and QA confirmed a sample are genuine `FName::ToString()` comparisons. ⛔ **FIFTH wrong count this wave; my own §22 covers it — a count is a hint, never a contract.**
- status: **ready-for-qa** (2026-08-03 — handoff `handoffs/TASK-451-programmer.md`; reviews under TASK-446. ⛔ **NOT compiled, NOT run — TASK-447 is the one gate, and nothing in this repo has been built this batch, so no test in these files has EVER executed.** **MECHANISM RE-DERIVED FROM ENGINE SOURCE, not from the spec:** `TestEqual(const TCHAR*, const FString&, const FString&)` forwards to the `TCHAR*` overload (`AutomationTest.h:1997-2000`), which compares with **`FCString::Stricmp`** (`AutomationTest.cpp:2163`); `TestEqualSensitive` uses **`FCString::Strcmp`** (`:2295`). ✅ Board's mechanism CONFIRMED for `FString`. **58 of 113 sites converted** — ⚠️ **the board's "117" is wrong: the real total is 113**, the 4-site gap being four prose mentions of the identifiers inside a ZoneA comment block (`:353,354,356,357`) counted as call sites. Grammar 26/53 · Guard 29/33 · Settings 3/14 · **ZoneA 0/13 — ALREADY FULLY COMPLIANT, untouched** (all 5 of its string claims were already `*Sensitive`, with the mechanism independently derived at `:353-362`). 50 integer/`.Num()` sites deliberately NOT churned. **No line added or removed in any converted file — every line number cited by the live TASK-441/423/436 reviews still resolves.** ⛔ **GUARD #2 DISCHARGED — one conversion PROVEN to bite:** at `SiegeAssistantGrammarTest.cpp:464` (*"Symbol casing does not change the emitted grammar"*), deleting the `.ToLower()` at `SiegeAssistantGrammar.cpp:355` — **the exact regression that test exists to catch** — makes `First` and `FromMixedCase` differ ONLY in case (`"\"footman\""` vs `"\"Footman\""`), so **`TestEqual` PASSES (reports SAFE) while `TestEqualSensitive` FAILS.** On the current correct code both pass, so the sweep does not turn TASK-447 red by itself. 🚩 **TWO FINDINGS RAISED, NOT SILENTLY PATCHED:** (1) **CONVENTIONS §13 needs an amendment — its bullet "the defect is confined to `FString` overloads" is NOT accurate**: `FStringView`, `FUtf8StringView`, **`FText`** (`EqualToCaseIgnored`) and **`FName`** (`IsEqual` defaults to `ENameCase::IgnoreCase`, `NameTypes.h:806`) are all case-insensitive too, **and `TestEqualSensitive` has NO `FText`/`FName` overload** — such a claim must go through `.ToString()`. No site in scope was affected; manager owes the edit. (2) ⚠️ **WARN-436-2 was NOT one line and is flagged for a ruling** — see below. Also: `SiegeSettingsTest.cpp:132`'s `TestNotEqual` was **left insensitive ON PURPOSE** — the slot name is a save FILENAME and Windows filenames are case-insensitive, so two names differing only in case are the SAME FILE; the insensitive comparator is the stronger one there. **Scope amendment items both done:** **WARN-437-1** — `Initialize();` added as the first statement of `USettingsMenuWidget::RebuildWidget()`, one line, all four of QA's engine claims re-derived independently and all hold (`UserWidget.cpp:159-162`, `:1197-1200`, `UserWidget.h:297`, `UserWidget.cpp:135-137`); **WARN-436-2** — fixed, ⚠️ **but it took +1 statement, +1 pre-condition assertion and 1 moved statement, NOT one line, because a one-line fix is mechanically impossible**: the only API that moves the reader off `true` is `SetAssistantConfirmEnabled`, which also persists to the same scratch slot (`SiegeSettingsSubsystem.cpp:47`), so the fix must reorder rather than insert. ⚠️ **QA: the one residual risk is the 12 converted `FName::ToString()` sites, which rely on `WITH_CASE_PRESERVING_NAME` (= `WITH_EDITORONLY_DATA` = 1 in these `EditorContext` tests) — contained, but unprovable without executing.** No compile · no Git · no editor · no MCP · no PIE.)
- blocked-by: **none — DISPATCHABLE NOW.** ⚠️ **Must land BEFORE TASK-447**, which runs these tests. Sequential re-ownership of four finished tasks' test files (417 · 423 · 436 · 441 — all delivered, handoffs written); ⛔ **never concurrent with TASK-443 if it adds tests.**
- parallel-safe: yes to WRITE; ⛔ never beside a compile gate
- spec: >
    ⛔ **A SHIPPED, QA-PASSED TEST IS ASSERTING NOTHING, AND IT IS THIS REPO'S OWN NAMED FAILURE CLASS: AN AUTOMATED GUARDRAIL THAT REPORTS
    SAFE.** Same family as `Build.bat` returning exit 0 on a failed build, `rig_character.py` silently ignoring an override that did not exist,
    and MCP readback passing on visually-broken UMG — **three confident greens that were not real.** ⚠️ **This one is worse in one specific way:
    it looks automated, and it passed a human QA gate.**
    **VERIFIED AT THE ARTIFACT (manager, raw `Read`, not a relay) — `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeAssistantGrammarTest.cpp`:**
    · **`:464` `TestEqual(TEXT("Symbol casing does not change the emitted grammar"), FromMixedCase, First)`** — ⛔ **a CASING claim asserted with
    a CASE-INSENSITIVE comparator. It cannot fail for the property it names. It is vacuous** — and the comment directly above it
    (`:456-459`) spells out that the guarantee under test is precisely the lower-casing of every symbol.
    · **`:454` `TestEqual(TEXT("Two builds from equal inputs are byte-identical"), Second, First)`** — a **BYTE-IDENTITY** claim with the same
    comparator.
    **(1) TRIAGE EVERY SITE.** ⛔ **COUNT CORRECTED 2026-08-03 — MY 117 WAS WRONG; THE TRUE DENOMINATOR IS 113.** Four of my "sites" were
    **prose mentions of the identifiers inside a ZoneA comment block, not call sites** — my figure came from a `Grep` count, and ⚖️ **`Grep`
    LOCATES; IT DOES NOT READ** (§10's trap, firing on me this time rather than on a reviewer). ⚠️ **A completeness claim is only auditable
    against a TRUE denominator, so an inflated one silently understates coverage.** True sizes: `SiegeAssistantGrammarTest.cpp` **53** ·
    `SiegeAssistantGuardTest.cpp` **33** · `SiegeSettingsTest.cpp` **14** · `SiegeAssistantZoneATest.cpp` **13**.
    ✅ **Integer / bool / `.Num()` comparisons are UNAFFECTED — do not churn them.** **Convert the string-typed ones whose claim is byte-,
    casing- or identity-shaped to `TestEqualSensitive`.** ⚠️ **The type list is WIDER than `FString`** — see CONVENTIONS §13's correction:
    `const TCHAR*`, `FString`, `FStringView`, `FUtf8StringView`, `FText` and `FName` are **all** case-insensitive under `TestEqual`, and
    ⛔ **`TestEqualSensitive` has NO `FText` and NO `FName` overload** (convert explicitly at the site and assert on the conversion).
    **(2) ⚠️ CONFIRM THE MECHANISM AGAINST THE ENGINE HEADER AND CITE `file:line` — DO NOT TAKE IT FROM THIS SPEC.** The claim is *corroborated*
    by the existence of `TestEqualSensitive` (you do not add a "Sensitive" variant unless the base is insensitive) and by TASK-423's programmer
    catching it in their own draft — **but the manager has no engine source to read and did NOT re-derive it.** ⇒ **You are the first party who
    can. Name the header and line in the handoff.** ⛔ **If the mechanism turns out to be different, STOP and report — do not convert 117 call
    sites on a wrong premise** (a wrong root cause ships a wrong fix AND a wrong lesson).
    **(3) ⚖️ PROVE AT LEAST ONE CONVERSION ACTUALLY BITES.** For `:464`, show that the **pre-fix** test passes on an input it should reject —
    i.e. that the vacuity was real — and that the **post-fix** test fails on it. ⛔ **A sweep that changes 117 lines and demonstrates nothing is
    the same defect one level up.** *(The TASK-417 idiom: the guard must be **exercised**, not merely present.)*
    **(4) ⛔ DO NOT WEAKEN OR DELETE A TEST TO MAKE IT PASS.** If a conversion reveals a REAL defect in shipped code, **STOP, report it, and let
    it route** — that is a finding, not an obstacle, and it is the entire point of doing this.
    ⛔ **No production-code changes, no `Content/`, no Git, no compile.**
    Handoff `handoffs/TASK-451-programmer.md` with the **full triage table** (site → claim shape → converted / left, with the reason).
    Post in ⚙️ Dev & QA.
- ⚠️ **TWO TASK-439 CARRIES FOLDED IN 2026-08-03 — both non-blocking, both ONE LINE, and neither justifies its own dispatch. Recorded as a deliberate judgment call against the one-task-one-deliverable rule, not an oversight.** This task additionally owns **`SettingsMenuWidget.cpp`** for carry (a) ONLY.
    **(a) `ConstructSettingsTree()` RUNS BEFORE SUPER'S DEFENSIVE `Initialize()`** — ⚠️ **unreachable via `CreateWidget`, so this is hardening, not a bug**; take the one-line fix and ⛔ **change nothing else in that file** (it is `qa-passed`).
    **(b) ⛔ TASK-436's `true`-DIRECTION SAVE/LOAD ASSERTION CANNOT DISTINGUISH *LOADED TRUE* FROM *NEVER LOADED*** — and **that is squarely this task's charter**: a test that passes for the wrong reason is the same defect class as a comparator that cannot see casing. ⚖️ **The default is `true`, so the assertion is satisfied by the very state it is meant to rule out.** Fix by asserting the **`false` direction** (set false → save → reload → still false), which **only a real round-trip can produce.**
    - ✅ **RULED AND RATIFIED 2026-08-03 — THE ONE-LINE CEILING IS WAIVED FOR THIS ITEM, AND THE WAIVER IS THE ORCHESTRATOR'S, NOT MINE (recorded here because the board is where rulings live).** The *"stop and report if it exceeds one line"* constraint was the orchestrator's; **TASK-451 flagged rather than quietly absorbing the overrun, which is exactly the behaviour that was asked for** and is the reason this was decided rather than discovered. ⚖️ **The mechanical reason is sound and was verified, not accepted: the only API that moves the reader off `true` (`SetAssistantConfirmEnabled`) PERSISTS TO THE SAME SCRATCH SLOT** (`SiegeSettingsSubsystem.cpp:47`), **so perturbing in place would clobber the value under test — the fix HAS to reorder.** Cost: **+1 statement, +1 pre-condition, 1 moved statement, all inside `FSiegeSettingsSaveLoadRoundTripTest`.** ⇒ ***"One line" meant NO SCOPE CREEP, not "abandon a correctness fix"*** — and a ceiling that would force a wrong test to ship is a ceiling misapplied. ⛔ **TASK-446 VERIFIES CONTAINMENT rather than taking anyone's word for it: the change must be confined to that one test body.**
- names: >
    `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeAssistantGrammarTest.cpp` · `SiegeAssistantGuardTest.cpp` ·
    `SiegeAssistantZoneATest.cpp` · `SiegeSettingsTest.cpp` · `TestEqualSensitive` (UE 5.8 `AutomationTest.h`) ·
    **`SettingsMenuWidget.cpp`** (carry (a) ONLY).
    Law: CONVENTIONS "Settings screen…" §13 (the comparator law) · §9c (**where an artifact has a parser, the parser is the reviewer of record**).

