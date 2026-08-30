# TASK-706 — [HELP-3] THE TAB CONTROLS OVERLAY — programmer handoff

**Agent:** gameplay-programmer · **Date:** 2026-08-30 · **Law:** `HELP-§1`/`§2`/`§3`/`§4`/`§5`/`§6` (cited, not restated) · `AS-§6 A-2` · `KBD-§0`/`§1`/`§2`/`§5`/`§7`/`§8` · `SC-§15`/`§32`/`§33` · `AS-§12g`
**Input:** `handoffs/TASK-704-programmer.md` (the 24-row registry + its §1.1 lane audit + its §3 seam) — adopted, including its **D-5 one-translation route** and its **F-1 ruling**.
**Consumers:** TASK-707 (the detail view — §4 is its contract) · TASK-708 (QA gate) · TASK-709 (compile + suite)

---

## 0. WHAT LANDED, AND WHAT DELIBERATELY DID NOT

| | |
|---|---|
| ✅ **Files written (2 new)** | `Source/GitClaudeUnrealTest/Siegebound/SiegeControlsHelpWidget.h` · `.cpp` |
| ✅ **Test file (1 new)** | `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeControlsHelpTest.cpp` |
| ✅ **Files edited (2)** | `Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.h` · `.cpp` — the wiring the spec names, and nothing else |
| ✅ **Pipeline files** | this handoff + this task's own status line on `TASKBOARD.md` |
| ⛔ **NOT done** | no compile · no editor · no MCP · no Git · no asset touched · no console sentence (the 552 latch is UNSPENT) · no board edit beyond this task's status line |
| ⛔ **NOT in scope** | the full-screen detail VIEW (TASK-707 — §4 below is its contract); this task ends at *"the row is clickable and reports the click"* (spec item 7) |

