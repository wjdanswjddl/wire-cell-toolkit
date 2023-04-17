#!/bin/bash

mydir=$(dirname $(dirname $(readlink -f $BASH_SOURCE)))

source $HOME/.nix-profile/etc/profile.d/nix.sh

for maybe in "$mydir/install-nix"
do
    if [ -d "$maybe" ] ; then
        echo "Located likely wire-cell toolkit installation: $maybe"
        export LD_LIBRARY_PATH=$maybe/lib
        PATH=$maybe/bin:$PATH
        break
    fi
done

echo "Starting subshell with Nix Python packages enabled, exit when done."
nix-shell -p python27Packages.matplotlib \
          -p python27Packages.virtualenv \
          -p python27Packages.jsonnet \
          -p python27Packages.click 
echo "Exiting Nix Python enabled subshell"

