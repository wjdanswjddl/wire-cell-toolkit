// Produce config for WC 3D imaging subgraph

local low = import "../../../low.jsonnet";
local pg = low.pg;
local wc = low.wc;

function(services, params) function(anode)
    local ident = low.util.idents(anode);

