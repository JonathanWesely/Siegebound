// Copyright Epic Games, Inc. All Rights Reserved.

#include "SiegeAssistantCommand.h"

#include "SiegeAssistantGrammar.h"

#include "Containers/StringView.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "UObject/Class.h"
#include "UObject/ReflectedTypeAccessors.h"

DEFINE_LOG_CATEGORY(LogSiegeAssistant);

namespace
{
	/** Builds a "code:detail" reason string. The FSM compares only the code half. */
	FString WithDetail(const TCHAR* Code, const FString& Detail)
	{
		return FString(Code) + TEXT(":") + Detail;
	}

	/** Strips any "EnumName::" qualifier UEnum may return for a scoped enum entry. */
	FString ShortEnumEntryName(const FString& EntryName)
	{
		int32 SeparatorIndex = INDEX_NONE;
		if (EntryName.FindLastChar(TEXT(':'), SeparatorIndex))
		{
			return EntryName.RightChop(SeparatorIndex + 1);
		}

		return EntryName;
	}

	/** True for the UHT-generated sentinel entry, which is never part of the wire vocabulary. */
	bool IsEnumMaxEntry(const FString& EntryName)
	{
		return EntryName.EndsWith(TEXT("_MAX"), ESearchCase::CaseSensitive);
	}

	/**
	 *  ⚠️ THE ONLY TWO PLACES IN THIS FILE THAT TOUCH A JSON KEY'S CONCRETE TYPE.
	 *
	 *  A JSON key is NOT an FString. In UE 5.8 `FJsonObject::Values` is a
	 *  `TMap<FJsonObject::FStringType, TSharedPtr<FJsonValue>>` where
	 *  `FStringType` is **`UE::FSharedString`** — keys are interned in a shared
	 *  string set so that many objects parsed from one document share one copy of
	 *  each key. `UE::FSharedString` has no `Equals` and does not convert to
	 *  FString.
	 *
	 *  THIS IS THE SAME ROOT CAUSE AS THE C4172 ABOVE, NOW NAMED BY THE COMPILER.
	 *  The old `TPair<FString, TSharedPtr<FJsonValue>>&` loop variable was
	 *  converting the key from `UE::FSharedString` to `FString` on every
	 *  iteration — which is what built the temporary pair whose address escaped.
	 *  Removing that conversion fixed the dangling pointer AND stopped silently
	 *  papering over the real key type at each use site.
	 *
	 *  ⚠️ NEITHER HELPER NAMES THE CONCRETE TYPE, DELIBERATELY. Both go through
	 *  `FJsonObject::FStringType`, and both use only `operator*` and `Len()` —
	 *  the two operations `UE::FSharedString` and `FString` BOTH provide. The
	 *  engine picks between them with `UE_JSONOBJECT_LEGACY_STRING_KEYS`
	 *  (JsonObject.h:240 keeps an FString-keyed storage class alive for now), so
	 *  this file compiles either way and will survive Epic flipping that switch.
	 *  Do not "simplify" these into FString conversions at the call sites.
	 */
	FStringView JsonKeyView(const FJsonObject::FStringType& Key)
	{
		return FStringView(*Key, Key.Len());
	}

	/** The same key as an OWNING FString, for a reason-code payload. Error paths only. */
	FString JsonKeyString(const FJsonObject::FStringType& Key)
	{
		return FString(*Key);
	}

