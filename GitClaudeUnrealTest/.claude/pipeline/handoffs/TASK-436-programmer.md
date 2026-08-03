# TASK-436 — [SET-1] `USiegeSettingsSaveGame` + `USiegeSettingsSubsystem` — programmer handoff

**Task:** TASK-436 (gameplay-programmer) · batch **SETTINGS+CONFIRM** · status → `ready-for-qa`
**Law followed:** CONVENTIONS "Settings screen + the assistant CONFIRM STEP + the non-orderable-kind guard (2026-08-03)" **§2, §3(d), §5, §8, §10** · "Deck-builder & saved decks (M6)" (the SaveGame idiom) · "Delegates (C++)" · "Logging (C++)"
⛔ **No compile, no Git, no editor, no MCP, no PIE, no `.ini`, no `Content/`.** New files only. TASK-447 is the one compile gate.

---

## 1. Files (all NEW — nothing existing was edited)

| File | Role |
|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/SiegeSettingsSaveGame.h` | `USiegeSettingsSaveGame : USaveGame`, the one v1 field |
| `Source/GitClaudeUnrealTest/Siegebound/SiegeSettingsSaveGame.cpp` | deliberately holds no definitions — see §7 note (c) |
| `Source/GitClaudeUnrealTest/Siegebound/SiegeSettingsSubsystem.h` | `USiegeSettingsSubsystem : UGameInstanceSubsystem`, `LogSiegeSettings`, `FOnSiegeSettingsChanged` |
| `Source/GitClaudeUnrealTest/Siegebound/SiegeSettingsSubsystem.cpp` | load/save/no-op/broadcast |
| `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeSettingsTest.cpp` | 6 automation tests (`Siegebound.Settings.*`) |

⛔ **Zero edits outside these five files.** No `Build.cs` change was needed or made (TASK-443 owns it): `Engine` already supplies `UGameplayStatics`/`USaveGame` and `Core` supplies `Misc/AutomationTest.h`.

---

## 2. PUBLISHED SIGNATURES — the §8 pin, character-for-character

**TASK-437 and TASK-443 compile against exactly this. Nothing was renamed, nothing was "improved."**

```cpp
// ── SiegeSettingsSaveGame.h ───────────────────────────────────────────────
UCLASS()
class GITCLAUDEUNREALTEST_API USiegeSettingsSaveGame : public USaveGame
{
    GENERATED_BODY()
public:
    UPROPERTY() bool bAssistantConfirmBeforeExecute = true;
};

// ── SiegeSettingsSubsystem.h ──────────────────────────────────────────────
DECLARE_LOG_CATEGORY_EXTERN(LogSiegeSettings, Log, All);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSiegeSettingsChanged, FName, SettingName);

