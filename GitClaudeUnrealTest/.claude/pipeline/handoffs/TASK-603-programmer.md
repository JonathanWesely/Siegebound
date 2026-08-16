# TASK-603 [ACC-5] — `UAccountMenuWidget`, the code-authored account panel — programmer handoff

**Status: ready-for-qa** · 2026-08-16 · gameplay-programmer
**Gate:** TASK-605 (`qa/TASK-605.md`) · compiles at TASK-606 only

## What was built

`UAccountMenuWidget` (`UUserWidget`), the Login / Create Account / Log In / Logged-in panel as ONE
mode-switched, code-authored widget per ACC-§5's fresh narrow ruling (RULING 5). No `.uasset` is
authored; `/Game/UI/WBP_AccountMenu` stays reserved (ACC-§5(c)).

## Files touched (new, exactly the task's names block — nothing else)

- `Source/GitClaudeUnrealTest/Siegebound/AccountMenuWidget.h`
- `Source/GitClaudeUnrealTest/Siegebound/AccountMenuWidget.cpp`

## Registry signatures consumed (ACC-§7, character-for-character)

- `UCLASS() class GITCLAUDEUNREALTEST_API UAccountMenuWidget : public UUserWidget` — block 4, verbatim.
- Calls into `USiegeAccountSubsystem` (TASK-600, not on disk yet — the TASK-416/417 precedent; this
  file DOES NOT COMPILE ALONE and links at TASK-606):
  `CreateAccount(const FString&, const FString&, FString&)` · `Login(const FString&, const FString&, FString&)` ·
  `Logout()` · `IsLoggedIn() const` · `GetActiveDisplayName() const` · `OnActiveProfileChanged`
  (`FOnSiegeActiveProfileChanged`, dynamic multicast, NO params — handler is a no-arg UFUNCTION).
- Log category `LogSiegeAccount` (ACC-§6 — declared in TASK-600's `SiegeAccountSubsystem.h`, which the
  .cpp includes).

## The ACC-§5 conditions, point by point

- **(a) Scope:** this class only; the header's doc comment states the three widget rulings are each
  sealed to their own class and a fourth widget may cite none.
- **(b) ORDER:** `RebuildWidget()` = `Initialize();` (the WARN-437-1 hardening, cloned) →
  `ConstructAccountTree();` → `return Super::RebuildWidget();` — tree built and
  `WidgetTree->RootWidget` set BEFORE Super. Escape hatch: `RootWidget != nullptr` ⇒ the
  code-authored branch is skipped whole; every child is `UPROPERTY(meta=(BindWidgetOptional))` and is
  constructed only while still null.
- **(c) Reserved name:** untouched; no asset referenced or created.
- **(d) Contract:** BindWidgetOptional members · BlueprintCallable wrappers (`CreateAccountChosen`,
  `LoginExistingChosen`, `SubmitPressed`, `LogoutPressed`, `BackPressed`) · ONE BIE
  `OnAccountMenuStateChanged(const FString& ModeName, const FString& StatusMessage)` — FString-only,
  fires on real (mode, status) changes with the settings lane's no-op suppression cloned. The mode
  enum `EAccountMenuMode : uint8` is plain C++ (no UENUM) and never crosses a BIE.
- **(e)** Rendering closes on Jonathan's pixels at TASK-609. **Nothing in this handoff is a claim
  about the screen** — every statement here is a claim about calls and structure (§17).

## Pinned children (ACC-§5, character-for-character — all 17)

`RootPanel` (UVerticalBox) · `BackdropBorder` (UBorder, tree ROOT) · `TitleText` · `StatusText` ·
`CreateAccountButton`+`CreateAccountLabelText` · `LoginExistingButton`+`LoginExistingLabelText` ·
`NameInputBox` · `PasswordInputBox` · `ConfirmPasswordInputBox` (both password boxes
`SetIsPassword(true)`) · `SubmitButton`+`SubmitLabelText` · `LogoutButton`+`LogoutLabelText` ·
`BackButton`+`BackLabelText`.

Note the pinned list names `BackdropBorder` AND `RootPanel`; the tree parents `RootPanel` INSIDE
`BackdropBorder` (the border is the root, the settings-lane shape). `BackdropBorder` is
`ESlateVisibility::Visible` — HIT-TEST VISIBLE, the click-through-into-Quit lesson; commented as
correctness, not styling.

## Modes (ACC-§5)

