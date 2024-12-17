#include "json.hh"
#include "log.hh"
#include "fs.hh"

#include <iostream>
#include <fstream>
#include <system_error>
#include <cerrno>

nlohmann::json readJSON(fs::path const& filename) {
	nlohmann::json json = nlohmann::json::array();
	SpdLogger::debug(LogSystem::JSON, "Parsing JSON file, path={}", filename);
	try {
		std::ifstream file(filename.string());
		if (!file.is_open()) {
			throw fs::filesystem_error(
				"Can't open file for reading", filename, std::error_code(errno, std::generic_category())
				);
		}
		json = nlohmann::json::parse(file);
	} catch(nlohmann::json::exception const& e) {
		SpdLogger::error(LogSystem::JSON, "Error parsing file, exception={}", e.what());
	} catch(fs::filesystem_error const& e) {
		SpdLogger::error(LogSystem::FILESYSTEM, "Cannot parse JSON file, exception={}", e.what());
	}
	return json;
}

void writeJSON(nlohmann::json const& json, fs::path const& filename) {
	try {
		std::ofstream outFile(filename.string());
		if (!outFile.is_open()) {
			throw fs::filesystem_error(
				"Can't open file for writing", filename, std::error_code(errno, std::generic_category())
				);
		}
		const int spacesCount = 4;
		outFile << json.dump(spacesCount);
		SpdLogger::info(LogSystem::JSON, "Saved {} objects to JSON file path={}", json.size(), filename);
	} catch (nlohmann::json::exception const& e) {
		SpdLogger::error(LogSystem::JSON, "Cannot serialize JSON, exception={}", e.what());
	} catch (fs::filesystem_error const& e) {
		SpdLogger::error(LogSystem::FILESYSTEM, "Cannot save JSON file, exception={}", e.what());
	}
}
