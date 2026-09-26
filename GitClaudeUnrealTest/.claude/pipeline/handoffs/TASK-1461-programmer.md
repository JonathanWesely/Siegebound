# TASK-1461 — [ASSET-AUTHORED-ISFOCUSABLE-CENSUS] programmer handoff (2026-09-24)

Marker: `TASK-1461-ASSET-AUTHORED-ISFOCUSABLE-CENSUS`
Law: `SC-§39` · `SC-§50` · `SC-§101` · `SC-§137` · `SC-§138` · `VER-§8` cl. 7
Read-only. No code, no `.uasset` write, no compile, no PIE, no git, no editor lifecycle action.
Editor probed live before any read: `get_headless_status` → `editor_connected`.
**Package dirtiness: `[]` before, `[]` after, asserted on every Python call via
`unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages()`. Nothing dirtied. Nothing saved.**

---

## 0. THE POSITIVE CONTROL — FIRST, AS THE ROW DEMANDS. AND IT DID NOT GO AS BOARDED.

### 0.1 The boarded control is GONE: `Btn_Jump` on `WBP_VictoryScreen` reads **`True`**, not `False`

```
/Game/UI/WBP_VictoryScreen.WBP_VictoryScreen:WidgetTree.Btn_Jump   IsFocusable = True
/Game/UI/WBP_VictoryScreen.WBP_VictoryScreen_C:WidgetTree.Btn_Jump IsFocusable = True   (generated class, same)
```

I did **not** accept that and report a clean sweep. A control that returns the wrong answer is either a broken
instrument or a stale premise, and those two have to be told apart before any zero counts.

**It is a stale premise, and here is the evidence chain.**

| # | Evidence | Source |
|---|---|---|
| E1 | The decline warning fired **exactly once, ever**: `2026.09.20-06.57.06` UTC | `Saved/Logs/GitClaudeUnrealTest_2-backup-2026.09.20-06.57.15.log:2898` |
| E2 | Commit `1d433ca` (**TASK-1314**) **modified `Content/UI/WBP_VictoryScreen.uasset`**, dated `2026-09-20 01:46:26 -0700` = `08:46 UTC` | `git show -s 1d433ca`, `git show --stat 1d433ca` |
| E3 | ⇒ **the warning fired ~2 h BEFORE the commit.** E1 is the pre-fix state, not the current one | E1 + E2 |
| E4 | LFS payload changed: old oid `7817dd7f…` 153 489 B → new oid `a26ad9a0…` 153 825 B | the two LFS pointers at `1d433ca^` / `1d433ca` |
| E5 | ⭐ **The pre-commit blob's name table CONTAINS `IsFocusable` (offset 4474). The current file does NOT contain the string at all (0 occurrences).** UE serialises a tagged property **only when it differs from the archetype default**; `UButton`'s default is `true` (`Button.cpp:48`) ⇒ old = an authored `False`, current = no override ⇒ `true` | byte grep of `.git/lfs/objects/78/17/7817dd7f…` vs the working-tree file |

⇒ **`TASK-1314` Route K-2 is DONE, not owed.** It shipped inside `1d433ca` — whose own message says
*"the victory screen now hands Slate the BUTTON"* — and **two surfaces still say otherwise and are STALE**:

- ⛔ `Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.cpp:2397-2402` — the comment
  *"AS MEASURED 2026-09-19 THE ASSET SAYS False … That asset half is TASK-1314 Route K-2"* and the
  warning text at `:2410` *"(TASK-1314 Route K-2 is owed)"*.
- ⛔ This row's own premise on `TASKBOARD.md`, and `handoffs/TASK-1398-programmer.md` §2.1 row 7.

⛔ **I did not repair any of them (`SC-§101`).** The guard at `:2403` is still correct and still load-bearing —
it just resolves *true* now. **Routing note for the manager: the comment is a stale claim in shipped source,
and it is the kind that gets believed. It is a one-line row for somebody, not mine to take.**

### 0.2 So I EARNED a control instead — and the instrument returned a real one

**`WBP_CardHand` → `Btn_Jump` → authored `IsFocusable = False`** and
**`WBP_HUD` → `Btn_Jump` → authored `IsFocusable = False`**, each confirmed by **three independent instruments**:

