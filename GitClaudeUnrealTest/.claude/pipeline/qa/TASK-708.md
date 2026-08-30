# QA Report — TASK-708

**Gate over TASK-704 + TASK-706 + TASK-707 as ONE artifact** (the registry · the TAB overlay · the full-screen detail view)
**Date:** 2026-08-30 · **Reviewer:** qa-reviewer · **Law:** `HELP-§1..§6` · `AS-§6 A-2` · `KBD-§0`/`§2a`/`§4`/`§5`/`§8` · `SC-§15`/`§20`/`§32`/`§33` · `AS-§12g`
**Artifacts reviewed:** `Source/GitClaudeUnrealTest/Siegebound/SiegeControlsHelpWidget.h` (1031 L) · `.cpp` (2933 L) · `Tests/SiegeControlsHelpTest.cpp` (1548 L) · `SiegePlayerController.{h,cpp}` (the 706 wiring) — **read in full, not sampled.**

## Verdict: **PASS**

**0 BLOCKER · 6 WARN · 6 NIT.** ⛔ No edits, no compile, no engine, no Git, no board write were performed (fences honoured). TASK-709 owns the wave's one compile.

---

## 0. ⭐ THE HEADLINE ITEM — THE `T`/`E` DEFECT: FIXED, AND THE FIX IS **COMPLETE**

704 §4 typed the literal letters `T` and `E` in four sentences (R-13, R-14, R-15, R-18). Those are exactly the two positions US-Dvorak moves (`T`→`Y`, `E`→`.`). A literal transfer would have printed **the wrong keys mid-paragraph on the very screen whose chips exist to print the right ones**, and it would have looked perfect on every machine in this pipeline.

**707's fix is correct, complete, and non-destructive:**

| Check | Result |
|---|---|
| Sentences kept word-for-word? | ✅ Diffed against 704 §4 line by line. Only the two letters moved. |
| Token occurrences in shipped prose | **11**, across **5 rows**: `{Orders.Attack}` ×5 (`:610 :650 :697 :704 :835`) · `{Orders.Defend}` ×4 (`:610 :650 :697 :835`) · `{Interface.ControlsHelp}` ×2 (`:1068 :1074`). **All 3 ids resolve to real registry rows.** |
| Tokens in comments, not prose | 3 (`:436` `:929` `:1263`) — correctly excluded. |
| ⛔ **ANY OTHER HARDCODED KEY LETTER IN PLAYER PROSE, ALL 24 ROWS?** | ⭐ **Swept the whole file with a standalone-letter regex over every `TEXT(...)` literal, then read all 24 rows.** The **only** letter literals in any player-facing string are R-20's `Z`s — the sanctioned `KBD-§8`/`KBD-§0`-ruling-1 exception, which is **Jonathan's flagged F-1 decision**, not an agent's. ⭐ **R-01 does NOT type `WASD`** ("forward, back, strafe left, strafe right"). R-07's `1` and R-10/R-18/R-19/R-21's `Escape`/`Enter`/right-click/wheel are **provably immovable** — `KBD-§4` tables the 26 letters and nothing else (`SiegeKeyboardLayoutStatics.cpp:57-63`). |
| One caveat inside R-20 | See **W-1** — two of the `Z`s are the text box's own undo shortcut, which is a *different* Z from the pinned accept key. |

⇒ **Criterion 1 (the diff grep for hardcoded key letters) PASSES.** This was the batch's highest-risk item and 707 caught it in 704's own prose rather than inheriting it.

---

## 1. THE DOCKET — RULED ON THE RECORD

### 1. ⭐ THE DOUBLE-TRANSLATE TRAP (`KBD-§`) — **CLOSED. Verified at source, not taken on report.**

`grep -rn GetPositionalKey Source/` → in `SiegeControlsHelpWidget.cpp` there are **exactly 2 call sites**, unchanged from 706. TASK-707 added **zero**.

| Site | Branch | Ruling |
|---|---|---|
| `.cpp:1135` | Lane C, **only** when `bLiteralKeyLabel == false` (the F-1 *reversal* branch) | ✅ **Exactly one** translation, on a raw letter Enhanced Input never touched. |
| `.cpp:1171` | Lane A **fallback**, only when `AppliedKeys.Num() == 0` | ✅ **Exactly one** translation, on a key provably nothing mapped. |

**The Lane-A primary path (`.cpp:1156-1160`) returns `AppliedKeys` verbatim and cannot reach a translation — the audit is closed STRUCTURALLY, not by care.** The detail lane adds no second route: `ComposeDetailContent`'s single `ComposeChipForRow` lambda (`.cpp:1318-1322`) is the *only* place a key becomes characters on a page — headline chip, related chips and in-prose tokens all funnel through it. `QueryAppliedKeysForRow` (`.cpp:2882-2933`) is lane-fenced and calls `GetPositionalKey` zero times.

**Corroborated at the engine:** `IEnhancedInputSubsystemInterface::QueryKeysMappedToAction` is documented *"Returns the keys mapped to the given action in the **active** input mapping contexts"* (`EnhancedInputSubsystemInterface.h:381-384`) — i.e. the retargeted duplicate. A second call would be the defect. **And it is asserted, not merely grepped:** test 2 injects both Dvorak hops (`F→U`, `U→G`) and asserts the answer is `U` and is **not** `G` — so a translation added to the primary path turns red **on a QWERTY machine**.

