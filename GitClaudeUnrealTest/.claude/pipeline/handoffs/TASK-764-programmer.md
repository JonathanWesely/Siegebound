# TASK-764 — Ghost pawn class asset wired (programmer handoff)

**Status:** `ready-for-qa`
**Scope:** one statement, one file. No compile, no editor, no MCP, no Git.

## The change

`Source/GitClaudeUnrealTest/Siegebound/SiegeGameMode.cpp`, in `ASiegeGameMode::ASiegeGameMode()`
(the commented-out `// GhostPawnClassAsset = <unset>;` placeholder is replaced):

```cpp
GhostPawnClassAsset = TSoftClassPtr<ASiegeGhostPawn>(FSoftObjectPath(TEXT("/Game/Blueprints/BP_SiegeGhostPawn.BP_SiegeGhostPawn_C")));
```

Precedent it mirrors, 8 lines above (unchanged):

```cpp
HeroPawnClassAsset = TSoftClassPtr<AHeroCharacter>(FSoftObjectPath(TEXT("/Game/Blueprints/BP_HeroCharacter.BP_HeroCharacter_C")));
```

Same idiom exactly: `TSoftClassPtr<T>` wrapping an `FSoftObjectPath` built from a `TEXT()`
literal, `Package.Asset_C` form, assigned in the constructor. No new idiom invented.

The surrounding comment was rewritten rather than left standing — it previously read
"⛔ DELIBERATELY LEFT UNSET", which the new value contradicts. Per the file's own
TASK-517 / HIGH-§1 idiom (a shipped comment that contradicts the shipped value is the
drift defect), it now records why the unset default existed and why it is now spent.

**No include was needed.** `#include "Siegebound/SiegeGhostPawn.h"` is already present at
line 21 (added by TASK-750), and `ASiegeGhostPawn` is already forward-declared in the
header at line 14. The header was NOT touched.

## Path verification (done statically — the editor was not opened)

Read the literal out of the `.uasset` name table with a byte-level string scan:

```
/Script/Engine.BlueprintGeneratedClass'/Game/Blueprints/BP_SiegeGhostPawn.BP_SiegeGhostPawn_C'
/Script/CoreUObject.Class'/Script/GitClaudeUnrealTest.SiegeGhostPawn'   <- parent class
```

- Object path is character-for-character what the code now carries, `_C` included.
- Parent is `ASiegeGhostPawn`, so the `TSoftClassPtr<ASiegeGhostPawn>` type filter passes
  and `LoadSynchronous()` returns non-null rather than being rejected as incompatible.
- `BP_HeroCharacter.uasset` was scanned as a control and produced the identical shape.
- File exists: `Content/Blueprints/BP_SiegeGhostPawn.uasset`.

**Branch landed in:** the authored-and-resolved branch of `ResolveGhostPawnClass()` —
`GhostPawnClassAsset.IsNull()` is now false (so the Log-once unset message does not fire)
and `LoadSynchronous()` resolves (so the Warning-once unresolvable message does not fire).
**Neither log line should appear.** If either appears at runtime, the path is wrong and
this task did not land — that is the cheap runtime tell for the integrator.

## "Cannot be done from the editor" — premises re-verified, all four true

| Premise | Verified |
|---|---|
| `GhostPawnClassAsset` is `EditDefaultsOnly`, not `config` | `SiegeGameMode.h` — `UPROPERTY(EditDefaultsOnly, Category = "Siegebound\|Classes")`, no `config` |
| `DefaultEngine.ini:12` names the raw C++ class | `GlobalDefaultGameMode=/Script/GitClaudeUnrealTest.SiegeGameMode` |
| No `BP_SiegeGameMode` exists | `find Content -iname "*GameMode*"` — 8 hits, none is a `SiegeGameMode` child |
| ⇒ nowhere for a native-CDO edit to persist | follows from the three above |

## ⚠️ ONE THING QA AND THE INTEGRATOR MUST SEE — a pre-existing test now contradicts this change

`Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeRespawnLifecycleTest.cpp` §8:

- Test name, line 659: `Siegebound.RespawnLifecycle.GhostPawnClassAssetIsTypedToTheGhostAndShipsUnset`
- Assertion (b), lines 699-700:

```cpp
TestTrue(TEXT("(b) GhostPawnClassAsset ships UNSET — the raw C++ ASiegeGhostPawn is the shipped ghost, with no per-match missing-asset warning"),
    GhostValue->IsNull());
```

**This assertion is now false by construction and the test will FAIL.** The suite COUNT is
unaffected — still 248 — but 1 of 248 goes red. I did **not** edit it: the test file is
outside this task's fence, and the dispatch reserved the test decision explicitly.

**I checked for further collisions and there are none.** Assertion (a) (the `MetaClass` type
check) still passes; every other ghost assertion in `SiegeRespawnLifecycleTest.cpp` and all
of `SiegeGhostPawnTest.cpp` asserts the shape of the C++ class itself, which is untouched.

**Proposed minimal fix (count-preserving, one test edited in place, no new test):** rename to
`…IsTypedToTheGhostAndPointsAtTheShippedBlueprint` and replace (b) with an equality check
against the exact path rather than a flipped null check —

```cpp
TestEqual(TEXT("(b) GhostPawnClassAsset points at BP_SiegeGhostPawn's GENERATED CLASS (_C) — TASK-764"),
    GhostValue->ToSoftObjectPath().ToString(),
    FString(TEXT("/Game/Blueprints/BP_SiegeGhostPawn.BP_SiegeGhostPawn_C")));
```

That is strictly stronger than `!IsNull()`: it catches the exact failure mode this task was
warned about — a near-miss path (notably a missing `_C`) that is non-null, looks correct, and
silently never spawns. Note also that the existing SELF-CHECK at lines 714-716 uses
"Hero is SET, Ghost is unset" as its discriminator; once both are set that contrast is gone,
which is a second reason (b) wants rewriting rather than merely inverting.

**Board it or grant the fence extension and I will apply it — I did not act unilaterally.**

## Untouched, as fenced

Death→ghost→180 s lifecycle · `ResolveHeroToRestore` · the `ClearMarks()` call ·
the `OnHeroRecallArrived` bind · `ResolveGhostPawnClass()` itself · `HeroRespawnDelay` ·
the header. `git diff --stat` = 1 file changed, 13 insertions, 6 deletions.

## Known-stale doc comment, deliberately NOT edited (fence)

`SiegeGameMode.h` (~line 470), the `GhostPawnClassAsset` docblock, still says
"⭐ IT SHIPS **UNSET**, AND THAT IS THE CORRECT DEFAULT RATHER THAN AN OVERSIGHT" and
"no task in the GHOST batch produces a ghost blueprint". Both sentences are now false.
The dispatch fenced the header to include-only, so I left it. Same for the unset branch's
log string in `ResolveGhostPawnClass` (still accurate about the branch, but that branch is
now unreachable in the shipped config). Recommend a doc-only sweep alongside the test fix.
