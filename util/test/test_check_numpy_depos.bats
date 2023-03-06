#!/usr/bin/env bats

load "wct-bats.sh"

@test "check muon depos" {
    usepkg util test
    run check_numpy_depos $test_src/data/muon-depos.npz
    echo "$output"
    [ "$status" -eq 0 ]
    [ $(echo "$output" | grep 'row=' | wc -l) = 32825 ]
}

