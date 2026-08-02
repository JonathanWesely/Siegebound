# TASK-376 — build-master handoff

**Date:** 2026-08-02 · **Agent:** build-master
**Scope:** TWO jobs — (1) TASK-376 `DT_Cards` reimport; (2) TASK-372's last open acceptance criterion, the `SK_Sorcerer` LOD chain.

> **NO COMMIT, NO PUSH.** Neither job committed anything. TASK-378 owns the commit and is gated on
> Jonathan's PIE ship gate (TASK-377), which has not happened. Working tree left dirty on purpose.

---

## VERDICT

| Job | Result |
|---|---|
| **JOB 1 — `DT_Cards`** | ✅ **DONE** — `Sorcerer` row live, `Footman` DeckCount 9, `sum(DeckCount) == 50`, saved |
| **JOB 2 — `SK_Sorcerer` LOD chain** | ✅ **DONE** — `lod_count` **1 → 3** by readback; TASK-372's last acceptance criterion **CLOSED** |

Both verified by hard readback from a **fresh editor process** after a full close/reopen cycle, not from the
in-memory state that produced them.

---

## JOB 1 — `DT_Cards` reimport

### The method, and why it is not a literal CSV reimport

The board spec's own note names the mechanism: *"the import-factory enum quirk that `set_rows` sidesteps."*
I applied the change via `DataTableTools.add_rows` + `set_rows` rather than `import_file`, and I did **not** edit
`Docs/Data/cards.csv` (read-only for this task, per spec).

**This was a measured decision, not a shortcut.** Before touching anything I diffed the CSV against a full
30-column dump of all 29 live rows. The CSV's genuine deltas vs the live table were **exactly the two intended
ones**. The only other difference was structural:

- **28 of 30 CSV rows carry an EMPTY `SpellDelivery` cell**, while the table holds the enum's index-0 default `Auto`.
- `Fireball` and `FrostNova` carry an explicit `HeroLine` in the CSV — matching the table exactly.

⚠️ **That empty enum column is precisely the "import succeeds while silently shifting values" trap the spec warned
about.** A wholesale CSV reimport would have pushed 28 empty cells at a `ESpellDelivery` column whose parse
behaviour on empty input is unverified — and `Fireball`/`FrostNova`'s `HeroLine` is real, non-default data that a
bad parse could have silently reverted. `set_rows` touches only the two intended rows, which makes drift on the
other 28 **structurally impossible** rather than merely verified-after-the-fact.

### Column-count trap: checked at the source, clean

`Docs/Data/cards.csv` parsed with a real CSV reader: **31 header fields, 30 data rows, ZERO width mismatches**,
CRLF throughout (31 CRLF / 0 bare LF). Header field count matches every row. The 30 named columns map 1:1 onto the
30 `FCardRow` properties. So the trap did not fire here — but the fix above means it could not have bitten anyway.

### Hard readback — `Sorcerer` row (every column verified, post-restart)

| Column | Value | |
|---|---|---|
| `CardType` | `Unit` | ✅ |
| `Cost` | **60** | ✅ |
| `MaxCopies` | **2** | ✅ |
| `HP` | 70 | ✅ |
| `Damage` / `Range` / `Cadence` | **0 / 0 / 0** | ✅ |
| `Speed` | 350 | ✅ |
| **`Profile`** | **`Standard`** | ✅ **LOAD-BEARING** |
| `DeckCount` | **2** | ✅ |
| `CardArt` | `/Game/UI/CardArt/T_CardArt_Sorcerer.T_CardArt_Sorcerer` | ✅ |

**`Profile == Standard` confirmed explicitly** — `IsGroupCommandEligible()` gates on it, so a `Support` value would
have silently made the unit uncommandable. It reads `Standard`.

**`CardArt` proven to be a REAL object reference, not a stored string:** `get_dependencies` on `DT_Cards` returned
**29** CardArt textures before the save and **30 after**, with `/Game/UI/CardArt/T_CardArt_Sorcerer` newly present.
The table picked the asset up; it did not merely retain the path text.

### Invariants

