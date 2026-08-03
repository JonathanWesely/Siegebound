# QA Report — TASK-439 (SET-QA1: gate covering TASK-436 + TASK-437)

**Reviewer:** qa-reviewer · **Date:** 2026-08-03 · **Type:** PRE-COMPILE review, by reading
**Scope:** TASK-436 (`USiegeSettingsSaveGame` + `USiegeSettingsSubsystem` + tests) · TASK-437 (`USettingsMenuWidget`)

## Verdict

| Task | Verdict | Blockers | Warnings | Nits |
|---|---|---|---|---|
| **TASK-436** | ✅ **PASS** | **0** | 2 | 2 |
| **TASK-437** | ✅ **PASS** | **0** | 2 | 3 |
| **TASK-439 (overall)** | ✅ **PASS** | **0** | 4 | 5 |

**Board flips I am asking the orchestrator to apply (I have no edit tool):**
- TASK-436 → `qa-passed`
- TASK-437 → `qa-passed`
- TASK-439 → `done` (report `qa/TASK-439.md`)

---

## 0. ⛔ WHAT I DID NOT RUN — read this before the findings

Per the spec's §9c duty, stated plainly and first:

- **NOTHING IN THIS BATCH HAS BEEN COMPILED.** No UBT, no UHT, no clang, no MSVC. **I did not run the parser, and on this project the parser is the reviewer of record.** Every statement below about whether something *compiles* is a reading, not a result. **First execution is TASK-447.**
- **NO TEST HAS BEEN RUN.** The seven `Siegebound.Settings.*` tests are assertions that *will be made*; none has produced a green tick. I reviewed what they assert and how they are isolated — not what they return.
- **NO PIXEL HAS BEEN RENDERED.** No editor, no PIE, no MCP, no viewport.
- **Tool attribution (the §10 `Grep` trap):** the mangling is real and I hit it live in this session — `WidgetBlueprintGeneratedClass.cpp:144` and `:303` came back through `Grep` as `\ Note:` where the file says `//`. ⇒ **Every line I quote or reason about structurally from a project file came from raw `Read`:** all five TASK-436 files, both TASK-437 files, and `GitClaudeUnrealTest.Build.cs`. `Grep` was used only to LOCATE, and for two same-line existence checks in files I did not read whole (`SessionMenuWidget.h`, `SiegeDeckSaveGame.h`) — I quote no comment syntax from those.
- **Engine source was read raw** from `C:\Program Files\Epic Games\UE_5.8\Engine\Source\...` for every claim I ruled on. Line numbers below are from that installed tree.

---

## 1. ⚠️ THE CROSS-TASK SEAM (checked first, as dispatched) — RULED: NO MISMATCH, AND NO MISMATCH IS POSSIBLE

TASK-437 was authored before TASK-436 published its constants. The dispatch asked whether 437 guessed. **It did not — and it did something better: it removed the guess from the failure surface entirely.**

**(a) The token.** Verified character-for-character in both files, raw `Read`:

- `SiegeSettingsSubsystem.cpp:20` — `const FName USiegeSettingsSubsystem::SettingName_AssistantConfirmBeforeExecute(TEXT("bAssistantConfirmBeforeExecute"));`
- `SettingsMenuWidget.cpp:55` — `const TCHAR* USettingsMenuWidget::ConfirmSettingName = TEXT("bAssistantConfirmBeforeExecute");`

Identical literal. Also matches the SaveGame field `SiegeSettingsSaveGame.h:74` and CONVENTIONS §2 (line 1176). ✅

**(b) The mismatch is not merely absent — it is unreachable.** `USettingsMenuWidget::HandleSettingsChanged` (`SettingsMenuWidget.cpp:450-467`) **never compares the payload to anything.** It logs the name and re-reads the live value:

```cpp
UE_LOG(LogSiegeSettings, Verbose, TEXT("[SettingsMenu] Settings changed broadcast ('%s') - refreshing the row."), *SettingName.ToString());
ApplyConfirmValueToRow(Settings->IsAssistantConfirmEnabled());
```

⇒ **The "silent name mismatch that compiles fine and never fires" failure mode does not exist in this code**, because there is no name-equality test on the hot path. 437's reasoning for that (`SettingsMenuWidget.h:152-157`: the broadcast token was TASK-436's to choose and was not pinned) is sound and I accept it as authored. The suppression that makes it safe is `ApplyConfirmValueToRow`'s no-op guard (`SettingsMenuWidget.cpp:479`), which stops a future *other* setting's broadcast from producing a spurious event. ✅

