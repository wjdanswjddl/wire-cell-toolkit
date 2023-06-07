#!/usr/bin/env bash

# BATS helper functions.
# To use inside a <pkg>/test/test_XXX.bats do:
#
# bats_load_library wct-bats.sh
#
# See test/docs/bats.org for more info.
#
# Note, these generally exit on error.

shopt -s extglob
shopt -s nullglob

# log <message>
#
# Echo message into BATS controlled logging.
function log () {
    echo "$@"
}

# yell <message>
#
# Echo message to terminal, escaping BATS controlled logging.
function yell () {
    echo "$@" 1>&3
}

# warn <message>
#
# Echo message to log and terminal
function warn () {
    local msg="warning: $@"
    log "$msg"
    yell "$msg" 
}

# die <message>
#
# Echo message to log and terminal and exit
function die () {
    local msg="FATAL: $@"
    log "$msg"
    yell "$msg" 
    exit 1
}


# file_larger_than <filename> <nbytes>
#
# Assert named file is larger than number of bytes.
function file_larger_than () {
    local filename=$1 ; shift
    local minsize=$1; shift
    echo "check file size for $filename"
    [[ -f "$filename" ]]
    local fsize="$(stat -c '%s' $filename)"
    echo "minsize is $minsize. $filename is size $fsize"
    [[ -n "$minsize" ]]
    [[ "$fsize" -gt "$minsize" ]]
}


# run_idempotently [options] -- <command line>
#
# Run command if any target file is missing or older than any source file.
#
# Options:
# -v, --verbose
#   Will "yell" about what it runs.
# -s, --source <filename>
#   Name a source file for idempotency checking (multiple okay).
# -t, --target <filename>
#   Name a target file for idempotency checking (multiple okay).
# --
#   Separate options from command line to run.
# <command line>
#   The command line to run.
function run_idempotently () {

    declare -a src
    declare -a tgt
    local verbose="no"
    while [[ $# -gt 0 ]] ; do
        case $1 in
            -s|--source) src+=( $2 ) ; shift 2;;
            -t|--target) tgt+=( $2 ) ; shift 2;;
            -v|--verbose) verbose="yes"; shift;;
            --) shift; break;
        esac
    done

    local need_to_run="no"
    if [ -z "$src" -o -z "$tgt" ] ; then
        # Always run if sink, source or atomic.
        need_to_run="yes"
    fi
    if [ "$need_to_run" = "no" ] ; then
        # Run if missing any targets
        for one in "${tgt[@]}"  
        do
            if [ ! -f "$one" ] ; then
                need_to_run="yes"
            fi
        done
    fi
    if [ "$need_to_run" = "no" ] ; then
        # Run if any source is newer than any targeta
        local src_newest="$(ls -t ${src[@]} | head -1)"
        local tgt_oldest="$(ls -t ${tgt[@]} | tail -1)"

        if [ "$src_newest" -nt "$tgt_oldest" ] ; then
            need_to_run="yes"
        fi
    fi
    
    if [ "$need_to_run" = "no" ] ; then
        yell "target newer than source not running: $@"
        return
    fi

    echo "running: $@"
    if [ "$verbose" = "yes" ] ; then
        yell "running: $@"
    fi
    run "$@"
    echo "$output"
    [[ "$status" -eq 0 ]]
}

# dumpenv
#
# Echo shell environment settings.
function dumpenv () {
    env | grep = | sort
    # warn $BATS_TEST_FILENAME
    # warn $BATS_TEST_SOURCE    
}

# tojson
#
# A filter reading stdin for lines like:
#
#  key1=value1
#  key2=value2
#
# and will emit JSON text for the equivalent JSON object.
#
# This requires the "jq" program.
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

# topdir
#
# Emit the top level source directory.
#
# Requries this file (wct-bats.sh) to be in its place in the source.
function topdir () {
    local name=$BASH_SOURCE
    [[ -n "$name" ]]
    name="$(realpath $name)"
    [[ -n "$name" ]]
    name="$(dirname $name)"
    [[ -n "$name" ]]
    name="$(dirname $name)"
    [[ -n "$name" ]]
    echo "$name"
}    

# blddir
#
# Emit the biuld directory.
function blddir () {
    echo "$(topdir)/build"
}

# srcdir <pkg>
# 
# Emit the sub-package source directory of hte given pkg name.
function srcdir () {
    local path="$1" ; shift
    local maybe="$(topdir)/$path"
    if [ -d "$maybe" ] ; then
        echo "$maybe"
    fi
}


