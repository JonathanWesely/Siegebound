# TASK-704 — [HELP-1] THE ACTION REGISTRY — programmer handoff

**Agent:** gameplay-programmer · **Date:** 2026-08-29 · **Law:** `HELP-§1` / `HELP-§2` (cited, not restated) · `AS-§6 A-2` · `KBD-§0`/`§1`/`§2`/`§4`/`§5`/`§7`/`§8`
**Consumers:** TASK-706 (rows + live key labels) · TASK-707 (detail pages) · TASK-708 (QA gate)

## 0. WHAT THIS FILE IS, AND WHAT IT IS NOT

This is the **input** to 706/707. It is a **data specification + a citation ledger**, not code.

- ⛔ **ZERO C++ was written.** ⛔ No editor, no MCP, no compile, no Git, no board edit beyond this task's own status line.
- ✅ **Files written: exactly one** — this handoff. (`names` block compliance.)
- ⭐ **Every factual sentence in §4's detail text carries a `file:line` citation to code I personally opened and read this session.** Where I could not verify something at source, it is in **§7 UNVERIFIABLE**, declared, not invented.
- ⚠️ Where the **shipped code disagrees with the board, the GDD or my expectation, the CODE WINS and I say so** — see §7 rows U-1…U-6 and §8 FOR-JONATHAN F-1/F-2.

**Inventory: 24 actions.** Full list in §4.

---

## 1. ⛔⛔ THE LANE LAW — THE DOUBLE-TRANSLATE AUDIT (`HELP-§1`, `SiegePlayerController.h:1222-1226`)

> **The trap, restated as a mechanism rather than a warning:** `USiegeKeyboardLayoutSubsystem` rewrites the applied `IMC_Hero` duplicate's `.Key` fields **wholesale** — `Target->GetMapping(Index).Key = TranslatedKey ? *TranslatedKey : SourceKey;` (`SiegeKeyboardLayoutStatics.cpp:236`). ⇒ **an Enhanced-Input-mapped action's live key is ALREADY translated.** Calling `GetPositionalKey` on it applies the map a **second** time. On QWERTY the map is empty, so the two are indistinguishable; **on Dvorak `F` → `U` → `G`, and the help screen teaches the wrong key.**

### 1.1 The four lanes, and the rule for each

| Lane | What it is | Members | ⇒ `GetPositionalKey`? | Authority |
|---|---|---|---|---|
| **A — Mapped** | The key lives in `IMC_Hero`; the subsystem retargets the applied duplicate | 18 of 24 rows | ⛔ **NEVER.** The live key is already translated. | `SiegePlayerController.h:1222-1226` · `SiegeKeyboardLayoutStatics.cpp:229-236` · `HeroCharacter.cpp:267-275` |
| **B — Raw, non-letter** | `WasInputKeyJustPressed(EKeys::…)` on a **mouse button / Escape / wheel** | LMB · RMB · Escape · MouseScrollUp/Down | ⛔ **Not needed** — these are absent from the 26-letter translation table, so the call is a provable identity. **Do not call it**; label the `FKey` directly. | `SiegePlayerController.cpp:592,608,624,636,662,682,695,2793,2797` · table = `SiegeKeyboardLayoutStatics.cpp:57-63` (A..Z only) · identity contract = `SiegeKeyboardLayoutSubsystem.h:238-240` |
| **C — Raw, LETTER** | A raw-polled **letter** compared through the accessor | **exactly one:** the assistant's `Z` accept | ✅ The shipped **comparison** calls it once (`SiegeAssistantConsoleWidget.cpp:966`). ⛔ **The DISPLAYED LABEL DOES NOT** — `KBD-§8` pins the human-facing string to literal `Z`. **See F-1.** | `SiegeAssistantConsoleWidget.cpp:883, 952-969, 1370-1377` · `SiegeKeyboardLayoutSubsystem.h:256-260` |
| **D — Pointer only** | A mouse click on a UI element; no key at all | 3 rows | n/a — no key chip | per-row citations in §4 |

### 1.2 ⭐ THE ONE-TRANSLATION SEAM — the exact algorithm 706 implements

```
Lane A:  Keys = EISubsystem->QueryKeysMappedToAction(ResolvedIA);   // ← ALREADY translated. ZERO GetPositionalKey calls.
         if (Keys.IsEmpty())                                        //   fallback ONLY (see 1.3)
             Keys = { LayoutSubsystem->GetPositionalKey(Row.QwertyReferenceKeys[i]) };   // EXACTLY ONE translation
Lane B:  Keys = Row.QwertyReferenceKeys;                            // identity by construction — no call
Lane C:  Keys = Row.QwertyReferenceKeys;   (bLiteralKeyLabel=true)  // KBD-§8 — the literal is for the human
Lane D:  no key chip
```

⛔ **In no branch is a translation applied twice.** That is the whole audit.

- `UEnhancedInputSubsystemInterface::QueryKeysMappedToAction(const UInputAction*)` — *"Returns the keys mapped to the given action in the **active** input mapping contexts"* (`Engine/Plugins/EnhancedInput/Source/EnhancedInput/Public/EnhancedInputSubsystemInterface.h:381-384`, UE 5.8 install). The **active** context is the retargeted duplicate `AHeroCharacter` passed to `AddMappingContext` (`HeroCharacter.cpp:271-275`). ⇒ this query IS the single translation.
- ⭐ **This route also survives a REBIND.** If anyone ever re-keys `IMC_Hero`, the row follows with **zero registry edits**. The `QwertyReferenceKeys` column below is therefore a **fallback and a test fixture — ⛔ never the displayed truth while the action resolves.**

### 1.3 Freshness — `HELP-§1`'s "re-derive on open, never cache"

`KBD-§8` is explicit: **the caller refreshes, the accessor reads** (`SiegeKeyboardLayoutSubsystem.h:246-250`). The shipped precedent is `USiegeAssistantConsoleWidget::OpenConsole()`, which calls `RefreshKeyboardLayout()` **once per open** (`SiegeAssistantConsoleWidget.cpp:609-612`).

⇒ **706 must do the same: `RefreshKeyboardLayout()` once at overlay open, before resolving any label.**
⛔ **Do NOT bind `OnKeyboardLayoutChanged` from the widget** — `KBD-§8` forbids it for a widget, and `SiegeAssistantConsoleWidget.cpp:605-608` records the reason (new lifetime state to unbind wrongly, for a value re-read at every open).
⛔ **Do NOT cache an `FKey` or an `FString` label across opens** (`KBD-§0` ruling 2: mid-session `Win+Space` is in scope; the poll is 1 Hz — `SiegeKeyboardLayoutSubsystem.h:296`).

### 1.4 ⭐ THE DVORAK TEST PAIRS `HELP-§6` ASKS FOR — derived, not guessed

`KBD-§4` tables all 26 letters (`SiegeKeyboardLayoutStatics.cpp:57-63`) and **identity entries are never stored**, so "no entry" and "identity" are the same answer (`SiegeKeyboardLayoutSubsystem.h:238-240`). Mapping US-QWERTY → US-Dvorak positionally:

| Shipped letter key | Action | Dvorak yield | Use in the acceptance test |
|---|---|---|---|
| **`F`** | `IA_CmdAmbush` | **`U`** | ⭐ **THE KEY THAT MOVES** — and it is **Jonathan's own worked example, verbatim.** Assert the label CHANGES. |
| `T` | `IA_CmdAttack` | `Y` | moves |
| `R` | `IA_CmdHold` | `P` | moves |
| `E` | `IA_CmdDefend` | `.` | moves |
| `C` | `IA_CmdFollow` | `J` | moves |
| `Q` | `IA_Rally` | `'` | moves |
| **`M`** | `IA_WarMap` | **`M`** | ⭐ **THE KEY THAT HOLDS** (Dvorak leaves `M` in place). Assert the label is UNCHANGED. |
| **`A`** | `IA_Move` (strafe left) | **`A`** | second holder |
| `Tab` · `Enter` · `Escape` · `LeftAlt` · `LeftShift` · `SpaceBar` · `One`..`Six` · mouse | — | identity | non-letters: never in the table |

⚠️ The test drives this through `SetTranslationMapForAutomationTests` (`SiegeKeyboardLayoutSubsystem.h:299-319`) — which **LATCHES the instance out of OS probing**, deliberately, so a QWERTY host can drive both states. The CVar lever is `siege.Input.LayoutPollEnabled` (`SiegeKeyboardLayoutSubsystem.cpp:75-76`).

---

