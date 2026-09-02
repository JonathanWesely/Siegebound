# TASK-765 — programmer handoff

**Repair the now-false "ships unset" assertion and its stale prose.**
Status → `ready-for-qa` (2026-09-01). Source: TASK-764.

---

## 1. Scope actually touched

| File | Change |
|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeRespawnLifecycleTest.cpp` | §8 banner, test name string, assertion (b), self-check |
| `Source/GitClaudeUnrealTest/Siegebound/SiegeGameMode.h` | `GhostPawnClassAsset` docblock (~:463-480) |

**`SiegeGameMode.cpp` was NOT touched.** Verified, not asserted: `git diff` on that file
shows only TASK-764's own uncommitted `+13 −6` (the rewritten comment block + the
assignment at `:67`). Nothing of mine is in it.

**No compile, no editor, no MCP, no Git.** The editor was up when I started and I never
went near it.

---

## 2. Suite count — the hard gate

**248, unchanged.**

```
grep -rhoE "IMPLEMENT_[A-Z_]*AUTOMATION_TEST[A-Z_]*" --include=*.cpp .
  => 248 IMPLEMENT_SIMPLE_AUTOMATION_TEST
SiegeRespawnLifecycleTest.cpp => 9   (unchanged)
```

Measured before and after the edits, across all 20 test files. No test added, none
removed. The test was **renamed** — only the name string moved; the `IMPLEMENT_*` macro
and the class `FSiegeGhostPawnClassAssetTest` are untouched (grep confirms the class name
has exactly two references, both inside this file, so the rename could not break a caller).

`Siegebound.RespawnLifecycle.GhostPawnClassAssetIsTypedToTheGhostAndShipsUnset`
→ `Siegebound.RespawnLifecycle.GhostPawnClassAssetIsTypedToTheGhostAndPointsAtTheGeneratedBlueprintClass`

---

## 3. The repaired assertion (b) — equality on the path, NOT `!IsNull()`

Was (`:699-700`), now false by construction:

```cpp
TestTrue(TEXT("(b) GhostPawnClassAsset ships UNSET — ..."), GhostValue->IsNull());
```

Now:

```cpp
const FString ExpectedGhostPath(TEXT("/Game/Blueprints/BP_SiegeGhostPawn.BP_SiegeGhostPawn_C"));

TestEqual(TEXT("(b) ⭐ GhostPawnClassAsset points at BP_SiegeGhostPawn's GENERATED CLASS, `_C` exact — a path that loses `_C` is non-null, reads as authored, and silently never spawns"),
    GhostValue->ToString(), ExpectedGhostPath);
```

**Why equality and not an inversion.** The failure mode this property actually has is not
"empty" — it is **non-empty and subtly wrong**. Drop `_C` and the path names the Blueprint
*asset* rather than its generated class: non-null, looks authored in the editor and in any
log line that prints it, and `LoadSynchronous` still returns null through a
`TSoftClassPtr`. The resolver then takes its authored-but-unresolvable branch, warns once,
falls back, and the player spends the full 180 s never seeing the blueprint ghost — with
nothing red anywhere. **An inverted `!IsNull()` passes on exactly that defect.** This
assertion is pinned to the literal path so it cannot.

The expected literal was taken from the shipped `.cpp:67`, not typed from memory.

---

## 4. The re-armed self-check — the part that mattered most

**The old discriminator was "Hero is SET, Ghost is UNSET" and TASK-764 killed it.** Both
are set now, so that contrast was dead: it would have gone on passing while testing
nothing. That is the `SHIP-§9`/`SC-§37` shape, and this is the fourth instance today, so
it was re-armed rather than deleted.

Re-armed on the **two axes that genuinely still differ — path and type**:

```cpp
TestEqual(TEXT("SELF-CHECK: the SAME reader returns a DIFFERENT, equally exact path ..."),
    HeroValue->ToString(), FString(TEXT("/Game/Blueprints/BP_HeroCharacter.BP_HeroCharacter_C")));

TestNotEqual(TEXT("SELF-CHECK: …and the two paths are genuinely different strings ..."),
    GhostValue->ToString(), HeroValue->ToString());

TestTrue(TEXT("SELF-CHECK: …and the reader still discriminates on TYPE ..."),
    HeroClassProperty->MetaClass == AHeroCharacter::StaticClass()
    && HeroClassProperty->MetaClass != GhostClassProperty->MetaClass);
