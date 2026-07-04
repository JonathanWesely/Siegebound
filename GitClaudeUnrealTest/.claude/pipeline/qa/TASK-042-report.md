# QA Report — TASK-042

Verdict: **PASS**

- Scope: Hero Rally ability + unit move-speed buff API (C++), files-only, pre-compile.
- Files reviewed: `Source/GitClaudeUnrealTest/Siegebound/HeroCharacter.h`/`.cpp`, `Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.h`/`.cpp` (+ cross-check of `MinerUnit.cpp`).
- Counts: **0 BLOCKER · 0 WARN · 3 NIT**

## Rulings on the flagged decisions

1. **Miners are buffed — ACCEPT (harmless, per literal spec).** `AMinerUnit : ASummonedUnit`, so `GetAllActorsOfClass(ASummonedUnit)` includes friendly miners; the spec says "every friendly same-team ASummonedUnit" and miners qualify. Critically, I confirmed `MinerUnit.cpp` has **no `MaxWalkSpeed` writer of its own** (grep clean), so the only writers on the miner are `LoadStatsAndStart` (once, at bind) and the buff apply/end pair. A buffed miner gets `base×1.25` for 5 s then restored to exactly its card `Speed` — no drift, no interaction with any miner-specific movement logic. A brief walk-speed boost on a gold-node walk is cosmetically inconsequential. No reason to add a `MinerUnit` include to exclude them; leaving it inclusive is correct.
2. **Center-to-center `DistSquared` range — ACCEPT.** Rally targets only units (small pawn capsules), so center distance is the natural reading of "within 600." The melee/AI closest-point pattern exists for the castle's ~800×800 footprint, which Rally never targets. Center-vs-closest-point differs only by ~capsule radius against a 600 radius — negligible and defensible.
3. **`ResetHero` now resets Rally — ACCEPT.** The addition (clear `RallyCooldownTimerHandle`, reset `LastRallyTime = -1e9`, broadcast `(true, 0)`) is purely additive, appended after the untouched melee/regen reset lines, and is an explicitly-sanctioned reset-path broadcast per the CONVENTIONS delegate law. Keeps a respawned hero's Rally usable and the HUD consistent. M1 `ResetHero` behavior otherwise byte-preserved.

