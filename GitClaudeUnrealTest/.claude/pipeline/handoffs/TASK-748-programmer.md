# TASK-748 — [RECALL-2] The 10-second channel — programmer handoff

**Agent:** gameplay-programmer · **Status:** `ready-for-qa` · **Gate:** TASK-753 · **Compile+suite:** TASK-754
**Law read at source before writing:** `RECALL-§1`…`§6`, `GHOST-§ G-6`, `TOWER-§9.2`, `AS-§6` A-2, `HIGH-§1`, `SHIP-§9c`, `SC-§32`, `KBD-§4`.

## Files touched (SOLE-owned only)

| file | change |
|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/HeroCharacter.h` | recall types, delegates, public API, tunables, private state, M8 declaration |
| `Source/GitClaudeUnrealTest/Siegebound/HeroCharacter.cpp` | recall region + four one-line seams (Tick, TakeDamage, HandleDeath, DoMeleeAttack) + ctor soft-ref + input binding |
| `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeRecallTest.cpp` | **NEW** — 12 tests |

⛔ Nothing else was opened for writing. `SiegeGameMode.*`, `SiegePlayerController.*`, `SummonedUnit.*`, `ClimbableTower.*`, `Tower.*`, `WarMapWidget.*`, `SiegeAssistantSnapshot.*` — **untouched**. No compile, no editor, no MCP, no Git. Airlock: no `Capture()`, no `EnsureSnapshot()`, Zone A untouched, no token figure.

---

## The state machine

**One struct is the entire state.** `FSiegeRecallState { bool bChannelling; double StartTimeSeconds; FVector AnchorLocation; }` — plus one optional spawned tell component. ⛔ **No timer handle**, deliberately: a timer is a second lifetime to leak, and this feature's whole design goal is having exactly one thing for its exits to clear. The channel is serviced from the hero's existing `Tick` (it already ticks for HP regen), so no new tick source was added either.

```
        IA_Recall (B) ─► HandleRecallInput()
                              │
                 channelling? ├─ yes ─► EndRecall(CancelledByInput)        [exit 2]
                              └─ no  ─► BeginRecall()
                                            │ CanBegin(state, bDead, IsMatchOver())
                                            └─► RecallState = Begin(now, GetActorLocation())
                                                StartRecallChannelEffect()
                                                OnRecallStateChanged(true, 10)

  Tick ─► TickRecall(now)   [order is deliberate — see below]
             1. IsMatchOver()                       ─► EndRecall(CancelledByMatchEnd)   [exit 6]
             2. HasLeftAnchor(state, loc, 25uu)     ─► EndRecall(CancelledByMovement)   [exit 3]
             3. IsComplete(state, now, 10.f)        ─► EndRecall(Completed)             [exit 1]

  TakeDamage ─ (after the shipped `ActualDamage <= 0` early-return)
             DamageInterrupts(ActualDamage)         ─► EndRecall(InterruptedByDamage)   [exit 4]
  HandleDeath ────────────────────────────────────► EndRecall(InterruptedByDeath)       [exit 5]
  EndPlay ────────────────────────────────────────► EndRecall(CancelledByEndPlay)       [exit 7]

  EndRecall(Exit):
      if (!bChannelling) return;                 ◄── ⭐ THE ONLY IDEMPOTENCY LATCH
      Arrival = BuildArrival(Exit, OnHeroRecallArrived.IsBound(), GetEffectiveMaxHP());
      RecallState = Cleared();  StopRecallChannelEffect();  OnRecallStateChanged(false, 10);
      if (!Arrival.bTeleportHome) return;        ◄── six of seven exits stop here
      OnHeroRecallArrived.Broadcast(this);       ◄── (1) THE TELEPORT — destination owner's job
      CurrentHP = Arrival.HealTargetHP;          ◄── (2) THE HEAL
      OnHPChanged.Broadcast(CurrentHP, GetMaxHP());
