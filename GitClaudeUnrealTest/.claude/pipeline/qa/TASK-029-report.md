# QA Report — TASK-029 — Card hand widget C++ base: UCardHandWidget
Verdict: PASS

Reviewed: `Source/GitClaudeUnrealTest/Siegebound/CardHandWidget.h`, `Source/GitClaudeUnrealTest/Siegebound/CardHandWidget.cpp`
Against: TASKBOARD TASK-029 spec + names block, M2 manager decisions, CONVENTIONS.md (widget-base pattern, delegate law, data-driven law), handoffs/TASK-029.md (11 flagged decisions), handoffs/TASK-023.md + landed SiegePlayerController.h/.cpp (the seam), handoffs/TASK-022.md + DeckComponent.h/.cpp (frozen contract), SiegePlayerState.h (TASK-005), CardRow.h (TASK-021), qa/TASK-005-report.md major 2, qa/TASK-018-report.md rulings 3/5/8 (precedents).
Review deliberately held until TASK-023 landed; paired with the concurrent TASK-023 review — this report owns the widget side of the seam.

Blockers: 0 · Warnings: 1 (carry-forward constraint on TASK-033, not a defect here) · Nits: 3

## Spec compliance (verified)

- **Class/files/names**: `UCardHandWidget : UUserWidget` in `Siegebound/CardHandWidget.h/.cpp`; functions `InitForController` / `RequestPlaySlot` / `RequestDiscardSlot`; BIEs `OnHandSlotUpdated(int32, const FString&, const FString&, int32, bool)` / `OnNextCardUpdated(const FString&, int32)` / `OnCardRefusedMessage(const FString&)` — all character-exact to the names block and spec. CONVENTIONS §Widgets-with-C++-bases satisfied (`UCardHandWidget` ↔ `/Game/UI/WBP_CardHand`, TASK-033).
- **BIE params MCP-authorable**: int32/FString/bool only — no enums, no structs, no FText, no FName crosses the BP boundary. Bonus safety: TASK-033 implements *inherited* events, so MCP never authors a param list at all.
- **Seed-then-bind (qa/TASK-005-report.md major 2)**: InitForController seeds (`RefreshAllHandSlots()` + `PushNextCardPreview(PeekNextCardID())`, cpp:65-69) strictly BEFORE the four `AddUniqueDynamic` binds (cpp:74-85). All four spec'd sources bound: OnDeckHandChanged, OnDeckNextCardChanged, OnGoldChanged, OnCardRefused. Refusals are events, not state — correctly nothing to seed.
- **Deck contract honored (TASK-022)**: payload-less OnDeckHandChanged → coarse re-pull of all slots via `GetHandCardID(0..GetHandSize()-1)` (no hardcoded 6 — uses the ruling-4 getter); every OnDeckNextCardChanged broadcast treated as authoritative with NO widget-side value filter (cpp:125-131) — exactly what TASK-022 flagged decision 7 requires; draws are never inferred from the preview delegate.
- **Affordability (§3.5)**: `bAffordable = ObservedPlayerState && CanAfford(Row->Cost)`; recomputed on every OnGoldChanged through the single shared `PushHandSlot` path. OnGoldChanged fires post-mutation (TASK-005 verified SetGold order), so no stale-read window. Gold pinned at cap produces no broadcast → no wasted re-push.
- **Data-driven law (§3.0)**: Cost/DisplayName read from `/Game/Data/DT_Cards.DT_Cards` rows at push time (soft path, LoadSynchronous, null-safe); nothing typed into UMG. `FCardRow::DisplayName` confirmed `FString` (CardRow.h:49) and `Cost` `int32` (CardRow.h:57) — the assignments compile (an FText DisplayName would have been a blocker; checked).
- **Acceptance walk**: (a) seeds 6 slots + preview before any delegate can fire — ordering is intra-function, unconditional; (b) gold dropping below a cost flips that slot's bAffordable on that very broadcast; (c) exactly one OnCardRefusedMessage per refused action on the widget side — single AddUniqueDynamic binding (re-inits cannot double-bind; re-targets drop old bindings first) + 1:1 forward in HandleCardRefused. (Controller-side one-broadcast-per-refusal is the paired TASK-023 review's confirmation.)
- **UE 5.8 API**: clean. UUserWidget FObjectInitializer ctor (the ONLY ctor UUserWidget offers — required, not stylistic); `FindComponentByClass<T>`, `AController::GetPlayerState<T>`, `TSoftObjectPtr::LoadSynchronous`, `UDataTable::FindRow<T>(FName, ctx, bWarnIfRowMissing=false)`, `AddUniqueDynamic`/`RemoveDynamic`, `GetNameSafe` — all current. UFUNCTION() present on all four dynamic-delegate handlers (reflection requirement). TObjectPtr + Transient on all UObject members (GC-safe); `TSet<FName>` non-UPROPERTY is fine (no GC concern). generated.h last; cpp includes own header first; no circular includes (controller header only included in the cpp).
- **Build.cs**: untouched and correctly so — "UMG" confirmed at line 20, "Engine" present. Handoff claim verified.
- **No tick, no timers, no per-frame work** — fully event-driven. Log category `LogGitClaudeUnrealTest` exists (GitClaudeUnrealTest.h:8).