## 2. THE SHIPPED INPUT SURFACE — `IMC_Hero`, MEASURED

`IMC_Hero` is a **binary** asset and I have no editor (fence). Two independent instruments, both cited:

**(a) The asset-side readback** performed by the art-director at TASK-568 over MCP — 24 rows verbatim in array order (`handoffs/TASK-568-artist.md:23-49`), plus the 25th appended at TASK-589 (`TASKBOARD.md:8561` — count `24 → 25`, first 24 keys identical in array order, `IA_Move`'s `SwizzleAxis_0/1` + `Negate_0/1` and `IA_Look`'s `Negate_2` proven **by name**).

| # | Key | Action | | # | Key | Action |
|---|---|---|---|---|---|---|
| 1 | `SpaceBar` | `IA_Jump` | | 14 | `Four` | `IA_Card4` |
| 2 | `W` | `IA_Move` (+`SwizzleAxis_0`) | | 15 | `Five` | `IA_Card5` |
| 3 | `S` | `IA_Move` (+`SwizzleAxis_1`,`Negate_0`) | | 16 | `Six` | `IA_Card6` |
| 4 | `A` | `IA_Move` (+`Negate_1`) | | 17 | `LeftAlt` | `IA_UICursor` |
| 5 | `D` | `IA_Move` | | 18 | `Q` | `IA_Rally` |
| 6 | `Mouse2D` | `IA_Look` (+`Negate_2`) | | 19 | `T` | `IA_CmdAttack` |
| 7 | `LeftShift` | `IA_Sprint` | | 20 | `R` | `IA_CmdHold` |
| 8 | `LeftMouseButton` | `IA_Attack` | | 21 | `E` | `IA_CmdDefend` |
| 9 | `One` | `IA_Card1` | | 22 | `F` | `IA_CmdAmbush` |
| 10 | `RightMouseButton` | `IA_CancelPlace` | | 23 | `C` | `IA_CmdFollow` |
| 11 | `Escape` | `IA_CancelPlace` | | 24 | `Enter` | `IA_AssistantConsole` |
| 12 | `Two` | `IA_Card2` | | 25 | `M` | `IA_WarMap` |
| 13 | `Three` | `IA_Card3` | | **26** | **`Tab`** | **`IA_ControlsHelp`** — see (b) |

**(b) ⭐ A LIVE BYTE-SCAN OF THE PACKAGE, RUN TWICE THIS SESSION, AND IT CAUGHT TASK-705 LANDING MID-TASK.**
I scanned `Content/Input/IMC_Hero.uasset`'s string table directly (read-only, no editor):

| scan | size | `Tab` | `IA_ControlsHelp` |
|---|---|---|---|
| first | **13,575 B** | ❌ absent | ❌ absent |
| second (mtime `Sat Aug 29 23:55:28 2026`) | **14,099 B** | ✅ present | ✅ present (`/Game/Input/Actions/IA_ControlsHelp`) |

⇒ **TASK-705's `IA_ControlsHelp` → `Tab` append LANDED while this task was running.** TAB is no longer inert.
⚠️ **My scan is corroboration, ⛔ NOT the proof.** A string scan **cannot** show mapping order, an empty `Triggers`/`Modifiers` array, or that the instanced modifier subobjects survived — and that is exactly the TASK-445 defect signature (`SiegeKeyboardLayoutStatics.cpp:227-233`). **The authoritative proof is TASK-705's own `KBD-§2a` survivor ledger.** 706 must not treat my scan as the gate.

All 22 `IA_*` package paths present in `IMC_Hero` were read off the same scan and are used verbatim in §4's `IA asset` column.

---

## 3. THE DATA SHAPE 706 CONSUMES (the seam contract)

Suggested — 706 owns the final form; ⛔ what is **not** negotiable is that the **raw action identity** crosses the seam and **exactly one** translation happens downstream (§1.2).

```cpp
UENUM()
enum class ESiegeInputLane : uint8 { MappedAction, RawNonLetter, RawLetter, PointerOnly };

USTRUCT()
struct FSiegeControlsHelpAction
{
    FName            ActionId;             // stable, e.g. "Orders.Ambush" — the test asserts on this
    FName            Category;             // Hero | Cards | Orders | PickMode | Interface
    FText            DisplayName;          // "Ambush"
    FText            OneLine;              // §4's one-liner
    FText            Detail;               // §4's detail text, VERBATIM (707)
    ESiegeInputLane  Lane;                 // §1.1
    TSoftObjectPtr<UInputAction> Action;   // Lane A only — the /Game/Input/Actions/IA_* path
    TArray<FKey>     QwertyReferenceKeys;  // Lane A: FALLBACK + test fixture. Lanes B/C: the truth.
    bool             bLiteralKeyLabel;     // Lane C only (KBD-§8) — true for exactly one row
    bool             bPointerOnly;         // Lane D — render a click affordance, not a key chip
};
```

**Fence note for 706, and it matters:** the controller-side actions (`Card1Action`…`WarMapAction`) are already resolved on `ASiegePlayerController` (`SiegePlayerController.cpp:438-468`) — **706 owns that file pair and may add a public const accessor.** The **hero-side** actions (`SprintAction`, `AttackAction`, `RallyAction`, `HeroMappingContext`) are **`protected` on `AHeroCharacter`** (`HeroCharacter.h:388-402`), a file 706 does **not** own ⇒ ⛔ **do not add a getter there.** Resolve those four, plus `IA_Move`/`IA_Look`/`IA_Jump`, **by soft path from this registry** — the paths in §4 are measured off `IMC_Hero` itself.

⛔ **An unresolvable action does NOT hide the row.** `HELP-§2` mechanism 2: the row still renders, with `"(undocumented — TODO)"` for missing text and no key chip for a missing binding. **A visible gap gets fixed; a silent omission does not.**

---

## 4. THE ACTION REGISTRY — 24 ROWS

> Citation convention: `path:line`. Every claim below was read this session. `SPC` = `Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.cpp` · `SPC.h` its header · `SU` = `SummonedUnit.cpp` · `HC` = `HeroCharacter.cpp`/`.h` · `UC.h` = `UnitCommand.h` · `ACW` = `SiegeAssistantConsoleWidget.cpp`.
> ⛔ **No number is restated in prose where it can be read from data** (the M7.7 lesson). Where a literal is unavoidable its source rides beside it, and 707 must keep that pairing.

### CATEGORY — HERO

---

**R-01 · `Hero.Move` · "Move"** — Lane **A** · `IA_Move` (`/Game/Input/Actions/IA_Move`) · QWERTY ref `W` `A` `S` `D`

*One line:* **Walk your hero around the battlefield.**

*Detail:* Four separate bindings on one action — forward, back, strafe left, strafe right. The back and left rows carry `Negate` modifiers and the forward/back rows carry `SwizzleAxis`, which is why these four rows must never be rewritten as a block (`handoffs/TASK-568-artist.md:23-49`; `SiegeKeyboardLayoutStatics.cpp:227-233`). Base walk speed is `AHeroCharacter::WalkSpeed` (`HeroCharacter.h:406`); the speed actually applied composes the Swift Boots upgrade on top and is `GetEffectiveWalkSpeed()` (`HeroCharacter.h:343`) — the base is never mutated. On a non-QWERTY layout these four keep their **physical positions**: the layout subsystem retargets the mapping context's keys, not your muscle memory (`SiegeKeyboardLayoutStatics.h:53`).

---

**R-02 · `Hero.Look` · "Look"** — Lane **A** · `IA_Look` (`/Game/Input/Actions/IA_Look`) · QWERTY ref `Mouse2D`

*One line:* **Move the mouse to swing the camera.**

*Detail:* Bound as a 2D mouse axis with a `Negate_2` modifier on the Y channel (`handoffs/TASK-568-artist.md:28`; `handoffs/TASK-399-artist.md:137` records `X✗ Y✓ Z✗`). Look is **suspended while you hold the interface-cursor key** — `OnUICursorPressed` calls `SetIgnoreLookInput(true)`, paired 1:1 with its release, so a click-drag on the HUD cannot nudge the camera (`SPC:768-770`, `SPC:783-790`). It is **not** suspended in placement, targeting or a group pick: those modes use `FInputModeGameAndUI` with `DoNotLock`, so the mouse steers the cursor while movement keys keep working (`SPC:4361-4370`).

---

**R-03 · `Hero.Jump` · "Jump"** — Lane **A** · `IA_Jump` (`/Game/Input/Actions/IA_Jump`) · QWERTY ref `SpaceBar`

