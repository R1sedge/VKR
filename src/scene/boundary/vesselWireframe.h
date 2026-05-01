#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <cstdint>
#include <cmath>

#include "BoundaryPlane.h"

struct VesselWireframe
{
    std::vector<glm::vec3> bodyVertices;   // вершины в body-space
    std::vector<uint32_t> lineIndices;     // пары индексов для GL_LINES

    bool empty() const
    {
        return bodyVertices.empty() || lineIndices.empty();
    }
};

// Добавляет линии прямоугольника перегородки и (опционально) круглого отверстия в существующий wireframe. 
inline void appendBaffleWireframe(VesselWireframe& wf, const InternalBoundaryPatch& patch, int circleSegments = 28)
{
    const uint32_t base = static_cast<uint32_t>(wf.bodyVertices.size());

    // 4 угла прямоугольника по часовой стрелке
    const glm::vec3 C0 = patch.point - patch.u * patch.halfWidth + patch.v * patch.halfHeight;
    const glm::vec3 C1 = patch.point + patch.u * patch.halfWidth + patch.v * patch.halfHeight;
    const glm::vec3 C2 = patch.point + patch.u * patch.halfWidth - patch.v * patch.halfHeight;
    const glm::vec3 C3 = patch.point - patch.u * patch.halfWidth - patch.v * patch.halfHeight;

    wf.bodyVertices.push_back(C0);
    wf.bodyVertices.push_back(C1);
    wf.bodyVertices.push_back(C2);
    wf.bodyVertices.push_back(C3);

    // 4 линии рамки
    wf.lineIndices.push_back(base + 0); wf.lineIndices.push_back(base + 1);
    wf.lineIndices.push_back(base + 1); wf.lineIndices.push_back(base + 2);
    wf.lineIndices.push_back(base + 2); wf.lineIndices.push_back(base + 3);
    wf.lineIndices.push_back(base + 3); wf.lineIndices.push_back(base + 0);

    // Круглое отверстие 
    if (patch.apertureType == InternalApertureType::Circle && patch.apertureRadius > 0.f)
    {
        const uint32_t circleBase = static_cast<uint32_t>(wf.bodyVertices.size());

        // Центр отверстия в мировых координатах
        const glm::vec3 center = patch.point + patch.u * patch.apertureCenter.x + patch.v * patch.apertureCenter.y;

        for (int i = 0; i < circleSegments; ++i)
        {
            const float angle = 2.f * 3.1415926535f * static_cast<float>(i) / static_cast<float>(circleSegments);
            const glm::vec3 p = center + patch.u * (std::cos(angle) * patch.apertureRadius) + patch.v * (std::sin(angle) * patch.apertureRadius);
            wf.bodyVertices.push_back(p);
        }

        // Линии окружности: каждый сегмент соединяет i и i+1 (последний замыкается на 0)
        for (int i = 0; i < circleSegments; ++i)
        {
            wf.lineIndices.push_back(circleBase + static_cast<uint32_t>(i));
            wf.lineIndices.push_back(circleBase + static_cast<uint32_t>((i + 1) % circleSegments));
        }
    }
}