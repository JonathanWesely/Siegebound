# QA Report — TASK-592 — [WR-38] THE DEFECT RE-GATE (diff-scoped to TASK-591)

**Verdict: PASS — ⛔ AND THE PASS DOES *NOT* MEAN THE DEFECT IS FIXED.**
**0 BLOCKER · 3 WARN · 4 NIT**
**Date:** 2026-08-16 · **QA loop 2 of 3** (a FAIL would have left exactly one; none was warranted)
**Scope:** `CommanderNpc.h` + `CommanderNpc.cpp`, and nothing else.

> ⛔⛔ **THE VERDICT LINE, IN THE TERMS CRITERION (10) DEMANDS.** `SC-§35` item 5: this defect class is invisible to the compiler, to `warnings_as_errors`, to the 111-test suite **and to code review** — it needs a live anim update on a spawned actor. **I am gating the SHAPE of the repair. ⛔ I have observed NOTHING about runtime and I certify NOTHING about runtime.** The original defect cleared a clean compile, 111/111 and two review gates; a third clean review is therefore **not** evidence of a fix. ⭐ **Only TASK-593's PIE Message-Log read can close it, and its `Blueprint Runtime Error` count is the whole verdict.**

⛔ **`qa/TASK-565.md` and `qa/TASK-584.md` were NOT reopened.** The eleven ratified `SC-§15` departures, the `WR-§2`/`WR-§2b` ledger and `W4-R3` were not examined and are not commented on. This gate ruled on **one diff**.
⛔ **Method:** read-only file access + the installed UE 5.8 source at `C:\Program Files\Epic Games\UE_5.8\Engine\Source`. **No compile · no editor · no MCP · no PIE · no Git · no code edited.** 🔒 Holdout unspent, nothing model-side, ⛔ **no token figure quoted or derived anywhere in this report** (`AS-§12g`).

---

## 0. THE ONE-PARAGRAPH RESULT

The repair is **structurally correct and it is the right shape**. Rung (a) — the load-bearing half — is real at the artifact: the constant is gone repo-wide (**my own count: 0**), `AvatarAnimClassAsset` is default-constructed and never assigned, and the four surviving `ABP_Footman` strings are **all comment prose** (I read each of the four in place). The base-class declaration is byte-identical and I found it **by symbol at `:141`**, exactly where the implementer said it moved to — the flagged line-number drift is real and was handled correctly. ⭐ **The finding I was told to scrutinise is TRUE and I confirmed it character-for-character in the engine, not from the handoff:** `AnimSingleNodeInstance.cpp:52-59` nulls the asset **only** when `CurrentAsset->GetSkeleton() == nullptr`, while its own comment says *"clear asset since we do not have matching skeleton"* — the code tests non-null, the comment claims matching. **There is no engine-side skeleton-identity enforcer, so the spec's "must degrade to ref pose" genuinely had no owner and the explicit check earns its place.** The ladder is null-safe at every rung, the log is structurally once-per-spawn, and nothing starts playback from `OnConstruction` — a requirement I can now report is **engine-backed rather than stylistic** (§4). `SC-§33` is **ZERO by my own enumeration**. The three WARNs are a comment that is *incomplete* rather than wrong, an escape hatch that is documentation-guarded because nothing else is possible, and the standing fact that this gate observes nothing.

---

## 1. CRITERION BY CRITERION

