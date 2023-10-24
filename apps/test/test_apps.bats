#!/usr/bin/env bats

bats_load_library wct-bats.sh

@test "test wct bats in apps" {
    usepkg util apps
    t=$(topdir)
    echo "TOP $t"
    [ -f "$t/build/apps/wire-cell" ] 
    [ -n "$util_src" ]
    wcsonnet
    [ -n "$output" ] 
}

@test "wire-cell help" {
    
    # This is actually a wrapper in wct-bats.sh which does basic
    # echoing of output checking of success status code
    wire-cell --help

    # Assert expected info
    [[ -n "$(echo $output | grep 'Wire-Cell Toolkit')" ]]
}
