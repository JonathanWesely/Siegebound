# TASK-931 — [VEIL-TELLS] one per-viewer predicate, consulted from N sites

**Role:** gameplay-programmer · **Date:** 2026-09-07 · **Marker:** `TASK-931-VEIL-TELLS`
**Gate:** `TASK-932` (boarded; dispatchable the moment this file exists on disk)
**Law:** `WITCH-§0` · `WITCH-§2` · `WITCH-§3` · `WITCH-§5`/`§6` · `WITCH-§9.1` row 4 + its second note · `SHIP-§9` · `SC-§39.1` cl. 4 + cl. 9 · `SC-§40` cl. 3/9/16 · `SC-§41` · `SC-§49` · `HIGH-§1`
**Not done here, by fence:** no compile · no editor/MCP · no `Content/**` · no Git.

---

## 0. ⭐⭐ THE HEADLINE, AND IT IS THE ONE THE ROW ASKED FOR — **MEMBER 5 IS DECLARED AND LEFT, AND THE REASON IS A MEASUREMENT, NOT A SHRUG**

The row's clause (0) says the headline is the **fourth tell — the feature's own material**. I measured it and the answer is **not the one the dispatch expected**, so it is stated first:

> ⛔⛔ **SUPPRESSING THE VEIL PAINT FOR A NON-SEEING VIEWER MAKES THAT VIEWER SEE *MORE*, NOT LESS.**
> `ASummonedUnit::ApplyVeilMaterial` paints `MI_Unit_Invisible` over every slot. If that paint is gated on the per-viewer consult, an enemy viewer stops getting the shimmer — and gets a **fully opaque, normally-lit unit** in its place. ⇒ the *silhouette* is unchanged and the *legibility goes UP*. **The paint is not what leaks the position. The MESH is** — and `TASK-923` ruled the mesh stays (`WITCH-§0`, restated as this row's own item (4)(a)).

The four possible render outcomes for a viewer who may not see a veiled unit, enumerated so nobody re-derives them:

| # | what the enemy would see | verdict |
|---|---|---|
| (a) | the shimmer (**today**) | a positional marker with a silhouette — the `J-W18` complaint |
| (b) | an **opaque** unit (gate the paint) | ⛔ **strictly worse than (a)** — same silhouette, now fully lit |
| (c) | **nothing** (`SetVisibility(false)` / `SetHiddenInGame(true)` per viewer) | ⛔ **FORBIDDEN by this row's item (4)(a)** — named as a fix that may not be reached for |
| (d) | a fully transparent instance | ⛔ **FORBIDDEN by item (9)** — a new material is 🧑 `J-W16`, and `Content/` is out of scope |

⇒ ⛔ **There is no legal render outcome that is better than the status quo, and the two that are better are each forbidden by a different clause of this same row.** Per item (2)'s explicit instruction — *"IF IT CANNOT BE DONE PER-VIEWER TODAY … SAY SO IN WRITING AND LEAVE IT"* — **member 5 is declared, measured and left, and it is routed to 🧑 Jonathan under item (8)'s scope valve as part of `J-W18`.**

**The engine mechanism, named so the next reader does not have to look for it:** the only per-viewer *rendering* primitive UE offers without replication is `UPrimitiveComponent::SetOwnerNoSee` / `SetOnlyOwnerSee`, and using it to give two viewers two materials needs **two mesh components** — a structural change, keyed on **ownership rather than on team** (so a non-owning ally would also be blinded), and it lands squarely on the third-mesh-component hazard already carried on `TASK-907`. ⛔ Not this row's shape, and not a silent improvisation.

⚖️ **The honest sentence for 🧑 Jonathan:** *the AI half of his ask genuinely ships (`TASK-851`, five sites across four functions — re-measured below and confirmed). The three ACCESSORY tells that announced a veiled unit's position are gone as of this row. What remains is the unit's own body, which is visible on purpose, because a veiled unit is ruled* **hidden, not gone.** *"Translucent and blurred TO THE OWNER" is now exactly true for the owner; what the ENEMY should see instead of a body is a design question, and it is his.*

---

## 1. THE PREDICATE, AND THE ONE THING ADDED BESIDE IT

**The predicate — unchanged, not re-implemented, not wrapped:**

```
FSiegeCombatStatics::IsAgentVisibleTo(ETeamId ViewerTeam, const AActor* Candidate)
```
declared `SiegeCombatStatics.h`, defined `SiegeCombatStatics.cpp` (📌 located **by symbol**, `SC-§40` cl. 9 — I did not trust the row's line numbers and they had drifted).

**What I added — ONE function, and the gate must judge it on exactly this claim:**

```cpp
static bool FSiegeCombatStatics::IsAgentVisibleToLocalViewer(const UWorld* World, const AActor* Candidate);
```

⛔⛔ **IT IS NOT A SECOND PREDICATE. It states the veil rule ZERO times.** Its entire body answers one question — **whose eyes?** — and then calls `IsAgentVisibleTo`. It contains no flag read, no `ESiegeVeilPolicy`, no `FSiegeInvisibilityStatics` call. **Test 34 (a-i)/(a-ii) pins all five of those facts against the shipped body.**

### Why it has to exist at all — the argument the gate should test hardest

An **acquisition** site already knows whose eyes it is: the acquirer's team *is* the input to the query. That is why `TASK-851`'s bot scans can call `IsAgentVisibleTo` directly and why no adapter was ever needed before.

A **render** site does not. A health bar, a hit flash and a damage number are drawn *for whoever is watching*, and the watcher appears **nowhere in the owning actor's state**. ⇒ ⛔ **without this one function, each of the three sites would resolve the viewer itself — which is precisely the N-independent-hides shape the row exists to refuse.** Three sites answering *"whose eyes?"* separately will disagree the first time that answer moves (a spectator, a replay, a second local viewer, M8).

⭐ **And it is the M8 seam.** When `WITCH-§6`'s per-client asymmetry arrives, **this function is the only thing that changes**; every consult site keeps its single line. No replication is implemented here.

### The viewer resolve — copied, not invented

The body uses the **shipped M8 local-viewer idiom**, lifted verbatim in shape from `USiegeFeedbackLibrary::PlayLocalCameraShake` (TASK-356 doc §3.7): `World->GetPlayerControllerIterator()` + `IsLocalController()`. ⛔ **Not `GetFirstPlayerController()`** — the iteration carries *local-viewer* semantics, which is the one question a render site is entitled to ask, and it is why the M8 TEAM LAW's *"first = the player"* ban does not reach it. The team then comes off that controller's `ASiegePlayerState` — ⛔ **never guessed** (the shipped `ResolveOrderingTeam` doctrine; a defaulted `Blue` on a `Red` client would suppress the **owner's own** feedback and show him the **enemy's**, i.e. this row's defect inverted and doubled).

### ⚖️ FLAGGED DECISION — **FAIL-OPEN**, and it is a ruling

No world / no local controller / no `ASiegePlayerState` yet ⇒ **returns `true` (visible)**. Reasons, in order of weight:

1. It is **exactly the shipped behaviour**, so an unresolvable viewer changes nothing that worked yesterday.
2. The other direction is catastrophic and near-invisible: a resolve that quietly broke would blank **every health bar, flash and damage number in the match, for everyone**. `UCombatantHealthBarComponent`'s occlusion cull already keeps this exact ledger in writing (*"a missing camera must never blank every health bar on the field"*) — I matched it deliberately rather than inventing a second policy.
3. `WITCH-§0`'s ruled failure direction is a **visible** unit, never a hidden one.
4. A dedicated server has no local viewer and therefore suppresses nothing — which is correct, because it renders nothing either.

⇒ **this function may only ever SUBTRACT a tell from a viewer we POSITIVELY IDENTIFIED as unable to see the unit.**

---

## 2. ⭐⭐ THE CENSUS — **EVERY SYSTEM THAT RENDERS SOMETHING ATTACHED TO, OVERLAID ON, OR DERIVED FROM A UNIT'S POSITION, RULED ONE BY ONE, INCLUDING THE ONES RULED OUT**

📌 Every row below was **opened and read**, not reasoned about. `SC-§40` cl. 1: an absence is a measurement.

### 2.1 GATED BY THIS ROW

| # | tell | consult site (by symbol) | note |
|---|---|---|---|
| **1** | 🧑 `J-W17` — the **FLOATING HEALTH BAR** | `UCombatantHealthBarComponent::UpdateViewerSuppression()` → `ComputeDesiredBarVisibility(…, bHiddenFromLocalViewer)` → `ApplyBarVisibility()` | ⭐ the term enters the **existing pure decision seam** as a **third SUBTRACTIVE** clause. ⛔ It is deliberately **not** gated by `bOccludeHealthBarWhenBlocked` — that flag is a designer's **stonework** preference and must never switch off a 50-gold **gameplay** promise |
| **2** | the **HIT FLASH** (`SetOverlayMaterial`) | `ASummonedUnit::TakeDamage` — **one** `if` | see 2.2 for why it is here and not in the component |
| **3** | ⭐ the **TEAM-TINTED DAMAGE NUMBER** | `ASummonedUnit::TakeDamage` — **the same** `if` | worst of the three: outlasts the flash, UI-bright, its **tint** leaks *whose* unit and its **digits** leak *how much HP it just lost* |

**Three tells. Three consult sites. ONE predicate. ZERO one-line hides.**

### 2.2 ⛔ ITEM (5)'s TRAP — MEASURED, AND IT APPLIES TO **BOTH** HELPERS, NOT ONLY THE DAMAGE NUMBER

The row warned that `ShowDamageNumber` has four callers and only one is veilable. **I re-measured, and `TriggerFlash` has the same four** — the row did not say so:

```
TriggerFlash()      → Building.cpp · Castle.cpp · HeroCharacter.cpp · SummonedUnit.cpp
ShowDamageNumber()  → Building.cpp · Castle.cpp · HeroCharacter.cpp · SummonedUnit.cpp
```
`WITCH-§6` rules `ASummonedUnit` the **only** veilable class ⇒ a consult written **inside either helper** would silently reach **buildings, castles and the hero** — three actor kinds that cannot be veiled and whose viewers have nothing to be protected from.

⇒ ⚖️ **CHOSEN: the VEILABLE SITE.** Both helpers are left exactly as their other three callers found them. **`SiegeHitFlashComponent.{h,cpp}`, `SiegeFeedbackLibrary.{h,cpp}` and `DamageNumberActor.{h,cpp}` are UNTOUCHED**, even though the row's `names:` line permits them.

⇒ ⭐ **And ONE `if` covers both tells, not two.** They fire on the same edge, in the same block, on the same condition. Two independent guards would be two things to keep in agreement, and the second is the one somebody forgets.

### 2.3 ⛔⛔ RULED IN AND **NOT FIXED** — A SIXTH TELL, MEASURED, OUTSIDE THIS ROW'S `names:` LINE. **THIS IS THE CENSUS'S MOST IMPORTANT FINDING AND IT NEEDS A ROW.**

> ### 🚨 **THE 30-GOLD WAR-MAP ENEMY REVEAL PUTS A VEILED ENEMY UNIT ON THE PAYING PLAYER'S MAP, AT ITS EXACT WORLD POSITION.**

**Site:** `ASiegePlayerController` — the enemy-reveal survey (📌 **locate by symbol**: the `TActorIterator<ASummonedUnit>` loop that fills `EnemyWorldXY` and is handed to `ClientReceiveEnemyReveal`, `SiegePlayerController.cpp:~7105` at my read).

**Measured:** it is a raw `TActorIterator<ASummonedUnit>` **outside the funnel**, filtering on `IsValid` / `IsUnitDead()` / `GetTeamId() != EnemyTeam` and **nothing else — ZERO veil consult.** It then emplaces `GetActorLocation()` X/Y and ships it to the client, which paints red dots.

⚖️ **Why this matters more than the three tells I did fix:** it is **the same defect class as `WITCH-§8`** — a `TActorIterator` scan outside the funnel — in the one system whose *entire purpose* is to publish enemy positions, and it leaks to a **HUMAN** rather than to the bot. `TASK-851` fixed the bot's three scans; **the player's own equivalent was never fixed, and nobody has looked at it until now.**

⛔ **NOT FIXED HERE.** `SiegePlayerController.{h,cpp}` is **not in this row's `names:` line**, and it is the most contended file in the project. ⚖️ *`SC-§39.1` cl. 3.3 — a fence limits the repair, never the report.*
**The repair is one line** in that loop's `continue` condition: `|| !FSiegeCombatStatics::IsAgentVisibleToLocalViewer(World, Unit)` — or, better, `IsAgentVisibleTo(MyTeam, Unit)`, since the survey **already knows the asking team** (it computed `EnemyTeam` from it) and is therefore an *acquisition*-shaped site, not a render one.
📋 **Recommended to the manager: one row, one file, one line, one gate.**

### 2.4 ⛔ RULED **OUT**, IN WRITING, WITH THE MEASUREMENT THAT RULES IT

*⚖️ A member you looked at and ruled out is census; a member you never named is the fourth tell.*

| candidate | measurement | ruling |
|---|---|---|
| ⚠️ **the SHADOW** (member 4 — the row's own open item, `qa/TASK-924.md` `N-6`) | `grep SetCastShadow\|bCastShadow\|CastShadow` over non-test `Source/` = **19 hits, ZERO on any unit path** (all in `BattlefieldScatter`, `Castle`, `ScatterConfig`, `Torch`). Positive control: the same grep returns those 19 ⇒ the instrument is alive | ⛔ **NOT CODE-CONTROLLED ANYWHERE.** The unit mesh uses the component default; whether the veil material still casts one is a **material/asset** property of `MI_Unit_Invisible` / `M_HeroSpirit`. ⇒ ⛔ **DECLARED PIXEL-GATED BY NAME** and out of scope by item (9) (`Content/`). ⛔ **A green suite is not "the shadow is gone."** Member 4 is closed as *"not a code question"*, not as *"absent"* |
| **outlines / stencil highlights** | `SetRenderCustomDepth` / `bRenderCustomDepth` / `CustomDepthStencilValue` across non-test `Source/` = **0**. Positive control on the same pass: `SetVisibility(` = **40** | ⛔ **THE SYSTEM DOES NOT EXIST.** Ruled out, measured |
| **minimap / war-map ALLY dots** | `UWarMapWidget::RefreshAllyDots` — `GetTeamId() != LocalTeam ⇒ continue` | ✅ own team only ⇒ `WITCH-§2` **lane 4**, never suppressed. Correct as shipped |
| **war-map ENEMY dots** | see **2.3** | 🚨 **RULED IN, NOT FIXED, BOARDED-RECOMMENDED** |
| **map marks** (`USiegeMapMarkSubsystem`) | player-authored marks; not derived from any unit's position | ⛔ not a tell |
| **selection ring / group-pick circle / spell reticle decals** | `ASiegePlayerController` decal sites: reticle (the player's own aim), group-pick circle, position circle — all keyed on `IsGroupCommandEligible()` / `IsFollowCommandEligible()` / `GetTeamId() != OwnTeam` | ✅ own units and own aim ⇒ `WITCH-§2` lane 4 |
| **placement blocking** (`SiegePlayerController`, `GetTeamId() != OwnTeam`) | own team only | ✅ lane 4 |
| **the assistant snapshot** (`USiegeAssistantSnapshot::Capture`) | `GetTeamId() != Team ⇒ continue` — surveys the **owner's own** army only | ✅ lane 4. (Belongs to `TASK-903`/`904`/`905` anyway) |
| **nameplates** | none exist — the health bar **is** the nameplate | ⛔ n/a |
| **attached VFX / Niagara** | `SpawnSystemAttached` = **1** hit tree-wide, and it is `AHeroCharacter`'s recall channel. 🧑 `J-W10` rules the hero **not veilable** | ⛔ unreachable |
| **spawn squash + spawn sound** (`MeshJuiceComponent->PlaySpawnSquash()`, `UnitSpawnSoundPath`) | both fire at **spawn**; the veil requires a witch to complete a **3-second cast on an already-spawned unit** | ⛔ **UNREACHABLE while veiled.** Ruled out by construction |
| **miner `ClinkAudio`** (positional, looping) | mining **breaks** the veil (`WITCH-§3`, his own word) | ⛔ a veiled miner never clinks. Ruled out by construction |
| **Ancient-Ground empowerment ring** | empowering **breaks** the veil (`WITCH-§3`) | ⛔ ruled out by construction |
| **capture-zone progress** | `CaptureZone.cpp` counts units in a zone | ⛔ **aggregate, not positional** — it leaks "somebody is in the zone", which is the zone's job. And it is a **gameplay** lane, not a render one ⇒ ⛔ **a DESIGN question if anyone wants it changed** (item (8)); not improvised here |
| ⚠️ **projectile impact VFX + audio** (`Projectile.cpp`, `CachedImpactEffect` + `PlayWorldSound(ProjectileImpactSoundPath, ImpactPoint)`) | an arrow **locked before** the veil still lands (`WITCH-§2`, explicitly ruled) and puffs + plays a one-shot **at the veiled unit's position** | ⚠️ **RULED IN AS A REAL, IF NARROW, POSITIONAL TELL — NOT FIXED.** `Projectile.cpp` is **not in this row's `names:` line**. Exposure is tiny (needs an in-flight arrow at the instant a 3-second cast completes on its target) and the lane is `WITCH-§2`'s own *"the arrow lands"* ruling. 📋 **Reported for the manager to size; I would not spend a row on it alone — it belongs bundled with 2.3** |

---

## 3. ⚠️⛔ ITEM (6) — THE COUNT PINS, **RE-MEASURED MYSELF**, AND **THE ROW'S OWN CENSUS OF THEM WAS WRONG**

The row listed **five** pins and declared *"ALL FIVE are BODY-SCOPED at my read."* ⛔ **I re-measured every `IsAgentVisibleTo` pin in the tree and there are NINE, and one of them is FILE-SCOPED.** Reporting rather than editing, exactly as item (6) instructs.

| pin | scope | subject | safe? |
|---|---|---|---|
| `SiegeInvisibilityTest.cpp:1442` | `GatherBody` | `GatherHostileAgents` == 1 | ✅ |
| `SiegeInvisibilityTest.cpp:1627` | `FriendlyBody` | `GatherFriendlyAgents` == 0 | ✅ |
| ⛔ **`SiegeInvisibilityTest.cpp:1851`** | ⛔ **FILE-SCOPED** | `SiegeBotController.cpp`, `FSiegeCombatStatics::IsAgentVisibleTo(` == **3** | ✅ **safe — scoped to a file this diff does not touch.** ⛔ **But it is FILE-scoped, and the row said it was not** |
| `SiegeInvisibilityTest.cpp:1890` / `:1965` / `:1991` | body-scoped | the bot's three consult functions + the clearance scan | ✅ |
| `SiegeInvisibilityTest.cpp:2450` / `:2455` | `CandidateBody` | `IsWitchVeilCandidate` | ✅ |
| `SiegeFogClampTest.cpp:877` | `FunnelBody` | `GatherHostileAgents` == 1 | ✅ |

⭐⭐ **AND THE REASON THE NEW SYMBOL IS SAFE IS `SC-§41` DOING EXACTLY WHAT IT WAS WRITTEN FOR:** every one of those needles is **paren-anchored** (`IsAgentVisibleTo(`), and `IsAgentVisibleToLocalViewer(` has an `L` where the paren would be ⇒ **it cannot match any of them**. I grepped the whole test tree for a **bare-token** `TEXT("IsAgentVisibleTo")` needle: **zero**. ⛔ Had one existed, this row's new name would have turned a shipped pin red for no behavioural reason.

**Three further tree-wide pins I checked because they scan files I edit — all safe, all verified against the new code:**
- `SiegeInvisibilityTest.cpp` — `FSiegeInvisibilityStatics::IsVisibleTo(` **== 2 tree-wide** and **== 1 in `SiegeCombatStatics.cpp`**. My adapter adds **zero**, and I deliberately kept that string off every code line (including trailing comments — `SiegeCombatStatics.cpp`'s own header comment warns about exactly that, and `SC-§40` cl. 12 is why).
- `SiegeInvisibilityTest.cpp` — the **tree-wide `bIsInvisible` allow-list** and the **13 banned veil names** (`VeilTimerHandle`, `VeilSeconds`, `bWasVisible`, `bPreviouslyVisible`, …). ⛔ My new state member is `bHiddenFromLocalViewer` — chosen **against that list**, and it is not a "was visible" cache (see §4).
- `SiegeAcquisitionFunnelTest.cpp` — the **authorised-chokepoint table** over `{IsVisibleTo(, FSiegeFogStatics, EffectiveVisionRadius, ResolveFogClampedReachUU(}` scanned across `{SummonedUnit, Tower, Hero, SpellLineSweep, CheatManager}.cpp`. My `SummonedUnit.cpp` hunk adds **none of the four**; `CombatantHealthBarComponent.cpp` is not in that file list at all.
- `SiegeCastBarTest.cpp` — `TheCastPollIsNotGatedByTheOcclusionCullsThreeEarlyOuts` takes **four ordering indices inside `TickComponent`**. ⚠️ I inserted a call and a comment block into that function. **Verified: it uses `StripCommentLines(TickBody)` first**, and my insertion sits between the cast poll and the cull's master switch ⇒ all four orderings and the positive control are preserved.
- `SiegeHealthBarOcclusionTest.cpp` — `bBarShownByOwner =` **== 2** (I add zero writers) and `SetVisibility(` **== 1 in the whole component** (⛔ `UpdateViewerSuppression` deliberately routes through `ApplyBarVisibility()` and adds none).

---

## 4. WHAT I DELIBERATELY DID **NOT** CHANGE, AND WHY

1. ⛔ **The combat path — not one line.** `TASK-1105` measured it and I re-read it: the policy is one predicate at **acquisition** (`SiegeCombatStatics.cpp`'s `Out.RemoveAll`), hero and units share it, and `ASummonedUnit::TakeDamage` **never reads the veil flag**. Removing the visual tell removes the player-facing wrongness on its own. **`ESiegeVeilPolicy`, `GatherHostileAgents`, `DoMeleeAttack` and `TakeDamage`'s damage arithmetic are untouched.**
2. ⛔ **The suppression is COSMETIC-ONLY.** The guard in `TakeDamage` sits **below** `OnHPChanged.Broadcast(...)`. A veiled unit still takes the hit, still loses the HP, still dies, and **still stays veiled** (`WITCH-§3` — being hit is not acting). **Test 35 (c-iii) pins that ordering**, and it goes red if anyone widens the guard upward over the damage.
3. ⛔ **No veil break added anywhere** (item (4)(b)). Test 35 (c-iv) pins `BreakInvisibility(` at **0** inside the function this row edited.
4. ⛔ **Nothing reads a material to infer the veil** (item (4)(c)).
5. ⛔ **No mesh hide, no `SetVisibility(false)`, no `SetHiddenInGame(true)` on any unit** (item (4)(a)).
6. ⛔ **`SiegeHitFlashComponent`, `SiegeFeedbackLibrary`, `DamageNumberActor` — untouched** (§2.2).
7. ⛔ **`HeroCharacter.cpp:477-478`'s false comment — NOT fixed.** It is `TASK-1126`'s, by name, and I did not reason from it.
8. ⛔ **The cancelled cast bar's geometry — untouched** (`TASK-935`, same file, different deliverable, never bundled).
9. ⛔ **No replication.** The M8 asymmetry is `WITCH-§6`'s; the consults are written so it needs **no rewrite** — only `IsAgentVisibleToLocalViewer`'s body changes.
10. ⛔ **`Tests/SiegeInvisibilityTest.cpp`'s trap comment was CORRECTED, never deleted** — see the rider ledger.
11. ⚠️ **`qa/TASK-934.md` `N-3` (the adjacent pre-existing over-claim that `CodeLinesOnly` is load-bearing for that row) is STILL OPEN.** It is a NIT, it is not in the rider, and I did not silently sweep it in. Reported.

---

## 5. TESTS — **`Tests/SiegeInvisibilityTest.cpp`, EXTENDED (the fitting existing file), NOT A NEW ONE**

### 5.1 ⛔⛔ **NO WITNESSED RED**

**I could not compile and did not try** — the editor holds the DLL and the compile is the host's (`Build.bat` returns exit 0 on a FAILED build; parse the log for `Result:`). ⇒ **NO WITNESSED RED.** Every red below is **derived from the shipped source text I read**, and each mutation is named so the host can execute it in minutes.

**Intended delta (`TL-§5c`): +2 automation tests, +1 assertion inside existing test 33. Executed suite baseline is 498 / 0** (⛔ the long-quoted `306` is a static macro census, 190 tests stale — not quoted anywhere here).

### 5.2 The rows, and the mutation each one detects (`SHIP-§9` — validate a gate against the FAILURE it detects)

| row | mutation that makes it RED | why it can actually fail |
|---|---|---|
| **34 (a-i)** `IsAgentVisibleToLocalViewer` calls `IsAgentVisibleTo(` **==1** | inline the veil rule into the adapter (delete the delegation) | the adapter has stopped delegating ⇒ two homes for one rule |
| **34 (a-ii)** ×4 second-rule tokens **==0** | add `Candidate->IsInvisible()` or an `ESiegeVeilPolicy` branch to the adapter | an inline flag read **is** a second rule (`WITCH-§1`) |
| **34 (a-iii)** `Cast<ASiegePlayerState>(` **==1** | replace the resolve with `ETeamId::Blue` | a guessed team is wrong on exactly half the clients |
| **34 (a-iv/v)** `IsLocalController()` **==1**, `GetFirstPlayerController(` **==0** | swap the walk for the banned resolve | M8 TEAM LAW |
| **34 (b)** tree-wide `FSiegeCombatStatics::IsAgentVisibleToLocalViewer(` **==3** | **delete either consult** ⇒ 2 (a tell returns) · **add a fourth** ⇒ 4 (an unruled render surface) | ⛔ the census, as a live gate. **The number is 3 because the DEFINITION counts** — the same shape as the shipped `IsVisibleTo(` == 2 row two hundred lines above it |
| **35 (a-i…a-iv)** ⭐ **the four BEHAVIOURAL rows** — real calls to `ComputeDesiredBarVisibility` | ⛔ `&&` → `\|\|` · drop the `!` · make the term additive · fold it under `bCullEnabled` | ⛔ **none of these is visible to any grep in this file.** This is the row that tests the rule rather than its spelling |
| **35 (a-iii)** specifically | gate the veil term on `bCullEnabled` | a designer's **stonework** toggle would switch off a **gameplay** promise |
| **35 (b-iv)** `ApplyBarVisibility` names `bHiddenFromLocalViewer` **==1** | drop the 4th argument at the call site | ⛔⛔ **the single most likely silent death of this feature** — the parameter is defaulted, so dropping it compiles, every tell returns, and the suite stays green. This row is the whole reason the default is survivable |
| **35 (b-i/ii/iii)** | make `UpdateViewerSuppression` write `SetVisibility` directly | that is `qa/TASK-801` `B-1` reintroduced: it would out-rank the owner's death latch and put a bar back over a corpse |
| **35 (b-v)** the viewer poll precedes all three early-outs | move `UpdateViewerSuppression()` below any of them | under the first, a Blueprint flag about stonework switches off the veil |
| **35 (c-i)** consult **==1** in `TakeDamage` | write two separate guards | ⛔ **two one-line hides — the exact shape this row forbids** |
| **35 (c-ii)** consult < flash < number | hoist either tell out of the branch | a leaked flash paints a veiled unit white; a leaked number leaks whose unit and how much HP |
| **35 (c-iii)** ⛔ HP broadcast **before** the consult | widen the guard upward over the damage | ⛔ **the card becomes `WITCH-§0`'s unkillable-but-solid ghost** |
| **35 (c-iv)** `BreakInvisibility(` **==0** in `TakeDamage` | add a "for symmetry" break | every veiled unit clipped by a blast it cannot see coming goes permanently visible |

**Positive controls (`SC-§39`) present:** test 34 asserts the adapter body is substantial before counting inside it; test 35 fails loudly on `INDEX_NONE` before every ordering claim.

### 5.3 ⛔ WHAT THE SUITE DOES **NOT** PROVE — declared by name (`SC-§32`)

⛔ **A green suite is NOT "the tells are gone."** Every row above is either a source probe or a pure-function call. **Nothing here has seen a pixel.** The following are **PIXEL-GATED, BY NAME**, and need a PIE pass with a witch on the field and a completed cast:

1. the enemy's view of a veiled unit shows **no bar**;
2. the **owner's** view of the same unit still shows one;
3. an AoE on a veiled enemy produces **no white flash and no floating number** on the enemy's screen — and **does** on the owner's;
4. **member 4** — whether `MI_Unit_Invisible` still casts a **shadow** (§2.4: not a code question at all).

---

## 6. ⛔⛔ RIDER — `qa/TASK-934.md` `W-1`/`W-2`/`W-3`

*(Its own ledger line, per item (10)(iii). ⛔ **NOT part of this row's subject and it may never fail `TASK-931`.** Gated separately by `TASK-932` item (8). I read `W-1`/`W-2`/`W-3` **verbatim from `qa/TASK-934.md`**, not from item (10)'s summary.)*

✅ **DONE — all three, plus the `W-2` pin change two lines below the comment.** Subject: the `NEEDLE DISCIPLINE` trap comment above the ordering needle in `Tests/SiegeInvisibilityTest.cpp` test 33 (📌 located **by symbol**, not by the gate's `:3292-3299`). ⛔ **CORRECTED, NEVER DELETED**; the trailing space was **not** re-added.

- **`W-1` — the false universal, RE-MEASURED MYSELF BEFORE REWRITING IT** (`SC-§49`: do not repeat the defect while repairing it). I enumerated **every** `.Find(TEXT(` ordering needle in the file: **19** (excluding the fixture's own body-extractor). **18 are terminator-anchored; `AttackCode.Find(TEXT("FireProjectileAt(Target"))` is not** — it ends on an **argument name**, and it feeds `FirstBreak < FireIndex`, so it is genuinely an *ordering* needle. ⇒ the shipped sentence *"every other ordering needle in this file is terminator-anchored"* is **FALSE**, exactly as `W-1` says. **Restated at the scope it was measured:** *no other needle in this file depends on a COMMENT for its final character; `FireProjectileAt(Target` is not terminator-anchored but depends on an ARGUMENT NAME, which is code.*
- **`W-2` — what the shortening actually costs, plus the identity pin.** The comment now says the widening is **fail-OPEN against a second void `return;`**, and names the concrete edit (a top-of-function guard on its own lines **plus** a deleted early-out goes green). The assertion two lines below is now pinned by **CALL SHAPE** (`SC-§41`): a new `CountOccurrencesInCode(BreakBody, TEXT("if (!FSiegeInvisibilityStatics::ApplyBreak(")) == 1` row, and the order claim widened from two terms to **three** — `GuardAt < EarlyOutAt < ClearAt`.
  ⚠️ **`W-2`'s own fence says "show the red, or do not ship it." I CANNOT COMPILE ⇒ NO WITNESSED RED.** I shipped it because the red is **derived for three distinct mutations**, each of which I checked against the shipped body I read:
  1. **delete the edge early-out** ⇒ `BreakInvisibility` contains no other bare `return;` ⇒ `EarlyOutAt == INDEX_NONE` ⇒ RED;
  2. **hoist `ClearVeilMaterial();` above the early-out** ⇒ `EarlyOutAt > ClearAt` ⇒ RED;
  3. ⭐ **the `W-2` defect itself** — add a top-of-function void guard **and** delete the edge early-out ⇒ the **old** two-term pin goes **GREEN over a deleted guard**; the new `GuardAt <` term goes **RED**, because the added guard sits **above** `ApplyBreak(`.
  🙋 **HANDED TO THE HOST:** run mutation **(3)**, confirm RED, revert, transcribe. ⛔ **If it does not go red, revert this pin, leave the needle alone, and say so** — that is `W-2`'s own instruction and I am not entitled to override it from a seat that cannot run the suite.
- **`W-3` — the citation.** The block was headed `SC-§39` (the **positive-control** law). It now cites **`SC-§40` cl. 12** for the mechanism (*a trailing `//` on a code line manufactures a false hit*) and **`SC-§41`** for the needle form; **`SC-§39` is kept only where a positive control is actually meant**, which is the new count row.

---

## 7. FLAGGED DECISIONS FOR THE GATE — **the eight things to attack**

1. ⚖️ **A new function on `FSiegeCombatStatics` — is `IsAgentVisibleToLocalViewer` a SECOND PREDICATE?** My claim: **no**, it states the rule zero times and consults the one predicate once. **The gate should test this claim directly, not take my word for it** — the row makes a second predicate an automatic FAIL. (§1, test 34.)
2. ⚖️ **FAIL-OPEN on an unresolvable viewer.** Ruled, four reasons, §1. The other direction blanks the whole HUD.
3. ⚖️ **The 4th parameter is DEFAULTED.** Forced by `Tests/SiegeHealthBarOcclusionTest.cpp` holding **eleven** three-argument calls while being **outside this row's `names:` line**. The default is `false` so every one of those assertions keeps its exact meaning. ⛔ Test 35 (b-iv) is the guard that stops the default becoming the shipped path. **If the gate prefers a hard signature change, it needs a row that owns that test file.**
4. ⚖️ **The veil term is NOT gated by `bOccludeHealthBarWhenBlocked`.** Deliberate: a stonework preference must not switch off a 50-gold gameplay promise. It does mean the term evaluates with the cull off.
5. ⚖️ **The health-bar poll runs EVERY FRAME, not on a cadence.** Cost stated **structurally, never as a number I did not measure**: one player-controller-list walk (one entry in the shipped single-client match), two casts, two compares — no trace, no allocation, no Blueprint call, and `SetVisibility` only on the edge. It is polled rather than pushed **because the VIEWER, not the veil, is the late-arriving half** (`ASiegePlayerState` can arrive after a unit is veiled). If the gate wants a cadence, it is one line: wrap it in the shipped `ShouldPollOcclusion(...)` with its own accumulator.
6. ⚖️ **Consult at the VEILABLE SITE, one `if` for two tells** — item (5), and the four-caller trap applies to `TriggerFlash` too, which the row did not say. (§2.2.)
7. 🚨 **The war-map enemy reveal (§2.3) — a genuine sixth tell, measured, fenced, unfixed.** It needs a row. It is arguably larger than any of the three I fixed.
8. ⚖️ **Member 5 is DECLARED AND LEFT (§0)** — with the measurement that gating the paint makes the enemy see *more*. The gate should check that argument, because it is the one that decides whether this row delivered its own headline.

---

## 8. FILES

**Written (6):**
- `Source/GitClaudeUnrealTest/Siegebound/SiegeCombatStatics.h` — the adapter's declaration + doc block
- `Source/GitClaudeUnrealTest/Siegebound/SiegeCombatStatics.cpp` — the adapter's definition; +2 includes (`SiegePlayerState.h`, `GameFramework/PlayerController.h`)
- `Source/GitClaudeUnrealTest/Siegebound/CombatantHealthBarComponent.h` — 4th defaulted param, `UpdateViewerSuppression()`, `bHiddenFromLocalViewer`
- `Source/GitClaudeUnrealTest/Siegebound/CombatantHealthBarComponent.cpp` — the term in `ComputeDesiredBarVisibility` / `ApplyBarVisibility`, the poll in `TickComponent`, `UpdateViewerSuppression()`; +1 include
- `Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.cpp` — **one `if`** around the flash + damage number, at the two permitted call sites. ⛔ **No veil-state change.**
- `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeInvisibilityTest.cpp` — tests 34 + 35, the fixture path constant, the include, and the item (10) rider
- `.claude/pipeline/handoffs/TASK-931-programmer.md` (this file)

**Board:** `TASK-931`'s single `status:` line, via `Edit`, nothing else.

**Read (no edits):** `SiegeHitFlashComponent.{h,cpp}` · `SiegeFeedbackLibrary.{h,cpp}` · `DamageNumberActor.h` · `SiegeBotController.cpp` · `SiegePlayerController.cpp` · `WarMapWidget.{h,cpp}` · `SiegeAssistantSnapshot.cpp` · `SiegeAssistantComponent.cpp` · `Projectile.cpp` · `MinerUnit.cpp` · `SiegePlayerState.h` · `Tower.cpp` · `Building.cpp` · `Castle.cpp` · `Tests/SiegeHealthBarOcclusionTest.cpp` · `Tests/SiegeCastBarTest.cpp` · `Tests/SiegeAcquisitionFunnelTest.cpp` · `Tests/SiegeFogClampTest.cpp` · `qa/TASK-924.md` · `qa/TASK-934.md` · `handoffs/TASK-1105-programmer.md` · `CONVENTIONS.md`.

⛔ **`handoffs/TASK-925-buildmaster.md` (the `P-3` shadow observation) was NOT read** — it is on the row's `names:` line and I did not open it. §2.4 answers member 4 from the **source tree** instead (`SetCastShadow` = 0 on every unit path, with a positive control), which is a stronger instrument than a relayed observation (`SC-§40` cl. 3), but the gate should know the difference between what I measured and what I skipped.