| # | criterion | verdict | what I read |
|---|---|---|---|
| **(1)** | the diff is **exactly 2 files**; a third path is an automatic BLOCKER | ✅ **PASS** (structural — I have no Git) | A modification-time-ordered `Glob` over the whole batch's touched set returns `CommanderNpc.cpp` and `CommanderNpc.h` **last, as the two newest files** — i.e. every other batch file's last write precedes this task's first edit. Combined with §2's greps (which show `SummonedUnit.{h,cpp}` and `Tests/` carrying no change of substance) this is the strongest form available to a tool-less gate. ⛔ **The byte-level reconciliation remains build-master's at TASK-570** (`SC-§29b`) |
| **(2)** | ⛔⛔ `class GITCLAUDEUNREALTEST_API ACommanderNpc : public AActor`, **UNCHANGED** | ✅ **PASS** | **Located BY SYMBOL per `SC-§18c`, ⛔ never by offset.** `CommanderNpc.h:141` reads `class GITCLAUDEUNREALTEST_API ACommanderNpc : public AActor` — **byte-identical**. ⭐ **The implementer's §0 warning was accurate and it saved a phantom-citation cycle:** the drift `:129 → :141` is exactly +12, and it reconciles — **+1** from `class UAnimSequence;` (h:12) and **+11** from the `SC-§35` item 6 refusal paragraph (h:68-77). ⛔ **No Pawn conversion was quietly taken.** ✅ And the refusal is now *in the class doc*, where the next author meets it before the temptation |
| **(3)** | ⛔ ZERO surviving `ABP_Footman` refs; ✅ converse: `SummonedUnit.cpp:60` untouched + still `ACharacter` | ✅ **PASS** | **My own count: 4 hits in the two owned files — `CommanderNpc.h:354`, `:360`, `CommanderNpc.cpp:31`, `:162` — and I read all four IN PLACE, not by filter: `.h:354`/`.h:360` sit inside the `/** … */` field doc opened at `:331`; `.cpp:31` inside the `/** … */` opened at `:26`; `.cpp:162` is a `//` line inside the constructor's comment block `:158-167`.** ⇒ ⛔ **zero code lines, zero symbols, zero soft paths.** `CommanderSharedLocomotionAbpPath` across `Source/` → **0 (my count).** **Converse CONFIRMED at the artifact:** `SummonedUnit.cpp:60` still `const TCHAR* SharedLocomotionAbpPath = TEXT("/Game/Characters/ABP_Footman.ABP_Footman_C");` and `SummonedUnit.h:118` still `class GITCLAUDEUNREALTEST_API ASummonedUnit : public ACharacter, public ITeamAgent, public IHealthBarProvider` ⇒ **a Pawn ⇒ its use is correct and it is undisturbed** |
| **(4)** | ladder null-safe at every rung · ⛔ no per-frame log · ⛔ no new `Warning` on the expected path · ⛔ no playback from `OnConstruction` | ✅ **PASS — see §4, where I verified the engine side too** | Every rung guarded; the terminal rung is *doing nothing*; **one** call site, in `BeginPlay` |
| **(5)** | ⭐ the header doc is a **deliverable** — owner-class contract + **BOTH** failure cases | ✅ **PASS, and it is genuinely the deliverable** | `CommanderNpc.h:331-373`. Names the contract as a **contract with the owner's class** (`:336-338`); names what *this field* demands — *"the assigned ABP's graph must make ⛔ NO PAWN ASSUMPTION about its owner"* (`:338-343`); and states **both** cases under explicit `(1)`/`(2)` headings (`:347-356`) — case (2) carrying the 1,806-in-49 s figure, the two-gates-and-111-tests admission, and the sentence that makes it a lesson rather than a note: *"This is the case the previous version was SILENT about, and the silence is what shipped."* ⇒ ⭐ **the reassuring-but-incomplete text criterion (5) exists to catch is gone.** `:357-358` lands `"NULL-SAFE" IS NOT "OWNER-SAFE"` (item 4) and `:360-364` records that `ABP_Footman` is **not a legal value** while `ASummonedUnit`'s use is correct |
| **(6)** | `FT-§12b` PAIR RULE — no stale cross-reference in **EITHER** direction | ✅ **PASS — re-checked at the artifact, ⛔ not taken on the manager's word** | I read `SummonedUnit.cpp:54-60` in full: the note is about **MCP's inability to author per-unit AnimBPs** and the shared `SK_Footman_Skeleton`; it names **no commander and makes no claim about `CommanderNpc`**. `Commander` across `SummonedUnit.{h,cpp}` → **0 (my count).** ⇒ removing this file's half of the *"PAIRED constant"* note leaves **zero** stale reference in either direction. ⛔ **Nothing to report under the stop-and-report clause** |
| **(7)** | `SC-§33` — defaulted parameters added: expected **ZERO** | ✅ **PASS — ZERO, by my own enumeration** | See §5. **The new function takes zero parameters**, so the law cannot fire structurally |
| **(8)** | M8 declaration present | ✅ **PASS** | Tier **C** intact and untouched in **both** places: class doc `CommanderNpc.h:45-59` and constructor `CommanderNpc.cpp:107-112`. **No replicated property, no new class, no new tier, no RPC.** `AvatarIdleAnimAsset` carries **no** `Replicated`/`ReplicatedUsing` specifier — an `EditAnywhere` class default, identical on both machines because it is not state |
| **(9)** | the 111-test non-invalidation grep present **and its result pasted** | ✅ **PASS — present in the handoff, and I RE-RAN IT** | **My own count: 8 hits, all in `Tests/SiegeWarMapTest.cpp` — `:11` (the `#include`), `:24`, `:79`, `:1565`, `:1577`, `:1581` (comment prose), `:1583` `GetDefault<ACommanderNpc>()`, `:1585` `TestNotNull`. ⛔ Not one reads anim state.** The single test touching this class asserts `GetEnemyRevealCost()` / `GetInteractRadius()` on the CDO; **neither accessor, neither number and no anim field is altered by this diff** ⇒ ✅ **NON-INVALIDATED.** ⛔ **111 is a count of what is REGISTERED, ⛔ not a prediction of what passes** |
| **(10)** | ⛔⛔ the limit on my own verdict, stated in those terms | ✅ **STATED** — the blockquote at the head, and §7 |

---

## 2. ⛔ THE MECHANICAL CHECKS — MY OWN COMMANDS, MY OWN RAW COUNTS

