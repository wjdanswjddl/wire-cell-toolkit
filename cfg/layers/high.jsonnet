// The "high level" layer returns an API object given a detector name
// and optional variant name.


// A lookup by detector and variant.  See gen-mids.sh.
local mids = import "mids.jsonnet";
local svcs = import "high/svcs.jsonnet";

local midapi = import "midapi.jsonnet";



{
    // Forward the various "standard" wirecell utillities as a
    // convenience to end user.
    wc :: import "wirecell.jsonnet",
    pg :: import "pgraph.jsonnet",

    // Ways to move data between file and WCT.
    fio :: import "high/fileio.jsonnet",

    // This gives main(graph), which produces the final WCT
    // configuration sequence.
    main :: import "high/main.jsonnet",

    // The service pack factory function.  By default, the default
    // service pack is passed in to the mid factory function.  If a
    // non-default service pack is wanted, the caller should call
    // services() with some options and pass the result when creating
    // the mid.
    services :: svcs,

    // The mid-level API factory function.
    mid :: function(detector, variant="nominal", services=svcs(), options={})
        midapi + mids[detector](services, variant, options=options),
}


