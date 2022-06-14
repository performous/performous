#include "backgrounds.hh"
#include "fs.hh"

#include "configuration.hh"
#include <algorithm>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <random>
#include <regex>

void Backgrounds::reload() {
	if (m_loading) return;
	// Run loading thread
	m_loading = true;
	m_thread = std::make_unique<std::thread>([this] { reload_internal(); });
}

void Backgrounds::reload_internal() {
	{	// Remove old ones
		std::lock_guard<std::mutex> l(m_mutex);
		m_bgs.clear();
		m_dirty = true;
	}
	// Go through the background paths
	Paths paths = getPaths();
	for (auto it = paths.begin(); m_loading && it != paths.end(); ++it) {
		if (!m_loading) break;
		*it /= "backgrounds";
		if (!fs::is_directory(*it)) { std::clog << "backgrounds/info: >>> Not scanning for backgrounds: " << *it << " (no such directory)" << std::endl; continue; }
		std::clog << "backgrounds/info: >>> Scanning " << *it << " (for backgrounds)" << std::endl;
		size_t count = m_bgs.size();
		reload_internal(*it); // Scan the found folder
		size_t diff = m_bgs.size() - count;
		if (diff > 0 && m_loading) std::clog << "backgrounds/info: " << diff << " backgrounds loaded" << std::endl;
	}
	m_loading = false;
	{	// Randomize the order
		std::lock_guard<std::mutex> l(m_mutex);
		std::shuffle(m_bgs.begin(), m_bgs.end(), std::mt19937(std::random_device()()));
		m_dirty = false;
		m_bgiter = 0;
	}
}

void Backgrounds::reload_internal(fs::path const& parent) {
	if (std::distance(parent.begin(), parent.end()) > 20) { std::clog << "backgrounds/info: >>> Not scanning: " << parent.string() << " (maximum depth reached, possibly due to cyclic symlinks)" << std::endl; return; }
	try {
		// Find suitable file formats
		std::regex expression(R"(\.(png|jpeg|jpg|svg)$)", std::regex_constants::icase);
		for (fs::directory_iterator dirIt(parent), dirEnd; m_loading && dirIt != dirEnd; ++dirIt) {
			fs::path p = dirIt->path();
			if (fs::is_directory(p)) { reload_internal(p); continue; }
			std::string name = p.filename().string(); // File basename
			std::string path = p.string(); // Path without filename
			path.erase(path.size() - name.size());
			if (!regex_search(name, expression)) continue;
			{
				std::lock_guard<std::mutex> l(m_mutex);
				m_bgs.push_back(path + name); // Add the background
				m_dirty = true;
			}
		}
	} catch (std::exception const&) {
		std::cout << "Error accessing " << parent << std::endl;
	}
}

/// Get a random background
std::string Backgrounds::getRandom() {
	if (m_bgs.empty()) throw std::runtime_error("No random backgrounds available");
	// This relies on that the bgs are in random order
	return m_bgs.at((++m_bgiter) % m_bgs.size());
}