- **`sum(DeckCount) == 50`** across all **30** rows — recomputed from a fresh post-restart dump. ✅
- `Footman` **DeckCount 11 → 9**, `Cost` **9 unchanged**. ✅
- Row count **29 → 30**; no row lost, no row renamed.

### No-drift proof (machine diff, three ways)

| Check | Result |
|---|---|
| **post vs baseline** (full 30-col dump of all rows, before vs after) | **exactly 2 deltas**: `+ Sorcerer` row, `~ Footman.deckCount 11 → 9` |
| **post vs `cards.csv`** (source of truth) | **0 mismatches** across all 30 rows × 30 columns |
| `sum(DeckCount)` | **50** |

`DT_Cards` saved, `is_dirty == false`. Benign `LogCSVImportFactory "missing CardType"` warnings: not chased, per spec.

---

## JOB 2 — `SK_Sorcerer` LOD chain (headless commandlet)

### The denied setting was NOT used, and NOT routed around

The previous agent's `bRemoteExecution` flip was denied by the permission system. **I did not enable it, and I did
not edit any plugin config on disk to reach the same end.** I used the project's other sanctioned lane: the headless
`-run=pythonscript` commandlet (`Tools/reimport_meshes.py` is the shipped precedent, and TASK-289 explicitly names
the commandlet as the alternative to remote-exec). That is a different mechanism, not a workaround of the denial.

### Pre-close dirty sweep — the gate before closing the editor

Enumerated **3,218 registry entries**, excluded **58** `__ExternalObjects__` OFPA stubs (registry entries with no
loadable asset — probing them errors), and ran `is_dirty` on all **3,160** remaining:

**`dirty_count == 0`. The dirty set was EMPTY.**

Both named risk assets were explicitly confirmed **present in the probe set** (not silently absent) and clean:

- **`MI_Sorcerer_PBR`** — `in_probe_set: true`, `dirty: false`. **Nothing to discard; it was never saved.**
- **`L_Arena`** (class `World`) — `in_probe_set: true`, `dirty: false`. **NEVER opened, NEVER saved.**

Because the dirty set was empty, the graceful close raised **no save prompt at all** — so the
`SK_Footman_Skeleton` "never Don't Save" hazard never arose.

### Close / run / reopen

- Editor PID 29868 closed via **`CloseMainWindow()` (WM_CLOSE) — a graceful close, not a force-kill**; exited
  cleanly. Verified **0** `UnrealEditor` / `UnrealEditor-Cmd` / `UnrealTraceServer` processes and no `.lock` files
  before running the commandlet.
- Commandlet script: `<scratchpad>/t376/regen_sorcerer_lods.py`, log `<scratchpad>/t376/commandlet.log`.
- Editor relaunched (PID 34472), MCP endpoint confirmed back up.

⚠️ **The commandlet script was deliberately written to the scratchpad, NOT to `Tools/`.** `Tools/**/*.py` is CODE
and needs a QA gate before commit; parking an un-reviewed script in the repo would create exactly the kind of file
that gets swept into a commit. If this recipe should become durable tooling, that is a new QA-gated task.

### Exit-code law applied

`UnrealEditor-Cmd.exe` returned **raw exit code 0** — which proves nothing. The script prints one authoritative
verdict line, and a **missing** verdict is treated as failure:

```
[SKLOD] RESULT: Succeeded
```

Corroborated by the engine's own line: `LogInit: Display: Success - 0 error(s), 2 warning(s)`, and a scan for
`LogPython: Error` / `Ensure condition failed` / `Fatal error` / `LogOutputDevice: Error` returned **0 hits**.

### The recipe as executed

Per `Content/RawAssets/Characters/Sorcerer.lod.json` (`siege_sk_lod_recipe_v1`): transient
`SkeletalMeshLODSettings` with 3 `SkeletalMeshLODGroupSettings` → `sk.lod_settings` → `regenerate_lod(sk, 3)` →
**restore `lod_settings` to `None`** → readback → save.

