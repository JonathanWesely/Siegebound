# TASK-512 — [KBD-4] `HeroCharacter.cpp` — ⛔ THE ONE BEHAVIOURAL EDIT IN THE WHOLE FEATURE

**Agent:** gameplay-programmer · **Status:** `ready-for-qa` · **Date:** 2026-08-04
**QA gate:** **TASK-513** → `.claude/pipeline/qa/TASK-513-keyboard-layout.md` (`SC-§29`)
**Law implemented:** CONVENTIONS `KBD-§5` · `KBD-§6` · **`KBD-§8` (the pin — I am a CONSUMER of it, I changed nothing in it)** · `KBD-§9` criteria **3 + 10** · `KBD-§10` · the complete-type include law (TASK-110) · `SC-§21` (guard placement)
**Upstream:** TASK-511 (`handoffs/TASK-511-programmer.md` §8 — the downstream note written for this task)

## ⛔ M8 DECLARATION (verbatim)

**adds no replicated property, no new replicated class, no new relevancy tier.**

✅ **Plus the clause `KBD-§10` requires of this task specifically: the edit sits inside the
local-player guard chain, so nothing about it executes for a non-local pawn.** No `DOREPLIFETIME`
line was added or moved; `AHeroCharacter::GetLifetimeReplicatedProps` is untouched and still
registers exactly one property (`Team`).

---

## 1. Files touched — EXACTLY ONE

| file | status |
|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/HeroCharacter.cpp` | **modified** — 2 includes + 1 function body |

⛔ **`HeroCharacter.h` IS UNTOUCHED, AND IT DID NOT NEED TO BE.** Everything the edit uses already
exists there: `HeroMappingContext` (`HeroCharacter.h:390`, `TObjectPtr<UInputMappingContext>`) and
the `NotifyControllerChanged` override declaration (`:314`). **No member was added, no member was
removed, no signature changed.**

⛔ **Nothing else was opened for edit.** ⛔ Did NOT touch `SiegeKeyboardLayoutStatics.{h,cpp}`,
`SiegeKeyboardLayoutSubsystem.{h,cpp}` or `Tests/SiegeKeyboardLayoutTest.cpp` — all three frozen and
`ready-for-qa`. ⛔ **No `Build.cs` change** (`EnhancedInput`/`InputCore` already public deps; the two
new includes are Engine + own-module). ⛔ **`SetupPlayerInputComponent` NOT touched**, no
reformatting, no "tidying" of neighbouring code. ⛔ **No compile, no Git, no editor/MCP/PIE.**

**Exclusive-ownership check (the task's `parallel-safe: no` clause):** the two lanes holding
uncommitted game-module code per the board's quiet-module pre-flight are TASK-479 (Stage-A,
`SiegeAssistantSubsystem`) and TASK-505 (`SiegeAssistantConsoleWidget.{h,cpp}`). **Neither names
`HeroCharacter.cpp`**; no other lane holds it. ⚠️ **Stated as a BOARD read, not a working-tree
read — I ran no git command** (the dispatch forbids Git). **TASK-514 re-verifies the tree with the
instrument, as its own pre-flight (a) already requires.**

**Board:** TASK-512 set `in-progress` at start, `ready-for-qa` at finish. Nothing else on the board
was edited.

---

## 2. ⭐ THE EXACT DIFF — all of it

### 2a. The two includes (`KBD-§9` criterion 3 — the TASK-110 class)

```diff
 #include "Engine/DataTable.h"
+#include "Engine/GameInstance.h" // TASK-512: complete type for GetGameInstance()->GetSubsystem<>() (Actor.h:3772 forward-declares UGameInstance)
 #include "Engine/LocalPlayer.h"
```

```diff
 #include "Siegebound/SiegeHitFlashComponent.h"
+#include "Siegebound/SiegeKeyboardLayoutSubsystem.h" // TASK-512: USiegeKeyboardLayoutSubsystem::GetPositionalContext — the positional remap's ONE call site
 #include "Siegebound/SiegeNavAreas.h" // TASK-349: team object channel for the capsule stamp
