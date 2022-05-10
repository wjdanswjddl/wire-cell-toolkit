// This file produces a top-level function that returns a WCT
// configuration sequence.  It may be given directly to the wire-cell
// command line program or in principle used from art/FHiCL.

// The high level layer API provides one-stop shopping for writting
// "main" functions in a detector independent manner.
local high = import "layers/high.jsonnet";
  
function(detector, variant="nominal", depofile="depos.npz",
         output="tut-high-%(type)s-anode%(anode)d.npz") 

    // Certain detector specific options can be passed in.  These MUST
    // have save defaults and should only allow variations that an
    // end-user would decide in order to connect the core graph
    // produced by the API to some graph fringe (eg, I/O).
    local options={sparse:false};

    // Create the mid-level API object
    local mid = high.mid(detector, variant, options=options);

    // I/O nodes are provided by "helpers.jsonnet" to which high
    // provides access.
    local source = high.fio.depo_file_source(depofile);

    // The simulation starts with a single pipeline for drifting
    // depos.
    local drifter = mid.drifter();

    // Every detector+variant has well defined list of anodes.  Even
    // if a detector has a single anode (eg uboone) a list must be
    // returned.
    local anodes = mid.anodes();

    // We next build per-anode pipelines holding simuilation and sink
    // subgraphs.
    local apipes = [
        high.pg.pipeline([
            // The sim object has a variety of functions each to return a
            // subgraph that performs TPC simulation for one anode.  Three
            // functions are required: .signal() which only simulates signal
            // (no noise), .noise() which does the opposite and .full() which
            // includes both.  Additional functions may be provided in a
            // detector-specific manner.  The sim object may be iterated, eg
            // via std.objectFields(mid.sim) to discover all simulation
            // variants and so extraneous attributes should not be exposed.
            mid.sim.signal(anode),
            mid.sim.noise(anode),
            mid.sim.digitizer(anode),

            // The PDSP noise filtering
            mid.nf(anode),

            // The PDSP sigproc. The sparse option defaults to true
            // which would be useful for a sink that represents
            // sparseness.  For here, we rely on compression.
            mid.sp(anode),

            // Each sink file name is expected to have a %-format which is
            // interpolated on the anode ident number.
            high.fio.tap('FrameFanout',
                         high.fio.frame_file_sink(std.format(output, {
                             type:"frames", anode: anode.data.ident})),
                         std.toString(anode.data.ident)),

            mid.img(anode),

            high.fio.cluster_file_sink(std.format(output, {
                type:"clusters", anode: anode.data.ident})),

        ]),
        for anode in anodes];
  
    
    // Last, we construct the DFP graph from the subgraphs we have
    // collected.  The high gives us also the pgraph set of functions.
    local body = high.pg.fan.fanout('DepoSetFanout', apipes);
    local graph = high.pg.pipeline([source, drifter, body], "main");

    // Return final sequence.  Beside the default 'Pgrapher', we may
    // gain multi-threaded graph execution by passing 'TbbFlow'.
    high.main(graph, 'Pgrapher')

                 
                 

