# TASK-1463 — [STALE-TASK-1314-CLAIM-REPAIR] — programmer handoff

marker `TASK-1463-STALE-TASK-1314-CLAIM-REPAIR` · 2026-09-24 · gameplay-programmer
Status → `ready-for-qa` → **`TASK-1464`** · rides the existing 5a (`TASK-1409`/`1411`/`1415`) · **no 5b** · commit host `TASK-1414`

---

## 0. One-line outcome

A shipped `UE_LOG` warning asserted two things that are false — that `Btn_Jump`'s `IsFocusable` is `False` in `WBP_VictoryScreen`, and that `TASK-1314` Route K-2 is owed. **I re-measured both on three independent limbs before touching a byte**, confirmed `TASK-1461`, and corrected the literal, the comment above it and the `TASK-1398` §2.1 row 7. **The guard and its load-bearing justification are byte-identical.**

---

## 1. Re-measurement — I took **all three** limbs, not the required two

I did **not** act on `TASK-1461`'s report (`SC-§138`). Every limb below was re-taken in this row.

### Limb (a) — live read, read-only asset inspector (editor **UP**, `get_headless_status` = `editor_connected`)

| object path | `IsFocusable` |
|---|---|
| `/Game/UI/WBP_VictoryScreen.WBP_VictoryScreen:WidgetTree.Btn_Jump` | **`True`** |
| `/Game/UI/WBP_VictoryScreen.WBP_VictoryScreen_C:WidgetTree.Btn_Jump` | **`True`** |
| `UButton` engine class default | `True` |

⭐ **The live reader was given its own positive control** — the same call, same property, on the two assets `TASK-1461` measured as genuinely opted out:

| control object path | `IsFocusable` |
|---|---|
| `/Game/UI/WBP_HUD.WBP_HUD:WidgetTree.Btn_Jump` | **`False`** |
| `/Game/UI/WBP_CardHand.WBP_CardHand:WidgetTree.Btn_Jump` | **`False`** |

⇒ the instrument **discriminates**; the `True` on the victory screen is a reading, not an instrument default.

### 🚨 Limb (a) carries a CDO-echo trap, and I nearly logged it as a contradiction

`get_asset_meta` on `WBP_VictoryScreen` prints, in its **Default Property Values** block:

```
uint8 bIsFocusable = False
```

⛔ **That is `WBP_VictoryScreen`'s own `UUserWidget` property — NOT `Btn_Jump`'s.** Confirmed directly: the generated-class CDO (`class=UserWidget`) reads `bIsFocusable=False`, while the widget-tree `Button` archetype reads `True`. The two are different objects and different properties that print under the same name. A reader who greps that block for `bIsFocusable` and stops there **reproduces the exact false conclusion this row exists to repair.** This is the trap already routed as `TASK-1461` output (2) → `TASK-1455` (2e) / `VER-§12`; this row is a **second, independent sighting of it in the wild**, and it is why I did not let limb (a) stand on the `get_asset_meta` summary alone.

### Limb (b) — byte detector on the current package, **with clause (1b) closed first**

```
Content/UI/WBP_VictoryScreen.uasset   153825 bytes
header: 301 203 * 236 ...   (UE package magic 0xC1832A9E — NOT "version https")
sha256: a26ad9a09d332ccd8a29df6a87d22a878b3bfd531d3199d934074addedc4b40b
```

⭐ **Clause (1b) is closed twice over.** The size is **153,825 B**, the expected post-commit LFS payload — three digits would have meant a pointer and a void limb. Stronger still: the working-tree **sha256 is byte-identical to the `oid` in `1d433ca`'s LFS pointer**, so the file on disk is provably *that* commit's payload, not merely "some smudged file of about the right size".

Same matcher, same asset, before vs after:

| payload | size | `IsFocusable` | `Btn_Jump` |
|---|---|---|---|
| **BEFORE** `7817dd7f…` (`1d433ca^`, from local LFS cache) | 153,489 B | **1 hit — offset `4474`** | 5 |
| **AFTER** `a26ad9a0…` (current working tree) | 153,825 B | **0 hits** | 5 |

⭐ This is a **controlled negative, not a bare zero** (`SC-§137`). The same matcher returns **1** on the before-blob at the exact offset `TASK-1461` reported, and `Btn_Jump` scores **5 in both** — so the name table is demonstrably readable in the *after* file specifically. The zero is a property of the asset, not of the instrument. Additional positive controls: `WBP_HUD.uasset` (881,516 B) and `WBP_CardHand.uasset` (747,869 B) each return **1**.

