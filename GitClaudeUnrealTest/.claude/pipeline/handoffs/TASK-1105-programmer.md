# TASK-1105 — [VEIL-SWING] the swing that played `AM_ComboAttack` and dealt no damage to a veiled unit at 115 uu

**Role:** gameplay-programmer · **Date:** 2026-09-07 · **Marker:** `TASK-1105-VEIL-SWING`
**Mode:** DIAGNOSE-ONLY, READ-ONLY. ⛔ **Zero edits to `Source/**`, `Tests/**`, `Content/**`. No compile, no editor, no MCP, no git.**
**Only write:** this file.
**Law:** `FIELD-§1` (report the answer he will not like) · `SC-§90` (a cause is a claim about reachability) · `SC-§91` · `SC-§94` · `WITCH-§2` · `WITCH-§9.1` row 4 · `J-W18`.

---

## 1. ⭐⭐ THE VERDICT — AND IT IS **TWO** VERDICTS, BECAUSE THE ROW ASKS **TWO** QUESTIONS

| # | question | verdict |
|---|---|---|
| **A** | ⭐ **Is `SuppressVeiled` the intended policy for a PLAYER melee swing?** | ✅ **CORRECT BY DESIGN — no defect in the combat lane.** The hero's swing *is* an acquisition, `WITCH-§2` lane 1 suppresses acquisition, and the policy did **not** "leak from AI onto the player's sword": the hero is **site 8 of 9** in a funnel that was built as one chokepoint on purpose (`WITCH-§1`), and the bot's own half was added **later and separately** (`TASK-851`). ⛔ **The hero was never the odd one out.** |
| **B** | ⭐ **THE PLAYER-FACING DECIDER — could the player SEE the unit while the swing missed?** | ⛔ **YES, HE COULD — so the player-facing outcome IS wrong.** ⛔ **BUT THE DEFECT IS NOT IN THE SWING. IT IS THE ALREADY-BOARDED RENDER SHORTFALL `J-W18` / ⭐ `TASK-931`, AND FIXING IT IN THE COMBAT LANE WOULD DELETE THE CARD.** |
| **C** | ⭐ **Was the observed 115 uu incident the veil?** | ⛔ **CANNOT TELL WITHOUT THE SESSION — AND THE VEIL IS THE *LEAST* LIKELY OF FOUR CANDIDATES.** ⛔ **It is UNREACHABLE without a Witch, and the session records none.** ⇒ ⛔ **`TASK-1094`'s stated reading is a PLAUSIBLE cause, not a MEASURED one (`SC-§90`), and three cheaper candidates reproduce the observed signature exactly.** |

### The one-sentence answer to the decider, with the source path that decides it

> ⛔ **A veiled unit today is visible to the enemy player as a shimmering distortion with a silhouette — because `ASummonedUnit::ApplyVeilMaterial` (`Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.cpp:3149`) paints `MI_Unit_Invisible` with a bare `ActiveMesh->SetMaterial(SlotIndex, ResolvedVeil)` and ⛔ NO owner / `IsLocallyControlled` / `IsAgentVisibleTo` gate whatsoever — so yes, the player can see what his sword refuses to hit, and by this row's own decider that is a defect; ⛔ it is however ⛔ ENTIRELY a defect of the PAINT, not of the SWING, and it is already boarded as ⭐ `TASK-931` under 🧑 `J-W18`.**

⭐⭐ **THE LOAD-BEARING CONSEQUENCE, STATED FIRST BECAUSE IT IS THE WHOLE POINT OF THE ROW: `TASK-931` (per-viewer suppression) makes the symptom disappear WITHOUT ONE LINE CHANGING IN `DoMeleeAttack` OR IN `ESiegeVeilPolicy`.** ⛔ When the enemy player can no longer *see* a veiled unit, *"I swung at a visible enemy and nothing happened"* ⛔ ceases to exist as an experience. ⇒ ⛔ **The correct fix for the player-facing complaint is the render row that already owns it. A veil-policy change is the wrong repair for this symptom and it is strictly worse than doing nothing** (see §6).

---

## 2. THE CALL PATH, `file:line`, MEASURED BY OPENING EVERY FRAME

