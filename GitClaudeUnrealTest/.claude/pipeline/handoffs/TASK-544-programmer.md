# TASK-544 — `FSiegeAssistantRegionStatics` — the membership library (ZERO CALL SITES)

- **agent:** gameplay-programmer
- **date:** 2026-08-05
- **status handed to:** `ready-for-qa`
- **QA gate:** ⛔ **TASK-550** (`.claude/pipeline/qa/TASK-550.md` — the batch's ONLY gate; `AS-§21.10`). ⛔ No per-task QA file.
- **law read in full before writing:** CONVENTIONS `AS-§21.4` (region-bearing ruling) · `AS-§21.9` (pinned registry) · `AS-§21.10` (naming/folder) · `AS-§21.11` (designed outcomes) · `AS-§21.12` (M8) · `SC-§15` (declared-departure law) · `NAV-§` RULING 2 (the zero-call-sites precedent)

---

## 1. WHAT CHANGED — THE WHOLE DIFF IS TWO NEW FILES

| file | state | lines |
|---|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/SiegeAssistantRegionStatics.h` | **NEW** | 103 |
| `Source/GitClaudeUnrealTest/Siegebound/SiegeAssistantRegionStatics.cpp` | **NEW** | 33 |

⛔ **NOTHING ELSE WAS TOUCHED.** No existing file edited, no include added anywhere, no `Build.cs`, no `Content/`, no `Tests/`, no `Docs/Data/*.csv`.

✅ **ZERO CALL SITES — MECHANICALLY VERIFIED, NOT ASSERTED.** `grep -rn "SiegeAssistantRegionStatics\|IsPointInRegion" Source/ Plugins/` returns **exactly 4 hits, all inside the new pair itself**: the `.cpp`'s own `#include`, the `.cpp`'s definition, the `.h`'s `struct` line, the `.h`'s declaration. **There is no fifth hit.** TASK-548 adds the only caller.

✅ **`git status --porcelain` shows my footprint as exactly two `??` entries** (the pair). Every other dirty path in the tree belongs to a parallel sibling task (541/542/545/546/…), not to this one.

⭐ **WHY IT SHIPS CALLER-LESS** — `AS-§21.9` / `NAV-§` RULING 2, the `FSiegeNavDiagnostics` precedent: **a diff that is one new file pair proves behaviour-freedom in ONE LOOK.** Nothing existing is edited, therefore nothing existing can have changed. ⚠️ **A "helpful" call site would have destroyed exactly that property**, which is the batch's main safety argument.

---

## 2. THE SIGNATURE AS WRITTEN, NEXT TO THE PIN

**`AS-§21.9` pin:**

```cpp
struct GITCLAUDEUNREALTEST_API FSiegeAssistantRegionStatics
{
    /** 2D XY box, Z IGNORED. Boundary INCLUSIVE (<=), mirroring AAncientGround::IsPointInZone. */
    static bool IsPointInRegion(const FVector& Point, const FVector& Centre, const FVector2D& HalfExtent);
};
```

**As written (`SiegeAssistantRegionStatics.h:62,102`):**

```cpp
struct GITCLAUDEUNREALTEST_API FSiegeAssistantRegionStatics
{
	/** 2D XY box, Z IGNORED. Boundary INCLUSIVE (<=), mirroring AAncientGround::IsPointInZone.
	 *  … (the pinned sentence is the doc comment's FIRST line; the edge cases follow it) */
	static bool IsPointInRegion(const FVector& Point, const FVector& Centre, const FVector2D& HalfExtent);
};
```

✅ **CHARACTER-FOR-CHARACTER ON EVERY LOAD-BEARING TOKEN:** the `struct` keyword, `GITCLAUDEUNREALTEST_API`, the type name, `static bool`, the function name, **all three parameter types, their `const&`-ness AND their order and spelling** — including **`Centre`** (British, as pinned; ⛔ *not* `Center`, which is what both donor bodies call their local). The pinned doc sentence is preserved verbatim as the comment's opening line; the header only **adds** below it.

📌 **ONE OBSERVATION FOR QA, DECLARED NOT BURIED (`SC-§15`) — `struct` vs `class`.** The pin says `struct`; the taskboard `names:` block independently says `struct`; **both agreed, so I wrote `struct`.** ⚠️ **But all four shipped `F`-prefixed statics libraries in the module use `class … { public: … }`**: `FSiegeCombatStatics` (`SiegeCombatStatics.h:23`), `FSiegeKeyboardLayoutStatics` (`:117`), `FSiegeNavDiagnostics` (`:86`), `FSiegeStuckStatics` (`:165`). **I followed the pin over the 4/4 house precedent — the pin is the cross-task contract and two sources agree on it.** ✅ **It is safe either way and I checked why:** MSVC's `C4099` (type-first-seen-as-`class`-now-`struct`) needs a *conflicting* declaration, and there is none — the type is forward-declared nowhere and named nowhere else in the module (see the grep above). The MSVC-mangled name of a static member function carries its enclosing type as a plain scope identifier, so `struct`/`class` **does not change the linked symbol** either. ⇒ **Not a finding; recorded so nobody "reconciles" it later by editing the pin.**

---

## 3. `SC-§15` — THE DECLARED THIRD INSTANCE, WITH BOTH EXISTING INSTANCES CITED AND QUOTED

⚠️ **This body already exists TWICE in the module. I read both in full before writing mine, as spec (2) requires.**

**Instance 1 — `AAncientGround::IsPointInZone`, `AncientGround.cpp:143-151`:**

```cpp
bool AAncientGround::IsPointInZone(const FVector& Point) const
{
	// 2D (XY) box about the actor origin; Z ignored (a region test) — byte-copy
	// of ACaptureZone::IsPointInZone so "standing in the zone" reads identically
	// for both zone actors.
	const FVector Center = GetActorLocation();
	return FMath::Abs(Point.X - Center.X) <= ZoneHalfExtent.X
		&& FMath::Abs(Point.Y - Center.Y) <= ZoneHalfExtent.Y;
}
```

**Instance 2 — `ACaptureZone::IsPointInZone`, `CaptureZone.cpp:112-118`:**

```cpp
bool ACaptureZone::IsPointInZone(const FVector& Point) const
{
	// 2D (XY) box about the actor origin; Z ignored (a spawn/region test).
	const FVector Center = GetActorLocation();
	return FMath::Abs(Point.X - Center.X) <= ZoneHalfExtent.X
		&& FMath::Abs(Point.Y - Center.Y) <= ZoneHalfExtent.Y;
}
```

✅ **THE TWO DO NOT DISAGREE IN ANY DETAIL — THE THREE STATEMENT LINES ARE BYTE-IDENTICAL** (only the comment text differs: *"a region test"* vs *"a spawn/region test"*, and `AncientGround`'s comment additionally states it is a deliberate byte-copy of `CaptureZone`'s). **The spec's STOP-AND-REPORT condition therefore did not fire**, and I did not have to pick one. ⚠️ **This is worth QA's attention as a positive finding: the donors are already an intentional two-instance pair, and each says so in its own comment.**

**Instance 3 — mine (`SiegeAssistantRegionStatics.cpp:11`):**

```cpp
	return FMath::Abs(Point.X - Centre.X) <= HalfExtent.X
		&& FMath::Abs(Point.Y - Centre.Y) <= HalfExtent.Y;
