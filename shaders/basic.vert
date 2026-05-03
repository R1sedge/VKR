#version 450 core

layout(location = 0) in vec3  aPos;
layout(location = 1) in vec4  aInstancePosPhase; // x, y, z, phase
layout(location = 2) in float aSpeed;

uniform mat4 uView;
uniform mat4 uProj;
uniform vec3 uCameraRight;
uniform vec3 uCameraUp;
uniform float uParticleRadius;

out vec2 vLocalPos;
out float vSpeed;
out float vPhase;
out float vRadius;
out vec3 vViewPos;

void main()
{
    vec3 worldPos = aInstancePosPhase.xyz;
    float radius   = uParticleRadius;
    vSpeed = aSpeed;
    vPhase = aInstancePosPhase.w;
    vRadius = radius;

    vec2 scaled = aPos.xy * radius;
    vec3 pos = worldPos + uCameraRight * scaled.x + uCameraUp * scaled.y;

    vec4 view = uView * vec4(pos, 1.0);
    vViewPos = view.xyz;
    vLocalPos = aPos.xy;
    gl_Position = uProj * view;
}