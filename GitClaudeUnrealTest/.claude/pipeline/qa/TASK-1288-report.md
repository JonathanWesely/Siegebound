# QA Report — TASK-1288 (gate) · subject: TASK-1271 — Verdict: PASS — 0 BLOCKER · 1 WARN · 1 NIT

- subject: TASK-1271 [TOWER-ACTIVECLIMBER-REFLECT] · gate row: TASK-1288 [TOWER-ACTIVECLIMBER-GATE]
- reviewer: qa-reviewer · 2026-09-14
- inputs read whole (`SC-§38a`): `handoffs/TASK-1271-programmer.md` · TASKBOARD rows TASK-1271 (:3066–3075) + TASK-1288 (:3257–3265) · `ClimbableTower.h` :930–989 · `ClimbableTower.cpp` :288–306, :574–589, :672–698, :898–915 · `HeroCharacter.h` :296–320, :650–659, :1441 · `SiegeLadderClimbStatics.h` :99 · CONVENTIONS `CONTACT-§9` (:8756) / `§10.1` (:8762) / `§12.6` (:8894) / `SC-§106` (:4756) · SLACK.md :31
- engine source read (UE 5.8): `ObjectMacros.h` :447 / :892 / :1104 / :1170 · `UnrealType.h` :1175–1186, :3108–3123, :3160–3173, :3248–3292 · `PropertyWeakObjectPtr.cpp` :104–123 · `EpicGames.UHT/Types/Properties/UhtWeakObjectPtrProperty.cs` :100–164 · `MovementBaseInterface.h` :17–18 · `TimelineComponent.h` :257–258 · `ConstraintInstance.h` :1300–1301
- inspected through `unreal_inspector` (read-only, no mutation, no lifecycle): one Python read of the RUNNING (pre-compile, old-binary) `AClimbableTower` CDO — `ActiveClimber` and `ActiveClimberPathComp` are NOT reflected on the running binaries (the pre-state the row repairs). This is the only inspector measurement; everything else in this report is a text-level verdict.
- NOT measured (`SC-§71b`, no `Bash`): `git diff` hunk/line counts and `git status` porcelain — accepted as declared, cross-checked against the session-start git snapshot (below) and the file's post-state. The host TASK-1289 re-measures.

## Findings

