#!/bin/bash

# Test some main jsonnet that are expected to compile w/out args

load ../../test/wct-bats.sh
usepkg cfg
jsonnet=$(wcb_env_value JSONNET)
wcsonnet=$(wcb_env_value WCONNET)

function myjsonnet () {
    local cfgfile=$1; shift
    t1=$(date +%s)
#    $jsonnet -J "${cfg_src}" $@ "${cfg_src}/pgrapher/experiment/uboone/$cfgfile"
    $wcsonnet -J "${cfg_src}" $@ "${cfg_src}/pgrapher/experiment/uboone/$cfgfile"
    t2=$(date +%s)
    [[ $(( $t2 - $t1 )) -le 2 ]]
}


@test "mains with no args" {

    local mains=(wct-jsondepo-sim-nf-sp.jsonnet
                 wct-sim-check.jsonnet
                 wct-sim-deposplat.jsonnet
                 wct-sim-zipper-check.jsonnet
                 wcls-data-nf-sp-common.jsonnet
                 wcls-sim-drift.jsonnet
                 wcls-sim-drift-simchannel.jsonnet
                 wcls-sim.jsonnet
                 wcls-sim-nf-sp.jsonnet
                 wct-sim-ideal-sig.jsonnet
                 wct-sim-ideal-sn-nf-sp.jsonnet)
    for main in ${mains[*]}
    do
        # yell $main
        run myjsonnet $main
        echo "$output"
        [[ "$status" -eq 0 ]]
        [[ -n "$output" ]]
    done
}    
                


@test "mains with epoch extVar" {

    mains=(wcls-data-nf-sp.jsonnet
           wcls-nf-sp.jsonnet)
    for main in ${mains[*]}
    do
        run myjsonnet -V epoch=after $main
        echo "$output"
        [[ "$status" -eq 0 ]]
        [[ -n "$output" ]]
    done
}

