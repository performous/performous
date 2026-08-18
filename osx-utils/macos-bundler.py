#!/usr/bin/env python3

from dmgbuild.core import build_dmg
from docopt import docopt

from os import cpu_count, uname
from re import match, search, findall
from typing import Optional
from pathlib import Path

import platform
import shutil
import subprocess
import sys
import termios
import tty


port_location = None
opencv_prefix: Path = None
ffmpeg_prefix: Path = None
fmt_prefix : Path = None
script_prefix: Path = None
performous_source_dir = None

include_feature_args = ["--enable-webcam", "--enable-midi", "--enable-webserver", "--build-tests"]
include_feature_opts = ["AUTO", "ON", "OFF" ]

find_dep_args = ["--internal-aubio", "--internal-ced", "--internal-json"]
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

def check_installed_port(name : str, file : str) -> Optional[Path]:
	p = subprocess.run(args = ["port", "contents", name], encoding="utf-8", capture_output=True)
	if p.returncode == 0:
		p2 = subprocess.run(args = ["grep", file], encoding="utf-8", capture_output=True, input=p.stdout)
		if p2.returncode == 0:
			return str_to_path(p2.stdout.strip()).parent.parent
		else:
			return None
	else:
		return None

def detect_prefix():
	global opencv_prefix, script_prefix, ffmpeg_prefix, fmt_prefix
	port_location = check_installed('port')
	if port_location != None:
		print("--- MacPorts install detected at: " + str(port_location) + "\n")
		for opencv_version in ["4", "3"]:
			check_opencv = check_installed_port(f"opencv{opencv_version}", "OpenCVConfig.cmake")
			if check_opencv != None:
				opencv_prefix = str(check_opencv)
				print(f"--- OpenCV {opencv_version} detected at: " + str(opencv_prefix) + "\n")
				break
		for ffmpeg_version in ["8", "7", "6", ""]:
			check_ffmpeg = check_installed_port(f"ffmpeg{ffmpeg_version}", "libavcodec.pc")
			if check_ffmpeg != None:
				ffmpeg_prefix = str(check_ffmpeg.parent)
				print(f"--- FFMpeg {ffmpeg_version or '4'} detected at: " + str(ffmpeg_prefix) + "\n")
				break
		for fmt_version in ["11", "10", "9", "8", "7"]:
			check_fmt = check_installed_port(f"libfmt{fmt_version}", "fmt-config.cmake")
			if check_fmt != None:
				fmt_prefix = str(check_fmt.parent)
				print(f"--- LibFMT {fmt_version} detected at: " + str(fmt_prefix) + "\n")
				break

	if arguments["--prefix"] != None:
		if str_to_path(arguments["--prefix"]).is_dir():
			script_prefix = str_to_path(arguments["--prefix"])
			return
		else:
			raise FileNotFoundError("Specified an inexistent prefix folder.")
	elif port_location is None:
		raise FileNotFoundError("Can't find a MacPorts install. MacPorts is the only supported package manager for building Performous on macOS.")
	else:
		script_prefix = port_location.parent.parent

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
	outFile = (performous_out_dir / (f"Performous-{package_version}-{arguments['--arch']}.dmg"))
	dmgDefines = {
		'app':str(performous_out_dir / 'Performous.app'),
		'background':str(performous_source_dir / 'osx-utils/resources/dmg-bg.png'),
		'license':str(performous_source_dir / 'LICENSE.md')
	}
	dmgVolumeName = f"Performous-{package_version}"
	dmgOutFile = outFile
	dmgSettingsFile = str(performous_source_dir / 'osx-utils/performous-dmg-settings.py')
	print(fr"""
		Will build dmg image with the following settings:

		build_dmg(
			{dmgOutFile},
			{dmgVolumeName},
			{dmgSettingsFile},
			defines={dmgDefines},
			lookForHiDPI=True,
			detach_retries=10
		)
		""")
	build_dmg(
		dmgOutFile,
		dmgVolumeName,
		dmgSettingsFile,
		defines=dmgDefines,
		lookForHiDPI=True,
		detach_retries=10
	)

