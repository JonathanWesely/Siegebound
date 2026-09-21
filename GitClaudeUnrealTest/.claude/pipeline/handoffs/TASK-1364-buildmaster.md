# TASK-1364 — [VRAM-BANNER-CENSUS] — build-master

**Limb carried (`SC-§134` cl. 7):** **(a) — THE DEFAULT CARVE-OUT.** My `names:` block carves out this row's own
`status:` line by name, so **I flipped it myself** (`Edit`, anchored on the unique `#### TASK-1364` header's row,
collision count measured at my own instant, `replace_all` never used — `SC-§120` / `SC-§127` / `SC-§138`).
Not limb (b): no flipper is named on the row, and the line is not known-stale by design.

**Instant:** my keystrokes are **2026-09-21 ~00:05–00:12 PDT**. The wave's events are dated **2026-09-20** by the
manager **deliberately** — two clocks, both right. ⛔ I have not "corrected" any manager date.

## 🚨 NO CAUSE IS ATTRIBUTED

**No cause is attributed to the VRAM banner by this row.** Nothing below is a diagnosis, a mechanism, a
proposed fix or a setting change. This row measured whether the escalation is real and whether it is one
instrument, and it censused the process field read-only. **Everything past a measurement is labelled a
hypothesis and left standing.**

---

## 1. The three sightings, re-verified AT SOURCE

⛔ Nothing below is taken on relay. Each figure is quoted from the artefact that actually carries it.

### Sighting 1 — `8.242 MB` — ✅ **IT SURVIVES. The manager's figure is REAL.**

| field | measured |
|---|---|
| source | **`.claude/pipeline/qa/TASK-1314-verify.md`** — the figure appears at **line 100** (Evidence block) and again at **line 103** (the VRAM paragraph) |
| verbatim | **`Video memory has been exhausted (8.242 MB over budget). Expect extremely poor performance.`** — the **complete** sentence, not truncated |
| frame | SOURCE `Saved/AuraVerify/pie_composited_c1_t84.53s_f1028360.png` (1,359,958 bytes, 1086×615, PIE `t=84.53 s`, frame 1028360) → TARGET `.claude/pipeline/playtest-evidence/2026-09-20/VER-TASK-1314-t01m24s-focus-holder-at-match-end.png` |
| **limb** | 🚨 **LIMB B — NOT Limb A.** See §1.1. |
| **editor PID** | 🚨 **22560** (`TASK-1314-verify.md:26`, `os.getpid()` inside the Aura-hosted editor) — **NOT 27484** |
| instance log | `Saved/Logs/GitClaudeUnrealTest.log`, line 1 `Log file open, **09/19/26 20:07:36**` |
| map | `/Game/Maps/L_Arena.L_Arena` (`:25`) |
| in the log? | **NO — 0 occurrences.** The report measured `"Video memory has been exhausted"` = 0 and `"over budget"` = 0 in its 59,875-byte slice ⇒ **pixel-only** |
| co-conditions | `60 FPS / 16.7 ms` on the **same** frame; match ended normally; pre-PIE `is_pie_active` read GPU `budget 7123 MB / usage 2521 MB / available 4601 MB` (RTX 5070 Laptop) |

#### 1.1 ⛔ CORRECTION OWED TO THE ROW: the limb label is wrong on the board

The row's spec (1) says ``8.242 MB` — ⭐ `TASK-1314` ⛔ **LIMB A**`` and the `names:` READS line says
*"the ⭐ `TASK-1314` **LIMB A** verify report"*. **Both are wrong; the dispatch's "Limb B" is right.** Measured:

- `TASK-1314-verify.md:89`, in the section that carries the 8.242 frame: *"**At Limb A** the guard `Warning` sat
  **between** these two lines. **Here** there is **nothing** between them."* ⇒ "here" ≠ Limb A.
