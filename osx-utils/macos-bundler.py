#!/usr/bin/env python3

from docopt import docopt

from os import cpu_count, uname
from re import match, search
from typing import Optional
from pathlib import Path


import platform
import shutil
import subprocess
import sys
import termios
import tty


port_location = None
brew_location = None
opencv_prefix: Path = None
openssl_prefix: Path = None
openssl_root = ""
script_prefix: Path = None
performous_source_dir = None

include_feature_args = ["--enable-webcam", "--enable-midi", "--enable-webserver", "--build-tests"]
include_feature_opts = ["AUTO", "ON", "OFF" ]

find_dep_args = ["--internal-aubio", "--internal-json"]
find_dep_opts = ["AUTO", "ALWAYS", "NEVER"]

majorVer = None
minorVer = None
semVersion = None

def execute(command):
	print(str(command))
	subprocess.run(command, shell=True, executable="/bin/bash", stdout=sys.stdout, stderr=subprocess.STDOUT, check=True)
	
def str_to_path(somePath: str) -> Path:
	ret : Path = Path(somePath)
	if somePath[0] == "~":
		ret = somePath.expanduser()
	return ret.resolve()


def file_exists(command):
	if str_to_path(command).is_file() != True:
		raise FileExistsError("Can't find specified file: " + str(command))
	return True

def check_installed(name : str) -> Optional[Path]:
	p = subprocess.run(args = ["/usr/bin/which", name], encoding="utf-8", capture_output=True)
	if p.returncode == 0:
		return str_to_path(p.stdout)
	else:
		return None

def check_brew_formula(name : str, file : str) -> Optional[Path]:
	p = subprocess.run(args = ["brew", "ls", "--verbose", name], encoding="utf-8", capture_output=True)
	if p.returncode == 0:
		p2 = subprocess.run(args = ["grep", file], encoding="utf-8", capture_output=True, input=p.stdout)
		return str_to_path(p2.stdout.strip()).parent
	else:
		return None

def check_installed_port(name : str, file : str) -> Optional[str]:
	p = subprocess.run(args = ["port", "contents", name], encoding="utf-8", capture_output=True)
	if p.returncode == 0:
		p2 = subprocess.run(args = ["grep", file], encoding="utf-8", capture_output=True, input=p.stdout)
		return str(str_to_path(p2.stdout.strip()).parent.parent)
	else:
		return None

def detect_prefix():
	global opencv_prefix, script_prefix, openssl_prefix, openssl_root
	port_location = check_installed('port')
	brew_location = check_installed('brew')
	if port_location != None:
		print("--- MacPorts install detected at: " + str(port_location) + "\n")
		check_opencv = check_installed_port("opencv4", "OpenCVConfig.cmake")
		if check_opencv != None:
			opencv_prefix = str(check_opencv)
			print("--- OpenCV 4+ detected at: " + str(opencv_prefix) + "\n")
		else:
			check_opencv = check_installed_port("opencv3", "OpenCVConfig.cmake")
			if check_opencv != None:
				opencv_prefix = str(check_opencv)
				print("--- OpenCV 3 detected at: " + str(opencv_prefix) + "\n")

	else:
		print("--- MacPorts does not appear to be installed.\n")
	if brew_location != None:
		print("--- Homebrew install detected at: " + str(brew_location))
		check_opencv = check_brew_formula("opencv", "OpenCVConfig.cmake")
		if check_opencv != None:
			opencv_prefix = str(check_opencv.parent)
			print("--- OpenCV 4+ detected at: " + str(opencv_prefix) + "\n")
		else:
			check_opencv = check_brew_formula("opencv@3", "OpenCVConfig.cmake")
			if check_opencv != None:
				opencv_prefix = str(check_opencv.parent)
				print("--- OpenCV 3 detected at: " + str(opencv_prefix) + "\n")
		check_openssl = check_brew_formula("openssl", "libcrypto.pc")
		if check_openssl != None:
			openssl_prefix = str(check_openssl)
			openssl_root = f"-DOPENSSL_ROOT_DIR='{str(check_openssl.parent.parent)}'"
			print("--- OpenSSL detected at: " + str(openssl_prefix) + "\n")
	else:
		print("--- Homebrew does not appear to be installed.\n")

	if arguments["--prefix"] != None:
		if str_to_path(arguments["--prefix"]).is_dir():
			script_prefix = str_to_path(arguments["--prefix"])
			return
		else:
			raise FileNotFoundError("Specified an inexistent prefix folder.")
	elif port_location == None and brew_location == None:
		raise FileNotFoundError("Can't find a suitable package manager.")
	else:
		if arguments["--prefer-homebrew"] == True:
			if brew_location == None and port_location != None:
				print("\n--- WARNING: Homebrew requested, but not found: defaulting to MacPorts install.\n")
				script_prefix = port_location.parent.parent
				return

		elif arguments["--prefer-macports"] == True:
			if port_location == None and brew_location != None:
				print("\n--- WARNING: Macports requested, but not found: defaulting to Homebrew install.\n")
				script_prefix = brew_location.parent.parent
				return
		else:
			script_prefix = port_location.parent.parent if port_location != None else brew_location.parent.parent

