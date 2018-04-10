#version 330 core
#extension GL_ARB_viewport_array : require

uniform float sepFactor;
uniform float z0;

layout(triangles) in;
layout(triangle_strip, max_vertices = 6) out;

struct vData {
	vec3 lightDir;
	vec2 texCoord;
	vec3 normal;
	vec4 color;
};

in vData vertex[];
out vData fragv;
flat out int viewp;

void passthru(int i) {
	gl_Position = gl_in[i].gl_Position;
	fragv = vertex[i];
	viewp = gl_ViewportIndex;
}

// Process all the vertices, applying code to them before emitting (do-while to convince Nvidia of the code getting executed)
#define PROCESS(code) i = 0; do { passthru(i); code; EmitVertex(); } while (++i < 3); EndPrimitive();

void main() {
	int i = 0;
	if (sepFactor == 0.0) {
		gl_ViewportIndex = 0; PROCESS(;); // No stereo
	} else {
		gl_ViewportIndex = 1; PROCESS(gl_Position.x -= sepFactor * (gl_Position.z - z0));  // Left eye
		gl_ViewportIndex = 2; PROCESS(gl_Position.x += sepFactor * (gl_Position.z - z0));  // Right eye
	}
}

