#version 120

//DEFINES

#ifdef ENABLE_BOGUS
varying float bogus;
#endif

in vec4 vertPos;
uniform mat4 colorMatrix;
varying mat4 colorMat;

#ifdef ENABLE_TEXTURING
in vec4 vertTexCoord;
varying vec4 texCoord;
varying vec4 vTexCoord;
#endif

#ifdef ENABLE_LIGHTING
in vec3 vertNormal;
varying vec3 normal;
varying vec3 vNormal;
#endif

#ifdef ENABLE_VERTEX_COLOR
in vec4 vertColor;
varying vec4 color;
varying vec4 vColor;
#endif

void main() {
#ifdef ENABLE_BOGUS
	bogus = 0.0;
#endif
	colorMat = colorMatrix;  // In case no geometry shader is used (otherwise it sets this)
	gl_Position = gl_ModelViewProjectionMatrix * vertPos;
#ifdef ENABLE_TEXTURING
	vTexCoord = texCoord = vertTexCoord;
#endif
#ifdef ENABLE_LIGHTING
	vNormal = normal = normalize(gl_NormalMatrix * vertNormal);
#endif
#ifdef ENABLE_VERTEX_COLOR
	vColor = color = vertColor;
#endif
}

