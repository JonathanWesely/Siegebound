# TASK-510 — [KBD-2] `SiegeKeyboardLayoutTest.cpp` — 7 tests that go green on a QWERTY machine

**Agent:** gameplay-programmer · **Status:** `ready-for-qa` · **Date:** 2026-08-04
**QA gate:** **TASK-513** → `.claude/pipeline/qa/TASK-513-keyboard-layout.md` (`SC-§29`)
**Law implemented:** CONVENTIONS **`KBD-§8` (the pin)** · `KBD-§4` · `KBD-§5` · `KBD-§7` · `SC-§13` · `SC-§32`
**Subject read before writing:** TASK-509's finished `SiegeKeyboardLayoutStatics.{h,cpp}` (per RULING 1 — this is the independent-check dispatch)

## ⛔ M8 DECLARATION (verbatim)

**adds no replicated property, no new replicated class, no new relevancy tier.**

It is stated verbatim in the test file's header comment too. This is a test file behind
`#if WITH_DEV_AUTOMATION_TESTS`; it adds no shipped surface of any kind.

---

## 1. Files touched — EXACTLY ONE, new

| file | status |
|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeKeyboardLayoutTest.cpp` | **new** (1443 lines) |

⛔ **Nothing else was opened for edit.** No `Build.cs` change (`InputCore` + `EnhancedInput` are
already public — `GitClaudeUnrealTest.Build.cs:14-16`). ⛔ Not `SiegeKeyboardLayoutStatics.{h,cpp}`
(TASK-509's, frozen) · ⛔ not `SiegeKeyboardLayoutSubsystem.{h,cpp}` (TASK-511's, in flight) ·
⛔ not `HeroCharacter.cpp` (TASK-512's). ⛔ **No compile, no test run, no Git, no editor/MCP/PIE** —
TASK-514 owns the batch's only compile and only test run.

**Board:** TASK-510 `in-progress` at start, `ready-for-qa` at finish. Nothing else edited.

---

## 2. The seven tests, as registered

| # | registered name | class |
|---|---|---|
| 1 | `Siegebound.Input.ScanCodeTable` | `FSiegeKeyboardLayoutScanCodeTableTest` |
| 2 | `Siegebound.Input.DvorakTranslation` | `FSiegeKeyboardLayoutDvorakTranslationTest` |
| 3 | `Siegebound.Input.QwertyIsPassThrough` | `FSiegeKeyboardLayoutQwertyIsPassThroughTest` |
| 4 | `Siegebound.Input.DegenerateProbesAreRefused` | `FSiegeKeyboardLayoutDegenerateProbesTest` |
| 5 | `Siegebound.Input.RetargetPreservesModifiers` | `FSiegeKeyboardLayoutRetargetPreservesModifiersTest` |
| 6 | `Siegebound.Input.NonIdentityDoesNotCompound` | `FSiegeKeyboardLayoutNonIdentityDoesNotCompoundTest` |
| 7 | `Siegebound.Input.LiveResolverMatchesRuntime` | `FSiegeKeyboardLayoutLiveResolverTest` |

All seven use `IMPLEMENT_SIMPLE_AUTOMATION_TEST` with
`EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter` — the
`SiegeSettingsTest.cpp:113-116` pattern, inside `#if WITH_DEV_AUTOMATION_TESTS`.

---

## 3. ⭐ HOW THE "GREEN ON QWERTY" PROPERTY IS ACTUALLY ACHIEVED (`KBD-§9` criterion 9)

⚖️ **The claim QA should test is not "a fake resolver exists" but "no assertion depends on the
host layout."** Here is the audit, path by path.

