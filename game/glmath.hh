#pragma once

#include <epoxy/gl.h>
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <boost/math/constants/constants.hpp>

// extern const double m_pi;

namespace glmath {

	template <typename T> T mix(T const& a, T const& b, double blend) {
		return (1.0-blend) * a + blend * b;
	}

	struct vec4;

	struct vec2 {
		GLfloat x, y;
		explicit vec2(GLfloat const* arr) { std::copy(arr, arr + 2, &x); }
		explicit vec2(float x = 0.0, float y = 0.0): x(x), y(y) {}
		GLfloat& operator[](unsigned j) { return (&x)[j]; }
		GLfloat const& operator[](unsigned j) const { return (&x)[j]; }
	};

	struct vec3 {
		GLfloat x, y, z;
		explicit vec3(GLfloat const* arr) { std::copy(arr, arr + 3, &x); }
		explicit vec3(float x = 0.0, float y = 0.0, float z = 0.0): x(x), y(y), z(z) {}
		explicit vec3(vec4 const& v);
		GLfloat& operator[](unsigned j) { return (&x)[j]; }
		GLfloat const& operator[](unsigned j) const { return (&x)[j]; }
	};

	struct vec4 {
		GLfloat x, y, z, w;
		explicit vec4(GLfloat const* arr) { std::copy(arr, arr + 4, &x); }
		explicit vec4(float x = 0.0, float y = 0.0, float z = 0.0, float w = 0.0): x(x), y(y), z(z), w(w) {}
		explicit vec4(vec3 const& v, float w = 0.0): x(v.x), y(v.y), z(v.z), w(w) {}
		GLfloat& operator[](unsigned j) { return (&x)[j]; }
		GLfloat const& operator[](unsigned j) const { return (&x)[j]; }
	};

	inline vec3::vec3(vec4 const& v): x(v.x), y(v.y), z(v.z) {}

	// Template functions to allow them to work with different types of floating-point values

	template <typename Scalar> static inline vec2 operator*(Scalar k, vec2 const& v) { return vec2(k * v.x, k * v.y); }
	template <typename Scalar> static inline vec3 operator*(Scalar k, vec3 const& v) { return vec3(k * v.x, k * v.y, k * v.z); }
	template <typename Scalar> static inline vec4 operator*(Scalar k, vec4 const& v) { return vec4(k * v.x, k * v.y, k * v.z, k * v.w); }

	static inline vec3 operator+(vec3 const& a, vec4 const& b) { return vec3(a.x + b.x, a.y + b.y, a.z + b.z); }
	static inline vec4 operator+(vec4 const& a, vec4 const& b) { return vec4(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w); }

	static inline float dot(vec2 const& a, vec2 const& b) {
		return a.x * b.x + a.y * b.y;
	}
	static inline float dot(vec3 const& a, vec3 const& b) {
		return a.x * b.x + a.y * b.y + a.z * b.z;
	}

	static inline float len(vec2 const& v) { return std::sqrt(dot(v, v)); }
	static inline float len(vec3 const& v) { return std::sqrt(dot(v, v)); }
	static inline vec2 normalize(vec2 const& v) { return (1 / len(v)) * v; }
	static inline vec3 normalize(vec3 const& v) { return (1 / len(v)) * v; }

	template <typename Vector> struct mat {
		typedef Vector vector_type;
		static const unsigned dimension = sizeof(Vector) / sizeof(GLfloat);
		static mat zero() { return mat(); }
		static mat identity() {
			mat ret;
			for (unsigned i = 0; i < dimension; ++i) ret(i,i) = 1.0;
			return ret;
		}
		static mat diagonal(Vector const& v) {
			mat ret;
			for (unsigned i = 0; i < dimension; ++i) ret(i,i) = v[i];
			return ret;
		}
		template <typename AnyVec> explicit mat(mat<AnyVec> const& m) {
			for (unsigned i = 0; i < dimension && i < m.dimension; ++i) cols[i] = Vector(m.cols[i]);
		}
		Vector cols[dimension];
		operator GLfloat*() { return &cols[0][0]; }
		operator GLfloat const*() const { return &cols[0][0]; }
		GLfloat& operator()(unsigned i, unsigned j) { return cols[j][i]; }
		GLfloat const& operator()(unsigned i, unsigned j) const { return cols[j][i]; }
	private:
		mat() {}
	};
	
	template <typename Vec> static inline std::ostream& operator<<(std::ostream& os, mat<Vec> const& m) {
		std::ostringstream oss;
		oss << std::setprecision(3) << std::fixed;
		for (int i = 0; i < m.dimension; ++i) {
			for (int j = 0; j < m.dimension; ++j) {
				oss.width(7);
				oss << m(i,j);
			}
			oss << '\n';
		}
		return os << oss.str() << std::endl;
	}

	template <typename Vec> static inline mat<Vec> operator*(mat<Vec> const& a, mat<Vec> const& b) {
		mat<Vec> ret = mat<Vec>::zero();
		for (unsigned i = 0; i < ret.dimension; ++i) {
			for (unsigned j = 0; j < ret.dimension; ++j) {
				GLfloat sum = 0.0;
				for (unsigned k = 0; k < ret.dimension; ++k) {
					sum += a(i, k) * b(k, j);
				}
				ret(i, j) = sum;
			}
		}
		return ret;
	}

	typedef mat<vec3> mat3;
	typedef mat<vec4> mat4;

	static inline mat4 translate(vec3 const& v) {
		mat4 ret = mat4::identity();
		ret(0,3) = v.x;
		ret(1,3) = v.y;
		ret(2,3) = v.z;
		return ret;
	}

	static inline mat4 scale(vec3 const& v) {
		mat4 ret = mat4::identity();
		ret(0,0) = v.x;
		ret(1,1) = v.y;
		ret(2,2) = v.z;
		return ret;
	}

	static inline mat4 scale(float k) { return scale(vec3(k, k, k)); }

	static inline mat4 rotate(float rad, vec3 axis) {
		mat4 ret = mat4::identity();
		vec3 u = normalize(axis);
		// Based on http://en.wikipedia.org/wiki/Rotation_mat4#Rotation_mat4_from_axis_and_angle
		float s = std::sin(rad);
		float c = std::cos(rad);
		float nc = 1 - c;
		// Column 0
		ret(0,0) = c + u.x * u.x * nc;
		ret(1,0) = u.y * u.x * nc + u.z * s;
		ret(2,0) = u.z * u.x * nc - u.y * s;
		// Column 1
		ret(0,1) = u.x * u.y * nc - u.z * s;
		ret(1,1) = c + u.y * u.y * nc;
		ret(2,1) = u.z * u.y * nc + u.x * s;
		// Column 2
		ret(0,2) = u.x * u.z * nc + u.y * s;
		ret(1,2) = u.y * u.z * nc - u.x * s;
		ret(2,2) = c + u.z * u.z * nc;
		return ret;
	}

	static inline mat4 frustum(float l, float r, float b, float t, float n, float f) {
		float w = r - l;
		float h = t - b;
		float d = n - f;
		mat4 ret = mat4::identity();
		ret(0,0) = 2 * n / w;
		ret(1,1) = 2 * n / h;
		ret(0,2) = (r + l) / w;
		ret(1,2) = (t + b) / h;
		ret(2,2) = (f + n) / d;
		ret(3,2) = -1.0;
		ret(2,3) = 2 * f * n / d;
		ret(3,3) = 0.0;
		return ret;
	}
}