- LOD1 **50% @ screen 0.4** · LOD2 **20% @ 0.15**
- ⚠️ Property spelling **`num_of_triangles_percentage`** (confirmed correct — the previous agent's verification held;
  `number_of_triangles_percentage` does not exist), with `SMOT_NUM_OF_TRIANGLES` + `SMTC_NUM_OF_TRIANGLES`.
- `regenerate_lod(3)` returned `True`; `lod_settings` restored to `None` and **asserted `None`** afterwards.
- **NO companion `SkeletalMeshLODSettings` asset was created** — `find` over `Content/` shows only the two
  pre-existing third-party `Mannequin_LODSettings` assets (Fire_Magic / Ice_Magic demo content), untouched.

### Acceptance — `lod_count == 3` ✅

Measured three times, including from a **fresh editor process** after the reopen:

| Stage | `lod_count` | verts |
|---|---|---|
| BEFORE | **1** | 15572 |
| after regen (in commandlet) | **3** | — |
| post-save reload (in commandlet) | **3** | — |
| **post-restart, fresh editor via MCP** | **3** | **15572 / 9845 / 5482** |

Vertex fractions **0.632 / 0.352** against the 0.5 / 0.2 **triangle** targets — vertex fractions legitimately run
higher than triangle fractions, and this matches the `SK_Ogre` precedent (TASK-341: 0.62 / 0.34) closely.

### Fleet norm — holds, and TASK-289's stale claim is disproved again

**All 13 units measure `lod_count == 3`** (12 shipped + Sorcerer). TASK-289's "SK fleet is LOD0-only" claim remains
**stale** — measured live, before and after:

| Unit | LOD verts |
|---|---|
| Footman | 13573 / 8705 / 5010 |
| Archer | 14016 / 9089 / 5235 |
| Knight | 14018 / 8854 / 5006 |
| Miner | 15032 / 9396 / 5292 |
| MilitiaMob | 14045 / 8735 / 4938 |
| Pikeman | 14141 / 8890 / 5041 |
| Sapper | 14955 / 9461 / 5256 |
| Cavalry | 14637 / 9245 / 5234 |
| Longbowman | 14654 / 9342 / 5313 |
| Cleric | 14150 / 8990 / 5080 |
| Ogre | 17395 / 10719 / 5836 |
| Wizard | 16029 / 10035 / 5553 |
| **Sorcerer** | **15572 / 9845 / 5482** ← was `[15572]` |

### `SK_Sorcerer` integrity — nothing else changed

Post-restart readback: skeleton **`/Game/Characters/SK_Footman_Skeleton`** (no new skeleton), **22 bones**,
material slots **`["TeamRegion", "SorcererPBR"]`**, dependencies exactly `[MI_TeamColor_Blue, MI_Sorcerer_PBR,
SK_Footman_Skeleton]` (+ Interchange script refs) — no dangling refs. `is_dirty == false`.

`find_assets("/Game","Sorcerer")` = **exactly 12**, zero strays.

**URO needs nothing** — `VisibilityBasedAnimTickOption` / `bEnableUpdateRateOptimizations` have no home on
`USkeletalMesh`; they are set in the `ASummonedUnit` constructor. Confirmed, not touched.

---

## FINAL MACHINE STATE (post-restart, verified)

- PIE **not running** · **0 dirty assets** project-wide · `L_Arena` never saved
- `DT_Cards`: 30 rows, `sum(DeckCount) == 50`, `Sorcerer` exact, `Footman` 9/9
- 13/13 units at `lod_count == 3`
- Editor open, MCP up — the state I found

---

## 🔴 FOR TASK-378 (the commit task) — READ BEFORE STAGING

### 1. ⚠️ THE EDITOR'S GIT PROVIDER AUTO-STAGES SAVED ASSETS — USE EXPLICIT PATHSPECS

`Provider=Git` in `Saved/Config/WindowsEditor/SourceControlSettings.ini` means saving an asset in-editor **stages
it**. **13 files are ALREADY STAGED right now** without anyone running `git add`:

