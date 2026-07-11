# QA Report — TASK-122
Verdict (round-1, RETAINED FOR RECORD): **PASS** (0 BLOCKER · 1 WARN · 3 NIT). **RE-REVIEW at the bottom — "QA LOOP 1 — ITEM A adjudication (2026-07-10)" — SUPERSEDES this header. Current verdict: PASS; the loop-1 C++ is HARMLESS HARDENING (not load-bearing); 1b is NOT closed until TASK-127.**

Pre-compile review of the M5.5 overhead-health-bar reversal (always-visible law + fill-drive
hardening). Scope reviewed: the two changed files ONLY —
`Source/GitClaudeUnrealTest/Siegebound/HealthBarComponent.cpp` / `.h`.
Cross-read (read-only, for verification): `UnitHealthBarWidget.h`, `HealthBarTarget.h`, `TeamId.h`
(complete-type + contract checks); `.claude/pipeline/CONVENTIONS.md` §"Overhead unit health bars
(M5.5)" (the reversed behavior law, line 163); the TASK-122 spec (TASKBOARD line 442-486); and — to
reconstruct the OLD PollHealth without Git access — the independent TASK-110 QA line-review, the
TASK-110/112 handoffs, and the manager's TASK-122 spec (which cites the OLD line numbers verbatim).

> **Constraint disclosure:** I have no Git/Bash access by design, so I could NOT diff the raw
> pre-change source directly. The OLD `PollHealth` is instead reconstructed from FOUR independent
> written records (below). Where that matters for the orchestrator's doubt it is called out.

---

## MANDATORY scans

**Inherited-reflected-member shadow scan (C4457/58/59): CLEAN.** TASK-122 adds NO new members —
`BarWidget` (TObjectPtr, UPROPERTY Transient), `bBarShown`, `PollTimerHandle` all pre-exist and were
cleared by the TASK-110 scan. The new/kept PollHealth locals (`Target`, `bAlive`, `bShouldShow`) and
BeginPlay locals (`LoadedWidgetClass`, `TeamAgent`, `BarColor`, `World`) shadow no reflected member of
`UWidgetComponent`/`UPrimitiveComponent`/`USceneComponent`/`UActorComponent`. No local/member/param is
named `Owner`, `Instigator`, `Controller`, `PlayerState`, or `Slot`. `BarWidget`≠`Widget`,
`HealthBarWidgetClass`≠`WidgetClass` (the two near-collisions from TASK-110) are untouched.

**Complete-type-include scan: CLEAN.** Every deref/`Cast<>` in the change targets a type whose full
header is already included in the .cpp: `UUnitHealthBarWidget` (`Siegebound/UnitHealthBarWidget.h`,
l.12) — used by the re-resolve `Cast<UUnitHealthBarWidget>(GetWidget())` and `BarWidget->OnHPChanged`;
`UUserWidget` from `GetWidget()` (`Blueprint/UserWidget.h`, l.5); `IHealthBarTarget`
(`Siegebound/HealthBarTarget.h`, l.10); `ITeamAgent`/`ETeamId` (`Siegebound/TeamId.h`, l.11);
`TSoftClassPtr` (`UObject/SoftObjectPtr.h`, header l.8); `FTimerManager` (`TimerManager.h`, l.9);
`UWorld` (`Engine/World.h`, l.7). The re-resolve introduces no new type usage — it reuses the exact
cast already present at BeginPlay.

**Deprecated/removed UE 5.8 APIs: NONE.** `SetWidgetSpace`, `SetDrawSize`, `SetCollisionEnabled`,
`SetGenerateOverlapEvents`, `SetVisibility(bool, bPropagateToChildren)`, `SetRelativeLocation`,
`SetWidgetClass`, `GetWidget`, `LoadSynchronous`, `IsNull`, `GetTimerManager`, `SetTimer`/`ClearTimer`,
`GetWorld`, `GetOwner`, `Cast<>`, `EWidgetSpace::Screen`, `ECollisionEnabled::NoCollision` are all
current in 5.8. No deprecated call in the change.

---

## Contract checks demanded by the task — all CONFIRMED

