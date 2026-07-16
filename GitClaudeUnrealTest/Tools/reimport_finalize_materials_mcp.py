# reimport_finalize_materials_mcp.py
# ---------------------------------------------------------------------------
# STEP 2 of the roster mesh reimport (companion to Tools/reimport_meshes.py) --
# M7 building/GoldNode wave. Superset of Tools/reimport_apply_materials_mcp.py
# that handles the mixed batch: 2-slot units/buildings, the GoldNode single-slot
# emissive VARIANT, and the CrystalTower emissive slot.
#
# This is NOT a standalone script -- it is the BODY handed to the Unreal MCP
# ProgrammaticToolset (call_tool -> toolset editor_toolset.toolsets.programmatic.
# ProgrammaticToolset, tool execute_tool_script, arguments {"script": <this>})
# against the RUNNING editor, AFTER the headless commandlet + relaunch.
#
# WHY A SECOND STEP: the commandlet does the MCP-impossible part (in-place FBX
# reimport preserving the SM_<CardID> UObject + refs) + slot names + Nanite-off +
# collision + the MI/texture ASSETS, but the commandlet's post-reimport BUILD
# resets the MESH's empty material-slot POINTERS to WorldGridMaterial. This step
# reliably assigns the slot->material pointers over MCP:
#   * 2-slot unit/building : slot0 -> MI_TeamColor_Blue, slot1 -> MI_<CardID>_PBR
#   * GoldNode (variant)   : ALL slots -> M_GoldGlow (warm-yellow emissive kept;
#                            NO TeamColor -- AGoldNode is team-agnostic)
#   * CrystalTower         : slot0 -> MI_TeamColor_Blue, slot1 -> MI_CrystalTower_PBR,
#                            and if a 3rd emissive slot exists -> M_CrystalGlow
#                            (glow preserved). If only 2 slots -> flagged.
# Units also get their <=4 convex hulls re-asserted; buildings/GoldNode collision
# is LEFT ALONE (the commandlet authored explicit manifest box hulls -- calling
# generate_convex_collisions here would destroy them).
# ---------------------------------------------------------------------------
import json

TEAM_MI = '/Game/Materials/Instances/MI_TeamColor_Blue.MI_TeamColor_Blue'
GOLD_MAT = '/Game/Materials/M_GoldGlow.M_GoldGlow'
CRYSTAL_MAT = '/Game/Materials/M_CrystalGlow.M_CrystalGlow'

UNITS_NEW = ['Sapper', 'Cleric', 'Longbowman', 'Miner']
BUILDINGS_STD = ['ArrowTower', 'BallistaTower', 'Barracks', 'BombTower']
CRYSTAL = 'CrystalTower'
GOLD = 'GoldNode'
UNITS_DONE = ['Knight', 'Cavalry', 'Pikeman', 'MilitiaMob']  # verify-only

# The ones the commandlet reimported this wave (get material finalize applied):
REIMPORTED = UNITS_NEW + BUILDINGS_STD + [CRYSTAL, GOLD]
# Everything we read back for verification:
ALL14 = UNITS_DONE + REIMPORTED


def et(name, args):
    return execute_tool(name, json.dumps(args))


def sm_ref(cid):
    return {'refPath': '/Game/Meshes/SM_%s.SM_%s' % (cid, cid)}


def _slots(cid):
    return et('editor_toolset.toolsets.static_mesh.StaticMeshTools.get_material_slots',
              {'mesh': sm_ref(cid)})['returnValue']


def _set_material(cid, slot, mat_obj):
    return et('editor_toolset.toolsets.static_mesh.StaticMeshTools.set_material',
              {'mesh': sm_ref(cid), 'slot_name': slot, 'material': {'refPath': mat_obj}})['returnValue']


def _get_material(cid, slot):
    rv = et('editor_toolset.toolsets.static_mesh.StaticMeshTools.get_material',
            {'mesh': sm_ref(cid), 'slot_name': slot})['returnValue']
    return rv.get('refPath') if isinstance(rv, dict) else rv


def _set_nanite(cid, enabled):
    return et('editor_toolset.toolsets.static_mesh.StaticMeshTools.set_nanite_enabled',
              {'mesh': sm_ref(cid), 'enabled': enabled})


def _is_nanite(cid):
    return et('editor_toolset.toolsets.static_mesh.StaticMeshTools.is_nanite_enabled',
              {'mesh': sm_ref(cid)})['returnValue']


def _gen_hulls(cid):
    return et('editor_toolset.toolsets.static_mesh.StaticMeshTools.generate_convex_collisions',
              {'mesh': sm_ref(cid), 'hull_count': 4, 'max_hull_verts': 16,
               'hull_precision': 100000})['returnValue']