| grep (over `Source/`, or the two files where noted) | handoff claim | **my count** | ✅ |
|---|---|---|---|
| `CommanderSharedLocomotionAbpPath` | 0 | **0** | ✅ |
| `ABP_Footman` in `CommanderNpc.{h,cpp}` | 4, all prose | **4** — `.h:354` `.h:360` `.cpp:31` `.cpp:162`, **each read in place inside its comment block** | ✅ |
| `Commander` in `SummonedUnit.{h,cpp}` | 0 | **0** | ✅ |
| `SharedLocomotionAbpPath` in `SummonedUnit.cpp` | untouched at `:60` | **`:60` declaration + `:459` use + `:448` comment** — byte-consistent with the pre-diff file | ✅ |
| `ApplyAvatarAnimation` across `Source/` | 1 call site | **1 call site (`CommanderNpc.cpp:199`) + 1 definition (`:289`) + 1 declaration (`h:433`); the other 3 hits are comments (`cpp:54`, `cpp:284`, `h:369`, `h:392`)** | ✅ |
| `ResolveAvatarMesh` across `Source/` | 2 call sites, signature unmoved | **`cpp:177` (OnConstruction) + `cpp:189` (BeginPlay)**; `void ResolveAvatarMesh(bool bWarnIfMissing)` byte-identical at `h:421` / `cpp:257` | ✅ |
| anim-state reads in `Tests/` | 8, none | **8, none** | ✅ |
| `TSoftClassPtr<UAnimInstance>` **assignments** across `Source/` | — | **2, both in `SummonedUnit.cpp` (`:455`, `:459`)** — see §6 | 📌 |

**Zone A, cheaply re-confirmed:** neither owned file is a Zone builder, neither is in `MeasuredCharCount`'s input surface, and the diff adds **no** `TEXT(` to any prompt lane. ⇒ **5658 cannot have moved by this diff, by construction rather than by inspection luck.**

---

## 3. ⭐⭐ THE FINDING I WAS TOLD TO SCRUTINISE — **TRUE. I READ IT AT THE ENGINE.**

⛔ **I did not take this from the handoff.** `C:\Program Files\Epic Games\UE_5.8\Engine\Source\Runtime\Engine\Private\Animation\AnimSingleNodeInstance.cpp`, `UAnimSingleNodeInstance::SetAnimationAsset`, **lines 44-60, character for character:**

```cpp
	USkeletalMeshComponent* MeshComponent = GetSkelMeshComponent();
	if (MeshComponent)
	{
		if (MeshComponent->GetSkeletalMeshAsset() == nullptr)
		{
			// if it does not have SkeletalMesh, we nullify it
			CurrentAsset = nullptr;
		}
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

⇒ ⭐⭐ **THE CLAIM IS EXACTLY RIGHT, INCLUDING THE COMMENT/CODE DIVERGENCE.** `:57` says *"matching skeleton"*; `:55` tests **`== nullptr`**. **The component's skeleton is never consulted.** The only asset-nulling guards are *no mesh at all* and *the sequence has no skeleton whatsoever*. An incompatible-but-non-null clip is passed straight to `Proxy.SetAnimationAsset(...)` at `:67`.

⇒ ⛔ **The spec's "a mismatch must degrade to ref pose" had NO engine-side enforcer.** Writing the comparison at the consumer is therefore **necessary**, not defensive decoration, and `CommanderNpc.cpp:358-367` is the right place for it — **before** `SetAnimationMode` and **before** `PlayAnimation`, so on a mismatch the component never even leaves its default mode.

**The rejection of `USkeleton::IsCompatibleForEditor` is also correct** — I located all three overloads at `Skeleton.h:702`, `:707`, `:712`, and confirmed with a whitespace-tolerant directive scan that they sit inside the `#if WITH_EDITORONLY_DATA` block opened at `:651` and closed at `:722`, **with no nested directive in between** ⇒ genuinely absent from a non-editor build. (The handoff and the shipped comment both say *"`WITH_EDITOR`"*; the actual guard is `WITH_EDITORONLY_DATA` — **NIT-1**, substance identical.)

⚠️ **But the rejection was not the whole engine picture — see WARN-1.**

**And the direction of error is the safe one, which is what makes this uncontroversial:** identity is *stricter* than compatibility, so the failure mode is a ref-pose commander, which `WR-§9`-style is a cosmetic downgrade with **no** effect on `InteractRadius`, the war-map gate or `EnemyRevealCost`. ⛔ **The skeleton hypothesis itself remains a hypothesis (`SC-§20`) and I did not close it — nobody may, from a code read.**

---

## 4. CRITERION (4), RUNG BY RUNG — AND THE ENGINE CORROBORATION THE HANDOFF DID **NOT** CLAIM

