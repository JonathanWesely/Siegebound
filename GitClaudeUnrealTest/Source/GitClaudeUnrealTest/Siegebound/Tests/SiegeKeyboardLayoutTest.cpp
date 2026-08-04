// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "EnhancedActionKeyMapping.h"
#include "Engine/GameInstance.h"
#include "InputAction.h"
#include "InputCoreTypes.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "InputTriggers.h"
#include "Siegebound/SiegeKeyboardLayoutStatics.h"
#include "Siegebound/SiegeKeyboardLayoutSubsystem.h"
#include "Templates/Casts.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UObjectGlobals.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 *  ═══ AUTOMATION TESTS for the positional keyboard-layout remap ═══
 *  (KEYBOARD-LAYOUT batch, TASK-510; subject = TASK-509's FSiegeKeyboardLayoutStatics and
 *  TASK-511's USiegeKeyboardLayoutSubsystem. QA gate: TASK-513.)
 *
 *  ⭐⭐ THE PROPERTY THAT MAKES THIS FILE POSSIBLE, AND THE FIRST THING TO CHECK IF ONE OF
 *      THESE EVER GOES RED: EVERY TEST BELOW PASSES ON A QWERTY MACHINE, WITH NO DVORAK
 *      HARDWARE ANYWHERE.
 *
 *  The seam is `FSiegeKeyResolver` (SiegeKeyboardLayoutStatics.h) — a TFunctionRef the tests
 *  fill with a FAKE resolver. BuildTranslationMap reads no engine state at all: the probe
 *  array is data and the resolver is injected, so a "Dvorak" result is produced from
 *  Dvorak-shaped DATA rather than from a Dvorak-shaped MACHINE. ⛔ If any test here starts
 *  needing the host layout to be anything in particular, the seam has been broken and that is
 *  a finding, not a workaround.
 *
 *  ⚠️ EXACTLY TWO PLACES IN THIS FILE READ THE REAL MACHINE, AND BOTH LOG RATHER THAN FAIL:
 *    • Siegebound.Input.LiveResolverMatchesRuntime — the whole test (test 7);
 *    • Siegebound.Input.QwertyIsPassThrough — the ONE subsystem assertion, because
 *      USiegeKeyboardLayoutSubsystem is specified to re-probe the OS layout (TASK-511 spec
 *      (4a)), which can legitimately overwrite the empty map this test forces in.
 *  ⚖️ Both degrade to AddInfo for the SAME reason: a hard assert there would red-fail on
 *  Jonathan's own Dvorak machine — the exact machine this feature exists to serve — which is
 *  backwards. Everything else in this file is a hard assertion.
 *
 *  ⛔ WHAT THESE TESTS CANNOT PROVE, STATED SO NOBODY MISTAKES GREEN FOR DONE (`SC-§32`):
 *  nothing here presses a key, opens PIE, or asks Windows anything. The scancode table is
 *  validated against the OS only by the subsystem's `MapVirtualKeyEx(0x11, …) == 'W'`
 *  self-check at Initialize (`KBD-§6`), and the feature's real gate is TASK-515 —
 *  Jonathan's own Dvorak PIE check. A full green run here means the LOGIC is right.
 *
 *  M8 DECLARATION (verbatim, `KBD-§10`): adds no replicated property, no new replicated
 *  class, no new relevancy tier. This is a test file; it adds no shipped surface at all.
 */

namespace SiegeKeyboardLayoutTestUtils
{
	/**
	 *  A virtual key no layout produces, used by DegenerateProbesAreRefused to drive the
	 *  "the resolver could not name this pair" branch deliberately instead of hoping for it.
	 *  ⚠️ Namespace scope, not a function local: a non-capturing lambda reads it below, and a
	 *  local would raise a capture question for no benefit.
	 */
	static constexpr uint32 UnresolvableVirtualKey = 0xFFFF;

	// ════════════════════════════════════════════════════════════════════════════════════
	//  THE FAKE RESOLVER — a faithful model of how the ENGINE resolves a (VK, char) pair
	// ════════════════════════════════════════════════════════════════════════════════════

	/**
	 *  Models FInputKeyManager::GetKeyFromCodes for the printable keys this feature can touch.
	 *
	 *  ⭐ WHY IT IS CHARACTER-DRIVEN AND NOT VIRTUAL-KEY-DRIVEN — this is the load-bearing
	 *  fact of the whole file, verified against installed UE 5.8 source rather than assumed:
	 *
	 *    • `GetKeyFromCodes` looks in `KeyMapVirtualToEnum` FIRST, then falls back to
	 *      `KeyMapCharToEnum` (InputCoreTypes.cpp:1595-1611).
	 *    • On Windows `KeyMapVirtualToEnum` is built by `FWindowsPlatformInput::GetKeyMap`
	 *      (WindowsPlatformInput.cpp:6-121), which registers mouse buttons, control keys,
	 *      F-keys and the numpad — ⛔ AND NO LETTER VKs AT ALL. It then collects the OEM VKs,
	 *      and REMOVES every one whose produced character already appears in the printable
	 *      char map (`:105-110`). `,` `.` `'` `;` are all in that map, so
	 *      ⛔ VK_OEM_COMMA / VK_OEM_PERIOD / VK_OEM_7 / VK_OEM_1 ARE NOT IN THE VK MAP EITHER.
	 *    • `KeyMapCharToEnum` comes from `FGenericPlatformInput::GetStandardPrintableKeyMap`
	 *      (GenericPlatformInput.cpp:5-95) — 'A'..'Z' (UPPERCASE only on Windows) plus the
	 *      punctuation list below. It is a STATIC table: ⭐ layout-independent, which is
	 *      precisely why this model can be trusted to behave the same on any host.
	 *
	 *  ⇒ For every key this feature is in scope for (`KBD-§4`, letters only), the engine
	 *  resolves BY CHARACTER. This function reproduces that rule exactly.
	 *
	 *  ⚠️ CONSEQUENCE FOR THE DVORAK FIXTURE, AND IT IS THE REASON THE "derived, not observed"
	 *  CAVEAT BELOW IS SURVIVABLE: the VirtualKey field of a probe influences
	 *  BuildTranslationMap ONLY through the `VirtualKey == 0` skip. So if TASK-515's live
	 *  probe reports different VK numbers than the ones this file guesses, ⭐ NOT ONE
	 *  ASSERTION IN DvorakTranslation CHANGES — they are all driven by the CharCodes, which
	 *  are the Dvorak characters themselves and are not in doubt.
	 *
	 *  `FKey(TEXT("W"))` IS `EKeys::W`: FKey compares by KeyName (InputCoreTypes.h:109) and
	 *  `EKeys::W` is constructed as `FKey("W")` (InputCoreTypes.cpp:88). This is the same
	 *  construction `FInputKeyManager::InitKeyMappings` uses at `:1581`.
	 */
	static FKey ResolveLikePlatform(uint32 VirtualKey, uint32 CharCode)
	{
		// BuildTranslationMap is contracted never to call a resolver for a failed probe, so
		// this branch should be unreachable through it. Kept because a resolver that answered
		// confidently for VK 0 would hide exactly that contract being broken.
		if (VirtualKey == 0)
		{
			return EKeys::Invalid;
		}

		if (CharCode >= static_cast<uint32>(TEXT('A')) && CharCode <= static_cast<uint32>(TEXT('Z')))
		{
			return FKey(*FString::Chr(static_cast<TCHAR>(CharCode)));
		}

		// The punctuation a 26-letter-position probe can produce on the Latin layouts this
		// feature cares about: Dvorak yields , . ' ; — AZERTY and QWERTZ add nothing outside
		// this set. Anything else is deliberately EKeys::Invalid, which BuildTranslationMap
		// skips, leaving that letter on its source key (`KBD-§5`).
		switch (static_cast<TCHAR>(CharCode))
		{
		case TEXT(','):  return EKeys::Comma;
		case TEXT('.'):  return EKeys::Period;
		case TEXT('\''): return EKeys::Apostrophe;
		case TEXT(';'):  return EKeys::Semicolon;
		case TEXT('/'):  return EKeys::Slash;
		case TEXT('-'):  return EKeys::Hyphen;
		case TEXT('='):  return EKeys::Equals;
		default:         break;
		}

		return EKeys::Invalid;
	}

	/** 'A'..'Z' for EKeys::A..EKeys::Z, 0 for anything else. EKeys::W is FKey("W"), so the
	 *  key's own name IS its uppercase character — no second lookup table to drift. */
	static uint32 UppercaseCharForLetterKey(const FKey& Key)
	{
		const FString KeyName = Key.ToString();
		if (KeyName.Len() == 1)
		{
			const TCHAR Char = KeyName[0];
			if (Char >= TEXT('A') && Char <= TEXT('Z'))
			{
				return static_cast<uint32>(Char);
			}
		}
		return 0;
	}

	// ════════════════════════════════════════════════════════════════════════════════════
	//  THE US-DVORAK FIXTURE
	// ════════════════════════════════════════════════════════════════════════════════════

	/**
	 *  ⚠️⚠️ PROVENANCE OF THE NUMBERS BELOW — READ BEFORE "CORRECTING" THEM.
	 *
	 *  ⛔ THE `VirtualKey` COLUMN WAS DERIVED, NOT OBSERVED. Nobody in this pipeline has a
	 *  Dvorak machine; the VKs are what the US-Dvorak layout (kbddv.dll) is understood to
	 *  assign to each scan code, reasoned from the character each position produces.
	 *  ⭐ **TASK-515 — Jonathan's own PIE gate on his Dvorak machine — PRINTS THE LIVE PROBE
	 *  AND IS THE AUTHORITATIVE SOURCE. If it reports different VKs, THIS TABLE IS WRONG AND
	 *  TASK-515 WINS.**
	 *
	 *  ✅ AND THE REASON THAT IS SAFE RATHER THAN ALARMING: see ResolveLikePlatform above —
	 *  the engine resolves these keys BY CHARACTER, and BuildTranslationMap uses VirtualKey
	 *  only as a non-zero "the probe answered" gate. ⇒ A wrong VK here cannot change a single
	 *  assertion in DvorakTranslation. The `CharCode` column is the load-bearing one, and it
	 *  is simply the US-Dvorak character at each QWERTY position — not in doubt.
	 *
	 *  The layout, read as three physical rows (QWERTY label ⇒ Dvorak character):
	 *      Q W E R T Y U I O P   ⇒   ' , . P Y F G C R L
	 *      A S D F G H J K L     ⇒   A O E U I D H T N
	 *      Z X C V B N M         ⇒   ; Q J K X B M
	 *
	 *  ⭐ It is a BIJECTION over the 26 letter positions — 22 letters + 4 punctuation, all
	 *  distinct — so no probe is ever refused by the injectivity guard. TASK-509 hand-traced
	 *  the same 26 and reached the same 24 accepted / 2 identity (`A`, `M`) split
	 *  (handoffs/TASK-509-programmer.md §5.1). This fixture is the mechanised form of that
	 *  hand trace, which is the point: an independently-authored table that agrees.
	 *
	 *  `MapVirtualKeyEx(vk, MAPVK_VK_TO_CHAR, hkl)` yields the UPPERCASE character for letter
	 *  keys, which is why the CharCode column is uppercase (and why `GetStandardPrintableKeyMap`
	 *  is called with bMapUppercaseKeys=true on Windows — WindowsPlatformInput.cpp:124).
	 */
	struct FSiegeDvorakCell
	{
		const FKey& QwertyKey;   // the physical position, named as QWERTY names it
		uint32      VirtualKey;  // ⚠️ DERIVED — TASK-515 is authoritative
		uint32      CharCode;    // the US-Dvorak character at that position — the real claim
	};

	/**
	 *  The 26 letter positions with US-Dvorak's answer at each. Function-local static so the
	 *  FKey references resolve long after InputCore's own statics exist (the
	 *  SiegeKeyboardLayoutStatics.cpp:22-30 precedent — no cross-module static-init order
	 *  question, and no FKey is copied to build it).
	 */
	static const FSiegeDvorakCell* FindDvorakCell(const FKey& QwertyKey)
	{
		static const FSiegeDvorakCell DvorakCells[] =
		{
			{ EKeys::A, 0x41,   static_cast<uint32>(TEXT('A'))  },  // identity
			{ EKeys::B, 0x58,   static_cast<uint32>(TEXT('X'))  },
			{ EKeys::C, 0x4A,   static_cast<uint32>(TEXT('J'))  },  // ⭐ asserted
			{ EKeys::D, 0x45,   static_cast<uint32>(TEXT('E'))  },  // ⭐ asserted
			{ EKeys::E, 0xBE,   static_cast<uint32>(TEXT('.'))  },  // ⭐ asserted (VK_OEM_PERIOD)
			{ EKeys::F, 0x55,   static_cast<uint32>(TEXT('U'))  },  // ⭐ asserted
			{ EKeys::G, 0x49,   static_cast<uint32>(TEXT('I'))  },
			{ EKeys::H, 0x44,   static_cast<uint32>(TEXT('D'))  },
			{ EKeys::I, 0x43,   static_cast<uint32>(TEXT('C'))  },
			{ EKeys::J, 0x48,   static_cast<uint32>(TEXT('H'))  },
			{ EKeys::K, 0x54,   static_cast<uint32>(TEXT('T'))  },
			{ EKeys::L, 0x4E,   static_cast<uint32>(TEXT('N'))  },
			{ EKeys::M, 0x4D,   static_cast<uint32>(TEXT('M'))  },  // identity
			{ EKeys::N, 0x42,   static_cast<uint32>(TEXT('B'))  },
			{ EKeys::O, 0x52,   static_cast<uint32>(TEXT('R'))  },
			{ EKeys::P, 0x4C,   static_cast<uint32>(TEXT('L'))  },
			{ EKeys::Q, 0xDE,   static_cast<uint32>(TEXT('\'')) },  // ⭐ asserted (VK_OEM_7)
			{ EKeys::R, 0x50,   static_cast<uint32>(TEXT('P'))  },  // ⭐ asserted
			{ EKeys::S, 0x4F,   static_cast<uint32>(TEXT('O'))  },  // ⭐ asserted
			{ EKeys::T, 0x59,   static_cast<uint32>(TEXT('Y'))  },  // ⭐ asserted
			{ EKeys::U, 0x47,   static_cast<uint32>(TEXT('G'))  },
			{ EKeys::V, 0x4B,   static_cast<uint32>(TEXT('K'))  },
			{ EKeys::W, 0xBC,   static_cast<uint32>(TEXT(','))  },  // ⭐ asserted (VK_OEM_COMMA)
			{ EKeys::X, 0x51,   static_cast<uint32>(TEXT('Q'))  },
			{ EKeys::Y, 0x46,   static_cast<uint32>(TEXT('F'))  },
			{ EKeys::Z, 0xBA,   static_cast<uint32>(TEXT(';'))  }   // VK_OEM_1
		};

		for (const FSiegeDvorakCell& Cell : DvorakCells)
		{
			if (Cell.QwertyKey == QwertyKey)
			{
				return &Cell;
			}
		}
		return nullptr;
	}

	/**
	 *  The shipped scan-code table with the US-Dvorak probe results filled in — i.e. exactly
	 *  what TASK-511's Win32 probe would hand BuildTranslationMap on Jonathan's machine.
	 *
	 *  ⭐ Built ON TOP OF the real GetQwertyLetterScanCodes() rather than hand-rolled from
	 *  nothing, and looked up BY FKey rather than by index, so the fixture cannot silently
	 *  disagree with the shipped table about which letter sits at which scan code.
	 */
	static TArray<FSiegePositionalKeyProbe> MakeDvorakProbes()
	{
		TArray<FSiegePositionalKeyProbe> Probes = FSiegeKeyboardLayoutStatics::GetQwertyLetterScanCodes();
		for (FSiegePositionalKeyProbe& Probe : Probes)
		{
			if (const FSiegeDvorakCell* Cell = FindDvorakCell(Probe.QwertyKey))
			{
				Probe.VirtualKey = Cell->VirtualKey;
				Probe.CharCode   = Cell->CharCode;
			}
		}
		return Probes;
	}

	/**
	 *  The shipped table filled in as a US-QWERTY host would answer it: every position yields
	 *  its own letter, so VK == CharCode == the uppercase letter. The 99% path.
	 */
	static TArray<FSiegePositionalKeyProbe> MakeIdentityProbes()
	{
		TArray<FSiegePositionalKeyProbe> Probes = FSiegeKeyboardLayoutStatics::GetQwertyLetterScanCodes();
		for (FSiegePositionalKeyProbe& Probe : Probes)
		{
			const uint32 LetterChar = UppercaseCharForLetterKey(Probe.QwertyKey);
			Probe.VirtualKey = LetterChar;   // on US-QWERTY the letter VKs ARE 'A'..'Z'
			Probe.CharCode   = LetterChar;
		}
		return Probes;
	}

	// ════════════════════════════════════════════════════════════════════════════════════
	//  THE SYNTHETIC IMC FIXTURE (RetargetPreservesModifiers / NonIdentityDoesNotCompound)
	// ════════════════════════════════════════════════════════════════════════════════════

	/**
	 *  ⛔⛔ THE `MapKey` NOTE — READ THIS BEFORE FILING A `KBD-§2` FINDING.
	 *
	 *  `UInputMappingContext::MapKey` IS USED BELOW, DELIBERATELY, AND IT IS NOT A BREACH.
	 *  `KBD-§2` bans MapKey / UnmapKey / UnmapAll on every SHIPPED path, because MapKey
	 *  APPENDS a mapping built from the 2-arg ctor with empty Modifiers and Triggers
	 *  (InputMappingContext.cpp:154-158) — TASK-445's failure rewritten in C++. Here it is
	 *  used only to CONSTRUCT A FIXTURE FROM NOTHING: there is no asset to preserve, the
	 *  modifiers are attached explicitly on the next lines, and this file ships no runtime
	 *  behaviour. Grep this file — there is exactly ONE MapKey call and it is the line below,
	 *  inside this scaffolding function.
	 *
	 *  ✅ AND THE ENGINE ITSELF SAYS SO: the doc comment immediately above these accessors,
	 *  InputMappingContext.h:224, reads "TODO_BH: Deprecate all of these functions below. They
	 *  are only used in the editor and FOR TESTS!" — i.e. a test fixture is the one caller Epic
	 *  still expects. (MapKey is public, at :228, in the block opening at :150.)
	 *
	 *  ⭐ AND THE THING UNDER TEST IS CHECKED BEHAVIOURALLY, NOT ONLY BY GREP: if
	 *  RetargetContextKeys were ever reimplemented as unmap+map, the modifiers on the
	 *  duplicate would come back EMPTY from the 2-arg ctor — which is precisely what
	 *  Siegebound.Input.RetargetPreservesModifiers asserts against. The static check
	 *  (`KBD-§9` criterion 2, a grep of the shipped diff) is TASK-513's; this file supplies
	 *  the dynamic one.
	 *
	 *  ⚠️ MapKey returns a REFERENCE INTO the mappings array, so it is discarded here and the
	 *  mappings are re-fetched through GetMapping(Index) after all of them exist — a held
	 *  reference would dangle the moment the array reallocated.
	 */
	static UInputMappingContext* MakeSourceContext(const UInputAction* Action, const TArray<FKey>& Keys)
	{
		UInputMappingContext* Context = NewObject<UInputMappingContext>(GetTransientPackageAsObject());
		if (!Context)
		{
			return nullptr;
		}

		for (const FKey& Key : Keys)
		{
			Context->MapKey(Action, Key);   // ⛔ TEST-ONLY SCAFFOLDING — see the note above.
		}
		return Context;
	}

	/**
	 *  Attaches the TASK-445 modifier pair plus a trigger to one mapping.
	 *
	 *  ⭐ THE OUTER IS THE IMC, AND THAT IS THE WHOLE MECHANISM OF THE TEST. `Modifiers` and
	 *  `Triggers` are `UPROPERTY(Instanced)` arrays (EnhancedActionKeyMapping.h:79-90), and
	 *  DuplicateObject deep-copies exactly those referenced objects that are SUBOBJECTS of the
	 *  thing being duplicated — i.e. whose Outer chain leads to it. Creating these with
	 *  `Context` as Outer is what makes the duplicate get its OWN modifier instances, which is
	 *  what the pointer-inequality assertion proves. (An external asset like the UInputAction
	 *  is NOT a subobject, so its reference is preserved by pointer — also asserted.)
	 */
	static void AttachTask445Modifiers(UInputMappingContext* Context, int32 MappingIndex)
	{
		if (!Context || !Context->GetMappings().IsValidIndex(MappingIndex))
		{
			return;
		}

		FEnhancedActionKeyMapping& Mapping = Context->GetMapping(MappingIndex);

		// The exact pair TASK-445 lost: a Swizzle that makes WASD's 1D inputs drive the right
		// axis, and a Negate that inverts one axis only. Non-default VALUES on purpose — a
		// default-constructed replacement (the TASK-445 failure) would still have the right
		// COUNT and the right CLASSES, so only the values can detect it.
		UInputModifierSwizzleAxis* Swizzle = NewObject<UInputModifierSwizzleAxis>(Context);
		if (Swizzle)
		{
			Swizzle->Order = EInputAxisSwizzle::YXZ;
			Mapping.Modifiers.Add(Swizzle);
		}

		UInputModifierNegate* Negate = NewObject<UInputModifierNegate>(Context);
		if (Negate)
		{
			// ⚠️ ALL THREE DIFFER FROM THE CLASS DEFAULT (true/true/true —
			// InputModifiers.h:259-264), so a default-constructed stand-in fails every one.
			Negate->bX = false;
			Negate->bY = true;
			Negate->bZ = false;
			Mapping.Modifiers.Add(Negate);
		}

		// Triggers sit beside Modifiers in the same ban clause and in the same instanced-array
		// hazard, so one is carried too — with a NON-DEFAULT threshold (the class default is
		// 1.0f, InputTriggers.h:334), because a 0-vs-0 count comparison would be a vacuous
		// assertion dressed as a strong one.
		UInputTriggerHold* Hold = NewObject<UInputTriggerHold>(Context);
		if (Hold)
		{
			Hold->HoldTimeThreshold = 0.42f;
			Hold->bIsOneShot        = true;
			Mapping.Triggers.Add(Hold);
		}
	}

	/** Source + its transient duplicate, both GC-rooted for the life of the test. */
	struct FScratchContexts
	{
		TStrongObjectPtr<UInputAction>         Action;
		TStrongObjectPtr<UInputMappingContext> Source;
		TStrongObjectPtr<UInputMappingContext> Duplicate;

		bool IsValid() const { return Action.IsValid() && Source.IsValid() && Duplicate.IsValid(); }
	};

	/**
	 *  Builds a source context over `Keys` and the transient DuplicateObject copy of it that
	 *  TASK-511's GetPositionalContext would build — the same call, so the test exercises the
	 *  shipped shape rather than a lookalike.
	 */
	static FScratchContexts MakeScratchContexts(const TArray<FKey>& Keys, bool bAttachModifiers)
	{
		FScratchContexts Scratch;

		// Outer = transient package, NOT the IMC: an InputAction is an external asset in the
		// shipped case, and that distinction is exactly what the reference-preservation
		// assertion checks.
		Scratch.Action.Reset(NewObject<UInputAction>(GetTransientPackageAsObject()));
		if (!Scratch.Action.IsValid())
		{
			return Scratch;
		}

		UInputMappingContext* Source = MakeSourceContext(Scratch.Action.Get(), Keys);
		if (!Source)
		{
			return Scratch;
		}
		if (bAttachModifiers)
		{
			AttachTask445Modifiers(Source, 0);
		}
		Scratch.Source.Reset(Source);

		// ⭐ THE SHIPPED DUPLICATION CALL (`KBD-§1` / TASK-511 spec (2)) — the source is only
		// ever read, and the duplicate is the only non-const UInputMappingContext anywhere.
		Scratch.Duplicate.Reset(DuplicateObject<UInputMappingContext>(Source, GetTransientPackageAsObject()));
		return Scratch;
	}

	// ════════════════════════════════════════════════════════════════════════════════════
	//  THE SUBSYSTEM FIXTURE
	// ════════════════════════════════════════════════════════════════════════════════════

	/**
	 *  A USiegeKeyboardLayoutSubsystem inside a throwaway UGameInstance.
	 *
	 *  ⚠️ THE OUTER IS NOT OPTIONAL: UGameInstanceSubsystem is UCLASS(Abstract, Within =
	 *  GameInstance), so a bare NewObject lands in the transient package and trips the
	 *  ClassWithin check in StaticAllocateObject. Same reasoning, same shape as
	 *  SiegeSettingsTest.cpp:66-98.
	 *
	 *  ⚠️ Initialize(FSubsystemCollectionBase&) is NEVER called — a collection cannot be
	 *  fabricated outside the engine's own creation path. That is deliberate here rather than
	 *  merely tolerated: it means no OS probe, no 1 Hz timer and no Slate activation hook run
	 *  during this test, so the only translation state in play is the one
	 *  SetTranslationMapForAutomationTests puts there.
	 */
	struct FScratchSubsystem
	{
		TStrongObjectPtr<UGameInstance>                 GameInstance;
		TStrongObjectPtr<USiegeKeyboardLayoutSubsystem> Layout;

		bool IsValid() const { return GameInstance.IsValid() && Layout.IsValid(); }
	};

	static FScratchSubsystem MakeScratchSubsystem()
	{
		FScratchSubsystem Scratch;
		Scratch.GameInstance.Reset(NewObject<UGameInstance>(GetTransientPackageAsObject()));
		if (Scratch.GameInstance.IsValid())
		{
			Scratch.Layout.Reset(NewObject<USiegeKeyboardLayoutSubsystem>(Scratch.GameInstance.Get()));
		}
		return Scratch;
	}

	// ════════════════════════════════════════════════════════════════════════════════════
	//  SHARED ASSERTION HELPERS
	// ════════════════════════════════════════════════════════════════════════════════════

	/** Readable in a failure message: "W", "Comma", or "<invalid>" for EKeys::Invalid. */
	static FString Describe(const FKey& Key)
	{
		return Key.IsValid() ? Key.ToString() : FString(TEXT("<invalid>"));
	}
}

// ════════════════════════════════════════════════════════════════════════════════════════
//  TEST 1 — Siegebound.Input.ScanCodeTable
// ════════════════════════════════════════════════════════════════════════════════════════

/**
 *  THE CONSTANT THE WHOLE FEATURE RESTS ON.
 *
 *  ⚖️ Asserted rather than eyeballed because there is NO scan-code table anywhere in engine
 *  source to cross-check against (`KBD-§6`) — a transposed digit here would silently bind one
 *  action to the wrong physical key, and every other test in this file would still pass,
 *  because they inject their own probe data. This is the one place the table itself is the
 *  subject.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeKeyboardLayoutScanCodeTableTest,
	"Siegebound.Input.ScanCodeTable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeKeyboardLayoutScanCodeTableTest::RunTest(const FString& Parameters)
{
	const TArray<FSiegePositionalKeyProbe> Probes = FSiegeKeyboardLayoutStatics::GetQwertyLetterScanCodes();

	// ALL 26, not just the ten the game binds today (`KBD-§4`).
	TestEqual(TEXT("GetQwertyLetterScanCodes returns all 26 letter positions"), Probes.Num(), 26);
	if (Probes.Num() != 26)
	{
		return false;   // every assertion below reads the table; a wrong length makes them noise
	}

	// ── THE TEN ANCHORS, ASSERTED BY LOOKUP ─────────────────────────────────────────────
	// Looked up BY FKey, never by index, so this cannot be satisfied by an accidental reorder.
	struct FSiegeAnchor { const FKey& Key; uint32 ScanCode; };
	const FSiegeAnchor Anchors[] =
	{
		{ EKeys::Q, 0x10 }, { EKeys::W, 0x11 }, { EKeys::E, 0x12 }, { EKeys::R, 0x13 },
		{ EKeys::T, 0x14 }, { EKeys::A, 0x1E }, { EKeys::S, 0x1F }, { EKeys::D, 0x20 },
		{ EKeys::F, 0x21 }, { EKeys::C, 0x2E }
	};

	for (const FSiegeAnchor& Anchor : Anchors)
	{
		const FSiegePositionalKeyProbe* Found = Probes.FindByPredicate(
			[&Anchor](const FSiegePositionalKeyProbe& Probe) { return Probe.QwertyKey == Anchor.Key; });

		if (!Found)
		{
			AddError(FString::Printf(TEXT("The scan-code table has no row for %s — a bound key is missing entirely."),
				*SiegeKeyboardLayoutTestUtils::Describe(Anchor.Key)));
			continue;
		}

		TestEqual(FString::Printf(TEXT("%s sits at scan code 0x%02X"),
			*SiegeKeyboardLayoutTestUtils::Describe(Anchor.Key), Anchor.ScanCode),
			static_cast<int32>(Found->ScanCode), static_cast<int32>(Anchor.ScanCode));
	}

	// ── NO DUPLICATE SCAN CODES, NO DUPLICATE FKeys ─────────────────────────────────────
	// Two letters on one scan code would double-bind a physical key; one letter twice would
	// make the result depend on iteration order.
	TSet<uint32> SeenScanCodes;
	TSet<FKey>   SeenKeys;
	int32        UnfilledCount = 0;

	for (const FSiegePositionalKeyProbe& Probe : Probes)
	{
		TestTrue(FString::Printf(TEXT("Row %s carries a VALID FKey"), *SiegeKeyboardLayoutTestUtils::Describe(Probe.QwertyKey)),
			Probe.QwertyKey.IsValid());

		bool bScanAlreadySeen = false;
		SeenScanCodes.Add(Probe.ScanCode, &bScanAlreadySeen);
		TestFalse(FString::Printf(TEXT("Scan code 0x%02X appears exactly once"), Probe.ScanCode), bScanAlreadySeen);

		bool bKeyAlreadySeen = false;
		SeenKeys.Add(Probe.QwertyKey, &bKeyAlreadySeen);
		TestFalse(FString::Printf(TEXT("FKey %s appears exactly once"), *SiegeKeyboardLayoutTestUtils::Describe(Probe.QwertyKey)),
			bKeyAlreadySeen);

		// A scan code of 0 is not a key; it would be read as "no answer" downstream.
		TestTrue(FString::Printf(TEXT("%s has a non-zero scan code"), *SiegeKeyboardLayoutTestUtils::Describe(Probe.QwertyKey)),
			Probe.ScanCode != 0);

		if (Probe.VirtualKey == 0 && Probe.CharCode == 0)
		{
			++UnfilledCount;
		}
	}

	TestEqual(TEXT("26 distinct scan codes"), SeenScanCodes.Num(), 26);
	TestEqual(TEXT("26 distinct FKeys"), SeenKeys.Num(), 26);

	// ⛔ THE TABLE KNOWS NOTHING ABOUT THE ACTIVE LAYOUT. VirtualKey/CharCode are the platform
	// probe's to fill (`KBD-§8`); a table that pre-filled them would be a second, drifting
	// source of truth — and it would make this whole file host-dependent.
	TestEqual(TEXT("Every row leaves VirtualKey and CharCode at 0 for the platform probe"), UnfilledCount, 26);

	// ── ALPHABETICAL A..Z ORDER ─────────────────────────────────────────────────────────
	// ⚠️ LOAD-BEARING, NOT COSMETIC: `KBD-§8` pins DescribeActiveTranslation to emit its
	// entries in "source-key order as returned by GetQwertyLetterScanCodes()", so a reorder
	// here silently changes a string another task is contracted to produce byte-for-byte.
	bool bAlphabetical = true;
	for (int32 Index = 0; Index < Probes.Num(); ++Index)
	{
		const uint32 ExpectedChar = static_cast<uint32>(TEXT('A')) + static_cast<uint32>(Index);
		if (SiegeKeyboardLayoutTestUtils::UppercaseCharForLetterKey(Probes[Index].QwertyKey) != ExpectedChar)
		{
			bAlphabetical = false;
			AddError(FString::Printf(TEXT("Row %d is %s; the A..Z ordering DescribeActiveTranslation is pinned to requires %s."),
				Index, *SiegeKeyboardLayoutTestUtils::Describe(Probes[Index].QwertyKey),
				*FString::Chr(static_cast<TCHAR>(ExpectedChar))));
			break;
		}
	}
	TestTrue(TEXT("The table is in alphabetical A..Z order (DescribeActiveTranslation depends on it)"), bAlphabetical);

	return true;
}

// ════════════════════════════════════════════════════════════════════════════════════════
//  TEST 2 — Siegebound.Input.DvorakTranslation  ⭐ THE KEYSTONE
// ════════════════════════════════════════════════════════════════════════════════════════

/**
 *  ⭐ THE KEYSTONE. A US-DVORAK RESULT, ASSERTED ON A US-QWERTY MACHINE.
 *
 *  Nothing here touches the OS. The probe array carries the VK/CharCode pairs US-Dvorak
 *  produces (SiegeKeyboardLayoutTestUtils::FindDvorakCell — read its provenance note, the
 *  VK column is DERIVED and TASK-515 is authoritative) and the resolver is a model of the
 *  engine's own character-driven resolution. Everything between them is
 *  BuildTranslationMap's pure logic, which is the thing under test.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeKeyboardLayoutDvorakTranslationTest,
	"Siegebound.Input.DvorakTranslation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeKeyboardLayoutDvorakTranslationTest::RunTest(const FString& Parameters)
{
	using namespace SiegeKeyboardLayoutTestUtils;

	const TArray<FSiegePositionalKeyProbe> Probes = MakeDvorakProbes();

	// FIXTURE SELF-CHECK, FIRST. A fixture that silently degraded (a missing row, an unfilled
	// probe) would make every assertion below pass for the wrong reason.
	TestEqual(TEXT("The Dvorak fixture carries all 26 letter positions"), Probes.Num(), 26);
	int32 FilledCount = 0;
	for (const FSiegePositionalKeyProbe& Probe : Probes)
	{
		if (Probe.VirtualKey != 0 && Probe.CharCode != 0)
		{
			++FilledCount;
		}
	}
	TestEqual(TEXT("Every one of the 26 Dvorak probes answered (VirtualKey and CharCode both set)"), FilledCount, 26);

	auto FakeResolver = [](uint32 VirtualKey, uint32 CharCode) { return ResolveLikePlatform(VirtualKey, CharCode); };

	TMap<FKey, FKey> Translation;
	const int32 NumTranslated = FSiegeKeyboardLayoutStatics::BuildTranslationMap(Probes, FakeResolver, Translation);

	// 26 positions, 24 of them non-identity: only A and M sit in the same place on Dvorak.
	TestEqual(TEXT("BuildTranslationMap reports 24 non-identity entries for US-Dvorak"), NumTranslated, 24);
	TestEqual(TEXT("The return value IS the map size"), Translation.Num(), NumTranslated);

	// ── THE NINE BOUND KEYS — the ones a wrong answer would actually break in play ───────
	struct FSiegeExpectedTranslation { const FKey& From; const FKey& To; const TCHAR* Why; };
	const FSiegeExpectedTranslation Expected[] =
	{
		{ EKeys::W, EKeys::Comma,      TEXT("move forward")  },
		{ EKeys::S, EKeys::O,          TEXT("move back")     },
		{ EKeys::D, EKeys::E,          TEXT("move right")    },
		{ EKeys::Q, EKeys::Apostrophe, TEXT("rally")         },
		{ EKeys::E, EKeys::Period,     TEXT("unit command")  },
		{ EKeys::R, EKeys::P,          TEXT("unit command")  },
		{ EKeys::T, EKeys::Y,          TEXT("unit command")  },
		{ EKeys::F, EKeys::U,          TEXT("unit command")  },
		{ EKeys::C, EKeys::J,          TEXT("unit command")  }
	};

	// ⚠️ A and M are NOT in this array on purpose — an identity is a claim about ABSENCE and
	// is asserted as one, immediately below. Listing them here with `To == From` would make
	// this loop pass on a map that wrongly CONTAINED an identity entry.
	for (const FSiegeExpectedTranslation& Case : Expected)
	{
		const FKey* Actual = Translation.Find(Case.From);
		if (!Actual)
		{
			AddError(FString::Printf(TEXT("Dvorak: %s (%s) has NO translation — it would stay on the QWERTY key, in the wrong physical place."),
				*Describe(Case.From), Case.Why));
			continue;
		}

		TestTrue(FString::Printf(TEXT("Dvorak: the %s position (%s) yields %s — got %s"),
			*Describe(Case.From), Case.Why, *Describe(Case.To), *Describe(*Actual)),
			*Actual == Case.To);
	}

	// ── THE TWO IDENTITIES — asserted as ABSENCES, which is the real contract ───────────
	// ⭐ An identity entry in the map would be a bug even though it "looks right": `KBD-§8`
	// pins DescribeActiveTranslation to omit identities, and the map size is the caller's
	// "is a remap active at all" signal.
	TestFalse(TEXT("A is ABSENT from the map — the A position is identical on Dvorak"),
		Translation.Contains(EKeys::A));
	TestFalse(TEXT("M is ABSENT from the map — the M position is identical on Dvorak too"),
		Translation.Contains(EKeys::M));

	// ── ⛔ NOTHING IS EVER EKeys::Invalid, AND NOTHING IS AN IDENTITY (`KBD-§5`) ─────────
	TSet<FKey> Targets;
	for (const TPair<FKey, FKey>& Entry : Translation)
	{
		TestTrue(FString::Printf(TEXT("Source key %s is valid"), *Describe(Entry.Key)), Entry.Key.IsValid());
		TestTrue(FString::Printf(TEXT("Target key %s is valid — NEVER EKeys::Invalid"), *Describe(Entry.Value)),
			Entry.Value.IsValid() && Entry.Value != EKeys::Invalid);
		TestTrue(FString::Printf(TEXT("%s -> %s is a real move, not an identity entry"),
			*Describe(Entry.Key), *Describe(Entry.Value)), Entry.Key != Entry.Value);
		Targets.Add(Entry.Value);
	}

	// ⭐ INJECTIVITY ON REAL DATA: 24 sources landing on 24 DISTINCT targets. Two actions
	// sharing one physical key is the failure this feature would be most embarrassed by.
	TestEqual(TEXT("The 24 translations land on 24 DISTINCT keys — no two actions share a physical key"),
		Targets.Num(), Translation.Num());

	// ⭐ THE SWAP THAT PROVES SIMULTANEITY IS EVEN POSSIBLE: the map holds D -> E AND
	// E -> Period at once. A translator that read its own output would cascade D -> Period.
	// RetargetContextKeys is where that is actually exercised (NonIdentityDoesNotCompound);
	// this asserts the map genuinely poses the problem, so that test is not vacuous.
	const FKey* DTarget = Translation.Find(EKeys::D);
	const FKey* ETarget = Translation.Find(EKeys::E);
	TestTrue(TEXT("The map really does contain the chained pair D->E and E->Period"),
		DTarget && ETarget && *DTarget == EKeys::E && *ETarget == EKeys::Period);

	return true;
}

// ════════════════════════════════════════════════════════════════════════════════════════
//  TEST 3 — Siegebound.Input.QwertyIsPassThrough
// ════════════════════════════════════════════════════════════════════════════════════════

/**
 *  THE 99% PATH COSTS NOTHING.
 *
 *  Two halves: the pure one (identity probes ⇒ an EMPTY map, asserted hard, host-independent)
 *  and the subsystem one (an empty map ⇒ GetPositionalContext hands back THE SAME POINTER —
 *  no DuplicateObject, no allocation, `KBD-§5` row 2).
 *
 *  ⚠️ THE SECOND HALF IS THE ONE PLACE OUTSIDE TEST 7 THAT CAN SEE THE REAL MACHINE, AND IT
 *  LOGS RATHER THAN FAILS WHEN IT DOES. TASK-511's GetPositionalContext is specified to
 *  RE-PROBE the OS layout on every call (TASK-511 spec (4a)), so on a genuinely non-QWERTY
 *  host it may legitimately replace the empty map this test forces in. The state is therefore
 *  re-read AFTER the call and the pointer claim is made only when the map really was empty
 *  for the whole call. ⚖️ A hard assert would red-fail on Jonathan's Dvorak machine — the one
 *  this feature exists for. The non-null claim below is unconditional, because `KBD-§5`
 *  admits no environment in which null is acceptable.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeKeyboardLayoutQwertyIsPassThroughTest,
	"Siegebound.Input.QwertyIsPassThrough",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeKeyboardLayoutQwertyIsPassThroughTest::RunTest(const FString& Parameters)
{
	using namespace SiegeKeyboardLayoutTestUtils;

	// ── HALF ONE: THE PURE CLAIM ────────────────────────────────────────────────────────
	const TArray<FSiegePositionalKeyProbe> Probes = MakeIdentityProbes();
	TestEqual(TEXT("The identity fixture carries all 26 letter positions"), Probes.Num(), 26);

	auto FakeResolver = [](uint32 VirtualKey, uint32 CharCode) { return ResolveLikePlatform(VirtualKey, CharCode); };

	// Seeded with a stale entry on purpose: BuildTranslationMap is contracted to RESET the map,
	// so a re-probe after a Win+Space back to QWERTY cannot inherit the layout it just left.
	TMap<FKey, FKey> Translation;
	Translation.Add(EKeys::W, EKeys::Comma);

	const int32 NumTranslated = FSiegeKeyboardLayoutStatics::BuildTranslationMap(Probes, FakeResolver, Translation);

	TestEqual(TEXT("A positionally-QWERTY host yields ZERO translations"), NumTranslated, 0);
	TestEqual(TEXT("...and the map is EMPTY, so the stale pre-seeded entry was reset away"), Translation.Num(), 0);
	TestFalse(TEXT("The stale W -> Comma entry did not survive the re-probe"), Translation.Contains(EKeys::W));

	// ── HALF TWO: THE ALLOCATION CLAIM, THROUGH THE SUBSYSTEM ───────────────────────────
	FScratchContexts Scratch = MakeScratchContexts({ EKeys::W, EKeys::A }, /*bAttachModifiers*/ false);
	if (!Scratch.Source.IsValid())
	{
		AddError(TEXT("Could not build a synthetic UInputMappingContext — the pass-through claim is untested."));
		return false;
	}

	FScratchSubsystem Subsystem = MakeScratchSubsystem();
	if (!Subsystem.IsValid())
	{
		AddError(TEXT("Could not construct a USiegeKeyboardLayoutSubsystem inside a UGameInstance."));
		return false;
	}

	// ⛔ Tests-only setter (`KBD-§8`), mirroring SiegeSettingsSubsystem::SetSlotNameForAutomationTests.
	// Forcing the state is what makes this a test of the CONTRACT rather than of this machine.
	const TMap<FKey, FKey> EmptyTranslation;
	Subsystem.Layout->SetTranslationMapForAutomationTests(EmptyTranslation);

	const UInputMappingContext* SourceContext = Scratch.Source.Get();
	const UInputMappingContext* Result        = Subsystem.Layout->GetPositionalContext(SourceContext);

	// ⛔ UNCONDITIONAL (`KBD-§5`): the worst outcome this feature may produce is "the game
	// behaves exactly as it did yesterday" — never a null context.
	TestTrue(TEXT("GetPositionalContext NEVER returns null for a non-null source"), Result != nullptr);

	if (!Subsystem.Layout->IsPositionalRemapActive())
	{
		// ⭐ THE ALLOCATION CLAIM. Pointer identity, not equality of contents: a duplicate that
		// happened to compare equal would still be an allocation on the 99% path.
		TestTrue(TEXT("An empty translation returns THE SAME POINTER — no duplicate, no allocation"),
			Result == SourceContext);
	}
	else
	{
		AddInfo(FString::Printf(
			TEXT("ENVIRONMENT: the subsystem re-probed and found a NON-QWERTY host layout (%s), so the forced empty map did not survive the call. ")
			TEXT("The same-pointer claim is SKIPPED rather than failed — this is the machine the feature is for, and a red here would be backwards. ")
			TEXT("The pure half of this test (identity probes -> zero translations) still ran and is unaffected."),
			*Subsystem.Layout->DescribeActiveTranslation()));
	}

	// The null-source row of the fail-safe table (`KBD-§5`), which is the caller's existing guard.
	TestTrue(TEXT("A null source yields null — the caller's own null-guard already covers it"),
		Subsystem.Layout->GetPositionalContext(nullptr) == nullptr);

	return true;
}