```
AHeroCharacter::DoMeleeAttack()                       HeroCharacter.cpp:479
  ├─ refusal guard (dead / suppressed / recall / climb)          :507   ← returns BEFORE the montage
  ├─ cooldown gate (MeleeCooldown 0.5 s)                         :522
  ├─ ⭐ PlayAnimMontage(AttackMontage, …)                        :530   ← ⛔ THE MONTAGE PLAYS HERE
  ├─ authority gate  (client returns, montage already played)    :551
  ├─ FSiegeVisionQuery::SeeingFrom(MyLocation, MeleeRange)       :582
  ├─ ⭐ FSiegeCombatStatics::GatherHostileAgents(…, SuppressVeiled, &Vision)   :585
  │     ├─ GatherTeamAgentsFiltered  (the ONE enumeration + team filter)  SiegeCombatStatics.cpp:33
  │     │     └─ IsHostileTeam(ViewerTeam, Agent->GetTeamId()) != bWantHostile  :70   ← ⛔ TEAM CUT
  │     ├─ the FOG cut  (runs only if EffectiveRadius < Requested)         :288
  │     └─ ⭐⭐ THE VEIL CUT — Out.RemoveAll(!IsAgentVisibleTo(...))        :369   ← ⛔ THE STAGE
  │           └─ FSiegeCombatStatics::IsAgentVisibleTo                     :79
  │                 └─ FSiegeInvisibilityStatics::IsVisibleTo(Viewer,Target,bInvisible)  :117 → SiegeInvisibilityStatics.cpp:16
  ├─ per-target RANGE test   (ActorGetDistanceToCollision > MeleeRange)  HeroCharacter.cpp:621
  ├─ per-target CONE test    (Dot(Facing, ToTarget) < cos 30°)           HeroCharacter.cpp:632   ← ⛔ ±30° ONLY
  └─ UGameplayStatics::ApplyDamage(Target, …)                            HeroCharacter.cpp:641
        └─ ASummonedUnit::TakeDamage                                      SummonedUnit.cpp:5396
              ├─ if (bDead || DamageAmount <= 0.f) return 0.f;            :5398   ← ⛔ CORPSE VETO
              └─ friendly-fire veto: AttackerTeam == Team → return 0.f    :5407   ← ⛔ TEAM VETO
```

### ⭐ THE FAILING STAGE, NAMED: **TARGET ACQUISITION, `Source/GitClaudeUnrealTest/Siegebound/SiegeCombatStatics.cpp:369`**

⛔ **IF the target had been veiled, the swing fails at ACQUISITION and at nothing else.** The veiled actor is removed from `Out` by the `RemoveAll` at `SiegeCombatStatics.cpp:369` — **before** the range test, **before** the cone test, **before** `ApplyDamage`. It is a **candidate-set** cut, not a damage-time veto.

⛔⛔ **AND THE OTHER HALF, MEASURED RATHER THAN ASSUMED: THERE IS ⛔ NO DAMAGE-TIME VEIL VETO ANYWHERE.** `ASummonedUnit::TakeDamage` (`SummonedUnit.cpp:5396`) reads `bDead`, the damage amount and the attacker's team — ⛔ **it never reads `bIsInvisible`, and its own trap comment says so out loud** (a veiled unit clipped by a blast takes full damage and does *not* un-veil — `WITCH-§3`'s "being hit is not acting"). ⇒ ⭐ **a veiled unit is HIDDEN, never INVULNERABLE — the suppression is 100% at the gather.**

---

## 3. HERO vs UNIT — **THE SAME RULE, AND IT IS THE SAME LINE OF CODE**

⛔ **Not hero-only. Not two rules. ONE.** Every acquisition in the game routes through the single `RemoveAll` at `SiegeCombatStatics.cpp:369`, and the bot's own perception (shipped separately at `TASK-851`) calls the *same* predicate directly rather than re-implementing it:

| consumer | site | policy |
|---|---|---|
| ⭐ **hero melee swing** | `HeroCharacter.cpp:585` | `SuppressVeiled` |
| **unit acquisition** (×2) | `SummonedUnit.cpp:1850` · `SummonedUnit.cpp:2477` | `SuppressVeiled` |
| **tower acquisition** (×2) | `Tower.cpp:238` · `Tower.cpp:423` | `SuppressVeiled` |
| **bot threat read** (×3, `TASK-851`) | `SiegeBotController.cpp:1001` · `:1089` · `:1186` | direct `IsAgentVisibleTo` — ⛔ the **same** predicate, ⛔ not a second implementation |
| ⛔ **blast** (`ApplyRadialDamage`) | `SiegeCombatStatics.cpp` blast lane | ⛔ `IncludeVeiled` — a blast is **not** an act of seeing (`WITCH-§2`, `J-W2`) |
| ⛔ **friendly acquisition** | `GatherFriendlyAgents`, `SiegeCombatStatics.cpp:373` | ⛔ **no policy parameter exists** — the owner's own lane can never be suppressed (`WITCH-§2` lane 4) |

