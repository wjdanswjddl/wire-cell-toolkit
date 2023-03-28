
file_base () {
    local noise="$1" ; shift
    local nsamples="${1:-6000}"

    local ver="$(wire-cell --version)"
    local base="$(basename ${BATS_TEST_FILENAME} .bats)"
    echo "test-addnoise-${ver}-${noise}-${nsamples}"
}    

make_noise () {
    local noise="$1" ; shift
    local nsamples="${1:-6000}"

    local base="$(file_base $noise $nsamples)"

    local cfgfile="${BATS_TEST_FILENAME%.bats}.jsonnet"

    local outfile="${base}.tar.gz"
    local logfile="${base}.log"

    rm -f "$logfile"            # appends otherwise
    if [ -s "$outfile" ] ; then
        echo "already have $outfile" 1>&3
        return
    fi
    run wire-cell -l "$logfile" -L debug \
        -A nsamples=$nsamples -A noise=$noise -A outfile="$outfile" \
        -c "$cfgfile"
    echo "$output"
    [[ "$status" -eq 0 ]]
    [[ -s "$outfile" ]]
}


# The intention is to run this test in multiple releases and compare across releases.
@test "generate simple noise for comparison with older releases" {


    
    for nsamples in 6000
    do
        for noise in empno 
        do
            make_noise $noise $nsamples
            local base="$(file_base $noise $nsamples)"

            wirecell-plot comp1d -o comp1d-addnoise-${ver}-${noise}-${nsamples}.pdf \
                          --markers 'o + x .' -t '*' -n spec \
                          --chmin 0 --chmax 800 -s --transform ac \
                          "${base}.tar.gz"
        done
    done
    # wirecell-plot comp1d -o comp1d-addnoise-${ver}-all-all.pdf \
    #               --markers 'o + x .' -t '*' -n spec \
    #               --chmin 0 --chmax 800 -s --transform ac \
    #               "test-addnoise-${ver}-empno-4096.tar.gz" \
    #               "test-addnoise-${ver}-inco-4096.tar.gz" \
    #               "test-addnoise-${ver}-empno-6000.tar.gz" \
    #               "test-addnoise-${ver}-inco-6000.tar.gz"

}



@test "inco and empno with external spectra file is identical" {

    for nsamples in 4095 4096 6000
    do
        for noise in empno inco
        do
            make_noise $noise $nsamples
        done
        
        wirecell-plot comp1d \
                      -o "$(file_base inco+empno $nsamples)-comp1d.pdf" \
                      --markers 'o .' -t '*' -n spec \
                      --chmin 0 --chmax 800 -s --transform ac \
                      "$(file_base inco $nsamples).tar.gz" \
                      "$(file_base empno $nsamples).tar.gz"
    
    done
    
}

