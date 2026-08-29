# TASK-680 — VID-002-S1 Back-fix: C++ standalone dismissal in `USessionMenuWidget::BackPressed()` — programmer handoff

- **Author:** gameplay-programmer · 2026-08-28
- **Law implemented:** the ⚖️ SESSION-BACK ruling (CONVENTIONS "Net class naming" (M8), 2026-08-28 sub-bullet) — the TASK-354 flagged decision REVERSED to C++-side dismissal.
- **Source of record:** `.claude/pipeline/footage/VID-002-multiplayer-back-inert.md`
- **Files touched (the COMPLETE set):** `Source/GitClaudeUnrealTest/Siegebound/SessionMenuWidget.h` + `Source/GitClaudeUnrealTest/Siegebound/SessionMenuWidget.cpp`. Nothing else. The `DeckBuilderWidget.*` / `DeckLibrary.*` modifications visible in `git status` are TASK-676's parallel lane — not mine, not read, not touched.
- **⛔ NOT done, by fence:** no compile (QUIET-MODULE — TASK-678 owns the shared gate), no editor, no Git write (the `git diff`/`git status` reads below are proof-gathering only), no board edits beyond the 680 status flip, no WBP/.uasset, no new BIE, no new test file.

## 1. What changed (the diff, verbatim)

### `SessionMenuWidget.cpp` — hunk 1: two explicit IWYU includes

```diff
 #include "Engine/Engine.h"
 #include "Engine/GameInstance.h"
 #include "Engine/World.h"
+#include "GameFramework/PlayerController.h"  // TASK-680: CreateWidget's owner static_assert needs the complete type
+#include "UObject/UObjectGlobals.h"          // TASK-680: LoadClass<> (explicit IWYU - no compile verifies transitive pulls)
 #include "SiegeSessionSubsystem.h"
```

Why (compile-trap defensiveness, stated for QA (g)): the `CreateWidget` template's owner-type `static_assert` walks `TIsDerivedFrom<OwnerType, APlayerController>`, which requires the COMPLETE `APlayerController` type at the instantiation point — nothing in the pre-existing include set guarantees it transitively. `LoadClass<>` lives in `UObject/UObjectGlobals.h`; same reasoning. With no compile allowed in this lane, both are included explicitly rather than gambled on transitive pulls. No other include-graph change.

### `SessionMenuWidget.cpp` — hunk 2: the standalone branch (old lines 125-128 → the fix)

```diff
-	// Plain standalone menu: Back is WBP-side panel navigation. Deliberately
-	// NOT LeaveMatch here - reloading L_MainMenu on every Back press would
-	// flicker-reset the menu for no reason (flagged in the TASK-354 handoff).
-	UE_LOG(LogSiegeNet, Log, TEXT("[SessionMenu] Back pressed - no session active; WBP handles panel dismissal."));
+	// Plain standalone menu: Back dismisses the panel HERE, C++-side.
+	// SUPERSEDED RATIONALE (2026-08-28, the SESSION-BACK ruling in
+	// CONVENTIONS, off VID-002): the TASK-354 flagged decision read "Back is
+	// WBP-side panel navigation", but the TASK-355 route-(A) zero-graph WBP
+	// never authored that navigation - measured on pixels, three full press
+	// cycles rendered and nobody closed the panel. The decision is REVERSED.
+	// The half of the old rationale that SURVIVES is the LeaveMatch refusal:
+	// reloading L_MainMenu on every standalone Back press would flicker-reset
+	// the menu for no reason, so dismissal is a viewport widget swap, never
+	// travel.
+	//
+	// The swap is the exact INVERSE of the TASK-355 open transition (the
+	// main-menu Multiplayer entry: RemoveFromParent(self) ->
+	// CreateWidget(WBP_SessionMenu_C) -> AddToViewport - the main menu is
+	// fully REMOVED from the viewport, not hidden, so Back must re-create
+	// it). Add-before-remove is LAW: the main menu enters the viewport BEFORE
+	// this panel leaves it, so no frame renders with neither widget; a failed
+	// class resolve leaves THIS panel up (ShowLocalError - one log, one error
+	// line), never a zero-UI strand. Deliberately NO SetInputMode on any
+	// path: L_MainMenu's posture (UIOnly + visible cursor) is owned by
+	// BP_MenuGameMode at level boot (Input-mode ownership law) and survives
+	// viewport widget swaps - the open transition made no input-mode call
+	// either, and VID-002 shows the swapped-in panel fully hover- and
+	// click-interactive under the surviving posture.
+	UClass* MainMenuClass = LoadClass<UUserWidget>(nullptr, TEXT("/Game/UI/WBP_MainMenu.WBP_MainMenu_C"));
+	APlayerController* OwningPlayer = GetOwningPlayer();
+	UUserWidget* MainMenu = (MainMenuClass && OwningPlayer)
+		? CreateWidget<UUserWidget>(OwningPlayer, MainMenuClass)
+		: nullptr;
+	if (!MainMenu)
+	{
+		// Panel STAYS up - the player keeps a live, clickable UI.
+		ShowLocalError(TEXT("Main menu unavailable."));
+		return;
+	}
+
+	UE_LOG(LogSiegeNet, Log, TEXT("[SessionMenu] Back pressed - no session active; returning to main menu."));
+	MainMenu->AddToViewport();
+	RemoveFromParent();
 }
```

