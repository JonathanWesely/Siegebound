# TASK-807 — the `GetSlotKeyLabel` PULL seam — handoff

**Agent:** gameplay-programmer · **Status:** `ready-for-qa` · **QA gate:** TASK-810 · **Compile + suite + pixels:** TASK-811
**Law:** `CARDBAR-§2` · `CARDBAR-§3` · `CARDBAR-§5` · `KBD-§4` · `HELP-§1` · `SC-§33` · `SC-§36.1` · `SC-§37`

---

## 1. ⭐ THE SEAM TASK-809 CONSUMES

```cpp
UFUNCTION(BlueprintCallable, Category = "Siegebound|UI")
FString GetSlotKeyLabel(int32 SlotIndex);
```

`UCardHandWidget` — `Source/GitClaudeUnrealTest/Siegebound/CardHandWidget.h:182-183`, character-for-character
from `CARDBAR-§3`'s pinned block, with the pinned doc comment above it.

**Contract for the WBP (TASK-809):**

| SlotIndex | Returns |
|---|---|
| `0..5`, key resolvable | the chip text for `IA_Card{Slot+1}`'s **applied `IMC_Hero` key** — e.g. `"1"`, or whatever it is rebound to |
| out of range, or nothing resolvable | **EMPTY `FString`** ⇒ ⭐ **HIDE the chip** (`CARDBAR-§3` degrade-open) |

⛔ It never returns `"(not bound)"`, never the pointer chip `"Mouse click"`, never `"?"`, never a guessed digit.
⛔ Call it from the `OnHandSlotUpdated` handler with the same `SlotIndex` the BIE delivered — it is a PULL,
exactly like `GetCardArtTexture` (`TASK-079` ruling 3).

---

## 2. ⭐⭐ PROOF THE LABEL IS TRANSLATED **EXACTLY ONCE**

### 2a. The chain, and where the one translation happens

```
GetSlotKeyLabel(Slot)
  └─ ComposeSlotKeyLabel(Slot, LayoutSubsystem, provider)
       ├─ FSiegeControlsHelpRegistry::FindAction("Cards.Play")      ← the SHIPPED row
       ├─ narrow Actions/QwertyReferenceKeys to [Slot]
       ├─ provider  →  USiegeControlsHelpWidget::QueryAppliedKeysForRow(SlotRow, ObservedController)
       │                 ⭐ THIS CALL *IS* THE TRANSLATION. QueryKeysMappedToAction reads the ACTIVE
       │                    contexts, which ARE the duplicate USiegeKeyboardLayoutSubsystem already
       │                    retargeted in place (SiegeKeyboardLayoutStatics.cpp:236, applied at
       │                    HeroCharacter.cpp:271-275). It calls GetPositionalKey ZERO times
       │                    (SiegeControlsHelpWidget.h:885, asserted at .cpp:2946-2951).
       ├─ FSiegeControlsHelpRegistry::ResolveRowDisplayKeys(...)    ← Lane-A primary branch hands the
       │                                                              applied keys through VERBATIM
       │                                                              (SiegeControlsHelpWidget.cpp:1180-1186)
       └─ FSiegeControlsHelpRegistry::ComposeKeyChipLabel(...)      ← the ONLY place a key becomes characters
```

**⛔ ZERO new key-resolution code was written anywhere.** Every step above is a call into the shipped
resolver — the same one the `TAB` screen's `Cards.Play` row renders. ⇒ the card bar and the help screen
cannot disagree, and a rebind of `IMC_Hero` moves both with zero code edits.

### 2b. The grep QA (TASK-810) should run — and how to read it

```
grep -n "GetPositionalKey" Source/GitClaudeUnrealTest/Siegebound/CardHandWidget.{h,cpp}
```
**6 hits, ⛔ ALL of them comments or an include comment, ⛔ ZERO call expressions:**
`CardHandWidget.cpp:13` (include comment), `:304`, `:375`, `:421` (block/inline comments) ·
`CardHandWidget.h:180`, `:206` (doc comments). ⭐ Every hit is a **warning against** the call, deliberately
placed so the next editor reads the trap before writing the line. **No `GetPositionalKey(` appears.**

