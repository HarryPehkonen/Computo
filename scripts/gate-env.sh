#!/usr/bin/env bash
#
# Computo's gate: the configuration every stage reads, and the helpers they share.
#
# The POLICY is gate.toml (which stages exist, which tier runs which of them, how a failure
# is recognised). The ENGINE is kit-ci, one binary installed once per machine
# (cmake --install build --prefix ~/.local). Everything a stage needs BEYOND that policy -
# a build dir, a job count, a fuzz budget - lives HERE, so the policy file stays a list of
# stages.
#
# SOURCED, never executed: scripts/gate.sh does not need it; every stage script does
# (`. "$(dirname "$0")/gate-env.sh"`). The knobs are the .ci.env knobs the old tools/ci.sh
# carried, each with the default it had there, and .ci.env is still sourced last, so a
# machine with no .ci.env behaves exactly as it did before the conversion
# (2026-10-06, card t_075c0a6f).

set -uo pipefail

REPO_ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
cd "$REPO_ROOT" || exit 1

# No colour from the tools: this script greps their output (warning:, error:) and ANSI
# escapes defeat the greps. The escapes this script prints itself are for the human.
export NO_COLOR=1

# `unset GIT_INDEX_FILE` below is the KIT'S line, not this repo's own invention: the kit adopted
# it at f9c3300 (card t_0a9a0018) and holds every copy to it with probes/git-index-file.sh —
# which this copy carries as tools/kit-probes/git-index-file.sh and runs in its own `kitprobes`
# stage. The comment block below is the one part of the fix that stays repo-specific; it is this
# repo's own measurement of the defect and of the safe place to unset.
#
# git exports GIT_INDEX_FILE to a hook when the commit is made with a PATHSPEC
# (`git commit -- <path>`, the form this repo's own notes recommend for a shared tree) — it
# points at git's TEMPORARY index for the commit in progress, not at this repo's index. Every
# process the gate runs inherits it, and cmake's FetchContent update step in the `build` stage
# runs `git --git-dir=.git status --porcelain` inside build/_deps/jsom-src: handed COMPUTO's
# index, it reads Computo's entries against the JSOM clone's object store and dies on the
# first blob that clone does not have —
#     fatal: unable to read 691e2bdafaf312970644391de042d38c2c5972d8
#     CMake Error at .../jsom-populate-gitupdate.cmake:186 (message): Failed to get the status
# — so a pathspec commit failed its OWN pre-commit gate at `build`, naming a dependency
# update, on a tree that builds fine (measured 2026-09-20, card t_9541aa62; the workaround was
# to stage first, which is not a fix).
#
# The variable is git's bookkeeping for one commit, not this repository: it says nothing about
# the tree the gate exists to certify, and its value is meaningless in any OTHER repository
# that the gate happens to run a git command in. Unset it once, here, rather than `env -u` on
# each of the five cmake configure lines — every process the gate starts inherits it, so a
# per-invocation fix would cover the instance (`build`) and leave the class (the four other
# configures, the `pristine` checkout, and anything a repo's own `kitprobes` scripts run).
#
# Measured before choosing this place, in a throwaway clone, on a real `git commit -- <path>`
# with the gate's own stages run BOTH ways: nothing the stages read differs. The hook's own
# GIT_INDEX_FILE showed the mechanism (.git/next-index-XXXXXX.lock), and with it and without
# it `git status --porcelain --untracked-files=no` names the same files, `git ls-files
# --others` the same one untracked file, `git diff --name-only HEAD` the same two files, and
# the source set `format` checks is the same 103 entries — with the `tree` and `format` stage
# OUTPUTS byte-identical (tree FAIL on that same untracked file, format pass on the same two
# files). The only differences are views no stage reads: `git diff --cached HEAD` (empty
# against the real index) and the staged-vs-unstaged letter, and there the real index is the
# truthful one, because the worktree has not been committed yet.
unset GIT_INDEX_FILE

