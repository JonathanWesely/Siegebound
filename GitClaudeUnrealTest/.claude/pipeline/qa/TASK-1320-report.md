Verdict: PASS — 0 BLOCKER / 2 WARN / 5 NIT

# QA Report — TASK-1320 (gate over TASK-1319)

**subject:** `TASK-1319` — [MATCHEND-UIONLY-SOFTLOCK]
**marker:** `TASK-1320-MATCHEND-UIONLY-GATE`
**reviewer:** qa-reviewer, 2026-09-20
**host:** ✅ `TASK-1321` (compile · suite · 5c) — `SC-§71b` named in §9 below
**file under review:** `Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.cpp` — `HandleMatchEnd` only

---

## 0. INSTRUMENT DECLARATION — WHAT I EXECUTED vs WHAT I DERIVED (`SC-§71b`)

⛔ **I have no `Bash` tool in this dispatch.** Every git-shaped claim in the handoff is therefore
**re-derived structurally or accepted as declared**, and I say which for each one. I ran **no** git command,
**no** compile and **no** suite.

⛔ **The `unreal_inspector` lane was UNAVAILABLE**: its `grep` returned
`{"error": "No valid session token found. Please ensure you are logged in."}` on first call. **It was not needed** —
this row is pure C++ text and I reached UE 5.8 engine source directly with `Read`/`Grep` at
`C:/Program Files/Epic Games/UE_5.8/Engine/Source/...`. ⛔ I did **not** route git or a shell through the inspector.

**EXECUTED BY ME (first-hand reads):** all UE 5.8 engine-source reads in §2 · all
`SiegePlayerController.cpp` / `.h` reads and greps · the `SC-§121` token census over the added region ·
the address-shift arithmetic in §1 · the `qa/TASK-007-report.md` verbatim comparison ·
the cross-check against `handoffs/TASK-1314-programmer.md` (a source **committed at `1d433ca`**, i.e. one that
could not have been shaped by this row).

**ACCEPTED AS DECLARED (`SC-§71b`) — `TASK-1321` owns the definitive measurement:**
`git diff --numstat` = `66 0` · `git diff -U0 | grep -c "^-[^-]"` = `0` · the one-hunk claim ·
the tree census · suite total `561` · compile `Result: Succeeded`.
⚠️ Corroborated (not produced) by the **session-start `gitStatus` snapshot** handed to me, which shows exactly one
`Source/` path modified — `Siegebound/SiegePlayerController.cpp` — **no `.h`, no `Content/`, no `.uasset`, no
`Saved/`, no `CONVENTIONS.md`, no `.uproject`, no `settings.local.json`, and no new untracked test file.**

---

## 1. 🚨 CHECK (1) — THE SUCCESS PATH IS BYTE-IDENTICAL. ✅ **CONFIRMED, AND CONFIRMED INDEPENDENTLY.**

The handoff claims proof by construction (66 added / 0 deleted, one hunk). ⛔ I did not take that on its word.
**I proved it from a source the row could not have touched.**

### 1.1 The independent proof: a UNIFORM `+66` SHIFT across the whole tail of the file

`handoffs/TASK-1314-programmer.md` §7(c) — **written and committed at `1d433ca`, before `TASK-1319` existed** —
records all five posture-line addresses as they stood **after** `TASK-1314`'s edit and **before** this one.
I measured the same five lines on disk today:

| line, quoted | pre-`TASK-1319` address (source: `TASK-1314` handoff §7(c), committed `1d433ca`) | measured on disk now | Δ |
|---|---|---|---|
| `bShowMouseCursor = true;` | 2273 | **2339** | **+66** |
| `bEnableClickEvents = true;` | 2274 | **2340** | **+66** |
| `FInputModeUIOnly InputMode;` | 2275 | **2341** | **+66** |
| `InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);` | 2329 | **2395** | **+66** |
| `SetInputMode(InputMode);` | 2330 | **2396** | **+66** |
| `InputMode.SetWidgetToFocus(PlayAgainSlate);` (`TASK-1314` handoff §, `// :2312`) | 2312 | **2378** | **+66** |