*One line:* **Jump.**

*Detail:* Inherited from the character template and bound on both press and release (`GitClaudeUnrealTestCharacter.cpp:59-60`). ⚠️ Falling out of the world is **a death, not a despawn** — `AHeroCharacter::FellOutOfWorld` deliberately does not call `Super` (which would `Destroy()` the pawn) and routes into the same path as lethal damage, so the standard respawn brings you back at your castle (`HeroCharacter.h:138-151`).

---

**R-04 · `Hero.Sprint` · "Sprint"** — Lane **A** · `IA_Sprint` (`/Game/Input/Actions/IA_Sprint`) · QWERTY ref `LeftShift`

*One line:* **Hold to run faster — it is a hold, not a toggle.**

*Detail:* Bound on `Started`, `Completed` **and** `Canceled`, so the sprint can never stick on if the press is interrupted (`SPC`-analogue at `HeroCharacter.cpp:292-294`). Pressing raises the max walk speed to `SprintSpeed` (`HeroCharacter.h:410`) and releasing returns it to `WalkSpeed` (`HeroCharacter.h:406`); both compose the Swift Boots move-speed bonus live via `GetEffectiveSprintSpeed()` / `GetEffectiveWalkSpeed()` rather than mutating the base (`HeroCharacter.h:343,346`). A dead hero cannot start a sprint — `StartSprint` early-outs on `bDead` — but `StopSprint` is unguarded so the state always releases (`HeroCharacter.cpp:323-338`). ✅ **Sprinting and attacking are independent**: sprint is a hold on one action, melee is a press on another, and nothing in `DoMeleeAttack` reads the sprint flag (`HeroCharacter.cpp:340-360`).

---

**R-05 · `Hero.Attack` · "Attack"** — Lane **A** · `IA_Attack` (`/Game/Input/Actions/IA_Attack`) · QWERTY ref `LeftMouseButton`

*One line:* **Swing at every enemy in front of you.**

*Detail:* One swing damages **all** enemy team agents within `MeleeRange` (`HeroCharacter.h:437`) **and** inside a `±MeleeHalfAngleDegrees` forward cone (`HeroCharacter.h:441`), rate-limited to one swing per `MeleeCooldown` seconds (`HeroCharacter.h:445`; the gate is at `HeroCharacter.cpp:354-360`). Damage per swing is **composed live** — base plus the Sharpened Blade stacks — through `GetEffectiveMeleeDamage()` (`HeroCharacter.h:234`, applied at `HeroCharacter.cpp:464`). **No friendly fire** (`HeroCharacter.cpp:581-585`). ⭐ **The same physical click confirms placement, spell targeting and every group-order stage** — and when it does, melee is suppressed so the click does one thing only: `SetMeleeSuppressed(true)` makes `DoMeleeAttack` a no-op that **does not even consume the cooldown** (`HeroCharacter.h:162-167`, `HeroCharacter.cpp:342-346`; set for the pick at `SPC:2723-2728`, released on every exit path at `SPC:3144-3148`).

---

**R-06 · `Hero.Rally` · "Rally"** — Lane **A** · `IA_Rally` (`/Game/Input/Actions/IA_Rally`) · QWERTY ref `Q`

*One line:* **Give every nearby friendly unit a temporary speed boost.**

*Detail:* Buffs every same-team `ASummonedUnit` within `RallyRadius` by `RallySpeedBonus` for `RallyDuration` seconds — **units only, never the hero, never enemy units** (`HeroCharacter.cpp:532-559`). Friendly miners are included, since `AMinerUnit` is a summoned-unit subclass (`HeroCharacter.cpp:535-536`). On cooldown the press is a no-op but **still broadcasts** `OnRallyStateChanged(false, remaining)` so the HUD can flash the time left (`HeroCharacter.cpp:525-529`); the cooldown length is `RallyCooldown` and `OnRallyReady` re-broadcasts `(true, 0)` when it elapses (`HeroCharacter.cpp:565`, `:568-572`). A dead hero cannot rally (`HeroCharacter.cpp:508-511`).
⚠️ *This action is not in Jonathan's list — see §6.*

### CATEGORY — CARDS

---

**R-07 · `Cards.Play` · "Play a card"** — Lane **A** · `IA_Card1`…`IA_Card6` (`/Game/Input/Actions/IA_Card1`…`IA_Card6`) · QWERTY ref `One` `Two` `Three` `Four` `Five` `Six`

*One line:* **Press a hand slot's number to play that card.**

*Detail:* Six keys, six hand slots, bound with the slot index as the payload (`SPC:482-489`). What happens next depends on the card's type, read from the data table and **never from code** (`SPC:858-868`):
• **Unit / Building / Economy** → **placement mode.** A ghost follows the cursor each frame and the card leaves your hand **only at confirm**, so backing out costs nothing (`SPC:895-911`, the M2 ruling at `:900-903`). Left-click confirms, right-click or `Escape` cancels (`SPC:682-698`).
• **Spell** → **targeting mode**, placement's sibling on the same input surface; same "card leaves the hand only at LMB confirm" law (`SPC:913-919`). The one exception is Gold Steal, which resolves instantly with no reticle because it is a global effect (`SPC:914-916,920`).
Gold is checked **before** the type is considered — the affordability refusal outranks the type refusal (`SPC:879-889`). A press is **quietly ignored** — not refused — after match end, mid-placement or mid-targeting (`SPC:812-836`). Key `1` has one legacy quirk: with hand slot 0 empty it falls back to the always-available Footman placement (`SPC:732-748`).

---

**R-08 · `Cards.CursorHold` · "Show the mouse cursor"** — Lane **A** · `IA_UICursor` (`/Game/Input/Actions/IA_UICursor`) · QWERTY ref `LeftAlt`

*One line:* **Hold to bring up the cursor so you can click the cards and buttons on your HUD.**

*Detail:* A **hold**, not a toggle. Holding puts the game in `GameAndUI` with the cursor visible and camera look suspended, so a click-drag on the HUD cannot nudge the camera (`SPC:756-771`). Bound on `Completed` **and** `Canceled` so the hold can never stick regardless of the action's trigger setup (`SPC:494-499`), and the release is guarded so a double release cannot unbalance the ignore-look counter (`SPC:783-790`). Releasing while placement mode is live **leaves the cursor to placement mode** — the two owners compose rather than fight (`SPC:775-780`; the composition is one boolean expression at `SPC:4357`).

---

**R-09 · `Cards.Discard` · "Discard a card"** — Lane **D** (pointer only; ⛔ no key binding exists)

*One line:* **Click a hand card's discard button to bin it and draw a replacement — it costs gold.**

*Detail:* Reached only from the HUD: `UCardHandWidget::RequestDiscardSlot` → `ASiegePlayerController::DiscardHandSlot` (`CardHandWidget.h:49`, `:103-107`). The fee is the fixed `DiscardCost` (`SPC.h:1007`) and it is charged **before** the pile moves; below the fee `SpendGold` refuses with no change and no broadcast, and you get the "Not enough gold" line (`SPC:1035-1044`). A replacement is drawn immediately (`SPC:1060-1062`).
⚠️ *Not in Jonathan's list, and it has no key — see §6.*

---

**R-10 · `Cards.Cancel` · "Cancel"** — Lane **A** · `IA_CancelPlace` (`/Game/Input/Actions/IA_CancelPlace`) · QWERTY ref `RightMouseButton`, `Escape`

*One line:* **Back out of whatever you are placing, targeting or circling — it never costs anything.**

