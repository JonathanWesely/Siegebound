// Copyright Epic Games, Inc. All Rights Reserved.

#include "Siegebound/SiegeKeyboardLayoutSubsystem.h"

// Complete-type include law (TASK-110, and it is a HARD COMPILE ERROR rather than a warning).
// Everything this translation unit dereferences, upcasts or calls a method on is included
// here explicitly rather than inherited transitively through another header:
//   - FEnhancedActionKeyMapping, the element type of the TArray<> returned by
//     UInputMappingContext::GetMappings() (which this file calls Num() on)
#include "EnhancedActionKeyMapping.h"
//   - UEnhancedInputLibrary::RequestRebuildControlMappingsUsingContext (EnhancedInputLibrary.h:36-37)
#include "EnhancedInputLibrary.h"
//   - UGameInstance, for GetGameInstance()->GetTimerManager() (GetGameInstance() returns a
//     forward-declared pointer from Subsystems/GameInstanceSubsystem.h)
#include "Engine/GameInstance.h"
//   - FSlateApplication::IsInitialized / Get / OnApplicationActivationStateChanged
#include "Framework/Application/SlateApplication.h"
//   - TAutoConsoleVariable for siege.Input.LayoutPollEnabled
#include "HAL/IConsoleManager.h"
//   - EKeys::W (the self-check row) and FKey::ToString
#include "InputCoreTypes.h"
//   - UInputMappingContext: StaticClass(), GetName(), and the DuplicateObject instantiation
#include "InputMappingContext.h"
//   - FTimerManager::SetTimer / ClearTimer (UGameInstance::GetTimerManager returns a ref to
//     an incomplete type without this)
#include "TimerManager.h"
//   - UPackage, because GetTransientPackage() returns UPackage* and passing it where a
//     UObject* is expected is an UPCAST, which needs the complete type
#include "UObject/Package.h"
//   - DuplicateObject, MakeUniqueObjectName, GetTransientPackage
#include "UObject/UObjectGlobals.h"

// ⛔ THE WINDOWS HEADER GOES LAST, ON PURPOSE. Windows.h defines macros (TEXT, GetObject,
// min/max, ...) that poison anything included after it; WindowsHWrapper.h brackets the
// include with Pre/PostWindowsApi.h, but keeping it after every UE header removes the
// question entirely. It is what InputCore/Private/Windows/WindowsPlatformInput.cpp:4 uses for
// this same API family, it lives in **Core**, and `user32.lib` is a UBT default
// (UEBuildWindows.cs:2093).
// ⛔ THEREFORE: NO Build.cs CHANGE, AND `ApplicationCore` IS NOT NEEDED (`KBD-§6`). A
// Build.cs edit in this batch is a finding — it means somebody reached for the wrong header.
#if PLATFORM_WINDOWS
#include "Windows/WindowsHWrapper.h"
#endif

// The ONE definition of the positional-input log category (`KBD-§7` — LogSiegeInputLayout
// lives in SiegeKeyboardLayoutSubsystem.{h,cpp}).
DEFINE_LOG_CATEGORY(LogSiegeInputLayout);

namespace SiegeKeyboardLayoutCVars
{
#if PLATFORM_WINDOWS

	/**
	 *  ⭐ THE REPO'S FIRST CONSOLE VARIABLE, AND THE NAMING PATTERN WAS PINNED IN
	 *  CONVENTIONS `KBD-§7` BEFORE THIS TASK ISSUED: `siege.<Domain>.<Thing>` — a lowercase
	 *  `siege.` prefix mirroring the engine's own `r.` / `net.` / `a.` families, PascalCase
	 *  after it. ⛔ Character-for-character: `siege.Input.LayoutPollEnabled`, int32, default
	 *  1 (ON), ECVF_Default.
	 *
	 *  ⛔ IT IS A DEV/TEST LEVER, NEVER A PLAYER-FACING SETTING (`KBD-§0` ruling 1). No UI
	 *  reads it, it gets no settings row and no USiegeSettingsSaveGame field. Jonathan ruled
	 *  the positional remap is a CORRECTNESS fix, not a preference — so the thing this CVar
	 *  disables is the 1 Hz *poll*, never the remap. With it off, mechanisms 1 and 2 (the
	 *  per-call re-probe and the application-activation hook) still run and a mid-session
	 *  Win+Space is still picked up, just later.
	 *
	 *  ⚠️ Read LIVE on every poll tick rather than latched at Initialize, so that typing
	 *  `siege.Input.LayoutPollEnabled 0` in the console actually stops the poll and `1`
	 *  restarts it. A lever that only takes effect at startup is not a runtime test lever.
	 *
	 *  ⛔ FENCED WITH THE POLL IT GOVERNS (`KBD-§6`: fence the poll timer and the activation
	 *  hook too). On a platform with no probe there is no poll to enable, and a console
	 *  variable that provably does nothing is worse than an absent one.
	 */
	static TAutoConsoleVariable<int32> CVarLayoutPollEnabled(
		TEXT("siege.Input.LayoutPollEnabled"),
		1,
		TEXT("1 (default) = poll the active Windows keyboard layout once per second and re-derive ")
		TEXT("the positional letter remap when it changes (this is what catches an in-place Win+Space). ")
		TEXT("0 = stop polling; the per-call re-probe and the application-activation hook still run. ")
		TEXT("DEV/TEST LEVER ONLY — it is not a player setting and never disables the remap itself."),
		ECVF_Default);

#endif // PLATFORM_WINDOWS
}

