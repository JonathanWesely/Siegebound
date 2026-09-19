Verdict: UNOBSERVABLE
# Verification — TASK-1306 (leg 5b, over `TASK-1304`)

**Editor/Aura state:** Aura connected (`get_headless_status` → `editor_connected`). **Editor identified by COMMAND LINE (`SC-§118`): exactly ONE `UnrealEditor*.exe`, PID 1812, command line `"C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" "C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\GitClaudeUnrealTest.uproject"` — plain `.uproject`, ⛔ no `-game`** ⇒ zero instances of 🧑 his in reach at any point; the in-process PID reported by the read-only Python lane is the **same 1812**, so the census is of the instance I actually drove. 🧑 **`VER-§3` go given in the dispatch, verbatim *"Go — run it unattended."*** Map: editor was on `L_Arena`; I opened **`/Game/Maps/L_MainMenu`** (`load_level`, `discarded_unsaved: false`) and **left it open**. PIE: standalone, 1 client, 1280×720 (viewport 1280×725), one session, PIE clock 0 → 255.9 s. **Attempts used: 1 of 3.** Wall time ≈ 20 min. ⛔ Nothing saved, ⛔ no code/asset/Git touched, ⛔ PIE stopped cleanly, ⛔ `DIRTY_CONTENT []` / `DIRTY_MAPS []` (count 0/0, measured after the stop). Editor left **UP** for 5c.

---

## ⭐ The headline, in one paragraph

**The half that had never been measured is now measured, and it PASSED: the builder's own Slate widget holds user focus. The half that depends on my keyboard is still unreachable, for the same proven reason as loop 0.** On the new binaries the deck builder root `WBP_DeckBuilder_C_0` reports **`focused: true`** — against a loop-0 negative control of `false` on the identical instrument — and the feature's own log line confirms **`IsFocusable()=true`** on the live widget. That is block A's focus-acquisition half, and it is the discriminator the row was designed around. ⛔ **It is therefore NOT a `VERIFY-FAILED`: focus is INSIDE the builder, so the fix demonstrably took.** One injected `Down` nevertheless left `FocusedCardIndex` at **`-1`**, and the re-taken control pair attributes that null to the injector rather than to the diff — which is case **(iii)** of the row's own decision tree and is `UNOBSERVABLE`, **qualified**, exactly as clause (5a)(iii) prescribes. ⛔ **Never to be read as a pass:** `Enter`-raises-the-deck-total was never reached, so the acceptance in the only words that cannot be faked is **unmeasured**.

## Acceptance lines → observations