// ════════════════════════════════════════════════════════════════════════════════════════
//  TEST 4 — Siegebound.Input.DegenerateProbesAreRefused
// ════════════════════════════════════════════════════════════════════════════════════════

/**
 *  EVERY DEGENERATE PROBE DEGRADES TO THE SOURCE KEY, AND NOTHING EVER BECOMES EKeys::Invalid.
 *
 *  ⚠️⚠️ THE COLLISION CASE CARRIES AN OPEN RULING — DO NOT "TIDY" THE ASSERTION AWAY.
 *
 *  TASK-509 found a genuine conflict between two authorities on what happens when two probe
 *  positions resolve to the SAME FKey (handoffs/TASK-509-programmer.md §5.2):
 *
 *    • THE PLAN (`ok-there-are-a-cheerful-mccarthy.md`, test 4) says "two positions colliding:
 *      EACH keeps the source key" ⇒ reads as BOTH refused.
 *    • THE BOARD, CONVENTIONS `KBD-§5` ("two positions claim one target | THAT LETTER keeps
 *      its source key" — singular) and TASK-509's dispatch all say the target already claimed
 *      by an EARLIER probe is refused ⇒ FIRST-WINS.
 *
 *  ⛔ TASK-509 IMPLEMENTED FIRST-WINS (SiegeKeyboardLayoutStatics.cpp:161-168), AND THIS TEST
 *  ASSERTS THE BEHAVIOUR THAT SHIPS RATHER THAN EITHER WORDING. ⚖️ **TASK-513 (QA) OWNS THE
 *  RULING.** If it rules for mutual refusal, the two assertions marked ⚖️ RULING below invert
 *  (the earlier probe stops being translated too) and the implementation changes with them.
 *  Encoding one reading silently, as if it were settled, is the thing this comment exists to
 *  prevent.
 *
 *  Context for the ruling, so it is decided on the merits: first-wins preserves ONE working
 *  binding where mutual refusal preserves NONE. Its one theoretical cost is that a REFUSED
 *  probe's untranslated key can itself be some ACCEPTED probe's target, producing one
 *  double-bound key. TASK-509 hand-traced Dvorak, AZERTY and QWERTZ and found ZERO refusals
 *  on all three — every Latin layout is a bijection over the 26 letter positions — so on every
 *  layout this project cares about the branch is unreachable. ⛔ Either way the `KBD-§5` floor
 *  holds: never null, never EKeys::Invalid, never a partial context.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeKeyboardLayoutDegenerateProbesTest,
	"Siegebound.Input.DegenerateProbesAreRefused",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeKeyboardLayoutDegenerateProbesTest::RunTest(const FString& Parameters)
{
	using namespace SiegeKeyboardLayoutTestUtils;

	// A resolver that ALSO refuses one sentinel VK outright, so the "resolver answered
	// EKeys::Invalid" path can be driven deliberately rather than hoped for.
	auto RefusingResolver = [](uint32 VirtualKey, uint32 CharCode) -> FKey
	{
		if (VirtualKey == UnresolvableVirtualKey)
		{
			return EKeys::Invalid;
		}
		return ResolveLikePlatform(VirtualKey, CharCode);
	};

	// ── CASE A: VirtualKey == 0 (the platform probe failed, or never ran) ───────────────
	{
		TArray<FSiegePositionalKeyProbe> Probes;
		FSiegePositionalKeyProbe& Failed = Probes.AddDefaulted_GetRef();
		Failed.QwertyKey  = EKeys::W;
		Failed.ScanCode   = 0x11;
		Failed.VirtualKey = 0;                                   // ⛔ no answer
		Failed.CharCode   = static_cast<uint32>(TEXT(','));      // even a plausible char cannot rescue it

		TMap<FKey, FKey> Translation;
		const int32 Num = FSiegeKeyboardLayoutStatics::BuildTranslationMap(Probes, RefusingResolver, Translation);

		TestEqual(TEXT("VirtualKey == 0: no translation is produced"), Num, 0);
		TestFalse(TEXT("VirtualKey == 0: W keeps its source key (no map entry)"), Translation.Contains(EKeys::W));
	}

	// ── CASE B: the resolver cannot name the pair ───────────────────────────────────────
	{
		TArray<FSiegePositionalKeyProbe> Probes;
		FSiegePositionalKeyProbe& Unresolvable = Probes.AddDefaulted_GetRef();
		Unresolvable.QwertyKey  = EKeys::A;
		Unresolvable.ScanCode   = 0x1E;
		Unresolvable.VirtualKey = UnresolvableVirtualKey;
		Unresolvable.CharCode   = 0;

		TMap<FKey, FKey> Translation;
		const int32 Num = FSiegeKeyboardLayoutStatics::BuildTranslationMap(Probes, RefusingResolver, Translation);

		TestEqual(TEXT("Resolver returned EKeys::Invalid: no translation is produced"), Num, 0);
		TestFalse(TEXT("Resolver returned EKeys::Invalid: A keeps its source key"), Translation.Contains(EKeys::A));
		// ⛔ The failure this asserts against is STORING the Invalid, not returning it: an
		// EKeys::Invalid value in the map would reach RetargetContextKeys and unbind a key.
		for (const TPair<FKey, FKey>& Entry : Translation)
		{
			TestTrue(TEXT("No map entry is EKeys::Invalid"), Entry.Value.IsValid() && Entry.Value != EKeys::Invalid);
		}
	}

	// ── CASE C: TWO POSITIONS RESOLVING TO ONE FKey  ⚖️ THE OPEN RULING ─────────────────
	{
		// Deliberate ORDER: S is the earlier probe, D the later one. Both answer EKeys::O.
		// Reserved up front so the two references below cannot be invalidated by a regrow —
		// they are written before the second Add today, and a future edit should not have to
		// notice that.
		TArray<FSiegePositionalKeyProbe> Probes;
		Probes.Reserve(2);

		FSiegePositionalKeyProbe& First = Probes.AddDefaulted_GetRef();
		First.QwertyKey  = EKeys::S;
		First.ScanCode   = 0x1F;
		First.VirtualKey = 0x4F;
		First.CharCode   = static_cast<uint32>(TEXT('O'));

		FSiegePositionalKeyProbe& Second = Probes.AddDefaulted_GetRef();
		Second.QwertyKey  = EKeys::D;
		Second.ScanCode   = 0x20;
		Second.VirtualKey = 0x4F;                                // the same answer — the collision
		Second.CharCode   = static_cast<uint32>(TEXT('O'));

		TMap<FKey, FKey> Translation;
		const int32 Num = FSiegeKeyboardLayoutStatics::BuildTranslationMap(Probes, RefusingResolver, Translation);

		// ⚖️ RULING — asserts FIRST-WINS, which is what TASK-509 implemented. Inverts to 0 /
		// TestFalse for both keys if TASK-513 rules for the plan's mutual-refusal wording.
		TestEqual(TEXT("Collision (first-wins, TASK-509 as implemented): exactly ONE translation survives"), Num, 1);

		const FKey* SurvivingTarget = Translation.Find(EKeys::S);
		TestTrue(TEXT("Collision (first-wins): the EARLIER probe S keeps its translation to O"),
			SurvivingTarget && *SurvivingTarget == EKeys::O);

		// ⛔ THIS HALF IS NOT PART OF THE RULING AND HOLDS UNDER EITHER READING: the loser is
		// never translated, and above all is never left holding EKeys::Invalid.
		TestFalse(TEXT("Collision: the LATER probe D is refused and keeps its source key"),
			Translation.Contains(EKeys::D));

		// ── AND THE CLAIM THAT ACTUALLY MATTERS AT RUNTIME: a context bound to both keys
		//    survives the retarget with D still on D, never unbound.
		FScratchContexts Scratch = MakeScratchContexts({ EKeys::S, EKeys::D }, /*bAttachModifiers*/ false);
		if (!Scratch.IsValid())
		{
			AddError(TEXT("Could not build the synthetic source+duplicate pair — the survival claim is untested."));
			return false;
		}

		int32 NumRetargeted = -1;
		const bool bApplied = FSiegeKeyboardLayoutStatics::RetargetContextKeys(
			Scratch.Source.Get(), Scratch.Duplicate.Get(), Translation, NumRetargeted);

		TestTrue(TEXT("The retarget applies even with a refused probe in play"), bApplied);
		TestEqual(TEXT("Exactly one mapping moved"), NumRetargeted, 1);

		const TArray<FEnhancedActionKeyMapping>& DupMappings = Scratch.Duplicate->GetMappings();
		if (DupMappings.Num() == 2)
		{
			TestTrue(FString::Printf(TEXT("The translated mapping moved S -> O (got %s)"), *Describe(DupMappings[0].Key)),
				DupMappings[0].Key == EKeys::O);
			TestTrue(FString::Printf(TEXT("The REFUSED mapping still holds its SOURCE key D (got %s)"), *Describe(DupMappings[1].Key)),
				DupMappings[1].Key == EKeys::D);
		}
		else
		{
			AddError(TEXT("The duplicate does not carry the two mappings it was built with."));
		}

		for (const FEnhancedActionKeyMapping& Mapping : DupMappings)
		{
			TestTrue(TEXT("⛔ No mapping is left holding EKeys::Invalid after a refusal"),
				Mapping.Key.IsValid() && Mapping.Key != EKeys::Invalid);
		}
	}

	// ── CASE D: a malformed table row (defensive; the shipped table never produces one) ──
	{
		TArray<FSiegePositionalKeyProbe> Probes;
		FSiegePositionalKeyProbe& Malformed = Probes.AddDefaulted_GetRef();
		Malformed.QwertyKey  = FKey();                            // default-constructed: invalid
		Malformed.ScanCode   = 0x11;
		Malformed.VirtualKey = 0x4F;
		Malformed.CharCode   = static_cast<uint32>(TEXT('O'));

		TMap<FKey, FKey> Translation;
		const int32 Num = FSiegeKeyboardLayoutStatics::BuildTranslationMap(Probes, RefusingResolver, Translation);

		TestEqual(TEXT("An invalid source FKey produces no translation"), Num, 0);
		TestFalse(TEXT("...and an invalid FKey never enters the map as a key"), Translation.Contains(FKey()));
	}

	// ── CASE E: a duplicated source row — first row wins ────────────────────────────────
	// ⚠️ Behaviour the pin left OPEN; TASK-509 chose first-row-wins and flagged it (§5.3).
	// Asserted because "whichever the loop happens to reach first" is the alternative, and an
	// order-dependent result in a pure function is exactly the kind of thing that ships.
	{
		TArray<FSiegePositionalKeyProbe> Probes;
		Probes.Reserve(2);

		FSiegePositionalKeyProbe& FirstRow = Probes.AddDefaulted_GetRef();
		FirstRow.QwertyKey  = EKeys::W;
		FirstRow.ScanCode   = 0x11;
		FirstRow.VirtualKey = 0xBC;
		FirstRow.CharCode   = static_cast<uint32>(TEXT(','));

		FSiegePositionalKeyProbe& DuplicateRow = Probes.AddDefaulted_GetRef();
		DuplicateRow.QwertyKey  = EKeys::W;                       // the same source, a different answer
		DuplicateRow.ScanCode   = 0x11;
		DuplicateRow.VirtualKey = 0xBE;
		DuplicateRow.CharCode   = static_cast<uint32>(TEXT('.'));

		TMap<FKey, FKey> Translation;
		const int32 Num = FSiegeKeyboardLayoutStatics::BuildTranslationMap(Probes, RefusingResolver, Translation);

		TestEqual(TEXT("A duplicated source row yields exactly one entry"), Num, 1);
		const FKey* Target = Translation.Find(EKeys::W);
		TestTrue(TEXT("The FIRST row wins, deterministically (not whichever the loop reached last)"),
			Target && *Target == EKeys::Comma);
	}

	// ── CASE F: the non-Windows path — the shipped table, entirely unfilled ─────────────
	// ⭐ HOST-INDEPENDENT EVEN THOUGH IT USES THE LIVE RESOLVER: every probe carries
	// VirtualKey == 0, and BuildTranslationMap skips on that BEFORE calling the resolver, so
	// the resolver provably never runs. This is `KBD-§5`'s "non-Windows ⇒ pass-through" row.
	{
		auto LiveResolver = [](uint32 VirtualKey, uint32 CharCode)
		{
			return FSiegeKeyboardLayoutStatics::ResolveKeyFromCodes(VirtualKey, CharCode);
		};

		const TArray<FSiegePositionalKeyProbe> UnfilledProbes = FSiegeKeyboardLayoutStatics::GetQwertyLetterScanCodes();
		TMap<FKey, FKey> Translation;
		const int32 Num = FSiegeKeyboardLayoutStatics::BuildTranslationMap(UnfilledProbes, LiveResolver, Translation);

		TestEqual(TEXT("An unfilled table (non-Windows: no probe ran) yields ZERO translations"), Num, 0);
		TestEqual(TEXT("...and an empty map, which is the pass-through signal"), Translation.Num(), 0);
	}

	return true;
}