- `TASK-1314-verify.md:27`: *"PARTIAL, by cl. 9, **AND STRONGER THAN LIMB A'S** … (**Limb A's** `LogInit: Command
  Line:` was empty)"* ⇒ the first section is the later limb.
- `.claude/pipeline/handoffs/TASK-1316-buildmaster.md:173`: *"drawn on the promoted **LIMB B** frame"*, quoting the
  same `8.242 MB` string.

⇒ **`qa/TASK-1314-verify.md` is ONE file carrying BOTH limbs.** Limb B is the first section (**PID 22560**, log
open `09/19/26 20:07:36`); **Limb A** is the second section from `:185` onward (**PID 6764**, log open
`09/19/26 18:54:06`, map also `/Game/Maps/L_Arena.L_Arena`). **The banner is on Limb B's frame only.**
⇒ ⛔ a manager line to correct; ⛔ not mine to edit (`SC-§50`).

#### 1.2 The `TASK-1358` NIT-2 correction was RIGHT, and so was the manager. Both.

`qa/TASK-1358-report.md:372–373` verbatim:

> **[NIT-2]** VRAM banner provenance — my dispatch cites *"an earlier one read 8.242 MB"*; **the earlier figure in
> the two reports I read is `172.082 MB`** (`TASK-1352`, both frames). I record what I measured.

⭐ **This is not a contradiction and neither party erred.** The gate read **two** reports (1352, 1357) and refused
to assert a figure from a **third** it had not opened — exactly correct (`SC-§138`). The `8.242` figure genuinely
exists, in that third report. **The gate was right to refuse it; the manager was right that it exists.** What was
wrong was only the **limb label**, and that is corrected at §1.1.

### Sighting 2 — `172.082 MB` — ✅ confirmed at source

| field | measured |
|---|---|
| source | **`.claude/pipeline/qa/TASK-1352-verify.md`** — **line 34** (header condition) and **line 162** (frame a1 description) |
| verbatim | **`Video memory has been exhausted (172.082 MB over budget). Expect extremely poor…`** — truncated **at the frame edge**, not a different string |
| editor PID | **27484**, command line `''` (EMPTY), `-game` **absent** (`:26`, `:32`, `:239`) |
| map | `/Game/Maps/L_Arena.L_Arena` (`:239`) |
| frame(s) | `VER-TASK-1352-a1-ghost-after-setmouselocation_t50.31s_f1883165.png` carries it verbatim |
| co-conditions | `59 FPS / 16.8 ms` on that frame; `Gold: 60`; two PIE sessions; **no editor lifecycle action taken** |

⚠️ **ONE UNRESOLVED TENSION AT SOURCE, REPORTED NOT RESOLVED.** The report's header (`:34`) asserts the banner was
*"visible on **both frames**"*, but its Evidence section lists **THREE** frames (`:158`–`:175`) and quotes the
banner verbatim on **exactly ONE** of them (a1, `:162`). The a2 frame — the report's own *"⭐ THE LOAD-BEARING
FRAME"* — is described with `Gold: 24`, `60 FPS / 16.7 ms`, and **no banner is mentioned either way**; the a1-sweep
frame likewise. ⇒ **The figure `172.082` is solid; the count of frames carrying it is not established by the
report's own body.** ⛔ I did not open the PNGs to settle it — that is a pixel adjudication, not this row's lane.

### Sighting 3 — `382.789 MB` — ✅ confirmed at source, **including its absence**

| field | measured |
|---|---|
| source | **`.claude/pipeline/qa/TASK-1357-verify.md`** — **lines 39–40** (header) and **line 142** (frame a3/"C") |
| verbatim | **`Video memory has been exhausted (382.789 MB over budget). Expect extremely poor…`** — truncated at the frame edge |
| editor PID | **27484**, command line `''` (EMPTY), `-game` **absent** (`:29`, `:37`, `:218`) |
| map | `/Game/Maps/L_Arena.L_Arena` (`:218`) |
| **absent on frame A** | ✅ **CONFIRMED AT SOURCE, AFFIRMATIVELY.** `:128` states of frame A: *"**No VRAM banner on this frame.**"* — an explicit negative, not a silence ⇒ **intermittent WITHIN one run**, as the row says |
| co-conditions | `60 FPS / 16.7 ms` on both A and C; three PIE sessions; **no editor lifecycle action taken** |

---

## 2. Is it ONE instrument? — **YES by string, units and emitter. NO as "three samples of one accumulator."**

### 2.1 The string is identical

All three read the same engine format string, same units (`N MB over budget`), same trailing sentence:

```
Video memory has been exhausted (N MB over budget). Expect extremely poor performance.
```

`TASK-1314` carries the **complete** sentence. `TASK-1352` and `TASK-1357` end at `Expect extremely poor…` — and
**that ellipsis is a frame-edge crop, not a different message**: both reports render it mid-frame on a 1086×615
composite. ⇒ ⛔ **no string divergence; this is not three messages wearing one sentence.**

### 2.2 The emitter is identified — and it is NOT process-scoped

Already measured and on record at `.claude/pipeline/handoffs/TASK-1081-buildmaster.md:159–161` (quoted, not
re-derived by me):

> The banner **`Video memory has been exhausted (N MB over budget). Expect extremely poor performance.`** is the
> **D3D12 adapter** message — DXGI `QueryVideoMemoryInfo` **whole-GPU local budget, across every process on the
> machine**. It is **NOT** the texture-streaming pool.

⇒ 🚨 **The quantity is machine-wide and instantaneous.** ⛔ I draw no conclusion from that here; it is recorded
because §4 cannot be read honestly without it.

### 2.3 All three are PIXEL-ONLY — and I measured that for the two that had not been measured

- `TASK-1314` measured it for itself: string count **0** in its log slice.
- `TASK-1352` and `TASK-1357` **never counted it** (1357 states plainly that *no log line is quoted as evidence
  anywhere in this report*). **So I counted it, read-only, at my own instant:**

| probe over `Saved/Logs/GitClaudeUnrealTest.log` (831,310 bytes; line 1 = `Log file open, 09/20/26 11:42:10`) | count |
|---|---|
| `Video memory has been exhausted` | **0** |
| `over budget` | **0** |
| **CONTROL — `LogInit`** (must be > 0) | **84** ✅ |

⭐ That log is **PID 27484's own**, and it spans **both** the `TASK-1352` and `TASK-1357` runs (opened 11:42:10,
un-rotated, still the live file). ⇒ **Neither the `172.082` nor the `382.789` reading appears in any log** — they
are reads of **rendered pixels**, exactly as `8.242` was. The control at 84 proves the zeros are measured zeros,
not a broken probe (`SC-§137`).

### 2.4 Therefore, precisely

✅ **Same instrument** — same emitter, same string, same units, all three pixel-only.
⛔ **NOT three successive samples of one accumulating counter.** Each is an *instantaneous* sample of a
*whole-machine* budget delta, taken by three different readers off three different composited PNGs at three
different instants. **A rising sequence of three instantaneous whole-machine samples is not, by itself, a
trend in any one process** (`SC-§137`).

### 2.5 ⚠️ THE TRIO IS A SUB-SELECTION OF A LARGER, NON-MONOTONE POPULATION

Read-only census of the identical string across `.claude/pipeline/`. **Ten** recorded readings exist:

| MB over budget | source | date context |
|---|---|---|
| **0.922** | `footage/VID-004-contact-climb-confirmed-visual-punchlist.md:71` (3× crop) | earlier wave |
| **8.242** | `qa/TASK-1314-verify.md:100` | **2026-09-20** ⟵ trio |
| **15x.xxx** | `qa/TASK-671-verify.md:14` (digits not legible in source) | earlier wave |
| **23.391** | `handoffs/TASK-1081-buildmaster.md:181` | earlier wave |
| **168.906** | `qa/TASK-1068-verify.md:14` | earlier wave |
| **172.082** | `qa/TASK-1352-verify.md:34` | **2026-09-20** ⟵ trio |
| **382.789** | `qa/TASK-1357-verify.md:39` | **2026-09-20** ⟵ trio |
| **450.516** | `handoffs/TASK-663-buildmaster.md:81` | earlier wave |
| **828.144** | 🧑 Jonathan's 2026-09-06 screenshot, `CONVENTIONS.md:11526` | earlier wave |
| **2798.546** | `footage/VID-007-fog-visibility-swings-4x.md:113` | earlier wave |

🚨 **MEASURED, NOT INTERPRETED:** the trio is monotone **within itself**, but readings **far below** (`0.922`) and
**far above** (`2798.546`, ~7.3× the trio's maximum) are **already on record from earlier dates**. The three
figures rank **2nd, 6th and 7th smallest** of the ten. ⇒ **"Escalating" is true of the three sampled; it is not
true of the population over time.** ⛔ **No cause is attributed, and I do not claim the population is the right
comparison set — I claim only that it exists and that the row's trio is a subset of it.**

---

## 3. The read-only process census — AT MY OWN INSTANT, ACTING ON NOTHING

Method: `Get-CimInstance Win32_Process` — **read-only**, **by command line** (`SC-§118` cl. 1). ⛔ No process was
started, stopped, signalled or touched.

### 3.1 The editor

| field | measured |
|---|---|
| image | `UnrealEditor.exe` |
| **PID** | **27484** |
| ParentProcessId | 27812 |
| **CreationDate** | **2026-09-20 11:42:09** |
| **CommandLine** | `"C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" "C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\GitClaudeUnrealTest.uproject"` |
| `-game` token | ⛔ **ABSENT** ⇒ **GUI editor, not 🧑 his** |
| switches | **none at all** — plain `.uproject` |
| WorkingSet | **2683.6 MB** |
| PageFileUsage (private) | **9383.8 MB** |

⭐ **UNRESTARTED, UNTOUCHED, across NINE consecutive rows now.** `2026-09-20 11:42:09` is **identical** to the
CreationDate `TASK-1361` censused. **Corroborated independently of WMI:** the live log's own line 1 reads
`Log file open, **09/20/26 11:42:10**` — one second after process creation.

### 3.2 Every other PID classified

| PID | image | classification |
|---|---|---|
| 15612 | `UnrealTraceServer.exe` | **child daemon of 27484** — cmdline `daemon -d --sponsor 27484`, created `2026-09-20 11:42:10` |
| 3276 | `LiveCodingConsole.exe` | 27484's Live Coding console — `-Group=UE_GitClaudeUnrealTest_0x4c213b44 -Hidden -ProjectName="GitClaudeUnrealTest"`. ⛔ **Present, and NOT invoked by me** |
| 28808 | `EpicWebHelper.exe` | CEF child, `--type=gpu-process` |
| 18600 | `EpicWebHelper.exe` | CEF child, `--type=utility … NetworkService` |
| 29860 | `EpicWebHelper.exe` | CEF child, `--type=utility … StorageService` |
| 26184 | `python.exe` | `Tools/blender_mcp_bridge.py` (Blender 5.1 bridge) — project-named, **not a UE instance** |
| 28932 | `cmd.exe` | **my own census shell** (Claude Code launcher) — declared, not hidden |

### 3.3 🧑 `-game` instances: **ZERO**, and the zero is MEASURED

| probe over **all 348** running processes | count |
|---|---|
| CommandLine contains ` -game` | **0** |
| image name matches `*Unreal*` or `*GitClaude*` | **2** ✅ |
| CommandLine contains `.uproject` | **1** ✅ |
| CommandLine contains `GitClaudeUnrealTest` | **7** ✅ |
| **TOTAL processes enumerated** | **348** ✅ |

⭐ `SC-§137` complement control: **four** non-zero counts from the **same** enumerator prove the probe fires.
⇒ **The `-game` zero is a measured absence, not the silence of a broken instrument.**
⇒ **Nothing of 🧑 Jonathan's is running; nothing of his was found, touched or asked about.**

### 3.4 🚨 THE DISCLAIMER THE ROW REQUIRES, STATED FLATLY

**WorkingSet `2683.6 MB` and private bytes `9383.8 MB` are SYSTEM memory** — host RAM and commit charge, reported
by `Win32_Process`. **The banner's `N MB over budget` is VIDEO memory** — the DXGI adapter's local budget.
🚨 **These are different quantities, on different devices, from different instruments.** ⛔ **No inference is
drawn from either to the other, and none is licensed by this report.** A large private-bytes figure is **not**
evidence about VRAM, and I do not offer it as such.

---

## 4. (4) PID continuity — **THE ANSWER IS NO**, and the hypothesis is NAMED AND LEFT

### 4.1 Measured

| sighting | figure | editor PID **as its own report records it** | that instance's log opened |
|---|---|---|---|
| `TASK-1314` **Limb B** | `8.242 MB` | 🚨 **22560** | `09/19/26 20:07:36` |
| `TASK-1352` | `172.082 MB` | **27484** | `09/20/26 11:42:10` |
| `TASK-1357` | `382.789 MB` | **27484** | `09/20/26 11:42:10` |

*(For completeness: `TASK-1314` **Limb A**, which carries **no** banner sighting, ran on a **third** instance,
**PID 6764**, log open `09/19/26 18:54:06`.)*

### 4.2 🚨 THE ROW'S HEADER PREMISE IS MEASURED FALSE

The row's header reads *"THREE ESCALATING … READINGS ACROSS THREE RUNS, **SAME DAY**, **SAME MAP**, **SAME EDITOR
PID**"*. Measured against source:

- ✅ **SAME DAY — HOLDS.** All three runs are **2026-09-20** local (1314's report written 01:38; 1352's 20:33;
  1357's 23:37).
- ✅ **SAME MAP — HOLDS.** `/Game/Maps/L_Arena.L_Arena` in all three, quoted from each report.
- ⛔ **SAME EDITOR PID — FALSE.** **22560 ≠ 27484.** PID 27484 was **created 2026-09-20 11:42:09**; the `8.242`
  sighting occurred in a **different process**, launched the **previous evening** and gone before 27484 existed.

### 4.3 What that kills, and what it does not

⇒ **The cheapest explanation is DEAD IN THE FORM STATED:** *"one editor has been up all day across all three
sightings"* is **false**. `TASK-1364`'s (4) says an answer of *no* here is worth as much as a yes, and this is it.

📌 **HYPOTHESIS, NAMED AND NOT TAKEN (`SC-§101`):** a **weaker, per-instance** variant remains *available* — that
each sighting occurred some hours into **its own** instance's life. ⛔ **I have NOT measured any per-instance VRAM
trajectory**, ⛔ have **not** shown that uptime and the figure co-vary, and ⛔ **have not** tested it in any way.
⭐ **Excluding one alternative is not establishing a mechanism.** Ruling out the single-instance story does
**nothing whatever** to support the per-instance story.

⭐ **AND A REASON TO BE SLOWER STILL, NOT FASTER:** §2.2's measured emitter fact — the banner is a **whole-GPU,
all-process** DXGI budget — means a **process-scoped** hypothesis may be the **wrong shape entirely**. Other GPU
consumers on this machine are visible in this very census (three `EpicWebHelper` CEF children, one with
`--type=gpu-process`; `TASK-1081` recorded `EpicWebHelper` *"already on the GPU"* at 6455 MiB used before any PIE).
⛔ **I am not claiming that either.** It is named so that the next reader does not treat "not one long-lived
editor" as though it implied "therefore per-instance uptime."

**I STOP HERE.**

---

## 5. (5) Impact — nothing is invalidated, nothing is re-opened

**`TASK-1352` and `TASK-1357` stand entirely.** Both **correctly recorded** the banner and **neither leaned on a
pixel under it**: 1352's numbers come from property reads and `ui_snapshot`, 1357's B1 answer is three
bit-identical coordinate triples from tool replies. `TASK-1314` likewise measured the banner's log-absence and
explicitly excluded it from its acceptance lines. `VER-§6` cl. 6 already governs this exact hazard
(*quote numbers from tool replies, never pixels under a banner*) and all three obeyed it.

⛔ **NO ROW IS RE-OPENED BY THIS CENSUS.** ⭐ **Three verifiers recorded this banner and none diagnosed it. That
was correct in every case**, and this row exists because the census belongs to the lane that holds `Bash`
(`VER-§8` cl. 8), not because anybody's lane failed.

---

## 6. Scope — what this row did and did not do

- ⛔ **NO code. NO asset. NO compile. NO suite. NO PIE. NO editor lifecycle action. NO Live Coding. NO git write.
  NO push.** All nine declared, not omitted.
- ⛔ **`CONVENTIONS.md`** — read only (`SC-§134` cl. 7, to identify my limb). **Never written.**
- ⛔ **`CLAUDE.md`** — 🚨 **UNTOUCHED. NOT staged, NOT committed, and NOT REVERTED.** It sits **`3 ins / 3 del`,
  unstaged**, exactly as `TASK-1361` left it. It is **`HELD-FOR: 🧑 Jonathan's own sentence`**; **no release
  reached me in this dispatch ⇒ it is not released.** ⭐ Reverting it would destroy the evidence and is as much an
  unauthorised content act as applying it. **This is the seventh refusal.**