- **`EndPlay` still clears `PollTimerHandle`** — `.cpp:99-107`, `World->GetTimerManager().ClearTimer(PollTimerHandle)`, unchanged. No dangling poll after the owner leaves play.
- **One-time team tint still happens** — `.cpp:82-87` in BeginPlay, guarded by `if (BarWidget)`, reads `ITeamAgent::GetTeamId` and pushes `BlueBarColor`/`RedBarColor` via `SetTeamColor` once. Tint is DATA, never hardcoded. **CAVEAT: only on the BeginPlay-resolved path — see WARN-1.**
- **Hide-on-death/destruction survives** — `bShouldShow = bShowHealthBar && bAlive` (`.cpp:127`); `!bShouldShow` ⇒ hide via the `bBarShown` latch and return (`.cpp:129-140`). `bAlive = IsHealthBarActorAlive()`. Units/buildings Destroy() (component + bar vanish via EndPlay); the hero HIDES and is caught within ≤1 poll. Intact.
- **Missing-widget-class ⇒ silent no-bar, logged once, never a crash** — `.cpp:58-71`: `IsNull() ? nullptr : LoadSynchronous()`; if null, a function-local `static bool` logs ONCE run-wide at `Log`, then early-return WITHOUT arming the timer. Unchanged, correct.
- **Fill driven every poll while alive (NOT edge-gated)** — `OnHPChanged(Current, Max)` at `.cpp:160-163` sits OUTSIDE the `if (!bBarShown)` visibility-toggle block (`.cpp:142-146`); it fires every poll while `bShouldShow` and `BarWidget` is non-null, full HP included (SetPercent 1.0). CONFIRMED — not gated on a `bBarShown` latch or an edge transition.
- **Retired `HealthBarFullEpsilon` left no dead references** — project-wide grep for `HealthBarFullEpsilon`/`Epsilon`/`Max - 0.01` returns only two hits, both explanatory COMMENTS (`.cpp:124`, `.h:29`) documenting the retirement. The anonymous-namespace constant is gone; the show-gate no longer references it. No dead code, no dangling symbol.
- **Zero combat/stat/damage change** — the component only reads HP getters; no HP/gold/damage/movement mutation. `ACastle`/`FOnCastleHPChanged`/`UCastleHealthBarWidget` are NOT in the change set (only 2 files changed); the castle push-model bar is untouched. CONFIRMED.

---

## Findings

