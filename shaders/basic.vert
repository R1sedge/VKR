#version 450 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec4 aInstance; // x, y, radius, speed

uniform mat4 uView;
uniform mat4 uProj;

out vec2 vLocalPos;
out float vSpeed;

void main()
{
    vLocalPos = aPos.xy;

    vec2  worldPos = aInstance.xy;
    float radius = aInstance.z;
    vSpeed = aInstance.w;

    // Частицы пока в плоскости z=0.
    // Здесь появится billboard с uCameraRight/uCameraUp.
    vec2 scaled = aPos.xy * radius;
    vec4 world  = vec4(worldPos.x + scaled.x,
                       worldPos.y + scaled.y,
                       0.0, 1.0);

    gl_Position = uProj * uView * world;
}