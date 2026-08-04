# TASK-511 — [KBD-3] `USiegeKeyboardLayoutSubsystem` — the probe, the cache, and BOTH change hooks

**Agent:** gameplay-programmer · **Status:** `ready-for-qa` · **Date:** 2026-08-04
**QA gate:** **TASK-513** → `.claude/pipeline/qa/TASK-513-keyboard-layout.md` (`SC-§29`)
**Law implemented:** CONVENTIONS `KBD-§0` · `KBD-§1` · `KBD-§2` · `KBD-§5` · `KBD-§6` · `KBD-§7` · **`KBD-§8` (the pin)** · `KBD-§10` · `KBD-§11`
**Design authority:** `C:\Users\wesel\.claude\plans\ok-there-are-a-cheerful-mccarthy.md`
**Upstream:** TASK-509 (`handoffs/TASK-509-programmer.md`) — I am the only shipped caller of `FSiegeKeyboardLayoutStatics`.

## ⛔ M8 DECLARATION (verbatim)

**adds no replicated property, no new replicated class, no new relevancy tier.**

Stated verbatim in the header's class comment as well. ✅ **And the reason is structural, not
incidental: `USiegeKeyboardLayoutSubsystem` is a `UGameInstanceSubsystem` — ONE PER CLIENT
PROCESS, client-local by construction.** In a listen-server match the host and each joining
client probe **their own** OS layout, which is the only correct behaviour; the duplicate IMC
lives in the transient package and is never seen by the network.

---

## 1. Files touched — EXACTLY TWO, both NEW

