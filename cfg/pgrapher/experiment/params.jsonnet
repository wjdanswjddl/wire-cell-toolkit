// A switchard for known parameters.
//
// This can be used to resolve params name by higher level code.
//
// Do not use std.extVar().  Instead, use top-level arguments.
//
// local params_lookup = import "pgrapher/experiment/params.jsonnet";
// local hs = import "pgrapher/common/helpers.jsonnet";
//
// function(detector, pname="simparams") {
// local params = params_lookup(detector, pname),
// ...the rest written detector-agnostically using helpers...
// }
//
// # wcsonnet -A detector=pdsp my-top-cfg.jsonnet

local known = {
    "pdsp": {
        params: import "pgrapher/experiment/pdsp/params.jsonnet",
        simparams: import "pgrapher/experiment/pdsp/simparams.jsonnet",
    },
    "dune10kt-1x2x6": {
        // warning, is violating norms and presents a function.
        params: import "pgrapher/experiment/dune10kt-1x2x6/params.jsonnet",
        simparams: import "pgrapher/experiment/dune10kt-1x2x6/simparams.jsonnet"
    },
    iceberg: {
        params: import "pgrapher/experiment/iceberg/params.jsonnet",
        simparams: import "pgrapher/experiment/iceberg/simparams.jsonnet",
    },
    uboone: {
        params: import "pgrapher/experiment/uboone/params.jsonnet",
        simparams: import "pgrapher/experiment/uboone/simparams.jsonnet",
    },
    sbnd: {
        params: import "pgrapher/experiment/sbnd/params.jsonnet",
        simparams: import "pgrapher/experiment/sbnd/simparams.jsonnet",
    },
    "dune-vd-coldbox": {
        // warning, violates norms by requiring an external variable
        params: import "pgrapher/experiment/dune-vd-coldbox/params.jsonnet",
        simparams: import "pgrapher/experiment/dune-vd-coldbox/simparams.jsonnet",
    },
    icarus: {
        params: import "pgrapher/experiment/icarus/params.jsonnet",
        simparams: import "pgrapher/experiment/icarus/simparams.jsonnet",
    },
    "dune-vd": {
        // warning, is violating norms and presents a function.
        params: import "pgrapher/experiment/dune-vd/params.jsonnet",
    },
};

function(detector, params="params") known[detector][params]


    
    
