#!/usr/bin/env bats

# -*- shell-script -*-

bats_load_library "wct-bats.sh"

DETECTOR=uboone

function gen_fname () {
    local tier="$1" ; shift
    local num="$1" ; shift
    local ext="${1:-npz}"
    printf "%s-%s-%02.0f.%s" "$DETECTOR" "$tier" "$num" "$ext"
}


# FIXME
# 
# Eventually we will have N canonical directions.  Each direction is specified
# by two angles.  We flatten that 2D space to select the directions in the
# original MB SP-1 paper.  Each direction will be refered to by its bin number.
#
# We currently lack a command to generate the depos for a given track direction
# so for now fake it with "wirecell-gen depo-lines".  Instead of giving
# meaningful angles to this command we give a pair of numbers to use for seeds
# and the numbers are simply the direction bin number.
directions=$(seq 10)

function generate_depos ()
{
    local num="$1" ; shift
    local fname="$(gen_fname depos $num)"

    # FIXME: this needs to be replaced with a new command that generates
    # canonical depos
    warn "Using depo-lines instead of real depo generator"
    wcpy gen depo-lines -o "$fname" --seed $num

    # avoid empty zip of 22 bytes
    file_larger_than "$fname" 23

    # The eventual replacement for depo-lines should also produce a JSON file
    # that collects all metadata describing the depos.  Here we fake it.
    local jname="$(gen_fname params $num json)"
    cat <<EOF > $jname
{
  "num": $num
  "theta": $num,
  "phi": $num
}
EOF
}
# FIXME


@test "spdir metric uboone generate depos" {
    cd_tmp file
    for num in ${directions[*]}
    do
        generate_depos "$num"
    done
}

@test "spdir metric uboone sim+nf+sp" {
    cd_tmp file
    local cfg_file="$(relative_path spdir-metric.jsonnet)"
    for num in ${directions[*]}
    do
        depos="$(gen_fname depos "$num")"
        frames="$(gen_fname frames "$num")"
        log="$(gen_fname wct "$num" log)"
        wire-cell -c "$cfg_file" \
                  -l "$log" -L debug \
                  -A input="$depos" \
                  -A output="$frames" \
                  -A detector=$DETECTOR
    done
}


# FIXME
#
# We do not yet have a command to generate a metric from each frame file.
# When we do, fix this function.  For now, it does some bogosity.
#
# This function should consume params.json
function calcualte_metrics ()
{
    local jname="$(gen_fname metrics $num json)"
    cat <<EOF > $jname
{
  "num": $num
  "theta": $num,
  "phi": $num,
  "nchan": 0,
  "sigtot": 0,
  "eff": [],
  "bias": [],
  "res": []
}
EOF
    # This metrics.json should produce rich but summary data.  Eg, the
    # eff/bias/res should be arrays the hit channels.
}
# FIXME

@test "spdir metric uboone metrics" {
    cd_tmp file
    for num in ${directions[*]}
    do
        calcualte_metrics "$num"
    done
    
}


# FIXME: we also need a canonical plotter that consumes all metric.json.  
@test "spdir metric uboone plots" {
    cd_tmp file
    for num in ${directions[*]}
    do
        ls -l "$(gen_fname metrics $num json)" >> "$(gen_fname plots all txt)"
    done
}
