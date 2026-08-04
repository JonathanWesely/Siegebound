# TASK-479 — [FT-A4] `DumpAssistantPrompt` — THE SHIPPED-LANE PROMPT DUMP

**Agent:** gameplay-programmer
**Date:** 2026-08-03
**Status:** ⛔ **BLOCKED-ESCALATED — NOT IMPLEMENTED. NO CODE WRITTEN. NO FILE CHANGED.**

> ⚖️ **This is a §15 declared departure, not a failure to deliver.** The spec's mandated
> mechanism is **unreachable as specced**, and the two ways around it are each **banned by
> this task's own spec**. Per the dispatch's standing instruction — *"If reading the prompt
> requires refactoring the thing that builds it, **stop and report** rather than reshaping
> the lane you are trying to measure"* — I stopped. **Nothing was written to the tree.**

---

## 1. WHAT I CHANGED

**Nothing.** Zero source files touched. Verified:

```
$ git status --porcelain
 M GitClaudeUnrealTest/.claude/pipeline/CONVENTIONS.md      <- pre-existing, not mine
 M GitClaudeUnrealTest/.claude/pipeline/TASKBOARD.md        <- concurrent agent, not mine
?? GitClaudeUnrealTest/.claude/pipeline/handoffs/TASK-470-buildmaster.md   <- not mine
```

