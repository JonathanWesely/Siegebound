# QA Report — TASK-979
**Gate over:** `TASK-979` ALONE (the per-unit notice/engagement channel, `600` → `2000`)
**Inputs:** the `TASK-979` board row whole, all four amendments (`TASKBOARD.md:17985–18061`) · `handoffs/TASK-979-programmer.md` · `qa/TASK-996.md` · 🧑 Jonathan's mid-review ruling of 2026-09-04 (§3a)
**Date:** 2026-09-04 · **Agent:** qa-reviewer · **Writes:** this file + the board status line + one Slack post. ⛔ Zero source edits, zero engine, zero MCP, zero Git.

## Verdict: **FAIL** — **1 BLOCKER** · 5 WARN · 4 NIT

> ⛔⛔ **SUPERSEDED 2026-09-04 BY THE LOOP-2 VERDICT AT THE FOOT OF THIS FILE (`§L2`): ✅ PASS, 0 BLOCKER.** The loop-1 record below is kept **verbatim and unedited** (`TL-§5c` cl. 4 — struck, never deleted), including the two places where loop 2 found it **wrong**: its stale-site list was short by **three**, and its WARN-1 framing of `TASK-574` is **withdrawn** in `§L2(B)`.

⛔ **`TL-§5c` — NOTHING WAS COMPILED OR EXECUTED FOR THIS REVIEW.** No build, no editor, no PIE, no MCP. Every statement below is a **source read**, reproducible from the `file:line` given. **No executed suite number appears in this report.**

⭐ **The BLOCKER is comment-only and costs no build to fix** — `TASK-987` has not compiled yet, so the cycle is free. **Every one of the four claims the dispatch asked me to verify HELD, and three of them held under adversarial arithmetic rather than on the author's word. Jonathan's mid-review ruling also HELD as a prediction against the built diff (§3a).** This is a strong diff with one uncorrected instance of the exact defect class it was written to eliminate.

---

## §0 — INSTRUMENTS (`SC-§39`), AND THE ONE THAT LIED AGAIN

| instrument | control | result |
|---|---|---|
| `Read` | every character-exact claim (access specifiers, `Min`/`Clamp` absence, the stale comments) | ✅ authoritative |
| `Grep` | **location only** — never quoted | ✅ per ⭐ `SC-§38a` |
| negative control | `->AggroRadius\|->LeashRange\|->LeashMarginMultiplier` over `Tests/` ⇒ **0** | ✅ scanner alive (same sweep returned 445 macro hits) |
| arithmetic | IEEE-754 binary32 re-derivation of the leash identity — ⛔ not relayed | ✅ |

⚠️ **`SC-§38a` RE-CONFIRMED LIVE THIS SESSION.** `Grep`'s content output over `SummonedUnit.cpp:2542+` rendered the C++ comment marker `//` as `\` on three lines (e.g. `\ ── CONTRACT 1:`). A stray `\` there is a compile-breaking token. `Read` on the same lines shows correct `//`. ⇒ **`Grep` located; `Read` adjudicated. Nothing in this report is quoted from `Grep`.**

---

## §1 — ⛔ THE FOUR CLAIMS THAT CARRY THE DIFF

### (1) THE NO-CLAMP PROPERTY — ✅ **CONFIRMED, AND IT GOES RED FOUR INDEPENDENT WAYS**

`SummonedUnit.cpp:4410–4437`, `Read`, whole body. Three branches and nothing else:

```cpp
if (!(ClassDefaultRadiusUU > 0.f))                                   { return ClassDefaultRadiusUU; }  // the seal, FIRST
if (RowNoticeRangeUU > 0.f && FMath::IsFinite(RowNoticeRangeUU))     { return RowNoticeRangeUU; }      // unmodified
return ClassDefaultRadiusUU;                                                                           // sparse
```

⛔ **Zero `FMath::Min`, zero `Clamp` on any code line of that function.** The seal is first, so a card cell can never un-seal a sealed class (the ordering the author flags in §10 item 2 is correct and present). `!(x > 0.f)` rather than `x <= 0.f` also catches a NaN class default — correct.

⛔⛔ **I CONFIRMED THE PROPERTY GENUINELY FAILS AGAINST A CLAMPED IMPLEMENTATION, which is what the dispatch asked and is not the same as reading the assertion:**

| row | against the shipped resolver | against `min(Row, 2000)` |
|---|---|---|
| `SiegeUnitNoticeRangeTest.cpp:304` value | `Resolve(2000,3600) == 3600` ✅ | `2000 ≠ 3600` ⇒ **RED** |
| `:312` **property** `Resolve(2000,3600) > 2000` | `3600 > 2000` ✅ | `2000 > 2000` false ⇒ **RED** |
| `:316–323` **synthetic** `DefaultUU * 2.f` | `4000 == 4000` ✅ | `2000 ≠ 4000` ⇒ **RED** |
| `SiegeFogClampTest.cpp:1157–1164` cross-file | `Eff(3600, fog off) == 3600` ✅ | `Eff(2000,…) = 2000 ≠ 3600` ⇒ **RED** |

⭐ The synthetic is `DefaultUU * 2.f` — **derived from the constant, not a shipped design number** — so a clamp cannot hide behind an allow-list of known card values. That closes `SC-§37`: this is not "inert by coincidence of today's data".

⭐ And the **structural** refusal (`:362–386`) extracts the body by the exact signature `float ASummonedUnit::ResolveNoticeRadiusUU(float ClassDefaultRadiusUU, float RowNoticeRangeUU)` — which I verified is present **verbatim** at `SummonedUnit.cpp:4410`, so the probe is not stale — and asserts `FMath::Min` == 0 and `Clamp` == 0. ⇒ **the refusal is asserted, not asserted-about.**

### (2) THE `1.5` LEASH FACTOR IS MEASURED — ✅ **CONFIRMED BY MY OWN ARITHMETIC, NOT BY THE AUTHOR'S WORD**

`SummonedUnit.cpp:4439–4460`. Trace `ResolveEffectiveLeashRangeUU(900.f, 600.f, 1.5f)`:

1. `FMath::IsFinite(600.f)` ⇒ true, no early return.
2. `SafeMultiplier = (1.5f > 1.f && IsFinite(1.5f)) ? 1.5f : 1.f` ⇒ **1.5f**.
3. `FMath::Max(900.f, 600.f * 1.5f)`.

⭐⭐ **THE BIT-IDENTITY IS EXACT AND I VERIFIED IT AT THE REPRESENTATION LEVEL, because "bit-identical" is the entire load-bearing claim.** `600` and `1.5` are both exactly representable in IEEE-754 binary32. Their product `900 = 2² × 225` needs 8 significand bits, far inside binary32's 24 ⇒ **`600.f * 1.5f == 900.f` with no rounding at all**, and `Max(900.f, 900.f) = 900.f`. ⇒ **`ResolveEffectiveLeashRangeUU(900, 600, 1.5)` returns the shipped `900` bit-identically. The claim holds.**

Likewise `SiegeUnitNoticeRangeTest.cpp:424`'s `GetLeashMarginMultiplier() == 900.f / 600.f` at `Exact` (0.f) tolerance: the quotient is exactly `1.5`, and the member default is `1.5f` (`SummonedUnit.h:1266`) ⇒ exact equality is a **valid** assertion here, not a lucky one.

⇒ ⚖️ **THIS IS A REPAIR, NOT A REBALANCE.** Fed the pre-ruling pair the new mechanism reproduces the old behaviour exactly. **The diff does NOT rebalance every existing engagement in the game**, and does not have to be graded as one.

Both drop sites verified by `Read` at `SummonedUnit.cpp:1734` (`UpdateState`) and `:1979` (`UpdateStateStandardCommanded`, `Attack`/`default`). My own counts over `SummonedUnit.cpp`: `> GetEffectiveLeashRangeUU()` ⇒ **2**; `> LeashRange` ⇒ **0**; the only surviving `LeashRange` code lines are the resolver's parameter and `GetEffectiveLeashRangeUU()`'s own read at `:4465`. `UpdateStateGrouped`'s single `LeashRange` mention (`:2024`) is a comment line. ✅ All three counts match the assertions at `:486`, `:491`, `:505`.

Ordering at every radius, re-derived: 600 ⇒ `max(900,900)=900 > 600` ✅ · 2000 ⇒ `3000 > 2000` ✅ · 3600 ⇒ `5400 > 3600` ✅ · 8000 ⇒ `12000 > 8000` ✅ · degenerate `margin = 0` ⇒ floored to 1.0 ⇒ `max(900,2000) = 2000 >= 2000` ✅.

### (3) THE RE-DERIVED CLAMP TEST — ✅ **CONFIRMED IT CAN ACTUALLY FAIL**

`SiegeFogClampTest.cpp:1021–1173`, `Read` in full.

- **No hand-typed `2000.f` and no `600.f` on any code line of the file.** The only occurrences are four comment lines (`:1016`, `:1018`, `:1088`, `:1091`) that narrate the retirement. ⇒ **the identical-defect-one-change-later trap was NOT walked into.**
- Operand one is `UnitDefaults->GetEngagementRadiusUU()` (`:1032`, off the CDO through the public accessor). Operand two is `Tuning.FogVisionCeilingUU` (the shipped tuning struct). **Both are read; neither is a constant of the test.**
- **Mentally reverting `AggroRadius` to `600`:**
  - **(a) premise** `600 > 609.6` ⇒ false ⇒ **RED**, and its message names the reason ("the whole argument below must be RE-DERIVED, not the numbers re-signed").
  - **(b) value** `min(600, 609.6) = 600 ≠ 609.6` ⇒ **RED**.
  - **(c) cost** `600 < 600` ⇒ false ⇒ **RED**.
  - **(d) the two excepted-card rows** also go red against a clamp.
  ⇒ **Three rows red on the revert, five on a clamp. The test can fail. `SC-§60` is discharged.**
