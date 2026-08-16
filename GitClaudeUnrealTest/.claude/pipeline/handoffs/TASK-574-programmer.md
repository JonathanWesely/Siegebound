# TASK-574 — [WR-20] DEFEND is a no-op at 9× — the engagement band re-derivation — programmer handoff

- **assignee:** gameplay-programmer
- **status on completion:** `ready-for-qa`
- **gate:** ⛔ **TASK-565** (`qa/TASK-565.md`) — this task is on that gate's 14-task roster.
- **law worked to:** CONVENTIONS **`WR-§2b` row B** (+ row G) · **`WR-§1`** · **`WR-§0`** · **`SC-§34`** · **`SC-§33`** · **`SC-§22`** · **`SC-§15`** · **`SC-§18c`**.
- **compile / editor / Git / MCP / PIE:** ⛔ **NONE PERFORMED.** File-only, as dispatched. The batch's only compile is TASK-566, its only QA gate TASK-565, its only commit TASK-570.
- **files touched:** `Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.h` · `Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.cpp` — **and nothing else.** ⛔ No `SorcererUnit.{h,cpp}`, no `Castle.{h,cpp}`, no `SiegeGameMode.{h,cpp}`, no `UnitCommand.h`, no `Tower.{h,cpp}`, no `Content/`, no `.csv`, no `Tests/`, no `Build.cs`.

---

## 1. The repair in one paragraph

`ASummonedUnit::DefendRadius` was a disc about the castle **CENTRE**. After the 9× remaster the castle's colliding half-width is ≈**3,657 uu**, so the whole 2,500 disc sat **inside the keep** — besiegers are physically stopped at the gate and stand at ≥3,657, so `AcquireEnemyNearPoint` returned `nullptr` on every tick and every Shield-Wall defender walked home while the castle was battered. Per the manager ruling (`WR-§2b` row B) the **mechanism** is repaired structurally, not by multiplying the constant: `DefendRadius` is now a **BAND MEASURED PAST THE WALL FACE**, converted to a centre radius at use by a new private helper that reads the own castle's **live colliding bounds**. The default moves `2500 → 1281`, chosen to be **behaviour-preserving rather than invented**.

### ⭐ The arithmetic — this is the whole argument that it is a repair and not a balance change

| castle | measured colliding half-extent | + band `1281` | resolved DEFEND disc |
|---|---|---|---|
| **3× (as shipped when the old number was authored)** | **1218.95** | + 1281 | = **2500.00** — ⭐ **byte-identical to the shipped behaviour** |
| **9× (today)** | **3656.85** | + 1281 | ≈ **4937.85** — a real, working band 1,281 uu past the wall |

⇒ **`1218.95 + 1281 = 2500` reproduces the old castle's band EXACTLY.** Nothing about DEFEND's *feel* on the old geometry changes; what changes is that the band still exists on the new geometry.

### ⚖️ The number is still Jonathan's

`DefendRadius` stays `UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ClampMin = "0")` in `Category = "Siegebound|Commands"`, and its doc keeps the ***"Q6 default; FLAGGED tunable"*** provenance verbatim in substance, now naming playtest item **D10** explicitly. ⛔ The flag was not deleted and nothing else about DEFEND was re-balanced. The ruling changed the **mechanism**; the **number** is his.

---

## 2. What changed, by symbol (⛔ located by symbol, never by line offset — `SC-§18c`)

### `SummonedUnit.h`

