# QA Report — TASK-810 (the gate over TASK-807)

**Verdict: PASS** — 0 BLOCKER · 5 WARN · 4 NIT
**Subject:** `UCardHandWidget::GetSlotKeyLabel` + `ComposeSlotKeyLabel` + `Tests/SiegeCardHandKeyLabelTest.cpp`
**Law applied (cited, ⛔ not restated):** `CARDBAR-§0..§5` · `KBD-§4` · `KBD-§5` · `HELP-§1` · `HELP-§2` · `SC-§29` · `SC-§33` · `SC-§36.1` · `SC-§37`
**Coverage ledger (`SC-§29`):** this gate covers **TASK-807 and nothing else.** ⛔ NOT covered: TASK-808, TASK-809 (`WBP_CardHand`), TASK-812, TASK-735, TASK-813, TASK-815.
**Fences honoured:** ⛔ no edits · ⛔ no compile · ⛔ no engine/MCP (the editor coming up for TASK-808 was not touched) · ⛔ no Git · ⛔ no TASKBOARD write (the board flip is the orchestrator's).

---

## ⭐⭐ 1. THE FINDING THAT MATTERS MOST — THE DIGIT-BLINDNESS ARGUMENT IS **CORRECT**, AND I VERIFIED BOTH HALVES AT SOURCE

### 1a. The dispatch's own claim was half right, and TASK-807 caught the missing half. ✅ CONFIRMED.

The dispatched premise — *"a Dvorak fixture is the only assertion that can catch a double translation"* — is **necessary but not sufficient**, and the reason is measurable, not rhetorical:

| Link in the chain | Read at | Reads |
|---|---|---|
| the card keys are DIGITS | `SiegeControlsHelpWidget.cpp:428` | `Row.QwertyReferenceKeys = { EKeys::One … EKeys::Six }` |
| digits are ⛔ not in the translation table | `KBD-§4` (CONVENTIONS:2367) | *"DELIBERATELY NOT REMAPPED: digits (the `1`–`6` hotkeys)… This is a design decision, not an omission"* |
| an unmapped key is returned **unchanged** | `SiegeKeyboardLayoutSubsystem.h:271-272` | *"…itself when the map holds no entry for it… ⛔ Never `EKeys::Invalid` for a valid input"* |

⇒ every card key is a **fixed point** of `GetPositionalKey` at **any number of hops**. A Dvorak fixture built on `1`..`6` returns `1` under a correct implementation **and** under a double-translating one. ⭐ **A Dvorak fixture on the card DIGITS proves exactly nothing.** The dispatch's correction is upheld.

### 1b. What TASK-807 did instead — each half confirmed independently.

**Half one — the blindness is ASSERTED, not commented** (`SiegeCardHandKeyLabelTest.cpp:350-365`). For all six of `Cards.Play`'s reference keys the test asserts `GetPositionalKey(k) == k` **and** `GetPositionalKey(GetPositionalKey(k)) == GetPositionalKey(k)`. That is the fixed-point claim at one hop and at two, stated as a live assertion whose failure message names the clause. It is the reason section 3 must use a letter, and it stops a later editor "simplifying" the letter back to a `1`.

**Half two — the real claim rides a LETTER through the applied lane** (`:388-432`). Fixture `F → U → G`, lifted verbatim from the shipped `SiegeKeyboardLayoutTest.cpp:204-229` rather than re-derived. The provider answers `{ U }` (= `IA_Card1` bound to the `F` position, already retargeted). Asserted: label **==** `ShortName(U)`, ⛔ **!=** `ShortName(G)`.

### 1c. ⭐ DOES THAT ROW GENUINELY GO RED THE INSTANT A SECOND TRANSLATION ENTERS THE PATH? — ✅ YES, AND ON **FOUR** INDEPENDENT LINES.

I traced the hypothetical defect (`GetPositionalKey` applied to the provider's answer inside `ComposeSlotKeyLabel`) through every assertion in test 2 section 3. Under the Dvorak fixture the defective build composes `G` where the correct one composes `U`:

| Assertion | line | Correct build | Defective build | Discriminates? |
|---|---|---|---|---|
| `MappedOnDvorak == ShortName(OnceTranslated)` | :398-401 | `U == U` ✅ | `G == U` | ⛔ **RED** |
| `MappedOnDvorak != ShortName(TwiceTranslated)` | :405-408 | `U != G` ✅ | `G != G` | ⛔ **RED** |
| `MappedOnQwerty == MappedOnDvorak` | :419-421 | `U == U` ✅ | `U == G` | ⛔ **RED** |
| `MappedWithNoSubsystem == MappedOnDvorak` | :431-432 | `U == U` ✅ | `U == G` | ⛔ **RED** |

The fixture self-check at `:336-342` (`Once != F`, `Twice != Once`) closes the last hole: it proves the injected map is *capable* of moving the key twice, so a green result cannot be an artefact of an inert fixture. ⇒ **the keystone is real. It is the only instrument in the suite that can see this defect, and it can see it.**

---

## 2. THE GATE'S ⛔ ONE AUTOMATIC-FAIL CHECK — `GetPositionalKey` ON THIS PATH

✅ **ZERO call expressions. PASS.**

The trap declared in advance held exactly as described. A name-grep of `CardHandWidget.{h,cpp}` returns **6 hits** — `.cpp:13, :304, :375, :421` and `.h:180, :206` — and I read **all six in full context**: every one is a comment *warning against* the call (`SC-§37`'s shape; the codebase deliberately names the refused shape so it can keep explaining itself, and a naive text scan would force it to stop).

I then ran the discriminating pattern rather than the naming one:

```
rg -n "GetPositionalKey\s*\(" Source/GitClaudeUnrealTest/Siegebound/CardHandWidget.*
→ CardHandWidget.h:180:  *  GetPositionalKey (KBD-§4 excludes digits; HELP-§1's double-translate exception). Empty
```

**One hit, and it is English prose with a space before the paren.** ⇒ ⛔ **no call expression exists in either file.**

---

## 3. ONE TRANSLATION, NOT TWO — THE CHAIN, VERIFIED END TO END

| Claim | Verified at | Result |
|---|---|---|
| `QueryAppliedKeysForRow` is a **public static** | `SiegeControlsHelpWidget.h:893`, `protected:` at `:895` | ✅ public, static, reachable — ⛔ no `FINDING` owed under spec clause (3), no access widened, nothing forked |
| it returns the **already-retargeted** key | `.cpp:2941-2956` — `QueryKeysMappedToAction` over the ACTIVE contexts | ✅ *"THIS QUERY \*IS\* THE TRANSLATION"* |
| it calls `GetPositionalKey` **zero** times | `.cpp:2909-2960` read in full | ✅ zero |
| `ResolveRowDisplayKeys`' Lane-A primary branch hands applied keys through **verbatim** | `.cpp:1183-1187` — `DisplayKeys = AppliedKeys; return DisplayKeys;` | ✅ verbatim, zero translation |
| the fallback's translation is the **first**, not a second | `.cpp:1189-1200` | ✅ reached only when nothing maps the action ⇒ nothing had touched that key |
| ⛔ **zero new key-resolution code** | `CardHandWidget.cpp:325-443` read in full | ✅ `FindAction` → narrow → provider → `ResolveRowDisplayKeys` → `ComposeKeyChipLabel`. Every step is a call into the shipped resolver — **the same chain the TAB screen's `Cards.Play` row renders** |

⇒ the card bar and the help screen cannot disagree, and a rebind of `IMC_Hero` moves both with zero code edits. `CARDBAR-§2`'s central requirement is met.

---

## 4. THE SEAM — `CardHandWidget.h:178-183` vs `CARDBAR-§3`'s PINNED BLOCK

✅ **CHARACTER-FOR-CHARACTER IDENTICAL.** Diffed against CONVENTIONS:6637-6643 — the four doc-comment lines, the `UFUNCTION(BlueprintCallable, Category = "Siegebound|UI")` and `FString GetSlotKeyLabel(int32 SlotIndex);` all match, including the ⛔ glyphs and the em-dash.

**Degrade-open, traced per branch** (`CARDBAR-§3` / `KBD-§5`):

| fault | line | returns |
|---|---|---|
| registry row renamed/removed | `.cpp:336-341` | **empty** |
| `SlotIndex < 0` or ≥ the row's action count (incl. `MIN_int32`, no UB) | `.cpp:346-349` | **empty**, ⛔ before the provider runs |
| pointer-only row / lane ≠ `MappedAction` | `.cpp:367-370` | **empty** — the `"Mouse click"` chip can never leak onto the bar |
| nothing mapped **and** no reference key | `.cpp:382-386` | **empty** |
| the composer's own no-key answer | `.cpp:396-399` | **empty** — derived by asking `ComposeKeyChipLabel(SlotRow, {})`, ⛔ never typed |

⛔ Never `"(not bound)"` (`SiegeControlsHelpWidget.cpp:100`), ⛔ never the pointer chip (`:97`), ⛔ never `"?"`, ⛔ never a guessed digit. ✅ Correct.
**Warn-once:** `TSet<int32> WarnedKeyLabelSlots` (`.h:295`), guarded at `.cpp:426-431`, cloned from the `WarnedCardArtIDs` shape. The message prints `SlotIndex` **raw** — F-5's `MIN_int32` overflow avoidance is real and correct.
**No caching:** the layout subsystem is re-resolved every call (`.cpp:409-412`), matching `KBD-§0` ruling 2 / `HELP-§1`'s no-stale-label law.

---

## 5. PRECONDITIONS

| Check | Result |
|---|---|
| `IA_Card1..6` exist | ✅ all six at `Content/Input/Actions/IA_Card{1..6}.uasset` |
| `Cards.Play` carries all six actions **in slot order**, index-parallel with `One..Six` | ✅ `SiegeControlsHelpWidget.cpp:422-428`, lane `MappedAction` — and asserted in test 1 so a reorder cannot pass silently |
| all six bound in `IMC_Hero`, `One`..`Six` each exactly once | ⚠️ **method does not prove the claim — see WARN-3.** Bounded impact: the seam is degrade-open, so a mis-binding is a cosmetic chip, ⛔ never a crash |

---

## 6. ⚖️ RULING ON **F-1** — THE FLAGGED SECOND SYMBOL: **ALLOWED**

`static FString UCardHandWidget::ComposeSlotKeyLabel(int32, const USiegeKeyboardLayoutSubsystem*, TFunctionRef<TArray<FKey>(const FSiegeControlsHelpAction&)>)` (`.h:223-226`).

**Why it is within TASK-807 clause (1) rather than a breach of it.** The clause's mischief is a second *Blueprint* surface and an invented idiom — its own sentences are *"⛔ No BIE signature changes"* and *"Follow its shape, ⛔ do not invent a second idiom."* Measured against that:

- ⛔ **not a `UFUNCTION`, ⛔ not a BIE** ⇒ adds **zero** Blueprint surface. TASK-809 still sees **exactly one** bindable symbol, which is the thing clause (1) protects.
- ✅ **the three BIE signatures are intact and byte-match their documented form** — `OnHandSlotUpdated` (`.h:127`) reads identically to `CARDBAR-§0`'s citation of the pre-change file; `OnNextCardUpdated` (`.h:136`), `OnCardRefusedMessage` (`.h:146`) unchanged. `TASK-079` ruling 3 holds.
- ✅ **⛔ no new idiom.** It is a structural clone of `FSiegeControlsHelpRegistry::ComposeDetailContent` (`SiegeControlsHelpWidget.h:391-394`) — same three parameters, same nullable subsystem, the **same** `TFunctionRef<TArray<FKey>(const FSiegeControlsHelpAction&)>` third parameter, same `SC-§33` "none defaulted" declaration.
- ✅ **`SC-§36.1`'s checkable tell is satisfied in the same file:** its only caller in the entire tree is `GetSlotKeyLabel`, sixteen lines below it (`.cpp:416`). A repo-wide grep for `ComposeSlotKeyLabel` returns three files only: the two source files and the new test.
- ✅ **declared, not smuggled** — F-1 in the handoff plus a 42-line justification in the header.

**⭐ THE COST OF REFUSING IT, STATED PLAINLY, BECAUSE IT IS THE REAL ARGUMENT.** Both live inputs — Enhanced Input (via `ObservedController`) and the layout subsystem (via the game instance) — are absent in an automation run. Without the injection seam the suite can only reach the **world-less fallback lane**, where `DisplayKeys` is the registry's own `QwertyReferenceKeys` — i.e. `One..Six`. And by §1a those are **fixed points of the translation map**, so the correct build and the double-translating build produce *the same string on every layout*. ⇒ **test 2 and test 3 would collapse to `"1" == "1"`** — `SC-§37`'s own definition of an assertion that proves nothing. Refusing F-1 deletes the only instrument that can detect the defect this entire task exists to prevent. **That is a bad trade, and the seam stands.**

---

## 7. ⭐ CAN EVERY ASSERTION GENUINELY FAIL? — TRACED ONE BY ONE

| # | Test | Verdict | The defect I injected on paper, and where it goes red |
|---|---|---|---|
| 1 | `…NarrowsToItsOwnCardAction` | ✅ **CAN FAIL** | *Fixed index 0:* `SeenActionPath` reads `IA_Card1` for slot 3 → RED at `:261`; label reads `F` not `E` → RED at `:272`. *Off-by-one:* asks `IA_Card5` for slot 3 → RED at `:261`. *No narrowing:* `SeenActionCount` = 6 → RED at `:258`, and the six-key chip `1 / 2 / 3 / 4 / 5 / 6` → RED at `:272`. ⭐ Six **distinct letters** (`F,T,R,E,C,Q`), and I checked for cross-contains collisions among their short display names — there are none, so `:283` is not a false positive |
| 2 | `…IsNeverDoubleTranslated` ⭐⭐ | ✅ **CAN FAIL — four ways.** §1c above | Plus: RED if the fixture ever stops moving the key twice (`:340-342`); RED if a digit entered *this file's* fixture map (`:356-364`) |
| 3 | `…PrefersTheLiveBindingOverTheReferenceKey` | ✅ **CAN FAIL** | ⭐ The `QwertyReferenceKeys[Slot]` implementation — **which would look PERFECT today**, printing `1..6` — composes `"1"` for slot 0 against the rebound `SpaceBar`: RED at `:484` (wrong key) **and** RED at `:490` (the stale `1` appears at all, which is what catches an *append* rather than a *replace*). The fallback loop at `:496-507` stops "prefers the live key" becoming "ignores the registry" |
| 4 | `…DegradesToEmptyNeverAPlaceholder` | ✅ **CAN FAIL** | Six bad indices incl. `MAX_int32`/`MIN_int32` → exactly empty. Placeholders are **derived** by asking the shipped composer (`:548-555`) — I verified the two probes resolve correctly against the real struct defaults (`FSiegeControlsHelpAction::Lane = MappedAction`, `bPointerOnly = false`, `SiegeControlsHelpWidget.h:177, :211`), so `NotBoundAnswer` = `"(not bound)"` and `PointerAnswer` = `"Mouse click"` — **distinct and non-empty**, and the self-check at `:559-560` makes the refusals non-vacuous. `bProviderWasCalled` (`:590`) catches a *late* range check that would still return empty and look fine. `:596-606` blocks the "always return empty" cheat |
| 5 | `…WarnsOncePerSlotNotPerCall` | ✅ **CAN FAIL** | `AddExpectedMessagePlain(…, Occurrences 2)` over **10** calls across 2 bad slots ⇒ **RED at 10** on a per-call log (the real bug — slots re-push every gold tick), **RED at 0** if the log is removed, **RED at 8** if a healthy slot warns. Drives the **live** `GetSlotKeyLabel` on a real `UCardHandWidget`. I confirmed `UUserWidget::GetWorld()` walks the outer chain to the transient package and returns `nullptr` safely ⇒ no crash, `KBD-§5`'s fail-safe state exercised |

**⛔ No test transcribes a value (`SC-§37`).** Every expectation derives from the shipped registry row, from `FKey::GetDisplayName(/*bLongDisplayName=*/false)` or from the shipped chip composer. I confirmed the explicit `false` (`:144`) matches `ComposeKeyChipLabel`'s own call (`SiegeControlsHelpWidget.cpp:1217`) — the engine default is `true`, and omitting it would silently make every expectation disagree with the shipped chip. There is no `TEXT("1")`..`TEXT("6")` anywhere in the touched files.

**Compile-shape pre-checks** (⛔ I did not compile — TASK-811 owns that): `GetTransientPackageAsObject()`, `AddExpectedMessagePlain(…, Occurrences)` and `EAutomationTestFlags::EditorContext | EngineFilter` all have shipped, compiling precedent in this repo. F-6's bare-`NoAppliedKeys` binding is **proven** — `SiegeControlsHelpTest.cpp:1169…1576` already passes a bare `NoAppliedKeys` function into `ComposeDetailContent`'s identical `TFunctionRef` parameter, and that file is in the shipped 306.

---

## Findings

- **[WARN] `CardHandWidget.cpp:416-424` — the LIVE provider lambda is outside every behavioural assertion.** Test 2 drives `ComposeSlotKeyLabel` with an *injected* provider; test 5 drives the live `GetSlotKeyLabel` but only in the world-less state where `LayoutSubsystem == nullptr`, so no translation could occur even in a defective build. ⇒ a future editor who wraps `QueryAppliedKeysForRow`'s answer in `GetPositionalKey` **inside that lambda** would leave the whole suite green. ⛔ Not a defect today — the grep in §2 proves zero call expressions, and the lambda contains no key-manipulating code at all. *Suggested fix (⛔ not owed by TASK-807):* a later task promotes the lambda to a named private static, or adds a subsystem-injecting overload, so test 2's four red lines cover the live route too. **TASK-811: record, do not block.**
- **[WARN] `handoffs/TASK-807-programmer.md:83-84` — test 2 section 2 is over-claimed as a live guard on the SHIPPED table.** The handoff says *"if digits were ever added to the table, this goes red and the message names the clause."* It would not: `SetTranslationMapForAutomationTests` **latches the instance out of OS probing** by its own contract (`SiegeKeyboardLayoutSubsystem.h:318-322, :430-434`), so the assertion measures the *test-file-local* `MakeDvorakTranslation()` map, ⛔ not `FSiegeKeyboardLayoutStatics::BuildTranslationMap`. Adding digits to the shipped table would leave this green. ⭐ **The assertion's real and load-bearing function — proving that *this file's* fixture cannot discriminate on digits, therefore section 3 must use a LETTER — is fully and correctly served, and that is the claim §1 rests on.** *Fix: correct one sentence in the handoff. ⛔ No code change owed.*
- **[WARN] `handoffs/TASK-807-programmer.md:106` — the `IMC_Hero` precondition's METHOD does not establish its claim.** *"`One`..`Six` each occur exactly once in `IMC_Hero.uasset` (raw scan)"* counts entries in the package's **FName table**, where a name appears once regardless of how many mappings reference it. The scan therefore cannot distinguish "six actions, six distinct digits" from "two actions sharing `One`". **Impact is bounded and cosmetic** — the seam is degrade-open, so any mis-binding yields a wrong-but-harmless chip, ⛔ never a crash, ⛔ never a broken bar. *Real verification: TASK-811's in-PIE pixel check reads the live chips per slot; that is the authoritative answer and it is already on its docket.*
- **[WARN] Additive integrity is NOT independently verifiable under this gate's fences (⛔ no git).** My structural reconstruction of the added regions gives **+62 header / +152 cpp = 214** against the declared **215** — a one-line difference wholly attributable to blank-line attribution in a hand count, ⛔ not evidence of a deletion. **What I *can* attest by reading both files end to end:** every pre-existing symbol is present and unaltered — all three BIE signatures, `GetCardArtTexture`, `GetNextCardArtTexture`, `InitForController`, `RequestPlaySlot`/`RequestDiscardSlot`, all four handlers, all five private helpers, all six members — and the `CARDBAR-§3` pinned block matches CONVENTIONS character-for-character. **TASK-811 owns the authoritative `git diff --stat` and must confirm `215 insertions(+), 0 deletions(-)` across the two source files as part of its `§25b` sweep.**
- **[WARN] ⛔ THE SUITE BASELINE MOVED — `311` IS AN ATTRIBUTION, ⛔ NOT A NUMBER TASK-811 MAY ASSERT.** See §8 below. TASK-807's own contribution is **exactly +5**, verified; but the shared tree already carries TASK-735's +10 and TASK-812's +10, and a module compile compiles all of `Tests/` regardless of lane.
- **[NIT] `CardHandWidget.h:191` names the wrong precedent.** It cites *"the `FSiegeMapMark::MakeSymbol` / `ASummonedUnit::HeightAdvantageMultiplier` idiom"*, while `:200-201` and the handoff correctly cite `ComposeDetailContent`. The **structural** clone is `ComposeDetailContent` (verified identical at `SiegeControlsHelpWidget.h:391-394`); the other two are only precedents for "a plain static that is deliberately not a `UFUNCTION`". Two different things named nine lines apart. Harmless.
- **[NIT] `ComposeSlotKeyLabel` is `public`** where only test-TU reachability is needed. Consistent with the family (`QueryAppliedKeysForRow` is likewise a public static on a `UUserWidget` subclass, and `FSiegeControlsHelpRegistry` is all public statics), so ⛔ no action — recorded only because clause (1) makes surface width the live question.
- **[NIT] The "half two" derived refusal (`.cpp:396-399`) costs one extra `ComposeKeyChipLabel` call on every SUCCESSFUL composition**, on a path the WBP calls 6× per gold tick. Declared honestly as structurally unreachable (an `FKey`'s `ToString` is never empty). **Measured, not assumed:** gold ticks at **1 Hz** (`SiegeGameMode.cpp:1745` — `BaseIncomeTickPeriod=1`), so this is ≤6 short string builds per second, against a path that already `LoadSynchronous`es the `IA_*` asset. ⛔ Not a performance concern. Keep — it makes "empty on every fault" a property of the code rather than of an argument.
- **[NIT] `F-4` (the `Key 1` Footman fallback) correctly DECLARED and ⛔ NOT fixed.** `SiegePlayerController.{h,cpp}` verified untouched by this task (a repo-wide grep for `TASK-807`/`GetSlotKeyLabel`/`ComposeSlotKeyLabel` returns **exactly three files**: `CardHandWidget.h`, `CardHandWidget.cpp`, `Tests/SiegeCardHandKeyLabelTest.cpp`). TASK-735 remains sole occupant of the controller. ✅ Fence held.

---

## 8. ⭐ SUITE — THE ONE NUMBER, RECONCILED

**TASK-807's contribution: exactly `+5`** — `Tests/SiegeCardHandKeyLabelTest.cpp` carries 5 `^IMPLEMENT_SIMPLE_AUTOMATION_TEST` and no complex tests. ✅ The declared delta is honest.

**Lane-A arithmetic: `306 + 5 = 311`.** ✅ Correct as an *attribution*. Baseline 306 is confirmed at source (`handoffs/TASK-803-programmer.md:6` — `301 → 306`; asserted `306/306` by `qa/TASK-804.md:249`).

⛔ **BUT `311` IS ⛔ NOT THE NUMBER TASK-811 MAY ASSERT.** My own census — `^IMPLEMENT_(SIMPLE|COMPLEX)_AUTOMATION_TEST` across `Source/GitClaudeUnrealTest/Siegebound/Tests/*.cpp`, the same column-0 method TASK-803 used — reads **331 across 26 files**, and it reconciles **exactly**:

```
306 baseline  +10 (TASK-735)  +10 (TASK-812)  +5 (TASK-807)  =  331
```

This is corroborated independently: `handoffs/TASK-735-programmer.md:139-140` reaches the same 331 by the same arithmetic and states *"THE INTEGRATION GATE MUST ASSERT THE WAVE TOTAL, ⛔ NOT `316`… TASK-813 / TASK-815 will raise it further. Whoever declares the number last owns it."*

📌 **RULING FOR TASK-811: declare the LIVE census taken at your own compile — currently `331`, and higher if TASK-813/815 land test files first. ⛔ Do NOT assert `311`; it will read as a red suite when nothing is wrong.** `311` is the correct answer to *"what did TASK-807 add?"* and the wrong answer to *"what should the runner report?"*

---

## Notes for build-master (TASK-811)

1. ⭐⭐ **The suite number is `331`, ⛔ not `311`** (§8). Take a fresh `^IMPLEMENT_` census at compile time — 813/815 may raise it again. 0 failures required either way.
2. ✅ **Confirm `git diff --stat` = `215 insertions(+), 0 deletions(-)`** across `CardHandWidget.{h,cpp}` — I could not (⛔ no git under this gate). This is the strongest available proof of BIE integrity; fold it into the `§25b` digest sweep.
3. ⚠️ **The `IMC_Hero` binding is verified only by the pixel check.** WARN-3 explains why the raw uasset scan cannot prove "six actions, six distinct digits". ⭐ **Your in-PIE look at the six chips IS the verification** — if any two cards show the same key, or a card shows a key Jonathan does not press, that is a real finding, ⛔ not a rendering artefact.
4. ⛔ **`GetSlotKeyLabel` takes a parameter ⇒ it CANNOT be a UMG property binding.** The board's TASK-809 wording *"its text bound to `GetSlotKeyLabel(SlotIndex)`"* must be read as a **call + `SetText` inside the `OnHandSlotUpdated` handler** — the `GetCardArtTexture` PULL shape (`TASK-079` ruling 3), which is exactly what the handoff specifies. If TASK-809 reports it wired as a `Bind` dropdown, that is wrong and could not have compiled. ⭐ **Empty return ⇒ `Collapsed`, ⛔ never a placeholder glyph.**
5. ⚠️ **`WBP_CardHand` is itself a duplicate of `WBP_HUD`** (`CARDBAR-§4`) — already in the hazardous duplicate+reparent class, where design-time has looked perfect while **runtime** repaint was broken (~9 wasted fixes). ⛔ A property readback is not acceptance here; your PIE look is the only gate that counts.
6. Record WARN-1 (the untested live lambda) in your handoff so the manager can board a follow-up if it wants the regression guard closed. ⛔ It is not a TASK-807 defect and must not re-open this gate.

**Gate status: TASK-807 → `qa-passed`.** ⛔ The board flip is the orchestrator's — this gate wrote no TASKBOARD entry.
