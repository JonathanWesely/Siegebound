<!-- MOVED from .claude/pipeline/CONVENTIONS.md on 2026-10-04 by Tools/split_conventions.py. Sections are byte-identical to the original; this comment is the only addition. Cite clauses by tag (e.g. `VER-§3 cl. 6`); CONVENTIONS.md '## Law index' maps tags to files. -->
## ⚖️ RECALL — the 10-second channel home (2026-09-01) — namespace **RECALL-§**

📌 **BORN WITH ITS NAMESPACE PREFIX (`RECALL-§N`).**

**Trigger — Jonathan's directive, verbatim:** *"I want to add a recall feature. From anywhere on the map, you can press "b", and that will allow the player to starting a recall animation similar to league of legends where after 10 seconds they teleport back to their castle and they completely refill their health. During this recall animation the player cannot attack and if they get hit with an attack it interupts the channel and they would have to press "b" again to start it from the beginning."*

### RECALL-§1 ⭐⭐ THE MEASUREMENT THAT SHRINKS THIS FEATURE — **THE TELEPORT AND THE FULL HEAL ALREADY SHIP**

- ✅ **`ASiegeGameMode` ALREADY OWNS A TELEPORT-HOME-AND-FULL-HEAL PATH**, read at source 2026-09-01 (`SiegeGameMode.h:46-51`): the hero's `FOnHeroDied` schedules a respawn *"exactly `HeroRespawnDelay` (5 s) later: **teleport to the PlayerStart** (L_Arena places it on the Blue side) **or — when no PlayerStart exists — next to the hero's own castle**, then repossess and `ResetHero()` (**full HP**, input restored)."*
- ⇒ ⭐ **RECALL IS ⛔ NOT A NEW DESTINATION RULE. It is a CHANNEL in front of a destination rule that already exists, and it must reuse that rule's resolution ⛔ rather than hand-type a location.** ⚠️ **`handoffs/TASK-569-buildmaster.md` row (n) — *"hero spawns OUTSIDE the keep"* — is the recorded precedent for getting exactly this wrong. Cite it; do not repeat it.**
- ⛔⛔ **BUT ⛔ RECALL MAY ⛔ NEVER CALL `ResetHero()`. THIS IS THE BATCH'S SHARPEST TRAP AND IT IS PRE-EMPTED HERE RATHER THAN AT A GATE.** `ResetHero()` is a **DEATH-path** function: it **re-applies the hero's cumulative upgrade mods onto a freshly-restored base** (`HeroCharacter.cpp:711-722`), **re-arms the War Banner aura** (`:752`), and **restores input that `HandleDeath` disabled** (`:739`). ⇒ ⛔ **Calling it on a LIVE hero double-applies upgrades and re-arms a running aura.** ✅ **Recall performs exactly two effects: the teleport, and `CurrentHP = GetEffectiveMaxHP()`.**
- ⛔⛔ **THE HEAL READS `GetEffectiveMaxHP()`, ⛔ NEVER `MaxHP`.** `HeroCharacter.h:337` names it *"the single source used by every HP clamp / regen cap / full-heal."* ⚖️ **A `MaxHP` read silently under-heals a Plate-Armor hero by up to 200 HP and looks completely correct in review** (`MaxHP` 200 + `MaxHPBonus` 100 × 2 stacks).

### RECALL-§2 ⛔ INPUT — **`B` GOES THROUGH THE LAYOUT SYSTEM, ⛔ NEVER A HARDCODED KEY**

- **New asset: `IA_Recall`** (`/Game/Input/Actions/IA_Recall`, Digital/bool) mapped to **B** in `/Game/Input/IMC_Hero`.
- ⛔⛔ **`B` IS ⛔ NOT ASSUMED FREE — IT IS PROVEN FREE, IN THE ASSET, BEFORE IT IS CLAIMED.** This is **`HELP-§4`'s law, verbatim, in its second application**: `IMC_Hero` is a **binary asset** and the bindings live inside it. ⛔ **FLAG — ⛔ never stomp — any conflict** (the `IA_CmdAmbush`/**F** precedent). **A conflict is a FOR-JONATHAN row, ⛔ not an agent's call.**
- ✅ **The append is the `KBD-§2a` EDITOR-TIME AUTHORING CARVE** — a scratchpad script, ⛔ never `Source/`, **ONE `MapKey`, ONE appended row**, ⛔ `UnmapKey`/`UnmapAll` stay banned, **EMPTY `Triggers` + EMPTY `Modifiers`**, and the survivors proven **by naming the modifier objects, ⛔ not the keys**.
- ⭐ **`KBD-§4` TABLES ALL 26 LETTERS ⇒ `B` INHERITS DVORAK SUPPORT WITH ⛔ ZERO EXTRA CODE.** ⛔ **No `EKeys::B` literal on any shipped path.**
- ✅ **Controller/hero side: soft-ref + null-safe resolve — a missing asset means `B` is INERT, ⛔ never a crash** (the `IA_Cmd*` pattern).

