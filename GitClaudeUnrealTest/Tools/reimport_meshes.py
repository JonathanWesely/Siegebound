# reimport_meshes.py
# ---------------------------------------------------------------------------
# Headless in-place FBX reimport for the Siegebound roster static meshes.
# Extended for the M7 building/GoldNode wave (TASK-172/173, gameplay-programmer).
#
# WHY THIS EXISTS (MCP debt, TASK-086/087/151/172):
#   The Unreal MCP `StaticMeshTools.import_file` CANNOT overwrite an existing
#   `SM_<CardID>` at the same content path -- it returns "already exists" and is
#   non-mutating. That leaves the M7 TRELLIS textured-mesh swaps stuck on a
#   manual Content-Browser right-click -> Reimport per mesh. This commandlet
#   automates the SAME operation for a whole batch, in place, preserving the
#   existing UObject (so every hard ref -- e.g. BP_Unit_<CardID>'s VisualMesh
#   pointer, or L_Arena's GoldNode placement -- and every string-resolved soft
#   ref stay intact). It is the automated equivalent of the manual Reimport,
#   NOT a delete+recreate.
#
# WHAT IT DOES per CardID (category-driven from pipeline_manifest.json):
#   1. TEXTURES + MI (units + non-GoldNode buildings, skipped if MI already
#      exists): import Content/RawAssets/Textures/<CardID>/T_<CardID>_{D,N,ORM}.png
#      -> /Game/Textures/T_<CardID>_{D,N,ORM} with the correct sRGB/compression
#      (D sRGB on / N normal-map / ORM LINEAR-Masks), then create
#      /Game/Materials/Instances/MI_<CardID>_PBR from /Game/Materials/M_AssetPBR
#      with BaseColor/Normal/ORM wired. This is the mechanical texture+MI step
#      TASK-172 did per-asset; the imagery is PRE-BAKED by the art pipeline (this
#      script authors NO art -- it only imports+wires already-baked PNGs).
#   2. In-place reimport Content/RawAssets/<CardID>.fbx OVER /Game/Meshes/SM_<CardID>
#      via unreal.AssetImportTask(replace_existing=True, automated=True) so the
#      asset object identity (and thus refs) is preserved.
#   3. Nanite OFF.
#   4. COLLISION (per-CardID mode -- the M7 building extension):
#        - unit               : <=4 convex decomposition hulls (TASK-037 recipe).
#        - building / GoldNode : explicit UCX-analog BOX hull(s) authored from the
#          manifest `ucx.boxes` (wall-footprint-exact, ground-center space). The
#          refined FBX also carries embedded UCX_ meshes, but we author the
#          manifest boxes deterministically so a building NEVER ships with no/auto
#          collision (the M1 castle-plinth placement dead-zone lesson). NO convex
#          decomposition on buildings (a single auto-hull on a hollow tower
#          re-introduces the interior dead-zone).
#   5. Save the package.
#   6. Best-effort readback (slots / nanite / hull|box count / refs) to the log.
#
# KNOWN COMMANDLET LIMITATION (mesh material slots):
#   In a headless -run=pythonscript commandlet the post-reimport mesh BUILD
#   resets empty material slots to WorldGridMaterial AFTER the script's
#   static_materials write, so the MI-per-slot assignment on the MESH does NOT
#   persist from the commandlet. The GEOMETRY reimport, the slot NAMES, Nanite-off
#   and the collision DO persist -- those are the MCP-impossible parts this script
#   exists to automate. The MI-per-slot assignment on the mesh is finalized over
#   MCP in the RELAUNCHED editor (StaticMeshTools.set_material by slot name). See
#   Tools/reimport_apply_materials_mcp.py.
#   NOTE: the MI ASSET itself (MI_<CardID>_PBR .uasset, step 1) and the TEXTURE
#   assets persist fine from the commandlet -- only the mesh's slot->material
#   POINTER is the flaky part.
#
# HOW TO RUN (interactive editor MUST be closed first -- the project is locked
# by the running editor; run this only against a closed project):
#
#   "C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" ^
#     "C:/GitProjects/GitHub/GitClaudeUnrealTesting/GitClaudeUnrealTest/GitClaudeUnrealTest.uproject" ^
#     -run=pythonscript ^
#     -script="C:/GitProjects/GitHub/GitClaudeUnrealTesting/GitClaudeUnrealTest/Tools/reimport_meshes.py" ^
#     -unattended -nosplash -nopause -stdout -FullStdOutLogOutput
#
# WHICH CARDS: edit DEFAULT_CARD_IDS below, OR drop a sidecar file
#   Tools/reimport_cards.txt  (one CardID per line, '#' comments allowed).
# The sidecar wins if present. Category (unit/building/GoldNode), collision mode
# and box hulls are read from Tools/ArtPipeline/pipeline_manifest.json, so no code
# edit is needed to add a card -- just list the ready CardID and rerun.
#
# SAFETY: this only ever writes /Game/Meshes/SM_<CardID>, /Game/Textures/T_* and
# /Game/Materials/Instances/MI_* packages (LFS-tracked .uasset). Those are
# committed at git HEAD, so a bad reimport is recoverable with
# `git checkout -- Content/...`. This script never touches Git and never deletes
# assets.
# ---------------------------------------------------------------------------

