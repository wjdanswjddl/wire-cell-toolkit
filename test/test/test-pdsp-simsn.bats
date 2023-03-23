#!/usr/bin/env bats

# Some tests for https://github.com/WireCell/wire-cell-toolkit/pull/195

load ../wct-bats.sh

# FIXME: this test provides use cases for the test system
#
# - [x] A systematic way to run a bats test in a pre-created directory
#   under build/tests/<test-name>/ to keep build tidy.  --> cd_tmp
#   
# 
# - [x] A "test data repository" with files for input and expected
#   output. --> ./wcb --test-data=/...  and wct-bats.sh helpers
#
# - This test implements the commands from the README.md in above URL.  It produces PDFs which clearly show a problem, but require a human.  In addition to the PDFs, this test should compare data at the "frames" file level.  This likely needs a "wirecell-util diff-array" command to be written.
#
# - This test is laboriously written to be idempotent.  Perhaps the common patterns can be abstracted.  
#
# - A common patter to name and extract "human interesting" products of the test, such as the PDFs.


function waveform_figure () {
    dat="$1" ; shift
    [[ -n "$dat" ]]
    [[ -f "$dat" ]]
    fig="$1"; shift
    [[ -n "$fig" ]]

    if [ -f "$fig" ] ; then
        warn "Reusing existing $fig"
    else
        run wirecell-plot frame -n wave "$dat" "$fig"
        echo "$output"
        [[ "$status" -eq 0 ]]
        [[ -s "$fig" ]]
        archive_append "$fig"
    fi
}

    
function channel_figure () {
    local chan="$1"; shift
    local old_dat="$1"; shift
    local new_dat="$1"; shift
    local fig="$1"; shift

    if [ -f "$fig" ] ; then
        warn "Reusing existing $fig"
    else
        run wirecell-plot wave-comp -c $chan $old_dat $new_dat $fig
        echo "$output"
        [[ "$status" -eq 0 ]]
        [[ -s "$fig" ]]
        archive_append "$fig"
    fi
}

@test "check pdsp signal" {

    skip_if_no_test_data

    # Assume we run from build/.
    cfgfile="$(relative_path pdsp-depos-simsn-frames.jsonnet)"
    echo "using config: $cfgfile"
    [[ -n "$cfgfile" ]]
    [[ -f "$cfgfile" ]]

    # Assure input file
    infile="$(test_data_file pdsp/sim/sn/depos.tar.bz2)"
    echo "infile: $infile"
    [[ -f "$infile" ]]

    cd_tmp

    logfile="${BATS_TEST_NAME}.log"
    outfile="${BATS_TEST_NAME}_frames.tar.gz"
    if [ -f "$logfile" -a -f "$outfile" ] ; then
        warn "Reusing existing output of wire-cell on $cfgfile"
    else
        echo "Running wire-cell on $cfgfile"
        wct -l "$logfile" -L debug \
            -A input="$infile" \
            -A output="$outfile" \
            -c "$cfgfile"
    fi
    [[ -f "$logfile" ]]
    [[ -s "$logfile" ]]
    [[ -f "$outfile" ]]
    [[ -s "$outfile" ]]
    

    # make figures
    local fmt="pdf"

    # make waveform figures for new data and old ("blessed") data.
    local new_dat="$outfile"
    local new_fig="${BATS_TEST_NAME}_frame_new.$fmt"
    waveform_figure $new_dat $new_fig

    local old_dat="$(test_data_file pdsp/sim/sn/frames-expected.tar.bz2)"
    [[ -n "$old_dat" ]]
    local old_fig="${BATS_TEST_NAME}_frame_old.$fmt"
    waveform_figure $old_dat $old_fig

    for chan in 700 701 702
    do
        fig="${BATS_TEST_NAME}_channel_$chan.$fmt"
        channel_figure $chan $old_dat $new_dat $fig
    done
    
    archive_saveout
}

## waiting on https://www.phy.bnl.gov/~yuhw/wct-ci/gen/check_pdsp_noise.jsonnet
# @test "check pdsp nosie" {
# }
