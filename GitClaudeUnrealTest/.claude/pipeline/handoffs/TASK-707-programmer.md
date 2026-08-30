# TASK-707 — [HELP-4] THE FULL-SCREEN DETAIL VIEW — programmer handoff

**Agent:** gameplay-programmer · **Date:** 2026-08-30 · **Law:** `HELP-§1`/`§2`/`§3`/`§4`/`§5`/`§6` (cited, ⛔ not restated) · `AS-§6 A-2` · `KBD-§0`/`§4`/`§5`/`§8` · `SC-§15`/`§32`/`§33`
**Inputs, both binding and both adopted whole:** `handoffs/TASK-704-programmer.md` §4 (the 24 detail texts + their citations) · `handoffs/TASK-706-programmer.md` §4 (the seam contract I fill)
**Consumers:** TASK-708 (QA gates 704+706+707 together) · TASK-709 (the wave's one compile + the suite)

**Jonathan's ask, verbatim — the acceptance bar:** clicking a row *"will show a much more detailed description on the entire screen of how it works and all the controls with it such as what the first, second, and third circles do, how to resize them, how to exit the command."*

---

## 0. WHAT LANDED, AND WHAT DELIBERATELY DID NOT

| | |
|---|---|
| ✅ **Files edited (3, and only 3)** | `Source/GitClaudeUnrealTest/Siegebound/SiegeControlsHelpWidget.h` · `.cpp` · `Tests/SiegeControlsHelpTest.cpp` |
| ✅ **Pipeline files** | this handoff + this task's own status line on `TASKBOARD.md` |
| ⛔ **NOT touched** | `SiegePlayerController.{h,cpp}` — **zero controller edits were needed**, see §1.4 · no second test file (`HELP-§6`'s "⛔ one file") · no asset |
| ⛔ **NOT done** | no compile (709 owns the wave's one) · no editor · no MCP · no Git · no console sentence (**the 552 latch is UNSPENT**) · no board edit beyond this task's status line |
| ⛔ **NOT re-decided** | 706's **D-1** (the overlay composes with pick-mode rather than joining the LMB exclusion) and its **D-10** (the one `UE_DEPRECATED` field write) are **TASK-708's rulings**. I neither changed nor argued them. |

**One new class, in the same pinned pair (`HELP-§3`):** `USiegeControlsDetailWidget`, plus two **plain (⛔ non-USTRUCT) C++ structs** `FSiegeControlsDetailEntry` / `FSiegeControlsDetailContent` and three new pure registry statics.

---

## 1. THE SEAM — WHAT I FILLED, EXACTLY AS 706 §4 SPECIFIED IT

### 1.1 The two functions

| 706 §4 | Filled with |
|---|---|
| **(c) `ShowDetailForAction(FName)`** (`.cpp:2770`) | Validates the id against the registry **again** (defence in depth — the function is `virtual`+`protected`), refuses null switcher / null view **by leaving the LIST up**, resolves the layout subsystem null-safely, composes the whole page through `FSiegeControlsHelpRegistry::ComposeDetailContent`, **stamps then switches**. |
| **(c) `ReturnToList()`** (`.cpp:2835`) | `ViewSwitcher->SetActiveWidgetIndex(ListViewIndex)`, null-guarded. **Idempotent by construction** — `UWidgetSwitcher::SetActiveWidgetIndex` early-outs on an unchanged index (`WidgetSwitcher.cpp:49-57`) — which is what lets `CloseHelp()` **and** `OpenHelp()` both call it unconditionally. |

### 1.2 ⛔ `ComposeDetailForDisplay`, never `Row.Detail` raw (706 §4d)

`ComposeDetailContent` reads the prose **only** through `ComposeDetailForDisplay(Row)` (`.cpp:1338`) and each related block the same way (`.cpp:1379`). ⛔ **`Row.Detail` is read raw in exactly zero places outside `ComposeDetailForDisplay` itself.** Greppable: `grep -n "\.Detail" SiegeControlsHelpWidget.cpp` — the hits are the 24 assignments, the one read inside `ComposeDetailForDisplay`, and one test-side `Row.Detail.ToString().Contains("{")` census that is deliberately measuring the RAW field.

### 1.3 The other seam surfaces, honoured unchanged

- **(a)/(b)** `OnRowActivated` → `HandleRowActivated` — ⛔ untouched except that its stale "TASK-707 fills this" comment now says what actually happens.
- **(e)** ⛔ **No second cursor owner.** Sweep over the pair: `SetInputMode` / `bShowMouseCursor` / `bEnableClickEvents` = **5 hits, all comments, zero calls.**
- **(f)** ⛔ **`Escape` untouched.** Sweep: `NativeOnKeyDown` / `NativeOnPreviewKeyDown` / `FReply::Handled` = **4 hits, all comments.** `EKeys::Escape` = **2 hits, both registry DATA** (`Cards.Cancel`, `PickMode.Cancel`). The detail view overrides **no key handler at all** — its way back is a **button**, and §2.3 explains that this is the reason it is a button.
- **(g)** ⭐ See §3 — the detail lane's keys derive through the **same** `ResolveRowDisplayKeys`, in two places.
- **(h)** `/Game/UI/WBP_ControlsDetail` **RESERVED and unauthored**; a soft `DetailWidgetClass` slot exists so it can win with zero C++ change (706's D-3 precedent). `RebuildWidget()` order law followed in the new class (tree + `RootWidget` set, **then** `Super`).

### 1.4 ⭐ ZERO CONTROLLER EDITS WERE NEEDED, AND THAT IS 706'S DESIGN PAYING OFF

`CloseHelp()` already called `ReturnToList()` first. ⇒ pressing **TAB** from inside a detail page closes the whole overlay *and* leaves the next open on the list, through the shipped controller path, with **no new close route invented** (`HELP-§5`'s "that is the complete list" survives intact). `ApplyCursorInputState()`'s composition is **byte-identical** to what 706 shipped.

---

## 2. THE FULL-SCREEN LAYOUT — THE DECISIONS, ARGUED

### 2.1 ⭐ A `UWidgetSwitcher`, ⛔ not two visibility flags

`BackdropBorder` → `ViewSwitcher` → **[0] `PanelBorder`** (the list) · **[1] `DetailView`** (the page).

- `SWidgetSwitcher` renders **at most one child**, so *"the list is never behind the detail page"* is **structural**. Two `Collapsed`/`Visible` writes would be a pair of statements that can fall out of step, and the failure mode is two screens of text on top of each other.
- ⭐ **`UWidgetSwitcher` constructs itself `SelfHitTestInvisible`** (`WidgetSwitcher.cpp:18`) ⇒ 706's §4 hit-test law is **unchanged**; its children still decide.
- The child order **is** the contract, and the two indices are named constants beside a comment saying so (`.cpp:182-183`).

### 2.2 ⭐ HOW "THE ENTIRE SCREEN" IS ACTUALLY ACHIEVED — the margin moved DOWN a level

706 put the 140/60 inset on `BackdropBorder`. A padding there insets **both** views identically. ⇒ I set `BackdropBorder` padding to **zero** and moved the inset onto the **switcher's list slot** (`UWidgetSwitcherSlot::SetPadding`, verified public `UMG_API` at `WidgetSwitcherSlot.h:49`), while the **detail slot carries none**.

⇒ **the list is still exactly the plate 706 shipped, and the detail page reaches the screen edge.** Per-slot padding is precisely how one container gives two children different margins.
⛔ **The fallback is real:** if `ViewSwitcher` fails to construct, `BackdropBorder` takes the margin back and the list becomes its direct content — i.e. it degrades to **the shipped TASK-706 shape**, never to a blank overlay.

### 2.3 ⚠️ THE ONE HIT-TEST DIFFERENCE FROM THE LIST, DECLARED (deviation **D-3**)

The detail page's `DetailBackdrop` is **`Visible`**, where the overlay's `BackdropBorder` is `SelfHitTestInvisible`.

**The geometry is different, not the principle.** 706's backdrop is hit-test-transparent because it is a **margin around a small panel** and the live match must keep receiving clicks that land in it. This plate **is the page**, at the size Jonathan asked for — and *a plate you are reading must absorb its own clicks or a click on a sentence places a card in the world underneath it.* That is the **exact rule `PanelBorder` already follows**; this one is simply screen-sized.

⚠️ **The honest consequence, stated:** while a detail page is open there is no "outside the panel", so a world click cannot be made through it. **⛔ Not a soft-lock:** TAB closes the overlay, **Back** returns to the list, and `Escape`/RMB are raw `PlayerTick` polls (`SPC:591-596`, `:678-686`) that this feature never touches. See **U-2** for what I could not measure.

### 2.4 Layout, top to bottom

`DetailBackdrop` (Visible, near-opaque, fills) → `DetailColumn`
· `DetailHeaderBox` = **derived key chip | title (40pt)**
· `DetailSummaryText` — the row's one-liner, in the category-header amber
· `DetailScrollBox` (**fills, scrolls**) → `DetailBodyText` (19pt, wrapped) · `RelatedHeaderText` (**collapsed unless the page has related controls**) · `RelatedBox`
· `BackButton` → **"Back to the controls list"**

⚠️ **The body MUST scroll** and it does: 704's prose runs to several paragraphs on the order pages, and a page that silently clipped its last paragraph would be `HELP-§2`'s own failure mode wearing a layout bug's clothes. `ScrollToStart()` on every stamp, so a short page after a long one does not open pre-scrolled.
⚠️ **The Back label names a DESTINATION, ⛔ never a keystroke** — because the only key a "go back" affordance would naturally have offered is `Escape`.

### 2.5 ⭐ "ALL THE CONTROLS WITH IT" — `RelatedActionIds`, and why it is not duplicated prose

New field `FSiegeControlsHelpAction::RelatedActionIds`. A page renders each related row's **own** chip, name and **own detail text**.

⇒ **The Ambush page answers all three of Jonathan's named questions with ZERO duplicated sentences anywhere in the feature** — `PickMode.Confirm` (the three circles, in order), `PickMode.Resize` (the wheel), `PickMode.Cancel` (how to exit) are rendered from **the same strings those rows show on their own pages**. One definition, two renderings, nothing to drift (`HELP-§2`).

⛔ **ONE LEVEL DEEP, STRUCTURALLY:** `ComposeDetailContent` reads a related row's text and **never touches its `RelatedActionIds`**. The pick-mode rows genuinely cross-reference each other, so the data contains a real cycle — and the suite asserts both that the cycle exists and that the composer refuses it. Unknown, empty and self-referential ids are **dropped**, never rendered blank.

**The 13 rows that declare related controls (25 references, all verified to resolve):**

| Page | Related | 704's own basis |
|---|---|---|
| `Orders.Hold` · `Orders.Ambush` · `Orders.Follow` | `PickMode.Confirm` · `Resize` · `Cancel` | R-13 *"the three-circle pick described in R-16..R-18"*, R-14 *"same three-circle pick"*, R-15/R-16 *"For FOLLOW the flow ENDS HERE"* |
| `PickMode.Confirm`/`Resize`/`Cancel` | the other two | one gesture, three verbs |
| `Cards.Play` | `Cards.Cancel` · `Cards.CursorHold` | R-07 *"right-click or Escape cancels"* |
| `Cards.Discard` | `Cards.CursorHold` | R-09 — the button is on the HUD, unclickable without the cursor hold |
| `Interface.AssistantConsole` ⇄ `AssistantAccept` | each other | R-20: the confirm step has **no buttons at all** |
| `Interface.WarMap` | `WarMapReveal` · `WarMapMarker` | R-21 names both |
| `WarMapReveal` → `WarMap` · `WarMapMarker` → `WarMap` + `AssistantConsole` | | R-22/R-23 |

---

## 3. ⭐⭐ THE DETAIL LANE'S KEYS — `HELP-§1` APPLIED TWICE, AND **ZERO** NEW `GetPositionalKey` CALLS

**Sweep, after this task: `grep -n GetPositionalKey` → exactly the same 2 call sites 706 shipped** (`.cpp:1120` the Lane-C F-1 reversal branch, `.cpp:1156` the Lane-A fallback). ⇒ **TASK-707 added ZERO translation calls.** The detail lane has no second way to reach a key: every chip on a page — the headline, the related blocks, and the keys named *inside the prose* — goes through `ResolveRowDisplayKeys` with its lane audit.

### 3.1 ⛔ THE DEFECT I CAUGHT IN 704's OWN TEXT, AND HOW I FIXED IT WITHOUT PARAPHRASING

⚠️ **704 §4 types the letters `T` and `E` in prose, in four sentences** (R-13, R-14, R-15, R-18 — *"Pressing `T` or `E` destroys this group"*, *"Unlike `T`/`E`, Follow does not release…"*, and the R-18 teardown list). **`T` and `E` are exactly the letters US-Dvorak moves** (to `Y` and `.`, 704 §1.4's own table). Transferring them literally would have printed the **wrong keys mid-paragraph on the very screen whose chips exist to print the right ones** — and it would have looked perfect on this machine.

**⛔ I did not paraphrase it away and ⛔ I did not silently "improve" it.** The sentences are kept **word for word**; the two letters became **`{ActionId}` tokens**, spliced with those rows' own derived chips at compose time by `FSiegeControlsHelpRegistry::ResolveDetailTokens`. On QWERTY the page reads **character-for-character as 704 wrote it**; on Dvorak it reads correctly.

| Property | How |
|---|---|
| Token shape | ONE definition, `MakeActionToken(FName)` — the writer and the resolver cannot disagree |
| Matching | **CASE-SENSITIVE and only against real registry ids** ⇒ an ordinary brace in future prose is untouched, and ⛔ a MISSPELLED token **survives visibly** rather than deleting a sentence's subject (`HELP-§2` mech. 2) |
| Cost | a `Contains("{")` gate first: 19 of the 24 pages pay **nothing** |
| Coverage | **5 token sites** across 4 rows + the overlay's own key (see below) |

⭐ **The overlay's own key is tokenised too** (`Interface.ControlsHelp`, R-24) — `HELP-§4`, *the menu documents its own key*, extended from 706's hint line to the detail prose. Typing "Tab" there would have been the one hardcoded key on a screen built to have none.

⛔ **The assistant's `Z` is deliberately NOT tokenised.** `KBD-§8`/`KBD-§0` ruling 1 pin every human-facing accept-key string to the literal `Z`, two of R-20's mentions are **quotations** (of Jonathan's ruling and of the console's own live status line), and 704 §8 **F-1** is a flagged Jonathan decision, not mine to move. ⭐ **The reversal is still exactly one flag**: `bLiteralKeyLabel = false` derives the chip, and this prose would then move to `{Interface.AssistantAccept}` tokens — a `KBD-§8` amendment, ⛔ not a help edit.

### 3.2 Keys left as literals, with the proof

`Escape` · right-click · left-click · the mouse wheel · `Enter` · the digit `1` (R-07's legacy-slot sentence). **`KBD-§4` tables the 26 LETTERS and nothing else** (`SiegeKeyboardLayoutStatics.cpp:57-63`) ⇒ none of these can ever be retargeted by this system. This is 706's stated rule, applied unchanged.
⚠️ R-07's `1` is specifically **not** tokenised: `{Cards.Play}`'s chip is all six keys, and the sentence is about the first one.

---

## 4. ⭐ THE 24-ROW TRANSFER PROOF

**Rule:** every detail string is 704 §4's prose for that row. ⛔ Nothing re-authored, ⛔ nothing invented. Five mechanical rules were applied and **every application is enumerated below** so QA can check any row in one reading. The rules are also pinned in the code, above `GetActions()` (`.cpp:158-196`).

| # | Rule | Why it is transfer and not paraphrase |
|---|---|---|
| **T1** | `file:line` citations and handoff cross-references (`(SPC:2851-2882)`, `(R-16..R-18)`) move **out of the prose and into the C++ comment above the string** | **TASK-706 §4(e) assigned this route to this task**, and it is `HELP-§2`'s own instruction for an unavoidable literal. ⭐ **Every citation is preserved, in the comment.** |
| **T2** | markdown markup (`**`, `*`, backticks) dropped | It is markup for a `.md` file. A backtick rendered on a `UTextBlock` is a stray character, ⛔ not the author's sentence. |
| **T3** | pipeline marker emoji (⭐ ⚠️ ⛔ ✅ ⚖️) dropped | They annotate the HANDOFF's reader. ⚠️ `⛔` in particular reads to a **player** as *"you may not do this"* — actively misleading on a sentence like *"⛔ The leash is what makes this HOLD"*. |
| **T4** | a **movable** key named in prose becomes a `{ActionId}` token | §3.1. `HELP-§1`. |
| **T5** | sentences addressed to an **implementer** (provenance of a ruling, a raw C++ fragment, a defect post-mortem) move into the comment; any connective left dangling gets the **minimum** grammatical repair | These are notes to the next person who reads the code, ⛔ not facts a player can act on. **Every instance is listed below.** |

**⛔ NO TUNABLE'S VALUE IS RE-TYPED** (704 U-5 / D-6, the M7.7 `"in 400"`/`AoERadius 700` lesson). The **one** number anywhere in the registry is the war map's **30 gold**, because Jonathan's own words are the source and 704 quoted them at the property (`CommanderNpc.h:297-311`).

### 4.1 The table — 24 rows

| Row | 704 §4 | Transferred | T1 citations carried into the comment | T4 tokens | T5 removals (enumerated) |
|---|---|---|---|---|---|
| R-01 `Hero.Move` | ✅ | ✅ | `TASK-568-artist.md:23-49` · `SiegeKeyboardLayoutStatics.cpp:227-233` · `HeroCharacter.h:406,343` · `SiegeKeyboardLayoutStatics.h:53` | — | — |
| R-02 `Hero.Look` | ✅ | ✅ | `TASK-568:28` · `TASK-399:137` · `SPC:768-770,783-790` · `SPC:4361-4370` | — | — |
| R-03 `Hero.Jump` | ✅ | ✅ | `GitClaudeUnrealTestCharacter.cpp:59-60` · `HeroCharacter.h:138-151` | — | — |
| R-04 `Hero.Sprint` | ✅ | ✅ | `HeroCharacter.cpp:292-294,323-338,340-360` · `.h:410,406,346,343` | — | — |
| R-05 `Hero.Attack` | ✅ | ✅ | `HeroCharacter.h:437,441,445,234,162-167` · `.cpp:354-360,464,581-585,342-346` · `SPC:2723-2728,3144-3148` | — | — |
| R-06 `Hero.Rally` | ✅ | ✅ | `HeroCharacter.cpp:532-559,535-536,525-529,565,568-572,508-511` | — | — |
| R-07 `Cards.Play` | ✅ | ✅ | `SPC:482-489,858-868,895-911,900-903,682-698,913-919,914-916,920,879-889,812-836,732-748` | — | — |
| R-08 `Cards.CursorHold` | ✅ | ✅ | `SPC:756-771,494-499,783-790,775-780,4357` | — | — |
| R-09 `Cards.Discard` | ✅ | ✅ | `CardHandWidget.h:49,103-107` · `SPC.h:1007` · `SPC:1035-1044,1060-1062` | — | — |
| R-10 `Cards.Cancel` | ✅ | ✅ | `SPC:1065-1090,501-506,678-686,900-903,917-919` · `TASK-568:32` | — | ⚠️ **1:** *"Jonathan closed AS-§6 A-2 permanently: nothing may absorb it. The controls overlay itself does not, and must not, claim it."* → an instruction to **this overlay's implementer**; honoured in code (zero key handlers) and recorded in the comment. The player-facing fact *"Escape is a shipped, bound cancel key"* is **kept**. |
| R-11 `Orders.Attack` | ✅ | ✅ | `SPC:1153-1162,1122-1151,1130-1134,1144-1147,1106-1114` · `UnitCommand.h:19-21,39-43` | — | — |
| R-12 `Orders.Defend` | ✅ | ✅ | `SPC:1196-1199,1122-1151` · `UnitCommand.h:21-22,23-31,28-31` | — | ⚠️ **2:** *"and the header says so on purpose"* and *"Reading it as a centre radius is the defect that made Defend acquire nobody at the 9× castle"* → both are notes to whoever next reads `DefendRadius`. The **behaviour** sentence (band past the wall face, radius derived per decision) is kept in full. |
| R-13 `Orders.Hold` | ✅ | ✅ | `SPC:1165-1170` · `SummonedUnit.cpp:1757-1767,1808-1864,1778-1794,1770-1774,1766-1767,1762-1766,1795-1805,1851-1856` | ⭐ `{Orders.Attack}` `{Orders.Defend}` | **1 (T1):** *"described in R-16..R-18"* → *"described below"* — on screen those three rows **are** rendered underneath by `RelatedActionIds`, so the reference is the page itself. |
| R-14 `Orders.Ambush` | ✅ | ✅ | `SPC:1172-1177` · `UnitCommand.h:56-62` · `SummonedUnit.cpp:1778,1788-1792,1808-1810,1770-1774,1795-1805` · `MinerUnit.h:26,45,200` | ⭐ `{Orders.Attack}` `{Orders.Defend}` | **1 + repair:** the raw C++ condition `if (CurrentTarget && Group.Type == ESiegeGroupCommandType::Hold)` → cited in the comment; *"skips that whole block"* → *"skips the zone drop-test entirely"* (the antecedent no longer exists on screen). Also **2:** *"Jonathan's own words, recorded in the header"* provenance around the miner quote — the **quote itself is kept verbatim**. |
| R-15 `Orders.Follow` | ✅ | ✅ | `SPC:1179-1193,2893-2901,2835-2847,2744-2750,3325-3329,1186-1189,3293-3302` · `UnitCommand.h:66-70,71-73` · `SummonedUnit.cpp:1912-1921,1937-1940,1886-1898,1900-1910,1923-1935,1286-1291,1277,1290-1291,1304-1311,1293-1295` · `MinerUnit.h:496-502` | ⭐ `{Orders.Attack}` ×2 · `{Orders.Defend}` | **1:** *"the core-loop fact this help screen exists to teach"* → a statement about this screen's purpose; honoured by the sentence being on the page at all. ⭐ **THE SPAWN DEFAULT paragraph is kept in full** — it is the core-loop fact the board called out as mandatory. |
| R-16 `PickMode.Confirm` | ✅ | ✅ | `SPC:604-611,2717-2728,2851-2882,2884-2891,2857-2862,2903-2907,2988-2995,2893-2901,2919-2935,2926,2947-2955,2759-2782,2822-2831,3253-3265,2968-2978` · `SPC.h:1277,1281,1279,1285,1283` · `HeroCharacter.h:162-167` · `SummonedUnit.cpp:1830` · `UnitCommand.h:149-155` | — | **1:** *"pushed through a material parameter named StageTint"* → the place a reader checks the claim, ⛔ not something a player can use. ⚠️ **The three colours are kept exactly as 704 states them** — see U-1. |
| R-17 `PickMode.Resize` | ✅ | ✅ | `SPC:2785-2806,2808-2817,2787-2791,599,2710,2909,2929,2808-2810` · `SPC.h:1265,1269,1273,1258-1262` | — | — |
| R-18 `PickMode.Cancel` | ✅ | ✅ | `SPC:591-596,1082-1089,591,1082-1084,3135-3141,3137-3148,3155-3161,3163-3182,3184-3187,3189-3191,2676-2683` | ⭐ `{Orders.Attack}` `{Orders.Defend}` | **2:** *"This is Jonathan's 'how to exit the command'"* (pipeline provenance) and *"and `EndPlay`"* from the exit list (an engine lifecycle callback, not a thing a player does; the full list stays cited). |
| R-19 `Interface.AssistantConsole` | ✅ | ✅ | `SPC:4440-4463,4378-4395,4484-4501,4479-4482,4357-4375` · `SiegeAssistantConsoleWidget.h:147-156,111-126,99-109,138-143,144-146` · `.cpp:761-769,771-819,817-818,907-918,637-641,812-818` | — | — |
| R-20 `Interface.AssistantAccept` | ✅ | ✅ | `SiegeAssistantConsoleWidget.h:159-165,167-170,186-190` · `.cpp:71,1374-1377,845,868-878,875-876,854-866,952-969` · `SiegeKeyboardLayoutSubsystem.h:230-235,256-260` | ⛔ **deliberately none** — §3.1 | — (the `Z`s are the sanctioned `KBD-§8` exception, argued in-line) |
| R-21 `Interface.WarMap` | ✅ | ✅ | `SPC:4767-4779,4718-4742,4744-4765,4781-4791,4797,4786-4790,4820-4854,4857-4873,643-671` · `CommanderNpc.h:274-295` | — | — |
| R-22 `Interface.WarMapReveal` | ✅ | ✅ | `CommanderNpc.h:297-311,311,298-299,301-307,304-307` · `SPC:5103-5121,5108-5119,4982-4992,5123-5134,5136-5139` | — | — ⭐ **the one sanctioned number (30 gold) transferred with Jonathan's quote intact** |
| R-23 `Interface.WarMapMarker` | ✅ | ✅ | `SPC:4919-4930,4943-4951,4949-4957,4959-4965` | — | — |
| R-24 `Interface.ControlsHelp` | ✅ | ✅ | `HELP-§4`/`§5` · `SiegeAssistantConsoleWidget.cpp:907-931` · `SPC:4326-4376,4357,222-239` | ⭐ `{Interface.ControlsHelp}` ×2 | **1:** *"Status: the action asset and its Tab mapping both landed during this task"* — a pipeline status line (704 §2b). |

**Machine-verified, not eyeballed:**

| Check | Result |
|---|---|
| `Row.Detail` assignments | **24 / 24** |
| Registry ids | **24** |
| `{Token}` sites in shipped prose | **5 sites, 3 distinct ids — all resolve to real rows** (`Orders.Attack` ×5, `Orders.Defend` ×4, `Interface.ControlsHelp` ×2) |
| `RelatedActionIds` references | **25 — all 25 resolve; ⛔ zero self-references** |
| Developer-only fragments in player prose (`.cpp:` `.h:` `handoffs/` `TASK-` `SPC:` `**` `` ` `` `§`) | **0 in any `Detail` string** (the only 2 hits in the whole file are pre-existing TASK-706 `UE_LOG` strings) — **and this is now an automation test, not a one-off scan** (test 9) |

---

## 5. SUITE DELTA — **151 → 156 (+5)**

Baseline **151** re-counted from `IMPLEMENT_*_AUTOMATION_TEST` across `Siegebound/Tests/*.cpp` (706's declared total, confirmed: 156 now − 5 added). **All 5 added to the ONE existing file** (`HELP-§6`), which now holds 13.

| # | Test | What it actually proves |
|---|---|---|
| 9 | `Siegebound.ControlsHelp.EveryRowHasAuthoredDetail` | ⭐ **The test that would have been RED before this task.** All 24 rows are genuinely authored (⛔ not the TODO fallback), detail ≠ the one-liner and is longer, **every related id resolves and no row lists itself** — and ⭐ **it machine-checks transfer rules T1/T2**: ⛔ no `file:line`, no `handoffs/`, no `TASK-`, no markdown may reach the player's screen. |
| 10 | ⭐⭐ `…DetailKeysDeriveThroughTheAccessor` | **The keystone.** The Ambush page's **prose** differs between the two simulated layouts and names **the accessor's own answer** on each — ⛔ never a typed letter, with a fixture self-check first so "it changed" is not vacuous. Plus: ⛔ **no unresolved token survives on ANY page or related block on EITHER layout**; ⭐ at least one page really carries a token (so the previous claim is not vacuous either); a token-free page **HOLDS**; the resolver replaces a real token exactly once, **leaves an unknown one visible**, and asks the provider **nothing** for token-free prose. |
| 11 | ⭐ `…DetailContentAnswersTheNamedQuestions` | **Jonathan's sentence, as far as a machine can carry it.** From `Orders.Hold`/`Ambush`/`Follow`, all three of `PickMode.Confirm`/`Resize`/`Cancel` are present with a **derived chip, a name and body prose** — and ⭐ each block is **the same text that row's own page shows** (one definition, two renderings). ⛔ **No recursion**: the related count equals the declared count, with a self-check proving the data really does contain a cycle. Unknown/self/empty ids dropped. |
| 12 | ⭐ `…DetailBackSeamReturnsToTheList` | The way OUT. ⭐⭐ Pins the **deliberate asymmetry**: an unstamped ROW reports nothing, but an **unstamped PAGE still fires Back** — because `Escape` is not available as a second way out and a page with no exit is the one defect a help screen must not have. Also: the stamp survives a widget with **no tree at all**, and unbinding is clean. |
| 13 | `…DetailComposerIsFailSafe` | Every degraded input a shipped build can hand it: ⛔ **no layout subsystem at all** (all 24 pages still compose with title/chip/summary/body), the **applied key outranks the registry** on a page just as on a row, a pointer-only row still renders a chip, and an **undocumented row yields the pinned TODO string — ⛔ never a blank screen** (whitespace-only too). |

⇒ **TASK-709 should expect 156.**

**Also edited (⛔ no macro change):** test 6 `EscapeIsNotClaimed`'s class array gained `USiegeControlsDetailWidget` — extended rather than duplicated, so there stays **exactly one** place that answers *"does this feature claim Escape?"*. Test 5's comment now records that its "never empty rather than is-the-TODO-string" wording **held unchanged across 707**, which is why it was worded that way.

---

## 6. M8 DECLARATION (verbatim)

**Adds no replicated property, no new replicated class, no new relevancy tier, no RPC.** The detail view is **client-local display only**: it reads the registry, the local player's Enhanced Input mappings and the local keyboard layout, and writes only to widgets. **⛔ Read-only on the world (`HELP-§5`)** — no order issued, no group cancelled, no card played, no gold moved, **no pause**. Does NOT consume the M8 Phase-1 checkpoint gate; does NOT substitute for Jonathan's owed feedback items.

**Airlock:** `Capture()`/`EnsureSnapshot()` = **0**. Zone A and the **552 latch untouched**; no token figure (`AS-§12g`).
**`SC-§33`:** every function added takes **zero defaulted parameters** — structurally immune. The two engine calls with trailing defaults I rely on are passed **explicitly**: `FKey::GetDisplayName(false)` (706's, unchanged) and `FString::Contains`/`ReplaceInline(..., ESearchCase::CaseSensitive)` — the engine's default there is `IgnoreCase`, and an `ActionId` is a case-sensitive identity.

---

## 7. DEVIATIONS (`SC-§15`) — DECLARED, NOT SILENT

| # | Deviation | Reason |
|---|---|---|
| **D-1** | ⭐⭐ **Four sentences of 704 §4 prose carry `{ActionId}` tokens instead of the letters `T`/`E` it typed.** | §3.1. Those are the two letters US-Dvorak moves. The sentences are otherwise **word for word**; on QWERTY the page is character-identical to 704. ⛔ The alternative was shipping the exact defect `HELP-§1` exists to prevent, invisibly. |
| **D-2** | **704 §4's `file:line` citations and its handoff-internal cross-references are carried as C++ comments beside each string, ⛔ not in player prose (T1).** | **TASK-706 §4(e) explicitly assigned this decision to this task**, and `HELP-§2` names the comment as the home for an unavoidable literal. ⭐ Every citation is preserved and is listed in §4.1. |
| **D-3** | **The detail page's plate is `Visible` where the overlay's backdrop is `SelfHitTestInvisible`.** | §2.3, argued from geometry. It is the rule `PanelBorder` already follows, at screen size. 🚩 Flagged; see **U-2**. |
| **D-4** | **`BackdropBorder`'s 140/60 padding moved to the switcher's LIST slot; the backdrop is now zero.** | §2.2 — this is the only way one container gives two children different margins, and it is what makes "the entire screen" true. The list's rendered inset is **unchanged**. The fallback restores the old shape if the switcher is missing. |
| **D-5** | **A new field `RelatedActionIds` on `FSiegeControlsHelpAction` (13 rows populated), which 704 §3's data shape did not have.** | §2.5. Jonathan's *"and all the controls with it"* is a first-class part of his sentence and there was no field for it. ⛔ The alternative — copying the three-circle prose onto three order pages — is the drift `HELP-§2` exists to prevent. |
| **D-6** | **`FSiegeControlsDetailEntry`/`FSiegeControlsDetailContent` are PLAIN C++ structs, ⛔ not USTRUCTs**, unlike `FSiegeControlsHelpAction`. | They are never a UPROPERTY, never a Blueprint parameter (`HELP-§3` bans struct BIE params outright), and keeping them out of reflection keeps test 6's FKey-property sweep meaningful. `FSiegeControlsHelpAction` is a USTRUCT because **704 §3 specified it**; these are mine. |
| **D-7** | **A `UWidgetSwitcher` was added to TASK-706's tree** (its two shipped Borders keep their exact roles and visibilities). | §2.1. ⛔ It changes no hit-test behaviour: `UWidgetSwitcher` is `SelfHitTestInvisible` by construction (`WidgetSwitcher.cpp:18`). |
| **D-8** | **`OpenHelp()` gained one line — `ReturnToList()`.** | Belt and braces: `CloseHelp()` already returned first, but *"every open starts on the list"* should be a guarantee, not a consequence of which close route ran. Idempotent and free. |
| **D-9** | **`USiegeControlsDetailWidget::RequestBack()` is NOT stamp-fenced**, though `USiegeControlsHelpRowWidget::ActivateRow()` is. | The two rules point opposite ways on purpose and test 12 pins both. A row must not report a click before it knows its identity; a **page must never refuse to let the player leave**, because `Escape` is not available as a second way out. |
| **D-10** | **`ListPanelMargin` is a function (`MakeListPanelMargin()`), ⛔ not a namespace-scope `const FMargin`.** | `FMargin`'s four-argument constructor is not `constexpr` (`Margin.h:83`), so an object would add a dynamic static initializer. The file already avoids that shape deliberately (the `SiegeKeyboardLayoutStatics.cpp:22-30` precedent). |

**⚠️ 706's open questions, carried NOT re-decided:** its **D-1** (compose-with-pick-mode rather than join the LMB exclusion) and its **D-10** (the pragma-wrapped `UButton::IsFocusable` write) are **TASK-708's rulings**. I neither changed the code nor argued the merits. ⓘ I did reuse `ApplyButtonNotFocusable` for the new Back button — so **if 708 reverts D-10, the revert is now three call sites, not two.**

---

## 8. ⛔ DECLARED — WHAT I COULD **NOT** VERIFY (`SC-§32`)

| # | Claim | Status |
|---|---|---|
| **U-1** | ⚠️ **That any of it renders**, that the page fills the screen, that the prose is legible at 19pt, that the scroll reaches the last paragraph. | **UNMEASURED — zero compiles, zero PIE, zero pixels (fence).** `HELP-§3` / `AS-§6` A(e): this closes on pixels or Jonathan's eyes, ⛔ never on a property readback. The `RebuildWidget()` order law is followed in the new class (tree + `RootWidget`, **then** `Super`) — the one defect that renders empty while passing every readback. |
| **U-2** | ⚠️ **Whether a `Visible` full-screen `UBorder` absorbs an LMB the game viewport would otherwise have received.** | **REASONED, ⛔ NOT OBSERVED.** `SBorder` does not override `OnMouseButtonDown`, so an unhandled click should fall through to the viewport — but that is read, not measured, and D-3 does not depend on it either way: the player is reading a page, TAB and Back both work, and `Escape`/RMB are raw `PlayerTick` polls this feature never touches. **Worth one press at the sitting.** |
| **U-3** | ⚠️ **The three circles' colours** (Select white, Position green, Attack red) as stated on the `PickMode.Confirm` page. | 704's **U-4**, carried: the C++ pushes `StageTint` (`SPC:3253-3265`) and a comment beside it claims the material *"does not carry it YET"*, which 704 measured **STALE** (`TASK-345-artist.md:39-41`; the string is present in `M_SpellReticle.uasset`). ⛔ **I invented no different colours** and softened nothing except removing the parameter's name from player prose. **Only pixels close a colour claim.** |
| **U-4** | **That `QueryKeysMappedToAction` returns the retargeted key in a live game.** | 706's **U-4**, unchanged and inherited: the detail lane uses the identical route and adds no new risk. Cheap live check unchanged — on a Dvorak machine, the Ambush page's chip **and the key named in its last sentence** must both read the key he actually presses. ⭐ The detail page is now a **second, independent place** that same defect would surface. |
| **U-5** | **That TAB reaches Enhanced Input while the overlay is up** (Slate's own focus-next key). | 706's **U-1**, unchanged. The new class's mitigation is the same and no stronger: `BackButton` non-focusable, `DetailScrollBox` non-focusable, ⛔ no `SetKeyboardFocus`/`SetUserFocus` anywhere. ⛔ I did **not** add a key handler to "fix" it speculatively. |
| **U-6** | **That the detail prose reads well and answers the question to Jonathan's satisfaction.** | ⛔ **Not claimable by any agent** (`HELP-§6`, his part). Test 11 proves the three controls are *reachable, keyed and non-empty* from the pages he will click; it cannot prove they are *useful*. |

---

## 9. 🚩 FOR JONATHAN — ONE ROW, AND IT IS A READABILITY CALL, NOT A CORRECTNESS ONE

### F-3 — 704 NAMED THE TUNABLES INSTEAD OF THEIR VALUES, AND THAT NAME NOW APPEARS ON YOUR SCREEN

The resize page reads: *"One notch changes the active circle's radius by **GroupRadiusWheelStep**, clamped between **GroupRadiusMin** and **GroupRadiusMax**."* Several hero pages likewise name `MeleeRange`, `MeleeCooldown`, `WalkSpeed`, `DiscardCost`, `EnemyRevealCost`.

- ⭐ **This is correct and it cannot rot.** It is 704's deliberate U-5/D-6 choice and the M7.7 lesson: the shipped `Notes` column once said *"in 400"* while the real radius was **700**. ⛔ I refused to re-type any number.
- ⚠️ **But it reads as jargon to a player**, and your acceptance bar is that the page explains *how it works*.
- 🙋 **The cheap fix, if you want it, is a follow-up task, not a rewrite:** extend the token mechanism from keys to values — `{value:GroupRadiusWheelStep}` resolved from the **live property** at compose time. Same anti-staleness guarantee as the key chips, same one-definition shape, and ⛔ still no number typed in prose. **I did not do it here** because it reaches outside this task's file fence into `ASiegePlayerController`'s property surface.
- ⛔ **I did not paraphrase the sentences to hide the names** — that is exactly the silent "improvement" the law forbids.

---

## 10. WHAT TASK-708 SHOULD SCRUTINISE

1. ⭐⭐ **§4.1's transfer table, row by row, against `handoffs/TASK-704-programmer.md` §4.** Your criterion 9 names the ambush three-circle/resize/exit block and the war-map 30-gold gate — both are in the table with their citations, and the 30 gold is the **only** number in the registry.
2. ⭐⭐ **The T5 removals (7 sentences across 5 rows), enumerated in §4.1.** Each is claimed to be implementer-facing rather than player-facing. **If you disagree with any one of them, the fix is data-only** — the sentence goes back into the string.
3. ⭐⭐ **§3.1, the `T`/`E` tokenisation.** This is the one place I did not transfer 704 literally, and the reason is that a literal transfer would have been a `HELP-§1` BLOCKER under your own criterion 1. **Check that no letter is typed and that the sentences are otherwise 704's.**
4. **Criterion 1's grep of the diff:** the only letter typed in any player-facing string in the pair is R-20's `Z`, in-line justified by `KBD-§8` + 704 F-1. The `{...}` tokens are ids, not letters.
5. ⭐ **`GetPositionalKey` call count is UNCHANGED at 2** — this task added **zero**. That is the strongest single fact about the detail lane and it is one grep.
6. **Criterion 4's `Escape` grep:** expect comments + the **2 registry-data entries** and nothing else. The new class overrides no key handler; the diff-grep is the authoritative gate (test 6 says so out loud).
7. **Criterion 5:** `SetInputMode`/`bShowMouseCursor` = 5 hits, all comments. `ApplyCursorInputState()` was **not edited at all** — the controller pair is untouched by this task.
8. **Criterion 6:** `RebuildWidget()` order in the NEW class (`.cpp:1640`) — tree, `RootWidget`, **then** `Super`.
9. 🚩 **D-3 (the `Visible` full-screen plate) + U-2.** It is the one behavioural difference from the list view and it is argued from geometry, not preference.
10. 🚩 **D-5 (`RelatedActionIds`) — a new registry field this task invented.** Confirm it is the right answer to *"all the controls with it"*, and that one-level-deep is the right depth.
11. **The engine APIs, all verified public in the 5.8 install before use** (706's blocker lesson): `UWidgetSwitcher::SetActiveWidgetIndex`/`GetActiveWidgetIndex` (`WidgetSwitcher.h:34,39`, `UMG_API`, public) · `UWidgetSwitcherSlot::SetPadding`/`SetHorizontalAlignment`/`SetVerticalAlignment` (`WidgetSwitcherSlot.h:49,55,60`) · `UPanelWidget::AddChild` (`PanelWidget.h:59`) · `UScrollBox::ScrollToStart` (`ScrollBox.h:313`, inside the `public:` at `:254`) · `UWidgetTree::ConstructWidget` on a `UUserWidget` subclass (`WidgetTree.h`, the `if constexpr` branch) · `FString::ReplaceInline` (`UnrealString.h.inl:1838`).
12. **Null-safety of the switch itself:** `SetActiveWidgetIndex` is null-safe and clamps without Slate (`WidgetSwitcher.cpp:49-79`), and `ScrollToStart` early-outs on an unbuilt box (`ScrollBox.cpp:232-238`).
13. **Suite total 156** against 709's gate (+5, one file).
14. ⚠️ **D-10 note for 706's ruling:** the new Back button also uses `ApplyButtonNotFocusable`, so if 708 reverts 706's deprecated-field write, **the revert is three call sites now, not two**.
15. 🚩 **F-3** — the tunable-name readability question. It is Jonathan's, not QA's, but it belongs in the live-items list for the sitting.

---

## 11. STATUS

- **TASK-707 → `ready-for-qa`.** TASK-708 gates 704 + 706 + 707 together; TASK-709 owns the wave's one compile and the suite (**expect 156**).
- ⛔ Nothing compiled, nothing committed, no editor touched, no asset modified, **no console sentence sent (the 552 latch is UNSPENT)**.
