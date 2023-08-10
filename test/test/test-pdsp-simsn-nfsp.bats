#!/usr/bin/env bats

# Run signal and noise simulation on PDSP APA0.

# Produces plots and history files
# bats file_tags=plots

# Started for https://github.com/WireCell/wire-cell-toolkit/pull/195

bats_load_library wct-bats.sh

log_file="wire-cell.log"
dag_file="dag.json"
tiers=(vlt adc nf sp)
function frame_file () {
    local tier="$1"; shift
    echo "frames-$tier.tar.gz"
}

# Compile Jsonnet to dag.json and run wire-cell.
setup_file () {

    skip_if_no_input

    cd_tmp file

    local cfg_file="${BATS_TEST_FILENAME%.bats}.jsonnet"
    [[ -n "$cfg_file" ]]
    [[ -s "$cfg_file" ]]

    local in_file="$(input_file depos/many.tar.bz2)"
    echo "in file: $in_file"
    [[ -f "$in_file" ]]

    declare -a args1=( $cfg_file $dag_file -A input="$in_file" )
    declare -a deps1=( -s "$cfg_file" -s "$in_file" -t "$dag_file" )
    declare -a deps2=( -s "$dag_file" -s "$in_file" )
    for tier in ${tiers[@]}
    do
        local ff=$(frame_file $tier)
        # echo "TIER: \"$tier\" ff=\"$ff\"" 1>&3
        args1+=( -A "${tier}output=${ff}" )
        deps2+=( -t $ff )
    done        
    run_idempotently ${deps1[@]} -- compile_jsonnet ${args1[@]}
    [[ -s "$dag_file" ]]

    run_idempotently ${deps2[@]} -- wire-cell -l $log_file -L debug -c $dag_file
    [[ -s "$log_file" ]]

    for tier in ${tiers[@]}
    do
        local ff=$(frame_file $tier)
        file_larger_than "$ff" 32 
    done

    [[ -z "$(grep ' E ' $log_file)" ]]
}

# bats test_tags=history
@test "make history" {
    cd_tmp file

    [[ -s "$dag_file" ]]
    saveout -c history "$dag_file" 
    for tier in ${tiers[@]}
    do
        local ff=$(frame_file $tier)
        [[ -s "$ff" ]]
        saveout -c history "$ff" 
    done
}


function plotframe () {
    skip_if_no_input

    cd_tmp file

    local name="$1"; shift
    local tier="$1" ; shift
    local type="$1"; shift
    local dat="$1"; shift
    local fig="${name}-${tier}-${type}.png"
    local tag='*'
    declare -a args
    if [ "$tier" = "vlt" ] ; then
        args=(--unit 'millivolt' --vmin -20 --vmax 20)
    fi
    if [ "$tier" = "adc" ] ; then
        args=(--unit 'ADC' --vmin -25 --vmax 25)
        tag='orig0'
    fi
    if [ "$tier" = "nf" ] ; then
        args=(--unit 'ADC' --vmin -25 --vmax 25)
        tag='raw0'
    fi
    if [ "$tier" = "sp" ] ; then
        args=(--unit 'eplus' --vmin 0 --vmax 5000)
        tag='gauss0'
    fi

    run_idempotently -s $dat -t $fig -- \
                     wcpy plot frame ${args[@]} --tag "$tag" --single --name $type --output "$fig" "$dat" 
    [[ -s "$fig" ]]
    saveout -c plots $fig
}

@test "plot frame vlt wave new" {
    plotframe current vlt wave "$(frame_file vlt)"
}
@test "plot frame adc wave new" {
    plotframe current adc wave "$(frame_file adc)"
}
@test "plot frame nf wave new" {
    plotframe current nf wave "$(frame_file nf)"
}
@test "plot frame sp wave new" {
    plotframe current sp wave "$(frame_file sp)"
}
@test "plot frame adc wave old" {
    plotframe blessed adc wave "$(input_file frames/pdsp-signal-noise.tar.gz)"
}

function comp1d () {
    skip_if_no_input
    cd_tmp file

    plot=$1; shift
    [[ -n "$plot" ]]
    tier="adc"
    

    local new_dat="$(realpath $(frame_file $tier))"
    [[ -s "$new_dat" ]]
    local old_dat="$(input_file frames/pdsp-signal-noise.tar.gz)"
    [[ -s "$old_dat" ]]

    local fig="${plot}.png"

    run_idempotently -s $new_dat -s $old_dat -t $fig -- \
                     wcpy plot comp1d -s --chmin 0 --chmax 800 \
                     -n $plot --transform ac \
                     -o $fig $old_dat $new_dat
    [[ -s "$fig" ]]

    saveout -c plots $fig
}

@test "plot comp1d wave" {
    comp1d wave
}
@test "plot comp1d spec" {
    comp1d spec
}


@test "dotify dag viz" {
    cd_tmp file
    dotify_graph $dag_file dag.svg
}

# bats test_tags=issue:220
@test "has wiener summary" {
    cd_tmp file

    local found_wiener=$(cat wire-cell.log | grep 'OmnibusSigProc' | grep 'output frame' | grep '"wiener0":2560 \[2560\]')
    [[ -n "$found_wiener" ]]
}
# bats test_tags=issue:220
@test "no threshold summary" {
    cd_tmp file

    local found_threshold=$(cat wire-cell.log | grep 'OmnibusSigProc' | grep 'output frame' | grep 'threshold')
    echo "found threshold: $found_threshold"
    [[ -z "$found_threshold" ]]
}
# bats test_tags=issue:220
@test "no wiener_threshold_tag configuration" {
    cd_tmp file

    local found_threshold=$(cat wire-cell.log | grep 'wiener_threshold_tag')
    echo "found threshold: $found_threshold"
    [[ -z "$found_threshold" ]]
}
