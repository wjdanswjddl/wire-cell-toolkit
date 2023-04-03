#!/usr/bin/env bats

# fixme: add asserts
# fixme: use history mechanism

# bats file_tags=implicit,plots

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

base="$(basename ${BATS_TEST_FILENAME} .bats)"

setup_file () {

    skip_if_no_input

    cd_tmp file

    # Assume we run from build/.
    cfgfile="$(relative_path pdsp-depos-simsn-frames.jsonnet)"
    echo "using config: $cfgfile"
    [[ -n "$cfgfile" ]]
    [[ -f "$cfgfile" ]]

    # Assure input file
    # was pdsp/sim/sn/depos.tar.bz
    infile="$(input_file depos/many.tar.bz2)"
    echo "infile: $infile"
    [[ -f "$infile" ]]

    cd_tmp

    logfile="${base}.log"
    outfile="${base}_frames.tar.gz"
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
    saveout -c history "$outfile"
}



function waveform_figure () {
    local dat="$1" ; shift
    [[ -n "$dat" ]]
    [[ -f "$dat" ]]
    local fig="$1"; shift
    [[ -n "$fig" ]]

    if [ -f "$fig" ] ; then
        warn "Reusing existing $fig"
        return
    fi

    local wcplot=$(wcb_env_value WCPLOT)

    # yell "$wcplot frame -s -n wave $dat $fig"
    run $wcplot frame -s -n wave "$dat" "$fig"
    pwd
    echo "$output"
    echo "status: $status"
    echo "status okay"
    [[ "$status" -eq 0 ]]
    echo "no fig: $fig"
    ls -l *.png
    [[ -s "$fig" ]]

    saveout -c plots "$fig"
}

    
function channel_figure () {
    local chan="$1"; shift
    local old_dat="$1"; shift
    local new_dat="$1"; shift
    local fig="$1"; shift

    if [ -f "$fig" ] ; then
        warn "Reusing existing $fig"
    fi
    local wcplot=$(wcb_env_value WCPLOT)
    # yell "$wcplot comp1d -c $chan $old_dat $new_dat $fig"

    run $wcplot comp1d -s -c $chan $old_dat $new_dat $fig
    echo "$output"
    [[ "$status" -eq 0 ]]
    [[ -s "$fig" ]]
    saveout -c plots "$fig"
}


@test "plot pdsp signal" {

    skip_if_no_input

    cd_tmp file

    outfile="${base}_frames.tar.gz"

    # make figures
    local fmt="png"

    # make waveform figures for new data and old ("blessed") data.
    local new_dat="$outfile"
    local new_fig="${base}_frame_new.$fmt"
    waveform_figure $new_dat $new_fig

    # was pdsp/sim/sn/frames-expected.tar.bz2
    local old_dat="$(input_file frames/pdsp-signal-noise.tar.bz2)"
    [[ -n "$old_dat" ]]
    local old_fig="${base}_frame_old.$fmt"
    waveform_figure $old_dat $old_fig

    local wcplot=$(wcb_env_value WCPLOT)
    for plot in wave spec
    do
        local fig="check-pdsp-signal-${plot}.png"
        $wcplot comp1d -s --chmin 0 --chmax 800 \
                -n $plot --transform ac \
                -o $fig -s $old_dat $new_dat
        [[ -s $fig ]]
        saveout -c plots $fig
    done
}

