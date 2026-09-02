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
#   4. LODs (M7.6 classic-LOD law, TASK-220): assign lod_group='LargeProp' (the
#      engine auto-generates that group's 4-LOD reduction chain on build) --
#      EXCEPT castle-class meshes (CASTLE_CLASS_CARD_IDS), which instead get the
#      explicit CASTLE_LOD_CHAIN reduction: LOD1 50% @ screenSize 0.4, LOD2 25%
#      @ 0.15, NO LOD3 (landmark-silhouette law, CONVENTIONS "Arena 10x
#      scale-up & LOD/perf (M7.6)").
#   5. COLLISION (per-CardID mode -- the M7 building extension). The category is
#      READ FROM THE MANIFEST AND NEVER GUESSED: an unmanifested CardID is a hard
#      STOP, because the only destructive branch ('unit') is the one a wrong guess
#      would pick (TASK-768 defect 2).
#        - unit                : <=4 convex decomposition hulls (TASK-037 recipe).
#          DESTRUCTIVE -- removes any authored UCX first.
#        - building / GoldNode WITH manifest `ucx.boxes`: explicit UCX-analog BOX
#          hull(s) authored from the manifest (wall-footprint-exact, ground-center
#          space), so a building NEVER ships with no/auto collision (the M1
#          castle-plinth placement dead-zone lesson). NO convex decomposition on
#          buildings (a single auto-hull on a hollow tower re-introduces the
#          interior dead-zone). This OVERWRITES agg_geom, UCX included.
#        - building WITH `ucx.boxes: []`: the FBX's own UCX_ hulls are the
#          collision authority and are KEPT (WatchTower, declared under SC-15).
#          Engine-FABRICATED hulls (bIsGenerated True) are stripped so the mesh
#          keeps only what an artist authored.
#        - decorative categories (castle-prop, `ucx: null`): may ship with no
#          collision; exempt from the zero-collision STOP.
#   6. Save the package.
#   7. Readback + VERDICT (slots / nanite / hull+box count / PER-HULL bIsGenerated
#      / LOD count+group / SOCKETS vs the manifest / refs). Collision is a real
#      gate, not a log line: zero collision on a unit/building/GoldNode, a box
#      count that disagrees with the manifest, or a failed collision readback all
#      set ok=False and print a STOP. A call that returned success is not
#      evidence.
#
# COLLISION IMPORT FLAG -- do not "tidy" it (TASK-768 defect 1):
#   _make_import_options() MUST pass auto_generate_collision=True. On UE 5.8 that
#   bool is the MASTER SWITCH for collision import, not a "fabricate simple
#   collision" toggle: FBX now goes through Interchange, which maps the bool onto
#   BOTH bCollision and the fallback Collision type, and with it False the UCX_
#   nodes are never even fetched (measured: False -> 0 hulls, True -> 8 hulls).
#   The long comment at that call site has the measurement and the engine
#   line references. The bug hid for so long because every branch overwrote
#   agg_geom afterwards -- the same overwrite that would have silently discarded
#   hand-authored collision and still reported DONE.
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

# Socket read-back (TASK-768). Per-axis tolerance in uu when comparing a shipped
# socket to the manifest's declared position. Not zero: the FBX round-trip lands
# exact-ish, not exact (a measured LadderTop read back Y=-0.0002).
SOCKET_TOLERANCE_UU = 0.5

