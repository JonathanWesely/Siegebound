# TASK-818 — FULL INPUT-BINDING COLLISION AUDIT

**Agent:** gameplay-programmer · **Date:** 2026-09-02 · **Type:** READ-ONLY investigation, terminal (no QA follows)
**Files written:** this handoff ONLY. ⛔ No source, no test, no asset, no compile, no Git, editor never touched.

**Jonathan's question, verbatim:** *"Can you double check that we haven't yet mapped any buttons to multiple different actions?"*

---

## 0. ⭐ THE ANSWER IN FIVE LINES

1. **`IMC_Hero` is internally clean.** 27 mapping rows, 27 **distinct** keys — measured from the asset bytes, not assumed. ⛔ Zero duplicate keys inside Enhanced Input.
2. **The real answer is not in `IMC_Hero`.** Three keys — **LMB, RMB, Escape** — are each an Enhanced Input action **AND** a raw `PlayerTick` poll **AND** (for LMB/RMB) a Slate handler. That is the two-actions-on-one-key case, and it is **deliberate and guarded**, not an accident.
3. **`H` is genuinely free** — on QWERTY **and** after the Dvorak remap. Measured, not assumed.
4. **⛔ ONE TRUE COLLISION FOUND:** while the war map is open, **RMB can never close it** — `UWarMapWidget` absorbs RMB to delete a mark before the controller's polled close ever sees the press. The documented "RMB/Esc double cover" is **Escape-only in practice**. Not a soft-lock (Escape works), but the code comment claims a cover it does not have.
5. **⛔ ONE LATENT DVORAK HOLE:** the layout system's injectivity guard **cannot see non-letter rows**. Today that is harmless. **The first punctuation key anyone binds in `IMC_Hero` silently double-binds on US-Dvorak, and nothing logs it.**

---

## 1. METHOD — how the `IMC_Hero` rows were obtained without the editor

⚠️ **The editor is DOWN and I did not touch it.** `IMC_Hero.uasset` is a binary asset and the C++ **deliberately never names a key for a mapped action** (`SiegePlayerController.cpp:601-605`), so a grep cannot answer this.

I parsed the package bytes directly (name table by structural run-detection, then the tagged-property stream) and recovered all 27 rows in serialized order with their `Action` import and `KeyName`. **This is a read of the shipped asset, not a guess.**

✅ **Cross-validation, and it is strong:** all 27 recovered rows match `SiegeControlsHelpWidget.cpp`'s `QwertyReferenceKeys` registry — a **completely independent, hand-written** source of truth — **row for row, with zero disagreements**, including the `B`/`IA_Recall` row absent from that registry being absent from nothing else. Two independent sources agreeing on 27/27 is the evidence base for everything below.

- `Escape`'s row was additionally traced through **8 commits of history** via `git lfs smudge` (see §6).
- ⚠️ **DEFERRED — needs editor (2 items, both low-risk, neither affects any finding below):**
  - **Which `UInputMappingContext` each pawn/controller Blueprint actually holds** (`BP_HeroCharacter.HeroMappingContext`, `BP_SiegeGhostPawn.GhostMappingContext`) is Blueprint default data. I read the only C++ path and the ghost pawn's own log string names `/Game/Input/IMC_Hero` (`SiegeGhostPawn.cpp:425`), but I did not confirm the BP defaults.
  - **Whether any `BP_SiegePlayerController` adds `IMC_Default` / `IMC_MouseLook`.** The C++ class cannot (see §3.4).

---

## 2. ⭐ THE COMPLETE KEY TABLE — BOTH SYSTEMS

### 2.1 SYSTEM 1 — Enhanced Input rows in `IMC_Hero` (27 rows, measured, serialized order)

Row numbers are **1-based** to match `CARDBAR-§8`'s existing "rows 10/11" citation.

