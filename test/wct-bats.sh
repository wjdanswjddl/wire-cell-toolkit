#!/usr/bin/env bash

# BATS helper functions.
# To use inside a <pkg>/test/test_XXX.bats do:
# load ../../test/wct-bats.sh

# Return the top level source directory
function top () {
    dirname $(dirname $(realpath $BASH_SOURCE))
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
