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

DEBUG=0
PERFORMOUS_CLEAN_BUILD=1
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
			echo "Homebrew install found at: ${brew_prefix}"
			PREFIXDIR="${brew_prefix}"
	fi
	if [[ ${port_retcode} = 0 ]]
		then
			port_prefix="$(dirname $(dirname $(which port)))"
			echo "MacPorts install found at: ${port_prefix}"
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

test -z ${PREFIXDIR} && detectPkgManager # Look for both Homebrew and Macports and use whichever is found. If for some reason, both are available, let the user choose. Alternatively, define this while invoking the script to bypass this behavior.
test -z ${DEPLOYMENT_TARGET} && DEPLOYMENT_TARGET=$(sw_vers -productVersion | cut -d . -f 1-2) # Change this if you want to target a different version of macOS.
test -z ${PERFORMOUS_SOURCE} && PERFORMOUS_SOURCE="${CURRDIR}/.." # Change this if using another copy of the source or if this file is not under path/to/performous/osx-utils/
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
	for arg in ${args}
	do
		if [[ ${arg} =~ ${debug} ]]
		then
				DEBUG=1
		else
			if [[ ${arg} =~ ${cleanbuild} ]]
			then
				PERFORMOUS_CLEAN_BUILD=0
			fi
		fi
	done
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
			ln -sf /Applications "${PERFORMOUS_SOURCE}/osx-utils/out/Applications"
			rm -f "${PERFORMOUS_SOURCE}/osx-utils/out/.DS_Store"
			/usr/bin/hdiutil create -ov -srcfolder out -volname Performous -fs HFS+ -fsargs "-c c=64,a=16,e=16" -format UDRW RWPerformous.dmg
			/usr/bin/hdiutil convert -ov RWPerformous.dmg -format UDZO -imagekey zlib-level=9 -o Performous.dmg
			rm -f RWPerformous.dmg
			cd ..
	elif [ "${FANCY_DMG}" == 1 ]
		then
			rm -f "${PERFORMOUS_SOURCE}/osx-utils/Performous.dmg"
			appdmg "${PERFORMOUS_SOURCE}/osx-utils/resources/dmg_spec.json" "${PERFORMOUS_SOURCE}/osx-utils/Performous.dmg"
	fi
}

