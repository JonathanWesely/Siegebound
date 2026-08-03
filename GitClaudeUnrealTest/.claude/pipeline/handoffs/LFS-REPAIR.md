# LFS REPAIR — llama.cpp binaries committed as raw blobs

**Date:** 2026-08-02
**Performed by:** build-master
**Status:** COMPLETE — verified, NOT pushed

## Summary

Jonathan's self-committed Follow batch swept 22 llama.cpp binaries into Git as **raw blobs**
because the repo-root `.gitattributes` had no `*.dll` / `*.lib` LFS rules. The commit was the
unpushed tip, so it was rewritten in place with the binaries routed through the LFS filter.

## Hashes

| | Hash | Files |
|---|---|---|
| **Old (bad) commit** | `20c8e48c4eb66d3ab8cd5d763f62d275590bd7e2` | 78 |
| **New (repaired) commit** | `e70f5ba` | 79 (78 + `.gitattributes`) |
| **Parent (untouched)** | `50ec1f3` | — |

Commit message reused verbatim: `follow command, and new miner behavior rework`
Original author + author date preserved (`Jonathan Wesely <wesely.jonathan@gmail.com>`,
`2026-08-02T16:52:59-07:00`).

`20c8e48` remains reachable via `git reflog` (`HEAD@{2}`) if rollback is ever needed.

## Scope correction

The briefing said 17 DLLs. The actual count is **19 DLLs + 3 `.lib` = 22 binaries**.
No other `.dll`/`.lib` files are tracked anywhere in the repo, so the new rules affect
exactly these 22 paths and nothing else.

## `.gitattributes` change

Targeted byte-append to `C:\GitProjects\GitHub\GitClaudeUnrealTesting\.gitattributes`
(no parse-and-rewrite). File uses **CRLF**; the append matched it.

```
*.dll filter=lfs diff=lfs merge=lfs -text
*.lib filter=lfs diff=lfs merge=lfs -text
```

- Before: **305 bytes**, CRLF, trailing CRLF
- After: **391 bytes** (305 + 86 = 2 lines x 43 bytes), CRLF preserved
- `git diff --stat`: `1 file changed, 2 insertions(+)` — zero deletions, no existing byte touched

## Proof 1 — `git check-attr`

```
$ git check-attr filter -- GitClaudeUnrealTest/Plugins/SiegeLlama/Source/ThirdParty/LlamaCpp/bin/Win64/ggml-vulkan.dll
GitClaudeUnrealTest/Plugins/SiegeLlama/Source/ThirdParty/LlamaCpp/bin/Win64/ggml-vulkan.dll: filter: lfs
```

All 22 binaries verified bound (`check-attr --stdin` -> 22 x `filter: lfs`).

## Proof 2 — `git lfs status` (excerpt, "Objects to be committed")

