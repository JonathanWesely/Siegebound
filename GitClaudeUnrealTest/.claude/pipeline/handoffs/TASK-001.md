# TASK-001 Handoff — Card data types, team types & cards.csv

- author: gameplay-programmer
- date: 2026-07-02
- status: implementation complete, files-only (no compile, no editor, no Git per task constraints)

## Files created

1. `Source/GitClaudeUnrealTest/Siegebound/TeamId.h` (header-only, per CONVENTIONS pure-data exception)
2. `Source/GitClaudeUnrealTest/Siegebound/CardRow.h` (header-only)
3. `Docs/Data/cards.csv`

No existing template files were modified. `GitClaudeUnrealTest.Build.cs` was intentionally left untouched: the module root `"GitClaudeUnrealTest"` is already in `PublicIncludePaths`, so downstream code includes these as `#include "Siegebound/TeamId.h"` / `#include "Siegebound/CardRow.h"`.

## Type contract for downstream tasks (TASK-002..011)

### TeamId.h
- `ETeamId` — `UENUM(BlueprintType) enum class ETeamId : uint8 { Blue, Red };`
- `UTeamAgent` — `UINTERFACE(MinimalAPI, NotBlueprintable)` (matches the template interface style, e.g. CombatDamageable.h)
- `ITeamAgent` — pure virtual `UFUNCTION(BlueprintCallable, Category="Team") virtual ETeamId GetTeamId() const = 0;`
  - NotBlueprintable: implementers MUST be C++ classes (ACastle, AHeroCharacter, ASummonedUnit all are). BP subclasses inherit the implementation.

### CardRow.h
- `ECardType` — `{ Unit, Building, Economy, Spell, HeroUpgrade, Utility }` (uint8, BlueprintType)
- `ECardProfile` — `{ None, Standard, Siege, Support }` (uint8, BlueprintType)
- `FCardRow : public FTableRowBase` — `USTRUCT(BlueprintType)`, `GITCLAUDEUNREALTEST_API`
- UPROPERTY names (exact, 1:1 with CSV columns): `DisplayName` (FString), `CardType` (ECardType), `Cost` (int32), `MaxCopies` (int32), `HP` (float), `Damage` (float), `Range` (float), `Cadence` (float), `Speed` (float), `Profile` (ECardProfile), `Notes` (FString)
- All properties `EditAnywhere, BlueprintReadOnly`. In-class defaults: numerics 0 except `Cadence = 1.0f` (avoids a zero attack interval if a future row omits it); `CardType = Unit`; `Profile = None`.

## cards.csv (import source for TASK-008 -> /Game/Data/DT_Cards)

- Header: `,DisplayName,CardType,Cost,MaxCopies,HP,Damage,Range,Cadence,Speed,Profile,Notes` — first column intentionally blank = row name per UE DataTable CSV convention (the importer ignores the first header cell).
- One data row, row name (CardID) `Footman`: DisplayName Footman, CardType `Unit`, Cost 3, MaxCopies 12, HP 80, Damage 12, Range 120, Cadence 1.0, Speed 400, Profile `Standard`, Notes "Basic melee line unit" — matches GDD section 4 exactly.
- Enum cells use short value names (`Unit`, `Standard`), which the DataTable CSV importer resolves against ECardType/ECardProfile.
- Encoding verified: all three files are pure ASCII (valid UTF-8, no BOM), LF-safe.

## Notes for TASK-008 (import)

- Import with row struct `FCardRow` to `/Game/Data/DT_Cards`; row name must come out as `Footman`.
- Expect zero warnings: every CSV column after the row-name column maps to an FCardRow UPROPERTY by exact name; there are no extra or missing columns.
- Keep `Docs/Data/cards.csv` as the persistent import source (GDD section 3.0) — balance edits are CSV re-imports.

## For QA to scrutinize

- Interface style copied from the shipped template `Variant_Combat/Interfaces/CombatDamageable.h` (pure virtual UFUNCTION in a NotBlueprintable MinimalAPI UINTERFACE) — flag if 5.8 UHT rules differ.
- Header-only files (no .cpp) — sanctioned by CONVENTIONS for pure data/interfaces; UHT-generated .gen.cpp handles registration.
- `generated.h` is the last include in both headers.