// ════════════════════════════════════════════════════════════════════════════════════════
//  TEST 5 — Siegebound.Input.RetargetPreservesModifiers  ⭐ TASK-445 MECHANISED
// ════════════════════════════════════════════════════════════════════════════════════════

/**
 *  ⭐⭐ TASK-445, MECHANISED — THE HIGHEST-VALUE TEST IN THIS BATCH.
 *
 *  handoffs/TASK-445-artist.md:95-105: rewriting an IMC's mappings array SILENTLY
 *  DEFAULT-CONSTRUCTED the instanced SwizzleAxis/Negate modifiers. WASD broke and mouse-look
 *  inverted — ⛔ WHILE THE PROPERTY TABLE STILL READ CORRECT. That last clause is why this
 *  test exists and why it asserts VALUES: the failure had the right modifier COUNT and the
 *  right modifier CLASSES. A test that checked only `Modifiers.Num()` would have gone green
 *  through the entire incident.
 *
 *  Four independent claims, and each catches a different way of getting this wrong:
 *    1. the Key moved                    — the retarget did its job at all;
 *    2. counts + classes match pairwise  — nothing was dropped or substituted;
 *    3. ⭐ the duplicate's modifier POINTERS DIFFER from the source's — a real deep copy, so
 *       nothing the duplicate does can ever reach back into IMC_Hero;
 *    4. ⭐ the VALUES survive             — the TASK-445 failure itself.
 *  Plus: the SOURCE is byte-for-byte untouched, which is `KBD-§1`'s central law.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeKeyboardLayoutRetargetPreservesModifiersTest,
	"Siegebound.Input.RetargetPreservesModifiers",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeKeyboardLayoutRetargetPreservesModifiersTest::RunTest(const FString& Parameters)
{
	using namespace SiegeKeyboardLayoutTestUtils;

	// Mapping 0 = W, carrying the TASK-445 modifier pair + a trigger. Mapping 1 = A, bare and
	// untranslated, so the "untranslated keys keep their source value" path is covered too.
	FScratchContexts Scratch = MakeScratchContexts({ EKeys::W, EKeys::A }, /*bAttachModifiers*/ true);
	if (!Scratch.IsValid())
	{
		AddError(TEXT("Could not build the synthetic source + duplicate contexts."));
		return false;
	}

	const TArray<FEnhancedActionKeyMapping>& SourceMappings = Scratch.Source->GetMappings();
	if (SourceMappings.Num() != 2 || SourceMappings[0].Modifiers.Num() != 2)
	{
		AddError(FString::Printf(TEXT("The fixture is wrong: %d mappings, %d modifiers on mapping 0 (expected 2 and 2)."),
			SourceMappings.Num(), SourceMappings.Num() > 0 ? SourceMappings[0].Modifiers.Num() : -1));
		return false;
	}

	TMap<FKey, FKey> Translation;
	Translation.Add(EKeys::W, EKeys::Comma);   // the Dvorak answer for the W position

	int32 NumRetargeted = -1;
	const bool bApplied = FSiegeKeyboardLayoutStatics::RetargetContextKeys(
		Scratch.Source.Get(), Scratch.Duplicate.Get(), Translation, NumRetargeted);

	TestTrue(TEXT("RetargetContextKeys applied"), bApplied);
	TestEqual(TEXT("Exactly one mapping moved"), NumRetargeted, 1);

	const TArray<FEnhancedActionKeyMapping>& DupMappings = Scratch.Duplicate->GetMappings();
	TestEqual(TEXT("The duplicate carries the same number of mappings"), DupMappings.Num(), SourceMappings.Num());
	if (DupMappings.Num() != 2)
	{
		return false;
	}

	// ── CLAIM 1: THE KEY MOVED ──────────────────────────────────────────────────────────
	TestTrue(FString::Printf(TEXT("The W mapping became Comma (got %s)"), *Describe(DupMappings[0].Key)),
		DupMappings[0].Key == EKeys::Comma);
	TestTrue(FString::Printf(TEXT("The untranslated A mapping kept its SOURCE key (got %s)"), *Describe(DupMappings[1].Key)),
		DupMappings[1].Key == EKeys::A);

	// ── ⛔ `KBD-§1`: THE SOURCE IS UNTOUCHED. In PIE the source IS the editor's loaded
	//    IMC_Hero, so this is the assertion standing between this feature and a dirtied asset.
	TestTrue(FString::Printf(TEXT("⛔ The SOURCE mapping still holds W — IMC_Hero is never written (got %s)"),
		*Describe(SourceMappings[0].Key)), SourceMappings[0].Key == EKeys::W);
	TestEqual(TEXT("⛔ The SOURCE still has both its modifiers"), SourceMappings[0].Modifiers.Num(), 2);

	// ── CLAIM 2: COUNTS AND CLASSES MATCH PAIRWISE ──────────────────────────────────────
	TestEqual(TEXT("The duplicate has the SAME NUMBER of modifiers"),
		DupMappings[0].Modifiers.Num(), SourceMappings[0].Modifiers.Num());
	TestEqual(TEXT("The duplicate has the SAME NUMBER of triggers"),
		DupMappings[0].Triggers.Num(), SourceMappings[0].Triggers.Num());
	TestEqual(TEXT("The bare mapping still has no modifiers"), DupMappings[1].Modifiers.Num(), 0);

	// The Action is an EXTERNAL asset, not a subobject of the IMC, so DuplicateObject must
	// preserve the reference by pointer rather than copy it.
	TestTrue(TEXT("The Action reference is preserved by pointer (it is not a subobject)"),
		DupMappings[0].Action.Get() == SourceMappings[0].Action.Get());

	if (DupMappings[0].Modifiers.Num() == SourceMappings[0].Modifiers.Num())
	{
		for (int32 Index = 0; Index < DupMappings[0].Modifiers.Num(); ++Index)
		{
			const UInputModifier* SourceModifier = SourceMappings[0].Modifiers[Index].Get();
			const UInputModifier* DupModifier    = DupMappings[0].Modifiers[Index].Get();

			if (!SourceModifier || !DupModifier)
			{
				AddError(FString::Printf(TEXT("Modifier %d is NULL after duplication (source: %s, duplicate: %s) — this is the TASK-445 shape."),
					Index, SourceModifier ? TEXT("ok") : TEXT("null"), DupModifier ? TEXT("ok") : TEXT("null")));
				continue;
			}

			TestTrue(FString::Printf(TEXT("Modifier %d keeps its class (%s)"), Index, *SourceModifier->GetClass()->GetName()),
				DupModifier->GetClass() == SourceModifier->GetClass());

			// ── ⭐ CLAIM 3: A REAL DEEP COPY ────────────────────────────────────────────
			// If these were the same object, every later write through the duplicate would
			// reach into the loaded IMC_Hero — the TASK-445 failure with a different cause.
			TestTrue(FString::Printf(TEXT("⭐ Modifier %d is a DISTINCT OBJECT in the duplicate (deep copy)"), Index),
				DupModifier != SourceModifier);
		}
	}

	// ── ⭐ CLAIM 4: THE VALUES SURVIVED — the assertion TASK-445 would have failed ───────
	const UInputModifierSwizzleAxis* DupSwizzle = nullptr;
	const UInputModifierNegate*      DupNegate  = nullptr;
	for (const TObjectPtr<UInputModifier>& Modifier : DupMappings[0].Modifiers)
	{
		if (!DupSwizzle) { DupSwizzle = Cast<UInputModifierSwizzleAxis>(Modifier.Get()); }
		if (!DupNegate)  { DupNegate  = Cast<UInputModifierNegate>(Modifier.Get()); }
	}

	if (DupSwizzle)
	{
		TestTrue(TEXT("⭐ The duplicate's SwizzleAxis still has Order == YXZ"),
			DupSwizzle->Order == EInputAxisSwizzle::YXZ);
	}
	else
	{
		AddError(TEXT("The duplicate lost its UInputModifierSwizzleAxis entirely — WASD would move on the wrong axis."));
	}

	if (DupNegate)
	{
		// ⛔ ALL THREE ASSERTED SEPARATELY. The class default is true/true/true, so a
		// default-constructed stand-in fails bX and bZ — which is exactly the readback that
		// "still looked correct" in TASK-445 when only the count was checked.
		TestFalse(TEXT("⭐ The duplicate's Negate still has bX == false"), DupNegate->bX);
		TestTrue (TEXT("⭐ The duplicate's Negate still has bY == true"),  DupNegate->bY);
		TestFalse(TEXT("⭐ The duplicate's Negate still has bZ == false"), DupNegate->bZ);
	}
	else
	{
		AddError(TEXT("The duplicate lost its UInputModifierNegate entirely — mouse-look would invert."));
	}

	// The trigger rides the same instanced-array hazard and is in the same ban clause, so its
	// non-default value is asserted too rather than merely counted.
	if (DupMappings[0].Triggers.Num() == 1 && SourceMappings[0].Triggers.Num() == 1)
	{
		const UInputTriggerHold* DupHold = Cast<UInputTriggerHold>(DupMappings[0].Triggers[0].Get());
		if (DupHold)
		{
			TestEqual(TEXT("⭐ The duplicate's Hold trigger keeps its non-default threshold"),
				DupHold->HoldTimeThreshold, 0.42f);
			TestTrue(TEXT("⭐ ...and its non-default bIsOneShot"), DupHold->bIsOneShot);
			TestTrue(TEXT("⭐ The trigger is a DISTINCT OBJECT in the duplicate (deep copy)"),
				DupHold != SourceMappings[0].Triggers[0].Get());
		}
		else
		{
			AddError(TEXT("The duplicate's trigger is not a UInputTriggerHold — the instanced trigger was substituted."));
		}
	}

	return true;
}