| symbol | change |
|---|---|
| `ASummonedUnit::DefendRadius` | **Default `2500.f` → `1281.f`.** Doc block rewritten: the new BAND semantic, why a centre radius cannot work at any scale, the `1218.95 + 1281 = 2500` arithmetic, the D10 provenance, and an explicit ⛔ *"never pass this member straight to `AcquireEnemyNearPoint`"*. **`UPROPERTY` specifiers, category and `ClampMin` are UNCHANGED.** |
| `ASummonedUnit::ResolveDefendEngagementRadius` | **NEW** private member function — `float ResolveDefendEngagementRadius(const ACastle* OwnCastle);` — declared immediately after `FindOwnCastle()`, in the same private block. Doc names the three contracts it is the sole keeper of. |
| `ASummonedUnit::bLoggedDefendBandDerived` | **NEW** private `bool` one-shot guard (next to `bWarnedNoAIController`). |
| `ASummonedUnit::bLoggedDefendBandFallback` | **NEW** private `bool` one-shot guard. |
| `ASummonedUnit::StructureGoalProjectionExtent` | ⭐ **COMMENT ONLY** — spec item (7) / `WR-§2b` row G / TASK-557 row S5. `"the 2437×2461 castle"` → the live `7313.7 × 7384.5 × 8082.6`, plus an explicit `SC-§34` (ii) note. ⛔ **The value `FVector(800, 800, 600)` is UNTOUCHED** (row S10 verified it; it measures a body-scale gap at a face, not a castle dimension). |
| `UpdateStateStandardCommanded` doc — the `• DEFEND` bullet | **COMMENT ONLY** — now describes the derived radius. |
| `GetDistanceToTarget` doc | **COMMENT ONLY** — `"~800x800"` → `7313.7 × 7384.5`. **Uncited instance found by this task's sweep (§4).** |

### `SummonedUnit.cpp`

| symbol | change |
|---|---|
| `UpdateStateStandardCommanded`, `case ESiegeUnitCommand::Defend` | **THE ONE BEHAVIOURAL LINE.** `AcquireEnemyNearPoint(OwnCastle->GetActorLocation(), DefendRadius)` → `AcquireEnemyNearPoint(OwnCastle->GetActorLocation(), ResolveDefendEngagementRadius(OwnCastle))`. The disc is still **centred on the castle**; only its radius is now derived. |
| `ASummonedUnit::ResolveDefendEngagementRadius` | **NEW** definition, placed between `FindOwnCastle` and `UpdateStateSiege`. |
| anonymous-namespace `StructureMoveAcceptanceRadius` doc | **COMMENT ONLY** — `"~800x800"` / cylinder `"~566"` → `7313.7 x 7384.5` / `~5175`. ⛔ **The `50.f` value is UNTOUCHED** (`SC-§34` (ii), move-acceptance tolerance). **Uncited instance found by this task's sweep.** |
| `GetDistanceToTarget` inline comment | **COMMENT ONLY** — `"~800x800 footprint"` → the live footprint. **Uncited instance found by this task's sweep.** |

---

## 3. ⭐⛔ THE SORCERER SEAL — the highest-value trap in this task, and how it is solved *from `SummonedUnit`*

**The trap:** `SorcererUnit.cpp` sets `DefendRadius = 0.f` with the comment *"at 0 the disc is empty, so a sorcerer never targets"* — half 1 of the never-attacks seal. A naive `Max(BoxExtent) + Band` turns that `0` into **≈3,657 uu at the 9× castle** and hands a unit that **cannot attack** a live target disc.

**The solution, and it is one branch:** `ResolveDefendEngagementRadius` tests the band **FIRST, before any geometry is queried**:

```cpp
if (DefendRadius <= 0.f)
{
    return 0.f;
}
```

- ⭐ **`0` is short-circuited to `0` — it never reaches `GetActorBounds`.** `AcquireEnemyNearPoint` receives `0`, its `RadiusSq` is `0`, its disc filter admits nobody, and the sorcerer marches home. **This is byte-for-byte the pre-change behaviour for a 0 band**, including the degenerate "candidate exactly at the centre" edge, which is deliberately left exactly as it was rather than tightened.
- ⛔ **`SorcererUnit.{h,cpp}` was NOT opened, NOT edited, and did not need to be.** The seal is honoured entirely from `SummonedUnit`, as the spec required.
- Tested `<= 0` rather than `== 0` so a hand-authored negative can never resolve into a live radius either.
- ⚠️ **A rename of `DefendRadius` was considered and REFUSED for a mechanical reason, recorded in the header:** `ASorcererUnit`'s constructor writes the member **by name**, and that file is outside this task's ownership — a rename would have broken a file I may not edit. **The MEANING moved; the SYMBOL stayed.** This is also why the spec's `names:` block says *member `DefendRadius` (semantic change)*.

