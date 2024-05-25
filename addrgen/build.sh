#!/bin/bash
set -ex
cd "$(dirname "$0")"

gcc addrgen.c -o addrgen -I../include -g
