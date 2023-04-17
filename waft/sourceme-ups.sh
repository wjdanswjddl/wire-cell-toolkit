#!/bin/bash

# source sourceme-ups.sh [version [qual]]

# This sets up a bash shell to use UPS products to supply WCT build
# and runtime environment.  For building see wct-configure-for-ups.sh
# in this same directory.  Although Wire Cell Toolkit does not depend
# on LArSoft, the environment is configured by piggy-backing on a
# "larsoft" UPS product of the given version and qualifiers.

version=${1:-v06_76_00}
quals=${2:-e15:prof}

source /cvmfs/larsoft.opensciencegrid.org/products/setup

setup larsoft $version -q $quals

export CXX=clang++
export CC=clang

# installed system git on Fermilab systems is too old so also set it up.
setup git


srcdir=$(dirname $(dirname $(readlink -f $BASH_SOURCE)))

export WIRECELL_DATA=$srcdir/cfg
echo "You will need to add your wire-cell-data directory to \$WIRECELL_DATA (currently $WIRECELL_DATA)"
