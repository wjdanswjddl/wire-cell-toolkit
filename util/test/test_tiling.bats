#!/usr/bin/env bats

bats_load_library wct-bats.sh

setup_file () {
    cd_tmp
    usepkg util

    local acts=$(relative_path activities3.txt)

    check check_act2viz -o blobs.svg -n 0.01 -d blobs.txt $acts
}

@test "reproduce act2vis blob finding" {

    cd_tmp file
    local act=$(relative_path activities3-act2viz.txt)
    check diff $act blobs.txt
    [ -z "$output" ] 
}

@test "no missing bounds" {
    cd_tmp file
    local got=$(grep -E 'pind:\[([0-9]+),\1\]' blobs.txt)
    [ -z "$got" ] 
}

