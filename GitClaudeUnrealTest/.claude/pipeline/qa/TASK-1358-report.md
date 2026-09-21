Verdict: PASS
# QA Report — TASK-1358 [AGENT-AIM-LATCH-GATE]
**Gate over `qa/TASK-1357-verify.md` (`Verdict: VERIFIED`, `verify: partial (4/6 observable)`).**
**BLOCKER 0 · WARN 4 · NIT 2.**

> ⛔ **THIS GATE PASSES AN INSTRUMENT, NOT AN ANSWER.** What it attests to is exactly this: the numbers in
> `TASK-1357` were taken under controls that could have failed, the arithmetic re-computes, the cell tokens
> are legal, and the probe write was reverted with a read-back. It does **NOT** attest that a card can be
> played, that a ceiling falls, or that the round-trip mechanism is established. **One sentence in that
> report is not entitled to any number in it — WARN-A — and it is the sentence most likely to be copied.**

---

## §0 — SCOPE, AND THE RELAY THAT MUST NOT BE DROPPED SILENTLY

🚨 **I FLIPPED NOTHING ON THE BOARD, AND THE FLIPPER IS NAMED: THE MANAGER.**
My `names:` reads *"WRITES: `qa/TASK-1358-report.md` ⛔ ONLY · ⛔ NEVER `TASKBOARD.md`"*, and the row's own
`status:` line carries `SC-§134` cl. 7(b): *"your `names:` does NOT include `TASKBOARD.md` ⇒ you do NOT flip
this line. THE FLIPPER IS NAMED: the MANAGER."* ⇒ **`TASK-1358`'s `status:` still reads `backlog` at my
instant and only the manager may move it to `qa-passed`.** My dispatch said *"plus the flips your `names`
grants"* — **it grants none**; board and law outrank the dispatch (`TL-§5e` cl. 7c). Recorded here so the
relay cannot be lost.

**My only write this run is this file.** No code, no asset, no `CONVENTIONS.md`, no `CLAUDE.md`, no
`*-verify.md`, no other row's line, no git, no compile, no suite, no PIE, **no editor lifecycle action**
(PID 27484 untouched).

⚠️ **I HOLD NO `Bash` TOOL IN THIS INVOCATION** (Read / Grep / Glob / Write / Edit + the read-only
inspector, and nothing else). **Everything below is therefore re-derived structurally or by read-only
inspection, never by shell.** ⛔ **I did NOT route git or a shell through the read-only inspector. Twelve
consecutive agents have declined; I MAKE IT THIRTEEN, and I say so.**

### What I EXECUTED vs what I DERIVED (`SC-§71b` — a permission is not a capability)

**EXECUTED (read-only inspector, live editor, zero mutation):**
- `read_datatable_keys` on `/Game/Data/DT_Cards` ⇒ **34 rows**, names and order returned.
- `read_datatable_values` on `Fireball` / `FrostNova` / `Lightning` / `Fog` / `BrightSun` ⇒ **live**
  `spellDelivery`, `spellEffect`, `cost`, `deckCount` from the asset, not from the CSV source.
- `get_text_file_contents` on `CONVENTIONS.md:4469` (the `SC-§137` header, which `Grep` returned as
  `[Omitted long matching line]`).
- ⚠️ **ONE TOOL FAILED AND I RECORD IT RATHER THAN HIDING IT:** `query_unreal_project_assets` returned
  `SSLError(CERTIFICATE_VERIFY_FAILED)` against `app.tryaura.dev`. **Recorded, not diagnosed.** No finding in
  this report depends on it — I reached `DT_Cards` by direct path instead.

**DERIVED (file reads / `Grep`, no shell):** the source structure behind B4, B5a and B1 · the whole-table
card census · all arithmetic · the law text of `VER-§1` cl. 1/3/3a/5a/6, `SC-§137`, `SC-§132` · the
existence of the three evidence PNGs (`Glob`, **measured at my own instant**, not inherited from a git
snapshot — `SC-§138`).

**NOT DONE:** no PIE, no frame capture, no `.sav` byte read, no process enumeration. Anything I did not
inspect is marked as such below, and this `PASS` is a text-and-structure verdict for those parts.

---

