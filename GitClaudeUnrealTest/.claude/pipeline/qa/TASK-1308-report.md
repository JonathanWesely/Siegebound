Verdict: PASS — 0 BLOCKER · 5 WARN · 5 NIT

subject: TASK-1307
gate: TASK-1308 · marker `TASK-1308-FOCUS-READBACK-GATE` · reviewer: qa-reviewer · 2026-09-18
host: TASK-1309 (unblocked by this PASS; ⛔ clause (0)'s fold is the MANAGER's call, not mine and not the host's)
baseline for the subtractive check: the committed blob at `1d4ae90`, read out of `.git` (see §0b)
⛔ read-only row: no compile, no editor lifecycle call, no git write, no source edit.

---

## 0 · HOW THIS GATE GOT ITS EVIDENCE — declared first, because `SC-§71b` makes provenance part of the verdict

### 0a · WHAT I INSPECTED (and therefore what this PASS covers)

- **The running editor, READ-ONLY** (`execute_unreal_python_readonly`; ⛔ nothing written, no asset touched,
  ⛔ no lifecycle tool called). Measured on the live `WBP_DeckBuilder_C`:
  - `CDO.TickFrequency = WidgetTickFrequency.AUTO` — the precondition item (1)(b) turns on.
  - `CDO.bIsFocusable = True` — `TASK-1304`'s shipped asset state intact (corroboration, not this row's claim).
- **Engine source at every line I cite** (`C:\Program Files\Epic Games\UE_5.8\Engine\Source\...`), re-read
  rather than relayed (`SC-§40` cl. 1).
- **The committed bytes** of `DeckBuilderWidget.{h,cpp}` at `1d4ae90` **and** at `HEAD = 89752eb`.
- **The whole handoff** (`handoffs/TASK-1307-programmer.md`, 467 lines, `SC-§38a`) and all three board rows
  `TASK-1307` / `TASK-1308` / `TASK-1309`.

### 0b · ⚠️ THE ONE METHOD NOTE, STATED BECAUSE IT IS UNUSUAL AND BECAUSE IT IS THE ONLY WAY ITEM (5) IS ANSWERABLE FROM THIS SEAT

