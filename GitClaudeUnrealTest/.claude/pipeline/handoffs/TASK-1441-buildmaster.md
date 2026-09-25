# TASK-1441 — [BASIC-MOVEMENT-COOK-AUDIT] — build-master

marker: `TASK-1441-BASIC-MOVEMENT-COOK-AUDIT`
law: `SC-§39` · `SC-§68` · `SC-§101` · `SC-§125` · `SC-§138` · `VER-§8` cl. 3(a) · `PKG-§`
date: 2026-09-24 · read-only · no editor touched · no git write · no cook · no package

---

## THE ANSWER — NO. IT DID NOT SHIP.

**`BP_Basic_Movement` is ABSENT from `packagedZIPofGame\Siegebound-Win64-Shipping-2026-09-09.zip`.**
**So is every one of the 76 files under `Content/sA_ArcheryVfxPack/`. Zero of 76 cooked.**

**Branch (4)(a) — THE WORRY CLOSES.** This is a dev-only defect. It is not a shipped defect.
`TASK-1440`'s remedy **does not** inherit a deadline, and the manager's *"background, not a milestone
blocker"* ruling **stands unoverturned**. No `🚨 Blockers` escalation was owed and none was posted.

---

## WHICH SOURCE ANSWERED (`SC-§39` — an absence from an unread source is not an absence)

Four independent instruments answered. **Each one carries a positive control that fired**, so each
was demonstrably capable of showing the asset had it been there.

| # | Instrument | Scope it names | ArcheryVfx / Basic_Movement | Positive control (fired) |
|---|---|---|---|---|
| 1 | `Manifest_UFSFiles_Win64.txt` **inside the zip** | 3540 lines; 1390 `.uasset` + 3 `.umap` | **0 / 0** | 48 `BP_` entries; 20 Content folders named |
| 2 | **Shipped IoStore index** `GitClaudeUnrealTest-Windows.utoc`, read by `UnrealPak -List` | 3448 files; 1390 `.uasset` + 3 `.umap` + 397 `.ubulk`, each by full path | **0 / 0** | `BP_Building_Wall`, `BP_SiegeFog`, `WBP_WarMap`, `L_Arena`, `L_MainMenu` → 1 hit each |
| 3 | **Shipped pak index** `GitClaudeUnrealTest-Windows.pak`, `UnrealPak -List` | 1745 files | **0 / 0** | lists `AssetRegistry.bin`, the 4 staged `Config/*.ini`, the `.uproject` |
| 4 | **Shipped `AssetRegistry.bin`**, extracted from that pak (469,056 B) | the build's own asset ledger | **0 / 0** | `BP_Building_Wall` ×2, `Maps` ×7 |
| 5 | **Cook log of that exact run** `.ship/20260910-063540/cook.out.log` (88,190 B, mtime `2026-09-09 23:37:55`) | the full cook, `Cooked packages 1400 Packages Remain 0 Total 1400` | **0 / 0** | the 1400/1400 tally and the verbatim command line |

Instrument 2 is the load-bearing one: it is **the shipped container's own directory index, read by the
engine's own reader**, and it names 1390 `.uasset` packages by full path — the same 1390 the staged
manifest names. It is not a proxy for what shipped; it *is* what shipped.

I also ran **all nine** `sA_ArcheryVfxPack` Blueprints individually against instrument 2, not just the
one the row names — `BP_ArrowShower`, `BP_Basic_Movement`, `BP_Projectile_7A`,
`BP_Projectile_Electricty`, `BP_Projectile_Spawner`, `BP_Projectile_Toxic`, `GM_TestGamemode`,
`LVL_Showroom`, `NS_ArrowShower`. **All zero.**

### Sources I could NOT reach — named, per `SC-§39`

- **The cooker's own abslog** `…\AutomationTool\Saved\Cook-2026.09.09-23.37.19.txt` (its path is
  quoted in the cook command line) — **deleted**. That directory holds no `Cook-*.txt` at all. It would
  have been the most granular per-package record; it is gone and I do not cite it.
- **`Saved/Cooked/Windows/`** survives with the right mtime (`2026-09-09 23:37`) but holds
  **0 `.uasset`** — only `Metadata/` and four `ShaderArchive-*.ushaderbytecode`. This is a ZenStore
  cook (`bUseZenStore=True`), so packages never land on disk here. **It is an empty instrument and its
  silence proves nothing. I do not count it as evidence either way.**
- **A raw byte-grep of the `.utoc`** returned 0 but also found only 1 readable occurrence of
  `Blueprints` and `strings` recovered no paths — the index is compressed. **That grep is not
  evidence** and is excluded; `UnrealPak -List` (instrument 2) is what actually read the container.

---

