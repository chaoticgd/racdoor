#!/bin/bash
set -ex
cd "$(dirname "$0")"

source "../../scripts/find_toolchain.sh"

RD_CXX_FLAGS="-ffreestanding -nostdinc -nostdlib -mabi=eabi -mno-abicalls -fno-exceptions"

"$RD_CXX" -c hello.cpp -o hello.o $RD_CXX_FLAGS
"$RD_LD" hello.o -o hello.elf -T ../linkfile.ld -m "$RD_LD_EMULATION"
