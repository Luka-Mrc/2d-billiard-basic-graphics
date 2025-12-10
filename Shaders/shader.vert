#version 330 core

layout(location = 0) in vec2 inPos;

uniform vec2 uPos;
uniform float uRadius;
uniform mat4 uProjection;

void main()
{
    vec2 scaledPos = inPos * uRadius;
    gl_Position = uProjection * vec4(scaledPos + uPos, 0.0, 1.0);
}