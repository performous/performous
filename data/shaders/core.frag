#version 120
#extension GL_ARB_texture_rectangle : require

//DEFINES

in float bogus;  // Nvidia will overwrite the first in variable with bogus data, so as a workaround we put a bogus variable here
in mat4 colorMat;

#ifdef SURFACE
uniform sampler2DRect tex;
in vec4 texCoord;
#define TEXFUNC texture2DRect(tex, texCoord.st)
#endif

#ifdef TEXTURE
uniform sampler2D tex;
in vec4 texCoord;
#define TEXFUNC texture2D(tex, texCoord.st)
#endif

#ifndef TEXFUNC
#define TEXFUNC vec4(1,1,1,1)
#endif

void main() {
	gl_FragColor = vec4(0,0,0,bogus * 1e-10) + colorMat * TEXFUNC;
}

