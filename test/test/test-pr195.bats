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


@test "check pdsp sim" {

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

    # infile="$(realpath depos.tar.bz2)"
    # if [ -f "$infile" ] ; then
    #     echo "Reusing prior depos file"
    # else
    #     echo "Downloading depos file.  Fixme: centralize this."
    #     wget --quiet -O $infile 'https://www.phy.bnl.gov/~hyu/wct-ci/gen/depos.tar.bz2'
    # fi
    # [[ -f "$infile" ]]

    cd_tmp

    logfile="frames-pr195.log"
    outfile="frames-pr195.tar.gz"
    if [ -f "$logfile" -a -f "$outfile" ] ; then
        warn "Reusing existing output of wire-cell on $cfgfile"
    else
        echo "Running wire-cell on $cfgfile"
        run wire-cell -l "$logfile" -L debug \
            -A input="$infile" \
            -A output="$outfile" \
            -c "$cfgfile"
        echo "$output"
        ls -l
        [[ "$status" -eq 0 ]]
    fi
    [[ -f "$logfile" ]]
    [[ -s "$logfile" ]]
    [[ -f "$outfile" ]]
    [[ -s "$outfile" ]]
    

    # make PDF
    pdffile="frames-pr195.pdf"
    if [ -f "$pdffile" ] ; then
        warn "Reusing existing $pdffile"
    else
        run wirecell-plot frame -n wave "$outfile" "$pdffile"
        echo "$output"
        [[ "$status" -eq 0 ]]
    fi
    saveout "$pdffile"

    # oldfile="$(download_file https://www.phy.bnl.gov/~hyu/wct-ci/gen/pr186-ref/frames-pr186.tar.bz2)"
    oldfile="$(resolve_file pdsp/sim/sn/frames-expected.tar.bz2)"
    [[ -f "$oldfile" ]] 
    [[ -s "$oldfile" ]] 


    cmpfile="frames-pr186-pr195.pdf"
    if [ -f "$cmpfile" ] ; then
        warn "Reusing existing $cmpfile"
    else
        run wirecell-plot wave-comp -c 700 $oldfile $outfile $cmpfile
        echo "$output"
        [[ "$status" -eq 0 ]]
    fi
    [[ -f "$cmpfile" ]]
    [[ -s "$cmpfile" ]]
    saveout "$cmpfile"

}