# saveout [options] <src> ...
#
# Save a file out of the test area to the build/tests/ area.
#
# options:
# -c, --category <cat>
#   Specify the category in which to save, default is "output".
#
# -t, --target <tgt>
#   Specify an explicit target file name to save.  Only the first
#   filename is saved out and <tgt> must be a relative path.
#
# <src>
#   The name of a file to save out.
#
# Files are saved to build/tests/<category>/<version>/<test-name>/
#
#   <category> is as given or defaults to "output"
#   <version> is the current wire-cell version
#   <test-name> is from BATS_TEST_FILENAME without the extension.
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

# skip_if_no_input <path> [...]
#
# Skip a test if the given path does not exist in the input area.
function skip_if_no_input () {
    local paths=( $@ )
    if [ -z "$paths" ] ; then
        paths=( "." )
    fi
    for path in ${paths[@]}
    do
        local input="$(blddir)/tests/input/$path"
        if [ ! -d "$input" ] ; then
            skip "no input test data at %input"
        fi
    done
}

# input_file <relpath> ...
#
# Emit full paths given relative paths in input category
function input_file () {
    local input="$(blddir)/tests/input"
    if [ ! -d "$input" ] ; then
        die "no input data"
    fi

    for path in $@
    do
        full="$input/$path"
        if [ -f "$full" ] ; then
            echo "$full"
            continue
        fi
        die "no such file or directory: $full"
    done
}

# downloads
# 
# Emit the download cache directory
function downloads () {
    local d="$(topdir)/downloads"
    mkdir -p "$d"
    echo $d
}

# wcb [options]
#
# Run the Wire-Cell build tool in the top directory.
function wcb () {
    local here="$(pwd)"
    cd "$(topdir)"
    ./wcb $@
    cd "$here"
}

# wcb_env_dump
#
# Emit all Waf environment variable settings.
#
# Note, these are not necessarily varibable settings of the shell
# environment.
function wcb_env_dump () {
    # Keep a cache if running in bats.
    if [ -n "${BATS_FILE_TMPDIR}" ] ; then
        local cache="${BATS_FILE_TMPDIR}/wcb_env.txt"
        if [ ! -f $cache ] ; then
            wcb dumpenv | grep 'wcb: ' | sed -e 's/^wcb: '// | sort > $cache
        fi
        cat $cache
        return
    fi

    # otherwise, do not keep cache as there is no good way to
    # invalidate it.  This suffers about 100ms per call.
    wcb dumpenv | grep 'wcb: ' | sed -e 's/^wcb: '// | sort
}    
    

# wcb_env_vars [variable] ...
#
# Emit key=var lines consisting of Waf build environment variables.
#
# With no arguments, emit all Waf variables, else emit only those named.
#
# User may run "./wcb dumpenv" to dump what is available.
function wcb_env_vars () {
    # Keep a cache if running in bats.
    if [ -z "$1" ] ; then
        wcb_env_dump
    fi
    for one in $@
    do
        grep "^${one}=" <(wcb_env_dump)
    done
}


# wcb_env_value <varname>
#
# Return the value for the Waf build environment variable named <varname>.
wcb_env_value () {
    wcb_env_vars $1 | sed -e 's/[^=]*=//' | sed -e 's/^"//' -e 's/"$//'
}


# wcb_env <varname> [...]
#
# Evaluate a Waf build environment variable to become a shell
# environment variable.
function wcb_env () {
    eval $(wcb_env_vars $@)
}


# usepkg <pkgname> [...]
#
# Set shell environment variables related to one or more named packages.
#
# This adds test program locations into PATH and defines a <pkg>_src
# variable pointing at the packages source directory.
function usepkg () {
    for pkg in $@; do
        local pkg=$1 ; shift
        local t=$(topdir)
        printf -v "${pkg}_src" "%s/%s" "$t" "$pkg"
        export ${pkg}_src
        PATH="$(blddir)/$pkg:$PATH"
    done
}

# compile_jsonnet <input> <output>
#
# Compile input Jsonnet file to output JSON file.
#
# Execute the Jsonnet compiler under Bats "run".
# Tries to use wcsonnet then falls back to jsonnet.
# The cfg dir is put into the search path and WIRECELL_PATH is ignored.
function compile_jsonnet () {
    local ifile="$1" ; shift
    local ofile="$1" ; shift
    local cfgdir="$(topdir)/cfg"

    local orig_wcpath=$WIRECELL_PATH
    WIRECELL_PATH=""

    local wcsonnet=$(wcb_env_value WCSONNET)
    [[ -n "$wcsonnet" ]]
    if [ -z "$wcsonnet" ] ; then
        yell "Failed to get WCSONNET!  Cache problem?"
        exit 1
    fi
    declare -a cmd=( "$wcsonnet" -o "$ofile" -P "$cfgdir" "$@" "$ifile" )

    echo "${cmd[@]}"
    run "${cmd[@]}"
    echo "$output"
    [[ "$status" -eq 0 ]]
    [[ -s "$ofile" ]]
    WIRECELL_PATH="$orig_wcpath"
}

