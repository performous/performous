#version 120

// Input from glVertexAttribPointer
/* layout (location = 0) */ in vec4 vertPos;
/* layout (location = 1) */ in vec4 vertTexCoord;
/* layout (location = 2) */ in vec3 vertNormal;
/* layout (location = 3) */ in vec4 vertColor;

varying float bogus;

uniform mat4 colorMatrix;
varying mat4 colorMat;

// Per-vextex for fragment shader (if no geometry shader)
varying vec4 texCoord;
varying vec3 normal;
varying vec4 color;

// Per-vertex for geometry shader (if one exists)
varying vec4 vTexCoord;
varying vec3 vNormal;
varying vec4 vColor;

void main() {
	bogus = 0.0;
	colorMat = colorMatrix;  // In case no geometry shader is used (otherwise it sets this)
	gl_Position = gl_ModelViewProjectionMatrix * vertPos;
	vTexCoord = texCoord = vertTexCoord;
	vNormal = normal = normalize(gl_NormalMatrix * vertNormal);
	vColor = color = vertColor;
}

