#!/usr/bin/env python3
"""
Grade README.md's result examples against the built engine.

WHY THIS EXISTS (INCIDENTS.md, 2026-09-20): README.md is the file the Pages site
publishes most prominently, and its result examples are hand-written prose - not the
YAML in docs/operators.yaml, which docs/test-examples.py already executes. So nothing
graded them, and when the engine's array output changed on 2026-02-10, 41 of them went
on stating results the CLI no longer prints (29 still showing the pre-2026-02-10
`{"array": [...]}` wrapper). This script is the sweep that found them, promoted from a
throwaway on card t_4a81d75f into a check the `docs` stage runs (card t_a609b673).

WHAT IT GRADES, AND WHAT IT DELIBERATELY DOES NOT
Two in-line forms are extracted, both of which carry an expression and a stated result
on one line:

    - `["keys", {"a": 1, "b": 2}]` → `["a", "b"]`            (an arrow example)
    - `- `["keys", {"a": 1, "b": 2}]`: Get object keys. Result: `["a", "b"]`.`
                                                             (a "Result:" example)

plus the fenced-block form where the stated Result: line follows a ```json block (the
expression is read back out of that block, and the input document README defines above
it is passed as the script's input).

Everything else is NOT graded, and the counts are printed on every run so the gap stays
visible: a line whose expression is not JSON, an illustrative right-hand side ("→ sorted
array", "→ Dynamic key"), a shell session inside a bash block, prose. Those lines are
listed by number at the end - a skip that is quiet is how this class of drift survived
seven months.

The floor is the guard against the silent failure mode of a prose parser: if the prose
changes shape (a table is reformatted, the arrow becomes "->", an example is reworded),
extraction finds FEWER examples and this check would happily pass while covering less.
MIN_GRADED_EXAMPLES makes that a loud failure instead. 96 examples were graded when this
check landed on 2026-09-20 (0 mismatched); the floor is 90, so removing a few examples is
not a script edit while a collapse of the extraction is a failed run.

The engine is taken from COMPUTO_BINARY (default ./build/computo), the same contract the
other docs/ scripts use, so a gate that built into another directory grades the binary it
just built. README is never modified: this is a read-only check.
"""

import json
import os
import re
import subprocess
import sys
import tempfile

BIN = os.environ.get("COMPUTO_BINARY", "./build/computo")
README = sys.argv[1] if len(sys.argv) > 1 else "README.md"

# 96 graded on 2026-09-20 with 0 mismatches. See the docstring for why there is a floor at
# all; lower it deliberately, not to make a red run green.
MIN_GRADED_EXAMPLES = 90

SPAN = re.compile(r"`([^`]*)`")
ARRAY_FLAG = " --array="


def spans(line):
    """(offset, text) for every backtick span on the line."""
    return [(m.start(), m.group(1)) for m in SPAN.finditer(line)]


def parse_script(text):
    """A backtick span that is a script: a JSON array, optionally followed by --array=<key>.

    Returns (expression, extra_args) or None. An example may state a result that depends
    on the array key (`--array="@data"`), so the flag travels with the expression instead
    of being prose next to it.
    """
    text = text.strip()
    try:
        value = json.loads(text)
        return (value, []) if isinstance(value, list) else None
    except Exception:
        pass
    if ARRAY_FLAG not in text:
        return None
    head, _, tail = text.rpartition(ARRAY_FLAG)
    try:
        value = json.loads(head.strip())
    except Exception:
        return None
    key = tail.strip().strip('"').strip("'")
    if not isinstance(value, list) or not key:
        return None
    return value, ["--array=" + key]


def parse_stated(text):
    """Parse a stated result: JSON, or a scalar the reference section writes as plain text."""
    text = text.strip().strip("`").strip()
    try:
        return True, json.loads(text)
    except Exception:
        return False, text


def run(expr, inputs=(), extra=()):
    paths = []
    try:
        for value in (expr, *inputs):
            f = tempfile.NamedTemporaryFile("w", suffix=".json", delete=False)
            json.dump(value, f)
            f.close()
            paths.append(f.name)
        r = subprocess.run([BIN, "--script", *paths, *extra],
                           capture_output=True, text=True, timeout=20)
        return r.returncode, r.stdout.strip(), r.stderr.strip()
    finally:
        for p in paths:
            os.unlink(p)


def last_fenced_json(lines, i):
    """Expression from the last ```json fence block ending before line i (0-based)."""
    end = None
    for j in range(i - 1, -1, -1):
        if lines[j].strip().startswith("```"):
            if end is None:
                end = j
            else:
                body = "\n".join(lines[j + 1:end])
                try:
                    value = json.loads(body)
                except Exception:
                    return None
                return json.dumps(value) if not isinstance(value, list) else value
    return None


def input_document(lines):
    """README's example input document: the first ```json block whose body starts with '{'."""
    for j, line in enumerate(lines):
        if line.strip() == "```json" and lines[j + 1].strip().startswith("{"):
            end = next((k for k in range(j + 1, len(lines)) if lines[k].strip() == "```"), None)
            if end is None:
                return None
            try:
                return json.loads("\n".join(lines[j + 1:end]))
            except Exception:
                return None
    return None