# wct [options]
# 
# Execute the "wire-cell" CLI under Bats "run", echo output and assert
# non-zero return value.
function wct () {
    cli=$(wcb_env_value WIRE_CELL)
    [[ -n "$cli" ]]
    [[ -x "$cli" ]]

    echo "$cli $@"              # for output on failure
    run $cli $@
    echo "$output"
    [[ "$status" -eq 0 ]]    
}

# version
#
# Emit the wire-cell version string.
function version () {
    cli=$(wcb_env_value WIRE_CELL)
    [[ -n "$cli" ]]
    [[ -x "$cli" ]]
    $cli --version
}

# dotify_graph <input> <output>
#
# Output a GraphViz "dot" graph representation of the input.
#
# Input may be Jsonnet or JSON.
#
# Output may be PNG, PDF, DOT
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



# divine_context
#
# Emit the Bats context.  Will emit: "test", "file" or "run".
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


# tmpdir
#
# Emit the Bats temporary directory in the context.
#
# With no arguments, this divines the correct one for the current Bats context.
#
# With a single argument it will return the temporary directory for that context.
#
# Note, to not purge the tmp call:
#
#   bats --no-tempdir-cleanup
#
# Or, set WCTEST_TMPDIR.
function tmpdir () {
    if [ -n "$WCTEST_TMPDIR" ] ; then
        echo "$WCTEST_TMPDIR"
        return
    fi

    local loc="${1:-test}"
    local ret=""
    case $loc in
        run) ret="$BATS_RUN_TMPDIR";;
        file) ret="$BATS_FILE_TMPDIR";;
        *) ret="$BATS_TEST_TMPDIR";;
    esac
    if [ -z "ret" ] ; then
        ret="$(mktemp -d /tmp/wct-bats.XXXXX)"
        warn "Not running under bats, will not remove tempdir: $ret"
    fi
    echo $ret
}

# cd_tmp [context]
#
# Change to the context temporary directory.
#
# If a context is given, that temporary directory will be used
# otherwise it will be divined.
function cd_tmp () {
    ctx="$1"
    if [ -z "$ctx" ] ; then
        ctx="$(divine_context)"
    fi
    local t="$(tmpdir $ctx)"
    mkdir -p "$t"
    cd "$t"
}

# relative_path <path>
#
# Resolve a file path relative to the path of the BATS test file.
function relative_path () {
    local want="$1"; shift
    [[ -n "$BATS_TEST_FILENAME" ]]
    realpath "$(dirname $BATS_TEST_FILENAME)/$want"
}

# resolve_pathlist <pathlist> ...
#
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

# resolve_path <path> <pathlist> ...
#
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

# find_paths <path> <pathlist>
#
# Emit all matching paths.
#
# usage:
# resolve_all emacs $PATH
# /usr/bin/emacs
# /usr/local/bin/emacso
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


# config_path <path>
#
# Emit an absolute path of a relative path found in config areas.
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

# resolve_file <file>
#
# Emit the absolute path for a given path.
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
# - $(topdir)/build/tests/
# - $WIRECELL_PATH
#
# See resolve_path to resolve paths in a policy-free manner.
#
# See relative_path to resolve paths relative to bats test file.
#
# See category_path to resovle paths against specific data repo category.
function resolve_file () {
    local want="$1" ; shift

    [[ -n "$BATS_TEST_FILENAME" ]]
    local mydir="$(dirname ${BATS_TEST_FILENAME})"
    local t="$(topdir)"
    local paths=$(resolve_pathlist "$t/test/data" "$t" "$t/build/tests" ${WIRECELL_PATH})

    if [ -f "$want" -o -d "$want" ] ; then
        realpath "$want"
        return
    fi
    
    for path in ${paths[*]}
    do
        local maybe=$path/$want
        if [ -f "$maybe" -o -d "$maybe" ] ; then
            echo $maybe
            return
        fi
    done
    die "No such file or directory: $want"
}



