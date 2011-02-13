#version 330
#extension GL_ARB_viewport_array : require

layout(triangles) in;
layout(triangle_strip, max_vertices = 6) out;

in vec4 vTexCoord[];
in vec3 vNormal[];
in vec4 vColor[];

out mat4 colorMat;
out vec4 texCoord;
out vec3 normal;
out vec4 color;

uniform mat4 colorMatrix;
uniform float sepFactor;
uniform float z0;

void main() {
	colorMat = colorMatrix;  // Supply color matrix for fragment shader
	// Fix Nvidia compile warnings ("might be used uninitialized")
	gl_Position = vec4(0);
	texCoord = vec4(0);
	normal = vec3(0);
	color = vec4(0);
	if (sepFactor == 0.0) {
		// No stereo
		gl_ViewportIndex = 0;
		for(int i=0; i < gl_in.length(); ++i) {
			gl_Position = gl_in[i].gl_Position;
			texCoord = vTexCoord[i];
			normal = vNormal[i];
			EmitVertex();
		}
		EndPrimitive();
	} else {
		// Render the left eye
		gl_ViewportIndex = 0;
		for(int i=0; i < gl_in.length(); ++i) {
			gl_Position = gl_in[i].gl_Position;
			gl_Position.x -= sepFactor * (gl_Position.z - z0);
			texCoord = vTexCoord[i];
			normal = vNormal[i];
			EmitVertex();
		}
		EndPrimitive();
		// Render the right eye
		gl_ViewportIndex = 1;
		for(int i=0; i < gl_in.length(); ++i) {
			gl_Position = gl_in[i].gl_Position;
			gl_Position.x += sepFactor * (gl_Position.z - z0);
			texCoord = vTexCoord[i];
			normal = vNormal[i];
			EmitVertex();
		}
		EndPrimitive();
	}
}

