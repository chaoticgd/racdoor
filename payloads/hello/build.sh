#!/bin/bash
set -ex
cd "$(dirname "$0")"

source "../../scripts/find_toolchain.sh"

RD_CXX_FLAGS="-ffreestanding -nostdinc -nostdlib -mabi=eabi -mno-abicalls -fno-exceptions -D_EE"

"$RD_CXX" -c hello.cpp -o hello.o $RD_CXX_FLAGS
../../addrgen/addrgen hello.o -t ../../tables/SCES-53285.csv -o addrtbl.o -v
"$RD_LD" hello.o addrtbl.o -o hello.elf -T ../linkfile.ld -m "$RD_LD_EMULATION"
