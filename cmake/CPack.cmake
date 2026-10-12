# CPack configuration for Computo — included at the END of CMakeLists.txt, after every
# install() rule: CPack consumes the install rules, so anything installed after this
# include would silently miss the packages.
#
# One CMake tree, two packages, Debian convention:
#
#   computo         the CLI binary (/usr/bin/computo) and its man page      — Section: utils
#   libcomputo-dev  the static library and the public header                — Section: libdevel
#
# The split rides on CPack COMPONENTS: every install() rule names a COMPONENT
# (`computo` or `devel`), and each component becomes one DEB / RPM subpackage below.
# libcomputo-dev carries no Depends on computo: the library is STATIC, so consumers
# need it at build time, not install time, and there is no runtime linkage reason.
#
# Usage (also documented in PACKAGING.md):
#   cmake -S . -B build && cmake --build build
#   cpack -G DEB --config build/CPackConfig.cmake -B build/pkgs
#   cpack -G RPM --config build/CPackConfig.cmake -B build/pkgs

set(CPACK_PACKAGE_VENDOR "Harri Pehkonen")
set(CPACK_PACKAGE_CONTACT "Harri Pehkonen <harry.pehkonen@gmail.com>")
set(CPACK_PACKAGING_INSTALL_PREFIX "/usr")

# README's own wording, not marketing copy.
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY
    "Safe, sandboxed, thread-safe JSON-native data transformation engine")
set(CPACK_PACKAGE_DESCRIPTION
    "Computo is a safe, sandboxed, thread-safe JSON-native data transformation
engine with a Lisp-like language syntax expressed in JSON. This package ships
the unified CLI tool: script execution mode, an interactive REPL, and
conversion between JSON syntax and the human-friendly sugar syntax.")

set(CPACK_COMPONENTS_ALL computo devel)
set(CPACK_DEB_COMPONENT_INSTALL ON)
set(CPACK_RPM_COMPONENT_INSTALL ON)

# ---------------------------------------------------------------- Debian
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Harri Pehkonen <harry.pehkonen@gmail.com>")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)          # picks up libreadline.so et al. automatically
set(CPACK_DEBIAN_ENABLE_COMPONENT_DEPENDS ON)

set(CPACK_DEBIAN_COMPUTO_PACKAGE_NAME computo)
set(CPACK_DEBIAN_COMPUTO_PACKAGE_SECTION utils)
set(CPACK_DEBIAN_COMPUTO_PACKAGE_DESCRIPTION
    "Safe, sandboxed, thread-safe JSON-native data transformation engine (CLI)")

set(CPACK_DEBIAN_DEVEL_PACKAGE_NAME libcomputo-dev)
set(CPACK_DEBIAN_DEVEL_PACKAGE_SECTION libdevel)
set(CPACK_DEBIAN_DEVEL_PACKAGE_DESCRIPTION
    "Static library and public header for the Computo JSON transformation
engine. The library is static: consumers need it at build time only, so this
package carries no runtime Depends.")

# ---------------------------------------------------------------- RPM
set(CPACK_RPM_FILE_NAME RPM-DEFAULT)
set(CPACK_RPM_PACKAGE_LICENSE "MIT")
set(CPACK_RPM_PACKAGE_AUTOREQPROV ON)

set(CPACK_RPM_COMPUTO_PACKAGE_NAME computo)
set(CPACK_RPM_COMPUTO_PACKAGE_SUMMARY "${CPACK_PACKAGE_DESCRIPTION_SUMMARY} (CLI)")
set(CPACK_RPM_COMPUTO_PACKAGE_DESCRIPTION "${CPACK_PACKAGE_DESCRIPTION}")

set(CPACK_RPM_DEVEL_PACKAGE_NAME computo-devel)
set(CPACK_RPM_DEVEL_PACKAGE_SUMMARY "Development files for Computo (static lib + header)")
set(CPACK_RPM_DEVEL_PACKAGE_DESCRIPTION
    "Static library and public header for the Computo JSON transformation engine.
The library is static: consumers need it at build time only.")

include(CPack)
