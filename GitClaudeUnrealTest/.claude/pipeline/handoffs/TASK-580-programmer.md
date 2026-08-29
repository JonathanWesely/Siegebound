# TASK-580 — THE REAL SNAPSHOT SEED: one at-rest `Capture(World, Team)` per match

- **Author:** gameplay-programmer · 2026-08-29
- **Spec:** the amended `(A0)..(A8)` block on TASKBOARD (the ONLY live instruction), aimed by `handoffs/TASK-691-programmer.md` (the premise inversion: BeginPlay's `EnsureSnapshot()` allocates but never surveys — a merely-allocated snapshot reproduces VID-003 byte-for-byte).
- **Files touched (the `W691-4` fence, exactly):** `Source/GitClaudeUnrealTest/Siegebound/SiegeAssistantComponent.h` · `SiegeAssistantComponent.cpp`. Nothing else. Diff: **+174 / −2** across the pair; zero non-comment lines removed (the −2 are the two rewritten comment lines: the `:899` note and the `Snapshot` member one-liner).
- ⛔ No compile (TASK-689 owns), no editor, no Git, no board writes beyond the status flip, no console sentence anywhere in this session (the 552 latch law untouched), no `Content/`, no `.csv`, no `Tests/`.

---

## 1. THE DIFF, functionally

| site | what |
|---|---|
| `USiegeAssistantComponent::BeginPlay()` (end of function) | Arms `SetTimerForNextTick(this, &USiegeAssistantComponent::TrySeedSnapshotAtRest)` — the seed is **scheduled**, never captured inline (the (A2) race, see §2). |
| `USiegeAssistantComponent::EndPlay()` (top) | `ClearTimer(SnapshotSeedTimerHandle)` — no pending fire into a dying component. Safe when never armed / already fired. |
| **NEW** `void USiegeAssistantComponent::TrySeedSnapshotAtRest()` (private; body placed directly below `CaptureTurnSnapshot()`, the `DebugCaptureAndComposePrompt` one-screen idiom) | The whole deliverable: runs SubmitUtterance's **STEP 7 AND NOTHING ELSE** — no BeginTurn, no TurnId, no compose, no dispatch, no SetState, no PushMessage. Gates then delegates to `CaptureTurnSnapshot()` → `Snapshot->Capture(World, OrderingTeam)` — the ONE shipped caller chain, so the whole Capture payload (`PlaceNames` / `PlaceLocations` / `PlaceHalfExtents` / `RegionPlaceNames`, roster tallies riding along) is filled by the REAL survey. ⛔ Zero hand-populated fields. |
| **NEW** members (header, session-state region) | `FTimerHandle SnapshotSeedTimerHandle` · `bool bSnapshotSeedRetryUsed = false` |
| **NEW** constant (cpp internal namespace) | `SnapshotSeedRetryDelaySeconds = 1.0f` (deliberately not a tunable — rationale in its comment) |
| Comment riders (see §6) | the `:899` header note + the `Snapshot` member one-liner |

**Entry point per (A1):** `CaptureTurnSnapshot()` reused **by symbol**, under the `DebugCaptureAndComposePrompt` at-rest whitelist discipline (its cpp block is the "⚠️ A WHITELIST, NOT A BLACKLIST" comment — Idle/Composing/Failed only, refuse-by-default). ⛔ Not from the widget, not on the map-open path — the seed lives on the component's own lifecycle; the widget pair has a 0-line diff from me (§4 grep).

## 2. (A2) THE ORDERING PROOF — seed demonstrably AFTER scatter/world population, cited at source

**The population side is synchronous inside the scatter's BeginPlay:**
1. `ASiegeBattlefieldScatter::BeginPlay()` (`BattlefieldScatter.cpp:202`) → authority branch calls `GenerateScatter()` (`:230-233`).
2. `GenerateScatter()` (`:259`) → `RunScatterPasses(Seed, /*bAuthoritativeGenerate*/ true)` (`:322`, body `:363`).
3. `RunScatterPasses` places everything inline: mines via `World->SpawnActor<AGoldNode>` (primary `:1624`, twin `:1632`) and `PlaceAncientGrounds(Seed, …)` (`:433` → `:1667`, deterministic-fallback guarantee `:1856` — "the objective never ships short"). **No async, no timer, no latent action on the placement path** ⇒ when `ASiegeBattlefieldScatter::BeginPlay` returns, every `AGoldNode`/`AAncientGround` of the match exists.

**The seed side is strictly after ALL BeginPlays:**
4. Every level actor's BeginPlay (the scatter's included) is dispatched inside the world's begin-play routing (engine: `UWorld::BeginPlay` → game mode `StartPlay`/`HandleMatchHasStarted` → `AWorldSettings::NotifyBeginPlay()`, which iterates all actors and calls `DispatchBeginPlay`) — this completes **before the first world tick**.
5. `FTimerManager::Tick` runs inside `UWorld::Tick`. A timer armed with `SetTimerForNextTick` during ANY BeginPlay therefore fires on the **first world tick — strictly after step 4 completes**, i.e. after the scatter's synchronous generation, regardless of the (unguaranteed) BeginPlay dispatch order between the scatter actor and this component. That unguaranteed order is exactly why the capture is NOT taken inline in BeginPlay.
6. If the owning controller is instead spawned after world begin play (travel/late join), the level-placed scatter began play at world start — the ordering holds a fortiori.

