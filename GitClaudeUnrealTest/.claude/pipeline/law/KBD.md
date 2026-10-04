<!-- MOVED from .claude/pipeline/CONVENTIONS.md on 2026-10-04 by Tools/split_conventions.py. Sections are byte-identical to the original; this comment is the only addition. Cite clauses by tag (e.g. `VER-§3 cl. 6`); CONVENTIONS.md '## Law index' maps tags to files. -->
## Keyboard layout / positional input (2026-08-04) — namespace **`KBD-§N`**

Added 2026-08-04 on Jonathan's directive, verbatim: *"we need to make the game handle alternative keyboard layouts. I currently use the Dvorak keyboard, so I want the game to be able to detect the layout and adjust which letter keys are mapped so that it still puts the keys in the same locations as if we were using qwerty."*

Design authority = the approved plan `C:\Users\wesel\.claude\plans\ok-there-are-a-cheerful-mccarthy.md` (produced in plan mode, every engine claim verified against installed UE 5.8 source, approved by Jonathan via ExitPlanMode). ⛔ **The plan file wins over any board summary. The architecture is NOT re-litigated** — this section is the naming/behaviour law derived from it, plus the batch's link contract.

> ### ⛔⛔ **STOP — DO NOT OPEN THAT PLAN FILE EXPECTING THIS FEATURE. THE PATH IS DEAD AND IT RESOLVES TO SOMEBODY ELSE'S PLAN.**
> **The plan-file slot was OVERWRITTEN later the same day. ⚠️⚠️ UPDATED 2026-08-05 — IT HAS NOW BEEN OVERWRITTEN *TWICE*: the file on disk at that path today is the *AI-COMMANDER ROBUSTNESS* plan (`AS-§21`). Generation 1 = this keyboard-layout plan (GONE) · generation 2 = the *UNIT-PATHING* plan (`NAV-§0..§14`) (GONE) · generation 3 = `AS-§21`'s.** ⚠️ **The path above is kept only as a provenance record of what this law was derived from — ⛔ it is NOT a live pointer, and following it lands you in a DIFFERENT feature (and no longer even the same different feature the earlier warning named).**
> - ✅ **YOU DO NOT NEED IT: `KBD-§0..§11` IS COMPLETE ON ITS OWN.** Every ruling, name, signature and limitation this feature has is in this section. ⛔ **There is nothing to re-read.**
> - ⛔ **AND THE MOVE THIS EXISTS TO PREVENT, NAMED SO NOBODY MAKES IT: having landed in the pathing plan, DO NOT "reconcile" the mismatch by editing `NAV-§`, and do not edit `KBD-§` to match what you read there.** ⚖️ **Two laws, one filename, zero overlap — the collision is in the PATH, never in the law.**
> - 📌 Mirrored at `NAV-§`'s own header and at `TASKBOARD.md:6877`. **Recorded on BOTH sides 2026-08-04 (`qa/TASK-537.md` follow-up) — it had been recorded only on the pathing side, which is the side that does not need the warning.**

📌 **THIS SECTION IS BORN WITH ITS NAMESPACE PREFIX (`KBD-§N`), BEFORE TASK-503's retrofit runs.** ⚖️ Cite as `KBD-§2`, never as a bare `§2` — this file already carries three sections whose bare `§12b` and `§14` collided (`qa/TASK-502.md`), and the cheapest time to prevent that is at authoring.

### KBD-§0. ⚖️ JONATHAN'S TWO RULINGS — DECIDED, BINDING, ⛔ NOT RE-OPENABLE BY ANY AGENT

Both answered by Jonathan directly via AskUserQuestion on 2026-08-04. Recorded here because the *"shouldn't this be a setting?"* question is the single most likely thing a future agent proposes.

1. ⛔ **ALWAYS-ON AUTO-DETECT. NO PLAYER-FACING SETTING.** ⛔ **No new field in `USiegeSettingsSaveGame`, no row in `USettingsMenuWidget`, no `SC-§8` registry addition.** A **CVar for runtime testing is permitted** (`KBD-§7`); a settings toggle is **out of scope and is a scope breach if added.** ⚖️ The remap is a *correctness* fix — the keys go where they are printed on a QWERTY reference — not a preference.
2. ⛔ **MID-SESSION LAYOUT SWITCHES ARE IN SCOPE, NOT DEFERRED.** Jonathan may `Win+Space` to QWERTY for gaming and back. **The 1 Hz HKL poll AND the `OnApplicationActivationStateChanged` hook both ship in this batch.** ⚠️ A design that only probes at `Initialize` does not satisfy the directive and is incomplete, not "phase 1".

### KBD-§1. ⛔ THE CENTRAL LAW — AND IT IS THE ONE THAT DESTROYS THIS FEATURE IF BROKEN

> ### ⛔ **THE POSITIONAL REMAP NEVER WRITES TO `IMC_Hero.uasset`. IT WRITES ONLY THE `.Key` FIELD OF A TRANSIENT `DuplicateObject` COPY — ⛔ NEVER `Modifiers`, ⛔ NEVER `Triggers`, ⛔ NEVER THE MAPPINGS ARRAY ITSELF.**

