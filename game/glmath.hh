#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <epoxy/gl.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_access.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/matrix_operation.hpp>
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace glmath {

typedef glm::highp_vec2 vec2;
typedef glm::highp_vec3 vec3;
typedef glm::highp_vec4 vec4;
typedef glm::highp_dvec4 dvec4;

typedef glm::highp_mat3 mat3;
typedef glm::highp_mat4 mat4;

	// Template functions to allow them to work with different types of floating-point values

	template <typename Scalar> static inline vec2 operator*(Scalar k, vec2 const& v) { return vec2(k * v.x, k * v.y); }
	template <typename Scalar> static inline vec3 operator*(Scalar k, vec3 const& v) { return vec3(k * v.x, k * v.y, k * v.z); }
	template <typename Scalar> static inline vec4 operator*(Scalar k, vec4 const& v) { return vec4(k * v.x, k * v.y, k * v.z, k * v.w); }

	template <typename T> T mix(T const& a, T const& b, double blend) {
		return (1.0-blend) * a + blend * b;
	}
	
	static inline mat4 translate(vec3 const& v) { return glm::translate(mat4(1.0f), v); }
	static inline mat4 scale(vec3 const& v) { return glm::scale(glm::mat4(1.0f), v); }
	static inline mat4 scale(float k) { return glm::scale(mat4(1.0f),vec3(k, k, k)); }
	static inline mat4 diagonal(vec4 const& v) { return glm::diagonal4x4(v); }	
	static inline mat4 rotate(float rad, vec3 axis) { return glm::rotate(mat4(1.0f), rad, glm::normalize(axis)); }
	
}

