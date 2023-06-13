#!/usr/bin/env bats

# Targetted test for tiling with check_act2viz using microboone wires

bats_load_library wct-bats.sh

setup_file () {
    usepkg util
    cd_tmp file

    local wf="$(resolve_file microboone-celltree-wires-v2.1.json.bz2)"
    [[ -s "$wf" ]]
    local af="$(relative_path activities3.txt)"
    [[ -s "$af" ]]
    check check_act2viz -o blobs.svg -w "$wf" -n 0.01 -d blobs.txt "$af"
    [[ -s blobs.svg ]]
    [[ -s blobs.txt ]]

    saveout -c plots blobs.svg
}

@test "reproduce act2vis blob finding" {

    local bf="$(relative_path activities3-act2viz-uboone.txt)"
    [[ -s "$bf" ]]

    check diff -u "$bf" "blobs.txt"
    [ -z "$output" ] 
}

@test "no missing bounds" {
    local got=$(grep -E 'pind:\[([0-9]+),\1\]' blobs.txt)
    [ -z "$got" ]
}