UE serialises a tagged property only when it differs from the class default, and `UButton`'s default is `true` (`Button.cpp:48`) ⇒ absence **is** `true`, which limb (a) then confirms directly.

### Limb (c) — the commit

```
1d433ca440519c6811b685ba3258c9a68c3f662f   Sun Sep 20 01:46:26 2026 -0700   Jonathan Wesely
TASK-1314: Play Again is keyboard-reachable at a real match end …
 .../Content/UI/WBP_VictoryScreen.uasset  |  4 +-
 9 files changed, 1011 insertions(+), 14 deletions(-)
```

LFS pointer moved `7817dd7f…`/153489 → `a26ad9a0…`/153825 across that commit.

⚠️ **`SC-§102` bit me and is worth re-recording:** `git show 1d433ca:Content/UI/…` failed with *"exists, but not"* — the git root is one level up. A mis-anchored pathspec here does not answer "absent", it answers with a **fatal that is easy to skim past as a negative result**. Re-anchored to `1d433ca:./Content/UI/…`.

### Verdict

**All three limbs agree. `TASK-1461` is confirmed.** Both clauses of the shipped literal were false.

---

## 2. Surface one — the shipped log literal (the headline)

**OLD, in full:**

```cpp
TEXT("ASiegePlayerController '%s': victory screen button '%s' does not support keyboard focus (IsFocusable is False on it in WBP_VictoryScreen) — no focus target set, Play Again stays mouse-only (TASK-1314 Route K-2 is owed)."),
```

**NEW, in full:**

```cpp
TEXT("ASiegePlayerController '%s': victory screen button '%s' resolved, but its Slate widget reports SupportsKeyboardFocus() == false — no focus target set, Play Again stays mouse-only. Check that button's IsFocusable in WBP_VictoryScreen."),
```

Rationale, against spec (2):

- It now reports **the condition actually detected** — `SupportsKeyboardFocus()` returned false on the resolved widget — which is the literal predicate the branch tested, and is true whenever the line prints.
- It **asserts nothing about the backlog.** The backlog moves; the binary does not.
- It states that the button **resolved**, which distinguishes this branch from the sibling lookup-failure `Warning` below it — a real diagnostic gain.
- `IsFocusable` survives only as an **imperative** ("Check that button's…"), which is where `SButton::SupportsKeyboardFocus()`'s `bIsFocusable` is baked from at `RebuildWidget()`. It is actionable guidance, not a claim about current state.
- The dead claim is **not re-typed anywhere**, including in any comment explaining its removal. Provenance lives in this row, `1d433ca` and `TASK-1461`'s handoff.

Verified repo-wide: `Route K-2` now appears **0 times in `Source/`**. The `TASK-1314` mentions in `Source/` stand at **exactly 8**, matching the spec's count of accurate history — deliberately untouched, including the sibling literal that correctly ends `(TASK-1314).` and the existing comment that already reads *"TASK-1314 shipped it at 1d433ca"*.

---

## 3. (2b) Reader census — **ZERO readers**, and here is the denominator

Searched **9,302 files** across `.claude/pipeline/**`, `Tests/**` and `Tools/**`.

| term | hits | verdict |
|---|---|---|
| `Route K-2` | 24 | prose only (board rows + handoffs) |
| `Route K-2 is owed` | 6 | prose only |
| `does not support keyboard focus` | 4 | all in `qa/TASK-1314-verify.md` |
| `stays mouse-only` | 1 | `qa/TASK-1314-verify.md` |
| `IsFocusable is False on it` | 2 | `qa/TASK-1314-verify.md` |

**`Tests/` + `Tools/` hits on every term: `0`.** No test, script or tool greps this literal — nothing executable keys on it.

The four `.claude/pipeline` hits are all in **`qa/TASK-1314-verify.md`**, a closed, dated verify report that **quotes a captured log line as archival evidence** of a 2026-09-20 run (its own table records the count going `1` → `0`, predicted "must drop to 0", ✅). Rewording the source cannot break it: it records what the binary emitted then, not what it emits now. **An archived transcript is not a live discriminator.**

⚠️ One hit matched my "does anything *instruct* a grep of this literal" probe — it is **`TASKBOARD.md:7433`, TASK-1463's own spec row**, describing the hypothetical future reader. Self-reference, not a reader.

