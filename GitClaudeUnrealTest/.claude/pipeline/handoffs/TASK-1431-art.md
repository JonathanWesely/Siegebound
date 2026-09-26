# TASK-1431 — [VICTORY-FOCUSABLE-FLIP] — art-director handoff

**Marker:** `TASK-1431-VICTORY-FOCUSABLE-FLIP`
**Re-scope honoured:** `TASK-1431-RESCOPED-TO-VERIFY-THEN-CLOSE-2026-09-25`
**Date:** 2026-09-25 · **Disposition:** `unnecessary — already flipped at 1d433ca, verified 2026-09-25`

---

## 0. The inverted gate — both digests, first

| reading | sha256 | bytes | mtime |
|---|---|---|---|
| **PRE** (before I opened anything) | `a26ad9a09d332ccd8a29df6a87d22a878b3bfd531d3199d934074addedc4b40b` | 153,825 | 2026-09-20 01:20:53 -0700 |
| **POST** (end of row) | `a26ad9a09d332ccd8a29df6a87d22a878b3bfd531d3199d934074addedc4b40b` | 153,825 | 2026-09-20 01:20:53 -0700 |

✅ **IDENTICAL — which under the re-scope is the PASS.** I wrote nothing under `Content/`. No save, no re-import, no property set, no widget compile, no PIE, no git, no editor lifecycle action. The mtime is four days old and is the flip commit's own; the file has not been re-serialised.

**Index did not move** (`git diff --cached -- GitClaudeUnrealTest/Content/` is empty), so the UE Git plugin's auto-stage never fired — independent corroboration that nothing was saved.

⚠️ Other dirt under `Content/` (`IMC_MainMenu.uasset` modified; `IA_MenuBack/Left/Right.uasset` untracked) belongs to the **input lane** (`TASK-1406`/`TASK-1409`) and was already present in this session's opening `git status`. **Not mine, not touched.**

---

## 1. The reading

**`Btn_Jump`.`IsFocusable` = `True`** in `/Game/UI/WBP_VictoryScreen`.

Read live from the editor (PID per dispatch, MCP `127.0.0.1:8000`) via read-only Python, by **object path** — `/Game/UI/WBP_VictoryScreen.WBP_VictoryScreen:WidgetTree.Btn_Jump`:

```
class=Button  IsFocusable=True  Visibility=VISIBLE  bIsEnabled=True
parent chain: Btn_Jump[Button] <- SizeBox_0[SizeBox] <- Overlay_19[Overlay]
```

**This is provably the object the code resolves.** `SiegePlayerController.cpp:2385` reads
`static const FName PlayAgainButtonName(TEXT("Btn_Jump"));` and `:2386` looks it up with
`GetWidgetFromName(PlayAgainButtonName)` — so the widget I read is the one the guard at `:2405` tests.

⇒ **The property is already satisfied. Nothing to flip. The row closes as already-discharged.**

---

## 2. My positive control — and it DID discriminate

A bare "I read `True`" is worthless if the instrument can only ever say `True`. **The same call, on the same property, on the same class, returned `False` twice:**

| widget blueprint | object path | `IsFocusable` | role |
|---|---|---|---|
| **`WBP_VictoryScreen`** | `…:WidgetTree.Btn_Jump` | ✅ **`True`** | **the subject** |
| `WBP_HUD` | `…:WidgetTree.Btn_Jump` | ⛔ **`False`** | **positive control — discriminated** |
| `WBP_CardHand` | `…:WidgetTree.Btn_Jump` | ⛔ **`False`** | **second control — discriminated** |

Identical class (`Button`), identical property name, identical `get_editor_property("IsFocusable")` call, one script, one execution. **The instrument emits both values, so the `True` is a property of the asset and not of the reader.** The controls are the two packages `qa/TASK-1464.md` §2.2 named, and they read as that report predicted.

### 2.1 Two instruments that failed *loudly*, declared rather than buried

