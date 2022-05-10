// return a mid-level API object

local low = import "../../low.jsonnet";
local sim = import "api/sim.jsonnet";
local sigproc = import "api/sigproc.jsonnet";
local img = import "api/img.jsonnet";

// Create a mid API object.  No options supported.
function(services, params, options={}) {

    anodes :: function()
        low.anodes(params.geometry.drifts, params.geometry.wires_file),

    drifter :: function(name="")
        low.drifter(services.random,
                    low.util.driftsToXregions(params.geometry.drifts),
                    params.lar, name=name),

    // track_depos, signal, noise, digitizer
    sim :: sim(services, params),

    // nf, sp, dnnroi
    sigproc :: sigproc(services, params, options),

    img :: img(services, params),
}

    