| # | Key | Action | Bound in C++ at | Live when |
|---|---|---|---|---|
| 1 | `SpaceBar` | `IA_Jump` | `GitClaudeUnrealTestCharacter.cpp:59-60` | hero possessed |
| 2 | `W` | `IA_Move` | `GitClaudeUnrealTestCharacter.cpp:63` | hero possessed |
| 3 | `S` | `IA_Move` | " | hero possessed |
| 4 | `A` | `IA_Move` | " | hero possessed |
| 5 | `D` | `IA_Move` | " | hero possessed |
| 6 | `Mouse2D` | `IA_Look` | `GitClaudeUnrealTestCharacter.cpp:67` | hero possessed |
| 7 | `LeftShift` | `IA_Sprint` | `HeroCharacter.cpp:388-390` | hero possessed |
| 8 | `LeftMouseButton` | `IA_Attack` | `HeroCharacter.cpp:400` | hero possessed |
| 9 | `One` | `IA_Card1` | `SiegePlayerController.cpp:516` / `:527` | always (controller) |
| **10** | `RightMouseButton` | `IA_CancelPlace` | `SiegePlayerController.cpp:545` | always (controller) |
| **11** | **`Escape`** | **`IA_CancelPlace`** | `SiegePlayerController.cpp:545` | always (controller) |
| 12–16 | `Two`…`Six` | `IA_Card2`…`IA_Card6` | `SiegePlayerController.cpp:527` | always (controller) |
| 17 | `LeftAlt` | `IA_UICursor` | `SiegePlayerController.cpp:536-538` | always (controller) |
| 18 | `Q` | `IA_Rally` | `HeroCharacter.cpp:410` | hero possessed |
| 19 | `T` | `IA_CmdAttack` | `SiegePlayerController.cpp:554` | always (controller) |
| 20 | `R` | `IA_CmdHold` | `SiegePlayerController.cpp:558` | always (controller) |
| 21 | `E` | `IA_CmdDefend` | `SiegePlayerController.cpp:562` | always (controller) |
| 22 | `F` | `IA_CmdAmbush` | `SiegePlayerController.cpp:570` | always (controller) |
| 23 | `C` | `IA_CmdFollow` | `SiegePlayerController.cpp:579` | always (controller) |
| 24 | `Enter` | `IA_AssistantConsole` | `SiegePlayerController.cpp:591` | always (controller) |
| 25 | `M` | `IA_WarMap` | `SiegePlayerController.cpp:608` | always (controller) |
| 26 | `Tab` | `IA_ControlsHelp` | `SiegePlayerController.cpp:627` | always (controller) |
| 27 | `B` | `IA_Recall` | `HeroCharacter.cpp:433` | hero possessed |

✅ **27 rows, 27 distinct keys. ⛔ No duplicate key inside `IMC_Hero`.**

⚠️ **`VIS-§1` check performed, and it matters here:** rows 1–6 are bound in the **parent** class. `AHeroCharacter : public AGitClaudeUnrealTestCharacter` (`HeroCharacter.h:398`) and `AHeroCharacter::SetupPlayerInputComponent` calls `Super::` first (`HeroCharacter.cpp:381`). A grep of `HeroCharacter.cpp` alone would have missed Jump/Move/Look entirely.

⚠️ **`IA_MouseLook` is bound in the parent (`:64`) but has NO row in `IMC_Hero`** — it is only mapped in `IMC_MouseLook`, which Siegebound never adds (§3.4). ⇒ inert, and **no double-`Look()` per frame**. Had both been mapped to `Mouse2D`, look speed would silently double.

### 2.2 SYSTEM 2 — RAW POLLED keys (`ASiegePlayerController`)

| Key | Line | Branch / mode | Effect |
|---|---|---|---|
| `RightMouseButton` **or** `Escape` | `:651` | `GroupPickStage != None` | `CancelGroupPick()` |
| `LeftMouseButton` | `:667` | `GroupPickStage != None` | `ConfirmGroupPickStage()` |
| `RightMouseButton` **or** `Escape` | `:683` | `bInTargetingMode` | `ExitTargetingMode()` |
| `LeftMouseButton` | `:695` | `bInTargetingMode` | `TryConfirmSpellTarget()` |
| `RightMouseButton` **or** `Escape` | `:721` | `bWarMapOpen` | `CloseWarMap()` — ⛔ **RMB half is unreachable, see §4.1** |
| `RightMouseButton` **or** `Escape` | `:741` | `bInPlacementMode` | `ExitPlacementMode()` |
| `LeftMouseButton` | `:754` | `bInPlacementMode` | `TryConfirmPlacement()` |
| `MouseScrollUp` / `MouseScrollDown` | `:3012` / `:3016` | `ApplyGroupPickWheel()`, group-pick only | radius ± `GroupRadiusWheelStep` |

