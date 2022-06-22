// Return a list of anodes
local wc = import "wirecell.jsonnet";
function(volumes, wires_file)
    local wires = {
        type:"WireSchemaFile",
        name: wires_file,
        data: {filename: wires_file}
    };
    [ {
        type: "AnodePlane",
        name: "%d" % vol.wires,
        data: {
            ident: vol.wires,
            wire_schema: wc.tn(wires),
            faces: vol.faces,
        },
        uses: [wires]
    } for vol in volumes]