def ask_user(prompt : str, opt1 : str = "y", opt2 : str = "n") -> bool:
	stdin = sys.stdin.fileno()
	tattr = termios.tcgetattr(stdin)
	try:
		response = ""
		print(f"{prompt} ({opt1}/{opt2}): ")
		tty.setcbreak(stdin, termios.TCSANOW)
		while response not in {opt1[0].lower(), opt2[0].lower()}:
			if response != "":
				print (f"Please answer {opt1} or {opt2}: ")
			response = sys.stdin.read(1).lower()
		return (response == opt1[0].lower())
	except KeyboardInterrupt:
		return False
	finally:
		termios.tcsetattr(stdin, termios.TCSANOW, tattr)

## Set the versions that will be changed in the copied Info.plist file.
## If it isn't there, fall back to what git provides for a version locally.
def set_version():
	global package_version, semVersion, majorVer, minorVer
	package_version = arguments["--package-version"] or str(subprocess.run(args = fr"/usr/bin/git describe", encoding="utf-8", shell = True, capture_output=True).stdout).strip() or "1.0.0"
	versionMatch = match(
	pattern=r"(?P<versionMajor>[0-9]+)\.(?P<versionMinor>[0-9]+)\.(?P<versionPatch>[0-9]+)",string=package_version)
	majorVer = versionMatch.group('versionMajor')
	minorVer = versionMatch.group('versionMinor')
	patchVer = versionMatch.group('versionPatch')
	semVersion = ".".join(versionMatch.groups())
	revisionMatch = search(
	pattern=r"-(?P<revisionPR>[0-9]+)-g?(?P<revisionCommit>[0-9a-fA-F]{7,})-?(?P<revisionType>(?:alpha|beta))?",string=package_version)
	if revisionMatch != None:
		revPR = revisionMatch.group('revisionPR')
		revCommit = revisionMatch.group('revisionCommit')
		revType = revisionMatch.group('revisionType') or None
		if revType != None:
			semVersion += (revType[0] + revPR)

def clean_build_dir():
	global performous_build_dir
	print("--- Wiping temporary build folder: " + str(performous_build_dir))
	shutil.rmtree(path=performous_build_dir, ignore_errors=True)
	performous_build_dir.mkdir(mode=0o755, exist_ok=True)

def create_dmg(fancy: bool = True):
	outFile = (performous_out_dir / ("Performous-" + package_version + ".dmg"))
	execute(fr"""
		dmgbuild \
		-s "{str(performous_source_dir / 'osx-utils/performous-dmg-settings.py')}" \
		-Dapp="{str(performous_out_dir / 'Performous.app')}" \
		-Dbackground="{str(performous_source_dir / 'osx-utils/resources/dmg-bg.png')}" \
		-Dlicense="{str(performous_source_dir / 'LICENSE.md')}" \
		"Performous-{package_version}" \
		"{outFile}"
	""")

def bundle_libs():
	global performous_out_dir
	print("Copying dependencies and fixing linkage inside Performous.app...")
	
	execute(fr"""
		dylibbundler -od -b \
		-x "{performous_out_dir}/Performous.app/Contents/MacOS/Performous" \
		-d "{performous_out_dir}/Performous.app/Contents/Resources/lib" \
		-p @executable_path/../Resources/lib/
	""")
	return