namespace SiegeKeyboardLayoutPlatform
{
#if PLATFORM_WINDOWS

	/**
	 *  ⛔⛔ THE ONE FUNCTION IN THIS ENTIRE FEATURE THAT TOUCHES WIN32. Every HKL,
	 *      GetKeyboardLayout and MapVirtualKeyEx symbol in the module is inside this body
	 *      (`KBD-§6`), which is what lets SiegeKeyboardLayoutSubsystem.h — and the whole of
	 *      SiegeKeyboardLayoutStatics.{h,cpp} — stay platform-agnostic.
	 *
	 *  Two jobs, because the caller has two very different needs and only one of them is
	 *  allowed to cost anything:
	 *    - InOutProbes == nullptr : read the active layout HANDLE and stop. This is the 1 Hz
	 *      poll's and the per-call re-probe's entire cost — one thread-local read.
	 *    - InOutProbes != nullptr : also fill every row's VirtualKey and CharCode from that
	 *      layout. 26 positions, 52 MapVirtualKeyEx calls, and it only runs when the handle
	 *      has actually moved.
	 *  ⚖️ A pointer rather than a bool on purpose: the parameter that is absent IS the work
	 *  that is skipped, so there is no boolean at the call site to read backwards.
	 *
	 *  ⛔ check(IsInGameThread()) IS NON-NEGOTIABLE (`KBD-§6`): GetKeyboardLayout(0) reports
	 *  the layout of the CALLING THREAD, so answering it off the game thread would silently
	 *  describe the wrong keyboard.
	 *
	 *  Engine precedent for both calls, in the same direction, on the same numbers:
	 *  WindowsApplication.cpp:3296 and :3377 (scan code -> VK, MAPVK_VSC_TO_VK_EX) and
	 *  WindowsApplication.cpp:3316-3317 (VK -> character, MAPVK_VK_TO_CHAR).
	 *
	 *  ⛔⛔ DO NOT MASK THE DEAD-KEY BIT 0x80000000 OUT OF CharCode. MapVirtualKeyEx sets the
	 *      top bit for a dead key, and WindowsApplication.cpp:3317 DOES NOT STRIP IT before
	 *      handing the value to FSlateApplication::OnKeyDown -> GetKeyFromCodes. Our table
	 *      must match runtime BYTE FOR BYTE or it stops agreeing with the key the player
	 *      actually presses, so the value is stored exactly as the OS returned it. ⚠️ This is
	 *      the single most "tidy-able" line in the feature — it looks like a leftover flag.
	 *      It is not. Leave it.
	 *
	 *  @param OutLayoutHandle  the HKL as an opaque uint64; 0 on any failure.
	 *  @param InOutProbes      optional; when supplied it MUST already carry the 26-row A..Z
	 *                          table from GetQwertyLetterScanCodes(), and every row's
	 *                          VirtualKey/CharCode is overwritten (0 for a position the OS
	 *                          could not answer, which BuildTranslationMap then skips).
	 *  @return how many positions the OS answered; 0 when only the handle was requested.
	 */
	static int32 ProbeActiveKeyboardLayout(uint64& OutLayoutHandle, TArray<FSiegePositionalKeyProbe>* InOutProbes)
	{
		check(IsInGameThread());

		OutLayoutHandle = 0;

		const HKL LayoutHandle = ::GetKeyboardLayout(0);
		if (LayoutHandle == nullptr)
		{
			// Fail-safe (`KBD-§5`): no handle means no probe, which means an empty
			// translation, which means the game behaves exactly as it did yesterday.
			if (InOutProbes)
			{
				InOutProbes->Reset();
			}
			return 0;
		}

		OutLayoutHandle = static_cast<uint64>(reinterpret_cast<UPTRINT>(LayoutHandle));

		if (!InOutProbes)
		{
			return 0;
		}

		int32 NumAnswered = 0;

		for (FSiegePositionalKeyProbe& Probe : *InOutProbes)
		{
			// Always overwritten, never merged: a re-probe after a Win+Space must not
			// inherit a single field from the layout it just left.
			Probe.VirtualKey = 0;
			Probe.CharCode = 0;

			if (Probe.ScanCode == 0)
			{
				continue;
			}

			// Scan code -> the virtual key THIS layout puts at that physical position. The
			// scan code is a property of the hardware and never moves; the VK is what the
			// layout decides. That invariance is the entire mechanism this feature stands on.
			const uint32 VirtualKey = ::MapVirtualKeyEx(Probe.ScanCode, MAPVK_VSC_TO_VK_EX, LayoutHandle);
			if (VirtualKey == 0)
			{
				// The OS has no answer for this position on this layout. Left at 0, which
				// BuildTranslationMap skips — that letter simply keeps its source key.
				continue;
			}

			Probe.VirtualKey = VirtualKey;

			// ⛔ UNMASKED. See the dead-key clause above.
			Probe.CharCode = ::MapVirtualKeyEx(VirtualKey, MAPVK_VK_TO_CHAR, LayoutHandle);

			++NumAnswered;
		}

		return NumAnswered;
	}

#else

