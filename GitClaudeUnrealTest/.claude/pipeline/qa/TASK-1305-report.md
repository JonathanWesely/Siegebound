Verdict: PASS — 0 BLOCKER · 3 WARN · 4 NIT
subject: TASK-1304 (blocks A + B + C, one diff on `6bd1aa5`)

# QA Report — TASK-1305
gate over `TASK-1304` · qa-reviewer · 2026-09-18 · host `TASK-1306`

Reviewed: `Source/GitClaudeUnrealTest/Siegebound/DeckBuilderWidget.h` · `.cpp` ·
`Tests/SiegeDeckSlotsTest.cpp` · `Content/UI/WBP_DeckBuilder.uasset` ·
`handoffs/TASK-1304-programmer.md` (whole, `SC-§38a`).

⭐ **Instruments used, named up front (`SC-§119`): `Read` + `Grep` over project source; `Read` /
`get_text_file_contents` / engine `grep` over `C:\Program Files\Epic Games\UE_5.8\Engine\Source`
(every engine line below was opened by me, not taken from the handoff); and the **read-only
`unreal_inspector`** against the live editor for the `.uasset` CDO, the `WBP_DeckBuilder` widget
tree and the `WBP_MainMenu` event graph. ⛔ No compile, no PIE, no git, no engine-lifecycle call,
no mutation of any kind.**

---

## 1 · REACHABILITY (row cl. (1)/(1b), law ⭐ `SC-§123`) — THE HEAVIEST CHECK, RUN FIRST

### 1.1 The circularity, stated in the required words

`grep -n "NativeOnPreviewKeyDown|NativeOnKeyDown|HandleCardGridKey|AcquireBuilderFocus|MoveCardFocus|SetFocusedCardIndex" Source/` — non-test callers:

| function | non-test callers |
|---|---|
| `HandleCardGridKey` | `DeckBuilderWidget.cpp:1728` (preview) · `:1745` (bubble) — **and nowhere else** |
| `AcquireBuilderFocus` | `DeckBuilderWidget.cpp:670` (`NativeConstruct`) — **one** |
| `MoveCardFocus` | `:1688` — inside `HandleCardGridKey` |
| `SetFocusedCardIndex` | `:1699` — inside `HandleCardGridKey`; `:1313` — inside `MoveCardFocus` |

⇒ **every non-test caller of every arming function lives inside `HandleCardGridKey`, so
`HandleCardGridKey`'s reachability IS the whole feature's reachability.** It has exactly two
entries: the two Slate doors. Nothing else in this class can arm the grid.

### 1.2 What a green `Siegebound.Deck.*` test actually proves — in my own sentence

A test that calls `SetFocusedCardIndex()` or `ExitCardGridFocus()` directly proves **the mechanism
behaves correctly when something invokes it, and says nothing whatever about whether anything
invokes it.** That is why **561 green tests coexisted with a feature in which not one line ever
ran**: the suite was measuring the room while the door was bricked up. The suite is therefore
**not** evidence for anything in this section; every claim below is a source read.

### 1.3 The two static halves — BOTH answered, read at engine source by me

**Half A — the leaf converts the arrow (the mechanism that ate every `Down` at `TASK-1286`).**

```
SWidget.cpp:411-414  FReply SWidget::OnPreviewKeyDown(...) { return FReply::Unhandled(); }   ← NO conversion
SWidget.cpp:415-429  FReply SWidget::OnKeyDown(...) { if (bCanSupportFocus && SupportsKeyboardFocus())
                       { EUINavigation Direction = ...GetNavigationDirectionFromKey(InKeyEvent);
                         if (Direction != EUINavigation::Invalid)
                           return FReply::Handled().SetNavigation(Direction, Genesis); } }    ← the eater
SlateApplication.cpp:5024-5034  tunnel: RouteAlongFocusPath(FTunnelPolicy(EventPath), ...)
                                  → CurrentWidget.Widget->OnPreviewKeyDown(...)   [if IsEnabled()]
SlateApplication.cpp:5045-5047  bubble: `if ( !Reply.IsEventHandled() )` → FBubblePolicy → OnKeyDown
SObjectWidget.cpp:221-229       SObjectWidget::OnPreviewKeyDown → WidgetObject->NativeOnPreviewKeyDown
SObjectWidget.cpp:231-239       SObjectWidget::OnKeyDown → NativeOnKeyDown FIRST, then SCompoundWidget::OnKeyDown
```

⇒ **half A is CLOSED, statically and completely.** The tunnel runs before the bubble, does no
navigation conversion, and reaches `UUserWidget::NativeOnPreviewKeyDown` on every enabled widget on
the focus path. The new override at `.cpp:1715` is therefore genuinely dispatched by a routine I
read, not by an expectation. The second door is also real: `SObjectWidget::OnKeyDown` calls
`NativeOnKeyDown` **before** falling through to `SCompoundWidget::OnKeyDown`, so a builder that is
itself the focused leaf still sees the arrow before `SWidget::OnKeyDown` can convert it.

**Half B — the builder must be ON the focus path.**

