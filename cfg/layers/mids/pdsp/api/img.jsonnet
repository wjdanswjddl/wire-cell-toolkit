// Produce config for WC 3D imaging subgraph

local low = import "../../../low.jsonnet";
local pg = low.pg;
local wc = low.wc;

function(services, params) function(anode)
    local ident = low.util.idents(anode);
    local img = low.img(anode);

    low.pg.pipeline([
        img.slicing(params.img.span),
        img.tiling[params.img.tiling_strategy](),
        img.clustering(),
        img.grouping(),
        img.charge_solving(),   // fixme: a few optins we may want to allow to specify in variant params
    ], ident)

