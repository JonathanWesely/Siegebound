// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

// FKey. It is held BY VALUE in FSiegePositionalKeyProbe, returned by value from
// ResolveKeyFromCodes, and is both the key AND the value type of TMap<FKey, FKey> — so the
// COMPLETE type is required here, not a forward declaration, and it is included explicitly
// rather than inherited transitively (complete-type include law). The same header carries
// EKeys and FInputKeyManager, which the .cpp uses.
#include "InputCoreTypes.h"

// TFunctionRef, the FSiegeKeyResolver alias below.
#include "Templates/Function.h"

// Only ever held/passed as a pointer in this header — the SiegeCombatStatics.h precedent.
// The .cpp includes "InputMappingContext.h" and "EnhancedActionKeyMapping.h" for the
// complete types it actually dereferences (complete-type include law, TASK-110).
class UInputMappingContext;

/**
 *  One physical key position: what QWERTY calls it, and what the ACTIVE layout yields there.
 *
 *  ⛔ PINNED — CONVENTIONS `KBD-§8`. Fields, order and types are the batch's link contract.
 */
struct FSiegePositionalKeyProbe
{
	FKey   QwertyKey;        // FKey this position carries on US-QWERTY, e.g. EKeys::W
	uint32 ScanCode   = 0;   // scan-code set 1, e.g. 0x11
	uint32 VirtualKey = 0;   // VK the ACTIVE layout yields here; 0 == probe failed
	uint32 CharCode   = 0;   // MapVirtualKeyEx(VK, MAPVK_VK_TO_CHAR, hkl) — dead-key bit UNMASKED
};

/**
 *  The injectable seam that lets every test run on a QWERTY machine.
 *
 *  ⭐ This alias is the whole reason BuildTranslationMap is testable: an automation test
 *  injects a fake resolver carrying US-Dvorak VK/CharCode pairs and asserts a Dvorak result
 *  on a US-QWERTY host, with no Dvorak hardware anywhere (TASK-510).
 *
 *  ⚠️ TFunctionRef deliberately does NOT coerce a function type to a pointer (Function.h:571),
 *  so bind it to the FUNCTION, not to a temporary function pointer:
 *      ✓ BuildTranslationMap(Probes, FSiegeKeyboardLayoutStatics::ResolveKeyFromCodes, Map);
 *      ✗ ...&FSiegeKeyboardLayoutStatics::ResolveKeyFromCodes  (a prvalue pointer)
 */
using FSiegeKeyResolver = TFunctionRef<FKey(uint32 VirtualKey, uint32 CharCode)>;