# M7.6 classic-LOD law (TASK-220; CONVENTIONS "Arena 10x scale-up & LOD/perf
# (M7.6)"): every reimported SM gets DEFAULT_LOD_GROUP (the engine auto-builds
# that group's 4-LOD reduction chain) -- EXCEPT castle-class meshes, which keep
# their landmark silhouette via the explicit CASTLE_LOD_CHAIN reduction (NO
# LOD3). Data-driven like the card list: add a CardID to CASTLE_CLASS_CARD_IDS
# to opt it out of the group default; no code-path edit needed.
DEFAULT_LOD_GROUP = "LargeProp"
CASTLE_CLASS_CARD_IDS = [
    "Castle", "Castle_Crumble01", "Castle_Crumble02", "Castle_Crumble03",
]
# (percent_triangles, screen_size) pairs, LOD0 first -- the set_lods contract.
CASTLE_LOD_CHAIN = [
    (1.00, 1.00),   # LOD0: full-detail landmark mesh
    (0.50, 0.40),   # LOD1: 50% tris @ screenSize 0.4
    (0.25, 0.15),   # LOD2: 25% tris @ screenSize 0.15 (NO LOD3 -- silhouette law)
]

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
        with open(sidecar, "r", encoding="utf-8") as fh:
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
    """Returns the manifest's `assets` dict, or None if it could not be loaded.

    None vs {} is the whole point. This used to swallow a decode failure and
    return {}, which is indistinguishable from "loaded fine, no entries" -- and
    downstream, {} meant every CardID looked absent. Composed with the old
    _category_of() default that turned into: a single typo in the JSON silently
    reclassified EVERY card as a unit, i.e. remove_collisions() +
    convex-decompose across the whole batch, behind a green-looking run.
    _category_of()'s STOP already blocks the destruction, but a failed manifest
    is a batch-level fact and deserves to be reported as one rather than as N
    confusing per-card STOPs, so main() halts on None.
    """
    try:
        with open(MANIFEST_PATH, "r", encoding="utf-8") as fh:
            return json.load(fh).get("assets", {})
    except Exception as ex:
        _err("MANIFEST_LOAD_FAILED: could not load %s: %s" % (MANIFEST_PATH, ex))
        return None


def _fbx_path_for(card_id):
    content_dir = unreal.Paths.project_content_dir()
    return os.path.normpath(os.path.join(content_dir, "RawAssets", "%s.fbx" % card_id))


def _png_path_for(card_id, suffix):
    content_dir = unreal.Paths.project_content_dir()
    return os.path.normpath(os.path.join(
        content_dir, "RawAssets", "Textures", card_id, "T_%s_%s.png" % (card_id, suffix)))


def _category_of(card_id, manifest):
    """Returns the manifest category ('unit' / 'building' / 'goldnode' /
    'castle-prop' / ...), or None when the CardID is UNMANIFESTED or declares no
    category.

    TASK-768 defect 2: this used to default a missing entry to 'unit'. 'unit' is
    the ONE destructive branch -- _apply_unit_hulls() calls remove_collisions()
    and then set_convex_decomposition_collisions(), i.e. an unmanifested CardID
    silently DELETED its hand-authored UCX hulls, let a solver pick the shape,
    and reported DONE. That is not a defaultable question, so there is no
    default any more: None means STOP, and _reimport_one() refuses the card
    BEFORE it touches anything.

    This is not hypothetical in this repo. Castle_Crumble01/02/03 carry 66
    authored UCX hulls each, are named in CASTLE_CLASS_CARD_IDS below (so this
    script clearly expects to process them), and have NO manifest entry -- under
    the old default, listing one in the sidecar decomposed 66 hulls into 4.
    """
    entry = manifest.get(card_id)
    if not entry:
        return None
    cat = entry.get("category")
    if not cat:
        return None
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
    """(Re)import D/N/ORM, then create+wire MI_<CardID>_PBR if it does not exist
    yet. Returns the MI object path or None.

    TASK-768 defect 3: this used to return EARLY when the MI already existed,
    which skipped the TEXTURE import along with it. That made a re-BAKE
    unshippable through this script -- the geometry reimports, the existing MI
    is reused, and the mesh keeps sampling the OLD T_<CardID>_* assets. It is a
    silent failure by construction: the MI, the slot names and the slot pointers
    all read back correct, and only the PIXELS are stale, so nothing in the
    readback can see it. It bites hardest on exactly the case this script exists
    for -- a mesh whose UVs were repacked by an edit (e.g. WatchTower's
    translation, which repacked 2508/2508 loop UVs), where stale textures mean
    the mesh samples garbage while every measurement says PASS.

    Textures are now ALWAYS reimported; only the MI CREATION is conditional.
    That is safe and ref-preserving: _import_texture() uses
    AssetImportTask(replace_existing=True) over the same
    /Game/Textures/T_<CardID>_* paths, so it is the same in-place overwrite this
    whole script is built on, and the MI's texture parameters keep pointing at
    the same UObjects.
    """
    mi_pkg = PBR_MI_FMT % card_id
    mi_exists = unreal.EditorAssetLibrary.does_asset_exist(mi_pkg)

    d = _import_texture(card_id, "D", True, unreal.TextureCompressionSettings.TC_DEFAULT)
    n = _import_texture(card_id, "N", False, unreal.TextureCompressionSettings.TC_NORMALMAP)
    orm = _import_texture(card_id, "ORM", False, unreal.TextureCompressionSettings.TC_MASKS)
    missing = [s for s, t in (("D", d), ("N", n), ("ORM", orm)) if t is None]
    if missing:
        result["notes"].append(
            "WARN: texture (re)import failed for %s: %s." % (card_id, ", ".join(missing)))
    else:
        result["steps"].append("textures")
        _log("%s: reimported T_%s_{D,N,ORM}." % (card_id, card_id))

    if mi_exists:
        result["notes"].append(
            "MI already exists (%s) -- textures REIMPORTED, MI creation skipped." % mi_pkg)
        return mi_pkg
    if missing:
        result["notes"].append("WARN: MI not created (textures incomplete).")
        return None

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