```
SlateApplication.cpp:3021-3036  for (WidgetIndex = Num-1; WidgetIndex >= 0; --WidgetIndex)
                                  if (WidgetToFocus.Widget->SupportsKeyboardFocus()) { NewFocusedWidget = ...; break; }
SObjectWidget.cpp:175-183       SObjectWidget::SupportsKeyboardFocus() → WidgetObject->NativeSupportsKeyboardFocus()
UserWidget.cpp:2411-2414        bool UUserWidget::NativeSupportsKeyboardFocus() const { return bIsFocusable; }
UserWidget.cpp:2421-2425        void UUserWidget::SetIsFocusable(bool In) { bIsFocusable = In; Invalidate(Paint); }
```

The read is **live, never cached** — so the code-side `SetIsFocusable(true)` at `.cpp:1484-1487`
genuinely takes effect on the already-built `SObjectWidget`, and the asset-side `True` and the
code-side line are belt-and-braces rather than duplicates. Both halves of `TASK-1286`'s inertness
are addressed, and **the diff does not fix one while leaving the other standing.**

⭐ **A decomposition I derived that the handoff does not state, and it matters to the host:**
once a tile holds focus, the builder is an **ancestor of the focused tile**, so it is on the focus
path **regardless of `bIsFocusable`**. ⇒ `bIsFocusable` is load-bearing for the **cold start only**
(getting the first `Down` in). The `Enter`-raises-the-total leg rides the tunnel unconditionally
once the grid is armed.

### 1.4 The caller exists AND fires at the right moment — MEASURED on the asset graph

`AcquireBuilderFocus()` is called from `NativeConstruct` (`.cpp:670`). The handoff asserts that this
coincides with "the builder opens"; **I checked it rather than accepting it**, on
`get_asset_graph(/Game/UI/WBP_MainMenu, ["EventGraph"])`:

```
OnClicked_Event_9  →  RemoveFromParent (K2Node_CallFunction_89, Target = Self)
                   →  CreateWidget  `Class` = WBP_DeckBuilder_C   (K2Node_CreateWidget_1)
                   →  AddToViewport `ZOrder` = 0                  (K2Node_CallFunction_90)
```
wired to the button whose label is `Conv_StringToText "Deck Builder"` via `AssignDelegate_5`.

Two consequences, both load-bearing and neither previously written down:

1. The builder is **created fresh on the click and added to the viewport there** ⇒ `NativeConstruct`
   fires **at the open gesture**, not early. `TASK-1304` (2)(b) is satisfied at the right instant.
2. ⭐ **The main menu calls `RemoveFromParent` ON ITSELF before creating the builder** ⇒ while the
   builder is up **the main menu is not in the viewport at all**. The "could the preview eat a key
   on the main menu?" regression vector is not merely fenced by a predicate — **there is no menu on
   screen to affect.** Corroborated independently at `SiegeMenuInputSubsystem.cpp:180-196`, whose
   `IsMenuUncovered()` enumerates *top-level* widgets and names "deck builder, settings, login,
   session panel" as things that **cover** the menu, and at `:116`, where `ApplyInitialFocus` is a
   one-shot next-tick timer that early-returns when covered ⇒ **no focus fight with the subsystem.**

### 1.5 The arming chain has no gap

`.cpp:1697-1701` cold `Down` → `SetFocusedCardIndex(0)` → `.cpp:1302-1307` sets the index **and
calls `FocusCardTile`**, which moves Slate focus onto the tile ⇒ `IsCardGridFocusLive()`
(`.cpp:1178-1217`: index valid **and** `Tile->HasAnyUserFocus() || HasFocusedDescendants()`) can
return true on the next key. The chain does not stop at "an index was set".

### 1.6 ⚖️ MY RULING ON THE DEFERRED RUNTIME READ — (a), (b), (c), in writing

**(a) Is the impossibility real, or a route argument wearing a blocker's clothes?**
**REAL.** Mechanically: the premise is *"in a live session, after one `Down`, `FocusedCardIndex != -1`."*
That value can only be produced by code that exists in a **compiled binary**; the code under review
exists only as text on disk; `TASK-1304`'s row fences it from compiling and the pipeline assigns the
compile to `TASK-1306` 5a and the runtime read to 5b. A PIE run from this row would exercise
`6bd1aa5`, in which `AcquireBuilderFocus` does not exist, and could therefore **only** re-confirm the
`-1` negative control already held in `qa/TASK-1286-verify.md`. An instrument that cannot return a
non-negative answer has not measured anything (`SC-§39`).
⛔ **And this is the opposite shape to the failure that killed `TASK-1286`.** There, four readers
accepted *"Slate will deliver the key"* **in place of** a measurement that was available the whole
time and would have said `-1`. Here the programmer (i) filed it as a **BLOCKER on his own
deliverable** rather than a caveat, (ii) named the instrument, the four-step read and what a pass
looks like, (iii) named the row that owes it, and (iv) named the existing negative control. The
difference between the two is the difference between *"nobody looked"* and *"here is exactly what
must be looked at, by whom, and what a pass looks like."*
⭐ It is also what **this row's own clause (6)** prescribes: it lists *"the runtime `FocusedCardIndex`
value"* among the accepted-as-declared items and names ⭐ `TASK-1306` as the row that measures them —
while forbidding me to accept **reachability** as declared, because that is a source read I hold the
tools for. I ran the source read (§1.1–§1.5); I am not empowered to run the other and the row says so.

