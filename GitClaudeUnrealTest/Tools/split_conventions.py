r"""split_conventions.py - split CONVENTIONS.md into the naming/process core + one law file per namespace,
and (re)generate the "## Law index" that maps every namespace tag to the files holding its headings.

WHY (2026-10-04, Jonathan's rule change): CONVENTIONS.md reached 12,617 lines / 3.4 MB. Agents cite it
by namespace tag (`VER-§3 cl. 6`) and grep for the tag, so the file's size buys nothing and costs every
dispatch. The generic naming and process law stays in CONVENTIONS.md; every feature wave and every
namespace lives whole in .claude/pipeline/law/.

RULES ENCODED HERE
  * A section = one `## ` heading to the next (code fences respected). Sections move whole and
    unedited; bytes do not change.
  * KEEP: the preamble (before the first `## `) and every section whose heading starts with one of
    KEEP_PREFIXES (the LAW MAINTENANCE section that holds SC-§143 is one of them). Everything else moves.
  * Destination: a heading carrying a namespace tag `XXX-§` goes to law/<XXX>.md (several sections of
    one namespace are concatenated in original order; an existing namespace file is appended to); a
    heading without a tag goes to law/NNN-<slug>.md, where NNN continues from the highest NNN already
    in law/ so a later wave never collides with an earlier file.
  * The index lists a file under a namespace ONLY when the file holds that namespace's section heading
    or a `### XXX-§N` clause heading. Citations in body text never count.
  * The citation rule is unchanged: cite `VER-§3 cl. 6`, never a bare `§3`; a file:line is a dated annotation.

SANITY CHECKS THAT CAN FAIL (TASK-1605; law SC-§39 - a check that cannot fail proves nothing):
  * before any write: the split is lossless (the sections re-joined are the original bytes), every
    section is in exactly one of keep / move, and no NNN destination exists yet;
  * after the law files are written, each is READ BACK: a new file must be header + body, an appended
    file must be its previous bytes + body, byte for byte;
  * after the core is written it is READ BACK: its non-index bytes must equal the original minus the
    moved sections minus the old index (`--apply`), or the pre-run non-index bytes (`--reindex`), and
    its index section must be the index that was generated;
  * a failed check names the first differing file and byte offset, exits 2, and rolls back every file
    this run wrote (new law files removed, appended files and CONVENTIONS.md restored).

USAGE (Python 3.7+; allow-listed interpreter: C:\Python314\python.exe)
  python Tools/split_conventions.py             # plan only (what --apply would move)
  python Tools/split_conventions.py --apply     # move sections out, write law files, write the index
  python Tools/split_conventions.py --reindex   # rebuild only the index from the law dir + the core

EXIT CODES: 0 ok - 2 sanity check failed (this run's writes rolled back) - 64 usage
"""
import re, sys, datetime, hashlib
if hasattr(sys.stdout, "reconfigure"):
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")  # Windows console is cp1252; headings carry em dashes and emoji
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CONV = ROOT / ".claude" / "pipeline" / "CONVENTIONS.md"
LAW_DIR = ROOT / ".claude" / "pipeline" / "law"

KEEP_PREFIXES = (
    "Asset prefixes", "Texture suffixes", "Non-`Content/` pipeline artefacts", "Static-mesh SOCKET names",
    "C++ (Source/", "C++ layout", "Blueprint subclasses", "Per-card visual assets", "Card artwork",
    "Data-driven card stats", "Team contract", "World axes",
    "⛔ THE QUIET-MODULE LAW", "⛔ GIT HAZARD LAWS", "⚖️ LAW MAINTENANCE", "🔍 THE RELAYED-DIAGNOSIS LAW",
    "Damage types", "Audio event cues", "Material & Niagara lane laws", "Raw asset sources",
    "Textured mesh law", "Delegates", "Logging", "Template-donor rule", "Widgets with C++ bases",
    "Input-mode ownership", "Dev / test tooling", "Numbering", "Cross-discipline rule", "Law index",
)
TAG_RE = re.compile(r"\b([A-Z]{2,8})-§")
CLAUSE_RE = re.compile(r"^#{3,4}\s+.*?\b([A-Z]{2,8}-§\d+[a-z]?(?:\.\d+)?)\b")
NN_FILE_RE = re.compile(r"^(\d{3})-.*\.md$")
MOVED_MARK = "<!-- MOVED"


def split_sections(lines):
    sections, cur, head, fence = [], [], None, False
    for ln in lines:
        s = ln.rstrip("\r\n")
        if s.startswith("```"):
            fence = not fence
        if not fence and s.startswith("## "):
            sections.append((head, cur))
            head, cur = s[3:].strip(), [ln]
            continue
        cur.append(ln)
    sections.append((head, cur))
    return sections