UCLASS()
class GITCLAUDEUNREALTEST_API USiegeSettingsSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    static const TCHAR* SettingsSlotName;   // == TEXT("SiegeSettings")

    UFUNCTION(BlueprintPure,     Category = "Siegebound|Settings") bool IsAssistantConfirmEnabled() const;
    UFUNCTION(BlueprintCallable, Category = "Siegebound|Settings") void SetAssistantConfirmEnabled(bool bEnabled);

    UPROPERTY(BlueprintAssignable, Category = "Siegebound|Settings") FOnSiegeSettingsChanged OnSettingsChanged;

    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
};
```

The only additions to the pinned lines are `GITCLAUDEUNREALTEST_API` + `GENERATED_BODY()` (the project's shipped idiom on `USiegeDeckSaveGame` and `USiegeSessionSubsystem`; the §8 registry omits both because it lists signatures, not files).

### 2b. ADDITIVE members — NOT in the §8 pin, safe to ignore, listed so QA can see everything

| Member | Why it exists |
|---|---|
| `static constexpr int32 SettingsUserIndex = 0;` | §2 pins user index 0; this makes it a named constant instead of a literal at four call sites (the `USiegeDeckSaveGame::UserIndex` precedent) |
| `static const FName SettingName_AssistantConfirmBeforeExecute;` == `TEXT("bAssistantConfirmBeforeExecute")` | the `OnSettingsChanged` payload. **The pin fixes the delegate's *type* but not the *value* it carries — this is that value, and TASK-437 needs it.** Same token as the SaveGame field and as CONVENTIONS §2's name for the setting |
| `void LoadSettingsFromSlot();` | the body of the load. Public **only** so an automation test can drive a load — `Initialize(FSubsystemCollectionBase&)` cannot be called outside the engine's own subsystem-creation path. ⛔ Documented in the header as not-a-gameplay-path; the game calls it exactly once, from `Initialize()` |
| `void SetSlotNameForAutomationTests(const FString&);` | ⛔ automation only — redirects this instance to a scratch slot. See §5 |
| `int32 SettingsChangeBroadcastCount` / `FName LastBroadcastSettingName` | diagnostics + the delegate test's observation point. No logic anywhere reads them |
| private: `ApplyBoolSetting` / `SaveSettingsToSlot` / `BroadcastSettingChanged` / `ResolveSlotName` | implementation |

### 2c. The resolution snippet TASK-437 and TASK-443 should both use

```cpp
UGameInstance* GI = GetGameInstance();                 // widget: GetGameInstance(); actor/component: GetGameInstance()
USiegeSettingsSubsystem* Settings = GI ? GI->GetSubsystem<USiegeSettingsSubsystem>() : nullptr;
const bool bConfirm = Settings ? Settings->IsAssistantConfirmEnabled() : true;   // ⚠️ fallback is TRUE
```

⛔ **The fallback when the subsystem does not resolve is `true` (confirm ON), never `false`.** An unresolvable lookup must degrade to **more** human review, not less. This is written into the header comment so it is not re-derived per consumer.

---

## 3. THE SAVE-SLOT CONTRACT

- **Slot:** `USiegeSettingsSubsystem::SettingsSlotName` == `TEXT("SiegeSettings")`, defined once in `SiegeSettingsSubsystem.cpp`, **user index 0** ⇒ `Saved/SaveGames/SiegeSettings.sav`.
- **`USaveGame`, not `UGameUserSettings`** (§2 ruling). ⛔ **No `.ini` was touched** — no `GameUserSettingsClassName`, no config edit of any kind.
- **LOAD ONCE:** `Initialize()` → `LoadSettingsFromSlot()`. That is the **only disk read on any shipped path.** `IsAssistantConfirmEnabled()` is a pure in-memory getter — no disk, no allocation, no world — so the confirm-time read on the order path costs nothing.
- **SAVE ON CHANGE:** `SetAssistantConfirmEnabled` writes memory **and** the slot — **only when the value actually changes.** A same-value write does nothing at all: no disk write, no broadcast.
- **NULL-SAFE, cloned from `UDeckBuilderWidget::LoadSaveGame` rather than reinvented:** `DoesSaveGameExist` first (so a first run is silent instead of triggering the engine's own missing-slot warning), then `Cast<USiegeSettingsSaveGame>(LoadGameFromSlot(...))`. **Missing / unreadable / foreign-class ⇒ the C++ defaults, one `Log` line on `LogSiegeSettings`, never a crash.**
- **The fallback value is read off `GetDefault<USiegeSettingsSaveGame>()`**, not re-typed in the subsystem, so the SaveGame's C++ default *is* the fallback and the two cannot drift. A test asserts they agree anyway.
- **A failed disk write does NOT revert the in-memory value.** The player's toggle takes effect for the session; the failure is a `Warning`. Snapping a checkbox back under the player's finger because a save failed is the worse behaviour — flagged here because it is a judgement call, not a spec line.
- **Version story:** v1 carries **exactly one field** and **no version int**, deliberately. SaveGame archives are tagged-property serialized, so a field added later is simply absent from an old `.sav` and loads at its C++ default. That is the whole migration story and it is why this is not a one-setting class; `SiegeSettingsSubsystem.h` carries a "HOW TO ADD SETTING #2" block (4 steps, none of which touch the slot contract, the delegate, or any consumer).

---

## 4. ⛔ THE TOGGLE REMOVES A HUMAN REVIEW STEP AND NEVER A MACHINE CHECK — what my design can and cannot do to that law

**Asked directly, as the dispatch required: does anything here make it possible for the toggle to disable a validation? No — and here is the reason rather than the assertion.**

- This task ships **a bool, a slot and a delegate.** `USiegeSettingsSubsystem` has **no reference to the assistant, the FSM, the snapshot, the guard, the parser or the executor** — it includes `GameplayStatics`, its own SaveGame, and nothing else. It cannot skip a check it cannot name.
- It exposes **exactly one read** (`IsAssistantConfirmEnabled`) whose meaning is *"show the human a preview first"*. There is no "validation level", no enum, no bitmask and no second flag that a downstream task could mistake for a safety switch. **The narrowness is the safety property** — the API offers no shape into which "and also skip the guard" could be folded without inventing a new member.
- The one way this file could still be complicit is a **consumer** reading it as a global bypass. Two guards against that are in the code, not just here: the header states the §5 law verbatim beside the getter (every check that runs with the toggle ON runs with it OFF; it gates no key), and the resolution snippet fixes the unresolvable-subsystem fallback at **`true`**.
- ⚠️ **What I cannot close from inside TASK-436:** the actual ON/OFF paths live in **TASK-443**, which is not written yet. **This handoff cannot certify that the off-path still runs the parse, the `Kinds.Num() == Counts.Num()` invariant, the shortfall/clarification path, eligibility, the authority refusal and the non-orderable-kind guard** — TASK-443's own handoff owes that trace side by side, check for check, and QA gate TASK-446 owes the verification. **Nothing here should be read as evidence about the off-path.**

---

## 5. TESTS — `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeSettingsTest.cpp`

`IMPLEMENT_SIMPLE_AUTOMATION_TEST` under `#if WITH_DEV_AUTOMATION_TESTS`, flags `EditorContext | EngineFilter` — the `SiegeAssistantGrammarTest.cpp` idiom, copied.

