#!/usr/bin/python
# Prints download stats from Launchpad PPA.

from launchpadlib.launchpad import Launchpad

PPAOWNER = "performous-team"  # PPA owner
PPA = "ppa"                   # PPA name

cachedir = "~/.cache/launchpadlib/"
apiurl = 'https://api.edge.launchpad.net/devel/ubuntu/'

lp_ = Launchpad.login_anonymously('ppastats', 'edge', cachedir, version='devel')
owner = lp_.people[PPAOWNER]
archive = owner.getPPAByName(name=PPA)

def printDLCount(distarch):
	for i in archive.getPublishedBinaries(status='Published',distro_arch_series=apiurl+distarch):
		# Uncomment last part of next line to get daily stats        
		print i.binary_package_name + "\t" + i.binary_package_version + "\t" + str(i.getDownloadCount()) #+ "\t" + str(i.getDailyDownloadTotals())

print "Package\tVersion\tDownloads" #"\tDaily DLs"
print
printDLCount("maverick/i386")
printDLCount("maverick/amd64")
printDLCount("lucid/i386")
printDLCount("lucid/amd64")
