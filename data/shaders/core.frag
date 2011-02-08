#version 120
#extension GL_ARB_texture_rectangle : require

//DEFINES

#ifdef SURFACE
uniform sampler2DRect tex;
in vec2 texcoord;
#define TEXFUNC texture2DRect(tex, texcoord)
#endif

#ifdef TEXTURE
uniform sampler2D tex;
in vec2 texcoord;
#define TEXFUNC texture2D(tex, texcoord)
#endif

#ifndef TEXFUNC
#define TEXFUNC vec4(1,1,1,1)
#endif

uniform mat4 colorMatrix;

void main() {
	gl_FragColor = colorMatrix * TEXFUNC;
}

