// The "high level" layer returns an API object given a detector name
// and optional variant name.


// A lookup by detector and variant.  See gen-mids.sh.
local mids = import "mids.jsonnet";
local midapi = import "midapi.jsonnet";

local svcs = import "svcs.jsonnet";

{
    // Forward the various "standard" wirecell utillities as a
    // convenience to end user.
    wc :: import "wirecell.jsonnet",
    pg :: import "pgraph.jsonnet",
    hs :: import "pgrapher/common/helpers.jsonnet",

    // The service pack factory function.  By default, the default
    // service pack is passed in to the mid factory function.  If a
    // non-default service pack is wanted, the caller should call
    // services() with some options and pass the result when creating
    // the mid.
    services :: svcs,

    // The mid-level API factory function.
    mid :: function(detector, variant="nominal", services=svcs())
        midapi + mids[detector](services, variant),
}


