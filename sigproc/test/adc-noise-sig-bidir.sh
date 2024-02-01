#!/bin/bash

# snakemake chokes an large graphs.  But the full graph has many smaller
# connected components for each input file.  We split that up by doing N input
# files at at time.  Here we need to know how many total inputs (1000)

mydir="$(dirname "$(realpath "$BASH_SOURCE")")"
log="adc-noise-sig-bidir.log"

date > $log
for num in $(seq 0 10 990)
do
    snakemake --config first=$num -j -s "$mydir/adc-noise-sig-bidir.smake" tier_render_all >> $log 2>&1
done
