// A function on an anode which gives factories for creating a
// configuration subgraph for imaging.

local util = import "util.jsonnet";
local pg = import "pgraph.jsonnet";
local wc = import "wirecell.jsonnet";

function (anode) {

    local ident = util.idents(anode),
    

    // The recommended slicer.  There is almost no reasonable default
    // to each detector variant may as well create the MaskSlices
    // directly.
    slicing :: function(tag, errtag, min_tbin=0, max_tbin=0, ext="",
                        span=4, active_planes=[0,1,2], masked_planes=[], dummy_planes=[]) 
        pg.pnode({
            type: "MaskSlices",
            name: ident+ext,
            data: {
                tag: tag,
                error_tag: errtag,
                tick_span: span,
                anode: wc.tn(anode),
                min_tbin: min_tbin,
                max_tbin: max_tbin,
                active_planes: active_planes,
                masked_planes: masked_planes,
                dummy_planes: dummy_planes,
            },
        }, nin=1, nout=1, uses=[anode]),


    // This slicing does not handle charge uncertainty nor bad channels.
    simple_slicing :: function(span=4, tag="") pg.pnode({
        type: "SumSlices",
        name: ident,
        data: {
            tag: tag,
            tick_span: span,
            anode: wc.tn(anode),
        },
    }, nin=1, nout=1, uses=[anode]),

    // A function that returns a tiling node for single faced anodes.
    // When used in the context of many tilings per anode face, a
    // unique name extension "ext" must be given.
    single_tiling :: function(face, ext="") pg.pnode({
        type: "GridTiling",
        name: "%s-%d%s"%[ident, face, ext],
        data: {
            anode: wc.tn(anode),
            face: face,
        }
    }, nin=1, nout=1, uses=[anode]),

    // A subgraph to do tiling for wrapped (two faced) anodes.  
    // When used in the context of many tilings per anode face, a
    // unique name extension "ext" must be given.
    wrapped_tiling :: function(ext="") {

        local slice_fanout = pg.pnode({
            type: "SliceFanout",
            name: ident+ext,
            data: { multiplicity: 2 },
        }, nin=1, nout=2),

        local tilings = [$.single_tiling(face) for face in [0,1]],

        local blobsync = pg.pnode({
            type: "BlobSetSync",
            name: ident+ext,
            data: { multiplicity: 2 }
        }, nin=2, nout=1),

        ret: pg.intern(
            innodes=[slice_fanout],
            outnodes=[blobsync],
            centernodes=tilings,
            edges=
                [pg.edge(slice_fanout, tilings[n], n, 0) for n in [0,1]] +
                [pg.edge(tilings[n], blobsync, 0, n) for n in [0,1]],
            name=ident+ext),
    }.ret,

    // Different tiling strategies.
    tiling : {
    
        // The "perfect" tiling aka "3-plane tiling" assumes all
        // channels are operational.  It produces one tiling node per
        // anode face.
        perfect :: function(ext="")
            if std.length(std.prune(anode.data.faces)) == 2 then
                $.wrapped_tiling(ext)
            else if std.type(anode.data.faces[0]) == "null" then
                $.single_tiling(1, ext)
            else
                $.single_tiling(0, ext),

        // The "mixed" tiling will add so called "2-plane" tiling with
        // signals from each plane ignored and instead with their dead
        // channels considered "fired" in addition to "perfect"
        // tiling.  This produces 4 tiling nodes per anode face.
        // mixed :: function(ext) ...
    },
    
    // A node that clusters blobs between slices w/in the given number
    // of slice spans.  Makes blob-blob edges
    clustering :: function(spans=1.0) pg.pnode({
        type: "BlobClustering",
        name: ident,
        data:  { spans : spans }
    }, nin=1, nout=1),    
    
    // A node that groups wires+channels into "measurements".  Makes
    // blob-measure edges.
    grouping :: function() pg.pnode({
        type: "BlobGrouping",
        name: ident,
        data:  { }
    }, nin=1, nout=1),

    // The recommended charge solving
    charge_solving :: function(meas_val_thresh=0.0,
                               meas_err_thresh=1.0e9,
                               blob_val_thresh=0,
                               whiten=true)
        pg.pnode({
            type: "ChargeSolving",
            name: ident,
            data:  {
                "meas_value_threshold": meas_val_thresh,
                "meas_error_threshold": meas_err_thresh,
                "blob_value_threshold": blob_val_thresh,
                "whiten": whiten,
            }
        }, nin=1, nout=1),

    // This solver is simplistic, prefer charge solving.
    blob_solving :: function(threshold=0.0) pg.pnode({
        type: "BlobSolving",
        name: ident,
        data:  { threshold: threshold }
    }, nin=1, nout=1),    

    // A function that projects blobs back to frames.  
    reframing :: function(tag="") pg.pnode({
        type: "BlobReframer",
        name: ident,
        data: {
            frame_tag: tag,
        }
    }, nin=1, nout=1),
}

