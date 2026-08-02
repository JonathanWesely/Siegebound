# TASK-359 — [AG-T1] NEW `AAncientGround` actor — programmer handoff

Status: **ready-for-qa** (files only; **NOT compiled** — TASK-366 compiles the batch). Implemented against the board spec `#### TASK-359`, `CONVENTIONS.md` §2 of "Ancient Grounds + Sorcerer + 180° terrain symmetry (2026-08-01)", and plan §2 (`C:\Users\wesel\.claude\plans\there-is-one-new-glittery-bentley.md` — the plan wins over board summaries). No compile, no Git, no editor, no MCP.

---

## ✅ READ THIS FIRST — THE SCOPE DEVIATION I FLAGGED IS **RULED AND AUTHORIZED**

**ORCHESTRATOR RULING (2026-08-01, verbatim intent — QA: this is NOT scope creep, it is authorized):**
> *"**Keep the comment-only `CaptureZone.h` edit.** You were right to surface the conflict rather than silently pick a side. The 'new files only' line in my dispatch and the both-headers debt note required by CONVENTIONS §2 and the task spec are genuinely contradictory, and a comment block with zero symbols and zero compile surface is the cheaper violation. `CaptureZone.h` is owned by no other task in this batch, so there is no collision risk. I'll tell QA it was authorized."*

**The conflict, for the record:** the dispatch said *"new files only — `AncientGround.{h,cpp}`"* and the board's `names:` field lists `CaptureZone.{h,cpp}` as a **read-only donor** — but the same spec (board §1), CONVENTIONS §2, and the dispatch's own point 2 all say **"record the 2-mirror as debt in BOTH headers."** Those cannot both be satisfied.

**What the edit is** — two doc-comment blocks in `CaptureZone.h`, **zero symbols, zero code, zero behavior, zero compile surface**, revert-safe with two deletions:
1. A `🔧 KNOWN DEBT — 2-MIRROR WITH AAncientGround` block appended to the class doc (why the duplication is load-bearing; ⚠️ do not "de-duplicate" via a base class).
2. A `⚠️ PAIRED TUNABLE with AAncientGround::ZoneHalfExtent` line on `ACaptureZone::ZoneHalfExtent`.

**Why it does not violate the parallel-file law:** the "new files only" limit exists to stop parallel code owners clobbering each other (M8 PARALLEL LAW). `CaptureZone.{h,cpp}` is owned by **no** task in this batch — 358/361 own `BattlefieldScatter`+`ScatterConfig`, 360 owns `SummonedUnit`+new `SorcererUnit`, 362 owns `HealthBarProvider`/`CombatantHealthBar*`, 363 owns `SiegeCheatManager`, 364 owns `DeckBuilderWidget`. No collision risk. **Status: RESOLVED — keep as-is.**

---

## 🔄 POST-SUBMIT AMENDMENT (2026-08-01) — TASK-374 carry-forward: decal `SortOrder`

TASK-374 finished authoring `M_AncientGround` and surfaced that **`SortOrder` cannot be set on the material**: UE 5.8's `UMaterial` exposes **no** SortOrder/SortPriority field (the artist dumped the full property list to confirm rather than assume). It is a **`UDecalComponent`** property, so it can only be set in this actor's C++ — and since these grounds are spawned procedurally there is no level actor for anyone to hand-tune either.

**This was already handled in my original submission** (`ApplyDecalFootprint()` set `GroundDecal->SortOrder = DecalSortOrder;`, default 10) — I had reached the same conclusion independently and flagged it as a judgement call in the QA-scrutiny list below. The amendment **converts the direct member write to the explicit engine setter** so it is grep-visible as `SetSortOrder` and idiomatic:

```cpp
GroundDecal->SetFadeScreenSize(DecalFadeScreenSize);   // 0.001
GroundDecal->SetSortOrder(DecalSortOrder);             // 10
```
Functionally identical (`UDecalComponent::SetSortOrder` is `SortOrder = Value; MarkRenderStateDirty();`, `DecalComponent.cpp:362`) — the setters just mark the render state dirty individually instead of relying on the single trailing `MarkRenderStateDirty()` that the `DecalSize` write still needs. `ApplyDecalFootprint()` runs at most 3 times per actor across 2 actors, so the extra dirty calls are free. I also expanded the in-code comment to record **TASK-374's finding, the Z-fight it prevents, and the verification**.

