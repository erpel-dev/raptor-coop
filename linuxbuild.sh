#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")"

APP_ID="io.github.erpel_dev.Raptor"
FLATPAK_MANIFEST="pkg/flatpak/${APP_ID}.yml"
FLATPAK_BUILD_DIR="build-flatpak"
FLATPAK_REPO_DIR="repo-flatpak"
FLATPAK_STATE_DIR=".flatpak-builder"
BUNDLE_NAME="raptor.flatpak"
LOCAL_REMOTE="raptor-local"

cleanup_flatpak_dirs() {
    rm -rf "${FLATPAK_BUILD_DIR}" "${FLATPAK_REPO_DIR}" "${FLATPAK_STATE_DIR}" "${BUNDLE_NAME}"
}

build_native() {
    echo "==> Native CMake release build"
    rm -rf build
    mkdir build
    (
        cd build
        cmake -DCMAKE_BUILD_TYPE=Release ..
        make -j"$(nproc)"
    )
    echo "Built: build/bin/raptor and build/bin/raptorsetup"
}

build_flatpak() {
    if ! command -v flatpak-builder >/dev/null 2>&1; then
        echo "flatpak-builder is required for Flatpak builds" >&2
        exit 1
    fi

    echo "==> Flatpak build (${APP_ID})"
    cleanup_flatpak_dirs

    # Temporarily move the native build tree aside so type:dir does not copy it.
    native_build_stash=""
    if [[ -d build ]]; then
        native_build_stash="build.native.$$"
        mv build "${native_build_stash}"
    fi

    restore_native_build() {
        if [[ -n "${native_build_stash}" && -d "${native_build_stash}" ]]; then
            mv "${native_build_stash}" build
        fi
    }
    trap restore_native_build EXIT

    flatpak-builder \
        --force-clean \
        --user \
        --install \
        --repo="${FLATPAK_REPO_DIR}" \
        --state-dir="${FLATPAK_STATE_DIR}" \
        "${FLATPAK_BUILD_DIR}" \
        "${FLATPAK_MANIFEST}"

    trap - EXIT
    restore_native_build

    echo "==> Creating bundle ${BUNDLE_NAME}"
    flatpak build-bundle "${FLATPAK_REPO_DIR}" "${BUNDLE_NAME}" "${APP_ID}"

    echo
    echo "Installed (user): ${APP_ID}"
    echo "Bundle: ${BUNDLE_NAME}"
    echo "Run: flatpak run ${APP_ID}"
    echo "Setup: flatpak run --command=raptorsetup ${APP_ID}"
    echo "Assets: ~/.var/app/${APP_ID}/data/Raptor/"
}

cat << EOF
********************************************************************************
*                                                                              *
* Raptor build script for Linux                                                *
*                                                                              *
********************************************************************************

Select build:

  1. Native release build (CMake)
  2. Flatpak build + install (user) + bundle
  3. Native + Flatpak
  4. Clean all
  0. Exit

EOF

read -r -p "Select 0-4: " select

case "${select}" in
    0)
        echo "Abort"
        exit 0
        ;;
    1)
        build_native
        ;;
    2)
        build_flatpak
        ;;
    3)
        build_native
        build_flatpak
        ;;
    4)
        rm -rf build
        cleanup_flatpak_dirs
        echo "Cleaned all"
        ;;
    *)
        echo "Incorrect entry" >&2
        exit 1
        ;;
esac
