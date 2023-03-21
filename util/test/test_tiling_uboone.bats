#!/usr/bin/env bats

# Targetted test for tiling with check_act2viz using microboone wires

load ../../test/wct-bats.sh

setup_file () {
    usepkg util
    cd_tmp file

    local wf="$(resolve_file microboone-celltree-wires-v2.1.json.bz2)"
    [[ -s "$wf" ]]
    local af="$(relative_path activities3.txt)"
    [[ -s "$af" ]]
    run check_act2viz -o blobs.svg -w "$wf" -n 0.01 -d blobs.txt "$af"
    echo "$output"
    [[ "$status" -eq 0 ]]
    [[ -s blobs.svg ]]
    [[ -s blobs.txt ]]
}

@test "reproduce act2vis blob finding" {
    local ft="$(tmpdir file)"
    local bf="$(relative_path activities3-act2viz-uboone.txt)"
    [[ -s "$bf" ]]

    run diff -u "$bf" "$ft/blobs.txt"
    echo "$BATS_RUN_COMMAND"
    echo "$output"
    [ "$status" -eq 0 ]
    [ -z "$output" ] 
}

# @test "reproduce full blob finding" {
#     local ft="$(tmpdir file)"
#     local af="$(relative_path activities3-full.txt)"
#     [[ -s "$af" ]]

#     run diff -u "$af" "$ft/blobs.txt"
#     echo "$BATS_RUN_COMMAND"
#     echo "$output"
#     [ "$status" -eq 0 ]
#     [ -z "$output" ] 
# }

@test "no missing bounds" {
    local ft="$(tmpdir file)"
    run grep -E 'pind:\[([0-9]+),\1\]' $ft/blobs.txt
    [ -z "$output" ]
}
