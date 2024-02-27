// This file defines the base "mid" layer API.
//
// It is a FLAT OBJECT parameterized by a set of service objects, detector
// params and an option object.  It's attributes are largely functions.
//
// People looking to configure specific detectors should likely NOT TOUCH IT. 
//
// Instead, work on mids/<detector>/api.jsonnet which provides overrides.
//
// Some functions are "pure virtual" in that a detector must implement.  Others
// provide usable defaults only require a detector override if they are not
// suitable.
//
// Always use single ":" even for functions.

local low = import "low.jsonnet";
local wc = low.wc;
local pg = low.pg;
local idents = low.util.idents;

function(services, params, options={})
{
    anodes : function()
        low.anodes(params.geometry.drifts, params.geometry.wires_file),

    // A node that returns a depo set drifter.
    drifter : function(name="")
        low.drifter(services.random,
                    low.util.driftsToXregions(params.geometry.drifts),
                    params.lar, name=name),

    // DepoSet to voltage Frame.
    signal : function(anode, tags=[]) 
        std.assertEqual("signal" == "implemented"),
    
    // Sparse Frame to dense Frame
    reframer : function(anode, tags=[], name=null)
        low.reframer(params, anode, tags=[], name=name),

    // Quiet voltage Frame to noisy voltage Frame.
    noise : function(anode)
        std.assertEqual("signal" == "implemented"),
    
    // Voltage Frame to ADC Frame. 
    digitizer : function(anode)
        pg.pnode({
            type: 'Digitizer',
            name: idents(anode),
            data: params.digi {
                anode: wc.tn(anode),
                frame_tag: "orig" + idents(anode),
            }
        }, nin=1, nout=1, uses=[anode]),
    
    // Pull out the responses as a short hand variable.
    local sim_res = low.resps(params).sim,

    // DepoSet to signal Frame (an ersatz sim+SP)
    splat : function(anode, name=null)
        pg.pnode({
            type: 'DepoFluxSplat',
            name: if std.type(name) == "null" then idents(anode) else name,
            data: params.splat {
                anode: wc.tn(anode),
                field_response: wc.tn(sim_res.fr), // note, only uses (period, origin)
            },
        }, nin=1, nout=1, uses=[anode, sim_res.fr]),

    // noisy ADC frame to quiet ADC frame
    nf : function(anode) 
        std.assertEqual("nf" == "implemented"),

    // ADC frame to signal frame
    sp : function(anode) 
        std.assertEqual("sp" == "implemented"),

    // Signal frame to better signal Frame. 
    dnnroi : function(anode)
        low.dnnroi(anode, params.dnnroi.model_file,
                   services.platform, params.dnnroi.output_scale),

    // frame to blob set
    img : function(anode) 
        std.assertEqual("img" == "implemented"),

}
