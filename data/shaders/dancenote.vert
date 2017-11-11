#version 330

uniform mat4 projMatrix;
uniform mat4 mvMatrix;
uniform mat3 normalMatrix;
uniform int noteType;
uniform float hitAnim;
uniform float clock;
uniform float scale;
uniform vec2 position;

layout(location = 0) in vec3 vertPos;
layout(location = 1) in vec2 vertTexCoord;
layout(location = 2) in vec3 vertNormal;
layout(location = 3) in vec4 vertColor;

// Per-vextex for fragment shader (if no geometry shader)
out vec3 vLightDir, gLightDir;
out vec2 vTexCoord, gTexCoord;
out vec3 vNormal, gNormal;
out vec4 vColor, gColor;

mat4 scaleMat(in float sc) {
	return mat4(sc,  0,  0,  0,
	             0, sc,  0,  0,
	             0,  0, sc,  0,
	             0,  0,  0,  1);
}

mat4 rotMat(in float ang) {
	float ca = cos(ang);
	float sa = sin(ang);
	return mat4(ca, -sa, 0, 0,
	            sa,  ca, 0, 0,
	             0,   0, 1, 0,
	             0,   0, 0, 1);
}

void main() {
	const vec3 lightPos = vec3(-10.0, 2.0, 15.0);
	
	vTexCoord = vertTexCoord;
	gTexCoord = vTexCoord;
	
	vNormal = normalMatrix * vertNormal;
	gNormal = vNormal;
	
	vColor = vertColor;

	mat4 trans = scaleMat(scale);
	vec4 posEye = mvMatrix * (vec4(position, 0, 0) + trans * vec4(vertPos, 1.0));  // Vertex position in eye space
	
	vLightDir = lightPos - posEye.xyz / posEye.w;  // Light position relative to vertex
	gLightDir = vLightDir;

	// Cursor arrows
	if (noteType == 0) {
		trans *= scaleMat(1.0 - hitAnim * .5);

	// Regular arrows
	} else if (noteType == 1) {
		trans *= scaleMat(1.0 + hitAnim);

	// Holds
	} else if (noteType == 2) {
		trans *= scaleMat(1.0 + hitAnim);

	// Mines
	} else if (noteType == 3) {
		trans *= scaleMat(1.0 + hitAnim);
		float r = radians(mod(clock*360.0, 360.0)); // They rotate!
		trans *= rotMat(r);
	}

	// Glow color for regular arrows or holds
	if (noteType == 1 || noteType == 2) {
		vColor = vec4(
		  min(vColor.r + hitAnim *.5, 1.0),
		  min(vColor.g + hitAnim *.5, 1.0),
		  min(vColor.b + hitAnim *.5, 1.0),
		  max(vColor.a - hitAnim, 0.0)
		);
	}
    gColor = vColor;
    
	gl_Position = projMatrix * mvMatrix * (vec4(position, 0, 0) + trans * vec4(vertPos, 1.0));
}