| Test | Asserts |
|---|---|
| `Siegebound.Settings.SlotContract` | slot string is **exactly** `"SiegeSettings"` · user index is 0 · the delegate payload name is `"bAssistantConfirmBeforeExecute"` · the scratch slot ≠ the shipped slot |
| `Siegebound.Settings.DefaultIsConfirmOn` | the SaveGame CDO defaults to **true** · a fresh store reports confirm ON **before any load** · the subsystem's compiled default agrees with the SaveGame's (**drift guard**) · construction broadcasts nothing |
| `Siegebound.Settings.MissingSlotYieldsDefaults` | a load with no file yields the default and does not crash · repeated loads of a missing slot are idempotent and broadcast nothing · **after `set(false)` + the file being deleted, a load RESTORES the default** (the fallback assigns; it does not leave stale memory) |
| `Siegebound.Settings.ForeignSlotClassYieldsDefaults` | a readable slot holding a **`USiegeDeckSaveGame`** (corrupt / colliding / older build) falls back to the default instead of crashing — the case the `Cast<>` exists for |
| `Siegebound.Settings.SetGetRoundTrip` | `set(false)`→`get()==false` · `set(true)`→`get()==true` · a same-value write leaves the value alone |
| `Siegebound.Settings.SaveLoadRoundTrip` | `set(false)` wrote the slot · a **separate** store loads `false` · a load that changes the value broadcasts **once** · re-loading the same file broadcasts **nothing** · the `true` direction round-trips too |
| `Siegebound.Settings.DelegateFiresOnRealChangeOnly` | fresh store = 0 broadcasts · a real change broadcasts **exactly once** and names the confirm setting · **a same-value write broadcasts NOTHING** (×2) · changing back broadcasts again |

**⛔ THE TESTS CANNOT TOUCH JONATHAN'S REAL SETTINGS FILE, BY CONSTRUCTION.** Every store is redirected to `"SiegeSettings_AutomationScratch"` via `SetSlotNameForAutomationTests` **before** any load or save, and an RAII `FScratchSlotGuard` deletes that slot on the way into and out of each test. `Siegebound.Settings.SlotContract` asserts the two names differ, so the guarantee is mechanical. **A settings test that clobbered `Saved/SaveGames/SiegeSettings.sav` would be a real defect, not a nuisance** — that is why the seam exists.

---

## 6. ⚠️ WHAT I VERIFIED AGAINST THE ENGINE SOURCE (I could not compile — these are reads, not runs)

Three things I refused to guess at, with the file and line so QA can re-check:

1. **`UGameInstanceSubsystem` is `UCLASS(Abstract, Within = GameInstance, MinimalAPI)`** (`Engine/Public/Subsystems/GameInstanceSubsystem.h:15`). A bare `NewObject<USiegeSettingsSubsystem>()` defaults its Outer to the transient package (`UObjectGlobals.h:1958`) and would trip `check(... || InOuter->IsA(InClass->ClassWithin))` (`UObjectGlobals.cpp:3477`) plus the editor `ensureMsgf` at `:3313`. ⇒ **The tests construct a throwaway `UGameInstance` and build the subsystem inside it.** Had I not checked, the test suite would have asserted at runtime rather than failed to compile — the worst kind of "it passed review."
2. **`UGameplayStatics::SaveGameToMemory` does NOT set `ArIsSaveGame`** (`Runtime/Engine/Private/GameplayStatics.cpp:2371-2388`) — it serializes through a plain `FObjectAndNameAsStringProxyArchive`. ⇒ **a bare `UPROPERTY()` round-trips; `UPROPERTY(SaveGame)` is NOT required** here. This matters because the pin says `UPROPERTY()` and the widely-repeated folk rule says otherwise; the `USiegeDeckSaveGame` precedent (which persists correctly with a non-`SaveGame` UPROPERTY) agrees with the source.
3. **Every assertion helper used exists as an exact-match overload** — `TestEqual(const TCHAR*, const FString&, const FString&)` (`AutomationTest.h:1997`), `TestEqual(const TCHAR*, const int32, const int32)` (`:1985`), `TestNotEqual(const TCHAR*, const FString&, const FString&)` (`:2006`) — so no call falls through to the `FStringView`/template overloads and none is ambiguous. `TStrongObjectPtr` has `Reset(T*)`, `Get()`, `IsValid()`, `operator->` (`Core/Public/UObject/StrongObjectPtrTemplates.h:101,117,152`).

