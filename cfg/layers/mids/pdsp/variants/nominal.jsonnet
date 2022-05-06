// This is the base for *pdsp* variant parameter objects. 

// Do NOT place any "if" in this file!  If a new variant is needed,
// make an empty, new file in this directory and either import/inherit
// from this structure overiding only the new values.  Do not
// copy-paste.  Refactor this file if needed to achieve breif
// variants.

local wc = import "wirecell.jsonnet";

{
    // Define the LAr properties.  In principle, this is NOT subject
    // to variance though in principle it could be (eg, top/bottom of
    // DUNE-VD may have different LAr temps?).  We include this here
    // to keep data out of the the mid code and because this info is
    // used in a few places.
    lar : {
        // Longitudinal diffusion constant
        DL : 4.0 * wc.cm2/wc.s,
        // Transverse diffusion constant
        DT : 8.8 * wc.cm2/wc.s,
        // Electron lifetime
        lifetime : 10.4*wc.ms,
        // Electron drift speed, assumes a certain applied E-field
        drift_speed : 1.565*wc.mm/wc.us, // at 500 V/cm
        // LAr density
        density: 1.389*wc.g/wc.centimeter3,
        // Decay rate per mass for natural Ar39.
        ar39activity: 1*wc.Bq/wc.kg,
    },

    // Extract all the location for planes used in geometry
    planes : {
        apa_cpa: 3.63075*wc.m, // LArSoft
        // apa_cpa = 3.637*wc.m, // DocDB 203
        cpa_thick: 3.175*wc.mm, // 1/8", from Bo Yu (BNL) and confirmed with LArSoft
        // cpa_thick = 50.8*wc.mm, // DocDB 203
        apa_w2w: 85.725*wc.mm, // between W planes, DocDB 203
        apa_g2g: 114.3*wc.mm, 
        // measured from grid to "anode" plane along drift direction
        offset: 4.76*wc.mm
    },

    // This function returns the minimal geometry needed by WCT.  In
    // the transverse plane the geometry is defined by the "wires
    // file" and in the drift direction by the "drifts" structure
    // which defines a trio of planes: "anode", "response" and
    // "cathode" which are perpendicular to drift.  Each are assumed
    // approximately equipotential along their plane.  If your
    // detector fails this, then use a "variant params" pack for each
    // detector region which does meet this requirement.
    // 
    geometry : {

        // The exhaustive list of the location of every single wire
        // (or strip) segment.
        wires_file: "protodune-wires-larsoft-v4.json.bz2",

        // Comments on how to chose the "anode" plane location: The
        // "anode" cut off plane, here measured from APA centerline,
        // determines how close to the wires do we consider any depo.
        // Anything closer will simply be discarded, else it will
        // either be drifted or "backed up" to the response plane.
        // This is somewhat arbitrary choice.  The "backing up" assume
        // parallel drift points and so some small error is made when
        // applied to ionization produce very close to the wires.  To
        // reduce this small error the "anode" plane can be moved
        // closer to the "response" plane but at the cost of not
        // simulate the volume left beyond the "anode" plane.

        // local apa_plane = 0.5*apa_g2g, // put anode plane at grid plane
        local apa_plane = 0.5*$.planes.apa_g2g - $.planes.offset, // put anode plane at first induction plane

        // Response plane MUST be located so that the wire plane
        // locations used in the FR calculation matches the wire plane
        // locations given to WCT.
        local res_plane = 0.5*$.planes.apa_w2w + $.ductor.response_plane,

        // The cathode plane is like the anode cut off plane.  Any
        // depo not between the two is dropped prior to drifting.  It
        // is demarks the face of the CPA.
        local cpa_plane = $.planes.apa_cpa - 0.5*$.planes.cpa_thick,

        // The drifts are then defined in terms of these above
        // numbers.  You can use "wirecell-util wires-info" or
        // "wirecell-util wires-volumes" or others to understand the
        // mapping of anode number to the 6 locations in global X and
        // Z coordinates.  For Larsoft wires the numbering is column
        // major starting at small X and Z so the centerline is
        // -/+/-/+/-/+.  Also important is that the faces are listed
        // "front" first.  Front is the one with the more positive X
        // coordinates and if we want to ignore a face it is made
        // null.
        drifts : [{
            local sign = 2*(n%2)-1,
            local centerline = sign*$.planes.apa_cpa,
            wires: n,       // anode number
            name: "apa%d"%n,
            faces:
                // top, front face is against cryo wall
                if sign > 0
                then [
                    {
                        anode: centerline + apa_plane,
                        response: centerline + res_plane,
                        cathode: centerline + cpa_plane, 
                    },
                    {
                        anode: centerline - apa_plane,
                        response: centerline - res_plane,
                        cathode: centerline - cpa_plane, 
                    }
                ]
                // bottom, back face is against cryo wall
                else [
                    {
                        anode: centerline + apa_plane,
                        response: centerline + res_plane,
                        cathode: centerline + cpa_plane, 
                    },
                    {
                        anode: centerline - apa_plane,
                        response: centerline - res_plane,
                        cathode: centerline - cpa_plane, 
                    }

                ],
        } for n in std.range(0,5)],
    },

    // The nominal time binning for data produced by the detector.
    binning : {
        tick: 0.5*wc.us,
        nticks: 6000,
    },


    // The electronics response parameters.
    elec : {
        // The FE amplifier gain in units of Voltage/Charge.
        gain : 14.0*wc.mV/wc.fC,

        // The shaping (aka peaking) time of the amplifier shaper.
        shaping : 2.2*wc.us,

        // A realtive gain applied after shaping and before ADC.
        // 
        // Don't use this to fix wrong sign depos.  If you neeed to
        // fix sign of eg larsoft depos its input component has a
        // "scale" parameter which can be given a -1.  Or better, fix
        // the C++ that sets the wrong sign to begin with.
        // 
        // Pulser calibration: 41.649 ADC*tick/1ke
        // theoretical elec resp (14mV/fC): 36.6475 ADC*tick/1ke
        postgain: 1.1365,
    },

    // The "RC" response
    rc : {
        width: 1.1*wc.ms,
    },

    // The ductor transforms drifted depos to currents
    ductor: {

        field_file: "dune-garfield-1d565.json.bz2",

        // The distance from the anode centerline to where the field
        // response calculation begins drifting.  Take care that field
        // response calculations are not always done with the
        // thickness of the anode in mind and may report their
        // "response plane" measured relative to a number of other
        // monuments.  This value is almost certainly required in the
        // "geometry" section.
        response_plane : 10*wc.cm,

        // The "absolute" time (ie, in G4 time) that the lower edge of
        // of final readout tick #0 should correspond to.  This is a
        // "fixed" notion.
        local tzero = -250*wc.us,

        // Simulate extra time so we can see the tail of response
        // from activity from depos that fall outside the readout
        // window.  This amount of extra is somewhat arbitrary but
        // the duration of the field response calculation is a
        // good measure.
        local pre = self.response_plane / $.lar.drift_speed,

        // The time bin where the readout should be considered to
        // actually start given the pre-signal simulated.
        tbin : wc.roundToInt(pre / self.binning.tick),
        binning: {
            // Ductor ticks at same speed as ADC
            tick : $.binning.tick,
            // Over the somewhat enlarged domain
            nticks : $.ductor.tbin + $.binning.nticks,
        },
        start_time : tzero - pre,
        readout_time : self.binning.nticks * self.binning.tick,
    },

    // Simulating noise
    noise : {
        spectra_file: "protodune-noise-spectra-v1.json.bz2",
        // These are frequency space binning which are not necessarily
        // same as some time binning - but here they are.
        period: $.binning.tick,     // 1/frequency
        nsamples: $.binning.nticks, // 
        // Optimize binning 
        wire_length_scale: 1.0*wc.cm,
        // Optimize use of randoms
        replacement_percentage: 0.02,
    },

    // Digitization model parameters.  
    digi : {
        // A relative gain applied just prior to digitization.  This
        // is not FE gain, see elec for that.
        gain: 1.0,

        // Voltage baselines added to any input voltage signal listed
        // in a per plan (U,V,W) array.
        //
        // Per tdr, chapter 2
        // induction plane: 2350 ADC, collection plane: 900 ADC
        baselines: [1003.4*wc.millivolt,1003.4*wc.millivolt,507.7*wc.millivolt],

        // The resolution (bits) of the ADC
        resolution: 12,

        // The voltage range as [min,max] of the ADC, eg min voltage
        // counts 0 ADC, max counts 2^resolution-1.
        //
        // The tdr says, "The ADC ASIC has an input
        // buffer with offset compensation to match the output of the
        // FE ASIC.  The input buffer first samples the input signal
        // (with a range of 0.2 V to 1.6 V)..."
        fullscale: [0.2*wc.volt, 1.6*wc.volt],
    },

    // The noise filter parameter pack
    nf : {
        // In principle, may use a different field response.
        field_file : $.ductor.field_file,

        binning: $.binning,

        // The name of the ChannelNoiseDb
        chndb: "actual",
        filters: {
            channel: ["single"], // sticky, single, gaincalib
            grouped: [],         // grouped,
            status: [],
        }
    },

    // Signal processing parameter pack
    sp: {
        // In principle, may use a different field response.
        field_file : $.ductor.field_file,
        binning : $.binning,
    }

}
    
