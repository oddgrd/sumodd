#!/usr/bin/env bash
set -euo pipefail

find app tests \
    -path 'tests/unity' -prune -o \
    -type f \( -name '*.c' -o -name '*.h' \) \
    ! -path 'app/config/SEGGER_RTT_Conf.h' \
    -print0 |
xargs -0 -r clang-format "$@"