/**
 *  ═══ Siegebound positional-keyboard-layout statics (TASK-509, KEYBOARD-LAYOUT batch) ═══
 *
 *  Jonathan types on Dvorak. On Windows the FKey the engine delivers is LAYOUT-DEPENDENT:
 *  the physical key at the QWERTY-W position arrives as EKeys::Comma on Dvorak, so every
 *  letter binding in IMC_Hero (WASD, Q rally, T/R/E/F/C unit commands) is in the wrong
 *  PHYSICAL place. This library converts a set of scan-code probes into a
 *  `QWERTY-FKey -> active-layout-FKey` translation, and applies that translation to a
 *  DUPLICATE of the mapping context.
 *
 *  Not a UObject / not reflected: a plain static library, so there is no BeginPlay, no GC
 *  surface, and NO Build.cs change — InputCore and EnhancedInput are already public
 *  dependencies (GitClaudeUnrealTest.Build.cs:15-16). Precedent: FSiegeCombatStatics
 *  (SiegeCombatStatics.h:23).
 *
 *  ⛔ FULLY PLATFORM-AGNOSTIC — THERE IS NOT ONE Win32 SYMBOL IN THIS PAIR. The
 *  MapVirtualKeyEx/GetKeyboardLayout probe that FILLS FSiegePositionalKeyProbe::VirtualKey
 *  and ::CharCode lives inside `#if PLATFORM_WINDOWS` in USiegeKeyboardLayoutSubsystem's
 *  .cpp (TASK-511). Everything here compiles and tests on every platform (CONVENTIONS
 *  `KBD-§6`). On a platform with no probe the array simply carries VirtualKey == 0
 *  everywhere, BuildTranslationMap returns 0, and the game behaves exactly as it does today.
 *
 *  ⚠️ FSiegePositionalKeyProbe and FSiegeKeyResolver share this header ON PURPOSE. They are
 *  pure data types forming ONE concept with the statics that consume them — the standing
 *  `TeamId.h` exception to one-class-per-header (CONVENTIONS "C++" bullet 3, restated in
 *  `KBD-§7`). ⛔ This is NOT a one-class-per-header violation and must not be read as one.
 *
 *  ⛔⛔ THE TWO SENTENCES THAT MUST BE READ BEFORE TOUCHING RetargetContextKeys — BOTH
 *      DESCRIBE BUGS THAT ARE INVISIBLE IN REVIEW:
 *
 *  (a) SIMULTANEOUS SUBSTITUTION, NOT IN-PLACE TRANSLATION. On Dvorak the translation map
 *      holds `D -> E` AND `E -> Period` at the same time. RetargetContextKeys re-derives
 *      EVERY value from the pristine `Source` array BY INDEX, never reading the target it is
 *      writing — that is what makes the substitution SIMULTANEOUS, so translations cannot
 *      cascade (`D -> E -> Period`) and cannot compound on a repeat call. Reading the target
 *      instead would produce a plausible-looking map and a hero that moves sideways.
 *
 *  (b) ⛔ ONLY `.Key` IS EVER ASSIGNED — NEVER Modifiers, NEVER Triggers, NEVER Action,
 *      NEVER PlayerMappableKeySettings, and NEVER the mappings array itself. THE PRECEDENT
 *      IS THIS REPO'S OWN AND IT COST A PLAYTEST: `.claude/pipeline/handoffs/
 *      TASK-445-artist.md:95-105` — rewriting the mappings array SILENTLY
 *      DEFAULT-CONSTRUCTED the instanced SwizzleAxis/Negate modifiers, BREAKING WASD AND
 *      INVERTING MOUSE-LOOK WHILE THE PROPERTY TABLE STILL READ CORRECT. The readback
 *      passed. That is why this is a law and not a code comment (CONVENTIONS `KBD-§1`).
 *      ⛔ UInputMappingContext::MapKey / UnmapKey / UnmapAll are BANNED with it:
 *      MapKey APPENDS a mapping from the 2-arg ctor with EMPTY Modifiers and Triggers
 *      (InputMappingContext.cpp:154-158) — TASK-445's failure rewritten in C++. The
 *      supported mutation path is GetMapping(Index) (InputMappingContext.h:220 — public,
 *      non-const ref, and NOT WITH_EDITOR-guarded), followed by
 *      UEnhancedInputLibrary::RequestRebuildControlMappingsUsingContext by the caller.
 *
 *  ⛔ FAIL-SAFE LAW (`KBD-§5`): NEVER nullptr, NEVER EKeys::Invalid, NEVER a partially
 *  retargeted context. The worst outcome anything here may produce is "the game behaves
 *  exactly as it did yesterday" — an unresolvable key keeps its SOURCE value, and a refused
 *  retarget writes nothing at all.
 *
 *  ⛔ EVERY SIGNATURE BELOW IS PINNED CHARACTER-FOR-CHARACTER IN CONVENTIONS `KBD-§8`.
 *  UBT compiles the whole module and TASK-510's tests plus TASK-511's subsystem link against
 *  this list — "improving" a shape here breaks another agent's build and is an automatic QA
 *  FAIL. Change the pin first, or not at all.
 *
 *  M8 DECLARATION (verbatim, `KBD-§10`): adds no replicated property, no new replicated
 *  class, no new relevancy tier. This is client-local derived state — the layout is a
 *  property of the OS the process is running on, so in a listen-server match the host and
 *  each joining client resolve their OWN layout, which is the only correct behaviour.
 *
 *  QA gate: TASK-513 (`.claude/pipeline/qa/TASK-513-keyboard-layout.md`).
 */
class GITCLAUDEUNREALTEST_API FSiegeKeyboardLayoutStatics
{
public:

