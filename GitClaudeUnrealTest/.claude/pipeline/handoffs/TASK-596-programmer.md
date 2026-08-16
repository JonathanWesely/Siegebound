# Handoff — TASK-596 — [WR-42] COMMENT-ONLY WAVE 4 (gameplay-programmer, 2026-08-16)

**Status: ready-for-qa. ⛔ COMMENT-ONLY — zero behaviour change, zero emitted bytes, not one executable line. This diff RIDES TASK-606's Build.bat per the boarding condition; this task ran no compile, no git, no editor, no MCP.**

**Spec:** TASKBOARD WAR-ROOM TASK-596 (items 1–4). **Sources:** `qa/TASK-592.md` WARN-1, WARN-2, R7. **Law:** SC-§35 items 1+3, SC-§15, SC-§27, RELAYED-DIAGNOSIS LAW.

---

## 1. THE DIFF, EXACTLY — 2 files, +35/−7, ALL 42 CHANGED LINES ARE COMMENT TEXT

| file | hunk | changed lines | nature |
|---|---|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/CommanderNpc.cpp` | old `:351-357` → new `:351-371` | 7 removed + 21 added, **every one begins `\t// `** | WARN-1 fix (spec item 1) |
| `Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.cpp` | inserted at `:454-467` (after old `:453`) | 14 added, 0 removed, **every one begins `\t// `** | R7 owner-class contract (spec item 3) |

**Comment-only proof, performed mechanically:** `git diff -U0` over exactly these two paths, reviewed line-by-line. Every `-` line and every `+` line in both hunks starts with a tab followed by `//`. **0 executable lines, 0 preprocessor lines, 0 signature/UPROPERTY/metadata lines, 0 whitespace-only code changes.** No `.h`, no `.uasset`, no `ABP_Footman` asset, no third file. Both files were verified **unmodified in git before my first edit** (empty `git status --porcelain` on both paths — reconciles the build-master pre-flight; my edits are the only change in them).

**Line-drift map for downstream readers (SC-§18c: locate by symbol, not offset):**
- `CommanderNpc.cpp`: `const USkeleton* const MeshSkeleton = …` moved `:358 → :372`; everything through `:350` (incl. rung 1 at `:301-310`) is untouched.
- `SummonedUnit.cpp`: `const FString AbpPath = …` moved `:454 → :468`; the two `TSoftClassPtr<UAnimInstance>` sites moved `:455 → :469` (composed) and `:459 → :473` (shared fallback); `SetAnimInstanceClass` `:464 → :478`.

---

## 2. SITE 1 — `CommanderNpc.cpp` (WARN-1): the declined runtime API, now ON the record

**BEFORE (old `:351-357`, verbatim):**
```cpp
	// ⚠️ Deliberately an EXACT identity test, and deliberately ⛔ NOT
	// USkeleton::IsCompatibleForEditor — that API is WITH_EDITOR-only and this
	// path runs at runtime. Identity is the CONSERVATIVE direction: a merely
	// "compatible" skeleton is refused and falls to ref pose, which is the safe
	// way to be wrong. The whole rigged fleet is retargeted onto the single
	// SK_Footman_Skeleton, so identity is the EXPECTED match and the conservatism
	// is expected to cost nothing.
```

**AFTER (new `:351-371`, verbatim):**
```cpp
	// ⚠️ Deliberately an EXACT identity test, and deliberately ⛔ NOT either of
	// the engine's skeleton-compatibility APIs — BOTH were considered and each
	// was DECLINED for its own reason (qa/TASK-592.md WARN-1):
	//   (a) USkeleton::IsCompatibleForEditor — UNAVAILABLE here: all three
	//       overloads sit inside Skeleton.h's #if WITH_EDITORONLY_DATA block,
	//       and this path runs at runtime in a non-editor build.
	//   (b) USkeleton::IsCompatibleMesh — AVAILABLE at runtime (it is declared
	//       OUTSIDE the editor-only block, and USkeletalMesh derives from
	//       USkinnedAsset, so IdleSequence->GetSkeleton()->IsCompatibleMesh(
	//       AvatarSkeletalMesh) would compile in a shipping build) — and STILL
	//       DECLINED: it is deliberately PERMISSIVE (bone-name matching plus an
	//       optional parent-chain walk against a percentage threshold — read at
	//       the engine source, not assumed), which is the OPPOSITE of the
	//       conservative direction this guard chose on purpose.
	// Identity is the CONSERVATIVE direction: a merely "compatible" skeleton is
	// refused and falls to ref pose, which is the safe way to be wrong. The
	// whole rigged fleet is retargeted onto the single SK_Footman_Skeleton, so
	// identity is the EXPECTED match and the conservatism is expected to cost
	// nothing. ⛔ Do NOT "complete" this guard by swapping the identity test for
	// IsCompatibleMesh — the permissive direction is a RECORDED REJECTION, not a
	// gap left for want of a runtime API.
```