⇒ ⭐ **A veiled unit is equally unhittable-by-acquisition to an enemy Footman, an enemy Tower, the bot and the hero. The asymmetry the player experiences is not in the CODE — it is that only the HUMAN has eyes, and the eyes are being lied to by `§1`'s paint.**

---

## 4. ⛔⛔ `SC-§90` — THE VEIL HYPOTHESIS IS **UNREACHABLE** IN THE `TASK-1094` SESSION AS RECORDED

`TASK-1094`'s reading (*"a veiled/fog-hidden target is legitimately skipped"*) is **a claim about reachability**, so I measured the reachability instead of weighing the plausibility.

**(a) ⛔ THE VEIL NEEDS A WITCH, AND THERE IS EXACTLY ONE DOOR.**
`bIsInvisible` is written `true` at **one** site in the project — `FSiegeInvisibilityStatics::ApplyVeil` (`SiegeInvisibilityStatics.cpp:47`) — reached from **one** method, `ASummonedUnit::GrantInvisibility` (`SummonedUnit.cpp:3084`), which has **exactly one caller in the whole tree**: `ASummonedUnit::CompleteWitchCast` (`SummonedUnit.cpp:3597`, and the code says so in a banner comment). That path requires a **live Witch unit on the field completing a channelled cast on an eligible friendly subject inside her position circle** (`bCasterFit && bSubjectFit`, `:3569`/`:3577`).
⇒ ⛔ **Without a Witch, `bIsInvisible` is `false` for every unit in the level and the `RemoveAll` at `:369` removes NOTHING.** `TASK-1094`'s handoff names only `BP_Unit_Footman_C` and `BP_Unit_MilitiaMob_C`; it records **no Witch, no cast, and no shimmer** — and its four captures (A–D) would have shown a translucent/refractive body if one had been veiled.

**(b) ⛔ THE FOG CANNOT DO IT EITHER — THE ARITHMETIC IS INERT AT MELEE REACH.**
The fog cut at `SiegeCombatStatics.cpp:288` runs **only** when `EffectiveRadiusUU < Vision->RequestedRadiusUU`. `MeleeRange = 150` (`HeroCharacter.h:1212`) and the ceiling is `609.6`; `min(150, 609.6) == 150` bit-identically ⇒ ⛔ **the loop never executes on a melee gather, in either fog state.** *"fog-hidden"* is not available as a cause here at all.

**(c) ⭐ THREE CANDIDATES THAT ARE *CHEAPER* AND EACH REPRODUCE THE EXACT SIGNATURE (montage plays, HP unchanged).**
⛔ Note what makes them indistinguishable from the outside: **the montage plays at `HeroCharacter.cpp:530`, BEFORE the gather, before the range test, before the cone test and before `ApplyDamage`.** ⇒ ⛔ *"the montage played and nothing happened"* is the signature of **every** miss in this function, and it discriminates ⛔ nothing.

| # | candidate | the measurement that makes it live | what `TASK-1094` did **not** record |
|---|---|---|---|
| **1** ⭐⭐ | ⛔ **THE ±30° CONE** — `MeleeHalfAngleDegrees = 30.f` (`HeroCharacter.h:1216`), tested at `HeroCharacter.cpp:632` | ⛔ At 115 uu a lateral offset of **> ~66 uu** is outside the cone. The hero was driven by `Pawn::AddMovementInput(+X)` and **teleported**; a teleport does **not** change yaw, and `bOrientRotationToMovement` only turns him while he is *moving*. ⭐ And `MilitiaMob` is a **SWARM — "spawns 4 copies for one cost"** (`Docs/Data/cards.csv:8`) ⇒ ⛔ the body that happened to be 115 uu away need not be the one in front of him | ⛔ **the hero's yaw, or the bearing to the target.** ⇒ this is my ⛔ **leading candidate** |
| **2** | ⛔ **TEAM** — `IsHostileTeam` at `SiegeCombatStatics.cpp:70`, and again the friendly-fire veto at `SummonedUnit.cpp:5407` | ⛔ A same-team mob is dropped at the gather **and** rejected at `TakeDamage` — belt and braces, both silent | ⛔ **the MilitiaMob's `TeamId`.** ⚠️ The handoff records the Footman's team explicitly (`hero BLUE, target RED`) and ⛔ **records nothing at all for the mob** |
| **3** | ⛔ **ALREADY DEAD** — `if (bDead …) return 0.f;` at `SummonedUnit.cpp:5398` | ⛔ `GatherTeamAgentsFiltered` has ⛔ **no dead filter** — a still-valid corpse is returned to the swing, is struck, and returns 0 | ⛔ **the mob's HP *before* the swing.** The Footman reading was `80.0 → 60.0`; the mob has ⛔ no "before" recorded |
| **4** | ⛔ the veil | ⛔ requires a Witch — ⛔ **§4(a): unreachable as recorded** | ⛔ n/a |

