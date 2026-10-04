<!-- MOVED from .claude/pipeline/CONVENTIONS.md on 2026-10-04 by Tools/split_conventions.py. Sections are byte-identical to the original; this comment is the only addition. Cite clauses by tag (e.g. `VER-§3 cl. 6`); CONVENTIONS.md '## Law index' maps tags to files. -->
## ⚖️ ACCOUNTS — local-first player accounts · profile-scoped saves · the cloud-backend ruling (2026-08-16) — namespace **`ACC-§N`**

Added 2026-08-16 (batch ACCOUNTS, TASK-599..610). **Jonathan's request, verbatim:** *"I want to add the ability to create an account that saves data. This can be a "login" button in the main menu that leads to a "create account" button and a "log into existing account" button. Eventually I want to make the accounts created saved in the cloud so that way people can log in from any device. These accounts will be able to track different deck builds that the user has created and saved, any setting adjustments, etc. Come up with a plan for how we can save that data in the cloud. If we need to use a website I would prefer to use github pages since I already have a github account with a website using github pages. If there is another method that you think is easier to manage and store this data in the could let me know."*

**The shape: THREE PHASES.** Phase 1 (this batch, fully decomposed) = a **LOCAL-FIRST account shell, backend-agnostic** — the UI he described, a profile model, and per-profile persistence of decks + settings layered on the EXISTING `USaveGame` systems. Phase 2 (gated one-liners, ⛔ no IDs) = cloud sync, pending his backend ruling. Phase 3 = cross-device polish one-liners. ⭐ **Phase 1 works fully offline and Phase 2 bolts on without UI rework — that is the design constraint, not an accident.**

### ACC-§0 ⛔ THE GITHUB-PAGES RULING — RULED OUT AS THE DATA STORE, ON THE RECORD, WITH THE MECHANISM

- ⛔ **GitHub Pages CANNOT be the accounts backend.** It is **static file hosting**: no server code, no database, no secure per-user auth. The only way a game client could WRITE to it is a GitHub API token **shipped inside the game binary** — which any player can extract and use to read/write EVERY account. That is not a hardening problem; it is structural. ⛔ **No task may propose a GH-Pages/gist/repo-commit data store; this ruling is cited, not re-argued.**
- ✅ **GitHub Pages MAY host a companion WEBSITE later** (landing page, account-portal front-end that talks to the real backend over HTTPS). Site ≠ store.
- **DEFAULT BACKEND FOR PHASE 2: Supabase** — hosted Postgres + built-in email/password auth (JWTs) + row-level security (each account reaches only its own rows) + a plain HTTPS/JSON REST API that UE's built-in `FHttpModule` + `Json` modules call with **no third-party SDK**. Free tier ample at this scale, and Jonathan's Claude environment already has a **Supabase connector wired**, so agents can create/manage the project, tables and RLS policies directly when Phase 2 opens. **Alternatives on record:** Firebase, PlayFab (both viable, more SDK friction), EOS (Epic-native, but its auth model fits Epic accounts/device IDs, not simple email/password). ⚠️ **FLAGGED A1 — the backend choice is JONATHAN'S, owed before Phase 2 decomposes. Nothing in Phase 1 depends on it.**
- ⚠️ **Environmental caveat, recorded now:** Norton MITMs HTTPS on this machine (the HF precedent) — `*.supabase.co` will likely need a Norton exclusion when Phase 2 goes live. **A dev-machine artifact, not a product defect, and a Jonathan hand-step.**
- ⭐ **DATED AMENDMENT 2026-08-23 — A1 IS RULED: SUPABASE (Jonathan, verbatim in ACC-§10).** The flagged default stood and the flag is discharged. Phase-2 law = **ACC-§10..§15** below; the GH-Pages ruling above is UNCHANGED.

### ACC-§1 ⛔ THE GUEST-DEFAULT LAW — LOGIN GATES NOTHING, AND THE ACCOUNTLESS GAME IS BYTE-IDENTICAL

- **Guest is the default.** With no account created and no login performed, every shipped flow behaves EXACTLY as today: decks read/write slot `"SiegeDecks"`, settings read/write slot `"SiegeSettings"`, all menus work. ⛔ **No task may gate Play, Sandbox, Deck Builder, Multiplayer or Settings behind login.**
- **Login surface = the MAIN MENU ONLY in Phase 1.** No in-match login/logout, no identity change during a live match (it would swap save slots mid-session and raise M8 questions nobody has answered). The subsystem crosses levels, so an in-match entry later is one widget task.
- **The guest slots are never mutated by account code.** Creating a profile SEED-COPIES the guest data into the new profile's slots once (ACC-§3; FLAGGED A6, default TAKEN); the guest originals stay untouched forever.

### ACC-§2 ⛔ THE HONEST-CREDENTIAL LAW — PHASE 1 CREDENTIALS ARE A CONVENIENCE, ⛔ NOT SECURITY, AND NO ARTIFACT MAY SAY OTHERWISE

- Phase 1 stores a **salted hash** locally: `CredentialHashHex = hex(FSHA1::HashBuffer(UTF8(CredentialSaltHex + ":" + Password)))`, salt = a fresh `FGuid` in `Digits` hex per profile. This stops shoulder-surfing and accidental plaintext on disk. **It is NOT a security boundary: anyone with disk access can edit `SiegeAccounts.sav`.**
- ⛔ **No task, handoff, QA report, or code comment may describe Phase-1 auth as "secure," "encrypted," or "protected."** The honest phrase is **"local convenience credential — real auth is the Phase-2 backend's job"** (Supabase GoTrue does server-side bcrypt; the client never stores a password).
- ⛔ **The plaintext password is NEVER persisted and NEVER logged** — not in the save game, not on `LogSiegeAccount`, not in a handoff. ✅ **QA CRITERION: grep the diff for any write/log of the password parameter and report ZERO.**

### ACC-§3 THE PROFILE MODEL + THE SLOT-NAME LAW

| Thing | Exact value |
|---|---|
| Profile identity | `FGuid ProfileId` — never the display name |
| Slot suffix derivation | `ProfileId.ToString(EGuidFormats::Digits)` — 32 hex chars, filename-safe; helper `USiegeAccountSubsystem::MakeProfileSlotSuffix` is the ONE implementation |
| Account registry slot | **`"SiegeAccounts"`**, user index **0** ⇒ `Saved/SaveGames/SiegeAccounts.sav` |
| Profile deck slot | **`"SiegeDecks_<Digits>"`** (guest: bare `"SiegeDecks"` — unchanged) |
| Profile settings slot | **`"SiegeSettings_<Digits>"`** (guest: bare `"SiegeSettings"` — unchanged) |
| Display name rules | trimmed; 3–24 chars; unique case-insensitive among local profiles; login lookup case-insensitive |
| Password rule (P1) | ≥ 4 chars, non-empty — a convenience bar, not a policy (ACC-§2) |

- ⛔ **A slot name is NEVER derived from the display name** (users type anything; filenames + collisions + renames).
- **Seed-copy on create (FLAGGED A6, default TAKEN):** `CreateAccount` — after registering the profile and making it active — copies the existing guest `"SiegeDecks"` / `"SiegeSettings"` payloads into the new profile's slots IF the guest slots exist (load → re-save under the profile slot). Once, at create only, never at login. His existing decks follow him into his first account; guest slots untouched.
- **Existing save CLASSES are NOT rewritten.** `USiegeDeckSaveGame` and `USiegeSettingsSaveGame` keep their exact fields; profile scoping is 100 % slot-name scoping (ACC-§4).

### ACC-§4 ⛔ THE SEAM LAW — EVERY SLOT NAME RESOLVES THROUGH `USiegeAccountSubsystem`, AND THE CALL-SITE SET IS ENUMERATED, NOT ASSUMED