def dedupe_rpaths():
	# dylibbundler can write the same LC_RPATH entry into a dylib more than once when
	# it's a shared transitive dependency of several bundled libraries. Recent dyld
	# versions hard-reject duplicate LC_RPATH entries at load time ("Library not
	# loaded... duplicate LC_RPATH"), so strip the extras here.
	bundle_lib_dir = Path(f"{performous_out_dir}/Performous.app/Contents/Resources/lib")
	targets = list(bundle_lib_dir.glob("*.dylib")) + [Path(f"{performous_out_dir}/Performous.app/Contents/MacOS/Performous")]
	for target in targets:
		result = subprocess.run(["otool", "-l", str(target)], capture_output=True, text=True, check=True)
		rpaths = findall(r"cmd LC_RPATH\n\s+cmdsize \d+\n\s+path (.*?) \(offset \d+\)", result.stdout)
		seen = set()
		changed = False
		for rpath in rpaths:
			if rpath in seen:
				subprocess.run(["install_name_tool", "-delete_rpath", rpath, str(target)], check=True)
				changed = True
			else:
				seen.add(rpath)
		if changed:
			# Match dylibbundler's own signing invocation rather than a bare re-sign
			# so nothing dylibbundler relied on gets silently dropped here.
			subprocess.run(["codesign", "--force", "--deep", "--preserve-metadata=entitlements,requirements,flags,runtime", "--sign", "-", str(target)], check=True)

def bundle_libs():
	global performous_out_dir
	print("Copying dependencies and fixing linkage inside Performous.app...")

	execute(fr"""
		dylibbundler -od -b \
		-x "{performous_out_dir}/Performous.app/Contents/MacOS/Performous" \
		-d "{performous_out_dir}/Performous.app/Contents/Resources/lib" \
		-p @executable_path/../Resources/lib/
	""")
	dedupe_rpaths()
	return

def verify_bundle():
	# Guards against shipping a .dmg with a broken .app inside it:
	# partial build/install, dylibbundler run that silently skipped a dependency,
	# or leftover stale files from a previous run under --no-clean can all produce an
	# .app that "successfully" builds but doesn't work.
	print("--- Verifying the bundle is self-contained and launches...")
	app_dir = performous_out_dir / "Performous.app"
	exe = app_dir / "Contents/MacOS/Performous"
	res_dir = app_dir / "Contents/Resources"
	lib_dir = res_dir / "lib"

	if not exe.is_file():
		raise RuntimeError(f"Bundle verification failed: executable not found at {exe}")

	# Game data (themes/sounds/shaders/config/etc) must have been installed alongside
	# the executable -- a build that only produces the raw binary is not a usable app.
	for required in ["themes", "sounds", "shaders", "config", "backgrounds"]:
		required_dir = res_dir / required
		if not required_dir.is_dir() or not any(required_dir.iterdir()):
			raise RuntimeError(f"Bundle verification failed: {required_dir} is missing or empty -- game data was not installed into the bundle.")

	if not lib_dir.is_dir() or not any(lib_dir.glob("*.dylib")):
		raise RuntimeError(f"Bundle verification failed: {lib_dir} has no bundled dylibs -- dependencies were not relinked into the bundle.")

	# No remaining references to the build machine's MacPorts prefix: everything must run standalone
	targets = [exe] + list(lib_dir.glob("*.dylib"))
	external_refs = []
	for target in targets:
		result = subprocess.run(["otool", "-L", str(target)], capture_output=True, text=True, check=True)
		for line in result.stdout.splitlines()[1:]:
			dep = line.strip().split(" (")[0]
			if dep.startswith("@executable_path") or dep.startswith("/usr/lib/") or dep.startswith("/System/"):
				continue
			external_refs.append((str(target.relative_to(app_dir)), dep))
	if external_refs:
		details = "\n".join(f"  {t}: {d}" for t, d in external_refs)
		raise RuntimeError(f"Bundle verification failed: found dependencies not relinked into the bundle (the .app is not standalone):\n{details}")

	# The bundle must actually launch: dyld resolves every dependency before main() runs, so invoke --version
	# to run without needing a display, audio device, or GUI event loop.
	try:
		result = subprocess.run([str(exe), "--version"], capture_output=True, text=True, timeout=30)
	except subprocess.TimeoutExpired:
		raise RuntimeError("Bundle verification failed: Performous.app did not respond within 30s.")
	if result.returncode != 0:
		crash_info = ""
		if result.returncode < 0:
			try:
				lldb_result = subprocess.run(
					["lldb", "-b", "-o", "run", "-o", "bt all", "--", str(exe), "--version"],
					capture_output=True, text=True, timeout=30,
				)
				crash_info = f"\n\n--- lldb backtrace (best-effort) ---\n{lldb_result.stdout}\n{lldb_result.stderr}"
			except (subprocess.TimeoutExpired, FileNotFoundError) as lldb_error:
				crash_info = f"\n\n--- lldb backtrace unavailable: {lldb_error} ---"
		raise RuntimeError(f"Bundle verification failed: Performous.app failed to launch (exit {result.returncode}):\n{result.stdout}\n{result.stderr}{crash_info}")
	print(f"--- Bundle verified OK: {result.stdout.strip()}")