```

⛔ **BOTH ARE REQUIRED AND NEITHER IS RELIED ON TRANSITIVELY** — verified against the installed
UE 5.8 tree, not assumed:

| include | the symbol that forces it | verified at |
|---|---|---|
| `Engine/GameInstance.h` | `AActor::GetGameInstance()` returns `class UGameInstance*` — **forward-declared in the return type itself** (`ENGINE_API class UGameInstance* GetGameInstance() const;`). I then call the **member template** `GetSubsystem<>` **on that pointer**, which requires the complete type. | `GameFramework/Actor.h:3772`; the template at `Engine/GameInstance.h:439-443` |
| `Siegebound/SiegeKeyboardLayoutSubsystem.h` | `USiegeKeyboardLayoutSubsystem` is named as the template argument (`::StaticClass()` is instantiated) **and** dereferenced for `GetPositionalContext`. | own module |

**Placement:** both inserted in the file's existing alphabetical order, in the block they belong to
(`Engine/…` and `Siegebound/…` respectively), with the file's existing `// TASK-###: reason`
trailing-comment style. **The include block is otherwise byte-identical.**

### 2b. The body — `AHeroCharacter::NotifyControllerChanged`, the only function touched

```diff
 				if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer))
 				{
-					Subsystem->AddMappingContext(HeroMappingContext, HeroMappingContextPriority);
+					// ─── POSITIONAL KEYBOARD LAYOUT (TASK-512, KEYBOARD-LAYOUT batch; CONVENTIONS `KBD-§5`/`KBD-§6`) ───
+					// On Dvorak the FKey Windows delivers is LAYOUT-dependent, so every letter binding in
+					// IMC_Hero (WASD, Q rally, T/R/E/F/C) lands on the wrong PHYSICAL key. The subsystem hands
+					// back a transient duplicate whose `.Key` fields are retargeted to the active layout — or,
+					// on a positionally-QWERTY host, THE SAME POINTER, with no duplicate and no allocation.
+					//
+					// ⛔ THE PLACEMENT IS PART OF THE CORRECTNESS, NOT A STYLE CHOICE (`SC-§21`; `KBD-§9`
+					// criterion 10). This resolve sits in the INNERMOST scope of the existing guard chain —
+					// HeroMappingContext -> APlayerController -> ULocalPlayer -> UEnhancedInputLocalPlayerSubsystem —
+					// beside the AddMappingContext call it feeds. NotifyControllerChanged ALSO RUNS ON THE
+					// SERVER FOR A REMOTE CLIENT'S PAWN; hoisting this above the GetLocalPlayer() check would
+					// probe an OS keyboard layout on behalf of a machine that is not there.
+					//
+					// ⛔ FAIL-SAFE (`KBD-§5`, last row): no GameInstance or no subsystem => ContextToApply stays
+					// HeroMappingContext and the behaviour is byte-identical to before this feature existed.
+					// GetPositionalContext never returns null for a non-null input, and this code does not
+					// depend on that trust — a null would simply fail AddMappingContext's own guard, so no
+					// redundant branch is added here that would hide a contract violation.
+					//
+					// ⛔ NO NEW STATE AND NO RE-APPLICATION PATH, DELIBERATELY (`KBD-§6`): on a mid-session
+					// Win+Space the subsystem re-targets that cached duplicate IN PLACE and calls
+					// RequestRebuildControlMappingsUsingContext, so the pointer handed over here stays valid
+					// and current. A cached pointer, a tick, or a delegate binding on AHeroCharacter would
+					// throw that property away.
+					const UInputMappingContext* ContextToApply = HeroMappingContext;
+					if (const UGameInstance* GameInstance = GetGameInstance())
+					{
+						if (USiegeKeyboardLayoutSubsystem* LayoutSubsystem = GameInstance->GetSubsystem<USiegeKeyboardLayoutSubsystem>())
+						{
+							ContextToApply = LayoutSubsystem->GetPositionalContext(HeroMappingContext);
+						}
+					}
+
+					Subsystem->AddMappingContext(ContextToApply, HeroMappingContextPriority);
 				}
```

