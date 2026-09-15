# TASK-1271 — [TOWER-ACTIVECLIMBER-REFLECT] — programmer handoff

**Status:** ready-for-qa — 2026-09-14 (gameplay-programmer)
**Row:** `TASKBOARD.md` marker `TASK-1271-TOWER-ACTIVECLIMBER-REFLECT`
**Law:** `CONTACT-§12` · `VER-§5` cl. 1 · `VER-§2` cl. 2 · `SC-§104`

## 1. What changed — the hunk (1 hunk, 2 insertions, 0 deletions, 1 file)

`git diff Source/GitClaudeUnrealTest/Siegebound/ClimbableTower.h`:

```diff
@@ -954,9 +954,11 @@ private:
 	 *  being refused, because it would have bricked this ladder for EVERY later climber, hero
 	 *  and unit alike, for the rest of the match. ⛔ Both halves ship together or neither does.
 	 */
+	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Siegebound|Tower|Debug")
 	TWeakObjectPtr<ACharacter> ActiveClimber;
 
 	/** The exact UPathFollowingComponent the link handed us for ActiveClimber — the one holding CurrentCustomLinkOb, so the handshake is closed against the right object even if the unit changed controller mid-climb. ⚠️ A CONTACT-started climb has no such handshake and leaves this null, deliberately. */
+	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Siegebound|Tower|Debug")
 	TWeakObjectPtr<UObject> ActiveClimberPathComp;
```

