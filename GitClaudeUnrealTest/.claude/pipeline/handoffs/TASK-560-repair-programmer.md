# TASK-560 — THE `C2248` REPAIR (spec item **(R)**, ruling `W4-R5`) — programmer handoff

**Date:** 2026-08-15 · **QA loop 1 of 3** · **Status → `ready-for-qa`** · **Gate = `qa/TASK-584.md`** · compile = TASK-566 re-run · commit = TASK-570
⛔ **FILE-ONLY, AND STRICTLY: no compile, no Git (⛔ zero git commands — see §7 D-1), no editor (⛔ left CLOSED, deliberately, per the build-master's note), no MCP, no PIE.**
⛔ **This is a NEW file. `handoffs/TASK-560-programmer.md` is a dated record and was NOT overwritten** — a pointer section was appended to its end (board `(R7)` + the dispatch's "append" instruction, reconciled in §7 D-2).

---

## 1. THE ONE-LINE RESULT

**Four lines of new code, two changed lines, one changed comment line, one changed include note.** The two `C2248`s are repaired by the pinned **shape 2**: two `public`, zero-parameter, non-`UFUNCTION` C++ statics on `UCombatantHealthBarComponent` that read the CDO's own `protected` fields. ⛔ **The fields stay `protected`. ⛔ No literal was hardcoded. ⛔ `CombatantHealthBarComponent.cpp:110` was not touched. ⛔ TASK-579's status-line work in `WarMapWidget.{h,cpp}` is intact and is quoted verbatim in §4.**

| file | change | shape |
|---|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/CombatantHealthBarComponent.h` | **+2 declarations** (+ their doc comment) | ⛔ **strictly additive** — 0 existing lines modified, moved or deleted |
| `Source/GitClaudeUnrealTest/Siegebound/CombatantHealthBarComponent.cpp` | **+2 one-line definitions** (appended after `HideBar()`) | ⛔ **strictly additive** — 0 existing lines modified, moved or deleted |
| `Source/GitClaudeUnrealTest/Siegebound/WarMapWidget.cpp` | **3 lines → 2**, 1 comment line, 1 include note | **5 changed lines** |
| `Source/GitClaudeUnrealTest/Siegebound/WarMapWidget.h` | ⛔ **NOT OPENED FOR EDIT — 0 changed lines** | — |

⭐ **The `(R5)` budget: the diff across the `WarMapWidget` pair is 5 changed lines, under the ~8 ceiling, and all five are inside the palette read + its include note.**

---

## 2. `(R1)` + `(R2)` — THE ADDED ACCESSORS, PASTED

### `CombatantHealthBarComponent.h` — inserted in the EXISTING `public:` block, after `HideBar()`, before `protected:`

```cpp
	/**
	 *  ⭐ THE TEAM PALETTE'S PUBLIC READ SEAM — WR-§6, manager ruling W4-R5 (added at TASK-560's
	 *  C2248 repair). These return this component's CLASS DEFAULTS (the CDO's BlueBarColor /
	 *  RedBarColor), so any display that must match a health bar's team tint — UWarMapWidget's
	 *  ally and enemy dots are the first caller — reads the ONE shipped owner instead of
	 *  re-typing the literals and drifting the day the palette moves.
	 *
	 *  ⛔ STATIC, AND THAT IS THE ENTIRE MECHANISM: a static member may read its own class's
	 *  protected members, so BlueBarColor/RedBarColor STAY protected below and gain NO writable
	 *  surface. ⛔ Plain C++ statics, NOT UFUNCTIONs — a palette read is not a Blueprint API and
	 *  reflecting it would invite a second caller. ⛔ ZERO parameters, deliberately: a
	 *  team-parameterised accessor would force Siegebound/TeamId.h into this header, which today
	 *  only the .cpp includes. (Zero parameters also means SC-§33 cannot fire structurally.)
	 *
	 *  ⚠️ CLASS DEFAULTS, ⛔ NOT AN INSTANCE READ, AND THE DIFFERENCE IS DELIBERATE: the per-bar
	 *  team tint applied in BeginPlay reads THIS INSTANCE's fields, which a BP subclass may
	 *  legitimately override. That read is a different question and is left exactly as it is —
	 *  routing it through these accessors would silently delete per-BP tint overrides.
	 */
	static FLinearColor GetDefaultBlueBarColor();
	static FLinearColor GetDefaultRedBarColor();
```

⛔ **NO `#include` WAS ADDED TO THAT HEADER.** Its include block is byte-unchanged: `CoreMinimal.h` · `Components/WidgetComponent.h` · `UObject/SoftObjectPtr.h` · the `.generated.h`. `FLinearColor` was already required by the two `UPROPERTY`s themselves.

### `CombatantHealthBarComponent.cpp` — appended after `HideBar()`, the file's last function

```cpp
FLinearColor UCombatantHealthBarComponent::GetDefaultBlueBarColor()
{
	// ⭐ A static member function may read its own class's protected members — which is the whole
	// reason this seam exists in this shape: BlueBarColor stays protected and no caller gains a
	// way to write it (WR-§6, ruling W4-R5).
	//
	// ⛔ NO NULL BRANCH, DELIBERATELY: GetDefault<T>() on a statically-linked native UCLASS always
	// returns that class's CDO, so a defensive branch here would be untestable dead code.
	return GetDefault<UCombatantHealthBarComponent>()->BlueBarColor;
}

FLinearColor UCombatantHealthBarComponent::GetDefaultRedBarColor()
{
	// Same contract as GetDefaultBlueBarColor() above: class defaults, no null branch, and the
	// field stays protected.
	return GetDefault<UCombatantHealthBarComponent>()->RedBarColor;
}
```

### ⭐ THE ONE COMPILE FACT I CHECKED AT THE ENGINE RATHER THAN ASSUMED — `GetDefault<T>()` NEEDS NO NEW INCLUDE

The template is declared at `Engine/Source/Runtime/CoreUObject/Public/UObject/UObjectGlobals.h:2194` (`inline const T* GetDefault()`). ⛔ It is **not** in `CoreMinimal.h` and **not** in `UObject/Object.h` (I listed Object.h's includes: `Script.h` · `ObjectMacros.h` · `UObjectBaseUtility.h` · `ObjectCompileContext.h` · `ResourceSize.h` · `PrimaryAssetId.h` · `VersePathFwd.h` · `Object.generated.h` — ⛔ no `UObjectGlobals.h`). ✅ **The chain that delivers it into this TU is real and short:** `CombatantHealthBarComponent.cpp:7` includes `GameFramework/Actor.h` → `:11 Templates/SubclassOf.h` → `:6 UObject/Class.h` → `:63 UObject/UObjectGlobals.h`. ⇒ **available, with zero new includes in either file.**
⚠️ **Honest bound (`SC-§32`):** this is an include-chain read, ⛔ not a compile. TASK-566's re-run remains the authority.

---

## 3. `(R4)` — THE CALL-SITE CHANGE, PASTED (⛔ located by symbol, `SC-§18c`)

The palette read lives in `UWarMapWidget::PaintMapContents`, immediately after the degenerate-rect early-out. **Before** (the two lines the compiler rejected, at the build-master's cited `:759-761`):

```cpp
	// enemy dot rows). The CDO carries UCombatantHealthBarComponent's C++ defaults, so blue
	...
	const UCombatantHealthBarComponent* const PaletteOwner = GetDefault<UCombatantHealthBarComponent>();
	const FLinearColor AllyColor = PaletteOwner->BlueBarColor;   // C2248
	const FLinearColor EnemyColor = PaletteOwner->RedBarColor;   // C2248
```

**After** (now `:754-760`, the whole surviving block so the preserved argument is visible):

```cpp
	// ⛔ THE TEAM PALETTE IS READ FROM THE SHIPPED OWNER, ⛔ NEVER RE-TYPED (WR-§6's ally and
	// enemy dot rows). The accessors return the owner's own class defaults (W4-R5), so blue
	// here is byte-identical to blue on every health bar in the game and a future palette
	// change reaches this map with no edit. Same structural-escape reasoning as
	// ResolveArenaHalfExtent, applied to a colour.
	const FLinearColor AllyColor = UCombatantHealthBarComponent::GetDefaultBlueBarColor();
	const FLinearColor EnemyColor = UCombatantHealthBarComponent::GetDefaultRedBarColor();
```

✅ **Exactly ONE comment line moved** — the mechanism sentence, because the CDO read now lives in the owner. ⭐ **The ARGUMENT survives word for word:** read from the shipped owner, never re-typed, a future palette change reaches this map with no edit, same structural escape as `ResolveArenaHalfExtent`. ⛔ **The `#include` was KEPT**, its note re-pointed at the accessors:

```cpp
#include "Siegebound/CombatantHealthBarComponent.h" // the SHIPPED team palette - GetDefaultBlueBarColor() / GetDefaultRedBarColor()
```

⛔ **No literal colour was typed into `WarMapWidget.cpp`.** The file still contains **zero** hand-typed palette values — the sweep in §5 shows every `BlueBarColor`/`RedBarColor` occurrence in `Source/` and none of them is a re-transcription by this repair.

---

## 4. ⛔⛔ `(R5)` — THE WRITE-RACE CHECK: TASK-579'S WORK SURVIVED, QUOTED

**I read the current `WarMapWidget.cpp` before editing and made two surgical `Edit`s** (the palette block, the include note). ⛔ **No re-generation from an earlier draft, no re-format, no re-order, and `WarMapWidget.h` was never opened for edit.** Post-edit read-back of every TASK-579 site:

| TASK-579 element | post-edit line | quoted, verbatim |
|---|---|---|
| the one string | `:87` | `static const TCHAR* NoSnapshotStatusText = TEXT("No place markers yet - send your commander one order in the console and they appear. The console opens over this map, so you do not have to close it.");` |
| decision site 1 — **the first open** | `:401`, `:413-415` | `// ⭐ TASK-579 - THE FIRST-OPEN STATUS LINE (WR-§9 row 12).` … `if (GetReadOnlySnapshot() == nullptr)` / `{` / `ShowNoSnapshotHint();` |
| decision site 2 — **the empty click**, discriminator MOVED off `Markers.Num()` | `:871`, `:876-878` | `// ⚠️ TASK-579 MOVED THIS DISCRIMINATOR FROM Markers.Num() TO THE SNAPSHOT, AND IT IS A` … `if (GetReadOnlySnapshot() == nullptr)` / `{` / `ShowNoSnapshotHint();` |
| decision site 3 — **the one-way latch on the EXISTING ally-dot timer** | `:593`, `:944`, `:952` | `// ⚠️ TASK-579 POINTED THIS AT HandleMapRefreshTimer RATHER THAN AT RefreshAllyDots DIRECTLY.` · `if (!bShowingNoSnapshotHint)` · `if (GetReadOnlySnapshot() == nullptr)` |
| the latch's single writer-for-false | `:910-917` | `// ⭐⛔ THE LATCH IS RETIRED BY EVERY LINE, AND RE-ARMED ONLY BY ShowNoSnapshotHint() BELOW,` … `bShowingNoSnapshotHint = false;` |
| the latch's single writer-for-true, armed AFTER | `:929-935` | `void UWarMapWidget::ShowNoSnapshotHint()` … `SetStatusLine(SiegeWarMap::NoSnapshotStatusText);` … `bShowingNoSnapshotHint = true;` |
| the header's TASK-579 doc + `StatusTextBlock` | `WarMapWidget.h:268`, `:271`, `:292`, `:552`, `:648`, `:662`, `:665`, `:670`, `:679`, `:703`, `:706` | ⛔ **file not edited at all** |

⭐ **The arithmetic that proves nothing else moved:** every TASK-579 site **above** the palette block sits at its original line number (`:87`, `:401`, `:413-415`, `:593`); every site **below** it sits at exactly **original − 1** (`:872→871`, `:877→876`, `:909→908`… as read back), which is the single net line this repair removed (3 lines → 2). ⛔ **A revert, a re-order or a stray re-format could not produce a uniform −1.**

---

## 5. `SC-§33` — THE ENUMERATED SWEEP, PASTED, EVERY HIT CLASSIFIED

⭐ **The structural answer first: both new functions take ZERO parameters (`GetDefaultBlueBarColor()` / `GetDefaultRedBarColor()`), so `SC-§33` cannot fire on them at all** — there is no parameter to default and no call site that could omit one. The sweeps are pasted anyway because the dispatch asks for them.

**Sweep 1 — every occurrence of the new symbols, whole `Source/` tree — RAW COUNT 8, 0 files outside the two owned pairs:**

| # | hit | classification |
|---|---|---|
| 1-2 | `CombatantHealthBarComponent.h:77`, `:78` | **the two declarations** — `()` empty parameter lists, visible in the paste above |
| 3 | `CombatantHealthBarComponent.cpp:229` | definition |
| 4 | `CombatantHealthBarComponent.cpp:240` | definition |
| 5 | `CombatantHealthBarComponent.cpp:242` | comment cross-reference (`// Same contract as GetDefaultBlueBarColor() above`) |
| 6 | `WarMapWidget.cpp:28` | the include note |
| 7-8 | `WarMapWidget.cpp:759`, `:760` | ⭐ **the ONLY two call sites in the module, both new, both passing zero arguments** |

⇒ ⛔ **ZERO pre-existing call sites** (the `SC-§33` scope exemption for a brand-new function applies, and is moot anyway at zero parameters).

**Sweep 2 — every occurrence of the two PROTECTED fields, whole `Source/` tree — RAW COUNT 20, every one classified:**
- **7** are inside `CombatantHealthBarComponent.h` (`:60`, `:61`, `:66` = my new doc comment · `:77`, `:78` = the declarations · `:137`, `:141` = **the two `UPROPERTY`s, still under `protected:`, byte-unchanged**).
- **7** are inside `CombatantHealthBarComponent.cpp` (`:110` = ⛔ **the INSTANCE read, NOT TOUCHED** — see §6 · `:229`, `:232`, `:237`, `:240`, `:242`, `:244` = my two definitions).
- **3** are in `WarMapWidget.cpp` (`:28`, `:759`, `:760`) and ⛔ **name the ACCESSORS, never the fields**.
- **2** are prose in other files — `SiegePlayerController.h:841` (*"…RedBarColor until the map closes"*) and `Torch.h:205` (*"BlueBarColor / RedBarColor are the combatant…"*). ⛔ **Comments, not code; not my surface; not touched.**
⇒ ⭐ **NO OTHER FILE IN THE MODULE READS EITHER PROTECTED FIELD ⇒ there is no second `C2248` waiting anywhere in `Source/`.**

**Sweep 3 — defaulted parameters in the two owned headers:**
- `CombatantHealthBarComponent.h`: **12** raw hits of the `=`-inside-parens shape, and **all 12 are macro arguments** — `UCLASS(ClassGroup = (Siegebound), meta = …)` at `:43` and eleven `UPROPERTY(EditDefaultsOnly, Category = …)` lines. ⛔ **ZERO function-parameter defaults, before or after this repair.**
- `WarMapWidget.h`: **2** hits, both **pre-existing and unchanged** — `TSubclassOf<UWarMapWidget> MapClass = nullptr,` and `int32 ZOrder = 0);` on `CreateAndAddToViewport`, whose single call site (`SiegePlayerController.cpp`, TASK-563) already passes both explicitly, as `qa/TASK-565.md` §2a verified.
⇒ ⛔ **This repair adds no defaulted parameter anywhere.**

---

## 6. `(R3)` + `(R6)` — WHAT I DELIBERATELY DID **NOT** DO

- ⛔ **`CombatantHealthBarComponent.cpp:110` IS UNTOUCHED.** It still reads `const FLinearColor BarColor = (TeamAgent && TeamAgent->GetTeamId() == ETeamId::Red) ? RedBarColor : BlueBarColor;` — **the INSTANCE's** fields, which a `BP_` subclass may legitimately override. ⭐ **Routing it through the new accessors would compile green and silently delete every per-BP tint override.** Two reads, two different questions: the bar asks *"what colour is THIS component configured to be"*, the map asks *"what colour is the SHIPPED default"*. **The distinction is recorded in the header doc comment instead of being coded around.**
- ⛔ **No literal was hardcoded** — the `(1.00, 0.10, 0.05)` / `(0.05, 0.30, 1.00)` values appear in exactly one place in the module, their `UPROPERTY` initialisers.
- ⛔ **`SiegeFeedbackLibrary::TeamTint`'s hand-typed mirror was NOT touched** — TASK-585 owns it, blocked-by the commit. My sweeps confirm it does not name either field symbol, so it is not a `C2248` risk.
- ⛔ **No `Tests/` edit** (TASK-564's in-flight surface) · ⛔ no behaviour change · ⛔ no new tunable · ⛔ no new log category · ⛔ no `Build.cs` change · ⛔ nothing in `Content/`, no `.csv`, no `L_Arena`.
- ⛔ **No `UFUNCTION`, no `BlueprintPure`, no reflection** on the two new statics — a reflected palette read invites a second caller (the `ComposeAppendedInput` precedent).

## 6b. 🔒 THE AIRLOCK — UNTOUCHED BY CONSTRUCTION

⛔ **`SiegeAssistantSnapshot.{h,cpp}` · `SiegeAssistantVocabulary.{h,cpp}` · `Tests/SiegeAssistantZoneATest.cpp` are ABSENT from my diff — none was opened, read for edit, or written.** This repair edits **three files**, all named in §1, and **zero prompt-zone bytes**. Zone A stays byte-frozen at **5658 chars**; no `who` shape, no grammar change, no schema change. ⛔ **NO TOKEN FIGURE IS QUOTED OR DERIVED ANYWHERE IN THIS HANDOFF** (`AS-§12g`). 🔒 The sealed holdout and TASK-552's one-shot latch were not approached.
✅ **The widget's outbound surface is still ONE delegate carrying ONE `FName`** (`OnPlacePicked.Broadcast(PickedSymbol)`, unchanged at `:893`), and **the file still contains no place-symbol literal.**

---

## 7. ⚖️ DECLARED DEPARTURES / RECONCILIATIONS (`SC-§15`)

**D-1 — ⛔ ZERO git commands, so I make NO diff-based claim.** The dispatch says FILE-ONLY *"no Git"* with no read-only exception; `qa/TASK-565.md` §11 permits read-only Git. **I took the stricter reading** (TASK-564's repair took the same line). ⇒ My line-count claims in §1 and §4 are derived from **reading the files**, ⛔ not from `git diff --numstat`. **The byte-level proof remains build-master's at the TASK-566 re-run.**

**D-2 — the handoff filename, reconciled rather than chosen.** Board spec `(R7)` requires a **NEW** file `handoffs/TASK-560-repair-programmer.md`; the dispatch says **append to** `handoffs/TASK-560-programmer.md`. ⇒ **Both are satisfied:** this file is the full record, and a pointer section carrying the required pasted evidence (accessors · call-site change · TASK-579 survival) was **appended** to the original, which was ⛔ **not overwritten and not edited above its final line**.

⛔ **No other departure.** The pinned shape was implemented character-for-character; ⛔ **I did not re-derive the choice between the three shapes.**

---

## 8. 📌 `(R7)` M8 DECLARATION

⛔ **No replicated property. ⛔ No new class. ⛔ No new relevancy tier. ⛔ No RPC.** Two `static` C++ functions returning a `FLinearColor` from a CDO are **client-local and stateless**; nothing here crosses the wire, and ⛔ neither is a `UFUNCTION`, so neither can be called from Blueprint or replicated. ⛔ `GetFirstPlayerController` is not used.

---

## 9. ⚠️ WHAT QA SHOULD SCRUTINISE (`qa/TASK-584.md`)

1. ⭐ **The write-race, first:** confirm TASK-579's three decision sites, its one string and its two latch writers are all present and unmodified (§4 lists them with post-edit line numbers), and that `WarMapWidget.h` is unmodified entirely.
2. **The diff size:** 5 changed lines across the `WarMapWidget` pair, 0 of them in the `.h`. ⛔ If you count more, that is the defect.
3. **`CombatantHealthBarComponent.cpp:110`** — verify it still reads the **instance** fields.
4. **The two `UPROPERTY`s** — verify they are still under `protected:` and their initialisers are byte-unchanged.
5. **The header's include block** — verify no `#include` was added, in particular **no `Siegebound/TeamId.h`**.
6. **The accessors' signatures** — `public`, `static`, zero parameters, ⛔ not `UFUNCTION`, one-line bodies, no null branch.
7. ⚠️ **`GetDefault<T>()`'s reachability in `CombatantHealthBarComponent.cpp`** is the one claim only the compiler settles; the include chain I read is in §2 and is falsifiable at `Actor.h:11` → `SubclassOf.h:6` → `Class.h:63`.
