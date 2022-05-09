// return a mid-level API object

local low = import "../../low.jsonnet";
local sim = import "api/sim.jsonnet";
local nf = import "api/nf.jsonnet";
local sp = import "api/sp.jsonnet";
local img = import "api/img.jsonnet";

function(services, params, options) {

    anodes :: function()
        low.anodes(params.geometry.drifts, params.geometry.wires_file),

    drifter :: function(name="")
        low.drifter(services.random,
                    low.util.driftsToXregions(params.geometry.drifts),
                    params.lar, name=name),

    // signal, noise, digitizer
    sim :: sim(services, params),

    nf :: nf(services, params),

    sp :: sp(services, params),

    img :: img(services, params),
}

    