### 2. `KBD-§2a` CONDITION 3 — **ALREADY REPAIRED; NO FALSE FAIL POSSIBLE.**

`CONVENTIONS.md:2336` was amended 2026-08-30 on TASK-705's own measurement: the literals `24 → 25` are **struck through**, and the binding text now reads *"count `N → N+1` where `N` is the count MEASURED IN THE ASSET IMMEDIATELY BEFORE THE APPEND"*, with the general rule *"⛔ NEVER PIN A GROWING COUNT AS A LITERAL IN LAW."* ⇒ TASK-705's `25 → 26` append satisfies the **relative invariant**. ⛔ **I raise no finding against it.** Nothing in the 704/706/707 diff asserts a mapping count, so this does not touch this gate.

### 3. ⛔ `AS-§6` RULING A-2 — `Escape` IS UNTOUCHED. **VERIFIED AT SOURCE.**

The diff-grep is the authoritative gate (test 6 says so itself, since a C++ virtual override is not reflected). Sweep over the pair:

| Token | Hits in `SiegeControlsHelpWidget.cpp` |
|---|---|
| `NativeOnKeyDown` / `NativeOnPreviewKeyDown` / `FReply::Handled` | **3, all comments** (`:1632 :1633` + the class comment). ⛔ **Zero overrides in any of the three classes.** |
| `EKeys::Escape` | **2, both registry DATA** — `Cards.Cancel` (`:505`) and `PickMode.Cancel` (`:814`). Documentation, not handlers. |
| `Escape` in prose | R-07, R-10, R-18, R-19, R-21, R-24 — all as the *shipped cancel gesture*, and R-24 explicitly says the overlay *"never claims Escape"*. |
| `SetInputMode` / `bShowMouseCursor` / `bEnableClickEvents` | **4, all comments.** ⛔ **Zero calls.** |
| `SetKeyboardFocus` / `SetUserFocus` | **0.** |

**Cursor ownership:** `ApplyCursorInputState` (`SPC.cpp:4431`) reads
`bInPlacementMode || bInTargetingMode || (GroupPickStage != None) || bAssistantConsoleOpen || bWarMapOpen || bControlsHelpOpen || bUICursorHeld` — ⭐ **`|| bControlsHelpOpen` is APPENDED as the sixth term and nothing else on the line moved.** `bControlsHelpOpen` appears at **exactly 5 sites in the whole controller** (`:4415` comment, `:4431` the composition, `:5186`/`:5191` its one setter, `:5210` the toggle) ⇒ ⛔ **no shipped guard gained a mirror clause**, and every shipped placement / targeting / group-pick cancel route is byte-unchanged. The detail view is **not** a second cursor owner. ✅ **Criteria 4 and 5 PASS.**

### 4. 707's THREE DECLARED QUESTIONS — ANSWERED

**(a) The T5 sentence removals — I reverse NONE.** I diffed all 24 shipped strings against `handoffs/TASK-704-programmer.md` §4 rather than trusting the table. Every removal I found is genuinely implementer-facing (a raw C++ condition, a provenance clause, a pipeline cross-reference, a defect post-mortem, an engine lifecycle callback, a "this screen's purpose" note), and **every one has its citation preserved in the adjacent C++ comment.** In particular the player-facing halves are correctly **kept**: R-10 keeps *"Escape is a shipped, bound cancel key"*; R-12 keeps the whole band/derived-radius behaviour; R-15 keeps **THE SPAWN DEFAULT paragraph in full**; R-16 keeps the three colours. ⚠️ The *enumeration* is incomplete — see **W-3**.

**(b) D-3 — the `Visible` full-screen plate: ACCEPTED, with a declared unknown.** Ruling in one line: *the asymmetry is correct — a plate you are reading must absorb its own clicks, which is the rule `PanelBorder` already follows, at screen size — and it cannot swallow the routes `HELP-§5` protects, because those are the raw `PlayerTick` polls of `Escape`/RMB and no widget here claims a key.* My own engine read agrees with U-2 (`SBorder` overrides no mouse-button handler, so an unhandled LMB bubbles up the path to `SViewport`), and the root user widget itself is set `SelfHitTestInvisible` on open (`.cpp:2581`) while `UWidgetSwitcher` is `SelfHitTestInvisible` **by construction** — I verified that at `WidgetSwitcher.cpp:18`, and it is load-bearing: had the switcher been `Visible`, it would have made the *list* view a full-screen absorber and broken 706's design. ⚠️ It is **read, not observed** → **W-5** and a press at TASK-710.

**(c) D-5 — `RelatedActionIds`: ACCEPTED, and the cycle refusal IS structural.** Ruling in one line: *it is the right answer to "all the controls with it" because it renders the other rows' OWN text — one definition, two renderings, zero duplicated prose to drift — and one level deep is the right depth.* Verified at source (`.cpp:1345-1376`): the loop reads each related row's `DisplayName` and `ComposeDetailForDisplay(*RelatedRow)` and **never touches `RelatedRow->RelatedActionIds`**, so recursion is impossible by construction rather than by a depth counter. Data: **13 rows, 25 references, all 25 resolve, zero self-references** (counted at source). Ambush/Hold/Follow each render `PickMode.Confirm` / `Resize` / `Cancel` — Jonathan's three named questions — from those rows' own strings. Test 11 asserts both halves: that the data really contains a cycle (`PickMode.Confirm` declares its own related ids) **and** that `Page.Related.Num() == Row.RelatedActionIds.Num()`, so a transitive expansion turns red.

