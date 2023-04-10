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

setup_file () {
    skip_if_no_input

    cd_tmp file

    run_with_fmt json
    run_with_fmt numpy    
}

@test "create graph" {
    cd_tmp file

    # only bother with default format (json)
    local base=$(base_name)
    local jcfgfile="${base}-dag.json"
    local svgfile="${base}-dag.svg"
    dotify_graph "$jcfgfile" "$svgfile"
    saveout -c plots "$svgfile"
}

@test "check log files" {
    cd_tmp file

    for fmt in json numpy
    do
        local base="$(base_name $fmt)"
        local log="${base}.log"

        local errors="$(egrep ' W | E ' $log)"
        echo "$errors"
        [[ -z "$errors" ]]

    done
}

@test "dump blobs" {
    cd_tmp file

    local wcimg=$(wcb_env_value WCIMG)
    for fmt in json numpy
    do
        local base="$(base_name $fmt)"
        echo $wcimg dump-blobs -o ${base}-clusters.dump  ${base}-clusters.tar.gz
        run $wcimg dump-blobs -o ${base}-clusters.dump  ${base}-clusters.tar.gz
        echo "wirecell-img is $wcimg"
        echo "$output"
        [[ "$status" -eq 0 ]]
    done

    local delta=$(diff -u $(base_name json)-clusters.dump $(base_name numpy)-clusters.dump)
    echo $delta
    [[ -z "$delta" ]]
}
    
