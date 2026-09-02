# TASK-761 — the three compile diagnostics from TASK-742 — PROGRAMMER HANDOFF

**Status: `ready-for-qa`** · 2026-09-01 · gameplay-programmer
**Suite delta: ⛔ ZERO. The total stays `248`.** (⛔ no test added, ⛔ none removed — measured below.)
⛔ No compile · ⛔ no editor · ⛔ no MCP · ⛔ no Git. The editor is DOWN and I left it down.

**Source read:** `qa/TASK-753.md` (build-master's appended diagnostics, lines 337–417).
⚠️ `handoffs/TASK-742-buildmaster.md` **does not exist** — confirmed absent, consistent with QA §10 and build-master's own note. Nothing was taken from it.

---

## 1. What was broken, and what I changed

Three diagnostics, **two root causes**, both closed at the source. `Result: Failed (OtherCompilationError)` — parsed from the log; the exit code was `0` and lied, exactly as the standing law predicts.

| # | diagnostic | fix |
|---|---|---|
| 1 | `SiegeGhostPawn.cpp(192,46) C2248` | call site re-pointed at a new **public** accessor |
| 2 | `SiegeGhostPawnTest.cpp(580,44) C2248` | same |
| 3 | `SiegeGameMode.cpp(483,15) C4458` | local renamed at **three** sites |

**Four files touched, and only four** (verified by mtime, since the whole wave is uncommitted and `git status` cannot separate my footprint from the batch's):

```
Source/GitClaudeUnrealTest/Siegebound/HeroCharacter.h            ← the ONE accessor only
Source/GitClaudeUnrealTest/Siegebound/SiegeGhostPawn.cpp
Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeGhostPawnTest.cpp
Source/GitClaudeUnrealTest/Siegebound/SiegeGameMode.cpp
```

⭐ **`HeroCharacter.cpp` was NOT edited at all.** The accessor is header-inline, so the `.cpp` needed nothing. The authorised fence crossing was used for less than it allowed.

---

## 2. ROOT CAUSE 1 — the public walk-speed accessor

### The precedent, quoted as shipped

`HeroCharacter.h:460-462`, the HP twin — a public `BlueprintPure` one-liner over an equally-`protected` effective getter:

```cpp
/** EFFECTIVE maximum hit points: base MaxHP (§3.1: 200) + Plate Armor bonus (§3.10). */
UFUNCTION(BlueprintPure, Category = "Siegebound|Hero")
float GetMaxHP() const { return GetEffectiveMaxHP(); }
```

`GetEffectiveMaxHP()` sits at `:620` under the `protected:` opened at `:544` — **the same block, and the same access problem** as `GetEffectiveWalkSpeed()` at `:626`. The HP side already had its public door; walk speed had none.

### The accessor as added

Placed in the same public block, directly after `GetEffectiveMeleeDamage()`, keeping the effective-stat getters contiguous (HP → melee → walk speed):

```cpp
/**
 *  EFFECTIVE base walk speed in u/s: base WalkSpeed (§3.1: 500) scaled by the Swift Boots
 *  bonus (§3.10). The PUBLIC read of the protected GetEffectiveWalkSpeed() — exactly the
 *  GetMaxHP()/GetEffectiveMaxHP() pairing above, for the speed half of the stat block.
 *  ⛔ NOT unused: ASiegeGhostPawn::BeginPlay derives the ghost's MaxWalkSpeed from this on
 *  the CDO (GHOST-§3 G-1 "the hero's OWN speed"), and SiegeGhostPawnTest test (d) asserts it
 *  is > 0. Deleting it as dead code re-duplicates the 500.f literal it exists to prevent.
 */
UFUNCTION(BlueprintPure, Category = "Siegebound|Movement")
float GetWalkSpeed() const { return GetEffectiveWalkSpeed(); }
```

**Shape mirrors `GetMaxHP` exactly:** doc comment → `UFUNCTION(BlueprintPure, Category = ...)` → `float GetX() const { return GetEffectiveX(); }`. `const`, inline, composes nothing extra, adds no state.

⚑ **The one character-level divergence, declared:** `Category = "Siegebound|Movement"` rather than `"Siegebound|Hero"`. The file already varies the category by domain (`GetMaxHP` is `Hero`, `GetEffectiveMeleeDamage` at `:466` is `Combat`), and `Movement` is the category the `WalkSpeed` UPROPERTY itself carries (`:725`). Say the word and it becomes `Hero`.

⚑ **The extra doc lines beyond `GetMaxHP`'s single line** are deliberate: this accessor's only callers are in *another class*, so it reads as dead code from inside `HeroCharacter.h`. The comment is what stops a future tidy-up from deleting it and re-landing this exact compile error.

### Why not `500.f`

⛔ Explicitly not done. `SiegeGhostPawn.cpp:178-187` and `SiegeGhostPawnTest.cpp` test (d) exist **precisely** to avoid that literal — a hard-coded number compiles green while silently discarding the property the test protects. `GetEffectiveWalkSpeed()` stays `protected` and **unmoved**; nothing else in `HeroCharacter.h` changed.

### Both call sites

**(a) `SiegeGhostPawn.cpp:197`** (was `:192`; the surrounding derivation comment block is untouched):

```cpp
const float HeroWalkSpeed = HeroDefaults->GetWalkSpeed();
```

**(b) `Tests/SiegeGhostPawnTest.cpp:583`** (was `:580`):

```cpp
const float HeroWalkSpeed = HeroDefaults->GetWalkSpeed();
```

⭐ **The test still asserts the same property, and now asserts it more precisely:** it reads the *exact same accessor* `BeginPlay` reads. Before, the test called a function the ghost could not legally call — so even had it compiled, it was asserting on a different reachability than production used. Test (d)'s `> 0` assertion, its message and its failure mode are unchanged.

### Verified: no external protected call survives

Every `GetEffectiveWalkSpeed` reference in the module, after the change:

- `HeroCharacter.cpp:775`, `:936` — **inside `AHeroCharacter`**, legal, untouched.
- `HeroCharacter.h:477` — the new wrapper's own body, legal.
- `SiegeControlsHelpWidget.cpp` ×4, `SiegeGhostPawn.cpp` ×2, `SiegeGhostPawnTest.cpp` ×1 — **all comment or string literal text**, not calls.

⇒ **zero external calls to the protected form remain.**

### Name safety

`GetWalkSpeed` collides with nothing: **0 hits** in `GameFramework/Character.h`, `Pawn.h`, `Actor.h` (the full inheritance chain) and **0 prior hits** anywhere in `Source/`. It is not an override, it hides nothing, and UHT has no duplicate reflected name to reject.

---

## 3. ROOT CAUSE 2 — the shadowed `Owner`, renamed at THREE sites

`AController* Owner` shadowed the inherited `AActor::Owner`; UE promotes C4458 to an error.

⚠️⚠️ **THE DISPATCH NAMED TWO SITES. THERE ARE THREE.** The brief (and the build-master diagnosis) named the declaration `:483` and `GetHeroStartTransform(Owner, …)` at `:493`. **`Cast<APlayerController>(Owner)` at `:499` is a third use that neither named.**

⭐ **That third site is the dangerous one, and it is the exact trap the brief warned about in the abstract.** Renaming only the first two leaves `:499` referring to a name that *still resolves* — to `AActor::Owner`, this game mode's own owner, which is **null**. Result: `Cast<APlayerController>(nullptr)` → null → `SetControlRotation` silently never fires, and **it compiles clean**. A recalling hero would arrive home facing the wrong way with no diagnostic anywhere.

All three renamed to **`RecallingController`**:

| site | before | after |
|---|---|---|
| decl (now `:488`) | `AController* Owner = RecallingHero->GetController();` | `AController* RecallingController = RecallingHero->GetController();` |
| now `:498` | `GetHeroStartTransform(Owner, RecallingHero->GetTeamId(), …)` | `GetHeroStartTransform(RecallingController, RecallingHero->GetTeamId(), …)` |
| now `:504` | `if (APlayerController* PC = Cast<APlayerController>(Owner))` | `if (APlayerController* PC = Cast<APlayerController>(RecallingController))` |

A comment at the declaration records that the rename is **all-or-nothing** and why, so the next reader cannot re-introduce a partial one.

**Verified complete:** the only remaining `Owner` token in `SiegeGameMode.cpp` is `SpawnParams.Owner` at `:814` — an unrelated `FActorSpawnParameters` field, ⛔ untouched — plus my own explanatory comment text. **Zero local `Owner` declarations remain.**

⛔ **Nothing else in `HandleHeroRecallArrived` changed.** Still `GetHeroStartTransform` → `SetActorLocationAndRotation(…, TeleportPhysics)` → `SetControlRotation` → one `Log`. ⛔ No `ResetHero()`, ⛔ no possession change, ⛔ no HP write. QA §4 / §7a(a) hold byte-for-byte apart from the identifier.

---

## 4. ⭐ THE SUITE STAYS 248 — measured, not assumed

Re-measured on disk after my edits, line-anchored `^\s*IMPLEMENT_[A-Z_]*AUTOMATION_TEST` across all 20 files in `Siegebound/Tests/`:

```
TOTAL = 248     (unanchored cross-check also 248 ⇒ no prose/string false positives)
```

Per-file, unchanged from QA §10's audited figures — including the two riders:
`SiegeLadderClimbTest.cpp = 14` (TASK-760's declared +1, ⛔ not a regression) · `SiegeClimbableTowerTest.cpp = 10` · `SiegeGhostPawnTest.cpp = 10` · `SiegeWarMapTest.cpp = 35` · `SiegeAssistantSelectionTest.cpp = 38` · `SiegeRecallTest.cpp = 12` · `SiegeMapMarkTest.cpp = 9`.

⛔ **I added no test and removed none.** My change to `SiegeGhostPawnTest.cpp` is *inside* an existing test body — one expression and comment text. **No `IMPLEMENT_*` frame was touched.**

---

## 5. ⭐ THE CHECK I ALMOST MISSED — TASK-748's T12 source scan reads `HeroCharacter.h`

**This deserves QA's eye, because it is the same shape as the two process notes below.**

TASK-748's test 12 is a **source scan of the shipped text**, and W-7 records that its sentinel comments are load-bearing prose. **I added prose to `HeroCharacter.h` containing the tokens `GetMaxHP` and `GetEffectiveMaxHP`** — so I had to prove I had not turned a comment into a red test. I read the instrument rather than reasoning about it (`SiegeRecallTest.cpp:845-960`):

| leg | scope | my exposure |
|---|---|---|
| (a) region ≥ 2000 chars | `HeroCharacter.cpp` region | ⛔ none — cpp untouched |
| (b) `ResetHero` == 0 | `HeroCharacter.cpp` **region** | ⛔ none — my text has no `ResetHero` |
| (c) `MaxHP` == `EffectiveMaxHP` + `GetMaxHP` | `HeroCharacter.cpp` **region** | ⛔ none — my text is in the **`.h`**, out of scope |
| (d) 5 input interceptors == 0 | **both** `.cpp` **and `.h`** | ✅ measured 0/0 for all five |
| (e) `EKeys::` == 0 | **both** | ✅ measured 0/0 |
| (f) `GetPositionalKey` == 0 | **both** | ✅ measured 0/0 |

Legs (d)/(e)/(f) **do** scan the header I edited — machine-verified all seven needles at 0 in both files after my change. Positive controls intact: `GetPositionalContext` = 3 in cpp (needs ≥1), `ResetHero` = 7 in cpp (needs ≥2).

⭐ **And leg (c) would have held even if it had been in scope:** every `MaxHP` in my comment is contained within either `GetMaxHP` or `GetEffectiveMaxHP`, so the identity `count(MaxHP) == count(EffectiveMaxHP) + count(GetMaxHP)` is preserved by construction. That is luck plus the precedent's own naming, not design — **worth knowing when the next person edits that header.**

---

## 6. Preserved — everything QA already passed, re-verified at the source

- ⛔ **Ghost still does NOT implement `ITeamAgent`.** `SiegeGhostPawn.h:247` is `: public ACharacter` and nothing else. All 6 `ITeamAgent` hits in that header are prose *forbidding* it. Untargetability across all eight acquisition sites (incl. every AoE) is structurally intact.
- ⛔ **Capsule still `ECC_Pawn`** — `SiegeGhostPawn.cpp:66`, untouched. No projectile shield for a dead player.
- ⛔ **Ghost still re-adds `IMC_Hero` itself** — `AddMappingContext(ContextToApply, GhostMappingContextPriority)` at `:465`, priority `1` at `:37`, both untouched.
- ⛔ **The recall channel's two traps** — `ResetHero()` still off the path; the heal still reads `GetEffectiveMaxHP()`, never `MaxHP`. `HeroCharacter.cpp` was not opened for writing at all.
- ⛔ **Ladder wave untouched** — `SummonedUnit.{h,cpp}`, `ClimbableTower.{h,cpp}`, `SiegeLadderClimbTest.cpp`, `SiegeClimbableTowerTest.cpp` all carry mtimes from TASK-760's session (18:12–18:13), not mine (18:47).

---

## 7. ⚠️ WHAT IS STILL UNOBSERVED — say this plainly rather than treating it as a failure

**THE LINK STEP NEVER RAN.** 29 of 32 steps executed; the compile aborted before linking. ⇒ **link errors are UNOBSERVED.**

⭐ **If the next compile fails at link, that is NEW information — ⛔ not a second loop against this finding, and ⛔ not a failure of this task.** These three diagnostics were the only three the compiler reached.

Also still open and **untouched by this task**: the **PIE row** (`GHOST-§4`/`SC-§35`, QA §9) remains a non-waivable commit gate, and W-1's seven unset ghost slots still make the ghost invisible and immobile until `BP_SiegeGhostPawn` can exist post-compile.

---

## 8. ⭐ THE TWO QA-PROCESS NOTES — carried as the orchestrator asked, and neither is a criticism

Both misses have the **same shape**, and it is worth naming because it is a class of error, not two accidents:

> **A review that reads a line for one property can pass it while a different property of that same line is broken.**

- **QA 753 §11(b)** inspected the *exact* failing call and checked whether `GetEffectiveWalkSpeed` had been **renamed**. It had not — the finding was correct. But the question that mattered was **accessibility**, and it was never asked. The member was *always* protected; nothing regressed.
- **QA's N-1** analysed `SiegeGameMode.cpp:483` for **null-safety** and pronounced it engine-safe. Also correct on its own terms — but **the declaration it quoted does not compile.** The line was examined for semantics and never for legality.

⭐ **My own near-miss in §5 is the third instance of the same shape**, which is why I recorded it: I changed a comment, and comments in that file are load-bearing test input. I checked the instrument instead of assuming.

**Suggested standing amendment** (build-master proposed the first half; I would widen it): *a cross-class call must be checked for **reachability**, not merely for spelling — and more generally, when a review clears a specific line, it should name which property it checked, so the unchecked properties are visible as unchecked.*

---

## 9. What QA should scrutinise

1. ⭐ **The third rename site (`:499`).** It was not in my brief. Confirm all three moved together and that `SpawnParams.Owner` at `:814` was correctly left alone.
2. ⭐ **The accessor's category** — `Siegebound|Movement` vs `GetMaxHP`'s `Siegebound|Hero`. Declared in §2; one word to change.
3. **That `GetEffectiveWalkSpeed()` is still `protected` and unmoved**, and that no other line of `HeroCharacter.h` changed. The fence permitted one accessor; one accessor is what landed.
4. **The T12 exposure in §5** — I believe legs (d)/(e)/(f) are clean and (c) is out of scope, but this is exactly the kind of reasoning that has been wrong twice in this batch. Please re-measure rather than accept.
5. **The suite total of 248** — recount independently; I assert it but did not run it (there is no binary).

---

*Written 2026-09-01 by gameplay-programmer. ⛔ No compile run, ⛔ no editor launched, ⛔ no MCP call, ⛔ no Git command. Board row TASK-761 added for status tracking (TASK-758/759 precedent) and the TASK-749/750 `qa-failed` rows annotated with the resolution so neither lane is re-looped.*
