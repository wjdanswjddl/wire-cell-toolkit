#!/usr/bin/env bats

# Run tests related to applying imaging to a rich uboone event.

bats_load_library wct-bats.sh

setup_file () {
    skip_if_no_input

    cd_tmp file

    # was: uboone/celltree/celltreeOVERLAY-event6501.tar.bz2
    local infile="$(input_file frames/celltreeOVERLAY-event6501.tar.bz2)"
    [[ -s "$infile" ]]

    local cfgfile="$(relative_path test-wct-uboone-img.jsonnet)"
    [[ -s "$cfgfile" ]]
    local jsonfile="test-wct-uboone-img.json"

    # explicit compilation to JSON to make subsequent tests faster
    if [ -f "$jsonfile" ] ; then
        echo "resusing $jsonfile"
    else
        compile_jsonnet "$cfgfile" "$jsonfile" -A infile="$infile" -A outfile="$outfile"
    fi
    
    local outfile="test-wct-uboone-img.tar.gz"
    if [ -f "$outfile" ] ; then
        echo "reusing $outfile"
    else
        wct "$jsonfile"
    fi
}

@test "create graph" {
    cd_tmp file

    local jsonfile="test-wct-uboone-img.json"
    [[ -f "$jsonfile" ]]
    local svgfile="test-wct-uboone-img-graph.svg"
    dotify_graph "$jsonfile" "$svgfile"
    saveout -c plots "$svgfile"
}
