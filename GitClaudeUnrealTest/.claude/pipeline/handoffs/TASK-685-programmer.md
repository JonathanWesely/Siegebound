# TASK-685 — [WM-A3] THE POI ICON LAYER + UNIT-DOT LEGIBILITY + THE W691 RIDERS — programmer handoff (2026-08-29)

Built ON TASK-684's landed working-tree diff (the declared base — no 684 line was reverted or
reworked). File-only per the fences: ⛔ no compile (TASK-689 owns it), no editor, no Git
writes, no board writes beyond the status flip, ⛔ no console sentence (the 552 latch law
untouched, latch UNSPENT).

## 1. Files touched (the whole 685 diff — nothing else)

| File | What |
|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/WarMapWidget.h` | 3 icon soft-ptr tunables + `AllyDotRadius` + POI internals (2 functions, 3 transient textures, 3 brushes, 4 census arrays, 1 log latch) + the §3/§3b/latch stale-narration comment corrections |
| `Source/GitClaudeUnrealTest/Siegebound/WarMapWidget.cpp` | the POI census + resolve + paint layer, the ally-dot size promotion, BOTH discriminator re-points (`W691-3`), the `:59-60`/`:524-527`/latch-proof narration corrections, the honest empty-state string |
| `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeWarMapTest.cpp` | EXTENDED (never a new file): test 26 appended |

⚠️ The working tree also carries **TASK-580's `SiegeAssistantComponent.{h,cpp}`** (the parallel
lane per `W691-4`) plus 683/687's content — none of it is mine; `git status` over `Source/`
shows exactly 580's pair + my three files, and I never opened 580's pair for write.

## 2. The POI icon layer (spec items 1–2; `WM-§1`)

**Layer/paint-order proof:** `NativePaint`'s layer block now reads
`ElevationLayer = MaxLayer+1 · PoiIconLayer = MaxLayer+2 · Ally +3 · Enemy +4 · Marker +5 ·
Label +6`; return stays `Max(MaxLayer, LabelLayer)`. The icons fill EXACTLY the slot 684's
comment reserved (between elevation and ally dots); every shipped layer above the slot moved up
one, ORDER PRESERVED — the 684 precedent repeated. The seven `ResolvePlace` markers stay the
topmost solids, so a marker draws ON TOP of any coincident POI icon (spec item 3), and nothing
painted on `PoiIconLayer` exists to the hit test.

**Icon-source census (which actors, which filters, which accessors):**

| POI | Iterator | Filter | Tint |
|---|---|---|---|
| Mines | `TActorIterator<AGoldNode>` | `IsValid` + **`!IsDepleted()`** (diagnosed at source: the one-way depletion latch `GoldNode.h:334`; a gold-less mine is not a POI and its dimmed glow already says so in-world) | `SiegeWarMap::MineIconTint` (named constant, gold) |
| **J4 exclusion** | — | `ADeepMine : public ABuilding` (`DeepMine.h:44`) — NOT an `AGoldNode` subclass, so the iterator excludes player-built DeepMines **by construction**, no filter to rot | — |
| Ancient grounds | `TActorIterator<AAncientGround>` | `IsValid` (team-neutral fixture, no state) | `SiegeWarMap::AncientGroundIconTint` (named constant, pale verdant) |
| Castles | `TActorIterator<ACastle>` | `IsValid` + `!IsCastleDestroyed()` (§6 deviation 1) — split by **`GetTeamId()`** into two arrays (`ETeamId` is exactly Blue/Red, `TeamId.h:14-18` — the else-arm is exhaustive) | **`W4-R5` accessors ONLY**: the paint reuses the already-fetched `AllyColor`/`EnemyColor` locals, which ARE `UCombatantHealthBarComponent::GetDefaultBlueBarColor()`/`GetDefaultRedBarColor()`, keyed by the CASTLE'S OWN team. Zero re-typed literals (grep: no new team-color literal anywhere in the diff) |

Census runs in `RefreshPoiIcons()` — seeded in `OpenMap` beside `RefreshAllyDots()`, refreshed
on the SAME shipped timer (`HandleMapRefreshTimer`, now one timer / FOUR jobs — comment updated),
cleared in `CloseMap`/`NativeConstruct`/`NativeDestruct` beside the dot arrays. Nothing iterates
while closed; nothing per-frame.

**Projection:** every icon centre goes through the SHIPPED
`WorldToMapUV → MapUVToLocal` chain — byte-the-same two calls the dots make; ⛔ zero new
transform arithmetic anywhere in the diff (the 684 `MapUVToWorld` inverse is untouched and unused
by this layer — icons only ever go world→screen).

**Textures + null-safety (the ATorch two-null-paths idiom, per asset):** three
`EditDefaultsOnly TSoftObjectPtr<UTexture2D>` members defaulting (constructor, never
`NativeConstruct` — the `ArenaConfigAsset` stomp argument) to the exact TASK-683 paths
`/Game/UI/WarMap/T_WarMap_Icon_{Mine,AncientGround,Castle}` in composed object-path form.
`FSoftObjectPath` arguments are BRACE-initialised (MOST-VEXING-PARSE law). Resolve is lazy on
`OpenMap` (`LoadSynchronous` — a load, so ⛔ never on the const paint path), idempotent, via ONE
namespace helper `SiegeWarMap::ResolvePoiIconTexture`:
CLEARED ⇒ silent opt-out; SET-but-unresolvable ⇒ ONE latched Log line naming which of the three
failed (`bWarnedPoiIconUnresolved`). EITHER null path degrades at paint to the SHIPPED dot
primitive (`PaintQuad`) in that POI's tint at the same footprint — drawn, never skipped, never a
crash, never silence. GC: resolved textures live in `UPROPERTY(Transient) TObjectPtr` hard refs
(the `ElevationTexture` idiom); the three `FSlateBrush` members only mirror them and stay
white-tinted — the per-element tint at draw is the only colorist (the textures are pure white on
every texel, 683's authored law — tint multiplies with zero fringe).

**Icon size:** `PoiIconDrawHalfSizePx = 12.f` (24 px — the exact size 683's glyphs were
legibility-gated at; bigger than a dot, smaller than a marker's 36-px hit rect).

## 3. ⛔ Icons are NOT clickable — the hit-test zero-diff, grep-PROVEN

- **`FSiegeWarMapProjection::FindMarkerIndexAtLocal` and `UWarMapWidget::BuildMarkerRects`
  byte-compare IDENTICAL to HEAD** (awk-extracted bodies, `diff -q`: `IDENTICAL`, 21 and 50
  lines) — a 0-line diff across BOTH 684 and 685, since 684 also declared 0.
- **`NativeOnMouseButtonDown` carries exactly ONE hunk**, and it is the miss-arm status
  discriminator — **rider (6)(a)'s explicitly ordered edit** (`W691-3` names the `:876`-era
  site by symbol). The click mechanics — the open guard, the absorb-all-buttons rule, the
  left-button filter, `LocalPoint`, the `BuildMarkerRects` call, `FindMarkerIndexAtLocal`, the
  hit arm (broadcast / status echo / log / `Handled`) — are all unchanged context outside the
  hunk. ⚖️ Reconciliation stated plainly: spec item (3)'s "byte-untouched" and rider (6)(a)
  collide inside this one function; the rider is the later, explicit, site-naming instruction,
  so the HIT-TEST MECHANICS are held to the 0-diff standard and the miss-arm CHROME takes the
  rider's edit. The `WR-§6` click→symbol law and the seven-marker clickable set stand untouched.

## 4. Unit-dot legibility (spec item 4; `WM-§3`)

- Ally-dot size located at source: the file-local `SiegeWarMap::DotDrawHalfSizePx = 3.f`
  literal, shared by ally + enemy loops. **Promoted** the ally side to
  `EditDefaultsOnly float AllyDotRadius` (Clamp 1..32), **bumped 3 → 5** (10-px square) for
  contrast against 684's dark elevation ground. Enemy dots deliberately keep the shipped
  literal (§6 deviation 3). ⛔ Dot COLOR routing untouched (`AllyColor`/`EnemyColor` accessor
  reads are byte-identical); ⛔ the refresh timer untouched (same handle, same interval, same
  enable/disable sites — `RefreshPoiIcons` was added INSIDE the existing callback).
- **NULL-snapshot render verification (the TASK-579 pattern, one line each, MEASURED not
  asserted):** grep over each function body for `GetReadOnlySnapshot|GetTurnSnapshot|Snapshot->`:
  - DOTS: `RefreshAllyDots` = 0 snapshot symbols (and its body is unchanged); ally/enemy paint loops = 0.
  - ICONS: `RefreshPoiIcons` = 0, `ResolvePoiIconTextures` = 0, the `NativePaint` POI block = 0.
  - REVEAL: `HandleRevealButtonClicked` / `ReceiveEnemyReveal` / `ClearEnemyReveal` = 0, all three byte-untouched.
  Only marker geometry (`BuildMarkerRects`, unchanged) and the status-line discriminators read
  the snapshot — nothing became snapshot-gated by this edit.

## 5. Item (6) — the riders, executed

**(a) The `W691-3` discriminator RE-POINT (both sites located BY SYMBOL, `SC-§18c` —
the `ee4aecd` `:413`/`:876` anchors sit ~90 lines lower post-684):**

| Site | Before | After |
|---|---|---|
| `OpenMap` status branch | `if (GetReadOnlySnapshot() == nullptr)` → hint, else `"War map"` | `StatusSnapshot == nullptr \|\| StatusSnapshot->GetPlaceNames().Num() == 0` → hint, else `"War map"` |
| `NativeOnMouseButtonDown` miss arm | `if (GetReadOnlySnapshot() == nullptr)` → hint, else the empty-click line | same null-OR-empty test → hint, else the empty-click line |
| `UpdateNoSnapshotHint` retire | retire when snapshot non-null | retire when snapshot non-null AND `GetPlaceNames().Num() > 0` |

- `GetPlaceNames()` is the exact list `BuildMarkerRects` consumes ⇒ the status line and the
  marker layer can no longer disagree; *"Click a marked place"* can never again render over a
  marker-less map. Null short-circuits into the same arm; the null-guards STAY as defensive code.
- **Honest empty-state string (declared, exact, 147 chars):**
  `No places surveyed yet - send your commander one order in the console and the markers appear. The console opens over this map; no need to close it.`
  (names the cause — the ruling's own words — and the remedy; pure ASCII; no
  snapshot/capture/assistant/turn vocabulary; inside the ~150-char wrap contract 687 styled for).
- **The latch (stated per the rider):** it now watches the transition **"zero places listed →
  at least one place listed"**, and it stays one-way/monotonic-in-practice: within an open,
  `PlaceNames` is filled only by `Capture()`'s resolved-slot loop over structural fixtures
  (castles/grounds/hero), so some→zero has no shipped route; if a future regression invented
  one, the latch fails SAFE — it only ever RETIRES the hint (the `SetStatusLine` one-writer-
  per-direction invariant is untouched), and the next `OpenMap` re-evaluates from scratch. Zero
  per-frame work added: the bool early-out is unchanged; the new cost is one `Num()` on ticks
  where the hint is still up. ✅ Correct with or without 580: pre-seed the honest line shows
  (true today); post-seed the at-rest capture lists places and the line stays away.
- ⛔ NOT re-pointed to `Markers.Num()`: a degenerate-panel frame would then claim "no places
  surveyed" — the wrong cause with the wrong remedy (TASK-579's original argument, kept).

**(b) Stale-narration corrections (comment-only, every site the rider names + the same lie
where it lives in the header I own):**
- `.cpp` `NoSnapshotStatusText` doc block (the `:59-60`-era story): "allocated on its turn
  path / does not exist until the first sentence" → the BeginPlay/`cd5f4ed` truth; the four
  string-decisions (1)–(4) kept verbatim (still true).
- `.cpp` `GetReadOnlySnapshot` latched log (the `:524-527`-era text): now says the snapshot is
  BeginPlay-allocated and a null means the READ ROUTE failed (no controller/component), points
  at `handoffs/TASK-691-programmer.md`. Latch (`bWarnedNoSnapshot`) unchanged.
- `.cpp` `UpdateNoSnapshotHint` latch-proof (the `:962-968`-era paragraph): the old
  "monotonic null→non-null" proof replaced with the corrected story (that transition happens at
  match start, watched by a latch that could never arm) + the new transition's argument.
- `.h` §3 residual, §3b, the latch bullet, `BuildMarkerRects`'s empty-cases line,
  `HandleMapRefreshTimer`'s job list — same correction, comment-only (§6 deviation 4).
- `NoSnapshotStatusText`/`ShowNoSnapshotHint`/`UpdateNoSnapshotHint`/`bShowingNoSnapshotHint`
  NAMES KEPT (the spec's "else keep the name and fix its comment" branch — §6 deviation 2).
- ⛔ `SiegeAssistantComponent.h:899` NOT touched — TASK-580's rider, its file, its lane.

## 6. Deviations of record (`SC-§15`) — six, each argued at source

1. **Destroyed castles are skipped from the icon census** (`IsCastleDestroyed()`). The spec's
   skip clause named only depleted mines; this is the symmetric diagnose-at-source call —
   `DestroyCastle` HIDES the actor (`Castle.h:843`), and "what the eye cannot see, the map must
   not claim" is the bake's own doctrine. One-line revert if ruled the other way.
2. **The `NoSnapshotStatusText` rename NOT taken** though permitted-if-trivial: its three
   sibling identifiers were not in the rename grant, and renaming one of four would read worse
   than the historical family name with corrected comments.
3. **Enemy dots keep the 3-px literal** while `AllyDotRadius` ships at 5. `WM-§3` scopes the
   legibility bump to Jonathan's blue dots; the reveal lane (`WR-§7`) is not this wave's to
   restyle. Making them ride a tunable is one line if Jonathan asks.
4. **Header-side narration corrected too** (rider (b) named `.cpp` sites): `W691-3` says the
   stale narration "is corrected wherever it lives", and `WarMapWidget.h` is mine by the
   `W691-4` file-ownership rule. Comment-only, zero code effect.
5. **The icon fallback footprint** is `PoiIconDrawHalfSizePx` (24 px), not the 6-px dot size —
   "the existing dot primitive" (`PaintQuad`) at glyph footprint, so icon-vs-fallback frames
   read identically in layout and TASK-690's pixel eye compares like for like.
6. **One new test added** although spec item (5) waives tests for the actor ITERATION: the
   dispatch pins "icon-placement projection cases on the same lattice idiom 684 used", and
   test 26 is geometry-only — no world, no iterator, no census assertion (the WAVE-2 boundary
   respected inside the test's own comment).

## 7. Tests + suite expectation

`Tests/SiegeWarMapTest.cpp` EXTENDED with **1** case (file 25 → 26 macros):

- **26 `Siegebound.WarMap.PoiIconProjectionStaysInsideTheMapRect`** — the icon layer's
  load-bearing geometry claim on 684's `(i+0.5)/N` lattice idiom: a 13×6 lattice over a world
  span **1.5×** each arena half-extent (outer ring engages the clamp on every edge) projects
  through the SHIPPED `WorldToMapUV → MapUVToLocal` chain into a `ComputeMapRectLocal` rect
  (1920×1080, shipped padding) and lands INSIDE it, boundary-inclusive, on the clean AND
  shipped-CDO arenas; plus closed-form spot pins — world origin ⇒ rect centre,
  `(-HalfX, +HalfY)` ⇒ rect top-left (the pinned Y flip), a 3×/4× out-of-arena point ⇒ pins
  EXACTLY to the bottom-right corner (edge-pin, not ejection).

**Suite expectation for TASK-689: 142 → 143** (measured from source after the edit:
`IMPLEMENT_SIMPLE_AUTOMATION_TEST` census across `Source/.../Tests/` = 143; this file 25 → 26;
684's declared 142 baseline re-measured and confirmed before my +1). Zero network, zero world,
zero Slate in the new test.

## 8. Airlock re-statement + M8 (spec item 5, the 684 items-(6) shape)

- **Invocations of `Capture(` / `EnsureSnapshot` / `GetTurnSnapshot` in added CODE lines: 0.**
  The added-lines census over the three-file diff finds 11 matches, every one a `//`/`*` comment
  or the corrected log's `TEXT()` prose — which is the rider's own deliverable (narration that
  corrects a false story about those functions must name them). No new call site of any of the
  three exists; `GetReadOnlySnapshot()` is called at exactly the three sites that already
  called it (the two discriminators + the retire check).
- `zoneA_tok` / `_tok` / any token figure in added lines: **0** (`AS-§12g`).
- `SiegeAssistantComponent/Snapshot/Vocabulary` + `Tests/SiegeAssistantZoneATest.cpp`: **absent
  from my diff** (the component pair IS dirty in the tree — that is TASK-580's parallel lane,
  never opened by me for write).
- `SC-§33`: zero defaulted parameters on any function, new or existing (`PaintPoiIcon`,
  `ResolvePoiIconTexture`, `ResolvePoiIconTextures`, `RefreshPoiIcons` — all fully spelled).
- 📌 **M8 declaration: this task adds NO replicated property, NO new class, NO RPC and NO new
  relevancy tier. The POI census, the icon layer, the dot bump and the discriminator re-point
  are all client-local display; nothing computed here leaves the widget** (the only outbound
  channel remains `OnPlacePicked`'s symbol, untouched).

## 9. QA scrutiny list (where I would look hardest)

1. **The dual-instruction reconciliation (§3)** — confirm you accept "hit-test mechanics 0-diff
   + rider-ordered miss-arm edit" as satisfying both spec item (3) and rider (6)(a); the grep
   artifacts are in §3.
2. **`IsDepleted`/`IsCastleDestroyed` census filters** — both are my diagnose-at-source calls
   (the spec pinned only "skip depleted if the class exposes it"); deviation 1 carries the
   castle argument.
3. **Castle tint reuse of the `AllyColor`/`EnemyColor` locals** — they ARE the `W4-R5`
   accessor values, reused keyed by castle team (comment at the loop states it); check you
   agree this satisfies "accessors only" without a second pair of accessor calls.
4. **A Red-team player sees his own castle RED** (absolute team tint per J2, while his units
   are blue) — designed per the law as written; belongs on Jonathan's sheet if it reads odd.
5. **Engine API spellings** (no compile here): `TSoftObjectPtr<UTexture2D>::LoadSynchronous()
   const` (the `ArenaConfigAsset` precedent in this same file), `UTexture2D::GetSizeX/Y`,
   `FSlateBrush::SetResourceObject` — all shipped idioms in this file pair already.
6. **The latch's theoretical some→zero edge** (§5) — argued monotonic-in-practice + fail-safe;
   if QA wants belt-and-braces, re-arming on `OpenMap` already covers every next open.
7. **`LoadSynchronous` on the open path** — up to three tiny UI textures once per widget
   lifetime, beside a 7,800-trace bake; declared, not hidden.

Status flip: TASKBOARD TASK-685 → **ready-for-qa** (688's fused gate over 684+685+580 takes it
from here, with `handoffs/TASK-691-programmer.md` as a gate input).