- **The file still declares 9 tests** — macro lines at `:292 :401 :486 :687 :802 :901 :999 :1185 :1278`. **Re-derived in place, not renumbered, not re-signed.** ✅ Exactly the disposition item (6f) demanded.

⚠️ One overstatement inside the fix — see **WARN-3**.

### (4) `445 DECLARED, +4` — ✅ **CONFIRMED INDEPENDENTLY, AND THE `TL-§5c` DISCIPLINE HELD**

My own anchored census of `IMPLEMENT_*_AUTOMATION_TEST` macro lines over `Source/GitClaudeUnrealTest/Siegebound/Tests/`: **445 across 34 files**, of which `SiegeUnitNoticeRangeTest.cpp` = **4** and `SiegeFogClampTest.cpp` = **9**. ⇒ **441 + 4 = 445 reconciles exactly.**

The word **"declared"** is used (§8 line 216, and again "445 is a **declared** count, not a pass count"), and the banner at handoff line 7 states *"No executed suite number appears in this document."* **I checked: none does.** ✅

⚠️ **Caveat, declared rather than hidden:** my census is at **my** instant, and `SiegeFogClampTest.cpp` is under a second reviewer's eye for `TASK-981` concurrently. If `981` lands macros in `SiegeFogTest.cpp` (10 today), 445 moves. **`TASK-987` derives its own number at its own instant.**

---

## §2 — ⭐ THE AUTHOR'S OWN §7 FINDINGS, GRADED

### §7(2) — item (3)'s prescribed CDO test cannot see the (6d-ii) defect: ✅ **CORRECT, AND IT IS THE SHARPEST THING IN THE HANDOFF. THE BOARD ROW NEEDS AMENDING.**

Verified at source. `LoadStatsAndStart` writes the **spawned instance** (`SummonedUnit.cpp:1302`, `AggroRadius = …`); `GetDefault<AMinerUnit>()->…` reads the **CDO**, which `LoadStatsAndStart` never touches at all. ⇒ a naive `AggroRadius = Row->NoticeRange` (or a resolver that consults the row before the seal) would leave **every spawned miner running at the row value** while item (3)'s own prescribed test stayed **permanently green**. **Board item (3) — *"Assert both are still `0` in a test"* — is inadequate as written and should be amended to require an assertion on the FUNCTION.**

The author's mitigation is the right shape and **can** fail: `SiegeUnitNoticeRangeTest.cpp:341–348` asserts `ResolveNoticeRadiusUU(0.f, 3600.f) == 0.f` and `(0.f, 0.f) == 0.f` on the pure seam. ⚠️ **Residual, declared here because the author declared it too (`SC-§40`):** this still does not prove the *instance* path is sealed at runtime — that is asserted only **structurally** (test 4(a): `ResolveNoticeRadiusUU(` appears exactly once in `LoadStatsAndStart`'s body). The file header states the gap at `:50–55`. **Honest, and correctly bounded.**

### §7(1) — "the false claim lived in THREE places": ⛔ **UNDERCOUNTED. IT IS FIVE, AND TWO ARE STILL LIVE.** ⇒ **BLOCKER-1.**

### §7(3) — the `2000` census: ✅ **CONFIRMED EXACTLY BY MY OWN COUNT.** Code lines, shipping source only (⛔ `Tests/` excluded, per the project's own census convention): `SummonedUnit.h:844` (**this diff — the constant**) · `Castle.h:359` + `:885` (HP) · `GitClaudeUnrealTestCharacter.cpp:35` + `Variant_SideScrolling/SideScrollingCharacter.cpp:43` (`BrakingDecelerationWalking`). **5 hits, 4 pre-existing and unrelated.** ⇒ **`TASK-985` item (1a) is SATISFIED: exactly one notice/engagement `2000`.** I did not file a false BLOCKER on the other four.

### The THREE SELF-CAUGHT DEFECTS — ✅ **ALL THREE GENUINELY FIXED. VERIFIED AT SOURCE, NOT ACCEPTED (`SC-§39`).**

