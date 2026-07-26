# TASK-292b — LWC-precision fix integration result (build-master)

**Branch:** `m7.6-arena10x` · **Date:** 2026-07-24 · scene edit + git, NO compile. Consumes the TASK-292 diagnosis (`handoffs/TASK-292.md`).
**Outcome: PARTIAL fix — the VISTA DF/GI source is eliminated (verified); a RESIDUAL scatter-DF NaN persists and needs a gameplay-programmer code fix (diagnosis §6.3 fallback b).**

## What I applied (build-master lane — done + verified)
On all **26 dressing components** (16 vista `StaticMeshActor_7..22` + 8 POI + 2 gold `SM_GoldNodeProp`), set via `ObjectTools.set_properties` on each `StaticMeshComponent0`:
- `bAffectDistanceFieldLighting = false`
- `bAffectDynamicIndirectLighting = false`
Readback-verified: `found=26, set_ok=26, verified_off=26` (both flags false on all 26). Used the label `GoldProp` (not `Gold`) so **no gameplay `GoldNode` actor was touched**. Saved `L_Arena`.

## Verify (fresh editor restart — required for a clean DF/Lumen rebuild)
Editor restarted (PID 16232 had crashed → MCP had reconnected to a live editor; I save-all'd + graceful-closed it, relaunched fresh PID 8644 on L_Arena). Fresh editor loaded my saved flags from disk (vista component reads `bAffectDistanceFieldLighting=false`). PIE:
- **Generate 1 (seed 1857084801): Message Log CLEAN** — NO `OriginX <= OriginMax` ensure, NO `InverseFast … non-invertible → NaN`, no per-frame NaN spam, not even the `InputMode:UIOnly` HUD warning. Traversability CONFIRMED (0 culls). ⇒ the vista DF/GI contribution is REMOVED.
- **Generate 2 (seed 1529263745): the ensure + InverseFast NaN RETURNED** — firing right after `GenerateScatter`. Traversability still CONFIRMED.

⇒ **The vista fix works (removes the vista source), but the NaN is now proven to ALSO come from the SCATTER layers' DF participation** — exactly the pre-existing TASK-287 source the diagnosis §6.3 predicted ("the OriginX ensure predates the vista … most likely the scatter layers' DF at 10× coords"). It is seed-dependent (which scattered instance's DF matrix goes singular), hence intermittent.

## Residual → gameplay-programmer (fallback b, a CODE change — NOT build-master's lane)
Fallback (a) "pull the vista ring inside 30,000" is **inapplicable** — the vistas are already out of DF (flags off), so the residual is not theirs. The correct lever is **fallback (b): drop `bAffectDistanceFieldLighting` (and `bAffectDynamicIndirectLighting`) on the scatter HISM layers.** There is NO DA field for this today (`FScatterLayer` has cull/shadow from TASK-284 but no DF flag), so it needs a **`BattlefieldScatter.cpp` change** (set the flags on each HISM at `ResolveComponentForMesh`/`ResolveProxyForVisual`, alongside the existing `SetCullDistances`/`SetCastShadow`), optionally a new `FScatterLayer` bool. That is gameplay-programmer code (+ QA + compile). **Handed back — recommend a follow-up task (e.g. TASK-292c).** Do NOT touch any `r.LWC.*`/world-tile ini (diagnosis §2 proves it can't help).

## Committed / not
Committed: `Content/Maps/L_Arena.umap` (the 26-component DF-flag fix, LFS) + `handoffs/TASK-292.md` (diagnosis) + this file + board. **Branch-owned-only: L_Arena only — no code/DA/fleet touched.** `DeckBuilderWidget`/`WBP_DeckBuilder` (parked) untouched. No push. The vista fix is correct + net-positive regardless (distant dressing should never feed DF/Lumen) and isolates the residual to the scatter for the follow-up. **The ensure/NaN is NOT fully eliminated until the scatter-DF follow-up lands** — flagged for Jonathan's W-gate + gameplay-programmer.