```

⇒ **The EXPRESSION is unchanged. The only difference is where the two operands come from.**

### Why a third instance is owed instead of a call to either

⛔ **Both existing instances are `const` MEMBER functions that read their geometry off a LIVE ACTOR** — `GetActorLocation()` plus the actor's own `ZoneHalfExtent` member. **Mine takes a STORED centre and half-extent.** That is not a preference; it is `AS-§21.4`'s ruling: **SNAPSHOT-TIME GEOMETRY, EXECUTION-TIME MEMBERSHIP.** The snapshot captures each region's geometry **once**, and the selector evaluates membership **later**, from those captured values, **without holding an actor pointer**. ⇒ **At the moment the answer is needed there is no actor to call `IsPointInZone` on.** Reuse is structurally unavailable; the honest move is a declared third instance with the semantics **copied, not re-decided**.

⚖️ **This is a checkable mechanism, not a quality judgement** (`SC-§15`'s own test): the reader can verify it by looking at the two donors' signatures — they are non-static members with no centre/extent parameters.

### ⛔ The NUMBER still has exactly one owner, and it is not this file

**There is not a single numeric literal in either new file.** The half-extent is read from the shipped accessors — `AAncientGround::GetZoneHalfExtent()` (`AncientGround.h:117`) and `ACaptureZone::GetZoneHalfExtent()` (`CaptureZone.h:131`) — by the snapshot's `Capture()` (**TASK-547**, not mine), and arrives here as a parameter. ⚠️ Those two extents are a **PAIRED TUNABLE** that both actor class docs flag (`AncientGround.h:46-47`, `CaptureZone.h:170-175`, both `FVector2D(840,840)`); **hard-coding one here would silently fork it**, which is precisely the failure `AS-§21.4` names — *"a unit that is 'in the ground' for one system and not the other."*

---

## 4. THE CONTRACT — BOUNDARY, Z, AND THE THIRD EDGE CASE

| property | ruling | where it lives |
|---|---|---|
| **shape** | **2D (XY) box** about `Centre` | `.h` doc + `.cpp` comment |
| **Z** | ⛔ **IGNORED ENTIRELY.** `Centre.Z` and `Point.Z` are read by nothing; the Z difference is never computed. A caller may pass anything, including infinities. | both |
| **boundary** | **INCLUSIVE — `<=`, not `<`. A point exactly on the edge is INSIDE.** | both |

⭐ **THE Z RULING IS COPIED, AND I SAY SO RATHER THAN RE-DECIDING IT.** Units stand on terrain of varying height and the hills are climbable, so a 3D test would exclude a unit standing on a rise **inside** the ground — *"everyone in the ancient ground"* would quietly omit whoever walked uphill. **Both shipped instances already made this choice and both say so in their own doc comments** — `AncientGround.h:108-110` states it outright: *"Z is IGNORED (a region test, exactly like `ACaptureZone::IsPointInZone`, so a unit on a slight rise inside the footprint still counts)."*

⚖️ **Why `<=` is not a detail:** this function exists to give the assistant **the same answer the game already gives.** Flipping it to `<` would make a unit *"in the mid"* for capture scoring and *"not in the mid"* for selection — the divergence class `AS-§21.4` forbids.

### The three edge cases, documented in the header because TASK-549 asserts every one

1. **Exactly ON the boundary ⇒ INSIDE** (`<=`, byte-for-byte as both donors).
2. **Z far above / far below ⇒ STILL INSIDE**, at any Z.
3. **A zero or negative half-extent ⇒ the region DEGENERATES, and it is NOT special-cased. THIS IS A CHOICE AND IT IS STATED, not left to be discovered.**
   - **Zero** extent on an axis ⇒ only points **exactly** on that axis of the centre pass.
   - **Negative** extent ⇒ **nothing** is ever inside, because `FMath::Abs(...)` is never negative.
   - ⛔ **No clamp, no `Abs` on the extent, no early-out guard.** **Two reasons: (a)** any guard would be a **DIVERGENCE from the two shipped instances**, which is exactly what this function exists *not* to be — neither donor guards, so a guarded third instance would answer differently from the game for the same geometry; **(b)** the degenerate answer is **FAIL-CLOSED**: an empty region selects nobody, and an empty selection is a **loud refusal with arithmetic** in the executor (`AS-§21.11`, designed outcome 4) — never a silently unfiltered army.
   - ⚠️ **Bonus, same fail-closed direction: a NaN coordinate is OUTSIDE**, because every comparison against NaN is false. Documented in the header for the same reason.

---

## 5. PURITY — THE PROPERTY TASK-549 DEPENDS ON

⛔ **No `UWorld`, no `AActor`, no `UObject`, no engine subsystem, no allocation, no state, no logging, no statics-with-storage.** Three plain values in, a `bool` out.

- **`.h` includes: `CoreMinimal.h` and nothing else.** `FVector`, `FVector2D` and `FMath` all arrive complete through it — **complete-type include law satisfied without a single extra include**, because no incomplete type is named anywhere in the pair (there is not even a forward declaration to make).
- **`.cpp` includes: its own header and nothing else**, with a comment saying that is deliberate and what it means if that changes — the `FSiegeStuckStatics.cpp` precedent, which carries the identical comment for the identical reason.
- ⛔ **No `Build.cs` change**, as specified: `Core`/`CoreUObject` already cover `FVector`/`FVector2D`, and the pair adds no other dependency.
- **Not a UObject / not reflected** ⇒ no `BeginPlay`, no GC surface, no `UCLASS`/`USTRUCT`/`GENERATED_BODY`, **no `.generated.h`** and therefore **no UHT participation at all**.

⭐ **THE POINT WORTH PUTTING IN FRONT OF QA: THE EXCLUSION FILTER THIS MIRRORS HAS NO AUTOMATED TEST AT ALL TODAY.** Because this predicate takes three plain values and touches no world, **TASK-549 can assert every branch of it headlessly** — boundary, far-Z, degenerate extent. ⇒ **This feature will be tested BETTER than the shipped feature it copies.** That is the strongest available argument for the file existing at all, and it is the direct consequence of the purity constraint.

---

## 6. STANDING TRAPS — WALKED, EACH ONE

| trap | status |
|---|---|
| **shadowing an inherited reflected member** | ⛔ **UNREACHABLE — there is no base class.** `FSiegeAssistantRegionStatics` inherits from nothing and holds **no data members at all** (not even private ones), so there is nothing to shadow and nothing to be shadowed. |
| **most-vexing-parse** | ⛔ **UNREACHABLE — the pair declares no local variable of class type.** The `.cpp` body is a single `return` of one boolean expression; there is no `T x(...)` anywhere, and no default-construction of anything. |
| **complete-type includes** | ✅ Every type named (`FVector`, `FVector2D`, `FMath`) is **complete** via `CoreMinimal.h`. No forward declaration is used, so no incomplete type can be dereferenced. |
| **`Build.cs`** | ✅ untouched. |
| **narrowing / precision** | ✅ `FVector` and `FVector2D` are both `double`-component in UE5, so `Abs(double) <= double` compares like-for-like — **identical to the donors**, which already compare an `FVector` delta against an `FVector2D` component. No cast, no `float` literal, no `KINDA_SMALL_NUMBER`. |
| **encoding** | ✅ UTF-8 **without BOM**, matching every existing file in the folder (verified by `od` on three of them); comment emoji are house style here (`SiegeNavDiagnostics.h` precedent). ⚠️ The ASCII-only constraint is `AS-§21.2`'s and applies to the **prompt byte budget** (`SiegeAssistantVocabulary.cpp`), **not to C++ comments** — this file moves **zero prompt bytes.** |

---

## 7. 📌 M8 DECLARATION (verbatim, `AS-§21.12`)

> **This feature adds no replicated property, no new replicated class, no new relevancy tier.**

✅ **And the reason is structural, not incidental:** `FSiegeAssistantRegionStatics` is a **non-UObject static library with no members and no instances** — there is no object to replicate, no `UPROPERTY`, no `GetLifetimeReplicatedProps`, and nothing that could enter a relevancy tier. **The membership test runs where the selector already runs — on the authority.** *"There is nothing to declare"* only counts when it is stated.

---

## 8. ⚠️ WHAT QA SHOULD SCRUTINISE (my own list, hardest first)

1. ⛔ **THE ZERO-CALL-SITES CLAIM — re-run the grep yourself, do not take my word.** `grep -rn "SiegeAssistantRegionStatics\|IsPointInRegion" Source/ Plugins/` must return **exactly 4 hits, all inside the new pair.** A fifth hit means the batch's behaviour-freedom argument is dead.
2. ⛔ **THE EXPRESSION vs BOTH DONORS, TOKEN BY TOKEN.** `AncientGround.cpp:143-151` and `CaptureZone.cpp:112-118` against `SiegeAssistantRegionStatics.cpp:31-32`. **`<=` on both axes, `X` against `HalfExtent.X`, `Y` against `HalfExtent.Y`, `&&` not `||`, and no Z term.** ⚠️ **A transposed axis (`Point.X` vs `HalfExtent.Y`) would pass every square-region test and only fail on a non-square extent** — and the shipped extents are square `(840,840)`, so **the shipped data cannot catch that mistake.** This is the single highest-value thing to eyeball, and it is worth telling TASK-549 to use a **deliberately NON-SQUARE** extent for exactly this reason.
3. **`Centre` vs `Center` spelling in the signature** — the pin is British, both donors' locals are American. I used the pin's. A "fix" toward the donors' spelling would break TASK-548's call against the pin.
4. **`struct` vs `class`** — §2 above. Pin + `names:` block both say `struct`; 4/4 house precedent says `class`. Declared, with the C4099 / name-mangling reasoning. **Ratify or rule; do not leave it ambiguous for TASK-548.**
5. **The degenerate-extent choice (edge case 3)** is a *decision*, not an oversight. If QA wants a guard, that is a **ruling that must also apply to the two shipped donors** — otherwise it re-creates the divergence this task exists to prevent.

---

## 9. NOTES / DECLARED DEPARTURES

- 📌 **BOARD STATUS WRITTEN ONCE, NOT TWICE — DECLARED.** The spec's flow is `in-progress` → `ready-for-qa`. **I wrote a single edit straight to `ready-for-qa`.** ⚖️ Reason: **six sibling tasks (541/542/543/545/546/547) are editing `TASKBOARD.md` in parallel** and this pipeline has a recorded board-write-race history; the work was already complete when the board was touched, so a transient `in-progress` write would have **doubled the race window for zero information.** ⚠️ Flagged rather than buried, per `SC-§15`.
- ✅ **Nothing in the spec was found wrong.** Every clause was implementable as written; the two donors matched each other, so the STOP-AND-REPORT branch of spec (2) did not fire. The `struct`/`class` note in §2 is a **precedent observation**, not a spec defect.
- ⛔ **NOT DONE, BY INSTRUCTION:** no compile (**TASK-551** owns the only compile), no tests (**TASK-549**), no call site (**TASK-548**), no Git.
