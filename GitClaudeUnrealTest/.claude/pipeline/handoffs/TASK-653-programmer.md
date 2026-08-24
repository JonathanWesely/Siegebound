# TASK-653 — [ACC-P2.1-1] THE RE-AUTH WIRE + THE SYNC-BASELINE GETTER (riders R1+R2) — programmer handoff

- **Date:** 2026-08-23 · **Status:** ready-for-qa (orchestrator flips the board; TASK-654 reviews)
- **Law:** ACC-§15 P2.1 (the dated 2026-08-23 seam, character-for-character) · the ACC-§11 token-reader dated addition · ACC-§11/§13 · P2-R2 · P2-R5 · P2-R6 · qa/TASK-648.md §1.5/§6 (R1/R2) · `handoffs/TASK-645-programmer.md` §5 D1 (the named swap site) · QUIET-MODULE (⛔ zero compile run here — TASK-655 owns it).

## 1. Files — exactly the board's list, nothing else (verified at porcelain)

| file | change |
|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/SiegeAccountSubsystem.h` | EDIT — the two P2.1 getters (R2 surface), pinned lines reproduced with a P2.1 section comment; nothing existing moved |
| `Source/GitClaudeUnrealTest/Siegebound/SiegeAccountSubsystem.cpp` | EDIT — `GetLastSyncUtc()` / `GetCloudRefreshToken()` bodies (pure `FindActiveProfile()` reads) |
| `Source/GitClaudeUnrealTest/Siegebound/SiegeCloudSync.cpp` | EDIT — the R2 swap at `MakeContext()`; session mirror RETIRED; D1 skip branch removed (behavior change, §5) |
| `Source/GitClaudeUnrealTest/Siegebound/AccountMenuWidget.h` | EDIT — P2.1 class-doc paragraph, `TryRefreshCloudSession` + `HandleCloudRefreshResult` declarations (plain methods), `bCloudSessionRefreshAttempted` latch (plain member) |
| `Source/GitClaudeUnrealTest/Siegebound/AccountMenuWidget.cpp` | EDIT — the R1 wire: latch reset in `NativeConstruct`, the attempt hook in `RefreshCloudBlock`'s linked branch, the two method bodies, two new text constants |
| `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeCloudTest.cpp` | EDIT — one new test (`Siegebound.Cloud.ReAuthSeamGetters`) + a dated amendment to the header's gap list. The existing 8 test bodies are byte-untouched |

⛔ **P2-R5 verified at the diff list, not the claim:** `git status --porcelain` over `DeckBuilderWidget.cpp` / `SiegePlayerController.cpp` / `SiegeSettingsSubsystem.{h,cpp}` = **empty** (run 2026-08-23, this session). The full `Source/` porcelain = exactly the six files above.

## 2. ACC-§15 P2.1 conformance table

| P2.1 pin | shipped | where |
|---|---|---|
| `FDateTime GetLastSyncUtc() const;` + pinned trailing comment | **EXACT** (tab-indented into the class body; everything after leading whitespace character-identical) | SiegeAccountSubsystem.h:229 |
| `FString   GetCloudRefreshToken() const;` + pinned trailing comment (incl. the 3-space alignment) | **EXACT** | SiegeAccountSubsystem.h:230 |
| ⛔ Neither getter UFUNCTION; P2.1 adds NO reflected member | **HOLDS across the whole diff** — the two subsystem getters, both widget methods and the latch member are plain C++; `git diff | grep '^+.*(UFUNCTION|UPROPERTY|UCLASS|USTRUCT)'` over Source/ = 3 hits, ALL doc-comment lines saying "not a UFUNCTION" (no macro added) | §7 grep C |
| Getters: active-profile reads, no mutation, no broadcast; guest/unlinked ⇒ `FDateTime()` / empty | **EXACT** — both funnel through `FindActiveProfile()` (the one predicate); test-pinned incl. broadcast-count and no-save assertions | SiegeAccountSubsystem.cpp:359-385; test §6 |
| R1 consumer: `UAccountMenuWidget` is the ONLY consumer of `GetCloudRefreshToken()` | **HOLDS** — one product call site (AccountMenuWidget.cpp:1429). The new test also reads it with SCRATCH placeholder values (declared, §8 D7 — the board spec's own test order) | §7 grep A |
| R1 predicate: linked ∧ client resolves ∧ `IsCloudConfigured()` ∧ ¬`IsCloudAuthenticated()` ∧ non-empty token | **EXACT**, split caller/callee: `RefreshCloudBlock`'s linked branch establishes linked+resolved+configured+not-in-flight (both pointers non-null by construction there); `TryRefreshCloudSession` adds the latch, the live-session check, the token read | AccountMenuWidget.cpp:1297-1305 (hook), 1412-1456 |
| Exactly ONE `RefreshSession` attempt per panel activation; no retry loop, no tick, no poll; nothing blocks on HTTP | **EXACT** — `bCloudSessionRefreshAttempted` latch, reset in `NativeConstruct` (= activation), set at the moment a request launches; delegate-async end to end; `RefreshSession` now has exactly ONE product caller in the Source tree (the R1 gap of qa/TASK-648 §1.5 closed) | AccountMenuWidget.cpp:1440, 1451 |
| Success ⇒ rotated token re-stored via `SetCloudLink(GetLinkedEmail(), Client->GetCloudUserId(), NewToken)` | **EXACT in shape** (userid via getter-first + payload-gap-fill — the 646 idiom, equal to the law whenever the getter answers; declared §8 D3). Store happens ONLY when a rotated token was parsed (§8 D4) | AccountMenuWidget.cpp:1500-1521 |
| Failure ⇒ one honest `CloudStatusText` line, ⛔ ZERO state mutation | **EXACT** — the pinned line replaces the raw error; no ClearCloudLink/SetCloudLink/stamp on the path; payload neither displayed nor logged. Wording carried in ASCII hyphen (§8 D2) | AccountMenuWidget.cpp:1461-1477 |
| Token never logged/displayed (P2-R6, extends 643's hygiene) | **HOLDS** — the token lives in `TryRefreshCloudSession`'s local + `HandleCloudRefreshResult`'s local, reaches exactly the `RefreshSession` grant and the `SetCloudLink` sink; the completion lambda captures ONLY `WeakThis`; every new log line is state-only ("token: held" literal at most) | §7 grep B |
| R2 swap: `MakeContext()` baseline ← `GetLastSyncUtc()`; mirror fate declared | **EXACT** — the named swap site now reads the persisted stamp; mirror **REMOVED** (SC-§15 note in-file at `FSessionState`), see §4/§5 | SiegeCloudSync.cpp:831-841 |
| ACC-§5(b) `RebuildWidget` order untouched · NO new widget children · escape law | **HOLDS** — `RebuildWidget()`/`ConstructAccountTree()` byte-untouched; zero `ConstructWidget` calls added (the ACC-§14 table does not change); zero `OnKeyDown/NativeOnKeyDown/EKeys` in the diff |  |
| The refresh triggers NO sync | **DELIBERATE** — the seam law orders none; Sync Now (trigger 3) stays the player's lane, its pull now live per R2 | AccountMenuWidget.cpp:1526-1529 |

## 3. The SetCloudLink rotated-token interaction analysis (the dispatch's named question)

- **The identical-values guard CANNOT swallow a rotation.** `SiegeAccountSubsystem.cpp:256-258` requires ALL THREE of `LinkedEmail == TrimmedEmail && CloudUserId == TrimmedUserId && CloudRefreshToken == RefreshToken` before no-opping. The R1 re-store passes the unchanged email/userid with a NEW token ⇒ the conjunction fails ⇒ real mutation: registry SAVES + `OnActiveProfileChanged` broadcasts (the upheld 644 decision 4). **Test-pinned** (§6: rotation stores + broadcasts exactly once + saves).
- **Trim/refuse:** email = `GetLinkedEmail()` (stored already-trimmed, non-empty while linked); userid = the pinned getter (non-empty after a successful refresh adoption). Degenerate corner — getter empty AND no `user.id` in the payload ⇒ `SetCloudLink` refuses WHOLE (empty-after-trim guard): a Warning, nothing stored, never a half-link. The rotation is then lost and the next activation's refresh fails honestly into the pinned line.
- **Same-token echo** (GoTrue reuse window): the whole triple is identical ⇒ the no-op guard correctly saves nothing and broadcasts nothing (the delegate law). Correct — there is nothing to store.
- **Success payload with NO rotated token:** store **NOTHING** (declared D4). Calling the pinned `SetCloudLink(..., "")` would be a REAL mutation that WIPES the held token on a *success* — a destructive write the law does not order (644 treats an empty token as a real value). The held token is kept; logged token-free.
- **Broadcast re-entrancy:** the store's broadcast lands `HandleActiveProfileChanged → RefreshModeFromSubsystem → ApplyMode(LoggedIn) → RefreshCloudBlock` while `bCloudRequestInFlight` is still true ⇒ the in-flight branch returns quietly; the latch additionally blocks any second attempt. No loop is possible.

## 4. SC-§15 declared choice — THE SESSION MIRROR IS REMOVED (not kept as a cache)

`FSessionState` now carries ONLY the D5 in-flight guard. Mechanism: two baseline authorities (persisted stamp + session cache) would need a merge rule the law does not have; the registry now pins a reader, so the persisted stamp is the single authority. The one thing the mirror used to mask — `FinishSync` with the account subsystem unresolvable (stamp not persisted) — now honestly re-pulls already-applied rows on a later cycle: idempotent (overwrite-on-collision with the same or newer rows, `ShouldPullRow` unchanged), never destructive. In-file SC-§15 note at the `FSessionState` comment block.

## 5. ⚠️ THE D1 BEHAVIOR CHANGES, STATED PLAINLY (R2 un-does 645-D1's compromise)

1. **SyncNow's pull phase ALWAYS runs.** Never-synced profile ⇒ baseline `FDateTime()` ⇒ pulls ALL rows (trigger 1's fresh-link semantics, now the lawful trigger-3 shape — the ACC-§13 letter). Previously: pull silently SKIPPED.
2. **The cross-session cost is GONE:** the first SyncNow of a NEW session now pulls strictly-newer rows against the PERSISTED stamp. Previously: pushed without pulling ("didn't fetch yet").
3. **PullAll (trigger-1 login pull) is now stamp-filtered ACROSS sessions** — a previously-synced profile's cloud login pulls only newer rows instead of re-pulling everything each session (D2's monotone stamping now persists). Fresh link and re-link still pull ALL: fresh profiles default the stamp and `ClearCloudLink` resets it.
4. The `"pull skipped (no session baseline …)"` summary string no longer exists in the product.
5. `MakeContext()`'s null-accounts fallback (`FDateTime()` ⇒ pull-all) is structurally unreachable — `PreflightRefusal` resolved the same subsystem in the same synchronous frame — kept as null-safety only.

## 6. Tests — ⭐ TASK-655 SUITE EXPECTATION: **127/127** (126 baseline + 1)

One new test, `Siegebound.Cloud.ReAuthSeamGetters` (file count 8 → 9 `IMPLEMENT_SIMPLE_AUTOMATION_TEST`, grep-counted): guest + unlinked getter defaults · `SetCloudLink → GetCloudRefreshToken` byte round trip · `SetLastSyncUtc → GetLastSyncUtc` round trip · pure-read discipline (read burst moves neither the broadcast counter nor a deleted registry file) · the rotation-through-644's-guards shape (stores + exactly-one broadcast + saves) · identical-triple no-op · cross-instance persistence (the exact values a next session's R2 baseline / R1 attempt read) · `ClearCloudLink` resets both. **Zero network:** `RefreshSession` still has zero callers in the test file; only pure getters and P1-pinned local-save mutators are driven. Scratch slot + `FCloudScratchGuard` as shipped; scratch placeholder tokens, ⛔ nothing JWT-shaped. FDateTime comparisons ride `TestTrue(==)` (the file's FGuid idiom — deliberate: the generic `TestEqual` has no guaranteed FDateTime debug-print lane; a compile break here would spend a QA loop). Second file edit: a dated ⭐ amendment to the header's "NOT COVERED" list (the value-readback gap is closed; no other gap-list line moves).

## 7. Self-check greps (run 2026-08-23; SC-§14 positive-controlled)

- **(A) Token containment:** `GetCloudRefreshToken` across `Source/` — definition pair + ONE product consumer (`AccountMenuWidget.cpp:1429`) + doc comments + the new test (scratch values, §8 D7). `RefreshSession` callers outside the client: exactly one product call site (`AccountMenuWidget.cpp:1451`) + comments — **the 648 §1.5 zero-caller finding is cured.**
- **(B) P2-R2** (`service_role|sb_secret|eyJ|cjgqqeogsynrphowdcdp`, case-insensitive, whole `Source/`): **ZERO real hits** — every `eyJ` match is the benign `…KeyJust…`/`EKeys` identifier class (the instrument demonstrably fires). Positive control: `service_role` on CONVENTIONS.md = 2 hits.
- **(C) P2-R6** (`CredentialHashHex|CredentialSaltHex`, Siegebound/): hits ONLY in the four P1 account files (fields, law comments, P1 test). **ZERO in any cloud-lane file.** Reflection grep over the diff: no `UFUNCTION/UPROPERTY/UCLASS/USTRUCT` macro added (3 comment-line hits only).
- **(D) Trailing-defaulted-parameter law: NOTHING OWED** — zero defaulted parameters on any new declaration (`GetLastSyncUtc`, `GetCloudRefreshToken`, `TryRefreshCloudSession`, `HandleCloudRefreshResult`); all four are brand-new functions, so the enumerated call-site audit is vacuous by the law's own scope clause.

## 8. ⚖️ SC-§15 declared deviations / judgments (each with its mechanism)

| # | deviation | mechanism |
|---|---|---|
| D1 | Session mirror **REMOVED** (the law offered keep-or-remove) | §4 — single baseline authority; degraded corner now idempotent-re-pull, not masked |
| D2 | The pinned failure line ships with an ASCII hyphen: `"cloud session expired - sign in again to re-link"` (law text carries an em dash) | Every shipped player-visible literal in this file uses `-`; non-ASCII in a TCHAR literal is a known mojibake/compile-charset hazard the comment-only em dashes never face. Wording otherwise character-identical. QA may rule the em dash in; a one-character edit |
| D3 | Userid for the re-store: pinned getter FIRST, payload `user.id` fills only an empty getter (`ParseAuthPayload`'s existing contract) | Equal to the law's `Client->GetCloudUserId()` whenever the getter answers (it does after every successful adoption); the fallback only defends the degenerate empty-getter corner, which then hits SetCloudLink's whole-refusal |
| D4 | Success with NO rotated token in the payload ⇒ store NOTHING | Wiping the held token on a success is a destructive write the law does not order; 644 treats empty-token as a real value, so the pinned call would erase it |
| D5 | Latch burns on LAUNCH, not on predicate miss | A healthy session or absent token must not spend the activation's one attempt; still at most ONE `RefreshSession` per activation, ever |
| D6 | "Panel activation" = `NativeConstruct` (latch reset there, before the mode seed that may spend it) | The panel is created fresh per open (TASK-607); a re-added instance re-Constructs = a new activation |
| D7 | The new test reads `GetCloudRefreshToken` (the ACC-§11 addition names the widget as the single lawful consumer) | The board spec's own clause (4) orders the getter round-trip pin; the test drives obviously-scratch placeholders on the scratch slot — the token LAW (custody of real token material) is untouched. Named, not waved |
| D8 | Panel dismissed and GC'd mid-refresh ⇒ the rotated token is lost (WeakThis dead ⇒ no store); the stored token stays the consumed one ⇒ the next activation's refresh fails honestly into the pinned line | The 646 WeakThis idiom kept deliberately — the seam law pins token custody on the WIDGET; a widget-independent store would move custody. Self-healing: the player re-links via the existing form |

## 9. 📌 M8 DECLARATION (P2 batch wording, verbatim)

**Adds no replicated property, no new replicated class, no new relevancy tier, no RPC. All cloud traffic is client-local HTTPS from USiegeCloudClient (a UGameInstanceSubsystem); nothing crosses the UE networking layer. Does NOT consume the M8 Phase-1 checkpoint gate; does NOT substitute for Jonathan's owed feedback items.**

**SC-§26 statement for TASK-655, explicit:** this diff adds **NO reflected member** — no UFUNCTION, no UPROPERTY, no new UCLASS/USTRUCT anywhere (verified §7 C). By the ACC-§15 P2.1 pin, no full editor restart is owed on P2.1's account.

## 10. For QA (TASK-654) — scrutiny list, honest

1. **D2 (ASCII hyphen in the pinned failure line)** is the one place the diff knowingly differs from a law-quoted string — rule it.
2. **The R1 hook site**: the attempt rides `RefreshCloudBlock`'s linked branch (the seam law's "on its cloud-block refresh"), not a bespoke NativeConstruct call — verify you agree the caller-established half of the predicate (linked/resolved/configured/not-in-flight) is airtight at that site; both dereferences are guarded by `bLinked`/`bConfigured` by construction.
3. **Latch semantics D5/D6** — mid-session expiry (~1 h) can trigger the activation's one attempt on a later cloud-block redraw if it wasn't spent at seed. Still ≤1 per activation; flag if you read the law stricter (seed-only).
4. **R2 ripple**: `BuildSuccessSummary` lost its skip branch and `FSyncContext` lost two members — confirm no other reader existed (grep `bPullSkippedNoBaseline` = zero).
5. **The §5 behavior changes** (especially #3, stamp-filtered login pull) — deliberate, law-derived; confirm against your ACC-§13 reading.
6. The subsystem getter comments name their consumers — check the "must not grow other callers" wording matches the ACC-§11 dated addition's intent.
7. Compile-trap sweep of the new lines: no `*/` inside any added doc comment; no shadowing (`StoredRefreshToken`/`RotatedToken`/`Accounts` are fresh in their scopes; the sync `MakeContext` local `Accounts` does not collide — the function had no prior local); `TestTrue(==)` FDateTime idiom per §6.

## 11. Slack

Posted to ⚙️ Dev & QA (`C0BF0QZP3CN`, thread `1783116269.740549`), prefix `⚙️ GAMEPLAY-PROGRAMMER:`, ✅ TASK-653 completion note.
