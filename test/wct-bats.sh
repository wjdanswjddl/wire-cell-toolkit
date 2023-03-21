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

# Return shell environment
function dumpenv () {
    env | grep = | sort
    # warn $BATS_TEST_FILENAME
    # warn $BATS_TEST_SOURCE    
}

# Return JSON text of flat object formed from given list of key=value arguments.
# Fixme: this currently requires "jq", perhaps add required functionality into wire-cell-python.
function tojson () {
    declare -a args=("-n")
    while read line
    do
        parts=(${line//=/ })
        key="${parts[0]}"
        val=${line#"${key}="}
        args+=("--arg" "$key" "$val")
    done < <(printf '%s\n' "$@")
    args+=('$ARGS.named')
    jq "${args[@]}"
}

# Return the top level source directory based on this file.
function topdir () {
    dirname $(dirname $(realpath $BASH_SOURCE))
}    

# Return the build directory.
function blddir () {
    echo "$(topdir)/build"
}

# Return the build directory.
function srcdir () {
    local path="$1" ; shift
    local maybe="$(topdir)/$path"
    if [ -d "$maybe" ] ; then
        echo "$maybe"
    fi
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
    local tpath="$(blddir)/output/${name}"
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
    local d="$(topdir)/downloads"
    mkdir -p "$d"
    echo $d
}

# Run the Wire-Cell build tool 
function wcb () {
    $(topdir)/wcb $@
}

# Return key=var lines from wcb' Waf build variables.
# With no arguments, all are returned.
# Otherwise keys may be specified and onlytheir key=var lines are returned.
#
# usage:
# echo "dump Waf vars:"
# wcb_env_vars
#
# usage:
# echo "wire cell version: "
# wcb_env_vars WIRECELL_VERSION
function wcb_env_vars () {
    # we keep a cache to save a whole 100 ms....
    local cache="${BATS_RUN_TMPDIR}/wcb_env.txt"
    if [ ! -f "$cache" ] ; then
        wcb dumpenv | grep = | sort > $cache
    fi
    if [ -z "$1" ] ; then
        cat "$cache"
    fi
    for one in $@
    do
        grep "^${one}=" "$cache"
    done
}


# Return the value for one variable
# usage:
# wcb_env_value WIRECELL_VERSION
wcb_env_value () {
    wcb_env_vars $1 | sed -e 's/[^=]*=//' | sed -e 's/^"//' -e 's/"$//'
}


# Forward arguments to wcb_env_vars and eval return.
#
# usage:
# wcb_env WIRECELL_VERSION
# echo "we have version $WIRECELL_VERSION"
#
# usage:
# wcb_env
# echo "we build subpackages: $SUBDIRS"
function wcb_env () {
    eval $(wcb_env_vars $@)
}


# Add package (util, gen, etc) test apps into PATH, etc.  define
# <pkg>_src to be that sub-package source directory.
function usepkg () {
    for pkg in $@; do
        local pkg=$1 ; shift
        printf -v "${pkg}_src" "%s/%s" "$t" "$pkg"
        local t=$(topdir)
        PATH="$(blddir)/$pkg:$PATH"
    done
#    echo $PATH
}

# Bats defines a base temp directory and two sub-dirs.
# - base :: $BATS_RUN_TMPDIR
# - file :: $BATS_FILE_TMPDIR
# - test :: $BATS_TEST_TMPDIR
#
# to not purge tmp:  bats --no-tempdir-cleanup
function tmpdir () {
    if [ -n "$WCTEST_TMPDIR" ] ; then
        echo "$WCTEST_TMPDIR"
        return
    fi

    local loc="${1:-test}"
    case $loc in
        base) echo "$BATS_RUN_TMPDIR";;
        run) echo "$BATS_RUN_TMPDIR";;
        file) echo "$BATS_FILE_TMPDIR";;
        *) echo "$BATS_TEST_TMPDIR";;
    esac
}

# Change to per test temporary directory
# usage:
# local origin="$(cd_tmp)"
# ...           $ do something
# cd "$origin"  # maybe return
function cd_tmp () {
    pwd
    local t="$(tmpdir $1)"
    mkdir -p "$t"
    cd "$t"
}

# Resolve a file path relative to the path of the BATS test file.
#
# usage:
# local mycfg=$(relative_path my-cfg-jsonnet)
function relative_path () {
    local want="$1"; shift
    realpath "$(dirname $BATS_TEST_FILENAME)/$want"
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

# Return an absolute path of a relative path found in config areas.
function config_path () {
    local path="$1" ; shift
    local maybe="$(top)/cfg/$path"
    if [ -f "$maybe" ] ; then
        echo "$maybe"
        return
    fi
    if [ -n "$WIRECELL_PATH" ] ; then
        resolve_path $path $WIRECELL_PATH
    fi
}

# Do best to resolve a file path and return (echo) an absolute path to
# that file.  If path is relative a variety of locations will be
# searched through a series of pats:
# - if absolute, return
# - cwd
# - search any paths given on command line
# - test/data/
# - $(topdir)/
# - $WIRECELL_PATH
# - TEST_DATA (from build config)
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

    wcb_env

    local t="$(topdir)"
    resolve_path $want "$t/test/data" "$t" ${WIRECELL_PATH} ${TEST_DATA} ${WIRECELL_TEST_DATA_PATH}
    local got="$(resolve_path $want $@)"
    if [ -n "$got" ] ; then
        echo $got
        return
    fi
}


# Echo the test data directory, if it exists
function test_data_dir () {
    wcb_env_value TEST_DATA
}


# Resolve relative path to a test data file.  This ONLY looks in the
# directory given by "./wcb --test-data".
function test_data_file () {
    resolve_path $1 $(test_data_dir)
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


# skip test if there is no test data repo
function skip_if_no_test_data () {
    local path="$1"             # may require specific path

    tdd="$(test_data_dir)"
    if [ -n "${tdd}" ] ; then
        if [ -d "$tdd" ] ; then
            if [ -z "$path" ] ; then
                return
            fi
            path="$(test_data_file $path)"
            if [ -f "${path}" ] ; then
                return
            fi
        fi
    fi
    # yell "Test data file missing: ${path}"
    skip "Test data missing"
}