usageHelp = f"""\nPerformous macOS Bundler

Usage:
	macos_bundler.py [--arch <architecture>]... [--prefer-macports | --prefer-homebrew] [options]
	macos_bundler.py [options]

Options:
	-h --help  Show this help message.
	-b --preserve-build  Don't remove temporary build files before starting.
	-d --debug  Create .app for debugging (skip copying and relinking of dependencies, as well as creation of .dmg package)
	-f --flat-output  Put output directly in the output folder, without versioned folders.
	-j <n>, --jobs <n>  Argument passed to make, specifying the max number of jobs to run. Defaults to the output of os.cpu_count() [default: {str(cpu_count())}]
	-k --no-clean  Don't erase tree structure of the .app before beginning work.
	-n --no-regenerate  Don't regenerate the CMake build system.
	-v <version>, --package-version <version>  Sets the version on the Info.plist of the created .app. By default, it's calculated according to the latest stable version and hash of the latest commit.
	-x --xcode-project  Generate an XCode project suitable for debugging. Note: to use this project, it's necessary to build the install target from XCode and then the performous target (or, alternatively, mirror the settings for the performous scheme on the install target)
	--enable-midi <auto | on | off>  Defines whether to include MIDI support [default: auto]
	--enable-webcam <auto | on | off>  Defines whether to include webcam support [default: auto]
	--enable-webserver <auto | on | off>  Defines whether to include webserver support [default: auto]
	--build-tests <auto | on | off>  Defines whether to build unit tests [default: auto]
	--script-debug  Print the resolved arguments and options passed to this utility.

Environment:
	--arch <architecture>...  Target architecture names passed to the compiler. Defaults to the currently detected architecture as reported by uname. [default: {uname().machine}] 
	--cc <path/to/compiler>  Change C compiler [default: /usr/bin/clang]
	--cxx <path/to/compiler>  Change C compiler [default: /usr/bin/clang++]
	--internal-aubio <auto | always | never>  Find previously installed aubio on system [default: auto]
	--internal-json <auto | always | never>  Find previously installed nlohmann-json on system [default: auto]
	-p <prefix>, --prefix <prefix>  Set prefix path for searching of libraries and headers. By default, the tool tries to detect whether MacPorts or Homebrew are installed and the prefix is set accordingly. Note: If both are installed, you can choose.
	(--prefer-macports | --prefer-homebrew)  Prefer either MacPorts or Homebrew.
	-s <path>, --source <path>  Path to the Performous source. Defaults to ../
	-o <path>, --output <path>  Path where the .app will be built. Defaults to <performous-source>/osx-utils/out[/xcode]
	-t <target>, --target <target>  macOS target version. Defaults to the currently running version, as reported by platform.mac_ver() [default: {str(float('.'.join(platform.mac_ver()[0].split('.')[:2])))}]"""

