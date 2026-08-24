# QA Report — TASK-648 — THE CLIENT-LANE QA GATE (ACCOUNTS PHASE 2)
Covers, by the ledger: **TASK-643 · TASK-644 · TASK-645 · TASK-646 · TASK-647** — the union IS the roster.
Reviewer: qa-reviewer · Date: 2026-08-23 · Law: CONVENTIONS `ACC-§10..§15`, P2-R2/R5/R6, `SC-§13/§14/§20/§27` · ⛔ No edits, no compile, no git writes, no TASKBOARD edit made here.

## Verdict

| Task | Verdict | Blockers | Warns | Nits |
|---|---|---|---|---|
| TASK-643 `USiegeCloudClient` + Build.cs D1 | **PASS** | 0 | 0 | 2 |
| TASK-644 cloud-link model | **PASS** | 0 | 0 | 1 |
| TASK-645 `FSiegeCloudSync` | **PASS** | 0 | 1 | 1 |
| TASK-646 `UAccountMenuWidget` P2 | **PASS** | 0 | 1 | 0 |
| TASK-647 `Tests/SiegeCloudTest.cpp` | **PASS** | 0 | 0 | 1 |
| **Cross-lane** | — | 0 | **1** | — |
| **GATE** | **PASS** | **0** | **3** | **5** |

**Gate verdict: PASS — 0 blockers / 3 warns / 5 nits.** TASK-649 may proceed. The three warns carry a mandatory manager-rider list (§6); none violates a binding law and none is fixable in-batch without amending the `ACC-§15` registry, which this gate cannot authorize.

---

## 1. The named seam docket — rulings ON THE RECORD

### 1.1 The `HTTP` Build.cs deviation (643 D1 / 645's open Q1 / 647's watch item) — ⚖️ ACCEPTED, 643 IS THE OWNER OF RECORD
- `ACC-§11` pins `FHttpModule` as the transport by law; the module had no HTTP dependency; without the token TASK-649 fails at `#include "HttpModule.h"`. The TASK-417 precedent (Json/JsonUtilities landed with their first user) applies cleanly — `SiegeCloudClient.cpp` is the module's only FHttpModule user.
- **Verified at the file** (`GitClaudeUnrealTest.Build.cs`): the diff is exactly one dependency token `"HTTP"` (line 104) + one comment block (80–103). The two standing "⛔ NOT HTTP / NOT Sockets" closed-decision comments (TASK-417 block lines 52–58, TASK-443 block lines 68–77) are **untouched byte-for-byte**. The reconciling comment correctly re-points: the LLM ruling rejected a LISTENING sidecar (inbound socket → firewall prompt); FHttpModule is outbound-only client HTTPS under the later ACC-§10/§11 law. **`"Sockets"` is verified ABSENT from the file.** Module name `"HTTP"` is the correct UE module identifier.
- 645's and 647's unowned flags are hereby discharged; the reconciliation wording is right.

### 1.2 The 643↔647 config-seam reconciliation — ⚖️ BOTH STAND, NO DIFF OWED
- 643's `LoadCloudConfigFile`/`ExtractCloudConfig` statics are **not dead code**: they ARE the production config path (`Initialize` → `LoadCloudConfigFile` → `ExtractCloudConfig`). No dangling symbol, no double implementation.
- 647 tested the ini seam via `FConfigFile::CombineFromBuffer`/`GetString` directly — the same `FConfigCacheIni` family 643's `Initialize` is pinned to, on the law's exact `[SiegeCloud]` shape including the custody-comment invisibility proof. It exercises the same PARSE contract 643 ships (section/key lookup + IsValid composition).
- Gap stated honestly: 643's A4 normalization (trim + trailing-`/` strip inside `ExtractCloudConfig`) is not exercised offline. NIT only — `ExtractCloudConfig` is public, static and pure, so a one-line future test can pin it; not owed this batch (`SC-§20`: hypothesis, not a demand).

