#version 440 core

out vec4 fcolor;

in vec3 texcoords;

uniform samplerCube skybox;

void main()
{    
    fcolor = texture(skybox, texcoords);
}
