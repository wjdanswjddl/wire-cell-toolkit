#!/usr/bin/env bash

# BATS helper functions.
# To use inside a <pkg>/test/test_XXX.bats do:
# load ../../test/wct-bats.sh

# Return the top level source directory
top () {
    dirname $(dirname $(realpath $BASH_SOURCE))
}    

# Add package (util, gen, etc) test apps into PATH, etc.  define
# <pkg>_src to be that sub-package source directory.
usepkg () {
    for pkg in $@; do
        local pkg=$1 ; shift
        printf -v "${pkg}_src" "%s/%s" "$t" "$pkg"
        local t=$(top)
        PATH="$t/build/$pkg:$PATH"
    done
    echo $PATH
}
