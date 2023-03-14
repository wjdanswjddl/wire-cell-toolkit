#!/usr/bin/env bash

# BATS helper functions.
# To use inside a <pkg>/test/test_XXX.bats do:
# load ../../test/wct-bats.sh

function log () {
    echo "$@"
}

function yell () {
    echo "$@" 1>&3
}

function warn () {
    local msg="warning: $@"
    log "$msg"
    yell "$msg" 
}

function die () {
    local msg="FATAL: $@"
    log "$msg"
    yell "$msg" 
    exit 1
}

function dumpenv () {
    env | grep = | sort
    # warn $BATS_TEST_FILENAME
    # warn $BATS_TEST_SOURCE    
}

# Return the top level source directory based on this file.
function top () {
    dirname $(dirname $(realpath $BASH_SOURCE))
}    

# Return the build directory.
function bld () {
    echo "$(top)/build"
}

# Save a product of the test to the build area.
# usage:
#
# save path/in/test/dir/file.ext [target/dir/file2.ext]
#
# If a second argument is given it is interpreted to be a relative
# path to where the file should be copied.  Actual copied file path
# will be like:
#
# build/output/<name>/target/dir/file2.ext
#
# where <name> is from BATS_TEST_FILENAME
function saveout () {
    local src="$1" ; shift
    local tgt="$1"
    local name="$(basename $BATS_TEST_FILENAME .bats)"
    local tpath="$(bld)/output/${name}"
    if [ -z "$tgt" ] ; then
        mkdir -p "$tpath"
        tpath="$tpath/$(basename $src)"
    else
        tpath="$tpath/$tgt"
        mkdir -p "$(dirname $tpath)"
    fi
    cp "$src" "$tpath"
    log "saved $src to $tpath"
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
    if [ -n "$WCTEST_TMPDIR" ] ; then
        mkdir -p "$WCTEST_TMPDIR"
        cd "$WCTEST_TMPDIR"
    else 
        cd "$BATS_TEST_TMPDIR"
    fi
}

# Resolve a file name in in or more path lists.
# A path list may be a :-separated list
#
# usage:
# local path="$(resolve_path emacs $PATH $MY_OTHER_PATH /sbin:/bin:/etc)"
function resolve_path () {
    local want="$1"; shift

    # already absolute
    if [[ "$want" =~ ^/.* ]] ; then
        echo $want
    fi


    for pathlst in $@ ; do
        for maybe in $(echo ${pathlst} | tr ":" "\n")
        do
            if [ -f "${maybe}/$want" ] ; then
                echo "${maybe}/$want"
                return
            fi
        done
    done
}

# Do best to resolve a file path and return (echo) an absolute path to
# that file.  If path is relative a variety of locations will be
# searched through a series of pats:
# - if absolute, return
# - cwd
# - search any paths given on command line
# - test/data/
# - $(top)/
# - $WIRECELL_PATH
# - $WIRECELL_TEST_DATA_PATH
#
# Note, the original BATS file path is not available here and so we
# can not check for siblings.  To check for siblings call like:
#
# local path="$(resolve_file $filename $(dirname $BATS_TEST_FILENAME))"
#
# See resolve_path for policy free path searches
function resolve_file () {
    local want="$1" ; shift

    if [ -f "$want" ] ; then
        realpath "$want"
        return
    fi

    local t="$(top)"
    resolve_path $want "$t/test/data" "$t" ${WIRECELL_PATH} ${WIRECELL_TEST_DATA_PATH}
    local got="$(resolve_path $want $@)"
    if [ -n "$got" ] ; then
        echo $got
        return
    fi
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

    wget -O "$path" "$url" 1>&2 || return
    echo "$path"
}

# Assure that test data repository is available and located by setting
# WIRECELL_TEST_DATA_PATH.  If this variable is already set, this
# function is a no-op.
function assure_wirecell_test_data () {
    if [ -n "$WIRECELL_TEST_DATA_PATH" ] ; then
        return
    fi

    declare -a lst
    # look for sibling of path in WIRECELL_PATH
    for one in $(echo ${WIRECELL_PATH} | tr ":" "\n") ; do
        local base="$(dirname $(realpath $one))"
        local maybe="$base/wire-cell-test-data"
        # log "AWCTD: $one $base $maybe"
        if [ -d "$maybe" ] ; then
            lst+="$maybe"
        fi
    done
    if [ -z "${lst[*]}" ] ; then
        die "no WIRECELL_TEST_DATA_PATH found"
    fi
    IFS=: ; printf -v WIRECELL_TEST_DATA_PATH '%s' "${lst[*]}"
#    export WIRECELL_TEST_DATA_PATH
}
