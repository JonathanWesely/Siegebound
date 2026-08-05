// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

// FKey. It is BOTH the key and the value type of the TMap<FKey, FKey> member below and is
// held BY VALUE, so the COMPLETE type is required here — included explicitly rather than
// inherited through SiegeKeyboardLayoutStatics.h (complete-type include law).
#include "InputCoreTypes.h"

// FTimerHandle — a USTRUCT held BY VALUE below for the 1 Hz layout poll. It lives in its own
// header (Engine/TimerHandle.h), NOT in CoreMinimal.h and NOT in EngineTypes.h.
#include "Engine/TimerHandle.h"

// FSiegePositionalKeyProbe — the element type of the TArray<> out-parameter on
// ProbeActiveLayout below, and the type this subsystem exists to fill in. TASK-509's file;
// this subsystem is its only shipped caller.
#include "Siegebound/SiegeKeyboardLayoutStatics.h"

#include "Subsystems/GameInstanceSubsystem.h"

#include "SiegeKeyboardLayoutSubsystem.generated.h"

// Held/passed only as a pointer in this header (including inside TObjectPtr<>, which UHT
// resolves from the generated code). Re-stated here rather than inherited from
// SiegeKeyboardLayoutStatics.h's own forward declaration — the .cpp includes the complete
// "InputMappingContext.h" for the members it actually dereferences.
class UInputMappingContext;

/**
 *  The positional-input log category (CONVENTIONS `KBD-§7`: `LogSiegeInputLayout`, declared
 *  in SiegeKeyboardLayoutSubsystem.h and defined in its .cpp, per the standing
 *  `LogSiege<Domain>` law in "Logging (C++)").
 */
DECLARE_LOG_CATEGORY_EXTERN(LogSiegeInputLayout, Log, All);

