#!/bin/bash
set -ex

# Search for a C toolchain to use.
if [ -z "$RD_CC" ]; then
	# Look for the old homebrew toolchain next.
	if [ "$(type -aP ee-gcc)" ]; then
		export RD_CC="$(type -aP ee-gcc)"
		export RD_CXX="$(type -aP ee-g++)"
		export RD_LD="$(type -aP ee-ld)"
		export RD_LD_EMULATION="elf32l5900"
	# Don't look for the new homebrew toolchain for now since its implementation
	# of the MIPS EABI doesn't seem to match up with the games.
	# elif [ "$(type -aP mips64r5900el-ps2-elf-gcc)" ]; then
	# 	export RD_CC="$(type -aP mips64r5900el-ps2-elf-gcc)"
	# 	export RD_CXX="$(type -aP mips64r5900el-ps2-elf-g++)"
	# 	export RD_LD="$(type -aP mips64r5900el-ps2-elf-ld)"
	# 	export RD_LD_EMULATION="elf32lr5900"
	else
		echo "RD_CC not set and can't be guessed." 1>&2
		exit 1
	fi
fi

if [ -z "$RD_LD" ]; then
	echo "RD_LD not set." 1>&2
	exit 1
fi

if [ -z "$RD_CXX" ]; then
	echo "RD_CXX not set." 1>&2
	exit 1
fi

echo "RD_CC=$RD_CC"
echo "RD_CC=$RD_CXX"
echo "RD_CC=$RD_LD"