**Verification (repo-wide, run after the edit):** `grep -rn "SortOrder" Source/GitClaudeUnrealTest/Siegebound/` returns hits in `AncientGround.{h,cpp}` **only** — confirming `ACaptureZone` leaves `SortOrder` at the engine default 0, so any positive value wins. 10 leaves headroom for a future decal to slot between them.

**Also confirmed by TASK-374's handoff, no change needed:** my `DecalSize (1024, 840, 840)` and `FadeScreenSize 0.001` are correct and were sanity-checked against the material's UV-space shaping. The material is authored and saved at `/Game/Materials/M_AncientGround.M_AncientGround` — the soft path in my constructor now resolves for real.

**File-ownership discipline held:** this amendment touched `AncientGround.cpp` only. `BattlefieldScatter.{h,cpp}` and `ScatterConfig.h` are TASK-361's (running concurrently) and were not read-modified or written.

---

## Files touched

| File | Kind | Lines |
|---|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/AncientGround.h` | **NEW** | 218 |
| `Source/GitClaudeUnrealTest/Siegebound/AncientGround.cpp` | **NEW** (incl. the `SetSortOrder` amendment) | 262 |
| `Source/GitClaudeUnrealTest/Siegebound/CaptureZone.h` | edited — **COMMENT-ONLY, ORCHESTRATOR-AUTHORIZED** (see the ruling above) | +19 |

**Assets referenced:** `/Game/Materials/M_AncientGround.M_AncientGround` — soft path only, null-safe. **Now authored and saved by TASK-374**, so it resolves for real.

---

## ⚖️ NET RELEVANCY TIER: **C — NOT REPLICATED**

Declared in the `AAncientGround` class doc **and** here, per the M8 NET RELEVANCY LAW declaration duty.

`bReplicates` **stays at the `AActor` default (false) and is never written** — I deliberately did not write `bReplicates = false;` in the constructor, because a redundant assignment invites a future reader to "fix" it by flipping it; the constructor carries a Tier-C comment instead.

**Rationale:** the actor holds **no replicated truth**. Both machines construct an identical pair *locally* from `ASiegeBattlefieldScatter`'s already-Tier-A replicated `ChosenSeed` / `GenerationIndex` (the `AGoldNode` / mine-pair precedent). The state it *produces* — `PermanentDamageStacks` — lives on `ASummonedUnit` and replicates with the unit fleet in M8 P2 (TASK-360 records that duty). It also has **no collision and no nav geometry** by construction: `USceneComponent` root + `UDecalComponent`, and `UDecalComponent` derives from `USceneComponent`, **not** `UPrimitiveComponent` (verified against installed UE 5.8, `Classes/Components/DecalComponent.h:24`) — so it cannot touch the traversability guarantee.

---

## ⚠️ AUTHORITY IS PUSHED, NEVER READ — the invariant, and how to check it in 3 seconds

```
$ grep -c HasAuthority Source/GitClaudeUnrealTest/Siegebound/AncientGround.cpp
0
```

`AncientGround.cpp` contains **zero occurrences** of `HasAuthority` — not a call, not a comment, not a string. The **only** occurrence in the pair is `AncientGround.h:65`, inside the class doc block that *explains the ban* (the law scopes the prohibition to the `.cpp`; the header needs the term to be greppable as documentation).

**The mechanism:**
- `void InitAncientGround(bool bAuthoritative)` — pinned signature, character-for-character — stores `bAuthoritativeBoost`. It is the **only** writer of that flag.
- `ApplyBoostTick()` gates on `if (!bAuthoritativeBoost) { return; }` and nothing else.
- Default is **`false` — fail-CLOSED**: an un-`Init`'d ground never boosts.
- TASK-361 calls `InitAncientGround(bAuthoritativeGenerate)`, threading the flag `RunScatterPasses` already carries (`BattlefieldScatter.cpp:275` true / `:303` false). Same shape as `RunScatterPasses(Seed, bAuthoritativeGenerate)`.

**🔎 QA/PIE diagnostic I added on purpose** — one grep-able `Log` line per ground (2 per match), because this is the feature's most dangerous spot and it should be *observable*, not merely argued:
```
[AncientGround_0] AncientGroundInit authoritativeBoost=false P=(-12000, 6000) halfExtent=(840, 840)
```
**On a CLIENT both grounds MUST print `authoritativeBoost=false`.** If a client ever prints `true`, the regression is in the scatter's threading (TASK-361), not in this actor's gate — the log localizes the fault for free.

---

## The boost tick — exactly as specced

1 Hz **timer**, never per-tick (TASK-004 law). `PrimaryActorTick.bCanEverTick = false`. **Zero overlap events** — occupancy is the same 2D box sweep `ACaptureZone` uses, so no collision primitive exists at all.

One `TActorIterator<ASummonedUnit>` sweep, two team buckets (`[0]` Blue, `[1]` Red via a file-local `TeamBucketIndex(ETeamId)`):
- skip `!IsValid` / `IsUnitDead()`; skip `!IsPointInZone(...)`;
- `IsAncientGroundEmpowerer()` ⇒ `++SorcererCount[team]` and **`continue`** — **a sorcerer is a SOURCE, never a sink; it never self-boosts and never boosts a fellow sorcerer**;
- else `CanReceiveDamageBoost()` is required to become an occupant.

Second pass: `AddPermanentDamageStacks(SorcererCount[its OWN team])` — **FRIENDLY-ONLY** (Jonathan ruling iii): a contested ground empowers **both** sides simultaneously through their own sorcerers. **Per-sorcerer stacking** (ruling iv): 2 friendly sorcerers ⇒ 2 stacks that tick.

**Zero grant ⇒ no call.** Two guards, belt and braces: an early `return` after pass 1 when *both* buckets are 0 (the common case — a ground with no sorcerer on it never even enters pass 2), plus a per-occupant `if (Grant > 0)`. Neither path can emit a spurious `FOnCombatantDamageBoostChanged` broadcast.

Buildings / towers / castles / gold nodes and `AHeroCharacter` are **excluded by type** (not `ASummonedUnit`) — the boost is a unit-fleet mechanic. `AMinerUnit` is an `ASummonedUnit` subclass so it *is* swept, and is then excluded by `CanReceiveDamageBoost()` (Support profile) — correct, and it keeps its boost bar row hidden.

### Pinned cross-task signatures consumed — called, never defined
`IsAncientGroundEmpowerer()` · `CanReceiveDamageBoost()` · `AddPermanentDamageStacks(int32)` — all three from CONVENTIONS §7, authored concurrently by TASK-360. **None of them exists yet**, so this file cannot compile until TASK-360 lands; that is expected and is why TASK-366 compiles the batch as a unit. **QA should diff my call sites against TASK-360's declarations character-for-character**, including that all three land in `ASummonedUnit`'s **`public:`** block (manager ruling 2 — `protected:` would fail to link, since I call them from outside the class).

Verified as already public on `ASummonedUnit`: `IsUnitDead()` (`SummonedUnit.h:306`) and `GetTeamId()` (`:126`).

---

## Shape / decal

- Plain `AActor`. `SceneRoot` (`USceneComponent`) + `GroundDecal` (`UDecalComponent`, relative pitch **−90** ⇒ faces −Z). No collision primitive.
- `FVector2D ZoneHalfExtent = (840, 840)` — **PAIRED TUNABLE**, cross-noted in both headers.
- `bool IsPointInZone(const FVector&) const` — 2D XY box, **Z ignored**; byte-copy of `CaptureZone.cpp:112` so "standing in the zone" reads identically for both zone actors.
- `float BoostTickInterval = 1.0f`; `EndPlay` clears the timer.
- `DecalSize = (DecalProjectionDepth 1024, ZoneHalfExtent.X 840, ZoneHalfExtent.Y 840)` — **`.Y`/`.Z` are the `ZoneHalfExtent` VERBATIM**; the decal matches the mechanic box and is commented never to be shrunk for looks (shrinking it would show the player a lie about where the boost applies).
- **`FadeScreenSize = 0.001`** set explicitly (the engine default 0.01 culls the decal at this arena's zoom-out).
- **`SetSortOrder(10)`** on the component. ✅ **Resolved, no longer a judgement call:** CONVENTIONS §2 states `SortOrder 10` under the *Material* bullet, but sort order is a **`UDecalComponent`** property, not a material property (verified UE 5.8 `DecalComponent.h:39`) — **independently confirmed by TASK-374**, which dumped `UMaterial`'s full property list and found no SortOrder/SortPriority field. These grounds are spawned procedurally, so C++ is the only place it can live. `ACaptureZone` leaves `SortOrder` at 0 (module-wide grep: no other hit), so 10 gives the intended "runes draw above the capture-zone tint where the footprints meet near the centerline" and prevents a Z-fight exactly where a sorcerer is most likely to be played. Exposed as `EditDefaultsOnly int32 DecalSortOrder`.
- **No MID, deliberately.** The ground is team-neutral — nothing drives a colour param at runtime, so `M_AncientGround`'s authored `GroundColor` default (jade) *is* the shipped look. Creating a MID would be dead code. (This is the one place I did **not** mirror `ACaptureZone`, which needs a MID for its owner tint.)
- **Null-safe:** missing material ⇒ **logged exactly once** (`bWarnedMissingMaterial`), no visual, **the mechanic still runs**.
- `ApplyDecalFootprint()` is called from the **constructor + `OnConstruction` + `BeginPlay`** (the `ACaptureZone` idiom) so instance edits take; one `MarkRenderStateDirty()` per call.

---

## Things QA should scrutinise (I am flagging them so nothing is a surprise)

1. ~~**THE SCOPE DEVIATION** — the comment-only `CaptureZone.h` edit.~~ ✅ **RULED AND AUTHORIZED by the orchestrator — keep it, do not write it up as scope creep.** See the ruling quoted at the top.
2. **THE TIMER IS ARMED UNCONDITIONALLY IN `BeginPlay`, and that is deliberate — this is the subtlest decision in the file.** Arming it *from* the authority flag would couple correctness to **spawn order**: a plain `World->SpawnActor<AAncientGround>()` runs `BeginPlay` **inside** the spawn call, i.e. **before** the scatter gets the pointer back to call `InitAncientGround` — so the flag is still `false` at `BeginPlay` **on the server too**, and a conditional arm would silently produce a ground that never boosts *anywhere*. Gating the **tick body** instead is order-independent and fail-closed. The cost on a non-authoritative machine is one predicted branch per second across exactly two actors. It also matches the law's literal wording: *"the tick gates on the STORED `bAuthoritativeBoost` flag ONLY."*
3. ~~**`SortOrder` placement** — component, not material.~~ ✅ **CONFIRMED by TASK-374** (`UMaterial` has no such field in UE 5.8) and converted to the explicit `SetSortOrder(10)` setter — see the amendment section.
4. **Log category is `LogGitClaudeUnrealTest`, not `LogSiege<Domain>`.** Deliberate: it exactly mirrors `ACaptureZone`, this class's acknowledged 2-mirror donor, so the two zone actors report material misses on one channel, and it avoids including `BattlefieldScatter.h` purely to borrow `LogSiegeTerrain`. If QA prefers a `LogSiege*` category, say which one and it is a one-line change — but note that changing it *only here* would split the pair.
5. **`BoostTickInterval` carries no `GDD §x.x` reference, on purpose.** The spec's `// GDD §x.x` is a placeholder — I checked `Docs/GDD.md` and **there is no ancient-grounds section** (the feature is a 2026-08-01 Jonathan directive, not a GDD entry). Writing a fake section number would violate the TRUTH LAW. The comment instead names it a MECHANIC RULE (UPROPERTY default, never a cards.csv column) and points at CONVENTIONS §2 as the rule's home of record. **Not a missing citation — an honest one.**