## RE-MEASURED AT MY OWN INSTANT (`SC-§138`) — memory was RIGHT on all three

The remembered figures were treated as unmeasured and re-measured. They agree; no disagreement to report.

| Figure | Remembered | **Measured 2026-09-24** | |
|---|---|---|---|
| entries | 71 | **71** | ✅ |
| size | 1.33 GB | **1,334,632,180 bytes** (1.3346 GB dec / 1.243 GiB) | ✅ |
| sha256 | `1CAF8DCD…3EA7F8` | **`1caf8dcd93fbff07294a0f7650d360d8ef2b0163455858ecca971e15833ea7f8`** | ✅ |

zip mtime `2026-09-10 00:37:01 -0700`. The three pak/ucas/utoc entries inside it are dated
`2026-09-09 23:37`, matching the `20260910-063540` cook run — so the log I read is the log of the
cook that made this zip, not a neighbouring run.

---

## THE SECOND QUESTION — DIRECTORY-DRIVEN OR REFERENCE-DRIVEN?

### Answer: **REFERENCE-DRIVEN, with an explicit eleven-folder directory allowlist bolted on.**
### It is **NOT** directory-driven over `Content/`. But it is **not purely reference-driven either.**

**Settings cited, not impressions:**

1. **`Config/DefaultGame.ini` has no `[/Script/UnrealEd.ProjectPackagingSettings]` section at all.**
   A grep across the whole of `Config/` for `ProjectPackagingSettings|DirectoriesToAlwaysCook|DirectoriesToNeverCook|bCookAll|MapsToCook|bSkipEditorContent` exits 1 — **zero matches in any of the 7 ini files.** The project contributes no packaging settings whatsoever.
2. **Engine `BaseGame.ini` `[/Script/UnrealEd.ProjectPackagingSettings]`** defines no `bCookAll` and no `DirectoriesToAlwaysCook` keys ⇒ both default off. (It does set `UsePakFile=True`, `bUseIoStore=True`, `bUseZenStore=True`, `bSkipEditorContent=false`.)
3. **The cooker's own self-report**, `cook.out.log` line 424: `LogCookStats: Display:     IsCookAll=false`.
4. **The actual driver is the UAT command line**, emitted by `Tools/Packaging/ship.ps1` and quoted verbatim at `cook.out.log` line 4:

```
BuildCookRun -project=…\GitClaudeUnrealTest.uproject -nop4 -utf8output -platform=Win64
  -clientconfig=Shipping -build -cook
  -map=/Game/Maps/L_MainMenu+/Game/Maps/L_Arena
  -pak -stage -prereqs -archive -archivedirectory=…\packagedZIPofGame
  -AdditionalCookerOptions="-COOKDIR=…\Content\Data -COOKDIR=…\Content\UI
   -COOKDIR=…\Content\Blueprints -COOKDIR=…\Content\Input -COOKDIR=…\Content\Characters
   -COOKDIR=…\Content\Meshes -COOKDIR=…\Content\Materials -COOKDIR=…\Content\Textures
   -COOKDIR=…\Content\VFX -COOKDIR=…\Content\Audio -COOKDIR=…\Content\LevelPrototyping"
```

So the cook seed is **two maps** plus **eleven named directories**. `Content/sA_ArcheryVfxPack` is in
neither, and nothing in `Source/` or `Config/` references it — hence 0 of 76.

### The two mechanisms are visibly distinguishable in the shipped container

On-disk `.uasset`+`.umap` count vs the count that reached the shipped `.utoc`:

```
COOKDIR folders (taken essentially wholesale)      ON-DISK   SHIPPED
  Blueprints                                            30        30
  UI                                                    47        47
  Data                                                   2         2
  Characters                                           233       233
  Textures                                              86        86
  VFX                                                   14        14
  Audio                                                  6         6
  LevelPrototyping                                      29        29
  Meshes                                                45        47   (+2)
  Materials                                             60        71   (+11)
  Input                                                 35        31   (-4)

NOT a COOKDIR (reference-driven only — heavily pruned)
  Tree_Pack_1                                           39        20
  Realistic_Grass_and_plant                            197        64
  Realistic_Rocks                                      213       104
  Variant_Combat                                        31         3
  Fire_Magic                                           255        29
  Ice_Magic                                            226        26
  MedievalWeaponsSFX                                  1368         6   (0.4%)
  sA_ArcheryVfxPack                                     76         0   (0%)
```

Eight of the eleven COOKDIRs ship **exactly** their on-disk count. The non-COOKDIR folders are pruned
to between 0% and 51%. That is the two mechanisms showing their fingerprints side by side, measured
rather than asserted.

### ⚠️ THE GENERALISING FINDING — bounded, but NOT empty

