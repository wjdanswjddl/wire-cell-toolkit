#!/bin/bash

cat <<EOF
This script needs to be refreshed to match change from 
wire-cell-build to wire-cell-toolkit.
EOF
exit -1

# Release management script for Wire Cell Toolkit.

# Note, this is meant to create and operate on a throw-away source
# area.  Do NOT use this on real working areas.  Do NOT reuse this on
# areas used for past releases.  If you do, all that you love will be
# torn and rent asunder and laid waste.

# 1) Create release source area.  This leaves the main repo and each
# submodule sitting at the tip of the given branch.  This has many
# steps which can also be done individually.  Warning: this hard-codes
# a list of submodules to purge out of the release so edit it in
# master before staring a release..
#
# $ ./make-release.sh bring-forward 0.5.x
#
# 2) If the master tips of each submodule are what is needed then tag
# everything and make a top level commit.  Note, this commit is not
# pushed.  Note: the source area is left in a slightly inconsistent
# state in that the submodule URLs are changed to HTTPS but no "git
# submodule sysnc" is done.  This is so the push step can still go via
# SSH.
#
# $ ./make-release.sh apply-tags 0.5.x 0.5.0 "Some useful one liner describing major changes."
#
# 3) If happy, push all the changes
#
# $ ./make-release.sh push-everything 0.5.x
#

#set -x

function get-source
{
    local target=$1 ; shift
    if [ -d "$target" ] ; then
        echo "source target exists in: $target"
        return
    fi
    git clone git@github.com:WireCell/wire-cell-build.git $target
}

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

function make-branch
{
    local branch=${1?must provide branch} ; shift
    goto $branch

    if [ -n "$(git show-ref refs/heads/$branch)" ] ; then
        echo "Already have local: $branch"
        git checkout $branch
    elif [ -n "$(git show-ref remotes/origin/$branch)" ] ; then
        echo "Branch on origin: $branch"
        git checkout -b $branch origin/$branch
    else        
        echo "Initial creation of branch: $branch"
        git checkout -b $branch origin/master
    fi
    goback
}

function purge-submodules
{
    local branch=${1?must provide branch} ; shift
    goto $branch
    local submodules=$@

    for sm in $submodules ; do
        if [ -d $sm ] ; then
            git submodule deinit $sm || exit 1
            git rm $sm || exit 1
        fi
    done
    goback
}
# alg bio dfp rio riodata rootdict rootvis tbb

function branch-submodules
{
    local branch=${1?must provide branch} ; shift
    goto $branch

    git submodule init || exit 1
    git submodule update || exit 1
    
    git submodule foreach "git checkout $branch || git checkout -b $branch --track origin/master"

    goback
}

function update-submodules
{
    local branch=${1?must provide branch} ; shift
    goto $branch
    git pull --recurse-submodules
    goback
}

function fix-dotgitmodules
{
    local branch=${1?must provide branch} ; shift

    local org="WireCell"
    local dev_url="git@github.com:$org"
    local usr_url="https://github.com/$org"

    goto $branch

    # move to anon-friend URL for releases
    sed -i -e 's|'$dev_url'|'$usr_url'|'g .gitmodules

    # Crazy hack to make sure .gitmodules is updated with new branch
    # for each surviving submodule.  There is probably a better way to
    # do this!
    git submodule foreach 'branch="$(git --git-dir=../.git rev-parse --abbrev-ref HEAD)"; sm="$(basename $(pwd))"; git config -f ../.gitmodules submodule.$sm.branch $branch'

    # Do NOT actually sync
    #git submodule sync

    goback
}


# kitchen sink function
function bring-forward
{
    local branch=${1?must give branch} ; shift

    echo -e "\ngetting source\n"
    get-source $branch || exit 1

    echo -e "\nmaking top branch\n"
    make-branch $branch ||exit 1

    unwanted_submodules="alg bio dfp rio riodata rootdict rootvis tbb"
    echo -e "\npurging submodules: $unwanted_submodules\n"
    purge-submodules $branch $unwanted_submodules

    echo -e "\nbranching submodules\n"
    branch-submodules $branch

    echo -e "\nupdating submodules\n"
    update-submodules $branch

    echo -e "\nswitch submodule URLs\n"
    fix-dotgitmodules $branch


    # fixme: specify which submodules to keep, purge all others
    # fixme bonus1: hard code some (waftools, util, iface)
    # fixme bonus2: figure out dependencies!

}
# now do tag-submodules, push-submodules, submodule-urls, push-main

function apply-submodule-tags
{
    local branch=${1?must provide branch} ; shift
    local tag=${1?must give branch} ; shift
    local message="$@"

    goto $branch
    git submodule foreach git tag -a -m "$message" $tag
    goback
}


function commit-tag-main
{
    local branch=${1?must provide branch} ; shift
    local tag=${1?must give branch} ; shift
    local message=${1?must give message} ; shift

    goto $branch
    git commit -a -m "$message"
    git tag -a -m "$message" "$tag"
    goback
}
# now by hand: git push origin $branch


# kitchen sink function
function apply-tags
{
    local branch=${1?must provide branch} ; shift
    local tag=${1?must give tag} ; shift
    local message="$@"

    echo -e "\napplying submodule tags\n"
    apply-submodule-tags $branch $tag "$message"

    echo -e "\ncommitting and tagging top\n"
    commit-tag-main $branch $tag "$message"

}


# warning this pushes!
function push-everything
{
    local branch=${1?must provide source directory aka branch} ; shift
    goto $branch

    git submodule foreach git push origin $branch
    git submodule foreach git push --tags

    git push origin $branch
    git push --tags

    goback
}

function tarball
{
    local branch=${1?must give branch} ; shift
    local tag=${1?must give tag} ; shift
    local base=${1:-wire-cell-toolkit} ; shift
    local name="${base}-${tag}"
    local tarfile="${name}.tar.gz"

    if [ -f "$tarfile" ] ; then
        echo "target tarball file exists, remove to continue: $tarfile"
        return
    fi
    if [ -f "$name" ] ; then
        echo "target directory exists, remove to continue: $name"
        return;
    fi

    git clone --recurse-submodules --branch=$branch \
        git@github.com:WireCell/wire-cell-build.git \
        $name

    cd $name
    git checkout $tag
    git submodule init
    git submodule update
    cd ..

    tar --exclude=.git* -czf $tarfile $name

    echo "when happy:"
    echo "cp $tarfile /var/www/lar/software/releases"
}


"$@"