and six more, from the `TASK-1319` handoff's own pre-edit reads, every one of them also exactly `+66`:

| citation | cited (pre-edit) | measured now | Δ |
|---|---|---|---|
| `VictoryWidget = nullptr;` in `HandleMatchReset` | 2374 | **2440** | **+66** |
| `ApplyCursorInputState`'s `if (bMatchEnded)` early-out | 6293 | **6359** | **+66** |
| `ApplyCursorInputState`'s GameAndUI arm | 6345–6348 | **6411–6414** | **+66** |
| `CanOpenAssistantConsole` `return !bMatchEnded` | 6370 | **6436** | **+66** |
| `CanOpenWarMap` `return !bMatchEnded` | 6657 | **6723** | **+66** |
| `CanOpenControlsHelp` `return !bMatchEnded;` | 7072 | **7138** | **+66** |

⭐ **A uniform `+66` displacement holding from line ~2273 all the way to 7138 is possible ONLY IF the file received
exactly one insertion of exactly 66 lines above 2273 and had ZERO net line change anywhere else.** A deletion, a
second hunk, or an added line below the insertion point would break the uniformity at the first address past it.
⇒ **`66 added / 0 deleted / one hunk` is CONFIRMED by me, not inherited.**

### 1.2 The text itself, quoted from disk (check (1) asks for the quote)

The four posture lines `TASK-1314`'s acceptance pins, plus its focus call, **as they stand on disk after this row's
edit** — compared character-for-character against `TASK-1314`'s own quoted block:

```cpp
2339		bShowMouseCursor = true;
2340		bEnableClickEvents = true;
2341		FInputModeUIOnly InputMode;
…
2351		if (VictoryWidget)
2358				static const FName PlayAgainButtonName(TEXT("Btn_Jump"));
2359			if (UWidget* PlayAgainButton = VictoryWidget->GetWidgetFromName(PlayAgainButtonName))
2363				const TSharedRef<SWidget> PlayAgainSlate = PlayAgainButton->TakeWidget();
2376				if (PlayAgainSlate->SupportsKeyboardFocus())
2378					InputMode.SetWidgetToFocus(PlayAgainSlate);
…
2395		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
2396		SetInputMode(InputMode);
2398		UE_LOG(LogGitClaudeUnrealTest, Log,
2399			TEXT("ASiegePlayerController '%s': match ended — winner %s."),
2400			*GetNameSafe(this), Winner == ETeamId::Blue ? TEXT("Blue") : TEXT("Red"));
```

✅ Identical in text to `TASK-1314`'s shipped block. **Not one instruction on the widget-exists path changed** —
`SetWinner` (`:2202`) / `SetLocalVictory` (`:2229`) / `AddToViewport(10)` (`:2245`) all sit **above** the hunk and are
structurally untouched. 🧑 **Jonathan's two hand-confirmed routes (click Play Again → restart; Enter → Play Again →
restart) are therefore not at risk: the code they exercise is the same code.**

### 1.3 The one residual, stated plainly rather than papered over