### `SessionMenuWidget.h` — the `BackPressed` contract comment (lines 62-69) rewritten to the new truth

Old falsified text ("panel dismissal there is the WBP's own navigation") replaced with the C++-side contract: resolve → CreateWidget(owning player) → AddToViewport → THEN RemoveFromParent; add-before-remove law; failed resolve = panel stays + ShowLocalError; still never a LeaveMatch reload (that half of the old rationale survives). Dated SUPERSEDED paragraph records the TASK-354 decision and its reversal — reasoning kept, not deleted (house style). `BackPressed()` signature, UFUNCTION specifiers, and category are byte-identical.

## 2. Declared log line (TASK-678 step (3) greps for this, character-for-character)

- **NEW line (present, exactly once in Source/):**
  `[SessionMenu] Back pressed - no session active; returning to main menu.`
  (full statement: `UE_LOG(LogSiegeNet, Log, TEXT("[SessionMenu] Back pressed - no session active; returning to main menu."));` — logged AFTER a successful resolve+create, immediately before the AddToViewport/RemoveFromParent pair, so the line never lies about an impending transition that then fails.)
- **OLD line (gone):** `[SessionMenu] Back pressed - no session active; WBP handles panel dismissal.` — grep over `Source/` returns **0 hits in 0 files** (verified post-edit).
- **Resolve-failure path logs once:** the single `ShowLocalError(TEXT("Main menu unavailable."))` call — its existing `LogSiegeNet` Warning (`[SessionMenu] Main menu unavailable.`) plus the ErrorTextBlock line plus the `OnSessionErrorShown` BIE. No second log was added on that path.

## 3. The open-transition inverse argument

TASK-355 (handoff §9, the shipped §9.6 rewire) wired the main menu's Multiplayer entry as `RemoveFromParent(self) → CreateWidget(WBP_SessionMenu_C) → AddToViewport` — the main-menu WBP is **fully REMOVED from the viewport, not collapsed/hidden** (TASK-355 handoff; confirmed by VID-002 finding #2: at panel-open the main-menu buttons vanish from every probe). Therefore Back cannot un-hide anything — it must re-create `/Game/UI/WBP_MainMenu.WBP_MainMenu_C` from its class. The fix is that transition inverted, with one deliberate asymmetry: the ruling's **add-before-remove law** (the open transition removed first; the inverse adds first) so no frame ever renders with neither widget, and a resolve/create failure aborts BEFORE `RemoveFromParent()` ever runs — the failure mode is "panel still up + error line", never an empty viewport. `AddToViewport()` at default ZOrder 0 — the same Z the menu boots at (TASK-049); no Z contest, since this panel is gone within the same frame.

## 4. The focus/input-mode finding (asked for by the spec — stated)

- **The posture of record:** CONVENTIONS "Input-mode ownership (level-travel law)": *"L_MainMenu / BP_MenuGameMode: menu posture is UIOnly + visible cursor (TASK-049) so WBP_MainMenu buttons stay clickable"* — `FInputModeUIOnly` (`SetInputMode_UIOnlyEx`) + `bShowMouseCursor`, set ONCE at level boot by the game mode, not by any widget.
- **Why the fix makes NO `SetInputMode` call:** (a) posture is level/game-mode-owned; the house idiom is that widgets never call `SetInputMode` (the `USiegeAssistantConsoleWidget` class-comment law — "a widget-initiated SetInputMode caller is how a level ends up stranded in the wrong posture"); (b) the empirical proof that a viewport widget swap does not disturb the posture is VID-002 itself — the TASK-355 open transition contains zero input-mode nodes, and the footage measures the swapped-in session panel fully hover- (+32) and press- (−20) interactive under the boot posture. The inverse swap rides the same surviving posture; mouse routing in UIOnly mode goes to whatever is under the cursor and does not depend on the SetInputMode-time focus widget.
- **Residual, stated honestly:** if BP_MenuGameMode's boot `SetInputMode_UIOnlyEx` set a WidgetToFocus on the ORIGINAL WBP_MainMenu instance, the re-created instance does not hold keyboard focus (it falls back to the viewport). This is mouse-irrelevant (the dead-mouse regression class does not apply — nothing touches the viewport ignore-input latch or capture mode) and the main menu is a mouse-driven surface (VID-002 finding #1: hover tints cycle; no keyboard navigation feature exists on it). Same residual already exists after the OPEN transition today.

