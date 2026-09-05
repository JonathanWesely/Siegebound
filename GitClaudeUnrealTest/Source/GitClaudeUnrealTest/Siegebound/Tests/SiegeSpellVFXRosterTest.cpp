// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Containers/Array.h"
#include "Containers/Map.h"
#include "Containers/Set.h"
#include "Containers/UnrealString.h"
#include "Engine/DataTable.h"
#include "HAL/FileManager.h"   // TASK-1025: IFileManager::FindFilesRecursive — the composer-mirror pin reads SpellLibrary.cpp off disk
#include "Misc/FileHelper.h"   // TASK-1025: the CSV roster and the composer-mirror pin both read shipped files
#include "Misc/Paths.h"        // TASK-1025: FPaths::ProjectDir()
#include "NiagaraSystem.h"     // TASK-1025: UNiagaraSystem — the TYPE the shipped spawn requires; a loadable non-Niagara asset at the path is still a silent no-op
#include "Siegebound/CardRow.h"
#include "Siegebound/SiegePlayerController.h"
#include "UObject/Class.h"
#include "UObject/SoftObjectPath.h"
#include "UObject/SoftObjectPtr.h"
#include "UObject/UnrealType.h"
#include "UObject/UObjectGlobals.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 *  ═══ THE SPELL-VFX ROSTER GATE (TASK-1025; law: CONVENTIONS ⭐⭐ `SC-§75`(A),
 *      ⭐ `SC-§37`, ⭐ `SC-§65`, ⭐⭐ `SC-§77`, `SC-§45`, `SC-§39`, `SC-§38`,
 *      `SC-§40` cl. 10, `SC-§50` cl. 4, `SC-§51`, `SC-§54` cl. 3, `SHIP-§9`,
 *      `FOG-§9.8e`) ═══
 *
 *  ⛔⛔ WHY THIS FILE EXISTS — THE DEFECT, IN ONE PARAGRAPH, FROM `VID-006`:
 *  Jonathan cast `Fog` twice and `BrightSun` once and saw NOTHING. ⛔ THE CARDS
 *  WORKED. Gold was deducted every single time — 985→935, 940→891, 893→834,
 *  exactly −50/−50/−60 — never refunded, with no "Spell fizzled" toast anywhere in
 *  the footage. Under the shipped confirm contract (deduct → resolve → refund AND
 *  fizzle on false) a PERMANENT deduction with NO fizzle can only mean
 *  `USpellLibrary::ResolveSpell` returned TRUE. The resolve was correct, the state
 *  was written, and the player could not tell it apart from a dead click.
 *
 *  ⇒ ⛔⛔ THE CAUSE: `Content/VFX/` holds no `NS_Spell_Fog` and no
 *  `NS_Spell_BrightSun`, and `USpellLibrary`'s spawn of `/Game/VFX/NS_Spell_<CardID>`
 *  is NULL-SAFE AND LOG-ONCE. A successful cast therefore spawned NOTHING and was
 *  BYTE-FOR-BYTE INDISTINGUISHABLE FROM A CARD THAT NEVER FIRED.
 *
 *  ⚖️ ⭐ THE NULL-SAFE SPAWN IS CORRECT ENGINEERING AND IS NOT TOUCHED BY THIS ROW.
 *  VFX must never be able to fail a spell. ⛔ WHAT WAS MISSING IS A GATE THAT ASKS
 *  WHETHER THE THING THE PLAYER IS SUPPOSED TO **SEE** ACTUALLY EXISTS. This file
 *  is that gate, and it is the first one of its kind in the tree.
 *
 *  ── ⛔⛔⭐⭐ THIS GATE WAS **BORN RED**, AND ITS RED CLOSED **DURING AUTHORING**
 *     ⚠️⚠️ THE ONE PERISHABLE PARAGRAPH IN THIS FILE (`SC-§76`) — everything that
 *     can go false with the state of `Content/VFX/` is CONCENTRATED HERE, so its
 *     supersession is a SINGLE EDIT. ⛔ Do not scatter this claim. ─────────────
 *
 *  ⛔ WHEN THIS FILE WAS WRITTEN, `/Game/VFX/NS_Spell_Fog` AND
 *  `/Game/VFX/NS_Spell_BrightSun` DID NOT EXIST, and this test failed on exactly
 *  those two cards. ⛔⛔ THAT WAS CORRECT AND IS WHY THE ASSERTION BELOW WAS NEVER
 *  SOFTENED, DISABLED OR EXEMPTED: the repair is the ASSET, never a weakened
 *  assertion here (`SC-§50` cl. 4).
 *
 *  ⭐ MEASURED 2026-09-04 AT `1a457df`, ⛔ RE-MEASURED RATHER THAN RELAYED
 *  (`SC-§40` cl. 9): ⭐ `TASK-1024` LANDED BOTH SYSTEMS WHILE THIS FILE WAS BEING
 *  WRITTEN — `Content/VFX/NS_Spell_Fog.uasset` and
 *  `Content/VFX/NS_Spell_BrightSun.uasset` were created at 22:31 and are staged,
 *  and that row's own handoff records the editor handing back class `NiagaraSystem`
 *  for both composed paths. ⇒ ⛔ THE EXPECTED RED COUNT FOR THE WALK IS NOW **ZERO**,
 *  ⛔ NOT TWO, and a reviewer working from the dispatch brief would otherwise read a
 *  green bar as a softened gate. ⚠️ Loadability is what this gate MEASURES AT RUN
 *  TIME; the author asserted it from file presence and the artist's read-back, and
 *  ran no compile and no engine (`TASK-1025` fences it).
 *
 *  ⇒ ⛔⛔ THE FREE RED LASTED HOURS, WHICH IS THE ARGUMENT FOR THE SYNTHESIS AT THE
 *  FOOT OF THIS TEST RATHER THAN AN OBJECTION TO IT. `SHIP-§9c` cl. 1 calls a gate
 *  that has only ever been seen passing "a status line wearing a gate's clothes";
 *  ⛔ THE PERMANENT NEGATIVE CONTROL IS NOW THE ONLY THING ON THIS PAGE THAT PROVES
 *  THE PROBE CAN STILL ANSWER ABSENT — see THE SYNTHESIS below. ⛔ It is not
 *  decoration and it must not be deleted as redundant.
 *
 *  ⚠️ A REAL MISS EMITS AN ENGINE LOAD WARNING
 *  (`LogUObjectGlobals: Failed to find object …`) AND NO REAL CARD'S WARNING IS
 *  DECLARED TO THE HARNESS, DELIBERATELY. Declaring a shipped card's expected
 *  warning would read as an exemption for exactly the card this gate exists to
 *  name. ⛔ Only the SYNTHETIC control's warning is declared, by MY OWN synthetic
 *  CardID.
 *
 *  ── ⛔ WALK THE DATA, NEVER A CARD LIST (⭐⭐ `SC-§77`, ⛔ `SC-§75`(B)) ───────
 *
 *  ⛔ THERE IS NO HAND-WRITTEN SPELL LIST IN THIS FILE. A list is a COORDINATE; the
 *  data is the KEY. A hand-enumerated roster would have to be edited for every new
 *  spell by the person least likely to remember it — which is THE SAME FAILURE ONE
 *  LEVEL UP, and it is precisely how this defect escaped: nothing anywhere asked
 *  "does the asset for this new card exist?".
 *
 *  ⭐ THE ROSTER IS DERIVED FROM **TWO INDEPENDENT SOURCES** AND THE TWO ARE
 *  ASSERTED TO AGREE:
 *
 *   (1) THE RUNTIME AUTHORITY — the `UDataTable` that `ASiegePlayerController`'s own
 *       CDO points at, read by reflection (`CardTableAsset` is not public). This is
 *       the exact asset the spell path loads rows from, so a row that exists ONLY
 *       in the table is still walked. Spellhood here is `Row->SpellEffect != None`,
 *       because THAT is what `USpellLibrary::ResolveSpell` dispatches on and what
 *       can therefore reach the spawn — `SC-§37`: join on the category the CODE
 *       COMPUTES, not the field the DATA DECLARES.
 *   (2) THE SPREADSHEET OF RECORD — `Docs/Data/cards.csv`, keyed on `CardType`.
 *       ⚠️ ITS FIRST COLUMN HEADER IS **EMPTY** (`FOG-§9.8e`; it is the DataTable
 *       row-name column), so `CardID` is FIELD 0 and that emptiness is ASSERTED
 *       rather than assumed — a header that grew a name here would shift every
 *       field by one and yield a well-formed table of blanks.
 *
 *  ⛔ THE WALK IS OVER THE **UNION** of the two, so NEITHER source can hide a card
 *  from the gate: a spell authored in the CSV but never imported into the table is
 *  still asserted, and so is a spell that exists only in the table.
 *
 *  ⚠️ ONLY COLUMNS THAT SIT **BEFORE** `Notes` ARE READ OUT OF THE CSV, and that
 *  ordering is asserted by name. `Notes` is free text and may carry an embedded
 *  comma, which pushes every LATER column right — `SpellEffect` sits after `Notes`,
 *  so it is read from the DATATABLE (whose importer parses quoting correctly) and
 *  never from this file's own split. That is the same precondition
 *  `SiegeAssistantSelectionTest.cpp` asserts, for the same reason.
 *
 *  ── ⛔⛔ THE FAILURE MESSAGE IS THE DELIVERABLE, NOT THE ASSERTION ───────────
 *
 *  ⛔ EVERY RED NAMES THE `CardID` **AND** THE EXACT EXPECTED ASSET PATH **AND**
 *  THE FILE TO CREATE. ⇒ AN ARTIST CAN ACT ON A RED WITHOUT OPENING A SINGLE
 *  SOURCE FILE. That is what turns this from a test into a WORK ORDER, and it is
 *  why the message carries the `Content/VFX/NS_Spell_<CardID>.uasset` form as well
 *  as the `/Game/…` object path.
 *
 *  ⛔ AND IT IS AN **EXISTENCE** ASSERTION ON THE **NAMED** ASSET — never a bare
 *  non-emptiness or "something was produced" check. ⚖️ A LIE AND A TRUTH HAVE THE
 *  SAME LENGTH: this batch has twice found a test that passed ON the defect.
 *
 *  ── ⛔ TWO FINDINGS, TWO REPAIRS — REPORTED SEPARATELY ──────────────────────
 *
 *   (a) ABSENT — nothing loads at the composed path at all. Repair: AUTHOR THE
 *       ASSET.
 *   (b) WRONG TYPE — something DOES load there, but it is not a `UNiagaraSystem`.
 *       Repair: REPLACE THE IMPOSTOR, not author a second asset.
 *  ⛔ Collapsing them into "no VFX" is how (b) gets "fixed" by making an asset that
 *  cannot be saved where the impostor already sits. The shipped consumer cannot
 *  tell them apart — its typed soft pointer returns null for both — so this file
 *  classifies with an UNTYPED probe and then ASSERTS the two agree.
 *
 *  ── ⛔ THE ONE COMPOSER IN THE TREE, AND THIS FILE'S MIRROR OF IT ───────────
 *
 *  ⚠️ DECLARED RESIDUAL, STATED PLAINLY (`SC-§49`: understating is the safe
 *  direction). Unlike the card-art gate — which reads an AUTHORED `TSoftObjectPtr`
 *  cell and therefore composes nothing — THE SPELL VFX PATH IS **COMPOSED IN CODE**
 *  from the `CardID`, inside a file-local (anonymous-namespace) helper that no test
 *  can call. ⇒ this file MIRRORS those two lines rather than invoking them, and a
 *  mirror can drift.
 *
 *  ✅ THE MITIGATION, AND IT IS THE STRONGEST ONE AVAILABLE WITHOUT A REFACTOR:
 *  the mirror is PINNED AGAINST THE SHIPPED SOURCE. This test locates
 *  `Siegebound/SpellLibrary.cpp` BY FILENAME (a key, `SC-§38`/`SC-§77` — never a
 *  line number) and asserts that BOTH format literals it mirrors are still present
 *  in it. ⇒ if somebody re-shapes the spell VFX path, THIS GATE GOES RED SAYING SO
 *  instead of silently asserting a path the game no longer uses.
 *  ⛔ The pin is a source-text assertion and `SC-§75`(A) warns those are brittle;
 *  it earns its place here because the composed path is UNREACHABLE from a test by
 *  any other means, so there is nothing else to assert.
 *
 *  ── ⛔⛔ WHAT THIS GATE DOES **NOT** COVER — READ BEFORE TRUSTING A GREEN BAR ─
 *
 *  ⛔ (R1) IT DOES NOT ASSERT THE VFX **LOOKS LIKE ANYTHING**. An empty Niagara
 *      system that emits zero particles passes here. This gate answers "does the
 *      named asset exist and load as a Niagara system?", which is the exact
 *      question `VID-006` proved nobody was asking. The visual bar is the art
 *      lane's.
 *  ⛔ (R2) IT DOES NOT ASSERT THE VFX IS ACTUALLY **SPAWNED AT RUNTIME**. That
 *      requires a world and a resolved cast; the arms that `return` before the
 *      shared tail (a refused fog, a refused BrightSun) deliberately spawn nothing,
 *      and this file makes no claim about them.
 *  ⛔ (R3) IT READS THE **C++ CDO**. A Blueprint subclass of `ASiegePlayerController`
 *      that overrode `CardTableAsset` would be walked at its C++ default. The CSV
 *      cross-walk is the partial answer: a spell in the spreadsheet is asserted
 *      whatever the table says.
 *  ⛔ (R4) IT DOES NOT COVER **NON-SPELL** VFX (`NS_ChainZap`, `NS_CastleDebris`,
 *      `NS_RecallChannel`, the `MI_Spell_*` materials). Those have no `CardID` to
 *      derive from and no roster to walk; they need their own gate if they ever
 *      want one.
 *  ⛔ (R5) THE HERO-LINE DELIVERY SPAWNS THE SAME NAMED SYSTEM AT THE MUZZLE
 *      instead of at the reticle. Same asset, same name, so the roster claim is
 *      unaffected — but this file asserts nothing about WHERE it appears.
 *
 *  ── ⭐⭐ EVERYTHING IS DERIVED. NOTHING IS TRANSCRIBED (`SC-§40` cl. 10) ─────
 *
 *  ⛔ NO EXPECTED SPELL COUNT AND NO ROW COUNT APPEARS IN ANY ASSERTION OR ANY
 *  RUNTIME MESSAGE IN THIS FILE. `TestEqual(SpellCards, 7)` would turn this gate
 *  red on spell #8 while the roster is perfectly healthy, and the number would be
 *  bumped by the person least likely to re-read it — the `TASK-874` trap rebuilt
 *  inside its own cure. Every assertion pins a RELATIONSHIP.
 *
 *  ── ⛔⛔ THE SYNTHESIS — `SC-§54` cl. 3, AND WHY IT IS NOT OPTIONAL HERE ─────
 *
 *  ⛔ THE FREE RED ALREADY EXPIRED — see the ONE perishable paragraph above. Once
 *  every shipped card passes, this file has NO remaining evidence that its probe
 *  can answer ABSENT at all. ⇒ the block at the foot drives the miss FOR REAL,
 *  permanently, through the SAME probe the walk uses, on a MEASURED-ABSENT
 *  synthetic `CardID` — so the proof RE-RUNS ON EVERY PASS FOREVER instead of
 *  decaying into this comment. ⚖️ A GATE THAT CANNOT BE SHOWN TO FAIL IS A STATUS
 *  LINE, and the window in which this one could show it was hours wide.
 *
 *  ⛔⛔ NOT ONE BYTE of `Content/VFX/`, `Content/Data/DT_Cards.uasset` or
 *  `Docs/Data/cards.csv` is written, moved or renamed. The synthetic is a STRING
 *  built on this function's stack. (A build-master tried the move-the-asset method
 *  on a sibling gate; the permission system refused it, and correctly.)
 *
 *  ⛔ THE SYNTHETIC CARDID WAS **MEASURED** ABSENT, NOT ASSUMED (`SC-§40` cl. 10),
 *  repo-wide with `.git`, `Binaries`, `Intermediate`, `DerivedDataCache` and
 *  `Saved` excluded, on 2026-09-04 at `84eec02`, alongside live positive controls
 *  so a dead grep could not read as a clean zero (`SC-§39`):
 *      `XxNoSuchSpellXx`      0 file(s)   ⇐ THE SYNTHETIC USED HERE
 *      `NS_Spell_`           38 file(s)   (instrument positive control)
 *      `Fireball`            98 file(s)   (instrument positive control)
 *  ⛔ Zero hits in BOTH substring directions: it is a substring of nothing in the
 *  tree, and nothing in the tree is a substring of it — the trap that nearly caught
 *  `TASK-964`.
 *
 *  ── ⭐ THE POSITIVE CONTROL (`SC-§39`, and `TASK-1025` item (4) names it) ────
 *
 *  ⛔ `Fireball` — a known-good spell whose `NS_Spell_Fireball` is shipped. WITHOUT
 *  IT, A GATE THAT REDS ON EVERYTHING IS INDISTINGUISHABLE FROM ONE THAT WORKS:
 *  "these particular cards are missing their VFX" and "the loader is broken and
 *  EVERY card looks missing" produce the same red bar. ⇒ the control is what
 *  separates them, ON THE VERY FIRST RUN — which is not a hypothetical worry on a
 *  file whose first run was expected to be red.
 *
 *  ── MECHANISM — read-only; ⛔ no world, ⛔ no PIE, ⛔ no spawns, ⛔ zero writes
 */

