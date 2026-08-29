# QA Report — TASK-688 (the WAR-MAP wave's fused gate: TASK-684 + TASK-685 + TASK-580)
Verdict: **PASS** — per-task: **684 PASS · 685 PASS · 580 PASS**
Blockers 0 · Warns 3 · Nits 3 — qa-reviewer, 2026-08-29

Inputs consumed: TASKBOARD specs (684/685/580-(A)/688) · CONVENTIONS `WM-§1..§6` incl. BOTH
`WM-§5` finalizations (`W691-1..5`) · `handoffs/TASK-684/685/580/691-programmer.md` (691 = gate
input) · the full diffs: `WarMapWidget.{h,cpp}` (684+685 combined), `Tests/SiegeWarMapTest.cpp`,
`SiegeAssistantComponent.{h,cpp}` (580) · source cite-checks in `BattlefieldScatter.cpp`,
`GoldNode.h`, `Castle.{h,cpp}`, `TeamId.h`, `SiegeAssistantSnapshot.h`.
Method note: this gate has no git lane; HEAD-vs-tree byte claims (hit-test 0-diff, ZoneA freeze)
were checked by content-read + suite-census corroboration, not an independent `git diff` (WARN-3).

---

## 1. TASK-684 — the elevation bake — PASS

- **Bake design vs `WM-§2`, recomputed:** 130×60 = 7,800 texel-CENTRE traces, lazy on `OpenMap`
  (`bElevationBakeFailedThisOpen` reset → `EnsureElevationBake`), cached behind world-key +
  sentinel; steady state on the 0.25 s timer = two pointer checks
  (`ElevationTexture != nullptr && ElevationBakedWorld.Get() == World` + `IsValid` on the
  sentinel); `NativePaint` only reads the cached brush ⇒ **zero per-frame — cost claims TRUE at
  the code**. Grid clamps belt-and-braces (2..1024) under the UPROPERTY clamps. Sampled rect from
  `ResolveArenaHalfExtent()` (asset→CDO); zero hand-typed dimensions (`SC-§34` ✓).
- **⚖️ THE LOAD-BEARING DEVIATION RULED — object-type query vs the spec's literal channel trace:
  the deviation is REQUIRED, and I verified it at `BattlefieldScatter.cpp`, not from the
  handoff.** `ResolveComponentForMesh` (:970-981): scatter statics get
  `SetCollisionObjectType(ECC_WorldStatic)` + `SetCollisionResponseToAllChannels(ECR_Ignore)`
  + Block on Pawn/Team/Visibility/Camera ONLY — and the in-source comment (:965-966) says
  outright that the WorldStatic RESPONSE stays Ignore **so `GroundZAt`'s WorldStatic channel
  trace passes through the hills**. A literal `LineTrace…ByChannel(ECC_WorldStatic)` therefore
  sees the flat floor + castles = the uniform-gray ground-only bake `WM-§2` itself refuses.
  The hills ARE WorldStatic OBJECTS, and object-type queries filter on object type, not channel
  responses (standard engine semantics) ⇒ floor + hills + rocks + castles + walls in;
  Pawn/WorldDynamic movers out by construction, no ignore list to rot. **Deviation ACCEPTED as
  the only implementation that delivers the ruling's WHY; the fallback (`ECC_Visibility`
  channel) was rightly rejected (per-pawn-mesh leak class).**
