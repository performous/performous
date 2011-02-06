
//DEFINES

uniform mat4 colorMatrix;

#ifdef SURFACE
#extension GL_ARB_texture_rectangle : require
uniform sampler2DRect tex;
#define TFUNC texture2DRect
#endif

#ifdef TEXTURE
uniform sampler2D tex;
#define TFUNC texture2D
#endif

void main() {
	gl_FragColor = colorMatrix * (gl_Color
#ifdef TFUNC
	  * TFUNC(tex, gl_TexCoord[0].st).rgba
#endif
	  );
}

