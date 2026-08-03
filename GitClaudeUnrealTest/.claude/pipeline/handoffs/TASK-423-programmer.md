# TASK-423 — gameplay-programmer handoff (2026-08-03)

**M8 DECLARATION DUTY, stated verbatim as required:**
> "adds no replicated property, no new replicated class, no new relevancy tier."

---

## ⛔ READ THIS FIRST — TWO THINGS THIS HANDOFF WILL NOT LET YOU BELIEVE

1. **WARN-5 IS *NOT* DISCHARGED.** It is now *instrumented*. The difference is
   the whole point of this task and it is spelled out in §5 below.
2. **THIS IS A PARTIAL DELIVERY OF TASK-423.** The Zone-A two-lane gate is
   delivered. `USiegeLlamaSubsystem` — the worker thread, queue depth 1, abort
   callback, the three timeouts, offload tiering and KV reuse, i.e. the bulk of
   the board's TASK-423 spec — **is NOT written.** See §7. Do not read
   `ready-for-qa` on the board as "TASK-423 is done".

---

## 1. What was delivered

**One new file, no edits to any existing file:**

`C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Source\GitClaudeUnrealTest\Siegebound\Tests\SiegeAssistantZoneATest.cpp`

Five automation tests under `Siegebound.Assistant.ZoneA.*`
(`EditorContext | EngineFilter`, guarded by `WITH_DEV_AUTOMATION_TESTS`):

| Test | Asserts | Unit |
|---|---|---|
| `TwoLaneByteEquality` | shipped `BuildZoneA(default vocab)` is byte-identical to a frozen verbatim copy of the spike's `AppendZoneA` | **characters (byte-exact compare)** |
| `MeasuredCharCount` | **both** lanes are exactly 5116 | **chars AND UTF-8 bytes, asserted separately** |
| `AsciiCleanliness` | both lanes contain no code unit > 127, and `Len() == utf8 bytes` | **chars / bytes** |
| `StaticPrefixContract` | Zone A byte-stable across calls, across two snapshot objects, across two separately-constructed default vocabularies | **characters** |
| `NullVocabularyIsNotTheMeasuredLane` | `BuildZoneA(nullptr)` is deterministic, prints `synonyms:\nnone\n`, and is **not** the measured lane; its length is derived **at runtime** from the live synonym table | **characters** |

**⛔ There is no token constant anywhere in the file.** `zoneA_tok~=1139` is
deliberately absent and the header says why: §12g's measured error band is
**−15 to +4 with unpredictable sign**, so a derived token figure baked into a
test is an assertion whose error direction is unknown — wearing the authority of
a green test. The token ceiling stays a **runtime** reading off the printed
`zoneA_tok~=`. Two artifacts, two quantities, neither doing the other's job.

## 2. How the "spike lane" is represented — and why it is a copy, not a call

**The two lanes cannot be linked into one process by any legal arrangement:**
`AppendZoneA` is a **file-static in the plugin's private .cpp**; the game module
does not depend on `SiegeLlama`; and the plugin may never include a Siegebound
header (CONVENTIONS §8 — the whole surface between lanes is
`(prompt, gbnf) -> string`). So the spike lane in the test is a **frozen,
verbatim, 98-line copy** of the spike's emitting lines.

**⚠️ That is the test's main weakness and it is stated plainly rather than
buried.** If the copy were wrong, the test would compare the shipped lane against
the wrong bytes. Two things bound that risk:

- **It is mechanically auditable and QA should actually run the check:** extract
  every `Out += TEXT(...)` line from `SiegeLlamaSpike.cpp`'s `AppendZoneA` and
  text-diff it against `BuildSpikeLaneZoneA()`. Plain text diff — no compiler, no
  model, no engine. **I ran exactly this: 98 lines, verbatim match, and the copy
  emits exactly 5116 chars (sha256 `f033199ae0ff6a1c…`).**
- The fixture carries a ⛔ block saying that once the spike is deleted the copy
  becomes **evidence**, that a divergence means **the shipped builder moved away
  from the measurement**, and that **editing the fixture to go green destroys the
  only evidence the test exists to hold**.

## 3. Offline derivation (a prediction, NOT a discharge)

I reconstructed both lanes offline from source text — including a
re-implementation of `BuildSynonymTable` / `AppendSection` / `NormaliseAliases`
and the `EPlaceSlot` places loop — and got:

```
SPIKE    5116 chars / 5116 UTF-8 bytes / ASCII-clean / sha256 f033199ae0ff6a1c…
SHIPPED  5116 chars / 5116 UTF-8 bytes / ASCII-clean / sha256 f033199ae0ff6a1c…
BYTE-EQUAL: True
```

**⛔ This is a DERIVATION and it is same-authorship, so it discharges nothing.**
It is exactly the class §12g and §9c warn about — I wrote both the simulator and
the test. It is reported only as a **prediction that the test will run green**,
which is falsifiable the moment the test actually runs. Chars are exact where
tokens are not, which is why the prediction is worth stating at all; it is still
not a measurement of the shipped lane.

## 4. Two defects found and fixed while writing this

- **⚠️ `TestEqual` on `FString` is CASE-INSENSITIVE.** It forwards to the
  `const TCHAR*` overload; that is precisely why `TestEqualSensitive` exists as a
  separate API (verified in UE 5.8 `Misc/AutomationTest.h:1997` vs `:2016`). My
  first draft used it. **A byte-equality gate written with `TestEqual` would
  report SAFE on a Zone A where `own_castle` had become `OWN_CASTLE`** — the
  literal "automated guardrail that reports safe" failure this task exists to
  remove, reintroduced by the task removing it. **Every string comparison in the
  file is now `TestEqualSensitive` / `TestNotEqualSensitive`**, with a comment
  saying why so the next editor cannot undo it silently. The 8 remaining
  `TestEqual` calls are all `int32` vs `int32`.