| rung | site | null-safety | log |
|---|---|---|---|
| entry | `cpp:296-299` | `if (!AvatarMesh) return;` | — |
| **1** — AnimBP | `cpp:306-310` | `LoadSynchronous()` on an **empty** soft class ptr returns `nullptr` without a load attempt ⇒ falls through. `TSoftClassPtr::LoadSynchronous()` returns `UClass*` (`SoftObjectPtr.h:1008`) ⇒ the `if (UClass* const …)` form compiles | **none** — ⛔ dormant, the field ships empty |
| **2a** — body | `cpp:315-321` | `GetSkeletalMeshAsset()` null ⇒ return | **deliberately silent** — `ResolveAvatarMesh` already warned once at `cpp:272`. ✅ Correct: a second line is the same fact twice |
| **2b** — opt-out | `cpp:323-328` | `IsNull()` ⇒ return | **deliberately silent** — the `AttackImpactEffect` `IsNull` designer-opt-out pattern. ✅ |
| **2c** — resolve | `cpp:330-340` | `LoadSynchronous()` null ⇒ return | **one line, `Log`** ✅ |
| **2d** — skeleton | `cpp:358-367` | `!MeshSkeleton \|\| IdleSequence->GetSkeleton() != MeshSkeleton` ⇒ return. ⭐ **Both `GetNameSafe` arguments in the log can be null and `GetNameSafe(nullptr)` yields `"None"` ⇒ ⛔ no null deref on the failure path itself** | **one line, `Log`** ✅ |
| play | `cpp:375-376` | reached only when mesh, sequence and skeleton identity all hold | none |
| **3** — ref pose | every early return | **doing nothing IS the rung** — there is no call to get wrong | — |

⛔ **No `Warning` is added on any path.** Both new lines are `Log`, both begin `[<ActorName>] CommanderNpc: idle sequence`, both end `— REF POSE (cosmetic only; …)`. ⇒ criterion (4)'s *"no new `Warning` on the EXPECTED path"* is met in the strongest form: **no new `Warning` on ANY path.**

⛔ **No per-frame path exists.** `PrimaryActorTick.bCanEverTick = false` (`cpp:105`), no timer, no delegate, no `Tick` override — and `ApplyAvatarAnimation` has **one** call site by my own grep.

⭐⭐ **THE COROBORATION I ADD, AND IT UPGRADES REQUIREMENT (d) FROM STYLE TO CORRECTNESS.** I checked whether "no playback from `OnConstruction`" is merely a house preference. **It is not — the engine says so itself.** `SkeletalMeshComponent.h:1257` declares:

```cpp
	UFUNCTION(BlueprintCallable, Category = "Components|Animation", meta = (Keywords = "Animation", UnsafeDuringActorConstruction = "true"))
	ENGINE_API void PlayAnimation(class UAnimationAsset* NewAnimToPlay, bool bLooping);
```

⇒ **`PlayAnimation` is flagged `UnsafeDuringActorConstruction` by Epic**, with the header's own note (`:1254-1256`) that it *"is not safe to be used during construction script — please use `OverrideAnimationData` for construction script."* ⇒ ⭐ **The extraction into `ApplyAvatarAnimation()` with a single `BeginPlay` call site is not one acceptable seam among several; it is the engine-sanctioned one.** The alternative the implementer weighed and rejected — a runtime `HasActorBegunPlay()` guard inside `ResolveAvatarMesh` — would have left a *reachable* `PlayAnimation` in the construction path guarded only by a runtime predicate. ✅ **The declared departure (3) is RATIFIED, and on stronger grounds than it claimed for itself.**

**And the whole call chain is null-safe on the engine side too, which I verified rather than assumed** (`SkeletalMeshComponent.cpp:4278-4329`): `PlayAnimation` → `SetAnimationMode` → `SetAnimation` → `Play`, each opening with a `bEnableAnimation` guard and each operating through a **null-checked** `GetSingleNodeInstance()`. ⛔ **No dereference anywhere in the path.** `SetAnimationMode` creates the instance only when `GetSkeletalMeshAsset() != nullptr` — a precondition rung 2a has already established. ⇒ **the `UAnimSingleNodeInstance` is guaranteed to exist by the time `PlayAnimation` reaches `SetAnimation`.**

**Deprecation sweep on every engine symbol this diff newly names — all clean in 5.8:** `GetSkeletalMeshAsset()` (`SkeletalMeshComponent.h:375`) · `SetAnimationMode(EAnimationMode::Type, bool = true)` (`:1246`) · `PlayAnimation(UAnimationAsset*, bool)` (`:1258`) · `SetAnimInstanceClass(UClass*)` (`:1096` — ⛔ **the current one; the two `UE_DEPRECATED` neighbours at `:1080` (4.23 `K2_SetAnimInstanceClass`) and `:1084` (5.5) are different functions and neither is called**) · `EAnimationMode::AnimationSingleNode` (`:192`, live) · `USkeletalMesh::GetSkeleton()` (`SkeletalMesh.h:747`/`:757` — ⛔ **the accessor is NOT deprecated; it wraps the deprecated `Skeleton` *member* in `PRAGMA_DISABLE_DEPRECATION_WARNINGS`, which is exactly why it must be used instead of the field**) · `UAnimationAsset::GetSkeleton()` (`AnimationAsset.h:1187`, **outside** the `#endif // WITH_EDITOR` at `:1184` ⇒ runtime-available). ✅

