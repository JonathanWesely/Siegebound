#pragma once

#include "CoreMinimal.h"
#include "Logging/LogMacros.h"

// ============================================================================
// LogSiegeLlama -- the PLUGIN's log category.
//
// TWO CATEGORIES, DELIBERATELY (CONVENTIONS section 5): LogSiegeLlama covers
// tokens, backends, timings and faults; LogSiegeAssistant covers snapshots,
// grammars, commands and FSM transitions and belongs to the GAME lane. The
// plugin is architecturally forbidden to know about Siegebound, so it must
// never log to LogSiegeAssistant.
//
// !! CROSS-TASK CONTRACT -- READ BEFORE TASK-423 !!
// CONVENTIONS section 5 pins this category as "declared/defined in
// SiegeLlamaSubsystem.h/.cpp". That file is TASK-423's EXCLUSIVELY OWNED file
// and does not exist yet, but TASK-409's module startup and Siege.Llama.Info
// both need the category NOW. So it is declared here and DEFINED ONCE in
// SiegeLlamaModule.cpp.
//
// TASK-423 MUST #include "SiegeLlamaLog.h" from SiegeLlamaSubsystem.h and MUST
// NOT re-declare or re-define LogSiegeLlama. A second DEFINE_LOG_CATEGORY is a
// duplicate-symbol LINK ERROR, not a compile error, so it will surface late and
// confusingly. Flagged in handoffs/TASK-409-buildmaster.md.
// ============================================================================
DECLARE_LOG_CATEGORY_EXTERN(LogSiegeLlama, Log, All);