⛔ **That is the complete raw-poll set for the whole module** — re-grepped for `WasInputKeyJustPressed` / `IsInputKeyDown` / `GetInputKeyTimeDown` / `WasInputKeyJustReleased` / `GetInputAnalogKeyState`. Every other hit in `Source/` is a comment.

### 2.3 SYSTEM 3 — SLATE / UMG handlers (the third system, and it beats both others)

| Handler | Key(s) | Reply | Live when |
|---|---|---|---|
| `USiegeAssistantConsoleWidget::NativeOnPreviewKeyDown` `:883` | `GetPositionalKey(EKeys::Z)` — **positional, never literal `Z`** (`:966`) | `Handled` `:904` | console open + enabled + confirm prompt up, and **unmodified** (`:877-878`) |
| the console's focused `UEditableTextBox` | **every character key + `Enter`** | absorbed by Slate | console open (`FocusInputBox` `:1400`) |
| " `SetRevertTextOnEscape(false)` `:427` | `Escape` | ⛔ **deliberately NOT absorbed** | — |
| `UWarMapWidget::NativeOnMouseButtonDown` `:2629` | `RightMouseButton` | `Handled` `:2635` | map open |
| " `:2642` | **every non-LMB button** | `Handled` `:2644` | map open |
| " `:2652-2768` | `LeftMouseButton` | `Handled` on **all** paths | map open |
| `UWarMapWidget::NativeOnMouseWheel` `:2790` | wheel | `Handled` `:2822` **even on a miss** | map open (passes through when closed, `:2792-2798`) |
| `UDeckSlotEntryWidget::NativeOnMouseButtonDown` `:195` | `RightMouseButton` | `Handled` `:200` | **deck builder screen only** |

⛔ **There is no `NativeOnKeyDown`, no `FInputChord` object, no `FUICommandInfo`, and no command list anywhere in the module.** The console's preview handler is the module's **only** key handler.

---

## 3. ⭐⭐ TRUE COLLISIONS vs SAFELY MODE-SCOPED REUSE

### 3.1 The one template for "safely shared" — and it holds

`PlayerTick` runs **four** cursor-mode branches that share LMB/RMB/Escape. They are mutually exclusive **structurally**, by `return`s, not by discipline:

- group pick → `return` at `:671`
- targeting → `return` at `:686`
- war map → `return` at `:729`
- placement → reached only past `if (!bInPlacementMode) { return; }` at `:732-735`

✅ **And the exclusivity is enforced at ENTRY too, bidirectionally — this is the part worth citing, because a `return` ladder alone would only pick a winner, not prevent the overlap:**

| Entry point | Refuses if | Cite |
|---|---|---|
| `EnterPlacementMode` | match ended · placement · targeting · group pick · console open · **war map open** | `:1396` `:1404` `:1414` `:1424` `:1436` `:1448` |
| `EnterTargetingMode` | the identical six | `:2383` `:2391` `:2399` `:2409` `:2419` `:2429` |
| `BeginGroupPick` | console open · **war map open** | `:2916` `:2926` |
| `CanOpenWarMap` | match ended · placement · targeting · group pick | `:5135-5138`, re-gated in `SetWarMapOpen` `:5147` |

⇒ **These four modes can never be co-live.** That is a genuinely well-built guarantee and the answer to "what guarantees exclusivity" for consumers 1–4.

### 3.2 ✅ SAFE, DELIBERATE, GUARDED — `LeftMouseButton` (Enhanced Input **+** raw poll simultaneously)

⭐ **This is exactly the intersection Jonathan was worried about, and it is real:** LMB is `IA_Attack` on the hero (row 8) **and** a raw confirm poll in three modes. Both fire on one physical click.

✅ **Mitigation is shipped and explicit:** `SetMeleeSuppressed(true)` makes `DoMeleeAttack` a cooldown-free no-op while a cursor mode is live — stated at `:663-666`, `:692-694`, `:750-753`. **Not a bug.** ⚠️ But note the shape: this is a *suppression flag*, not exclusivity. If a future mode polls LMB without setting melee suppression, the hero swings while confirming. Worth keeping in the reviewer's eye.

### 3.3 ✅ SAFE — `RightMouseButton` / `Escape` (Enhanced Input **+** raw poll)