def _make_import_options(import_collision):
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
    # COLLISION MASTER SWITCH -- MUST STAY True (TASK-768 defect 1).
    #
    # The comment that used to sit here claimed UCX_ nodes are "recognized by
    # the importer regardless of this flag", and set it False. That is FALSE on
    # UE 5.8. Measured one variable at a time on WatchTower.fbx (8 authored
    # UCX_SM_WatchTower_00..07 nodes), everything else held constant:
    #     auto_generate_collision=False -> hulls=0, is_generated=[]
    #                                      MeshPayload [1 836 558]  (1 mesh)
    #     auto_generate_collision=True  -> hulls=8, is_generated=[False x8]
    #                                      MeshPayload [9 928 620]  (1 + 8 UCX)
    # (Saved/Logs/GitClaudeUnrealTest-backup-2026.09.02-03.17.35.log:2712-2720;
    # the payload counts are the proof the UCX nodes are not merely ignored --
    # with False they are never even FETCHED.)
    #
    # WHY the flag gates processing and not fabrication: UE 5.6+ imports FBX
    # through INTERCHANGE, not the legacy UnFbx::FFbxImporter. (This project's
    # log shows LogInterchangeImport + InterchangeFbxParser and ZERO LogFbx
    # lines.) Interchange collapses this one legacy bool onto TWO settings --
    # InterchangeFbxAssetImportDataConverter.cpp:316-317:
    #     MeshPipeline->bCollision = bAutoGenerateCollision
    #     MeshPipeline->Collision  = bAutoGenerateCollision ? Convex18DOP : None
    # and bCollision is a MASTER SWITCH: InterchangeLODDataParser.cpp:739 drops
    # every collision-mesh UID when it is false, and
    # InterchangeStaticMeshFactory.cpp:796 gates ALL collision work on it.
    # The old comment was not nonsense -- it described PRE-Interchange
    # behaviour, where ImportCollisionModels() ran unconditionally
    # (FbxStaticMeshImport.cpp:598) and the bool only gated the KDOP18 fallback
    # (:2434). It is STALE, and it outlived the importer it described.
    #
    # THE COST OF True -- the Convex18DOP FALLBACK is now armed. The factory
    # fabricates it ONLY when no custom collision was imported
    # (InterchangeStaticMeshFactory.cpp:798, `!bImportedCustomCollision`) and
    # stamps it bIsGenerated=True (GeomFitUtils.cpp:114). So a mesh WITH UCX is
    # untouched, but a mesh WITHOUT UCX now gains one fabricated hull where it
    # previously got none. _strip_generated_hulls() removes exactly those, which
    # keeps this flip a no-op for every card except the one it is meant to fix.
    sm_data.set_editor_property("auto_generate_collision", bool(import_collision))
    return opts