**✅ QA should verify by reading `SorcererUnit.cpp:25-28` unchanged:** its operative sentence — *"at 0 the disc is empty, so a sorcerer under DEFEND falls back to marching home"* — **is still true** after this change. Only its incidental mechanism clause (*"targets via `AcquireEnemyNearPoint(own castle, DefendRadius)`"*) is now slightly imprecise; it is reported in §5 as (iii), not edited.

---

## 4. `SC-§22` SWEEP — I treated the citation as a lower bound, and it was one

The spec cited **one** stale claim in my files (`SummonedUnit.h:723`). **The shape — *"a comment asserting the castle's footprint as a present-tense fact"* — was swept for, and three MORE instances were found in the same two files, none of them cited by anybody.**

**CMD-A (the shape sweep):**
```
grep -niE "2437|2461|800x800|800 x 800|~810|810-unit|3x castle|3× castle|hollow 3|1219|1218" \
  Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.h \
  Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.cpp
```

| hit | disposition |
|---|---|
| `SummonedUnit.h` — `StructureGoalProjectionExtent` doc, *"the 2437×2461 castle"* | ✅ **FIXED** — the cited one (spec item 7). |
| `SummonedUnit.h` — `GetDistanceToTarget` doc, *"the castle's ~800x800 base"* | ⭐ **FIXED — UNCITED.** Two remasters stale (it quotes the **M1** castle). |
| `SummonedUnit.cpp` — `StructureMoveAcceptanceRadius` doc, *"(~800x800 footprint)"* + *"cylinder (radius ~566)"* | ⭐ **FIXED — UNCITED.** Both figures M1-era. |
| `SummonedUnit.cpp` — `GetDistanceToTarget` body comment, *"an ~800x800 footprint"* | ⭐ **FIXED — UNCITED.** |
| `SummonedUnit.h` / `.cpp` — two *"the hollow 3× castle"* references (TASK-349 lineage) | ⛔ **DELIBERATELY LEFT, and here is why:** they are **historical narrative** — statements about the castle that *existed when TASK-349 shipped its fix* — not present-tense claims about today's geometry, and both remain true as history. Rewriting them would have destroyed the lineage `SC-§34` exists to preserve. |

⚠️ **Note for the manager, because it validates the rule:** `WR-§2b` row G named exactly one line in my files and there were **four**. Independently, **TASK-577 item (3) is fixing the *identical* `"~800x800"` sentence in `HeroCharacter.cpp:428`** — so this same M1-era claim was copy-propagated across at least **five** sites in three files. **Every one of them was comment-only; not one had a wrong constant behind it.**

**CMD-B (the castle-scale constant sweep — the `SC-§34` enumeration, run over my files):**
```
grep -nE "^[[:space:]]*(constexpr[[:space:]]+)?(float|int32|double|FVector)[[:space:]]+[A-Za-z_]+[[:space:]]*=.*[0-9]{4,}" \
  Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.h \
  Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.cpp
```
**RAW HIT COUNT: 1** — `SummonedUnit.h: float DefendRadius = 1281.f;` ⇒ ✅ **`DefendRadius` is the ONLY four-digit numeric member in either file.** There is no second castle-scale constant hiding here.

---

## 5. `SC-§34` RE-DERIVATION LEDGER — every constant in my two files, one row each

⛔ **Silence is not a row.** ✅ **Most rows read "already correct", and per `SC-§22`'s closing rule that is reported as a RESULT.**

