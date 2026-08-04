// Copyright Epic Games, Inc. All Rights Reserved.

#include "Siegebound/SiegeKeyboardLayoutStatics.h"

// Complete-type include law (TASK-110): this .cpp calls methods on UInputMappingContext and
// writes a member of FEnhancedActionKeyMapping, so BOTH complete types are included here
// rather than inherited transitively. InputCoreTypes.h is re-stated for FInputKeyManager /
// EKeys for the same reason — the header's include of it is an implementation detail of the
// header, not a promise to this translation unit.
#include "EnhancedActionKeyMapping.h"
#include "InputCoreTypes.h"
#include "InputMappingContext.h"

// ⛔ NO Win32 HEADER, NO `#if PLATFORM_WINDOWS`, NO Windows/WindowsHWrapper.h IN THIS FILE.
// FSiegeKeyboardLayoutStatics is fully platform-agnostic and its tests run everywhere; the
// GetKeyboardLayout/MapVirtualKeyEx probe that fills FSiegePositionalKeyProbe::VirtualKey and
// ::CharCode lives in USiegeKeyboardLayoutSubsystem's .cpp (TASK-511), fenced there
// (CONVENTIONS `KBD-§6`). A Win32 symbol appearing in this file is a finding.

TArray<FSiegePositionalKeyProbe> FSiegeKeyboardLayoutStatics::GetQwertyLetterScanCodes()
{
	// A REFERENCE member on purpose: the rows only take the addresses of InputCore's EKeys
	// statics, so the table below constructs and copies no FKey at all. As a function-local
	// static it is also built on first call — i.e. long after InputCore's own statics exist —
	// so there is no cross-module static-initialisation-order question to answer.
	struct FSiegeQwertyLetterRow
	{
		const FKey& QwertyKey;
		uint32      ScanCode;
	};

	// ── THE HAND-AUTHORED TABLE ──────────────────────────────────────────────────────────
	// Scan-code SET 1 (the "XT set"): the codes Windows reports in the WM_KEYDOWN lParam and
	// the ones MapVirtualKeyEx(code, MAPVK_VSC_TO_VK_EX, hkl) consumes. Scan codes are a
	// property of the PHYSICAL keyboard, not of the layout — that invariance is the entire
	// mechanism this feature stands on.
	//
	// ⚠️ NO SCAN-CODE TABLE EXISTS ANYWHERE IN ENGINE SOURCE TO CROSS-CHECK THIS AGAINST
	// (CONVENTIONS `KBD-§6`), which is why the subsystem logs a self-check at Initialize:
	// on a US-QWERTY host MapVirtualKeyEx(0x11, MAPVK_VSC_TO_VK_EX, hkl) must return 'W'
	// (0x57). That one assertion validates this table against the OS.
	//
	// The three physical rows, so the table can be read against a keyboard:
	//   Q W E R T Y U I O P  =  0x10 0x11 0x12 0x13 0x14 0x15 0x16 0x17 0x18 0x19
	//   A S D F G H J K L    =  0x1E 0x1F 0x20 0x21 0x22 0x23 0x24 0x25 0x26
	//   Z X C V B N M        =  0x2C 0x2D 0x2E 0x2F 0x30 0x31 0x32
	//
	// ⚠️ STORED ALPHABETICALLY, A..Z — NOT in the physical-row order above, and that is
	// load-bearing: DescribeActiveTranslation is pinned to emit entries in "source-key order
	// as returned by GetQwertyLetterScanCodes()" (`KBD-§8`), so reordering these rows changes
	// a string TASK-510 asserts byte-for-byte.
	//
	// ⛔ ALL 26 (`KBD-§4`) — not only the ten the game binds today. Free, and it removes the
	// "update the table when you add a key" footgun.
	static const FSiegeQwertyLetterRow LetterRows[] =
	{
		{ EKeys::A, 0x1E }, { EKeys::B, 0x30 }, { EKeys::C, 0x2E }, { EKeys::D, 0x20 },
		{ EKeys::E, 0x12 }, { EKeys::F, 0x21 }, { EKeys::G, 0x22 }, { EKeys::H, 0x23 },
		{ EKeys::I, 0x17 }, { EKeys::J, 0x24 }, { EKeys::K, 0x25 }, { EKeys::L, 0x26 },
		{ EKeys::M, 0x32 }, { EKeys::N, 0x31 }, { EKeys::O, 0x18 }, { EKeys::P, 0x19 },
		{ EKeys::Q, 0x10 }, { EKeys::R, 0x13 }, { EKeys::S, 0x1F }, { EKeys::T, 0x14 },
		{ EKeys::U, 0x16 }, { EKeys::V, 0x2F }, { EKeys::W, 0x11 }, { EKeys::X, 0x2D },
		{ EKeys::Y, 0x15 }, { EKeys::Z, 0x2C }
	};

	TArray<FSiegePositionalKeyProbe> Probes;
	Probes.Reserve(UE_ARRAY_COUNT(LetterRows));

	for (const FSiegeQwertyLetterRow& Row : LetterRows)
	{
		FSiegePositionalKeyProbe& Probe = Probes.AddDefaulted_GetRef();
		Probe.QwertyKey = Row.QwertyKey;
		Probe.ScanCode  = Row.ScanCode;

		// VirtualKey and CharCode stay 0 on purpose. This function knows nothing about the
		// active layout; the platform probe fills them, and a position it cannot answer is
		// LEFT at 0, which BuildTranslationMap skips.
	}

	return Probes;
}