---

## 3. SITE 2 — `SummonedUnit.cpp` (R7 / spec item 3): the owner-class contract SC-§35 item 1 requires

**BEFORE:** the block at `:445-453` ended at *"…ref pose, null-safe, never a crash (matches the soft-ref discipline above)."* and went straight to `const FString AbpPath = …`. It documented the skeleton contract and resolution order only — the owner-class half was absent (the only remaining undocumented `TSoftClassPtr<UAnimInstance>` assignment in the project).

**AFTER (inserted `:454-467`, verbatim — the pre-existing lines around it are byte-identical):**
```cpp
	//
	// ⛔ SC-§35 item 1 — THE OWNER-CLASS CONTRACT, the half the paragraph above does not
	// state (added by TASK-596, from qa/TASK-592.md R7): BOTH classes resolved below — the
	// composed per-unit ABP_<CardID> and the shared ABP_Footman fallback — ASSUME A PAWN
	// OWNER. ABP_Footman's EventGraph drives its Set GroundSpeed / Set bIsMoving nodes
	// through UAnimInstance::TryGetPawnOwner(), which returns None on a non-Pawn owner.
	// ASummonedUnit is an ACharacter (see the class declaration in SummonedUnit.h), i.e. a
	// Pawn — which is exactly why ABP_Footman is a LEGAL value HERE and this assignment is
	// CORRECT. Assigning either class to a non-Pawn actor is the exact defect SC-§35 was
	// written for: 1,806 Blueprint runtime errors in 49 s with 2 NPCs alive, through a
	// clean compile, 111/111 tests and two QA gates. The composed path below is the shape
	// future unit authors will copy — if the copying class is NOT a Pawn, the repair is
	// made at that consumer (single-node playback, SC-§35 items 2 + 3), ⛔ never by
	// editing the shared ABP_Footman.
```

Placement: between the resolution-order paragraph and the first assignment line, so it sits **at the assignment** per SC-§35 item 1's letter, covering both sites (`:469` composed — the copy-target — and `:473` shared fallback). `ABP_Footman` itself: ⛔ NOT touched (spec item 3's last clause; SC-§35 item 3).

---

## 4. SPEC ITEM 2 (WARN-2) — DIAGNOSED FIRST, AS ORDERED. VERDICT: THE GUARD CANNOT BE MADE STRUCTURAL — PROSE LEFT BYTE-IDENTICAL

**The diagnosis (mine, at the engine and the artifact, not relayed):**
1. Rung 1 (`CommanderNpc.cpp:301-310`, untouched) hands the loaded `UClass*` to `SetAnimInstanceClass`. A structural guard would have to answer, at assignment time, *"does this compiled AnimBlueprint's graph query its owner as a Pawn?"* **No runtime query exists for that:** the EventGraph node set (the `TryGetPawnOwner` call) is editor-side authoring data on the Blueprint asset — stripped in cooked builds — and the generated class exposes no flag, interface, or metadata recording its owner assumptions. C++ at this site cannot enumerate a compiled graph's nodes.
2. The only pseudo-structural alternative — requiring the class to `IsChildOf` a bespoke marker base (e.g. a purpose-built `UAnimInstance` subclass) — is (a) a **code change**, forbidden by item (4), and (b) **not actually structural**: deriving from a marker cannot prevent a graph from calling `TryGetPawnOwner`; it asserts intent, not behaviour. It would be the same prose guard wearing a type.
3. The defect's only observable is the per-frame Blueprint runtime error stream in a live anim update — SC-§35 item 5's instrument is PIE, and no assignment-time check can front-run it.

⇒ **The guard genuinely cannot be made structural.** Per the spec's own conditional — *"say so and leave the prose"* — I am saying so here, and the prose at rung 1 was **left byte-identical** (the diff contains no hunk before `:351`). The once-suggested rung-1 `Warning` was **not** added: the field ships empty, rung 1 is unreachable on every shipped path, so the line could never fire — an unfireable log is thoroughness theater, not a guard — and adding it would have been a code change regardless. QA's WARN-2 ruling ("INHERENT, not a coding mistake; documentation is the only control") is **confirmed, not contradicted**.

---

## 5. SYMBOL VERIFICATION — EVERY NAME WRITTEN INTO A COMMENT, CHECKED AT ITS SOURCE FIRST (the phantom-`ResolveHeroStart` lesson this task descends from)

