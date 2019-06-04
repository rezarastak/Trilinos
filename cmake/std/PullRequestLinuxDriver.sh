#!/usr/bin/env bash
# set -x  # echo commands

# Identify the path to this script
SCRIPTPATH="$(cd "$(dirname "$0")" ; pwd -P)"
echo -e "SCRIPTPATH: ${SCRIPTPATH}"

# Identify the path to the trilinos repository root
REPO_ROOT=`readlink -f ${SCRIPTPATH}/../..`
echo -e "REPO_ROOT : ${REPO_ROOT}"

# This is the old Linux Driver (deprecated)
#${SCRIPTPATH}/PullRequestLinuxDriver-old.sh

# Both scripts will need access through the sandia
# proxies so set them here.
export https_proxy=http://wwwproxy.sandia.gov:80
export http_proxy=http://wwwproxy.sandia.gov:80
no_proxy='localhost,localnets,.sandia.gov,127.0.0.1,169.254.0.0/16,forge.sandia.gov'


# Call the script to handle merging the incoming branch into
# the current trilinos/develop branch for testing.
${SCRIPTPATH}/PullRequestLinuxDriver-Merge.py ${TRILINOS_SOURCE_REPO:?} \
                                              ${TRILINOS_SOURCE_BRANCH:?} \
                                              ${TRILINOS_TARGET_REPO:?} \
                                              ${TRILINOS_TARGET_BRANCH:?} \
                                              ${TRILINOS_SOURCE_SHA:?} \
                                              ${WORKSPACE:?}

# Call the script to handle driving the testing
${SCRIPTPATH}/PullRequestLinuxDriver-Test.sh

