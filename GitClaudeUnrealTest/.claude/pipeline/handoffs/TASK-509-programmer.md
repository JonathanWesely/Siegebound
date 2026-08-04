# TASK-509 — [KBD-1] `FSiegeKeyboardLayoutStatics` — the pure, testable core

**Agent:** gameplay-programmer · **Status:** `ready-for-qa` · **Date:** 2026-08-04
**QA gate:** **TASK-513** → `.claude/pipeline/qa/TASK-513-keyboard-layout.md` (`SC-§29`)
**Law implemented:** CONVENTIONS `KBD-§1` · `KBD-§2` · `KBD-§4` · `KBD-§5` · `KBD-§6` · `KBD-§7` · **`KBD-§8` (the pin)**
**Design authority:** `C:\Users\wesel\.claude\plans\ok-there-are-a-cheerful-mccarthy.md`

## ⛔ M8 DECLARATION (verbatim)

**adds no replicated property, no new replicated class, no new relevancy tier.**

It is also stated verbatim in the header's class comment. The reason is structural, not
incidental: this is a plain static library computing client-local derived state from the OS
keyboard layout of the process it runs in. Nothing here is a `UObject`, nothing is reflected,
nothing is ever seen by the network.

---

## 1. Files touched — EXACTLY TWO, both NEW