	/**
	 *  NON-WINDOWS (`KBD-§6`, `KBD-§11`): there is no probe, so there are no probes. Clears
	 *  both out-parameters and returns 0 ⇒ an empty translation ⇒ pure pass-through.
	 *  ⛔ Mac/Linux COMPILE CLEAN AND BEHAVE EXACTLY AS THEY DO TODAY. macOS
	 *  (TISCopyCurrentKeyboardLayoutInputSource) and Linux (XKB) would each need their own
	 *  probe; until then pass-through is the CORRECT outcome, not a broken one.
	 */
	static int32 ProbeActiveKeyboardLayout(uint64& OutLayoutHandle, TArray<FSiegePositionalKeyProbe>* InOutProbes)
	{
		OutLayoutHandle = 0;
		if (InOutProbes)
		{
			InOutProbes->Reset();
		}
		return 0;
	}

#endif // PLATFORM_WINDOWS
}

void USiegeKeyboardLayoutSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

#if PLATFORM_WINDOWS

	// ─── ⭐ THE SCAN-CODE SELF-CHECK. IT SHIPS; IT IS NOT A DEBUG EXTRA (`KBD-§6`) ───
	//
	// ⚖️ NO SCAN-CODE TABLE EXISTS ANYWHERE IN UE 5.8 SOURCE TO CROSS-CHECK OURS AGAINST, so
	// this one line is the ONLY validation of the hand-authored table in
	// SiegeKeyboardLayoutStatics.cpp against the actual OS. On a US-QWERTY host,
	// MapVirtualKeyEx(0x11, MAPVK_VSC_TO_VK_EX, hkl) MUST return 'W' (0x57).
	//
	// ⚠️ IT IS READ OFF THE SHIPPED TABLE ROW RATHER THAN FROM A SECOND HARD-CODED 0x11, so
	// it validates the row the game actually uses. And it is ⛔ LOGGED, NEVER check()ed: on
	// Jonathan's Dvorak host the correct answer is VK_OEM_COMMA (0xBC), which is the whole
	// reason this feature exists. An assert here would fire on exactly the machine the
	// feature is for.
	{
		TArray<FSiegePositionalKeyProbe> SelfCheckProbes;
		uint64 SelfCheckLayoutHandle = 0;
		const int32 NumAnswered = ProbeActiveLayout(SelfCheckProbes, SelfCheckLayoutHandle);

		const FSiegePositionalKeyProbe* WProbe = SelfCheckProbes.FindByPredicate(
			[](const FSiegePositionalKeyProbe& Candidate) { return Candidate.QwertyKey == EKeys::W; });

		if (WProbe)
		{
			const bool bMatchesUsQwerty = (WProbe->VirtualKey == 0x57);
			UE_LOG(LogSiegeInputLayout, Log,
				TEXT("[SiegeInputLayout] Scan-code self-check: scancode 0x%02X (the QWERTY 'W' position) -> VK 0x%02X, CharCode 0x%08X, FKey '%s'. %s (%d/%d positions answered, layout handle %llu)."),
				WProbe->ScanCode,
				WProbe->VirtualKey,
				WProbe->CharCode,
				*FSiegeKeyboardLayoutStatics::ResolveKeyFromCodes(WProbe->VirtualKey, WProbe->CharCode).ToString(),
				bMatchesUsQwerty
					? TEXT("MATCHES US-QWERTY (VK 0x57 'W') — the scan-code table agrees with the OS")
					: TEXT("differs from US-QWERTY's VK 0x57 'W' — EXPECTED on a non-QWERTY layout, and it is exactly what this feature exists to correct"),
				NumAnswered,
				SelfCheckProbes.Num(),
				SelfCheckLayoutHandle);
		}
		else
		{
			// Only reachable if GetQwertyLetterScanCodes() lost its W row or the probe
			// cleared the array (no layout handle). Neither is fatal — the translation is
			// simply empty and the game behaves as it did yesterday.
			UE_LOG(LogSiegeInputLayout, Error,
				TEXT("[SiegeInputLayout] Scan-code self-check could not run: the probe table has no EKeys::W row (%d row(s), layout handle %llu). The positional remap will degrade to pass-through."),
				SelfCheckProbes.Num(), SelfCheckLayoutHandle);
		}
	}

	// ─── MECHANISM 2 OF 3: THE APPLICATION-ACTIVATION HOOK (`KBD-§6`) ───
	//
	// ⚠️ WM_INPUTLANGCHANGE reaches Slate (SlateApplication.cpp:5138-5141 ->
	// FInputKeyManager::InitKeyMappings()) but FSlateApplication::OnInputLanguageChanged is a
	// BARE VIRTUAL WITH NO DELEGATE — there is nothing to bind to, which is why this feature
	// needs three mechanisms instead of one. This one covers alt-tab out, change the layout,
	// alt-tab back in.
	//
	// ⛔ IsInitialized() BEFORE Get() (the SiegeAssistantConsoleWidget.cpp:1024 precedent):
	// a commandlet or a -nullrhi automation run has no Slate application and Get() would
	// assert. ⛔ AND IT IS UNBOUND IN Deinitialize — FSlateApplication OUTLIVES THE
	// GameInstance, so a surviving binding is a crash on the next activation.
	if (FSlateApplication::IsInitialized())
	{
		ActivationChangedHandle = FSlateApplication::Get().OnApplicationActivationStateChanged().AddUObject(
			this, &USiegeKeyboardLayoutSubsystem::HandleApplicationActivationChanged);
	}
	else
	{
		UE_LOG(LogSiegeInputLayout, Log,
			TEXT("[SiegeInputLayout] Slate is not initialised (commandlet / -nullrhi), so the application-activation hook is not bound. The per-call re-probe and the 1 Hz poll still cover a mid-session layout change."));
	}

	// ─── MECHANISM 3 OF 3: THE 1 Hz HKL POLL (`KBD-§6`, `KBD-§0` ruling 2) ───
	//
	// ⭐ THE ONLY ONE OF THE THREE THAT CATCHES AN IN-PLACE Win+Space — no window focus
	// changes and no context is re-applied, so nothing else in the engine would ever notice.
	// That is the realistic case: Dvorak users commonly toggle to QWERTY for games and back.
	//
	// ⚠️ THE GAME INSTANCE'S TIMER MANAGER, NOT THE WORLD'S, AND THAT IS THE REQUIREMENT
	// RATHER THAN A PREFERENCE: this subsystem outlives every OpenLevel, and a world timer
	// dies with its world. UWorld::GetTimerManager() forwards to exactly this manager when a
	// game instance owns the world (World.cpp:8056-8059), so it ticks on the normal schedule.
	if (UGameInstance* OwningGameInstance = GetGameInstance())
	{
		OwningGameInstance->GetTimerManager().SetTimer(
			LayoutPollTimerHandle,
			this,
			&USiegeKeyboardLayoutSubsystem::PollForLayoutChange,
			LayoutPollIntervalSeconds,   // ⛔ the named constant (`KBD-§7`), never a bare 1.0f
			/*bLoop*/ true);
	}
	else
	{
		UE_LOG(LogSiegeInputLayout, Error,
			TEXT("[SiegeInputLayout] No owning UGameInstance at Initialize, so the 1 Hz layout poll was not started. The per-call re-probe still runs; an in-place Win+Space will only be picked up on the next GetPositionalContext call."));
	}

