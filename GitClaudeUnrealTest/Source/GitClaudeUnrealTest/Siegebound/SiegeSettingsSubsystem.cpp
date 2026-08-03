// Copyright Epic Games, Inc. All Rights Reserved.

#include "Siegebound/SiegeSettingsSubsystem.h"

#include "Kismet/GameplayStatics.h"
#include "Siegebound/SiegeSettingsSaveGame.h"
#include "UObject/UObjectGlobals.h"

// The ONE definition of the settings log category (CONVENTIONS "Settings
// screen…" §2 — LogSiegeSettings lives in SiegeSettingsSubsystem.{h,cpp}).
DEFINE_LOG_CATEGORY(LogSiegeSettings);

// ⚠️ CHARACTER-FOR-CHARACTER, AND DEFINED EXACTLY ONCE. Changing this string
// silently orphans every player's saved settings — they revert to defaults with
// no error and no warning. An automation test asserts it.
const TCHAR* USiegeSettingsSubsystem::SettingsSlotName = TEXT("SiegeSettings");

// The delegate payload for the confirm toggle. Same token as the SaveGame field
// and as CONVENTIONS §2's name for the setting, so all three stay greppable.
const FName USiegeSettingsSubsystem::SettingName_AssistantConfirmBeforeExecute(TEXT("bAssistantConfirmBeforeExecute"));

void USiegeSettingsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// LOAD ONCE. This is the only disk read on any shipped path — every later
	// read is the in-memory getter (§2: a LoadGameFromSlot on the order path
	// would be a disk hit inside a mid-battle interaction).
	LoadSettingsFromSlot();

	UE_LOG(LogSiegeSettings, Log,
		TEXT("[SiegeSettings] Subsystem initialized — slot '%s' (user %d), bAssistantConfirmBeforeExecute=%s."),
		*ResolveSlotName(), SettingsUserIndex, bAssistantConfirmBeforeExecute ? TEXT("true") : TEXT("false"));
}

bool USiegeSettingsSubsystem::IsAssistantConfirmEnabled() const
{
	// PURE IN-MEMORY. No disk, no allocation, no world — safe to call on the
	// order path at confirm time.
	return bAssistantConfirmBeforeExecute;
}

void USiegeSettingsSubsystem::SetAssistantConfirmEnabled(bool bEnabled)
{
	// Everything (the no-op check, the save, the broadcast) lives in the one
	// mutation path so setting #2 cannot re-derive it slightly differently.
	ApplyBoolSetting(bAssistantConfirmBeforeExecute, bEnabled, SettingName_AssistantConfirmBeforeExecute, /*bPersistToDisk*/ true);
}

void USiegeSettingsSubsystem::LoadSettingsFromSlot()
{
	const FString SlotName = ResolveSlotName();

	// DoesSaveGameExist FIRST so the normal first run (nothing saved yet) is
	// SILENT — LoadGameFromSlot on a missing slot emits an engine warning of its
	// own (the UDeckBuilderWidget::LoadSaveGame idiom, copied not reinvented).
	USiegeSettingsSaveGame* Loaded = nullptr;
	if (UGameplayStatics::DoesSaveGameExist(SlotName, SettingsUserIndex))
	{
		// Cast, never assume: a corrupt or FOREIGN-CLASS slot casts to nullptr
		// rather than crashing (the TASK-113 carry-forward).
		Loaded = Cast<USiegeSettingsSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, SettingsUserIndex));
	}

	// ⚠️ THE FALLBACK IS READ OFF THE SaveGame CLASS'S CDO, NOT RE-TYPED HERE.
	// USiegeSettingsSaveGame's C++ default IS the fallback contract; deriving it
	// means the two cannot drift when a default is retuned.
	const USiegeSettingsSaveGame* Source = Loaded ? Loaded : GetDefault<USiegeSettingsSaveGame>();

	if (!Loaded && !bLoadFallbackLogged)
	{
		// LOG ONCE (§3). Log, not Warning: an absent slot is the normal first
		// run, not a fault.
		bLoadFallbackLogged = true;
		UE_LOG(LogSiegeSettings, Log,
			TEXT("[SiegeSettings] No readable settings in slot '%s' (user %d) — falling back to C++ defaults. Normal on a first run; also covers an unreadable or foreign-class slot."),
			*SlotName, SettingsUserIndex);
	}

	// Applied through the SAME mutation path a setter uses, but WITHOUT
	// persisting: a loaded value equal to what is in memory is a no-op and does
	// not broadcast; a differing one broadcasts exactly once. At Initialize no
	// consumer can be bound yet, so in practice this is silent — it stays
	// correct anyway if a later caller reloads.
	ApplyBoolSetting(bAssistantConfirmBeforeExecute, Source->bAssistantConfirmBeforeExecute,
		SettingName_AssistantConfirmBeforeExecute, /*bPersistToDisk*/ false);
}

