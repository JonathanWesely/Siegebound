# QA Report — TASK-001 — Card data types, team types & cards.csv
Verdict: PASS

Reviewer: qa-reviewer · Date: 2026-07-02 · Blockers: 0

Files reviewed:
- `Source/GitClaudeUnrealTest/Siegebound/TeamId.h`
- `Source/GitClaudeUnrealTest/Siegebound/CardRow.h`
- `Docs/Data/cards.csv`
Against: TASKBOARD TASK-001 spec + names block, M1 manager decisions, CONVENTIONS.md, GDD §3.0 / §4 (Core Set, Footman row), handoff `.claude/pipeline/handoffs/TASK-001.md`.

## Ruling: the UINTERFACE pattern (explicitly requested)

**LEGAL under UE 5.8 UHT — no change required.** `TeamId.h:25-40` declares
`UINTERFACE(MinimalAPI, NotBlueprintable)` with a pure-virtual
`UFUNCTION(BlueprintCallable) virtual ETeamId GetTeamId() const = 0;` in `ITeamAgent`.
The usual UHT error ("Blueprint accessible interface functions must be declared
'BlueprintImplementableEvent' or 'BlueprintNativeEvent'") is suppressed when the interface
carries the `CannotImplementInterfaceInBlueprint` metadata, and in UE5's UHT the
`NotBlueprintable` interface specifier applies exactly that metadata. Decisive in-project proof:
Epic's shipped template `Source/GitClaudeUnrealTest/Variant_Combat/Interfaces/CombatDamageable.h:14-40`
uses the identical combination (four pure-virtual BlueprintCallable UFUNCTIONs in a
MinimalAPI, NotBlueprintable UINTERFACE) in this same module, and the project compiles and
runs with it. The pure virtual is safe because NotBlueprintable guarantees every implementer
is a C++ class that must override it; the generated BP thunk dispatches through the vtable.

**Standing constraint (record for TASK-002/003/004):** this is legal ONLY while `UTeamAgent`
stays `NotBlueprintable`. If anyone later makes the interface Blueprintable/BlueprintType-implementable,
`GetTeamId` must be converted to `BlueprintNativeEvent` (and cannot remain pure virtual).

## Findings

- [NIT] `Source/GitClaudeUnrealTest/Siegebound/TeamId.h:1`, `CardRow.h:1` — new original files carry the template's "Copyright Epic Games, Inc." header. Consistent with the rest of the project, so acceptable; note for a future housekeeping pass. No action required.
- [NIT] `Source/GitClaudeUnrealTest/Siegebound/CardRow.h:53` — in-class default `CardType = ECardType::Unit` means a code-constructed (non-CSV) FCardRow silently reads as a Unit. Harmless for the DataTable path (CSV always supplies every column) and the choice is documented in the handoff. No action required.

No blockers. No majors.

## Verification detail

**UE 5.8 correctness — clean**
- `TeamId.h`: includes `CoreMinimal.h`, `UObject/Interface.h`, then `TeamId.generated.h` last; generated include filename matches header name. `GENERATED_BODY()` present in both the U-class and I-class. `enum class ETeamId : uint8` with `UENUM(BlueprintType)`.
- `CardRow.h`: includes `Engine/DataTable.h` (provides `FTableRowBase`), then `CardRow.generated.h` last. `USTRUCT(BlueprintType)` with `GENERATED_BODY()` first in the struct. API macro `GITCLAUDEUNREALTEST_API` matches the module (same macro used by existing module code, e.g. `Variant_Combat/AI/EnvQueryContext_Danger.h`). No deprecated APIs used.
- Header-only files are fine: UHT emits the .gen.cpp registration; nothing needs a .cpp. `Build.cs` untouched, and the module root is already on `PublicIncludePaths`, so `#include "Siegebound/TeamId.h"` works downstream as the handoff states.

**CSV ↔ struct contract — exact**
- Header row: blank first cell (row-name column), then `DisplayName,CardType,Cost,MaxCopies,HP,Damage,Range,Cadence,Speed,Profile,Notes` — 11 columns mapping 1:1, character-for-character, onto the 11 FCardRow UPROPERTYs; no extra/missing columns, no stray spaces → zero-warning import expected in TASK-008.
- Enum cells use short value names `Unit` / `Standard`, which the DataTable CSV importer resolves against ECardType/ECardProfile (short or fully-qualified names both accepted).
- Row `Footman`: Unit, Cost 3, MaxCopies 12, HP 80, Damage 12, Range 120, Cadence 1.0, Speed 400, Profile Standard — matches GDD §4 Core Set (Docs/GDD.md:139) exactly. Notes cell contains no commas (no quoting needed).
- Exactly one data row (Footman), per the M1 manager decision that hero/castle stats are NOT card rows.
- All three files scanned: pure ASCII, no BOM (handoff claim verified).

**Conventions — compliant**
- Names and paths match the TASK-001 names block character-for-character: `Siegebound/TeamId.h` (ETeamId, UTeamAgent, ITeamAgent), `Siegebound/CardRow.h` (ECardType, ECardProfile, FCardRow), `Docs/Data/cards.csv`, row name `Footman`.
- E/F/I prefixes correct; shared header-only file sanctioned by the CONVENTIONS pure-data exception; team contract (`ETeamId { Blue, Red }`) matches CONVENTIONS verbatim.

## Notes for build-master (PASS)

- TASK-001 is files-only; the compile IS the remaining acceptance gate. Expect UHT to accept both headers as-is (pattern proven by CombatDamageable.h in the same module).
- Do NOT import the CSV yet — DT_Cards import is TASK-008 (blocked on this task integrating). Keep `Docs/Data/cards.csv` in the commit as the persistent import source (GDD §3.0).
- Nothing here touches template code, Build.cs, or any Content asset.