`SiegeCheatManager.{h,cpp}`, `SiegeAssistantComponent.{h,cpp}` and
`SiegeAssistantSnapshot.{h,cpp}` are **byte-unchanged**. No Git, no compile, no editor,
no MCP, no PIE. `MaxRosterKinds` untouched. Nothing on the protected list
(`GetVocabulary`'s fallback, `BeginPlay`'s prefix registration,
`EnsureStaticPrefixRegistered`, the `ShortfallCount` row, `PushMessage`'s `{Intent}`
argument, `DescribeCommandForPlayer`'s verb-neutral frames) was read-modified or
otherwise disturbed.

**M8 DECLARATION (owed by TASK-480 criterion 7), stated verbatim:**
*"adds no replicated property, no new replicated class, no new relevancy tier."*
Trivially true — nothing was added.

---

## 2. THE BLOCKER — TWO INDEPENDENT FAULTS, BOTH VERIFIED BY SYMBOL (§18c)

Spec clause **(2)** is unambiguous:

> **⛔ IT MUST GO THROUGH `USiegeAssistantSnapshot::Capture` AND `ComposeTurnPrompt` —
> NEVER A REIMPLEMENTATION.** A dump that assembles its own string measures nothing.

### FAULT A — `ComposeTurnPrompt` is **private**, and its privateness is a **documented architectural invariant**, not an oversight

Verified by symbol, not offset. `SiegeAssistantComponent.h` has **exactly one**
`public:` (line 621) and **exactly one** `private:` (line 913). Everything the dump needs
sits **after 913**:

| Symbol | Line | Access |
|---|---|---|
| `USiegeAssistantComponent::ComposeTurnPrompt(const FString&)` | 1200 | **private** |
| `USiegeAssistantComponent::CaptureTurnSnapshot()` | 1197 | **private** |
| `USiegeAssistantComponent::GetCachedZoneA()` | 1213 | **private** |
| `USiegeAssistantComponent::GetVocabulary()` | 1237 | **private** |
| `USiegeAssistantComponent::BuildPendingLine()` | 1188 | **private** |

**There is no `friend` declaration anywhere in the file.** §14 positive control: the same
regex alternation that returned `public:` (621), `private:` (913) and 13 `UFUNCTION`
hits returned **zero** matches for `friend ` — so the negative is trustworthy rather than
a silent tool failure.

⛔ **And this is the part that turns an inconvenience into an escalation.** The
privateness is **mechanism #2 of the four** that the class comment says make the
translator-not-agent law *structurally true rather than merely observed*
(`SiegeAssistantComponent.h:49-65`):

> *"Four mechanisms, each of which a later edit would have to **DELETE** rather than merely
> overlook — which is the difference between a rule and a comment: […]
> 2. ⛔ THE FSM — NOT THE EXECUTOR, AND NOT THE MODEL CALL — COMPOSES THE PROMPT.
> **`ComposeTurnPrompt()` is private** and assembles Zone A + Zone B + Zone C itself; the
> TASK-443 seam receives a FINISHED prompt string and has no route to a zone builder.
> ⇒ The one function that could feed the model its own previous words cannot reach the
> model, and the one that reaches the model cannot compose."*

⇒ **Making it public is not a neutral access flip.** It hands *every* future caller —
including the executor seam the mechanism exists to fence off — a route to the composer.
The mechanism's whole value is that it is structural.

⚠️ **The tempting precedent does not cover this, and I checked it before relying on it.**
`SiegePlayerController.h:299-310` records a sanctioned private→public move
(`SpawnGroupCircleDecal`, TASK-440), complete with *"The BODY IS UNTOUCHED — this is an
access change and nothing else"* and an explicit rejection of `friend` as the wider grant.
**That precedent is about a function carrying no invariant.** `ComposeTurnPrompt` carries
a named, numbered one. The precedent licenses the *technique*; it does not license
spending *this* invariant.

### FAULT B — `ComposeTurnPrompt` **cannot return what the spec asks it to emit**, even if access were granted

Spec clause **(1)** requires the exec to write *"the **exact bytes** plus `zoneA_chars` /
`zoneB_chars` / `zoneC_chars`"*. The shipped composer
(`SiegeAssistantComponent.cpp:2727-2752`) returns **only the concatenation**:

```cpp
FString USiegeAssistantComponent::ComposeTurnPrompt(const FString& Utterance)
{
	const FString& ZoneA = GetCachedZoneA();
	const FString  ZoneB = Snapshot->BuildZoneB();
	const FString  ZoneC = Snapshot->BuildZoneC(Utterance, BuildPendingLine());

	ReportFirstCapture(ZoneA, ZoneB, ZoneC);

	FString Prompt;
	Prompt.Reserve(ZoneA.Len() + ZoneB.Len() + ZoneC.Len());
	Prompt += ZoneA;
	Prompt += ZoneB;
	Prompt += ZoneC;

	return Prompt;
}
```

There is **no out-param and no member** holding the three lengths, and the prompt is
never stored (mechanism #1: the raw string is *"A LOCAL FOREVER"*; the only stored zone is
`CachedZoneA`, private, line 1354).

⚠️ **And the one place that does print the three counts is latched to fire once per
session.** `ReportFirstCapture` (`SiegeAssistantComponent.cpp:2902-2932`) returns
immediately on `bLoggedFirstCapture` (line 2904, set line 2909, **never reset**). So on
any session where the player has already typed one sentence, the counts are simply
**unavailable** — and on the first invocation the exec would *consume* the shipped
first-capture audit, corrupting an observation TASK-447 owns.

⇒ Satisfying clause (1) needs a **new accessor** on `USiegeAssistantComponent`, not merely
an access change. That is a larger edit to the same non-owned file.

---

## 3. WHY I DID NOT ROUTE AROUND IT — BOTH ALTERNATIVES ARE BANNED, AND ONE IS ALSO *WRONG*

**Alternative 1 — reassemble the zones in `USiegeCheatManager`.** Mechanically possible:
`USiegeAssistantSnapshot` is `GITCLAUDEUNREALTEST_API` and `Capture` / `BuildZoneA` /
`BuildZoneB` / `BuildZoneC` are all public.

⛔ **Banned by spec clause (2) verbatim, and it would produce a false artifact.** To
rebuild Zone A it must choose a vocabulary; `GetVocabulary()` is private, so it would have
to **guess the lane** — and that is *precisely* the defect this batch exists to catch. The
shipped code records the cost (`SiegeAssistantComponent.cpp:2844-2852`): the first live run
printed `zoneA_chars=3029` with `vocabulary=none` against a measured lane of **5116**,
i.e. the whole synonym table missing while every eval number described the other lane. To
rebuild Zone C it must supply the pending line; `BuildPendingLine()` is private, so it
would pass empty and silently diverge on every clarification turn.

⇒ A dump built this way would be **lane four** — a confident green describing a different
string (**T1** and **T8**). It would make M3 *pass* while measuring nothing. That is the
single worst outcome available here.

**Alternative 2 — drive the shipped public entry `SubmitUtterance`.** ⛔ Not read-only:
it advances the FSM, increments `TurnId`, pushes player-facing messages, dispatches to the
model and can execute an order. Spec clause (1): *"this is the ONLY exec this batch adds,
and it is **READ-ONLY** — no state change."* It also never returns the prompt.

**Alternative 3 — read `GetTurnSnapshot()` (public, line 869).** Returns
`const USiegeAssistantSnapshot*`, **null before the first capture**, and reflects whatever
sentence the *player* last submitted — not the utterance passed to the exec. It cannot
answer "what would the shipped lane build for *this* string".

---

## 4. WHY THIS IS AN ESCALATION AND NOT A TASK DECISION

The fix — whichever form it takes — lands in **`SiegeAssistantComponent.{h,cpp}`**.

- ⛔ **Manager ruling 11 (SINGLE OWNER PER FILE):** `SiegeAssistantComponent.{h,cpp}` =
  **TASK-489 ONLY**.
- ⛔ **TASK-480's own gate scopes that file as "Reference-only"** for this review
  (*"Files under review: `SiegeLlamaSpike.cpp` · `SiegeCheatManager.{h,cpp}`.
  Reference-only: `SiegeAssistantComponent.cpp`, `SiegeAssistantSnapshot.cpp`"*). QA is not
  set up to gate a change there under this task.
- ⛔ **TASK-489 cannot absorb it — the dependency runs the wrong way.** TASK-489 is
  `blocked-by: TASK-486` (Jonathan's D1 ruling), TASK-486 is fed by **M3's diff**, and M3's
  shipped half is **this task**. Routing the fix through TASK-489 makes the batch circular:
  479 → M3 → 486 → 489 → 479.

⚖️ **This is structurally the same shape as D2, which the board already rules is
Jonathan's:** *"all three candidate fixes move something frozen ⇒ it is his."* Here the
candidate fixes move a **documented architectural mechanism** and a **single-owner file
lock**. I am not authorised to spend either, and doing it quietly is exactly how a
mechanism decays into a comment.

⚠️ **I also deliberately did not write the exec "ready to compile once unblocked".** The
game module has exactly **one** Stage-A compile gate (TASK-481) under the QUIET-MODULE LAW
across two live batches. Landing knowingly-non-compiling code into it would burn that gate
and count as a QA loop — a self-inflicted version of the failure this batch is trying to
measure.

---

## 5. THE DECISION THE ORCHESTRATOR / MANAGER NEEDS TO MAKE

Three options, cheapest first. **All three edit `SiegeAssistantComponent.{h,cpp}`.**

| # | Option | Cost | What it spends |
|---|---|---|---|
| **A** | **New dev-only accessor**, `#if !UE_BUILD_SHIPPING`, e.g. `bool BuildDebugPromptDump(const FString& Utterance, FString& OutPrompt, int32& OutZoneA, int32& OutZoneB, int32& OutZoneC);` — public, calls `CaptureTurnSnapshot()` + the same three builders in the same order, returns the counts. | ~25 lines | Keeps `ComposeTurnPrompt` **private** ⇒ **mechanism #2 survives intact**. ⚠️ But it is a *second* composer unless it is written to delegate — and a second composer is lane four again. Must be built as `ComposeTurnPrompt` + out-params, not beside it. |
| **B** | **`ComposeTurnPrompt` private → public** (SpawnGroupCircleDecal idiom, body untouched) **+** a separate route to the counts. | ~2 lines + counts | ⛔ Spends **mechanism #2**. Needs an explicit ruling that the invariant is being deliberately weakened, recorded where mechanism #2 is stated. |
| **C** | **Re-scope TASK-479's output**: emit the **exact prompt bytes only**, drop `zoneA/B/C_chars` from the exec and take them from the existing `FIRST LIVE CAPTURE` log line instead. | smallest | ⛔ Still needs A or B for access. Only removes FAULT **B**, not FAULT **A**. |

📌 **My recommendation, offered as input and not as a decision: Option A**, written as a
thin wrapper that calls the *existing* `ComposeTurnPrompt` body via out-params (i.e.
refactor `ComposeTurnPrompt` to fill three `int32&` it already has in hand, and have the
public dev accessor call it). It satisfies clause (2)'s *"never a reimplementation"*
literally — one composer, one code path — while leaving mechanism #2's fence standing,
because the public surface returns **bytes and counts**, never a route to a zone builder.

⛔ **I have NOT implemented it.** It is TASK-489's file and it touches a named invariant.

---

## 6. WHAT QA SHOULD SCRUTINISE

1. ⭐ **Verify FAULT A yourself against the artifact, not against this note** (the
   RELAYED-DIAGNOSIS LAW). `SiegeAssistantComponent.h`: one `public:` at 621, one
   `private:` at 913, `ComposeTurnPrompt` at 1200, no `friend`. If you find a public route
   I missed, this whole handoff is wrong and TASK-479 is simply undone.
2. **Verify FAULT B** — read `ComposeTurnPrompt`'s body (cpp 2727-2752) and confirm it
   returns only the concatenation, and that `ReportFirstCapture` is latched (cpp 2904/2909).
3. **Confirm I changed nothing.** `git status --porcelain` should show no source file.
4. **Rule on whether TASK-480 can gate at all.** Three of its four inputs (TASK-476/477/478,
   plugin module) may be deliverable; **criterion (6) — *"TASK-479 goes through the SHIPPED
   path"* — has nothing to inspect.** §29's coverage-ledger point stands: this is a property
   of the SET, and the set is incomplete.

---

## 7. UNVERIFIABLE WITHOUT A COMPILE

Everything in this handoff is a **static, symbol-level** reading of the tree — access
specifiers, signatures, latch flags, call order. **None of it required or received a
compile**, and none of it is a runtime claim.

⚠️ **What remains unobserved and is NOT claimed here:** the actual shipped byte counts
(`zoneA_chars` / `zoneB_chars` / `zoneC_chars`), whether the roster truncation log fires on
a live board, and the D2 delta itself. **§12g's standing WARN is unchanged by this task:
no command prints the shipped `BuildZoneA` today, and after TASK-479 that is still true.**
Every byte figure on record still comes from the spike's `AppendZoneA`.

📌 **One corroboration worth carrying into M3 anyway, found while tracing (read-only):**
**D2 is already declared in the shipped source.** `SiegeAssistantSnapshot.cpp:931-934`:

> *"⚠️ SHIPPED-VS-MEASURED HAZARD, DECLARED: `[FORCES]` is truncated to `MaxRosterKinds` (8)
> in the shipped snapshot but NOT in the spike fixture, which prints all 13."*

⚠️ **And D2 is sharper than the board states it.** The shipped Zone C emits `other_kinds:`
as a **fixed key that is ALWAYS present** — it prints `other_kinds: none` when nothing
collapsed (`SiegeAssistantSnapshot.cpp:1313-1320`), and the fixed key order is
*places, roster, **other_kinds**, stances, hero, pending, order* (line 1325). ⇒ **M3's diff
will show a KEY-SET difference, not merely a truncated value list** — the harness has no
such key at all. Whoever runs M3 should expect that and not read it as a second defect.

---

## 8. SLACK

Posted to `#siegeboundue5agentteam`, thread **⚙️ Dev & QA** (`1783116269.740549`),
prefixed `⚙️ GAMEPLAY-PROGRAMMER · ⛔ TASK-479`.

---
---

# TASK-479 — PART 2: IMPLEMENTED (re-dispatch, 2026-08-03)

**Agent:** gameplay-programmer
**Status:** ✅ **IMPLEMENTED — `ready-for-qa` (gate TASK-480)**

> ⛔ **PART 1 ABOVE IS THE RECORD OF WHY THIS SHAPE WAS CHOSEN AND IS NOT OVERWRITTEN.**
> It is the escalation that produced the ruling; this part is what the ruling authorised.
> ⚠️ **One claim in PART 1 §7 is now KNOWN FALSE and is corrected in §2.6 below** —
> I am flagging my own error rather than leaving it for a reader to trip over.

---

## 2.1 THE CARVE — EXACTLY ONE ADDITION, AND IT IS PROVABLY AN ADDITION

Four files, **354 insertions, ⭐ ZERO DELETIONS.** Verified read-only, not asserted:

```
$ git diff --numstat -- Source/GitClaudeUnrealTest/Siegebound/
83   0   SiegeAssistantComponent.cpp
106  0   SiegeAssistantComponent.h
107  0   SiegeCheatManager.cpp
58   0   SiegeCheatManager.h

$ git diff -U0 -- Source/GitClaudeUnrealTest/Siegebound/ | grep -E "^-[^-]"
(no output)
```

⭐ **NOT ONE PRE-EXISTING LINE WAS MODIFIED OR REMOVED IN EITHER FILE.** That is a
stronger statement than "no shipped behaviour changed" and it is mechanically
checkable in one command — QA should run it rather than read this paragraph
(the RELAYED-DIAGNOSIS LAW).

⚠️ **`Plugins/SiegeLlama/Source/SiegeLlama/Private/SiegeLlamaSpike.cpp` also shows
as modified (+625). ⛔ THAT IS TASK-476's, NOT MINE.** I opened it **read-only**
twice (§2.6) and never wrote to it. Stated here because the two batches are live
in the same tree and a later reader will otherwise attribute it to this task.

---

## 2.2 THE ACCESSOR — EXACT SIGNATURE

`Source/GitClaudeUnrealTest/Siegebound/SiegeAssistantComponent.h`, in the **`public:`**
block, immediately before `private:`:

```cpp
#if !UE_BUILD_SHIPPING
	FString DebugCaptureAndComposePrompt(const FString& RawUtterance);
#endif // !UE_BUILD_SHIPPING
```

- ⛔ **NOT a `UFUNCTION`.** No exec, no Blueprint node, no reflection entry — so it is
  not a surface a Blueprint or a future seam can discover. Its only caller is
  `USiegeCheatManager::DumpAssistantPrompt`.
- ⛔ **NO OUT-PARAM, NO NEW MEMBER, NO SECOND `ReportFirstCapture` CALL** (criterion 6d).
  It returns the finished string and nothing else; **empty == refused**, which is
  unambiguous because a successful compose can never be empty (Zone A alone is
  thousands of chars).
- ⚠️ **NAME NOT PINNED BY CONVENTIONS §5** — the accessor was invented by ruling §13(b),
  which names its shape but not its identifier. **Manager may want to add
  `DebugCaptureAndComposePrompt` to the §5 table**; I did not edit CONVENTIONS.
- ⚠️ **UHT SAFETY, CHECKED AGAINST THE ENGINE RATHER THAN ASSUMED:** an arbitrary
  `#if` cannot wrap a **reflected** member, but a **plain** member is fine — engine
  precedent in reflected classes is `UCheatManager::TickCollisionDebug` and
  `APlayerController::Debug_GetMostRecentInputStack`, both plain members wrapped in
  `#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)` inside a `UCLASS` body. **This is why
  the accessor is deliberately NOT a UFUNCTION** — the two constraints agree.

---

## 2.3 ⭐ THE MECHANISM-#2 ARGUMENT, AS IT APPEARS IN THE HEADER (criterion 8)

Quoted from the declaration's comment, verbatim in substance — the reader meets it
where the grant is made, per the `SpawnGroupCircleDecal` idiom:

> **⚖️ WHY THIS DOES NOT SPEND MECHANISM #2 — WRITTEN HERE, WHERE THE NEXT READER
> MEETS IT, BECAUSE AN ADDITION THAT LEAVES NO WRITTEN TRACE OF WHY IT IS SAFE HAS
> QUIETLY CONVERTED A MECHANISM INTO A COMMENT.**
>
> MECHANISM #2 (the class comment, §1's four) reads: ⛔ THE FSM — NOT THE EXECUTOR,
> AND NOT THE MODEL CALL — COMPOSES THE PROMPT. *"`ComposeTurnPrompt()` is private and
> assembles Zone A + Zone B + Zone C itself; the TASK-443 seam receives a FINISHED
> prompt string and HAS NO ROUTE TO A ZONE BUILDER. ⇒ The one function that could feed
> the model its own previous words cannot reach the model, and the one that reaches
> the model cannot compose."*
>
> ⚠️ **ITS PROPERTY IS "NO ROUTE TO A ZONE BUILDER" — ⛔ NOT "no public function ever
> returns a prompt". That distinction IS the ruling:**
>  - **THIS HANDS OUT A FINISHED, IMMUTABLE `FString` AND NOTHING ELSE.** No zone
>    builder, no `USiegeAssistantSnapshot`, no vocabulary, no pending line. A caller
>    holding the return value still cannot assemble a prompt, cannot re-order the
>    three zones, and cannot build a fourth.
>  - ⛔ **`ComposeTurnPrompt()` STAYS PRIVATE.** There is still **EXACTLY ONE COMPOSER**;
>    this CALLS it and never reproduces it. Two composers would be the same defect
>    this dump was created to measure — a confident green describing a string the game
>    does not build (traps T1/T8).
>  - **THE TASK-443 SEAM IS UNTOUCHED.** It still receives a finished string and still
>    cannot compose. ⚠️ **NOTHING THAT COULD NOT REACH A ZONE BUILDER YESTERDAY CAN
>    REACH ONE TODAY** — which is the mechanism, stated as a **property** rather than
>    as an access keyword.
>
> ⇒ **THIS IS AN ADDITION, NOT A RE-EXPOSURE.** ⛔ Widening `ComposeTurnPrompt` itself
> WOULD delete the stated barrier, and it was REFUSED (§13(b)). So was
> `friend class USiegeCheatManager`, on the ground
> `ASiegePlayerController::SpawnGroupCircleDecal`'s own comment already records
> (**cited by SYMBOL, not by line — §18c**) — friendship exposes EVERY private member
> to reach one function, i.e. **it is THE WIDER GRANT, NOT THE NARROWER ONE.**

---

## 2.4 PROOF THE COMPOSER IS STILL PRIVATE, AND THAT THERE IS STILL EXACTLY ONE

⭐ **Run these; do not take my word (§14 positive control is built into the first).**

```
$ grep -nE "^\s*(public|private|protected):|ComposeTurnPrompt|DebugCaptureAndComposePrompt|friend " SiegeAssistantComponent.h
625:  public:
1015: FString DebugCaptureAndComposePrompt(const FString& RawUtterance);   <- PUBLIC (625 < 1015 < 1018)
1018: private:
1305: FString ComposeTurnPrompt(const FString& Utterance);                 <- ⭐ STILL PRIVATE (> 1018)
```

- ✅ **`ComposeTurnPrompt` sits BELOW the sole `private:` — criterion 6b passes.**
- ✅ **`DebugCaptureAndComposePrompt` sits ABOVE it, below the sole `public:`.**
- ✅ **ZERO `friend` declarations.** ⚠️ **§14 positive control:** the same regex
  alternation returned `public:`, `private:` and both function hits, so the `friend`
  negative is a real absence and not a silent tool failure. (`friend` appears only as
  the word inside my new comment, which is why the check must be by declaration, not
  by substring.)

```
$ grep -nE "ComposeTurnPrompt|ReportFirstCapture" SiegeAssistantComponent.cpp
2727: FString USiegeAssistantComponent::ComposeTurnPrompt(...)   <- the ONE definition
 796:   const FString Prompt = ComposeTurnPrompt(Utterance);     <- caller 1: SubmitUtterance step 8
2833:   return ComposeTurnPrompt(Utterance);                     <- caller 2: MINE, delegating
2743:   ReportFirstCapture(ZoneA, ZoneB, ZoneC);                 <- the ONE call, inside the composer
2985: void USiegeAssistantComponent::ReportFirstCapture(...)     <- definition, not a call
```

- ✅ **ONE composer, TWO callers, and mine is a `return` of the delegate's value** —
  criterion 6c passes.
- ✅ **`ReportFirstCapture` still has EXACTLY ONE call site** and it is not mine —
  criterion 6d passes.
- ✅ **`SiegeCheatManager.{h,cpp}` contain NO `BuildZone*` call and NO write to
  `MaxRosterKinds`** (it appears only inside a log string and a comment). The zones
  are not reassembled anywhere — §13(b)'s hard ban.

---

## 2.5 WHAT THE ACCESSOR DOES, AND THE FOUR REFUSALS

It runs **`SubmitUtterance`'s steps 7 and 8 and stops**: ⛔ no `BeginTurn`, no `TurnId`,
no dispatch, no `SetState`, no `PushMessage`, no order executed, no deferred latch touched.

| Gate | Behaviour | Why it is there |
|---|---|---|
| **empty utterance** | trims with the **same single `TrimStartAndEnd()`** gate 1 uses, then refuses | ⚠️ That trim is the **ONLY** transform the shipped lane applies before step 8. `SanitizeForPrompt`, the `MaxUtteranceBytes` cap and the flattening warnings all live **INSIDE `BuildZoneC`** and are therefore **inherited by delegating, never repeated.** |
| **no authority** | mirrors gate 3, refuses | On a client the composer is never reached, so a dump taken there would describe **a lane that does not run** — this batch's founding defect, re-committed by the instrument built to close it. |
| **FSM not at rest** | ⭐ **WHITELIST**: `Idle` / `Composing` / `Failed` admitted, everything else refused | ⛔ **THE ONE REAL HAZARD I FOUND, AND IT IS NOT IN THE SPEC.** `Capture()` **re-surveys the single snapshot object IN PLACE**, and `GetTurnSnapshot()`'s own contract says *"a second survey mid-turn would silently answer a different question from the one the model was asked."* `Thinking` / `AwaitConfirm` / `Clarify` / `Deferred` each still consult the current survey downstream (`RouteParsedCommandInternal`, `ConfirmPressed`'s execution, `FindShortfall`). **Without this guard the dump would silently corrupt a live turn — i.e. it WOULD change shipped behaviour.** ⚠️ A **whitelist** so a state added later is refused by default rather than admitted by omission. |
| **survey failed** | refuses, logs | `CaptureTurnSnapshot()` already refuses rather than guesses a team. |

⚠️ **GATE 2 (the fault latch) IS DELIBERATELY *NOT* MIRRORED, AND I AM DECLARING IT
RATHER THAN LETTING QA FIND IT.** The fault latch means **the model** is unavailable; it
says nothing about the **prompt builder**. Refusing there would make the instrument
useless in exactly the session where you most want to read the prompt, and it cannot
change a single byte the builder produces. **If QA disagrees this is a one-line add.**

⚠️ **WHAT IT CONSUMES — DECLARED, NOT OVERLOOKED.** Both are session-latched one-shots
that fire **inside** the delegated path, so delegation cannot avoid them without
becoming a second composer:
1. **`bLoggedFirstCapture`** — the `FIRST LIVE CAPTURE` audit. ✅ Ruling R3 is confirmed
   against the artifact: `ReportFirstCapture` is called **inside `ComposeTurnPrompt`**
   (cpp 2743), so it is spent by whatever reaches the composer first, **including the
   player's first real sentence.** ⇒ **TASK-485 must run the exec and quote the audit
   line in the SAME session.** The success log says so explicitly, so the operator is
   told at the moment it matters.
2. ⚠️ **`bZoneAIdentityChecked` — A SECOND CONSUMABLE NOBODY HAS BOARDED.**
   `GetCachedZoneA()` runs §8's Zone-A byte-identity re-check **on the second call of a
   session** (cpp 2786-2808) and latches. **The dump may be that second call.** This is
   the same class of resource §13(c)'s new law names, and it is the second instance —
   📌 **recommend the manager board it the same way** (it is harmless, but it is one
   more one-shot that two tasks could both plan to spend).

---

## 2.6 ⛔ I AM CORRECTING MY OWN PART-1 CLAIM, AND I VERIFIED THE CORRECTION FIRST-HAND

**PART 1 §7 asserted:** *"M3's diff will show a KEY-SET difference, not merely a
truncated value list — the harness has no such key at all."* ⛔ **THAT IS FALSE ON THE
ARTIFACT.** The manager caught it (CONVENTIONS "THE FINE-TUNE RUNG" §3). ⚠️ **I did not
take the correction on relay either — I read both lanes myself:**

| lane | evidence | verdict |
|---|---|---|
| spike | `SiegeLlamaSpike.cpp` — a single `other_kinds` emission, `Out += TEXT("other_kinds: none\n");`, **outside any `if`**, commented *"ALWAYS EMITTED … Nothing collapses in EITHER fixture by construction"*, after a roster loop with **no cap** | **13 rows + a HARDCODED `none`** |
| shipped | `SiegeAssistantSnapshot.cpp` — **two** branches, `none` and `Appendf("other_kinds: %d kinds, %d units")` | **8 rows + a COMPUTED value** |

✅ **THE KEY IS PRESENT IN BOTH LANES; THE KEY SET IS IDENTICAL** (both files carry the
same fixed-key-order comment). ⇒ **On `t0`, expect EXACTLY TWO differences: five absent
roster rows, and the `other_kinds:` VALUE.** **That corrected expectation is written
into the success log line**, so whoever reads the artifact is warned in the artifact's
own output and does not chase a defect that is not there.

⚠️ **The sharper point, which I only saw on re-reading:** the spike's `other_kinds:` is a
**constant**, so **the harness cannot express collapse AT ALL, on any fixture** — it does
not disagree about this board, it has no mechanism to disagree with.

---

## 2.7 FILES TOUCHED

- `Source/GitClaudeUnrealTest/Siegebound/SiegeAssistantComponent.h` — **+106/-0.** The
  accessor declaration + its mechanism-#2 argument, in `public:` before `private:`.
- `Source/GitClaudeUnrealTest/Siegebound/SiegeAssistantComponent.cpp` — **+83/-0.** The
  definition, placed **immediately below `ComposeTurnPrompt`** so one screen shows both
  and "it delegates" stays checkable by eye.
- `Source/GitClaudeUnrealTest/Siegebound/SiegeCheatManager.h` — **+58/-0.** The
  `UFUNCTION(exec) void DumpAssistantPrompt(FString Utterance);` (**§5's pinned
  signature, character-for-character**) + §8's "this is §5's own idiom" rationale on the
  class comment.
- `Source/GitClaudeUnrealTest/Siegebound/SiegeCheatManager.cpp` — **+107/-0.** The I/O
  shell + 6 includes (`HAL/FileManager.h`, `Misc/DateTime.h`, `Misc/FileHelper.h`,
  `Misc/Paths.h`, `Siegebound/SiegeAssistantCommand.h`, `Siegebound/SiegeAssistantComponent.h`).

**Assets referenced:** none — this task creates no asset and references no
`/Game/` path.

**M8 DECLARATION (owed by TASK-480 criterion 7), verbatim:**
*"adds no replicated property, no new replicated class, no new relevancy tier."*
✅ True: nothing added is a `UPROPERTY`, nothing replicates, and the accessor **refuses
without authority** — so it cannot even run on a client.

---

## 2.8 THE ARTIFACT THE EXEC PRODUCES

`Saved/SiegeAssistant/ShippedPrompt_<YYYYMMDD_HHMMSS>.txt` (UTC), absolute path logged.

- ⛔ **BYTES AND NOTHING BUT BYTES** — no header, no banner, no counts, no re-ordering,
  no trimming, no normalising. **It diffs against the harness dump with no
  interpretation step.**
- ⛔ **`ForceUTF8WithoutBOM` IS LOAD-BEARING, NOT STYLE.** ⚠️ `SaveStringToFile`'s
  **default is `AutoDetect`, which writes ANSI for an all-ANSI string and UTF-16
  otherwise** — i.e. **the file's ENCODING would depend on its CONTENT**, and one stray
  non-ASCII glyph would silently turn a byte diff into noise. Forcing it makes the
  encoding a property of the call. `SaveStringToFile` performs **no line-ending
  translation**, so the builders' `\n` survive as `\n`.
- ⚠️ **The filename is TIMESTAMPED so a second invocation cannot overwrite the first** —
  the first invocation is the one that may have spent the audit latch, i.e. the least
  replaceable file the command can produce. **The timestamp is in the NAME, never in
  the file.**
- **On a write failure ONLY**, the full bytes go to the log as a recovery copy, labelled
  as line-prefixed and **explicitly NOT the diff artifact.** On success the log carries
  the path and the length, so the clean artifact is the only thing anyone diffs.

---

## 2.9 ⚠️ WHAT QA SHOULD SCRUTINISE

1. ⭐ **Run the two greps in §2.4 yourself** (RELAYED-DIAGNOSIS LAW). If
   `ComposeTurnPrompt` is not below the sole `private:`, this is an automatic FAIL
   under criterion 6b **whatever this handoff says** — and I would rather you found it.
2. ⭐ **`git diff -U0 | grep -E "^-[^-]"` must print NOTHING** for the four files. Zero
   deletions is the whole no-shipped-behaviour-changed argument; it is one command.
3. ⭐ **Rule on the at-rest whitelist (§2.5).** It is **the one thing I added that the
   spec did not ask for**, and it is a behaviour decision: without it the dump can
   replace the snapshot a live turn is still reading. **If you think a diagnostic
   should never refuse, say so — but then the hazard needs another answer, not none.**
4. **Rule on the gate-2 (fault latch) non-mirror**, declared in §2.5. One-line add if
   you disagree.
5. **Check the guard token is `!UE_BUILD_SHIPPING` in all THREE places** — header decl,
   cpp definition, cheat-manager call site. ⚠️ **A mismatch is a LINK error in a Test
   build, not a compile error**, i.e. exactly the class of failure that survives a
   casual read.
6. **Confirm the exec signature matches §5 character-for-character:**
   `DumpAssistantPrompt(FString Utterance)` — by value, one param.
7. **Second consumable (`bZoneAIdentityChecked`, §2.5)** — decide whether it wants
   boarding under §13(c)'s new law. I did not board it; that is the manager's call.

---

## 2.10 ⛔ UNVERIFIABLE WITHOUT A COMPILE — AND I DID NOT COMPILE

**TASK-481 is the game module's only Stage-A compile gate under the QUIET-MODULE LAW.**
No build, no editor, no MCP, no PIE, no Git write. Everything above is a **static,
symbol-level** reading.

**Specifically NOT claimed:**
- ⚠️ **That it compiles.** The API calls are matched against in-tree precedent
  (`FString::TrimStartAndEnd`, `GetOwner()/HasAuthority()` — the exact pair
  `SubmitUtterance` gate 3 already uses — `IFileManager::MakeDirectory`,
  `FPaths::ProjectSavedDir`, `FFileHelper::SaveStringToFile`) and against engine
  headers for the UHT question, **but nothing here has been through a compiler.**
- ⚠️ **Every runtime number remains unobserved.** ⛔ **§12g's standing WARN is still
  true until TASK-485 actually runs this**: the shipped `BuildZoneA`'s byte count, the
  D2 delta's magnitude, and whether the roster-truncation log fires on a live board are
  all **still unmeasured**. **This task builds the instrument; it does not take the
  reading.** ⚠️ A PASS here means *"the instrument is correctly built"*, **never**
  *"the shipped lane has been measured."*

---

## 2.11 SLACK

Posted to `#siegeboundue5agentteam`, thread **⚙️ Dev & QA** (`1783116269.740549`),
prefixed `⚙️ GAMEPLAY-PROGRAMMER · ✅ TASK-479`.
