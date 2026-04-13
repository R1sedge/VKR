#version 450 core

in vec2 vLocalPos;
in float vSpeed;
in float vRadius;
in vec3 vViewPos;
out vec4 FragColor;

uniform float uMaxSpeed;
uniform mat4 uProj;

void main()
{
    float d = length(vLocalPos);
    if (d > 1.0)
        discard;

    // Нормаль на поверхности сферы (в пространстве квада)
    float zOffset = sqrt(1.0 - d * d);
    vec3 normal = normalize(vec3(vLocalPos, zOffset));

    // Корректная глубина: позиция точки на поверхности сферы в view-space
    vec3 viewSurfPos = vViewPos + vec3(0.0, 0.0, zOffset * vRadius);
    vec4 clipPos = uProj * vec4(viewSurfPos, 1.0);
    gl_FragDepth = (clipPos.z / clipPos.w) * 0.5 + 0.5;

    // Phong-освещение
    vec3 L = normalize(vec3(0.5, 1.0, 0.8));
    float diff = max(dot(normal, L), 0.0);

    vec3 R = reflect(-L, normal);
    vec3 V = normalize(-vViewPos);
    float spec = pow(max(dot(R, V), 0.0), 32.0);

    // Цвет по скорости: синий → красный
    float t = clamp(vSpeed / uMaxSpeed, 0.0, 1.0);
    vec3 baseColor = mix(vec3(0.2, 0.4, 1.0), vec3(0.88, 0.16, 0.16), t);

    vec3 ambient = baseColor * 0.25;
    vec3 color = ambient + baseColor * diff + vec3(0.3) * spec;

    FragColor = vec4(color, 1.0);
}
