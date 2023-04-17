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

# bats file_tags=noise,history

bats_load_library wct-bats.sh

# The intention is to run this test in multiple releases and compare across releases.
# bats test_tags=history,plots,implicit
@test "generate simple noise for comparison with older releases" {

    cd_tmp

    local nsamples=6000
    local noise=empno
    local name="test-addnoise-${noise}-${nsamples}"
    local adcfile="${name}.tar.gz" # format with support going back the longest
    local cfgfile="${BATS_TEST_FILENAME%.bats}.jsonnet"

    run wire-cell -l "$logfile" -L debug \
        -A nsamples=$nsamples -A noise=$noise -A outfile="$adcfile" \
        -c "$cfgfile"
    echo "$output"
    [[ "$status" -eq 0 ]]
    [[ -s "$adcfile" ]]
    saveout -c history "$adcfile"

    local wcplot=$(wcb_env_value WCPLOT)
    for what in spec wave
    do
        local pout="${name}-comp1d-${what}.png"
        $wcplot comp1d \
                -o $pout \
                -t '*' -n $what \
                --chmin 0 --chmax 800 -s --transform ac \
                "${adcfile}"
        echo "$output"
        [[ "$status" -eq 0 ]]
        [[ -s "$pout" ]]
        saveout -c plots "$pout"
    done
}