**(b) Independent of runtime, does the static evidence establish that Slate will route a key into
`HandleCardGridKey`?**
**Half B (`SWidget::OnKeyDown` converting arrows at the leaf): YES, completely** — §1.3 half A,
four engine files read at source. **Half A (the focus path): YES up to one link.** The caller exists
(`.cpp:670`), fires at the open (§1.4, measured on the graph), the flag is `True` on the live CDO
(§3, measured by me), the flag is re-asserted in code on a live-read path (§1.3 half B), and the
leaf→root walk stops on this widget iff that flag is set (`SlateApplication.cpp:3021-3036`).
**The one link static reading cannot close** is whether `FSlateApplication::SetUserFocus` succeeds
synchronously or the deferred `ULocalPlayer::GetSlateOperations()` lane lands it on the next frame.
I verified the deferred lane is the engine's **own** idiom — `UWidget::SetUserFocus` at
`Widget.cpp:738-751` is the identical shape, `SetUserFocus(...)` then `CancelFocusRequest()` on
success, `DelayedSlateOperations.SetUserFocus(...)` on failure — which is the strongest static
evidence obtainable, and it is still not *"this widget held focus in this session."*
⇒ **No handler in this diff rests on "the engine calls this". One precondition rests on an engine
idiom rather than an observation, and it is named rather than smuggled.**

**(c) What remains unmeasured, who owes it, and the binding.**
⛔ **UNMEASURED, ONE SENTENCE: whether a live `AcquireBuilderFocus()` actually leaves this builder's
own `SObjectWidget` holding user focus after the deferred flush — and therefore whether one `Down`
moves `FocusedCardIndex` off `-1`. It is owed by ⭐ `TASK-1306`'s 5b leg (playtest-verifier), on the
newly compiled binaries.**
⛔ **`TASK-1306` CANNOT CLOSE WITHOUT IT.** This row carries a runtime acceptance criterion, so
routing rule 5b/5c and `VER-§` already forbid a commit without a `VERIFIED` report; I am naming it
so no one reads this PASS as discharging it. **A PASS here is a verdict on the bytes, not on the
behaviour.** The four-step read at the head of `handoffs/TASK-1304-programmer.md` is correct and
sufficient; see §6 for the two traps in running it.

**Why this is a PASS and not a FAIL.** A FAIL bounces the row to an agent who is fenced from
compiling and cannot produce the measurement either; it would buy a rewritten handoff, not a number,
and would burn one of three loops for zero information. The code is, as far as source reading can
establish, correct. The honest disposition is PASS **with the measurement named, assigned and
declared blocking on the next row** — which is what the pipeline's own gate structure exists to do.

### 1.7 ⚠️ The withdrawn route test — verified at source, and the claim is stronger than the source

`UserWidget.h:1572` is `protected:`; `NativeOnPreviewKeyDown` is declared at `:1607` and
`NativeOnKeyDown` at `:1608`, inside it — **confirmed by me.** `UDeckBuilderWidget`'s own overrides
sit in its `protected:` section (`.h:619`–`:693`, next specifier `private:` at `:694`) — **confirmed.**
⇒ no automation test can call either door today. See WARN-1 for the one word I take issue with.

---

## 2 · THE PREVIEW FENCE (row cl. (2) + (3b)(b)(c)) — EVERY RETURN TRACED

`NativeOnPreviewKeyDown` (`.cpp:1715-1735`) has **exactly two** exits: it returns the `FReply` from
`HandleCardGridKey` **iff** `Reply.IsEventHandled()`, otherwise `Super::NativeOnPreviewKeyDown`
(which is `UUserWidget`'s, i.e. the Blueprint's own preview event, unchanged). `NativeOnKeyDown`
(`:1737-1752`) is byte-for-byte the same shape with its own `Super`. **There is no second key table
and no preview-only branch** — both doors take the same decision function, so the preview can claim
**exactly** the keys the bubble claims, in **exactly** the states it claims them.

Every `FReply::Handled()` in `HandleCardGridKey`, traced:

| line | returns Handled | reachable only when |
|---|---|---|
| `:1588` | Accept | `bGridFocused` **and** `Key ∈ {Enter, Virtual_Gamepad_Accept.GetVirtualKey(), Gamepad_FaceButton_Bottom}` |
| `:1612` | Remove | `bGridFocused` **and** `Key ∈ {Delete, Gamepad_FaceButton_Left}` |
| `:1677` | **exit gesture** | `bGridFocused` **and** `Key ∈ {Escape, Gamepad_FaceButton_Right, Virtual_Gamepad_Back.GetVirtualKey()}` **and** `ExitCardGridFocus()` returned `true` |
| `:1689` | move focus | `bGridFocused` **and** `NavigationFromKey(Key) != Invalid` |
| `:1700` | enter grid | **not** `bGridFocused`, `Direction == Down`, `GetCollectionCardIDs().Num() > 0` |
| `:1712` | — | everything else returns `FReply::Unhandled()` |

`bGridFocused` is `IsCardGridFocusLive()` (`.cpp:1178`), which is `false` unless
`FocusedCardIndex != INDEX_NONE` **and** a real tile reports `HasAnyUserFocus() ||
HasFocusedDescendants()`. The `TASK-1290` WARN-3 fallback is **still `return false`** at `.cpp:1210`
— verified, unchanged.

