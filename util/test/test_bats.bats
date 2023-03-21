#!/usr/bin/env bats

load "../../test/wct-bats.sh"

@test "test wct bats" {
    usepkg util apps
    t=$(topdir)
    [ -f "$t/build/apps/wire-cell" ] 
    [ -n "$util_src" ]
    [ -n "$(wcsonnet)" ] 
}

@test "dump env" {
    wcb dumpenv
    [[ "$status" -eq 0 ]]
}
    