namespace SiegeSpellVFXRosterTestFixture
{
	/** The automation-test name, reused as `FindRow`'s context string so a table warning names its caller. */
	static const TCHAR* const TestContext = TEXT("Siegebound.CardRoster.EverySpellCardHasItsCastVFXSystem");

	/** The spreadsheet of record, relative to the project directory. */
	static const TCHAR* const CardsCsvPath = TEXT("Docs/Data/cards.csv");

	/**
	 *  ⭐ THE POSITIVE CONTROL (`SC-§39`) — a known-present spell whose cast VFX must
	 *  resolve. If `Fireball` is ever retired this control goes red and a new one must
	 *  be chosen; that cost is deliberate and is what a control is for. ⛔ It must NOT
	 *  be "fixed" by deleting the control.
	 */
	static const TCHAR* const PositiveControlCardID = TEXT("Fireball");

	/**
	 *  ⛔ THE SYNTHETIC, MEASURED-ABSENT `CardID` (`SC-§39`, `SC-§40` cl. 10). It is a
	 *  CardID rather than a path, on purpose: pushing it through the SAME composer the
	 *  walk uses means the negative control also proves the COMPOSER still runs, and it
	 *  needs no `/Game/…` literal of its own. See the file header for the census and
	 *  its positive controls.
	 */
	static const TCHAR* const SyntheticAbsentCardID = TEXT("XxNoSuchSpellXx");