def _strip_generated_hulls(sm, result):
    """Remove convex hulls the ENGINE fabricated, keep the ones the FBX authored.

    FKShapeElem::bIsGenerated is "True when the shape was created by the engine
    and was not imported" (ShapeElem.h:128-130), and only the GeomFitUtils
    generators set it (GeomFitUtils.cpp:114). Imported UCX hulls keep the
    constructor default False (ShapeElem.h:41).

    This exists because _make_import_options() must pass
    auto_generate_collision=True to get UCX at all, and Interchange arms the
    Convex18DOP FALLBACK off the same bool. A mesh WITH UCX never triggers the
    fallback, but a mesh WITHOUT UCX would now come back carrying one fabricated
    hull it did not have before. Stripping by bIsGenerated keeps the flag fix
    end-state-neutral for those. Returns (kept, stripped).
    """
    try:
        body = sm.get_editor_property("body_setup")
        agg = body.get_editor_property("agg_geom")
        elems = list(agg.get_editor_property("convex_elems"))
        kept = [e for e in elems if not bool(e.get_editor_property("is_generated"))]
        stripped = len(elems) - len(kept)
        # `if stripped:` is load-bearing, not an optimisation. It keeps the
        # no-op case from round-tripping authored hulls through python structs
        # at all -- FKConvexElem's VertexData/IndexData are bare UPROPERTY()
        # (ConvexElem.h:36-40), reflected but not editor-visible, so they ride
        # along on a struct copy but cannot be inspected to prove it.
        # In practice the mixed case cannot arise: the Convex18DOP fallback only
        # fires when NOTHING was imported (InterchangeStaticMeshFactory.cpp:798),
        # so `kept` is either every hull (stripped == 0 -> no write) or empty
        # (-> we assign [], which carries no vertex data to lose).
        if stripped:
            agg.set_editor_property("convex_elems", kept)
            body.set_editor_property("agg_geom", agg)
            try:
                body.invalidate_physics_data()
            except Exception:
                pass
            sm.set_editor_property("body_setup", body)
            result["steps"].append("stripped_generated_hulls(%d)" % stripped)
            _log("%s: stripped %d engine-FABRICATED hull(s); %d authored kept."
                 % (result["card_id"], stripped, len(kept)))
        return len(kept), stripped
    except Exception as ex:
        result["notes"].append("WARN: could not filter generated hulls (%s)." % ex)
        _warn("%s: could not filter generated hulls (%s)." % (result["card_id"], ex))
        return None, None


def _apply_unit_hulls(sm, result):
    """<=4 convex decomposition hulls (TASK-037 unit recipe). NON-FATAL.

    DESTRUCTIVE: remove_collisions() discards ANY authored UCX. Guarded so it can
    only ever run for a card the manifest explicitly calls a unit -- see
    _category_of()'s TASK-768 note.
    """
    if result.get("category") != "unit":
        result["notes"].append(
            "STOP: _apply_unit_hulls refused for category=%r (destructive; units only)."
            % result.get("category"))
        _err("%s: _apply_unit_hulls refused for category=%r."
             % (result["card_id"], result.get("category")))
        return False
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
        # DELIBERATE PATH, not a hole: `ucx.boxes: []` means "the FBX's UCX_
        # hulls ARE the collision authority" (WatchTower's manifest entry
        # declares this under SC-15). We keep what the importer brought in --
        # minus anything the engine FABRICATED, which the Convex18DOP fallback
        # would otherwise leave on a UCX-less mesh (see _make_import_options).
        kept, stripped = _strip_generated_hulls(sm, result)
        result["notes"].append(
            "no manifest boxes for %s; KEEPING imported UCX collision "
            "(authored hulls kept=%s, fabricated stripped=%s)." % (card_id, kept, stripped))
        result["steps"].append("collision_ucx_preserved(%s)" % kept)
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