def _tris(cid):
    return et('editor_toolset.toolsets.static_mesh.StaticMeshTools.get_triangle_count',
              {'mesh': sm_ref(cid), 'lod_index': 0})['returnValue']


def _refs(cid):
    return et('editor_toolset.toolsets.asset.AssetTools.get_referencers',
              {'asset_path': '/Game/Meshes/SM_%s' % cid})['returnValue']


def _pbr_mi(cid):
    return '/Game/Materials/Instances/MI_%s_PBR.MI_%s_PBR' % (cid, cid)


def _finalize_two_slot(cid, pbr_mat, rec):
    sl = _slots(cid)
    rec['slots'] = sl
    if len(sl) >= 1:
        rec['set_slot0'] = _set_material(cid, sl[0], TEAM_MI)
    if len(sl) >= 2:
        rec['set_slot1'] = _set_material(cid, sl[1], pbr_mat)
    else:
        rec['WARN'] = 'expected >=2 slots, got %d' % len(sl)
    return sl


def run():
    out = {'applied': {}, 'verify': {}, 'flags': []}
    save_paths = []

    # --- Units (new): 2-slot + convex hulls ---
    for cid in UNITS_NEW:
        rec = {}
        _finalize_two_slot(cid, _pbr_mi(cid), rec)
        _set_nanite(cid, False)
        rec['collision'] = _gen_hulls(cid)   # units: <=4 hulls
        out['applied'][cid] = rec
        save_paths.append('/Game/Meshes/SM_%s' % cid)

    # --- Standard buildings: 2-slot, leave commandlet box collision ---
    for cid in BUILDINGS_STD:
        rec = {}
        _finalize_two_slot(cid, _pbr_mi(cid), rec)
        _set_nanite(cid, False)
        rec['collision'] = 'manifest-boxes (left as authored by commandlet)'
        out['applied'][cid] = rec
        save_paths.append('/Game/Meshes/SM_%s' % cid)

    # --- CrystalTower: 2-slot PBR + preserve M_CrystalGlow if a 3rd slot exists ---
    rec = {}
    sl = _finalize_two_slot(CRYSTAL, _pbr_mi(CRYSTAL), rec)
    if len(sl) >= 3:
        rec['set_slot2_emissive'] = _set_material(CRYSTAL, sl[2], CRYSTAL_MAT)
        rec['emissive'] = 'preserved on slot %s -> M_CrystalGlow' % sl[2]
    else:
        rec['emissive'] = 'NOT preserved: refined FBX has %d slots (no dedicated crystal slot)' % len(sl)
        out['flags'].append(
            'CrystalTower: crystal glow NOT preserved -- M_AssetPBR has no emissive param and '
            'the refined FBX authored only %d slots. Textured mesh shipped; emissive needs an '
            'art-director pass (T_CrystalTower_E + emissive-capable master, or a dedicated '
            'M_CrystalGlow slot authored into the FBX). Manifest _emissive_note flagged this.' % len(sl))
    _set_nanite(CRYSTAL, False)
    rec['collision'] = 'manifest-boxes (left as authored by commandlet)'
    out['applied'][CRYSTAL] = rec
    save_paths.append('/Game/Meshes/SM_%s' % CRYSTAL)

    # --- GoldNode: single-slot emissive VARIANT -> M_GoldGlow on every slot ---
    rec = {}
    sl = _slots(GOLD)
    rec['slots'] = sl
    for s in sl:
        _set_material(GOLD, s, GOLD_MAT)
    if len(sl) != 1:
        rec['NOTE'] = ('refined FBX authored %d slots; spec wants 1. M_GoldGlow applied to ALL '
                       'slots (all-gold glow preserved, no TeamColor).' % len(sl))
        out['flags'].append(
            'GoldNode: refined FBX authored %d material slots (spec = single slot). M_GoldGlow '
            'applied to every slot so the warm-yellow glow + team-agnostic look are preserved; '
            'flagged because refine_trellis_glb.py still emits 2 slots when team_region is null '
            '(manifest _variant warning).' % len(sl))
    _set_nanite(GOLD, False)
    rec['collision'] = 'manifest-boxes (left as authored by commandlet)'
    out['applied'][GOLD] = rec
    save_paths.append('/Game/Meshes/SM_%s' % GOLD)

    # --- Save all reimported meshes ---
    out['saved'] = et('editor_toolset.toolsets.asset.AssetTools.save_assets',
                      {'asset_paths': save_paths})['returnValue']

    # --- Verification readback for ALL 14 (done units included) ---
    for cid in ALL14:
        try:
            sl = _slots(cid)
            v = {
                'tris': _tris(cid),
                'slots': sl,
                'slot_mats': [_get_material(cid, s) for s in sl],
                'nanite': _is_nanite(cid),
                'referencers': _refs(cid),
            }
        except Exception as e:
            v = {'ERROR': str(e)}
        out['verify'][cid] = v
    return out