The row asked whether *every* unreferenced broken asset in `Content/` is a shipping risk. **It is not —
but eleven folders' worth are.**

**Inside those eleven COOKDIRs the cook IS directory-driven: an asset there cooks whether or not
anything references it.** `Content/Blueprints/` is one of the eleven — that is the folder our own
gameplay Blueprints live in. Combined with `SC-§125` (a cooked build does not recompile a Blueprint on
load), an unreferenced Blueprint with unresolved compiler errors sitting in any of
`Data · UI · Blueprints · Input · Characters · Meshes · Materials · Textures · VFX · Audio · LevelPrototyping`
**would ship as a real defect while every in-editor gate read green.**

`sA_ArcheryVfxPack` escaped only because it happens to sit outside the allowlist. That is luck of
folder placement, not a guard. **Recommended follow-up row for the manager:** a broken-Blueprint census
scoped to those eleven directories. I did not run it — it needs the editor, and this row is fenced out
of the editor.

### What would RE-OPEN this worry

Any one of these, and the answer flips without warning:

1. `Content/sA_ArcheryVfxPack` (or any path under it) being **added as a `-COOKDIR`** in `Tools/Packaging/ship.ps1`.
2. **Anything reachable from `L_MainMenu` or `L_Arena`** — or from any asset in the eleven COOKDIRs —
   acquiring a reference to `BP_Basic_Movement` or its pack. One actor placed in `L_Arena` does it.
3. A **third map** being added to `-map=`, if that map touches the pack.
4. A `[/Script/UnrealEd.ProjectPackagingSettings]` section appearing in `Config/DefaultGame.ini` with
   `bCookAll=True` or a `+DirectoriesToAlwaysCook=` covering `Content/`. **Today there is no such
   section at all**, so this is a one-line edit away.
5. The broken asset being **moved into one of the eleven COOKDIRs** — e.g. tidied into `Content/Blueprints/`.

Item 5 deserves flagging to the manager: a well-meant *"move the junk somewhere sensible"* cleanup that
relocates `BP_Basic_Movement` into `Content/Blueprints/` would **convert this closed dev-only worry
into a shipped defect.** If `TASK-1440`'s remedy is *delete*, that is safe. If it is *move*, it is not.

---

## Not examined / limitations

- **The cooker's per-package abslog is gone** (`Cook-2026.09.09-23.37.19.txt`). My cook-log evidence is
  the UAT-side `cook.out.log`, which carries the command line and the 1400/1400 tally but not a
  per-package roll. The container indices (instruments 2–4) cover that gap directly and better.
- **`Saved/Cooked/Windows/` is an empty instrument** (ZenStore cook, 0 `.uasset`). Explicitly excluded.
- **I did not extract or verify the 1.09 GB `.ucas` payload.** I read the `.utoc` index that addresses
  it. A file named in the index but absent from the payload would not be caught — irrelevant here,
  since my finding is an *absence from the index*, which is strictly stronger.
- **`Input` shipped 31 against 35 on disk**, and `Meshes`/`Materials` shipped *more* than on disk.
  So "COOKDIR takes the folder wholesale" is **approximately**, not exactly, true; I have not
  characterised the deltas (editor-only exclusions and cook-generated assets are the likely causes).
  This does not touch the headline — `sA_ArcheryVfxPack` is not a COOKDIR and shipped 0 of 76.
- **I did not census which other assets have unresolved compiler errors.** Only `BP_Basic_Movement` is
  known broken (🧑 Jonathan's photograph of the editor modal). Whether any of the eleven COOKDIRs
  currently harbours a broken asset is **unmeasured** — that is the recommended follow-up row.
- **Only the Shipping zip was audited.** `Siegebound-Win64-Development-2026-08-29.zip`
  (1,409,955,049 B, 2026-08-29) sits beside it and was **not** examined; the row scoped the Shipping one.
- **This audit is of the 2026-09-09 artifact.** It says nothing about a future cook — that is exactly
  what the "what would re-open it" list above is for.
- Fences honoured: no build, no cook, no package, no editor, no PIE, no MCP, no git write. `TASK-1399`'s
  editor on PID 1792 was not touched. The zip was **not** unpacked into the repo — five small entries
  went to the session scratchpad at
  `C:\Users\wesel\AppData\Local\Temp\claude\C--GitProjects-GitHub-GitClaudeUnrealTesting-GitClaudeUnrealTest\d36961fb-c9ad-4c45-b8a7-d2b80c7a159c\scratchpad\t1441`
  and nothing from the package entered the index.

---

## Routing

No QA row (zero code). No `🚨 Blockers` post (the answer is NO, so none was owed).
Returns to the **manager → `TASK-1440`**, which may proceed on its existing background priority —
with the item-5 caveat above attached: **prefer delete over move.**
