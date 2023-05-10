// This documents the "cluster graph schema" and "cluster array
// schema".

local moo = import "moo.jsonnet";
local cs = moo.oschema.schema("wirecell.cluster");
local hier = {
    desc: cs.number("Desc", "u4", doc="A graph vertex descriptor"),
    ident: cs.number("Ident", "i4", doc="Type dependent identifier"),

    val: cs.number("Value", "f8", doc="Central value of a signal"),
    unc: cs.number("Uncertainty", "f8", doc="Uncertainty in a signal value"),

    index: cs.number("Index", "u4", doc="Index unique in some per-type context"),
    wpid: cs.number("WPID", "i4", doc="Packed wire plane identifier"),
    node_code: cs.string("Code", doc="Node type code"),
                                              
    coord: cs.number("Coord", "f8", doc="A coordinate in some space"),
    point: cs.record("Point", [
        cs.field("x", self.coord),
        cs.field("y", self.coord),
        cs.field("z", self.coord)
    ], doc="A point in Cartesian space"),

    time: cs.number("Time", "f8", doc="A temporal coordinate"),

    signal: cs.record("Signal", [
        cs.field("val", self.val),
        cs.field("unc", self.unc),
    ], doc="Common base to nodes objects with signal representation"),

    wire: cs.record("Wire", [
        cs.field("index", self.index),
        cs.field("wpid", self.wpid),
        cs.field("chid", self.ident),
        cs.field("seg", self.index),
        cs.field("tail", self.point),
        cs.field("head", self.point),
    ], doc="A wire node object"),

    channel: cs.record("Channel", [
        cs.field("index", self.index),
        cs.field("wpid", self.wpid),
    ], doc="A channel node object"),

    # minItems=3, maxItems=12
    corners: cs.sequence("Corners", self.point, doc="Blob corner points"),

    range: cs.record("Range", [
        cs.field("beg", self.index),
        cs.field("end", self.index),
    ], doc="A half-open integer range"),
    bounds: cs.sequence("Bounds", self.range, doc="Range for each wire plane"),
        
    blob: cs.record("Blob", [
        cs.field("faceid", self.wpid, doc="Packed WirePlaneId with face and anode numbers"),
        cs.field("sliceid", self.ident),
        cs.field("start", self.time),
        cs.field("span", self.time),
        cs.field("bounds", self.bounds), 
        cs.field("corners", self.corners),
    ], bases=[self.signal], doc="A blob node object"),

    measure: cs.record("Measure", [
        cs.field("wpid", self.wpid),
    ], bases=[self.signal], doc="A measure node object"),

    channel_activity: cs.record("ChannelActivity", [
        cs.field("ident", self.ident, doc="channel ident"),
    ], bases=[self.signal], doc="Amount of signal in one channel"),
    channel_activities: cs.sequence("Activities", self.channel_activity),

    slice: cs.record("Slice", [
        cs.field("frameid", self.ident),
        cs.field("start", self.time),
        cs.field("span", self.time),
        cs.field("signal", self.channel_activities),
    ], doc="A slice node object"),

    node_object: cs.anyOf("NodeObject", [
        self.blob, self.channel, self.measure, self.slice, self.wire
    ], doc="The node object"),

    node: cs.record("Node", [
        cs.field("type", self.node_code, doc="Identify the type of the node"),
        cs.field("ident", self.ident, doc="The vertex identifier"),
        cs.field("data", self.node_object),
    ], doc="A cluster graph node"),
    nodes: cs.sequence("Nodes", self.node),

    edge: cs.record("Edge", [
        cs.field("tail", self.index, doc="Index into nodes array of edge tail"),
        cs.field("head", self.index, doc="Index into nodes array of edge head"),
        // data....
    ], doc="A cluster graph edge"),
    edges: cs.sequence("Edges", self.edge, doc="Set of graph edges"),


    cluster: cs.record("Cluster", [
        cs.field("edges", self.edges),
        cs.field("nodes", self.nodes),
    ], doc="A cluster graph"),

};
moo.oschema.sort_select(hier, "wirecell.cluster")
