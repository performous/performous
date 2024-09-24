# Copyright 1999-2019 Gentoo Authors
# Distributed under the terms of the GNU General Public License v2

EAPI=8
[[ ${PV} = 9999 ]] && GIT="git-r3"

CMAKE_REMOVE_MODULES_LIST=(FindALSA FindBoost FindGettext FindJpeg FindPng FindTiff FindZ)

inherit cmake ${GIT}

SONGS_PN=ultrastar-songs

DESCRIPTION="SingStar GPL clone"
HOMEPAGE="https://performous.org"
SRC_URI="songs? (
	mirror://sourceforge/performous/${SONGS_PN}-restricted-3.zip
	mirror://sourceforge/performous/${SONGS_PN}-jc-1.zip
	mirror://sourceforge/performous/${SONGS_PN}-libre-3.zip
	mirror://sourceforge/performous/${SONGS_PN}-shearer-1.zip
)"

GIT_BASE_URI="https://github.com/performous"

PATCHES=(
)

CMAKE_MAKEFILE_GENERATOR="emake"
if [ "$PV" != "9999" ]; then
    SRC_URI="${GIT_BASE_URI}/performous/archive/${PV}.tar.gz -> ${P}.tar.gz $SRC_URI"
	PATCHES=(
		$PATCHES
	)
else
    EGIT_REPO_URI="${GIT_BASE_URI}/performous.git"
	PATCHES=(
		$PATCHES
	)
fi

LICENSE="GPL-2
	songs? (
		CC-BY-NC-SA-2.5
		CC-BY-NC-ND-2.5
	)"
SLOT="0"
KEYWORDS="~amd64 ~x86"
IUSE="midi songs tools webcam"

RDEPEND="dev-cpp/libxmlpp:3.0
	media-libs/portaudio
	dev-libs/boost[threads(+)]
	dev-libs/glib
	dev-libs/libxml2
	gnome-base/librsvg
	tools? ( media-gfx/imagemagick )
	virtual/jpeg
	media-libs/libpng:0
	media-libs/glm
	media-libs/libsdl2[joystick,video]
	virtual/ffmpeg
	virtual/opengl
	virtual/glu
	virtual/cblas
	sys-libs/zlib
	dev-libs/libfmt
	virtual/libintl
	x11-libs/cairo
	x11-libs/gdk-pixbuf
	x11-libs/pango
	midi? ( media-libs/portmidi )
	webcam? ( media-libs/opencv )"
DEPEND="${RDEPEND}
	media-libs/libepoxy
	sys-apps/help2man
	sys-devel/gettext"

src_unpack() {
	if [[ ${PV} == 9999 ]]; then
		git-r3_src_unpack
		for dep in compact_enc_det aubio; do
			git-r3_fetch "${GIT_BASE_URI}/${dep}.git/"
			git-r3_checkout "${GIT_BASE_URI}/${dep}.git/" "deps/${dep}.git"
		done
	else
		unpack ${P}.tar.gz
	fi

	use songs && unpack ${SONGS_PN}-restricted-3.zip
	use songs && unpack ${SONGS_PN}-jc-1.zip
	use songs && unpack ${SONGS_PN}-libre-3.zip
	use songs && unpack ${SONGS_PN}-shearer-1.zip
}

src_prepare() {
	cmake_src_prepare
}

src_configure() {
	local mycmakeargs=(
		-DCMAKE_INSTALL_PREFIX="${EPREFIX}/usr"
		-DENABLE_TOOLS="$(usex tools)"
		-DENABLE_WEBCAM="$(usex webcam)"
		-DENABLE_MIDI="$(usex midi)"
		-DENABLE_WEBSERVER=OFF
	)
	if [[ ${PV} == 9999 ]]; then
		mycmakeargs+=(
			-DSELF_BUILT_GIT_BASE=${WORKDIR}/deps/
			-DSELF_BUILT_AUBIO=NEVER
			-DSELF_BUILT_CED=ALWAYS
		)
	fi
	cmake_src_configure
}

src_compile() {
	cmake_src_compile
}

src_install() {
	cmake_src_install
	if use songs ; then
		insinto "${EPREFIX}/usr/share"/${PN}
		doins -r "${WORKDIR}/songs"
	fi
	dodoc docs/{Authors,instruments}.txt
}

pkg_postinst() {
	xdg_desktop_database_update
}

pkg_postrm() {
	xdg_desktop_database_update
}
