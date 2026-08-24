# TASK-647 — Programmer handoff — OFFLINE AUTOMATION TESTS `Tests/SiegeCloudTest.cpp`

- **Status: ready-for-qa** (orchestrator flips the board — no TASKBOARD edit made here)
- **Files touched:** exactly ONE, new: `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeCloudTest.cpp`
  - ⛔ Zero implementation files touched (QUIET-MODULE + the task fence held). Zero Build.cs edits ("Json" is ALREADY a dependency — verified at line 59; the "Http" addition is TASK-643's, see §6).
  - ⛔ Zero compile run (TASK-649 owns the lane's one). ⛔ Zero network, zero editor/MCP, zero git.
- **Written against the ACC-§15 pinned signature registry, character-for-character** — NOT against the in-flight 643/644/645 files (authored in parallel). The compile gate (TASK-649) is where the two sides reconcile; the file header carries the standard my-defect-vs-finding rule.

## 1) Test inventory — 8 new tests, all `Siegebound.Cloud.*`, all offline

| # | Test | What it observes |
|---|---|---|
| 1 | `Siegebound.Cloud.ConfigValidityTruthTable` | `FSiegeCloudConfig::IsValid` four-row truth table (neither/url-only/key-only/both) per the pinned "both non-empty". Default-constructed = the missing-ini cloud-OFF state ⇒ invalid. |
| 2 | `Siegebound.Cloud.ConfigIniParseSeam` | The ACC-§11 config-home FORMAT seam from a **scratch string buffer** via `FConfigFile::CombineFromBuffer`/`GetString` (the same `FConfigCacheIni` family 643's `Initialize` is pinned to): `[SiegeCloud]` `ProjectUrl`/`AnonKey` parse byte-identically; the `; DbPassword=` custody line is INVISIBLE to the parser (comment, the game never reads it); an incomplete buffer ⇒ invalid config (cloud OFF, not error); no-section buffer ⇒ clean lookup failure. No file on disk. |
| 3 | `Siegebound.Cloud.ShouldPullRowBoundaryMatrix` | `FSiegeCloudSync::ShouldPullRow` pure matrix, 7 rows: one-second newer ⇒ pull · **one-TICK newer ⇒ pull (strictness boundary)** · older ⇒ no · **equal ⇒ no (ACC-§13 is strictly `>`)** · unset LastSyncUtc + real cloud stamp ⇒ pull · unset cloud stamp ⇒ no · both unset ⇒ no. |
| 4 | `Siegebound.Cloud.DeckRowJsonShape` | `MakeDeckRowJson` output parses as a JSON object whose top-level key set is **EXACTLY** `deck_name\|payload\|user_id` (sorted, `TestEqualSensitive` on the joined key names — SC-§13); `updated_at` ABSENT and `id` ABSENT asserted by name; `user_id`/`deck_name` round-trip byte-identically (deck name carries embedded quotes ⇒ escaping proven); `payload` is a JSON **object** (jsonb projection, not a double-encoded string) with string/number/array fields intact. |
| 5 | `Siegebound.Cloud.SettingsRowJsonShape` | Same instrument for `MakeSettingsRowJson`: exact key set `payload\|user_id`; `updated_at` and `deck_name` ABSENT; payload object round-trips number/bool/string. |
| 6 | `Siegebound.Cloud.CloudLinkRoundTrip` | The 644 link API on a scratch registry slot: create → pre-link unlinked/empty-email → `SetCloudLink` ⇒ `IsCloudLinked` true, `GetLinkedEmail` byte-identical, **exactly one** `OnActiveProfileChanged` broadcast (via the P1 `ActiveProfileChangedBroadcastCount` observation point), registry file REAPPEARS after a pre-delete (isolates SetCloudLink's own save) · linking changes NO local identity (ProfileId/display name/deck+settings slot names byte-identical) · `SetLastSyncUtc` re-saves (delete-then-reappear again) · **cross-instance persistence**: a second scratch instance + `LoadAccountsFromSlot` restores the active profile AND the link (the SaveGame-tagged P2 fields persisted) · `ClearCloudLink` on the reloaded instance ⇒ unlinked, email empty, one broadcast, saved, and **the LOCAL profile survives** (still logged in, same ProfileId/name — ACC-§11). Also pins scratch-slot ≠ shipped-slot mechanically. |
| 7 | `Siegebound.Cloud.GuestSafeDefaults` | Guest queries: `IsCloudLinked` false, `GetLinkedEmail` empty. All three 644 mutators as guest are complete no-ops: nothing links, no profile fabricated, **no broadcast** (P1 delegate law: never on a no-op), **no registry write** (P1 save-on-CHANGE law). Writes nothing at all. |
| 8 | `Siegebound.Cloud.ClientUnconfiguredDefaults` | A `USiegeCloudClient` constructed but never `Initialize`d (= the missing-ini state): `IsCloudConfigured`/`IsCloudAuthenticated` false, `GetCloudUserId` empty; `SignOut()` is a graceful no-op leaving all three unchanged. No async method is ever called. |

## 2) 📊 THE NEW SUITE EXPECTATION FOR TASK-649

**Baseline 118 + 8 new `Siegebound.Cloud.*` = 126.** No other client-lane task (643..646) adds tests, so the lane's expected full-suite count at the TASK-649 gate is **126 / 126**, of which `Siegebound.Cloud.*` = 8 and the P1 `Siegebound.Account.*` set is untouched at its shipped count.

## 3) Fixtures used (all in-file, none on disk)

- **Scratch registry slot `"SiegeAccounts_CloudAutomationScratch"`** — distinct from BOTH the shipped `"SiegeAccounts"` (asserted mechanically in test 6) and `SiegeAccountTest.cpp`'s `"SiegeAccounts_AutomationScratch"` (so the two test files can never collide in one session). Redirected via the shipped `SetSlotNameForAutomationTests` before any operation.
- **`FCloudScratchGuard`** — the `FAccountScratchGuard` janitor, cloned: deletes the scratch registry in/out plus every RULING-8 seed-copy (`SiegeDecks_<Digits>`/`SiegeSettings_<Digits>`) of tracked profiles. **CI-clean both ways:** no save files needed (guest slots absent on CI ⇒ no seed copies made), none left behind either way.
- **Scratch ini string buffer** (test 2) — never a file; scratch project ref `scratchref`, placeholder key strings.
- **Scratch identities** — arbitrary uuids, lowercase emails (no casing claim rides on an email), placeholder token strings. ⛔ **P2-R2 self-check: no `service_role`, no `sb_secret`, no `eyJ`-prefixed literal, no live project ref (`cjgqqeogsynrphowdcdp`) anywhere in the file** — the "keys" are visibly-fake `scratch-*` strings.

## 4) Zero-network law — how it holds MECHANICALLY

No test calls any I/O member of the registry (`SignUp`/`SignIn`/`RefreshSession`/`FetchRows`/`UpsertRow`/`PullAll`/`PushAll`/`SyncNow` — never invoked). The one `USiegeCloudClient` is never `Initialize`d (no `FSubsystemCollectionBase` can be fabricated — the SiegeAccountTest precedent), so it holds no `ProjectUrl` and could not name an endpoint even through a defect. The single client mutation exercised, `SignOut()`, is pinned by ACC-§11/643 to no-op while unconfigured; with no URL and no token it cannot reach anything. Config parsing is from a string buffer.

## 5) Declared derivations / deviations (SC-§15 posture — where I pinned beyond the registry's literal text)

1. **Deck row: `id` ABSENT asserted** (beyond the spec's named `updated_at`-absent). Justification: `MakeDeckRowJson` takes no id; `id` is server-defaulted `gen_random_uuid()`; a client-sent id would fight the `Prefer: resolution=merge-duplicates` upsert lane pinned on `UpsertRow`. The exact-key-set instrument enforces it anyway; the named assertion just makes the failure legible. If 645 deliberately ships an `id` key, that is a FINDING to rule, not an auto-fix.
2. **`ShouldPullRow` equality ⇒ false.** Derived from ACC-§13's literal "updated_at **>** LastSyncUtc" (strictly greater). An implementation using `>=` fails rows 3–4 and 7 — by my reading that would be a registry/law deviation, not a test bug.
3. **`SetCloudLink`/`ClearCloudLink` broadcast EXACTLY once per call.** Derived from the ACC-§15 tail ("save the registry slot, then broadcast") + the P1 delegate law (never on a no-op; "broadcasts", singular). Asserted via the shipped `ActiveProfileChangedBroadcastCount`.
4. **Guest mutators: no broadcast AND no registry write.** Derived from the P1 delegate law + the load-once/save-on-CHANGE contract restated in the §15 tail ("mutate ONLY the active profile" — none exists ⇒ nothing changed ⇒ no save, no broadcast). If 644 read this differently (e.g. save-anyway), QA should rule which side moves.
5. **`SetLastSyncUtc` broadcast NOT asserted** — deliberate: the ACC-§15 tail names only SetCloudLink/ClearCloudLink as broadcasters while the TASK-644 spec sentence includes SetLastSyncUtc. The test asserts only the uncontested "saves registry" (file-reappearance). QA: this ambiguity is 644's to resolve; the test passes under either resolution.
6. **`SignOut()` called on an unconfigured client** (a mutation, beyond the "queries return safe defaults" minimum) — justified in §4; it is the one offline-observable slice of the ACC-§11 no-op law on the client surface.
7. **`payload` must embed as a JSON OBJECT, not a re-encoded string.** Derived from ACC-§13 "payloads = jsonb projections" + the `TSharedRef<FJsonObject>` parameter type; a string-embedded payload would double-encode into the jsonb column.
8. **Config truth table: whitespace-only strings deliberately NOT asserted** — the registry pins "both non-empty" only.

## 6) What is NOT covered offline — the honest gap list (owned by TASK-650 live smoke + TASK-651 sitting, per the ledger)

- **`Initialize()`'s real ini load** off `Config/SiegeCloudDev.ini` (not drivable in-process; driving it would also read the developer's REAL ini — a hermeticity violation). Test 2 covers the format seam only. → 650/651.
- **All HTTP request shaping beyond the row JSON**: endpoints, `apikey`/`Authorization: Bearer` headers, the `Prefer: resolution=merge-duplicates` header, query suffixes. No pinned pure seam exists for request construction. → 648 review-by-eye + 650 live smoke.
- **All response parsing** (GoTrue auth JSON, PostgREST row arrays) — lives inside 643's callbacks, no pinned pure seam. → 650.
- **`PullAll`/`PushAll`/`SyncNow` orchestration** incl. "LastSyncUtc = server now", load-modify-save of local slots, guest-never-syncs at the engine level. Only `ShouldPullRow` and the row serializers are pure. → 650/651.
- **The unconfigured ASYNC error shape** (does an unconfigured `SignIn` invoke the callback with bOk=false, or never invoke it?) — unpinned; deliberately not asserted. → QA's reading of 643 + 650.
- **`OnCloudStateChanged` broadcasts** (binding a dynamic delegate needs a UFUNCTION host — out of proportion offline). → 650/651 observation.
- **`CloudRefreshToken` / `LastSyncUtc` VALUE readback** — no pinned getter; the save-write and the reloaded link state are the observables here. → 650 (sync behavior proves the values) / 651.
- **RLS / server behavior** — entirely 650's negative probe + 642's advisors (already done).
- **Untested edge, named for QA's eye on 644:** partial `SetCloudLink` (e.g. empty `UserId`) — the `IsCloudLinked` conjunction pin ("LinkedEmail + CloudUserId") vs `GetLinkedEmail`'s "empty when unlinked" pin interact here; I did not assert it to avoid pre-judging 644's guard style.

## 7) QA scrutiny list (TASK-648)

- **Include-path contract:** the file includes `Siegebound/SiegeCloudClient.h` and `Siegebound/SiegeCloudSync.h` by the ACC-§14 names — 643/645 must land files at exactly those paths or the 649 compile reconciles against them.
- **Registry symbols consumed** (the full cross-task surface this file compiles against): `FSiegeCloudConfig{ProjectUrl,AnonKey,IsValid}` · `USiegeCloudClient{IsCloudConfigured,IsCloudAuthenticated,GetCloudUserId,SignOut}` · `FSiegeCloudSync{ShouldPullRow,MakeDeckRowJson,MakeSettingsRowJson}` (statics) · `USiegeAccountSubsystem{IsCloudLinked,GetLinkedEmail,SetCloudLink,ClearCloudLink,SetLastSyncUtc}` + the shipped P1 surface.
- **Build.cs observation (not my edit):** "Json" is already present (line 59) — my file needs nothing added. "HTTP" is absent and is 643's to add; ⚠️ the Build.cs carries a twice-stated "⛔ NOT HTTP" closed-decision comment scoped to the **LLM lane** (CONVENTIONS "In-match LLM command assistant" §6) — 643's addition must distinguish the cloud lane from that closed decision rather than silently deleting the comment. Flagging so 648 checks it deliberately.
- **Dependency direction:** this test file includes Cloud, Account, Deck and Settings headers — legal for a TEST (the ACC-§4/648 direction criterion binds the implementation classes, not the test TU). State it so the grep doesn't false-positive.
- **P2-R2/P2-R6 greps** will hit the words "AnonKey"/"refresh-token" in scratch-string literals — every such literal is a visibly-fake `scratch-*` value; zero `eyJ`, zero `service_role`, zero live ref. Run the SC-§14 positive control as always.
- The **exactly-once broadcast** pins (§5.3) and the **guest no-write** pins (§5.4) are the two most likely places a registry-conformant-but-differently-read 644 could collide with this file — rule them from the law, not from whoever compiled first.

## 8) M8 DECLARATION (batch-level, verbatim)

**Adds no replicated property, no new replicated class, no new relevancy tier, no RPC.** All cloud traffic is client-local HTTPS from `USiegeCloudClient` (a `UGameInstanceSubsystem`); nothing crosses the UE networking layer. Does NOT consume the M8 Phase-1 checkpoint gate; does NOT substitute for Jonathan's owed feedback items.

*(This task's file itself performs no cloud traffic at all — zero network is the task's hard law and §4 states the mechanism.)*

---

## 9) LOOP 1 FIX — 2026-08-23 — the TASK-649 C2672 at `JoinedSortedKeys` (qa/TASK-648.md §8)

**The one error:** `SiegeCloudTest.cpp(180,23): error C2672: GenerateKeyArray: no matching overloaded function found` — `FJsonObject::Values` in the installed UE 5.8 is NOT keyed on `FString`, so `GenerateKeyArray(TArray<FString>&)` cannot deduce. Exactly one site (`JoinedSortedKeys`), exactly one file, fixed minimally in place.

### The diff (before → after of the helper body; the doc comment and signature are untouched)

Before:
```cpp
		TArray<FString> Keys;
		if (JsonObject.IsValid())
		{
			JsonObject->Values.GenerateKeyArray(Keys);
		}
		Keys.Sort();
		return FString::Join(Keys, TEXT("|"));
```

After:
```cpp
		TArray<FString> Keys;
		if (JsonObject.IsValid())
		{
			// UE 5.8: FJsonObject::Values is keyed on UE::FSharedString
			// (TSharedString<TCHAR>), not FString (JsonObject.h:99/237/324), so
			// GenerateKeyArray(TArray<FString>&) cannot deduce (TASK-649 C2672).
			// TSharedString::operator*() returns the null-terminated TCHAR*
			// (SharedString.h:79-83) — construct each FString key from it.
			Keys.Reserve(JsonObject->Values.Num());
			for (const auto& Pair : JsonObject->Values)
			{
				Keys.Emplace(*Pair.Key);
			}
		}
		Keys.Sort();
		return FString::Join(Keys, TEXT("|"));
```

### Header evidence (read from the installed engine, `C:\Program Files\Epic Games\UE_5.8`)

- `Runtime/Json/Public/Dom/JsonObject.h` — line 324: `FJsonObject : public UE::JSON::Private::FJsonObjectSharedStringStorage`; line 99 (inside that storage class): `using FStringType = UE::FSharedString;`; line 237: `TMap<FStringType, TSharedPtr<FJsonValue>> Values;`. So the key type is `UE::FSharedString` = `UE::TSharedString<TCHAR>`, exactly as 649's error note names it. (The legacy `FStringType = FString` storage at line 248 is a different, unused-by-`FJsonObject` variant.)
- `Runtime/Core/Public/Containers/SharedString.h` — lines 79–83: `operator*()` returns "a pointer to the start of the referenced null-terminated string" (`const CharType*`, `&NullChar` when empty). That null-terminated `const TCHAR*` feeds `TArray<FString>::Emplace` → the `FString(const TCHAR*)` constructor — a clean, allocation-per-key copy with no reliance on any implicit conversion operator. (I verified `TSharedString` has NO implicit `FString`/`FStringView` conversion — `ToView()` at lines 91–95 is a named method — which is why the explicit per-key construction is the right minimal shape, not a one-liner cast.)

### Behavior preservation

Same observable output byte-for-byte: the same top-level key set is collected (ranged-for over the same map), then the identical `Keys.Sort()` (default `FString` ordering, unchanged) and identical `FString::Join(Keys, TEXT("|"))`. No expected value in tests 4/5 (`DeckRowJsonShape`/`SettingsRowJsonShape`) changed; no assertion touched; suite expectation stays **126/126**.

### Scope confirmation

- Only write this loop: the one `Edit` to `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeCloudTest.cpp`, helper region only. No other file touched; no compile run (TASK-649's re-run owns it); no editor, no git write, no board edit.
- `git diff --stat` note, stated honestly: the file is still **untracked** (`?? Source/.../Tests/SiegeCloudTest.cpp`, HEAD `1025160`) because the whole client lane commits at TASK-650 — so git holds no pre-fix baseline to stat against. The before/after block above IS the diff of record; `git status --short` on the file is unchanged from the original handoff's single-new-file declaration.

---

## LOOP 2 FIX — 2026-08-23 — the ini `//`-comment truncation (qa/TASK-648.md §10)

**Diagnose-first per the dispatch (SC-§20): the production question was answered at the installed engine source BEFORE any edit.**

### STEP-1 evidence table (engine = `C:\Program Files\Epic Games\UE_5.8`)

| # | File:line | Fact established |
|---|---|---|
| 1 | `Runtime/Core/Private/Misc/ConfigCacheIni.cpp:2216-2219` | `FConfigFile::Read(const FString& Filename)` = `::FillFileFromDisk(this, Filename, false)` — the production file-load entry 643's `Initialize()` → `LoadCloudConfigFile` uses |
| 2 | `ConfigCacheIni.cpp:2172-2188` | `FillFileFromDisk` loads the file text, then calls the template `FillFileFromBuffer(File, Text, bHandleSymbolCommands, Filename)` |
| 3 | `ConfigCacheIni.cpp:2201-2204` | `FConfigFile::CombineFromBuffer` = `::FillFileFromBuffer(this, Buffer, bHandleSymbolCommands, FileHint)` — the SAME template. The only parameter difference vs Read is `bHandleSymbolCommands` (true vs false), which governs the `+`/`-`/`.`/`!` prefix commands (:2076-2097), NOT comment handling |
| 4 | `ConfigCacheIni.cpp:1930-1934` | `FillFileFromBuffer` reads every line via `FParse::LineExtended` with flags including `ELineExtendedFlags::SwallowDoubleSlashComments` — set UNCONDITIONALLY for both callers (the only exception is `bPythonConfigParserMode`, which we never set) |
| 5 | `Runtime/Core/Private/Misc/Parse.cpp:1245-1250` | `ParseLineExtended`: `if (!bIsQuoted && (bHandleDoubleSlashComments && Stream[0]=='/' && Stream[1]=='/')) bIgnore = true;` — everything from an unquoted `//` to end-of-line is swallowed. The guard is QUOTE-AWARE: `bIsQuoted` toggles on `"` at :1287, so `//` inside double quotes survives |
| 6 | `ConfigCacheIni.cpp:2148-2155` | Value processing after whitespace trim: `if (*Value=='\"') FParse::QuotedString(Value, ProcessedValue); else ProcessedValue = Value;` — a quoted value is STORED with the quotes stripped |
| 7 | `Parse.cpp:381-421` (`ParseQuotedString`, the body of `FParse::QuotedString` :513) | Requires the opening `"` (:391), appends content up to the closing `"` (stop set `"\n\r\0`, :396-410); escape processing touches only `\` sequences — a URL has none, so it round-trips byte-identically, quotes gone |
| 8 | `ConfigCacheIni.h:815` (from the §4 sweep) + rows 6-7 | `GetString` returns the stored value — i.e. the already-quote-stripped bare URL. Quoting is invisible to `ExtractCloudConfig`; its trim + trailing-`/` normalization applies unchanged |

### The production verdict, stated plainly

**PRODUCTION WAS BROKEN AS SHIPPED.** Rows 1-5: the FILE-load path (`Initialize()` → `LoadCloudConfigFile` → `FConfigFile::Read`) reaches the exact same `SwallowDoubleSlashComments` line reader as the test's `CombineFromBuffer` — the semantic is shared, not buffer-only. The real gitignored `Config/SiegeCloudDev.ini` carried `ProjectUrl=https://cjgqqeogsynrphowdcdp.supabase.co` unquoted, so at every editor/game launch it truncated to `https:`. Worse than clean cloud-OFF: `FSiegeCloudConfig::IsValid()` is a non-empty conjunction and `https:` is non-empty (the AnonKey is base64url — it cannot contain `/` and parses fine), so the client came up `IsCloudConfigured()==true`, logged `Cloud configured: https:.`, and every request would have targeted `https:/auth/v1/…` / `https:/rest/v1/…` — unreachable garbage URLs. The failure mode was per-request honest errors, NOT the ACC-§11 clean OFF degrade. No live sitting ever exercised it (TASK-650's smoke is still pending), so nothing user-visible occurred yet — the seam test caught exactly the class it exists for.

**Fix sized to the diagnosis:** rows 5-7 prove double-quoting the value alone is sufficient — the quoted `//` survives the swallow, quotes are stripped on read, the client receives the bare URL. **Therefore ZERO `SiegeCloudClient.{h,cpp}` edits** (643's reviewed file stands untouched).

### Diffs (every one; the real ini's AnonKey line is referenced by name, never printed — P2-R2)

**1. `Config/SiegeCloudDev.ini` (REAL, gitignored — DECLARED dev-local edit, custody per handoffs/TASK-641-buildmaster.md; edited in place, AnonKey line and all custody comments untouched):**
```diff
 [SiegeCloud]
+; ProjectUrl MUST stay double-quoted: UE's ini line reader swallows an
+;   unquoted // as an inline comment (the value truncates to "https:") -
+;   TASK-649 loop-2 finding, pinned by Siegebound.Cloud.ConfigIniParseSeam.
+;   Quotes are stripped on read; the client receives the bare URL.
-ProjectUrl=https://cjgqqeogsynrphowdcdp.supabase.co
+ProjectUrl="https://cjgqqeogsynrphowdcdp.supabase.co"
```

**2. `Config/SiegeCloudDev.ini.example` (committed template, same shape):**
```diff
 [SiegeCloud]
+; ProjectUrl MUST stay double-quoted: UE's ini line reader swallows an
+;   unquoted // as an inline comment (the value truncates to "https:") —
+;   TASK-649 loop-2 finding, pinned by Siegebound.Cloud.ConfigIniParseSeam.
+;   Quotes are stripped on read; the client receives the bare URL.
-ProjectUrl=https://<project-ref>.supabase.co
+ProjectUrl="https://<project-ref>.supabase.co"
```

**3. `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeCloudTest.cpp` — `ConfigIniParseSeam` now exercises the SHIPPING (quoted) format and pins the hazard:**

- Doc comment (after "…owned by TASK-650/651."): appended the ⭐ LOOP-2 FINDING paragraph carrying the full engine citation chain (rows 1-7 above) and the shipping-format rule.
- The main scratch buffer, before → after:
```
-		TEXT("ProjectUrl=https://scratchref.supabase.co\n")
+		TEXT("ProjectUrl=\"https://scratchref.supabase.co\"\n")
```
  (buffer lead-in comment extended with the ⛔ quoted-shipping-format note)
- The ProjectUrl equality assertion message, before → after (expected value UNCHANGED — `https://scratchref.supabase.co`; it now proves quote-strip-on-read):
```
-	TestEqualSensitive(TEXT("The parsed ProjectUrl is byte-identical to the buffer's value"),
+	TestEqualSensitive(TEXT("The QUOTED ProjectUrl reads back as the bare URL — quotes stripped, // intact (the shipping-format proof)"),
```
- NEW hazard-pin block (inserted between the valid-composition check and the incomplete-buffer section): a fresh `FConfigFile UnquotedFile` combines `[SiegeCloud]` + unquoted `ProjectUrl=https://scratchref.supabase.co` + the scratch AnonKey line and asserts (a) `GetString` still SUCCEEDS (the failure is silent, not a parse error), (b) the value is exactly `https:` — the documented engine truncation, byte-compared — and (c) the truncated config still composes `IsValid()==true` — the silent-break mechanism the quoting rule exists to prevent. Three assertions; nobody can un-quote the ini without this test failing loudly.
- The incomplete-buffer section's ProjectUrl line also quoted (shipping-format consistency; its assertions are unchanged):
```
-		TEXT("ProjectUrl=https://scratchref.supabase.co\n");
+		TEXT("ProjectUrl=\"https://scratchref.supabase.co\"\n");
```

Assertion-count delta: +3 (the hazard pin). Test count unchanged: 8 `Siegebound.Cloud.*`; suite expectation for the TASK-649 loop-2 re-run stays **126/126**.

### 📋 ACC-§11 FORMAT-AMENDMENT FLAG — for the manager (I do not write law)

CONVENTIONS `ACC-§11` (line ~4008, "THE CONFIG HOME") pins the keys as `ProjectUrl=` / `AnonKey=` without a quoting rule. It needs a dated amendment recording: **the `ProjectUrl` value MUST be double-quoted** (UE's ini reader treats an unquoted `//` as an inline comment-start — `FParse::ELineExtendedFlags::SwallowDoubleSlashComments`, shared by the file-load and buffer parse paths; measured TASK-649 loop 2, engine citations above), quotes are stripped on read, and `AnonKey` stays unquoted (base64url — cannot contain `/`; quoting it is harmless but not required). Both shipped ini files and the seam test now carry/pin this format.

### Fences held this loop

Files touched = exactly `Tests/SiegeCloudTest.cpp` + `Config/SiegeCloudDev.ini.example` + the real `Config/SiegeCloudDev.ini` (declared above) — `SiegeCloudClient.{h,cpp}` deliberately UNTOUCHED (the evidence made quoting sufficient). No new file. No compile (the 649 loop-3 re-run owns it), no editor/MCP, no git write, no TASKBOARD edit. No secret material printed here or anywhere: the anon key line was not modified and is referenced only by key name.