#else

	// ⛔ THE ONE NON-WINDOWS LOG LINE (`KBD-§6` point 6). Pass-through there is CORRECT, not
	// broken (`KBD-§11`): macOS would need TISCopyCurrentKeyboardLayoutInputSource and Linux
	// XKB, and neither is in this batch's scope.
	UE_LOG(LogSiegeInputLayout, Log,
		TEXT("[SiegeInputLayout] The positional keyboard remap is WINDOWS-ONLY. On this platform there is no layout probe, so the translation stays empty and every mapping context is handed back unchanged — behaviour is byte-identical to a build without this feature."));

#endif // PLATFORM_WINDOWS

	// The first probe. Deliberately the SAME path a mid-session change takes, so there is one
	// implementation of "derive the translation" rather than a startup copy that can drift.
	// ⚠️ On a non-QWERTY host this broadcasts OnKeyboardLayoutChanged into an empty delegate
	// list (nothing can have bound yet at Initialize). That is harmless and is preferred to a
	// special case that would have to be kept honest forever.
	RefreshKeyboardLayout();

	// Hoisted rather than called inside the UE_LOG argument list: mixing `*FStringTemporary`
	// with a TEXT() literal in a ternary makes the two branches different pointer types and
	// puts a temporary's lifetime inside a varargs call. Both are legal; neither is worth
	// making a reader check.
	const FString ActiveTranslationDescription = DescribeActiveTranslation();

	UE_LOG(LogSiegeInputLayout, Log,
		TEXT("[SiegeInputLayout] Subsystem initialized — positional remap %s, %d translated letter position(s). Translation: [%s]"),
		IsPositionalRemapActive()
			? TEXT("ACTIVE")
			: TEXT("inactive (the host layout is positionally QWERTY, so every mapping context is handed back unchanged)"),
		TranslationMap.Num(),
		*ActiveTranslationDescription);
}

void USiegeKeyboardLayoutSubsystem::Deinitialize()
{
#if PLATFORM_WINDOWS

	// ⛔ UNBIND THE ACTIVATION HOOK (`KBD-§9` criterion 12 checks exactly this).
	// FSlateApplication is a process-lifetime singleton and OUTLIVES the GameInstance, so a
	// binding left behind here is a call into a destroyed UObject on the next alt-tab.
	// Remove(Handle) rather than RemoveAll(this): it removes precisely what we added.
	if (ActivationChangedHandle.IsValid())
	{
		if (FSlateApplication::IsInitialized())
		{
			FSlateApplication::Get().OnApplicationActivationStateChanged().Remove(ActivationChangedHandle);
		}
		ActivationChangedHandle.Reset();
	}

	// ⛔ AND CLEAR THE POLL TIMER, for the same reason in the other direction: the timer holds
	// a weak reference to this object, but clearing it is what makes the teardown ordered
	// rather than lucky.
	if (LayoutPollTimerHandle.IsValid())
	{
		if (UGameInstance* OwningGameInstance = GetGameInstance())
		{
			OwningGameInstance->GetTimerManager().ClearTimer(LayoutPollTimerHandle);
		}
		LayoutPollTimerHandle.Invalidate();
	}

#endif // PLATFORM_WINDOWS

	// The duplicates lose their only GC root here, which is correct: nothing may still be
	// holding one once the GameInstance is going away. ⛔ The SOURCE contexts are released,
	// never destroyed — they are owned by their packages and this class only ever held them
	// through const pointers (`KBD-§1`).
	PositionalContexts.Reset();
	TranslationMap.Reset();
	CachedLayoutHandle = 0;

	Super::Deinitialize();
}

