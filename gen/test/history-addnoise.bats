#!/usr/bin/env bats

# This test consumes historical files produced by test-addnoise.bats
#
# See eg bv-generate-history-haiku for example how to automate
# producing historical files.  O.w. collect one or more history/
# directories and add their parent locations to WCTEST_DATA_PATH.

load ../../test/wct-bats.sh

@test "historical addnoise comp1d plots" {

    local wcplot=$(wcb_env_value WCPLOT)
    
    local -a vers
    local -A verpaths
    for vpath in $(category_version_paths -c history)
    do
        ver=$(basename $vpath)
        vers+=( $ver )
        verpaths[$ver]=$vpath
    done

    local vers=( $(printf "%s\n" ${vers[@]} | sort -u | tail -4 ) )
    declare -a ordered
    for ver in ${vers[*]}
    do
        ordered+=( "${verpaths[$ver]}/test-addnoise/test-addnoise-empno-6000.tar.gz"  )
    done

    local allvers="$(printf -- "-%s" ${vers[@]})"
    for plot in wave spec
    do
        local plotfile="comp1d-${plot}-history${allvers}.png"
        $wcplot \
            comp1d -n $plot --markers 'o + x .' -t '*' \
            --chmin 0 --chmax 800 -s --transform ac \
            -o $plotfile ${ordered[@]}
        saveout -c reports $plotfile

    done

    local plotfile="comp1d-spec-history${allvers}-zoom1.png"
    $wcplot \
        comp1d -n spec --markers 'o + x .' -t '*' \
        --chmin 0 --chmax 800 -s --transform ac \
        --xrange 100 800 \
        -o $plotfile ${ordered[@]}
    saveout -c reports $plotfile

    local plotfile="comp1d-spec-history${allvers}-zoom2.png"
    $wcplot \
        comp1d -n spec --markers 'o + x .' -t '*' \
        --chmin 0 --chmax 800 -s --transform ac \
        --xrange 1000 2000 \
        -o $plotfile ${ordered[@]}
    saveout -c reports $plotfile


}