### RECALL-§3 ⛔⛔ `Escape` IS UNTOUCHABLE — **AND THE CHANNEL ADDS ⛔ NO KEY HANDLER AT ALL**

- ⛔ **`AS-§6` A-2 is PERMANENT.** ⛔ **The channel may ⛔ NOT absorb, re-route, consume or "harmlessly handle" `Escape`** — ⛔ not via `NativeOnKeyDown`, ⛔ not `NativeOnPreviewKeyDown`, ⛔ not an Enhanced Input action, ⛔ not a Slate `FReply::Handled()`, ⛔ not a viewport intercept. **Returning `Handled` for `Escape` is overturning a Jonathan ruling and is an AUTOMATIC QA FAIL.**
- ✅ ⇒ **THE CHANNEL IS CANCELLED BY `B` OR BY MOVEMENT AND BY NOTHING ELSE. That is the complete list.** ⚠️ **The shipped cancel routes (placement, spell targeting, group-pick) must keep firing byte-identically while a channel runs.**

### RECALL-§4 ⚖️ THE RULINGS — decided, with reasons, ⛔ none blocking

| # | Question | ⚖️ **Ruling (proceeding default)** | Why |
|---|---|---|---|
| **R-1** | Does *any* damage interrupt, or only damage that LANDS? | ✅ **ONLY DAMAGE THAT LANDS** — a `TakeDamage` that reduces HP by **> 0 after mitigation**. ⛔ A miss / blocked / 0-damage event does ⛔ NOT interrupt | ⚖️ **"if they get hit with an attack" is the player's language for *taking damage*.** ⭐ **And it is the only version that is TESTABLE at a single seam:** `AHeroCharacter::TakeDamage` (`HeroCharacter.h:136`) already returns the applied amount. ⛔ Coupling to *attempted* attacks would need a new notification surface for a rule nobody asked for |
| **R-2** | May the player cancel deliberately? | ✅ **YES — re-pressing `B`, or moving.** ⛔ **Never `Escape`** (`RECALL-§3`) | ⭐ **Movement-cancel is the League convention he invoked by name.** ⛔ A channel with no voluntary exit is a trap the player walks into once and never uses again |
| **R-3** | Is the channel visible to the enemy? | ✅ **YES — a local visual tell ships.** ⛔ **The M8 replication shape is DECLARED and ⛔ NOT BUILT** | ⭐ **He named League; the tell is the mechanic's counterplay and the reason it has a duration at all.** ⚠️ **Honest limit: today's only opponent is `ASiegeBotController`, which does ⛔ not look at it** ⇒ **the tell is for the HUMAN observer and costs nothing now.** ⛔ **Do ⛔ not describe it as counterplay the bot exercises** (`ACC-§8`'s reserved-not-authored precedent) |
| **R-4** | Restart-from-zero on re-press? | ✅ **YES, from zero. ⛔ No resume, ⛔ no partial credit** | ⛔ **His own words: *"start it from the beginning."*** ⛔ Not re-litigable |
| **R-5** | Can the hero attack while channelling? | ⛔ **NO** — ride the hero's **shipped** attack entry point with a state term, ⛔ **never a new suppression mechanism** | ⭐ **`TOWER-§9.2`'s idiom, second application: a state term at an EXISTING guard point, ⛔ not a fourth guard point.** ⚠️ Movement is ⛔ NOT restricted — moving *cancels* (`R-2`), which is a different rule and must not be conflated |

- **`RecallChannelSeconds = 10.f`**, `EditDefaultsOnly`, **with its consequence written beside it** (`HIGH-§1`'s law: a number whose consequence is not written next to it gets retuned by someone who does not know what they are changing).
- ⛔ **EVERY EXIT IS ENUMERATED AND EACH CLEARS THE CHANNEL EXACTLY ONCE** — completion · re-press · movement · damage-that-landed · hero death · match end · `EndPlay`. ⚠️⚠️ **`TOWER-§8`'s hanging-unit lesson generalises: a timed state whose exits are not enumerated will strand the player in one of them.** **Name all seven in the handoff and say which test covers each.**

### RECALL-§4a ⚖️ **THE TELL'S COLOUR — RULED ⭐ CYAN/FROST (2026-09-01). THE BOARD'S "ORANGE-RED" RECORD WAS ***STALE***; ⛔ THE ASSET WAS RIGHT.**

- ✅ **THE SHIPPED ASSET RENDERS CYAN.** `/Game/VFX/NS_RecallChannel`, re-donored from `/Game/Ice_Magic/VFX_Niagara/NS_Ice_Magic_Orb` (worktree sha `a0d0fea3…992600`, 1,955,375 B; was `a082d797…90cdc`), committed in `4a03e03`, wired to `RecallChannelEffect` on the `BP_HeroCharacter` CDO at TASK-742 and **proven on pixels in PIE**.
- ⛔⛔ **THE STALE PARTY WAS `TASKBOARD.md`'s TASK-747 STATUS LINE, AND IT HAS BEEN REPAIRED. ⛔ THE ASSET WAS ⛔ NOT TOUCHED.** ⚖️ **When the board and a committed asset disagree, ⭐ MEASURE THE ASSET — ⛔ never "reconcile" the disagreement by editing what shipped.** *(This is the `PKG-§7a` lesson in a new lane: a record repaired on a relay is a record damaged.)*
- ⚖️ **THE HUE RULING (manager):** the tell may ⛔ **not read as *damage*** and may ⛔ **not wear the ENEMY banner colour** — fire does both. ⇒ **cool-toned.** The remaining question was only *which* cool effect, and it was settled by measurement, ⛔ not taste.
- ⭐⭐ **TASK-747's EARLIER CYAN REJECTION WAS OVERTURNED ***BY MEASUREMENT***, AND THE REASON IS THE REUSABLE PART: THE REJECTION CAPTURE CONTAINED ⛔ NO CHARACTER AT ALL.** Re-shot with a hero-sized skeletal mesh at the **code-faithful** offset (`SpawnSystemAttached(..., SnapToTarget)` ⇒ capsule origin ⇒ feet + 96 uu) at the shipped **400 uu** `CameraBoom.TargetArmLength`: **head, both arms, torso and both legs all read.** ⚖️ *An occlusion claim measured without the thing being occluded is not a measurement.*
- 🚩 **THE HONEST COST, KEPT IN THE LAW RATHER THAN BURIED:** identical camera pose against a Simulate-off control frame — **cyan repaints 36.9 % of the hero's screen band vs fire's 8.7 %** (touched 54.9 % vs 15.7 %), and it is **TRANSLUCENT, ⛔ not a blowout** (band luminance **135 → 129**, i.e. it *darkens*). ✅ **`R-1` counterplay survives: the enemy can see and hit the recalling hero.** ⚠️ **All captures are DAYLIGHT — in the 12-cd castle interior (`TASK-620..622`) a bright cool orb will bloom harder than these numbers suggest.**
- 🙋 **JONATHAN MAY OVERRULE ON SIGHT AND IT COSTS ONE PROPERTY** — a duplicate at the same path plus the Niagara-editor open (see the inertness law under "Material & Niagara lane laws"). ⛔ **No agent re-colours it on an opinion, in either direction.** 📌 **`FAB-008` remains the real answer; this is the best effect the project owns, ⛔ not a purpose-built one.**

### RECALL-§5 ⛔ THE HELP OBLIGATION IS PART OF THIS FEATURE, ⛔ NOT A FOLLOW-UP

`HELP-§2` mechanism 2 is explicit: **a new action is made SURFACE-ABLE, ⛔ not automatically documented**, and *"a row whose text is missing renders as an explicit **(undocumented — TODO)**"*. ⇒ ⛔ **Recall ships with its `HELP-§` row AND its detail page in the same batch, or it ships visibly undocumented. There is no third option.** ⛔ **Every sentence in the detail page is traceable to a file:line the author actually read** (`HELP-§2` mechanism 3).

### RECALL-§6 📌 M8 DECLARATION

⛔ **NO replicated property and ⛔ NO RPC are AUTHORED in this batch.** ⚠️ **The channel is authority-relevant state and `R-3` names an enemy-visible tell** ⇒ **the M8 shape (`Server`/`Client` verb-noun, `ACC-§8`'s reserved-not-authored discipline) is DECLARED in a header comment and ⛔ NOT built.** ⛔ **"Nothing to declare" is false here and must not be copied from another batch's boilerplate** (`WR-§8`'s standing warning).

---