	/**
	 *  CASE-SENSITIVE presence test.
	 *
	 *  ⚠️ FJsonObject::HasField / TryGetXField cannot be used for strictness:
	 *  FJsonObject::Values is a TMap keyed by FString, and FString's map key
	 *  comparison is CASE-INSENSITIVE, so HasField(TEXT("intent")) happily
	 *  matches a key spelled "Intent". JSON keys are case-sensitive and the
	 *  grammar only ever emits lower case, so anything else is off-grammar output
	 *  and must be reported as an unknown key rather than quietly accepted.
	 *
	 *  ⚠️ NOTHING IN THIS FILE RETURNS A POINTER OR REFERENCE INTO Values, AND
	 *  THAT IS DELIBERATE (fix for C4172, caught by TASK-401's gate). The earlier
	 *  version of this helper returned `&Field.Value` from the loop below and was
	 *  a real dangling pointer: TMap's iterator does not yield exactly
	 *  `TPair<FString, TSharedPtr<FJsonValue>>&`, so naming that type as the loop
	 *  variable materialised a CONVERTED TEMPORARY pair each iteration and the
	 *  returned address pointed into it — a temporary that dies at the end of the
	 *  iteration. It survived casual reading because the memory usually still
	 *  held the right bytes; under optimisation it would not have.
	 *
	 *  The lesson is encoded structurally rather than by comment: presence is
	 *  answered by a bool, and values are handed back by VALUE as a TSharedPtr,
	 *  which is a shared OWNER whose lifetime does not depend on the map at all.
	 *  The loops use `auto&` so no conversion — and so no temporary — can occur
	 *  whatever element type the container iterates as.
	 */
	bool HasFieldExact(const TSharedPtr<FJsonObject>& Object, const TCHAR* Key)
	{
		for (const auto& Field : Object->Values)
		{
			if (JsonKeyView(Field.Key).Equals(Key, ESearchCase::CaseSensitive))
			{
				return true;
			}
		}

		return false;
	}

	/**
	 *  CASE-SENSITIVE lookup returning an OWNING handle, invalid when the key is
	 *  absent. Every reader below takes its value through this, so no read
	 *  depends on a preceding key-set check having been run — each one is
	 *  independently null-safe and reports bad_type rather than dereferencing
	 *  nothing.
	 *
	 *  Presence and validity stay distinct: a key explicitly present with a JSON
	 *  `null` yields a VALID pointer to an FJsonValueNull, so it is reported as
	 *  bad_type rather than as a missing key.
	 */
	TSharedPtr<FJsonValue> FindFieldValue(const TSharedPtr<FJsonObject>& Object, const TCHAR* Key)
	{
		for (const auto& Field : Object->Values)
		{
			if (JsonKeyView(Field.Key).Equals(Key, ESearchCase::CaseSensitive))
			{
				return Field.Value;
			}
		}

		return TSharedPtr<FJsonValue>();
	}

	/** Rejects every key of Object that is not in AllowedKeys, and every AllowedKey that is absent. */
	bool ValidateExactKeySet(const TSharedPtr<FJsonObject>& Object, const TArray<const TCHAR*>& AllowedKeys, FString& OutError)
	{
		// `auto&` for the same reason as above: naming the pair type explicitly
		// converts, and a conversion here would copy an FString and churn a
		// TSharedPtr refcount for every key of every object parsed.
		for (const auto& Field : Object->Values)
		{
			const FStringView FieldKey = JsonKeyView(Field.Key);

			bool bAllowed = false;
			for (const TCHAR* AllowedKey : AllowedKeys)
			{
				if (FieldKey.Equals(AllowedKey, ESearchCase::CaseSensitive))
				{
					bAllowed = true;
					break;
				}
			}

			if (!bAllowed)
			{
				OutError = WithDetail(SiegeAssistantReason::UnknownKey, JsonKeyString(Field.Key));
				return false;
			}
		}

		for (const TCHAR* AllowedKey : AllowedKeys)
		{
			if (!HasFieldExact(Object, AllowedKey))
			{
				OutError = WithDetail(SiegeAssistantReason::MissingKey, FString(AllowedKey));
				return false;
			}
		}

		return true;
	}

	/** A JSON number that is exactly an integer and fits int32. Rejects 5.5, 1e40 and "5". */
	bool TryGetStrictInt(const TSharedPtr<FJsonValue>& Value, int32& OutInt)
	{
		if (!Value.IsValid() || Value->Type != EJson::Number)
		{
			return false;
		}

		const double Number = Value->AsNumber();
		if (Number != FMath::TruncToDouble(Number))
		{
			return false;
		}

		if (Number < static_cast<double>(MIN_int32) || Number > static_cast<double>(MAX_int32))
		{
			return false;
		}

		OutInt = static_cast<int32>(Number);
		return true;
	}