import os
import json
import unreal

# M7 building/GoldNode + U2 unit wave (TASK-173/172). The 4 U1 units
# (Knight/Cavalry/Pikeman/MilitiaMob) are already done+verified -- excluded here
# (verify-only over MCP); add them to the sidecar for an idempotent re-run.
DEFAULT_CARD_IDS = [
    "Sapper", "Cleric", "Longbowman", "Miner",           # U2 units
    "ArrowTower", "BallistaTower", "Barracks", "BombTower", "CrystalTower",  # buildings
    "GoldNode",                                            # emissive economy prop
]

MESH_FOLDER = "/Game/Meshes"
TEXTURE_FOLDER = "/Game/Textures"
MI_FOLDER = "/Game/Materials/Instances"
MASTER_PBR = "/Game/Materials/M_AssetPBR"
TEAM_MI_PATH = "/Game/Materials/Instances/MI_TeamColor_Blue"
PBR_MI_FMT = "/Game/Materials/Instances/MI_%s_PBR"

HULL_COUNT = 4          # units: <= 4 convex hulls
HULL_MAX_VERTS = 16
HULL_PRECISION = 100000

MANIFEST_PATH = os.path.join(
    os.path.dirname(os.path.abspath(__file__)), "ArtPipeline", "pipeline_manifest.json")

TAG = "[REIMPORT]"


def _log(msg):
    unreal.log("%s %s" % (TAG, msg))


def _warn(msg):
    unreal.log_warning("%s %s" % (TAG, msg))


def _err(msg):
    unreal.log_error("%s %s" % (TAG, msg))


def _resolve_card_ids():
    """Sidecar Tools/reimport_cards.txt (one CardID per line) wins; else default."""
    sidecar = os.path.join(os.path.dirname(os.path.abspath(__file__)), "reimport_cards.txt")
    if os.path.isfile(sidecar):
        ids = []
        with open(sidecar, "r") as fh:
            for line in fh:
                s = line.strip()
                if s and not s.startswith("#"):
                    ids.append(s)
        if ids:
            _log("Using sidecar CardID list (%s): %s" % (sidecar, ids))
            return ids
        _warn("Sidecar %s present but empty; falling back to DEFAULT_CARD_IDS." % sidecar)
    return list(DEFAULT_CARD_IDS)


def _load_manifest():
    try:
        with open(MANIFEST_PATH, "r") as fh:
            return json.load(fh).get("assets", {})
    except Exception as ex:
        _err("Could not load manifest %s: %s" % (MANIFEST_PATH, ex))
        return {}


def _fbx_path_for(card_id):
    content_dir = unreal.Paths.project_content_dir()
    return os.path.normpath(os.path.join(content_dir, "RawAssets", "%s.fbx" % card_id))


def _png_path_for(card_id, suffix):
    content_dir = unreal.Paths.project_content_dir()
    return os.path.normpath(os.path.join(
        content_dir, "RawAssets", "Textures", card_id, "T_%s_%s.png" % (card_id, suffix)))


def _category_of(card_id, manifest):
    """Returns one of 'unit', 'building', 'goldnode'."""
    entry = manifest.get(card_id, {})
    cat = entry.get("category", "unit")
    if cat == "building" and entry.get("team_region", "MISSING") is None:
        return "goldnode"   # emissive single-slot variant (team_region:null)
    return cat


# --- Textures + material instance -----------------------------------------

