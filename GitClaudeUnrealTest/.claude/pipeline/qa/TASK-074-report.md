# QA Report — TASK-074 (Menu→arena travel input loss: arena-side input normalization)
Verdict: **PASS**

Counts: **0 BLOCKER / 0 WARN / 1 NIT**

Reviewed: `Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.cpp` (BeginPlay 56–115, normalization call at 76; ApplyCursorInputState 1580–1610; HandleMatchEnd 678–760; HandleMatchReset 762–802; OnUICursorPressed/Released/ClearUICursorHold 272–306; Enter/ExitPlacementMode call sites 635/675), `SiegePlayerController.h` (full), `SiegeGameMode.cpp` 655–744 (StartMatch/StartSandboxMatch, read-only), `Config/DefaultInput.ini`, `Config/DefaultEngine.ini`, `Content/Blueprints/BP_MenuGameMode.uasset` (binary string scan), handoffs/TASK-049.md, TASKBOARD spec, CONVENTIONS "Input-mode ownership (level-travel law)".

## Findings

- [NIT] handoffs/TASK-074.md §1c/§2 — the handoff names the viewport API `SetCaptureMouseOnClick`; the actual UE5 `UGameViewportClient` API written by `FInputMode*::ApplyInputMode` is `SetMouseCaptureMode` (the mechanics and values described are otherwise correct). Handoff-text imprecision only; no code impact, no fix required.

No blockers. No warns. No deprecated/removed UE 5.8 APIs in the change (the insertion is one call to an existing function plus comments; `SetInputMode`/`FInputModeGameOnly` are current API). No new null-deref surface (see ruling 1). Naming/conventions clean.

## Root-cause verification (audit step 0) — CONFIRMED

1. **Menu poisons persistent state:** BP_MenuGameMode.uasset contains `SetInputMode_UIOnlyEx` (independently re-scanned this review: 1 hit). `FInputModeUIOnly::ApplyInputMode` writes `SetIgnoreInput(true)`, mouse-capture `NoCapture`, and the lock mode onto the `UGameViewportClient`, which is owned by the engine/GameInstance layer and survives `OpenLevelBySoftObjectPtr` absolute travel. `UGameViewportClient::InputKey/InputAxis` early-out while `IgnoreInput()` — total input death matching Jonathan's symptom on BOTH buttons (both route through the same travel; Sandbox=1 irrelevant — manager ruling 1 confirmed).
2. **Pre-fix arena set no input mode:** the only `SetInputMode` sites in the class are HandleMatchEnd's UIOnly block (cpp 755) and ApplyCursorInputState (1603/1608), plus the new BeginPlay normalization — matches the manager's evidence base.
3. **Fresh-controller assumption verified against the header/constructor, not the claim:** `bInPlacementMode`/`bUICursorHeld`/`bMatchEnded` are in-class-initialized `false` (h:592/595/598); the constructor (cpp 30–54) never touches them; no code path can set them before BeginPlay (their only writers are input handlers and HandleMatchEnd, all post-BeginPlay). So the BeginPlay call takes the else-branch exactly: `bShowMouseCursor=false`, `bEnableClickEvents=false`, `SetInputMode(FInputModeGameOnly())`.
4. **The fix actually clears the latch:** `FInputModeGameOnly::ApplyInputMode` writes `SetIgnoreInput(false)` + `SetMouseCaptureMode(CapturePermanently_IncludingInitialMouseDown)` (bConsumeCaptureMouseDown defaults true) + `SetMouseLockMode(LockOnCapture)` + slate ops focusing the game viewport — all three poisoned persistent fields reset unconditionally. Acceptance "GameOnly + hidden cursor REGARDLESS of prior viewport/input state" holds.
5. **Menu PC class (spec question):** engine-default `APlayerController`. Independently corroborated: BP_MenuGameMode.uasset has ZERO occurrences of `SiegePlayerController` and ZERO of `PlayerControllerClass` (no override serialized), and handoffs/TASK-049.md:35 says "PlayerController class left default". The spec (2)(d) STOP-and-escalate clause correctly did not trigger.

