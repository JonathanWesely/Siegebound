# TASK-646 — Programmer Handoff — `UAccountMenuWidget` grows its P2 cloud block

**Agent:** gameplay-programmer · **Date:** 2026-08-23 · **Status requested:** ready-for-qa (orchestrator flips the board)

---

## 1. Files touched (the registry-assigned set, nothing else)

| File | Change |
|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/AccountMenuWidget.h` | ACC-§14 pinned children (6), `EAccountMenuMode::CloudLinkForm`, 2 BlueprintCallable entry points, 2 UFUNCTION click thunks, 1 UFUNCTION cloud-state handler, the plain-C++ cloud-lane helpers + `ECloudSyncOpContext`, `bCloudRequestInFlight`, P2 M8 declaration + P2 doc block |
| `Source/GitClaudeUnrealTest/Siegebound/AccountMenuWidget.cpp` | Tree construction for the 6 new children (inside `ConstructAccountTree()`, i.e. BEFORE Super — §3), bind/unbind, `ApplyMode`/`ShowStatus`/`SetFormsEnabled` extensions, `SubmitPressed` CloudLinkForm branch, the whole async cloud lane, `ResolveCloudClient` |

⛔ **ZERO other files.** No `.uasset`, no editor/MCP, no `WBP_MainMenu` change, no Build.cs, no TASKBOARD edit, no git. P2-R5 protected files untouched (`DeckBuilderWidget.cpp` / `SiegePlayerController.cpp` / `SiegeSettingsSubsystem.*` — not opened for writing).

## 2. ACC-§15 / ACC-§14 registry conformance (character-for-character)

**Children added (ACC-§14 widget rows, exact):** `EmailInputBox` (`UEditableTextBox`) · `CloudStatusText` (`UTextBlock`) · `LinkCloudButton` + `LinkCloudLabelText` · `SyncNowButton` + `SyncNowLabelText`. All `UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))`, construct-only-if-null, `TObjectPtr` — the P1 idiom cloned. Mode added: `EAccountMenuMode::CloudLinkForm` (appended after `LoggedIn`; no ordinal is compared or serialized anywhere).

**Pinned sibling surfaces CONSUMED (exact spellings):**
- `USiegeCloudClient` (643): `IsCloudConfigured()`, `GetCloudUserId()`, `SignUp(Email, Password, OnDone)`, `SignIn(Email, Password, OnDone)`, `OnCloudStateChanged` (AddUniqueDynamic/RemoveDynamic), `FSiegeCloudResult::CreateLambda`, `LogSiegeCloud`.
- `USiegeAccountSubsystem` 644 additions: `IsCloudLinked()`, `GetLinkedEmail()`, `SetCloudLink(Email, UserId, RefreshToken)`.
- `FSiegeCloudSync` (645): `PushAll(*GameInstance, OnDone)`, `PullAll(*GameInstance, OnDone)`, `SyncNow(*GameInstance, OnDone)`.

**Pinned surfaces deliberately NOT consumed:** `RefreshSession`, `SignOut`, `FetchRows`, `UpsertRow` (643's internals/other lanes), `IsCloudAuthenticated` (see §7.3), `ClearCloudLink`, `SetLastSyncUtc` (see §6.4/§6.5).

**⛔ Will not compile alone** — by design (the TASK-416/417 precedent): includes `SiegeCloudClient.h` / `SiegeCloudSync.h`, authored in parallel by 643/645. The batch links at TASK-649's single compile gate. Do not open a QA loop over unresolved sibling symbols.

## 3. ACC-§5 — the five conditions, unmoved

- **(a)** Same widget only. **(b)** `RebuildWidget()` body is BYTE-IDENTICAL to P1 (`Initialize(); ConstructAccountTree(); return Super::RebuildWidget();`) — all new construction is inside `ConstructAccountTree()`, so tree → `RootWidget` → Super holds. **(c)** `/Game/UI/WBP_AccountMenu` still reserved; escape hatch untouched. **(d)** BindWidgetOptional + BlueprintCallable wrappers; the BIE `OnAccountMenuStateChanged(FString, FString)` signature UNCHANGED — the new mode crosses as the string `"CloudLinkForm"`; cloud results deliberately do NOT route through the BIE (they land in `CloudStatusText`, which a future WBP reads directly). **(e)** rendering closes on Jonathan's pixels (TASK-651).
- Shipped P1 laws held: panel still overlays without removing the menu · `BackPressed` = `RemoveFromParent()` only, every mode · `BackdropBorder` hit-test VISIBLE untouched · **Escape untouched — no key handling anywhere (AS-§6 A-2 permanent)** · no trailing defaulted parameter on any new function.

## 4. UI-state table (C++-intended states; ⚠️ NOTHING visual verified — see the honesty row)

| Mode / state | StatusText (local, P1 surface) | CloudStatusText | Visible cloud controls |
|---|---|---|---|
| Chooser / CreateForm / LoginForm | P1 lines, unchanged | collapsed | none |
| CloudLinkForm | guidance: email+password; confirm FILLED = new cloud account, BLANK = existing | empty on entry → busy → error on failure | Email, Password, Confirm (reused), Submit ("Link to Cloud") |
| LoggedIn · cloud unconfigured | "Logged in as \<DisplayName\>" | states unconfigured, honest "nothing is lost" | Link **or** Sync per link-state, both DISABLED (states-and-disables, never hides — ACC-§11) |
| LoggedIn · unlinked | same | "Not linked to the cloud…" | LinkCloudButton enabled |
| LoggedIn · linked | same | **"Linked as \<email\>"** (pinned) | SyncNowButton enabled |
| LoggedIn · request in flight | same | busy/interim line preserved | both cloud buttons disabled; Back always live |
| After link+initial sync / Sync Now | same | "Linked as \<email\> - \<outcome\>"; failure appends the error string | steady state restored |

**⚠️ NOT VERIFIABLE WITHOUT PIXELS (stated per the deliverable):** that any of the above renders, the row order/legibility of the cloud block, password masking on the reused boxes in CloudLinkForm, hint-text swap on the confirm box, the disabled-button look, and that the backdrop still swallows clicks with the taller tree. MCP readback has passed on visually broken UMG here before — **TASK-651 (Jonathan) owns all of it.** Nothing on screen is claimed by this handoff.

## 5. Flow wiring (the ACC-§13 trigger mapping, exactly)

1. **Sign-UP** (confirm filled → `SignUp`) → success → `SetCloudLink` → **trigger 2 / A4: `PushAll`** (first-link upload).
2. **Sign-IN** (confirm blank → `SignIn`) → success → `SetCloudLink` → **trigger 1: `PullAll`** (cloud-login pull).
3. **Sync Now button** → **trigger 3: `SyncNow`** (pull-newer → push-all → LastSyncUtc, all inside 645).

All delegate-async (`FSiegeCloudResult::CreateLambda` + `TWeakObjectPtr` guard); no flow blocks on HTTP; a mid-flight `Back`/logout is safe (weak-this; logout race drops the link result with a warning, stores nothing). `SetCloudLink`'s own `OnActiveProfileChanged` broadcast lands the panel on LoggedIn; the in-flight flag keeps the block quiet until the initial sync completes.

## 6. SC-§15 declared deviations / judgment calls (rule on these, don't guess)

1. **Sign-up vs sign-in disambiguation:** the spec pins ONE mode + reuse of both password boxes but no selector child; chosen convention = confirm box FILLED ⇒ SignUp, BLANK ⇒ SignIn, stated to the player in the form's status line and the confirm hint. Zero new children; smallest lawful design.
2. **Refresh token + fallback user-id are parsed from the auth `PayloadOrError` JSON** (top-level `refresh_token`; `user.id` only when the pinned `GetCloudUserId()` returned empty). The registry pins no refresh-token getter, so the response body is the only lawful source. Missing token ⇒ link stored with an empty token + ONE token-free log line.
3. **Sync/auth SUCCESS payloads are never displayed or logged verbatim** — static outcome lines instead ("every result lands in CloudStatusText" honored as outcome + reason). Rationale: an auth success payload CONTAINS TOKENS (on-screen leak) and a sync payload may be bulky row JSON. FAILURE strings display verbatim (contractually token-free per 643).
4. **No cloud-unlink UI:** ACC-§14 pins no child for it; `ClearCloudLink` is never called here. Recorded gap → manager (Phase-3 or a pinned-child one-liner).
5. **`SetLastSyncUtc` never called from the widget** — LastSyncUtc bookkeeping is 645's per ACC-§15's own comments; the widget improvising it would double-write the A3 clock.
6. **`TSharedRef<FSiegeCloudSync>` is captured in its own completion lambda** so the engine outlives a dismissed panel mid-flight. If 645 retains the completed delegate forever this cycle leaks one small object — QA: cross-check 645 releases/overwrites its stored delegate after firing.
7. **`Json` module dependency assumed present via TASK-643's Build.cs edit** (their FHttpModule+Json mandate). This task's fence forbids touching Build.cs. If 643 didn't add it, TASK-649's compile surfaces it in 643's file-set.

## 7. QA scrutiny list

1. **ACC-§5(b):** confirm the 646 diff leaves `RebuildWidget()` byte-identical (it should).
2. **P2-R6 grep:** every `UE_LOG` this diff adds interpolates only `%d` (enum context) or static branch strings — no password/token/payload/email-free? Note: **email is not treated as a credential** (it is the pinned on-screen "Linked as \<email\>" identity) but it is also never logged by the new lines; verify. `RefreshToken` reaches exactly one sink: `SetCloudLink`. The two `%s` families in the file are shipped P1 lines (OutReason / DisplayName).
3. **Cross-lane gap (flag, don't fail 646):** after a game restart the access token (memory-only) is gone; if neither 643 nor 645 internally uses `RefreshSession`/the stored `CloudRefreshToken`, Sync Now will fail honestly into `CloudStatusText` with no in-widget re-auth path while linked (LinkCloudButton is hidden when linked, per the spec's mapping). The widget CANNOT read `CloudRefreshToken` (no pinned getter — correct containment). Route to manager if 645/643 don't cover it.
4. **Cloud-off byte-identity argument (ACC-§11):** with no config, `IsCloudConfigured()` is false ⇒ every new branch lands in states-and-disables or guarded no-ops; no P1 code path was edited except the four marked extension points (`SubmitPressed` early branch, `ApplyMode` additions, `SetFormsEnabled` +EmailInputBox, `ShowStatus` switch case) — each is mode- or null-gated off in a Phase-1 world.
5. Self-run greps (positive-controlled): `service_role|sb_secret|eyJ` in `AccountMenuWidget.*` = ZERO; no literal `"SiegeDecks"`/`"SiegeSettings"`; no `Escape`/`OnKeyDown` handling added.

## 8. M8 DECLARATION (batch verbatim)

**Adds no replicated property, no new replicated class, no new relevancy tier, no RPC.** All cloud traffic is client-local HTTPS from `USiegeCloudClient` (a `UGameInstanceSubsystem`); nothing crosses the UE networking layer. Does NOT consume the M8 Phase-1 checkpoint gate; does NOT substitute for Jonathan's owed feedback items.
