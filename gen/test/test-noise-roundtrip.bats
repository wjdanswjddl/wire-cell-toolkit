#!/usr/bin/env bats

load ../../test/wct-bats.sh

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
    cd_tmp

    local tname="$(basename $BATS_TEST_FILENAME .bats)"
    local cfgfile="$(relative_path ${tname}.jsonnet)"
    if [ -f log ] ; then
        echo "reusing output of $cfgfile"
    else
        wct -l log -L debug $cfgfile
        echo "$output"
        [[ "$status" -eq 0 ]]
    fi
    [[ -s "log" ]]
}

@test "plot adc" {
    cd_tmp file                 # access file level temp context

    local tname="$(basename $BATS_TEST_FILENAME .bats)"
    local tier="adc"
    local plotter=$(wcb_env_value WCPLOT)

    for label in inco cohe
    do
        local base="${tname}-${label}-${tier}"
        local fig="${base}.png"
        run $plotter frame-means -o "$fig" "${base}.npz"
        echo "$output"
        [[ "$status" -eq 0 ]]
        [[ -s "$fig" ]]
        archive_append "$fig"
    done
}

@test "plot spectra" {
    cd_tmp file                 # access file level temp context
    local tname="$(basename $BATS_TEST_FILENAME .bats)"
    local plotter=$(wcb_env_value WCSIGPROC)
    local tier="spectra"

    for label in inco cohe
    do
        local base="${tname}-${label}-${tier}"
        local fig="${base}.pdf"
        run $plotter plot-noise-spectra "${base}.json.bz2" "$fig"
        echo "$output"
        [[ "$status" -eq 0 ]]
        [[ -s "$fig" ]]
        archive_append "$fig"
    done

    #
    # wirecell-sigproc plot-noise-spectra \
    #    test-noise-roundtrip-inco-spectra.json.bz2 \
    #    inco-spectra.pdf
    echo
}

@test "no weird endpoints of mean waveform" {

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

    local checker=$(wcb_env_value WCGEN)

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
        
    done < <($checker frame_stats test-noise-roundtrip-inco-adc.npz)
    
    
}

teardown () {
    archive_saveout
}