function main {
	printf "\n"
	if [[ "${DEBUG}" = 1 ]]
		then
			echo "Will create bundle for debugging..."
			RELTYPE=Debug
			ENABLE_TOOLS=OFF
	else
		RELTYPE=Release
		ENABLE_TOOLS=ON
		if exists dylibbundler
			then
				echo "dylibbundler found!"
		else
			echo "dylibbundler not found! you need to install it before creating the bundle."
			exit 1
		fi

		if exists appdmg
			then
				FANCY_DMG=1
				echo "appdmg found! Will build styled bundle."
			else
				if exists npm
					then
						if askPrompt "* appdmg is not installed, would you like to install it? (y/n)"
							then
								sudo npm install -g https://github.com/LinusU/node-appdmg.git && FANCY_DMG=1
							else
								echo "Will build DMG without the fancy style then..."
								FANCY_DMG=0
						fi
					else
						echo "npm not found!"
						echo "Will build DMG without the fancy style then..."
						FANCY_DMG=0
				fi
		fi
	fi

# first compile performous, build dir shouldn't exist at this stage

# define Bundle structure
	TEMPDIR="${PERFORMOUS_SOURCE}/osx-utils/out/Performous.app/Contents"
	RESDIR="${TEMPDIR}/Resources"
	LIBDIR="${RESDIR}/lib"
	LOCALEDIR="${RESDIR}/Locales"
	FRAMEWORKDIR="${RESDIR}/Frameworks"
	BINDIR="${TEMPDIR}/MacOS"
	ETCDIR="${RESDIR}/etc"

	printf "\n"
	echo "--- Performous source: ${PERFORMOUS_SOURCE}"
	echo "--- Build folder: ${PERFORMOUS_SOURCE}/build"
	echo "--- Output folder: ${PERFORMOUS_SOURCE}/osx-utils/out"
	printf "\n"
	if [[ "${PERFORMOUS_CLEAN_BUILD}" == 1 ]]
		then
			echo "Wiping build and output bundle folders..."
			printf "\n"
			rm -rf "${PERFORMOUS_SOURCE}/build"
			rm -rf "${TEMPDIR}"
			mkdir -p "${TEMPDIR}"
		else
			echo "No-clean mode enabled; preserving previous build directory."
			printf "\n"
			rm -rf "${TEMPDIR}"
			mkdir -p "${TEMPDIR}"
	fi
	
	cmake \
	  -DCMAKE_INSTALL_PREFIX="$TEMPDIR" \
	  -DCMAKE_PREFIX_PATH="${PREFIXDIR}" \
	  -DCMAKE_BUILD_TYPE="${RELTYPE}" \
	  -DCMAKE_VERBOSE_MAKEFILE="ON" \
	  -DCMAKE_OSX_DEPLOYMENT_TARGET="${DEPLOYMENT_TARGET}" \
	  -DCMAKE_C_COMPILER="${CCPATH}" \
	  -DCMAKE_CXX_COMPILER="${CXXPATH}" \
	  -DCMAKE_EXPORT_COMPILE_COMMANDS="ON" \
	  -DSHARE_INSTALL="Resources" \
	  -DLOCALE_DIR="Resources/Locales" \
	  -DCMAKE_CXX_FLAGS="-Wall -Wextra" \
	  -DCMAKE_OSX_ARCHITECTURES="x86_64" \
	  -DPERFORMOUS_VERSION="${PACKAGE_VERSION}" \
	  -B "${PERFORMOUS_SOURCE}/build" \
	  -S "${PERFORMOUS_SOURCE}" \
	  -DALLOW_SELF_BUILT_AUBIO="TRUE" \
	  -DALLOW_SELF_BUILT_JSON="TRUE"
	
	make -C "${PERFORMOUS_SOURCE}/build" -j${MAKE_JOBS} install # You can change the -j value in order to spawn more build threads.

# then create the rest of the app bundle

	mv "${TEMPDIR}/bin" "${BINDIR}"

	cp "${PERFORMOUS_SOURCE}/osx-utils/resources/performous-launcher" "${BINDIR}"
	if [[ "${DEBUG}" = 1 ]]
		then
			sed -i '' -e 's|"\${CURRDIR}\/performous"|"\${CURRDIR}\/performous" --log debug|g' "$BINDIR/performous-launcher" # enable debug logging.
	fi
	cp "${PERFORMOUS_SOURCE}/osx-utils/resources/performous.icns" "${RESDIR}"

	## Copy Info.plist and change the token values
	cp "${PERFORMOUS_SOURCE}/osx-utils/resources/Info.plist" "${TEMPDIR}"
	sed -i '' s/@@CFBundleShortVersionString@@/${PACKAGE_MAJOR}\.${PACKAGE_MINOR}/ "${TEMPDIR}/Info.plist"
	sed -i '' s/@@CFBundleLongVersionString@@/${PACKAGE_VERSION}/ "${TEMPDIR}/Info.plist"
	sed -i '' s/@@CFBundleVersion@@/${PACKAGE_SEM_VER}/ "${TEMPDIR}/Info.plist"


	mkdir -p "${FRAMEWORKDIR}"
	mkdir -p "${LIBDIR}"

	if [[ "${DEBUG}" = 0 ]]
		then
			bundlelibs
		else
			echo "Skipping dylibbundler..."
	fi

	mkdir -p "${ETCDIR}"/fonts
	cp -pLR "${PREFIXDIR}"/etc/fonts "${ETCDIR}"

	cd $ETCDIR/fonts
	PREFIX_REGEX=$(echo ${PREFIXDIR} | sed -e 's|\/|\\\/|g')
	sed -i '' -e "s|${PREFIX_REGEX}\/share|\.\.\/\.\.\/\.\.\/Resources|g" fonts.conf
	sed -i '' -e "s|${PREFIX_REGEX}\/var\/cache|\~\/\.cache|g" fonts.conf
	sed -i '' -e 's|\<\!-- Font directory list --\>|\<\!-- Font directory list --\>\
    <dir>\.\.\/\.\.\/pixmaps</dir>|g' fonts.conf

	cd "$CURRDIR"

# then build the disk image
	if [[ "${DEBUG}" = 0 ]]
		then
			createdmg
		else
			echo "Skipping creation of .dmg image."
	fi
}

check_arguments

main
