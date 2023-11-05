#pragma once

#include <memory>

struct IValue {
	virtual ~IValue() = default;

	virtual float get() const = 0;
	virtual operator float() const = 0;
};

using ValuePtr = std::shared_ptr<IValue>;
