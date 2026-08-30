# TASK-705 — `IA_ControlsHelp` → **Tab** in `IMC_Hero` (art-director)

**Status:** work COMPLETE, all proofs PASS. → `ready-for-integration`
**Date:** 2026-08-29
**Law applied:** `HELP-§4` · `KBD-§2a` (the editor-time authoring carve) · `KBD-§1`/`§2`
**Route used:** in-editor Python against the LIVE editor over the PythonScriptPlugin
remote-execution lane (`bRemoteExecution=True`). **No editor bounce. No commandlet.
No PIE. No compile. No `Source/` edit. No Git operation.**

---

## ⚠️ H1 — THE TAB-CONFLICT VERDICT: **TAB IS FREE. NO CONFLICT. NOTHING WAS STOMPED.**

Proven **in the asset**, not in `Source/` — `HELP-§4` is explicit that the zero-hits
grep is *necessary and not sufficient* because `IMC_Hero` is binary.

- Read `IMC_Hero.DefaultKeyMappings.mappings` **before** any mutation: **25 rows, zero
  carrying key `Tab`.**
- The authoring script re-checks the gate **internally** and calls `bail()` before
  touching anything if a `Tab` row exists — so a conflict could only ever have been
  flagged, never overwritten.
- ⇒ **Row H1 needs no ruling from Jonathan.** Tab was unclaimed; it is now
  `IA_ControlsHelp`'s.

