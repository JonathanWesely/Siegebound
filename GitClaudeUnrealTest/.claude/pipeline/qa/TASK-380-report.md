# QA Report — TASK-380 (pre-compile review of TASK-379)

**Verdict: PASS** — **0 BLOCKER · 2 WARN · 3 NIT.** Cleared for **TASK-389** (compile + commit).
**Date:** 2026-08-02 · **Reviewer:** qa-reviewer · **Under review:** `handoffs/TASK-379-programmer.md`
**Files:** `Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.h` · `…/SiegeCheatManager.cpp` · `…/DeckBuilderWidget.cpp`
**Authority:** TASKBOARD `#### TASK-379` / `#### TASK-380` · `qa/TASK-365-report.md` "THE SIMPLIFICATION VERDICT" · CONVENTIONS "Ancient Grounds + Sorcerer" §4/§6/§8

> ⚠️ **FILENAME.** TASK-380's spec names the report `qa/TASK-380.md`. **This file — `qa/TASK-380-report.md` — is the
> authoritative one** (same deviation TASK-365 recorded). Append any build errors HERE.

**No Git in this review by design** — every claim below was verified by reading the files on disk, not by diffing.
Where a claim is only provable with `git diff`, it is called out explicitly and handed to build-master.

---

## THE TWO SPEC PROBLEMS — BOTH RULED, NEITHER DEFERRED

### RULING 1 — the ACCEPTANCE clause is a **SPEC DEFECT**. The implementer's reading is **UPHELD**; the code stands.

TASK-379's ACCEPTANCE says *"the card text renders the identical STRING today"*. **That clause cannot be satisfied
together with the task it belongs to, and the task is the part that is right.** Three independent statements in the
same authority chain say the Sorcerer's string is *expected* to change:

1. TASK-380 criterion **(d)** — *"no **other** card's description changes by one character"*. The word "other" is
   only meaningful if this card's does.
2. TASK-379 requires the handoff to quote the panel *"so QA can **diff** it against TASK-364's"*. A diff presupposes
   a difference.
3. `qa/TASK-365-report.md`'s SIMPLIFICATION VERDICT — the named authority, which the spec itself says is *"more
   precise than this summary"* — states the interpolation *"(⇒ "+5%")"* and *"(⇒ "+400%")"* in so many words.

An interpolation that reproduced today's number-free string would be a **no-op**, and the task would have no purpose.
The clause is a drafting error: it was carried over from the "no behavior change" framing that correctly governs
parts (1) and (2) and does not survive contact with part (3).

**Amended acceptance, as I read it and as I have tested against (manager: fold this back into the board):**
*"No behavior change anywhere. The composed Sorcerer panel is byte-identical to the panel TASK-364 shipped
**except for the two derived magnitudes this task exists to insert**; no other card's description changes by one
character."*

**Verified against that standard — independently, using TASK-364's own handoff (`handoffs/TASK-364-programmer.md:26`)
as the baseline rather than the implementer's summary. The delta is exactly two insertions, zero re-wording:**

| # | TASK-364 (shipped) | TASK-379 (now) |
|---|---|---|
| 1 | `…in that same ground hits harder for each second…` | `…in that same ground hits **+5%** harder for each second…` |
| 2 | `…second after second to a hard ceiling. A second sorcerer…` | `…second after second to a hard ceiling **of +400%**. A second sorcerer…` |

Every other character of both rule lines matches `handoffs/TASK-364-programmer.md:25-26` — I compared clause by
clause, including the hyphen style, the `SorcererRole` line (`DeckBuilderWidget.cpp:55`, byte-identical), and the
final `Units that never attack - miners, healers and sorcerers themselves - gain nothing.` sentence. The identity
line, both stat lines, the U+00B7 separator and the `\n\n`/`\n` joins are produced by untouched code paths
(`IdentitySeparator()` :202-205, `AppendStatLines` :800-866).

**The minimisation was the right call.** Re-wording the sentence to accommodate the number would have re-opened
every truth-law clause TASK-365 §(I) already verified; inserting into the existing clauses keeps that verification
valid, which is why I can pass this without re-litigating TASK-364.

### RULING 2 — the spec's deletion list WAS incomplete. Baking the name into the literal is **CORRECT and ACCEPTED**.

