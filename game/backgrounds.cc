#include "backgrounds.hh"

#include "configuration.hh"
#include "fs.hh"

#include <boost/bind.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <cstdlib>

void Backgrounds::reload() {
	if (m_loading) return;
	// Run loading thread
	m_loading = true;
	m_thread.reset(new boost::thread(boost::bind(&Backgrounds::reload_internal, boost::ref(*this))));
}

void Backgrounds::reload_internal() {
	{
		boost::mutex::scoped_lock l(m_mutex);
		m_bgs.clear();
		m_dirty = true;
	}
	Paths paths = getPaths();
	for (Paths::iterator it = paths.begin(); m_loading && it != paths.end(); ++it) {
		*it /= "backgrounds";
		if (!fs::is_directory(*it)) { std::cout << ">>> Not scanning for backgrounds: " << *it << " (no such directory)" << std::endl; continue; }
		std::cout << ">>> Scanning " << *it << " (for backgrounds)" << std::endl;
		size_t count = m_bgs.size();
		reload_internal(*it);
		size_t diff = m_bgs.size() - count;
		if (diff > 0 && m_loading) std::cout << diff << " backgrounds loaded" << std::endl;
	}
	m_loading = false;
	{	
		boost::mutex::scoped_lock l(m_mutex);
		random_shuffle(m_bgs.begin(), m_bgs.end());
		m_dirty = false;
		m_bgiter = 0;
	}
}

void Backgrounds::reload_internal(fs::path const& parent) {
	namespace fs = fs;
	if (std::distance(parent.begin(), parent.end()) > 20) { std::cout << ">>> Not scanning: " << parent.string() << " (maximum depth reached, possibly due to cyclic symlinks)" << std::endl; return; }
	try {
		boost::regex expression("(.*\\.(png|jpeg|jpg|svg|bmp|gif))$", boost::regex_constants::icase);
		boost::cmatch match;
		for (fs::directory_iterator dirIt(parent), dirEnd; m_loading && dirIt != dirEnd; ++dirIt) {
			fs::path p = dirIt->path();
			if (fs::is_directory(p)) { reload_internal(p); continue; }
			std::string name = p.leaf(); // File basename
			std::string path = p.directory_string(); // Path without filename
			path.erase(path.size() - name.size());
			if (!regex_match(name.c_str(), match, expression)) continue;
			{
				boost::mutex::scoped_lock l(m_mutex);
				m_bgs.push_back(path + name);
				m_dirty = true;
			}
		}
	} catch (std::exception const& e) {
		std::cout << "Error accessing " << parent << std::endl;
	}
}

std::string Backgrounds::getRandom() {
	if (m_bgs.empty()) throw std::runtime_error("No random backgrounds available");
	return m_bgs.at((++m_bgiter) % m_bgs.size());
}