const UInputMappingContext* USiegeKeyboardLayoutSubsystem::GetPositionalContext(const UInputMappingContext* Source)
{
	// ─── RUNG 1: null in, null out (`KBD-§5`, row 1). ───
	// ⛔ THE ONLY nullptr THIS FUNCTION EVER RETURNS, and it is checked FIRST so a null input
	// costs nothing and has no side effects at all. AHeroCharacter's existing
	// `if (HeroMappingContext)` guard already covers it.
	if (!Source)
	{
		return nullptr;
	}

	// ─── MECHANISM 1 OF 3: re-probe on every call (`KBD-§6`). ───
	// Idempotent and ~free: one GetKeyboardLayout(0) unless the HKL actually moved. This is
	// what catches a layout that was already set before the process launched, and it is why
	// this function is not const.
	RefreshKeyboardLayout();

	// ─── RUNG 2: ⭐ THE QWERTY PATH, AND IT MUST STAY FREE (`KBD-§5`, row 2). ───
	// The SAME POINTER back. No DuplicateObject, no allocation, nothing entered in the cache
	// — on the overwhelmingly common host this whole feature costs one map-size check.
	// ⚠️ Note the ORDER: this sits ABOVE the cache lookup on purpose. After a switch BACK to
	// QWERTY the map empties and new callers get Source again, while a duplicate already
	// handed out stays valid and has been retargeted to identity in place by
	// RefreshKeyboardLayout — so both pointers behave identically. That is the intended
	// outcome of never swapping a pointer under AHeroCharacter (`KBD-§6`).
	if (TranslationMap.Num() == 0)
	{
		return Source;
	}

	// ─── RUNG 3: the cache. ───
	if (const TObjectPtr<UInputMappingContext>* CachedDuplicate = PositionalContexts.Find(Source))
	{
		if (*CachedDuplicate)
		{
			return *CachedDuplicate;
		}

		// Should be unreachable — the UPROPERTY map is the duplicate's GC root — but if a
		// duplicate ever went null we rebuild rather than hand back null (`KBD-§5`).
		UE_LOG(LogSiegeInputLayout, Error,
			TEXT("[SiegeInputLayout] The cached positional duplicate of '%s' was null; rebuilding it."),
			*Source->GetName());
		PositionalContexts.Remove(Source);
	}

	// ─── RUNG 4: build it. ───
	// ⛔ INTO THE TRANSIENT PACKAGE. The duplicate is derived state with the lifetime of a
	// process, it is never saved, and it is never seen by the network (`KBD-§10`).
	// ⛔ AND NOTE WHAT IS *NOT* HERE: no const_cast. DuplicateObject<T> takes `T const*`
	// (UObjectGlobals.h:2016) and AddMappingContext takes a `const UInputMappingContext*`
	// (EnhancedInputSubsystemInterface.h:265), so the source is held const from end to end
	// and the duplicate is the ONLY non-const UInputMappingContext* in the feature
	// (`KBD-§1`). ⚠️ In PIE that source IS the editor's loaded IMC_Hero asset — this is what
	// makes a PIE session structurally unable to dirty it.
	const FString DuplicateBaseName = FString::Printf(TEXT("%s_Positional"), *Source->GetName());
	const FName DuplicateName = MakeUniqueObjectName(
		GetTransientPackage(), UInputMappingContext::StaticClass(), FName(*DuplicateBaseName));

	UInputMappingContext* Duplicate = DuplicateObject<UInputMappingContext>(Source, GetTransientPackage(), DuplicateName);
	if (!Duplicate)
	{
		UE_LOG(LogSiegeInputLayout, Error,
			TEXT("[SiegeInputLayout] DuplicateObject failed for '%s'; handing back the untranslated source context. Letter keys will sit at their QWERTY positions."),
			*Source->GetName());
		return Source;
	}

	int32 NumRetargeted = 0;
	if (!FSiegeKeyboardLayoutStatics::RetargetContextKeys(Source, Duplicate, TranslationMap, NumRetargeted))
	{
		// ⛔ THE DUPLICATE IS DISCARDED, NEVER CACHED AND NEVER HANDED OUT (`KBD-§5`, row 4).
		// It is not even partially written: every one of RetargetContextKeys' refusals
		// (null argument, profile overrides present, mapping-array length mismatch) is
		// checked BEFORE its first write. Unreferenced, it is collected on the next GC.
		UE_LOG(LogSiegeInputLayout, Error,
			TEXT("[SiegeInputLayout] RetargetContextKeys REFUSED '%s' (profile overrides present, or the duplicate's mapping array differs in length); discarding the duplicate and handing back the untranslated source context."),
			*Source->GetName());
		return Source;
	}

	PositionalContexts.Add(Source, Duplicate);

	const FString ActiveTranslationDescription = DescribeActiveTranslation();
	UE_LOG(LogSiegeInputLayout, Log,
		TEXT("[SiegeInputLayout] Built the positional duplicate '%s' of '%s' — %d of %d mapping(s) retargeted. Translation: [%s]"),
		*Duplicate->GetName(), *Source->GetName(), NumRetargeted, Source->GetMappings().Num(), *ActiveTranslationDescription);

	return Duplicate;
}

bool USiegeKeyboardLayoutSubsystem::IsPositionalRemapActive() const
{
	// ⛔ PINNED MEANING (`KBD-§8`): "the host layout is not positionally QWERTY". NOT "a
	// duplicate exists" — the two diverge on the very first call, because the map is derived
	// at Initialize and the first duplicate is only built when a caller asks for one.
	return TranslationMap.Num() > 0;
}

