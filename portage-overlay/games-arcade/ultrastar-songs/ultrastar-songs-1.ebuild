# Copyright 1999-2007 Gentoo Foundation
# Distributed under the terms of the GNU General Public License v2

inherit eutils games

DESCRIPTION="UltraStar series songs pack"
HOMEPAGE="http://sourceforge.net/projects/ultrastar-ng/"
SRC_URI="mirror://sourceforge/ultrastar-ng/${P}.tar.bz2"

LICENSE="CCPL-Attribution-ShareAlike-NonCommercial-2.5"
SLOT="0"
KEYWORDS="~amd64 ~x86"
IUSE=""

DEPEND="games-arcade/ultrastar-ng"

src_install() {
	dodir "${GAMES_DATADIR}"/ultrastar
	mv songs "${D}${GAMES_DATADIR}"/ultrastar/
}
