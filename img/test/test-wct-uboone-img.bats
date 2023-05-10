#!/usr/bin/env bats

# Run tests related to applying imaging to a rich uboone event.

bats_load_library "wct-bats.sh"

dag_file="dag.json"
log_file="wire-cell.log"
img_numpy_file="clusters-numpy.tar.gz"
img_json_file="clusters-json.tar.gz"

setup_file () {
    skip_if_no_input

    cd_tmp file

    # was: uboone/celltree/celltreeOVERLAY-event6501.tar.bz2
    local infile="$(input_file frames/celltreeOVERLAY-event6501.tar.bz2)"
    [[ -s "$infile" ]]

    local cfg_file="$(relative_path test-wct-uboone-img.jsonnet)"
    [[ -s "$cfg_file" ]]

    run_idempotently -s "$cfg_file" -t "$dag_file" -- \
                     compile_jsonnet "$cfg_file" "$dag_file" \
                     -A infile="$infile" -A outpat="clusters-%s.tar.gz" -A formats="json,numpy"
    [[ -s "$dag_file" ]]

    run_idempotently -s "$dag_file" -s "$infile" \
                     -t "$img_numpy_file" -t "$img_json_file" -t "$log_file" -- \
                     wct -l "$log_file" -L debug -c "$dag_file"
    echo $output 1>&3
    [[ -s "$log_file" ]]
}


# bats test_tags=history
@test "make history" {
    cd_tmp file

    [[ -s "$img_numpy_file" ]]
    [[ -s "$img_json_file" ]]
    saveout -c history "$img_numpy_file"
    saveout -c history "$img_json_file"
}

# bats test_tags=dotify
@test "dotify dag" {
    cd_tmp file

    dotify_graph "$dag_file" "dag.svg"
    saveout -c plots "dag.svg"
}

@test "check wire-cell log file" {
    cd_tmp file

    local errors="$(egrep ' W | E ' $log_file)"
    echo "$errors"
    [[ -z "$errors" ]]
}

function do_blobs () {
    local what="$1"; shift
    local args=( $@ )

    cd_tmp file

    declare -A logs

    local wcimg=$(wcb_env_value WCIMG)
    for fmt in json numpy
    do
        local log="${what}-${fmt}.log"
        logs[$fmt]="$log"

        local dat="clusters-${fmt}.tar.gz"
        echo "$wcimg $what ${args[@]} -o $log $dat" 
        run $wcimg $what "${args[@]}" -o "$log" "$dat"
        echo "$output"
        echo "$status"
        [[ "$status" -eq 0 ]]
        [[ -s "$log" ]]
    done

    local delta="$(diff -u ${logs[*]})"
    echo "$delta"
    [[ -z "$delta" ]]
}

@test "inspect blobs quietly" {
    do_blobs inspect
}
@test "inspect blobs verbosely" {
    do_blobs inspect --verbose
}
@test "dump blobs" {
    do_blobs dump-blobs
}

    
@test "at least one multi-blob measure" {
    cd_tmp file

    local got=$(grep 'nn for m' inspect-numpy.log | grep -v 'b=1\b')
    echo "$got"
    [[ -n "$got" ]]
}

# bats test_tags=plots
@test "plot blobs" {
    cd_tmp file

    local wcimg=$(wcb_env_value WCIMG)
    run $wcimg plot-blobs --single --plot views clusters-numpy.tar.gz blob-views.png

}

@test "valid cluster graph schema" {
    cd_tmp file

    local moo="$(which moo)"
    if [ -z "$moo" ] ; then
        skip "No moo available to check schema (see https://brettviren.github.io/moo/moo.html#install)"
    fi

    local sfile=$(relative_path ../../aux/docs/cluster-graph-schema.jsonnet)
    [[ -s "$sfile" ]]

    [[ -s clusters-json.tar.gz ]]
    tar -xf clusters-json.tar.gz

    local dfile="cluster_6501_graph.json"
    [[ -s "$dfile" ]]

    run $moo validate -t wirecell.cluster.Cluster -s "$sfile" "$dfile"
    echo "$output"
    [[ "$status" -eq 0 ]]
}

function roundtrip () {
    local ifmt=$1; shift
    local ofmt=$1; shift
    local ifile=clusters-${ifmt}.tar.gz
    local ofile=clusters-${ifmt}-${ofmt}.tar.gz
    local lfile=roundtrip-${ifmt}-${ofmt}.log
    local cfg=$(relative_path test-wct-uboone-img-roundtrip.jsonnet)

    cd_tmp file

    wct -l $lfile -L debug \
        -A infile=$ifile \
        -A outfile=$ofile \
        -A infmt=$ifmt \
        -A outfmt=$ofmt \
        -c $cfg

    local errors="$(egrep ' W | E ' $lfile)"
    echo "$errors"
    [[ -z "$errors" ]]
    
    local wcimg=$(wcb_env_value WCIMG)

    ilog=roundtrip-inspect-${ifmt}2${ofmt}-ilog-${ifmt}.log
    olog=roundtrip-inspect-${ifmt}2${ofmt}-olog-${ofmt}.log
    run $wcimg inspect --verbose -o $ilog $ifile
    run $wcimg inspect --verbose -o $olog $ofile

    local delta="$(diff -u $ilog $olog)"
    echo "$delta"
    [[ -z "$delta" ]]
}

@test "round trip json to json" {
    roundtrip json json
}
@test "round trip numpy to numpy" {
    roundtrip numpy numpy
}
@test "round trip numpy to json" {
    roundtrip numpy json
}
@test "round trip json to numpy" {
    roundtrip json numpy
}