def slug(heading):
    base = re.sub(r"[^A-Za-z0-9]+", "-", heading).strip("-")
    return re.sub(r"-{2,}", "-", base)[:60].strip("-") or "section"


def clause_key(c):
    parts = re.findall(r"\d+|[a-z]+", c.split("-§")[1])
    return tuple((0, int(x)) if x.isdigit() else (1, x) for x in parts)


def next_nn():
    """One above the highest NNN- prefix already in LAW_DIR (1 when there is none)."""
    hi = 0
    if LAW_DIR.is_dir():
        for f in LAW_DIR.glob("*.md"):
            m = NN_FILE_RE.match(f.name)
            if m:
                hi = max(hi, int(m.group(1)))
    return hi + 1


def build_index(sections_by_file, nl, today):
    """sections_by_file: {filename: [(heading, [lines])]} with 'CONVENTIONS.md' as the core."""
    index = {}
    for fname, items in sections_by_file.items():
        for head, bl in items:
            for tg in TAG_RE.findall(head or ""):
                index.setdefault(tg, {}).setdefault(fname, [])
            fence = False
            for ln in bl:
                s = ln.rstrip("\r\n")
                if s.startswith("```"):
                    fence = not fence
                if fence:
                    continue
                cm = CLAUSE_RE.match(s)
                if cm:
                    tg = cm.group(1).split("-§")[0]
                    index.setdefault(tg, {}).setdefault(fname, []).append(cm.group(1))
    idx = [f"## Law index — namespaces and where they live (generated {today} by `Tools/split_conventions.py`){nl}", nl,
           "The generic naming and process law stays in this file. Every feature wave and every namespace moved whole "
           "into `.claude/pipeline/law/` (bytes unchanged). **Citation rule unchanged:** cite by tag (`VER-§3 cl. 6`), never a bare "
           "`§3`; a `file:line` inside a law is a dated annotation and the tag is the key. To find a clause: "
           "`grep -rn \"VER-§3\" .claude/pipeline/law/ .claude/pipeline/CONVENTIONS.md`. A file is listed under a namespace only when it "
           "HOLDS that namespace's section or clause headings (citations do not count). Re-run `--reindex` after editing a law file; "
           f"re-run `--apply` after a new wave lands in this file to move it out.{nl}", nl,
           f"| Namespace | Files holding its sections/clauses | Clause headings found |{nl}|---|---|---|{nl}"]
    for tg in sorted(index):
        files = index[tg]
        file_list = ", ".join(f"`{('law/' + f) if f != 'CONVENTIONS.md' else f}`" for f in sorted(files))
        clauses = sorted({c for cl in files.values() for c in cl}, key=clause_key)
        shown = ", ".join(clauses[:40]) + (" …" if len(clauses) > 40 else "")
        idx.append(f"| `{tg}-§` | {file_list} | {shown} |{nl}")
    idx.append(nl)
    idx.append(f"Files without a namespace tag in their heading (feature waves) — `ls .claude/pipeline/law/`:{nl}")
    for fname in sorted(f for f in sections_by_file if re.match(r"^\d{3}-", f)):
        idx.append(f"- `law/{fname}` — {sections_by_file[fname][0][0][:120]}{nl}")
    idx.append(nl)
    return idx


def law_sections():
    """{filename: [(heading, lines)]} for every law file, header comment stripped."""
    out = {}
    for f in sorted(LAW_DIR.glob("*.md")):
        fl = f.read_bytes().decode("utf-8").splitlines(keepends=True)
        if fl and fl[0].startswith(MOVED_MARK):
            fl = fl[1:]
        out[f.name] = [(h, b) for h, b in split_sections(fl) if h is not None]
    return out


def is_index(head):
    return head is not None and head.startswith("Law index")


def core_text(core_sections, idx, nl):
    """The core file's text: the preamble, the generated index, then the kept sections in order."""
    out = []
    for i, (h, b) in enumerate(core_sections):
        out.extend(b)
        if i == 0:
            if out and not out[-1].endswith(nl):
                out.append(nl)
            out.extend(idx)
    return "".join(out)


def nonindex_bytes(text):
    """The core with its `## Law index` section removed, as bytes - the part no run may change."""
    secs = split_sections(text.splitlines(keepends=True))
    return "".join("".join(b) for h, b in secs if not is_index(h)).encode("utf-8")


def index_bytes(text):
    """The core's `## Law index` section alone, as bytes ('' when absent)."""
    secs = split_sections(text.splitlines(keepends=True))
    return "".join("".join(b) for h, b in secs if is_index(h)).encode("utf-8")


