#version 450 core

layout (location=0) in vec3 aPos;

uniform mat4 uModel;
uniform mat4 uProjection;

out vec2 vLocalPos; // Локальные координаты quad

void main()
{
    vLocalPos = aPos.xy;
    gl_Position = uProjection * uModel * vec4(aPos, 1);
}