| # | constant / mechanism | disposition |
|---|---|---|
| 1 | **`DefendRadius` `2500`** | ⛔ **(i) RE-DERIVED, STRUCTURALLY.** Mechanism → band past the wall face, resolved from live bounds. Default `2500 → 1281`. Arithmetic in §1. **A ×3 to `7500` was available and was refused** — it buys exactly one resize before rotting, and the manager banned it for this row. |
| 2 | **`StructureGoalProjectionExtent` `(800, 800, 600)`** | ✅ **(ii) DELIBERATELY UNCHANGED — `WR-§1` / `SC-§34` human-scale exemption.** It measures the **GAP AT A FACE** (agent-radius erosion ≈34 uu + interior-area hull-box margin), both keyed to a **unit's body**, not to any castle dimension. Units did not grow. TASK-557 row S10 verified the value; only the citation was wrong. **Citation fixed, number untouched.** |
| 3 | **`StructureMoveAcceptanceRadius` `50`** (anon namespace, `.cpp`) | ✅ **(ii) DELIBERATELY UNCHANGED.** A move-acceptance tolerance keyed to a unit's body and its path-follow granularity. ⭐ Its comment's *argument* (*"a bounding-cylinder reach test would stop the unit well outside attack range"*) gets **stronger** as the castle grows, never weaker — so the reasoning survives verbatim and only the two cited figures were stale. |
| 4 | **`GetDistanceToTarget` / `ActorGetDistanceToCollision`** | ✅ **(ii) ALREADY CORRECT AND SELF-DERIVING — this is the "already correct is a RESULT" row, and it is the good news.** Distance is measured to the **closest point on the target's collision**, so it auto-follows the mesh at any scale. ⭐ **This is precisely why unit-vs-castle combat did NOT break at 9× while DEFEND did** — one mechanism derives, the other transcribed. Citation fixed, mechanism untouched. |
| 5 | **`AggroRadius` 600 · `LeashRange` 900 · `TieBreakDistance` 100** | ✅ **(ii) DELIBERATELY UNCHANGED — GDD §3.8 profile constants keyed to BODIES, never castle-derived.** Recorded because they are the obvious false positives for a *"the castle got bigger, scale the AI numbers"* pass, and scaling them **is** the defect (`WR-§1`). |
| 6 | **`UnitDamageNumberHeightZ` 110** (anon namespace, `.cpp`) | ✅ **(ii) DELIBERATELY UNCHANGED** — height over a **unit's** head. |
| 7 | **`UnitCommand.h:22`** — *"enemies attacking it (within DefendRadius)"* | ⛔ **(iii) OUTSIDE THIS TASK'S OWNERSHIP — NAMED.** `UnitCommand.h` is in my ⛔ NOT-touched list. Now imprecise (a band, not a radius). ⚠️ **It is NOT covered by TASK-577 either** — that task owns only `SiegePlayerState.cpp` / `CaptureZone.h` / `HeroCharacter.cpp` and is explicitly forbidden from extending. **Reported per `SC-§15`, not fixed. Cost of leaving it: one imprecise word in a doc comment; zero emitted bytes.** |
| 8 | **`SorcererUnit.cpp:26`** — *"targets via `AcquireEnemyNearPoint(own castle, DefendRadius)`"* | ⛔ **(iii) OUTSIDE OWNERSHIP — NAMED.** ⭐ Its **operative** claim (*"at 0 the disc is empty, so a sorcerer never targets"*) **is still true** (§3); only the mechanism clause drifted. Same disposition and same reason as row 7. |
| 9 | **`DeckBuilderWidget.cpp:54`** — *"its ctor's AggroRadius/DefendRadius 0"* | ✅ **STILL LITERALLY TRUE.** No change owed, in any file. Recorded so nobody re-opens it. |

---

## 6. ⛔ `SC-§33` — THE TRAILING-DEFAULT LAW: **NO new trailing defaulted parameter was added**, and here is the pasted, enumerated grep

⭐ **`SC-§33` does not bind this task, and I am proving it rather than asserting it.** The new function `ResolveDefendEngagementRadius(const ACastle* OwnCastle)` takes **one REQUIRED parameter, no default**, and shipped with **zero prior call sites** — both of `SC-§33`'s own scope exclusions. `AcquireEnemyNearPoint`'s signature is **unchanged**.

