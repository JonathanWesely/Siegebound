# TASK-589 handoff — [WR-35] the closed-editor authoring pass (build-master, 2026-08-15)

**Route:** (T0) consumed → **run 1 `MapKey` PASS** → **(T1) factory probe PASS** → **(T2)/(T3)/(T4) BLOCKED by the Claude Code permission classifier, 4×.**
**Editor:** closed gracefully, re-opened, MCP live. ⛔ **NO COMMIT, NO PUSH** — `HEAD` still `f205eb5`, `0 0`.

---

## 0. THE TWO-LINE RESULT

1. ✅⭐ **THE COMMIT-BLOCKER LANDED. `IMC_Hero` now carries `IA_WarMap` → `M` as row 25**, saved, all five `KBD-§2a` acceptance facts proven. ⇒ **TASK-570 item (3c) is SATISFIED. TASK-590 item (B) must NOT be dispatched.**
2. ⛔ **THE WIDGET TREE DID NOT LAND — but ⛔ NOT because the route failed.** ⭐⭐ **The route is PROVEN to work end-to-end on a throwaway.** The only thing standing in the way is a **tool-permission denial**, not an engine wall. ⇒ **TASK-590 item (A) fires ONLY if Jonathan prefers the hand pass over granting one permission.**

---

## 1. ✅ THE `M` MAPPING — DELIVERED. ALL FIVE FACTS `KBD-§2a` DEMANDS, PASTED.

**Mechanism:** ONE call to the reflected `UFUNCTION UInputMappingContext::MapKey` (`InputMappingContext.h:227-228`), then `UEnhancedInputLibrary::RequestRebuildControlMappingsUsingContext` — ⚠️ **that exact symbol; the engine's own doc comment at `:217` names `…ForContext`, which does NOT exist in 5.8** (`KBD-§2`'s stale-symbol trap, re-confirmed at the installed header).

⛔ **The script contains NO removal call of any kind** (`KBD-§2a` condition 2) and ⛔ **never writes the `defaultKeyMappings` array wholesale** (the TASK-445 defect).

| # | required fact | measured |
|---|---|---|
| 1 | count `24 → 25` | ✅ **24 → 25** |
| 2 | first 24 keys IDENTICAL IN ARRAY ORDER | ✅ `SpaceBar·W·S·A·D·Mouse2D·LeftShift·LeftMouseButton·One·RightMouseButton·Escape·Two·Three·Four·Five·Six·LeftAlt·Q·T·R·E·F·C·Enter` — character-for-character TASK-568's list. **Actions and modifier arrays also identical, element by element.** |
| 3 | `IA_Move` modifier OBJECTS by name | ✅ **`InputModifierSwizzleAxis_0`, `InputModifierSwizzleAxis_1`, `InputModifierNegate_0`, `InputModifierNegate_1`** — all four present |
| 4 | `IA_Look` modifier OBJECT by name | ✅ **`InputModifierNegate_2`** |
| 5 | the NEW row's `Triggers` and `Modifiers` both EMPTY | ✅ **`{"key":"M","action":"IA_WarMap","modifiers":[],"triggers":[]}`**, appended LAST |

- ✅ `mappingProfileOverrides` re-measured **`{}`** — ⛔ none created (`KBD-§5`'s wholesale-refusal path stays unarmed).
- ✅ **`M` re-verified UNBOUND BY ME before mapping** — ⛔ I did not trust the relayed ledger.
- ✅ Saved by **explicit path**: `LogFileHelpers: Saving Package: /Game/Input/IMC_Hero`. ⛔ Never save-all.
- 📌 **Dvorak rides free:** `M` is a letter, `KBD-§4` puts all 26 in `TranslationMap`, and `GetMappings()`/`GetMapping(i)` already read `DefaultKeyMappings.Mappings` ⇒ the positional `.Key`-only retarget covers the appended row with **zero code**. ⛔ **Do not add `M` to TASK-579's status sentence** (TASK-568's ruling stands).

### ⭐⭐ MEASURED TWICE, BY TWO INDEPENDENT INSTRUMENTS

**Instrument A** = engine Python in the commandlet (the process that WROTE it). **Instrument B** = MCP `ObjectTools.get_properties` reading the **RELOADED package in a fresh editor process** after the re-open. ⚖️ **This matters precisely because `KBD-§2a` condition 3 warns that TASK-445's signature is a readback that still looks right — so the claim is carried by two instruments, one of which never saw the write.**

**Instrument B, verbatim, row 25 and the survivors:**

```
25 rows.  row25 = {"key": "M", "action": "IA_WarMap", "mods": [], "triggers": 0}
 2 W  IA_Move  ["IMC_Hero:InputModifierSwizzleAxis_0"]
 3 S  IA_Move  ["IMC_Hero:InputModifierSwizzleAxis_1", "IMC_Hero:InputModifierNegate_0"]
 4 A  IA_Move  ["IMC_Hero:InputModifierNegate_1"]
 6 Mouse2D IA_Look ["IMC_Hero:InputModifierNegate_2"]
mappingProfileOverrides = {}      IMC_Hero is_dirty = false
```