def _import_texture(card_id, suffix, srgb, compression):
    """Import T_<CardID>_<suffix>.png with explicit sRGB/compression. Returns
    the loaded UTexture2D or None."""
    png = _png_path_for(card_id, suffix)
    name = "T_%s_%s" % (card_id, suffix)
    dest_obj = "%s/%s" % (TEXTURE_FOLDER, name)
    if not os.path.isfile(png):
        _warn("%s: texture source missing: %s" % (card_id, png))
        return None
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", png)
    task.set_editor_property("destination_path", TEXTURE_FOLDER)
    task.set_editor_property("destination_name", name)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("automated", True)
    task.set_editor_property("save", False)
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    asset_tools.import_asset_tasks([task])
    tex = unreal.EditorAssetLibrary.load_asset(dest_obj)
    if tex is None:
        _warn("%s: texture import produced nothing for %s" % (card_id, name))
        return None
    tex.set_editor_property("srgb", srgb)
    tex.set_editor_property("compression_settings", compression)
    unreal.EditorAssetLibrary.save_loaded_asset(tex, False)
    return tex


def _ensure_textures_and_mi(card_id, result):
    """Import D/N/ORM + create+wire MI_<CardID>_PBR. Idempotent: skips if the MI
    already exists. Returns the MI object path or None."""
    mi_pkg = PBR_MI_FMT % card_id
    if unreal.EditorAssetLibrary.does_asset_exist(mi_pkg):
        result["notes"].append("MI already exists (%s) -- texture/MI step skipped." % mi_pkg)
        return mi_pkg

    d = _import_texture(card_id, "D", True, unreal.TextureCompressionSettings.TC_DEFAULT)
    n = _import_texture(card_id, "N", False, unreal.TextureCompressionSettings.TC_NORMALMAP)
    orm = _import_texture(card_id, "ORM", False, unreal.TextureCompressionSettings.TC_MASKS)
    if d is None or n is None or orm is None:
        result["notes"].append("WARN: one or more textures failed to import; MI not created.")
        return None
    result["steps"].append("textures")

    master = unreal.EditorAssetLibrary.load_asset(MASTER_PBR)
    if master is None:
        result["notes"].append("ERROR: master %s missing; MI not created." % MASTER_PBR)
        return None

    # NOTE: UMaterialInstanceConstantFactoryNew.InitialParent is NOT a reflected
    # editor property in UE 5.8 (set_editor_property raises). Create with a bare
    # factory, then set the parent via MaterialEditingLibrary (reliable).
    factory = unreal.MaterialInstanceConstantFactoryNew()
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    mi = asset_tools.create_asset("MI_%s_PBR" % card_id, MI_FOLDER,
                                  unreal.MaterialInstanceConstant, factory)
    if mi is None:
        result["notes"].append("ERROR: could not create MI_%s_PBR." % card_id)
        return None
    mel = unreal.MaterialEditingLibrary
    mel.set_material_instance_parent(mi, master)
    mel.set_material_instance_texture_parameter_value(mi, "BaseColor", d)
    mel.set_material_instance_texture_parameter_value(mi, "Normal", n)
    mel.set_material_instance_texture_parameter_value(mi, "ORM", orm)
    unreal.EditorAssetLibrary.save_loaded_asset(mi, False)
    result["steps"].append("mi")
    _log("%s: created MI_%s_PBR (BaseColor/Normal/ORM wired)." % (card_id, card_id))
    return mi_pkg


# --- Collision -------------------------------------------------------------

def _make_import_options(auto_collision):
    opts = unreal.FbxImportUI()
    opts.set_editor_property("import_mesh", True)
    opts.set_editor_property("import_as_skeletal", False)
    opts.set_editor_property("import_animations", False)
    opts.set_editor_property("import_materials", False)   # do NOT create M_ assets
    opts.set_editor_property("import_textures", False)
    opts.set_editor_property("create_physics_asset", False)
    opts.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_STATIC_MESH)
    sm_data = opts.get_editor_property("static_mesh_import_data")
    sm_data.set_editor_property("combine_meshes", True)
    sm_data.set_editor_property("generate_lightmap_u_vs", False)
    # We author collision ourselves (units: convex hulls; buildings: manifest
    # boxes). UCX_ collision nodes in the FBX are still recognized by the
    # importer regardless of this flag; we overwrite the agg_geom afterwards.
    sm_data.set_editor_property("auto_generate_collision", False)
    return opts