**CMD-1 — every `DefendRadius` site in the entire tree:**
```
grep -rn "DefendRadius" Source/ Content/ Tests/ Config/
```
**RAW HIT COUNT: 22.** Enumerated:

| # | site | classification |
|---|---|---|
| 1 | `DeckBuilderWidget.cpp:54` | (ii) **LEFT** — comment, still true (ledger row 9). |
| 2–3 | `SorcererUnit.cpp:25,26` | (iii) **OUTSIDE OWNERSHIP** — comment (ledger row 8). |
| 4 | `SorcererUnit.cpp:28` — `DefendRadius = 0.f;` | ⭐ **(ii) DELIBERATELY LEFT AND IT IS LOAD-BEARING.** The **only write** outside the declaration. Its meaning is **preserved by construction** via the `<= 0` early-out (§3). ⛔ Not edited; not editable by this task. |
| 5 | `SummonedUnit.cpp:1655` | (i) **UPDATED** — new comment at the call site. |
| 6–13 | `SummonedUnit.cpp:2162, 2171, 2190, 2192, 2221, 2223, 2226, 2238` | (i) **NEW** — all inside `ResolveDefendEngagementRadius` (1 comment, 1 guard, 6 reads/returns). |
| 14–21 | `SummonedUnit.h:716, 740, 1129, 1183, 1186, 1196, 1500, 1706` | (i) **UPDATED/NEW** — 1 declaration (`:740`), 7 doc references. |
| 22 | `UnitCommand.h:22` | (iii) **OUTSIDE OWNERSHIP — NAMED** (ledger row 7). |

**⭐ Reads of `DefendRadius` that feed a target query: exactly ONE before this change (`SummonedUnit.cpp:1647`), exactly ONE after (inside the resolver). There is no second consumer to have missed.**

**CMD-2 — every site of the new function:**
```
grep -rn "ResolveDefendEngagementRadius" Source/ Content/ Tests/
```
**RAW HIT COUNT: 6** — `SummonedUnit.cpp:1660` (the **one and only call site**, fully argumented) · `SummonedUnit.cpp:2159` (definition) · `SummonedUnit.h:1208` (declaration) · 3 doc references (`SummonedUnit.h:710, 1127`, `SummonedUnit.cpp:1658`). ✅ **One declaration, one definition, one call — no under-called site is possible.**

**CMD-3 — every `AcquireEnemyNearPoint` call site (signature UNCHANGED, swept anyway):**
```
grep -rn "AcquireEnemyNearPoint(" Source/
```
**RAW HIT COUNT: 9.** Four are real call sites:

| call site | classification |
|---|---|
| `SummonedUnit.cpp:1660` — DEFEND | (i) **UPDATED** — now passes the resolved radius. |
| `SummonedUnit.cpp:1787` — grouped, monotone upgrade, `Group.AttackCenter/AttackRadius` | ✅ **(ii) DELIBERATELY UNCHANGED — and NOT castle-derived at all.** Group zones are **player-drawn** (`FSiegeUnitGroup`), so their centre and radius come from the player's own gesture, not from any castle dimension. ⛔ Applying a wall-face band here would corrupt a zone the player drew. |
| `SummonedUnit.cpp:1816` — grouped tier 1 | ✅ **(ii) DELIBERATELY UNCHANGED** — same reason. |
| `SummonedUnit.cpp:1820` — grouped tier 2, `Group.PositionCenter/PositionRadius` | ✅ **(ii) DELIBERATELY UNCHANGED** — same reason. |

Remaining 5 hits: 1 definition (`:2057`), 1 declaration (`SummonedUnit.h:1162`), 3 doc/comment references. ✅ **Result: no call site of any function was left under-argumented.**

