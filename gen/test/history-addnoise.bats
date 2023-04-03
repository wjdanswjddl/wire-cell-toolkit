#!/usr/bin/env bats

# This test consumes historical files produced by test-addnoise.bats
#
# See eg bv-generate-history-haiku for example how to automate
# producing historical files.  O.w. collect one or more history/
# directories and add their parent locations to WCTEST_DATA_PATH.

load ../../test/wct-bats.sh

# bats test_tags=time:1

@test "historical addnoise comp1d plots" {

    local wcplot=$(wcb_env_value WCPLOT)
    
    # will cd here to make plots have minimal filename labels 
    local rundir=$(blddir)/tests/history
    # but will deposite plot files to our temp dir
    local outdir=$(tmpdir)


    local inpath="test-addnoise/test-addnoise-empno-6000.tar.gz"
    local frame_files=( $(historical_files -v $(version) -l 2 $inpath) )
    # yell "frame files: ${frame_files[@]}"
    frame_files=( $(realpath --relative-to=$rundir ${frame_files[*]}) )

    cd $rundir

    for plot in wave spec
    do
        local plotfile="$outdir/comp1d-${plot}-history.png"
        $wcplot \
            comp1d -n $plot --markers 'o + x .' -t '*' \
            --chmin 0 --chmax 800 -s --transform ac \
            -o $plotfile ${frame_files[*]}
        saveout -c reports $plotfile

    done

    local plotfile="$outdir/comp1d-spec-history-zoom1.png"
    $wcplot \
        comp1d -n spec --markers 'o + x .' -t '*' \
        --chmin 0 --chmax 800 -s --transform ac \
        --xrange 100 800 \
        -o $plotfile ${frame_files[*]}
    saveout -c reports $plotfile

    local plotfile="$outdir/comp1d-spec-history-zoom2.png"
    $wcplot \
        comp1d -n spec --markers 'o + x .' -t '*' \
        --chmin 0 --chmax 800 -s --transform ac \
        --xrange 1000 2000 \
        -o $plotfile ${frame_files[*]}
    saveout -c reports $plotfile


}

