#version 450 core

layout (location=0) in vec3 aPos;
layout (location=1) in vec4 aInstance; // x, y, radius, speed

uniform mat4 uProjection;

out vec2 vLocalPos; // Локальные координаты quad
out float vSpeed;

void main()
{
    vLocalPos = aPos.xy; // для круга во fragment shader

    float radius = aInstance.z;
    vec2 worldPos = aInstance.xy;
    vSpeed = aInstance.w;

    // Масштабируем локальные координаты квадрата и сдвигаем в world
    vec2 scaled = aPos.xy * radius;
    vec4 world = vec4(worldPos + scaled, 0.0, 1.0);

    gl_Position = uProjection * world;
}