**(c) The delegate TYPE binds.** `DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSiegeSettingsChanged, FName, SettingName)` (`SiegeSettingsSubsystem.h:26`) vs handler `UFUNCTION() void HandleSettingsChanged(FName SettingName)` (`SettingsMenuWidget.h:182-183`). Signature and `UFUNCTION` both present ⇒ `AddUniqueDynamic` at `SettingsMenuWidget.cpp:364` resolves. ✅ Same check passed for `OnCheckStateChanged`→`HandleConfirmToggleChanged(bool)` and `OnClicked`→`HandleBackClicked()`.

**(d) The fallback-TRUE rule.** 437 is the *editor* of the value, not a consumer that decides on it, so it never computes `bConfirm`. Its no-subsystem path is nonetheless **consistent with** fallback-TRUE and does not contradict it — `ShowRowUnavailable()` (`SettingsMenuWidget.cpp:503-511`) shows the row **CHECKED and DISABLED**, and the substituted hint (`:47-48`) says *"Settings are unavailable right now, so AI orders will always be shown for review."* That is the honest rendering of "an unresolvable lookup degrades to MORE human review." ✅
⚠️ **Recorded, not certified:** that string asserts a behaviour of **TASK-443**, which does not exist yet. **TASK-446 owes the confirmation that 443's unresolvable-subsystem path actually is confirm=ON.** If 443 ships a `false` fallback, this screen becomes a lie and it is 443 that is wrong, not this file.

**(e) Deviation from the published resolution snippet — accepted.** 436's §2c snippet uses `GetGameInstance()`; 437 uses `GetWorld() → GetGameInstance() → GetSubsystem<>` (`SettingsMenuWidget.cpp:524-532`). Functionally identical, null-safe at every hop, and it matches the shipped `USessionMenuWidget::ResolveSessionSubsystem` shape the spec item (6) told it to clone. Not a finding.

---

## 2. THE FOUR DECLARED ITEMS — RULED INDIVIDUALLY

### Item 1 — TASK-437's deliberate spec departure (tree built BEFORE `Super::RebuildWidget()`)

⚖️ **RULING: THE DEPARTURE IS CORRECT. THE SPEC'S LITERAL WORDING IS WRONG. ACCEPTED AS AUTHORED — and the LAW TEXT should be amended, not the code.**

**I verified the engine claim myself rather than accepting the relayed diagnosis.** `UMG/Private/UserWidget.cpp:1214`, read raw:

```cpp
TSharedRef<SWidget> UserRootWidget = WidgetTree->RootWidget ? WidgetTree->RootWidget->TakeWidget() : TSharedRef<SWidget>(SNew(SSpacer));
```

`UUserWidget::RebuildWidget()` reads `WidgetTree->RootWidget` **as it stands at that instant** and returns an `SSpacer` when it is null. **Constructing after `Super` and returning `Super`'s result therefore ships a widget whose Slate root is an empty spacer while every UPROPERTY reads back perfectly** — precisely the defect class ruling 9 exists for. The programmer's claim is exact.

