# Copyright 1999-2010 Gentoo Foundation
# Distributed under the terms of the GNU General Public License v2
# $Header: $

EAPI=3

inherit games

DESCRIPTION="Song editor for the game Frets On Fire"
HOMEPAGE="http://www.t3-i.com/eof.htm"
SRC_URI="http://www.t3-i.com/apps/${PN}/downloads/${PN}-${PV}-linux.tar.gz"

LICENSE=""
SLOT="0"
KEYWORDS="~amd64"
IUSE=""

DEPEND="
	media-libs/allegro
	media-sound/vorbis-tools
	media-sound/lame
"
RDEPEND="${DEPEND}"

src_compile() {
	cd "${WORKDIR}/src/"
	emake -f makefile.linux || die "emake failed"
}

src_install() {
	cd "${WORKDIR}"
	cat >> bin/eof.script <<-EOF
		#!/bin/bash
		TMP_DIR=\`mktemp -d || echo "/tmp/falback"\`
		echo ">> Using \\"\${TMP_DIR}\\" as temporary location"
		cp /usr/share/eof/* "\${TMP_DIR}"
		cd "\${TMP_DIR}"
		eof.bin
		rm -rf "\${TMP_DIR}"
	EOF
	newbin bin/eof eof.bin
	newbin bin/eof.script eof
	insinto "/usr/share/${PN}"
	doins bin/eof.dat bin/check.wav
}
