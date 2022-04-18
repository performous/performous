#include <nlohmann/json.hpp>

nlohmann::json readJSON(fs::path const& filename); ///< Reads a JSON file into memory.
void writeJSON(nlohmann::json const& json, fs::path const& filename); ///< Write a JSON file.