	/**
	 *  Reads a quantity: an integer in 1..GrammarCountMax, or the string "all"
	 *  (mapped to 0) when bAllowAll.
	 *
	 *  bAllowAll is false for the deferred trigger's `at_least`, where "all" is
	 *  not a condition that can ever become true. The grammar already makes it
	 *  unsayable; this is the receive-side half of the same rule.
	 */
	bool ParseQuantity(const TSharedPtr<FJsonValue>& Value, bool bAllowAll, int32& OutCount, FString& OutError)
	{
		if (!Value.IsValid())
		{
			OutError = WithDetail(SiegeAssistantReason::BadType, FString(SiegeAssistantJsonKeys::N));
			return false;
		}

		if (Value->Type == EJson::String)
		{
			const FString Text = Value->AsString();
			if (bAllowAll && Text.Equals(SiegeAssistantSymbols::All, ESearchCase::IgnoreCase))
			{
				OutCount = 0;
				return true;
			}

			OutError = WithDetail(SiegeAssistantReason::CountOutOfRange, Text);
			return false;
		}

		int32 Quantity = 0;
		if (!TryGetStrictInt(Value, Quantity))
		{
			OutError = WithDetail(SiegeAssistantReason::CountOutOfRange,
				Value->Type == EJson::Number ? FString::SanitizeFloat(Value->AsNumber()) : FString(TEXT("not_a_number")));
			return false;
		}

		if (Quantity < USiegeAssistantGrammar::GrammarCountMin || Quantity > USiegeAssistantGrammar::GrammarCountMax)
		{
			OutError = WithDetail(SiegeAssistantReason::CountOutOfRange, FString::FromInt(Quantity));
			return false;
		}

		OutCount = Quantity;
		return true;
	}

	/** Reads a unit-kind symbol. Empty and the reserved "none" are both rejected. */
	bool ParseKindSymbol(const TSharedPtr<FJsonValue>& Value, FName& OutKind, FString& OutError)
	{
		if (!Value.IsValid() || Value->Type != EJson::String)
		{
			OutError = WithDetail(SiegeAssistantReason::BadType, FString(SiegeAssistantJsonKeys::Kind));
			return false;
		}

		const FString Symbol = Value->AsString().TrimStartAndEnd().ToLower();
		if (Symbol.IsEmpty() || Symbol.Equals(SiegeAssistantSymbols::None, ESearchCase::IgnoreCase))
		{
			OutError = WithDetail(SiegeAssistantReason::BadKind, Symbol);
			return false;
		}

		OutKind = FName(*Symbol);
		return true;
	}
}

FString SiegeAssistantReasonCode(const FString& Reason)
{
	int32 SeparatorIndex = INDEX_NONE;
	if (Reason.FindChar(TEXT(':'), SeparatorIndex))
	{
		return Reason.Left(SeparatorIndex);
	}

	return Reason;
}

FString SiegeAssistantIntentToSymbol(ESiegeAssistantIntent Intent)
{
	if (Intent == ESiegeAssistantIntent::None)
	{
		return FString();
	}

	const UEnum* IntentEnum = StaticEnum<ESiegeAssistantIntent>();
	if (IntentEnum == nullptr)
	{
		return FString();
	}

	const int32 EntryIndex = IntentEnum->GetIndexByValue(static_cast<int64>(Intent));
	if (EntryIndex == INDEX_NONE)
	{
		return FString();
	}

	return ShortEnumEntryName(IntentEnum->GetNameStringByIndex(EntryIndex)).ToLower();
}

