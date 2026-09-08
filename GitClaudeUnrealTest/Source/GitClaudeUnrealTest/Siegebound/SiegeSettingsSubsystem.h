// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "SiegeSettingsSubsystem.generated.h"

/**
 *  The settings log category (CONVENTIONS "Settings screen + the assistant
 *  CONFIRM STEP + the non-orderable-kind guard (2026-08-03)" §2: declared and
 *  defined in SiegeSettingsSubsystem.{h,cpp} by law).
 */
DECLARE_LOG_CATEGORY_EXTERN(LogSiegeSettings, Log, All);

/**
 *  Fired when a setting's value ACTUALLY CHANGES, carrying WHICH setting changed
 *  so one delegate serves every future setting (the delegate law, CONVENTIONS
 *  "Delegates (C++)"). ⛔ Never broadcast on a no-op write — a delegate that
 *  fires on refused mutations trains consumers to ignore it.
 *
 *  For the confirm toggle the payload is
 *  USiegeSettingsSubsystem::SettingName_AssistantConfirmBeforeExecute, which is
 *  the FName TEXT("bAssistantConfirmBeforeExecute").
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSiegeSettingsChanged, FName, SettingName);

/**
 *  THE PERSISTED SETTINGS STORE (batch SETTINGS+CONFIRM, TASK-436; CONVENTIONS
 *  "Settings screen…" §2, §8, §10). Owns the in-memory settings values, loads
 *  them once at Initialize, and writes the slot on every real change.
 *
 *  ⚠️ IT IS A UGameInstanceSubsystem, AND THAT IS THE WHOLE REQUIREMENT RATHER
 *  THAN A STYLE CHOICE: the value is WRITTEN in L_MainMenu (the settings screen)
 *  and READ in L_Arena, mid-match, at confirm time. A widget-owned or
 *  controller-owned value does not survive OpenLevel — the Input-mode ownership
 *  law is this same lesson wearing different clothes. Precedents:
 *  USiegeSessionSubsystem, USiegeLlamaSubsystem.
 *
 *  ⚠️ LOAD ONCE, SAVE ON CHANGE, NEVER TOUCH THE DISK FROM A GAMEPLAY PATH.
 *  IsAssistantConfirmEnabled() is a PURE IN-MEMORY GETTER. A LoadGameFromSlot on
 *  the order path would put a disk hit inside a mid-battle interaction, so the
 *  only disk reads are Initialize()'s single load and an explicit
 *  LoadSettingsFromSlot() call (automation only).
 *
 *  HOW CONSUMERS RESOLVE IT (the snippet TASK-437 and TASK-443 both use):
 *
 *      UGameInstance* GI = GetGameInstance();               // or Actor->GetGameInstance()
 *      USiegeSettingsSubsystem* Settings =
 *          GI ? GI->GetSubsystem<USiegeSettingsSubsystem>() : nullptr;
 *      const bool bConfirm = Settings ? Settings->IsAssistantConfirmEnabled()
 *                                     : true;               // ⚠️ FAIL SAFE, see below
 *
 *  ⛔ AND THE FALLBACK WHEN THE SUBSYSTEM DOES NOT RESOLVE IS true (CONFIRM ON),
 *  NEVER false. An unresolvable subsystem must degrade to MORE human review, not
 *  less; silently executing because a lookup failed is exactly the failure this
 *  feature exists to prevent.
 *
 *  ⛔ THE TOGGLE REMOVES A HUMAN REVIEW STEP AND NEVER A MACHINE CHECK (§5, the
 *  ruling that makes it safe to ship). This class stores ONE BOOL and exposes it.
 *  It does not gate, skip, weaken or short-circuit any validation, and no caller
 *  may use it to. With confirm OFF the assistant still runs the parse, the
 *  Kinds.Num() == Counts.Num() invariant, the shortfall/clarification path, the
 *  eligibility checks, the authority refusal and the non-orderable-kind guard.
 *  Every check that runs with the toggle ON runs with it OFF.
 *
 *  ⛔ AND IT GATES NO KEY. §2 of the assistant law is unchanged: every keyboard
 *  command still works byte-identically and no setting may ever gate one.
 *
 *  ─── HOW TO ADD A SETTING (this is deliberately not a one-setting class) ───
 *   1. Add a UPROPERTY to USiegeSettingsSaveGame (tagged-property serialization
 *      means old .sav files load it at its C++ default — no version field, no
 *      migration code).
 *   2. Add `static const FName SettingName_<Thing>;` here and define it in the
 *      .cpp beside the existing one.
 *   3. Add a BlueprintPure getter + a BlueprintCallable setter; the setter is a
 *      one-line forward to ApplyBoolSetting(...) (or its typed sibling), which
 *      already owns the no-op check, the save and the broadcast.
 *   4. Extend LoadSettingsFromSlot()'s apply block by one line.
 *   5. 🚨⭐ EXTEND SaveSettingsToSlot() BY ONE LINE — `SaveObj-><Field> = <Field>;`.
 *
 *  🚨⭐ STEP 5 WAS MISSING FROM THIS RECIPE UNTIL TASK-1120 (which is setting #2)
 *  FOLLOWED IT AND FOUND THE HOLE. It is recorded here rather than quietly fixed,
 *  because the failure it produces is the nastiest shape a settings bug has:
 *  SaveSettingsToSlot() copies the in-memory values onto a FRESHLY CONSTRUCTED
 *  SaveGame object field by field, so a setting added by steps 1-4 alone would
 *  be written to disk AT ITS C++ DEFAULT, every time, forever. In the running
 *  session everything looks perfect — the getter returns the new value, the
 *  delegate fires, the UI updates, the save reports SUCCESS — and the preference
 *  silently evaporates on the next launch, with no error and nothing in the log
 *  to point at. ⛔ A steps-1-4 setting is not "unfinished", it is WRONG WHILE
 *  APPEARING CORRECT (SC-§94 cl. A). An automation test now pins it: the
 *  round-trip test below loads into a SECOND store rather than re-reading the
 *  first, which is what makes step 5's absence a red instead of a green.
 *
 *  Nothing in steps 1-5 touches the slot contract, the delegate, or any consumer.
 *
 *  M8 DECLARATION (§8, verbatim): adds no replicated property, no new replicated
 *  class, no new relevancy tier. The settings value is client-local by
 *  construction — it governs a LOCAL human review step and never an
 *  authoritative outcome, so there is nothing to replicate and no relevancy
 *  question to answer.
 */