- **Three em dashes were inside my own `TEXT()` literals.** Replaced with ASCII.
  A file that asserts ASCII-cleanliness must not ship non-ASCII prompt-adjacent
  literals. (Comments still contain non-ASCII; that is fine and unchanged.)

## 5. ⛔ IS WARN-5 DISCHARGED? — **NO. STILL OPEN.**

**It is instrumented, not discharged, and it must not be reported as discharged.**

**What still stands between here and discharge:**

1. **⛔ THE TEST HAS NEVER RUN.** No compile, no editor, no automation run
   happened in this task (single-gate law — build-master's TASK-447 owns the one
   compile; seven tasks share this UBT module). **Today this file is unexecuted
   code that asserts a claim — the same status §12g criticises `BuildZoneA`
   itself for.** A test that has never executed proves exactly as much as a
   careful reading, which is what WARN-5 already had.
2. **WARN-5 is discharged only when TASK-447's compile lands AND
   `Siegebound.Assistant.ZoneA.*` runs GREEN in the editor.** That run is the
   discharge event. Whoever sees it green should say so against the run, not
   against this handoff.
3. **Even green, three things stay out of reach and must not be claimed:**
   - **The ASSET lane is untested.** `DA_AssistantVocabulary` overrides the C++
     defaults **wholesale**; the test covers the **code-default** lane only. If
     the shipped runtime passes the asset, Zone A's bytes are whatever the artist
     last saved and **no test says they are 5116.** ⚠️ This is a real open gap and
     it is arguably the next one worth closing.
   - **Tokens are untouched.** Nothing here says Zone A is 1139 tokens. That
     remains a runtime reading.
   - **It proves the lanes are equal NOW**, not that they were equal at the
     instant the 5116 run was taken. (The count matching 5116 corroborates it; it
     does not prove it.)

## 6. ⚠️ THE SPIKE DELETION IS DEFERRED — SAYING SO, AS AMENDMENT A REQUIRES

**`SiegeLlamaSpike.cpp` and `SpikeCorpus.inl` were NOT deleted.** Amendment A
permits deferral provided it is declared; this declares it. Three reasons, the
third decisive:

1. It is the **only** command that prints `zoneA_tok` (`Siege.Llama.SpikePrompt`,
   which §12c's hard gate reads). No two-lane **token** reading has been taken.
2. While it exists, the fixture in §2 is **auditable by text-diff**. Delete it and
   that audit becomes impossible forever.
3. **⛔ Its replacement does not exist yet.** The rule is that two model-load paths
   must never coexist — but `USiegeLlamaSubsystem` is not written (§7), so
   deleting the spike now leaves **zero** model-load paths, not one. The deletion
   belongs in the same commit as the subsystem, not before it.

## 7. ⛔ WHAT IS NOT DELIVERED (the rest of TASK-423)

Not written, not started: `USiegeLlamaSubsystem` (`UGameInstanceSubsystem`, async
model load, faulted latch), the dedicated `FRunnable` at `TPri_BelowNormal`, FSM-
enforced queue depth 1, `abort_callback` cancellation, the three timeouts
(4 s / 10 s / 96 tok), offload tiering + `llama_memory_seq_rm` KV reuse, SEH
safety, and the spec's item (9) budget work (`MaxSnapshotChars` retirement,
`MaxSnapshotTokens = 400`, `SnapshotPreFilterMaxChars = 3000`, the re-anchored
truncation logging, `ZoneBCharReserve` re-measurement).

**This dispatch scoped me to the equality test explicitly**, so this is a scope
boundary, not an omission I am hiding. The subsystem needs its own dispatch.

**Amendment B is noted for whoever writes it:** the live symbols in the vendored
header are **`llama_memory_seq_rm`** (`llama.h:735`) and
**`load_mode = LLAMA_LOAD_MODE_MMAP`**. `llama_kv_cache_seq_rm` and `use_mmap`
**do not exist**. The board spec's item (6) still uses both dead spellings.

**Amendment D (§12h, the repeat law) is noted and unused:** nothing in this task
quotes a score, a delta or a "+1 row", from one run or any number of runs.

## 8. What QA should scrutinise

1. **Run the §2 text-diff.** It is the one check that does not depend on trusting
   me, and it is cheap.
2. **Confirm no token constant crept in.** Grep the file for `tok`.
3. **Confirm every string comparison is `*Sensitive`.** This is the defect from §4
   and it is invisible on a green run — a case-insensitive compare passes.
4. **`FTCHARToUTF8` carries a commented-out deprecation** in UE 5.8
   (`StringConv.h:1013`) suggesting `StringCast<UTF8CHAR>`. It is **not** actually
   deprecated and compiles, but flag it if house style prefers `StringCast`.
5. **Concurrency note:** `SiegeAssistantSnapshot.{h,cpp}` are **dirty in the
   working tree** from TASK-441 (`ValidateCommandAgainstSnapshot`,
   `GetOrderableCount`, `IsKindOrderable`, inserted *before* `BuildZoneA`).
   I verified the pending diff **adds and removes zero `Out += TEXT` lines**, so
   no emitted prompt byte moves. **I edited no shared file** — this task is one
   new file, which is the smallest possible footprint under the quiet-module law.
6. **If the test ever goes red, the correct response is to RE-MEASURE**, never to
   edit the frozen fixture. The file says so in three places; please keep it that
   way.
