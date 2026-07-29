# TASK-354 — [M8-session] `USiegeSessionSubsystem` + `USessionMenuWidget` — programmer handoff

- **Author:** gameplay-programmer · 2026-07-28 · **NEW FILES ONLY** — zero edits to any existing file (verified via `git status`: the four files below are the only new source entries; every `M` entry belongs to the TASK-349 lane or art staging)
- **Implements:** the SIGNED `handoffs/TASK-353-architecture.md` §5 (D1/D2/D5/D11) + TASKBOARD TASK-354 spec + CONVENTIONS "Networked 1v1 (M8)"
- **Status:** ready-for-qa (QA: `qa/TASK-354.md` per the TASK-354-QA spec)

## 1. File inventory (all NEW, all self-contained — stash/restore-clean per the board's stash contingency)

| File | What it is |
|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/SiegeSessionSubsystem.h` | `USiegeSessionSubsystem` (UGameInstanceSubsystem) — host/join/leave + failure surfaces; **declares `LogSiegeNet`** (CONVENTIONS ruling: the category lives in these files; TASK-356 may `extern`-use it — both lanes compile together at TASK-357) |
| `Source/GitClaudeUnrealTest/Siegebound/SiegeSessionSubsystem.cpp` | Implementation; **defines `LogSiegeNet`**; strict IPv4[:port] parser; GEngine failure-handler bodies |
| `Source/GitClaudeUnrealTest/Siegebound/SessionMenuWidget.h` | `USessionMenuWidget` (UUserWidget C++ base for `/Game/UI/WBP_SessionMenu`) — the doc §5 six-name contract + optional BindWidget conveniences |
| `Source/GitClaudeUnrealTest/Siegebound/SessionMenuWidget.cpp` | Implementation — thin null-safe forwarding shell over the subsystem |

**NOT touched:** `Build.cs` (no new module dependency needed — see flagged decision 1), `Config/DefaultEngine.ini` (parallel law d), any existing source file, TASKBOARD (orchestrator updates status).

## 2. What the subsystem does (doc §5 compliance)

- **`HostListenMatch()`** — null-safe world resolve → status broadcast "Hosting - waiting for opponent" → `UGameplayStatics::OpenLevel(this, "/Game/Maps/L_Arena", true, "listen")` (OpenLevel composes `L_Arena?listen` — the exact URL the D2 `bNetworkedMatch` latch keys on at InitGame; **the latch itself is TASK-356's GameMode work**, this subsystem only produces the travel).
- **`JoinMatch(Address)`** — validates BEFORE any travel (trimmed non-empty, strict IPv4 dotted quad, optional single `:port` 1–65535, default **7777**); invalid ⇒ error broadcast + **NO travel, never a hang** (gate f). Valid ⇒ `ClientTravel("<ip>:<port>", TRAVEL_Absolute)` on the menu world's single local PC via `UGameInstance::GetFirstLocalPlayerController` — the doc-sanctioned session-plumbing resolve, noted in-code against the `GetFirstPlayerController` gameplay ban.
- **`LeaveMatch()`** — absolute `OpenLevel` to `/Game/Maps/L_MainMenu` on either role; **idempotent** via a `bReturnToMenuInFlight` latch cleared on `PostLoadMapWithWorld` (also used to make failure handling loop-free).
- **Error surfaces** — `GEngine->OnNetworkFailure()` + `OnTravelFailure()` bound at `Initialize`, removed at `Deinitialize`; failures logged **verbatim** on `LogSiegeNet`, broadcast as user-facing FStrings, auto-fallback to the menu per the role rules in flagged decision 3. A `bJoinInFlight` latch disambiguates the bad-IP pending-connection timeout (which fires while the MENU world is still current/standalone — error text shows, no travel needed, menu stays interactive).
- **Every transition logs on `LogSiegeNet`** (host, join, leave, arrivals via PostLoadMap, every failure).
- **Practice mode untouched:** "Play vs Bot" enters via `ASiegeGameMode::StartMatch` and never routes through this subsystem; standalone sessions leave it inert (ruling 3 byte-identity — this task adds zero code to any existing path).

## 3. THE WBP CONTRACT FOR TASK-355 (`/Game/UI/WBP_SessionMenu`, reparent to `USessionMenuWidget`, fresh-built — never duplicate+reparent)

**The doc §5 contract (exact names, always available):**

| Member | Kind | Signature |
|---|---|---|
| `HostPressed` | BlueprintCallable | `()` |
| `JoinPressed` | BlueprintCallable | `(const FString& AddressText)` |
| `BackPressed` | BlueprintCallable | `()` |
| `OnSessionStatusUpdated` | BlueprintImplementableEvent | `(const FString& StatusText)` — FString-only (widget law) |
| `OnSessionErrorShown` | BlueprintImplementableEvent | `(const FString& ErrorText)` — FString-only (widget law) |

**Convenience layer (ALL optional — `BindWidgetOptional`, nothing is required to exist):** name widgets exactly and the C++ base does the wiring with zero graph work:

| Widget name in the WBP | Type | Auto-behavior when present |
|---|---|---|
| `HostButton` | Button | OnClicked → `HostPressed` |
| `JoinButton` | Button | OnClicked → reads `AddressTextBox` → `JoinPressed` |
| `BackButton` | Button | OnClicked → `BackPressed` |
| `AddressTextBox` | EditableTextBox | read by the auto-wired Join click (unbound ⇒ empty string ⇒ "enter an IP" error) |
| `StatusTextBlock` | TextBlock | kept updated on every status; **cleared error line on new status** |
| `ErrorTextBlock` | TextBlock | kept updated on every error |

Two supported builds for TASK-355: **(A)** use the six widget names above → zero graph wiring needed (BIEs still fire for extra presentation and may be left unimplemented); **(B)** any widget names → wire buttons to the three wrappers + implement the two BIEs. Text-block names avoid the BIE param names (`StatusText`/`ErrorText`) deliberately — no shadowing.

**Back semantics for the WBP:** in the plain standalone menu, `BackPressed` logs and does nothing else — panel dismissal (hide WBP_SessionMenu, reshow the main menu panel) is the WBP's own navigation. With a session or pending join active it routes to `LeaveMatch` (cancels/leaves).

## 4. How TASK-357 verifies host+join (maps to the doc §8 plan)

- **Lane B (twin `-game` processes, the real path):** host presses Host → log `[SiegeSession] HostListenMatch: opening '/Game/Maps/L_Arena' as a LISTEN server` → arena arrives with `?listen`. Joiner enters `127.0.0.1` (or `127.0.0.1:7777`) → `[SiegeSession] JoinMatch: client travel to '127.0.0.1:7777'` → client lands in the host's arena. Bad-IP: `10.255.255.1` → pending-connection timeout → verbatim `NetworkFailure` log + "Could not join: ..." error text, menu still interactive, **no hang**.
- **Immediate-reject sweep (no travel may occur, error text per case):** empty, `abc`, `1.2.3`, `1.2.3.4.5`, `300.1.2.3`, `1..2.3`, `1.2.3.4:0`, `1.2.3.4:99999`, `1.2.3.4:abc`, `::1` (IPv6 rejected), `1.2.3.4:7777:1`.
- **Leave:** Back during a pending join cancels to menu; LeaveMatch from either role returns to `L_MainMenu`; double-press is a logged no-op (idempotent latch).
- **Standalone regression (gate g):** Play vs Bot never touches these classes — nothing to regress by construction; the subsystem merely initializes and logs one line.
- **Lane A note:** PIE Play-As-Listen-Server does not exercise this subsystem's travel (PIE spins its own instances) — that lane verifies TASK-356's NetMode belt; the menu flow is Lane B's job (gates a/f).

## 5. Flagged decisions (each is a deliberate call — QA please rule)

1. **`FIPv4Address::Parse` NOT used** (doc §5 names it): it lives in the `Networking` module, which is **not** in `Build.cs` — adding it means editing an existing, unowned shared file (task law: new files only; board: Build.cs "only if truly needed"; stash contingency: keep the lane self-contained). Replaced with a self-contained **equivalent-or-stricter** parser (`ParseJoinAddress`, static + side-effect-free, reviewable in isolation). If the manager prefers the engine type, the one-line `"Networking"` dependency belongs to TASK-357's session (single-owner, post-350).
2. **"One dynamic multicast" read as one delegate TYPE, two instances:** the doc's §5 wording says one dynamic multicast the widget forwards, but the widget contract carries TWO BIEs (status + error). Implemented `FOnSessionStatusChanged` once, exposed as `OnSessionStatus` + `OnSessionError` — each feeds its matching BIE.
3. **Hard-failure role split for auto-`LeaveMatch`:** client-role failures and net-DRIVER-level failures (listen/create/already-exists) auto-return to the menu; a **host-role connection-level failure (the joiner dropped) does NOT yank the host to the menu** — status "Opponent disconnected." + keep the world. The doc says "auto-LeaveMatch on hard failures" without a role split; this is the reading that doesn't end the host's match on an opponent rage-quit. Bad-IP timeout (the doc's named case) is fully covered via the `bJoinInFlight` path.
4. **`BackPressed` is conditional** (see §3): LeaveMatch only when net-active or a pending connection exists; the pure-menu Back does not reload `L_MainMenu` (avoids a visible menu flicker-reset on every Back press). The pending-connection check reads `FWorldContext::PendingNetGame` — public engine API, needed because a pending join keeps the menu world standalone.
5. **Optional BindWidget layer added on top of the doc's contract** — strictly additive convenience (`BindWidgetOptional` everywhere, nothing required); the doc's six-name contract stands unmodified and alone suffices for the WBP.
6. **ASCII-only source** — the doc's status string "Hosting — waiting for opponent" is written `"Hosting - waiting for opponent"` (no non-ASCII in source; MSVC encoding trap). Verified zero non-ASCII bytes in all four files.
7. **`LogSiegeNet` defined here** per the CONVENTIONS ruling (`DECLARE` in `SiegeSessionSubsystem.h`, `DEFINE` in the .cpp) — TASK-356 uses it by including the header (in-tree together at 357's compile).

## 6. QA scrutiny pointers

- New-files-only law: confirm via `git status` (nothing existing modified by this lane).
- The parser edge cases in §4's reject sweep against `ParseJoinAddress` (SiegeSessionSubsystem.cpp).
- Failure-handler loop-freedom: `bReturnToMenuInFlight` guards `LeaveMatch` re-entry; a travel failure DURING the menu travel clears the latch and does not retry (bounded).
- PIE multi-instance filter: both failure handlers and PostLoadMap early-out unless the world belongs to this subsystem's GameInstance.
- The `GetFirstLocalPlayerController` use in `JoinMatch` is the doc-§5-sanctioned session-plumbing resolve (menu world, single local PC) — not a gameplay-identity read; the banned `UWorld::GetFirstPlayerController` appears nowhere.
- Delegate hygiene: engine delegates bound Initialize/removed Deinitialize by handle; widget uses AddUniqueDynamic/RemoveDynamic symmetric pairs.
