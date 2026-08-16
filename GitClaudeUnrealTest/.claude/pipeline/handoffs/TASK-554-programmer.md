# TASK-554 handoff — [GDD-REFS] backfill the `// GDD §x.x` placeholders (gameplay-programmer, 2026-08-15)

- **Gate:** `.claude/pipeline/qa/TASK-565.md` — ⛔ **this task names TASK-565 as its gate.** (RULING 9: adopted into the WAR-ROOM batch; build-master refuses to commit without this naming.)
- **Commit:** TASK-570. ⛔ **This task opened NO compile of its own** (spec + `WR` batch law).
- **Status:** `ready-for-qa`.
- **Discipline honoured:** ⛔ no compile · no Git · no editor · no MCP · no PIE · no `.csv` · no `Content/` · no `Build.cs` · no `Tests/`. ⛔ **No token figure is quoted, derived or reasoned from anywhere in this handoff** (`AS-§12g`, batch-wide ban).
- **Files touched — exactly the three in `names:`:**
  - `Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.h`
  - `Source/GitClaudeUnrealTest/Siegebound/AncientGround.h`
  - `Source/GitClaudeUnrealTest/Siegebound/SiegeGameMode.cpp`
- ⛔ **NOT touched:** `AncientGround.cpp` · `SorcererUnit.{h,cpp}` · `SiegePlayerState.{h,cpp}` · `SiegeBotController.{h,cpp}` · `Docs/GDD.md` · `Docs/Data/cards.csv` · any `Content/` · any `Build.cs` · any `Tests/`.

---

## 0. ⛔ EVERY REFERENCE BELOW WAS TRACED AT `Docs/GDD.md`, NOT INFERRED FROM THE PLACEHOLDER'S NEIGHBOURHOOD

`SC-§20` binds: each section number in the spec arrived as a **HYPOTHESIS**. I read the GDD before typing. All three hypotheses **CONFIRMED at the artifact**; none was adopted on the spec's word.

| spec hypothesis | traced to | the sentence that confirms it | verdict |
|---|---|---|---|
| `PermanentDamageBonusPerStack` ⇒ `§3.12` | `Docs/GDD.md:205` (`### 3.12`, heading at `:199`) | *"One stack is **+5% of that unit's base damage**."* — the shipped default is `0.05f` | ✅ CONFIRMED |
| `MaxPermanentDamageStacks` ⇒ `§3.12` | `Docs/GDD.md:205` | *"The cap is **80 stacks = +400%**."* — the shipped default is `80` | ✅ CONFIRMED |
| `BoostTickInterval` ⇒ `§3.12` + `§3.0` | `Docs/GDD.md:209` and `:27` | `:209` — *"These magnitudes are mechanic rules, not card stats (§3.0): the +5% per stack, the 80-stack cap and **the 1 s tick** are engine tunables"* · `:27` (§3.0) — *"**Mechanic rules are not card stats** … The Ancient-Ground boost magnitudes (§3.12) … are all mechanic rules."* | ✅ CONFIRMED — **§3.12 names this tick explicitly**, and §3.0 does state the law in its own right |
| `SiegeGameMode.cpp` sandbox comment ⇒ `1 gold per 1 s, TASK-278` | code, not the GDD (see §3 below) | `SiegePlayerState.h:320` `BaseIncomeTickPeriod = 1` · `:314` *"defaults: 1 gold per 1 s — 2026-07-24 balance directive TASK-278, reverts the TASK-089 … 1-per-2s income change"* | ✅ CONFIRMED against the shipped defaults |

📌 **Style precedent checked before typing, so the token matches its neighbours:** `SummonedUnit.h` already carries `// GDD §3.0` (×4, `ChargeMoveSeconds` · `ChargeMultiplier` · `SlayerMultiplier` · `SlayerHPThreshold`) and `// GDD §4` (×2, the two BattleCry bonuses). `// GDD §3.12` is the same shape — ⛔ no new convention invented.

