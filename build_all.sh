#!/bin/bash
set -ex
cd "$(dirname "$0")"

racdoor/build.sh
payloads/hello/build.sh
