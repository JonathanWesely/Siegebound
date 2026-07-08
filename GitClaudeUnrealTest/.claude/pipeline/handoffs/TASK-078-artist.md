# TASK-078 handoff — Card artwork: 22 PNGs imported as UTexture2D (art-director)

**Status: complete — 22/22 imported to `/Game/UI/CardArt/`, verified, saved, not-dirty.**
Editor session: PID 34120, IsPIERunning=false throughout, no editor bounce. No other Content/
mutation (WBP_CardHand untouched — TASK-080). No TASKBOARD edits, no Git commands run.

## What was done

Each `Content/RawAssets/CardArt/<CardID>.png` (TASK-077 renders) was imported via Unreal MCP
(TextureTools.import_file, batched through the ProgrammaticToolset) as
`/Game/UI/CardArt/T_CardArt_<CardID>`, then set/verified per CONVENTIONS "Card artwork (hand UI)":
Texture Group (LODGroup) = TEXTUREGROUP_UI, sRGB = true, compression left default (TC_Default).
All 22 saved via save_assets; is_dirty readback = false for all 22 afterward.

## Per-asset verification table (all values from post-import readback, not assumptions)

| CardID | Asset path (/Game/) | Size | LODGroup | sRGB | Compression | Saved (not dirty) |
|---|---|---|---|---|---|---|
| Footman | /Game/UI/CardArt/T_CardArt_Footman | 512×512 | TEXTUREGROUP_UI | true | TC_Default | yes |
| Archer | /Game/UI/CardArt/T_CardArt_Archer | 512×512 | TEXTUREGROUP_UI | true | TC_Default | yes |
| Knight | /Game/UI/CardArt/T_CardArt_Knight | 512×512 | TEXTUREGROUP_UI | true | TC_Default | yes |
| Miner | /Game/UI/CardArt/T_CardArt_Miner | 512×512 | TEXTUREGROUP_UI | true | TC_Default | yes |
| ArrowTower | /Game/UI/CardArt/T_CardArt_ArrowTower | 512×512 | TEXTUREGROUP_UI | true | TC_Default | yes |
| Wall | /Game/UI/CardArt/T_CardArt_Wall | 512×512 | TEXTUREGROUP_UI | true | TC_Default | yes |
| MilitiaMob | /Game/UI/CardArt/T_CardArt_MilitiaMob | 512×512 | TEXTUREGROUP_UI | true | TC_Default | yes |
| Pikeman | /Game/UI/CardArt/T_CardArt_Pikeman | 512×512 | TEXTUREGROUP_UI | true | TC_Default | yes |
| Sapper | /Game/UI/CardArt/T_CardArt_Sapper | 512×512 | TEXTUREGROUP_UI | true | TC_Default | yes |
| Cavalry | /Game/UI/CardArt/T_CardArt_Cavalry | 512×512 | TEXTUREGROUP_UI | true | TC_Default | yes |
| Longbowman | /Game/UI/CardArt/T_CardArt_Longbowman | 512×512 | TEXTUREGROUP_UI | true | TC_Default | yes |
| Cleric | /Game/UI/CardArt/T_CardArt_Cleric | 512×512 | TEXTUREGROUP_UI | true | TC_Default | yes |
| Ogre | /Game/UI/CardArt/T_CardArt_Ogre | 512×512 | TEXTUREGROUP_UI | true | TC_Default | yes |
| BombTower | /Game/UI/CardArt/T_CardArt_BombTower | 512×512 | TEXTUREGROUP_UI | true | TC_Default | yes |
| BallistaTower | /Game/UI/CardArt/T_CardArt_BallistaTower | 512×512 | TEXTUREGROUP_UI | true | TC_Default | yes |
| Barracks | /Game/UI/CardArt/T_CardArt_Barracks | 512×512 | TEXTUREGROUP_UI | true | TC_Default | yes |
| DeepMine | /Game/UI/CardArt/T_CardArt_DeepMine | 512×512 | TEXTUREGROUP_UI | true | TC_Default | yes |
| Masons | /Game/UI/CardArt/T_CardArt_Masons | 512×512 | TEXTUREGROUP_UI | true | TC_Default | yes |
| SharpenedBlade | /Game/UI/CardArt/T_CardArt_SharpenedBlade | 512×512 | TEXTUREGROUP_UI | true | TC_Default | yes |
| PlateArmor | /Game/UI/CardArt/T_CardArt_PlateArmor | 512×512 | TEXTUREGROUP_UI | true | TC_Default | yes |
| SwiftBoots | /Game/UI/CardArt/T_CardArt_SwiftBoots | 512×512 | TEXTUREGROUP_UI | true | TC_Default | yes |
| WarBanner | /Game/UI/CardArt/T_CardArt_WarBanner | 512×512 | TEXTUREGROUP_UI | true | TC_Default | yes |

