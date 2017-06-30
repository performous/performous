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
				sysConf = (Platform::currentOS() == Platform::macos) ? "/Library/Preferences/Performous" : "/etc/xdg/performous";
			}
			
			// Home
			{
			char const* p = getenv("HOME");
				if (p) home = p;
			}
			
			// Config
			{
				if (Platform::currentOS() == Platform::macos) {
					conf = (home / "Library/Preferences/Performous");
				}
				else {
					char const* p = getenv("XDG_CONFIG_HOME");
					conf = (p ? p : home / ".config");
					conf /= performous;
				}
			}
			
			// Data
			{
				if (Platform::currentOS() == Platform::macos) {
					data = share;
				}
				else {
				char const* p = getenv("XDG_DATA_HOME");
				data = (p ? p / performous : home / ".local" / SHARED_DATA_DIR);
				}
			}
			// Cache
			{
				if (Platform::currentOS() == Platform::macos) {
					cache = (home / "Library/Caches/Performous"); 
				}
				else {
				char const* p = getenv("XDG_CACHE_HOME");
				cache = (p ? p / performous : home / ".cache" / performous);
				}
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
				if (Platform::currentOS() == Platform::macos) {
				char const* p = getenv("XDG_CONFIG_HOME");
				fs::path oldConf = (p ? p : home / ".config/performous");
				if (fs::is_directory(oldConf)) {
				std::clog << "fs/info: Configuration files found in old path, " << oldConf << std::endl;
				conf = (home / "/Library/Preferences/Performous");
					if (bootstrapping) {
						copyDirectoryRecursively(oldConf, conf);
					try {
						fs::remove_all(oldConf);
						fs::path oldCache = (home / ".cache/performous");
						fs::remove_all(oldCache);
						didMigrateConfig = true;
					}
					catch (...) {
						throw std::runtime_error("There was an error migrating configuration to " + conf.string());
					}
					}
				}
				if (didMigrateConfig) { std::clog << "fs/info: Succesfully moved configuration files to their new location: " << conf.string() << std::endl; }
				}
			// Data dirs
			std::string logmsg = "fs/info: Determining data dirs (search path):\n";
			{
				Paths dirs;
				dirs.push_back(data);  // Adding user's data dir
				dirs.push_back(share);  // Adding system data dir (relative to performous executable or PERFORMOUS_ROOT)
				
				// Adding XDG_DATA_DIRS
				{
					char const* xdg_data_dirs = getenv("XDG_DATA_DIRS");
					std::istringstream iss(xdg_data_dirs ? xdg_data_dirs : "/usr/local/share/:/usr/share/");
					for (std::string p; std::getline(iss, p, ':'); dirs.push_back(p / performous)) {}
				}
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