void USiegeSettingsSubsystem::SetSlotNameForAutomationTests(const FString& InSlotName)
{
	// ⛔ AUTOMATION ONLY. Exists so a test run can never overwrite the player's
	// real Saved/SaveGames/SiegeSettings.sav.
	SlotNameOverride = InSlotName;
}

void USiegeSettingsSubsystem::ApplyBoolSetting(bool& OutValue, bool bNewValue, FName SettingName, bool bPersistToDisk)
{
	if (OutValue == bNewValue)
	{
		// ⛔ NO-OP WRITE: no disk hit and NO BROADCAST (the delegate law + the
		// qa/TASK-005 major-2 lesson — a delegate that fires on refused or
		// meaningless mutations trains its consumers to ignore it).
		//
		// Note the deliberate consequence: a same-value write also skips the
		// save, so a first run where the player "sets" the value it already has
		// writes no file. That is not a gap — an absent slot loads as exactly
		// that default.
		UE_LOG(LogSiegeSettings, Verbose,
			TEXT("[SiegeSettings] '%s' is already %s — no-op (no save, no broadcast)."),
			*SettingName.ToString(), bNewValue ? TEXT("true") : TEXT("false"));
		return;
	}

	OutValue = bNewValue;

	if (bPersistToDisk)
	{
		// A failed write logs a Warning inside SaveSettingsToSlot and is
		// DELIBERATELY NOT reverted: the player's choice takes effect for this
		// session. Snapping the checkbox back under their finger because a disk
		// write failed is the worse of the two behaviours.
		SaveSettingsToSlot();
	}

	BroadcastSettingChanged(SettingName);
}

bool USiegeSettingsSubsystem::SaveSettingsToSlot() const
{
	const FString SlotName = ResolveSlotName();

	USiegeSettingsSaveGame* SaveObj = Cast<USiegeSettingsSaveGame>(
		UGameplayStatics::CreateSaveGameObject(USiegeSettingsSaveGame::StaticClass()));
	if (!SaveObj)
	{
		UE_LOG(LogSiegeSettings, Warning,
			TEXT("[SiegeSettings] SaveSettingsToSlot: could not create a USiegeSettingsSaveGame — slot '%s' NOT written."), *SlotName);
		return false;
	}

	SaveObj->bAssistantConfirmBeforeExecute = bAssistantConfirmBeforeExecute;

	if (!UGameplayStatics::SaveGameToSlot(SaveObj, SlotName, SettingsUserIndex))
	{
		UE_LOG(LogSiegeSettings, Warning,
			TEXT("[SiegeSettings] SaveGameToSlot('%s', user %d) FAILED — the change is live for this session but was NOT persisted."),
			*SlotName, SettingsUserIndex);
		return false;
	}

	UE_LOG(LogSiegeSettings, Log,
		TEXT("[SiegeSettings] Saved slot '%s' (user %d): bAssistantConfirmBeforeExecute=%s."),
		*SlotName, SettingsUserIndex, bAssistantConfirmBeforeExecute ? TEXT("true") : TEXT("false"));
	return true;
}

void USiegeSettingsSubsystem::BroadcastSettingChanged(FName SettingName)
{
	// ⚠️ THE ONLY OnSettingsChanged.Broadcast IN THIS FILE. The counter is bumped
	// here and nowhere else, which is what lets an automation test observe "did
	// it broadcast?" without a bound UFUNCTION listener (a dynamic multicast
	// needs one, and a UCLASS cannot be declared in a test .cpp).
	++SettingsChangeBroadcastCount;
	LastBroadcastSettingName = SettingName;

	UE_LOG(LogSiegeSettings, Log,
		TEXT("[SiegeSettings] '%s' changed — broadcasting OnSettingsChanged (broadcast #%d)."),
		*SettingName.ToString(), SettingsChangeBroadcastCount);

	OnSettingsChanged.Broadcast(SettingName);
}

FString USiegeSettingsSubsystem::ResolveSlotName() const
{
	// SlotNameOverride is empty on every shipped path — only an automation test
	// ever sets it (SetSlotNameForAutomationTests).
	return SlotNameOverride.IsEmpty() ? FString(SettingsSlotName) : SlotNameOverride;
}