// ════════════════════════════════════════════════════════════════════════════════════════
//  TEST 6 — Siegebound.Input.NonIdentityDoesNotCompound
// ════════════════════════════════════════════════════════════════════════════════════════

/**
 *  THE PROPERTY THAT MAKES THE MID-SESSION RELAYOUT PATH CORRECT.
 *
 *  TASK-511 re-targets its CACHED duplicate IN PLACE when the OS layout changes, so that the
 *  pointer AHeroCharacter handed to AddMappingContext never changes (`KBD-§6`, and it is what
 *  keeps TASK-512 at four lines). ⇒ RetargetContextKeys is called AGAIN on a context that has
 *  ALREADY been retargeted. That is only safe because every value is re-derived from the
 *  pristine Source by index and the target is never read back.
 *
 *  ⭐ TWO DISTINCT CLAIMS, AND THE FIRST IS THE SUBTLER ONE:
 *    (a) NO CASCADE, EVEN ON THE FIRST CALL. The map holds D -> E AND E -> Period at once. A
 *        translator that read its own output would produce D -> Period — a hero that moves the
 *        wrong way, from a translation table that reads perfectly correct.
 *    (b) NO COMPOUNDING ON THE SECOND CALL. Repeating the operation changes nothing.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeKeyboardLayoutNonIdentityDoesNotCompoundTest,
	"Siegebound.Input.NonIdentityDoesNotCompound",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeKeyboardLayoutNonIdentityDoesNotCompoundTest::RunTest(const FString& Parameters)
{
	using namespace SiegeKeyboardLayoutTestUtils;

	// W, A, S, D, E — the chained pair D->E / E->Period is what makes this test non-vacuous.
	FScratchContexts Scratch = MakeScratchContexts(
		{ EKeys::W, EKeys::A, EKeys::S, EKeys::D, EKeys::E }, /*bAttachModifiers*/ true);
	if (!Scratch.IsValid())
	{
		AddError(TEXT("Could not build the synthetic source + duplicate contexts."));
		return false;
	}

	// The real US-Dvorak answers for these five positions.
	TMap<FKey, FKey> Translation;
	Translation.Add(EKeys::W, EKeys::Comma);
	Translation.Add(EKeys::S, EKeys::O);
	Translation.Add(EKeys::D, EKeys::E);        // ⭐ D lands on E...
	Translation.Add(EKeys::E, EKeys::Period);   // ⭐ ...while E is itself moving to Period
	// A is deliberately absent: identity on Dvorak.

	int32 FirstPassCount = -1;
	const bool bFirstApplied = FSiegeKeyboardLayoutStatics::RetargetContextKeys(
		Scratch.Source.Get(), Scratch.Duplicate.Get(), Translation, FirstPassCount);

	TestTrue(TEXT("The first retarget applied"), bFirstApplied);
	TestEqual(TEXT("Four of the five mappings moved (A is identity)"), FirstPassCount, 4);

	const TArray<FEnhancedActionKeyMapping>& DupMappings = Scratch.Duplicate->GetMappings();
	if (DupMappings.Num() != 5)
	{
		AddError(TEXT("The duplicate does not carry the five mappings it was built with."));
		return false;
	}

	// ── CLAIM (a): NO CASCADE ───────────────────────────────────────────────────────────
	TestTrue(FString::Printf(TEXT("W -> Comma (got %s)"), *Describe(DupMappings[0].Key)), DupMappings[0].Key == EKeys::Comma);
	TestTrue(FString::Printf(TEXT("A stays A (got %s)"),  *Describe(DupMappings[1].Key)), DupMappings[1].Key == EKeys::A);
	TestTrue(FString::Printf(TEXT("S -> O (got %s)"),     *Describe(DupMappings[2].Key)), DupMappings[2].Key == EKeys::O);
	TestTrue(FString::Printf(TEXT("⭐ D -> E and STOPS THERE — not D -> E -> Period (got %s)"),
		*Describe(DupMappings[3].Key)), DupMappings[3].Key == EKeys::E);
	TestTrue(FString::Printf(TEXT("E -> Period (got %s)"), *Describe(DupMappings[4].Key)), DupMappings[4].Key == EKeys::Period);

	// Snapshot everything the second pass must not disturb.
	TArray<FKey> KeysAfterFirstPass;
	for (const FEnhancedActionKeyMapping& Mapping : DupMappings)
	{
		KeysAfterFirstPass.Add(Mapping.Key);
	}
	const int32 ModifierCountAfterFirstPass = DupMappings[0].Modifiers.Num();
	const UInputModifier* ModifierAfterFirstPass =
		ModifierCountAfterFirstPass > 0 ? DupMappings[0].Modifiers[0].Get() : nullptr;

	// ── CLAIM (b): THE SECOND CALL IS A NO-OP ───────────────────────────────────────────
	int32 SecondPassCount = -1;
	const bool bSecondApplied = FSiegeKeyboardLayoutStatics::RetargetContextKeys(
		Scratch.Source.Get(), Scratch.Duplicate.Get(), Translation, SecondPassCount);

	TestTrue(TEXT("The second retarget also applies (it is a legal repeat, not an error)"), bSecondApplied);
	// ⚠️ The count is derived from SOURCE vs translation, so it is the SAME 4, not 0. A 0 here
	// would mean the function had started reading its own output — the exact bug this guards.
	TestEqual(TEXT("The second pass reports the same 4 — it re-derives from Source, it does not diff"),
		SecondPassCount, FirstPassCount);

	for (int32 Index = 0; Index < DupMappings.Num(); ++Index)
	{
		TestTrue(FString::Printf(TEXT("⭐ Mapping %d is UNCHANGED by the second pass (%s)"),
			Index, *Describe(KeysAfterFirstPass[Index])),
			DupMappings[Index].Key == KeysAfterFirstPass[Index]);
	}

	// ⭐ Specifically: D did NOT walk on to Period, which is what a target-reading
	// implementation would do on exactly this call.
	TestTrue(FString::Printf(TEXT("⭐ D is still E after the repeat — it did not compound to Period (got %s)"),
		*Describe(DupMappings[3].Key)), DupMappings[3].Key == EKeys::E);

	// And the modifiers are still the same OBJECTS: `.Key` really is the only field written.
	TestEqual(TEXT("The repeat did not touch the modifier array"),
		DupMappings[0].Modifiers.Num(), ModifierCountAfterFirstPass);
	if (ModifierCountAfterFirstPass > 0)
	{
		TestTrue(TEXT("⛔ The repeat did not re-instance the modifiers — same objects, untouched"),
			DupMappings[0].Modifiers[0].Get() == ModifierAfterFirstPass);
	}

	// ── AND THE WHOLESALE REFUSAL, since it shares this function's contract ─────────────
	// A length mismatch means Target is not the duplicate the contract assumes.
	{
		FScratchContexts Mismatched = MakeScratchContexts({ EKeys::W }, /*bAttachModifiers*/ false);
		if (Mismatched.Source.IsValid())
		{
			int32 RefusedCount = -1;
			const bool bRefusedApply = FSiegeKeyboardLayoutStatics::RetargetContextKeys(
				Scratch.Source.Get(), Mismatched.Source.Get(), Translation, RefusedCount);

			TestFalse(TEXT("A mapping-count mismatch is REFUSED WHOLESALE"), bRefusedApply);
			TestEqual(TEXT("...and OutNumRetargeted is 0 on the refusal path"), RefusedCount, 0);
			if (Mismatched.Source->GetMappings().Num() == 1)
			{
				TestTrue(TEXT("...and the refused target was not written at all"),
					Mismatched.Source->GetMappings()[0].Key == EKeys::W);
			}
		}
	}

	// Null guards, from the same refusal ladder (`KBD-§5`).
	{
		int32 NullCount = -1;
		TestFalse(TEXT("A null Source is refused"),
			FSiegeKeyboardLayoutStatics::RetargetContextKeys(nullptr, Scratch.Duplicate.Get(), Translation, NullCount));
		TestEqual(TEXT("...with OutNumRetargeted set to 0"), NullCount, 0);

		NullCount = -1;
		TestFalse(TEXT("A null Target is refused"),
			FSiegeKeyboardLayoutStatics::RetargetContextKeys(Scratch.Source.Get(), nullptr, Translation, NullCount));
		TestEqual(TEXT("...with OutNumRetargeted set to 0"), NullCount, 0);
	}

	return true;
}

