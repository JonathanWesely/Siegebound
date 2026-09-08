// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "SiegeSettingsSaveGame.generated.h"

/**
 *  PERSISTED PLAYER SETTINGS (batch SETTINGS+CONFIRM, TASK-436; CONVENTIONS
 *  "Settings screen + the assistant CONFIRM STEP + the non-orderable-kind guard
 *  (2026-08-03)" §2 and the §8 pinned signature registry).
 *
 *  Written to the fixed slot USiegeSettingsSubsystem::SettingsSlotName
 *  ("SiegeSettings") at user index USiegeSettingsSubsystem::SettingsUserIndex (0)
 *  => Saved/SaveGames/SiegeSettings.sav. USiegeSettingsSubsystem is the ONLY
 *  reader and the ONLY writer; nothing else may load or save this slot.
 *
 *  ⚠️ USaveGame AND NOT UGameUserSettings — RULED (§2), and the reason is
 *  recorded because the other answer looks more idiomatic: UGameUserSettings is
 *  the engine's home for video/audio/input, and adopting it means a
 *  GameUserSettingsClassName edit in DefaultEngine.ini — a shipped-config change
 *  plus a new engine coupling, to store one gameplay bool. USiegeDeckSaveGame +
 *  a fixed slot is this project's proven, package-safe, runtime-writable idiom
 *  and needs no config edit. If a real graphics/audio pass ever lands,
 *  UGameUserSettings is the right home FOR THOSE and the two coexist.
 *
 *  ⚠️ THIS IS NOT A ONE-SETTING CLASS, AND THE VERSIONING STORY IS ALREADY PAID
 *  FOR. SaveGame archives use TAGGED PROPERTY serialization: a field added here
 *  later is simply ABSENT from an older .sav and therefore loads at its C++
 *  default, and a field removed later is skipped. That is why v1 carries no
 *  version int32 and needs none. Adding setting #2 is: one UPROPERTY here + one
 *  getter/setter pair + one FName constant on the subsystem — see the
 *  "HOW TO ADD SETTING #2" block in SiegeSettingsSubsystem.h.
 *
 *  NULL-SAFETY CONTRACT (cloned from USiegeDeckSaveGame, not reinvented): a
 *  missing, unreadable or FOREIGN-CLASS slot casts to nullptr and the subsystem
 *  falls back to the C++ defaults below, logs once on LogSiegeSettings, and
 *  never crashes.
 *
 *  M8 DECLARATION (§8, verbatim): adds no replicated property, no new
 *  replicated class, no new relevancy tier. This value is client-local by
 *  construction — it governs a LOCAL human review step and never an
 *  authoritative outcome.
 */
UCLASS()
class GITCLAUDEUNREALTEST_API USiegeSettingsSaveGame : public USaveGame
{
	GENERATED_BODY()

public:

	/**
	 *  Assistant: show the parsed order + ghost circles for review before executing.
	 *
	 *  ⛔ DEFAULT IS true (ON), AND THE DEFAULT IS ARGUED FROM MEASUREMENT, NOT
	 *  CAUTION (§5): 4 of the 5 stable eval failures are wrong-place / wrong-count
	 *  on orders the model otherwise understood — exactly the class a ground
	 *  preview makes obvious at a glance.
	 *
	 *  ⛔ AND THE ONE LAW THAT TRAVELS WITH THIS FIELD: IT REMOVES A HUMAN REVIEW
	 *  STEP AND NEVER A MACHINE CHECK. With it false the FSM skips AwaitConfirm and
	 *  executes; it does NOT skip the parse, the Kinds.Num() == Counts.Num()
	 *  invariant, the shortfall/clarification path, eligibility, the authority
	 *  refusal, or the non-orderable-kind guard. Every check that runs with this
	 *  true runs with it false.
	 *
	 *  ⚠️ THIS DEFAULT IS THE SINGLE SOURCE OF TRUTH FOR THE FALLBACK VALUE.
	 *  USiegeSettingsSubsystem reads it off the CDO (GetDefault<>) when the slot
	 *  is missing/unreadable rather than repeating the literal, so the two cannot
	 *  drift; an automation test asserts they agree.
	 */
	UPROPERTY()
	bool bAssistantConfirmBeforeExecute = true;

	/**
	 *  ⭐ SETTING #2 (TASK-1120, `GFX-§7`): show the in-match FPS / frame-time
	 *  counter. The player opts in from Settings → Graphics.
	 *
	 *  ⛔ DEFAULT IS false (OFF), AND THAT IS A RULING, NOT TIMIDITY (board cl. 4).
	 *  A permanent counter on a shipped game's HUD is debug UI; it is on screen for
	 *  every second of every match, over the battlefield, forever. The player who
	 *  wants to tune asks for it — and the moment they ask, they get a number
	 *  measured in `L_Arena` rather than in the menu, which is the whole point of
	 *  `GFX-§7`'s second clause.
	 *
	 *  ⛔ AND THIS IS THE ONE NAMED EXCEPTION TO `GFX-§3` (per-machine, not
	 *  per-profile). Every OTHER graphics value lives in `UGameUserSettings`
	 *  because it describes HARDWARE; this one describes a PREFERENCE about UI, so
	 *  it belongs to the person and follows them across machines with their
	 *  account. `GFX-§3` names this field as the exception in so many words — it is
	 *  not an inconsistency to be tidied away later.
	 *
	 *  ⚠️ THE VERSIONING STORY IS THE SAME ONE THE CLASS COMMENT ALREADY PAID FOR:
	 *  tagged-property serialization means a `.sav` written before this field
	 *  existed simply does not carry it, so it loads at the C++ default above.
	 *  ⛔ NO version int32 was added and NO migration code was written; adding
	 *  either would be the defect, not the safeguard.
	 *
	 *  ⚠️ AND, LIKE ITS NEIGHBOUR, THIS DEFAULT IS THE SINGLE SOURCE OF TRUTH FOR
	 *  THE FALLBACK. `USiegeSettingsSubsystem` reads it off the CDO rather than
	 *  repeating the literal; an automation test asserts the two agree.
	 */
	UPROPERTY()
	bool bShowFrameRateCounter = false;
};