# --- LODs (M7.6 classic-LOD law, TASK-220) ---------------------------------

def _apply_lods(sm, card_id, result):
    """Default: lod_group=DEFAULT_LOD_GROUP ('LargeProp' -- the engine
    auto-generates that group's 4-LOD reduction chain). Castle-class meshes
    (CASTLE_CLASS_CARD_IDS) instead get the explicit CASTLE_LOD_CHAIN reduction
    via StaticMeshEditorSubsystem.set_lods (LOD1 50% @ 0.4 / LOD2 25% @ 0.15,
    NO LOD3 -- landmark silhouette). UE 5.8 API (QA-verified vs the installed
    engine, qa/TASK-216-217-220-qa.md): unreal.StaticMeshReductionOptions /
    unreal.StaticMeshReductionSettings (StaticMeshEditorSubsystemHelpers.h:18-53)
    + the always-loaded StaticMeshEditorSubsystem (StaticMeshEditorSubsystem.h:51)
    -- NOT the pre-5.0 EditorScripting* names, and no EditorStaticMeshLibrary
    fallback (its SetLods is a C++-only deprecated shim, not python-callable).
    NON-FATAL: a LOD hiccup is flagged but never blocks the reimport
    (collision-helper precedent); result['lod_ok'] + the LOD_STEP_FAILED token
    keep failures loud in the batch summary."""
    try:
        if card_id in CASTLE_CLASS_CARD_IDS:
            options = unreal.StaticMeshReductionOptions()
            options.set_editor_property("auto_compute_lod_screen_size", False)
            settings = []
            for pct, screen in CASTLE_LOD_CHAIN:
                s = unreal.StaticMeshReductionSettings()
                s.set_editor_property("percent_triangles", float(pct))
                s.set_editor_property("screen_size", float(screen))
                settings.append(s)
            options.set_editor_property("reduction_settings", settings)
            subsys = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
            if subsys is None:
                raise RuntimeError("StaticMeshEditorSubsystem unavailable")
            # Explicit per-mesh reduction only applies when NO LOD group is
            # set; clear it (only now that the options built -- keeps the
            # failure path state-neutral) so a re-run stays idempotent even
            # after a mistaken group assignment.
            sm.set_editor_property("lod_group", "None")
            lod_count = subsys.set_lods(sm, options)
            if int(lod_count) < 0:
                raise RuntimeError("set_lods returned %s" % lod_count)
            result["steps"].append("lods_castle(%d)" % int(lod_count))
        else:
            sm.set_editor_property("lod_group", DEFAULT_LOD_GROUP)
            result["steps"].append("lod_group(%s)" % DEFAULT_LOD_GROUP)
        result["lod_ok"] = True
        return True
    except Exception as ex:
        result["lod_ok"] = False
        result["notes"].append("LOD_STEP_FAILED (%s); mesh keeps prior LODs." % ex)
        _warn("%s: LOD_STEP_FAILED (%s); mesh keeps prior LODs." % (card_id, ex))
        return False


# --- Sockets (TASK-768) -----------------------------------------------------

