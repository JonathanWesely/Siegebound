# TASK-809 handoff — THE CARD-BAR EDIT (art-director)

- **Author:** art-director · **Date:** 2026-09-02 (editor session PID **22940**, started 21:32:41)
- **Asset edited:** `/Game/UI/WBP_CardHand` — ⛔ **IN PLACE. No duplicate, no reparent, no new WBP.**
- **Law:** `CARDBAR-§0` · `§1` · `§2` · `§3` · `§4` (corrected) · `§6` · `§7` · `§11`/`§11a`–`§11e` · `§13` · `HELP-§2` · `AS-§6` A(e)
- **Method:** granular node surgery (`create_node`/`delete_node`/`connect_pins`/`break_pins`/`set_pin_value`) — ⛔ **NOT a wholesale `write_graph_dsl` rewrite of any existing graph**, per spec item (5) and `TASK-033:113-119`'s LOSSY finding. The **only** `write_graph_dsl` call in this task targeted a **brand-new, empty** function graph (nothing to lose).

---

## 0. ⭐⭐ THE HARD GATE — `GetSlotKeyLabel` IS IN THE PALETTE. MEASURED, THREE WAYS.

Spec item (0) / `CARDBAR-§13`. Run **before any edit**:

| Probe | `TASK-808` (2026-09-03, old DLL) | ⭐ **NOW** |
|---|---|---|
| `find_node_types(filter "KeyLabel")` | ⛔ `[]` **empty** | ✅ **`["Siegebound\|UI\|GetSlotKeyLabel"]`** |
| `find_node_types(filter "Siegebound\|UI\|")` — positive control | **7** functions, `GetSlotKeyLabel` ⛔ absent from its own category | ✅ **8** functions, `GetSlotKeyLabel` **present** |
| `get_node_type_pins("Siegebound\|UI\|GetSlotKeyLabel")` | n/a | ✅ resolves with **real reflected pins**: in `self : Card Hand Widget Object Reference`, `SlotIndex : Integer`; out `ReturnValue : String` |

⭐ **The third probe is the strongest of the three:** the editor resolved the function's *reflected signature*, which only exists if UHT compiled **and registered** the `UFUNCTION`. Corroborated on disk: DLL **7,620,096 B @ 21:26:13**, editor started **21:32:41** — i.e. the running editor loaded the **new** binary.

⇒ ✅ **Gate PASSED. ⛔ No digit was ever typed. The label is a live call to `GetSlotKeyLabel`.**

⚠️ **MCP liveness was proven with a REAL request, ⛔ not a port check** (`SHIP-§9e`): `list_graphs` on `WBP_CardHand` returned the 7 shipped graphs matching `TASK-808` node-for-node.

---

## 1. WHAT CHANGED — NODE BY NODE

### 1a. `BuildHandTree` (the templated loop body) — ⛔ 16 nodes deleted, ⭐ 8 added, 1 retargeted

**DELETED — the Play block (8):** `GenericCreateObject_26` (Button) · `GenericCreateObject_27` (TextBlock) · `CallFunction_65` (`ToText "Play"`) · `CallFunction_66` (`SetText`) · `CallFunction_67` (`AddChild btn←text`) · `CallFunction_68` (`AddChild vbox←btn`) · `VariableGet_18` (`GetSlotPlayButtons`) · `CallArrayFunction_12` (`Array|Add`)

**DELETED — the discard block (8):** `GenericCreateObject_28` (Button) · `GenericCreateObject_29` (TextBlock) · `CallFunction_69` (`ToText "1"`) · `CallFunction_70` (`SetText`) · `CallFunction_71` · `CallFunction_72` · `VariableGet_19` (`GetSlotDiscardButtons`) · `CallArrayFunction_13`

