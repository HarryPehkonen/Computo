#!/usr/bin/env bash
#
# package — prove the install rules and the CPack packages, not just that they exist.
#
# Called from gate.toml as `[stage.package] cmd = "scripts/package.sh"`. The verdict
# vocabulary (ci_begin/ci_pass/ci_fail/ci_skip) is defined in scripts/gate-env.sh.
#
# What this stage asserts, and why each assertion exists:
#
#   1. Both DEB packages build, with the Debian-conventional names (computo, libcomputo-dev).
#   2. The CLI package carries /usr/bin/computo and the man page; the dev package carries
#      the static library and the header. A package whose contents nobody lists is how the
#      CodeBundler leak shipped (FSMgine files inside the first .deb — INCIDENTS.md there).
#   3. NO jsom path appears in either package. JSOM is FetchContent'd in-tree; if its
#      install rules ever leak into ours, this line catches it in the gate, not in an
#      installed system.
#   4. Control fields: Section, Maintainer, Depends — read back with dpkg-deb -I.
#   5. The packaged binary runs: dpkg-deb -x into a scratch root, execute --version from
#      there. Layout alone does not prove the package works.
#   6. Both RPM subpackages build and list the same file split (rpmbuild is available on
#      this host; the nobara-pc install test remains the real RPM verdict).
#
set -uo pipefail
. "$(dirname "$0")/gate-env.sh"

