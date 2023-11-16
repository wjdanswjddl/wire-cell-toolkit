// This file is used by adc-noise-sig-bidir.smake.
//
// This generates noise, adds it to ADC voltage level input from file
// and runs NF+SP.
// 
// See "wirecell-util npz-to-wct" as one way to produce ADC voltage
// level from ADC array.
//
// This is a variant of adc-noise-sig.jsonnet which adds TLA control over which
// intermediate file taps are applied.

local high = import "layers/high.jsonnet";
local wc = high.wc;
local pg = high.pg;

function(adcvoltfile, spfrfile,sig,
         aid="0",
         detector="pdsp", variant="nominal",
         adcname="*", signame="gauss0",
         src="", ref="", dig="", noi=""
)

    local options = {params: {sp:{field_file:spfrfile},
                              nf:{field_file:spfrfile} }};
    local mid = high.mid(detector, variant, options=options);
    local anodes = mid.anodes();
    local anode = anodes[std.parseInt(aid)];

    local source = high.fio.frame_file_source(adcvoltfile, tags=[adcname]);
    local sink = high.fio.frame_file_sink(sig, tags=[signame],
                                          dense={chbeg:0,chend:800,tbbeg:0,tbend:6000});

    local tap_src = if src == ""
                    then []
                    else [ high.fio.tap('FrameFanout',
                                        high.fio.frame_file_sink(src, tags=[])) ];
    local tap_ref = if ref == ""
                    then []
                    else [ high.fio.tap('FrameFanout',
                                        high.fio.frame_file_sink(ref, tags=[])) ];

    local tap_noi = if noi == ""
                    then []
                    else [ high.fio.tap('FrameFanout',
                                        high.fio.frame_file_sink(noi, tags=[])) ];

    local tap_dig = if dig == ""
                    then []
                    else [ high.fio.tap('FrameFanout',
                                        high.fio.frame_file_sink(dig, tags=[])) ];


    local graph = pg.pipeline(
      [ source ] + tap_src +
      [ mid.sim.reframer(anode) ] + tap_ref +
      [ mid.sim.noise(anode) ] + tap_noi +
      [ mid.sim.digitizer(anode) ] + tap_dig +
      [ mid.sigproc.nf(anode),
        mid.sigproc.sp(anode),
        sink ], "main");

    local executor = "TbbFlow";
    high.main(graph, executor)

