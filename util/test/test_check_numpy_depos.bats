#!/usr/bin/env bats

load ../../test/wct-bats.sh

@test "check muon depos" {
    usepkg util test

    npz=$(resolve_file muon-depos.npz)
    [[ -n "$npz" ]]

    run check_numpy_depos "$npz"
    echo "$output"
    [ "$status" -eq 0 ]
    [ $(echo "$output" | grep 'row=' | wc -l) = 32825 ]
}

