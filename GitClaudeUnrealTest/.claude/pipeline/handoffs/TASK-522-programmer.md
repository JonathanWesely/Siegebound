# TASK-522 — [AX-7] EXECUTOR — apply `ExcludeKinds` to the `who:"all"` selection

**Agent:** gameplay-programmer · **Date:** 2026-08-04 · **Status:** ready-for-qa · **QA gate:** TASK-525
**Law applied:** CONVENTIONS `AS-§20.1` (the pinned registry + EMPTY-AFTER-EXCLUSION) · `AS-§2` (the same public APIs the keys call) · `AS-§3` (templates only) · `AS-§4` (the unit-registry rejection) · `AS-§8` (the executor seam) · the RELAYED-DIAGNOSIS LAW

> **M8 DECLARATION, verbatim:** *"adds no replicated property, no new replicated class, no new relevancy tier."*

Reason: this task adds **no member, no `UPROPERTY`, no delegate and no class**. It reads an existing
`TArray<FName>` off a struct that was already pinned `uint8`/`int32`/`FName`-only, inside a `const`
selector and a `const` describer. The wire property `AS-§3` asserts is untouched.

---

## 1. ⭐⭐ THE HIGHEST-RISK LINE IN THE BATCH — STATED FIRST, GREPPABLE IN ONE COMMAND

> ### ✅ **`SiegeAssistantComponent.cpp:987` NOW PASSES `Command.ExcludeKinds`.**

```cpp
-		if (!SiegeAssistantValidateSelection(Command.Kinds, Command.Counts, SelectionError))
+		if (!SiegeAssistantValidateSelection(Command.Kinds, Command.Counts, SelectionError, Command.ExcludeKinds))
```

**QA's one-command check:**
`grep -n "SiegeAssistantValidateSelection" Source/GitClaudeUnrealTest/Siegebound/SiegeAssistantComponent.cpp`
⇒ **exactly one hit, and it must carry four arguments.**

📌 **The line number moved: TASK-518 and the board both say `:930`. It is `:987` today** — TASK-520
landed in the same file first and shifted it. Same call, same function (`HandleModelCompletion`),
same and only call site in this file. Nothing was missed; the citation is just older than the file.

⛔ **Why this was the batch's most dangerous line:** the 4th parameter is **trailing and defaulted**
(TASK-518 §5 — forced by file ownership, not chosen), so the three-argument call **compiled cleanly
and validated nothing about exclusion** — the cap, the no-repeat rule and the never-both rule all
reporting SAFE while checking nothing. **No compiler diagnostic would ever have fired.** A block
comment at the call site now says exactly that, so the next reader who "tidies" the argument away
has to read why it is there first.

⚠️ **This also closes the M8 P2 wire hole**, which is the reason the invariants live in the
validator at all: a peer-supplied `FSiegeAssistantCommand` now meets the exclusion invariants at the
same gate as everything else, before it reaches the executor.

---

## 2. The diff — five edits in three files

