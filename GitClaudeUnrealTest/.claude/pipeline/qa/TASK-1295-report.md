Verdict: PASS — 0 BLOCKER · 5 WARN · 4 NIT

# QA Report — TASK-1295 (gate over TASK-1294)

subject: `TASK-1294` · reviewer: qa-reviewer · date 2026-09-18 · marker `TASK-1295-SUITE-AURA-401-GATE`
diff reviewed: `Tools/run_suite_bounded.ps1` (+96/−0, two hunks) · `handoffs/TASK-1294-programmer.md` (new) · one board status line.
⛔ No `Source/` file in this diff ⇒ no compile is owed by this row. ⛔ I ran no compile, no Git, no engine, no suite.

**INSTRUMENT DECLARATION (`SC-§39`).** My row forbids `Bash` (spec cl. (6)). Every count below was taken with the
`Grep` tool (ripgrep) over the real files on disk, and every code citation with `Read` at the quoted line. Where a
number could not be taken without a shell (`git diff`, `git status`), I say so and mark it ACCEPTED-AS-DECLARED.
Nothing in this report is quoted from the handoff without being re-taken, except where explicitly labelled.

---

## 0. THE ROUTING RULING — THE THREE SUITE RUNS (read this first)

**The deferral is LEGITIMATE. It is recorded here as an acceptance criterion that is NOT met and is carried to the
host — ⛔ not waived, ⛔ not excused, ⛔ not deemed satisfied.**

- **What is unmeasured:** `TASK-1294` acceptance (3) — three suite runs, with the per-run triple (`401` count ·
  total `LogAura` count · reds by name) and the totals. **Zero runs exist.** Nothing in this row's claim about
  *run-time behaviour* is proven; what is proven is the *mechanism* (§1) and the *instrument* (§3–§4).
- **Who owes it:** `TASK-1298` cl. (3), which owns it **by name** — verified at source, `TASKBOARD.md:3478`:
  *"**(3) THE SUITE — ⛔ THREE RUNS, ⛔ NOT TWO, AND THE REASON IS ON `TASK-1294`'s ROW"*. Its cl. (4) additionally
  requires the host to confirm the **GUI editor's Aura still loads** after relaunch and calls a suite-lane exclusion
  that also disabled the editor's plugin *"a ⛔ STOP, ⛔ not a nit"*.
- **The host cannot close without it:** `TASK-1298` is a commit host; cl. (3) is one of its own acceptance lines and
  its cl. (4)/(6) sit behind it. There is no path from here to a commit that does not pass through those three runs.

**Why PASS and not FAIL.** A FAIL bounces the row to an agent that is *also* fenced from measuring it — the dispatch
fences the compile lane, the row is `parallel-safe: ⛔ NO vs any compile/verify task`, two sibling lanes are live in
`Source/` (measured: §5), and the GUI editor is up. The return would be a rewritten handoff, not a measurement.
Three further reasons, each at source:

1. **The board itself says the runs cannot settle this claim.** `TASK-1294` `CONSEQUENCE 3` (`TASKBOARD.md:3418`):
   *"a statistical argument from run counts is ⛔ now provably unable to settle this"*, and `CONSEQUENCE 2`
   (`:3414`): the three runs are *"the ⛔ flake evidence, ⛔ not the mechanism evidence, and ⛔ this row now owes
   ⛔ both."* The **mechanism** half is delivered and I have verified it at source (§1–§4). The **flake** half
   requires the suite lane the row may not enter.
2. **`VER-§8` cl. 2** (`CONVENTIONS.md:11878`): *"discovering the ceiling at 5b is ⛔ a MANAGER defect, ⛔ not a
   verifier finding."* An acceptance criterion that needs a lane the assignee is fenced out of is a boarding-time
   ceiling. **cl. 3(a)** (`:11880`) prescribes exactly what this row did: split the criterion at the ceiling, measure
   the half you can, name the half you cannot — *"a ceiling on ⛔ one half is ⛔ never a licence to stop measuring the
   ⛔ other"*, and this row did not stop.
