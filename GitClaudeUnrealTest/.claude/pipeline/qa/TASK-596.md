# QA Report — TASK-596 — [WR-42] COMMENT-ONLY WAVE 4 (SC-§27 diff-scoped verdict)

**Verdict: PASS**
**0 BLOCKER · 1 WARN · 3 NIT**
**Date:** 2026-08-16 · **Scope (SC-§27):** the two hunks — `CommanderNpc.cpp` old `:351-357` → new `:351-371` and `SummonedUnit.cpp` inserted `:454-467` — plus the fences they must not have disturbed. ⛔ Nothing already passed by `qa/TASK-592.md` was re-litigated.
**Gate position:** this verdict gates **TASK-608's integration commit**, not the build — the diff rides **TASK-606's** `Build.bat` per the boarding condition, and owes its line in the ACCOUNTS batch's ledger (pre-flight row 3).
**Method:** read-only file access + the installed UE 5.8 source at `C:\Program Files\Epic Games\UE_5.8\Engine\Source`. ⛔ No compile · no editor · no MCP · no PIE · **no Git (by design — see §1's instrument statement)** · no code edited · no board edit (flips in §6 for the orchestrator to proxy). 🔒 Nothing model-side; no token figure quoted.

---

## 0. THE ONE-PARAGRAPH RESULT

**The comment-only claim — the whole gate — holds under my own instruments, not the handoff's.** Every line in both hunk regions read first-hand begins tab + `//`; every pre-hunk anchor independently recorded by `qa/TASK-592.md` is byte-consistent and unmoved; every post-hunk anchor in both files has drifted by exactly **+14** — the arithmetic of +21/−7 and +14/−0 — and the claimed +35/−7 = 42 reconciles. **Every symbol the new comments name exists where claimed, verified at the engine and the codebase first-hand** (§3), and the phantom `ResolveHeroStart` stands at **ZERO sites in `Source/`, with a passing positive control** (§4). The one declared SC-§15 departure is **RATIFIED** (§5). Spec item (2)'s no-edit outcome is confirmed: no hunk exists before `:351`, rung 1 (`:301-310`) is unmoved and matches WARN-2's citation character-for-character.

---

## 1. THE COMMENT-ONLY CLAIM — MY OWN INSTRUMENT, ITS RESULT, AND ITS STATED LIMIT

⛔ **I have no Git access, so `git diff -U0` is the programmer's evidence, not mine.** My independent instrument is three-legged, and each leg is against an artifact the handoff does not control:

1. **Direct read of both hunk regions.** `CommanderNpc.cpp:351-371` — 21 lines, every one begins `\t// `. `SummonedUnit.cpp:454-467` — 14 lines, every one begins `\t// `. Zero executable text, zero preprocessor text, zero string literals (`TEXT(` count in both hunks: **0**). Both hunks sit **inside function bodies** (`:351` inside `ApplyAvatarAnimation()`, defined `:289`; `:454` inside the anim-resolution body between the `:441-443` facing code and the `:468` `AbpPath` line) ⇒ nothing UHT-visible; no `.gen.cpp` regeneration is even claimed by this diff.
2. **Independent BEFORE baseline = `qa/TASK-592.md`** (written by the prior gate against the pre-596 tree, not by this task's author). Its recorded offsets: old comment block `:351-357` (7 lines — matches the −7); rung 1 at `:301-310`; locals `AvatarSkeletalMesh :315` / `IdleSequence :330`; skeleton-check code `:358-367`; play calls `:375-376`; `ResolveWarTableMesh` def `:383`; `SummonedUnit.cpp` `:60` / `:445-453` / assignments `:455`/`:459` / `:526` / `:565`.
3. **Drift arithmetic against that baseline, at the current file:** pre-hunk anchors **unmoved** — `CommanderNpc.cpp:301-310` (rung 1, content matching WARN-2's citation), `:315`, `:330`, `:342-350`, `cpp:162` (`ABP_Footman` comment), `h:342`/`h:354`/`h:360` (header untouched); `SummonedUnit.cpp:57`, `:60` (byte-identical `SharedLocomotionAbpPath` declaration), `:445-453` (ends *"…never a crash (matches the soft-ref discipline above)."* — byte-consistent with both the handoff's BEFORE and R7's description), `SummonedUnit.h:118` (still `: public ACharacter, public ITeamAgent, public IHealthBarProvider`). Post-hunk anchors **all exactly +14**: `MeshSkeleton` `:358→:372` · play calls `:375/:376→:389/:390` · `ResolveWarTableMesh` `:383→:397` · `AbpPath` `:454→:468` · assignments `:455/:459→:469/:473` · `SetAnimInstanceClass` `:464→:478` · `PlayAnimation` sites `:526/:565→:540/:579`. **Seven pre-hunk and eight post-hunk anchors, two files, zero exceptions.**

⚠️ **The residual, stated honestly:** this instrument cannot exclude a same-line-count in-place edit that dodges every recorded anchor. **The byte-level backstop is build-master's commit-time diff at TASK-608** (the same division `qa/TASK-592.md` criterion (1) used, `SC-§29b`): the staged diff for these two paths must show **exactly two hunks, +35/−7** — a third hunk or a third file chargeable to this task is a write-race finding, ⛔ not a formatting detail.

✅ **On this evidence: no executable, preprocessor, signature, `UPROPERTY`, or metadata change exists in this diff.** The comment-only claim is CONFIRMED to the strongest form available to a tool-less gate.

---

## 2. SPEC ITEMS (1)–(4), EACH AT THE ARTIFACT

| item | verdict | what I read |
|---|---|---|
| **(1)** WARN-1 — name `IsCompatibleMesh` + why DECLINED | ✅ **DONE, and the content is TRUE** (§3) | New `:351-371` names **both** engine APIs, the availability of each, the DECLINED reason for each, and closes with the do-not-"complete"-this-guard tripwire — exactly the *"declined option with its reason is a decision"* shape the spec ordered |
| **(2)** WARN-2 — diagnose-first; structural guard or say so | ✅ **DIAGNOSED; NO EDIT; PROSE BYTE-IDENTICAL — the spec's own conditional taken** | No hunk before `:351`; rung 1 `:301-310` unmoved, content matches WARN-2's citation. The diagnosis (handoff §4) is sound: a compiled ABP's graph node set is not runtime-queryable (editor-side authoring data, stripped in cooked builds; the generated class exposes no owner-assumption flag), a marker-base `IsChildOf` asserts intent not behaviour, and any such guard would be a code change item (4) forbids. Consistent with `SC-§35` item 5: the instrument is PIE, and no assignment-time check can front-run it. See NIT-2 for one qualifier |
| **(3)** the owner-class contract at `SummonedUnit.cpp` | ✅ **DONE, at the assignment, covering BOTH sites** | Inserted block sits between the resolution-order paragraph and the first assignment line, immediately above `:469` (composed — the copy-target) and `:473` (shared fallback) ⇒ `SC-§35` item 1's letter (*"in a comment at the assignment"*) is met. Names the Pawn assumption, the `TryGetPawnOwner` mechanism, `ASummonedUnit : ACharacter` ⇒ Pawn ⇒ **this assignment is CORRECT**, the 1,806-error defect, and repair-at-the-consumer (items 2+3). `ABP_Footman` asset ⛔ untouched; `Commander` in `SummonedUnit.{h,cpp}` → **0 (my count)** — the fence R7 set is intact |
| **(4)** what the task may not do | ✅ **HELD** | §1: nothing executable, nothing UHT-visible, no `.h`, no `.uasset`, no third file, both hunks inside function bodies. No compile/git/editor claimed or evidenced |

**Names-block conformance:** both edits land inside the spec's named regions located **by symbol** (`SC-§18c` — the spec's `:351-357` / `:445-455` offsets were as-of-authoring and have rotted exactly as that law predicts).

---

## 3. SYMBOL VERIFICATION — EVERY CITED NAME RE-CHECKED AT ITS SOURCE, NOT FROM THE HANDOFF

This task exists because a wrong name in prose replicates (`ResolveHeroStart`, 1→6 sites). So every name the new comments write was re-verified **first-hand this session**:

| claim in the new comment text | my verification |
|---|---|
| `USkeleton::IsCompatibleForEditor` — all three overloads inside `#if WITH_EDITORONLY_DATA` | ✅ `Skeleton.h:702` / `:707` / `:712`, inside the block opened `:651`, closed `:722`, **no nested directive between** (read contiguously). 📌 A second, separate `WITH_EDITORONLY_DATA` block `:726-732` holds only the compatibility delegate — `IsCompatibleMesh` is outside **both** |
| `USkeleton::IsCompatibleMesh` — runtime-available | ✅ `Skeleton.h:766`: `ENGINE_API bool IsCompatibleMesh(const USkinnedAsset* InSkinnedAsset, bool bDoParentChainCheck=true) const;` — after the unguarded `public:` at `:734`, outside every editor-only block |
| "PERMISSIVE (bone-name matching plus an optional parent-chain walk against a percentage threshold — read at the engine source, not assumed)" | ✅ **TRUE at both layers**: header doc `:753-765` (*"bone hierarchy matches … parent chain matches … more than 50 % of bones matches"*, `bDoParentChainCheck` optional) **and** the impl at **`Skeleton.cpp:648`** with the engine's own comment `:651` *"at least % of bone should match"* — the handoff's cited location is exact |
| "`IdleSequence->GetSkeleton()->IsCompatibleMesh(AvatarSkeletalMesh)` would compile in a shipping build" | ✅ Sound: `USkeletalMesh : public USkinnedAsset` (`SkeletalMesh.h:439`) ⇒ the pointer upcasts; `UAnimationAsset::GetSkeleton()` runtime-available (established at `qa/TASK-592.md` §4's deprecation sweep); locals `IdleSequence`/`AvatarSkeletalMesh` exist in the enclosing scope (`CommanderNpc.cpp:330` / `:315`) |
| `UAnimInstance::TryGetPawnOwner()` returns None on a non-Pawn owner | ✅ `AnimInstance.h:465`: `ENGINE_API virtual APawn* TryGetPawnOwner() const;` — the return-type mechanism matches; the None-on-non-Pawn behaviour is `SC-§35`'s measured defect record |
| `ASummonedUnit` is an `ACharacter` ⇒ a Pawn | ✅ `SummonedUnit.h:118`, read in place |
| `SK_Footman_Skeleton` / `ABP_Footman` / `ABP_<CardID>` / `Set GroundSpeed` / `Set bIsMoving` / `qa/TASK-592.md` WARN-1·R7 / `SC-§35` | ✅ Conform to CONVENTIONS, the `SummonedUnit.cpp:60` path literal, `SC-§35`'s defect record, and the existing report/section names |

---

## 4. THE MECHANICAL COUNTS — MY OWN GREPS, WITH THE §14 POSITIVE CONTROL

| grep | handoff claim | **my count** | ✅ |
|---|---|---|---|
| `ResolveHeroStart` in `Source/` | 0 | **0** — repo-wide hits exist **only** in pipeline docs (`CONVENTIONS`/`TASKBOARD`/`qa/`/`handoffs/`), where they are the historical record of the phantom, not sites | ✅ **ZERO SITES** |
| **positive control (§14):** `GetHeroStartTransform` in `Source/` | — | **10+ hits incl. the definition `SiegeGameMode.cpp:642` and declaration `SiegeGameMode.h:422`** ⇒ the instrument demonstrably finds symbols in this corpus; the zero above is evidence, not a tool artifact | ✅ |
| `IsCompatibleMesh` in `Source/` | 3× in `CommanderNpc.cpp`, comments only | **3** — `:357`, `:359`, `:370`, all `//` lines, no other file | ✅ |
| `IsCompatibleForEditor` in `Source/` | 1× | **1** — `:354`, comment | ✅ |
| `TryGetPawnOwner` in `SummonedUnit.cpp` | 1×, comment | **1** — `:459`; the other two repo hits (`CommanderNpc.cpp:33`, `CommanderNpc.h:342`) are pre-existing TASK-591 doc text, untouched | ✅ |
| `ABP_Footman` in `SummonedUnit.cpp` | +4 prose hits in the inserted block | **7 total = 3 pre-existing (`:57`, `:60`, `:448`) + 4 new (`:457`, `:458`, `:461`, `:467`), all four inside the inserted block, all comment prose, zero new code lines** | ✅ delta exact — ⚠️ see WARN-1 on the handoff's enumeration of the *pre-existing* three |
| `ABP_Footman` in `CommanderNpc.{h,cpp}` | unchanged at 4 | **4** — `cpp:31`, `cpp:162`, `h:354`, `h:360` — the identical four `qa/TASK-592.md` recorded | ✅ |
| `Commander` in `SummonedUnit.{h,cpp}` | still 0 | **0 + 0** | ✅ |

**Zone A:** neither file is in `MeasuredCharCount`'s input surface and the diff adds no `TEXT(` (both hunks read; count 0) ⇒ **5658 cannot have moved**, by construction.

---

## 5. THE DECLARED SC-§15 DEPARTURE — ADJUDICATED

**D1 (the `WITH_EDITORONLY_DATA` correction inside the re-authored WARN-1 sentence): ✅ RATIFIED.** The mechanism is checkable without trusting the refuser — `Skeleton.h:651` reads `#if WITH_EDITORONLY_DATA`, verified first-hand (§3) — and the departure is **declared, not silent**. `qa/TASK-592.md` NIT-1 ruled "No action" on the *standing* text; this task **re-authored** that exact sentence, and writing a known-wrong engine token into fresh comment text would recommit the precise failure mode (a wrong name in prose replicating) that TASK-596 descends from. Correcting it in passing, inside text already being rewritten, at zero marginal diff, is the departure law's exact intended shape. **Substance identical either way (absent from a non-editor build); behaviour implication zero.**

**No undeclared departure found:** item (2) produced no edit by the spec's own conditional, and both edits sit inside the named regions.

---

## 6. FINDINGS

- **[WARN-1]** `handoffs/TASK-596-programmer.md` §7 — **the enumeration of the PRE-EXISTING `ABP_Footman` hits in `SummonedUnit.cpp` is wrong in one slot, and it is a §14 trap for a downstream verifier.** The handoff says the prior hits are *"the `:60`-declaration/`:473`-use/`:448`-comment"* — ⛔ **the `:473` code line (`SharedAbpSoft{ FSoftObjectPath(FString(SharedLocomotionAbpPath)) }`) contains no `ABP_Footman` literal at all** (it reaches the path through the `:60` constant, which is the whole point of the constant). The actual third pre-existing hit is the **`:57` comment**. A reviewer who greps `:473` to confirm the handoff gets a negative and could file a false finding — the exact shape §14 warns about, in the handoff of the very task about name precision. ✅ **The load-bearing claim survives intact and is verified by my own count: 3 pre-existing + 4 new = 7, all four new hits inside the inserted block, zero code lines gained the string.** ⛔ **No code change required; the corrected enumeration above is the record.** Fix/owner: none owed — this report supersedes the handoff's parenthetical.
- **[NIT-1]** handoff §4 — the sentence *"rung 1 is unreachable on every shipped path, so the line could never fire — an unfireable log is thoroughness theater"* carries its truth in the qualifier **"shipped"** and then drops it. WARN-2's suggested rung-1 `Warning` **would** fire in the future-misconfiguration world (a designer setting `AvatarAnimClassAsset`) — that tripwire value was WARN-2's point, and "could never fire" must not be read as a claim about that world. ✅ **The outcome is unaffected and correct on the decisive grounds the same sentence also states:** adding the line is a **code change**, forbidden by spec item (4), and the spec's own conditional ordered *"say so and leave the prose."* Recorded so the diagnosis is quoted with its qualifier.
- **[NIT-2]** handoff — the verbatim M8 declaration sentence is absent. **Supplied here so the record is explicit rather than implied: this diff adds no replicated property, no new replicated class, and no new relevancy tier — trivially, since it emits zero bytes.** A comment-only diff cannot move replication; noted, not charged.
- **[NIT-3]** `qa/TASK-592.md` WARN-1 (**pre-existing, out of diff, ⛔ not charged to TASK-596**) internally says the editor-only block *"closes at `:732`"* while its own §3 says `:722`. My read: the `IsCompatibleForEditor` block is **`:651-722`**; `:726-732` is a **separate** `WITH_EDITORONLY_DATA` block holding only the compatibility delegate; `IsCompatibleMesh` `:766` is outside both. ✅ **The shipped comment text is precise on this and inherits neither error.**

---

## 7. WHAT THIS PASS DOES AND DOES NOT CERTIFY

✅ Certified: the diff is comment text only (to §1's stated instrument limit); every name it writes exists where claimed; the phantom is at zero sites with a live positive control; the fences (`rung 1`, both files' code lines, `CommanderNpc.h`, `SummonedUnit.h`, `ABP_Footman` the asset) are undisturbed; the SC-§15 departure is ratified; spec items (1)–(4) are each satisfied.
⛔ Not certified: runtime behaviour — nothing here needed it (zero emitted bytes) and nothing here observed it. ⛔ This verdict does not extend to any other diff riding TASK-606's compile (`SC-§27b`: a build covers the tree it built; this gate covers this diff).

## Notes for build-master (PASS → TASK-608)

1. ⭐ **The byte-level reconciliation is yours at commit time:** the staged diff for `Source/GitClaudeUnrealTest/Siegebound/CommanderNpc.cpp` + `SummonedUnit.cpp` must show **exactly two hunks, +35/−7**, both pure comment. A third hunk/file chargeable to TASK-596 = write-race finding, stop and report.
2. This diff **rides TASK-606's `Build.bat`** — it owes its named line in the ACCOUNTS batch's gate ledger (pre-flight row 3). A comment-only diff cannot change a compiled byte, but the ledger line is owed regardless.
3. ⛔ Parse the build log for `Result:` — never `$LASTEXITCODE` (standing machine law).
4. **No `.gen.cpp` movement is attributable to this diff** (both hunks inside function bodies; nothing UHT-visible).
5. Zone A **5658** unaffected by this diff (no `TEXT(`, neither file in the measured surface) — re-confirm, don't re-derive.
6. M8: nothing replicated moved (NIT-2's declaration).

## Board status flips (⛔ qa-reviewer has no partial-edit tool — orchestrator to proxy)

- **TASK-596** → `qa-passed / ready-for-integration ← ✅ SC-§27 diff-scoped QA PASSED 2026-08-16 — qa/TASK-596.md: 0 BLOCKER, 1 WARN (handoff enumeration slip, corrected in the report), 3 NIT. Comment-only claim CONFIRMED first-hand; all cited symbols verified at engine + codebase; phantom at ZERO sites with positive control; SC-§15 D1 RATIFIED. Rides TASK-606's compile; gates TASK-608's commit, which owes the byte-level two-hunk reconciliation (report §7 note 1).`