## §1 — 🚨 WARN-A: THE MECHANISM ESCAPES ITS LABEL IN THE HEADLINE. THIS IS THE FINDING.

The report's **Hypotheses** section labels the mechanism correctly, and with unusual rigor:

> **H-A** … *"The disturbance **correlates** with the MCP round trip itself … **Mechanism NOT MEASURED** — I
> have not shown *what* about a round trip moves the OS cursor, and **a correlation is not the claim**."*

But the section titled **"THE B1 ANSWER, IN ONE LINE, FIRST"** — the most-copied position in the whole
document — asserts the same claim flatly, with no hedge:

> *"**The aim point was never jittering. It was being overwritten between round trips.**"*

⇒ **The same proposition is a hypothesis on one page and a fact on another.** A manager drafting the
`VER-§8` cl. 7 ceiling amendment will quote the headline, not H-A. **That is `SC-§101`'s
prescribed-remedy-as-claim in a new place, and this gate says so in advance.**

⛔ **DO NOT CITE.** *"It was being overwritten between round trips"* is **not entitled by any number in
that report.** No instrument in `TASK-1357` observed a round trip doing anything to a cursor.

✅ **THE ENTITLED SENTENCE — safe to build a ceiling amendment on, and it is the whole of what was measured:**

> *The same `SetMouseLocation(300,420)` repeated inside one batched `run_verification_sequence` reproduces
> the same world point to **≤ 1.2026 uu** (**exactly 0.000000 uu** within a fixed read-mode), at a camera
> proven byte-identical before and after, against the **≈648 uu** spread `TASK-1352` measured when the same
> repeats were separated by MCP round trips. **WHY batching removes the scatter is NOT MEASURED.**

⭐ **AND A CREDIT THE REPORT DID NOT CLAIM FOR ITSELF — ONE COMPETING EXPLANATION *IS* MEASURED OUT.**
The obvious confound is not round trips but **elapsed PIE time**: `TASK-1352`'s 648 uu pair sat **3.08 s**
apart (`t=11.23 → 14.31`) while the batched reads sit ~0.08 s apart. **B2's own numbers break that
confound** — inside a batch the point held **bit-identical across 4.04 s** with no re-set
(`t=94.07 → 96.09 → 98.12`), a **longer** interval than the 3.08 s that scattered 648 uu across round trips.
⇒ **elapsed time alone does not explain the scatter.** H-A is materially better supported than its author
claimed. ⛔ **It is NOT promoted: excluding one alternative is not establishing a mechanism.** *What* a round
trip does to the OS cursor remains unmeasured, and **H-A stays a hypothesis.**

