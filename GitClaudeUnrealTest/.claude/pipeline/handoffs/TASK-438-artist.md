# TASK-438 — [SET-3] `Btn_Settings` — the main-menu entry, ADDITIVE — art-director handoff

- **Author:** art-director · 2026-08-03 · hosted inside **TASK-447's** editor session (**PID 9828**, booted 14:55:44, MCP up at `http://127.0.0.1:8000/mcp`)
- **Status on exit:** `ready-for-integration`
- **Editor handed back RUNNING, PID 9828.** Never closed, never force-killed, no PIE, no reopen. The build stayed green and untouched.
- ⛔ **Zero Git.** One read-only `git status --porcelain -- Content/` footprint audit, nothing else. Staging left exactly as found.

> ⚠️ **READ THIS FIRST — WHAT I AM AND AM NOT CLAIMING.**
> **The button is PLACED and WIRED.** That is a statement about the Blueprint graph, verified by node-identity readback.
> ⛔ **It is NOT a claim that it renders, that it looks right, or that clicking it opens the panel.** On this project an MCP/UMG readback has passed on visually-broken UMG before (TASK-355: 6/6 green bindings on six controls stacked in a 165×48 px corner box). **A readback cannot see geometry.** Everything on-screen is Jonathan's check at **TASK-448** — §6 below.

---

## 1. 🔒 `L_Arena` — THE HASH CHAIN IS KEPT

Verified by **SHA256, not mtime**, immediately before the first MCP call and again after the save:

| | value |
|---|---|
| **SHA256** | `B3DBC5D9AE484A7BD02CAFAD52B4681DA68B011477479B65EE7781AE459F8268` |
| **Bytes** | **535,522** |
| **mtime** | `2026-07-29 03:53:38` (unchanged — it did not even get touched) |
| **`is_dirty` (live, before AND after my save)** | `false` |

✅ **Matches the expected baseline exactly.** `L_Arena` was never opened, never loaded, never saved. Build-master's five-checkpoint chain continues unbroken through this task.

## 2. What changed — exactly one asset, exactly one file

**`/Game/UI/WBP_MainMenu`** — saved via `save_assets` with that single explicit path. Nothing else was saved.

- On disk: `Content/UI/WBP_MainMenu.uasset` — **341,978 bytes**, mtime `2026-08-03 15:49:34`, SHA256 `B56E5C91BAA8FBA0589E40B6293669140C543305B44D8C4ABAFBFBA530BC4F61`
- `git status --porcelain -- Content/` returns exactly one line: `` M Content/UI/WBP_MainMenu.uasset`` — **unstaged**, and I changed no staging state.
- ⛔ **No `WBP_SettingsMenu` was authored** — confirmed live, `exists("/Game/UI/WBP_SettingsMenu")` → **`false`**. The `CreateWidget` node picks the **C++ class**, per §3 condition (c).
- ⛔ **Nothing was duplicated or reparented.** The duplicate-and-reparent corruption law was never approached.

## 3. THE PRE-FLIGHT THAT GATED THIS TASK — the compile did land

The whole reason TASK-438 waited on TASK-447 is that a `CreateWidget` node cannot pick an uncompiled C++ class. **Verified before touching the graph:**

```
search_subclasses(base=/Script/UMG.UserWidget, name="SettingsMenuWidget")
  -> ["/Script/GitClaudeUnrealTest.SettingsMenuWidget"]
get_class("/Script/GitClaudeUnrealTest.Default__SettingsMenuWidget")
  -> "/Script/GitClaudeUnrealTest.SettingsMenuWidget"
```

✅ The class resolves live in the running editor. No stop-and-report was needed.

## 4. What was built

### The handler — post-compile `read_graph_dsl`, verbatim

```lisp
(event Custom|SettingsBtnClicked
  (bind _returnvalue (UserInterface|CreateWidget "/Script/GitClaudeUnrealTest.SettingsMenuWidget"))
  (UserInterface|Viewport|AddToViewport _returnvalue 10))
```

✅ `CreateWidget(USettingsMenuWidget)` → `AddToViewport(**ZOrder 10**)`.
⛔ **NO `RemoveFromParent` on the main menu** — CONVENTIONS §4. The panel sits ON TOP; `BackPressed()` removes only itself and the menu is revealed again, already alive. (The Deck Builder and Multiplayer handlers **do** call `RemoveFromParent` — mine deliberately does not, and that asymmetry is the ruling, not an oversight.)