UCLASS()
class GITCLAUDEUNREALTEST_API USiegeSettingsSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	/**
	 *  THE fixed SaveGame slot, character-for-character: TEXT("SiegeSettings")
	 *  => Saved/SaveGames/SiegeSettings.sav. Defined once in the .cpp so every
	 *  translation unit links the same string.
	 *
	 *  ⚠️ A CHANGED SLOT NAME SILENTLY ORPHANS EVERY PLAYER'S SAVED SETTINGS —
	 *  no error, no warning, just settings that quietly revert to defaults. An
	 *  automation test asserts this string, on purpose.
	 */
	static const TCHAR* SettingsSlotName;

	/** User index for the settings slot (single local player => 0), the USiegeDeckSaveGame::UserIndex precedent. */
	static constexpr int32 SettingsUserIndex = 0;

	/**
	 *  The OnSettingsChanged payload for the confirm toggle ==
	 *  FName(TEXT("bAssistantConfirmBeforeExecute")) — the same token CONVENTIONS
	 *  §2 pins as the setting's name, so the delegate payload, the SaveGame field
	 *  and the law all read alike and stay greppable.
	 *
	 *  USettingsMenuWidget (TASK-437) forwards it to its
	 *  OnSettingsValueChanged(const FString& SettingName, bool bValue) event as
	 *  SettingName.ToString().
	 */
	static const FName SettingName_AssistantConfirmBeforeExecute;

	/**
	 *  ⭐ SETTING #2's delegate payload (TASK-1120, `GFX-§7`, pinned by `GFX-§10`)
	 *  == FName(TEXT("bShowFrameRateCounter")) — the same token as the SaveGame
	 *  field, exactly as its neighbour above, so the payload, the field and the law
	 *  stay one greppable word.
	 *
	 *  ⛔ IT IS A DIFFERENT NAME FROM ITS NEIGHBOUR AND THAT IS THE POINT: a
	 *  consumer that filters on the payload (the in-match counter does not; the
	 *  Graphics panel's row does) must be able to tell the two settings apart. An
	 *  automation test asserts the payload of a frame-counter change is THIS name
	 *  and not the confirm toggle's — a broadcast COUNT could not tell them apart,
	 *  and this lane has already shipped one counter that read identically under a
	 *  correct and a broken branch (qa/TASK-1119.md § LOOP 1, the M21 coincidence).
	 */
	static const FName SettingName_ShowFrameRateCounter;

	/**
	 *  PURE IN-MEMORY READ — no disk, no allocation, safe on the order path.
	 *  True (the default) means the assistant shows the parsed order + ghost
	 *  circles for a HUMAN review step before executing.
	 */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Settings")
	bool IsAssistantConfirmEnabled() const;

	/**
	 *  Writes the in-memory value AND saves the slot — but only when the value
	 *  actually changes. A same-value write is a complete no-op: no disk write,
	 *  no broadcast (§4 / the delegate law).
	 *
	 *  A failed disk write does NOT revert the in-memory value: the player's
	 *  toggle takes effect for this session and the failure is a Warning on
	 *  LogSiegeSettings. Snapping the checkbox back under the player's finger
	 *  because a save failed is the worse of the two behaviours.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Settings")
	void SetAssistantConfirmEnabled(bool bEnabled);

	/**
	 *  ⭐ SETTING #2 (TASK-1120): does the player want the in-match FPS /
	 *  frame-time counter on screen? PURE IN-MEMORY, no disk — it is read once
	 *  when the in-match counter is created and again on every broadcast, never
	 *  per frame.
	 *
	 *  ⛔ AND THE FALLBACK WHEN THIS SUBSYSTEM DOES NOT RESOLVE IS false, which is
	 *  the OPPOSITE polarity to IsAssistantConfirmEnabled()'s true — deliberately.
	 *  There the fail-safe direction is MORE human review; here it is LESS debug
	 *  UI. A lookup failure must never put a diagnostic overlay on a shipped
	 *  player's battlefield.
	 */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Settings")
	bool IsFrameRateCounterEnabled() const;

	/**
	 *  Writes the in-memory value AND saves the slot — but only on a real change,
	 *  through the same one mutation path as its neighbour. A same-value write is
	 *  a complete no-op: no disk write, no broadcast.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Settings")
	void SetFrameRateCounterEnabled(bool bEnabled);

	/**
	 *  Broadcast on every ACTUAL value change, never on a no-op write. UI
	 *  consumers SEED FROM THE GETTER FIRST, THEN BIND (qa/TASK-005 major-2: a
	 *  bind-only widget created at a pinned value stays stale).
	 */
	UPROPERTY(BlueprintAssignable, Category = "Siegebound|Settings")
	FOnSiegeSettingsChanged OnSettingsChanged;

	//~ Begin USubsystem interface
	/** Loads the slot ONCE. Missing / unreadable / foreign-class => C++ defaults, logged once, never a crash. */
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	//~ End USubsystem interface

	/**
	 *  Loads the settings slot into memory (missing / unreadable / foreign-class
	 *  => C++ defaults + one log line). Applied through the SAME path as a
	 *  setter, so a loaded value that differs from what is in memory broadcasts,
	 *  and one that matches does not. ⛔ NEVER writes the disk.
	 *
	 *  ⛔ NOT A GAMEPLAY PATH. The game calls this exactly once, from
	 *  Initialize(). It is public only so an automation test can drive a load
	 *  without fabricating an FSubsystemCollectionBase.
	 */
	void LoadSettingsFromSlot();

	/**
	 *  ⛔ AUTOMATION TESTS ONLY — nothing in the game may call this.
	 *
	 *  Redirects THIS INSTANCE's load and save to a scratch slot so a test run
	 *  can never overwrite the player's real Saved/SaveGames/SiegeSettings.sav.
	 *  An empty string restores the pinned SettingsSlotName. It does not touch
	 *  the in-memory value.
	 */
	void SetSlotNameForAutomationTests(const FString& InSlotName);

	/**
	 *  ACC-§7 / TASK-601: runs when USiegeAccountSubsystem broadcasts
	 *  OnActiveProfileChanged (create / login / logout re-point the resolved
	 *  settings slot). Reloads through LoadSettingsFromSlot() — so a missing
	 *  profile slot falls back to C++ defaults via the CDO, and any value that
	 *  ACTUALLY changes broadcasts the EXISTING OnSettingsChanged through the
	 *  one mutation path (never on a no-op: the delegate law).
	 *  USettingsMenuWidget already subscribes (SettingsMenuWidget.cpp:377), so
	 *  the UI refreshes for free; a reload that lands on the value already in
	 *  memory needs no refresh because the widget is already showing it.
	 */
	void ReloadForActiveProfile();

	/**
	 *  DIAGNOSTICS + THE AUTOMATION TESTS' OBSERVATION POINT: how many times
	 *  OnSettingsChanged has been broadcast this session. Incremented inside
	 *  BroadcastSettingChanged(), which is the ONLY place in the .cpp that calls
	 *  OnSettingsChanged.Broadcast — so the count cannot diverge from the
	 *  broadcasts. Read-only for consumers; no logic anywhere reads it.
	 */
	int32 SettingsChangeBroadcastCount = 0;

	/** The setting name carried by the most recent broadcast (diagnostics; NAME_None before the first one). */
	FName LastBroadcastSettingName = NAME_None;

