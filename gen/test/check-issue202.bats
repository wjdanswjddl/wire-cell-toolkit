#!/usr/bin/env bats

# Test for:
# https://github.com/WireCell/wire-cell-toolkit/issues/202

# bats file_tags=issue:202,topic:noise,time:2

# Commentary
# 
# The way noise generaion is supposed to work: For each frequency bin
# we draw 2 normal (mean=0, sigma=1) random numbers and multiply each
# by the input real-valued "mode" (not technically "mean") spectral
# amplitude at that frequency.  We then interpret the two numbers as
# the real and imaginary parts of the sampled spectrum at that
# frequency.  This is equivalent to a Rayleigh-sampled radius and a
# uniform-sampled phase angle.
# 
# The bug shifted the two sampled "normals" from being distributed
# around mean=0 to being distributed around mean=0.04.  This shifts
# the center of the complex distribution from 0 to 0.04*(1+i).  Thus
# the "mode" of the sampled (real part) distribution should be
# increased by the bug.  Right?  What is the mean distance from origin
# to a circle that is offset from the origin by less than one radius?
#
# In the process of understanding that but it was found that a change
# in the recent past introduced round() on the floating point ADC
# value prior to its return by Digitizer.  It's effect is also
# checked here.
#
# CAUTION: after fixing 202, this tests relies an undocumented option
# [redacted] to the {Incoherent,Coherent}AddNoise components in order
# to explicitly reinstate this bug.  This is to produce plots to show
# the effect which may be useful to experiments with results derived
# from the buggy noise.  When/if the option [redacted] option is
# removed in the future this file must be reduced to merely test for
# it instead of reproducing it.
#
# The bug fixed in 202 cause the use of a "normal" distribution with
# mean=0.04 instead of mean=0.0.  We test for this mean and a "twice
# sized bug" mean.  When/if removing the [redacted] option, any use of
# "smlbug" and "bigbug" must be deleted.
declare -A bugmean=( [nobug]=0.0 [smlbug]=0.04 [bigbug]=0.08 )

bats_load_library wct-bats.sh

# Policy helpers
#
# Sub directory name for a variant test.
#
variant_subdir () {
    local trunc=$1 ; shift
    local bug=${1:-nobug}
    echo "${trunc}-${bug}"
}
#
# A file name for a data tier relative to a variant subdir
#
tier_path () {
    local ntype="${1}"; shift
    local tier=${1:-adc};
    local ext="npz"
    if [ "$tier" = "spectra" ] ; then
        ext="json.bz2"
    fi
    echo "test-issue202-${ntype}-${tier}.${ext}"
}

# Run a wire-cell job to make files that span:
#
# ntype in:
# - inco : parameterized group noise model applied incoherently
# - cohe : parameterized group noise model applied coherently
# - empno : measured empirical noise model applied incoherently
#
# tier in:
# - vin = voltage level
# - adc = ADC counts
# - dac = ADC scaled to voltage
# - spectra = output of NoiseModeler
#
# files like:
# test-issue202-{ntype}-{tier}.npz
# test-issue202-{ntype}-spectra.json.bz2
#
setup_file () {

    local cfgfile="${BATS_TEST_FILENAME%.bats}.jsonnet"

    # files named commonly in each variant subidr
    local jsonfile="$(basename ${cfgfile} .jsonnet).json"
    local logfile="$(basename ${cfgfile} .jsonnet).log"

    # explicitly compile cfg to json to pre-check [redacted] is implemented.
    for bugsiz in nobug smlbug bigbug
    do
        for trunc in round floor
        do
            cd_tmp

            wd="$(variant_subdir $trunc $bugsiz)"
            mkdir -p $wd
            cd $wd

            if [ -f "${jsonfile}" ] ; then
                echo "already have ${wd}/${jsonfile}"
                continue
            fi

            local bug=${bugmean[$bugsiz]}
            check bash -c "wcsonnet -A bug202=$bug -A round=$trunc $cfgfile > ${jsonfile}"

            if [ "$trunc" = "round" ] ; then
                [[ "$(grep -c '"round" : true' ${jsonfile})" -eq 3 ]]
            else
                [[ "$(grep -c '"round" : false' ${jsonfile})" -eq 3 ]]
            fi

            # representative output file
            local adcfile="$(tier_path empno)";
            if [ -f "${adcfile}" ] ; then
                echo "Already have ${wd}/${adcfile}" 1>&3
                continue
            fi

            # run actual job on pre-compiled config
            wire-cell -l "${logfile}" -L debug "${jsonfile}"

            [[ -s "$logfile" ]]
            if [ "$trunc" = "round" ] ; then
                [[ -n "$(grep round=1 ${logfile})" ]]
            else
                [[ -n "$(grep round=0 ${logfile})" ]]
            fi
            echo "$adcfile"
            [[ -s "$adcfile" ]]

        done
    done
    cd_tmp
}