## Ordering / null-safety at BeginPlay

- `Super::BeginPlay()` first, normalization as the first statement after it — correct and deliberate. `APlayerController::BeginPlay` does not touch input modes, so nothing re-poisons after the call; calling before Super would be the risky variant and was not done.
- Nothing dereferenced is uninitialized in ANY map: ApplyCursorInputState reads only the three in-class-initialized flags, writes two engine-initialized `APlayerController` members, and `SetInputMode` is internally guarded (`GameViewportClient && LocalPlayer` — no-ops on dedicated server / non-local controller). `FInputModeGameOnly::ApplyInputMode` additionally early-outs if the viewport widget is invalid. Zero deref risk.
- LocalPlayer availability: in the `UEngine::LoadMap` travel flow (the bug case) `SpawnPlayActor` (which attaches the ULocalPlayer) runs before `World->BeginPlay()`, so the mode applies. Empirical proof from the same function: this BeginPlay already does `CreateWidget(this)`+`AddToViewport` and the HUD demonstrably appears in every PIE flow since M1 — LocalPlayer is attached by BeginPlay in all this project's flows.
- Other spawners of this controller: `GlobalDefaultGameMode=SiegeGameMode` (DefaultEngine.ini), so a non-arena map without an override (e.g. the template Lvl_ThirdPerson) would also run the normalization — harmless there (GameOnly + hidden cursor equals engine-default posture; all calls null-safe). L_MainMenu is exempt by its BP_MenuGameMode override.
- `ASiegeBotController : public AAIController` (SiegeBotController.h:77) — not a subclass; the bot never runs this BeginPlay.

## Flagged-decision rulings (handoff §6 — all six ruled)

1. **Normalization placed FIRST in BeginPlay (after Super):** PASS. Correct ordering (above); deck build and HUD creation are input-mode-independent, and putting posture before the HUD enters the viewport is the right intent.
2. **Reused ApplyCursorInputState() instead of raw SetInputMode(FInputModeGameOnly()):** PASS. It is the spec's preferred implementation verbatim, keeps a single input-posture owner (no parallel path), and additionally normalizes `bShowMouseCursor`/`bEnableClickEvents` — no-op writes on direct boot, real protection against any future traveler that leaves click events on.
3. **No FlushPressedKeys / no ignore-counter clearing:** ACCEPTED. Residual-state table verified line by line: the three viewport-client fields are the only travel survivors and all three are rewritten by the fix; `bShowMouseCursor`/`bEnableClickEvents`/ignore-move-look counters/`UPlayerInput` pressed-key state live on the menu's controller, which is destroyed by absolute travel — the fresh `ASiegePlayerController` has zeroed counters and an empty key map, so a flush would be dead code on every path; Slate focus on the menu widget dies with the widget/world and GameOnly re-focuses the viewport anyway. Argument is sound.
4. **StartMatch / StartSandboxMatch untouched:** PASS. Read both in full (SiegeGameMode.cpp 666–697 / 699–733): they match the TASK-071-committed behavior exactly (StartMatch: no options; StartSandboxMatch: `bAbsolute=true` + `TEXT("Sandbox=1")`), and neither sets any input mode — posture-agnostic per the law. Caveat: QA has no git access, so "byte-identical" is verified by inspection against the committed spec/handoffs; the final byte check is build-master's `git diff` at TASK-076 (ONLY SiegePlayerController.h/.cpp may appear).
5. **Header edit doc-comment only:** PASS by inspection. BeginPlay's declaration (h:241) is signature-unchanged (`virtual void BeginPlay() override`, protected); the only TASK-074 references in the header are inside the doc comment (h:231–240); no new members/UPROPERTY/UFUNCTION/includes. Same git-diff caveat as ruling 4.
6. **Shadow law (C4457/58/59):** PASS. The insertion is a 17-line comment block plus one argumentless call — zero new locals, parameters, or loop variables. Whole-of-BeginPlay scan: the only local is the pre-existing `HUDClass`, which shadows no inherited reflected UPROPERTY. Clean.