📌 **NO PHANTOM IN THIS TASK'S CITATIONS.** All four cited sites **exist**, located **by symbol** (`SC-§18c`), and the spec's line hints happened to still be accurate: `SummonedUnit.h:795` / `:805` exact; `SiegeGameMode.cpp:1161` exact; `AncientGround.h`'s `BoostTickInterval` doc block was found at `:180-189` (the spec gave no line for it). ⛔ **Nothing was hunted for and nothing was manufactured.**

---

## 1. `SummonedUnit.h` — the two placeholders (spec item 1)

⛔ **ONLY the section token moved. The surrounding ratified doc comments are byte-identical.**

**Site A — `PermanentDamageBonusPerStack`**

```diff
-	float PermanentDamageBonusPerStack = 0.05f; // GDD §x.x
+	float PermanentDamageBonusPerStack = 0.05f; // GDD §3.12
```

**Site B — `MaxPermanentDamageStacks`**

```diff
-	int32 MaxPermanentDamageStacks = 80; // GDD §x.x
+	int32 MaxPermanentDamageStacks = 80; // GDD §3.12
```

⭐ **QA should know this and it is verifiable without a compile: these two produce ZERO UHT metadata change.** A trailing `//` comment on the declaration line is **not** captured by UHT — only the preceding `/** */` block is. Proof from the existing generated file (unmodified, pre-existing artifact): `grep -c "x.x" Intermediate/…/UHT/SummonedUnit.gen.cpp` → **0**, while the same file's `NewProp_PermanentDamageBonusPerStack_MetaData` carries only the `/** */` block above the `UPROPERTY`. ⇒ **These two edits change no metadata, no tooltip, no emitted byte.**

---

## 2. `AncientGround.h` — `BoostTickInterval`'s doc paragraph (spec item 2)

The false sentence was *"the GDD has no ancient-grounds section yet, so this comment is the rule's home of record alongside CONVENTIONS §2."*

**Before:**

```
	/**
	 *  Seconds between boost evaluations. MECHANIC RULE — a UPROPERTY default,
	 *  NEVER a cards.csv column (CONVENTIONS "mechanic rules aren't card stats";
	 *  the GDD has no ancient-grounds section yet, so this comment is the rule's
	 *  home of record alongside CONVENTIONS §2). 1.0 s is the design unit: each
	 *  friendly sorcerer on the ground grants exactly ONE stack per second.
	 *  Changing it re-scales the whole boost rate. FLAGGED tunable.
	 */
```

**After:**

```
	/**
	 *  Seconds between boost evaluations. MECHANIC RULE — a UPROPERTY default,
	 *  NEVER a cards.csv column (CONVENTIONS "mechanic rules aren't card stats";
	 *  GDD §3.0 now states that law in its own right, and the DESIGN home of
	 *  record is GDD §3.12 "Ancient Grounds & the Sorcerer", which names this
	 *  1 s tick as one of the three engine tunables — the ENGINEERING law still
	 *  lives at CONVENTIONS §2). 1.0 s is the design unit: each friendly
	 *  sorcerer on the ground grants exactly ONE stack per second. Changing it
	 *  re-scales the whole boost rate. FLAGGED tunable.
	 */
```

**Every clause the spec ordered kept, kept:**
- ✅ **MECHANIC RULE / NEVER a cards.csv column** — verbatim, untouched.
- ✅ **The CONVENTIONS §2 cross-reference SURVIVES** and is now *sharpened* rather than deleted: it is named as the **ENGINEERING** law's home, which is precisely the spec's instruction (*"the engineering law still lives there; the GDD only gained the DESIGN home"*).
- ✅ **§3.0 is credited as stating the mechanic-rules-aren't-card-stats law in its own right** (spec's wording, traced to `GDD.md:27`).
- ✅ **The 1.0 s design-unit rationale and the FLAGGED-tunable marker** — unchanged in substance; the last two sentences were re-wrapped only (identical words, same line count).

