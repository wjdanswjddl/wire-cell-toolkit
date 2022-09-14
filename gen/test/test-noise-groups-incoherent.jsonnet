// Make an incoherent GroupNoiseModel "map file".
//
// Here we assume the test-noise-spectra.jsonnet with 10 groups and
// PDSP APA0 channel numbering.
//
// We make each plane channel range use a different spectra.
[
    // Note the "plane" attribute is not standard and shoud/must be
    // ignored.
    {
        plane: "u",
        groupID: 9,             // noisiest
        channels: std.range(0, 800-1),
    },
    {
        plane: "v",
        groupID: 5,
        channels: std.range(800, 800 + 800-1),
    },
    {
        plane: "w",
        groupID: 0,             // quietest
        channels: std.range(800 + 800, 800 + 800 + 960-1),
    },
]    