def extract(lines, i):
    """(expression, extra_args, stated, expr_text) for line i (0-based), or None."""
    line = lines[i]
    spans_ = spans(line)
    expr = expr_text = None
    extra = ()

    if "→" in line:
        arrow = line.index("→")
        for pos, text in spans_:
            if pos < arrow:
                parsed = parse_script(text)
                if parsed is not None:
                    expr, extra = parsed
                    expr_text = text
        right = [(pos, t) for pos, t in spans_ if pos > arrow]
        if expr is None or not right:
            return None
        return expr, extra, right[0][1], expr_text

    marker = re.search(r"Result:?", line)
    if marker is None:
        return None
    for pos, text in spans_:
        if pos < marker.start():
            parsed = parse_script(text)
            if parsed is not None:
                expr, extra = parsed
                expr_text = text
    if expr is None:
        # Bullet continuation ("  - Result: `...`") or a fenced block above this line.
        for j in range(i - 1, -1, -1):
            if not lines[j].strip():
                continue
            for pos, text in spans(lines[j]):
                parsed = parse_script(text)
                if parsed is not None:
                    expr, extra = parsed
                    expr_text = text
            if expr is not None:
                break
            if lines[j].strip().startswith("```"):
                candidate = last_fenced_json(lines, i)
                if candidate is not None:
                    expr_text = json.dumps(candidate)
                    expr = candidate
                break
    if expr is None:
        return None
    right = [(pos, t) for pos, t in spans_ if pos > marker.end()]
    if not right:
        return None
    return expr, extra, right[0][1], expr_text


def main():
    try:
        with open(README, encoding="utf-8") as f:
            lines = f.read().split("\n")
    except OSError as e:
        print(f"Error: cannot read {README}: {e}", file=sys.stderr)
        return 1

    print(f"engine under test: {BIN}")
    print(f"readme under test: {README}")
    if not os.path.isfile(BIN) or not os.access(BIN, os.X_OK):
        print(f"Error: no executable engine at {BIN} — this check grades the binary the "
              "build stage produced; run the build stage first, or point COMPUTO_BINARY "
              "at the binary to grade", file=sys.stderr)
        return 1

    # A line that looks like it carries a result example is a *candidate*; the ones this
    # script cannot extract are reported by number at the end instead of disappearing.
    candidates = [i for i, line in enumerate(lines) if "→" in line or re.search(r"Result:?", line)]
    graded = []
    mismatches = []
    errors = []
    skipped = []
    input_doc = None

    for i in candidates:
        found = extract(lines, i)
        if found is None:
            skipped.append((i + 1, "no expression/stated result this script can read"))
            continue
        expr, extra, stated, expr_text = found
        rc, out, err = run(expr, (), extra)
        if rc != 0 and "$input" in (expr_text or ""):
            # README's "Working with Nested Data Structures" examples run against the
            # input document those sections define above them.
            if input_doc is None:
                input_doc = input_document(lines)
            if input_doc is not None:
                rc, out, err = run(expr, (input_doc,), extra)
        if rc != 0:
            errors.append((i + 1, expr_text, stated, (err or out).split("\n")[0][:120]))
            continue
        stated_is_json, stated_value = parse_stated(stated)
        try:
            actual = json.loads(out)
        except Exception:
            actual = out
        if stated_is_json:
            if json.dumps(actual, sort_keys=True) == json.dumps(stated_value, sort_keys=True):
                graded.append(i + 1)
            else:
                mismatches.append((i + 1, expr_text, stated, out.replace("\n", "")))
        elif out == stated_value.strip():
            # The engine's output stated as plain text (no quotes) - still a real claim.
            graded.append(i + 1)
        else:
            skipped.append((i + 1, f"stated result {stated.strip()!r} is not JSON"))

    for line_no, expr_text, stated, actual in mismatches:
        print(f"  ✗ L{line_no}: {expr_text}")
        print(f"      README says: {stated}")
        print(f"      engine says: {actual}")
    for line_no, expr_text, stated, err in errors:
        print(f"  ✗ L{line_no}: {expr_text}")
        print(f"      README states a result, the engine will not run it: {err}")
    for line_no, why in skipped:
        print(f"  - not graded L{line_no}: {why}")

    print("=" * 60)
    print(f"README examples: {len(graded)} graded, {len(mismatches)} mismatched, "
          f"{len(errors)} failed to run, {len(skipped)} not graded")

    failed = False
    if mismatches or errors:
        print(f"  {len(mismatches) + len(errors)} README example(s) disagree with the engine")
        failed = True
    if len(graded) < MIN_GRADED_EXAMPLES:
        print(f"  only {len(graded)} examples were extracted, floor is {MIN_GRADED_EXAMPLES}: "
              "README's example prose changed shape, so this check now covers less than it "
              "did (see this script's docstring). Fix the extractor, or lower the floor "
              "deliberately if the README really lost examples.")
        failed = True
    if not failed:
        print(f"  every extracted example matches the engine"
              f" ({len(skipped)} lines were not extractable, listed above)")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
