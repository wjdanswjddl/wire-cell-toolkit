#!/bin/bash

set -x

tstdir="$(dirname $(realpath $BASH_SOURCE))"
auxdir="$(dirname $tstdir)"
topdir="$(dirname $auxdir)"
docdir="$auxdir/docs"

blddir="$topdir/build"
gendir="$topdir/gen"


wirecell-pgraph dotify \
    $gendir/test/test-noise-roundtrip.jsonnet \
    $docdir/test-noise-roundtrip-flow-graph.svg || exit 1

wirecell-sigproc plot-noise-spectra \
                 $gendir/test/test-noise-spectra.jsonnet \
                 $blddir/test-noise-spectra-in-all10.pdf || exit 1

wirecell-sigproc plot-noise-spectra \
                 -A ngrps=1  \
                 $gendir/test/test-noise-spectra.jsonnet \
                 $docdir/test-noise-spectra-in-%d.svg

wire-cell -l $blddir/test-noise-roundtrip.log -l stderr -L debug \
          -c $gendir/test/test-noise-roundtrip.jsonnet || exit 1


wirecell-sigproc \
    plot-noise-spectra \
    test-noise-roundtrip-inco-spectra.json.bz2 \
    inco-spectra.pdf || exit 1

wirecell-sigproc \
    plot-noise-spectra \
    test-noise-roundtrip-cohe-spectra.json.bz2 \
    cohe-spectra.pdf || exit 1