## SEAM — TASK-029 ↔ TASK-023 (landed controller), character-for-character

| # | Widget side | Controller side (landed) | Verdict |
|---|---|---|---|
| 1 | `HandleCardRefused(const FString& Reason)` (h:159), bound via AddUniqueDynamic (cpp:85) | `DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCardRefused, const FString&, Reason)` (SiegePlayerController.h:38); public `UPROPERTY(BlueprintAssignable) OnCardRefused` (h:96-98) | MATCH — `const FString&` ↔ `const FString&`, exact; dynamic-delegate binds require this and get it. Widget correctly does NOT bind the older M1 `OnCardPlayRefused(FName, FText)` |
| 2 | `RequestPlaySlot` → `ObservedController->PlayHandSlot(Slot)` (cpp:100) | `void PlayHandSlot(int32 Slot)` public BlueprintCallable (h:111-112) | MATCH |
| 3 | `RequestDiscardSlot` → `DiscardHandSlot(Slot)` (cpp:113) | `void DiscardHandSlot(int32 Slot)` public BlueprintCallable (h:122-123) | MATCH |
| 4 | `Controller->FindComponentByClass<UDeckComponent>()` (cpp:39) | `CreateDefaultSubobject<UDeckComponent>(TEXT("DeckComponent"))` (SiegePlayerController.cpp:41); member is protected | RESOLVES — FindComponentByClass searches OwnedComponents regardless of member visibility; default subobjects are registered there. The additive public `GetDeckComponent()` (h:126-127) is a future convenience, not a correction |
| 5 | Seed-then-bind at widget Construct | Controller BeginPlay: `BuildAndShuffle()` (cpp:64-71) BEFORE HUD `CreateWidget`/`AddToViewport` (cpp:79-100), comment citing the seed-then-bind law | MATCH — real flow seeds a dealt hand; reverse order ALSO safe (widget seeds empty, then `OnDeckHandChanged.Broadcast()` + `BroadcastPreview(force=true)` on build, DeckComponent.cpp:98-99, repaint it) |
| 6 | `HandleDeckHandChanged()` / `HandleDeckNextCardChanged(FName)` | `FOnDeckHandChanged` (no params) / `FOnDeckNextCardChanged(FName)` (DeckComponent.h:23/33) | MATCH |
| 7 | `HandleGoldChanged(int32)`, `CanAfford(int32)` | `FOnGoldChanged(int32, NewGold)` public BlueprintAssignable; `CanAfford` public BlueprintPure (SiegePlayerState.h:14/33/41) | MATCH |

**Seam verdict: CLEAN — all cross-task symbols match character-for-character; the compile unit (TASK-039 batch) is consistent on this seam.**

## Rulings on the 11 flagged decisions

