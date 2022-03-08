// This makes a job that sends an empty frame down through imaging to assure nothing crashes.

local wc = import "wirecell.jsonnet";
local pg = import "pgraph.jsonnet";

local params = import "pgrapher/experiment/uboone/simparams.jsonnet";

// this can maybe be moved to a generic spot
local hs = import "pgrapher/common/helpers.jsonnet";

local tools_maker = import 'pgrapher/common/tools.jsonnet';
local tools = tools_maker(params);

local wires = hs.aux.wires(params.files.wires);
local anodes = hs.aux.anodes(wires, params.det.volumes);

local src = pg.pnode({
    type: "SilentNoise",
    data: {
        noutputs: 10,
    },        
}, nin=0, nout=1);

local anode = anodes[0];

local imgpipe(anode) = pg.pipeline([
    hs.img(anode).nominal(),
    hs.io.cluster_file(anode.data.ident,
                       "test-empty-frame-clusters-%d.tar.bz2"%anode.data.ident)
], "img-" + anode.name);

local graph = pg.pipeline([src, imgpipe(anodes[0])], "main");
local app = {
  type: 'Pgrapher',
  data: {
    edges: pg.edges(graph),
  },
};

local cmdline = {
    type: "wire-cell",
    data: {
        plugins: ["WireCellGen", "WireCellPgraph", "WireCellSio", "WireCellSigProc", "WireCellRoot", "WireCellImg"],
        apps: ["Pgrapher"]
    }
};

[cmdline] + pg.uses(graph) + [app]