def _readback_sockets(sm, card_id, manifest, result):
    """Read the shipped static-mesh SOCKETS back BY LITERAL NAME and compare them
    to the manifest's `sockets` block. NON-FATAL, but loud.

    WHY HERE AND NOT OVER MCP: UStaticMesh::Sockets is declared bare `UPROPERTY()`
    with no EditAnywhere/BlueprintReadWrite (StaticMesh.h:1500-1501), so the
    generic editor-property read MCP uses fails outright -- MCP cannot see
    sockets at all. UStaticMesh::FindSocket IS UFUNCTION(BlueprintPure)
    (StaticMesh.h:2315-2316), so a -run=pythonscript commandlet can reach them.
    This script is already the commandlet, so this is the cheap instrument.

    Looking sockets up BY NAME (not enumerating) is the point: it catches a
    MISNAMED socket as a miss, and the C++ that consumes these reads them by
    literal FName and DEGRADES OPEN to fallback literals when the lookup fails --
    so a typo is otherwise completely silent.
    """
    spec = (manifest.get(card_id) or {}).get("sockets") or {}
    wanted = dict((k, v) for k, v in spec.items() if not k.startswith("_"))
    if not wanted:
        return True
    report = {}
    bad = []
    for name in sorted(wanted):
        expect = wanted[name]
        try:
            sock = sm.find_socket(name)
        except Exception as ex:
            report[name] = "read failed (%s)" % ex
            bad.append("%s(read failed)" % name)
            continue
        if sock is None:
            report[name] = None
            bad.append("%s(MISSING)" % name)
            _err("%s: SOCKET %s MISSING on the shipped mesh." % (card_id, name))
            continue
        loc = sock.get_editor_property("relative_location")
        got = (round(float(loc.x), 4), round(float(loc.y), 4), round(float(loc.z), 4))
        exp = tuple(round(float(c), 4) for c in expect)
        match = all(abs(g - e) <= SOCKET_TOLERANCE_UU for g, e in zip(got, exp))
        report[name] = {"got": got, "expected": exp, "match": match}
        if not match:
            bad.append("%s(got %s want %s)" % (name, got, exp))
            _err("%s: SOCKET %s MISPLACED -- got %s, manifest wants %s."
                 % (card_id, name, got, exp))
        else:
            _log("%s: socket %-12s %s OK" % (card_id, name, got))
    result["sockets"] = report
    if bad:
        result["sockets_ok"] = False
        result["notes"].append("SOCKET_MISMATCH: %s" % "; ".join(bad))
        return False
    result["sockets_ok"] = True
    result["steps"].append("sockets(%d ok)" % len(wanted))
    return True


# --- Per-card ---------------------------------------------------------------