| file | edit |
|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/SiegeAssistantComponent.cpp` | **(a)** `:987` — pass `Command.ExcludeKinds` (§1). **(b)** `SelectUnitsForOrder` — the exclusion filter in the `Kinds.Num() == 0` branch, the empty-after-exclusion refusal, two success logs, and a never-silently-ignore backstop at the top. **(c)** `DescribeCommandForPlayer` — the confirm sentence NAMES the exception. **(d)** `ReportFirstCapture` — the stale `B+C=738` reference figure corrected from TASK-517's measurements. |
| `Source/GitClaudeUnrealTest/Siegebound/SiegeAssistantComponent.h` | doc only — the selector's contract, `DescribeCommandForPlayer`'s, and two rows of the class comment's ON/OFF gate trace. |
| `Source/GitClaudeUnrealTest/Siegebound/SiegeCheatManager.cpp` | **ONE LINE (`:670`)**, spec-authorized — the now-false 13-kind roster-divergence claim. ⛔ Nothing else in that file was opened for editing. |

⛔ **NOT touched:** `SiegeAssistantCommand.*` / `SiegeAssistantGrammar.*` (TASK-518, frozen) ·
`SiegeAssistantSnapshot.*` (TASK-517/521) · `SiegeAssistantConsoleWidget.*` (TASK-519) · `Tests/**`
(TASK-523) · `Docs/Data/*.csv` (TASK-524) · `Plugins/SiegeLlama/**` (D4 — and specifically
**`SiegeLlamaSpike.cpp:5785` was NOT touched**, per TASK-517's finding that it stays green because
it measures the spike's own lane) · `SiegePlayerController.*` · `HeroCharacter.*` ·
`SummonedUnit.*` / `MinerUnit.*` (read-only). **No compile, no Git, no editor/MCP/PIE, no `Content/`
asset — this task references no asset at all.**

### (b) The functional core, `SelectUnitsForOrder` (comments elided; they ship in full)

```cpp
 	if (Command.Kinds.Num() == 0)
 	{
+		const int32 EligibleCount = Eligible.Num();
+		int32 ExcludedCount = 0;
+
 		for (const FSelectorCandidate& Candidate : Eligible)
 		{
+			if (Command.ExcludeKinds.Contains(Candidate.Unit->GetCardID()))
+			{
+				++ExcludedCount;
+				continue;
+			}
+
 			OutMembers.Add(Candidate.Unit);
 		}

 		if (OutMembers.Num() == 0)
 		{
+			if (ExcludedCount > 0)
+			{
+				UE_LOG(LogSiegeAssistant, Log,
+					TEXT("Selector refused - the exclusion emptied the selection: %d eligible unit(s) ... %d removed by the %d-kind exclusion, %d left. ..."),
+					EligibleCount, ExcludedCount, Command.ExcludeKinds.Num(), OutMembers.Num());
+			}
+			else
+			{
 				UE_LOG(LogSiegeAssistant, Log, TEXT("Selector found no eligible unit at all for an army-wide selection. NOTHING was executed."));
+			}
 			return false;
 		}
+
+		if (ExcludedCount > 0)      { /* the SUCCESS arithmetic - see §5 */ }
+		else if (Command.ExcludeKinds.Num() > 0) { /* an exclusion that removed NOBODY - see §6 */ }

 		return true;
 	}
