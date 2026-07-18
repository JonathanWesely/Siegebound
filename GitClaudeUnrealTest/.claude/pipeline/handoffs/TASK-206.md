# TASK-206 handoff — art-director (2026-07-18)

## What shipped
Two purchase-request entries authored at status `requested` in `.claude/pipeline/fab/FAB-REQUESTS.md` (FAB-001..004 untouched). File-only task — no editor, no assets, no Git.

### FAB-005 — Stylized RTS Buildings & Props Pack
- fab-link: https://www.fab.com/listings/a4b43ae5-e442-4d51-93f2-fea8d77e9f37
- Roster map: Town Center/Fortress → SM_Castle tier (Fortress also a crumble-stage candidate), Barracks → SM_Barracks, Watch Tower → SM_ArrowTower tier (BombTower/BallistaTower donor-adapt), wall segments → SM_Wall (closes the Wall gap), Blacksmith + props → dressing, construction meshes → build-up states.
- Drop: `Content/Fab/StylizedRTSBuildings/` (READ-ONLY quarantine). Integration: TASK-208.

### FAB-006 — Rigged stylized medieval unit pack (ONE pack; Jonathan picks in the launcher)
- Candidates recorded with tradeoffs: **TAB Medieval Knights (recommended — Epic-skeleton rig, cleanest UE 5.8 retarget)**, Toon RTS Units (best roster breadth, custom rig), Animated Stylized Knight (74 anims, single character), Stylized Warrior Pack (modular parts, rig unverified).
- Drop: `Content/Fab/<UnitPack>/` (name per chosen pack). Integration: TASK-209 (retarget via IK_/RTG_ law).

## License + budget (mandatory notes present in both entries)
Both entries flag: must permit game use (expected Fab Standard License); Jonathan verifies exact license tier + price at checkout — agents cannot browse Fab (blocks bots), prices unverifiable agent-side. Budget context ~$100–200 total across both packs.

## Next
- **TASK-207 (Jonathan, external gate):** review/approve (or substitute — I update the entry), purchase via Epic Launcher, drop into `Content/Fab/<Pack>/`, flip entries to `fulfilled` (or say so and art-director flips them).
- Then TASK-208 (buildings inventory/conform/first-wave swaps) and TASK-209 (unit conform + retarget).
- Not touched: pipeline_manifest.json (TASK-194 owns it), no purchases, no `approved` flips.