Before the route above worked, **two earlier attempts silently measured nothing**, and each would have reported a clean "no `False` found" if I had trusted the absence:

1. `WidgetBlueprint.WidgetTree` → **protected, cannot be read** ⇒ tally `True=0 False=0` across **17/17** widget blueprints.
2. `WidgetTree.RootWidget` → **also protected** ⇒ **0/17 reached**, same empty tally.

⭐ **Both produced a zero that looked exactly like a finding and was an instrument failure.** I only caught them because I carried a reached-count and a tally instead of just a verdict. Recommend this as the cheap antidote for any future widget-tree walk: **the object-path route (`<pkg>.<pkg>:WidgetTree.<WidgetName>`) works where the property route is refused**, and `UWidget::GetParent()` is readable for walking upward.

### 2.2 The byte instrument, re-run by me — I doubted it and it survived

I re-ran the `qa/TASK-1464.md` §2.2 byte scan with **two independent matchers** (Python `bytes.count`, and `grep -o | wc -l`):

| package | `IsFocusable` occurrences | editor reading |
|---|---|---|
| `WBP_VictoryScreen.uasset` | **0** | `True` (= `UButton`'s default ⇒ nothing serialised) |
| `WBP_HUD.uasset` | **1** | `False` (≠ default ⇒ tag written) |
| `WBP_CardHand.uasset` | **1** | `False` (≠ default ⇒ tag written) |

⭐ **The two instruments agree and explain each other.** UE serialises a tagged property only when it differs from the class default, and `UButton`'s default is `true` ⇒ absence of the tag *is* `True`. Mid-row I suspected the `IsFocusable` hits in the controls were merely Blueprint function names in a sorted name table (their neighbours are `IsEmpty`, `Is Valid`, `IsOvertimeActive`); **the live reading refuted my suspicion and vindicated QA's byte argument.** Recording that I challenged it and lost.

---

## 3. What I measured myself vs what I received

| fact | status |
|---|---|
| pre/post `sha256` + byte length + mtime | ✅ **measured by me** |
| index did not move | ✅ **measured by me** |
| `Btn_Jump`.`IsFocusable` = `True` in the live editor | ✅ **measured by me** |
| positive controls returning `False` | ✅ **measured by me** |
| byte-level tag presence/absence, 2 matchers | ✅ **measured by me** |
| `PlayAgainButtonName == "Btn_Jump"` at `SiegePlayerController.cpp:2385` | ✅ **measured by me** (read-only grep) |
| working tree == `HEAD` for this asset | ✅ **measured by me**, LFS **oid-vs-sha256**, never size |
| `HEAD` blob == `1d433ca` blob for this asset | ✅ **measured by me** |
| `1d433ca` is what changed it | ✅ **measured by me** — `1d433ca~1` oid `7817dd7f…` / 153,489 B → `1d433ca` oid `a26ad9a0…` / 153,825 B |
| `TASK-1461`'s three readings · `TASK-1463` (1b) | ⛔ **received**, not re-run |
| `TASK-1413`'s runtime observation | ⛔ **received — and see §4, I think it is misattributed** |

### 3.1 The git limb, and the trap I hit on the way (`SC-§102`)

My first `git status`/`cat-file` pass was **anchored one level too deep** and answered with **silence and a `fatal: path does not exist`** for a file that is tracked and unmodified. **The repo root is `C:/GitProjects/GitHub/GitClaudeUnrealTesting`; the in-repo path is `GitClaudeUnrealTest/Content/UI/WBP_VictoryScreen.uasset`.** Re-anchored, everything resolved. `SC-§102` earned its place again.

Second trap on the same limb: `git cat-file -p HEAD:<path>` returns **`9a20d465…`**, which is *not* the working file's digest — because the blob is a **131-byte LFS pointer**. Compared correctly (pointer `oid sha256:` vs working-tree `sha256`) they match exactly. **A naive blob-digest comparison here would have read as "the working tree is modified" and manufactured a false alarm.**

---

## 4. 🚨 A finding I am NOT acting on — the `TASK-1413` corroboration is probably about a different widget

The dispatch and the board cite, as ⭐ NEW runtime corroboration, that `TASK-1413` saw `Btn_Jump` as `Collapsed` / `realized: false` at `Overlay_19/SizeBox_0/Btn_Jump` (`qa/TASK-1413-verify.md:83-84`).

**That path is not unique.** I measured the parent chain of all three `Btn_Jump`s:

| package | chain | authored `Visibility` |
|---|---|---|
| `WBP_VictoryScreen` | `Btn_Jump <- SizeBox_0 <- Overlay_19` | **`Visible`** |
| `WBP_HUD` | `Btn_Jump <- SizeBox_0 <- Overlay_19` | `Visible` |
| `WBP_CardHand` | `Btn_Jump <- SizeBox_0 <- Overlay_19` | **`Collapsed`** |

⛔ **All three are byte-for-byte the same path, name and structure — a UE template leftover.** The one whose authored visibility is **`Collapsed`** is **`WBP_CardHand`'s**, which *is* on screen during a match; the victory screen's reads **`Visible`** and the victory screen is *not* up mid-match. ⇒ **`TASK-1413` most likely snapshotted `WBP_CardHand`'s `Btn_Jump`, not this row's.**

**This does not change the disposition** — my own direct reading plus the byte evidence plus `1d433ca`'s diff settle the row without it. But:
- the fourth cited reading should be **struck from this row's evidence chain** rather than counted, and
- ⚠️ **`Overlay_19/SizeBox_0/Btn_Jump` is a name-collision hazard for any `ui_snapshot`-based focus walker** — a walker that matches on that path cannot tell which widget it found. Worth a manager row; **I have not boarded one** (not my lane).

---

## 5. Spec limbs that remain live — nobody may report victory twice

- ⚠️ **Spec (4) is UNCHANGED and STILL OPEN.** The victory screen runs under `FInputModeUIOnly`, and whether Enhanced Input is deaf under it is **`TASK-1429` (5)'s** question. **Closing this row does NOT make the screen drivable, and I am not claiming it does.**
- ⚠️ `TASK-1413` recorded the live `Btn_Jump` as **`realized: false`** — the Slate widget had not been constructed — so **`SupportsKeyboardFocus()` has still never been exercised at runtime on this screen.** My reading is of the **authored asset**, which is what the row asked for; it is not a runtime observation.
- ⛔ **The (3) human keystroke is spent and no longer owed** — this row writes nothing.
- ⛔ The census could not reach this screen (it needs a **match** to conclude); that debt is unchanged by this row.

---

## 6. ⚠️ Stale downstream instruction the manager should repair (not mine to edit)

`TASK-1436`'s `blocked-by` currently reads: *"`TASK-1431` COMPLETE (with its `sha256` **CHANGED**)"*. **That is now exactly backwards** — this row's correct outcome is an **IDENTICAL** digest. A faithfully-followed stale gate would block a correct wave at 5b. The row's own `ROUTING` line (5b = `TASK-1436`, commit = `TASK-1437`) is likewise written for an edit that no longer exists.

Also live, per the re-scope's own note: the wave's **"2 code + 1 asset"** budget in `handoffs/TASK-1398-programmer.md` §D counted *this* row as the asset. With it closing `unnecessary`, re-sizing the wave is 🧑 **Jonathan's call** — boarded by the manager, not decided by me.

---

## 7. Assets

**None created, none modified, none imported.** This row produced **a reading, not an artifact**.

- Asset inspected (read-only): `/Game/UI/WBP_VictoryScreen` → `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Content\UI\WBP_VictoryScreen.uasset`
- Controls inspected (read-only): `/Game/UI/WBP_HUD`, `/Game/UI/WBP_CardHand`
- Files I wrote: **this handoff** + **`TASK-1431`'s `status:` line only**.