# ---------------------------------------------------------------- defaults + config
CI_JOBS=${CI_JOBS:-$(nproc 2>/dev/null || echo 4)}
CI_BUILD_DIR=${CI_BUILD_DIR:-build}
CI_ASAN_BUILD_DIR=${CI_ASAN_BUILD_DIR:-build-asan}
CI_TSAN_BUILD_DIR=${CI_TSAN_BUILD_DIR:-build-tsan}
CI_LOG_DIR=${CI_LOG_DIR:-.ci-logs}
CI_STRICT_TOOLS=${CI_STRICT_TOOLS:-0}           # 1 = a missing tool fails instead of SKIPping
CI_KEEP_TMP=${CI_KEEP_TMP:-0}                   # 1 = keep the pristine temp dir for inspection
# The two hook tiers, ONE definition each. Both hooks NAME a tier instead of repeating a list, so a
# stage added below cannot be run by a hand run and skipped by a push (or the reverse) — the failure
# this repo's own incident log records elsewhere in the fleet, where a push ran eleven stages of
# twelve and still printed GATE PASSED. The lines below are what probes/hook-tiers-agree.sh compares
# the two variables against, and against the tier the hooks name.
#
#
# `format` is in the fast tier deliberately: it is the one check that says "the file you are about to
# commit is not the file clang-format would write", it costs well under a second on a warm tree, and
# its absence from a fast tier is how an unformatted commit reached FSMTable's main branch on
# 2026-10-06. `full` IS the default list, so a hand run and a push run the same stages and only
# --require-clean differs.
CI_FAST_STAGES=${CI_FAST_STAGES:-"format build tests"}
CI_FULL_STAGES=${CI_FULL_STAGES:-"tree format kitprobes build tests docs release version asan tsan tidy pristine"}
CI_DEFAULT_STAGES=${CI_DEFAULT_STAGES:-$CI_FULL_STAGES}
CI_TIDY_BASELINE=${CI_TIDY_BASELINE:-.ci/tidy-baseline.txt}
CI_BUILD_TYPE=${CI_BUILD_TYPE:-Debug}
# The source set the format and tidy stages own. Extend for your layout.
CI_SOURCE_GLOBS=${CI_SOURCE_GLOBS:-"'*.cpp' '*.cc' '*.cxx' '*.hpp' '*.hh' '*.h'"}
# Where the SECOND copy of the version number lives. Two forms are both fine:
#   the CMake-generated header  (configure_file(version.hpp.in ...) -> ${CMAKE_BINARY_DIR}/generated/version.hpp)
#   a tracked header            (include/<project>/version.hpp) — point this knob at it
CI_VERSION_HEADER=${CI_VERSION_HEADER:-"$CI_BUILD_DIR/generated/version.hpp"}
# The test runner. ctest is the portable default; override with a single test binary if
# your project does not register tests with add_test().
CI_TEST_CMD=${CI_TEST_CMD:-"ctest --test-dir \$CI_BUILD_DIR --output-on-failure -j \$CI_JOBS"}
CI_FORMAT_FALLBACK_HEAD=${CI_FORMAT_FALLBACK_HEAD:-1}   # 0 = do not re-check the last commit when level with origin/main

# ---- Computo's own defaults (rationale in the adaptation notes at the top) ----
# NOTHING is filtered out of the test run. Until 2026-09-20 this file passed
# `GTEST_FILTER=-ThreadSafetyTest.PerformanceUnderThreadLoad:...:MemorySafetyTest.*` to
# ctest here, for two wall-clock throughput assertions and two RSS-based leak heuristics
# that could not pass on this machine. Both classes are now fixed in the tests themselves
# (a liveness ceiling and a sanitizer-aware measurement - see INCIDENTS.md), so the filter
# is gone and CI_ASAN_TEST_CMD is unset: every sanitizer stage runs the same command in its
# own build dir, which is the kit's default behaviour.
CI_VERSION_BINARIES=${CI_VERSION_BINARIES:-'$CI_BUILD_DIR/computo'}
# This assignment is direct (not ${VAR:-...}) on purpose: the kit's line above already
# set the variable, so the :- form would silently keep the kit's longer list.
# `tools/ci.sh --list` prints the effective list, which is the only place it is visible.
CI_DEFAULT_STAGES="tree format kitprobes build tests docs release version asan tsan tidy pristine"

# The optimized configuration (the `release` stage - not in the kit, see the adaptation
# notes at the top). Its own build dir: `build-release`, which .gitignore's `build-*/`
# already covers, and which the tree stage audits like the other three.
CI_RELEASE_BUILD_DIR=${CI_RELEASE_BUILD_DIR:-build-release}
CI_RELEASE_BUILD_TYPE=${CI_RELEASE_BUILD_TYPE:-Release}

if [ -f .ci.env ]; then
    # shellcheck disable=SC1091
    . ./.ci.env
fi


# The old tools/ci.sh took these as FLAGS (--require-clean, --changed, --extra-checks,
# --write-tidy-baseline, --write-coverage-baseline). kit-ci's vocabulary has no per-run flags
# for a stage: a caller sets the knob in the ENVIRONMENT it launches the gate with, and the
# default below is what the script had. .githooks/pre-push does exactly that with
# CI_REQUIRE_CLEAN=1. (Until this line the assignment overwrote whatever the caller exported,
# so the push hook's flag-as-env-var had no effect at all - measured 2026-10-06.)

REQUIRE_CLEAN=${CI_REQUIRE_CLEAN:-0}
ALLOW_UNTRACKED=0
WRITE_TIDY_BASELINE=0
STAGES_REQUESTED=()

