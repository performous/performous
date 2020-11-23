#pragma once

#include "video_driver.hh"

class YCoordinate {
	float m_y = 0.0f;

  public:
	float raw() const { return m_y; }
	/// Constructor/Destructor
	YCoordinate(): m_y(0.0f) {};
	YCoordinate(float y): m_y(y) {}; ///< Construct from a float value.
	~YCoordinate(){};
	/// Copy from another YCoordinate.
	YCoordinate(const YCoordinate& rhs): m_y(rhs.raw()) {};  
	YCoordinate(YCoordinate& rhs): m_y(rhs.raw()) {};

	/// Conversion functions
	explicit operator const float() const { return raw(); }
	explicit operator float() { return raw(); }
	float operator ()() const { return raw() * Window::virtH(); }
	explicit operator double() { return static_cast<double>(raw()); }
	explicit operator long double() { return static_cast<long double>(raw()); }
	explicit operator std::string() { 
		std::ostringstream os;
		os << *this;
		return os.str();
	}
	explicit operator const std::string() const { 
		std::ostringstream os;
		os << *this;
		return os.str();
	}	
	friend std::ostream& operator<<(std::ostream& os, YCoordinate& _yc) { return (os << std::to_string(_yc.raw())); }
	friend std::ostream& operator<<(std::ostream& os, YCoordinate const& _yc) { return (os << std::to_string(_yc.raw())); }

	/// Assignment
	YCoordinate operator =(float rhs) { 
		m_y = rhs;
		return *this;
	}
	YCoordinate operator =(YCoordinate const& rhs) {
		if (this != &rhs) {
			m_y = rhs.raw();
		}
		return *this;
	}
	/// Binary Assignment
	YCoordinate& operator +=(float rhs) {
		m_y = raw() + rhs;
		return *this;
	}
	YCoordinate& operator -=(float rhs) {
		m_y = raw() - rhs;
		return *this;
	}
	YCoordinate& operator *=(float rhs) {
		m_y = raw() * rhs;
		return *this;
	}
	YCoordinate& operator /=(float rhs) {
		m_y = raw() / rhs;
		return *this;
	}
	YCoordinate& operator +=(YCoordinate const& rhs) { return (*this += rhs.raw()); }
	YCoordinate& operator -=(YCoordinate const& rhs) { return (*this -= rhs.raw()); }
	YCoordinate& operator *=(YCoordinate const& rhs) { return (*this *= rhs.raw()); }
	YCoordinate& operator /=(YCoordinate const& rhs) { return (*this /= rhs.raw()); }
	
	/// Binary Arithmetic
	friend YCoordinate& operator +(YCoordinate lhs, float rhs) { return (lhs += rhs); }
	friend YCoordinate& operator -(YCoordinate lhs, float rhs) { return (lhs -= rhs); }
	friend YCoordinate& operator *(YCoordinate lhs, float rhs) { return (lhs *= rhs); }
	friend YCoordinate& operator /(YCoordinate lhs, float rhs) { return (lhs /= rhs); }

	friend YCoordinate operator +(YCoordinate lhs, YCoordinate const& rhs) { return (lhs += rhs); }
	friend YCoordinate operator -(YCoordinate lhs, YCoordinate const& rhs) { return (lhs -= rhs); }
	friend YCoordinate operator *(YCoordinate lhs, YCoordinate const& rhs) { return (lhs *= rhs); }
	friend YCoordinate operator /(YCoordinate lhs, YCoordinate const& rhs) { return (lhs /= rhs); }

	/// Comparison
	friend bool operator== (YCoordinate const& lhs, float rhs){ return (lhs.raw() == rhs); }
	friend bool operator== (YCoordinate const& lhs, YCoordinate const& rhs) { return (lhs == rhs.raw()); }
	friend bool operator!= (YCoordinate const& lhs, float rhs){ return !(lhs == rhs); }
	friend bool operator!= (YCoordinate const& lhs, YCoordinate const& rhs) { return !(lhs == rhs); }
	friend bool operator< (YCoordinate const& lhs, float rhs) { return (lhs.raw() < rhs); }
	friend bool operator< (YCoordinate const& lhs, YCoordinate const& rhs) { return (lhs < rhs.raw()); }
	friend bool operator> (YCoordinate const& lhs, float rhs) { return (rhs < lhs); }
	friend bool operator> (YCoordinate const& lhs, YCoordinate const& rhs) { return (rhs < lhs); }
	friend bool operator>= (YCoordinate const& lhs, float rhs) { return !(lhs < rhs); }
	friend bool operator>= (YCoordinate const& lhs, YCoordinate const& rhs) { return !(lhs < rhs); }
	friend bool operator<= (YCoordinate const& lhs, float rhs) { return !(rhs < lhs); }
	friend bool operator<= (YCoordinate const& lhs, YCoordinate const& rhs) { return !(rhs < lhs); }
};