## Regression contract — evidence per item (spec step 2)

- **(a) Alt-held IA_UICursor:** OnUICursorPressed (272–287), OnUICursorReleased (289–297), ClearUICursorHold (299–306) untouched; the GameAndUI branch of ApplyCursorInputState (1595–1604) byte-identical. BeginPlay fires once at spawn, before any hold can exist; later holds compose exactly as before. PASS.
- **(b) Placement-mode cursor + click-confirm:** EnterPlacementMode sets `bInPlacementMode=true` then calls ApplyCursorInputState (635) → GameAndUI + `bEnableClickEvents=true`; ExitPlacementMode restores (675). Both untouched. PASS.
- **(c) HandleMatchEnd UIOnly + PlayAgain restore:** HandleMatchEnd (678–760) sets `bMatchEnded=true` before its own UIOnly block (746–755), and ApplyCursorInputState's `bMatchEnded` guard (1584–1587) means nothing can override the end screen. PlayAgain resets in place (HandleMatchReset 762–802 — clears the latch then re-applies) with NO controller respawn, so the new BeginPlay call never fires mid-flow. All untouched. PASS.
- **(d) L_MainMenu buttons clickable:** the fix lives in a class that does not exist in L_MainMenu (ruling above: engine-default APlayerController, zero `SiegePlayerController`/`PlayerControllerClass` strings in the asset). Arena-scoped by construction. PASS.
- **(e) M1 WARN-4 posture:** the normalized state IS the WARN-4 posture (GameOnly free-look, click events off, HUD interactive only under Alt/placement cursor). The GameAndUI/else branches are byte-identical. PASS.
- **(f) Direct-PIE L_Arena feel byte-identical:** proven no-op by value against the project's own config — `Config/DefaultInput.ini:77-78` sets `DefaultViewportMouseCaptureMode=CapturePermanently_IncludingInitialMouseDown` and `DefaultViewportMouseLockMode=LockOnCapture`, exactly what `FInputModeGameOnly` writes; `bIgnoreInput` defaults false on a fresh viewport; cursor/click flags written false equal fresh-controller defaults. The once-at-boot slate ops (viewport focus + lock request) are the same code path M1's PlayAgain already exercises live via HandleMatchReset → ApplyCursorInputState with no felt change; LockOnCapture defers any actual lock until capture. PASS — with a build-master spot-check listed below.

## Notes for build-master (TASK-076)

1. **Diff gate:** `git diff` must show ONLY `SiegePlayerController.cpp` + `SiegePlayerController.h`. That closes the "byte-identical elsewhere" caveat in rulings 4/5 (QA has no git access by design).
2. Post-fix line refs for readbacks: normalization call cpp:76 (handoff said ~77 — off by one, harmless); HandleMatchEnd UIOnly `SetInputMode` cpp:755; ApplyCursorInputState cpp:1580.
3. Direct-L_Arena regression PIE: alongside the scripted checks, confirm first-click mouse capture still engages at match start and free-look feels unchanged from t=0 (the boot-time GameOnly slate ops are the only new work on that path; expected no-op per item (f)).
4. Expect NO new log lines from this change (it adds no logging).

## Carry-forward WATCH items (for TASK-076 / Jonathan)

- **Jonathan's ONE menu click** remains the only closure for the live bug (menu→arena not machine-drivable, TASK-073 precedent): from L_MainMenu, either button → full WASD + hotkeys 1–6 + cards, cursor hidden, Alt-cursor works. Expected mechanism now verified end-to-end by inspection.
- Future levels/travelers: the level-travel law now has its reference implementation here; any NEW map that spawns a different controller class must normalize its own posture the same way (not a defect — a law to enforce in future QA).
- TASK-075 cancellation condition verified correct: the menu's UIOnly posture is exactly what the law prescribes for L_MainMenu; no editor change exists that would improve on the arena-side fix.
