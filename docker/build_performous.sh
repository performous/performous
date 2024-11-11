#!/bin/bash -ex
## Pull in /etc/os-release so we can see what we're running on
. /etc/os-release

test -f ./platform_flags.sh && . ./platform_flags.sh

## Default Vars
GIT_REPOSITORY='https://github.com/performous/performous.git'

## Function to print the help message
usage() {
	set +x
	echo ""
	echo "Usage: ${0}"
	echo ""
	echo "Optional Arguments:"
	echo "	-b <Git Branch>: Build the specified git branch, tag, or sha"
	echo "	-D </path/to/build/directory>: Disable cloning the repo from git and build in the specified directory"
	echo "	-E <'Extra Cmake Args'>: A quoted list of extra arguments to pass directly to cmake"
	echo "	-g : Generate Packages"
	echo "	-p <Pull Request #>: Build the specified Github Pull Request number"
	echo "	-r <Repository URL>: Git repository to pull from"
	echo "	-R : Perform a 'Release' Cmake Build (Default is 'RelWithDebInfo')"
	echo "	-h : Show this help message"
	exit 1
}

## Set up getopts
while getopts "b:D:E:gp:r:R:h" OPTION; do
	case ${OPTION} in
	"b")
		GIT_BRANCH=${OPTARG};;
	"D")
		BUILD_DIRECTORY=${OPTARG};;
	"E")
		EXTRA_CMAKE_ARGS="${EXTRA_CMAKE_ARGS} ${OPTARG}";;
	"g")
		GENERATE_PACKAGES=true;;
	"p")
		PULL_REQUEST=${OPTARG};;
	"r")
		GIT_REPOSITORY=${OPTARG};;
	"R")
		PERFORMOUS_BUILD_TYPE=${OPTARG};;
	"h")
		HELP=true;;
	esac
done

if [[ "${HELP}" == "true" ]]; then
	usage
fi

# Pull in any platform-specific flags that may have been set under each docker container.
if [[ "$(command -v import_platform_flags > /dev/null ; echo $?)" == "0" ]]; then
	import_platform_flags
fi

## All the git stuff
if [[ "${BUILD_DIRECTORY}" == "" ]]; then
	git clone ${GIT_REPOSITORY}
	cd performous
	if [[ "${PULL_REQUEST}" != "" ]]; then
		git fetch origin pull/${PULL_REQUEST}/head:pr
		git checkout pr
	elif [[ "${GIT_BRANCH}" != "" ]]; then
		git checkout ${GIT_BRANCH}
	fi
	git submodule update --init --recursive
else
	mkdir -p "${BUILD_DIRECTORY}"
	cd "${BUILD_DIRECTORY}"
fi


## If PACKAGE_TYPE is unset, use tgz.
if [[ "${PACKAGE_TYPE}" == "" ]]; then
		PACKAGE_TYPE="TGZ"
fi


## Build with cmake
mkdir build
cd build
cmake ${EXTRA_CMAKE_ARGS} \
	${PLATFORM_CMAKE_FLAGS} \
		-DENABLE_WEBSERVER=ON \
		-DCMAKE_VERBOSE_MAKEFILE=ON \
		-DENABLE_WEBCAM=ON \
		-DCMAKE_BUILD_TYPE=${PERFORMOUS_BUILD_TYPE} \
		-DBUILD_TESTS=ON \
		-DSELF_BUILT_CED=ALWAYS \
		..
CPU_CORES=$(nproc --all)
make -j${CPU_CORES}
if [[ "${GENERATE_PACKAGES}" == "true" ]]; then
	cpack -G ${PACKAGE_TYPE}
fi
cd ..
