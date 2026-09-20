# INCIDENTS — every real failure, and the check that now catches it

Newest first. One entry per incident that changed how this repo works.

The rule: **when something breaks, the fix is not done until a check exists that would
have caught it, and the incident is written here next to that check.** A check with no
written rationale looks arbitrary to the next hurried contributor (or agent), and
arbitrary checks get deleted. The rationale is the load-bearing part.

    ## YYYY-MM-DD — <one-line failure>
    What broke:        <the user-visible symptom>
    Check added:       <file> + <gate stage that now catches it>
    Why it must stay:  <why deleting this check re-enables the bug>

---

## 2026-09-20 — the Pages deploy was red for seven months and only CI could see it

What broke:        `Documentation Validation and Deployment` failed on every push from
                   2026-02-11 onward, so GitHub Pages — the only production target this repo
                   has — kept serving the 2026-02-10 build. The failing step is `Build Computo`:
                   the runner's gcc 13.3 raises `-Werror=maybe-uninitialized` at
                   /usr/include/c++/13/variant:224 while compiling src/operators/control_flow.cpp.
                   The diagnostic names `jsom::JsonDocument::storage_`, but the code it objects
                   to is libstdc++'s variant machinery, inlined from json_document.hpp:81
                   through `TailCall` and `std::make_unique`. Nothing local reproduced it: the
                   workflow passes no build type, and JSOM's subproject (FetchContent) defaults
                   `CMAKE_BUILD_TYPE` to Release, so that build is -O3 -DNDEBUG, while every stage
                   of tools/ci.sh builds CI_BUILD_TYPE=Debug. Measured 2026-09-20: the same
                   sources trip at -O2/-O3 and are clean at -O0/-O1 under both gcc 13.3 and gcc
                   14.2, and `./build.sh` (Release) had been broken on this box the whole time
                   with no stage in a position to notice.
Check added:       CMakeLists.txt: `-Wno-maybe-uninitialized` for Computo's own targets,
                   bounded to GNU < 15 and non-Debug configs, declared next to the `-Werror` it
                   bounds. No stage of tools/ci.sh configures an optimized build, so no *stage*
                   would have caught this — that missing check is follow-up card `t_45a28893`,
                   not something this entry can claim.
Why it must stay:  The scope is what makes an optimized build possible at all on gcc <= 14:
                   the runner is gcc 13 and this box is gcc 14, so removing it re-reds the Pages
                   deploy, `./build.sh`, and any consumer building this repo at -O2/-O3, over a
                   compiler bug — the gcc#101905 false-positive family, fixed in gcc 15+, which
                   is why the bound is `< 15` and disappears by itself on a newer compiler —
                   rather than over a defect in this repo's code. What it costs, stated rather
                   than implied: gcc reports the simple uninitialized-read diagnostics under
                   `-Wmaybe-uninitialized` too (measured: a definite uninitialized read at -O0
                   is reported as `[-Wmaybe-uninitialized]` and silent with the flag), so
                   optimized configurations lose that family. The Debug configuration every
                   gate stage builds keeps the whole set — the bound is deliberately not
                   "all configs". Same decision and same compiler bound as JSOM's own
                   CMakeLists.txt, which scopes the flag to its jsom_tests target.

---

## 2026-09-20 — the gate printed GATE PASSED with 9 of its 10 stages never run

What broke:        A `git push` re-ran the full tier and printed `all 10 stage(s) passed in 0s`
                   while only `tree` had really run. `format` died with
                   `tools/ci.sh: sources: unbound variable` (stage_format, on a touched set with no C++ file) — `set -u` plus
                   `local -a sources` declared and never filled — and bash unwound out of the
                   stage function *and* out of the dispatch loop, so the run fell through to the
                   end, where the verdict is printed unconditionally. The trigger is the common
                   case, not an exotic one: any commit whose touched set holds no C++ file (a
                   docs, script or record change) takes that branch.
