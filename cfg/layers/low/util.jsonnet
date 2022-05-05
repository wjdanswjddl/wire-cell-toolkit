{
    // Extract and return unique plane trio objects aka xregions from
    // an array of "drifts".
    driftsToXregions :: function(vols)
        std.set(std.prune(std.flattenArrays([v.faces for v in vols])),
            function(o) std.toString(o)),
}
