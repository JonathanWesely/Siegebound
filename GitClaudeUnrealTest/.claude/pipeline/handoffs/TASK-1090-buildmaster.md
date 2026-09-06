# TASK-1090 — [MESHY-SHIP] handoff (build-master)

**Marker:** `TASK-1090-MESHY-SHIP` · **Law:** `SC-§83` · `SC-§87` · `SC-§68` · `CHAR-§3`
**Gate honoured:** `qa/TASK-1089.md` — **PASS, 0 BLOCKER · 7 WARN · 3 NIT** (read and verified on disk before staging).
**Status set to:** `done`

---

## 0. 🚨 THE STATE CHANGED UNDER THE DISPATCH — READ THIS FIRST

The dispatch said `HEAD = 05c3fc0`, **ahead 30, not pushed**. **That was no longer true when I started.**

| | dispatched | measured |
|---|---|---|
| `HEAD` | `05c3fc0` | **`09b89cd`** — *"fog works!"* |
| ahead / behind `origin/main` | 30 / 0, unpushed | **0 / 0 — `origin/main` IS `09b89cd`** |

🧑 **Jonathan committed `09b89cd` himself and PUSHED it**, discharging the entire 30-commit backlog. This is the known pattern (memory: *"Jonathan self-commits milestones"*) and exactly why the pre-commit HEAD/origin check exists. I did **not** amend, rebase, or re-land anything.

