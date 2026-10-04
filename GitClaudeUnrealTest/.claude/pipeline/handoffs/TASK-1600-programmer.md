# TASK-1600 — [BOT-SWITCH-CODE] `SetBotEnabled`, the dev-only bot switch — programmer handoff

Marker `TASK-1600-BOT-SWITCH-CODE`. 2026-10-04. gameplay-programmer. Status → `ready-for-qa`. Gate: `TASK-1601`. 5a: `TASK-1602`. 5b: `TASK-1603`. 5c: the build-master's own dispatch (form (b), no host row).

Source only: eight files edited, one test file created. ⛔ No compile, no Live Coding, no PIE, no editor call of any kind (PID 10976 untouched), no asset load, no asset save, no git write (the only git I ran is read-only `git --no-optional-locks diff` / `diff --stat`, declared here). ⛔ No `DT_Cards` / `cards.csv`, no `CONVENTIONS.md`, no `law/`. Declared tooling: `sha256sum`, `wc -l`, `grep`, one Python edit script in the session scratchpad (`t1600/apply_1600.py`: exact-anchor replacement, every anchor asserted to occur exactly once, all-or-nothing write, each file re-encoded with its own pre-existing CRLF/LF convention), and read-only greps of the installed 5.8 engine source (`IConsoleManager.h`, `Build.h`, `ConsoleManager.cpp`, `Controller.h`, `ObjectPtr.h`).

## §0 Start state and end state (measured)

| file | sha256 before (start of this task) | sha256 after | lines before → after | EOL (unchanged) |
|---|---|---|---|---|
| `Siegebound/SiegeBotController.h` | `0ba9ec12…1d8b50` | `fe65702e…98a6945` | 766 → 851 | CRLF |
| `Siegebound/SiegeBotController.cpp` | `388ec82c…bbdd441` | `1e840051…79b3b374` | 1833 → 1971 | LF |
| `Siegebound/SiegePlayerController.h` | `c0c558f3…c3fb2b7` | `cda41f5b…f3b0697` | 3626 → 3665 | CRLF |
| `Siegebound/SiegePlayerController.cpp` | `32e582f0…a95a7d9` | `1c2b5791…df48c99c` | 7789 → 7876 | LF |
| `Siegebound/SiegeCheatManager.h` | `54d9028a…c12d6f99a` | `34dd8131…6578563` | 172 → 187 | LF |
| `Siegebound/SiegeCheatManager.cpp` | `16ec49f5…36b264c2` | `0ab5bd27…ec3a87da5` | 682 → 703 | LF |
| `Siegebound/SiegeGameMode.h` | `af7d9996…a0aa13c2` | `b3b0b7fd…75c336a3c` | 810 → 823 | CRLF |
| `Siegebound/SiegeGameMode.cpp` | `3f604166…cf433d09` | `47850e1c…3fc7b9384` | 1803 → 1812 | CRLF |
| `Siegebound/Tests/SiegeBotSwitchTest.cpp` | — (did not exist) | `62f9a922…a61cf473` | — → 358 | LF (the `Tests/` majority: 46 of 48 siblings are LF) |