**Header/cpp and reflection consistency, checked line by line:** all three private declarations match their definitions (`h:421`↔`cpp:257`, `h:433`↔`cpp:289`, `h:436`↔`cpp:383`); `AvatarIdleAnimAsset` carries `UPROPERTY(EditAnywhere, Category = "Siegebound|CommanderNpc")` matching its two siblings exactly; `class UAnimSequence;` is forward-declared (`h:12`) with `Animation/AnimSequence.h` included in the `.cpp` (`:6`) — **the identical split this file already ships for `USkeletalMesh`/`UAnimInstance`, so the pattern is proven to compile here**; the two new includes are in correct alphabetical position (`AnimInstance` < `AnimSequence` < `Skeleton`) and **both are genuinely required** — `AnimSequence.h` for `Cast<UAnimSequence>` inside `LoadSynchronous` and the `UAnimationAsset*` upcast, `Skeleton.h` for the `const USkeleton*` → `const UObject*` upcast into `GetNameSafe`. ⛔ **No GC exposure added: soft pointers hold no strong reference, and no raw `UObject*` member was introduced.**

---

## 5. `SC-§33` — **ZERO, AND I RE-DERIVED IT MYSELF FROM THE HEADER**

⛔ **I did not paste the handoff's table.** I enumerated **every** function declaration in `CommanderNpc.h`:

`ACommanderNpc()` `:147` · `InitCommanderNpc(ETeamId)` `:165` · `GetCommanderTeam() const` `:176` · `IsPlayerInRange(const FVector&) const` `:192` · `GetInteractRadius() const` `:196` · `GetEnemyRevealCost() const` `:200` · `FindCommanderNpcForTeam(UWorld*, ETeamId)` `:226` · `OnConstruction(const FTransform&)` `:229` · `BeginPlay()` `:234` · `ResolveAvatarMesh(bool)` `:421` · **`ApplyAvatarAnimation()` `:433` ← NEW, ⛔ ZERO PARAMETERS** · `ResolveWarTableMesh(bool)` `:436`.

⇒ ⛔ **Not one defaulted parameter exists in this class, before or after the diff.** ✅ **And the structural choice is the right one and I endorse it explicitly:** a parameterless function **cannot grow a default later**, whereas threading a second `bool` into `ResolveAvatarMesh` would have created exactly the two-`bool`-signature shape `SC-§33` was written to prevent — while *also* leaving `PlayAnimation` reachable from the construction path (§4). **One decision, three problems avoided.**
📌 The field initialisers (`InteractRadius = 400.f`, `EnemyRevealCost = 30`, `CommanderTeam = ETeamId::Blue`, the two `bWarned…= false`) are **member defaults, not parameter defaults** — outside the law, and all byte-unchanged.
📌 The engine's own `SetAnimationMode(EAnimationMode::Type, bool bForceInitAnimScriptInstance = true)` **does** carry a trailing default and is called with one argument. It is an **engine** signature this task did not author, so `SC-§33` does not reach it. ✅ **Correctly pre-empted rather than left for me to wonder about.**

---

## 6. FINDINGS