bool SiegeAssistantIntentFromSymbol(const FString& Symbol, ESiegeAssistantIntent& OutIntent)
{
	const FString Wanted = Symbol.TrimStartAndEnd().ToLower();
	if (Wanted.IsEmpty())
	{
		return false;
	}

	const UEnum* IntentEnum = StaticEnum<ESiegeAssistantIntent>();
	if (IntentEnum == nullptr)
	{
		return false;
	}

	const int32 EntryCount = IntentEnum->NumEnums();
	for (int32 EntryIndex = 0; EntryIndex < EntryCount; ++EntryIndex)
	{
		const FString EntryName = ShortEnumEntryName(IntentEnum->GetNameStringByIndex(EntryIndex));
		if (EntryName.IsEmpty() || IsEnumMaxEntry(EntryName))
		{
			continue;
		}

		const int64 EntryValue = IntentEnum->GetValueByIndex(EntryIndex);
		if (EntryValue == static_cast<int64>(ESiegeAssistantIntent::None))
		{
			continue;
		}

		if (EntryName.ToLower() == Wanted)
		{
			OutIntent = static_cast<ESiegeAssistantIntent>(EntryValue);
			return true;
		}
	}

	return false;
}

void SiegeAssistantIntentSymbols(TArray<FString>& OutSymbols)
{
	OutSymbols.Reset();

	const UEnum* IntentEnum = StaticEnum<ESiegeAssistantIntent>();
	if (IntentEnum == nullptr)
	{
		return;
	}

	const int32 EntryCount = IntentEnum->NumEnums();
	OutSymbols.Reserve(EntryCount);

	for (int32 EntryIndex = 0; EntryIndex < EntryCount; ++EntryIndex)
	{
		const FString EntryName = ShortEnumEntryName(IntentEnum->GetNameStringByIndex(EntryIndex));
		if (EntryName.IsEmpty() || IsEnumMaxEntry(EntryName))
		{
			continue;
		}

		if (IntentEnum->GetValueByIndex(EntryIndex) == static_cast<int64>(ESiegeAssistantIntent::None))
		{
			continue;
		}

		OutSymbols.Add(EntryName.ToLower());
	}
}

void SiegeAssistantAskCodes(TArray<FString>& OutCodes)
{
	OutCodes.Reset();
	OutCodes.Add(FString(SiegeAssistantAsk::WhichUnit));
	OutCodes.Add(FString(SiegeAssistantAsk::WhichPlace));
	OutCodes.Add(FString(SiegeAssistantAsk::HowMany));
	OutCodes.Add(FString(SiegeAssistantAsk::WhichIntent));
	OutCodes.Add(FString(SiegeAssistantAsk::Unsupported));
}

bool SiegeAssistantIntentTakesSelection(ESiegeAssistantIntent Intent)
{
	switch (Intent)
	{
	case ESiegeAssistantIntent::Send:
	case ESiegeAssistantIntent::Guard:
	case ESiegeAssistantIntent::Ambush:
	case ESiegeAssistantIntent::Follow:
		return true;

	default:
		// Charge / Fallback / Rally are army-wide or hero-only: they carry no
		// selection at all (CONVENTIONS §8 executor seam). None reaches here too.
		return false;
	}
}

bool SiegeAssistantValidateSelection(const TArray<FName>& Kinds, const TArray<int32>& Counts, FString& OutError)
{
	OutError.Reset();

	// ⚠️ A MISMATCH IS A FAILURE, NEVER A TRUNCATION. Trimming the longer array to
	// the shorter one would produce a perfectly well-formed order that is not the
	// one anybody asked for — the exact failure class this design exists to stop.
	if (Kinds.Num() != Counts.Num())
	{
		OutError = WithDetail(SiegeAssistantReason::SelectionMismatch,
			FString::FromInt(Kinds.Num()) + TEXT("/") + FString::FromInt(Counts.Num()));
		return false;
	}

	if (Kinds.Num() > SiegeAssistantMaxSelectionKinds)
	{
		OutError = WithDetail(SiegeAssistantReason::SelectionOverflow, FString::FromInt(Kinds.Num()));
		return false;
	}

	for (int32 Index = 0; Index < Kinds.Num(); ++Index)
	{
		if (Kinds[Index].IsNone())
		{
			OutError = WithDetail(SiegeAssistantReason::BadKind, FString::FromInt(Index));
			return false;
		}

		// A repeated kind would leave the executor holding two quantities for one
		// unit type with no rule for which wins, so one of them would be silently
		// discarded. Reject instead.
		for (int32 EarlierIndex = 0; EarlierIndex < Index; ++EarlierIndex)
		{
			if (Kinds[EarlierIndex] == Kinds[Index])
			{
				OutError = WithDetail(SiegeAssistantReason::DuplicateKind, Kinds[Index].ToString());
				return false;
			}
		}
	}

	return true;
}