| file | status |
|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/SiegeKeyboardLayoutStatics.h` | **new** |
| `Source/GitClaudeUnrealTest/Siegebound/SiegeKeyboardLayoutStatics.cpp` | **new** |

⛔ **Nothing else was opened for edit.** No `Build.cs` change (`InputCore` and `EnhancedInput`
are already public dependencies — `GitClaudeUnrealTest.Build.cs:15-16`). No
`SiegeKeyboardLayoutSubsystem.*` (TASK-511's). No `Tests/SiegeKeyboardLayoutTest.cpp`
(TASK-510's). No `HeroCharacter.cpp` (TASK-512's, exclusively). No compile, no Git, no
editor/MCP/PIE.

**Board:** TASK-509 set `in-progress` at start, `ready-for-qa` at finish. Nothing else on the
board was edited.

---

## 2. ⭐ PIN-CONFORMANCE STATEMENT — each pinned signature, registry version beside as-written

Character-for-character against CONVENTIONS `KBD-§8`. Only leading indentation differs
(the registry is written with spaces; the repo's C++ is tab-indented, and continuation lines
are aligned under the first parameter).

### 2.1 `FSiegePositionalKeyProbe`

| | |
|---|---|
| **`KBD-§8`** | `struct FSiegePositionalKeyProbe { FKey QwertyKey; uint32 ScanCode = 0; uint32 VirtualKey = 0; uint32 CharCode = 0; };` |
| **as written** | identical — field names, order, types and the four trailing comments carried over verbatim |

```cpp
struct FSiegePositionalKeyProbe
{
	FKey   QwertyKey;        // FKey this position carries on US-QWERTY, e.g. EKeys::W
	uint32 ScanCode   = 0;   // scan-code set 1, e.g. 0x11
	uint32 VirtualKey = 0;   // VK the ACTIVE layout yields here; 0 == probe failed
	uint32 CharCode   = 0;   // MapVirtualKeyEx(VK, MAPVK_VK_TO_CHAR, hkl) — dead-key bit UNMASKED
};
```

### 2.2 `FSiegeKeyResolver`

| | |
|---|---|
| **`KBD-§8`** | `using FSiegeKeyResolver = TFunctionRef<FKey(uint32 VirtualKey, uint32 CharCode)>;` |
| **as written** | `using FSiegeKeyResolver = TFunctionRef<FKey(uint32 VirtualKey, uint32 CharCode)>;` |

### 2.3 The class and its four functions

| | |
|---|---|
| **`KBD-§8`** | `class GITCLAUDEUNREALTEST_API FSiegeKeyboardLayoutStatics` |
| **as written** | `class GITCLAUDEUNREALTEST_API FSiegeKeyboardLayoutStatics` |

| # | `KBD-§8` | as written |
|---|---|---|
| 1 | `static TArray<FSiegePositionalKeyProbe> GetQwertyLetterScanCodes();` | `static TArray<FSiegePositionalKeyProbe> GetQwertyLetterScanCodes();` |
| 2 | `static FKey ResolveKeyFromCodes(uint32 VirtualKey, uint32 CharCode);` | `static FKey ResolveKeyFromCodes(uint32 VirtualKey, uint32 CharCode);` |
| 3 | `static int32 BuildTranslationMap(const TArray<FSiegePositionalKeyProbe>& Probes, FSiegeKeyResolver Resolver, TMap<FKey, FKey>& OutTranslation);` | `static int32 BuildTranslationMap(const TArray<FSiegePositionalKeyProbe>& Probes,`<br>`                                 FSiegeKeyResolver Resolver,`<br>`                                 TMap<FKey, FKey>& OutTranslation);` |
| 4 | `static bool RetargetContextKeys(const UInputMappingContext* Source, UInputMappingContext* Target, const TMap<FKey, FKey>& Translation, int32& OutNumRetargeted);` | `static bool RetargetContextKeys(const UInputMappingContext* Source,`<br>`                                UInputMappingContext* Target,`<br>`                                const TMap<FKey, FKey>& Translation,`<br>`                                int32& OutNumRetargeted);` |

⛔ **Nothing was "improved".** No parameter renamed, no `const` added or removed, no return type
changed, no default argument introduced, no overload added, no function marked `static
constexpr`/`FORCEINLINE`. TASK-510's tests and TASK-511's subsystem link against this list.

---

## 3. What each function does, and the fences a reviewer should test

### `GetQwertyLetterScanCodes()`

- **All 26** letters, `EKeys::A`..`EKeys::Z`, each paired with its **scan-code set 1** code
  (`KBD-§4`). `VirtualKey`/`CharCode` left `0` for the platform probe to fill.
- Every anchor in the spec is present and correct — worth checking by eye against the table:
  `Q=0x10 W=0x11 E=0x12 R=0x13 T=0x14 Y=0x15 U=0x16 I=0x17 O=0x18 P=0x19` ·
  `A=0x1E S=0x1F D=0x20 F=0x21 G=0x22 H=0x23 J=0x24 K=0x25 L=0x26` ·
  `Z=0x2C X=0x2D C=0x2E V=0x2F B=0x30 N=0x31 M=0x32`. 26 rows, 26 distinct scan codes,
  26 distinct FKeys.
- ⚠️ **Returned in ALPHABETICAL A..Z order, not physical-row order.** That is deliberate and
  load-bearing: `KBD-§8` pins `DescribeActiveTranslation()` to emit its entries in
  *"source-key order as returned by `GetQwertyLetterScanCodes()`"*, and adds the parenthetical
  *"(i.e. A..Z)"*. A reorder here silently changes a string TASK-510 asserts byte-for-byte.
  The `.cpp` also carries the three physical rows as a comment so the table can still be read
  against a keyboard.
- The table is a function-local `static` of a struct whose `QwertyKey` is a **reference**, so
  no `FKey` is copied or constructed at static-init time and there is no cross-module
  static-initialisation-order question.

### `ResolveKeyFromCodes(VirtualKey, CharCode)`

- One line: `return FInputKeyManager::Get().GetKeyFromCodes(VirtualKey, CharCode);`
  (`InputCoreTypes.h:858` in the installed 5.8 tree — the plan cites `:857`, which is the
  doc-comment line immediately above; same declaration).
- ⛔ **`CharCode` is passed through UNMASKED** — the dead-key bit `0x80000000` is *not*
  stripped, `WindowsApplication.cpp:3317` does not strip it, and a comment says so explicitly
  so a future reader does not "clean it up" (`KBD-§6`).
- Verified against engine source that the failure return is exactly `EKeys::Invalid`
  (`InputCoreTypes.cpp:1611`).

### `BuildTranslationMap(Probes, Resolver, OutTranslation)`

- **Pure.** No engine state read, no globals, resolver injected — the seam that lets every
  TASK-510 test run on a QWERTY host.
- `OutTranslation` is **`Reset()` first**, so a re-probe after a `Win+Space` cannot inherit
  entries from the layout it just left.
- Skips, in the exact order the code evaluates them (the header lists them in the same order
  so header and `.cpp` read alike): **(1)** invalid `QwertyKey` (defensive) · **(2)**
  `VirtualKey == 0` · **(3)** duplicate source row (first wins) · **(4)** resolver returns
  `EKeys::Invalid` or an unregistered FKey · **(5)** identity · **(6)** injectivity — target
  already claimed.
- ⛔ **Nothing skipped ever produces `EKeys::Invalid`** — a skipped letter simply gets no map
  entry, so `RetargetContextKeys` leaves it on its source key (`KBD-§5`).
- Returns `OutTranslation.Num()`; identity entries are never added, so that **is** the count of
  non-identity entries. `0` ⇒ the host is positionally QWERTY.

### `RetargetContextKeys(Source, Target, Translation, OutNumRetargeted)`

The `KBD-§1` function. The shipped loop, verbatim:

```cpp
for (int32 Index = 0; Index < SourceMappings.Num(); ++Index)
{
	const FKey& SourceKey     = SourceMappings[Index].Key;
	const FKey* TranslatedKey = Translation.Find(SourceKey);

	Target->GetMapping(Index).Key = TranslatedKey ? *TranslatedKey : SourceKey;

	if (TranslatedKey && *TranslatedKey != SourceKey)
	{
		++OutNumRetargeted;
	}
}
```

- ⛔ **`.Key` is the only field assigned anywhere in either file.** Grep the pair for
  `Modifiers`, `Triggers`, `Action`, `PlayerMappableKeySettings`, `MapKey`, `UnmapKey`,
  `UnmapAll`, `.Add(`/`RemoveAt`/`Empty` **on an IMC** — zero hits. The only `.Add(` calls in
  the pair are on a local `TSet<FKey>` and a local `TMap<FKey, FKey>`.
- ⛔ **Every value is re-derived from `Source` by index; `Target` is never read back.** That is
  what makes the substitution simultaneous (`D → E` and `E → Period` coexist without cascading)
  and what makes a repeat call idempotent. Both sentences the spec required are in the header
  class comment **and** repeated at the loop in the `.cpp`.
- ⛔ **`Source` is `const` throughout.** There is no `const_cast` and no non-const use of the
  source IMC anywhere in the pair — the `KBD-§1` reviewable invariant.
- **Refuses wholesale, before the first write**, returning `false` with `OutNumRetargeted == 0`:
  null `Source` or `Target` · `Source->GetProfilesWithOverridenMappings().Num() > 0` ·
  mapping-array length mismatch. **All three checks precede the loop**, so "changes nothing" is
  structural rather than a promise.

---

## 4. Engine facts re-verified against the installed UE 5.8 tree (not taken on trust)

| claim | verified |
|---|---|
| `FInputKeyManager::GetKeyFromCodes(const uint32, const uint32) const` | `InputCoreTypes.h:858`; `FInputKeyManager::Get()` at `:850` |
| it returns `EKeys::Invalid` on failure | `InputCoreTypes.cpp:1611` |
| it may synthesize + register a new FKey for an unknown printable char | `InputCoreTypes.cpp:1604-1610` |
| `UInputMappingContext::GetMapping(Index)` is **public**, returns a **non-const ref**, and is **NOT** `WITH_EDITOR`-guarded | `InputMappingContext.h:220` (public section opens at `:112`; the `#if WITH_EDITOR` block is `:114-116` and closes long before) |
| `GetMappings()` is public + `const` | `InputMappingContext.h:219` |
| `GetProfilesWithOverridenMappings() const` returns `TArray<FString>` | `InputMappingContext.h:198` |
| `MappingProfileOverrides` is a **separate protected member** `GetMapping()` cannot reach | `InputMappingContext.h:109-110` (protected section `:91-111`) |
| `FEnhancedActionKeyMapping::Key` is a **public** `FKey`; `PlayerMappableKeySettings` is **protected** | `EnhancedActionKeyMapping.h:98` and `:120,:131` — confirms `KBD-§3` reason 2 |
| `FKey` has `operator==`, `operator!=` and `GetTypeHash` (so `TMap<FKey,FKey>` / `TSet<FKey>` are legal) | `InputCoreTypes.h:108-111` |
| `InputCore` + `EnhancedInput` are already **public** dependencies | `GitClaudeUnrealTest.Build.cs:15-16` |

---

## 5. ⚠️ THINGS I DECIDED THAT THE PIN LEFT OPEN — flagged, not buried

### 5.1 ⭐ An identity probe CLAIMS ITS OWN KEY (a strengthening of the injectivity guard)

The spec says skip *"any target FKey already claimed by an earlier probe."* Taken literally,
only **accepted translation targets** would be claimed — which leaves a hole: if position `A`
resolves to `A` (identity, no map entry) and position `X` later resolves to `A`, the literal
guard would accept `X → A`, and the physical `A` key would then fire **both** the A-bound and
the X-bound action. So a probe that keeps its key — identity, unusable (`VirtualKey == 0`),
unresolvable, or refused — **also enters the claim set**. I read that as *inside* the clause
(such a probe is an earlier probe, and it is claiming a key), not as an amendment to it.

⭐ **It costs nothing on any real layout, and I checked rather than assumed.** For any layout
whose 26 letter positions produce 26 *distinct* keys — every Latin layout, i.e. a bijection —
no probe is ever refused. I hand-traced the full 26 for **US-Dvorak**: 24 accepted, 2 identity
(`A`, `M`), **zero refusals**, and the result matches the plan's expected assertions exactly
(`W→Comma, S→O, D→E, Q→Apostrophe, E→Period, R→P, T→Y, F→U, C→J`, `A` absent). Also traced
**AZERTY** (`A→Q, Q→A, W→Z, Z→W, M→Comma`, zero refusals) and **QWERTZ** (`Y→Z, Z→Y`, zero
refusals). Two-key swaps survive because an accepted probe **vacates** its QWERTY key.

⚠️ **Residual, stated rather than hidden** (also in the header): the claim set is built in one
forward pass, so it cannot retract a translation already accepted onto a key whose *own* probe
later turns out unusable (position `A` → VK_B while position `B`'s probe fails). Closing that
needs a fixpoint pass over a case no real layout can produce. **If it ever happened the outcome
is one double-bound key — still never `nullptr`, never `EKeys::Invalid`, never partial**, so
`KBD-§5` holds. I judged the fixpoint not worth the complexity in a pinned pure function.
**QA/TASK-513 may overrule.**

### 5.2 ⚠️ COLLISION SEMANTICS — a genuine wording conflict between the plan and the board

- The **plan** (`ok-there-are-a-cheerful-mccarthy.md`, `DegenerateProbesAreRefused`) says
  *"two positions colliding: **each** keeps the source key"* → reads as **both refused**.
- The **board** (TASK-509 spec) says *"**any target key already claimed** (injectivity guard)"*,
  **`KBD-§5`**'s table says *"two positions claim one target | **that letter** keeps its source
  key"* (singular), and my dispatch prompt says *"any target FKey **already claimed by an
  earlier probe**"* → all three read as **first wins, the later one is refused**.

**I implemented first-wins** (three sources to one, and the two most specific are the board and
the dispatch). ⛔ **TASK-510 must not encode the plan's "each" reading without TASK-513 ruling
first** — if the ruling goes the other way it is a ~4-line change in one place, and I would
rather it be decided than silently diverge.

### 5.3 Duplicate source rows — first row wins

Not mentioned in the pin at all. `GetQwertyLetterScanCodes()` never produces one, but a fake
probe array can. Without a guard, `TMap::Add` would let the *last* duplicate win while the
claim set reflects the *first* — order-dependent and confusing. First-row-wins makes it
deterministic and matches the injectivity rule's direction.

### 5.4 `OutNumRetargeted` counts MAPPINGS whose key actually changed

Not distinct keys — one physical key bound to two actions counts twice. And the count tests
`*TranslatedKey != SourceKey`, not merely "an entry was found", because
`SetTranslationMapForAutomationTests` can legally inject an identity entry that
`BuildTranslationMap` itself never emits.

### 5.5 The profile-override refusal checks `Source` only

Exactly as pinned. It is sufficient **because `Target` is by contract a `DuplicateObject` of
`Source`** (TASK-511's only use), so it cannot carry overrides the source lacks. I deliberately
did **not** add a `Target` check — that would be an extra refusal condition not in the pin.
Documented as a precondition in the header instead.

### 5.6 `Source == Target` is a documented contract violation, not a coded refusal

A single such call still yields the right answer (each index is read then written before the
next is touched), but the idempotence guarantee is lost because the "pristine source" would
have been overwritten. I documented it as a precondition rather than adding a fourth refusal
branch outside the pin. **A one-line `if (Source == Target) { return false; }` is trivial to add
if TASK-513 prefers it.**

### 5.7 Synthesized FKeys are passed through, not filtered

`GetKeyFromCodes` may register a synthetic key (`FKey::SyntheticCharPrefix` + code) flagged
`NotBlueprintBindableKey | NotActionBindableKey` for an unknown printable char
(`InputCoreTypes.cpp:1604-1610`). The pin says drop only `EKeys::Invalid`, so I do not filter on
bindability. Per `KBD-§11` this path should not fire for Cyrillic/Greek (Windows keeps
`VK_A..VK_Z` at QWERTY positions, so the *virtual-key* lookup hits first). ⚠️ Recording it
because if some exotic layout ever did route through synthesis, the mapping would land on a key
Enhanced Input considers non-action-bindable — a silent no-op for that one letter, not a crash.
**Out of scope to fix here; noted for the record.**

### 5.8 No `check(IsInGameThread())` in this pair

`KBD-§6` places that assertion in the **Win32 probe** (TASK-511), and QA criterion 12 checks it
there. `ResolveKeyFromCodes` does touch `FInputKeyManager`'s lazily-created singleton (and
`GetKeyFromCodes` can mutate it through a `const_cast`), so I documented the game-thread
expectation in the header rather than adding an assert the pin does not call for — an assert
here could also change how TASK-510's tests are allowed to call it.

---

## 6. 🔍 WHAT QA SHOULD SCRUTINISE (`KBD-§9` criteria, mapped to this task)

| criterion | where it lands here |
|---|---|
| **1** no write to `IMC_Hero.uasset` | `Source` is `const` on every path; no `const_cast`; the only non-const IMC pointer in the file is the `Target` parameter |
| **2** only `.Key` written, no banned API | grep the pair for `MapKey`/`UnmapKey`/`UnmapAll`/`Modifiers`/`Triggers`/`PlayerMappableKeySettings` — zero hits; one assignment target: `Target->GetMapping(Index).Key` |
| **3** complete-type include law | `.cpp` includes `EnhancedActionKeyMapping.h`, `InputCoreTypes.h`, `InputMappingContext.h` explicitly; the header forward-declares `UInputMappingContext` (pointer-only there), matching `SiegeCombatStatics.h` |
| **4** shadowing of inherited reflected members | N/A structurally — no base class, no reflection. No local is named `Owner`/`Instigator`/`Controller`/`PlayerState`/`Slot` |
| **5** most-vexing-parse | no `const T Name(Other(x));` anywhere. The one const local from a call is `const FKey ResolvedKey = Resolver(...)` — copy-init with `=`. The table uses `= { ... }` |
| **6** every failure degrades to the source | §3 above traces each early return; nothing can emit `EKeys::Invalid` or a partial write |
| **7** `#if PLATFORM_WINDOWS` fences | **there is no Win32 symbol and no platform `#if` in this pair at all** — it compiles identically everywhere |
| **8** `TestEqualSensitive` | N/A (no test file in this task) |
| **9** tests run without Dvorak hardware | the `FSiegeKeyResolver` parameter is the only route into `BuildTranslationMap`; that function reads no engine state whatsoever |
| **11** M8 declaration | stated verbatim at the top of this file **and** in the header class comment |
| **12** pinned-registry conformance | §2 above, signature by signature |
| **13** `KBD-§0` scope | no save-game field, no settings row, no CVar, no digit/modifier/mouse remap — this task adds none of those |

⚠️ **My limit, stated plainly: nothing here has been compiled and nothing has been seen on
screen.** TASK-514 owns the batch's only compile; the Dvorak outcome is TASK-515's and
Jonathan's alone.

---

## 7. Downstream notes for TASK-510 / TASK-511

- ⚠️ **Bind `FSiegeKeyResolver` to the FUNCTION, not to a function-pointer temporary.**
  `TFunctionRef` deliberately does not coerce a function type to a pointer
  (`Templates/Function.h:571`), so pass
  `FSiegeKeyboardLayoutStatics::ResolveKeyFromCodes` (**no `&`**). Noted in the header.
- `GetQwertyLetterScanCodes()` returns **A..Z order** — `DescribeActiveTranslation()` (TASK-511)
  gets its pinned ordering for free by iterating that array and skipping letters absent from
  the map.
- `BuildTranslationMap` returning `0` is the subsystem's *"return the same pointer, allocate
  nothing"* signal, and `IsPositionalRemapActive()` is pinned to mean `TranslationMap.Num() > 0`
  — the two agree by construction.
- `RetargetContextKeys` is idempotent, which is exactly what lets TASK-511 re-target its
  **cached duplicate in place** on a layout change without handing `AHeroCharacter` a new
  pointer.
- Read §5.2 before writing `DegenerateProbesAreRefused`.

## 8. Slack

Posted in **⚙️ Dev & QA** (`C0BF0QZP3CN`, thread_ts `1783116269.740549`), prefix
`⚙️ GAMEPLAY-PROGRAMMER:` — `🔧 TASK-509` at start, `🧪 TASK-509` at ready-for-qa.