FKey FSiegeKeyboardLayoutStatics::ResolveKeyFromCodes(uint32 VirtualKey, uint32 CharCode)
{
	// ⭐ The engine's OWN resolver, called with the SAME two numbers the Windows message pump
	// hands FSlateApplication::OnKeyDown (WindowsApplication.cpp:3317/3321 ->
	// SlateApplication.cpp:4953-4957). That is what makes this table agree with runtime BY
	// CONSTRUCTION rather than by a second hand-written source of truth that drifts.
	//
	// ⛔ CharCode IS PASSED THROUGH UNMASKED — DO NOT "CLEAN UP" THE DEAD-KEY BIT 0x80000000.
	// WindowsApplication.cpp:3317 does not mask it either, and the table must match runtime
	// byte for byte. Masking it here would produce a translation that looks tidier and
	// disagrees with the key the player actually presses (CONVENTIONS `KBD-§6`).
	//
	// Returns EKeys::Invalid when the pair maps to nothing (InputCoreTypes.cpp:1611); it may
	// also SYNTHESIZE and register a new FKey for an unknown printable character
	// (InputCoreTypes.cpp:1604-1610) — that path is deliberate engine behaviour for non-QWERTY
	// keyboards and is passed through, not filtered.
	return FInputKeyManager::Get().GetKeyFromCodes(VirtualKey, CharCode);
}

int32 FSiegeKeyboardLayoutStatics::BuildTranslationMap(const TArray<FSiegePositionalKeyProbe>& Probes,
                                                      FSiegeKeyResolver Resolver,
                                                      TMap<FKey, FKey>& OutTranslation)
{
	// Fully replaced, never merged — a caller re-probing after a Win+Space must not inherit
	// entries from the layout it just left.
	OutTranslation.Reset();

	// Every FKey some position will END UP holding: the target of an accepted translation,
	// or the QWERTY key of a position that keeps it. ⭐ An identity probe CLAIMS ITS OWN KEY —
	// it is still a claim, and leaving it out would let a later position be translated onto a
	// key another action already answers to, firing two actions from one physical press.
	TSet<FKey> ClaimedKeys;
	ClaimedKeys.Reserve(Probes.Num());

	for (const FSiegePositionalKeyProbe& Probe : Probes)
	{
		// (1) A malformed table row. GetQwertyLetterScanCodes never produces one; a fake probe
		//     array in a test might. Checked FIRST so an invalid FKey can never enter
		//     ClaimedKeys and sit there as a phantom claim.
		if (!Probe.QwertyKey.IsValid())
		{
			continue;
		}

		// (2) The platform probe failed for this position (or never ran — non-Windows).
		//     0 is never a real virtual key, so it is an unambiguous "no answer".
		if (Probe.VirtualKey == 0)
		{
			ClaimedKeys.Add(Probe.QwertyKey);
			continue;
		}

		// (3) Duplicated source row — first occurrence wins, so the result can never depend
		//     on which of two identical rows the loop happened to reach first.
		if (OutTranslation.Contains(Probe.QwertyKey))
		{
			continue;
		}

		const FKey ResolvedKey = Resolver(Probe.VirtualKey, Probe.CharCode);

		// (4) ⛔ The resolver could not name this pair. DROP IT — the letter keeps its source
		//     key and the map simply has no entry. Nothing this feature produces may ever be
		//     EKeys::Invalid (`KBD-§5`).
		if (ResolvedKey == EKeys::Invalid || !ResolvedKey.IsValid())
		{
			ClaimedKeys.Add(Probe.QwertyKey);
			continue;
		}

		// (5) Identity: this physical position already yields its QWERTY key, so there is
		//     nothing to translate. No entry — but the key IS claimed (see above).
		if (ResolvedKey == Probe.QwertyKey)
		{
			ClaimedKeys.Add(Probe.QwertyKey);
			continue;
		}

		// (6) ⛔ INJECTIVITY GUARD. Two actions must never collide onto one physical key, so
		//     a target another position already holds is refused outright. First probe in
		//     A..Z order wins; this letter keeps its source key, and now claims it.
		if (ClaimedKeys.Contains(ResolvedKey))
		{
			ClaimedKeys.Add(Probe.QwertyKey);
			continue;
		}

		// Accepted. The position VACATES its QWERTY key (a later position may legitimately be
		// translated onto it — that is exactly how a two-key swap survives) and CLAIMS its
		// new one.
		ClaimedKeys.Add(ResolvedKey);
		OutTranslation.Add(Probe.QwertyKey, ResolvedKey);
	}

	// Identity entries are never added, so this IS the count of non-identity entries.
	// ⭐ 0 means the host layout is positionally QWERTY: the caller allocates nothing and
	// hands the source context straight back.
	return OutTranslation.Num();
}