# ---- sanity checks (each callable on its own, so a driver can prove it goes red) --------------------

def sha(b):
    return hashlib.sha256(b).hexdigest()


def first_diff(a, b):
    """Offset of the first differing byte of two byte strings (len of the shorter when one is a prefix)."""
    n = min(len(a), len(b))
    for i in range(n):
        if a[i] != b[i]:
            return i
    return n


def check_file_equals(path, expected):
    """READ BACK `path`; None when its bytes equal `expected`, else a message with the first differing byte."""
    got = path.read_bytes()
    if got == expected:
        return None
    return f"{path} differs at byte {first_diff(expected, got)} (expected {len(expected)} B sha256 {sha(expected)[:16]}, read {len(got)} B sha256 {sha(got)[:16]})"


def check_file_appended(path, prev, body):
    """READ BACK an appended law file: its tail must be `body` and the bytes before it must be `prev`."""
    got = path.read_bytes()
    if len(got) < len(prev) + len(body):
        return f"{path} is {len(got)} B, shorter than previous {len(prev)} B + moved section {len(body)} B"
    tail = got[-len(body):]
    if tail != body:
        return f"{path} tail differs from the moved section at file byte {len(got) - len(body) + first_diff(body, tail)} (section {len(body)} B, file {len(got)} B)"
    if got[:len(prev)] != prev:
        return f"{path} pre-existing bytes changed at byte {first_diff(prev, got[:len(prev)])}"
    return None


def check_core_nonindex(expected):
    """READ BACK CONVENTIONS.md; None when its non-index bytes equal `expected`, else the first differing byte."""
    got = nonindex_bytes(CONV.read_bytes().decode("utf-8"))
    if got == expected:
        return None
    return f"{CONV} non-index bytes differ at byte {first_diff(expected, got)} (expected {len(expected)} B sha256 {sha(expected)[:16]}, read {len(got)} B sha256 {sha(got)[:16]})"


def check_core_index(expected):
    """READ BACK CONVENTIONS.md; None when its `## Law index` section equals the generated `expected` bytes."""
    got = index_bytes(CONV.read_bytes().decode("utf-8"))
    if got == expected:
        return None
    return f"{CONV} index section differs from the generated index at byte {first_diff(expected, got)} (generated {len(expected)} B, read {len(got)} B)"


def reindex():
    text = CONV.read_bytes().decode("utf-8")
    nl = "\r\n" if "\r\n" in text else "\n"
    sections = split_sections(text.splitlines(keepends=True))
    if "".join("".join(b) for _, b in sections) != text:
        print("SANITY FAIL: the section split is not lossless (re-joined sections != CONVENTIONS.md bytes)"); return 2
    core = [(h, b) for h, b in sections if not is_index(h)]
    before = "".join("".join(b) for _, b in core).encode("utf-8")
    by_file = {"CONVENTIONS.md": [(h, b) for h, b in core if h is not None]}
    by_file.update(law_sections())
    idx = build_index(by_file, nl, datetime.date.today().isoformat())
    new_text = core_text(core, idx, nl)
    CONV.write_bytes(new_text.encode("utf-8"))
    msg = check_core_nonindex(before) or check_core_index("".join(idx).encode("utf-8"))
    if msg:
        print(f"SANITY FAIL: {msg}")
        CONV.write_bytes(text.encode("utf-8")); print("  restored CONVENTIONS.md to its pre-run bytes")
        return 2
    print(f"reindexed: {len(by_file) - 1} law files; CONVENTIONS.md now {new_text.count(nl)} lines; index {len(idx)} lines; "
          f"non-index bytes unchanged ({len(before)} B, sha256 {sha(before)[:16]})")
    return 0


