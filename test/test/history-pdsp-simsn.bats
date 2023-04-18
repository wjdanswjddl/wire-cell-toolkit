#!/usr/bin/env bats

# Process history files from test-pdsp-simsn.bats

bats_load_library wct-bats.sh

# bats file_tags=report


function comp1d () {
    local kind=$1; shift
    local iplane=$1; shift

    local letters=("u" "v" "w")
    local chmins=(0 800 1600)
    local chmaxs=(800 1600 2560)

    local letter=${letters[$iplane]}
    local chmin=${chmins[$iplane]}
    local chmax=${chmaxs[$iplane]}
    local fig="pdsp-comp1d-${kind}-${letter}.png"
    local wcplot=$(wcb_env_value WCPLOT)

    $wcplot comp1d -s --chmin $chmin --chmax $chmax \
            -n $kind --transform ac \
            -o "$fig" $@
    [[ -s "$fig" ]]
    saveout -c reports $fig
}

@test "pdsp comp1d" {
    local past=( $(historical_files --current test-pdsp-simsn/frames-adc.tar.gz | tail -3) )
    for kind in wave spec
    do
        for ipln in 0 1 2
        do
            comp1d $kind $ipln ${past[@]}
        done
    done
}