## 5. The LeaveMatch zero-diff proof (fence: `.cpp:117-123` byte-untouched)

- `git diff` hunk header for the branch region: `@@ -122,10 +124,45 @@` — the first changed line is OLD line 125 (the standalone comment); old lines 117-123 (`if (bNetActive || bPendingConnection) { ... Session->LeaveMatch(); return; }`) appear ONLY as unchanged context. The whole earlier function body (subsystem resolve, `bNetActive`, `bPendingConnection` detection) is untouched — the .cpp diff has exactly two hunks: the includes and the standalone branch.
- Post-edit grep (`Session->LeaveMatch\(\);` ±4 lines) shows the branch verbatim at new lines 119-125, byte-identical to the pre-edit text.

## 6. M8 declaration (standalone-branch UI — the posture, stated)

Local-client UI code only, in the widget layer. The edited branch is reachable **only** when `GetNetMode() == NM_Standalone` AND no `PendingNetGame` exists — i.e. the plain menu level, never a networked world. No replicated state, no RPC, no authority-gated mutation, no `GetFirstPlayerController()` (the owner resolve is `GetOwningPlayer()` on self — the widget's own controller). The net-active lane (`LeaveMatch`) is byte-untouched, so M8 session behavior is unchanged by construction. No replication tier applies (nothing replicates); no new violations added.

## 7. Compliance ledger (the remaining fences)

- **Escape law:** untouched — no key handling added or changed; the fix lives entirely inside the existing `OnClicked → BackPressed()` path.
- **Trailing-default law (SC-§33):** no function gained a parameter, defaulted or otherwise — `BackPressed()` signature unchanged; no call-site enumeration owed.
- **Compile-trap laws:** the C2445 `TSubclassOf`/`UClass*` two-conversion trap does not arise — the only ternary's arms are both `UUserWidget*`; the widget class is carried as a raw `UClass*` into `CreateWidget`'s `TSubclassOf<UUserWidget>` parameter (one implicit conversion, unambiguous). The two includes in hunk 1 close the incomplete-type risks that a QUIET-MODULE lane cannot compile-check.
- **Composed-path law:** `/Game/UI/WBP_MainMenu.WBP_MainMenu_C` character-for-character per the ruling.
- **No new test file:** per the ruling, UI navigation closes on the live-drive instrument + the footage lane (FR-§); TASK-678/682 own that.

## 8. QA scrutiny list (TASK-681)

1. **(b) add-before-remove:** confirm `AddToViewport()` precedes `RemoveFromParent()` in control flow AND that every early-return happens before `RemoveFromParent()` is reachable — the failure path must leave the panel up.
2. **(e) log strings:** diff the declared NEW line in §2 against the code character-for-character; grep Source/ for the OLD line (expect 0).
3. **(c) net-active branch:** re-run the §5 proof yourself (diff hunk boundaries + branch grep).
4. **Owner-null semantics:** a null `GetOwningPlayer()` is folded into the same failure path as a failed `LoadClass` (panel stays + `ShowLocalError`). Judgment call worth a look: the spec named only "resolve-failure"; I treated an unresolvable owner identically because `CreateWidget` requires a non-null owner and the never-zero-UI principle covers both.
5. **Error string:** `ShowLocalError(TEXT("Main menu unavailable."))` is the spec's exact string; confirm no drift.
6. **Includes (g):** exactly two new includes, both justified in §1; verify no other include-graph change and that `SiegeSessionSubsystem.h` stays last (module-local-last idiom preserved).
7. **Synchronous `LoadClass` on a click:** deliberate — menu level, the asset is the level's own boot UI (warm), one press per navigation; no caching member added (least-diff, no header state). Flag if you disagree.
8. **Comment truth (f):** `.h` contract + `.cpp` rationale both carry the dated supersession with the surviving LeaveMatch-refusal reasoning kept, not deleted.

## 9. State ledger

- Editor: not touched. Engine: not touched. Git: zero writes (diff/status reads only). Board: TASK-680 status → `ready-for-qa`, nothing else.
- Working-tree deltas from me: exactly `SessionMenuWidget.h` + `SessionMenuWidget.cpp` + this handoff + the one board status line.