```

**Three properties, each one a spec clause:**

1. **It is a predicate inside a loop that already existed.** ⛔ Still **ONE pass over the world**, no
   registry, no actor cache, no dirty flag, no subscription list (`AS-§4` rejects all four on sight).
   The added cost on a command with no exclusion is `TArray::Contains` against an **empty** array —
   which returns immediately — per already-eligible candidate.
2. **The match is on the canonical kind symbol, with NO second transform.** `Command.ExcludeKinds`
   holds symbols the snapshot's `CanonicalKind` produced (the card row name lower-cased);
   `GetCardID()` returns the row name; `TArray<FName>::Contains` uses the **same case-insensitive
   `FName::operator==`** as the sibling seam 40 lines below, whose shipped comment says exactly why
   there is no lower-casing there. ⛔ **I did not re-lower-case and did not re-derive the mapping.**
3. **`Eligible` is still the full eligible population**, so `EligibleCount` is a true "before" figure
   and the arithmetic in the log is a measurement rather than a re-derivation.

### (c) The confirm sentence

```cpp
+	if (Command.ExcludeKinds.Num() > 0)
+	{
+		Selection += Selection.IsEmpty() ? "all except " : ", except ";   // (written as if/else in source)
+		// ... the excluded symbols, comma-separated, raw, exactly the "all %s" idiom above
+	}
```

⇒ **"Send all except miner (mid) - accept?"** ⛔ **No new frame, no new `NSLOCTEXT` row, no new
reason code.** The clause is built into `{Selection}`, so the three existing frames render it
unchanged and `§30`'s *"one string, not three"* stays true. Read aloud on every path its values can
supply (the mechanical test `handoffs/TASK-471-programmer.md` pins): *"Send all except miner (mid)"*
· *"Guard all except miner, cleric (mine_near)"* · *"Follow all except miner"*.

---

## 3. ⭐ THE CONFIRM PREVIEW — WHAT I FOUND, AND WHY THE ANSWER IS THE SENTENCE AND NOT THE CIRCLES

The spec says the preview *"must reflect the post-exclusion set"*. **Read at the artifact, it cannot,
and that is a property of the shipped preview rather than something this task declined to do:**

> **`SpawnConfirmPreview` draws exactly TWO PLACE DECALS** — `ConfirmPositionDecal` and
> `ConfirmAttackDecal`, both `SpawnGroupCircleDecal` at the **resolved destination**. **Nothing in
> the preview is per-unit.** No ghost is spawned on a selected unit, no count is drawn, and the
> geometry is byte-identical whether the order moves 30 units or 3.

⇒ **There is no ghost that shows a unit the order will not move**, so the defect class the spec names
cannot occur through the circles. **What CAN occur — and what I fixed — is the SUMMARY LINE lying by
omission:** before this change, *"send everyone except the miners to mid"* rendered as
**"Send (mid) - accept?"**, a confirm prompt describing a **different order from the one that would
execute**. That is the same failure wearing different clothes, and it is now closed (§2c). The same
sentence is reused by the `Executed` line, the deferred *"waiting for…"* line and
`GetPendingConfirmSummary()`, so all four moved together.

⚠️ **What I deliberately did NOT do, flagged rather than silently skipped:** I did **not** add a
*"(N units)"* count to the prompt. It would require running the selector at **prompt** time, on a
board that can change before the player presses accept — a **second, staler survey** whose number
would then disagree with the one execution actually selects. That is a product/UX call and a bigger
change than this task owns. ⇒ **If TASK-525 or the manager wants the count, name the owner.**

---

## 4. HOW AN EMPTY POST-EXCLUSION SELECTION IS REFUSED — the whole path, end to end

1. `SelectUnitsForOrder` subtracts, finds `OutMembers.Num() == 0`, logs the arithmetic
   (`%d eligible, %d removed by the %d-kind exclusion, %d left`) at **`Log`**, and **returns false**.
2. `ExecuteZoneOrder` / `ExecuteFollowOrder` return **false** immediately (their existing
   `if (!SelectUnitsForOrder(...)) return false;`).
3. `ExecutePendingCommand` returns **false**.
4. `ExecuteAndReport` runs its shipped line:
   `PushMessage(bExecuted ? Executed : AskUnsupported, ExecutedArgs)` ⇒ **the player is told, through
   the EXISTING unsupported-ask outcome.**

✅ **`AS-§20.1` satisfied structurally:** ⛔ **no new `ask` symbol** (the ask alternatives are part of
the grammar the model samples from) · ⛔ **no new reason code authored at a call site** (`AS-§3`) ·
⛔ **never a silent no-op** — *"nothing happened and nothing was said"* is impossible on this path,
because step 4 is unconditional and is the same statement that reports every other refusal.
⚠️ **This is the shipped shortfall path's pattern, reused rather than re-invented** — same return
shape, same log-carries-the-arithmetic rule, same stated reason for not authoring a narrower
sentence. It is written 20 lines below mine in the same function.

**Both `AwaitConfirm` states behave identically**: with the confirm toggle ON the player accepts and
then gets the refusal; with it OFF the refusal arrives directly. The toggle removes a human review
step and never a machine check — unchanged by this task.

---

## 5. ⚠️ THE SUCCESS LOG IS LOAD-BEARING, NOT DECORATION

TASK-523 §7 records that **the executor's exclusion filter is NOT unit-testable** in this batch
(`EditorContext` simple tests have no world and no actors) and that it is *"covered by TASK-522's
logs and by Jonathan at TASK-527."* ⇒ I emit the arithmetic **on the path that WORKED** too:

```
Selector applied an EXCLUSION: 24 eligible, 3 removed by the 1-kind exclusion, 21 selected.
The order moves everyone EXCEPT the named kind(s).
```

**A log that only speaks on failure cannot prove a success**, and without this line the only evidence
that an exception was *subtracted* rather than *parsed-and-dropped* would be counting units on
screen. **⇒ TASK-527's check is: type "send all units except miners to mid" and read this one line.**

---

## 6. ⛔ ANSWERING SPEC (5) FROM THE ARTIFACT: **ARE MINERS IN THE `who:"all"` POPULATION? — YES, BOTH KINDS OF IT**

**Verified by reading, not relayed.** The lead in the spec was correct, and here is the chain:

| link | artifact | verdict |
|---|---|---|
| the selector's zone gate | `SiegeAssistantComponent.cpp` — `Unit->IsGroupCommandEligible()` | |
| that predicate | `SummonedUnit.cpp:1946-1960` — `return CanTakeZoneOrders() && Team == ETeamId::Blue && !bDead && !bAIFrozen;` | |
| the base | `SummonedUnit.cpp:1925` — `return Profile == ECardProfile::Standard;` | a miner is not `Standard` |
| ⭐ **the override** | **`MinerUnit.h:413` — `virtual bool CanTakeZoneOrders() const override { return true; }`** | ✅ **UNCONDITIONALLY TRUE** (TASK-397) |

⇒ ✅ **A live, un-frozen Blue miner IS selected by `who:"all"` for `send` / `guard` / `ambush`
today.** Jonathan's flagship sentence *"send all units except miners"* therefore **changes what
happens on screen** — the miners stay mining instead of marching. **The feature is not a no-op for
his own example.**

**And the FOLLOW half, which the spec did not ask about but which the same selector serves:**
`IsFollowCommandEligible()` → `AMinerUnit::CanFollowHero()` → `bFollowOnSpawn ||
bSpawnFollowEnrollWindowClosed` (`MinerUnit.h:392`). That window is **one synchronous call stack
wide and closes unconditionally in `AMinerUnit::BeginPlay` (`MinerUnit.cpp:127`)**, so from the
instant a miner finishes spawning it is follow-eligible too. ⇒ ✅ **Miners are in BOTH populations**,
and *"follow me except the miners"* is equally meaningful. The `§5` spawn-default ruling (a miner
spawns mining, not following) is a **spawn-time** rule and does not narrow the population here.

📌 **The Sorcerer, for completeness:** `SorcererUnit` keeps `Profile == Standard` deliberately, so it
was **always** in this population — `AS-§20.2` already exonerated the executor and named the prompt
as the cause. **Nothing in this task changes the sorcerer's selection**, and nothing needed to.

---

## 7. ⛔ ANSWERING SPEC (4): WHICH INTENTS REACH THIS SELECTOR — AND IS THE PARSER'S REFUSAL COMPLETE?

**Read off `ExecutePendingCommand`'s switch (`.cpp:1660-1702`), every case:**

| intent | executor | reaches `SelectUnitsForOrder`? | `SiegeAssistantIntentTakesSelection` |
|---|---|---|---|
| `Send` | `ExecuteZoneOrder(Hold)` | ✅ yes | ✅ true |
| `Guard` | `ExecuteZoneOrder(Hold)` | ✅ yes | ✅ true |
| `Ambush` | `ExecuteZoneOrder(Ambush)` | ✅ yes | ✅ true |
| `Follow` | `ExecuteFollowOrder` | ✅ yes (`bFollowOrder = true`) | ✅ true |
| `Charge` | `Controller->ApplyArmyWideStance(Attack)` — **inline, one statement** | ⛔ **no** | ⛔ false |
| `Fallback` | `Controller->ApplyArmyWideStance(Defend)` — **inline, one statement** | ⛔ **no** | ⛔ false |
| `Rally` | `ExecuteRallyOrder` → `Hero->Rally()` | ⛔ **no** | ⛔ false |
| `None`/default | logs and returns false | ⛔ no | ⛔ false |

> ### ⭐ **THE SET OF INTENTS THAT REACH THE SELECTOR IS EXACTLY THE SET FOR WHICH `SiegeAssistantIntentTakesSelection()` IS TRUE. The two are the same four, with no gap in either direction.**

⇒ ✅ **TASK-518's refusal covers every route into the three army-wide executors**, because it gates on
**that same shipped predicate** rather than on a second list of verbs. `Charge` and `Fallback` are
**two inline statements** in the switch — there is no helper either could reach the selector
through — and `ExecuteRallyOrder` takes no `Command` at all (its signature is
`(ASiegePlayerController&)`), so it is **structurally incapable** of seeing an `ExcludeKinds`.

⛔ **I added NO second enforcement for those three**, exactly as instructed. The only new refusal in
the executor is for a shape the parser and the validator already refuse identically — see §8.

**Route completeness into `RouteParsedCommand`, checked so "every route" is a reading and not a
hope** — three call sites, all inside this component: `HandleModelCompletion` (validated at `:987`,
now WITH the exclusion), the shortfall short-circuit (`.cpp:2614`, a copy of an already-validated
command whose `Kinds` is non-empty by construction, so `ExcludeKinds` is necessarily empty), and the
deferred fire (`.cpp:2150`, replaying a command validated when it was parsed). **No fourth entry
point exists** — the cheat manager reaches the composer, never the executor.

---

## 8. ⚠️ THE ONE JUDGEMENT CALL I MADE — A BACKSTOP AT THE TOP OF THE SELECTOR, FLAGGED FOR A RULING

```cpp
if (Command.ExcludeKinds.Num() > 0 && Command.Kinds.Num() > 0)
{
    UE_LOG(LogSiegeAssistant, Warning, TEXT("Selector refused - ... BOTH a %d-kind selection and a %d-kind exclusion ..."));
    return false;
}
```

**Why it is here:** the spec's own words — *"make sure nothing in the executor silently ignores an
exclusion it was handed."* The per-kind loop below has **no subtraction step**, so a command carrying
both would execute *"send 10 footmen except the miners"* as *"send 10 footmen"* — the exception
parsed and then dropped, which is the failure the schema was shaped around.

**Why it is NOT the forbidden second enforcement:** the thing the spec bars is a **divergent** second
ruling on the **army-wide intents**, and I added none of that. This refuses a **different** shape
(selection + exclusion), in the **same direction**, with the **same verdict**, as two gates that both
already refuse it — the parser's cross-field check 2 and `SiegeAssistantValidateSelection`, which
now runs with the exclusion. It cannot produce an outcome those gates would not.

⚠️ **`Warning`, not `Log`, and that is a decision I want ruled on.** The file's standing rule is that
a refusal caused by the *model* is logged at `Log` because the automation runner reads a `Warning` as
a test failure. Reaching **this** line is not a model error — it is a code or wire defect in a state
two gates refuse — so it should be loud. No automation test can trigger it (the TASK-523 tests have
no world), so the `Warning` costs no green bar. ⇒ **If TASK-525 prefers `Log` for uniformity, say so;
I will not defend the level, only the check.**

---

## 9. ⛔ DEV-32 — THE REFUSAL FOR A KIND THAT IS NOT IN THE ROSTER AT ALL IS **NOT** MINE, AND I DID NOT PAPER OVER IT

DEV-32 is *"send everyone except the catapults at their castle"*, `ExpectOutcome = Refuse`. **Checked
at the artifact, that refusal is owned by two layers ABOVE the executor and neither moved:**

1. **The grammar.** `exceptlist` is generated from the **live roster kinds**, so a noun that is not a
   `DT_Cards` row **has no terminal to be sampled into**. The model physically cannot emit
   `all_except:["catapult"]`.
2. **Zone A's `[FORCES]` rule** — *"if the unit named is not a kind in `[FORCES]`, answer
   `{"ask":"unsupported"}`"* — which is what actually produces the `Refuse` the corpus scores. ⭐
   TASK-517's `other_kinds:` **names** change is what makes that rule TRUE again on a collapsed
   board, and TASK-521 owns its wording.
3. ⛔ **`ValidateCommandAgainstSnapshot` — the non-orderable-kind guard — CANNOT reach it, and I
   confirmed rather than assumed this:** it **early-returns `true` when `Command.Kinds.Num() == 0`**
   (`SiegeAssistantSnapshot.cpp:702-705`), which is every exclusion command. **It is TASK-517/521's
   file and I did not touch it.**

⇒ ⚠️ **STATED PLAINLY FOR THE GATE: at the EXECUTOR layer, an excluded kind that is not alive
subtracts nothing and the order proceeds — I log it explicitly rather than refusing** (§2b's third
log). **That is deliberate:** *"send everyone except the miners"* with no live miner **is** *"send
everyone"*, and refusing a correct sentence because the board happens to be empty of the excepted
kind would punish the player. **Refusing it here would ALSO paper over DEV-32** by producing the
right verdict for the wrong reason, at a layer that cannot tell *"not alive"* from *"not a unit at
all"*. 📌 **If DEV-32 fails at TASK-527, the instrument to look at is Zone A's rule (TASK-521) and
the corpus note (TASK-524) — NOT this executor.**

---

## 10. ⚠️ AUTHORIZED SCOPE ADDITIONS — the two stale diagnostics, declared so the gate rules on them

### (a) ⛔ `SiegeCheatManager.cpp:670` — was **OUTRIGHT FALSE**, spec-authorized, **ONE LINE**

**Before:** *"EXPECTED ROSTER DIVERGENCE ON A 13-KIND BOARD … this lane prints MaxRosterKinds=8 rows
plus a COMPUTED 'other_kinds: `<N>` kinds, `<M>` units'; the harness prints 13 rows plus a HARDCODED
'other_kinds: none'. Same key set, five absent rows and a different value."*

**Every clause of that is now wrong:** TASK-517 raised the cap to **13** *and* changed
`other_kinds:` to print **names**. On a 13-kind board this lane now prints **13 rows** and
**`other_kinds: none`** — identical to the harness. An operator diffing the two lanes would have been
told to expect a divergence that is not there, on the one command whose whole job is comparing lanes.

**After:** the divergence is recorded as **CLOSED**, with the condition under which it re-appears
(**more than 13** commandable kinds ⇒ this lane collapses to `other_kinds: <names> (<N> units)` while
the harness still prints a hardcoded `none`), and the standing ⛔ *do NOT 'align' anything* kept
verbatim. ⛔ **Nothing else in that file was opened for editing** — the diff is one line.

### (b) `SiegeAssistantComponent.cpp` `ReportFirstCapture` — the stale `B+C=738`

The `FIRST LIVE CAPTURE` reference line claimed *"the SHIPPED builder was DERIVED at B+C=738"*, which
was derived at `MaxRosterKinds = 8`. **I used TASK-517's measured numbers and invented none:** Zone C
= **887** of an **893**-char budget, head **108** / roster **621** / tail **158**, ~**6 chars** of
headroom, an owner-accepted risk. ⛔ **I did NOT assert a shipped `B+C`**, because **the shipped
`zoneB` has still never been printed** — taking that reading is exactly what the `OWED READING` line
below it exists for, and inventing a sum from the spike's `zoneB=68` would have re-created the
lane-mixing defect that line was written to end. The spike fixture's `zoneB=68 zoneC=887 (B+C=955)`
reference is left **untouched** — it is a real, correctly-attributed reading.

### (c) ⛔ `SiegeLlamaSpike.cpp:5785` — **NOT TOUCHED**, as instructed

TASK-517 verified it stays green because it measures the **spike's own** `AppendZoneB`/`AppendZoneC`.
**I did not open `Plugins/SiegeLlama/**` at all** (D4 / `FT-§16`).

---

## 11. STANDING TRAPS — checked, one by one

| trap | status |
|---|---|
| shadowing an inherited reflected member | ✅ **no new member of any kind** — the only new names are three function locals (`EligibleCount`, `ExcludedCount`, and a loop `Index`) |
| most-vexing-parse | ✅ no new object is default-constructed; `Contains(...)` takes an rvalue `FName` returned by a call |
| complete-type includes | ✅ **no new include needed.** `ASummonedUnit` (`GetCardID`) and `FSiegeAssistantCommand` were already complete in this TU — the same `Candidate.Unit->GetCardID()` call exists 40 lines below mine |
| broadcasting a delegate on a no-op | ✅ **no delegate is broadcast by this task at all.** The refusal travels as an existing `PushMessage`, which the file's own comment classifies as an EVENT rather than a value |
| `FName` case | ✅ relies on the shipped case-insensitive `operator==`; ⛔ no second lower-casing |
| a `nullptr` `Candidate.Unit` | ✅ impossible — a candidate is only appended after `IsValid(Unit)` in the same loop, and `Eligible` is a function local that cannot outlive the call |
| format specifiers | ✅ four `%d`, four `int32` arguments, in order, in each of the three new log lines |

---

## 12. ⚠️ WHAT QA SHOULD SCRUTINISE (TASK-525)

1. ⭐⭐ **§1 — the four-argument call at `:987`.** One grep. If it has three arguments, this task
   failed and nothing would have told you.
2. ⭐ **§4 — the empty-after-exclusion refusal actually reaches the player.** Follow the four steps;
   the load-bearing one is that `ExecuteAndReport`'s `PushMessage(... : AskUnsupported)` is
   **unconditional**.
3. **§8 — the backstop's existence and its `Warning` level.** The one place I exercised judgement.
   Rule on it rather than inheriting it.
4. **§3 — my claim that the ghost circles are place-only.** It is the premise of my whole answer to
   the preview clause; check `SpawnConfirmPreview` and disagree if you read it differently.
5. **§9 — the DEV-32 division of labour**, and specifically that I was RIGHT not to refuse a
   dead-but-real excluded kind in the executor.
6. **§6's chain** — `MinerUnit.h:413` is the whole finding; if that override ever becomes
   conditional, Jonathan's flagship example silently stops doing anything.
7. **§10(a)** — the authorized `SiegeCheatManager` line: confirm the new text is true and that the
   diff really is one line.

## 13. Not done, by instruction
⛔ Not compiled (**TASK-526** owns the only compile) · ⛔ no tests (**TASK-523**) · ⛔ no corpus edit
(**TASK-524**) · ⛔ no Git · ⛔ no editor / MCP / PIE · ⛔ no `Content/` asset · ⛔ no parser, grammar,
snapshot, widget or controller edit.
