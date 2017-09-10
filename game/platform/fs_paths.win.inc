		// 2. pathBootstrap is called to find out static system paths (critical for logging and for loading config files)
		void pathBootstrap() {
			if (!base.empty()) return;  // Only bootstrap once
			// Base (e.g. /usr/local), share (src or installed data files) and locale (built or installed .mo files)
			{
				char const* root = getenv("PERFORMOUS_ROOT");
				base = fs::absolute(root ? root : execname().parent_path());
				do {
					if (base.empty()) throw std::runtime_error("Unable to find Performous data files. Install properly or set environment variable PERFORMOUS_ROOT.");
					for (fs::path const& infix: { fs::path(SHARED_DATA_DIR), fs::path("data"), fs::path() }) {
						if (!fs::exists(base / infix / configSchema)) continue;
						share = base / infix;
						goto found;
					}
					// Use locale .mo files from build folder?
					if (base.filename() == "build" && fs::exists(base / "lang")) {
						locale = base / "lang";
					}
					base = base.parent_path();
				} while (true);
			found:;
				if (locale.empty() && fs::exists(base / LOCALEDIR)) locale = base / LOCALEDIR;
			}
			// System-wide config files
			{
				sysConf = execname().parent_path() / "config";
			}
			// Home
			{
			char const* p = getenv("USERPROFILE");
				if (p) home = p;
			}
			// Config
			{
				ITEMIDLIST* pidl;
				HRESULT hRes = SHGetSpecialFolderLocation(nullptr, CSIDL_APPDATA|CSIDL_FLAG_CREATE, &pidl);
				if (hRes != NOERROR) throw std::runtime_error("Unable to determine where Application Data is stored");
				char p[MAX_PATH];
				SHGetPathFromIDList(pidl, p);
				conf = p;
				conf /= performous;
			}
			// Data
			{
				data = conf;
			}
			// Cache
			{
				cache = data / "cache";  // FIXME: Should we use GetTempPath?
			}
			pathInit();
		}
		/// Initialize/reset data dirs (search path).
		void pathInit() {
			bool bootstrapping = paths.empty();  // The first run (during bootstrap)
			if (!bootstrapping) {
				std::string logmsg = "fs/info: Found system paths:\n";
				logmsg += "  base:     " + base.string() + '\n';
				logmsg += "  share:    " + share.string() + '\n';
				logmsg += "  locale:   " + locale.string() + '\n';
				logmsg += "  sysConf:  " + sysConf.string() + '\n';
				logmsg += "  home:     " + home.string() + '\n';
				logmsg += "  config:   " + conf.string() + '\n';
				logmsg += "  data:     " + data.string() + '\n';
				logmsg += "  cache:    " + cache.string() + '\n';
				std::clog << logmsg << std::flush;
			}
			// Data dirs
			std::string logmsg = "fs/info: Determining data dirs (search path):\n";
			{
				Paths dirs;
				dirs.push_back(data);  // Adding user's data dir
				dirs.push_back(share);  // Adding system data dir (relative to performous executable or PERFORMOUS_ROOT)

				// Adding paths from config file (during bootstrap config options are not yet available)
				if (!bootstrapping) {
					auto const& conf = config["paths/system"].sl();
					for (std::string const& dir: conf) dirs.splice(dirs.end(), pathExpand(dir));
				}
				// Check if they actually exist and print debug
				paths.clear();
				std::set<fs::path> used;
				for (auto dir: dirs) {
					dir = fs::absolute(dir);
					if (used.find(dir) != used.end()) continue;
					logmsg += "  " + dir.string() + '\n';
					paths.push_back(dir);
					used.insert(dir);
				}
			}
			if (!bootstrapping) std::clog << logmsg << std::flush;
		}