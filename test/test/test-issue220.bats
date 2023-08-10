#!/usr/bin/env bats

# We eradicate the trace tag "threshold" and put its trace summary on
# "wiener".

bats_load_library wct-bats.sh

# bats file_tags=issue:220

@test "no wiener_threshold_tag in cfg" {

    local cfgdir=$(srcdir cfg)

    local found=$( grep -r wiener_threshold_tag $cfgdir )
    echo -e "found with wiener_threshold_tag:\n$found"
    [[ -z "$found" ]]
    
}

@test "no threshold tags in cfg - this fails until wclsFrameSaver issue resolved" {

    local cfgdir=$(srcdir cfg)

    local found1=$( grep -r "'threshold'" $cfgdir )
    echo -e "found with 'threshold' tags: \n$found1"
    [[ -z "$found1" ]]
    
    local found2=$( grep -r '"threshold"' $cfgdir )
    echo -e "found with \"threshold\" tags:\n$found2"
    [[ -z "$found2" ]]
    
}

