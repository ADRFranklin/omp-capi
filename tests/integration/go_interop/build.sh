#!/usr/bin/env sh
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")/../../.." && pwd)
output=${1:-"$root/build/capi-go-interop.so"}
cd "$root/tests/integration/go_interop"
CGO_ENABLED=1 go build -buildmode=c-shared -o "$output" main.go