# ---------------------------------------------------------------- plumbing
RESULT_LINES=()
FAILED_STAGE=""
RAN_STAGES=()
RUN_TMP_DIRS=()





require_tool() {  # require_tool <tool> <stage>; returns 1 when the caller should skip
    local tool="$1" stage="$2"
    if command -v "$tool" >/dev/null 2>&1; then return 0; fi
    if [ "$CI_STRICT_TOOLS" = "1" ]; then
        ci_fail "$stage" "$tool not installed (CI_STRICT_TOOLS=1) — nothing was checked"
    fi
    ci_skip "$stage" "$tool not installed — this gate was NOT exercised on this machine"
    return 1
}

# The file set the format gate owns: same list the CMake `format` target should use.
# --others --exclude-standard includes new files that are not committed yet: while
# working, a new source file is not in `git ls-files`, and a gate that cannot see it
# lets it through unformatted until after it is committed (that incident is in
# INCIDENTS.md).
ci_sources() {
    # shellcheck disable=SC2086
    eval "git ls-files --cached --others --exclude-standard -- $CI_SOURCE_GLOBS" | sort -u
}

# clang-tidy needs a compile database entry per translation unit, so headers are not
# passed here: diagnostics inside headers still surface through the sources that include
# them (that is what .clang-tidy's HeaderFilterRegex is for).
ci_tidy_sources() {
    # shellcheck disable=SC2086
    eval "git ls-files --cached --others --exclude-standard -- $CI_SOURCE_GLOBS" \
        | grep -E '\.(cpp|cc|cxx)$' | sort -u
}

# The comparable form of a clang-tidy finding — applied to BOTH sides of the baseline
# comparison, so the file a repo captures and the log this gate just wrote are the same
# shape:
#
#   <repo>/src/foo.cpp:42:7: warning: ...   ->   src/foo.cpp: warning: ...
#
#   * the repo root is stripped: clang-tidy reports the path it was handed by the compile
#     database, which CMake writes as an absolute path, so a baseline captured in one clone
#     names no finding in a checkout at another path (the nightly clean checkout, a
#     colleague's machine) and every inherited finding reads as new;
#   * :line:column is stripped, so the same finding after an unrelated edit above it is
#     still the same finding. This is line-blind on purpose, and the flip side is worth
#     knowing: a SECOND identical finding in a file that already has one collapses into the
#     first. Fix the baselined finding instead of growing the baseline.
#
# `tools/ci.sh --write-tidy-baseline` captures the baseline through this same function, so
# the documented way to accept findings cannot drift from the way they are compared.
tidy_key() {
    awk -v root="$REPO_ROOT/" '
        { i = index($0, root); if (i) $0 = substr($0, i + length(root)); print }' \
        | sed 's/:[0-9]*:[0-9]*:/:/'
}

run_tests() {  # run_tests <build-dir> <logfile>
    # shellcheck disable=SC2086
    eval "$CI_TEST_CMD" > "$2" 2>&1
}

mkdir -p "$CI_LOG_DIR"


# ---------------------------------------------------------------- stage helpers
cleanup() {
    local dir
    if [ "$CI_KEEP_TMP" != "1" ]; then
        for dir in "${RUN_TMP_DIRS[@]:-}"; do
            [ -n "$dir" ] && [ -d "$dir" ] && rm -rf "$dir"
        done
    fi
}

trap cleanup EXIT

# ---------------------------------------------------------------- verdict shims
# kit-ci calls a stage and reads its EXIT STATUS: 0 is a pass, non-zero is a failure, and
# the engine names the stage and prints the first lines of a failed stage's output itself
# (SPEC.md §4). The stage bodies in this directory were split verbatim out of the old
# tools/ci.sh and still speak that script's vocabulary, so it is defined here, once:
#
#   ci_pass <stage>                 END the stage, exit 0
#   ci_fail <stage> <reason> [log]  print why, show the log's tail, exit 1
#   ci_skip <stage> <reason>        say so; the caller then exits 0 (a SKIP is not a failure,
#                                   which is also what kit-ci's `when = "tool:<name>"` means)
#   ci_begin <title>                the old banner line
ci_begin() { printf '\n==> %s\n' "$1"; }
ci_pass() { exit 0; }
ci_skip() { printf '    SKIP: %s\n' "$2"; }
ci_fail() {
    printf 'FAILED: %s — %s\n' "$1" "$2" >&2
    [ -n "${3:-}" ] && show_log "$3"
    exit 1
}
show_log() {  # show_log <file> — the tail, for out-of-order output like a build log
    local file="${1:-}"
    if [ -n "$file" ] && [ -f "$file" ]; then
        printf -- '--- %s (last 25 lines) ---\n' "$file" >&2
        tail -n 25 "$file" | sed 's/^/      /' >&2
    fi
    return 0
}