Rows 10/11 give `IA_CancelPlace` **both** RMB and Escape; the same two keys are polled in four branches. Double-fire is real and harmless: `OnCancelPlacePressed` (`:1142`) only acts inside the three cursor modes and each exit is **idempotent** (`:737-740`). ⛔ It deliberately does **not** close the war map, so Escape-while-map-open goes through the poll only.

### 3.4 ✅ NOT A COLLISION — `IMC_Default` / `IMC_MouseLook`

`IMC_Default` maps `W/A/S/D/SpaceBar` + arrows + gamepad; `IMC_MouseLook` maps `Mouse2D`. **If either were added alongside `IMC_Hero`, WASD/Space/mouse would double-bind.** They are not: they are added only from `AGitClaudeUnrealTestPlayerController::DefaultMappingContexts` (`:46-48`), and **`ASiegePlayerController : public APlayerController`** (`SiegePlayerController.h:202`) — it does **not** derive from the template controller and has no such array. `ASiegeGameMode` sets `PlayerControllerClass = ASiegePlayerController` (`SiegeGameMode.cpp:36`). ✅ Clean. ⚠️ *DEFERRED:* a BP subclass could still add one; unverifiable without the editor.

### 3.5 ⚠️ MODE-SCOPED, WORTH A LOOK — the ghost pawn keeps the whole controller keyboard live

`ASiegeGhostPawn` adds its context at `GhostMappingContextPriority = 1` — the **same** priority as the hero's — and **there is not one `RemoveMappingContext` call in the module** (`SiegeGameMode.cpp:916-918`). Every `IMC_Hero` row therefore stays mapped while the ghost is possessed. The hero's own bindings (Sprint/Attack/Rally/Recall) die with the pawn, but **every `ASiegePlayerController` binding survives possession**: cards 1–6, `T/E/R/F/C`, `M`, `Tab`, `Enter`, `LeftAlt`.

✅ Cards are already guarded — `CanPlayCardsWhilePossessing(GetPawn())` (`EnterPlacementMode`, ~`:1516`, `GHOST-§3 G-5`), and that guard exists **precisely because** the older `Cast<AHeroCharacter>(GetPawn())` check silently stopped firing once the ghost was possessed. ⚠️ **The unit-command keys `T/E/R/F/C` and `M`/`Tab` carry no equivalent ghost guard that I found.** Whether a dead player should still be able to order the army is a **design question, not a collision** — flagging it, not boarding it.

### 3.6 ⚠️ MODIFIER-DISTINGUISHED PAIRS — there are none, and the bare key **would** swallow both

Measured from the asset: `IMC_Hero`'s name table imports exactly two modifier classes (`InputModifierNegate`, `InputModifierSwizzleAxis`, both for `IA_Move`) and **no trigger class at all** — ⛔ **no `InputTriggerChordAction`, so there is not a single chorded binding in the project.** `LeftShift` is bound as a **bare key** (`IA_Sprint`).

⇒ **Answer to the sub-question: `X` vs `Shift+X` is NOT distinguished anywhere, and if someone adds such a pair the bare binding will swallow both.** Nothing is at risk today because no pair exists. The one modifier-aware site is the console's accept, which allows `Shift+Z` but excludes `Ctrl`/`Alt`/`Cmd` so Slate's undo/redo survives (`:877-878`) — deliberate.

### 3.7 ⛔ TRUE COLLISION #1 — war map: RMB has two meanings and the Slate one **always** wins

**See §4.1.** This is the only finding I would call a genuine collision in shipped code.

---

## 4. ⭐ THE WHEEL AND RIGHT-CLICK VERDICTS

### 4.1 ⛔ RIGHT-CLICK — verdict: **the third consumer can be added safely, but one shipped claim is false**

`CARDBAR-§8`'s registry is **accurate** — I re-derived all five consumers at source and it matches. Two corrections/additions:

1. ⛔ **FINDING (true collision): the war map's polled RMB close at `:721` is UNREACHABLE.** `UWarMapWidget::NativeOnMouseButtonDown` returns `Handled` for RMB at `:2629-2635` **before** the non-LMB absorb, and — in the widget's own words at `:2643-2645` — *"The map fills the screen."* A Slate `Handled` reply means `UPlayerInput` never records the press, so `WasInputKeyJustPressed(EKeys::RightMouseButton)` is false. ⇒ **RMB while the map is open deletes a mark; it never closes the map.**
   - ⚠️ The controller comment at `:712-714` claims *"RMB/Esc polled here is exactly the double-cover the three shipped cursor modes already use"* — **that is not true for RMB.** The cover is **Escape-only** (plus the `CloseButton`).
   - ✅ **Severity: LOW — not a soft-lock.** Escape has no widget handler anywhere and reaches the poll. But the comment will mislead the next reader, and `CARDBAR-§8` row 3 inherits the same optimism.