/**
 *  Fired when the ACTIVE OS keyboard layout produces a DIFFERENT positional translation than
 *  the one currently in force — i.e. on a real change, never on a no-op (the delegate law,
 *  CONVENTIONS "Delegates (C++)", matching FOnSiegeSettingsChanged's contract).
 *
 *  ⛔ IT IS NOT BROADCAST WHEN THE HKL MOVES BUT THE TRANSLATION DOES NOT. Switching between
 *  two layouts that are positionally identical (US-QWERTY <-> UK-QWERTY, say) changes the
 *  HKL and changes nothing a consumer could care about; a delegate that fires on that trains
 *  consumers to ignore it.
 *
 *  ⚠️ CONSUMERS DO NOT NEED THIS TO KEEP WORKING. It exists for diagnostics and for a future
 *  key-glyph UI. The remap itself needs no consumer at all: on a layout change the subsystem
 *  re-targets its CACHED DUPLICATE IN PLACE, so the pointer AHeroCharacter handed to
 *  AddMappingContext never changes (`KBD-§6`).
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSiegeKeyboardLayoutChanged);

/**
 *  ═══ POSITIONAL KEYBOARD-LAYOUT SUBSYSTEM (TASK-511, KEYBOARD-LAYOUT batch) ═══
 *
 *  Jonathan types on Dvorak. On Windows the FKey the engine delivers is LAYOUT-DEPENDENT:
 *  the physical key at the QWERTY-W position arrives as EKeys::Comma, so every letter
 *  binding in IMC_Hero (WASD, Q rally, T/R/E/F/C unit commands) is in the wrong PHYSICAL
 *  place. This subsystem owns the three platform pieces the pure statics deliberately do not:
 *  the Win32 scan-code probe, the duplicate-context cache, and the mid-session layout watch.
 *
 *  ⛔ THE ONE CALL-SITE CONTRACT IS GetPositionalContext(). AHeroCharacter resolves
 *  IMC_Hero through it before AddMappingContext (TASK-512, four lines) and needs nothing
 *  else — no new member, no re-application code, no binding to OnKeyboardLayoutChanged.
 *
 *  ⛔⛔ THE CENTRAL LAW (`KBD-§1`) — THE SOURCE ASSET IS ONLY EVER HELD THROUGH `const`
 *      POINTERS. THE ONLY NON-CONST UInputMappingContext* IN THIS ENTIRE FEATURE IS THE
 *      DUPLICATE. UEnhancedInputLocalPlayerSubsystem::AddMappingContext already takes a
 *      `const UInputMappingContext*` (EnhancedInputSubsystemInterface.h:265), so NO CAST IS
 *      NEEDED ANYWHERE — and a const_cast appearing in this pair is itself the finding.
 *      ⚠️ IN PIE THE SOURCE **IS** THE EDITOR'S LOADED IMC_Hero ASSET. Read-only access is
 *      what makes a PIE session structurally unable to dirty it. The precedent is this
 *      repo's own and it cost a playtest: handoffs/TASK-445-artist.md:95-105 — rewriting the
 *      mappings array silently default-constructed the instanced SwizzleAxis/Negate
 *      modifiers, breaking WASD and inverting mouse-look WHILE THE PROPERTY TABLE STILL READ
 *      CORRECT.
 *
 *  ⛔ FAIL-SAFE LAW (`KBD-§5`) — EVERY FAILURE DEGRADES TO THE UNTRANSLATED SOURCE CONTEXT.
 *  NEVER nullptr for a non-null Source, NEVER EKeys::Invalid, NEVER a half-retargeted
 *  duplicate. The worst outcome this feature may produce is "the game behaves exactly as it
 *  did yesterday."
 *
 *  ⛔ BANNED (`KBD-§2`): UInputMappingContext::MapKey / UnmapKey / UnmapAll. MapKey APPENDS a
 *  mapping built from the 2-arg ctor with EMPTY Modifiers and Triggers
 *  (InputMappingContext.cpp:154-158) — TASK-445's failure rewritten in C++. ALL key mutation
 *  in this feature goes through FSiegeKeyboardLayoutStatics::RetargetContextKeys, which
 *  assigns ONLY `.Key` and re-derives every value from the pristine source BY INDEX.
 *
 *  ─── THE THREE LAYOUT-CHANGE MECHANISMS, AND WHY THERE ARE THREE (`KBD-§6`) ───
 *
 *  ⚠️ Windows fires WM_INPUTLANGCHANGE and Slate DOES handle it
 *  (SlateApplication.cpp:5138-5141 -> FInputKeyManager::InitKeyMappings()), but
 *  FSlateApplication::OnInputLanguageChanged is a BARE VIRTUAL WITH NO DELEGATE — the whole
 *  Runtime tree was grepped and there is nothing to bind to. Hence three mechanisms, none of
 *  which is redundant:
 *
 *    1. RE-PROBE ON EVERY GetPositionalContext() CALL. Free (one GetKeyboardLayout(0) unless
 *       the HKL actually moved) and it covers a layout chosen before the process launched.
 *    2. FSlateApplication::OnApplicationActivationStateChanged (SlateApplication.h:1690-1691),
 *       bound in Initialize and ⛔ UNBOUND IN Deinitialize. Covers alt-tab out, change the
 *       layout, alt-tab back in.
 *    3. A 1 Hz HKL poll (LayoutPollIntervalSeconds), behind `siege.Input.LayoutPollEnabled`.
 *       ⭐ THE ONLY ONE THAT CATCHES AN IN-PLACE Win+Space, which is the realistic case:
 *       Dvorak users commonly toggle to QWERTY for games and back.
 *
 *  ⚠️ MID-SESSION SWITCHES ARE IN SCOPE BY JONATHAN'S OWN RULING (`KBD-§0` ruling 2). A
 *  subsystem that only probes at Initialize does not satisfy the directive.
 *
 *  ⛔ NO PLAYER-FACING SETTING (`KBD-§0` ruling 1). There is no USiegeSettingsSaveGame field,
 *  no settings-menu row and no SC-§8 registry entry for this. `siege.Input.LayoutPollEnabled`
 *  is a DEV/TEST lever read by nothing but the poll. Adding a settings toggle is a scope
 *  breach even if a reviewer agrees with it.
 *
 *  ─── WHY A UGameInstanceSubsystem ───
 *
 *  The active OS keyboard layout is a property of the PROCESS, not of a level, a pawn or a
 *  controller — and the duplicate contexts must outlive OpenLevel exactly as the pointer
 *  AHeroCharacter holds does. Precedents: USiegeSettingsSubsystem (SiegeSettingsSubsystem.h:88-89,
 *  Initialize at .cpp:22-34), USiegeSessionSubsystem, USiegeLlamaSubsystem.
 *
 *  M8 DECLARATION (verbatim, `KBD-§10`): adds no replicated property, no new replicated
 *  class, no new relevancy tier. ✅ AND THE REASON IS STRUCTURAL: a UGameInstanceSubsystem is
 *  ONE PER CLIENT PROCESS, client-local by construction. In a listen-server match the host
 *  and each joining client probe THEIR OWN OS layout, which is the only correct behaviour;
 *  the duplicate IMC lives in the transient package and is never seen by the network.
 *
 *  QA gate: TASK-513 (`.claude/pipeline/qa/TASK-513-keyboard-layout.md`).
 */