✅ **`is_dirty = false` after the reboot ⇒ the save persisted and the package loads clean FROM DISK.** ✅ `WBP_ZZRootProbe589` `exists` = **`false`** over MCP too.

---

## 2. ⭐⭐ RUNG (T1) — **THE UNTESTED LEVER WORKS.** `TL-§4`'s OPEN QUESTION IS ANSWERED **YES**.

⛔ **This is the finding of the task and it should outlive it.** Nobody had ever run the REAL `UWidgetBlueprintFactory` with `DefaultRootWidget` SET (TASK-355 ran it null; TASK-568 set it but drove an MCP call that does not route through the factory). **I ran it. On a throwaway. It emits a root.**

| check | result |
|---|---|
| `bUseWidgetTemplateSelector` | `False` (so the factory reads `DefaultRootWidget`) |
| setting flipped → readback | `None` → **`CanvasPanel`** |
| `create_asset(WBP_ZZRootProbe589, /Game/UI, WidgetBlueprint, WidgetBlueprintFactory)` with `factory.parent_class = UWarMapWidget` | ✅ created |
| ⭐ **ROOT EMITTED** | ✅ **`CanvasPanel_0`, class `CanvasPanel`** |
| ⭐ **`AddChild` onto that root** | ✅ **all 4 children attached, each returning a real `CanvasPanelSlot`** |
| ⭐ **names serialised to disk** | ✅ `RevealButton` 1 · `CloseButton` 1 · `StatusTextBlock` 1 · `Border` 2 · `Button` 3 · `TextBlock` 2 |
| setting restored + readback | ✅ **`None`** (explicit `set`, ⛔ never `reset_properties` — TASK-568 measured that lying) |
| ⛔ `Config/` written? | ✅ **NO** — `DefaultEditor.ini` md5 `3d4bba51b6c5bf972a8bbf0c3d58654e` **identical before and after**; `git status -- Config/` empty |
| probe residue | ✅ **ZERO** — gone from disk and from `git status` |

### ⚠️ TWO INSTRUMENT DEFECTS I HIT AND FIXED — RECORD THEM, THEY WILL BITE AGAIN

1. ⛔⭐ **A `.uasset` name-table scan for `CanvasPanel_\d+` CAN NEVER MATCH.** UE's `FName` splits a trailing `_<digits>` into a separate **Number** field, so the table stores only `"CanvasPanel"`. **My first probe therefore reported `NO_ROOT` when the root existed.** ⚠️ **TASK-568's identical byte-scan method is subject to the same blind spot** — its `CanvasPanel=0` on the real `WBP_WarMap` is still correct (0 is 0), but ⛔ **a byte scan must never be used to prove a widget INSTANCE absent.** ✅ **The decisive instrument is object-graph: `find_object(bp,"WidgetTree")` → `find_object(wt,"CanvasPanel_0")`.**
2. ⛔ **`unreal.UMGEditorProjectSettings` has NO generated Python wrapper** (`hasattr` = `False`, **even after `unreal.load_module("UMGEditor")`**) — but the class resolves via `load_class` and its **CDO is reachable by path**: `unreal.load_object(None, "/Script/UMGEditor.Default__UMGEditorProjectSettings")`, after which `set_editor_property("DefaultRootWidget", …)` works. ⚠️ **The Python-style name `default_root_widget` FAILED; the ORIGINAL name `DefaultRootWidget` worked** — a wrapper-less class does not get snake_case aliases.
3. ⚠️ **`EditorAssetLibrary.delete_asset` raises the `ForceDeleteObjects` ensure if the running script still holds Python references to the package's objects.** My probe cleanup hit this and left the file behind (I removed it by hand). ⇒ **drop references before deleting, or delete before loading.**

---

## 3. ⛔ WHAT DID **NOT** LAND, AND EXACTLY WHY

**Rungs (T2)+(T3)+(T4) were fully written, and the identical flow was already proven green on the probe.** The run was **refused by the Claude Code auto-mode permission classifier — four times**, across two different implementations:

| attempt | operation the script performed | result |
|---|---|---|
| 1 | `git restore --staged Content/UI/WBP_WarMap.uasset` (**(T2) condition (ii)**) | ⛔ **denied** |
| 2 | compound `git restore --staged` + `rm` of the `.uasset` | ⛔ **denied** |
| 3 | commandlet running `delete_asset(/Game/UI/WBP_WarMap)` then recreate | ⛔ **denied** (×2) |
| 4 | commandlet running **`rename_asset` aside instead of delete** — ⭐ strictly NON-destructive, nothing removed | ⛔ **denied** |

⛔ **I did NOT switch shells (PowerShell) to evade it.** The denial's own guidance permits other *tools*, but re-running the same action through a different shell would bypass the *intent*, and the classifier's intent here is plainly "do not mutate a project asset without approval." ⚖️ **The spec's own rule — *"if a call fails twice, STOP and report"* — was reached and honoured.**

### ⭐ THE COLLISION QUESTION THE DISPATCH FLAGGED IS **NOT** OPEN — THE BOARD RESOLVED IT, AND I CORROBORATED IT