✅ **Deck bar (grid cold):** `Left` / `Right` / `Enter` / `Escape` / gamepad **B** all fall to `:1712`
`Unhandled` **on both passes** ⇒ `DECK-§3`'s horizontal bar keeps every key it has and the
navigation Jonathan confirmed by hand at `TASK-1274` is untouched. The **one** key taken is `Down`,
on a horizontal bar where `Down` does nothing today — the already-gated `TASK-1290`/`TASK-1299`
entry design.
✅ **Main menu:** not on the focus path — and, per §1.4, **not on screen at all**.
✅ **Card grid live:** the table above, which is the point of the feature.

### 2.1 🚨 `Escape` specifically (row cl. (3b), block B's authority)

I read the amended law first, at source: `CONVENTIONS.md:796` carries the old *"read at its
WIDEST"* sentence **struck in place**, and `:797`–`:799` carry the **A-2 SCOPE** bullet — 🧑 his
2026-09-18 ruling, verbatim *"Scoped — Escape may exit the card grid"* — which permits a menu-side
widget on `L_MainMenu` to absorb `Escape` as an exit gesture, *"still conditional, never blanket:
consume iff the grid route is live AND the exit actually changed state"*, and keeps A-2 **unrelaxed**
for the assistant console and `ASiegePlayerController`'s in-match cancel routes.

⇒ `EKeys::Escape` at `.cpp:1673` is **REQUIRED by this row, and I do not flag it.**

**The preview question, answered by control flow and not by the claim:** `Escape` **is** in the
preview's key set, because there is one shared decision function. It is fenced there **structurally,
not by a parallel guard**: the exit `if` at `:1673` is **lexically nested inside** the
`if (bGridFocused)` block opened at `:1569` and closed at `:1680`. There is no path on which the
preview reaches that line with the grid cold. And the second condition is enforced by the callee,
not by a duplicated test: `ExitCardGridFocus()` returns **`false`** immediately when
`FocusedCardIndex == INDEX_NONE` (`.cpp:1408-1411`) ⇒ `Handled` at `:1677` is unreachable unless the
call actually changed state. A `Handled` on an `Escape` that changed nothing **cannot be written by
this code**. `TASK-1286` (C) stands untouched.
Falling out of the exit `if` continues to the directional block, where `NavigationFromKey(Escape)`
is `Invalid` (`.cpp:1220-1241` — no `Escape`, no `Enter`) ⇒ `:1712` `Unhandled`. ✅