def _apply_unit_hulls(sm, result):
    """<=4 convex decomposition hulls (TASK-037 unit recipe). NON-FATAL."""
    try:
        lib = getattr(unreal, "EditorStaticMeshLibrary", None)
        if lib is not None:
            lib.remove_collisions(sm)
            lib.set_convex_decomposition_collisions(sm, HULL_COUNT, HULL_MAX_VERTS, HULL_PRECISION)
        else:
            subsys = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
            subsys.remove_collisions(sm)
            subsys.set_convex_decomposition_collisions(sm, HULL_COUNT, HULL_MAX_VERTS, HULL_PRECISION)
        result["steps"].append("collision_hulls")
        return True
    except Exception as ex:
        result["notes"].append("WARN: unit hull generation failed (%s); finish via MCP." % ex)
        _warn("%s: unit hull generation failed (%s)." % (result["card_id"], ex))
        return False


def _apply_box_collision(sm, boxes, result):
    """Author explicit UCX-analog BOX hull(s) from the manifest. Buildings +
    GoldNode. Deterministic wall-footprint collision (never auto/none)."""
    card_id = result["card_id"]
    if not boxes:
        result["notes"].append("WARN: no manifest boxes for %s; leaving imported collision." % card_id)
        return False
    try:
        # Bootstrap a body_setup deterministically (guarantees one exists even if
        # the FBX's UCX did not import), then overwrite its geometry with boxes.
        lib = getattr(unreal, "EditorStaticMeshLibrary", None)
        if lib is not None:
            lib.set_convex_decomposition_collisions(sm, 1, 16, HULL_PRECISION)
        else:
            subsys = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
            subsys.set_convex_decomposition_collisions(sm, 1, 16, HULL_PRECISION)

        body = sm.get_editor_property("body_setup")
        agg = body.get_editor_property("agg_geom")
        box_elems = []
        for b in boxes:
            c = b["center"]
            s = b["size"]
            be = unreal.KBoxElem()
            be.set_editor_property("center", unreal.Vector(float(c[0]), float(c[1]), float(c[2])))
            be.set_editor_property("x", float(s[0]))
            be.set_editor_property("y", float(s[1]))
            be.set_editor_property("z", float(s[2]))
            box_elems.append(be)
        agg.set_editor_property("convex_elems", [])
        agg.set_editor_property("sphere_elems", [])
        agg.set_editor_property("sphyl_elems", [])
        agg.set_editor_property("box_elems", box_elems)
        body.set_editor_property("agg_geom", agg)
        body.set_editor_property("collision_trace_flag",
                                 unreal.CollisionTraceFlag.CTF_USE_DEFAULT)
        try:
            body.invalidate_physics_data()
        except Exception:
            pass
        sm.set_editor_property("body_setup", body)
        result["steps"].append("collision_boxes(%d)" % len(box_elems))
        return True
    except Exception as ex:
        result["notes"].append("WARN: box collision authoring failed (%s)." % ex)
        _warn("%s: box collision authoring failed (%s)." % (card_id, ex))
        return False


# --- Per-card ---------------------------------------------------------------

