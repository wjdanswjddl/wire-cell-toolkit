// A switchard for getting known parameters in manner that tries to be
// uniform across all detectors.
//
// Any use of std.extVar() in low-level files will BREAK.  If you've
// added such then this switchyard will not work for your detector.
//
// Some detectors' low leve files are too variant and will also not
// work.
//
// Example use:
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

local frame_bounds = function(nchannels, nticks, first_chid=0, first_tick=0) {
    chbeg: first_chid, chend: first_chid+nchannels,
    tbbeg: first_tick, tbend: first_tick+nticks
};

local known = {
    "pdsp": {
        params: import "pgrapher/experiment/pdsp/params.jsonnet",
        simparams: import "pgrapher/experiment/pdsp/simparams.jsonnet",
        sp_filters: import "pgrapher/experiment/pdsp/sp-filters.jsonnet",
        chndb : import "pgrapher/experiment/pdsp/chndb-base.jsonnet",
        dense(anode) :: frame_bounds(2560, self.params.nticks, anode.data.ident*2560),
    },
    "dune10kt-1x2x6": {
        // warning, is violating norms and presents a function.
        params: import "pgrapher/experiment/dune10kt-1x2x6/params.jsonnet",
        simparams: import "pgrapher/experiment/dune10kt-1x2x6/simparams.jsonnet",
        sp_filters: import "pgrapher/experiment/dune10kt-1x2x6/sp-filters.jsonnet",
        chndb: import "pgrapher/experiment/dune10kt-1x2x6/chndb-base.jsonnet",
    },
    iceberg: {
        params: import "pgrapher/experiment/iceberg/params.jsonnet",
        simparams: import "pgrapher/experiment/iceberg/simparams.jsonnet",
        sp_filters: import "pgrapher/experiment/iceberg/sp-filters.jsonnet",
        chndb: import "pgrapher/experiment/iceberg/chndb-base.jsonnet",
    },
    uboone: {
        params: import "pgrapher/experiment/uboone/params.jsonnet",
        simparams: import "pgrapher/experiment/uboone/simparams.jsonnet",
        sp_filters: import "pgrapher/experiment/uboone/sp-filters.jsonnet",
        chndb_base: import "pgrapher/experiment/uboone/chndb-base.jsonnet",
        chndb_perfect: import "pgrapher/experiment/uboone/chndb-base.jsonnet",
        dense(anode) :: frame_bounds(8256, self.params.daq.nticks),
    },
    sbnd: {
        params: import "pgrapher/experiment/sbnd/params.jsonnet",
        simparams: import "pgrapher/experiment/sbnd/simparams.jsonnet",
        sp_filters: import "pgrapher/experiment/sbnd/sp-filters.jsonnet",
        chndb: import "pgrapher/experiment/sbnd/chndb-base.jsonnet",
    },
    "dune-vd-coldbox": {
        // warning, violates norms by requiring an external variable
        params: import "pgrapher/experiment/dune-vd-coldbox/params.jsonnet",
        simparams: import "pgrapher/experiment/dune-vd-coldbox/simparams.jsonnet",
        sp_filters: import "pgrapher/experiment/dune-vd-coldbox/sp-filters.jsonnet",
        chndb: import "pgrapher/experiment/dune-vd-coldbox/chndb-base.jsonnet",
    },
    icarus: {
        params: import "pgrapher/experiment/icarus/params.jsonnet",
        simparams: import "pgrapher/experiment/icarus/simparams.jsonnet",
        sp_filters: import "pgrapher/experiment/icarus/sp-filters.jsonnet",
        chndb: import "pgrapher/experiment/icarus/chndb-base.jsonnet",
    },
    "dune-vd": {
        // warning, is violating norms and presents a function.
        params: import "pgrapher/experiment/dune-vd/params.jsonnet",
        sp_filters: import "pgrapher/experiment/dune-vd/sp-filters.jsonnet",
        chndb: import "pgrapher/experiment/dune-vd/chndb-base.jsonnet",
    },
};

function(detector, params_type="params") {
    local parts = known[detector],
    local params = parts[params_type],
    ret: params {
        // config for the CNDB is such a mess
        chndb :: function(anode, fr) { 
            type: 'OmniChannelNoiseDB',
            name: anode.name,
            data: if detector == "uboone"
                  then parts.chndb_perfect(params, anode, fr)
                  else parts.chndb(params, anode, fr, anode.data.ident),
            uses: [anode, fr],  // dft?....
        },
        sp_filters: parts.sp_filters,
        dense: parts.dense,
    },    
}.ret


    
    
