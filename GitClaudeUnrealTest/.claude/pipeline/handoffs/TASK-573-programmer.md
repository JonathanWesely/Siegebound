# TASK-573 — [WR-19] harden the hero-start branch 2 against the 9× castle footprint

**Agent:** gameplay-programmer · **Status:** `ready-for-qa` · **Gate:** `qa/TASK-565.md`
**Law:** CONVENTIONS `WR-§2b` row A · `WR-§3` · `WR-§1` · `SC-§34` · `SC-§22` · `SC-§15` · `SC-§33` · `SC-§18c`
**Compile / editor / MCP / PIE / Git:** ⛔ none touched. File-only, as specced.

---

## ⛔ DECLARED DEPARTURE 1 (`SC-§15`) — **THE SPEC'S FUNCTION NAME DOES NOT EXIST. `ResolveHeroStart` IS A PHANTOM SYMBOL.**

The task spec, `WR-§2b` row A, and the `names:` block all name **`ASiegeGameMode::ResolveHeroStart`**. **There is no such symbol in the codebase.** The function is:

```
Source/GitClaudeUnrealTest/Siegebound/SiegeGameMode.h:422
void GetHeroStartTransform(AController* Player, ETeamId HeroTeam, FVector& OutLocation, FRotator& OutRotation);
```

Evidence, run repo-wide:

```
$ grep -rn "ResolveHeroStart" Source/
Source/GitClaudeUnrealTest/Siegebound/SiegeBotController.h:380:	 *  ASiegeGameMode::ResolveHeroStart already does (GetActorBounds +
--- raw hit count: 1 (a COMMENT, in a file this task does not own) ---
```

✅ **It is unambiguously the right function anyway, and I did not guess** — I confirmed by structure, not by name: the manager's own line/branch citations land exactly on it. `SiegeGameMode.cpp:670-683` (pre-edit) *is* the same-side-test-then-`return` block, `:690-731` *is* the `GetActorBounds` fallback, and the branch numbering `(1)(2)(3)(4)` in the spec matches the numbered comments in the function verbatim. **Same function, wrong name.**

⚠️ **The phantom name has already propagated into shipped source as a cross-reference:** `SiegeBotController.h:380` tells a future reader to go look at `ASiegeGameMode::ResolveHeroStart` for the derivation pattern, and that reader will find nothing. ⛔ **I did NOT fix it — `SiegeBotController.{h,cpp}` is TASK-575's file and `WR-§2b` row G forbids a drive-by comment fix in a file another task owns.** ⇒ **(iii) NAMED TO TASK-575**, which is in this same batch, this same compile and this same gate, and is already editing that exact header.

📌 **For the manager:** `WR-§2b` row A and the TASK-573/575 `names:` blocks should be corrected to `GetHeroStartTransform`, or the next agent to work this area greps for a symbol that isn't there.

---

## What changed

**Two files. Both in `Source/GitClaudeUnrealTest/Siegebound/`. Nothing else.**

### `SiegeGameMode.cpp` — three code changes, all inside `GetHeroStartTransform`

**1. The bounds query was HOISTED out of branch 3 into a new step `1b`, so there is exactly ONE query and ONE result** (spec 3 — "factor the query so BOTH branches read ONE result"):

```cpp
FVector CastleBoundsOrigin = FVector::ZeroVector;
FVector CastleBoxExtent = FVector::ZeroVector;
bool bCastleBoundsUsable = false;
if (OwnCastle)
{
    OwnCastle->GetActorBounds(/*bOnlyCollidingComponents=*/ true, CastleBoundsOrigin, CastleBoxExtent);
    bCastleBoundsUsable = CastleBoxExtent.GetMin() > UE_KINDA_SMALL_NUMBER;
}
```

