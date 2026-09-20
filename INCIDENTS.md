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

## 2026-09-20 — the documentation pipeline had no local stage, and the published README's result examples had drifted with it

What broke:        The three tools that grade this repo's documentation ran in exactly one place:
                   .github/workflows/docs.yml. No stage of tools/ci.sh touched them, so the
                   2026-02-10 output change (eeb2014c) could disagree with 14 examples - the
                   entry below documents those - and with the file the Pages site publishes, and
                   nothing on this machine could see it. The pipeline was also unrunnable here by
                   another route: `cmake --build build --target docs-coverage` died with
                   `Error: [Errno 2] No such file or directory: 'python'`, measured 2026-09-20 on
                   this laptop, because docs/validate-coverage.py called a hardcoded interpreter
                   name and only `python3` exists here. README.md had drifted the same way and was
                   covered by no check at all: measured 2026-09-20 against the built CLI, 41 of its
                   result examples disagreed with the engine - 29 still showed the pre-2026-02-10
                   `{"array": [...]}` output wrapper, 8 had stale number formatting (`6.0` where
                   the engine prints `6`), and 4 were claims inside fenced blocks, including the
                   whole `--array=<key>` section, whose stated default output was wrong and whose
                   custom-key example showed a shape the CLI never produces.
Check added:       tools/ci.sh: a `docs` stage - this repo's own, no kit counterpart - in
                   CI_DEFAULT_STAGES and in .githooks/pre-push. It grades the binary the build
                   stage produced ($CI_BUILD_DIR/computo, handed to the scripts as COMPUTO_BINARY),
                   runs all 66 examples and the coverage check, and requires
                   docs/LANGUAGE_REFERENCE.md and both generated indexes to be byte-identical to
                   what docs/operators.yaml generates (generated into a temp dir, so the stage
                   never rewrites a tracked file). python3 + PyYAML are gate prerequisites now and
                   a missing one FAILS the run rather than SKIPping. docs/validate-coverage.py uses
                   sys.executable, and both scripts take the engine path from COMPUTO_BINARY.
                   README.md's 41 wrong results are corrected against the built CLI - 96/96 of its
                   result-side examples now match it; the sweep that found them is on card
                   t_4a81d75f, not in the repo.
Why it must stay:  Two reasons, one per half. The YAML half: those examples ARE the published
                   reference - docs/generate-reference.py builds docs/LANGUAGE_REFERENCE.md from
                   them and the Pages job publishes it - so an example that disagrees with the
                   engine is either a false statement on a public page or a red deploy, and the
                   only thing that ever noticed was GitHub. Local-only was the whole defect: the
                   gate is what runs on every push, and a stage that SKIPs when python3 is absent
                   would restore exactly the same silence with a green light beside it, which is
                   why kit rule 1 is read here as "fail loudly" rather than "skip quietly". The
                   README half is deliberately not claimed: that sweep is a script, not a stage,
                   because its results are prose in a 1200-line file and a parser for them would
                   fail on future prose. What this entry records instead is that they are
                   uncovered - and the two further README defects the same audit turned up (a stray
                   closing fence at line 1210, and the `**Returns**: {"array": [...]}` type lines
                   that describe the engine's internal wrapper while the CLI prints a bare array at
                   the top level) are on their own card rather than silently fixed here.

---

## 2026-09-20 — no optimized build existed on gcc <= 14, and the only place one ran was CI

What broke:        The Pages workflow's `Build Computo` step failed on every push from
                   2026-09-08/09 onward (the deploy was already red before that, for the
                   separate reason in the next entry — the two together are the seven months
                   the site served the 2026-02-10 build). The runner's gcc 13.3 raises
                   `-Werror=maybe-uninitialized` at /usr/include/c++/13/variant:224 while
                   compiling src/operators/control_flow.cpp. The diagnostic names
                   `jsom::JsonDocument::storage_`, but the code it objects to is libstdc++'s
                   variant machinery, inlined from json_document.hpp:81 through `TailCall` and
                   `std::make_unique`. Nothing local reproduced it: the workflow passes no build
                   type, and JSOM's subproject (FetchContent) defaults `CMAKE_BUILD_TYPE` to
                   Release, so that build is -O3 -DNDEBUG, while every stage of tools/ci.sh
                   builds CI_BUILD_TYPE=Debug. Measured 2026-09-20: the same sources trip at
                   -O2/-O3 and are clean at -O0/-O1 under both gcc 13.3 and gcc 14.2, and
                   `./build.sh` (Release) had been broken on this box the whole time with no
                   stage in a position to notice.
