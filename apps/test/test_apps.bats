#!/usr/bin/env bats

load "../../test/wct-bats.sh"

@test "test wct bats in apps" {
    usepkg util apps
    t=$(topdir)
    echo "TOP $t"
    [ -f "$t/build/apps/wire-cell" ] 
    [ -n "$util_src" ]
    [ -n "$(wcsonnet)" ] 
}

@test "wire-cell help" {
    
    # Defined by wcb_env, "run" by bats
    run wct --help
    
    # Bats will only show this if test fails.
    echo "$output"
    
    # Assert no error status code
    [[ "$status" -eq 0 ]]
    
    # Assert expected info
    [[ -n "$(echo $output | grep 'Wire-Cell Toolkit')" ]]
}