def main(argv):
    flags = set(argv[1:])
    if flags - {"--apply", "--dry-run", "--reindex"}:
        print(__doc__); return 64
    if "--reindex" in flags:
        return reindex()
    apply = "--apply" in flags
    text = CONV.read_bytes().decode("utf-8")
    nl = "\r\n" if "\r\n" in text else "\n"
    lines = text.splitlines(keepends=True)
    sections = split_sections(lines)

    keep, move, matched, nn = [], [], set(), next_nn()
    for head, body in sections:
        if head is None:
            keep.append((head, body)); continue
        kp = next((k for k in KEEP_PREFIXES if head.startswith(k)), None)
        if kp:
            matched.add(kp); keep.append((head, body)); continue
        m = TAG_RE.search(head)
        if m:
            dest = f"{m.group(1)}.md"
        else:
            dest = f"{nn:03d}-{slug(head)}.md"; nn += 1
        move.append((dest, head, body))

    unmatched = [k for k in KEEP_PREFIXES if k not in matched and k != "Law index"]
    print(f"CONVENTIONS.md: {len(lines)} lines, {len(sections)} sections -> keep {len(keep)} / move {len(move)}")
    if unmatched:
        print("  note: keep-prefixes matching no heading (fine after a split):", unmatched)
    dests = {}
    for dest, head, body in move:
        dests.setdefault(dest, []).append((head, body))
    for dest, items in dests.items():
        state = "append" if (LAW_DIR / dest).exists() else "new"
        print(f"  {dest:40s} {sum(len(b) for _, b in items):6d} lines  {state:6s} <- " + " | ".join(h[:50] for h, _ in items))
    print(f"  kept core: {sum(len(b) for _, b in keep)} lines")
    if not move:
        print("nothing to move")
    if not apply:
        print("\n(dry run; pass --apply to write, --reindex to rebuild the index only)"); return 0

    # ---- in-memory checks before any write ----------------------------------------------------------
    if "".join("".join(b) for _, b in sections) != text:
        print("SANITY FAIL: the section split is not lossless (re-joined sections != CONVENTIONS.md bytes)"); return 2
    if len(keep) + len(move) != len(sections):
        print(f"SANITY FAIL: partition {len(keep)} keep + {len(move)} move != {len(sections)} sections"); return 2
    clash = [LAW_DIR / d for d in dests if NN_FILE_RE.match(d) and (LAW_DIR / d).exists()]
    if clash:
        for p in clash:
            print(f"SANITY FAIL: would overwrite {p}")
        return 2

    core = [(h, b) for h, b in keep if not is_index(h)]
    expected_core = "".join("".join(b) for _, b in core).encode("utf-8")   # original minus moved minus old index
    today = datetime.date.today().isoformat()
    hdr = (f"<!-- MOVED from .claude/pipeline/CONVENTIONS.md on {today} by Tools/split_conventions.py. "
           f"Sections are byte-identical to the original; this comment is the only addition. "
           f"Cite clauses by tag (e.g. `VER-§3 cl. 6`); CONVENTIONS.md '## Law index' maps tags to files. -->{nl}")

    # ---- write the law files, then READ THEM BACK -----------------------------------------------------
    LAW_DIR.mkdir(exist_ok=True)
    written = []   # (path, prev_bytes_or_None)
    for dest, items in dests.items():
        target = LAW_DIR / dest
        body = "".join("".join(bl) for _, bl in items).encode("utf-8")
        prev = target.read_bytes() if target.exists() else None
        target.write_bytes((prev + body) if prev is not None else (hdr.encode("utf-8") + body))
        written.append((target, prev))
        msg = check_file_appended(target, prev, body) if prev is not None else check_file_equals(target, hdr.encode("utf-8") + body)
        if msg:
            print(f"SANITY FAIL: {msg}"); rollback(written); return 2
        print(f"  ok: {dest}  {'appended' if prev is not None else 'new'}  {len(body)} B read back == moved  sha256 {sha(body)[:16]}")

    # ---- write the core LAST, then READ IT BACK ---------------------------------------------------------
    by_file = {"CONVENTIONS.md": [(h, b) for h, b in core if h is not None]}
    by_file.update(law_sections())
    idx = build_index(by_file, nl, today)
    new_text = core_text(core, idx, nl)
    CONV.write_bytes(new_text.encode("utf-8"))
    msg = check_core_nonindex(expected_core) or check_core_index("".join(idx).encode("utf-8"))
    if msg:
        print(f"SANITY FAIL: {msg}")
        CONV.write_bytes(text.encode("utf-8")); print("  restored CONVENTIONS.md to its pre-run bytes")
        rollback(written); return 2
    print(f"\napplied: {len(dests)} destination file(s) in {LAW_DIR}; CONVENTIONS.md now {new_text.count(nl)} lines "
          f"(non-index bytes {len(expected_core)} B == original minus moved minus old index); "
          f"namespaces indexed: {sum(1 for l in idx if l.startswith('| `'))}")
    return 0


def rollback(written):
    """Undo this run's law-file writes: remove new files, restore appended ones to their previous bytes."""
    for path, prev in written:
        try:
            if prev is None:
                path.unlink(); print(f"  removed {path}")
            else:
                path.write_bytes(prev); print(f"  restored {path}")
        except OSError as e:
            print(f"  could not roll back {path}: {e}")


if __name__ == "__main__":
    sys.exit(main(sys.argv))