| Instrument | On `WBP_HUD.Btn_Jump` / `WBP_CardHand.Btn_Jump` | On a known-`True` node |
|---|---|---|
| `get_asset_meta` `parts=["WidgetTree"]` | prints the line `IsFocusable=False` | omits the line entirely |
| `execute_unreal_python_readonly` → `get_editor_property("IsFocusable")` on the widget-tree subobject | `False` | `True` |
| byte detector: `grep -a "IsFocusable" <uasset>` (name-table presence) | 1 hit | 0 hits |

⇒ ⭐ **`SC-§137` IS SATISFIED. The matcher has returned a one.** And the same three instruments return the
*opposite* answer on the same property on other nodes, so this is discrimination, not a constant.

### 0.3 The three things that had to be ruled out before any zero counted

1. ⛔ **CDO blindness (the trap named in the dispatch).** `get_asset_meta`'s trailing
   `uint8 bIsFocusable = False` block is the **root `UUserWidget` CDO**, and it prints `False` for
   *every* widget blueprint in the project. **I never read it as a per-node answer.** Every value in §2
   comes from the **authored widget tree**.
2. ⛔ **A deprecated-shim read.** `UButton::IsFocusable` is `UE_DEPRECATED(5.2)` for *direct C++ access*, but
   it is still the one and only serialised `UPROPERTY` (`Button.h:67-70`), and `GetIsFocusable()` returns it
   verbatim (`Button.cpp:236-238`). There is no second, shadow property. I checked the 5.8 headers rather
   than assuming: `UWidget` declares no focusable property at all.
3. ⛔ **A reader that only ever echoes class defaults.** Live mechanism control, same verb, same object family,
   same type: `WBP_MainMenu:WidgetTree.SizeBox_0.bOverride_WidthOverride` reads **`True`** while
   `USizeBox`'s class default is **`False`** (`WidthOverride` `130.0` vs default `0.0`).
   ⇒ the read surfaces **authored non-default bools**, it does not echo the CDO.

---

## 1. METHOD, AND THE FENCES I KEPT

- ⛔ **No sweep-load.** Every asset was loaded **individually, by explicit name**. Nine widget blueprints,
  one at a time, no asset-registry sweep, no `Content/` walk into the editor. No wedge.
- Tree traversal: the Blueprint's `WidgetTree` property and `UWidgetTree::RootWidget` are both **protected**
  and not readable from Python (`Property 'WidgetTree' … is protected and cannot be read`), and
  `UWidgetTree::GetAllWidgets` / `UUserWidget::GetRootWidget` are **not `UFUNCTION`s** in 5.8, so neither is
  Python-exposed. Route taken instead: load one known node by subobject path
  (`<pkg>:WidgetTree.<node>`), climb with the BlueprintCallable `UWidget::GetParent()` to the root, then walk
  down with `UPanelWidget::GetChildrenCount()` / `GetChildAt()`. Seeds came from `get_asset_meta` or from the
  C++ `BindWidget` names.
- Property probe order per node: `IsFocusable` (the `UButton`/`UCheckBox`/`USlider`/`UScrollBox` property),
  then `bIsFocusable` (the `UUserWidget` one). Nodes exposing neither are reported as such, not as zeros.

---

## 2. THE CENSUS — PER SCREEN, NAME LIST, NOT A COUNT

### 2.1 The ten menu/overlay screens of `TASK-1398`'s enumerated set

`AUTHORED IsFocusable=False` column = hits among the four admitted classes
(`UButton` / `UCheckBox` / `USlider` / `UEditableTextBox`).

| # | Screen | Asset | Nodes | Admitted-class nodes, **each with its read-back value** | Authored `False` |
|---|---|---|---|---|---|
| 1 | **Main menu** | `/Game/UI/WBP_MainMenu` | 5 | `Btn_Jump` · `UButton` · **`IsFocusable=True`** | **0** |
| 2 | **Deck builder** | `/Game/UI/WBP_DeckBuilder` | 6 | `Btn_Jump` · `UButton` · **`IsFocusable=True`** | **0** |
| 3 | **Settings** | ⛔ **NO `.uasset` EXISTS** | — | — | **n/a — cannot carry one** |
| 4 | **Graphics** | ⛔ **NO `.uasset` EXISTS** | — | — | **n/a — cannot carry one** |
| 5 | **Login / Account** | ⛔ **NO `.uasset` EXISTS** | — | — | **n/a — cannot carry one** |
| 6 | **Session / lobby** | `/Game/UI/WBP_SessionMenu` | 12 | `HostButton` · `UButton` · **`True`**<br>`JoinButton` · `UButton` · **`True`**<br>`BackButton` · `UButton` · **`True`**<br>`AddressTextBox` · `UEditableTextBox` · **no focusable property on the class** | **0** |
| 7 | **Victory / defeat** | `/Game/UI/WBP_VictoryScreen` | 5 | `Btn_Jump` · `UButton` · **`IsFocusable=True`** ⭐ *repaired at `1d433ca`, see §0.1* | **0** |
| 8 | **War map** ⚠️ | `/Game/UI/WBP_WarMap` | 7 | `RevealButton` · `UButton` · **`True`**<br>`CloseButton` · `UButton` · **`True`** | **0** |
| 9 | **Controls help (Tab)** | ⛔ **NO `.uasset` EXISTS** | — | — | **n/a — cannot carry one** |
| 10 | **Assistant console** | ⛔ **NO `.uasset` EXISTS** | — | — | **n/a — cannot carry one** |

