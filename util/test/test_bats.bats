#!/usr/bin/env bats

load "../../test/wct-bats.sh"

@test "test wct bats" {
    usepkg util apps
    t=$(topdir)
    [ -f "$t/build/apps/wire-cell" ] 
    [ -n "$util_src" ]
    [ -n "$(wcsonnet)" ] 
}

@test "dump waf env" {
    run wcb dumpenv
    echo "$output"
    [[ "$status" -eq 0 ]]
}
@test "dump shell env" {
    run env
    echo "$output"
    [[ "$status" -eq 0 ]]
}
    