**CMD-4 — is the header default LIVE, or shadowed by a saved asset? (the *"TWO ARTIFACTS, TWO TASKS"* diagnosis, `WR-§2b` rows D/E):**
```
grep -c "DefendRadius" Content/Blueprints/Units/*.uasset      # → 0 for all 13 unit BPs
grep -rl "DefendRadius" Content/                              # → no files
grep -c "CardID"       Content/Blueprints/Units/BP_Unit_Footman.uasset   # → 1  (POSITIVE CONTROL)
```
✅ **`DefendRadius` appears in ZERO `Content/` assets.** The `CardID` positive control proves the grep genuinely reads the `.uasset` name table (an overridden property *does* serialise its name into the BP), so the zero is a real negative and not a tooling artefact. ⇒ ⭐ **No BP subclass overrides `DefendRadius`; the C++ header default is LIVE and this edit is NOT inert.** ⚠️ **Stated as evidence, not proof** — a definitive answer needs the editor, and I am file-only by dispatch. **If TASK-566/569 sees the old 2,500 band at runtime, this is the first thing to re-check in-editor.**

---

## 7. Behavioural surface — exactly what does and does not change

| path | change |
|---|---|
| DEFEND, Blue Standard, own castle standing, band > 0 | ⭐ **THE ONE CHANGE.** Disc radius `2500` → `castle half-width + 1281`. At the 3× castle: **identical (2500.00)**. At the 9× castle: **≈4937.85 instead of a dead 2500**. |
| DEFEND, band == 0 (**the Sorcerer**) | ✅ **BYTE-IDENTICAL.** Short-circuits before any geometry (§3). |
| DEFEND, no own castle | ✅ **UNCHANGED** — `UpdateStateStandardCommanded` still `EnterIdle()`s **before** the resolver is ever called. The resolver's own null branch is defensive and **unreachable from today's single call site** — declared here rather than hidden, and kept because the null-safety contract belongs to the function that does the dereference. |
| DEFEND, degenerate castle bounds | ✅ **PRE-CHANGE BEHAVIOUR** — authored value used as a plain centre radius, warned once. ⛔ Never `0` by accident, never a crash. |
| **ATTACK · HOLD · every group order · Siege · Support · Miner · every bot/Red unit** | ✅ **BYTE-FOR-BYTE UNCHANGED.** Not one of them reads `DefendRadius`. |
| `AMinerUnit`'s DEFEND | ✅ **UNCHANGED** — it walks to the castle **interior anchor** (TASK-398) and never touches `DefendRadius`. Verified, not assumed. |
| `ASummonedUnit::IsTargetAlive` | ✅ ⛔ **NOT TOUCHED, AS INSTRUCTED.** It still returns `true` for unknown `ITeamAgent` types — the property `ACommanderNpc` depends on by deliberately not implementing that interface. I read it and left it alone. |

**Cost:** one extra `GetActorBounds` call per **defending** unit per **0.25 s** state tick, and only inside the DEFEND branch. Same query the hero-spawn path already runs. ⛔ **No frame-rate claim is made** (`WR-§4` perf clause).

---

## 8. 📌 M8 DECLARATION (`WR-§8`)

⛔ **NOTHING M8-relevant.** No replicated property, no `GetLifetimeReplicatedProps` change, no new class, no new relevancy tier, no RPC, no `Server*`/`Client*` function. The new helper is a **private, non-replicated, non-`UFUNCTION` C++ member** on an existing actor; the two new members are **plain `bool`s, not `UPROPERTY`**, so they are not replicated and not serialised. The state machine that calls it already runs **authority-side only**, exactly as before. `DefendRadius` remains `EditDefaultsOnly` (a class default, not per-instance state). **DECLARED.**

---

## 9. ⚠️ WHAT QA SHOULD SCRUTINISE (my own list of where I could be wrong)

