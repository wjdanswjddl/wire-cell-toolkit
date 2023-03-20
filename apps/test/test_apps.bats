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

@test "wire-cell help" {
    
    # Set environment with certain wcb Waf build variables
    wcb_env WIRE_CELL
    [[ -n "$WIRE_CELL" ]]
    
    # Defined by wcb_env, "run" by bats
    run $WIRE_CELL --help
    
    # Bats will only show this if test fails.
    echo "$output"
    
    # Assert no error status code
    [[ "$status" -eq 0 ]]
    
    # Assert expected info
    [[ -n "$(echo $output | grep 'Wire-Cell Toolkit')" ]]
}
