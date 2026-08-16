# TASK-591 handoff — [WR-37] the `ACommanderNpc` anim-owner repair (gameplay-programmer, 2026-08-16)

- **Gate:** `.claude/pipeline/qa/TASK-592.md` — ⚠️ **QA loop 2 of 3.**
- **Compile + the row (m) re-observation:** TASK-593. **Commit:** TASK-570. ⛔ **This task opened NO compile of its own.**
- **Status:** `ready-for-qa`.
- **Discipline honoured:** ⛔ no compile · no build · no Git · no editor · no MCP · no PIE · no `Content/` asset · no `.uasset` · no `BP_CommanderNpc` edit · no `.csv` · no `Tests/` file · no `Build.cs` · no third source file. ⛔ **No token figure is quoted, derived or reasoned from anywhere in this handoff.** ⛔ **No Zone builder and no prompt asset is touched — Zone A cannot have moved, and the greps in §7 are the mechanical form of that claim.**
- **Files modified — EXACTLY the two in `names:`, both pre-existing:**
  - `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Source\GitClaudeUnrealTest\Siegebound\CommanderNpc.h`
  - `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Source\GitClaudeUnrealTest\Siegebound\CommanderNpc.cpp`
- **Assets referenced by path only (none created, none opened, none required to exist):** `/Game/Characters/Anims/A_Sorcerer_Idle` (present on disk — see §5) · `/Game/Characters/SK_Sorcerer` · `/Game/Meshes/SM_WarTable`.

---

## 0. ⛔⛔ READ THIS FIRST — I MOVED THE LINE NUMBER IN QA CRITERION (2). THE CODE LINE IS BYTE-IDENTICAL.

`qa/TASK-592.md` criterion (2) says: *"`CommanderNpc.h:129` READS `class GITCLAUDEUNREALTEST_API ACommanderNpc : public AActor`, UNCHANGED."*

⛔ **The declaration is byte-identical and the base class is untouched. ⚠️ IT IS NO LONGER ON LINE 129 — IT IS ON LINE 141**, because the forward-declaration block gained one line (`class UAnimSequence;`) and the class doc gained the `SC-§35` item 6 refusal (§4c). **Locate by symbol, ⛔ never by offset (`SC-§18c`)**:

```
$ grep -n "class GITCLAUDEUNREALTEST_API ACommanderNpc" Source/GitClaudeUnrealTest/Siegebound/CommanderNpc.h
141:class GITCLAUDEUNREALTEST_API ACommanderNpc : public AActor
```

⚖️ **Declared under `SC-§15` rather than left for the gate to trip over**: adding a comment above a declaration is not "touching" it, but criterion (2) is written as a line quote, and a gate that greps `:129` finds the wrong text. **The base class stays `AActor`. The Pawn conversion was refused, and I did not quietly take it — I wrote the refusal INTO the class doc so the next author meets it before he meets the temptation.**

---

## 1. ⭐ THE FIX — THE BEFORE/AFTER ASSIGNMENT, PASTED

**BEFORE** (`CommanderNpc.cpp`, constructor + the file-scope constant it composed from):

```cpp
	/**
	 *  The ONE shared locomotion AnimBP, byte-identical to
	 *  ASummonedUnit's SharedLocomotionAbpPath (SummonedUnit.cpp). Deliberate
	 *  duplicate of a string, NOT a refactor: SummonedUnit.{h,cpp} is another
	 *  task's file this batch and promoting the constant would have touched it.
	 *  ⚠️ PAIRED with that constant — if the shared ABP is ever renamed, both
	 *  sites change together.
	 */
	const TCHAR* CommanderSharedLocomotionAbpPath = TEXT("/Game/Characters/ABP_Footman.ABP_Footman_C");

	...

	AvatarMeshAsset = TSoftObjectPtr<USkeletalMesh>(FSoftObjectPath(CommanderAvatarMeshPath));
	WarTableMeshAsset = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(CommanderWarTableMeshPath));
	AvatarAnimClassAsset = TSoftClassPtr<UAnimInstance>(FSoftObjectPath(CommanderSharedLocomotionAbpPath));
```