// ════════════════════════════════════════════════════════════════════════════════════════
//  TEST 7 — Siegebound.Input.LiveResolverMatchesRuntime
// ════════════════════════════════════════════════════════════════════════════════════════

/**
 *  ⚠️ THE ONLY TEST THAT READS THE REAL MACHINE — AND IT LOGS RATHER THAN FAILS.
 *
 *  ⛔ WHY IT CANNOT BE A HARD ASSERT, STATED BECAUSE THE OMISSION LOOKS LIKE COWARDICE AND IS
 *  NOT: `ResolveKeyFromCodes` forwards to `FInputKeyManager::GetKeyFromCodes`, whose
 *  `KeyMapVirtualToEnum` is REBUILT FROM THE ACTIVE OS LAYOUT on every WM_INPUTLANGCHANGE
 *  (SlateApplication.cpp:5138-5141 -> InitKeyMappings, InputCoreTypes.cpp:1552-1588), and
 *  whose platform key map differs per OS. ⇒ Its answers are a property of THE MACHINE THE
 *  TEST IS RUNNING ON, not of our code.
 *
 *  ⚖️ AND THE DECIDING ARGUMENT: Jonathan runs Dvorak. A hard assert here would go RED on the
 *  one machine this entire feature exists to serve, in a suite whose whole promise is that it
 *  is green everywhere. A red that means "your keyboard is unusual" is not a test result — it
 *  is a false alarm that trains people to ignore the suite. So this test reports and never
 *  fails, and the real verification of the live path is TASK-515: Jonathan's own PIE check.
 *
 *  ✅ It is still worth running. The two pairs below SHOULD resolve identically on every
 *  Windows host, because both land in `KeyMapCharToEnum`, which is built from the STATIC
 *  `GetStandardPrintableKeyMap` table (GenericPlatformInput.cpp:5-95) and does not vary with
 *  the layout. A mismatch is genuine news about the engine, and the log says so.
 *
 *  ⛔ DO NOT ADD PROBES WITH UNKNOWN CHARACTERS TO THIS TEST. `GetKeyFromCodes` SYNTHESIZES
 *  AND PERMANENTLY REGISTERS a new FKey for any CharCode > 32 it cannot name
 *  (InputCoreTypes.cpp:1604-1610) — a test that swept a character range would pollute the
 *  editor's global key registry for the rest of the session.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeKeyboardLayoutLiveResolverTest,
	"Siegebound.Input.LiveResolverMatchesRuntime",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeKeyboardLayoutLiveResolverTest::RunTest(const FString& Parameters)
{
	using namespace SiegeKeyboardLayoutTestUtils;

	struct FSiegeLiveCase
	{
		uint32      VirtualKey;
		uint32      CharCode;
		const FKey& Expected;
		const TCHAR* Description;
	};

	const FSiegeLiveCase LiveCases[] =
	{
		{ static_cast<uint32>(TEXT('W')), static_cast<uint32>(TEXT('W')), EKeys::W,
		  TEXT("a US-QWERTY W press ('W', 'W')") },
		{ 0xBC, static_cast<uint32>(TEXT(',')), EKeys::Comma,
		  TEXT("what the QWERTY-W position sends on Dvorak (VK_OEM_COMMA, ',')") }
	};

	int32 MatchCount = 0;
	for (const FSiegeLiveCase& Case : LiveCases)
	{
		const FKey Resolved = FSiegeKeyboardLayoutStatics::ResolveKeyFromCodes(Case.VirtualKey, Case.CharCode);

		if (Resolved == Case.Expected)
		{
			++MatchCount;
			AddInfo(FString::Printf(TEXT("LIVE RESOLVER OK: %s resolves to %s, as expected."),
				Case.Description, *Describe(Case.Expected)));
		}
		else
		{
			// ⛔ AddInfo, NOT AddError / AddWarning — a warning still colours the run and would
			// train people to ignore it. This is an observation about the host, not a defect.
			AddInfo(FString::Printf(
				TEXT("LIVE RESOLVER DIFFERS (NOT A FAILURE): %s resolved to %s, not %s. ")
				TEXT("FInputKeyManager's virtual-key map is rebuilt from the ACTIVE OS layout, so this reports THIS MACHINE, not the code. ")
				TEXT("The pure logic is covered by the six tests above; the live path is verified by TASK-515 on Jonathan's Dvorak machine."),
				Case.Description, *Describe(Resolved), *Describe(Case.Expected)));
		}
	}

	AddInfo(FString::Printf(TEXT("LIVE RESOLVER SUMMARY: %d of %d probes matched the US-QWERTY expectation on this host."),
		MatchCount, static_cast<int32>(UE_ARRAY_COUNT(LiveCases))));

	// ── THE ONE HARD CLAIM THAT IS TRUE ON EVERY HOST ───────────────────────────────────
	// ⛔ `KBD-§5`: nothing this feature produces may be EKeys::Invalid. An unresolvable pair
	// must come back as Invalid so BuildTranslationMap can DROP it — the danger is a resolver
	// that invents an answer. VK 0 with CharCode 0 is not a key on any layout, and CharCode 0
	// is below the > 32 threshold that would trigger the synthesis path, so this is layout-
	// and platform-independent.
	const FKey Unresolvable = FSiegeKeyboardLayoutStatics::ResolveKeyFromCodes(0, 0);
	TestTrue(FString::Printf(TEXT("An unresolvable (0, 0) pair comes back as EKeys::Invalid, never invented (got %s)"),
		*Describe(Unresolvable)), Unresolvable == EKeys::Invalid);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