The third reference (`*SiegeCheatPerStackPropertyName.ToString()` feeding a `%s` at the `SkippedCount` warning) was
not in the spec's list. The implementer's choice is the only one that actually completes the deletion — keeping an
`FName` constant alive purely to feed one `Warning` log would have preserved the exact dead weight the task exists
to remove. **Accepted.**

**Verified now at `SiegeCheatManager.cpp:549-551`:**

```cpp
UE_LOG(LogGitClaudeUnrealTest, Warning,
    TEXT("USiegeCheatManager::SetTestDamageBoost — skipped %d unit(s) whose PermanentDamageBonusPerStack is <= 0 (percent-to-stacks is undefined there)."),
    SkippedCount);
```

- Format-specifier count (`%d`, one) == argument count (one). ✓ No dangling `%s`, no orphan argument — the exact
  failure mode that turns a log line into a crash.
- The baked token `PermanentDamageBonusPerStack` matches the property's authored spelling at `SummonedUnit.h:686`
  **character-for-character** (I compared them directly). `FName::ToString()` preserves authored display case, so the
  substitution is faithful.
- **Limit of my verification, stated plainly:** the *surrounding prose* of the old format string cannot be diffed
  without Git, which I do not have. I verified the substituted token, the specifier/argument arithmetic, and that no
  other reference survives. The residual is one Warning-only line with no behavioral consumer → **NIT-3**, closable by
  build-master with one `git diff` hunk.

---

## NAMED CRITERIA — VERIFIED

### (a) Both getters are genuinely `public:` — **VERIFIED MYSELF, not taken from the handoff**

This is the criterion the batch already failed once, so I re-derived the block boundaries from scratch:

- Access specifiers in `SummonedUnit.h`, whole file: **exactly three** — `121:public:` · `484:protected:` · `745:private:`.
- Class body: `UCLASS()` `:116` → `class GITCLAUDEUNREALTEST_API ASummonedUnit : …` `:117` → `{` `:118` →
  `GENERATED_BODY()` `:119` → closing `};` **`:1399`**. The only other `};` in the file is `:44` (the
  `ESummonedUnitState` enum, *before* the class). **There is no nested class/struct/union inside the class body**, so
  nothing can re-open or re-scope an access region between `:121` and `:484`.
- The getters sit at **`:466-467`** and **`:481-482`** → strictly inside `121 … 484` → **`public:`**. ✓
- They are nowhere near the `protected:` trap (`ShouldHoldDeathAnim()`; the `:370` comment marks that region).
- **Shape:** character-for-character with the pinned block (`UFUNCTION(BlueprintPure, Category = "Siegebound|Unit")`
  + the one-line inline body), tab-indented in house style, placed immediately after `GetPermanentDamageMultiplier()`
  (`:447-448`) exactly as specced.
- **Types agree with the properties:** `float PermanentDamageBonusPerStack = 0.05f` (`:686`) → `float` getter;
  `int32 MaxPermanentDamageStacks = 80` (`:696`) → `int32` getter. Both `const`, both inline → **no `.cpp` definition
  needed and no link surface**.
- **UHT safety:** module-wide grep — neither name appears anywhere except the declaration and the two call sites. No
  collision with a reflected member of `ACharacter`/`AActor`/`ITeamAgent`/`IHealthBarProvider`, no duplicate
  Blueprint-exposed name in the hierarchy, no shadowing. `BlueprintPure` + `const` is satisfied (a non-const
  `BlueprintPure` is a UHT error; both are const).
