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


# Assure a file is larger than given number of bytes
#
# usage:
# file_larger_than <filename> <nbytes>
function file_larger_than () {
    local filename=$1 ; shift
    local minsize=$1; shift
    [[ -f "$filename" ]]
    [[ -n "$minsize" ]]
   [[ "$(stat -c '%s' $filename)" -gt "$minsize" ]]
}


# Run command if any target file is missing or older than any source file.
#
# usage:
#   run_idempotently [-s/--source <srcfile>] [-t/--target <dstfile>] -- <command line>
#
# May give multiple -s or -t args
function run_idempotently () {
    declare -a src
    declare -a tgt

    while [[ $# -gt 0 ]] ; do
        case $1 in
            -s|--source) src+=( $2 ) ; shift 2;;
            -t|--target) tgt+=( $2 ) ; shift 2;;
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
        for one in ${tgt[@]}    
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
    # yell "running: $@" 
    run "$@"
    [[ "$status" -eq 0 ]]
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

# Skip test if no input 
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

# Return full paths given relative paths in input category
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

function wcb_env_dump () {
    # Keep a cache if running in bats.
    if [ -n "${BATS_RUN_TMPDIR}" ] ; then
        local cache="${BATS_RUN_TMPDIR:-/tmp}/wcb_env.txt"
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
    # Keep a cache if running in bats.
    if [ -z "$1" ] ; then
        wcb_env_dump
    fi
    for one in $@
    do
        grep "^${one}=" <(wcb_env_dump)
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

    echo "$cmd"                 # for output on failure
    # yell "$cmd"
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

    echo "$cli $@"              # for output on failure
    run $cli $@
    echo "$output"
    [[ "$status" -eq 0 ]]    
}

# Emit version string
function version() {
    cli=$(wcb_env_value WIRE_CELL)
    [[ -n "$cli" ]]
    [[ -x "$cli" ]]
    $cli --version
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
    [[ -n "$BATS_TEST_FILENAME" ]]
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



# Emit all versions paths found for category.
#
# usage:
# category_version_paths [-c/--category <cat>] [-d/--dirty]
#
# Default category is "history".
# 
# Default matches only release versions, -d/--dirty will also match locally modified versions
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

 # Same as category_version_paths but emit only version string.
function find_category_versions () {
    for one in $(category_version_paths $@)
    do
        basename $one
    done
}

# Emit the category path for the version
#
# usage:
# category_path [-c/--category <cat>] [-v/--version <version>] path [path ...]
#
# If no category given, "history" is assumed.
# If no version given, current version used.
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
    

# Emit the versions for which there are known historical files.
#
function historical_versions () {
    local tdv=$(wcb_env_value TEST_DATA_VERSIONS)
    printf '%s\n' $tdv
}

# Emit full paths to the files for a relative path across known history
#
# usage:
# historical_files [-v/--version <version>] [-l/--last <number>] <path> [<path> ...]
#
# One or more -v/--version options can add versions to the list of historical versions.
#
# A -l/--last number will return only the more recent versions (lexical sort)  
#
# A -c/--current will include the current software version
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

# Skip test if category is missing.
#
# usage:
# skip_if_no_category [-v/version <version>] <category>
#
# If no version given, use current version.
# If not category given, use "history"
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