### 5. CARRIED, ⛔ NOT RE-DECIDED — 706's D-1 and D-10

- **706 D-1 (compose, don't exclude):** carried unchanged. It is not a `HELP-§5` violation — that law requires the shipped cancel routes keep firing, which they do; it does not require the overlay to refuse to open. The declared residual (a card key can start placement under the overlay) is real, is not a soft-lock, and is **Jonathan's design call at TASK-710**, not QA's.
- **706 D-10 (the pragma-wrapped `UButton::IsFocusable` write): RULING — LEAVE IT, three call sites and all.** I re-measured the 5.8 headers myself rather than accept the report: `UButton::InitIsFocusable` is **`protected`** (`Button.h:206`, inside the `protected:` opened at `:187`) ⇒ the obvious call really would be **two hard C2248s**; the field `IsFocusable` really is **public** (`Button.h:70`, inside the `public:` at `:36`) and merely `UE_DEPRECATED(5.2)`. The write is the only route short of a `UButton` subclass, which would break the `HELP-§3` WBP bind hatch. Three sites (`.cpp:1442` RowButton, `:1900` BackButton, `:2399` CloseButton) is still a trivial revert — **and reverting would delete the only mitigation for U-1 (TAB vs Slate's focus-next key), which is this batch's top PIE risk.** Carrying one pragma-wrapped deprecated field write is far cheaper than shipping a toggle that may stop closing after the first row click.

### 6. THE TRANSFER PROOF — SPOT-CHECKED AT SOURCE, ⛔ NOT TRUSTED

| Claim | My measurement |
|---|---|
| 24/24 `Detail` fields filled | ✅ `Row.Detail = FText::FromString` × **24**. |
| 25 `RelatedActionIds` references resolve | ✅ counted at source; all 25 name real ids, zero self-references. |
| 5 token sites resolving | ✅ 11 occurrences / 3 ids, all resolve (table in §0). |
| Every 704 citation preserved beside its string | ✅ present on every row — **but see W-2, the coordinates have drifted.** |
| ⭐ **The war-map 30-gold gate, against the cited file:line** | ✅ **VERIFIED AT SOURCE:** `CommanderNpc.h:311` — `int32 EnemyRevealCost = 30;` with Jonathan's quote *"You can pay 30 gold to reveal all enemy locations"* at `:298-299` and *"THE AI NEVER SPENDS GOLD"* at `:304-307`. The page's sentence is accurate and the number is quoted from his own words at the property — the **one** number in the whole registry, correctly. |
| ⭐ **The ambush leash exemption, against the cited file:line** | ✅ **VERIFIED AT SOURCE:** `SummonedUnit.cpp:1778` — the zone drop-test really is wrapped in `if (CurrentTarget && Group.Type == ESiegeGroupCommandType::Hold)`, with the intent stated verbatim at `:1788-1792` (*"AMBUSH deliberately skips this whole block while a live target exists … it keeps the target until the kill, then the ladder resumes"*). 707's T5 repair (*"skips that whole block"* → *"skips the zone drop-test entirely"*) is faithful. Dead target dropped for both types at `:1770-1774` ✅; HOLD-only monotone upgrade at `:1795-1805` ✅. |
| The three-circle / resize / exit block vs 704 §4 | ✅ Read side by side. Every clause of R-16/R-17/R-18 transferred; only T1 citations, T3 emoji and the enumerated T5 clauses removed. **Every tunable NAME on screen exists verbatim as a real property** — `GroupRadiusWheelStep`/`Min`/`Max` (`SPC.h:1380/1384/1388`), `GroupSelectRadiusDefault`/`Position`/`Attack` (`:1392/1396/1400`), `MeleeRange`/`HalfAngle`/`Cooldown` (`HeroCharacter.h:437/441/445`), `WalkSpeed`/`SprintSpeed` (`:406/410`), `Rally*` (`:449-461`). ⇒ ⛔ **no number is re-typed and no name can rot.** |
| The AI-chat close routes vs `AS-§6 A-2` | ✅ All four enumerated routes transferred exactly (`CONVENTIONS.md:733-737`), including *"empty-Enter closes even with a confirm prompt up"* and *"a close is not a cancel"*. |
| "Zero developer-only fragments" is a **TEST**, not a scan | ✅ **CONFIRMED** — test 9 (`EveryRowHasAuthoredDetail`) runs `.cpp:` `.h:` `handoffs/` `TASK-` `SPC:` `**` `` ` `` `§` over all 24 rows on **every suite run**. ⚠️ Its coverage limit, stated plainly: it does **not** catch a bare C++ symbol name (**W-6**) and it does **not** catch a hardcoded key letter — that guarantee rests on test 10 and on my grep, not on test 9. |

### 7. SUITE DELTA 151 → 156 — **VERIFIED BY COUNT, AND THE TESTS TEST BEHAVIOUR**

`IMPLEMENT_*_AUTOMATION_TEST` across the module = **156 exactly**, across 13 files, with **13 in `SiegeControlsHelpTest.cpp`** (8 from 706 + 5 from 707). ⇒ 709's gate is **156**.

The five new tests assert behaviour, ⛔ not the pin they were written from:
- **9** — detail ≠ TODO, ≠ the one-liner, longer than it; forbidden fragments; related ids resolve; no self-reference. These fail on the *data*, not on a remembered string.
- **10 (keystone)** — the Ambush page's **prose** differs across the two simulated layouts and **contains the accessor's own answer on each**, with a fixture self-check first so "it changed" is not vacuous; no unresolved token survives on any page or related block on either layout; ⭐ at least one page really carries a token (so the previous claim is not vacuous either); a token-free page **HOLDS**; the resolver replaces once, leaves an unknown token **visible**, and asks the provider nothing for token-free prose. ⛔ Nowhere does it assert a typed letter.
- **11** — the three named controls reachable, keyed and non-empty from all three circle orders, **and identical to those rows' own pages**; the no-recursion count with its cycle self-check; unknown/self/empty ids dropped.
- **12** — ⭐ pins the deliberate asymmetry (unstamped **row** reports nothing; unstamped **page** still fires Back), the stamp surviving a widget with no tree at all, and clean unbinding.
- **13** — null subsystem composes all 24 pages; applied key outranks the registry on a page; pointer-only still chips; undocumented and whitespace-only both yield the pinned TODO string.

**Is `ReturnToList` genuinely idempotent and null-safe?** ✅ **Yes, and I verified the engine claim myself rather than accept it:** `.cpp:2843-2848` returns early on a null switcher, and `UWidgetSwitcher::SetActiveWidgetIndex` is `if (ActiveWidgetIndex != Index) { … }` (`WidgetSwitcher.cpp:50-58`) ⇒ an unchanged index costs nothing and broadcasts nothing. That is what lets both `CloseHelp()` and `OpenHelp()` call it unconditionally. ⚠️ **It is proven by engine source, not by a test** — see **N-2**.

### 8. COMPILE-READINESS (this is pre-compile QA; TASK-709 owns the only compile)

**Every engine API in the diff re-verified in the installed 5.8 headers by me, ⛔ not taken from the handoff:**

| API | Verified |
|---|---|
| `UButton::InitIsFocusable` | **`protected`**, `Button.h:206` ⇒ D-10's C2248 diagnosis is **correct** |
| `UButton::IsFocusable` (field) | **public**, `Button.h:70`, `UE_DEPRECATED(5.2)` ⇒ the pragma-wrapped write is legal |
| `UScrollBox::SetIsFocusable` / `ScrollToStart` | `ScrollBox.h:252` / `:313` — both **public** (`public:` at `:50` / `:254`) |
| `UScrollBox::SetConsumeMouseWheel` / `SetAlwaysShowScrollbar` / `SetOrientation` | `:154` / `:179` — public |
| `UTextBlock::SetFontSize(float)` | `TextBlock.h:222` — public; the call sites pass floats |
| `UWidgetSwitcher::SetActiveWidgetIndex` / `GetActiveWidgetIndex` | `WidgetSwitcher.h:38` / `:34` — public `UMG_API` |
| `UWidgetSwitcher` ctor ⇒ `SelfHitTestInvisible` | `WidgetSwitcher.cpp:18` ✅ **load-bearing and true** |
| `UWidgetSwitcherSlot::SetPadding` / `SetHorizontalAlignment` / `SetVerticalAlignment` | `WidgetSwitcherSlot.h:50/55/60` — public `UMG_API` |
| `UWidgetTree::ConstructWidget` | `WidgetTree.h:106-122` — parameter is `TSubclassOf<WidgetT>`; the `if constexpr` UUserWidget branch routes through `CreateWidget`. The call at `.cpp:2449` passes a matching `TSubclassOf<USiegeControlsDetailWidget>` ✅ |
| `ULocalPlayer::GetSubsystemFromController<T>(const APlayerController*)` | `LocalPlayer.h:389` ✅ — the **const** pointer at `.cpp:2894` compiles |
| `IEnhancedInputSubsystemInterface::QueryKeysMappedToAction(const UInputAction*) **const**` | `EnhancedInputSubsystemInterface.h:384` ✅ — **const**, so calling it through a `const` subsystem pointer compiles |
| `GetTransientPackageAsObject()` | `UObjectGlobals.h:276`, `COREUOBJECT_API`; the test file includes `UObject/UObjectGlobals.h` ✅ |

**Other compile/lifetime checks, all clean:**
- ⛔ **`RebuildWidget()` ORDER — all three classes correct:** `Initialize()` → build tree → set `WidgetTree->RootWidget` → (re-apply stamp) → `return Super::RebuildWidget();` (`.cpp:1385-1403`, `:1640-1653`, `:2176-2183`). The `UserWidget.cpp:1214` empty-render trap is closed in the new class too. ✅ **Criterion 6 PASSES.**
- **674-1 ordering hole closed in the new class:** `SetDetailContent` stores **unconditionally** and `RebuildWidget` re-applies (`.cpp:1969-1978`), and test 12 drives the harshest version (a stamp onto a widget with no tree at all).
- **No dangling pointer at `.cpp:1303`:** `ReplaceInline(*Token, *ChipProvider(...).ToString(), …)` — the `FText` temporary lives to the end of the full expression, so the `TCHAR*` is valid for the call.
- **No reference invalidation in the registry:** `Rows.Reserve(32)` precedes 24 `AddDefaulted_GetRef()` ⇒ the `FSiegeControlsHelpAction&` handed out by `AddRow` can never dangle.
- **GC-safe pointers:** `RowWidgets` is `UPROPERTY(Transient) TArray<TObjectPtr<>>`; every child widget is a `UPROPERTY`; `ControlsHelpWidget` is `UPROPERTY(Transient)`. The plain structs hold **FText/FName only** — no UObject pointer, no `FKey`.
- **UHT:** neither plain struct is ever a `UFUNCTION` parameter or a `UPROPERTY`; `SetDetailContent`/`RequestBack`/`GetDetailActionId`/`ShowDetailForAction`/`ReturnToList`/`QueryAppliedKeysForRow` are all **non-`UFUNCTION`**, so `HELP-§3`'s struct-BIE ban cannot fire. `ESiegeInputLane` is a plain `uint8` `UENUM` and is never a BIE parameter.
- **Delegate lifetimes:** bound in `NativeConstruct` (children do not exist until `RebuildWidget` has run) and unbound in `NativeDestruct`, including `DetailView->OnBackRequested.Unbind()` and every row's `OnRowActivated.Unbind()` before release. `AddUniqueDynamic`/`RemoveDynamic` paired. The controller's `OnHelpOpenChanged` bind is taken **exactly once** at the single creation site (`SPC.cpp:5335`) and removed in `EndPlay` **before** `CloseHelp()` (`:454-455`) so no broadcast lands in a dying controller.
- **Re-entrancy terminates by construction:** `ApplyOpenState` writes `bHelpOpen` **before** broadcasting (`.cpp:2577` then `:2589`), so the controller's `HandleControlsHelpOpenChanged(true)` → refused → `CloseHelp()` → broadcast(false) chain lands on the `!bOpen` branch and stops.
- **`SC-§33`:** every new function takes **zero** defaulted parameters — structurally immune. The two engine calls with trailing defaults are passed **explicitly**: `FKey::GetDisplayName(/*bLong=*/false)` and `Contains`/`ReplaceInline(…, ESearchCase::CaseSensitive)`. ✅
- **`AS-§12g` / airlock:** `Capture()` / `EnsureSnapshot()` = **0**. Zone A untouched, **552 latch UNSPENT**, no token figure. ✅
- **`KBD-§1`/`§2` read-only:** `MapKey` / `UnmapKey` / `UnmapAll` = **0 calls in `Source/`** from this diff; no `IMC_Hero` write. ✅ **Criterion 7 PASSES.**
- **Criterion 8 (soft + null-safe `IA_ControlsHelp`):** `ControlsHelpActionAsset` is a `TSoftObjectPtr` resolved through the shared `ResolveInputAction` (`SPC.cpp:507`); a null resolve simply skips the binding (`:624`). **TAB inert, never a crash.** ✅
- **Criteria 3, 10, 11, 12, 15:** labels re-derive on every open (`OpenHelp` → `RefreshKeyboardLayout()` → `ReturnToList()` → `RefreshRows()`, which **destroys every row first**), ⛔ no `FKey` or label string is a member anywhere, ⛔ nothing binds `OnKeyboardLayoutChanged`; the detail page re-queries at **click** time rather than reusing the list's pass; no blank rows (the pinned TODO string, driven by tests 5/13); read-only on the world and **no pause** — I grepped the pair for a mutating controller call and there is none; the M8 declaration is present in all three class comments and in the test file. ✅

---

## Findings

- **[WARN] `SiegeControlsHelpWidget.cpp:939` — R-20's `Ctrl+Z` / `Ctrl+Shift+Z` are letter literals that sit OUTSIDE the `KBD-§8` accept-key pin, and 707's stated reversal path is wrong for them.** The accept key is pinned to the literal `Z` because the console's own status line says `Z` (F-1, Jonathan's). But `Ctrl+Z`/`Ctrl+Shift+Z` describe the **Slate text box's own undo/redo**, which `USiegeKeyboardLayoutSubsystem` does not retarget at all — so that `Z` does **not** follow the accept key's pinned position. ⇒ 707 §3.1's claim that an F-1 reversal moves *"this prose"* to `{Interface.AssistantAccept}` tokens is **false for this one sentence**; blanket-tokenising it would *introduce* a defect. **Fix (comment-only, no behaviour change):** one line beside `:939` recording that this `Z` belongs to the text box, not to the accept key, and must not be tokenised if F-1 is ever reversed. ⛔ Not a blocker — the sentence is descriptive, is within 704's flagged R-20 block, and instructs the player to do nothing.
- **[WARN] `SiegeControlsHelpWidget.cpp` (every `SPC:` / `SPC.h:` citation in the new comments) — the transfer carried 704's line numbers, which were read BEFORE TASK-706 grew that very file in this same batch.** Measured drift: `.cpp` **≈ +56…+58** (the pick RMB/`Escape` poll cited `SPC:591-596` is now at **`:647-654`**; `ApplyGroupPickWheel` cited `:2785-2806` is at **`:2843`**; `CancelGroupPick` cited `:3135-3141` is at **`:3193`**), `.h` **≈ +115** (`GroupRadiusWheelStep` cited `SPC.h:1265` is at **`:1380`**). ⭐ **Every citation into a file this batch did NOT touch verified exactly** (`CommanderNpc.h:311`, `SummonedUnit.cpp:1778`, `HeroCharacter.h:437-461`). The **facts are all true** — I re-read the pick branch and it is exactly what the prose says; only the coordinates rotted, and most comments name the symbol as well. **Fix:** comment-only sweep as a follow-up task. ⛔ Do **not** block 709 on it.
- **[WARN] `handoffs/TASK-707-programmer.md` §4.1 / §10 — the declared T5 enumeration is incomplete and self-inconsistent.** §10 says *"7 sentences across 5 rows"*; §4.1's own table carries T5 entries on **7** rows; and two further removals are shown as "—": R-17 lost *"because `ApplyGroupPickWheel` is only ever called from the pick branch"* and R-22 lost *"quoted at the property"*. **I did not rely on the table — I diffed all 24 strings against 704 §4 directly, and I reverse none of them:** every removal is genuinely implementer-facing and its citation survives in the adjacent comment. **Fix:** none required in code; the count in the handoff is wrong, the code is not.
- **[WARN] `SiegeControlsHelpWidget.cpp:2698-2705` + `:1168-1173` — the `(not bound)` / `HintNoKey` "honest degradation" is UNREACHABLE for any row carrying a QWERTY reference key.** Lane A's fallback always yields a chip from `QwertyReferenceKeys`, so for `Interface.ControlsHelp` (ref key `Tab`) `ControlsHelpOwnChip` is never empty and the `HintNoKey` branch is dead code — the hint would read *"Press Tab again…"* even with `IA_ControlsHelp` missing. ⇒ **706's U-3 and the board status line (*"an unresolved asset … renders the honest '(not bound)' chip with a degraded hint line that names no key"*) are FALSE as implemented.** **Practical impact: NIL** — TAB is the *only* route that opens the overlay, so with the asset missing the misleading line can never be seen. **Fix:** none for 709; ⛔ just do not use `(not bound)` as a diagnostic for a missing asset.
- **[WARN] `SiegeControlsHelpWidget.cpp:1691` — D-3's `Visible` full-screen `DetailBackdrop` is ACCEPTED but UNOBSERVED.** While a detail page is up the entire screen is hit-testable, where the list leaves everything outside its 140/60 plate transparent. My own engine read supports U-2 (`SBorder` overrides no mouse-button handler ⇒ an unhandled LMB bubbles to `SViewport`) and `Escape`/RMB are raw `PlayerTick` polls no widget here claims, so `HELP-§5`'s byte-identical guarantee should hold — **but that is read, not measured. Fix:** one press at TASK-710 (see live items). ⛔ Not a soft-lock in any case: TAB closes the overlay and **Back** returns to the list.
- **[WARN] Readability, for Jonathan — 707's F-3 is REAL and it is WIDER than the tunables.** Player prose also names C++ **symbols**: `UCardHandWidget::RequestDiscardSlot → ASiegePlayerController::DiscardHandSlot` (`:488-489`), `ASummonedUnit::ResolveDefendEngagementRadius` (`:575`), `AHeroCharacter::FellOutOfWorld … does not call Super (which would Destroy() the pawn)` (`:345-346`), `SetMeleeSuppressed(true)` / `DoMeleeAttack` (`:394`), `GetEffectiveSprintSpeed()`/`GetEffectiveWalkSpeed()` (`:366-367`), `CancelPressed()` — *"public API with no caller today, kept deliberately"* (`:879-880`), and *"which is why these four rows must never be rewritten as a block"* (`:307`, an instruction to an implementer). Test 9's forbidden-fragment list does not catch bare symbol names. ⭐ **707 was RIGHT not to re-author** — its mandate was verbatim transfer and the M7.7 lesson bans re-typing values. ⇒ **This is ONE product decision for Jonathan at TASK-710, taken alongside F-3, not twelve tasks and not a code defect.** The fix, whichever way he rules, is **data-only**.
- **[NIT] `SiegeControlsHelpWidget.h:223-265` — D-6's rationale is inverted.** Keeping `FSiegeControlsDetailEntry`/`FSiegeControlsDetailContent` out of reflection makes test 6's `FKey`-property sweep **weaker**, not "meaningful" — a non-reflected member is precisely what the sweep cannot see. The substantive claim still holds: I read both structs and neither carries an `FKey`. Plain C++ is still the right call for the other stated reasons.
- **[NIT] `SiegeControlsHelpWidget.cpp:2770-2861` — zero machine coverage on the OVERLAY side of the seam.** `ShowDetailForAction` / `ReturnToList` / `HandleDetailBackRequested` / `IsDetailViewActive` are never driven (they need a live tree); only the two widget-side entry points (`ActivateRow`, `RequestBack`) are. The idempotency + null-safety claims are proven by engine source, which I re-verified. **Put the round trip on 710's press list.**
- **[NIT] `Tests/SiegeControlsHelpTest.cpp:1164-1183` — test 10 asserts `!Body.Contains("{")` on every page.** A future row whose prose legitimately contains a brace turns it red. Acceptable brittleness; worth knowing before someone is puzzled by it.
- **[NIT] `SiegeControlsHelpWidget.cpp:2196-2201` — misleading log on a second `RebuildWidget()`.** The `RootWidget != nullptr` early-out logs *"An asset-authored tree is present — the code-authored branch is skipped"* even when the tree is the code-authored one being reused. Log text only.
- **[NIT] `SiegeControlsHelpWidget.cpp:325` — R-02 says "a `Negate_2` modifier on the **Y** channel".** `Y` is an axis here, not a key; a player may read it as the Y key. Data-only if Jonathan cares (fold into the W-6 ruling).
- **[NIT] `SiegeControlsHelpWidget.cpp:2028-2124` — `RebuildRelatedBlocks` constructs ≤5 widgets per block (≤3 blocks) on every stamp and abandons the previous ones to GC.** Correct and bounded — **per click, never per tick** — and the same shape `RefreshRows` already ships. No change wanted: destroying them is what makes "no page shows the previous page's controls" structural.

---

## Notes for build-master (TASK-709)

1. ⛔ **Parse the log for `Result:` — the exit code LIES** (Build.bat returns 0 on a failed build).
2. ⚠️ **If Smart App Control is enforced the build dies in ~2 s with `0x800711C7`. That is MACHINE STATE, ⛔ not a code error — report it and stop; do NOT loop QA.**
3. **Expect suite total 156** (13 files; **13** in `Tests/SiegeControlsHelpTest.cpp`). I counted it, so a 155 or 157 is not this batch's.
4. **The only `UE_DEPRECATED` access in the diff** is `Button->IsFocusable = false` inside `PRAGMA_DISABLE/ENABLE_DEPRECATION_WARNINGS` at `.cpp:176-178`, reached from **three** sites (`:1442`, `:1900`, `:2399`). QA has ruled it **stays**. ⛔ If you see a **C2248 on `InitIsFocusable`**, someone reverted it the wrong way — restore the field write, do not "fix" it by calling the protected initializer.
5. **If a `C2445` appears**, it is the `TSubclassOf`-vs-`UClass*` ambiguity the file documents twice (`:2143-2149`, `:2441-2450`). The fix is `.Get()` on both arms / two statements — ⛔ never a ternary.
6. **Commit cargo, explicit paths only:** `Source/GitClaudeUnrealTest/Siegebound/SiegeControlsHelpWidget.{h,cpp}` · `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeControlsHelpTest.cpp` · `Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.{h,cpp}` · pipeline files (board / handoffs 704·706·707 / `qa/TASK-708.md`). ⛔ `git add -A` banned · ⛔ never push.
7. ⚠️ **If TASK-705's two assets ride in this commit, honour its declared trap:** both paths are LFS-tracked and the editor's SCC auto-staged the new asset while `IMC_Hero`'s index entry still holds the **pre-append** blob — **re-`add` BOTH and verify by oid-vs-sha256, ⛔ never by size.**
8. **Say plainly in the handoff whether `IA_ControlsHelp` exists.** Per the board TASK-705 landed (`IMC_Hero` 25→26), so TAB should be live. If it did not, note **W-4**: the hint will still read *"Press Tab again"* and the row chip will still read *"Tab"* — that is the Lane-A fallback, not a defect, and the overlay would be unopenable anyway.
9. ⛔ **Nothing in this diff is engine-risky at runtime:** no tick work, no `FindObject`/`LoadObject` in a hot path (`LoadSynchronous` is per-open / per-click on already-cooked soft refs), no timer, no unbound delegate, no world mutation.

## ⭐ LIVE ITEMS FOR TASK-710 (Jonathan's sitting) — in the order I would press them

1. ⭐⭐ **U-1 — does TAB still CLOSE the overlay after you have clicked a row?** `Tab` is Slate's own focus-next key. Mitigated by three non-focusable buttons and two non-focusable scroll boxes and by ⛔ never adding a key handler. **This is the first suspect if TAB opens but will not close — and the fix is a focus release, ⛔ NEVER an `Escape` grab.**
2. ⭐⭐ **U-4 — on your Dvorak layout: the Ambush row's CHIP *and* the key named in the last sentence of the Ambush PAGE must both read the key you actually press.** The detail page is now a **second, independent** place that defect would surface.
3. **D-3 / U-2 — with a detail page open:** right-click and `Escape` must still cancel a live placement or pick, and a click on a sentence must not place a card in the world.
4. **The seam round trip (untested by machine):** row → page → **Back** → another row → TAB out → TAB in **must land on the LIST**.
5. **F-3 + W-6 — the readability ruling, as ONE decision:** the pages name tunables (`GroupRadiusWheelStep`) *and* C++ symbols (`ASummonedUnit::ResolveDefendEngagementRadius`, `CancelPressed()`). 707 correctly refused to re-type a number (the M7.7 lesson). Your call; the fix either way is data-only, and the cheap route is a `{value:...}` token read from the live property.
6. **U-3 — the three circle colours** (Select white / Position green / Attack red) close on pixels only.
7. **706 D-1 — the design call:** with the overlay open a card key can still start placement underneath it. Not a soft-lock; the alternative costs three shipped guards.

---

# ⛔ TASK-709 INTEGRATION GATE — APPENDED BY BUILD-MASTER, 2026-08-30

**The compile PASSED. The suite is RED. ⛔ NOTHING WAS COMMITTED.** Routing back per CLAUDE.md rule 6 (counts as a QA loop).

## 1. COMPILE — ✅ **`Result: Succeeded`** (parsed, ⛔ not the exit code)

Build.bat returned **exit 0**, which is worthless. The log's line 43 reads **`Result: Succeeded`**. Independently corroborated: zero `error C####`, zero `error LNK`, zero `0x800711C7`, and ⭐ **zero `C2248` and zero `C2445`** — QA's notes 4 and 5 did not fire, so D-10's pragma-wrapped field write compiled exactly as ruled. All three batch translation units built (`SiegeControlsHelpTest.cpp`, `SiegeControlsHelpWidget.cpp`, `SiegePlayerController.cpp`) and `UnrealEditor-GitClaudeUnrealTest.dll` linked. 18 actions, 19.42 s.

**Smart App Control was pre-checked, ⛔ not assumed:** `HKLM:\SYSTEM\CurrentControlSet\Control\CI\Policy\VerifiedAndReputablePolicyState = 0` (OFF). Editor confirmed down before the build; no cook or other gate in flight.

## 2. SUITE — ⚠️ **155 / 156 — ONE FAILURE, AND IT IS THIS BATCH'S OWN TEST**

Ran headless (`-nullrhi -unattended -nopause -nosplash`, no desktop). **Count is 156 exactly** — QA's §7 number is confirmed on both instruments: the static macro sweep (156 across 13 files, 13 in `SiegeControlsHelpTest.cpp`) **and** 156 unique executed test paths. ⇒ **the total is right; the colour is not.**

```
Siegebound.ControlsHelp.RawLanesAreIdentity      Result={Fail}
  Error: Expected '...and it is EXACTLY ONE translation, never two' to be true.
  SiegeControlsHelpTest.cpp(632)
```

All other 155 pass, including ⭐ **test 2 `MappedLaneIsNeverDoubleTranslated` — GREEN**, so the structural double-translate guard on the shipped Lane-A primary path is proven at runtime, not merely at source.

### ⭐ THE SHIPPED CODE IS CORRECT. THE DEFECT IS ONE ASSERTION IN THE FIXTURE.

Measured, ⛔ not inferred. `MakeDvorakTranslation()` (`:85-98`) injects **11 entries whose domain is entirely letters**. The accept key maps `Z → Semicolon` (`:98`).

| Step | Value |
|---|---|
| `ZTranslated = GetPositionalKey(Z)` | `Semicolon` |
| `DerivedResolved[0]` | `Semicolon` ⇒ **exactly one translation — line 630 PASSED** |
| `GetPositionalKey(ZTranslated)` = `GetPositionalKey(Semicolon)` | `Semicolon` — **identity, because `Semicolon` is not a letter and so is not in the map's domain** |
| The assertion at `:633` | `Semicolon != Semicolon` ⇒ **FALSE by construction** |

⇒ **The probe cannot pass no matter how correct the code is, and worse, it cannot distinguish one translation from two at this position** — the very thing it claims to measure. Contrast the deliberate authoring at `:88-89`, where `F → U` **and** `U → G` exist precisely so test 2's second hop lands somewhere distinct. Test 4 reuses that idiom on a key that has no second hop.

⛔ **The failing branch is not even the shipped state.** It is the Lane-C **F-1 reversal** case (`bLiteralKeyLabel == false`). The shipped pin — literal `Z` on every layout — is asserted at `:613-619` and **PASSES**.

### ⛔ WHY BUILD-MASTER DID NOT FIX IT
A test is code. Per the standing fence, any diff here owes its own **`SC-§27` diff-scoped verdict** before it may be committed, and Jonathan is asleep. **Two candidate repairs, ⛔ neither authored, for the programmer to choose and QA to rule on:** (a) give `Semicolon` a second hop in the fixture map so the probe becomes meaningful; or (b) drop/restate `:632-633` for Lane C, recording that `Z`'s translated position is a fixed point by construction and that test 2 already carries the real double-translate guarantee.

### ⚖️ THE PROCESS NOTE WORTH KEEPING
This is a defect **pre-compile QA structurally cannot catch** — QA correctly never runs the suite, and this batch had never been executed before tonight. QA §7's "the tests assert behaviour" is true of their *design*; the assertion above is simply unsound for its chosen key. ⛔ No criticism of the PASS is implied — it is the gate working as intended, one stage later.

## 3. STATE LEFT BEHIND — DECLARED

- ⛔ **No commit. No push. No editor. No MCP. `main` untouched at `22728c8`.**
- ⭐ **TASK-705's LFS trap was REAL and IS NOW DISARMED IN THE INDEX — do not re-arm it.** As found, `IMC_Hero.uasset` was staged at oid `9654fae4…2950`, byte-identical to HEAD — the **pre-append** blob — while the working tree held `9ba4aeb0…46ab`. **A commit as-found would have shipped the TAB mapping ABSENT while every file "looked" staged.** Both assets have been re-`add`ed and verified **oid-vs-sha256** (⛔ never by size); both now match. **This staging is deliberately left in place** so the next integrator inherits the disarmed state.
- ✅ **`IA_ControlsHelp` EXISTS** — `Content/Input/Actions/IA_ControlsHelp.uasset`, sha256 `b3757ca0…3df2f`, matching TASK-705's declared value.
