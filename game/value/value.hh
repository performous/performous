#pragma once

#include "ivalue.hh"
#include "constant_value_provider.hh"

#include <chrono>
#include <vector>

class Value : public IValue {
public:
	Value(float value = 0.f);
	Value(ValuePtr ptr);
	Value(Value const&);

	Value& operator=(Value const&);

	float get() const override;
	operator float() const override;

	ValuePtr clone() const override;

	ValuePtr ptr() const;

private:
	ValuePtr m_value;
};

namespace value {
	ValuePtr Float(float value);

	ValuePtr Time();

	ValuePtr Min(std::vector<Value> const& values);
	ValuePtr Max(std::vector<Value> const& values);
	ValuePtr Clamp(Value const& value, Value const& min, Value const& max);

	ValuePtr Negate(Value const& value);

	ValuePtr Add(Value const& valueA, Value const& valueB);
	ValuePtr Subtract(Value const& valueA, Value const& valueB);

	ValuePtr Multiply(Value const& valueA, Value const& valueB);

	ValuePtr Mix(Value const& valueA, Value const& valueB, Value const& a);

	ValuePtr Sinus(Value const& value);
	ValuePtr SquareRoot(Value const& value);

	ValuePtr Random();
	ValuePtr Random(Value const& min, Value const& max);

	ValuePtr Constant(std::string const&, ConstantValueProviderPtr);
}
