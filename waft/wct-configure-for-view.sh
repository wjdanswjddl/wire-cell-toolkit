#!/bin/bash

usage () {
    cat <<EOF
Configure WCT source for a "view" providing external dependencies.

This script should be kept in the "waftools" git submodule of WCT.
But, it may be run from anywhwere:

  ./waftools/wct-configure-for-view.sh <path-to-view> [<path-to-install>]

 - <path-to-view> :: the file system path to the top of the view directory.

 - <path-to-install> :: optional location for installing WCT.  It
   defaults to installing into the view.

Note: A likely way to create a "view" directory is with Spack:

  spack view add -i /opt/spack/views/wct-dev wirecell-toolkit

EOF
    exit 1
}

view="$1"
if [ -z "$view" ] ; then
    usage
fi
inst="${2:-$view}"
echo "Will 'wcb install' to $inst"

topdir=$(dirname $(dirname $(readlink -f $BASH_SOURCE)))

"$topdir/wcb" \
    configure \
    --with-jsoncpp="$view" \
    --with-jsonnet="$view" \
    --with-tbb="$view" \
    --with-eigen-include="$view/include/eigen3" \
    --with-root="$view" \
    --with-fftw="$view" \
    --boost-includes="$view/include" \
    --boost-libs="$view/lib" \
    --boost-mt \
    --with-tbb=false \
    --prefix="$inst"


#--with-fftw-include="$view/include" \
#--with-fftw-lib="$view/lib" \

cat <<EOF
# For runtime setup, copy-paste:
PATH=$inst/bin:$view/bin:\$PATH
export LD_LIBRARY_PATH=$inst/lib:$view/lib:\$LD_LIBRARY_PATH

# or if preferred:
source $view/bin/thisroot.sh
source $view/bin/geant4.sh

# and this may be needed
export ROOT_INCLUDE_PATH=$view/include
EOF




