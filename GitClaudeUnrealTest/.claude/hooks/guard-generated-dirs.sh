#!/usr/bin/env bash
# PreToolUse guard (Edit|Write): deny edits under UE-generated directories.
# Reading those dirs (e.g. Saved/Logs for QA) is unaffected; only Edit/Write is blocked.
input=$(cat)

fp=$(printf '%s' "$input" | grep -o '"file_path"[[:space:]]*:[[:space:]]*"[^"]*"' | head -1)

# Match Binaries/Intermediate/Saved/DerivedDataCache as a full path segment,
# with either separator style; leading '"' covers relative paths.
if printf '%s' "$fp" | grep -qE '["/\\](Binaries|Intermediate|Saved|DerivedDataCache)[/\\]'; then
  printf '{"hookSpecificOutput":{"hookEventName":"PreToolUse","permissionDecision":"deny","permissionDecisionReason":"%s"}}\n' \
    "Blocked: this path is inside a UE-generated directory (Binaries/Intermediate/Saved/DerivedDataCache). These are build outputs - never hand-edit them; rebuild or let the editor regenerate them instead."
  exit 0
fi

exit 0
