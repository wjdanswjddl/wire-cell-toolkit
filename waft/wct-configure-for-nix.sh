#!/bin/bash

usage () {
    cat <<EOF
Configure WCT source to build against Nix-provided packages.

You must already have sourced 

  $HOME/.nix-profile/etc/profile.d/nix.sh

or equivalent.

This script should be kept in the "waftools" git submodule of WCT.
But, it may be run from anywhwere:

  ./waftools/wct-configure-for-nix.sh [<path-to-install>]

 - <path-to-install> :: optional location for installing WCT.  It
   defaults to installing into the view.

Note: this assumes a Nix profile exists.  New packages will be added.

EOF
    exit 1
}

topdir=$(dirname $(dirname $(readlink -f $BASH_SOURCE)))
inst="${1:-$topdir/install-nix}"
echo "Will 'wcb install' to $inst"

if [ -z "$NIX_PATH" ] ; then
    echo "Nix not yet configured."
    nixsh="$HOME/.nix-profile/etc/profile.d/nix.sh"
    if [ -f "$nixsh"] ; then
        echo "source $nixsh"
    else
        echo "... and no nix profile even."
    fi
    exit 1
fi

view="$(dirname $(dirname $(which root-config)))"
if [ -z "$view" -o ! -d "$view" ] ; then
    echo "Fail to find nix profile directory: $view"
    exit 1
fi

assure_packages () {
    echo "Assuring packages.  This may take some time"
    # To get "dev" outputs from multi-ouput packages requires some hoop jumping
    for devpkg in fftwFloat boost  zlib
    do
        echo "Assuring dev package: $pkg"
        echo
        nix-env -i -E '_: with import <nixpkgs> {}; let newmeta = ( '$devpkg'.meta // { outputsToInstall = ["out" "dev"]; } ); in '$devpkg' // { meta = newmeta; }' || exit 1
        echo
    done
    for pkg in gcc python jsonnet jsoncpp eigen root tbb
    do
        echo "Assuring package: $pkg"
        echo
        nix-env -iA nixpkgs.$pkg
        echo
    done
}
# assure_packages


# fixme: would like to test clang too...
export CC=`which gcc`
export CXX=`which g++`

# needed to shut up eigen 3.2.10
export CXXFLAGS='-Wno-misleading-indentation -Wno-int-in-bool-context -Wvla'

"$topdir/wcb" \
    configure  \
    --with-jsoncpp="$view" \
    --with-jsonnet="$view" \
    --with-eigen-include="$view/include/eigen3" \
    --with-fftw="$view" \
    --with-zlib="$view" \
    --with-root="$view" \
    --boost-includes="$view/include" \
    --boost-libs="$view/lib" \
    --boost-mt \
    --with-tbb=$view \
    --prefix="$inst"


#


