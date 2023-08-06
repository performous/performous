#pragma once

template<class Type>
class Cycle {
  public:
	Cycle(Type value = 0, Type max = 1, Type min = 0);

	Type get() const;
	Cycle& set(Type value);
	Cycle& forward();
	Cycle& backward();

	operator Type() const;
	Cycle& operator=(Type value);

	Type getMin() const;
	Type getMax() const;

private:
	Type m_min;
	Type m_max;
	Type m_value;
};


#include <stdexcept>

template<class Type>
Cycle<Type>::Cycle(Type value, Type max, Type min)
: m_min(min), m_max(max), m_value(value) {
	if(min >= max) {
		throw std::logic_error("min must less than max");
	}
	if(value > max) {
		throw std::logic_error("value must less or equal than max");
	}
	if(value < min) {
		throw std::logic_error("value must greater or equal than min");
	}
}

template<class Type>
Type Cycle<Type>::get() const {
	return m_value;
}

template<class Type>
Cycle<Type>& Cycle<Type>::set(Type value) {
	if(value > m_max) {
		throw std::logic_error("value must less or equal than max");
	}
	if(value < m_min) {
		throw std::logic_error("value must greater or equal than min");
	}

	m_value = value;

	return *this;
}

template<class Type>
Cycle<Type>& Cycle<Type>::operator=(Type value) {
	return set(value);
}

template<class Type>
Cycle<Type>& Cycle<Type>::forward() {
	if(m_value == m_max) {
		m_value = m_min;
	}
	else {
		++m_value;
	}

	return *this;
}

template<class Type>
Cycle<Type>& Cycle<Type>::backward() {
	if(m_value == m_min) {
		m_value = m_max;
	}
	else {
		--m_value;
	}

	return *this;
}

template<class Type>
Cycle<Type>::operator Type() const {
	return m_value;
}

template<class Type>
Type Cycle<Type>::getMin() const {
	return m_min;
}

template<class Type>
Type Cycle<Type>::getMax() const {
	return m_max;
}