Full hashes are in the session scratchpad; the short forms above are the first/last 8 hex characters. `git diff --stat` over the eight edited files: 409 insertions, 2 deletions (the two deleted lines are the two replaced lines of the `LogSiegeBot` category comment, §6). No BOM on any file before or after. The four LF files were LF before this task (git's `autocrlf` warning on them predates me and is not a change).

## §1 What changed, file by file (the expanded list; every symbol with its line as written)

**`Source/GitClaudeUnrealTest/Siegebound/SiegeBotController.h`**
- `:17–36` — the `LogSiegeBot` category comment amended (§6). `DECLARE_LOG_CATEGORY_EXTERN(LogSiegeBot, Log, All)` unchanged at `:36`.
- `:125–126` — `ResetBot()` doc gains the sentence that `bBotEnabled` is NOT reset by it.
- `:139–164` — **`UFUNCTION(BlueprintCallable, Category = "Siegebound|Bot") void SetBotEnabled(bool bEnabled);`** (public).
- `:166–174` — **`UFUNCTION(BlueprintPure, Category = "Siegebound|Bot") bool IsBotEnabled() const;`** (public).
- `:815–828` — **`UPROPERTY(VisibleInstanceOnly, Transient, Category = "Siegebound|Bot") bool bBotEnabled = true;`** (private).
- `:830–839` — `bool bLastLoggedEffectiveBotEnabled = true;` (private, plain C++, the transition-log latch — a helper the row does not name; §12 item 6).
- `:841–850` — `void RefreshBotEnabledTransition();` (private helper — not in `names:`, declared here; §12 item 6).

**`Source/GitClaudeUnrealTest/Siegebound/SiegeBotController.cpp`**
- `:12` — `#include "HAL/IConsoleManager.h"` (the `TAutoConsoleVariable` complete type).
- `:34–68` — `#if !UE_BUILD_SHIPPING` / `namespace SiegeBotCVars { static TAutoConsoleVariable<int32> CVarSiegeBotEnabled(TEXT("siege.BotEnabled"), 1, <help>, ECVF_Cheat); }` / `#endif` (§4).
- `:383–389` — in `BeginPlay()`, inside `#if !UE_BUILD_SHIPPING`: `RefreshBotEnabledTransition();` immediately BEFORE `StartDecisionTimer();` (the match-start cvar read).
- `:436–449` — at the TOP of `EvaluateDecisions()`, inside `#if !UE_BUILD_SHIPPING`: `RefreshBotEnabledTransition(); if (!IsBotEnabled()) { return; }` — step "(00)", before the existing "(0) MATCH-ACTIVE GATE" (§7).
- `:1891–1894` — in `ResetBot()`, a comment (no code) recording that the flag and its latch are deliberately not reset.
- `:1905–1932` — **`void ASiegeBotController::SetBotEnabled(bool bEnabled)`** body (§5 guard 1).
- `:1934–1945` — **`bool ASiegeBotController::IsBotEnabled() const`** body.
- `:1947–1971` — `void ASiegeBotController::RefreshBotEnabledTransition()` body (the ONE place the two log lines are printed, §3).

**`Source/GitClaudeUnrealTest/Siegebound/SiegeGameMode.h`**
- `:230–241` — **`UFUNCTION(BlueprintPure, Category = "Siegebound|Bot") ASiegeBotController* GetBotController() const;`** (public, right under `HasMatchEnded()`). `class ASiegeBotController;` was already forward-declared at `:13`.

**`Source/GitClaudeUnrealTest/Siegebound/SiegeGameMode.cpp`**
- `:1669–1676` — **`ASiegeBotController* ASiegeGameMode::GetBotController() const { return BotController; }`** (one-line body, placed directly above `SpawnBot()`; §12 item 1 explains why it is in the .cpp and not inline).

**`Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.h`**
- `:1700–1714` — a `//~` block explaining why the reach lives on the player controller (the verifier's `call_actor_function` needs a `BlueprintCallable` UFUNCTION on an actor it can find).
- `:1716–1727` — **`UFUNCTION(Exec, BlueprintCallable, Category = "Siegebound|Dev") void SetBotEnabled(bool bEnabled);`** (public, end of the public section).
- `:1729–1737` — **`UFUNCTION(BlueprintPure, Category = "Siegebound|Dev") bool IsBotEnabled() const;`** (public).

**`Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.cpp`**
- `:38` — `#include "Siegebound/SiegeBotController.h"` (complete type for the two forwarded calls). `Siegebound/SiegeGameMode.h` was ALREADY included at `:45` (pre-existing, M8 `RequestPlayAgain`) — the spec's "`SiegeGameMode.h` in `SiegePlayerController.cpp`" is satisfied by that existing line, nothing added for it.
- `:7791–7839` — anonymous-namespace free function `ResolveBotForDevSwitch(const ASiegePlayerController& SiegeController, const TCHAR*& OutReason)`: authority → world → `Cast<ASiegeGameMode>(World->GetAuthGameMode())` → `GetBotController()`, each failure naming its reason.
- `:7841–7868` — **`void ASiegePlayerController::SetBotEnabled(bool bEnabled)`** body (§5 guard 2).
- `:7870–7876` — **`bool ASiegePlayerController::IsBotEnabled() const`** body (pure, silent, `false` with no bot).

**`Source/GitClaudeUnrealTest/Siegebound/SiegeCheatManager.h`**
- `:173–186` — **`UFUNCTION(exec) void SetBotEnabled(bool bEnabled);`** (public, after `DumpAssistantPrompt`).

**`Source/GitClaudeUnrealTest/Siegebound/SiegeCheatManager.cpp`**
- `:684–703` — **`void USiegeCheatManager::SetBotEnabled(bool bEnabled)`** body (§5 guard 3): `Cast<ASiegePlayerController>(GetOuterAPlayerController())` → forward, else one Warning. `Siegebound/SiegePlayerController.h` was already included at `:22`.

**`Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeBotSwitchTest.cpp`** (NEW, 358 lines) — §10.

Nothing else in `Source/` was touched. Not touched, by name: `DeckComponent.*`, `SiegePlayerState.*`, `SiegeGameState.*`, every rule body inside `EvaluateDecisions` below the new gate, `ResetBot`'s code (comment only), `SpawnBot`, `StartDecisionTimer`, `StopDecisionTimer`, the income path.

## §2 The exact reflected surface the verifier will call (`TASK-1603`, `VER-§7` cl. 2)

| actor to find in the PIE world | class | function | parameter | effect |
|---|---|---|---|---|
| the local player controller (class `SiegePlayerController`, or its `BP_` subclass if one is in play — find by class, quote the actor name) | `ASiegePlayerController` | **`SetBotEnabled`** (`UFUNCTION(Exec, BlueprintCallable, Category = "Siegebound|Dev")`) | **`bEnabled`** : `bool` — `false` to stop the bot's decisions, `true` to restart them | resolves the bot through `ASiegeGameMode::GetBotController()` and forwards to `ASiegeBotController::SetBotEnabled`; on success ONE `LogGitClaudeUnrealTest` Log line `ASiegePlayerController '<pc>': SetBotEnabled(false) — forwarding to bot '<bot>' (dev-only switch, TASK-1600).` then the bot's own `LogSiegeBot` transition line (§3) iff the EFFECTIVE state changed |
| same actor | `ASiegePlayerController` | **`IsBotEnabled`** (`UFUNCTION(BlueprintPure, Category = "Siegebound|Dev")`) | none → returns `bool` | the bot's EFFECTIVE state; `false` when no bot is resolvable. Corroboration only, never the verdict (the row says so) |
| the bot itself, if the verifier prefers (class `SiegeBotController`) | `ASiegeBotController` | `SetBotEnabled` / `IsBotEnabled` (`Category = "Siegebound|Bot"`) | `bEnabled` : `bool` / none | the ONE implementation; same log behaviour |
| the game mode (`SiegeGameMode`) | `ASiegeGameMode` | `GetBotController` (`BlueprintPure`) | none → `ASiegeBotController*` | the bot, or null (Sandbox, networked, pre-spawn, failed spawn) |

Console equivalents (Development builds): `SetBotEnabled 0` / `SetBotEnabled 1` (the controller's Exec is tried before the cheat manager's, so the duplicate name on `USiegeCheatManager` is harmless); `siege.BotEnabled 0` / `siege.BotEnabled 1` (the console variable; picked up within one `DecisionIntervalSeconds`, 2 s today, and announced once); `-ExecCmds="siege.BotEnabled 0"` at launch gives a bot that is off from match start (the disabled line prints once, at the bot's `BeginPlay`). The last of these is NOT driven by `TASK-1603` (the verifier launches nothing) and stays unmeasured, as the row already declares.

What the switch does NOT do (so the verifier's P4 and the "existing Red units keep fighting" property hold by construction): it never touches the income timer, `ASiegePlayerState`, miners, gold accrual, the deck, the hand, or any live unit; it never stops the decision timer (the timer keeps calling `EvaluateDecisions`, which now returns at step (00)). `ResetBot()` (Play Again) does NOT reset the flag — a bot switched off stays off across Play Again until `SetBotEnabled(true)`.

## §3 The exact log strings (`LogSiegeBot`, verbosity `Log`)

Printed ONLY by `ASiegeBotController::RefreshBotEnabledTransition()` (`SiegeBotController.cpp:1947–1971`), ONLY when the EFFECTIVE state (`bBotEnabled && siege.BotEnabled != 0`) differs from the last announced state:

- enabled → disabled: `TEXT("[Bot %s] bot disabled by SetBotEnabled")` with `*GetNameSafe(this)` ⇒ **`[Bot <name>] bot disabled by SetBotEnabled`**
- disabled → enabled: `TEXT("[Bot %s] bot enabled by SetBotEnabled")` with `*GetNameSafe(this)` ⇒ **`[Bot <name>] bot enabled by SetBotEnabled`**

Same wording whether the function or the console variable drove the transition (the row's (3): "the transition logging in (2) is driven by the EFFECTIVE state"). A same-value `SetBotEnabled` call prints NOTHING on `LogSiegeBot`; it prints one **Verbose** line on `LogGitClaudeUnrealTest` (`ASiegeBotController '<name>': SetBotEnabled(<v>) repeats the current flag — no change.`), hidden at default verbosity. No other new line goes to `LogSiegeBot`.

Other new lines, all on `LogGitClaudeUnrealTest`, none on `LogSiegeBot`:
- `Log` — `ASiegePlayerController '<pc>': SetBotEnabled(<v>) — forwarding to bot '<bot>' (dev-only switch, TASK-1600).` (once per successful controller call; §12 item 4 flags its level for QA's ruling)
- `Warning` — `ASiegePlayerController '<pc>': SetBotEnabled(<v>) did nothing — <reason>.` where `<reason>` is one of: `this controller has no authority — a client cannot reach the server's bot` · `no world` · `no ASiegeGameMode is running — not an arena match` · `no bot controller is spawned in this match — a Sandbox or networked match, a failed spawn, or SpawnBot has not run yet` (exactly one Warning, then return — never a crash).
- `Warning` — `USiegeCheatManager::SetBotEnabled — no owning ASiegePlayerController; nothing changed.`

## §4 The console variable

`siege.BotEnabled` — symbol **`SiegeBotCVars::CVarSiegeBotEnabled`**, `static TAutoConsoleVariable<int32>`, default **`1`**, flags **`ECVF_Cheat`**, declared at `SiegeBotController.cpp:58–66` INSIDE `#if !UE_BUILD_SHIPPING` (`:34`…`:68`). Mirrors `siege.Input.LayoutPollEnabled` (`SiegeKeyboardLayoutSubsystem.cpp:75`, a `static TAutoConsoleVariable<int32>` in a named `…CVars` namespace) with the two differences the row pins: `ECVF_Cheat` instead of `ECVF_Default`, and the Shipping guard. Read with `GetValueOnGameThread()` (the keyboard file's own read idiom, `:752`), at `BeginPlay` and at the top of every `EvaluateDecisions` tick, through `IsBotEnabled()`.

Help text (plain words; no `::`, no `()`; checked by grep over the declaration — zero hits): *"1 is the default: the Red bot makes its decisions as normal. 0 stops the bot deciding: no card play, no discard, no spell cast, no gold spent. Its gold, miners and the units already on the field carry on as before. Read at match start and again before every decision, so a change lands within one decision interval. Dev and test lever only. It does not exist in a Shipping build."*

Engine fact behind `ECVF_Cheat` (read off the installed 5.8 source, not assumed): `Build.h:440` `#define DISABLE_CHEAT_CVARS (UE_BUILD_SHIPPING || (UE_BUILD_TEST && !ALLOW_CHEAT_CVARS_IN_TEST))`; `IConsoleManager.h:468–473` `IsEnabled()` returns false for a cheat variable only under that macro; `ConsoleManager.cpp:3122` refuses console INPUT for a disabled variable. ⇒ in Development (PIE, the editor, the suite) the console and `-ExecCmds` CAN set `siege.BotEnabled`; in Shipping the variable is compiled out by my guard before the engine's gate is even reached; `IConsoleVariable::Set` from code (the test) is never cheat-gated.

## §5 The Shipping guards, located by symbol

The row's three guarded BODIES (declarations unconditional):

| # | symbol | `#if !UE_BUILD_SHIPPING` | `#else` path | `#endif` |
|---|---|---|---|---|
| 1 | `ASiegeBotController::SetBotEnabled(bool)` | `SiegeBotController.cpp:1907` | `:1927–1930` — comment, `(void)bEnabled;`, `return;` — no state write, no log | `:1931` |
| 2 | `ASiegePlayerController::SetBotEnabled(bool)` | `SiegePlayerController.cpp:7843` | `:7863–7866` — comment, `(void)bEnabled;`, `return;` — no resolve, no log | `:7867` |
| 3 | `USiegeCheatManager::SetBotEnabled(bool)` | `SiegeCheatManager.cpp:686` | `:698–701` — comment, `(void)bEnabled;`, `return;` — no log | `:702` |

The other guards the row requires:

| what | where | Shipping result |
|---|---|---|
| the cvar declaration (`SiegeBotCVars::CVarSiegeBotEnabled`) | `SiegeBotController.cpp:34`…`:68` | the variable does not exist |
| the `BeginPlay` read (`RefreshBotEnabledTransition();` before `StartDecisionTimer();`) | `SiegeBotController.cpp:383`…`:389` | no read, no log; `BeginPlay` is byte-identical to today |
| the `EvaluateDecisions` gate (`RefreshBotEnabledTransition(); if (!IsBotEnabled()) return;`) | `SiegeBotController.cpp:436`…`:449` | compiled out — the decision path is today's |
| `ASiegeBotController::IsBotEnabled() const` | `:1936`…`:1944`, `#else return true;` | always `true` |
| `ASiegeBotController::RefreshBotEnabledTransition()` | `:1949`…`:1970` (whole body) | empty function |

Why `bBotEnabled` cannot leave `true` in Shipping: its ONLY writer is the `#if` half of guard 1 (`SiegeBotController.cpp:1920`, `bBotEnabled = bEnabled;`). `grep -n "bBotEnabled = " SiegeBotController.cpp` returns that one line; the header default is `= true`; nothing else in `Source/` names the member. (The `bLastLoggedEffectiveBotEnabled` latch is likewise written only inside guarded code, `:1955`.)

**The UHT reason the three UFUNCTION declarations are unconditional:** UnrealHeaderTool does not evaluate an arbitrary `#if` around a `UFUNCTION` declaration — a guarded declaration either breaks generated code (the `.generated.h` thunk references a function that does not exist in that configuration) or is rejected outright. So the three declarations (`SiegeBotController.h:164`, `SiegePlayerController.h:1727`, `SiegeCheatManager.h:186`) and the two `IsBotEnabled` getters plus `GetBotController` stay unconditional, and the no-op lives in the BODY. The Shipping binary still exports a `SetBotEnabled` that does nothing; it does not export a cvar. Also, as the existing `SiegeCheatManager.h` header comment already records, the engine never instantiates a `UCheatManager` in Shipping, so guard 3 is belt-and-braces over an already-unreachable object.

`(void)bEnabled;` in each `#else` branch: the row's shape is `#else return; #endif`; the one extra statement is a no-op cast that pre-empts an unreferenced-parameter diagnostic on a toolchain that enables one (MSVC disables C4100 in UE builds, so it is harmless there; it does nothing and logs nothing — flagged as §12 item 2 for QA's acceptance).

## §6 The `LogSiegeBot` header comment amendment (row (2))

`SiegeBotController.h:17–35` (was `:17–25`). The old close — *"stay on LogGitClaudeUnrealTest so this category holds exactly one line per actual play/discard."* — was already false before this task: `BeginPlay` has printed a `[Bot <name>] Deck select: …` line on `LogSiegeBot` since M6 `TASK-114` (`SiegeBotController.cpp:354`, `:361`, `:369` after this diff). The comment now reads: the category holds exactly one line per FIRED §4 rule, PLUS three named non-rule lines and no others — the deck-select line (one per match start) and the two switch lines (one per EFFECTIVE transition, never on a same-value repeat, compiled out of Shipping). "Every other line on this category is one fired play / discard / spell cast." That sentence is checkable: `grep -n "UE_LOG(LogSiegeBot" SiegeBotController.cpp` lists the three deck-select lines, the two switch lines, and the rule lines; nothing else. The other mentions of the one-line law elsewhere in the header (`bLogSpawnZDiagnostic`, `bWarnedNoCastleBounds`, `bLoggedMineLockout`, `bRule2SpawnFailureLogged`) say that DIAGNOSTICS stay off `LogSiegeBot`, which remains true and was not edited.

## §7 The gate precedes every rule (QA item c) — the lines that prove it

`SiegeBotController.cpp:434` `void ASiegeBotController::EvaluateDecisions()` → `:436` `#if !UE_BUILD_SHIPPING` → `:444` `RefreshBotEnabledTransition();` → `:445–448` `if (!IsBotEnabled()) { return; }` → `:449` `#endif` → `:451` the pre-existing "(0) MATCH-ACTIVE GATE" comment → `:454` `if (!IsMatchActive()) return;` → the deck / player-state / `CardTable` resolves → the hand gather → rules 1–5. Every `SpendGold`, `ConfirmPlayFromHand`, `DiscardFromHand` and `USpellLibrary::ResolveSpell` call in this function sits below `:449` (they are all inside the rule bodies, which begin after the hand gather). Nothing was inserted or moved inside the rules. The income path is not in this function at all (it is `ASiegePlayerState`'s timer), and `StartDecisionTimer` / `StopDecisionTimer` are untouched — so the timer keeps ticking and the economy keeps accruing while the gate returns early. The gate deliberately sits BEFORE the match-active gate so "precedes everything" is a one-line read, and so a cvar transition is still announced once even on a degenerate post-match tick.

## §8 The transition log fires exactly once per EFFECTIVE change — the trace (QA item d)

State: `bBotEnabled` (flag), `C` = `siege.BotEnabled != 0`, `E = bBotEnabled && C` (effective), `L = bLastLoggedEffectiveBotEnabled` (last announced; starts `true`). `RefreshBotEnabledTransition()`: `E == L` ⇒ return silently; else `L = E` and print the one line for `E`.

| input sequence | flag | C | E | L before → after | `LogSiegeBot` |
|---|---|---|---|---|---|
| fresh bot, `BeginPlay` read, cvar default | true | 1 | true | true → true | nothing |
| `SetBotEnabled(false)` | false | 1 | false | true → false | `bot disabled by SetBotEnabled` (once) |
| `SetBotEnabled(false)` again (same value) | false | 1 | false | false → false | nothing (Verbose on the generic category only) |
| `SetBotEnabled(true)` | true | 1 | true | false → true | `bot enabled by SetBotEnabled` (once) |
| console `siege.BotEnabled 0`, next `EvaluateDecisions` tick | true | 0 | false | true → false | `bot disabled by SetBotEnabled` (once, within one `DecisionIntervalSeconds`) |
| every following tick while the cvar stays 0 | true | 0 | false | false → false | nothing |
| `SetBotEnabled(false)` while cvar 0 | false | 0 | false | false → false | nothing — the EFFECTIVE state did not move |
| `SetBotEnabled(true)` while cvar 0 | true | 0 | false | false → false | nothing — AND, not OR |
| console `siege.BotEnabled 1`, next tick | true | 1 | true | false → true | `bot enabled by SetBotEnabled` (once) |
| `-ExecCmds="siege.BotEnabled 0"` at launch, bot `BeginPlay` | true | 0 | false | true → false | `bot disabled by SetBotEnabled` (once, at `BeginPlay`) |

`IsBotEnabled()` is a pure read and never prints, so a verifier polling the readback cannot inflate the count. The call sites of `RefreshBotEnabledTransition()` are exactly three: `BeginPlay` (`:388`), the top of `EvaluateDecisions` (`:444`), and `SetBotEnabled` (`:1925`).

## §9 Includes, complete types, shadows, most-vexing parse (QA item f) and `GetBotController` null-safety (item g)

- New dereferences and their complete types: `ASiegeBotController::SetBotEnabled` / `IsBotEnabled` in `SiegePlayerController.cpp` ⇐ `#include "Siegebound/SiegeBotController.h"` added at `:38`; `ASiegeGameMode::GetBotController` in `SiegePlayerController.cpp` ⇐ `Siegebound/SiegeGameMode.h` already at `:45`; `ASiegePlayerController::SetBotEnabled` in `SiegeCheatManager.cpp` ⇐ `Siegebound/SiegePlayerController.h` already at `:22`; `TAutoConsoleVariable` in `SiegeBotController.cpp` ⇐ `HAL/IConsoleManager.h` added at `:12`; the `TObjectPtr<ASiegeBotController>` → `ASiegeBotController*` conversion in `ASiegeGameMode::GetBotController` lives in `SiegeGameMode.cpp`, which includes `Siegebound/SiegeBotController.h` at `:20`. The header `SiegeGameMode.h` keeps only its existing forward declaration (`:13`).
- C4458 / C4457 / C4459 scan over every added line: the locals are `Bot`, `Reason`, `UnusedReason`, `World` (inside a FREE function, not a member — `AActor` has no `World` member in scope there), `SiegeMode`, `SiegeController` (the free function's parameter, named so it cannot be mistaken for `APawn::Controller`), `SiegePC`, `bEffectiveEnabled`, `bFlag`, `CVar`, `PriorValue`, `Scoped`, `HeldAtZero`, `FlagProperty`, `SpawnParams`, `WorldName`, `WorldContext`. None is `PlayerState`, `Owner`, `Controller`, `Instigator` or `Slot`. A regex scan of the added lines for `\b(PlayerState|Owner|Controller|Instigator)\b\s*(=|;|\))` returned nothing.
- Most-vexing parse: the only parenthesised constructions in new code are `FScopedPlayWorld Scoped;` (no parentheses), `FScopedBotCVarHeldAtZero HeldAtZero(CVar);` (a variable argument, not a type), `FActorSpawnParameters SpawnParams;`, and the `TAutoConsoleVariable` definition with literal arguments — none is parseable as a function declaration.
- `GetBotController()` null-safety at every caller: it has exactly one caller outside the game mode, `ResolveBotForDevSwitch` (`SiegePlayerController.cpp:7832–7837`), which null-checks and names the reason; the two controller bodies consume that result through `if (!Bot)` (`:7846`) and `Bot ? … : false` (`:7875`). Sandbox / networked / pre-spawn / failed-spawn all surface as the same null with the one Warning in `SetBotEnabled` and a silent `false` in `IsBotEnabled`. No authority (`!HasAuthority()`) is checked FIRST, before `GetAuthGameMode()` (which is null on a client anyway), and named as its own reason.
- The free function is `ResolveBotForDevSwitch` in a new anonymous namespace at the end of `SiegePlayerController.cpp` (multiple anonymous namespaces in one TU are legal and merge); `grep -rn ResolveBotForDevSwitch Source/` finds only its definition and its two callers. It is used in BOTH build configurations (by `IsBotEnabled`), so no unused-function diagnostic arises in Shipping.

## §10 The offline tests (QA item h)

File: `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeBotSwitchTest.cpp`, everything after the includes under `#if WITH_DEV_AUTOMATION_TESTS` (`:16`) … `#endif // WITH_DEV_AUTOMATION_TESTS` (`:358`). Two `IMPLEMENT_SIMPLE_AUTOMATION_TEST`s with the sibling flags `EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter`:

- **`Siegebound.Bot.SetBotEnabled.FlagRoundTrip`** (`FSiegeBotSetBotEnabledFlagRoundTripTest`, `:211–270`): precondition `siege.BotEnabled == 1`; test world; spawn `ASiegeBotController`; `IsBotEnabled()` true → `SetBotEnabled(false)` → false → same-value repeat → still false → `SetBotEnabled(true)` → true. At each step the PRIVATE `bBotEnabled` UPROPERTY is read back by reflection (`FindFProperty<FBoolProperty>(ASiegeBotController::StaticClass(), TEXT("bBotEnabled"))` → `GetPropertyValue_InContainer`), so the FLAG is shown to move, not just the composed answer. Ends by asserting the cvar still reads 1 and is not held at `SetByCode`.
- **`Siegebound.Bot.SetBotEnabled.CVarComposes`** (`FSiegeBotSetBotEnabledCVarComposesTest`, `:283–356`): records the prior value (asserted 1) and that the variable is NOT at `SetByCode`; test world + bot; `IsBotEnabled()` true; then inside an RAII scope `FScopedBotCVarHeldAtZero` (`Set(0, ECVF_SetByCode)` on entry, **`Unset(ECVF_SetByCode)` on EVERY exit path**): cvar reads 0, `IsBotEnabled()` false, the reflected flag is STILL true; `SetBotEnabled(false)` then `SetBotEnabled(true)` under cvar 0 both leave `IsBotEnabled()` false (AND, not OR); after the scope: the cvar reads its prior value AND is no longer at `SetByCode` (both read back off the machine), and `IsBotEnabled()` is true again with NO further `SetBotEnabled` call — the controlled proof that only the console half had moved.
- **The restore idiom** is `Unset(ECVF_SetByCode)`, which drops that priority layer so the variable falls back to its constructor default and its `SetBy` returns to `Constructor` — the house release idiom (`AFogVolume::ReleaseFogRenderFloor`, `FogVolume.cpp:1513`), chosen over `Set(prior, SetByCode)` for the reason that file records (a `Set` restore leaves the layer pinned for the rest of the process). Nothing in `Source/` other than this test writes `siege.BotEnabled` from code.
- **The world** (declared deviation, in the file's own header comment and here): the row says *"spawn an `ASiegeBotController` in a test world"*, so the file uses a real `UWorld` through a verbatim copy of the `TASK-1173` fixture (`SiegeFogVisualTest.cpp`, `SiegeFogRealWorldFixture::FScopedPlayWorld` — the engine's `FActorTestSpawner` shape: `CreateWorld(EWorldType::Game, false, name, GetTransientPackage())` → `CreateNewWorldContext` → `AddToRoot` → `SetCurrentWorld` → `InitializeActorsForPlay(FURL())` → `BeginPlay`; teardown `RouteEndPlay` loop (inert, as that file measured) → `ShutdownWorldNetDriver` → `DestroyWorld(true)` → `SetPhysicsScene(nullptr)` → `DestroyWorldContext` → `RemoveFromRoot`). ⛔ Several older test files carry the sentence *"not one `SpawnActor` and not one `UWorld::CreateWorld` anywhere in `Siegebound/Tests/`"*; `TASK-1173` / `TASK-1178` already recorded that sentence as stale, and this is the second such file. The suite that ships today (573/573) runs that fixture green. **Why it is safe for THIS actor (measured off the fixture's own `TASK-1174/1178` finding, not assumed):** the world has no game mode, so `World->HasBegunPlay()` is false for its whole life ⇒ `SpawnActor` runs the bot's constructor and `PostInitializeComponents` (`InitPlayerState` finds no game mode and no game state and spawns nothing) but **never `BeginPlay`** — no deck build, no `DT_Cards` load, no decision timer, no deck-select line. The only project code the tests execute is the switch itself. `UDeckComponent` overrides none of `OnRegister` / `InitializeComponent` / `BeginPlay` (grep), so the default subobject is inert too. Spawn parameters mirror `ASiegeGameMode::SpawnBot` (`AlwaysSpawn`, `RF_Transient`).
- Expected suite delta: `573 + 2 = 575` — a PREDICTION; `TASK-1602` reports the count it measures. ⛔ No witnessed red: nothing here was compiled or run (forbidden on this row).

## §11 Numeric literals in the diff (QA item a) — none exists in `DT_Cards`

Census of every numeric literal in the ADDED code lines of the eight edited files (comments stripped): `1` — the cvar default (`CVarSiegeBotEnabled(TEXT("siege.BotEnabled"), 1, …)`); `0` — the comparison `GetValueOnGameThread() != 0`; `"1`/`"0` — the help text's "1 is the default" / "0 stops the bot"; `1600` — inside `TEXT("… TASK-1600 …")`; `32` — the type name `int32`. In the new test file additionally: `0` / `1` as the cvar's off value and asserted default, and `1173` / `1174` / `1178` / `1600` / `1601` / `1602` / `1603` / `573` in comments and test labels. No `Cost`, `HP`, `Damage`, `Range`, `Speed`, `AoERadius`, `DeckCount`, `MaxCopies`, `SwarmCount`, `GoldSteal` or any other card-stat number appears; the cadence stays `DecisionIntervalSeconds`; no new UPROPERTY carries a default other than `bBotEnabled = true` (a bool).

## §12 Decisions QA should scrutinize (flagged, each with its grounds)

1. **`ASiegeGameMode::GetBotController()` is defined in `SiegeGameMode.cpp`, not inline in the header.** The row says "a new one-line public accessor"; the BODY is one line (`return BotController;`). Grounds: the header holds only a forward declaration of `ASiegeBotController`, and the `TObjectPtr<T>` → `T*` conversion is a C-style cast on the resolved `UObject*` inside the engine template; the .cpp includes the complete type at `:20`, so the complete-type include law is satisfied by construction rather than argued. (The engine's own `AController` keeps its raw-pointer `K2_GetPawn()` out of line for the same family of reasons.) If the gate prefers the inline form, it is a one-line move; the `UFUNCTION` and signature are the row's.
2. **`(void)bEnabled;` in the three `#else` branches** (§5) — one no-op statement beyond the row's literal `#else return; #endif`. Does nothing, logs nothing.
3. **`ASiegePlayerController::IsBotEnabled() const` is NOT guarded.** It is a pure read with no callers in shipped code; in Shipping it resolves the bot (read-only) and returns the bot's own Shipping answer (`true`), or `false` with no bot. The row's guard list names the three `SetBotEnabled` bodies, the cvar and the `BeginPlay` read; the getters are pure. Leaving it unguarded also keeps `ResolveBotForDevSwitch` referenced in every configuration (no unused-function warning in Shipping).
4. **The controller's success line is `Log`, not `Verbose`** (`SiegePlayerController.cpp:7856–7858`, `LogGitClaudeUnrealTest`): one line per successful call so the verifier can see the call landed on a named bot. It is not on `LogSiegeBot`, so the one-line law is untouched; the row's "Verbose … is fine" sentence is about the bot's same-value repeat, which IS Verbose (`SiegeBotController.cpp:1914`). If the gate wants this line Verbose too, it is a one-token change.
5. **The gate sits BEFORE the match-active gate** in `EvaluateDecisions` (§7), so it provably precedes everything; a side effect is that a cvar transition is announced even on a tick after match end (only reachable in a degenerate world, since the freeze stops the timer).
6. **Two private symbols not in `names:`:** `bLastLoggedEffectiveBotEnabled` (the announce latch — required to make "exactly once per EFFECTIVE transition" true for BOTH inputs) and `RefreshBotEnabledTransition()` (the one print site). Both private, both plain C++ (no UPROPERTY, no UFUNCTION), both documented in the header. In Shipping the latch is never read or written (MSVC emits no diagnostic for an unused private data member).
7. **The test file uses a real `UWorld`** (§10) — by the row's explicit wording, through the `TASK-1173` precedent, declared in the file.
8. **`SiegeGameMode.cpp` is genuinely edited** (item 1), so all nine files on the row's `FILES` list carry a diff.
9. **The `LogSiegeBot` comment now ENUMERATES the non-rule lines** (§6). If a future task adds another, it must amend the list — that is the point.

## §13 Fences honoured

No compile, no Live Coding, no editor or MCP call, no PIE, no asset read/write, no git write (read-only `diff` only), no `DT_Cards` / `cards.csv`, no `CONVENTIONS.md`, no `law/`, no other row's line on the board. The editor on PID 10976 was never contacted. QUIET-MODULE: this task was the only `Source/` writer; no compile may start until `TASK-1601` passes and `TASK-1602` takes the module.

## Recipe candidates

None minted here (a recipe is seeded only from a measured run — `VER-§13`). → **`TASK-1603`** proposes `RCP-vsbot-bot-disabled-run.md` in its `## Recipe candidates` after the measured run: *Play vs Bot by the `RCP-vsbot-capture-center-and-summon.md` menu steps, `SetBotEnabled(false)` at t≈5 s by `call_actor_function` on the player controller (§2 row 1 of this handoff: actor = the `SiegePlayerController` found by class, function `SetBotEnabled`, arg `bEnabled=false`), then a long-horizon capture with no hero-death clock; `SetBotEnabled(true)` to hand the match back.* The manager mints the recipe ROW when `qa/TASK-1603-verify.md` lands.