def _reimport_one(card_id, manifest):
    result = {"card_id": card_id, "ok": False, "steps": [], "notes": []}
    category = _category_of(card_id, manifest)
    result["category"] = category
    pkg = "%s/SM_%s" % (MESH_FOLDER, card_id)
    obj = "%s.SM_%s" % (pkg, card_id)
    fbx = _fbx_path_for(card_id)

    # Preconditions -------------------------------------------------------
    if not os.path.isfile(fbx):
        result["notes"].append("SKIP: FBX not found: %s" % fbx)
        _warn("%s: FBX not found (%s) -- skipping." % (card_id, fbx))
        return result
    if not unreal.EditorAssetLibrary.does_asset_exist(pkg):
        result["notes"].append("SKIP: existing SM not found: %s (reimport only, never create)." % pkg)
        _warn("%s: %s does not exist -- skipping." % (card_id, pkg))
        return result

    result["referencers_before"] = list(
        unreal.EditorAssetLibrary.find_package_referencers_for_asset(pkg, False))

    # 1. Textures + MI (units + non-GoldNode buildings) -------------------
    # GoldNode keeps its authored M_GoldGlow emissive (spec: single-slot, NO
    # TeamColor, warm-yellow glow preserved) -- it gets NO MI_GoldNode_PBR and
    # no PBR textures (they would be orphan/unused and would drop the glow).
    if category != "goldnode":
        try:
            _ensure_textures_and_mi(card_id, result)
        except Exception as ex:
            # NON-FATAL: textured-mesh geometry is the priority. A texture/MI hiccup
            # is flagged and can be fixed later; it must never block the reimport.
            result["notes"].append("WARN: texture/MI step raised (%s); geometry reimport continues." % ex)
            _warn("%s: texture/MI step raised (%s); continuing to geometry reimport." % (card_id, ex))
    else:
        result["notes"].append("GoldNode: keeps M_GoldGlow emissive; no PBR MI/textures (spec).")

    # 2. In-place reimport ------------------------------------------------
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", fbx)
    task.set_editor_property("destination_path", MESH_FOLDER)
    task.set_editor_property("destination_name", "SM_%s" % card_id)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("replace_existing_settings", True)
    task.set_editor_property("automated", True)
    task.set_editor_property("save", False)
    task.set_editor_property("options", _make_import_options(auto_collision=False))
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    asset_tools.import_asset_tasks([task])
    imported = list(task.get_editor_property("imported_object_paths") or [])
    result["imported_object_paths"] = imported
    _log("%s: reimport issued %s -> %s (imported=%s)" % (card_id, fbx, obj, imported))
    result["steps"].append("reimport")

    sm = unreal.EditorAssetLibrary.load_asset(pkg)
    if sm is None:
        result["notes"].append("ERROR: could not load %s after reimport." % pkg)
        _err("%s: load_asset None after reimport." % card_id)
        return result

    static_materials = sm.get_editor_property("static_materials")
    slot_names = [str(m.get_editor_property("material_slot_name")) for m in static_materials]
    result["slot_names"] = slot_names
    _log("%s: %d slot(s) after reimport: %s" % (card_id, len(static_materials), slot_names))

    # 3. Nanite OFF -------------------------------------------------------
    nanite = sm.get_editor_property("nanite_settings")
    nanite.set_editor_property("enabled", False)
    sm.set_editor_property("nanite_settings", nanite)
    result["steps"].append("nanite_off")

    # 4. Collision per category ------------------------------------------
    if category == "unit":
        _apply_unit_hulls(sm, result)
    else:  # building or goldnode -> manifest box hulls
        boxes = (manifest.get(card_id, {}).get("ucx") or {}).get("boxes", [])
        _apply_box_collision(sm, boxes, result)

    # 5. Save -------------------------------------------------------------
    unreal.EditorAssetLibrary.save_loaded_asset(sm, False)
    result["steps"].append("save")

    # 6. Best-effort readback --------------------------------------------
    try:
        body = sm.get_editor_property("body_setup")
        agg = body.get_editor_property("agg_geom")
        result["convex_hull_count"] = len(agg.get_editor_property("convex_elems"))
        result["box_count"] = len(agg.get_editor_property("box_elems"))
    except Exception as ex:
        result["convex_hull_count"] = "unknown (%s)" % ex
        result["box_count"] = "unknown"
    result["nanite_enabled"] = bool(
        sm.get_editor_property("nanite_settings").get_editor_property("enabled"))
    result["referencers_after"] = list(
        unreal.EditorAssetLibrary.find_package_referencers_for_asset(pkg, False))
    result["ok"] = True
    _log("%s: DONE cat=%s slots=%s nanite=%s hulls=%s boxes=%s refs=%s"
         % (card_id, category, slot_names, result["nanite_enabled"],
            result.get("convex_hull_count"), result.get("box_count"),
            result["referencers_after"]))
    return result


def main():
    card_ids = _resolve_card_ids()
    manifest = _load_manifest()
    _log("Reimport batch starting for: %s" % card_ids)
    results = []
    for cid in card_ids:
        try:
            results.append(_reimport_one(cid, manifest))
        except Exception as ex:
            _err("%s: unhandled exception: %s" % (cid, ex))
            results.append({"card_id": cid, "ok": False, "notes": ["EXCEPTION: %s" % ex]})

    ok = [r["card_id"] for r in results if r.get("ok")]
    bad = [r["card_id"] for r in results if not r.get("ok")]
    _log("Batch complete. OK=%s  NOT_OK=%s" % (ok, bad))
    _log("SUMMARY_JSON " + json.dumps(results))
    return results


# PythonScriptCommandlet executes the module top-level.
main()
