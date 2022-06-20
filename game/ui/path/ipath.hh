#pragma once

#include "ui/point.hh"

#include <memory>

struct IPath {
	virtual ~IPath() = default;

	virtual Point getPoint(float location) const = 0;
	virtual float getLength() const = 0;
};

using PathPtr = std::shared_ptr<IPath>;