def _reimport_one(card_id, manifest):
    result = {"card_id": card_id, "ok": False, "steps": [], "notes": []}
    category = _category_of(card_id, manifest)
    result["category"] = category

    # HARD STOP: unmanifested CardID (TASK-768 defect 2) -------------------
    # Refused BEFORE anything is imported or overwritten, so the mesh on disk is
    # untouched. The category decides whether the destructive unit branch runs;
    # guessing it is how hand-authored hulls get silently decomposed.
    if category is None:
        msg = ("STOP: %s has no category in %s -- refusing to process it. "
               "The old code defaulted to 'unit', which would remove_collisions() "
               "and convex-decompose whatever the FBX authored. Add an entry with "
               "an explicit \"category\" and rerun." % (card_id, MANIFEST_PATH))
        result["notes"].append(msg)
        result["stop"] = "UNMANIFESTED_CARD"
        _err("%s: %s" % (card_id, msg))
        return result

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
    task.set_editor_property("options", _make_import_options(import_collision=True))
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

    # 4. LODs (M7.6 classic-LOD law, TASK-220) ---------------------------
    _apply_lods(sm, card_id, result)

    # 5. Collision per category ------------------------------------------
    boxes = ((manifest.get(card_id) or {}).get("ucx") or {}).get("boxes", [])
    if category == "unit":
        _apply_unit_hulls(sm, result)
    else:  # building / goldnode / prop -> manifest box hulls, else keep UCX
        _apply_box_collision(sm, boxes, result)
    # Which cards are ALLOWED to end up collisionless. A unit, a building and a
    # GoldNode are all placed, walked into or shot at, so zero collision on one
    # is a STOP (WatchTower's manifest _acceptance says so in as many words).
    # Decorative categories (e.g. castle-prop, whose entries carry `ucx: null`)
    # legitimately ship with none, so they are exempt rather than false-failed.
    result["collision_required"] = category in ("unit", "building", "goldnode")
    result["expected_boxes"] = len(boxes)

    # 6. Save -------------------------------------------------------------
    unreal.EditorAssetLibrary.save_loaded_asset(sm, False)
    result["steps"].append("save")

    # 7. Readback + VERDICT ----------------------------------------------
    # Readback is best-effort, but the collision verdict is NOT: a call that
    # returned success is not evidence, and this step used to set ok=True no
    # matter what the mesh actually ended up with.
    stops = []
    try:
        body = sm.get_editor_property("body_setup")
        agg = body.get_editor_property("agg_geom")
        convex = list(agg.get_editor_property("convex_elems"))
        result["convex_hull_count"] = len(convex)
        result["box_count"] = len(agg.get_editor_property("box_elems"))
        # bIsGenerated distinguishes hulls the FBX AUTHORED (False) from ones the
        # engine FABRICATED (True). Without it, one fallback-generated 18-DOP
        # reads exactly like real authored collision in the summary.
        #
        # IT ALSO ANSWERS "did this come from the FBX I just imported, or did it
        # SURVIVE from last time?" -- which a same-path reimport makes a real
        # question. InterchangeStaticMeshFactory.cpp:500-506 stashes the old
        # AggGeom and empties it, then :746-792:
        #   - if the new import produced NO collision, it restores the ENTIRE old
        #     AggGeom verbatim (:753-755);
        #   - otherwise it drops the old IMPORTED elements (EmptyImportedElements
        #     keeps only bIsGenerated ones) and re-creates the old EDITOR-GENERATED
        #     ones, copying bIsGenerated=True back with them (:760-791).
        # So after an import that DID bring collision, bIsGenerated False means
        # "from the FBX just imported" and True means "survivor or fallback".
        #
        # This is also the sharpest reason the old auto_generate_collision=False
        # was dangerous. It guaranteed zero imported collision, which took the
        # :753-755 restore branch -- so a building on the "leave imported
        # collision" path would have kept LAST BUILD'S hulls on the NEW geometry
        # and read back a healthy non-zero count. Worse than 0 hulls, and silent.
        gen = [bool(e.get_editor_property("is_generated")) for e in convex]
        result["hull_is_generated"] = gen
        result["authored_hull_count"] = gen.count(False)
        if any(gen):
            result["notes"].append(
                "COLLISION_GENERATED: %d of %d hull(s) were fabricated by the engine, "
                "not authored in the FBX." % (gen.count(True), len(gen)))
            _warn("%s: COLLISION_GENERATED %d/%d hulls." % (card_id, gen.count(True), len(gen)))
        total = result["convex_hull_count"] + result["box_count"]
        if result.get("collision_required") and total == 0:
            stops.append("COLLISION_ZERO")
            result["notes"].append(
                "COLLISION_ZERO: %s (category=%s) ended with NO collision at all. "
                "This is a STOP -- do not ship it. Check that the FBX's UCX_ nodes "
                "imported (auto_generate_collision must be True) or that the manifest "
                "declares ucx.boxes." % (pkg, category))
            _err("%s: COLLISION_ZERO -- 0 hulls and 0 boxes on %s." % (card_id, pkg))
        exp_boxes = result.get("expected_boxes", 0)
        if exp_boxes and result["box_count"] != exp_boxes:
            stops.append("BOX_COUNT_MISMATCH")
            result["notes"].append(
                "BOX_COUNT_MISMATCH: manifest declares %d box(es), mesh has %d."
                % (exp_boxes, result["box_count"]))
            _err("%s: BOX_COUNT_MISMATCH manifest=%d mesh=%d."
                 % (card_id, exp_boxes, result["box_count"]))
    except Exception as ex:
        result["convex_hull_count"] = "unknown (%s)" % ex
        result["box_count"] = "unknown"
        result["hull_is_generated"] = "unknown"
        stops.append("COLLISION_READBACK_FAILED")
        result["notes"].append("COLLISION_READBACK_FAILED (%s) -- collision UNVERIFIED." % ex)
        _err("%s: collision readback failed (%s); collision UNVERIFIED." % (card_id, ex))

    # Sockets: loud, but never blocks the geometry reimport (art-side fix).
    try:
        _readback_sockets(sm, card_id, manifest, result)
    except Exception as ex:
        result["sockets_ok"] = False
        result["notes"].append("SOCKET_READBACK_FAILED (%s)." % ex)
        _warn("%s: socket readback failed (%s)." % (card_id, ex))

    try:
        result["lod_count"] = int(sm.get_num_lods())
        result["lod_group"] = str(sm.get_editor_property("lod_group"))
    except Exception as ex:
        result["lod_count"] = "unknown (%s)" % ex
        result["lod_group"] = "unknown"
    result["nanite_enabled"] = bool(
        sm.get_editor_property("nanite_settings").get_editor_property("enabled"))
    result["referencers_after"] = list(
        unreal.EditorAssetLibrary.find_package_referencers_for_asset(pkg, False))
    result["stops"] = stops
    result["ok"] = not stops
    _log("%s: %s cat=%s slots=%s nanite=%s hulls=%s authored=%s boxes=%s lods=%s "
         "lod_group=%s sockets=%s refs=%s"
         % (card_id, "DONE" if result["ok"] else "STOP(%s)" % ",".join(stops),
            category, slot_names, result["nanite_enabled"],
            result.get("convex_hull_count"), result.get("authored_hull_count"),
            result.get("box_count"), result.get("lod_count"), result.get("lod_group"),
            result.get("sockets_ok", "n/a"), result["referencers_after"]))
    return result