**Why this is WARN and not BLOCKER, stated so the grade can be audited:** every *number* in the report is
entitled to its sentence (spec (0)'s single question); the unentitled item is a prose clause carrying no
number, and **the report contains its own correction** in H-A. A `FAIL` would bounce a row with **no code
and no defective measurement** — the exact error `VER-§1` cl. 3a's teeth paragraph exists to prevent. The
effective remedy is this scoping box, delivered to the lane that writes the amendment.

---

## §2 — CHECK ONE: DID THE CONTROLS FIRE, AND COULD THEY HAVE FAILED?

| line | control named | same session? | taken before treatment? | **could it have failed?** |
|---|---|---|---|---|
| **B1** | set-omitted limb, **same batch** (`t=50.72/50.79/50.86`) vs treatment (`t=50.94…`) | ✅ A | ✅ `50.72 < 50.94` | ✅ **YES — and there is a RECORDED INSTANCE OF IT DOING SO** |
| **B2** | the `bShowMouseCursor: false` arm (`t=25.57–25.74`, and the −45 hold `t=93.96–98.12`) | ✅ B (session B = `t=20.72→141.84`) | n/a — paired arms | ✅ yes: if the flag mattered, the `false` arm scatters |
| **B3** | same-session pitch-0 triple (`t=25.57/25.66/25.74`) vs pitch −45 (`t=26.16/26.24/26.33`) | ✅ **B — NOT cross-session** | ✅ | ✅ yes — and the probe **demonstrably fired** (§4) |
| **B4** | the refusal log line **FIRING at least once** | — | — | ⛔ **IT DID NOT FIRE** (`total_matches=0`) ⇒ scored `unobs`, §3 |
| **B5a** | set-omitted limb, same batch (`t=11.83/11.90/11.96`) vs treatment (`t=12.03/12.11/12.21`) | ✅ C | ✅ `11.83 < 12.03` | ✅ yes |
| **B5b** | — | — | — | no claim made ⇒ none owed |

🚨 **B1's CONTROL COULD GENUINELY HAVE FAILED, AND I CAN POINT AT THE OCCASION.**
The control limb's exact condition — *no `SetMouseLocation` issued yet in this session* — is recorded
returning a **live hit** rather than the dead baseline: `TASK-1352` §5, session B's very first ghost read,
**before any set that session**, read `bHidden: false` at `(X=-23071.000023, Y=1265.141468, Z=1414.891231)`.
⇒ the set-omitted limb is **not structurally pinned to `(0,0,0)`**; it reads whatever the OS cursor happens
to project to, and on at least one recorded occasion that was a valid ground hit.

**Structurally corroborated at source, by me:** `ASiegePlayerController::UpdatePlacementGhost`
(`SiegePlayerController.cpp:3082`) calls `TraceCursorToGround(Hit)` at `:3085` and — the function's own
comment at `:3107` — *"already writes the ghost's visibility and location every frame."* ⇒ **B1's observable
is a live per-frame readout, not a latched value**, and `bHidden: true` is an affirmative statement that the
trace missed this frame. **The green check could have been red. That is what this row exists to establish.**

⚠️ **WARN-D — A CORRECTION TO MY OWN DISPATCH, RECORDED SO IT CANNOT HARDEN.** My dispatch states the
set-omitted control *"returned `bHidden: true` at `(0,0,0)` ×3, **in all three sessions**."* **The report
does not claim that and the quoted values do not support it.** The dead-baseline control appears in **TWO**
sessions — **A** (B1) and **C** (B5a). **Session B carries no set-omitted baseline**, and correctly needs
none: its claim-making lines (B2, B3) carry controls appropriate to *their own* questions (the `false` arm;
the same-session pitch-0 triple). **Every claim-making line has a firing control — but "all three sessions"
is a sentence nobody measured. Do not write it.**

---

## §3 — CHECK TWO: IS ANY NEGATIVE INCOMPLETE? (THE `TASK-1349` WARN-B CHECK — THE REASON THIS ROW EXISTS)

### 🚨 B4 — `unobs`, AND IT DID **NOT** READ AN ABSENCE AS A PASS. CONFIRMED, AND STRONGER THAN CLAIMED.

**(a) The structural ground — re-derived by me, and it is decisive.**
I censused the entire `Source` tree: **`DeprojectMousePositionToWorld` appears EXACTLY ONCE in the whole
project**, at `SiegePlayerController.cpp:3722`:

```cpp
3697    if (!bTargetingSurfaceValid)          // ← ASiegePlayerController::TryConfirmSpellTarget(), :3688
...
3722        if (!IsValid(TargetingHero) || !DeprojectMousePositionToWorld(RayOrigin, RayDirection))
3723        {
3725            TEXT("ASiegePlayerController '%s': line-spell confirm refused for '%s' — no hero/deproject …")
```

⇒ the quoted refusal sits inside `if (!bTargetingSurfaceValid)` and the deproject is called **only** in that
branch, **with no alternate call path anywhere in the project.** Under mechanism (i)'s success case the
cursor **is** on a surface ⇒ `bTargetingSurfaceValid` is **true** ⇒ **the branch is unreachable and the
deproject never runs.** ⇒ **an absence of that log line would have proven only that we were not in the
no-surface branch — precisely the false pass the row warned of.** **The row's refusal is correct.**
⭐ It is **stronger** than the report stated: the report said the deproject is called *only in that branch*;
I measured that it is called **only there, full stop.**

**(b) The card census — I re-ran it on a STRICTLY BROADER criterion and the negative is COMPLETE.**
The report enumerated **authored** `spellDelivery: "HeroLine"` cells ⇒ `{Fireball, FrostNova}`. But
`CardRow.h:176` / `:389` say an **`Auto`** cell resolves *per-effect: **AoEDamage/Freeze → HeroLine***. ⇒ the
truly reachable set is `authored HeroLine` ∪ `Auto + (AoEDamage | Freeze)`, **larger than the criterion the
report stated.** I ran the resolving criterion over **all 34 rows**: `AoEDamage|Freeze` matches **exactly two
rows in the entire table** — `Fireball` (cost 21) and `FrostNova` (cost 18) — **the same two.**
Confirmed on the **LIVE ASSET**, not merely the CSV source (`read_datatable_values`, my instant):

| card | `spellDelivery` (live) | `spellEffect` (live) | resolves to a line spell? |
|---|---|---|---|
| Fireball | **`HeroLine`** | `AoEDamage` | **yes** — cost 21 ✓ |
| FrostNova | **`HeroLine`** | `Freeze` | **yes** — cost 18 ✓ |
| Lightning | `Auto` | `TopTargetsDamage` | **no** |
| Fog | `Auto` | `FogCover` | **no** |
| BrightSun | `Auto` | `FogClear` | **no** |

⇒ **COMPLETE, not merely unenumerated** — the distinction `TASK-1349` WARN-B was bought with. The report's
stated criterion was narrower than the rule; **the answer is unchanged**, which is why this is a NIT
(NIT-1) and not a blocker.

**(c) ⭐ THE INSTRUMENT CONTROL — AND IT IS `SC-§137`'s EXACT SHAPE, EXECUTED.** The report's first-pass
`>=4`-character filter **missed `Fog`** — a card **demonstrably drawn in session B's hand** — and the
raw-byte search **found** it. **A pattern whose meaning depends on the filter's dialect is an uncontrolled
instrument** (`SC-§137`, minted 2026-09-20 on `TASK-1343`'s own two malformed probes). This row **caught,
re-ran and published its own** — same discipline, same day, unprompted. **Full credit.**

⚠️ **WARN-C — the one negative in this report I could NOT complete, and a contradiction a reader will hit.**
The `.sav` census names only **5 of 34** card names (`Fireball` F, `FrostNova` F, `Footman`/`BrightSun`/`Fog`
T). The sentence *"the deck's only spells are `BrightSun` and `Fog`"* — which is **B5b's stated reason** — is
therefore **broader than the census shown**. I hold no way to enumerate the remaining 29 (no `Bash`; I
declined to route a byte read for it). ⇒ **Scope B5b's reason to what was measured:** *"`Fireball` and
`FrostNova` are measured ABSENT from the active deck; the deck's full spell inventory was not enumerated."*
**B5b's token (`unobs`) is unaffected** — nothing was probed either way — and **B4 is unaffected**, because
`Fireball`/`FrostNova` were measured directly and (a) is independently decisive.

⚠️ **AND PRE-EMPT THIS, BECAUSE SOMEONE WILL "CORRECT" B4 FROM THE WRONG TABLE:** `DT_Cards`' own
`deckCount` column reads **Fireball 2 · FrostNova 1 · Fog 0 · BrightSun 0** — the **OPPOSITE** of the `.sav`
census. **The `.sav` is authoritative and the report used the right instrument**: `Fog` was *demonstrably
drawn in session B* despite `deckCount = 0`, which measures the override directly. ⇒ **the naive instrument
(`DT_Cards.deckCount`) would have returned "a line spell IS in the deck" and been wrong.** The report went
to the active profile deck save instead. **Correct, and non-obvious.**

---

## §4 — CHECK THREE: THE SPREAD ARITHMETIC, RE-COMPUTED BY ME FROM THE QUOTED COORDINATES

**B1, five treatment reads, C(5,2) = 10 pairs:**

| mode | reads | coordinate | pairs | distance |
|---|---|---|---|---|
| set → wait 0.05 s → read | 3 (`t=50.9412/51.0246/51.1079`) | `(-21382.831129, 407.684206, 29.000000)` | 3 | **0.000000 uu** |
| set → read, no wait | 2 (`t=51.1412/51.1746`) | `(-21382.831129, 408.886796, 29.000000)` | 1 | **0.000000 uu** |
| cross-mode | — | ΔX = 0 · **ΔY = +1.202590** · ΔZ = 0 | 6 | **1.202590 uu** (= \|ΔY\|, exactly) |

⇒ **max pairwise across all five = `1.2026` uu** ✓ **the report's arithmetic re-computes exactly, and it
shows its work** (spec (3) satisfied — this is not an asserted spread).
⇒ against `TASK-1352` §4's **≈648 uu** un-batched baseline that is a **≈539×** reduction on the
**conservative** number, and unbounded on the within-mode number.

⚠️ **WARN-B — CITE THE ENVELOPE, NOT THE HEADLINE.** *"Spread = `0.000000` uu"* is true **within a fixed
read-mode**; the number that survives **every** read this run is **≤ 1.2026 uu**. The report discloses both
in the same cell — it did not hide it — but a downstream citation that drops the 1.2026 over-claims.
**Recommend the manager write `≤1.2026 uu across all five treatment reads (0.000000 within a fixed
read-mode)`: unattackable, and the conclusion is identical either way.** (Note the 1.2026 is a
**between-condition** difference — wait vs no-wait — not within-condition scatter; the report is right to
separate them, and right to publish the larger number anyway.)

**B3, re-derived independently:**
- displacement pitch 0 → pitch −45: `Δ = (+339.282994, −147.471471, −29.000000)` ⇒ **‖Δ‖ = 371.08 uu** ✓
- camera→hit distance: pitch 0 ⇒ **≈878.5 uu**; pitch −45 ⇒ **≈560.7 uu** ✓ **it moved NEARER** — the
  direction a real downward projection gives.
- ⭐ **A CONSISTENCY PROOF THE REPORT DID NOT CLAIM, AND IT CONFIRMS THE PROBE FIRED A SECOND WAY:** the
  quoted camera positions sit at a **fixed 400.0 uu boom** from an unchanged pawn at **both** pitches —
  pitch 0 offset `(+400, 0, 0)` ⇒ 400.0; pitch −45 offset `(+282.842, 0, +282.843)` ⇒ **‖·‖ = 400.0**, and
  `400·cos45° = 282.843`. ⇒ **the camera rotated exactly 45° about an unmoved pawn on an unmoved boom**, and
  the quoted camera numbers are internally consistent to six decimals. **B3's probe demonstrably reached the
  camera rig.**

⛔ **"MOOT" IS NOT "REFUTED", AND THE REPORT SAYS SO CORRECTLY — KEEP IT THAT WAY.** `TASK-1352`'s **H2**
(grazing-ray jitter) is **neither confirmed nor refuted**: the spread was **already exactly `0.000000` at
pitch 0**, so *there was no scatter left for a downward pitch to compress.* **Sub-pixel jitter below this
instrument's floor is UNEXCLUDED.** ⛔ **Anyone who records B3 as "H2 refuted" has written a fact the data
does not contain.** The report's own wording — *"neither confirmed nor refuted; it is MOOT"* — is the
correct one and must survive downstream verbatim.

**B2 — `TASK-1352`'s H1 IS genuinely REFUTED** (the stronger word, and here it is earned): the point held
**bit-identical across 4.04 s with the flag `false`** and across the flag write **and its revert**. H1 said
the software cursor being off is *why* the point does not persist. It persisted with the flag off. ⇒
refuted **as the explanation for non-persistence**, which is exactly how the report scopes it.

---

## §5 — CHECK FOUR: CELL TOKENS AGAINST `VER-§1` cl. 3a

| line | last cell | first word is a legal token? | correct kind? |
|---|---|---|---|
| B1 | `pass` | ✅ | observable **moved**, control dead ⇒ `pass` ✓ |
| B2 | `measured` | ✅ | **controlled negative**: probe fired (3 read-backs, both directions), observable did not move ✓ |
| B3 | `measured` | ✅ | **controlled negative**: probe fired (371.08 uu, 45° boom), the *spread* did not move ✓ |
| B4 | `unobs` | ✅ | **control did NOT fire** ⇒ `unobs`, **never `measured`** ✓ |
| B5a | `pass` | ✅ | observable moved, control dead ✓ |
| B5b | `unobs` | ✅ | never probed ✓ |

- **All six first words ARE the token** (cl. 3a's NIT-1 limb: trailing prose permitted, the first word must
  be the token). ✓
- ⛔ **No controlled negative is mis-scored `fail`** ⇒ no mechanical `VERIFY-FAILED` derivation against a row
  with **no code under test**. ✓
- 🚨 **THE ANTI-ABUSE LIMB, APPLIED IN BOTH DIRECTIONS — THIS IS THE MOST IMPORTANT TOKEN CALL IN THE TABLE
  AND IT IS RIGHT.** cl. 3a: *"A negative WITHOUT a discriminating control is `unobs`, NEVER `measured` —
  `measured` is EARNED BY THE CONTROL."* **B2 and B3 earned it** (their controls are named and fired).
  **B4 did NOT earn it and was not given it** — its control returned `total_matches=0`, so it is `unobs`.
  ⇒ **the one cell where a softer token would have bought an unearned conclusion is the one cell scored
  `unobs`.**
- **Derivation:** no `fail`, ≥1 `pass` ⇒ cl. 5a **branch (2)** ⇒ **`VERIFIED`**. ✓ The `pass`+`measured`
  mixture is the case cl. 5a rules explicitly, and it is ruled the way the report took it. Line 1 is
  byte-literal `Verdict: VERIFIED` with **nothing preceding it** (cl. 1) ✓ — `head -1` IS the verdict.
- **`measured` cells listed under *Not examined / limitations* with their controls**, exactly as cl. 5a
  directs ✓.
- **Coverage `verify: partial (4/6 observable)`** = the four non-`unobs` lines of six ✓, and `VER-§5` cl. 4
  is respected (partiality is **coverage**, so the row is neither `UNOBSERVABLE` nor `MEASURED`).
- **Attempts 3/3, all three kept and named** `-a1`/`-a2`/`-a3`, one frame per session (cl. 6) ✓ — **all
  three PNGs exist on disk at my own instant**, paths byte-exact, no slug contains "pass" or "fail".

---

## §6 — CHECK FIVE: B2's REVERT — SHOWN BY READ-BACK, NOT ASSERTED. NOTHING LEFT LIVE.

⚠️ **A DISPATCH/BOARD DIVERGENCE, RESOLVED IN THE BOARD'S FAVOUR.** My dispatch described spec (5) as
*"which row carried the four-word edit"*. **The board's spec (5) is the `bShowMouseCursor` REVERT check.**
Board outranks dispatch (`TL-§5e` cl. 7c) ⇒ **I ran the revert check.** (The four-word item is answered as a
relayed account at §7.)

1. **Found at `true`** — inside placement mode, 0.35 s after entry. ⇒ **the specced treatment write was a
   NO-OP**, disclosed in the tool's own words: `{"previous_value":"true","applied_value":"true"}`.
   ⭐ **The row disclosed this rather than presenting the no-op as its treatment**, and re-measured a
   **boarded premise** (`SC-§91`) instead of inheriting it: `bShowMouseCursor` reads `false` at rest **only
   OUTSIDE placement**; placement mode itself raises it via `ApplyCursorInputState`. **The real
   discriminating pair came from the REVERT, and the report says so plainly.** That is the opposite of the
   failure mode this gate exists for.
2. **Write proven LIVE in both directions** — `true`→read `true` · `true`→`false`→read `false` ·
   `false`→`true`→read `true`. ⇒ a null here would have been a real null.
3. ✅ **REVERTED TO THE VALUE IT WAS FOUND AT, AND READ BACK:** restored to **`true` at `t=136.4924`**,
   **read back `true` at `t=136.5091`**, before the session ended. **Shown, not asserted.** ✓
4. ✅ **NOTHING REACHED DISK:** 5/5 `.sav` files identical by **sha256 AND mtime**, `SAV_COUNT` 5→5 ·
   `L_Arena.umap` hash `1f78419d…15622`, mtime and 605098 B unchanged · **`DIRTY_PACKAGES = NONE`** · no save
   prompt raised or answered.
⇒ **No probe write is left live in the editor. The finding spec (5) was hunting does not exist here.**
5. ⛔ The row **refuses to propose shipping `bShowMouseCursor: true`** by name, and this gate concurs: a
   visible cursor in a player's match is a change nobody asked for.

---

## §7 — ⛔ TWO THINGS THIS GATE DOES **NOT** SAY

### 1. 🚨 THE CONFIRM IS UNTESTED. NOBODY MAY CITE THIS ROW — OR THIS GATE — AGAINST `VER-§8` cl. 7 WALL (a).

**Re-derived structurally at my own instant, not accepted as declared:**
- `TryConfirmSpellTarget()` is declared at **`SiegePlayerController.h:2502`** as a **plain member function —
  there is no `UFUNCTION` macro on it** (line 2501 is the closing `*/` of a doc comment; 2502 is the bare
  declaration). ⇒ **it is not reflected ⇒ `call_actor_function` cannot reach it.**
- Its **only** entry is the raw poll: `if (WasInputKeyJustPressed(EKeys::LeftMouseButton))` at
  **`SiegePlayerController.cpp:847`** → `TryConfirmSpellTarget()` at **`:849`**; the group-pick twin sits at
  **`:921`**. Exactly the sites `VER-§8` cl. 7 wall (a) names.
- **`TASK-1357` pressed no mouse button, confirmed no placement and spent no card.** ⇒ **wall (a) is
  UNTOUCHED.**

⛔ ***A REPRODUCIBLE AIM POINT IS NOT A PLAYED CARD.*** This gate passes the **aim** measurement and says
**nothing whatever** about the **confirm**.

### 2. ⛔ THE CEILING SENTENCE IS THE MANAGER'S, AND I DO NOT MINT IT EITHER.

`SC-§82` / `SC-§101`. `TASK-1357` wrote no law and proposed no code — **I confirm that: its Hypotheses
section explicitly declines to cost mechanism (ii) or (B), and its narrowing blockquote reserves the
amendment to the manager.** **This report mints nothing.** Everything above is **endorsement and scoping**.
Any law-shaped text here is **flagged, not written** — the `TASK-1349` §7 shape, which the board itself
names as the model (spec (7)).

**Flagged for the manager, NOT written by me:** if a `VER-§8` cl. 7 amendment is drafted, the gate's view is
that it may rest on **§1's entitled sentence** and must not rest on the headline's mechanism clause; and
that the `measured`-vs-`unobs` discipline demonstrated at B4 (§5) is the shape worth generalising. **Those
are recommendations to the lane that rules, not rulings.**

---

## §8 — GRADING NOTE (`SC-§132` IN FORCE, AS SPEC (6) DIRECTS)

`SC-§132`: *a guard that answers GREEN when it COULD NOT LOOK outranks one that answers RED about the wrong
thing — a **fail-silent is always the higher grade**.* Applied here: **the one place `TASK-1357` could have
gone fail-silent is B4** — an absent log line, in a branch that cannot execute, is the textbook green that
could not look. **It scored `unobs` and said so in terms.** ⇒ **the highest-consequence grade available to
this row was taken correctly, by its own author, before I arrived.** Nothing in this report is a fail-silent
by my measure.

---

## Findings

- **[WARN-A]** `qa/TASK-1357-verify.md` §"THE B1 ANSWER, IN ONE LINE, FIRST" — *"It was being overwritten
  between round trips"* is stated **as fact** in the most-copied position, while H-A correctly labels the
  same claim **"Mechanism NOT MEASURED"**. ⇒ **the label does not survive into the headline.**
  **Fix (for the consuming row, not for this report):** cite **§1's entitled sentence**; ⛔ **never** the
  headline clause. **The measurement stands alone and does not need the mechanism.**
- **[WARN-B]** ibid., B1 cell — *"spread = `0.000000` uu"* is within-a-fixed-read-mode; the all-reads
  envelope is **≤1.2026 uu**. Both are disclosed in the cell, but a citation that drops 1.2026 over-claims.
  **Fix:** cite `≤1.2026 uu (0.000000 within a fixed read-mode)`.
- **[WARN-C]** ibid., B5b / B4(1) — *"the deck's only spells are `BrightSun` and `Fog`"* rests on a **5-name**
  `.sav` census, not a 34-name one; and `DT_Cards.deckCount` reads the **opposite** for four of five cards.
  **Fix:** scope to *"`Fireball` and `FrostNova` are measured ABSENT; the full inventory was not
  enumerated"*, and state that the **`.sav` overrides the table** (proven by `Fog` being drawn at
  `deckCount 0`). **No token changes; B4's decisive ground is structural.**
- **[WARN-D]** *dispatch text, not the report* — *"the control … returned the dead baseline **in all three
  sessions**"* is **unsupported**: it fired in **two** (A, C). The report never claims three. ⛔ Do not carry
  that sentence into the amendment.
- **[NIT-1]** ibid., B4(1) — the card criterion should be the **resolving** rule (`authored HeroLine` ∪
  `Auto`+`AoEDamage`/`Freeze`, per `CardRow.h:176`/`:389`), not the authored cell. **I re-ran it on the
  broader rule over all 34 rows and on the live asset: the answer is identical.** Wording only.
- **[NIT-2]** VRAM banner provenance — my dispatch cites *"an earlier one read 8.242 MB"*; **the earlier
  figure in the two reports I read is `172.082 MB`** (`TASK-1352`, both frames). I record what I measured.

## Notes for build-master / manager (PASS)

- ⛔ **NOTHING TO COMPILE, COMMIT OR VERIFY FROM THIS ROW.** `TASK-1358` is a read-only gate: **no code
  diff, no asset, no board flip by me.** The only artefact is this file.
- 🚨 **THE BOARD FLIP IS THE MANAGER'S** (`SC-§134` cl. 7(b), quoted on the row's own `status:` line).
  `TASK-1358` still reads **`backlog`** at my instant. **If the manager does not flip it, no one will.**