- **(i) the compile-blocking access defect (`error C2248`) — FIXED.** `Read` confirms `SummonedUnit.h` access specifiers at **`public:` 192 → `protected:` 1111 → `private:` 1706** (the handoff's `1101`/`1696` are ~10 lines stale — NIT-1, harmless). The three members sit at **1240 / 1253 / 1266**, i.e. **protected**. The five public accessors sit at **927 / 934 / 943 / 944 / 963**, all above `1111` ⇒ **public**. My negative sweep for `->AggroRadius|->LeashRange|->LeashMarginMultiplier` over `Tests/` returns **0 matches**. ⇒ **zero remaining C2248 sites.** ⭐ This one matters most: **nothing has been compiled this wave**, so a second access error would have surfaced only at `TASK-987`. It is closed.
  ⭐ The root cause is worth keeping exactly as the author wrote it: `ABuilding::MaxStackHeightMultiplier` **is** public (`Building.h:186`), so the pattern copied from `SiegeBuildingStackTest.cpp:119` is legal *there* and illegal *here* — **the difference is one access specifier ~800 lines away.**
- **(ii) the tautology inside the fix for `SC-§60` — FIXED, and the replacement genuinely cannot be tautological.** `SiegeUnitNoticeRangeTest.cpp:257–262` now asserts `UnitDefaults->GetEngagementRadiusUU() != MinerDefaults->GetEngagementRadiusUU()`. **I confirmed it fails against the implementation it names:** `return UnitEngagementRadiusUU;` makes both sides `2000`, `!=` is false ⇒ **RED**. A constant cannot produce a difference between two classes. ⭐ And it is the row `TASK-980` depends on.
- **(iii) the stale `TASK-838` clause 30 lines above the code that falsified it — FIXED.** `SummonedUnit.cpp:2398–2411`, `Read`: the clause is quoted, struck, and amended with its reason ("the observation WAS correct and Jonathan then ruled the narrowing IN, by name"), and the real warning is honoured — the bound is a site-local cut and `SeeingFromUnbounded` (`:2412`) is untouched. **Amended, not deleted.** ✅
- **the `%.1f` NIT — FIXED, and I audited the class rather than the instance.** All 13 `FString::Printf` calls in the changed/added rows have matching specifier counts and argument counts, `%%` correctly escaped at `SiegeFogClampTest.cpp:1161` and `SiegeUnitNoticeRangeTest.cpp:607`. **No format/argument mismatch anywhere in the diff.**

---

## Findings

- **[BLOCKER-1]** `Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.h:1895` (+ `:125`, `:131`) — ⛔⛔ **THE STALE-CLAIM DEFECT THE AUTHOR MADE THE HEADLINE OF ITS OWN HANDOFF IS STILL LIVE IN THREE PLACES IN SHIPPED SOURCE, AND ONE OF THEM WAS FALSIFIED BY THIS DIFF ON THE DECLARATION OF THE FUNCTION IT CHANGED.** All three quoted from **`Read`**, character-exact:
  - **`:1891–1897`, the doc block on `AcquireEnemyNearPoint`'s own declaration:** *"…the eligibility gate is a 2D disc — a candidate's LOCATION must lie within Radius of Center — **instead of AggroRadius-from-self**."* ⛔ **This diff added exactly an `AggroRadius`-from-self bound** (`SummonedUnit.cpp:2443` `const float NoticeRadiusUU = GetEngagementRadiusUU();` → `:2478` `if (Distance > NoticeRadiusUU)`), as a SECOND term beside the disc. The declaration now states the opposite of the definition.
  - **`:125`:** *"Acquire   — nearest alive enemy ITeamAgent within AggroRadius (**600**);"* ⇒ shipped value is `2000` (3600 via the row).
  - **`:131`:** *"Reacquire — target dead/destroyed or beyond **LeashRange (900)**: resume Advance."* ⇒ no drop site reads `LeashRange` any more; the effective leash is `3000` at the default.

  ⚠️⚠️ **WHY THIS IS BLOCKER-GRADE AND NOT A NIT, ON THIS ROW SPECIFICALLY:** ⛔ item **(6d-v)** orders *"REWRITE `SummonedUnit.h:1037`'s COMMENT — do NOT leave it lying"* and cites `HIGH-§1` / the `TASK-517` idiom by name (*"a shipped comment contradicting the shipped value is the drift defect this project keeps paying for"*). The author rewrote **only the site the board named by line number** and left the two the board did not. ⛔ item **(6f)**'s own recorded root-cause law says *"a census that greps for the stale NUMBER misses this; only a search for the stale CLAIM finds all three"* — **and here even a grep for the stale NUMBER would have found `:125` and `:131`.** ⛔ **`:1895` is the imminent hazard:** `TASK-980` is the very next row against this file and the board tells it to route *"THIS existing read"* (`SummonedUnit.cpp:2443`) through the fog accessor — while `AcquireEnemyNearPoint`'s declaration doc tells its implementer that read does not exist and was deliberately refused. ⇒ ⚖️ ***The author's §7(0a) generalisation — "a comment that documents why something was NOT done becomes false the moment someone does it, and it is invisible to every test, to a grep for the changed value, and to the author's own review of their own diff" — happened a THIRD time, in the same file, in the same diff, and its own adversarial pass did not close the class it discovered.***
  *Suggested fix (comment-only, ⛔ zero logic, ⛔ zero compile risk, and `TASK-987` has not built yet so the cycle costs no build): amend `:1891–1897` to state the TWO-term gate (zone disc AND notice-from-self) with the `TASK-979` item (6b) citation; correct `:125`'s `(600)` and `:131`'s `LeashRange (900)` to the shipped default and `GetEffectiveLeashRangeUU()`. ⛔ Do NOT re-type `2000`/`3000` as literals where the symbol will do.*

- **[WARN-1]** `SummonedUnit.cpp:2478` (the bound) vs `:1928` (an unnamed caller) — ⚠️⚠️ **THE ITEM-(6b) BOUND LANDS ON FOUR CALL SITES AND THE HANDOFF DESCRIBES ONE. THE SECOND FAMILY IS THE `DEFEND` STANCE, AND ITS DISC IS WIDER THAN THE GUARD CIRCLE THE HANDOFF SIZED AGAINST.** `AcquireEnemyNearPoint` is called from **`:1928` (DEFEND)**, `:2059`, `:2097`, `:2101` (the three grouped-zone tiers). §5(6b) and `J-F28` are written entirely about the grouped guard circle (`GroupRadiusMax = 5000`, 2.5×). **Measured myself:** the DEFEND disc is `CastleHalfWidth + DefendRadius` (`:2609`), with `DefendRadius = 1281.f` (`SummonedUnit.h:1309`) and the code's own derivation log at `:2614–2615` recording **≈3656.85 + 1281 ≈ 4937.9 uu at the shipped 9× castle** ⇒ **2.47× the 2000 notice radius**, and a defender standing at one wall face is **≈7,300 uu** from a besieger at the opposite face. ⇒ **a Blue Standard unit under DEFEND no longer notices an enemy on the far side of its own castle**, where before the diff it did (the lane had no self-distance term at all). ⚠️ **That is the shape `TASK-574` was written to repair** — its own comment at `:1921–1927` records *"DEFEND could never acquire the besiegers standing at the gate"* and the header at `:1289–1294` records *"every defender walked home while the castle was battered, and no bounds readback could see it."* ⛔ **NOT graded a BLOCKER:** it is literally the commanded lane, Jonathan's *"due to fog OR ANYTHING"* rules it in, the fall-back is `EnterAdvance(OwnCastle)` (nothing bricks), and the magnitude the author DID declare (84% blind at 2.5×) is within 1% of the DEFEND figure. **But it is a second, differently-sized consequence on a lane with a scar, and it is unstated.** *Suggested fix: add the DEFEND caller and the ≈4,938 uu figure to `J-F28` before it goes to Jonathan; put "DEFEND stance acquires nothing while the castle is hit on the far side" on `TASK-987`'s regression watch. It is a finding, never "flaky".*

- **[WARN-2]** `SummonedUnit.cpp:4465` — ⚠️ **ITEM (6b) SAYS THE CLAMP ACTS ON *ACQUISITION AND RETENTION*; THE DIFF IMPLEMENTS ACQUISITION ONLY, AND THE GAP GREW ~8× ON THIS DIFF.** `GetEffectiveLeashRangeUU()` composes `ResolveEffectiveLeashRangeUU(LeashRange, **AggroRadius**, LeashMarginMultiplier)` — the **raw** notice radius, never the fog-effective one. ⇒ **under fog a unit acquires at `609.6` and retains/chases to `3000`** — 4.9× its own fog vision, up from `900 / 600 = 1.5×` before this diff. A melee unit will now chase something it provably cannot see for **2,390 uu**. ⛔ **I do NOT grade this a BLOCKER and I do not think the author should have "fixed" it silently:** clamping retention collides head-on with *"commanded units DO NOT LOSE THEIR COMMANDS"* and with the deliberately leash-free grouped lane, so it needs a ruling, not a diff. **But item (6b)'s own sentence claims retention is covered and it is not, and the consequence is unstated beside `J-F27`'s 3000/5400.** *Suggested fix: state it as a rider on `J-F27` — "the 3000 is a CLEAR-WEATHER number; under fog it means chasing 3000 uu on 609.6 uu of sight" — and route the acquisition-vs-retention question to the manager for a ruling, on `TASK-980`'s row or a new one.*

- **[WARN-3]** `Tests/SiegeFogClampTest.cpp:1028–1031` — ⚠️ **A FALSE CLAIM INSIDE THE FIX FOR A FALSE CLAIM.** The comment above the CDO read states the accessor *"returns the live per-unit value … it tracks a Blueprint override and a bound card row, exactly as the game does."* ⛔ **On the base CDO it tracks NEITHER:** a Blueprint override lives on a *different* class's CDO (which `GetDefault<ASummonedUnit>()` never reaches — that is the whole point of `GetClassDefaultEngagementRadiusUU()`), and a bound card row is written onto the **spawned instance** by `LoadStatsAndStart`, which never touches any CDO (the author's own §7(2)). The read is correct and the operand is the right one; **only the justification beside it is wrong**, and it is wrong in the direction of claiming more coverage than the row has. *Suggested fix: reduce the sentence to what is true — "the shipped class default for `ASummonedUnit`, which is the operand this table's unit row is about."*

- **[WARN-4]** `handoffs/TASK-979-programmer.md:202–206` (§7 finding 5) — ⚠️ **THE ARITHMETIC IS RIGHT AND THE FINDING IS MATERIAL; THE WORD "BACKWARDS" IS TOO STRONG. THE BOARD IS INCOMPLETE, NOT INVERTED.** Board item (6d-iii-a) separates **(i)** the −4.8% firing cut (a `TASK-980` ceiling question) from **(ii)** the dead band, and (ii) says the top 100 uu is dead *"ANYWAY"* — i.e. **the board never claims `TASK-979` creates it.** What the board omits is the **1500-uu baseline**, and that omission is exactly what would make Jonathan read the 100 uu as a cost of this row. *Suggested fix: manager adds the 1500-uu baseline to `J-F29` before it is put to him. Full independent determination in §3 below.*

- **[WARN-5]** process — ⚠️ **THIS REPORT IS NOT THE BOARD'S GATE.** `TASKBOARD.md:18225–18273` defines the gate as **`TASK-985`**, one gate over **`TASK-979` + `TASK-980`**, reporting to **`qa/TASK-985.md`**. This dispatch directed a `TASK-979`-only review to `qa/TASK-979.md`. ⇒ **`TASK-985` is NOT discharged by this file**, and its items (2), (7b), (7c) and the `980`-side of (1a) remain unexamined. Recorded per `SC-§40` rather than implied. *Suggested fix: when `TASK-980` lands, `TASK-985` runs over both diffs and cites this file as the `979` half.*

