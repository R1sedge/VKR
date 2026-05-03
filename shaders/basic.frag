#version 450 core

in vec2 vLocalPos;
in float vSpeed;
in float vPhase;
in float vRadius;
in vec3 vViewPos;

out vec4 FragColor;

uniform float uMaxSpeed;
uniform mat4 uProj;

uniform vec4 uPhase0Color;
uniform vec4 uPhase1Color;

// 0 = speed colormap, 1 = phase colors
uniform int uColorMode;

vec3 speedColorMap(float t)
{
    t = clamp(t, 0.0, 1.0);

    // blue -> cyan -> green -> yellow -> red
    vec3 c1 = vec3(0.0, 0.2, 1.0);
    vec3 c2 = vec3(0.0, 1.0, 1.0);
    vec3 c3 = vec3(0.0, 1.0, 0.2);
    vec3 c4 = vec3(1.0, 1.0, 0.0);
    vec3 c5 = vec3(1.0, 0.0, 0.0);

    if (t < 0.25)
        return mix(c1, c2, t / 0.25);
    else if (t < 0.50)
        return mix(c2, c3, (t - 0.25) / 0.25);
    else if (t < 0.75)
        return mix(c3, c4, (t - 0.50) / 0.25);
    else
        return mix(c4, c5, (t - 0.75) / 0.25);
}

void main()
{
    float d = length(vLocalPos);
    if (d > 1.0)
        discard;

    // Нормаль на поверхности сферы в локальном пространстве billboard-квада
    float zOffset = sqrt(1.0 - d * d);
    vec3 normal = normalize(vec3(vLocalPos, zOffset));

    // Корректная глубина: позиция точки на поверхности сферы в view-space
    vec3 viewSurfPos = vViewPos + vec3(0.0, 0.0, zOffset * vRadius);
    vec4 clipPos = uProj * vec4(viewSurfPos, 1.0);
    gl_FragDepth = (clipPos.z / clipPos.w) * 0.5 + 0.5;

    // Выбор базового цвета частицы
    vec3 baseColor;

    if (uColorMode == 1)
    {
        vec4 phaseColor = (vPhase > 0.5) ? uPhase1Color : uPhase0Color;
        baseColor = phaseColor.rgb;
    }
    else
    {
        float t = vSpeed / max(uMaxSpeed, 0.0001);
        baseColor = speedColorMap(t);
    }

    // Phong-освещение
    vec3 L = normalize(vec3(0.5, 1.0, 0.8));
    float diff = max(dot(normal, L), 0.0);

    vec3 R = reflect(-L, normal);
    vec3 V = normalize(-vViewPos);
    float spec = pow(max(dot(R, V), 0.0), 32.0);

    vec3 ambient = baseColor * 0.25;
    vec3 color = ambient + baseColor * diff + vec3(0.3) * spec;

    FragColor = vec4(color, 1.0);
}