⛔ **`SC-§39` — the asymmetry, stated rather than flattened into a zero.** Rows 3/4/5/9/10 are **100 % C++**
(`TASK-1398` §4.2 verified the assets absent; `WBP_SettingsMenu` and `WBP_ControlsHelp` are RESERVED names
that were never authored). **"There is no asset" and "there is an asset with no opt-out" are different
measurements**, and only rows 1/2/6/7/8 are the second kind.
⚠️ Row 8 (war map) is **excluded from the epic by Jonathan's own choice**; measured here because it cost one
read, and reported so nobody has to wonder.

### 2.2 Folded focus surfaces and coverage-gate widgets — **AND THE TWO HITS ARE HERE**

| Widget | Asset | Node · class · read-back | Authored `False` |
|---|---|---|---|
| **Deck card tile** (the grid's focus stop — folded into `TASK-1423`) | `/Game/UI/WBP_DeckCardTile` | `Btn_Jump` · `UButton` · **`True`** | **0** |
| 🚨 **HUD** (`TASK-1429`'s coverage-gate subject) | `/Game/UI/WBP_HUD` | 🚨 **`Btn_Jump` · `UButton` · `IsFocusable=False`** · **`Visibility = Visible`** | **1** |
| 🚨 **Card hand** (in-match, child of `WBP_HUD`) | `/Game/UI/WBP_CardHand` | 🚨 **`Btn_Jump` · `UButton` · `IsFocusable=False`** · `Visibility = Collapsed` | **1** |

**What these two actually are, said plainly so nobody over-reads them:** both are **untouched UE
third-person-template leftovers** — the same `Btn_Jump` styled with
`/Engine/MobileResources/HUD/VirtualJoystick_Thumb` that appears in five of these assets. They are **not**
authored Siegebound menu controls. The template ships that button with `IsFocusable=False`; in the other
copies somebody has since cleared the override.

⚠️ **But `WBP_HUD`'s is `Visible`, at ZOrder 0, never collapsed in C++ (`SiegePlayerController.cpp:444`),
present for the whole match.** A walker registered against the HUD tree collects a `UButton` that will
decline focus. That is `TASK-1429`'s exact territory. `WBP_CardHand`'s is `Collapsed`, so it is inert.

### 2.3 Project-wide byte-detector sweep (independent of the editor, cannot miss an unreached node)

`grep -a` over all 17 widget assets in `Content/`. `IsFocusable` counts include the substring inside
`bIsFocusable`, so the two columns are read together.

| Asset | `IsFocusable` | `bIsFocusable` | Reading |
|---|---|---|---|
| `WBP_HUD` | 1 | 0 | 🚨 standalone tree-node override ⇒ §2.2 hit |
| `WBP_CardHand` | 1 | 0 | 🚨 standalone tree-node override ⇒ §2.2 hit |
| `WBP_DeckBuilder` | 1 | 1 | the single occurrence **is** `bIsFocusable` ⇒ **not** a tree-node hit; it is the class-settings flag, resolved below |
| `WBP_MainMenu` · `WBP_SessionMenu` · `WBP_VictoryScreen` · `WBP_WarMap` · `WBP_DeckCardTile` · `WBP_CastleHealthBar` · `WBP_CombatantHealthBar` · `UI_Thumbstick` · `UI_TouchSimple` · `UI_LifeBar` · `UI_SideScrolling` · 3 × `UI_TouchInterface_*` | 0 | 0 | no override of either kind anywhere in the file |

⇒ **the sweep and the tree walk agree exactly: two hits, both in §2.2, none on any of the ten menu screens.**

### 2.4 The `UUserWidget` class-settings flag (`bIsFocusable`), read from each CDO

Default is `False`. This is the screen-level flag, **not** a per-control opt-out — reported because it is the
other half of "authored IsFocusable" and because the `WBP_DeckBuilder` byte hit resolves to it.

`WBP_DeckBuilder` = **`True`** (the only one). All nine others — `WBP_MainMenu`, `WBP_SessionMenu`,
`WBP_VictoryScreen`, `WBP_WarMap`, `WBP_HUD`, `WBP_CardHand`, `WBP_DeckCardTile`, `WBP_CastleHealthBar`,
`WBP_CombatantHealthBar` = **`False`**, which is the engine default and is what `UUserWidget` ships.

⚠️ **This is not a defect list.** `False` here is normal; a `UserWidget` root is not a focus stop and the
screens work through their children. It is recorded so a later row does not "discover" it and mis-read a
default as an opt-out.

---

## 3. THE FINDING THAT MATTERS MOST FOR THE EIGHT REMAINING SCREENS

⛔ **THE GOOD NEWS IS REAL: zero of the ten menu screens carries an asset-authored `IsFocusable=False` on any
admitted-class control. The silent-failure class this row was boarded to find DOES NOT EXIST on the subjects
of wave C.** The two hits are on template leftovers outside the epic's screen set.

🚨 ⛔ **BUT THE CENSUS CANNOT SIZE THE RISK IT WAS ASKED TO SIZE, AND SAYING SO IS THE HONEST ANSWER.**

**Five of the ten widget blueprints have byte-identical authored trees** — the untouched template overlay
`Overlay_19 → SizeBox_0 → Btn_Jump` + `Thumbstick_Move` + `Thumbstick_Aim`, and **nothing else**:
`WBP_MainMenu` (5 nodes) · `WBP_DeckBuilder` (6, + `DeckBar`) · `WBP_DeckCardTile` (5) · `WBP_HUD` (5) ·
`WBP_CardHand` (5).

⛔ **`WBP_MainMenu`'s seven menu entries ARE NOT IN ITS AUTHORED TREE.** Neither are the deck builder's grid,
its tiles, or its details panel. They are constructed at runtime — in the EventGraph for the main menu
(`TASK-1398` §4.2: *"The 7 menu entries … are all in the EventGraph the index cannot see"*), in C++ for the
builder. ⇒ **for those five screens, an authored-tree census is structurally incapable of seeing the nodes a
focus walker will actually meet.**

⇒ ⚖️ **THE CONSEQUENCE, WHICH IS A FINDING AND NOT A REMEDY (`SC-§101`):** a runtime-constructed `UButton`
takes `UButton`'s constructor default `IsFocusable = true` (`Button.cpp:48`) **unless some graph node or C++
line sets it otherwise** — and *that* site would be invisible to this census by construction. The
`Source/`-only census (`SiegeControlsHelpWidget.cpp:177` / `:2240` / `:2787`) plus this asset census
**together still leave the Blueprint-EventGraph case unmeasured**, and that case covers the main menu, which
is the one screen already shipping navigation.

⛔ **I am not proposing the follow-up.** Naming the gap is the deliverable; choosing what to do about it is
the manager's (`SC-§101`).

**Only three of the ten screens are genuinely and completely measured by an authored-tree read**, because
only they author their controls in the asset: `WBP_SessionMenu` (12 nodes, 4 admitted, all clean),
`WBP_VictoryScreen` (5) and `WBP_WarMap` (7). **Five have no asset at all and are complete by a different
route — their C++ is readable end to end.** The remaining two (`WBP_MainMenu`, `WBP_DeckBuilder`) are the
partially-measured ones.

---

## 4. WHAT QA / THE MANAGER SHOULD SCRUTINISE

1. ⛔ **§0.1 is the load-bearing claim of this handoff** — that the boarded positive control no longer holds.
   It rests on E1–E5. **E5 is the strong one** (the name-table presence/absence across one commit); E1–E4 are
   corroboration. If E5 is wrong, so is the staleness finding — but the byte detector's behaviour is
   independently confirmed by §0.2, where it agrees with two other instruments on two other assets.
2. ⛔ **§2.4 must not be read as ten opt-outs.** `bIsFocusable=False` on a `UUserWidget` CDO is the engine
   default, not an authored refusal. The distinction is the whole reason §2.1 and §2.4 are separate tables.
3. ⛔ **§2.3's `WBP_DeckBuilder` row** is the one place a careless grep would have manufactured a third hit:
   `grep "IsFocusable"` matches inside `bIsFocusable`. The two columns exist to kill that false positive.
4. ⚠️ **§3 is the part that should change somebody's plan**, not §2. The zero in §2 is real and it is good
   news; §3 says the zero is narrower than the question the row was asking.

---

## 5. Not examined / limitations

- ⛔ **The Blueprint EventGraph case is UNMEASURED and is the biggest gap** — see §3. Any
  `SetIsFocusable` / `Set Is Focusable` node inside `WBP_MainMenu`'s, `WBP_HUD`'s, `WBP_CardHand`'s or
  `WBP_DeckCardTile`'s graph is invisible to every instrument used here. The byte sweep in §2.3 found
  **zero** occurrences of `SetIsFocusable` / `InitIsFocusable` / `GetIsFocusable` in any widget `.uasset`,
  which is *suggestive* that no such node exists — ⛔ **but a Blueprint call node serialises under the target
  function's name, and I have not proven that a `Set Is Focusable` node would put that string in the name
  table. I am NOT claiming the graphs are clean.** Treat §2.3's third column as unproven.
- ⛔ **Runtime `SetIsFocusable` from C++ is out of scope by construction** and is the other census's subject
  (`TASK-1407` WARN-2's `Source/`-only list). The known sites — `SiegeControlsHelpWidget.cpp:177`
  (`CloseButton`), `:2240` (`DetailScrollBox`), `:2787` (`RowScrollBox`) — are **runtime C++ on assetless
  screens**, so they can neither appear in, nor be contradicted by, this census.
- ⛔ **`UEditableTextBox` exposes no focusable property at all** (confirmed on the class). `AddressTextBox` on
  `WBP_SessionMenu` and the three Login fields therefore **cannot** carry an authored opt-out. That is a
  property-model fact, not a measured zero.
- ⛔ **`USlider` and `UCheckBox` were probed for but do not occur in any authored tree** in this project —
  every one of them lives in the C++-only Settings/Graphics screens (`TASK-1398` F4). So the census returns
  no data for two of the four admitted classes, **because there are no instances to measure**, not because
  they were skipped.
- ⛔ **Not read:** `UI_TouchInterface_*`, `UI_TouchSimple`, `UI_LifeBar`, `UI_SideScrolling`,
  `WBP_CastleHealthBar`, `WBP_CombatantHealthBar` were covered by the §2.3 **byte sweep only** (all zero);
  their trees were not walked, because none is a menu/overlay screen in the epic's set.
- ⛔ **No screen in the enumerated set was unreachable.** Every one of the ten is accounted for in §2.1 —
  five by an authored-tree read, five by the recorded absence of any asset.
- ⛔ The previous (`1d433ca^`) version of `WBP_VictoryScreen.uasset` could **not** be loaded into the editor to
  re-read its flag through the reflection instrument, because loading a `.uasset` requires it to sit under a
  mounted content root and **that would have been an asset write**. §0.1 E5 is a byte-level read of the LFS
  object instead, which is why it is stated as name-table presence rather than as a property value.

---

## 6. Files

- **Written:** this file; `TASKBOARD.md` — this row's `status:` line only.
- **Read (assets, one at a time, by name):** `/Game/UI/` `WBP_VictoryScreen` · `WBP_MainMenu` ·
  `WBP_SessionMenu` · `WBP_DeckCardTile` · `WBP_DeckBuilder` · `WBP_WarMap` · `WBP_CardHand` · `WBP_HUD`
  (+ `WBP_CastleHealthBar` / `WBP_CombatantHealthBar` generated-class CDOs only).
- **Read (source / engine / git):** `SiegePlayerController.cpp:2380-2415` ·
  `UE_5.8/…/UMG/Public/Components/Button.h:55-80` · `…/UMG/Private/Components/Button.cpp` ·
  `…/UMG/Public/Blueprint/UserWidget.h` · `…/UMG/Public/Blueprint/WidgetTree.h` ·
  `Saved/Logs/GitClaudeUnrealTest_2-backup-2026.09.20-06.57.15.log:2898` · `git show` at `1d433ca` / `1d433ca^`
  · `.git/lfs/objects/78/17/7817dd7f…`.
- **Changed:** ⛔ **no `.uasset`, no code, no git, no compile, no PIE, no editor lifecycle action.**