The `+66` arithmetic cannot detect an **in-place edit of equal line count** (a character changed inside one of
`TASK-1314`'s lines). I mitigated it by comparing the **text** of every line `TASK-1314` quoted (above — all match).
What remains uncompared is the *comment prose and the two Warning strings inside the focus block*, which
`TASK-1314`'s handoff does not quote at byte level. That residual is covered by the declared
`grep -c "^-[^-]"` = `0` (accepted per `SC-§71b`) and will be surfaced definitively by ⭐ `TASK-1321`'s
`git show --stat HEAD`. ⛔ **If that stat shows any deletion on this file, this PASS does not cover it.**

⛔ **`VictoryWidget->TakeWidget()` occurrences in the file: `0`** — MEASURED by me. The file's only `TakeWidget`
is `PlayAgainButton->TakeWidget()` at `:2363`, which is `TASK-1314`'s. **`TASK-1311`'s deleted call was NOT re-added.** ✅
⛔ **`SetWidgetToFocus` count: `1`** — MEASURED, matching `TASK-1314`'s recorded `= 1`. ✅

---

## 2. 🚨 CHECK (3) — THE SHIPPING-BUILD SEVERITY CHAIN, VERIFIED AT ENGINE SOURCE. ✅ **IT HOLDS.**

⛔ I re-read every link myself in UE 5.8 (`SC-§91`). **Every cited address is correct and every quote is accurate.**

| link | address, read by me | what I found |
|---|---|---|
| UI-only turns input off | `Engine/Private/PlayerController.cpp:6384` | `GameViewportClient.SetIgnoreInput(true);` inside `FInputModeUIOnly::ApplyInputMode` (`:6372`) ✅ |
| the enforcer (`SC-§70`) | `Engine/Private/GameViewportClient.cpp:767-770` | `if (IgnoreInput()) { return ViewportConsole ? ViewportConsole->InputKey(...) : false; }` ✅ **verbatim as quoted** |
| …and it really is *before* the controller | same file, `:772` / `:789` / `:799` | the early-out precedes `OnInputKeyEvent.Broadcast` (`:772`), `OnOverrideInputKeyEvent` (`:789`) and `TargetPlayer->PlayerController->InputKey(EventArgs)` (`:799`) ✅ ⇒ **no key reaches the PC, so none reaches Enhanced Input** |
| console constructed only under a flag | `GameViewportClient.cpp:2807-2809` | `#if ALLOW_CONSOLE` / `ViewportConsole = NewObject<UConsole>(...)` ✅ |
| the flag in Shipping | `Core/Public/Misc/Build.h:348` and `:215` | Shipping block: `#define ALLOW_CONSOLE ALLOW_CONSOLE_IN_SHIPPING`; and `#define ALLOW_CONSOLE_IN_SHIPPING 0` ✅ |
| the other surfaces refuse | `SiegePlayerController.cpp:6436` / `:6723` / `:7138` | `CanOpenAssistantConsole` / `CanOpenWarMap` / `CanOpenControlsHelp` each begin `return !bMatchEnded …` ✅ |
| nothing else owns the posture | `SiegePlayerController.cpp:6359` | `ApplyCursorInputState` early-outs on `bMatchEnded` **by design** ✅ |

⇒ ⛔ **THE SEVERITY CLAIM IS CONFIRMED, NOT ASSERTED. In `Siegebound-Win64-Shipping` the degraded state was a
TOTAL input blackout — no screen, no button, no key, only Alt+F4.** In editor/Development the tilde console is the
sole survivor.

⭐ **AND THE HANDOFF UNDERSTATED IT.** I found two links it did not claim, both in its favour:
- `UGameViewportClient::InputAxis` (`:815`), `InputTouch` (`:928`) and `InputGesture` (`:968`) return **`false`
  outright** under `IgnoreInput()` — **with no console fallback at all**. So it is not only keys: **axis, touch and
  gesture input are dead too**, in Development as well as Shipping.
- The `bMatchEnded` latch has **eight** further refusal sites in this controller (`:989`, `:1038`, `:1385`, `:1498`,
  `:1714`, `:1889`, `:3419`, `:3927`) on top of the three `CanOpen*` — corroborating "every card play is refused".

⇒ **Check (3) PASSES on the strongest reading.** This was **not** a cosmetic WARN, and `SC-§50`'s judgement to
board it after three declarations is vindicated by the source.

---

## 3. CHECK (2) — THE CHANGE IS SCOPED TO THE DEGRADED BRANCH. ✅ **CONFIRMED.**

The whole payload is one guarded block at `:2302-2325` ending in `return;`, inserted **after** both degraded `else`
branches and **before** the UI-only block:

```cpp
2302	if (!VictoryWidget)
2303	{
2304		bShowMouseCursor = true;
2305		bEnableClickEvents = true;
2306
2307		FInputModeGameAndUI DegradedInputMode;
2308		DegradedInputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
2309		DegradedInputMode.SetHideCursorDuringCapture(false);
2310		SetInputMode(DegradedInputMode);
…
2324		return;
2325	}
```

**The predicate is sound and I checked it rather than accepting it (`SC-§70`):**
- `VictoryWidget` has exactly **two** assignments in the file — MEASURED: `:2195` `CreateWidget<UUserWidget>` (inside
  the class-resolved branch only) and `:2440` `= nullptr` in `HandleMatchReset` under `if (IsLocalController())`.
- `AddToViewport(/*ZOrder=*/10)` at `:2245` is the **last, unconditional** statement of the create-success branch.
- `HandleMatchEnd` returns early for a non-local controller (`:2177-2180`) and re-entry is latched out (`:2162-2168`),
  so the end/reset pairing holds and a **stale** non-null from a previous match is impossible.
⇒ **`!VictoryWidget` ⟺ "nothing was put on screen this match"**, exactly and only. ✅

**Nothing the early return skips is lost:** after `:2325` the function contains only the UI-only posture block, the
`if (VictoryWidget)` focus block (which would be skipped anyway) and the winner log (re-emitted at `:2321-2323`).
The function ends at `:2401`. ✅

**⭐ TERM-FOR-TERM COPY CONFIRMED** against `ApplyCursorInputState`'s GameAndUI arm, which I read at `:6406-6414`:

```cpp
6403	bShowMouseCursor = bWantCursor;                                            // true
6404	bEnableClickEvents = bWantCursor;                                          // true
6411		FInputModeGameAndUI InputMode;
6412		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
6413		InputMode.SetHideCursorDuringCapture(false);
6414		SetInputMode(InputMode);
```
Identical in every term; only the local variable name differs (`DegradedInputMode`). ✅

**And the stranding argument is TRUE AT SOURCE:** `FInputModeGameAndUI::ApplyInputMode`
(`PlayerController.cpp:6398`) calls **`GameViewportClient.SetIgnoreInput(false);` at `:6410`** — MEASURED. ✅
Keys are delivered again rather than discarded at the viewport.

**No stuck posture on recovery:** `HandleMatchReset` clears `bMatchEnded` at `:2428` and calls
`ApplyCursorInputState()` at `:2479`, which recomputes the posture from its own owner ladder. ✅

---

## 4. CHECK (5) — FENCES, AND THE `SC-§121` CENSUS RE-DERIVED BY ME. ✅ **EMPTY SET CONFIRMED.**

Census run over the **added region only** (`:2261-2326`), by token, first-hand:

| token | occurrences in `:2261-2326` | how I know |
|---|---|---|
| `EKeys::` | **0** | all 12 `EKeys::` lines in the file are at `:770, 803, 819, 835, 847, 873, 893, 921, 3301, 3305, 4068, 4072` — none in range. File total **12**, matching the declared pre-edit baseline ✅ |
| `EKeys::Escape` | **0 added** | the 4 occurrences (`:803, 835, 873, 893`) are pre-existing and match `TASK-1314`'s recorded baseline of **4** ✅ |
| `IA_` | **0** | nearest hits `:2157` and `:2443`, both outside ✅ |
| `IMC_` | **0** | nearest hits `:1865` and `:609`ff, outside ✅ |
| `BindAction` | **0** | all hits ≤ `:779` ✅ |
| `UEnhancedInput` | **0** | hits at `:644`, `:785` only ✅ |
| `VictoryWidget->TakeWidget` | **0** | file-wide ✅ |
| `SetWinner` / `SetLocalVictory` | **0 / 0** | both at `:2202` / `:2229`, above the hunk, untouched ✅ |

⇒ ⛔ **`SC-§121` CENSUS = EMPTY SET, INDEPENDENTLY RE-DERIVED. No key is bound, rebound, absorbed or made live.
Nothing to escalate to Jonathan. `AS-§6 A-2` untouched.** ✅

**Other fences:** no `.uasset` and no `Content/` path in the change (corroborated by the session-start snapshot) ⇒
no undeclared `SC-§125` keystroke ✅ · Play Again button / `OnClicked` / `SetWinner` / `SetLocalVictory` diff = 0 ✅ ·
`SiegePlayerController.h` unmodified ⇒ header/cpp consistency intact ✅ · `Saved/**` absent ✅.
⚠️ `TASKBOARD.md` and `Tools/run_suite_bounded.ps1` **are** dirty in the tree, but they belong to the **parallel guard
chain** (`TASK-1335`/`1336`/`1337`) and to board flips; the handoff §11 names both and states it staged nothing.
**Held, not orphaned** (`TL-§5e` cl. 7a-v). ⛔ `TASK-1321`'s pre-flight owns the authoritative census.

---

## 5. CHECK (4) — THE REMEDY WAS CHOSEN, NOT INHERITED. ✅ **CONFIRMED.**

Taken: **shape (i)+(ii) combined** — apply UI-only *only* when a widget exists, and keep game input live otherwise.
Rejected, each with a reason: **(iii) synthesise a minimal exit** (would require a fresh binding — `SC-§121`
escalation — and is structurally self-defeating under any UI-only posture) · **an `Escape` absorb** (fenced at
boarding; `AS-§6 A-2`) · **calling `ApplyCursorInputState()`** (a measured no-op here — it early-outs on
`bMatchEnded`, the `SC-§36.1` shape) · **removing `TASK-1314`'s now-redundant guard** (byte-identity fence).

⛔ I verified the `ApplyCursorInputState` rejection at source rather than accepting it: the early-out is real, at
`:6359`, and the comment above it says `HandleMatchEnd` owns the end-screen state. **Calling it would have compiled,
reviewed clean and done nothing** — the rejection is correct and is recorded in-code so it is not "simplified" back.
The row's enumeration was non-binding and I am **not** faulting the shape (`SC-§59` cl. 5). ✅

---

## 6. ⚖️ RULING ON THE TWO DECLARED TRADES (§10) — **BOTH UPHELD. NEITHER IS TO BE RESHAPED. NO FOLLOW-UP ROW.**

I am ruling explicitly so neither becomes an unowned nit (`SC-§50`).

### TRADE 1 — the now-always-true `if (VictoryWidget)` guard at `:2351`. ⚖️ **UPHELD. KEEP IT.**
**Measured:** after the early return at `:2324`, control reaches `:2351` only with `VictoryWidget` non-null, and
nothing between `:2325` and `:2351` is executable (comments only). So yes — **provably always-true today.**
**Ruling and why it is not merely tolerated but correct:**
1. It is a **guard on a `UPROPERTY(Transient) TObjectPtr<UUserWidget>` member** (declared at `SiegePlayerController.h:3591-3592` — I checked; GC-safe, correctly reflected). If any future edit reintroduces a path
   reaching `:2351` with it null, this guard is what stops `GetWidgetFromName` dereferencing null. **Removing a
   correct null check on a member pointer to buy tidiness is a bad trade in any review.**
2. Removing it would re-indent ~42 lines that `TASK-1314` shipped one commit ago and that 🧑 Jonathan hand-confirmed
   live — **destroying the exact byte-identity this gate exists to verify**, and converting a pure insertion into a
   mixed insert/modify diff.
⇒ ⛔ **NOT a BLOCKER, NOT a follow-up row, and I do not want it "cleaned up" on a later touch either.** The
rationale is already in-code at `:2261-2301`.

### TRADE 2 — the duplicated `match ended — winner %s.` log (`:2321-2323` vs `:2398-2400`). ⚖️ **UPHELD.**
**Measured:** the two sites are **textually identical** and **mutually exclusive** (the early return guarantees
exactly one fires per match end), so no observer can ever see both or a diverging pair *within one run*.
The alternatives both cost byte-identity: an `else` wrapper re-indents `TASK-1314`'s block; hoisting the log
re-orders the success path's log sequence.
⇒ ⛔ **UPHELD as the cheaper cost, and no row.** See WARN-2 for the one correction I am putting on the record about
how the "no consumer" measurement was characterised.

### (Unasked third, ruled anyway) — `FInputModeGameAndUI` rather than `FInputModeGameOnly` (§10 item 3). ⚖️ **CORRECT AS WRITTEN.**
`qa/TASK-007-report.md`'s WARN asks in its own words for *"GameAndUI + cursor so the session stays inspectable"*.
GameOnly would hide the cursor and contradict the provenance this row exists to discharge. **Keep GameAndUI.**

---

## 7. CHECK (6) — THE SUITE REASONING. ✅ **VERIFIED AT SOURCE, NOT REFUTED. NO DISCRIMINATING TEST IS AVAILABLE.**

**Declared:** `UNTESTABLE-IN-SUITE`, suite **561 → 561 (±0)**, no test added, reconciled by name as the empty set
(`SC-§104`). I ran no suite (⛔ `TASK-1321`'s leg).

⛔ **I verified the mechanism rather than the conclusion.** `APlayerController::SetInputMode`, read by me at
`Engine/Private/PlayerController.cpp:6454-6468`:

```cpp
6454	void APlayerController::SetInputMode(const FInputModeDataBase& InData)
6455	{
6456		UGameViewportClient* GameViewportClient = GetWorld()->GetGameViewport();
6457		ULocalPlayer* LocalPlayer = Cast< ULocalPlayer >( Player );
6458		if ( GameViewportClient && LocalPlayer )
6459		{ … }        // ← the ENTIRE body, nothing outside it
6468	}
```

✅ **The whole body is inside that guard**, exactly as declared. A headless `-nullrhi` world has no game viewport
client ⇒ the call is a **no-op on both old and new code** ⇒ a test could not fail on `HEAD` (`SC-§39`).
⭐ **And there is a SECOND layer the handoff did not claim:** both `FInputModeUIOnly::ApplyInputMode` (`:6374`) and
`FInputModeGameAndUI::ApplyInputMode` (`:6400`) wrap their own bodies in `if (ViewportWidget.IsValid())`, so even a
fabricated viewport client would not produce the observable. The two headlessly-writable members
(`bShowMouseCursor`, `bEnableClickEvents`) are set **`true` on both sides** (old `:2339-2340`, new `:2304-2305`) —
**MEASURED, they cannot discriminate.** ✅

⛔ **I actively hunted for a discriminator and found only one candidate, which dies at the same guard** — recorded
here so it is not re-discovered as an open question (NIT-5): `FInputModeGameAndUI::ShouldFlushInputOnViewportFocus()`
returns **`false`** (`PlayerController.h:196`) where the base returns **`true`** (`h:110`), so
`bShouldFlushInputWhenViewportFocusChanges` *would* differ old-vs-new — **but it is written at
`PlayerController.cpp:6461`, inside the very same `if (GameViewportClient && LocalPlayer)`**. It is unreachable
headlessly, and it is a proxy flag rather than the claim ("keys are delivered"). ⇒ **It corroborates the
declaration instead of refuting it.**

⇒ ⛔ **NO BLOCKER. `UNTESTABLE-IN-SUITE` stands, and writing a green pin over this path would have spent the claim.**
The handoff's rejection of a source-text regex probe (brittle, can pass for the wrong reason) is also correct.

---

## 8. FINDINGS

- **[WARN] `handoffs/TASK-1319-programmer.md` §6 — mixed-instant evidence in the single most load-bearing quote.**
  The quoted hunk header `@@ -2258,6 +2258,71 @@` is arithmetically inconsistent with the quoted `--numstat 66 0`:
  6 context lines + 66 added = **72**, not 71. `+…,71` encodes **65** added lines. Explanation found and confirmed:
  §5 records that a late comment reword *"is the only reason the diff is 66 lines and not 65"* — so the hunk header
  **and** §6's post-edit addresses (`:2338`/`:2339`/`:2394`/`:2395`, each exactly **1 low** against the measured
  `:2339`/`:2340`/`:2395`/`:2396`) were captured at the 65-line instant, while `--numstat` was re-run after.
  ⛔ **The substance is unaffected — I proved 66-added/0-deleted independently in §1** — but the artifact a gate is
  invited to rely on was stale on arrival. **Fix: re-capture every git artifact at ONE instant, after the last
  keystroke (`SC-§91`).** Not a blocker: the claim is true and I verified it another way.

- **[WARN] `handoffs/TASK-1319-programmer.md` §7 — "no consumer" is measured correctly but the conclusion drawn from
  it is broader than the measurement.** The claim *"no consumer anywhere in `Source/`, `Tools/` or the suite"* is
  **literally TRUE** (I re-ran it). But the conclusion *"nothing reads it that could drift"* overreaches: the string
  `match ended — winner` is quoted as **runtime evidence** in `qa/TASK-1314-verify.md`, `qa/TASK-1311-verify.md` and
  `qa/TASK-1068-verify.md` — **this project's `playtest-verifier` lane demonstrably reads this exact line.**
  ⛔ It does **not** change my ruling (the two sites are mutually exclusive and textually identical, so a log
  scraper gets exactly one hit either way, and the degraded site cannot fire in a healthy build). **It does mean the
  two literals now have a real downstream reader and must be kept in sync.** Named remedy **for a future touch of
  this function only, explicitly NOT now and explicitly NOT a row**: extract the winner record into one private
  `LogMatchEnded(ETeamId)` helper at the point where re-indentation is no longer a cost.

- **[NIT] `handoffs/TASK-1319-programmer.md` §1 — an address is mislabelled as pre-edit.** *"after `1d433ca` the
  posture block starts at `:2338`"* — the true pre-edit address is **2273**, per `TASK-1314`'s own committed handoff
  §7(c); `2338` is the *post-edit-at-65-lines* address. Same root cause as WARN-1.

- **[NIT] `SiegePlayerController.cpp:2290`** — the new comment still contains the bare word `Escape` in prose
  (*"the closed `Escape` ruling at `AS-§6 A-2` is left exactly as it is"*), although §5 says the comment was reworded
  precisely so a grep census would stay clean. ⛔ **Not a fence breach** — the fence is `EKeys::Escape`, whose added
  count is **0** and whose file total is **4**, unchanged and matching `TASK-1314`'s recorded baseline. But a naive
  `grep -c Escape` census would hit it. Naming it so nobody re-opens it as a finding later.

- **[NIT] `SiegePlayerController.cpp:2302-2310`** — the degraded posture deliberately takes
  `ApplyCursorInputState`'s **cursor-up** arm even though, at that moment, **no cursor owner is live** (placement,
  targeting, group pick, war map and help were all torn down at `:2141-2160`). That is intentional and right — the
  point is an inspectable session, and it is what the WARN asked for — but it is a deliberate deviation from that
  function's composition rule. The in-code comment covers it; flagging so nobody later "fixes" it to compose.