3. **It was declared, not buried.** Handoff §4 states *"I did not run the suite. Not once."* in its own heading and
   asks to be routed rather than passed silently (§7 item 3). `SC-§70`'s disease is an absence quoted without its
   cause; this is an absence quoted with four causes and a named owner.

**Precedent followed:** `TASK-1305` — PASS with the unmeasured half named and its owner named.

⚠️ **AND THE CONDITION ON THIS PASS — read `W1` before the host runs anything.** `TASK-1298` cl. (3) currently
prescribes the **bare** `grep -c 'Response code: 401'`, which I have measured to be incapable of settling this row's
claim. If the host runs cl. (3) as written, it will produce three zeroes that prove nothing, and this gate's whole
purpose is defeated one row downstream.

---

## 1. THE LEVER CHAIN — VERIFIED AT SOURCE, END TO END

`Read` on `C:/Program Files/Epic Games/UE_5.8/Engine/Source/Runtime/Projects/Private/{PluginManager,PluginReferenceDescriptor}.cpp`.
I walked all five links myself rather than accept the handoff's chain.

| link | claimed | measured | verdict |
|---|---|---|---|
| command line runs before the target/`.uproject` | `:2043` vs `:2049` | `FindCommandLinePlugins` at **`:2044`**, `FindTargetPlugins` at **`:2050`** (`:2043` is `FindCompileTimePlugins`, `:2049` is a comment) | ✅ **ORDERING HOLDS** (off-by-one in the citation — `W3`) |
| the value is parsed with no space, split on commas | `:1478-1479` | `FParse::Value(FCommandLine::Get(), InListKey, PluginsListStr, false);` then `ParseIntoArray(PluginsList, TEXT(","))` | ✅ exact |
| `DisablePlugins=` builds a **disabled** reference | `:1587` / `:1592` | `ExtraPluginsToDisable = ParsePluginsList(TEXT("DisablePlugins="));` `:1587`; `FPluginReferenceDescriptor(DisablePluginName, false)` `:1592` | ✅ exact |
| the name is recorded in `ConfiguredPluginNames` | `:1597` | `Context.ConfiguredPluginNames.Add(DisablePluginName);` `:1597` | ✅ exact |
| a disabled reference short-circuits | `PluginReferenceDescriptor.cpp:41-47` | `IsEnabledForPlatform` opens `// If it's not enabled at all, return false` / `if(!bEnabled) { return false; }` | ✅ verbatim |
| …and therefore never enters `EnabledPlugins` | `:2425-2428` | `if(!Reference.IsEnabledForPlatform(Platform) …) { UE_LOGF(… "Ignoring plugin '%ls' for platform/configuration"); continue; }` — before the plugin is ever found or added | ✅ exact |
| the target receipt is gated on that set | `:1636` | `if (!Context.ConfiguredPluginNames.Contains(PluginName))` | ✅ exact |
| **the `.uproject` array is gated on that set** | `:1709` / `:1731-1733` | `ProcessPluginConfigurations` gate at `:1709`; called at `:1732` with `TArray<FPluginReferenceDescriptor> PluginReferences(ProjectDescriptor->Plugins);` `:1731` and source string `"Enabled plugins in .uproject for %s"` `:1733` | ✅ exact |
| `.uplugin EnabledByDefault` is gated on that set | `:1748` | `if (Plugin->IsEnabledByDefault(…) && !Context.ConfiguredPluginNames.Contains(PluginName))` | ✅ exact |

