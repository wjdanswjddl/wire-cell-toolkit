#!/usr/bin/env bats

bats_load_library wct-bats.sh

@test "check muon depos" {
    skip_if_no_input

    usepkg util                 # so check_numpy_depos is in PATH

    npz=$(input_file depos/muon.npz)
    [[ -n "$npz" ]]

    check check_numpy_depos "$npz"
    [ $(echo "$output" | grep 'row=' | wc -l) = 32825 ]
}