Check added:       Two halves, one per copy of the defect.
                   (1) CMakeLists.txt: `-Wno-maybe-uninitialized` for Computo's own targets,
                   bounded to GNU < 15 and non-Debug configs, declared next to the `-Werror` it
                   bounds.
                   (2) tools/ci.sh: the `release` stage (card `t_45a28893`) — a second build
                   dir configured at `CI_RELEASE_BUILD_TYPE` (Release), built, held to the same
                   "no `warning:` anywhere" rule the `build` stage applies to its own log, and
                   run through the same test command. Until it existed, NO stage of this file
                   configured an optimized build at all, which is the whole reason the deploy
                   could be red for weeks behind a green local gate. It is in
                   `CI_DEFAULT_STAGES` and in `.githooks/pre-push`, so every push builds and
                   tests both configurations. Measured cost on this box (4 cores, load ~2.5):
                   cold configure 2.9 s + build 146 s + ctest 0.3 s; ~4.6 s on a one-source
                   push and 0.3 s with nothing changed, because the build dir is reused, so it
                   is affordable in the full tier and deliberately NOT in the pre-commit tier.
                   Proven to have teeth in a throwaway clone, not in this checkout: with the
                   bound removed — the state the repo was actually in before 83a9040 —
                   `tools/ci.sh release` fails with the gcc 14.2 `-Werror=maybe-uninitialized`
                   error while `tools/ci.sh build` still passes. That pair is the blind spot
                   reproduced: the Debug-only gate cannot see it, the release stage can.
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
                   The `release` stage must stay for the other half of the same reason: a check
                   that only ever runs on GitHub is a check nobody here can run, and the
                   configuration CI builds is the one that was broken while every local stage
                   said green. Deleting the stage puts this repo back in exactly that state —
                   `./build.sh` and any -O2/-O3 consumer red, no local signal — and the cost of
                   keeping it (146 s cold, ~5 s per push on a warm dir) is the smallest number
                   in this file.

## 2026-09-20 — the documented examples had been failing since 2026-02-10, and nothing local ran them

What broke:        Behind the build failure above sat a second and older one: `docs/test-examples.py`
                   reported "52 passed, 14 failed, 66 total" on every push since 2026-02-11, so
                   the workflow's `Run Documentation Validation` step exited 1 and `Deploy to
                   GitHub Pages` was skipped. That, not the compiler, is why the site served the
                   2026-02-10 build for seven months. Cause: commit eeb2014c (2026-02-10,
                   "unwrap array wrapper in CLI output and detect .computo file extension") stopped
                   printing `{"array": [...]}` for array results and updated
                   tests/test_cli_array_key.cpp and tests/test_cli_integration.cpp — but not the 14
                   examples in docs/operators.yaml that asserted the wrapper. Every failure has
                   the same shape: expected `{"array": [3, 4]}`, got `[3, 4]`.
Check added:       docs/operators.yaml: the 14 `result:` values now say what the engine prints,
                   and docs/LANGUAGE_REFERENCE.md is regenerated from them (14 lines, nothing
                   else moved). The test for this class already existed and already runs in CI:
                   `docs/test-examples.py`, 66/66 green locally after the change. What does not
                   exist is a *stage* of tools/ci.sh that runs the docs pipeline at all — that is
                   why seven months passed — and it is follow-up card `t_4a81d75f`, not something
                   this entry can claim.
Why it must stay:  The examples ARE the documentation: docs/generate-reference.py builds
                   docs/LANGUAGE_REFERENCE.md out of them and the Pages job publishes that file,
                   so an expectation that disagrees with the engine is either a false statement on
                   the public site or a red deploy — here it was both, for seven months, on a
                   pipeline whose only failure signal was a red x on GitHub. Editing the
                   expectations rather than restoring the wrapper is the direction the engine
                   went deliberately in eeb2014c; `--array=<key>` is the opt-in for getting the
                   wrapper back in output, and the input-side `{"array": [...]}` convention (how a
                   literal array is passed as one argument) is untouched.

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