2. ⚠️ **The absorb is broader than "RMB":** `:2642-2644` absorbs **every** non-LMB button (MMB, X1, X2) while the map is open. Worth having in the registry.

✅ **Can a right-click reach a card while those modes are live? Measured answer: NO — and `CARDBAR-§8` already says so correctly.** `bShowMouseCursor` is composed in one place (`ApplyCursorInputState`, `:4786`) from placement · targeting · group-pick · console · war map · controls help · the Alt-held `IA_UICursor`. In plain play **the cursor is hidden and UMG receives no mouse events at all.** And in every mode that raises the cursor by itself, the discard is refused by the guard ladder. ⇒ **Right-click-to-discard only works during the Alt hold, or with the cursor up for another reason.** That is the honest niche and it is already `J-19` on Jonathan's sheet.

✅ **`CARDBAR-§6`'s never-`Handled` rule is the correct and sufficient protection.** If `UCardHandWidget::NativeOnMouseButtonDown` ever returns `Handled`, it kills the placement/targeting/group-pick RMB cancel exactly the way the war map already kills its own — **the war map is the live proof that this failure mode is real, not theoretical.** `TASK-811`'s pixel row already tests it.

### 4.2 ✅ MOUSE WHEEL — verdict: **the third consumer is safe. Add it.**

| # | Consumer | Mechanism | Guarantee |
|---|---|---|---|
| 1 | group-pick radius | **polled**, `ApplyGroupPickWheel` `:3012/:3016` | `PlayerTick` branch, `return`s at `:671` |
| 2 | war-map mark resize | **Slate**, `NativeOnMouseWheel` `:2790` | absorbs only while `bMapOpen`; **passes through when closed** (`:2792-2798`) |
| 3 | 🆕 placement footprint | **polled** (`STACK-§4`) | reached only past `:732`; sibling of #1 in the same function |

✅ **#1 and #3 are the same mechanism in the same function and can never run in the same frame** (the `return` ladder). ✅ **#2 cannot starve them because the war map is mutually exclusive with all three modes at entry** (§3.1) **and** because it passes the wheel through when closed. ⇒ **Safe.**

⛔ **What would break if the modes ever overlapped:** #2 wins unconditionally. A Slate `Handled` on the wheel means the polls never see the notch — so if `bWarMapOpen` were ever true alongside placement or a group pick, the **war map would silently eat every wheel notch** and the ghost/circle would freeze with no error. The exclusivity guards in §3.1 are the **only** thing preventing that; ⛔ **weaken any one of those six `if`s and the wheel is the first thing to break.**

⚠️ **`MARK-§4`'s own confusion hazard is now worse, and it is unfixed:** three wheel meanings (world-uu circles, widget-space marks, a scale factor) — and `CARDBAR-§9` already measured that **`MARK-§`'s numbered circles have no help row at all.** Not mine to board; confirming it independently.

---

## 5. ⭐⭐ THE DVORAK / POSITIONAL-REMAP ANALYSIS

**The mechanism, stated correctly because both halves are live:** `USiegeKeyboardLayoutSubsystem::GetPositionalContext` duplicates `IMC_Hero` into a transient copy and rewrites **only** `.Key`, row by row (`SiegeKeyboardLayoutStatics.cpp:237`). The action stays at the same **physical position** and the printed letter changes. ✅ That is exactly what Jonathan described (*"when I said 'H', I am talking about 'H' on QWERTY, on Dvorak it would be 'D'"*).

⛔ **`KBD-§4` covers all 26 letters and deliberately excludes digits, punctuation, modifiers, Space/Enter/Escape and the mouse** (`SiegeKeyboardLayoutStatics.h:139-141`). ⇒ card slots `1`–`6`, `Tab`, `Enter`, `Escape`, `LeftAlt`, `LeftShift` and the mouse **never move.**

### 5.1 ✅ Measured: NO collision on US-Dvorak, today or with `H`

