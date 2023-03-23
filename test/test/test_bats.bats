#!/usr/bin/env bats

load "../../test/wct-bats.sh"

setup_file () {
    [[ "$(divine_context)" == "file" ]]
}

@test "devine context" {
    [[ "$(divine_context)" == "test" ]]
}

@test "test wct bats" {
    usepkg util apps cfg
    t=$(topdir)
    [ -f "$t/build/apps/wire-cell" ] 
    [ -n "$util_src" ]
    [ -d "$util_src" ]
    [ -n "$apps_src" ]
    [ -d "$apps_src" ]
    [ -n "$cfg_src" ]
    [ -d "$cfg_src" ]
}

@test "dump waf env" {
    run wcb dumpenv 
    echo "$output"
    [[ "$status" -eq 0 ]]
}
@test "dump shell env" {
    run bash -c "env|grep =|sort"
    echo "$output"
    [[ "$status" -eq 0 ]]
}
@test "dump bats env" {
    run bash -c "env|grep BATS|sort"
    echo "$output"
    [[ "$status" -eq 0 ]]
}
    