```
.gitattributes (Git: eceac58 -> Git: 3775398)
.../LlamaCpp/bin/Win64/ggml-base.dll (LFS: 1132f6b)
.../LlamaCpp/bin/Win64/ggml-cpu-alderlake.dll (LFS: 6599201)
.../LlamaCpp/bin/Win64/ggml-cpu-cannonlake.dll (LFS: d2179de)
.../LlamaCpp/bin/Win64/ggml-cpu-cascadelake.dll (LFS: 728c3f8)
.../LlamaCpp/bin/Win64/ggml-cpu-cooperlake.dll (LFS: b1bedec)
.../LlamaCpp/bin/Win64/ggml-cpu-haswell.dll (LFS: 9a7abc1)
.../LlamaCpp/bin/Win64/ggml-cpu-icelake.dll (LFS: 9dedb40)
.../LlamaCpp/bin/Win64/ggml-cpu-ivybridge.dll (LFS: fd2a2f6)
.../LlamaCpp/bin/Win64/ggml-cpu-piledriver.dll (LFS: e582745)
.../LlamaCpp/bin/Win64/ggml-cpu-sandybridge.dll (LFS: 26945fb)
.../LlamaCpp/bin/Win64/ggml-cpu-sapphirerapids.dll (LFS: b4f8377)
.../LlamaCpp/bin/Win64/ggml-cpu-skylakex.dll (LFS: 10d2c4e)
.../LlamaCpp/bin/Win64/ggml-cpu-sse42.dll (LFS: 93ac39f)
.../LlamaCpp/bin/Win64/ggml-cpu-x64.dll (LFS: 00c7875)
.../LlamaCpp/bin/Win64/ggml-cpu-zen4.dll (LFS: 00ee95b)
.../LlamaCpp/bin/Win64/ggml-vulkan.dll (LFS: 6e544d6)
.../LlamaCpp/bin/Win64/ggml.dll (LFS: 566679f)
.../LlamaCpp/bin/Win64/libomp140.x86_64.dll (LFS: 4a20c1e)
.../LlamaCpp/bin/Win64/llama.dll (LFS: 080eaf3)
.../LlamaCpp/lib/Win64/ggml-base.lib (LFS: fec9807)
.../LlamaCpp/lib/Win64/ggml.lib (LFS: c99b60b)
.../LlamaCpp/lib/Win64/llama.lib (LFS: 8aca5a4)
```

Headers / `.cpp` / `.h` / `.Build.cs` correctly stayed `Git:`.

## Blob size — before / after

| | In-tree blob |
|---|---|
| `ggml-vulkan.dll` before (`20c8e48`) | **52,332,032 bytes** |
| `ggml-vulkan.dll` after (`e70f5ba`) | **133 bytes** (LFS pointer) |
| All 22 binaries before | **74,171,256 bytes** (~70.7 MB) |
| All 22 binaries after | **2,893 bytes** |

Pointer content confirms the payload is preserved:

```
version https://git-lfs.github.com/spec/v1
oid sha256:6e544d654669db65d7694914b86ffc1c1fce194b129adc1f1ad6f8fe6cfaee48
size 52332032
```

## Verification of the repair

- `git diff --name-status 20c8e48 e70f5ba` -> **23 `M`, zero `A`, zero `D`** — identical path set,
  no file gained or lost.
- `git diff --stat 20c8e48 e70f5ba` -> `23 files changed, 2 insertions(+)`. The only new content
  anywhere in the repo is the two `.gitattributes` lines. The other 56 files are byte-identical.
- Insertion count moved 14,085 -> 14,153 = +66 (22 binaries x 3 pointer lines) +2 (`.gitattributes`).
- `git lfs fsck --pointers` -> `Git LFS fsck OK`
- `git lfs ls-files` -> 22 `.dll`/`.lib` entries tracked
- Working tree file on disk still the real binary: 52,332,032 bytes
- LFS object stored locally at full size: `.git/lfs/objects/6e/54/6e544d65...`
- `git status` -> working tree clean

## State

- `main` is **5 ahead of `origin/main`, 0 behind. NOTHING PUSHED.**
- No push was performed — Jonathan has not asked for one.
- No file content was edited. This was a re-commit, not an edit.
- `git reset --hard` and `git clean` were never used.

## Follow-up for the manager

1. **Push decision is Jonathan's.** The repair is only valuable while unpushed; once pushed, the
   LFS-clean version is what lands. Nothing blocks a push, but it needs his explicit go-ahead.
2. **CONVENTIONS entry:** the `*.dll` / `*.lib` LFS law is now real in `.gitattributes` but is not
   yet written into `.claude/pipeline/CONVENTIONS.md`. Worth recording so the trap cannot re-fire.
3. **Binary-extension sweep:** only `.dll`/`.lib` were added. Other binary types that could appear
   in third-party drops (`.exe`, `.pdb`, `.so`, `.dylib`, `.a`, `.gguf` model weights) still have no
   rule. `.gguf` is the notable one — `Tools/fetch_llm_model.py` downloads model weights, which are
   multi-GB and must never enter Git raw.