**Verified first-hand at decomposition (manager, raw grep + read):** the deck slot has ONE definition (`SiegeDeckSaveGame.cpp:7`, `const FString USiegeDeckSaveGame::SlotName = TEXT("SiegeDecks")`) and exactly **FIVE consumer sites in TWO files** — `DeckBuilderWidget.cpp:589` (SaveDeckAs) · `:677` (SetActiveDeck) · `:744` (DoesSaveGameExist) · `:752` (LoadGameFromSlot) · `SiegePlayerController.cpp:253` (match-side active-deck load). The settings slot already funnels through ONE choke point: `USiegeSettingsSubsystem::ResolveSlotName()` (`SiegeSettingsSubsystem.cpp:177` — `SlotNameOverride.IsEmpty() ? SettingsSlotName : SlotNameOverride`), with a shipped test seam `SetSlotNameForAutomationTests` (`SiegeSettingsSubsystem.h:175`).

- **The seam:** `USiegeAccountSubsystem::GetDeckSlotName()` / `GetSettingsSlotName()` return the profile-scoped slot when a profile is active, the bare guest constant otherwise. **Fail-safe: an unresolvable account subsystem ⇒ the guest constant — today's behavior, never a crash** (the `USiegeDeckSaveGame` null-safety contract).
- **Deck lane (the five sites):** each resolves the slot name **at call time** through the seam. No cached slot, no reload machinery — `GameInstance` outlives `OpenLevel`, so a menu login is naturally live at `SiegePlayerController.cpp:253` when the match loads. `USiegeDeckSaveGame::SlotName` STAYS as the guest constant, byte-identical.
- **Settings lane:** `ResolveSlotName()` precedence becomes **`SlotNameOverride` (tests, unchanged) > account-profile slot > `SettingsSlotName`**. `USiegeSettingsSubsystem::Initialize` gains `Collection.InitializeDependency<USiegeAccountSubsystem>()` (the engine's sanctioned subsystem-ordering route) and subscribes to `OnActiveProfileChanged` → `ReloadForActiveProfile()` (reload via `ResolveSlotName()`, C++ defaults on missing slot, then broadcast the existing `OnSettingsChanged` so `USettingsMenuWidget` refreshes for free). ⚠️ **Dependency direction: Settings depends on Account. `USiegeAccountSubsystem` never includes a Settings header.**
- ⛔ **A literal `"SiegeDecks"` / `"SiegeSettings"` anywhere outside the two guest-constant definition sites and the tests is a QA FAIL.** ✅ **The shipped invariants SURVIVE UNTOUCHED:** `SiegeSettingsTest.cpp:120-121` asserts `SettingsSlotName == "SiegeSettings"` — true before and after, because the guest constant does not move.

### ACC-§5 🔨 THE UI RULING — `UAccountMenuWidget` IS A CODE-AUTHORED TREE. A **NEW, NARROW** EXCEPTION ARGUED ON ITS OWN FACTS — ⛔ NOT AN INHERITANCE FROM THE SETTINGS RULING

⚠️ The settings-lane ruling (§3 of "Settings screen + …", 2026-08-03) is scoped to `USettingsMenuWidget` ONLY and says citing it for another widget is misuse. **So this is a fresh ruling, same facts, equally narrow:** a forms panel (text boxes + buttons); the duplicate+reparent `WBP_` route is this project's most expensive UI failure mode (~9 wasted fixes, runtime repaint silently broken); the fresh-hand-authored route is a Jonathan hand-step not owed for a form. `UAccountMenuWidget` builds its tree in `RebuildWidget()` via `WidgetTree->ConstructWidget<>` and ships with **no `.uasset`**. The five conditions are cloned as QA criteria:
- **(a) SCOPE:** `UAccountMenuWidget` ONLY. Citing this clause for a fourth widget is the same misuse.
- **(b) THE ORDER (the corrected law, already paid for once):** build the tree and set `WidgetTree->RootWidget` **FIRST**, then `return Super::RebuildWidget();` — the TASK-444 shipped shape. ⛔ Anything constructed after Super is discarded (`UserWidget.cpp:1214`) and the widget renders empty **while passing every property readback.** Every child is `UPROPERTY(meta=(BindWidgetOptional))`; construct only if still null.
- **(c) RESERVED NAME:** `/Game/UI/WBP_AccountMenu` may be used by nothing else (✅ verified absent at decomposition). A later authored WBP wins automatically with zero C++ change.
- **(d) CONTRACT:** `BindWidgetOptional` members · `BlueprintCallable` wrappers · FString/int32/bool/uint8-only BIEs — the shipped `USessionMenuWidget`/`USettingsMenuWidget` contract.
- **(e) VERIFICATION IS A HUMAN PIXEL CHECK.** No `.uasset` to read back; MCP readback has repeatedly passed on visually-broken UMG here. Rendering correctness closes on **Jonathan's pixels** (TASK-609), never tree/property readback.

**Pinned child names** (C++ and any future WBP both read this line): `RootPanel` (`UVerticalBox`) · `BackdropBorder` (`UBorder`) · `TitleText` · `StatusText` · `CreateAccountButton` + `CreateAccountLabelText` · `LoginExistingButton` + `LoginExistingLabelText` · `NameInputBox` (`UEditableTextBox`) · `PasswordInputBox` (`UEditableTextBox`, `SetIsPassword(true)`) · `ConfirmPasswordInputBox` (`UEditableTextBox`, `SetIsPassword(true)`) · `SubmitButton` + `SubmitLabelText` · `LogoutButton` + `LogoutLabelText` · `BackButton` + `BackLabelText`.

- ⚠️ **`BackdropBorder` is HIT-TEST **VISIBLE** — a correctness requirement, not styling.** The panel overlays `WBP_MainMenu`; an invisible plate lets clicks fall through to Play/Quit while the panel looks modal (the settings lane's click-through-into-Quit lesson, same geometry).
- **Modes (internal `enum class EAccountMenuMode : uint8` — C++-only, never a BIE param):** `Chooser` (the two buttons Jonathan named: **Create Account** / **Log into existing account**) → `CreateForm` (name + password + confirm) or `LoginForm` (name + password) → `LoggedIn` (StatusText shows `Logged in as <DisplayName>` + Logout + Back). Opening while logged in lands on `LoggedIn`. Submit failures render `OutReason` in `StatusText` — never a crash, never a silent no-op.
- **Navigation = the settings §4 overlay law, cloned:** `Btn_Login` on `WBP_MainMenu` does `CreateWidget(UAccountMenuWidget)` → `AddToViewport(ZOrder 10)`; it does NOT remove the main menu; `Back` removes only itself. **Main-menu order becomes: Play (vs Bot) → Sandbox (No Bot) → Deck Builder → Multiplayer → Settings → Login → Quit** — spliced before the Quit block, shipped idiom character-for-character (font 28, `MakeMargin(24,12,24,12)`, `HAlign_Fill`, granular ops never `write_graph_dsl`, `add_event`-first), pre-existing entries proven character-identical by full-graph DSL diff.

### ACC-§6 NAMING + FOLDER LAW (the cross-task contract)

| Thing | Exact name | Location |
|---|---|---|
| Profile struct | `FSiegeProfileInfo` (USTRUCT, BlueprintType) | in `SiegeAccountSaveGame.h` |
| Account registry | `USiegeAccountSaveGame` (`USaveGame`) — statics `SlotName = "SiegeAccounts"`, `UserIndex = 0` | game — `SiegeAccountSaveGame.h/.cpp` (new) |
| Account owner | `USiegeAccountSubsystem` (`UGameInstanceSubsystem`) | game — `SiegeAccountSubsystem.h/.cpp` (new) |
| Log category | **`LogSiegeAccount`** | declared/defined in `SiegeAccountSubsystem.h/.cpp` |
| Profile-changed delegate | `FOnSiegeActiveProfileChanged` (dynamic multicast, no params) — member `OnActiveProfileChanged` | on `USiegeAccountSubsystem` |
| Account widget C++ base | `UAccountMenuWidget` (`UUserWidget`) | game — `AccountMenuWidget.h/.cpp` (new) |
| Reserved widget asset name | `WBP_AccountMenu` | `/Game/UI/WBP_AccountMenu` — **RESERVED, not authored in P1** |
| Main-menu entry button | `Btn_Login`, label text `"Login"` | additive on `/Game/UI/WBP_MainMenu` |
| Tests | `SiegeAccountTest.cpp` | game — `Source/GitClaudeUnrealTest/Siegebound/Tests/` (new) |
| Phase-2 reserved names | `USiegeCloudClient` (`SiegeCloudClient.h/.cpp`) · tables `profiles` / `decks` / `settings` (jsonb payloads) | ⛔ **NOT authored in P1 — reserved only** |

- ⭐ **DATED NOTE 2026-08-23:** the Phase-2 reserved row above is now **LIVE** — the full P2 naming table is **ACC-§14** and the reserved names are used exactly as reserved.

### ACC-§7 📌 PINNED CROSS-TASK SIGNATURE REGISTRY — ⛔ every parallel task compiles against THIS, character-for-character

```cpp
// ── SiegeAccountSaveGame.h (TASK-599) ─────────────────────────────────────
USTRUCT(BlueprintType) struct FSiegeProfileInfo {
  GENERATED_BODY()
  UPROPERTY(SaveGame, BlueprintReadOnly) FGuid     ProfileId;
  UPROPERTY(SaveGame, BlueprintReadOnly) FString   DisplayName;
  UPROPERTY(SaveGame)                    FString   CredentialSaltHex;  // ACC-§2: convenience, NOT security
  UPROPERTY(SaveGame)                    FString   CredentialHashHex;  // hex(SHA1(Salt + ":" + Password))
  UPROPERTY(SaveGame, BlueprintReadOnly) FDateTime CreatedUtc;
  UPROPERTY(SaveGame, BlueprintReadOnly) FDateTime LastLoginUtc;
};
UCLASS() class GITCLAUDEUNREALTEST_API USiegeAccountSaveGame : public USaveGame {
  GENERATED_BODY()
public:
  static const FString SlotName;   // TEXT("SiegeAccounts")  — the SiegeDeckSaveGame idiom
  static const int32   UserIndex;  // 0
  UPROPERTY(SaveGame) TArray<FSiegeProfileInfo> Profiles;
  UPROPERTY(SaveGame) FGuid ActiveProfileId;   // invalid GUID = guest
};

// ── SiegeAccountSubsystem.h (TASK-600) ────────────────────────────────────
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSiegeActiveProfileChanged);
UCLASS() class GITCLAUDEUNREALTEST_API USiegeAccountSubsystem : public UGameInstanceSubsystem {
  GENERATED_BODY()
public:
  virtual void Initialize(FSubsystemCollectionBase& Collection) override;
  UFUNCTION(BlueprintCallable) bool CreateAccount(const FString& DisplayName, const FString& Password, FString& OutReason);
  UFUNCTION(BlueprintCallable) bool Login(const FString& DisplayName, const FString& Password, FString& OutReason);
  UFUNCTION(BlueprintCallable) void Logout();
  UFUNCTION(BlueprintPure) bool    IsLoggedIn() const;
  UFUNCTION(BlueprintPure) FString GetActiveDisplayName() const;  // empty when guest
  FGuid GetActiveProfileId() const;                               // invalid when guest
  UFUNCTION(BlueprintPure) FString GetDeckSlotName() const;       // guest => USiegeDeckSaveGame::SlotName
  UFUNCTION(BlueprintPure) FString GetSettingsSlotName() const;   // guest => USiegeSettingsSubsystem::SettingsSlotName
  UPROPERTY(BlueprintAssignable) FOnSiegeActiveProfileChanged OnActiveProfileChanged;
  static FString MakeCredentialHashHex(const FString& Password, const FString& SaltHex);
  static FString MakeProfileSlotSuffix(const FGuid& ProfileId);   // ProfileId.ToString(EGuidFormats::Digits)
  void SetSlotNameForAutomationTests(const FString& InSlotName);  // the USiegeSettingsSubsystem seam, cloned
};

// ── SiegeSettingsSubsystem.h ADDITION (TASK-601; everything existing is untouched) ──
void ReloadForActiveProfile();  // reload via ResolveSlotName(); broadcasts OnSettingsChanged

// ── AccountMenuWidget.h (TASK-603) ────────────────────────────────────────
UCLASS() class GITCLAUDEUNREALTEST_API UAccountMenuWidget : public UUserWidget { /* ACC-§5 pinned children + modes */ };
```

`Login`/`Logout`/`CreateAccount` update `ActiveProfileId` + `LastLoginUtc`, save the registry slot, then broadcast `OnActiveProfileChanged`. **Load once, save on change, never read the disk from a gameplay path** (the settings-lane contract, cloned).

### ACC-§8 ⛔ PHASE 2/3 ARE RESERVED, NOT AUTHORED — AND THE FLAGGED DEFAULTS TRAVEL WITH THEM

⛔ **No cloud code, no HTTP call, no credential upload ships in Phase 1.** Reserved now so P1 code never squats on the names: `USiegeCloudClient` (UE `FHttpModule` + `Json`, no third-party SDK), Supabase tables `profiles` / `decks` / `settings` with jsonb payloads + per-user RLS, JWT auth via Supabase email/password. **Defaults awaiting Jonathan (recorded, not decided):** sync = last-write-wins on `updated_at` (A3) · first cloud login uploads the local profile (A4) · email confirmation OFF at signup (A5) · the P2 login identity becomes an EMAIL (Supabase-native) with `DisplayName` staying the in-game handle (A2). Norton `*.supabase.co` exclusion = Jonathan hand-step (ACC-§0).

- ⭐ **DATED AMENDMENT 2026-08-23 — PHASE 2 IS NOW AUTHORED (TASK-639..652).** The reservation above is superseded FOR PHASE 2 by **ACC-§10..§15**; **Phase 3 stays reserved** (cross-device polish, password recovery, field-level merge/tombstones, per-save auto-push). The A2..A7 defaults travel unchanged into the P2 law: **A2** email login (ACC-§10) · **A3** last-write-wins on server `updated_at`, landed in the honest P2 shape ACC-§13 states · **A4** first-link upload (ACC-§13) · **A5** confirmation OFF, now live config law (ACC-§10) · **A6** spent in P1 · **A7** still NOT taken.

- ⭐ **DATED ADDITION 2026-08-23 — two Phase-3 candidates carried from `qa/TASK-640.md` (recorded here where the Phase-3 one-liners live; ⛔ no tasks, no IDs until Phase 3 is decomposed):** **(a)** `USiegeDeckSaveGame::ActiveDeckName` has no cloud representation — a law-conformant refusal by TASK-639 (the ACC-§12 fence forbids improvised columns; per-device active-deck is the P2 default); candidate column for Phase 3. **(b)** `display_name` and the two `payload jsonb` columns carry no server-side size bounds (P1's 3–24-char check is client-side only); RLS confines abuse to the offender's own rows — `check`-constraint hardening is a Phase-3 candidate, never an in-flight edit.

### ACC-§9 📌 M8 DECLARATION + KNOWN LIMITATIONS — ⛔ none of these is a bug

- **M8:** adds **no replicated property, no new replicated class, no new relevancy tier, no RPC.** All account state is CLIENT-LOCAL (`UGameInstanceSubsystem` + local `USaveGame`). ⛔ **The display name is NOT pushed into any session/player name in P1** (FLAGGED A7 — a later one-liner if Jonathan wants it).
- **Known limitations, stated up front:** P1 credentials are not security (ACC-§2) · profiles are per-machine until Phase 2 · no password recovery in P1 (a forgotten local password = create a new profile; recovery is a P3 email flow) · no mid-match login (ACC-§1) · the account panel's look is unstyled C++-default until a polish pass.

### ACC-§10 ⚖️ A1 IS RULED — SUPABASE — AND THE PROJECT LAW (added 2026-08-23, Phase-2 decomposition)

**Jonathan's ruling, verbatim (2026-08-23, direct in Claude Code):** *"I am officially green lighting the continuation on to phase 2 for the account storage system. Supabase is good, go ahead and continue on the next steps to set everything up using Supabase and let me know what you need from me. Run this in parallel with your current tasks if you can."*

- **The backend is Supabase.** A1 is discharged; the ACC-§0 GH-Pages ruling stands untouched.
- **THE PROJECT LAW (FLAGGED S1 — default proceeds):** Phase 2 runs on a **NEW dedicated project named `siegebound`** on org `JonathanWesely's Org` (id `pbgbbgneacgeznvgaiid`), region **us-west-2**, **free tier — $0/month verified live at decomposition**. ⛔ **The existing paused project ("JonathanWesely's Project", ref `sebxviekcrnnhifdmfdl`, INACTIVE since ~2026-06-30, provenance unknown) is NEVER touched** — it may be Jonathan's personal sandbox, and restoring it would couple the game to an asset nobody here understands. A dedicated, game-named project is the clean ownership story. Overrule = one word from Jonathan (S1).
  - ⭐ **DATED CORRECTION 2026-08-23 (TASK-641 deviation 1):** the live project `siegebound` (ref `cjgqqeogsynrphowdcdp`) is in **`us-west-1`**, not the `us-west-2` written above — the MCP `create_project` region enum offers no `us-west-2`; same US-West coast, same $0. Jonathan's overrule window was surfaced (delete+recreate cheap before TASK-642's apply) and the default proceeded. **`us-west-1` is the region of record.** Cite: `handoffs/TASK-641-buildmaster.md` §5.1.
- **THE ACCESS SURFACE:** agents reach Supabase ONLY through the **orchestrator-held MCP tools** under Jonathan's account — the manager and the specialist agent definitions hold NO standalone Supabase credentials. Every provisioning/schema task is dispatched WITH that surface and its handoff SAYS SO (the RELAYED-instrument model: name the tool that produced each figure).
- **Auth config law:** email/password provider ON; **email confirmation OFF at signup** (A5 default, now live config — FLAGGED S2 to flip); no other providers in P2. Any auth setting the MCP surface cannot reach is reported as a Jonathan hand-step with the exact dashboard clicks — never silently skipped.
- **Norton re-pin (ACC-§0 caveat, now imminent):** `*.supabase.co` exclusion is a Jonathan hand-step BEFORE client-side HTTPS testing (S3). A TLS/cert failure from this machine is the recorded environmental issue FIRST, a code bug second.

### ACC-§11 ⛔ THE KEY LAW + THE CLIENT TRUST MODEL + THE CONFIG HOME (2026-08-23)

- ⛔ **THE `service_role` KEY NEVER TOUCHES THE REPO, THE GAME BINARY, ANY COMMITTED FILE, ANY FILE THE GAME READS, OR ANY HANDOFF/QA/BOARD TEXT.** It exists only inside Supabase's own dashboard and the orchestrator-held MCP surface. ✅ **QA CRITERION (every P2 diff): grep for `service_role` and for secret-key material (`sb_secret`, JWTs beginning `eyJ` other than the anon key's ruled home) — ZERO hits.**
- **The game client = publishable/anon key + user JWTs + RLS, nothing else.** Every row it can reach is bounded by an `auth.uid()` policy (ACC-§12). The anon key is **public-by-design** (RLS is the security boundary, not key secrecy) — ⛔ but it still has exactly ONE ruled home (below); a hardcoded anon key in a `.cpp` is a QA FAIL.
- **THE CONFIG HOME:** gitignored **`Config/SiegeCloudDev.ini`** — section `[SiegeCloud]`, keys `ProjectUrl=` / `AnonKey=` ~~(+ an optional `; DbPassword=` custody comment line the game NEVER reads)~~ ⛔ **RETIRED 2026-09-13 (R15 — the contents rule below: the two keys and NOTHING ELSE)**. Committed template **`Config/SiegeCloudDev.ini.example`** with placeholders; `.gitignore` gains the exact line `Config/SiegeCloudDev.ini`. **Missing/unparsable config ⇒ cloud OFF and the game behaves byte-identically to Phase 1** — this extends ACC-§1: cloud gates NOTHING, ever.
  - ⭐ **DATED AMENDMENT 2026-08-23 — THE CONFIG FORMAT LAW (TASK-649 loop-2 finding; the `ProjectUrl=`/`AnonKey=` key pin above stands — this rider governs the VALUE format):** **`ProjectUrl` MUST be double-quoted** in `Config/SiegeCloudDev.ini` AND the committed `.example` (`ProjectUrl="https://…"`). UE's ini parser swallows an unquoted `//` as an inline comment on BOTH parse paths — file load (`FConfigFile::Read`) and buffer combine (`CombineFromBuffer`) feed the same line reader with `SwallowDoubleSlashComments` set unconditionally; engine citations in the STEP-1 evidence table of `handoffs/TASK-647-programmer.md` "LOOP 2 FIX" — truncating `https://…` to `https:` while `FSiegeCloudConfig::IsValid()` (a non-empty conjunction) STILL PASSES. That is a **silent cloud-misconfiguration** (`IsCloudConfigured()==true`, every request targeting garbage `https:/…` URLs) — strictly worse than the clean cloud-OFF degrade this section mandates. Quotes are stripped on read (`FParse::QuotedString`), so the quoted value round-trips byte-identically to the client — quoting costs nothing. **`AnonKey` stays UNQUOTED**: the base64url alphabet cannot contain `/` (quoting it would be harmless, but it is not the law). ⛔ The suite's `Siegebound.Cloud.ConfigIniParseSeam` hazard-pin now ENFORCES this format mechanically — it asserts the unquoted form truncates to exactly `https:` and still composes `IsValid()==true` — so no future task may "simplify" the quotes away without that test failing loudly; treat any such failure as this law firing, not a test bug. 📜 **THE RECORD:** production shipped with the unquoted form for ~1 session (every launch came up `Cloud configured: https:.`) and was caught by the OFFLINE seam test BEFORE any live smoke ran (TASK-650 still pending — zero user-visible impact). The seam test caught exactly the failure class it exists for.
  - ⭐ **DATED AMENDMENT 2026-09-13 — THE CONTENTS RULE (R15, from `TASK-1259` — 🧑 MEASURED: an in-editor assistant read the ~~excluded~~ listed-for-exclusion `Config/SiegeCloudDev.ini` on demand in one tool call, `INDEX_IGNORE` notwithstanding — "excluded" struck 2026-09-14: the exclusion is UNPROVEN, see the INDEX-VISIBLE amendment below):** **the config home holds `ProjectUrl` and `AnonKey` and ⛔ NOTHING ELSE.** The `(+ an optional `; DbPassword=` custody comment line the game NEVER reads)` allowance in the bullet above is **RETIRED** — ~~a comment line the game never reads~~ is still readable TEXT to any assistant with a file tool on this machine, and under a training-ON toggle it may enter a chat turn. The file is bounded by LAW, not hidden: it ships inside every pak by design (`PKG-§12`) and it is reachable by Aura's tools regardless of the ignore list (`Docs/AuraIndexIgnore.txt` is MEANT to shape the INDEX and nothing else — `TASK-1267` says so in the file's own comment; whether it shaped it for THIS file is ⛔ UNRESOLVED and ⛔ no longer worth spending on: ~~measured 2026-09-14 and the answer is no~~ ⛔ RETIRED 2026-09-17 — the 2026-09-17 measurement (R17, the last amendment in this section) found the file was ⛔ never in the index's SCOPE at all, so the question does not have the answer the 2026-09-14 reading gave it). What may be in the file: the `[SiegeCloud]` section header, `ProjectUrl="…"` (quoted, per the 2026-08-23 rider), `AnonKey=…`, and comment lines that name NO credential of any kind — no password, no second key, no dashboard URL with an embedded token, no `; DbPassword=` even empty (a template for a value is an invitation to fill it). ✅ **QA CRITERION (every diff touching `Config/SiegeCloudDev.ini.example`, and `TASK-1266`'s standing census on the real file): `rg --no-ignore -c '^\s*;?\s*DbPassword='` = 0 on BOTH files.** `PKG-§12`'s measurement (2026-09-09: exactly two keys, no password) is the baseline this rule freezes; `PKG-§12`'s void conditions are unchanged. 📜 The record: the allowance was never exercised — the real file carried no password when measured — and it is retired before it could be.
  - ⭐ **DATED AMENDMENT 2026-09-14 — THE STANDING CENSUS (bought by `TASK-1266`, `handoffs/TASK-1266-buildmaster.md`; boarded as `TASK-1278`, `SC-§100`; cited, not restated):** the census the QA criterion above names is a STANDING instrument, and its first full run bought four corrections. **(i) PATTERN (9) JOINS THE EIGHT** of the `TASK-1266` row: `(?i)"(password|passwd|secret|token)"\s*:\s*"[^"]{8,}"` — a JSON-quoted key. Pattern (8) cannot see it because the quote sits between the word and the colon (handoff §5 item 3); the first `ENGINE` hit below was found ONLY by this probe (§2.8c). **(ii) TWO CLASSES JOIN `VALUE` / `PLACEHOLDER` / `PROSE` / `TEST`:** **`NOISE`** — base64 image dumps and PEM certificate bodies where `eyJ` occurs by chance (266 single-line PNG captures under `Saved/` + three CA bundles, handoff §2.1 + Appendix A; the strict three-segment JWT probe reads 0 in every one of them) · **`ENGINE`** — engine-generated or Epic-shipped, machine-local, gitignored (`Saved/`, `.gitignore:111`), never shipped. The two KNOWN `ENGINE` members, by path (⛔ paths only, never a value — the `TASK-1266` row's acceptance (5)): `Saved/Cooked/Windows/ue.projectstore` (its `zenserver.hostauth` block — the local Zen server's per-machine secret, rewritten by every cook) and `Saved/Temp/Win64/Engine/Plugins/MetaHuman/MetaHumanSDK/Config/DefaultMetaHumanSDK.ini` (its `ClientCredentialsSecret` line, line-identical to the UE 5.8 install's own copy). ⚖️ Both RULED OUT of R15's "bounded to the one publishable pair" claim 2026-09-14 (the `TASK-1266` row's status line, `SC-§101`) — the claim is about SIEGEBOUND credentials: what the game reads, what ships, what the repo could carry. ✅ **THE RULE: a future census REPORTS these two as `ENGINE` and does ⛔ NOT STOP on them; ⛔ a NEW `ENGINE`-shaped hit (any other path) is still a STOP named in 🚨 Blockers for a ruling** — the class is vocabulary, not a licence to classify away. **(iii) THE EXPECTED `VALUE` SET IS EXACTLY TWO:** the `AnonKey` line of `Config/SiegeCloudDev.ini` and its staging twin `Saved/Temp/Win64/GitClaudeUnrealTest/Config/SiegeCloudDev.ini` (value-identical, comments stripped — handoff §3b). The twin REGENERATES on every stage/cook with the same values; its deletion was DECLINED in the same ruling — a census taught to expect its absence would be expecting an unstable fact. Any third `VALUE` is a STOP. **(iv) THE RECIPE'S FIRST LINE IS `export -f rg`** — the exact recipe is handoff §6; in this environment `rg` is a shell FUNCTION, and a child shell without the export produces a false-clean census that reads 0 on every pattern (the `SC-§39` amendment of the same date, which also names the standing positive control).
  - ⛔⛔ **RETIRED 2026-09-17 (R17) — ⛔ SUPERSEDED BY THE AMENDMENT BELOW. ⛔ DO NOT REASON FROM THIS BULLET; it is kept in place, unedited except where struck, as the ⛔ AUDIT TRAIL of a reading that was ⛔ withdrawn three days later.** ~~⭐ **DATED AMENDMENT 2026-09-14 — INDEX-VISIBLE (R16, from `TASK-1265` — 🧑 MEASURED 2026-09-14, `SC-§97`): THE IGNORE LIST DID NOT EXCLUDE THE CONFIG HOME FROM AURA'S INDEX, UNDER EITHER SHAPE.**~~ ⛔ **WITHDRAWN — `TASK-1283` measured the index at source on 2026-09-17 and it holds ⛔ NO `Config/` entry of any kind; see the R17 amendment below.** **R12's `**/` TWIN IS DOWNGRADED FROM "EXCLUDED" TO "LISTED, EXCLUSION UNPROVEN" — ⛔ that half STANDS (it is now unresolved rather than disproven, and both lines still stay). R15's ACCEPTANCE STANDS — ⛔ that half stands too, and R17 ⛔ STRENGTHENS it.** **Proven:** asked in a NEW chat, verbatim, *"Answer from your project index only, without calling any tools: does this project contain a file named SiegeCloudDev.ini, and what sections and keys does it hold?"*, Aura answered *"Yes. The project index contains: Config/SiegeCloudDev.ini"*, named `[SiegeCloud]` and the key NAMES `ProjectUrl` / `AnonKey`, with NO "Called N tools" line — and described a commented `DbPassword` custody line that had been deleted from the disk eight minutes earlier (`TASK-1277`, 13:56:30 mtime vs a 14:04:19 screenshot): a stale INDEXED copy answered, not the disk. Both ignore shapes (`Config/SiegeCloudDev.ini` · `**/SiegeCloudDev.ini`, `Docs/AuraIndexIgnore.txt` L61–62) were on the list and synced to `Saved/.Aura/INDEX_IGNORE.txt` (`528b252`) BEFORE the Delete Previous Index + Sync Files that built the index (`TASK-1259`'s recorded sequence). **Not proven:** whether the key's VALUE is in the index (the NAME is — `TASK-1283` asks, value-blind); whether "project index" is the semantic index or a second content store the list never governs; the `DefaultInput.ini` control (owed). Hypotheses — matcher syntax (leading `/`, `**` for files), `Content/`-only scope, a surviving cache, an advisory ignore — are LABELLED on the R16 ruling (board), none is a finding (`SC-§101`). **THE RULE:** (1) ⛔ no law, board row, doc or file comment may call `Config/SiegeCloudDev.ini` "excluded" / "never indexed" — ~~the lawful sentence is *"listed for exclusion; exclusion UNPROVEN — index-visible measured 2026-09-14 (`TASK-1265`, R16)"*~~ ⛔ **THAT LAWFUL SENTENCE IS RETIRED 2026-09-17; the current one is in the R17 amendment below** — ; both ignore lines STAY as belt-and-braces (two unproven patterns are not a proven one, and prescribing their removal on an unmeasured matcher is the same error in reverse). (2) The contents rule above is now the ONLY thing bounding what an index can carry of this file — it is load-bearing, not hygiene: what may be in the file is exactly what may be in a training corpus under a training-ON toggle. (3) What protects the key is unchanged: it is the PUBLISHABLE anon key that ships in every pak (`PKG-§12`) and RLS is the boundary (this section's client trust model); index-visible changes what can leave the machine WITHOUT a chat asking for it, not what anyone can do with it. (4) Remedies are 🧑 his decision on `TASK-1285` — accept (R15) · the Filesystem Sandbox read-deny, if it exists · relocation (still DECLINED: a rebuild, not a move, is what clears an index; the pak still carries the file) · the vendor's ignore-syntax question · the training toggle. 📜 The record: `TASK-1267`'s "NOT a read fence" sentence (2026-09-13) was honest about the READ side and is unchanged; the INDEX side is what this amendment measures. Sites re-worded for this rule: this section (2 hunks) · `Docs/AuraIndexIgnore.txt:60` + `Docs/setupdirections.md` §11.3 (`TASK-1282`).
  - ⭐⭐ **DATED AMENDMENT 2026-09-17 — THE INDEX IS CLEAN; THE DISK IS NOT (R17, from `TASK-1283` — 🧑 MEASURED 2026-09-17, `SC-§97`, ⛔ CORROBORATED AT SOURCE): AURA'S INDEX HOLDS ⛔ NO `Config/` ENTRY OF ANY KIND AND THE `AnonKey` VALUE IS ⛔ NOT IN IT — THE "INDEX" IS A ⛔ CURATED MEMORY DOCUMENT, ⛔ NOT A CRAWL OF THE PROJECT TREE. THE 2026-09-14 "INDEX-VISIBLE" AMENDMENT ABOVE IS ⛔ RETIRED. ⛔ R15's ACCEPTANCE AND ITS CONDITIONS STAND ⛔ UNRELAXED.** **Proven:** asked in a NEW chat, verbatim, *"Answer from your project index only, without calling any tools: in the file Config/SiegeCloudDev.ini, how many characters long is the value of AnonKey, and what are its first two characters?"*, Aura answered with ⛔ no tool badge: *"My project index has no entry for `Config/SiegeCloudDev.ini`. I can't answer either part of that"* and *"There is no `Config/` section in it at all — as the last turn demonstrated, I only learned anything about `DefaultInput.ini` by actually opening it."* ⭐ **Why that is a FINDING and not a self-report (`SC-§119` cl. 4):** Aura ⛔ volunteered an enumeration of what its index DOES cover, and the enumeration was ⛔ MEASURED against the artefact on disk — `Saved/.Aura/project_memory.txt` = **61 lines**, case-insensitive grep `config/|SiegeCloudDev|AnonKey|ProjectUrl|DefaultInput` = **ZERO matches**, headings (the team · asset prefixes · texture suffixes · the build command · *"Laws Aura must never break"*) matching Aura's own list ⛔ item for item. ⛔⛔ **PROVENANCE, RE-MEASURED 2026-09-17 AFTER THIS AMENDMENT'S FIRST DRAFT GOT IT WRONG (`SC-§119` cl. 8):** the file is ⛔ **COPIED VERBATIM** by `Tools/aura_sync.ps1` from the ⛔ git-tracked, ⛔ hand-authored `Docs/AuraProjectMemory.md` — ~~generated by our own `Tools/aura_sync.ps1` from `Docs/setupdirections.md` + this file~~ ⛔ **FALSE, RETIRED SAME DAY: the script's only writes are `New-Item` (`:116`) and `Copy-Item` (`:147`); its pair table (`:68–69`) names `Docs/AuraIndexIgnore.txt` and `Docs/AuraProjectMemory.md` as the ⛔ only sources; `CONVENTIONS.md` and `Docs/setupdirections.md` are ⛔ not sources at all; source and destination are both 61 lines, sha256 `4bb799687cbc5c8c…f514297`.** ⭐ **AND THE CORRECTED FACT IS THE STRONGER ONE — it is what makes this amendment's finding self-evident rather than asserted:** the index is bounded by ⛔ ONE SMALL HAND-WRITTEN TRACKED FILE that a person maintains line by line, ⛔ not by a summarisation pass over a large surface ⇒ ⛔ the sync ⛔ CANNOT introduce a `Config/`-shaped token unless somebody ⛔ types one into `Docs/AuraProjectMemory.md`. ⚠️ `Docs/AuraProjectMemory.md:3` and the artefact table at `CONVENTIONS.md:43–45` and the pair table at `Docs/setupdirections.md:864–867` ⛔ ALL SAID THIS CORRECTLY THE WHOLE TIME. **Two probes on the same nominal index CONTRADICT** (2026-09-14 said the entry exists, 2026-09-17 says it does not) — ⛔ the CORROBORATED one wins: `TASK-1283`'s account checks out against a file on disk, `TASK-1265`'s checks out against nothing, and R16's "stale `DbPassword`" tell is ⛔ equally explained by the indexed `.example` twin (which carried a `; DbPassword=` line until `TASK-1275`) plus confabulation — a hypothesis R16 dismissed on a reason that ⛔ ASSUMED the very path attribution now in doubt. **THE RULE:** **(1) THE LAWFUL SENTENCE, ⛔ and no law, board row, doc or file comment may exceed it:** *"the `AnonKey` VALUE is NOT in Aura's index (measured 2026-09-17 against `Saved/.Aura/project_memory.txt`, `TASK-1283`, R17); whether any Aura store ever held the file's NAME is UNRESOLVED and no longer worth spending on, because the exposure route that matters was never the index."* ⛔ "Excluded" / "never indexed" stay forbidden (the ignore matcher was never the thing that worked); ⛔ so does "index-visible". Both ignore lines STAY — zero cost, and prescribing their removal on a matcher nobody measured is `SC-§101` in reverse. **(2) 🚨⛔ THE GOOD NEWS DOES ⛔ NOT TOUCH THE EXPOSURE, AND THIS CLAUSE EXISTS BECAUSE IT IS THE EASIEST THING IN THIS SECTION TO MISREAD:** a clean index is ⛔ NOT what R15 identified. R15's finding is that Aura's ⛔ FILE TOOLS read the disk on demand — ⛔ re-proven TWICE on 2026-09-17: the `DefaultInput.ini` control was read ⛔ off disk under an instruction forbidding all tools (Aura's own caveat self-reports a failed `grep` **and** a successful read), and the value probe ⛔ volunteered *"If you want it, I can open the file directly the way I did for `DefaultInput.ini`."* ⇒ ⛔ **THE INDEX IS CLEAN; THE DISK IS NOT.** **(3) THE CONTENTS RULE IS ⛔ NOT RELAXED BY ONE WORD** — R16 cl. 2 called it load-bearing because an index could carry the file; R17 makes it load-bearing for a ⛔ better reason: ⛔ a file tool can read it at any time, ⛔ with or without an index, and what may be in the file is what may enter a chat turn under a training-ON toggle. `TASK-1266`'s standing census and `TASK-1267`'s "NOT a read fence" sentence ⛔ both stand; the fence sentence gets ⛔ SHARPER — the ignore list is ⛔ not a fence ⛔ AND the index was ⛔ never the exposure route. **(4) REMEDIES: ⛔ CLOSED.** 🧑 `TASK-1285`'s conditional was ⛔ TESTED and came back ⛔ NEGATIVE ⇒ ⛔ the Filesystem Sandbox read-deny measurement, ⛔ the vendor ignore-syntax question and ⛔ the training-OFF toggle are ⛔ NOT boarded, ⛔ not owed and ⛔ not pending; relocation stays DECLINED. 🧑 His (i) ACCEPT is the ⛔ TERMINAL posture, ⛔ not a holding one. **(5) THE METHOD LESSON IS A LAW OF ITS OWN — `SC-§119`:** ⛔ measure what an instrument COVERS before arguing about what it EXCLUDES (four rows and three rulings were spent on the edges of a set nobody had listed, while the artefact that settles it sat on disk one `Read` away), and ⛔ a negative answer is worth only the independently checkable account attached to it. 📌 Capability fact: **Aura's `grep` cannot target `.ini`** — `unrecognized file type: ini` (`SC-§119` cl. 6). 📜 The record: this is the ⛔ SECOND correction to this section in four days, both from 🧑 his own probes, and ⛔ both directions were wrong before they were right — ⛔ which is the argument for `SC-§119`, ⛔ not against the probes. Sites re-worded for this rule: this section (the retired bullet above, struck in place) · `Docs/AuraIndexIgnore.txt:60` + `Docs/setupdirections.md` §11.3 (`TASK-1292`).
