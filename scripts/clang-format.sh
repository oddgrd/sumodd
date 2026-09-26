#!/usr/bin/env bash
set -euo pipefail

find app tests \
    \( -path 'app/drivers/vl53l0x' -o -path 'tests/unity' \) -prune -o \
    -type f \( -name '*.c' -o -name '*.h' \) \
    ! -path 'app/config/SEGGER_RTT_Conf.h' \
    -print0 |
xargs -0 -r clang-format "$@"