⛔ **Every one was verified by `type_id` and pin-value read-back BEFORE deletion** (e.g. `CallFunction_65`'s `InString` pin read literally `"Play"`).

**Exec healed:** `Array|Add(SlotCostTexts)` → `WrapCardFace` (was → Play Button).

**ADDED (8):** `GenericCreateObject_2` = **`slotColumn : VerticalBox`** · `GenericCreateObject_3` = **`KeyChip : TextBlock`** · `CallFunction_26` = `SetFontSize 12.0` · `CallFunction_27` = `SetVisibility "HitTestInvisible"` · `CallFunction_28` = `AddChild(slotColumn, KeyChip)` · `CallFunction_29` = `AddChild(slotColumn, FaceOverlay)` · `VariableGet_10` = `GetSlotKeyChips` · `CallArrayFunction_2` = `Array|Add(SlotKeyChips, KeyChip)`

**RETARGETED (the one-line change, `§11e`):** `CallFunction_74`'s `Content` pin — was `WrapCardFace.FaceOverlay`, now **`slotColumn`**. ⇒ `AddChild(_handbox, slotColumn)` replaces `AddChild(_handbox, _faceoverlay)`.

**The loop body now reads back as:**

```
(bind _returnvalue_1 VerticalBox)                  ← slotVBox (unchanged)
  NameText 16 → AddChild → Array|Add SlotNameTexts
  CostText 14 → AddChild → Array|Add SlotCostTexts
(bind (_faceoverlay _artimage) (WrapCardFace _returnvalue_1))
(Array|Add Img_CardArt _artimage)
(bind _returnvalue_4 VerticalBox)                  ⭐ slotColumn   NEW
(bind _returnvalue_5 TextBlock)                    ⭐ KeyChip      NEW
(SetFontSize _returnvalue_5 12.0)                  ⭐ smallest shipped size
(SetVisibility _returnvalue_5 "HitTestInvisible")  ⭐ NON-INTERACTIVE
(AddChild _returnvalue_4 _returnvalue_5)           ⭐ KeyChip   = child [0]  ← ABOVE
(AddChild _returnvalue_4 _faceoverlay)             ⭐ FaceOverlay = child [1]
(Array|Add SlotKeyChips _returnvalue_5)
(AddChild _returnvalue _returnvalue_4)             ⭐ slotColumn into _handbox
(Array|Add SlotBoxes _returnvalue_1)               ✅ STILL slotVBox
```

✅ **`§11e` honoured exactly:** ⛔ no `SizeBox` introduced · sizing inherited (`HorizontalBoxSlot` Auto) · *"a bit smaller"* = **font 12** · the chip is a `TextBlock` with hit-testing **OFF**, ⛔ not a `Button`.

### 1b. `EventGraph` — ⛔ 61 nodes deleted, ⭐ 1 added

**The atomic removal (`§11b` (a)+(b)+(c)) — 60 nodes, all `type_id`-verified in a single scripted pass that would have ABORTED on any unexpected type:**

| Group | Count | Verified as |
|---|---|---|
| `AssignDelegate_14..25` | 12 | all `Button\|Event\|AssignOnClicked` |
| `CustomEvent_32..54` (even) | 12 | `OnPlaySlot0..5` + `OnDiscardSlot0..5` |
| `CallFunction_87..98` | 12 | 6× `RequestPlaySlot` + 6× `RequestDiscardSlot` |
| `GetArrayItem_16..27` | 12 | all `Utilities\|Array\|Get(acopy)` |
| `VariableGet_24..35` | 12 | 6× `GetSlotPlayButtons` + 6× `GetSlotDiscardButtons` |

**Exec healed:** `SetVisibility(self,"SelfHitTestInvisible")` → `CastToSiegePlayerController`.

**⭐ THE THIRD CONSUMER — the one that fired every gold tick (`§11b`, spec 1b):** deleted `CallFunction_71` (`Widget|SetIsEnabled`) + `GetArrayItem_31` + `VariableGet_39` (`GetSlotPlayButtons`) from `EventOnHandSlotUpdated`. ⛔ No exec heal needed — it was the tail of the else branch. ✅ **The affordability grey-out SURVIVES** via `UpdateSlotArt`'s independent art tint; ⛔ I did not try to "preserve" the disable.

**ADDED (1):** `CallFunction_19` = **`CallFunction|UpdateSlotKeyChip(SlotIndex, CardID)`**, spliced `UpdateSlotArt → UpdateSlotKeyChip → Branch`, **inside the `IsValid` guard, beside the existing `UpdateSlotArt` call** — exactly the shipped PULL shape.

### 1c. ⭐ NEW function graph `UpdateSlotKeyChip(SlotIndex, CardID)`

⛔ **NOT a `Bind` dropdown** — `GetSlotKeyLabel` takes a parameter and can never be a UMG property binding (`§11d`, `qa/TASK-810.md` §4). It is a **CALL + `SetText`**:

```
(fn UpdateSlotKeyChip (SlotIndex CardID)
  (bind _slotkeychips (Variables|Default|GetSlotKeyChips))
  (bind _output (Utilities|Array|Get(acopy) _slotkeychips SlotIndex))
  (Utilities|IsValid _output
    (:"Is Valid"
      (if (Utilities|String|IsEmpty CardID)
        (Widget|SetVisibility _output "Collapsed")            ← empty slot ⇒ chip hides
        (else
          (bind _returnvalue (Siegebound|UI|GetSlotKeyLabel self SlotIndex))
          (if (Utilities|String|IsEmpty _returnvalue)
            (Widget|SetVisibility _output "Collapsed")        ← DEGRADE-OPEN, never a glyph
            (else
              (Widget|SetText(Text) _output (Utilities|Text|ToText(String) _returnvalue))
              (Widget|SetVisibility _output "HitTestInvisible"))))))))
```

⚠️ **THE DELIBERATE DECISION `§11e` DEMANDS BE STATED, ⛔ not made by accident:** `SlotBoxes[i]` **still points at `slotVBox`** — ⛔ I did **not** re-point it at `slotColumn` (that would be a semantic change). ⇒ **the chip therefore needs its OWN collapse on an empty slot, and it has one** — the `IsEmpty(CardID)` branch above. ⭐ Stated explicitly because a chip left visible over a hidden card would be a visible defect.

### 1d. Member variables

- ⭐ **ADDED `SlotKeyChips : TextBlock[]`** — the direct analogue of the shipped `SlotNameTexts`/`SlotCostTexts`/`SlotBoxes`; required so the update handler can reach the chip. ✅ Verified as a real property on the **compiled CDO**.
- ⛔ **REMOVED `SlotPlayButtons`, `SlotDiscardButtons`** (`§11b` (d)) — **only after proving ZERO references remain across all 8 graphs**, and after `grep` proved they are pure-Blueprint (⛔ **no** C++ `BindWidget`/`UPROPERTY` — `grep` over `Source/` returns **no matches**). Variable count 14 → 12.

---

## 2. ⛔ WHAT I DID *NOT* DO — the enumerated fence

- ⛔ **`Btn_Jump`: NOT deleted, NOT reparented, NOT renamed, NOT collapsed.** Left at its shipped `Visible` (the collapse requirement was withdrawn with item (8); leaving it is the status quo and regresses nothing). ✅ **Chain re-verified AFTER all edits:** `Btn_Jump.Slot` → `SizeBox_0.SizeBoxSlot_0` → `Overlay_19.OverlaySlot_0` → `Overlay_19.Slot` = **`None`** (root). ⇒ `CastToOverlay(GetParent(GetParent(Btn_Jump)))` still resolves; **the bar will attach.**
- ⛔ **ITEM (8) — nothing.** No hit-test flip. `UpdateSlotArt` is **byte-identical** (its `HitTestInvisible` node untouched) · the NEXT-card preview untouched · **no mouse handler added** — confirmed positively: `OnMouseButtonDown` reads `bIsImplemented: false`. ⛔ Nothing half-flipped was left behind; ⛔ none of the withdrawn item (8) ever landed in this session.
- ⛔ Root visibility **not changed in either direction**. ⭐ **Reported as a value (spec 3b):** `Overlay_19` = **`SelfHitTestInvisible`**, and `EventConstruct` still re-sets `self` to `SelfHitTestInvisible` at runtime.
- ⛔ No `WBP_HUD` · no `IMC_Hero`/`IA_Card1..6` · no `DT_Cards` · no C++ · no `HELP-§` row · **no Git**.
- ⛔ The 19 pre-existing inert `OnClicked_Event_*` stubs and the **orphaned donor chain** (`AssignDelegate_2` = `AssignOnGoldChanged`, `AssignDelegate_3` = the donor `Btn_Jump` `OnClicked`) were **left alone** — pre-existing dead code, ⛔ not mine.

---

## 3. ⚠️ TWO FINDINGS I AM DECLARING RATHER THAN BURYING

### 🚨 FINDING 1 — `WBP_CardHand.uasset` WAS SAVED TO DISK BEFORE I ARRIVED, AGAINST `CARDBAR-§13`'s "DECLINE"

`TASK-808` measured the file pristine at HEAD: **2026-07-07, 821274 B, sha `5c3ce7…`**. I found it at **20:06:14 today, 832878 B, sha `aad0d7…`**, with the **LFS pointer already moved** in the worktree (`git diff` shows `oid 5c3ce7…→aad0d7…`). ⇒ **the save prompt at the editor close for `TASK-824`'s build was ACCEPTED, not declined.**

✅ **Assessed, not merely reported: the saved content was BENIGN.** My first read of all 7 graphs matched `TASK-808`'s measured tree **node for node** (5 design-time nodes, the same `BuildHandTree`, the same 12 `AssignOnClicked`, the same 19 stubs). ⇒ it was a **re-serialize against the newer parent class**, ⛔ not a logic change. **I proceeded on that basis, and I took a rollback copy first** (below). ⛔ Not mine to fix — flagged for build-master at `TASK-811`.

### ⚠️ FINDING 2 — DELETING AN `AssignOnClicked` SPAWNS AN INERT `OnClicked_Event_N` STUB (`TASK-041`'s quirk, in reverse)

After the 12 deletions the stub count went **19 → 20** (`OnClicked_Event_19` appeared) and 12 `LogBlueprint: Warning: User provided name was invalid Name is already in use` lines were emitted. I inspected it — **zero connections on every pin**, identical to the pre-existing 19 — and **deleted it**. ✅ Stub count is back to its original **19 + 1 real**. ⭐ Recorded because the next person to delete a delegate binding in this namespace will see the same warnings and should not read them as damage.

---

## 4. VERIFICATION — WHAT IS PROVEN, AND HOW

| Claim | Evidence |
|---|---|
| Blueprint **compiles** | `UpdateSlotKeyChip` reads back `bIsImplemented: **true**`; `SlotKeyChips` is a **real property on the compiled CDO**. ⛔ A failed compile produces neither. `LogBlueprint` shows the compile with **⛔ zero errors** |
| Both buttons **gone** | Token sweep over **all 8 graphs**: `SlotPlayButtons`, `SlotDiscardButtons`, `RequestPlaySlot`, `RequestDiscardSlot`, literal `"Play"` ⇒ **ZERO hits each** |
| No dangling `Accessed None` | The three `SlotPlayButtons` consumers and both `SlotDiscardButtons` consumers are **all** removed; the arrays themselves no longer exist |
| Chip is **above** the card | `AddChild(slotColumn, KeyChip)` executes **before** `AddChild(slotColumn, FaceOverlay)` ⇒ children `[0]`/`[1]` in a `VerticalBox` |
| Chip is **non-interactive** | `SetVisibility(KeyChip, "HitTestInvisible")` at construction; it is a `TextBlock`, ⛔ not a `Button` |
| Label is **not** a typed digit | `GetSlotKeyLabel` node present in `UpdateSlotKeyChip`; ⛔ no `SetText` with a literal anywhere in that graph |
| `Btn_Jump` intact | parent chain re-read **after** all edits (§2 above) |
| `L_Arena` **not** saved | `is_dirty` = **false**; disk mtime **Aug 27 15:05**, untouched |
| Only ONE asset saved | `save_assets(["/Game/UI/WBP_CardHand"])` — ⛔ **never an empty list** (which would save all). `is_dirty` → false after |

**Disk:** `Content/UI/WBP_CardHand.uasset` **725094 B @ 21:53:37**, sha `26166FBD…2A8A` (was 832878 B). The shrink is consistent with removing **76 nodes + 2 variables**.
**Rollback copy (pre-edit, sha `AAD0D720…E723`):** `C:\Users\wesel\AppData\Local\Temp\claude\C--GitProjects-GitHub-GitClaudeUnrealTesting-GitClaudeUnrealTest\10fcb540-8a89-457a-9798-070dabfaf278\scratchpad\WBP_CardHand.PRE-809.uasset`

**Git (⛔ I ran no Git write):** `Content/UI/WBP_CardHand.uasset` is **modified, NOT staged**. ⚠️ SCC has `Content/Input/Actions/IA_DiscardAll.uasset` **staged** and `IMC_Hero.uasset` modified — ⛔ **NOT mine**: both were written at **19:57:42 / 20:01:54**, before my session began (~21:38). They are `TASK-820`'s.

---

## 5. ⛔⛔ WHAT I COULD **NOT** VERIFY — PIXELS. THIS IS THE HONEST LIMIT.

⛔ **Everything in §4 is a property/graph/CDO read-back, and `AS-§6` A(e) is explicit that a readback is NOT a pixel.** MCP readback has repeatedly passed on visually-broken UMG on this project.

⛔ **The card bar does not exist at design time** — every slot is built by `BuildHandTree` on `EventConstruct` (`CARDBAR-§4` corrected). ⇒ **seeing it requires PIE**, and PIE is a state change on an editor whose lifecycle is the orchestrator's. ⛔ **I did not start PIE. The live bar remains unobserved.**

### 🧑 WHAT NEEDS JONATHAN'S EYE AT `TASK-811`

1. ⭐⭐ **The chip reads the REAL key `1`..`6`, and sits ABOVE each card** — the headline.
2. ⛔ **No Play button and no `"1"` discard button under any card.**
3. ⚠️⚠️ **THE FAILURE MODE TO LOOK FOR SPECIFICALLY — ⛔ ALL SIX CHIPS MISSING.** The chip is **degrade-open**: an empty `GetSlotKeyLabel` return ⇒ `Collapsed`. ⇒ if the resolver cannot reach the controller at runtime, **every chip silently vanishes and the bar looks merely "buttons deleted"** — which is ⛔ exactly what a *successful* removal also looks like. ⭐ **"No chips" is a FAILURE, not a pass** — it must not be read as "the removal worked."
4. **Chip size/padding** — font **12** is my reading of *"a bit smaller"*; it is a taste call and his to overrule.
5. **The `H` key discard-all** — `TASK-811`'s required pixel row (⛔ not mine; nothing here touches it).
6. 🧑 **`Btn_Jump`'s 240×80 bottom-centre overlap** (`§11a`) — still `Visible`, still overlapping the bar's own bottom-centre attach. ⭐ **An open question for his eye, deliberately NOT fixed by an agent.** Now that both buttons are gone the bar is shorter, so the overlap may read differently than before.
7. **Layout after two children left every column** — auto-sizing says the columns just shorten; ⛔ unverified on pixels.

---

## 6. FILES

- **Edited:** `/Game/UI/WBP_CardHand` → `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Content\UI\WBP_CardHand.uasset`
- **This handoff:** `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\.claude\pipeline\handoffs\TASK-809-artist.md`
- **Rollback copy:** `…\scratchpad\WBP_CardHand.PRE-809.uasset`
- ⛔ **Not touched:** `WBP_HUD` · `L_Arena` · `IMC_Hero` · `IA_Card1..6` · `DT_Cards` · all C++ · Git

## 7. FOR THE BUILD-MASTER (`TASK-811`)

- ⛔ **`TASK-809` and `TASK-821` must ship in the SAME commit** (`TASK-811` item 4e) — `821`'s help page says *"there is no longer any way to bin one card on its own at any price"*, which **this task is what makes true**.
- ⚠️ **LFS:** verify by **oid-vs-worktree-sha256**, ⛔ never size. Expected worktree sha256 = `26166fbd55fc8de56d9150c1f2119279afa6cfd7bb5b2dd085aba5e2d8ce2a8a`.
- ⚠️ **Finding 1** — the pre-existing `aad0d7…` save is already in the worktree diff; the commit will carry it. That is unavoidable now and is **benign** (re-serialize), but it should be **known**, not discovered.
- ⛔ **No C++ changed by me** ⇒ my work adds **zero** to the suite count.