### 2c. ⭐⭐ THE DVORAK FIXTURE — and why a Dvorak fixture ALONE would not have been enough

`Tests/SiegeCardHandKeyLabelTest.cpp`, **test 2** (`…IsNeverDoubleTranslated`), the keystone.

⚠️⚠️ **The dispatch's own warning was half right and the missing half is the interesting part.** A Dvorak
fixture is necessary — and for THIS feature it is **still blind on its own**, because `KBD-§4` deliberately
leaves the number row untranslated. `GetSlotKeyLabel(0)` reads `"1"` on US-Dvorak under a correct
implementation **and** under a double-translating one. So the test does three things:

1. **Section 1 — fixture self-check.** `Once = GetPositionalKey(F)` ≠ `F`, and `Twice = GetPositionalKey(Once)`
   ≠ `Once`. ⇒ the injected map genuinely moves the key **twice**, so the defect is *detectable at all*.
   (`F → U → G`, lifted from the shipped fixture at `Tests/SiegeKeyboardLayoutTest.cpp:204-229` — ⛔ not
   re-derived, per that file's own drifting-copy warning.)
2. **Section 2 — ⭐⭐ THE DIGIT-BLINDNESS PROOF, asserted rather than commented.** For all six of
   `Cards.Play`'s reference keys it asserts `GetPositionalKey(key) == key` **and**
   `GetPositionalKey(GetPositionalKey(key)) == GetPositionalKey(key)`. ⇒ **a digit fixture provably cannot
   detect a double translation, at any number of hops.** That is why section 3 uses a **letter**, and the
   assertion stops anyone "simplifying" it back to a `1`. It also encodes `KBD-§4` as a live test: if digits
   were ever added to the table, this goes red and the message names the clause.
3. **Section 3 — the claim, on a LETTER, through the mapped lane.** The provider answers `{ Once }`
   (i.e. "`IA_Card1` is bound to the `F` position and the layout already retargeted it"). Asserts:
   - label **==** `Once`'s short display name (handed through unchanged), and
   - ⛔ label **!=** `Twice`'s short display name — **the line that goes red the instant anyone adds a
     `GetPositionalKey` call to this path**, and
   - the same applied key reads **identically on the QWERTY and Dvorak fixtures** (the mapped lane is
     layout-independent by construction; a residual translation would make them diverge), and
   - a **null** subsystem does not change the mapped lane's answer (`KBD-§5` fail-safe).

⭐ Every expected string is **derived** from `FKey::GetDisplayName(false)` or from the shipped registry row.
**There is no literal `"1"` anywhere in the test file** — checked: `grep -E 'TEXT\("[1-6]"\)'` returns nothing
across all three touched files.

---

## 3. ✅ VERIFICATION THAT `IA_Card1..6` EXIST AND ARE BOUND (done BEFORE any code was written)

| Check | Result |
|---|---|
| assets exist | ✅ `Content/Input/Actions/IA_Card1.uasset` … `IA_Card6.uasset` — all six on disk |
| bound in `IMC_Hero` | ✅ all six names present in `Content/Input/IMC_Hero.uasset` (raw scan, six for six) |
| the keys are the digit row | ✅ `One`,`Two`,`Three`,`Four`,`Five`,`Six` each occur **exactly once** in `IMC_Hero.uasset` — i.e. six actions, six distinct digits, no doubles |
| the resolver returns the **already-retargeted** key (ONE translation, ⛔ not two) | ✅ `USiegeControlsHelpWidget::QueryAppliedKeysForRow` (`SiegeControlsHelpWidget.h:879-893`, `.cpp:2909-2959`) — its own contract says *"⛔ IT APPLIES NO TRANSLATION OF ITS OWN AND CALLS GetPositionalKey ZERO TIMES"*, and `ResolveRowDisplayKeys`' Lane-A primary branch (`.cpp:1180-1186`) returns `AppliedKeys` verbatim |
| the resolver is reachable (not `private`/awkward) | ✅ `QueryAppliedKeysForRow` is a **public static** on `USiegeControlsHelpWidget` (`.h:893`, above `protected:` at `:895`); `FSiegeControlsHelpRegistry` is a `GITCLAUDEUNREALTEST_API` struct of public statics. ⇒ **no `FINDING` owed under spec clause (3)**, no access widened, nothing forked |
| `Cards.Play` carries all six actions in SLOT ORDER | ✅ `SiegeControlsHelpWidget.cpp:420-427` — `Actions = {IA_Card1..IA_Card6}`, `QwertyReferenceKeys = {One..Six}`, index-parallel. **Asserted in test 1** so a reorder cannot pass silently |

---

## 4. FILES TOUCHED

| File | Change |
|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/CardHandWidget.h` | **+63 lines, ⛔ 0 deletions.** `GetSlotKeyLabel`, `ComposeSlotKeyLabel`, `WarnedKeyLabelSlots`, 2 includes, 2 forward decls |
| `Source/GitClaudeUnrealTest/Siegebound/CardHandWidget.cpp` | **+152 lines, ⛔ 0 deletions.** The two function bodies + 4 includes |
| `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeCardHandKeyLabelTest.cpp` | **NEW.** 5 tests |

⭐ **`git diff --stat` reads `215 insertions(+)`, `0 deletions(-)` across both source files.** That is the
strongest available proof for QA's gate that **the three BIE signatures are byte-identical to before**:
nothing in the file was deleted or modified at all. ⛔ No BIE change, ⛔ no model-side member, ⛔ no other file.

---

## 5. ⚠️ FLAGGED DECISIONS — QA should scrutinise these first

### F-1 ⚠️ **A SECOND SYMBOL WAS ADDED, AND THE SPEC SAID "EXACTLY ONE". I am declaring it, not hiding it.**

`static FString UCardHandWidget::ComposeSlotKeyLabel(int32, const USiegeKeyboardLayoutSubsystem*, TFunctionRef<TArray<FKey>(const FSiegeControlsHelpAction&)>)`
— ⛔ **a plain C++ static, ⛔ NOT a `UFUNCTION`, ⛔ not a Blueprint surface, ⛔ not a BIE.**

**Why it is not optional, stated as the trade it is:** spec clause (6) and the dispatch both require an
assertion that can FAIL, and the *only* instrument that can discriminate a double translation is a fixture
that pushes a **letter** through the **applied** lane on a **simulated Dvorak** layout. Both of those inputs
are live (`ObservedController` → Enhanced Input; the layout subsystem → the game instance) and neither
exists in an automation run. Without an injection seam the suite could only reach the world-less fallback
path — which, for digits, is `1 == 1`: `SC-§37`'s own definition of an assertion that proves nothing.

**Why this shape and not another:** it is a verbatim clone of
`FSiegeControlsHelpRegistry::ComposeDetailContent` (`SiegeControlsHelpWidget.h:371-394`), which takes the
layout subsystem as a nullable pointer and Enhanced Input as a `TFunctionRef` **for exactly this reason**.
⛔ No new idiom was invented. `SC-§36.1`'s checkable tell is satisfied in the same task and the same file:
its only caller is `GetSlotKeyLabel`, immediately below it.

⚖️ **If QA rules the second symbol out of bounds, the fix is a rollback of the split, and it costs test 2 and
test 3's discriminating power.** I would rather the ruling be made explicitly than have the seam smuggled in.

### F-2 ⚠️ **The registry row is COPIED and narrowed, ⛔ not rebuilt.**
`SlotRow = *PlayRow;` then `Actions`/`QwertyReferenceKeys` narrowed to `[SlotIndex]`. This carries `Lane`,
`bLiteralKeyLabel` and `bPointerOnly` across from the one row the `TAB` screen resolves. A hand-built
synthetic row would look cleaner and would **silently stop tracking** on the first edit to `Cards.Play` —
the exact second-copy-of-the-truth failure `CARDBAR-§2` exists to prevent.

### F-3 ⭐ **The placeholder refusal is in two halves, and the second one is derived, not typed.**
`ComposeKeyChipLabel` answers `"(not bound)"` / `"Mouse click"` when it has no key — **correct for the TAB
screen** (`HELP-§2` mechanism 2 wants a *visible* gap) and **wrong for a card chip**. Both constants are
file-local to `SiegeControlsHelpWidget.cpp` and unreachable from here, so:
- **half one, structural:** refuse the lane up front (`bPointerOnly || Lane != MappedAction`) and refuse an
  empty `DisplayKeys` before the composer is ever asked;
- **half two, derived:** ask the **same composer** what it says with nothing (`ComposeKeyChipLabel(SlotRow, {})`)
  and refuse exactly that answer. ⛔ Typing `"(not bound)"` here would be a second copy that rots on a
  reword; deriving it cannot. ⚠️ **Declared honestly: half two is structurally unreachable today** (an
  `FKey`'s `ToString` is never empty). It stays because it makes "empty on every fault" a property of the
  code rather than of an argument, and it costs one call on a path that already loads an asset.

### F-4 ⚠️ **DECLARED, ⛔ NOT FIXED — the `Key 1` legacy quirk (spec clause 5).**
`ASiegePlayerController::OnCard1Pressed` falls back to always-available Footman placement when hand slot 0
is empty. ⭐ With a real 6-card hand slot 0 is never empty (§3.4 redraws immediately), so the label is
truthful in play, and the shipped `TAB` help already documents the quirk verbatim
(`SiegeControlsHelpWidget.cpp`, the `Cards.Play` detail's closing sentence). **Out of fence
(`SiegePlayerController.{h,cpp}` is TASK-735/813/815's), untouched, and ⛔ not tidied by this task.**

### F-5 ⚠️ **The log message deliberately derives nothing from `SlotIndex`.**
An earlier draft printed `IA_Card%d` from `SlotIndex + 1`. That is **signed-overflow UB on `MIN_int32`** —
and test 4 passes `MIN_int32`. The message now prints the raw index only. ⚖️ *A diagnostic must not be the
one line in the function that misbehaves on the input it exists to report.*

### F-6 ⚠️ **`TFunctionRef` is bound to the FUNCTION, ⛔ never to `&function`.**
`TFunctionRef` does not coerce a function type to a pointer, so the ampersand form passes a prvalue pointer
and does not compile — recorded verbatim at `SiegeKeyboardLayoutStatics.h:42-46`, which learned it once
already. The test file passes `NoAppliedKeys` bare, and carries the warning at its definition.

---

## 6. TESTS — `Tests/SiegeCardHandKeyLabelTest.cpp` (NEW FILE)

**⛔ CHECKED FIRST, per spec clause (6): no existing card/hand test frame fits.** `Tests/` holds 23 files and
**none** is a card-hand subject. `SiegeDeckSlotsTest.cpp` is the **deck-builder's** fixed slots (a save-game
migration subject), and `SiegeControlsHelpTest.cpp` is the `TAB` overlay's own suite, which `HELP-§6` binds
to ONE file for ONE feature. ⇒ **new frame, declared** — ⛔ not a duplicate.

| # | Test | ⭐ The property it measures — and how it FAILS |
|---|---|---|
| 1 | `Siegebound.CardHand.SlotKeyLabelNarrowsToItsOwnCardAction` | Slot N's label comes from `IA_Card{N+1}` **and nothing else**. Each slot gets a **distinct letter** as its applied key, answered by the provider *keyed off the action the implementation actually asked about*. ⇒ **RED** on an off-by-one, on a narrowing that always reads action 0, and on no narrowing at all (which would render all six keys on every card). Also asserts the narrowed row is exactly ONE action, inherits `Cards.Play`'s lane, and that the registry's action list is in SLOT ORDER |
| 2 | `Siegebound.CardHand.SlotKeyLabelIsNeverDoubleTranslated` ⭐⭐ | **The keystone.** §2c above. ⇒ **RED** the instant a `GetPositionalKey` call enters this path; **RED** if digits ever enter the translation table; **RED** if the fixture stops being able to detect the defect |
| 3 | `Siegebound.CardHand.SlotKeyLabelPrefersTheLiveBindingOverTheReferenceKey` | Rebinds slot 0 to `SpaceBar` in the fixture. ⇒ **RED** on an implementation that renders `QwertyReferenceKeys[Slot]` — which would look **perfect today** (it would print 1..6) and is a hardcoded digit wearing a resolver's clothes. Also asserts the stale reference key does not appear *at all* (catches an append), and that the unmapped fallback still derives each slot's own reference key (catches "ignores the registry") |
| 4 | `Siegebound.CardHand.SlotKeyLabelDegradesToEmptyNeverAPlaceholder` | Six out-of-range indices incl. `MAX_int32`/`MIN_int32` → **exactly empty**, and ⛔ **not** the two placeholders — which are **derived by asking the shipped composer**, with a fixture self-check that both are non-empty so the assertions are not vacuous. ⭐ Also asserts the provider is **never invoked** for a bad slot (the range check precedes the live query — a late check would still return empty and look fine). ⭐ And asserts valid slots are **not** degraded, so "always return empty" cannot pass this test |
| 5 | `Siegebound.CardHand.SlotKeyLabelWarnsOncePerSlotNotPerCall` | Drives the **live** `GetSlotKeyLabel` on a real `UCardHandWidget`. `AddExpectedMessagePlain(..., Occurrences 2)` over **ten** calls spanning two distinct bad slots. ⇒ **RED at 10** on a per-call log (the real bug: slots re-push on every gold tick), **RED at 0** if the warning is removed, **RED at 8** if a healthy slot warns. Also asserts the live path composes through the same seam |

### ⭐ SUITE DELTA — **306 → 311 (+5)**

Counted, ⛔ not assumed: `grep -c IMPLEMENT_SIMPLE_AUTOMATION_TEST|IMPLEMENT_COMPLEX_AUTOMATION_TEST` over
`Siegebound/Tests/*.cpp` = **306** before (matching the declared total), **311** after.
📌 **TASK-811: declare 311.**

---

## 7. AIRLOCK / M8 / FENCES

- **M8 (`CARDBAR-§5`):** ⛔ **no replicated property, ⛔ no RPC, ⛔ no relevancy change.** `GetSlotKeyLabel` is
  a **client-local, read-only presentation pull** over an already-authoritative model — it reads the shipped
  registry (a static table), Enhanced Input's local key mappings, and the local game instance's layout
  subsystem. It writes nothing but a local spam-guard `TSet`. `RequestPlaySlot`/`RequestDiscardSlot` are
  untouched pass-throughs; the controller keeps every refusal.
- **Airlock:** ⛔ no `Capture()`, ⛔ no `EnsureSnapshot()`, Zone A untouched, ⛔ no token figure.
- **Fences honoured:** ⛔ no `WBP_CardHand` or any WidgetBlueprint (TASK-808 measures, TASK-809 edits) ·
  ⛔ no `SiegePlayerController.{h,cpp}` (TASK-735/813/815 serialise on it) · ⛔ `IMC_Hero`, `IA_Card1..6`,
  `DT_Cards`, `WBP_HUD` and every `HELP-§` row untouched · ⛔ **no compile** (TASK-811 owns it) · ⛔ no editor,
  ⛔ no MCP, ⛔ no Git.
- **Line endings:** all three files uniformly CRLF, matching the repo. ⛔ No mixed endings introduced.

---

## 8. WHAT QA (TASK-810) SHOULD SCRUTINISE

1. ⭐⭐ **The `GetPositionalKey` grep — and read §2b before calling the 6 hits a failure.** All six are
   comments warning against the call. ⛔ There is no call expression.
2. ⚠️ **F-1, the second symbol.** It is a declared trade, not an oversight. Rule on it explicitly.
3. ✅ **BIE integrity:** `git diff --stat` = `215 insertions(+), 0 deletions(-)`. Nothing was modified.
4. ⭐ **`SC-§37`:** confirm no test transcribes a value. There is no `TEXT("1")`..`TEXT("6")` in any touched
   file; every expectation is derived from the registry row, from `FKey::GetDisplayName(false)`, or from the
   shipped chip composer.
5. ⚠️ **The `false` in `GetDisplayName(/*bLongDisplayName=*/false)`** — the engine's default is `true`, and
   omitting it in the test would silently make every expectation disagree with the shipped chip. It is
   explicit in the one helper that names keys.
6. ⭐ **Warn-once:** the guard is `TSet<int32> WarnedKeyLabelSlots`, cloned from `WarnedCardArtIDs`, and it is
   measured by test 5 rather than argued.