if __name__ == "__main__":
	arguments = docopt(docstring = usageHelp, default_help = False)

	if arguments["--script-debug"] == True:
		print("\n")
		print(arguments)
		print("\n\n")


	if arguments["--help"] == True:
		print (usageHelp)
		sys.exit(0)

	for arg in find_dep_args:
		if (arguments[arg].upper() not in find_dep_opts):
			print(f"Invalid value for {arg}; options are: {', '.join(find_dep_opts)}")
			sys.exit(1)

	for arg in include_feature_args:
		if (arguments[arg].upper() not in include_feature_opts):
			print(f"Invalid value for {arg}; options are: {', '.join(include_feature_opts)}")
			sys.exit(1)

	detect_prefix()
	set_version()

	script_dir = str_to_path(__file__).parent
	if arguments["--source"] != None and str_to_path(arguments["--source"]).is_dir():
		performous_source_dir = str_to_path(arguments["--source"])
	else:
		if arguments["--source"] != None:
			print(f"\n--- WARNING: Can't find path to Performous source at {arguments['--source']}, defaulting to ../\n")
		performous_source_dir = script_dir.parent

	if arguments["--xcode-project"] == True:
		build_dir = "build.xcode"
		cmake_gen = "Xcode"
		xcode_gen_scheme="ON"
		
	else:
		build_dir = "build"
		cmake_gen = "Unix Makefiles"
		xcode_gen_scheme="OFF"
	
	performous_build_dir = performous_source_dir / build_dir

	if arguments["--preserve-build"] != True:
		clean_build_dir()

	if arguments["--output"] != None and str_to_path(arguments["--output"]).is_dir():
		performous_out_dir = str_to_path(arguments["--output"])
	else:
		out_dir = "out.xcode" if arguments["--xcode-project"] == True else "out"
		performous_out_dir = performous_source_dir / "osx-utils" / out_dir
		if arguments["--output"] != None:
			print("\n--- WARNING: Can't find path to Output folder at " + arguments["--output"] + ", defaulting to " + performous_out_dir + "\n")
			
	if arguments["--flat-output"] != True:
		performous_out_dir = performous_out_dir / f"Performous-{package_version}"
	print("Performous source: " + str(performous_source_dir)+"\n\n")


	if arguments["--debug"] != True:
		release_type = "RelWithDebInfo"
		log_level = "info"
		
		if check_installed("dylibbundler") == None:
			raise FileNotFoundError("dylibbundler needs to be installed in order to create a release application bundle.")
		
	else:
		release_type = "Debug"
		log_level = "debug"
	
	temp_dir = performous_out_dir / "Performous.app/Contents"
	res_dir = temp_dir / "Resources"
	etc_dir = temp_dir / "etc"
	lib_dir = res_dir / "lib"
	locale_dir = res_dir / "Locale"
	bin_dir = temp_dir / "MacOS"
	
	if arguments["--xcode-project"] != True and arguments["--no-clean"] != True:
		print ("--- Deleting output bundle at: " + str(temp_dir.parent))
		shutil.rmtree(str(temp_dir.resolve()), ignore_errors=True)
		temp_dir.mkdir(mode=0o755, parents=True)
	else:
		print ("--- No-clean mode enabled. Won't wipe output bundle at: " + str(temp_dir.parent))

	file_exists(arguments["--cc"])
	file_exists(arguments["--cxx"]) 

	print("--- Performous source: " + str(performous_source_dir))
	print("--- Performous build folder: " + str(performous_build_dir))
	print("--- Performous output folder: " + str(performous_out_dir) + "\n")
	
	if arguments["--no-regenerate"] != True:
		prefix = str(script_prefix)
		if opencv_prefix != None:
			prefix += (";" + str(opencv_prefix))
		if openssl_prefix != None:
			prefix += (";" + str(openssl_prefix))
		command = fr"""
		cmake \
		{openssl_root} \
		-DPKG_CONFIG_USE_CMAKE_PREFIX_PATH:BOOL=ON \
		-DCMAKE_INSTALL_PREFIX:PATH="{str(performous_out_dir)}" \
		-DCMAKE_VERBOSE_MAKEFILE:BOOL=ON \
		-DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=ON \
		-DSELF_BUILT_AUBIO:STRING="{arguments['--internal-aubio'].upper()}" \
		-DSELF_BUILT_JSON:STRING="{arguments['--internal-json'].upper()}" \
		-DENABLE_MIDI:STRING="{arguments['--enable-midi'].upper()}" \
		-DENABLE_WEBCAM:STRING="{arguments['--enable-webcam'].upper()}" \
		-DENABLE_WEBSERVER:STRING="{arguments['--enable-webserver'].upper()}" \
		-DBUILD_TESTS:STRING="{arguments['--build-tests'].upper()}" \
		-DFETCHCONTENT_QUIET:BOOL=ON \
		-DCMAKE_POLICY_DEFAULT_CMP0126=NEW \
		-DCMAKE_PREFIX_PATH:STRING="{prefix}" \
		-DCMAKE_BUILD_TYPE:STRING={release_type} \
		-DCMAKE_OSX_DEPLOYMENT_TARGET:STRING={arguments['--target']} \
		-DCMAKE_C_COMPILER:PATH="{arguments['--cc']}" \
		-DCMAKE_CXX_COMPILER:PATH="{arguments['--cxx']}" \
		-DCMAKE_OSX_ARCHITECTURES="{";".join(arguments['--arch'])}" \
		-DCMAKE_XCODE_GENERATE_SCHEME:BOOL={xcode_gen_scheme} \
		-DPERFORMOUS_LOG_LEVEL:STRING="{log_level}" \
		-DPERFORMOUS_VERSION:STRING="{package_version}" \
		-DPERFORMOUS_SEMVER:STRING="{semVersion}" \
		-DPERFORMOUS_SHORT_VERSION:STRING="{majorVer}.{minorVer}" \
		-G='{cmake_gen}' \
		-S="{str(performous_source_dir)}" \
		-B="{str(performous_build_dir)}"
		"""
	print(f"Generating Buildsystem with command:\n")
	execute(command)

	if arguments["--xcode-project"] == True:
		if ask_user("Would you build the XCode project we just created?") == True:
			execute(f"xcodebuild -project {str(performous_build_dir / 'Performous.xcodeproj')} -scheme install")

		if ask_user("Would you like to open the XCode project we just created?") == True:
			execute(f"open {str(performous_build_dir / 'Performous.xcodeproj')}")
	else:
		execute(f"make -C {performous_build_dir} -j {arguments['--jobs']} install VERBOSE=1")
		if arguments["--debug"] != True:
			bundle_libs()
			create_dmg()