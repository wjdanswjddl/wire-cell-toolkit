#!/usr/bin/env bats

# This test consumes historical files produced by test-addnoise.bats
#
# See eg bv-generate-history-haiku for example how to automate
# producing historical files.  O.w. collect one or more history/
# directories and add their parent locations to WCTEST_DATA_PATH.

bats_load_library wct-bats.sh

# bats test_tags=time:1

@test "historical pdsp_sim_sp comp1d plots" {

    local wcplot=$(wcb_env_value WCPLOT)
    
    # will cd here to make plots have minimal filename labels 
    local rundir=$(blddir)/tests/history
    # but will deposite plot files to our temp dir
    local outdir=$(tmpdir)


    local inpath="check_pdsp_sim_sp/check_pdsp_sim_sp.tar.bz2"
    local frame_files=( $(historical_files -v $(version) -l 2 $inpath) )
    yell "frame files: ${frame_files[@]}"
    frame_files=( $(realpath --relative-to=$rundir ${frame_files[*]}) )

    cd $rundir

    for plot in wave
    do
        local plotfile="$outdir/comp1d-${plot}-history.png"
        $wcplot \
            comp1d -n $plot --markers 'o + x .' -t 'orig' \
            --chmin 700 --chmax 701 -s \
            -o $plotfile ${frame_files[*]}
        saveout -c reports $plotfile

        local plotfile="$outdir/comp1d-${plot}-history-zoom1.png"
        $wcplot \
            comp1d -n $plot --markers 'o + x .' -t 'orig' \
            --chmin 700 --chmax 701 -s \
            --xrange 2000 3200 \
            -o $plotfile ${frame_files[*]}
        saveout -c reports $plotfile

        local plotfile="$outdir/comp1d-${plot}-history-zoom2.png"
        $wcplot \
            comp1d -n $plot --markers 'o + x .' -t 'orig' \
            --chmin 700 --chmax 701 -s \
            --xrange 4400 5200 \
            -o $plotfile ${frame_files[*]}
        saveout -c reports $plotfile
    done


}

