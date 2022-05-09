// This file is part of WCT "high layer" configuration structure.  It
// provides a "main()" function used to return the final WCT
// configuration sequence given a previously constructed graph.

local pg = import "pgraph.jsonnet";

local basic_plugins = [
    "WireCellSio", "WireCellAux",
    "WireCellGen", "WireCellSigProc", "WireCellImg", 
    "WireCellApps"];

local app_plugins = {
    'TbbFlow': ["WireCellTbb"],
    'Pgrapher': ["WireCellPgraph"],
};

function(graph, app='Pgrapher', extra_plugins = [])
    local plugins = std.set(basic_plugins + extra_plugins + app_plugins[app]);
    local appcfg = {
        type: app,
        data: {
            edges: pg.edges(graph)
        },
    };
    local cmdline = {
        type: "wire-cell",
        data: {
            plugins: plugins,
            apps: [appcfg.type]
        }
    };
    [cmdline] + pg.uses(graph) + [appcfg]

