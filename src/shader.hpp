struct shaderinfo {
	GLenum type;
	const char *fpath;
	GLuint shader;
};

class Shader {
public:
	Shader(struct shaderinfo *shaders);
	void bind(void) { glUseProgram(program); }
	void uniform_mat4(const GLchar *name, glm::mat4 matrix) 
	{
		glUseProgram(program);
		glUniformMatrix4fv(glGetUniformLocation(program, name), 1, GL_FALSE, &matrix[0][0]);
	}
	void uniform_vec3(const GLchar *name, glm::vec3 vector) const
	{
		glUseProgram(program);
		glUniform3fv(glGetUniformLocation(program, name), 1, glm::value_ptr(vector));
	}
	void uniform_array_mat4(const GLchar *name, unsigned int count, glm::mat4 *matrices)
 	{
	glUseProgram(program);
	glUniformMatrix4fv(glGetUniformLocation(program, name), count, GL_FALSE, glm::value_ptr(matrices[0]));
 	}
	void uniform_bool(const GLchar *name, bool boolean) const
	{
	glUseProgram(program);
	glUniform1i(glGetUniformLocation(program, name), boolean);
	}


private:
	GLuint program;

	GLuint loadshaders(struct shaderinfo *shaders);
	GLuint substitute(void);
};
