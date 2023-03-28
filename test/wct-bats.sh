#!/usr/bin/env bash

# BATS helper functions.
# To use inside a <pkg>/test/test_XXX.bats do:
#
# load ../../test/wct-bats.sh
#
# See test/docs/bats.org for more info.
#
# Note, these generally exit on error.

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
    [[ -n "$src" ]]
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
    yell "saved $src to $tpath"
}

# Return the download cache directory
function downloads () {
    local d="$(topdir)/downloads"
    mkdir -p "$d"
    echo $d
}

# Run the Wire-Cell build tool 
function wcb () {
    local here="$(pwd)"
    cd "$(topdir)"
    ./wcb $@
    cd "$here"
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
        wcb dumpenv | grep 'wcb: ' | sed -e 's/^wcb: '// | sort > $cache
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
        local t=$(topdir)
        printf -v "${pkg}_src" "%s/%s" "$t" "$pkg"
        export ${pkg}_src
        PATH="$(blddir)/$pkg:$PATH"
    done
#    echo $PATH
}

# Execute the Jsonnet compiler under Bats "run".
# Tries wcsonnet then jsonnet.
# The cfg dir is put into the search path and WIRECELL_PATH is ignored.
#
# usage:
# compile_jsonnet file.jsonnet file.json [-A/-V etc options, no -J/-P]
function compile_jsonnet () {
    local ifile=$1 ; shift
    local ofile=$1 ; shift
    local cfgdir="$(topdir)/cfg"

    local orig_wcpath=$WIRECELL_PATH
    WIRECELL_PATH=""

    cmd=$(wcb_env_value WCSONNET)
    if [ -n "$cmd" -a -x "$cmd" ] ; then
        ## switch to this when wcsonnet gets "-o"
        # cmd="$cmd -P $cfgdir -o $ofile $@ $ifile"
        ## until then
        cmd="$cmd -o $ofile -P $cfgdir $@ $ifile"
    fi
    if [ -z "$cmd" ] ; then
        cmd=$(wcb_env_value JSONNET)
        if [ -z "cmd" ] ; then
            cmd="jsonnet"       # hail mary
        fi
        if [ -n "$cmd" -a -x "$cmd" ] ; then        
            cmd="$cmd -o $ofile -J $cfgdir $@ $ifile"
        fi
    fi
    [[ -n "$cmd" ]]
    echo "$cmd"
    run $cmd
    echo "$output"
    [[ "$status" -eq 0 ]]
    WIRECELL_PATH="$orig_wcpath"
}

# Execute wire-cell CLI under Bats "run".
# This wraps a normal wire-cell CLI in some boilerplate.
#
# usage:
# wct [args]
function wct () {
    cli=$(wcb_env_value WIRE_CELL)
    [[ -n "$cli" ]]
    [[ -x "$cli" ]]

    run $cli $@
    echo "$output"
    [[ "$status" -eq 0 ]]    
}


# Execute the "wirecell-pgraph dotify" program under Bats "run".
#
# usage:
# dotify_graph input.{json,jsonnet} output.{svg,png,pdf}
function dotify_graph () {
    local ifile=$1 ; shift
    [[ -n "$ifile" ]]
    [[ -f "$ifile" ]]
    local ofile=$1 ; shift
    [[ -n "$ofile" ]]
    local cfgdir="$(topdir)/cfg"
    [[ -d "$cfgdir" ]] 

    cmd=$(wcb_env_value WCPGRAPH)
    [[ -n "$cmd" ]]
    [[ -x "$cmd" ]]    
    
    cmd="$cmd dotify -J $cfgdir $@ $ifile $ofile"
    echo "$cmd"
    run $cmd
    echo "$output"
    [[ "$status" -eq 0 ]]
}



