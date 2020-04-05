#version 430 core

#define MAX_JOINT_MATRICES 128

layout(location = 0) in vec4 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec2 texcoord;
layout(location = 3) in ivec4 joints;
layout(location = 4) in vec4 weights;

uniform mat4 project, view, model;
uniform mat4 u_joint_matrix[MAX_JOINT_MATRICES];
uniform bool skinned;

out VERTEX {
	vec3 color;
	vec2 texcoord;
} vertex;

void main(void)
{
	if (skinned == true) {
		mat4 skin_matrix =
		weights.x * u_joint_matrix[int(joints.x)] +
		weights.y * u_joint_matrix[int(joints.y)] +
		weights.z * u_joint_matrix[int(joints.z)] +
		weights.w * u_joint_matrix[int(joints.w)];

		vertex.color = color;
		vertex.texcoord = texcoord;
		vec4 pos = view * model * skin_matrix * position;

		gl_Position = project * pos;
	} else {
		gl_Position = project * view * model * position;
	}
}
