#!/usr/bin/env bats

# fixme: this is first developed outside of check-test branch.  It
# should be refactored to use that Bats support, once merged.
# It does assume a new bats such as the one provided by check-test.

#   coherent/incoherent
#   vin = voltage level
#   adc = ADC counts
#   dac = ADC scaled to voltage
#   spectra output of NoiseModeler
# test-noise-roundtrip-inco-vin.npz
# test-noise-roundtrip-inco-dac.npz
# test-noise-roundtrip-inco-adc.npz
# test-noise-roundtrip-inco-spectra.json.bz2
# test-noise-roundtrip-cohe-vin.npz
# test-noise-roundtrip-cohe-dac.npz
# test-noise-roundtrip-cohe-adc.npz
# test-noise-roundtrip-cohe-spectra.json.bz2

setup_file () {
    # cd_tmp

    local cfgfile="${BATS_TEST_FILENAME%".bats"}.jsonnet"
    run wire-cell -l log -L debug $cfgfile
    echo "$output"
    [[ "$status" -eq 0 ]]
    
}

@test "plot adc" {
    # cd_tmp file
    local mydir="$(dirname ${BATS_TEST_FILENAME})"
    python $mydir/check_noise_roundtrip.py plot -o inco-adc.png  test-noise-roundtrip-inco-adc.npz
    python $mydir/check_noise_roundtrip.py plot -o cohe-adc.png  test-noise-roundtrip-cohe-adc.npz
    # archive these
}

@test "no weird endpoints of mean waveform" {
    local mydir="$(dirname ${BATS_TEST_FILENAME})"
    declare -a line

    ## good lines are:
    # U t 4096 0.02282684326171875 0.06892286163071752 4096 1291 185 13 2 0 0 0 0 0 0
    # U c 800 0.02282684326171875 0.4955734777788893 800 376 0 0 0 0 0 0 0 0 0
    # V t 4096 0.01374267578125 0.03856538100586203 4096 1334 192 13 1 0 0 0 0 0 0
    # V c 800 0.01374267578125 0.49734063258678657 800 386 0 0 0 0 0 0 0 0 0
    # W t 4096 0.009838104248046875 0.016252509189760622 4096 1328 177 14 0 0 0 0 0 0 0
    # W c 960 0.009838104248046875 0.49116500530978957 960 463 0 0 0 0 0 0 0 0 0
    ## bad lines are like:
    # U t 4096 0.23914672851562502 1.838315680187899 4096 30 22 15 15 14 13 13 13 11 10
    # U c 800 0.239146728515625 0.4333089727023359 800 201 0 0 0 0 0 0 0 0 0
    # V t 4096 0.24620086669921876 1.0176595165745093 4096 30 22 15 15 14 13 13 13 11 10
    # V c 800 0.24620086669921876 0.43083546962103825 800 198 0 0 0 0 0 0 0 0 0
    # W t 4096 0.23283335367838542 0.20440386815418213 4096 39 26 15 14 14 14 13 11 10 10
    # W c 960 0.23283335367838542 0.4332668683930272 960 249 0 0 0 0 0 0 0 0 0    

    while read line ; do
        parts=($line)

        echo "$line"
        if [ "${parts[1]}" = "c" ] ; then
            continue
        fi
        # require nothing past 5 sigma
        [[ "${parts[10]}" = "0" ]]
        
        # mean should be small.  dumb subst for floating point check.
        [[ -n "$(echo ${parts[3]} | grep '^0\.0')" ]]

        # rms should be small.  dumb subst for floating point check.
        [[ -n "$(echo ${parts[4]} | grep '^0\.0')" ]]
        
    done < <(python $mydir/check_noise_roundtrip.py stats test-noise-roundtrip-inco-adc.npz)
    
    
}