**Client caveat, closed by the gates:** on a non-authority machine the scatter waits for `OnRep_GenerationIndex` (`:234-239`) with no tick bound — but the seed **refuses on non-authority** (§3), so no client seed can ever race replication. This mirrors the shipped lane: `SubmitUtterance` gate 3 refuses clients too, so no capture of any kind ever runs there today.

**No `SiegeGameMode` hook was needed** — the component-internal shape sufficed; ⛔ no SC-§15 surface amendment taken. (No scatter-completion broadcast exists to subscribe to — verified: zero multicast delegates in `BattlefieldScatter.h`.)

## 3. (A3) THE FSM TABLE — refuse-by-default, one retry, never a loop, never a crash

The function can run at most **twice** per component lifetime (BeginPlay's next-tick arm + the single retry). Every path below logs at most once by construction; no `Warning` on any designed path; no per-frame anything; the component still never ticks.

| condition at fire time | verdict | why |
|---|---|---|
| `TurnId > 0` (a real sentence beat the seed) | **skip silently** (Verbose log) | the sentence's capture is fresher than any seed; re-seeding buys nothing |
| no authority (`GetOwner()` null or `!HasAuthority()`) | **stand down immediately, no retry** (one Log) | permanent condition — mirrors SubmitUtterance gate 3; retrying cannot change a machine's authority |
| FSM ∉ {Idle, Composing, Failed} | **retry once** at +1.0 s, else stand down (one Log) | the at-rest whitelist, verbatim from `DebugCaptureAndComposePrompt`; ⛔ short-circuit order (`bAtRest && CaptureTurnSnapshot()`) guarantees no capture is ever forced through a busy FSM |
| `CaptureTurnSnapshot()` false (no world / `ResolveOrderingTeam` refuses) | **retry once** at +1.0 s, else stand down (one Log) | the transient match-start races (PlayerState seating is login-time — `SiegeGameMode.cpp` `InitNewPlayer` tags Blue at login, so this should never actually fire on tick 1; the retry is belt) |
| all gates pass | **capture** + one Log line stating `%d places resolved (%d region-bearing)` | the acceptance instrument for QA and TASK-690's eye |

⛔ `bAssistantFaulted` is deliberately NOT a gate — same as `DebugCaptureAndComposePrompt` (which gates only empty-utterance/authority/FSM): the fault latch disables the **console**, and the seed feeds the display path only; a dead model must not also cost the player the map's markers.

## 4. (A7) THE AIRLOCK PROOF — pasted, not asserted

**Zero prompt characters move.** `TrySeedSnapshotAtRest` calls `CaptureTurnSnapshot()` and nothing else — no `GetCachedZoneA`, no `BuildZoneA/B/C`, no `ComposeTurnPrompt`, no grammar, no vocabulary touch, no `.csv`. `Capture()` fills snapshot STATE (verified at `SiegeAssistantSnapshot.cpp:286-299`: `ResetSnapshot()` first, then actor passes — zone text is built only later, at compose time, which the seed never reaches). `ReportFirstCapture` fires only inside `ComposeTurnPrompt` — the first-execution audit latch is untouched.

**The seed is unreachable from any map/widget path** — tree-wide grep, every hit:
```
$ grep -rn "TrySeedSnapshotAtRest" Source/ Content/ Tools/
SiegeAssistantComponent.cpp:644:   this, &USiegeAssistantComponent::TrySeedSnapshotAtRest);      # BeginPlay arm
SiegeAssistantComponent.cpp:3291:  void USiegeAssistantComponent::TrySeedSnapshotAtRest()        # definition
SiegeAssistantComponent.cpp:3363:  &USiegeAssistantComponent::TrySeedSnapshotAtRest,             # the single retry arm
SiegeAssistantComponent.h:906/:1444/:1598                                                        # 2 comments + the private declaration
```
6 hits, all in the owned pair; the function is `private`, non-UFUNCTION, non-exec — no external caller can exist.

**The widget path invokes nothing** — every `Capture(`/`EnsureSnapshot` hit in `WarMapWidget.{h,cpp}` is comment prose (16 hits, 0 invocations; grep run 2026-08-29 against the working tree that includes TASK-684's in-flight edit).

**ZoneA byte-frozen:**
```
$ git status --porcelain -- Source/.../Tests/SiegeAssistantZoneATest.cpp
(no output — byte-untouched)
```
(`Tests/SiegeWarMapTest.cpp` IS dirty in the tree — that is **TASK-684's owned extension**, not mine; my diff contains no `Tests/` path.)

**The not-touched fences hold:** `git status --porcelain` on `SiegeAssistantSnapshot.{h,cpp}` · `SiegeAssistantVocabulary.{h,cpp}` · `SiegeAssistantConsoleWidget.{h,cpp}` · `SiegeGameMode.{h,cpp}` → **no output**. The only dirty files from this task are the owned pair.

⛔ No token figure quoted or derived anywhere (`AS-§12g`).

**Per-match re-seed, verified at source (one line + one declared caveat):** the component re-`BeginPlay`s per world and `Capture()` `Reset()`s first thing (`SiegeAssistantSnapshot.cpp:293`) ⇒ a new world always gets a fresh seed, no cross-match staleness. ⚠️ **DECLARED CAVEAT (SC-§15 finding, reported not repaired):** `PlayAgain` is an **in-place reset that never re-runs BeginPlay** (`SiegeGameMode.cpp:1200`, its own comment) yet DOES re-scatter (`:1078-1083`) — so a PlayAgain match keeps this seed's survey (mines/grounds at their PREVIOUS scatter positions on the map) until the player's first sentence re-captures. Same self-healing class as `W691-1`; fixing it would need a game-mode hook or broadcast — outside the preferred component-internal shape and outside the A-block's "one capture per match at match start" deliverable. Flagged for the manager to board or accept; documented in the BeginPlay comment.

## 5. (A4) THE RESOLVED-PLACES LIST (`W691-2` acceptance basis) — derived at source for L_Arena at seed time

I cannot compile or run (fences), so this is the source derivation the seed log will confirm live (the log prints the counted truth):

| place | resolves at first-tick seed? | source basis |
|---|---|---|
| `own_castle` / `enemy_castle` | **yes** | level-placed `ACastle`; `SiegeGameMode.cpp:110` — "every ACastle exists by BeginPlay" (warning branch `:122` otherwise); pass 2, `SiegeAssistantSnapshot.cpp:384-399` |
| `ancient_ground_near` / `ancient_ground_far` | **yes** (both, distinct) | two `AAncientGround` spawned synchronously pre-tick-1 (§2); rotational twins ⇒ near ≠ far (pass 3, `:401-437`); region-bearing (default `ZoneHalfExtent` (840,840) > 0) |
| `nearest_mine` | **yes** | `AGoldNode::FindBestMineFor` over the synchronously-spawned mines (pass 4, `:443-447`); none depleted at tick 1 |
| `hero` | **yes** | hero spawned/possessed at login (before world begin-play completes); alive at tick 1 ⇒ pass 1 anchors it (`:362-372`; un-possessed covered by the fallback iterator `:351-359`) |
| `mid` | **yes** | `ACaptureZone` — ONE level instance, `CaptureZone_Center` at origin (pass 5, `:449-467`); region-bearing |

**Expected seed log in L_Arena: `7 places resolved (3 region-bearing)`** — counted against resolved slots, ⛔ never asserted as a literal 7 (a world without a capture zone legitimately lacks `mid`; "empty is a LEGAL state", `SiegeAssistantSnapshot.h:862`). The log line IS the paste-point for the live list at TASK-690.

## 6. (A5) STALENESS + (A6) THE RIDERS

- **(A5) accepted + documented, not repaired (`W691-1`):** `hero` and `nearest_mine` markers freeze at seed-time positions until the first sentence re-captures. ⛔ No periodic re-capture added (CONVENTIONS §4's once-per-sentence law stands — the seed is ONE extra at-rest survey per match, not a cadence). Clicking stays CORRECT: a click inserts the SYMBOL; the executor resolves at sentence time. Stated in the seed's success log for TASK-690's sheet.
- **(A6) the `:899` rider, done:** `SiegeAssistantComponent.h` `GetTurnSnapshot()` doc — "⚠️ Null before the first capture" (false since TASK-447 `cd5f4ed`) rewritten to the verified story: non-null from match start, field-empty until first `Capture()`, seeded by this task, readers keep their null checks. Same-file sweep for further "null before first sentence / turn path" narration: **zero other hits** (grep `[Nn]ull (before|until)|first sentence|turn path` — only `:899` matched; the `:709` hit is console-listener prose, unrelated). The `Snapshot` member one-liner (`:1558`) amended to name the seed so it cannot go stale the same way. ⛔ `WarMapWidget`'s stale narration (`:59-60`, `:524-527`, `:962-968`, header `:267-298`) is TASK-685's rider block — untouched by me, and its `.h:278` "reached only from the turn path" line is now doubly stale for 685 to take.

## 7. (A8) M8 + SUITE

- **M8 DECLARED:** no replicated property, no new class, no new tier, no RPC. The seed is a client-local survey on the owning machine's component (and by the authority gate it only ever runs where the shipped capture lane already ran).
- **No new unit test owed** (the A-block's own ruling — a world-population race cannot be asserted headless; the instruments are §2's source-order proof + TASK-690's eye, the WAVE-2 precedent). **Suite expectation: 142 — UNCHANGED from TASK-684's declaration** (`handoffs/TASK-684-programmer.md`: 140 → 142; my delta is 0).
- `SC-§33` discharged: the new function takes ZERO parameters; no defaulted parameter added to anything; the engine `SetTimer` call passes `bLoop` explicitly.

## 8. FOR QA (TASK-688's fused gate — `handoffs/TASK-691-programmer.md` is a gate input beside this)

1. The (A0) test: the seed is a REAL `Capture()` through the one shipped caller — confirm no hand-populated snapshot field anywhere in the diff (there is none).
2. The short-circuit order in `bAtRest && CaptureTurnSnapshot()` — load-bearing for "never force a capture through a busy FSM"; flag if anything reorders it.
3. `SetTimerForNextTick` returning into `SnapshotSeedTimerHandle`, and the same handle reused for the single retry — intended (the next-tick fire has completed by then); EndPlay clears whichever is pending.
4. The `TurnId > 0` moot-gate reads state only; verify my claim that `BeginTurn()` (which increments it) precedes the shipped lane's capture (SubmitUtterance steps 6→7).
5. The PlayAgain caveat in §4 — a finding for the manager, not a defect in this diff.

## Status
- TASKBOARD: TASK-580 → **ready-for-qa** (gate = TASK-688 fused).
