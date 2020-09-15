#ifdef GL_IS_ES
#version 310 es
#extension GL_OES_viewport_array : require
#extension GL_OES_geometry_shader : require
#else
#version 330 core
#extension GL_ARB_viewport_array : require
#endif
precision highp float;

layout (std140) uniform stereoParams {
	float sepFactor;
	float z0;
	float padding[2];
};

layout(triangles) in;
layout(triangle_strip, max_vertices = 6) out;

in vData {
	vec3 lightDir;
	vec2 texCoord;
	vec3 normal;
	vec4 color;
} vertices[];

out vData {
	vec3 lightDir;
	vec2 texCoord;
	vec3 normal;
	vec4 color;
} fragv;

void passthru(int vp, int i) {
	gl_ViewportIndex = (sepFactor == 0.0) ? 0 : vp;
	fragv.lightDir = vertices[i].lightDir;
	fragv.texCoord = vertices[i].texCoord;
	fragv.normal = vertices[i].normal;
	fragv.color = vertices[i].color;
}

void main() {
// Render the left eye
	for (int i=0; i < gl_in.length(); i++) {
		passthru(1, i);
		gl_Position = gl_in[i].gl_Position;
		gl_Position.x -= (sepFactor * (gl_in[i].gl_Position.z - z0));
		EmitVertex();
	}
	EndPrimitive();
	
// Render the right eye
	for (int i=0; i < gl_in.length(); i++) {
		passthru(2, i);
		gl_Position = gl_in[i].gl_Position;
		gl_Position.x += (sepFactor * (gl_in[i].gl_Position.z - z0));
		EmitVertex();
	}
	EndPrimitive();
}