I hold **no git tool**, and the read-only Python sandbox **refuses `import subprocess`** (measured: *"Aura
disallows import of subprocess module"*). Item (5) requires committed bytes. So I read them **straight out of
the object store**: `.git/objects/1d/4ae90…` → `zlib.decompress` → commit → root tree
`265fa37b7461935bf8ce4ca99b2a2a1825eda75e` → tree walk → blob. Pure reads; **no git process ran, no index was
touched, nothing was written.** The same route gave me `HEAD`'s tree `ef52d822…`.

⭐ **AND THE FIRST THING IT PROVED IS THAT THE BASELINE IS THE RIGHT ONE:**
`DeckBuilderWidget.cpp` and `DeckBuilderWidget.h` are **byte-identical at `89752eb` and at `1d4ae90`**
(`blob(HEAD) == blob(1d4ae90)` → `True` for both). ⇒ `1cfee95` and `89752eb` really do carry no code on these
two paths, and **the file the programmer edited is exactly the file `DECK-§9` was measured from.**

### 0c · ⛔ WHAT I DID **NOT** INSPECT — so nothing in this PASS is read wider than it was earned

- ⛔ **I did not compile.** Compilability here is a **text-level** judgement: I checked the include chain, the
  override signatures against `UserWidget.h`, the member types, and the **format-specifier/argument parity by
  hand** (see §8). A compile finding is `TASK-1309`'s to make.
- ⛔ **I did not run the suite** and I did not read a runtime log. The row's own instrument is the runtime log
  at `TASK-1309`'s verify leg.
- ⛔ **`bClassRequiresNativeTick` on the live class could not be measured** — it is `protected` and the Python
  property API refused it (*"Property 'bClassRequiresNativeTick' … is protected and cannot be read"*). That one
  link in item (1)(b)'s chain is established **at source and by the engine's own documented semantics**, not
  on the asset. §1b says what that does and does not entitle us to.
- ⛔ **`Content/Blueprints/BP_MenuGameMode.uasset`'s committed bytes** are in a packfile, so I could not
  byte-diff it. §6 attributes it by surrounding content and by **absence** from this row's diff, and declares
  the limit rather than dressing it up.

### 0d · RESUMED-SESSION INTEGRITY — asked because the machine hard-shut-off mid-session, answered because a half-written file would be new information

⛔ **Nothing looks half-written.** Both source files are structurally complete: the three appended `.cpp`
functions each open and close (`NativeTick` 1820→1844, `NativeDestruct` 1846→1857, `LogPostFlushFocusReadback`
1859→1914) with their `Super::` calls present, and line 1916 resumes the pre-existing `LoadDefaultDeck`. The
five header additions are complete declarations. `TASKBOARD.md` reads **36,528 lines** in the working tree,
matching the figure I was given, and `DeckBuilderWidget.{h,cpp}` at `HEAD` are byte-identical to `1d4ae90`,
so **nothing was silently half-committed as the power went.** No finding here.

---

## 1 · 🚨⭐ ITEM (1) — THE CENTRAL GATE ITEM. **CAN THE NEW READ-BACK RETURN BOTH ANSWERS? ⭐ YES — AND THE OLD ONE'S STRUCTURAL IMPOSSIBILITY IS REMOVED, NOT RELOCATED.**

### 1a · (a) WHERE the deferred read is scheduled · (b) WHAT runs it and WHEN · (c) WHAT it compares

**(a) SCHEDULED — `DeckBuilderWidget.cpp:1550-1551`:**
```cpp
bFocusReadbackPending = true;
FocusRequestFrameCounter = GFrameCounter;
```
Armed **inside `AcquireBuilderFocus`, AFTER its three early returns** (`:1489` no Slate, `:1503` no cached
widget / no local player) — so a session that cannot take focus leaves it **disarmed** and the absence of a
post-flush line there is *correct*, not silent failure. Consumed at `:1835-1842`.

**(b) THE MECHANISM, AND THE PHASE ORDER RE-READ AT ENGINE SOURCE (⛔ I did not accept the handoff's table —
`LaunchEngineLoop.cpp`, `Runtime/Launch/Private`):**

| phase | line | what I read there |
|---|---|---|
| (a) world tick | **:5859** | `GEngine->Tick(FApp::GetDeltaTime(), bIdleMode);` — ⇒ `FTimerManager`, i.e. where `SetTimerForNextTick` lands |
| (b) ⭐ **THE FOCUS FLUSH** | **:5918** | `ProcessLocalPlayerSlateOperations();` under `SCOPED_NAMED_EVENT(ProcessLocalPlayerSlateOperations…)`, comment *"Process slate operations accumulated in the world ticks."* (definition at **:5231**) |
| (c) input | **:5921** | `FSlateApplication::Get().Tick(ESlateTickType::PlatformAndInput);` — a mouse click on the menu button runs here |
| (d) ⭐ **WIDGET TICK** | **:5991** | `FSlateApplication::Get().Tick(bRenderingSuspended ? ESlateTickType::Time : ESlateTickType::TimeAndWidgets);` |
| (e) counter | **:6131** | `GFrameCounter++;` — comment *"Increment global frame counter. Once for each engine tick."* |

⇒ **(b) precedes both (c) and (d) inside one iteration, and (e) is after all four.** The route into the
widget: `SObjectWidget::Tick` (`Runtime/UMG/Private/Slate/SObjectWidget.cpp:114`) → `if (CanRouteEvent())`
(`:126`) → `WidgetObject->NativeTick(AllottedGeometry, InDeltaTime)` (`:128`) → the new override at
`DeckBuilderWidget.cpp:1820`.

⭐ **THE GUARD IS RIGHT, AND `>` RATHER THAN `>=` IS LOAD-BEARING** (`cpp:1835`,
`if (bFocusReadbackPending && GFrameCounter > FocusRequestFrameCounter)`): in the route a human actually
produces — the builder opened from a Slate click at **(c) of frame N** — the focus op is queued *after* frame
N's flush, so **(d) of frame N is still pre-flush**. With `>=` the read would fire there and the row would have
shipped its own defect one phase along. With `>`, the first eligible tick is **(d) of frame N+1, whose (b) has
already run.** In the other route (construct from the world tick at (a)) the flush is same-frame and `>` merely
reports one frame late — conservative in the safe direction. ⭐ **The programmer rejected the row's own first
suggestion (`SetTimerForNextTick`) for the right reason: it fires at (a) of N+1, i.e. BEFORE (b) of N+1.**
I confirm that from the same lines.

**(b) — THE PRECONDITION, AND THE LINE THAT ESTABLISHES IT (`SC-§123` cl. 3: *"the engine calls this"* is not evidence):**
Slate routes a widget tick **only** while the `SWidget` carries `EWidgetUpdateFlags::NeedsTick`
(`SlateCore/Public/Widgets/SWidget.h:677-678`, `SetCanTick`/`GetCanTick`). That flag is set by
**`SafeGCWidget->SetCanTick(bCanTick)` at `UMG/Private/UserWidget.cpp:2379`**, inside
`UUserWidget::UpdateCanTick()` (`:2347`), which is called **from the `SObjectWidget` constructor itself**
(`SObjectWidget.cpp:36`) and again right after `Construct()` (`UserWidget.cpp:1915-1916`). `bCanTick` needs:
- `TickFrequency == EWidgetTickFrequency::Auto` (`:2356`) — ⭐ **MEASURED `AUTO` on the live CDO**; and
- one disjunct of `:2360-2364`; the operative one is **`WidgetBPClass->ClassRequiresNativeTick()` (`:2360`)** —
  the serialized `UPROPERTY() uint32 bClassRequiresNativeTick:1` (`WidgetBlueprintGeneratedClass.h:99-100`,
  accessor `:193`), written at **widget-blueprint compile time** (`WidgetBlueprintCompiler.cpp:675-676`) from
  **`!NativeParent->HasMetaData("DisableNativeTick")` (`UMGEditor/Private/WidgetBlueprint.cpp:1561-1563`)**.
  `UUserWidget` itself carries `meta=(… DisableNativeTick)` (`UserWidget.h:279`); **`UDeckBuilderWidget` is a
  bare `UCLASS()` (`DeckBuilderWidget.h:45`, `: public UUserWidget` at `:46`) and carries no such meta.**
  The designed semantics is stated twice in the engine: `UserWidget.h:122-127` (*"If the widget inherits from
  something other than UserWidget it will also tick so that native C++ or inherited ticks function"*) and the
  compiler's own comment at `WidgetBlueprint.cpp:1607` (*"the generated class is not a direct child of
  UUserWidget (means it could have a native tick) then it will definitely tick"*).

⭐ **A CONSEQUENCE WORTH SPELLING OUT, because it kills a trap nobody boarded:** that flag does **not** depend
on whether `NativeTick` is overridden — it depends only on the native parent. ⇒ **this widget was already
ticking before the diff**, `WBP_DeckBuilder.uasset` does **not** need recompiling for the instrument to fire,
and the override adds one virtual dispatch plus one branch while the builder is open and **nothing** once it
has fired. Not a per-tick performance finding.

**(c) THE COMPARATOR — ⭐ POINTER IDENTITY, not a type name.** `cpp:1894`:
```cpp
const bool bIsThisBuilder = Focused.IsValid() && Focused == SelfSlate;
```
`TSharedPtr<SWidget>` vs `TSharedPtr<SWidget>` ⇒ raw-pointer comparison — **the same predicate the untouched
return at `cpp:1613` has always made**, now made at a moment where it can be true. The two type names are
separate `%s` arguments and the line's own text says they are context.

### 1b · ⭐ BOTH ANSWERS ARE REACHABLE — the finding this row exists to produce

- ⭐ **`MATCH` IS REACHABLE.** Three independent things have to line up and all three are established:
  (i) the read happens after `:5918` of a later frame (§1a); (ii) `SetUserFocus`'s leaf→root walk must stop
  **on** this builder rather than an ancestor, which needs `bIsFocusable` — ⭐ **MEASURED `True` on the live
  CDO**; (iii) the state itself must occur — ⭐ `TASK-1306`'s post-flush `ui_snapshot` read
  `WBP_DeckBuilder_C_0 "focused": true` **three times on the very run where the old line printed `'<none>'`**.
  ⇒ the new line samples a state that **demonstrably happens**.
- **`NO-MATCH` is reachable** — focus on an ancestor (exactly the `TASK-1286` failure) or on any other widget.
- **`IDENTITY=UNREADABLE` is a distinguishable third answer** (`cpp:1876-1882`) for "I could not look".

⇒ ⛔ **The old instrument's defect — structurally incapable of ever printing a positive — is REMOVED. The row
did not reproduce it.** ⭐ And the pessimistic direction is closed at the **old** line too: the retained
construct-time line now says in its own text that a `NO-MATCH` there is **EXPECTED**.

⚠️ **THE ONE DECLARED LIMIT ON THIS FINDING:** the `bClassRequiresNativeTick` link is source-established, not
measured (§0c). ⛔ **It cannot manufacture a false pass.** If it were false the widget would not tick and
**zero** post-flush lines would print — and both the handoff (§5) and `TASK-1309` (4) declare a PRE-FLUSH-only
log **NOT a pass**. The failure mode of a wrong precondition here is a visible absence, not a green light.

---

## 2 · ITEM (2) — THE ONCE-PER-OPEN GUARD AND THE TEARDOWN PATH, BOTH READ AT SOURCE

**(a) CAN THE NEW LINE FIRE MORE THAN ONCE PER OPEN? ⛔ NO.** Full executable census of the two members
(comments excluded), so this is a state assertion rather than an impression:

| symbol | `.h` | `.cpp` | reading |
|---|---|---|---|
| `bFocusReadbackPending` | `826` (decl, `bool … = false`) | `1550` arm · `1835` guard · `1841` **clear** · `1854` disarm | 4 sites, all accounted for |
| `FocusRequestFrameCounter` | `837` (decl, `uint64 … = 0`) | `1551` stamp · `1835` guard · `1908` printed delta | the delta is diagnostic only |

- ⭐ **Cleared at `:1841`, on the line BEFORE the call at `:1842`** — so exactly one line per open **whatever
  the read-back does**, including each of its own early returns. Clearing after would have let a widget that
  momentarily lost its local player re-enter on the next frame. Right call, right order.
- ⭐ **PER CONSTRUCT, NOT PER CLASS.** It is a plain per-instance member — **`static` appears on these two
  members 0 times** (the `static` hits in the file are `static_cast` and pre-existing statics). It is armed
  inside `AcquireBuilderFocus`, whose **only call site I measured myself**: `cpp:670`, the last statement
  before the enclosing function's `}` at `:671` — one call site, no other. ⇒ it **re-arms on every open of the
  same instance**, which is exactly what spec (3)(i) demanded.

**(b) CAN A DEFERRED CALLBACK OUTLIVE THE WIDGET? ⛔ NO — AND IT IS STRUCTURAL, NOT CLEANED UP AFTER.**
⭐ **`SetTimer` occurrences in `DeckBuilderWidget.h` and `.cpp` = 0.** Choosing `NativeTick` over a timer means
there is **no handle and no lambda in existence** — nothing that *could* dangle into a destroyed `UUserWidget`.
The crash class the row warned about is removed by construction, which is strictly better than clearing a
handle. `NativeDestruct` (`cpp:1846`) still exists for a **different** reason, and it is the right one: a
`UUserWidget` is **reused** across `RemoveFromParent`/`AddToViewport`, so a builder torn down before its first
post-flush tick would otherwise carry a **stale armed flag into its next open** and print a read-back that open
never requested. Disarm at `:1854`, `Super::NativeDestruct()` at `:1856` (last — correct: the disarm must
precede base teardown). `NativeTick` calls `Super::NativeTick` **first** (`:1822`), so
`UUserWidget::NativeTick`'s `ensureMsgf(TickFrequency != Never …)` (`UserWidget.cpp:2104`) and the script tick
still run exactly as before.

---

## 3 · ⭐ ITEM (3) — THE `DECK-§9` FENCE. **CENSUS RE-RUN BY ME: ⛔ 20 BOUND EXPRESSIONS ON ⛔ 7 EXECUTABLE LINES. UNCHANGED.**

⛔ **I did not accept the handoff's number** (`SC-§40` cl. 1). Derived from the working-tree bytes, counting
`EKeys::` occurrences on executable lines inside `NavigationFromKey` + `HandleCardGridKey`:

| worktree line | exprs | keys |
|---|---|---|
| **`cpp:1225`** | 3 | `Left` · `Gamepad_DPad_Left` · `Gamepad_LeftStick_Left` |
| **`cpp:1229`** | 3 | `Right` · `Gamepad_DPad_Right` · `Gamepad_LeftStick_Right` |
| **`cpp:1233`** | 3 | `Up` · `Gamepad_DPad_Up` · `Gamepad_LeftStick_Up` |
| **`cpp:1237`** | 3 | `Down` · `Gamepad_DPad_Down` · `Gamepad_LeftStick_Down` |
| **`cpp:1639`** | 3 | `Enter` · `Virtual_Gamepad_Accept.GetVirtualKey()` · `Gamepad_FaceButton_Bottom` |
| **`cpp:1663`** | 2 | `Delete` · `Gamepad_FaceButton_Left` |
| **`cpp:1727`** | 3 | `Escape` · `Gamepad_FaceButton_Right` · `Virtual_Gamepad_Back.GetVirtualKey()` |
| **TOTAL** | ⭐ **20 on 7 lines** | matches `DECK-§9` cl. 2 (`CONVENTIONS.md:6974`) exactly, per-line and in total |

⭐ **AND A STRONGER ASSERTION THAN THE TALLY (`SC-§104`): the seven key lines are TEXT-IDENTICAL to their
committed form at `1d4ae90`** — I compared the extracted key-line text lists and they are equal, in order.
The committed addresses were `1225 · 1229 · 1233 · 1237 · 1585 · 1609 · 1673`, i.e. **the exact seven
addresses `DECK-§9` cl. 2 names**, with `(3)(3)(3)(3)(3)(2)(3)` matching cell for cell.
`EKeys::Tab` = **0 committed, 0 now**. No letter and no digit was added anywhere.
⇒ ⛔ **The key table did not move. No BLOCKER.**

---

## 4 · ITEM (4) — THE RIDER IS COMMENT-ONLY: **PROVEN, NOT ASSUMED** (and the contract conflict, ruled)

**MEASURED on the `.h` diff vs `1d4ae90`:** `committed 910 lines → worktree 1013`; **1 removed line** (a
comment: `//  digits/punctuation/Enter/Escape BY DESIGN, CONVENTIONS:2519-2522); ⛔ Tab`), **104 added** of
which **99 comment/blank** and **exactly 5 executable**:

```
virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;   h:730
virtual void NativeDestruct() override;                                             h:744
bool   bFocusReadbackPending = false;                                               h:826
uint64 FocusRequestFrameCounter = 0;                                                h:837
void   LogPostFlushFocusReadback();                                                 h:855
```

⭐ **All five sit OUTSIDE the rider's hunk.** The rider is `h:227-233`; the declarations are at
`h:730/744/826/837/855`. ⇒ **the rider itself contributes 0 executable lines — the clause is satisfied on the
substance.**

✅ **And the citation shape is exactly what the clause asked for, re-measured by me rather than relayed:**
`### KBD-§4. SCOPE LAW …` is **`CONVENTIONS.md:2525`**, and the claim the comment makes is the **second bullet
at `:2528`** — *"⛔ DELIBERATELY NOT REMAPPED: digits (the `1`–`6` hotkeys), punctuation, modifiers …,
`Space`, `Enter`, `Escape`."* — **one line, not the four-line range the old citation named.** The new comment
cites the **section**, with the line as corroboration, and says so in its own text. ⇒ ⛔ **not a bare new line
number ⇒ no WARN on that half of the clause.**

⚠️ **THE CONFLICT, RULED RATHER THAN BURIED — see `WARN-3`.** This item's second sentence
(*"`DeckBuilderWidget.h`'s diff must contain ZERO executable lines"*) and `TASK-1307` spec (3)(i)/(ii)
(*"guard it with a member flag"*, *"clear it in `NativeDestruct`"*) **cannot both be satisfied by any route** —
a member and an override are undeclarable without touching the header. `TASK-1307`'s own **Acceptance (6)**
scopes the phrase — *"`(4)`'s rider declared as comment-only"* — and that is the reading I apply. The
programmer **raised it instead of quietly deviating** (handoff §10 item 1), which is the behaviour the register
laws exist to reward. ⇒ **not a BLOCKER**: a FAIL here would force a re-cut that provably cannot succeed, which
is the expensive error, not the safe one.

---

## 5 · ITEM (5) — BEHAVIOUR UNCHANGED: THE SUBTRACTIVE CHECK, **AGAINST THE COMMITTED BYTES, BY NAME**

**The whole `.cpp` delta vs `1d4ae90`:** `committed 2558 lines → worktree 2720`; **163 added** (102
comment/blank, **61 executable**) and ⭐ **exactly ONE removed line** — the old log's format string:

```
TEXT("UDeckBuilderWidget::AcquireBuilderFocus: focus %s this frame; focused widget is '%s' (this builder's Slate widget is '%s'); IsFocusable()=%s."),
```

⭐ **The 61 executable additions account for themselves completely: 11 inside `AcquireBuilderFocus`'s log block
(2 arm lines, 6 `TEXT()` format lines, the `MATCH`/`NO-MATCH` argument, 2 pointer `static_cast`s) and 50 in the
three functions appended after `NativeOnKeyDown`.** Since a function body can only change through an added or a
removed line, and every add and the single removal are accounted for outside them, the named sites are
**byte-identical**, not merely "look the same":

| named site | `file:line` (worktree) | verdict |
|---|---|---|
| `SetUserFocus` (the real focus call) | `cpp:1520` | ⛔ **UNCHANGED** |
| the deferred `GetSlateOperations().SetUserFocus` | `cpp:1537` | ⛔ **UNCHANGED** |
| `EFocusCause::SetDirectly` | `cpp:1520` · `cpp:1537` | ⛔ **UNCHANGED** |
| `CancelFocusRequest` | `cpp:1524` | ⛔ **UNCHANGED** |
| `return bTookFocus && Focused == SelfSlate;` | `cpp:1613` | ⛔ **UNCHANGED — byte for byte** |
| `SetFocusedCardIndex` · `IsCardGridFocusLive` · `ExitCardGridFocus` | — | ⛔ **UNCHANGED** (no add/removal falls inside them) |
| `NativeOnPreviewKeyDown` · `NativeOnKeyDown` (the two-pass fence) | `cpp:1769` · `cpp:1791` | ⛔ **UNCHANGED** |

⭐ **Nothing the row added calls a focus-SETTING API.** `LogPostFlushFocusReadback` calls only
`GetUserFocusedWidget` — a read — plus `GetCachedWidget`, `GetOwningPlayer`, `GetLocalPlayer`,
`GetUserIndexForController`, `GetTypeAsString`, `IsFocusable`. ⇒ **instrument-only. No behavioural delta. No
BLOCKER.**

---

## 6 · ITEM (6) — SCOPE, AND EVERY OTHER DIRTY PATH ATTRIBUTED **BY CONTENT** (`SC-§59` cl. 2(iii))

**This row's diff is exactly two source files + its handoff.** In it: **0 `.uasset`, 0 test file, 0
`Saved/**`, 0 board or law file.** ⇒ no BLOCKER on scope.

⛔ **The rest of the tree is other rows' work, and I am saying so on content I read, not on a relay:**

| dirty path | what I measured in it | whose |
|---|---|---|
| `Tools/run_suite_bounded.ps1` | **+96 / −0**; changed lines carry `TASK-1294` ×3 and are the `-DisablePlugins=Aura` block (*"THIS IS THE FIX, NOT A FALLBACK"*, the boot-time Aura client defect) | ⭐ **`TASK-1294`** — and `qa/TASK-1295-report.md` line 5 reads `subject: TASK-1294`, `Verdict: PASS` |
| **`Source/.../Siegebound/Tests/SiegeMenuInputTest.cpp`** | **0 added / 15 removed**; the removed block is the standing-engine-error declaration about `BP_MenuGameMode` BeginPlay → `SetInputMode_UIOnlyEx(WidgetToFocus = WBP_MainMenu)` → *"Attempting to focus Non-Focusable widget SObjectWidget [Widget.cpp(976)]"*, citing `TASK-1274`/`1280`/`1281` | ⭐ **`TASK-1296`** — `qa/TASK-1297-report.md` line 5 reads `subject: TASK-1296`, `Verdict: PASS` |
| `Content/Blueprints/BP_MenuGameMode.uasset` | binary, and its committed blob is **packed** ⇒ ⛔ I could not byte-diff it. Attributed by the surrounding content (it is the exact asset the removed block above describes) and by **absence** from this row's diff | **`TASK-1296`** — declared as an attribution, with its limit |
| `.claude/pipeline/TASKBOARD.md` | **6 added / 6 removed**, and every added line is a `- status:` or `- blocked-by:` line for `TASK-1294`/`1295`/`1296`/`1297` plus **`TASK-1307`'s own `ready-for-qa`**. ⛔ No spec text, no law text, no other row's row | the two Lane-C gates' flips + this subject's own |
| untracked `handoffs/TASK-1294-…`, `TASK-1296-…`, `qa/TASK-1295-report.md`, `qa/TASK-1297-report.md` | the two reports' own `subject:` lines name `TASK-1294` and `TASK-1296` | Lane C |
| untracked `handoffs/TASK-1307-programmer.md` | this row's declared handoff | ⭐ **this subject** |

⭐ ⛔ **`SiegeMenuInputTest.cpp` IS A TEST FILE AND IT IS DIRTY, AND IT IS EMPHATICALLY NOT THIS ROW'S DEFECT.**
A mechanical reading of item (6) (*"any test file ⇒ BLOCKER"*) would fail `TASK-1307` for a file it never
opened: `TASK-1307`'s diff contains **zero** references to `BP_MenuGameMode`, `SetInputMode`, `WBP_MainMenu` or
this test, and the dirt there is a **removal** of a comment block about a menu-mode engine error — a subject
matter this row does not touch. **`SC-§59` cl. 5: a wrong FAIL is the expensive error.** It belongs to
`TASK-1296`, already `qa-passed` by gate `TASK-1297`.

---

## 7 · ITEM (7) — NO TEST IS EXPECTED, AND **THE REASON IS PRESENT AND IS A MEASUREMENT**

The handoff (§8) states it at the line, and I verified the line: `AcquireBuilderFocus` returns early at
```cpp
if (!FSlateApplication::IsInitialized()) { return false; // offline automation lane: there is no Slate focus to take
```
**`cpp:1489`** (pre-existing, untouched), and the **same guard is repeated at `cpp:1864`** inside the new
function. ⇒ in the offline `-nullrhi` lane the arm at `cpp:1550` is **never reached**, there is no Slate focus
to read and **no `ProcessLocalPlayerSlateOperations` flush to be after** — so a test written there would assert
the **early-return path** and be **green on the bug**, green before the diff and green after it.
⇒ **reason stated ⇒ no `SC-§70` WARN**, and **no test asserts the early-return path ⇒ no BLOCKER**.
⭐ **And the handoff declined to add a direct-invocation test *on the right ground*:** a test that calls
`LogPostFlushFocusReadback()` itself **chooses the frame**, so it cannot fail in the way the bug failed. That
is `SC-§123` applied to the reviewer's own convenience, and it is the correct refusal. If anyone wants the
partial guard-arithmetic test, it is **its own row**.
⛔ Zero test files added, removed or edited by this row. Suite delta expected **0** against **561** —
`TASK-1309`'s to measure, by name (`SC-§104`), not mine to claim.

---

## 8 · THE COMPILE-RISK PASS I OWE, SINCE I CANNOT COMPILE

- ⭐ **`GFrameCounter` IS IN SCOPE.** Declared `extern CORE_API uint64 GFrameCounter;` at
  `Core/Public/CoreGlobals.h:532`, and `CoreGlobals.h` is included by `CoreMinimal.h:97`, which
  `DeckBuilderWidget.h:5` includes. ⛔ Worth checking rather than assuming: this is the project's **first**
  use of `GFrameCounter` (census = 0 elsewhere in `Source/`), so nothing else was holding the include in.
- ⭐ **FORMAT-SPECIFIER / ARGUMENT PARITY, counted by hand — a mismatch is a runtime garbage read, not a
  compile error:** PRE-FLUSH `cpp:1598-1611` = **7 specifiers / 7 args** (`%s`, `%s`, `%016llX`, `%s`,
  `%016llX`, `%s`, `%s`); POST-FLUSH `cpp:1900-1913` = **7 / 7** (`%s`, `%d`, `%016llX`, `%s`, `%016llX`,
  `%s`, `%s`); UNREADABLE `cpp:1876-1881` = **2 / 2**. ✅ All three balance, and each `%016llX` is fed a
  `static_cast<uint64>` and each `%d` a `static_cast<int32>`.
- **Override signatures match `UserWidget.h:1585-1586`** (`NativeDestruct()`,
  `NativeTick(const FGeometry&, float)`); `FGeometry` was already in the header's scope via the existing
  `NativeOnKeyDown`/`NativeOnPreviewKeyDown` declarations.
- **No deprecated or removed API.** Every engine call in the added code (`FSlateApplication::IsInitialized`,
  `::Get`, `GetUserIndexForController`, `GetUserFocusedWidget`, `GetCachedWidget`, `GetOwningPlayer`,
  `GetLocalPlayer`, `IsFocusable`, `GetTypeAsString`) **already appears in the committed file that compiled at
  `1d4ae90` with 0 `C4996`**; `GFrameCounter` and `UPTRINT` are not deprecated. ⇒ I expect **no new `C4996`**,
  which is `TASK-1309`'s (2) baseline.
- **GC safety:** the two new members are a plain `bool` and a `uint64` — **no `UPROPERTY` needed and correctly
  absent** (no `UObject` pointer, no serialization, no Blueprint surface; the `bWarnedMissingTileClass` shape
  one field above). No new `TObjectPtr`, no delegate, no timer.
- **Null/validity safety in the new function:** `IsInitialized()` before `Get()`; `PC ? PC->GetLocalPlayer() :
  nullptr`; `!SelfSlate.IsValid() || LocalPlayer == nullptr` → labelled early return; `Focused.IsValid()`
  guarded before every `Focused->` dereference; `SelfSlate->` only after its validity check. ✅ Clean.
- **Token separability, checked as strings rather than trusted:** `"IDENTITY=NO-MATCH"` does **not** contain
  `"IDENTITY=MATCH"` (the character before `MATCH` is `-`, not `=`), and `"IDENTITY=UNREADABLE"` contains
  neither. The PRE-FLUSH line contains the substring `POST-FLUSH` **zero** times. ⭐ The hyphen in `NO-MATCH`
  is the one deliberate deviation from the row's `MATCH`/`NO MATCH` wording and it is **an improvement** — with
  a space, every grep for the pass would also hit every fail. ⚠️ But see **`WARN-1`**.

---

## 9 · FINDINGS

### BLOCKERS — ⛔ none.

### WARN

- **[WARN-1]** `DeckBuilderWidget.cpp:1599` + `:1901` — ⛔ **`TASK-1309` MUST PIN THE FULL TOKEN AND MUST NOT
  SHORTEN IT.** The **PRE-FLUSH** line can legitimately print **`IDENTITY=MATCH`**: `SetUserFocus` returns
  `false` when focus has **not changed** (`SlateApplication.cpp:3029`, *"Is we aren't changing focus then simply
  return"*), so on a re-entrant / already-focused open the pre-flush `Focused == SelfSlate` is **true** and that
  line reads `PRE-FLUSH read-back: IDENTITY=MATCH … SetUserFocus returned DEFERRED`. ⇒ a grep for the bare
  substring `IDENTITY=MATCH` **can be satisfied by the non-answering line**. The handoff §5 and `TASK-1309` (4)
  both already write the full token `POST-FLUSH read-back: IDENTITY=MATCH`, which is safe — **this is a
  do-not-abbreviate warning to the host**, because the abbreviation is exactly the kind of convenience that
  re-creates this row's disease one level up. *Suggested fix: none in code; host pins the full string.*
- **[WARN-2]** `DeckBuilderWidget.cpp:1894` + `:1907` — ⛔ **`NO-MATCH` CANNOT DISTINGUISH "AN ANCESTOR HAS
  FOCUS" (the `TASK-1286` defect) FROM "A DESCENDANT OF THIS BUILDER HAS FOCUS" (a card tile or a deck-bar
  button — a HEALTHY state that `SetFocusedCardIndex`/`ExitCardGridFocus` deliberately produce).** The line
  prints the focused widget's address and type but no name, path, or ancestry test. This matters **downstream**:
  `TASK-1309` (4) rules that a `NO-MATCH` on a run where `ui_snapshot` also shows `WBP_DeckBuilder_C_0`
  `"focused": true` is a `VERIFY-FAILED` — but a **descendant-focused** run can produce exactly that pair
  **without the instrument lying**, because this class's own `IsCardGridFocusLive` already treats
  `HasAnyUserFocus() || HasFocusedDescendants()` as distinct states (`cpp:1217`). Non-blocking: in the
  one-frame window between the flush (`:5918`) and the widget tick (`:5991`) nothing in this class moves focus
  to a child, and the first post-flush tick is the frame after construct. *Suggested fix (a future row, not
  this one): add a third labelled field — `DESCENDANT=true/false` from `SelfSlate->HasFocusedDescendants()` —
  so the three-way outcome is readable from the line alone.*
- **[WARN-3]** `TASK-1308` spec item (4) vs `TASK-1307` spec (3)(i)/(ii) and the `names:` parenthetical
  *"(⛔ comment-only, rider (4))"* — ⛔ **A CONTRACT CONFLICT WITH NO SATISFYING ROUTE, AND IT IS THE
  MANAGER'S TO REWORD.** Item (4)'s literal *"the `.h` diff must contain ZERO executable lines"* is
  incompatible with spec (3)'s binding demand for a member flag and a `NativeDestruct` override. I ruled on the
  substance via `TASK-1307` Acceptance (6) (*"`(4)`'s rider declared as comment-only"*) and **measured** the
  separation: 5 executable header lines, all spec-(3) machinery, **all outside the rider hunk**, rider
  contribution **0**. ⇒ PASS. *Suggested fix: reword item (4) as "the RIDER's hunk must contain zero executable
  lines" and the `names:` parenthetical as "comment-only apart from the spec-(3) declarations".*
- **[WARN-4]** `CONVENTIONS.md:6963-6974` (`DECK-§9` cl. 1 table + cl. 2 census) — ⛔ **ELEVEN SITE CELLS ARE
  NOW STALE BY `+54`.** MEASURED by me, not relayed: `:1562`→`1616` · `:1585`→`1639` · `:1609`→`1663` ·
  `:1673`→`1727` · `:1683`→`1737` · `:1697`→`1751` · `:1706`→`1760` · `:1715`→`1769` · `:1737`→`1791`
  (`:1224`, `:1225`–`:1237` unmoved). ⭐ **The law's SUBSTANCE is intact** — the census is still `20/7` and the
  key lines are byte-identical — so this is address drift, not a `DECK-§9` violation. ⛔ **I may not edit
  `CONVENTIONS.md`**, and neither may the programmer or the host; the manager owns it.
  ⭐ **And note the irony the row earned: the diff whose rider fixed a drifted line citation moved eleven line
  citations of its own, without touching a single thing they describe.** The durable fix is `DECK-§9`'s Site
  column citing the **function** plus the line, the way rider (4) now cites the `§` plus the line.
- **[WARN-5]** `DeckBuilderWidget.cpp:1613` — **the untouched `return bTookFocus && Focused == SelfSlate;` has
  a false-negative of its own, in the same family as the bug this row fixes.** By
  `SlateApplication.cpp:3029`, an already-focused open returns `bTookFocus == false` **while focus is exactly
  where it belongs**, so the function returns `false` on a healthy re-entrant open. ⛔ **Harmless today and
  ⛔ explicitly NOT this row's to fix** (spec (5) forbids touching that expression, and the return value is
  **discarded** at its only call site, `cpp:670`). Recorded so that the next row that decides to branch on
  that return knows it is reading a weaker predicate than the new log line prints. *Suggested fix: if a caller
  ever needs it, `return Focused == SelfSlate || bTookFocus;` — its own row.*

### NIT

- **[NIT-1]** handoff §2 + `DeckBuilderWidget.h:845-846` cite `Widget.cpp:975` / `:980` for
  `SNew(SObjectWidget, Widget)`. The `SNew` lines are **`:976` and `:981`**; `975`/`980` are the
  `return TakeWidget_Private([]…` lines that open the lambdas. The claim is **confirmed**; only the address is
  off by one — and it is corroborated from an unrelated direction: the engine's own error text quoted in
  `SiegeMenuInputTest.cpp` prints `[Widget.cpp(976)]`. ⚠️ Same class of defect as the rider that row fixes.
- **[NIT-2]** `SWidget.cpp:1116-1119` is cited for `GetTypeAsString`; the function spans `:1116-1119` with the
  body at `:1118` (`return this->TypeOfWidget.ToString();`). Immaterial, listed only so the next reader does
  not re-derive it.
- **[NIT-3]** Raw `SWidget` addresses (`0x%016llX`) ship in a `Log`-verbosity line at four sites
  (`cpp:1607`, `:1609`, `:1909`, `:1911`). The handoff invited a ruling. ⛔ **I found no house rule against
  it, and I would KEEP them:** when both type names are the constant `"SObjectWidget"`, the two addresses are
  the only thing in the line that lets a human tell "an ancestor" from "a sibling" from "the same widget" — for
  a focus instrument that is the difference between a clue and a shrug. They are labelled context and the
  verdict does not depend on them.
- **[NIT-4]** A healthy open now emits **two** lines containing the substring `AcquireBuilderFocus`, not one.
  Any comparison against `TASK-1306`'s raw `total_matches` baseline must expect **×2**; the answering line is
  the one carrying `POST-FLUSH`.
- **[NIT-5]** `TASK-1309` (4) requires the post-flush line **exactly once**. That is correct **per builder
  open** — two opens in one PIE session legitimately produce two lines. Worth one sentence in the verify
  report so a second, correct line is not read as a guard failure.

---

## 10 · NOTES FOR BUILD-MASTER (`TASK-1309`)

1. ⛔ **CLAUSE (0) FIRST, AND I AM DELIBERATELY NOT RULING ON IT.** At the moment I write this, `TASK-1294`
   and `TASK-1296` are both `qa-passed` and `TASK-1298` has not run — i.e. **clause (0)'s merge test reads
   TRUE** — so on its own words this row **hands back to the manager** rather than executing. ⛔ **Whether
   `TASK-1307` folds into `TASK-1298`'s single compile wave is a MANAGER board edit (`SC-§100`), not this
   gate's and not the host's.** Run the test at **your** instant and report the branch on the evidence you
   took it on.
2. **Compile lane:** C++ only, and ⛔ **Live Coding cannot carry it** — the diff adds two new virtual overrides
   and two new members. Graceful-quit lane, then relaunch. ⚠️ The editor was up as **PID 2572** (GUI, ours) on
   the `1d4ae90` binaries when I inspected; **re-census by command line, twice, `SC-§118` cl. 8** — the field
   refills itself and the MCP bridge spawns a headless editor when the GUI editor closes.
3. **Expected warning delta: 0 new `C4996`** (§8). Any new one is a finding with its own row, never a quiet
   fold.
4. **Suite: expected delta 0 against 561**, reconciled **by name**. This row adds no test, and the reason is
   measured (§7).
5. **Verify leg — pin the FULL token**, `AcquireBuilderFocus POST-FLUSH read-back`, pass =
   `POST-FLUSH read-back: IDENTITY=MATCH`. ⛔ **`WARN-1`: do not shorten to `IDENTITY=MATCH`** — the PRE-FLUSH
   line can carry that exact substring on an already-focused open. ⛔ **`WARN-2`: a `NO-MATCH` whose focused
   address is a DESCENDANT of the builder is not the instrument lying** — before verdicting `VERIFY-FAILED`,
   quote the focused `SWidget` address and say whether it equals the builder's, a parent's, or a child's.
   ⛔ **PRE-FLUSH-only is NOT a pass** (it means the widget stopped ticking or was torn down), and
   **`PRE-FLUSH … IDENTITY=NO-MATCH` is EXPECTED and is evidence of nothing.**
6. **Commit BY PATHSPEC.** `DeckBuilderWidget.cpp` · `DeckBuilderWidget.h` · `handoffs/TASK-1307-programmer.md`
   · `qa/TASK-1308-report.md` · `TASKBOARD.md`. ⛔ **Expect ZERO `.uasset`.** ⛔ **Do NOT sweep
   `Tools/run_suite_bounded.ps1`, `Source/.../Tests/SiegeMenuInputTest.cpp` or
   `Content/Blueprints/BP_MenuGameMode.uasset`** — §6 attributes all three to Lane C **by content**, and a bare
   `git commit -a` would swallow two other rows' work into a `TASK-1307`-subject commit. Verify the **COMMIT**
   via `git show --stat HEAD`, never the index (`SC-§106`).
   ⚠️ **`SC-§102`: the git root is `C:\GitProjects\GitHub\GitClaudeUnrealTesting`, one level up** — a
   mis-anchored pathspec answers with silence. I hit exactly that during this review: two greps anchored at a
   relative `Source` returned **empty** for symbols that demonstrably exist, and only an absolute path found
   them. An empty result is not a zero.
7. 🙋 **For the manager, carried forward rather than fixed here:** `WARN-3` (the item (4) / spec (3) wording
   conflict) and `WARN-4` (`DECK-§9`'s eleven `+54` Site cells). Both are documentation; neither blocks the
   commit; ⛔ neither is mine, the programmer's or the host's to write.