UCLASS()
class GITCLAUDEUNREALTEST_API USiegeKeyboardLayoutSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	/**
	 *  ⭐ THE ONE CALL-SITE CONTRACT. Hands back the mapping context that puts every letter
	 *  binding on the PHYSICAL key its QWERTY key occupies — which on a positionally-QWERTY
	 *  host is Source itself, unchanged and unallocated.
	 *
	 *  THE EARLY-RETURN LADDER **IS** THE FAIL-SAFE LAW (`KBD-§5`); QA traces each rung:
	 *    - Source == nullptr .................. return nullptr (the ONLY null this ever
	 *      returns; the caller's existing null guard already covers it);
	 *    - ⭐ TranslationMap is EMPTY ......... return SOURCE — THE SAME POINTER. No
	 *      duplicate, no allocation, nothing added to the cache. This is the QWERTY path and
	 *      it must stay free;
	 *    - a duplicate is already cached ...... return it;
	 *    - otherwise .......................... DuplicateObject into the transient package,
	 *      RetargetContextKeys, cache, return the duplicate;
	 *    - ⛔ ANY failure (duplicate is null, retarget refused) ... UE_LOG(Error) and return
	 *      SOURCE. ⛔ NEVER nullptr for a non-null input, and ⛔ never a half-retargeted
	 *      duplicate — a refused retarget writes nothing at all (TASK-509's refusals all
	 *      precede the first write), and the discarded duplicate is never cached and never
	 *      handed out.
	 *
	 *  ⚠️ NOT const, and it is not an oversight: this is mechanism 1 of 3 above. Every call
	 *  re-probes the active layout first, which is what catches a layout chosen before the
	 *  process launched, and it can populate the duplicate cache.
	 *
	 *  ⛔⛔ THIS IS DELIBERATELY **NOT** A UFUNCTION, AND THAT IS NOT AN OVERSIGHT EITHER
	 *      (`KBD-§8`). UHT REJECTS A `const UObject*` RETURN TYPE ON A REFLECTED FUNCTION. A
	 *      well-meaning "expose it to Blueprint" edit does not fail code review — it fails
	 *      UHT, loudly, at this batch's only compile gate. Use IsPositionalRemapActive() /
	 *      DescribeActiveTranslation() from Blueprint instead; both are reflected.
	 *
	 *  @param Source the pristine, on-disk context (IMC_Hero). ⛔ Held const throughout and
	 *                never written to on any path.
	 *  @return Source itself, or a transient duplicate whose `.Key` fields have been
	 *          retargeted. ⛔ Never nullptr unless Source was nullptr.
	 */
	const UInputMappingContext* GetPositionalContext(const UInputMappingContext* Source);

	/**
	 *  ⛔ MEANS `TranslationMap.Num() > 0` — i.e. "the host layout is NOT positionally
	 *  QWERTY." It does ⛔ NOT mean "a duplicate exists" (`KBD-§8` pins this because the two
	 *  diverge on the very first call: the map is built at Initialize, the first duplicate
	 *  only when a caller asks for one — so a test asserting the wrong one passes for the
	 *  wrong reason).
	 */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Input")
	bool IsPositionalRemapActive() const;

	/**
	 *  A human-readable dump of the active translation, for logs, QA and a future key-glyph
	 *  UI. ⛔ FORMAT IS PINNED (`KBD-§8`):
	 *    - entries separated by `", "`, source and target separated by `" → "`
	 *      (U+2192 RIGHTWARDS ARROW, with one space on each side);
	 *    - ⛔ SOURCE-KEY ORDER AS RETURNED BY GetQwertyLetterScanCodes(), i.e. **A..Z**;
	 *    - identity entries omitted;
	 *    - ⛔ the EMPTY STRING when the map is empty — not "none", not "(identity)".
	 *
	 *  ⚠️ DO NOT COPY `KBD-§8`'s WORKED EXAMPLE `"W → Comma, S → O, D → E"` AS A LITERAL
	 *  EXPECTATION: it illustrates the SEPARATORS and is NOT in A..Z order. On US-Dvorak this
	 *  function emits `"A → Q, C → J, D → E, ..."`. A byte/format claim about this string is
	 *  asserted with TestEqualSensitive, never TestEqual (`SC-§13` — TestEqual compares
	 *  FStrings case-INSENSITIVELY, so a byte claim made with it is vacuous).
	 */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Input")
	FString DescribeActiveTranslation() const;

	/**
	 *  Re-derives the translation from the OS if — and only if — the active layout handle has
	 *  moved, then re-targets every CACHED DUPLICATE IN PLACE from its pristine source and
	 *  asks Enhanced Input to rebuild the control mappings that use it.
	 *
	 *  ⭐ IDEMPOTENT AND SAFE TO CALL EVERY FRAME. When nothing has changed it costs one
	 *  GetKeyboardLayout(0) and returns; that is what lets GetPositionalContext call it
	 *  unconditionally (mechanism 1 of 3).
	 *
	 *  ⛔ IT NEVER SWAPS POINTERS AND NEVER RE-ADDS A MAPPING CONTEXT (`KBD-§6`). The cached
	 *  duplicate is mutated in place, so the pointer AHeroCharacter handed to
	 *  AddMappingContext stays valid and current — which is precisely why TASK-512 needs ZERO
	 *  new state and ZERO re-application code. A "refactor" that re-applies contexts from
	 *  here throws that property away and is refused.
	 *
	 *  ⛔ NEVER BROADCASTS ON A NO-OP: OnKeyboardLayoutChanged fires only when the newly
	 *  derived translation actually DIFFERS from the one in force.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Input")
	void RefreshKeyboardLayout();

	/**
	 *  ⭐ ADDED 2026-08-04 (batch ASSISTANT-EXCLUDE, TASK-516) — THE SINGLE-KEY QUERY.
	 *  "What does the physical position that QWERTY calls `QwertyKey` yield on the ACTIVE
	 *  layout?" `KBD-§4` already puts ALL 26 LETTERS in the translation, so the answer is
	 *  computed and stored; this is the only way to ask for ONE of them.
	 *
	 *  ⛔ DIRECTION — GETTING IT BACKWARDS COMPILES AND SILENTLY BINDS THE WRONG KEY. The map
	 *  is SOURCE (QWERTY) -> what the ACTIVE layout yields at that physical position.
	 *  ⇒ ON US-DVORAK, GetPositionalKey(EKeys::Z) RETURNS EKeys::Semicolon, because the
	 *  physical position QWERTY calls `Z` produces `;` on Dvorak.
	 *  ⇒ A key-press test is `InKeyEvent.GetKey() == GetPositionalKey(EKeys::Z)`, ⛔ NEVER a
	 *    reverse lookup of the pressed key back into QWERTY space.
	 *
	 *  ⛔ IT NEVER RETURNS EKeys::Invalid (`KBD-§5`'s fail-safe law applied to a scalar).
	 *  ⚠️ IDENTITY IS NEVER *STORED* (BuildTranslationMap emits no identity entry), so "no
	 *  entry" and "identity" are THE SAME ANSWER and both return the input unchanged — an
	 *  extra `if (Translated == QwertyKey)` branch would be dead code, do not add one.
	 *  ⚠️ AN INVALID INPUT RETURNS THAT SAME INVALID INPUT: this is a LOOKUP, not a
	 *  validator, and it must not start refusing keys the caller already holds.
	 *
	 *  ⛔⛔ `const`, AND THEREFORE IT DOES **NOT** RE-PROBE — unlike GetPositionalContext(),
	 *      which is non-const precisely because it re-probes (mechanism 1 of 3). It reads the
	 *      map AS IT STANDS. ⚖️ RULED (`KBD-§8`): THE CALLER REFRESHES, THE ACCESSOR READS.
	 *      ⇒ A caller that needs a current answer calls RefreshKeyboardLayout() ITSELF first —
	 *        USiegeAssistantConsoleWidget::OpenConsole() does it ONCE PER OPEN, which is free
	 *        by mechanism 1's own standard and stays correct even when
	 *        `siege.Input.LayoutPollEnabled` has been turned off for testing.
	 *      ⛔ DO NOT bind OnKeyboardLayoutChanged from a widget for this — new lifetime state
	 *        to unbind wrongly, for a value that is re-read at every open anyway.
	 *      ⛔ DO NOT "fix" the staleness by making this non-const, by adding a `mutable`
	 *        member, by const_cast-ing, or by hanging a timer off it.
	 *
	 *  ⛔⛔ AND THE PLAYER-FACING STRING DOES **NOT** USE THIS FUNCTION. ANY PROMPT THAT NAMES
	 *      THE ACCEPT KEY SAYS `Z`, ON EVERY LAYOUT (`KBD-§8`, `KBD-§0` ruling 1). The player
	 *      is on QWERTY HARDWARE with a Dvorak SOFTWARE layout — their keycap reads `Z`, so
	 *      telling them to "press `;`" would be the bug, not the fix.
	 *      ⇒ THE LOOKUP IS FOR THE COMPARISON; THE LITERAL IS FOR THE HUMAN.
	 *
	 *  M8 DECLARATION (verbatim): adds no replicated property, no new replicated class, no new
	 *  relevancy tier. ✅ It is a const read of one client-local map on a
	 *  UGameInstanceSubsystem — see the class comment for why that is structural.
	 *
	 *  ⚠️ QA gate for THIS addition is TASK-525 (`.claude/pipeline/qa/TASK-525.md`), NOT the
	 *  class-level TASK-513 gate below — this member arrived in a later, separate batch.
	 *
	 *  @param QwertyKey the key as it is PRINTED on a US-QWERTY reference, e.g. EKeys::Z.
	 *  @return the FKey the active layout yields at that physical position; ⛔ QwertyKey
	 *          itself when the map holds no entry for it (the QWERTY-host path, and every
	 *          fail-safe path). ⛔ Never EKeys::Invalid for a valid input.
	 */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Input")
	FKey GetPositionalKey(const FKey& QwertyKey) const;

	/**
	 *  Broadcast on every ACTUAL change of the active translation, never on a no-op. UI
	 *  consumers SEED FROM DescribeActiveTranslation()/IsPositionalRemapActive() FIRST, THEN
	 *  BIND (qa/TASK-005 major-2: a bind-only widget created at a pinned value stays stale).
	 */
	UPROPERTY(BlueprintAssignable, Category = "Siegebound|Input")
	FOnSiegeKeyboardLayoutChanged OnKeyboardLayoutChanged;

	/**
	 *  The 1 Hz layout poll's interval, in seconds (`KBD-§7`: a NAMED constant, ⛔ never a
	 *  bare 1.0f at the SetTimer call). Public and constexpr so an automation test can assert
	 *  it without reaching into the .cpp — the USiegeSettingsSubsystem::SettingsUserIndex
	 *  precedent (SiegeSettingsSubsystem.h:107).
	 *
	 *  ⚖️ 1 Hz is chosen against a HUMAN reaction, not a frame budget: the worst case is that
	 *  a Win+Space takes up to a second to take effect, which is imperceptible next to the
	 *  act of pressing Win+Space. The poll's cost when nothing changed is ONE
	 *  GetKeyboardLayout(0) — a thread-local read, no allocation, no table walk.
	 */
	static constexpr float LayoutPollIntervalSeconds = 1.0f;

	/**
	 *  ⛔⛔ AUTOMATION TESTS ONLY — NOTHING IN THE GAME MAY CALL THIS. Mirrors
	 *      USiegeSettingsSubsystem::SetSlotNameForAutomationTests (SiegeSettingsSubsystem.h:175),
	 *      and it is flagged exactly as loudly for the same reason.
	 *
	 *  Forces the in-memory translation to InTranslation so a test can drive BOTH pinned
	 *  states on ANY host: an EMPTY map forces the pass-through state (GetPositionalContext
	 *  returns the same pointer), a populated map forces the remap state.
	 *
	 *  ⚠️ AND IT LATCHES: after this is called, THIS INSTANCE STOPS PROBING THE OS ENTIRELY —
	 *  RefreshKeyboardLayout() becomes a no-op. ⛔ THAT LATCH IS LOAD-BEARING, NOT A
	 *  CONVENIENCE: GetPositionalContext re-probes on every call (mechanism 1 of 3), so
	 *  without it the very next call on a QWERTY host would rebuild an EMPTY translation and
	 *  silently destroy the map the test just injected — the test would then pass or fail for
	 *  a reason that has nothing to do with the code under test. The seam that lets every
	 *  TASK-510 test run on a QWERTY machine is FSiegeKeyResolver; this is the seam that lets
	 *  the SUBSYSTEM's two states be driven on one.
	 *
	 *  ⚠️ It also DROPS the duplicate cache (a duplicate built under the previous map carries
	 *  the previous map's keys), does NOT touch CachedLayoutHandle, does NOT start or stop
	 *  the poll timer, and ⛔ does NOT broadcast OnKeyboardLayoutChanged — it is a test seam,
	 *  not a detected layout change. The latch is permanent for the instance; a test that
	 *  wants a probing subsystem constructs a fresh one.
	 */
	void SetTranslationMapForAutomationTests(const TMap<FKey, FKey>& InTranslation);

	//~ Begin USubsystem interface
	/**
	 *  Logs the scan-code self-check, binds the two Windows change hooks, and takes the first
	 *  layout probe. ⛔ Never fails, never crashes: on any platform without a probe the
	 *  translation is simply empty and the game behaves exactly as it does today.
	 */
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/**
	 *  ⛔ UNBINDS OnApplicationActivationStateChanged AND CLEARS THE POLL TIMER. Both are
	 *  checked by `KBD-§9` criterion 12. A subsystem that outlives its bindings is a crash on
	 *  the next application activation, and FSlateApplication outlives the GameInstance.
	 */
	virtual void Deinitialize() override;
	//~ End USubsystem interface

