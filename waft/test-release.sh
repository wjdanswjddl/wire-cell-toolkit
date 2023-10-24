#!/bin/bash

cat <<EOF
This script needs to be refreshed to match change from 
wire-cell-build to wire-cell-toolkit.
EOF
exit -1


# Test a release.  To make a release see make-release.sh
#
# 1) Get source code for a tagged release
#
# $ ./test-release.sh  get-source <path/to/source> <release-tag>
#
# 2) Configure the source by telling it where to find externals and where to install (must not exist)
#
# $ ./test-release.sh configure-source <path/to/source> <path/to/externals> <path/to/install>
#
# 3) Build, install and run tests
#
# $ ./test-release.sh install <path/to/source>
####


function goto
{
    local dir=${1?no directory given} ; shift
    if [ ! -d $dir ] ; then
        echo "No such directory: $dir"
        exit 1
    fi
    pushd $dir >/dev/null
}
function goback
{
    popd >/dev/null
}

function get-source
{
    local source=${1?no sourcedir given} ; shift
    local tag=${1?no tag given} ; shift

    if [ -d $source ] ; then
        echo "Source directory already exists: $source"
        exit 1
    fi

    git clone https://github.com/WireCell/wire-cell-build.git $source
    goto $source
    git checkout -b $tag $tag
    git submodule init
    git submodule update
    git submodule foreach git checkout -b $tag $tag
    goback
}



function configure-source
{
    local source=$(readlink -f ${1?must provide source directory}) ; shift
    local externals=$(readlink -f ${1?must provide externals directory}) ; shift
    local install=$(readlink -f ${1?must provide install directory}) ; shift


    if [ ! -d "$externals" ] ; then
        echo "No externals directory: $externals"
        exit 1
    fi
    if [ -d "$install" ] ; then
        echo "Install directory already exits: $install"
        exit 1
    fi
    mkdir -p $install

    goto $source

    ./wcb configure --prefix=$install \
          --boost-includes=$externals/include \
          --boost-libs=$externals/lib \
          --boost-mt \
          --with-eigen-include=$externals/include/eigen3 \
          --with-jsoncpp=$externals \
	  --with-jsonnet=$externals \
          --with-zlib=$externals \
          --with-tbb=no \
          --with-fftw=$externals \
          --with-root=$externals
    

    cat <<EOF >tester.sh
#!/bin/bash
env LD_LIBRARY_PATH=$externals/lib:$install/lib "\$@"
EOF
    chmod +x tester.sh

    goback
}

function install
{
    local source=$(readlink -f ${1?must provide source directory}) ; shift
    goto $source

    ./wcb --notest || exit 1
    ./wcb --notest install || exit 1
    ./wcb --alltests --testcmd="$source/tester.sh %s" || exit 1

    goback
}


"$@"
