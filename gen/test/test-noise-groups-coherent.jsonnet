// Make a coherent GroupNoiseModel "map file"
//
// Here we assume the test-noise-spectra.jsonnet with 10 groups and
// PDSP APA0 channel numbering.
//
// We simply stripe each plane with 10 coherent groups.

[
    {
        groupID: ind%10,
        channels: std.range(ind*10, ind*10 + 10 - 1),
    } for ind in std.range(0, 256-1)
]