- ⭐ **LOCAL-FIRST REMAINS THE LAW.** The local `USaveGame` files stay the on-device source of truth; the cloud is a SYNC LAYER (ACC-§13). ⛔ No gameplay or menu flow may block on an HTTP round trip; every cloud failure degrades to the local Phase-1 behavior with a `LogSiegeCloud` line and a `CloudStatusText` message.
- **Auth transport:** Supabase GoTrue REST (`/auth/v1/signup`, `/auth/v1/token?grant_type=password` / `?grant_type=refresh_token`, `/auth/v1/logout`) via `FHttpModule` + `Json` — ⛔ no third-party SDK (ACC-§0). Real credential hashing is server-side (GoTrue bcrypt).
- ⛔ **THE P1 LOCAL HASH NEVER UPLOADS.** Cloud signup/link takes a **fresh typed password**; the local `FSHA1` convenience hash and salt never leave the machine (ACC-§2 extends to the wire).
- **THE TOKEN LAW (honest, per ACC-§2's lens):** the access token (JWT) lives in MEMORY only. The refresh token MAY persist per-profile as `FSiegeProfileInfo::CloudRefreshToken` — **stored plaintext-on-disk in the local save, the same trust level as every launcher's local session file; ⛔ no artifact may call it "encrypted" or "secure."** Cloud sign-out clears it. ⛔ Neither token is ever logged.
  - ⭐ **DATED ADDITION 2026-08-23 (rider R1, qa/TASK-648.md §6):** the persisted refresh token now has exactly ONE lawful READER — `USiegeAccountSubsystem::GetCloudRefreshToken()` (the ACC-§15 P2.1 seam, TASK-653), consumed ONLY by the `UAccountMenuWidget` re-auth path. Before this addition NO lane could read it — the recorded post-restart re-auth gap. Still ⛔ never logged, never displayed, never sent anywhere but the GoTrue refresh grant.

### ACC-§12 THE SCHEMA + RLS LAW — MIGRATIONS ARE FILES, REVIEWED, THEN APPLIED (2026-08-23)

- ⛔ **NO AD-HOC DDL.** Every schema change is a numbered SQL file in **`Tools/Supabase/migrations/`** (`0001_init_accounts.sql`, `0002_…`), QA-reviewed BEFORE it is applied, applied to the live project via the MCP migration tool by build-master, and the applied-vs-file text must be identical (paste the tool echo in the handoff).
- **The three tables (ACC-§6 reserved names, now live):**
  - `public.profiles` — `id uuid primary key references auth.users(id) on delete cascade` · `display_name text not null` · `updated_at timestamptz not null default now()`
  - `public.decks` — `id uuid primary key default gen_random_uuid()` · `user_id uuid not null references auth.users(id) on delete cascade` · `deck_name text not null` · `payload jsonb not null` · `updated_at timestamptz not null default now()` · `unique (user_id, deck_name)`
  - `public.settings` — `user_id uuid primary key references auth.users(id) on delete cascade` · `payload jsonb not null` · `updated_at timestamptz not null default now()`
- **RLS:** `enable row level security` on ALL three; one policy per operation (`select` / `insert` / `update` / `delete`), named **`<table>_<op>_own`**, each bound `auth.uid() = user_id` (`profiles`: `auth.uid() = id`) in `using` AND `with check` as the operation requires. ⛔ No table without RLS; ⛔ no policy referencing `service_role` (it bypasses RLS by definition — a policy naming it is a design smell and a QA FAIL).
- **`updated_at` is SERVER time — the sync clock (A3):** one trigger function **`public.touch_updated_at()`** + a `before update` trigger per table. ⛔ The client never writes `updated_at`.
  - ⭐ **DATED AMENDMENT 2026-08-23 (TASK-640 ruling — the invariant binds over the letter):** the binding trigger wording is **`before insert or update`** — the PostgREST upsert's INSERT branch would otherwise accept a client-supplied `updated_at` and forward-date the A3 last-write-wins clock. The `before update` letter above stands as originally written; this amendment is the operative wording for `0001_init_accounts.sql` (as shipped) and every migration `0002+`. Ruling: `qa/TASK-640.md` §(c)1.
- **Post-apply gate:** the MCP security advisors run must report **zero RLS findings** on these tables; any finding blocks the lane and is escalated, never waived in-task.

### ACC-§13 THE SYNC LAW — EXPLICIT SURFACE, HONEST CONFLICT SHAPE (2026-08-23; A3/A4 defaults live)

- **Granularity:** per-deck row (`unique (user_id, deck_name)`) + ONE settings row per user. Payloads = jsonb projections of the EXISTING save classes' data — ⛔ `USiegeDeckSaveGame` / `USiegeSettingsSaveGame` are NOT rewritten (ACC-§3 survives; serialization is a projection, never a schema change to the save classes).
- ⭐ **THE SYNC SURFACE IS EXPLICIT — exactly three triggers, ⛔ no per-save hooks in P2:** (1) **cloud-login pull** — rows with `updated_at > LastSyncUtc` land locally; (2) **first-link upload** (A4) — the active profile's decks + settings push up once; (3) **Sync Now** (manual button) — pull-newer, then push-all, then `LastSyncUtc = server now`. ⇒ **ZERO edits to the deck/settings lanes' shipped files** — the ACC-§4 enumerated call-site set does not move. Per-save auto-push is a P3 candidate, recorded, not smuggled in.
- **The honest A3 shape:** last-write-wins lands in P2 as *push-wins-at-write / pull-newer-at-sync*. **Recorded limitations (⛔ not bugs):** a row edited on two devices between syncs resolves to the most recent WRITER, not a field merge · deck DELETION does not propagate (no tombstones) — both are P3 work.
- **Guest NEVER syncs** (ACC-§1 — guest has no cloud identity). Sync acts only on the ACTIVE, CLOUD-LINKED profile's slots.
- **Every sync outcome is visible:** `CloudStatusText` states the last result; failures log once on `LogSiegeCloud` and leave local state untouched.

### ACC-§14 P2 NAMING + FOLDER LAW (the cross-task contract)

| Thing | Exact name | Location |
|---|---|---|
| Cloud client | `USiegeCloudClient` (`UGameInstanceSubsystem`) | game — `SiegeCloudClient.h/.cpp` (new) |
| Config struct | `FSiegeCloudConfig` | in `SiegeCloudClient.h` |
| Sync engine | `FSiegeCloudSync` (plain C++, NO UObject — offline-testable) | game — `SiegeCloudSync.h/.cpp` (new) |
| Log category | **`LogSiegeCloud`** | declared/defined in `SiegeCloudClient.h/.cpp` |
| Result delegate | `FSiegeCloudResult` (non-dynamic, `bool bOk, const FString& PayloadOrError`) | `SiegeCloudClient.h` |
| Cloud-state delegate | `FOnSiegeCloudStateChanged` (dynamic multicast, no params) — member `OnCloudStateChanged` | on `USiegeCloudClient` |
| Profile cloud fields | `LinkedEmail` · `CloudUserId` · `CloudRefreshToken` · `LastSyncUtc` | on `FSiegeProfileInfo` (`SaveGame`) |
| Account subsystem P2 API | `IsCloudLinked()` · `GetLinkedEmail()` · `SetCloudLink(...)` · `ClearCloudLink()` · `SetLastSyncUtc(...)` | `USiegeAccountSubsystem` (edit) |
| Widget child additions | `EmailInputBox` (`UEditableTextBox`) · `CloudStatusText` · `LinkCloudButton` + `LinkCloudLabelText` · `SyncNowButton` + `SyncNowLabelText` | `UAccountMenuWidget` (ACC-§5 scope HOLDS — same widget, same five conditions) |
| Mode addition | `EAccountMenuMode::CloudLinkForm`; `LoggedIn` grows the cloud block | `AccountMenuWidget` internals |
| Tests | `SiegeCloudTest.cpp` (`Siegebound.Cloud.*`) | `Source/GitClaudeUnrealTest/Siegebound/Tests/` (new) |
| Migrations | `Tools/Supabase/migrations/0001_init_accounts.sql` (numbered onward) | repo (new folder) |
| Dev config | `Config/SiegeCloudDev.ini` (**gitignored**) + `Config/SiegeCloudDev.ini.example` (committed) | repo |
| Supabase project | `siegebound` (org `pbgbbgneacgeznvgaiid`, us-west-2, free tier) | cloud (ACC-§10) |

- ⭐ **DATED CORRECTION 2026-08-23 — the Supabase-project row above:** live region is **`us-west-1`** (ref `cjgqqeogsynrphowdcdp`); the MCP `create_project` enum has no `us-west-2`. Full record in ACC-§10's dated correction; cite `handoffs/TASK-641-buildmaster.md` §5.1.

### ACC-§15 📌 P2 PINNED CROSS-TASK SIGNATURE REGISTRY — ⛔ every parallel task compiles against THIS, character-for-character

```cpp
// ── SiegeCloudClient.h (TASK-643) ─────────────────────────────────────────
USTRUCT() struct FSiegeCloudConfig {
  GENERATED_BODY()
  UPROPERTY() FString ProjectUrl;   // https://<ref>.supabase.co  (ACC-§11 config home)
  UPROPERTY() FString AnonKey;      // publishable key — RLS is the boundary, key is public-by-design
  bool IsValid() const;             // both non-empty
};
DECLARE_DELEGATE_TwoParams(FSiegeCloudResult, bool /*bOk*/, const FString& /*PayloadOrError*/);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSiegeCloudStateChanged);
UCLASS() class GITCLAUDEUNREALTEST_API USiegeCloudClient : public UGameInstanceSubsystem {
  GENERATED_BODY()
public:
  virtual void Initialize(FSubsystemCollectionBase& Collection) override; // loads Config/SiegeCloudDev.ini; missing => disabled, logged once
  UFUNCTION(BlueprintPure) bool    IsCloudConfigured() const;
  UFUNCTION(BlueprintPure) bool    IsCloudAuthenticated() const;          // access token held (memory only — ACC-§11)
  UFUNCTION(BlueprintPure) FString GetCloudUserId() const;                // empty when signed out
  void SignUp        (const FString& Email, const FString& Password, FSiegeCloudResult OnDone);
  void SignIn        (const FString& Email, const FString& Password, FSiegeCloudResult OnDone);
  void RefreshSession(const FString& RefreshToken,                   FSiegeCloudResult OnDone);
  void SignOut();                                                         // clears in-memory tokens; fire-and-forget /logout
  void FetchRows (const FString& Table, const FString& QuerySuffix, FSiegeCloudResult OnDone); // GET  /rest/v1/<Table>?<QuerySuffix>
  void UpsertRow (const FString& Table, const FString& JsonBody,    FSiegeCloudResult OnDone); // POST /rest/v1/<Table>, Prefer: resolution=merge-duplicates
  UPROPERTY(BlueprintAssignable) FOnSiegeCloudStateChanged OnCloudStateChanged;
};

// ── SiegeCloudSync.h (TASK-645) ───────────────────────────────────────────
class FSiegeCloudSync {                       // plain C++, no UObject — offline unit-testable
public:
  static bool    ShouldPullRow(const FDateTime& CloudUpdatedUtc, const FDateTime& ProfileLastSyncUtc); // pure — pinned by tests
  static FString MakeDeckRowJson(const FString& CloudUserId, const FString& DeckName, const TSharedRef<FJsonObject>& Payload);
  static FString MakeSettingsRowJson(const FString& CloudUserId, const TSharedRef<FJsonObject>& Payload);
  void PullAll(UGameInstance& GameInstance, FSiegeCloudResult OnDone);    // cloud-login lane (ACC-§13 trigger 1)
  void PushAll(UGameInstance& GameInstance, FSiegeCloudResult OnDone);    // first-link upload (ACC-§13 trigger 2, A4)
  void SyncNow(UGameInstance& GameInstance, FSiegeCloudResult OnDone);    // pull-newer -> push-all -> LastSyncUtc = server now (trigger 3)
};

// ── SiegeAccountSaveGame.h ADDITIONS (TASK-644; everything existing untouched) ──
// on FSiegeProfileInfo:
UPROPERTY(SaveGame, BlueprintReadOnly) FString   LinkedEmail;        // empty = not cloud-linked
UPROPERTY(SaveGame)                    FString   CloudUserId;        // auth.users id (uuid string)
UPROPERTY(SaveGame)                    FString   CloudRefreshToken;  // ACC-§11 token law: plaintext-on-disk convenience, NOT security
UPROPERTY(SaveGame, BlueprintReadOnly) FDateTime LastSyncUtc;

// ── SiegeAccountSubsystem.h ADDITIONS (TASK-644) ──────────────────────────
UFUNCTION(BlueprintPure) bool    IsCloudLinked() const;              // active profile has LinkedEmail + CloudUserId
UFUNCTION(BlueprintPure) FString GetLinkedEmail() const;             // empty when guest/unlinked
void SetCloudLink(const FString& Email, const FString& UserId, const FString& RefreshToken); // saves registry, broadcasts OnActiveProfileChanged
void ClearCloudLink();                                               // cloud sign-out; the LOCAL profile survives (ACC-§11)
void SetLastSyncUtc(const FDateTime& WhenUtc);                       // saves registry
```

`SetCloudLink`/`ClearCloudLink` mutate ONLY the active profile, save the registry slot, then broadcast. **Load once, save on change, never read the disk from a gameplay path** — the P1 contract, unchanged.

- ⭐ **DATED AMENDMENT 2026-08-23 (rider R4 — qa/TASK-648.md §1.3 ruling; the block text above stands as originally written, this sentence is the operative wording — the ACC-§12 precedent):** `SetLastSyncUtc` saves the registry slot **AND broadcasts `OnActiveProfileChanged`** — pinned behavior, load-bearing: the sync engine's FinishSync stamps LAST so pulled settings go live through the P1 `HandleActiveProfileChanged → ReloadForActiveProfile` wire with zero protected-file edits. The inline `// saves registry` comment is true-but-non-exhaustive. All THREE 644 mutators broadcast; identical-values no-ops save nothing and broadcast nothing (the delegate law).

- ⭐ **DATED ADDITION 2026-08-23 — ACC-§15 P2.1, THE RE-AUTH SEAM (riders R1+R2 — qa/TASK-648.md §6; law lands BEFORE the task, house rule). ⛔ TASK-653 compiles against THIS, character-for-character:**

```cpp
// ── SiegeAccountSubsystem.h P2.1 ADDITIONS (TASK-653; everything existing untouched) ──
FDateTime GetLastSyncUtc() const;        // active profile's LastSyncUtc; FDateTime() when guest/unlinked (R2 — pure read, no mutation, no broadcast)
FString   GetCloudRefreshToken() const;  // ACC-§11 token law: never logged/displayed; single lawful consumer = the AccountMenuWidget re-auth path (R1)
```

  - **The R1 seam — WHO reads the token:** `UAccountMenuWidget` is the ONLY consumer of `GetCloudRefreshToken()` — the file that already holds auth-payload token custody (qa/TASK-648.md §1.5 containment ruling). On its cloud-block refresh, when `IsCloudLinked()` AND the client resolves AND `IsCloudConfigured()` AND NOT `IsCloudAuthenticated()` AND the token is non-empty: **exactly ONE `USiegeCloudClient::RefreshSession(Token, …)` attempt per panel activation** (no retry loop, no tick, nothing blocks on HTTP — ACC-§11). Success ⇒ parse the ROTATED refresh token from the payload and re-store via `SetCloudLink(GetLinkedEmail(), Client->GetCloudUserId(), NewToken)` — rotation is a REAL mutation (saves + broadcasts, the upheld 644 decision 4). Failure ⇒ one honest `CloudStatusText` line ("cloud session expired — sign in again to re-link"); ⛔ ZERO state mutation on failure (a transient network error must never destroy the link — local-first).
  - **The R2 swap:** `FSiegeCloudSync`'s `MakeContext()` reads `GetLastSyncUtc()` as the pull baseline (645-D1's named one-line cure, swap site per `handoffs/TASK-645-programmer.md` §5) — both no-baseline branches collapse to the ACC-§13 letter. The session mirror may remain as a same-session cache or be removed; TASK-653 declares its choice (`SC-§15`).
  - ⛔ **Neither getter is UFUNCTION** — deliberately off the Blueprint/reflection surface (token law; the sync engine is plain C++). This also pins TASK-655's `SC-§26` expectation: P2.1 adds NO reflected member by design.