Check added:       `tools/ci.sh`: the arrays are declared `=()`, a stage that returns non-zero
                   without reporting a verdict fails the run, and the verdict is derived from
                   what RAN (`FAILED: N of M stage(s) did not run`). Carried into this repo as
                   `tools/kit-probes/gate-stage-guards.sh`, the kit's probe for exactly this fix
                   (card `t_0cc793fb`; the incident, the measurement and the rationale are in the
                   kit's INCIDENTS.md), and run by the `kitprobes` stage on every push.
Why it must stay:  Contract rule 1 is "a stage that did not run must never read as green". The
                   `BLOCK` lines cover the stages behind a *reported* failure; these guards cover
                   a stage that dies without reporting anything at all. Without them a green
                   verdict can sit over a gate that checked almost nothing — the failure the kit
                   exists to remove, and the one that hid a broken tidy baseline in four repos.

## 2026-09-20 — a wall-clock throughput threshold is a flake, not a test

What broke:        When the two-tier gate was first wired (2026-09-19) it could not run
                   `tests` at all without `GTEST_FILTER` exclusions: two cases in
                   tests/test_thread_safety.cpp assert `EXPECT_GT(ops_per_second, 10000.0)`
                   and this 4-core box measured 5723-7174 ops/s on an idle machine (the
                   case failed roughly two runs in three) and 6890 ops/s while a parallel
                   build ran. A gate that is red two runs in three is a gate that gets
                   `--no-verify`, and then the tests it was protecting never run.
Check added:       tests/test_thread_safety.cpp: the absolute floor is replaced by a
                   liveness ceiling on the whole 32-thread stress run (30 s, against
                   129 ms idle / 505 ms under ASan = ~59x headroom) plus a scale-out floor
                   (`> 50 ops/s` single-threaded, `> 0.1` ratio per extra thread) - all
                   three carrying their measured references in the comment at the
                   assertion. `tools/ci.sh` runs plain `ctest`: no GTEST_FILTER anywhere.
Why it must stay:  The assertion measures the machine, not the code. Any threshold tight
                   enough to be a real performance claim is loose enough to fail on a
                   loaded box, and the machine's load is not this repo's business. These
                   numbers can only detect a COLLAPSE (a deadlock, an accidental global
                   lock, an O(n^2) path); real throughput measurement lives in
                   tests/test_performance.cpp (the `benchmark` target, `ctest -L
                   performance`). Re-tightening the floor re-creates the flake, and the
                   next person deletes the whole case instead of fixing it.

---

## 2026-09-20 — the leak heuristic was measuring the sanitizer's quarantine

What broke:        The ASan stage could not run two cases of tests/test_memory_safety.cpp:
                   their fixture measures the process's RSS before and after a test and
                   fails on growth over 10 MB, and reported "Potential memory leak
                   detected: 179064 KB" (LargeArrayMapOperation) and 147152 KB
                   (LargeArrayFilterOperation). Both are clean in the normal build. The
                   memory was not leaked - ASan's allocator keeps freed memory in its
                   quarantine on purpose, because that is what makes use-after-free
                   detectable, so the measurement measured the tool.
Check added:       tests/test_memory_safety.cpp: the fixture detects an AddressSanitizer
                   build (`__SANITIZE_ADDRESS__` / `__has_feature(address_sanitizer)`) and
                   skips the RSS heuristic there, because the ASan stage in tools/ci.sh
                   runs with `ASAN_OPTIONS=detect_leaks=1` - LeakSanitizer, which is an
                   exact leak check and strictly stronger than an RSS heuristic. In every
                   other build the heuristic still runs.
Why it must stay:  A sanitizer-aware test is the difference between a stage that runs and a
                   stage that is excluded. The alternatives are both worse: excluding the
                   cases (they then never run anywhere under ASan) or shrinking ASan's
                   quarantine (trading real use-after-free detection for a weaker check).
                   If the skip is ever "simplified" away, the ASan stage goes red again for
                   a reason that is not a bug, and the exclusions come back.

---

## 2026-09-20 — the linter was analysing a fetched dependency, and a file nobody compiled

What broke:        `tidy` could not be wired into the gate. Its HeaderFilterRegex
                   `'^.*/(src|include)/.*\.(hpp|cpp)$'` also matched the FetchContent'd JSOM
                   headers (build/_deps/jsom-src/include/...), so 5178 of its 5601 findings
                   were in code this repo cannot change (115 distinct, re-reported once per
                   translation unit) - and the same pattern MISSED tests/*.hpp, so it was
                   simultaneously too wide and too narrow. Underneath that, one committed
                   source named no CMake target at all, so the stage's compile-database
                   coverage check refused to certify the analysis: "1 of 45 sources are
                   missing from build/compile_commands.json - the result would be a lie".
Check added:       .clang-tidy: `HeaderFilterRegex: 'Computo/(src|include|tests)/.*\.(hpp|cpp)$'`
                   (positive and repo-scoped; LLVM's regex has no negative lookahead, so
                   "anything but _deps" cannot be written) plus the value-only check set
                   JSOM and jsonTools use. CMakeLists.txt: tests/test_json_pointer.cpp is
                   now in the `test_computo` target, so it is compiled, run (7 cases), and
                   present in the compile database. `tidy` is in CI_DEFAULT_STAGES and in
                   .githooks/pre-push.
Why it must stay:  A linter that reports a dependency's code is a linter nobody can satisfy:
                   the findings are real, they are simply not this repo's to fix, and a
                   gate that can never go green is a gate everyone bypasses. The coverage
                   check is the other half: a compile database that exists is NOT one that
                   covers your sources (it can hold a dependency's translation units and
                   none of yours), so the stage asserts coverage BEFORE it reports findings
                   - which is exactly how the orphaned test file was found.
                   The orphaned file is the real lesson: it was committed, reviewed, and
                   never compiled or run, and nothing said so until a tool demanded a
                   per-source database entry. Zero findings is now required with no
                   baseline file; adding `.ci/tidy-baseline.txt` back means agreeing to keep
                   findings instead of fixing them.

---

## 2026-09-20 — format drift made every later one-line edit expensive

What broke:        31 of the repo's 54 committed source files pre-dated .clang-format
                   (clang-format 19.1.7): they do not conform. The gate's `format` stage is
                   branch-scoped, so editing any of them produced a 50-300 line reformat
                   mixed into the real change - in src/repl.cpp, changing one line cost 255
                   diff lines. That is why a version-string fix had been reverted twice: the
                   correct fix was a one-liner in a file nobody could afford to touch.
Check added:       One mechanical clang-format commit (a589d2b) over exactly the 31 files,
                   with `.git-blame-ignore-revs` so `git blame` can skip it. The format
                   stage is back to the kit's default behaviour, including the
                   level-checkout fallback (`CI_FORMAT_FALLBACK_HEAD` is no longer overridden
                   to 0).
Why it must stay:  Format-on-touch only works if the tree is already formatted - otherwise
                   every touch is a reformat, and the reformat is what makes the change
                   unreviewable. The proof that this commit was formatting-only is
                   token-level, not `git diff -w`: for all 31 files every token outside an
                   `#include` directive is byte-identical and in the same order (adjacent
                   string literals merged, because BreakStringLiterals splits long
                   literals; includes and using-declarations compared as multisets, because
                   SortIncludes and SortUsingDeclarations are the two reorderings
                   clang-format may make). Evidence: log/kanban-t_7c46709f/item4-format.log.

---

<!--
Copy this file into a repo root as INCIDENTS.md and keep this example as the shape
reference (or delete it once you have two real entries). Then, forever after: the commit
that fixes a failure also adds its entry here and the check that catches it.
-->