	/**
	 *  ⭐⭐ THE COMPOSER, MIRRORED FROM `USpellLibrary`'s file-local `SpawnSpellVFX` —
	 *  its two lines, in its order, with its format literals. ⛔ NOT tidied into one
	 *  `Printf`: the shipped code builds the ASSET NAME first and then repeats it on
	 *  both sides of the dot, and a single-expression "simplification" here would stop
	 *  mirroring the thing it claims to mirror.
	 *
	 *  ⚠️ THIS IS THIS FILE'S ONE DECLARED RESIDUAL — the shipped helper lives in an
	 *  anonymous namespace and cannot be called. The pin in `PinComposerAgainstSource`
	 *  is what stops the mirror drifting silently.
	 */
	static FString ComposeSpellVFXObjectPath(const FString& CardID)
	{
		const FString AssetName = FString::Printf(TEXT("NS_Spell_%s"), *CardID);
		return FString::Printf(TEXT("/Game/VFX/%s.%s"), *AssetName, *AssetName);
	}

	/** The `Content/…uasset` form of the same asset — the half an ARTIST acts on. Derived from the same composer, never typed twice. */
	static FString ComposeSpellVFXContentFile(const FString& CardID)
	{
		return FString::Printf(TEXT("Content/VFX/NS_Spell_%s.uasset"), *CardID);
	}

	/**
	 *  The three outcomes of resolving a spell's cast VFX. ⛔ ABSENT and WRONG TYPE are
	 *  TWO different findings with TWO different repairs (author it vs replace it), and
	 *  the shipped consumer — a typed soft pointer — cannot tell them apart, because it
	 *  returns null for both. This file can, and says which.
	 */
	enum class ESpellVFXOutcome : uint8
	{
		Present,
		Absent,
		WrongType
	};

	/** The outcome NAME for human-readable messages — every enumerator named, no `default:` label (`SC-§51` cl. 5, `SC-§75`(B1)'s spirit). */
	static const TCHAR* SpellVFXOutcomeSymbol(ESpellVFXOutcome Outcome)
	{
		switch (Outcome)
		{
		case ESpellVFXOutcome::Present:   return TEXT("PRESENT");
		case ESpellVFXOutcome::Absent:    return TEXT("ABSENT");
		case ESpellVFXOutcome::WrongType: return TEXT("WRONG TYPE");
		}

		// Reached only by a value the enum does not name. Reported, never silently
		// folded into one of the three above.
		return TEXT("<ESpellVFXOutcome value nobody named>");
	}

	/**
	 *  ⭐⭐ THE PROBE. ⛔ ONE FUNCTION, TWO CALLERS — the roster walk AND the synthesis.
	 *  A synthesis driving a private copy would be a test of its own scratch work.
	 *
	 *  ⛔ IT RUNS **BOTH** EXPRESSIONS AND HANDS BACK BOTH ANSWERS:
	 *   • `OutConsumerSystem` — the shipped consumer's OWN expression, character for
	 *     character: `TSoftObjectPtr<UNiagaraSystem>(FSoftObjectPath(Path)).LoadSynchronous()`.
	 *     ⛔ THIS is the value the game's spawn actually branches on.
	 *   • the returned outcome — classified with an UNTYPED `TryLoad()`, which is the
	 *     only way to separate "nothing is there" from "something is there and it is
	 *     the wrong kind of thing".
	 *  ⇒ the caller asserts the two AGREE, so the classifier can never quietly diverge
	 *    from the predicate the player is subject to.
	 */
	static ESpellVFXOutcome ProbeSpellVFX(const FString& ObjectPath, UNiagaraSystem*& OutConsumerSystem, FString& OutFoundClassName)
	{
		OutConsumerSystem = nullptr;
		OutFoundClassName.Reset();

		const FSoftObjectPath SoftPath(ObjectPath);

		// The UNTYPED load — the classifier. Anything at all that lives at this path
		// comes back here, whatever its class.
		UObject* const AnyObject = SoftPath.TryLoad();

		// The CONSUMER's own typed expression, run separately and on purpose.
		OutConsumerSystem = TSoftObjectPtr<UNiagaraSystem>(SoftPath).LoadSynchronous();

		if (!AnyObject)
		{
			return ESpellVFXOutcome::Absent;
		}

		OutFoundClassName = GetNameSafe(AnyObject->GetClass());
		return Cast<UNiagaraSystem>(AnyObject) ? ESpellVFXOutcome::Present : ESpellVFXOutcome::WrongType;
	}

	/**
	 *  ⚠️ THE LOOKUP CHECKS THE POINTED-TO TYPE, NOT JUST THE FIELD NAME. A property
	 *  renamed OR retyped must be reported BY NAME, because the alternative is a roster
	 *  that stays empty and a gate that passes because there was nothing to check.
	 *  (Deliberate duplicate of the sibling roster gates' helper — the namespaces are
	 *  what keep the copies apart in a unity build; ⛔ do not flatten them.)
	 */
	static const FSoftObjectPtr* FindSoftObjectField(const UObject* Object, const TCHAR* FieldName, const UClass* ExpectedPointeeClass)
	{
		const FSoftObjectProperty* const SoftProperty = FindFProperty<FSoftObjectProperty>(Object->GetClass(), FieldName);
		// `.Get()` rather than comparing the TObjectPtr directly — the raw-pointer
		// comparison is well-defined across every build configuration.
		if (!SoftProperty || SoftProperty->PropertyClass.Get() != ExpectedPointeeClass)
		{
			return nullptr;
		}
		return SoftProperty->ContainerPtrToValuePtr<FSoftObjectPtr>(Object);
	}

	/** ⛔ Strips any `EnumName::` qualifier `UEnum` may return for a scoped entry — the shipped idiom. */
	static FString ShortEnumEntry(const FString& EntryName)
	{
		int32 SeparatorIndex = INDEX_NONE;
		if (EntryName.FindLastChar(TEXT(':'), SeparatorIndex))
		{
			return EntryName.RightChop(SeparatorIndex + 1);
		}
		return EntryName;
	}

