# TASK-744 — [MARK-1] `FSiegeMapMark` + `USiegeMapMarkSubsystem` — programmer handoff

**Status:** `ready-for-qa` · **QA gate:** TASK-753 · **Compile + suite gate:** TASK-754 (⛔ serializes behind TASK-742)
**Law obeyed (cited, ⛔ not restated):** `MARK-§0`..`MARK-§6` (esp. `§1` the zero-Zone-A proof, `§2` the Zone-C cost + why `circle_1`, `§3` M-1..M-6, `§4` the wheel amendment, `§5` names, `§6` M8) · `HIGH-§1` · `SHIP-§9c` · `SC-§13` · `WR-§6` (airlock only — ⛔ no file of `WR-§6`'s was touched).

---

## 1. FILES — all NEW, ⛔ nothing existing was edited

| file | state |
|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/SiegeMapMark.h` | **NEW** — header-only pure data + the symbol seam |
| `Source/GitClaudeUnrealTest/Siegebound/SiegeMapMarkSubsystem.h` | **NEW** — the store's declaration + the tunables |
| `Source/GitClaudeUnrealTest/Siegebound/SiegeMapMarkSubsystem.cpp` | **NEW** — the allocator, the cap, the radius gate |
| `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeMapMarkTest.cpp` | **NEW** — 9 tests (checked first: ⛔ no existing map-mark test file, ⛔ no duplicate frame) |

⛔ **`WarMapWidget.{h,cpp}` (745), `SiegeAssistantSnapshot.{h,cpp}` (746), `HeroCharacter.{h,cpp}` (748), `SiegeGameMode`/`SiegePlayerController` (750), `SummonedUnit.{h,cpp}` (738), `ClimbableTower.{h,cpp}` (734) were READ ONLY WHERE CITED AND WRITTEN NOWHERE.** `.Build.cs` untouched (UBT globs the module dir). ⛔ No compile, ⛔ no editor, ⛔ no MCP, ⛔ no Git.

---

## 2. ⭐ THE PUBLIC API 745 / 746 / 751 COMPILE AGAINST — **the pinned registry, character-for-character**

```cpp
// ── SiegeMapMark.h ──
struct FSiegeMapMark
{
    int32     Number   = 0;
    FVector2D WorldXY  = FVector2D::ZeroVector;
    float     RadiusUU = 0.f;

    static FString MakeSymbol(int32 InNumber);          // inline-defined in the header
    static constexpr int32 FirstMarkNumber = 1;         // ⭐ ADDITIVE — see §3
};

// ── SiegeMapMarkSubsystem.h ──
DECLARE_LOG_CATEGORY_EXTERN(LogSiegeMapMark, Log, All); // ⭐ ADDITIVE — see §3

UCLASS()
class GITCLAUDEUNREALTEST_API USiegeMapMarkSubsystem : public ULocalPlayerSubsystem
{
    GENERATED_BODY()
public:
    bool AddMark(const FVector2D& WorldXY, float RadiusUU, FSiegeMapMark& OutMark);
    bool RemoveMark(int32 Number);
    bool SetMarkRadius(int32 Number, float NewRadiusUU);
    const TArray<FSiegeMapMark>& GetMarks() const;
    void ClearMarks();

    const FSiegeMapMark* FindMark(int32 Number) const;  // ⭐ ADDITIVE — see §3

    UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Map Marks") int32 MaxMapMarks     = 9;
    UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Map Marks") float MinMarkRadiusUU = 250.f;    // ⭐ ADDITIVE
    UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Map Marks") float MaxMarkRadiusUU = 12000.f;  // ⭐ ADDITIVE
};
```

**Every pinned signature landed unchanged — ⛔ zero divergence, ⛔ zero rename.** ⭐ Reaching the subsystem is the ordinary local-player route (`ULocalPlayer::GetSubsystem<USiegeMapMarkSubsystem>()` / `GetLocalPlayer()->GetSubsystem<...>()`); ⛔ there is no custom getter and none is needed.

### Contract notes the two parallel authors need

- **`MakeSymbol(N)`** → `"circle_N"` for `N >= 1`; ⛔ **the EMPTY string for `N < 1`** (⛔ never `circle_0`). It does ⛔ NOT enforce the upper bound — the cap lives on the tunable, and a second copy of it here would drift.
- ⭐ **746 — the sanctioned symbol→mark direction is `MakeSymbol(Mark.Number)` compared against the incoming symbol, over at most 9 marks. ⛔ Do NOT write a parser** — a parser is a second spelling of the rule, and the day the two disagree the map inserts one string while `ResolvePlace` answers another.
- **`AddMark` on refusal returns `false` and ⛔ does NOT write `OutMark`** (asserted with a sentinel). 745 must show the cap status line; ⛔ never a silent no-op.
- **`SetMarkRadius` returns `true` when the mark EXISTS** — clamping is ⛔ NOT a refusal (a wheel notch against the stop must not surface as a failure). It returns `false` only for an unknown number.
- **`GetMarks()` is held ASCENDING BY `Number`, holes included** (1, 3, 4 reads as "2 was deleted") — a declared invariant, stable paint order for 745.
- ⛔ **The index is NEVER the identity.** `Marks[1]` is not `circle_2`. Every entry point takes a NUMBER.
- **751 (help rows):** the player-facing number is `1`; the model-facing symbol is `circle_1`; ⛔ neither may be shown in the other's place (`MARK-§2`). The cap phrase to use is *"up to `MaxMapMarks` (9) circles"*.

---

## 3. ⭐ DECLARED ADDITIONS BEYOND THE PINNED REGISTRY (`SC-§15` / `AS-§21.9` — declared, ⛔ never silent)

⭐ **None of them changes a pinned signature, so ⛔ neither 745 nor 746 can break on them.** Each is offered as a law amendment if the manager wants it in `MARK-§5`:

1. **`FSiegeMapMark::FirstMarkNumber = 1`** — the allocator's floor and `MakeSymbol`'s validity floor must AGREE; two literal `1`s that can drift would let the store hand out a number whose symbol is the empty string.
2. **`USiegeMapMarkSubsystem::FindMark(int32) const`** — convenience over `GetMarks()`, walks the same array. ⛔ Not a second source of truth. The returned pointer is invalidated by the next mutation (documented at the declaration). Nothing in 745/746 is required to use it.
3. **`MinMarkRadiusUU = 250.f` / `MaxMarkRadiusUU = 12000.f`** — the MODEL's sanity fence in **world uu**, ⛔ NOT the wheel's feel. `MARK-§4` reserves the wheel step/min/max to 745 in **widget space** and forbids reusing `GroupRadiusWheelStep`'s numbers; these two are separately named, `EditDefaultsOnly`, and each carries its consequence (`HIGH-§1`). Consequences, verbatim from the header: below 250 uu (= 2.5 m) a mark is smaller than a unit's footprint and the drawn digit no longer fits its own circle; 12,000 uu is the arena's SHORT half-extent (`ArenaHalfExtent` ships `(26000, 12000)`, WarMapWidget.h:107), so a mark at the ceiling already spans the battlefield and anything larger denotes "everywhere", which is not a place.
4. **`LogSiegeMapMark`** — the standing `LogSiege<Domain>` law. The cap REFUSAL logs at `Log` (grep-able after a playtest); adds/removes/clears at `Verbose`. ⛔ The refusal is deliberately not a Warning — hitting a cap is legal play.
5. **⛔ NOT ONE `UFUNCTION` ON THE CLASS, and it is deliberate:** UHT would REJECT every reflected form here — `FSiegeMapMark` is a plain struct (the pinned shape) so it cannot be a reflected parameter, and `GetMarks()` returns a `const TArray<...>&`, not a legal reflected return type. ⚠️ A well-meaning "expose to Blueprint" edit does not fail review, it fails UHT at TASK-754 (the `KBD-§8` trap, recorded in the header so nobody re-discovers it).

---

## 4. ⛔ HOW THE HOLE-PRESERVING NUMBERING IS **ENFORCED** (not merely intended)

**Three mechanisms, and the third is the one that survives a future "tidy-up":**

1. **There is NO counter to renumber from.** The class holds ⛔ no `NextId` member. The next number is DERIVED at each add by `FindLowestFreeNumber()`, which scans `Candidate = FirstMarkNumber .. MaxMapMarks` and returns the first number no live mark holds (`INDEX_NONE` = full). ⇒ **"lowest free" and "clear resets numbering" are the SAME mechanism** — `ClearMarks` cannot forget to reset an allocator that does not exist.
2. **`RemoveMark` removes by NUMBER via `IndexOfByPredicate` + `RemoveAt`, and writes ⛔ nothing else.** `RemoveAt` shifts INDICES; no surviving mark's `Number` is touched on any path. ⛔ There is no compaction pass anywhere in the file.
3. **The AIRLOCK REASON is written at the allocator, as the spec requires** — verbatim in `FindLowestFreeNumber`'s comment: the map writes a symbol into the input box and *the player sends it himself*, so an arbitrary interval separates composing `circle_2` from pressing Enter; renumbering 2 → 1 would make a symbol already sitting unsent denote DIFFERENT GROUND — an order already given, quietly redirected. The comment also records that lowest-free REUSE keeps a smaller version of the same hazard and that `M-1` ACCEPTED it because the cap is 9.
4. ⛔ **The scan is over numbers, never over indices, and does not depend on the array's order** — so the ascending-by-`Number` display invariant can never corrupt the allocator.
5. **ONE refusal gate.** The cap is `FindLowestFreeNumber()` returning `INDEX_NONE` and nothing else — a second `Marks.Num() >= MaxMapMarks` check could disagree with it, and two rules for one question is how a cap ships off-by-one.

---

## 5. TESTS — 9 new, and **what each would CATCH** (`SHIP-§9c`: every assertion can FAIL)

`Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeMapMarkTest.cpp`, `#if WITH_DEV_AUTOMATION_TESTS`, `IMPLEMENT_SIMPLE_AUTOMATION_TEST` + `EditorContext | EngineFilter`, names under **`Siegebound.MapMarks.*`** (a new, collision-free namespace).

| # | test | ⛔ the wrong implementation it kills |
|---|---|---|
| 1 | `SymbolIsCircleUnderscoreNumber` | a BARE DIGIT symbol (`"1"` — already a COUNT token in Zone A); a case/spacing/hyphen drift (asserted with **`TestEqualSensitive`**, ⛔ never `TestEqual`, per `SC-§13`); `circle_0` handed out for an invalid number |
| 2 | `SequentialAssignment` | 0-based numbering; the array INDEX used as identity; a number filed differently from the one returned. ⛔ Expected symbols are **literals** (`"circle_1"`…), ⛔ never `MakeSymbol` called on both sides |
| 3 | `DeleteLeavesTheHoleAndNeverRenumbers` | ⭐ **THE `M-1` ASSERTION.** After deleting 2 of {1,2,3} the survivors are **{1, 3}**, ⛔ not {1,2} — and each survivor still denotes the ground it was drawn on (a renumbering store passes a numbers-only check and fails the pairing), and `FindMark(2)` answers **nothing** |
| 4 | `LowestFreeNumberIsReused` | `Num()+1` and `Highest+1` allocation; a remembered scan position. With holes at 1 and 3, the next add must take **1**, then **3** |
| 5 | `CapRefusesAndMutatesNothing` | an off-by-one cap; a silent no-op returning true; a refused add that still clobbers `OutMark` (**sentinel-checked on all three fields**); a refusal that consumed a number (freeing one slot must re-admit exactly one add, at the freed number) |
| 6 | `RadiusIsClamped` | no clamp; a "clamp" that pins everything to one bound (an in-range radius must survive EXACTLY); a NaN reaching a stored mark — ⚠️ and specifically that a NaN must become the **MINIMUM**, because `FMath::Clamp` is `Max(Min(X,Max),Min)` so an unguarded implementation stores the **MAXIMUM** for a NaN |
| 7 | `ClearMarksEmptiesAndNumberingRestarts` | a monotonic "next id" that survives the clear ⇒ the new match's first circle labelled 4 |
| 8 | `RemoveRefusesUnknownNumbers` | ⭐ **remove-by-INDEX wearing remove-by-number's clothes:** `RemoveMark(0)` must refuse and mark 1 must survive; also `-1`, out-of-range, empty-store and double-delete |
| 9 | `StoreIsLocalPlayerScoped` | a reparent to `UGameInstanceSubsystem`/`UWorldSubsystem` (asserted at the TYPE **and** at `ClassWithin == ULocalPlayer`), which would break `M-2` and void the `MARK-§6` M8 declaration structurally; **and a cap silently raised past Jonathan's 9** |

**Fixture:** a throwaway `ULocalPlayer` (`NewObject<ULocalPlayer>(GEngine)`) outers the subsystem — `ULocalPlayerSubsystem` is `UCLASS(Within = LocalPlayer)`, so a bare `NewObject` trips the `ClassWithin` check (UObjectGlobals.cpp:3313). Verified at the source that this is safe: `ULocalPlayer`'s ctor only sets `PendingLevelPlayerControllerClass` (LocalPlayer.cpp:232-237) and its subsystem COLLECTION is initialised in `PlayerAdded` (:262), which nothing here calls ⇒ ⛔ no PIE, ⛔ no viewport, ⛔ no world, ⛔ no second instance. An invalid fixture FAILS the test; ⛔ it never silently skips.

**⚠️ ONE DELIBERATE DIVERGENCE FROM TWO SHIPPED TEST FILES, declared in-line at the code:** this file DOES construct a NaN, where `SiegeAssistantSelectionTest.cpp:2986-2992` and `SiegeLadderClimbTest.cpp:228-232` both refused to. Both reasons were checked at the source and neither applies: the NaN is built from its **bit pattern** (⛔ no arithmetic for a fast-math build to fold) and no NaN COMPARISON is asserted; both the shipped guard (`FMath::IsFinite`) and the check (`FMath::IsNaN`) are **bit-mask** tests (GenericPlatformMath.h:573-586), not comparisons; and the NaN goes into a bare `float`, ⛔ never into the `FVector2D`, so `DiagnosticCheckNaN` is never reached. A fixture self-check asserts the value really is a NaN before the claim is made.

### Suite total for TASK-754 — ⚠️ **the arithmetic, ⛔ not a frozen number**

- **HEAD baseline: 171** ✅ (measured: `git grep IMPLEMENT_SIMPLE_AUTOMATION_TEST HEAD -- .../Tests/` = 171, which matches the board).
- **This task's delta: +9.**
- ⚠️ **The working tree is MOVING under this count while the wave runs** — it read **187** at the start of my authoring and **204** afterwards (my +9 plus other live tasks' files: `SiegeGhostPawnTest.cpp` (749, 8 tests) appeared, and `SiegeLadderClimbTest.cpp`, `SiegeClimbableTowerTest.cpp`, `SiegeAssistantSelectionTest.cpp`, `SiegeAssistantZoneATest.cpp` are all dirty). ⇒ ⛔ **TASK-754 must NOT take 204 from this note as its one number.** **TASK-753 reconciles `171 + Σ(declared deltas)`; my declared delta is exactly `+9`.**

---

## 6. 🚩 FINDINGS / GAPS QA SHOULD SCRUTINISE

1. ⚠️⚠️ **`ClearMarks()` SHIPS WITH ⛔ NO CALLER — `M-4`'s match-reset wiring IS NOT DONE BY ANY TASK I CAN SEE.** The hook belongs in `ASiegeGameMode::PlayAgain()` (`SiegeGameMode.h:117`), a file **TASK-750 sole-owns**, and TASK-750's spec (board §1-§5) does ⛔ not mention marks. ⭐ **This matters more than it reads:** a `ULocalPlayerSubsystem` OUTLIVES an in-place `PlayAgain` and outlives level travel, so ⛔ **without that one line, last match's circles are still on the map in the next match** — and `M-4` explicitly rules that they must not be. ⇒ **ONE LINE, in TASK-750's file or a follow-up:** `if (ULocalPlayer* LP = ...GetLocalPlayer()) { if (USiegeMapMarkSubsystem* S = LP->GetSubsystem<USiegeMapMarkSubsystem>()) { S->ClearMarks(); } }`. ⛔ I did not write it — it is outside my fence, and reaching into 750's file is exactly the collision this batch was decomposed to avoid.
2. 📌 **`EditDefaultsOnly` on a `ULocalPlayerSubsystem` has no editor surface** (a subsystem has no asset to open), so in practice `MaxMapMarks` is retuned in C++. The specifier is present because `MARK-§5` pins it and because it keeps the cap a DATA decision rather than a literal buried in the allocator. ⛔ I did NOT add `config=` — an unrequested ini surface is scope I do not own. Flagged, ⛔ not "fixed".
3. ✅ **AIRLOCK: clean and stricter than required.** ⛔ No `Capture()`, ⛔ no `EnsureSnapshot()`, ⛔ no include of any assistant header, ⛔ no Zone A byte touched, ⛔ no `552` latch, ⛔ no token figure. The only strings this feature produces are `MakeSymbol`'s symbol and log lines that print **numbers and counts only** — ⛔ never a coordinate or a radius (deliberately stricter than the law, so "can arena geometry leave this class in a string?" has one answer instead of a case analysis).
4. ✅ **`MARK-§4` — the wheel:** ⛔ this task adds **no** wheel consumer at all. `ASiegePlayerController::PlayerTick`'s polled group-pick branch is untouched and unread. The widget-space wheel tunables are 745's; my two radius bounds are world-space model bounds and are named differently.
5. ✅ **`M-6` — the region fence:** ⛔ nothing here publishes a region, and `FSiegeMapMark` carries no region field. `GetRegionPlaceNames()` was not touched (it is 746's file anyway).
6. ⚠️ **Cross-module derivation was VERIFIED, ⛔ not assumed:** `ULocalPlayerSubsystem` is `MinimalAPI` in UE 5.8, so I checked that a class in another module derives from it with exactly this shape — `UMassTestLocalPlayerSubsystem` (Developer/MassEntityTestSuite/Public/MassEntityTestTypes.h:597-601), `UCLASS()` + `GENERATED_BODY()`, no constructor.
7. 📌 **No `Initialize`/`Deinitialize` override, deliberately** — nothing to set up, no timer, no delegate, no asset. That is also what lets the whole suite run headless off a bare `NewObject`.
8. ⚠️ **`FSiegeMapMark` holds no `UObject` pointer, and the design depends on it:** it is a plain struct (the pinned shape) ⇒ `TArray<FSiegeMapMark> Marks` cannot be a `UPROPERTY`, which is safe ONLY because there is nothing for the GC to keep alive. Adding a `UObject*` field to that struct silently breaks the argument; the hazard is written at both ends.

---

## 7. WHAT THIS TASK DOES **NOT** DO (so nobody reads green as done)

⛔ No map UI, no click, no wheel, no draw (TASK-745) · ⛔ no `places:` publication and no `ResolvePlace` answer (TASK-746) · ⛔ no help rows (TASK-751) · ⛔ no `PlayAgain` wiring (finding 1) · ⛔ no compile (TASK-754, behind TASK-742) · ⛔ no editor, MCP or Git.
