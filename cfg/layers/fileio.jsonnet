// This provides the high.fio module and consists of
// detector-independent methods to produce configuration for WCT
// components that move data between files and a larger WCT graph.

// Abslolutely NO detector-dependent code may exist in this (or any
// other "high layer") file.

local pg = import "pgraph.jsonnet";
local wc = import "wirecell.jsonnet";

{
    // A source of depos from file.  This picks a format-specific
    // source component based on filename extension.
    depo_file_source(filename, name="", scale=1.0) ::
        if std.endsWith(filename, ".npz")
        then $.depo_npz_source(name, filename)
        else $.depo_tar_source(name, filename, scale),

    // The "preferred" depo file source
    depo_tar_source(name, filename, scale=1.0) ::
        pg.pnode({
            type: 'DepoFileSource',
            name: name,
            data: { inname: filename, scale: scale }
        }, nin=0, nout=1),

    // The "deprecated" depo file source
    depo_npz_source(name, filename, loadby="set") ::
        if loadby == "set"
        then pg.pnode({
            type: 'NumpyDepoSetLoader',
            name: name,
            data: {
                filename: filename
            }
        }, nin=0, nout=1)
        else pg.pnode({
            type: 'NumpyDepoLoader',
            name: name,
            data: {
                filename: filename
            }
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


    // Like a frame_file_sink but pass input frames both to output
    // port 0 and to a sink.
    frame_file_tap(filename, tags=[], digitize=false, dense=null) ::
        pg.fan.tap('FrameFanout', 
                   $.frame_file_sink(filename, 
                                     tags=tags,
                                     digitize=digitize,
                                     dense=dense), filename),

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

}