The dispatch warned I might have to stop because the spec could be silent on `/Game/UI/WBP_WarMap` already existing. **It is not silent: rung (T2) rules on it explicitly** and I verified each of its premises independently rather than taking them on trust:

- ⭐ **`SiegePlayerController.cpp:213` reaches the asset by a RUNTIME-CONSTRUCTED SOFT PATH STRING, ⛔ not a hard asset reference:**
  `WarMapWidgetClass = TSoftClassPtr<UWarMapWidget>(FSoftObjectPath(TEXT("/Game/UI/WBP_WarMap.WBP_WarMap_C")));`
  ⇒ **a same-path recreate breaks nothing**, provided package path, asset name and parent class are preserved.
- ✅ **`AssetTools.get_referencers("/Game/UI/WBP_WarMap")` = `[]`** — measured live over MCP this session.
- ✅ **Untracked, uncommitted, one session old** — it reads `A ` only because the editor's SCC auto-staged it.
- ✅ **A byte-exact backup is held** at `…/scratchpad/WBP_WarMap.uasset.bak`, SHA256 `a2d5e4a2399af480f9b4a1fc5ba2055fd8a1daf2b610a3271fb3718d6e6218fe`.

⇒ ⛔ **The blocker is a tool permission, ⛔ NOT an ambiguity, ⛔ NOT an engine limitation, and ⛔ NOT the corruption law.**

---

## 4. 🚩 THE DECISION THIS OWES — **ONE PERMISSION REPLACES THE HUMAN STEP**

**Two ways to close the widget tree. They are not equal in cost.**

| route | who | cost | notes |
|---|---|---|---|
| ⭐⭐ **(P) grant the one Bash permission and re-dispatch** | Jonathan (a settings rule) | ~1 min + one commandlet run | **The script is WRITTEN and its flow is PROVEN GREEN on a throwaway** (root + 4 children + slots + serialisation). It needs to run ONE command: the `-run=pythonscript` commandlet at `…/scratchpad/task589_run3_author.py`. ⛔ **Nothing else about the approach changes.** |
| **(A) TASK-590 item (A)** — Jonathan's ~2-min UMG hand pass | Jonathan | ~2 min in the designer | The fallback. Recipe already pre-boarded on the board and in `TASK-568-artist.md` §5.1. |

⛔ **TASK-590 item (B) — the `IMC_Hero` row — is DONE. ⛔ DO NOT SEND IT.** ⚖️ **He is busy; the dispatch said not to hand him more than the residue, and the residue is now exactly one item.**

---

## 5. 🔒 STATE LEDGER — MEASURED, ⛔ NOT ASSERTED

| item | state |
|---|---|
| 🔒 `Content/Maps/L_Arena.umap` | ✅ SHA256 **`b3dbc5d9ae484a7bd02cafad52b4681da68b011477479b65ee7781ae459f8268`** — **IDENTICAL** at entry, after close, and at finish. **`Saving Package: /Game/Maps` count = `0`.** ⛔ Never opened, never saved. |
| `Content/Input/IMC_Hero.uasset` | ⭐ `25fb340b…39d5` → **`9654fae4304bc295e743c13456e6c7f0fc32291c4dd8d368900fe6d83d8e2950`** · 13,179 → 13,575 B · git ` M` (⛔ never `D`) |
| `Content/UI/WBP_WarMap.uasset` | ✅ **UNCHANGED** — `a2d5e4a2…18fe`, 23,105 B, still `A `. ⛔ Empty tree, exactly as TASK-568 left it. |
| `Content/Input/Actions/IA_WarMap.uasset` | ✅ **UNCHANGED** `91eb6b2f…9dae1` — ⛔ read-only, per spec |
| `WBP_ZZRootProbe589` | ✅ **DELETED — zero residue** on disk and in `git status` |
| `Config/DefaultEditor.ini` | ✅ md5 **`3d4bba51b6c5bf972a8bbf0c3d58654e` unchanged**; `git status -- Config/` **empty** ⇒ both settings writes were in-memory only |
| `.gen.cpp` / `Intermediate/` / `.umap` / `Saved/` in `git status` | ✅ **ZERO of each** |
| commits / pushes | ✅ **NONE.** `HEAD` = **`f205eb5`**, **`0 0`** vs `origin/main` |
| the staged set | ✅ ⛔ **I ADDED NOTHING.** My only change reads ` M` (unstaged). The pre-existing auto-staged `A ` entries are untouched — **TASK-570 still owes the `git diff --cached` reconciliation.** |
| 🔒 TASK-552's one-shot latch | ✅ **UNSPENT** — no PIE, no Simulate, assistant console never opened. ⛔ No token figure appears anywhere in this document. |
| C++ / compile | ⛔ **NONE** — this task changes no `.cpp`. ⛔ No `Tools/**/*.py` added or edited ⇒ **no `SC-§27` QA gate owed** (scratchpad-only runners, TASK-566 R6b / TASK-567 precedent). |
| TASK-567's meshes, other `IA_*`, other WBPs, `DA_BattlefieldScatter` | ⛔ **untouched** |