1. ⭐⭐ **THE 0-BAND SEAL IS THE ONE THAT MATTERS.** Read `ResolveDefendEngagementRadius`'s first branch and satisfy yourself that a `DefendRadius` of `0` **cannot** reach `GetActorBounds`. If it can, a Sorcerer starts picking fights it cannot finish and half the never-attacks seal is gone.
2. **The `FMath::Max` double/float trap.** `FVector` components are `double` in UE5 and `FMath::Max` is a single-type template. I take the max in **double** then `static_cast<float>` **once** — mirroring `SiegeGameMode.cpp`'s branch-3 comment about the same trap. ⚠️ **If you think template deduction still fails here, that is a real compile blocker and TASK-566 will eat it** — please check it now.
3. **`bOnlyCollidingComponents = true`.** If this ever flips to `false`, the castle HP-bar widget (≈9,450 uu up at 9×) folds into the extent and the band silently inflates.
4. **`UE_KINDA_SMALL_NUMBER`, not `KINDA_SMALL_NUMBER`.** Chosen to match the in-file idiom (`SummonedUnit.cpp` already uses `UE_KINDA_SMALL_NUMBER`; `HeroCharacter.cpp` too). No new `#include` was needed — `Castle.h` was already included.
5. **Log volume.** One `Log` line per unit lifetime, never re-armed. This is the **`AMinerUnit::bLoggedInteriorAnchor` idiom verbatim** and it is deliberate: it is the **acceptance instrument** (§10). If you judge one line per defending unit too noisy, say so — but the alternative is a PIE row with nothing to read.
6. **The two fallback causes share one guard flag.** Deliberate: both are static over a unit's lifetime and produce identical behaviour, so a second flag could only ever add a duplicate line. The message names which cause fired.
7. **Non-ASCII in comments and inside two `UE_LOG` `TEXT()` format strings.** ✅ **Checked against precedent before writing:** this exact module already ships 33 such comment characters in `MinerUnit.cpp`, 274 in `SiegeAssistantComponent.cpp`, and **14 `UE_LOG` `TEXT()` literals with non-ASCII in `MinerUnit.cpp` alone**. Not a new risk; recorded so it does not cost a finding.
8. ⚠️ **Comment-filtered-diff artefact, so you do not think a line vanished:** a filter that strips lines beginning with `*` also strips the `*GetNameSafe(this), ...` argument continuations of my `UE_LOG` calls (C++ dereference vs doc-comment star). They are present in the file; only the filter drops them.

---

## 10. ⭐⛔ THE ACCEPTANCE INSTRUMENT — TASK-569 PIE rows (n)(o)(p): **exactly what a human should observe**

⚠️ **The manager ruled TASK-564 will NOT unit-test this** — a test here would assert `GetActorBounds`, i.e. the engine, not our logic. ⇒ **PIE is the whole acceptance, so here is precisely what to do and what a PASS looks like.**

### (n) THE LOG LINE — the derivation, read back with no editor probe

**Do:** boot PIE on `L_Arena`, summon **two or three Blue Standard units** (Footman/Knight/Archer — ⛔ **not** the Sorcerer, ⛔ **not** the Miner), press the **DEFEND** stance key, and search the Output Log for **`DEFEND band resolved`**.

**PASS —** one line per defending unit, of this shape, with a half-width in the **3,600s** and a resolved radius in the **4,900s**:
```
[BP_Unit_Footman_C_0] DEFEND band resolved: castle <Castle> measured colliding half-width 3656.85
+ authored band 1281 (past the wall face) => engagement radius 4937.85 uu.
```
- ⛔ **FAIL — half-width ≈1218**: the castle in the level is still the **3× mesh**; the art/integration side did not land. **NOT a defect in this code** — the code is doing exactly the right thing to the wrong castle.
- ⛔ **FAIL — no line at all, ever**: the DEFEND branch is not being entered (check the stance actually latched, the units are **Blue** and **Standard**, and `HasIssuedCommand()` is true) — or the DEFEND branch took the null-castle path.
- ⛔ **FAIL — a `DEFEND band:` *Warning*** (either `no own castle to measure` or `degenerate colliding bounds`): the fallback fired. The band is running at the authored `1281` as a plain centre radius, which is **worse than the old 2500** and must not ship. The warning names which cause.

### (o) THE BEHAVIOUR — the defect itself, reproduced and then absent

**Do:** let a bot attack wave reach the Blue castle so besiegers are **standing at the wall/gate**. With the wave in contact, issue **DEFEND** to a handful of Blue Standard units posted out in the field.