⚠️ **DECLARED FOR THE GATE, SO IT IS NOT MIS-READ AS A VIOLATION OF TASK-565 CRITERION (9).** Unlike §1 above, **this block IS a `/** */` doc comment directly above a `UPROPERTY`, so UHT extracts it into `AncientGround.gen.cpp` as `Comment` + `ToolTip` metadata.** ⇒ **On the batch's next compile, `Intermediate/…/UHT/AncientGround.gen.cpp` WILL regenerate with the new tooltip string.** ⚖️ **That is EDITOR-ONLY metadata — a Details-panel tooltip.** ⛔ **No gameplay byte, no behaviour, no replicated property, no prompt character.** The `UPROPERTY(...)` line, the `meta = (ClampMin = "0.05")` and the `= 1.0f` default are all **byte-identical**. `Intermediate/` is build output, not source, and is not part of this diff.

---

## 3. `SiegeGameMode.cpp` — the stale economy claim (spec item 3) — ⚠️ **TWO SITES, NOT ONE**

**Site A — the cited one, `GrantSandboxStartingGold` (spec's `~:1161`, found there):**

```diff
 	// both apply — NEVER a raw Gold field write. The BASE gold rate is untouched
-	// (no AddIncome), so the normal base economy stands (1 gold per 2 s, TASK-089;
-	// spec: keep the normal rate). NOTE: MaxGold (999) clamps SandboxStartingGold
+	// (no AddIncome), so the normal base economy stands (1 gold per 1 s, TASK-278
+	// (2026-07-24) reverting TASK-089's 1-per-2-s rate — ASiegePlayerState's
+	// GoldPerTick=1 / BaseIncomeTickPeriod=1 are the record of truth, GDD §3.2;
+	// spec: keep the normal rate). NOTE: MaxGold (999) clamps SandboxStartingGold
 	// (9999) to 999 — still a full generous pile for the 22-card roster (flagged
```

The spec's hypothesis was `1 gold per 1 s, TASK-278 (reverting TASK-089)`. ✅ Adopted, and **pointed at the code that owns the fact** rather than leaving a bare assertion — the exact failure mode the spec named (*"a sandbox comment asserting the wrong economy rate is exactly what a future balance task would read and trust"*) is best closed by naming the two fields a reader can check.

### ⭐ Site B — **UNCITED, SAME FILE, SAME DEFECT. `SC-§22` is why it was found.**

The spec named one site. `SC-§22` says a citation is a **lower bound**, so I swept the **claim**, not the comment — and `Play Again`'s clock-reset rationale in **this same file** carried the identical false rate, plus a second falsehood the cited site did not have.

```diff
 	//     rate by reading this latch live — clearing it first lands the rate
-	//     on the pre-overtime base (display +1/s round-up, true 1 gold per
-	//     2 s, TASK-089).
+	//     on the pre-overtime base (display +1/s, and with the 2026-07-24
+	//     defaults it is EXACT, not a round-up — true accrual is 1 gold per
+	//     1 s, TASK-278 reverting TASK-089's 1-per-2-s rate).
```

⚠️ **Why this one is arguably the worse half, and why it is a genuine second instance rather than the same line twice:**
1. **It asserts the same false rate** (*"true 1 gold per 2 s"*).
2. ⛔ **It ALSO asserts a false MECHANISM — *"display +1/s round-up"*.** With the shipped `BaseIncomeTickPeriod = 1` the display average is **EXACT**; the round-up is a no-op. `SiegePlayerState.h:174-181` says so in terms (*"with the 2026-07-24 defaults (BaseIncomeTickPeriod=1, TASK-278) the average is EXACT — … so the round-up is a no-op"*). ⇒ This is precisely `SC-§22`'s hardest shape: **a conclusion resting on a dead mechanism**, which reads as verified and is not.
3. 📌 **The provenance confirms it is one defect authored twice:** `handoffs/TASK-089.md` lines 44-45 record BOTH sites being written in the same pass (*"~L579 … true 1 gold per 2 s"* and *"~L867 … the normal base economy stands (1 gold per 2 s, TASK-089)"*). TASK-278 reverted the behaviour and updated `SiegePlayerState`'s own headers — **neither `SiegeGameMode.cpp` site was in its declared file list**, so both survived. ⇒ **Fixing only the cited line would have left this file still asserting the wrong economy — this task's own stated failure mode, achieved by a task reporting success.**

---

## 4. ⛔ THE `SC-§22` SWEEP — COMMANDS, RAW HIT COUNTS, ONE LINE PER HIT

Scope swept: `Source/GitClaudeUnrealTest` **and** `Plugins/SiegeLlama/Source` (the game module was ordered; the plugin was added for free). **String literals are included by construction — `grep` is content-based, not comment-aware — and were additionally swept explicitly in leg F**, because `SC-§22` names runtime log strings as the **priority** surface.

| # | command (run from repo root, `$S`=`Source/GitClaudeUnrealTest`, `$P`=`Plugins/SiegeLlama/Source`) | raw hits | result |
|---|---|---|---|
| A | `grep -rn "GDD §x.x" $S $P` | **0** | ✅ **The placeholder shape is now EXTINCT in source.** (Was 2 pre-edit — both fixed in §1.) |
| A2 | `grep -rniE "§x\.x\|§\?\.\?\|section x\.x\|GDD §TBD\|GDD §\?" $S $P` | **0** | ✅ **No unresolved GDD reference of ANY placeholder shape survives anywhere in source.** |
| B1 | `grep -rn "per 2 s\|per 2s" $S $P` | **3** | 2 ruled CORRECT, 1 stale + out-of-ownership — see below |
| B2 | `grep -rniE "1 gold per 2\|\+1 per 2\|gold per 2\|1-per-2\|1/2 ?s\|0\.5 ?gold\|0\.5/s" $S $P` | **8** (post-edit) | 5 correct, 3 stale + out-of-ownership — see below |
| C | `grep -rni "no ancient-grounds section\|ancient-grounds section\|no ancient ground section" $S $P` | **0** | ✅ **The false "GDD has no section" sentence is EXTINCT in source.** (Was 1 pre-edit — fixed in §2.) |
| D | `grep -rn "TASK-089" $S $P` | **23** | every one re-read; only the 2 in §3 asserted a stale RATE — the other 21 cite TASK-089 for facts it still owns (StartingGold **10**, the decomposed-accrual design, the display-average ruling, the tick-parity counter). ⛔ **TASK-089 is not wrong, only its RATE was reverted — the citations stay.** |
| E | `grep -rniE "GDD has no\|not (yet )?in the GDD\|no GDD section\|home of record\|GDD (does not\|doesn't) " $S $P` | **0** | ✅ **No other comment anywhere in source claims the GDD lacks a home for a mechanic.** |
| F | `grep -rn "TEXT(" $S $P \| grep -iE "gold per\|gold/s\|per 2 s\|\+1/s\|\+2/s\|base income"` | **4** | ✅ **ALL FOUR CORRECT — zero stale claims in any runtime string literal.** See ruling below. |

### Leg F ruled in full (`SC-§22`'s priority surface — a false log line is believed over the code)
- `DeckBuilderWidget.cpp:46` — Miner *"+1 gold per second"* ✅ **CORRECT.** Miner income was never touched by TASK-089 or TASK-278 (both explicitly exempt it); matches GDD §3.3.
- `DeckBuilderWidget.cpp:49` — Deep Mine *"+2 gold per second"* ✅ **CORRECT**, same reasoning; matches GDD §8.
- `SiegeBotController.cpp:528` — *"all-depleted endgame = base income + Deep Mine"* ✅ **CORRECT** — states no rate at all.
- `SiegeGameState.cpp:129` — *"Overtime started … GDD §3.2 — base income doubles."* ✅ **CORRECT** — `OvertimeIncomeMultiplier = 2` is intact and TASK-278 preserved it.

### Legs B1/B2 ruled in full

**RULED CORRECT — ⛔ deliberately NOT edited, because editing them would be the defect:**
- `SiegeBotController.cpp:521` and `SiegeBotController.h:568` — *"never per 2 s tick"*. ✅ **NOT an economy claim.** This is the **bot decision cadence**, and it is verified live: `SiegeBotController.h:220` `float DecisionIntervalSeconds = 2.f;`, consumed at `SiegeBotController.cpp:360`. **The number is correct and current.** ⚠️ A keyword sweep that "fixed" these would have introduced a falsehood.
- `SiegePlayerState.h:50`, `:179`, `:314`, `:318`, `:334` — all five mention `1-per-2s` / `0.5/s` **as explicitly labelled PAST-TENSE history** (*"reverts the … 1-per-2s income change"*, *"Pre-TASK-278 the period-2 base averaged 0.5/s"*, *"was 2 ⇒ every 2 s"*). ✅ **These are the record of the revert and are correct as written.** ⛔ Not a finding.
- `SiegeGameMode.cpp:1162` — a **B2 hit on my own new text** (`"1-per-2-s rate"` inside the corrected sentence). ✅ Expected; it is the historical clause of the fix.
- `SiegePlayerState.cpp:208-209` — *"the HUD's rounded-up DISPLAY average"*. ✅ **Ruled CORRECT, narrowly:** `FMath::DivideAndRoundUp` **is** still called (`:274`), so the sentence is literally true even though the round-up is now a no-op. ⛔ Not a finding.

### ⚠️ FOUND, STALE, AND ⛔ **OUT OF THIS TASK'S OWNERSHIP — NAMED TO AN OWNER** (`SC-§22` / `SC-§34` clause (iii) shape)

⛔ **I did NOT edit these.** `SiegePlayerState.cpp` is not in this task's `names:` block, and the dispatch was explicit: *"Stay inside your three files."* **Three stale assertions, all in `SiegePlayerState.cpp`, all contradicting `BaseIncomeTickPeriod = 1` (`SiegePlayerState.h:320`):**

| site | the stale text | why it is false |
|---|---|---|
| `SiegePlayerState.cpp:210` | *"(default **2** ⇒ 1 gold per 2 s, §3.2 amended)"* | the shipped default is **1** ⇒ 1 gold per **1** s |
| `SiegePlayerState.cpp:262` | *"base GoldPerTick (1) per BaseIncomeTickPeriod (**2**) ticks"* | same — the period is **1** |
| `SiegePlayerState.cpp:270-271` | *"defaults show +1/s pre-overtime (**true 0.5/s** — ruled acceptable, "+0/s" over a visibly rising counter reads as broken)"* | with period **1** the average is **EXACT**; the whole "acceptable inaccuracy" justification describes a state that no longer exists |

⭐ **The sharpest form of the finding, and the reason it deserves a task rather than a shrug: `SiegePlayerState.cpp` now CONTRADICTS ITS OWN HEADER.** `SiegePlayerState.h:174-181` states the corrected fact explicitly (*"with the 2026-07-24 defaults (BaseIncomeTickPeriod=1, TASK-278) the average is EXACT"*) while the `.cpp` three screens away still explains the old behaviour as current. ⚠️ **A reader who opens the implementation — which is where you go to learn what the code DOES — gets the false version.**

📌 **PROVENANCE: these were KNOWN AND CONSCIOUSLY DEFERRED, not missed.** `handoffs/TASK-278.md` §114 surfaced two of them by their then-line-numbers (`:134`, `:178` — now `:210`, `:262`) as *"out-of-scope stale comments NOT edited … outside the declared file list"* and asked for *"a possible future comment-hygiene pass."* ⇒ ⚖️ **That pass has now been requested twice and has still not been boarded. Recommend the manager board it** — it is comment-only, needs no editor, and would ride the next game-module commit exactly as this task did. **`SiegePlayerState.cpp:270-271` is the one worth doing first**, because it is a dead-mechanism justification rather than a bare wrong number.

📌 **ALSO OBSERVED, RULED ⛔ NOT A DEFECT — `Docs/GDD.md` (out of scope, and correct by its own style).** `GDD.md:53` (§3.2) and `:445` (§8) still lead with *"+1 gold per 2 s"* — but **both carry an explicit trailing as-built correction bracket** (`:53` — *"⚠️ THE LINE ABOVE IS NO LONGER THE SHIPPED RATE … base accrual is +1 gold per 1 s"*). ⇒ **That is the v4 pass's deliberate original-line-plus-annotation convention, not a stale claim.** ⛔ No action, and ⛔ `Docs/GDD.md` was not touched.

📌 **SPEC ITEM (5) HONOURED — `Docs/Data/cards.csv` NOT TOUCHED.** Verified **read-only** that the record is still accurate for whoever picks it up: row 26 `Lightning` — the free-text `Notes` cell says *"in 400"* while the `AoERadius` column reads **700**. ⛔ **Left exactly as found**, per the spec's *"DO NOT FIX IT HERE."*

---

## 5. What QA should scrutinise (TASK-565)

1. ⛔ **The zero-behaviour claim, and the one honest exception.** `SummonedUnit.h` (§1) and `SiegeGameMode.cpp` (§3) are pure `//` comment text — **no metadata, no emitted byte.** `AncientGround.h` (§2) is a `UPROPERTY` doc block, so **`AncientGround.gen.cpp` regenerates with a new editor tooltip** — declared up front in §2. ⚖️ **Please rule on it explicitly rather than silently**, so criterion (9) (*"no emitted byte, no behaviour and no prompt character moved"*) has a recorded answer. **No `.uasset`, no prompt payload, no `TEXT()` literal was touched by any of the five edits.**
2. ⛔ **Re-trace the section numbers at the GDD, don't take my word.** `GDD.md:199` (heading `### 3.12`), `:205` (the +5% and the 80 cap), `:209` (the 1 s tick as a mechanic rule), `:27` (§3.0's law). **A number I traced is still a number you should re-trace** — that is the gate's half of `SC-§20`.
3. ⭐ **The uncited second site (§3B) is the finding most worth an independent check.** Confirm `SiegeGameMode.cpp`'s Play-Again clock-reset comment was in fact stale and that the replacement matches the shipped `BaseIncomeTickPeriod = 1` / `GoldPerTick = 1`. **If you judge it outside this task's remit, say so and I will revert it** — but it is inside a declared file, it is the identical defect, and `SC-§22`'s implementer's half is explicit that a one-line fix is not done until the shape has been swept.
4. ⚠️ **Re-run the sweep table in §4 and paste your own counts** (`SC-§22` + the second-leg pattern). **A/A2/C/E should all read 0.** If B1/B2/D move, that is a real divergence worth reporting.
5. ⛔ **Rule on the three out-of-ownership `SiegePlayerState.cpp` sites** — I deliberately left them and named them. **Confirm leaving them was correct** (I judged "stay inside your three files" to outrank the sweep's reach), or fail me and say which file list should have won.
6. ✅ **The legs that found nothing are RESULTS, not gaps** (`SC-§22`'s closing rule): the placeholder shape is extinct, the false "GDD has no section" sentence is extinct, and **no runtime string literal anywhere in the module or the plugin asserts a stale economy rate.**

## 6. Scope ledger — the five edits, in full, and nothing else

| # | file | symbol located by | change |
|---|---|---|---|
| 1 | `SummonedUnit.h` | `PermanentDamageBonusPerStack` | `§x.x` → `§3.12` (token only) |
| 2 | `SummonedUnit.h` | `MaxPermanentDamageStacks` | `§x.x` → `§3.12` (token only) |
| 3 | `AncientGround.h` | `BoostTickInterval` | doc paragraph re-pointed at GDD §3.12 + §3.0; CONVENTIONS §2 kept |
| 4 | `SiegeGameMode.cpp` | `GrantSandboxStartingGold` | `1 gold per 2 s, TASK-089` → `1 gold per 1 s, TASK-278` |
| 5 | `SiegeGameMode.cpp` | the `ResetClock()` / Play-Again step 3b block | ⭐ **uncited** — same stale rate + a dead round-up mechanism, corrected |

⛔ **No code statement, no expression, no default value, no `UPROPERTY` specifier, no `meta =` clause and no include was added, removed or modified anywhere in this task.**
