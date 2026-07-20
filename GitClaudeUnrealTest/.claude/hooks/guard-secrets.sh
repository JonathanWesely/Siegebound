#!/usr/bin/env bash
# PreToolUse guard (Edit|Write): deny writes whose content contains token-like secrets.
# Scans the raw hook-input JSON directly so it needs no JSON parser (no jq on this machine).
input=$(cat)

deny() {
  printf '{"hookSpecificOutput":{"hookEventName":"PreToolUse","permissionDecision":"deny","permissionDecisionReason":"Secret guard: %s. Use a placeholder or a non-committed local config instead of a real credential."}}\n' "$1"
  exit 0
}

# High-confidence token formats (AWS, GitHub, Slack, Anthropic/OpenAI, Google, JWT, PEM keys,
# Meshy msy_ — observed prefix, TASK-198)
if printf '%s' "$input" | grep -qE 'AKIA[0-9A-Z]{16}|gh[oprsu]_[A-Za-z0-9]{36,}|github_pat_[A-Za-z0-9_]{22,}|hf_[A-Za-z0-9]{20,}|msy_[A-Za-z0-9]{20,}|xox[abprs]-[0-9A-Za-z-]{10,}|sk-(ant-|proj-)?[A-Za-z0-9_-]{24,}|AIza[0-9A-Za-z_-]{35}|eyJ[A-Za-z0-9_-]{8,}\.eyJ|-----BEGIN [A-Z ]*PRIVATE KEY-----'; then
  deny "content matches a known API-token format"
fi

# Credential-keyword assignments (SecurityToken=..., ApiKey: "...", etc.).
# Value must be 16+ chars AND contain a digit, so identifier-only lines and
# placeholders like YOUR_PASSWORD_HERE or CreateDefaultSubobject don't trip it.
kv=$(printf '%s' "$input" | grep -oiE '(securitytoken|secret|api_?key|access_?token|auth_?token|passwd|password|credential)[a-z_]*[[:space:]]*[=:][[:space:]]*\\?"?[A-Za-z0-9+/=_-]{16,}')
if [ -n "$kv" ] && printf '%s' "$kv" | grep -q '[0-9]'; then
  deny "content assigns a credential-like value (e.g. SecurityToken=..., ApiKey=...)"
fi

exit 0
