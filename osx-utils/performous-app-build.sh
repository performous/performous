#!/bin/bash
set -o errexit
# the very first step is to check that dylibbundler exists,
# without it the bundle would be broken

SOURCE="${BASH_SOURCE[0]}"
while [ -h "${SOURCE}" ]
	do # resolve $SOURCE until the file is no longer a symlink
		DIR="$( cd -P "$( dirname "${SOURCE}" )" && pwd )"
		SOURCE="$(readlink "${SOURCE}")"
		[[ "${SOURCE}" != /* ]] && SOURCE="${DIR}/${SOURCE}" # if $SOURCE was a relative symlink, we need to resolve it relative to the path where the symlink file was located
	done

CURRDIR="$( cd -P "$( dirname "$SOURCE" )" && pwd )"
args="$@"
debug="^(--debug|-d)$"
cleanbuild="^(--no-clean|-k)$"
genproject="^(--xcode-project|-x)$"
xcodebundle="^(--xcode-bundle|-b)$"
noregen="^(--no-regenerate|-n)$"

DEBUG=0
PERFORMOUS_CLEAN_BUILD=1
PERFORMOUS_GEN_PROJECT=0
PERFORMOUS_XCODE_BUNDLE=0
PERFORMOUS_NO_REGENERATE=0
brew_retcode=-1
port_retcode=-1

function askPrompt {
	echo -n "$1 "
		while read -r -n 1 -s answer
			do
				if [[ $answer = [YyNn] ]]
					then
						echo -n $answer
						[[ $answer = [Yy] ]] && retval=0
						[[ $answer = [Nn] ]] && retval=1
						break
				fi
			done

	echo # just a final linefeed, optics...

	return $retval
}

function detectPkgManager {
	brew_retcode=$(brew -v 1>/dev/null 2>&1; echo $?)
	port_retcode=$(port version 1>/dev/null 2>&1; echo $?)
	if [[ ${brew_retcode} = 0 ]]
		then
			brew_prefix="$(dirname $(dirname $(which brew)))"
			echo "--- Homebrew install found at: ${brew_prefix}"
			opencv_installed=$(brew list opencv 2>/dev/null | grep "No available formula or cask" 1>/dev/null 2>/dev/null; echo $?)
			opencv3_installed=$(brew list opencv@3 2>/dev/null | grep "No available formula or cask" 1>/dev/null 2>/dev/null; echo $?)			
			if [[ ${opencv_installed} ]]
				then
					opencv_prefix="$(dirname $(dirname '$(brew ls opencv | grep OpenCVConfig.cmake)'))"
			elif [[ ${opencv3_installed} ]]
				then
					opencv_prefix="$(dirname $(dirname '$(brew ls opencv@3 | grep OpenCVConfig.cmake)'))"
			else
				opencv_prefix=
			fi

			PREFIXDIR="${brew_prefix}"
	fi
	if [[ ${port_retcode} = 0 ]]
		then
			port_prefix="$(dirname $(dirname $(which port)))"
			echo "--- MacPorts install found at: ${port_prefix}"
			opencv4_installed=$(port -q installed opencv4 | grep -v "None of the specified" >/dev/null; echo $?)
			opencv3_installed=$(port -q installed opencv3 | grep -v "None of the specified" >/dev/null; echo $?)
			if [[ ${opencv4_installed} ]]
				then
					opencv_prefix="$(dirname $(dirname $(port -q contents opencv4 | grep OpenCVConfig.cmake)))/"
			elif [[ ${opencv3_installed} ]]
				then
					opencv_prefix="$(dirname $(dirname $(port -q contents opencv3 | grep OpenCVConfig.cmake)))/"
			else
				opencv_prefix=
			fi
			PREFIXDIR="${port_prefix}"
	fi
	if [[ ${port_retcode} = 0 && ${brew_retcode} = 0 ]]
		then
			if askPrompt "Would you like to use Homebrew (y), or MacPorts (n)?"
				then
					PREFIXDIR="${brew_prefix}"
				else
					PREFIXDIR="${port_prefix}"
			fi
	fi
}

MAKE_JOBS=$(sysctl -n hw.ncpu)
test -z ${CC} && CCPATH="/usr/bin/clang" # Path to system Clang, change if you want another compiler.
test -z ${CXX} && CXXPATH="/usr/bin/clang++" # Path to system Clang, change if you want another compiler.

## Set the versions that will be changed in the copied Info.plist file.
## On the MacOS builder, 'PACKAGE_VERSION' is exported and will be picked up.
## If it isn't there, fall back to what git provides for a version locally.
if [ -z ${PACKAGE_VERSION} ]; then
	PACKAGE_VERSION=$(git describe --tags || echo 1.0.0)
fi
PACKAGE_SEM_VER=$(echo ${PACKAGE_VERSION} | grep -oE "[0-9]+\.[0-9]+\.[0-9]+")
if [ $(echo ${PACKAGE_VERSION} | grep -c alpha) -gt 0 ]; then
	PR_NUM=$(echo ${PACKAGE_VERSION} | cut -d'-' -f2)
	PACKAGE_SEM_VER="${PACKAGE_SEM_VER}a${PR_NUM}"
elif [ $(echo ${PACKAGE_VERSION} | grep -c beta) -gt 0 ]; then
	BETA_NUM=$(echo ${PACKAGE_VERSION} | cut -d'-' -f2)
	PACKAGE_SEM_VER="${PACKAGE_SEM_VER}b${BETA_NUM}"
fi
PACKAGE_MAJOR=$(echo ${PACKAGE_SEM_VER} | cut -d'.' -f1)
PACKAGE_MINOR=$(echo ${PACKAGE_SEM_VER} | cut -d'.' -f2)


function exists {
	if hash "$1" 2>/dev/null
		then
			return 0
	else
		return 1
	fi
}

function check_arguments {
	shopt -s nocasematch
	_OUT_DIR="osx-utils/out"
	for arg in ${args}
	do
		if [[ ${arg} =~ ${debug} ]]
		then
				DEBUG=1
		elif [[ ${arg} =~ ${cleanbuild} ]]
			then
				PERFORMOUS_CLEAN_BUILD=0
		elif [[ ${arg} =~ ${genproject} ]]
			then
				DEBUG=1
				PERFORMOUS_GEN_PROJECT=1
		elif [[ ${arg} =~ ${xcodebundle} ]]
			then
				DEBUG=1
				PERFORMOUS_XCODE_BUNDLE=1
		elif [[ ${arg} =~ ${noregen} ]]
			then
				PERFORMOUS_NO_REGENERATE=1
		fi
	done
	if [[ ${PERFORMOUS_XCODE_BUNDLE} == 1 ]]
		then
			_OUT_DIR="build/xcode-proj/xcode-out"
	elif [[ ${PERFORMOUS_GEN_PROJECT} == 1 ]]
		then
			_OUT_DIR="build/xcode-proj/xcode-out"
	fi
	shopt -u nocasematch
}

function bundlelibs {
	dylibbundler -od -b -x "${BINDIR}/performous" -d "${LIBDIR}" -p @executable_path/../Resources/lib/
		declare -a performous_tools
		performous_tools=(gh_fsb_decrypt gh_xen_decrypt itg_pck ss_adpcm_decode ss_archive_extract ss_chc_decode ss_cover_conv ss_extract ss_ipu_conv ss_pak_extract)
		for i in "${performous_tools[@]}"
			do
				if [ -f "${BINDIR}/$i" ]
					then
						dylibbundler -of -b -x "$BINDIR/$i" -d "$LIBDIR" -p @executable_path/../Resources/lib/
				fi
			done
}

function createdmg {
	if [ "${FANCY_DMG}" == 0 ]
		then
			ln -sf /Applications "${PERFORMOUS_OUTPUT_DIR}/Applications"
			rm -f "${PERFORMOUS_OUTPUT_DIR}/.DS_Store"
			/usr/bin/hdiutil create -ov -srcfolder out -volname Performous -fs HFS+ -fsargs "-c c=64,a=16,e=16" -format UDRW RWPerformous.dmg
			/usr/bin/hdiutil convert -ov RWPerformous.dmg -format UDZO -imagekey zlib-level=9 -o Performous.dmg
			rm -f RWPerformous.dmg
			cd ..
	elif [ "${FANCY_DMG}" == 1 ]
		then
			rm -f "${PERFORMOUS_OUTPUT_DIR}/Performous.dmg"
			appdmg "${PERFORMOUS_SOURCE_DIR}/osx-utils/resources/dmg_spec.json" "${PERFORMOUS_OUTPUT_DIR}/Performous.dmg"
	fi
}

function finalize_bundle {
# 	mv "${TEMPDIR}/bin" "${BINDIR}"
	cp -v "${PERFORMOUS_SOURCE_DIR}/osx-utils/resources/performous-launcher" "${BINDIR}/"
	if [[ "${DEBUG}" = 1 ]]
		then
			sed -i '' -e 's|"\${CURRDIR}\/performous"|"\${CURRDIR}\/performous" --log debug|g' "$BINDIR/performous-launcher" # enable debug logging.
	fi
	cp -v "${PERFORMOUS_SOURCE_DIR}/osx-utils/resources/performous.icns" "${RESDIR}"

	## Copy Info.plist and change the token values
	cp -v "${PERFORMOUS_SOURCE_DIR}/osx-utils/resources/Info.plist" "${TEMPDIR}"
	sed -i '' s/@@CFBundleShortVersionString@@/${PACKAGE_MAJOR}\.${PACKAGE_MINOR}/ "${TEMPDIR}/Info.plist"
	sed -i '' s/@@CFBundleLongVersionString@@/${PACKAGE_VERSION}/ "${TEMPDIR}/Info.plist"
	sed -i '' s/@@CFBundleVersion@@/${PACKAGE_SEM_VER}/ "${TEMPDIR}/Info.plist"


	mkdir -p "${FRAMEWORKDIR}"
	mkdir -p "${LIBDIR}"

	if [[ "${DEBUG}" = 0 ]]
		then
			bundlelibs
		else
			echo "--- Skipping dylibbundler..."
	fi

	mkdir -p "${ETCDIR}"/fonts
	cp -pLR "${PREFIXDIR}"/etc/fonts "${ETCDIR}"

	cd $ETCDIR/fonts
	PREFIX_REGEX=$(echo ${PREFIXDIR} | sed -e 's|\/|\\\/|g')
	sed -i '' -e "s|${PREFIX_REGEX}\/share|\.\.\/\.\.\/\.\.\/Resources|g" fonts.conf
	sed -i '' -e "s|${PREFIX_REGEX}\/var\/cache|\~\/\.cache|g" fonts.conf
	sed -i '' -e 's|\<\!-- Font directory list --\>|\<\!-- Font directory list --\>\
	<dir>\.\.\/\.\.\/pixmaps</dir>|g' fonts.conf
}


function main {
	printf "\n"
	if [[ "${DEBUG}" = 1 ]]
		then
			echo "--- Will create bundle for debugging..."
			RELTYPE=Debug
			ENABLE_TOOLS=OFF
	else
		RELTYPE=Release
		ENABLE_TOOLS=ON
		if exists dylibbundler
			then
				echo "--- dylibbundler found!"
		else
			echo "--- dylibbundler not found! you need to install it before creating the bundle."
			exit 1
		fi

		if exists appdmg
			then
				FANCY_DMG=1
				echo "--- appdmg found! Will build styled bundle."
			else
				if exists npm
					then
						if askPrompt "* appdmg is not installed, would you like to install it? (y/n)"
							then
								sudo npm install -g https://github.com/LinusU/node-appdmg.git && FANCY_DMG=1
							else
								echo "--- Will build DMG without the fancy style then..."
								FANCY_DMG=0
						fi
					else
						echo "--- npm not found!"
						echo "--- Will build DMG without the fancy style then..."
						FANCY_DMG=0
				fi
		fi
	fi

# first compile performous, build dir shouldn't exist at this stage

# define Bundle structure
	TEMPDIR="${PERFORMOUS_OUTPUT_DIR}/Performous.app/Contents"
	RESDIR="${TEMPDIR}/Resources"
	LIBDIR="${RESDIR}/lib"
	LOCALEDIR="${RESDIR}/Locales"
	FRAMEWORKDIR="${RESDIR}/Frameworks"
	BINDIR="${TEMPDIR}/MacOS"
	ETCDIR="${RESDIR}/etc"

	if [[ "${PERFORMOUS_CLEAN_BUILD}" == 1 ]]
		then
			echo "--- Wiping build folder..."
			printf "\n"
			rm -rf "${PERFORMOUS_SOURCE_DIR}/build"
			if [[ "${PERFORMOUS_GEN_PROJECT}" != 1 && "${PERFORMOUS_XCODE_BUNDLE}" != 1 ]]
				then
					echo "--- Wiping output folder..."
					printf "\n"
					rm -rf "${TEMPDIR}"
					mkdir -p "${TEMPDIR}"
			fi
		else
			echo "--- No-clean mode enabled; preserving previous build directory."
			printf "\n"
			if [[ "${PERFORMOUS_GEN_PROJECT}" != 1 && "${PERFORMOUS_XCODE_BUNDLE}" != 1 ]]
				then
					echo "--- We aren't making an Xcode Project or bundle..."
					printf "\n"
					echo "--- Wiping output folder..."
					rm -rf "${TEMPDIR}"
					mkdir -p "${TEMPDIR}"
			fi
	fi
	
	if [[ "${PERFORMOUS_GEN_PROJECT}" == 1 || "${PERFORMOUS_XCODE_BUNDLE}" == 1 ]]
		then
			CMAKE_GENERATOR="Xcode"
			PERFORMOUS_BUILD_DIR="${PERFORMOUS_SOURCE_DIR}/build/xcode-proj"
			XCODE_GENERATE_SCHEME="ON"
		else
			CMAKE_GENERATOR="Unix Makefiles"
			PERFORMOUS_BUILD_DIR="${PERFORMOUS_SOURCE_DIR}/build"
			XCODE_GENERATE_SCHEME="OFF"
	fi
	
	printf "\n"
	echo "--- Performous source: ${PERFORMOUS_SOURCE_DIR}"
	echo "--- Build folder: ${PERFORMOUS_BUILD_DIR}"
	echo "--- Output folder: ${PERFORMOUS_OUTPUT_DIR}"
	printf "\n"

	if [[ "${PERFORMOUS_NO_REGENERATE}" != 1 ]]
		then
			cmake \
			  -DCMAKE_INSTALL_PREFIX="${TEMPDIR}" \
			  -DCMAKE_PREFIX_PATH="${PREFIXDIR};${opencv_prefix}" \
			  -DCMAKE_BUILD_TYPE="${RELTYPE}" \
			  -DCMAKE_VERBOSE_MAKEFILE="ON" \
			  -DCMAKE_OSX_DEPLOYMENT_TARGET="${DEPLOYMENT_TARGET}" \
			  -DCMAKE_C_COMPILER="${CCPATH}" \
			  -DCMAKE_CXX_COMPILER="${CXXPATH}" \
			  -DCMAKE_EXPORT_COMPILE_COMMANDS="ON" \
			  -DCMAKE_CXX_FLAGS="-Wall -Wextra" \
			  -DCMAKE_OSX_ARCHITECTURES="x86_64" \
			  -DCMAKE_XCODE_GENERATE_SCHEME="${XCODE_GENERATE_SCHEME}" \
			  -DPERFORMOUS_VERSION="${PACKAGE_VERSION}" \
			  -DSELF_BUILT_AUBIO="ALWAYS" \
			  -DFETCHCONTENT_QUIET="ON" \
			  -B "${PERFORMOUS_BUILD_DIR}" \
			  -G "${CMAKE_GENERATOR}" \
			  -S "${PERFORMOUS_SOURCE_DIR}"
	
		if [[ "${PERFORMOUS_GEN_PROJECT}" == 0 ]]
			then
				make -C "${PERFORMOUS_BUILD_DIR}" -j${MAKE_JOBS} install # You can change the -j value in order to spawn more build threads.
			else
				if askPrompt "Would you like to open the XCode Project we just created? (y/n)"
					then
						open "${PERFORMOUS_BUILD_DIR}/Performous.xcodeproj"
				fi
		fi
	fi
	
	# then create the rest of the app bundle
	if [[ "${PERFORMOUS_GEN_PROJECT}" == 0 || "${PERFORMOUS_XCODE_BUNDLE}" == 1 ]]
		then
			if [[ "${PERFORMOUS_XCODE_BUNDLE}" == 1 ]]
				then
					echo "--- Finalizing bundle created by Xcode"
			fi
			finalize_bundle
	
		cd "$CURRDIR"
	# then build the disk image
		if [[ "${DEBUG}" = 0 ]]
			then
				createdmg
			else
				echo "--- Skipping creation of .dmg image."
		fi
	fi
}

check_arguments

test -z ${PREFIXDIR} && detectPkgManager  # Look for both Homebrew and Macports and use whichever is found. If for some reason, both are available, let the user choose. Alternatively, define this while invoking the script to bypass this behavior.
test -z ${DEPLOYMENT_TARGET} && DEPLOYMENT_TARGET=$(sw_vers -productVersion | cut -d . -f 1-2) # Change this if you want to target a different version of macOS.
test -z ${PERFORMOUS_SOURCE_DIR} && PERFORMOUS_SOURCE_DIR="${CURRDIR}/.." # Change this if using another copy of the source or if this file is not under path/to/performous/osx-utils/
test -z ${PERFORMOUS_OUTPUT_DIR} && PERFORMOUS_OUTPUT_DIR="${PERFORMOUS_SOURCE_DIR}/${_OUT_DIR}" # Change this to modify where the bundle gets built.

main
