function(params) {
    sp: {
        type: "FieldResponse",
        name: "sp",
        data: { filename: params.sp.field_file }
    },
    nf: {
        type: "FieldResponse",
        name: "nf",
        data: { filename: params.nf.field_file }
    },
    sims: [{
        type: "FieldResponse",
        name: fn,
        data: { filename: fn }
    } for fn in params.ductor.uboone_field_files],
}

