# TASK-684 — [WM-A2] THE ELEVATION BACKGROUND — programmer handoff (2026-08-29)

Runtime one-time trace bake per match (`WM-§2`), painted as the LOWEST native layer of the
war-map rect. File-only per the fences: ⛔ no compile (TASK-689 owns it), no editor, no Git,
no board writes beyond the status flip.

## 1. Files touched (the whole diff — nothing else)

| File | What |
|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/WarMapWidget.h` | pinned `HeightToBrightness` seam + `MapUVToWorld` on `FSiegeWarMapProjection` (declared addition, §5) + the three pinned tunables + private bake functions/state + comment-truth updates (`HandleMapRefreshTimer` = three jobs) |
| `Source/GitClaudeUnrealTest/Siegebound/WarMapWidget.cpp` | the bake + cache + paint-layer rework, fenced `═══ The elevation background (TASK-684; WM-§2) ═══` between `ResolveArenaHalfExtent` and the Ally-dots section; TASK-685 layers AFTER this fence |
| `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeWarMapTest.cpp` | EXTENDED (never a new file): tests 24 + 25 appended |

⚠️ The working tree also carries TASK-683 (textures), TASK-687 (`WBP_WarMap` + `DefaultEngine.ini`),
TASK-691's handoff and the manager's board/CONVENTIONS updates — none of it is mine; my diff is
exactly the three files above.

## 2. The bake design table

| Aspect | Decision | Source of authority |
|---|---|---|
| Grid | `ElevationGridX` **130** × `ElevationGridY` **60** = **7,800** cell-CENTRE samples, both `EditDefaultsOnly` (ClampMin 2 / ClampMax 1024; belt-and-braces re-clamp in the bake) | `WM-§2` pinned defaults |
| Sampled rect | `ResolveArenaHalfExtent()` — the SHIPPED asset→CDO resolver; ⛔ zero hand-typed dimensions | `SC-§34` |
| Sample points | texel-centre UV → world via **new pure `FSiegeWarMapProjection::MapUVToWorld`** — the exact inverse of the shipped `WorldToMapUV`, in the SAME single-owner struct; round trip pinned by test 25 | `SC-§15` declared addition (§5.1) |
| Trace | `LineTraceMultiByObjectType` for **object type `ECC_WorldStatic`**, ±50,000 Z bracket (the `GroundZAt` bracket, copied), `bTraceComplex=false` (the shipped idiom; hill hulls == render by the climbable-geometry law); **HIGHEST VISIBLE hit wins** (max, not first — no engine-sort assumption; `IsVisible()` rejects the invisible tree-trunk proxies) | §5.2 — THE load-bearing deviation |
| Excluded movers | pawns / units / projectiles excluded **BY OBJECT TYPE** (they are `Pawn`/`WorldDynamic` objects) — the spec's "ignore pawns/units/projectiles" with no per-actor ignore list to go stale | `WM-§2` |
| Normalization | `GroundZ` = **minimum sampled hit Z** (measured off the bake, never a transcribed "floor = 0"); brightness = pinned pure `HeightToBrightness(HitZ, GroundZ, ElevationReliefCeiling)` = `clamp((HitZ−GroundZ)/max(Ceiling,1), 0, 1)`; **above-ceiling ⇒ exactly 1 = full white** (castles/walls read as "walls" — the `WM-§6` declared designed outcome) | `WM-§2` |
| `ElevationReliefCeiling` default | **1,000 uu, MEASURED**: tallest hill mesh `SM_Hill_02` authored height **400 uu** (CONVENTIONS "Climbable terrain (M6.6)" table: 250/400/350, base pivot z_min=0) × the shipped DA Hills-layer `ScaleRange` max **2.5** (`handoffs/TASK-251.md`; uniform scale by the scatter law) = 1,000. Cross-check: the W1-PREP nav-Z cap law keeps the tallest crown under ±1,200 — 1,000 < 1,200 ✓ | derivation pasted per `WM-§2` |
| Output | ONE transient `UTexture2D` (`CreateTransient`, `PF_B8G8R8A8`, 130×60), sRGB, **bilinear** (texels stretch into smooth slopes); texel byte = `Lerp(ElevationFloorLuma 0.10, 1.0, brightness)` — the pure seam owns the RAMP, the painter owns only the two screen ends, so the pinned 3-param signature stays exact (`SC-§33`) | `WM-§2` (no `.uasset`, no editor bake, no per-seed content) |
| GC | `UPROPERTY(Transient) TObjectPtr<UTexture2D>` roots the texture; the non-reflected `FSlateBrush` only mirrors it (stated at the member) | — |
| Cost | 7,800 traces + one texture upload **ONCE per match** (plus once per Play Again); steady state = TWO pointer checks per 0.25 s timer tick; **ZERO per-frame** (`NativePaint` draws one cached brush) | `WM-§2` |
| Failure | no world / zero hits / allocation failure ⇒ `false` ⇒ texture dropped, **the shipped flat background stands**, ONE latched `LogSiegeWarMap` **Log** line; ⛔ never a crash, ⛔ no per-frame or Warning spam; at most ONE failed attempt per open (`bElevationBakeFailedThisOpen`), later opens retry | spec item (4) |
| Success log | ONE line per bake with figures: `samples, hits, groundZ, maxZ (relief), ceiling, clamped-white texel count` — TASK-689's verify instrument | — |

## 3. Paint-order proof (`WM-§2`'s draw-order law)

`NativePaint` layer block (the only paint change): `ElevationLayer = MaxLayer + 1` — MaxLayer is
`Super::NativePaint`'s return, i.e. everything the WBP painted, so the elevation sits **above the
WBP background panel and below every native layer**. Every shipped layer moved up exactly one,
**order preserved**: Ally `+2`, Enemy `+3`, Marker `+4`, Label `+5`; return stays
`Max(MaxLayer, LabelLayer)`. The comment in the block reserves the gap between Elevation and Ally
for TASK-685's POI icons by name. The elevation box is drawn only when `ElevationTexture != nullptr`
— a null texture paints the byte-identical pre-684 frame. `NativePaint` stays `const` and only
READS the texture/brush; every writer is a non-const path (OpenMap / the refresh timer) — the
"painting cannot change what is painted" guarantee is untouched (stated at the members).
Known interaction, declared: any WBP chrome INSIDE the map rect now sits under the elevation layer
— per the TASK-568 contract chrome lives outside the rect, and TASK-687 is moving the status line
clear of the screen bands anyway.

## 4. The Play-Again re-bake path (the `SC-§20` diagnosis — read this, QA)

The spec's suggested `TWeakObjectPtr<UWorld>` key is **kept but provably insufficient alone**:
**Play Again is an IN-PLACE reset** — `ASiegeGameMode::PlayAgain` drives `ClearScatter()` +
`GenerateScatter()` on the SAME `UWorld` (no travel; verified across BattlefieldScatter.h/.cpp's
own Play-Again lifecycle comments), so the hills re-roll while the world pointer never changes.
Diagnosed staleness key, layered on the world key:

- **The scatter-generation SENTINEL**: `ClearScatter` DESTROYS the scatter-spawned mines and the
  ancient-ground pair and every generate spawns FRESH ones ("destroyed, never pooled" —
  BattlefieldScatter.h). A `TWeakObjectPtr<const AActor>` to one of them (mine first — the economy
  law guarantees them on the shipped field; else an ancient ground) goes invalid EXACTLY when the
  field re-rolled. ⛔ **Identity only** — no position/name/state is ever read off it; this is NOT a
  place iterator (`WR-§6` — `ResolvePlace` stays the single owner of place→position), and the
  top-of-file EngineUtils comment + the Ally-dots fence comment were updated to audit it.
- **Re-bake sites**: `OpenMap` (lazy first bake + stale-on-reopen) AND `HandleMapRefreshTimer`
  (the existing timer, the TASK-579 one-timer doctrine) — so a Play Again UNDER an open map
  self-heals within one 0.25 s interval at two-pointer-check steady cost.
- **The unarmed branch**: baked with NO scatter actors present (early-open client whose
  deterministic regenerate hadn't run; debug fields) ⇒ the timer watches for one APPEARING and
  re-bakes then. A permanently scatter-less field bakes once and never thrashes.

## 5. Deviations of record (`SC-§15`) — three, each argued at source

1. **`MapUVToWorld` added to `FSiegeWarMapProjection`** (public pure static, not in the pinned
   names). The bake needs the world→map transform run BACKWARDS; writing `(2u−1)·HalfX` inline in
   the widget would be a second, unshared copy of the orientation contract — the exact
   "re-derive the transform" drift the single-owner struct exists to prevent. Same zero-divide
   floor, no clamp (documented why), round-trip-pinned by test 25 on the bake's own texel lattice.
2. **Object-type query, not a channel trace.** The spec spells `ECC_WorldStatic`; the
   scatter-channel law (verified at `BattlefieldScatter.cpp` `ResolveComponentForMesh`:
   ignore-all + Block Pawn/Team/Visibility/Camera only) makes every hill HISM **IGNORE the
   `ECC_WorldStatic` channel** — deliberately, so `GroundZAt`'s floor trace passes through them.
   A by-channel trace returns the flat floor + castles: the uniform-gray ground-only bake
   `WM-§2` itself refuses. The hills ARE WorldStatic **objects** (`SetCollisionObjectType`),
   and object queries filter on object type, not channel responses ⇒ floor + hills + rocks +
   castles + walls in, pawns/units/projectiles out by construction. The pinned token is still
   the filter — as the object type, the reading that delivers the ruling's WHY.
3. **`bTraceComplex=false`** (the dispatch wording said "complex down-traces"): the shipped
   `GroundZAt` idiom; simple collision IS the ground truth by authored law (hill hulls are
   `hull_count=1` with hull ZMax == mesh ZMax — the climbable-geometry acceptance gate; the
   castle's 61 hulls are its collision truth). Per-poly would cost more to say the same thing.

## 6. Tests + suite expectation

`Tests/SiegeWarMapTest.cpp` EXTENDED with **2** new cases (file 23 → 25 macros):

- **24 `Siegebound.WarMap.HeightToBrightnessRampFloorCeilingAndClamp`** — floor⇒0 dark (at Z=0
  AND at a lifted measured ground), ceiling⇒1 light, castle-class 8,000⇒exactly 1 (the clamp IS
  the law), quarter-points exact, 61-step monotonic sweep (non-decreasing everywhere, strictly
  increasing in the open band), below-ground⇒0, degenerate zero/negative ceiling ⇒ finite +
  clamped (the 1-uu floor).
- **25 `Siegebound.WarMap.MapUvToWorldInvertsTheProjection`** — centre/corners on clean + shipped
  CDO extents (the Y row carries the pinned axis flip), round trip over the bake's own
  `(i+0.5)/N` texel lattice on both arenas, degenerate (0,0) extent finite + round trip closes.

**Suite expectation: 140 → 142** (baseline measured: `IMPLEMENT_SIMPLE_AUTOMATION_TEST` count
across `Source/.../Tests/` = 140, matching TASK-678's 140/140 gate; my delta = +2; total counted
from source after the edit = 142).

## 7. Airlock re-statement (spec item 6) + M8

- Discharge grep over my three-file diff's ADDED lines for
  `Capture( | EnsureSnapshot | GetTurnSnapshot | zoneA_tok | _tok` ⇒ **0 matches**. The bake path
  never touches the snapshot route; `GetReadOnlySnapshot` is unreferenced by any new code.
- `SiegeAssistantComponent/Snapshot/Vocabulary` + `Tests/SiegeAssistantZoneATest.cpp`: **ABSENT
  from my diff** (git status confirms; the only source files changed are the three in §1).
- ⛔ No token figure anywhere in code, comments, or this handoff (`AS-§12g`).
- `SC-§33`: zero defaulted parameters added to any function, new or existing;
  `HeightToBrightness` is exactly the pinned 3-param signature, `public`, plain static, not a
  `UFUNCTION`.
- **📌 M8 declaration: this task adds NO replicated property, NO new class (the one new type-level
  addition is a static function on the existing non-reflected projection struct), NO RPC, and NO
  new relevancy tier. Every byte of the elevation layer is client-local display; nothing it
  computes leaves the widget.**
- The `WR-§6` click→symbol law untouched: **the hit-test path (`NativeOnMouseButtonDown`, the
  marker-rect builder, `FindMarkerIndexAtLocal`) has a 0-line diff** — verified by grep over the
  diff hunks.

## 8. QA scrutiny list (where I'd look hardest)

1. **The object-type-query claim** (§5.2) — the feature stands on "object queries ignore channel
   responses"; the source argument is in the bake's block comment. If QA disputes the engine
   semantics, the fallback is an `ECC_Visibility` channel trace (hills Block it, proxies
   deliberately don't) at the cost of a per-pawn-mesh leak class — I chose the shape with no
   ignore list to rot.
2. **`IsVisible()` as the proxy filter** — rejects `SetVisibility(false)` components (the trunk
   proxies). A HIDDEN-actor edge (e.g. a destroyed castle via `SetActorHiddenInGame`) may still
   report component-visible; consequence is a white castle texel on a dead castle until re-bake —
   cosmetic, match-rare, and self-corrects on Play Again.
3. **Engine API spellings** (no compile allowed here): `UTexture2D::CreateTransient` /
   `GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE)` / `LineTraceMultiByObjectType` /
   `FSlateBrush::SetResourceObject` — all standard 5.x idioms, includes added by name
   (`CollisionQueryParams.h`, `Engine/HitResult.h`, `Engine/Texture2D.h`, `TextureResource.h`,
   `Components/PrimitiveComponent.h`).
4. **The sentinel iterators** (`AGoldNode` → `AAncientGround`) — confirm the identity-only reading
   satisfies `WR-§6`'s no-place-iterator law as argued at `FindScatterGenerationSentinel`.
5. **sRGB=true on the transient texture** — the luma byte is authored perceptually (like any UI
   PNG), so sRGB decode gives a perceptually-even ramp on screen. If Jonathan wants more contrast
   in the low band, `ElevationFloorLuma` (named constant) and `ElevationReliefCeiling` (tunable)
   are the two knobs.
6. **Line-number drift for TASK-691/580 readers**: my diff moves `WarMapWidget.cpp` anchors
   (the old `:524` no-snapshot line and `:871-873` case now sit ~90 lines lower). The strings
   themselves are byte-untouched; locate by symbol per `SC-§18c`. The stale §3/§3b snapshot
   narration TASK-691 flagged is deliberately NOT touched here — the manager split that rider to
   TASK-685/580 by file ownership, and 685 is the next writer of this file pair.