- Specifier list is the row's verbatim: `VisibleInstanceOnly, Transient, Category = "Siegebound|Tower|Debug"`.
- The 940–956 comment block above `ActiveClimber` is untouched (verbatim). No include added, no rename, no accessor, no default, no `.cpp` line touched (`ClimbableTower.cpp` diff = 0).
- Line endings preserved (file is LF in the working copy; the `git diff` CRLF warning is the repo's autocrlf notice and pre-dates this change — insertions only, no line rewritten).
- UE 5.8 names verified at source: `CPF_Transient = 0x2000` (`ObjectMacros.h:447`), `VisibleInstanceOnly` (`ObjectMacros.h:1170`), `Transient` (`:892`/`:1104`); `TWeakObjectPtr<T>` reflects as `FWeakObjectProperty` (`UnrealType.h:3252`), a legal `UPROPERTY` type. A `UPROPERTY` on a `private:` member is legal without Blueprint specifiers (none used here, so no `AllowPrivateAccess` needed).

## 2. GC / ownership analysis — NOT a hard reference

The dispatch asked whether a raw pointer becoming a `UPROPERTY` turns into a GC-tracked hard reference. **It does not apply here: both members were already `TWeakObjectPtr` (`:957`, `:960`), and a `TWeakObjectPtr` `UPROPERTY` is an `FWeakObjectProperty` — reflection makes it readable, it does NOT make it strong.** The reference collector skips weak-object properties; a destroyed climber still nulls the handle on the next `Get()`/`IsValid()` exactly as before. No lifetime, GC, or destruction-order semantics change. `TWeakObjectPtr` is also what the code semantically wants (the header at `:973` and `:950` in the `.cpp` state "weak on purpose"), so the row's prescription is the right type and nothing is converted.

### Exit paths — the slot is cleared on every one (unchanged, documented for QA)

| Path | Site | Clears `ActiveClimber` / `ActiveClimberPathComp` |
|---|---|---|
| Tower dies under the climber (`EndPlay`) | `ClimbableTower.cpp:293–303` | aborts via `ILadderClimber::AbortLadderClimb()` then `ReleaseClimber`, then both `.Reset()` unconditionally |
| Climber's own climb end (delegate `HandleLadderClimbEnded` → `ReleaseClimber`) | `ClimbableTower.cpp:678–697` | `bWasActive` ⇒ both `.Reset()` |
| Climber destroyed without any completion path (stale handle) | `ClimbableTower.cpp:693` | `!ActiveClimber.IsValid()` ⇒ defensive clear (`TOWER-§8 (6)`) |
| Hero exits H-1..H-10 (`HeroCharacter.h:299–318`) | all funnel through the hero's `OnLadderClimbEnded` broadcast (`CONTACT-§12`), which the tower binds in `BeginClimb`/`TryBeginContactClimb` before setting the slot (`:578–587`, `:902–913`) | via `ReleaseClimber` above |

Because the handle is weak, even an exit that somehow skipped the delegate could not pin the climber alive; the `IsValid()` arm at `:693` frees the ladder on the next release.

## 3. Serialization / package-dirtiness

`Transient` ⇒ `CPF_Transient`: never saved or loaded, so the `BP_Building_WatchTower` CDO, `L_Arena`, and every placed instance stay byte-identical on disk. Expected at the host: `AssetTools.is_dirty("/Game/Maps/L_Arena")` false after a load (acceptance (4), build-master reads it back). `VisibleInstanceOnly` (no `Edit*`) ⇒ `CPF_EditConst`-style read-only detail panel row on instances, hidden on the CDO/archetype — nothing becomes authorable.

## 4. Optional (2) — the hero's climb-state member: LEFT AS IS

The report's "likewise unreflected" member is `AHeroCharacter::LadderClimb` (`FSiegeLadderClimbState`, `HeroCharacter.h:1441`). **Left as is, reason:** `FSiegeLadderClimbState` is a plain C++ struct with no `USTRUCT` (`SiegeLadderClimbStatics.h:99`), so it cannot carry a `UPROPERTY` at all without first reflecting the struct in a file outside this row's write set — and `HeroCharacter.h:655–656` records that it is "deliberately UNREFLECTED, so nothing in it can be replicated by accident even after a later refactor — a STRUCTURAL guarantee" (`CONTACT-§9`). Reflecting it would spend that guarantee for a debug read; the hero's climb is already observable through the reflected proxies the pilot used (`CharacterMovement.MovementMode`, capsule Z) and, after this row, through the tower's slot itself. If the team wants a hero-side read later, the shape is a reflected `bool` mirror or a `BlueprintPure` accessor — a separate row, not a silent widening of this one.

## 5. Test — none added, by the row's own acceptance

Acceptance (3) requires "suite count unchanged, green", so a new automation test would fail the row. A `FindFProperty<FObjectProperty>` reflection assert (`SC-§104` shape) is also not the right instrument here: the property is an `FWeakObjectProperty`, not an `FObjectProperty`, so the assert as written in the dispatch would return null on a correct build. Acceptance (5) names the real runtime instrument — `get_actor_property_in_pie ActiveClimber` on a staged `BP_Building_WatchTower_C` — recorded by the next verification that stages a tower. If QA nevertheless wants a compile-time proof, the one-liner is `FindFProperty<FWeakObjectProperty>(AClimbableTower::StaticClass(), TEXT("ActiveClimber"))` non-null with `HasAnyPropertyFlags(CPF_Transient)` true — and it would need a board rescope (`SC-§100`) because of acceptance (3).

## 6. Something QA should look at (not in the diff, flagged rather than touched)

`ClimbableTower.h:978` (the `LadderContacts` comment) says *"⛔ NOT A UPROPERTY and ⛔ not reflected — the `ActiveClimber` precedent"*. After this row the precedent is half-true: `ActiveClimber` is still weak, never authored, never serialized, but it is now reflected. The row's acceptance (1) ("diff = the two `UPROPERTY` lines and nothing else") forbids editing that sentence here; it is a one-word doc drift for a later doc row (`CONTACT-§10.1` cite hygiene), not a code defect. `LadderContacts` itself should stay unreflected (a `TArray` of a non-`USTRUCT` struct cannot be a `UPROPERTY` anyway).

## 7. Files touched

- `Source/GitClaudeUnrealTest/Siegebound/ClimbableTower.h` — +2 lines (`:957`, `:961` after insertion)
- `.claude/pipeline/handoffs/TASK-1271-programmer.md` — this note (new)
- `.claude/pipeline/TASKBOARD.md` — the TASK-1271 `status:` line only

No compile, no Live Coding, no editor lifecycle (GUI editor PID 11576 left up, untouched), nothing staged, no Git, no asset, no `KBD-§`/`IMC_*`, no deck/controller/HUD file (the TASK-1270 lane).

`git status --porcelain` at handoff (the TASK-1270 sibling's files, if present, are theirs):

```
 M GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/ClimbableTower.h
```

(plus `TASKBOARD.md` and this handoff once written — see the board flip.)
