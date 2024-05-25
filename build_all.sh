#!/bin/bash
set -ex
cd "$(dirname "$0")"

addrgen/build.sh
racdoor/build.sh
payloads/hello/build.sh
