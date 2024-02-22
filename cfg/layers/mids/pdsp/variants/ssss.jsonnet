local wc = import "wirecell.jsonnet";

local nominal = import "nominal.jsonnet";

local override = nominal + {
    ductor: super.ductor + {
        tbin : 0, 
        binning: {
            tick : $.binning.tick,
            nticks : $.ductor.tbin + $.binning.nticks,
        },
        start_time : -self.response_plane / $.lar.drift_speed,
        readout_time : self.binning.nticks * self.binning.tick,
    }
};

local smeared = override + {
    splat: super.splat + {

        // Run wirecell-gen morse-* to find these numbers that match the extra
        // spread the sigproc induces.  CAVEAT: these results are inderectly
        // tied to the field response used.
        "smear_long": [
            2.691862363980221,
            2.6750200122535057,
            2.7137567141154055
        ],
        "smear_tran": [
            0.7377218875719689,
            0.7157764520393882,
            0.13980698710556544
        ]        
    }
};


{
    // Used for test-morse-pdsp
    morse_nominal: override,

    // Used for test-ssss-pdsp
    ssss_nominal: override,
    ssss_smeared: smeared,

    // Used for test/scripts/spdir 
    spdir_lofr: smeared {
        detector: super.detector {
            fields: "pdsp-fields-lo.json.bz2"
        },
    },
    spdir_hifr: smeared {
        detector: super.detector {
            fields: "pdsp-fields-hi.json.bz2"
        },
    },
}