- ⚠️ **THE PRECEDENT IS THIS REPO'S OWN AND IT ALREADY COST A PLAYTEST — `.claude/pipeline/handoffs/TASK-445-artist.md:95-105`: rewriting the mappings array SILENTLY DEFAULT-CONSTRUCTED the instanced `SwizzleAxis`/`Negate` modifiers. WASD broke and mouse-look inverted — WHILE THE PROPERTY TABLE STILL READ CORRECT.** ⛔ That last clause is the whole reason this is a law and not a code comment: **the readback passed.** It is the project's named failure class (`SC-§17`) wearing an input asset's clothes.
- ✅ **THE STRUCTURAL FORM OF THE FIX — copy it, do not merely obey it:** every value written is **re-derived from the pristine `Source` array BY INDEX**, and the only field ever assigned is `.Key`. ⇒ **The TASK-445 failure mode is ABSENT BY CONSTRUCTION, not dodged by care** (`AS-§17`'s standing preference: a hazard closed by choosing a different shape does not reopen when somebody is tired).
  ```cpp
  const TArray<FEnhancedActionKeyMapping>& Src = Source->GetMappings();
  for (int32 i = 0; i < Src.Num(); ++i)
  {
      const FKey* T = Translation.Find(Src[i].Key);
      Target->GetMapping(i).Key = T ? *T : Src[i].Key;   // ONLY .Key. Ever.
  }
  ```
- ⚠️ **RE-DERIVING FROM `Source` IS ALSO WHAT MAKES THE SUBSTITUTION SIMULTANEOUS.** On Dvorak the map holds **`D→E` AND `E→Period`** — reading the *target* would cascade `D→E→Period`. **This sentence belongs in the header comment**, because the bug it prevents is invisible in review and looks like a model failure at playtest.
- ⛔ **REVIEWABLE INVARIANT, AND IT IS THE CHEAPEST CHECK IN THE GATE: the source asset is only ever held through `const` pointers. THE ONLY NON-CONST `UInputMappingContext*` IN THE ENTIRE FEATURE IS THE DUPLICATE.** `AddMappingContext` takes `const UInputMappingContext*` (`EnhancedInputSubsystemInterface.h:265`), so **no cast is needed** — a cast appearing here is itself the finding. ⚠️ In PIE the source **is** the editor's loaded asset; read-only access is what makes PIE structurally unable to dirty it.

### KBD-§2. ⛔ `MapKey` / `UnmapKey` / `UnmapAll` ARE BANNED ON EVERY SHIPPED PATH — AND THE SUPPORTED PATH, WITH ITS STALE-SYMBOL TRAP

- ⛔ **BANNED, VERIFIED AT `InputMappingContext.cpp:154-158`: `MapKey` APPENDS a mapping built from the 2-arg `FEnhancedActionKeyMapping` ctor (`EnhancedActionKeyMapping.h:44`) with EMPTY `Modifiers` and EMPTY `Triggers`.** ⇒ An unmap+map of `IA_Move`/`W` **drops the Swizzle** — ⚠️ **TASK-445's failure rewritten in C++**, same silence, same wrong-feeling movement. **`UnmapKey` and `UnmapAll` are banned with it**, since they exist only to enable that round-trip.
- ✅ **THE SUPPORTED MUTATION PATH IS `UInputMappingContext::GetMapping(Index)`** — `InputMappingContext.h:220`, verified **public**, **non-const ref**, and ⭐ **NOT `WITH_EDITOR`-guarded** (that last property is what makes the whole approach shippable, and it is the one a reviewer will doubt).
- ⛔ **FOLLOWED BY `UEnhancedInputLibrary::RequestRebuildControlMappingsUsingContext(const UInputMappingContext* Context, bool bForceImmediately)`** — `EnhancedInputLibrary.h:36-37`.
- ⚠️⛔ **THE STALE-SYMBOL TRAP, RECORDED SO NOBODY GREPS FOR THE WRONG NAME: THE ENGINE'S OWN DOC COMMENT AT `InputMappingContext.h:217` NAMES `…ForContext`, WHICH DOES NOT EXIST IN 5.8.** The correct symbol is **`RequestRebuildControlMappingsUsingContext`**. ⇒ ⚖️ **A grep for the documented name returns nothing, and `AS-§14` already ruled that a search result is evidence about the search — an agent that trusts the engine comment here will conclude the API was removed and go looking for `MapKey`, which is the banned path.** *The trap leads directly to the ban.*

### KBD-§2a. ⚖️⭐ AMENDMENT 2026-08-15 (`W7-R3`) — **THE EDITOR-TIME AUTHORING CARVE.** ⛔ `KBD-§2` IS **NOT** RELAXED; ITS SCOPE IS **STATED**, AND THE ENGINE'S OWN COMMENT DRAWS THE SAME LINE

⚠️ **Raised by TASK-568, which needed ONE new mapping (`IA_WarMap` → `M`) in `IMC_Hero` and correctly REFUSED to guess.** ⭐ **It also refused the one operation MCP *would* have accepted — rewriting the whole `defaultKeyMappings` array — which is verbatim the TASK-445 defect. ✅ That refusal is RATIFIED and is the reason this carve can be written at all.**

- ✅ **THE CARVE: `UInputMappingContext::MapKey` IS PERMITTED IN AN *EDITOR-TIME ASSET-AUTHORING SCRIPT*, TO APPEND A **BRAND-NEW** MAPPING.** ⛔ **`KBD-§2`'s ban is unchanged on every SHIPPED path — its own title says *"ON EVERY SHIPPED PATH"*, and its stated mechanism is an unmap+map ROUND-TRIP of an EXISTING mapping dropping `IA_Move`'s Swizzle.** ⇒ ⚖️ **An append of an action that is REQUIRED to carry zero modifiers and zero triggers cannot express that failure. The ban's mechanism cannot fire.**
- ⭐⭐ **AND THE ENGINE DRAWS THE IDENTICAL LINE, IN ITS OWN WORDS — ✅ MANAGER-READ AT THE INSTALLED 5.8 SOURCE, ⛔ NOT RELAYED (`InputMappingContext.h:222`, the comment directly above all four Map/Unmap declarations):** *"Don't want to encourage Map/Unmap calls here where context switches would be desirable. **These are intended for use in the config/binding screen only.**"* ⇒ ⭐ **`KBD-§2` and Epic's own note are THE SAME RULE: banned for runtime context switching, sanctioned for authoring. That is a rare and load-bearing agreement — cite it, do not re-argue it.**
- ✅ **THE REFLECTED-SYMBOL CHECK PASSES** (the standing law: *before offering the commandlet, NAME the reflected symbol it will call*): **`UFUNCTION(BlueprintCallable, Category = "Mapping")` at `InputMappingContext.h:227-228`** ⇒ callable as `imc.map_key(action, key)` from in-editor Python **and** from `-run=pythonscript`. ⛔ **No MCP toolset calls UFUNCTIONs at all, so MCP is structurally out of this lane** — that is a capability fact, not a tooling complaint.
- ⛔ ~~**FOUR CONDITIONS.**~~ ⭐ **FIVE CONDITIONS (amended 2026-09-03, TASK-820 — condition 5 is new). EACH IS AN ACCEPTANCE CRITERION, ⛔ NOT ADVICE:**
  1. ⛔ **The call lives in a SCRATCHPAD authoring script. ⛔ NEVER in `Source/`.** A `MapKey` in a `.cpp` is still a `KBD-§2` violation and QA still fails it.
  2. ⛔ **ONE call, ONE appended row. ⛔ `UnmapKey` / `UnmapAllKeysFromAction` / `UnmapAll` STAY BANNED IN EVERY LANE** — they exist only to enable the round-trip. **Their presence in the script is a STOP.**
  3. ⛔⛔ **PROVE THE SURVIVORS BY NAMING THE MODIFIER OBJECTS, ⛔ NOT THE KEYS.** ⚖️ **TASK-445's whole signature is *"the property table still reads correct"*, so a key-list readback is precisely the instrument the defect defeats.** **Required: count `N → N+1` where `N` is the count MEASURED IN THE ASSET IMMEDIATELY BEFORE THE APPEND; the first `N` keys identical IN ARRAY ORDER; and `IA_Move`'s four rows still carrying `InputModifierSwizzleAxis_0` / `SwizzleAxis_1` + `Negate_0` / `Negate_1`, and `IA_Look` still carrying `Negate_2`, BY NAME.**
     - ⚖️⚠️ **REPAIRED 2026-08-30 ON TASK-705's MEASUREMENT — THE LITERALS WERE STALE AND WOULD HAVE CAUSED A FALSE QA FAIL AGAINST A CORRECT APPEND.** ~~count `24 → 25`; the first 24 keys~~ was true when written (2026-08-15) and **went stale the moment `IA_WarMap`→`M` landed (TASK-568): the live count was `25 → 26`, and each future append moves it again.** ⇒ ⛔ **A reviewer checking the LITERAL `24` against a correct `25 → 26` append fails a good diff** — and *"the law says 24"* is an argument that beats a tired implementer who is, in fact, right. **The invariant was always RELATIVE; only its expression was absolute.**
     - 🧊⛔⛔ **EVERY NUMBER IN THE TWO SENTENCES ABOVE IS A ⛔ FROZEN HISTORICAL EXAMPLE. ⛔ IT IS ⛔ NOT THE EXPECTED VALUE, AND IT MAY ⛔ NEVER BE QUOTED AS ONE — ⛔ NOT `24`, ⛔ NOT `25`, ⛔ NOT `26`, ⛔ NOT ANY NUMBER THAT APPEARS ANYWHERE IN THIS CLAUSE.** ⭐ **MANAGER RULING 2026-09-03 (TASK-820 `D4`), and it is deliberately the ⛔ OPPOSITE of "refresh them":** refreshing the illustration to today's `27 → 28` would ⛔ **re-arm the exact scheduled false failure this sub-clause exists to disarm**, and the bullet directly below already ⛔ bans pinning a growing count as a literal in law. ⛔ **Refreshing would have made this clause violate itself.**
       - ⛔⛔ **THE ONLY EXPECTED VALUE IS `N → N+1` WHERE `N` IS MEASURED AT THE ASSET IN THE SAME SESSION AS THE APPEND.** ⛔ **A QA verdict that cites a literal from this clause as the expected count is an ⛔ AUTOMATIC FALSE FAIL and the ⛔ reviewer's error, ⛔ not the implementer's.**
       - 📌 **THE REPORTING LOOP IS ⛔ CLOSED — ⛔ DO ⛔ NOT REPORT THESE NUMBERS AS STALE AGAIN.** ⚠️ **TWO consecutive tasks correctly flagged them** (TASK-747-era, then TASK-820 `D4`, which measured the live `27 → 28` and satisfied the RELATIVE law exactly as written). ✅ **Both were right to flag it and ⛔ neither was owed an amendment.** ⇒ ⭐ **The 🧊 marker IS the amendment: the numbers are frozen prose, the invariant is the law, and a third report costs a task's attention for ⛔ nothing.** ⚖️ *A stale literal is dangerous because it is CONFIDENT; a literal ⛔ labelled frozen is inert — and inert beats fresh, because fresh goes stale again on the next append.*
     - ⭐ **THE GENERAL RULE THIS BUYS, AND IT IS THE POINT: ⛔ NEVER PIN A GROWING COUNT AS A LITERAL IN LAW.** A number that increments with normal work is a **scheduled false failure**. ✅ **Pin the INVARIANT (`N → N+1`, first `N` identical in order, named modifiers surviving) and require `N` to be MEASURED AT THE ASSET IN THE SAME SESSION AS THE APPEND** — which also makes the check strictly stronger, because it re-reads the truth instead of trusting a remembered one. ⚠️ **Same family as the M7.7 `Notes` drift (*"in 400"* vs `AoERadius` **700**) and `PKG-§9a-3`'s inert key: a stale literal is more dangerous than a missing one, because it is CONFIDENT.**
  4. ⛔ **The appended row carries EMPTY `Triggers` and EMPTY `Modifiers`** — matching the shipped `IA_AssistantConsole` row. ⭐ **Its emptiness is the SPECIFICATION, not a side effect. That is exactly why the append is safe and the round-trip is not.**
  5. ⭐⭐⛔⛔ **NEW 2026-09-03 (bought by TASK-820): THE APPEND IS ⛔ NOT DONE UNTIL THE ⛔ ON-DISK BYTES CHANGE.** ⛔ **Record the `IMC_Hero.uasset` **sha256 + byte length BEFORE** the `map_key` and **AFTER** the save. **Identical ⇒ ⛔ NOTHING WAS WRITTEN ⇒ ⛔ STOP** — ⛔ the task is ⛔ not complete and the key is ⛔ UNBOUND.
     - ⛔ **THE MECHANISM, MEASURED — ⛔ do not re-derive it:** `UInputMappingContext::MapKey` is a pure `DefaultKeyMappings.Mappings.Add_GetRef(...)` + a control-mapping rebuild request. ⛔ **It calls neither `Modify()` nor `MarkPackageDirty()`** ⇒ `is_dirty` reads **`false`** immediately after a *successful* append ⇒ **`EditorAssetLibrary.save_asset`, whose `only_if_is_dirty` defaults to `True`, ⛔ NO-OPS AND RETURNS `True` ANYWAY.**
     - ✅ **THE WORKING ROUTE: `imc.modify()` then `save_asset(path, only_if_is_dirty=False)`.** ⚠️ **`imc.mark_package_dirty()` does ⛔ NOT exist on the Python binding (`AttributeError`); `Package.is_dirty()` is ⛔ not exposed either — use MCP `AssetTools.is_dirty` for the query.**
     - ⚖️ **THIS CONDITION IS `§17a` APPLIED TO THIS LANE, AND IT OUTRANKS CONDITIONS 1–4 IN ORDER OF FAILURE: ⛔ conditions 1–4 all interrogate the ARRAY, and TASK-820 measured the array reading a ⛔ PERFECT `27 → 28` while the file on disk was ⛔ byte-identical to its pre-append state.** ⇒ ⛔⛔ **All four proofs can PASS on an append that was never written.** ⭐ **Do condition 5 LAST and treat it as the gate.**
     - 📌 **⛔ NOT a contradiction of TASK-705/747 — both landed their bytes** (TASK-820's pre-read hash `88f7f6e8…332fa1` ⛔ *is* TASK-747's recorded post-append value, so the chain of custody is unbroken). ⚠️ **Whatever dirtied the package for them did ⛔ not happen here** ⇒ ⛔ **never rely on the append path happening to dirty the asset; the hash is the ⛔ only reliable gate.**
- ⛔ **THE ARRAY REWRITE STAYS FORBIDDEN, AND IT IS REACHABLE, WHICH IS WHY THE PROHIBITION IS LOAD-BEARING RATHER THAN THEORETICAL: `DefaultKeyMappings` is `UPROPERTY(config, BlueprintReadOnly, EditAnywhere)` (`InputMappingContext.h:100-101`)** ⇒ **MCP genuinely CAN write the whole struct.** ⛔ **It may not.**
- ⭐⭐ **THE 5.7 DEPRECATION, MEASURED BY TASK-568 AND ✅ CONFIRMED BY THE MANAGER AT THE HEADER — RECORDED SO NO FUTURE BATCH RE-DERIVES IT OR PANICS AT IT: `UInputMappingContext::Mappings` is `UE_DEPRECATED(5.7, "Use the DefaultKeyMappings struct instead.")` and reads `[]`; the live data is `DefaultKeyMappings.Mappings`** (`:93-95` vs `:100-101`). ✅ **`KBD-§` IS UNAFFECTED: `GetMappings()` and `GetMapping(i)` (`:219-220`) BOTH already return the new array** ⇒ **`USiegeKeyboardLayoutSubsystem`'s index-by-index `.Key`-only retarget of its transient duplicate works verbatim, and any APPENDED mapping is covered for free.** ⇒ ⭐ **A letter key added today inherits Dvorak support with ZERO code (`KBD-§4` puts all 26 letters in the table).**

- ⭐⭐ **TWO `unreal.Key` FACTS, MEASURED BY TASK-820 AND RECORDED SO ⛔ NOBODY RE-DERIVES THEM — BOTH DIFFER FROM THE OBVIOUS GUESS, AND THE SECOND ONE MANUFACTURES A PHANTOM DEFECT:**
  1. ⛔ **`unreal.Key(key_name="H")` RAISES `TypeError` — the type takes ⛔ NO constructor arguments** (`call() takes at most 0 arguments (1 given)`; `unreal.Key("H")` fails identically). ✅ **The working route is `k = unreal.Key()` then `k.set_editor_property("key_name", "H")`.** ⚠️ **Attribute access `k.key_name` ⛔ also fails — `key_name` is ⛔ REFLECTION-ONLY.**
  2. ⛔⛔ **`Key.__eq__` IS ⛔ NOT A VALUE COMPARISON FOR A FRESHLY CONSTRUCTED KEY: two *independently constructed* keys both named `B` compare ⛔ `False`, and constructed-`B` vs live-`B` compares ⛔ `False`, while live-vs-live compares `True`.** ⇒ ⛔ **A struct-equality check here proves ⛔ NOTHING and reads exactly like a real defect.**
     - ⭐⭐ **AND THE BEHAVIOUR TO COPY IS ⛔ NOT THE FACT, IT IS THE METHOD: TASK-820 ran a ⛔ CONTROL EXPERIMENT rather than filing the first `False` as a finding** (constructed-vs-constructed, constructed-vs-live, live-vs-live — three comparisons, one conclusion). ⚖️ **`§14`'s standing shape, pointed at an equality operator: the surprising result was evidence about the ⛔ INSTRUMENT, ⛔ not about the asset.** ✅ **The field that actually serializes is `key_name`, and `B`/`Tab` in this very asset were authored by this identical route and work in the shipped game.** ⇒ ⛔ **A `False` from `Key.__eq__` is ⛔ never a blocker; ⛔ never spend a gate on it.**

### KBD-§3. ⛔ THE `UEnhancedInputUserSettings` / `MapPlayerKey` REJECTION — THREE INDEPENDENT REASONS, RECORDED SO IT IS NOT PROPOSED AGAIN

It is the API a competent reader reaches for first, which is exactly why the refusal is written down rather than left to be re-derived.

1. ⛔ **IT REQUIRES `PlayerMappableKeySettings` ON EVERY MAPPING** (`EnhancedInputUserSettings.cpp:1689-1695`) — i.e. **precisely the `IMC_Hero.uasset` edit `KBD-§1` forbids.**
2. ⛔ **AND THAT MEMBER CANNOT BE SUPPLIED AT RUNTIME EVEN IF ONE WANTED TO: it is `protected` with NO public setter** (`EnhancedActionKeyMapping.h:120-132`). ⇒ The asset edit is not a shortcut around the law; it is the **only** way in.
3. ⛔ **IT PERSISTS — `Saved/SaveGames/EnhancedInputUserSettings.sav` — AND IT WINS OVER IMC DEFAULTS.** ⚠️ **A layout-derived remap MUST NOT persist**: switch back to QWERTY and the player is left with stale Dvorak bindings, saved to disk, with no UI to clear them. ⚖️ This one is fatal on its own — the feature is *derived state*, and derived state that outlives its input is a bug generator.
- ✅ **IT REMAINS THE RIGHT HOME FOR A FUTURE REBIND SCREEN.** ⛔ **This is a rejection FOR THIS FEATURE, not a ban on the API** — a task that proposes it for player-authored rebinding is correct and must not be failed by citing this clause.

### KBD-§4. SCOPE LAW — ⛔ LETTERS ONLY, ALL 26, AND THE EXCLUSIONS ARE DELIBERATE

- ✅ **ALL 26 LETTERS `EKeys::A`..`EKeys::Z` ARE IN THE TABLE — not just the 10 the game binds today.** ⚖️ It is free, and it removes the *"update the table when you add a key"* footgun, which is the kind of debt that surfaces as a mystery input bug six months later.
- ⛔ **DELIBERATELY NOT REMAPPED: digits (the `1`–`6` hotkeys), punctuation, modifiers (`Shift`, `LeftAlt`, `Ctrl`), the mouse, `Space`, `Enter`, `Escape`.** ⚠️ **This is a design decision, not an omission** — Dvorak's number row is identical anyway, and positionally remapping digits would actively hurt AZERTY. **A task or QA finding that "the hotkeys weren't remapped" is WRONG and is answered by this clause.**

### KBD-§5. ⛔ THE FAIL-SAFE LAW — EVERY FAILURE MODE DEGRADES TO THE UNTRANSLATED SOURCE CONTEXT

> ### ⛔ **NEVER `nullptr`. NEVER `EKeys::Invalid`. NEVER A PARTIALLY-RETARGETED CONTEXT. THE WORST OUTCOME THIS FEATURE MAY PRODUCE IS *"THE GAME BEHAVES EXACTLY AS IT DID YESTERDAY."***

Every one of these degrades to the pristine `Source` context and logs — and **QA traces each early return individually** (`KBD-§9` criterion 6):

| failure | outcome |
|---|---|
| `Source == nullptr` | return `nullptr` (the caller's existing null-guard already covers it) |
| translation map empty (host is positionally QWERTY) | ⭐ return **`Source` — the SAME POINTER**, no duplicate, no allocation |
| `DuplicateObject` returns null | return `Source`, `UE_LOG(Error)` |
| `RetargetContextKeys` returns false (length mismatch / profile overrides present) | return `Source`, `UE_LOG(Error)`, ⛔ **and the duplicate is discarded, never handed out half-written** |
| probe yields `VirtualKey == 0`, or the resolver yields `EKeys::Invalid`, or two positions claim one target | that letter keeps its **source** key (identity); the map simply has no entry |
| non-Windows (`ProbeActiveLayout` returns 0 probes) | empty translation ⇒ pass-through, one `Log` line at `Initialize` |
| the subsystem itself unresolvable from `GetGameInstance()` | `AHeroCharacter` applies `HeroMappingContext` unchanged — **byte-identical to today's behaviour** |

- ⚠️ **`MappingProfileOverrides` IS THE NON-OBVIOUS ONE AND IT IS WHY `RetargetContextKeys` HAS A FALSE RETURN AT ALL:** profile overrides live in `MappingProfileOverrides` (`InputMappingContext.h:109-110`), **which `GetMapping()` CANNOT REACH.** ⇒ If `Source->GetProfilesWithOverridenMappings().Num() > 0` the retarget would be **silently partial** — so it refuses wholesale instead. ⚖️ *Refusing loudly beats retargeting half a context.*

### KBD-§6. ⛔ THE PROBE IS A WIN32 SCANCODE PROBE, AND THE ENGINE OFFERS NO SHORTCUT — RECORDED SO IT IS NOT "SIMPLIFIED"

- **UE 5.8 exposes NO layout-independent physical key.** `FKeyEvent::GetKeyCode()` (`SlateCore/Public/Input/Events.h:488`) *claims* to be a pre-conversion hardware code and **on Windows carries the layout-dependent VK**; the real scancode is consumed inside `WindowsApplication.cpp` (Shift disambiguation only) and **never propagated.**
- ⭐ **THE SEAM THAT MAKES THE TABLE CORRECT BY CONSTRUCTION: `ResolveKeyFromCodes` IS A THIN WRAPPER ON `FInputKeyManager::Get().GetKeyFromCodes` (`InputCoreTypes.h:857`, module `InputCore` — ⛔ ALREADY A PUBLIC DEPENDENCY, verified at `GitClaudeUnrealTest.Build.cs:15`).** Calling **the engine's own resolver with the same two numbers the message pump supplies** (`WindowsApplication.cpp:3248-3321` → `SlateApplication.cpp:4953-4957`) means the translation table **agrees with runtime by construction**, including on layouts nobody tested.
- ⛔ **DO NOT MASK THE DEAD-KEY BIT `0x80000000`** — `WindowsApplication.cpp:3317` does not, and the table must match runtime byte for byte.
- **`check(IsInGameThread())`** — `GetKeyboardLayout(0)` is per-thread. Non-negotiable.
- ⭐ **THE SELF-CHECK IS PART OF THE FEATURE, NOT A DEBUG EXTRA: on a US-QWERTY host `MapVirtualKeyEx(0x11, MAPVK_VSC_TO_VK_EX, hkl)` MUST return `'W'` (0x57).** ⚖️ **No scancode table exists anywhere in engine source to cross-check against, so that one assertion is the ONLY thing validating our hand-authored table against the OS.** Log it at `Initialize`.
- ✅ **NO `Build.cs` CHANGE. `ApplicationCore` IS NOT NEEDED.** Include `Windows/WindowsHWrapper.h` (what `InputCore/Private/Windows/WindowsPlatformInput.cpp:4` uses; it is in **Core**); `user32.lib` is a UBT default (`UEBuildWindows.cs:2093`). ⛔ **A `Build.cs` edit in this batch is a finding** — it means somebody reached for the wrong header.
- ⛔ **EVERY Win32 SYMBOL LIVES INSIDE `#if PLATFORM_WINDOWS`, CONFINED TO ONE STATIC FUNCTION IN THE `.cpp`.** The **header stays Win32-free** — the HKL is stored as an opaque `uint64`. Fence the poll timer and the activation hook the same way.
- **MID-SESSION SWITCH — WHY THERE ARE THREE MECHANISMS AND NOT ONE:** Windows fires `WM_INPUTLANGCHANGE` and Slate handles it (`SlateApplication.cpp:5138-5141` → `FInputKeyManager::InitKeyMappings()`), but ⛔ **`OnInputLanguageChanged` is a bare virtual with NO delegate — the whole Runtime tree was grepped, zero.** So: (1) re-probe on every `GetPositionalContext` call (free; covers a layout set before launch) · (2) `FSlateApplication::Get().OnApplicationActivationStateChanged()` (`SlateApplication.h:1690-1691`) — covers alt-tab-out/change/alt-tab-in, ⛔ **unbind in `Deinitialize`** · (3) the 1 Hz HKL poll — **the only thing that catches an in-place `Win+Space`.**
- ⭐ **RE-APPLY WITHOUT TOUCHING THE HERO:** on a change, retarget **the cached duplicate IN PLACE** (re-derived from the pristine source, so no compounding) and call `RequestRebuildControlMappingsUsingContext(Dup)`. ⇒ **The pointer `AHeroCharacter` handed to `AddMappingContext` never changes — ZERO new state and ZERO re-application code in `HeroCharacter`.** ⚖️ That property is the reason the hero edit is four lines, and a "refactor" that re-applies contexts from the subsystem throws it away.

### KBD-§7. NAMING + FOLDER LAW (the cross-task contract)

| thing | law |
|---|---|
| statics | **`FSiegeKeyboardLayoutStatics`** — `Source/GitClaudeUnrealTest/Siegebound/SiegeKeyboardLayoutStatics.{h,cpp}`. Plain static library, **not a UObject**, `GITCLAUDEUNREALTEST_API`. Precedent: `FSiegeCombatStatics` (`SiegeCombatStatics.h:23`). ⭐ **ALL pure, testable logic lives here.** |
| probe struct + resolver alias | **`FSiegePositionalKeyProbe`** and **`FSiegeKeyResolver`** share `SiegeKeyboardLayoutStatics.h`. ✅ **This is the existing "pure data types may share a header when they form one concept" exception (`TeamId.h` precedent) — it is NOT a one-class-per-header violation and QA must not flag it as one.** |
| subsystem | **`USiegeKeyboardLayoutSubsystem : UGameInstanceSubsystem`** — `SiegeKeyboardLayoutSubsystem.{h,cpp}`, `GITCLAUDEUNREALTEST_API`. Clones the `USiegeSettingsSubsystem` shape (`SiegeSettingsSubsystem.h:88-89`, `Initialize` at `.cpp:22-34`). |
| log category | **`LogSiegeInputLayout`** — the standing `LogSiege<Domain>` law ("Logging (C++)"). Declared in `SiegeKeyboardLayoutSubsystem.h`, defined in its `.cpp`. |
| delegate | **`FOnSiegeKeyboardLayoutChanged`** / member **`OnKeyboardLayoutChanged`** — the `FOn<Owner><Event>` / `On<Owner><Event>` law ("Delegates (C++)"), matching `FOnSiegeSettingsChanged`/`OnSettingsChanged`. |
| tests | **`Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeKeyboardLayoutTest.cpp`**, `#if WITH_DEV_AUTOMATION_TESTS`, `IMPLEMENT_SIMPLE_AUTOMATION_TEST` with `EAutomationTestFlags::EditorContext \| EAutomationTestFlags::EngineFilter` (the `SiegeSettingsTest.cpp:113-116` pattern). **Test names live under `Siegebound.Input.<Name>`.** |
| ⭐ **CVar (NEW PATTERN — this batch introduces the repo's FIRST console variable; the pattern is added here BEFORE the task issues, per the house rule)** | **`siege.<Domain>.<Thing>`** — lowercase `siege.` prefix mirroring the engine's own `r.` / `net.` / `a.` families, PascalCase after it. **This batch's only CVar: `siege.Input.LayoutPollEnabled`** (int32, **default 1 = ON**, `ECVF_Default`). ⛔ **It is a DEV/TEST lever, never a player-facing setting (`KBD-§0` ruling 1)** — it is not read by any UI and gets no settings row. |
| tunable | **`LayoutPollIntervalSeconds` = `1.0f`** — a named constant, ⛔ never a bare `1.0f` at the timer call. |

⛔ **The `names:` block of each task is the single source of truth (the standing cross-discipline rule). Every symbol above appears there character-for-character.**

### KBD-§8. ⚠️ PINNED CROSS-TASK SIGNATURE REGISTRY (this batch's link contract)

⚠️ **UBT compiles the whole module.** Every task in this batch compiles against this list **character-for-character**; "improving" a pinned signature breaks the link and is an **automatic QA FAIL.** ⛔ **`HeroCharacter.cpp` compiles against `GetPositionalContext`, and the test file compiles against all four statics** — precedent for pinning before dispatch: `:453`, `:564`, `:915`, `:1345`.

```cpp
// ── SiegeKeyboardLayoutStatics.h ──────────────────────────────────────────

/** One physical key position: what QWERTY calls it, and what the ACTIVE layout yields there. */
struct FSiegePositionalKeyProbe
{
    FKey   QwertyKey;        // FKey this position carries on US-QWERTY, e.g. EKeys::W
    uint32 ScanCode   = 0;   // scan-code set 1, e.g. 0x11
    uint32 VirtualKey = 0;   // VK the ACTIVE layout yields here; 0 == probe failed
    uint32 CharCode   = 0;   // MapVirtualKeyEx(VK, MAPVK_VK_TO_CHAR, hkl) — dead-key bit UNMASKED
};

/** The injectable seam that lets every test run on a QWERTY machine. */
using FSiegeKeyResolver = TFunctionRef<FKey(uint32 VirtualKey, uint32 CharCode)>;

class GITCLAUDEUNREALTEST_API FSiegeKeyboardLayoutStatics
{
public:
    static TArray<FSiegePositionalKeyProbe> GetQwertyLetterScanCodes();

    static FKey ResolveKeyFromCodes(uint32 VirtualKey, uint32 CharCode);

    static int32 BuildTranslationMap(const TArray<FSiegePositionalKeyProbe>& Probes,
                                     FSiegeKeyResolver Resolver,
                                     TMap<FKey, FKey>& OutTranslation);

    static bool RetargetContextKeys(const UInputMappingContext* Source,
                                    UInputMappingContext* Target,
                                    const TMap<FKey, FKey>& Translation,
                                    int32& OutNumRetargeted);
};

// ── SiegeKeyboardLayoutSubsystem.h ────────────────────────────────────────
DECLARE_LOG_CATEGORY_EXTERN(LogSiegeInputLayout, Log, All);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSiegeKeyboardLayoutChanged);

UCLASS()
class GITCLAUDEUNREALTEST_API USiegeKeyboardLayoutSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    /** THE ONE CALL-SITE CONTRACT. Never null-for-non-null-input; degrades to Source. */
    const UInputMappingContext* GetPositionalContext(const UInputMappingContext* Source);

    UFUNCTION(BlueprintPure,     Category = "Siegebound|Input") bool IsPositionalRemapActive() const;
    UFUNCTION(BlueprintPure,     Category = "Siegebound|Input") FString DescribeActiveTranslation() const;
    UFUNCTION(BlueprintCallable, Category = "Siegebound|Input") void RefreshKeyboardLayout();

    /** ⭐ ADDED 2026-08-04 (batch ASSISTANT-EXCLUDE, TASK-516). THE SINGLE-KEY QUERY. */
    UFUNCTION(BlueprintPure,     Category = "Siegebound|Input") FKey GetPositionalKey(const FKey& QwertyKey) const;

    UPROPERTY(BlueprintAssignable, Category = "Siegebound|Input") FOnSiegeKeyboardLayoutChanged OnKeyboardLayoutChanged;

    /** ⛔ Automation tests ONLY. Mirrors SiegeSettingsSubsystem::SetSlotNameForAutomationTests (.h:175). */
    void SetTranslationMapForAutomationTests(const TMap<FKey, FKey>& InTranslation);

    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

private:
    /** HKL as an opaque uint64 — this is what keeps the header Win32-free. */
    uint64 CachedLayoutHandle = 0;

    UPROPERTY(Transient)
    TMap<TObjectPtr<const UInputMappingContext>, TObjectPtr<UInputMappingContext>> PositionalContexts;

    TMap<FKey, FKey> TranslationMap;
};
```

- ⛔ **`GetPositionalContext` IS DELIBERATELY *NOT* A `UFUNCTION`, AND THAT IS NOT AN OVERSIGHT.** UHT rejects a `const UObject*` **return** type on a reflected function. ⚠️ **A well-meaning "expose it to Blueprint" edit does not fail review — it fails UHT, loudly, at the batch's only compile gate.** Say so in the header.
- ⛔ **`IsPositionalRemapActive()` MEANS `TranslationMap.Num() > 0`** — i.e. *"the host layout is not positionally QWERTY."* It does **not** mean *"a duplicate exists"*. ⚖️ Pinned because the two diverge on the very first call and a test asserting the wrong one passes for the wrong reason.
- **`DescribeActiveTranslation()` format:** `"W → Comma, S → O, D → E"` — `", "` separated, `" → "` between, **source-key order as returned by `GetQwertyLetterScanCodes()`** (i.e. A..Z), identity entries omitted, **empty string when the map is empty.** ⛔ **A byte/format claim about this string is asserted with `TestEqualSensitive`, never `TestEqual` (`SC-§13`).**

⭐ **PIN AMENDED 2026-08-04 — `GetPositionalKey` (batch ASSISTANT-EXCLUDE). THE PIN IS EDITED *BEFORE* THE TASK ISSUES, WHICH IS THE ONLY ORDER THIS REGISTRY EVER PERMITS.**

- **WHY IT EXISTS:** `KBD-§4` puts **all 26 letters** in `TranslationMap` — so the answer to *"what does the physical QWERTY-`Z` position yield on this layout?"* is already computed and stored. ⛔ **But `TranslationMap` is `private` and the class exposes NO single-key query** — only `GetPositionalContext` (whole-IMC) and `DescribeActiveTranslation` (a human-readable dump). A caller that needs **one** key today would have to parse a display string or duplicate the probe, and **both of those are second sources of truth for a mapping this subsystem already owns.** One accessor closes it.
- **THE CONTRACT, and it is `KBD-§5`'s fail-safe table applied to a scalar:**
  - **Returns the `FKey` the ACTIVE layout yields at `QwertyKey`'s physical position.** US-QWERTY host ⇒ returns `QwertyKey` (identity is never *stored*, so "no entry" and "identity" are the same answer and both return the input).
  - ⛔ **IT NEVER RETURNS `EKeys::Invalid` AND NEVER RETURNS AN INVALID `FKey` FOR A VALID INPUT.** Absent entry ⇒ the input, unchanged. ⚠️ **An invalid input returns that same invalid input** — the function is a *lookup*, not a validator, and it must not start refusing keys the caller already holds.
  - ⛔ **`const`, and therefore it does NOT re-probe.** ⚠️ **`RefreshKeyboardLayout()` is what makes the map current** (`KBD-§6`'s three mechanisms). ⇒ **RULED: the caller refreshes; the accessor reads.** `USiegeAssistantConsoleWidget::OpenConsole()` calls `RefreshKeyboardLayout()` **once per open** — one probe per console open is free by `KBD-§6`'s own standard (`GetPositionalContext` re-probes on *every* call) and it makes the accept key correct **even when `siege.Input.LayoutPollEnabled` has been turned off for testing.** ⛔ **Do NOT bind `OnKeyboardLayoutChanged` from a widget** — that is new lifetime state to unbind wrongly, for a value that is re-read at every open anyway.
- ⚠️ **DIRECTION, STATED BECAUSE GETTING IT BACKWARDS COMPILES AND SILENTLY BINDS THE WRONG KEY:** the map is **source (QWERTY) → what the active layout yields at that position.** On US-Dvorak `GetPositionalKey(EKeys::Z)` returns **`EKeys::Semicolon`**, because the physical position QWERTY calls `Z` produces `;` on Dvorak. ⇒ **A key-press comparison is `InKeyEvent.GetKey() == GetPositionalKey(EKeys::Z)`, never the reverse lookup.**
- ⛔ **AND THE PLAYER-FACING STRING DOES *NOT* USE THIS FUNCTION. ANY PROMPT THAT NAMES THE ACCEPT KEY SAYS `Z`, ON EVERY LAYOUT.** ⚖️ **That is `KBD-§0` ruling 1 restated, not an inconsistency:** the whole feature exists so *"the keys go where they are printed on a QWERTY reference"* — the player is on QWERTY **hardware** with a Dvorak **software** layout, so their keycap says `Z` and telling them to *"press `;`"* would be the bug, not the fix. **The lookup is for the comparison; the literal is for the human.**

### KBD-§9. THE QA GATE — ⛔ `.claude/pipeline/qa/TASK-513-keyboard-layout.md`, AND IT NAMES 509 · 510 · 511 · 512

⚠️ **NAMING DEPARTURE, DECLARED (`SC-§15`): the plan proposed `qa/TASK-509-keyboard-layout.md`. ⛔ SUPERSEDED — the gate file is named for the GATE task**, matching `qa/TASK-506.md` (which gates TASK-505). ⚖️ Reason: `qa/TASK-464.md` was already nearly mis-cited as a gate on TASK-438 because its number resembled its subject (`SC-§29`); naming a gate after a task it does not belong to manufactures that trap. **Nobody greps for `TASK-509-keyboard-layout.md`; it will not exist.**

**Criteria 1–9 are carried VERBATIM from the approved plan's "QA gate criteria" section:**

1. ⛔ **No path writes to `IMC_Hero.uasset`** — grep the diff for any non-const use of the *source* IMC. *(TASK-445.)*
2. ⛔ **No code writes `Modifiers`, `Triggers`, or the mappings array — only `.Key`.** `MapKey`/`UnmapKey`/`UnmapAll`/`Add`/`RemoveAt`/`Empty` must not appear on any IMC on a shipped path. *(TASK-399/445 failure class.)*
3. Complete-type include law on `HeroCharacter.cpp`. *(TASK-110.)*
4. No shadowing of inherited reflected members (C4457/C4458).
5. Most-vexing-parse scan. *(TASK-416.)*
6. Every failure mode degrades to the untranslated source context — never null, never `EKeys::Invalid`. Trace each early return.
7. `#if PLATFORM_WINDOWS` encloses every Win32 symbol; non-Windows compiles and is pass-through.
8. `TestEqualSensitive`, not `TestEqual`, for FString claims. *(§13.)*
9. The tests genuinely run without Dvorak hardware — confirm the injected-resolver seam has no hidden dependence on the host layout.

**Criteria 10–13 are MANAGER ADDITIONS (added, never substituted — the nine above are unaltered):**

10. ⛔ **GUARD PLACEMENT, not guard presence (`SC-§21`): the `GetPositionalContext` resolve sits INSIDE the existing `LocalPlayer`/`EnhancedInputLocalPlayerSubsystem` guard chain in `NotifyControllerChanged`.** ⚠️ **That function runs on the server for a remote client's pawn too** — a probe hoisted above the guard would run a layout probe for a machine that is not there. **"The guard is present" is not the review; *where* it sits is.**
11. ⛔ **THE M8 DECLARATION IS PRESENT VERBATIM** in every code task's handoff and in both new headers: *"adds no replicated property, no new replicated class, no new relevancy tier."* **"Tier not declared" is a QA FAIL** (the standing M8 declaration duty).
12. **Pinned-registry conformance, character-for-character against `KBD-§8`** — every signature, the CVar name `siege.Input.LayoutPollEnabled`, `check(IsInGameThread())` present in the Win32 probe, the activation hook **unbound in `Deinitialize`**, and **no `Build.cs` change** (`KBD-§6`).
13. ⛔ **`KBD-§0` SCOPE: no `USiegeSettingsSaveGame` field, no settings-menu row, no digit/modifier/mouse remap.** ⚠️ **A settings toggle is a scope breach EVEN IF THE REVIEWER AGREES WITH IT** — it is Jonathan's decided ruling (the `AS-§6`/`Escape` precedent, same shape).

⛔ **`SC-§27b` — A COMPILE-GATE PASS ATTACHES TO A COMMIT, NEVER TO A LANE. ✅ THIS BATCH IS ORDERED SO THAT IS SATISFIED STRUCTURALLY: all four code tasks land BEFORE the single gate, and the single gate precedes the single compile+commit.** ⇒ **No second verdict is owed** — *unless* the gate fails and a fix lands, in which case `SC-§27`'s **diff-scoped** verdict applies (the fix's changed lines + the fences it must not have disturbed, ⛔ never a re-litigation of the passed design).

### KBD-§10. M8 DECLARATION — STATED, BECAUSE *"THERE IS NOTHING TO DECLARE"* ONLY COUNTS WHEN IT IS STATED

⛔ **THIS FEATURE ADDS NO REPLICATED PROPERTY, NO NEW REPLICATED CLASS, AND NO NEW RELEVANCY TIER.**

- ✅ **AND THE REASON IS STRUCTURAL, NOT INCIDENTAL: `USiegeKeyboardLayoutSubsystem` IS A `UGameInstanceSubsystem` — one per client process, client-local by construction.** In a listen-server match the host and the joining client each probe **their own** OS layout, which is the correct behaviour and the only possible one. **The duplicate IMC lives in the transient package and is never seen by the network.**
- ⚠️ **The `AHeroCharacter` edit sits inside the local-player guard chain (`KBD-§9` criterion 10), so nothing about it executes for a non-local pawn.**

### KBD-§11. ⚠️ KNOWN LIMITATIONS — STATED UP FRONT, ⛔ NONE OF THEM ARE BUGS

Carried from the plan so a playtest report does not spend a QA loop on a designed outcome.

- ⚠️ **The `~` console key MOVES.** `InputSettings.cpp:173-195` picks it by CULTURE (`VK_OEM_7`/`5`/`3`) and this feature does not touch it — it lands wherever Dvorak puts it.
- ⚠️ **Digits and punctuation are deliberately not remapped** (`KBD-§4`).
- ⚠️ **Windows only.** macOS (`TISCopyCurrentKeyboardLayoutInputSource`) and Linux (XKB) need their own probes; **pass-through there is CORRECT, not broken.**
- ⚠️ **Non-Latin layouts (Cyrillic/Greek) come out identity-equivalent** — Windows keeps `VK_A`..`VK_Z` at QWERTY positions for them and `GetKeyFromCodes` synthesizes a matching FKey (`InputCoreTypes.cpp:1603-1609`). **Correct outcome; recorded so it is not reported as a miss.**
- ⚠️ **A future rebind screen will display `"Comma"`, not `"W"`.** `DescribeActiveTranslation()` exists so a settings row can eventually read *"Key positions: QWERTY (layout: US-Dvorak)"* — ⛔ **that is a FUTURE task and is not in this batch's scope** (`KBD-§0` ruling 1).

