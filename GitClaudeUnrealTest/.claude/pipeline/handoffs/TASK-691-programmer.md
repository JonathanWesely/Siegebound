# TASK-691 — DIAGNOSE-FIRST: why VID-003 showed MARKERS = 0 with the WITH-snapshot status branch live

- **Author:** gameplay-programmer · 2026-08-29
- **Mode:** READ-ONLY discharged as specced — zero source edits, zero engine calls, zero console sentences, zero gold. No remote-exec lane was needed: the session log covering the exact VID-003 window already exists on disk and decides the question (spec item 2's "one log pull"), so no node was touched at all.
- **Anchor law:** every `WarMapWidget` / `SiegeAssistant*` line number below is the **`ee4aecd` committed state**, read via `git show ee4aecd:GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/<file>` — ⛔ never the working tree (TASK-684 is concurrently editing `WarMapWidget.{h,cpp}`). Current HEAD `8c5f8af` is TASKBOARD-only on top of `ee4aecd`, so code-at-HEAD ≡ code-at-`ee4aecd`, and the live binary is the `ee4aecd` build — runtime observations match these anchors.

---

## 1. VERDICT — mechanism (a), refined: the snapshot genuinely held zero place markers, because it was the **BeginPlay-allocated, never-`Capture()`d snapshot**

The with-snapshot status branch does NOT mean "a sentence was sent." It means only "the assistant component finished `BeginPlay()`." The chain, every link at source:

1. **`USiegeAssistantComponent::BeginPlay()` allocates the snapshot at match start** — `EnsureSnapshot()` at `SiegeAssistantComponent.cpp:574` (inside `BeginPlay`, `:552`), which is `if (!Snapshot) Snapshot = NewObject<USiegeAssistantSnapshot>(this);` (`:3215-3224`). The pointer is non-null from match start, forever (never re-nulled; monotonic).
2. **`GetTurnSnapshot()` returns that member raw** (`SiegeAssistantComponent.h:903`) → `UWarMapWidget::GetReadOnlySnapshot()` (`WarMapWidget.cpp:494-531`) is **non-null for the entire match**, sentence or no sentence.
3. So `OpenMap()`'s discriminator (`WarMapWidget.cpp:413-420`) takes the `else` arm → **"War map"**, and the empty-click discriminator (`:876-883`) takes the `else` arm → **"Click a marked place to add its name to the console."** — exactly VID-003's two observed strings, with no sentence ever sent.
4. **A never-captured snapshot has empty place data.** `PlaceNames`/`PlaceLocations`/`PlaceHalfExtents`/`RegionPlaceNames` have exactly ONE append site — `Capture()`'s resolved-slot loop (`SiegeAssistantSnapshot.cpp:485-516`) — and `Reset()` wipes them first thing in every `Capture()` (`SiegeAssistantSnapshot.h:724`). A fresh `NewObject` snapshot is field-empty by construction.
5. `BuildMarkerRects()` (`WarMapWidget.cpp:667-716`) iterates `Snapshot->GetPlaceNames()` — zero elements → **zero iterations → zero markers**. The `:871-873` documented case, entered by the allocation-without-capture route.
6. **`Capture()` is reachable through exactly three call sites**, none of which is BeginPlay or map-open: `SubmitUtterance` (`SiegeAssistantComponent.cpp:841` — a typed sentence), `PollDeferredIntent` (`:2572` — requires a prior deferred order, i.e. a sentence), `DebugCaptureAndComposePrompt` (`:3342` — the at-rest diagnostic exec, `:3278`). None ran in the VID-003 session (§2).

**Mechanism (b) — projection/filter drops at paint time — KILLED:** `BuildMarkerRects` never received a name to project, and the panel was not degenerate: the lone ally dot painted, and `NativePaint` early-returns before painting ANY dot when `RectSize ≤ 0` (`:749-752`) using the same `ComputeMapRectLocal` on the same geometry the marker builder uses.

**Mechanism (c) — a pre-clip console sentence entered the with-snapshot branch — KILLED by the log, not merely unproven:** the clip's session contains zero console/turn traffic end to end (§2). The half of (c) that survives, corrected: the with-snapshot branch was indeed entered "by design pre-580" — but the entry mechanism is the **BeginPlay allocation**, not any sentence.

### ⭐ The stale-premise finding underneath it
`WarMapWidget.cpp`'s own story — `:59-60` "*ResolvePlace lives on a snapshot the assistant allocates on its turn path, so it does not exist until the player's first sentence*", the `:524-527` log text, and `GetTurnSnapshot()`'s header note "*⚠️ Null before the first capture*" (`SiegeAssistantComponent.h:899`) — **is false at `ee4aecd` and has been false since the assistant first landed**: `git log -S "EnsureSnapshot"` touches exactly ONE commit, `cd5f4ed` (TASK-447), and at `cd5f4ed` `BeginPlay` (line 519 there) already called `EnsureSnapshot()` (line 528 there). ⇒ **The no-snapshot state TASK-560 named and TASK-579 built its hint for is unreachable in any normally-initialized match.** `ShowNoSnapshotHint()`, `UpdateNoSnapshotHint()`'s wait-loop, `NoSnapshotStatusText` (`:87`) and the `:524` log line are live-dead code today. WR-§9 row 12's "self-heals the moment one sentence is sent" was aimed at a state that never occurs — what actually ships on every first open is the `:871-873` empty-with-snapshot state.

---

## 2. THE LOG EVIDENCE — the analyst's one-log-pull, answered from disk

**Session log:** `Saved/Logs/GitClaudeUnrealTest-backup-2026.08.29-19.38.54.log` — `Log file open, 08/29/26 12:37:39`, `Log file closed, 08/29/26 12:38:54` (local; in-log stamps are UTC `19.3x`). The VID-003 clip (recording 12:38:23 → 12:38:49) sits **entirely inside** this 75-second session — and this is the WHOLE session. ⭐ VID-003's "mid-session, a pre-recording sentence cannot be excluded from pixels" caveat is hereby CLOSED: the un-recorded 44 s before the clip contain only engine load and the walk-in; the full session transcript is on disk.

| line | stamp | evidence |
|---|---|---|
| 1928 | `19.37.57:607` | `USiegeAssistantComponent ready on 'SiegePlayerController_0' (state Idle). zoneA_chars=5658 …` — `BeginPlay` ran ⇒ **snapshot allocated ~43 s before the map open**. |
| 2025 | `19.38.38:409` | `[WarMap] Created (class 'WBP_WarMap_C', ZOrder 4), closed.` — widget lazily created ≈2 s before the observed 17.2 s open (≈12:38:40). |
| — | — | **`[WarMap] No assistant snapshot yet` (`WarMapWidget.cpp:524`): ABSENT** while the map demonstrably opened ⇒ by the spec's own disambiguation: **the snapshot-present-but-empty case is LIVE.** (Per §1 it is also the only reachable case — the `:524` line cannot fire after BeginPlay.) |
| — | — | Total `LogSiegeAssistant` lines in the session: **3**, all from `BeginPlay` (`:1927` vocab CDO fallback · `:1928` ready · `:1929` static-prefix submit). **Zero** `[AssistantConsole]` lines, zero `SubmitUtterance`/capture/`zoneB_chars` traffic, zero exec diagnostics ⇒ **no console sentence and no capture of any kind existed in this session.** |

---

## 3. ROUTE ENUMERATION (spec item 1 — every route to `Markers.Num()==0` under the with-snapshot status branch)

| candidate | verdict | evidence |
|---|---|---|
| Snapshot null, but "the one-way latch already spent earlier in the session" masks it | **KILLED** | Both discriminators test `GetReadOnlySnapshot() == nullptr` LIVE at each event (`:413`, `:876`). `bWarnedNoSnapshot` (`:516-518`) gates only the log line; `bShowingNoSnapshotHint` only retires the hint text. No latch can make a null snapshot render "War map". |
| Snapshot present; `ResolvePlace` returns unset/zero for all seven symbols | **structurally IMPOSSIBLE for a captured snapshot; VACUOUS for the actual one** | `PlaceNames` admits only slots with `bSlotResolved` (`SiegeAssistantSnapshot.cpp:485-491`, one append site, parallel arrays), so `ResolvePlace` on any listed name succeeds by construction (`:734-747`). On the never-captured snapshot the marker loop body never executes at all. |
| Marker-rect build rejects all rects (degenerate panel) | **KILLED by pixels** | The lone ally dot painted; `NativePaint` returns before ANY dot when `RectSize ≤ 0` (`:749-752`), same `ComputeMapRectLocal`, same geometry as `BuildMarkerRects` (`:683-688`). |
| **Snapshot present but NEVER captured — `PlaceNames` empty (the BeginPlay allocation)** | **✅ CONFIRMED — the mechanism** | §1 source chain + §2 log. |

---

## 4. THE PRESCRIPTION FOR TASK-580 (input to the manager's amendment — ⛔ not authorization)

1. **The as-specced premise is inverted, and WM-§5's own fear is the shipped baseline.** 580 was framed as "the snapshot seed" against an absent snapshot; the snapshot is never absent. **The forbidden thing — "an empty non-null snapshot … a lie you can click" — is what EVERY match starts with today**, courtesy of `BeginPlay`'s `EnsureSnapshot()`. ⇒ Allocation is not the gap and must not be the deliverable. **580's diff must guarantee that one REAL `Capture(World, Team)` — a genuine at-rest survey — runs per match before/independent of the first map open.** A seed that only ensures the object exists reproduces VID-003 byte-for-byte.
2. **Which data:** the entire Capture-filled payload — `PlaceNames` / `PlaceLocations` / `PlaceHalfExtents` / `RegionPlaceNames` (roster tallies ride along and are harmless). ⛔ Never hand-populate fields; run the real survey (WM-§5's "REAL at-rest survey" clause is confirmed exactly right).
3. **Which entry point:** reuse `CaptureTurnSnapshot()` (`SiegeAssistantComponent.cpp:3226-3248`) under the at-rest whitelist discipline `DebugCaptureAndComposePrompt` already models (`:3319-3340` — Idle/Composing/Failed only, refuse-by-default). At any sane seed moment the FSM is `Idle`. ⛔ NOT from the widget and NOT on the map-open path — `WR-§6`'s "opening this panel surveys nothing" stands untouched; the seed belongs to the component's own lifecycle.
4. **Which refresh points / the two timing hazards the amendment must rule on:**
   - **Seed timing vs world population:** `BeginPlay` itself may be too EARLY — the scatter (mines, hills) populates at match start, and a capture racing it under-resolves places, yielding a partial marker set that looks like a half-broken map. The seed needs a demonstrably-after-population moment (deferred one-tick/short-timer, or a match-start broadcast); the amendment should pin the mechanism and require 580's handoff to prove ordering against scatter completion at source.
   - **Two of the seven places MOVE** (`hero` — every frame; `nearest_mine` — "the best gold mine for the player now"; vocabulary table `SiegeAssistantSnapshot.cpp:88-97`): a once-per-match seed freezes their markers at seed-time positions until the first sentence re-captures. Clicking stays CORRECT (a click inserts the SYMBOL; the executor resolves against the sentence-time capture) but the drawn position goes stale. Options: accept + document on Jonathan's sheet (identical in kind to the inter-sentence staleness the snapshot already carries), or sparse at-rest re-captures — ⚠️ the latter collides with CONVENTIONS §4 "once per sentence, never per tick" and needs an explicit manager ruling if chosen. Default recommendation: accept + document.
   - **Per-match re-seed is free by construction:** the component re-`BeginPlay`s per world, and `Capture()` `Reset()`s first — no cross-match staleness, provided the seed is on the lifecycle path, not a static/one-shot latch.
5. **Acceptance shape:** the seed guarantees a marker for every place that RESOLVES in the world at seed time — in L_Arena that should be all seven, but a world without a capture zone legitimately lacks `mid` ("empty is a LEGAL state", `SiegeAssistantSnapshot.h:862`). Count against resolving places, ⛔ not a literal 7.
6. **Rider for the amendment (flag, not scope-grab):** after 580 lands, TASK-579's whole no-snapshot lane (`NoSnapshotStatusText` `:87`, `ShowNoSnapshotHint`/`UpdateNoSnapshotHint`, the `:524` log) is doubly dead — it is already unreachable today (§1). Retiring it, or re-pointing the two discriminators (`:413`, `:876`) from "snapshot null" to "no place resolved" so the status line can never again say *"Click a marked place"* over a marker-less map if a future regression empties the seed, is the manager's call — the re-point is the defensive option and would have turned VID-003's confusing chrome into an honest sentence.

---

## 5. SURPRISES
- **TASK-579 shipped a hint for a state that has never existed at runtime.** The BeginPlay allocation predates the entire war-map lane (TASK-447 `cd5f4ed`), so the null-snapshot branch was dead on arrival; every comment asserting "allocated on the turn path / null before first capture" is stale narration, including the one-way-latch proof in `UpdateNoSnapshotHint` (`:962-968`) — the monotonic null→non-null transition it watches for happens at match start, before any open.
- **VID-003's mid-session caveat is fully closed** — the clip's session was 75 seconds long and sentence-free end to end; the full transcript is on disk (§2), so the diagnosis needed no live instrument at all.

## Files touched
- THIS FILE ONLY (`.claude/pipeline/handoffs/TASK-691-programmer.md`). Zero source edits, zero board edits (the spec's ZERO-files-except-the-handoff fence), zero engine/MCP calls, zero console sentences, zero gold. Status flip to `done-pending-manager` left to the orchestrator per the dispatch's "(orchestrator routes)".
