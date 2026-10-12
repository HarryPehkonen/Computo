# Packaging Computo with CPack

One CMake tree produces the DEB (Debian 13, amd64) and RPM (Fedora/Nobara)
packages. The install rules use GNUInstallDirs (`CMAKE_INSTALL_LIBDIR` and
friends), so the same tree lands files in `lib/x86_64-linux-gnu` on Debian and
`lib64` on Fedora — no per-distro paths in the repo.

One wrinkle is handled explicitly: GNUInstallDirs applies its Debian multiarch
default (`lib/<tuple>`) only when the configure prefix is already `/usr`, while
a plain configure here uses `/usr/local` and CPack moves the install root to
`/usr` only at packaging time (`CPACK_PACKAGING_INSTALL_PREFIX`). By then the
libdir is baked — the first packaging run shipped `/usr/lib/libcomputo.a` that
way. CMakeLists.txt therefore re-applies the multiarch rule to the computed
default right after `include(GNUInstallDirs)`, guarded by `/etc/debian_version`
plus `CMAKE_LIBRARY_ARCHITECTURE`, so Fedora keeps GNUInstallDirs' own `lib64`
default. An explicit `-DCMAKE_INSTALL_LIBDIR=...` from the caller still wins.

## What gets built

| package (DEB name) | RPM name | contents | Section / group |
|---|---|---|---|
| `computo` | `computo` | `/usr/bin/computo`, man page `usr/share/man/man1/computo.1` | utils |
| `libcomputo-dev` | `computo-devel` | `usr/lib/x86_64-linux-gnu/libcomputo.a` (Debian; `usr/lib64/` on Fedora), `usr/include/computo.hpp` | libdevel |

The split rides on CPack components: every `install()` rule in CMakeLists.txt
names a component (`computo` or `devel`), and `cmake/CPack.cmake` maps each
component to one DEB/RPM subpackage. `libcomputo-dev` carries no runtime
Depends: the library is static, so consumers need it at build time, not
install time. The CLI's runtime Depends are computed by
`CPACK_DEBIAN_PACKAGE_SHLIBDEPS` (dpkg-shlibdeps) on Debian and by
`AUTOREQPROV` on the RPM side.

The REPL links readline when it is found; a build without it degrades to the
basic-input REPL (a WARNING at configure). The package build does not accept
that degradation: `scripts/package.sh` configures with
`-DCOMPUTO_REQUIRE_READLINE=ON`, which turns a missing readline into a
configure error — otherwise the deb's feature set would depend on whichever
`-dev` packages happened to be installed on the build host. The gate then
asserts the result: the packaged binary's NEEDED entries must be covered by
the deb's Depends, and readline must be among them.

The version is not set here: CPack inherits `project(Computo VERSION 1.0.0)`,
the same single source the gate's `version` stage certifies.

## Usage

```bash
cmake -S . -B build && cmake --build build -j
cpack -G DEB --config build/CPackConfig.cmake -B build/pkgs   # Debian
cpack -G RPM --config build/CPackConfig.cmake -B build/pkgs   # Fedora/Nobara
```

Inspect before installing:

```bash
dpkg-deb -I build/pkgs/computo_*.deb
dpkg-deb -c build/pkgs/computo_*.deb
rpm -qlp build/pkgs/computo-*.x86_64.rpm
```

## The dependency-leak guard

JSOM is FetchContent'd in-tree, so its install rules could ship JSOM's
artifacts inside Computo's packages — the exact failure mode demonstrated on
CodeBundler, where FSMgine files leaked into the first .deb. Two guards:

1. `JSOM_INSTALL` is forced OFF before `FetchContent_MakeAvailable` in
   CMakeLists.txt (JSOM already defaults it OFF when not top-level; the FORCE
   pins it against cache staleness or upstream change).
2. The gate's `package` stage (`scripts/package.sh`) builds fresh, runs
   `cpack -G "DEB;RPM"`, and fails on any jsom path in any package, plus on
   any split regression (dev artifacts in the CLI package or vice versa), on
   missing man page, wrong Section/Maintainer fields, a libdir that is not the
   Debian multiarch path, a Depends that does not cover the packaged binary's
   NEEDED entries, or a packaged binary that does not run from an extracted
   root.

The RPM subpackages build locally where `rpmbuild` exists (Debian 13 host has
it); the real RPM *install* test belongs on nobara-pc, which is Fedora-based.
