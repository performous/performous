#include "json.hh"
#include "log.hh"

#include <iostream>
#include <fstream>

nlohmann::json readJSON(fs::path const& filename) {
	nlohmann::json json = nlohmann::json::array();
	std::clog << "json/debug: Will try to parse JSON in: " << filename.string() << std::endl;
	try {
		std::ifstream file(filename.string());
		if (!file.is_open()) throw std::runtime_error("Can't open file.");
		json = nlohmann::json::parse(file);
	} catch(nlohmann::json::exception const& e) {
		std::clog << "json/error: " << e.what() << std::endl;
	} catch(std::exception const& e) {
		std::clog << "fs/error: " << e.what() << std::endl;
	}
	return json;
}

void writeJSON(nlohmann::json const& json, fs::path const& filename) {
	try {
		std::ofstream outFile(filename.string());
		if (!outFile.is_open()) throw std::runtime_error("Can't open file.");
		const int spacesCount = 4;
		outFile << json.dump(spacesCount);
		std::clog << "json/info: saved " + std::to_string(json.size()) + " objects to JSON file: " + filename.string() << std::endl;
	} catch (nlohmann::json::exception const& e) {
		std::clog << "json/error: Could not serialize json." << std::endl;
	} catch (std::exception const& e) {
		std::clog << "fs/error: Could not save " + filename.string() + ": " + e.what() << std::endl;
	}
}
