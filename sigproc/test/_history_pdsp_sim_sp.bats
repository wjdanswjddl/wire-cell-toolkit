#!/usr/bin/env bats

# This test consumes historical files produced by check_pdsp_sim_sp.bats

bats_load_library wct-bats.sh

# bats test_tags=time:1

@test "historical pdsp_sim_sp comp1d plots" {


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
        check wcpy plot \
            comp1d -n $plot --markers 'o + x .' -t 'orig' \
            --chmin 700 --chmax 701 -s \
            -o $plotfile ${frame_files[*]}
        saveout -c reports $plotfile

        local plotfile="$outdir/comp1d-${plot}-history-zoom1.png"
        check wcpy plot \
            comp1d -n $plot --markers 'o + x .' -t 'orig' \
            --chmin 700 --chmax 701 -s \
            --xrange 2000 3200 \
            -o $plotfile ${frame_files[*]}
        saveout -c reports $plotfile

        local plotfile="$outdir/comp1d-${plot}-history-zoom2.png"
        check wcpy plot \
            comp1d -n $plot --markers 'o + x .' -t 'orig' \
            --chmin 700 --chmax 701 -s \
            --xrange 4400 5200 \
            -o $plotfile ${frame_files[*]}
        saveout -c reports $plotfile
    done


}

