# TASK-1161 — [FOGFLOOR-COMMENT-FIX] — gameplay-programmer handoff

**Date:** 2026-09-08 · **Baseline:** `61702e1` (last **measured** suite `552 / 0`) · **Gate:** WAIVED (board's written grounds (a)–(c)) · **Host:** `TASK-1149`
**Law:** `FOG-§12.1` *as corrected* · `FOG-§12.5a` · `SC-§83` Addendum B · `SC-§97` · `SC-§102`
**Suite delta: `0`.** No declaration, signature, symbol, string literal or statement changed — see § 4. ⛔ That is a **derivation, not a measurement**: I did not compile and did not run the suite (`TASK-1149` hosts both).

---

## 0. What this row is

Three **prose** repairs from `qa/TASK-1148.md` § LOOP 1 — `WARN-7`, `NIT-5`, `NIT-6`. **Zero compiled bytes.** The first one is the row's whole point: `TASK-1149` cl. (4a) sends its author to read `SiegeGraphicsMenuWidget.cpp`'s Shadows-hint comment block **before composing a commit message whose subject is "corrects a false menu string"** — and that block still contained a **second false sentence**, about the same fog, four lines above the strings.

---

## 1. 🚨 WARN-7 — `SiegeGraphicsMenuWidget.cpp` (was `:158-161`, now `:158-180`)

### OLD (verbatim, deleted)

```
	 *  ⛔ It became FALSE the instant AFogVolume::EnforceFogRenderFloor() landed:
	 *  the Fog card's SIEGE fog is now floored from code at ECVF_SetByCode and
	 *  appears at EVERY Shadows level, because its PRESENCE is a gameplay fact and
	 *  not a graphics option (GFX-§12 — its cost may scale, its presence may not).
	 *  ⭐ It stays TRUE of the world's AMBIENT height fog, which nothing floors
```

**Why it was false — two independent falsehoods in one sentence, which is why the gate rated it WARN and not NIT:**

1. **The mechanism.** The floor does **not** reach the card's fog. `EnforceFogRenderFloor()` pins `r.VolumetricFog` + the two froxel-grid cvars at `ECVF_SetByCode` (`FogVolume.cpp:1004`, `:1010-1011`) and forces the height-fog component's own `bEnableVolumetricFog` on (`:1016-1019`) — **every one of those six terms is froxel/ambient**. `BP_SiegeFog` is a raymarched translucent mesh outside the froxel grid (`TASK-1151`, on pixels + a node census), so none of them touches it. The **same comment block already said this correctly** fifteen lines below, at what was `:177-183`.
2. **The claim about the old string.** *"It became FALSE the instant the floor landed"* — the old string (`"Also drives volumetric fog … Currently: fog ON."`) was a claim about `r.VolumetricFog`, i.e. the **ambient** fog. It was true of that fog and **never governed the card's**, so nothing the floor did could falsify it.

### NEW (verbatim, `:158-180`)

```
	 *  ⛔⛔ CORRECTED 2026-09-08 (TASK-1161, from TASK-1148 WARN-7). THESE LINES USED
	 *  TO SAY that the old text "became FALSE the instant EnforceFogRenderFloor()
	 *  landed, because the Fog card's SIEGE fog is now floored from code at
	 *  ECVF_SetByCode and appears at EVERY Shadows level". ⛔ THAT WAS FALSE TWICE:
	 *    (a) ⛔ THE FLOOR DOES NOT REACH THE CARD'S FOG. It pins r.VolumetricFog and
	 *        the two froxel-grid cvars at ECVF_SetByCode and forces the height-fog
	 *        component's own bEnableVolumetricFog on (FogVolume.cpp:1004-1019) ⇒ what
	 *        it holds up is the world's AMBIENT ExponentialHeightFog. BP_SiegeFog is a
	 *        raymarched translucent mesh OUTSIDE the froxel grid, so no variable in
	 *        that set governs it — which is exactly what the "What IS established"
	 *        paragraph at :196-202 below says correctly, a few lines further down.
	 *    (b) ⛔ The old text never governed the card's fog in the first place, so
	 *        nothing the floor did could have falsified it.
	 *  ⭐ WHAT WAS ACTUALLY WRONG WITH THE OLD TEXT IS ITS NOUN: "volumetric fog",
	 *  unqualified, on a panel a player reaches having just watched a Fog card's wash
	 *  roll over the field — he reads it as the fog HE CAN SEE and concludes that
	 *  Shadows=Low will delete it. This is a SCOPING repair, not the repair of a lie.
	 *  ⛔⛔ AND THE CARD'S HALF IS STILL OPEN: no measured route deletes BP_SiegeFog
	 *  (FOG-§12.1 AS CORRECTED; TASK-1147's 28-package scan, ruled sound by TASK-1148)
	 *  and no frame of this game has ever been captured at Shadows=Low with a card up
	 *  (TASK-1160) ⇒ the reported exploit is UNEXPLAINED — ⛔ NOT confirmed, ⛔ NOT
	 *  refuted, ⛔ NOT CLOSED. ⛔ NO COMMIT MESSAGE, ROW, HANDOFF OR SLACK POST MAY
	 *  SOURCE A "the exploit is fixed" SENTENCE FROM THIS BLOCK.
	 *  ⭐ THE OLD TEXT stays TRUE of the world's AMBIENT height fog, which nothing
```

**Three things this rewrite does deliberately:**

- It **keeps the re-wording justified** — the old string really did need changing — but relocates the reason from *"it became false"* to *"its noun was unscoped"*. That is the true reason, and it is the one that still holds after `TASK-1160` reports, whichever way it comes out.
- It **states the FOG-§12.1 position in the file**, so a commit author who reads this block reaches *UNEXPLAINED — not confirmed, not refuted, not closed* rather than the old block's *"floored … appears at EVERY Shadows level"*. This is the `TASK-1149` cl. (4a) hazard, closed at the source.
- One **word-only** change outside the four lines: `:181` `⭐ It stays TRUE…` → `⭐ THE OLD TEXT stays TRUE…`. The pronoun's antecedent was the old text, and my inserted paragraphs put nineteen lines between them. Declared here because it is outside the gate's quoted range.

⛔ **The two user-facing hint strings are BYTE-IDENTICAL** (`ShadowHintFogOn` / `ShadowHintFogOff`, now `:224` / `:227`) — proven in § 4.

---

## 2. NIT-5 — `FogVolume.cpp`, the component half of `ReleaseFogRenderFloor()` (`:1114-1126`; the new comment is `:1116-1121`)

The gate ruled the teardown-time component restore **safe as reasoned**, but the reasoning lived only in `handoffs/TASK-1147-programmer.md`. It now lives at the declaration a reader actually meets:

```
	if (Prior.bHeightFogFound)
	{
		// ⛔ AND WHY THIS IS SAFE ON THE TEARDOWN PATH, WRITTEN HERE RATHER THAN ONLY IN A HANDOFF
		// (TASK-1148 NIT-5): reached from EndPlay, the AExponentialHeightFog may ALREADY have run its
		// own EndPlay. TActorIterator skips pending-kill actors, so the lookup below simply returns
		// nullptr, and SetVolumetricFog on an unregistered component is harmless ⇒ the worst case is a
		// SKIPPED restore of a component whose world is dying, which costs nothing. ⭐ On today's
		// L_Arena it is a no-op either way (authored `true` == captured `true`).
		if (UExponentialHeightFogComponent* const HeightFog = FindHeightFogComponent(GetWorld()))
```

Both mechanical claims were **read back before I wrote them**, not recalled: `FindHeightFogComponent` is a `TActorIterator<AExponentialHeightFog>` with an `IsValid()` filter (`FogVolume.cpp:854-877`), and the write is the plain `HeightFog->SetVolumetricFog(...)` at `:1124`.

---

## 3. NIT-6 — `handoffs/TASK-1147-programmer.md` § L1.2, M26

**Struck in place, not deleted** (the `TASK-1083` precedent). Old line, now struck:

> ~~⇒ **RED: ≥ 2 rows** — the *"TEARDOWN LETS THE FLOOR GO"* row (`1` → `0`) and *"…and never calls the reconciler…"* (`:1250-1251`, `0` → `1`). **BOTH BRANCHES:** ⛔ not equal on either.~~

Replaced with a `SC-§83` Addendum B prediction: **`RED: ≥ 4 rows, including` …** the two originally named rows **plus** M25's row 1 (whole-file release count `2` → `1`, `SiegeFogVisualTest.cpp:1219-1220`) and M25's row 3 (the pairing row, `2 == 2` → `1` vs `2`, `:1226-1228`) — both red because **replacing the teardown release removes a release call**. The block also carries an explicit *struck by name* list of the rows that are **equal on both branches** (whole-file enforce `:1217-1218`, `EndPlay`'s `DestroyFogVisual();` `:1242-1243`, `EndPlay`'s enforce-refusal `:1248-1249`, both `RefreshBody` rows `:1258-1261`, the ordering rows, and the three-exits loop `:531-548`).

⚠️ **A note on the finding's wording:** the gate reported the handoff as saying *"RED 2 rows"*, an exact count. On disk it read *"RED: ≥ 2 rows — the X row and the Y row"* — the `≥` was present, but with **no `including`** and a closed two-item list, so it read as exhaustive. **I am treating the gate as right, because the defect it names is real**: per the board's own restatement, *a bare exact count is the defect, not just the wrong number*. The number was also low (≥ 2 vs ≥ 4), and the gate's own M26 row records *"RED, WIDER — ≥ 4"*. Both are now fixed, and the block says explicitly that **the prediction held** (wider ⊇ predicted = confirmation under Addendum B) and that only its **form** failed.

⛔ **The board's copy was already corrected by the manager on `TASK-1147`'s status line — I did not edit that row.**

---

## 4. 🚨 ZERO EXECUTABLE LINES — PROVEN, NOT ASSERTED

The two source files were **already dirty** at session start (they carry `TASK-1147`'s uncommitted work, which `TASK-1149` hosts), so `git diff HEAD` cannot isolate my edits. I snapshotted both files **before touching them** and diffed pre → post.

**(a) Line census of my diff:**

| file | added | removed | added/removed lines that are **not** comment lines |
|---|---|---|---|
| `FogVolume.cpp` | **+6** | **−0** | **0** |
| `SiegeGraphicsMenuWidget.cpp` | **+25** | **−6** | **0** |

The filter was `CountOccurrencesInCode`'s **own** comment-line test, character for character (`SiegeAcquisitionFunnelTest.cpp:124-129`): trimmed line starts with `//`, `* `, `*/`, `/*`, or is a bare `*`. **Every changed line matches.** ⇒ the wiring tests that read these files as text **skip every line I wrote**.

**(b) The stronger check — comment-stripped byte identity.** Strip every full-line comment from pre and post and diff the remainder:

```
FogVolume.cpp              : IDENTICAL   (607 non-comment lines)
SiegeGraphicsMenuWidget.cpp: IDENTICAL   (2389 non-comment lines)
```

Not "no statement changed" — **the non-comment content of both files is byte-for-byte the file I was given.** No line mixed code and comment; no line was modified in place.

**(c) Delimiter balance** (a comment edit's real failure mode is commenting code out): `/*` count `1 → 1` and `*/` count `1 → 1` in `FogVolume.cpp`; `35 → 35` and `35 → 35` in `SiegeGraphicsMenuWidget.cpp`. Nothing I added contains `/*`, `*/`, `{` or `}`.

**(d) The two hint strings:** `diff` of the `ShadowHintFog*` declarations pre vs post → **BYTE-IDENTICAL**. `WARN-1`/`WARN-6` ruled them KEPT and they are untouched.

**(e) Why the suite delta is `0`:** the only tests that read these files as text are `SiegeFogVisualTest`'s wiring rows, which go through `CountOccurrencesInCode` (comment-stripping, proven in (a)) and `ExtractFunctionBody`/`SubstringBefore` over `EndPlay`, `RefreshFogVisual` and `EnforceFogRenderFloor` — **none of which contains my edit** (it is inside `ReleaseFogRenderFloor`, and adds no brace). `SiegeGraphicsMenuTest`'s fog rows (`:988-1009`) assert on the **composed runtime strings** `FogOn`/`FogOff`, never on the source file; **no test in the suite loads `SiegeGraphicsMenuWidget.cpp` as text** (censused). ⛔ Still a derivation — `TASK-1149`'s compile and run are the check.

**Cross-references I introduced were resolved against the post-edit file, not guessed:** `FogVolume.cpp:1004-1019` (`->Set(` at `:1004`/`:1010`/`:1011`, `SetVolumetricFog(true)` at `:1018`) and `:196-202` (the `"What IS established"` paragraph). Both verified after the final edit.

---

## 5. ⚖️ RULING ASKED FOR: does `TASK-1147`'s shipped hint sentence over-reach against `FOG-§12.1` as corrected?

**The sentence:** *"Lowering Shadows does not remove the Fog card's siege fog — its presence is a gameplay rule, not a graphics option."*

**Ruling: DEFENSIBLE. KEEP IT UNCHANGED.** Four reasons, then the one caveat that is genuinely owed.

1. **It is `FOG-§12.1`'s own claim, narrowed — not widened.** The law says *"`BP_SiegeFog` is a raymarched mesh, NOT a froxel participant, and **no measured route** deletes it."* The string restricts that to **Shadows**, the one group whose entire fog lever is `r.VolumetricFog` + the two grid axes — froxel terms, all three. The earlier draft that said *"appears at every setting"* **was** an over-reach across all ten quality groups, and its author withdrew it; the shipped sentence is the withdrawal.
2. **It makes no claim the law forbids.** `FOG-§12.1`'s prohibition is on saying the exploit is **closed / confirmed / fixed**. The string says nothing about Jonathan's report, nothing about a fix, and nothing about the floor. A player reading it learns a **design rule**, not a verdict.
3. **The normative half is true by construction.** *"Its presence is a gameplay rule, not a graphics option"* is `GFX-§12` stated to the player — the standard the project holds itself to, and the standard `TASK-1160` is being run to **verify**, not to establish. ⚠️ Worth saying plainly, because it is precisely what WARN-7's comment got wrong: today that rule holds because `BP_SiegeFog` **is not a froxel participant**, **not** because the floor protects it. My corrected comment now says that four lines above the string, so the next reader cannot mis-attribute it the way the old comment did.
4. **Its falsifier is already written in the file, with an owner and a deadline.** Falsifier (1) of the same comment block names this exact sentence, records that **no frame has ever been captured at Shadows=Low with a card up**, and binds the author to correct it *in the same action* if the capture shows the wash dying. The gate saw all of this and ruled the strings **KEPT** (`WARN-1`/`WARN-6`); the board fences me from touching them.

⚠️ **THE CAVEAT, RECORDED RATHER THAN WAIVED — and it is the honest half of this ruling.** This is the lane's **only user-facing claim about a rendered outcome nobody has rendered**. Its evidence is a node census plus a 28-package source scan — strong, and of the right kind, but **inferential**. The alternative is worse in both directions (a silent panel is the *"honest and useless"* string this block already rejected; a hedged one teaches the player the menu is unsure of itself), so shipping it is right. But:

- ⛔ **`TASK-1160` is its falsifier, not its formality.** If the capture shows the card's wash dying at `Shadows=Low`, **this string is false** and must be corrected in the same action that reports the finding.
- ⛔ **Nobody may cite this string as evidence that ask (A) is closed.** It is a menu hint derived from the same census the law calls *unexplained*; quoting it back at the question would be circular. `TASK-1149`'s message composes from `FOG-§12.1`, never from this file — which is now stated inside the file itself.

---

## 6. 📌 For the manager (not mine to edit — `SC-§82`)

`FOG-§12.5`'s pinned row for the panel hint reads *"`ShadowQualityHintText`'s string at `SiegeGraphicsMenuWidget.cpp:155` / `:158`"*. Those line numbers are **stale, and were stale before this row ran** — the strings sat at `:205` / `:208` when I opened the file and sit at **`:224` / `:227`** now. ⚠️ `:155`/`:158` currently lands **inside the comment block**, i.e. on the WARN-7 text itself — a reader following the pin lands on the sentence the gate flagged instead of on the string. The **names** in the pin are correct and unambiguous; only the numbers drifted.

---

## 7. Files touched

| file | change |
|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/SiegeGraphicsMenuWidget.cpp` | **comments only** — WARN-7 rewrite at `:158-180` + one pronoun at `:181`. Strings byte-identical. |
| `Source/GitClaudeUnrealTest/Siegebound/FogVolume.cpp` | **comments only** — NIT-5, six comment lines at `:1116-1121`. |
| `.claude/pipeline/handoffs/TASK-1147-programmer.md` | NIT-6 — M26 prediction struck in place and restated as `≥ 4, including <named>`. |
| `.claude/pipeline/handoffs/TASK-1161-programmer.md` | this file. |
| `.claude/pipeline/TASKBOARD.md` | **`TASK-1161`'s single `- status:` line only**, via `Edit`. |

⛔ **NOT touched:** zero `Content/**` · zero `Config/**` · zero `.uasset` · `L_Arena` never opened · no editor, no MCP (an art-director held `TASK-1160` in the editor concurrently) · **no compile, no suite run, no Git** — `TASK-1149` is the host.

---

## 8. What QA / the host should scrutinise

1. **§ 4 (b)** is the load-bearing claim. Re-derive it if you like: `grep -vE '^[[:space:]]*(\*|/\*|//)'` both files against `git show HEAD:` **won't** work (the files were already dirty) — you need the pre-edit content, which is `TASK-1147`'s working tree, or simply confirm that `TASK-1149`'s compile is clean and the suite is `552 / 0`.
2. **The `:196-202` cross-reference** in the new comment is a **line number inside the same file** and will drift the next time this block is edited. It is paired with the paragraph's opening words (`"What IS established"`) so it can be re-found when it drifts.
3. **§ 5's ruling** is a judgement, not a measurement. If you disagree that the hint sentence is defensible, the fix is a new row against the string — **not** an edit here: `WARN-1`/`WARN-6` ruled it KEPT and this row is fenced out of it.
4. **The M26 restatement (§ 3)** predicts `≥ 4` rows red under a mutation **nobody has executed**. It is derived from the row texts at `SiegeFogVisualTest.cpp:1219-1251`, quoted above; if `TASK-1149` runs the M25/M26 witnesses, that is the measurement.
