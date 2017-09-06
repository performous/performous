#!/bin/bash
set -o errexit
# the very first step is to check that dylibbundler exists,
# without it the bundle would be broken

args=("$@")
debug="^(--debug|--DEBUG|-d|-D)$"

function asksure {
	echo -n "$1"
		while read -r -n 1 -s answer
			do
				if [[ $answer = [YyNn] ]]
					then
						[[ $answer = [Yy] ]] && retval=0
						[[ $answer = [Nn] ]] && retval=1
						break
				fi
			done

	echo # just a final linefeed, optics...

	return $retval
}

function exists {
	if hash "$1" 2>/dev/null
		then
			return 0
	else
		return 1
	fi
}

function arewedebug {
	if [[ ${args[0]} =~ ${debug} ]]
		then
			DEBUG=1
	else
		DEBUG=0
	fi
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
			ln -sf /Applications "${CURRDIR}/out/Applications"
			rm "${CURRDIR}/out/.DS_Store"
			/usr/bin/hdiutil create -ov -srcfolder out -volname Performous -fs HFS+ -fsargs "-c c=64,a=16,e=16" -format UDRW RWPerformous.dmg
			/usr/bin/hdiutil convert -ov RWPerformous.dmg -format UDZO -imagekey zlib-level=9 -o Performous.dmg
			rm -f RWPerformous.dmg
			cd ..
	elif [ "${FANCY_DMG}" == 1 ]
		then
			rm -f "${CURRDIR}/Performous.dmg"
			appdmg "${CURRDIR}/resources/dmg_spec.json" "${CURRDIR}/Performous.dmg"
	fi
}

function main {
	if [[ "${DEBUG}" = 1 ]]
		then
			echo "Will create bundle for debugging..."
	else
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
						if asksure "appdmg is not installed, would you like to install it? (y/n)"
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

	SOURCE="${BASH_SOURCE[0]}"
	while [ -h "${SOURCE}" ]
		do # resolve $SOURCE until the file is no longer a symlink
			DIR="$( cd -P "$( dirname "${SOURCE}" )" && pwd )"
			SOURCE="$(readlink "${SOURCE}")"
			[[ "${SOURCE}" != /* ]] && SOURCE="${DIR}/${SOURCE}" # if $SOURCE was a relative symlink, we need to resolve it relative to the path where the symlink file was located
		done
	CURRDIR="$( cd -P "$( dirname "$SOURCE" )" && pwd )"

# first compile performous, build dir shouldn't exist at this stage

# define Bundle structure
	TEMPDIR="${CURRDIR}/out/Performous.app/Contents"
	RESDIR="${TEMPDIR}/Resources"
	LIBDIR="${RESDIR}/lib"
	LOCALEDIR="${RESDIR}/Locales"
	FRAMEWORKDIR="{$RESDIR}/Frameworks"
	BINDIR="${TEMPDIR}/MacOS"
	ETCDIR="${RESDIR}/etc"

	DEPLOYMENT_TARGET="10.9"
	MAKE_JOBS=8

	rm -rf "${TEMPDIR}"
	mkdir -p "${TEMPDIR}"

	rm -rf ./build
	mkdir build
	cd build

	cmake -DCMAKE_INSTALL_PREFIX=$TEMPDIR -DENABLE_TOOLS=ON -DCMAKE_BUILD_TYPE=Release -DENABLE_WEBSERVER=ON -DCMAKE_VERBOSE_MAKEFILE=1 -DFreetype_INCLUDE_DIR=/opt/local/include/freetype2 -DCMAKE_OSX_DEPLOYMENT_TARGET=${DEPLOYMENT_TARGET} -DFontconfig_INCLUDE_DIR=/opt/local/include/fontconfig -DPng_INCLUDE_DIR=/opt/local/include/libpng -DAVCodec_INCLUDE_DIR=/opt/local/include/libavcodec -DAVFormat_INCLUDE_DIR=/opt/local/include/libavformat -DSWScale_INCLUDE_DIR=/opt/local/include/libswscale -DFreetype_INCLUDE_DIR=/opt/local/include/freetype2/ -DLibXML2_LIBRARY=/opt/local/lib/libxml2.dylib -DLibXML2_INCLUDE_DIR=/opt/local/include/libxml2 -DLibXML++Config_INCLUDE_DIR=/opt/local/lib/libxml++-2.6/include -DGlibmmConfig_INCLUDE_DIR=/opt/local/lib/glibmm-2.4/include -DGlibConfig_INCLUDE_DIR=/opt/local/lib/glib-2.0/include -DCMAKE_C_COMPILER=/usr/bin/clang -DCMAKE_CXX_COMPILER=/usr/bin/clang++ -DCMAKE_C_FLAGS="-arch x86_64" -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DSHARE_INSTALL=Resources -DLOCALE_DIR=Resources/Locales -DCMAKE_CXX_FLAGS="-Wall -Wextra -stdlib=libc++ -arch x86_64" -DCMAKE_EXE_LINKER_FLAGS="-stdlib=libc++ -lc++ -lc++abi -arch x86_64" -DCMAKE_OSX_ARCHITECTURES="x86_64" ../..
	make -j${MAKE_JOBS} install # You can change the -j value in order to spawn more build threads.

# then create the rest of the app bundle

	mv "${TEMPDIR}/bin" "${BINDIR}"

	cp ../resources/performous-launcher "${BINDIR}"
	if [[ "${DEBUG}" = 1 ]]
		then
			sed -i '' -e 's|"\${CURRDIR}\/performous"|"\${CURRDIR}\/performous" --log debug|g' "$BINDIR/performous-launcher" # enable debug logging.
	fi
	cp ../resources/performous.icns "${RESDIR}"
	cp ../resources/Info.plist "${TEMPDIR}"

	mkdir -p "${FRAMEWORKDIR}"
	mkdir -p "${LIBDIR}"

	if [[ "${DEBUG}" = 0 ]]
		then
			bundlelibs
		else
			echo "Skipping dylibbundler..."
	fi

	mkdir -p "${ETCDIR}"/fonts
	cp -pLR /opt/local/etc/fonts "${ETCDIR}"

	cd $ETCDIR/fonts
	sed -i '' -e 's|\/opt\/local/share|\.\.\/\.\.\/\.\.\/Resources|g' fonts.conf
	sed -i '' -e 's|\/opt\/local/var/cache|\~\/\.cache|g' fonts.conf
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

arewedebug

main