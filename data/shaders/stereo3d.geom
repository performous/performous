#version 330
#extension GL_ARB_viewport_array : require

uniform float sepFactor;
uniform float z0;

layout(triangles) in;
layout(triangle_strip, max_vertices = 6) out;

in vec4 vTexCoord[];
in vec3 vNormal[];
in vec4 vColor[];

out float bogus;  // Workaround for http://www.nvnews.net/vbulletin/showthread.php?p=2401097
out vec4 texCoord;
out vec3 normal;
out vec4 color;

int i;

void passthru() {
	gl_Position = gl_in[i].gl_Position;
	texCoord = vTexCoord[i];
	normal = vNormal[i];
	color = vColor[i];
}

// Process all the vertices, applying code to them before emitting (do-while to convince Nvidia of the code getting executed)
#define PROCESS(code) i = 0; do { passthru(); code; EmitVertex(); } while (++i < gl_in.length()); EndPrimitive();

void main() {
	bogus = 0.0;
	if (sepFactor == 0.0) {
		gl_ViewportIndex = 0; PROCESS(); // No stereo
	} else {
		gl_ViewportIndex = 1; PROCESS(gl_Position.x -= sepFactor * (gl_Position.z - z0));  // Left eye
		gl_ViewportIndex = 2; PROCESS(gl_Position.x += sepFactor * (gl_Position.z - z0));  // Right eye
	}
}