FString USiegeKeyboardLayoutSubsystem::DescribeActiveTranslation() const
{
	// ⛔ THE EMPTY STRING, not "none" and not "(identity)" (`KBD-§8`).
	if (TranslationMap.Num() == 0)
	{
		return FString();
	}

	TArray<FString> Entries;
	Entries.Reserve(TranslationMap.Num());

	// ⛔ SOURCE-KEY ORDER AS RETURNED BY GetQwertyLetterScanCodes(), i.e. A..Z — pinned, and
	// load-bearing. Iterating TranslationMap directly would emit TMap's internal order, which
	// is unspecified and would make a byte-for-byte assertion on this string flaky.
	// TASK-509's table is authored alphabetically for exactly this reason
	// (SiegeKeyboardLayoutStatics.cpp:48-51).
	//
	// Hoisted into a named local rather than iterated as a temporary: lifetime extension in a
	// range-based for would make it correct either way, but this is a diagnostics-only
	// function and a reader should not have to prove that.
	const TArray<FSiegePositionalKeyProbe> OrderedProbes = FSiegeKeyboardLayoutStatics::GetQwertyLetterScanCodes();

	for (const FSiegePositionalKeyProbe& Probe : OrderedProbes)
	{
		const FKey* TranslatedKey = TranslationMap.Find(Probe.QwertyKey);
		if (!TranslatedKey)
		{
			continue;
		}

		// Identities are omitted. BuildTranslationMap never emits one, but
		// SetTranslationMapForAutomationTests can legally inject one, so the filter is here
		// rather than assumed.
		if (*TranslatedKey == Probe.QwertyKey)
		{
			continue;
		}

		// ⛔ THE SEPARATOR IS " → " — SPACE, U+2192 RIGHTWARDS ARROW, SPACE — and the
		// entry separator is ", ", both pinned in `KBD-§8`. The literal below is that exact
		// codepoint. ⚠️ TASK-510 asserts this string with TestEqualSensitive (`SC-§13`), so
		// its expected value must carry the SAME codepoint: an ASCII "->" will not match.
		// (UBT passes /utf-8 unconditionally — VCToolChain.cs:708 — so a UTF-8 literal in a
		// BOM-less source file is correct here; SiegeSettingsSubsystem.cpp:32 already ships
		// one.)
		//
		// ⚠️ AND DO NOT COPY `KBD-§8`'s WORKED EXAMPLE "W → Comma, S → O, D → E" AS AN
		// EXPECTED VALUE: it illustrates the separators and is NOT in A..Z order. On
		// US-Dvorak this emits "A → Q, C → J, D → E, ..." — FKey::ToString() returns the key
		// NAME (InputCoreTypes.cpp:1310-1313), so the target of the QWERTY-W position reads
		// "Comma", not ",".
		Entries.Add(FString::Printf(TEXT("%s → %s"), *Probe.QwertyKey.ToString(), *TranslatedKey->ToString()));
	}

	return FString::Join(Entries, TEXT(", "));
}