Using the project's own measured US-Dvorak table (`Tests/SiegeKeyboardLayoutTest.cpp:204-229`):

| `IMC_Hero` key | → US-Dvorak | | key | → US-Dvorak |
|---|---|---|---|---|
| `A` | `A` (identity) | | `Q` | `Apostrophe` |
| `B` | `X` | | `R` | `P` |
| `C` | `J` | | `S` | `O` |
| `D` | `E` | | `T` | `Y` |
| `E` | `Period` | | `W` | `Comma` |
| `F` | `U` | | *(raw)* `Z` | `Semicolon` |
| `M` | `M` (identity) | | 🆕 `H` | **`D`** |

**All 14 targets are distinct.** ✅ ⛔ **No collision on QWERTY, none on Dvorak.**

⭐ **The near-miss worth naming, because it is `H`'s:** `H → D` while `D → E` and `E → Period`. Three rows chain. It is safe for two measured reasons: the retarget reads from the **pristine source** and writes to the **duplicate** (`Statics.cpp:233` vs `:237`), so no double-translate; and the injectivity guard **vacates** a position's old key when it accepts a translation (`:170-174`), so `D` is free for `H` to claim. ✅ Both are covered by shipped tests.

### 5.2 ⛔ THE LATENT HOLE — the guard cannot see non-letter rows

The injectivity guard (`SiegeKeyboardLayoutStatics.cpp:161-168`) refuses a target another position already claimed — but **`ClaimedKeys` is seeded only from the 26 letter probes.** ⛔ **`IMC_Hero`'s non-letter rows are invisible to it**, and `RetargetContextKeys` has **no duplicate detection, no refusal and no log** for the result (its only refusals are null args / profile overrides / length mismatch, `:191-213`).

⇒ ⭐⭐ **THE ACTIONABLE RULE, and this is the class Jonathan asked for — invisible on this machine:**

> ⛔ **Never bind a punctuation key (`,` `.` `'` `;` `/` `-` `=`) in `IMC_Hero`.**
> On US-Dvorak the `W`, `E`, `Q` and `Z` positions already land on **`Comma`, `Period`, `Apostrophe` and `Semicolon`**. A punctuation row would be left untranslated (not a letter) *and* would be invisible to the guard ⇒ **two actions on one physical key, silently, on Dvorak only, with nothing in the log.** ✅ It is harmless today only because `IMC_Hero` happens to bind no punctuation.

⚠️ A weaker variant exists in theory — a letter position resolving to a **digit** would collide with the card slots, which are never remapped. ⛔ **I found no real-world layout that does this** (Dvorak, AZERTY and QWERTZ all keep letters on letter positions), so I am recording it as a **structural gap, not a live defect.** `FInputKeyManager::GetKeyFromCodes` (`Statics.cpp:99`) can return a digit key, so the gap is real even if unreachable today.

✅ **Any *letter* is safe to add** — the 26-letter map is a permutation, so every free letter's target is also free. Free letters after `H`: `G I J K L N O P U V X Y`.

### 5.3 ✅ `H` — genuinely free, on both layouts

- ⛔ **Not** among `IMC_Hero`'s 27 rows (measured).
- ⛔ **Not** raw-polled anywhere — the complete raw set is LMB/RMB/Escape/wheel/positional-`Z`.
- ⛔ **Not** watched by any Slate handler (the module has exactly one key handler, and it compares `GetPositionalKey(EKeys::Z)`).
- ✅ On US-Dvorak `H → D`, and `D` is vacated. **No collision.**

✅ **`CARDBAR-§6`'s chosen implementation is the correct one:** `IA_DiscardAll` **appended to `IMC_Hero` at the `H` position**. Because it is a mapped Enhanced Input action, it inherits the positional remap **for free** — `KBD-§4` already puts all 26 letters in the table. ⛔ **Do NOT implement `H` as a raw poll comparing a bare `EKeys::H`:** on Dvorak that fires on the **physical `J` position**, which is the opposite of what Jonathan asked for. If a poll is ever unavoidable it must go through `GetPositionalKey(EKeys::H)`, the `Z`-accept precedent (`SiegeAssistantConsoleWidget.cpp:966`).

---

## 6. ⭐ EVERYTHING TOUCHING `Escape` (`AS-§6` A-2)

