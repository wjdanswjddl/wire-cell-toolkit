#!/usr/bin/env bats

# Run tests related to applying imaging to a rich uboone event.

#load ../../test/wct-bats.sh
# load "/home/bv/wrk/wct/check-test/toolkit/test/wct-bats.sh"
# set BATS_LIB_PATH externally
bats_load_library "wct-bats.sh"

base_name () {
    local fmt="${1:-json}"
    echo "test-wct-uboone-img-${fmt}"
}

run_with_fmt () {
    local fmt="${1:-json}"

    # was: uboone/celltree/celltreeOVERLAY-event6501.tar.bz2
    local infile="$(input_file frames/celltreeOVERLAY-event6501.tar.bz2)"
    [[ -s "$infile" ]]

    local cfgfile="$(relative_path test-wct-uboone-img.jsonnet)"
    [[ -s "$cfgfile" ]]


    local base=$(base_name $fmt)
    local jcfgfile="${base}-dag.json"
    local outfile="${base}-clusters.tar.gz"

    if [ -f "$outfile" ] ; then
        echo "reusing $outfile" 1>&3
        return
    fi

    compile_jsonnet "$cfgfile" "$jcfgfile" -A fmt=$fmt -A infile="$infile" -A outfile="$outfile"
    echo "$output"
    [[ "$status" -eq 0 ]]
    [[ -s "$jcfgfile" ]]

    local log="${base}.log"
    rm -f $log
    wct -L debug -l $log "$jcfgfile"
    echo "$output"
    [[ "$status" -eq 0 ]]
}

run_both_formats () {

    # was: uboone/celltree/celltreeOVERLAY-event6501.tar.bz2
    local infile="$(input_file frames/celltreeOVERLAY-event6501.tar.bz2)"
    [[ -s "$infile" ]]

    local cfgfile="$(relative_path test-wct-uboone-img.jsonnet)"
    [[ -s "$cfgfile" ]]


    local jcfgfile="$(base_name dag).json"
    local jout="$(base_name json).tar.gz"
    local nout="$(base_name numpy).tar.gz"

    if [ -f "$jout" -a -f "$nout" ] ; then
        echo "reusing wire-cell output" 1>&3
        return
    fi

    compile_jsonnet "$cfgfile" "$jcfgfile" -A infile="$infile" -A outpat="$(base_name '%s').tar.gz"
    echo "$output"
    [[ "$status" -eq 0 ]]
    [[ -s "$jcfgfile" ]]

    local log="$(base_name "log").txt"
    local err="$(base_name "err").txt"
    rm -f $log
    # wct -L debug -l $log "$jcfgfile"
    echo wire-cell -L debug -l $log "$jcfgfile" 
    wire-cell -L debug -l $log "$jcfgfile" > $err 2>&1
    echo "$output"
    [[ "$status" -eq 0 ]]
}

setup_file () {
    skip_if_no_input

    cd_tmp file

    # run_with_fmt json
    # run_with_fmt numpy    
    run_both_formats
}

@test "create graph" {
    cd_tmp file

    # only bother with default format (json)
    local base="$(base_name dag)"
    dotify_graph "${base}.json" "${base}.svg"
    saveout -c plots "${base}.svg"
}

@test "check log files" {
    cd_tmp file

    local log="$(base_name "log").txt"

    local errors="$(egrep ' W | E ' $log)"
    echo "$errors"
    [[ -z "$errors" ]]

}


@test "inspect blobs" {

    cd_tmp file

    declare -A logs

    local wcimg=$(wcb_env_value WCIMG)
    for fmt in json numpy
    do
        local base="$(base_name $fmt)"

        local log="${base}.inspect"
        logs[$fmt]="$log"

        local dat="${base}.tar.gz"
        echo "$wcimg inspect --verbose -o $log $dat"
        run $wcimg inspect --verbose -o "$log" "$dat"
        echo "$output"
        [[ "$status" -eq 0 ]]
        [[ -s "$log" ]]
    done

    local delta="$(diff -u ${logs[*]})"
    [[ -z "$delta" ]]

}


@test "dump blobs" {

    cd_tmp file

    local wcimg=$(wcb_env_value WCIMG)
    for fmt in json numpy
    do
        local base="$(base_name $fmt)"
        echo $wcimg dump-blobs -o ${base}.dump  ${base}.tar.gz
        run $wcimg dump-blobs -o ${base}.dump  ${base}.tar.gz
        echo "$output"
        [[ "$status" -eq 0 ]]
        [[ -s "${base}.dump" ]] 
    done

    run diff -u $(base_name json).dump $(base_name numpy).dump
    echo "$output"
    [[ "$status" -eq 0 ]]
    [[ -z "$delta" ]]
}
    
@test "at least one multi-blob measure" {
    local fname="$(base_name numpy).inspect"
    echo $fname

    local got=$(grep 'nn for m' "${fname}" | grep -v 'b=1\b')
    echo "$got"
    [[ -n "$got" ]]
}