**PASS —** ✅ **the defenders turn around and ENGAGE the besiegers at the wall.**
⛔ **FAIL — the exact defect this task exists to repair:** the defenders **walk home and stand there** while the castle takes damage, never swinging. That is `AcquireEnemyNearPoint` returning `nullptr`, i.e. the disc is still inside the keep.
⚠️ **Read this row carefully — it is the only observation that can distinguish "fixed" from "still broken", because the broken build looks calm and orderly, not crashy.**

### (p) THE SORCERER SEAL — the regression guard

**Do:** summon a **Sorcerer**, issue **DEFEND** with enemies near the castle.

**PASS —** ✅ **the Sorcerer marches home and never attacks, exactly as it does today**, and ⛔ **NO `DEFEND band resolved` line is ever printed for it** (the 0-band short-circuits before the log).
⛔ **FAIL —** a Sorcerer that walks at an enemy, stands next to one, or prints a `DEFEND band resolved` line. Any of those means the `<= 0` early-out was bypassed.

### 📌 Bonus, free, and worth one glance
The three corrected comments are **comment-only, zero emitted bytes** — ⛔ **there is nothing to observe at runtime for them**, and no PIE row should be spent trying.

---

## 11. `SC-§15` — DECLARED DEPARTURES AND ADDITIONS OVER THE SPEC

⭐ **Nothing below is silent. Each is a departure or an addition, stated so the gate can refuse it.**

1. ⭐ **ADDITION — three UNCITED stale comments fixed beyond the one the spec named.** Spec item (7) named `SummonedUnit.h:723`; the `SC-§22` sweep found **three more of the same shape** in the same two files (§4). All comment-only, all inside my owned files, all under the *"while you are in the file"* rule (`WR-§2b` row G). ⛔ **No file outside my ownership was opened for a comment.**
2. ⭐ **ADDITION — a per-unit informational `Log` line on successful derivation**, not requested by the spec. **Justification:** the manager ruled TASK-564 will not test this, so **PIE is the entire acceptance**, and without this line a human has nothing to read. It follows the shipped `AMinerUnit::bLoggedInteriorAnchor` idiom, which exists for the identical reason. **Refusable:** deleting it costs three lines and one header bool, and costs TASK-569 its (n) row.
3. ⚠️ **DEPARTURE FROM A LITERAL READING — the member was NOT renamed to match its new meaning.** Reason is mechanical, not stylistic: `ASorcererUnit`'s constructor writes it **by name** from a file I may not edit (§3). The spec's `names:` block anticipated this (*member `DefendRadius` (semantic change)*), so I read this as compliance — but it is flagged because *"a member whose name says radius and whose meaning is band"* is a real readability cost, and it is paid deliberately. ⭐ **Mitigation shipped: the header states ⛔ "NEVER pass this member straight to `AcquireEnemyNearPoint`" and names the single legal read.** A rename is a clean follow-up for any later wave that owns **both** files.
4. **`SC-§34` (iii) rows NAMED, not fixed:** `UnitCommand.h:22` and `SorcererUnit.cpp:26` (ledger rows 7–8). ⚠️ **Neither is covered by TASK-577**, which is scoped to three other files and explicitly forbidden from extending. **They need a home in a later batch or they will still be stale next resize.**
5. ⚠️ **The `!IsValid(OwnCastle)` branch is unreachable from today's only call site** and is kept anyway (§7). Declared so it is not mistaken for an oversight or for dead code that slipped through.
6. 📌 **`git status --porcelain` shows `PermanentDamageBonusPerStack` / `MaxPermanentDamageStacks` `// GDD §x.x → §3.12` in `SummonedUnit.h`'s diff. ⛔ THOSE ARE TASK-554'S, NOT MINE.** TASK-554 authored them in this same file before I started (it is my `blocked-by`). Recorded so the gate and TASK-570 attribute them correctly.
7. ⛔ **QUOTED NO TOKEN FIGURE** anywhere in this handoff (batch-wide ban, `WR-§6`). Every number here is uu, a hit count, or a line number.
