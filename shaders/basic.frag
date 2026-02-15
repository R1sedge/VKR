#version 450 core

in vec2 vLocalPos;
out vec4 FragColor;

uniform vec4 uColor;
uniform float uRadius; // радиус в локальных координатах (0..1)

void main()
{
    float dist = length(vLocalPos); // расстояние до центра
    
    if (dist > uRadius)
        discard;

    FragColor = uColor;
}