varying vec3 normal;

void main()
{
	normal = normalize(gl_NormalMatrix * gl_Normal);

	gl_FrontColor = gl_Color;
	gl_BackColor = gl_Color;
	gl_TexCoord[0] = gl_MultiTexCoord0;

	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
}
