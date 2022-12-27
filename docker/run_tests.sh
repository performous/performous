#!/bin/bash -ex
## Pull in /etc/os-release so we can see what we're running on
. /etc/os-release

## Default Vars
CLONE_DIRECTORY='/github_actions_build'
CTEST_TESTING_DIRECTORY='build'
GTEST_TESTING_DIRECTORY='build/testing'
RUN_CTEST=true
RUN_GTEST=true

usage() {
  set +x
  echo ""
  echo "Usage: ${0}"
  echo ""
  echo "Optional Arguments:"
  echo "  -c : Run only the ctest suite"
  echo "  -g : Run only the gtest suite"
  echo "  -D <clone/directory>: The absolute path to the directory where Performous"
  echo "     is cloned to from GitHub. Defaults to ${CLONE_DIRECTORY}"
  echo "  -t <test/directory>: The relative path to the testing directory for google tests"
  echo "     under the clone of the Performous repo. Defaults to ${GTEST_TESTING_DIRECTORY}"
  echo "  -T <test/directory>: The relative path to the testing directory for 'make test'"
  echo "     under the clone of the Performous repo. Defaults to ${CTEST_TESTING_DIRECTORY}"
  echo "  -h : Show this help message"
  exit 1
}

## Set up getopts
while getopts "cgD:t:T:h" OPTION; do
  case ${OPTION} in
    "c")
      RUN_GTEST=false;;
    "g")
      RUN_CTEST=false;;
    "D")
      CLONE_DIRECTORY=${OPTARG};;
    "t")
      GTEST_TESTING_DIRECTORY=${OPTARG};;
    "T")
      CTEST_TESTING_DIRECTORY=${OPTARG};;
    "h")
      HELP=true;;
  esac
done

if [ ${HELP} ]; then
  usage
fi

## Run the ctests
## This crazy if statement detecting the OS's can be removed once
## Debian 10 is retired, since it doesn't have the versions of software
## available to run the tests. Once it is retired, just the RUN_CTEST
## statement should be all that is needed.
if ([ "${ID}" = "ubuntu" ] || [ "${ID}" = "fedora" ] || ([ "${ID}" = "debian" ] && [ "${VERSION_ID}" = "11" ])) && [ ${RUN_CTEST} = true ]; then
  echo "Run unit tests"
  cd ${CLONE_DIRECTORY}/${CTEST_TESTING_DIRECTORY}
  make test
  echo -e "\n\n\n\n\n"
fi

## Run the gtests
## See the comment above for a note about this if statement
if ([ "${ID}" = "ubuntu" ] || [ "${ID}" = "fedora" ] || ([ "${ID}" = "debian" ] && [ "${VERSION_ID}" = "11" ])) && [ ${RUN_GTEST} = true ]; then
  echo "Run gtest unit tests"
  cd ${CLONE_DIRECTORY}/${GTEST_TESTING_DIRECTORY}
  ./performous_test --gtest_filter=UnitTest*
fi
