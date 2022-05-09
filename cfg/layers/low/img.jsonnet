// A function on an anode which gives factories for creating a
// configuration subgraph for imaging.

local util = import "util.jsonnet";

function (anode) {

    local ident = util.idents(anode),

    // The recommended slicer.  
    mask_slicing :: function(span=4, tag="") pg.pnode({
        type: "MaskSlices",
        name: name,
        data: {
            tag: tag,
            tick_span: span,
            anode: wc.tn(anode),
        },
    }, nin=1, nout=1, uses=[anode]),
    

    // This slicing does not handle charge uncertainty.
    simple_slicing :: function(span=4, tag="") pg.pnode({
        type: "SumSlices",
        name: name,
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

        local slice_fanout = function(face) pg.pnode({
            type: "SliceFanout",
            name: "%s-%d%s"%[ident, face, ext],
            data: { multiplicity: 2 },
        }, nin=1, nout=2),

        local tilings = [single_tiling(face) for face in [0,1]],

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
    
    // Switch between wrapped or single face tiling config
    tiling :: function(ext="")
        if std.length(std.prune(anode.data.faces)) == 2 then
            $.wrapped_tiling(ext)
        else if std.type(anode.data.faces[0]) == "null" then
            $.single_tiling(1, ext)
        else
            $.single_tiling(0, ext),
        
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
    charge_solving :: function(meas_val_thresh=10.0,
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
        name: name,
        data: {
            frame_tag: tag,
        }
    }, nin=1, nout=1),