void USiegeKeyboardLayoutSubsystem::RefreshKeyboardLayout()
{
	// ⛔ THE AUTOMATION LATCH (see SetTranslationMapForAutomationTests). FALSE ON EVERY
	// SHIPPED PATH. Without it, a test that injected a translation would have it silently
	// destroyed by the very next GetPositionalContext call on a QWERTY host.
	if (bTranslationOverriddenForTests)
	{
		return;
	}

	const uint64 CurrentLayoutHandle = ReadActiveLayoutHandle();

	// ─── ⭐ THE IDEMPOTENCE GUARD — THIS IS WHAT MAKES THE FUNCTION SAFE TO CALL EVERY FRAME.
	// One thread-local read and out. 0 == 0 also covers two states correctly and on purpose:
	// "not probed yet on a platform with no probe" (non-Windows, permanently pass-through)
	// and "GetKeyboardLayout failed" (fail-safe to pass-through). On Windows the real HKL is
	// never 0, so the FIRST call always falls through to a full probe.
	//
	// ⚠️ Yes, PollForLayoutChange makes the same comparison before calling this. That is
	// deliberate, not a leftover: the poll's guard is what keeps the 1 Hz path to a single
	// Win32 call with no function-call ceremony, and THIS guard is what makes the PUBLIC,
	// Blueprint-callable function idempotent for every other caller.
	if (CurrentLayoutHandle == CachedLayoutHandle)
	{
		return;
	}

	TArray<FSiegePositionalKeyProbe> Probes;
	uint64 ProbedLayoutHandle = 0;
	const int32 NumAnswered = ProbeActiveLayout(Probes, ProbedLayoutHandle);

	// ⛔ PASS THE FUNCTION, NOT A POINTER TO IT: TFunctionRef deliberately does not coerce a
	// function type to a pointer (Templates/Function.h:571), so there is no `&` here
	// (SiegeKeyboardLayoutStatics.h's own note to callers).
	TMap<FKey, FKey> NewTranslation;
	FSiegeKeyboardLayoutStatics::BuildTranslationMap(
		Probes, FSiegeKeyboardLayoutStatics::ResolveKeyFromCodes, NewTranslation);

	// Record the handle we actually read the table from, whatever the outcome — otherwise a
	// layout whose probe yields nothing would be re-probed on every single call.
	CachedLayoutHandle = ProbedLayoutHandle;

	// ⛔ NEVER BROADCAST ON A NO-OP (the delegate law, and `KBD-§8`'s no-op clause). The HKL
	// moving is NOT the event consumers care about — US-QWERTY -> UK-QWERTY changes the
	// handle and changes nothing positional. The event is the TRANSLATION changing.
	if (NewTranslation.OrderIndependentCompareEqual(TranslationMap))
	{
		UE_LOG(LogSiegeInputLayout, Verbose,
			TEXT("[SiegeInputLayout] Layout handle is now %llu (%d/%d positions answered) but the positional translation is unchanged — nothing re-targeted, nothing broadcast."),
			CachedLayoutHandle, NumAnswered, Probes.Num());
		return;
	}

	const FString PreviousDescription = DescribeActiveTranslation();

	TranslationMap = MoveTemp(NewTranslation);

	// ─── ⭐ RE-APPLY WITHOUT TOUCHING THE HERO (`KBD-§6`). ───
	// ⛔ THE CACHED DUPLICATE IS RE-TARGETED **IN PLACE**, from its own PRISTINE SOURCE, so
	// the pointer AHeroCharacter handed to AddMappingContext NEVER CHANGES. That single
	// property is why TASK-512's edit is four lines with zero new state and zero
	// re-application code — and a "refactor" that re-applies contexts from here throws it
	// away. ⛔ No pointer is swapped, no context is added or removed, nothing is unmapped.
	//
	// ⚠️ Re-deriving from Source is also what makes this non-compounding: RetargetContextKeys
	// never reads the target back, so applying a second translation to an already-translated
	// duplicate yields the same answer as applying it to a fresh copy (D → E and E → Period
	// coexist without cascading).
	//
	// ⚠️ AND WHEN THE MAP IS NOW EMPTY (a switch BACK to QWERTY) THIS STILL RUNS AND IS
	// EXACTLY RIGHT: an empty translation retargets every key to its source value, so the
	// duplicate the hero is holding becomes a faithful copy of IMC_Hero again. Dropping the
	// cache instead would leave the hero holding a stale Dvorak context.
	for (const TPair<TObjectPtr<const UInputMappingContext>, TObjectPtr<UInputMappingContext>>& CachedPair : PositionalContexts)
	{
		const UInputMappingContext* PristineSource = CachedPair.Key;
		UInputMappingContext* Duplicate = CachedPair.Value;

		if (!PristineSource || !Duplicate)
		{
			continue;
		}

		int32 NumRetargeted = 0;
		if (!FSiegeKeyboardLayoutStatics::RetargetContextKeys(PristineSource, Duplicate, TranslationMap, NumRetargeted))
		{
			// ⚠️ THE ONE RESIDUAL IN THIS FEATURE, STATED RATHER THAN HIDDEN: a refusal here
			// leaves the duplicate on the PREVIOUS layout's keys, which is worse than
			// "behaves as yesterday". It is also close to unreachable — every refusal
			// condition is a property of Source, and Source has not changed since the
			// duplicate was built successfully. Nothing better is available: we may not hand
			// the hero a different pointer, and a partial write is forbidden outright.
			UE_LOG(LogSiegeInputLayout, Error,
				TEXT("[SiegeInputLayout] RetargetContextKeys REFUSED the cached duplicate '%s' of '%s' after a layout change; it keeps the previous layout's keys."),
				*Duplicate->GetName(), *PristineSource->GetName());
			continue;
		}

		// ⛔ THE SUPPORTED REBUILD CALL IS RequestRebuildControlMappingsUsing**Context**
		// (EnhancedInputLibrary.h:36-37). ⚠️ THE ENGINE'S OWN DOC COMMENT AT
		// InputMappingContext.h:217 NAMES A STALE SYMBOL, `...ForContext`, WHICH DOES NOT
		// EXIST IN 5.8 — do not grep for it and do not conclude the API was removed and reach
		// for MapKey, which is the banned path (`KBD-§2`).
		UEnhancedInputLibrary::RequestRebuildControlMappingsUsingContext(Duplicate);
	}

	const FString CurrentDescription = DescribeActiveTranslation();

	UE_LOG(LogSiegeInputLayout, Log,
		TEXT("[SiegeInputLayout] Keyboard layout changed (handle %llu, %d/%d positions answered): %d translated letter position(s). Was: [%s]. Now: [%s]. %d cached context(s) re-targeted in place."),
		CachedLayoutHandle, NumAnswered, Probes.Num(), TranslationMap.Num(),
		*PreviousDescription, *CurrentDescription, PositionalContexts.Num());

	// ⛔ LAST, AFTER the re-target and after the log — a consumer that reads
	// DescribeActiveTranslation() from this callback must see the NEW state, and a consumer
	// that touches a cached context must find it already correct.
	OnKeyboardLayoutChanged.Broadcast();
}