- **[WARN-1]** `CommanderNpc.cpp:351-357` — **the skeleton-check rationale is CORRECT but INCOMPLETE, and incompleteness in a load-bearing comment is the exact thing this batch was bought by.** The comment rejects `USkeleton::IsCompatibleForEditor` because it is editor-only (**true — verified, `Skeleton.h:651-722`**) and will be read as *"no runtime alternative existed."* ⛔ **One does.** `ENGINE_API bool USkeleton::IsCompatibleMesh(const USkinnedAsset* InSkinnedAsset, bool bDoParentChainCheck = true) const` sits at **`Skeleton.h:766`, OUTSIDE the editor-only block** (which closes at `:732`) ⇒ **runtime-available**, and `USkeletalMesh` derives from `USkinnedAsset`, so `IdleSequence->GetSkeleton()->IsCompatibleMesh(AvatarSkeletalMesh)` would compile in a shipping build. It also answers the *better-posed* question — *can this clip's skeleton drive this mesh* — rather than *are the two skeleton assets the same object*. ⚖️ **MY RULING: KEEP THE IDENTITY TEST. ⛔ DO NOT CHANGE THE CODE.** `IsCompatibleMesh` is deliberately **permissive** (bone-hierarchy match, parent-chain match, >50% of bones), which is the *opposite* of the conservative direction the task chose on purpose; and the fleet is retargeted onto one skeleton, so identity is the expected match anyway. **What is owed is one comment sentence naming the runtime API and why it was still declined** — otherwise a future author re-derives this from scratch or, worse, "fixes" the comment by loosening the check. **Fix/owner: comment-only, next author in `CommanderNpc.cpp` ("while you are in the file"), or a manager board item. ⛔ NOT a required fix in this loop and ⛔ NOT commit-blocking.**
- **[WARN-2]** `CommanderNpc.cpp:301-310` (rung 1) — **the escape hatch is unguarded against the very defect class this task repairs, and documentation is the only control that exists.** If anyone ever sets `AvatarAnimClassAsset` on `BP_CommanderNpc` (or a future child) to a Pawn-assuming ABP, `SetAnimInstanceClass` runs, the graph instantiates on an `AActor`, and **the 1,806-error defect returns verbatim — with no compile error, no test failure, no log line, and rung 2's skeleton guard bypassed entirely by the `return`.** ⚖️ **This is INHERENT, not a coding mistake:** C++ cannot inspect a Blueprint graph's node set at runtime, so no code-level guard is possible, and the mitigation actually shipped (`h:331-373` + `cpp:158-167`) is as thorough as prose can be. ⭐ **Suggested follow-up, board-only:** one `UE_LOG(LogGitClaudeUnrealTest, Warning, …)` at rung 1 naming `SC-§35` whenever the hatch is taken — ✅ **it cannot fire on the shipped path, because the field ships empty and rung 1 is therefore unreachable**, so it would **not** violate criterion (4)'s no-new-`Warning`-on-the-expected-path rule. ⛔ **Not required now — it would spend a loop, and this is loop 2 of 3.** ⛔ Board edits are the manager's.
- **[WARN-3]** batch-wide — ⛔⛔ **NOTHING IN THIS GATE OBSERVES THE DEFECT, AND A THIRD CLEAN REVIEW IS NOT EVIDENCE.** The original 1,806 errors survived a clean compile, 111/111 and **two** PASS gates; this is the third read of the same class by the same instrument. `SC-§35` item 5 is explicit that the instrument is PIE and there is no substitute. ⇒ **Owed to TASK-593 row (m), and its `Blueprint Runtime Error` count — with the duration, ⛔ never "looked clean" — is the whole verdict.**
- **[NIT-1]** handoff §3 and shipped comment `CommanderNpc.cpp:352-353` both say `IsCompatibleForEditor` sits in a **`#if WITH_EDITOR`** block. The actual guard is **`#if WITH_EDITORONLY_DATA`** (`Skeleton.h:651` → `:722`). **Substance identical** — absent from a non-editor build either way — and the rejection is correct. **No action.**
- **[NIT-2]** `CommanderNpc.cpp:375` — `SetAnimationMode(EAnimationMode::AnimationSingleNode)` is **functionally redundant**: `PlayAnimation` calls it itself at `SkeletalMeshComponent.cpp:4286`. ⭐ **I verified the redundancy is HARMLESS rather than assuming it:** the first call flips `AnimationMode` from the default `AnimationBlueprint` (`bNeedChange == true`) and builds the `UAnimSingleNodeInstance`; `PlayAnimation`'s internal call then sees `bNeedChange == false` **and** `AnimationMode != AnimationBlueprint`, so `InitializeAnimScriptInstance` is **not** re-entered and the just-created instance survives. ⇒ **no double-init, no wasted re-initialisation. KEEP IT** — the code declares the redundancy and its purpose (`:369-374`), and making `SC-§35` item 2's shape visible in one line without opening the engine is worth one no-op call at `BeginPlay`.
- **[NIT-3]** the fall-through log is *"structurally once"* **per spawned actor** — which is the correct and intended claim — but with two castles that is **up to TWO identical `Log` lines per PIE session, one per commander.** Recorded so TASK-593 does not read a pair as a repeat or a per-frame symptom. ✅ The handoff's grep hint (`CommanderNpc: idle sequence`, *"at most one line per spawned commander"*) is accurate as written.
- **[NIT-4]** `CommanderNpc.cpp:137` (**pre-existing, out of diff, ⛔ not charged**) — `VisibilityBasedAnimTickOption = OnlyTickPoseWhenRendered` means the single-node pose does **not** evaluate while the commander is off-screen, and he will not be rendered at `BeginPlay`. Benign for a looping idle, but an observer walking into the hall may catch one un-posed frame before the pose ticks. ⛔ **That is not a skeleton mismatch and ⛔ not a finding** — recorded only so TASK-593 row (3) is not misfiled. The discriminator remains the log line, ⛔ never the first frame.

---

## 7. ⚖️ THE RULINGS THIS GATE OWES

**R1 — the `bWarnedX` departure (declared departure 4), which explicitly asked for my ruling: ✅ RATIFIED. ⛔ DO NOT ADD THE BOOL. The reasoning is right and I verified its premise rather than accepting it.** The two existing one-shots guard `ResolveAvatarMesh`/`ResolveWarTableMesh`, which are reachable from **`OnConstruction`** (`cpp:177-178`) — a function that re-runs on **every editor property tweak**. That is a genuinely different hazard. `ApplyAvatarAnimation` has **one** call site by my own repo-wide grep, in `BeginPlay`, and `AActor::BeginPlay` cannot run twice on one instance ⇒ **once-per-spawn is structural, not conventional.** A guard bool would be **dead state that asserts a per-frame path exists**, which is precisely the false signal a reviewer of this file must not be given. ✅ **And the invariant is defended where it can be seen:** `h:428-431` forbids a second call site in terms, and `cpp:282-286` plants a *do-not-re-add-an-anim-call-here* marker at the exact spot the old code sat. ⇒ **The local pattern is correctly NOT followed, and the divergence is correctly declared rather than left to read as an omission.**

