#!/usr/bin/env bats

# Run signal and noise simulation on PDSP APA0.

# Produces plots and history files
# bats file_tags=plots

# Started for https://github.com/WireCell/wire-cell-toolkit/pull/195

bats_load_library wct-bats.sh

log_file="wire-cell.log"
sig_file="frames-sig.tar.gz"
adc_file="frames-adc.tar.gz"
dag_file="dag.json"

# Compile Jsonnet to dag.json and run wire-cell.
setup_file () {

    skip_if_no_input

    cd_tmp file

    local cfg_file="$(relative_path pdsp-depos-simsn-frames.jsonnet)"
    echo "using config: $cfg_file"
    [[ -n "$cfg_file" ]]
    [[ -f "$cfg_file" ]]

    # Assure input file
    # was pdsp/sim/sn/depos.tar.bz
    local in_file="$(input_file depos/many.tar.bz2)"
    echo "in file: $in_file"
    [[ -f "$in_file" ]]

    run_idempotently -s $cfg_file -t $dag_file -- \
                     compile_jsonnet $cfg_file $dag_file \
                     -A input="$in_file" -A sigoutput="$sig_file" \
                     -A adcoutput="$adc_file"
    [[ -s "$dag_file" ]]

    run_idempotently -s $dag_file -s $in_file -t $sig_file -t $adc_file -t $log_file -- \
                     wct -l $log_file -L debug -c $dag_file
    [[ -s "$log_file" ]]

    file_larger_than "$sig_file" 32  # assure non-empty .tar.gz
    file_larger_than "$adc_file" 32  # assure non-empty .tar.gz

    [[ -z "$(grep ' E ' $log_file)" ]]
}

# bats test_tags=history
@test "make history" {
    cd_tmp file

    [[ -s "$dag_file" ]]
    [[ -s "$sig_file" ]]
    [[ -s "$adc_file" ]]
    saveout -c history "$dag_file" 
    saveout -c history "$sig_file" 
    saveout -c history "$adc_file" 
}


function plotframe () {
    skip_if_no_input

    cd_tmp file

    local name="$1"; shift
    local tier="$1" ; shift
    local type="$1"; shift
    local dat="$1"; shift
    local fig="${name}-${tier}-${type}.png"
    local tag='orig'
    declare -a args
    if [ "$tier" = "sig" ] ; then
        args=(-u 'millivolt' -r 10)
        local tag='*'
    fi

    local wcplot=$(wcb_env_value WCPLOT)
    [[ -n "$wcplot" ]]
    [[ -x "$wcplot" ]]

    run_idempotently -s $dat -t $fig -- \
                     $wcplot frame ${args[@]} -t "$tag" -s -n $type "$dat" "$fig"
    [[ -s "$fig" ]]
    saveout -c plots $fig
}

@test "plot frame adc wave new" {
    plotframe current adc wave "$adc_file"
}
@test "plot frame sig wave new" {
    plotframe current sig wave "$sig_file"
}
@test "plot frame adc wave old" {
    plotframe blessed adc wave "$(input_file frames/pdsp-signal-noise.tar.bz2)"
}

function comp1d () {
    skip_if_no_input
    cd_tmp file

    plot=$1
    [[ -n "$plot" ]]
    
    local wcplot=$(wcb_env_value WCPLOT)
    local new_dat="$adc_file"
    [[ -s "$new_dat" ]]
    local old_dat="$(input_file frames/pdsp-signal-noise.tar.bz2)"
    [[ -s "$old_dat" ]]

    local fig="${plot}.png"

    run_idempotently -s $new_dat -s $old_dat -t $fig -- \
                     $wcplot comp1d -s --chmin 0 --chmax 800 \
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


@test "dag viz" {
    cd_tmp file
    dotify_graph $dag_file dag.svg
}
