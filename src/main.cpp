#include <iostream>
#include <SDL2/SDL.h>
#include <GL/glew.h>
#include <GL/gl.h>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "shader.hpp"
#include "camera.hpp"
#include "texture.hpp"

#include "gltf.h"

#define WINWIDTH 1280
#define WINHEIGHT 960

#define BUFFER_OFFSET(offset) ((void *)(offset))

struct mesh {
	GLuint VAO; // vertex array object binding
	GLenum mode; // rendering mode
	GLsizei ecount; // element count
	GLenum etype; // element type for indices
	bool indexed;
};

struct mesh make_cubemap(void)
{
	struct mesh m = {0};
	m.ecount = 36;
	m.mode = GL_TRIANGLES;
	m.etype = GL_UNSIGNED_SHORT;
	m.indexed = true;

	const GLfloat VERTICES[] = {
	-1.0f, 1.0f, -1.0f, 1.0f,
	1.0f, 1.0f, -1.0f, 1.0f,
	-1.0f, 1.0f, 1.0f, 1.0f,
	1.0f, 1.0f, 1.0f, 1.0f,
	-1.0f, -1.0f, -1.0f, 1.0f,
	1.0f, -1.0f, -1.0f, 1.0f,
	-1.0f, -1.0f, 1.0f, 1.0f,
	1.0f, -1.0f, 1.0f, 1.0f,
	};

	const GLushort INDICES[] = {
	5, 1, 0, 4, 5, 0, 4, 0, 2, 6, 4, 2, 6, 2, 3, 7, 6, 3,
	2, 0, 1, 2, 1, 3, 6, 7, 4, 7, 5, 4, 5, 3, 1, 5, 7, 3,
	};

	glGenVertexArrays(1, &m.VAO);
	glBindVertexArray(m.VAO);

	GLuint ebo;
	GLuint vbo;

	glGenBuffers(1, &ebo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(INDICES), INDICES, GL_STATIC_DRAW);

	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(VERTICES), VERTICES, GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

	glBindVertexArray(0);
	glDisableVertexAttribArray(0);
	glDeleteBuffers(1, &vbo);
	glDeleteBuffers(1, &ebo);

	return m;
}

Shader base_shader(void)
{
	struct shaderinfo pipeline[] = {
		{GL_VERTEX_SHADER, "usr/shaders/basev.glsl"},
		{GL_FRAGMENT_SHADER, "usr/shaders/basef.glsl"},
		{GL_NONE, NULL}
	};

	Shader shader(pipeline);

	shader.bind();

	const float aspect = (float)WINWIDTH/(float)WINHEIGHT;
	glm::mat4 project = glm::perspective(glm::radians(90.f), aspect, 0.1f, 800.f);
	shader.uniform_mat4("project", project);

	glm::mat4 model = glm::mat4(1.f);
	shader.uniform_mat4("model", model);

	return shader;
}

Shader skybox_shader(void)
{
	struct shaderinfo pipeline[] = {
		{GL_VERTEX_SHADER, "usr/shaders/skyboxv.glsl"},
		{GL_FRAGMENT_SHADER, "usr/shaders/skyboxf.glsl"},
		{GL_NONE, NULL}
	};

	Shader shader(pipeline);

	shader.bind();

	const float aspect = (float)WINWIDTH/(float)WINHEIGHT;
	glm::mat4 project = glm::perspective(glm::radians(90.f), aspect, 0.1f, 200.f);
	shader.uniform_mat4("project", project);

	return shader;
}

void display_skybox(struct mesh skybox)
{
	glBindVertexArray(skybox.VAO);
	glDrawElements(skybox.mode, skybox.ecount, skybox.etype, NULL);
}

void render_loop(SDL_Window *window, std::string fpath)
{
	SDL_SetRelativeMouseMode(SDL_TRUE);

	gltf::Model testmodel;
	testmodel.importf(fpath, 0.1f);

	const char *cubemap_fpath[6] = {
	"media/textures/skybox/dust_ft.tga",
	"media/textures/skybox/dust_bk.tga",
	"media/textures/skybox/dust_up.tga",
	"media/textures/skybox/dust_dn.tga",
	"media/textures/skybox/dust_rt.tga",
	"media/textures/skybox/dust_lf.tga",
	};
	struct mesh cube = make_cubemap();
 	GLuint cubemap = load_cubemap_texture(cubemap_fpath);
	Shader skybox = skybox_shader();
	Shader shader = base_shader();
	Camera cam(glm::vec3(1.0, 1.0, 1.0));

	SDL_Event event;
	bool running = true;
	float start = 0.f;
	float end = 0.f;
	static float timer = 0.f;

	while (running == true) {
	// input and time measuring
		start = 0.001f * SDL_GetTicks();
		const float delta = start - end;
		while(SDL_PollEvent(&event));
		if (event.type == SDL_QUIT) { running = false; }

	// update states
		cam.update(delta);

		timer += delta;
		if (testmodel.animations.empty() == false) {
			if (timer > testmodel.animations[0].end) { timer -= testmodel.animations[0].end; }
			testmodel.updateAnimation(0, timer);
		}

	// rendering
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glm::mat4 view = cam.view();
		shader.uniform_mat4("view", view);
		shader.uniform_vec3("campos", cam.eye);

		shader.bind();
		testmodel.display(&shader);

		glDepthFunc(GL_LEQUAL);
		skybox.bind();
		skybox.uniform_mat4("view", view);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap);
		display_skybox(cube);
		glDepthFunc(GL_LESS);

		SDL_GL_SwapWindow(window);
		end = start;
	}
}

int main(int argc, char *argv[])
{
	SDL_Init(SDL_INIT_VIDEO);
	SDL_Window *window = SDL_CreateWindow("glTF viewer", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WINWIDTH, WINHEIGHT, SDL_WINDOW_OPENGL);

	if (window == NULL) {
		std::cerr << "error: could not create window: " << SDL_GetError() << std::endl;
		exit(EXIT_FAILURE);
	}

	SDL_GLContext glcontext = SDL_GL_CreateContext(window);
	if (glewInit()) {
		std::cerr << "error: unable to init glew: " << SDL_GetError() << std::endl;
		exit(EXIT_FAILURE);
	}
	glViewport(0, 0, WINWIDTH, WINHEIGHT);
	glClearColor(0.f, 0.f, 0.f, 0.f);
	glEnable(GL_DEPTH_TEST);

	render_loop(window, argv[1]);

	SDL_DestroyWindow(window);
	SDL_Quit();

	exit(EXIT_SUCCESS);
}
