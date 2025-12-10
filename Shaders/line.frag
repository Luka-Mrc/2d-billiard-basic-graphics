#version 330 core

out vec4 outCol;

uniform vec3 uColor;
uniform float uAlpha;

void main()
{
    outCol = vec4(uColor, uAlpha);
}