1. **FindComponentByClass over direct member access — PASS.** Null-safe, visibility-independent, resolves the default subobject, once per widget lifetime (nothing per-frame). Survives TASK-023's later getter addition unchanged.
2. **Empty slot = empty FString, never "None" — PASS.** `CardID.IsNone()` branch precedes any `ToString()` on both the slot path (cpp:176-183) and preview path (cpp:208-213); the `NAME_None.ToString()` → `"None"` ambiguity is unreachable. Non-empty degrade emits the raw CardID (non-empty by construction). Documented in the BIE comment + TASK-033 contract.
3. **Gold re-grey = stateless full re-push through one shared path — PASS, cost ACCEPTED at this tier.** ~6 BIE calls + 6 TMap FindRow lookups per income tick (1 Hz) + per spend — trivial. A stateless re-push cannot hold a stale grey; the spec pins exactly three BIEs (no room for a separate affordability event); the CONVENTIONS value-change filter governs delegate *broadcasts*, not view pushes — correct reading. Binding consequence on TASK-033 recorded as WARN-1 below.
4. **Missing row/table degrade (raw CardID, cost 0, greyed; once-per-fault logs) — PASS.** On-screen identifiable, unplayable-looking, crash-safe. Log growth bounded: one bool for the table + a TSet bounded by distinct dealt CardIDs (≤ table row count). Suppression never suppresses retries — lookups re-run every push, so a mid-session TASK-031 reimport + ResetDeck self-heals silently.
5. **Null-deck / null-player-state at init: warn + serve what is servable — PASS.** No fabricated refusal messages (keeps one-message-per-refusal provable); unknown gold renders conservative grey; and OnCardRefused is still bound even with a null deck/player state (cpp:85 sits outside both guards) — the refusal surface survives degraded modes. Good detail.
6. **Request* pass-throughs with no local validation — PASS.** Spec's literal "pass-throughs"; a widget-side pre-check would be a second refusal authority that can race a same-frame gold change. Out-of-range slots are handled downstream (deck logs + refuses; controller refuses empty slots with a broadcast).
7. **Re-target + AddUniqueDynamic + NO NativeDestruct unbind — PASS.** Direct precedent: qa/TASK-018-report.md ruling 8 (ACCEPTED). Dynamic multicast delegates hold weak object references and skip/compact dead listeners — no dangling-callback risk on widget teardown. Lifetimes are coupled anyway: the HUD (hosting this widget) is created in controller BeginPlay and lives until EndPlay; HandleMatchReset does not recreate it. A removed-but-alive widget receiving pushes is a harmless no-op-visible cost. The classic dangling-delegate failure needs raw/native binds or timers — none exist here.
8. **Pre-build seed window — PASS, with one factual correction (NIT-1).** Crash-safe: verified in DeckComponent.cpp:102-115 — in-range slots (0..5 vs HandSize 6) on the pre-build empty Hand take the `IsValidIndex ? : NAME_None` branch. But the handoff's claim that this seed "logs one Warning per slot" is wrong: the deck's out-of-range warning fires only for slots OUTSIDE [0..HandSize-1]; the pre-build in-range reads return NAME_None silently (deck cpp:112-114 comment says so explicitly). Outcome is strictly better than documented — zero log noise — and the self-heal is unchanged (build's forced broadcasts repaint). The "self-diagnosing log" property claimed in the handoff does not exist; nothing depends on it. Doc fix only.
9. **Plain `UCLASS()`, not Abstract — PASS.** Identical rationale QA accepted in TASK-018 ruling 3; TASK-033's MCP reparent flow is still unproven against Abstract parents; non-abstract costs nothing. Keep until the reparent tooling is proven.
10. **HandleGoldChanged ignores NewGold — PASS.** Single code path for seed/hand/gold refreshes; OnGoldChanged broadcasts post-mutation so CanAfford already reads the new value; no drift window exists.
11. **FObjectInitializer constructor — PASS**, noting it is REQUIRED, not a choice: UUserWidget declares only the FObjectInitializer form, so a default ctor would not compile. Soft default `/Game/Data/DT_Cards.DT_Cards` byte-identical to the controller/deck pattern; EditDefaultsOnly override intact.

## Findings

- **[WARN-1 — carry-forward constraint on TASK-033, not a defect in this code]** CardHandWidget.h:38-42/119 + cpp:133-142 — by design (decision 3), `OnHandSlotUpdated` fires for all 6 slots with mostly-identical data on EVERY gold change (~1 Hz income + each spend) and every hand change; `OnNextCardUpdated` re-fires on build/reset even when unchanged. TASK-033's UMG implementations MUST be idempotent pure setters (text/enabled/tint only) — a per-call animation, sound, or "card flip" trigger in these events would visibly pulse at 1 Hz. Already contract-documented in the header and handoff; recorded here so TASK-033's QA explicitly checks it. Same treatment as prior cross-task WARN carry-forwards (TASK-022 WARN-1 → TASK-023).
- **[NIT-1]** handoffs/TASK-029.md flagged decision 8 (line 91) — the "one Warning per slot" claim for the pre-build seed is factually wrong (see ruling 8; behavior is silent + safe). Correct the handoff text at leisure; no code change.
- **[NIT-2]** CardHandWidget.cpp:253 (`ResolveCardRow`) — `CardTableAsset.LoadSynchronous()` runs once per slot push (6x per refresh, ~6/s). On a loaded table this is a StaticFindObject by path — trivial at this rate, but hoisting one resolve per `RefreshAllHandSlots` pass (table passed into `PushHandSlot`/`ResolveCardRow`) would be marginally cleaner. Optional; do not hold the batch for it.
- **[NIT-3]** CardHandWidget.cpp:219-225 — the preview's hide signal is "empty DisplayName" (spec-pinned), which collides with the hypothetical data fault of a PRESENT row carrying an empty DisplayName cell: the preview would hide while a next card exists. TASK-021's audited CSV has no empty DisplayNames and TASK-031's acceptance verifies row values, so this is data-contract-guarded. Optional hardening: fall back to `NextCardID.ToString()` when a resolved row's DisplayName is empty. (Slot faces are immune — their hide decision is CardID-keyed per the handoff contract.)

## Notes for build-master

- Compile-safe as written for the TASK-039 batch: new file pair only, normal UBT pickup; no Build.cs change (UMG line 20 verified); compiles only together with TASK-023's controller files (designed parallel-dev pattern) — both sides of the seam verified consistent above.
- Grep-tool note for the record: some tool outputs rendered leading `//` comment markers in SiegePlayerController.cpp/DeckComponent.cpp as `\`; direct file reads confirmed correct `//` in every checked instance (controller cpp:105/661, deck cpp:28) — display artifact, not file content.
- Post-compile PIE sanity (pre-TASK-031/033, widget asset absent by design): nothing visual changes yet; no new log spam expected from this class (its warnings require a live widget instance, which arrives with TASK-033).
- After TASK-031 + TASK-033: full acceptance run — 6 seeded slots + preview at boot, 1-Hz re-grey flips exactly at affordability boundaries, one refusal message per refused play/discard, empty-slot faces hidden via empty CardID string (never the literal "None").
- Carry WARN-1 into TASK-033's spec/QA checklist.

## Board status line (orchestrator applies — paired QA in flight, board not edited by QA)

`- status: qa-passed (qa/TASK-029-report.md — 0 blockers, 1 warn, 3 nits; all 11 flagged decisions ruled PASS; TASK-023 seam verified character-for-character CLEAN; WARN-1 = TASK-033 idempotent-BIE constraint, carry-forward)`

---

## BUILD-MASTER — COMPILE FAILURE (TASK-039 batch, 2026-07-04)

**Verdict: qa-failed.** The M2 C++ batch (TASK-021..030) was compiled together via the standard Build.bat (`GitClaudeUnrealTestEditor Win64 Development -waitmutex`) after an editor bounce. The build **halted at the UnrealHeaderTool stage** (before any .cpp compile/link) with two hard errors, both in this task's `CardHandWidget.h`. Nothing was committed; the editor was left down. This counts as a QA loop — routing back to gameplay-programmer.

**Exact compiler output:**

```
C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Source\GitClaudeUnrealTest\Siegebound\CardHandWidget.h(98): Error: Function parameter: 'Slot' cannot be defined in 'RequestPlaySlot' as it is already defined in scope 'UWidget' (shadowing is not allowed)
C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Source\GitClaudeUnrealTest\Siegebound\CardHandWidget.h(106): Error: Function parameter: 'Slot' cannot be defined in 'RequestDiscardSlot' as it is already defined in scope 'UWidget' (shadowing is not allowed)
Unhandled 1 aggregate exceptions

Result: Failed (OtherCompilationError)
```

**Root cause (diagnosis for the programmer — build-master does not edit code):** `UCardHandWidget` derives from `UUserWidget` → `UWidget`, and `UWidget` already declares a reflected `UPROPERTY` member named `Slot` (the panel slot). UHT forbids a `UFUNCTION` parameter from shadowing an inherited reflected property, so the `int32 Slot` parameters on the two `BlueprintCallable` pass-throughs `RequestPlaySlot(int32 Slot)` (h:98) and `RequestDiscardSlot(int32 Slot)` (h:106) are rejected. Only the WIDGET side is affected — the controller-side `PlayHandSlot(int32 Slot)` / `DiscardHandSlot(int32 Slot)` (`ASiegePlayerController`, an `APlayerController` with no `Slot` member) are fine and do NOT need to change. Fix is scoped to renaming the two parameters in `CardHandWidget.h` and `CardHandWidget.cpp` (e.g. `SlotIndex` / `InSlot`); the seam signatures the QA report verified are otherwise unchanged (only the local parameter identifier moves).

**Important — batch not fully verified past UHT:** because UHT failed first, the C++ compiler never ran on the rest of the TASK-021..030 batch. Once this is fixed and TASK-039 is re-dispatched, additional compile/link errors elsewhere in the batch may surface for the first time.

---

## BUILD-MASTER — COMPILE FAILURE #2 (TASK-039 batch, attempt #2, 2026-07-04)

**Verdict: qa-failed (again).** After the attempt-#1 UHT fix (UFUNCTION params `Slot→SlotIndex`) cleared the header shadow, attempt #2 got **past UHT and into the C++ compile stage**, where `CardHandWidget.cpp` failed with **two more C4458 "declaration hides class member" errors** (UE treats C4458 as a hard error via `ShadowVariableWarningLevel = Error`). These are a *different* pair of `Slot` shadows than attempt #1 — same `UWidget::Slot` inherited member, but now the offenders are a **local loop variable** and a **private helper's parameter** inside the .cpp (which UHT does not inspect — only the C++ compiler catches them). The attempt-#1 build-fix note's claim that a "proactive sibling-shadow scan found NONE beyond these two" did not cover these .cpp-internal `Slot` identifiers. Nothing committed; HEAD stays `4f95730`; editor left down. This is TASK-029's second build-fix loop (per-task counter 2/3); routing back to gameplay-programmer.

**Exact compiler output (from the build log):**

```
[7/19] Compile [x64] CardHandWidget.cpp
C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Source\GitClaudeUnrealTest\Siegebound\CardHandWidget.cpp(162,13): error C4458: declaration of 'Slot' hides class member
	for (int32 Slot = 0; Slot < SlotCount; ++Slot)
	           ^
C:\Program Files\Epic Games\UE_5.8\Engine\Source\Runtime\UMG\Public\Components\Widget.h(264,25): note: see declaration of 'UWidget::Slot'
	TObjectPtr<UPanelSlot> Slot;
	                       ^
C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Source\GitClaudeUnrealTest\Siegebound\CardHandWidget.cpp(168,42): error C4458: declaration of 'Slot' hides class member
void UCardHandWidget::PushHandSlot(int32 Slot)
                                         ^
C:\Program Files\Epic Games\UE_5.8\Engine\Source\Runtime\UMG\Public\Components\Widget.h(264,25): note: see declaration of 'UWidget::Slot'
	TObjectPtr<UPanelSlot> Slot;
	                       ^

Result: Failed (OtherCompilationError)
```

**Root cause (diagnosis for the programmer — build-master does not edit code):** same inherited member as attempt #1 — `UWidget::Slot` (`TObjectPtr<UPanelSlot>`), the panel slot every `UWidget` carries — but shadowed this time by two identifiers the UHT-only fix missed:
- line 162: the `for (int32 Slot = 0; ...)` loop counter in `RefreshAllHandSlots` (or equivalent). Rename to `SlotIndex` (matching the public API rename) or `i`.
- line 168: the parameter of the private helper `void UCardHandWidget::PushHandSlot(int32 Slot)` — this is declared in both `.h` and `.cpp`, so rename it in **both** (e.g. `int32 SlotIndex`) to keep the declaration/definition signatures matched.

Both are local identifiers, not stats/DT values or seam symbols, so the rename is mechanical and does not disturb any seam the QA verified. **Suggested for the programmer:** do a full `Slot` sweep across the whole `CardHandWidget.h/.cpp` pair (not just UFUNCTION params) so a third attempt does not surface yet another internal `Slot`; the compiler flags every declaration that hides an inherited reflected member, including plain locals and helper params UHT never sees.

**Batch status:** all 14 other translation units compiled clean; the ONLY two files with errors this attempt are `CardHandWidget.cpp` (this task, 2 errors) and `MinerUnit.cpp` (TASK-025, 4 `Owner`/`PlayerState` shadows — see qa/TASK-025-report.md). The compiler processed every TU before failing, so this is the complete compile-stage error set. Link did not run, so link-stage errors remain unverified until both files compile clean and TASK-039 is re-dispatched (attempt #3).