def main():
    card_ids = _resolve_card_ids()
    manifest = _load_manifest()
    if manifest is None:
        _err("BATCH STOP: the manifest could not be loaded, so no card's category "
             "is known. Nothing was imported and nothing was modified. Fix %s and "
             "rerun." % MANIFEST_PATH)
        _log("SUMMARY_JSON " + json.dumps(
            [{"card_id": c, "ok": False, "stop": "MANIFEST_LOAD_FAILED"} for c in card_ids]))
        return []
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

    # STOPs first and loudest -- these mean "do not ship this mesh".
    stopped = [(r["card_id"], r.get("stops") or [r.get("stop")])
               for r in results if r.get("stops") or r.get("stop")]
    if stopped:
        _err("STOP on %d asset(s): %s"
             % (len(stopped), ", ".join("%s(%s)" % (c, ",".join(s)) for c, s in stopped)))
    zero = [r["card_id"] for r in results if "COLLISION_ZERO" in (r.get("stops") or [])]
    if zero:
        _err("COLLISION_ZERO on %s -- those meshes have NO collision. Do NOT commit them."
             % zero)

    lod_bad = [r["card_id"] for r in results if r.get("lod_ok") is False]
    if lod_bad:
        _warn("LOD_STEP_FAILED on %d asset(s): %s -- LODs NOT applied there "
              "(meshes keep their prior chain; grep LOD_STEP_FAILED above)." % (len(lod_bad), lod_bad))
    sock_bad = [r["card_id"] for r in results if r.get("sockets_ok") is False]
    if sock_bad:
        _err("SOCKET_MISMATCH on %d asset(s): %s -- a missing or misplaced socket is "
             "SILENT at runtime (the C++ degrades open to fallback literals). Fix the "
             "FBX before this ships." % (len(sock_bad), sock_bad))
    _log("SUMMARY_JSON " + json.dumps(results))
    return results


# PythonScriptCommandlet executes the module top-level.
main()
