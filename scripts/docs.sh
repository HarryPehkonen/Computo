#!/usr/bin/env bash
#
# docs — split out of the old tools/ci.sh (2026-10-06, card t_075c0a6f).
#
# Called from gate.toml as `[stage.docs] cmd = "scripts/docs.sh"`. The verdict
# vocabulary (ci_begin/ci_pass/ci_fail/ci_skip) is defined in scripts/gate-env.sh.
set -uo pipefail
. "$(dirname "$0")/gate-env.sh"

run_stage() {
    ci_begin "docs (examples, coverage, generated reference + indexes)"
    if ! command -v python3 >/dev/null 2>&1; then
        ci_fail docs "python3 is not installed — the documentation pipeline (66 examples, operator coverage, generated reference) cannot run; install python3 (Debian/Ubuntu: apt install python3 python3-yaml)"
    fi
    if ! python3 -c 'import yaml' >/dev/null 2>&1; then
        ci_fail docs "python3 has no PyYAML, which the examples and the coverage check need to read docs/operators.yaml (Debian/Ubuntu: apt install python3-yaml)"
    fi
    local binary="$CI_BUILD_DIR/computo"
    if [ ! -x "$binary" ]; then
        ci_fail docs "no executable at $binary — this stage grades the binary the build stage produced; run the build stage first, or point CI_BUILD_DIR at the build you mean"
    fi
    # The scripts default to ./build/computo. Say which binary to grade instead of letting a
    # CI_BUILD_DIR override make this stage report on a different engine than the one built.
    export COMPUTO_BINARY="$binary"

    local tmp
    tmp=$(mktemp -d "${TMPDIR:-/tmp}/ci-docs-XXXXXX")
    RUN_TMP_DIRS+=("$tmp")
    printf '    engine under test: %s\n' "$binary"

    # 1. every example, executed. A failing example is printed WITH its expression, its
    # expected result and what the engine actually answered - the failure is the evidence here.
    if ! python3 docs/test-examples.py > "$CI_LOG_DIR/docs-examples.log" 2>&1; then
        grep -E '^  ✗|^      (Expression|Inputs|Expected|Error):|^Results:' "$CI_LOG_DIR/docs-examples.log" \
            | head -40 | sed 's/^/      /'
        ci_fail docs "documented examples disagree with the engine (every failure is listed above)" "$CI_LOG_DIR/docs-examples.log"
    fi
    grep -E '^Results:' "$CI_LOG_DIR/docs-examples.log" | sed 's/^/    /'

    # 2. README.md's OWN result examples, same engine. They are hand-written prose in a
    #    1300-line file, not the YAML docs/test-examples.py reads, so they had no check at
    #    all: when the engine's array output changed on 2026-02-10, 41 of them went on
    #    stating results the CLI no longer printed and only a human reading the page could
    #    see it. docs/check-readme-examples.py grades the two in-line forms it can read
    #    unambiguously (arrow examples and "Result:" examples), prints every line it could
    #    NOT read by number, and fails if extraction ever covers fewer than its floor - so
    #    a prose rewrite cannot quietly reduce this check to nothing (INCIDENTS.md).
    if ! python3 docs/check-readme-examples.py > "$CI_LOG_DIR/docs-readme.log" 2>&1; then
        grep -E '^  ✗|^      |^README examples:' "$CI_LOG_DIR/docs-readme.log" \
            | head -40 | sed 's/^/      /'
        ci_fail docs "README.md's result examples disagree with the engine, or the extraction floor was missed (above)" "$CI_LOG_DIR/docs-readme.log"
    fi
    grep -E '^README examples:' "$CI_LOG_DIR/docs-readme.log" | sed 's/^/    /'

    # 3. coverage: every implemented operator documented, and no documented one missing.
    if ! python3 docs/validate-coverage.py > "$CI_LOG_DIR/docs-coverage.log" 2>&1; then
        tail -n 30 "$CI_LOG_DIR/docs-coverage.log" | sed 's/^/      /'
        ci_fail docs "operator coverage is incomplete (report above)" "$CI_LOG_DIR/docs-coverage.log"
    fi
    grep -E 'Implemented operators|Documented operators|COMPLETE' "$CI_LOG_DIR/docs-coverage.log" | sed 's/^/    /'

    # 4. the generated reference must be what operators.yaml generates, byte for byte: it is
    # tracked and published, so a stale one is a false statement on the site. Generated to a
    # temp file - a gate that rewrites a tracked file is a gate nobody can trust.
    if ! python3 docs/generate-reference.py docs/operators.yaml -o "$tmp/LANGUAGE_REFERENCE.md" \
        > "$CI_LOG_DIR/docs-generate.log" 2>&1; then
        ci_fail docs "docs/generate-reference.py failed" "$CI_LOG_DIR/docs-generate.log"
    fi
    if ! diff -q docs/LANGUAGE_REFERENCE.md "$tmp/LANGUAGE_REFERENCE.md" >/dev/null; then
        diff -u docs/LANGUAGE_REFERENCE.md "$tmp/LANGUAGE_REFERENCE.md" | head -40 | sed 's/^/      /'
        ci_fail docs "docs/LANGUAGE_REFERENCE.md is not what docs/operators.yaml generates — regenerate it (cmake --build $CI_BUILD_DIR --target docs-generate) and review the diff before committing"
    fi
    printf '    docs/LANGUAGE_REFERENCE.md is what operators.yaml generates\n'

    # 5. the two published indexes, same rule. docs/generate-indexes.py writes in place, so
    # the committed copies are put back afterwards: this gate never leaves a modified tree.
    cp docs/alpha/index.md "$tmp/alpha-index.md" || ci_fail docs "docs/alpha/index.md is missing"
    cp docs/task/index.md "$tmp/task-index.md" || ci_fail docs "docs/task/index.md is missing"
    if ! python3 docs/generate-indexes.py > "$CI_LOG_DIR/docs-indexes.log" 2>&1; then
        ci_fail docs "docs/generate-indexes.py failed" "$CI_LOG_DIR/docs-indexes.log"
    fi
    local stale=""
    diff -q docs/alpha/index.md "$tmp/alpha-index.md" >/dev/null || stale="$stale docs/alpha/index.md"
    diff -q docs/task/index.md "$tmp/task-index.md" >/dev/null || stale="$stale docs/task/index.md"
    cp "$tmp/alpha-index.md" docs/alpha/index.md
    cp "$tmp/task-index.md" docs/task/index.md
    if [ -n "$stale" ]; then
        ci_fail docs "generated index file(s) are stale:$stale — regenerate them with python3 docs/generate-indexes.py and commit the result"
    fi
    printf '    docs/alpha/index.md and docs/task/index.md are what operators.yaml generates\n'
    ci_pass docs
}

run_stage "$@"