- ✅ **WROTE exactly two things:** this file, and **this row's own `status:` line** (`Edit`, never `replace_all`).
- ⛔ **NO COMMIT.** This row produces nothing but its board line and this handoff. ⭐ **My tail rides the NEXT
  commit host** (`TL-§5e` cl. 7a; derived at that host's instant, `SC-§133`), namely:
  1. `.claude/pipeline/handoffs/TASK-1364-buildmaster.md` (this file)
  2. `.claude/pipeline/TASKBOARD.md` (this row's two flips)
  ⛔ **No second commit is invented to swallow it.**

---

## 7. For the manager — NAMED AND LEFT (`SC-§50`)

⛔ I board nothing. Five items, in descending order of how load-bearing they are:

1. 🚨 **THE ROW'S LIMB LABEL IS WRONG.** Spec (1) and `names:` say `TASK-1314` **LIMB A**; the `8.242` frame is
   **LIMB B** (PID 22560). Proof at §1.1. **A manager line to correct.**
2. 🚨 **THE ROW'S HEADER PREMISE "SAME EDITOR PID" IS MEASURED FALSE** (§4.2). The trio spans **two** editor
   instances, not one. *Same day* and *same map* both hold.
3. ⚠️ **`TASK-1352`'s "both frames" is not corroborated by its own evidence body** (§ Sighting 2) — banner quoted
   verbatim on 1 of the 3 listed frames. The **figure** is unaffected; the **frame count** is open. Settling it
   needs a pixel read of two PNGs — ⛔ not this row's lane.
4. ⚠️ **THE ESCALATION IS A 3-SAMPLE SUB-SELECTION OF A 10-SAMPLE, NON-MONOTONE POPULATION** (§2.5), with
   `2798.546 MB` already on record from an earlier date — ~7.3× the trio's maximum.
5. 📌 **IF a real VRAM measurement is ever wanted**, the instrument already exists on record: `TASK-1081`'s
   converged **10-samples-at-2 s** method (`SC-§88`), which distinguishes a converged state from one frame, and
   which produced this project's only whole-GPU used/free table. ⛔ **I name it. I do not propose it, scope it or
   recommend it.**

## 🚨 CLOSING LINE, AS THE ACCEPTANCE REQUIRES

**NO CAUSE IS ATTRIBUTED.** The escalation across the three sampled sightings is **real as arithmetic**
(`8.242 → 172.082 → 382.789`), the instrument is **the same instrument** in all three, the readings are **all
pixel-only** with **zero log occurrences**, and the **single-long-lived-editor explanation is measured false**.
**Why the number is what it is remains unmeasured, and this row does not guess.**
