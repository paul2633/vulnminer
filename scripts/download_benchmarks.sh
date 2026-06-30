#!/usr/bin/env bash

clone_if_missing() {
    local url="$1"
    local dir="$2"

    if [ ! -d "$dir" ]; then
        git clone "$url" "$dir"
    fi
}

mkdir -p benchmarks

clone_if_missing https://github.com/Genymobile/scrcpy benchmarks/scrcpy
clone_if_missing https://github.com/pbatard/rufus benchmarks/rufus
