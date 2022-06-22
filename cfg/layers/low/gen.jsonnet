// Low layer API to kinematic generator related
local pg = import "pgraph.jsonnet";
local wc = import "wirecell.jsonnet";

{
    // Internally generated depos from ideal line tracks.
    track_depos(tracks, name="", step=1.0*wc.mm) :: pg.pnode({
        type: "TrackDepos",
        name: name,
        data: {
            step_size: step,
            tracks: tracks,
        }
    }, nin=0, nout=1),

    // Collect individual depos into a set with time in window at
    // offset.
    bagger(window, toffset=0) :: pg.pnode({
        type:'DepoBagger',
        data: {
            gate: [toffset, toffset+window],
        },
    }, nin=1, nout=1),

}