⭐ **The strongest single piece of evidence that the class binding is real:** after setting the `Class` pin, `CreateWidget.ReturnValue` **changed type on its own** from `User Widget Object Reference` to **`Settings Menu Widget Object Reference`**. That is the engine's type system reacting — an observable *effect*, not a tool's claim about its own call (§17's unifying rule).

### The construction block — the shipped idiom, lifted not invented

Spliced into `EventConstruct`, between the Multiplayer block and the Quit block:

| property | value | source |
|---|---|---|
| label | **"Settings"** | board `names:` line |
| font size | **28.0** | read off the shipped buttons |
| padding | **`MakeMargin(24.0, 12.0, 24.0, 12.0)`** | read off the shipped buttons |
| h-alignment | **`HAlign_Fill`** | the node's own default — same as every shipped entry, so nothing was set |

**Resulting order: Play (vs Bot) → Sandbox (No Bot) → Deck Builder → Multiplayer → Settings → Quit.**
Vertical order follows `AddChildToVerticalBox` call order, so the block had to be spliced **before** the Quit block — appending at the chain tail would have put Settings *below* Quit. **Quit stays last.**

### Node ledger — granular ops ONLY, `write_graph_dsl` was NEVER called

**14 nodes created** (`add_event` ×1, `create_node` ×13), **10 `set_pin_value`**, **1 `break_pins`**, **27 `connect_pins`**. Zero `delete_node`.

| # | Op | Node | Detail |
|---|---|---|---|
| 1 | `add_event` | `K2Node_CustomEvent_22` | **`SettingsBtnClicked`** @ (13900,3300) — **created FIRST, on purpose (§5)** |
| 2 | `create_node` | `K2Node_GenericCreateObject_10` | `Game\|ConstructObjectfromClass`, Class=`/Script/UMG.Button` |
| 3 | `create_node` | `K2Node_GenericCreateObject_11` | same, Class=`/Script/UMG.TextBlock` |
| 4 | `create_node` | `K2Node_CallFunction_74` | `Utilities\|Text\|ToText(String)`, InString=**`Settings`** |
| 5 | `create_node` | `K2Node_CallFunction_75` | `Widget\|SetText(Text)` |
| 6 | `create_node` | `K2Node_CallFunction_76` | `Appearance\|SetFontSize` **28.0** |
| 7 | `create_node` | `K2Node_CallFunction_77` | `Widget\|Panel\|AddChild` |
| 8 | `create_node` | `K2Node_CallFunction_78` | `Panel\|AddChildToVerticalBox` |
| 9 | `create_node` | `K2Node_CallFunction_79` | `Layout\|VerticalBoxSlot\|SetHorizontalAlignment` `HAlign_Fill` |
| 10 | `create_node` | `K2Node_MakeStruct_5` | `Utilities\|Struct\|MakeMargin` 24/12/24/12 |
| 11 | `create_node` | `K2Node_CallFunction_80` | `Layout\|VerticalBoxSlot\|SetPadding` |
| 12 | `create_node` | `K2Node_AssignDelegate_6` | `Button\|Event\|AssignOnClicked` |
| 13 | `create_node` | `K2Node_CreateWidget_3` | `UserInterface\|CreateWidget`, Class=`/Script/GitClaudeUnrealTest.SettingsMenuWidget` |
| 14 | `create_node` | `K2Node_CallFunction_81` | `UserInterface\|Viewport\|AddtoViewport`, **ZOrder 10** |
| 15 | `break_pins` | `CallFunction_71.then` ↔ `GenericCreateObject_5.execute` | **the ONE existing link touched** — re-closed at op 16 |
| 16 | `connect_pins` ×27 | — | the block, re-closed into the Quit block, + the binding + the handler |

## 5. ⛔ THE `AssignOnClicked` AUTO-RENAME TRAP — it fired, and the `add_event`-first pattern defeated it

**Confirmed live, exactly as CONVENTIONS predicts.** `create_node` on `Button|Event|AssignOnClicked` **silently auto-spawned its own custom event** (`K2Node_CustomEvent_23`, named `OnClicked_Event_13`) already wired into its `Delegate` pin.

Because I had created `SettingsBtnClicked` **first**, the fix was one connect: `Delegate` is single-link, so connecting my event's `OutputDelegate` **displaced** the auto-spawned link. Verified by readback:

```
K2Node_CustomEvent_22 (SettingsBtnClicked).OutputDelegate -> K2Node_AssignDelegate_6.Delegate
K2Node_AssignDelegate_6.self                              <- K2Node_GenericCreateObject_10 (the Settings Button)
K2Node_CustomEvent_22.then                                -> K2Node_CreateWidget_3.execute
```

⚠️ **TWO inert stub events were left behind and I am recording them rather than burying them:** `OnClicked_Event_13` (`CustomEvent_23`, the auto-spawn) and `OnClicked_Event_14` (`CustomEvent_25`, from the compile's graph reconstruction). **Both read ZERO connections on every pin** — verified, not assumed. They are the same artifact class as the nine already in this graph.

⚖️ **I deliberately did NOT delete them, and this is a considered deviation from TASK-355's op-16.** TASK-355 *measured* that deleting the auto-stub triggers a reconstruction that mints a replacement stub — so the delete buys **zero** net cleanup while spending a graph reconstruction right next to five bindings I am required to prove untouched. Leaving them is strictly the lower-risk option. **They belong to the deliberately-deferred stub sweep, which now has 11 to collect.**

⚠️ **Related, and NOT hidden:** the log carries five `LogBlueprint: Warning: User provided name was invalid Name is already in use. - node named CustomEvent` lines at `22.21.41`. These are **node-creation/reconstruction chatter, not compiler output** (same line TASK-355 recorded), and they are the origin of the two stubs above.

## 6. ⛔ RULING 3 — THE FIVE PRE-EXISTING ENTRIES ARE PROVEN UNTOUCHED, NOT ASSERTED

### ⚠️ FIRST — A TRAP FOR THE NEXT READER: the raw DSL diff LOOKS like I renamed the Quit button

The DSL printer numbers its `_returnvalue_N` binds **sequentially by traversal order**, so inserting a block **renumbers everything after it**. The naive diff shows:

```
-      (Widget|SetText(Text) _returnvalue_12 (Utilities|Text|ToText(String) "Quit"))
+      (Widget|SetText(Text) _returnvalue_12 (Utilities|Text|ToText(String) "Settings"))
```

⛔ **That is NOT the Quit button being relabelled.** `_returnvalue_12` now denotes *my* TextBlock; Quit's shifted to `_returnvalue_15` and still reads `"Quit"` in the same diff. **This is a rendering artifact of the DSL printer** — the identical artifact TASK-355 recorded. **Do not read the label-diff as a graph change.**

### THE REAL PROOF — by NODE IDENTITY, which does not shift

Full `get_node_infos` dump of **every** node before and after, compared on type, position, every pin value and every connection:

- **BEFORE: 101 nodes → AFTER: 117 nodes. REMOVED: 0.** The 16 added = my 14 + the 2 inert stubs.
- ⭐ **96 of 101 pre-existing nodes are byte-identical.** ⭐ **NOT ONE PIN VALUE CHANGED ANYWHERE IN THE GRAPH** — no label, font, margin, alignment or class on any existing node.
- The **5** pre-existing nodes that differ are the intended splice points and nothing else:

| node | change | why |
|---|---|---|
| `CallFunction_71` (Multiplayer SetPadding) | `.then` → `GenericCreateObject_10` | the splice in |
| `GenericCreateObject_5` (Quit's Button) | `.execute` ← `CallFunction_80` | **the re-close — Quit still runs, just after Settings** |
| `GenericCreateObject_0` (root VerticalBox) | `.ReturnValue` **+1** link | my `AddChildToVerticalBox`; **all 6 prior links intact** |
| `Self_0` | `.self` **+2** links | my two `ConstructObject` nodes; **all 11 prior links intact** |
| `AssignDelegate_4` (Multiplayer's binding) | `.then` `[]` → `AssignDelegate_6` | appended on a **previously free** pin — purely additive |

✅ **`Play (vs Bot)`→`StartMatch`, Sandbox→`BuildSandboxButton`+`StartSandboxMatch`, `Deck Builder`→`OnClicked_Event_9`, `Multiplayer`→`MultiplayerBtnClicked`, `Quit`→`OnClicked_Event_0`+`QuitGame` — every binding character-identical.**
✅ **The `BuildSandboxButton` function graph diff is EMPTY** — byte-for-byte identical, 684 chars before and after. It was never opened for edit.

## 7. Compile — two passes, `warnings_as_errors=true`, silence on the second

Per the lane law earned at TASK-355 (the design-time GUID self-heal is an `ensureAlwaysMsgf`, so **silence on the second compile is the proof it persisted**):

| pass | call | result |
|---|---|---|
| 1 | `compile_blueprint(warnings_as_errors=true)` | returned without raising → clean |
| 2 | `compile_blueprint(warnings_as_errors=true)` | returned without raising → **clean, and silent** |

`LogBlueprint` shows exactly two `Compiling Blueprint '/Game/UI/WBP_MainMenu.WBP_MainMenu'` lines (`22.44.20` and `22.45.00`) with **no compiler error or warning result lines following either.** Zero ensure / AccessedNone / Fatal.

Save corroborated by **two independent signals**, not by the return value: `is_dirty` flipped `true`→`false`, **and** the `.uasset` changed size + hash on disk.

## 8. ⚠️ WHAT MCP REPORTED THAT I COULD NOT INDEPENDENTLY CORROBORATE

**Stated plainly, because "the call returned" is not "the thing happened."**

1. ⛔ **EVERYTHING STRUCTURAL IN §4–§7 COMES THROUGH ONE MCP SURFACE.** The node dumps, the DSL, the pin values and the 96/101 identity proof are all the same tool describing itself. They are mutually consistent and cross-checked against the shipped block, but they are **not independent**. My only genuinely independent evidence is the filesystem: the `.uasset` size/hash/mtime changed and `L_Arena.umap`'s did not.
2. ⛔ **THE COMPILE "CLEAN" VERDICT IS AN INFERENCE.** `compile_blueprint` returns `null` on success and I read only the **`LogBlueprint` category**, not the full output log. "No error lines in that category" is weaker than "the compiler emitted nothing." I did not run a C++ build and did not verify the widget at runtime.
3. ⛔ **`find_nodes` ENUMERATION IS TRUSTED, NOT VERIFIED.** The 101→117 counts rest on the tool returning every node. I have no second enumerator.
4. ⛔ **THE TWO INERT STUBS ARE "0 CONNECTIONS" BY READBACK ONLY.**
5. ⛔ **NOTHING WAS RENDERED, AND NO PIE WAS RUN.** No pixel was produced or observed by me at any point.
6. ⚠️ **`is_dirty` on `L_Arena` read `false` throughout** — corroborated by the SHA256, which is the authority. The hash is the claim I stand behind; `is_dirty` is decoration.

## 9. ⚠️ NAMING — `Btn_Settings` NAMES THE ENTRY, NOT AN OBJECT, AND THAT IS THE SHIPPED IDIOM

**Flagging this so nobody greps for `Btn_Settings` and concludes the task failed.** `WBP_MainMenu` builds its menu buttons **at runtime** via `ConstructObjectfromClass` into local pins — there is **no design-time `Btn_Settings` variable, exactly as there is no `Btn_Multiplayer`, `Btn_Quit` or `Btn_DeckBuilder`.** The only design-time button in this widget is the touch-control `Btn_Jump`.

The board's `names:` line pins the **entry** (label "Settings" → `CreateWidget(USettingsMenuWidget)` → `AddToViewport(10)`), and every part of that contract is met. Following the shipped idiom character-for-character was spec item (1) and it is mutually exclusive with minting a design-time variable. **The named, greppable identifier this entry ships is the handler event `SettingsBtnClicked`**, matching the graph's existing `PlayBtnClicked` / `QuitBtnClicked` / `MultiplayerBtnClicked` style. **No code references any of these by name**, so nothing downstream binds to it.

## 10. 🎨 WHAT JONATHAN MUST CHECK AT TASK-448 — I closed none of this

**Items 1–2 are the ones my work makes necessary and they are the blockers.** Items 3–7 are TASK-437's checklist, repeated so the settings screen is tested as one thing.

1. **The main menu shows SIX entries, in this order:** `Play (vs Bot)` → `Sandbox (No Bot)` → `Deck Builder` → `Multiplayer` → **`Settings`** (new) → `Quit`. ⚠️ **`Quit` must still be LAST**, and the new entry must match the others' size/spacing (font 28, same 24/12 padding). ⚠️ **This is the exact failure class TASK-355 shipped past a green readback — please look, do not trust my table.**
2. ⭐ **NEW, AND IT IS MINE: click `Settings` and confirm the MAIN MENU DOES NOT DISAPPEAR.** The panel must appear **on top of** a still-present menu. ⛔ **If the main menu vanishes, my handler is wrong** — `Deck Builder` and `Multiplayer` both `RemoveFromParent` and mine deliberately must not. This is the single most likely way I got it wrong and it is invisible to every readback I ran.
3. ⭐ **ALSO NEW, AND IT IS THE ONE THAT COULD BITE HARDEST: press `Back`, then use the main menu normally.** Because the menu was never removed, its buttons are still live underneath — **confirm `Play`, `Sandbox`, `Deck Builder`, `Multiplayer` and `Quit` all still work after a Settings open/close round-trip**, and that nothing double-fires.
4. ⛔ **THE BACKDROP CLICK-THROUGH TEST — already on your list, and my ZOrder-10 overlay is what it is testing.** With Settings open, click **directly where a main-menu button sits** (especially **Quit**) on the dim area outside the panel column. **NOTHING must happen.** If the game starts or **quits**, `BackdropBorder` is not absorbing → blocker.
5. **The check box reflects the real setting on open** — first launch shows it **CHECKED**. Toggle off → `Back` → re-open: still off. Quit the game entirely, relaunch, open Settings: **still off** (the save round-trip).
6. **The label is clickable**, not just the box. **`Back` dismisses only the panel.**
7. **The hint sentence is fully readable** — long, auto-wrapping, must not clip or overlap at your resolution.

## 11. Lane knowledge earned (recommend for CONVENTIONS)

1. ⭐ **THE CREATABLE `type_id` FOR ADD-TO-VIEWPORT IS `UserInterface|Viewport|AddtoViewport` — LOWERCASE `t`** — even though `read_graph_dsl` and `get_node_infos` both **print** it as `AddToViewport`. `find_node_types` returns the creatable spelling; the readback spellings are display strings. ⚠️ **Copying the type_id out of a DSL readback is the natural move and it is the wrong one.** Resolve every `type_id` through `find_node_types` before `create_node`.
2. **`find_node_types` returns an ARRAY OF PLAIN STRINGS**, not objects. Filtering it on `.type_id` yields zero matches and reads exactly like "the node type does not exist." The count is the signal.
3. **`find_node_types` costs ~14 s per call** on this graph — a loop over a dozen ids blows a 2-minute tool timeout. That is tool cost, **not** an editor hang.
4. **Confirming all 12 type_ids resolved to exactly ONE match each let me omit `declaring_class` everywhere** — TASK-355 lane note 2 (a `declaring_class` filter can reject a valid node).
5. **`AssetTools` params are `path` for `exists` but `asset_path` for `is_dirty`/`get_dependencies`** — inconsistent across the same toolset; the error message names the right one.
6. ⭐ **THE DSL PRINTER'S SEQUENTIAL BIND LABELS MAKE ANY MID-CHAIN INSERT LOOK LIKE A RENAME.** ⇒ **Ruling-3 proofs should be done by NODE IDENTITY (`get_node_infos` before/after), not by diffing DSL text.** A text diff of an insert is unreadable as evidence; the node-identity diff gave a clean "5 of 101 changed, 0 pin values changed."

## 12. State ledger

- Editor **RUNNING**, **PID 9828**, MCP up and answering. **Handed back alive.** No close, no reopen, no force-kill, no PIE, no `-run=` commandlet.
- **Saved by me: `/Game/UI/WBP_MainMenu` ONLY.** No foreign save. `L_Arena` never opened or saved — SHA256 verified before and after.
- `Content/` working-tree delta is exactly `M Content/UI/WBP_MainMenu.uasset`, **unstaged**. I altered no staging state; the hostile auto-stager was not provoked.
- **Zero Git commands** beyond one read-only `git status --porcelain -- Content/`. HEAD unchanged at `56acf10`. **No compile of C++, no Build.bat** — the green build was not disturbed.
- ⛔ **No `.uasset` created, duplicated, deleted or reparented.** ⛔ **`write_graph_dsl` NEVER called.**
