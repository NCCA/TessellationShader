#version 400
layout (location = 0) in vec3 inVert;
uniform mat4 MVP;

out vec3 vPosition;

void main()
{
		vPosition = inVert;
}
