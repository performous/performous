#pragma once

#include <memory>

struct IValue;

using ValuePtr = std::shared_ptr<IValue>;

struct IValue {
	virtual ~IValue() = default;

	virtual float get() const = 0;
	virtual operator float() const = 0;

	virtual ValuePtr clone() const = 0;
};

