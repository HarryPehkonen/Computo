# Computo — C++ Coding Standards (MANDATORY for all agents and humans)

This file is the binding standard for all C++ work in this repository. It
implements the *C++ Core Guidelines* (Type / Bounds / Lifetime profiles) in a
form that is machine-checkable and agent-followable. Exceptions are ALLOWED
for error handling — this is not MISRA/JSF.

Reference: https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines

## Hard rules (no exceptions)

1. **C++17 minimum.** No C-style code in new work.
2. **No raw owning pointers.** Use `std::unique_ptr` / `std::shared_ptr` and
   RAII. A raw pointer (or reference) is a *non-owning view* only.
3. **No `new` / `delete`**, no `malloc` / `free` in project code.
4. **Views over ranges:** `std::span` for contiguous buffers, `std::string_view`
   for read-only string parameters, instead of pointer+length.
5. **No `reinterpret_cast` or C-style casts.** `const_cast` only with a comment.
6. **No undefined behavior:** no reliance on signed overflow, no out-of-bounds
   access (use `.at()` or checked access), nothing read uninitialized —
   initialize every variable at declaration.
7. **`[[nodiscard]]`** on accessors and pure functions. If a call site must
   discard the value, cast to `(void)` with a comment.
8. **Exceptions are for error handling only**, never control flow. RAII
   guarantees cleanup on throw. Throw the project's documented exception type,
   never built-ins.
9. **No global mutable state.** Threaded code must be race-free: mutex/atomic,
   prefer immutable data.
10. **Never keep iterators or references across container mutation.** Re-fetch
    after any mutating call (the Lifetime profile's core rule).
11. **Zero warnings.** Project targets compile with
    `-Wall -Wextra -Wpedantic -Werror` where wired (see Tooling status).
12. **TDD.** Write the failing test first, watch it fail, implement, watch it
    pass. Bug fixes too: failing test that isolates the bug → fix → green.
13. **Sanitizer gate:** all tests must pass under ASan+UBSan before a change is
    done (see Tooling status for this repo's exact command).

## Style

- clang-format per the repo's `.clang-format` (run the repo's format target if
  present). Match existing naming conventions.
- Keep the naming/architecture conventions documented in this repo's
  CLAUDE.md / README.

## Definition of done (agent checklist)

- [ ] Zero-warning build (see Tooling status — `-Werror` where wired)
- [ ] All tests pass
- [ ] Tests pass under ASan+UBSan
- [ ] Tests pass in an OPTIMIZED build too — the `release` stage (Debug is not the only config
      CI builds)
- [ ] clang-tidy — the `tidy` stage is clean: zero findings, no baseline file
- [ ] No raw owning pointers / `new` / `reinterpret_cast` introduced
- [ ] Test written first (RED) for every behavior change or bug fix

## Tooling status (this repo, as of 2026-09-20)

**The gate is `tools/ci.sh`** — one script, three callers: run it by hand, and the two git
hooks call it (`.githooks/pre-commit` = the fast tier, `.githooks/pre-push` = the full tier).
Run `./tools/ci.sh` before declaring a change done; `./tools/ci.sh --list` prints the stages.
The reasons each stage exists, with the measurements behind them, are in `INCIDENTS.md` and
in the adaptation notes at the top of the script.

- Warnings: `-Wall -Wextra -Wpedantic -Werror` on Computo's own targets (CMakeLists.txt).
  The `build` stage additionally fails on any `warning:` a target without `-Werror` emitted.
  One exception, bounded: on GNU < 15 and non-Debug configs only, those targets add
  `-Wno-maybe-uninitialized`, because gcc 13 and gcc 14 both report that false positive
  inside libstdc++'s variant machinery at -O2/-O3 (it is what kept the Pages deploy red).
  The gate builds Debug, where the diagnostic does not exist, so its warning set is
  unchanged; INCIDENTS.md (2026-09-20) has the measurements and the cost.
- Optimized builds: the `release` stage — a second build dir at `CI_RELEASE_BUILD_TYPE`
  (`Release`), the same "no `warning:` anywhere" rule applied to its log, and the same test
  command run in it. Every other stage builds `CI_BUILD_TYPE=Debug`, and the Pages workflow
  passes no build type while JSOM's subproject defaults it to Release, so this stage is the
  only place an optimized build of this repo is checked at all. Measured (4 cores, load ~2.5):
  146 s cold, ~5 s on a one-source push, because the dir is reused — full tier only, never
  pre-commit.
- Tests: `ctest --test-dir build --output-on-failure` (the `tests` stage). Nothing is
  filtered out of it.
- Sanitizers: the `asan` and `tsan` stages, each in its own build dir. `CMakeLists.txt`
  still carries `ENABLE_ASAN` / `ENABLE_UBSAN` / `ENABLE_TSAN` switches for one-off local
  builds; the stages are what the gate runs.
- clang-tidy: the `tidy` stage — **zero findings required, no baseline file**. The curated
  value-only check set and the repo-scoped `HeaderFilterRegex` live in `.clang-tidy`.
  `make lint` is the older CMake target and analyses only part of the tree, so prefer the
  stage.
- clang-format: the `format` stage (the files the branch touches, with the level-checkout
  fallback).
- Documentation: the `docs` stage — all 66 examples in docs/operators.yaml executed against
  the binary the `build` stage produced, README.md's own result examples graded by
  `docs/check-readme-examples.py` against the same binary (99 of them when that check landed
  on 2026-09-20; it FAILS if it can extract fewer than 90, so a prose rewrite cannot quietly
  reduce the coverage), operator coverage, and docs/LANGUAGE_REFERENCE.md plus the two
  generated indexes held byte-for-byte to what docs/operators.yaml generates.
  It needs python3 + PyYAML (`apt install python3 python3-yaml`) and FAILS rather than
  skipping when they are missing. The README examples the script cannot read unambiguously
  are counted in its summary and listed by line number in its output; the 41 that had drifted
  before this check existed are in INCIDENTS.md (2026-09-20).
- Dependency on a clean tree: the `pristine` stage builds and tests `git archive HEAD` in a
  temp dir, which is what proves the committed tree is complete.

## Upstream reference

https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines — the Profiles
chapter (Type / Bounds / Lifetime) is the conceptual core.