- ⚠️ **VRAM BANNER — SECOND SIGHTING, AND IT IS GROWING. FOR THE MANAGER TO BOARD; ⛔ I DO NOT DIAGNOSE IT
  AND NEITHER REPORT DID (both correctly recorded it).** Same day, same map, same editor PID 27484:
  `TASK-1352` **`172.082 MB` over budget** (both frames) → `TASK-1357` **`382.789 MB` over budget** (frame C;
  **absent on frame A**, so it is intermittent within a run). **≈2.2× growth across two runs.** ⇒ this is now
  a **pattern across runs, not an incident**. A row that wants it diagnosed should be boarded; ⛔ nothing in
  either verify report is invalidated by it, and no finding above depends on a frame that carries it.
- ⭐ **CREDIT — THE BLUE ARC.** The report **declines to assert** that the blue arc in frame a3 is the pick
  circle's rim, because it **took no `bHidden` read on the decal after the sets**. That restraint is correct
  and I confirm why it matters: `UpdateGroupPickReticle` drives the decal's visibility off `bSurfaceHit`
  **every frame** (`:4105`), so the decal's state at capture time is genuinely unmeasured by a pixel.
  ***A pixel that merely looks right is not a measurement*** — and the **position** claim is carried by the
  property reads, which is where it belongs.
