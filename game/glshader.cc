#include "glutil.hh"

namespace {
	const char *vertex_glsl = 
		"void main()"
		"{"
		"	gl_FrontColor = gl_Color;"
		"	gl_BackColor = gl_Color;"
		"	gl_TexCoord[0] = gl_MultiTexCoord0;"
		"	gl_Position = ftransform();"
		"}\000";

	const char *fragment_glsl =
		"uniform int texMode;"
		"uniform sampler2D tex;"
		"uniform sampler2DRect texrect;"
		"void main()"
		"{"
		"	vec4 texel;"
		"	vec4 color;"
		"	if (texMode == 1) {"
		"		texel = texture2D(tex,gl_TexCoord[0].st).rgba;"
		"	} else if (texMode == 2) {"
		"		texel = texture(texrect,gl_TexCoord[0].st).rgba;"
		"	} else if (texMode == 0) {"
		"		texel = gl_Color;"
		"	} else {"
		"		texel = vec4(1.0,0.0,0.0,1.0);"
		"	}"
		"	gl_FragColor = vec4(texel.rgb*gl_Color.rgb,texel.a*gl_Color.a);"
		"}\000";
}

void glshader::newShader(struct glshader::Shader *s) {
	s->vert_shader = glCreateShader(GL_VERTEX_SHADER);
	s->frag_shader = glCreateShader(GL_FRAGMENT_SHADER);

	glShaderSource(s->vert_shader, 1, &vertex_glsl, NULL);
	glShaderSource(s->frag_shader, 1, &fragment_glsl, NULL);

	glCompileShader(s->vert_shader);
	glGetShaderiv(s->vert_shader,GL_COMPILE_STATUS,&(s->gl_response));
	if (s->gl_response != GL_TRUE) std::cerr << "Something went wrong compiling the vertex shader." << std::endl;

	glCompileShader(s->frag_shader);
	glGetShaderiv(s->frag_shader,GL_COMPILE_STATUS,&(s->gl_response));
	if (s->gl_response != GL_TRUE) std::cerr << "Something went wrong compiling the fragment shader." << std::endl;

	s->program = glCreateProgram();

	glAttachShader(s->program,s->vert_shader);
	glAttachShader(s->program,s->frag_shader);

	glLinkProgram(s->program);
	glUseProgram(s->program);
}

void glshader::deleteShader(struct glshader::Shader *s) {
	glDeleteProgram(s->program);
	glDeleteShader(s->vert_shader);
	glDeleteShader(s->frag_shader);
}
