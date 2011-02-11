#version 330
#extension GL_ARB_viewport_array : require

layout(triangles) in;
layout(triangle_strip, max_vertices = 6) out;

uniform mat4 colorMatrix;
uniform float sepFactor;
uniform float z0;

out mat4 colorMat;
out vec2 texcoord;

void main() {
	colorMat = colorMatrix;  // Supply color matrix for fragment shader
	// Fix Nvidia compile warnings ("might be used uninitialized")
	texcoord = vec2(0);
	gl_Position = vec4(0);
	if (sepFactor == 0.0) {
		// No stereo
		gl_ViewportIndex = 0;
		for(int i=0; i < gl_in.length(); ++i) {
			texcoord = gl_in[i].gl_TexCoord[0].st;
			gl_Position = gl_in[i].gl_Position;
			EmitVertex();
		}
		EndPrimitive();
	} else {
		// Render the left eye
		gl_ViewportIndex = 0;
		for(int i=0; i < gl_in.length(); ++i) {
			texcoord = gl_in[i].gl_TexCoord[0].st;
			gl_Position = gl_in[i].gl_Position;
			gl_Position.x -= sepFactor * (gl_Position.z - z0);
			EmitVertex();
		}
		EndPrimitive();
		// Render the right eye
		gl_ViewportIndex = 1;
		for(int i=0; i < gl_in.length(); ++i) {
			texcoord = gl_in[i].gl_TexCoord[0].st;
			gl_Position = gl_in[i].gl_Position;
			gl_Position.x += sepFactor * (gl_Position.z - z0);
			EmitVertex();
		}
		EndPrimitive();
	}
}

