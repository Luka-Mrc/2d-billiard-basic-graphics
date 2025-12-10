#version 330 core

out vec4 outCol;

uniform vec3 uColor;

void main()
{
    outCol = vec4(uColor, 1.0);
}