#pragma once

#include <GL/glew.h>
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <sstream>

namespace glmath {

	struct Vec3 {
		GLfloat x, y, z;
		explicit Vec3(float x = 0.0, float y = 0.0, float z = 0.0): x(x), y(y), z(z) {}
	};

	struct Vec4 {
		GLfloat x, y, z, w;
		explicit Vec4(GLfloat const* arr) { std::copy(arr, arr + 4, &x); }
		explicit Vec4(float x = 0.0, float y = 0.0, float z = 0.0, float w = 0.0): x(x), y(y), z(z), w(w) {}
		explicit Vec4(Vec3 const& v, float w = 1.0): x(v.x), y(v.y), z(v.z), w(w) {}
		GLfloat& operator[](unsigned j) { return (&x)[j]; }
		GLfloat const& operator[](unsigned j) const { return (&x)[j]; }
	};

	static inline Vec3 operator*(float k, Vec3 const& v) { return Vec3(k * v.x, k * v.y, k * v.z); }

	static inline float dot(Vec3 const& a, Vec3 const& b) {
		return a.x * b.x + a.y * b.y + a.z * b.z;
	}

	static inline float len(Vec3 const& v) { return std::sqrt(dot(v, v)); }
	static inline Vec3 normalize(Vec3 const& v) { return (1 / len(v)) * v; }

	struct Matrix {
		static Matrix zero() {
			Matrix ret;
			ret(0,0) = ret(1,1) = ret(2,2) = ret(3,3) = 0.0;
			return ret;
		}
		static Matrix diagonal(Vec4 const& v) {
			Matrix ret;
			for (unsigned i = 0; i < 4; ++i) ret(i,i) = v[i];
			return ret;
		}
		Vec4 cols[4];
		/// Identity matrix
		Matrix() { for (unsigned k = 0; k < 4; ++k) cols[k][k] = 1.0; }
		operator GLfloat*() { return &cols[0][0]; }
		operator GLfloat const*() const { return &cols[0][0]; }
		GLfloat& operator()(unsigned i, unsigned j) { return cols[j][i]; }
		GLfloat const& operator()(unsigned i, unsigned j) const { return cols[j][i]; }
	};

	static inline std::ostream& operator<<(std::ostream& os, Matrix const& m) {
		std::ostringstream oss;
		oss << std::setprecision(3) << std::fixed;
		for (int i = 0; i < 4; ++i) {
			for (int j = 0; j < 4; ++j) {
				oss.width(7);
				oss << m(i,j);
			}
			oss << '\n';
		}
		return os << oss.str() << std::endl;
	}

	static inline Matrix get(GLenum mode_matrix) {
		Matrix ret;
		glGetFloatv(mode_matrix, ret);
		return ret;
	}

	static inline Matrix getMatrix() {
		GLint mode;
		glGetIntegerv(GL_MATRIX_MODE, &mode);
		if (mode == GL_MODELVIEW) return get(GL_MODELVIEW_MATRIX);
		if (mode == GL_PROJECTION) return get(GL_PROJECTION_MATRIX);
		if (mode == GL_TEXTURE) return get(GL_TEXTURE_MATRIX);
		throw std::logic_error("Unknown current matrix mode in glmath::get()");
	}
	static inline void upload(Matrix const& m) { glLoadMatrixf(m); }

	static inline Matrix operator*(Matrix const& a, Matrix const& b) {
		Matrix ret;
		for (unsigned i = 0; i < 4; ++i) {
			for (unsigned j = 0; j < 4; ++j) {
				GLfloat sum = 0.0;
				for (unsigned k = 0; k < 4; ++k) {
					sum += a(i, k) * b(k, j);
				}
				ret(i, j) = sum;
			}
		}
		return ret;
	}

	static inline Matrix translate(Vec3 const& v) {
		Matrix ret;
		ret(0,3) = v.x;
		ret(1,3) = v.y;
		ret(2,3) = v.z;
		return ret;
	}

	static inline Matrix scale(Vec3 const& v) {
		Matrix ret;
		ret(0,0) = v.x;
		ret(1,1) = v.y;
		ret(2,2) = v.z;
		return ret;
	}

	static inline Matrix scale(float k) { return scale(Vec3(k, k, k)); }

	static inline Matrix rotate(float rad, Vec3 axis) {
		Matrix ret;
		Vec3 u = normalize(axis);
		// Based on http://en.wikipedia.org/wiki/Rotation_matrix#Rotation_matrix_from_axis_and_angle
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

	static inline Matrix frustum(float l, float r, float b, float t, float n, float f) {
		float w = r - l;
		float h = t - b;
		float d = n - f;
		Matrix ret;
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