**That is the whole diff.** ⛔ **One line replaced by nine lines of code (plus a comment block);
zero lines removed anywhere else in the file.** The final state is `HeroCharacter.cpp:227-280`.

**Two deviations from the board's literal snippet, both cosmetic and both declared:**

1. The local is named **`LayoutSubsystem`**, not `Layout`. ⚖️ **Deliberate:** the enclosing scope
   already binds a local called `Subsystem` (the Enhanced Input one), and a reader scanning
   `Subsystem` vs `Layout` has to hold two different vocabularies. `LayoutSubsystem` reads
   unambiguously beside it and cannot be mistaken for the input subsystem.
2. The `UGameInstance` local is named **`GameInstance`**, matching the three existing precedents in
   this module (`SettingsMenuWidget.cpp:543`, `SessionMenuWidget.cpp:175`,
   `SiegeAssistantComponent.cpp:2205`).

⛔ **No pinned signature was touched.** `GetPositionalContext(const UInputMappingContext*)` is called
exactly as `KBD-§8` pins it.

---

## 3. ⭐ CRITERION 10 — THE STATEMENT QA ASKED FOR, PLAINLY

> ### ⛔ **THE `GetPositionalContext` RESOLVE SITS INSIDE THE EXISTING `LocalPlayer` GUARD CHAIN — in the innermost block, in the same scope as, and immediately above, the `AddMappingContext` call it feeds.**

The chain, unaltered, and where the new code sits in it:

| depth | guard (pre-existing, unchanged) | line |
|---|---|---|
| 1 | `if (HeroMappingContext)` | `:234` |
| 2 | `if (const APlayerController* PC = Cast<APlayerController>(GetController()))` | `:236` |
| 3 | ⭐ `if (const ULocalPlayer* LocalPlayer = PC->GetLocalPlayer())` | `:238` |
| 4 | `if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<…>(LocalPlayer))` | `:240` |
| — | **← the entire TASK-512 edit lives here, at depth 4** | `:242-275` |

⛔ **Nothing was hoisted. All four guards are byte-identical to their pre-edit text** — the diff in
§2b shows the `if` lines as unchanged context, not as `+`/`-`.

⚠️ **Why it matters, in the failure's own terms:** `NotifyControllerChanged` fires on the **server**
for a remote client's pawn. On the server, that pawn's `PC->GetLocalPlayer()` is **null**, so under
this placement the whole block — including the layout resolve — never runs for it. Hoisted above
depth 3, the server would call `GetPositionalContext`, which re-probes the **server machine's** OS
keyboard layout (mechanism 1 of 3) and can build+cache a duplicate, on behalf of a player who is not
sitting at that machine. The guard is not decoration here; **its position is the whole check.**

---

## 4. ⛔ THE FAIL-SAFE, TRACED — every exit of the new code (`KBD-§9` criterion 6)

| condition | `ContextToApply` | what `AddMappingContext` receives |
|---|---|---|
| `GetGameInstance()` returns null (no world / teardown) | **stays `HeroMappingContext`** | the pristine source — **byte-identical to pre-TASK-512 behaviour** |
| the GameInstance has no `USiegeKeyboardLayoutSubsystem` | **stays `HeroMappingContext`** | ditto |
| subsystem present, host is positionally QWERTY | `GetPositionalContext` returns **`Source` — the same pointer** (`KBD-§5` row 2) | ditto — ⭐ **the common path allocates nothing** |
| subsystem present, host is Dvorak/AZERTY/… | the cached transient duplicate | the retargeted context |
| subsystem present, any internal failure (`DuplicateObject` null, retarget refused) | `Source` again, subsystem logs `Error` | pristine source |
| non-Windows | translation map is always empty ⇒ same-pointer path | pristine source (`KBD-§11`: **correct, not broken**) |