**Three classes / one struct, all in the one pinned file pair (`HELP-§3`, the `UAccountMenuWidget` precedent — ⛔ no WBP):**
`FSiegeControlsHelpRegistry` (the 24 rows + the pure label lane) · `USiegeControlsHelpRowWidget` (one clickable row) · `USiegeControlsHelpWidget` (the scrolling overlay) · `ESiegeInputLane` + `FSiegeControlsHelpAction` (704 §3's shape, adopted).

---

## 1. ⭐⭐ THE LABEL LANE — THE IMPLEMENTATION TABLE

All four of 704's lanes are implemented, in **one** function: `FSiegeControlsHelpRegistry::ResolveRowDisplayKeys(Row, AppliedKeys, LayoutSubsystem)` (`SiegeControlsHelpWidget.cpp:410-490`).

| Lane | Rows | Where the displayed key comes from | `GetPositionalKey` calls | Chip |
|---|---|---|---|---|
| **A — Mapped** (primary) | 18 of 24 | `QueryKeysMappedToAction(IA_*)` on the **active** contexts — ⚠️ **already retargeted** by the layout system | ⛔ **ZERO** | the keys' own short display names |
| **A — Mapped** (fallback) | same rows, only when **no active context maps the action** (asset absent / context not applied) | `GetPositionalKey(QwertyReferenceKeys[i])` | ✅ **EXACTLY ONE** — and on a key nothing has translated | same |
| **B — Raw, non-letter** | LMB · RMB · `Escape` · wheel up/down · Mouse2D | `QwertyReferenceKeys` verbatim | ⛔ **ZERO** — identity is *proven*, not assumed (`SiegeKeyboardLayoutStatics.cpp:57-63` is A..Z only) | same |
| **C — Raw, LETTER** | exactly one (`Interface.AssistantAccept`) | `QwertyReferenceKeys` verbatim **while `bLiteralKeyLabel`** (`KBD-§8`); ⭐ **derives when the flag is cleared** | ⛔ zero pinned / ✅ exactly one reversed | `Z` |
| **D — Pointer only** | 3 (`Cards.Discard`, `Interface.WarMapReveal`, `Interface.WarMapMarker`) | no key exists | n/a | the **pointer chip** ("Mouse click"), ⛔ never a key |
| **(no key resolved)** | any keyed row whose action + reference both fail | — | — | the explicit **"(not bound)"** chip — ⛔ **the row is never hidden** (`HELP-§2` mech. 2) |

**The live query is isolated so the pure part stays pure:** `USiegeControlsHelpWidget::QueryAppliedKeysForRow(Row, OwningController)` (`.cpp:1400-1450`) is the *only* code that touches Enhanced Input, refuses to answer for any non-Lane-A row **by lane**, and is null-safe at every hop (no controller / no local player / no subsystem / unresolvable asset ⇒ empty ⇒ the fallback runs).

**⭐ F-1 is honoured AND kept to one flag.** `bLiteralKeyLabel` is not decorative — the Lane-C branch implements **both** states, and `Siegebound.ControlsHelp.RawLanesAreIdentity` asserts both. Reversing Jonathan's F-1 is literally `Row.bLiteralKeyLabel = false` on one line (`.cpp:~352`), and the code says so beside it — including that the reversal also implies amending the console's own status line (a `KBD-§8` amendment, not a help edit). ⛔ I did not re-litigate it.

---

## 2. ⭐ THE ONE-TRANSLATION PROOF

`grep -rn "GetPositionalKey" Source/GitClaudeUnrealTest/` — **every hit in the new files, classified:**

| File:line | Kind |
|---|---|
| `SiegeControlsHelpWidget.cpp:490` | **CALL** — Lane C, the F-1 *reversal* branch only (a raw letter Enhanced Input never touched) |
| `SiegeControlsHelpWidget.cpp:526` | **CALL** — Lane A **fallback** only (no active mapping ⇒ nothing translated it) |
| `SiegeControlsHelpWidget.cpp:468, 500, 1284, 1475` | comments (the audit, stated at the branch) |
| `SiegeControlsHelpWidget.h:44, 49, 61, 72, 198, 199, 379, 548` | comments (the lane law) |
| `SiegePlayerController.cpp:617`, `.h:1337` | comments — the new binding states **why it does NOT call it** |

⇒ **CALL SITES ON THE LANE-A PRIMARY PATH: `0`.** Total calls added by this task: **2**, both on branches where the key is provably untranslated. ⛔ In no branch is a translation applied twice — that is the whole audit, and it is **closed structurally** (the primary path returns `AppliedKeys` verbatim and cannot reach a translation) rather than by care.

**And it is asserted, not just grepped.** `Siegebound.ControlsHelp.MappedLaneIsNeverDoubleTranslated` injects a Dvorak map containing **both hops** (`F→U` *and* `U→G`), hands the resolver the already-retargeted `U`, and asserts the answer is `U` and **is not `G`**. ⭐ A `GetPositionalKey` call added to the primary path turns that test red **on a QWERTY machine** — which is the only way this defect can ever be caught here.

**Other proofs, run:**

| Sweep (over the 3 new files) | Result |
|---|---|
| `SetInputMode` / `bShowMouseCursor` / `bEnableClickEvents` | **3 hits, all comments** ("⛔ NO SetInputMode …"). Zero calls. |
| `MapKey` / `UnmapKey` / `UnmapAll` | **1 hit, a comment** in the `.h` stating the ban. Zero calls. `KBD-§1`/`§2` untouched; ⛔ no `IMC_Hero` write. |
| `Capture()` / `EnsureSnapshot` | **0** (airlock clean). Zone A + the 552 latch untouched; no token figure (`AS-§12g`). |
| `TEXT("<single letter>")` | **0 in code.** One hit inside a test *comment* explaining the anti-pattern. |
| `Escape` | **13 hits: 11 comments + 2 `EKeys::Escape` entries in registry rows' `QwertyReferenceKeys`** (`Cards.Cancel`, `PickMode.Cancel`) — i.e. **documentation data**. ⛔ Zero handlers, zero `FReply::Handled()`, zero `NativeOnKeyDown`/`NativeOnPreviewKeyDown` overrides in either class. |

**Prose literals naming keys — the rule I applied, stated so QA can check it in one reading** (`SiegeControlsHelpWidget.cpp:28-50`): a key named in a *description* is a defect **only if that key can move**. `KBD-§4` tables the **26 letters and nothing else**, so *"right-click"*, *"the mouse wheel"*, *"Escape"* and *"left-click"* are provably layout-invariant. ⇒ **the only letter named in prose anywhere in the feature is the assistant's `Z`**, which is 704 §10.3's explicitly sanctioned `KBD-§8` exception and carries its justification and citation in-line.

---

## 3. ⛔ THE CURSOR-OWNER INTEGRATION — ONE LINE, AND A DECLARED RULING

**The composition (`SiegePlayerController.cpp:4357` → now `:4370`):**

```
bInPlacementMode || bInTargetingMode || (GroupPickStage != None) || bAssistantConsoleOpen || bWarMapOpen || bControlsHelpOpen || bUICursorHeld
```

⭐ **`|| bControlsHelpOpen` is appended and NOTHING ELSE on that line moves.** The five existing owners keep their exact shipped precedence; the overlay joins the ladder and ⛔ never re-orders it (`HELP-§5`). Like every term before it, it is `false` on every pre-existing path, so the expression evaluates **byte-identically** to before for every shipped flow.

**The widget contains no posture code at all** — `ASiegePlayerController::SetControlsHelpOpen` is the one writer and `ApplyCursorInputState()` the one applier (TASK-074's level-travel law). `OnHelpOpenChanged` exists solely so a **Close-button** close — a route the controller never sees — still releases the posture (the `UWarMapWidget::OnMapOpenChanged` precedent).

### 🚩 3.1 DECLARED RULING — THE OVERLAY **COMPOSES**, IT DOES NOT **EXCLUDE**

`CanOpenControlsHelp()` carries **one clause: `!bMatchEnded`.** ⛔ It does **not** refuse against placement / spell targeting / the group-order pick / the console / the war map, and ⛔ **no shipped guard gained a mirror clause against it** — zero shipped predicates changed.

**Why, argued rather than assumed:**
- ⭐ **The decisive fact: Jonathan's own named question for this feature is *"how to exit the command"*, and the moment a player needs that answer is DURING a pick.** A help screen that refused to open in exactly the situation it was built for is a design defect wearing a guard's clothing.
- The four-way exclusion exists because those modes each own the **LMB over the world**. This overlay hit-tests **one panel** and never the world: `BackdropBorder` is `SelfHitTestInvisible` and only `PanelBorder` is `Visible`. ⇒ every shipped cancel route keeps firing **byte-identically** underneath it, which is precisely what `HELP-§5` requires.
- That is the **`bUICursorHeld` lane**, which is already the one existing owner that composes rather than fights (*"the two owners compose rather than fight"*, `SPC:775-780`).
- The one retained clause is load-bearing for the console's own documented reason: `ApplyCursorInputState` **early-outs** while `bMatchEnded` is latched, so an overlay opened on the end screen would be a cursor owner whose posture is never applied.
- ⚠️ **The residual, stated honestly:** with the overlay open, a card key can still start placement underneath it. That is **not a soft-lock** — Tab closes the overlay, and RMB/`Escape` still cancel through the raw `PlayerTick` polls — and clicks outside the panel still reach the world. Closing that hole would require editing three shipped guards, which the spec's fence and `HELP-§5` both forbid.
- 🙋 **This is the one place I do not clone the console/war-map shape. QA/Jonathan may overrule it; it is one clause list in `CanOpenControlsHelp` either way.**

**Also declared:** `ControlsHelpWidgetZOrder = 6` (`SPC.cpp:~117`) — above the HUD (0), the map (4) and the console (5). The map sits *below* the console because a marker click must deliver into the console's box; the help overlay delivers into nothing, so it is the surface that must be legible. It steals no clicks by sitting on top — its backdrop is hit-test-transparent.

---

## 4. ⭐ THE TASK-707 SEAM CONTRACT — COMPLETE, AND IT IS THE ONLY ONE

Also pinned verbatim in the class comment §7 (`SiegeControlsHelpWidget.h:~455-480`) so it cannot drift from this file.

| # | Surface | Contract |
|---|---|---|
| **(a)** | `USiegeControlsHelpRowWidget::OnRowActivated` — `DECLARE_DELEGATE_OneParam(…, FName /*ActionId*/)`, **non-dynamic** (the `UDeckSlotEntryWidget::OnLeftClicked` precedent) | Fires on a row click. Bound by the overlay in `RefreshRows`, **exactly one bind site**; ⛔ nothing else may bind it. An **unstamped** row fires nothing. |
| **(b)** | `USiegeControlsHelpWidget::HandleRowActivated(FName)` | The **single** row-click entry point. Validates the id against the registry (⛔ an unknown id is dropped, never routed on), stores `SelectedActionId`, calls `ShowDetailForAction`, **then** broadcasts `OnRowSelected`. |
| **(c)** | `virtual void ShowDetailForAction(FName)` · `virtual void ReturnToList()` — **protected** | ⭐ **The two functions TASK-707 fills.** Today they log / no-op. `CloseHelp()` already calls `ReturnToList()` first, so 707 needs **no new close path**. |
| **(d)** | `FSiegeControlsHelpRegistry::ComposeDetailForDisplay(Row)` | ⛔ **707 reads THIS, never `Row.Detail` raw** — the raw field is empty on every row today, and reading it directly would render a **blank page** instead of the pinned `"(undocumented — TODO)"` string. |
| **(e)** | `FSiegeControlsHelpAction::Detail` (`FText`, **EMPTY on all 24 rows**) | ⭐ **707 fills it from `handoffs/TASK-704-programmer.md` §4, VERBATIM**, and owns the decision to carry §4's `file:line` citations as **C++ comments beside each string** rather than in player-facing prose (`HELP-§2`'s own instruction for an unavoidable literal). ⚠️ 707 must keep `U-5` discipline: render a tunable from the **live property** or leave the **name** — ⛔ never re-type a number (the M7.7 `"in 400"` / `AoERadius 700` lesson). |
| **(f)** | `OnRowSelected` (`BlueprintAssignable`, one `FName`) · `GetSelectedActionId()` | The outward seam. ⛔ **Deliberately NOT bound by the controller today** — a controller binding with no consumer would be a second owner of the seam. |
| **(g)** | Keys named on a detail page | ⭐ Every one derives through `ResolveRowDisplayKeys` with the **same lane audit** — the ambush page's LMB / wheel / exit keys are already registry rows (`PickMode.Confirm`, `PickMode.Resize`, `PickMode.Cancel`) with derived chips. ⛔ A hardcoded letter in the detail lane is the identical failure the row lane forbids. |
| **(h)** | Fences 707 inherits | ⛔ no `SetInputMode` / `bShowMouseCursor` and ⛔ **no second cursor owner** — the overlay's ONE registration covers both views · ⛔ no `Escape` handler · same `RebuildWidget()` order law · `/Game/UI/WBP_ControlsDetail` RESERVED + unused. |

**⚖️ Why `Detail` is empty here rather than pre-filled** (declared as deviation **D-2** below): the board fences TASK-706 at *"this task ends at the row is clickable and reports the click"*, and 707's spec assigns the detail prose. `HELP-§2` mech. 2 makes the interim a **visible** gap (the TODO string), which the law explicitly prefers to a silent one. ⛔ If QA rules the other way, the fix is data-only: fill 24 `Row.Detail` assignments.

---

## 5. SUITE DELTA — **143 → 151 (+8)**

Baseline **143** verified by counting `IMPLEMENT_*_AUTOMATION_TEST` macros across `Siegebound/Tests/*.cpp` before this task. **8 macros added, all in the one new file** (`HELP-§6`'s "⛔ one file"):

| # | Test | What it actually proves |
|---|---|---|
| 1 | `Siegebound.ControlsHelp.RegistryCoversTheActionSet` | 24 required `ActionId`s present (**by id, ⛔ never by key**) incl. `Interface.ControlsHelp` (`HELP-§4`: the menu documents its own key) · unique ids · lane/actions/keys consistency · ⭐ **exactly ONE** `bLiteralKeyLabel` row, and it is the accept key |
| 2 | ⭐⭐ `…MappedLaneIsNeverDoubleTranslated` | **The keystone.** Both Dvorak hops injected; the applied key is handed through unchanged and is **not** the twice-translated key. Also: the applied answer **outranks** the registry (survives a rebind), and `QueryAppliedKeysForRow` is null-safe and lane-fenced |
| 3 | ⭐ `…DvorakFallbackDerivesLabels` | `HELP-§6`'s named case: **CHANGES** for `F`/`T`/`C`, **HOLDS** for `M`/`A`/`Tab`. ⛔ Every label claim is made **against the accessor's own answer**, never a typed letter. Plus the `KBD-§5` null-subsystem fail-safe and a 4-key row deriving all four |
| 4 | `…RawLanesAreIdentity` | Lane B identity **proven** (the accessor agrees) · Lane C's `KBD-§8` literal survives Dvorak · ⭐ **the F-1 reversal really is one flag** (both states driven) · Lane D stays keyless even if handed applied keys |
| 5 | `…NoRowRendersBlank` | Every shipped row's one-liner / detail / category header is non-empty; every shipped one-liner is **authored, not the TODO fallback**; the TODO path driven deliberately incl. whitespace-only |
| 6 | `…EscapeIsNotClaimed` | `Escape` is in exactly the **two** cancel rows and nowhere else; the overlay's own row does not list it; reflection sweep for key-handler `UFUNction`s and **FKey members** on both classes |
| 7 | ⭐ `…RowClickSeamReportsActionId` | The **TASK-707 seam** end to end: unstamped fires nothing · stamped fires once with the id · the id resolves in the registry · unbinding is clean |
| 8 | ⭐ `…KeyChipComposition` | Pointer chip / "not bound" chip / multi-key join — all asserted by **asking the FKey for its own display name** · ⭐ **end-to-end over the real registry on both layouts: ≥1 chip CHANGES and ≥1 HOLDS**, which neither a hardcoded nor a translate-everything implementation can satisfy |

⇒ **TASK-709 should expect 151.** (TASK-707 declares the final total on top of this.)

⚠️ **Declared test deviation (D-4):** `HELP-§6` words the Dvorak case as *"via the `KBD-§7` CVar"*. That CVar gates the **1 Hz OS poll**, and the scratch subsystem never has `Initialize(FSubsystemCollectionBase&)` called — no timer, no probe, nothing for it to gate (`SiegeKeyboardLayoutTest.cpp:439-443` records the same reasoning for the same fixture). The state is driven through `SetTranslationMapForAutomationTests`, which is what **704 §1.4 specifies** and is strictly stronger: it forces both states deterministically instead of asking the machine.

---

## 6. M8 DECLARATION (verbatim)

**Adds no replicated property, no new replicated class, no new relevancy tier, no RPC.** The overlay is **client-local display only**: it reads the local player's Enhanced Input mappings and the local keyboard layout and writes nothing anywhere. The controller additions are one `bool`, one `TObjectPtr` widget slot, two soft asset paths and one input binding — none replicated, none authority-guarded because none mutate. Does NOT consume the M8 Phase-1 checkpoint gate; does NOT substitute for Jonathan's owed feedback items.

**Read-only on the world (`HELP-§5`), and greppable:** no order issued, no group cancelled, no card played, no gold moved, **no pause**. `EndPlay`'s new teardown block closes the widget and clears the flag and touches nothing else.

---

## 7. DEVIATIONS (`SC-§15`) — DECLARED, NOT SILENT

| # | Deviation | Reason |
|---|---|---|
| **D-1** | **`CanOpenControlsHelp()` carries ONE clause, and no shipped guard gained a mirror clause.** The console/war-map shape would have carried four clauses and edited three shipped guards. | §3.1 in full. Short form: refusing during a pick would make the help unavailable in exactly the situation Jonathan named it for, and the overlay never owns the LMB over the world. 🚩 Flagged for QA/Jonathan. |
| **D-2** | **`FSiegeControlsHelpAction::Detail` ships EMPTY on all 24 rows.** | The board fences 706 at "reports the click" and assigns the detail prose to 707. `HELP-§2` mech. 2 makes the interim a visible gap. Fix is data-only if overruled. §4(e). |
| **D-3** | **A `TSoftClassPtr` slot was added for BOTH reserved WBPs** (`ControlsHelpWidgetClass` on the controller, `RowWidgetClass` on the overlay) even though `HELP-§3` calls them "RESERVED and **unused**". | Without the slot, `HELP-§3`'s own promise — *"a future WBP wins with ZERO C++ change"* — cannot be true. Both resolve to **null today** (nothing is used), and both fall back to the C++ class. The `WarMapWidgetClass` precedent, which shipped the same way before `WBP_WarMap` existed. |
| **D-4** | The Dvorak case is driven through `SetTranslationMapForAutomationTests`, ⛔ not the `KBD-§7` CVar. | §5's note. 704 §1.4 specifies this seam; the CVar has nothing to gate on an un-Initialized scratch subsystem. |
| **D-5** | **704's D-1 followed:** `Cards.Play` is **one row carrying six actions and six keys**, not six rows. | 704 left it to 706 and its data supports either. Six near-identical rows would bury the fifteen commands around them; the chip composer renders all six from the live query and the coverage test keys on `ActionId`. |
| **D-6** | **F-2 resolved to its declared default:** multi-key rows render **one chip, slash-separated** (`" / "`). | 704 §8 F-2: *"Default if you say nothing: one chip, slash-separated."* ⛔ No behaviour rides on it; the separator is one named constant. |
| **D-7** | Every button is constructed **non-focusable**, via a local helper `ApplyButtonNotFocusable()` that writes `UButton::IsFocusable` under `PRAGMA_DISABLE/ENABLE_DEPRECATION_WARNINGS` — ⛔ **not** the obvious `InitIsFocusable(false)`. | ⚠️ `Tab` is Slate's own focus-next key, and **clicking a row is the whole point of this screen** — a focusable `SButton` takes keyboard focus on that click, after which `Tab` would navigate focus instead of reaching Enhanced Input. Non-focusable buttons stay fully mouse-clickable. **This is a mitigation I could not test — see U-1.** ⭐ **The route is forced, and it was MEASURED in the 5.8 headers (see D-10) — it is not a preference.** |
| **D-10** | ⭐ **A BLOCKER I CAUGHT AND FIXED PRE-COMPILE** (self-initiated compile-risk review, before this handoff was final). The first draft called `RowButton->InitIsFocusable(false)` / `CloseButton->InitIsFocusable(false)`. **`UButton::InitIsFocusable` is `protected`** (`Button.h:206`, inside the protected section opened at `:187`; its only engine caller is `UCommonButtonBase`, a UButton **subclass**) ⇒ **two hard C2248s** in TASK-709's compile. | Verified at source, and every alternative was checked before choosing: `UButton` exposes `GetIsFocusable()` (`:156`) and **no public setter** (its UPROPERTY declares `Getter` with no `Setter`, `:68`); `UWidget` has **no focusable API at all** in 5.8; `UUserWidget::SetIsFocusable` governs the **user widget**, not the inner SButton. The field `IsFocusable` **is public** (`Button.h:70`, inside the `public:` at `:36`) and merely `UE_DEPRECATED(5.2)`. ⭐ **The write is byte-equivalent to the call** — `InitIsFocusable` is nothing but `IsFocusable = InIsFocusable;` (`Button.cpp:250-252`) and `RebuildWidget` reads the field once at `Button.cpp:84`; the only requirement is "before the SWidget is built", which the construction pass already guarantees. ⭐ **And the pragma pair is the ENGINE'S OWN idiom for this exact field** — `UButton::InitIsFocusable`'s definition sits inside a `PRAGMA_DISABLE_DEPRECATION_WARNINGS` region closed at `Button.cpp:254`. ⚖️ A bespoke `UButton` subclass with a public wrapper was considered and **rejected**: it adds a fourth reflected class to a pair `HELP-§3` pins to three, and the `BindWidgetOptional` members must stay typed `UButton` or an asset-authored `WBP_ControlsHelpRow` using a plain Button would stop binding (the `HELP-§3` escape hatch). |
| **D-8** | The overlay's **backdrop is `SelfHitTestInvisible`** (only the panel absorbs clicks) — the **opposite** of `UAccountMenuWidget`'s `Visible` backdrop. | The geometry is opposite: that panel overlays a MENU whose Play/Quit must not be click-through; this one overlays a **live, unpaused match** whose cancel clicks `HELP-§5` requires to keep firing byte-identically. A full-screen absorber would eat them. |
| **D-9** | The hint line is **composed at refresh time** from the `Interface.ControlsHelp` row's own derived chip, via two string constants and a concatenation. | `HELP-§4` (*the menu documents its own key*) forbids typing it; `FString::Printf` statically requires a **TCHAR array literal**, so a `const TCHAR*` format constant fails the engine's own `static_assert` (the `UAccountMenuWidget` "Logged in as `<DisplayName>`" note records the same trap). |

---

## 8. ⛔ DECLARED — WHAT I COULD **NOT** VERIFY (`SC-§32`)

| # | Claim | Status |
|---|---|---|
| **U-1** | ⚠️⚠️ **That `Tab` actually reaches Enhanced Input while the overlay is on the viewport.** `Tab` is Slate's focus-next key. | **UNMEASURED — a PIE observation, and the one I would put first at the sitting.** The code-side mitigation is D-7 (nothing in the overlay is focusable, so Slate has nothing to Tab-navigate to and no widget grabs keyboard focus — the overlay never calls `SetKeyboardFocus`/`SetUserFocus`). ⛔ I did **not** add a key handler to "fix" it speculatively, because the only handler that could would sit next to `Escape`. **If TAB opens but will not close, this is the first suspect and the fix is a `HELP-§5`-compatible one (an explicit focus release), ⛔ never an `Escape` grab.** |
| **U-2** | **That any of it renders.** Zero compiles, zero PIE, zero pixels (fence). | `HELP-§3` / `AS-§6` A(e): verification closes on pixels or Jonathan's eyes, ⛔ never on a property readback. The `RebuildWidget()` order law is followed in **both** classes (tree + `RootWidget` set, **then** `Super`) — the one defect that renders empty while passing every readback. |
| **U-3** | **That `IA_ControlsHelp` is correctly mapped.** I did not open the editor. | TASK-705's `KBD-§2a` survivor ledger is the proof; 704 §2(b)'s byte scan is corroboration. ⭐ **And it does not gate this task:** an unresolved asset leaves Tab inert (one log line) and the overlay's own row renders the honest "(not bound)" chip with a degraded hint line that names no key. |
| **U-4** | **That `QueryKeysMappedToAction` returns the retargeted key in a live game.** Read from engine source (`EnhancedInputSubsystemInterface.h:381-384` — *"the **active** input mapping contexts"*) + the shipped apply site (`HeroCharacter.cpp:271-275`) + the in-place retarget (`SiegeKeyboardLayoutStatics.cpp:236`). | The chain is read, not observed. ⚠️ **If it turned out to return the PRISTINE key, the labels would be right on QWERTY and wrong on Dvorak — the exact failure mode this feature exists to prevent.** Cheap live check: on Jonathan's Dvorak machine, `Orders.Ambush`'s chip must read the key he actually presses. |
| **U-5** | **The 24 rows' QWERTY reference keys** are 704's, which took them from an MCP readback in a handoff. | ⭐ **Structurally harmless by design** (704 U-1): the reference column is the *fallback*, so a wrong entry cannot corrupt a label while the action resolves. |

---

## 9. WHAT TASK-708 SHOULD SCRUTINISE

1. ⭐⭐ **The two `GetPositionalKey` call sites (`.cpp:451`, `:487`) and whether either can be reached with an already-translated key.** §2 argues no: `:451` is a raw letter Enhanced Input never touched, `:487` runs only when `AppliedKeys` is empty (⇒ nothing mapped it). **This is the whole task; everything else is secondary.**
2. 🚩 **D-1 / §3.1 — the compose-not-exclude ruling.** It is the one place I do not clone the shipped console/war-map shape. It is argued, not assumed, and it is one clause list to reverse.
3. 🚩 **D-2 — `Detail` empty on all 24 rows.** Confirm the 706/707 line is where the board drew it, or rule that 706 should have pre-filled.
4. **`Escape`:** the diff-grep is the **authoritative** gate (criterion 4), not test 6 — a C++ `NativeOnKeyDown` override is not reflected and the test says so out loud. Expect **11 comments + 2 registry-data entries** and nothing else.
5. **The cursor line:** confirm `|| bControlsHelpOpen` is *appended* and no existing term moved, and that no `SetInputMode`/`bShowMouseCursor` exists in the widget (§2's sweep).
6. **`RebuildWidget()` order in BOTH classes** — the `UserWidget.cpp:1214` empty-render trap.
7. **The no-cache claim:** `RefreshRows()` destroys and rebuilds every row on every open, and no `FKey` or label `FString` is a member anywhere (test 6's reflection sweep asserts the `FKey` half). Confirm `OpenHelp()` calls `RefreshKeyboardLayout()` **before** `RefreshRows()` and that ⛔ nothing binds `OnKeyboardLayoutChanged`.
8. **`SC-§33`:** the three new statics (`CreateAndAddToViewport`/3, `QueryAppliedKeysForRow`/2, `SetRowContent`/4) and every registry function default **zero** parameters — structurally immune. The one engine call with a trailing default I rely on is `FKey::GetDisplayName`, and `false` is passed **explicitly** (`.cpp:~500`) precisely because the engine's default is `true`.
9. **U-1 (the `Tab`/Slate-focus risk)** — worth an explicit line in the 709 live-items list so Jonathan knows what to press first.
10. **Suite total 151** against 709's gate.
11. **D-10's deprecated-field write** (`ApplyButtonNotFocusable`, `SiegeControlsHelpWidget.cpp:144`; call sites `:677` and `:1097`). It is the only `UE_DEPRECATED` access this task adds, it is pragma-wrapped exactly as the engine wraps its own, and the alternatives were measured and rejected in writing. ⚖️ **If QA would rather not carry a deprecated access at all, the clean revert is to delete the helper and its two calls** — the overlay still works and the Close button still closes it; only the U-1 mitigation is lost, and U-1 was never a soft-lock because ⛔ `Escape` is untouched and the Close button is a law-required second close route.

---

## 10. STATUS

- **TASK-706 → `ready-for-qa`** (TASK-708 gates the lane; TASK-707 is unblocked and §4 is its input).
- ⛔ Nothing compiled, nothing committed, no editor touched, no asset modified, no console sentence sent.