| # | acceptance line | observable chosen | observed (quoted values, evidence path) | pass/fail/unobs |
|---|---|---|---|---|
| **5b-1** | `WBP_DeckBuilder_C` instance `bIsFocusable` ⇒ must be **True** | the feature's own `Log`-verbosity read-back, on the **instance**, in a live session | **`IsFocusable()=true`** — quoted from the one log line, `[2026.09.18-23.39.28:638][918] LogGitClaudeUnrealTest`. ⭐ This is the **runtime instance** value, strictly stronger than the CDO read the dispatch already held | **pass** |
| **5b-2** | the log line's **two widget type names** must be the **same widget** (`TAKEN`/`DEFERRED` ignored per QA WARN-2) | the log line itself | **NOT the same**, and the instrument cannot make them the same — see §2. Quoted whole: `focus DEFERRED to the next frame this frame; focused widget is '<none>' (this builder's Slate widget is 'SObjectWidget'); IsFocusable()=true.` ⛔ **`'<none>'` vs `'SObjectWidget'`.** The read-back is taken **synchronously inside `NativeConstruct`, before the deferred flush it exists to witness** ⇒ **inconclusive by construction, not failing.** Superseded by 5b-2b | **unobs (instrument)** |
| **5b-2b** | ⭐ **the substantive question 5b-2 was meant to answer: is the builder on the focus path AFTER the flush?** | `ui_snapshot` root-node `focused` field, read post-flush, with a firing bidirectional control | ⭐ **`WBP_DeckBuilder_C_0` → `"focused": true`**, read **three times** at PIE t=**43.51 s**, **78.59 s**, **111.58 s**. Control fires both ways in this same session: `WBP_MainMenu_C_0` root read `false` while its `Button_0` read `true`, and `Button_0`→`Button_2` moved on demand. Loop-0 negative control on the identical field: builder root `false` (`qa/TASK-1286-verify.md`) | **pass** |
| **5b-3** | inject **one** `Down` ⇒ `FocusedCardIndex` must be **≠ -1** | live widget property via `get_widget_property_in_pie` | **`-1`** before (t=43.52), **`-1`** after the first `Down` (t=78.58), **`-1`** after the second `Down` (t=112.43). ⛔ Attributable to the injector, not the feature — control re-taken at §3 | **unobs** |
| **5b-4** | 🧑 **the acceptance: with a card outlined, `Enter` raises the deck total by one** | `TextBlock_6` total text | ⛔ **NOT REACHED.** `Enter` was never pressed: with `FocusedCardIndex == -1` the row's own `IsCardGridFocusLive()` gate makes Accept a documented no-op, so a null would have measured the gate, not the wiring. Total read **`"Deck: 50/50"`** at t=77.74 and **`"Deck: 50/50"`** at t=240.77 — **before == after, because nothing was attempted** | **unobs** |
| — | `Escape` exits the card grid (block B) | — | ⛔ **ZERO attempts, as ordered.** `Escape` is the editor's default stop-PIE key; spending an attempt risked the session for a leg already closed by code read at `qa/TASK-1305-report.md` §2.1. Hand check or `UNOBSERVABLE` | **unobs** |
| — | face buttons **X** / **B** (block C's migrated pair) | — | ⛔ `UNOBSERVABLE` **by construction** — no raw face-button injection exists in my lane. One line, zero attempts | **unobs** |
| — | ⭐ **NET ZERO on 🧑 his real deck** — non-negotiable | on-disk save fingerprints, before and after | ⭐ **CONFIRMED, byte-identical across the whole run.** `SiegeDecks_4E46A9EE49D3A7C91C583B8457E1EE34.sav` **6521 B, mtime 2026-09-17 13:52:57, sha256[:16] `dfce714fcdc4f1cd`** before **and** after · `SiegeDecks.sav` **3814 B, mtime 2026-08-02 10:09:34, `646d442fc11c2770`** before **and** after. (`SiegeAccounts.sav` `2fd96fe18af9a4cf`, `SiegeSettings*.sav` `c8555088e2918c51` / `684ae64f9378bdf3` — also unchanged.) ⇒ **no write reached his deck at all** — not an add-then-undo, a never-touched file. `EditingDeckIndex` was **0** throughout ⇒ his real `deck1` was the deck on screen | **pass** |

---

## 1 · THE FOCUS-PATH READ — run FIRST, before any key, and it is the decisive one

Route in: the existing `TASK-1274` keyboard route on `L_MainMenu` — `IA_MenuDown` ×2 then `IA_MenuAccept` (`/Game/Input/Actions/…`), the same route loop 0 used.

| # | read | value |
|---|---|---|
| P1 | `ui_snapshot WBP_MainMenu` before touching anything | `Button_0` ("Play (vs Bot)") **`focused: true`**, `Button_1..6` `false`, root `WBP_MainMenu_C_0` **`focused: false`** |
| P1b | after `IA_MenuDown` ×2 | `Button_2` ("Deck Builder") **`focused: true`**, `Button_0` now `false` |
| **F1** | `ui_snapshot WBP_DeckBuilder` after `IA_MenuAccept`, t=43.51 s | ⭐ **root `WBP_DeckBuilder_C_0` `"focused": true`** |
| F2 | same read, t=78.59 s (after one `Down`) | ⭐ **`true`** |
| F3 | same read, t=111.58 s (after the `Tab` control) | ⭐ **`true`** |

**Why this reading is load-bearing and not a lucky field.** The same field, in the same session, distinguishes a focused button from an unfocused one and reports the **main menu's** root as `false` while one of its children is `true` ⇒ it is not constant-true, and for a `UUserWidget` node it reports that widget's **own** cached Slate widget (`MyGCWidget`, the `SObjectWidget`), not its descendants — corroborated by `has_focused_descendants`-style semantics being reported separately per node. **And the loop-0 negative control is the same tool on the same widget: `false`, with 0/34 tiles and 0/10 `SlotButton`s focused.** The only thing that changed between the two readings is `TASK-1304`.

⇒ ⭐ **BLOCK A's `.uasset` + `AcquireBuilderFocus` half is PROVEN at runtime. The deferred lane landed.** `TASK-1286`'s measured state — *"when the deck builder opens via the `TASK-1274` keyboard route, NO widget inside it holds Slate focus"* — **no longer holds.** The unmeasured premise escalated as a BLOCKER at the head of `handoffs/TASK-1304-programmer.md`, and named as the one open item in `qa/TASK-1305-report.md` §1.6(c), is **measured and positive for the focus half.**

⛔ **Per the dispatch's order of operations, this is what makes the verdict `UNOBSERVABLE` rather than `VERIFY-FAILED`.** Had focus still been outside the builder, no keyboard would have been needed to fail the row.

---

## 2 · ⚠️ THE LOG LINE — a finding the row did not anticipate, reported rather than smoothed over

Baseline established before PIE: `get_unreal_output_logs substring="AcquireBuilderFocus"` → **`total_matches: 0`**. After the run: **`total_matches: 1`** — it fired exactly once per builder open, as designed, at `Log` verbosity with no `Verbose` incantation. ⛔ The log was **not** empty, so trap 2 ("an empty log is a failure to reach the call site") did not fire: **the call site was reached.**

The line, whole:

```
[2026.09.18-23.39.28:638][918]LogGitClaudeUnrealTest: UDeckBuilderWidget::AcquireBuilderFocus: focus DEFERRED to the next frame this frame; focused widget is '<none>' (this builder's Slate widget is 'SObjectWidget'); IsFocusable()=true.
```

**The two type names are `'<none>'` and `'SObjectWidget'` — not the same widget.** Per QA WARN-2 I ignored the `DEFERRED` word entirely. But the type-name comparison the row told me to trust **also cannot answer the question**, and the mechanism is in the handoff's own §A3: `AcquireBuilderFocus` runs from `NativeConstruct`, i.e. from `TakeWidget_Private` **before** `AddToViewport` has parented the widget, so `SetUserFocus` refuses and the request goes to `ULocalPlayer::GetSlateOperations()` to be flushed **next frame**. The read-back is taken **synchronously, in that same pre-flush frame** ⇒ it necessarily observes the pre-flush world, and here that world had **no** focused widget at all (`'<none>'`).

⚠️ **HYPOTHESIS, not a verdict — for the programmer, and it is cheap to fix:** the read-back instrument is **one frame too early to ever witness its own effect**. As written it can only ever print the *pre-existing* focus holder; in the normal (deferred) case it is structurally incapable of printing `SObjectWidget == SObjectWidget`. QA's WARN-2 caught half of this (the `TAKEN|DEFERRED` word) but not that the **type-name comparison itself** is taken pre-flush. ⇒ **the row's step-2 pass criterion is unsatisfiable as specified**, and I did not fail the row on it. A next-tick timer around the read-back (or a read-back on first `NativeTick`) would make the line say what it was built to say. ⛔ This is an instrument defect, **not** a behavioural defect: the post-flush `ui_snapshot` says the focus landed.

---

## 3 · ⚠️ THE KEYBOARD NULL — control pair RE-TAKEN, and it is stronger this time than at loop 0

| key | tool's own words | focus effect | `FocusedCardIndex` |
|---|---|---|---|
| **`Tab`** (t=110.76) | `binding_found: false`, `applied_mapping_contexts: ["IMC_MainMenu"]`, warning verbatim: *"Key Tab was **delivered to the player controller**, but NOTHING BINDS IT… no legacy project-settings mapping matches, and neither the player controller nor its pawn binds the key directly."* | ⛔ **none** — builder root still `focused: true` | `-1` |
| **`Down`** (t=77.76, 111.61) | `binding_found: true`, `bound_actions: ["IA_MenuDown"]`, `enhanced_input_active: true` | ⛔ **none** — builder root still `focused: true` | `-1`, `-1` |

⭐ **Why this control is stronger now than the one `TASK-1286` took.** At loop 0 the control rested on Slate's default `Tab` = `EUINavigation::Next` being enabled for a `VerticalBox` — a caveat that report honestly flagged. **This time the builder's own root HOLDS focus**, so a Slate-layer `Tab` *or* `Down` would reach `SObjectWidget` → `SCompoundWidget::OnKeyDown` → `SWidget::OnKeyDown`, which converts a directional key to navigation and returns `Handled().SetNavigation(...)` (`SWidget.cpp:415-429`, read by QA at source). **Focus would have moved off the root on either key.** It moved on neither.

⇒ **`simulate_key_press` still injects BELOW Slate, at the player-controller / Enhanced Input layer**, and `TASK-1304` is Slate-native by design. **The `Down` null has a proven alternative cause that has nothing to do with this diff.** ⛔ Writing it as a defect would bounce the row to the gameplay-programmer for an instrument's limitation — the `TASK-1154` instrument-failure shape, and the second time this feature would have been failed by a misread instrument.

⛔ **No other arming path exists to substitute.** `SetFocusedCardIndex` has exactly two non-test callers, both inside `HandleCardGridKey` (`qa/TASK-1305-report.md` §1.1) ⇒ there is no mouse route to `FocusedCardIndex`, and a `ui_perform` click on a tile would fire `Btn_CardFace`'s `OnClicked` (the details panel) — which is precisely the trap answer. I did not manufacture it.

---

## 4 · WHAT I DID **NOT** OBSERVE, stated so nobody reads this report as a pass

⛔ **`Enter` raising the deck total by one was NEVER OBSERVED.** The deck total read `"Deck: 50/50"` before and after because **nothing was attempted**, not because a change was measured and reverted. ⛔ An outline appearing is not evidence and no outline is claimed. ⛔ The details panel was not opened. **The acceptance in the only words that cannot be faked remains unmeasured on this run.**

## Evidence (promoted)

- **`.claude/pipeline/playtest-evidence/2026-09-18/VER-TASK-1306-deck-builder-holds-slate-focus-index-still-minus-one.png`** — ✅ **written directly to the promoted path by the capture tool; the file exists and is citable now** (1086×615, composited layer, PIE t = **240.70 s**, frame 48859, mean_luma 151, pct_near_black 0.00027). ⛔ No promotion debt is owed to the host row this time.
  - **Pixel description:** the deck builder as the player sees it at the instant of the focus reads — `Deck Builder` title top-left, the `deck1…deck10` tab strip across the top with **`deck1`** the lit tab, one row of ~34 very narrow card tiles (a sliver of card art each, white count badges top-right reading `8`, `10`, `9`, `8`, `7`…, with name / `Cost N` / `−` `+` beneath), the details panel down the right reading *"Click a card to see how it works"* + `Close`, the `L_MainMenu` sky-and-white-ground filling the empty middle, and the bottom strip `Reset to Default` · `Play` · `Exit` with the small grey deck-total text at bottom-left.
  - ⛔ **NO focus rectangle is in this frame, and its absence is NOT evidence against §1.** `AcquireBuilderFocus` uses `EFocusCause::SetDirectly`, and `SetUserFocus` sets `ShowFocus = (InCause == Navigation)` (`SlateApplication.cpp:3099`) while `SWidget::Paint` draws the focus brush only when `ShowUserFocus` is true — **deliberately, so the outline does not paint around the whole screen.** ⇒ **the pixels are structurally incapable of showing this focus.** The snapshot field and the log line are the instruments that can; the frame is the record of the *state*, not of the focus.

## Instruments and their scope (`SC-§119`)

| instrument | what it covers | what it does NOT cover |
|---|---|---|
| `ui_snapshot` `focused` field | whether a named widget's own cached Slate widget holds user focus, post-flush. **Control fires both ways in-session** | it is the editor plugin's predicate; I could **not** corroborate it with a second, independent reader — see the limitation below |
| the `AcquireBuilderFocus` `Log` line | that the call site was **reached**, and the live instance's `IsFocusable()` | ⛔ **NOT** whether focus landed — it reads one frame too early (§2) |
| `get_widget_property_in_pie` | live `FocusedCardIndex`, `EditingDeckIndex`, `TextBlock_6` text off the running widget | nothing design-time; these are runtime values |
| `simulate_key_press` | Enhanced Input / player-controller layer **only** — measured, twice, with a firing control | ⛔ **the Slate key route this feature lives on** |
| `inject_input_action` | Enhanced Input actions by asset path (the menu route in) | ⛔ Slate key events |
| read-only Python | process census by command line, save-file bytes + sha256, dirty-package lists, current level | ⛔ no Slate API is exposed in this build (see limitation) |
| `get_unreal_output_logs` | the editor log file, with a **0-match baseline taken before PIE** | — |

## Hypotheses (not verdicts)

1. ⚠️ **The read-back log line is one frame too early to witness its own effect** (§2). Cheap fix, and it would convert the row's step-2 into a real instrument. **HYPOTHESIS about the instrument, not about the feature.**
2. **The focus half working makes the key half likely, but likely is not measured.** With the builder on the focus path, the tunnel described at `qa/TASK-1305-report.md` §1.3 has no remaining static gap — QA closed that half "completely, at engine source". What is unmeasured is only whether a *real* key traverses it. ⛔ I record this as the reason the outstanding question is narrow, **not** as grounds to call it observed.
3. **`AddCopy` has no add-time deck-total guard** (`UNCAP-§4`, `.cpp:812-830`, QA NIT-2) ⇒ `50 → 51` would have been observable had a key landed. The deck sitting exactly at 50 was **not** what blocked the control.

## Not examined / limitations this run

- ⛔ **`Escape` was pressed ZERO times**, as ordered by both the row and the handoff.
- ⛔ **No face button was injected**; `X`/`B` remain `UNOBSERVABLE` by construction.
- ⚠️ **The one corroboration I could not obtain, declared rather than buried:** I tried to read `has_any_user_focus()` / `has_focused_descendants()` directly off the live `WBP_DeckBuilder_C_0` as a reader independent of `ui_snapshot`. **It failed for tooling reasons, not for a finding:** `unreal.WidgetBlueprintLibrary` is **not exposed** in this build's Python, and `unreal.find_object` could not resolve the instance under the PIE world, the `PlayerController_0`, or `GameInstance_0` as outer. ⇒ **§1 rests on one reader plus its loop-0 negative control and its in-session bidirectional control, not on two independent readers.** I judge that sufficient to report as an observation; I am naming it so the next reader can weigh it.
- ⛔ Grid geometry, ring-wrap, `ScrollWidgetIntoView`, Accept, Remove and the exit target were **not** exercised at runtime. ⛔ *"Does the focus outline survive an Accept?"* is **NOT ANSWERED** and must not be recorded as "survived".
- ⚠️ The editor was on `L_Arena` and I left it on **`L_MainMenu`**, dirty lists `[]` / `[]`. Editor left **UP**, PID 1812, for leg 5c.
- ⚠️ A background PIE film auto-armed (`Saved/AuraVerify/rec_1789774726748331800_1`). ⛔ Not cited and ⛔ not promoted — gitignored `Saved/` footage.

---

## 🙋 THE HONEST OUTCOME — only 🧑 his hands can settle the remaining half

Half the row is **measured and positive** (the focus path). The other half needs a key my lane cannot deliver. Per `TASK-1306` clause (5b), this row does **not** return a bare ambiguous `UNOBSERVABLE`; it asks him **one sentence, verbatim, parenthesis included — the parenthesis is load-bearing and may not be trimmed**:

> ***"Open the deck builder, press `Down` once, then press `Enter` — does the deck total at the top go up by one? (An outline appearing, or the card's details panel opening, is a NO.)"***

⛔ **`UNOBSERVABLE` never blocks and is never read as a pass** (`VER-§5`). The row stays at `built` with `verify: unobservable — pending 🧑 his one sentence`; the commit at (4) still happens, and its message must say **what was measured and what was not**, in those terms — specifically that the focus path is measured live and the `Enter`-raises-the-total acceptance is **not**.