```
Content/Blueprints/Units/BP_Unit_Sorcerer.uasset      Content/Meshes/SM_Sorcerer.uasset
Content/Characters/Anims/A_Sorcerer_Attack.uasset     Content/Textures/T_Sorcerer_D.uasset
Content/Characters/Anims/A_Sorcerer_Death.uasset      Content/Textures/T_Sorcerer_N.uasset
Content/Characters/Anims/A_Sorcerer_Idle.uasset       Content/Textures/T_Sorcerer_ORM.uasset
Content/Characters/Anims/A_Sorcerer_Walk.uasset       Content/UI/CardArt/T_CardArt_Sorcerer.uasset
Content/Characters/SK_Sorcerer.uasset                 Content/Materials/Instances/MI_Sorcerer_PBR.uasset
Content/Materials/M_AncientGround.uasset
```

**A bare `git commit` would sweep all 13 in, including `MI_Sorcerer_PBR`.** Stage with explicit pathspecs only.
Note `MI_Sorcerer_PBR` currently reads **clean** in-editor (the cached shader-map dirtiness described earlier is not
present) — it is still a deliberate per-deliverable decision whether it belongs in commit B, not something to
inherit from the index.

⚠️ `SK_Sorcerer.uasset` and several others are `AM` — **staged-add AND further modified in the worktree**. My LOD
change is in the *unstaged* half. Re-`add` the explicit path so the LOD chain actually lands in the commit;
committing the stale index entry would ship the pre-LOD bytes.

⚠️ **`Content/Data/DT_Cards.uasset` is `M` and NOT staged** — it must be added explicitly or the whole of JOB 1
silently misses the commit.

⚠️ **`L_Arena` is not staged and not dirty. Keep it that way — never save it, never stage it.**

### 2. `main == origin/main` at `10f14de` — 0 ahead, 0 behind

The memory note "main 9 ahead UNPUSHED" is **stale**: Jonathan has pushed. Verify before assuming an ahead-count,
and **do not push** (standing law).

### 3. Two Sorcerer FBX files are BOTH legitimate — not a stray

- `Content/RawAssets/Sorcerer.fbx` — 611,068 B, static-mesh lane (`SM_Sorcerer`, TASK-370/371)
- `Content/RawAssets/Characters/Sorcerer.fbx` — 820,348 B, rigged lane (`SK_Sorcerer`, TASK-372)

Different content (distinct md5s), different pipelines. Commit both; do not "de-duplicate" them.

### 4. `rig_character.py` CRLF (QA's open WARN-4)

`git diff --numstat` gives **65 added / 3 removed** — a real content diff, not a whole-file EOL rewrite. Safe to
stage. (`concept_prompts.json` and `rig_manifest.json` emit "LF will be replaced by CRLF" warnings; their numstats
are 5/0 and 16/1 respectively — also real diffs.)

### 5. Untracked files this batch owns

`Content/RawAssets/{CardArt/Sorcerer.png, Concepts/Sorcerer.png, Characters/*.fbx, Characters/Sorcerer.lod.json,
Textures/Sorcerer/*}`, `Source/.../AncientGround.{h,cpp}`, and the `handoffs/` + `qa/` set. None are staged.

---

## FOLLOW-UPS (reported, not acted on — for the manager)

1. **`Tools/` LOD tooling is not durable.** The commandlet lives in the scratchpad by design. If the SK-LOD regen
   should be repeatable (it will be needed on every same-path SK reimport — CONVENTIONS §516 says a reimport drops
   the mesh to LOD0-only), it needs a QA-gated task to land it under `Tools/`.
2. **`EditorSkeletalMeshLibrary.get_vertex_count` returns `None` inside a `-run=pythonscript` commandlet** (it works
   fine over MCP in a live editor). Minor, but it means per-LOD vert verification has to happen post-restart —
   worth knowing for any future commandlet that tries to self-verify a reduction.
3. **`find_assets("/Game","")` returns 58 `__ExternalObjects__` OFPA registry entries that are not loadable assets**
   and raise on `is_dirty`. Any future project-wide asset sweep must filter `__External`, or it aborts.
4. TASK-372's other recorded flags (24 fps clips, `A_Wizard_*` `bForceRootLock=false`, the Sorcerer capsule
   `88/34` vs mesh-matched ≈`91/40`) are **untouched** by this pass and still open.