| symbol written | verified where (first-hand, this session) |
|---|---|
| `USkeleton::IsCompatibleMesh(const USkinnedAsset*, bool bDoParentChainCheck=true) const` | `Skeleton.h:766` — **outside** every editor-only block; impl read at `Skeleton.cpp:648-…`: bone-name match loop, `bDoParentChainCheck` parent-chain walk, *"at least % of bone should match"* threshold ⇒ the "PERMISSIVE" characterization is read from the source, not from the QA report |
| `USkeleton::IsCompatibleForEditor` (3 overloads) | `Skeleton.h:702/:707/:712`, inside `#if WITH_EDITORONLY_DATA` opened `:651`, closed `:722` — **the correct guard is `WITH_EDITORONLY_DATA`**, see the SC-§15 declaration below |
| `USkeletalMesh : public USkinnedAsset` | `SkeletalMesh.h:439` |
| `UAnimInstance::TryGetPawnOwner()` | `AnimInstance.h:465` |
| `ASummonedUnit : public ACharacter` | `SummonedUnit.h:118` (⇒ a Pawn; `ACharacter : APawn` is engine hierarchy) |
| `IdleSequence`, `AvatarSkeletalMesh` | locals of the enclosing `ApplyAvatarAnimation()` (`CommanderNpc.cpp:330`, `:315`) |
| `SK_Footman_Skeleton`, `ABP_Footman`, `ABP_<CardID>` | asset names per CONVENTIONS/spec; prose-only here |
| `Set GroundSpeed` / `Set bIsMoving` | the ABP node names as measured and recorded in SC-§35's defect record |
| `qa/TASK-592.md` WARN-1 / R7, `SC-§35`, `TASK-596` | exist at `.claude/pipeline/qa/TASK-592.md` / CONVENTIONS / the board |

All engine references are cited **by symbol and header**, never by bare line offset, in the comment text itself (the `:129 → :141` drift lesson, SC-§18c).

## 6. SC-§15 — ONE DECLARED DEPARTURE

**D1 (rider on WARN-1, NIT-1's substance):** the replaced sentence said the rejected editor API was *"WITH_EDITOR-only"*. `qa/TASK-592.md` NIT-1 established the actual guard is **`WITH_EDITORONLY_DATA`** and ruled "No action" — but this task re-authors that exact sentence, and re-writing a known-wrong engine name into fresh comment text would recommit the phantom-name failure mode TASK-596 exists to prevent. The new text carries the correct guard, verified first-hand (§5). Substance identical (absent from a non-editor build either way), comment-only, zero behaviour implication. Declared, not silent.

No other departure. Item (2) produced **no edit** by the spec's own conditional; the `SummonedUnit.cpp` insertion sits inside the names-block's cited region, at the assignment.

## 7. WHAT QA SHOULD SCRUTINIZE (SC-§27 diff-scope) + GREP-COUNT DELTAS vs `qa/TASK-592.md` §2

- The **whole reviewable surface is the two hunks in §2/§3** — confirm every changed line is comment text (`git diff -U0` on the two paths reproduces my proof) and that `CommanderNpc.cpp:301-310` (rung 1) plus both files' code lines are byte-identical.
- **Expected count deltas (all comment prose, none a code change):** `ABP_Footman` in `SummonedUnit.cpp` gains **4** prose hits (in the inserted block) on top of the prior `:60`-declaration/`:473`-use/`:448`-comment; `ABP_Footman` in `CommanderNpc.{h,cpp}` **unchanged at 4** (my new text there never names it); `IsCompatibleMesh` appears **3×** and `IsCompatibleForEditor` **1×** in `CommanderNpc.cpp`, comments only; `TryGetPawnOwner` appears **1×** in `SummonedUnit.cpp`, comment only. `Commander` in `SummonedUnit.{h,cpp}`: **still 0** (the inserted block deliberately says "non-Pawn actor", not "commander").
- Zone A: neither file is in `MeasuredCharCount`'s input surface; no `TEXT(` added anywhere (the diff shows none) ⇒ 5658 cannot have moved.

## 8. WHAT I DID NOT DO

⛔ No compile (`Build.bat` untouched — **TASK-606 owns the gate; this diff rides it and owes its line in the ACCOUNTS batch's ledger**) · ⛔ no git commands beyond read-only `status`/`diff` · ⛔ no editor, no MCP, no PIE · ⛔ no `.h`, no `.uasset`, no `ABP_Footman`, no board edit beyond the single TASK-596 status flip · ⛔ no logic, no guard, no signature, no UPROPERTY/metadata (nothing UHT-visible changed: both hunks are inside function bodies, so no `.gen.cpp` claim is even generated). 🔒 Nothing model-side; no token figure quoted.