run_stage() {
    ci_begin "package (CPack DEB + RPM: names, contents, fields, no dependency leaks)"

    local tool missing=""
    for tool in cmake cpack dpkg-deb rpmbuild rpm; do
        command -v "$tool" >/dev/null 2>&1 || missing="$missing $tool"
    done
    if [ -n "$missing" ]; then
        ci_fail package "missing tool(s):$missing — the packaging stage must not SKIP silently (CI_STRICT_TOOLS=1 would fail anyway)"
    fi

    local tmp
    tmp=$(mktemp -d "${TMPDIR:-/tmp}/ci-package-XXXXXX")
    RUN_TMP_DIRS+=("$tmp")
    local pkgroot="$tmp/pkg"

    # --- 1. fresh configure + build (a stale tree could hide an install-rule regression)
    printf '    fresh configure + build in %s/build\n' "$tmp"
    if ! cmake -S . -B "$tmp/build" -DCMAKE_BUILD_TYPE=Release \
            > "$CI_LOG_DIR/package-configure.log" 2>&1; then
        ci_fail package "cmake configure failed" "$CI_LOG_DIR/package-configure.log"
    fi
    if ! cmake --build "$tmp/build" -j "$CI_JOBS" \
            > "$CI_LOG_DIR/package-build.log" 2>&1; then
        ci_fail package "cmake --build failed" "$CI_LOG_DIR/package-build.log"
    fi

    # --- 2. CPack: DEB and RPM from the same tree
    if ! cpack -G "DEB;RPM" --config "$tmp/build/CPackConfig.cmake" -B "$pkgroot" \
            > "$CI_LOG_DIR/package-cpack.log" 2>&1; then
        ci_fail package "cpack failed" "$CI_LOG_DIR/package-cpack.log"
    fi
    ls -1 "$pkgroot" | sed 's/^/    /'

    local cli_deb dev_deb
    cli_deb=$(ls "$pkgroot"/computo_*_amd64.deb 2>/dev/null | head -1)
    dev_deb=$(ls "$pkgroot"/libcomputo-dev_*_amd64.deb 2>/dev/null | head -1)
    [ -n "$cli_deb" ] || ci_fail package "no computo_<version>_amd64.deb in $pkgroot — CPACK_DEBIAN_COMPUTO_PACKAGE_NAME / DEB-DEFAULT naming regressed (see $CI_LOG_DIR/package-cpack.log)"
    [ -n "$dev_deb" ] || ci_fail package "no libcomputo-dev_<version>_amd64.deb in $pkgroot — the dev subpackage is gone (see $CI_LOG_DIR/package-cpack.log)"

    # --- 3. contents: the split must hold, and nothing may leak
    local contents split_bad="" leak
    contents=$(dpkg-deb -c "$cli_deb"; dpkg-deb -c "$dev_deb")
    # The split, as globs over the package listing (dpkg paths start with ./).
    # `grep` (NOT grep -q): with pipefail, -q's early exit SIGPIPEs dpkg-deb and the
    # pipeline fails even when the match is there — measured 2026-10-12.
    has_entry() {  # has_entry <pkg> <regex> — exit 0 when one listing line matches
        dpkg-deb -c "$1" | grep -E "$2" >/dev/null
    }
    has_entry "$cli_deb" '\./usr/bin/computo$' \
        || ci_fail package "CLI package does not ship /usr/bin/computo"
    has_entry "$cli_deb" '\./usr/share/man/man1/computo\.1(\.gz)?$' \
        || ci_fail package "CLI package does not ship the man page (usr/share/man/man1/computo.1)"
    has_entry "$dev_deb" '\./usr/lib.*/libcomputo\.a$' \
        || ci_fail package "dev package does not ship libcomputo.a under the multiarch libdir"
    has_entry "$dev_deb" '\./usr/include/computo\.hpp$' \
        || ci_fail package "dev package does not ship include/computo.hpp"
    has_entry "$cli_deb" '\.(a|hpp|so)$' \
        && split_bad="CLI package ships dev/library artifacts"
    has_entry "$dev_deb" '\./usr/bin/' \
        && split_bad="$split_bad dev package ships a binary"
    [ -z "$split_bad" ] || ci_fail package "package split regressed: $split_bad"

    leak=$(printf '%s\n' "$contents" | grep -i jsom || true)
    [ -z "$leak" ] || ci_fail package "dependency leak: jsom files inside a Computo package (the CodeBundler failure mode):
$leak"

    # --- 4. control fields
    local fields
    fields=$(dpkg-deb -I "$cli_deb")
    grep -q '^ Package: computo$' <<<"$fields"    || ci_fail package "CLI deb: wrong Package field"
    grep -q '^ Section: utils$'   <<<"$fields"    || ci_fail package "CLI deb: wrong Section (want utils)"
    grep -q 'harry.pehkonen@gmail.com' <<<"$fields" || ci_fail package "CLI deb: Maintainer is not Harri Pehkonen <harry.pehkonen@gmail.com>"
    fields=$(dpkg-deb -I "$dev_deb")
    grep -q '^ Package: libcomputo-dev$' <<<"$fields" || ci_fail package "dev deb: wrong Package field"
    grep -q '^ Section: libdevel$'  <<<"$fields"     || ci_fail package "dev deb: wrong Section (want libdevel)"

    # --- 5. the packaged binary runs, from the extracted layout
    dpkg-deb -x "$cli_deb" "$tmp/root"
    local version
    version=$("$tmp/root/usr/bin/computo" --version 2>&1) \
        || ci_fail package "the packaged computo does not run from the extracted root"
    grep -q '1\.0\.0' <<<"$version" \
        || ci_fail package "packaged computo --version does not report 1.0.0: $version"
    printf '    packaged binary reports: %s\n' "$version"

    # --- 6. RPM subpackages: same split, read back with rpm -qlp
    local cli_rpm dev_rpm
    cli_rpm=$(ls "$pkgroot"/computo-[0-9]*.x86_64.rpm 2>/dev/null | head -1)
    dev_rpm=$(ls "$pkgroot"/computo-devel-*.x86_64.rpm 2>/dev/null | head -1)
    [ -n "$cli_rpm" ] || ci_fail package "no computo-<version>.x86_64.rpm — the RPM generator produced no CLI subpackage"
    [ -n "$dev_rpm" ] || ci_fail package "no computo-devel-<version>.x86_64.rpm — the RPM generator produced no devel subpackage"
    rpm -qlp "$cli_rpm" 2>/dev/null | grep '^/usr/bin/computo$' \
        || ci_fail package "RPM CLI subpackage does not ship /usr/bin/computo"
    rpm -qlp "$dev_rpm" 2>/dev/null | grep '^/usr/lib.*/libcomputo\.a$' \
        || ci_fail package "RPM devel subpackage does not ship libcomputo.a"
    rpm -qlp "$cli_rpm" "$dev_rpm" 2>/dev/null | grep -i jsom >/dev/null 2>/dev/null \
        && ci_fail package "dependency leak: jsom files inside an RPM subpackage"
    rpm -qp --qf '%{LICENSE}\n' "$cli_rpm" | grep -qi MIT \
        || ci_fail package "RPM License tag is not MIT"

    ci_pass package
}

run_stage