- **[WARN]** `HealthBarComponent.cpp:153-163` — **the self-healing re-resolve heals the FILL but NOT the team tint: a late-created widget ends up UNTINTED.** `SetTeamColor` is pushed ONLY in BeginPlay (`.cpp:82-87`). If `BarWidget` is null at BeginPlay (the deferred Screen-space Slate-construction race the re-resolve is explicitly built to defend against) and only resolves later in PollHealth via `.cpp:153-156`, the re-resolve block calls `OnHPChanged` but NEVER `SetTeamColor` — so in exactly its target failure mode the bar renders with the WBP's design-time/default fill color instead of the blue/red team tint. This is the precise gap the orchestrator asked me to audit, and it is real. **Not a BLOCKER:** it never crashes, and the manager's spec + TASK-112 MCP evidence indicate `BarWidget` resolves at BeginPlay in the normal case (so the re-resolve is dead code in practice and the tint is applied normally). But the hardening as written is INCOMPLETE. **Suggested fix:** apply the tint at the null→non-null transition, e.g. on the poll where the re-resolve first succeeds, re-run the same `ITeamAgent`→`SetTeamColor` push (factor BeginPlay's tint block into an `ApplyTeamTint()` helper and call it from both BeginPlay and the re-resolve success path). If TASK-124 phase-B PIE shows the deferred-widget path is ever live, this WARN must be promoted to a fix before ship. [LOOP-1 UPDATE: the programmer implemented exactly this `ApplyTeamTint()` fix — WARN-1 is now addressed; see the recreate-gap NIT in the ITEM A section.]

- **[NIT]** `HealthBarComponent.cpp:113` — `Cast<IHealthBarTarget>(GetOwner())` runs every poll (~0.15 s). The owner is fixed for the component's life; caching the interface pointer at BeginPlay would drop a per-poll dynamic cast. Carry-forward from TASK-110 (already noted there), negligible at ~60 actors, correctly defensive. Optional.

- **[NIT]** `HealthBarComponent.cpp:153-156` — in the mis-authored-widget case (WBP not parented to `UUnitHealthBarWidget`), `BarWidget` stays null forever and the re-resolve runs a `Cast<>` on `GetWidget()` every poll for the actor's whole life. One cast per 0.15 s per actor is trivial, and this is the correct null-safe behavior (no crash, fill simply stays blank) — noted only so build-master doesn't mistake the perpetual cast for a smell.

- **[NIT]** `HealthBarComponent.cpp` — the WidgetComponent render-tick remains enabled (screen-space widgets must tick to follow the actor on screen — the `ACastle::HPBarWidget` precedent). No GAMEPLAY work on tick (HP is timer-driven). Carry-forward from TASK-110; called out so it isn't read as a per-tick smell.

---

## Ruling on the orchestrator's doubt (which of the three is true)

**Possibility 1 (the OLD code pushed `OnHPChanged` only on the show *transition*, not every poll ⇒ a
second real fill bug) — REFUTED.** Four independent written records converge on: the OLD code pushed
`OnHPChanged` EVERY poll while shown, guarded only by `if (BarWidget)`, as a block SEPARATE from (and
after) the `bBarShown` visibility toggle — i.e. the same structure the NEW code has:

1. **TASK-110 QA line-review** (`qa/TASK-110-report.md:96-98`): "`bShouldShow = bShowHealthBar && bAlive && (Current < Max − 0.01)` … else **SHOW + `OnHPChanged(Current, Max)`**." The `bBarShown` latch is documented (flag 5) as gating only the `SetVisibility` toggle to avoid redundant render dirties — NOT the push.
2. **TASK-110 handoff** (`handoffs/TASK-110-programmer.md:59`): "show → `SetVisibility(true)` + `OnHPChanged(Current, Max)`."
3. **The manager's own TASK-122 spec** (`TASKBOARD.md:457-458`) cites the OLD layout by line number: the show/hide "runs regardless of BarWidget … `OnHPChanged` at **:152-154** is guard-skipped." OnHPChanged at OLD `:152-154` is a distinct `if (BarWidget)`-guarded block AFTER the visibility toggle (the edge block sits ~:142-146) — not inside the edge. Had it been edge-gated it would sit adjacent to `SetVisibility(true)`, not 6+ lines below it.
4. **The UNCHANGED contract** in `UnitHealthBarWidget.h:16` and `CONVENTIONS.md:162`: "calls `OnHPChanged` **each poll while the bar is shown**." (Design law, not the programmer's self-report.)

So there was NO edge-only-push bug, and the programmer's "1b was a symptom of 1a" is NOT contradicted
by a hidden second fill defect in the C++. **Possibility 1 is off the table.**

**Between Possibility 2 (BarWidget actually null at runtime) and Possibility 3 (imprecise observation):
static review CANNOT fully settle it, and the programmer OVERSTATES his certainty.** Given #1, if the
OLD code truly pushed every poll on a NON-NULL `BarWidget`, then a unit surviving two hits SHOULD have
shown a visible drop (e.g. 60%→30%) — which is exactly the orchestrator's contradiction, and it is a
legitimate one. It resolves only one of three ways, none of which the static C++ can prove:

- **(3-lean)** Jonathan's read was imprecise: at full HP the bar was HIDDEN, so on first damage it
  appeared already partly drained at whatever HP it had, and arena units often die in a burst before a
  second poll — so no clean full→drop was ever witnessed, reading as "castle works, the others don't."
  This is the most likely single explanation and is consistent with all the static evidence.
- **A fourth cause the three-way framing omits — a WBP RENDER defect (TASK-123's domain).** The
  programmer's `read_graph_dsl` proves the GRAPH wires `SetPercent(Current/Max)` to `GetBar`, but a DSL
  graph readback does NOT prove the ProgressBar's fill BRUSH visually reflects percent (a style/appearance
  fact, not graph wiring). If the "Bar" fill is a static/full-only image, `SetPercent` runs but nothing
  moves on screen — "don't drop at all" with correct C++. TASK-123 is explicitly tasked to verify this.
- **(2)** A genuine runtime-null `BarWidget` (deferred Slate construction racing the single BeginPlay
  cast). The programmer REFUTED the null theory only at the ASSET level (`get_parent` = reparented) and
  the UObject-config level (TASK-112 live components resolved the class) — neither proves `BarWidget` was
  non-null at the BeginPlay cast timing under damage at runtime. He concedes this residual and added the
  re-resolve for it. It cannot be excluded by static analysis; only PIE can.

**Net ruling:** Possibility **1 = false** (proven). The truth is most likely **3** (compounded by the
hide-at-full gate), with a live, unexcluded contribution from a **WBP-render defect** and/or **runtime
null (2)** that ONLY TASK-123/124 PIE can settle. Consequently the programmer's verdict is CORRECT that
there is no C++ fill-push bug, but he OVERSTATES it as "1b was merely a symptom of 1a": the always-visible
change is NECESSARY and correct, yet may not be SUFFICIENT for Jonathan to see the fill drop if a
WBP-render defect or a real deferred-null exists. Both are correctly deferred to TASK-123 (WBP) and
TASK-124 phase-B (PIE) — which must treat "the fill visibly moves AND is team-tinted" as a hard gate,
NOT assume TASK-122 alone closed 1b. **[LOOP-1 OUTCOME: PIE proved the fill DOES drain (log) and the
WBP-render/contrast cause (the fourth cause above) is the live one — see ITEM A. Possibility 2 is now
also REFUTED by the runtime log + engine source.]**

## Flagged decisions for the orchestrator (not code blockers)

- **FLAG A — tint gap in the re-resolve (WARN-1).** Ship-blocking ONLY if TASK-124 PIE shows the
  deferred-widget path is live. Recommend the manager pre-authorize the one-line `ApplyTeamTint()` fix so
  it doesn't cost a full QA loop if PIE surfaces it. [LOOP-1: pre-authorized and implemented.]
- **FLAG B — the "1b symptom of 1a" claim is not fully proven.** Do not let TASK-123/124 downgrade to a
  rubber-stamp verify. The fill-visibly-moves + tint-reads check must be an authoritative PIE gate, since
  the WBP-render-defect and runtime-null causes remain open (see ruling above). **[LOOP-1: vindicated —
  the claim was empirically refuted; the real fix is WBP-side, TASK-127.]**
- **`bShowHealthBar` opt-out KEPT** (default true, EditDefaultsOnly) per CONVENTIONS OPEN QUESTION /
  manager FLAG 3 — correct; it gates WHETHER a type carries a bar, orthogonal to the always-visible law.

---

## Notes for build-master (TASK-124)
- Compiles-clean expected (warnings-as-errors): shadow-scan CLEAN, complete-type-include CLEAN, no
  deprecated APIs, no new members, epsilon fully retired with zero dangling references.
- Phase-A: just compile — no code issue blocks it. Phase-B PIE is the authoritative gate for 1b:
  **verify the fill VISIBLY drops from full as HP falls AND the fill is team-tinted (blue friendly / red
  enemy)** on a unit, tower, wall, Barracks, Deep Mine, miner, and the hero — on BOTH teams. If any bar
  shows the wrong/untinted color specifically after appearing late, that is WARN-1 manifesting — route
  back to gameplay-programmer with the `ApplyTeamTint()` fix.
- Castle bar (`FOnCastleHPChanged` / `WBP_CastleHealthBar`) and gold nodes must stay exactly as before —
  outside this change set; confirm no regression during the PIE sweep.

---

# QA LOOP 1 — playtest failure + runtime diagnosis (gameplay-programmer, 2026-07-10)

**Trigger:** Jonathan PIE'd the compiled round-1 build (DLL 23:42:34). Result: *"health bars are now
always visible, but they still do not drop when the characters and tower take damage."* → **1a shipped,
1b NOT fixed.** The round-1 "1b was merely a symptom of 1a" verdict is empirically REFUTED (QA FLAG B was
right to withhold endorsement). New evidence relayed: overhead bars are NOT team-tinted (both teams same
color); the hero DOES have a bar.

## Read-only + live-PIE runtime diagnosis (measured, not theorized)
Ran a fresh PIE session via MCP and probed with `ObjectTools`/`LogsToolset`/`CaptureEditorImage`:
- `get_parent(WBP_UnitHealthBar)` = `/Script/GitClaudeUnrealTest.UnitHealthBarWidget` — reparent CORRECT.
- `read_graph_dsl` — `OnHPChanged`→`if MaxHP>0`→`SetPercent(GetBar, CurrentHP/MaxHP)`; `SetTeamColor`→
  `SetFillColorAndOpacity(GetBar, MakeLinearColor(R,G,B,1))`. Graph WIRED.
- Live hero `HPBarWidget` (HealthBarComponent): `WidgetClass`=`WBP_UnitHealthBar_C`, `bShowHealthBar`=true,
  `Space`=Screen, `bVisible`=true. Hero `Team`=Blue, `CurrentHP`=200/`MaxHP`=200.
- Authored `Bar`: `Percent`=1, `FillColorAndOpacity`=white(1,1,1,1), `fillImage.tintColor`=white — nothing
  bakes red or freezes the fill on the asset side.
- Output log: **ZERO "Accessed None"** across the PIE session → the WBP graph is NOT hitting a null `Bar`
  (so either the graph never runs, or `Bar` is valid).
- **Tooling limit that blocked a direct read:** `get_properties` CANNOT serialize the transient
  `Widget`/`BarWidget` UUserWidget pointers ("could not be read: Widget/BarWidget"), and `CurrentHP` is
  not settable — so a live damage probe and a direct null/non-null read of `BarWidget` are impossible.

## Root-cause finding (the decisive engine fact two prior verdicts missed)
`UWidgetComponent::SetWidgetClass` (engine `WidgetComponent.cpp:2361`) sets `WidgetClass`
**unconditionally**, but only runs `CreateWidget`+`SetWidget` when `HasBegunPlay()` is true at that
instant. **Therefore "WidgetClass is set at runtime" does NOT prove the widget was constructed** — a set
`WidgetClass` with a null `GetWidget()` is possible, and that produces EXACTLY visible+frozen+untinted
(component `SetVisibility` runs regardless; `OnHPChanged`+`SetTeamColor` both guard-skip on the null
`BarWidget` cast). Combined with the coordinator's logic (round-1 already shipped the self-healing
re-resolve; had it EVER resolved non-null the fill would move while tint stayed missing — Jonathan has
neither) this is the **leading root cause: `BarWidget` is null at runtime because `SetWidgetClass`'s
construction side-effect did not fire / did not persist.** This is C++-side, NOT a WBP defect. It is a
LEADING (measured-consistent) cause, not yet 100% proven — the transient pointer is unreadable by the
tool — so a one-pass runtime log is added to confirm.

## Fix + instrumentation shipped this loop (HealthBarComponent.cpp/.h)
1. **Explicit widget construction** (the fix): after `SetWidgetClass`, if `GetWidget()` is null, call
   `SetWidget(CreateWidget<UUnitHealthBarWidget>(World, LoadedWidgetClass))` — do not rely on the
   conditional side-effect. Null-safe.
2. **`ApplyTeamTint()` helper** (round-1 WARN-1, now ship-relevant): tint factored out, called from
   BeginPlay AND the poll re-resolve, idempotent via `bTeamTintApplied` — a late-resolved widget is never
   left untinted.
3. **`[TASK122DIAG]` throttled logs** (Attempt B, to PROVE the cause in one PIE pass): BeginPlay logs
   `side-effect-created=YES/NO`, actual `GetWidget` class, and `BarWidget-cast=VALID/NULL`; PollHealth
   logs (first 3 polls + every 40th) `alive/Current/Max/BarWidget/PUSHED|SKIPPED`. **These logs are
   TEMPORARY — remove once root cause is confirmed.**

## MANDATORY scans (re-run on the loop-1 change)
- **Shadow scan: CLEAN.** New members `bTeamTintApplied`, `DiagPollCount`; new locals `bSideEffectCreated`,
  `Current`, `Max` — none shadow an inherited reflected UPROPERTY.
- **Complete-type-include: CLEAN.** `CreateWidget`/`SetWidget` from `Blueprint/UserWidget.h`+
  `Components/WidgetComponent.h` (included); `UUnitHealthBarWidget` (included); `GetNameSafe` global
  (used across the module). No new type usage lacks a complete header.

## What build-master (TASK-124) must do this loop
- Phase-A: compile (warnings-as-errors expected clean).
- **Read the `[TASK122DIAG]` log after ONE PIE pass** and record it back here:
  - `side-effect-created=NO` → the widget was never constructed; the explicit-create fallback now fixes it
    → bars should drop → confirm in PIE, then a follow-up loop trims the diag logs. **C++-side, closed.**
  - `side-effect-created=YES` + `BarWidget-cast=VALID` + poll shows `Current` dropping but the bar still
    doesn't move → the defect is WBP-render-side → route to **TASK-123 (art-director)**.
- Verify the fill VISIBLY drops from full AND is team-tinted (blue friendly / red enemy) on both teams.

---

# QA LOOP 2 — loop-1 refuted; fault isolated to Percent→pixels (gameplay-programmer, 2026-07-10)

**Loop-1 (null BarWidget) is REFUTED by runtime proof:** art-director + the `[TASK122DIAG]` poll log show `OnHPChanged`/`SetTeamColor` EXECUTE, `BarWidget=VALID`, and `Bar.Percent` (UPROPERTY) drops 0.85→0.7 and holds under damage. The C++→BIE→`Bar.Percent(UPROPERTY)` chain is proven end to end. **Only `Percent`(UPROPERTY) → rendered pixels is unproven** — the screen-space `UWidgetComponent` presentation layer (Source/, mine). WBP exonerated.

## Measured (not assumed)
- **Castle diff:** InitWidget-order is IDENTICAL to ours (`Castle.cpp:60-101` calls `Super::BeginPlay`→`InitWidget(null)` then `SetWidgetClass`), so that hypothesis is refuted by the control. The one structural diff: castle's screen-space bar is visible from construction; ours started hidden (`SetVisibility(false)`) and toggled from the poll.
- **Engine source:** screen-space widgets paint by being added to `FWorldWidgetScreenLayer` in `UpdateWidgetOnScreen()` (`WidgetComponent.cpp:1308`), end of `TickComponent`, gated on component `IsVisible()`; for Screen space `UpdateWidget()` is a near-no-op and nothing syncs component `SetVisibility` to the widget's Slate visibility.
- **CDO read (`Default__HealthBarComponent`):** `Space=Screen, DrawSize=90x12, TickMode=Enabled, bVisible=false`. `TickMode=Enabled` means the invisible-tick-disable path (`:1264`) never fires → component ticks continuously → *weakens* the visibility-toggle theory.

## Change reviewed this loop (`HealthBarComponent.cpp/.h`)
1. **`LogDiag()` instrumentation** (the five requested measurements): widget Slate cached, `Bar` ProgressBar Slate cached + live `GetPercent`, `GetWidget()==BarWidget` identity, `IsInViewport`/`IsVisible`/`ownerHidden`/`tickEnabled`, `Space`/`DrawSize`. `[TASK122DIAG]`, TEMPORARY (TASK-128).
2. **Candidate fix (labeled, low-risk, castle-aligned, NOT asserted):** removed constructor `SetVisibility(false)`; BeginPlay seeds initial visibility from `bShowHealthBar` (`bBarShown=bShowHealthBar; SetVisibility(bShowHealthBar)`), removing the hide→show toggle for alive opted-in bars. Does not mask the log.

## MANDATORY scans (re-run on the loop-2 change): CLEAN
- **Shadow:** new locals `BarPB`, `Draw`, param `Phase`; no new data members; no inherited-reflected-UPROPERTY collision.
- **Complete-type-include:** added `Components/ProgressBar.h` (`UProgressBar::GetPercent`/`GetCachedWidget`, `Cast<UProgressBar>`) and `GameFramework/Actor.h` (`GetOwner()->IsHidden()` — the existing code only `Cast<>`d `GetOwner()`, which does not need the complete type; this new deref does). All other new calls (`GetWidgetFromName`, `IsInViewport`, `GetCachedWidget`, `CreateWidget`, `GetWidgetSpace`, `GetDrawSize`) resolve via already-included `Blueprint/UserWidget.h` / `Components/WidgetComponent.h`.

## Root cause: HONESTLY UNDETERMINED from static analysis
Leading candidates the LogDiag will discriminate in ONE PIE pass: (a) `BarSlate=0` — the `Bar` ProgressBar's `SMyProgressBar` isn't live when `SetPercent` runs (writes UPROPERTY, paints nothing — `GetPercent` reads the UPROPERTY so it can't tell); (b) `identity=0` — the on-screen `GetWidget()` differs from the `BarWidget` we push. The visibility diff is a weak candidate (`TickMode=Enabled`).

## build-master (TASK-124): read `[TASK122DIAG] LogDiag` after ONE PIE pass and record here
- fill drains (human WATCH) + `BarSlate=1`/`identity=1` → **closed** (TASK-128 strips diagnostics).
- `BarSlate=0` or `identity=0` → C++-side loop-3 fix directed by the reading.
- Final "fill visibly drains, blue/red, grey track, legible" is a **human WATCH for Jonathan** (screen-space Slate uncapturable — OVERNIGHT-AUTH §3); record OPEN, do not fake. If loop 3 fails, escalate (OVERNIGHT-AUTH §5).

---

# QA LOOP 1 — ITEM A adjudication (qa-reviewer, 2026-07-10)

**RE-REVIEW VERDICT: PASS — 0 BLOCKER · 2 WARN · 2 NIT.**
**The loop-1 C++ change is HARMLESS DEFENSIVE HARDENING, NOT load-bearing. KEEP it (no revert).**
**1b (the bar VISIBLY dropping + team-tinted) is NOT closed by this C++ — it is closed only when
TASK-127 lands. The board must NOT record 1b as fixed by TASK-122.**

Adjudicating the manager's ITEM A mandate against the `git diff` + the `[TASK122DIAG]` log + the actual
UE 5.8 engine source (I have no Git tool; I reconstructed the diff by comparing the current source against
the round-1 source I hold in-session, and I read `Engine/.../WidgetComponent.cpp` directly).

## The mandate question, answered
**"Is the loop-1 C++ change load-bearing, or harmless hardening that coincided with the symptom
disappearing?" → HARMLESS HARDENING. The C++ was never the bug.**

### 1. Line-by-line diff (loop-1 vs the round-1 code I passed) — H3 refuted
Loop-1's ONLY deltas vs round-1 are exactly three, and NONE touches the visual fill path:
1. **CreateWidget fallback** — BeginPlay `.cpp:82-89`: `bSideEffectCreated=(GetWidget()!=nullptr)`; then
   `if(!GetWidget()){ if(World) SetWidget(CreateWidget<UUnitHealthBarWidget>(World,LoadedWidgetClass)); }`.
2. **`ApplyTeamTint()` + `bTeamTintApplied`** — the round-1 inline `if(BarWidget){…SetTeamColor…}`
   refactored into an idempotent helper called from BeginPlay `.cpp:108` AND the re-resolve `.cpp:194`.
   **Same team lookup, same values, same guard condition.**
3. **`[TASK122DIAG]` `UE_LOG(Warning)` lines** — BeginPlay one-shot + throttled poll (`.cpp:97-103,204-212`).
The always-visible gate (`.cpp:165`), the `if(!BarWidget)` re-resolve (`.cpp:191-195`), and the every-poll
`OnHPChanged` push (`.cpp:216-219`) are **byte-identical to round-1.** Nothing else changed. H3 (something
else load-bearing) is refuted by reading the diff.

### 2. The log refutes the programmer's stated root cause
His loop-1 conclusion — "`SetWidgetClass` side-effect never fired → `GetWidget()` null → `BarWidget` null
every poll" — is refuted by his own instrumentation: **23/23 actors logged `side-effect-created=YES` +
`BarWidget-cast=VALID`; the explicit `CreateWidget` fallback NEVER executed.** 40 poll lines show
`Current<Max … BarWidget=VALID -> OnHPChanged PUSHED` with dropping HP (hero 200→160→60→128, archer 45→25,
cavalry 140→80). Delta 1 (the "fix") is **dead code in this run.**

### 3. H2 (non-deterministic construction) — REFUTED against the real engine source
Read `Engine/Source/Runtime/UMG/Private/Components/WidgetComponent.cpp`:
- **`SetWidgetClass` (l.2359-2381):** early-outs ONLY when `WidgetClass==InWidgetClass`; otherwise, if
  `FSlateApplication::IsInitialized() && HasBegunPlay() && !GetWorld()->bIsTearingDown`, it
  `CreateWidget`+`SetWidget` (constructs the widget).
- **`InitWidget` (l.1746-1777, called from `OnRegister`, BEFORE BeginPlay):** constructs iff
  `Slate initialized && WidgetClass && Widget==nullptr && !bIsTearingDown`; early-returns on a dedicated server.

Our component sets the SOFT `HealthBarWidgetClass` in the ctor, **NOT** the base `WidgetClass` — so base
`WidgetClass` is null at `OnRegister` ⇒ `InitWidget` builds nothing there; then our BeginPlay (after
`Super::BeginPlay` sets `HasBegunPlay=true`) calls `SetWidgetClass(loaded)` with `WidgetClass(null)!=loaded`
⇒ it **constructs.** In a PIE client/standalone world with Slate up and a non-tearing world, `GetWidget()`
is **deterministically non-null** after BeginPlay — via `SetWidgetClass` here, or (if a BP ever
pre-serialized the base `WidgetClass`) via `OnRegister::InitWidget`, in which case `SetWidgetClass`
early-outs harmlessly because the widget already exists. **There is no run-to-run-variable null path in
this game.** The only null paths — dedicated server / torn-down world — do not occur in single-player-vs-bot.
This matches DIAG 23/23=YES. So the fallback guards a case that cannot occur here; **H2 does not hold.**

### 4. The clincher — why round-1 read "frozen/untinted" and loop-1 read "working"
**TASK-123 was a NO-OP: the art-director did NOT modify `WBP_UnitHealthBar`** (board TASK-123 status:
"verified CORRECT in every dimension and NOT modified"). So the WBP asset is **byte-identical across the
round-1 and loop-1 PIE builds,** and every loop-1 C++ delta is non-visual (fallback dead, tint identical,
logs observational). **Therefore the fill rendered IDENTICALLY in both runs — round-1 was ALREADY draining
and ALREADY pushing the team tint.** The differing report is a PERCEPTION difference, not a code
difference: a WHITE fill on a white/near-white track (TASK-123 audit: `FillColorAndOpacity`=white,
`fillImage.tintColor`=white) makes the moving boundary and the tint imperceptible. Jonathan's loop-1 ask —
*"make the fill red/blue and the background grey"* — is a request for exactly the contrast that would have
made round-1's drain obvious. **This is H1.**

## The six required items
1. **Hypothesis: H1** (C++ was never the bug), on the diff + the DIAG log + the engine source + the
   TASK-123 no-op. H2 and H3 refuted above. Possibility 2 from the round-1 report (runtime-null BarWidget)
   is now also refuted (engine source + `side-effect-created=YES` 23/23).
2. **Load-bearing or harmless: HARMLESS.** 1b (VISIBLE drop + team tint) **remains OPEN until TASK-127
   lands.** Do NOT let the board record 1b as fixed by TASK-122 — the user-visible fix is ITEM B (TASK-127:
   team-tinted fill + grey track + contrast).
3. **Keep or revert: KEEP** (the manager's inclination is correct). The fallback is null-safe
   belt-and-suspenders — dead here, but a legitimate guard for a dedicated-server / torn-down-world path and
   for a future BP that pre-serializes the base `WidgetClass`; `ApplyTeamTint()` is a genuine improvement
   (it closes round-1 WARN-1). A revert is pure churn. Caveat: the `[TASK122DIAG]` logs are NOT part of the
   keeper — TASK-128 strips them (item 6).
4. **Does `bTeamTintApplied` introduce a defect? YES — dormant (NIT).** If the widget is ever destroyed and
   recreated, GC nulls `BarWidget` (UPROPERTY), the poll re-resolve reacquires the NEW instance, but
   `ApplyTeamTint()` early-outs on `bTeamTintApplied==true` ⇒ the recreated widget renders UNTINTED. The
   latch keys on "tinted once ever," not "this instance tinted." It does NOT fire in normal play (a
   Screen-space WidgetComponent's widget persists for the actor's life; `SetWidgetClass` is called once),
   so it is a NIT, not a blocker. Correct fix if ever needed: reset `bTeamTintApplied=false` whenever
   `BarWidget` is (re)assigned, or key the tint on instance identity.
5. **Standard checks on the loop-1 code:**
   - **Shadow scan CLEAN** — new members `bTeamTintApplied`(bool), `DiagPollCount`(int32); new local
     `bSideEffectCreated`; new method `ApplyTeamTint` — none collide with a `UWidgetComponent`/parent
     reflected member; no `Owner`/`Instigator`/`Slot`/`Controller`/`PlayerState` local.
   - **Complete-type-include CLEAN** — `CreateWidget<UUnitHealthBarWidget>` + `SetWidget` resolve from
     `Blueprint/UserWidget.h` (l.5) + `Components/WidgetComponent.h` (header l.6); `UUnitHealthBarWidget`
     complete (l.12). `GetNameSafe`/`GetClass` (DIAG logs) come from `UObject/…` pulled transitively via the
     already-included engine headers — standard and already PROVEN by TASK-124 phase-A ("loop-1 DLL compiled
     clean 00:51:28").
   - **No deprecated 5.8 APIs** — `CreateWidget`, `SetWidget`, `GetWidget`, `GetNameSafe` all current.
   - **Null-safety solid** — fallback guarded by `if(!GetWidget())`+`if(World)`; DIAG BeginPlay log guards
     the class deref (`GetWidget()?…:nullptr`); `ApplyTeamTint` early-outs on `!BarWidget`, `Cast<ITeamAgent>`
     null-safe, null agent ⇒ Blue default; `OnHPChanged` still behind `if(BarWidget)`.
   - **`EndPlay` clears `PollTimerHandle`** — `.cpp:120-128`, unchanged.
   - **`ApplyTeamTint` null-safety** — confirmed (guards `!BarWidget`; null-safe team lookup).
6. **`[TASK122DIAG]` logs MUST NOT SHIP — CONFIRMED.** They are `UE_LOG(..., Warning, ...)`: one line per
   actor at BeginPlay (~60 lines) PLUS a throttled poll line (first 3 polls + every 40th ≈ every 6 s) per
   actor — sustained Warning-level spam at ~60 actors, and Warning pollutes the shipping log. The
   programmer's own comments mark them TEMPORARY. **TASK-128 (gameplay-programmer) must remove EVERY
   `[TASK122DIAG]` line AND the now-orphaned `DiagPollCount` member before TASK-124 phase-B recompiles +
   commits.** The board already schedules exactly this (keep diagnostics ON through the TASK-127 color PIE;
   TASK-128 strips LAST, immediately before the commit). TASK-128 is code→QA, so it returns to me.

## Verdict rationale (why PASS, not FAIL)
No BLOCKER: the change compiles (phase-A proved it), is crash-safe, scans clean, and mutates no
combat/stat/HP/castle path. The two WARNs are pipeline-owned, not un-actioned TASK-122 code defects:
(WARN) the `[TASK122DIAG]` logs must not ship — owned by TASK-128 before the commit; (WARN) 1b is not
user-visibly closed until TASK-127. Per the manager's ITEM A mandate the deliverable is this recorded
adjudication, and a PASS may KEEP the harmless hardening. **PASS.**

## Board status line requested (I have no partial-edit tool — please proxy)
`- status: qa-passed (ITEM A) — loop-1 C++ = HARMLESS HARDENING, NOT load-bearing (H1: WBP unchanged across both PIE runs + all 3 deltas non-visual; DIAG side-effect-created=YES 23/23 + engine SetWidgetClass analysis refute the runtime-null story). KEEP (no revert). 1b NOT closed until TASK-127. [TASK122DIAG] Warning logs + DiagPollCount must be stripped by TASK-128 before TASK-124 phase-B commit. bTeamTintApplied recreate-gap = dormant NIT. Report: qa/TASK-122-report.md "QA LOOP 1 — ITEM A".`
