# TASK-074 — Menu→arena travel input loss: audit + arena-side input normalization (C++) — HANDOFF

**Agent:** gameplay-programmer
**Date:** 2026-07-07
**Status:** ready-for-qa (file-only C++ — no editor, no compile, no Git, per spec)

## VERDICT UP FRONT

- **Editor change needed: NO.** TASK-075 should be **cancelled** by the manager (rationale in §4).
- Fix shipped: `ASiegePlayerController::BeginPlay` now normalizes the input posture via the existing
  `ApplyCursorInputState()` before doing anything else. Two files touched, nothing else:
  - `Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.cpp` (BeginPlay, +1 call with comment block)
  - `Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.h` (BeginPlay doc comment only)
- `StartMatch` / `StartSandboxMatch` NOT touched (read-only audit confirmed they only OpenLevel — posture-agnostic,
  per the CONVENTIONS law).
- **Live bug-fix confirmation is Jonathan's ONE menu click** — the menu→arena path is not machine-drivable
  (TASK-073 precedent: MCP cannot click menu buttons or pass level-open URL options in PIE). Build-master
  machine-verifies only the direct-L_Arena regression (TASK-076).

## 1. Root-cause audit (spec step 0) — CONFIRMED, prime suspect correct

### 1a. Where the menu establishes its posture (the exact site)

**Asset:** `/Game/Blueprints/BP_MenuGameMode` (`Content/Blueprints/BP_MenuGameMode.uasset`), **EventBeginPlay
graph** — authored in TASK-049 (handoffs/TASK-049.md §2): `GetPlayerController(0)` → `SetShowMouseCursor(true)` →
`CreateWidget(WBP_MainMenu)` → `AddToViewport` → **`SetInputModeUIOnly(PC, focus = menu widget)`**.

Binary corroboration (ASCII scan of the .uasset performed this task):

| String in BP_MenuGameMode.uasset | Present? | Meaning |
|---|---|---|
| `SetInputMode_UIOnlyEx` | **YES** | the graph calls `UWidgetBlueprintLibrary::SetInputMode_UIOnlyEx` (the BP "Set Input Mode UI Only" node) |
| `bShowMouseCursor` | YES | cursor-on write on the menu PC |
| `WidgetBlueprintLibrary`, `CreateWidget`, `AddToViewport`, `GetPlayerController` | YES | the TASK-049 graph as documented |
| `SiegePlayerController` | **NO** | see 1b |
| `PlayerControllerClass` | **NO** | see 1b |

### 1b. Which PlayerController class L_MainMenu uses (spec question — recorded answer)

**The engine default `APlayerController`, NOT `ASiegePlayerController`.** Evidence: (i) handoffs/TASK-049.md §2
verbatim: "PlayerController class left default" on BP_MenuGameMode (parent `GameModeBase`); (ii) the binary scan
above — the asset serializes NO `PlayerControllerClass` property tag (never overridden from the GameModeBase
default) and NO reference to `SiegePlayerController` anywhere. **Consequence:** the spec's (2)(d) STOP-and-escalate
clause does NOT trigger — `ASiegePlayerController` never exists in L_MainMenu, so an arena-side normalization in
its BeginPlay is inherently arena-scoped and cannot affect menu clickability.

### 1c. Why the posture survives OpenLevel travel (mechanism, engine-source reasoning — sanctioned by the spec)

`APlayerController::SetInputMode(InData)` forwards to `InData.ApplyInputMode(LocalPlayer->GetSlateOperations(),
*GameViewportClient)`. `FInputModeUIOnly::ApplyInputMode` (engine `PlayerController.cpp`) writes three things onto
the **UGameViewportClient**:

1. `GameViewportClient.SetIgnoreInput(true)` ← the killer
2. `GameViewportClient.SetCaptureMouseOnClick(EMouseCaptureMode::NoCapture)`
3. `GameViewportClient.SetMouseLockMode(<mode>)`

The `UGameViewportClient` (and the `ULocalPlayer`) are owned by the GameInstance/engine layer, **not** the world —
`UGameplayStatics::OpenLevelBySoftObjectPtr` (SiegeGameMode.cpp:696 StartMatch / :732 StartSandboxMatch) performs
an absolute travel that destroys the world, the menu `APlayerController`, and the menu widget, but the viewport
client and its `bIgnoreInput` / capture-mode / lock-mode fields carry over untouched. Nothing in the travel path
resets them.