**AFTER** (the constant is deleted outright; the constructor's third assignment is the idle CLIP, and the ABP field is left empty with the emptiness explained at the site):

```cpp
	const TCHAR* CommanderAvatarIdleAnimPath = TEXT("/Game/Characters/Anims/A_Sorcerer_Idle.A_Sorcerer_Idle");

	...

	AvatarMeshAsset = TSoftObjectPtr<USkeletalMesh>(FSoftObjectPath(CommanderAvatarMeshPath));
	WarTableMeshAsset = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(CommanderWarTableMeshPath));
	AvatarIdleAnimAsset = TSoftObjectPtr<UAnimSequence>(FSoftObjectPath(CommanderAvatarIdleAnimPath));

	// ⛔⛔ AvatarAnimClassAsset IS DELIBERATELY LEFT EMPTY, AND ⛔ THE EMPTINESS IS
	// THE FIX (TASK-591 / SC-§35) — it is ⛔ NOT an oversight and ⛔ must not be
	// "completed". This comment exists precisely BECAUSE the three assignments
	// above make a missing fourth one look like a slip: the last AnimBlueprint
	// assigned here (ABP_Footman) cost 1,806 Blueprint runtime errors in 49 s,
	// because its graph resolves its owner as a Pawn and this actor is an AActor.
	// ⛔ Anything ever assigned here MUST be an ABP whose graph makes NO Pawn
	// assumption about its owner. See the field's header doc for both failure
	// cases; the commander's idle now comes from AvatarIdleAnimAsset, which runs
	// no graph at all.
```

⭐ **`AvatarAnimClassAsset` is left DEFAULT-CONSTRUCTED (null), not explicitly cleared.** `BP_CommanderNpc` carries zero overrides, so it holds no delta record for this property and inherits the new empty CDO default — no Blueprint edit is needed, and none was made. **The comment is load-bearing:** a constructor with three soft-ref assignments and a silently-absent fourth is exactly the shape someone "completes" six months from now.

---

## 2. ⛔ THE THREE-RUNG LADDER — WHERE IT LIVES, AND WHY IT MOVED OUT OF `ResolveAvatarMesh`

`SetAnimInstanceClass` used to live at the bottom of `ResolveAvatarMesh`, which is called from **both** `OnConstruction` and `BeginPlay`. Requirement **(d)** forbids playback from the construction script, so **the whole ladder was extracted into a new private `ApplyAvatarAnimation()` with exactly one call site, in `BeginPlay`, after both mesh resolves.**

```
$ grep -rn "ResolveAvatarMesh\|ApplyAvatarAnimation" Source/
CommanderNpc.cpp:177:	ResolveAvatarMesh(/*bWarnIfMissing=*/ false);      <- OnConstruction  (mesh only)
CommanderNpc.cpp:189:	ResolveAvatarMesh(/*bWarnIfMissing=*/ true);       <- BeginPlay
CommanderNpc.cpp:199:	ApplyAvatarAnimation();                            <- BeginPlay  (THE ONLY ANIM CALL SITE)
CommanderNpc.cpp:257:void ACommanderNpc::ResolveAvatarMesh(bool bWarnIfMissing)
CommanderNpc.cpp:289:void ACommanderNpc::ApplyAvatarAnimation()
CommanderNpc.h:421:	void ResolveAvatarMesh(bool bWarnIfMissing);
CommanderNpc.h:433:	void ApplyAvatarAnimation();
```

⭐ **The single call site is the begun-play gate AND the once-ness guarantee at the same time** — it is why the fall-through log needs no `bWarnedX` bool (§8, declared). The rungs:

| rung | condition | action | log |
|---|---|---|---|
| **1** | `AvatarAnimClassAsset` resolves | `SetAnimInstanceClass` | none — **dormant, the field ships EMPTY** |
| **2** | mesh set **and** idle soft-ref non-null **and** sequence resolves **and** skeletons identical | `SetAnimationMode(AnimationSingleNode)` + `PlayAnimation(Seq, true)` | none |
| **3** | any rung-2 condition fails | **nothing — ref pose** | ⚠️ **one** `Log` line, **only** for *unresolved sequence* or *skeleton mismatch* |

⛔ **Two rung-3 paths are deliberately SILENT:** a missing avatar mesh (`ResolveAvatarMesh` already reported it once at `Warning` — a second line is the same fact twice) and a **cleared** `AvatarIdleAnimAsset` (a deliberate designer opt-out, the `AttackImpactEffect` `IsNull` pattern). **No `Warning` is added on any expected path; no log is reachable per-frame.**

---

## 3. ⛔⛔ A REAL FINDING QA SHOULD SCRUTINISE — **UE 5.8 DOES NOT CHECK THE SKELETON FOR US.** I DID NOT ASSUME THIS; I READ IT.

The spec required a mismatch to degrade to ref pose and never crash. I went to the engine to see who was going to enforce that, and **the answer is nobody**:

`Engine/Source/Runtime/Engine/Private/Animation/AnimSingleNodeInstance.cpp`, `UAnimSingleNodeInstance::SetAnimationAsset` — the only asset-nulling guard is:

```cpp
		else if (CurrentAsset != nullptr)
		{
			// if we have an asset, make sure their skeleton is valid, otherwise, null it
			if (CurrentAsset->GetSkeleton() == nullptr)
			{
				// clear asset since we do not have matching skeleton
				CurrentAsset = nullptr;
			}
		}
```

⛔ **It nulls the asset only when the sequence has NO skeleton AT ALL. It does NOT compare the sequence's skeleton against the component's** — the code comment says *"matching skeleton"* while the code tests *"non-null skeleton"*. ⇒ **an incompatible clip is handed to the pose evaluator, not refused.** So `ApplyAvatarAnimation` makes the comparison **itself**, before `PlayAnimation` is ever called:

```cpp
	const USkeleton* const MeshSkeleton = AvatarSkeletalMesh->GetSkeleton();
	if (!MeshSkeleton || IdleSequence->GetSkeleton() != MeshSkeleton)
	{
		UE_LOG(LogGitClaudeUnrealTest, Log, TEXT("... — REF POSE (cosmetic only; ...)"), ...);
		return;
	}
```

⚠️ **Deliberately an EXACT identity test, and deliberately NOT `USkeleton::IsCompatibleForEditor`** — that API sits inside a `#if WITH_EDITOR` block in `Skeleton.h` and is unavailable at runtime. **Identity is the conservative direction**: a merely *compatible* skeleton is refused and falls to ref pose, which is the safe way to be wrong. The fleet is retargeted onto the one `SK_Footman_Skeleton`, so identity is the expected match.

⚠️ **This is the finding most worth a second pair of eyes**, because if I have misread the engine the cost is asymmetric: over-strictness costs a cosmetic idle, under-strictness costs an unguarded pose evaluation.

---

## 4. EVERY OTHER EDIT, ENUMERATED

**(a) `CommanderNpc.h` — `AvatarAnimClassAsset`'s doc, REWRITTEN to `SC-§35` item 1** (QA criterion (5); this is the deliverable, not decoration). It now states, in this order: that an AnimBlueprint is a **contract with its owner's class**; the contract this field demands (*the graph must make no Pawn assumption*); **both** failure cases — **(1)** missing/cleared/unresolvable ⇒ ref pose, harmless, *"the case the previous version reasoned about carefully, and its reasoning was correct"*; **(2)** resolves-and-assumes-a-Pawn ⇒ a graph failing every frame per instance, *"the case the previous version was SILENT about, and the silence is what shipped"* — with the measured 1,806-in-49 s figure and the fact that it cleared a clean compile, 111/111 and two gates; that `ABP_Footman` is **not a legal value for this field** and that its use by `ASummonedUnit` is correct; and the precedence ladder.

**(b) `CommanderNpc.h` — the `AvatarMesh` component doc, CORRECTED.** It claimed the shared ABP *"gives it an idle for free"*. It did not. The retired sentence is quoted in place so the correction is recognisable.

**(c) `CommanderNpc.h` — the class doc's *"WHY AActor AND NOT APawn/ACharacter"* paragraph, EXTENDED with `SC-§35` item 6.** ⚖️ **Reason this is worth the added lines:** the paragraph previously carried only the **navmesh** reason, and a future author whose case does not involve the navmesh reads that as permission. It now also names the pawn-shaped-query cost (`GetPawnIterator`, AI perception, targeting ⇒ *the commander becomes acquirable as a combat target*) and points at the `ITeamAgent` measurement below it as the same failure through a second door. ⭐ **This is the change that shifts line 129 → 141 (§0).**

**(d) Includes:** `Animation/AnimSequence.h` (required — `TSoftObjectPtr<UAnimSequence>::LoadSynchronous` and the `UAnimationAsset*` upcast into `PlayAnimation`) and `Animation/Skeleton.h` (required — `GetNameSafe(const UObject*)` on a `USkeleton*` needs the type complete for the upcast). Both inserted in the block's existing alphabetical order.

**(e) Doc-comment touch-ups** on `BeginPlay` and `ResolveAvatarMesh`, plus a ⛔ *do-not-re-add-an-anim-call-here* note where the `SetAnimInstanceClass` block used to sit.

**(f) `AvatarIdleAnimAsset`** — new `UPROPERTY(EditAnywhere, Category = "Siegebound|CommanderNpc") TSoftObjectPtr<UAnimSequence>`, same category as its siblings, defaulting to `/Game/Characters/Anims/A_Sorcerer_Idle.A_Sorcerer_Idle`.

⛔ **Not touched, verified:** the base-class declaration · `/Game/Characters/ABP_Footman` (not opened) · `SummonedUnit.{h,cpp}` · `BP_CommanderNpc` · `Castle.{h,cpp}` · `Tests/` · any `Content/` asset. **`InteractRadius` 400, `EnemyRevealCost` 30, `CommanderAvatarYawOffset` −90, `CommanderWarTableForwardOffset` 200 and both `NoCollision` profiles are UNCHANGED** — `WR-§9` rows 8 and 13 stand exactly as written, and `GetCommanderTeam()` / `InitCommanderNpc(ETeamId)` / the non-implementation of `ITeamAgent` are all preserved (TASK-559's design facts).

---

## 5. ⚠️ THE SKELETON HYPOTHESIS — WHAT I CAN AND CANNOT SAY, AND WHAT TASK-593 SHOULD EXPECT

**What I verified:** `A_Sorcerer_Idle.uasset` **exists on disk** at `Content/Characters/Anims/`, alongside `A_Sorcerer_Attack/Death/Walk` and the same quartet for every other fleet unit. The soft path in the code therefore resolves to a real asset.

**What I did NOT verify, and ⛔ do not assert:** that it is built on `SK_Sorcerer`'s skeleton. **Opening the asset needs the editor, and the scope fence forbids it.** It remains a hypothesis (`SC-§20`).

⭐ **One piece of evidence that STRENGTHENS the manager's circumstantial case — offered as evidence, ⛔ not as a measurement, and ⛔ it does not close the question:** `ASummonedUnit::CacheSkeletalClips` composes `/Game/Characters/Anims/A_<CardID>_Attack` and `_Death` and plays them through the **same** `USkeletalMeshComponent::PlayAnimation` single-node API onto the matching `SK_<CardID>` body (`SummonedUnit.cpp:495-570`). If that path works for the Sorcerer today, `A_Sorcerer_*` and `SK_Sorcerer` already agree on a skeleton. ⚠️ **But nobody has confirmed that the Sorcerer's clip path is exercised in play, so this is one more strand of circumstance, not the measurement.** **TASK-593 row (3) is the measurement.**

---

## 6. ⛔⛔ WHAT TASK-593 SHOULD EXPECT TO SEE IN THE MESSAGE LOG

**The headline, row (m) — `Blueprint Runtime Error` count must be EXACTLY ZERO.**

⚖️ **The mechanism, stated so the expectation is falsifiable rather than hopeful:** all 1,806 errors came from **one** Blueprint graph — `ABP_Footman`'s EventGraph — evaluating on two `ACommanderNpc` instances. That graph now has **no way to be instantiated on this actor**: the only field that could assign it ships empty, the constant that composed it is deleted, and the replacement path (`PlayAnimation` single-node) creates a `UAnimSingleNodeInstance`, **which has no graph and cannot call `TryGetPawnOwner()`**. ⇒ **Zero is the predicted count, and the prediction fails loudly if wrong.**

⚠️ **`ASummonedUnit` still uses `ABP_Footman` and MUST keep working** — that class is an `ACharacter`, its use is correct, and nothing in this diff touches it. ⛔ **If unit locomotion breaks, that is a finding about THIS diff having over-reached, not an expected consequence.**

**Row (3), the cosmetic observation — BOTH ANSWERS SHIP, and here is how to tell them apart:**

| what you see | what it means | is it a problem? |
|---|---|---|
| the commander **breathes / idles** | `A_Sorcerer_Idle` is on `SK_Sorcerer`'s skeleton — the hypothesis held | ✅ no |
| the commander stands **stock still (ref pose)** + a `Log` line naming a **skeleton mismatch** | the hypothesis was wrong, and the guard did its job | ✅ **no — cosmetic only, ⛔ does NOT block the commit** |
| the commander stands **stock still (ref pose)** + a `Log` line saying the sequence **did not resolve** | the path is wrong or the asset is not cooked into the session | ⚠️ **a FINDING worth a line in the handoff, still ⛔ NOT commit-blocking** |
| ref pose with **no `Log` line at all** | ⛔ **unexpected** — `AvatarIdleAnimAsset` was cleared on the instance, or `ApplyAvatarAnimation` never ran | ⚠️ **report it** |

**Both log lines are at `Log`, both begin `[<ActorName>] CommanderNpc: idle sequence`, and both end `— REF POSE (cosmetic only; the war-map gate and EnemyRevealCost are unaffected).`** ⇒ **grep `CommanderNpc: idle sequence` — at most one line per spawned commander, and none at all in the success case.** ⛔ **Neither line is a `Warning`. If a `Warning` appears from this class it is the pre-existing missing-mesh warning, not this task's.**

📌 **Unchanged and still expected:** the two `CommanderNpcInit team=…` lines at `Log` (one per castle) — this diff does not touch `InitCommanderNpc`.

---

## 7. THE MANDATED GREPS — COMMANDS AND RAW COUNTS, PASTED

**(a) The deleted constant — repo-wide, expect ZERO:**
```
$ grep -rn "CommanderSharedLocomotionAbpPath" Source/
RAW HIT COUNT: 0
```

**(b) `ABP_Footman` in the two owned files — 4 hits, and the mechanical proof that ALL FOUR are comment prose:**
```
$ grep -n "ABP_Footman" CommanderNpc.h CommanderNpc.cpp
CommanderNpc.h:354   *        assigning /Game/Characters/ABP_Footman here cost 1,806 Blueprint
CommanderNpc.h:360   *  ⛔ /Game/Characters/ABP_Footman IS NOT A LEGAL VALUE FOR THIS FIELD. It is
CommanderNpc.cpp:31  *  SC-§35). The retired line assigned /Game/Characters/ABP_Footman — the
CommanderNpc.cpp:162 // assigned here (ABP_Footman) cost 1,806 Blueprint runtime errors in 49 s,
RAW HIT COUNT: 4

$ grep -vE "^\s*(\*|//|/\*)" CommanderNpc.h CommanderNpc.cpp | grep -nE "ABP_Footman|SharedLocomotionAbpPath"
(no output — exit 1)
```
⇒ ⛔ **ZERO code lines, ZERO symbols, ZERO soft paths.** ⭐ **The four survivors are deliberate — they are the record of WHY the field is empty. Deleting them would delete the reason and re-open the defect to the next author.**

**(c) `FT-§12b` PAIR RULE, re-checked at the artifact in BOTH directions (⛔ not taken on the manager's word):**
```
$ sed -n '54,60p' SummonedUnit.cpp
	//~ TASK-159/165: M7 shared-locomotion fallback AnimBP. ...
	const TCHAR* SharedLocomotionAbpPath = TEXT("/Game/Characters/ABP_Footman.ABP_Footman_C");   <- UNTOUCHED

$ grep -n "class GITCLAUDEUNREALTEST_API ASummonedUnit" SummonedUnit.h
118:class GITCLAUDEUNREALTEST_API ASummonedUnit : public ACharacter, ...                          <- still a Pawn ⇒ correct use

$ grep -rn "Commander" SummonedUnit.h SummonedUnit.cpp
RAW HIT COUNT: 0
```
✅ **Confirmed: `SummonedUnit` names no commander, so removing this file's half of the *"PAIRED constant"* note leaves ZERO stale cross-reference in either direction. ⛔ No stale reference found ⇒ nothing to report under the spec's stop-and-report clause.**

**(d) THE 111-TEST NON-INVALIDATION GREP — result pasted, ⛔ not an impression:**
```
$ grep -rn "CommanderNpc\|AvatarAnimClassAsset\|AvatarIdleAnimAsset\|AvatarMeshAsset\|CommanderTeam\
|SetAnimInstanceClass\|PlayAnimation\|SetAnimationMode\|AnimInstance\|AnimSequence" \
      Source/GitClaudeUnrealTest/Siegebound/Tests/
RAW HIT COUNT: 8 — all 8 in SiegeWarMapTest.cpp, and NOT ONE reads anim state:
  :11    #include "Siegebound/CommanderNpc.h"
  :24 :79 :1565 :1577 :1581   comment prose
  :1583  const ACommanderNpc* const CommanderDefaults = GetDefault<ACommanderNpc>();
  :1585  TestNotNull(... CommanderDefaults)
```
**The one test that touches this class (`Siegebound.WarMap.EnemyRevealCostIsThirtyOnTheCommanderCdo`) asserts `GetEnemyRevealCost()` and `GetInteractRadius()` on the CDO. ⛔ Neither accessor, neither number, and no anim field is altered by this diff.** ⇒ ✅ **NON-INVALIDATED. Expect 111/111.**

**(e) Scope fence, by timestamp (the tree carries the whole uncommitted batch, so `git status` cannot show this task's scope):**
```
$ find Source Content Tools -type f -newermt '-20 minutes'
Source/GitClaudeUnrealTest/Siegebound/CommanderNpc.cpp
Source/GitClaudeUnrealTest/Siegebound/CommanderNpc.h
```
⇒ **EXACTLY TWO FILES.** ⚠️ **`BP_CommanderNpc.uasset`, `BP_Torch.uasset` and `WBP_WarMap.uasset` also show as modified in the working tree — their mtimes are 23:14 and 23:32, i.e. TASK-589's authoring pass, BEFORE this task's first edit at 00:14. ⛔ I did not touch them and I added nothing to the git index.**

---

## 8. `SC-§33`, M8, AND THE DECLARED DEPARTURES

**`SC-§33` (the trailing-default law) — THE ANSWER IS ZERO, AND IT IS STRUCTURAL:**
```
$ grep -nE "^\s*(void|bool|float|int32|static|virtual|ETeamId|ACommanderNpc\*)\s.*\(.*\)" CommanderNpc.h
165: void InitCommanderNpc(ETeamId InTeam);              196: float GetInteractRadius() const
176: ETeamId GetCommanderTeam() const                    200: int32 GetEnemyRevealCost() const
192: bool IsPlayerInRange(const FVector&) const          226: static ACommanderNpc* FindCommanderNpcForTeam(UWorld*, ETeamId)
229: virtual void OnConstruction(const FTransform&)      421: void ResolveAvatarMesh(bool bWarnIfMissing)
234: virtual void BeginPlay()                            433: void ApplyAvatarAnimation()      <- NEW, ZERO PARAMETERS
                                                         436: void ResolveWarTableMesh(bool bWarnIfMissing)
```
⛔ **NO defaulted parameter exists anywhere in this class, before or after.** **The one new function takes ZERO parameters, so `SC-§33` cannot fire structurally** — chosen over the alternative of threading a second `bool` into `ResolveAvatarMesh`, precisely because a parameterless function cannot grow a default later. ⛔ **`ResolveAvatarMesh`'s signature is byte-identical and both of its call sites are unchanged.**
📌 *For completeness: the engine's `SetAnimationMode(EAnimationMode::Type, bool bForceInitAnimScriptInstance = true)` has a trailing default and I call it with one argument. It is an ENGINE signature this task did not author or modify, so `SC-§33` does not reach it — named here so the gate's grep does not have to wonder.*

**M8 — DECLARED:** ⛔ no replicated property · ⛔ no new replicated class · ⛔ no new relevancy tier · ⛔ no RPC. `AvatarIdleAnimAsset` is an `EditAnywhere` **class default**, identical on both machines because it is not state. **`WR-§8`'s Tier C declaration for `ACommanderNpc` stands untouched** in both the class doc and the constructor comment.

**DECLARED DEPARTURES / ADDITIONS OVER THE SPEC (`SC-§15`) — four, none silent:**
1. ⚠️ **The base-class declaration moved from line 129 to line 141** (byte-identical text). §0 — this is the one that interacts with a gate criterion.
2. ⭐ **The class doc gained the `SC-§35` item 6 refusal** (§4c). An addition over the spec's edit list, in an owned file, mandated by cited law. **Reason: recording only the navmesh objection leaves the next author a gap to walk through.**
3. ⭐ **The anim ladder was EXTRACTED into `ApplyAvatarAnimation()` rather than gated inside `ResolveAvatarMesh`.** The spec left the seam to me ("*the seam is yours; the requirement is not*"). **A single call site is checkable in one grep; a runtime `HasActorBegunPlay()` guard would depend on engine ordering the reviewer cannot see.**
4. ⚠️ **No `bWarnedX` one-shot bool was added for the rung-3 log**, diverging from this file's local `bWarnedMissingAvatarMesh` / `bWarnedMissingWarTableMesh` pattern. **Reason: those guard a function reachable from `OnConstruction`, which re-runs on every property tweak. `ApplyAvatarAnimation` has one call site, in `BeginPlay`, so it is once-per-spawn structurally — a guard bool would be dead state that implies a per-frame path exists.** ⇒ ⛔ **If QA prefers the local pattern for consistency, say so and I will add it; I am declaring the choice rather than letting it read as an omission.**

---

## 9. ⛔ WHAT THIS TASK CANNOT CLAIM

⛔ **I have not observed anything.** No compile, no PIE, no editor. **`SC-§35` item 5 is explicit that this defect class is invisible to the compiler, to the suite and to code review** — and it is exactly why 1,806 errors survived a clean build, 111/111 and two gates. ⇒ ⭐ **A PASS at TASK-592 gates the SHAPE of this repair. Only TASK-593's PIE Message-Log read can close the defect, and its `Blueprint Runtime Error` count is the whole verdict.**
