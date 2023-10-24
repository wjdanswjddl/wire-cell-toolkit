// The DNNROI subgraph is non-trivial but only depends on a few
// parameters.  This returns it.

local util = import "util.jsonnet";
local wc = import "wirecell.jsonnet";
local pg = import "pgraph.jsonnet";

// Translate from services platform name to TorchContext devname.
// Fixme: need to rationalize the configuration given potential
// sharing of the underlying TorchContext with the Torch based DFT.
local plat2dev = {
    cpu: "cpu",
    torchcpu: "cpu",
    torchgpu: "gpu",
    torchgpu0: "gpu0",
    torchgpu1: "gpu1",
    // ...
};

function(anode, model, platform='cpu', output_scale=1.0, concurrency=1)
    local ident = util.idents(anode);
    local prename = "dnnroi"+ident;

    // This tag spelling is baked in!
    local intags = ['loose_lf' + ident,
                    'mp2_roi' + ident,
                    'mp3_roi' + ident];

    local ts = {
        type: "TorchService",
        name: platform,
        data: {
            model: model,
            device: plat2dev[platform],
            concurrency: 1,
        },
    };

    local dnnroi_u = pg.pnode({
        type: "DNNROIFinding",
        name: prename+"u",
        data: {
            anode: wc.tn(anode),
            plane: 0,
            intags: intags,
            decon_charge_tag: "decon_charge" + ident,
            outtag: "dnnsp" + ident + "u",
            output_scale: output_scale,
            forward: wc.tn(ts)
        }
    }, nin=1, nout=1, uses=[ts, anode]);

    local dnnroi_v = pg.pnode({
        type: "DNNROIFinding",
        name: prename+"v",
        data: {
            anode: wc.tn(anode),
            plane: 1,
            intags: intags,
            decon_charge_tag: "decon_charge" + ident,
            outtag: "dnnsp"+ident+"v",
            output_scale: output_scale,
            forward: wc.tn(ts)
        }
    }, nin=1, nout=1, uses=[ts, anode]);

    local dnnroi_w = pg.pnode({
        type: "PlaneSelector",
        name: prename+"w",
        data: {
            anode: wc.tn(anode),
            plane: 2,
            tags: ["gauss" + ident],
            tag_rules: [{
                frame: {".*":"DNNROIFinding"},
                trace: {["gauss" + ident]:"dnnsp"+ident+"w"},
            }],
        }
    }, nin=1, nout=1, uses=[anode]);

    local dnnpipes = [dnnroi_u, dnnroi_v, dnnroi_w];
    local dnnfanout = pg.pnode({
        type: "FrameFanout",
        name: prename,
        data: {
            multiplicity: 3
        }
    }, nin=1, nout=3);

    local dnnfanin = pg.pnode({
        type: "FrameFanin",
        name: prename,
        data: {
            multiplicity: 3,
            tag_rules: [{
                frame: {".*": "dnnsp" + ident},
                trace: {".*": "dnnsp" + ident},
            } for plane in ["u", "v", "w"]]
        },
    }, nin=3, nout=1);
    
    pg.intern(innodes=[dnnfanout],
              outnodes=[dnnfanin],
              centernodes=dnnpipes,
              edges=[pg.edge(dnnfanout, dnnpipes[ind], ind, 0) for ind in [0,1,2]] +
              [pg.edge(dnnpipes[ind], dnnfanin, 0, ind) for ind in [0,1,2]])