⛔ **There is no path on which this edit can hand `AddMappingContext` something the old code would
not have handed it, other than the retargeted duplicate.** ⛔ **And no redundant null-branch was
added around the return value** — the board's clause (4) forbids it, because a `if (!ContextToApply)
ContextToApply = HeroMappingContext;` would *hide* a `KBD-§5` contract violation instead of letting
it surface. A null there would simply fail `AddMappingContext`'s own internal guard, exactly as a
null `HeroMappingContext` does today.

---

## 5. ⛔ WHAT I DID **NOT** ADD (clause 5, checked against my own diff)

- ⛔ **No new member on `AHeroCharacter`** — `HeroCharacter.h` is untouched. (Clause 5's tripwire:
  *"if you find yourself adding a member, stop."* I did not.)
- ⛔ **No cached `const UInputMappingContext*` member**, no `TObjectPtr` to a duplicate.
- ⛔ **No tick work** — `Tick` untouched; `PrimaryActorTick` untouched.
- ⛔ **No binding to `OnKeyboardLayoutChanged`** and no re-application path. The subsystem re-targets
  its cached duplicate **in place** and calls `RequestRebuildControlMappingsUsingContext`, so the
  pointer handed to `AddMappingContext` here stays valid and current across a mid-session
  `Win+Space` (`KBD-§6`; TASK-511 handoff §8).
- ⛔ **No `RemoveMappingContext` / re-add**, no second `AddMappingContext` call anywhere.
- ⛔ **No `const_cast`, no non-const use of the source IMC** (criterion 1). `ContextToApply` is
  `const UInputMappingContext*` and `HeroMappingContext` is only ever *read* — `AddMappingContext`
  itself takes `const UInputMappingContext*` (`EnhancedInputSubsystemInterface.h:265`, verified), so
  no cast is needed and none is present.
- ⛔ **No settings field, no menu row, no CVar** (criterion 13 / `KBD-§0` ruling 1).
- ⛔ **No `MapKey`/`UnmapKey`/`UnmapAll`, no touch of `Modifiers`/`Triggers`/the mappings array**
  (criterion 2). This file does not write an IMC field at all, on any path.

---

## 6. 🔍 WHAT QA SHOULD SCRUTINISE

| criterion (`KBD-§9`) | where it lands in this file |
|---|---|
| **1** no write to `IMC_Hero.uasset` | `HeroMappingContext` is read-only here; the only new pointer is `const`. Zero `const_cast` in the diff. |
| **2** only `.Key`, no banned API | N/A — this file mutates no IMC. Grep the diff for `MapKey`/`UnmapKey`/`UnmapAll`/`Modifiers`/`Triggers`: **zero hits.** |
| **3** ⭐ **complete-type include law — THIS FILE IS THE ONE CRITERION 3 NAMES** | §2a: both includes added explicitly, each annotated with the symbol that forces it and the engine line that proves the forward declaration. **Nothing relied on transitively.** |
| **4** shadowing of inherited reflected members (C4457/C4458) | Three names introduced: `ContextToApply`, `GameInstance`, `LayoutSubsystem`. ⛔ **None is a member of `AHeroCharacter`, `ACharacter`, `APawn` or `AActor`** — checked specifically against the shadow-prone reflected set (`Owner`, `Instigator`, `Controller`, `PlayerState`, `Role`, `Team`) and against this class's own members (`HeroMappingContext`, `SprintAction`, `AttackAction`, `HitFlashComponent`, …). ⚠️ `GameInstance` is worth a second look and it is clean: the engine's game-instance member is `UWorld::OwningGameInstance`, a different name on a different class, and this module already uses the local name `GameInstance` three times (`SettingsMenuWidget.cpp:543`, `SessionMenuWidget.cpp:175`). ⛔ **`Subsystem` is the PRE-EXISTING local and I did not reuse or shadow it** — my local is `LayoutSubsystem`. |
| **5** most-vexing-parse (TASK-416) | All three new declarations use `=` or the `if`-init `=` form. ⛔ **No `const T Name(Other(x));` anywhere in the diff.** |
| **6** every failure degrades to the source | §4 traces all six. |
| **7** `#if PLATFORM_WINDOWS` fences | N/A — ⛔ **this file contains no platform `#if` and no Win32 symbol, and the edit added none.** The platform detail is entirely behind `GetPositionalContext`. |
| **8** / **9** `TestEqualSensitive` / no Dvorak hardware | N/A — no test file, no `FString` claim in this diff. |
| **10** ⭐ **guard placement** | §3, with the depth table and the pre/post-edit statement. |
| **11** M8 declaration verbatim | top of this file. ⚠️ **Not restated in a header — this task adds no header** (`HeroCharacter.h` untouched, and the batch's two new headers carry it from TASK-509/511). |
| **12** pinned-registry conformance | I am a **consumer** of the pin, not an author of it. `GetPositionalContext(const UInputMappingContext*)` called exactly as pinned; ⛔ **no signature in `KBD-§8` was read as improvable and none was changed.** No `Build.cs` change. |
| **13** `KBD-§0` scope | Nothing player-facing added. No digit/modifier/mouse key is involved — this edit does not name a key at all. |

### Engine facts re-verified against the installed UE 5.8 tree (not taken on trust)

| claim | verified at |
|---|---|
| `AActor::GetGameInstance()` returns a **forward-declared** `class UGameInstance*` ⇒ the include is mandatory | `GameFramework/Actor.h:3771-3772` |
| `UGameInstance::GetSubsystem<T>()` is a **`const` member** returning a **non-const** `T*` ⇒ `const UGameInstance* GameInstance` is legal and the board's snippet compiles as written | `Engine/GameInstance.h:439-443` |
| `AddMappingContext(const UInputMappingContext*, int32, const FModifyContextOptions&)` — already takes a **const** context ⇒ ⛔ **no cast needed and none present** | `EnhancedInputSubsystemInterface.h:264-265` |
| `TObjectPtr<UInputMappingContext>` → `const UInputMappingContext*` is one user-defined conversion (`operator T*() const`) plus a qualification conversion ⇒ `const UInputMappingContext* ContextToApply = HeroMappingContext;` is well-formed, as is passing it to `GetPositionalContext` | `UObject/ObjectPtr.h:722`; `HeroCharacter.h:390` for the member's type |
| `GetPositionalContext` asserts the game thread (via the probe's `check(IsInGameThread())`) and `NotifyControllerChanged` satisfies it | TASK-511 handoff §8 + `SiegeKeyboardLayoutSubsystem.cpp:388` |

⚠️ **My limit, stated plainly: nothing here has been compiled and nothing has been seen on screen.**
**TASK-514 owns the batch's only compile; the Dvorak outcome is TASK-515's and Jonathan's alone.**
A green compile would prove the two includes are sufficient — it would **not** prove a Dvorak
keyboard drives the hero correctly.

---

## 7. ⚠️ ONE THING FOR TASK-513 TO BE AWARE OF — flagged, not buried

**`GetPositionalContext` is called once per possession, and on the SAME frame as `AddMappingContext`.**
That is the design (TASK-511 §8), and mechanism 1's per-call re-probe means a layout chosen **before
the process launched** is caught right here at the first possession. ⚖️ **But note what this file
therefore does NOT cover, so nobody looks for it here:** a layout switched **after** possession is
handled entirely inside the subsystem (mechanisms 2 and 3 + the in-place re-target). **If TASK-515
reports that a mid-session `Win+Space` does not take effect, the investigation belongs in
`SiegeKeyboardLayoutSubsystem.cpp`, not in this file** — `HeroCharacter.cpp` has, by design, nothing
left to do after the first `AddMappingContext`.

## 8. Slack

Posted in **⚙️ Dev & QA** (`C0BF0QZP3CN`, thread_ts `1783116269.740549`), prefix
`⚙️ GAMEPLAY-PROGRAMMER:` — `🧪 TASK-512` at `ready-for-qa`.