Full pre-state key list, in array order (the `IA_CmdAmbush`/**F** precedent row is
index 21 and is untouched):

`SpaceBar · W · S · A · D · Mouse2D · LeftShift · LeftMouseButton · One ·
RightMouseButton · Escape · Two · Three · Four · Five · Six · LeftAlt · Q · T · R ·
E · F · C · Enter · M`

---

## Assets

| Asset | /Game/ path | Disk path |
|---|---|---|
| `IA_ControlsHelp` (new) | `/Game/Input/Actions/IA_ControlsHelp` | `Content/Input/Actions/IA_ControlsHelp.uasset` |
| `IMC_Hero` (append only) | `/Game/Input/IMC_Hero` | `Content/Input/IMC_Hero.uasset` |

⛔ **No other asset was created, modified, or saved.** Saves were explicit and
per-asset (`EditorAssetLibrary.save_asset` × 2). There is no `save_all` in the script.

### `IA_ControlsHelp` readback — matches the shipped digital template exactly

Confirmed by **two independent instruments** (in-editor Python, and an MCP
`get_properties` call on a different code path):

```
ValueType            = Boolean      (Digital/bool, per spec)
bConsumeInput        = true
bTriggerWhenPaused   = false
bReserveAllMappings  = false
Triggers             = []           Modifiers = []
```

Byte-for-byte the same property set as the shipped `IA_AssistantConsole` and
`IA_WarMap`, which were read first as the template rather than guessed.

---

## `KBD-§2a` — THE FOUR CONDITIONS, EACH DISCHARGED

**1. The `MapKey` call lives in a SCRATCHPAD script, never in `Source/`.** ✅
Script: `<scratchpad>/task705_author.py`. `Source/` was not opened or edited;
`git status` shows zero `Source/` paths from this task.

**2. ONE call, ONE appended row; the removal APIs stay banned.** ✅
Exactly one `imc.map_key(ia, tab_key)`. The driver **mechanically refuses to send**
any payload containing `unmap_key` / `unmap_all` / `UnmapKey` / `UnmapAll` /
`unmap_all_keys_from_action` / `UnmapAllKeysFromAction` — it scanned this payload and
reported `banned-token scan: CLEAN` on every send. The ban is enforced by the tool,
not by my care.
⛔ **The mappings array was never rewritten** — that is the TASK-445 defect and the
one operation MCP *would* have accepted.

**3. SURVIVORS PROVEN BY NAMING THE MODIFIER OBJECTS, NOT THE KEYS.** ✅
A key-list readback is exactly the instrument TASK-445 defeats, so the proof names
the instanced objects:

| row | modifier objects AFTER the append |
|---|---|
| `IA_Move` / `W` | `InputModifierSwizzleAxis_0` |
| `IA_Move` / `S` | `InputModifierSwizzleAxis_1`, `InputModifierNegate_0` |
| `IA_Move` / `A` | `InputModifierNegate_1` |
| `IA_Move` / `D` | *(none — and none before)* |
| `IA_Look` / `Mouse2D` | `InputModifierNegate_2` |

- `proof_survivors_identical: true` (before-dict == after-dict)
- All five required objects present **by name**: `true, true, true, true, true`
- MCP cross-check confirms the same objects by **refPath**, e.g.
  `/Game/Input/IMC_Hero.IMC_Hero:InputModifierSwizzleAxis_0`
- **Count 25 → 26** (`proof_count_n_to_n_plus_1: true`)
- **First 25 keys identical IN ARRAY ORDER** (`true`) — and the first 25 *actions*
  identical in array order too (an extra check I added)

**4. The appended row carries EMPTY `Triggers` and EMPTY `Modifiers`.** ✅

```
index 25 → action=IA_ControlsHelp  key=Tab
          triggers=[]  modifiers=[]
          settingBehavior=InheritSettingsFromAction
          playerMappableKeySettings=None
```

Identical in shape to the shipped `IA_AssistantConsole` row. Its emptiness is the
specification, which is why the append cannot express the ban's failure mechanism.

`ALL_PROOFS_PASS: true`

---

## Post-save digests — for the integrator's **§25b**

⛔ **Both paths are LFS-tracked** (`git check-attr filter` → `lfs` on both), so per
§25b the instrument is **oid vs worktree sha256**, ⛔ **never a byte size.**

| path (repo-relative) | worktree sha256 | index oid | verdict |
|---|---|---|---|
| `GitClaudeUnrealTest/Content/Input/IMC_Hero.uasset` | `9ba4aeb0a86904d8c35e6f8f39afe809e5f19d1d8ebc25a47bf724693ffc46ab` | `9654fae4304bc295e743c13456e6c7f0fc32291c4dd8d368900fe6d83d8e2950` | ⛔ **STALE — re-`add` required** |
| `GitClaudeUnrealTest/Content/Input/Actions/IA_ControlsHelp.uasset` | `b3757ca05fcc6ca36f0c9cc87c04cec0febd0bf8e38587a22b8ec3af7103df2f` | `b3757ca05fcc6ca36f0c9cc87c04cec0febd0bf8e38587a22b8ec3af7103df2f` | ✅ match |

⚠️⚠️ **§25b FINDING THE INTEGRATOR MUST NOT SKIM — THE EDITOR'S SCC AUTO-STAGED THE
NEW ASSET BY ITSELF.** `IA_ControlsHelp.uasset` arrived in the index as `A ` without
any Git command from me (I ran none). This is verbatim the phenomenon §25b records —
*"auto-staged by the editor's SCC between them."*
- `IMC_Hero`'s index entry **still holds the PRE-append blob** (`9654fae4…` is the
  sha I measured *before* the work). A size check would have read 13575 vs 14099 and
  "noticed" — but §25b bans size checks precisely because the refined size form
  *certifies the wrong blob*. **The digest is what separates the worlds here.**
- ⇒ **Re-`git add` BOTH paths and re-verify each by digest before committing.** Treat
  the `A ` on the new asset as unproven until its oid is re-read post-`add`.

Pre-change baseline, recorded so the diff base is unambiguous:
`IMC_Hero` was `9654fae4…2950`, 13575 B, mtime 2026-08-15 22:06:54 → now 14099 B
(+524 B, consistent with one appended row plus one new object reference).

---

## Node-identity + never-save compliance

- **Port-8000 owner probed BEFORE any editor-shaped call:** PID **17044**,
  `UnrealEditor.exe`, launched with the `.uproject` and **no `-game` flag**. The only
  Unreal process on the machine. (TASK-678 lesson honored.)
- The 5.8 remote-exec node announcement carries **no `process_id`**, so identity was
  bound in two independent places instead of guessed: (a) exactly one node, whose
  `project_root` matches ours; (b) **the payload itself asserts `os.getpid() == 17044`
  and aborts before touching an asset otherwise.** Every run reported `pid 17044
  confirmed`. A `-game` client answering as its own node would have failed (b).
- ⛔ **`L_Arena` never-save law HELD, and it is measured, not asserted:**
  `Content/Maps/L_Arena.umap` sha256 =
  `9CCD54EFEB0459DF9ED15204FD7E5274797E5F6093F5504A5730C3C9D5EA0E58`
  — an exact match to the ledger `9ccd54ef…0e58`, mtime unchanged (2026-08-27
  15:05:16). The level was never opened, dirtied, or saved.
- Blast radius confirmed by `git status`: my two asset paths only. (The two
  `Config/*.ini` modifications in the tree are **TASK-698's** parallel lane, not mine.)

---

## Notes for integration

- **Nothing blocks on this task and nothing here needs a compile.** The controller's
  resolve is soft-ref + null-safe, so this asset simply makes TAB *live* where it was
  previously inert (the `IA_Cmd*` pattern).
- The action is **Digital/bool** — bind it as a triggered/pressed event.
- ⭐ Because `KBD-§4` tables all 26 letters and the layout subsystem retargets **only
  `.Key`** on a transient duplicate **by index**, this appended row inherits Dvorak
  handling with **zero code**. The append landing at the END of the array is exactly
  why the index-by-index retarget is unaffected.
- ⛔ Per `HELP-§5`, `Escape` remains untouchable — this task added no `Escape`
  handling of any kind.

---

## ⚠️ DEVIATIONS / FINDINGS (`SC-§15`)

**D1 — `KBD-§2a` condition 3 carries STALE LITERAL COUNTS. Recommend the manager
amend it.**
The law text says *"Required: count **24 → 25**; the first **24** keys identical."*
The measured live count was **25 → 26**. The law's numbers were written before
`IA_WarMap`→`M` (TASK-568) landed as row 24; that row is present and shipped.
⇒ **This is documentation drift, not a defect in this work.** The TASKBOARD spec
correctly generalizes to `N→N+1`, which is what I satisfied. Flagging it because **a
QA reviewer reading the literal law text would raise a false FAIL** against a correct
append. Suggested fix: replace the literals with `N → N+1` in `KBD-§2a` cond. 3.

**D2 — `UInputMappingContext::GetMappings()` is NOT exposed to Python in 5.8.**
`imc.get_mappings()` raises `AttributeError`. The working readback is the
`DefaultKeyMappings.mappings` struct property. `KBD-§2a` already records that
`DefaultKeyMappings` is the live array and the bare `Mappings` is 5.7-deprecated, so
this is consistent with the law — but the *Python* binding gap is newly measured and
worth recording so the next agent does not conclude the API was removed. `map_key`
**is** exposed (it is a `BlueprintCallable` UFUNCTION), exactly as `KBD-§2a`'s
reflected-symbol check predicted.

**D3 — two factory/constructor names differ from the obvious guess; both were probed,
not assumed.**
- The InputAction factory is **`unreal.InputAction_Factory`** (underscore).
  `unreal.InputActionFactory` **does not exist**.
- `unreal.Key`'s constructor takes **no arguments** — both `unreal.Key("Tab")` and
  `unreal.Key(key_name="Tab")` raise `call() takes at most 0 arguments`. The working
  form is `k = unreal.Key()` then `k.set_editor_property("key_name", "Tab")`.
- One intermediate run failed on `unreal.InputCoreTypes` (which does not exist).
  **That failure was clean and is worth stating plainly:** it aborted *before* the
  `map_key` call, and I verified `IMC_Hero` was still byte-identical to its pre-state
  (`9654fae4…`) at that moment. **No partial append ever existed.** The re-run took
  the script's idempotent "already existed, loaded, not recreated" branch.

**D4 — BOARD NOT UPDATED, DELIBERATELY.** The dispatch fenced me `no board` while
also asking for `Status → ready-for-integration`. With TASK-698/701/704 dispatched in
parallel and board write races a known hazard on this project, I honored the **fence**
and did not edit `TASKBOARD.md`. ⇒ **The orchestrator should flip TASK-705 to
`ready-for-integration`.** Flagging rather than silently choosing.

---

## Reproduction artifacts (scratchpad, not repo)

- `ue_remote_driver.py` — remote-exec driver; enforces the banned-token scan, the
  sole-node + project-root check, and the `__EXPECT_PID__` payload self-assertion.
- `task705_preflight.py` — read-only baseline + factory probe (mutates nothing).
- `task705_keyprobe.py` — read-only FKey-construction probe.
- `task705_author.py` — **the `KBD-§2a` scratchpad authoring script** (the one
  `map_key`).