**R2 — declared departure 1 (the `:129 → :141` drift): ✅ CORRECTLY HANDLED, and flagging it was the right call.** The text is byte-identical, the base class is untouched, and the +12 reconciles exactly against the two declared additions. ⭐ **Locating by symbol (`SC-§18c`) is what made this a non-event; a gate that had grepped `:129` would have read the wrong line and this project has already burned a task on exactly that.**

**R3 — declared departure 2 (the class doc gained the `SC-§35` item 6 refusal, an addition over the spec's edit list): ✅ RATIFIED. KEEP IT.** It is mandated by cited law, lands in an owned file, and its reasoning is correct: the paragraph previously carried **only** the navmesh objection (`h:61-66`), which a future author whose case does not involve the navmesh reads as permission. `h:68-77` now also names the pawn-shaped-query cost (`GetPawnIterator`, AI perception, targeting ⇒ **the commander becomes acquirable as a combat target**) and ties it to the `ITeamAgent` measurement below it as *"the same failure through a second door"* — which is materially true: `h:98-107` already records that an `ITeamAgent` commander would be an unkillable aggro sink. ⇒ **Two independent doors, one refusal, both now written down.**

**R4 — declared departure 3 (extraction over an in-place `HasActorBegunPlay()` guard): ✅ RATIFIED ON STRONGER GROUNDS THAN IT CLAIMED.** See §4: `PlayAnimation` is `meta = (UnsafeDuringActorConstruction = "true")` in the engine header. The spec left the seam to the implementer; **the engine had already chosen it.**

**R5 — the skeleton hypothesis: ⛔ NOT CLOSED, AND ⛔ I MAY NOT CLOSE IT.** `A_Sorcerer_Idle.uasset` existing on disk is not evidence about which skeleton it was built on, and opening the asset needs the editor. ✅ The `ASummonedUnit::CacheSkeletalClips` argument is **legitimate circumstantial support** — I confirmed at the artifact that `SummonedUnit.cpp:526` and `:565` drive `A_<CardID>_*` clips through the **same** `USkeletalMeshComponent::PlayAnimation` single-node API onto the matching `SK_<CardID>` body — **but it is one more strand, ⛔ not the measurement**, and the handoff is right to say so. ⇒ **TASK-593 row (3) is the measurement, and BOTH of its outcomes ship.**

**R6 — scope discipline: ✅ HELD.** `qa/TASK-565.md` and `qa/TASK-584.md` were **not** reopened; the eleven ratified `SC-§15` departures, `W4-R3` and the `WR-§2`/`WR-§2b` ledger were **not** re-examined. `WR-§9` rows 8 and 13 were confirmed **unchanged only** (`InteractRadius` 400 `h:295` · `EnemyRevealCost` 30 `h:311` · `CommanderAvatarYawOffset` −90 `cpp:77` · `CommanderWarTableForwardOffset` 200 `cpp:96` · both `NoCollision` profiles `cpp:126`/`:146`), ⛔ not re-litigated. **This gate ruled on one diff.**

**R7 — 📌 ONE ITEM TO BOARD, ⛔ EXPLICITLY NOT A REASON TO FAIL THIS GATE (criterion: *"if you find something outside the diff, BOARD IT"*).** After this repair, the **only** remaining `TSoftClassPtr<UAnimInstance>` assignments in the project are `SummonedUnit.cpp:455` (per-unit `ABP_<CardID>`, a **composed** path) and `:459` (the shared fallback). ⛔ **`SC-§35` item 1 requires code assigning one to state, in a comment at the assignment, WHICH OWNER CLASS THE ABP ASSUMES — and the block at `SummonedUnit.cpp:445-453` does not.** I read it in full: it documents the *skeleton* contract thoroughly (*"all units share `SK_Footman_Skeleton`"*) and the MCP-authoring history, but **never names the owner-class half** — it does not say *"these ABPs assume a Pawn owner; `ASummonedUnit` is an `ACharacter`, which is why this is legal."* ⭐ **Why this is worth a board item rather than a shrug: `:455` is a COMPOSED path over an arbitrary CardID, so it is the pattern a future author will copy** — and the file that shipped the defect had thorough documentation of the *other* half too. ⇒ **A comment-only follow-up, ⛔ NOT this task's** (TASK-591 was correctly forbidden to touch that file, and correctly did not — `Commander` in `SummonedUnit.{h,cpp}` → 0). ⛔ Board edits are the manager's. **✅ `ASummonedUnit`'s USE remains CORRECT and must not be disturbed.**

---

## 8. ⛔ WHAT I DID NOT DO

⛔ No compile · ⛔ no editor (**deliberately left alone — TASK-586 may be working in it; I neither opened it nor asked for it**) · ⛔ no MCP · ⛔ no PIE · ⛔ no Git · ⛔ **no code edited** (`SC-§27`) · ⛔ no board edit (no partial-edit tool — flips named in §9 for the orchestrator to proxy).
🔒 Holdout unspent · ⛔ nothing model-side · ⛔ no `DumpAssistantPrompt` / `SpikeEval` / `SpikePrompt` · ⛔ **no token figure quoted or derived** (`AS-§12g`) — Zone A referenced only as **5658 chars**.
⛔ **No new test is owed and that ruling is not reopened.** The defect needs a live anim update on a spawned actor; a CDO-default assertion would assert a literal against itself. ⛔ **`TestEqual`-on-`FString` case-insensitivity: no assertion in scope — `Tests/` is untouched by this diff and the 8 existing hits contain no `FString` comparison.**

---

## Notes for build-master (PASS → TASK-593)

1. ✅ **Cleared to compile.** Expect **exactly two** changed files from this diff — `Source/GitClaudeUnrealTest/Siegebound/CommanderNpc.h` and `CommanderNpc.cpp`. ⛔ **A third path from this task is a write-race finding, not a formatting detail.** ⚠️ The working tree carries the whole uncommitted batch, so `git status` will show far more; the reconciliation against the derived path list is `SC-§29b`'s and TASK-570's.
2. ⛔ **Parse the build log for `Result: Failed`. ⛔ NEVER `$LASTEXITCODE`** — `Build.bat` returns 0 on a failed build under the Live Coding mutex (the standing machine law).
3. ⛔ **Row (m) is the verdict, and this gate cannot substitute for it. Report the COUNT and the DURATION — ⛔ never "looked clean."** Expect **exactly 0** `Blueprint Runtime Error`; the original reading was 1,806 in 49 s, so run at least that long. The 13 benign Live Coding `ggml-cpu-*.dll` notices are **not** this row — count them separately.
4. ⭐ **Row (3), the cosmetic observation — the discriminator is the LOG LINE, ⛔ never the first frame** (NIT-4). Grep **`CommanderNpc: idle sequence`**: **zero lines = the idle is playing** (hypothesis held); **a `Log` line naming a skeleton mismatch = the guard did its job** (⛔ still not commit-blocking); **a `Log` line saying the sequence did not resolve = a finding worth recording, still not commit-blocking**; **ref pose with no line at all = report it.** ⚠️ **Up to TWO lines is normal — one per commander** (NIT-3). ⛔ **Neither line is a `Warning`; any `Warning` from this class is the pre-existing missing-mesh one.**
5. ⚠️ **`ASummonedUnit` still uses `ABP_Footman` and MUST keep working** — it is an `ACharacter`, its use is correct, and nothing in this diff touches it (my count: `Commander` in `SummonedUnit.{h,cpp}` = 0). ⛔ **If unit locomotion breaks, that is this diff having over-reached, not an expected consequence.**
6. 📌 **111 is a count of what is REGISTERED, ⛔ not a prediction of what passes.** A total that is not 111 is itself a finding.
7. ⛔ **The matrix did NOT improve** — nothing here touches input injection; the 11 unobserved rows stay unobserved and inherited (`W8-R4`). ⛔ Never report the matrix as passed.
8. 🔒 Zone A: neither owned file is in `MeasuredCharCount`'s input surface, so the **5658** blob-SHA equality proof stands and needs re-confirmation only, ⛔ not re-derivation. ⛔ **Quote no token figure.**
9. 📌 `.gen.cpp` regeneration from the UHT-visible doc blocks and the new `AvatarIdleAnimAsset` `UPROPERTY` is **expected**; `Intermediate/` is gitignored and must not appear in the commit.

---

## Board status flips (⛔ qa-reviewer has no partial-edit tool — orchestrator to proxy)

- **TASK-592** → `done ← ✅ QA PASSED 2026-08-16 — qa/TASK-592.md: 0 BLOCKER, 3 WARN, 4 NIT. Diff-scoped to TASK-591's two files; qa/TASK-565.md + qa/TASK-584.md NOT reopened. ⛔ THE PASS GATES THE SHAPE, NOT THE RESULT — SC-§35 item 5: only TASK-593's PIE Message-Log read can close the defect.`
- **TASK-591** → `qa-passed / ready-for-integration`, gate `qa/TASK-592.md`. ⛔ **Still COMMIT-BLOCKING for TASK-570 until TASK-593 row (m) reads ZERO.**
- **TASK-593** → unblocked (its hard gate `qa/TASK-592.md` = PASS is satisfied).
- 📌 **For the manager, from R7 — a new comment-only board item:** `SummonedUnit.cpp:445-453` documents the *skeleton* contract for its two `TSoftClassPtr<UAnimInstance>` assignments (`:455`, `:459`) but **not the owner-class contract `SC-§35` item 1 now requires**. ⛔ Not a defect (the use is correct); it is the **only** remaining undocumented assignment in the project and `:455` is a composed path future authors will copy.
