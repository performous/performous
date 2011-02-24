#version 120

//DEFINES

varying float bogus;

in vec4 vertPos;
uniform mat4 colorMatrix;
varying mat4 colorMat;

in vec4 vertTexCoord;
varying vec4 texCoord;
varying vec4 vTexCoord;

in vec3 vertNormal;
varying vec3 normal;
varying vec3 vNormal;

in vec4 vertColor;
varying vec4 color;
varying vec4 vColor;

void main() {
	bogus = 0.0;
	colorMat = colorMatrix;  // In case no geometry shader is used (otherwise it sets this)
	gl_Position = gl_ModelViewProjectionMatrix * vertPos;
	vTexCoord = texCoord = vertTexCoord;
	vNormal = normal = normalize(gl_NormalMatrix * vertNormal);
	vColor = color = vertColor;
}

