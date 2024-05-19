#!/bin/bash
set -ex

# Search for a C toolchain to use.
if [ -z "$EE_CC" ]; then
	# Look for the new homebrew compiler first.
	if [ "$(type -aP mips64r5900el-ps2-elf-gcc)" ]; then
		export EE_CC="$(type -aP mips64r5900el-ps2-elf-gcc)"
		export EE_LD="$(type -aP mips64r5900el-ps2-elf-ld)"
	# Look for the old homebrew compiler next.
	elif [ "$(type -aP ee-gcc)" ]; then
		export EE_CC="$(type -aP ee-gcc)"
		export EE_LD="$(type -aP ee-ld)"
	else
		echo "EE_CC not set and can't be guessed." 1>&2
		exit 1
	fi
fi

if [ -z "$EE_LD" ]; then
	echo "EE_LD not set." 1>&2
	exit 1
fi

echo "EE_CC=$EE_CC"