bool ParseSiegeAssistantCommand(const FString& Json, FSiegeAssistantCommand& OutCommand, FString& OutError)
{
	// Reset on ENTRY as well as on every failure path, so a caller that ignores
	// the return value is left holding an inert None command rather than the
	// wreckage of a half-parsed order.
	OutCommand = FSiegeAssistantCommand();
	OutError.Reset();

	auto Fail = [&OutCommand, &OutError](const FString& Reason) -> bool
	{
		OutCommand = FSiegeAssistantCommand();
		OutError = Reason;
		UE_LOG(LogSiegeAssistant, Verbose, TEXT("ParseSiegeAssistantCommand rejected: %s"), *Reason);
		return false;
	};

	const FString Trimmed = Json.TrimStartAndEnd();
	if (Trimmed.IsEmpty())
	{
		return Fail(FString(SiegeAssistantReason::EmptyInput));
	}

	TSharedPtr<FJsonObject> RootObject;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Trimmed);
	if (!FJsonSerializer::Deserialize(Reader, RootObject) || !RootObject.IsValid())
	{
		return Fail(FString(SiegeAssistantReason::MalformedJson));
	}

	// --- the question branch --------------------------------------------------
	// The model declined to produce a command. Not a malformed result and not a
	// model failure — it is the one honest answer the grammar always leaves
	// reachable, and it returns FALSE because a question is not executable. The
	// FSM routes "ask:*" to Clarify, everything else to Failed.
	if (HasFieldExact(RootObject, SiegeAssistantJsonKeys::Ask))
	{
		TArray<const TCHAR*> QuestionKeys;
		QuestionKeys.Add(SiegeAssistantJsonKeys::Ask);

		FString KeyError;
		if (!ValidateExactKeySet(RootObject, QuestionKeys, KeyError))
		{
			return Fail(KeyError);
		}

		const TSharedPtr<FJsonValue> AskValue = FindFieldValue(RootObject, SiegeAssistantJsonKeys::Ask);
		if (!AskValue.IsValid() || AskValue->Type != EJson::String)
		{
			return Fail(WithDetail(SiegeAssistantReason::BadType, FString(SiegeAssistantJsonKeys::Ask)));
		}

		const FString AskCode = AskValue->AsString().TrimStartAndEnd().ToLower();

		TArray<FString> KnownAskCodes;
		SiegeAssistantAskCodes(KnownAskCodes);
		if (!KnownAskCodes.Contains(AskCode))
		{
			return Fail(WithDetail(SiegeAssistantReason::UnknownAsk, AskCode));
		}

		return Fail(WithDetail(SiegeAssistantReason::Ask, AskCode));
	}

	// --- the command branch: exact key set ------------------------------------
	TArray<const TCHAR*> CommandKeys;
	CommandKeys.Add(SiegeAssistantJsonKeys::Intent);
	CommandKeys.Add(SiegeAssistantJsonKeys::Who);
	CommandKeys.Add(SiegeAssistantJsonKeys::Where);
	CommandKeys.Add(SiegeAssistantJsonKeys::When);

	{
		FString KeyError;
		if (!ValidateExactKeySet(RootObject, CommandKeys, KeyError))
		{
			return Fail(KeyError);
		}
	}

	FSiegeAssistantCommand Parsed;

	// --- intent ---------------------------------------------------------------
	{
		const TSharedPtr<FJsonValue> IntentValue = FindFieldValue(RootObject, SiegeAssistantJsonKeys::Intent);
		if (!IntentValue.IsValid() || IntentValue->Type != EJson::String)
		{
			return Fail(WithDetail(SiegeAssistantReason::BadType, FString(SiegeAssistantJsonKeys::Intent)));
		}

		const FString IntentSymbol = IntentValue->AsString().TrimStartAndEnd();
		if (!SiegeAssistantIntentFromSymbol(IntentSymbol, Parsed.Intent))
		{
			return Fail(WithDetail(SiegeAssistantReason::UnknownIntent, IntentSymbol.ToLower()));
		}
	}

	// --- who ------------------------------------------------------------------
	bool bWhoIsNone = false;
	{
		const TSharedPtr<FJsonValue> Who = FindFieldValue(RootObject, SiegeAssistantJsonKeys::Who);
		if (!Who.IsValid())
		{
			return Fail(WithDetail(SiegeAssistantReason::BadType, FString(SiegeAssistantJsonKeys::Who)));
		}

		if (Who->Type == EJson::String)
		{
			const FString Selector = Who->AsString().TrimStartAndEnd().ToLower();
			if (Selector.Equals(SiegeAssistantSymbols::All, ESearchCase::IgnoreCase))
			{
				// Empty Kinds/Counts == "every eligible unit".
			}
			else if (Selector.Equals(SiegeAssistantSymbols::None, ESearchCase::IgnoreCase))
			{
				bWhoIsNone = true;
			}
			else
			{
				return Fail(WithDetail(SiegeAssistantReason::BadWho, Selector));
			}
		}
		else if (Who->Type == EJson::Array)
		{
			const TArray<TSharedPtr<FJsonValue>>& Items = Who->AsArray();

			if (Items.Num() < 1 || Items.Num() > SiegeAssistantMaxSelectionKinds)
			{
				return Fail(WithDetail(SiegeAssistantReason::WhoArity, FString::FromInt(Items.Num())));
			}

			Parsed.Kinds.Reserve(Items.Num());
			Parsed.Counts.Reserve(Items.Num());

			TArray<const TCHAR*> ItemKeys;
			ItemKeys.Add(SiegeAssistantJsonKeys::Kind);
			ItemKeys.Add(SiegeAssistantJsonKeys::N);

			for (const TSharedPtr<FJsonValue>& Item : Items)
			{
				if (!Item.IsValid() || Item->Type != EJson::Object)
				{
					return Fail(WithDetail(SiegeAssistantReason::BadType, FString(SiegeAssistantJsonKeys::Who)));
				}

				const TSharedPtr<FJsonObject> ItemObject = Item->AsObject();
				if (!ItemObject.IsValid())
				{
					return Fail(WithDetail(SiegeAssistantReason::BadType, FString(SiegeAssistantJsonKeys::Who)));
				}

				FString ItemKeyError;
				if (!ValidateExactKeySet(ItemObject, ItemKeys, ItemKeyError))
				{
					return Fail(ItemKeyError);
				}

				FName ItemKind = NAME_None;
				FString ItemError;
				if (!ParseKindSymbol(FindFieldValue(ItemObject, SiegeAssistantJsonKeys::Kind), ItemKind, ItemError))
				{
					return Fail(ItemError);
				}

				int32 ItemCount = 0;
				if (!ParseQuantity(FindFieldValue(ItemObject, SiegeAssistantJsonKeys::N), /*bAllowAll*/ true, ItemCount, ItemError))
				{
					return Fail(ItemError);
				}

				Parsed.Kinds.Add(ItemKind);
				Parsed.Counts.Add(ItemCount);
			}
		}
		else
		{
			return Fail(WithDetail(SiegeAssistantReason::BadType, FString(SiegeAssistantJsonKeys::Who)));
		}
	}

	// --- where ----------------------------------------------------------------
	// The place symbol is NOT validated against a place list here: this function
	// is pure and has no world. Resolvability is the executor's question, asked
	// through USiegeAssistantSnapshot::ResolvePlace.
	{
		const TSharedPtr<FJsonValue> WhereValue = FindFieldValue(RootObject, SiegeAssistantJsonKeys::Where);
		if (!WhereValue.IsValid() || WhereValue->Type != EJson::String)
		{
			return Fail(WithDetail(SiegeAssistantReason::BadType, FString(SiegeAssistantJsonKeys::Where)));
		}

		const FString PlaceSymbol = WhereValue->AsString().TrimStartAndEnd().ToLower();
		if (PlaceSymbol.IsEmpty())
		{
			return Fail(WithDetail(SiegeAssistantReason::BadWhere, PlaceSymbol));
		}

		if (!PlaceSymbol.Equals(SiegeAssistantSymbols::None, ESearchCase::IgnoreCase))
		{
			Parsed.Where = FName(*PlaceSymbol);
		}
	}

	// --- when -----------------------------------------------------------------
	{
		const TSharedPtr<FJsonValue> When = FindFieldValue(RootObject, SiegeAssistantJsonKeys::When);
		if (!When.IsValid())
		{
			return Fail(WithDetail(SiegeAssistantReason::BadType, FString(SiegeAssistantJsonKeys::When)));
		}

		if (When->Type == EJson::String)
		{
			const FString WhenSymbol = When->AsString().TrimStartAndEnd().ToLower();
			if (!WhenSymbol.Equals(SiegeAssistantSymbols::Now, ESearchCase::IgnoreCase))
			{
				return Fail(WithDetail(SiegeAssistantReason::BadWhen, WhenSymbol));
			}

			// "now" leaves TriggerKind = NAME_None and TriggerAtLeast = 0.
		}
		else if (When->Type == EJson::Object)
		{
			const TSharedPtr<FJsonObject> TriggerObject = When->AsObject();
			if (!TriggerObject.IsValid())
			{
				return Fail(WithDetail(SiegeAssistantReason::BadType, FString(SiegeAssistantJsonKeys::When)));
			}

			TArray<const TCHAR*> TriggerKeys;
			TriggerKeys.Add(SiegeAssistantJsonKeys::Kind);
			TriggerKeys.Add(SiegeAssistantJsonKeys::AtLeast);

			FString TriggerKeyError;
			if (!ValidateExactKeySet(TriggerObject, TriggerKeys, TriggerKeyError))
			{
				return Fail(TriggerKeyError);
			}

			FString TriggerError;
			if (!ParseKindSymbol(FindFieldValue(TriggerObject, SiegeAssistantJsonKeys::Kind), Parsed.TriggerKind, TriggerError))
			{
				return Fail(TriggerError);
			}

			// bAllowAll is FALSE: "wait until I have all footmen" is a condition
			// that can never become true, so it is not accepted here.
			if (!ParseQuantity(FindFieldValue(TriggerObject, SiegeAssistantJsonKeys::AtLeast), /*bAllowAll*/ false, Parsed.TriggerAtLeast, TriggerError))
			{
				return Fail(TriggerError);
			}
		}
		else
		{
			return Fail(WithDetail(SiegeAssistantReason::BadType, FString(SiegeAssistantJsonKeys::When)));
		}
	}

	// --- THE ONE CROSS-FIELD CHECK -------------------------------------------
	// `who` = "none" and `who` = "all" both land on an empty Kinds/Counts pair,
	// because the struct has no field that distinguishes them. For an army-wide
	// verb that is harmless (the executor ignores the selection). For a
	// selection-bearing verb it would silently turn "send nobody" into "send
	// everybody" — so it is rejected here rather than allowed to become a
	// well-formed order nobody asked for. Every other cross-field question is
	// the executor's.
	if (bWhoIsNone && SiegeAssistantIntentTakesSelection(Parsed.Intent))
	{
		return Fail(WithDetail(SiegeAssistantReason::WhoRequired, SiegeAssistantIntentToSymbol(Parsed.Intent)));
	}

	// --- the selection invariants --------------------------------------------
	{
		FString SelectionError;
		if (!SiegeAssistantValidateSelection(Parsed.Kinds, Parsed.Counts, SelectionError))
		{
			return Fail(SelectionError);
		}
	}

	OutCommand = MoveTemp(Parsed);
	return true;
}