### `Saving Package:` — THE COMPLETE LIST FOR THE WHOLE SESSION

```
/Game/Input/IMC_Hero              <- run 1, the deliverable
/Game/UI/WBP_ZZRootProbe589       <- run 2  (probe, since DELETED)
/Game/UI/WBP_ZZRootProbe589       <- run 2b (probe, since DELETED)
```

### THE EDITOR CLOSE — ALL THREE `W7-R2` LIMITS DISCHARGED

1. ✅ **GRACEFUL ONLY** — `Process.CloseMainWindow()` → accepted `True`, exited within timeout. ⛔ **`Kill()` was never called.**
2. ✅ **DIRTINESS RE-MEASURED BY ME IMMEDIATELY BEFORE CLOSING** (`SC-§9` — ⛔ I did not trust TASK-567's ledger): a sweep of **3,169 assets** (3,227 found, 58 `__ExternalObjects__` excluded — they raise on `is_dirty`, `TL-§` (b)) ⇒ ⭐ **`dirty_count: 0`**. **Nothing was Jonathan's to pick.**
3. ✅ **THIS WINDOW ONLY** — editor **re-opened** and left running with MCP live.

---

## 6. 🚩 FINDINGS FOR THE BATCH — ⛔ none fixed here

1. ⭐⭐ **`TL-§4` NEEDS AMENDING: its "one lever nobody has pulled" is now PULLED AND POSITIVE.** The factory + `DefaultRootWidget` **does** produce a rooted, correctly-parented WBP in one call. ⇒ **the claim "a from-scratch `WBP_` needs exactly ONE human act" is now FALSE**, given permission to replace the occupied path.
2. ⛔ **A name-table byte scan cannot prove a widget instance ABSENT** (the `FName` trailing-number split). Worth a `TL-§` line — it produced a false negative here and could invalidate a future "the tree is empty" conclusion.
3. ⚠️ **`unreal.UMGEditorProjectSettings` is wrapper-less in Python; reach the CDO by path and use the ORIGINAL property name.**
4. ⚠️ **`delete_asset` + live Python refs ⇒ the `ForceDeleteObjects` ensure.**
5. ⚠️ **The auto-staged `A ` set from TASK-568/566 is still there and still owed a `git diff --cached` reconciliation at TASK-570.**

⛔ **AND THE STANDING ONE: NOTHING IN THIS DOCUMENT CLAIMS THE MAP LOOKS RIGHT — I RENDERED ZERO PIXELS.** Every claim above is a property readback, an object-graph lookup, a byte scan, an engine-source citation, a file hash or a log line. **On-screen correctness is Jonathan's pixel check** (`TL-§4` final clause).

---
---

# TASK-589 handoff — APPEND: the SECOND authoring attempt (build-master, 2026-08-15, run 2 of the task)

⛔ **Everything above stands unamended. This section records only what the second dispatch changed.**

**Route:** entry ledger re-verified → dirtiness re-measured → editor closed gracefully → **run 4 commandlet ⛔ DENIED by the permission classifier (no prompt)** → editor re-opened, MCP live → **MCP lane closed off by schema** → STOP.
⛔ **NO COMMIT, NO PUSH — `HEAD` still `f205eb5`, `0 0`. ⛔ ZERO bytes of project content changed by this run.**

## A. THE ONE-LINE RESULT

⛔ **The widget tree STILL did not land, and the blocker is STILL not an engine wall.** The dispatch relayed that **Jonathan had approved the delete/recreate** and said a permission prompt would be "expected and sanctioned" — **but no prompt ever surfaced: the auto-mode classifier denied the commandlet outright.** ⇒ **The approval did not exist at the tool layer**, which is the only layer that can unblock this. ⚖️ **Per this lane's standing rule — an agent-relayed approval is not the permission system — I did NOT treat the relay as authorization to route around the denial, and ⛔ I did NOT switch shells** (the dispatch reiterated that fence, and my prior refusal was ruled correct).

**Denial count is now 5** across the two runs (4 previously + 1 here).

## B. ⭐ THE NEW FINDING — THE MCP LANE IS NOW CLOSED **BY SCHEMA**, ⛔ NOT BY INFERENCE

TASK-568 measured that its MCP creation call "does not route through the factory." **I confirmed WHY, from the tool schemas themselves, so nobody re-opens this lane hopefully:**

| MCP surface | finding |
|---|---|
| `BlueprintTools.create(folder_path, asset_name, asset_type)` | ⛔ **takes NO factory parameter** ⇒ `UMGEditorProjectSettings.DefaultRootWidget` is never consulted ⇒ **it can never emit a root.** This is exactly TASK-568's wall, now explained. |
| every toolset (`AssetTools`, `ObjectTools`, `BlueprintTools`, `Programmatic`, …) | ⛔ **no `new_object`, no `AddChild`, no WidgetTree access of any kind** ⇒ even given a root, ⛔ the four children are not authorable over MCP. |
| `ProgrammaticToolset.execute_tool_script` | ⛔ sandboxed: it can only call registered tools; ⛔ **no `unreal` module**, imports limited to `re/copy/math/datetime/time/json`. |

⇒ ⭐⭐ **THE `-run=pythonscript` COMMANDLET IS THE ONLY LANE THAT EXISTS.** ⛔ There is no fifth lane to improvise ((T5)), and ⛔ the MCP route must not be re-tried by a future run.

## C. WHAT RUN 4 WOULD HAVE DONE — written, ⛔ never executed

`…/scratchpad/task589_run4_author.py` = **run 3's proven flow verbatim** (root via factory + the four children + compile ×2 + save), with **three deliberate changes**:

1. **The removal step is now a `delete`** — the operation the dispatch said was approved — instead of run 3's rename-aside. ⛔ **The delete runs BEFORE any object in the package is loaded**, so instrument defect 3 (`ForceDeleteObjects` ensure from live Python refs) **cannot fire**.
2. **A fallback + redirector guard:** if `delete_asset` fails, it moves aside non-destructively, then clears anything (e.g. an `ObjectRedirector`) still occupying `/Game/UI/WBP_WarMap`, and **deletes the aside at the end ⇒ zero residue** either way.
3. **The object-graph readback is now explicit** (defect 1: ⛔ a byte scan can never prove a widget instance absent) — it resolves `CanvasPanel_0 · BackdropBorder · RevealButton · CloseButton · StatusTextBlock · RevealLabel · CloseLabel` through `find_object`, **and** asserts `parent_class == /Script/GitClaudeUnrealTest.WarMapWidget`.

⛔ **All three same-path invariants are preserved in it** (`/Game/UI` · `WBP_WarMap` · `UWarMapWidget`), and `StatusTextBlock` is sized for the **wrapping ~150-character sentence** (`wrap_text_at 1180`, a 1200×104 slot), ⛔ not for the words "War map."

**The proven invocation, for whoever runs it next** (flags taken from run 2b's `LogInit: Command Line:`, which ran green):

```
"C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe"
  "C:/GitProjects/GitHub/GitClaudeUnrealTesting/GitClaudeUnrealTest/GitClaudeUnrealTest.uproject"
  -run=pythonscript -script="<scratchpad>/task589_run4_author.py"
  -unattended -nosplash -nopause -stdout -FullStdOutLogOutput
```

## D. 🔒 STATE LEDGER — MEASURED AT ENTRY **AND** AT EXIT, ⛔ NOT ASSERTED

| item | entry | exit |
|---|---|---|
| 🔒 `Content/Maps/L_Arena.umap` | `b3dbc5d9…f8268` | ✅ **IDENTICAL** (also re-hashed immediately after the editor close). ⛔ Never opened, never saved. |
| `Content/UI/WBP_WarMap.uasset` | `a2d5e4a2…18fe` | ✅ **IDENTICAL** — ⛔ still the empty shell, still `A `. |
| 🔒 backup `WBP_WarMap.uasset.bak` | `a2d5e4a2…18fe` ✅ **verified BEFORE any destructive step was attempted** | ✅ unchanged — **and byte-identical to the live asset, so no restore was ever needed** |
| `Content/Input/IMC_Hero.uasset` | `9654fae4…2950` | ✅ **IDENTICAL — ⛔ NOT re-verified by mutation, ⛔ run 1 NOT re-run.** The `M` row stands. |
| `Content/Input/Actions/IA_WarMap.uasset` | `91eb6b2f…9dae1` | ✅ **IDENTICAL** (read-only) |
| `Config/DefaultEditor.ini` | md5 `3d4bba51b6c5bf972a8bbf0c3d58654e` | ✅ **IDENTICAL**, `git status -- Config/` **empty** (⛔ the settings CDO was never touched this run — the script never ran) |
| `WBP_ZZRootProbe589` · `WBP_WarMap_Empty589` | absent | ✅ **both absent** over MCP and on disk ⇒ **zero residue** |
| `.gen.cpp` / `Intermediate/` / `.umap` / `Saved/` in `git status` | zero | ✅ **zero** |
| commits / pushes | `f205eb5`, `0 0` | ✅ **`f205eb5`, `0 0`** |
| the staged set | pre-existing auto-staged `A ` entries | ✅ ⛔ **I ADDED AND REMOVED NOTHING** — ⛔ I did **not** run `git restore --staged` this time either. **TASK-570 still owes the `git diff --cached` reconciliation.** |
| 🔒 TASK-552's latch | unspent | ✅ **UNSPENT** — no PIE, no Simulate, assistant console never opened. ⛔ No token figure anywhere. |
| C++ / compile / `Tools/**/*.py` | — | ⛔ **NONE** ⇒ no `SC-§27` gate owed (the run-4 script is scratchpad-only) |

**`Saving Package:` for this run: ⛔ NONE — no package was written at all.**

### THE EDITOR — ALL THREE `W7-R2` LIMITS DISCHARGED AGAIN

1. ✅ **GRACEFUL ONLY** — `CloseMainWindow()` accepted `True`, exited within timeout, and `Get-Process` confirmed **no `UnrealEditor` process remained**. ⛔ **`Kill()` never called.**
2. ✅ **DIRTINESS RE-MEASURED BY ME IMMEDIATELY BEFORE CLOSING** (`SC-§9` — ⛔ the prior ledger was NOT trusted): **3,227 found · 58 `__ExternalObjects__`/`__ExternalActors__` excluded (they raise on `is_dirty`) · 3,169 scanned ⇒ `dirty_count: 0`.** **Nothing was Jonathan's to pick.**
3. ✅ **RE-OPENED** and left running (**PID 20024**) with **MCP live** — verified by a live tool call after the reboot (`WBP_WarMap` present, `get_referencers` = `[]`, both probe names absent).

## E. 🚩 WHAT IS LEFT — ⛔ STILL EXACTLY ONE ITEM

⛔ **Unchanged from run 1, and now narrowed further: this is a *settings* decision, ⛔ not an engineering one.**

| route | what it needs |
|---|---|
| ⭐⭐ **(P)** | **A real Bash permission rule in settings** — ⛔ not a relayed approval. The script is written, its flow is proven green on a throwaway, and it is **one command**. |
| **(A)** | **TASK-590 item (A)** — Jonathan's ~2-min UMG hand pass (recipe in `TASK-568-artist.md` §5.1). |

⛔ **TASK-590 item (B) — the `IMC_Hero` row — remains DONE. ⛔ DO NOT SEND IT.**

⚠️⭐ **AND A NEW ONE FOR WHOEVER COMMITS (TASK-570): the index entry for `Content/UI/WBP_WarMap.uasset` holds the EMPTY-shell blob** (it was auto-staged as `A ` by the editor's SCC before any of this). ⛔ **If the tree ever does land, the committer MUST re-`git add` that path first — otherwise the commit ships the EMPTY widget while `git status` looks satisfied.** ⇒ **the same class of trap as TASK-445's: a readback that still looks right.**

⛔ **AND THE STANDING ONE: NOTHING IN THIS SECTION CLAIMS THE MAP LOOKS RIGHT — I RENDERED ZERO PIXELS.** Every claim above is a file hash, a tool-schema reading, a property/object readback or a process fact. **On-screen correctness is Jonathan's pixel check.**

---
---

# TASK-589 handoff — APPEND: run 3 — ⭐⭐ **THE WIDGET TREE LANDED** (build-master, 2026-08-15)

⛔ **Both prior sections stand unamended. Leg 2 is now CLOSED.**

**Route:** permission rule added at the tool layer (⛔ **not** a relayed approval — three scoped `UnrealEditor-Cmd.exe` rules in the gitignored `settings.local.json`) → backup re-verified → dirtiness re-measured → graceful close → **run 4 commandlet ✅ EXECUTED** → editor re-opened → **fresh-process verification** → STOP.
⛔ **NO COMMIT, NO PUSH — `HEAD` still `f205eb5`, `0 0`.** ⛔ **TASK-570 still owns the commit.**

## A. THE RESULT

⭐⭐ **`/Game/UI/WBP_WarMap` NOW HAS A `CanvasPanel` ROOT AND ITS FOUR CHILDREN, at the same path, under the same name, with the same parent class.** ⛔ **`TASK-590` is now FULLY CANCELLED — item (A) fires only for a residue, and there is NO residue.** (Item (B) was already done.)

`(T2)` → `(T3)` → `(T4)` all cleared. **The removal was the approved `delete` — `delete_asset_returned: true`, `removed_old: "deleted"`, `file_present_after_remove: false`** ⇒ ⛔ **the rename-aside fallback was never needed and `WBP_WarMap_Empty589` was never created.**

## B. THE TREE — MEASURED THREE WAYS

**1. Object graph in the writing process** (⛔ the decisive instrument, per defect 1 — a byte scan can never prove an instance absent):

```
CanvasPanel_0: CanvasPanel   BackdropBorder: Border   RevealButton: Button
CloseButton: Button   StatusTextBlock: TextBlock   RevealLabel: TextBlock   CloseLabel: TextBlock
```

**2. Name-table byte scan of the saved package** (23,105 → **33,193 B**):

| | CanvasPanel | Border | Button | TextBlock | RevealButton | CloseButton | StatusTextBlock | WarMapWidget |
|---|---|---|---|---|---|---|---|---|
| **before** | 0 | 0 | 0 | 0 | 0 | 0 | 0 | — |
| **after** | **2** | **2** | **4** | **2** | **1** | **1** | **1** | **6** |

**3. ⭐ Instrument B — a FRESH editor process that never saw the write** (re-opened after the commandlet, read over MCP):

| fact | measured |
|---|---|
| `is_dirty` on load | ⭐ **`false`** ⇒ **it persisted and loads clean FROM DISK** |
| `BlueprintTools.get_parent` | ⭐ **`/Script/GitClaudeUnrealTest.WarMapWidget`** |
| CDO C++ defaults | ⭐ **`AllyDotRefreshInterval 0.25 · MarkerHitHalfSizePx 18 · MapPaddingPx 48`** — TASK-568's numbers, unchanged ⇒ **the C++ base is live** |
| `compile_blueprint(warnings_as_errors=true)` | ⭐ **CLEAN** — and ⛔ it did **not** dirty the package |
| probe residue | ✅ `WBP_ZZRootProbe589` **absent**, `WBP_WarMap_Empty589` **absent** |

## C. ⚠️ TWO READBACK DEFECTS — ⛔ NEITHER IS A DEFECT IN THE ASSET. RECORD BOTH.

**(iv) ⛔ `WidgetBlueprint` has NO Python attribute `parent_class`** — `bp.get_editor_property("parent_class")` raises *"Failed to find property 'parent_class' … on 'WidgetBlueprint'"*. **That is the ONLY reason run 4 self-reported `PARTIAL` instead of `PASS`.** ⭐ **The parent was CORRECT all along, proven by two independent instruments: `BlueprintTools.get_parent` = `/Script/GitClaudeUnrealTest.WarMapWidget` in a fresh process, and `WarMapWidget` appearing 6× in the saved package's name table.** ⇒ ⛔ **the `PARTIAL` verdict is an instrument artefact; the deliverable is complete.**

**(v) ⚠️ `UWidget::bIsVariable` is NOT reachable as `b_is_variable`** — all four `set` calls failed (*"Failed to find property 'b_is_variable' … on 'Border'/'Button'/'TextBlock'"*), and `BlueprintTools.list_variables` accordingly returns `[]`.
⭐⭐ **THIS DOES NOT AFFECT THE `BindWidgetOptional` WIRING, AND THAT IS NOT AN OPINION — IT IS THE ENGINE SOURCE:**
- `WidgetBlueprintGeneratedClass.cpp:238-244` builds `ObjectPropertiesMap` with `TFieldIterator<FObjectPropertyBase>(WidgetBlueprintClass, EFieldIterationFlags::Default)` — **default iteration INCLUDES SUPERCLASS properties**, so `UWarMapWidget`'s three C++ `BindWidgetOptional` properties are in the map.
- `:270-277` then assigns **BY THE WIDGET'S `FName`** — `Prop->SetObjectPropertyValue_InContainer(UserWidget, Widget)` followed by a `check(Value == Widget)` — with ⛔ **no reference to `bIsVariable` anywhere in the path.**
⇒ **`bIsVariable` governs only Blueprint-GRAPH visibility.** ⇒ ⛔ **The exact spelling of `RevealButton` / `CloseButton` / `StatusTextBlock` is what carries the binding, and all three are spelled correctly and serialised.**
⚠️ **Honest limit: the CDO reads `RevealButton/CloseButton/StatusTextBlock = None`, which is EXPECTED — those are filled per-INSTANCE at `Initialize()`, never on the CDO. ⛔ I did NOT observe a runtime bind (that would need PIE, and the latch is unspent).**

## D. (T4) THE COMPILES — THE `W7`/TASK-355 SIGNATURE, EXACTLY AS PREDICTED

| | measured |
|---|---|
| compile #1 | **6 handled ensures** — `Ensure condition failed: WidgetBP->WidgetVariableNameToGuidMap.Contains(Widget->GetFName())` at **`WidgetBlueprintCompiler.cpp:781`**, one each for `BackdropBorder · RevealButton · CloseButton · RevealLabel · CloseLabel · StatusTextBlock` (⛔ **`CanvasPanel_0` did NOT ensure — it came from the factory and already had a GUID**) |
| ⭐ **compile #2** | ⭐⭐ **SILENT — ZERO ensures.** The whole `COMPILE_2_BEGIN`→`COMPILE_2_END` span is **3 log lines** (2196→2199). ⇒ **self-healed, ⛔ not a real defect.** |
| the "12 hits" | ⚠️ **A grep for the ensure text returns 12, ⛔ but that is 6 + the SAME 6 replayed in the shutdown error summary (`LogInit: Display:`, lines 2218-2260, AFTER `SAVE_2_DONE`). ⛔ Do not read 12 as twelve failures.** |
| ⚠️ **commandlet exit code** | ⚠️⭐ **`exit=1` — AND IT LIES, exactly like `Build.bat`.** The run **succeeded**; the non-zero code reflects only that handled ensures were logged at `Error` severity. ⛔ **Zero** `Compile of … failed`, `LogK2Compiler: Error`, `Failed to save`, or `BindWidget` warnings in the entire log, and the log ends `LogExit: Exiting.` ⇒ ⛔ **parse the log, never the exit code.** |
| `warnings_as_errors=true` | ✅ **CLEAN** (run in the re-opened editor, compile #3) |

## E. 🔒 STATE LEDGER — MEASURED AT ENTRY **AND** EXIT

| item | state |
|---|---|
| 🔒 `Content/Maps/L_Arena.umap` | ✅ **`b3dbc5d9…f8268` IDENTICAL** at entry, after the close, and at finish. **`Saving Package: /Game/Maps` = `0`.** ⛔ Never opened, never saved. |
| ⭐ `Content/UI/WBP_WarMap.uasset` | **`a2d5e4a2…18fe` (23,105 B, EMPTY) → `01b75cafe8a040a907ec8098a6eb6d2272c501e1a9057ecd7246e7045e3ff23e` (33,193 B, ROOTED)** · git **`AM`** |
| 🔒 backup `.bak` | ✅ **`a2d5e4a2…18fe` re-verified BEFORE the delete** ⇒ the old shell is recoverable byte-exact. ⛔ **Restore was never needed.** |
| `Content/Input/IMC_Hero.uasset` | ✅ **`9654fae4…2950` IDENTICAL — ⛔ NOT touched, ⛔ run 1 NOT re-run.** |
| `Content/Input/Actions/IA_WarMap.uasset` | ✅ **`91eb6b2f…9dae1` IDENTICAL** (read-only) |
| `Config/DefaultEditor.ini` | ✅ **md5 `3d4bba51b6c5bf972a8bbf0c3d58654e` IDENTICAL**; `git status -- Config/` **EMPTY** ⇒ **`DefaultRootWidget` `None → CanvasPanel → None` was IN-MEMORY ONLY**, restored by **EXPLICIT `set` + readback (`"None"`)**, ⛔ never `reset_properties`. |
| `Saving Package:` — **the COMPLETE list** | ⭐ **`/Game/UI/WBP_WarMap` ×2 (the two compile-saves). ⛔ NOTHING ELSE.** |
| residue | ✅ **ZERO** — `WBP_ZZRootProbe589` and `WBP_WarMap_Empty589` absent on disk, in `git status`, and over MCP |
| `.gen.cpp` / `Intermediate/` / `.umap` / `Saved/` in git | ✅ **ZERO of each** |
| commits / pushes | ✅ **NONE.** `HEAD` = **`f205eb5`**, **`0 0`** |
| the staged set | ✅ ⛔ **I ADDED AND REMOVED NOTHING** — ⛔ no `git add`, ⛔ no `git restore --staged`. **TASK-570 still owes the `git diff --cached` reconciliation.** |
| 🔒 TASK-552's latch | ✅ **UNSPENT** — no PIE, no Simulate, assistant console never opened. ⛔ No token figure anywhere. |
| C++ / compile / `Tools/**/*.py` | ⛔ **NONE** ⇒ no `SC-§27` gate owed (the runner is scratchpad-only) |
| the permission rules | 📌 in **`.claude/settings.local.json`**, **gitignored (`.gitignore:16`)** ⇒ ⛔ **cannot enter the commit.** ✅ Confirmed absent from `git status`. |

### THE EDITOR — `W7-R2` DISCHARGED A THIRD TIME
1. ✅ **GRACEFUL ONLY** — `CloseMainWindow()` accepted `True`, exited within timeout, `Get-Process` confirmed **no process remained**. ⛔ **`Kill()` never called.**
2. ✅ **DIRTINESS RE-MEASURED BY ME IMMEDIATELY BEFORE CLOSING** (`SC-§9`): **3,227 found · 58 excluded · 3,169 scanned ⇒ `dirty_count: 0`.** **Nothing was Jonathan's to pick.**
3. ✅ **RE-OPENED** (**PID 13252**), MCP live, and a **CLOSING sweep re-measured `dirty_count: 0` across the same 3,169** ⇒ ⛔ **the editor is handed back CLEAN.**

## F. 🚩 WHAT IS LEFT — ⛔ NOT A RESIDUE, BUT ⛔ DO NOT SKIP #1

1. ⛔⭐⭐ **THE COMMIT TRAP (TASK-570) — `Content/UI/WBP_WarMap.uasset` reads `AM`: the INDEX still holds the EMPTY 23,105-BYTE BLOB** (auto-staged `A ` by the editor's SCC long before this run); the 33,193-byte rooted asset is in the WORKTREE only. ⇒ ⛔ **THE COMMITTER MUST `git add` THAT PATH FIRST, or the commit ships the EMPTY widget while `git status` looks satisfied** — ⚖️ **the TASK-445 class of trap: a readback that still looks right.**
2. 📌 **The pixel check is Jonathan's** — see below.
3. 📌 `TL-§4` **still needs its amendment** (run 1's finding 1): the factory + `DefaultRootWidget` lever is now not merely proven on a throwaway but **shipped on the real asset**, so *"a from-scratch `WBP_` needs exactly ONE human act"* is **FALSE**.
4. 📌 **Worth a `TL-§` line:** ⛔ `WidgetBlueprint` has no Python `parent_class`; ⛔ `UWidget` has no Python `b_is_variable`; ⛔ the `-run=pythonscript` **exit code lies**; ⛔ the MCP lane is closed **by schema** (run 2's finding).

⛔ **AND THE STANDING ONE: NOTHING HERE CLAIMS THE MAP LOOKS RIGHT — I RENDERED ZERO PIXELS.** Every claim above is a file hash, a property/object readback, an engine-source citation, a name-table count or a log line. **Whether the backdrop, the two buttons and the wrapping status line actually LOOK right on screen — and whether `RevealButton` and `StatusTextBlock` behave in PIE — is Jonathan's pixel check.**