| file | status |
|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/SiegeKeyboardLayoutSubsystem.h` | **new** |
| `Source/GitClaudeUnrealTest/Siegebound/SiegeKeyboardLayoutSubsystem.cpp` | **new** |

⛔ **Nothing else was opened for edit.** ⛔ **NO `Build.cs` CHANGE** — `InputCore`,
`EnhancedInput`, `Slate` and `SlateCore` are already public dependencies
(`GitClaudeUnrealTest.Build.cs:15,16,22,48`), `Windows/WindowsHWrapper.h` lives in **Core**, and
`user32.lib` is a UBT default (`UEBuildWindows.cs:2093`). ⛔ `ApplicationCore` is **not** needed.
⛔ Did not touch `SiegeKeyboardLayoutStatics.{h,cpp}` (TASK-509's, frozen),
`Tests/SiegeKeyboardLayoutTest.cpp` (TASK-510's, in flight) or `HeroCharacter.cpp` (TASK-512's).
⛔ No compile, no Git, no editor/MCP/PIE.

**Board:** TASK-511 set `in-progress` at start, `ready-for-qa` at finish. Nothing else on the board was edited.

---

## 2. ⭐ PIN-CONFORMANCE STATEMENT — `KBD-§8`, line by line

| `KBD-§8` | as written | ✓ |
|---|---|---|
| `DECLARE_LOG_CATEGORY_EXTERN(LogSiegeInputLayout, Log, All);` | identical (header); `DEFINE_LOG_CATEGORY(LogSiegeInputLayout);` in the `.cpp` | ✅ |
| `DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSiegeKeyboardLayoutChanged);` | identical | ✅ |
| `UCLASS()` / `class GITCLAUDEUNREALTEST_API USiegeKeyboardLayoutSubsystem : public UGameInstanceSubsystem` | identical | ✅ |
| `const UInputMappingContext* GetPositionalContext(const UInputMappingContext* Source);` | identical, and ⛔ **still not a `UFUNCTION`** (comment says why) | ✅ |
| `UFUNCTION(BlueprintPure, Category = "Siegebound\|Input") bool IsPositionalRemapActive() const;` | identical (UFUNCTION on its own line, repo style) | ✅ |
| `UFUNCTION(BlueprintPure, …) FString DescribeActiveTranslation() const;` | identical | ✅ |
| `UFUNCTION(BlueprintCallable, …) void RefreshKeyboardLayout();` | identical | ✅ |
| `UPROPERTY(BlueprintAssignable, …) FOnSiegeKeyboardLayoutChanged OnKeyboardLayoutChanged;` | identical | ✅ |
| `void SetTranslationMapForAutomationTests(const TMap<FKey, FKey>& InTranslation);` | identical | ✅ |
| `virtual void Initialize(FSubsystemCollectionBase& Collection) override;` / `virtual void Deinitialize() override;` | identical | ✅ |
| `uint64 CachedLayoutHandle = 0;` | identical | ✅ |
| `UPROPERTY(Transient) TMap<TObjectPtr<const UInputMappingContext>, TObjectPtr<UInputMappingContext>> PositionalContexts;` | identical | ✅ |
| `TMap<FKey, FKey> TranslationMap;` | identical | ✅ |

Also present character-for-character from the `names:` block: `ProbeActiveLayout()` ·
`HandleApplicationActivationChanged(bool)` · `PollForLayoutChange()` · **CVar
`siege.Input.LayoutPollEnabled`** (int32, default **1**, `ECVF_Default`) ·
**`LayoutPollIntervalSeconds` = `1.0f`** (a named `static constexpr float`, ⛔ never a bare
`1.0f` at the `SetTimer` call).

⛔ **Nothing was "improved".** No parameter renamed, no `const` added or removed, no return type
changed, no overload added. **`GetPositionalContext` is still NOT reflected** — UHT rejects a
`const UObject*` return type, and the header says so in capitals so a well-meaning
"expose to Blueprint" edit gets stopped at review instead of at the batch's only compile gate.

### ⚠️ FOUR PRIVATE MEMBERS THE PIN DOES NOT LIST — declared here, not buried

`KBD-§8`'s private block is a *link contract*, not an exhaustive member list (it also omits
`ProbeActiveLayout`/`HandleApplicationActivationChanged`/`PollForLayoutChange`, which the
`names:` block does require). These four are what the three mechanisms cost:

| member | why it exists |
|---|---|
| `uint64 ReadActiveLayoutHandle() const` | the cheap half of the probe (handle only). ⭐ It is what makes the 1 Hz poll and the per-call re-probe free — without it every re-probe walks 26 positions. |
| `FDelegateHandle ActivationChangedHandle` | ⛔ required to **unbind** in `Deinitialize` with `Remove(Handle)` rather than a blunt `RemoveAll(this)`. |
| `FTimerHandle LayoutPollTimerHandle` | ⛔ required to clear the poll timer in `Deinitialize`. |
| `bool bTranslationOverriddenForTests` | ⛔ **load-bearing, not a convenience — see §5.1.** |

---

## 3. `GetPositionalContext` exactly as written (the four rungs)

```cpp
const UInputMappingContext* USiegeKeyboardLayoutSubsystem::GetPositionalContext(const UInputMappingContext* Source)
{
    if (!Source) { return nullptr; }                 // RUNG 1 — the ONLY null it ever returns

    RefreshKeyboardLayout();                         // MECHANISM 1 OF 3 — free unless the HKL moved

    if (TranslationMap.Num() == 0) { return Source; } // RUNG 2 — ⭐ same pointer, no allocation

    if (const TObjectPtr<UInputMappingContext>* CachedDuplicate = PositionalContexts.Find(Source))
    {
        if (*CachedDuplicate) { return *CachedDuplicate; }        // RUNG 3 — cache hit
        UE_LOG(LogSiegeInputLayout, Error, …);                    // unreachable; rebuild, never null
        PositionalContexts.Remove(Source);
    }

    // RUNG 4 — build it. ⛔ No const_cast anywhere: DuplicateObject<T> takes `T const*`.
    const FString DuplicateBaseName = FString::Printf(TEXT("%s_Positional"), *Source->GetName());
    const FName DuplicateName = MakeUniqueObjectName(
        GetTransientPackage(), UInputMappingContext::StaticClass(), FName(*DuplicateBaseName));

    UInputMappingContext* Duplicate = DuplicateObject<UInputMappingContext>(Source, GetTransientPackage(), DuplicateName);
    if (!Duplicate)                       { UE_LOG(…Error…); return Source; }

    int32 NumRetargeted = 0;
    if (!FSiegeKeyboardLayoutStatics::RetargetContextKeys(Source, Duplicate, TranslationMap, NumRetargeted))
    {                                       UE_LOG(…Error…); return Source; }   // duplicate DISCARDED

    PositionalContexts.Add(Source, Duplicate);
    UE_LOG(…Log…);
    return Duplicate;
}
```

**Fail-safe trace (`KBD-§9` criterion 6), every early return:**

| exit | returns | logs |
|---|---|---|
| `Source == nullptr` | `nullptr` — ⛔ the only one | — |
| `TranslationMap.Num() == 0` | **`Source`, the same pointer** — no `DuplicateObject`, no allocation, nothing cached | — |
| cache hit | the cached duplicate | — |
| cached duplicate somehow null | rebuilds (never returns null) | `Error` |
| `DuplicateObject` returned null | **`Source`** | `Error` |
| `RetargetContextKeys` returned false | **`Source`**; ⛔ the duplicate is discarded, never cached, never handed out — and by TASK-509's contract it was never written to at all (all three refusals precede the first write) | `Error` |

⛔ **Nothing on any path returns `EKeys::Invalid`, `nullptr` for a non-null input, or a
half-retargeted context.**

---

## 4. The three layout-change mechanisms — wired and unwired

| # | mechanism | wired | unwired | fenced |
|---|---|---|---|---|
| 1 | **re-probe on every `GetPositionalContext`** | first statement after the null check | n/a (no state) | not needed — `RefreshKeyboardLayout` is platform-agnostic and is a no-op where `ReadActiveLayoutHandle()` returns 0 |
| 2 | **`FSlateApplication::Get().OnApplicationActivationStateChanged().AddUObject(this, &…::HandleApplicationActivationChanged)`** | `Initialize`, ⛔ **behind `FSlateApplication::IsInitialized()`** (the `SiegeAssistantConsoleWidget.cpp:1024` precedent — a commandlet/`-nullrhi` run has no Slate app and `Get()` would assert); the returned `FDelegateHandle` is stored | ⛔ **`Deinitialize`** — `…OnApplicationActivationStateChanged().Remove(ActivationChangedHandle)` then `Reset()`, again behind `IsInitialized()` | `#if PLATFORM_WINDOWS` on the bind, the unbind **and** the handler body |
| 3 | **1 Hz HKL poll** | `Initialize` — `GetGameInstance()->GetTimerManager().SetTimer(LayoutPollTimerHandle, this, &…::PollForLayoutChange, LayoutPollIntervalSeconds, /*bLoop*/ true)` | ⛔ **`Deinitialize`** — `ClearTimer(LayoutPollTimerHandle)` then `Invalidate()` | `#if PLATFORM_WINDOWS` on the timer, the CVar **and** the handler body |