(Handoff also flagged #4 optional refusal broadcast and #5 UPROPERTY house-style flags — both accepted; see NIT-3 and Notes.)

## Findings

- [NIT] `SummonedUnit.cpp:210` / `:298` — Theoretical base-capture-vs-stat-load ordering: if `ApplyMoveSpeedBuff` ran on a unit *before* `LoadStatsAndStart` bound its card `Speed`, the buff would capture the pre-load default as base, `LoadStatsAndStart` would then overwrite `MaxWalkSpeed = Row->Speed`, and `EndMoveSpeedBuff` would restore the stale default. **Not reachable in the current codebase** — the spawn flow (SpawnActorDeferred→InitUnit→FinishSpawning, or plain SpawnActor + immediate InitUnit) completes stat binding synchronously at spawn, before any player-input frame where `Rally()` can fire; `bStatsLoaded` gates `LoadStatsAndStart` to run exactly once. No fix required; noted so a future async/late-InitUnit spawn path doesn't reintroduce it.
- [NIT] `SummonedUnit.cpp:829` — `HandleDeath()` clears `StateTimerHandle`/`AttackTimerHandle` but not `MoveSpeedBuffTimerHandle`. Harmless: `HandleDeath` calls `Destroy()`, `EndPlay` clears it (line 123), and UE auto-clears object-bound timers on destroy; even a stray fire on a dead unit is a no-op restore. Optional belt-and-suspenders only.
- [NIT] `HeroCharacter.cpp:287` — The refused-press broadcast `OnRallyStateChanged(false, remaining)` is a minor tension with the CONVENTIONS "never broadcast on refused/ignored mutations" rule. Acceptable here: that rule targets value-change delegates (e.g. don't fire `OnGoldChanged` when gold is unchanged); this is a UI-feedback state delegate whose spec explicitly lists the refusal broadcast as *optional*, it asserts no value change (it reports current cooldown), and a seed-then-bind HUD is unaffected. Keep as-is.

## Verification notes (the focus checks)

- **Drift-free base speed (critical): PASS.** `MoveSpeedBuffBaseSpeed` captured ONLY on inactive→active (`if (!bMoveSpeedBuffActive)`, SummonedUnit.cpp:210). Walk speed is only ever written as `Base×Multiplier` (:218) or exactly `Base` (:247). A refresh while active reuses the stored base and cannot recapture a buffed value; multiple applies (even with differing multipliers) never compound and never drift. The full `MaxWalkSpeed`-writer census on `ASummonedUnit`+`AMinerUnit` is exactly {LoadStatsAndStart once, buff apply, buff end} — no other writer can collide. Units have no sprint path (only the hero does, and Rally never touches the hero), so the M2 sprint pattern cannot interfere.
- **FreezeAI: PASS.** `FreezeAI` calls `EndMoveSpeedBuff()` (:175) which clears `MoveSpeedBuffTimerHandle` and restores exact base; nothing after re-writes `MaxWalkSpeed`, so zero residual. Miners inherit via `AMinerUnit::FreezeAI()`→`Super::FreezeAI()` (MinerUnit.cpp:163, confirmed). `EndPlay` clears the timer (:123). Idempotent (`EndMoveSpeedBuff` early-outs when inactive).
- **Delegate: PASS.** `FOnRallyStateChanged(bool bReady, float CooldownRemaining)` `TwoParams`, `UPROPERTY(BlueprintAssignable) OnRallyStateChanged`; names match the `names:` block character-for-character. Broadcasts on use `(false, RallyCooldown)` (:324), on-ready `(true, 0)` (:331), on refused press `(false, remaining)` (:287), and reset `(true, 0)` (:467).
- **Cooldown logic: PASS.** First press allowed (`LastRallyTime = -1e9`); a press inside the window early-outs before `LastRallyTime` is updated and before the timer is set — a true no-op (no double-buff, no cooldown-reset abuse). Timer-based `OnRallyReady` and the `LastRallyTime` gate both use `RallyCooldown` and stay consistent.
- **C4458/C4457/C4459 shadow scan: PASS.** No local/param/loop var in `Rally`, `OnRallyReady`, `ApplyMoveSpeedBuff`, `EndMoveSpeedBuff`, or the `ResetHero` additions shadows an inherited reflected member. Loop var is `FriendlyUnit`; `Movement` local is safe (ACharacter's member is `CharacterMovement`, not `Movement`, and this name is already used elsewhere in the file). No `Owner`/`Instigator`/`PlayerState`/`Controller` collisions.
- **Deprecated APIs / null-safety: PASS.** All APIs valid for UE 5.8 (`GetAllActorsOfClass`, `GetWorldTimerManager`/`SetTimer`, member-ptr timer callbacks need no `UFUNCTION`, dynamic-multicast `Broadcast`). `RallyAction` is a null-safe unbound slot (bound in TASK-048). World/movement-component/cast null checks present. `FTimerHandle`/delegate macros resolve via the existing `Character.h` transitive include (matches pre-existing `StateTimerHandle`). Includes added correctly (`SummonedUnit.h`, `TimerManager.h`).
- **M1/M2 regression: PASS.** Melee, lunge, ranged, sprint, TakeDamage, state machine, and the existing FreezeAI/EndPlay/ResetHero bodies are byte-identical apart from the additive lines reviewed above.

## Notes for build-master (if PASS)

- Clean pre-compile PASS; no blockers. Compile as part of the M3 batch (TASK-051) with the other qa-passed M3 code tasks.
- `RallyAction` is intentionally null/unbound until TASK-048 (IA_Rally, key Q); `OnRallyStateChanged` is bound by the HUD in TASK-050 — neither is required for this compile.
- No new module/`Build.cs` dependency introduced.

---

## BUILD-MASTER COMPILE RESULT — 2026-07-04 (M3 recompile after Rally binding + path-tracing fix)

**Verdict: BUILD FAILED — exit code 6 (`Result: Failed (OtherCompilationError)`). NOT committed. Editor NOT booted. Handing back for a code fix.**

### The Rally change (TASK-042) is INNOCENT — it compiled clean
- `[1/5] Compile [x64] HeroCharacter.cpp` produced **zero** diagnostics. The one-line guarded `BindAction(RallyAction, ETriggerEvent::Started, ...)` addition is fine. No C4457/8/9 shadow warnings, as predicted.

### Actual failure: latent duplicate-symbol (ODR) collision under adaptive unity — owned by TASK-045
The failing action was `[2/5] Compile [x64] Module.GitClaudeUnrealTest.cpp` (the module unity blob). Both of these files declare, at **file scope inside an anonymous `namespace`**, two identically-named `constexpr` constants:
- `SiegeBotController.cpp:28` — `constexpr float SpawnGroundClearance = 2.f;`  (comment: "mirrors ASiegePlayerController")
- `SiegeBotController.cpp:31` — `constexpr float DefaultCapsuleHalfHeight = 88.f;`
- `SiegePlayerController.cpp:32` — `constexpr float SpawnGroundClearance = 2.f;`  (pre-existing)
- `SiegePlayerController.cpp:35` — `constexpr float DefaultCapsuleHalfHeight = 88.f;` (pre-existing)

Compiler errors (verbatim):
```
SiegePlayerController.cpp(32,18): error C2374: '`anonymous-namespace'::SpawnGroundClearance': redefinition; multiple initialization
    note: SiegeBotController.cpp(28,18): see declaration of '`anonymous-namespace'::SpawnGroundClearance'
SiegePlayerController.cpp(32,18): error C2086: 'const float `anonymous-namespace'::SpawnGroundClearance': redefinition
SiegePlayerController.cpp(35,18): error C2374: '`anonymous-namespace'::DefaultCapsuleHalfHeight': redefinition; multiple initialization
    note: SiegeBotController.cpp(31,18): see declaration of '`anonymous-namespace'::DefaultCapsuleHalfHeight'
SiegePlayerController.cpp(35,18): error C2086: 'const float `anonymous-namespace'::DefaultCapsuleHalfHeight': redefinition
```

### Why it "passed" at 2f6a8fc but fails now (NOT a regression from my edit)
Adaptive unity excluded my just-edited `HeroCharacter.cpp` from the unity file (`[Adaptive Build] Excluded from GitClaudeUnrealTest unity file: HeroCharacter.cpp`). That shifted the unity bucket boundaries so `SiegePlayerController.cpp` and `SiegeBotController.cpp` were merged into the **same** translation unit for the first time, where their internal-linkage constants collide. At 2f6a8fc (TASK-051's full compile) a different unity layout kept them in separate TUs, so the collision was masked. **The defect is real and layout-fragile** — it will recur on any future re-bucket. Do NOT "fix" this by forcing a full/non-unity rebuild to dodge it; the duplicate symbols must be removed.

### Recommended fix (gameplay-programmer, route to TASK-045 — do NOT let build-master edit code)
Dedupe the shared constants. Cleanest: hoist `SpawnGroundClearance` and `DefaultCapsuleHalfHeight` into one shared spot (e.g. a small `SiegeSpawnConstants.h` or an existing shared header) and include it in both controllers, removing both local copies. Alternatively give the bot's copies distinct names. Either restores unity-safety.

### Build-master state after failure (all per protocol)
- No commit made. Working tree still holds the intended, uncommitted edits: `Config/DefaultEngine.ini` (r.PathTracing=False), `Source/.../HeroCharacter.cpp` (Rally binding), `.claude/pipeline/TASKBOARD.md`, `handoffs/TASK-042.md`, and untracked `handoffs/TASK-051.md`. The Rally fix + path-tracing fix are preserved for the re-batch once TASK-045 is fixed & re-QA'd.
- The two stray editor re-saves (`BP_Building_ArrowTower.uasset`, `L_Arena.umap`) were restored to HEAD before the build.
- Editor NOT booted (gated on a clean build; a boot now would trigger rebuild-on-boot and hit the same error).
- Full build log: scratchpad `build_m3.log` (exit code in `build_m3.rc`).