- **[NIT] the rejected discriminator** — `ShouldFlushInputOnViewportFocus` differs `true`/`false` between the two
  modes (`PlayerController.h:110` vs `:196`) but is written inside the same dead guard. **Recorded as closed, not
  open** (§7). No row.

**UE 5.8 API validity:** ✅ `FInputModeGameAndUI`, `SetLockMouseToViewportBehavior`, `SetHideCursorDuringCapture`,
`EMouseLockMode::DoNotLock`, `SetInputMode` — all present and **non-deprecated** in 5.8 (read at
`PlayerController.h:180-200`, `PlayerController.cpp:6398-6414`). **Zero deprecated API; `C4996` risk nil.**
**Compile risk: nil** — no new `#include` is needed because `FInputModeGameAndUI` is already constructed in this very
translation unit at `:6411`. **GC safety:** no new members; `VictoryWidget` is correctly
`UPROPERTY(Transient) TObjectPtr<UUserWidget>`. **Performance:** no per-tick work, no `LoadObject` in a hot path.
**Conventions:** `LogGitClaudeUnrealTest` category, existing naming, no new names introduced.

---

## 9. CHECK (7) — ACCEPTED-AS-DECLARED, AND THE DECLARED CEILING

**Accepted as declared, owner ⭐ `TASK-1321`** (`SC-§71b`, itemised in §0): the git `--numstat`/`-U0` figures ·
the one-hunk claim · the tree census · suite `561` · compile `Result: Succeeded`.

