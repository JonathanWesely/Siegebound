# TASK-645 — [ACC-P2-7] `FSiegeCloudSync` — the explicit three-trigger sync engine — programmer handoff

- **Date:** 2026-08-23 · **Status:** ready-for-qa (orchestrator flips the board)
- **Law:** ACC-§13 (implemented in full, nothing more) · ACC-§15 block 2 (character-for-character) · ACC-§12 (row shapes) · ACC-§11 (cloud-off/no-block) · ACC-§3 (projections) · P2-R5 (protected files untouched) · P2-R6 · QUIET-MODULE (⛔ zero compile run here — TASK-649 owns the lane's one).

## 1. Files

| file | status |
|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/SiegeCloudSync.h` | **NEW** — the pinned class, plain C++, no UObject, no API macro (the registry pins none; the Tests folder is same-module) |
| `Source/GitClaudeUnrealTest/Siegebound/SiegeCloudSync.cpp` | **NEW** — the whole pipeline |

⛔ Nothing else touched. **P2-R5 verified at the diff list, not the claim:** `git status --porcelain` over `DeckBuilderWidget.cpp` / `SiegePlayerController.cpp` / `SiegeSettingsSubsystem.h/.cpp` = **empty** (run 2026-08-23, this handoff's authoring session).

## 2. Registry conformance (ACC-§15 block 2)

All six pinned signatures reproduced character-for-character, including the pinned trailing comments: `ShouldPullRow` / `MakeDeckRowJson` / `MakeSettingsRowJson` (statics) + `PullAll` / `PushAll` / `SyncNow` (instance, `UGameInstance&` + by-value `FSiegeCloudResult`). **Zero public-surface drift:** every helper (payload projections, slot IO, the pipeline context, the session mirror) is internal to the .cpp's `SiegeCloudSyncLocal` namespace. The class carries **zero instance members** — each running sync lives in a self-contained `TSharedRef` context held only by the HTTP continuation delegates, so engine-object lifetime is irrelevant to a round trip on the wire and `this` is never captured.

**Consumed cross-task surfaces (pinned only):** 643 — `USiegeCloudClient::{IsCloudConfigured, IsCloudAuthenticated, GetCloudUserId, FetchRows, UpsertRow}`, `FSiegeCloudResult`, `LogSiegeCloud` (declared in their header, per ACC-§14). 644 — `USiegeAccountSubsystem::{IsCloudLinked, SetLastSyncUtc}` (block 4) + the shipped P1 seam `{IsLoggedIn, GetDeckSlotName, GetSettingsSlotName}`. ⛔ No sibling FILE was consumed — signatures only.

## 3. The sync state machine

| phase | PullAll (trigger 1) | PushAll (trigger 2, A4) | SyncNow (trigger 3) |
|---|---|---|---|
| **Preflight** (shared) | client resolves → configured → accounts resolve → logged-in AND cloud-linked (⛔ guest never syncs) → authenticated + non-empty uid → not already in flight. Any refusal = one `LogSiegeCloud` line + `OnDone(false, reason)`, zero local mutation | same | same |
| **Pull** | baseline = session mirror; **unset ⇒ epoch ⇒ pull ALL rows** (trigger 1's fresh-link semantics). GET `decks?user_id=eq.<uid>&select=deck_name,payload,updated_at`, per-row gate = `ShouldPullRow` (the ONE predicate, client-side); apply = load-modify-save the seam slots (merge by deck name, overwrite-on-collision, case-sensitive to mirror the server's text unique; never removes a local deck; `ActiveDeckName` preserved). Then GET `settings?...` (0/1 rows), same gate | — (skipped) | same as PullAll **except** baseline-unset ⇒ **pull phase SKIPPED** (D1 — local-first: never stomp local edits with an unfiltered pull) |
| **Push** | — (skipped) | snapshot seam slots → one `MakeDeckRowJson` upsert per named deck (sequential, `decks?on_conflict=user_id,deck_name` — D4) → ONE `MakeSettingsRowJson` upsert (plain `settings`, PK=user_id), settings LAST deliberately | same |
| **Stamp** | max server `updated_at` observed; empty cloud ⇒ `UtcNow` fallback (D3) | read-back GET of the just-written settings row's `updated_at` = true server now (D3) | same as PushAll |
| **Finish** | session mirror ← stamp, then `SetLastSyncUtc(stamp)` **LAST** (its broadcast rides P1's `HandleActiveProfileChanged` → the live settings subsystem re-reads the just-written slot — pulled settings go LIVE with zero protected-file edits), then `OnDone(true, summary)` | same | same |
| **Any failure** | one funnel: one Warning line + `OnDone(false, reason≤200 chars)`; **no stamp** (next cycle re-pulls/re-pushes; upserts idempotent); local state never rolled back | same | same |

⛔ **Nothing blocks on HTTP:** every hop is a `FSiegeCloudResult` continuation; every continuation re-resolves through a `TWeakObjectPtr<UGameInstance>` null-safe (dead world ⇒ chain dropped silently; the in-flight guard clears by RAII in the context destructor on every exit path).

## 4. Conflict / LWW posture (the honest A3 shape, ACC-§13)

- **Clock = server-side `updated_at`** (the ACC-§12 `before insert or update` trigger, qa/TASK-640's ruling): `Make*RowJson` emit ONLY the client-writable columns — **`updated_at` and `id` are never present in any body this engine sends**, so the client cannot forward-date the clock even by accident (grep C in §7).
- Push-wins-at-write / pull-newer-at-sync; `ShouldPullRow` is **strictly** newer (equal ⇒ no pull; unset baseline ⇒ everything pulls).
- **Recorded limitations restated, not discovered:** two-device edits resolve to the most recent writer, no field merge · deck deletion does not propagate (no tombstones; pull never removes, push never deletes) · `ActiveDeckName` has no cloud column (QA-640 P3 candidate) and every pull preserves the local value · settings payload carries the one shipped field (`assistant_confirm_before_execute`); payload-interior keys are this file's projection schema, applied by known-key with tagged-property absence semantics.

## 5. ⚖️ SC-§15 DECLARED DEVIATIONS (each with its checkable mechanism)

| # | deviation | mechanism + consequence |
|---|---|---|
| **D1** | **The ACC-§15 registry pins `SetLastSyncUtc` and NO reader**, and the account registry slot has a single-reader law (USiegeAccountSaveGame.h) — the engine cannot read the persisted `LastSyncUtc`. | Session-scoped mirror (file-static `TMap<CloudUserId, state>`, game thread), seeded only by this engine's completed cycles. Branches, both LOCAL-FIRST-SAFE: PullAll+no-baseline ⇒ pull ALL (= trigger 1's own fresh-link semantics); SyncNow+no-baseline ⇒ **pull phase skipped** (an unfiltered pull could land older cloud rows on local edits, and push-all would then upload the stomped values — the failure mode chosen instead is "didn't fetch yet", never "destroyed local"). Cross-session cost: the first SyncNow of a session pushes but does not pull; remote rows land at the next cloud login (PullAll) or a later SyncNow. **One-line cure named:** a `LastSyncUtc` getter on `USiegeAccountSubsystem` (644's files — P3 candidate or an orchestrator-ruled rider) collapses both branches to the law's letter; the swap site is `MakeContext()`'s one mirror read. |
| **D2** | Law names the LastSyncUtc stamp only in trigger 3; this engine stamps after ANY fully-successful cycle. | Keeps `ShouldPullRow` monotone: a login-pull's rows can never re-stomp later local edits at the next SyncNow. Strictly protective; no trigger gains a pull it shouldn't have. |
| **D3** | "Server now" with no clock endpoint on the pinned client surface. | Push cycles: one read-back GET of the settings row just written — its trigger-stamped `updated_at` IS server time (settings pushes LAST so it is the cycle max). Pure pulls: max observed `updated_at`. Empty-cloud pull only: `FDateTime::UtcNow()` fallback (clock-ahead ⇒ under-pull, recoverable at next login; clock-behind ⇒ over-pull, idempotent). |
| **D4** | The registry comment pins `UpsertRow` as `POST /rest/v1/<Table>` + `Prefer: resolution=merge-duplicates`; deck pushes pass **`Table = "decks?on_conflict=user_id,deck_name"`**. | merge-duplicates resolves on the PK by default; decks' PK is `gen_random_uuid()` and never collides, so a bare `"decks"` POST takes the INSERT branch and 409s on the ACC-§12 unique `(user_id, deck_name)` on every re-push. The suffix makes PostgREST merge on the law's own constraint (TASK-642 §5 names exactly this conflict target). **Seam note for 643/QA below.** |
| **D5** | In-flight guard (one sync per cloud user at a time), not in the law. | Hygiene: a second trigger while one runs is refused with "a sync is already in progress"; cleared by RAII on every exit path including dropped chains. |

## 6. Call-site audits

- **SC-§33 trailing-default law: NOTHING OWED** — this task adds no defaulted parameter to any existing function; all six public functions are brand-new, no-default, per the registry (the law's own scope clause: a new function with zero call sites by design owes nothing).
- **New-function call sites by design:** `FSiegeCloudSync`'s triggers currently have ZERO shipped callers — TASK-646 (the widget) is the designated caller, authoring in parallel against the same registry. The pure statics' designated caller is TASK-647's test suite.
- **Constant-only include, the ACC-§4 precedent cloned:** `SiegeCloudSync.cpp` includes `SiegeSettingsSubsystem.h` to read exactly ONE symbol — `USiegeSettingsSubsystem::SettingsUserIndex` (the settings slot's user index; the slot NAME comes from the seam). No settings-subsystem type or member is otherwise used; the file itself is untouched (§1). Same pattern the P1 account subsystem uses for the slot-name constants.
- **Dependency direction (648's named criterion, stated up front):** `SiegeCloudSync.cpp` includes the deck/settings SAVE-DATA classes (`SiegeDeckSaveGame.h`, `SiegeSettingsSaveGame.h`, `DeckTypes.h`) — this is the task spec's own order ("load-modify-save the profile's LOCAL slots via the existing save classes") and is Cloud→data-model, never Cloud→widget/controller/subsystem-logic. `SiegeCloudSync.h` includes neither (it includes only `SiegeCloudClient.h` for the pinned delegate type + `Dom/JsonObject.h` for the pinned `TSharedRef<FJsonObject>` parameters, complete-type law).

## 7. Self-check greps (run 2026-08-23; SC-§14 positive control included)

- **(A) P2-R2/P2-R6 secret-material grep** (`service_role|sb_secret|eyJ|AnonKey=|password|CredentialHashHex|CredentialSaltHex|RefreshToken|AccessToken`, case-insensitive, both files): **1 hit — a COMMENT** (SiegeCloudSync.cpp:276, the failure-funnel doc stating the P2-R6 law itself: "Reason strings carry no token, password, or credential material"). No code identifier, no literal, no log/upload of any credential — the P1 precedent (SiegeAccountSaveGame.h's law comments) says comment mentions pass. **Positive control:** same grep on `SiegeAccountSaveGame.h` = 2 hits (`CredentialHashHex`) — the instrument fires.
- **(B) P2-R5 protected files:** `git status --porcelain` over the three protected file-sets = **empty output** (exit 0).
- **(C) `updated_at` write audit:** every occurrence in the .cpp is a read (`select=`/parse), the key constant, or a law comment; **zero** occurrences inside any `Make*RowJson`/upsert body construction. The client never writes the A3 clock.

## 8. 📌 M8 DECLARATION (batch-level, verbatim)

**Adds no replicated property, no new replicated class, no new relevancy tier, no RPC.** All cloud traffic is client-local HTTPS from `USiegeCloudClient` (a `UGameInstanceSubsystem`); nothing crosses the UE networking layer. Does NOT consume the M8 Phase-1 checkpoint gate; does NOT substitute for Jonathan's owed feedback items.

## 9. For QA (TASK-648) — scrutiny list, honest

1. **D1's branch pair** (§5) is the file's riskiest judgment call — the mechanism is stated precisely so it can be ruled on, not discovered. If 648/manager rules the getter rider in (one line on 644's surface), the swap site is named.
2. **Cross-task compile risks the ONE compile (649) inherits — none in my files, all named:**
   - `GitClaudeUnrealTest.Build.cs` has **NO `"HTTP"` module dependency** today, and its two standing ⛔ NOT-HTTP comments guard the LLM-sidecar decision (CONVENTIONS "In-match LLM command assistant" §6). ACC-§11 now ORDERS `FHttpModule` for the cloud lane — **643's class is the HTTP linker, so the Build.cs edit + a dated comment re-point is naturally theirs; no batch task's `names:` block owns Build.cs.** Flag to orchestrator: confirm ownership before 649.
   - My header includes `Siegebound/SiegeCloudClient.h` (643, parallel, may not exist on disk at review time) — registry-sanctioned; verify at the gate that 643's file carries the pinned types character-for-character.
   - **D4's seam use:** 643's `UpsertRow` URL construction must be plain concatenation (`/rest/v1/` + Table) with no encoding/validation of the Table string, or the on_conflict suffix breaks. One sentence in their handoff settles it.
   - **644's `SetLastSyncUtc` must broadcast `OnActiveProfileChanged`** (their task spec pins it; their in-progress header's doc comment already says so) — my FinishSync ordering (slot writes first, stamp LAST) makes pulled settings go live through P1's reload wiring. If 644 ships it non-broadcasting, pulled settings stay on-disk-only until the next profile activation (degraded, not broken) — QA should check their diff.
3. `ShouldPullRow` boundary semantics for 647's matrix: strictly-newer pulls; **equal = false**; unset baseline (ticks 0) = pull. Row keys are snake_case exactly (`user_id`, `deck_name`, `payload`); `updated_at` absent from every body.
4. Deck merge is CASE-SENSITIVE by name (mirrors Postgres text unique). If the deck-builder's local collision rule is case-insensitive, two names differing only by case could coexist locally after a pull — cosmetic, named here so it is a known judgment, not a surprise.
5. Static-teardown corner: a context alive at process exit (sync mid-flight during shutdown) has a destructor touching the file-static map — ordinary UE module-before-CRT teardown covers it; stated for completeness.
6. Refusal/error strings: all reasons are this file's own fixed text plus ≤120 chars of the client's error text (bound by 643's no-token-in-errors law); no email, no token, no password anywhere.

## 10. Slack

Posted to ⚙️ Dev & QA (`C0BF0QZP3CN`, thread `1783116269.740549`), prefix `⚙️ GAMEPLAY-PROGRAMMER:`, ✅ TASK-645 completion note.