- ⭐ **CREDIT — THE RIGHT INSTRUMENT AT B5a.** `GroupPickLocation` (`h:3428`) and `bGroupPickSurfaceValid`
  (`h:3425`) are **plain C++ members with no `UPROPERTY`** — I confirmed this in the header. The tool's
  *"Property not found"* is therefore **a fact about the code, not a broken instrument**, and the
  substitution (the decal actor's transform, which `:4108` writes **from** `GroupPickLocation`) is a
  faithful, verifiable proxy. **Named, not worked around.**
- ⭐ **B5a IS A GENUINELY INDEPENDENT CALLER**, confirmed at source: `UpdateGroupPickReticle` calls the
  **same** `TraceCursorToGround` (`:4094`), from a mode entered by `IA_CmdFollow` (an **input action**, so
  the deck cannot block it), in a **different PIE session** — and lands on the same world point to six
  decimals. **Two callers, three sessions, two independent runs, one value.**
- **No conventions or naming violations found.** No deprecated UE API, no unsafe pointer use, no
  reflection defect — **there is no diff in this row to carry one.** The source sites I read
  (`UpdatePlacementGhost`, `UpdateGroupPickReticle`, `TryConfirmSpellTarget`) were read **as the report's
  subject matter**, not as code under review, and I propose no change to any of them.