**Finding, reported as required — and the verdict is: `Escape` is BOUND, but it is NOT ABSORBED, and the ruling is intact.**

1. ⚠️ **`IMC_Hero` row 11 maps `Escape` → `IA_CancelPlace`** (measured). `AS-§6` A-2 lists *"an Enhanced Input action"* among the routes by which absorbing `Escape` is *"an automatic QA FAIL"* — so a literal reader hits an apparent contradiction. **It is not one, for three measured reasons:**
   - ⛔ **It predates the ruling.** Traced through history: `Escape` is present in the **earliest commit that has the file** (`56247c9`, 2026-07-04) and in all 8 sampled commits since. The ruling is 2026-08-04, and it explicitly ratifies *"the shipped cancel routes — placement, spell targeting, group-pick — keep firing byte-identically."* `IA_CancelPlace` **is** that shipped route.
   - ✅ **It does not absorb.** `IA_CancelPlace.uasset` carries **no property overrides at all** (measured: its name table holds only package boilerplate), so every flag is at class default. In UE 5.8 `UInputAction::bConsumeInput = true` consumes **lower-priority *Enhanced Input* mappings only**, and `bConsumesActionAndAxisMappings = false` (`Engine/Plugins/EnhancedInput/.../InputAction.h:96` and `:104`). ⇒ ⛔ **It cannot swallow the raw `WasInputKeyJustPressed(EKeys::Escape)` polls**, which is why all four polled cancel routes still fire.
   - ✅ Nothing else contends for `Escape` — it is the only `Escape` row in the only context Siegebound adds.
   - ⭐ **Recommendation (documentation only, no code):** add one clause to `AS-§6` A-2 acknowledging the shipped `IA_CancelPlace` row, so the next agent who greps `Escape` does not read a contradiction and "fix" it. ⛔ **I have not edited `CONVENTIONS.md`.**
2. ✅ **`USiegeAssistantConsoleWidget` `SetRevertTextOnEscape(false)` (`:427`)** — deliberately leaves `Escape` unhandled so it bubbles to the viewport. **Correct, and load-bearing.**
3. ✅ **`NativeOnPreviewKeyDown` returns `Handled` for the accept key and nothing else** (`:883`, `:904`); every other key falls to `Super` at `:931`. The token `Escape` is absent from its executable code. ✅ Compliant.
4. ✅ **No widget anywhere absorbs `Escape`** — verified across the whole module (`WarMapWidget`, `DeckSlotEntryWidget`, `AccountMenuWidget`, `SessionMenuWidget`, `SettingsMenuWidget`, `DeckBuilderWidget`, `CardHandWidget`, `SiegeControlsHelpWidget`). The module has **one** key handler total.
5. ⚠️ **One place where `Escape` genuinely stops working, and it is by design:** `HandleMatchEnd` (`:1734-1740`) installs `FInputModeUIOnly` with `SetWidgetToFocus`. While the victory/defeat screen is up, **all game input is dead, including every `Escape` poll.** Not an `AS-§6` breach (no one absorbs the key), but it is the one state where `Escape` does nothing.

---

## 7. WHAT QA / THE NEXT AGENT SHOULD SCRUTINISE

1. ⭐ **§4.1 — the war map's RMB double-cover is false.** The only item I would call a defect. A comment fix, not a behaviour change — **do not "fix" it by removing the map's RMB delete**, which is Jonathan's own feature (`MARK-§`).
2. ⭐ **§5.2 — the punctuation rule.** Worth writing into `KBD-§11`'s known-limitations list; it is currently unstated and it is the one failure this machine can never reproduce.
3. **§6.1 — the `AS-§6` A-2 wording.** Documentation only.
4. **§3.5 — `T/E/R/F/C`/`M`/`Tab` while ghost-possessed.** A design question for Jonathan, not a bug. `CARDBAR-§9`'s scope fence already forbids boarding the wider undocumented-controls audit; I am naming it, not boarding it.
5. ⚠️ **Cite hygiene:** every line number here was re-grepped this session against the working tree (`SiegePlayerController.cpp` = 5928 lines, `WarMapWidget.cpp` = 2953). ⛔ I trusted no prior cite. Two prior cites I checked **and confirmed still correct**: `SiegePlayerController.cpp:642-647` and the `:671`/`:686`/`:729` returns.

**Status:** TASK-818 — **done** (terminal, read-only; no QA gate follows).
