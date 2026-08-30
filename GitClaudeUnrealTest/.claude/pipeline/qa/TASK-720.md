# QA Report — TASK-720
Verdict: **PASS** — 0 BLOCKER · 0 WARN · 3 NIT

Scope ruled: ONE diff — the assertion block now at `Tests/SiegeControlsHelpTest.cpp:634-668` and `handoffs/TASK-719-programmer.md`.
`qa/TASK-708.md`'s PASS over TASK-704/706/707 is **not reopened**. No compile, no git, no board write, `Tools/Packaging/` untouched.

---

## The six rulings

**1. CAN THE NEW ASSERTION ACTUALLY FAIL? — ✅ YES, verified at source, not on the handoff's word.**
Three links checked independently:
- fixture `:88-89` — `F → U`, `U → G` (both pre-existing; see ruling 6);
- `SiegeKeyboardLayoutSubsystem.cpp:663-682` — `GetPositionalKey` is a **single** `TranslationMap.Find`, no transitive closure ⇒ `GetPositionalKey(F) == U`, `GetPositionalKey(U) == G`;
- `SiegeControlsHelpWidget.cpp:1119-1137` — `RawLetter` + `bLiteralKeyLabel == false` takes the **one-`GetPositionalKey`-per-key** loop.
⇒ correct code returns **`U`** ⇒ `[0] == OneHop(U)` true, `!= TwoHops(G)` true ⇒ **green**.
⇒ a genuine double translation returns **`G`** ⇒ first conjunct `G == U` **false** ⇒ **RED**, printing *"got G; one hop is U, two hops would be G"*. That is verbatim the defect `SiegeControlsHelpWidget.cpp:1150-1152` names. It also goes red on a *no*-translation regression (`F`) and on any third value.

**2. STRICTLY STRONGER THAN WHAT IT REPLACED? — ✅ YES.** The old text asserted only `!= the double image`; the new one asserts `== the single image` **and** `!= the double image`. `== OneHop` alone already rejects every wrong answer the old form would have let through, and it is made at a position where the two images actually differ.

**3. FIXTURE SELF-CHECK GENUINELY PREVENTS VACUITY? — ✅ YES, and it fails LOUD rather than quiet.** `:654-659` asserts `TwoHops != OneHop` **before** the claim, printing `F -> U -> G`. If anyone ever drops the `U → G` entry, `TwoHops == OneHop` and the self-check goes red *and* the claim below goes red — it can never silently reduce to `X != X` the way the `Z` predecessor did. This is `SHIP-§9` in a test: the instrument is validated against the failure it exists to detect. Same idiom as the already-green test 2 (`:365-369`).

**4. COUNT STILL 156 — ✅ CONFIRMED.** `IMPLEMENT_SIMPLE_AUTOMATION_TEST` in this file = **13** (`:209, 345, 441, 563, 699, 770, 870, 942, 1060, 1136, 1284, 1411, 1488`) — identical to `qa/TASK-708.md` §7 and to build-master's 156 executed paths. The new code sits at `:634-668`, **inside** `FSiegeControlsHelpRawLanesTest::RunTest` (`:568-681`), before its `return true`. ⇒ **suite total 156, expected 156/156.**

**5. THE FOUR DEVIATIONS — all four ruled sound.**
- **D1 (refusing to extend `MakeDvorakTranslation()` with a `Semicolon → …` source entry) — ⭐ the refusal is RIGHT.** The shipped translation's source domain is the 26 letters and only the 26 (`SiegeKeyboardLayoutStatics.cpp:55-64`), so a non-letter *source* entry would model a map the shipped subsystem can never build — and it would corrode Lane B's own identity assertions (`:596-600`) and the widget's Lane-B proof (`:1110-1117`), which both rest on that domain. The fixture is also shared by 12 other tests; leaving it byte-identical is the conservative and correct call.
- **D2 (the `Z` derive claim kept verbatim at `:630-631`) — sound.** `DerivedResolved[0] == ZTranslated` is a real, non-degenerate claim (`Semicolon != Z`) and it is the one that ties the F-1 reversal to the *accept key's own row*. Only the double-translation half was unmeasurable there; splitting them is correct.
- **D3 (+3 assertions replacing 1) — accepted.** Self-check + arity guard + repaired claim, no new test. The `if (Num() == 1)` guard prevents the indexed read running off an empty array — matches the surrounding blocks (`:614`, `:628`, `:662`).
- **D4 (the ~10-line comment at `:634-645`) — accurate.** All three citations verified at source: the letters-only table, the absent-key-returns-unchanged contract (`SiegeKeyboardLayoutSubsystem.h:237-240`), and the `F -> U -> G` defect text. It is the thing that stops the next reader "simplifying" this back into the fixed point.

**6. DIFF CONFINED TO ONE FILE, ONE BLOCK, NOTHING WEAKENED — ✅ CONFIRMED.**
- `SiegeControlsHelpWidget.cpp:1097-1174` is content-identical to what `qa/TASK-708.md` ruled on — Lane C's single loop at `:1132-1136` and Lane A's zero-call path intact; **no** `GetPositionalKey` added or removed.
- Fixture `:85-102` unchanged, and **proven pre-existing**: test 2 (`:365-369`) already consumes `F → U → G` and was **GREEN** in build-master's 155/156 run — nothing was added to the fixture to make the new claim true.
- Grep for `#if 0` / `Disabled` / commented-out `TestTrue|TestEqual|TestFalse` across the file: **no matches**. Both pre-existing assertion sites in test 4 (`:611`, `:630`) still present and unmodified. No expected value edited to force green.
- ⚠️ Method note: git was fenced for this review, so scope is established by content comparison against the 708-reviewed text plus mtime ordering (the test `.cpp` is the most recently modified of the three ControlsHelp files). Build-master should still eyeball `git diff --stat` before staging — it should show **one** file.

## Findings
- [NIT] `SiegeControlsHelpTest.cpp:651-652` — `OneHop`/`TwoHops` come from the same accessor the code under test calls, so this is not an independent oracle for the *map*. Acceptable: the subject here is the **hop count**, not the table, and the table is owned by the keyboard-layout suite. Identical to test 2's accepted idiom.
- [NIT] `:667` — `&& DerivedHopResolved[0] != TwoHops` is logically implied by `== OneHop` given the self-check. Harmless and it keeps the failure message honest; keep it.
- [NIT] Not compiled (correctly — 709 owns that gate). Syntax is the file's own idiom: `FString::Printf` + `TestTrue` precedent at `:593` and `:616`, `using namespace SiegeControlsHelpTestUtils;` in scope at `:570`, no new include, symbol or type.

## Notes for build-master (TASK-709 re-run)
✅ **TASK-709's re-run MAY PROCEED TO COMPILE + COMMIT.** No blocker stands against it.
1. **Expect 156 / 156.** `:632`'s failure was the only red; its replacement at `:634-668` is measurable and should be green. A 155 or 157 is not this batch's.
2. Parse the log for `Result:` — **the exit code lies**. Smart App Control enforced = machine state, report and stop, do not loop QA.
3. `git diff --stat` must show **one** source file: `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeControlsHelpTest.cpp`. Anything else in `Source/` is a finding, not cargo.
4. All of `qa/TASK-708.md`'s notes 4-9 (the `IsFocusable` deprecation ruling, C2445, explicit-path commit cargo, LFS re-`add`) carry over unchanged — this task adds nothing to that list.
5. M8: no replicated property, no new replicated class, no relevancy tier, no RPC. Test file; zero shipped surface.
