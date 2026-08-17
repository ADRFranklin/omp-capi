#!/usr/bin/env sh
set -eu

root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
out=${1:-"$root/build"}
mkdir -p "$out"

cc -O3 -fPIC -shared "$root/callback.c" -o "$out/libbench_c.so"
c++ -O3 "$root/host.cpp" -ldl -o "$out/bench_host"
(cd "$root/go" && CGO_ENABLED=1 go build -buildmode=c-shared -o "$out/libbench_go.so" callback.go)
rustc -C opt-level=3 --crate-type cdylib "$root/rust/callback.rs" -o "$out/libbench_rust.so"

"$out/bench_host" native native > "$out/results.csv"
"$out/bench_host" c "$out/libbench_c.so" | tail -n +2 >> "$out/results.csv"
"$out/bench_host" go "$out/libbench_go.so" | tail -n +2 >> "$out/results.csv"
"$out/bench_host" rust "$out/libbench_rust.so" | tail -n +2 >> "$out/results.csv"
cat "$out/results.csv"