# category_version_paths [options]
#
# Emit all versions paths found for category.
#
# options:
#
# -c, --category <cat>
#   Set the category.  Default is "history".
#
# -d, --dirty
#   If given, will match locally modified versions.
#
# Results are emitted in lexical order of version string.
function find_category_version_paths () {
    local category="history"
    # match "git describe --tags" 
    local matcher='+([0-9])\.+([0-9])\.+([x0-9])'
    local dirty=""
    while [[ $# -gt 0 ]] ; do
        case $1 in
            -c|--category) category="$2"; shift 2;;
            -d|--dirty) dirty="${matcher}-*"; shift;;
            *) break;;          # leave others args in place
        esac
    done

    local catdir="$(blddir)/tests/$category"

    declare -a vers
    declare -A paths

    for one in $(echo $catdir/$matcher)
    do
        if [ -d "$one" ] ; then
            local ver=$(basename $one)
            vers+=( $ver )
            paths[$ver]=$one
        fi
    done
    if [ -n "$dirty" ] ; then
        for one in $(echo $catdir/$dirty)
        do
            if [ -d "$one" ] ; then
                local ver=$(basename $one)
                vers+=( $ver )
                paths[$ver]=$one
            fi
        done
    fi
    for ver in $(printf '%s\n' ${vers[@]} | sort -u)
    do
        echo ${paths[$ver]}
    done
}

# find_category_version <path> ...
#
# Same as category_version_paths but emit only version string.
function find_category_versions () {
    for one in $(category_version_paths $@)
    do
        basename $one
    done
}

# category_path [options] <path> ...
#
# Resolve and emit path to a category path.
#
# options
#
# -c, --category <cat>
#   Set the category, default is "history".
#
# -v, --version <version>
#   Set the version, default uses current version.
#
# <path>
#   A relative path to resolve.
function category_path () {
    local cat="history"
    local ver=$(version)
    declare -a paths
    while [[ $# -gt 0 ]] ; do
        case $1 in
            -v|--version) ver="$2"; shift 2;;
            -c|--category) cat="$2"; shift 2;;
            -*) die "unknown argument: $1";;
            *) paths+=( "$1" ); shift;;
        esac
    done
            
    local catdir="$(blddir)/tests/$cat/$ver"

    for path in ${paths[*]}
    do
        if [ -f "$catdir/$path" -o -d "$catdir/$path" ] ; then
            echo "$catdir/$path"
        else
            die "No such file or directory: $catdir/$path"
        fi
    done
}
    

# historical_versions
#
# Emit the versions for which there are known historical files.
function historical_versions () {
    local tdv=$(wcb_env_value TEST_DATA_VERSIONS)
    printf '%s\n' $tdv
}

# historical_files [options] <path> ...
#
# Emit full paths to the files for a relative path across known history
#
# options:
#
# -v, --version <version>
#   Set the versions to consider.  Multiple -v options are allowed.
#
# -l, --last <number>
#   Set the oldest version to consider.
#
# -c, --current
#   Consider the version for the current software.
function historical_files () {
    local versions=( $(historical_versions) )
    local last=""
    declare -a paths
    while [[ $# -gt 0 ]] ; do
        case $1 in
            -l|--last) last="$2"; shift 2;;
            -v|--version) versions+=( $2 ); shift 2;;
            -c|--current) versions+=( $(version) ); shift;;
            -*) die "unknown option $1" ;;
            *) paths+=( $1 ); shift;;
        esac
    done
    
    versions=( $(printf '%s\n' ${versions[@]} | sort -u) )
    if [ -n "$last" ] ; then
        versions=( $(printf '%s\n' ${versions[@]} | tail -n $last ) )
    fi
    for ver in ${versions[@]}
    do
        # yell "historical files: $ver $@"
        category_path -c history -v $ver ${paths[@]}
    done
}

# skip_if_no_category [options] <category>
#
# Skip test if category is missing.
#
# options:
# -v, --version <version>
#   Set the version of the category, default to current version.
#
# <category>
#   Set the category.  Defaults to "history".
function skip_if_no_category () {
    local ver=$(version)
    local cat="history"
    while [[ $# -gt 0 ]] ; do
        case $1 in
            -v|--version) ver="$2"; shift 2;;
            *) category="$1"; shift;;
        esac
    done
    cdir="$(blddir)/tests/$cat/$ver"
    if [ -d "$cdir" ] ; then
        skip "no category: $cat/$ver"
    fi
}

    

# download_file <url> [<tgt>]
#
# Download a file from a URL.
#
# Return (echo) a path to the target file.  This will be a location in a cache.
# 
# <url>
#   The URL to download.
#
# <tgt>
#   An optional relative path to the file name the URL content should be saved.
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



function help_one () {
    local func="$1" ; shift
    cat "$0" | awk "/^# $func\>/{flag=1}/^function $func /{flag=0}flag" | sed -e 's/^ //g' -e 's/^#//g'
}

function help_all () {
    for func in $(grep ^function "$0" | sed -n 's/^function \(\S*\) .*/\1/p'|sort)
    do
        help_one $func | head -1
    done
}

if [[ "$(basename -- "$0")" == "wct-bats.sh" ]]; then
    if [ -z "$1" ] ; then
        help_all
    else
        for one in "$@" ; do
            help_one $one
        done
    fi
fi