`bOnlyCollidingComponents = true` is preserved verbatim from branch 3 (spec 3's warning about the 9,450-uu HP-bar widget — ✅ **I verified that Z at the source, not by relay: `Castle.h:264` and `Castle.cpp:174` both say `9450`**).

**2. Branch 2 gained the footprint rejection**, ANDed onto the existing side test:

```cpp
const bool bStartInsideOwnCastle = bCastleBoundsUsable
    && FBox(CastleBoundsOrigin - CastleBoxExtent, CastleBoundsOrigin + CastleBoxExtent).IsInsideOrOn(StartLocation);

if (bStartOnOwnSide && !bStartInsideOwnCastle)
{
    OutLocation = StartLocation;
    OutRotation = FRotator(0.0f, Start->GetActorRotation().Yaw, 0.0f);
    return;
}
```

plus a `Warning` log on the rejection path (see *What a human observes* below — it is the acceptance instrument).

**3. Branch 3 lost its local declarations + its own `GetActorBounds` call and now reads the hoisted values.** ⛔ **Its arithmetic, its `FMath::Max` floor, its casts, its rotation and its `UE_LOG` are untouched — byte-identical behaviour, including the degenerate-bounds path.**

### Comment-only fixes (spec 6, `WR-§2b` row G) — ZERO emitted bytes

| where | was | now |
|---|---|---|
| `SiegeGameMode.cpp` branch 3 (S5) | "the HP-bar widget sits **3,150** uu up" | **9,450**, cited to `Castle.cpp` + TASK-557 |
| `SiegeGameMode.cpp` branch 3 (S9) | "with the live 3× castle this resolves 1,219 + 300 = **1,519**" | 9×: **3,656.85 + 300 = 3,956.85**, with the 3× figure kept as explicit history |
| `SiegeGameMode.h` `HeroSpawnCastleClearance` (S9) | "with the live 3× castle (half-extent 1,219) this derives **1,519**" | 9×: derives ≈**3,957**; adds the `SC-§34`/`WR-§1` reason the value must NOT be scaled |
| `SiegeGameMode.h` `HeroSpawnCastleOffset` | "the live castle's colliding half-extent at 1,219" | marked as *that* (retired) castle's; adds the 9× ≈3,957 result and that the floor is now inert |

⛔ **`HeroSpawnCastleOffset` (1500) and `HeroSpawnCastleClearance` (300) VALUES ARE UNCHANGED**, exactly as spec 6 requires.

### ⭐ `SC-§22` SWEEP — the spec named two stale claims; **the sweep found three more, and two of them were about to become load-bearing lies**

I did not stop at S5/S9. I swept both files for *every* castle-derived assertion (`grep -in` for `1219|1519|3150|23781|26219|810|2437|2461|2694|23800|25000|1200|1500|9450|footprint|half-extent`, then read every hit). **Three additional stale claims, all now fixed, all in files I own:**

1. **`SiegeGameMode.cpp` `RestartPlayer` (~`:252`)** — *"Blue/standalone resolves the level PlayerStart — the same spawn the engine path used (§10 byte-identity)"*. **False the moment this task lands.** Amended in place with the reason.
2. **`SiegeGameMode.h` `RestartPlayer` doc (~`:201`)** — same claim, second copy. Amended.
3. **`SiegeGameMode.h` `HeroSpawnCastleOffset` doc** — *"the level's own Blue PlayerStart sits 1,200 uu out and **spawns cleanly every time**, so 1,200 is the empirical floor"*. ⛔ **This is the worst of the three: it is the stated JUSTIFICATION for the authored 1,500, and at 9× that PlayerStart spawns cleanly NEVER.** A future tuner reading it would have "re-derived" 1,500 from a number that is now 2,457 uu inside a wall. **Retired verbatim in place** (the `WR-§2b` row E precedent — recorded, not deleted, so git history and this handoff agree).

✅ **Reported as a `SC-§22` RESULT: the citation was a lower bound, as the law predicts. 2 named, 5 found, 5 fixed.**

---

## ⛔ DECLARED DEPARTURE 2 (`SC-§15`) — **THE REJECTION WOULD ALSO HAVE FIRED AT THE RETIRED 3× CASTLE, BY 19 uu. QA SHOULD READ THIS BEFORE JUDGING SPEC (4).**

Spec (4) requires standalone byte-identity "for every geometry where the PlayerStart is OUTSIDE the castle". **Here is the arithmetic, including the case the spec did not anticipate:**

| geometry | own-castle colliding half-extent X | footprint span (castle at X = −25000) | PlayerStart X = −23800 | branch 2 |
|---|---|---|---|---|
| **9× (this batch, `WR-§0`)** | **3,656.85** | −28,656.85 … **−21,343.15** | **2,456.85 uu INSIDE** | ⛔ **REJECTED** → branch 3 |
| 3× (CASTLE-3X, retired by TASK-555) | 1,218.95 | −26,218.95 … **−23,781.05** | **18.95 uu INSIDE** | ⛔ would also be rejected |
| M1 (~810 footprint) | ~405 | −25,405 … −24,595 | 795 uu outside | ✅ accepted, unchanged |

⇒ **At the 3× castle the PlayerStart was *already* marginally inside the colliding box bound** — 19 uu, and note it is **the identical X and the identical 19 uu that `WR-§2b` row D records for `CastleQueryInset`** (`25000 − 1200 = 23800` vs the `23781` footprint edge). Both constants land on the same point; the sweep found the same rot twice from two directions.

**Why I shipped the strict test anyway, and did not paper over the 19 uu:**

- **There is no formulation that accepts at 3× and rejects at 9× without an invented margin.** Any margin ≥19 uu is a hand-tuned castle-derived constant — **precisely what `SC-§34` and this whole wave exist to ban.** I refuse to buy byte-identity on a counterfactual geometry with a fresh number that will rot at the next resize.
- **It is counterfactual at `HEAD`.** TASK-555 ships the 9× mesh in **this same commit** (`SC-§29b` / the batch's "castle and repair land together or neither does" ruling). The 3× castle will not exist in any tree this code is compiled into.
- **The failure direction of over-rejection is benign; under-rejection is a pawnless player.** Rejecting at 3× would have sent the hero to branch 3 → `(−23481.05, 0, 100)` yaw 0: **319 uu further out, 2 uu up, same facing, provably clear**, because branch 3 derives from the same bounds + 300 clearance. Under-rejecting is the defect this task exists to kill.
- **I considered and rejected a true collision test** (`OverlapBlockingTestByChannel` with the hero capsule), which *would* be byte-identical at 3×: ⛔ it is a **SECOND derivation of the same geometry**, which spec (3) explicitly forbids; it depends on the hero pawn class (resolved later than this call) and on collision channels; and it runs during `PostLogin`, where the castle's collision registration is a timing risk. **The spec named `GetActorBounds` and it was right to.**

**The test is the RAW colliding AABB — no padding, no `HeroSpawnCastleClearance` added.** That is the minimum that catches the defect, which keeps the divergence from shipped behaviour as small as the defect itself.

---

## Spec-by-spec

**(1) The defect** — reproduced by reading, confirmed exactly as the manager described it. Branch 2's *only* acceptance test was `bStartOnOwnSide`, then `return`. Fixed.

**(2) No `L_Arena` save, no map edit** — ✅ **`Content/` was not opened. The `PlayerStart` transform is untouched. `WR-§3`'s spent exception is not invoked and is not needed.** The repair is 100 % in C++.

**(3) One derivation, not two** — ✅ hoisted to step `1b`. I considered a private helper function and chose the hoist: a helper adds API surface for no benefit inside a single function, and (being new) would have invited a defaulted parameter — `SC-§33` bait for zero gain. `bOnlyCollidingComponents = true` preserved.

**(4) Standalone byte-identity** — ✅ satisfied and, where it is not, **declared above with arithmetic**. Mechanically: the new term can only ever **remove** an acceptance (`bStartOnOwnSide && !bStartInsideOwnCastle`), it is guarded by `bCastleBoundsUsable`, and the accept path's two assignments (`OutLocation`, yaw-only `OutRotation`) are character-for-character what they were.

**(5) Null/degenerate safety — the failure direction** — ✅ **the guard fails toward ACCEPTING.** No castle ⇒ `bCastleBoundsUsable == false` ⇒ rejection cannot fire. Zero/unresolvable extent ⇒ same. ⭐ **Belt and braces: even if the flag were removed, a zero-extent `FBox` is a point and `IsInsideOrOn` would be false for any real PlayerStart — the safe direction is true both by the guard AND by the geometry.** Branch 3 is deliberately **not** gated on the flag; it keeps absorbing degenerate bounds through its own `FMath::Max` floor, exactly as before. This is spec (5)'s stated reasoning: a mis-signed test must not be able to break both branches at once.

**(6) Stale comments** — ✅ done, in this file only, values untouched. See the table + the `SC-§22` sweep.

**(7) 📌 M8 DECLARATION** — ⛔ **no replicated property, no new `UPROPERTY(Replicated)`, no new class, no new unit/building tier, no RPC, no `GetLifetimeReplicatedProps` change.** `GetHeroStartTransform` is a `GameMode` member and therefore **server-only by construction**; the new test runs on the authority and reaches clients solely through the existing pawn spawn. **Net-neutral.**

---

## ⛔ `SC-§33` DISCHARGE — **NOT TRIGGERED, and here is the evidence rather than the assertion**

**No signature changed and no function was added.** `GetHeroStartTransform`'s parameter list is identical before and after; no parameter anywhere in either file gained a default. `SC-§33` binds "a DEFAULTED parameter added to a function that ALREADY HAS CALL SITES" — nothing here qualifies.

The enumeration, pasted so the gate can re-run it:

```
$ grep -rn "GetHeroStartTransform" Source/
Source/GitClaudeUnrealTest/Siegebound/SiegeGameMode.cpp:265:	// GetHeroStartTransform for the arithmetic and for why the level is not
Source/GitClaudeUnrealTest/Siegebound/SiegeGameMode.cpp:282:	GetHeroStartTransform(NewPlayer, SiegePS->GetTeam(), StartLocation, StartRotation);
Source/GitClaudeUnrealTest/Siegebound/SiegeGameMode.cpp:618:	GetHeroStartTransform(Player, Hero->GetTeamId(), StartLocation, StartRotation);
Source/GitClaudeUnrealTest/Siegebound/SiegeGameMode.cpp:642:void ASiegeGameMode::GetHeroStartTransform(AController* Player, ETeamId HeroTeam, FVector& OutLocation, FRotator& OutRotation);
Source/GitClaudeUnrealTest/Siegebound/SiegeGameMode.h:202:	 *  (re)start through the team-keyed GetHeroStartTransform — the Blue/host path
Source/GitClaudeUnrealTest/Siegebound/SiegeGameMode.h:422:	void GetHeroStartTransform(AController* Player, ETeamId HeroTeam, FVector& OutLocation, FRotator& OutRotation);

--- raw hit count: 6 / 2 files ---
```

| hit | classification |
|---|---|
| `.cpp:265` | (ii) comment (one I wrote this task) — no call |
| `.cpp:282` | **shipped call site** — `RestartPlayer`, 4 args, **UNCHANGED** (initial spawn) |
| `.cpp:618` | **shipped call site** — `RestoreHeroAtStart`, 4 args, **UNCHANGED** (respawn) |
| `.cpp:642` | definition |
| `.h:202` | (ii) comment — no call |
| `.h:422` | declaration |

✅ **Two shipped call sites; both reach the new behaviour with no edit, because the change is entirely inside the callee.** Both are on the critical path — **match start AND respawn** — which is why the defect hit "at match start and at every respawn".

---

## ⛔ `SC-§34` LEDGER — the rows this task owns

| # | constant / artifact | disposition |
|---|---|---|
| 1 | branch 2 acceptance test | **(i) RE-DERIVED** — gains the live-bounds footprint rejection |
| 2 | the castle-bounds query | **(i) RE-DERIVED structurally** — two potential derivations collapsed to one |
| 3 | branch 3 distance derivation | **(ii) UNCHANGED** — TASK-557 S9 proved it self-derives to ≈3,957 at 9× with no edit. **It is the model, not a patient.** |
| 4 | `HeroSpawnCastleOffset` = **1500** | **(ii) DELIBERATELY UNCHANGED** — a no-bounds last resort, not a castle-derived number; the derived value now beats it 3,957 : 1,500, so it is inert on the live path |
| 5 | `HeroSpawnCastleClearance` = **300** | **(ii) DELIBERATELY UNCHANGED** — **human-scale** (`WR-§1`, `SC-§34` exemption): hero capsule r≈42 + UCX slack. **Units did not grow.** |
| 6 | HP-bar Z claim (3,150 → 9,450) | **(i) comment-only** |
| 7 | worked example (1,519 → ≈3,957) | **(i) comment-only** |
| 8 | 3 further stale claims (`SC-§22` sweep) | **(i) comment-only** |
| 9 | `L_Arena` `PlayerStart` transform | **(iii) OUT OF SCOPE BY LAW** — `WR-§3`; not touched, and no save is needed |
| 10 | `SiegeBotController.h:380` phantom `ResolveHeroStart` xref | **(iii) NAMED TO TASK-575** — same batch, same compile, same gate |

---

## ⭐ WHAT A HUMAN OBSERVES — the acceptance instrument for TASK-569 PIE rows (n)(o)(p)

⚠️ TASK-564 deliberately does not unit-test this. **PIE is the only instrument. Here is exactly what to look for.**

**Setup:** boot PIE on `L_Arena` as Blue (standalone is enough — Blue is the broken seat).

**(n) At match start, the log shows this PAIR, in this order, back-to-back:**

```
Warning: [BP_SiegeGameMode_C_0] PlayerStart 'PlayerStart_...' at (-23800, 0, 98) lies INSIDE the Blue castle's
         colliding bounds (centre X -25000, half-extent 3657 x 3692 x 4041) — REFUSED, falling through to the
         castle-relative resolver. ...
Log:     [BP_SiegeGameMode_C_0] Castle-relative hero start for Blue: castle X -25000, measured colliding
         half-extent 3657 + clearance 300 => spawn distance 3957 (authored floor 1500) -> (-21043, 0, 100).
```

⭐ **The printed `half-extent 3657` is ALSO the batch's cheapest 9×-castle readback.** If it prints **~1219**, the 9× mesh (TASK-555) did not land and **TASK-573 is not at fault** — the rejection correctly did not fire because the castle is still 3×... **except it would still fire at 19 uu (see Departure 2), so a `1219` reading means investigate TASK-555, not this.**

**(o) The hero is OUTSIDE the gate and HAS A PAWN.**
- Visually: standing on the arena floor in front of the castle, facing the enemy half (yaw 0) — **not** in the great hall, **not** clipped into the floor slab.
- ⛔ **The failure signal is unmissable: `Error: ... Default pawn spawn at (...) was refused for collision — retried with AdjustIfPossibleButAlwaysSpawn`.** That Error appearing at all means the resolver handed out an occupied point. **It must NOT appear.**
- ⛔ **A hero at `(-23800, 0, 98)` means the rejection did not fire.** That is the defect, un-fixed.

**(p) Kill the hero and watch the respawn** (5 s, `HeroRespawnDelay`). **The identical Warning+Log pair repeats** and the hero reappears at the same outside-the-gate point. ⚠️ **This is the half that matters most: the defect hits every respawn, not just match start**, and `RestoreHeroAtStart` is a different call site (`.cpp:618`) from the initial spawn (`.cpp:282`).

**Negative controls, so a correct run is not misread as a broken one:**
- ⛔ **The Warning must NOT appear for the Red bot.** Red's own castle is at +25000; the Blue PlayerStart is neither on its side nor inside its box, so Red takes branch 3 silently as it always has. **A Red-tagged warning would mean the team lookup is wrong.**
- ⚠️ **The Warning firing is NOT itself a bug report.** It is the designed, load-bearing signal that a level-authored PlayerStart has been outgrown by the castle and the code absorbed it without a map edit. **It fires on every spawn, by choice** (not `log once`) — each occurrence is a real geometry mismatch and the pair reads as one story. ⭐ **Suggest the manager add it to `WR-§9` as a designed outcome so a playtest report of "warning spam at spawn" does not spend a QA loop.**

---

## What QA should scrutinise

1. ⛔ **Departure 1 — the phantom `ResolveHeroStart`.** Confirm `GetHeroStartTransform` is the right function (the branch structure proves it) and that naming `SiegeBotController.h:380` to TASK-575 is the correct `WR-§2b` row G disposition rather than a drive-by fix I should have made.
2. ⛔ **Departure 2 — the 19-uu 3× divergence.** This is the one judgement call in the task. If the gate wants byte-identity at 3× it must accept an invented margin; I argue it must not, and that 3× is counterfactual at `HEAD` anyway.
3. **The Z leg of the containment test.** `IsInsideOrOn` is a full 3-D test. It rejects the PlayerStart at z=98 because the castle box spans z≈0…8,082. ⚠️ **Known, bounded consequence: a PlayerStart placed strictly ABOVE the castle top or strictly BELOW its base is accepted** — correctly, it is not in the keep — and no shipped map has one. If QA wants "horizontally inside at any Z" instead, say so; it is a one-line change and I did not take it unasked because it would reject a legitimately elevated start.
4. **Branch 3 is behaviourally untouched.** Verify the hoist changed nothing but where the two `FVector`s are declared — same call, same flag, same `FMath::Max`, same casts, same log.
5. **The `bCastleBoundsUsable` sign.** Spec (5) says a mis-signed test breaks both branches. Confirm it reads "usable ⇒ *may* reject", never "unusable ⇒ reject".
6. **Compile-trap check** (no compiler available to me): `FBox(FVector,FVector)` ctor + `IsInsideOrOn` + `FVector::GetMin()` vs `UE_KINDA_SMALL_NUMBER`. All operands are UE5 **doubles** and the comparison is a plain `>`, **not** `FMath::Max` — so the double/float template-deduction trap the existing branch-3 comment warns about is **not** re-introduced. `FBox` and `UE_KINDA_SMALL_NUMBER` both arrive via `CoreMinimal.h` (`SiegeGameMode.h:5`); `GameFramework/PlayerStart.h` and `EngineUtils.h` were already included. **No new `#include` was needed or added.**
7. **The `UE_LOG` format string** — 10 specifiers, 10 arguments, verified by hand; doubles to `%.0f` matches the existing branch-3 log's convention in this same file.

---

## Files touched

- `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Source\GitClaudeUnrealTest\Siegebound\SiegeGameMode.cpp`
- `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Source\GitClaudeUnrealTest\Siegebound\SiegeGameMode.h`

## Files deliberately NOT touched

`Content/Maps/L_Arena.umap` · the `PlayerStart` actor/transform · any `Content/` asset · `Castle.{h,cpp}` (TASK-562, read-only for the 9450 verification) · `SummonedUnit.{h,cpp}` (574) · `SiegeBotController.{h,cpp}` (575 — **owns the phantom-xref fix**) · `ScatterConfig.h` / `BattlefieldScatter.{h,cpp}` (576) · `SiegePlayerState.cpp` / `CaptureZone.h` / `HeroCharacter.cpp` (577) · `SiegePlayerController.{h,cpp}` (563) · any `.csv` · `Tests/`
