#!/usr/bin/env bats
# -*- shell-script -*-

# Caution: this file is fully generated.
# Any edits are subject to loss.
# Instead, edit spdir-metric.tmpl.

bats_load_library "wct-bats.sh"

DETECTOR=pdsp

function gen_fname () {
    local tier="$1" ; shift
    local num="$1" ; shift
    local ext="${1:-npz}"
    printf "%s-%s-%02.0f.%s" "$DETECTOR" "$tier" "$num" "$ext"
}


# FIXME
# 
# Eventually we will have N canonical directions.  Each direction is specified
# by two angles (theta_y, theta_xz).  We flatten that 2D space to select the
# directions in the original MB SP-1 paper.  Each direction will be refered to
# by its index in the sequence that matches the x axis bins.
#
# We currently lack a command to generate the depos for a given track direction
# so for now fake it with "wirecell-gen depo-lines".  Instead of giving indices
# into a sequence of meaningful angles simply pass the index as the random seed.
#
# Eventually we have about a dozen bins but for now, keep it short to speed
# things up.
directions=$(seq 3)

function generate_depos ()
{
    local num="$1" ; shift
    local fname="$(gen_fname depos $num)"
    local pname="$(gen_fname params $num json)"

    # FIXME: this needs to be replaced with a new command that generates
    # canonical depos
    warn "Using depo-lines instead of real depo generator"
    wcpy gen depo-lines --track-info $pname -o "$fname" --seed $num

    # avoid empty zip of 22 bytes
    file_larger_than "$fname" 23
}
# FIXME


@test "spdir metric pdsp generate depos" {
    cd_tmp file
    for num in ${directions[*]}
    do
        generate_depos "$num"
    done
}

@test "spdir metric pdsp sim+nf+sp one job graph viz" {
    cd_tmp file
    local cfg_file="$(relative_path spdir-metric.jsonnet)"
    local num="${directions[0]}"
    local depos="$(gen_fname depos "$num")"
    # save out the intermediate drifted depos for input to metric 
    local drifts="$(gen_fname drifts "$num")"
    local splats="$(gen_fname splats "$num")"
    local frames="$(gen_fname frames "$num")"
    local pdf="$(gen_fname graph "$num" pdf)"
    local log="$(gen_fname wct "$num" log)"
    wirecell-pgraph dotify \
                    -A input="$depos" \
                    -A output="{drift:\"$drifts\",splat:\"$splats\",sp:\"$frames\"}" \
                    -A detector=$DETECTOR \
                    "$cfg_file" "$pdf"
}

@test "spdir metric pdsp sim+nf+sp run job" {
    cd_tmp file
    local cfg_file="$(relative_path spdir-metric.jsonnet)"
    for num in ${directions[*]}
    do
        local depos="$(gen_fname depos "$num")"
        # save out the intermediate drifted depos for input to metric 
        local drifts="$(gen_fname drifts "$num")"
        local splats="$(gen_fname splats "$num")"
        local frames="$(gen_fname frames "$num")"
        local log="$(gen_fname wct "$num" log)"
        wire-cell -c "$cfg_file" \
                  -l "$log" -L debug \
                  -A input="$depos" \
                  --tla-code output="{drift:\"$drifts\",splat:\"$splats\",sp:\"$frames\"}" \
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

@test "spdir metric pdsp metrics" {
    cd_tmp file
    for num in ${directions[*]}
    do
        calcualte_metrics "$num"
    done
    
}


# FIXME: we also need a canonical plotter that consumes all metric.json.  
@test "spdir metric pdsp plots" {
    cd_tmp file
    for num in ${directions[*]}
    do
        ls -l "$(gen_fname metrics $num json)" >> "$(gen_fname plots all txt)"
    done
}
