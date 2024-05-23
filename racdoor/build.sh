#!/bin/bash
set -ex
cd "$(dirname "$0")"

gcc racdoor.c -o racdoor -I../include