- [WARN] `.claude/pipeline/qa/TASK-1288-report.md` (this gate, provenance) — the hunk count "1 hunk / +2 / −0" is accepted as declared, not measured: this gate holds no `Bash` and the inspector has no git lane. What IS measured: the post-state of `ClimbableTower.h` :957 and :961 carries the exact prescribed specifier line and nothing else in the file mentions `ActiveClimber` beyond comments and the two declarations (grep: :167, :889, :957–962, :975, :980); the session-start git snapshot lists `ClimbableTower.h` as the ONLY modified file in the tower/hero set (`ClimbableTower.cpp` and `HeroCharacter.h` unmodified). — suggested handling: build-master quotes `git diff --stat Source/GitClaudeUnrealTest/Siegebound/ClimbableTower.h` at TASK-1289 (expected `1 file changed, 2 insertions(+)`); any deletion or a third insertion is a stop.
- [NIT] `Source/GitClaudeUnrealTest/Siegebound/ClimbableTower.h:980` — the `LadderContacts` doc says *"⛔ NOT A UPROPERTY and ⛔ not reflected — the `ActiveClimber` precedent"*; after TASK-1271 the precedent is half-true (`ActiveClimber` is still weak / never authored / never serialized, but it IS now reflected, read-only). The handoff cites it as `:978`; in the post-insertion file the sentence starts at `:980` (`CONTACT-§10.1` line-cite rot — cosmetic, the handoff's line was pre-insertion). Correctly left untouched here (acceptance (1) forbids a third line). — suggested fix: one-word rewording ("the `ActiveClimber` precedent for authoring/serialization, ⛔ not for reflection") on TASK-1271's NEXT touch of this header if one comes, else a doc-hygiene row; it must not ride TASK-1289.

## The eight checks

**(1) Hunk = the two specifier lines and nothing else.** Post-state quoted, `ClimbableTower.h` :954–963:

```
 	 *  being refused, because it would have bricked this ladder for EVERY later climber, hero
 	 *  and unit alike, for the rest of the match. ⛔ Both halves ship together or neither does.
 	 */
+	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Siegebound|Tower|Debug")
 	TWeakObjectPtr<ACharacter> ActiveClimber;

 	/** The exact UPathFollowingComponent the link handed us for ActiveClimber — … leaves this null, deliberately. */
+	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Siegebound|Tower|Debug")
 	TWeakObjectPtr<UObject> ActiveClimberPathComp;
```

The `+` lines match the row's verbatim prescription character-for-character. The 940–956 comment block reads intact (the `ACharacter`-not-`APawn` argument, the ghost-pawn free property, the TASK-787 two-halves paragraph, closing `*/` at :956). No include, rename, accessor, default, or `.cpp` change is visible in the file; no accessor named for either member exists anywhere in the header. Declared counts: 1 hunk, +2, −0, 1 file — accepted as declared (WARN above). PASS.

**(2) Specifier set vs the row and UE 5.8.** `VisibleInstanceOnly` is a live `UP` enumerator (`ObjectMacros.h:1170`); `Transient` (`:892`/`:1104`) maps to `CPF_Transient = 0x2000` (`:447`, "shouldn't be saved or loaded"); `Category` is a legal metadata specifier on a non-Blueprint property. `TWeakObjectPtr<T>` inside a `UPROPERTY` is the `FWeakObjectProperty` type (`UnrealType.h:3252`, `class FWeakObjectProperty : public TFObjectPropertyBase<FWeakObjectPtr>`). Both members sit in the class's `private:` section (`:804`, class `UCLASS()` at :218, `GENERATED_BODY()` at :221): a `UPROPERTY` on a private member is legal without `meta=(AllowPrivateAccess)` because no `Blueprint*` specifier is used — `AllowPrivateAccess` only matters when Blueprint read/write is requested; reflection + detail-panel visibility need nothing more. `VisibleInstanceOnly` without any `Edit*` = read-only in the instance details panel, hidden on the CDO/archetype — nothing becomes authorable. PASS.

**(3) GC — weak stays weak.** `TFObjectPropertyBase<FWeakObjectPtr>::ContainsObjectReference` (`UnrealType.h:3116–3121`) answers a `Strong` query with `!TIsWeakPointerType<InTCppType>::Value` = false for `FWeakObjectPtr`, and `FWeakObjectProperty` (`:3252–3292`) does NOT override `EmitReferenceInfo` — only `FObjectProperty` does (`:3171`) — so the GC schema never emits a reference for it. Reflection makes the slot readable; it does not pin the climber. No lifetime change. PASS.

**(4) Exit paths still clear the slot.** Read at source: `EndPlay` `ClimbableTower.cpp:293–303` — aborts through `ILadderClimber::AbortLadderClimb()`, calls `ReleaseClimber`, then `ActiveClimber.Reset(); ActiveClimberPathComp.Reset();` unconditionally (:302–303). `ReleaseClimber` :678–697 — `bWasActive || !ActiveClimber.IsValid()` ⇒ both `.Reset()` (:693–696), including the defensive stale-handle arm (`TOWER-§8 (6)`). Both bind-before-set sites are untouched: `BeginClimb` :586–588 and `TryBeginContactClimb` :912–914 set the slot then `AddUniqueDynamic` the ended-delegate (`CONTACT-§12.6` order intact). Hero exits H-1..H-10 (`HeroCharacter.h:299–301`, enum :313+) all funnel through the hero's `OnLadderClimbEnded` broadcast → `HandleLadderClimbEnded` → `ReleaseClimber`. The `.cpp` is unmodified (git snapshot), so none of this moved. PASS.

**(5) No test — ACCEPTED.** Acceptance (3) reads "suite count unchanged, green", so a new automation test would fail the row by construction. The handoff is also right that a `FindFProperty<FObjectProperty>` assert would return null on a CORRECT build (the property is an `FWeakObjectProperty`, a sibling of `FObjectProperty` under `FObjectPropertyBase`, not a subclass); the correct one-liner, if a later row wants it, is `FindFProperty<FWeakObjectProperty>(AClimbableTower::StaticClass(), TEXT("ActiveClimber"))` non-null + `HasAnyPropertyFlags(CPF_Transient)`. The real instrument is acceptance (5): `get_actor_property_in_pie ActiveClimber` on a staged `BP_Building_WatchTower_C` at the next tower-staging verification (null is a value). No WARN.

**(6) `:978`/`:980` stale comment.** NIT above. Belongs to TASK-1271's next touch of `ClimbableTower.h` if the header is reopened, otherwise a doc row — not TASK-1289 (the host stages only what its line names, `SC-§102`).

**(7) `git status` — accepted as declared (`SC-§71b`), corroborated by the session-start snapshot:** `M ClimbableTower.h` (TASK-1271) · `M DeckBuilderWidget.cpp` + `M DeckBuilderWidget.h` (NOT-MINE — the concurrent TASK-1270 programmer's deck files; the handoff's porcelain predates them and correctly disclaims "the TASK-1270 sibling's files, if present, are theirs") · `M TASKBOARD.md` · `?? handoffs/TASK-1271-programmer.md`. No `ClimbableTower.cpp`, no `HeroCharacter.h`, no asset. TASK-1289 must stage `ClimbableTower.h` + the TASK-1271 handoff for this row and must not sweep the `DeckBuilderWidget.*` pair under TASK-1271's name (they belong to TASK-1270 / gate TASK-1287).

**(8) UHT accepts `TWeakObjectPtr<UObject>`.** `UhtWeakObjectPtrProperty.cs:135–146` resolves the `TWeakObjectPtr` keyword via `ParseTemplateObject(Normal)` and `CreateWeakProperty` (:121–129); the ONLY rejection is `classObj.IsChildOf(Session.UClass)` — "Class variables cannot be weak, they are always strong" — i.e. `TWeakObjectPtr<UClass>`; `UObject` is not a child of `UClass`, so it is accepted. Engine precedent, shipped in 5.8: `MovementBaseInterface.h:17–18` (`UPROPERTY() TWeakObjectPtr<UObject> PhysicsObjectOwner;`), `TimelineComponent.h:257–258`, `ConstraintInstance.h:1300–1301` (15 hits in `Engine/Classes` alone). PASS.

## Hero member decision (row acceptance, stated)

`AHeroCharacter::LadderClimb` (`HeroCharacter.h:1441`, type `FSiegeLadderClimbState`, `SiegeLadderClimbStatics.h:99` — a plain `struct`, no `USTRUCT`) — **left as is**, reason quoted from the handoff and confirmed at source: `HeroCharacter.h:655–656` records the struct is "deliberately UNREFLECTED, so ⛔ nothing in it can be replicated by accident even after a later refactor — a STRUCTURAL guarantee" (`CONTACT-§9`). A `UPROPERTY` on it is impossible without first reflecting the struct in a file outside TASK-1271's write set, and doing so would spend that guarantee for a debug read. Accepted; the gate does not ask for the struct to be reflected (scope, per the TASK-1288 row).

## Notes for build-master (TASK-1289)

1. Expected `git diff --stat` for this row: `Source/GitClaudeUnrealTest/Siegebound/ClimbableTower.h | 2 ++` — one file, two insertions, zero deletions. Anything else is a stop (this gate's only unmeasured claim).
2. The compile is a header change on a `UCLASS` — a full UBT build, never Live Coding (`VER-§2` cl. 2, the GUI editor closes).
3. Acceptance (4): after the relaunch, `AssetTools.is_dirty("/Game/Maps/L_Arena")` must read false after a load — `CPF_Transient` guarantees no serialization delta, but read it back.
4. Acceptance (5) is NOT owed by TASK-1289 unless it stages a tower; if it does, one line: `get_actor_property_in_pie ActiveClimber` on a `BP_Building_WatchTower_C` returns a value (null counts).
5. The `DeckBuilderWidget.cpp/.h` pair in the tree is TASK-1270's — stage it only under the name the TASK-1289 line carries after TASK-1287's verdict; never under TASK-1271.