**And I checked the semantic half he said was preserved.** `UUserWidget::Initialize()` (`UserWidget.cpp:135-202`):
- WBP path — `BGClass->InitializeWidget(this)` at `:153` → `UWidgetBlueprintGeneratedClass::InitializeWidget` (`WidgetBlueprintGeneratedClass.cpp:327-351`) → `InitializeWidgetStatic` → **`Prop->SetObjectPropertyValue_InContainer(UserWidget, Widget);` at `WidgetBlueprintGeneratedClass.cpp:275`.** That is where `BindWidget`/`BindWidgetOptional` members are assigned, and it runs **inside `Initialize()`, before `RebuildWidget()` is ever entered.**
- Native path (no `.uasset`, which is this widget's shipped path) — `InitializeNativeClassData()` at `:157`; nothing ever assigns the members, so they stay null.

⇒ In **both** paths "still null" is fully determined before `RebuildWidget()` runs. **The ordering clause in the spec is not just unfollowable, it is semantically vacuous** — there is nothing `Super::RebuildWidget()` could have changed. The escape hatch (`SettingsMenuWidget.cpp:101-106`, early return on `WidgetTree->RootWidget != nullptr`) is intact and §3 condition (b) is satisfied. ✅

📋 **ACTION FOR MANAGER (not a finding against 437):** `CONVENTIONS.md:1195` (§3 condition (b)) and TASKBOARD TASK-437 spec item (1) both still say *"constructs a child only if that member is still null **after** `Super::RebuildWidget()`"*. **That sentence is wrong and it will mislead the next code-authored widget into shipping an empty panel.** Recommend amending it to *"…constructs the tree **before** `Super::RebuildWidget()`, which returns an `SSpacer` when `RootWidget` is null (`UserWidget.cpp:1214`); 'only if still null' is fully determined by then because `BindWidget*` members resolve in `Initialize()`."*

### Item 2 — TASK-441 appended `int32 Orderable` to `FSiegeAssistantRosterEntry`

⚖️ **RULING: NO EXPOSURE. Neither task assumed the old shape, because neither task knows the struct exists.**
`Grep` for `RosterEntry|Snapshot|AssistantCommand|SiegeAssistant` across all `*Settings*` sources returns **exactly one hit**, and it is prose: `SettingsMenuWidget.h` names `USiegeAssistantConsoleWidget` while explaining that §6 ruling A is *not* being cited. **Zero structural references, zero includes, zero `FSiegeAssistantRosterEntry` usage in either task.** ✅

### Item 3 — should the definition-free `SiegeSettingsSaveGame.cpp` exist?

⚖️ **RULING: KEEP IT. Do not delete.** Grounds, in order of weight:
1. **CONVENTIONS §2 (line 1169) writes the pair into the law** — `SiegeSettingsSaveGame.h/.cpp`. Deleting the `.cpp` is a deviation from a written line for no gain, and would then need its own sanction.
2. **It is not an empty TU and has no compile or link consequence.** It includes its header (`SiegeSettingsSaveGame.cpp:3`), so it is a well-formed translation unit. The UHT glue (`IMPLEMENT_CLASS`, `Z_Construct_UClass_USiegeSettingsSaveGame`) is emitted into the generated `.gen.cpp`, **not** into this hand-written file — the class registers whether or not this file has a line in it.
3. **It cannot be misread as an oversight**, which was the programmer's stated worry: `SiegeSettingsSaveGame.cpp:5-17` says so in the first line (*"INTENTIONALLY EMPTY OF DEFINITIONS — this is not a stub"*) and names the `USiegeDeckSaveGame` precedent it deliberately departs from.
4. The stated future use (an out-of-line `static const` table / migration helper for setting #2) is real; `USiegeDeckSaveGame` already needs one for `static const FString SlotName` (`SiegeDeckSaveGame.h:34`).

Recorded as **NIT-436-2** so the decision is on the record rather than re-litigated at TASK-447.

### Item 4 — `BackdropBorder` is `ESlateVisibility::Visible`, not hit-test-invisible

⚖️ **RULING: THE REASONING IS CORRECT AND I VERIFIED ITS MECHANICAL BASIS. Accepted.** ⚠️ **AND ONLY A HUMAN CAN CLOSE IT.**

Verified against installed source:
- `UWidget::ConvertSerializedVisibilityToRuntime` (`UMG/Private/Components/Widget.cpp:1674-1679`): `ESlateVisibility::Visible` → `EVisibility::Visible`, which hit-tests **self and children** — as opposed to `HitTestInvisible` (`:1684-1685`) which hit-tests neither. So `SettingsMenuWidget.cpp:135` does make the plate absorb.
- The claim about the owning widget is exact too: `UUserWidget`'s constructor sets `SetVisibilityInternal(ESlateVisibility::SelfHitTestInvisible)` at `UserWidget.cpp:100`. "I do not hit-test, my children do" — the border is the child doing the absorbing. ✅
- The "not dependent on painted pixels" claim is right: Slate builds the widget path from **visibility + geometry**, and an unhandled event bubbles **up the path**, never down to a lower-Z sibling. A main-menu button that is not in the path never sees the click.

⛔ **WHAT THAT DOES NOT ESTABLISH, and I will not let it drift:** the absorb only holds **where the border's geometry actually covers the button.** That depends on TASK-438 giving the panel a full-screen viewport slot and on the border actually stretching — **runtime geometry, which no readback on this project has ever been able to see** (TASK-355: 6/6 green on six controls stacked in a 165×48 px corner box). ⇒ **The click-through test is Jonathan's, at TASK-448, and it is the blocker criterion. QA passes the CODE, not the SCREEN.**

---

## 3. ⛔ TWO CLAIMS THIS REPORT DOES NOT MAKE

**(1) THE OFF-PATH IS NOT CERTIFIED HERE, AND NOTHING IN TASK-436 MAY BE CITED AS EVIDENCE FOR IT.**
TASK-436's handoff §4 declines to certify the toggle's off-path and names TASK-443/TASK-446 as its owners. **I decline as well, and I decline for the same reason: the off-path does not exist yet.** What I *did* verify — by reading, and it is a claim about scope only — is that `SiegeSettingsSubsystem.cpp` includes `Kismet/GameplayStatics.h`, its own SaveGame, and `UObject/UObjectGlobals.h`, **and nothing else**; there is no reference anywhere in either task to the assistant, the FSM, the snapshot, the guard, the parser or the executor, and the public surface is one `bool` getter and one `bool` setter. ⇒ **These two tasks cannot skip a check.** ⛔ **That is NOT the same sentence as "the off-path still runs every check", it does not imply it, and it must not be quoted as though it did.** The obligation stands unchanged on TASK-443's handoff (the side-by-side check-for-check trace) and TASK-446's gate: *every check that runs with the toggle ON runs with it OFF.*

**(2) NOTHING ON SCREEN IS VERIFIED.** UI rendering is not machine-verifiable on this project (ruling 9). Every appearance, layout, legibility and click claim in both handoffs is **untested**, including the ones the programmer wrote honestly as untested. **TASK-437 handoff §7 is the checklist and it goes to Jonathan at TASK-448 in full.** Two items I want carried forward with emphasis:
- **§7 item 2 — the click-through test over Play / Sandbox / Deck Builder / Multiplayer / QUIT.** This is the blocker criterion of the whole widget.
- **§7 item 1 — a blank or invisible panel.** If it is blank, the `RebuildWidget` ordering is *not* the suspect (I verified it is right); look at `WARN-437-1` below and at TASK-438's `AddToViewport`.
- Additional item I am adding to that checklist from the code: `ConfirmToggleLabelText` is `SetAutoWrapText(true)` (`SettingsMenuWidget.cpp:192`) **while living inside the check box's content slot**, which is a plain `UPanelSlot` with no width constraint. Auto-wrap inside an unconstrained content slot can produce an odd desired width. **Look at the toggle row's label specifically** — is it on one line, and is the click target the whole phrase?

---

## 4. TASK-436 — findings

### PASS criteria confirmed (all by raw `Read`)

| Criterion (from the TASK-439 spec) | Result | Evidence |
|---|---|---|
| §8 pin, character-for-character | ✅ | `SiegeSettingsSubsystem.h:104,119,126-127,139-140,147-148,152`, `SiegeSettingsSaveGame.h:73-74`, delegate+log at `:14,26`. Only additions are `GITCLAUDEUNREALTEST_API` + `GENERATED_BODY()` — required, and the shipped idiom. |
| Slot name exactly `"SiegeSettings"` | ✅ | `SiegeSettingsSubsystem.cpp:16`. Asserted by test at `SiegeSettingsTest.cpp:120-121`. |
| Default `true` | ✅ | `SiegeSettingsSaveGame.h:74`; in-memory mirror `SiegeSettingsSubsystem.h:210`; drift guard asserted at `SiegeSettingsTest.cpp:176-178`. |
| Missing/wrong-class slot ⇒ defaults, logged once, no crash | ✅ | `DoesSaveGameExist` first (`:58`), `Cast<>` (`:62`), CDO fallback (`:68`), one-shot latch `bLoadFallbackLogged` (`:70-78`) at `Log` not `Warning`. |
| No `LoadGameFromSlot` on any read path | ✅ | `IsAssistantConfirmEnabled()` (`:36-41`) returns the member. The **only** `LoadGameFromSlot` in the file is `:62`, reached only from `LoadSettingsFromSlot()`, called only from `Initialize()` (`:29`). |
| `OnSettingsChanged` never fires on a same-value write | ✅ | **Structural, not promised:** `ApplyBoolSetting` (`:96-126`) is the only mutation path and returns early at `:98-112`; `BroadcastSettingChanged` (`:157-171`) is the only `.Broadcast` call site in the file. Both verified by reading the whole `.cpp`. |
| Load path never writes disk | ✅ | `:85-86` passes `bPersistToDisk = false`. |
| No `.ini` edit | ✅ | `Grep` over `Config/` for `GameUserSettingsClassName` and `SiegeSettings`: **zero hits.** (Scope note: this proves the feature added no config key; I have no Git access to diff `Config/` wholesale.) |
| M8 declaration verbatim | ✅ | Handoff §8; and in the code at `SiegeSettingsSubsystem.h:82-86` and `SiegeSettingsSaveGame.h:41-44`. |

### Engine claims I re-derived rather than accepted (RELAYED-DIAGNOSIS LAW)

- **`UGameInstanceSubsystem` is `UCLASS(Abstract, Within = GameInstance, MinimalAPI)`** — confirmed at `Engine/Public/Subsystems/GameInstanceSubsystem.h:15`, exactly the line cited. The tests' throwaway-`UGameInstance` outer (`SiegeSettingsTest.cpp:86-89`) is therefore **necessary, not defensive**, and without it the suite would assert at runtime rather than fail to build. Good catch by the programmer; I confirm it.
- **`UGameInstance` is instantiable** — `UCLASS(config=Game, transient, BlueprintType, Blueprintable, MinimalAPI)` at `GameInstance.h:150`. **Not abstract**, so `NewObject<UGameInstance>` at `SiegeSettingsTest.cpp:86` is legal. (This was the one thing that could have silently broken the whole suite; it holds.)
- **`GetTransientPackageAsObject()` exists** — `UObjectGlobals.h:276`, and it is the same object `NewObject`'s default outer uses (`:1958`).
- **File-scope `const FName` is safe against static-init order** — `FName`'s pool is lazily constructed on first use, not a static-init-order-dependent global: `UnrealNames.cpp:2223-2246` (`bNamePoolInitialized` guard, placement-new on first call). ⇒ `SiegeSettingsSubsystem.cpp:20` is safe. **I checked this because it is the project's first file-scope `const FName` definition** (every other one in `Siegebound/` is a function-local static — `Projectile.cpp:405-406`, `SiegePlayerController.cpp:1305,1332,1869,3028,3974`), so there was no green precedent to lean on.
- **Automation flags idiom matches the green precedent** — `EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter` is byte-identical to the already-compiling `SiegeAssistantZoneATest.cpp:301` / `SiegeAssistantGuardTest.cpp:152`.
- **No duplicate symbols** — one `DEFINE_LOG_CATEGORY(LogSiegeSettings)` in the module (`SiegeSettingsSubsystem.cpp:11`); one declaration of each of the three new classes.

### Findings

- **[WARN-436-1]** `SiegeSettingsSubsystem.h:165,175,184,187` — **four test seams sit on the SHIPPED public surface, unguarded.** `LoadSettingsFromSlot()`, `SetSlotNameForAutomationTests(const FString&)`, `SettingsChangeBroadcastCount`, `LastBroadcastSettingName`. Each is documented as not-a-gameplay-path and none has a caller outside `Initialize()` and the tests — I verified that by reading — but the *capability* to redirect the player's settings slot, hit the disk mid-match, or build logic on a diagnostics counter ships in retail. **Suggested fix (non-blocking, cheap):** wrap the two functions and the two counters in `#if WITH_DEV_AUTOMATION_TESTS` — `SiegeSettingsTest.cpp` is already inside that guard (`:13`), so nothing else moves. Deferring is also defensible; the point is that it be a decision.
- **[WARN-436-2]** `SiegeSettingsTest.cpp:364-375` — **the `true`-direction round-trip assertion has no discriminating power on its own.** `SecondReader`'s compiled default is already `true`, so `TestTrue("save(true) → load() returns true")` passes both when the load worked and when the load silently did nothing. (The `false` direction at `:336-355` *is* strong and does carry the test.) **Suggested fix:** call `SecondReader.Settings->SetAssistantConfirmEnabled(false)` before the load so `true` can only arrive from disk — one line, and it makes the second half of the test mean what its name says.
- **[NIT-436-1]** `SiegeSettingsSubsystem.h:152` — no `ShouldCreateSubsystem` override, so the subsystem is constructed (and performs its one disk read) in **every** `UGameInstance`, including a dedicated server and editor preview worlds, where a client-local review-step preference has no meaning. Harmless — one small file, once — recorded because M8 is networked and the pattern will be copied.
- **[NIT-436-2]** `SiegeSettingsSaveGame.cpp` — **ruled KEEP** (see §2, item 3). No action.

---

## 5. TASK-437 — findings

### PASS criteria confirmed (all by raw `Read`)

| Criterion | Result | Evidence |
|---|---|---|
| §8 pin, character-for-character | ✅ | `SettingsMenuWidget.h:121-122,130-131,145-146`. |
| All 8 pinned children present, exact names | ✅ | `SettingsMenuWidget.h:223-252` — `BackdropBorder` `RootPanel` `TitleText` `ConfirmToggleCheckBox` `ConfirmToggleLabelText` `ConfirmToggleHintText` `BackButton` `BackLabelText`. Constructed under the same `FName`s at `.cpp:111,149,164,187,198,237,260,270`. |
| Every child `BindWidgetOptional`, never `BindWidget` | ✅ | 8/8 at `SettingsMenuWidget.h:223-252`. Declaration is **character-identical to the shipped `USessionMenuWidget` idiom** (`SessionMenuWidget.h:126-147`), i.e. the compiling precedent. |
| Constructed only when null (escape hatch) | ✅ | Whole-tree early return `.cpp:101-106`; per-child `if (X == nullptr)` at `:109,147,162,185,196,235,258,268`. |
| `BackdropBorder` hit-test VISIBLE | ✅ | `.cpp:135`. See §2 item 4 for the ruling. |
| Seed-then-bind | ✅ | `.cpp:342-375` — remove delegate → read `IsAssistantConfirmEnabled()` → `ApplyConfirmValueToRow` → **then** `AddUniqueDynamic`. The engine claim behind it holds: `UCheckBox::SetIsChecked` (`CheckBox.cpp:188-203`) does **not** broadcast; only `SlateOnCheckStateChangedCallback` does (`CheckBox.cpp:278`). The cited lines `:188` / `:267` are exact. **No feedback loop.** |
| BIE params `FString`/`bool` only | ✅ | `SettingsMenuWidget.h:146` — `const FString&`, `bool`. No enum, no struct. |
| `BackPressed()` = `RemoveFromParent()` and nothing else | ✅ | `.cpp:406-416`. `RemoveFromViewport` (deprecated 5.1) is **not** used anywhere. |
| Null-safe throughout | ✅ | `ResolveSettingsSubsystem` null-safe at every hop (`.cpp:529-531`); all three call sites handle null (`:347-369`, `:420-429`, `:456-461`); `ShowRowUnavailable` log latched once per instance (`:496-501`). `BackButton` bound **unconditionally and last** (`:380-383`) — the panel is never a trap. |
| No disk I/O | ✅ | No `LoadGameFromSlot`/`SaveGameToSlot`/`.ini` anywhere in either file. |
| No `.uasset`, `/Game/UI/WBP_SettingsMenu` still reserved | ✅ | Referenced only in comments (`.h:63`, `.cpp:96`). |
| M8 declaration verbatim | ✅ | Handoff §? and in code at `SettingsMenuWidget.h:20-21`. |
| No `Build.cs` edit needed | ✅ | `UMG`, `Slate`, `SlateCore` already public deps (`GitClaudeUnrealTest.Build.cs:21-36`). |

### ⚠️ THE `NativeOnInitialized` TRAP (raised mid-review from TASK-444) — **CONFIRMED NEGATIVE, 437 IS CORRECT**

Ruled explicitly, because the coordinator asked for the verdict either way and this trap is now documented twice in one wave.

- **`USettingsMenuWidget` has NO `NativeOnInitialized` override.** `Grep` across `Siegebound/` returns three hits in `SettingsMenuWidget.cpp` — `:305`, `:308`, `:310` — and **all three are comment text explaining why it is absent.** The header declares only `RebuildWidget`, `NativeConstruct`, `NativeDestruct` (`SettingsMenuWidget.h:164-166`).
- **All wiring happens in `NativeConstruct()`** (`SettingsMenuWidget.cpp:301-315` → `SeedAndBind()` at `:324-384`). That covers **both** things the coordinator named: the check box's seed-then-bind off `IsAssistantConfirmEnabled()` (`:362-364`) and `BackButton->OnClicked` (`:380-383`). Unbind is symmetric in `NativeDestruct` (`:317-322`).
- **I re-derived the engine ordering rather than trusting the comment:** `NativeOnInitialized()` is called from inside `UUserWidget::Initialize()` at `UserWidget.cpp:175`; `NativeConstruct()` is called from `OnWidgetRebuilt()` at `UserWidget.cpp:1234`, i.e. **after** `RebuildWidget()` has run. ⇒ On a code-authored tree the children genuinely do not exist at `NativeOnInitialized` time, and binding there would no-op in total silence. **437 is on the right side of this.**
- 📌 **Worth recording for the next reader:** the shipped `USessionMenuWidget` **does** override `NativeOnInitialized` (`SessionMenuWidget.cpp:13`) — and that is correct *for it*, because it is WBP-backed and its `BindWidget` members are resolved inside `Initialize()` (`WidgetBlueprintGeneratedClass.cpp:275`). **Copying the shipped precedent here would have been the bug.** 437 did not copy it, and said why at `.cpp:305-313`. That is the trap closed by reasoning rather than by luck.

### Findings

- **[WARN-437-1]** `SettingsMenuWidget.cpp:80` / `:86-91` — **`ConstructSettingsTree()` runs BEFORE the engine's own defensive `Initialize()`, so on any path that reaches `RebuildWidget()` un-initialized the tree is never built.** `UUserWidget::RebuildWidget` self-heals with `if (!bInitialized) { Initialize(); }` (`UserWidget.cpp:1197-1200`), and `WidgetTree` is only allocated inside `Initialize()` (`UserWidget.cpp:160-163`). Our override runs first, finds `WidgetTree == nullptr`, logs an `Error` and returns — then `Super` initializes, finds `RootWidget` null, and returns an **`SSpacer`: the silently-empty panel.**
  **Why this is a WARN and not a BLOCKER:** the shipped path is `CreateWidget`, and `UUserWidget::CreateWidgetInstance` calls `NewWidget->Initialize()` at `UserWidget.cpp:2815` **before** anything can take the widget — so `WidgetTree` is non-null in every path TASK-438 can produce. The engine's defensive branch exists for blueprint-compiler in-memory replacement, which requires a `UWidgetBlueprintGeneratedClass` and therefore cannot apply to a class with no `.uasset`. And it degrades to a logged Error, never a crash.
  **Suggested fix — one line, free, and it removes the last route to a blank panel:** call `Initialize();` at the top of `USettingsMenuWidget::RebuildWidget()` before `ConstructSettingsTree()`. It is **public** on `UUserWidget` (`UserWidget.h:285` `public:` … `:297`) and **idempotent** (`UserWidget.cpp:138` guards on `bInitialized`).
- **[WARN-437-2]** `SettingsMenuWidget.cpp:55` — **the pinned token `bAssistantConfirmBeforeExecute` now exists as three independent literals** (SaveGame field `SiegeSettingsSaveGame.h:74`, subsystem constant `SiegeSettingsSubsystem.cpp:20`, widget constant here). Today they agree; the **only** control is the §8 pin, i.e. a document. 436's test asserts two of the three against the literal (`SiegeSettingsTest.cpp:126-128`) but **nothing checks the widget's copy.** A future divergence would be invisible — it surfaces only if someone later adds a name filter, at which point the row stops updating with no error.
  ⛔ **Do NOT "fix" this by initializing `ConfirmSettingName` from `USiegeSettingsSubsystem::SettingName_AssistantConfirmBeforeExecute`** — that is a cross-TU static-initialization-order fiasco and would be strictly worse than the duplication. **Suggested fix:** at the next touch of `SiegeSettingsTest.cpp`, add `TestEqual(TEXT("The widget's reported setting name matches the subsystem's"), FString(USettingsMenuWidget::ConfirmSettingName), USiegeSettingsSubsystem::SettingName_AssistantConfirmBeforeExecute.ToString());` to `Siegebound.Settings.SlotContract`. Out of scope for this gate; recorded so it is not lost.
- **[NIT-437-1]** `SettingsMenuWidget.cpp:390` — `UnbindAll()` resolves the subsystem through `GetWorld()`, which can be null during teardown, so `OnSettingsChanged` may not be explicitly unbound. **Verified harmless:** `DECLARE_DYNAMIC_MULTICAST_DELEGATE` stores weak object references, so a dead widget is skipped and compacted rather than called. The handoff asserts this; I confirm it. No dangling call, no GC leak.
- **[NIT-437-2]** `SettingsMenuWidget.cpp:162-177,235-252,258-266` — **the "add it to the parent" step is nested INSIDE each `if (Member == nullptr)` construction block.** A member that is already non-null while `RootWidget` is null would be constructed-never, parented-never — an orphan the tree owns and nothing shows. Unreachable today thanks to the whole-tree early return at `:101-106`, so this is a structural note, not a defect: **construction and parenting are two concerns and they are welded together.** If a future setting row is added, separate them.
- **[NIT-437-3]** `SettingsMenuWidget.cpp:503-511` — `ShowRowUnavailable()` has no re-enable path; once the row is disabled it stays disabled for the life of the widget instance even if the subsystem later resolves. Defensible (dismiss/reopen fixes it) and arguably the honest behaviour. Recorded only.
- **[NIT-437-4]** `SettingsMenuWidget.cpp:215-233` — the orphan-label adoption branch after a failed check-box construction is thoughtful but unreachable in practice (`WidgetTree->ConstructWidget` returns null only when the class is null, and it is a hard-coded `StaticClass()`). No action; it costs nothing.

---

## 6. Notes for build-master (TASK-447)

1. **This is a PASS on the CODE, not on the BUILD and not on the SCREEN.** Nothing here has been compiled. **TASK-447's compile is the first parser to see any of these seven files**, and a UHT/UBT failure at that gate is expected-normal, not a QA miss — route it back as a build failure per routing rule 6.
2. **The two tasks do not compile alone and must not be built separately.** `SettingsMenuWidget.cpp:15` includes `SiegeSettingsSubsystem.h` and uses `LogSiegeSettings`; that is the TASK-416/417 precedent and the board says so twice. Build the quiesced module once.
3. **No `Build.cs` change is required by either task** — `UMG`/`Slate`/`SlateCore`/`Engine`/`Core` already cover every symbol used (`GitClaudeUnrealTest.Build.cs:11-49`). If TASK-443 edits that file (it is 443's by ruling 10), nothing in 436/437 needs to move.
4. **First-run behaviour to expect in the log, and it is correct:** on a machine with no `Saved/SaveGames/SiegeSettings.sav`, `LogSiegeSettings` emits exactly one `Log`-level line — *"No readable settings in slot 'SiegeSettings' (user 0) — falling back to C++ defaults"* — followed by the `Subsystem initialized` line. **That is not an error and must not be reported as one.**
5. **The seven `Siegebound.Settings.*` tests are hermetic by construction** — every store is redirected to `"SiegeSettings_AutomationScratch"` before any I/O (`SiegeSettingsTest.cpp:92-95`) and an RAII guard deletes it in and out (`:60-64`). **A test run cannot touch Jonathan's real settings file.** If you run the suite, that guarantee is asserted by `Siegebound.Settings.SlotContract` itself (`:132-133`). ⚠️ **`Siegebound.Settings.*` has never executed** — treat first-run results as new information, not confirmation.
6. **For TASK-438, inside your session, after the compile:** the `CreateWidget` node must pick the **C++ class `USettingsMenuWidget`** — there is no `WBP_SettingsMenu` and there must not be one. `UCLASS()` on `SettingsMenuWidget.h:109-110` is concrete and not `Abstract`, so it *should* appear in the picker. **If it does not, the compile did not land — stop and report, do not author a WidgetBlueprint to work around it** (437 handoff §6 says the same; I concur and am repeating it because it is the one place this could go quietly wrong).
7. **Carry TASK-437 handoff §7 verbatim into TASK-448's checklist**, plus the label-wrap item I added in §3 above. ⛔ **Item 2 — the click-through over Quit — is the blocker criterion and no readback closes it.**
8. **Amendment owed to the manager**, not to you: CONVENTIONS `:1195` still says children are constructed *after* `Super::RebuildWidget()`. See §2 item 1.

---

## 7. Summary

Both tasks are **well above the bar**, and the two things that could have made this a FAIL — a guessed delegate-payload name, and `NativeOnInitialized` wiring on a code-authored tree — are **both absent, both by deliberate reasoning that the author wrote down, and both independently re-derived here from the installed UE 5.8 source rather than accepted from the handoffs.** No deprecated API is used in either task. No blocker found.

The four warnings are hardening and test-strength items, none of which can break the TASK-447 compile and none of which touches the shipped behaviour of the feature. **The open risk in this batch is not in the code — it is on the screen, and it is Jonathan's to close.**