⛔ **I CONFIRM THE DECLARED CEILING RATHER THAN RE-DERIVING IT (`VER-§8` cl. 2):** `TASK-1319` (4c) pre-authorised
**`UNOBSERVABLE`** on the 5b leg at boarding, with its reason — **the degraded state only occurs when the victory
screen fails to load, which is exactly what does not happen in a healthy build** — and there is **deliberately no
🧑 hand check**, because asking Jonathan to break his own install to watch a fallback is not an ask.
⇒ ⛔ **`TASK-1321` records `verify: unobservable (declared at boarding — degraded path unreachable in a healthy
build)` and routes to 5c. ⛔ IT IS NEVER READ AS A PASS.**
⭐ And the success path 🧑 he *did* confirm by hand is byte-identical (§1), so nothing he verified is at risk.

---

## 10. NOTES FOR BUILD-MASTER (⭐ `TASK-1321`)

1. ⛔ **One check I could not run and you must:** `git show --stat HEAD` after 5c — **the deletion count on
   `SiegePlayerController.cpp` must be `0`.** My byte-identity proof is structural (§1) plus an accepted-as-declared
   `grep -c "^-[^-]"`; your stat is the definitive instrument. **A non-zero deletion count invalidates this PASS.**
2. Expected diff shape: **exactly one `Source/` file, +66 / −0, one hunk.** No `.h`. **No test file was added** —
   suite delta must be **±0 (561 → 561)**, reconciled as the empty set on both sides (`SC-§104`).