@test "no weird endpoints of mean waveform" {

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

    # Neither round nor floor must exhibit the bug
    for trunc in round floor
    do
        local wd="$(variant_subdir $trunc)"

        local adcfile="$wd/$(tier_path empno)"
        echo "$adcfile"
        [[ -s "$adcfile" ]]

        while read line ; do
            parts=($line)

            echo "$line"
            if [ "${parts[1]}" = "c" ] ; then
                continue
            fi

            # Bash does not speak floating point.  To avoid relying on
            # dc/bc to be installed, we do these FP tests lexically.

            # require nothing past 5 sigma
            [[ "${parts[10]}" = "0" ]]

            # mean should be small.
            [[ -n "$(echo ${parts[3]} | grep '^0\.0')" ]]

            # rms should be small.
            [[ -n "$(echo ${parts[4]} | grep '^0\.0')" ]]
            
        done < <(wcpy gen frame_stats "$adcfile")
    done
}


# bats test_tags=implicit,plots
@test "plot adc frame means" {

    local tname="$(basename $BATS_TEST_FILENAME .bats)"

    for bugsiz in nobug smlbug bigbug
    do
        for trunc in round floor
        do
            cd_tmp file
            local wd="$(variant_subdir $trunc $bugsiz)"
            [[ -d "$wd" ]]
            cd "$wd"
            for ntype in inco cohe
            do

                local adcfile="$(tier_path $ntype)"
                local figfile="$(basename $adcfile .npz)-means.png"

                check wcpy plot frame-means -o "$figfile" "$adcfile"
                [[ -s "$figfile" ]]
                saveout -c plots "$figfile"
            done
        done
    done
}


# bats test_tags=implicit,pltos
@test "plot spectra" {

    for bugsiz in nobug smlbug bigbug
    do
        for trunc in round floor
        do

            cd_tmp file
            local wd="$(variant_subdir ${trunc} ${bugsiz})"
            [[ -d "$wd" ]]
            cd "$wd"

            for ntype in inco empno
            do
                local sfile="$(tier_path $ntype spectra)"

                # Plot OUTPUT spectra
                local ofile="$(basename $sfile .json.bz2)-output.pdf"
                check wcpy sigproc plot-noise-spectra -z "${sfile}" "${ofile}"

                # Plot INPUT spectra.
                local ifile="$(basename $sfile .json.bz2)-input.pdf"
                # The method depends on the type.
                
                # Spectra is inside configuration
                if [ "$ntype" = "inco" ] ; then
                    check wcpy sigproc plot-configured-spectra -c ac -n "$ntype" test-issue202.json "${ifile}" 
                fi
                # Spectra is in auxiliary file
                if [ "$ntype" = "empno" ] ; then
                    # warning: hard-coding empno spectra out of laziness
                    # and assuming it's the one used in the Jsonnet!
                    check wcpy sigproc plot-noise-spectra -z \
                        protodune-noise-spectra-v1.json.bz2 "${ifile}"
                fi

                saveout -c plots "$ifile" "$ofile"

            done
        done
    done
}

# bats test_tags=implicit,plots
@test "compare round vs floor no bug" {
    # cd_tmp file
    cd_tmp file
    
    local fadc="$(variant_subdir floor)/$(tier_path inco)"
    local radc="$(variant_subdir round)/$(tier_path inco)"

    for bl in ac none
    do
        for thing in wave spec
        do
            for grp in $(seq 10)
            do
                local num=$(( $grp - 1 ))
                local chmin=$(( $num * 256 ))
                local chmax=$(( $grp * 256 ))

                local outpng="comp1d-floor+round-nobug-${thing}-${bl}-grp${grp}.png"

                wirecell-plot comp1d \
                    --transform $bl \
                    --single \
                    --tier '*' -n $thing --chmin $chmin --chmax $chmax \
                    --output "$outpng" \
                    "$fadc" "$radc"

                    if [[ $grp -le 3 ]] ; then
                        saveout -c plots $outpng
                    fi
            done
        done
    done
}

# bats test_tags=implicit,plots
@test "compare bug and no bug with round and floor" {

    cd_tmp file

    for trunc in round floor
    do
        local radcnb="$(variant_subdir $trunc nobug)/$(tier_path inco)"
        local radcsb="$(variant_subdir $trunc smlbug)/$(tier_path inco)"
        local radcbb="$(variant_subdir $trunc bigbug)/$(tier_path inco)"

        for bl in ac none
        do
            for thing in wave spec
            do
                for grp in $(seq 10)
                do
                    local num=$(( $grp - 1 ))
                    local chmin=$(( $num * 256 ))
                    local chmax=$(( $grp * 256 ))

                    outpng="comp1d-${trunc}-bugs-${thing}-${bl}-grp${grp}.png"

                    wirecell-plot comp1d \
                                  --transform $bl \
                                  --single \
                                  --tier '*' -n $thing --chmin $chmin --chmax $chmax \
                                  --output "$outpng" \
                                  "$radcnb" "$radcsb" "$radcbb"
                    if [[ $grp -eq 10 ]] ; then
                        saveout -c plots $outpng
                    fi
                done
            done
        done
    done
}