⚖️ ***The `SC-§90` sentence for this row: the host offered a mechanism that EXISTS and is CORRECTLY DESCRIBED, and that is not the same as a mechanism that could have RUN. Three of the four candidates need no Witch; the one that was named is the only one that does.***

---

## 5. ⚠️ ONE FINDING IN PASSING, DECLARED RATHER THAN HIDDEN — **A FALSE SENTENCE IN `DoMeleeAttack`'s OWN HEADER**

`Source/GitClaudeUnrealTest/Siegebound/HeroCharacter.cpp:477-478`:

```cpp
// (WITCH-§1) ⇒ the hero ⛔ cannot auto-swing at a veiled enemy. ⚠️ He can still hit one he aims
// at manually if he knows where it is — veiled units are ⛔ hidden, ⛔ not invulnerable.
```

⛔ **The second sentence is FALSE OF THE FUNCTION IT IS ATTACHED TO.** `DoMeleeAttack` has **no manual-aim lane**: the swing's entire candidate set is the output of `GatherHostileAgents(…, SuppressVeiled, …)` at `:585`, and the range and cone tests **only narrow** that set — ⛔ **no aim, no knowledge and no precision can put a veiled actor back into `HostileAgents`.** The distinction *"hidden, not invulnerable"* is true of the **BLAST** lane (`IncludeVeiled`, `WITCH-§2` / `J-W2`) and it is ⛔ **not true of the sword**; sitting in the sword's header it reads as a property of the sword.
⇒ ⚖️ ***Exactly the `WITCH-§9.1` row 4 / `SC-§39.1` cl. 9 species: prose in the present tense describing a behaviour the tree does not exhibit — and note it would have MISDIRECTED THIS VERY DIAGNOSIS, because a reader who trusts it concludes the swing has a manual path and goes looking for why the manual path failed.*** ⛔ **NOT FIXED — this row is read-only.** Specced in §6 as a doc-only row.

---

## 6. ⛔ IF A FIX IS WANTED: THE SPECIFICATION, AND WHY I STOPPED

