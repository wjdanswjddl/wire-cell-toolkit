#!/usr/bin/env bash

# BATS helper functions.
# To use inside a <pkg>/test/test_XXX.bats do:
#
# load ../../test/wct-bats.sh
#
# See test/docs/bats.org for more info.
#
# Note, these generally exit on error.

shopt -s extglob
shopt -s nullglob

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


# Save out a file to the build/output/ directory
#
# usage:
#
#  saveout [-c/--category <cat>] [-t/--target <tgt>] <src> [<src> ...]
#
# If the optional -c/--category <cat> is given, the file(s) are saved
# out to a category specific sub directory.  Default category is
# "output".
# 
# If the optional -t/--target <tgt> is given, only the first <src> is
# saved copied to that target.  <tgt> must be relative.
#
# Files are copied to build/tests/<category>/<version>/<test-name>/
#
# Where:
# - <version> is the current wire-cell version
# - <test-name> is from BATS_TEST_FILENAME without the extension.
function saveout () {
    declare -a src=()
    subdir="output"
    tgt=""
    while [[ $# -gt 0 ]] ; do
        case $1 in
            -t|--target) tgt="$2"; shift 2;;
            -c|--category) subdir="$2"; shift 2;;
            -*|--*) die "Unknown option $1";;
            *) src+=("$1"); shift;;
        esac
    done
          
    [[ -n "$src" ]]

    local name="$(basename $BATS_TEST_FILENAME .bats)"
    local base="$(blddir)/tests/$subdir/$(version)/${name}"

    # single, directed target
    if [ -n "$tgt" ] ; then
        tgt="${base}/${tgt}"
        mkdir -p "$(dirname $tgt)"
        cp "$src" "$tgt"
        return
    fi

    mkdir -p "${base}"
    # multi, implicit target
    for one in ${src[*]}
    do
        local tgt="${base}/$(basename ${one})"
        mkdir -p "$(dirname $tgt)"
        cp "$src" "$tgt"
    done
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
    local cache="${BATS_RUN_TMPDIR:-/tmp}/wcb_env.txt"
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

# Emit version string
function version() {
    wct --version
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

# Resolve multiple pathlists to a single list, removing any that do
# not exist.  Consequitive duplicates are removed.
#
# usage:  assume X does not exist
# paths=( $(resolve_pathlist A:X A B:C) )
# A
# B
# C
function resolve_pathlist () {
    declare -a parts=( $(echo $@ | tr ':' ' ') )
    declare -a exists
    for one in ${parts[@]}
    do
        if [ -d $one ] ; then
            exists+=( $one )
        fi
    done
    printf "%s\n" ${exists[@]} | awk '!seen[$0]++'

}

# Resolve a file name in in or more path lists.
# A path list may be a :-separated list
#
# usage:
# local path="$(resolve_path emacs $PATH $MY_OTHER_PATH /sbin:/bin:/etc)"
# /usr/bin/emacs
function resolve_path () {
    local want="$1"; shift

    # already absolute
    if [[ "$want" =~ ^/.* ]] ; then
        echo $want
    fi

    for maybe in $(resolve_pathlist $@) ; do
        if [ -f "${maybe}/$want" ] ; then
            echo "${maybe}/$want"
            return
        fi
    done
}


# Emit all matching paths.
#
# usage:
# resolve_all emacs $PATH
# /usr/bin/emacs
# /usr/local/bin/emacso
#
function find_paths () {
    local want="$1" ; shift

    # already absolute, only one answer
    if [[ "$want" =~ ^/.* ]] ; then
        echo $want
    fi
    
    for maybe in $(resolve_pathlist $@) ; do
        if [ -f "${maybe}/$want" ] ; then
            echo "${maybe}/$want" 
        fi
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

# Emit the absolute path for a given path.
#
# usage:
# resolve_file file1.txt
#
# If absolute, emit as given.  If not found, exit.
#
# The following path lists will be checked for the given relative path
# on a first one wins basis.
# 
# - current working directory
# - relative to directory holding current Bats test file
# - any path lists given on function command line
# - $(topdir)/test/data/
# - $(topdir)/
# - $(topdir)/build/output/
# - $WIRECELL_PATH
# - TEST_DATA (from "./wcb --test-data" option)
# - $WCTEST_DATA_PATH
#
# See resolve_path to resolve paths in a policy-free manner.
#
# See relative_path to resolve paths relative to bats test file.
#
function resolve_file () {
    local want="$1" ; shift

    local test_data="$(wcb_env_value TEST_DATA)"
    local mydir="$(dirname ${BATS_TEST_FILENAME})"
    local t="$(topdir)"
    local paths=$(resolve_pathlist "$t/test/data" "$t" ${WIRECELL_PATH} ${test_data} ${WCTEST_DATA_PATH})

    if [ -f "$want" -o -d "$want" ] ; then
        realpath "$want"
        return
    fi
    
    for $path in ${paths[*]}
    do
        local maybe=$path/$want
        if [ -f "$maybe" -o -d "$maybe" ] ; then
            echo $maybe
            return
        fi
    done
    die "No such file or directory: $want"
}

# Emit all versions paths found for category.
#
# usage:
# category_version_paths [-c/--category <cat>] [-d/--dirty] [-p|--pathlist <pathlist>]
#
# Default category is "output".
# 
# Default matches only release versions, -d/--dirty will also match locally modified versions
#
# -p/--pathlist gives a ":"-separated path list to check prior to checking $(blddir)/tests and WCTEST_DATA_PATH.
#
# The path lists are checked for <category>/<version>/
function category_version_paths () {
    local category="output"
    # match "git describe --tags" 
    local matcher='+([0-9])\.+([0-9])\.+([x0-9])'
    local dirty=""
    declare -a pathlist
    while [[ $# -gt 0 ]] ; do
        case $1 in
            -c|--category) category="$2"; shift 2;;
            -d|--dirty) dirty="${matcher}-*"; shift;;
            -p|--pathlist) pathlist+=( "$2" ); shift 2;;
            *) break;;          # leave others args in place
        esac
    done

    pathlist=( $(resolve_pathlist ${pathlist[*]} $(blddir)/tests ${WCTEST_DATA_PATH}) )

    declare -a ret

    for path in ${pathlist[*]}
    do
        for one in $(echo $path/$category/$matcher)
        do
            if [ -d "$one" ] ; then
                ret+=( $one )
            fi
        done
        if [ -z "$dirty" ] ; then
            continue
        fi
        for one in $(echo $path/$category/$dirty)
        do
            if [ -d "$one" ] ; then
                ret+=( $one )
            fi
        done
    done
    echo ${ret[*]}
}

# Emit the category path for the version
# usage:
# category_path [-c/--category <cat>] [-d/--dirty] [-p|--pathlist <pathlist>] <version>
#
# See category_version_paths for details.
function category_path () {
    declare -a pdirs=( $(category_version_paths $@) )
    echo "pdirs: ${pdirs[*]}" 1>&2
    echo "left: $@" 1>&2
    local ver="$1" ; shift


    for maybe in ${pdirs[*]}
    do 
        echo "maybe: $maybe" 1>&2
        if [ "$ver" = "$(basename $maybe)" ] ; then
            echo $maybe
            return
        fi
    done
    die "no category path for version $ver"
}
    
    
# build/tests/history/<ver>/test-addnoise/test-addnoise-empno-6000.tar.gz


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

    skip "Test data missing"
}
