#!/usr/bin/env bats

load "../../test/wct-bats.sh"

@test "test wct bats in apps" {
    usepkg util apps
    t=$(top)
    echo "TOP $t"
    [ -f "$t/build/apps/wire-cell" ] 
    [ -n "$util_src" ]
    [ -n "$(wcsonnet)" ] 
}