private:

	/**
	 *  Fills OutProbes (the 26-row A..Z table from GetQwertyLetterScanCodes()) with the
	 *  VirtualKey/CharCode the ACTIVE layout yields at each scan-code position, and reports
	 *  the layout handle it read them from.
	 *
	 *  ⛔ EVERY Win32 SYMBOL IN THIS FEATURE IS REACHED THROUGH THIS FUNCTION AND ITS SIBLING
	 *  ReadActiveLayoutHandle() — both forward to ONE `#if PLATFORM_WINDOWS`-fenced static in
	 *  the .cpp. The header stays Win32-free (`KBD-§6`), which is why the handle is carried
	 *  as an opaque uint64 rather than an HKL.
	 *
	 *  ⛔ NON-WINDOWS: clears both out-parameters and returns 0 — no probes, an empty
	 *  translation, pure pass-through, one Log line at Initialize. Mac/Linux compile clean
	 *  and behave exactly as they do today (`KBD-§11`: that is CORRECT, not broken).
	 *
	 *  @return how many of the 26 positions the OS actually answered (VirtualKey != 0).
	 */
	int32 ProbeActiveLayout(TArray<FSiegePositionalKeyProbe>& OutProbes, uint64& OutLayoutHandle) const;

	/**
	 *  The cheap half of the probe: the active layout handle (HKL as an opaque uint64), and
	 *  nothing else. 0 means "no layout known" — the initial state, a failed read, and the
	 *  permanent state on every non-Windows platform.
	 *
	 *  ⭐ THIS IS WHAT MAKES BOTH THE 1 Hz POLL AND THE PER-CALL RE-PROBE FREE: comparing the
	 *  handle costs one thread-local read, and the 26-position table walk only happens when
	 *  the handle has actually moved.
	 */
	uint64 ReadActiveLayoutHandle() const;

	/**
	 *  Mechanism 2 of 3: bound to FSlateApplication::OnApplicationActivationStateChanged in
	 *  Initialize, unbound in Deinitialize. Refreshes on the way BACK IN only — the layout
	 *  that matters is the one active while the game has focus, and a change made while we
	 *  were away is exactly what this hook exists to catch.
	 */
	void HandleApplicationActivationChanged(bool bIsActive);

	/**
	 *  Mechanism 3 of 3: the LayoutPollIntervalSeconds timer callback. Gated on
	 *  `siege.Input.LayoutPollEnabled` (read LIVE, so the lever works at runtime rather than
	 *  only at Initialize), then compares the active layout handle against CachedLayoutHandle
	 *  and refreshes only on a difference.
	 */
	void PollForLayoutChange();

	/**
	 *  The layout handle the current TranslationMap was derived from — an HKL stored as an
	 *  opaque uint64, which is what keeps this header free of Windows.h (`KBD-§6`).
	 *  ⭐ 0 means "no layout known": the initial state and the permanent non-Windows state,
	 *  which is why the pass-through path there costs nothing at all.
	 */
	uint64 CachedLayoutHandle = 0;

	/**
	 *  pristine source context -> its transient retargeted duplicate.
	 *
	 *  ⛔ THIS UPROPERTY IS THE DUPLICATES' ONLY GC ROOT. A transient DuplicateObject whose
	 *  outer is the transient package is NOT kept alive by its outer, so dropping this
	 *  UPROPERTY (or "simplifying" it to a raw TMap) collects a context AHeroCharacter is
	 *  still holding.
	 *
	 *  ⛔ THE KEY IS `TObjectPtr<const ...>` ON PURPOSE — it is the reviewable invariant in
	 *  type form (`KBD-§1`): the source is held const even in the cache, so no path in this
	 *  class can write to IMC_Hero. The engine's own precedent for a const TObjectPtr as a
	 *  UPROPERTY TMap key is AnimBlueprintGeneratedClass.h:427.
	 *
	 *  ⚠️ It also keeps the SOURCE asset resident for the GameInstance's lifetime. That is
	 *  intended and harmless — IMC_Hero is referenced by the hero anyway — but it is why this
	 *  map is keyed by the source rather than by a name: identity, not spelling, is what
	 *  makes a cache hit correct.
	 */
	UPROPERTY(Transient)
	TMap<TObjectPtr<const UInputMappingContext>, TObjectPtr<UInputMappingContext>> PositionalContexts;

	/**
	 *  QWERTY FKey -> active-layout FKey, from FSiegeKeyboardLayoutStatics::BuildTranslationMap.
	 *  ⭐ EMPTY means the host is POSITIONALLY QWERTY, and that is the whole of what
	 *  IsPositionalRemapActive() reports. Identity entries are never present.
	 */
	TMap<FKey, FKey> TranslationMap;

	/** The OnApplicationActivationStateChanged binding, so Deinitialize can remove exactly it. */
	FDelegateHandle ActivationChangedHandle;

	/** The LayoutPollIntervalSeconds looping timer, held on the GameInstance's timer manager. */
	FTimerHandle LayoutPollTimerHandle;

	/**
	 *  ⛔ AUTOMATION ONLY (SetTranslationMapForAutomationTests). FALSE ON EVERY SHIPPED PATH —
	 *  nothing in the game sets it — and while it is true this instance never probes the OS.
	 *  See the setter's comment for why the latch is required rather than convenient.
	 */
	bool bTranslationOverriddenForTests = false;
};