⇒ **Nothing breaks. No discriminator is being silently retired.**

---

## 4. Surface two — the comment

**OLD (the stale half), verbatim:**

```cpp
// ⚠️ AS MEASURED 2026-09-19 THE ASSET SAYS False: Btn_Jump carries an authored
// IsFocusable=False that overrides UButton's engine default of true (Button.cpp:48),
// and UButton exposes no runtime setter (InitIsFocusable is constructor-time only,
// Button.h:205-206). So until that ONE property is flipped in the Blueprint editor
// and hand-saved, this resolves, declines, and logs — and the end screen behaves
// exactly as it does today. That asset half is TASK-1314 Route K-2 (SC-§125).
```

**NEW, verbatim:**

```cpp
// ⚠️ THE GUARD RESOLVES TRUE TODAY — AND THAT IS EXACTLY WHY IT STAYS. Btn_Jump once
// carried an authored IsFocusable=False that overrode UButton's engine default of
// true (Button.cpp:48), so the else-branch below was the live path. That property
// was flipped in the Blueprint editor and hand-saved, and 1d433ca shipped the
// asset; the package no longer serialises IsFocusable at all, which is how UE
// records "equal to the default". UButton still exposes no runtime setter
// (InitIsFocusable is constructor-time only, Button.h:205-206), so C++ cannot
// re-assert this if the asset regresses — the guard above is the only backstop.
```

It keeps every fact that is still true (`Button.cpp:48`, `Button.h:205-206`, no runtime setter), replaces the four now-false sentences with the corrected history and the `1d433ca` provenance, and — deliberately — **ends by re-arguing why the guard must survive its own example going stale**, which is the failure mode the manager named.

---

## 5. Clause (4) — **THE GUARD AND ITS JUSTIFICATION ARE UNTOUCHED**

Stated explicitly, as required:

- **`if (PlayAgainSlate->SupportsKeyboardFocus())` is byte-identical.** sha256 of the exact line, working tree vs `HEAD`:
  `0d45f61b1deb54148729fee7cf31a7b700d38ce82b27b634d6c1ff5101601a87` — **identical on both sides.**
- Lines matching the guard in the diff prefixed `+`/`-`: **0** (it appears only as context).
- The **"⛔ THIS GUARD IS LOAD-BEARING, NOT DEFENSIVE PADDING…"** block, including `PlayerController.cpp:6343`, `STRUCTURALLY INCAPABLE`, the `Non-Focusable widget` Error text and `TASK-1311 removed at 1c93610`: lines prefixed `+`/`-` in the diff: **0**. Untouched, word for word.
- Nothing was removed, weakened, simplified or "tidied". I corrected **a claim about the world**, not the code that acts on it.

---

## 6. Diff shape

`SiegePlayerController.cpp` — **9 insertions, 7 deletions** = 6 comment lines → 8 comment lines (net +2), plus one 1-for-1 literal swap.

**Strip-comments diff — every changed line that is not a `//` line:**

```
-					TEXT("ASiegePlayerController '%s': victory screen button '%s' does not support keyboard focus (IsFocusable is False on it in WBP_VictoryScreen) — no focus target set, Play Again stays mouse-only (TASK-1314 Route K-2 is owed)."),
+					TEXT("ASiegePlayerController '%s': victory screen button '%s' resolved, but its Slate widget reports SupportsKeyboardFocus() == false — no focus target set, Play Again stays mouse-only. Check that button's IsFocusable in WBP_VictoryScreen."),
```

**That is the entire non-comment diff.** Zero change to any branch, condition, call, declaration or signature. Line endings: the file is 100 % LF (0 CRLF lines) and the edited block is LF — convention preserved; git's `LF will be replaced by CRLF` notice is the repo's pre-existing `autocrlf` behaviour on the whole file, not something this row introduced.

---

## 7. Surface three — `handoffs/TASK-1398-programmer.md` §2.1 row 7 (strike, never delete)

**Exactly one line changed in that file** (`--numstat` = `1 1`, hunk `@@ -81 +81 @@`). The original text remains visible under `~~…~~`:

