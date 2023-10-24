#!/usr/bin/env bats

# A test to run on current, past and future releases to check for any
# unexepcted change in noise simulation.
#
# The test is meant to be self locating so that one can do:
#
# cd ~/path/to/new-wct/
# alias bats-$(realpath test/bats/bin/bats)
# cd ~/path/to/old/release
# bats ~/path/to/new-wct/gen/test/test-addnoise.bats

# bats file_tags=noise

bats_load_library wct-bats.sh

log_file="wire-cell.log"
adc_file="frames-adc.tar.gz"
dag_file="dag.json"

setup_file () {
    cd_tmp

    local nsamples=6000
    local noise=empno
    local cfg_file="${BATS_TEST_FILENAME%.bats}.jsonnet"

    run_idempotently -s $cfg_file -t $dag_file -- \
                     compile_jsonnet $cfg_file $dag_file \
                     -A nsamples=$nsamples -A noise=$noise -A outfile="$adc_file"

    run_idempotently -s $dag_file -t $adc_file -t $log_file -- \
                     wire-cell -l $log_file -L debug -c $dag_file


    # run wire-cell -l "$logfile" -L debug \
    #     -A nsamples=$nsamples -A noise=$noise -A outfile="$adc_file" \
    #     -c "$cfgfile"
}

# bats test_tags=history
@test "make history" {
    cd_tmp file
    [[ -s "$dag_file" ]]
    [[ -s "$adc_file" ]]
    saveout -c history "$dag_file"
    saveout -c history "$adc_file"
}

# The intention is to run this test in multiple releases and compare across releases.
# bats test_tags=plots,implicit
@test "generate simple noise for comparison with older releases" {

    cd_tmp file

    for what in spec wave
    do
        local pout="comp1d-${what}.png"
        check wcpy plot comp1d \
                -o $pout \
                -t '*' -n $what \
                --chmin 0 --chmax 800 -s --transform ac \
                "${adc_file}"
        [[ -s "$pout" ]]
        saveout -c plots "$pout"
    done
}

