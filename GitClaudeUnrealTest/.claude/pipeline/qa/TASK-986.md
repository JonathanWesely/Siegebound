# QA Report — TASK-986
Verdict: **PASS** — ⛔ **0 BLOCKERS** · 4 WARN · 4 NIT

Gate over: **`TASK-981`** + **`TASK-982`**
Inputs: `handoffs/TASK-981-programmer.md` · `handoffs/TASK-982-programmer.md` · `qa/TASK-981.md` (**PASS**, 0 BLOCKERS · 4 WARN · 5 NIT — ⛔ **cited, not re-reviewed**)
Subjects read: `FogVolume.h` · `FogVolume.cpp` · `CardRow.h` · `SpellLibrary.cpp` · `Tests/SiegeBrightSunTest.cpp` · **`Tests/SiegeFogVolumeTest.cpp`** (added to scope on the author's declaration — correctly; its 4 forced edits are graded in §6) · `SiegeFogStatics.{h,cpp}` + `Tests/SiegeFogTest.cpp` (read only to re-derive the three density numbers, per board item 1)
Coverage (`SC-§29`): this report covers **`TASK-981` + `TASK-982` and nothing else.** It is not a gate over `TASK-998` (`SC-§37`: a gate over a dependent row is not a gate over its dependency).

---

## §0 — INSTRUMENT DECLARATION (`SC-§39`, declared ABOVE the findings)

- Every character-exact claim is quoted from **`Read`**. `Grep` located; every located line was re-read before being judged.
- Arithmetic **re-derived independently**, never confirmed from the author's or the board's numbers.
- Counts were produced by `Grep` and then **reconciled by hand against the comment-skip rule the suite's own `CountOccurrencesInCode` applies** (leading `//`, `* `, `*/`, `/*`, bare `*`), because that rule is load-bearing in three of the assertions below.
- ⛔ **I hold NO `Bash` and NO Git (`SC-§71b`).** ⇒ every `HEAD`-vs-tree, "byte-unchanged", "untracked", or `.uasset` statement below is marked **ACCEPTED AS DECLARED — NEVER MEASURED**, and must not travel downstream as verified.
- ⛔ **NOTHING IN THIS BATCH HAS BEEN COMPILED OR EXECUTED.** Every count is a **source-text** count. `461` is not a pass count; `+8` is not `8/8`.
- ⛔ Not done, by fence: no compile · no engine/editor/MCP · no Git · no code edit. My only write outside this report is `TASK-986`'s own `status:` line.

---

## §1 — ⛔⛔ THE CENTRAL CLAIM: THE WRITER CENSUS, MEASURED BY ME

**The claim:** exactly three states; the forbidden **"fogged AND shielded"** is unreachable *by construction*; `ApplyBrightSun` + `RaiseFog` + `ResetFog` are the **only** writers of either deadline, at **3 + 2**.

### 1.1 My census (tree-wide symbol sweep over `Source/`, then read-back of every hit)

| scalar | ASSIGNMENT sites | non-write hits (verified, discounted) |
|---|---|---|
| `FogActiveUntilTimeSeconds` | `FogVolume.cpp:120` (`RaiseFog`, `=`) · **`:177`** (`ApplyBrightSun`, `= 0.0`) · `:197` (`ResetFog`, `= 0.0`) ⇒ **3** | `:124` UE_LOG arg · `:219` comparison · `FogVolume.h:451` in-class initialiser · `h:82`/`:99`/`:460` doc comments |
| `FogPreventedUntilTimeSeconds` | **`FogVolume.cpp:182`** (`ApplyBrightSun`) · `:205` (`ResetFog`) ⇒ **2** | `:188` UE_LOG arg · `:232` comparison · `:250` subtraction · `FogVolume.h:472` initialiser · `h:83`/`:99` doc comments |

> ### ✅ **3 + 2 CONFIRMED. NO FOURTH WRITER EXISTS ANYWHERE IN `Source/`.**

### 1.2 ⭐⭐ AND IT IS STRONGER THAN THE AUTHOR CLAIMED — the census is enforced by the LANGUAGE, not by the grep

Both deadlines are `private:` and **`AFogVolume` declares no `friend`** (I read the whole header). ⇒ a fourth writer is not merely *absent*, it is **inexpressible outside `FogVolume.cpp`**. Test 6's file-scoped census guards the one file where a fourth writer could legally be added — which is exactly the right scope, and it is complete.

The one theoretical bypass is **reflection**, and it is closed too: neither deadline is `BlueprintReadWrite`/`BlueprintReadOnly` (both are `VisibleInstanceOnly, Transient`), so there is no Blueprint write node, and no reflection-based write to either symbol exists tree-wide.

⇒ **"by construction" survives my check as a language guarantee, not a convention.**

### 1.3 The fourth state, reasoned rather than grepped

To be FOGGED ∧ SHIELDED you need both deadlines in the future at once.
- `FogPrevented > now` is written **only** at `:182`, which is **unconditionally preceded, in the same straight-line block with no branch and no early return between them**, by `:177` zeroing the fog deadline.
- `FogActive > now` is written **only** at `:120`, which is **unconditionally preceded** by the `IsFogPrevented()` early-return at `:104-111`.
- `ResetFog` zeroes both.

⇒ **no interleaving of the three writers can produce the fourth state.** ✅

⭐ And the refusal branch at `:153-165` deliberately does **not** zero the fog deadline — which I initially flagged and then cleared: reaching it requires `RemainingSeconds > 0` ⇒ SHIELDED ⇒ fog is already `0.0` by the above. Correct, not an omission.

### 1.4 ⭐ THE ONE-WAY DOOR — ⛔ CONFIRMED: NOTHING STASHES IT

`AFogVolume`'s **complete** member list is 5 `EditDefaultsOnly` floats + 2 `Transient` doubles. **There is no third scalar, no `bool`, no `TOptional`, no container — there is nowhere for a remainder to live.** `ApplyBrightSun:176` reads `IsFogActive()` into a local `bWasFogged` used **only** to pick a word in the log string, then discards it. After `FogActiveUntilTimeSeconds = 0.0`, `IsFogActive()` evaluates `GetTimeSeconds() < 0.0` = **false forever**. No expiry handler is needed and none exists. ✅ His sentence holds.

---

## §2 — ⛔ `J-F18`: THE ONLY ASSERTION THAT CAN SEE THE RULING

**It exists:** `Tests/SiegeBrightSunTest.cpp:659-664` — `CountOccurrencesInCode(ApplyBody, TEXT("FMath::Max")) == 0`.

**Would it go red?** I checked the mechanism rather than the intent:
- `ExtractFunctionBody("bool AFogVolume::ApplyBrightSun(")` terminates at the first **column-0** `\n}`. Every interior brace in that function is tab-indented (`:134`, `:165`), so the extracted body is genuinely `FogVolume.cpp:128-189` — not a truncated stub.
- Its zeros are **controlled both ways on the same body** (`SC-§39`): `GetFogPreventionSecondsRemaining()` == 1 and `GetBrightSunWindowSeconds(` == 1 would both fail on an empty or mis-extracted body. An extraction failure cannot read as SAFE.
- Restoring the struck `max(remaining, new)` with `FMath::Max` puts the token inside that body ⇒ **1 ≠ 0 ⇒ RED.** ✅

**And the author's reason for why this is the *only* test that can see it — I verified it is true.** `max` and "reset if longer" produce the identical stored expiry in the longer case; they diverge only in the shorter case, where `max` keeps the timer *and* the caller still bills 60 gold and eats the card. No assertion about a resulting **duration** can separate them. Confirmed by reading the branch at `:152-165`.

⚠️ **BOUNDED — see WARN-1.** The pin bans a **spelling**, not the semantics. It has also **never been seen red**, because nothing in this batch has been executed; its redness is derived from reading, not observed. I record that distinction rather than launder it.

**The strict `<` and the boundary:** `NewWindowSeconds < RemainingSeconds` ⇒ refuse. His words were *"UNLESS that new time would be LESS than the current time"* ⇒ **EQUAL resets**. That is the literal reading, it is declared as a default (not as his word) in both the code and the handoff, and I agree with it.

---

## §3 — ⛔ THE SUPERSEDED-VERDICT TRAP: **DEFUSED, NOT INHERITED**

⛔ **First, a coordinate correction that is the dispatch's, not the diff's:** the contract locates this at `FogVolume.h:136-143`. It is at **`FogVolume.h:303-325`** (`:136-143` is now the `FindOrSpawn` doc). `SC-§38` — located by content. No finding against the author.

**Measured at source:**

| claim | shipped value | my derivation | verdict |
|---|---|---|---|
| the notice radius | `5000` | `ASummonedUnit::UnitEngagementRadiusUU = 5000.f` — **`SummonedUnit.h:895`**, read | ✅ |
| the reduction | **`87.8%`** | `1 − 609.6/5000 = 0.87808` ⇒ 87.808% | ✅ |
| who it binds | *"⛔ **EVERY** unit on BOTH sides — ⛔ melee included, ⛔ not only the ranged ones"* (`:306-307`) | — | ✅ |

**And no sentence still cites the retired figures as true.** `2000` and `69.5%` survive at `:316`/`:319` — **only inside the struck block**, wrapped in `~~…~~`, labelled *"All three parts were false"*, with `69.5%` explicitly identified as *"the arithmetic of the retired default"* (`SC-§53` cl. 3: named rather than deleted, so a reader who remembers them finds the replacement). ✅

⭐ **The trap itself is named in the file**, at `:321-325`: *"`qa/TASK-1011.md` NIT-2 graded this very block 'TRUE TODAY'. ⛔ That verdict is ⛔ SUPERSEDED … ⛔ Cite `qa/TASK-1014.md`; ⛔ never NIT-2."* ⇒ the stale verdict cannot travel forward out of this file. **This is the right shape and I am recording it as precedent: a superseded verdict must be retired at the artefact it cleared, not only in the report that supersedes it.**

⚠️ The **fourth** false sentence in that block (*"until `TASK-840` lands there is nothing on the other side of it"*) is **deliberately left standing and flagged in place** at `:330-334`, routed to `TASK-1016` item (3). ⛔ **Correctly declined** — repairing prose about a wiring that does not exist yet would be the same defect inverted. Reported, not swept. ✅

---

## §4 — THE "ALSO VERIFY" LEDGER

| # | item | verdict | evidence |
|---|---|---|---|
| 1 | `AFogVolume` **EXTENDED**, never duplicated | ✅ | `FogPreventedUntilTimeSeconds` at `FogVolume.h:472` sits beside `FogActiveUntilTimeSeconds` at `:451`, **same class, same file pair**. One `SpawnActor<AFogVolume>` (`.cpp:71`). No second state actor, no second `ReadFogState`, no per-actor flag — tree-wide grep for the second symbol returns **only** `FogVolume.{h,cpp}` + tests |
| 2 | item (0) landed | ✅ | `CardRow.h:67-75`. *"for `EffectDuration`"* is gone; the correction states the duration is `AFogVolume::FogDurationSeconds` read off the CDO by `RaiseFog()` **which takes no argument**, that the cell is **not read**, that the old line was true only by the `300`/`300` coincidence, and it names ⭐ **`TASK-1016`**. ⛔ **Zero executable change** |
| 3 | `ESpellEffect::FogClear` appended **LAST** (== 7) | ✅ | `CardRow.h:142`, after `FogCover` at `:108`. No explicit `= N` anywhere in the enum ⇒ nothing overloaded, nothing renumbered. ⭐ `TASK-1015`'s property **kept**: `Tests/SiegeFogVolumeTest.cpp:225-230` still pins `FogCover == 6` as the anti-insert canary, and `:232-236` adds `FogClear == 7`. **Compile-safety measured at all three `switch (Row.SpellEffect)` sites myself: `SpellLibrary.cpp:600` (arm added), `DeckBuilderWidget.cpp:1342` (`default:` at `:1395`), `SpellLineSweep.cpp:177` (`default:` at `:256`) ⇒ no unhandled-enum error** |
| 4 | `SiegeGameMode.cpp` learns **NO** fog policy (`SC-§62`) | ✅ **MEASURED** | I grepped the whole file for all five policy needles: `BrightSun` **0** · `FogPrevent` **0** · `FogDurationSeconds` **0** · `FogVisionCeilingUU` **0** · `FSiegeFogTuning` **0**. The file's entire fog contact is `#include` (`:17`), `TActorIterator<AFogVolume>` (`:1402`) and **`It->ResetFog();` (`:1404`)** — one entry point, second zero inside `ResetFog`. ⛔ **No second policy value leaked. No blocker** |
| 5a | `J-F13` flat datum, ⛔ no trace | ✅ | `ArenaGroundReferenceZUU = 0.f` (`FogVolume.h:433`). I swept `FogVolume.cpp` for all six trace needles myself — `LineTraceSingleByChannel`, `LineTraceSingleByObjectType`, `SweepSingleByChannel`, `FHitResult`, `FCollisionQueryParams`, `ECC_` — **all 0**. Hills are **not** excluded from the hero's earned height; the *datum* is flat. Correct reading of his sentence |
| 5b | `J-F15` sampled **AT CAST** | ✅ | `FogVolume.cpp:140` — one frozen sample; `:264` comment states it is the only freeze. The accessor itself re-samples live (`:271`) |
| 5c | `J-F16` fog-on-fog **RESETS** (`=`, never `+=`) | ✅ | `FogVolume.cpp:120` is a plain `=`. `RaiseFog` has **no** `IsFogActive()` branch (verified: the body at `:85-125` contains `IsFogPrevented()` once and `IsFogActive()` **zero** times) ⇒ the reset is a property of the assignment, not a guard |
| 5d | `J-F14` **UNCAPPED** | ✅ | `FogVolume.cpp:323` returns `Base + Bonus × CompletedSteps` with no clamp; asserted at 100 steps (`SiegeBrightSunTest.cpp:316-321`) — deliberately absurd, because a cap would be invisible at every reachable perch |
| 5e | `BrightSunHeightStepUU = 1524.f`, INDEPENDENT | ✅ | `FogVolume.h:390`. ⛔ Not `609.6`. **`FogVisionCeilingUU` appears on ZERO code lines of `FogVolume.{h,cpp}`** — the 3 header hits (`:307`, `:374`, `:380`) are all ` * ` doc lines, which I checked individually against the scanner's skip rule. `FSiegeFogTuning`: 0. ⭐ **And stronger than the count: `FogVolume.cpp` does not even `#include` `SiegeFogStatics.h` — the decoupling is at the include level** |
| 5f | ⛔ `SummonedUnit.h`'s `152.4f` **UNTOUCHED** | ✅ present / ⚠️ **ACCEPTED AS DECLARED** | `HeightBonusStepUU = 152.4f` at **`SummonedUnit.h:1600`**, `HeightBonusPerStep = 0.10f` at `:1613` — both correct today, both **read-only** in this diff (test 3 reads them by reflection; `HeightAdvantageMultiplier` is `public` at `:1107`, so **no access was widened**). ⛔ That `TASK-982` did not *write* the file is a Git claim I cannot measure |
| 6 | 3 pin moves + 1 rename | ✅ all four located, all four justified | §6 below |
| 7 | suite delta `+8` | ✅ **DERIVED = DECLARED** | §7 below |

### Board-spec items (1)–(9)

- **(1) The three numbers, RE-DERIVED BY ME** — σ = −ln(0.02)/609.6 = 3.9120230/609.6 = **0.00641736 /uu**; `609.6` ⇒ 1 − 0.02 = **0.9800000** · `304.8` ⇒ 1 − √0.02 = **0.85857864** · `152.4` ⇒ 1 − 0.02^¼ = **0.62393969**. Shipped form confirmed at `SiegeFogStatics.cpp:83` (`Clamp(1 − Exp(−σ·d))`, σ derived at `:71-72`). Agrees with `FOG-§9.2` (62.4 / 85.86 / 98.00 %) and corroborates `qa/TASK-981.md` §2. ⛔ **The board's corrected `0.6239397` is right; the retired `0.6238` misses by `1.397e-4` — larger than the file's `1e-4` and it would have failed a correct build.**
- **(2) The separation** — ✅ `EffectiveVisionRadius` is still a **HARD CUT** (`SiegeFogStatics.cpp:114-117`, and `IsVisibleThroughFog` is `Distance <= EffectiveVisionRadius(...)` at `:139`). **`FogDensityAt(` occurs EXACTLY TWICE in shipping source** — its declaration (`SiegeFogStatics.h:404`) and its definition (`.cpp:20`). ⛔ **ZERO gameplay callers. No blocker.**
- **(3) The `√` relation** — settled half; `qa/TASK-981.md` PASS cited, not re-reviewed. Corroborated in passing: the `FMath::Sqrt` uses build **absolute** expectations and never compare two `FogDensityAt` calls, so the banned theorem-test is absent.
- **(4) `FogDensityExponent` retired, not dormant** — 3 surviving textual hits (`SiegeFogStatics.h:41`, `:55`, `:273`), **all doc-comment lines**, **0 code references tree-wide**. ⛔ `FOG-§7b` intact and **not removed**: `ClampMin = "304.8"` at `SiegeFogStatics.h:267` and `Ceiling <= 0.f` at `.cpp:114`. No blocker.
- **(5) One-way door** — ✅ §1.4.
- **(6) `1524.f`, independent** — ✅ row 5e. Test **re-derives** it: `SiegeBrightSunTest.cpp:272` asserts `StepUU == 50.f * 30.48f`, not a retyped `1524`.
- **(7) `floor` boundaries** — ✅ `SiegeBrightSunTest.cpp:289-298` asserts **0 / 1523 / 1524 / 3047 / 3048**, plus a hill at **+1600 ⇒ 1 step** (the row that would have gone red against the struck trace default) and **−500 ⇒ 0 steps**. `1523`/`3047` are exactly the inputs that separate `floor` from `round`. ✅
- **(8) `J-F19` spends zero gold, no silent no-op** — ✅ Both refusal branches **log** (`FogVolume.cpp:106`, `:160`; `SpellLibrary.cpp:692`, `:742`) and both **`return`** rather than falling through to `bResolved = true`, so a refused cast also spawns **no VFX**. I verified both refund sites and both orderings at source: `AddGold(Row.Cost)` at `SiegePlayerController.cpp:4578` **before** `ConfirmInstantDraw(` at `:4595`; `ResolveSpell(` at `:3316` **before** `AddGold(TargetingCost)` at `:3326` **before** `ConfirmPlayFromHand(` at `:3353`. ⛔ **Two properties, asserted separately.** ✅
- **(9) `FogDurationSeconds == 300.f`** — ✅ `FogVolume.h:337`, not `30.f`.

---

## §5 — 🧑 THE ONE JUDGEMENT CALL ROUTED TO ME: **HERO Z IS THE ACTOR ORIGIN (≈ +88 uu), NOT THE FEET**

> ### ⚖️ **RULING: THE DECISION SHIPS. THE STATED REASON IS PARTLY WRONG AND MUST NOT TRAVEL AS A CLEARANCE. IT DOES *NOT* NEED JONATHAN'S WORD BEFORE `TASK-987` — AND IT IS *NOT* SETTLED BY THE CONSISTENCY ARGUMENT.**

**(a) The consistency claim is factually TRUE — I measured it.** `SummonedUnit.cpp:4660-4664` feeds `HeightAdvantageMultiplier(GetActorLocation().Z, Target->GetActorLocation().Z, …)` — the raw actor origin for **both** operands. `FogVolume.cpp:281` reads `Hero->GetActorLocation().Z`. Same accessor, same quantity. ✅

**(b) ⛔ But the two lanes are NOT equally exposed, and this is where the reasoning overstates.** The damage lane takes a **difference** of two same-kind Zs, so a constant capsule-centre offset **cancels exactly** — it is *invisible* there. The window lane takes an **absolute** height against a fixed datum, so the offset **survives** as a real +88 uu bias. ⇒ *"a hand-rolled foot offset here would make the two lanes diverge by construction"* is **not right**: the damage lane cannot observe a correction made here at all. The two lanes are not measuring the same thing; they are merely calling the same accessor.

**(c) What actually settles it, and it is a better argument than the one offered:**
1. **Magnitude:** 88 / 1524 = **5.77%** of one step (the author's 5.8% ✅).
2. **Zero shipped-case impact — I re-derived both boundaries myself, I did not accept them:** flat grass ⇒ `floor(88 / 1524) = 0` steps = the feet-Z answer; a ×2 Watch Tower ⇒ `floor(2488 / 1524) = 1` = `floor(2400 / 1524) = 1`. **No shipped case moves.**
3. **There is no shipped "feet Z" accessor.** A correction means inventing a constant Jonathan never gave — and this batch's own record (`TASK-839` declining to invent a state home, ruled **correct**) says an invented constant is the worse failure. ⭐ **Declining to route around an under-specified ruling by inventing a number is the right failure mode.**

**(d) When it stops being free — the trigger, made checkable:** the first perch whose true **foot** altitude lands in **1436–1524 uu** (generally any `n·1524 − 88` band) earns a whole extra minute it has not stood up for. That is the moment to ask him, and it is a measurable condition rather than a feeling.

⇒ **Not a blocker. Not a WARN against the behaviour.** A **WARN against the stated reason** (WARN-3) plus a boarded one-line question with the trigger above. ⛔ **Do not hold `TASK-987` for it.**

---

## §6 — THE 3 PIN MOVES + 1 RENAME, GRADED

All four are in `Tests/SiegeFogVolumeTest.cpp`, all four located, all four carry old value / new value / why **at the site**:

| # | pin | old ⇒ new | my grade |
|---|---|---|---|
| 1 | test 1 "highest declared value" + floor | `FogCover` ⇒ `FogClear`; `>= 7` ⇒ `>= 8` (`:276`, `:279-284`) | ✅ **FORCED and PRE-AUTHORISED** — the file's own comment instructed it in advance (*"WHEN `FogClear` IS APPENDED BELOW, THIS ROW MOVES TO IT; it does not get deleted"*). Appending **correctly** is what turns the old row red. The comment now instructs the *next* author the same way |
| 2 | test 2(c) `AFogVolume::FindOrSpawn(` in `ResolveSpell` | `== 1` ⇒ `== 2` (`:347`) | ✅ **NOT a loosening** — I verified the per-arm strength is preserved elsewhere: `SiegeBrightSunTest.cpp:716`/`:721` pin `case ESpellEffect::FogClear:` and `->ApplyBrightSun(` at 1 each, and `:353` keeps `->RaiseFog()` at 1. The file-wide number became a **census**; the discrimination moved, it did not evaporate |
| 3 | test 4 extraction signature | `void AFogVolume::RaiseFog(` ⇒ `bool …` (`:453`) | ✅ **FORCED** — `ExtractFunctionBody` fails loudly on a stale signature by design (`SC-§38`), so an unmoved pin would have gone red and looked like a defect in shipped code |
| 4 | test 1 **registered name** | `…AndFogCoverIsLast` ⇒ `…AndTheNewestValueIsLast` (`:205`) | ✅ **CORRECT and the right shape** — the old name asserted the **opposite** of what the row now checks, and a registered name is the one string a reader sees without opening the file. ⭐ The new name is **value-agnostic**, so the next append does not move it again. The C++ class `FSiegeFogVolumeEnumIsAppendOnlyTest` is **unchanged** ⇒ no registration churn |

**⛔ AND THE CHECK THE AUTHOR ASKED FOR BUT DID NOT GET CREDIT FOR — I looked for pins `TASK-982` BROKE and did NOT move.** Every count in `Tests/SiegeFogVolumeTest.cpp` that reaches outside itself, re-derived against the shipped tree:

`case ESpellEffect::FogCover:` file-wide **1** ✅ (the new arm is a different needle) · in `ResolveBody` **1** ✅ · `AFogVolume::FindOrSpawn(` **2** ✅ (`SpellLibrary.cpp:668`, `:722`) · `->RaiseFog()` **1** ✅ · `AFogVolume::Find(` in `ResolveBody` **0** ✅ (`FindOrSpawn(` is not `Find(` — no false positive) · seam pins `:529`/`:533`/`:542`/`:549` untouched by this row ✅ · `SpawnActor<AFogVolume>` in `FogVolume.cpp` **1** ✅ (`.cpp:71`) · `RaiseBody` `FogActiveUntilTimeSeconds =` **1** / `+=` **0** ✅ · `ResetBody` `= 0.0;` **1** ✅.

> ⇒ ⛔ **NO UNMOVED PIN. The 4 declared moves are the complete set.**

---

## §7 — SUITE DELTA: **DECLARED `+8`** — DERIVED INDEPENDENTLY, AND IT RECONCILES

**My derivation (source-text, `^IMPLEMENT_SIMPLE_AUTOMATION_TEST` over `Siegebound/Tests/*.cpp`):**
- `Tests/SiegeBrightSunTest.cpp` = **8** registrations (lines 225 · 378 · 460 · 551 · 632 · 743 · 854 · 941) and 8 distinct `"Siegebound.BrightSun.…"` names ⇒ **+8**.
- `Tests/SiegeFogVolumeTest.cpp` = **7**, **unchanged** — the rename moved the registered *string*, not the count.
- ⇒ **`TASK-982` delta = `+8`. DECLARED = DERIVED.** ⭐ The author's self-correction `+7 → +8` (counted, not recalled) is **right**.
- `TASK-981`'s own `+2` still stands: `SiegeFogTest.cpp` = **10**, `SiegeFogClampTest.cpp` = **9**. ✅

**⚠️ THE RENAME CAVEAT THE CONTRACT WARNED ABOUT, MADE CONCRETE:** a **name-keyed** suite diff will report **9 additions and 1 removal**, not `+8` — `…AndFogCoverIsLast` disappears and `…AndTheNewestValueIsLast` appears. ⛔ **Reconcile by registration COUNT, or by name with that pair named explicitly.** Anyone diffing names blind will be off by one in both columns and will go looking for a deleted test that was never deleted.

**⛔ THE ABSOLUTE, AND WHY IT MUST NOT BE QUOTED AS `441 + 8 = 449`:** I measure the tree **now** at **461 registrations across 36 files**. `TASK-981` declared its post-edit state at 441/33. The extra **+12 across 2 more files** belongs to **concurrent lanes** (`TASK-979`/`980`/`993`/`997` …), **not** to either subject of this gate. ⇒ the only trustworthy statement is the **per-row delta**; the absolute must be **re-measured by `TASK-987` after every lane has landed**. ⛔ **And 461 is a source-text count — nothing here has been compiled or executed** (`TL-§5c`).

---

## §8 — FINDINGS

### BLOCKERS
> ### ⛔ **NONE.**

### WARN

- **[WARN-1] `Tests/SiegeBrightSunTest.cpp:664` — the `J-F18` guard bans a SPELLING, not the semantics; a hand-rolled `max` slips past it.**
  The pin catches the named regression (`FMath::Max` restored inside `ApplyBrightSun`) and I confirmed it would go red. It does **not** catch the same economics written without that token — e.g. `if (NewWindowSeconds > RemainingSeconds) { …stamp… } return true;`, which keeps the timer *and* bills 60 gold *and* eats the card. Every other pin in test 5 stays green through that variant (`GetFogPreventionSecondsRemaining()` still 1, `GetBrightSunWindowSeconds(` still 1).
  ⇒ **Suggested fix, for a LATER row — ⛔ not this diff, which is correct:** add two pins on the same extracted body — `CountOccurrencesInCode(ApplyBody, TEXT("NewWindowSeconds < RemainingSeconds")) == 1` and `CountOccurrencesInCode(ApplyBody, TEXT("return false;")) == 2`. That converts a ban on a token into an assertion about the **branch**, which is what the ruling actually is.
  ⚠️ And recorded plainly: **this pin has never been seen red, because nothing in this batch has been executed.**

- **[WARN-2] `Tests/SiegeBrightSunTest.cpp` test 6 pins four things inside `SiegePlayerController.cpp` — a file `TASK-989` AND `TASK-991` are both boarded to write — and neither row's spec names this test file.**
  The pins: `AddGold(Row.Cost) == 1` in `ResolveSpellInstant`, `AddGold(TargetingCost) == 1` in `TryConfirmSpellTarget`, and the two refund-before-consume orderings. **All four are correct today — I verified them at source** (`:4578 < :4595`; `:3316 < :3326 < :3353`). But `TASK-989`/`991` add refusal branches to exactly those two functions; a second refund call turns this file red and its author will have no idea where the pin lives. The same applies to `return false; == 8` over `USpellLibrary::ResolveSpell` (**I measured 8 myself**: `SpellLibrary.cpp` 584 · 590 · 675 · 696 · 729 · 746 · 761 · 766 ✅).
  ⇒ **Fix (manager, bookkeeping):** add `Tests/SiegeBrightSunTest.cpp` to `TASK-989`'s and `TASK-991`'s "files your diff can turn red" note. ⛔ This is a forward-coupling hazard, **not** a defect in this diff.

- **[WARN-3] `handoffs/TASK-982-programmer.md` §7(1) + `FogVolume.cpp:276-280` — the hero-Z reason is stated more strongly than it is true.**
  *"A hand-rolled offset here would make the two elevation lanes measure differently by construction"* — the damage lane takes a **difference** of two Zs, so a constant capsule offset **cancels** there and a correction made here would be **invisible** to it. The decision is right; the reason is not the one that makes it right (see §5(c)).
  ⇒ **Fix:** replace the sentence with the real grounds — 5.77% of one step, **zero shipped-case impact** (both boundaries re-derived), and **no authored feet-Z datum exists**. Add the trigger: **the first perch whose foot altitude lands in `n·1524 − 88`**. ⛔ Comment-only, zero behaviour, and it does **not** block the commit.

- **[WARN-4] `FogVolume.h:251-270` — the duration accessor advertises "samples on every call" without saying "⛔ not for per-frame polling".**
  `GetBrightSunWindowSeconds` runs a `TActorIterator<AHeroCharacter>` per call. Today that is once per cast plus two boarded **event-driven** callers, so there is no hot-path issue. But `TASK-991`'s message, or any HUD countdown, is one refactor away from polling it per frame, at which point an actor iteration becomes per-tick work (`TASK-004` never-per-tick law).
  ⇒ **Fix:** one sentence in the accessor doc — *"call it at click/cast time; ⛔ never per frame."*

### NIT

- **[NIT-1]** Dispatch coordinate drift, **not** a diff defect: item (0a) is at `FogVolume.h:303-325`, not `:136-143`. Content verified correct. `SC-§38` — locate by content.
- **[NIT-2]** `FogVolume.h:450`/`:471` — `meta = (AllowPrivateAccess = "true")` is **inert** on both deadlines: neither is `BlueprintReadOnly`/`ReadWrite`, so it grants nothing. It matches the first scalar exactly (`TASK-998`'s, already reviewed), so keeping it is the right consistency call. Recorded **only** so nobody reads it as a Blueprint write door — **it is not one**, which is load-bearing for §1.2.
- **[NIT-3]** `DeckBuilderWidget.cpp:1342-1397` — the glossary switch falls **both** fog effects to `default: break;`, so neither `Fog` nor `BrightSun` gets a glossary line. Already owned by ⭐ `TASK-999`; recorded so that row's gate knows the omission **predates** its diff.
- **[NIT-4]** `SiegeBrightSunTest.cpp:492-500` re-asserts `152.4` and `0.10`, which `Tests/SiegeHighGroundTest.cpp:620` already covers. **Declared by the author and I agree with keeping it** — the tie is meaningless without proving *which* numbers were tied — but it is a second pin on the same shipped constants, so a future retune now turns two files red. Accepted, recorded.

### 📌 ROUTED, NOT FIXED (the author's §8, re-checked and endorsed)

1. **🚨 `SiegePlayerController.cpp:1037` routes to the no-reticle instant path on `GoldSteal` and nothing else** ⇒ both fog cards enter **targeting mode** and the player aims a reticle at a map-wide effect, contradicting `FOG-§10.1`. ⛔ **Correctly declined** (not this row's file). `TASK-999` item (3b) covers the *glossary text*, ⛔ **not the routing**. ⇒ **manager: this likely wants its own row.**
2. `SiegeCombatStatics.h:405` — *"its **ONE** scalar"* is now false (there are two, though the **seam** still reads only one, which is correct). A **fourth clause for `TASK-1007`**. ⛔ Correctly not swept.
3. `SpellLibrary.h`'s class-doc effect list omits **both** `FogCover` and `FogClear`. Predates this row. Report only.
4. `FSiegeAssistantSnapshot::MarkPlaceGroundZ = 0.f` — a **third** site holding the arena-floor datum. ⛔ **Correctly not pinned**: its own comment licenses it to be retuned for hill-accurate marks, so pinning it would convert a documented divergence into a false coupling (`FOG-§9.5`'s trap, inverted).
5. `TASK-981` §7: `SiegeCombatStatics.cpp:122` still names *"exponent 2"*, a tunable that no longer exists. Comment-only, one word, routed. ⛔ Still open — **not a blocker**, and it is `TASK-980`/`839`/`1007` territory.

---

## §9 — NOTES FOR BUILD-MASTER (`TASK-987`)

1. ⛔ **Nothing here blocks the commit.** 0 blockers across both rows; `qa/TASK-981.md` is PASS and this report is PASS.
2. ⛔ **THE FIRST COMPILE AND THE FIRST EXECUTION OF THIS ENTIRE LANE ARE YOURS.** `TASK-981` rewrote the curve suite and `TASK-982` added a new 8-test file; **neither has ever been built or run**. Expect the curve rows and the BrightSun rows to be genuinely unproven until your run.
3. ⛔ **Suite: report the ABSOLUTE only after you measure it.** My source-text count is **461 across 36 files** in `Siegebound/Tests/*.cpp`, and it already includes concurrent lanes. ⛔ Do **not** publish `441 + 8 = 449` — it will not reconcile. Per-row deltas are `TASK-981 +2` and `TASK-982 +8`.
4. ⚠️ **The registered-test RENAME will make a name-keyed suite comparison read `+9 / −1`.** `Siegebound.Fog.TheSpellEffectEnumIsAppendOnlyAndFogCoverIsLast` ⇒ `…AndTheNewestValueIsLast`. **It was not deleted.** Do not diagnose the missing name as a dropped test.
5. ⚠️ **ACCEPTED AS DECLARED, NEVER MEASURED** (I hold no Git): that `FogVolume.{h,cpp}` and `Tests/SiegeFogVolumeTest.cpp` are still **untracked** (`??`, not `M`) and enter the repository for the first time with you; that `SiegeCombatStatics.{h,cpp}`, `SummonedUnit.{h,cpp}`, `Tests/SiegeFogTest.cpp` and `Tests/SiegeFogClampTest.cpp` were not written by `TASK-982`; and every `.uasset` claim in either handoff. ⛔ **Verify the staged set yourself before committing — none of it is verified here.**
6. ⭐ **A structural pin that will bite you if a file moves:** `Tests/SiegeBrightSunTest.cpp` reads five shipped source files by **relative path** (`FogVolume.h/.cpp`, `SpellLibrary.cpp`, `SiegeGameMode.cpp`, `SiegePlayerController.cpp`). A rename of any of them fails the probe **loudly** (by design, `SC-§38`) — that red is a stale probe, not a code defect.
7. 📌 If the build breaks, the highest-prior suspects are **not** in this diff: the three `switch (Row.SpellEffect)` sites are all `default:`-covered (**verified**), the const-correctness of `Hero->GetTeamId()`/`IsDead()` is **verified** (`HeroCharacter.h:432`/`:584`), and `ASummonedUnit::HeightAdvantageMultiplier` is **public** (`SummonedUnit.h:1107`), so no access was widened.

---

## §10 — STATUS LINES FOR THE ORCHESTRATOR (⛔ I flipped ONLY `TASK-986`'s own row)

- **`TASK-981`** — currently `✅✅ qa-passed 2026-09-04` (`qa/TASK-981.md`). ⇒ **ready-for-integration on `TASK-987`.** ⛔ This gate adds no finding against it. **Yours to sequence.**
- **`TASK-982`** — currently `✅ ready-for-qa 2026-09-04 (gate ⭐ TASK-986)` at board line **18323**. ⇒ **should become `qa-passed` / ready-for-integration on `TASK-987`.** ⛔ **I did not touch it.** **Yours to sequence.**
- **`TASK-986`** — flipped by me to **`qa-passed`**.
