#pragma once

namespace glshader {
	struct Shader {
		GLuint program, vert_shader, frag_shader;
		int gl_response;
	};

	void newShader(struct Shader *s);
	void deleteShader(struct Shader *s);
}