**Resulting failure state in L_Arena:** `UGameViewportClient::InputKey`/`InputAxis` early-out while
`IgnoreInput()` is true, so no key/axis event ever reaches the player input stack → WASD dead, hotkeys 1–6 dead
(Enhanced Input receives nothing), LMB dead; `NoCapture` means no mouse-look either; the fresh
`ASiegePlayerController` spawns with default `bShowMouseCursor=false`, so there isn't even a cursor. Total input
death — exactly Jonathan's symptom, on BOTH buttons (both route through the same OpenLevel travel; the Sandbox=1
option is irrelevant, confirming manager ruling 1).

**Why direct-PIE L_Arena works:** a fresh PIE session creates a fresh viewport client — `bIgnoreInput` defaults
false, capture/lock modes come from `UInputSettings` defaults (`CapturePermanently_IncludingInitialMouseDown` /
`LockOnCapture`). And pre-fix `ASiegePlayerController::BeginPlay` set no input mode at all (verified: the only
`SetInputMode` sites in the class were HandleMatchEnd's UIOnly block, cpp ~746 post-edit / ~729 pre-edit, and
`ApplyCursorInputState()`, cpp ~1609 post-edit / ~1562 pre-edit) — the arena simply inherited whatever the
viewport had, which was healthy on a direct boot and poisoned after menu travel.

## 2. The fix (spec step 1)

`SiegePlayerController.cpp`, `BeginPlay`: **first statement after `Super::BeginPlay()`** is now
`ApplyCursorInputState();` with a comment block citing TASK-074 + the CONVENTIONS law. On a fresh controller
`bInPlacementMode` / `bUICursorHeld` / `bMatchEnded` are all false, so the call takes the existing
else-branch verbatim: `bShowMouseCursor=false`, `bEnableClickEvents=false`,
`SetInputMode(FInputModeGameOnly())`. `FInputModeGameOnly::ApplyInputMode` writes
`SetIgnoreInput(false)` + `SetCaptureMouseOnClick(CapturePermanently_IncludingInitialMouseDown)` +
`SetMouseLockMode(LockOnCapture)` + focuses the game viewport — i.e. it resets **all three** persistent viewport
fields the menu poisoned. No parallel path invented (manager fix-direction ruling followed); no new members, no
new locals, no signature changes.

`SiegePlayerController.h`: `BeginPlay`'s doc comment extended to document the normalization contract. No other
declaration touched.

**Acceptance mapping:** a fresh `ASiegePlayerController` beginning play in L_Arena now applies GameOnly + hidden
cursor REGARDLESS of prior viewport/input state — the normalization overwrites the persistent fields
unconditionally, not conditionally.

## 3. Why NO extension beyond ApplyCursorInputState() (spec step 1 "extend minimally if insufficient")

Audited the candidate residuals; every one is either reset by the fix or cannot survive travel:

| Residual state | Where it lives | Survives travel? | Handled? |
|---|---|---|---|
| `bIgnoreInput` | UGameViewportClient (persistent) | YES | reset by FInputModeGameOnly::ApplyInputMode |
| Mouse capture mode (NoCapture) | UGameViewportClient | YES | reset (CapturePermanently_IncludingInitialMouseDown) |
| Mouse lock mode | UGameViewportClient | YES | reset (LockOnCapture) |
| `bShowMouseCursor` / `bEnableClickEvents` | APlayerController | NO (controller destroyed + respawned) | fresh defaults false; ApplyCursorInputState re-writes false anyway |
| IgnoreLook/IgnoreMove counters | APlayerController::PlayerInput state | NO | fresh controller = zeroed counters |
| Stuck pressed keys (`FlushPressedKeys` candidate) | UPlayerInput (per-controller) | NO | fresh UPlayerInput — nothing to flush; adding a flush would be dead code |
| Slate keyboard focus on the menu widget | Slate / widget | NO (widget destroyed with the world) | FInputModeGameOnly additionally SetUserFocus(viewport) |

Hence: reusing `ApplyCursorInputState()` alone is sufficient; nothing was extended (spec permits extension only
when proven insufficient).

## 4. Editor change needed: NO — TASK-075 cancellation rationale (spec step 4)

The menu side is **correct as authored**: per the CONVENTIONS law, L_MainMenu's posture IS UIOnly + visible cursor
(that is what keeps WBP_MainMenu clickable), and the static travel entries are correctly posture-agnostic. The
defect was solely the arena trusting inherited viewport state, which the C++ normalization fixes at the root
(arena owns its posture now, regardless of which level traveled in — menu today, anything else tomorrow). There is
no BP_MenuGameMode / WBP_MainMenu / L_MainMenu edit that would add anything: a menu-side "restore GameOnly before
travel" node would flash game input under the menu and re-introduce the exact trust-the-traveler antipattern the
law forbids. **Manager: flip TASK-075 to cancelled; TASK-076 proceeds without waiting.**