## Things that are correct by ABSENCE — please do not file these

- **No `Reset...()` and no `SiegeGameMode` edit.** The ground **latches no state** — no owner, no progress, no accumulator — so there is literally nothing to reset. Play Again step 7 already calls `ClearScatter()` + `GenerateScatter()`, which destroys this pair and re-places a fresh one for free. (`ACaptureZone` needs `ResetCaptureZone()` precisely because it *does* latch `CaptureOwner`.)
- **Does not derive from `ACaptureZone`** — load-bearing, argued in both headers.
- **No `GetLifetimeReplicatedProps`, no `OnRep_`, no `bReplicates`** — Tier C.
- **No spawn/placement code** — `PlaceAncientGrounds` + the `USiegeScatterConfig` fields are TASK-361, which is blocked on this task for the `AAncientGround` type.
- **No `SummonedUnit.{h,cpp}` / `BattlefieldScatter` / `ScatterConfig.h` edits** — those belong to TASK-360 / 358 / 361.

## Downstream contracts this task hands off

- **TASK-361** spawns `AAncientGround` and **must** call `InitAncientGround(bAuthoritativeGenerate)` on **each** of the two actors. Skipping it ⇒ the ground silently never boosts (fail-closed) — the `AncientGroundInit` log line makes that visible immediately.
- **TASK-374** authors `/Game/Materials/M_AncientGround` at that exact path. Missing ⇒ one warning, no visual, mechanic intact.
- **TASK-377 (human PIE gate):** grep `AncientGroundInit` on both machines — the client must show `authoritativeBoost=false` on both grounds.