	/**
	 *  Returns the hand-authored QWERTY letter-position table: ALL 26 letters,
	 *  EKeys::A..EKeys::Z, each paired with its scan-code set 1 code.
	 *
	 *  VirtualKey and CharCode are left 0 — this function knows nothing about the active
	 *  layout. The platform probe (TASK-511) fills those two fields in place, and a probe it
	 *  could not answer is LEFT at VirtualKey == 0, which BuildTranslationMap skips.
	 *
	 *  ⭐ ALL 26, not just the 10 the game binds today (W/A/S/D/Q/T/R/E/F/C). It costs
	 *  nothing and it removes the "remember to update the table when you add a key" footgun,
	 *  which is exactly the debt that surfaces as a mystery input bug six months later
	 *  (CONVENTIONS `KBD-§4`).
	 *
	 *  ⚠️ ORDER IS ALPHABETICAL, A..Z — NOT scan-code order, and that is load-bearing:
	 *  USiegeKeyboardLayoutSubsystem::DescribeActiveTranslation is pinned to emit its
	 *  entries in "source-key order as returned by GetQwertyLetterScanCodes()" (`KBD-§8`),
	 *  so reordering this table silently changes a string another task asserts byte-for-byte.
	 *
	 *  ⛔ Digits, punctuation, modifiers, Space/Enter/Escape and the mouse are DELIBERATELY
	 *  absent (`KBD-§4`) — Dvorak's number row is identical anyway and positionally remapping
	 *  digits would actively hurt AZERTY. "The hotkeys weren't remapped" is not a defect.
	 */
	static TArray<FSiegePositionalKeyProbe> GetQwertyLetterScanCodes();

	/**
	 *  Resolves a (virtual key, character code) pair to the FKey the engine itself would
	 *  deliver for it — a thin wrapper on FInputKeyManager::Get().GetKeyFromCodes
	 *  (InputCoreTypes.h:857-858 — `KBD-§8` cites :857, the doc comment; the declaration
	 *  itself is :858. Module InputCore, already a public dependency).
	 *
	 *  ⭐ THIS IS THE ANSWER TO "why not just hardcode a Dvorak lookup table?". Windows
	 *  hands FSlateApplication::OnKeyDown exactly these two numbers
	 *  (WindowsApplication.cpp:3317/3321 -> SlateApplication.cpp:4953-4957), which Slate
	 *  passes to this same resolver. Calling the ENGINE'S OWN resolver with the SAME TWO
	 *  NUMBERS means the translation table agrees with runtime BY CONSTRUCTION — including
	 *  on layouts nobody on this project has ever tested, and including after Slate
	 *  re-initialises its key maps on WM_INPUTLANGCHANGE. A hand-written table would be a
	 *  second source of truth that drifts.
	 *
	 *  @return the resolved FKey, or EKeys::Invalid when the pair maps to nothing.
	 *          ⛔ Callers must DROP an EKeys::Invalid result, never store it (`KBD-§5`).
	 *
	 *  ⚠️ Game thread only in practice: FInputKeyManager::Get() lazily constructs its
	 *  singleton and GetKeyFromCodes may synthesize-and-register a new FKey for an unknown
	 *  character (InputCoreTypes.cpp:1604-1610), mutating shared state through a const_cast.
	 *  Every shipped caller is the subsystem's probe, which asserts IsInGameThread().
	 */
	static FKey ResolveKeyFromCodes(uint32 VirtualKey, uint32 CharCode);

	/**
	 *  PURE. Builds the `QWERTY FKey -> active-layout FKey` translation from a filled probe
	 *  array, resolving each probe through the injected Resolver.
	 *
	 *  OutTranslation is fully REPLACED (reset first), so a caller may reuse one map across
	 *  repeated layout probes without stale entries surviving.
	 *
	 *  A probe is SKIPPED — meaning that letter simply gets NO map entry and therefore keeps
	 *  its SOURCE key — when any of these holds (`KBD-§5`; ⛔ none of them may ever put
	 *  EKeys::Invalid into the map):
	 *  (listed in the order the implementation evaluates them, so header and .cpp read alike)
	 *    1. QwertyKey is not a valid FKey — a malformed table row (defensive only);
	 *    2. VirtualKey == 0            — the platform probe failed for that position;
	 *    3. the same QWERTY source appears twice in Probes — first row wins, so a duplicated
	 *       table row can never make the result depend on iteration order;
	 *    4. the Resolver returns EKeys::Invalid or an unregistered FKey;
	 *    5. IDENTITY — the position already yields its QWERTY key, so there is nothing to
	 *       translate. ⭐ The key is still CLAIMED (see below): an identity probe holds its
	 *       own key just as firmly as a translated one does;
	 *    6. INJECTIVITY — the target FKey was already claimed by an earlier probe. ⛔ Two
	 *       actions must never collide onto one physical key. First probe in A..Z order
	 *       wins; the later one keeps its source key.
	 *
	 *  ⚠️ RESIDUAL, STATED RATHER THAN HIDDEN: the claim set is built in one forward pass, so
	 *  it cannot retract a translation that was already accepted onto a key whose OWN probe
	 *  later turns out to be unusable (position A resolves to VK_B while position B's probe
	 *  fails). That needs a layout where the OS answers for one letter position and not for
	 *  another that maps onto it; no real layout does, every real Latin layout being a
	 *  bijection over the 26 letter positions. The outcome if it ever happened is one
	 *  double-bound key — still never nullptr, never EKeys::Invalid, never a partial context.
	 *
	 *  @param Probes         the table from GetQwertyLetterScanCodes() with VirtualKey /
	 *                        CharCode filled in by the platform probe
	 *  @param Resolver       normally ResolveKeyFromCodes; a fake in automation tests
	 *  @param OutTranslation RESET then filled; contains ONLY non-identity entries
	 *  @return the number of non-identity entries written — i.e. OutTranslation.Num().
	 *          ⭐ 0 means the host layout is POSITIONALLY QWERTY and the caller must do
	 *          nothing at all: no duplicate, no allocation, hand the source context back.
	 */
	static int32 BuildTranslationMap(const TArray<FSiegePositionalKeyProbe>& Probes,
	                                 FSiegeKeyResolver Resolver,
	                                 TMap<FKey, FKey>& OutTranslation);

