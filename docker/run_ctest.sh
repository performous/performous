#!/bin/bash -ex
## Pull in /etc/os-release so we can see what we're running on
. /etc/os-release

## Default Vars
CLONE_DIRECTORY='/github_actions_build'
TESTING_DIRECTORY='build'

usage() {
  set +x
  echo ""
  echo "Usage: ${0}"
  echo ""
  echo "Optional Arguments:"
  echo "  -C <clone/directory>: The absolute path to the directory where Performous"
  echo "     is cloned to from GitHub. Defaults to ${CLONE_DIRECTORY}"
  echo "  -T <test/directory>: The relative path to the testing directory under the"
  echo "     clone of the Performous repo. Defaults to ${TESTING_DIRECTORY}"
  echo "  -h : Show this help message"
  exit 1
}

## Set up getopts
while getopts "C:T:h" OPTION; do
  case ${OPTION} in
    "C")
      CLONE_DIRECTORY=${OPTARG};;
    "T")
      TESTING_DIRECTORY=${OPTARG};;
    "h")
      HELP=true;;
  esac
done

if [ ${HELP} ]; then
  usage
fi

if ([ "${ID}" = "ubuntu" ] || [ "${ID}" = "fedora" ] || ([ "${ID}" = "debian" ] && [ "${VERSION_ID}" = "11" ])); then
  echo "Run unit tests"
  cd ${CLONE_DIRECTORY}/${TESTING_DIRECTORY}
  make test
fi

