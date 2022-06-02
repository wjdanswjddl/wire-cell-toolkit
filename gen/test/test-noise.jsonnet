// This tests noise simulation using a fictional.

// Note, this file is for a fictional detector to be self-contained.
// It is not a good example of a configuration file for real
// detectors.  See cfg/layers/ for the preferred approach.

local wc = import "wirecell.jsonnet";
local pg = import "pgraph.jsonnet";

local nsamples = 200;

// normally these MUST be give to the various components to "use" but
// as special case, default types need only be inserted in the final
// configuration sequence.
local services = [
    { type: "Random" },
    { type: "FftwDFT" },
];

local absurd = pg.pnode({
    type: 'SilentNoise',
    data: {
        noutputs: 1,
        nchannels: 100,
    }}, nin=0, nout=1);

local incomodel = {
    type: "GroupNoiseModel",
    name: "incoherent",
    data: {
        spectra_file: "test-noise-spectra.jsonnet",
        // To generate this map file run:
        // wcsonnet -S ngroups=2 -S nper=50 \
        //   gen/test/test-noise-groups.jsonnet 
        //   > test-noise-groups-incoherent.json
        map_file: "test-noise-groups-incoherent.json",
        nsamples: nsamples,
        tick: wc.us,
    }};
local cohemodel = {
    type: "GroupNoiseModel",
    name: "coherent",
    data: {
        spectra_file: "test-noise-spectra.jsonnet",
        // Default generates 10x10
        map_file: "test-noise-groups.jsonnet",
        nsamples: nsamples,
        tick: wc.us,
    }};

local incoadd = pg.pnode({
    type: 'IncoherentAddNoise', // aka AddNoise
    data: {
        model: wc.tn(incomodel),
        nsamples: nsamples,
    }}, nin=1, nout=1, uses=[incomodel]);

local coheadd = pg.pnode({
    type: 'CoherentAddNoise', // aka AddNoise
    data: {
        model: wc.tn(cohemodel),
        nsamples: nsamples,
    }}, nin=1, nout=1, uses=[cohemodel]);

local sinks = { [one]: pg.pnode({
    local fname="test-noise-%s.tar"%one, // fixme, use npz
    type: "FrameFileSink",
    name: fname,
    data: {
        outname: fname,
        tags: "",
        digitize: false,
    },
}, nin=1, nout=0) for one in ["inco","both"]};

local tap = pg.fan.tap('FrameFanout', sinks.inco, name="inco");

local graph = pg.pipeline([absurd, incoadd, tap, coheadd, sinks.both]);

local app = {
    type: 'Pgrapher',
    data: {
        edges: pg.edges(graph),
    },
};
local cmdline = {
    type: "wire-cell",
    data: {
        plugins: ["WireCellGen", "WireCellApps", "WireCellPgraph", "WireCellSio"],
        apps: app.type
    }
};
[cmdline] + services + pg.uses(graph) + [app]

                                 
