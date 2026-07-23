#pragma once

#include "ui/path/ipath.hh"

struct IPathProvider {
	virtual ~IPathProvider() = default;

	virtual PathPtr getPath() const = 0;
};

using PathProviderPtr = std::shared_ptr<IPathProvider>;
