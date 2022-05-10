// This provides the high.fio module and consists of
// detector-independent methods to produce configuration for WCT
// components that move data between files and a larger WCT graph.

// Abslolutely NO detector-dependent code may exist in this (or any
// other "high layer") file.

local pg = import "pgraph.jsonnet";
local wc = import "wirecell.jsonnet";

{
    // Reminder to use this to convert a sink to a filter.
    // tap('FrameFanout', frame_sink, "name").
    tap :: pg.fan.tap,

    // A source of depos from file. Several file formats supported.
    depo_file_source(filename, name="", scale=1.0) ::
        pg.pnode({
            type: 'DepoFileSource',
            name: name,
            data: { inname: filename, scale: scale }
        }, nin=0, nout=1),

    // If a frame is written out "dense" it the sink needs to know
    // what the bounds are in channel IDs (rows) and ticks (columns).
    frame_bounds(nchannels, nticks, first_chid=0, first_tick=0) :: {
        chbeg: first_chid, chend: first_chid+nchannels,
        tbbeg: first_tick, tbend: first_tick+nticks
    },

    /// Sink a stream of frames to file. 
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
    cluster_file_sink :: function(name, filename=null, format="json", prefix="cluster") pg.pnode({
        type: 'ClusterFileSink',
        name: name,
        data: {
            outname: if std.type(filename) == "null" then "clusters-"+name+".tar.bz2" else filename,
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
            data: {
                filename: filename,
                EventNo: std.toString(recid), // note, must NOT be a number but a string....
                frames: frame_tags,
                "in_branch_base_names": branches,
                "out_trace_tags": trace_tags,
            },
        }, nin=0, nout=1),

}