usageHelp = f"""\nPerformous macOS Bundler

Usage:
	macos_bundler.py [--arch <architecture>] [options]
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
	--arch <architecture>  Target architecture name passed to the compiler. Defaults to the currently detected architecture as reported by uname. [default: {uname().machine}]
	--cc <path/to/compiler>  Change C compiler [default: /usr/bin/clang]
	--cxx <path/to/compiler>  Change C compiler [default: /usr/bin/clang++]
	--internal-aubio <auto | always | never>  Find previously installed aubio on system [default: auto]
	--internal-ced <auto | always | never>  Find previously installed ced on system [default: auto]
	--internal-json <auto | always | never>  Find previously installed nlohmann-json on system [default: auto]
	-p <prefix>, --prefix <prefix>  Set prefix path for searching of libraries and headers. Defaults to the detected MacPorts install prefix.
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

		if check_installed("dylibbundler") is None:
			raise FileNotFoundError("dylibbundler needs to be installed in order to create a release application bundle.")

	else:
		release_type = "Debug"

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

	# Clear any stale .dmg from a previous run before starting
	stale_dmg = performous_out_dir / f"Performous-{package_version}-{arguments['--arch']}.dmg"
	if stale_dmg.exists():
		print("--- Removing stale .dmg from a previous run: " + str(stale_dmg))
		stale_dmg.unlink()

	file_exists(arguments["--cc"])
	file_exists(arguments["--cxx"])

	print("--- Performous source: " + str(performous_source_dir))
	print("--- Performous build folder: " + str(performous_build_dir))
	print("--- Performous output folder: " + str(performous_out_dir) + "\n")

	if arguments["--no-regenerate"] != True:
		prefix = ""
		if script_prefix != None:
			prefix += str(script_prefix)
		if opencv_prefix != None:
			prefix += (";" + str(opencv_prefix))
		if ffmpeg_prefix != None:
			prefix += (";" + str(ffmpeg_prefix))
		if fmt_prefix != None:
			prefix += (";" + str(fmt_prefix + "/cmake"))
		command = fr"""
		cmake \
		-DPKG_CONFIG_USE_CMAKE_PREFIX_PATH:BOOL=ON \
		-DCMAKE_INSTALL_PREFIX:PATH="{str(performous_out_dir)}" \
		-DCMAKE_VERBOSE_MAKEFILE:BOOL=ON \
		-DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=ON \
		-DSELF_BUILT_AUBIO:STRING="{arguments['--internal-aubio'].upper()}" \
		-DSELF_BUILT_CED:STRING="{arguments['--internal-ced'].upper()}" \
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
		-DCMAKE_OSX_ARCHITECTURES="{arguments['--arch']}" \
		-DCMAKE_XCODE_GENERATE_SCHEME:BOOL={xcode_gen_scheme} \
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
			verify_bundle()
			create_dmg()
