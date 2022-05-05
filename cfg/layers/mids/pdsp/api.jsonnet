// return a mid-level API object

local low = import "../../low.jsonnet";
local sim = import "api/sim.jsonnet";

function(services, params) {

    anodes :: function()
        low.anodes(params.geometry.drifts, params.geometry.wires_file),

    drifter :: function(name="")
        low.drifter(services.random,
                    low.util.driftsToXregions(params.geometry.drifts),
                    params.lar, name=name),

    // returns an object of methods taking "anode"
    sim :: sim(services, params),

}

    