bool FSiegeKeyboardLayoutStatics::RetargetContextKeys(const UInputMappingContext* Source,
                                                     UInputMappingContext* Target,
                                                     const TMap<FKey, FKey>& Translation,
                                                     int32& OutNumRetargeted)
{
	// Set before every early return, so a caller that ignores the bool still reads 0.
	OutNumRetargeted = 0;

	if (!Source || !Target)
	{
		return false;
	}

	// ⛔ REFUSE WHOLESALE ON PROFILE OVERRIDES. They live in MappingProfileOverrides
	// (InputMappingContext.h:109-110), a SEPARATE array that GetMapping() cannot reach — so
	// retargeting would cover the defaults, silently miss the overrides, and hand out a
	// half-translated context. Refusing loudly beats that (`KBD-§5`). Checking Source alone
	// is sufficient because Target is, by this function's contract, a DuplicateObject of it.
	if (Source->GetProfilesWithOverridenMappings().Num() > 0)
	{
		return false;
	}

	const TArray<FEnhancedActionKeyMapping>& SourceMappings = Source->GetMappings();

	// A length mismatch means Target is not the duplicate this contract assumes, so an
	// index-by-index derivation would be meaningless (and out of bounds).
	if (SourceMappings.Num() != Target->GetMappings().Num())
	{
		return false;
	}

	// ⛔⛔ THE LOOP CONVENTIONS `KBD-§1` IS ABOUT. Read both clauses before editing it:
	//
	//  (a) EVERY value is re-derived from SOURCE BY INDEX, and Target is never read back.
	//      That makes the substitution SIMULTANEOUS: on Dvorak the map holds `D -> E` AND
	//      `E -> Period` at the same time, and reading the target instead would cascade
	//      `D -> E -> Period`. It is also what makes a repeat call idempotent, which is what
	//      lets the subsystem re-target its CACHED duplicate in place on a mid-session layout
	//      change without ever handing AHeroCharacter a new pointer.
	//
	//  (b) ⛔ ONLY `.Key` IS ASSIGNED. NEVER Modifiers, NEVER Triggers, NEVER Action, NEVER
	//      PlayerMappableKeySettings, and NEVER the mappings ARRAY (no MapKey / UnmapKey /
	//      UnmapAll / Add / RemoveAt / Empty). handoffs/TASK-445-artist.md:95-105 — rewriting
	//      the array silently default-constructed the instanced SwizzleAxis/Negate modifiers,
	//      breaking WASD and inverting mouse-look WHILE THE PROPERTY TABLE STILL READ CORRECT.
	//      The failure mode is absent by CONSTRUCTION here, not dodged by care.
	for (int32 Index = 0; Index < SourceMappings.Num(); ++Index)
	{
		const FKey& SourceKey = SourceMappings[Index].Key;
		const FKey* TranslatedKey = Translation.Find(SourceKey);

		// Fail-safe: an untranslated key keeps its SOURCE value. It never becomes
		// EKeys::Invalid and it is never left holding a previous call's result.
		Target->GetMapping(Index).Key = TranslatedKey ? *TranslatedKey : SourceKey;

		// Counts MAPPINGS whose key actually moved, not distinct keys — one physical key
		// bound to two actions counts twice. The `!=` matters because a hand-injected map
		// (SetTranslationMapForAutomationTests) may legally contain an identity entry, which
		// BuildTranslationMap itself never emits.
		if (TranslatedKey && *TranslatedKey != SourceKey)
		{
			++OutNumRetargeted;
		}
	}

	return true;
}
