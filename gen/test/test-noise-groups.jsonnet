// Make a GroupNoiseModel "map file"

function(ngroups=10, nper=10) [ {
    groupID: grp,
    channels: std.range(grp, grp+nper-1),
} for grp in std.range(0, ngroups-1)]