---

## 7. ⚠️ NOT VERIFIABLE WITHOUT A COMPILE — stated plainly, not softened

- **(a) NOTHING HERE HAS BEEN COMPILED, AND NO TEST HAS BEEN RUN.** There is no green tick behind any claim in §5 — those are the assertions the tests *will make*, not results. **First execution is TASK-447.** UHT correctness (the `UPROPERTY`/`UFUNCTION`/`GENERATED_BODY` glue, the dynamic-delegate codegen) is likewise unproven.
- **(b) The `L_MainMenu` → `L_Arena` level-crossing claim is architectural, not observed.** `UGameInstanceSubsystem` surviving `OpenLevel` is the documented engine contract and the reason §2 mandates the class, but **nobody has watched this value cross a level travel.** It closes on Jonathan's playtest (TASK-448), not here.
- **(c) `SiegeSettingsSaveGame.cpp` contains no definitions** — the slot contract is pinned onto the *subsystem* by §8, not onto the SaveGame (the one deliberate difference from the `USiegeDeckSaveGame` precedent, which puts `SlotName`/`UserIndex` on the SaveGame). The file exists because §2 pins the `.h/.cpp` pair. **If QA prefers it deleted, say so — it is a one-line change and I would rather be told than have a near-empty file read as an oversight.**
- **(d) The delegate test observes `SettingsChangeBroadcastCount`, not a bound listener.** The counter is incremented inside `BroadcastSettingChanged()`, which is the **only** function in the `.cpp` that calls `OnSettingsChanged.Broadcast` — so it cannot diverge — **but the counter is what is read.** Binding the real dynamic multicast needs a `UFUNCTION` on a `UCLASS`, and a `UCLASS` cannot be declared in a test `.cpp` (it requires a UHT-generated `.generated.h`, which is produced per-header). The end-to-end path — bind → toggle → widget repaints — closes on **Jonathan's pixel check**, per ruling 9.
- **(e) A same-value write skips the disk save as well as the broadcast.** Consequence, stated so it is a decision and not a surprise: on a first run, "setting" the value it already has writes no file. That is behaviourally identical to the absent-slot path, which loads exactly that default.
- **(f) `LoadSettingsFromSlot()` is public.** Nothing on a gameplay path calls it (only `Initialize()` does), and the header says so in as many words — but the *capability* to hit the disk later exists. Flagged rather than hidden.

---

## 8. M8 DECLARATION (verbatim, §8 — and why it is true here)

> **adds no replicated property, no new replicated class, no new relevancy tier.**

Not merely asserted: **this value is client-local by construction.** It governs whether a **local human review step** is shown before the local player's own order is issued — it never participates in an authoritative outcome, so there is nothing for a server to own, nothing for a client to predict, and no relevancy question to answer. Concretely: no `UPROPERTY(Replicated*)`, no `GetLifetimeReplicatedProps`, no `bReplicates`, no RPC, and no `NetMulticast`/`Server`/`Client` `UFUNCTION` in either class. The delegate is a `DECLARE_DYNAMIC_MULTICAST_DELEGATE` (a Blueprint-assignable local delegate), which is **not** network multicast.

---

## 9. FOR QA TO SCRUTINISE (my own list, in the order I would check it)

1. **The §8 pin, character-for-character** — slot string, delegate declaration, both `UFUNCTION` categories/specifiers, the `Initialize` override. A drift here breaks TASK-437 and TASK-443 at the link, and neither exists yet to notice.
2. **The `Within = GameInstance` construction in the tests** (§6 item 1). If my reading is wrong, the tests assert at runtime instead of failing to build.
3. **The no-op rule** — verify by inspection that `ApplyBoolSetting` is the **only** mutation path and `BroadcastSettingChanged` the **only** `Broadcast` call site. That structure, not a comment, is what makes "never on a no-op" true for every future setting.
4. **The load path never writes the disk** (`bPersistToDisk == false` on the load call) and the getter never reads it.
5. **§4's limits** — please do not read this handoff as evidence about the toggle's off-path; that is TASK-443/TASK-446's ground.
6. **Whether `SiegeSettingsSaveGame.cpp` should exist at all** (§7c).
