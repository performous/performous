
//DEFINES

#ifdef TEXTURE_RECT
#extension GL_ARB_texture_rectangle : enable
#define SAMPLER sampler2DRect
#define TEXTURE texture2DRect
#else
#define SAMPLER sampler2D
#define TEXTURE texture2D
#endif

uniform mat4 colorMatrix;
uniform SAMPLER tex;

void main() {
	gl_FragColor = colorMatrix * (gl_Color * TEXTURE(tex, gl_TexCoord[0].st).rgba);
}