- ⛔ **The GameInstance's timer manager, not the world's** — this subsystem outlives every
  `OpenLevel` and a world timer dies with its world. `UWorld::GetTimerManager()` forwards to
  exactly this manager when a game instance owns the world (`World.cpp:8056-8059`), so it ticks
  on the normal schedule. `UGameInstance::TimerManager` is constructed in the GameInstance
  constructor (`GameInstance.cpp:55`), so it is valid at `Initialize`.
- **`RefreshKeyboardLayout()` never swaps a pointer and never re-adds a context.** On a detected
  change it re-targets each cached duplicate **in place from its pristine source key-by-key** and
  calls `UEnhancedInputLibrary::RequestRebuildControlMappingsUsingContext(Duplicate)`
  (`EnhancedInputLibrary.h:36-37`). ⚠️ The stale symbol `…ForContext` at
  `InputMappingContext.h:217` is called out in a comment so nobody greps for it, concludes the
  API was removed, and reaches for the banned `MapKey`.
- ⛔ **`OnKeyboardLayoutChanged` is broadcast only when the newly derived translation actually
  DIFFERS** (`TMap::OrderIndependentCompareEqual`, `Map.h.inl:116`) — a US-QWERTY → UK-QWERTY
  switch moves the HKL and broadcasts nothing. It fires **after** the re-target and the log, so a
  consumer sees the new state.
- `RefreshKeyboardLayout()` is **idempotent and safe to call every frame**: `if (CurrentLayoutHandle == CachedLayoutHandle) return;`.

**CVar name (character-for-character): `siege.Input.LayoutPollEnabled`** — `TAutoConsoleVariable<int32>`,
default `1`, `ECVF_Default`, read **live** on every poll tick via `GetValueOnGameThread()` so
`siege.Input.LayoutPollEnabled 0` actually stops the poll at runtime and `1` restarts it. ⛔ It
disables the **poll**, never the remap — mechanisms 1 and 2 keep running.

---

## 5. ⚠️ THINGS I DECIDED THAT THE PIN LEFT OPEN — flagged, not buried

### 5.1 ⭐ `SetTranslationMapForAutomationTests` LATCHES, and without the latch the seam does not work

