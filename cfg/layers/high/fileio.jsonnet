// This provides the high.fio module and consists of
// detector-independent methods to produce configuration for WCT
// components that move data between files and a larger WCT graph.

// A source or a sink takes a filename as first arg and this is used
// to provide a name for the component instance.

// Abslolutely NO detector-dependent code may exist in this (or any
// other "high layer") file.

local pg = import "pgraph.jsonnet";
local wc = import "wirecell.jsonnet";

{

    // A source of depos from file. Several file formats supported.
    depo_file_source(filename, scale=1.0) ::
        pg.pnode({
            type: 'DepoFileSource',
            name: filename,
            data: { inname: filename, scale: scale }
        }, nin=0, nout=1),

    // A sink of depos.  Several file formats supported
    depo_file_sink(filename) ::
        pg.pnode({
            type: 'DepoFileSink',
            name: filename,
            data: { outname: filename }
        }, nin=1, nout=0),

    // If a frame is written out "dense" it the sink needs to know
    // what the bounds are in channel IDs (rows) and ticks (columns).
    frame_bounds(nchannels, nticks, first_chid=0, first_tick=0) :: {
        chbeg: first_chid, chend: first_chid+nchannels,
        tbbeg: first_tick, tbend: first_tick+nticks
    },

    /// Sink a stream of frames to file.  The tags give an array of
    /// tags to look for in the frame and save.  If empty or if a "*"
    /// tag is given then all traces in the frame are saved with a
    /// name derived from "*".
    frame_file_sink(filename, tags=[],
                    digitize=false, dense=null) :: 
        pg.pnode({
            type: "FrameFileSink",
            name: filename,
            data: {
                outname: filename,
                tags: tags,
                digitize: digitize,
                dense: dense,
            },
        }, nin=1, nout=0),
        
    /// Source a stream of frames from file.
    frame_file_source(filename, tags=[]) ::
        pg.pnode({
            type: "FrameFileSource",
            name: filename,
            data: {
                inname: filename,
                tags: tags,
            },
        }, nin=0, nout=1),


    // A cluster file is a tar stream, with optional compression,
    // holding json, dot or numpy format files.
    cluster_file_sink :: function(filename, format="json", prefix="cluster") pg.pnode({
        type: 'ClusterFileSink',
        name: filename,
        data: {
            outname: filename,
            format: format,
            prefix: prefix,
        },
    }, nin=1, nout=0),

    // A somewhat slippery format.
    celltree_file_source :: function(filename, recid, 
                                     branches = ["calibWiener", "calibGaussian"],
                                     frame_tags=["gauss"],
                                     trace_tags = ["wiener", "gauss"])
        pg.pnode({
            type: "CelltreeSource",
            name: filename,
            data: {
                filename: filename,
                EventNo: std.toString(recid), // note, must NOT be a number but a string....
                frames: frame_tags,
                "in_branch_base_names": branches,
                "out_trace_tags": trace_tags,
            },
        }, nin=0, nout=1),


    // Adapt a sink to "tap" pattern via a 2-way fanout.  First output
    // port forwards input, second output port forwards to the sink.
    // The type should be a fanout type such as FrameFanout, etc.
    tap :: function(type, sink, tag_rules=[])
        local fanout = pg.pnode({
            type: type,
            name: pg.pname(sink),
            data: {
                multiplicity: 2,
                tag_rules: tag_rules,
            },
        }, nin=1, nout=2);
        pg.intern(innodes=[fanout],
                  outnodes=[fanout],
                  centernodes=[sink],
                  edges=[pg.edge(fanout, sink, 1, 0)],
                  name=pg.pname(sink)),

}
