#!/usr/bin/env bats

load "../../test/wct-bats.sh"

@test "test wct bats" {
    usepkg util apps
    t=$(top)
    [ -f "$t/build/apps/wire-cell" ] 
    [ -n "$util_src" ]
    [ -n "$(wcsonnet)" ] 
}

