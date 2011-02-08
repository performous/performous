#version 330 compatibility
#extension GL_ARB_viewport_array : require

layout(triangles) in;
layout(triangle_strip, max_vertices = 6) out;

uniform mat4 leftTransform;
uniform mat4 rightTransform;

out vec4 color;
out vec2 texcoord;

void main() {
	// Fix Nvidia compile warnings ("may be used uninitialized")
	color = vec4(0);
	texcoord = vec2(0);
	gl_Position = vec4(0);
	// Render the left eye
	gl_ViewportIndex = 0;
	for(int i=0; i < gl_in.length(); ++i) {
		color = gl_in[i].gl_FrontColor;
		texcoord = gl_in[i].gl_TexCoord[0].st;
		gl_Position = leftTransform * gl_in[i].gl_Position;
		EmitVertex();
	}
	EndPrimitive();
	// Render the right eye
	gl_ViewportIndex = 1;
	for(int i=0; i < gl_in.length(); ++i) {
		color = gl_in[i].gl_FrontColor;
		texcoord = gl_in[i].gl_TexCoord[0].st;
		gl_Position = rightTransform * gl_in[i].gl_Position;
		EmitVertex();
	}
	EndPrimitive();
}

