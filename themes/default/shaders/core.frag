#version 330
#extension GL_ARB_texture_rectangle : require

//DEFINES

#ifdef SURFACE
uniform sampler2DRect tex;
#define TFUNC texture2DRect
#endif

#ifdef TEXTURE
uniform sampler2D tex;
#define TFUNC texture2D
#endif

uniform mat4 colorMatrix;

in vec4 color;
in vec2 texcoord;

void main() {
	gl_FragColor = colorMatrix * (color
#ifdef TFUNC
	  * TFUNC(tex, texcoord).rgba
#endif
	  );
}