private:

	/**
	 *  THE ONE MUTATION PATH for a bool setting: no-op check, then value, then
	 *  (optionally) disk, then broadcast. Every setter and the loader funnel
	 *  through here, which is what makes "broadcasts on every real change, never
	 *  on a no-op" a structural property instead of a promise repeated at each
	 *  call site.
	 */
	void ApplyBoolSetting(bool& OutValue, bool bNewValue, FName SettingName, bool bPersistToDisk);

	/** Serializes the current in-memory settings to the resolved slot. Returns false (and logs a Warning) when the write fails. */
	bool SaveSettingsToSlot() const;

	/** The ONLY caller of OnSettingsChanged.Broadcast — also bumps the diagnostics counter and logs the change. */
	void BroadcastSettingChanged(FName SettingName);

	/**
	 *  UFUNCTION forwarder for USiegeAccountSubsystem::OnActiveProfileChanged —
	 *  a DYNAMIC multicast binds by UFUNCTION name, while ReloadForActiveProfile()
	 *  stays the plain member the ACC-§7 registry pins character-for-character.
	 *  Forwards, nothing else.
	 */
	UFUNCTION()
	void HandleActiveProfileChanged();

	/**
	 *  ACC-§4 precedence (TASK-601): SlotNameOverride (tests — semantics
	 *  UNCHANGED) > the account-profile slot via
	 *  USiegeAccountSubsystem::GetSettingsSlotName() (an unresolvable game
	 *  instance or account subsystem FALLS THROUGH — today's behavior, never a
	 *  crash) > the pinned SettingsSlotName (guest, byte-identical).
	 */
	FString ResolveSlotName() const;

	/** In-memory confirm-toggle value. Mirrors USiegeSettingsSaveGame's C++ default; LoadSettingsFromSlot re-derives the fallback from that class's CDO so the two cannot drift (an automation test asserts they agree). */
	bool bAssistantConfirmBeforeExecute = true;

	/** ⭐ In-memory in-match FPS-counter preference (TASK-1120). Same drift guard as its neighbour: the literal below and USiegeSettingsSaveGame's C++ default are asserted equal by an automation test, and only the SaveGame's is ever used as the load fallback. */
	bool bShowFrameRateCounter = false;

	/** ⛔ Automation only (SetSlotNameForAutomationTests). Empty in every shipped path. */
	FString SlotNameOverride;

	/** Latches the "no readable save, using defaults" line to ONE emission per subsystem instance. */
	bool bLoadFallbackLogged = false;
};
