#version 450 core

in vec2 vLocalPos;
in float vSpeed;
out vec4 FragColor;

uniform float uRadius; // Радиус в локальных координатах (0..1)
uniform float uMaxSpeed; // Для нормализации 

void main()
{
    float dist = length(vLocalPos); // расстояние до центра
    if (dist > uRadius)
        discard;

     // нормализуем скорость в [0, 1]
     float t = clamp(vSpeed / uMaxSpeed, 0.0, 1.0);

     // простой градиент: синий → красный
     vec3 color = mix(vec3(0.2, 0.4, 1.0), vec3(0.88, 0.16, 0.16), t);

     FragColor = vec4(color, 1.0);

}