	/** The `ECardType` entry NAME (the symbol, `SC-§38`) — never a raw integer in a message a human reads. */
	static FString CardTypeSymbol(ECardType CardType)
	{
		const UEnum* const CardTypeEnum = StaticEnum<ECardType>();
		if (!CardTypeEnum)
		{
			return FString::Printf(TEXT("<ECardType reflection unavailable; raw %d>"), static_cast<int32>(CardType));
		}

		FString Symbol = ShortEnumEntry(CardTypeEnum->GetNameStringByValue(static_cast<int64>(CardType)));
		if (Symbol.IsEmpty())
		{
			Symbol = FString::Printf(TEXT("<not an ECardType enumerator; raw %d>"), static_cast<int32>(CardType));
		}
		return Symbol;
	}

	/** The `ESpellEffect` entry NAME (the symbol, `SC-§38`) — the value `ResolveSpell` dispatches on, named in every message. */
	static FString SpellEffectSymbol(ESpellEffect SpellEffect)
	{
		const UEnum* const SpellEffectEnum = StaticEnum<ESpellEffect>();
		if (!SpellEffectEnum)
		{
			return FString::Printf(TEXT("<ESpellEffect reflection unavailable; raw %d>"), static_cast<int32>(SpellEffect));
		}

		FString Symbol = ShortEnumEntry(SpellEffectEnum->GetNameStringByValue(static_cast<int64>(SpellEffect)));
		if (Symbol.IsEmpty())
		{
			Symbol = FString::Printf(TEXT("<not an ESpellEffect enumerator; raw %d>"), static_cast<int32>(SpellEffect));
		}
		return Symbol;
	}