**The A-2 citation, at every prose site, reading as a GRANT (`SC-§121` cl. 6):**
`.cpp:1630-1662` (quotes the prohibition, then the ruling, the date and `TASK-1300`, then records
A-2 as unrelaxed for the console and the in-match routes) · `.h:266-294` (same shape) ·
`Tests/SiegeDeckSlotsTest.cpp:1393-1402` and `:1497-1501` (*"his A-2 SCOPE grant (2026-09-18) is for
LEAVING A LIVE GRID"*). **Present, dated, attributed, nowhere deleted, nowhere left as a
prohibition.** ✅

---

## 3 · THE `.uasset` FENCE (row cl. (3)) — POST-STATE AND IDENTITY MEASURED BY ME

Read-only inspector, live editor:

```
/Game/UI/WBP_DeckBuilder  | genclass=/Game/UI/WBP_DeckBuilder.WBP_DeckBuilder_C
                          | cdo_class=WBP_DeckBuilder_C | is_DeckBuilderWidget=True | bIsFocusable=True
/Game/UI/WBP_DeckCardTile | genclass=.../WBP_DeckCardTile_C | bIsFocusable=False   (untouched)
/Game/UI/WBP_MainMenu     | genclass=.../WBP_MainMenu_C     | bIsFocusable=False   (untouched)
NATIVE UDeckBuilderWidget CDO bIsFocusable = False
```
plus `get_asset_meta(/Game/UI/WBP_DeckBuilder)` → **`Parent Class: DeckBuilderWidget`**.

✅ **The identity claim is CONFIRMED, two independent ways:** the generated class is still
`WBP_DeckBuilder_C` at the same path and the CDO still resolves as a `UDeckBuilderWidget` with
parent `DeckBuilderWidget`. **This is not a duplicate-and-reparent** — the failure mode that
silently breaks *runtime* repaint while design time looks perfect is not present.
✅ **The `True` is this asset's own override**, because the native parent CDO still reads `False`.
✅ **Siblings unchanged**, as fenced.
✅ Exactly one `.uasset` in the working tree (`Content/UI/WBP_DeckBuilder.uasset`) — and this fence
is **authorised** by `TASK-1304` (4); I do not flag it against `TASK-1286`'s zero-`.uasset` fence.

**Judgement on the pre-state (asked explicitly):** the pre-state is **adequately evidenced for the
only purpose it serves.** What the row needs is that an **authored override now exists on this
asset**, and that is established by the post-state plus the native default (`False`) without relying
on the pre-state at all: a `True` on the BP CDO when the parent default is `False` can only be this
asset's own override. The write lane's contradictory `PRE: True` echo therefore cannot change any
outcome, the script is idempotent, and reporting the anomaly rather than smoothing it was the
correct call. See WARN-3 for the part that genuinely cannot be reviewed by anyone.

---

## 4 · `SC-§122` (row cl. (5), absorbed verbatim from `TASK-1302` cl. 1) — MARKER, PER SYMBOL, WITH A FIRING CONTROL

⛔ I did **not** write *"n symbols read at the line, 0 deprecated."* I searched the **marker by
name**, per symbol, at the **declaration** and at the **definition/registration** site.

**Instrument 1 — declaration site.** `grep -n "UE_DEPRECATED" -A1
Engine/Source/Runtime/InputCore/Classes/InputCoreTypes.h` over the **whole file**:

```
744:  UE_DEPRECATED(5.7, "Use Virtual_Gamepad_Accept.GetVirtualKey() instead")
745-  static INPUTCORE_API const FKey Virtual_Accept;
746:  UE_DEPRECATED(5.7, "Use Virtual_Gamepad_Back.GetVirtualKey() instead")
747-  static INPUTCORE_API const FKey Virtual_Back;
```
**Two hits in the entire header, and they are exactly the two symbols being retired.**
⭐ **The control FIRES and it fires in the tightest possible form**: the same instrument, in the same
file, on **adjacent lines**, returns **1** on `Virtual_Accept`/`Virtual_Back` and **0** on their
replacements `Virtual_Gamepad_Accept` (`:749`) and `Virtual_Gamepad_Back` (`:750`). It could have
returned another number and on the good symbols it did.

**Per-symbol answer for every key this diff binds** (`grep -B1` on each declaration): `Escape :368`
· `Enter :364` · `Delete :381` · `Left/Up/Right/Down :375-378` · `Gamepad_FaceButton_Bottom :516` ·
`Gamepad_FaceButton_Right :517` · `Gamepad_FaceButton_Left :518` · `Gamepad_DPad_* :526-527` ·
`Gamepad_LeftStick_* :532-533` · `Virtual_Gamepad_Accept :749` · `Virtual_Gamepad_Back :750` —
**marker hits: 0 on every one.**

**Instrument 2 — definition site**, `InputCoreTypes.cpp:421-428`, and this is corroboration the
handoff did not claim:
```
PRAGMA_DISABLE_DEPRECATION_WARNINGS
const FKey EKeys::Virtual_Accept = FPlatformInput::GetGamepadAcceptKey();
const FKey EKeys::Virtual_Back   = FPlatformInput::GetGamepadBackKey();
PRAGMA_ENABLE_DEPRECATION_WARNINGS
const FKey EKeys::Virtual_Gamepad_Accept("Virtual_Gamepad_Accept");
const FKey EKeys::Virtual_Gamepad_Back("Virtual_Gamepad_Back");
```
**The engine has to suppress its own deprecation warning to define the two retired symbols, and does
not suppress anything for the two replacements.** Definition-site marker: 2 and 0.

**Every OTHER symbol the delta newly calls, checked the same way** — this is where the last two
C4996s hid, so I did not stop at the key names:

| newly called symbol | declaration | marker |
|---|---|---|
| `FKey::GetVirtualKey()` | `InputCoreTypes.cpp:1445-1449` / `InputCoreTypes.h:208` | none |
| `UUserWidget::IsFocusable()` | `UserWidget.h:1102` | **none** |
| `UUserWidget::SetIsFocusable(bool)` | `UserWidget.h:1104` | **none** |
| `FSlateApplication::GetUserIndexForController(int32)` | `SlateApplication.h:1822` | **none** |
| `FSlateApplication::GetUserFocusedWidget(uint32)` | `SlateApplication.h:1686` | none |
| `FReply::CancelFocusRequest()` | `Reply.h:63-67` | none |
| `SWidget::GetTypeAsString()` | `SWidget.h:1556` | none |
| `UUserWidget::NativeOnPreviewKeyDown` | `UserWidget.h:1607` | none |

⚠️ **Two of those were live near-misses and are worth the record:**
* `UserWidget.h:1030` **does** carry `UE_DEPRECATED(5.2, "Direct access to bIsFocusable is
  deprecated. Please use the getter...")` — on the **UPROPERTY**, not on the accessors. The diff
  uses `IsFocusable()` / `SetIsFocusable(true)` (`.cpp:1484-1486`), i.e. **exactly the prescribed
  non-deprecated form.** Had it touched `bIsFocusable` directly it would have been a third C4996.
* The engine's own `UWidget::SetUserFocus` assigns this call into a `TOptional<int32>`
  (`Widget.cpp:740`), which made me check whether `const int32 UserIndex = ...` compiles.
  It does: `SlateApplication.h:1822` declares the **1-arg `int32`** form (undeprecated) and
  `:1829` a separate **2-arg `TOptional<int32>`** overload. No issue, and no new warning.

⇒ **predicted C4996 count from this delta: 0 new, 2 removed.** The 2-warning baseline itself is
`TASK-1306`'s to measure (`SC-§71b`).

### 4.1 🚨 THE HARD STOP — walked myself, at source, not accepted as traced

```
InputCoreTypes.cpp:423   const FKey EKeys::Virtual_Accept = FPlatformInput::GetGamepadAcceptKey();
InputCoreTypes.cpp:424   const FKey EKeys::Virtual_Back   = FPlatformInput::GetGamepadBackKey();
InputCoreTypes.cpp:728   AddVirtualKey(FKeyDetails(EKeys::Virtual_Gamepad_Accept, ...,
                             FKeyDetails::GamepadKey | FKeyDetails::Virtual, ...),
                             FPlatformInput::GetGamepadAcceptKey());
InputCoreTypes.cpp:729   AddVirtualKey(FKeyDetails(EKeys::Virtual_Gamepad_Back, ..., | Virtual, ...),
                             FPlatformInput::GetGamepadBackKey());
InputCoreTypes.cpp:1017  void EKeys::AddVirtualKey(const FKeyDetails& VirtualKeyDetails, const FKey& VirtualKeyValue)
InputCoreTypes.cpp:1031    InputKeys[VirtualKeyDetails.GetKey()]->VirtualKeyValue = VirtualKeyValue;
InputCoreTypes.h:208     inline const FKey& FKeyDetails::GetVirtualKey() const { return bIsVirtual ? VirtualKeyValue : Key; }
InputCoreTypes.cpp:1445  FKey FKey::GetVirtualKey() const { ConditionalLookupKeyDetails();
                             return (KeyDetails.IsValid() ? KeyDetails->GetVirtualKey() : FKey()); }
```
`EKeys::Virtual_Gamepad_Accept.GetVirtualKey()` → its `FKeyDetails` (registered with the `Virtual`
flag at `:728`, so `bIsVirtual` is true) → `VirtualKeyValue`, stored at `:1031` as
`FPlatformInput::GetGamepadAcceptKey()` — **which is precisely what `EKeys::Virtual_Accept` is at
`:423`.** Identically for Back via `:729`/`:424`.
✅ **SAME `FKey`. The migration is a call-shape change, not a rebinding. The `TASK-1301` HARD STOP is
NOT triggered, and I did not weaken it.** 🧑 His **X**/**B** ruling and the key table are untouched.
⚠️ One behavioural difference, real but benign: the old symbol resolved at **static init**, the new
expression resolves through a **key-registry lookup** cached on first use by
`ConditionalLookupKeyDetails`. Same value, one map lookup once, game thread, inside a key handler
long after `EKeys::Initialize()`. Not a finding.

**The edit itself:** `.cpp:1585` and `.cpp:1673`. Surrounding predicate, operator and **operand
order unchanged**; no reformatting. ✅
**Repo-wide census** `grep -rn "EKeys::Virtual_" Source/` → **10 hits, all inside
`DeckBuilderWidget.{h,cpp}`**: executable **exactly two** (`:1585`, `:1673`), the other eight are
comments (`.cpp:1572, 1575, 1576, 1593, 1597, 1666` · `.h:244, 248`). **Zero sites outside this
feature** ⇒ nothing out of scope to name and nothing else touched. ✅
Block C adds **no** test. ✅ I do **not** flag the fold: a gate read the migrated bytes, and I am it.

---

## 5 · THE CENSUS — RUN BY ME, BOTH INSTRUMENTS, RECONCILED (`SC-§119`, `SC-§104`)

**Instrument A — `grep -n "EKeys::" DeckBuilderWidget.cpp` (line count, includes comments): 16.**
Lines `1225, 1229, 1233, 1237, 1572, 1575, 1576, 1585, 1593, 1597, 1609, 1636, 1648, 1666, 1670, 1673`.
**Executable: 7** (`1225, 1229, 1233, 1237, 1585, 1609, 1673`) · **comment-only: 9** (the rest).

**Instrument B — bound key expressions, as named state:**

| line | expressions |
|---|---|
| `:1225` | `Left` · `Gamepad_DPad_Left` · `Gamepad_LeftStick_Left` |
| `:1229` | `Right` · `Gamepad_DPad_Right` · `Gamepad_LeftStick_Right` |
| `:1233` | `Up` · `Gamepad_DPad_Up` · `Gamepad_LeftStick_Up` |
| `:1237` | `Down` · `Gamepad_DPad_Down` · `Gamepad_LeftStick_Down` |
| `:1585` | `Enter` · `Virtual_Gamepad_Accept.GetVirtualKey()` · `Gamepad_FaceButton_Bottom` |
| `:1609` | `Delete` · `Gamepad_FaceButton_Left` |
| `:1673` | **`Escape`** · `Gamepad_FaceButton_Right` · `Virtual_Gamepad_Back.GetVirtualKey()` |

**= 20**, each exactly once. ✅ matches the expected 20.

**⚠️ THE TWO INSTRUMENTS DISAGREE, AND BOTH ARE RIGHT — reconciled, not adjusted.**
An independent whole-file grep returns `EKeys::Escape` = **4** and `EKeys::` = **16**. Mine return
`EKeys::Escape` **in code = 1** and bound expressions = **20**. The gap is **scope, not
disagreement**: the raw grep counts **lines** and includes **comments**; the census counts **bound
expressions** in **executable** code. Reconciliation: `EKeys::Escape` appears on 4 lines —
`:1636`, `:1648`, `:1670` (the A-2 grant prose, which the row *requires* to be there) and `:1673`
(the one binding). **Executable `Escape` site: `.cpp:1673`, count 1 — REQUIRED by this row.** ✅

Other fences, each measured:
* `EKeys::Tab` — `grep "EKeys::Tab"` over `Siegebound/`: **0** in `DeckBuilderWidget.{h,cpp}` and in
  the test file. Inherited and unbound; stays that way. ✅
* **Letters: 0 · digits: 0** in this feature. (`EKeys::Z`, `EKeys::W/A/S/D`, `EKeys::One..Six`,
  `EKeys::H`, `EKeys::Q` exist in the project but **only** in `SiegeAssistantConsoleWidget` and
  `SiegeControlsHelpWidget` — none in `DeckBuilderWidget.{h,cpp}`.) ⇒ `KBD-§4`'s remap lane is never
  entered. ✅
* `KBD-§` layout tables: **diff 0** — no `KBD-§` table file is in the changed set. ✅
* `DECK-§3` right-click gets no keyboard twin; `+`/`−` button bodies untouched; Accept/Remove call
  the **same** entry points the mouse calls — `AcceptFocusedCard` → `AddCopy(CardID)` (`.cpp:1327`),
  `RemoveFocusedCard` → `RemoveCopy(CardID)` (`.cpp:1346`). ✅
* `IsCardGridFocusLive()`'s live-Slate/no-tile fallback: **still `return false`** (`.cpp:1210`). ✅
* **Prose counts re-derived** (`grep -c "Escape"`): `.h` **16** · `.cpp` **15** · tests **7** —
  all three match the handoff exactly.
* `Tests/SiegeDeckSlotsTest.cpp`: `grep -c "EKeys::"` → **0**. ✅ Prose only; the A-2 grant record is
  present at `:1393-1402` and `:1497-1501`. (The **+4-line / 0-tests-added** claim is a *diff*
  statement I cannot measure without git — `TASK-1306` confirms it via the **561** suite baseline.)

### 5.1 The two board numbers the programmer reported rather than adjusted

* **HEAD comment-`EKeys::` count is 4, not 3** — I cannot re-derive a HEAD count without git, but
  `:1470` (`EKeys::Virtual_Back` inside the Remove block's citation) is exactly the kind of site a
  scan anchored on the `Escape` prose misses, and the correction is in the safe direction
  (reporting **more** than the board claimed). Recorded, not contested.
* **`DeckBuilderWidget.h:227` is correctly left unedited — CONFIRMED.** The sentence there is about
  **`KBD-§4`'s remap table** excluding *"digits/punctuation/Enter/Escape"* from **positional
  remapping**. That is a different instrument from *binding* a key in this widget's own handler, it
  is **still true** after block B, and inverting it would have written a falsehood into the header
  and breached the `KBD-§`-diff-0 fence. Leaving it alone was right. (See NIT-3 on its line pointer.)

---

## Findings

### BLOCKER
**None.**

### WARN
- **[WARN-1] `handoffs/TASK-1304-programmer.md` §"Why the suite cannot reach it either" — the word
  "structurally" overstates what the source supports; the conclusion is right for a better reason.**
  Verified: `UserWidget.h:1572 protected:` with the decls at `:1607-1608`, and this class's own
  overrides sit in its `protected:` section (`DeckBuilderWidget.h:619`–`:693`). So no test can call
  them **today** — but a derived class may widen access to its own override, or the test could be
  made a `friend`, so this is a **refused test-only seam, not an impossibility.** ⛔ The refusal is
  nevertheless **correct, and for the stronger reason the handoff does not give**: a
  direct-invocation test would prove *the mechanism when invoked* — precisely the class of evidence
  `SC-§123` says proves nothing about routing, and precisely what 561 green tests already provide —
  while a **genuine** route test through `FSlateApplication::ProcessKeyDownEvent` on a real focus
  path is unavailable, because the automation lane has no live Slate (the code's own
  `!FSlateApplication::IsInitialized()` branches at `.cpp:1185` and `:1489` exist for that lane).
  ⇒ **suite delta 0 is the right answer on a reachability fix here**; only the justifying sentence
  should read *"refused seam that would prove nothing"* rather than *"structurally cannot."*
  *Measured by:* `grep -n "protected:" DeckBuilderWidget.h` → `619`; engine read of
  `UserWidget.h:1566-1611`. *No code change requested.*

- **[WARN-2] `DeckBuilderWidget.cpp:1520-1559` — the `TAKEN|DEFERRED` word in the new log line is
  NOT a pass/fail signal, and `TASK-1306`'s 5b step 2 must not read it as one.**
  `FSlateApplication::SetUserFocus` returns **`false` when the requested widget is ALREADY the
  focused widget** — `SlateApplication.cpp:3029-3033`: `if (WidgetToFocus.Widget ==
  OldFocusedWidget) { return false; }`. So in an already-focused or re-entrant case `bTookFocus` is
  `false`, the line prints `DEFERRED`, and `AcquireBuilderFocus()` returns `false` **even though
  focus is exactly where it should be**. No behavioural defect — the return value is deliberately
  discarded at `.cpp:670` and the deferred lane is harmless — but this feature has already been
  failed once by an instrument that was misread. ⇒ **the verifier reads the TWO TYPE NAMES** (
  `focused widget is '<type>'` vs `this builder's Slate widget is '<type>'`) **and they must be the
  same widget**; `TAKEN` vs `DEFERRED` is diagnostic colour only.
  *Measured by:* engine read `SlateApplication.cpp:3014-3039`.

- **[WARN-3] `Content/UI/WBP_DeckBuilder.uasset` — the byte delta is unreviewable by anyone, and
  `compile_blueprint` ran before the save.** A binary asset admits no line diff; `compile_blueprint`
  may re-serialise more than the single property. The right substitutes were run and I ran them
  myself rather than accepting them (§3: same asset path, same generated class `WBP_DeckBuilder_C`,
  CDO still a `UDeckBuilderWidget`, parent still `DeckBuilderWidget`, `bIsFocusable = True`, native
  parent default still `False`, both fenced siblings still `False`). The pre-state itself is gone
  and cannot be re-measured by anyone; it is **accepted as declared on the read-only lane's reading**
  and, as argued in §3, **nothing downstream depends on it.** ⇒ **Action for `TASK-1306`: confirm
  `git status` carries EXACTLY ONE `.uasset` and commit by pathspec** — the editor's Git provider
  auto-stages saved assets, so verify the **commit**, never the index
  (`[[ue-git-plugin-autostages-index]]`).

### NIT
- **[NIT-1] `DeckBuilderWidget.cpp:1728` + `:1745` — `HandleCardGridKey` now evaluates twice per
  unclaimed key press** (tunnel, then bubble), and each evaluation calls `IsCardGridFocusLive()` →
  `FindTileForCard` over the tile set. Per **input event**, not per tick, bounded by ~34 tiles.
  Acceptable; recorded so nobody rediscovers it as a mystery cost. *No change requested.*
- **[NIT-2] `DeckBuilderWidget.cpp:1585-1589` — Accept returns `Handled` unconditionally**, even if
  `AddCopy` refuses. Out of scope per row cl. (4) (`TASK-1290`/`TASK-1299` passed the Accept logic),
  and defensible anyway — a live grid should own `Enter` rather than let it fall to
  `Btn_CardFace`'s details panel. ⭐ **The useful half for the verifier:** `AddCopy` (`.cpp:812-830`)
  refuses **only** `None` and an unresolvable `DT_Cards` row — the `MaxCopies` cap was deleted at
  `UNCAP-§4` and there is **deliberately no add-time deck-total guard** ⇒ **`Enter` on any valid
  card raises the total even on a 50-card deck**, so the acceptance question is safe to ask against
  whatever deck is active. That removes the obvious false-negative trap.
- **[NIT-3] `DeckBuilderWidget.h:227` — the citation `CONVENTIONS:2519-2522` has drifted.** The
  claim is true, but it now lives at **`CONVENTIONS.md:2528`** (*"DELIBERATELY NOT REMAPPED: digits
  (the `1`–`6` hotkeys), punctuation, modifiers … `Space`, `Enter`, `Escape` … a design decision,
  not an omission"*); `:2519-2522` today reads as the `PlayerMappableKeySettings` / `IMC_Hero`
  argument. **Pre-existing, not introduced by this diff, and the line was correctly left untouched**
  (`KBD-§` diff 0). `CONVENTIONS.md` is itself dirty in this tree, so line anchors into it decay by
  nature. *Fix opportunistically on a future authorised touch of that header — not on this row.*
- **[NIT-4] Board-number corrections, both confirmed as reported:** the HEAD comment-`EKeys::` count
  is 4 (not 3), and the manager's `Escape`-site list omits `.h:227`. Reporting them rather than
  silently conforming is the behaviour this feature's history asks for.

---

## 6 · ACCEPTED-AS-DECLARED — what I could NOT reach, and who measures it (`SC-§71b`)

I hold no `Bash`, no git and no write-side MCP. Measured by ⭐ **`TASK-1306`**, not by me:
the **compile** (`Result: Succeeded`) · the **warning delta** (target **0** against the declared
2-warning baseline; `SC-§122` cl. 6) · the **suite** (baseline **561**, expected delta **0**) ·
**`git status`** (the named files + exactly one `.uasset`) · and the **runtime `FocusedCardIndex`
value**.
⛔ **The line, drawn explicitly:** accepted-as-declared covered **none** of §1's reachability work.
That is a source read, I hold `Read` and `Grep`, and I ran it — §1.1 through §1.5, on eight engine
files I opened myself. The only thing deferred is a **runtime value no static tool can produce**.

### Notes for build-master and the verifier (if PASS)

1. ⛔ **The 5b runtime read is BLOCKING on `TASK-1306`, not optional.** The four-step read at the
   head of `handoffs/TASK-1304-programmer.md` is correct and sufficient. Step 2 must be read per
   **WARN-2** (compare the two widget **type names**, ignore `TAKEN`/`DEFERRED`).
2. 🧑 **The acceptance question goes to Jonathan verbatim, and only the first answer is this
   feature:** ***"With a card outlined, does pressing `Enter` raise the deck total by one — or does
   it only open the card's details panel?"*** ⛔ An outline appearing is **not** evidence: Slate's
   own navigation draws a focus rectangle on `Btn_CardFace` and `Enter` there fires that button's
   existing `OnClicked`. **The builder will look and feel navigable while every line of this feature
   is dead** — that is exactly how `TASK-1286` passed two gates, a clean compile and 561/561.
   Per NIT-2, a full deck is **not** a confound; `AddCopy` has no cap.
3. ⛔ **`Escape` is the editor's default stop-PIE key.** Spend **zero** injection attempts on it —
   the `Escape` leg is a hand check or it is recorded `UNOBSERVABLE`.
4. The new `AcquireBuilderFocus` log line is at **`Log`** verbosity, so it prints with no `Log
   LogGitClaudeUnrealTest Verbose` incantation. ⚠️ It fires **once per builder open** — if the log
   is empty, that is a **failure to reach the call site**, not a quiet pass.
5. The editor (PID 8288) is **up** and must be closed for the compile; the orchestrator owns the
   bounce. C++ changed ⇒ relaunch on the new binaries before 5b (rule 5a, graceful-quit lane, never
   Live Coding — `Ctrl+Alt+F11` cannot carry a new override anyway).
6. Commit message should carry the verbatim acceptance question (row cl. (8)(6)).