Size verified twice: live Texture2D readback (get_size) AND the asset-registry `Dimensions` tag
("512x512") for all 22. Note: immediately after import, get_size transiently reported 32×32 on
some textures — that is the UE5 async-texture-compilation placeholder; all settled to 512×512
within seconds and before save.

## Exact-casing verification

- find_assets listing of /Game/UI/CardArt returned exactly 22 assets; every path compared
  string-exact (character-for-character, not case-insensitive) against
  `/Game/UI/CardArt/T_CardArt_<CardID>` for the 22 cards.csv row names — all match.
- DT_Cards cross-check (DataTableTools.get_rows on /Game/Data/DT_Cards, ALL 22 rows): each row's
  `CardArt` cell equals `/Game/UI/CardArt/T_CardArt_<CardID>.T_CardArt_<CardID>` exactly and
  matches the imported object path — 22/22 match=true. The soft paths resolve; no silent-miss risk.
- Visual spot-check: editor thumbnail renders of T_CardArt_Footman and T_CardArt_SwiftBoots
  eyeballed — art intact, colors correct (sRGB behaving).

## Import warnings

- ZERO warnings/errors from the 22 texture imports themselves (LogInterchangeEngine clean:
  22 × "start importing" + "import completed"; LogTexture built all 22 at 512x512 TFO_AutoDXT).
- One benign `LogScript: Warning: import_asset: T_CardArt_Footman at /Game/UI/CardArt already
  exists` — my own idempotency guard tripping on a retried batch script (first script run imported
  Footman then aborted on a sandbox dict quirk; the retry skipped it correctly). Not an asset issue.
- PRE-EXISTING, not from this task: 22 × `LogCSVImportFactory: Property 'CardArt' on row '<X>' is
  the incorrect type. Expected String, got Object.` timestamped 00:06:58 — that is the TASK-081
  phase-1 DT_Cards reimport (my imports started 00:18:50). Informational CSV-factory noise for a
  TSoftObjectPtr column; harmless in effect — the DT cross-check above proves all 22 CardArt cells
  imported with the exact correct paths. Flagging for build-master's phase-1/phase-2 record.

## Notes for TASK-080 (UMG)

- UMG needs NO direct texture references: the runtime path is
  `UCardHandWidget::GetCardArtTexture(CardID)` / `GetNextCardArtTexture()` (TASK-079 resolvers)
  → `SetBrushFromTexture`. DT_Cards CardArt soft paths are live and verified resolving.
- Images are full-bleed opaque 512×512 squares — no alpha, no borders; safe edge-to-edge on the
  card face. Per TASK-077 handoff, every card keeps clear headroom top-center and a floor strip
  bottom for the DisplayName/cost overlays.

## Notes for TASK-081 phase 2 (commit)

- New .uassets on disk: `Content/UI/CardArt/T_CardArt_<CardID>.uasset` × 22 (all saved,
  none dirty in-editor).
- Git state observed (read-only status check, nothing touched): the editor's Git revision-control
  provider AUTO-STAGED all 22 new .uassets (`A` in index) — known behavior, left as-is per ruling;
  build-master adjudicates. The 22 source PNGs at `Content/RawAssets/CardArt/` are still
  UNTRACKED (`??`) and must be added to the TASK-077..081 commit per the raw-asset rule.
- No other Content/ files were dirtied by this task.
