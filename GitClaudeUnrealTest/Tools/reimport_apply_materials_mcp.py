# reimport_apply_materials_mcp.py
# ---------------------------------------------------------------------------
# STEP 2 of the roster mesh reimport (companion to Tools/reimport_meshes.py).
#
# This is NOT a standalone script -- it is the BODY to hand to the Unreal MCP
# ProgrammaticToolset tool `execute_tool_script` against the RUNNING editor
# (mcp__unreal-mcp__call_tool -> toolset editor_toolset.toolsets.programmatic.
# ProgrammaticToolset, tool execute_tool_script, arguments {"script": <this>}).
#
# WHY A SECOND STEP: the headless commandlet (reimport_meshes.py) does the
# MCP-impossible part -- the in-place FBX reimport that preserves the SM_<CardID>
# UObject and all its refs -- plus the 2 named slots, Nanite-off, <=4 hulls and
# the save. But the commandlet's post-reimport BUILD resets empty material slots
# to WorldGridMaterial, so the MI-per-slot assignment does NOT persist from the
# commandlet. This step, run over MCP in the RELAUNCHED editor, reliably assigns:
#     slot "TeamRegion"   -> MI_TeamColor_Blue
#     slot "<CardID>PBR"  -> MI_<CardID>_PBR
# re-asserts Nanite-off, guarantees <=4 convex hulls, saves, and reads back the
# result for verification.
#
# FULL FLOW for the remaining 12 units (or any batch):
#   1. Save all + cleanly close the interactive editor (releases the project lock).
#   2. Edit Tools/reimport_meshes.py DEFAULT_CARD_IDS (or drop Tools/reimport_cards.txt)
#      to the ready CardIDs, then run the headless commandlet (see that file's header).
#   3. Relaunch the editor; wait for MCP (port 8000).
#   4. Set IDS below to the same CardIDs and run this body via execute_tool_script.
#   Result: fully textured SM_<CardID> with correct slots -- ZERO manual
#   Content-Browser Reimport clicks.
#
# Edit IDS to the batch you are finalizing:
# ---------------------------------------------------------------------------
import json

IDS = ['Knight', 'Cavalry', 'Pikeman', 'MilitiaMob']
TEAM_MI = '/Game/Materials/Instances/MI_TeamColor_Blue.MI_TeamColor_Blue'


def sm_ref(cid):
    return {'refPath': '/Game/Meshes/SM_%s.SM_%s' % (cid, cid)}


def _slots(cid):
    return execute_tool('editor_toolset.toolsets.static_mesh.StaticMeshTools.get_material_slots',
                        json.dumps({'mesh': sm_ref(cid)}))['returnValue']


def _set_material(cid, slot, mat_obj):
    return execute_tool('editor_toolset.toolsets.static_mesh.StaticMeshTools.set_material',
                        json.dumps({'mesh': sm_ref(cid), 'slot_name': slot,
                                    'material': {'refPath': mat_obj}}))['returnValue']


def _set_nanite(cid, enabled):
    return execute_tool('editor_toolset.toolsets.static_mesh.StaticMeshTools.set_nanite_enabled',
                        json.dumps({'mesh': sm_ref(cid), 'enabled': enabled}))


def _gen_collision(cid):
    return execute_tool('editor_toolset.toolsets.static_mesh.StaticMeshTools.generate_convex_collisions',
                        json.dumps({'mesh': sm_ref(cid), 'hull_count': 4,
                                    'max_hull_verts': 16, 'hull_precision': 100000}))['returnValue']


def _get_material(cid, slot):
    return execute_tool('editor_toolset.toolsets.static_mesh.StaticMeshTools.get_material',
                        json.dumps({'mesh': sm_ref(cid), 'slot_name': slot}))['returnValue']['refPath']


def _tris(cid):
    return execute_tool('editor_toolset.toolsets.static_mesh.StaticMeshTools.get_triangle_count',
                        json.dumps({'mesh': sm_ref(cid), 'lod_index': 0}))['returnValue']


def _refs(cid):
    return execute_tool('editor_toolset.toolsets.asset.AssetTools.get_referencers',
                        json.dumps({'asset_path': '/Game/Meshes/SM_%s' % cid}))['returnValue']


def run():
    out = {}
    save_paths = []
    for cid in IDS:
        sl = _slots(cid)
        rec = {'slots': sl}
        if len(sl) >= 1:
            rec['set_slot0'] = _set_material(cid, sl[0], TEAM_MI)
        if len(sl) >= 2:
            rec['set_slot1'] = _set_material(cid, sl[1], '/Game/Materials/Instances/MI_%s_PBR.MI_%s_PBR' % (cid, cid))
        else:
            rec['WARN'] = 'expected 2 slots, got %d' % len(sl)
        _set_nanite(cid, False)
        rec['collision'] = _gen_collision(cid)
        out[cid] = rec
        save_paths.append('/Game/Meshes/SM_%s' % cid)

    out['_saved'] = execute_tool('editor_toolset.toolsets.asset.AssetTools.save_assets',
                                 json.dumps({'asset_paths': save_paths}))['returnValue']

    # verification readback
    verify = {}
    for cid in IDS:
        sl = _slots(cid)
        verify[cid] = {
            'tris': _tris(cid),
            'slots': sl,
            'slot0_mat': _get_material(cid, sl[0]) if len(sl) >= 1 else None,
            'slot1_mat': _get_material(cid, sl[1]) if len(sl) >= 2 else None,
            'referencers': _refs(cid),
        }
    out['_verify'] = verify
    return out