**The seam.** `BuildTranslationMap(Probes, Resolver, OutMap)` reads no engine state whatsoever
(verified by reading TASK-509's `.cpp`: no globals, no singletons, no platform calls). Probe data
in, resolver injected, map out. Tests 2, 3, 4 drive it entirely with hand-built data.

**⭐ The fake resolver is a MODEL of the engine, not an answer key** —
`SiegeKeyboardLayoutTestUtils::ResolveLikePlatform`. It is **character-driven**, and that is a
verified engine fact rather than a convenience:

- `GetKeyFromCodes` tries `KeyMapVirtualToEnum` then falls back to `KeyMapCharToEnum`
  (`InputCoreTypes.cpp:1595-1611`).
- On Windows `KeyMapVirtualToEnum` is built by `FWindowsPlatformInput::GetKeyMap`
  (`WindowsPlatformInput.cpp:6-121`), which registers mouse/control/F-keys/numpad and ⛔ **no
  letter VKs at all.** It then collects the OEM VKs and **removes every one whose character is
  already in the printable char map** (`:105-110`) — so `VK_OEM_COMMA` / `VK_OEM_PERIOD` /
  `VK_OEM_7` / `VK_OEM_1` are ⛔ **not in the VK map either**.
- `KeyMapCharToEnum` comes from `FGenericPlatformInput::GetStandardPrintableKeyMap`
  (`GenericPlatformInput.cpp:5-95`) — a **STATIC table**, ⭐ **layout-independent**.

⇒ For every key in scope (`KBD-§4`, letters only) the engine resolves **by character**, and the
model reproduces that rule. `FKey(TEXT("W"))` **is** `EKeys::W` (FKey compares by KeyName,
`InputCoreTypes.h:109`; `EKeys::W` is `FKey("W")`, `InputCoreTypes.cpp:88`) — the same construction
`InitKeyMappings` uses at `:1581`.

**The two environment-sensitive spots, both log-not-fail, both flagged in-file:**

| where | why it can see the machine | behaviour |
|---|---|---|
| test 7, the two probe pairs | reads the live `FInputKeyManager` | `AddInfo` on mismatch, ⛔ never `AddError`/`AddWarning` |
| test 3, the ONE subsystem pointer claim | TASK-511's `GetPositionalContext` is specified to **re-probe on every call** (spec 4a), which can overwrite the forced empty map | re-reads `IsPositionalRemapActive()` **after** the call; asserts the same-pointer claim only if the map really stayed empty, else `AddInfo` |

⚖️ **Both for the same reason: a hard assert there red-fails on Jonathan's own Dvorak machine —
the exact machine this feature exists to serve.** ⛔ The non-null claim in test 3 is
**unconditional**, because `KBD-§5` admits no environment in which null is acceptable. Everything
else in the file is a hard assertion.

---

## 4. ⚠️ THE COLLISION RULING — HOW I HANDLED IT, AND WHAT TASK-513 IS BEING ASKED

**I asserted FIRST-WINS — the behaviour TASK-509 actually implemented
(`SiegeKeyboardLayoutStatics.cpp:161-168`) — and I marked the two assertions in the source with
`⚖️ RULING` so the inversion point is one grep away.** The test-file comment above
`DegenerateProbesAreRefused` states the conflict in full, names both authorities, and says plainly
that TASK-513 owns the decision. ⛔ **Nothing is encoded as if it were settled.**

The two assertions that move if the ruling goes the other way (test 4, CASE C):

```cpp
TestEqual(TEXT("Collision (first-wins, TASK-509 as implemented): exactly ONE translation survives"), Num, 1);
TestTrue(TEXT("Collision (first-wins): the EARLIER probe S keeps its translation to O"), ...);
```
Under mutual refusal these become `Num == 0` and `TestFalse(... Contains(EKeys::S))`. **The rest of
CASE C holds under either reading** and is labelled as such: the later probe is refused, and
⛔ **nothing is ever left holding `EKeys::Invalid`.**

**⚖️ CONTEXT FOR THE RULING, so it is decided on the merits rather than on which document was read
last:**

- **First-wins preserves ONE working binding; mutual refusal preserves NONE.** On a layout where
  two positions genuinely collided, first-wins leaves one action reachable.
- **Its one theoretical cost:** a *refused* probe's untranslated key can itself be the target of
  some *accepted* probe, producing one double-bound key. TASK-509 states this residual openly
  (handoff §5.1) rather than hiding it.
- ⭐ **It is unreachable on every layout this project cares about.** TASK-509 hand-traced
  US-Dvorak, AZERTY and QWERTZ: **zero refusals on all three.** Every Latin layout is a bijection
  over the 26 letter positions, so no two positions can claim one target. **I independently
  mechanised the Dvorak trace as this file's fixture and reached the same 24 accepted / 2 identity
  (`A`, `M`) split** — an independently-authored table that agrees with the hand trace.
- ⛔ **Under either reading the `KBD-§5` floor holds:** never null, never `EKeys::Invalid`, never
  a partial context.

**My recommendation, offered not asserted: keep first-wins.** It is what `KBD-§5`'s own table says
in the singular ("**that letter** keeps its source key"), it is strictly better on the reachable
cases, and the plan's "each" is one word in a summary line, not a designed behaviour.

**Test 4 CASE E** asserts a second thing the pin left open and TASK-509 chose (handoff §5.3):
a **duplicated source row → first row wins**. Same treatment — flagged in-file as an open choice,
asserted as implemented, because an order-dependent result in a pure function is the kind of thing
that ships silently.

---

## 5. What each test actually proves (and the assertions that are stronger than they look)

### 1 · `ScanCodeTable`
26 rows · all 10 anchors (`Q=0x10 W=0x11 E=0x12 R=0x13 T=0x14 A=0x1E S=0x1F D=0x20 F=0x21 C=0x2E`)
asserted **by FKey lookup, never by index**, so a reorder cannot satisfy them · 26 distinct scan
codes · 26 distinct FKeys · every row's FKey valid · every scan code non-zero ·
⭐ **every row leaves `VirtualKey`/`CharCode` at 0** (a pre-filled table would be a second source of
truth and would make the whole file host-dependent) · ⭐ **alphabetical A..Z order asserted**, because
`KBD-§8` pins `DescribeActiveTranslation` to that order and a reorder silently changes a string
TASK-511 must produce byte-for-byte.

### 2 · `DvorakTranslation` ⭐ the keystone
The fixture is **the shipped `GetQwertyLetterScanCodes()` table with the Dvorak VK/CharCode pairs
filled in by FKey lookup** — not hand-rolled from nothing — so it cannot silently disagree with the
shipped table about which letter sits at which scan code. **Fixture self-check runs first** (26 rows,
26 filled). Then: 24 non-identity entries · all nine bound keys asserted individually
(`W→Comma, S→O, D→E, Q→Apostrophe, E→Period, R→P, T→Y, F→U, C→J`) · ⭐ **`A` and `M` asserted as
ABSENCES**, not as identity entries (listing them as `To == From` would have made the loop pass on a
map that wrongly *contained* an identity) · no value is `EKeys::Invalid` · no entry is an identity ·
⭐ **injectivity on real data: 24 sources → 24 distinct targets** · and the map is asserted to really
contain the chained pair `D→E` + `E→Period`, so test 6 is provably not vacuous.

### 3 · `QwertyIsPassThrough`
Identity probes ⇒ `BuildTranslationMap` returns **0**. The out-map is **pre-seeded with a stale
`W→Comma`** so the `Reset()` contract is actually tested rather than assumed. Then, through
`USiegeKeyboardLayoutSubsystem` + `SetTranslationMapForAutomationTests`: **same pointer** returned
(pointer identity, not content equality — a duplicate that compared equal would still be an
allocation), plus `GetPositionalContext(nullptr) == nullptr`.

### 4 · `DegenerateProbesAreRefused`
Six sub-cases: **A** `VirtualKey == 0` · **B** resolver → `EKeys::Invalid` · **C** the collision
(⚖️ the ruling, §4 above) *including a real retarget proving the refused key survives on `D` and
nothing becomes Invalid* · **D** a malformed row (invalid `QwertyKey`) · **E** a duplicated source
row · **F** ⭐ the **non-Windows path**: the shipped table entirely unfilled ⇒ 0 translations —
and this one uses the **live** resolver yet is still host-independent, because every probe carries
`VirtualKey == 0` and the skip happens *before* the resolver is called, so it provably never runs.

### 5 · `RetargetPreservesModifiers` ⭐ TASK-445 mechanised
Synthetic IMC, `SwizzleAxis(YXZ)` + `Negate{false,true,false}`, `DuplicateObject`, retarget. Asserts:
Key became `Comma` · the untranslated `A` mapping kept its source key · ⛔ **the SOURCE still holds
`W` and still has both modifiers** (`KBD-§1` — in PIE the source *is* the loaded `IMC_Hero`) ·
counts match · classes match pairwise · ⭐ **modifier pointers DIFFER pairwise** (real deep copy) ·
⭐ **`bX/bY/bZ` still `false/true/false`, asserted as three separate claims** · Swizzle `Order` still
`YXZ` · the `UInputAction` reference **preserved by pointer** (it is not a subobject).

⭐ **A trigger is carried too** — `UInputTriggerHold` with `HoldTimeThreshold = 0.42f` and
`bIsOneShot = true`, both non-default. `Triggers` sits in the same ban clause and the same instanced-array
hazard as `Modifiers`, and a `0 == 0` count comparison would have been exactly the vacuous assertion
the board's spec item (4) warns about.

**Why the deep copy happens at all, stated because it is the mechanism and not luck:** the modifiers
are created with the **IMC as their Outer**, so `IsIn(SourceObject)` is true and
`StaticDuplicateObject` duplicates them and remaps the references. The `UInputAction` is created with
the **transient package** as Outer, so it is not a subobject and its reference is preserved. Both
facts are asserted, in opposite directions.

### 6 · `NonIdentityDoesNotCompound`
Five mappings (`W A S D E`), the real Dvorak map including the chained pair. Two claims:
**(a) no cascade even on the FIRST call** — `D → E` and stops, not `D → E → Period`; **(b) the second
call changes nothing** — every key snapshot-compared, `D` specifically re-asserted, and ⛔ the
modifier array untouched **by object identity** (proving `.Key` really is the only field written).
⚠️ **`OutNumRetargeted` is asserted to be the SAME 4 on the repeat, not 0** — it is derived from
Source vs the translation, not a diff, and a 0 there would mean the function had started reading its
own output. Plus the wholesale-refusal ladder: length mismatch (refused, target unwritten,
`OutNumRetargeted == 0`), null Source, null Target.

### 7 · `LiveResolverMatchesRuntime`
`ResolveKeyFromCodes('W','W') == EKeys::W` and `ResolveKeyFromCodes(0xBC, ',') == EKeys::Comma`,
**reported via `AddInfo`, never failed** — with the reason written out in full (the VK map is rebuilt
per OS layout; a red here would mean "your keyboard is unusual", which trains people to ignore the
suite). ⛔ It also carries a **do-not-extend warning**: `GetKeyFromCodes` *synthesizes and permanently
registers* a new FKey for any `CharCode > 32` it cannot name (`InputCoreTypes.cpp:1604-1610`), so a
test that swept a character range would pollute the editor's global key registry for the session.

⭐ **It does carry one HARD claim that is true on every host:** `ResolveKeyFromCodes(0, 0)` must be
`EKeys::Invalid`. VK 0 is in no platform key map, char 0 is in no char map, and 0 is below the `> 32`
synthesis threshold — so this is layout- **and** platform-independent, and it is the claim that
matters (`KBD-§5`: the danger is a resolver that *invents* an answer).

---

## 6. ⚠️ THE ONE NUMBER IN THIS FILE THAT IS NOT VERIFIED — AND WHY IT IS SAFE ANYWAY

⛔ **The Dvorak `VirtualKey` column was DERIVED, NOT OBSERVED.** Nobody in this pipeline has a Dvorak
machine. The VKs (`0xDE` for the `'` at the Q position, `0xBC` at W, `0xBE` at E, `0xBA` at Z, and
the letter VKs elsewhere) are what US-Dvorak is understood to assign, reasoned from the character
each position produces. **TASK-515 prints the live probe and is authoritative.** The provenance note
sits directly above the fixture in the source, not only here.

✅ **AND THE REASON A WRONG VK CANNOT BREAK THIS TEST — this is the design point, please check it:**

1. The engine resolves these keys **by CharCode** (§3 above, verified against engine source).
2. `BuildTranslationMap` uses `VirtualKey` **only** as the non-zero "the probe answered" gate
   (`SiegeKeyboardLayoutStatics.cpp:129-133`).

⇒ **Not one assertion in `DvorakTranslation` changes if TASK-515 reports different VKs.** The
`CharCode` column is the load-bearing one, and it is simply the US-Dvorak character at each QWERTY
position — not in doubt. If TASK-515 does report different VKs, the fix is a data edit to
`FindDvorakCell`'s table with **no assertion churn**.

---

## 7. 🔍 WHAT QA SHOULD SCRUTINISE

| criterion (`KBD-§9`) | where it lands here |
|---|---|
| **2** no banned API on a shipped path | ⚠️ **`MapKey` APPEARS IN THIS FILE — ONCE, IN `MakeSourceContext`, AS TEST SCAFFOLDING.** `KBD-§2` bans it on **shipped** paths; here it builds a fixture from nothing (no asset to preserve, modifiers attached explicitly on the next lines). ⛔ **Please read the in-file note before filing this.** ✅ The engine's own comment at `InputMappingContext.h:224` says these accessors are "only used in the editor and **for tests**". And the ban is checked **behaviourally** too: an unmap+map reimplementation of `RetargetContextKeys` would produce empty modifiers, which is exactly what test 5 asserts against. |
| **8** `TestEqualSensitive` not `TestEqual` for FString | ⭐ **There is not a single FString claim in this file** — grep it: zero `TestEqual` calls on an `FString`. Every `FString::Printf` is a *message*, never a compared value. The one place `SC-§13` would bind is a `DescribeActiveTranslation()` byte claim, and I **deliberately did not make one** (see the open question below). |
| **9** tests run without Dvorak hardware | §3 above, path by path — plus the two logged spots named explicitly |
| **11** M8 declaration | verbatim at the top of this file **and** in the test file's header comment |
| **12** pin conformance | every call is against `KBD-§8` character-for-character: `GetQwertyLetterScanCodes()` · `ResolveKeyFromCodes(uint32, uint32)` · `BuildTranslationMap(const TArray<FSiegePositionalKeyProbe>&, FSiegeKeyResolver, TMap<FKey,FKey>&)` · `RetargetContextKeys(const UInputMappingContext*, UInputMappingContext*, const TMap<FKey,FKey>&, int32&)` · `GetPositionalContext(const UInputMappingContext*)` · `IsPositionalRemapActive()` · `DescribeActiveTranslation()` · `SetTranslationMapForAutomationTests(const TMap<FKey,FKey>&)`. ⛔ **No `Build.cs` change.** |
| **13** `KBD-§0` scope | no save-game field, no settings row, no CVar, no digit/modifier/mouse assertion |
| **5** most-vexing-parse | no `const T Name(Other(x));` anywhere; every local is `= `-initialised or brace-initialised |

**Also worth a look:**

- ⚠️ **`FSiegeKeyResolver` is bound to NAMED LAMBDA LVALUES, never to a temporary and never to a
  function name.** `TFunctionRef`'s converting ctor is marked `UE_LIFETIMEBOUND`
  (`Templates/Function.h:567`) and deliberately does **not** coerce a function type to a pointer;
  named lvalues sidestep both questions entirely. TASK-511 binds the function directly per TASK-509's
  header note — that form is *its* to exercise, and I did not duplicate it here.
- The subsystem fixture never calls `Initialize` (a `FSubsystemCollectionBase` cannot be fabricated),
  which is **deliberate, not tolerated**: no OS probe, no 1 Hz timer, no Slate hook runs during the
  test, so the only state in play is what the tests-only setter puts there.
- Both IMC fixtures and the subsystem are held by `TStrongObjectPtr` — the Outer chain does not keep
  a UObject alive on its own (the `SiegeSettingsTest.cpp:66-98` precedent).

---

## 8. 🔍 WHAT I THINK MAY BE WRONG IN TASK-509's IMPLEMENTATION — reported, ⛔ NOT changed

Per the spec's item (5): I did not touch a pinned contract. These are for TASK-513.

1. ⚖️ **THE COLLISION SEMANTICS (§4).** A genuine open ruling, not a defect. My read: **first-wins is
   the better rule and should be confirmed**, and TASK-509 was right to implement it and flag it
   rather than silently follow the plan's one-word wording.
2. ⚠️ **`BuildTranslationMap` skip (3) — the duplicate-source-row guard — does NOT claim the key,
   while skips (2), (4), (5) and (6) all do.** (`.cpp:137-140` vs `:131, :149, :157, :166`.) I
   believe this is **correct as written** and I am recording it only so QA does not read the
   asymmetry as an oversight: the first occurrence of that source already resolved the claim, so
   claiming again is either redundant (identity/refused) or actively wrong (an accepted probe
   **vacates** its QWERTY key, and re-claiming it from the duplicate row would block a legitimate
   two-key swap). ⛔ **Do not "fix" this to match the others.** ✅ Test 4 CASE E pins the observable
   behaviour either way.
3. ⚠️ **`Source == Target` is a documented precondition, not a coded refusal** (TASK-509 §5.6). I
   agree with leaving it out of a pinned function — but note that **nothing in the batch enforces
   it**, and my tests do not cover it because doing so would assert an unsupported call. If TASK-513
   wants the one-line guard, it is cheap and it is TASK-509's line to add, not mine.
4. ✅ **No defect found in the four functions themselves.** I traced every early return in
   `BuildTranslationMap` and every refusal in `RetargetContextKeys` against `KBD-§5`, and each
   degrades to the source key with `OutNumRetargeted` set before the return on every path.

---

## 9. ⚠️ OPEN QUESTIONS / DELIBERATE GAPS (`SC-§32` — stated, not implied)

1. ⛔ **`DescribeActiveTranslation()` IS NOT ASSERTED BYTE-FOR-BYTE, AND THAT IS A DELIBERATE
   OMISSION I AM FLAGGING RATHER THAN HIDING.** `KBD-§8` pins its format (`"W → Comma, S → O"`,
   `", "`-separated, `" → "` between, A..Z order, identities omitted, empty when the map is empty)
   and `SC-§13`/spec item (3) bind any such claim to `TestEqualSensitive`. **I did not write that
   test**, because the string is produced by TASK-511's file, which did not exist while I was
   writing, and a format assertion authored blind against a spec is the thing RULING 1 says is
   worthless. ⚖️ **If TASK-513 wants it, it is a ~10-line addition to test 3 using
   `TestEqualSensitive`, and I would rather be told to add it than have guessed.** The A..Z ordering
   the format depends on **is** asserted (test 1).
2. **Nothing here compiles or runs.** TASK-514 owns both. Every engine symbol used was read out of
   the installed 5.8 tree (§10), but "verified by reading" is not "verified by compiling".
3. **The tests do not press a key, open PIE, or ask Windows anything.** A full green run means the
   **logic** is right. ⛔ The scancode table is validated against the OS only by TASK-511's
   `MapVirtualKeyEx(0x11, …) == 'W'` self-check, and the feature's real gate is **TASK-515 —
   Jonathan's own Dvorak PIE check, which no agent can substitute for.**
4. **Test 3's subsystem half depends on TASK-511's not-yet-written file.** If TASK-511's public
   surface differs from `KBD-§8` in any way, this file fails to compile at TASK-514 — which is the
   pin working as designed, and the failure would be TASK-511's to fix, not mine.

---

## 10. Engine facts re-verified against the installed UE 5.8 tree (not taken on trust)

| claim | verified at |
|---|---|
| `GetKeyFromCodes` tries VK map, then char map, then synthesizes for `CharCode > 32`, else `EKeys::Invalid` | `InputCoreTypes.cpp:1595-1612` |
| `FWindowsPlatformInput::GetKeyMap` registers **no letter VKs**, and drops OEM VKs whose char is in the printable map | `WindowsPlatformInput.cpp:6-121` (the removal loop at `:105-110`) |
| `GetStandardPrintableKeyMap` is a static table: `'A'..'Z'` + `; = , - . / \` [ \\ ] ' space` | `GenericPlatformInput.cpp:5-95` |
| `EKeys::Comma/Period/Apostrophe/Semicolon/Slash/Hyphen/Equals` all exist | `InputCoreTypes.cpp:136-149` |
| `EKeys::Invalid` is `FKey(NAME_None)`; `FKey::IsValid()` is false for it | `InputCoreTypes.cpp:414`, `:1300-1308` |
| `FKey(const TCHAR*)` ctor; equality is by `KeyName` | `InputCoreTypes.h:62-65`, `:109` |
| `UInputMappingContext::MapKey` is **public**, `UE_API`, and the engine comments it as editor/test-only | `InputMappingContext.h:224-228` (public block opens `:150`) |
| `GetMapping(Index)` public, non-const ref, not `WITH_EDITOR`-guarded | `InputMappingContext.h:220` |
| `Modifiers`/`Triggers` are `UPROPERTY(Instanced) TArray<TObjectPtr<…>>` | `EnhancedActionKeyMapping.h:79-90` |
| `UInputModifierNegate::bX/bY/bZ` default **true/true/true** | `InputModifiers.h:259-264` |
| `UInputModifierSwizzleAxis::Order`, `EInputAxisSwizzle::YXZ` | `InputModifiers.h:390-420` |
| `UInputTriggerHold::HoldTimeThreshold` (default `1.0f`), `bIsOneShot` (default `false`) | `InputTriggers.h:333-338` |
| `TFunctionRef`'s converting ctor is `UE_LIFETIMEBOUND` and does not coerce functions to pointers | `Templates/Function.h:561-573` |
| `TestTrue`/`TestFalse`/`TestEqual` overloads taking `const FString&` exist | `Misc/AutomationTest.h:2029, 2369, 2605` |
| `GetTransientPackageAsObject()`, `DuplicateObject<T>(T const*, UObject*)` | `UObject/UObjectGlobals.h:276, :2016` |

---

## 11. Slack

Posted in **⚙️ Dev & QA** (`C0BF0QZP3CN`, thread_ts `1783116269.740549`), prefix
`⚙️ GAMEPLAY-PROGRAMMER:` — `🧪 TASK-510` at `ready-for-qa`, naming the collision ruling and the
`DescribeActiveTranslation` gap as the two things TASK-513 must decide.
