# TASK-576 — [WR-22] THE SCATTER'S CASTLE-DERIVED CONSTANTS — `AncientGroundMaxAbsX` + `CastleQueryInset`

**Agent:** gameplay-programmer · **Date:** 2026-08-15 · **Status → `ready-for-qa`**
**Law:** CONVENTIONS `WR-§2b` rows **D** + **E** · ruling **`W2-R1`** · the amended **"Ancient Grounds …"** clause · `SC-§34` · `SC-§33` · `SC-§22` · `SC-§18c` · `SC-§15`
**Gate:** `qa/TASK-565.md` (the batch's ONLY gate) · **Compile:** TASK-566 (the ONLY compile) · **Commit:** TASK-570 (the ONLY commit)

⛔ **No compile · no editor · no MCP · no PIE · no Git · no `Content/` asset · no `.csv` · no `Tests/`.** Three files, all in my `names:` block.
⛔ **No token figure is quoted anywhere in this handoff** (batch-wide ban).
⛔ **Every edit was located BY SYMBOL** (`SC-§18c`) — the line numbers below are read-backs for QA's convenience, never how anything was found.

**Files touched — exactly the three I own:**
`Source/GitClaudeUnrealTest/Siegebound/ScatterConfig.h` · `Source/GitClaudeUnrealTest/Siegebound/BattlefieldScatter.h` · `Source/GitClaudeUnrealTest/Siegebound/BattlefieldScatter.cpp`
**NOT touched:** `AncientGround.{h,cpp}` · `CaptureZone.{h,cpp}` · `Castle.{h,cpp}` · `SiegeBotController.{h,cpp}` · `SiegeGameMode.{h,cpp}` · `SummonedUnit.{h,cpp}` · `SiegePlayerController.{h,cpp}` · any `Content/` asset · any `.csv` · `Tests/` · `Build.cs`.
⛔ **TASK-557's diff is intact.** I re-read `handoffs/TASK-557-programmer.md` first. Its `ScatterConfig.h` edit is `CastleKeepClearRadius 1500 → 4500`; I **read** that field (twice, in the derivation below) and **wrote** only `AncientGroundMaxAbsX` and its doc block. `ArenaHalfExtent`, `AncientGroundHalfExtent`, `AncientGroundMinAbsX`, `AncientGroundMaxAbsY`, the mine constants and the `|Y| ≤ 1000` corridor ruling are byte-untouched.

---

## 1. ⭐ THE ARITHMETIC FOR `16080`, VERIFIED BY ME AND PASTED (spec item 1)

I did **not** re-derive it from a multiplier. I checked the one property the ruling rests on — *that it re-solves the shipped relationship instead of inventing a new margin* — and **both original margins come back exactly**:

| quantity | BEFORE (retired) | AFTER (`W2-R1`) | preserved? |
|---|---|---|---|
| `SpawnBoxHalfExtent.X` (TASK-557 row 1, three sites) | 2,460 | 7,380 | — (the cause) |
| spawn-box inner edge = 25,000 − half-extent | **22,540** | **17,620** | — (the cause) |
| ceiling `AncientGroundMaxAbsX` | 21,000 | **16,080** | the output |
| **centre margin** = edge − ceiling | 22,540 − 21,000 = **1,540** | 17,620 − 16,080 = **1,540** | ✅ **exact** |
| footprint edge = ceiling + `AncientGroundHalfExtent.X` (840) | 21,840 | **16,920** | — |
| **footprint-edge clearance** = edge − footprint edge | 22,540 − 21,840 = **700** | 17,620 − 16,920 = **700** | ✅ **exact** |

**The defect it repairs, checked both ways:**
- **Spawn box:** at the retired ceiling a ground centre sat 21,000 − 17,620 = **3,380 uu INSIDE** its own team's spawn box (4,220 at the footprint edge) ⇒ *"you fight over it, you do not spawn on it"* was **inverted**. At 16,080 it is 1,540 uu in front of the edge again.
- **Castle footprint:** the 9× colliding half-depth on X is **3,656.85** ⇒ the footprint starts at |X| ≥ 25,000 − 3,656.85 = **21,343.15**. The retired footprint edge **21,840 overlapped it by 496.85 uu**. The new edge **16,920 is 4,423.15 uu clear**.
- **Keep-clear disc, i.e. the figure the amendment retires:** with `CastleKeepClearRadius` **4,500** the disc bites at 25,000 − 4,500 = **20,500**, which is **3,580 uu OUTSIDE** the 16,920 edge ⇒ ⛔ **the omitted disc test in `PlaceAncientGrounds` is STILL a provable no-op**, for exactly the reason it always had. (The retired `23,500` figure is 25,000 − 1,500, i.e. the pre-`WR-§2`-row-4 radius — internally consistent, and now dead.)

**Edit:** `ScatterConfig.h` → `float AncientGroundMaxAbsX = 16080.f;`, and the doc block rewritten so it **names its source instead of transcribing an absolute** (`SC-§34`): *"anyone tuning this band measures against the LIVE `SpawnBoxHalfExtent`"*. The retired **21,000** is **recorded, not deleted**, with its cause — a future tuner meets it in git history.

---

## 2. ⛔⛔ TWO ARTIFACTS, TWO TASKS — SAID LOUDLY, **AND THE OVERRIDE PATH WAS ACTUALLY MEASURED, NOT ASSUMED**

> ### ⛔ **`AncientGroundMaxAbsX` IS A `USiegeScatterConfig` FIELD. `ASiegeBattlefieldScatter` READS THE DATAASSET. ⇒ THE HEADER EDIT ABOVE IS ONLY HALF THE CHANGE, AND ⛔ TASK-569 OWNS THE OTHER HALF (its spec item 1b). IF TASK-569 DOES NOT LAND IT, THE RULING SHIPS INERT.**

📌 **AND A FINDING THAT SHARPENS TASK-569's STEP — declared under `SC-§15` because it contradicts a checkable line of my own spec (item 2 states flatly that the saved asset overrides this field):**

I inspected the saved packages by property-name presence, **with a positive control so the method is not a bare negative** (a `.uasset` writes only properties that DIFFER from the CDO, and each written property's name lands in the package name table as plain text):

```
grep -rla "<PropertyName>" Content/            # over Content/Data/DA_BattlefieldScatter.uasset
```

| property | in `DA_BattlefieldScatter`? | reading |
|---|---|---|
| `CastleKeepClearRadius` | ✅ **PRESENT** | **positive control** — this one IS overridden, exactly as `WR-§2` row 4 says. The method works. |
| `AncientGroundMaxAbsX` | ❌ absent | no override serialised |
| `AncientGroundMinAbsX` / `HalfExtent` / `MaxAbsY` / `MineClear` / `ClearRadius` | ❌ absent | **no `AncientGround*` property is overridden at all** |
| `ArenaHalfExtent`, `PlayerStartKeepClearRadius`, `MineClass` | ❌ absent | consistent |

**Corroboration:** the asset's last-saved timestamp is in the **CASTLE-3X window**, i.e. *before* the ANCIENT-GROUNDS batch added these fields — an asset cannot carry an override for a property that did not exist when it was saved.

⇒ ⚖️ **Expected reality: this header default is LIVE, and TASK-569's step (1b) is a VERIFY-AND-CLEAR, not a blind retype.** ⛔ **I did not weaken the instruction and neither should QA** — file-only evidence is evidence, not a verdict, and the editor is the only authority. **TASK-569: open `DA_BattlefieldScatter`, look at the `AncientGroundMaxAbsX` row; if it shows an override marker, reset it (or set it to `16080`); read the value back and report it either way.** `CastleKeepClearRadius` (1a) is unaffected — that one demonstrably IS overridden and MUST be edited.

⚠️ **BOARD NIT, worth two lines because a missing path makes a smaller commit silently:** TASK-569's `names:` block still reads `/Game/Data/DA_BattlefieldScatter (CastleKeepClearRadius → 4500)` — **one field, while its own spec item (1) says "TWO FIELDS NOW".** The `names:` line has not caught up with the amendment.

---

## 3. ⚠️ THE NARROWED BAND — SANITY-CHECKED AND REPORTED, NOT JUST TYPED (spec item 3)

**Band per side: `|X| ∈ [4000, 16080]`, `|Y| ≤ 10800`** (was `|X| ∈ [4000, 21000]`).

| | before | after |
|---|---|---|
| X width | 17,000 | **12,080** (**71.06 %** retained) |
| draw-band area (one half) | 367,200,000 uu² | **260,928,000 uu²** |

**Does the two-draw, 48-attempt loop still have room? Yes, with an enormous margin — the two rejection tests are LOCAL and roughly uniform in X, so narrowing the band barely moves the per-attempt acceptance probability, it only moves the area.**

1. **Mine clearance (`AncientGroundMineClear` 1800), the one I can bound exactly.** `MineCountPerSide = 3`. The three Red twins can never bind: a Blue candidate has X ≤ −4,000 and a Red mine has X > 0, so the separation exceeds 1,800 by construction — and the P′ test against the Red set is the mirror of the P test against the Blue set. ⇒ at most **3 discs of r = 1,800** bite: 3 × π × 1800² = **30,536,281 uu² = 11.70 %** of the band, and that is an over-count (a mine may sit outside the band entirely).
2. **Flat-ground-only (hills rejected outright).** The hill layer comes from the DataAsset, so I do not quote a count — but hill density per unit area is **unchanged** by this edit, and the strip that was removed (`|X| ∈ (16080, 21000]`) carried its own share of hills and mines, so it removed exclusion area along with legal area.
3. **The loop's failure probability.** With acceptance probability *p*, failure = (1−*p*)⁴⁸. Even at a wildly pessimistic *p* = 0.48 (mine discs plus **40 %** of the field under hills) that is **≈2.3 × 10⁻¹⁴**; at an absurd *p* = 0.2 it is still ≈2.2 × 10⁻⁵. **The deterministic-fallback Error line is not going to start firing.**
4. ✅ **The deterministic fallback slot survives the narrowing — I checked, because it is a LITERAL and is deliberately NOT clamped into the configured band.** `(−12000, +6000)`: |X| 12,000 ≤ 16,080 ✓ and |Y| 6,000 ≤ 10,800 ✓ ⇒ still **inside** the legal band. Had the ruling gone below 12,000 the fallback would have landed outside its own band; at 16,080 it does not.
5. ✅ **The mis-tuned-DataAsset guard stays a no-op**, with more headroom than before: `Min(16080, 26000 − 840 = 25160) = 16080` and `Min(10800, 11160) = 10800`.

⇒ ⚖️ **I do NOT believe the band is too tight, so `SC-§15` does not fire here — this is a reported result, not a departure.**

---

## 4. ✅ SEED DISCIPLINE — CHECKED, AND I STATE IT PLAINLY (spec item 4)

- The dedicated stream `FRandomStream GroundStream(Seed ^ 0x41474E44)` is **untouched**.
- **Exactly two draws per attempt, in fixed X-then-Y order** — untouched. I changed the **VALUE of a bound passed to `FRandRange`**, never the number of draws, never their order, never their source stream. Everything downstream stays draw-free.
- **Host/client:** the ancient-ground pass runs on **both** machines from the replicated seed, and both read the same config artifact ⇒ both compute the same `MaxAbsX` ⇒ **bit-identical layout**. No draw was added, removed or reordered on either side, and no other pass's stream moved.
- ⚠️ **Existing seeds WILL produce a different ancient-ground location.** That is the documented, expected consequence of a bound change (the "Ancient Grounds" clause's own precedent: intra-build reproducibility + host==client is the contract, cross-build layout stability never was). **Layer and mine layouts are completely unaffected** — separate streams, zero draws moved.
- **The `CastleQueryInset` half adds ZERO draws of any kind** (`GetActorBounds` is a measurement) and, decisively, **`ValidateTraversability` and its destructive culls are AUTHORITY-ONLY**: `RunScatterPasses` returns at `if (!bAuthoritativeGenerate)` **before** `StartNavSettlePoll`, logging *"Client passes complete … nav validation/culls are authority-only"*. ⇒ a client never runs the path query, so a state-dependent inset cannot desync anything. The settled-only determinism switch (`bCullOnProvisionalFailure = false`) is untouched.

---

## 5. ⚖️ `CastleQueryInset` — RE-DERIVED, NOT MULTIPLIED (spec items 5 + 6 + 7)

### 5a. The diagnosis, with the arithmetic that rules out the cheap fix

Its own retired doc — *"≈1200 clears a ~810-unit castle footprint"* — is the proof it is castle-derived: a ~810-uu footprint is a **~405** half-extent, so the author's real quantity was **1,200 − 405 = 795 uu of pad past the wall face.**

| geometry | castle half-depth X | flat 1,200 inset lands | verdict |
|---|---|---|---|
| the castle it was authored for | ≈405 | 795 uu **past** the face | ✅ as designed |
| CASTLE-3X | 1,218.95 | **18.95 uu INSIDE** | ⛔ already the false-negative it exists to prevent — **stale since CASTLE-3X** |
| 9× | 3,656.85 | **2,456.85 uu INSIDE** | ⛔⛔ both endpoints inside the nav-carved keep |
| 9×, naive ×3 (3,600) | 3,656.85 | **56.85 uu INSIDE** | ⛔ **the ×3 does NOT fix it** — this row is why `WR-§2b` bans multipliers |

⚖️ **Ruling `W2-R3`'s rule applied: it rotted at CASTLE-3X, not here — and a batch that makes a latent defect three times worse owns it.**

### 5b. The repair — a band past the measured wall face, the authored literal demoted to a floor

**Shipped shape (`BattlefieldScatter.h`):**
- `CastleQueryInset = 1200.f` — ⛔ **value UNCHANGED on purpose**; it is now the **authored floor + the degenerate-bounds fallback** (spec item 7's "today's behaviour").
- `CastleQueryFacePad = 795.f` — **NEW**, the live tunable: how far past the measured colliding wall face the endpoint sits. ⭐ **795 is not a new number — it is the original author's pad, recovered**, exactly the W2-R2 / row-B pattern (*restore the human-chosen quantity; a multiplier preserves an accident*).

**`ASiegeBattlefieldScatter::ResolveCastleQueryInset(ETeamId, const FVector& CastleLocation)` (`BattlefieldScatter.cpp`), called once per team from `ValidateTraversability`:**

```
FaceDistance = (BoundsOrigin.X − CastleLocation.X) · TowardCenterline + BoxExtent.X   // GetActorBounds(bOnlyCollidingComponents = true)
Pad          = (KeepClearRadius − FaceDistance > 0) ? Min(FacePad, KeepClearRadius − FaceDistance) : FacePad
Inset        = Max(CastleQueryInset, FaceDistance + Pad)                              // authored value = FLOOR
if (KeepClearRadius > FaceDistance)  Inset = Min(Inset, KeepClearRadius)              // ... and the floor never pushes the pad out of the disc
```

**What it resolves to, at every geometry this project has had:**

| geometry | `FaceDistance` | derived inset | endpoint \|X\| | outside the footprint? | inside the keep-clear disc? |
|---|---|---|---|---|---|
| ~810-uu castle | 405 | **1,200.00** | 23,800 | ✅ by 795 | ✅ |
| CASTLE-3X | 1,218.95 | **2,013.95** | 22,986 | ✅ by 795 | ✅ |
| **9× (shipped)** | **3,656.85** | **4,451.85** | **20,548.15** | ✅ **by 795** (footprint starts at 21,343.15) | ✅ **by 48.15** under the live 4,500 disc |

⭐ **At the castle the constant was written for, the derivation reproduces `1200` EXACTLY (405 + 795).** ⇒ the repair is behaviour-preserving where the old value was correct and only diverges where it was already broken.
⚖️ **The honest band the spec named — `> 3,657` and `< 4,500` — is satisfied: 4,451.85.** ⚠️ **The headroom under the disc is only 48.15 uu**, which is precisely why the pad is **capped** at the room the disc leaves rather than trusted: another castle growth, or a smaller radius, silently squeezes it and the cap absorbs that.

**Why the cap is CONDITIONAL, and this is the one judgement call in the file:** until TASK-569 lands `4500` in the DataAsset, `CastleKeepClearRadius` reads **1,500 — narrower than the castle itself**. An unconditional `Min(Inset, KeepClearRadius)` would then drive the endpoint **back inside the footprint**, i.e. re-create the defect while looking like a safety clamp. So the cap is skipped whenever the disc is narrower than the footprint: **clearing the wall face outranks sitting in the disc**, because outside-the-disc merely *risks* a stray blocker (and the path query projects to the navmesh anyway) while inside-the-footprint is a *certain* false negative. ⇒ ✅ **the repair is correct BOTH BEFORE AND AFTER TASK-569's asset edit; that edit upgrades it from "outside the footprint" to "outside the footprint AND on guaranteed-clear pad".**

### 5c. ⛔ THE OVERRIDE-PATH DIAGNOSIS FOR `CastleQueryInset` — DONE FIRST, AND IT DOES **NOT** MIRROR §2 (spec item 6)

`CastleQueryInset` is `EditDefaultsOnly` on an **ACTOR** (`ASiegeBattlefieldScatter`) — ⛔ **not** a `USiegeScatterConfig` DataAsset field — so the DataAsset cannot own it. The live question was the **BP subclass**, and there is one: **`Content/Blueprints/BP_BattlefieldScatter.uasset`** (referenced by `L_Arena.umap`).

```
grep -rla "SiegeBattlefieldScatter" Content/   →  Content/Blueprints/BP_BattlefieldScatter.uasset, Content/Maps/L_Arena.umap
grep -rla "CastleQueryInset"        Content/   →  (0 hits)
```

Same name-table method as §2, again **with positive controls inside the very same package**: `SiegeBattlefieldScatter`, `ScatterConfig` and `SimpleConstructionScript` are **PRESENT** in `BP_BattlefieldScatter.uasset` (`ScatterConfig` being the DataAsset pointer the BP genuinely does override), while `CastleQueryInset`, `MaxNavSettleWait`, `CorridorWidenStep`, `MaxPlacementAttemptsPerInstance` and `bCullOnProvisionalFailure` are all **absent** — i.e. the BP overrides *some* parent defaults and this is not one of them.

⇒ ✅ **NO BP class-default shadows `CastleQueryInset`. Nothing shadows it anywhere in `Content/`. The header default is live, and a C++ repair is sufficient — TASK-569 owes NOTHING on this constant.** ✅ **A check that finds nothing is a RESULT** (`SC-§22`) — and it is also the answer to *"is the runtime derivation the only safe repair?"*: it is the **preferred** one (`WR-§2b`'s governing principle), not the forced one.

### 5d. ⛔ NULL-SAFE, AND NEVER A ZERO INSET (spec item 7)

Two fallback branches, both returning **the authored literals** (today's behaviour) and both routed through **one shared warn-once lambda** (`bWarnedCastleInsetFallback`, the `bWarnedNoNav` idiom already in the file):
1. **no live `ACastle` for that team** — `ResolveCastleLocation` took its ±25,000 fallback too, so there is no geometry to measure;
2. **degenerate colliding bounds** — guarded as `!(FaceDistance > 1.f)`, which also catches NaN, a zero extent, and a pivot offset that swallows the extent.

⛔ **Never zero:** the fallback is `Max(CastleQueryInset, CastleQueryFacePad)`, so **a single mis-typed `0` in either field still cannot produce a centre-of-castle endpoint**; and on every healthy path `Inset > FaceDistance > 0` by construction. ⚠️ **Residual, declared rather than hidden:** a designer who zeroes **both** `EditDefaultsOnly` fields gets a zero inset. I did **not** raise the `ClampMin` metadata to enforce that — changing a designer-facing clamp is a data decision I was not asked to make, and both doc blocks now say in terms that a non-zero value is load-bearing. **If QA wants `ClampMin = "1"` on both, that is a one-line ruling and I will take it.**

---

## 6. ⛔ `SC-§33` — THE PASTED, ENUMERATED CALL-SITE GREP. **IT DOES NOT FIRE, AND HERE IS THE MECHANICAL PROOF**

`SC-§33` binds *a **trailing defaulted** parameter added to a function that **already has call sites***. I added **no defaulted parameter to any existing function**. Commands run from `Source/`, raw hit counts pasted, one line per hit:

```
G1  grep -rn "ResolveCastleLocation\|ResolveCastleActor\|ResolveCastleQueryInset" .      → 17 hits
G2  grep -rnE "float (ResolveCastleQueryInset|ResolveCastleActor)\(.*=.*\)" .            →  0 hits   (no default-argument syntax introduced)
G3  grep -rn "CastleQueryInset\|CastleQueryFacePad" .                                    → 16 hits
G4  grep -rn "AncientGroundMaxAbsX" .                                                    →  4 hits
G5  grep -rn "PlaceAncientGrounds(" .                                                    →  4 hits
```

**G1 — the three functions, classified (17):**
| hit | classification |
|---|---|
| `BattlefieldScatter.cpp:2070,2071` — `ResolveCastleLocation(Blue/Red)` | **(ii) UNCHANGED CALL SITES.** Signature untouched; behaviour byte-identical (it now delegates). |
| `BattlefieldScatter.h:556` + `.cpp:2599` — `ResolveCastleLocation` decl/def | **(i) body re-pointed at the new accessor; signature, fallback and first-match semantics identical.** |
| `BattlefieldScatter.h:553` + `.cpp:2578` — `ResolveCastleActor` decl/def | **NEW function, ONE required parameter.** `SC-§33` explicitly does not bind a brand-new function. |
| `BattlefieldScatter.cpp:2601, 2633` — the two `ResolveCastleActor` calls | **(i) both new, both in this task's diff.** |
| `BattlefieldScatter.h:569` + `.cpp:2609` — `ResolveCastleQueryInset` decl/def | **NEW function, TWO required parameters, zero defaults.** |
| `BattlefieldScatter.cpp:2072,2073` — its two calls | **(i) both new; both pass both arguments** (the instance-1/instance-2 failure mode cannot exist without a default). |
| `BattlefieldScatter.h:548,550`, `.cpp:2580,2583,2636` | comment references only. |

**G3 — the two tunables (16):** `.h:749` / `.h:777` = the two declarations · `.cpp:2617,2618` = the two reads inside the derivation · `.cpp:2072,2073,2086` = the call site + the new Log line · `.cpp:2629` = the fallback Warning · remaining 7 = doc/comment text. ⇒ **every read of `CastleQueryInset` now goes through `ResolveCastleQueryInset`; there is no surviving direct use of it as the inset.**
**G4 — `AncientGroundMaxAbsX` (4):** `ScatterConfig.h:546` (the value, mine) · `BattlefieldScatter.cpp:1701` (the only consumer — reads the config field by name, so it needed **no** edit: `SC-§34`'s structural escape already working) · `BattlefieldScatter.h:439` (documents the band **by symbol name**, so it did not rot) · **`Castle.h:362` — STALE, and NOT MINE ⇒ see §7.**
**G5 — `PlaceAncientGrounds` (4):** definition, declaration, its one call in `RunScatterPasses`, one comment. **Signature untouched** (it already carries the two-arg form ruled at TASK-365 R2).

✅ **A sweep that finds every call site already correct is a RESULT and is reported as one.** The gate re-runs these five commands.

---

## 7. `SC-§22` — WHAT ELSE THE SWEEP TURNED UP (and it is one row, named to its owner)

| # | site | finding | mark |
|---|---|---|---|
| **R1** | **`Castle.h:362`** (`SpawnBoxHalfExtent`'s doc block) | Quotes *"`USiegeScatterConfig::AncientGroundMaxAbsX` (21,000) — that constant is left at its law value and FLAGGED to the manager"*. **Ruling `W2-R1` has now been made and the value is 16,080** ⇒ the sentence is doubly false (wrong number, wrong status). | **(iii) → TASK-562**, which owns `Castle.{h,cpp}` in this wave. ⛔ Row G's rule: fixed by the task already in the file, never by a drive-by. |
| **R2** | `BattlefieldScatter.h:439` (`PlaceAncientGrounds` doc) | States the band **by symbol name** (`|X| in [AncientGroundMinAbsX, AncientGroundMaxAbsX]`) ⇒ **could not rot.** | **(ii) no edit.** Recorded because it is the positive example of `SC-§34`'s structural escape and reads as an omission otherwise. |
| **R3** | `BattlefieldScatter.cpp` `PlaceAncientGrounds` band comment | Quoted `21,000` and the retired `22,540` as absolutes, plus the retired `≥ 23,500` disc figure. **My file** ⇒ fixed here, in the same edit, with the re-derivation and the *"measure against the live `SpawnBoxHalfExtent`"* instruction replacing the transcription. | **(i) done.** |
| **R4** | `BattlefieldScatter.cpp:1163` | *"the ≈(−23800,0) fallback (M7.6: 1200 in front of `Castle_Blue`)"* — still **literally true** (it is 1,200 in front of the castle **centre**). It is the `PlayerStart` whose relationship to the **footprint** broke, which is TASK-573's row. | **(ii) no edit**, so my silence is not read as a miss. |

---

## 8. 📌 M8 DECLARATION — DECLARED, NOT COPIED

⛔ **No replicated property · no new replicated class · no new relevancy tier · no RPC.** The two new members are **plain `bool` log-once guards** (not `UPROPERTY`, not replicated, pure local diagnostics); the one new `UPROPERTY` (`CastleQueryFacePad`) is `EditDefaultsOnly` **design-time data, identical on both machines by construction**; `AncientGroundMaxAbsX` is likewise a config default. `GetLifetimeReplicatedProps` is untouched. ⚠️ **The scatter is seed-replicated — the network-relevant reasoning is §4**, and its two load-bearing facts are that the ancient-ground pass moves **zero draws**, and that the traversability query/culls are **authority-only**.

---

## 9. ⛔ WHAT A HUMAN SHOULD OBSERVE — MY ACCEPTANCE INSTRUMENT (no unit test is owed; TASK-564 correctly boards none)

⚖️ **A test here would assert `GetActorBounds`, not our logic.** TASK-569's rows **(n)(o)(p)** belong to TASK-573/574/575; **wave 2 boards no row for mine**, so here is exactly what to watch, with the numbers to compare against. **Report each as an observation, never a conclusion.**

**A. The new grep-able line, printed once per actor lifetime (`LogSiegeTerrain`, at PIE):**
```
[BattlefieldScatter 'BattlefieldScatter'] CastleQueryInset derived: Blue <I> -> endpoint X <XB>, Red <I> -> endpoint X <XR> (authored floor 1200 + face pad 795).
```
- ✅ **PASS:** `<I> ≈ 4452` for both teams, `<XB> ≈ −20548`, `<XR> ≈ +20548`.
- ⛔ **The single number that matters: the inset must EXCEED the castle half-depth (≈3,657).** `<I> = 1200` means the derivation fell back — and then a `CastleQueryInset could not be derived …` **Warning** will say which of the two reasons fired. **Report both lines verbatim.**
- ⚠️ If `<I>` sits at exactly `4500`, the disc cap bit — expected only if the castle grew again.

**B. The traversability verdict still prints and the field is NOT culled for nothing.** Grep `CONFIRMED (nav settled:` (⛔ the bare string `Traversability CONFIRMED` no longer exists — `NAV-§4`/`NAV-§12`). The pre-fix hazard is a **false negative**: endpoints buried inside the keep make the Blue→Red query fail, which fires the **widening corridor cull** and **DELETES scatter instances on a perfectly walkable field** (visible pop). ⇒ ✅ watch for **no `DiscCull`/`CullCorridorBlockers` lines from the traversability path** and `ReachabilityAttempt` staying at 0.

**C. The ancient grounds — one line, one number.** `AncientGroundsPass seed=… P=(x,y,z) M=(…) fb=no culls=…`
- ✅ **`|P.X| ≤ 16080`** (and `|P.Y| ≤ 10800`). ⛔ **A value in `(16080, 21000]` means the DataAsset is still overriding the header — that is TASK-569 item (1b) not landed, i.e. the change shipped INERT.**
- ✅ **`fb=no`** on a healthy field (§3 says the fallback should be effectively unreachable). `fb=yes` recurring across seeds is a real signal — report it.
- 👁️ **The eyeball test, and it is the one that shows the ruling working:** the jade rune square must sit **in front of** the spawn area, not under it. ⛔ **Units materialising at match start should never be standing ON the runes** — that inversion is the whole defect `W2-R1` repairs.
- 🔁 **Play Again ×3:** a fresh `P` each time, still inside the band, still `fb=no`.

---

## 10. ⚠️ DEPARTURES AND JUDGEMENT CALLS, DECLARED (`SC-§15`) — ⛔ ALL FOUR ARE MINE TO OWN AND QA'S TO RULE ON

1. **I ADDED a second tunable (`CastleQueryFacePad`) instead of REPURPOSING `CastleQueryInset` in place.** Rows B/C/W2-R2 repurpose the existing tunable into a band — but **spec item (7) requires the authored literal to survive as the degenerate-bounds fallback ("today's behaviour")**, and a repurposed field cannot be both the band and the fallback. Repurposing would also have made the *name* lie (it is no longer the inset). ⛔ **I did not rename `CastleQueryInset`** — a rename is not in my `names:` block.
2. **I refactored `ResolveCastleLocation` to delegate to a new `ResolveCastleActor`.** Behaviour is byte-identical (same iteration, same `IsValid` + team test, same first-match, same ±25,000 fallback). The reason is structural: the endpoint's **location** and its **bounds** must come from the **same actor**, not from two independent iterations a duplicate or mis-teamed castle could split. Both functions are private and confined to my file.
3. **My derivation includes a bounds-origin-vs-pivot term that the cited model omits.** `ASiegeGameMode::ResolveHeroStart` branch 3 uses `BoxExtent.X` alone; I use `(Origin.X − Location.X)·Toward + Extent.X`, which is exact when the bounds centre does not sit on the actor pivot. **Strictly stronger, never weaker** (it reduces to branch 3's form when the pivot is centred) — declared because it differs from the model `WR-§2b` names.
4. **§2's finding contradicts my own spec item (2)** on whether the saved DataAsset overrides `AncientGroundMaxAbsX`. **I kept the instruction and sharpened the step to VERIFY-AND-CLEAR** rather than acting on file-only evidence.

---

## 11. WHAT QA (TASK-565) SHOULD SCRUTINISE HARDEST

1. ⭐ **The conditional disc cap (§5b).** It is the one branch where I chose an ordering — *clear the wall face first, sit inside the disc second* — and it is what keeps the repair correct in the window **before** TASK-569 edits the DataAsset (where `CastleKeepClearRadius` reads 1,500, i.e. **narrower than the castle**). **Convince yourself an unconditional `Min(Inset, KeepClearRadius)` would re-create the defect.**
2. ⛔ **The invariant `Inset > FaceDistance` on every branch.** Walk all four: healthy-uncapped, healthy-capped, no-castle, degenerate-bounds. If any path can put the endpoint back inside the footprint, the whole task failed.
3. **Re-derive `795` yourself** from *"≈1200 clears a ~810-unit castle footprint"*, and check my claim that `405 + 795 = 1200` makes the change **behaviour-preserving at the geometry the constant was authored for**. If you think the recovered pad should instead be the 9× band's midpoint (≈4,078), say so — I judged "restore the human-chosen quantity" (W2-R2) to outrank "maximise margin".
4. **The 48.15 uu of headroom under the 4,500 disc.** That is tight. Is the cap the right response, or should the pad have been reduced?
5. **Re-run my five greps (§6) and the two package name-table probes (§2, §5c) — including the POSITIVE CONTROLS**, which are what make an absent-string result meaningful rather than a bare negative.
6. **Item 3's band argument.** I claim the narrowed band cannot starve the 48-attempt loop. Pressure-test the mine-disc bound and the assumption that hill density is X-uniform.
7. **§10 row 1** — second tunable vs repurpose. If you rule "repurpose", it is a rename plus a fallback redesign and I need that said explicitly.
8. ⛔ **Confirm §2 is loud enough**: `AncientGroundMaxAbsX` is half-landed by design and **TASK-569 owns the other half** — and that TASK-569's `names:` block is one field short of its own spec.

---

## 12. NOT DONE, BY DESIGN

⛔ No compile · no editor / MCP / PIE · no Git · no `Content/` asset (⛔ **`DA_BattlefieldScatter` NOT touched — TASK-569 owns it**) · no `.csv` · no `Tests/` · no `Build.cs` · no `L_Arena` · **no new trailing defaulted parameter** (§6) · **no token figure quoted** · **no navmesh claim** (`WR-§3` — whether the interior is navigable is TASK-569's *measurement*, and I predicted nothing about it) · ⛔ **`CastleKeepClearRadius` READ ONLY, exactly as instructed** · ⛔ **no edit to `ArenaHalfExtent`, `AncientGroundHalfExtent`, `AncientGroundMinAbsX`, `AncientGroundMaxAbsY`, the mine constants, or the `|Y| ≤ 1000` corridor ruling.**