⇒ **`-DisablePlugins=Aura` genuinely beats `GitClaudeUnrealTest.uproject:54-61` for that one process.** The fix is
real, the `.uproject` needed no edit, and cl. (1)'s hard stop correctly did not trigger. **The route is EXCLUSION,
it is stated as the fix and not the fallback, and the fallback is named as not-taken both in the handoff (§0(iii))
and at the site (`Tools/run_suite_bounded.ps1:388-391`, *"DO NOT 'simplify' this to an expected-error /
log-suppression pin"*)** ⇒ `SC-§70` satisfied; check (2) PASS.

**The one residual, stated because I did not close it (`SC-§119`):** the *only* route into `EnabledPlugins` that is
**not** gated on `ConfiguredPluginNames` is the dependency queue inside `ConfigureEnabledPluginForTarget`
(`:2418-2429`) — a plugin can still be pulled in as a **dependency of another enabled plugin**. `Aura.uplugin:40-147`
declares 23 dependencies *outward*; I found nothing declaring Aura *inward*, but I did not enumerate every `.uplugin`
on the machine. This is precisely what `grep -c "Mounting Engine plugin Aura" == 0` settles at run time (§4), so it
is closed by the host's measurement, not by my reading.

---

## 2. `VER-§7` / GUI-EDITOR SAFETY — UNAFFECTED **BY CONSTRUCTION**, MEASURED

- `Grep "DisablePlugins"` over the repo → **11 hits in `Tools/run_suite_bounded.ps1` only** (plus board/handoff
  prose). The flag literal is typed in exactly **one** place: `Tools/run_suite_bounded.ps1:400`.
- `Grep "run_suite_bounded|New-EditorCommandLine"` over `Tools/` → **18 hits, all in that one file** ⇒ ⛔ no other
  script dot-sources the composer.
- `Grep "UnrealEditor\.exe"` over `Tools/` → **0**. ⛔ No tool in this repo launches the **GUI** editor.
- The only launcher is `Start-Process -FilePath $EditorCmd` (`:1577`) where `$EditorCmd =
  '…\UnrealEditor-**Cmd**.exe'` (`:1499`).
- Corroborated on real artefacts: `Saved/Logs/run_suite_bounded_suite_20260918-161724.log.cmdline` — the launched
  line **is** the composed `$parts` join, verbatim, and (being pre-change) carries no `-DisablePlugins`.

⇒ check (2)'s GUI-editor clause **PASS**. The verifier's Aura lane and Aura's MCP tooling cannot be reached by this
diff. (`TASK-1298` cl. (4) re-confirms it empirically after relaunch — correct belt-and-braces.)

**Also verified, because the host will run this three times with the GUI editor up:** the runner **never kills an
editor it did not start**. `:1552-1563` warns and explicitly *"NOT killing it — that is a human decision"*;
`:1663-1667` censuses orphans and says *"Some may be Jonathan's. NOT killing them."* `Stop-Process` is aimed only at
its own `$proc`. ⛔ No `SC-§118` hazard is introduced or present.

---

## 3. THE SELF-TEST — `SC-§122`: THE INSTRUMENT CAN RETURN A NON-ZERO ANSWER

Read at `Tools/run_suite_bounded.ps1:1237-1275`. Two new cases (`:1259`, `:1275`) ⇒ 52 → **54**, consistent with the
reported `-SelfTest` 54/54.

The predicate is `$hasAuraFlag = { … $Line -cmatch '(^|\s)-DisablePlugins=Aura(\s|$)' }` (`:1254`). I traced all four
degenerates (`:1264-1267`) against it by hand:

| degenerate | resulting token | predicate | correct? |
|---|---|---|---|
| deleted | *(absent)* | no match | ✅ refused |
| space after `=` | `-DisablePlugins= Aura` | no match (`=Aura` not contiguous) | ✅ refused |
| value emptied | `-DisablePlugins=` | no match | ✅ refused |
| name mistyped | `-DisablePlugins=AuraModelGenerator` | no match (`(\s|$)` fails on `M`) | ✅ refused |
| **the real line** | `… -NoLiveCoding -DisablePlugins=Aura -log …` | match | ✅ accepted |

⭐ The controls go **through** the predicate, not beside it, and `:1272-1274` closes `SC-§39`'s own loop — *"the
predicate must still be able to say YES, or the four NOs above are just the answer of a needle that matches
nothing."* **That final clause is also the mechanism that makes the reported mutation results come out at 52/54**: I
traced M1/M2/M3 and each fails case 1 *and* trips `:1274`, giving exactly two reds ⇒ **52/54, exit 5**, as reported.
The reported mutation run is internally consistent with the code as written.

⛔ **What the self-test does NOT prove, stated plainly:** it asserts the **string**. It cannot assert that the engine
honours the flag — that is §1's job (structural) and §4's (empirical, at the host).

**Diff-size corroboration (I cannot run `git diff`):** hunk 1 = comment `:338-391` + flag `:400` ≈ 55 lines; hunk 2 =
`:1236-1276` ≈ 41 lines; 55 + 41 = **96**, reconciling the declared +96/−0 against the bytes on disk.

---

## 4. THE LOG INSTRUMENT — RE-MEASURED BY ME ON ALL 18 LOGS, AND IT REPRODUCES EXACTLY

`Glob "Saved/Logs/run_suite_bounded_suite_*.log"` → **18 files**, matching the handoff's corpus.

| command (`Grep`, `output_mode: count`, over `Saved/Logs`, glob `run_suite_bounded_suite_*.log`) | result |
|---|---|
| `LogAura` | **15 files, 16–31** (`345` total); ⛔ **0 on the three 2026-09-09 logs** |
| `Response code: 401` | 12 files: **2** ×9, **4** ×3; 0 on six |
| `\]LogAura: .*Response code: 401` (**FAULT**) | **2** on exactly those 12; 0 on the other 6 |
| `\]LogAutomationController: .*Response code: 401` (**CAPTURE**) | **2** on exactly **4** files (`…143207`, `…143418`, `…211827`, `…211945`); 0 on 14 |
| `Result=\{Fail\}` | **exactly those same 4 files** (2/2/1/1); 0 on 14 |
| `Mounting Engine plugin Aura` | **1** on all 15 post-install logs; ⛔ **absent on all three 2026-09-09 logs** |
| `Test Started\.` / `Test Completed\. Result=` on the 09-17/09-18 logs | 559/559 ×2 then **561/561 ×4** |
| `IMPLEMENT_SIMPLE_AUTOMATION_TEST` over `Siegebound/Tests` | **561 across 47 files** |

**Every number in the handoff's 18-row triple table reproduces, value for value.** I found no discrepancy.

- ✅ **FIRING POSITIVE CONTROL:** `LogAura` returns 16–31 on 15 runs and `Mounting Engine plugin Aura` returns 1 on
  15 runs ⇒ both instruments demonstrably *can* answer non-zero.
- ✅ **NEGATIVE CONTROL, AND IT IS REAL HISTORY:** both return **0** on the three 2026-09-09 pre-install runs ⇒ a
  zero is a *previously observed* state produced by genuine plugin absence, in this exact log shape.
- ✅ **`CONSEQUENCE 2`'s HOLE IS CLOSED ON EXACTLY THE THREE RUNS THAT WOULD HAVE LIED.** `20260917-212139`,
  `-212229`, `20260918-161724` are the "`401` = 0 with no fix applied" runs. Bare `401` = **0 / 0 / 0**; structural
  `LogAura` = **16 / 18 / 18**; `Mounting Engine plugin Aura` = **1 / 1 / 1** ⇒ the discriminator exists and
  discriminates correctly on precisely those three runs. ✅ **Check (4)'s structural requirement is SATISFIED.**
- ✅ **AND THE PRESCRIBED PIN IS SOUND — I checked it could legitimately return 0 on success.** `PluginManager.cpp`
  emits `"Mounting %ls plugin %ls"` at **`:1926`**, inside a loop whose array is built at **`:1893-1902`** under
  `// only process enabled plugins` / `if (Plugin->bEnabled && !Plugin->Descriptor.bExplicitlyLoaded)`. ⇒ that line
  is **enable-time, not discovery-time**; it will genuinely vanish when the flag works. A pin that could not return 0
  on a correct fix would have been as bad as one that cannot return non-zero — it is not one.
- ✅ **561 is cross-checked two independent ways** (log totals ×4 and the `IMPLEMENT_` census) and this diff adds no
  test and removes none (`Source/` untouched by it) ⇒ **check (5) PASS**, baseline **561/561**, any other total a STOP.

**Two tests exist in the corpus that reference `Aura`?** No — `Grep "Aura"` over `Siegebound/Tests` returns exactly
**1** hit and it is a comment (`SiegeMenuInputTest.cpp:35`). ⛔ Zero code dependency, as claimed. (The `61` from
`grep -rn Aura Source/` is the `WarBannerAura` homonym; the handoff flags it correctly and I did not quote it.)

---

## 5. THE HARD FENCES (check (3)) — EACH MEASURED, NONE TAKEN ON TRUST

| fence | how I measured it | result |
|---|---|---|
| `GitClaudeUnrealTest.uproject` diff = **0** (🧑 his file, R11) | the session-start `git status` snapshot, taken **independently of the handoff**, lists 6 modified paths and the `.uproject` is **not among them**; `Read` confirms the Aura entry intact at `:54-61` | ✅ **0** |
| `.claude/settings.local.json` diff = 0 | same snapshot — absent | ✅ 0 |
| `Saved/**` staged/modified | same snapshot — absent | ✅ 0 |
| no test added / removed / renamed / reordered **by this diff** | this diff is one file, `Tools/` — `IMPLEMENT_…` census **561**, unchanged | ✅ 0 |
| no credential / token / auth anywhere in the diff | `Grep "eyJ[A-Za-z0-9_-]{8,}\|AnonKey=.\|DbPassword=.\|hf_[A-Za-z0-9]{8,}\|Bearer \|apikey"` on `Tools/run_suite_bounded.ps1` → **0**, with the **firing positive control** required by `SC-§39`'s 2026-09-17 amendment: the same prefix-plus-shaped-body regex class `DisablePlugins=[A-Za-z0-9_-]{3,}` on the **same file through the same tool** returns **9** ⇒ the zero is a measured zero, not a blind needle. ⛔ Shaped, not literal, per `SC-39-SHAPED-PIN-NOT-LITERAL` | ✅ 0 |
| the change adds no authentication to the plugin | the diff disables the plugin; no credential path is introduced | ✅ |

**`git status` (check (6)) — ACCEPTED-AS-DECLARED, with one attribution I resolved myself.** My session-start
snapshot shows `Source/.../Tests/SiegeMenuInputTest.cpp` dirty, which the handoff's §6 porcelain does not list.
I did **not** assume; `handoffs/TASK-1296-programmer.md:141` claims that file with ` <- MINE` and `:157` describes it
as *"a pure deletion — 1 hunk, 15/0"*. ⇒ it is `TASK-1296`'s, not this row's. `TASKBOARD.md` is likewise dirty and is
the board itself. `TASK-1298` cl. (1) re-measures porcelain and must stage none of the sibling dirt.

---

## 6. THE FAULT/CAPTURE FINDING — ADJUDICATED

**The finding reproduces exactly under my own greps (§4), on all 18 logs, with zero exceptions:**
`bare == FAULT + CAPTURE` holds 18/18 (2 = 2+0 ×9; 4 = 2+2 ×3; 0 = 0+0 ×6), and — a check the handoff invites but
does not itself publish as a count — `Result={Fail}` is non-zero on **exactly** the 4 logs where CAPTURE = 2 and on
no others, so `CAPTURE ≥ 1 ⟺ ≥ 1 red` on 18/18 runs.

**The strength claim is HONEST and I endorse it as written.** The handoff says: a **measured correlate over 18 runs**;
it says *whether*, not *why*; the mechanism remains un-isolated; and it is **moot under the fix**. That is the
correct strength, and it resists the two errors available here — it does not inflate into causation, and it does not
retreat into "just a proxy". ⛔ I record the boundary explicitly: this is attribution, not counterfactual causation;
a test carrying the copied `401` in its error list could in principle also have failed for its own reason, and
`20260914-143207`/`-143418` are exactly that shape (2 reds, CAPTURE 2 — one of the two, `DownTwiceThenAccept…`, is a
genuine failure present in both runs).

**One refinement I measured that strengthens it (`NIT N3`):** `FAULT = 2` is **not** "Aura fired twice". The two
lines are one burst — `…211827:4565` `LogAura: Error: Response code: 401` and `:4568`
`LogAura: Error: Error: Response code: 401, Response content: {"message":"User not authenticated","status":"error"}`
— and the CAPTURE pair at `:4575`/`:4578` is the controller copying **those same two lines** 31 ms later. So the
"2 vs 4" is one event, counted twice by an unanchored needle. The handoff's reading is right and is if anything
understated.

**Recommendation to the manager: ADOPT the needle correction. Specifically —**
1. **Anywhere a row still prescribes the bare `grep -c 'Response code: 401'`, replace it** with the anchored pair
   `\]LogAura: .*Response code: 401` (FAULT) + `\]LogAutomationController: .*Response code: 401` (CAPTURE). The most
   urgent site is `TASKBOARD.md:3478` (`TASK-1298` cl. (3)) — see `W1`.
2. **Keep the structural pair as the claim** (`LogAura` total = 0, `Mounting Engine plugin Aura` = 0); FAULT/CAPTURE
   is the *adjudication* instrument, not the proof of the fix. Under the fix all four read 0 and the anchored pair
   retains value only as the regression detector if Aura ever returns to the lane.
3. **`CONSEQUENCE 3`'s "conditions we have not isolated" stands as written.** This finding does not isolate them; it
   shows the *distinction between the two states was legible by prefix all along*. That is a real correction to how
   the evidence was read, and it is not a correction to the mechanism. ⛔ Do not let the row's text be trimmed to
   read as though the mechanism is now understood.

---

## Findings

- **[WARN] `TASKBOARD.md:3478` (TASK-1298 cl. (3)) — the host's prescribed needle is the one this gate proved cannot
  settle the claim.** cl. (3) says *"quote … `grep -c 'Response code: 401'` on that run's log"* — the **bare**
  needle. Measured: that needle returned **0 on three runs with no fix applied** (`20260917-212139`, `-212229`,
  `20260918-161724`), and my own counts confirm it. A host that runs cl. (3) as written produces three zeroes that
  prove nothing and commits a fix nobody measured. **Fix (manager's — ⛔ I may not edit that row):** amend cl. (3) to
  require, per run, the four anchored greps in `handoffs/TASK-1294-programmer.md:288-295` plus
  `grep -c "Mounting Engine plugin Aura"`, with the pass shape `LogAura` **0** · `Mounting…` **0** · FAULT **0** ·
  CAPTURE **0** · reds none · **561/561**. Until that lands, the handoff's §4 is the only place the right commands
  exist, and a host is not obliged to read a handoff's prose for its own acceptance line.
- **[WARN] `Tools/run_suite_bounded.ps1:400` — the flag also removes Aura's *dependency closure* from the suite lane,
  and no document names it.** `Aura.uplugin:40-147` declares 23 plugin dependencies. Plugins enabled **only** via
  that chain (candidates: `GameplayAbilities`, `GeometryScripting`, `PCG`, `IKRig`, `Interchange`,
  `MeshModelingToolset`, `AssetSearch`, `PythonScriptPlugin`, `RemoteControl`, `WebBrowserWidget`,
  `EditorScriptingUtilities`, `ProceduralMeshComponent`, `AVCodecsCore`, `NVCodecs`, `CascadeToNiagaraConverter`)
  will no longer be enabled in that process. **I checked the ones that could bite and they do not:** `EnhancedInput`
  is `"EnabledByDefault": true` (`EnhancedInput.uplugin:13`) so the menu-input tests are safe; `StateTree`,
  `GameplayStateTree`, `ModelContextProtocol`, `SiegeLlama`, `ModelingToolsEditorMode` are enabled directly by the
  `.uproject`; and every module in `GitClaudeUnrealTest.Build.cs:11-105` resolves through those. **Residual:** an
  *asset* referencing a class from a closure-only plugin. **The discriminator is already in the host's hands** —
  if run 1's total ≠ **561** or a **new** red appears that is not the `401`, suspect this, not the race.
- **[WARN] `Tools/run_suite_bounded.ps1:365-366` (and handoff §0(iii) item 1) — the shipped comment's engine citation
  is off by one, in the one link the whole fix rests on.** It says *"`FindCommandLinePlugins` FIRST
  (PluginManager.cpp:2043), BEFORE `FindTargetPlugins` (:2049)"*. Measured: `:2043` is `FindCompileTimePlugins`,
  `FindCommandLinePlugins` is **`:2044`**; `:2049` is a comment, `FindTargetPlugins` is **`:2050`**. ⭐ **The claim
  itself is TRUE and I verified it** (2044 < 2050) — every other citation in the chain is exact — but this comment is
  a durable in-code artefact, and `SC-§99` is the law that a reader must be able to open the cited line and find the
  named thing. Suggested fix: `:2044` / `:2050`. ⛔ Not a blocker: no behaviour depends on the number.
- **[WARN] `Tools/run_suite_bounded.ps1:59-69` — the header's "NEVER launched" block is measurably false, correctly
  left alone, and needs its own manager row.** It states *"As of 2026-09-09 this script has NEVER launched
  UnrealEditor-Cmd.exe"* and *"no real editor has ever been killed by this file"*. **Measured:** `Glob
  "Saved/Logs/*.cmdline"` returns **19 sidecars** — one per launch, written at `:1574` immediately before
  `Start-Process` at `:1577` — i.e. **18 suite launches + 1 command-lane launch**, the earliest four on **2026-09-09
  itself**. ⇒ the handoff's *"15 runs"* undercounts; and `run_suite_bounded_command_20260909-045326.log.cmdline`
  falsifies `CONVENTIONS.md:6152`'s *"the COMMAND LANE has NEVER DISPATCHED THROUGH THE SCRIPT"* as well.
  ✅ **The programmer was right not to touch it:** the block's own `:68-69` reserves the strike (*"only the manager may
  strike it (SC-82)"*), and `TL-§6`'s `NOT MEASURED` bullet (`CONVENTIONS.md:6150-6153`) says the same. **Belongs in
  its own manager row, not in this one** — it is out of this row's `names:` scope and `SC-§82` puts law text and this
  block off-limits to both the programmer and to me.
- **[WARN] `TASKBOARD.md:3420` — the "does not exist" statement is stale, but it is NOT in `TL-§6`; the handoff names
  the wrong site.** Handoff §0 says *"`TL-§6`'s named executor … The standing note that it 'DOES NOT EXIST on disk'
  is stale."* **Measured: `CONVENTIONS.md:6144` already carries that claim STRUCK AND CORRECTED** —
  `~~⛔ MEASURED 2026-09-09: ⛔ THIS FILE ⛔ DOES NOT EXIST~~ ⇒ ✅ CORRECTED 2026-09-09 (EVENING): ⛔ IT ⛔ EXISTS —
  ⛔ 1616 LINES`. The site still asserting it live is **`TASK-1294`'s own spec cl. (0)** at `TASKBOARD.md:3420`:
  *"`Tools/run_suite_bounded.ps1` is named by `TL-§6` as the sanctioned executor and ⛔ DOES NOT EXIST on disk"*.
  ⇒ the flag is correct that a stale statement exists and that it should be retired; it points at a file that already
  fixed itself. Both corrections are the **manager's** (board text + the `NOT MEASURED` bullet). ⛔ Neither belongs in
  this row.

- **[NIT] `handoffs/TASK-1294-programmer.md:327-332` — the quoted porcelain is a snapshot taken before the board flip
  and before a sibling's write.** It omits `TASKBOARD.md` and `SiegeMenuInputTest.cpp`, both dirty at my session
  start. Attribution resolved independently (§5) — no action beyond `TASK-1298` cl. (1)'s re-measure.
- **[NIT] `handoffs/TASK-1294-programmer.md:56` / `Tools/run_suite_bounded.ps1:375-376`** — the `.uproject` Aura entry
  spans **`:54-61`**, not `:54-60`; the comment's `:55-56` points at the two inner keys. Cosmetic.
- **[NIT] §3 of the handoff — record that `FAULT = 2` is one burst of two lines, not two firings** (measured at
  `…211827:4565`/`:4568`; the CAPTURE pair mirrors both at `:4575`/`:4578`). It strengthens the finding and forestalls
  the next reader re-deriving "it fired twice".
- **[NIT] `Tools/run_suite_bounded.ps1:1267`** — the fourth degenerate (`-DisablePlugins=AuraModelGenerator`) is a
  *plausible* mistype only because `AuraModelGenerator` is a **module** of the Aura plugin, not a plugin. That is the
  right control to have, and the comment could say so in four words so nobody later "corrects" the test by adding
  that name to the flag.

---

## Notes for build-master (TASK-1298)

1. ⛔ **Do not run the three suite runs on the bare `401` needle alone.** Run, per log, all five:
   `grep -c "LogAura"` (**MUST be 0** — this is the claim) · `grep -c "Mounting Engine plugin Aura"` (**0**) ·
   `grep -cE "\]LogAura: .*Response code: 401"` (**0**) · `grep -cE "\]LogAutomationController: .*Response code: 401"`
   (**0**) · reds by name · `Test Started.` / `Test Completed. Result=` = **561/561**. A bare `401` of 0 has happened
   three times with no fix applied and is not a pass (`W1`).
2. ✅ **First, confirm the flag actually reached the process:** the runner writes
   `Saved/Logs/<log>.cmdline` beside every log (`:1574`). It must now contain `-DisablePlugins=Aura` between
   `-NoLiveCoding` and `-log`. That is a zero-cost check that the composed line is the launched line.
3. ⚠️ **Your cl. (4) GUI-Aura re-check is the right instrument and it is not redundant** — my §2 proves the flag
   *cannot* leak into a GUI launch from this repo, but only your post-relaunch check proves Aura still works there.
4. 🚨 **Cross-lane hazard, not this row's defect, but it lands on your three runs — ⛔ AND THE MECHANISM IS NOT THE
   ONE I FIRST WROTE HERE.** `TASK-1296` carries an open condition: `BP_MenuGameMode` is saved but **not recompiled**
   (`BS_DIRTY`). ⛔ **Correction, adopted from `qa/TASK-1297-report.md` while I was writing this (it measured what I
   had only read):** `BaseEditorPerProjectUserSettings.ini:242` `AutoRecompileBlueprints=True` with no project
   override ⇒ `bPromptForCompile=false` ⇒ the modal **will not fire** and **will not hang** your run; instead the
   pre-PIE sweep **silently auto-compiles in the live editor**, giving a **green suite over a stale committed asset**
   (`PlayLevel.cpp:1290`/`:2656`). ⇒ the discriminator is **the `.uasset` sha256 changing** from `fd9817f0…954962`,
   not the suite's colour. ⭐ **This matters to *your* three runs for `TASK-1294` too:** a green 561/561 does not by
   itself tell you which bytecode produced it. My earlier "it would hang" reading came from `TASK-1296`'s own status
   prose; `TASK-1297` measured it and it is superseded — ⛔ use the sha gate.
5. ⛔ **If run 1's total ≠ 561, or a new red appears that is not the `401`:** check `W2` (Aura's dependency closure)
   before bouncing `TASK-1294`. Quote the red's name and the plugin-enumeration lines.
6. 📌 The runner never kills an editor it did not start (`:1552-1563`, `:1663-1667`) — the GUI editor (PID 1812) is
   safe from it, but the script's own warning about `Saved/`/DDC contention still applies to your run.

## Carried-forward, unmet (⛔ not waived)

`TASK-1294` acceptance (3): **three suite runs with the per-run triple.** Runs executed by this row: **0**.
Owner: **`TASK-1298` cl. (3)** (`TASKBOARD.md:3478`), which names it and cannot close without it.
Until those three runs land, this row's verified claim is *"the lever is real, reaches the lane, is guarded, and the
instrument that will judge it can return both answers"* — ⛔ **not** *"the 401 no longer reds a random test."*