⭐ **His commit also swept `.claude/agents/qa-reviewer.md`** — one of the two files the dispatch told me to leave unstaged. It is therefore **no longer dirty**; there was nothing for me to avoid. The other, `Content/UI/WBP_CardHand.uasset`, **is** still dirty and I left it untouched (`TASK-1080`'s host).

⇒ **New ahead-count after my commit is `1`, not `31`.** NOT pushed.

---

## 1. ⭐ THE ONE THING QA COULD NOT CHECK — THE SCOPE CLAIM, CONFIRMED AGAINST `git`

QA holds no `git`, so it could only establish the *in-file* half of the scope claim. I ran the other half.

```
$ git status --porcelain -- Tools/ArtPipeline/
 M Tools/ArtPipeline/meshy_generate.py
?? Tools/ArtPipeline/test_meshy_multiimage.py
```

✅ **EXACTLY TWO FILES. No third path. No scope blocker.** Diffstat `+245 / −14`, one file — **matching the handoff's declared numbers exactly**.

### ⛔ The `Inbox/` crops: VERIFIED gitignored — not assumed, not force-added

`TASK-1087`'s crops were **not** taken on the author's word. Measured with `git check-ignore -v`:

```
.gitignore:25:Tools/ArtPipeline/Inbox/*   Tools/ArtPipeline/Inbox/MainCharacter_Front.png
.gitignore:25:Tools/ArtPipeline/Inbox/*   Tools/ArtPipeline/Inbox/MainCharacter_Side.png
.gitignore:25:Tools/ArtPipeline/Inbox/*   Tools/ArtPipeline/Inbox/MainCharacter_Back.png
.gitignore:25:Tools/ArtPipeline/Inbox/*   Tools/ArtPipeline/Inbox/MainCharacter_Detail
```

✅ All 30 `Inbox/` entries resolve to the **same ignore rule**, `.gitignore:25`. **None appears as untracked** ⇒ none was force-added. `Cache/` likewise never surfaced. Nothing forced.

---

## 2. ⭐⭐ THE MUTATION — I RAN IT, AND THE RED IS MINE, NOT A QUOTATION

`SC-§83` was discharged at source by the author; Python is executable here, so I **re-earned it independently** rather than inheriting it.

**Anchor uniqueness re-measured before touching anything:** `if missing:` = **1 occurrence, line 935 of 1908**. Unique, exactly as QA reported.

| step | result |
|---|---|
| **BASELINE** `uv run test_meshy_multiimage.py` | ✅ **55/55, exit 0** |
| **MUTANT** line 935 `if missing:` → `if False:` | ⛔ **51/55, exit 1** |
| **RESTORE** from scratch copy | ✅ **byte-exact** |
| **POST-RESTORE** re-run | ✅ **55/55, exit 0** |

**The witnessed red, verbatim:**
```
  [FAIL] exit code is 5 (input missing)  -- got 0
  [FAIL] no task was created (create/poll/download never entered)  -- 1 call(s) - must be 0
  [FAIL] the message names the missing file  -- TestKnight_Back.png
  [FAIL] the message says MISSING and names the exit code
51/55 assertions passed.
```

⭐ **The detail is the point, not the word FAIL.** `got 0` alongside **`1 call(s)`** means the mutant **generated from two views and reported success** — the exact `SC-§36.1` shape (*a surface that reviews clean and does the wrong thing*) this project has shipped twice. The guard is load-bearing, and the test genuinely detects its removal.

### ⛔ The restore was done the safe way, and the hash proves it

`git restore` would have **deleted the entire uncommitted diff**, not the mutation. I restored from a scratch copy at
`C:\Users\wesel\AppData\Local\Temp\claude\...\scratchpad\task1090\meshy_generate.py.ORIG` and verified **three** ways:

- `sha256 04c0d0ba0fa9902d12392cbfe1d004ebe490686342f1b68ab413d8e9d9343fbc` — **identical to the author's own declared post-restore hash**
- `cmp` → IDENTICAL (both files)
- `git diff --stat` → back to **+245 / −14, one file**

⚠️ **Recorded for the next agent:** `sed -i` in Git Bash **rewrote every line ending in the file** — `diff` reported `1,1908c1,1908`, i.e. all 1908 lines changed, for a one-token edit. Semantically irrelevant to the mutant run and fully undone by the `cp` restore, but a `sed -i` mutation restored by anything *other* than a byte-exact copy would have silently committed a whole-file CRLF→LF churn on top of the real diff. **The scratch-copy rule is not ceremony; it is what caught this.**

---

## 3. THE STANDING PREFLIGHT — `--check`, LIVE, EXIT `0`

`uv run meshy_generate.py --check`, bounded per `SC-§87` (150 s cap; it returned well inside it — **no hang, no expiry**):

```
[meshy] --check: degeneracy guard control PASSED - 11 fixtures (8 rejected, 3 accepted), all 5 checks fire in isolation,
        and the control itself goes RED when the guard is broken (7 disagreements with a never-rejects guard, 10 with an always-rejects one).
[meshy] API key resolved from process env MESHY_TOKEN (value redacted everywhere).
[meshy] --check: probing https://api.meshy.ai (free read endpoints; no credit spend)
[meshy]   /openapi/v1/balance - OK (credits remaining: 3200)
[meshy]   /openapi/v1/retexture - OK (list endpoint reachable, 0 task(s) in first page)
[meshy]   /openapi/v1/image-to-3d - OK (list endpoint reachable, 0 task(s) in first page)
[meshy]   /openapi/v1/multi-image-to-3d - OK (list endpoint reachable, 0 task(s) in first page)
[meshy] --check PASSED: key valid, API reachable, documented endpoints present.
```

**Exit code: `0`, reported verbatim.**

- ✅ **`/openapi/v1/multi-image-to-3d` is reachable on this account** — `TASK-1091`'s Branch A is live, now confirmed by me and not only by the author.
- ✅ **CREDITS: 3200, UNCHANGED.** I spent **zero**. A real generation is `TASK-1091`'s, and double-spending would have stalled that lane for a day.
- ✅ **NO SECRET LEAKED.** The tool names the *env var* (`MESHY_TOKEN`) and redacts the value. No token appears in this note, the commit, or any log excerpt.

---

## 4. NO COMPILE, NO EDITOR

**No C++ changed** — the diff is Python tooling only ⇒ **no UE build required**, so there was no log to parse and `$LASTEXITCODE` never entered it.

⛔ **The editor was never touched.** `TASK-1081` holds it (PID 17008) for a measurement; I did not close it, drive it, or connect to MCP. **`L_Arena` was never opened and never saved.**

---

## 5. WHAT WAS STAGED — BY NAMED PATH, NEVER BY DIRECTORY

**5 paths, each named individually.** No `git add Tools/`, no `-A`, no `.`:

1. `Tools/ArtPipeline/meshy_generate.py` *(M, +245/−14)*
2. `Tools/ArtPipeline/test_meshy_multiimage.py` *(new)*
3. `.claude/pipeline/handoffs/TASK-1088-programmer.md`
4. `.claude/pipeline/qa/TASK-1089.md`
5. `.claude/pipeline/handoffs/TASK-1090-buildmaster.md` *(this note)*

Pathspec **derived from `TASK-1088`'s WRITES table**, not improvised.

### ⛔ Deliberately LEFT UNSTAGED (dirty, and not mine)

| path | owner |
|---|---|
| `Content/UI/WBP_CardHand.uasset` | ⭐ **`TASK-1080`** (the gated oval fix) — **left dirty** |
| `.claude/pipeline/CONVENTIONS.md` | parallel lane |
| `.claude/pipeline/TASKBOARD.md` | shared; board write-race hazard — status edited, **not staged** |
| `handoffs/TASK-1079-artist.md`, `-1082-artist.md`, `-1087-artist.md` | other commit hosts |
| `.claude/pipeline/playtest-evidence/2026-09-06/` | `TASK-1087` / `TASK-1091` |
| `Tools/ArtPipeline/Inbox/**`, `Cache/**` | **gitignored** — never forced |

🧑 `.claude/agents/qa-reviewer.md` needed no action from me: **Jonathan had already swept it into `09b89cd`.**

---

## 6. WHAT SHIPPED, IN ONE SENTENCE

`--mode multiimage` sends **all three of 🧑 his authored concept views** — `_Front`, `_Side`, `_Back` — to Meshy's multi-image endpoint as one ordered `image_urls` array, so the knight's back (**where the tattered cloak and its red cross live**) is *shown* to the solver rather than invented by it; a **named-and-absent view stops at exit 5 before any network call**, and a refused endpoint exits 4 loudly rather than **silently degrading to a single-image run**.

---

## 7. 🙋 FOLLOW-UPS I OBSERVED (⛔ manager's to board — not mine to fix)

These are **QA's**, carried forward so they are not lost. I changed no code:

1. **WARN-1** — `parse_views()` **superset fence** + `view_count` / `view_order` on the `SUCCESS:` line. Today a **2-view run and a 3-view run both exit 0** with a success line that never names the count. This is the one that most directly threatens 🧑 his **twice-stated** "follow the views" instruction.
2. **WARN-6** ⛔ **PRE-EXISTING, affects all three modes** — `state.json` / `state_failed.json` are written to `CACHE_DIR / asset` with **no `_require_inside_cache()`**. The GLB itself cannot escape (`commit_staged` confines it), but a traversal-shaped `CardID` on argv could place a JSON file outside `Cache/`. Out of `TASK-1088`'s fence; **not** a condition on this PASS.
3. **WARN-4 / WARN-5 / WARN-7** — poll-404 narrative mis-attribution, a missing `retexture` control case, stale `--ai-model` help (`meshy-7` is undocumented in the tool but live in the API enum).
4. ⭐ **For `TASK-1091` specifically:** run `--mode multiimage MainCharacter` with **NO `--views`**; on **exit 5, produce the missing crop — never narrow the set to route around the block.** And **read the first create's response body before concluding "gated"** — a 400 naming a parameter is a params problem, not a plan problem.

---

## 8. WHAT THIS COMMIT DOES **NOT** PROVE (`SC-§94` cl. B)

⛔ **NO MESH HAS BEEN GENERATED AND NO CREDIT HAS BEEN SPENT.** 55/55 green, a witnessed red, and a live `--check` establish that **the route is present and the payload is correct**. They do **not** establish that Meshy returns a usable multi-view mesh for this character. **That is `TASK-1091`'s to answer, and only there.**
