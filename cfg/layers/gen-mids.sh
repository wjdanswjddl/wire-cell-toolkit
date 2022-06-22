#!/usr/bin/env bash

# in-place generator for mids.jsonnet to save some typing.

cat <<EOF >mids.jsonnet
// This file is generated, do not edit.
// Generated on $(date) by $USER on $(hostname --fqdn)

{
EOF

for one in mids/*/mids.jsonnet
do
    det=$(basename $(dirname $one))
    echo "    $det : import \"$one\"," >> mids.jsonnet
done
echo "}" >> mids.jsonnet
    