`Chooser` (Jonathan's exact two buttons: "Create Account" / "Log into existing account") →
`CreateForm` (name+password+confirm) or `LoginForm` (name+password) → `LoggedIn` (StatusText =
`Logged in as <DisplayName>` + Logout + Back). Opening while logged in lands on `LoggedIn`
(`RefreshModeFromSubsystem()` seeds from `IsLoggedIn()` in `NativeConstruct`). Submit failures render
the subsystem's `OutReason` in `StatusText`. Title/Status/Back are visible in every mode; hidden rows
are `Collapsed`. `Back` = `RemoveFromParent()` on self only, every mode.

## ACC-§2 — the honest-credential law, as implemented here

- The plaintext password exists ONLY as `const FString` locals inside `SubmitPressed()` (plus the two
  password `UEditableTextBox` controls the player types into — the input surface itself).
- **Zero logs of the password parameter** (self-grep of all 15 `UE_LOG` sites: they carry `OutReason`,
  display names, and widget names only — `OutReason` is contractually password-free per TASK-600).
- No member outlives the submit call holding a password; both password boxes are cleared on every mode
  change and before every return of a submit attempt.
- Confirm-mismatch is checked widget-side (case-sensitive), since the ACC-§7 subsystem signature takes
  one password; the message is the constant "Passwords do not match."
- No artifact text calls this "secure"/"encrypted"/"protected" — grep hit for QA's criterion: the one
  case-insensitive match of "protected" in these files is the C++ access specifier `protected:`
  (`AccountMenuWidget.h:170`), a false positive.

## Null-safety / fail-safe (ACC-§1)

Unresolvable `USiegeAccountSubsystem` ⇒ `StatusText` explains ("…playing as a guest — decks and
settings still save normally"), all account controls disable via `SetFormsEnabled(false)`, **Back is
deliberately excluded from the disable set**, logged ONCE per instance, never a crash. The
`ResolveSettingsSubsystem` shape is cloned (`World → GameInstance → GetSubsystem`, null-safe at every
hop).

## Deviations / self-caught items QA should scrutinize

1. **`FString::Printf` format constant refused by mechanism:** the "Logged in as %s" string is a
   literal at its one Printf site, not a `static const TCHAR*` in the text namespace — `FString::Printf`
   statically requires a TCHAR ARRAY literal (the engine's format-string `static_assert`); a pointer
   constant does not compile. Commented at the constant block and at the site.
2. **Belt-and-braces mode applies:** on create/login success and on logout the widget calls
   `ApplyMode` directly even though the subsystem's `OnActiveProfileChanged` broadcast (fired inside
   the call) already landed the same mode via `HandleActiveProfileChanged`. Idempotent by
   construction; the BIE no-op suppression makes the duplicate cost zero events.
3. **Re-entrancy note:** the broadcast handler runs DURING `CreateAccount`/`Login`/`Logout` (dynamic
   multicast is synchronous). All widget state it touches (visibility, StatusText, password-box
   clears) is idempotent; the submit locals are copies and unaffected.
4. **`SetFontSize` / `SetHintText` / `SetIsPassword` APIs**: same calls as the shipped, 0/0-compiled
   `SettingsMenuWidget.cpp` precedent (SetFontSize) and stock UMG `UEditableTextBox` UFUNCTIONs.
5. **BIE state cache** (`LastNotifiedModeName`/`LastNotifiedStatus`): holds mode names and status
   strings only — statuses can contain `OutReason` (contractually password-free) and constants, never
   a password. Called out because "FString members near a password path" is exactly what the ACC-§2
   grep should look twice at.

## Standing-law compliance statements

- **Trailing defaults:** NO new defaulted parameter anywhere in these files (SC-§33 audit: vacuous —
  `ApplyMode`/`ShowStatus` were deliberately split rather than given a defaulted status param).
- **Escape (AS-§6 A-2):** no key handling overridden anywhere; `Escape` stays unabsorbed.
- **No editor, no MCP, no compile, no git, no `.uasset`, no other file touched.** The five parallel
  siblings' files (`SiegeAccountSaveGame.*` — already on disk from TASK-599 — `SiegeAccountSubsystem.*`,
  `SiegeSettingsSubsystem.*`, `DeckBuilderWidget.cpp`, `SiegePlayerController.cpp`,
  `Tests/SiegeAccountTest.cpp`) were read-only inputs at most; none modified.
- **Bare slot literals (ACC-§4 grep):** zero `"SiegeDecks"`/`"SiegeSettings"` literals in these files.

## M8 DECLARATION (batch header, verbatim)

Adds no replicated property, no new replicated class, no new relevancy tier, no RPC. All account
state is client-local (`UGameInstanceSubsystem` + local `USaveGame`); the display name touches no
session/player name (A7). Does NOT consume the M8 Phase-1 checkpoint gate; does NOT substitute for
Jonathan's owed feedback items.

## For TASK-606 / TASK-607 downstream

- New reflected type ⇒ RULING 6 / `SC-§26`: full editor restart on the new binary before the
  `Btn_Login` splice references class `UAccountMenuWidget`.
- TASK-607 wires `CreateWidget(UAccountMenuWidget)` → `AddToViewport(ZOrder 10)`; this class assumes
  exactly that (never removes the main menu, Back removes only itself).