3. ⛔ **Not this row's, do not stage:** `Tools/run_suite_bounded.ps1`, `handoffs/TASK-1335-programmer.md`,
   `qa/TASK-1336-report.md`, `handoffs/TASK-1333-buildmaster.md` (the parallel guard chain), and any `TASKBOARD.md`
   content beyond the three flips. ⛔ **Never any `.uasset`** — this row is code-only by construction.
4. ⛔ **Git root is ONE LEVEL UP** (`SC-§102`); commit **by pathspec**, never `-a`, never a push.
5. ⛔ **Three flips** (`SC-§103`): `TASK-1319` · `TASK-1320` · `TASK-1321` — **commit before you edit the board**.
   ⚠️ `SC-§127`: I measured **2 identical** `- status: backlog — ⛔ **BOARDED, ⛔ NOT DISPATCHED.**` full lines on the
   board while doing my own flips. **Anchor on a task-ID-bearing or `blocked-by` line; `replace_all` is banned.**

---

**Verdict: PASS. 0 BLOCKER / 2 WARN / 5 NIT.** The fix is correct, minimal, scoped to the degraded branch, copied
term-for-term from the controller's own in-match posture, binds nothing, and restores key delivery at the viewport.
The success path is byte-identical and I proved it from a source this row could not have touched. The
Shipping-build severity chain holds at engine source on its strongest reading. Both declared trades are upheld.