```

**What it now discriminates, and why it is not ceremonial.** TASK-764 made the two
properties nearly identical in shape — both `TSoftClassPtr` on the same CDO, both authored
to `/Game/Blueprints/BP_*_C`. That is precisely the condition under which a reader that
silently resolved *both* names onto *one* storage slot would go unnoticed, **and such a
reader would make the new assertion (b) pass by reading the wrong field.** Pinning the
hero to its own exact path and asserting the two reads differ is what rules that out.

**What would make it fail:**
- `FindPropertyByName` / `ContainerPtrToValuePtr` collapsing the two properties onto one
  storage slot — the paths would compare equal → `TestNotEqual` red.
- The hero reference rotting, emptying, or losing its own `_C` → the hero `TestEqual` red.
- The two `MetaClass`es ceasing to be distinguishable (e.g. someone retypes
  `GhostPawnClassAsset` to `AHeroCharacter`, or the ghost stops being its own type) →
  the type assertion red.
- A null CDO read on either property → explicit `AddError` + early `return false`.

I also promoted the hero's null read from an inline `HeroValue != nullptr &&` conjunct to
an explicit guarded `AddError` + `return false`, matching the ghost's guard above it —
otherwise a null hero read would have been reported as a *contrast* failure rather than as
the reader failure it actually is.

---

## 5. The swept docblock (`SiegeGameMode.h` ~:463)

The `⭐ IT SHIPS **UNSET**, AND THAT IS THE CORRECT DEFAULT` paragraph was rewritten to
`⭐ IT SHIPS **SET**, to BP_SiegeGhostPawn's generated class`. Per `HIGH-§1` / TASK-517 the
retired claim is **quoted and explained, not erased** — it records that the blank was
correct while it stood and that the reason is now *spent*, which is the same shape 764 used
in the adjacent `.cpp`. Added the `_C`-is-load-bearing warning and a note that the test
pins the full path literally.

**Deliberately preserved:** the two-branch resolver note (UNSET at Log once vs
AUTHORED-BUT-UNRESOLVABLE at Warning once) and the "never a crash, never a dead 180
seconds" guarantee. Both are still true and neither branch is dead code — a designer
clearing the field is still a legitimate route back to the raw C++ ghost.

---

## 6. For QA to scrutinise

1. **The expected path literals** (`:717` ghost, `:751` hero) against the shipped `.cpp:47`
   and `:67`. If these two drift from the constructor the suite goes red — that is the
   intended coupling, but it is a coupling and QA should see it.
2. **Case sensitivity.** I used `TestEqual`, not `TestEqualSensitive`. Deliberate: package
   and asset names are `FName`s, so the engine resolves them case-insensitively — a
   case-sensitive test could go red on a config that works. The `_C` difference is a token,
   not a case, and is caught either way. Flag if you disagree.
3. **Overload resolution.** `FSoftObjectPtr::ToString()` returns `FString` by value;
   `TestEqual(const TCHAR*, const FString&, const FString&)` (`AutomationTest.h:1997`) is an
   exact match and beats the `FStringView` overload (`:1996`, needs a user-defined
   conversion), so there is no ambiguity. `TestNotEqual` likewise at `:2006`. Both
   verified against the UE 5.8 header, not assumed.
4. **`GhostClassProperty` / `GhostValue` are still in scope** at the self-check (declared
   `:679` / `:710`, used `:754` / `:758`) — the self-check now reads both properties, which
   the old one did not.
5. **Nothing in the lifecycle was disturbed** — death→ghost→180 s, `ResolveHeroToRestore`,
   `ClearMarks()`, `OnHeroRecallArrived`. I did not open those regions.
6. **Assertion (a) untouched**, and no other ghost assertion touched. Independently
   re-checked 764's claim of no further collisions: `grep -rn "GhostPawnClassAsset"` over
   the whole tree returns 16 sites, and the only `IsNull()`-style assertion on it was the
   one at `:699`. `SiegeGhostPawnTest.cpp` does not reference the property at all.

---

## 7. Raised, not edited — out of fence

**`SiegeGameMode.cpp:293`** — the runtime log string still reads:

> `No ghost pawn blueprint configured (GhostPawnClassAsset is unset — the shipped default, TASK-750)`

The branch itself is still reachable and still correct (clearing the field is legitimate),
but the clause **"the shipped default"** is now false — the shipped default is *set*. It is
stale prose of exactly the `SC-§36` kind this task exists to sweep, and it will read as a
contradiction to the next person who greps for it.

**I did not touch it: `SiegeGameMode.cpp` was explicitly fenced.** Handing it to the
manager as a one-clause follow-up, the same way 764 handed this task up rather than
reaching across its own fence.