# Return the Bats context name as divined by the Bats environment.
#
# test, file or run
function divine_context () {
    if [ -n "$BATS_TEST_NAME" ] ; then
        echo "test"
    elif [ -n "$BATS_TEST_FILENAME" ] ; then
        echo "file"
    elif [ -n "$BATS_RUN_TMPDIR" ] ; then
        echo "run"
    else
        die "apparently not running under bats"
    fi
}


# Return a Bats temporary directory.
#
# With no arguments, this divines the correct one for the current Bats context.
#
# With a single argument it will return the temporary directory for that context.
#
# To not purge the tmp:  bats --no-tempdir-cleanup
#
# If WCTEST_TMPDIR is defined, it trumps all.
function tmpdir () {
    if [ -n "$WCTEST_TMPDIR" ] ; then
        echo "$WCTEST_TMPDIR"
        return
    fi

    local loc="${1:-test}"
    case $loc in
        run) echo "$BATS_RUN_TMPDIR";;
        file) echo "$BATS_FILE_TMPDIR";;
        *) echo "$BATS_TEST_TMPDIR";;
    esac
}

# Change to the context temporary directory.
#
# If a context is given, that temporary directory will be used
# otherwise it will be divined.
#
# usage:
# cd_tmp
function cd_tmp () {
    ctx="$1"
    if [ -z "$ctx" ] ; then
        ctx="$(divine_context)"
    fi
    local t="$(tmpdir $ctx)"
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
    local maybe="$(topdir)/cfg/$path"
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
    [[ -n "$got" ]]
    [[ -f "$got" ]]
    echo $got
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


# Return the file name of a archive in a test context.
#
# Normally, this name is opaque to testing code.
#
# A tmpdir context name may be given.  Otherwise the result is context
# dependent.  
function archive_name () {
    ctx="$1"
    if [ -n "$ctx" ] ; then
        ctx="$(divine_context)"
    fi

    local t="$(tmpdir $ctx)"
    [[ -n "$t" ]]
    echo "$t/archive.zip"
}

# Append one or more files to an archive.
#
# The first argument may be a tmpdir context name ("file" or "test")
# to select a specific archive file, o.w. the context is detected.
#
# Otherwise, arguments are interpeted as file paths.
#
# Only RELATIVE and DESCENDING paths are allowed.  All others (eg
# absolute "/..." or ascending "../...") are an error.
#
# usage:
#
# cd_tmp
# touch file1.txt file2.txt
# archive_append file1.txt file2.txt
# 
# mkdir subdir && cd subdir
# touch file3.txt
# archive_append file3.txt  # added as "subdir/file3.txt"
function archive_append () {
    local ctx="test"
    if [ -n "$(echo "test file run" | grep "\b$1\b")" ] ; then
        ctx="$1"
        shift
    fi

    local arch="$(archive_name $ctx)"
    local base="$(tmpdir $ctx)"
    local here="$(pwd)"
    for one in $@
    do
        if [[ $one =~ ^/.* || $one =~ ^\.\./.* ]] ; then
            die "illegal archive path: $one"
        fi
        cd "$here"
        abs="$(realpath $one)"
        rel="$(realpath --relative-to=$base $abs)"
        cd "$base"
        zip -g "$arch" "$rel"
    done
    cd $here
}


# Save an archive out of tmpdir and to build dir.
# A tmpdir context may be passed.
function archive_saveout () {
    local ctx="test"

    if [ -n "$1" -a -n "$(echo "test file run" | grep "\b$1\b")" ] ; then
        ctx="$1"
        shift
    fi

    local arch="$(archive_name $ctx)"
    local tname="${BATS_TEST_NAME}"

    tgt="archive/$(basename $BATS_RUN_TMPDIR)"
    if [ "$ctx" = "test" ] ; then
        tgt="$tgt/${tname}.zip"
    elif [ "$ctx" = "file" ] ; then
        tgt="$tgt/file.zip"
    elif [ "$ctx" = "run" ] ; then
        tgt="$tgt/run.zip"
    fi
    saveout "$arch" "$tgt"
}