- **[NIT-1]** `handoffs/TASK-979-programmer.md` — line citations drift 2–14 lines against the file as it now sits: `SummonedUnit.h:842`→**844**, access specifiers `1101`/`1696`→**1111**/**1706**; `SummonedUnit.cpp:4396`→**4410**, `:4425`→**4460**, `:4454`/`:4461`→**4468**/**4477**, `:2429`/`:2464`→**2443**/**2478**. `:1302`, `:1734`, `:1979`, `:2375` are exact. **Nothing is load-bearing** — every symbol is unique and findable — but `SC-§40 cl. 9`: a coordinate in prose rots. (The board's own item (6b) cites `:2362` for a function at `:2375`.)

- **[NIT-2]** `SummonedUnit.cpp:1811`/`:1835` vs `:2443` — `AcquireTarget` still reads the **raw** `AggroRadius` member while `AcquireEnemyNearPoint` reads `GetEngagementRadiusUU()`. Identical value, asymmetric idiom. ⚠️ Flagged **for `TASK-980`**: its firing seam must know which of the two sites it is routing, and `SiegeAcquisitionFunnelTest.cpp:786` pins `AcquireTarget`'s body to contain the token `AggroRadius` — **so a "consistency" rewrite of `:1811`/`:1835` to the accessor would turn that pre-existing row RED.** ⇒ ⛔ **Do not "tidy" it.** (I verified the probe: it passes today.)

- **[NIT-3]** `Tests/SiegeUnitNoticeRangeTest.cpp:608–610`, `:202` — `ASummonedUnit::UnitEngagementRadiusUU` is passed **by value** into a variadic `FString::Printf` and bound to `TestEqual`'s parameter, both of which **ODR-use** the `static constexpr` member. Legal only because C++17 makes `static constexpr` data members implicitly `inline`; UE 5.8 compiles C++20, so **no out-of-line definition is needed and there is no linker risk.** Recorded because it sits beside the author's own §10(8) UPROPERTY-NSDMI risk (`float AggroRadius = UnitEngagementRadiusUU;`, `SummonedUnit.h:1240`) and both are UHT/linker questions no reader can retire without the build.

- **[NIT-4]** `Tests/SiegeUnitNoticeRangeTest.cpp:379` — the `Clamp` probe counts the **bare token**, so it would also catch `FMath::Clamp`, `Clamp(` and a hypothetical `ClampMin` inside the resolver body. That is a deliberate over-catch and it is **correct** here (a `.cpp` function body cannot legitimately carry `ClampMin`). No action; noted so a future reader does not "tighten" it to `FMath::Clamp` and lose the other spellings.

---

## §3 — 🧑 MY INDEPENDENT DETERMINATION ON `J-F29` (Jonathan is waiting on this)

**Verified at source, not relayed.** `Docs/Data/cards.csv` — `Archer` line 3 `Range = 2100` · `Wizard` line 30 `Range = 2100` · `Longbowman` line 12 `Range = 3600`. Pre-diff `AggroRadius = 600` / `LeashRange = 900` (`qa/TASK-996.md` §1(2), read at `SummonedUnit.h:1039`/`:1043` before this diff).

| | acquisition | firing | unusable band | real engagement envelope |
|---|---|---|---|---|
| **today (shipped)** | 600 | 2100 | **1500 uu** | **~900** (the leash drops beyond it) |
| **after `TASK-979`** | 2000 | 2100 | **100 uu** | **2100** (effective leash 3000 > 2100) |

⭐ **THE AUTHOR'S ARITHMETIC IS CORRECT. `1500 → 100` is a genuine 15× improvement, and `TASK-979` is the row that FIXES most of `J-F29` rather than the row that causes it.** Both halves check out independently: the dead band is real today, and the leash lift is what makes the *whole* 2100 usable rather than just the acquisition number.

⚠️ **TWO CORRECTIONS I OWE, ONE TO THE AUTHOR AND ONE TO THE FRAMING:**

1. **The board is INCOMPLETE, not backwards** (WARN-4). Item (6d-iii-a) marks the 100 uu *"DEAD ANYWAY"* and keeps the −4.8% firing question on a separate numbered clause. It never says this row creates the band. What it omits is the baseline.
2. ⛔⛔ **THE 15× IS A CLEAR-WEATHER FIGURE, AND NEITHER THE AUTHOR NOR THE BOARD SAYS SO.** Under fog every unit's acquisition is `min(anything, 609.6) = 609.6`, so the Archer's and Wizard's usable band under fog is `609.6` **before and after this row — unchanged, and unchanged by any `NoticeRange` cell.** The improvement is real and it is entirely a fair-weather improvement.

⇒ ⚖️ **RECOMMENDED WORDING WHEN IT GOES TO HIM:** *"Your Archer and Wizard print 2100 but today they only notice at 600 and let go at 900, so most of that card has never been usable. This row makes 2,000 of the 2,100 live in clear weather (under fog everyone is 609 regardless). Do you want the last 100 — one `NoticeRange` cell of 2100 on each, zero code?"* ⛔ **Not** *"this row leaves your two cards nerfed."*

---

## §3a — 🧑 JONATHAN'S MID-REVIEW RULING, USED AS AN ADVERSARIAL PREDICATE (not restated as a requirement)

⚖️ **His words, verbatim 2026-09-04:** *"all nonranged units should notice at 2000 without fog and notice at 609 in fog."*

✅ **THE PREDICTION HOLDS. THE RULING IS A NO-OP AGAINST WHAT `TASK-979` BUILT — and I checked it as a thing the diff had to satisfy, not as a thing to write down.** Traced end to end for a `Footman`:

| step | site | value |
|---|---|---|
| CDO default | `SummonedUnit.h:1240` `float AggroRadius = UnitEngagementRadiusUU;` | **2000** |
| the card cell | `cards.csv` has **no `NoticeRange` header** ⇒ `FCardRow::NoticeRange = 0.0f` (`CardRow.h:214`) | **0** |
| the resolver | `ResolveNoticeRadiusUU(2000, 0)` ⇒ branch 3, the sparse case (`SummonedUnit.cpp:4436`) | **2000** |
| clear-weather notice | `AcquireTarget` → `SeeingFrom(MyLocation, AggroRadius)` (`:1811`) | **2000** ✅ |
| fogged notice | funnel → `EffectiveVisionRadius(2000, true, Tuning)` = `min(2000, 609.6)` | **609.6** ✅ |

⇒ ⭐ **The channel's default DOES reach the cards that never opt in** — the majority of the roster, and the case nobody writes a test for. It reaches them because the sparse branch returns the *class default* rather than the row, and because the class default is the **constant** rather than a re-typed literal.

### ⛔ (1) FOG MUST NOT RAISE A MELEE ATTACK RANGE — ✅ **CONFIRMED, AND ON THIS DIFF IT IS STRUCTURALLY UNREACHABLE**

Two independent facts, both re-`Read` at **my** instant (⚠️ `SiegeFogStatics.cpp` is `TASK-981`'s file and was in flight — this is a **fresh** read, ⛔ not inherited from `qa/TASK-996.md`):

1. **`SiegeFogStatics.cpp:122` is `return FMath::Min(RequestedRadiusUU, Ceiling);`** — ⛔ **a `min`, ⛔ not a clamp**, with **no floor anywhere in the function**. Its own comment at `:119–121` names this hazard by name: *"⛔ MIN, ⛔ NOT CLAMP. There is no floor and none may be invented: a Cleric's 400 stays 400 and a melee unit's 120 stays 120."* ⇒ `min(120, 609.6) = 120`. **A Footman stays at 120.**
2. ⭐⭐ **AND ON THIS DIFF THE COLLAPSE IS NOT MERELY ABSENT — IT IS UNREPRESENTABLE.** My census over `SummonedUnit.cpp` for `EffectiveVisionRadius|FogVisionCeiling|ReadFogState|SiegeFogStatics` returns **0 occurrences in the entire file**. ⇒ **the unit cannot name the ceiling**, so no site in it can hand `609.6` to a firing gate. Both firing gates (`:1762`, `:2002`) compare `GetDistanceToTarget(...) <= AttackRange` against the raw member, and `AttackRange = Row->Range` (`:1288`) is **byte-untouched by this row**. **Notice and firing are two members, two columns and two code paths; nothing in this diff joins them.**

⇒ ⚖️ **The risk is real and correctly identified — and it is `TASK-980`'s to carry, not `979`'s.** `TASK-980` is the row that introduces `GetEffectiveFiringRangeUU()` and *does* route firing through the fog seam. ⛔ **Flagged forward as a `TASK-985` gate item: `TASK-980` must preserve the `min` semantics AND must not apply its firing accessor in a way that lengthens a melee 120.** ⭐ And the standing rule *"notice is identical to firing for ranged units"* **deliberately does not extend to non-ranged ones** — a melee unit notices at 2000 precisely so it can **chase**, and attacks at 120. That asymmetry is the design, and this diff preserves it.

### ⚠️ (2) THE CLERIC AND THE WITCH — ✅ **NEITHER IS DRAGGED TO 2000, AND NEITHER IS TOUCHED BY FOG**

`Docs/Data/cards.csv` read directly: **`Cleric` line 13 — `Range 400`, `bRanged=false`, Profile `Support`** · **`Witch` line 33 — `Range 400`, `bRanged=false`, Profile `Support`**, whose own `Notes` cell states *"Range 400 is the ungrouped veil radius not an attack range (WITCH-4)"*.

- ⛔ **Nothing in this diff assumes "range means attack range".** `NoticeRange` is a **separate `FCardRow` column** (`CardRow.h:214`) bound over a **separate member**; `AttackRange = Row->Range` is unchanged. The Cleric's heal radius and the Witch's veil radius are both `AttackRange` reads and are **byte-untouched**. Their **notice** rises to 2000 (correct — a Cleric must notice a damaged friendly in order to walk to it); their **400 stays 400**.
- `min(400, 609.6) = 400` ⇒ **fog leaves both untouched**, exactly as `SiegeFogStatics.cpp:119–120` says by name.
- ⚠️ Forward-flag, from my own `qa/TASK-996.md` §1(4): `ResolveWitchPositionCircle` takes the `AttackRange`-derived radius on **every early-out path**, and `Follow` is the spawn default for a follow-eligible Blue unit ⇒ **a fresh Witch takes it as the COMMON case, not an edge.** ⛔ **`TASK-980` must NOT route that site through a fog-clamped firing accessor** — it would be a live Witch nerf. `TASK-985` item (2) exists for this; **this row does not breach it.**

### ⚠️ (3) THE MINER — ⛔ **ONE CLAUSE OF THE RELAY NEEDS CORRECTING, AND IT MATTERS**

⛔ **`2000` is NOT the correct value for a miner under this ruling, and the diff correctly does not give it one.** `AMinerUnit`'s CDO carries `AggroRadius = 0.f` (`MinerUnit.cpp:72`, byte-untouched) as **half of a class contract**, and the resolver's **first** branch — `!(ClassDefaultRadiusUU > 0.f)` — returns `0` and **ignores the row entirely**. A miner resolves to **0**, not 2000. ⛔ Board item (3) makes raising either seal an **AUTOMATIC FAIL**, and *"all nonranged units"* cannot sanely be read as *"give the sealed non-combatants an aggro radius"*: `AMinerUnit`/`ASorcererUnit` are `CanEverAttack() == false` with `Profile None`, so the number would be inert at best and an un-sealing at worst. ⇒ **the ruling does not reach them, and the diff is right to keep them at 0.**

⛔⛔ **AND THE RULING DOES NOT LAUNDER §7(2).** That finding is about the **binding path**, not the number: `LoadStatsAndStart` writes the **instance** and never the CDO, so a CDO-only assertion is green regardless of what the binding does. **A correct value arriving through a broken path is still a broken path, and it produces the wrong value the moment any card sets the cell.** That is exactly why the seal is asserted on the **pure function** (`SiegeUnitNoticeRangeTest.cpp:341–348`, `ResolveNoticeRadiusUU(0.f, 3600.f) == 0.f`), which **can** fail, in addition to the CDO rows, which cannot. ⇒ **§7(2) stands unchanged, and board item (3) still needs the amendment recommended in §2.**

### 🧑 THE CONSEQUENCE JONATHAN SHOULD HEAR — ✅ **FIGURE CONFIRMED**

`(2000 − 609.6) / 2000 = 0.6952` ⇒ ⭐ **in fog, a non-ranged unit's chase radius shrinks 69.5%** (2000 → 609.6). **Melee becomes markedly more passive under fog and will not charge across open ground.** That follows directly from his ruling and is almost certainly what he wants — but he has not been told it. ⭐ **Cross-check that the figure is in the right family:** the same arithmetic on the Longbowman's 3600 gives **−83.1%**, which is the number already shipped in `SiegeFogStatics.h`'s prose and which the board confirms is still correct. The two agree ⇒ **69.5% is right.**

⚠️ **ONE RESIDUAL ON THE PREDICATE, DECLARED (`SC-§40`):** *"every non-ranged card gets 2000"* is proven **in C++** and is **UNVERIFIED against the shipped `BP_Unit_*.uasset` archetypes**. `AggroRadius` is `EditAnywhere`, so any `BP_Unit_*` that ever had the value touched carries a serialised override that **silently wins** over the C++ default — and it would win as the **old 600**, invisibly, on exactly the cards nobody re-checks. The diff records the hazard (`SummonedUnit.h:1233–1237`) and the per-class read is what makes such an override survive rather than be stamped over; ⛔ **but the scan itself needs the editor and I do not have one.** It cuts **toward** risk, not away.

---

## §4 — WHAT I COULD NOT DETERMINE (`SC-§40` — an absence is a measurement)

1. **Anything at runtime.** Pure source read: no compile, no PIE, no editor, no MCP. Every claim above is textual or arithmetic and reproducible from its `file:line`.
2. **Whether the two UHT/linker constructs compile** — the UPROPERTY NSDMI at `SummonedUnit.h:1240` and the ODR-use of the `constexpr` member (NIT-3). Precedent exists for both; **neither can be retired without `TASK-987`'s build.** ⛔ If UHT objects to the NSDMI, the fallback is the constructor — ⛔ **never a typed `2000.f`.**
3. **Whether a `.uasset` Blueprint CDO overrides `AggroRadius`.** Not re-scanned; inherited as open from `qa/TASK-996.md` and sharpened by §3a's residual. ⚠️ It cuts **toward** risk: an override would silently win, which the diff records at `SummonedUnit.h:1233–1237` and tests at `SiegeUnitNoticeRangeTest.cpp:355–358`.
4. **The gameplay load** (far more units simultaneously engaged ⇒ pathing/AI/animation) and the newly-live per-candidate fog cut. **Both declared unmeasured by the author and NOT guessed here.** Both need a profiled match.
5. **`TASK-985`'s `TASK-980` half** — out of this review's scope entirely (WARN-5).

---

## Notes for build-master (`TASK-987`) — ⛔ THIS ROW IS **qa-failed**, DO NOT COMPILE IT AS-IS

1. ⛔ **`TASK-979` must return through `gameplay-programmer` for BLOCKER-1 before it enters your compile.** The fix is **three comment blocks in one file** — ⛔ zero logic, zero new symbols, zero test changes. It cannot move a number.
2. ⭐⭐ **NOTHING ELSE IN THIS DIFF NEEDS UNWINDING.** The channel, the resolver, the leash inversion, the commanded-lane bound, the `FCardRow` field, the re-derived fog rows and all four new tests are **correct as they stand**, verified above — and Jonathan's mid-review ruling is a **no-op against them** (§3a). Do not let the fix cycle touch them.
3. ⚠️ **TWO CONSTRUCTS ONLY YOUR BUILD CAN RETIRE** (§4 item 2): the UPROPERTY NSDMI `float AggroRadius = UnitEngagementRadiusUU;` (`SummonedUnit.h:1240`) and the `constexpr` ODR-uses in `SiegeUnitNoticeRangeTest.cpp`. If UHT or the linker objects, **the fallback is the constructor / an out-of-line definition — ⛔ NEVER a hand-typed `2000.f`.**
4. ⚠️ **REGRESSION WATCH, from WARN-1:** *"defenders under the DEFEND stance ignore besiegers on the far side of their own castle."* That is `AcquireEnemyNearPoint`'s new notice bound meeting a ≈4,938 uu DEFEND disc — **a finding, never "flaky".**
5. ⚠️ **REGRESSION WATCH, from `qa/TASK-996.md` WARN-1 and confirmed here:** raising the notice radius newly **activates** the funnel's per-candidate `RemoveAll` collision-query loop under fog (`SiegeCombatStatics.cpp:207`'s strict `<`). Any fog-plus-many-units slowdown is **this**.
6. ⚠️ **REGRESSION WATCH, from §3a:** *"melee units are passive under fog and will not charge"* is **expected** — a 69.5% chase-radius reduction is the direct consequence of Jonathan's ruling. ⛔ Do not let anyone "fix" it back.
7. ⛔ **`TL-§5c` — I state no suite total as an executed number.** My **declared** census at my instant is **445** (34 files); derive yours at your own instant, and expect `TASK-981`'s file to move it.
8. ⚠️ **`TASK-985` still owes the `TASK-980` half** (WARN-5). This file is the `979` half only.

---
---

# §L2 — QA LOOP 2 (RE-REVIEW OF THE REPAIR)

**Date:** 2026-09-04 · **Agent:** qa-reviewer · **Loop 2 of max 3** · **Input:** `handoffs/TASK-979-programmer.md` §11 (the repair record) + the board row at `TASKBOARD.md:17991–18085`
**Scope:** ⛔ **RE-REVIEW OF THE REPAIR, not a fresh review.** The four loop-1 load-bearing verifications were **spot-checked** for survival, ⛔ not re-derived.
**Writes:** this append + one Slack post. ⛔ Zero source edits, zero compile, zero engine, zero MCP, zero Git.

## Verdict: ✅ **PASS** — **0 BLOCKER** · 3 WARN · 2 NIT

⛔ **`TL-§5c` — NOTHING WAS COMPILED OR EXECUTED FOR THIS RE-REVIEW EITHER.** Every statement is a source read or arithmetic, reproducible from its `file:line`. **No executed suite number appears.**

⭐ **BLOCKER-1 IS CLOSED AT ALL SITES, AND THE REPAIR IS BETTER THAN THE FIX I ASKED FOR.** The author replaced the three stale numbers with **symbol names** rather than re-typing the new values, so the corrected lines **cannot rot again** — which is more than the suggested fix required.

---

## §L2(A) — ⭐⭐ MY INDEPENDENT CLAIM-SHAPE CENSUS: **IT IS TEN, NOT NINE**

⛔ **I did NOT accept the author's enumeration and I did NOT re-run a value grep and conclude agreement.** I re-derived the census from the behaviours the diff changed, swept the **whole module** rather than the two edited files, and adjudicated every hit with `Read` (`SC-§38a`).

| behaviour the diff changed | claim shapes I censused | scope |
|---|---|---|
| `AggroRadius` 600 → per-unit `UnitEngagementRadiusUU` | `[Aa]ggro` · `\b600\b` · `profile constant` · `NOT A CARD STAT` | ⛔ **all of `Source/`** |
| `LeashRange` 900 → `GetEffectiveLeashRangeUU()` | `[Ll]eash` · `\b900\b` | ⛔ **all of `Source/`** |
| `AcquireEnemyNearPoint` gained a from-self bound | `AcquireEnemyNearPoint` · `SeeingFromUnbounded` · `UNBOUNDED\|[Uu]nbounded` · `\b700\b` | ⛔ **all of `Source/`** |

⭐ **The scope widening is the whole point, and it is where the tenth surfaced.** The author censused *"both files"*; a claim about `AcquireEnemyNearPoint`'s eligibility can live in **any** file that documents the vision contract — and one does.

### ✅ THE AUTHOR'S NINE — ALL GENUINE, ALL VERIFIED FIXED BY `Read`

| # | site (post-repair coords, mine) | state at my instant |
|---|---|---|
| 1 | `SummonedUnit.h:1231` `AggroRadius` doc | ✅ old line quoted-as-struck, then corrected |
| 2 | `SiegeFogClampTest.cpp:1095–1116` | ✅ retired + inverted, argument re-derived |
| 3 | `SummonedUnit.cpp:1794–1805` `AcquireTarget` para | ✅ quoted-as-struck + amended, closing sentence kept |
| 4 | `SummonedUnit.h:125–128` | ✅ **now carries NO number** — names `UnitEngagementRadiusUU` + the `NoticeRange` cell |
| 5 | `SummonedUnit.h:134–136` | ✅ **now names `GetEffectiveLeashRangeUU()`** and calls the raw member "only its FLOOR" |
| 6 | `SummonedUnit.h:1915–1960` (the BLOCKER) | ✅ **rewritten as TWO NAMED TERMS**, + 4 callers, + the `TASK-980` instruction |
| 7 | `SummonedUnit.h:153–158` | ✅ three numbers removed; the old text quoted as historically stale |
| 8 | `SiegeFogClampTest.cpp:1031–1038` (my WARN-3) | ✅ reduced to what is true |
| 9 | `SiegeFogClampTest.cpp:1054–1057` | ✅ label + a 3-line comment drawing the gather-vs-pick distinction |

⭐ **Site 7's pre-existing `Archer 700` is DECLARED, NOT LAUNDERED**, and the disposal is the right one: the numbers were **deleted rather than corrected to 2100**, so the line names members and cannot go stale a third time. ✅ I confirm `cards.csv` line 3 `Range = 2100` and that this defect predates `TASK-979`.

### ⛔⛔ THE TENTH, WHICH NEITHER THE AUTHOR NOR MY LOOP-1 REPORT NAMED — `SiegeCombatStatics.h:137–141`

`Read`, character-exact:

> `*  ⭐ A site whose eligibility is NOT a range from the viewer — a commanded unit's zone disc`
> `*  (`AcquireEnemyNearPoint`, gated on a disc around a COMMANDED POINT) and a chain zap's`

⛔ **FALSE as of this diff, and it is the SAME CLAIM AS BLOCKER-1 IN A THIRD FILE.** `AcquireEnemyNearPoint`'s eligibility is now **two terms**, and term (2) — `if (Distance > NoticeRadiusUU)` at `SummonedUnit.cpp:2495`, measured from **self** — **is** a range from the viewer. The author's own repaired declaration says so verbatim (*"THE ELIGIBILITY GATE IS **TWO TERMS**"*, `SummonedUnit.h:1915`).

⭐⭐ **AND IT IS THE ORIGIN OF THE COPY THE AUTHOR ALREADY AMENDED ONCE.** `SiegeCombatStatics.h:144–146` reads *"Handing over some OTHER radius (a unit's `AggroRadius`, a tower's `AttackRange`) would narrow these two sites ⛔ WITH FOG OFF — a shipped behaviour change wearing a fog card's commit message."* That is **near-verbatim** the `TASK-838` clause the author struck at `SummonedUnit.cpp:2399–2401`. ⇒ ⚖️ ***The author's own §7(1) law — "the claim travels by copy-paste" — is confirmed a second time, and the un-amended copy is the one in the header the clause was copied FROM.***

⚖️ **WHY I GRADE THIS **WARN** AND ⛔ NOT A BLOCKER — the reasoning, not a softening:**
1. ⛔ **`SiegeCombatStatics.{h,cpp}` is NOT on `TASK-979`'s `names:` line** (`TASKBOARD.md:18085`), and the author declared it "NOT touched, deliberately". `TASKBOARD.md:15–16` makes `names:` **the manager's exclusively**. ⇒ **Grading BLOCKER would order a fence breach or produce a no-op loop.** I will not spend loop 3 on an edit the author is forbidden to make.
2. **Only the framing clause is false.** The counterfactual at `:143–148` is still **literally true** — nobody handed a radius to `SeeingFromUnbounded`, and doing so *would* still narrow the **gather** with fog off. The doc's **purpose** (justifying why the query is unbounded) survives intact; `SummonedUnit.cpp:2412` still calls `SeeingFromUnbounded`.
3. **The imminent-hazard test that made BLOCKER-1 blocker-grade is already discharged elsewhere.** `TASK-980`'s implementer is now told, on the **declaration it must edit**, that the read exists and must not be re-added (`SummonedUnit.h:1926–1927`). The header is a secondary reference, not the work surface.

⇒ ⛔ **ROUTED TO THE MANAGER, not left silent** (`SC-§40`): amend a `names:` line to admit the 4-line comment fix — **preferably onto `TASK-980`**, which owns this seam next and will be reading `FSiegeVisionQuery`'s contract anyway. *Fix, in substance: `AcquireEnemyNearPoint`'s eligibility is TWO terms since `TASK-979` item (6b); the **GATHER** is unbounded and the **PICK** is not — the exact distinction the author already wrote at `SiegeFogClampTest.cpp:1054–1056`.*

### ✅ FALSE POSITIVES I CHECKED AND REFUSED TO RAISE (`SC-§40` — a refused finding is a measurement)
`Tower.cpp:411–415` makes the same "would narrow … WITH FOG OFF" claim but is scoped to **the tower's own gather**, which genuinely has no range to hand over ⇒ **still true.** · `SorcererUnit.h:38` / `SummonedUnit.cpp:2072` *"the two AcquireEnemyNearPoint tiers"* ⇒ **correct** (the monotone upgrade is not a tier). · `SummonedUnit.h:1354` `600.f` = projection-extent Z · `:2320` `(600)` = Cavalry **movement speed** · `:1271` `LeashRange = 900.f` = the live **floor**, correct. ⇒ **I confirm the author's own §11(E)(5) refusals.**

---

## §L2(B) — ⚖️⚖️ MY RULING ON THE DEFEND DECLINATION (loop-1 WARN-1)

# ⚖️ **THE DECLINATION IS SOUND. `TASK-574` IS ⛔ NOT REGRESSED. MY LOOP-1 FRAMING WAS TOO STRONG AND I WITHDRAW IT.**

⛔ **This is an explicit ruling, not a deferral.** WARN-1 is **CLOSED** as a declared consequence. It does ⛔ not carry forward as an open dispute, and it is ⛔ not a BLOCKER.

**Operands re-verified by `Read` at my own instant — ⛔ not relayed from the handoff:**

| operand | site | value | mine vs author |
|---|---|---|---|
| `DefendRadius` | `SummonedUnit.h:1327` | `1281.f` | ✅ exact |
| the disc | `SummonedUnit.cpp:2626` `CastleHalfWidth + DefendRadius` | ≈`3656.85 + 1281` = **4937.85** | ✅ exact |
| callers | `:1928` · `:2059` · `:2097` · `:2101` | **4** | ✅ exact |
| the fallback | `EnterAdvance(OwnCastle)` (`SummonedUnit.h:1880–1884`) | **untouched** | ✅ confirmed |

### THE THREE REASONS, IN ORDER OF WEIGHT

**1. ⭐⭐ THE DECISIVE ONE, AND IT IS THE AUTHOR'S — AN EXEMPTION WOULD HAVE MANUFACTURED A BLOCKER UNDER A DIFFERENT GATE ITEM.** I checked the board text myself rather than taking the citation: **`TASKBOARD.md:18314`, item (7c)** grades it BLOCKER *"if the commanded-lane bound exists ONLY under fog, clear-weather behaviour silently differs ⇒ BLOCKER, and say so in those terms."* `TASK-980` routes term (2) through the fog accessor for **all four callers**; exempting DEFEND from term (2) therefore produces **exactly** one stance whose clear-weather behaviour differs from its fogged behaviour. ⇒ ⛔ **The fix I implied in loop 1 was itself blocker-grade under item (7c). The author was right to refuse it, and right about why.**

**2. ✅ TASK-574's REPAIR IS INTACT, AND THE TEST THAT SETTLES IT IS A REACHABILITY TEST, NOT A MAGNITUDE ONE.** I applied my own discriminator: ***is there any defender position from which the besieger is unacquirable?***
- **TASK-574's defect:** the disc lay **entirely inside the keep** ⇒ `AcquireEnemyNearPoint` returned nothing for **any** defender at **any** position. **Total and position-independent.**
- **This bound:** a per-candidate distance cut from **self**. A defender within `2000` uu of the besieger **acquires it**, and the fallback `EnterAdvance(OwnCastle)` keeps the unit walking toward the castle rather than idling. **Partial and position-dependent.**
⇒ ⛔ **Different defect, strictly narrower, nothing bricks, no unit idles.** The author's characterisation — *"a defender does not notice a besieger on a face it is not standing on"* — is accurate and is the correct statement of the residue.

**3. ✅ ACQUISITION vs MOVEMENT IS A REAL LINE, AND THE BOARD DRAWS IT.** Item (6b) at `TASKBOARD.md:18036`: *"THE CLAMP ACTS ON TARGET ACQUISITION AND RETENTION. IT NEVER ACTS ON THE ORDER ASSIGNMENT."* Re-pointing a defender at the battered face changes the **goal**, which is order-assignment territory ⇒ correctly out of scope, and correctly identified as *"the silent rebalance riding a fix cycle"* this batch has paid for repeatedly.

⇒ 🧑 **THE RESIDUE IS A BOARDABLE MOVEMENT QUESTION FOR JONATHAN**, not a code defect on this row: *"should a defender sweep its own walls, or hold the face it is on?"* It is recorded in three places (`SummonedUnit.h:1943–1960`, `SummonedUnit.cpp:2439–2455`, `J-F28`) and on `TASK-987`'s regression watch. ✅ **That is the correct disposition of a WARN the board expressly permitted to be declined** (`:18070` — *"you MAY resolve it OR decline it — but the declination must be EXPLICIT AND MEASURED, never silence"*). **It was explicit and it was measured.**

✅ **AND I ACCEPT THE AUTHOR'S CORRECTION OF ME:** `:2059` is the **position→attack MONOTONE UPGRADE**, not a third zone tier — verified at `SummonedUnit.cpp:2059` (`if (AActor* UpgradeTarget = AcquireEnemyNearPoint(...))`) and documented at `SummonedUnit.h:1935–1938`. My loop-1 report called all three grouped callers "tiers" and **that was wrong**. The upgrade consequence (a held position-tier target can no longer be upgraded to an out-of-reach attack-zone enemy) is a real, separate effect that I missed.

### ⛔⛔ **BUT THE `83.6% ≈ 84%` "CONVERGENCE" IS ⛔ FALSE CORROBORATION — [WARN-6]**

The dispatch asked whether this is genuine convergence or two figures that merely look alike. **It is the latter, and the author's stated reason for trusting the number is a non-sequitur.**

Both figures are **the same formula with the same numerator**, evaluated at two radii that happen to be close:
- grouped: `1 − (2000 / 5000)²` = `1 − 0.1600` = **84.00%**
- DEFEND:  `1 − (2000 / 4937.85)²` = `1 − 0.1641` = **83.59%**

⇒ ⛔ **`4937.85` and `5000` differ by 1.24%, so the outputs are forced to agree.** There is **one** operand (the 2000 notice radius) and **one** function, not *"two different operands, one figure."* The agreement would have been just as tight if **both** derivations were wrong about the notice radius — which is precisely what corroboration must exclude and this does not.

⛔ **The arithmetic itself is CORRECT** (I re-derived both). ⛔ **Only the confidence claim is invalid.** *Action: do ⛔ NOT quote the near-agreement to Jonathan as a cross-check. Quote the two disc sizes and the one formula.*

---

## §L2(C) — ⛔ THE `:5572` INVARIANT: ARITHMETIC VERIFIED, AND MY DETERMINATION

⚠️ **First, a coordinate correction: the comment is at `SummonedUnit.cpp:5589–5592`, ⛔ not `:5572–5575`.** `:5573` is the 2-arg `GetDistanceToTarget` overload; the claim lives inside the 3-arg one at `:5579`. (See NIT-5.)

**The text, `Read`, character-exact:** *"the castle's origin sits at the center of a 7313.7 x 7384.5 uu footprint and would never come within Range/AggroRadius of a unit standing at its walls"*.

### ✅ THE ARITHMETIC IS EXACT — I RE-DERIVED EVERY FIGURE

| notice radius | margin = `3656.85 − notice` | author | mine |
|---|---|---|---|
| `600` (pre-diff) | `3056.85` | 3056.85 | ✅ |
| `2000` (new default) | `1656.85` | 1656.85 | ✅ |
| `3600` (Longbowman) | **`56.85`** | 56.85 | ✅ |

Margin cut: `(3056.85 − 56.85) / 3056.85` = **98.14%** ⇒ "cut by 98%" ✅. Half-width `7313.7 / 2 = 3656.85` ✅. And `3656.85` is the **binding** operand here (the minimum origin-to-boundary distance is the half of the **shorter** dimension) ⇒ the author picked the right one. ✅

### ⚖️ **MY DETERMINATION: 56.85 uu IS ACCEPTABLE, AND THE INVARIANT DOES ⛔ NOT NEED RE-DERIVATION BEFORE `TASK-993` LANDS. TWO REASONS, AND THE SECOND MATTERS MORE THAN THE MARGIN.**

**1. ⭐⭐ NOTHING BRANCHES ON THIS CLAIM. It is a JUSTIFICATION, not a runtime invariant.** It explains why `ActorGetDistanceToCollision` (closest point on collision) is used instead of actor origin — and that metric is applied **unconditionally** at `SummonedUnit.cpp:5595`. ⇒ **If the margin goes negative, no code path changes and no value moves.** The cost of falsification is documentation accuracy (the `HIGH-§1`/BLOCKER-1 class), ⛔ **not behaviour.** There is no state in which a negative margin produces a wrong result.

**2. ⚠️ AND `TASK-993` AS SPECCED DOES NOT FALSIFY IT ANYWAY — the author slightly over-alarms.** `TASK-993`'s cell is **`3600`**, and `3600 < 3656.85` ⇒ **the claim stays TRUE after `TASK-993` lands.** Falsification needs a cell **above ≈3656.85**, which no boarded row proposes. ⇒ **`TASK-993` is not blocked on this and `TASK-985` need not gate it.**

### ⛔⛔ **BUT THE REAL HAZARD IS NOT THE CELL — IT IS THE MESH, AND NOBODY HAS NAMED IT — [WARN-7]**

⭐ **`3656.85` IS A RUNTIME MEASUREMENT, ⛔ NOT A CONSTANT.** `ResolveDefendEngagementRadius` reads it live: `OwnCastle->GetActorBounds(/*bOnlyCollidingComponents=*/ true, …)` at **`SummonedUnit.cpp:2603`**, then `FMath::Max(CastleBoxExtent.X, CastleBoxExtent.Y)` at `:2610`.

⇒ ⛔ **A castle mesh re-import can falsify the `:5589` claim with ZERO code change and ZERO data change** — and `Content/Meshes/SM_Castle.uasset` is **modified in the working tree right now**. The same re-import simultaneously moves the DEFEND disc, the `2.469×`, the `83.6%` and the opposite-faces `7313.7`. **Every figure in §L2(B) rides on one live bounds query.** *Action: `TASK-987`'s regression watch and any future castle-mesh row should re-read the DEFEND derivation log line (`:2636–2638`) rather than trusting the transcribed 3656.85.*

⚠️ **AND A LIVE ~35 uu INCONSISTENCY BETWEEN TWO SHIPPED COMMENTS, both pre-existing, declared rather than laundered:** `:5589` states the footprint as `7313.7 x 7384.5` ⇒ `max` half-extent = **3692.25**, but `:2632` states the measured **`max(X,Y)`** half-width as **3656.85** (= `7313.7 / 2`, i.e. the half of the **smaller** dimension). **These cannot both describe the same castle under the same query.** ⛔ Neither is this row's to fix and **neither changes any conclusion** — I re-ran §L2(B) at 3692.25 and the disc becomes 4973.25 (`2.487×`, 83.8% blind): **immaterial.** But the author relayed `3656.85` into four new places without noticing the other figure disagrees. *Routed to the manager alongside WARN-6.*

---

## §L2(D) — ✅ THE `TEXT(...)` CHANGE IS GENUINELY BEHAVIOUR-NEUTRAL — **VERIFIED, NOT ACCEPTED**

⭐ **This is exactly where a silently weakened assertion hides, so I traced the operand rather than reading the claim.**

1. **The struct has two members** (`SiegeFogClampTest.cpp:1041–1045`): `const TCHAR* Label;` and `float RequestedUU;`.
2. **The changed row `:1057` keeps its operand:** `{ TEXT("the UNBOUNDED gather sentinel (…)"), TNumericLimits<float>::Max() }`. ⛔ **`RequestedUU` is untouched.**
3. **Both consuming loops assert on `RequestedUU` and use `Label` ONLY as a message argument:**
   - `:1060–1070` — `TestEqual(Printf(…, Site.Label), EffectiveVisionRadius(Site.RequestedUU, false, Tuning), Site.RequestedUU, Exact)` ⇒ **`Label` is the 1st (message) parameter; both compared operands are `RequestedUU`.**
   - `:1075–1084` — `TestFalse(Printf(…, Site.Label), EffectiveVisionRadius(Site.RequestedUU, false, Tuning) < Site.RequestedUU)` ⇒ **same.**
4. **Format/argument counts check out:** `:1064–1067` has exactly one `%s` and one argument; `:1079–1082` the same. ⛔ No `%.1f` was introduced against a `const TCHAR*`.

⇒ ⚖️ **NO ASSERTION OPERAND, TOLERANCE OR PREDICATE CHANGED. The label cannot weaken an assertion because it never reaches one.** ✅ **The author's claim is accurate, and its refusal to call the repair "comment-only" is the correct disclosure** — the distinction it drew against itself is real and I confirm it.

---

## §L2(E) — THE REMAINING VERIFICATIONS

### ✅ THE FOUR LOOP-1 CLAIMS — SPOT-CHECKED, ALL STILL INTACT (⛔ not re-derived, per scope)
| claim | spot-check at my instant | result |
|---|---|---|
| no-clamp | `ResolveNoticeRadiusUU` (`SummonedUnit.cpp:4427–4454`) — seal first, row branch **unmodified**, sparse third; **zero `FMath::Min`, zero `Clamp`** | ✅ intact |
| leash bit-identity at 600 | `:4477` still `FMath::Max(LeashRangeUU, NoticeRadiusUU * SafeMultiplier)`, floor at 1.0 at `:4470` | ✅ intact |
| the clamp test can fail | rows (a)(b)(c)(d) at `:1120–1183` — **every operand still READ** (`UnitNoticeRadiusUU` from the CDO, `Tuning.FogVisionCeilingUU`); revert to 600 ⇒ (a) `600 > 609.6` false ⇒ RED | ✅ intact |
| `445` declared, `+4` | my own re-census: **445 across 34 files**, `SiegeUnitNoticeRangeTest.cpp` = 4, `SiegeFogClampTest.cpp` = 9 | ✅ **unchanged** |

### ✅ SUITE (`TL-§5c`)
**445 DECLARED across 34 files** at my instant — **identical to my loop-1 census.** ⇒ ✅ **the repair added zero `IMPLEMENT_*_AUTOMATION_TEST` macros, as claimed.** The word **"declared"** is used throughout §11(F) and ⛔ **no executed number appears anywhere in the handoff.** ✅ Both lanes agree at 445/34.

### ✅ `TASK-981`'s FENCE — THE AUTHOR STAYED OUT
Census of `TASK-979|NoticeRange|UnitEngagementRadiusUU|GetEffectiveLeashRangeUU` over the whole module returns **exactly 5 files**: `CardRow.h` · `SummonedUnit.h` · `SummonedUnit.cpp` · `Tests/SiegeFogClampTest.cpp` · `Tests/SiegeUnitNoticeRangeTest.cpp`. ⇒ ⛔ **`SiegeFogStatics.{h,cpp}` and `Tests/SiegeFogTest.cpp` carry ZERO `TASK-979` symbols.** ✅ **No `names:` breach; `TASK-981`'s uncompiled diff is undisturbed.**

### ✅ NO NEW `C2248` SURFACE
Exactly **three** access specifiers in `SummonedUnit.h` — `public:` **200**, `protected:` **1129**, `private:` **1724** (I swept for **indented** specifiers too: none). Members at **1258 / 1271 / 1284** ⇒ protected. Accessors at **945 / 952 / 961 / 962** ⇒ public. Negative sweep `->AggroRadius|->LeashRange|->LeashMarginMultiplier|\.AggroRadius|\.LeashRange` over `Tests/` ⇒ **0 matches** (scanner alive: the same directory returns `[Aa]ggro` hits in four files). ✅ **Zero compile-blocking access sites.**

### ✅ WARN-2 WAS GENUINELY ROUTED, ⛔ NOT QUIETLY DROPPED — confirmed in **three** places
`SummonedUnit.h:916–924` (shipped source, the `J-F27` fog rider, ending *"routed to the manager as an open question on TASK-980's row"*) · handoff §11(D) · the board status line at `TASKBOARD.md:17996`. ⚠️ **Recorded absence (`SC-§40`): the DESTINATION row does not yet carry it** — my grep over `TASKBOARD.md` finds no retention/acquisition rider on `TASK-980`. ⛔ **That is not the author's failing** — `TASKBOARD.md:15–16` reserves `spec:` edits to the manager. ⇒ **ACTION ON THE MANAGER**, not a defect of this row.

### ✅ WARN-4 CONCEDED — matches my loop-1 finding exactly ("BACKWARDS" withdrawn; the board is INCOMPLETE, missing the 1500-uu baseline). ✅ **WARN-3 fixed** (`:1031–1038` now states only what is true). ✅ **WARN-5 acknowledged.** ✅ **NIT-2 correctly left untouched** — I re-confirm `SiegeAcquisitionFunnelTest.cpp:786` still pins `AggroRadius` inside `AcquireTarget`'s body, so a "consistency" rewrite would turn a pre-existing row RED.

### ⭐ NIT-1 / `SC-§38` — THE RE-ANCHORING IS **SUBSTANTIALLY COMPLETE**, AND I CHECKED EVERY ROW
I verified all 16 symbols in §11(C) against the file. **14 exact** (`public:`/`protected:`/`private:` 200/1129/1724 · constant 852 · `AggroRadius` 1258 · `LeashRange`/`LeashMarginMultiplier` 1271/1284 · accessors 945/952/961/962 · `ResolveNoticeRadiusUU` 882/4427 · `ResolveEffectiveLeashRangeUU` 4456 · `GetClassDefaultEngagementRadiusUU` 981/4485 · `AcquireEnemyNearPoint` **def** 2375 · the bound 2460/2495 · `DefendRadius` 1327 · the sum 2626). ⇒ ⭐ **`SC-§38` vindicated: the symbol is normative and it held.** Two residual rots in NIT-5.

---

## §L2 — Findings

- **[WARN-6]** `Source/GitClaudeUnrealTest/Siegebound/SiegeCombatStatics.h:137–141` — ⛔ **THE TENTH STALE SITE, NAMED BY NEITHER THE AUTHOR NOR MY LOOP-1 REPORT.** *"A site whose eligibility is NOT a range from the viewer — a commanded unit's zone disc (`AcquireEnemyNearPoint`…)"* is **false** since item (6b) added the from-self bound; and `:144–146` is the **un-amended origin** of the copy struck at `SummonedUnit.cpp:2399–2401`. ⛔ **Graded WARN and not BLOCKER only because the file is outside `TASK-979`'s `names:` fence** (`TASKBOARD.md:18085`), which `TASKBOARD.md:15–16` reserves to the manager. *Fix (4 comment lines, zero logic): state the gather-vs-pick distinction the author already wrote at `SiegeFogClampTest.cpp:1054–1056`. ⇒ **manager: amend a `names:` line, preferably `TASK-980`'s**, which owns this seam next.*
- **[WARN-7]** `SummonedUnit.cpp:2603` / `:2632` vs `:5589` — ⚠️ **EVERY FIGURE IN THE DEFEND ARGUMENT RIDES ON A LIVE BOUNDS QUERY, NOT A CONSTANT**, and `SM_Castle.uasset` is dirty in the tree. Plus a **~35 uu inconsistency** between two shipped comments about the same footprint (`3656.85` as `max(X,Y)` vs a `7313.7 x 7384.5` footprint implying `3692.25`). Both pre-existing; **immaterial to every conclusion** (re-run at 3692.25: 83.8% vs 83.6%). *Routed to the manager with WARN-6; `TASK-987` should re-read the `:2636` log line rather than the transcribed number.*
- **[WARN-8]** the `83.6% ≈ 84%` agreement is **NOT corroboration** — one operand, one formula, two radii 1.24% apart. ⛔ **Do not quote it to Jonathan as a cross-check** (§L2(B)). The arithmetic is correct; only the confidence claim is invalid.
- **[NIT-5]** two coordinates in the repair record still rot, **both harmless, both symbol-findable**: `AcquireEnemyNearPoint`'s **declaration** is at `SummonedUnit.h:1962`, §11(C) says **1957**; the castle-footprint invariant is at `SummonedUnit.cpp:5589–5592`, §11(E)(4) and the board say **`:5572–5575`**. ⇒ ⭐ **`SC-§38` again, inside the section that invoked it.** Pre-existing coordinate rot of the same family, ⛔ not this row's: `SiegeGhostPawn.h:49` cites `:2262` and `SiegeInvisibilityStatics.h:234` cites `:1654`/`:2199` for functions now at `:1811`/`:2375` — **two different stale numbers, so they rotted independently long before `TASK-979`.**
- **[NIT-6]** `Tests/SiegeFogClampTest.cpp:1055–1056` — dangling construction: *"Since TASK-979 item (6b) AcquireEnemyNearPoint applies its own … bound AFTER the gather, so the GATHER is unbounded…"* (`Since … so …`). **The content is correct**; only the conjunction is doubled. Comment text, zero risk.

---

## §L2 — Notes for build-master (`TASK-987`) — ✅ **THIS ROW IS `qa-passed`. IT MAY ENTER YOUR COMPILE.**

1. ✅ **BLOCKER-1 is CLOSED at all sites; ⛔ nothing in the diff needs unwinding.** All loop-1 "do not touch" items stand: the channel, the resolver, the leash inversion, the commanded bound, the `FCardRow` field, the re-derived fog rows and all four new tests.
2. ⚠️ **The two build-only constructs from loop 1 are UNCHANGED and still yours to retire** (the UPROPERTY NSDMI `float AggroRadius = UnitEngagementRadiusUU;` at **`SummonedUnit.h:1258`**, and the `constexpr` ODR-uses in `SiegeUnitNoticeRangeTest.cpp`). ⛔ If UHT or the linker objects, the fallback is the constructor / an out-of-line definition — ⛔ **NEVER a hand-typed `2000.f`.**
3. ⚠️ **REGRESSION WATCH (revised, from §L2(B)):** *"a DEFEND defender does not notice a besieger on a castle face it is not standing on."* ⛔ **This is an ACCEPTED, RULED consequence — ⛔ not a bug and ⛔ never "flaky".** `TASK-574` is **not** regressed. Do not let anyone "fix" it without a boarded movement row.
4. ⚠️ **REGRESSION WATCHES 5 and 6 from loop 1 stand unchanged** (the newly-live per-candidate fog cut under fog-plus-many-units; melee passivity under fog at −69.5% chase radius — **expected**, do not "fix" it back).
5. ⭐ **From §L2(C): when you read back any castle-derived number, take it from the DEFEND derivation log line (`SummonedUnit.cpp:2636–2638`), ⛔ never from a transcribed `3656.85`** — it is a live `GetActorBounds` measurement and `SM_Castle.uasset` is dirty.
6. ⛔ **`TL-§5c` — I state no suite total as an executed number.** My **declared** census at my instant is **445 / 34 files**, unchanged across both loops. Derive yours at your own instant; `TASK-981` may still move it.
7. ⚠️ **`TASK-985` still owes the `TASK-980` half.** This file is the `979` half, now **PASS**, and is incorporated by reference per `TASKBOARD.md:18271`.

## §L2 — ⛔ RECORDED ABSENCE (`SC-§40`) — THE BOARD FLIP I COULD NOT MAKE

⛔ **I did NOT flip `TASK-979`'s `status:` line to `qa-passed`.** I have no line-editing tool in this session, and `TASKBOARD.md` is ~18,600 lines — rewriting it wholesale to change one line is an unacceptable risk of destroying the board. ⇒ **Recording the absence rather than faking the action.**

🧑 **ACTION REQUIRED (orchestrator / manager):** set `TASK-979` → **`status: qa-passed 2026-09-04 (QA LOOP 2 — PASS, 0 BLOCKER, 3 WARN, 2 NIT; qa/TASK-979.md §L2)`**, and carry **WARN-6**, **WARN-7** and the still-unlanded **WARN-2** retention question onto `TASK-980`'s row. **This file is the authoritative verdict.**