FKey USiegeKeyboardLayoutSubsystem::GetPositionalKey(const FKey& QwertyKey) const
{
	// ⛔ const, AND IT DOES NOT RE-PROBE. RefreshKeyboardLayout() is what makes the map
	// current; this reads it as it stands (`KBD-§8`: the caller refreshes, the accessor
	// reads). No mutable member, no const_cast, no timer.
	const FKey* TranslatedKey = TranslationMap.Find(QwertyKey);

	// ⛔ NEVER EKeys::Invalid (`KBD-§5` applied to a scalar). Absent means IDENTITY — identity
	// is never stored, so "no entry" and "maps to itself" are the same answer and both return
	// the input unchanged; an extra equality branch here would be dead code.
	//
	// ⛔ AND DO NOT "SIMPLIFY" THIS TO TranslationMap.FindRef(QwertyKey): FindRef returns a
	// DEFAULT-CONSTRUCTED FKey on a miss, whose KeyName is left NAME_None
	// (InputCoreTypes.h:53-55) — and EKeys::Invalid IS FKey(NAME_None)
	// (InputCoreTypes.cpp:414), compared by KeyName alone (InputCoreTypes.h:109). So FindRef
	// returns EXACTLY the one value this function is forbidden to return, on the single most
	// common path: a QWERTY host, where the map is EMPTY and every lookup misses.
	// The Find-then-fallback idiom is the same one DescribeActiveTranslation uses above.
	return TranslatedKey ? *TranslatedKey : QwertyKey;
}

void USiegeKeyboardLayoutSubsystem::SetTranslationMapForAutomationTests(const TMap<FKey, FKey>& InTranslation)
{
	// ⛔⛔ AUTOMATION TESTS ONLY — NOTHING IN THE GAME CALLS THIS. The latch is what makes the
	//     seam work at all: GetPositionalContext re-probes on every call, so without it the
	//     next call on a QWERTY host would rebuild an EMPTY translation and destroy whatever
	//     the test just injected. See the header for the full contract.
	bTranslationOverriddenForTests = true;

	TranslationMap = InTranslation;

	// A duplicate built under the previous map carries the previous map's keys, so the cache
	// is dropped rather than re-targeted: a test is forcing a STATE, not observing a change.
	PositionalContexts.Reset();

	// ⛔ No broadcast: this is a test seam, not a detected layout change.
	UE_LOG(LogSiegeInputLayout, Warning,
		TEXT("[SiegeInputLayout] ⛔ AUTOMATION OVERRIDE: the translation was set by SetTranslationMapForAutomationTests (%d entr(y/ies)) and this instance will no longer probe the OS. This line must never appear in a shipped session."),
		TranslationMap.Num());
}

int32 USiegeKeyboardLayoutSubsystem::ProbeActiveLayout(TArray<FSiegePositionalKeyProbe>& OutProbes, uint64& OutLayoutHandle) const
{
	// The 26-row A..Z table, then the platform fills VirtualKey/CharCode in place. The split
	// is the whole architecture: the table and every rule about it are pure and testable in
	// SiegeKeyboardLayoutStatics; only the two MapVirtualKeyEx calls are platform.
	OutProbes = FSiegeKeyboardLayoutStatics::GetQwertyLetterScanCodes();
	OutLayoutHandle = 0;

	return SiegeKeyboardLayoutPlatform::ProbeActiveKeyboardLayout(OutLayoutHandle, &OutProbes);
}

uint64 USiegeKeyboardLayoutSubsystem::ReadActiveLayoutHandle() const
{
	// nullptr for the probe array == "the handle and nothing else". One thread-local read;
	// no allocation and no table walk. This is what the 1 Hz poll and the per-call re-probe
	// actually cost when the layout has not moved.
	uint64 LayoutHandle = 0;
	SiegeKeyboardLayoutPlatform::ProbeActiveKeyboardLayout(LayoutHandle, nullptr);
	return LayoutHandle;
}

void USiegeKeyboardLayoutSubsystem::HandleApplicationActivationChanged(bool bIsActive)
{
#if PLATFORM_WINDOWS

	// On the way BACK IN only. The layout that matters is the one active while the game has
	// focus, and a change made while we were away is precisely what this hook exists to
	// catch; re-probing on the way out would only describe a keyboard nobody is using.
	if (!bIsActive)
	{
		return;
	}

	UE_LOG(LogSiegeInputLayout, Verbose,
		TEXT("[SiegeInputLayout] Application re-activated — re-checking the keyboard layout."));

	RefreshKeyboardLayout();

#endif // PLATFORM_WINDOWS
}

void USiegeKeyboardLayoutSubsystem::PollForLayoutChange()
{
#if PLATFORM_WINDOWS

	// Read LIVE so `siege.Input.LayoutPollEnabled 0` stops the poll at runtime and `1`
	// restarts it. The timer keeps ticking either way — a lever that can only be thrown
	// before startup is not a runtime test lever.
	if (SiegeKeyboardLayoutCVars::CVarLayoutPollEnabled.GetValueOnGameThread() == 0)
	{
		return;
	}

	// ⭐ THE COMPARISON THE DESIGN ASKS FOR, AND THE POLL'S ENTIRE COST WHEN NOTHING CHANGED:
	// GetKeyboardLayout(0) != CachedLayoutHandle. No allocation, no MapVirtualKeyEx, no log.
	if (ReadActiveLayoutHandle() == CachedLayoutHandle)
	{
		return;
	}

	// ⭐ THIS IS THE ONLY MECHANISM THAT SEES AN IN-PLACE Win+Space: no window focus changes,
	// no context is re-applied, and there is no engine delegate for WM_INPUTLANGCHANGE.
	UE_LOG(LogSiegeInputLayout, Log,
		TEXT("[SiegeInputLayout] The active keyboard layout handle moved (was %llu) — re-deriving the positional translation."),
		CachedLayoutHandle);

	RefreshKeyboardLayout();

#endif // PLATFORM_WINDOWS
}