### ⛔⛔ FIX (i) — THE ONE THAT ACTUALLY ADDRESSES THE PLAYER COMPLAINT: **⭐ `TASK-931`, ALREADY BOARDED. ⛔ NO NEW ROW NEEDED.**
- **Row:** `TASK-931` — [VEIL-TELLS], *"one per-viewer predicate, consulted from N sites"*. Board status today: `boarded — NOT dispatchable until TASK-925 has COMMITTED` (⛔ **that blocker is stale — `TASK-925` shipped long ago; the row's status line simply has not been re-read**).
- **What it fixes:** the paint at `SummonedUnit.cpp:3149` becomes viewer-conditional, so an enemy player stops seeing a positional marker with a silhouette. ⇒ ⛔ **the swing's behaviour is then indistinguishable from correct, with zero combat code touched.**
- **Recommendation to the manager:** ⛔ **re-check `TASK-931`'s blocker and dispatch it. That is the whole repair for question B.**

### ⚠️ FIX (ii) — DOC-ONLY, TINY, AND IT PREVENTS THE NEXT WRONG DIAGNOSIS
- **File:** `Source/GitClaudeUnrealTest/Siegebound/HeroCharacter.cpp` · **symbol:** the header comment above `AHeroCharacter::DoMeleeAttack` (📌 *as of today `:477-478` — ⛔ `SC-§38`: locate by symbol*).
- **Shape:** replace *"He can still hit one he aims at manually"* with the measured truth — ⛔ **the sword can NEVER reach a veiled unit; the exemption is the BLAST lane (`IncludeVeiled`), which is where "hidden, not invulnerable" is actually true.**
- **Test needed?** ⛔ **No.** Comment-only, no behaviour, no symbol. ⛔ It is a `Source/**` edit, so it still needs an authoring row + a QA gate + a commit host — ⛔ **which is precisely why I did not make it here.**

### ⛔ FIX (iii) — **NOT RECOMMENDED, SPECIFIED ONLY SO NOBODY REACHES FOR IT: exempting the hero's sword from `SuppressVeiled`.**
- ⛔ **It would delete the card.** A 50-gold veil whose whole promise is *"undetectable"* becomes a unit the hero point-blank-sweeps regardless — and the swing is a **cone auto-sweep**, ⛔ **not an aimed single-target strike**, so "the player earned it by aiming" is not what would ship: what would ship is *every* veiled unit inside a ±30° / 150 uu arc taking full damage from a blind swing.
- ⛔ It touches the ⛔ **one** guard point that `WITCH-§0`/`§1` built the entire funnel to have exactly one of, and it re-opens a ruling (`WITCH-§2` lane 1) that is 🧑 **Jonathan's**.
- ⇒ ⛔ **If anyone ever wants it, it is a MANAGER AMENDMENT TO `WITCH-§2`, never a code row.**

### ⚪ OPTIONAL (iv) — CLOSING THE 115 uu INCIDENT ITSELF
⛔ **Low value, and I would not spend a row on it.** The session is gone and the observation is unreproducible from source. The cheap durable fix is a **dispatch-text rider on the next hero PIE proof**, not a task: ⛔ *when recording a melee reading, record the target's `TeamId`, its HP BEFORE the swing, and the horizontal angle between the hero's forward vector and the target* — ⛔ **those three numbers discriminate all four candidates in §4(c), and `TASK-1094` recorded none of them for the mob while recording the first for the Footman.**

### ⛔ WHY I STOPPED
This row is ⛔ **DIAGNOSE-ONLY** and holds ⛔ **no gate and no ship host** (its own `names:` line: *"GATE: NONE NEEDED — read-only diagnosis, produces NO `Source/` artefact"*). ⛔ Every fix above is either **already boarded** (i), a **`Source/**` edit needing its own authoring row + QA gate + commit host** (ii), or a **design amendment that is not mine to make** (iii). ⛔ **A veil-policy change touches combat targeting for every unit on the field and must not be smuggled into a diagnosis** — the row says so, and it is right.

---

## 7. WHAT QA / THE MANAGER SHOULD SCRUTINISE

1. ⛔ **My §4(a) reachability claim rests on `GrantInvisibility` having exactly ONE caller.** Re-measure by symbol if you doubt it: `grep -rn "GrantInvisibility" Source/` returns the definition (`SummonedUnit.cpp:3084`), the one call (`:3597`), the header declaration and prose. ⛔ **If a second caller ever exists, my verdict C changes.**
2. ⛔ **I could not observe the session.** Verdict C is *"cannot tell"* on purpose — ⛔ **I am ranking candidates by reachability, not declaring the cone guilty.** ⛔ Do not let anyone downstream promote my leading candidate to a measured cause.
3. ⭐ **Verdict A and verdict B are not in tension and must not be merged.** The code is correct; the feature still does not reach the player — ⛔ the same shape `WITCH-§8`/`§9` records twice already.
4. ⛔ **The §5 comment is a live falsehood in shipped source, and I was fenced from fixing it.** ⚖️ *`SC-§39.1` cl. 3.3 — a fence limits the repair, never the report.*

## 8. FILES

- **Written:** `.claude/pipeline/handoffs/TASK-1105-programmer.md` (this file) — ⛔ **the only write.**
- **Read (no edits):** `Source/GitClaudeUnrealTest/Siegebound/HeroCharacter.{cpp,h}` · `SiegeCombatStatics.{cpp,h}` · `SiegeInvisibilityStatics.{cpp,h}` · `SummonedUnit.cpp` · `SiegeBotController.cpp` · `Tower.cpp` · `Docs/Data/cards.csv` · `.claude/pipeline/CONVENTIONS.md` (`WITCH-§`) · `.claude/pipeline/handoffs/TASK-1094-buildmaster.md` · `.claude/pipeline/TASKBOARD.md` (rows `TASK-1105`, `TASK-931`).
- **Board:** `TASK-1105` status line only.
