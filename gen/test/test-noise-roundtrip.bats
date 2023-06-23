#!/usr/bin/env bats

# Test noise "round tripo"

# bats file_tags=topic:noise

bats_load_library wct-bats.sh

setup_file () {

    local cfgfile="$(relative_path test-noise-roundtrip.jsonnet)"

    # files named commonly in each variant subidr
    local jsonfile="dag.json"
    local logfile="wire-cell.log"

    cd_tmp

    run_idempotently -s "$cfgfile" -t "$jsonfile" -- \
                     compile_jsonnet "$cfgfile" "$jsonfile"
    # check bash -c "wcsonnet $cfgfile > ${jsonfile}"

    # run actual job on pre-compiled config
    run_idempotently -s "$jsonfile" -t "$logfile" -- \
                     wire-cell -l "${logfile}" -L debug "${jsonfile}"
    [[ -s "$logfile" ]]

}

@test "no weird endpoints of mean waveform" {
    skip_if_missing jq

    cd_tmp file

    ## good lines are like:
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

    for noise in empno inco
    do

        local adcfile="test-noise-roundtrip-${noise}-adc.npz"
        [[ -s "$adcfile" ]]
        info "adcfile for frame stats: $adcfile"

        local statfile="stats-${noise}.json"
        wcpy gen frame-stats -o "$statfile" "$adcfile"

        for plane in "U" "V" "W"
        do
            # no outliers
            [[ $(jq ".$plane.t.outliers[-1]" < "$statfile") -eq 0 ]]
            # mean should be small
            [[ $(jq ".$plane.t.mu > -0.5 and .$plane.t.mu < 0.5" < "$statfile") = "true" ]]
            # mean should be rms
            [[ $(jq ".$plane.t.rms < 0.2" < "$statfile") = "true" ]]
        done
    done
}


# bats test_tags=history
@test "plot spectra" {


    cd_tmp file

    for ntype in inco empno
    do
        local sfile="test-noise-roundtrip-${ntype}-spectra.json.bz2"

        # Plot OUTPUT spectra
        local ofile="$(basename $sfile .json.bz2)-output.pdf"
        check wcpy sigproc plot-noise-spectra -z "${sfile}" "${ofile}"

        # Plot INPUT spectra.
        local ifile="$(basename $sfile .json.bz2)-input.pdf"
        # The method depends on the type.
        
        # Spectra is inside configuration
        if [ "$ntype" = "inco" ] ; then
            check wcpy sigproc plot-configured-spectra -c ac -n "$ntype" dag.json "${ifile}" 
        fi
        # Spectra is in auxiliary file
        if [ "$ntype" = "empno" ] ; then
            # warning: hard-coding empno spectra out of laziness
            # and assuming it's the one used in the Jsonnet!
            check wcpy sigproc plot-noise-spectra -z protodune-noise-spectra-v1.json.bz2 "${ifile}"
        fi

        saveout -c history "$ifile" "$ofile"

    done
}