- **`SummonedUnit.cpp` untouched — corroborated:** zero occurrences of either getter name and **zero `TASK-379`
  markers** in that file (the module's three `TASK-379` tags land only in the three files under review). TASK-396 and
  TASK-397 get a clean `.cpp`. *Definitive proof is `git diff --name-only`; see build-master note 1.*

### (b) `SiegeCheatManager.cpp` — reflection retired, nothing else moved

- **Include gone:** the list at `:3-18` contains **no `UObject/UnrealType.h`**; `Siegebound/SummonedUnit.h` is still
  present at `:17` (required by the new member call). No header gained an include.
- **Constant + block gone, no dangling reference:** module-wide grep for
  `SiegeCheatPerStackPropertyName|CastField|FindPropertyByName|FFloatProperty|UnrealType\.h` returns **exactly one
  hit** — `SiegeCheatManager.cpp:484`, inside the comment that *records* the deletion. **Zero code references.**
  Accepted on the precedent this office set in `qa/TASK-365-report.md` §(G) (a retired symbol may survive in
  explanatory prose).
- **⚠️ THE READ IS PER-INSTANCE, NOT A CDO READ — verified by position:** the range-for opens at `:498`
  (`for (ASummonedUnit* Unit : Targets)`) and `const float PerStack = Unit->GetPermanentDamageBonusPerStack();` is at
  **`:500`, inside it**. A per-Blueprint override is honoured exactly as it was under reflection. The `PerStack <= 0`
  skip + `++SkippedCount` (`:501-505`) is unchanged.
- **⚠️ THE CEIL + `1e-4` EPSILON ARITHMETIC IS UNTOUCHED — confirmed, and I re-ran the ruling-R9 case:**
  `SiegeCheatStackEpsilon = 1.e-4` (`:51`) with its full sizing rationale (`:34-50`) and
  `SiegeCheatMaxRequestableStacks` (`:32`) are intact; `RawStacks` (`:519`) / `ClampedStacks` (`:520`) /
  `CeilToInt64` (`:521`) are intact, still in `double`, still round **UP**.
  `101` ⇒ 101/100/0.05f = 20.2 − 1e-4 → **ceil 21 → 105%**; `100` ⇒ 19.99999… − 1e-4 → **ceil 20 → exactly 100%**.
  **Jonathan's most important gate row remains performable.** No round-to-nearest crept in.
- **§6 routing intact — never a raw field write:** `Unit->ClearPermanentDamageStacks();` (`:530`) then
  `if (Stacks > 0) Unit->AddPermanentDamageStacks(Stacks);` (`:531-534`), with the "THE ORDER IS LOAD-BEARING"
  rationale unchanged. No assignment to any boost field exists anywhere in the file.
- **Untouched behavior confirmed by reading, not assumed:** the `!PC` / `!World` / `!IsFinite(Percent)` guards
  (`:386-405`), team resolve (`:407-408`), targeting (`:410-452`), the empty-target warning (`:454-460`), the
  `Percent <= 0 ⇒ clear only` early return (`:462-476` — the *code* did not move; only the comment's stated reason
  changed, correctly, since the "property cannot be resolved" failure mode no longer exists), the read-back logs
  (`:559-570`).
- **Include-removal risk swept:** everything the TU still uses is either explicitly included or comes via
  `CoreMinimal` — `TNumericLimits` (`:495-496`), `FMath::CeilToInt64`, `Cast<>`, `GetNameSafe`. I found **no
  transitive-only dependency on `UObject/UnrealType.h`**. (Unity builds could mask such a thing; there is nothing here
  to mask.)
- The `SiegeCheatManager.h` doc (`:97`) — *"uses each unit's OWN `PermanentDamageBonusPerStack`"* — is still exactly
  true, and the header contains no reflection language. Correctly left untouched.

### (c) `DeckBuilderWidget.cpp` — §8 TRUTH LAW held, and the format string cannot emit a malformed number

- **`%` escaping — counted directly off `:82`:** exactly **two `%s`** and **two `%%`** (`+%s%%` … `of +%s%%.`), and
  **no other `%` anywhere in the literal**. Two arguments passed at `:912-913`. Specifier count == argument count. ✓
  `%%` → one literal `%`; precedent in the same namespace: `SpellAllyBuffFmt` (`:146`, `50%% faster`).
- **Printf shape is compile-safe:** `constexpr TCHAR SorcererGroundBoostFmt[]` passed to `FString::Printf` satisfies
  the `TIsArrayOrRefOfTypeByPredicate<…>` static_assert (it is a TCHAR *array*, not a `const TCHAR*`). Identical shape
  to the shipped `FString::Printf(SiegeboundCardGlossary::UpgradeTailFmt, Row.MaxCopies)` at `:942` in the same TU.
  The format string is a named `TEXT()` literal and is never computed ⇒ **the TASK-268 `TCheckedFormatString`/C7595
  trap does not apply.** The `const TCHAR[]` → `constexpr TCHAR[]` storage change matches the file's own Fmt-vs-plain
  convention and keeps internal linkage.
- **Arithmetic re-derived from first principles (I did not take the handoff's word):**
  `0.05f` = `0.0500000007450580596923828125`. `100.f × 0.05f` = `5.00000007450580596923828125` exactly; the ULP at
  5.0 is `2^-21 ≈ 4.768e-7`, so half a ULP is `2.384e-7`, and the excess over 5.0 is `7.45e-8` — **inside half a ULP
  ⇒ rounds to exactly `5.0f`.** Then `5.0f × 80.0f` = **exactly `400.0f`** (both exactly representable).
  `FormatStatValue` (`:208-215`) takes its `IsNearlyEqual(V, RoundToFloat(V))` branch on both and prints `%d` of
  `RoundToInt` ⇒ **"5"** and **"400"**. No decimal tail, no `5.000000`, no scientific notation. The `100.f`-first
  operand order matches `ASummonedUnit::GetDamageBoostPercent()` (`SummonedUnit.h:154`) — the order this office
  already proved exact.
- **Robust under retune (this matters more than today's exactness, because both are FLAGGED levers):**
  `IsNearlyEqual`'s `KINDA_SMALL_NUMBER` (1e-4) tolerance absorbs residual float error at other tunings; a genuinely
  fractional lever (0.075 ⇒ 7.5) takes the `%.1f` branch and renders "7.5" cleanly. Negatives are excluded by
  `meta = (ClampMin = "0")` on both properties. **No tuning in any sane range can emit a malformed number.**
- **No magnitude is baked as a literal.** Both come from the getters (`:908-909`); the only numeric literals in the
  new code are the `100.f` unit conversion and the cast. CONVENTIONS §8's **PREFERRED** branch is now satisfied and
  the drift risk is gone.
- **TRUTH LAW, the two new claims re-verified against code on disk (implementer's question 4 — RULED CORRECT):**
  `AAncientGround::BoostTickInterval = 1.0f` (`AncientGround.h:161`) and `Grant = SorcererCount[own team]` per tick
  (`AncientGround.cpp:223`, one stack per friendly sorcerer per tick — CONVENTIONS §4 line 274) ⇒ **"+5% … for each
  second" is true for ONE sorcerer**, and the *untouched* following sentence ("A second sorcerer … builds it twice as
  fast") carries the multi-sorcerer case exactly as TASK-364 shipped it. **The insertion points are the right
  clauses.** Ceiling: `80 × 5% = 400%`, clamped in `AddPermanentDamageStacks` (`SummonedUnit.cpp:810`). "Hits harder"
  is still damage-only. See WARN-1 for the one residual coupling.
- **The unguarded `GetDefault<ASummonedUnit>()` (`:907`) — ACCEPTED.** `GetDefault<T>()` on a statically-linked
  `UCLASS` cannot return null, and the project already ships this exact unguarded pattern:
  `SiegeGameMode.cpp:977`/`:1014` and `SpellLineSweep` reads at `SpellLibrary.cpp:132`/`:174`. Not deprecated in
  UE 5.8. The CDO-here / instance-there split is deliberate and documented, and it is the correct split (see NIT-2).

### (d) No other card's description changes

- The only glossary constant altered is the Sorcerer's; `SorcererGroundBoost` no longer exists under its old name and
  the only reference to the new `…Fmt` is the Sorcerer branch (`:912`). The other 25+ constants (`:45-174`) are intact
  with their `// mirrors` comments, and the `GlossaryCardID_*` block (`:185-192`) is unchanged.
- Every other branch of `AppendRuleLines` and all of `AppendStatLines` is structurally untouched.
- **Byte-level proof for the other cards needs Git** → build-master note 2. Nothing I can read suggests otherwise.

### (e) Standing pre-compile sweeps

- **Shadowing (C4457/8/9 = hard errors here):** the three new locals — `UnitCDO`, `PerStackPercent`, `CeilingPercent`
  — appear in `DeckBuilderWidget.cpp` **only** at `:907-913`. No collision with the enclosing function's parameters
  (`CardID`, `Row`, `OutLines`), with a `UDeckBuilderWidget`/`UUserWidget` member, or with a file-scope name. The
  `UnitCDO` hits in `Barracks.cpp:126` and `SiegePlayerController.cpp:3637` are other TUs and other functions —
  irrelevant. The cheat-manager change **removed** a local and added none. `SummonedUnit.h` added no variables.
- **Complete-type include law:** `DeckBuilderWidget.cpp` gained `Siegebound/SummonedUnit.h` (`:13`) — required for
  `GetDefault<ASummonedUnit>()` and both member calls. `SiegeCheatManager.cpp` retains it at `:17`. Correct, minimal,
  `.cpp`-only.
- **Deprecated UE 5.8 APIs:** none introduced. `GetDefault<T>`, `FString::Printf`, `FMath::IsNearlyEqual/RoundToFloat/
  RoundToInt`, `UFUNCTION(BlueprintPure)` are all current. Nothing removed in 5.8 is used.
- **Null-safety:** one new dereference (`GetDefault`, provably non-null, precedent-backed). `Unit` in the cheat loop
  comes from an already-validated target list; the getter is a pure member read and adds no new failure mode — it
  **removes** one (the "refusing to guess" path can no longer trigger).
- **`GetFirstPlayerController()`** — not used, not added, in any of the three files.
- **No new tick/timer/poll/allocation.** All three getters are inline pure reads; the widget work runs once per
  card-details composition, as before. No `FindObject`/`LoadObject` in any hot path.
- **Comment hygiene:** no literal `*/` inside any new doc comment; the `⚠️`/`✅`/`×`/`§` characters are UTF-8 in
  *comments* only, matching the established, already-compiled precedent (`AncientGround.h:196`). **All new
  player-facing string literals are pure ASCII** — verified on `:82`.
- **Conventions:** getter names follow `Get<PropertyName>()`, category `Siegebound|Unit` matches the neighbouring
  getter, and no asset path is involved in this task.

---

## Findings

- **[WARN] `DeckBuilderWidget.cpp:82` — the sentence now pairs an EXACT magnitude with an un-interpolated CADENCE.**
  `+5%` is derived, but *"for each second"* still hardcodes `AAncientGround::BoostTickInterval = 1.0f` — **the third
  FLAGGED §4 balance lever**, and the one lever this task did *not* put on §8's preferred branch. This is not a
  regression (the phrase is TASK-364's and the mirrors-comment at `:58` names `BoostTickInterval`), but the failure
  mode got sharper: *"hits harder … each second"* degraded gracefully under a tick retune, whereas *"+5% … each
  second"* becomes a **precisely falsifiable false claim** the moment the tick moves off 1 Hz.
  *Suggested fix (manager follow-up, NOT this task, no code change now):* either add the rate to the interpolation
  (needs an `AAncientGround` getter and couples the widget to that class — probably not worth it), or record in
  CONVENTIONS §4 that **retuning `BoostTickInterval` requires editing `SorcererGroundBoostFmt`'s "each second"
  clause.** Cheap, and it closes the last drift hole in this card.

- **[WARN] Cross-task — `SummonedUnit.h`'s new block vs the in-flight TASK-396/397 seam (link-time class of bug).**
  TASK-379 adds a contiguous 34-line block ending at `:483`, immediately before `protected:` at `:484`. **Today there
  is no collision:** `SummonedUnit.h` carries **zero `TASK-39x` markers** (TASK-397 landed only in `MinerUnit.{h,cpp}`
  and deliberately deferred its base-class one-liner — see `MinerUnit.cpp:488`), and `SiegePlayerController.*` /
  `UnitCommand.h` (TASK-395) are disjoint from all three files here. **The risk is forward:** if TASK-396/397 later
  inserts near the same anchor, or inserts an access specifier between `:121` and the getters, the getters silently
  become non-public and every call site fails to link — the exact failure this task exists to retire.
  *Suggested fix:* build-master re-runs the three-line access-specifier grep (below) **after** any TASK-396/397 header
  edit lands and **before** compiling. It costs three seconds.

- **[NIT] `SummonedUnit.h` line drift +34 — three pipeline documents now cite stale lines.** The properties moved
  `652 → 686` and `662 → 696`, and the `protected:` block opens at `484`, not `450`. Stale citations live in
  `qa/TASK-365-report.md:413` (R12), `handoffs/TASK-364-programmer.md:79`, `handoffs/TASK-360-programmer.md:60-61`
  and `TASKBOARD.md:2301`. **No stale citation survives in code** — I grepped `Source/` for `SummonedUnit.h:6xx` and
  found none; `DeckBuilderWidget.cpp`'s old comment was correctly rewritten. Documents only; recorded so the next
  reader does not chase them.

- **[NIT] `SiegeCheatManager.cpp:550` — the re-baked warning's byte-identity is verified only to the token level.**
  See RULING 2. Everything checkable pre-compile checks out; the surrounding prose needs one `git diff` hunk.

- **[NIT] `DeckBuilderWidget.cpp:907` — the card states the CLASS DEFAULT, so a per-Blueprint override would make it
  wrong for that unit** while the gameplay and cheat paths (correctly) honour the override. For a menu screen with no
  unit in the world the CDO is the only available *and* the right authority, it is the value Jonathan edits when he
  retunes, and the asymmetry is documented in-comment at `:900-906`. No C++ override exists; a `.uasset`-level sweep
  of `BP_Unit_*` is out of scope pre-compile. **Accepted as designed** — recorded so it is not "discovered" later.

---

## Notes for build-master (TASK-389)

1. **Confirm the touch-set with Git before compiling** — the two claims I could not verify without it:
   `git diff --name-only` must list **exactly three files**: `SummonedUnit.h`, `SiegeCheatManager.cpp`,
   `DeckBuilderWidget.cpp`. **`SummonedUnit.cpp` must NOT appear** (TASK-396/397 need it clean), and neither may
   `SiegeCheatManager.h`, `DeckBuilderWidget.h`, `SiegePlayerController.*`, `MinerUnit.*` or `UnitCommand.h` on
   *this* task's account.
2. **`git diff -- Source/GitClaudeUnrealTest/Siegebound/DeckBuilderWidget.cpp` should be confined to four regions:**
   the new include (`:13`), the file-header GLOSSARY-MIRROR block, the Sorcerer constant + its doc (`:57-82`), and the
   Sorcerer branch (`:900-913`). Anything touching another glossary constant would break criterion (d) — that is the
   only part of (d) I could not close by reading.
3. **One-line closure for NIT-3:** in `git diff -- …/SiegeCheatManager.cpp`, check the `SkippedCount` warning hunk —
   the old `%s` + `*SiegeCheatPerStackPropertyName.ToString()` against the new baked literal. If the prose differs by
   more than the substituted token, tell me; it is log-only, so it is a note, not a rebuild.
4. **Re-verify the access boundary if any other header edit lands first.** Three-second check, and it is the one
   failure this task exists to prevent:
   `rg -n "^\s*(public|protected|private):" Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.h`
   → must return `public:` first, then `protected:` **after** both getters. Today: `121` / `484` / `745`, getters at
   `467` / `482`. **If a specifier ever appears between 121 and the getters, STOP** — every call site will fail to
   link.
5. **Expect a WIDE recompile.** `SummonedUnit.h` is included broadly. Both additions are new *inline* `UFUNCTION`s —
   source-compatible with every existing consumer; no signature changed, nothing removed. UHT will regenerate
   `SummonedUnit.generated.h` with two new BlueprintPure thunks; that is expected, not a defect.
6. **This is a code-only, behavior-preserving change: no editor step, no asset save, no `DT_Cards` edit, no
   `L_Arena` save.** The Sorcerer card panel changes only in what it *renders*, and only by the two magnitudes.
7. **Post-compile smoke (cheap, optional but recommended):** open the deck builder, select **Sorcerer**, confirm the
   second rule line reads `…hits +5% harder…` and `…a hard ceiling of +400%.` — with **no** decimal tail and a single
   `%` in each place. Then `SetTestDamageBoost 101` still ⇒ **21 stacks / 105%** and `SetTestDamageBoost 100` ⇒
   **exactly 100%** (ruling R9's gate rows, unchanged by this task).

---

## Verdict

**PASS — 0 BLOCKERS.** Both flagged spec problems are ruled in the implementer's favour on the merits: RULING 1
finds the ACCEPTANCE clause self-contradictory and amends it (the code is right, the spec sentence is wrong), and
RULING 2 accepts the third deletion as the only choice that completes the task. The access level — the thing that
cost this batch two workarounds — is **verified public by my own reading of the block boundaries**, the per-instance
cheat read and the CEIL+`1e-4` arithmetic are **untouched**, no magnitude is baked, and the panel delta is exactly
the two insertions the task exists to make.

**TASK-379 → `qa-passed`. Cleared for TASK-389. TASK-396's `.cpp` is clean.**
