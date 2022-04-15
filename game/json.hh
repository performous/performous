#pragma once

#include "fs.hh"

#include <nlohmann/json.hpp>

#include <optional>

nlohmann::json readJSON(fs::path const& filename); ///< Reads a JSON file into memory.
void writeJSON(nlohmann::json const& json, fs::path const& filename); ///< Write a JSON file.

template <typename T> std::optional<T> getJsonEntry(nlohmann::json const& json, const char *name) {
        if (json.contains(name)) return json.at(name).get<T>();
        return {};
}