**The conflict is real and TASK-510 will hit it.** `GetPositionalContext` re-probes on every call
(mechanism 1). So a test that calls `SetTranslationMapForAutomationTests(DvorakMap)` and then
`GetPositionalContext(Ctx)` on a **QWERTY host** would, with a literal implementation, re-probe →
build an **empty** translation → destroy the injected map → return `Source`. The test would then
pass or fail for a reason with nothing to do with the code under test.

⇒ `SetTranslationMapForAutomationTests` sets `bTranslationOverriddenForTests = true`, and
`RefreshKeyboardLayout()` returns immediately while it is set. **The flag is false on every
shipped path (nothing in the game calls the setter), the latch is permanent for that instance,
and a test that wants a probing subsystem constructs a fresh one.** It also drops the duplicate
cache (a duplicate built under the previous map carries the previous map's keys) and ⛔ does
**not** broadcast — it is a test seam, not a detected layout change. It logs at `Warning`, with
the words *"This line must never appear in a shipped session."*

⚖️ I judged this **inside** the pin (`SetTranslationMapForAutomationTests` is pinned to exist and
to be tests-only; how it defends itself is not pinned) rather than an amendment. **TASK-513 may
overrule** — but note that removing the latch does not simplify anything, it removes TASK-510's
ability to test the remap state at all.

### 5.2 The "ONE static function" clause, and the `nullptr` parameter

`KBD-§6` says every Win32 symbol lives "inside `#if PLATFORM_WINDOWS`, confined to ONE STATIC
FUNCTION IN THE `.cpp`." It is **literally one**: `SiegeKeyboardLayoutPlatform::ProbeActiveKeyboardLayout(uint64& OutLayoutHandle, TArray<FSiegePositionalKeyProbe>* InOutProbes)`.
Passing `nullptr` for the probe array means *"read the handle and stop"*, which is what the poll
and the per-call re-probe use; passing the array also walks the 26 positions.
⚖️ A pointer rather than a `bool` because **the parameter that is absent IS the work that is
skipped** — there is no boolean at the call site to read backwards. The `#else` branch is the
same signature, clears both out-params and returns 0.

### 5.3 The handle is compared TWICE (poll, then refresh) and that is deliberate

`PollForLayoutChange` checks the CVar, then `ReadActiveLayoutHandle() == CachedLayoutHandle`, then
calls `RefreshKeyboardLayout()`, which makes the same comparison. The poll's guard is what keeps
the 1 Hz path to a single Win32 call; **Refresh's own guard is what makes the public,
Blueprint-callable function idempotent for every other caller** (including `GetPositionalContext`
on every possession). The redundant read costs one `GetKeyboardLayout(0)` on the rare change path.
Commented at both sites.

### 5.4 ⭐ The self-check reads the SHIPPED table row, not a second hard-coded `0x11`

`Initialize` probes once, finds the row whose `QwertyKey == EKeys::W`, and logs
`ScanCode → VirtualKey → CharCode → resolved FKey`. That row's `ScanCode` **is** `0x11`, so the
call made is exactly `MapVirtualKeyEx(0x11, MAPVK_VSC_TO_VK_EX, hkl)` as specified — but sourced
from the table the game actually uses, so it validates **that row** rather than a copy of it.
⛔ **Logged, never `check()`ed**: on Jonathan's Dvorak host the correct answer is `VK_OEM_COMMA`
(`0xBC`), so an assert would fire on precisely the machine the feature exists for. The log states
both outcomes explicitly. ⚠️ This costs a second probe at `Initialize` (the self-check's, then
`RefreshKeyboardLayout`'s) — ~104 `MapVirtualKeyEx` calls, once, ever, in exchange for each
function doing one job.

### 5.5 ⚠️ The CVar is FENCED under `#if PLATFORM_WINDOWS` — a declared interpretation

The dispatch says *"Put it behind a CVar, default ON"* and *"Guard all three on
`#if PLATFORM_WINDOWS`"*; `KBD-§6` says *"Fence the poll timer and the activation hook the same
way."* I read the CVar as part of mechanism (c) and fenced it with the poll it governs — on a
platform with no probe there is no poll to enable, and **a console variable that provably does
nothing is worse than an absent one**. ⚠️ **Consequence for TASK-510: on a non-Windows host the
CVar does not exist.** Every host in this project is Windows, so no test is affected today.
**TASK-513 may prefer it unfenced; that is a two-line move.**

### 5.6 `TranslationMap.Num() == 0` sits ABOVE the cache lookup (as pinned) — and the consequence

After a switch **back** to QWERTY the map empties, so **new** callers get `Source` while a
duplicate already handed to `AHeroCharacter` stays live. That is correct and not a divergence:
`RefreshKeyboardLayout` retargets that duplicate with the now-empty map, which writes every key
back to its **source** value, so the two pointers behave identically. ⛔ **Dropping the cache
instead would leave the hero holding a stale Dvorak context** — this is why the empty-map branch
still runs the re-target loop.

### 5.7 The one residual, stated rather than hidden

If `RetargetContextKeys` **refuses** during the in-place re-target after a layout change, the
cached duplicate keeps the **previous** layout's keys — which is worse than "behaves as
yesterday". It is close to unreachable (every refusal condition is a property of `Source`, and
`Source` has not changed since the duplicate was built successfully), and nothing better exists:
we may not hand the hero a different pointer, and a partial write is forbidden outright. Logged
at `Error`, commented at the site.

### 5.8 `Initialize` broadcasts on a non-QWERTY host

The first probe goes through the same `RefreshKeyboardLayout()` path as a mid-session change, so
on a Dvorak host it broadcasts `OnKeyboardLayoutChanged` into an **empty** delegate list (nothing
can have bound yet at `Initialize`). Harmless, and preferred to a startup special case that would
have to be kept honest forever. Commented.

### 5.9 TASK-509's §5.2 collision-semantics conflict is NOT touched by this file

`BuildTranslationMap` is called with the shipped resolver and its result used as-is. Nothing here
depends on first-wins vs. both-refused; whichever way TASK-513 rules, this file is unaffected.

---

## 6. ⚠️ NOTES FOR TASK-510 (tests) — READ BEFORE ASSERTING ON THIS SUBSYSTEM

1. **To drive the two subsystem states on a QWERTY host, use `SetTranslationMapForAutomationTests`:**
   an **empty** map ⇒ `GetPositionalContext` returns **the same pointer**; a populated map ⇒ it
   builds and caches a duplicate. ⛔ **Without that call the subsystem probes the real host and
   your injected state is destroyed on the next call** (§5.1).
2. **`UGameInstanceSubsystem` is `UCLASS(Abstract, Within = GameInstance)`** — construct it with a
   throwaway `UGameInstance` as its Outer, exactly like `SiegeSettingsTest.cpp`'s
   `MakeScratchStore()`, and hold both in `TStrongObjectPtr`. `Initialize()` cannot be called
   outside the engine's creation path; nothing in the public surface requires it.
3. ⛔ **`DescribeActiveTranslation()` — the separator is `" → "`: SPACE, `U+2192` RIGHTWARDS
   ARROW, SPACE**, entries joined with `", "`. An ASCII `"->"` will NOT match. (UBT passes
   `/utf-8` unconditionally — `VCToolChain.cs:708` — and `SiegeSettingsSubsystem.cpp:32` already
   ships a UTF-8 literal in a BOM-less file, so a raw glyph in your expected string is correct.)
4. ⛔ **Do NOT use `KBD-§8`'s worked example `"W → Comma, S → O, D → E"` as an expected value** —
   it illustrates the separators and is **not** in A..Z order. The order is
   `GetQwertyLetterScanCodes()`'s, i.e. **A..Z**, identities omitted. For US-Dvorak that is
   `"A → Q, C → J, D → E, …"`. `FKey::ToString()` returns the key **name**
   (`InputCoreTypes.cpp:1310-1313`), so the QWERTY-W target reads `"Comma"`, not `","`.
5. **Empty map ⇒ empty string** (not `"none"`), and `IsPositionalRemapActive()` is
   `TranslationMap.Num() > 0` — ⛔ **not** "a duplicate exists".
6. `LayoutPollIntervalSeconds` is a public `static constexpr float` — assertable without reaching
   into the `.cpp`.
7. ⛔ **`TestEqualSensitive`, never `TestEqual`, for every `FString` claim** (`SC-§13`).

---

## 7. 🔍 WHAT QA SHOULD SCRUTINISE (`KBD-§9` criteria, mapped to this task)

| criterion | where it lands here |
|---|---|
| **1** no write to `IMC_Hero.uasset` | `Source` is `const` on every path; **zero `const_cast` in the pair**; the cache KEY is `TObjectPtr<const UInputMappingContext>`, so the invariant is enforced by the type. The only non-const `UInputMappingContext*` is `Duplicate`. |
| **2** only `.Key` written, no banned API | this file never writes an IMC field at all — all mutation is delegated to `RetargetContextKeys`. Grep the pair for `MapKey`/`UnmapKey`/`UnmapAll`/`Modifiers`/`Triggers`/`PlayerMappableKeySettings` — zero hits. |
| **3** complete-type include law | every include is annotated with the symbol that requires it: `EnhancedActionKeyMapping.h` (`GetMappings().Num()`), `EnhancedInputLibrary.h`, `Engine/GameInstance.h` (`GetTimerManager` on a forward-declared return), `TimerManager.h`, `InputMappingContext.h`, **`UObject/Package.h`** (`GetTransientPackage()` returns `UPackage*` and passing it as `UObject*` is an **upcast**), `UObject/UObjectGlobals.h`, `InputCoreTypes.h`, `Engine/TimerHandle.h` (header). |
| **4** shadowing of inherited reflected members | `USubsystem`/`UGameInstanceSubsystem` declare **no** `UPROPERTY`. No local named `Owner`/`Instigator`/`Controller`/`PlayerState`/`Slot`. The `Collection` parameter is the base's own override signature, not a shadow. |
| **5** most-vexing-parse | every local with an initialiser uses `=`; there is no `const T Name(Other(x));` anywhere. Checked specifically: `DuplicateBaseName`, `DuplicateName`, `LayoutHandle`, `VirtualKey`, `OrderedProbes`, `NumAnswered`, the three hoisted `FString` descriptions. |
| **6** every failure degrades to the source | §3's table traces all six exits individually. |
| **7** `#if PLATFORM_WINDOWS` fences | **one** fenced static holds every Win32 symbol; the CVar, the timer bind/clear, the activation bind/unbind and both handler bodies are fenced; the **header contains no platform `#if` and no Windows type** — the HKL is an opaque `uint64`. Non-Windows: `ProbeActiveKeyboardLayout` clears its out-params and returns 0 ⇒ empty translation ⇒ pass-through, with **one `Log` line at `Initialize`**. |
| **8** `TestEqualSensitive` | N/A (no test file in this task) — but §6.3/§6.7 tell TASK-510 what to assert and how. |
| **9** tests run without Dvorak hardware | §5.1 / §6.1: `SetTranslationMapForAutomationTests` + its latch is what makes the SUBSYSTEM's two states drivable on a QWERTY host, the way `FSiegeKeyResolver` does it for the statics. |
| **10** guard placement in `NotifyControllerChanged` | N/A — TASK-512's file, untouched. |
| **11** M8 declaration verbatim | top of this file **and** in the header's class comment. |
| **12** pinned-registry conformance | §2, signature by signature. `check(IsInGameThread())` is the FIRST statement of the Win32 probe. The activation hook IS unbound in `Deinitialize`. ⛔ **No `Build.cs` change.** CVar name is `siege.Input.LayoutPollEnabled`. |
| **13** `KBD-§0` scope | no `USiegeSettingsSaveGame` field, no settings-menu row, no `SC-§8` registry entry, no digit/modifier/mouse remap. The CVar is a dev lever no UI reads. |

### Engine facts re-verified against the installed UE 5.8 tree (not taken on trust)

| claim | verified |
|---|---|
| `UEnhancedInputLibrary::RequestRebuildControlMappingsUsingContext(const UInputMappingContext*, bool bForceImmediately = false)` | `EnhancedInputLibrary.h:36-37` — and the doc comment at `InputMappingContext.h:217` really does name the non-existent `…ForContext` |
| `FSlateApplication::OnApplicationActivationStateChanged()` returns `FApplicationActivationStateChangedEvent&`, `DECLARE_EVENT_OneParam(…, const bool)` | `SlateApplication.h:1690-1691` (so `AddUObject`/`Remove` are available and a `void(bool)` handler matches — top-level `const` on a parameter is not part of the function type) |
| `FSlateApplication::OnInputLanguageChanged` is a bare virtual with **no delegate** | `SlateApplication.cpp:5138-5141`; the only other hits in the whole Runtime tree are the declaration (`SlateApplication.h:1701`), the base virtual (`GenericApplicationMessageHandler.h:266`) and the caller (`WindowsApplication.cpp:3225`) |
| `MapVirtualKey(scancode, MAPVK_VSC_TO_VK_EX)` and `MapVirtualKey(vk, MAPVK_VK_TO_CHAR)`, **dead-key bit not masked** | `WindowsApplication.cpp:3296`, `:3377`, `:3316-3317` |
| `Windows/WindowsHWrapper.h` is in **Core** and fully cleans up after itself (pops `TEXT`, undefines `GetObject`/`SendMessage`/`DeleteFile`/`INT`/`UINT`/…) | `Core/Public/Windows/{WindowsHWrapper,PreWindowsApi,PostWindowsApi}.h` — so it is safe in a unity build; `MinWindows.h` leaves `NOVIRTUALKEYCODES`/`NOUSER` commented out, so `VK_*` and the USER32 declarations are available |
| `DuplicateObject<T>(T const* SourceObject, UObject* Outer, FName Name)` — takes a **const** source | `UObjectGlobals.h:2015-2019` ⇒ ⛔ **no cast is needed and none is present** |
| `MakeUniqueObjectName(UObject*, const UClass*, FName, EUniqueObjectNameOptions)` | `UObjectGlobals.h:1061` |
| `GetTransientPackage()` returns `UPackage*` | `UObjectGlobals.h:275` (hence the `UObject/Package.h` include) |
| a `UPROPERTY` `TMap` keyed by `TObjectPtr<const T>` is legal and hashes | engine precedent `AnimBlueprintGeneratedClass.h:426-427`; `TCallTraits<TObjectPtr<T>>::ConstPointerType` at `ObjectPtr.h:1294-1298`; the implicit ctor from `T*` at `ObjectPtr.h:589-597`; `operator T*() const` at `ObjectPtr.h:722` |
| `TMap::OrderIndependentCompareEqual` is public | `Containers/Map.h.inl:116` |
| `UGameInstance::GetTimerManager()` is inline + always valid (constructed in the ctor), and is the manager `UWorld::GetTimerManager()` forwards to | `GameInstance.h:424`, `GameInstance.cpp:55`, `World.cpp:8056-8059` |
| `TAutoConsoleVariable<int32>(const TCHAR*, const T&, const TCHAR*, uint32 Flags = ECVF_Default)` + `GetValueOnGameThread()` | `IConsoleManager.h:2025-2032`, `:2076-2085` |
| `FTimerManager::SetTimer(FTimerHandle&, UserClass*, TMethodPtr<UserClass>, float, bool, float)` | `TimerManager.h:167-170` |
| `FKey::ToString()` returns the key **name** (`"Comma"`, not `","`) | `InputCoreTypes.cpp:1310-1313`; `EKeys::Comma("Comma")` at `:138` |
| UBT passes `/utf-8` unconditionally | `VCToolChain.cs:708` |

⚠️ **My limit, stated plainly: nothing here has been compiled and nothing has been seen on
screen.** TASK-514 owns the batch's only compile; **the Dvorak outcome is TASK-515's and
Jonathan's alone** — every claim above is about source, and a green compile would still not be
evidence that a Dvorak keyboard drives the hero correctly.

---

## 8. Downstream note for TASK-512 (`HeroCharacter.cpp`)

Nothing changed in what you were specced: `GetPositionalContext` has the pinned signature, it
never returns null for a non-null input, and **the pointer it hands you never changes** — a
mid-session layout switch re-targets that same object in place. ⇒ ⛔ **Zero new members on
`AHeroCharacter`, zero re-application code**, and the resolve goes **inside** the existing
`LocalPlayer` / `UEnhancedInputLocalPlayerSubsystem` guard chain (`SC-§21`, `KBD-§9`
criterion 10) — that function also runs on the server for a remote client's pawn, and probing a
keyboard on behalf of a machine that is not there is the failure the placement prevents.
⚠️ `GetPositionalContext` asserts the **game thread** (via the probe's `check(IsInGameThread())`),
which `NotifyControllerChanged` satisfies.

## 9. Slack

Posted in **⚙️ Dev & QA** (`C0BF0QZP3CN`, thread_ts `1783116269.740549`), prefix
`⚙️ GAMEPLAY-PROGRAMMER:` — `🔧 TASK-511` at start, `🧪 TASK-511` at ready-for-qa.
