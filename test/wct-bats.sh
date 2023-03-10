#!/usr/bin/env bash

# BATS helper functions.
# To use inside a <pkg>/test/test_XXX.bats do:
# load ../../test/wct-bats.sh

# Return the top level source directory
function top () {
    dirname $(dirname $(realpath $BASH_SOURCE))
}    

# Return the build directory.
function bld () {
    return $(top)/build
}

# Return the download cache directory
function downloads () {
    local d="$(top)/downloads"
    mkdir -p "$d"
    echo $d
}

# Run the Wire-Cell build tool 
function wcb () {
    $(top)/wcb $@
}

# Set Waf environment variables as shell environment variables
function wcb_env () {
    eval $(wcb dumpenv | grep = )
}

# Add package (util, gen, etc) test apps into PATH, etc.  define
# <pkg>_src to be that sub-package source directory.
function usepkg () {
    for pkg in $@; do
        local pkg=$1 ; shift
        printf -v "${pkg}_src" "%s/%s" "$t" "$pkg"
        local t=$(top)
        PATH="$t/build/$pkg:$PATH"
    done
    echo $PATH
}

# Bats defines a base temp directory and two sub-dirs.
# - base :: $BATS_RUN_TMPDIR
# - file :: $BATS_FILE_TMPDIR
# - test :: $BATS_TEST_TMPDIR
#
# to not purge tmp:  bats --no-tempdir-cleanup

# Change to per test temporary directory
# usage:
# local origin="$(cd_tmp)"
# ...           $ do something
# cd "$origin"  # maybe return
function cd_tmp () {
    pwd
    cd $BATS_TEST_TMPDIR
}


# Do best to take a relative file name and return (echo) an absolute
# path to that file.  
function resolve_file () {
    local want="$1" ; shift

    # it's RIGHT there
    if [ -f "$want" ] ; then
        echo "$want"
        return
    fi

    if [ -f "$(downloads)/$want" ] ; then
        echo "$(downloads)/$want"
        return
    fi

    if [ -f "$(top)/test/data/$want" ] ; then
       echo "$(top)/test/data/$want"
       return
    fi

    for maybe in $(echo ${WIRECELL_PATH} | tr ":" "\n")
    do
        if [ -f "${maybe}/$want" ] ; then
            echo "${maybe}/$want"
            return
        fi
    done

    for maybe in $(echo ${WIRECELL_TEST_DATA_PATH} | tr ":" "\n")
    do
        if [ -f "${maybe}/$want" ] ; then
            echo "${maybe}/$want"
            return
        fi
    done
}

# Download a file from a URL.
#
# usage:
# download_file https://www.phy.bnl.gov/~hyu/wct-ci/gen/depos.tar.bz2
#
# or with optional target name
# download_file https://www.phy.bnl.gov/~hyu/wct-ci/gen/depos.tar.bz2 my-depos.tar.bz2
#
# Return (echo) a path to the target file.  This will be a location in a cache.
function download_file () {
    local url="$1"; shift
    local tgt="${1:-$(basename $url)}"

    local path="$(downloads)/$tgt"
    if [ -s $path ] ; then
        echo "$path"
        return
    fi

    wget --quiet -O "$path" "$url" || return
    echo "$path"
}

# # Return absolute path to file.
# #
# # This will search in order:
# # - the directory holding BATS_TEST_FILENAME
# # - BATS_CWD
# # - BATS_CWD/.. if BATS_CWD ends in /build/
# # - every dir in WIRECELL_PATH
# # - every dir in WIRECELL_TEST_PATH
# # usage:
# # local f=$(data_file muon-depos.npz
# function resolve () {
#     local f="$1" ; shift
#     local t="$(top)"
#     if [ -f "$t/test/data/$1" ] then
#        echo "$t/test/data/$1"
#        return
#     fi
# }