## 5. Regression contract walkthrough (spec step 2, ruling 4) — all six by inspection

- **(a) Alt-held IA_UICursor:** OnUICursorPressed/Released and ApplyCursorInputState internals untouched. The
  BeginPlay call happens once at spawn, before any Alt hold can exist; later holds compose exactly as before.
- **(b) Placement-mode cursor + click-confirm:** EnterPlacementMode/ExitPlacementMode → ApplyCursorInputState
  untouched; bEnableClickEvents/GameAndUI branch byte-identical.
- **(c) HandleMatchEnd UIOnly + PlayAgain restore:** HandleMatchEnd (sets bMatchEnded, then its own UIOnly block)
  untouched; ApplyCursorInputState's bMatchEnded guard untouched, so nothing can override the end screen;
  HandleMatchReset untouched. BeginPlay runs only at controller spawn — PlayAgain resets in place (no respawn), so
  the new call never fires mid-flow.
- **(d) L_MainMenu buttons clickable:** the fix lives in ASiegePlayerController, which does not exist in
  L_MainMenu (§1b — engine-default APlayerController there, proven by handoff + binary scan). Arena-scoped by
  construction; the STOP-and-escalate clause did not trigger.
- **(e) M1 WARN-4 clickability posture:** the normalized posture IS the WARN-4 posture — GameOnly free-look, HUD
  non-interactive until Alt/placement shows the cursor. Unchanged.
- **(f) Direct-PIE L_Arena feel byte-identical:** on a fresh boot every value written already equals the default
  (ignore-input false, same capture/lock modes from UInputSettings defaults, cursor flags false) — a no-op by
  value. The one nuance QA should be aware of: the FInputModeGameOnly slate ops (viewport focus + lock-to-widget
  request) are now queued once at boot; this is the exact same code path M1's PlayAgain already exercises via
  HandleMatchReset → ApplyCursorInputState in live PIE, where it demonstrably does not disturb feel (mouse capture
  still engages on first click, LockOnCapture means no lock until capture).

## 6. Flagged-decision list for QA (explicit rulings requested)

1. **Normalization placed FIRST in BeginPlay** (before deck build + HUD creation) — deliberate: posture is
   established before any match UI enters the viewport; deck/HUD logic is input-mode-independent, so ordering is
   about intent, not correctness.
2. **Reused ApplyCursorInputState() rather than a raw SetInputMode(FInputModeGameOnly())** — per the manager's
   preferred implementation; also re-writes bShowMouseCursor/bEnableClickEvents=false, which are fresh-controller
   defaults (no-op writes on direct boot, real fixes after travel if any future traveler leaves click events on).
3. **No FlushPressedKeys / no ignore-counter clearing added** — §3 table: every candidate residual either lives on
   the destroyed-and-respawned controller (nothing to clear) or on the viewport client (cleared by the fix).
   Adding them would be dead code on every path.
4. **StartMatch / StartSandboxMatch untouched** (spec step 3 default) — audit proved the travel call itself is
   fine; the poison is viewport state, not the travel.
5. **Header edit is doc-comment only** — no declaration/UPROPERTY/signature changes anywhere.
6. **Shadow law (C4457/58/59):** the edit introduces ZERO new locals, parameters, or loop variables — nothing can
   shadow an inherited reflected UPROPERTY. QA scan should come back trivially clean on the diff.

## 7. What QA / build-master should scrutinize

- QA: confirm the ApplyCursorInputState else-branch really is reachable at BeginPlay with all three flags false
  (they are in-class-initialized false; no code path sets them before BeginPlay).
- Build-master (TASK-076): machine-verify ONLY the direct-L_Arena regression (WASD, 1–6, Alt-cursor, placement,
  match-end → PlayAgain). The menu→arena click is **Jonathan's WATCH** — expected result: either button →
  L_Arena with full WASD + hotkeys + cards, cursor hidden, Alt-cursor working.
- Post-fix line references for TASK-076's readbacks: normalization call = SiegePlayerController.cpp ~77 (inside
  BeginPlay, first statement after Super); HandleMatchEnd UIOnly block ~746; ApplyCursorInputState ~1609.

## Files touched

- `Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.cpp` — BeginPlay: +ApplyCursorInputState() with
  audit-comment block (only functional change).
- `Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.h` — BeginPlay doc comment.
- NOT touched: SiegeGameMode.h/.cpp, all other Source files, all Content assets, Config, TASKBOARD.md.
