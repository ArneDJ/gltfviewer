#version 440 core

layout(location = 0) in vec4 position;

out vec3 texcoords;

uniform mat4 project, view;

void main()
{
	mat4 skybox_view = mat4(mat3(view));
	texcoords = vec3(position.x, position.y, position.z);

	vec4 pos = project * skybox_view * position;
	gl_Position = pos.xyww;
}  
