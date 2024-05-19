#!/bin/bash
set -ex
cd "$(dirname "$0")"

source "../../scripts/find_toolchain.sh"

CC_FLAGS="-ffreestanding -nostdinc -nostdlib"

"$EE_CC" -c hello.c -o hello.o $CC_FLAGS
"$EE_LD" hello.o -o hello.elf -T ../linkfile.ld
