#!/bin/bash

tstdir=$(dirname $(realpath $BASH_SOURCE))
laydir=$(dirname $tstdir)
cfgdir=$(dirname $laydir)

mkdir -p $tstdir/tmp
# tmpdir=$(mktemp -d $tstdir/tmp/tut-high.XXXXX)
tmpdir=$tstdir/tmp

tutjson=$tmpdir/tut-high.json
oldsrc="$cfgdir/pgrapher/experiment/pdsp/wcls-sim-drift-simchannel.jsonnet"
oldjson=$tmpdir/old.json

set -x
#if [ ! -f $tutjson ] ; then
    jsonnet -o $tutjson -J $cfgdir $laydir/tut-high.jsonnet -A detector=pdsp 
#fi
if [ ! -f $oldjson ] ; then
    jsonnet -J $cfgdir \
            --ext-code DL='4.0' --ext-code DT='8.8' \
            --ext-code lifetime='10.4' --ext-code driftSpeed='1.565' \
            $oldsrc > $oldjson
fi
set +x

classes=( $(jq -r '.[].type' < $tutjson | sort -u) )


for class in ${classes[*]}
do
    if [ "$class" = "Pgrapher" -o "$class" = "TbbFlow" ] ; then
        continue;
    fi
    old=$(cat $oldjson | jq '[.[]|select(.type == "'$class'")]')
    new=$(cat $tutjson | jq '[.[]|select(.type == "'$class'")]')
    if [ "$old" = "[]" ] ; then
        echo "Old has no $class"
        continue
    fi
    if [ "$new" = "[]" ] ; then
        echo "new has no $class"
        continue
    fi

    diff --color -u \
         <(echo $old |gron|grep -v '\.name'|sed -e 's/:[^"]*"/"/') \
         <(echo $new |gron|grep -v '\.name'|sed -e 's/:[^"]*"/"/') 
    echo "type: $class"
done 


echo "$tmpdir"