```
~~⚠️ **AN ATTEMPT THAT CURRENTLY DECLINES.**~~ `InputMode.SetWidgetToFocus(PlayAgainSlate)` `:2405`,
guarded by `if (PlayAgainSlate->SupportsKeyboardFocus())` `:2403`. ~~⛔ **Measured 2026-09-19:
`Btn_Jump` carries authored `IsFocusable=False`** (`:2397-2402`) ⇒ the guard declines and logs.
**Nothing is focused today**~~ · ⭐ **CORRECTED 2026-09-24
(`TASK-1463-STALE-TASK-1314-CLAIM-REPAIR`, from `TASK-1461`): the asset flip SHIPPED at `1d433ca`
(2026-09-20) — `IsFocusable` is absent from the current package's name table (present at offset 4474
in the pre-commit blob `7817dd7f…`, absent from `a26ad9a0…`), i.e. it now equals `UButton`'s default
`true`. The guard resolves **TRUE** and `Btn_Jump` **IS** focused. The struck `:2397-2402` / `:2403` /
`:2405` are as-of this handoff's writing; `TASK-1463` shifted them +2.**
```

I left the struck line numbers intact rather than silently renumbering them (strike, never delete) and declared the +2 shift in the appended correction instead.

---

## 8. ⚠️ A FOURTH STALE SURFACE — FOUND, **NOT** FIXED, FENCED OUT

`handoffs/TASK-1398-programmer.md` **line 216** (section D lane table, **outside §2.1 row 7**) still reads:

> `Victory additionally needs `Btn_Jump`'s authored `IsFocusable=False` flipped in the Blueprint editor and hand-saved (`SiegePlayerController.cpp:2397-2402`) — ⛔ **an art/asset row, not a code row**`

This is the **same dead claim**, in the same file, in a different section. My `names:` fence is *"§2.1 row 7 strike-and-append **ONLY**"*, so I did **not** touch it. The manager's brief said "three surfaces + one handoff"; the count is **four surfaces** if this one is in scope for anybody. It is stale in both clauses (the flip is not "needed", and the cited `:2397-2402` now points at my replacement comment). ⇒ **Escalating rather than widening. Needs a one-line board decision: strike it under this marker, or board it.**

---

## 9. Fences honoured

- ⛔ No compile, no build, no Live Coding. Rides the existing 5a convoy; **no second build opened.**
- ⛔ No PIE. No editor lifecycle action. Editor left exactly as found (`editor_connected`).
- ⛔ No `.uasset` written. **Dirty-package set measured `0` after all inspection** — every MCP call was `get_headless_status`, `get_asset_meta`, or `execute_unreal_python_readonly`.
- ⛔ No git write of any kind (`git show` / `git diff` / `git lfs` reads only). No commit, no stage.
- ⛔ `SiegeMenuInputSubsystem.{cpp,h}` and `SettingsMenuWidget.{cpp,h}` **never opened** (`TASK-1409` / `TASK-1415` fences).
- ⛔ No `CONVENTIONS.md`. No other row's `status:`.
- Files written: `SiegePlayerController.cpp` (comment + one literal), `handoffs/TASK-1398-programmer.md` (row 7 only), this handoff, and this row's `status:`.

---

## Not examined / limitations

1. **I did not observe the new literal at runtime, and it is unreachable by construction.** The only way to print it is to make the guard fail, and the guard now resolves `true` on the one asset that reaches this site. Per spec: **no 5b, and this is not `UNOBSERVABLE`** — an unreachable literal has no runtime criterion to begin with (the `TASK-1411` precedent). The new string's `%s` arity is unchanged (2 args, both `TCHAR*`), so the change is format-compatible by inspection, **not by execution**.
2. **The new text is unverified against a compiler.** No compile was run (fenced). The edit is inside one `TEXT(...)` literal with balanced quotes and no escape sequences introduced; `==` inside the literal is inert text. QA should still eyeball it, and 5a is the real gate.
3. **Limb (a) reads the editor's in-memory asset**, which I confirmed is undirtied — but an in-memory object is one smudge away from disk. Limbs (b)/(c) are the disk-side evidence and they agree, which is why I took all three.
4. **The `1d433ca^` "before" payload came from the local LFS cache**, not from the remote. If that cache object were corrupt the offset-4474 hit would be unreliable — mitigated by its size matching the pointer's declared 153,489 B exactly, and by `Btn_Jump` scoring 5 in it.
5. **I did not audit the other 24 `Route K-2` prose mentions** for staleness beyond classifying them as non-readers; several are board rows that are legitimately historical. Surface (8) above is the one I judged a genuine defect.
6. **`Saved/Logs/` was not swept** for historical emissions of the old string. Old logs will keep the old text; that is correct and unfixable, and is an argument for the repair, not against it.