	/**
	 *  PURE. Rewrites Target's mapping keys so that each mapping sits on the ACTIVE-layout
	 *  key occupying the same PHYSICAL position its QWERTY key occupies.
	 *
	 *  ⛔ THIS IS THE FUNCTION CONVENTIONS `KBD-§1` IS ABOUT — read (a) and (b) in the class
	 *  comment above before editing it. Only `.Key` is assigned, via Target->GetMapping(i),
	 *  and every value is re-derived from Source->GetMappings() BY INDEX.
	 *
	 *  ⚠️ PRECONDITION: Target is a DISTINCT DuplicateObject of Source (that is TASK-511's
	 *  only use). That is what makes the index-by-index derivation meaningful and what makes
	 *  the single profile-override check on Source sufficient. ⛔ Passing the SAME context as
	 *  both arguments is a contract violation, not a supported call: a single such call still
	 *  produces the right answer (each index is read then written before the next is touched)
	 *  but the idempotence guarantee below is lost, because the "pristine source" would have
	 *  been overwritten. Source is const throughout: ⛔ THE ONLY NON-CONST
	 *  UInputMappingContext IN THIS ENTIRE FEATURE IS THE DUPLICATE, which is what makes it
	 *  structurally impossible for PIE to dirty the loaded IMC_Hero asset.
	 *
	 *  ⭐ IDEMPOTENT BY CONSTRUCTION. Because nothing is ever read back out of Target, calling
	 *  this twice with the same arguments produces the same result the first call did — the
	 *  subsystem relies on that to re-target its CACHED duplicate in place when the OS layout
	 *  changes mid-session, without ever handing AHeroCharacter a new pointer.
	 *
	 *  ⛔ REFUSES WHOLESALE — returns false, writes NOTHING to either context, and leaves
	 *  OutNumRetargeted at 0 — when:
	 *    - Source or Target is null;
	 *    - Source->GetProfilesWithOverridenMappings().Num() > 0. ⚠️ THIS IS THE NON-OBVIOUS
	 *      ONE AND IT IS WHY THIS FUNCTION HAS A false RETURN AT ALL: profile overrides live
	 *      in MappingProfileOverrides (InputMappingContext.h:109-110), a SEPARATE array that
	 *      GetMapping() CANNOT REACH. Retargeting would silently cover the defaults and miss
	 *      the overrides — a half-translated context. Refusing loudly beats that (`KBD-§5`);
	 *    - the two mapping arrays differ in length, which means Target is not the duplicate
	 *      this contract assumes and index-by-index derivation would be meaningless.
	 *  ⛔ Every refusal is checked BEFORE the first write, so "changes nothing" is structural.
	 *  The caller's fail-safe on false is to hand back the untranslated Source context.
	 *
	 *  @param Source            the pristine context; read-only, the authority for every value
	 *  @param Target            the transient duplicate that receives the retargeted keys
	 *  @param Translation       from BuildTranslationMap; an empty map is legal and yields a
	 *                           faithful key-for-key copy of Source's keys (0 retargeted)
	 *  @param OutNumRetargeted  MAPPINGS whose key actually changed — not distinct keys, so
	 *                           one physical key bound to two actions counts twice. Set to 0
	 *                           before anything else, on every path.
	 *  @return true if the retarget was applied (even if it changed nothing); false if refused.
	 */
	static bool RetargetContextKeys(const UInputMappingContext* Source,
	                                UInputMappingContext* Target,
	                                const TMap<FKey, FKey>& Translation,
	                                int32& OutNumRetargeted);
};