### 1.3 TASK-644's four declared decisions — ⚖️ ALL FOUR UPHELD
1. **`SetLastSyncUtc` broadcasts — UPHELD.** The TASK-644 board spec sentence (the task authority) explicitly lists all three mutators as broadcasting; the `ACC-§15` inline comment ("saves registry") is true-but-non-exhaustive and the §15 footer sentence describes the two link mutators without excluding a third. **Verified at source: `SiegeAccountSubsystem.cpp:356` broadcasts** — and this is load-bearing: 645's `FinishSync` stamps LAST so this broadcast rides the P1 `HandleActiveProfileChanged → ReloadForActiveProfile` wire and pulled settings go live with zero protected-file edits. The cross-lane dependency HOLDS at source. (Doc-only rider R4 below closes the §15 wording ambiguity.)
2. **`ClearCloudLink` resets `LastSyncUtc` — UPHELD.** A stale clock surviving re-link would suppress the trigger-1 pull for every older row (verified against `ShouldPullRow`'s strict `>`). The ACC-§11 "local profile survives" pin holds and is test-pinned by 647.
3. **`SetCloudLink` trims + refuses empty Email/UserId, empty RefreshToken accepted — UPHELD.** Whole-refusal keeps the `IsCloudLinked` conjunction predicate never-half-set (this also answers 647's named-untested partial-link edge: it cannot occur); refusals save nothing and broadcast nothing per the delegate law.
4. **Identical-values no-op guards — UPHELD.** Delegate law correctly extended; token ROTATION is a real mutation (saves + broadcasts). Consistent with 647's exactly-once pins: fresh-create → first `SetCloudLink` always differs → exactly one broadcast; `ClearCloudLink` on a linked profile always has state → exactly one.
- Open Q (shared `BroadcastActiveProfileChanged` reuse) — **ACCEPTED**; the amended delegate doc covers the semantics. NIT: the funnel's log line "Active profile changed — now X" is slightly misleading wording on a link mutation where identity did not change (harmless; consumers re-read state per contract). The consequent settings reload on every cloud-link mutation is idempotent and menu-path-only.
- 647's exactly-once-broadcast and guest-no-write pins are **CORRECT against 644 as shipped** (guest paths early-return before any save; verified lines 235–242, 285–291, 332–340).

### 1.4 TASK-645's D1–D5 — ⚖️ ALL FIVE ACCEPTED AS SHIPPED
- **D1 (session-scoped mirror; no-baseline SyncNow SKIPS pull) — ACCEPTED, with rider R2.** The registry pins `SetLastSyncUtc` and no reader, so the mirror is the lawful shape. The conservative branch is right under LWW: an unfiltered SyncNow pull could stomp local edits and then push-all would upload the stomped values. Cross-session cost honestly stated (first SyncNow of a session pushes without pulling). WARN-level recorded limitation; the named one-line cure (a `LastSyncUtc` getter on 644's surface, swap site `MakeContext()`) becomes **manager rider R2** — currently its cost is shadowed by the cross-lane re-auth gap (§1.5) anyway, so the two riders bundle naturally.
- **D2** (stamp after any full success) — strictly protective, keeps `ShouldPullRow` monotone. ACCEPTED.
- **D3** (server-now = read-back of the just-written settings row; settings pushed LAST; UtcNow fallback only on empty-cloud pull) — sound; skew posture honestly declared. ACCEPTED.
- **D4 — VERIFIED AT 643's SOURCE:** `UpsertRow` builds `FString::Printf(TEXT("%s/rest/v1/%s"), *ProjectUrl, *Table)` — plain concatenation, Table verbatim, no encoding/validation (`SiegeCloudClient.cpp:255`). `decks?on_conflict=user_id,deck_name` passes through intact. The seam HOLDS.
- **D5** (per-user in-flight guard, RAII-cleared) — verified: `~FSyncContext` clears the flag on every exit path including dropped chains. ACCEPTED. (Static-teardown corner and case-sensitive deck-name merge: recorded judgments, accepted; the latter mirrors the server's text unique — nit-level cosmetic possibility only.)

### 1.5 TASK-646's deviations + ⚠️ THE CROSS-LANE RE-AUTH QUESTION
- **Refresh token parsed from auth `PayloadOrError` — containment CORRECT.** `ParseAuthPayload` never logs; the token lives in `HandleCloudAuthResult` locals, reaches exactly one sink (`SetCloudLink`), and the raw payload is never displayed or logged on any path. The registry pins no getter, so the response body is the only lawful source. ACCEPTED.
- **⚠️ THE RE-AUTH GAP IS REAL — cross-lane WARN + mandatory rider R1.** Verified at the call graph: **`RefreshSession` has ZERO callers in the entire Source tree** (643 implements it; 645's preflight merely refuses "not signed in to cloud"; 646 deliberately does not consume it; no lane reads the persisted `CloudRefreshToken` — no getter exists). After a game restart the access token (memory-only) is gone, `IsCloudLinked` is still true, and `RefreshCloudBlock` hides `LinkCloudButton` while linked — **so a linked profile has NO in-game path to re-authenticate: every Sync Now fails honestly ("not signed in to cloud") until the code changes.** No law is violated (nothing blocks, gates, or crashes; degrade is honest per ACC-§11), and the fix requires an `ACC-§15` amendment this gate cannot authorize — so it does not block, but it is NAMED, not waved: **manager rider R1**, recommended before Jonathan's second TASK-651 sitting (the single-session link→sync→play script is unaffected).
- **No cloud-unlink UI — confirmed at the call graph** (`ClearCloudLink` has zero product callers; tests only). Recorded gap → **rider R3**.
- **`TSharedRef<FSiegeCloudSync>` lifetime — CONFIRMED NO LEAK.** 645 holds the completion delegate only inside the per-sync `FSyncContext`; the context is referenced only by the HTTP continuation lambdas; when the chain ends (success, failure, or dropped chain on request completion) the context destructs, releasing `OnDone` and with it the widget's captured `Sync` ref. The engine object holds no reference to the context — no retain cycle is possible.
- Sign-up/sign-in confirm-box convention (filled=SignUp, blank=SignIn, stated to the player) — lawful, smallest design. Success payloads never displayed/logged — verified. `Json` already present (Build.cs line 59) — 646's assumption discharged by 643's file.
- **WARN (646): the confirm-email-pending shape.** If SignUp returns success WITHOUT a session (S2 dashboard flip still owed), 646 still stores the link (user id from the payload) and then PushAll fails into `CloudStatusText` as "Linked as <email> - sync failed: not signed in to cloud" — 643's cross-task note 4 asked for a "confirm your email" surface when `IsCloudAuthenticated()` is still false, which 646 did not implement. Honest, non-blocking, and moot once Jonathan flips S2 — named for TASK-651 awareness and folded into rider R5.
- **ACC-§5(b) — HOLDS.** `RebuildWidget()` is the three-line shape (`Initialize(); ConstructAccountTree(); return Super::RebuildWidget();`); all new construction is inside `ConstructAccountTree()` before Super; every child BindWidgetOptional + construct-only-if-null; the escape hatch returns first. (Byte-identity to P1 verified structurally — this role holds no git surface; the shape and order are exactly the TASK-444 law.) **Escape law intact: zero `OnKeyDown`/`NativeOnKeyDown`/`EKeys` hits in the file.** `BackPressed` = `RemoveFromParent()` only; BackdropBorder hit-test VISIBLE untouched; Back never disabled on any cloud path.

### 1.6 TASK-647 — the suite
- **8 tests, all `Siegebound.Cloud.*`; suite expectation 118 → 126 for TASK-649 — arithmetic confirmed** (8 `IMPLEMENT_SIMPLE_AUTOMATION_TEST` blocks counted; no other client-lane task adds tests; P1 `Siegebound.Account.*` untouched — scratch slots verified distinct by name: `SiegeAccounts_CloudAutomationScratch` vs `SiegeAccounts_AutomationScratch`).
- **Zero-network — MECHANICALLY CONFIRMED**, and one step further than the handoff: 643's `SignOut()` places its unauthenticated early-return BEFORE any HTTP construction (`SiegeCloudClient.cpp:171-176`), so the one client mutation the tests exercise structurally cannot reach the wire even against a defect. No I/O member is ever invoked; the client is never `Initialize`d; config parse is a string buffer.
- The exactly-once and guest-no-write pins are ruled correct (§1.3). Derivations 1–8 all law-consistent; the `SetLastSyncUtc`-broadcast non-assertion passes under the §1.3 ruling as designed.
- Fixture hygiene: `FCloudScratchGuard` deletes in AND out incl. seed-copy artifacts; CI-clean both ways. Scratch identities visibly fake; zero `eyJ`-shaped literal, zero live ref (grep-confirmed).

---

## 2. `ACC-§15` registry conformance — RE-DERIVED, character-for-character
Compared directly against the CONVENTIONS §15 code block (not the handoff tables):
- **Block 1** (`SiegeCloudClient.h`): `FSiegeCloudConfig` (fields, inline comments, `bool IsValid() const`), `FSiegeCloudResult`, `FOnSiegeCloudStateChanged`, class decl + all 10 pinned members incl. alignment spacing and pinned trailing comments — **EXACT**. Additions (config-seam statics, `CloudStateChangedBroadcastCount`, privates) are additive, declared, non-colliding.
- **Block 2** (`SiegeCloudSync.h`): all six signatures incl. the class-line comment and pinned trailing comments — **EXACT**. Zero public-surface drift; zero instance members.
- **Block 3** (`SiegeAccountSaveGame.h`): four fields, registry order, appended after `LastLoginUtc`, specifiers exact; the doc-comment-per-field formatting transformation matches the shipped P1 precedent — **EXACT** in specifier/type/name/order.
- **Block 4** (`SiegeAccountSubsystem.h`): five methods verbatim incl. `bool    ` alignment; three mutators correctly non-UFUNCTION; footer contract (mutate active-only → save → broadcast) implemented — **EXACT**.
- Consumers (645/646/647) consume only pinned spellings — spot-verified at every call site read.

## 3. The mandated greps (all run by this reviewer, `SC-§14` positive-controlled)
| Grep | Scope | Result |
|---|---|---|
| P2-R2: `service_role\|sb_secret\|eyJ\|cjgqqeogsynrphowdcdp` | entire `Source/` | **ZERO real hits** — every `eyJ` match is the substring in `…KeyJust…`/`EKeys` identifiers (which doubles as proof the instrument fires); positive control `service_role` on CONVENTIONS.md = 2 hits, fires |
| P2-R6: `CredentialHashHex\|CredentialSaltHex` | Siegebound/ | Hits ONLY in the P1 account files + P1 test (fields, law comments, P2 fence comments). **ZERO in any cloud-lane file**; no new API reads either field; log-line audit per file: no password/token/hash/salt/email-as-credential ever logged (644's one token-adjacent line prints `held|none` literals only) |
| P2-R5 protected files: `SiegeCloud\|CloudClient\|CloudSync\|CloudLink\|LastSyncUtc` | `DeckBuilderWidget.cpp` · `SiegePlayerController.cpp` · `SiegeSettingsSubsystem.{h,cpp}` | **ZERO hits in all four** — no cloud-lane content exists in any protected file. (This role has no shell/git surface; content-zero here + 645's recorded porcelain-empty check is the evidence; TASK-649 re-runs the git trio per its own spec `SC-§9`.) |
| ACC-§4 slot literals: `TEXT("SiegeDecks")\|TEXT("SiegeSettings")` | Siegebound/ | Only the two definition sites + the tests (the law's exempt set). Zero in cloud files |
| Escape law: `OnKeyDown\|NativeOnKeyDown\|EKeys::Escape` | `AccountMenuWidget.cpp` | **ZERO** |

## 4. Compile-trap + API sweep (this lane compiles nowhere until 649 — reviewed by eye + installed-engine headers)
- **UE 5.8 API surface verified against `C:\Program Files\Epic Games\UE_5.8` headers:** `EHttpResponseCodes::IsOk` (IHttpResponse.h:38) ✓ · `FConfigFile::Read(const FString&)` exists, **returns void** — 643 calls it as a statement, correct ✓ · `FConfigFile::GetString(Section, Key, Value) const` (ConfigCacheIni.h:815) ✓ · `FConfigFile::CombineFromBuffer(Buffer, FileHint, bool=true)` — 647's 2-arg call valid ✓ · `TDelegate::BindWeakLambda` ✓ · `GetTransientPackageAsObject` (UObjectGlobals.h:276) ✓ · `UTextBlock::SetFontSize(float)` (TextBlock.h:222) ✓.
- **P1 surfaces the lane consumes verified present:** `USiegeDeckSaveGame::{SlotName, UserIndex(constexpr), SavedDecks, ActiveDeckName}` · `FDeckList::{DeckName, Cards}` / `FDeckCardEntry::{CardID, Count}` · `USiegeSettingsSubsystem::{SettingsSlotName, SettingsUserIndex, ReloadForActiveProfile, HandleActiveProfileChanged}` · `USiegeSettingsSaveGame::bAssistantConfirmBeforeExecute`.
- No `*/` inside any doc comment (643's snippet comment deliberately de-fanged); all `Printf`/`UE_LOG` formats literal with matching args (incl. the `*FString`-temporary idiom — lifetime covers the full expression); no shadowing (645's inner lambdas rename to `bFetchOk2`/`Body2`); complete-type includes present (`SiegeCloudSync.h` includes `SiegeCloudClient.h` + `Dom/JsonObject.h` for pinned types; fwd-decls elsewhere); `const_cast` twin sound (non-const storage, non-const `this`); `FMath::Max<FDateTime>` valid; TSharedRef→TSharedPtr conversions valid.
- **Trailing-defaulted-parameter law: ZERO new defaulted parameters across all five tasks** — every new declaration read; all audits vacuous exactly as the handoffs state (644 additionally pre-grepped its nine new names to zero pre-existing call sites).
- **M8 declarations:** present verbatim in all five handoffs AND in the code headers (643 h · 644 h/save h · 645 h · 646 h · 647 file header); consistent with the code — no replication, no RPC, client-local HTTPS only.

## 5. Cloud-off byte-identity + no-block (ACC-§11), per task
- **643:** subsystem auto-instantiates; with no ini it loads nothing, logs ONE line, every entry point fails fast through its delegate. Verified: only the four sibling-task files reference `USiegeCloudClient`. No Deck/Settings/Account include in the pair.
- **644:** guest path executable-diff zero (all three deletions are doc sentences, extended in place); new fields default-empty; guest calls early-return. 647 pins it.
- **645:** every trigger prefights configured→linked→authenticated and refuses with one log line + `OnDone(false,…)`, zero local mutation. Everything delegate-async; dead-world drops the chain.
- **646:** cloud-off lands on the ACC-§11 states-and-disables branch (the law itself mandates the block RENDERS disabled — never hides, gates nothing); the four P1 extension points are mode- or null-gated off in a Phase-1 world; no local flow gated anywhere.
- **647:** tests never require or leave any file; guest/unconfigured defaults pinned.
- **No gameplay path blocks on HTTP anywhere in the lane** — verified: every network call is delegate-continuation; no wait, no poll, no tick.
- **Dependency direction (stated as found):** Cloud client includes no Deck/Settings/Account header ✓ · the sync engine includes the deck/settings SAVE-DATA headers (`SiegeDeckSaveGame.h`, `SiegeSettingsSaveGame.h`, `DeckTypes.h`) per its spec's own load-modify-save order, plus a constant-only `SiegeSettingsSubsystem.h` include (`SettingsUserIndex`) — Cloud→data-model, never Cloud→widget/controller/live-subsystem-state ✓ · Account includes ZERO cloud headers (644's API required none) ✓ · the test TU includes everything (exempt, stated) ✓.

## 6. 📋 THE MANAGER-RIDER LIST (carried out of this gate)
1. **R1 — post-restart re-auth lane (functional, decide before TASK-651's second sitting):** no lane calls `RefreshSession` and nothing can read the persisted `CloudRefreshToken` (no getter pinned) ⇒ after a game restart a linked profile can NEVER re-authenticate in-game; Sync Now fails honestly forever and `LinkCloudButton` is hidden while linked. Cure = an `ACC-§15` amendment (a token read or an internal 644→643 refresh wire) + one caller. Single-session use (the 651 script) is unaffected.
2. **R2 — `LastSyncUtc` getter on 644's surface** (645-D1's named one-line cure; swap site `MakeContext()`): collapses both D1 branches to the law's letter. Bundles with R1.
3. **R3 — cloud-unlink UI:** `ClearCloudLink` has zero product callers; ACC-§14 pins no child for it. One pinned-child one-liner or Phase-3.
4. **R4 — doc-only:** dated `ACC-§15` comment amendment recording that `SetLastSyncUtc` broadcasts (this gate's §1.3 ruling, into the law text).
5. **R5 — small/UX:** surface "confirm your email" when SignUp succeeds without a session (`IsCloudAuthenticated()` still false) — moot once Jonathan's S2 dashboard flip lands; until then 651 will see "sync failed: not signed in to cloud" on a confirm-pending signup. Awareness note for the 651 script.

## 7. Notes for build-master — TASK-649 PRE-FLIGHT FILE ENUMERATION
**Reviewed by THIS gate (the five tasks' complete `Source/` + Build.cs set):**
1. `Source/GitClaudeUnrealTest/GitClaudeUnrealTest.Build.cs` — EDIT (643 D1: the `"HTTP"` token + comment block)
2. `Source/GitClaudeUnrealTest/Siegebound/SiegeCloudClient.h` — NEW (643)
3. `Source/GitClaudeUnrealTest/Siegebound/SiegeCloudClient.cpp` — NEW (643)
4. `Source/GitClaudeUnrealTest/Siegebound/SiegeAccountSaveGame.h` — EDIT (644)
5. `Source/GitClaudeUnrealTest/Siegebound/SiegeAccountSubsystem.h` — EDIT (644)
6. `Source/GitClaudeUnrealTest/Siegebound/SiegeAccountSubsystem.cpp` — EDIT (644)
7. `Source/GitClaudeUnrealTest/Siegebound/SiegeCloudSync.h` — NEW (645)
8. `Source/GitClaudeUnrealTest/Siegebound/SiegeCloudSync.cpp` — NEW (645)
9. `Source/GitClaudeUnrealTest/Siegebound/AccountMenuWidget.h` — EDIT (646)
10. `Source/GitClaudeUnrealTest/Siegebound/AccountMenuWidget.cpp` — EDIT (646)
11. `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeCloudTest.cpp` — NEW (647)

**Riding the same compile slot under its OWN gate (GH-R13 — TASK-637 sweep, gated by TASK-638, NOT reviewed here):**
12. `Source/GitClaudeUnrealTest/Siegebound/Torch.h` · 13. `Source/GitClaudeUnrealTest/Siegebound/Castle.h` · 14. `Source/GitClaudeUnrealTest/Siegebound/Castle.cpp`

**Expected ABSENT from the Source/ diff:** `SiegeAccountSaveGame.cpp` (644's names block lists it; ZERO edits shipped — its appearance in a diff would be an unexplained line). **Any `Source/` or Build.cs line outside 1–14 is unreviewed by this gate — stop and escalate.**

**Known non-Source tree residents (TASK-650 commit cargo, not 649's compile surface):** `Tools/Supabase/migrations/0001_init_accounts.sql` · `Config/SiegeCloudDev.ini.example` · the one `.gitignore` line · handoffs 639..648 · `qa/TASK-640.md` + this report · board/CONVENTIONS deltas. ⛔ `Config/SiegeCloudDev.ini` is gitignored and must never stage (P2-R2 hard stop at 650).

**Suite expectation at 649: 126/126** (118 baseline + 8 `Siegebound.Cloud.*`); `SC-§26` full editor restart is mandatory (new reflected types: `USiegeCloudClient`, `FSiegeCloudConfig`, the profile fields, the widget additions). Re-run the git trio yourself (`SC-§9`) — this reviewer's session-start git snapshot was stale and this role holds no shell.

## Findings (consolidated, format `[SEV] file:line — issue — suggested fix`; every suggestion is a HYPOTHESIS per SC-§20)
- [WARN] cross-lane (SiegeCloudClient.h:132 / SiegeCloudSync.cpp:807 / AccountMenuWidget.cpp:1278) — `RefreshSession` has zero callers and `CloudRefreshToken` has no reader ⇒ no post-restart re-auth path for a linked profile — manager rider R1 (registry amendment + one caller).
- [WARN] SiegeCloudSync.cpp:366 — SyncNow with no session baseline skips its pull phase (645-D1); first SyncNow of a session pushes without pulling — accepted as local-first-safe; rider R2 (LastSyncUtc getter) collapses it to the law's letter.
- [WARN] AccountMenuWidget.cpp:1130-1170 — SignUp success-without-session (S2 not yet flipped) stores the link, then the A4 push fails "not signed in to cloud" instead of a "confirm your email" surface — rider R5; moot after the S2 hand-step.
- [NIT] SiegeCloudClient.h:158-165 — `LoadCloudConfigFile` doc says the seam exists "so TASK-647's offline tests can drive it"; 647 drove `FConfigFile` directly — re-point the sentence at the next legitimate touch of this file.
- [NIT] SiegeCloudClient.cpp:81 — the Norton/S3 sentence inside a player-visible error literal is dev-machine-specific — Phase-3 polish candidate, per the handoff's own note.
- [NIT] SiegeAccountSubsystem.cpp:514-516 — "Active profile changed — now X" log wording on cloud-link mutations where identity did not change — cosmetic; the amended delegate doc already covers semantics.
- [NIT] SiegeCloudSync.cpp:472-476 — case-sensitive deck-name merge vs a possibly case-insensitive local collision rule could leave two case-variant decks after a pull — recorded judgment (mirrors the server's text unique), cosmetic.
- [NIT] SiegeCloudClient.cpp:294-317 — `ExtractCloudConfig`'s trim + trailing-slash normalization has no offline test — one-line future test candidate (public static pure seam).

*A false pass costs an engine crash; nothing in this lane executes until TASK-649's compile + SC-§26 bounce, and TASK-650's live smoke re-verifies the wire first-hand. This gate's PASS is scoped to what a pre-compile eye can hold: conformance, safety, law, and the seams above.*

---

## 8. ⛔ TASK-649 COMPILE FAILURE — appended by build-master, 2026-08-23 (spends a QA loop)

**`Result: Failed (OtherCompilationError)` — 16.65 s total, UBA 11.95 s (NOT the ~2 s SAC signature; a real compile error).** Full log: session scratchpad `build-649.log`. Pre-flight had passed on every line (HEAD `1025160` unmoved; Source delta = exactly §7 items 1–14; `SiegeAccountSaveGame.cpp` absent; the TASK-637 trio confirmed comment-only at the byte level, net +6/+11/+9; `L_Arena` ENTRY hash match + `is_dirty` false). Editor bounced gracefully under Jonathan's explicit grant (clean exit ~6 s, ZERO save prompts) before the build.

**EXACTLY ONE error, file-named — owning task TASK-647 (gameplay-programmer):**

```
C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Source\GitClaudeUnrealTest\Siegebound\Tests\SiegeCloudTest.cpp(180,23): error C2672: 'TMapBase<KeyType,ValueType,SetAllocator,KeyFuncs>::GenerateKeyArray': no matching overloaded function found
note: could not deduce template argument for 'TArray<UE::TSharedString<TCHAR>,ArrayAllocator> &' from 'TArray<FString,FDefaultAllocator>'
with KeyType=UE::JSON::Private::FJsonObjectSharedStringStorage::FStringType (= UE::TSharedString<TCHAR>)
```

- Site: `JoinedSortedKeys()` helper, `SiegeCloudTest.cpp:177-183` — `TArray<FString> Keys; JsonObject->Values.GenerateKeyArray(Keys);`
- Mechanism (information, not a prescribed fix — SC-§20): in the installed UE 5.8, `FJsonObject::Values` is keyed on `UE::JSON::Private::FJsonObjectSharedStringStorage::FStringType` = `UE::TSharedString<TCHAR>`, NOT `FString` — so `GenerateKeyArray` cannot fill a `TArray<FString>`. This is the one 5.8 API surface §4's sweep did not reach (the sweep verified `CombineFromBuffer`/`GetString` etc., but not the `Values` key-storage type change).
- Blast radius: this single TU only. `SiegeCloudClient.cpp` [7/18] and `SiegeCloudSync.cpp` [8/18] compiled clean; no other error line in the log (`grep -E "error"` → 1 hit). No link ran; the editor binary on disk is the pre-build binary, untouched.
- Not run (gate stopped): the 126-suite. Suite expectation stays 126/126 for the re-run.
- ⛔ Fence held: no code edited by build-master, no commit, no TASKBOARD edit, no map save; `L_Arena` hash `b3dbc5d9…f8268` byte-identical through close. Any fix diff = CODE ⇒ SC-§27 verdict before TASK-650 commits.

---

## ADDENDUM — LOOP 1 FIX VERDICT — 2026-08-23 (qa-reviewer; scoped to the TASK-647 fix ONLY, per SC-§27)

**Verdict: PASS — 0 blockers / 0 warns / 0 nits on the fix diff.**

**Basis (everything recomputed first-hand, nothing trusted from the handoff):** The shipped helper body (`SiegeCloudTest.cpp:177-192`) is byte-identical to the handoff §9 after-text, and the change is confined to exactly that region — the helper's doc comment and signature are untouched, the 8 `IMPLEMENT_SIMPLE_AUTOMATION_TEST` blocks stand as this gate reviewed them, and tests 4/5's expected joined-key strings (`deck_name|payload|user_id` at :413, `payload|user_id` at :482) are unmodified. The header evidence was re-verified at the installed engine: `JsonObject.h:99` (`using FStringType = UE::FSharedString;`), `:237` (`TMap<FStringType, TSharedPtr<FJsonValue>> Values;`), `:322-324` (`FJsonObject` derives from the shared-string storage in the non-legacy branch — which the §8 error text itself proves is the active configuration), and `SharedString.h:80-83` (`[[nodiscard]] inline const CharType* operator*() const` returning a null-terminated pointer, `&NullChar` when empty — const-qualified, so it is callable on the map's const key). Therefore `Keys.Emplace(*Pair.Key)` perfect-forwards a `const TCHAR*` into the `FString(const TCHAR*)` constructor and the C2672 cannot recur at this site; the grep confirms `GenerateKeyArray` survives only inside the explanatory comment and `->Values` appears nowhere else in the file — no other key-type assumption exists. Behavior is preserved: the ranged-for collects the same key set the old call would have (once per key; `Reserve` is allocation-only), then the identical `Keys.Sort()` and `FString::Join(Keys, TEXT("|"))` run unchanged, so the observable contract feeding tests 4/5 is byte-identical. Robustness note: the fix compiles under BOTH storage branches (legacy `FStringType = FString` would also feed `Emplace` a valid `const TCHAR*` via `FString::operator*`). Compile-trap scan of the new lines: `//` comments only (no `*/` hazard), `Pair` is a fresh identifier with no shadowing anywhere in the file, `TMap::Num()`/ranged iteration/`Emplace` all valid on the deduced types, and no new include is needed (`Dom/JsonObject.h` line 6 and `Containers/UnrealString.h` line 5 already present). §8's own log evidence (exactly one error hit in the TU) confirms the rest of the file was already clean past this site.

**For the TASK-649 re-run:** the §7 pre-flight file enumeration is UNCHANGED — the fix touched only item 11 (`Tests/SiegeCloudTest.cpp`, still untracked-new as originally declared; the handoff §9 diff block is the diff of record since git holds no baseline). Suite expectation stays **126/126**; SC-§26 full restart still mandatory. This addendum discharges the SC-§27 verdict owed before TASK-650 commits.

---

## 10. ⛔ TASK-649 RE-RUN — SUITE FAILURE — appended by build-master, 2026-08-23 (spends QA loop 2 of 3)

**Compile: `Result: Succeeded` — 4.82 s, exactly 4 actions (`[1/4] Compile SiegeCloudTest.cpp` → lib → dll → metadata); the loop-1 C2672 is CURED on the real compiler.** Pre-flight passed on every line (HEAD `1025160` unmoved; Source delta = exactly §7 items 1–14; `SiegeAccountSaveGame.cpp` absent; 637 trio re-confirmed comment-only; `L_Arena` ENTRY hash match + `is_dirty` false; **PIE guard: `IsPIERunning` = false** — no playtest interrupted; graceful close, ZERO save prompts).

**Suite ACTUAL: 125/126 — EXACTLY ONE FAILURE** (headless, the TASK-593/606/625 command; `Automation Test Queue Empty 126 tests performed`, wall ~6 s; full log: session scratchpad `suite-649.log`):

```
Test Completed. Result={Fail} Name={ConfigIniParseSeam} Path={Siegebound.Cloud.ConfigIniParseSeam}
Error: Expected 'The parsed ProjectUrl is byte-identical to the buffer's value' to be
"https://scratchref.supabase.co", but it was "https:".
[Source\GitClaudeUnrealTest\Siegebound\Tests\SiegeCloudTest.cpp(277)]
```

- Site: `FSiegeCloudConfigIniParseTest::RunTest`, `SiegeCloudTest.cpp:264-278` — the scratch buffer's unquoted `ProjectUrl=https://scratchref.supabase.co` line, driven through `FConfigFile::CombineFromBuffer` + `GetString`.
- Mechanism (information, not a prescribed fix — SC-§20): **the installed UE 5.8 `CombineFromBuffer` parse treats `//` as an inline comment-start in an unquoted value** — everything from the first `//` is stripped, leaving `https:`. Measured live on this machine; this is a second 5.8 API surface no pre-compile eye could reach (§4's sweep verified the calls exist, not this parse semantic).
- ⚠️ **Blast radius is NOT test-only — the ACC-§11 shape itself is implicated.** TASK-643's production `Initialize()` reads the real `Config/SiegeCloudDev.ini` through the same FConfigCacheIni family, and `Config/SiegeCloudDev.ini.example` ships the same unquoted `ProjectUrl=https://…` shape. Whether the file-load path shares the buffer path's comment semantics is UNVERIFIED (hypothesis for the owner to measure) — if it does, the real config truncates identically at runtime and cloud silently never configures. A known-good UE idiom exists (quoted ini values survive with quotes stripped on read), but the cure and its `ACC-§11`/`.ini.example` ripple are the owner's call, not this gate's.
- Owning files: `Tests/SiegeCloudTest.cpp` (TASK-647) at minimum; potentially `SiegeCloudClient.cpp` Initialize + `Config/SiegeCloudDev.ini.example` (TASK-643) if the file-load path shares the semantic → both gameplay-programmer.
- The other 125 PASS, including the remaining 7 `Siegebound.Cloud.*`; the 118 baseline is intact — no regression outside this one seam test.
- ⛔ Fences held: no code edited by build-master, no commit (650's), no TASKBOARD edit, no map save. Editor relaunched on the NEW binary per SC-§26 and left UP for Jonathan; `L_Arena` hash byte-identical through the whole bounce.
- Any fix diff = CODE ⇒ SC-§27 verdict before TASK-650 commits; suite expectation for the loop-2 re-run stays **126/126**.

---

## ADDENDUM — LOOP 2 FIX VERDICT — 2026-08-23 (qa-reviewer; scoped to the §10 ini `//`-truncation fix ONLY, per SC-§27)

**Verdict: PASS — 0 blockers / 0 warns / 0 nits on the fix diff.**

**Basis 1 — the engine-evidence chain, recomputed first-hand at `C:\Program Files\Epic Games\UE_5.8`:** every load-bearing row of the handoff's STEP-1 table was re-read at the cited engine source and confirmed. `FConfigFile::Read(Filename)` = `::FillFileFromDisk(this, Filename, false)` (ConfigCacheIni.cpp:2216-2219) → `FillFileFromBuffer(File, Text, …)` (:2182) — the SAME template `CombineFromBuffer` wraps (:2201-2204). That shared reader sets `ELineExtendedFlags::SwallowDoubleSlashComments` UNCONDITIONALLY (:1930-1934; the only exception is `bPythonConfigParserMode`, never ours). The swallow itself is `if (!bIsQuoted && (bHandleDoubleSlashComments && Stream[0]=='/' && Stream[1]=='/')) bIgnore = true` (Parse.cpp:1245-1250), quote-aware via the toggle at :1287 (the quote chars themselves are appended into the line, so value processing sees the leading `"`); the value is then quote-stripped via `FParse::QuotedString` (ConfigCacheIni.cpp:2148-2155 → ParseQuotedString, Parse.cpp:391-410 — opening `"` required, stop set `"\n\r\0`, escape handling touches only `\` sequences, which a URL cannot contain, so the round-trip is byte-identical minus the quotes). I additionally verified the ONE parameter that differs between the two entry paths, `bHandleSymbolCommands`, governs only the `+ - . ! @ * ^` prefix commands (:2076-2097) — no comment or value semantics — so the buffer↔file seam equivalence is exact. **Both handed-down conclusions therefore follow from the source as read: production WAS broken as shipped** (643's `LoadCloudConfigFile` calls `ConfigFile.Read(IniFilePath)` at `SiegeCloudClient.cpp:290`; the real ini's unquoted `https://…` truncated to `https:`, which is non-empty, so `IsValid()` still composed true — the silent-break, worse than clean ACC-§11 OFF) **and quoting alone suffices** — which is why zero `SiegeCloudClient.{h,cpp}` edits is the correct minimal shape, not a shortcut.

**Basis 2 — the diffs, verified at all three files:** the real gitignored `Config/SiegeCloudDev.ini` (read for SHAPE only) carries the quoted `ProjectUrl="https://cjgqqeogsynrphowdcdp.supabase.co"` — the live project ref matches — plus the must-stay-quoted comment; the AnonKey line and every custody comment are untouched, and AnonKey is correctly left UNQUOTED: a JWT is three base64url segments joined by `.`, and the base64url alphabet (`A-Za-z0-9-_`) structurally cannot contain `/` (nor `;`), so no comment swallow can ever fire on it — confirmed against the actual line's shape (value not reproduced here, P2-R2). `Config/SiegeCloudDev.ini.example` carries the identical quoted shape on the `<project-ref>` placeholder + the same comment. The handoff prints only the ProjectUrl line; the AnonKey value appears nowhere in it.

**Basis 3 — the test, verified at the file:** the main buffer (:285) and the incomplete buffer (:343) both use the quoted shipping format; the main ProjectUrl assertion's EXPECTED value is unchanged (`https://scratchref.supabase.co`, :296) so the assertion now proves quote-strip-on-read; the new hazard-pin block (:324-337) is exactly the three declared assertions — the unquoted readback still SUCCEEDS through `GetString` (silent, not a parse error), equals exactly `https:` (byte-compared; the value the engine's trim rules produce, matching §10's live measurement), and still composes `IsValid()==true` — pinning the silent-break so the ini can never be un-quoted without a loud suite failure. Test count still 8 `IMPLEMENT_SIMPLE_AUTOMATION_TEST` blocks, all `Siegebound.Cloud.*` (grep-counted); tests 4/5's expected joined-key strings intact (`deck_name|payload|user_id` :454, `payload|user_id` :523); the loop-1 helper region stands unmodified; no other test block touched. Compile-trap scan of the new lines: the ⭐ LOOP-2 doc paragraph contains no `*/` sequence; `UnquotedFile`/`TruncatedUrl`/`TruncatedConfig` are fresh identifiers with no shadowing; the escaped-quote `TEXT()` literals are well-formed; no new include needed (`Misc/ConfigCacheIni.h` at line 10 already covers everything the block uses).

**Basis 4 — scope + secrets:** exactly the three declared files. `SiegeCloudClient.{h,cpp}` verified genuinely unedited — this gate's prior line anchors still hold at their exact cited lines (`SignOut` early-return :171-176 · the `UpsertRow` Printf :255 · `ExtractCloudConfig` :294-317, whose trim + trailing-`/` normalization applies unchanged to the already-bare URL) and a case-insensitive `quote|loop-2|truncat` grep over the pair returns ZERO. The `Tests/` directory holds no new file. P2-R2 re-run over `Source/`: zero real hits (every `eyJ` match is the benign `…KeyJust…`/`EKeys` class — the instrument demonstrably fires; zero `service_role`/`sb_secret`/live-ref).

**For the TASK-649 loop-3 re-run:** the §7 pre-flight file enumeration is UNCHANGED — the fix touched only item 11 (`Tests/SiegeCloudTest.cpp`, still untracked-new) plus the two `Config/` ini files, which are NOT Source/Build.cs surface: `SiegeCloudDev.ini.example` is the already-declared TASK-650 commit cargo (§7's non-Source residents list) in its new quoted shape, and the real `SiegeCloudDev.ini` remains gitignored and must never stage (P2-R2 hard stop at 650). Suite expectation stays **126/126**. This addendum discharges the §10 SC-§27 rider owed before TASK-650 commits. One follow-on for the manager joins §6 as **R6 (doc-only):** the handoff's flagged dated `ACC-§11` format amendment — `ProjectUrl` MUST be double-quoted (engine swallows unquoted `//`; quotes stripped on read), `AnonKey` stays unquoted (base64url, `/` impossible).