```

**Why the Tick order is match-end → movement → completion:** a completion landing on the same frame as a match end or a walk-away must lose. Checking completion last means nothing can teleport-and-heal under the end screen, and a player who steps away on the final frame does not still arrive.

### The seven exits and the test that covers each

| # | exit | fires from | test |
|---|---|---|---|
| 1 | **Completion** | `TickRecall` → `IsComplete` | **T2** boundary (9.9 s is not enough / 10.0 s is), **T7(b)** only-granting sweep, **T8** heal payload |
| 2 | **Re-press `B`** | `HandleRecallInput` / `CancelRecall` | **T3** restart-from-zero, **T7(c)** grants nothing |
| 3 | **Movement** | `TickRecall` → `HasLeftAnchor` | **T6** (X/Y/Z rows, tolerance band, cleared-state row), **T7(c)** |
| 4 | **Damage that landed** | `TakeDamage` → `DamageInterrupts` | **T4** (0 / negative do not, 0.0001 does), **T7(c)** |
| 5 | **Hero death** (`G-6`) | `HandleDeath` (covers KillZ — `FellOutOfWorld` routes there) | **T7(c)**, **T10(b)** dead cannot begin |
| 6 | **Match end** | `TickRecall` → `IsMatchOver` | **T10(d)** cannot begin after end, **T7(c)** |
| 7 | **`EndPlay`** | `EndPlay` override (before `Super`, while the tell is still valid) | **T7(a)** enum closure, **T7(c)** |

**"Exactly once" is structural, not a promise:** every exit routes through `EndRecall`, whose first line refuses a second run for one channel. Lethal damage genuinely reaches exits 4 and 5 in one call stack; the second lands on the latch. ⚠️ **That the four non-Tick call sites are wired is TASK-753's diff read** (`SC-§32` — named rather than faked; a headless suite cannot watch them).

---

## ⭐ Proof for the two traps

### Trap one — `ResetHero()` is NOT on this path
- The arrival's **entire** effect list is a two-field `USTRUCT` (`FSiegeRecallArrival { HealTargetHP, bTeleportHome }`). The death-path restore's five extra effects (cumulative upgrade re-apply, aura re-arm, Rally-cooldown reset, input restore, loadout re-broadcast) are **not expressible** in it. **T11** enumerates the struct by reflection and fails on a third field.
- **T12(b) asserts it directly on the shipped text**: the recall region of `HeroCharacter.cpp` contains **zero** occurrences of the token, with a **positive control** that the token appears ≥ 2 times elsewhere in the same file (so the probe cannot pass on a typo). Verified now: **0 in region, 7 in file.**
- The delegate's own doc comment carries the ban **at the site TASK-750 will bind**, so the trap is pre-empted where the next author actually reads.

### Trap two — the heal reads `GetEffectiveMaxHP()`, never the base field
- The single call site is `EndRecall`: `BuildArrival(Exit, bBound, GetEffectiveMaxHP())`.
- **T12(c) asserts it on the shipped text**: inside the region, `count("MaxHP") == count("EffectiveMaxHP") + count("GetMaxHP")` — i.e. **every** mention of the base field's name is part of an effective read; a bare base read (or a `MaxHPBonus` read) breaks the equality. Verified now: **4 == 3 + 1.** Positive control: `GetEffectiveMaxHP()` appears ≥ 1 time.
- **T8 proves the arithmetic matters**, with both numbers **re-derived from the CDO by reflection** rather than transcribed: base + bonus × 2 stacks, asserting the arrival carries the full effective value and that the gap to the base is exactly the law's figure. It also asserts the bonus is non-zero, so the claim cannot pass vacuously on a hero where the two readings agree.

---

## ⚖️ My three rulings (all match `RECALL-§4`; none blocked)

1. **Only damage that LANDS interrupts** (`R-1`). The seam is `TakeDamage`'s already-shipped applied-amount return: friendly fire, a hit on a dead hero and a fully-mitigated hit all return 0 and leave the channel running. Coupling to *attempted* attacks would need a new notification surface for a rule nobody asked for, and would cancel the player's recall every time an ally brushed him.
2. **Deliberate cancel: YES — re-press `B`, or move.** ⛔ Never `Escape`. Movement-cancel is the League convention he named; a channel with no voluntary exit is a trap the player walks into once.
3. **Visible to the enemy: YES**, via a world-space tell attached to the hero + the `OnRecallStateChanged` delegate for the local HUD. ⚠️ **Honest limit, stated in code:** today's only opponent is `ASiegeBotController`, which does not look at it — the tell is for the human observer and costs nothing now. ⛔ It is not counterplay the bot exercises.

### ⚑ Two further rulings I had to make, declared for QA/Jonathan

4. **The arrival is ATOMIC** — with nothing bound to `OnHeroRecallArrived` there is no teleport, and therefore **no heal** (one warning, once per hero). ⚖️ Rationale: a heal without a teleport would be a free full refill from anywhere on the map, handed out by a *wiring gap*. A feature that is inert until integrated is a visible bug; one that half-fires into an exploit is an invisible one. Pinned by **T9**.
5. **Rally is NOT disarmed by the channel.** His sentence names attacking only, and Rally is a friendly buff, not an attack. ⛔ I did not widen the disarm beyond his words.

---

## The transient disarm (`R-5`, `TOWER-§9.2` idiom)

`DoMeleeAttack`'s **existing** guard gained one term:
`if (bDead || bMeleeSuppressed || FSiegeRecallStatics::IsAttackDisarmed(RecallState))`

⭐ **`IsAttackDisarmed` takes the channel's STATE.** That signature is the ruling: the same hero answers *disarmed* now and *armed* ten seconds later. A `const` class-identity seal is a constant for a given class and structurally cannot say that — which is exactly why the tower wave's `CanEverAttack()` shape was refused here. **T5** proves it four ways: the same predicate flips on two states; the CDO is armed by default; no `CanEverAttack`/`bCanEverAttack`/`bCanAttack` was added (with `DoMeleeAttack` as the positive control); and no parallel `SetRecallSuppressed` suppression mechanism exists beside the shipped `SetMeleeSuppressed`.

⚠️ **Movement is NOT restricted** — nothing on this path touches the movement component. Moving *cancels*, which is a different rule; the two are not conflated.

---

## `Escape` (`RECALL-§3` / `AS-§6` A-2)

The channel adds **no key handler of any kind**. **T12(d)** asserts, across **both** hero files, zero occurrences of `NativeOnKeyDown`, `NativeOnPreviewKeyDown`, `FReply`, `SetInputMode`, `bShowMouseCursor`. Verified now: **all zero.** The shipped cancel routes (placement, spell targeting, group-pick) are untouched and keep firing byte-identically while a channel runs. Cancel is `B` or movement — that is the complete list.

## Keyboard layout (`KBD-§4`)

⛔ **No key is named in code.** `IA_Recall` rides `IMC_Hero`, whose whole context is retargeted for the active OS layout by `GetPositionalContext` in the shipped `NotifyControllerChanged` — so `B` inherits Dvorak with **zero extra code**. ⛔ The subsystem's **single-key** positional lookup is deliberately not called: that API is for RAW POLLED keys and would DOUBLE-TRANSLATE an already-remapped context key (`SiegePlayerController.h:1223-1225`) — wrong *only* on non-QWERTY, i.e. invisible here and immediately visible to Jonathan. **T12(e)/(f)** assert both halves, with `GetPositionalContext` as the positive control (so "no layout support at all" also fails).

---

## ⚠️⚠️ WHAT TASK-750 MUST KNOW — the one integration this feature needs

> **`ASiegeGameMode` must bind `AHeroCharacter::OnHeroRecallArrived` and teleport that hero to the start it already resolves** — the same `GetHeroStartTransform` resolution the respawn path uses.

**Why it is a delegate and not a direct call:** `GetHeroStartTransform` / `RestoreHeroAtStart` are **`protected` on `ASiegeGameMode`** (verified at source: the only `public:` block ends at `SiegeGameMode.h:161`), and `SiegeGameMode.{h,cpp}` is **TASK-750's sole-owned surface**, so the hero cannot reach the resolution and I may not open the file. The delegate is this class's own shipped loose-coupling idiom — `OnHeroDied` exists for exactly this reason (`HeroCharacter.h:24-27`: *"the hero itself knows nothing about respawn schedules"*). ⛔ I did **not** hand-type a location, invent a fallback destination, or duplicate the resolver — `handoffs/TASK-569-buildmaster.md` row (n) is the recorded cost of doing that.

**The binder's contract, in three lines:**
1. **Teleport only.** ⛔ **NEVER call `ResetHero()`** — on a live hero it double-applies every upgrade stack and re-arms a running aura. The heal is already done by the hero.
2. Reuse `GetHeroStartTransform(Controller, Hero->GetTeamId(), …)` + `SetActorLocationAndRotation(..., ETeleportType::TeleportPhysics)`. ⛔ No repossess (the hero is alive and possessed), ⛔ no input change, ⛔ no visibility change.
3. Bind where `OnHeroDied` is bound (`SetPlayerDefaults`), so every pawn the mode hands out is covered.

⚠️ **Until that binding exists the feature is INERT by design** (ruling 4): `B` starts and cancels a channel and the tell plays, but a completed channel logs one warning and does nothing. ⛔ That is a *declared* state, not a defect — and it is deliberately not a free heal.

**Also for TASK-750:** exit 5 already aborts the channel on death (`G-6`), so the ghost hand-off inherits a hero with no channel running. TASK-750 does **not** need to touch recall.

## What TASK-747 must produce (unchanged from its spec)

`/Game/Input/Actions/IA_Recall` (Digital/bool) + **one** appended `IMC_Hero` row mapping it to **B**, empty Triggers/Modifiers. The hero soft-resolves `/Game/Input/Actions/IA_Recall.IA_Recall`; a missing asset logs one line and leaves the key **inert** (the shipped `IA_Cmd*` pattern) — which is why these two tasks did not serialize.

## For TASK-751 (`HELP-§`) — the derivation surface

`IsRecalling()` · `GetRecallChannelSeconds()` (⭐ **derive the duration from this — do not type "10 seconds"**) · `GetRecallProgress01()` · `GetRecallRemainingSeconds()` · `OnRecallStateChanged(bChannelling, ChannelSeconds)`. **Lane note for `HELP-§2` mechanism 2:** `IA_Recall` is an **IMC-context** action, so its label comes from the already-remapped context — ⛔ **not** from the single-key positional API.

---

## Tunables (`HIGH-§1` — consequence written beside each)

| name | value | flags |
|---|---|---|
| `RecallChannelSeconds` | `10.f` | `EditDefaultsOnly`, `ClampMin 0.1`. Consequence written both ways (shorter = free reposition + deletes the counterplay window; longer = unusable under pressure) and cross-referenced to `GHOST-§0`'s death timer. |
| `RecallMoveCancelToleranceUU` | `25.f` | `EditDefaultsOnly`, `ClampMin 0`. Consequence: 0 cancels on capsule/animation jitter; large = a free repositioning budget. |
| `RecallChannelEffect` | **unset** | `EditAnywhere`, null-safe. |

## 📌 M8 declaration (`RECALL-§6` — declared, NOT built, and NOT boilerplate)

In the class comment: reserved `ServerBeginRecall()` / `ServerCancelRecall()` (Server, Reliable, WithValidation) with client-predicted tell and server-owned clock/teleport/heal; reserved replicated `bRecallChannelling` + `RecallStartTimeSeconds`; ⛔ **no new relevancy tier** — it rides the hero's existing Tier B. ⛔ No RPC and no replicated property authored. **Named P1 gap:** the match-end exit reads `GetAuthGameMode()`, which is null on a client — authoritative on host/standalone (every shipped configuration today), and a future client build must move that exit onto replicated match state.

---

## 🔍 Things QA should scrutinise

1. **⚑ DECLARED NEW INSTRUMENT — T12 is a SOURCE SCAN.** It reads `HeroCharacter.{h,cpp}` from `FPaths::ProjectDir()` and asserts on the shipped text between two sentinel comments. **There is no precedent for this in `Siegebound/Tests/`.** I introduced it because TASK-753's own gate requires both traps *"asserted by a test, ⛔ not merely absent"*, and a headless suite has no other way to watch a call that must never happen. It is self-checking (missing/duplicated/out-of-order sentinels ⇒ `AddError` + fail, never a quiet pass) and every claim has a positive control. **If QA rules the instrument out, tests 1–11 still stand and the two traps fall back to the diff read.**
2. **The sentinel comments are load-bearing.** Editing the recall region must not introduce the tokens `ResetHero` or a bare `MaxHP` **even in prose** — the sentinel block above the region says so.
3. **Ruling 4 (atomicity)** — is "no binder ⇒ no heal" the behaviour the gate wants, or should the heal proceed? I ruled it must not, and the reasoning is above.
4. **3D movement-cancel** — falling cancels. Deliberate (a channel that survived a fall would arrive from a spot the player never chose), and it fails against a horizontal-only implementation.
5. **`DamageInterrupts(ActualDamage)` is called where `ActualDamage > 0` is already guaranteed** by the shipped early-return. That is intentional, not redundancy-by-accident: it puts `R-1` in one named, testable place that survives a future change to the early return. Comment says so at the site.
6. **`EndPlay` is defined inside the recall region** (out of declaration order) because its entire body is this feature's teardown and it belongs where the scan can see it.
7. **`SpawnSystemAttached` / `NiagaraComponent.h` are new to this module** (no prior user). `Niagara` is already a public dependency. Worth a compile-time eye at TASK-754.
8. **`R-3`'s tell ships as a null-safe slot, not an asset.** ⛔ I did not invent an emitter path — no task in this batch produces one, and naming a path the Artist is not making would violate the naming law. **FOR THE MANAGER: the world-space tell needs an art task**; until then the shipped tell is the HUD's, via the delegate + progress getters.

## Suite delta

**+12 tests** (`SiegeRecallTest.cpp`). ⚠️ **Do not reconcile against a raw file count:** two sibling test files (`SiegeGhostPawnTest.cpp`, `SiegeMapMarkTest.cpp`) landed on disk while this task was in flight. Per TASK-753 item (8), the baseline is the ladder wave's **post-TASK-742** total from `handoffs/TASK-742-buildmaster.md`, plus each task's declared delta — **⛔ not 171 and ⛔ not from memory.**

## Names produced

`ESiegeRecallExit` (7 values) · `FSiegeRecallArrival` · `FSiegeRecallState` · `FSiegeRecallStatics` · `FOnHeroRecallArrived` · `FOnHeroRecallStateChanged` · `RecallChannelSeconds` · `RecallMoveCancelToleranceUU` · `RecallChannelEffect` · `RecallAction` / `RecallActionAsset` · `HandleRecallInput` / `BeginRecall` / `CancelRecall` / `IsRecalling` / `GetRecallChannelSeconds` / `GetRecallProgress01` / `GetRecallRemainingSeconds`.
