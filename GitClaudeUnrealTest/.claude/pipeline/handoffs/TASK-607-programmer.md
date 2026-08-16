# TASK-607 — [ACC-9] `Btn_Login` on `WBP_MainMenu` — the main-menu splice — programmer handoff

- **Author:** gameplay-programmer · 2026-08-16 · inside TASK-606's editor session (**PID 29812**, fresh binary, MCP at `http://127.0.0.1:8000/mcp`)
- **Status on exit:** `ready-for-integration` (gated by TASK-608's integration check per the coverage ledger — NOT by a qa/ report)
- **Editor handed back RUNNING, PID 29812.** Never closed, no PIE, no `M`, no console sentence — the TASK-571 latch stays unspent.
- ⛔ **Zero Git mutations.** Two read-only `git status --porcelain -- Content/` calls, nothing else. Staging untouched.

> ⚠️ **WHAT I AM AND AM NOT CLAIMING (§17 / ACC-§5(e)).**
> **The button is PLACED and WIRED** — a statement about the Blueprint graph, verified by node-identity readback and one independent filesystem signal.
> ⛔ **It is NOT a claim that it renders, looks right, or that clicking it opens the panel on screen.** MCP readback has passed on visually-broken UMG on this project before; a readback cannot see geometry. **Everything on-screen closes on Jonathan's pixels at TASK-609.**

---

## 1. 🔒 `L_Arena` — the hash chain is kept

SHA256 verified before the first MCP call and again after the save:

| | value |
|---|---|
| SHA256 (both checkpoints) | `B3DBC5D9AE484A7BD02CAFAD52B4681DA68B011477479B65EE7781AE459F8268` |
| Bytes | 535,522 |

✅ Identical to TASK-606's four-checkpoint exit value. `L_Arena` was never opened, loaded, or saved; git shows the map clean. No save prompt of any kind appeared.

## 2. What changed — exactly one asset, exactly one file

**`/Game/UI/WBP_MainMenu`** — saved via `save_assets` with that single explicit path. Nothing else saved.

- On disk: `Content/UI/WBP_MainMenu.uasset` — **341,978 → 380,675 bytes**, new SHA256 `03CC0A395EBE072D4248F6429027622A9FECCB99752EA8D5F3CD35DA42564AE3`, mtime 2026-08-16 13:31:33. (BEFORE hash `B56E5C91…BC61` = byte-identical to TASK-438's save — nothing had touched it since.)
- `git status --porcelain -- Content/` = exactly one line: `M .../Content/UI/WBP_MainMenu.uasset`, **unstaged**; I changed no staging state.
- ⛔ **No `WBP_AccountMenu` was authored** — verified live before the first graph op: `exists("/Game/UI/WBP_AccountMenu")` → `false`, and it is still false. The `CreateWidget` node picks the **C++ class** per ACC-§5(c); the reserved name stays reserved.
- ⛔ **Nothing duplicated or reparented.** The duplicate+reparent corruption law was never approached. `UWidgetBlueprintFactory` was not needed — the spec authors no WBP.

## 3. Pre-flight (RULING 6 re-verified first-hand, not relayed)

```
search_subclasses(base=/Script/UMG.UserWidget, class_name="AccountMenuWidget")
  -> ["/Script/GitClaudeUnrealTest.AccountMenuWidget"]
```
✅ The class resolves in the RUNNING editor binary (the SC-§26 bounce held). `is_dirty(WBP_MainMenu)` was `false` at start. Jonathan was warned via Slack before the first graph op and the editor was not taken mid-interaction.

## 4. What was built — the shipped Settings idiom, lifted not invented

### The handler — post-compile `read_graph_dsl`, verbatim

```lisp
(event Custom|LoginBtnClicked
  (bind _returnvalue (UserInterface|CreateWidget "/Script/GitClaudeUnrealTest.AccountMenuWidget"))
  (UserInterface|Viewport|AddToViewport _returnvalue 10))
```

✅ `CreateWidget(UAccountMenuWidget)` → `AddToViewport(ZOrder 10)`.
⛔ **NO `RemoveFromParent` on the main menu** — the Settings precedent followed deliberately (spec (2): the panel overlays; `UAccountMenuWidget::BackPressed` removes only itself, per TASK-603). Deck Builder and Multiplayer DO remove the menu; this asymmetry is the ruling, not an oversight.

⭐ Strongest single evidence the class binding is real: after setting the `Class` pin, `CreateWidget_4.ReturnValue` **changed type on its own** from `User Widget Object Reference` to **`Account Menu Widget Object Reference`** — the engine's type system reacting, an effect, not a tool describing its own call.

### The construction block — spliced between the Settings block and the Quit block

| property | value | source |
|---|---|---|
| label | **"Login"** | `ACC-§6` row 8 / board `names:` |
| font size | **28.0** | read off the shipped buttons |
| padding | **`MakeMargin(24.0, 12.0, 24.0, 12.0)`** | read off the shipped buttons |
| h-alignment | **`HAlign_Fill`** | the node's own default — same as every shipped entry, nothing set |

**Resulting order: Play (vs Bot) → Sandbox (No Bot) → Deck Builder → Multiplayer → Settings → Login → Quit.** Vertical order follows `AddChildToVerticalBox` call order, so the block was spliced **before** the Quit block. **Quit stays last.**

### Node ledger — granular ops ONLY, `write_graph_dsl` NEVER called

**14 nodes created** (`add_event` ×1 — `K2Node_CustomEvent_27` = `LoginBtnClicked`, created FIRST — plus `create_node` ×13), **10 `set_pin_value`**, **1 `break_pins`**, **27 `connect_pins`**, **0 `delete_node`**. Every `type_id` resolved through `find_node_types` before creation (never copied from readback).

New nodes: `GenericCreateObject_12` (Button) · `GenericCreateObject_13` (TextBlock) · `CallFunction_82` ToText(String)="Login" · `CallFunction_83` SetText(Text) · `CallFunction_84` SetFontSize 28.0 · `CallFunction_85` AddChild · `CallFunction_86` AddChildToVerticalBox · `CallFunction_87` SetHorizontalAlignment (default HAlign_Fill) · `MakeStruct_6` MakeMargin 24/12/24/12 · `CallFunction_88` SetPadding · `AssignDelegate_7` AssignOnClicked · `CreateWidget_4` (Class=`/Script/GitClaudeUnrealTest.AccountMenuWidget`) · `CallFunction_91` AddToViewport ZOrder 10.

The ONE existing link touched: `CallFunction_80.then` (Settings SetPadding) ↔ `GenericCreateObject_5.execute` (Quit's Button) — broken, my block inserted, re-closed at `CallFunction_88.then → GenericCreateObject_5.execute`.

### Lane traps confirmed live (both defeated)

1. **`AssignOnClicked` auto-rename trap fired exactly as CONVENTIONS predicts:** `create_node` on it silently auto-spawned its own custom event (`K2Node_CustomEvent_28`) pre-wired into `Delegate`. Because `LoginBtnClicked` was created FIRST, one connect displaced the stub (Delegate is single-link). Verified: `CustomEvent_27.OutputDelegate → AssignDelegate_7.Delegate`, and `CustomEvent_28` reads **0 connections on every pin**.
2. **Lowercase-t creatable spellings:** `UserInterface|Viewport|AddtoViewport` (the TASK-438 lane note) **and `Panel|AddChildtoVerticalBox`** — the same artifact class on a second node type; readback prints both with capital T.

## 5. ⛔ THE SIX PRE-EXISTING ENTRIES — PROVEN UNTOUCHED BY NODE IDENTITY, NOT ASSERTED

Full `get_node_infos` before/after dumps compared on type, position, every pin value, and every connection:

- **BEFORE: 118 nodes → AFTER: 134. REMOVED: 0.** The 16 added = my 14 + 2 inert stubs (§6).
- ⭐ **113 of 118 pre-existing nodes byte-identical.** ⭐ **ZERO pin-value changes anywhere in the pre-existing graph** — no label, font, margin, alignment, or class on any existing node.
- The **5** changed pre-existing nodes are the intended splice points and nothing else:

| node | change | why |
|---|---|---|
| `CallFunction_80` (Settings SetPadding) | `.then` → `GenericCreateObject_12` | the splice in |
| `GenericCreateObject_5` (Quit's Button) | `.execute` ← `CallFunction_88` | the re-close — **Quit still runs, after Login** |
| `GenericCreateObject_0` (root VerticalBox) | `.ReturnValue` **+1** link | my AddChildToVerticalBox; **all 7 prior links intact** |
| `Self_0` | `.self` **+2** links | my two ConstructObject outers; **all 13 prior links intact** |
| `AssignDelegate_6` (Settings binding) | `.then` `[]` → `AssignDelegate_7` | appended on a **previously free** pin — purely additive |

✅ Binding table after: Play→`CustomEvent_1` · Quit→`CustomEvent_3` · Deck Builder→`CustomEvent_9` · Sandbox→`CustomEvent_18` · Multiplayer→`CustomEvent_16` · Settings→`CustomEvent_22` — **every pre-existing Delegate link character-identical**; Login (new)→`CustomEvent_27`.
✅ **`BuildSandboxButton` function graph: 684 chars — byte-count-identical to TASK-438's recorded figure.** Never opened for edit.

### The full-graph DSL diff (TASK-608's core evidence) — pasted verbatim

⚠️ **Read this with the DSL-printer renumbering artifact in mind (TASK-438 §6, same artifact):** the printer numbers `_returnvalue_N` sequentially by traversal, so the insert renumbers Quit's block from `_14..16` to `_17..19`. The `-"Quit"/+"Login"` line at `_returnvalue_15` is **my insertion, NOT a relabel of Quit** — Quit still reads `"Quit"` at `_returnvalue_18` five lines below in the same hunk. **The node-identity table above is the authoritative proof; this text diff is the required evidence exhibit.**

```diff
--- before.dsl
+++ after.dsl
@@ -21,2 +21,4 @@
 (event Custom|OnClicked_Event_14)
+
+(event Custom|OnClicked_Event_15)
 
@@ -72,3 +74,3 @@
       (bind _returnvalue_15 (Game|ConstructObjectfromClass "/Script/UMG.TextBlock" _self))
-      (Widget|SetText(Text) _returnvalue_15 (Utilities|Text|ToText(String) "Quit"))
+      (Widget|SetText(Text) _returnvalue_15 (Utilities|Text|ToText(String) "Login"))
       (Appearance|SetFontSize _returnvalue_15 28.0)
@@ -78,6 +80,15 @@
       (Layout|VerticalBoxSlot|SetPadding _returnvalue_16 (Utilities|Struct|MakeMargin 24.0 12.0 24.0 12.0))
-      (Button|Event|AssignOnClicked _returnvalue_14 (AddEvent|Custom|OnClicked_Event_0))
+      (bind _returnvalue_17 (Game|ConstructObjectfromClass "/Script/UMG.Button" _self))
+      (bind _returnvalue_18 (Game|ConstructObjectfromClass "/Script/UMG.TextBlock" _self))
+      (Widget|SetText(Text) _returnvalue_18 (Utilities|Text|ToText(String) "Quit"))
+      (Appearance|SetFontSize _returnvalue_18 28.0)
+      (Widget|Panel|AddChild _returnvalue_17 _returnvalue_18)
+      (bind _returnvalue_19 (Panel|AddChildToVerticalBox _returnvalue _returnvalue_17))
+      (Layout|VerticalBoxSlot|SetHorizontalAlignment _returnvalue_19)
+      (Layout|VerticalBoxSlot|SetPadding _returnvalue_19 (Utilities|Struct|MakeMargin 24.0 12.0 24.0 12.0))
+      (Button|Event|AssignOnClicked _returnvalue_17 (AddEvent|Custom|OnClicked_Event_0))
       (Button|Event|AssignOnClicked _returnvalue_5 (AddEvent|Custom|OnClicked_Event_9))
       (Button|Event|AssignOnClicked _returnvalue_8 (AddEvent|Custom|MultiplayerBtnClicked))
-      (Button|Event|AssignOnClicked _returnvalue_11 (AddEvent|Custom|SettingsBtnClicked)))
+      (Button|Event|AssignOnClicked _returnvalue_11 (AddEvent|Custom|SettingsBtnClicked))
+      (Button|Event|AssignOnClicked _returnvalue_14 (AddEvent|Custom|LoginBtnClicked)))
     (:CastFailed
@@ -122,2 +133,4 @@
 
+(event Custom|OnClicked_Event_16)
+
 (event Custom|SettingsBtnClicked
@@ -125 +138,5 @@
   (UserInterface|Viewport|AddToViewport _returnvalue 10))
+
+(event Custom|LoginBtnClicked
+  (bind _returnvalue (UserInterface|CreateWidget "/Script/GitClaudeUnrealTest.AccountMenuWidget"))
+  (UserInterface|Viewport|AddToViewport _returnvalue 10))
```

Full evidence set (before.dsl · after.dsl · dsl-diff.txt · before-nodes.json · after-nodes.json · after-BuildSandboxButton.dsl) is in the session scratchpad `task607/` folder; the diff above and §5's table are the complete substance of it.

## 6. ⚠️ Recorded, not buried — the two inert stubs and observed count notes

1. **Two inert stub events left behind:** `K2Node_CustomEvent_28` (the AssignOnClicked auto-spawn) and `K2Node_CustomEvent_30` (from the compile's graph reconstruction). **Both read ZERO connections on every pin** — verified, not assumed. Deliberately NOT deleted (TASK-355 measured that deleting a stub mints a replacement while spending a graph reconstruction next to bindings I must prove untouched — TASK-438 made the same call). The deferred stub sweep now has **13** to collect.
   - ⚠️ Naming note, stated because it looks like an inconsistency: the node dump titles them `OnClicked_Event_16`/`OnClicked_Event_17` while the DSL prints the two new bare events as `OnClicked_Event_15`/`OnClicked_Event_16`. Same two 0-connection nodes either way; the display-name divergence is printer/reconstruction cosmetics on inert artifacts, and no code or binding references either name.
2. **BEFORE enumeration was 118 nodes, not the 117 TASK-438 recorded at its exit.** The on-disk asset was hash-identical to TASK-438's save, so the +1 is a load-time reconstruction artifact of the current session or a counting-method difference in that handoff — not a foreign edit. My before/after pair comes from the same session minutes apart and is internally consistent; the diff evidence stands on it.
3. Log chatter: six `LogBlueprint: Warning: User provided name was invalid Name is already in use. - node named CustomEvent` lines at node-creation time — the same non-compiler chatter class TASK-355/438 recorded; the stub origin.

## 7. Compile — two passes, `warnings_as_errors=true`, silence on the second

| pass | call | result |
|---|---|---|
| 1 | `compile_blueprint(warnings_as_errors=true)` | returned without raising → clean |
| 2 | `compile_blueprint(warnings_as_errors=true)` | returned without raising → **clean, and silent** (the GUID self-heal persistence proof, the TASK-355 lane law) |

`LogBlueprint` shows exactly two `Compiling Blueprint '/Game/UI/WBP_MainMenu.WBP_MainMenu'` lines (20.28.20 / 20.28.24) with **no compiler error or warning lines following either.** No C++ compile, no Build.bat — TASK-608's suite re-run clause is NOT triggered by me (nothing recompiled outside the one Blueprint).

Save corroborated by two independent signals, not the return value: `is_dirty` flipped `true`→`false`, AND the `.uasset` changed size + hash on disk.

## 8. ⚠️ What MCP reported that I could not independently corroborate

1. ⛔ Everything structural in §4–§5 comes through ONE MCP surface (the node dumps, the DSL, the identity proof are the same tool describing itself — mutually consistent and cross-checked against the shipped Settings block, but not independent). My only genuinely independent evidence is the filesystem: `WBP_MainMenu.uasset` changed size/hash/mtime and `L_Arena.umap`'s did not.
2. ⛔ The compile "clean" verdict is an inference from the return + the `LogBlueprint` category, not the full output log.
3. ⛔ `find_nodes` enumeration is trusted, not verified — no second enumerator exists.
4. ⛔ The stubs' "0 connections" is readback only.
5. ⛔ **NOTHING was rendered and no PIE was run. No pixel was produced or observed at any point.** On-screen correctness — the seven entries, the order, the panel opening on click, the menu NOT vanishing — is owed to **Jonathan's pixels at TASK-609** in full.

## 9. ⚠️ Naming — `Btn_Login` names the ENTRY, not a design-time object (the shipped idiom)

`WBP_MainMenu` builds its menu buttons at runtime via `ConstructObjectfromClass` into local pins — there is no design-time `Btn_Login` variable, exactly as there is no `Btn_Settings`, `Btn_Multiplayer`, `Btn_Quit`, or `Btn_DeckBuilder` (TASK-438 §9 flagged this precedent so nobody greps and concludes failure). The board's `names:` line pins the entry (label "Login" → `CreateWidget(UAccountMenuWidget)` → `AddToViewport(10)`), and every part of that contract is met. The greppable identifier this entry ships is the handler event **`LoginBtnClicked`**, matching the graph's `SettingsBtnClicked`/`MultiplayerBtnClicked` style. No code references it by name.

## 10. Standing-law compliance

- **AS-§6 A-2:** no key handling touched anywhere; `Escape` stays unabsorbed. No new key bindings.
- **RULING 7 fences:** no PIE, no console sentence, no `M` press — the latch and the empty-map instrument are untouched.
- **ACC-§2:** no credential surface exists in this task; nothing here logs or stores anything.
- **Editor coordination (pre-flight row 4):** Slack heads-up posted before the first graph op; editor handed back running, PID 29812; Jonathan's session never taken mid-interaction.
- Housekeeping: the one temp dump this task wrote under project `Saved/` was deleted; `Saved/` is untracked either way.

## 11. M8 DECLARATION (batch header, verbatim)

Adds no replicated property, no new replicated class, no new relevancy tier, no RPC. All account state is client-local (`UGameInstanceSubsystem` + local `USaveGame`); the display name touches no session/player name (A7). Does NOT consume the M8 Phase-1 checkpoint gate; does NOT substitute for Jonathan's owed feedback items.

## 12. For TASK-608 / TASK-609 downstream

- **TASK-608:** the integration-check evidence is §5 (identity table + the pasted DSL diff). The commit roster gains exactly one Content path from this task: `Content/UI/WBP_MainMenu.uasset` (binary/LFS-lane check per SC-§25). Nothing recompiled ⇒ no suite re-run owed on my account. The 596 `SC-§27` verdict precondition is unaffected by me.
- **TASK-609 (Jonathan):** (a) SEVEN entries in order, Quit last, Login matching the others' size/spacing; (b) click Login → panel appears **on top of a still-present menu** (if the menu vanishes, my handler is wrong); (c) `Back` → menu still fully alive, nothing double-fires; (d) with the panel open, clicks on the dim area over Play/Quit must do NOTHING (`BackdropBorder` hit-test — TASK-603's work, visible only to pixels).

— gameplay-programmer, TASK-607, 2026-08-16