*Detail:* One action, **two keys**, and it is the same gesture everywhere. It exits placement mode, exits spell targeting, or aborts a group-order pick at **any** stage, leaving every existing group and stance untouched (`SPC:1065-1090`). ⭐ **The same two keys are ALSO polled directly every frame**, so cancelling still works even if the input asset is missing — the deliberate double-cover, and re-firing is harmless because the exits are idempotent (`SPC:501-506`, `SPC:678-686`). Nothing has been spent at the moment you cancel: cards leave the hand only at confirm (`SPC:900-903`, `:917-919`).
⛔ **`Escape` is a shipped, bound cancel key** — row 11 of `IMC_Hero` (`handoffs/TASK-568-artist.md:32`). Jonathan closed `AS-§6 A-2` permanently: **nothing may absorb it.** The controls overlay itself does not, and must not, claim it (`HELP-§5`; the console's own compliance is at `SiegeAssistantConsoleWidget.cpp:907-931`).

### CATEGORY — ORDERS

---

**R-11 · `Orders.Attack` · "Attack (army order)"** — Lane **A** · `IA_CmdAttack` (`/Game/Input/Actions/IA_CmdAttack`) · QWERTY ref `T`

*One line:* **Send your whole army at the enemy castle, right now — no circle to draw.**

*Detail:* Immediate, army-wide, and it **releases every standing group order**. The sequence is one function, shared with the assistant's `charge`: abort any in-flight pick → clear all unit groups → latch the stance (`SPC:1153-1162` → `ApplyArmyWideStance` at `SPC:1122-1151`). ⛔ **The release runs BEFORE the latch and that ordering is load-bearing** — units re-read the stance on their next state tick, so releasing after latching would let a group about to be destroyed re-assert its station for one tick (`SPC:1130-1134`). Under Attack, units march the enemy castle, clearing defenders inside the enemy spawn box first; local self-defence aggro is unchanged (`UnitCommand.h:19-21`). Ignored after match end (`SPC:1144-1147`). The stance is **latched** — it persists until replaced — and `bHasIssuedCommand` flips true on your first command and stays true for the match, so the pre-command legacy behaviour never returns mid-match (`SPC:1106-1114`; `UnitCommand.h:39-43`).

---

**R-12 · `Orders.Defend` · "Defend (army order)"** — Lane **A** · `IA_CmdDefend` (`/Game/Input/Actions/IA_CmdDefend`) · QWERTY ref `E`

*One line:* **Pull your whole army back to your own castle and fight only what comes to it.**

*Detail:* The exact mirror of Attack — same guard, same two calls, same order, same final latch, through the same one implementation (`SPC:1196-1199`; `SPC:1122-1151`). Units fall back toward your own castle and engage only enemies inside the defend band (`UnitCommand.h:21-22`). ⚠️ **What "the band" means CHANGED and the header says so on purpose:** `DefendRadius` is **no longer a disc centred on the castle** — it is the band **past the castle's wall face**, and the acquisition radius is **derived at every decision** from the castle's live colliding half-width plus that band, by `ASummonedUnit::ResolveDefendEngagementRadius`, which is the only supported reader (`UnitCommand.h:23-31`). ⛔ Reading it as a centre radius is the defect that made Defend acquire nobody at the 9× castle (`UnitCommand.h:28-31`).

---

**R-13 · `Orders.Hold` · "Hold"** — Lane **A** · `IA_CmdHold` (`/Game/Input/Actions/IA_CmdHold`) · QWERTY ref `R`

*One line:* **Pick a squad, give it a patch of ground to stand on and a patch to fight over — it disengages the moment its target leaves both.**

*Detail:* Opens the **three-circle pick** described in R-16..R-18 (`SPC:1165-1170`). At the end you have a group with a **position zone** and an **attack zone**, and its units run a strict priority ladder every state tick: enemies in the **attack** zone first, else enemies in the **position** zone, else walk to the unit's own station inside the position zone and wait (`SummonedUnit.cpp:1757-1767`, ladder at `:1808-1864`).
⛔ **The leash is what makes this HOLD and not AMBUSH.** A target that is alive but has left **both** zones is dropped **that tick** — the unit disengages and returns toward its station (`SummonedUnit.cpp:1778-1794`). A dead target is dropped for both types (`:1770-1774`).
There is deliberately **no separate leash range**: the zones **are** the leash (`SummonedUnit.cpp:1766-1767`). Targets are **sticky** — a live, zone-valid target is kept and re-acquisition runs only when target-less, which is what stops the goal flipping every tick (`:1762-1766`, `:1808-1810`). One monotone upgrade exists, HOLD only: a position-tier target yields to an attack-zone enemy the moment one appears, and never the other way, so the two tiers cannot oscillate (`:1795-1805`). Stations are spread by a sunflower offset so the squad does not mill at one point (`:1851-1856`).
Pressing `T` or `E` **destroys** this group (R-11/R-12).

---

**R-14 · `Orders.Ambush` · "Ambush"** — Lane **A** · `IA_CmdAmbush` (`/Game/Input/Actions/IA_CmdAmbush`) · QWERTY ref `F` (**Dvorak: `U`**)

*One line:* **Set a squad to wait at a position and then chase to the kill anything that enters its attack zone, even after it leaves.**

*Detail:* Identical to HOLD in every respect — same three-circle pick, same priority ladder, same stations — **except the leash** (`SPC:1172-1177`; `UnitCommand.h:56-62`).
⭐ **The difference, at the line that implements it:** the zone drop-test is wrapped in `if (CurrentTarget && Group.Type == ESiegeGroupCommandType::Hold)` — **AMBUSH skips that whole block while a live target exists.** It keeps the target until the kill, then the ladder resumes (`SummonedUnit.cpp:1778`, and the comment stating the intent verbatim at `:1788-1792`). Ambush **acquires** through exactly the same two tiers; the exemption governs only when an already-held target is **released** (`SummonedUnit.cpp:1808-1810`).
A dead target is still dropped, for both types (`SummonedUnit.cpp:1770-1774`). The single monotone position→attack upgrade is **HOLD-only** and does not run for Ambush (`:1795-1805`, gated by the same `Type == Hold` test).
⚠️ **For miners the two orders collapse into one behaviour** — Jonathan's own words, recorded in the header: *"'Ambush' is the same thing as 'hold'"* for a miner, and it is implemented once (`MinerUnit.h:26`, `:45`, `:200`).
Pressing `T` or `E` destroys this group (R-11/R-12).

---

**R-15 · `Orders.Follow` · "Follow"** — Lane **A** · `IA_CmdFollow` (`/Game/Input/Actions/IA_CmdFollow`) · QWERTY ref `C`

*One line:* **Circle units to make them escort you — they walk with you and will not fight.**

*Detail:* ⭐ **ONE circle, one stage.** Deliberately **not** the three-stage flow with two stages switched off: the pick enters at Select and **confirms there**, and the later stages are structurally unreachable (`SPC:1179-1193`; `SPC:2893-2901`; the tripwire that fails loud if it were ever reached is at `SPC:2835-2847`). Jonathan's reason is quoted in the header: *"There is only one mouse scroll circle used for this, and it is just the circle used to indicate what units follow"* (`UnitCommand.h:66-70`). Its stage-1 prompt is worded separately so it never promises a second stage (`SPC:2744-2750`).
The anchor is **you**, not a piece of ground: a follow group carries zero radii, zero centres and no marker decals, and the select circle is destroyed at confirm rather than left on the map (`UnitCommand.h:71-73`; `SPC:3325-3329`). The station is your **live** position plus that unit's own sunflower offset, resolved **every tick and never cached** — which is exactly what makes hero respawn work for free (`SummonedUnit.cpp:1912-1921`, `:1937-1940`).
⛔ **Followers never attack.** The target is forced null every tick and the follow body calls none of the acquire/attack functions (`SummonedUnit.cpp:1886-1898`). ✅ **A following Cleric still heals** — healing is not attacking, and an escorting medic is the point of a support unit told to follow (`SummonedUnit.cpp:1900-1910`).
**If you die, followers hold position** — no target, no march, no attack — and resume the instant a live pawn exists again, including a brand-new one after respawn (`SummonedUnit.cpp:1923-1935`).
Unlike `T`/`E`, Follow **does not release** your other groups: it adds the circled units to the one follow group, stealing them out of any Hold/Ambush group, and units you did not circle keep their orders (`SPC:1186-1189`; `SPC:3293-3302`).
⭐⭐ **THE SPAWN DEFAULT — the core-loop fact this help screen exists to teach:** every follow-eligible Blue unit **spawns already following you**, on every spawn path, and **nothing player-side auto-engages any more — you personally order every fight** (`SummonedUnit.cpp:1286-1291`, the call site at `:1277`). Siege units (Ogre/Sapper) and the whole enemy side are unaffected, because the eligibility predicate excludes them (`SummonedUnit.cpp:1290-1291`, `:1304-1311`). Enrolment is **unconditional**: a unit spawned after you pressed `T` still spawns following — reinforcements do **not** inherit your last order (`SummonedUnit.cpp:1293-1295`). Miners are the one exception and **spawn mining** (`MinerUnit.h:496-502`).

### CATEGORY — PICK MODE (the shared three-circle flow — Jonathan's named questions)

---

**R-16 · `PickMode.Confirm` · "Confirm the circle"** — Lane **B** (raw-polled) · `LeftMouseButton`

*One line:* **Left-click to lock in the circle you are drawing and move to the next one.**

*Detail:* Polled directly every frame while a pick is live, not bound to an input action (`SPC:604-611`). Your hero does **not** swing on that click — melee is suppressed for the whole pick (`SPC:2717-2728`; `HeroCharacter.h:162-167`).
⭐ **THE THREE CIRCLES, IN ORDER — what each one actually does:**
1. **SELECT** — opens at `GroupSelectRadiusDefault` (`SPC.h:1277`). At confirm, **every eligible unit inside it (2D) joins the group** (`SPC:2851-2882`). ⚠️ **An empty circle is refused and you STAY in the stage** — a different circle can still succeed — with "No units in the circle" on the HUD (`SPC:2884-2891`). The eligibility test **differs by order**: Hold/Ambush use the narrower zone-order predicate, while **Follow is wider — it also admits the Support Cleric and the Miner** — and both exclude Siege units (Ogre/Sapper) and the entire enemy side (`SPC:2857-2862`). This circle is a **transient pick visual**: it is destroyed at the final confirm and never becomes a marker (`SPC:2903-2907`, `:2988-2995`). For **FOLLOW the flow ENDS HERE** (`SPC:2893-2901`).
2. **POSITION** — opens at `GroupPositionRadiusDefault` (`SPC.h:1281`). This is **the ground the squad stands on**: the station zone it spreads inside, *and* the second-priority engage disc (`SPC.h:1279`; `SPC:2919-2935`). At the final confirm this circle **stays on the map as the group's permanent position marker** (`SPC:2926`, `:2947-2955`).
3. **ATTACK** — opens at `GroupAttackRadiusDefault` (`SPC.h:1285`). This is the **first-priority engage trigger**: an enemy entering it is what the squad goes for first (`SPC.h:1283`; `SummonedUnit.cpp:1830`). It also **stays as a permanent marker** (`SPC:2947-2955`).
Both surviving markers die with the group (`UnitCommand.h:149-155`).
The circle you are drawing **traces to the surface under the cursor** — flat floor, hill crown or flank alike, never a flat plane — and it **hides while the cursor is on the sky**; the confirm refuses on the same flag, so what you see is what the click does (`SPC:2759-2782`, and the matching refuse-and-stay at `SPC:2822-2831`).
The circles are colour-coded by stage: **Select white, Position green, Attack red**, pushed through a material parameter named `StageTint` (`SPC:3253-3265`). See U-4.
The completion line reports the count that **joined**, not the count you circled, because members that died mid-flow are dropped (`SPC:2968-2978`).

---

**R-17 · `PickMode.Resize` · "Resize the circle"** — Lane **B** (raw-polled) · `MouseScrollUp` / `MouseScrollDown`

*One line:* **Scroll the mouse wheel to grow or shrink the circle you are currently drawing.**

*Detail:* One notch changes the **active** circle's radius by `GroupRadiusWheelStep`, clamped between `GroupRadiusMin` and `GroupRadiusMax` (`SPC:2785-2806`; the three values at `SPC.h:1265`, `:1269`, `:1273`). Each stage **opens at its own default** and resizing one circle never touches an earlier one (`SPC.h:1258-1262`; `SPC:2710`, `:2909`, `:2929`). The decal resizes **in place** as you scroll (`SPC:2808-2817`). ⛔ **The wheel is polled, not bound**, and it is verified globally unbound elsewhere — it is **inert everywhere except inside a pick**, because `ApplyGroupPickWheel` is only ever called from the pick branch (`SPC:2787-2791`, call site `SPC:599`). If the circle material is missing the radius still changes and the confirm still uses it — you just get no visual (`SPC:2808-2810`).

---

**R-18 · `PickMode.Cancel` · "Exit the command"** — Lane **B** (raw-polled) · `RightMouseButton` / `Escape`

*One line:* **Right-click or press Escape to abandon the order at any stage — it costs nothing and changes nothing.**

*Detail:* This is Jonathan's *"how to exit the command"*. Polled every frame at the top of the pick branch, **before** anything else runs (`SPC:591-596`), and also reachable through the bound cancel action (R-10) — the deliberate double-cover (`SPC:1082-1089`). Cancelling **leaves every existing group and stance unchanged** (`SPC:591`, `:1082-1084`).
The teardown is one function and every exit funnels through it — the final confirm, either cancel route, `T`/`E`, match end, hero death, unpossess, match reset and `EndPlay` (`SPC:3135-3141`). It releases the melee suppression **before any early-out** (`SPC:3137-3148`), clears the stage and all pick scratch (`SPC:3155-3161`), **destroys every circle the flow still owns** — a cancel at any stage kills all live circles, while a completed flow only loses its Select circle because the other two were handed to the group (`SPC:3163-3182`), clears the HUD prompt so the stance display returns (`SPC:3184-3187`), and restores free-look **unless the interface-cursor key is still held**, because the cursor owners compose (`SPC:3189-3191`).
⚠️ **Re-pressing the same order key mid-flow does nothing** — it is a silent ignore; right-click/`Escape` is the cancel surface (`SPC:2676-2683`).

### CATEGORY — INTERFACE

---

**R-19 · `Interface.AssistantConsole` · "AI chat"** — Lane **A** · `IA_AssistantConsole` (`/Game/Input/Actions/IA_AssistantConsole`) · QWERTY ref `Enter`

*One line:* **Open the chat box and tell your assistant what to do in plain English — press it again to close.**

*Detail:* A **toggle**, and it asks *"is it open?"* **before** *"may it open?"* — the close half is deliberately un-gated, because a close that can be refused is a close that can strand your cursor (`SPC:4440-4463`). Opening is gated: it is refused while placement, spell targeting or a group pick owns the cursor, or after match end (`SPC:4378-4395`), and **nothing is created or shown until the posture is granted** — on a refusal literally nothing happens on screen (`SPC:4484-4501`).
⭐ **It works anywhere.** Unlike the war map there is **no proximity check, no NPC reference and no range condition** — Jonathan's ruling, verbatim in the code: *"the console still works anywhere"* (`SPC:4479-4482`; `SiegeAssistantConsoleWidget.h:147-156`).
**Sending:** type and press `Enter` — only a genuine `Enter` commits; moving focus away or clearing the box does not submit a half-typed sentence (`SiegeAssistantConsoleWidget.cpp:761-769`).
⭐ **CLOSING — the complete list, and it is a contract (`AS-§6 A-2`):** (1) press the open key again; (2) `CancelPressed()` — public API with **no caller today**, kept deliberately; (3) the fault latch `SetConsoleEnabled(false)`; (4) **`Enter` on an empty box** — Jonathan's directive: *"if you press enter without anything typed in the box then it will close"* (`SiegeAssistantConsoleWidget.h:111-126`; route 4 implemented at `SiegeAssistantConsoleWidget.cpp:771-819`). Empty-`Enter` closes **even with a confirm prompt up** — that is the point of the feature (`:817-818`).
⛔⛔ **`Escape` does NOT close the chat box, permanently.** It is left unabsorbed so the shipped placement / targeting / group-pick cancel routes keep firing byte-identically while the box is open — measured from engine source and closed by Jonathan on 2026-08-04 (`SiegeAssistantConsoleWidget.h:99-109`; the implementation note that the token `Escape` does not appear in the key handler **at all** is at `SiegeAssistantConsoleWidget.cpp:907-918`).
**A close is not a cancel:** closing the window broadcasts no cancellation — only the assistant's own state machine may turn one into the other (`SiegeAssistantConsoleWidget.cpp:637-641`, `:812-818`).
The console never sets the input mode itself; the controller owns that in one place (`SiegeAssistantConsoleWidget.h:138-143`; `SPC:4357-4375`). A faulted assistant disables **this box and nothing else** — no key, no card, no command changes (`SiegeAssistantConsoleWidget.h:144-146`).

---

**R-20 · `Interface.AssistantAccept` · "Accept the assistant's plan"** — Lane **C** (raw-polled letter) · `Z` · `bLiteralKeyLabel = true`

*One line:* **When the assistant asks you to confirm an order, press Z to accept — or just close the box to discard it.**

*Detail:* There are **no accept and cancel buttons**; Jonathan removed both. His ruling, verbatim in the code: *"instead of it being a cancel button and an accept button, lets make it to where there is no cancel button (they just simply close the chat box), and instead of an accept button they press 'z'"* (`SiegeAssistantConsoleWidget.h:159-165`). While a prompt is up the status line reads, exactly: **"Press Z to accept, or close this box to discard"** (`SiegeAssistantConsoleWidget.cpp:71`, raised at `:1374-1377`).
The key is caught in **preview** — it tunnels down the focus path from the root **before** the focused text box, which is why a plain key handler could never see a printable key the box already ate (`SiegeAssistantConsoleWidget.h:167-170`; `SiegeAssistantConsoleWidget.cpp:845`).
⛔ **Modified presses are not the accept key** — `Ctrl+Z` / `Ctrl+Shift+Z` are the text box's own undo and redo, and consuming them would both kill undo **and** execute an order you never asked for (`SiegeAssistantConsoleWidget.cpp:868-878`). ✅ `Shift+Z` **is** accepted — it is still "the Z key" to a human (`:875-876`).
The grab is as narrow as it can be: it fires **only** while the box is open, enabled, and a confirm prompt is actually up, so you can still type the letter `z` the rest of the time (`SiegeAssistantConsoleWidget.cpp:854-866`). ⚠️ Accepted consequence, recorded: while a prompt **is** up you cannot type `z` into the box (`SiegeAssistantConsoleWidget.h:186-190`).
⭐ **On a non-QWERTY layout the game listens at the QWERTY-`Z` PHYSICAL POSITION** — the comparison resolves through the layout subsystem (`SiegeAssistantConsoleWidget.cpp:952-969`; the direction contract at `SiegeKeyboardLayoutSubsystem.h:230-235`).
⛔⛔ **THIS ROW'S LABEL IS THE ONE EXCEPTION TO `HELP-§1`** — see **F-1**. `KBD-§8` pins the human-facing string to a literal `Z` on every layout (`SiegeKeyboardLayoutSubsystem.h:256-260`; `SiegeAssistantConsoleWidget.cpp:1370-1373`). **706 must NOT derive this one.**

---

**R-21 · `Interface.WarMap` · "Open the map"** — Lane **A** · `IA_WarMap` (`/Game/Input/Actions/IA_WarMap`) · QWERTY ref `M` (**Dvorak: `M` — this key does not move**)

*One line:* **Walk up to your commander in your castle, then press it to open the war map — press again to close.**

*Detail:* A toggle, and **close is asked first and never gated**, for the same reason as the chat box (`SPC:4767-4779`).
⭐ **THE PROXIMITY GATE — and it is proximity to YOUR OWN commander.** The team is resolved from your player state and **never guessed**: with no player state there is no honest answer and no commander is returned, because a wrong default on the wrong side would gate the map on the **enemy's** commander and price the reveal off the wrong actor (`SPC:4718-4742`). The distance test and its radius both belong to the commander — `ACommanderNpc::IsPlayerInRange` reading `InteractRadius` — and the controller re-implements neither (`SPC:4744-4765`; `CommanderNpc.h:274-295`). ⚠️ **The gate is checked BEFORE the cursor posture is touched**, deliberately, so an out-of-range press cannot be felt as a one-frame flicker mid-fight (`SPC:4781-4791`). Out of range you get one HUD line naming the reason — **"Walk up to your commander in the castle to use the war map"** (`SPC:4797`).
⛔ **This gate is the map's and the map's only** — it is never applied to the chat box (`SPC:4786-4790`).
**Nothing appears until the posture is granted**, and if the widget then fails to create or fails to report itself open, the posture is **rolled back** rather than left as a cursor owner with no UI (`SPC:4820-4854`).
⭐ **Closing the map DISCARDS the paid reveal, unconditionally** — *"red dots vanish the moment the map closes, even one second after paying — that is the mechanic"* (`SPC:4857-4873`). The map can also be closed by right-click or `Escape`, polled every frame; that poll exists because a marker click opens the chat box, whose focused text field would otherwise swallow the toggle key and type it into your sentence instead (`SPC:643-671`).

---

**R-22 · `Interface.WarMapReveal` · "Reveal enemy positions"** — Lane **D** (pointer only) · the map's Reveal button

*One line:* **Pay gold on the war map to reveal every enemy position — and they vanish again the moment you close it.**

*Detail:* The price is `EnemyRevealCost` — **Jonathan's own number**, quoted at the property: *"You can pay 30 gold to reveal all enemy locations"* (`CommanderNpc.h:297-311`, `EnemyRevealCost = 30` at `:311`). It is a **mechanic rule, so it is a property default and never a card-table column**, and the commander class only **holds** the number: it never reads a balance and never spends (`CommanderNpc.h:301-307`). The cost is read off your **own team's** commander at the moment of purchase and never re-typed (`SPC:5103-5121`); with no commander the purchase **fails closed** rather than inventing a fallback price (`SPC:5108-5119`).
⛔ **The refusal is net-zero: you are refused BEFORE any gold moves**, both in the local check that produces the HUD message and again on the authority (`SPC:4982-4992`; `SPC:5123-5134`). Past the spend there is deliberately **no early-out**, so a partial spend is impossible — even an empty survey is a legitimate paid-for answer (`SPC:5136-5139`).
The spend is always initiated by **your** click, which is what keeps the standing ruling *"THE AI NEVER SPENDS GOLD"* true (`CommanderNpc.h:304-307`).

---

**R-23 · `Interface.WarMapMarker` · "Click a place on the map"** — Lane **D** (pointer only) · a map marker

*One line:* **Click a marker on the war map to drop that place's name into the chat box — you still send the sentence yourself.**

*Detail:* Clicking a marker **opens the chat box first, through the proper open path**, then appends the symbol. That order is pinned: the append never opens the box and never submits, and opening clears the input field on **every** open — so appending first and opening second would silently eat your click (`SPC:4919-4930`).
The symbol is moved **opaquely** — the controller never spells it and must not learn which symbols exist, because a validation branch there would be a second, drifting copy of a vocabulary it does not own (`SPC:4943-4951`). The append's return value **is** checked; a refused insert is logged and inserts nothing rather than mis-delivering (`SPC:4949-4957`).
⭐⭐ **NOTHING IS SUBMITTED, AND THAT IS THE RULING:** *"the map writes the symbol into the box and THE PLAYER SENDS THE SENTENCE HIMSELF. An auto-submit would turn a click into an order"* (`SPC:4959-4965`).

---

**R-24 · `Interface.ControlsHelp` · "Controls"** — Lane **A** · `IA_ControlsHelp` (`/Game/Input/Actions/IA_ControlsHelp`) · QWERTY ref `Tab`

*One line:* **Open this list of controls — press it again to close.**

*Detail:* A toggle. ⭐ **It documents its own key**: `Tab` is a positional key like any other, so on a layout that moves it this row re-derives with everything else (`HELP-§4`). The overlay **does not pause the game** — single-player has no pause today and the enemy keeps marching; a pause would be a new mechanic, not a side effect of a help screen (`HELP-§5`). It is **read-only on the world**: opening it issues no order, cancels no group, plays no card and moves no gold (`HELP-§5`).
⛔ **It closes on `Tab` or its own Close button, and that is the complete list — it never claims `Escape`** (`HELP-§5`; the shipped precedent it copies is `SiegeAssistantConsoleWidget.cpp:907-931`). Every shipped cancel route keeps firing while it is open.
**Cursor posture is added to `ApplyCursorInputState()`'s one composition and nowhere else** — the existing owners keep their exact shipped precedence (`SPC:4326-4376`, the composition at `:4357`). ⛔ A direct `SetInputMode`/`bShowMouseCursor` call from a widget is the defect that once booted the arena input-dead (`SPC:222-239`).
**Status:** the action asset and its `Tab` mapping both landed during this task — see §2(b).

---

## 5. CITATION INDEX — every source I opened this session

| Topic | Read at |
|---|---|
| Pinned soft-ref block (the action set) | `SiegePlayerController.cpp:194-215` |
| `BeginPlay` posture normalization / level-travel law | `SiegePlayerController.cpp:218-239` |
| `SetupInputComponent` — every binding + the "no `GetPositionalKey` here" note | `SiegePlayerController.cpp:431-577`, esp. `:561-565` |
| `PlayerTick` — pick / targeting / war-map / placement branches, all raw polls | `SiegePlayerController.cpp:579-699` |
| Card play routing (placement vs targeting vs instant), gold gate | `SiegePlayerController.cpp:732-754`, `:792-921` |
| Alt cursor hold + release + counter pairing | `SiegePlayerController.cpp:756-790` |
| Discard fee + net-zero refusal | `SiegePlayerController.cpp:1030-1063` |
| Cancel action (three modes) | `SiegePlayerController.cpp:1065-1090` |
| Stance latch + `bHasIssuedCommand` | `SiegePlayerController.cpp:1092-1120` |
| `ApplyArmyWideStance` — release-before-latch | `SiegePlayerController.cpp:1122-1151` |
| The five order handlers | `SiegePlayerController.cpp:1153-1199` |
| `BeginGroupPick` — guards, melee suppress, stage-1 prompts | `SiegePlayerController.cpp:2636-2757` |
| `UpdateGroupPickReticle` / `ApplyGroupPickWheel` | `SiegePlayerController.cpp:2759-2818` |
| `ConfirmGroupPickStage` — all three stages + the Follow fork | `SiegePlayerController.cpp:2820-3008` |
| Stage tint (white/green/red) | `SiegePlayerController.cpp:3236-3266` |
| `ConfirmFollowPick` | `SiegePlayerController.cpp:3282-3330` |
| `CancelGroupPick` — the one teardown | `SiegePlayerController.cpp:3135-3192` |
| `ApplyCursorInputState` — the cursor ladder | `SiegePlayerController.cpp:4326-4376` |
| Console open/close gating + toggle | `SiegePlayerController.cpp:4378-4514` |
| War map open/close/gate/proximity | `SiegePlayerController.cpp:4684-4905` |
| War-map marker click → symbol insert | `SiegePlayerController.cpp:4907-4966` |
| The 30-gold reveal (local pre-check + authority) | `SiegePlayerController.cpp:4968-5139` |
| Group tunables (6 flagged values) | `SiegePlayerController.h:1257-1286` |
| ⭐ The double-translate precedent | `SiegePlayerController.h:1209-1229` |
| Hero: sprint / melee / rally / speeds | `HeroCharacter.cpp:282-318`, `:323-360`, `:402-464`, `:506-572` |
| Hero: the class contract + melee suppression API | `HeroCharacter.h:69-98`, `:138-171`, `:232-234`, `:406-445` |
| Hero: the positional-context resolve (the ONE call site) | `HeroCharacter.cpp:240-278` |
| Template Move/Look/Jump bindings | `GitClaudeUnrealTestCharacter.cpp:59-67`, `.h:38-50` |
| Grouped priority ladder + ⭐ the HOLD/AMBUSH leash | `SummonedUnit.cpp:1757-1865` |
| Follow body + hero-death ruling + anti-repath | `SummonedUnit.cpp:1867-1990` |
| ⭐ The spawn auto-enrol (DEFAULT-STANCE law) | `SummonedUnit.cpp:1249-1311` |
| Enum + group struct semantics (the definitive HOLD/AMBUSH/FOLLOW header) | `UnitCommand.h:11-156` |
| Miner Hold≡Ambush collapse; miner spawn default | `MinerUnit.h:26,45,200,496-502` |
| Console: close routes, `Escape` law, `Z` accept, layout refresh | `SiegeAssistantConsoleWidget.h:75-190`; `.cpp:47,71,600-648,761-819,845-932,952-969,1360-1378` |
| Layout subsystem: `GetPositionalKey` direction, identity, `KBD-§8` | `SiegeKeyboardLayoutSubsystem.h:203-320`, `.cpp:663-676` |
| Layout statics: the 26-letter table, ⭐ the retarget loop | `SiegeKeyboardLayoutStatics.h:40-60,110-200`; `.cpp:57-63,190-250` |
| Commander: `InteractRadius`, `EnemyRevealCost` | `CommanderNpc.h:274-311` |
| Engine: `QueryKeysMappedToAction` | `UE_5.8/Engine/Plugins/EnhancedInput/Source/EnhancedInput/Public/EnhancedInputSubsystemInterface.h:380-384` |
| `IMC_Hero` asset readback (24 rows) | `handoffs/TASK-568-artist.md:20-60` |
| `IMC_Hero` row list w/ actions (22 rows) + modifier ledger | `handoffs/TASK-399-artist.md:64-95,133-137` |
| `Enter` row 24 + the free-key proof | `handoffs/TASK-445-artist.md:11-28,87-89` |
| Row 25 `M` + the five `KBD-§2a` proofs | `TASKBOARD.md:8561` |
| `StageTint` param authored + wired | `handoffs/TASK-345-artist.md:39-41` |

---

## 6. THE "ETC." — ACTIONS JONATHAN'S LIST DID NOT NAME

His directive named: follow · attack · defend · ambush · hold · sprint · attack · play cards · open the map · open the AI chat · **"etc."** Derived from the shipped surface, the **etc.** is:

| # | Row | Why it belongs |
|---|---|---|
| 1 | **R-06 Rally** (`IA_Rally`, `Q`) | A real bound hero ability with a cooldown and a HUD readout. It is in `IMC_Hero` (row 18) and nothing else documents it to the player. |
| 2 | **R-08 Alt cursor** (`IA_UICursor`, `LeftAlt`) | Named by the spec, not by Jonathan. Without it the HUD cards are **unclickable** — arguably the single least discoverable control in the game. |
| 3 | **R-10 Cancel** (`IA_CancelPlace`, RMB + `Escape`) | Spec-named. Also the row that documents `Escape` honestly as **the shipped cancel gesture**. |
| 4 | **R-16/17/18 the pick verbs** (LMB confirm · wheel resize · RMB/`Esc` exit) | ⭐ These are literally Jonathan's three named questions — *"what the first, second, and third circles do, how to resize them, how to exit the command"* — and they are **shared by Hold, Ambush and Follow**, so they earn their own rows rather than being triplicated. |
| 5 | **R-20 the `Z` accept** | The assistant's confirm step has **no buttons at all**; if this key is not documented the order silently vanishes and reads as "the assistant ate my order" (`SiegeAssistantConsoleWidget.h:181-185`). |
| 6 | **R-22 reveal / R-23 marker click** | Two pointer-only actions with real consequences — one spends gold, one writes into your chat box. Neither has a key, so neither is discoverable from a key list. |
| 7 | **R-01/02/03 Move / Look / Jump** | The floor of any controls screen, and `IMC_Hero` rows 1–6 make them ours to document. |
| 8 | **R-09 Discard** | Pointer-only, costs gold, and is the only way to cycle a dead hand. |
| 9 | **R-24 the overlay itself** | `HELP-§4`: the menu documents its own key. |

**Deliberately EXCLUDED, with reasons** (so a reviewer does not read the omission as a miss):
- `IA_MouseLook` — the asset exists (`Content/Input/Actions/IA_MouseLook.uasset`) and the template character binds a `MouseLookAction` slot (`GitClaudeUnrealTestCharacter.cpp:64`), but **it is not in `IMC_Hero`** (§2 shows 26 rows, `Mouse2D`→`IA_Look` only). It is not reachable in the arena.
- `IMC_Default` / `IMC_MouseLook` / `Content/Input/Touch/**` — template contexts the hero never applies (`HeroCharacter.cpp:271-275` applies `HeroMappingContext` only).
- **Play Again** at match end — a post-match UI button on the end screen, outside the in-match overlay's scope (`HELP-§5` scopes this to an in-match overlay on `L_Arena`). ⚠️ **Flagged for Jonathan** if he wants it listed.
- Cheat exec commands (`USiegeCheatManager`) — non-shipping only (`SiegePlayerController.cpp:187-190`). Not player-facing.
- `siege.Input.LayoutPollEnabled` and friends — CVars, not controls.

---

## 7. ⛔ DECLARED — WHAT I COULD **NOT** VERIFY AT SOURCE

| # | Claim | Status |
|---|---|---|
| **U-1** | **The QWERTY reference keys are asserted by an MCP READBACK IN A HANDOFF, not by my own read of the binary.** `IMC_Hero` is binary and I have no editor (fence). | ⭐ **Structurally harmless, and that is by design:** §1.2 makes `QueryKeysMappedToAction` the primary route, so the reference keys are a **fallback and a test fixture only**. If a reference key is wrong, the shipped label is still right. ⚠️ QA should not accept them as gospel; they are corroborated three ways (`TASK-399-artist.md:64-95` · `TASK-445-artist.md:24` · `TASK-568-artist.md:23-49`) plus my own package byte-scan of the 22 `IA_*` **paths**. |
| **U-2** | **Row 26 (`Tab` → `IA_ControlsHelp`) landed mid-task and I proved only that the STRINGS are in the package.** | A byte-scan cannot show array order, empty `Triggers`/`Modifiers`, or that the instanced modifier subobjects survived — the exact TASK-445 signature. ⛔ **The gate is TASK-705's `KBD-§2a` survivor ledger, not my scan.** |
| **U-3** | **I did not observe a single one of these controls actually fire.** Zero compiles, zero PIE, zero pixels (fence). | Everything above is read from shipped source. `SC-§32` applies: read is not observed. |
| **U-4** | **The three circles' colours.** The C++ pushes `StageTint` = white/green/red (`SPC:3253-3265`), and the comment beside it says the material *"does not carry it YET"*. | ⚠️ **THE COMMENT IS STALE — the CODE-adjacent record wins.** `handoffs/TASK-345-artist.md:39-41` records the `StageTint` vector parameter as **authored and wired** to Emissive, and I confirmed the string `StageTint` is present in `Content/Materials/M_SpellReticle.uasset`. ⇒ I state the colours in R-16 **as driven by the C++**, and flag that only pixels close a colour claim (`AS-§6 A(e)`). 707 may soften the wording; ⛔ it may not invent different colours. |
| **U-5** | **The exact values behind the named tunables.** I deliberately wrote `GroupRadiusWheelStep`, `MeleeCooldown`, `EnemyRevealCost` etc. **as names with a `file:line`**, not as numbers in prose. | ⭐ **This is the M7.7 lesson applied, not an omission**: the shipped `Notes` column once said *"in 400"* while the real radius was **700**. ⛔ **707 must render these from the live properties, or leave the name.** The one number I do state is **30 gold**, because Jonathan's own words are the source and they are quoted at the property (`CommanderNpc.h:298-299`). |
| **U-6** | **Whether Slate's focused text box swallows `Enter` before Enhanced Input sees it** — i.e. whether the open key double-fires while the box has focus. | ⚠️ **Open since TASK-445, still unmeasured**, and its own author flagged it: *"that is reasoning, not a measurement, and I did not test it"* (`handoffs/TASK-445-artist.md:207`). ⇒ **R-19's detail text does NOT claim either behaviour.** If the double-fire is real, the help text stays true. |

---

## 8. 🚩 FOR JONATHAN — TWO ROWS NO AGENT MAY DECIDE

### F-1 ⭐⭐ THE `Z` ROW IS THE ONE PLACE TWO OF YOUR OWN RULINGS POINT OPPOSITE WAYS

- **`HELP-§1`, from your controls-menu directive:** *"display the 'f' key if the player is on QWERTY … or the 'u' key if they are on the Dvorak keyboard."* ⇒ **show the TRANSLATED letter.**
- **`KBD-§8` / `KBD-§0` ruling 1, from your assistant-console ruling:** *"ANY PROMPT THAT NAMES THE ACCEPT KEY SAYS `Z`, ON EVERY LAYOUT … their keycap reads `Z`, so telling them to 'press `;`' would be the bug, not the fix"* (`SiegeKeyboardLayoutSubsystem.h:256-260`). ⇒ **show the LITERAL letter.**

Both are yours; both are written down; they disagree. The reasoning behind each is sound in its own frame — the split is **QWERTY hardware with Dvorak software** (your keycaps still read QWERTY) versus **genuinely Dvorak-labelled hardware** (your keycaps read Dvorak). **The game cannot tell those two apart** — it can only read the software layout.

**My ruling for this batch, chosen to be the smallest and most reversible:** row **R-20 keeps `bLiteralKeyLabel = true`** and shows **`Z`**, because that matches the string the console **itself** puts on screen right beside it (`SiegeAssistantConsoleWidget.cpp:71,1374-1377`) — **a help screen that contradicts the live prompt two inches away is worse than either choice alone.** Every other row derives, per your controls directive.
⇒ 🙋 **If you want R-20 to derive too, it is a one-flag change** (`bLiteralKeyLabel = false`) — **and it also means changing the console's own status line**, which is a `KBD-§8` amendment, not a help-screen edit.

### F-2 THE MENU SHOWS TWO KEYS FOR ONE ACTION IN TWO PLACES

`IA_CancelPlace` is bound to **both** `RightMouseButton` **and** `Escape` (rows 10 and 11 of `IMC_Hero`), and `IA_Move` is bound to four keys. The registry hands 706 an **array** of keys per row, so both render.
🙋 **Your call on presentation only:** `"RMB / Esc"` on one chip, or two chips. ⛔ No behaviour rides on it. Default if you say nothing: one chip, slash-separated.

---

## 9. DEVIATIONS FROM THE SPEC (`SC-§15`) — DECLARED, NOT SILENT

| # | Deviation | Reason |
|---|---|---|
| **D-1** | The spec's enumerated minimum lists **"play cards (1–6)"** as one item; I also render it as **one row (R-07) carrying six keys**, rather than six rows. | Six near-identical rows would bury the fifteen commands around them. The row's `QwertyReferenceKeys` array holds all six, so 706 derives all six labels and `HELP-§6`'s coverage test still passes on `ActionId`. ⛔ 706 may split it; the data supports either. |
| **D-2** | The spec lists **"cancel placement"** as its own item; I generalised it to **R-10 "Cancel"** and added **R-18 "Exit the command"** as a separate pick-mode row. | The shipped action is *one* handler serving **three** modes (`SPC:1065-1090`), and Jonathan asked specifically for *"how to exit the command"* on the group orders. One row would have to lie about one of them. |
| **D-3** | I added **three rows the spec did not enumerate** — R-16 confirm, R-17 resize, R-18 exit — as first-class rows rather than only as detail prose. | These are verbatim Jonathan's three questions. Burying them inside three separate command pages would triplicate the text and let the copies drift. |
| **D-4** | I added **six further rows** the spec did not enumerate: Move, Look, Jump, Rally, Discard, and the two war-map click actions. | The spec says *"his list + the shipped surface"* and `HELP-§2` mechanism 2 keys the rows to **the actual `IA_*` assets the controller resolves** plus the hero's. Omitting a shipped `IMC_Hero` row from a controls screen is precisely the silent gap that law exists to prevent. |
| **D-5** | §1.2 makes **`QueryKeysMappedToAction`** the primary derivation and demotes the registry's QWERTY keys to a fallback. The spec's phrasing (*"the QWERTY reference key"*) reads as if the registry supplies the displayed truth. | ⭐ It is a **strictly stronger** reading of `HELP-§1`: it eliminates the second copy of the truth entirely and survives a rebind with zero registry edits. The QWERTY column is retained in full for the fallback path and for the Dvorak test fixture. |
| **D-6** | I **named** tunables instead of restating their values in prose (U-5). | `HELP-§2`'s explicit *"⛔ NO NUMBER IS RESTATED IN PROSE IF IT CAN BE READ FROM DATA"*, and the M7.7 `"in 400"` / `AoERadius 700` precedent. |
| **D-7** | I performed **read-only byte scans** of `IMC_Hero.uasset` and `M_SpellReticle.uasset` (Python `open(...,'rb')`), which the spec did not ask for. | It upgraded U-1 and U-4 from "a handoff says so" to a measurement I made myself, **and it caught TASK-705 landing mid-task** (§2b). ⛔ Read-only: no editor, no write, no asset touched. |

---

## 10. WHAT TASK-708 SHOULD SCRUTINISE

1. ⭐ **The lane audit is the whole task.** Check every §4 row's Lane against §1.1, and check that 706's implementation calls `GetPositionalKey` **zero** times on a Lane-A row. A second call is invisible on this machine and instant on Jonathan's.
2. **F-1 is a flagged decision, not an agent's ruling.** If 706 derives R-20's label, that is a `KBD-§8` breach — but it is also a defensible reading of `HELP-§1`, so it is a **FLAG**, not automatically a FAIL.
3. **No literal letter anywhere.** Grep 706/707's diff for `TEXT("F")`, `TEXT("M")`, `"Press "` etc. The **only** sanctioned literal is R-20's `Z`, and it must be justified in-line by `KBD-§8`.
4. **U-5 discipline survives 707.** If 707 renders a number that this registry left as a property name, it has re-introduced the M7.7 defect.
5. **`Escape` appears in R-10 and R-18 as DOCUMENTATION and nowhere as a handler.** Any `FReply::Handled()` on `EKeys::Escape` in the diff is an automatic FAIL (`HELP-§5`).
6. **Coverage:** 24 `ActionId`s. R-24 must be present — the menu documents its own key.

---

## 11. STATUS

- **TASK-704 → `ready-for-qa`.** (`TASK-708` gates the lane; 706 is unblocked and may start now — §3 is its input.)
- ⛔ Nothing compiled, nothing committed, no editor touched, no asset modified.
- Files written: **`.claude/pipeline/handoffs/TASK-704-programmer.md`** — this one, and only this one.