- **`bTraceComplex=false` vs "complex" — RATIFIED:** the shipped `GroundZAt` idiom (±50,000
  bracket confirmed at `BattlefieldScatter.cpp:1249-1250`, copied not invented); hull-truth
  holds by authored law (hill hulls hull_count=1 with hull ZMax == mesh ZMax per the
  climbable-geometry gate; the castle's 61 hulls are its collision truth). Per-poly buys nothing.
- **`IsVisible()` proxy rejection — REQUIRED, verified:** the invisible tree-trunk proxies are
  ALSO `SetCollisionObjectType(ECC_WorldStatic)` (`BattlefieldScatter.cpp:1094`) with
  `SetVisibility(false)` — without the filter the heightmap grows a white spike per tree.
  The highest-VISIBLE-hit max-scan makes no engine-sort assumption. ✓
- **`MapUVToWorld` inverse — REAL and exact:** algebra checked by hand
  (X = (2u−1)·HalfX; Y = (1−2v)·HalfY inverts the shipped forward map exactly, same 1-uu floor,
  same single-owner struct); round trip pinned by test 25 on the bake's own `(i+0.5)/N` lattice,
  both arenas, plus closed-form corners carrying the pinned Y flip, plus the degenerate (0,0)
  extent closing through the shared floor. Deviation 1 ACCEPTED (`SC-§15` declared).
- **The scatter-lifecycle SENTINEL vs `WR-§6` — RULED COMPLIANT:** `FindScatterGenerationSentinel`
  returns actor IDENTITY only; no `GetActorLocation`, no name, no state is read, nothing enters
  a prompt zone; `ResolvePlace` remains the single owner of place→position. The `SC-§20`
  diagnosis (Play Again = in-place reset ⇒ world key alone insufficient; ClearScatter's
  destroy-and-respawn is the observable) is correct and better than the spec's suggested shape.
- **Normalization:** GroundZ = measured min-hit-Z (never a transcribed 0); ramp via the pinned
  pure seam; above-ceiling clamps to exactly 1 (castles/walls = white BY DESIGN, `WM-§6`
  declared). `ElevationReliefCeiling = 1000` derivation checks out (SM_Hill_02 400 uu × DA
  scale max 2.5; < the ±1,200 nav-Z cap). The luma remap (`ElevationFloorLuma` 0.10 → 1.0)
  correctly lives OUTSIDE the seam so the pinned 3-param signature stays exact (`SC-§33` ✓).
- **Layer insertion:** `ElevationLayer = MaxLayer+1`, every shipped layer +1, order preserved,
  return stays `Max(MaxLayer, LabelLayer)`; null texture ⇒ byte-identical pre-684 frame; the
  painter stays `const` and never writes bake state. ✓
- **Engine spellings (pre-compile eye):** `UTexture2D::CreateTransient(X,Y,PF_B8G8R8A8)` ·
  `GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE)`/`Unlock` · `UpdateResource()` ·
  `LineTraceMultiByObjectType(TArray<FHitResult>&, Start, End, FCollisionObjectQueryParams,
  FCollisionQueryParams)` · `FCollisionObjectQueryParams(ECC_WorldStatic)` ·
  `USceneComponent::IsVisible()` · `TBitArray<>(false, N)` + `FBitReference` assignment ·
  `FSlateBrush::SetResourceObject` · `FVector2f` at every Slate boundary — all valid UE 5.x
  idioms; every include named (`CollisionQueryParams.h`, `Engine/HitResult.h`,
  `Engine/Texture2D.h`, `TextureResource.h`, `Components/PrimitiveComponent.h`). No trap found.
- **Failure ladder:** no world / zero hits / alloc failure / null mip ⇒ `false` ⇒ texture
  dropped (a stale bake never stands), ONE latched Log line, one failed attempt per open, later
  opens retry. Never a crash, never Warning spam. ✓

## 2. TASK-685 — POI icons + dot legibility + the W691 riders — PASS

- **Paint-layer arithmetic recomputed:** Elevation +1 · **PoiIcon +2 (the reserved slot)** ·
  Ally +3 · Enemy +4 · Marker +5 · Label +6; return `Max(MaxLayer, LabelLayer)`. No shipped
  layer lost its relative order; the seven markers stay the topmost solids over any coincident
  icon. ✓
- **Projection:** every icon centre goes through the SHIPPED `WorldToMapUV → MapUVToLocal`
  chain — I grepped the whole POI block for new transform arithmetic: **none exists** (the only
  arithmetic is the `PaintQuad`-idiom centre−half-size offset). 684's inverse is unused by this
  layer. ✓
- **⚖️ The hit-test claim + the reconciliation — RATIFIED:** `FindMarkerIndexAtLocal` and
  `BuildMarkerRects` bodies contain no 684/685 symbol and read as the shipped design (the byte-
  compare is the programmer's pasted artifact; see WARN-3 for the residual verification debt).
  `NativeOnMouseButtonDown` carries exactly ONE functional change — the miss-arm
  discriminator — which is rider (6)(a)'s explicitly ordered edit; the open guard, absorb-all,
  left-filter, `BuildMarkerRects` call, `FindMarkerIndexAtLocal`, and the hit arm are unchanged.
  The rider is the later, site-naming instruction; hit-test MECHANICS hold the 0-diff standard.
  `WR-§6` click→symbol stands: nothing on `PoiIconLayer` exists to the hit test.
- **Census filters — RATIFIED at source:** `AGoldNode::IsDepleted()` (one-way latch,
  `GoldNode.h:183-185`, member `:334` — the handoff's cite exact); `ACastle::IsCastleDestroyed()`
  (`Castle.h:176`) with `DestroyCastle` → `SetActorHiddenInGame` verified at `Castle.cpp:1243`
  ("what the eye cannot see, the map must not claim" — deviation 1 ACCEPTED, one-line revert if
  Jonathan rules otherwise); `ADeepMine : ABuilding` excluded by construction (J4);
  `ETeamId = {Blue, Red}` exactly (`TeamId.h:14-18`) ⇒ the else-arm is exhaustive. ✓
- **W4-R5 tint reuse — RATIFIED:** `AllyColor`/`EnemyColor` locals ARE
  `GetDefaultBlueBarColor()`/`GetDefaultRedBarColor()`; castle loops reuse them keyed by the
  castle's OWN team; zero re-typed team literals in the diff (mine/ancient-ground tints are the
  ruled named-constant chrome class). A second accessor-call pair would add nothing.
- **⚖️ The W691-3 re-point, traced in BOTH worlds:** all THREE sites verified in the code —
  `OpenMap` (:668-678), the miss arm (:1650-1660), and the `UpdateNoSnapshotHint` retire
  (:1739-1743) — all test `nullptr || GetPlaceNames().Num() == 0`, null short-circuiting into
  the same arm, null-guards kept as defensive code. **Seed-success world:** places listed ⇒
  "War map", hint never arms, markers on first open. **Seed-failure world (client / FSM-busy
  twice):** the honest empty-state string shows — counted at **exactly 147 chars**, pure ASCII,
  names cause + remedy, no snapshot/capture/assistant/turn vocabulary, inside the ~150-char wrap
  687 styled for. Latch: one-way (retire mirrors arm; `SetStatusLine` single-writer invariant
  untouched; fail-safe on the theoretical some→zero; re-evaluated each open); zero per-frame
  work added (bool early-out first; one `Num()` only while the hint is up). NOT re-pointed to
  `Markers.Num()` — TASK-579's degenerate-panel argument correctly preserved. ✓
- **Snapshot-gating verification (spec item 4):** re-checked — `RefreshAllyDots`,
  `RefreshPoiIcons`, `ResolvePoiIconTextures`, both dot paint loops, the POI paint block, and
  the reveal trio read NO snapshot; only marker geometry + the discriminators do. Nothing became
  snapshot-gated. ✓
- **Textures:** soft refs default in the CONSTRUCTOR (the `ArenaConfigAsset` stomp argument),
  brace-initialised `FSoftObjectPath` (most-vexing-parse law), exact 683 composed object paths,
  `LoadSynchronous` on the open path only (never the const paint), two-null-paths degrade to the
  tinted dot primitive at glyph footprint (deviation 5 — sensible: like-for-like frames for
  690's eye), GC-rooted via `UPROPERTY(Transient) TObjectPtr` with non-reflected brushes
  mirroring (the `ElevationTexture` idiom). ✓
- **All six `SC-§15` deviations ruled:** (1) destroyed-castle skip — ACCEPTED (above);
  (2) `NoSnapshotStatusText` name kept — ACCEPTED (paper-trail greppability, comment corrected);
  (3) enemy dots keep the 3-px literal — ACCEPTED (`WM-§3` scopes the bump to blue;
  `WR-§7` lane not this wave's); (4) header-side narration corrected — ACCEPTED (`W691-3`
  "wherever it lives" + `W691-4` file ownership); (5) fallback at 24 px — ACCEPTED;
  (6) test 26 added despite the iteration waiver — ACCEPTED (geometry-only, no world, no
  iterator; the dispatch pinned projection cases; the WAVE-2 boundary respected).
- **Question (4) — Red player sees his OWN castle RED (absolute team tint) while his units are
  blue:** recorded in the handoff (§9.4) and at the census comment — ⛔ NOT silently shipped —
  but it is NOT yet a row on TASK-690's known-designed-outcomes list. **WARN-1: manager should
  append it to 690's sheet before the playtest** so a report of it doesn't spend a QA loop.

## 3. TASK-580 — the real snapshot seed — PASS

- **(A0) A REAL capture, zero hand-populated fields:** `TrySeedSnapshotAtRest` delegates to
  `CaptureTurnSnapshot()` → `Snapshot->Capture(World, OrderingTeam)` — the ONE shipped caller
  chain; no snapshot field is written anywhere in the diff. ✓
- **The A-block verbatim:** (A1) entry by symbol under the at-rest whitelist ✓; (A2) armed
  `SetTimerForNextTick` at the END of `BeginPlay` — **ordering proof cite-checked at source**:
  `ASiegeBattlefieldScatter::BeginPlay` → `GenerateScatter` → `RunScatterPasses` spawns mines
  (`SpawnActor<AGoldNode>`) and `PlaceAncientGrounds` synchronously, no async/latent on the
  placement path; all actor BeginPlays complete before the first world tick; `FTimerManager::
  Tick` runs inside `UWorld::Tick` ⇒ a next-tick timer fires strictly after world population,
  independent of BeginPlay dispatch order — the exact reason the capture is not inline. Client
  caveat closed by the authority gate (mirrors SubmitUtterance gate 3). No GameMode hook, no
  `SC-§15` surface amendment — correct. ✓
- **The named load-bearing item — the short-circuit order:** `bAtRest && CaptureTurnSnapshot()`
  present exactly as specified (cpp:3344); no capture can be forced through a busy FSM. ✓
- **`TurnId > 0` moot-gate vs `BeginTurn()` — VERIFIED steps 6→7 at source:** SubmitUtterance
  calls `BeginTurn()` at :879 BEFORE `CaptureTurnSnapshot()` at :882, and `BeginTurn` (:2959)
  is "THE ONLY PLACE TurnId MOVES" ⇒ a non-zero TurnId always means a real turn's capture (or a
  refused turn the next sentence heals) superseded the seed. Claim holds. ✓
- **`bAssistantFaulted` deliberately NOT a gate — RATIFIED:** matches the
  `DebugCaptureAndComposePrompt` diagnostic precedent (authority/FSM gates only); the fault
  latch disables the CONSOLE, the seed feeds display state; a dead model must not also cost the
  player the markers. Marker clicks into a faulted console insert text the player cannot send —
  harmless. ✓
- **Timer-handle reuse (their §8.3):** intended and safe — the next-tick fire has completed
  before the retry re-arms the SAME handle from inside the fire; `EndPlay` clears whichever is
  pending, first, before the teardown ladder; `bSnapshotSeedRetryUsed` caps the lifetime at 2
  runs. `SetTimerForNextTick` returning `FTimerHandle` is the correct engine signature. ✓
- **FSM table:** all five rows verified in the body; at most one Log per path, no Warning on
  designed paths, component still never ticks; the retry constant lives inside
  `SiegeAssistantComponentInternal` (cpp:106) so the qualified reference compiles. ✓
- **(A4/W691-2):** the success log prints counted `GetPlaceNames().Num()` /
  `GetRegionPlaceNames().Num()` (both accessors verified to exist) — counted, never a literal 7;
  the L_Arena derivation (7/3) is plausible and the log line is 690's paste-point. ✓
- **(A6):** the `:899`-era header note rewritten to the verified story (now h:901-914), readers
  keep null checks; the `Snapshot` member one-liner names the seed (h:1598). ✓
- **⚖️ W691-5 — RATIFIED as the manager already ruled:** the PlayAgain caveat is declared
  (`SC-§15`, handoff §4 + the BeginPlay comment), accept+document, NO GameMode hook, **no diff
  owed** — "TASK-688 may gate the 580 diff AS-IS" honored; the icons-vs-markers PlayAgain split
  is already a named row on 690's sheet. ✓

## 4. The airlock, re-swept across ALL THREE diffs (my own greps, not the handoffs')

- `WarMapWidget.{h,cpp}`: every `Capture(`/`EnsureSnapshot` hit is comment or log prose; the
  ONLY code invocation in the family is the pre-existing read-only `Assistant->GetTurnSnapshot()`
  (cpp:780). Zero new invocations. ✓
- `SubmitUtterance(`/`AppendToInput(`/`ComposeAppendedInput(`/`InsertText` in the widget diff:
  **0 hits** ⇒ nothing in any diff types or sends a console sentence; the seed is
  `Capture`-only and structurally cannot reach `SubmitUtterance` (private, non-UFUNCTION,
  6 tree hits all in the owned pair). **The 552 latch is untouched and unspent.** ✓
- Token figures: zero in any added line; the only `tok` prose in the component pair is
  pre-existing TASK-455/463 narration carrying no figure; `zoneA_tok` appears once in the
  widget header as the pre-existing "never printed" prose. `AS-§12g` ✓
- `Tests/SiegeAssistantZoneATest.cpp`: 5 macros, consistent with the 143 global census =
  140 + 684's 2 + 685's 1 ⇒ no test-count movement outside the war-map file; byte-freeze
  asserted by 580's pasted `git status` (see WARN-3 for 689's one-line re-check).
- `SC-§33`: zero defaulted parameters added anywhere (`HeightToBrightness` exact pinned 3-param
  signature; `PaintPoiIcon` 7/none; `ResolvePoiIconTexture` 3/none; `TrySeedSnapshotAtRest`
  0-param; `SetTimer` passes `bLoop` explicitly). ✓  M8 declared in all THREE handoffs ✓.

## 5. Cross-diff coherence — the first-open path, traced end-to-end

Tick 0: component `BeginPlay` → `EnsureSnapshot` + seed armed. Tick 1: seed fires → gates pass
on the authority → real `Capture` → places listed, log `7 places resolved (3 region-bearing)`.
Scatter finished synchronously inside its own BeginPlay before tick 1 (both the seed AND 684's
sentinel rely on this — coherent). Player opens the map: bake runs (7,800 traces once), icons
resolve, dots + POI census seed before the first painted frame, discriminator sees
`Num() == 7 > 0` → "War map" — **elevation + icons + dots + markers + labels all present on the
FIRST open with no sentence sent**. On a client / stood-down seed: the honest 147-char line
shows and dots/icons/elevation still render (none snapshot-gated). On Play Again: the sentinel
re-bakes elevation, icons read live actors, markers stay at the previous survey until sentence
#1 — the named `W691-5` designed outcome, already on 690's sheet.

## Findings

- **[WARN-1]** TASK-690 sheet — the absolute-tint fact ("a Red player's OWN castle icon is RED
  while his units are blue", 685 handoff §9.4) is recorded in handoff + code comment but missing
  from 690's known-designed-outcomes rows. Fix: manager appends one row before the playtest.
- **[WARN-2]** `WarMapWidget.cpp` bake cache — a castle destroyed MID-MATCH keeps its full-white
  elevation block for the rest of the match (cached bake; collision-off means a re-bake would
  clear it, but no re-bake fires until Play Again/world change) while its ICON correctly
  disappears — a declared, cosmetic layer disagreement (684 handoff §8.2). Fix: none owed this
  wave; add to 690's sheet alongside WARN-1 so it cannot spend a loop.
- **[WARN-3]** process — the two byte-freeze claims (hit-test bodies identical to HEAD; ZoneA
  test untouched) are programmer-pasted artifacts corroborated here by content-read + the exact
  143 census, but not independently git-diffed (this gate has no git lane). Fix: TASK-689
  eyeballs `git diff` hunk placement for `WarMapWidget.cpp` (exactly one hunk inside
  `NativeOnMouseButtonDown`; zero inside `BuildMarkerRects`/`FindMarkerIndexAtLocal`) and
  confirms `SiegeAssistantZoneATest.cpp` absent from the diff — one minute, before the commit.
- **[NIT-1]** `EnsureElevationBake` unarmed-sentinel branch iterates two actor classes 4×/s on a
  scatter-less field while the map is open — declared, dot-sweep cost class, fine.
- **[NIT-2]** A FAILED re-bake under an open map (Play Again edge) leaves the flat background
  until the next open (`bElevationBakeFailedThisOpen`) — declared policy, degraded-never-broken.
- **[NIT-3]** `AllyColor`/`EnemyColor` local names read relative while the castle usage is
  absolute-team — the loop comment carries the explanation; naming only, no action.

## Notes for build-master (TASK-689 pre-flight)

**Suite expectation: 143** — recounted from source (`IMPLEMENT_SIMPLE_AUTOMATION_TEST` census
across `Source/.../Tests/` = 143; `SiegeWarMapTest.cpp` = 26; chain 140 → 142 (684) → 143 (685);
580 delta 0). New cases: `Siegebound.WarMap.HeightToBrightnessRampFloorCeilingAndClamp` ·
`Siegebound.WarMap.MapUvToWorldInvertsTheProjection` ·
`Siegebound.WarMap.PoiIconProjectionStaysInsideTheMapRect`.

**The ONE commit's cargo (verify then stage together):**
- Code: `Source/.../Siegebound/WarMapWidget.h` · `WarMapWidget.cpp` ·
  `Tests/SiegeWarMapTest.cpp` · `SiegeAssistantComponent.h` · `SiegeAssistantComponent.cpp`
- Art (683): `Content/UI/WarMap/T_WarMap_Icon_Mine.uasset` (sha `4ac8fcdd…`, 15,339 B) ·
  `T_WarMap_Icon_AncientGround.uasset` (`6a11abc6…`, 17,300 B) ·
  `T_WarMap_Icon_Castle.uasset` (`94859080…`, 13,123 B) — ⚠️ **these three arrive AUTO-STAGED
  (`A ` rows, the hostile-index/editor-integration behavior, TASK-568 §6 precedent; the artist
  ran zero git write commands)** — fold them into the one commit, don't be surprised by the index.
- Raws (683): `Content/RawAssets/Textures/WarMap/T_WarMap_Icon_{Mine,AncientGround,Castle}.png`
  (untracked at artist exit).
- WBP (687): `Content/UI/WBP_WarMap.uasset` (property styling only; sha `6827303b…`).
- Pipeline: handoffs 683/684/685/687/580 (+691 if untracked) · this report · board/CONVENTIONS/
  SLACK updates. Message names TASK-683..689 + 580.
- ⚠️ **Foreign dirt to ADJUDICATE, not auto-sweep:** `Config/DefaultEngine.ini` (modified,
  mtime pre-dates 683's editor launch; remote-exec block intact per 687) — exclude from the wave
  commit unless its content is ruled wave-legitimate.
- Editor verify: the 3 textures at `/Game/UI/WarMap/` with SRGB=true, LODGroup=UI, 128×128;
  WBP styling saved. Compile via the CLAUDE.md Build.bat line — **parse the log for `Result:`**
  (the exit-code lie law). QUIET-MODULE confirm before dispatch. Never push.
- WARN-3 discharge: the one-minute `git diff` hunk check described above.

**The live/pixel list (689 machine-verify where a route exists; 690 = Jonathan's eye):**
1. Map open on a fresh match: elevation shading (lighter = higher; castle shells + boundary rim
   FULL WHITE by design) · POI icons at every live mine (gold pickaxe), both ancient grounds
   (verdant ring), both castles (keep glyph, blue/red by the castle's own team) · blue ally dots
   (10 px) · **all seven markers + labels present on the FIRST open, no sentence sent**.
2. The seed log line in L_Arena:
   `Snapshot seeded at rest (TASK-580): 7 places resolved (3 region-bearing)…`.
3. The bake log line: `[WarMap] Elevation baked: 130x60 samples, … clamped full white.` —
   relief ≈ the hill band, clamp count plausible (castles + rim).
4. Legibility on pixels (687's three fixes): status line 22 pt white/outlined, CLEAR of the deck
   bar; "Reveal Enemies (30 Gold)" fits its button; the button pair clear of the Rally HUD.
5. Projection sanity at BOTH castles: each castle ICON coincides with its
   `own_castle`/`enemy_castle` MARKER.
6. Marker click → the SYMBOL lands in the console box; **the player presses Enter himself**
   (the 552 latch: no agent, no automation, NOBODY sends a sentence).
7. Known-not-a-bug rows (read before reporting): hero/nearest_mine markers at match-start
   positions until the first order (`W691-1`); after Play Again the markers reflect the previous
   layout while icons are live-true, healing on sentence #1 (`W691-5`); + the WARN-1/WARN-2 rows
   once the manager adds them.

Status flip (`ready-for-qa` → `qa-passed` ×3) is the orchestrator's — this gate's dispatch fences
board writes.
