#!/bin/bash

# Copyright (c) 2019 Tim Perkins

set -o errexit
set -o nounset
set -o pipefail
IFS=$'\n\t'

readonly CPP_FILENAME_REGEX=".*\.[ch]pp$"
readonly CPP_LOCKFILE_REGEX=".*\.#.+\.[ch]pp$"
readonly DIRS='"tsig" "tests" "examples"'

readonly SCRIPT_DIR="$(dirname "$(readlink -f "$0")")"

function detect_clang_format {
    readonly CLANG_FORMAT_COMMANDS=(
        "clang-format-13"
        "clang-format-12"
        "clang-format-11"
        "clang-format-10"
        "clang-format"
    )
    for clang_format_cmd in "${CLANG_FORMAT_COMMANDS[@]}"; do
        if command -v "${clang_format_cmd}" &>/dev/null; then
            echo "${clang_format_cmd}"
            return
        fi
    done
}

readonly CLANG_FORMAT_COMMAND="$(detect_clang_format)"

if [ -z "${CLANG_FORMAT_COMMAND}" ]; then
    echo "Could not detect clang-format command!" 2>&1
    exit 1
fi

readonly lockfiles="$(cd "${SCRIPT_DIR}" && eval "find ${DIRS} -regex '${CPP_LOCKFILE_REGEX}'")"
if [ -n "${lockfiles}" ]; then
    echo "Emacs Lockfiles detected! Save your files!" 2>&1
    exit 1
fi

(cd "${SCRIPT_DIR}" && eval "find ${DIRS} -regex '${CPP_FILENAME_REGEX}'" \
    | xargs "${CLANG_FORMAT_COMMAND}" -i)

exit 0