	/** Every `.cpp` under `Source/`, absolute paths — the composer pin's search space. */
	static void FindAllSourceCppFiles(TArray<FString>& OutFiles)
	{
		OutFiles.Reset();
		const FString SourceRoot = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("Source"));
		IFileManager::Get().FindFilesRecursive(OutFiles, *SourceRoot, TEXT("*.cpp"), /*Files=*/ true, /*Directories=*/ false, /*bClearFileNames=*/ false);
	}

	/** The ONE spell-VFX composer's own record, so a red can say WHICH half of the mirror went stale. */
	struct FComposerPin
	{
		bool    bScanAlive          = false;
		bool    bFileFound          = false;
		bool    bAssetNameFormat    = false;
		bool    bObjectPathFormat   = false;
		int32   SourceFilesScanned  = 0;
		FString SpellLibraryPath;
	};

	/**
	 *  ⭐⭐ THE COMPOSER PIN (`SC-§75`(A)'s source-text instrument, used for the one case
	 *  it is meant for: the behaviour is UNREACHABLE from a test, so there is nothing
	 *  else to assert). Locates the shipped spell library BY FILENAME — a key, never a
	 *  line number (`SC-§77`) — and looks for the two format literals this file mirrors.
	 *
	 *  ⛔ A DEAD SCAN MUST NOT READ AS A CLEAN PIN: the file count is reported and
	 *  asserted by the caller, so "I found the literals" and "I found no files at all"
	 *  can never be the same answer (`SC-§39`).
	 */
	static FComposerPin PinComposerAgainstSource()
	{
		FComposerPin Pin;

		TArray<FString> SourceFiles;
		FindAllSourceCppFiles(SourceFiles);
		Pin.SourceFilesScanned = SourceFiles.Num();
		Pin.bScanAlive = (SourceFiles.Num() >= 20);

		for (const FString& File : SourceFiles)
		{
			if (!FPaths::GetCleanFilename(File).Equals(TEXT("SpellLibrary.cpp"), ESearchCase::CaseSensitive))
			{
				continue;
			}

			FString Text;
			if (!FFileHelper::LoadFileToString(Text, *File))
			{
				continue;
			}

			Pin.bFileFound = true;
			Pin.SpellLibraryPath = File;
			Pin.bAssetNameFormat  = Text.Contains(TEXT("NS_Spell_%s"), ESearchCase::CaseSensitive);
			Pin.bObjectPathFormat = Text.Contains(TEXT("/Game/VFX/%s.%s"), ESearchCase::CaseSensitive);
			break;
		}

		return Pin;
	}

	/** One walked card, carrying everything its message needs. */
	struct FSpellCandidate
	{
		FString CardID;
		FString CardTypeSymbolText   = TEXT("<not in the card table>");
		FString SpellEffectSymbolText = TEXT("<not in the card table>");
		bool    bFromCardTable       = false;   // the runtime authority named it
		bool    bFromCsv             = false;   // the spreadsheet of record named it
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeSpellVFXRosterTest,
	"Siegebound.CardRoster.EverySpellCardHasItsCastVFXSystem",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeSpellVFXRosterTest::RunTest(const FString& Parameters)
{
	using namespace SiegeSpellVFXRosterTestFixture;

	// ══ SELF-CHECKS ═════════════════════════════════════════════════════════
	// ⛔ EVERY ONE OF THESE RETURNS FALSE RATHER THAN CONTINUING. A gate that
	//    cannot reach its subject must FAIL, never report SAFE — an unreadable
	//    instrument and a healthy roster otherwise produce the identical bar.

	const UObject* const ControllerDefaults = ASiegePlayerController::StaticClass()->GetDefaultObject();
	if (!ControllerDefaults)
	{
		AddError(TEXT("⛔ SELF-CHECK FAILED: ASiegePlayerController's CDO is null — the spell roster this gate walks is derived from it, so nothing below would mean anything."));
		return false;
	}

	// ── (a) THE CARD TABLE, OFF THE SPELL PATH'S OWN CONSUMER ────────────────
	// ⭐ The CONTROLLER's table rather than the card-hand widget's, deliberately:
	//    the controller is what reads the row and calls USpellLibrary::ResolveSpell,
	//    so it is the consumer whose roster decides which VFX can ever be spawned.
	const FSoftObjectPtr* const ControllerTableValue =
		FindSoftObjectField(ControllerDefaults, TEXT("CardTableAsset"), UDataTable::StaticClass());
	if (!ControllerTableValue)
	{
		AddError(TEXT("⛔ SELF-CHECK FAILED: ASiegePlayerController::CardTableAsset (TSoftObjectPtr<UDataTable>) was not reachable on the CDO — it was RENAMED or RETYPED. The spell roster cannot be derived, and a gate that walked zero spells would pass VACUOUSLY."));
		return false;
	}

	const FSoftObjectPath ControllerTablePath = ControllerTableValue->ToSoftObjectPath();
	if (ControllerTablePath.IsNull())
	{
		AddError(TEXT("⛔ SELF-CHECK FAILED: ASiegePlayerController::CardTableAsset is UNSET on the CDO — the shipped constructor assigns it, so an empty path means that assignment was removed and every card play would silently fall back to no row at all."));
		return false;
	}

	const UDataTable* const CardTable = TSoftObjectPtr<UDataTable>(ControllerTablePath).LoadSynchronous();
	if (!CardTable)
	{
		AddError(FString::Printf(TEXT("⛔ SELF-CHECK FAILED: the card table '%s' — the exact asset the shipped player controller loads — did not load as a UDataTable."), *ControllerTablePath.ToString()));
		return false;
	}

	if (CardTable->GetRowStruct() != FCardRow::StaticStruct())
	{
		AddError(FString::Printf(TEXT("⛔ SELF-CHECK FAILED: card table '%s' has row struct '%s', not FCardRow — every FindRow<FCardRow> below would return null and the walk would silently check nothing."),
			*ControllerTablePath.ToString(), *GetNameSafe(CardTable->GetRowStruct())));
		return false;
	}

	AddInfo(FString::Printf(TEXT("card table derived from the spell path's own consumer CDO (never typed here): '%s', row struct FCardRow."), *ControllerTablePath.ToString()));

	// ── (b) THE COMPOSER PIN — THIS FILE'S ONE DECLARED RESIDUAL, GUARDED ────
	// ⛔ NOT a self-check that returns: the walk below is still meaningful if the
	//    pin fails, and a reader must see BOTH facts. But it IS an assertion, and
	//    it is the only thing standing between this gate and a silently stale
	//    mirror of a composer no test can call.
	const FComposerPin Composer = PinComposerAgainstSource();

	TestTrue(FString::Printf(TEXT("SELF-CHECK — the recursive scan of Source/ is ALIVE (found %d .cpp file(s)); without this, 'the composer literals are missing' and 'I read no files' would be the same red"),
		Composer.SourceFilesScanned), Composer.bScanAlive);

	TestTrue(TEXT("SELF-CHECK — the shipped spell library 'SpellLibrary.cpp' was located under Source/ by FILENAME (a key, never a line number). If this reds, the file was RENAMED or MOVED and the composer pin below is measuring nothing"),
		Composer.bFileFound);

	TestTrue(TEXT("⭐⭐ COMPOSER PIN (a) — SpellLibrary.cpp still builds the spell VFX ASSET NAME with the `NS_Spell_%s` format this test mirrors. ⛔ If this reds, the shipped composer MOVED and every path asserted below is the wrong path — repair the MIRROR in this file, never the assertion"),
		Composer.bAssetNameFormat);

	TestTrue(TEXT("⭐⭐ COMPOSER PIN (b) — SpellLibrary.cpp still builds the spell VFX OBJECT PATH with the `/Game/VFX/%s.%s` format this test mirrors. ⛔ Same repair: the mount point or the package layout moved, and this file's mirror must follow it"),
		Composer.bObjectPathFormat);

	if (Composer.bFileFound)
	{
		AddInfo(FString::Printf(TEXT("composer pinned against '%s' (located by filename; asset-name format %s, object-path format %s)."),
			*Composer.SpellLibraryPath,
			Composer.bAssetNameFormat ? TEXT("FOUND") : TEXT("⛔ MISSING"),
			Composer.bObjectPathFormat ? TEXT("FOUND") : TEXT("⛔ MISSING")));
	}

	// ── (c) THE ENUM SYMBOLS, OFF THE SHIPPED ENUMS ──────────────────────────
	const UEnum* const CardTypeEnum = StaticEnum<ECardType>();
	if (!CardTypeEnum)
	{
		AddError(TEXT("⛔ SELF-CHECK FAILED: StaticEnum<ECardType>() returned null — the CSV cross-walk compares its CardType cell against this enum's own entry name, so it would match nothing and the CSV roster would come back EMPTY."));
		return false;
	}

	const FString SpellTypeName = ShortEnumEntry(CardTypeEnum->GetNameStringByValue(static_cast<int64>(ECardType::Spell)));
	if (SpellTypeName.IsEmpty())
	{
		AddError(TEXT("⛔ SELF-CHECK FAILED: ECardType did not yield an entry name for its Spell value — the CSV filter below would match nothing and would report a healthy zero."));
		return false;
	}

	// ══ ROSTER SOURCE 1 — THE RUNTIME AUTHORITY (the DataTable the game loads) ══
	//
	// ⛔ SPELLHOOD IS `SpellEffect != None`, NOT `CardType == Spell` — join on the
	//    category the CODE COMPUTES (`SC-§37`). `USpellLibrary::ResolveSpell`
	//    switches on `Row.SpellEffect`; its `None` arm refuses and returns before
	//    the shared VFX tail, so a None-effect row can never reach the spawn and
	//    asserting an asset for it would be a FALSE red. The `CardType == Spell`
	//    rows are collected too — and the two are asserted to COINCIDE, which is
	//    what stops the effect-keyed walk silently skipping a spell card.

	const TArray<FName> RowNames = CardTable->GetRowNames();

	int32 TotalRows = 0;
	TMap<FString, FSpellCandidate> Candidates;
	TSet<FString> TableSpellEffectRows;      // rows that can REACH the spawn
	TSet<FString> TableSpellTypedRows;       // rows the data DECLARES as spells
	TArray<FString> SpellTypedRowsWithNoEffect;

	for (const FName& RowName : RowNames)
	{
		const FCardRow* const Row = CardTable->FindRow<FCardRow>(RowName, TestContext, /*bWarnIfRowMissing=*/ false);
		if (!Row)
		{
			AddError(FString::Printf(TEXT("card table row '%s' is named by GetRowNames() but FindRow<FCardRow> returned null — the row exists and cannot be read, which is NOT the same as a row that is absent. A spell hiding in an unreadable row would be walked by nothing."), *RowName.ToString()));
			continue;
		}

		++TotalRows;

		const FString CardID = RowName.ToString();
		const bool bHasEffect  = (Row->SpellEffect != ESpellEffect::None);
		const bool bSpellTyped = (Row->CardType == ECardType::Spell);

		if (bHasEffect)  { TableSpellEffectRows.Add(CardID); }
		if (bSpellTyped) { TableSpellTypedRows.Add(CardID); }

		if (bSpellTyped && !bHasEffect)
		{
			SpellTypedRowsWithNoEffect.Add(CardID);
		}

		if (!bHasEffect && !bSpellTyped)
		{
			continue;
		}

		FSpellCandidate& Candidate = Candidates.FindOrAdd(CardID);
		Candidate.CardID = CardID;
		Candidate.CardTypeSymbolText = CardTypeSymbol(Row->CardType);
		Candidate.SpellEffectSymbolText = SpellEffectSymbol(Row->SpellEffect);
		Candidate.bFromCardTable = true;
	}

	// ⛔ COVERAGE, NOT A STYLE POINT: this is what makes the effect-keyed walk above
	//    provably total over the spell cards. A `CardType Spell` row with a `None`
	//    effect is a card that can NEVER resolve — `ResolveSpell` refuses it outright
	//    and the caller refunds — so it is a real defect with its own repair, and it
	//    must not be silently absorbed into "this row has no VFX".
	TestEqual(TEXT("⛔ COVERAGE — ZERO card rows are typed Spell while carrying SpellEffect None (such a row can never resolve, so the effect-keyed spell walk would skip it and this gate would under-report)"),
		SpellTypedRowsWithNoEffect.Num(), 0);

	for (const FString& CardID : SpellTypedRowsWithNoEffect)
	{
		AddError(FString::Printf(TEXT("⛔ SPELL ROW THAT CAN NEVER RESOLVE: card '%s' is CardType %s but its SpellEffect cell is %s. USpellLibrary::ResolveSpell refuses it at the `case ESpellEffect::None:` arm, the caller refunds, and it never reaches the cast VFX at all. ⛔ The repair is the ROW's SpellEffect cell — a cast VFX asset would not help this card."),
			*CardID, *SpellTypeName, *SpellEffectSymbol(ESpellEffect::None)));
	}

	// ══ ROSTER SOURCE 2 — THE SPREADSHEET OF RECORD (`Docs/Data/cards.csv`) ═══
	//
	// ⛔ A probe that cannot read its subject FAILS rather than reporting SAFE.

	TSet<FString> CsvSpellRows;
	int32 CsvRowsRead = 0;
	bool  bCsvPositiveControlSeen = false;
	bool  bCsvPositiveControlIsSpell = false;

	const FString CsvFullPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / FString(CardsCsvPath));
	FString CsvText;
	if (!FPaths::FileExists(CsvFullPath) || !FFileHelper::LoadFileToString(CsvText, *CsvFullPath))
	{
		AddError(FString::Printf(TEXT("⛔ SELF-CHECK FAILED: could not read '%s'. The spreadsheet of record is one of this gate's two rosters, and a gate that silently walked only one of them would report a narrower fact than it publishes."), *CsvFullPath));
		return false;
	}

	TArray<FString> CsvLines;
	CsvText.ParseIntoArrayLines(CsvLines, /*bCullEmpty*/ true);
	if (CsvLines.Num() < 10)
	{
		AddError(FString::Printf(TEXT("⛔ SELF-CHECK FAILED: '%s' parsed to %d line(s) — the CSV probe is dead, and a dead probe reads as a roster with no spells in it."), *CsvFullPath, CsvLines.Num()));
		return false;
	}

	TArray<FString> CsvHeader;
	CsvLines[0].ParseIntoArray(CsvHeader, TEXT(","), /*InCullEmpty*/ false);

	// ⚠️⛔ `FOG-§9.8e` — THE DataTable CSV CONVENTION GIVES THE ROW-NAME COLUMN AN
	//    **EMPTY** HEADER, which is exactly what makes `CardID` field 0. ⛔ ASSERTED,
	//    never assumed: a header that grew a name here would shift every field by one
	//    and this walk would come back as a well-formed table of blanks.
	if (!CsvHeader.IsValidIndex(0) || !CsvHeader[0].IsEmpty())
	{
		AddError(FString::Printf(TEXT("⛔ SELF-CHECK FAILED: '%s' column 0 has the header '%s', not the EMPTY row-name header the DataTable CSV convention requires (FOG-§9.8e). Every CardID below would be mis-read."),
			*CsvFullPath, CsvHeader.IsValidIndex(0) ? *CsvHeader[0] : TEXT("<absent>")));
		return false;
	}

	// ⛔ COLUMNS BY NAME, NEVER BY INDEX. `IndexOfByKey` is EXACT equality, so
	//    `SpawnCardID` cannot be mistaken for `CardType` the way a substring search
	//    would allow (`SC-§41`: pin the role, not a loose token).
	const int32 CardTypeColumn = CsvHeader.IndexOfByKey(FString(TEXT("CardType")));
	const int32 NotesColumn    = CsvHeader.IndexOfByKey(FString(TEXT("Notes")));
	if (CardTypeColumn == INDEX_NONE || NotesColumn == INDEX_NONE)
	{
		AddError(FString::Printf(TEXT("⛔ SELF-CHECK FAILED: '%s' has no `CardType` column (%d) or no `Notes` column (%d) — this probe is STALE, so it FAILS rather than guessing."),
			*CsvFullPath, CardTypeColumn, NotesColumn));
		return false;
	}

	// ⛔⛔ THE COMMA-SAFETY PRECONDITION, ASSERTED RATHER THAN ASSUMED. `Notes` is
	//    free text and may carry an embedded comma, which pushes every LATER column
	//    right. This walk reads only `CardID` (field 0) and `CardType`, and it is
	//    safe not to skip mis-aligned rows ONLY because both sit BEFORE `Notes`.
	//    ⇒ that ordering is checked here, by name. ⚠️ It is also why `SpellEffect`
	//    — which sits AFTER `Notes` — is read from the DATATABLE and never from here.
	if (CardTypeColumn >= NotesColumn)
	{
		AddError(FString::Printf(TEXT("⛔ SELF-CHECK FAILED: `CardType` (column %d) is no longer BEFORE `Notes` (column %d) — an embedded comma in the free-text Notes cell could now mis-index it, so this parse is unsafe and FAILS rather than guessing."),
			CardTypeColumn, NotesColumn));
		return false;
	}

	for (int32 Index = 1; Index < CsvLines.Num(); ++Index)
	{
		TArray<FString> Fields;
		CsvLines[Index].ParseIntoArray(Fields, TEXT(","), /*InCullEmpty*/ false);

		// ⛔ A SHORT row is malformed and is a FAILURE, not something to skip: a
		//    truncated row could silently drop a spell from this roster. A LONG row is
		//    the legal embedded-comma case and its pre-`Notes` fields are still placed.
		if (Fields.Num() < CsvHeader.Num())
		{
			AddError(FString::Printf(TEXT("⛔ SELF-CHECK FAILED: '%s' has a row with %d field(s) against a %d-field header (its CardID reads '%s') — the row is truncated and a spell could be lost from this roster silently."),
				*CsvFullPath, Fields.Num(), CsvHeader.Num(), Fields.IsValidIndex(0) ? *Fields[0] : TEXT("<absent>")));
			return false;
		}

		++CsvRowsRead;

		const FString CardID   = Fields[0].TrimStartAndEnd();
		const FString CardType = Fields[CardTypeColumn].TrimStartAndEnd();

		if (CardID.IsEmpty())
		{
			continue;
		}

		// ⭐ THE CSV HALF OF THE POSITIVE CONTROL (`SC-§39`), tallied for EVERY row —
		//    including ones the filter rejects — because "I found no Spell rows" and
		//    "I could not read the CardType column" are otherwise the same answer.
		if (CardID.Equals(PositiveControlCardID, ESearchCase::CaseSensitive))
		{
			bCsvPositiveControlSeen = true;
			bCsvPositiveControlIsSpell = CardType.Equals(SpellTypeName, ESearchCase::CaseSensitive);
		}

		if (!CardType.Equals(SpellTypeName, ESearchCase::CaseSensitive))
		{
			continue;
		}

		CsvSpellRows.Add(CardID);

		FSpellCandidate& Candidate = Candidates.FindOrAdd(CardID);
		Candidate.CardID = CardID;
		Candidate.bFromCsv = true;
		if (!Candidate.bFromCardTable)
		{
			// ⛔ A spell the TABLE never named. Its CardType is known (the spreadsheet
			//    just said so) but its SpellEffect is not readable from here — that
			//    column sits after `Notes`. Said in the message rather than guessed.
			Candidate.CardTypeSymbolText = CardType;
			Candidate.SpellEffectSymbolText = TEXT("<unreadable — this card has no row in the shipped card table>");
		}
	}

	// ⭐ THE CSV INSTRUMENT'S OWN CONTROL. Without it, a CardType column read as
	//    blanks would produce ZERO spell rows and would look exactly like a CSV with
	//    no spells in it — a silent halving of this gate's roster.
	TestTrue(FString::Printf(TEXT("⭐ CSV POSITIVE CONTROL — the known-present card '%s' was REACHED by the spreadsheet walk (%d row(s) read from '%s')"),
		PositiveControlCardID, CsvRowsRead, *CsvFullPath), bCsvPositiveControlSeen);

	TestTrue(FString::Printf(TEXT("⭐ CSV POSITIVE CONTROL — '%s' reads as CardType '%s' in the spreadsheet, so the CardType column CAN answer 'this is a spell'. ⛔ THE LIVE MEMBER: without it, a column read as blanks yields zero spells and is indistinguishable from a spreadsheet that has none"),
		PositiveControlCardID, *SpellTypeName), bCsvPositiveControlIsSpell);

	// ══ THE TWO ROSTERS, ASSERTED TO AGREE ══════════════════════════════════
	//
	// ⛔ NOT a tidiness check. A spell authored in the spreadsheet but never imported
	//    into `DT_Cards` is UNPLAYABLE and nothing else in the tree notices; a spell
	//    in the table with no CSV row is an asset edited out from under the source of
	//    record. Two different defects, both invisible, both named here.

	TArray<FString> CsvOnlySpells;
	for (const FString& CardID : CsvSpellRows)
	{
		if (!TableSpellTypedRows.Contains(CardID))
		{
			CsvOnlySpells.Add(CardID);
		}
	}

	TArray<FString> TableOnlySpells;
	for (const FString& CardID : TableSpellTypedRows)
	{
		if (!CsvSpellRows.Contains(CardID))
		{
			TableOnlySpells.Add(CardID);
		}
	}

	CsvOnlySpells.Sort();
	TableOnlySpells.Sort();

	TestEqual(TEXT("ROSTER AGREEMENT (a) — ZERO spells exist in Docs/Data/cards.csv but NOT in the shipped card table (such a card was authored and never imported: it is unplayable, and no other gate in the tree looks)"),
		CsvOnlySpells.Num(), 0);
	if (CsvOnlySpells.Num() > 0)
	{
		AddError(FString::Printf(TEXT("⛔ SPELL IN THE SPREADSHEET BUT NOT IN THE TABLE: %s. The row was authored in Docs/Data/cards.csv and never reached Content/Data/DT_Cards — the card cannot be played at all. ⛔ The repair is the IMPORT, not this assertion."),
			*FString::Join(CsvOnlySpells, TEXT(", "))));
	}

	TestEqual(TEXT("ROSTER AGREEMENT (b) — ZERO spells exist in the shipped card table but NOT in Docs/Data/cards.csv (such a row was edited into the asset behind the source of record and will vanish on the next re-import)"),
		TableOnlySpells.Num(), 0);
	if (TableOnlySpells.Num() > 0)
	{
		AddError(FString::Printf(TEXT("⛔ SPELL IN THE TABLE BUT NOT IN THE SPREADSHEET: %s. The row lives only in Content/Data/DT_Cards and would be lost the next time Docs/Data/cards.csv is re-imported. ⛔ The repair is the CSV row."),
			*FString::Join(TableOnlySpells, TEXT(", "))));
	}

	// ══⭐⭐ THE WALK — EVERY SPELL, ITS OWN NAMED ASSET ══════════════════════

	TArray<FString> WalkOrder;
	Candidates.GetKeys(WalkOrder);
	WalkOrder.Sort();   // deterministic message order; a TMap's iteration order is not a fact about the roster

	int32 ProbesExecuted = 0;
	int32 PresentCount   = 0;
	int32 AbsentCount    = 0;
	int32 WrongTypeCount = 0;
	int32 ConsumerAgreements = 0;

	TArray<FString> AbsentCardIDs;
	TArray<FString> WrongTypeCardIDs;
	TSet<FString> DistinctVFXPaths;

	bool bPositiveControlWalked  = false;
	bool bPositiveControlPresent = false;
	FString PositiveControlPath;
	// ⛔ The control's MEASURED outcome, kept so every message about it reports what
	//    the probe ACTUALLY answered rather than the value this file hoped for
	//    (`SC-§37`). Seeded with a value the walk must overwrite: if the control is
	//    never reached, the reports say ABSENT and the VACUITY assertion says why.
	ESpellVFXOutcome PositiveControlOutcome = ESpellVFXOutcome::Absent;

	for (const FString& CardID : WalkOrder)
	{
		const FSpellCandidate& Candidate = Candidates[CardID];

		// ⛔ THE PATH IS COMPOSED FROM THE CARDID BY THE MIRRORED COMPOSER — the same
		//    two lines the shipped spawn runs, pinned against its source above.
		const FString ObjectPath  = ComposeSpellVFXObjectPath(CardID);
		const FString ContentFile = ComposeSpellVFXContentFile(CardID);

		const bool bIsPositiveControl = CardID.Equals(PositiveControlCardID, ESearchCase::CaseSensitive);
		if (bIsPositiveControl)
		{
			bPositiveControlWalked = true;
			PositiveControlPath = ObjectPath;
		}

		++ProbesExecuted;
		UNiagaraSystem* ConsumerSystem = nullptr;
		FString FoundClassName;
		const ESpellVFXOutcome Outcome = ProbeSpellVFX(ObjectPath, ConsumerSystem, FoundClassName);

		// ⛔ THE CLASSIFIER AND THE CONSUMER MUST AGREE. The player is subject to the
		//    consumer's typed expression; everything this file reports is the
		//    classifier's. A divergence would mean the message and the behaviour had
		//    come apart, and the message is this gate's whole deliverable.
		const bool bConsumerAgrees = ((ConsumerSystem != nullptr) == (Outcome == ESpellVFXOutcome::Present));
		if (bConsumerAgrees)
		{
			++ConsumerAgreements;
		}

		DistinctVFXPaths.Add(ObjectPath);

		// ⛔⛔ THE ASSERTION THIS WHOLE FILE EXISTS FOR — EXISTENCE OF THE **NAMED**
		//     ASSET, never a bare "something was produced".
		const bool bPresent = (Outcome == ESpellVFXOutcome::Present);
		TestTrue(FString::Printf(TEXT("⭐ SPELL CAST VFX — card '%s' (CardType %s, SpellEffect %s) has its Niagara system at '%s'"),
			*CardID, *Candidate.CardTypeSymbolText, *Candidate.SpellEffectSymbolText, *ObjectPath), bPresent);

		if (bIsPositiveControl)
		{
			bPositiveControlPresent = bPresent;
			PositiveControlOutcome  = Outcome;
		}

		switch (Outcome)
		{
		case ESpellVFXOutcome::Present:
			++PresentCount;
			break;

		case ESpellVFXOutcome::Absent:
			++AbsentCount;
			AbsentCardIDs.Add(CardID);
			// ⛔⛔ THE WORK ORDER. It names the CardID, the exact object path AND the
			//     file to create, so an artist can act on this red without opening one
			//     source file.
			AddError(FString::Printf(
				TEXT("⛔ SPELL WITH NO CAST VFX — THE ASSET IS ABSENT: card '%s' (CardType %s, SpellEffect %s) has NO Niagara system at '%s'. ")
				TEXT("⛔ WHAT TO MAKE AND WHERE — no source file needs to be opened: author a Niagara system and save it as '%s' (asset name 'NS_Spell_%s' — ")
				TEXT("USpellLibrary COMPOSES that name from the CardID, so it must match character for character). ")
				TEXT("⛔ WHY IT IS URGENT: the spawn is NULL-SAFE and LOG-ONCE by design, so this card still charges its gold, still returns true and still logs 'resolved' while the player sees NOTHING — ")
				TEXT("VID-006 measured exactly that on 'Fog' (985 → 935) and 'BrightSun' (893 → 834): charged, no fizzle, no refund, no pixels. A successful cast is byte-for-byte indistinguishable from a dead click. ")
				TEXT("⛔ THE REPAIR IS THE ASSET, never a weakened assertion here (CONVENTIONS SC-§50 cl. 4)."),
				*CardID, *Candidate.CardTypeSymbolText, *Candidate.SpellEffectSymbolText, *ObjectPath, *ContentFile, *CardID));
			break;

		case ESpellVFXOutcome::WrongType:
			++WrongTypeCount;
			WrongTypeCardIDs.Add(CardID);
			AddError(FString::Printf(
				TEXT("⛔ SPELL WITH NO CAST VFX — THE PATH RESOLVES TO THE WRONG ASSET TYPE: card '%s' (CardType %s, SpellEffect %s) has an object at '%s', but it is a '%s' and NOT a UNiagaraSystem. ")
				TEXT("⛔ THIS IS NOT A MISSING-ASSET GAP: the file is there, so authoring a second one is impossible at that path. ⛔ WHAT TO DO: replace '%s' with a Niagara system, or move the impostor aside and save the system in its place. ")
				TEXT("⛔ WHY IT IS URGENT: the shipped spawn asks for a UNiagaraSystem and gets null, so it no-ops EXACTLY as if nothing were there at all — the card charges its gold and shows nothing. ")
				TEXT("⛔ THE REPAIR IS THE ASSET, never a weakened assertion here (CONVENTIONS SC-§50 cl. 4)."),
				*CardID, *Candidate.CardTypeSymbolText, *Candidate.SpellEffectSymbolText, *ObjectPath, *FoundClassName, *ContentFile));
			break;
		}
	}

	// ══ THE COUNTS, ASSERTED — ⛔ NOT MERELY LOGGED ═════════════════════════
	//
	// ⭐ RELATIONSHIPS, NOT LITERALS (`SC-§40` cl. 10). ⛔ NO EXPECTED SPELL COUNT
	//    APPEARS ANYWHERE: spell #8 would turn this file red while the roster is
	//    perfectly healthy, and the obvious repair would be to bump the number.

	TestTrue(TEXT("COUNT (subsumed tell, kept for its message) — the derived roster yielded spells to walk"),
		WalkOrder.Num() > 0);

	TestEqual(TEXT("PARTITION — every candidate was PROBED (a probe count below the candidate count means cards were skipped, not passed)"),
		ProbesExecuted, WalkOrder.Num());

	TestEqual(TEXT("PARTITION — the outcomes are TOTAL: present + absent + wrong-type == probes executed (a card that landed in no bucket was silently dropped from the gate)"),
		PresentCount + AbsentCount + WrongTypeCount, ProbesExecuted);

	TestEqual(TEXT("⛔ CONSUMER AGREEMENT — for EVERY walked card the shipped consumer's own typed expression and this file's classifier returned the SAME verdict. If this reds, the message this gate publishes has come apart from the behaviour the player is subject to, and the MESSAGE is this gate's deliverable"),
		ConsumerAgreements, ProbesExecuted);

	TestEqual(TEXT("DISTINCTNESS — every walked spell composed a DISTINCT VFX path (two cards sharing one path would mean the composer stopped depending on the CardID, and one card would silently wear another's effect)"),
		DistinctVFXPaths.Num(), WalkOrder.Num());

	// ⛔ THE TWO FINDINGS, ASSERTED SEPARATELY AND BY COUNT — different repairs.
	//    ⚠️ Each is SUBSUMED by its own per-card assertion above; kept because a
	//    reader of the summary must be able to tell "author it" from "replace it".
	TestEqual(TEXT("FINDING (a) — ZERO spell cards are MISSING their cast VFX asset"),
		AbsentCount, 0);

	TestEqual(TEXT("FINDING (b) — ZERO spell cards have a cast VFX path occupied by a NON-Niagara asset"),
		WrongTypeCount, 0);

	// ══ THE POSITIVE-CONTROL CHAIN (`SC-§39`) ═══════════════════════════════
	//
	// ⛔ THESE TWO ARE **ONE TELL** (`SC-§51` cl. 2): WALKED false ⟹ PRESENT false.
	//    ⭐ THE LIVE MEMBER IS `PRESENT`. Now that the roster is whole, this control is
	//    not a formality — it is the only thing separating "no spell is missing its
	//    VFX" from "the loader is blind and every path resolves to nothing".

	TestTrue(FString::Printf(TEXT("VACUITY (subsumed member, kept for its message) — the known-good spell '%s' was REACHED by the walk"), PositiveControlCardID),
		bPositiveControlWalked);

	TestTrue(FString::Printf(TEXT("⭐ POSITIVE CONTROL — '%s' RESOLVED its cast VFX at '%s'. ⛔ THE LIVE MEMBER: it is what separates 'these specific cards are missing their VFX' from 'the walk never ran' AND from 'the loader cannot return PRESENT for anything'"),
		PositiveControlCardID, *PositiveControlPath), bPositiveControlPresent);

	// ══ ⛔⛔⭐⭐ THE SYNTHESIS — `SC-§54` cl. 3 + `SHIP-§9` ═══════════════════
	//
	// ⛔ THIS GATE WAS BORN RED AND THAT RED HAS ALREADY CLOSED (see the one
	//    perishable paragraph in the file header). The roster is whole, so ⛔ NOTHING
	//    ELSE ON THIS PAGE demonstrates that the probe can still answer ABSENT. ⇒ the
	//    miss is driven here for real, on a MEASURED-ABSENT synthetic CardID, through
	//    the SAME probe and the SAME composer the walk just used — so the proof
	//    RE-RUNS FOREVER rather than living in a transcript nobody re-opens.
	//
	// ⛔ THE ASSERTIONS ARE INVERTED: they assert the MISS. This block is GREEN
	//    **BECAUSE** the probe answered ABSENT. ⛔ A control that turns the suite red
	//    is a bug wearing a control's clothes, and nothing here calls AddError on an
	//    expected miss.
	//
	// ⛔ Declared to the harness by MY OWN synthetic CardID, never by the engine's
	//    message prose (`SC-§38`), with Occurrences = -1, which the framework
	//    documents as "silently ignored" ⇒ this line CANNOT turn the suite red if the
	//    engine ever stops emitting the warning or rewords it.
	AddExpectedMessagePlain(SyntheticAbsentCardID, ELogVerbosity::Warning,
		EAutomationExpectedMessageFlags::Contains, /*Occurrences*/ -1);

	const FString SyntheticPath = ComposeSpellVFXObjectPath(FString(SyntheticAbsentCardID));

	TestTrue(FString::Printf(TEXT("SYNTHESIS PREMISE — the synthetic path '%s' is WELL-FORMED and DIFFERENT from the positive control's '%s' (identical strings would make the discrimination assertion below vacuous)"),
		*SyntheticPath, *PositiveControlPath),
		!SyntheticPath.IsEmpty() && SyntheticPath != PositiveControlPath);

	UNiagaraSystem* SyntheticConsumerSystem = nullptr;
	FString SyntheticFoundClass;
	const ESpellVFXOutcome SyntheticOutcome = ProbeSpellVFX(SyntheticPath, SyntheticConsumerSystem, SyntheticFoundClass);

	TestTrue(FString::Printf(TEXT("⭐⭐ SYNTHESISED RED / NEGATIVE CONTROL — the MEASURED-ABSENT CardID '%s', pushed through the SAME composer to '%s', is classified %s (got %s). ⛔ THE LOAD-BEARING ASSERTION OF THIS FILE NOW THAT EVERY SHIPPED SPELL PASSES: it is the ONLY one that catches a loader gone BLIND and answering non-null for everything — a mode in which every card passes, every count is healthy and every partition balances"),
		SyntheticAbsentCardID, *SyntheticPath, SpellVFXOutcomeSymbol(ESpellVFXOutcome::Absent), SpellVFXOutcomeSymbol(SyntheticOutcome)),
		SyntheticOutcome == ESpellVFXOutcome::Absent);

	TestNull(FString::Printf(TEXT("⭐⭐ SYNTHESISED RED — the shipped consumer's OWN typed expression also returns NULL for '%s', which is precisely the value that makes a cast spawn nothing while still resolving true"), *SyntheticPath),
		SyntheticConsumerSystem);

	// ── DISCRIMINATION: one probe, two answers, one run ──────────────────────
	// ⚠️ ⛔ NOT COUNTED AS A TELL (`SC-§51` cl. 2): it cannot fire while the positive
	//    control and the synthesis above both pass. Kept because it states the whole
	//    property in one readable line — and because deleting a subsumed assertion is
	//    how the live ones get deleted next.
	TestTrue(FString::Printf(TEXT("DISCRIMINATION (subsumed, kept for its message) — ONE probe (ProbeSpellVFX) returned TWO DIFFERENT outcomes in the SAME run: %s for the shipped '%s' and %s for the synthetic '%s'. A probe stuck on EITHER answer fails here, and it fails at the two assertions above first"),
		SpellVFXOutcomeSymbol(PositiveControlOutcome), PositiveControlCardID,
		SpellVFXOutcomeSymbol(SyntheticOutcome), SyntheticAbsentCardID),
		PositiveControlOutcome == ESpellVFXOutcome::Present && SyntheticOutcome != ESpellVFXOutcome::Present);

	// ══ THE REPORT ══════════════════════════════════════════════════════════
	// Published under the SAME predicate it was measured under (`SC-§49`), with the
	// two findings NAMED and SEPARATE — never summarised as "no VFX".

	AddInfo(FString::Printf(TEXT("SPELL VFX ROSTER — %d card row(s) read; %d spell(s) walked (%d named by the card table, %d named by cards.csv); %d PRESENT; %d ABSENT; %d WRONG TYPE."),
		TotalRows, WalkOrder.Num(), TableSpellEffectRows.Num(), CsvSpellRows.Num(), PresentCount, AbsentCount, WrongTypeCount));

	for (const FString& CardID : WalkOrder)
	{
		const FSpellCandidate& Candidate = Candidates[CardID];
		AddInfo(FString::Printf(TEXT("  spell %-14s CardType %-10s SpellEffect %-18s source: %s%s"),
			*CardID, *Candidate.CardTypeSymbolText, *Candidate.SpellEffectSymbolText,
			Candidate.bFromCardTable ? TEXT("card-table") : TEXT("          "),
			Candidate.bFromCsv ? TEXT(" + cards.csv") : TEXT("")));
	}

	if (AbsentCount > 0)
	{
		AddInfo(FString::Printf(TEXT("⛔ FINDING (a) ABSENT — %d spell(s) whose cast VFX does not exist: %s   [repair: AUTHOR the Niagara system at the named path]"),
			AbsentCount, *FString::Join(AbsentCardIDs, TEXT(", "))));
	}

	if (WrongTypeCount > 0)
	{
		AddInfo(FString::Printf(TEXT("⛔ FINDING (b) WRONG TYPE — %d spell(s) whose cast VFX path holds a non-Niagara asset: %s   [repair: REPLACE the impostor, not author a second asset]"),
			WrongTypeCount, *FString::Join(WrongTypeCardIDs, TEXT(", "))));
	}

	AddInfo(FString::Printf(TEXT("POSITIVE CONTROL — ProbeSpellVFX('%s' -> '%s') = %s"),
		PositiveControlCardID, *PositiveControlPath, SpellVFXOutcomeSymbol(PositiveControlOutcome)));
	AddInfo(FString::Printf(TEXT("SYNTHESISED RED  — ProbeSpellVFX('%s' -> '%s') = %s   [in-memory string; no asset touched]"),
		SyntheticAbsentCardID, *SyntheticPath, SpellVFXOutcomeSymbol(SyntheticOutcome)));
	AddInfo(TEXT("⭐ ⇒ the spell-VFX probe answered PRESENT for a shipped system and ABSENT for a measured-absent one, through ONE symbol, in ONE run. The walk above asserts that same probe returns PRESENT for every spell the DATA names — never for a list this file keeps."));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
